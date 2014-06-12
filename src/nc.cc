#include <iostream>
#include <iomanip>
#include <thread>
#include <functional>
#include <fstream>

#include <zlib.h>
#include <boost/thread/shared_mutex.hpp>

#include "DumpFile.hh"
#include "MemoryUtil.hh"
#include "NanoCube.hh"
#include "Query.hh"
#include "QueryParser.hh"
#include "Server.hh"
#include "NanoCubeQueryResult.hh"
#include "NanoCubeSummary.hh"
#include "json.hh"


#include "util/timer.hh"

//
// geometry.hh and maps.hh
// are used just for the serveTile routine
// that deals with a hardcoded x:29bits, y:29bits, level: 6bits
// scheme of quadtree addresses on 64-bit unsigned integer
//
#include "geometry.hh"
#include "maps.hh"

using namespace nanocube;

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

//-----------------------------------------------------------------------------
// Options
//-----------------------------------------------------------------------------

struct Options {
    Options(const std::vector<std::string> &argv);

    Options(const Options& other) = default;
    Options& operator=(const Options& other) = default;

    int      initial_port        {  29512 };
    int      no_mongoose_threads {     10 };
    uint64_t max_points          {      0 };
    uint64_t report_frequency    { 100000 };
    uint64_t batch_size          {   1000 };
    uint64_t sleep_for_ns        {    100 };

    uint64_t    logmem_delay     {   5000 };
    std::string logmem;
    
    std::string input_filename;
};

Options::Options(const std::vector<std::string> &argv)
{
    // read options
    // find type of dump
    try {
        for (std::string param: argv) {
            std::vector<std::string> tokens;
            split(param,'=',tokens);
            if (tokens.size() < 1) {
                continue;
            }
            if (tokens[0].compare("--max") == 0) {
                max_points = std::stoull(tokens[1]);
            }
            if (tokens[0].compare("--threads") == 0) {
                no_mongoose_threads = std::stoi(tokens[1]);
            }
            if (tokens[0].compare("--port") == 0) {
                initial_port = std::stoi(tokens[1]);
            }
            if (tokens[0].compare("--batch_size") == 0 || tokens[0].compare("--bs") == 0) {
                batch_size = std::stoull(tokens[1]);
            }
            if (tokens[0].compare("--sleep_for") == 0 || tokens[0].compare("--sf") == 0) {
                sleep_for_ns = std::stoull(tokens[1]);
            }
            if (tokens[0].compare("--report-frequency") == 0 || tokens[0].compare("--rf") == 0) { // report frequency
                report_frequency = std::stoull(tokens[1]);
            }
            if (tokens[0].compare("--in") == 0) { // report frequency
                input_filename = tokens[1];
            }
            if (tokens[0].compare("--logmem") == 0) { // report frequency
                logmem = tokens[1];
            }
            if (tokens[0].compare("--logmem-delay") == 0) { // report frequency
                logmem_delay = std::stoull(tokens[1]);
            }
        }
    }
    catch (...)
    {}
}

//------------------------------------------------------------------------------
// NanoCube
//------------------------------------------------------------------------------

typedef nanocube::NanoCubeTemplate<
    boost::mpl::vector<LIST_DIMENSION_NAMES>,
    boost::mpl::vector<LIST_VARIABLE_TYPES>> NanoCube;

typedef typename NanoCube::entry_type Entry;

typedef typename NanoCube::address_type Address;

//------------------------------------------------------------------------------
// NanoCubeServer
//------------------------------------------------------------------------------

struct NanoCubeServer {

public: // Constructor

    NanoCubeServer(NanoCube &nanocube, Options &options, std::istream &input_stream);

private: // Private Methods

    void parse(std::string              query_st,
               ::nanocube::Schema        &schema,
               ::query::QueryDescription &query_description);

public: // Public Methods

    void serveQuery     (Request &request, bool json, bool compression);
    void serveTimeQuery (Request &request, bool json, bool compression);
    void serveTile      (Request &request);
    void serveStats     (Request &request);
    void serveSchema    (Request &request, bool json);
    void serveSetValname(Request &request);
    void serveTBin      (Request &request);
    void serveSummary   (Request &request);
    void serveGraphViz  (Request &request);
    void serveTiming    (Request &request);
    void serveVersion   (Request &request);

public:

    void write();

public: // Data Members

    NanoCube &nanocube;

    Options       options;
    std::istream &input_stream;

    Server server;

    boost::shared_mutex       shared_mutex; // one writer multiple readers

};

struct RunWrite {
    RunWrite(NanoCubeServer &server):
        server(server)
    {}
    void run() {
        server.write();
    }
    NanoCubeServer &server;
};

//------------------------------------------------------------------------------
// NanoCubeServer Impl.
//------------------------------------------------------------------------------

NanoCubeServer::NanoCubeServer(NanoCube &nanocube, Options &options, std::istream &input_stream):
    nanocube(nanocube),
    options(options),
    input_stream(input_stream)
{
    // start a thread to write on nanocube
    // auto f = std::bind(&NanoCubeServer::write, *this);

    RunWrite rw(*this);
    std::thread writer_thread(&RunWrite::run, &rw);

    //
    server.port = options.initial_port;

    bool json        = true;
    bool binary      = false;
    bool compression = true;
    bool plain       = false;

    auto json_query_handler    = std::bind(&NanoCubeServer::serveQuery, this, std::placeholders::_1, json,       plain);
    auto binary_query_handler  = std::bind(&NanoCubeServer::serveQuery, this, std::placeholders::_1, binary,     plain);

    auto json_tquery_handler   = std::bind(&NanoCubeServer::serveTimeQuery, this, std::placeholders::_1, json,   plain);
    auto binary_tquery_handler = std::bind(&NanoCubeServer::serveTimeQuery, this, std::placeholders::_1, binary, plain);

    // auto json_query_comp_handler    = std::bind(&NanoCubeServer::serveQuery, this, std::placeholders::_1, json,       compression);
    // auto json_tquery_comp_handler   = std::bind(&NanoCubeServer::serveTimeQuery, this, std::placeholders::_1, json,   compression);

    auto binary_query_comp_handler  = std::bind(&NanoCubeServer::serveQuery, this, std::placeholders::_1, binary,     compression);
    auto binary_tquery_comp_handler = std::bind(&NanoCubeServer::serveTimeQuery, this, std::placeholders::_1, binary, compression);

    auto stats_handler         = std::bind(&NanoCubeServer::serveStats, this, std::placeholders::_1);

    auto binary_schema_handler = std::bind(&NanoCubeServer::serveSchema,     this, std::placeholders::_1, binary);
    
    auto schema_handler        = std::bind(&NanoCubeServer::serveSchema,     this, std::placeholders::_1, json);

    auto valname_handler       = std::bind(&NanoCubeServer::serveSetValname, this, std::placeholders::_1);

    auto version_handler       = std::bind(&NanoCubeServer::serveVersion,    this, std::placeholders::_1);

    auto tbin_handler          = std::bind(&NanoCubeServer::serveTBin, this, std::placeholders::_1);

    auto summary_handler       = std::bind(&NanoCubeServer::serveSummary, this, std::placeholders::_1);

    auto graphviz_handler      = std::bind(&NanoCubeServer::serveGraphViz, this, std::placeholders::_1);

    auto timing_handler        = std::bind(&NanoCubeServer::serveTiming, this, std::placeholders::_1);

    auto tile_handler         = std::bind(&NanoCubeServer::serveTile, this, std::placeholders::_1);

    
    // register service

    server.registerHandler("query",      json_query_handler);
    server.registerHandler("binquery",   binary_query_handler);
    server.registerHandler("binqueryz",  binary_query_comp_handler);

    server.registerHandler("tile",       tile_handler);

    server.registerHandler("tquery",     json_tquery_handler);
    server.registerHandler("bintquery",  binary_tquery_handler);
    server.registerHandler("bintqueryz", binary_tquery_comp_handler);

    server.registerHandler("stats",     stats_handler);
    server.registerHandler("schema",    schema_handler);
    server.registerHandler("binschema", binary_schema_handler);
    server.registerHandler("valname",   valname_handler);
    server.registerHandler("tbin",      tbin_handler);
    server.registerHandler("summary",   summary_handler);
    server.registerHandler("graphviz",  graphviz_handler);

    server.registerHandler("version",  version_handler);

    server.registerHandler("timing",  timing_handler);

    server.registerHandler("start",  graphviz_handler);

    int tentative=0;
    while (tentative < 100) {
        tentative++;
        try {
            std::cout << "Starting NanoCubeServer on port " << server.port << std::endl;
            server.start(options.no_mongoose_threads);
        }
        catch (ServerException &e) {
            std::cout << e.what() << std::endl;
            server.port++;
        }
    }

    writer_thread.join();

}

void NanoCubeServer::parse(std::string              query_st,
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

void NanoCubeServer::serveQuery(Request &request, bool json, bool compression)
{
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
        if (json) {
            request.respondJson(e.what());
        }
        else {
            request.respondOctetStream(nullptr,0);
        }
        return;
    }


#if 0
    // Preprocess query description. The only case now
    // is to prepare a traversal mask for quadtree
    // dimensions based on a sequence of polygonal data.
    // A bit hacky for now.
    int dimension = -1;
    for (query::Target* target: query_description.targets) {
        ++dimension;
        if (target->asSequenceTarget() == nullptr)
            continue;

        dumpfile::Field* field = nanocube.schema.dump_file_description.getFieldByName(nanocube.schema.dimension_keys.at(dimension));
        if (field->field_type.name.find("nc_dim_quadtree") >= 0) {

            // ok we found a quadtree dimension. interpret sequence
            // of addresses as a polygon.
            // field->
        }
    }
#endif

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
        if (json) {
            request.respondJson("Unexpected problem. Server might be unstable now.");
        }
        else {
            request.respondOctetStream(nullptr,0);
        }
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
}

void NanoCubeServer::serveTile(Request &request)
{
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
}

void NanoCubeServer::serveTimeQuery(Request &request, bool json, bool compression)
{
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
}

void NanoCubeServer::serveStats(Request &request)
{
    boost::shared_lock<boost::shared_mutex> lock(shared_mutex);

    report::Report report(nanocube.DIMENSION + 1);
    nanocube.mountReport(report);
    std::stringstream ss;
    for (auto layer: report.layers) {
        ss << *layer << std::endl;
    }

    request.respondJson(ss.str());
}

void NanoCubeServer::serveGraphViz(Request &request)
{
    boost::shared_lock<boost::shared_mutex> lock(shared_mutex);

    report::Report report(nanocube.DIMENSION + 1);
    nanocube.mountReport(report);
    std::stringstream ss;
    report_graphviz(ss, report);
    request.respondJson(ss.str());
}

void NanoCubeServer::serveTiming(Request &request)
{
    boost::shared_lock<boost::shared_mutex> lock(shared_mutex);

    server.toggleTiming(!server.isTiming());
    request.respondJson(server.isTiming() ? "Timing is On" : "Timing is Off");
}

void NanoCubeServer::serveVersion(Request &request)
{
    boost::shared_lock<boost::shared_mutex> lock(shared_mutex);
    request.respondJson(VERSION);
}

void NanoCubeServer::serveSetValname(Request &request)
{
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
}

void NanoCubeServer::serveSchema(Request &request, bool json)
{
    auto &dump_file = nanocube.schema.dump_file_description;
    
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
        ss << nanocube.schema.dump_file_description;
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


void NanoCubeServer::serveTBin(Request &request)
{
    boost::shared_lock<boost::shared_mutex> lock(shared_mutex);

    auto &metadata = nanocube.schema.dump_file_description.metadata;
    auto it = metadata.find("tbin");
    if (it != metadata.end()) {
        auto str = it->second;
        request.respondJson(str);
    }
}

void NanoCubeServer::serveSummary(Request &request)
{
    boost::shared_lock<boost::shared_mutex> lock(shared_mutex);


    nanocube::Summary summary = mountSummary(this->nanocube);
    std::stringstream ss;
    ss << summary;
    request.respondJson(ss.str());
}



void NanoCubeServer::write()
{
    uint64_t batch_size = options.batch_size; // add 10k points before

    stopwatch::Stopwatch sw;
    sw.start();

    uint64_t count = 0;

    // std::cout << "reading data..." << std::endl;

    bool done = false;

    auto record_size = this->nanocube.schema.dump_file_description.record_size;
    auto num_bytes_per_batch = record_size * batch_size;
    
    std::stringstream ss;
    char buffer[num_bytes_per_batch];
    while (!done) {
        
        // std::cerr << "reading " << num_bytes_per_batch << "...";
        input_stream.read(buffer,num_bytes_per_batch);
        auto read_bytes = input_stream.gcount();
        // std::cerr << " " << read_bytes << " were read" << std::endl;
        
        ss.clear();
        ss.write(buffer, read_bytes);

        // write a batch of points
        if (read_bytes > 0)
        {
            boost::unique_lock<boost::shared_mutex> lock(shared_mutex);
            for (uint64_t i=0;i<batch_size && !done;++i)
            {
                // std::cout << i << std::endl;
                bool ok = nanocube.add(ss);
                if (!ok) {
                    // std::cout << "not ok" << std::endl;
                    done = true;
                }
                else {
                    // std::cout << "ok" << std::endl;
                    ++count;
                    done = (options.max_points > 0 && count == options.max_points);
                }
            }
        }

        // shared_mutex.
        if (options.sleep_for_ns == 0) {
            std::this_thread::yield(); // if there is a query it should wake up
        }
        else {
            std::this_thread::sleep_for(std::chrono::nanoseconds(options.sleep_for_ns));
        }

        // make sure report frequency is a multiple of batch size
        if (count % options.report_frequency == 0) {
            std::cout << "count: " << std::setw(10) << count << " mem. res: " << std::setw(10) << memory_util::MemInfo::get().res_MB() << "MB.  time(s): " <<  std::setw(10) << sw.timeInSeconds() << std::endl;
        }

        // std::this_thread::sleep_for(std::mill)

//        bool ok = nanocube.add(std::cin);
//        if (!ok) {
//            // std::cout << "not ok" << std::endl;
//            break;
//        }

//        { // generate pdf
//            report::Report report(nanocube.DIMENSION + 1);
//            nanocube.mountReport(report);
//            std::string filename     = "/tmp/bug"+std::to_string(count)+".dot";
//            std::string pdf_filename = "/tmp/bug"+std::to_string(count)+".pdf";
//            std::ofstream of(filename);
//            report::report_graphviz(of, report);
//            of.close();
//            system(("dot -Tpdf " + filename + " > " + pdf_filename).c_str());
//        }



    } // loop to insert objects into the nanocube

    std::cout << "count: " << std::setw(10) << count << " mem. res: " << std::setw(10) << memory_util::MemInfo::get().res_MB() << "MB.  time(s): " << std::setw(10) << sw.timeInSeconds() << std::endl;
    // std::cout << "count: " << count << " mem. res: " << memory_util::MemInfo::get().res_MB() << "MB." << std::endl;

    // test query using query language
    std::cout << "Number of points inserted " << count << std::endl;

}


//------------------------------------------------------------------------------
// main
//------------------------------------------------------------------------------

int main(int argc, char *argv[]) {

    std::vector<std::string> params;
    for (int i=1;i<argc;i++) {
        params.push_back(argv[i]);
    }

    Options options(params);
    
    
    
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
    
    
    auto run = [&options](std::istream& is) {
        // read input file description
        dumpfile::DumpFileDescription input_file_description;
        is >> input_file_description;
        
        // create nanocube_schema from input_file_description
        ::nanocube::Schema nanocube_schema(input_file_description);
        
        // create nanocube
        NanoCube nanocube(nanocube_schema);
        
        // start nanocube http server
        NanoCubeServer nanocube_server(nanocube, options, is);
    };
    
    if (options.input_filename.size()) {
        std::ifstream is(options.input_filename);
        run(is);
    }
    else {
        run(std::cin);
    }
    
    // join write thread
    return 0;

}












//stopwatch::Stopwatch sw;
//sw.start();

//uint64_t count = 0;
//std::cout << "reading data..." << std::endl;
//while (1) {

////        std::cout << "count: " << count << std::endl;

//    if (options.max_points > 0 && count == options.max_points) {
//        break;
//    }

//    bool ok = nanocube.add(std::cin);
//    if (!ok) {
//        // std::cout << "not ok" << std::endl;
//        break;
//    }

////        { // generate pdf
////            report::Report report(nanocube.DIMENSION + 1);
////            nanocube.mountReport(report);
////            std::string filename     = "/tmp/bug"+std::to_string(count)+".dot";
////            std::string pdf_filename = "/tmp/bug"+std::to_string(count)+".pdf";
////            std::ofstream of(filename);
////            report::report_graphviz(of, report);
////            of.close();
////            system(("dot -Tpdf " + filename + " > " + pdf_filename).c_str());
////        }

//    // std::cout << "ok" << std::endl;
//    count++;


//    if (count % options.report_frequency == 0) {
//        std::cout << "count: " << std::setw(10) << count << " mem. res: " << std::setw(10) << memory_util::MemInfo::get().res_MB() << "MB.  time(s): " <<  std::setw(10) << sw.timeInSeconds() << std::endl;
//    }


//} // loop to insert objects into the nanocube

//std::cout << "count: " << std::setw(10) << count << " mem. res: " << std::setw(10) << memory_util::MemInfo::get().res_MB() << "MB.  time(s): " << std::setw(10) << sw.timeInSeconds() << std::endl;
//// std::cout << "count: " << count << " mem. res: " << memory_util::MemInfo::get().res_MB() << "MB." << std::endl;

//// test query using query language
//std::cout << "Number of points inserted " << count << std::endl;
