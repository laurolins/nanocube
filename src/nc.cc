#include <iostream>
#include <iomanip>
#include <thread>
#include <functional>
#include <fstream>
#include <mutex>

#include <zlib.h>

#include <boost/thread/shared_mutex.hpp>
#include <boost/asio.hpp>

#include "DumpFile.hh"
#include "MemoryUtil.hh"
#include "NanoCube.hh"
#include "Query.hh"
#include "QueryParser.hh"
#include "NanoCubeQueryResult.hh"
#include "NanoCubeSummary.hh"
#include "json.hh"

#include "util/signal.hh"

#include "util/timer.hh"

#include "tclap/CmdLine.h"

// query API 3
#include "address.hh"
#include "tile.hh"
#include "Server.hh"
#include "nanocube_language.hh"

#include "tree_store_nanocube.hh"

//
// geometry.hh and maps.hh
// are used just for the serveTile routine
// that deals with a hardcoded x:29bits, y:29bits, level: 6bits
// scheme of quadtree addresses on 64-bit unsigned integer
//
#include "geometry.hh"
#include "maps.hh"

#include "cache2.hh"

#include "polycover/polycover.hh"

using namespace nanocube;

//------------------------------------------------------------------------------
// Options
//------------------------------------------------------------------------------

struct Options {

    Options(std::vector<std::string>& args);

    TCLAP::CmdLine cmd_line { "Nanocube Leaf - local process", ' ', "2.3", true };

    // -s or --schema
    TCLAP::ValueArg<std::string> schema {  
            "s",              // flag
            "schema",         // name
            "Nanocube schema file (if not coming from stdin)", // description
            false,            // required
            "",               // value
            "schema-filename" // type description
            };

    // -d or --data
    TCLAP::ValueArg<std::string> data {
        "d",              // flag
        "data",         // name
        "Data coming from a file (if schema read header from schema)", // description
        false,            // required
        "",               // value
        "data-filename" // type description
    };

    TCLAP::ValueArg<int> query_port {
            "q",              // flag
            "query-port",     // name
            "Port for querying",     // description
            true,                                 // required
            0,                                    // value
            "query-port"                         // type description
            };

    TCLAP::ValueArg<int> insert_port {
            "i",                     // flag
            "insert-port",           // name
            "Port for inserting records via tcp",     // description
            false,                                    // required
            0,                                        // value
            "insert-port"                             // type description
            };

    TCLAP::ValueArg<int> no_mongoose_threads {  
            "t",              // flag
            "threads",        // name
            "Number of threads for querying (mongoose)",     // description
            false,                                 // required
            10,                                   // value
            "threads"                         // type description
            };
    
    // should be std::size_t
    TCLAP::ValueArg<int> max_points {
            "m",              // flag
            "max-points",        // name
            "Insert only max-points",     // description
            false,                                 // required
            0,                                   // value
            "max-points"                         // type description
            };

    // should be std::size_t
    TCLAP::ValueArg<int> report_frequency {
            "f",              // flag
            "report-frequency",        // name
            "Report a line when inserting",     // description
            false,                                 // required
            100000,                                   // value
            "report-frequency"                         // type description
            };


    // should be std::size_t
    TCLAP::ValueArg<int> batch_size {
            "b",              // flag
            "batch-size",        // name
            "Batch Size (default: 1)",     // description
            false,                                 // required
            1,                                   // value
            "batch-size"                         // type description
            };

    // should be std::size_t
    TCLAP::ValueArg<int> sleep_for_ns {
            "y",              // flag
            "sleep",        // name
            "sleep (default: 0ns)",     // description
            false,                                 // required
            0,                                   // value
            "sleep"                         // type description
            };


    // -d or --data
    TCLAP::ValueArg<int> mask_cache_budget {
        "M",              // flag
        "mask-cache-budget",         // name
        "Number of masks that are kept in the cache. Every time we have 20% more masks in the cache we reduce it to the budget. Default 1000.", // description
        false,            // required
        1000,             // value
        "data-filename"   // type description
    };

    TCLAP::ValueArg<int> sliding {
        "w",                      // flag
        "sliding-window",                // name
        "Sliding Window",         // description
        false,                    // required
        0,                        // value
        "sliding window units"    // type description
    };
    


};


Options::Options(std::vector<std::string>& args)
{
    cmd_line.add(schema); // add command option
    cmd_line.add(data);
    cmd_line.add(query_port);
    cmd_line.add(insert_port);
    cmd_line.add(no_mongoose_threads);
    cmd_line.add(max_points);
    cmd_line.add(report_frequency);
    cmd_line.add(batch_size);
    cmd_line.add(sleep_for_ns);
    cmd_line.add(mask_cache_budget);
    cmd_line.add(sliding);
    cmd_line.parse(args);
}


//------------------------------------------------------------------------------
// zlib_compress
//------------------------------------------------------------------------------

std::vector<char> zlib_compress(const char* ptr, std::size_t size)
{
    std::vector<char> result;

    uLongf uncompressed_size = size;             // initially this is an upper bound
    uLongf compressed_size   = compressBound(size); // initially this is an upper bound

    result.resize(compressed_size + 8); // reserve 8 bytes for the original size plus
                                    // enough bytes to fit compressed data

    // write uncompressed size in first 8 bytes of stream
    *reinterpret_cast<uint64_t*>(&result[0]) = (uint64_t) uncompressed_size;

    const Bytef* uncompressed_ptr = reinterpret_cast<const Bytef*>(ptr);
    Bytef*       compressed_ptr   = reinterpret_cast<Bytef*>(&result[8]);

    int status = compress(compressed_ptr, &compressed_size, uncompressed_ptr, uncompressed_size);

    result.resize(compressed_size + 8); // pack

    if (status != Z_OK) {
        throw std::runtime_error("zlib error while compressing");
    }

    return result;
}

//------------------------------------------------------------------------------
// zlib_compress
//------------------------------------------------------------------------------

std::vector<char> zlib_decompress(const char* ptr, std::size_t size)
{
    std::vector<char> result;

    uint64_t aux_uncompressed_size = *reinterpret_cast<const uint64_t*>(ptr);
    uLongf uncompressed_size = (uLongf) aux_uncompressed_size;
    uLongf compressed_size   = size - 8; //

    result.resize(uncompressed_size);  // reserve 8 bytes for the original size

    const Bytef* compressed_ptr   = reinterpret_cast<const Bytef*>(ptr + 8);
    Bytef*       uncompressed_ptr = reinterpret_cast<Bytef*>(&result[0]);

    int status = uncompress(uncompressed_ptr, &uncompressed_size, compressed_ptr, compressed_size);

    // result.resize(output_size + 8);

    if (status != Z_OK) {
        throw std::runtime_error("zlib error while compressing");
    }

    return result;
}

//-----------------------------------------------------------------------------
// Split routines
//-----------------------------------------------------------------------------

std::vector<std::string> &split(const std::string &s, char delim,
                                std::vector<std::string> &elems)
{
    std::stringstream ss(s);
    std::string item;
    while(std::getline(ss, item, delim)) {
         elems.push_back(item);
    }
    return elems;
}


//------------------------------------------------------------------------------
// NanoCube
//------------------------------------------------------------------------------

typedef nanocube::NanoCubeTemplate<
    boost::mpl::vector<LIST_DIMENSION_NAMES>,
    boost::mpl::vector<LIST_VARIABLE_TYPES>> NanoCube;

using Entry   = typename NanoCube::entry_type;
using Address = typename NanoCube::address_type;

using MaskCache = cache2::Cache<std::string, ::query::Mask>;


//-----------------------------------------------------------------
// AnnotatedSchema
//-----------------------------------------------------------------

struct AnnotatedSchema {
public:
    enum DimensionType { QUADTREE, CATEGORICAL, TIME };
    AnnotatedSchema(Schema &schema);
    
    
    DimensionType dimType(int dimension_index) const;
    int dimSize(int dimension_index) const;
    
    ::nanocube::DimAddress convertRawAddress(int dimension_index, std::size_t raw_address);
    std::size_t convertPathAddress(int dimension_index, DimAddress &path);
    
public:
    Schema& schema;
    std::vector<DimensionType> dimension_types;
    std::vector<int>           dimension_sizes; // interpretation depends on type
};

//-----------------------------------------------------------------
// AnnotatedSchema Impl.
//-----------------------------------------------------------------

AnnotatedSchema::AnnotatedSchema(Schema &schema):
schema(schema)
{
    for (auto field: schema.dump_file_description.fields) {
        auto type_name = field->field_type.name;
        if (type_name.find("nc_dim_quadtree") != std::string::npos) {
            dimension_types.push_back(QUADTREE);
            auto st = type_name.substr(std::string("nc_dim_quadtree_").size());
            dimension_sizes.push_back(std::stoi(st)); // num levels
        }
        else if (type_name.find("nc_dim_cat") != std::string::npos) {
            dimension_types.push_back(CATEGORICAL);
            auto st = field->field_type.name.substr(std::string("nc_dim_cat_").size());
            dimension_sizes.push_back(std::stoi(st)); // num bytes
        }
        else if (type_name.find("nc_dim_time") != std::string::npos) {
            dimension_types.push_back(TIME);
            auto st = field->field_type.name.substr(std::string("nc_dim_time_").size());
            dimension_sizes.push_back(std::stoi(st)); // num bytes
        }
        else if (type_name.find("nc_var") != std::string::npos) {
            break;
        }
        else throw std::runtime_error("oooops");
    }
}

auto AnnotatedSchema::dimType(int dimension_index) const -> DimensionType {
    return dimension_types.at(dimension_index);
}

int AnnotatedSchema::dimSize(int dimension_index) const {
    return dimension_sizes.at(dimension_index);
}

::nanocube::DimAddress AnnotatedSchema::convertRawAddress(int dimension_index, std::size_t raw_address) {
    
    auto dimension_type = dimension_types.at(dimension_index);
    auto dimension_size = dimension_sizes.at(dimension_index);
    
    DimAddress result;
    
    if (dimension_type == QUADTREE) {
        uint64_t level = ( raw_address             ) >> (64-6);
        uint64_t y     = ( raw_address << 6        ) >> (64-29);
        uint64_t x     = ( raw_address << (6 + 29) ) >> (64-29);
        nanocube::Tile tile((int) x, (int) y, (int) level);
        result = tile.toAddress();
    }
    else if (dimension_type == AnnotatedSchema::CATEGORICAL) {
        auto root_address = (1ULL << (dimension_size * 8)) - 1;
        if (raw_address == root_address) {
            result = DimAddress(); // empty path (root!)
        }
        else {
            result = { (Label) raw_address }; // empty path (root!)
        }
    }
    else if (dimension_type == AnnotatedSchema::TIME) {
        // binary representation of the raw address using dimension_size bytes (8 bits, 16 bits)
    }
    
    return result;
    
}

std::size_t AnnotatedSchema::convertPathAddress(int dimension_index, DimAddress &path) {
    auto dimension_type = dimension_types.at(dimension_index);
    auto dimension_size = dimension_sizes.at(dimension_index);
    
    uint64_t result = 0;
    
    if (dimension_type == AnnotatedSchema::QUADTREE) {
        nanocube::Tile tile(path);
        uint64_t x = tile.x;
        uint64_t y = tile.y;
        uint64_t z = tile.level;
        result = (z << 58) | (y << 29) | x;
    }
    else if (dimension_type == AnnotatedSchema::CATEGORICAL) {
        if (path.size() == 0) {
            result = (1ULL << (dimension_size * 8)) - 1;
        }
        else if (path.size() == 1) {
            result = path[0];
        }
        else {
            throw std::runtime_error("invalid address for categorical dimension");
        }
    }
    return result;
}

//
// API_3
//

enum OutputEncoding { JSON, BINARY, TEXT };

struct BranchTargetOnTime {
public:
    BranchTargetOnTime() = default;
    BranchTargetOnTime(int base, int width, int count):
    active(true),
    base(base),
    width(width),
    count(count)
    {}
public:
    bool active { false };
    int base  { 0 };
    int width { 0 };
    int count { 0 };
};

struct FormatOption {
    enum Type { NORMAL, RELATIVE_IMAGE };
    Type       type { NORMAL };
    DimAddress base_address;
};

//------------------------------------------------------------------------------
// NanocubeServer
//------------------------------------------------------------------------------

struct NanocubeServer {

public: // Constructor

    NanocubeServer(::nanocube::Schema &schema, Options &options, std::istream &input_stream);

private: // Private Methods

    void parse(std::string              query_st,
               ::nanocube::Schema        &schema,
               ::query::QueryDescription &query_description);
    
    void initializeQueryServer();
    void runQueryServer();
    void stopQueryServer();

public: // Public Methods
    
    void serveQuery(Request &request, ::nanocube::lang::Program &program);
    // void serveQuery     (Request &request, bool json, bool compression);

    void serveTimeQuery (Request &request, bool json, bool compression);
    void serveTile      (Request &request);
    void serveSchema    (Request &request, bool json);
    void serveSetValname(Request &request);
    void serveTiming    (Request &request);
    void serveVersion   (Request &request);
//    void serveShutdown  (Request &request);

public:

    void insert_from_stdin();
    void insert_from_tcp();
    
    void addMessage(std::string s);
    void printMessages();
    
    void cacheMask(const std::string& key, ::query::Mask* mask);
    
private:
    
    void parse_program_into_query(const ::nanocube::lang::Program &program,
                                  AnnotatedSchema           &annotated_schema,
                                  ::query::QueryDescription &query_description,
                                  OutputEncoding &output_encoding,
                                  BranchTargetOnTime &branch_target_on_time,
                                  std::vector<FormatOption> &format_options);


public: // Data Members

    std::mutex messages_mutex;
    std::vector<std::string> messages;
    
    std::uint64_t inserted_points { 0 };
    
    // when sliding window is active, keep rotating on this 3 element array
    std::unique_ptr<NanoCube> nanocubes[3];

    int active_window { 0 };
    bool sliding { false };
    struct {
        bool      empty                { true };
        uint64_t  size                 { 0 };
        uint64_t  active_window_t0     { 0 };
        uint64_t  previous_window_t0   { 0 };
        int       time_record_offset   { 0 };
        int       time_record_bytes    { 0 };
    } sliding_window;
    
    Schema       &schema;
    Options      &options;
    std::istream &input_stream;

    Server        server;
    
    boost::asio::io_service io_service; // io_service for tcp insert port (clean this up)

    bool                      finish { false };

    boost::shared_mutex       shared_mutex; // one writer multiple readers
    
    MaskCache mask_cache;

};

void NanocubeServer::addMessage(std::string s) {
    std::lock_guard<std::mutex> lock(messages_mutex);
    messages.push_back(s);
}

void NanocubeServer::printMessages() {
    std::lock_guard<std::mutex> lock(messages_mutex);
    for (auto &s: messages) {
        std::cerr << s;
    }
    messages.clear();
}

//------------------------------------------------------------------------------
// NanocubeServer Impl.
//------------------------------------------------------------------------------

NanocubeServer::NanocubeServer(Schema &schema, Options &options, std::istream &input_stream):
    schema(schema),
    options(options),
    input_stream(input_stream),
    finish(false)
{
    // create one active nanocube (if not sliding window, it will be the only nanocube)
    nanocubes[active_window].reset(new NanoCube(schema));

    sliding_window.size = (uint64_t) options.sliding.getValue();
    sliding = sliding_window.size > 0;
    
    if (sliding) {
        // store location of timestamp on input records on the sliding window info
        for (auto field: schema.dump_file_description.fields) {
            if (field->field_type.name.find("nc_dim_time_") == 0) {
                sliding_window.time_record_offset = field->offset_inside_record;
                sliding_window.time_record_bytes  = field->getNumBytes();
            }
        }
    }
    
    try {
        initializeQueryServer();

        // initial message
        std::stringstream ss;
        ss << "query-port: " << options.query_port.getValue() << std::endl;
        // << "insert-port: " <<  options.insert_port.getValue() << std::endl; // disabled
        addMessage(ss.str());
    }
    catch (ServerException &e) {
        finish = true;
        std::cerr << "[PROBLEM] Could not bind query-port:  " <<  server.port << std::endl;
    }

    if (!finish) {
        
        // start threads for serving queries (uses mongoose)
        std::thread http_server(&NanocubeServer::runQueryServer, this);
        
        // start thread to insert records coming from stdin
        std::thread insert_from_stdin_thread(&NanocubeServer::insert_from_stdin, this);
        
        // start thread to insert records coming from tcp port (if one was defined)
        std::thread insert_from_tcp_thread(&NanocubeServer::insert_from_tcp, this);
        
        while (!finish) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            printMessages();
        }
        
        printMessages();
        
        // TODO: move this somewhere else
        io_service.stop();
        
        insert_from_stdin_thread.join();
        insert_from_tcp_thread.join();
        
        stopQueryServer();
        http_server.join();
        
    }

}

void NanocubeServer::stopQueryServer()
{
    server.stop();
}

void NanocubeServer::initializeQueryServer()
{
    
    // initialize query server
    server.port = options.query_port.getValue();
    
    auto &nc_server = *this;
    
    using Handler = std::function<void(Request& request, ::nanocube::lang::Program &program)>;
    
    std::map<std::string, Handler> handlers;
    
    // schema handler
    handlers["schema"] = [&nc_server](Request& request, ::nanocube::lang::Program &program) {
        bool json = true;
        if (program.findCallByName("text"))
            json = false;
        nc_server.serveSchema(request, json);
    };
    
    // topk handler
    handlers["topk"] = [&nc_server](Request& request, ::nanocube::lang::Program &program) {
        // nc_server.serveQuery(request, program, ::collector_heap::TOPK);
    };
    
    // topk handler
    handlers["unique"] = [&nc_server](Request& request, ::nanocube::lang::Program &program) {
        // nc_server.serveQuery(request, program, ::collector_heap::UNIQUE_COUNT);
    };
    
    // topk handler
    handlers["count"] = [&nc_server](Request& request, ::nanocube::lang::Program &program) {
        nc_server.serveQuery(request, program);
    };

    // timing handler
    handlers["timing"] = [&nc_server](Request& request, ::nanocube::lang::Program &program) {
        nc_server.serveTiming(request);
    };

    // version handler
    handlers["version"] = [&nc_server](Request& request, ::nanocube::lang::Program &program) {
        nc_server.serveVersion(request);
    };

    // shutdown handler
    // handlers["shutdown"] = [&nc_server](Request& request, ::nanocube::lang::Program &program) {
    //     nc_server.serveShutdown(request);
    // };
    
    // topk handler
    handlers["words"] = [&nc_server](Request& request, ::nanocube::lang::Program &program) {
        // nc_server.serveWords(request, program);
    };
    
    // topk handler
    handlers["ids"] = [&nc_server](Request& request, ::nanocube::lang::Program &program) {
        // nc_server.serveIds(request, program);
    };
    
    
    // std::string command("query.a(0,find([],2)).a(1,find([],1)).a(2,find([],1))");
    auto handler = [&nc_server, handlers](Request &request) {
        try {
            //
            ::nanocube::lang::Parser<std::string::const_iterator> parser;
            parser.parse(request.request_string.begin(), request.request_string.end());
            
            // route program to right handler based on the program name
            ::nanocube::lang::Program &program = *parser.program;
            
            auto it = handlers.find(program.name);
            if (it == handlers.end()) {
                request.respondText("error");
            }
            else {
                it->second(request, program);
            }
        }
        catch (...) {
            // request.respond
        }
    };
    
    server.setHandler(handler);
    
    server.init(options.no_mongoose_threads.getValue());
}

void NanocubeServer::runQueryServer()
{
    server.run();
}

//
// http://stackoverflow.com/questions/13059091/creating-an-input-stream-from-constant-memory
//
struct membuf: std::streambuf {
    membuf(char const* base, size_t size) {
        char* p(const_cast<char*>(base));
        this->setg(p, p, p + size);
    }
};

struct imemstream: virtual membuf, std::istream {
    imemstream(char const* base, size_t size)
    : membuf(base, size)
    , std::istream(static_cast<std::streambuf*>(this)) {
    }
};

void NanocubeServer::insert_from_stdin()
{
    auto batch_size       = options.batch_size.getValue(); // add 10k points before
    auto report_frequency = options.report_frequency.getValue();
    auto maximum          = options.max_points.getValue();
    
    if (maximum  && batch_size > maximum) {
        batch_size = maximum;
        std::cerr << "[Warning] (stdin) Maximum records is less than the batch size." << std::endl
        << "[Warning]         Reducing the batch size to match maximum records of "
        << batch_size << std::endl;
    }
    
    if (maximum  && report_frequency > maximum) {
        report_frequency = maximum;
        std::cerr << "[Warning] (stdin) Maximum records is less than the report frequency." << std::endl
        << "[Warning]         Reducing the report frequency to match maximum records of "
        << report_frequency << std::endl;
    }

    if (report_frequency && batch_size > report_frequency) {
        batch_size = report_frequency;
        std::cerr << "[Warning] (stdin) Report frequency less than the batch size." << std::endl
                  << "[Warning]         Reducing the batch size to match report frequency of "
            << batch_size << std::endl;
    }

    auto adjusted_report_frequency = (decltype(report_frequency)) (ceil(report_frequency / (1.0 * batch_size)) * batch_size);
    if (adjusted_report_frequency != report_frequency) {
        report_frequency = adjusted_report_frequency;
        std::cerr << "[Warning] (stdin) Report frequency needs to be multiple of " << std::endl
                  << "[Warning]         batch size. It was redefined as " << report_frequency << std::endl;
    }
    
    stopwatch::Stopwatch sw;
    sw.start();

    // std::cout << "reading data..." << std::endl;

    bool done = false;

    auto record_size = schema.dump_file_description.record_size;
    auto num_bytes_per_batch = record_size * batch_size;
    
    std::stringstream ss;
    std::vector<char> buffer(num_bytes_per_batch);

    // {
    //     std::ofstream ofs("/tmp/verification.tmp", std::ofstream::out | std::ofstream::app);
    //     ofs << "open verification.tmp, num bytes per batch: " << num_bytes_per_batch << std::endl;
    // }
    
    while (!done) {

        //std::cerr << "reading " << num_bytes_per_batch << "...";
        input_stream.read(&buffer[0],num_bytes_per_batch);
        
        //std::cerr << " gcout..." << std::endl;
        auto read_bytes = input_stream.gcount();
        //std::cerr << " " << read_bytes << " were read" << std::endl;
        
        // write a batch of points
        if (read_bytes > 0)
        {
            imemstream ss(&buffer[0], read_bytes);
            boost::unique_lock<boost::shared_mutex> lock(shared_mutex);
            for (uint64_t i=0;i<batch_size && !done;++i)
            {
                // std::cout << i << std::endl;
                bool ok = true;

                if (!sliding) {
                    ok = nanocubes[active_window].get()->add(ss);
                }
                else {
                    // get timestamp from record
                    char *ptr = &buffer[0] + i * record_size + sliding_window.time_record_offset;
                    std::uint64_t timestamp = 0;
                    std::copy(ptr, ptr + sliding_window.time_record_bytes, (char*) &timestamp);
                    
                    if (sliding_window.empty) {
                        sliding_window.active_window_t0 = timestamp;
                        sliding_window.empty = false;
                        nanocubes[active_window].get()->add(ss);
                    }
                    else {
                        // rotate windows
                        int previous_window = active_window == 0 ? 2 : active_window - 1;
                        int next_window     = active_window == 2 ? 0 : active_window + 1;

                        if (timestamp >= sliding_window.active_window_t0 + sliding_window.size) {
                            
                            // new active window

                            // TODO: turn the deletion of the previous window to
                            // another thread. for now let it rest there
                            
                            nanocubes[next_window].reset(new NanoCube(schema));
                            nanocubes[previous_window].reset(); // erase previous window
                            
                            active_window = next_window;
                            
                            sliding_window.previous_window_t0 = sliding_window.active_window_t0;
                            sliding_window.active_window_t0   = timestamp;
                            
                            ok = nanocubes[active_window].get()->add(ss);

                            std::stringstream ss;
                            ss << "(stdin     ) switching to new window" << std::endl;
                            addMessage(ss.str());

                            
                        }
                        else if (timestamp >= sliding_window.active_window_t0) {

                            ok = nanocubes[active_window].get()->add(ss);

                        }
                        else if (timestamp >= sliding_window.previous_window_t0 && timestamp < sliding_window.previous_window_t0 + sliding_window.size) {
                            ok = nanocubes[previous_window].get()->add(ss);
                        }
                    }
                }
                
                if (!ok) {
                    // std::cout << "not ok" << std::endl;
                    done = true;
                }
                else {
                    // std::cout << "ok" << std::endl;
                    ++inserted_points;
                    done = (maximum && inserted_points == maximum);
                }
            }
        }
        else if (read_bytes == 0) {
            break;
        }

        // shared_mutex.
        if (options.sleep_for_ns.getValue() == 0) {
            std::this_thread::yield(); // if there is a query it should wake up
        }
        else {
            std::this_thread::sleep_for(std::chrono::nanoseconds(options.sleep_for_ns.getValue()));
        }

        // make sure report frequency is a multiple of batch size
        if (inserted_points % report_frequency == 0) {
            std::stringstream ss;
            ss << "(stdin     ) count: " << std::setw(10) << inserted_points
            << " mem. res: " << std::setw(10) << memory_util::MemInfo::get().res_MB() << "MB."
            << " time(s): " <<  std::setw(10) << sw.timeInSeconds() << std::endl;
            addMessage(ss.str());
        }

    } // loop to insert objects into the nanocube


    if (inserted_points > 0) {
        std::stringstream ss;
        ss << "(stdin:done) count: " << std::setw(10) << inserted_points
        << " mem. res: " << std::setw(10) << memory_util::MemInfo::get().res_MB() << "MB."
        << " time(s): " <<  std::setw(10) << sw.timeInSeconds() << std::endl;
        addMessage(ss.str());
    }
}


void NanocubeServer::insert_from_tcp()
{
#if 0
    using Socket   = boost::asio::ip::tcp::socket;
    using Acceptor = boost::asio::ip::tcp::acceptor;
    using boost::asio::ip::tcp;

    auto port = options.insert_port.getValue();
    if (port== 0)
        return;

    try {
        
#if 1
        
        // using async_accept(...)

        auto process_incoming_data = [&](Socket &socket, Acceptor& acceptor) {
          
            while (true) {
//                {
//                    std::stringstream ss;
//                    ss << "(socket) established connection" << std::endl;
//                    addMessage(ss.str());
//                }
                
                stopwatch::Stopwatch sw;
                sw.start();
                
                bool done = false;
                
                auto record_size = nanocube.schema.dump_file_description.record_size;
                // std::size_t record[record_size];
                
                std::size_t bytes_on_stream = 0;
                
                std::stringstream ss;
                
                bool eof = false;
                
                char buf[1024];
                while (!done && !finish)
                {
                    boost::system::error_code error;
                    
                    std::size_t len = socket.read_some(boost::asio::buffer(buf), error);
                    
                    if (error == boost::asio::error::eof) {
                        // addMessage("socket.read_some: EOF... restarting");
                        // restart = true; // wait for a new connection on the same port
                        // io_service.stop();
                        eof = true;
                        break; // Connection closed cleanly by peer.
                    }
                    else if (error) {
                        // addMessage("socket.read_some: NOT EOF... not restarting");
                        // restart = false;
                        throw boost::system::system_error(error); // Some other error.
                    }
                    
                    
                    // TODO: copy m
                    ss.write(buf, len);
                    // ss.flush();
                    bytes_on_stream += len;
                    
                    //        while (bytes_on_stream > record_size) {
                    //            ss.read(&record[0], record_size);
                    //
                    //            for (int i=0;i<record_size;++i) {
                    //                unsigned char value = record[i];
                    //                std::cout << " " << std::hex << (int) value << std::dec;
                    //            }
                    //            std::cout << std::endl;
                    //
                    //            nanocube.schema.dump_file_description.writeRecordTextualDescription(std::cout, &record[0], record_size);
                    //            std::cout << std::endl;
                    //            bytes_on_stream -= record_size;
                    //        }
                    
                    
                    //        std::cout << "there are " << bytes_on_stream << " bytes on the stream" << std::endl;
                    //        unsigned char ch;
                    //        int i=0;
                    //        while (bytes_on_stream > 0) {
                    //            // ch = buf[i];
                    //            ss.read((char*) &ch,1);
                    //            std::cout << " " << std::hex << (int) ch << std::dec;
                    //            --bytes_on_stream;
                    //            ++i;
                    //        }
                    //        std::cout << std::endl;
                    
                    auto num_records_to_add = bytes_on_stream / record_size;
                    if (num_records_to_add > 0) {
                        boost::unique_lock<boost::shared_mutex> lock(shared_mutex);
                        for (std::size_t k=0;k<num_records_to_add;++k) {
                            bool ok = nanocube.add(ss);
                            if (!ok) {
                                done = true;
                                break;
                            }
                            ++inserted_points;
                            // make sure report frequency is a multiple of batch size
                            if ((inserted_points % options.report_frequency.getValue()) == 0) {
                                std::stringstream ss;
                                ss << "(tcp) count: " << std::setw(10) << inserted_points
                                << " mem. res: " << std::setw(10) << memory_util::MemInfo::get().res_MB() << "MB."
                                << " time(s): " <<  std::setw(10) << sw.timeInSeconds() << std::endl;
                                addMessage(ss.str());
                            }
                            bytes_on_stream -= record_size;
                            
                        }
                        
                    }
                    
                }
                
                if (eof) {
                    socket.close();
//                    std::stringstream ss;
//                    ss << "(socket) eof received, entering accept(...) to wait for new connection on the same port" << std::endl;
//                    addMessage(ss.str());
                    acceptor.accept(socket);
                }
            }
        };
        
        using boost::asio::ip::tcp;

        Acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), port));
        Socket   socket(io_service);

        auto handler = [&](const boost::system::error_code& ec) {
            if (ec)
                return;
            
            try {
                process_incoming_data(socket, acceptor);
            }
            catch(...) {
                finish = true;
            }
        };
        
        acceptor.async_accept(socket, handler);
        io_service.run();
        
#else

        // simple method without a clean exit mechanism while
        // accept(...) socket connection
        
        while (true) {
            
            Acceptor acceptor(io_service, tcp::endpoint(tcp::v4(), port));
            Socket   socket(io_service);
            acceptor.accept(socket);
            
//            {
//                std::stringstream ss;
//                ss << "(socket) established connection" << std::endl;
//                addMessage(ss.str());
//            }
            
            stopwatch::Stopwatch sw;
            sw.start();
            
            bool done = false;
            
            auto record_size = nanocube.schema.dump_file_description.record_size;
            // std::size_t record[record_size];
            
            std::size_t bytes_on_stream = 0;
            
            std::stringstream ss;
            
            bool eof = false;
            
            char buf[1024];
            while (!done && !finish)
            {
                boost::system::error_code error;
                
                std::size_t len = socket.read_some(boost::asio::buffer(buf), error);
                
                if (error == boost::asio::error::eof) {
                    // addMessage("socket.read_some: EOF... restarting");
                    // restart = true; // wait for a new connection on the same port
                    // io_service.stop();
                    eof = true;
                    break; // Connection closed cleanly by peer.
                }
                else if (error) {
                    // addMessage("socket.read_some: NOT EOF... not restarting");
                    // restart = false;
                    throw boost::system::system_error(error); // Some other error.
                }
                
                ss.write(buf, len);
                // ss.flush();
                bytes_on_stream += len;
                
                //        while (bytes_on_stream > record_size) {
                //            ss.read(&record[0], record_size);
                //
                //            for (int i=0;i<record_size;++i) {
                //                unsigned char value = record[i];
                //                std::cout << " " << std::hex << (int) value << std::dec;
                //            }
                //            std::cout << std::endl;
                //
                //            nanocube.schema.dump_file_description.writeRecordTextualDescription(std::cout, &record[0], record_size);
                //            std::cout << std::endl;
                //            bytes_on_stream -= record_size;
                //        }
                
                
                //        std::cout << "there are " << bytes_on_stream << " bytes on the stream" << std::endl;
                //        unsigned char ch;
                //        int i=0;
                //        while (bytes_on_stream > 0) {
                //            // ch = buf[i];
                //            ss.read((char*) &ch,1);
                //            std::cout << " " << std::hex << (int) ch << std::dec;
                //            --bytes_on_stream;
                //            ++i;
                //        }
                //        std::cout << std::endl;
                
                auto num_records_to_add = bytes_on_stream / record_size;
                if (num_records_to_add > 0) {
                    boost::unique_lock<boost::shared_mutex> lock(shared_mutex);
                    for (std::size_t k=0;k<num_records_to_add;++k) {
                        bool ok = nanocube.add(ss);
                        if (!ok) {
                            done = true;
                            break;
                        }
                        ++inserted_points;
                        // make sure report frequency is a multiple of batch size
                        if ((inserted_points % options.report_frequency.getValue()) == 0) {
                            std::stringstream ss;
                            ss << "(tcp) count: " << std::setw(10) << inserted_points
                            << " mem. res: " << std::setw(10) << memory_util::MemInfo::get().res_MB() << "MB."
                            << " time(s): " <<  std::setw(10) << sw.timeInSeconds() << std::endl;
                            addMessage(ss.str());
                        }
                        bytes_on_stream -= record_size;
                        
                    }
                    
                }
                
            }
            
//            if (eof) {
//                std::stringstream ss;
//                ss << "(socket) eof received, entering accept(...) to wait for new connection on the same port" << std::endl;
//                addMessage(ss.str());
//            }
        }
        
#endif
        
        
    }
    catch(std::exception &e) {
        std::stringstream ss;
        ss << "Problem binding insert-port: " << port << std::endl;
        addMessage(ss.str());
        finish = true; // turn of problem flag
    }
#endif
}

#if 0
void NanocubeServer::parse(std::string              query_st,
                           ::nanocube::Schema        &schema,
                           ::query::QueryDescription &query_description)
{
    ::query::parser::QueryParser parser;
    parser.parse(query_st);

    // std::cout << ::query::parser::Print(parser) << std::endl;

    for (::query::parser::Dimension *dimension: parser.dimensions) {
        try {
            int index = schema.getDimensionIndex(dimension->name);
            // std::cout << "Index of dimension " << dimension->name << " is " << index << std::endl;

            bool anchored = dimension->anchored;

            query_description.setAnchor(index, anchored);

            ::query::parser::TargetExpression *target = dimension->target;


            target->updateQueryDescription(index, query_description);


            RawAddress replacement = 0ULL;
            { // replace raw addresses ~0ULL;
                auto field    = schema.dump_file_description.getFieldByName(dimension->name);
                // if it is a categorical field replacement will be 2^(bytes * 8) - 1;
                if (field->field_type.name.find("nc_dim_cat_") == 0) {
                    replacement = (1ULL << (field->getNumBytes() * 8)) - 1;
                }
            }
            
            query_description.targets[index]->replace(~0ULL, replacement);
            

        }
        catch(dumpfile::DumpFileException &e) {
            std::cout << "(Warning) Dimension " << dimension->name << " not found. Disconsidering it." << std::endl;
            continue;
        }
    }
}
#endif

// void NanocubeServer::serveQuery(Request &request, bool json, bool compression)


void NanocubeServer::cacheMask(const std::string& key, ::query::Mask* mask)
{
    this->mask_cache.insert(key,mask);
    if (mask_cache.size() > (std::size_t)(1.2 * options.mask_cache_budget.getValue())) {
        mask_cache.enforce_budget();
    }
}

void NanocubeServer::parse_program_into_query(const ::nanocube::lang::Program &program,
                                              AnnotatedSchema           &annotated_schema,
                                              ::query::QueryDescription &query_description,
                                              OutputEncoding &output_encoding,
                                              BranchTargetOnTime &branch_target_on_time,
                                              std::vector<FormatOption> &format_options)
{
    // default values
    output_encoding       = JSON;
    branch_target_on_time.active = false;
    
    auto &that = *this;
    
    //
    using Node   = ::nanocube::lang::Node;
    using Number = ::nanocube::lang::Number;
    using String = ::nanocube::lang::String;
    using Call   = ::nanocube::lang::Call;
    using List   = ::nanocube::lang::List;
    // using Program = ::nanocube::lang::Program;
    
    const auto NUMBER = ::nanocube::lang::NUMBER;
    const auto STRING = ::nanocube::lang::STRING;
    const auto LIST = ::nanocube::lang::LIST;
    const auto CALL = ::nanocube::lang::CALL;
    
    //        if (program.name.compare("query") != 0)
    //            throw std::runtime_error("program name is invalid");
    
    auto get_number = [](Node *node) -> int {
        if (node->type == NUMBER) {
            auto number = reinterpret_cast<Number*>(node);
            return (int) number->number;
        }
        else {
            throw std::runtime_error("cannot decode dimension from ast node");
        }
    };
    
    auto get_dimension_number = [&](Node *node) -> int {
        if (node->type == NUMBER) {
            return get_number(node);
        }
        else if (node->type == STRING) {
            auto string = reinterpret_cast<String*>(node);
            auto index = annotated_schema.schema.getDimensionIndex(string->st);
            
            if (index >= 0) {
                return index;
            }
            else {
                throw std::runtime_error("Found no mapping between dimension name and dimension number");
            }
        }
        else {
            throw std::runtime_error("cannot decode dimension from ast node");
        }
    };
    
    auto get_string = [&](Node *node) -> std::string {
        if (node->type == STRING) {
            auto st_node = reinterpret_cast<String*>(node);
            return st_node->st;
        }
        else {
            throw std::runtime_error("cannot decode address");
        }
    };
    
    auto get_address = [&](Node *node) -> DimAddress{
        if (node->type == LIST) {
            auto &list = reinterpret_cast<List&>(*node);
            DimAddress addr;
            for (auto x: list.items) {
                if (x->type == NUMBER) {
                    auto number = reinterpret_cast<Number*>(x);
                    addr.push_back((int)number->number);
                }
                else {
                    throw std::runtime_error("cannot decode address");
                }
            }
            return addr;
        }
        else if (node->type == CALL) {
            auto &call = reinterpret_cast<Call&>(*node);
            if (call.name.compare("tile") == 0 || call.name.compare("tile2d") == 0) {
                if (call.params.size() != 3)
                    throw std::runtime_error("invalid number of parameters for range2d");
                
                auto x     = get_number(call.params[0]);
                auto y     = get_number(call.params[1]);
                auto level = get_number(call.params[2]);
                auto addr = ::nanocube::Tile(x,y,level).toAddress();
                return addr;
            }
            else if (call.name.compare("tile1d") == 0) {
                if (call.params.size() != 2)
                    throw std::runtime_error("invalid number of parameters for range1d");
                
                auto x     = get_number(call.params[0]);
                auto level = get_number(call.params[1]);
                auto addr = ::nanocube::Tile1d(x,level).toAddress();
                return addr;
            }
            else {
                throw std::runtime_error("cannot decode address");
            }
        }
        else if (node->type == NUMBER) {
            DimAddress addr;
            auto number = reinterpret_cast<Number*>(node);
            addr.push_back((int)number->number);
            return addr;
        }
        else {
            throw std::runtime_error("cannot decode dimension from ast node");
        }
    };
    
    auto set_target = [&](int dimension_index, Node *node) {
        if (node->type == LIST) {
            auto addr = get_address(node);
            auto raw_addr = annotated_schema.convertPathAddress(dimension_index, addr);
            query_description.setFindAndDiveTarget(dimension_index, raw_addr, 0);
        }
        else if (node->type == NUMBER) {
            auto label = get_number(node);
            DimAddress addr;
            addr.push_back(label);
            auto raw_addr = annotated_schema.convertPathAddress(dimension_index, addr);
            query_description.setFindAndDiveTarget(dimension_index, raw_addr, 0);
        }
        else if (node->type == CALL){
            auto &call = reinterpret_cast<Call&>(*node);
            if (call.name.compare("set") == 0) {
                std::vector<::query::RawAddress> raw_addresses;
                for (auto *param: call.params) {
                    auto addr  = get_address(param);
                    auto raw_addr = annotated_schema.convertPathAddress(dimension_index, addr);
                    raw_addresses.push_back(raw_addr);
                }
                query_description.setSequenceTarget(dimension_index, raw_addresses);
            }
            else if (call.name.compare("dive") == 0) {
                if (call.params.size() != 2)
                    throw std::runtime_error("invalid number of parameters for dive");
                auto addr  = get_address(call.params[0]);
                auto depth = get_number(call.params[1]);
                auto raw_addr = annotated_schema.convertPathAddress(dimension_index, addr);
                query_description.setFindAndDiveTarget(dimension_index, raw_addr, depth);
                format_options[dimension_index].base_address = addr;
            }
            else if (call.name.compare("range2d") == 0) {
                if (call.params.size() != 2)
                    throw std::runtime_error("invalid number of parameters for range2d");
                auto addr1  = get_address(call.params[0]);
                auto addr2  = get_address(call.params[1]);
                auto raw_addr1 = annotated_schema.convertPathAddress(dimension_index, addr1);
                auto raw_addr2 = annotated_schema.convertPathAddress(dimension_index, addr2);
                query_description.setRangeTarget(dimension_index, raw_addr1, raw_addr2);
            }
            else if (call.name.compare("mask") == 0) {
                if (call.params.size() < 1)
                    throw std::runtime_error("invalid number of parameters for mask");
                
                auto code = get_string(call.params[0]);
                auto level = 0;
                if (call.params.size() >= 2) {
                    level = (int) get_number(call.params[1]);
                }

                std::string key = std::string("mask_level") + std::to_string(level) + std::string("_") + code;

                auto mask = mask_cache[key];
                if (!mask) {
                    mask = ::polycover::labeled_tree::load_from_code(code);
                    if (level > 0)
                        mask->trim(level);

                    
                    that.cacheMask(key, mask);
                }
                
                query_description.setMaskTarget(dimension_index, mask);
            }
            else if (call.name.compare("degrees_mask") == 0 || call.name.compare("mercator_mask") == 0) {
                
                bool degrees = call.name.compare("degrees_mask") == 0;
                
                if (call.params.size() < 2)
                    throw std::runtime_error("invalid number of parameters for mask");
                
                auto points_st = get_string(call.params[0]);
                auto level     = get_number(call.params[1]);
                
                std::string prefix = degrees ? std::string("degrees_mask") : std::string("mercator_mask");
                std::string key = prefix + std::string("_level") + std::to_string(level) + std::string("_") + points_st;
                
                auto mask = mask_cache[key];
                if (!mask) {
                    
                    // split on the commas x0,y0,x1,y1,x2,y2;x0,y0,x1,y1,x2,y2;
                    std::stringstream ss(points_st);
                    std::string contour_st;
                    
                    // polygons
                    std::vector<polycover::Polygon> polygons;
                    
                    // sstd::vector<polycover::
                    while (std::getline(ss,contour_st,';')) {
                        std::stringstream ss2(contour_st);
                        std::string coord_st;
                        polygons.push_back(polycover::Polygon());
                        auto &poly = polygons.back();
                        double x,y;
                        int parity = 0;
                        while (std::getline(ss2,coord_st,',')) {
                            if (parity == 0) {
                                parity = 1;
                                x = std::stof(coord_st);
                            }
                            else {
                                parity = 0;
                                y = std::stof(coord_st);
                                
                                // convert to mercator
                                if (degrees) {
                                    x = x / 180.0;
                                    auto lat_rad = (y * M_PI/180.0);
                                    y = std::log(std::tan(lat_rad/2.0 + M_PI/4.0)) / M_PI;
                                }
                                
                                poly.points.push_back({x,y});
                            }
                        }
                        poly.makeItCW(); // make sure it is clock-wise before running the compute cover
                    }
                    
                    // TODO: caching and memory release of masks...
                    mask = ::polycover::TileCoverEngine(level,8).computeCover(polygons);
                    
                    // insert on the cache
                    that.cacheMask(key, mask);
                }

                query_description.setMaskTarget(dimension_index, mask);
                
            }
            else if (call.name.compare("region") == 0) {
                
                if (call.params.size() < 1)
                    throw std::runtime_error("invalid number of parameters for mask");
                
                auto region_path = get_string(call.params[0]);
                
                int level = 0;
                if (call.params.size() >= 2) {
                    level = (int) get_number(call.params[1]);
                }
                
                std::string key = std::string("region") + std::string("_level") + std::to_string(level) + std::string("_") + region_path;
                
                auto mask = mask_cache["key"];

                if (!mask) {

                    // TODO: get environment variable NANOCUBE_REGIONS
                    std::string nanocube_regions_path(std::getenv("NANOCUBE_REGIONS"));
                    // std::string nanocube_regions_path("/Users/llins/tests/polycover/data/geofences");
                    
                    // append .ttt.gz and check if file exists
                    std::string path = nanocube_regions_path + region_path + ".ttt";
                    std::ifstream f(path);
                    
                    if (!f.good()) {
                        throw std::runtime_error("could not find region " + region_path);
                    }

                    // TODO: make it more efficient
                    
                    polycover::labeled_tree::Parser parser;
                    parser.signal.connect([&mask, &level](const std::string& name, const polycover::labeled_tree::Node &node) {
                        std::stringstream ss;
                        ss << node;
                        mask = polycover::labeled_tree::load_from_code(ss.str());
                        if (level > 0) {
                            mask->trim(level);
                        }
                    });
                    parser.run(f,1);
                    
                    // insert on the cache
                    that.cacheMask(key, mask);
                    
                }
                
                query_description.setMaskTarget(dimension_index, mask);
                
            }
            
            
            
            //            else if (call.name.compare("range1d") == 0) {
            //                if (call.params.size() != 2)
            //                    throw std::runtime_error("invalid number of parameters for range2d");
            //                auto addr1  = get_address(call.params[0]);
            //                auto addr2  = get_address(call.params[1]);
            //                return nanocube::Target::range1d(addr1, addr2);
            //            }
            else if (call.name.compare("interval") == 0) {
                //
                // binary trees and time dimension: since there is no binary tree
                // here, assume only time dimension
                //
                if (annotated_schema.dimType(dimension_index) != AnnotatedSchema::TIME)
                    throw std::runtime_error("Interval only works in the time dimension");
                if (call.params.size() != 2)
                    throw std::runtime_error("invalid number of parameters for interval");
                auto a  = get_number(call.params[0]);
                auto b  = get_number(call.params[1]);
                query_description.setBaseWidthCountTarget(dimension_index, a, b-a+1, 1);
            }
            else if (call.name.compare("mt_interval_sequence") == 0 || call.name.compare("mt_intseq") == 0) {
                if (annotated_schema.dimType(dimension_index) != AnnotatedSchema::TIME)
                    throw std::runtime_error("Interval only works in the time dimension");
                if (call.params.size() != 3)
                    throw std::runtime_error("invalid number of parameters for interval");
                auto base   = get_number(call.params[0]);
                auto width  = get_number(call.params[1]);
                auto count  = get_number(call.params[2]);
                
                query_description.setBaseWidthCountTarget(dimension_index, base, width, count);
                
                branch_target_on_time = BranchTargetOnTime(base, width, count); // this is a branch target!
            }
            else {
                throw std::runtime_error("cannot decode call: " + call.name);
            }
        }
        // throw std::runtime_error("could not decode target");
    };
    
    
    
    
    Call *call_p = program.first_call;
    while (call_p != nullptr) {
        Call &call = *call_p;
        
        if (call.name.compare("a") == 0) {
            
            // get three parameters
            if (call.params.size() < 2)
                throw std::runtime_error("invalid number of parameters");
            
            // param 0 should be the dimension (if string convert from name)
            auto dim    = get_dimension_number(call.params[0]); // 0: dim
            set_target(dim, call.params[1]); // 1: target
            
            if (call.params.size() > 2) {
                std::string hint = get_string(call.params[2]);
                if (hint.compare("img") == 0) {
                    format_options[dim].type = FormatOption::RELATIVE_IMAGE;
                    query_description.setImgHint(dim, true);
                }
            }
            
            bool anchor = true;
            query_description.setAnchor(dim, anchor);
        }
        else if (call.name.compare("r") == 0) {
            // get three parameters
            if (call.params.size() != 2)
                throw std::runtime_error("invalid number of parameters");
            
            // param 0 should be the dimension (if string convert from name)
            auto dim    = get_dimension_number(call.params[0]); // 0: dim
            set_target(dim, call.params[1]); // 1: target
            
            
            bool anchor    = true;
            bool no_anchor = false;
            if (branch_target_on_time.active && dim == annotated_schema.dimension_sizes.size()-1)
                query_description.setAnchor(dim, anchor);
            else
                query_description.setAnchor(dim, no_anchor);
        }
        else if (call.name.compare("text") == 0) {
            output_encoding = TEXT;
        }
        else if (call.name.compare("json") == 0) {
            output_encoding = JSON;
        }
        else if (call.name.compare("bin") == 0) {
            output_encoding = BINARY;
        }
        call_p = call_p->next_call;
    }
}



//
// serveQuery
//

void NanocubeServer::serveQuery(Request &request, ::nanocube::lang::Program &program)
{
    boost::unique_lock<boost::shared_mutex> lock(shared_mutex);

    auto process = [&]() {
        
        ::query::QueryDescription query_description;
        
        AnnotatedSchema annotated_schema(schema);
        
        OutputEncoding output_encoding = JSON;
        
        BranchTargetOnTime branch_target_on_time_dimension;
        
        std::vector<FormatOption> format_options(::query::QueryDescription::MAX_DIMENSIONS);
        
        parse_program_into_query( program,
                                  annotated_schema,
                                  query_description,
                                  output_encoding,
                                  branch_target_on_time_dimension,
                                  format_options );
        
        //
        // it will be tricky to translate the multi_target aspect of the query
        //
        
        // count number of anchored flags
        int num_anchored_dimensions = 0;
        std::vector<int> anchored_dimensions;
        {
            int i = 0;
            for (auto flag: query_description.anchors) {
                if (flag) {
                    num_anchored_dimensions++;
                    anchored_dimensions.push_back(i);
                }
                ++i;
            }
        }
        
        // big hack
        
        ::nanocube::TreeValue treestore_result(num_anchored_dimensions);
        
        ::query::result::Result result(treestore_result);
        
        nanocubes[active_window].get()->query(query_description, result);

        if (sliding) {

            // add the result from the previous window too...
            int previous_window = active_window == 0 ? 2 : active_window-1;
            
            nanocubes[active_window].get()->query(query_description, result);
            
            if (nanocubes[previous_window].get()) {
                nanocubes[previous_window].get()->query(query_description, result);
            }
            
        }
        
        // set level names
        int level=0;
        int i=0;
        for (auto flag: query_description.anchors) {
            if (flag) {
                if (annotated_schema.dimType(i) == AnnotatedSchema::TIME) {
                    if (branch_target_on_time_dimension.active)
                        treestore_result.setLevelName(level, std::string("multi-target:") + schema.getDimensionName(i));
                    else
                        throw std::runtime_error("Cannot anchor on time dimension");
                }
                else {
                    treestore_result.setLevelName(level, std::string("anchor:") + schema.getDimensionName(i));
                }
                ++level;
            }
            i++;
        }
        
        std::stringstream ss;
        SimpleConfig::parameter_type parameter;
        using Writer    = ::tree_store::Writer<SimpleConfig, typename SimpleConfig::parameter_type>;
        using LabelType = typename SimpleConfig::label_type;
        
        if (output_encoding == JSON) {
            Writer writer;
            
            int layer = 0;
            int dim   = 0;
            for (auto &format_option: format_options)
            {
                if (query_description.anchors[dim])
                    ++layer;

                if (format_option.type == FormatOption::RELATIVE_IMAGE) {
                    using fmt_func = typename Writer::format_label_func;
                    fmt_func f = [&format_option](const LabelType& lbl) {
                        std::stringstream ss;
                        auto x = lbl.at(0);
                        auto y = lbl.at(1);
//                        auto n = format_option.base_address.size();
//                        auto suffix = LabelType(lbl.begin()+n,lbl.end());
//                        ::nanocube::Tile tile(suffix);
//                        ss << "\"x\":" << tile.x << ", " << "\"y\":" << tile.y;
                        ss << "\"x\":" << x << ", " << "\"y\":" << y;
                        return ss.str();
                    };
                    writer.setFormatLabelFunction(layer, f);
                }
                ++dim;
            }
            
            
            writer.json(treestore_result, ss, parameter);
            
//            ::tree_store::json(treestore_result, ss, parameter);
            request.respondJson(ss.str());
        }
        else if (output_encoding == TEXT) {
            ::tree_store::text(treestore_result, ss, parameter);
            request.respondText(ss.str());
        }
        else if (output_encoding == BINARY) {

            // if the img flag was used, treat long path
            // as a 2 entry path: local x and local y
            
//            std::vector<bool> dimensions_to_transform_to_img(treestore_result.getNumLevels());
//            std::vector<int>  base_address_sizes(treestore_result.getNumLevels());
//            
//            bool convert = false;
//            
//            int layer = 0;
//            int dim   = 0;
//            for (auto &format_option: format_options)
//            {
//                if (query_description.anchors[dim]) {
//                    dimensions_to_transform_to_img[layer] = format_option.type == FormatOption::RELATIVE_IMAGE;
//                    base_address_sizes[layer] = (int) format_option.base_address.size();
//                    convert |= dimensions_to_transform_to_img[layer];
//                    ++layer;
//                }
//                ++dim;
//            }
//            
//            if (convert && !treestore_result.empty()) {
//                TreeValueIterator it(treestore_result);
//                while (it.next()) {
//                    auto item = it.getCurrentItem();
//                    if (dimensions_to_transform_to_img[item.layer]) {
//                        
//                        auto node = item.node->asInternalNode();
//                        
//                        
//                        // std::unordered_map<
//                        int prefix_size = base_address_sizes[item.layer];
//                        
//                        auto convert_label = [prefix_size](const LabelType& lbl) {
//                            auto suffix = LabelType(lbl.begin()+prefix_size,lbl.end());
//                            ::nanocube::Tile tile(suffix);
//                            return LabelType { tile.x, tile.y };
//                        };
//                        
//                        // clear node->children and add all them back again
//                        // with a new label
//                        node->relabelPathToChildren(convert_label);
//                        
//                    }
//                }
//            }
            
            ::tree_store::serialize(treestore_result, ss);
            const auto &text = ss.str();
            request.respondOctetStream(text.c_str(), text.size());
        }
        
    };
    
    try {
        process();
    } catch (std::runtime_error &e) {
        request.respondText(e.what());
    } catch (...) {
        request.respondText("ooops");
    }

}

void NanocubeServer::serveTile(Request &request)
{
    
#if 0
    //
    // assuming the query result is a list of quadtree addresses
    //

    boost::shared_lock<boost::shared_mutex> lock(shared_mutex);

    // first entry in params is the url (empty right now)
    // second entry is the handler key, starting from
    // index 2 are the parameters;
    std::stringstream ss;
    bool first = true;
    for (auto it=request.params.begin()+2; it!=request.params.end(); it++) {
        if (!first) {
            ss << "/";
        }
        ss << *it;
        first = false;
    }

    // query string
    // std::string query_st = "src=[qaddr(0,0,2),qaddr(0,0,2)]/@dst=qaddr(1,0,1)+1";
    std::string query_st = ss.str();

    // log
    // std::cout << query_st << std::endl;

    ::query::QueryDescription query_description;

    try {
        this->parse(query_st, nanocube.schema, query_description);
    }
    catch (::query::parser::QueryParserException &e) {
        request.respondJson(e.what());
        return;
    }

    // count number of anchored flags
    int num_anchored_dimensions = 0;
    for (auto flag: query_description.anchors) {
        if (flag)
            num_anchored_dimensions++;
    }

    ::query::result::Vector result_vector(num_anchored_dimensions);

    // set dimension names
    int level=0;
    int i=0;
    for (auto flag: query_description.anchors) {
        if (flag) {
            result_vector.setLevelName(level++, nanocube.schema.getDimensionName(i));
        }
        i++;
    }

    ::query::result::Result result(result_vector);

    // ::query::result::Result result;
    try {
        nanocube.query(query_description, result);
    }
    catch (::nanocube::query::QueryException &e) {
        request.respondJson(e.what());
        return;
    }
    catch (...) {
        request.respondJson("Unexpected problem. Server might be unstable now.");
        return;
    }

    if (result_vector.getNumLevels() != 1) {
        request.respondJson("tile query result should have a single level.");
    }
    else {

        using Edge  = ::vector::Edge;
        using Value = ::vector::Value;
        using INode = ::vector::InternalNode;

        // tile
        ::maps::Tile tile;
        auto *target = query_description.getFirstAnchoredTarget();
        if (target) {
            auto *find_and_dive_target = target->asFindAndDiveTarget();
            if (find_and_dive_target) {
                ::query::RawAddress raw_address = find_and_dive_target->base;
                tile = maps::Tile(raw_address);
            }
            else {
                request.respondJson("problem");
                return;
            }
        }
        else {
            request.respondJson("problem");
            return;
        }

        uint32_t written_bytes = 0;
        std::ostringstream os;
        if (result_vector.root) {
            INode *inode = result_vector.root->asInternalNode();
            for (auto it: inode->children) {
                Edge &e = it.second;

                maps::Tile subtile(e.label);
                maps::Tile relative_tile = tile.relativeTile(subtile);

                Value value = e.node->asLeafNode()->value;
                uint8_t ii = relative_tile.getY();
                uint8_t jj = relative_tile.getX();

                // i, j, value
                os.write((char*) &ii, sizeof(uint8_t));
                os.write((char*) &jj, sizeof(uint8_t));
                os.write((char*) &value, sizeof(Value));

                written_bytes += sizeof(uint8_t) + sizeof(uint8_t) + sizeof(Value);

            } // transfering data to main matrix
        }
        request.respondOctetStream(os.str().c_str(), written_bytes);
    }
    
#endif
    
}

void NanocubeServer::serveTimeQuery(Request &request, bool json, bool compression)
{
#if 0
    boost::shared_lock<boost::shared_mutex> lock(shared_mutex);

    // first entry in params is the url (empty right now)
    // second entry is the handler key, starting from
    // index 2 are the parameters;
    std::stringstream ss;
    bool first = true;
    for (auto it=request.params.begin()+2; it!=request.params.end(); it++) {
        if (!first) {
            ss << "/";
        }
        ss << *it;
        first = false;
    }

    // query string
    // std::string query_st = "src=[qaddr(0,0,2),qaddr(0,0,2)]/@dst=qaddr(1,0,1)+1";
    std::string query_st = ss.str();

    // log
    // std::cout << query_st << std::endl;

    ::query::QueryDescription query_description;

    try {
        this->parse(query_st, nanocube.schema, query_description);
    }
    catch (::query::parser::QueryParserException &e) {
        request.respondJson(e.what());
        return;
    }

    int time_dimension = nanocube.schema.dump_file_description.getTimeFieldIndex();
    if (query_description.anchors[time_dimension] ||
            query_description.targets[time_dimension]->type != ::query::Target::ROOT) {
        request.respondJson("time queries should not constrain time dimension");
        return;
    }

    // set
    query_description.anchors[time_dimension] = true;


    // count number of anchored flags
    int num_anchored_dimensions = 0;
    for (auto flag: query_description.anchors) {
        if (flag)
            ++num_anchored_dimensions;
    }

    ::query::result::Vector result_vector(num_anchored_dimensions);

    // set dimension names
    int level=0;
    int i=0;
    for (auto flag: query_description.anchors) {
        if (flag) {
            result_vector.setLevelName(level++, nanocube.schema.getDimensionName(i));
        }
        i++;
    }

    ::query::result::Result result(result_vector);

    // ::query::result::Result result;
    try {
        nanocube.timeQuery(query_description, result);
    }
    catch (::nanocube::query::QueryException &e) {
        request.respondJson(e.what());
        return;
    }
    catch (...) {
        request.respondJson("Unexpected problem. Server might be unstable now.");
        return;
    }



    if (json) { // json query

        ::nanocube::QueryResult query_result(result_vector, nanocube.schema);
        ss.str("");
        query_result.json(ss);
        // ss << result;
        request.respondJson(ss.str());

    }
    else { // binary query

        std::ostringstream os;
        ::query::result::serialize(result_vector,os);
        const std::string st = os.str();

        // compress data
        if (compression) {
            auto compressed_data = zlib_compress(st.c_str(), st.size());
            request.respondOctetStream(&compressed_data[0], compressed_data.size());
        }
        else {
            request.respondOctetStream(st.c_str(), st.size());
        }

    }
#endif
}

void NanocubeServer::serveTiming(Request &request)
{
    boost::shared_lock<boost::shared_mutex> lock(shared_mutex);
    server.toggleTiming(!server.isTiming());
    request.respondJson(server.isTiming() ? "\"Timing is On\"" : "\"Timing is Off\"");
}

void NanocubeServer::serveVersion(Request &request)
{
    boost::shared_lock<boost::shared_mutex> lock(shared_mutex);
    request.respondJson(NANOCUBE_VERSION);
}

// void NanocubeServer::serveShutdown(Request &request)
// {
//     boost::shared_lock<boost::shared_mutex> lock(shared_mutex);
//     request.respondJson("Nanocube server shutting down in 3 seconds...");
//     std::cout << "Nanocube server shutting down in 3 seconds..." << std::endl;
//     sleep(3);
//     finish = true;
// }

void NanocubeServer::serveSetValname(Request &request)
{
#if 0
    boost::shared_lock<boost::shared_mutex> lock(shared_mutex);
    
    // first entry in params is the url (empty right now)
    // second entry is the handler key, starting from
    // index 2 are the parameters;
    
    
    //
    // http://.../valname/id/0/iPhone
    // http://.../valname/id/1/Android
    // http://.../valname/id/2/Windows
    //
    
    if (request.params.size() < 5) {
        request.respondJson("wrong number of parameters: e.g. http://<url>/valname/device/0/iPhone");
        return;
    }
    
    
    try {
        auto field_name = request.params.at(2);
        auto key   = std::stoull(request.params.at(3));
        auto value = request.params.at(4);
        
        auto &descr = nanocube.schema.dump_file_description;
        
        auto field = descr.getFieldByName(field_name);
        if (!field) {
            request.respondJson("nanocube has no dimension named: " + field_name);
            return;
        }
        field->addValueName(key, value);
        request.respondJson("Success!");
        
    }
    catch(...) {
        request.respondJson("problem on setValName request");
        return;
    }
#endif
}

void NanocubeServer::serveSchema(Request &request, bool json)
{
    auto &dump_file = schema.dump_file_description;
    
    std::stringstream ss;
    if (json) {
    
        json::JsonWriter writer(ss);
        
        // everything is a big dictionary
        {
            json::ContextGuard g = writer.dict();
            {
                json::ContextGuard g2 = writer.list("fields");
                for (auto f: dump_file.fields) {
                    json::ContextGuard g3 = writer.dict()
                    .dict_entry("name", f->name)
                    .dict_entry("type", f->field_type.name);
                    {
                        json::ContextGuard g4 = writer.dict("valnames");
                        for (auto it: f->map_value_to_valname) {
                            writer.dict_entry(it.second, it.first);
                        }
                    }
                }
            }
            {
                json::ContextGuard g2 = writer.list("metadata");
                for (auto it: dump_file.metadata) {
                    auto key   = it.first;
                    auto value = it.second;
                    json::ContextGuard g3 = writer.dict()
                    .dict_entry("key", key)
                    .dict_entry("value", value);
                }
                {
                    json::ContextGuard g3 = writer.dict()
                    .dict_entry("key", "name")
                    .dict_entry("value", dump_file.name);
                }
            }
        }
        request.respondJson(ss.str());
    }
    else {
        ss << schema.dump_file_description;
        request.respondText(ss.str());
    }
    
    //    {
    //        json::ContextGuard metadata = writer.list("metadata");
    //        for (auto it: std::string level_name: result.level_names) {
    //            json::ContextGuard g3;
    //            writer.dict()
    //                  .dict_entry("name", f->name)
    //                  .dict_entry("type", f->field_type.name);
    //        }
    //    }
    //    dump_file.fields
    // report::Report report(nanocube.DIMENSION + 1);
    // nanocube.mountReport(report);
    //    std::stringstream ss;
    //    ss << nanocube.schema.dump_file_description;
}


// //-----------------------------------------------------------------------------
// // Options
// //-----------------------------------------------------------------------------

// struct Options {
//     Options(const std::vector<std::string> &argv);

//     Options(const Options& other) = default;
//     Options& operator=(const Options& other) = default;

//     int      initial_port        {  29512 };
//     int      no_mongoose_threads {     10 };
//     uint64_t max_points          {      0 };
//     uint64_t report_frequency    { 100000 };
//     uint64_t batch_size          {   1000 };
//     uint64_t sleep_for_ns        {    100 };

//     uint64_t    logmem_delay     {   5000 };
//     std::string logmem;
    
//     std::string input_filename;
// };

// Options::Options(const std::vector<std::string> &argv)
// {
//     // read options
//     // find type of dump
//     try {
//         for (std::string param: argv) {
//             std::vector<std::string> tokens;
//             split(param,'=',tokens);
//             if (tokens.size() < 1) {
//                 continue;
//             }
//             if (tokens[0].compare("--max") == 0) {
//                 max_points = std::stoull(tokens[1]);
//             }
//             if (tokens[0].compare("--threads") == 0) {
//                 no_mongoose_threads = std::stoi(tokens[1]);
//             }
//             if (tokens[0].compare("--port") == 0) {
//                 initial_port = std::stoi(tokens[1]);
//             }
//             if (tokens[0].compare("--batch_size") == 0 || tokens[0].compare("--bs") == 0) {
//                 batch_size = std::stoull(tokens[1]);
//             }
//             if (tokens[0].compare("--sleep_for") == 0 || tokens[0].compare("--sf") == 0) {
//                 sleep_for_ns = std::stoull(tokens[1]);
//             }
//             if (tokens[0].compare("--report-frequency") == 0 || tokens[0].compare("--rf") == 0) { // report frequency
//                 report_frequency = std::stoull(tokens[1]);
//             }
//             if (tokens[0].compare("--in") == 0) { // report frequency
//                 input_filename = tokens[1];
//             }
//             if (tokens[0].compare("--logmem") == 0) { // report frequency
//                 logmem = tokens[1];
//             }
//             if (tokens[0].compare("--logmem-delay") == 0) { // report frequency
//                 logmem_delay = std::stoull(tokens[1]);
//             }
//         }
//     }
//     catch (...)
//     {}
// }


//------------------------------------------------------------------------------
// main
//------------------------------------------------------------------------------

#define xDEBUG_NC_PROCESS

int main(int argc, char *argv[]) {

    std::vector<std::string> params(argv, argv+argc);
    Options options(params);
    
#ifdef DEBUG_NC_PROCESS
    std::cerr << "starting nc_... child process start" << std::endl;
#endif
    
    
#if 0
    std::unique_ptr<std::ostream> ostream;
    std::unique_ptr<timer::Timer> timer_ptr;
    if (options.logmem.size() > 0) {
        ostream = std::unique_ptr<std::ostream>(new std::ofstream(options.logmem));
        timer_ptr = std::unique_ptr<timer::Timer>(new timer::Timer(timer::Milliseconds(options.logmem_delay)));
        timer_ptr->subscribe([&ostream]() {
            auto mem_info = memory_util::MemInfo::get();
            auto memory = mem_info.res_MB();
            ostream->operator<<(memory) << "MB" << std::endl;
        });
    }
#endif
    
    auto run = [&options](std::istream& is, dumpfile::DumpFileDescription input_file_description) {

        // create nanocube_schema from input_file_description
        ::nanocube::Schema nanocube_schema(input_file_description);
        
#ifdef DEBUG_NC_PROCESS
        std::cerr << "starting nc_... child process " << std::endl;
#endif
        
        // start nanocube http server
        NanocubeServer nanocube_server(nanocube_schema, options, is);

    };
    
    
    try {
        if (options.schema.getValue().size()) {
            // read schema from file
            dumpfile::DumpFileDescription input_file_description;
            std::ifstream ifs(options.schema.getValue());
            ifs >> input_file_description;
            
            if (options.data.getValue().size()) {
                std::ifstream ifs2(options.data.getValue());
                run(ifs2, input_file_description);
            }
            else {
                run(std::cin, input_file_description);
            }
        }
        else {
            if (options.data.getValue().size()) {
                std::ifstream ifs2(options.data.getValue());
                
                // read schema form input
                dumpfile::DumpFileDescription input_file_description;
                ifs2 >> input_file_description;
                
                run(ifs2, input_file_description);
            }
            else {
                dumpfile::DumpFileDescription input_file_description;
                std::cin >> input_file_description;
                
                run(std::cin, input_file_description);
            }
        }
    }
    catch (...) {
        std::cerr << "[Problem] A problem happened on nc_... (possible cause: bad schema description on .dmp header)" << std::endl;
        return 1;
    }

    
    
#ifdef DEBUG_NC_PROCESS
    std::cerr << "finishing nc_... process" << std::endl;
#endif
    
    // join write thread
    return 0;

}
