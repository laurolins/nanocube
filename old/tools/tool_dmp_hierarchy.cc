#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <chrono>
#include <memory>
#include <cassert>

#include "hierarchy.hh"
#include "mmap.hh"

#include "tclap/CmdLine.h"

//-----------------------------------------------------------------
// Options
//-----------------------------------------------------------------

struct Options {
public:
    Options(const std::vector<std::string>& args);
    
public:
    TCLAP::CmdLine cmd_line { "compressed-nanocube-dmp-hierarchy", ' ', "0.1_2016.02.26" };
    

    TCLAP::ValueArg<std::string> input {
        "i",              // flag
        "input",         // name
        ".dmp filename", // description
        true,            // required
        "",               // value
        "filename" // type description
    };
    
    TCLAP::ValueArg<std::string> fields {
        "f",              // flag
        "fields",         // name
        "eg. latitude,longitude or time", // description
        true,            // required
        "",               // value
        "data" // type description
    };

    TCLAP::ValueArg<int> depth {
        "d",              // flag
        "depth",         // name
        "depth of hierarchy", // description
        true,            // required
        0,               // value
        "integer" // type description
    };

    TCLAP::ValueArg<int> max_records {
        "m",              // flag
        "max-records",         // name
        "max number of records", // description
        false,                    // required
        0,                       // value
        "integer"                // type description
    };

    TCLAP::ValueArg<int> status_frequency {
        "s",              // flag
        "status-frequency",         // name
        "max number of records", // description
        false,                    // required
        100000,                    // value
        "integer"                // type description
    };

};


Options::Options(const std::vector<std::string>& args)
{
    cmd_line.add(input); // add command option
    cmd_line.add(fields);
    cmd_line.add(depth);
    cmd_line.add(max_records);
    cmd_line.add(status_frequency);
    auto &a = const_cast<std::vector<std::string>&>(args);
    cmd_line.parse(a);
}



struct FieldType {
public:
    FieldType(const std::string& name, char kind, int bytes):
        _name(name), _kind(kind), _bytes(bytes)
    {}
    char  kind() const { return _kind; }
    int   bytes() const { return _bytes; }
    const std::string& name() const { return _name; }
public:
    std::string _name;
    char        _kind;
    int         _bytes;
};

struct Field {
public:
    Field(const std::string& name,
          const FieldType *type,
          std::size_t index,
          std::size_t offset):
        _name(name), _type(type), _index(index), _offset(offset)
    {}
    const std::string& name() const { return _name; }
    const FieldType* type() const { return _type; }
    const std::size_t index() const { return _index; }
    const std::size_t offset() const { return _offset; }
public:
    std::string       _name;
    const FieldType  *_type;
    std::size_t       _index;
    std::size_t       _offset;
};


struct Record {
public:

    Record() = default;

    void insert(const std::string& field_name, const FieldType *type) {
        auto i = _fields.size();
        _fields.push_back(std::unique_ptr<Field>(new Field(field_name, type, i, _bytes)));
        _bytes += type->bytes();
    }
    
    const Field* field(const std::string& field_name) const {
        for (auto &it: _fields) {
            if (it->name().compare(field_name) == 0) {
                return it.get();
            }
        }
        return nullptr;
    }
    
    std::size_t bytes() const { return _bytes; }
    
public:
    std::vector<std::unique_ptr<Field>> _fields;
    std::size_t        _bytes { 0 };
};

struct FieldTypeDB {
    void insert(const FieldType& field_type) {
        _field_types.push_back(std::unique_ptr<FieldType>(new FieldType(field_type)));
    }
    const FieldType* get(const std::string& type_name) const {
        for (auto &it: _field_types) {
            if (it->name().compare(type_name) == 0) {
                return it.get();
            }
        }
        return nullptr;
    }
    std::vector<std::unique_ptr<FieldType>> _field_types;
};

static const std::vector<FieldType> field_types {
    {"uint8",'i',1},
    {"uint16",'i',2},
    {"uint24",'i',3},
    {"uint32",'i',4},
    {"uint40",'i',5},
    {"uint48",'i',6},
    {"uint56",'i',7},
    {"uint64",'i',8},
    {"uint64",'i',8},
    {"float",'f',4},
    {"nc_dim_quadtree_17",'q',8},
    {"nc_dim_quadtree_20",'q',8},
    {"nc_dim_quadtree_25",'q',8},
    {"nc_dim_bintree_16",'i',2},
    {"nc_dim_cat_1",'i',1},
    {"nc_dim_cat_2",'i',2},
    {"nc_dim_cat_3",'i',3},
    {"nc_dim_cat_4",'i',4},
    {"nc_dim_time_2",'i',2},
    {"nc_dim_time_4",'i',2},
    {"nc_var_uint_4",'i',4},
    {"nc_var_uint_8",'i',8}
};


std::vector<std::string> tokenize(const std::string& st, const std::string& sep, int max=0) {
    std::vector<std::string> tokens;
    std::stringstream ss;

    auto insert = [&ss,&tokens]() {
        auto token = ss.str();
        ss.str("");
        if (token.length()) {
            tokens.push_back(token);
        }
    };
    
    for (auto &ch: st) {
        if (sep.find(ch) != std::string::npos) {
            insert();
            if (max && tokens.size() == max) {
                break;
            }
        }
        else {
            ss << ch;
        }
    }
    insert();
    
    return tokens;
}



//struct Tile {
//public:
//    Tile(int x, int y, int level):
//        _x(x), _y(y), _level(level)
//    {}
//    int x() const { return _x; }
//    int y() const { return _y; }
//    int level() const { return _level; }
//public:
//    int _x;
//    int _y;
//    int _level;
//};

void quadtree_path(float lon, float lat, std::vector<hierarchy::Label> &output) {
    auto depth = output.size();
    
    double mx = lon / 180.0;
    double my = std::log(std::tan((lat  * M_PI/180.0)/2.0f + M_PI/4.0f)) / M_PI;
    
    int cx = (int) ((mx + 1.0)/2.0 * (1u << depth));
    int cy = (int) ((my + 1.0)/2.0 * (1u << depth));
    
    for (auto i=0;i<depth;++i) {
        hierarchy::Label label =
            ((cx & (1u << (depth -1 -i))) ? 1 : 0) +
            ((cy & (1u << (depth -1 -i))) ? 2 : 0);
        output[i] = label;
    }
}

void quadtree_path(int cx, int cy, std::vector<hierarchy::Label> &output) {
    auto depth = output.size();
    
    for (auto i=0;i<depth;++i) {
        hierarchy::Label label =
        ((cx & (1u << (depth -1 -i))) ? 1 : 0) +
        ((cy & (1u << (depth -1 -i))) ? 2 : 0);
        output[i] = label;
    }
}

void binary_path(uint64_t value, std::vector<hierarchy::Label> &output) {
    auto depth = output.size();
    for (auto i=0;i<depth;++i) {
        hierarchy::Label label = ((value & (1u << (depth -1 -i))) ? 1 : 0);
        output[i] = label;
    }
}


using Timestamp = std::chrono::time_point<std::chrono::high_resolution_clock>;
struct Stopwatch {
    Stopwatch() {
        t0 = std::chrono::high_resolution_clock::now();
    }
    double elapsed() const {
        auto t1 = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(t1-t0).count()/1.0e3;
    }
    Timestamp t0;
};

int main(int argc, char** argv) {

    Options options(std::vector<std::string>(argv, argv+argc));
    
    std::cerr << options.cmd_line.getMessage() << " version: " << options.cmd_line.getVersion() << std::endl;
    
    FieldTypeDB field_type_db;
    for (auto &f: field_types) {
        field_type_db.insert(f);
    }
    
    Record record;
    
    std::istream *ist = &std::cin;
    std::ifstream ist_file;
    if (options.input.getValue().compare("-") != 0) {
        ist_file.open(options.input.getValue(),std::ifstream::in);
        ist = &ist_file;
    }
    std::istream &istream = *ist;
    
    
    std::string line;
    std::size_t line_no = 0;
    while (std::getline(istream,line,'\n')) {
        ++line_no;
        if (!line.length()) {
            break; // data is coming
        }
        
        // check for field line
        auto tokens = tokenize(line,":, ",3);
        
        if (tokens.size() == 3) {
            if (tokens[0].compare("field") == 0) {
                auto type = field_type_db.get(tokens[2]);
                if (!type) {
                    std::cerr << "[Error] Could not recognize type: " << tokens[3] << std::endl;
                    exit(-1);
                }
                record.insert(tokens[1],type);
            }
        }
    }
    
    // check if we can find fields
    auto field_names = tokenize(options.fields.getValue(),",");

    
    //
    // ff
    // qX
    // uY
    //
    
    struct Mode {
        enum Type { INVALID, MODE_Q, MODE_FF, MODE_I };
        Mode(const std::vector<const Field*>& fields, int depth):
            _fields(fields),
            _labels(depth)
        {
            std::stringstream ss;
            for (auto f: fields) {
                ss << f->type()->kind();
            }
            auto type_st = ss.str();
            if (type_st.compare("q")==0) {
                _bits = 2;
                _type = MODE_Q;
                _offsets[0] = fields[0]->offset();
                _offsets[1] = fields[0]->offset() + 4;
            }
            else if (type_st.compare("ff") == 0) {
                _bits = 2;
                _type = MODE_FF;
                _offsets[0] = fields[0]->offset();
                _offsets[1] = fields[1]->offset();
            }
            else if (type_st.compare("i") == 0) {
                _bits = 1;
                _type = MODE_I;
                _offsets[0] = fields[0]->offset();
                _offsets[1] = fields[0]->offset() + fields[0]->type()->bytes();
            }
            else {
                _type = INVALID;
            }
        }

        void process(hierarchy::Hierarchy& hierarchy, const char *record) {
            if (_type == MODE_Q) {
                auto cx = *((int*)(record + _offsets[0]));
                auto cy = *((int*)(record + _offsets[1]));
                quadtree_path(cx,cy,_labels);
                hierarchy.insert(hierarchy::LabelArray(&_labels[0],_labels.size()));
            }
            else if (_type == MODE_FF) {
                auto lat = *((float*)(record + _offsets[0]));
                auto lon = *((float*)(record + _offsets[1]));
                quadtree_path(lon,lat,_labels);
                hierarchy.insert(hierarchy::LabelArray(&_labels[0],_labels.size()));
            }
            else if (_type == MODE_I) {
                uint64_t value = 0;
                std::copy(record + _offsets[0], record + _offsets[1], (char*) &value);
                binary_path(value,_labels);
                hierarchy.insert(hierarchy::LabelArray(&_labels[0],_labels.size()));
            }
            else {
                assert(0 && "cannot process invalid mode");
            }
        }
        
        
        std::vector<hierarchy::Label> _labels;
        bool is(Type t) const { return t == _type; }
        int bits() const { return _bits; }
        operator bool() const { return _type != INVALID; }
        Type _type { INVALID };
        int  _bits { 0 };
        uint64_t _offsets[2];
        std::vector<const Field*> _fields;
    };

    
    std::vector<const Field*> fields;
    for (auto &fn: field_names) {
        auto f = record.field(fn);
        if (!f) {
            std::cerr << "[Error] Field not found: " << fn << std::endl;
            return -1;
        }
        fields.push_back(f);
    }
    
    //
    Mode mode(fields, options.depth.getValue());

    if (!mode) {
        std::cerr << "[Error] Mode is invalid. lat,lon float or quadtree25 or number uint" << std::endl;
        return -1;
    }

    // hierarchy
    alloc::memory_map::MMap mmap(1ULL<<30);
    auto& allocator = alloc::slab::allocator(mmap.memory_block());
    auto hierarchy_cache = allocator.cache_create("Hierarchy", sizeof(hierarchy::Hierarchy));
    auto &hierarchy = *(new (hierarchy_cache->alloc()) hierarchy::Hierarchy(allocator,mode.bits()));
    allocator.root(&hierarchy);
    
    // records
    auto record_size = record.bytes();
    std::vector<char> buffer(record_size);
    char *p = &buffer[0];

    std::size_t records = 0;

    Stopwatch stopwatch;
    
    while (istream.read(p,record_size)) {
        
        if (istream.gcount() != record_size)
            break;
        
        if (options.max_records.getValue() && records == options.max_records.getValue())
            break;
        
        mode.process(hierarchy, p);
        
        ++records;
        
        if (options.status_frequency.getValue() && (records % options.status_frequency.getValue()) == 0) {
            auto e = stopwatch.elapsed();
            std::cerr
            << "[status] | processed " << std::setw(10) << std::right << records
            << "  | elapsed time " << std::setw(10) << std::right << e
            << "  | rate " << std::setw(10) << std::right << std::setprecision(6) <<  (records/e)
            << " |" << std::endl;
        }
        
    }

    auto stats = hierarchy.stats();
    std::cerr
        << "nodes: " << stats.nodes()
        << "   length_sum: " << stats.length_sum()
        << "   ratio: " << std::fixed << ((double) stats.length_sum() / stats.nodes())
        << "   bytes: " << stats.bytes()
        << "   pio bytes: " << allocator.memory_block().size()
        << "   records: " << records
        << std::endl;

}