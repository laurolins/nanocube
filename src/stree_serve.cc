#include <iostream>
#include <random>
#include <chrono>
#include <deque>
#include <sstream>
#include <iomanip>
#include <set>

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>

#include <cmath>
#include <cstdlib>
#include <ctime>
#include <cstdio>

#include <unordered_map>
#include <unordered_set>

#include <string>
#include <fstream>

#include <Stopwatch.hh>
#include <TimeSeries.hh>
#include <QuadTree.hh>
#include <QuadTreeUtil.hh>
#include <MercatorProjection.hh>

#include <STree.hh>
#include <FlatTree.hh>

// boost pre-processor
#include <boost/preprocessor/repetition/repeat_from_to.hpp>

// boost::mpl Metaprogramming Template
#include <boost/mpl/if.hpp>
#include <boost/mpl/at.hpp>
#include <boost/mpl/push_back.hpp>
#include <boost/mpl/push_front.hpp>
#include <boost/mpl/reverse.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/begin_end.hpp>
#include <boost/mpl/back.hpp>
#include <boost/mpl/front.hpp>
#include <boost/type_traits/is_same.hpp>

#include <boost/tokenizer.hpp>

// some string manipulation
#include <boost/algorithm/string.hpp>

#include <MemoryUtil.hh>

//#include <TimeSeriesSerialization.hh>
//#include <FlatTreeSerialization.hh>
//#include <QuadTreeSerialization.hh>
//#include <STreeSerialization.hh>

#include <Server.hh>
Server *g_server;

#include <TimeBinFunction.hh>
#include <unordered_map>
#include <functional>

static std::string gs_hname;
static int         gs_hport;
static std::string gs_schema_name;
static std::string gs_nanodb = "dt-jklosow.client.research.att.com:29999";

#define xDEBUG_STREE_SERVER

// #include <boost/mpl/next_prior.hpp>
// #include <boost/mpl/for_each.hpp>
// #include <boost/mpl/range_c.hpp>


typedef uint64_t NumberOfPoints;

//-----------------------------------------------------------------------------
// Entry
//-----------------------------------------------------------------------------

#ifndef FAST_ENTRY
struct Entry
{
    typedef uint16_t TimeType;
    typedef uint64_t CountType;

    static void clipTime(int &t) {
        if (t < 0) {
            t = 0;
        }
        else if (t > 65535) {
            t = 65535;
        }
    }

    Entry() {
        time()  = 0;
        count(0);
    }

    Entry(TimeType time, CountType count) {
        this->time()  = time;
        this->count(count);
    }

    bool operator<(const Entry &e) const {
        return time() < e.time();
    }

#if 1
    inline TimeType &time() {
        return *((TimeType*) (&data[0]));
    }

    inline const TimeType &time() const {
        return *((TimeType*) (&data[0]));
    }

    inline CountType count() const {
        CountType c = 0;
        memcpy(&c, &(data[2]), 6);
        return c;
    }

    inline void count(CountType count) {
        memcpy(&(data[2]), &count, 6);
    }

    inline void increment_count(CountType increment) {
        CountType c = 0;
        memcpy(&c, &(data[2]), 6);
        c += increment;
        memcpy(&(data[2]), &c, 6);
    }

private:
    char data[8];
#else
    inline TimeType &time() {
        return _time;
    }

    inline CountType &count() {
        return _count;
    }

    inline const TimeType &time() const {
        return _time;
    }

    inline const CountType &count() const {
        return _count;
    }

private:
    TimeType  _time;
    CountType _count;
#endif
};
#else
struct Entry
{
    typedef uint16_t TimeType;
    typedef uint32_t CountType;
    TimeType _time;
    CountType _count;

    static void clipTime(int &t) {
        if (t < 0) {
            t = 0;
        }
        else if (t > 65535) {
            t = 65535;
        }
    }

    Entry() {};

    Entry(TimeType time, CountType count) {
        _time  = time;
        _count = count;
    }

    bool operator<(const Entry &e) const {
        return _time < e._time;
    }

    inline TimeType &time() {
        return _time;
    }

    inline CountType &count() {
        return _count;
    }

    inline const TimeType &time() const {
        return _time;
    }

    inline const CountType &count() const {
        return _count;
    }
};
#endif

//---------------------------------------------------------------------------
// TimeSeries
//---------------------------------------------------------------------------

template <typename TimeType>
struct TSeries {

    typedef double CountType;

    TSeries(TimeType first, TimeType incr, TimeType count):
        first(first), incr(incr), count(count), data(nullptr)
    {
        if (count) {
            data = new CountType[count];
            std::fill(data, data + count, 0);
        }
    }

    ~TSeries() {
        if (count > 0) {
            delete [] data;
        }
    }

    CountType operator[](int index) const {
        return data[index];
    }

    int getSizeInBytes() {
        return count * sizeof(CountType);
    }

    int binIndex(TimeType b) const {
        return (b - first) / incr;
    }

    TimeType first;     // [first,...,first + incr); [bin + incr, ..., bin + 2 x incr); ... [bin + incr, ..., bin + count x incr);
    TimeType incr;
    TimeType count;

    CountType *data;
};

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

void deque_split(const std::string &s, char delim,
                  std::deque<std::string> &elems)
{
    std::stringstream ss(s);
    std::string item;
    while(std::getline(ss, item, delim)) {
         elems.push_back(item);
    }
}


//-----------------------------------------------------------------------------
// Date and Time Auxiliar Functions
//-----------------------------------------------------------------------------

std::time_t mkTimestamp(std::string st, std::string format="%Y-%m-%d %H:%M:%S")
{
    struct tm tm;
    memset(&tm, 0, sizeof(struct tm));
    strptime(st.c_str(), format.c_str(), &tm);
    return mktime(&tm);
}

std::time_t mkTimestamp(int year, int month, int day, int hour, int min, int sec)
{
    struct tm timeinfo;
    memset(&timeinfo, 0, sizeof(struct tm));
    timeinfo.tm_year = year	 - 1900;
    timeinfo.tm_mon  = month - 1;
    timeinfo.tm_mday = day;
    timeinfo.tm_hour = hour;
    timeinfo.tm_min  = min;
    timeinfo.tm_sec  = sec;
    timeinfo.tm_isdst = 1; // <--- consider dst

    //
    return mktime ( &timeinfo );;
}

//-----------------------------------------------------------------------------
// FieldType
//-----------------------------------------------------------------------------

enum FieldType { FLOAT, UINT8, UINT64 };

static size_t getFieldTypeSize(FieldType field_type) {
    if (field_type == FLOAT) {
        return 4;
    }
    else if (field_type == UINT8) {
        return 1;
    }
    else if (field_type == UINT64) {
        return 8;
    }
    else throw std::string("invalid field type");
}

//-----------------------------------------------------------------------------
// Field
//-----------------------------------------------------------------------------

struct Field {

    enum Flag   {USER, ATTACHED};
    enum Status {ACTIVE, EXCLUDED};

    Field() {}
    Field(std::string name, FieldType type, int record_offset, Flag flag = USER):
        name(name), type(type), record_offset(record_offset), flag(flag), status(ACTIVE)
    {}

    void addValueName(int value, std::string name) {
        value_to_name[value] = name;
        name_to_value[name]  = value;
    }

//    int getValueForName(std::string name) const {
//        auto it = name_to_value.find(name);
//        if (it == name_to_value.end())
//            throw std::string("value not found on Field::getValueName");
//        else
//            return it->second;
//    }

    bool getValueForName(std::string name, int &value) const {
        auto it = name_to_value.find(name);
        if (it == name_to_value.end()) {
            return false;
        }
        else {
            value = it->second;
            return true;
        }
    }

    std::string getNameForValue(int value) const {
        auto it = value_to_name.find(value);
        if (it == value_to_name.end())
            return std::string("<null>");
        else
            return it->second;
    }

    int getFieldSize() const {
        return getFieldTypeSize(type);
    }


    bool isActive() const {
        return status == ACTIVE;
    }

    bool isExcluded() const {
        return status == EXCLUDED;
    }

    std::string name;
    FieldType   type;

    std::unordered_map<int, std::string> value_to_name;
    std::unordered_map<std::string, int> name_to_value;

    int         record_offset;
    Flag        flag;
    Status      status;
};

//-----------------------------------------------------------------------------
// Schema
//-----------------------------------------------------------------------------

struct Schema {

    Schema():
        record_size(0)
    {}

  ~Schema() {
    while (!fields.empty()) {
      delete fields.back();
      fields.pop_back();
    }
    fields.clear();
  }

    Field* addField(std::string field_name, FieldType field_type, Field::Flag flag = Field::USER) {
        Field *field = new Field(field_name, field_type, record_size, flag);
        if (flag == Field::USER) {
            record_size += getFieldTypeSize(field_type);
        }

        fields.push_back(field);
        name_to_field[field->name] = field;

        return field;
    }

    Field* getField(std::string field_name) const {
        auto it = name_to_field.find(field_name);
        if (it == name_to_field.end()) {
            return nullptr;
        } else {
            return it->second;
        }
    }

    std::string                             name;
    std::vector<Field*>                     fields;
    std::unordered_map<std::string, Field*> name_to_field;

    int record_size;

};

//-----------------------------------------------------------------------------
// Read Schema from Input Stream
//-----------------------------------------------------------------------------

std::istream &operator>>(std::istream &is, Schema &schema) {

    char buffer[1000];

    // read line
    while (1)
    {
        is.getline(buffer, 1000);
        std::string line(buffer);

        if (!line.size())
            break;

        boost::char_separator<char> sep(":, ");
        tokenizer<boost::char_separator<char>> tokens(line, sep);

        std::vector<std::string> tok;
        for ( auto it = tokens.begin(); it != tokens.end(); ++it)
        {
            std::string st(*it);
            if (st.size() > 0)
                tok.push_back(st);
        }

        if (tok.at(0).compare("name") == 0) {
            schema.name = tok.at(1);
        }
        else if (tok.at(0).compare("field") == 0) {
            std::string field_name(tok.at(1));
            std::string field_type_st(tok.at(2));
            FieldType field_type = FLOAT;
            if (field_type_st.compare("float") == 0) {
                field_type = FLOAT;
            }
            else if (field_type_st.compare("uint8") == 0) {
                field_type = UINT8;
            }
            else if (field_type_st.compare("uint64") == 0) {
                field_type = UINT64;
            }
            else {
                throw std::string("Unkown Field Type");
            }
            schema.addField(field_name, field_type);
        }
        else if (tok.at(0).compare("valname") == 0) {
            std::string field_name(tok.at(1));
            int value = atoi(tok.at(2).c_str());

            std::stringstream ss;
            for (size_t i=3;i<tok.size();i++) {
                ss << tok.at(i);
                if (i < tok.size()-1) {
                    ss << "_";
                }
            }
            std::string name(ss.str());

            Field *field = schema.getField(field_name);

            if (!field)
                throw std::string("Field not found");

            field->addValueName(value, name);
        }
    }

    return is;

}

//-----------------------------------------------------------------------------
// schema output impl.
//-----------------------------------------------------------------------------

std::ostream &operator<<(std::ostream &os, const FieldType &field_type) {
    if (field_type == UINT8) {
        os << "uint8";
    }
    else if (field_type == UINT64) {
        os << "uint64";
    }
    else if (field_type == FLOAT) {
        os << "float";
    }
    else throw std::string("unkown FieldType");
    return os;
}

std::ostream &operator<<(std::ostream &os, const Field &field) {
    os << "field: " << field.name << ", " << field.type << std::endl;
    for (auto it: field.value_to_name) {
        os << "valname: " << field.name << ", " << it.first << ", " << it.second << std::endl;
    }
    return os;
}

std::ostream &operator<<(std::ostream &os, const Schema &schema) {

    os << "name: " << schema.name << std::endl;
    for (auto f: schema.fields) {
        Field &field = *f;
        os << field;
    }
    return os;
}

//------------------------------------------------------------------------
// SpecializedSchema
//------------------------------------------------------------------------

struct SpecializedSchema {

    SpecializedSchema(const Schema &schema):
        schema(schema)
    {
        field_latitude  = schema.getField("latitude");
        field_longitude = schema.getField("longitude");
        field_time      = schema.getField("time");
        field_weight    = schema.getField("weight");

        // check if required fields are present
        if ((field_latitude  == nullptr || field_latitude->type  != FLOAT) ||
            (field_longitude == nullptr || field_longitude->type != FLOAT) ||
            (field_time      == nullptr || field_time->type      != UINT64))
            throw std::string("Required fields not matched: latitude (float), longitude (float), time (uint64)");

        std::unordered_set<Field*> required_fields { field_latitude, field_longitude, field_time };

        if (field_weight != nullptr) {
            if (field_weight->type != UINT64)
                throw std::string("weight field must be of type uint64");
            else
                required_fields.insert(field_weight);
        }

        // check if extra fields are all uint8
        for (auto f: schema.fields) {
            if (required_fields.count(f) == 0) {
                if (f->type != UINT8) {
                    throw std::string("Extra field type must be uint8");
                }
                extra_fields.push_back(f);
            }
        }
    }

    bool weightField() const {
        return field_weight != nullptr;
    }

    int getNumExtraFields() const {
        return extra_fields.size();
    }

    int getNumExtraFieldsUtil() const {
        int exclude_count = 0;
        for (Field *f: extra_fields) {
            if (f->isExcluded()) {
                exclude_count++;
            }
        }
        return extra_fields.size() - exclude_count;
    }

    Field* getExtraFieldUtil(std::string name, int &extra_field_index) const {
        int index_util = -1;
        for (size_t i=0;i < extra_fields.size();i++) {
            Field *f = extra_fields[i];
            if (f->isExcluded()) {
                continue;
            }
            index_util++;
            if (f->name.compare(name) == 0) {
                extra_field_index = index_util;
                return f;
            }
        }
        return nullptr;
    }

    Field* getExtraFieldUtil(int index) const {
        int index_util = -1;
        for (size_t i=0;i < extra_fields.size();i++) {
            Field *f = extra_fields[i];
            if (f->isExcluded()) {
                continue;
            }
            index_util++;
            if (index_util == index) {
                return f;
            }
        }
        return nullptr;
    }


    Field* getExtraField(std::string name, int &extra_field_index) const {
        for (size_t i=0;i < extra_fields.size();i++) {
            Field *f = extra_fields[i];
            if (f->name.compare(name) == 0) {
                extra_field_index = i;
                return f;
            }
        }
        return nullptr;
    }

    //
    Field *field_latitude;
    Field *field_longitude;
    Field *field_time;
    Field *field_weight;

    //
    std::vector<Field*> extra_fields;

    const Schema& schema;

};

//-----------------------------------------------------------------------------
// GeneralQuery
//-----------------------------------------------------------------------------

struct GeneralQueryDescription {

    GeneralQueryDescription(const SpecializedSchema &specialized_schema, std::string query_st);

    void dump(std::ostream &os) const;

    // bool getTraversalValues(Field *field, std::vector<uint8_t> values) const;

    bool fieldIsConstrainedByWhereClause(Field *field) const;

    bool isGroupBy(Field *field) const;

    const SpecializedSchema &specialized_schema;

    bool region_flag;  // if region flag is on, tile flag must be off and we are
    int  region_level;
    int  region_x0;
    int  region_y0;
    int  region_x1;
    int  region_y1;

    bool tile_flag;    // if tile flag is on, region flag must be off
    int  tile_level;
    int  tile_x;
    int  tile_y;
    int  tile_offset;

    bool tseries_flag; // if tseries flag is not defined we are assuming a count
    int  tseries_bin0;
    int  tseries_incr;
    int  tseries_count;

    std::vector<Field*>                                     group_by_fields;

    std::unordered_map<Field*, std::unordered_set<uint8_t>> where;

};


bool GeneralQueryDescription::isGroupBy(Field *field) const {
    auto it = std::find(this->group_by_fields.begin(), this->group_by_fields.end(), field);
    return it != group_by_fields.end();
}

bool GeneralQueryDescription::fieldIsConstrainedByWhereClause(Field *field) const {
    auto it = this->where.find(field);
    return it != where.end();
}

#if 0
bool GeneralQueryDescription::getTraversalValues(Field *field, std::vector<uint8_t> values) const {
    // check if field is constrained to a fixed set of
    // values because of a where clause
    if (fieldIsConstrainedByWhereClause(field)) {
        for (uint8_t value: this->where[field]) {
            values.push_back(value);
        }
        return true; // it is constrained
    }
    else if (this->isGroupBy(field)) {
        // add all values
        for (auto it: field->name_to_value) {
            values.push_back(it.second);
        }
        return true; // it is constrained
    }
    else return false; // no constraint on this field (follow the general path)
}
#endif


GeneralQueryDescription::GeneralQueryDescription(const SpecializedSchema &specialized_schema, std::string query_st):
    specialized_schema(specialized_schema),
    region_flag(false),
    region_level(0),
    region_x0(0),
    region_y0(0),
    region_x1(0),
    region_y1(0),
    tile_flag(false),    // if tile flag is on, region flag must be off
    tile_level(0),
    tile_x(0),
    tile_y(0),
    tile_offset(0),
    tseries_flag(false), // if tseries flag is not defined we are assuming a count
    tseries_bin0(0),
    tseries_incr(0),
    tseries_count(0)
{
    // std::cerr << query_st << std::endl;

    std::deque<std::string> tokens;
    deque_split(query_st, '/', tokens);

    std::unordered_set<Field*> set_group_by_fields;

    while (tokens.size() > 0) {

        std::string current_token = tokens.front(); tokens.pop_front();

        if (current_token.compare("field") == 0) {

            if (tokens.size() < 1) {
                std::cerr << "Warning: not enough parameters for field name: " << std::endl;
                break;
            }

            std::string field_name = tokens.front();
            tokens.pop_front();

            Field* field = specialized_schema.schema.getField(field_name);

            if (field == nullptr) {
                std::cerr << "Warning: group-by field not found: " << field_name << std::endl;
            }
            else if (std::count(group_by_fields.begin(),group_by_fields.end(),field) == 0) {
                // group_by_fields.push_back(field);
                set_group_by_fields.insert(field);
            }
            else {
                std::cerr << "Warning: group-by field already added: " << field_name << std::endl;
            }
        }

        else if (current_token.compare("tseries") == 0) {

            if (tokens.size() < 3) {
                std::cerr << "Warning: not enough parameters for tseries bin0/incr/count: " << std::endl;
                break;
            }

            this->tseries_flag  = true;
            this->tseries_bin0  = std::stoi(tokens.front()); tokens.pop_front();
            this->tseries_incr  = std::stoi(tokens.front()); tokens.pop_front();
            this->tseries_count = std::stoi(tokens.front()); tokens.pop_front();
        }

        else if (current_token.compare("tile") == 0) {

            if (tokens.size() < 4) {
                std::cerr << "Warning: not enough parameters for tile level/x/y/offset: " << std::endl;
                break;
            }

            this->tile_flag   = true;
            this->tile_level  = std::stoi(tokens.front()); tokens.pop_front();
            this->tile_x      = std::stoi(tokens.front()); tokens.pop_front();
            this->tile_y      = std::stoi(tokens.front()); tokens.pop_front();
            this->tile_offset = std::stoi(tokens.front()); tokens.pop_front();
        }

        else if (current_token.compare("region") == 0) {

            if (tokens.size() < 5) {
                std::cerr << "Warning: not enough parameters for region level/x0/y0/x1/y1: " << std::endl;
                break;
            }

            this->region_flag   = true;
            this->region_level  = std::stoi(tokens.front()); tokens.pop_front();
            this->region_x0     = std::stoi(tokens.front()); tokens.pop_front();
            this->region_y0     = std::stoi(tokens.front()); tokens.pop_front();
            this->region_x1     = std::stoi(tokens.front()); tokens.pop_front();
            this->region_y1     = std::stoi(tokens.front()); tokens.pop_front();
        }

        else if (current_token.compare("where") == 0) {

            if (tokens.size() < 1) {
                std::cerr << "Warning: not enough parameters for where " << std::endl;
                break;
            }

            // /where/age=15_or_less|16_to_24;income=50k_or_less|50k_75k
            std::string where_value_st = tokens.front(); tokens.pop_front();

            std::deque<std::string> label_values_clauses;
            deque_split(where_value_st, ';', label_values_clauses);

            while (!label_values_clauses.empty()) {

                std::string label_values_clause_st =  label_values_clauses.front(); label_values_clauses.pop_front();

                std::deque<std::string> label_values;
                deque_split(label_values_clause_st, '=', label_values);

                if (label_values.size() != 2) {
                    std::cerr << "Warning: invalid string, expected label=val1|val2: " << label_values_clause_st << std::endl;
                    break;
                }

                std::string label_st  = label_values.front(); label_values.pop_front();

                // find label
                Field *field = specialized_schema.schema.getField(label_st);

                if (field == nullptr) {
                    std::cerr << "Warning: field not found on where clause: " << label_st << std::endl;
                    break;
                }

                std::string values_st =  label_values.front(); label_values.pop_front();

                std::deque<std::string> values;
                deque_split(values_st, '|', values);


                while (!values.empty()) {
                    std::string value_st = values.front(); values.pop_front();
                    int value;
                    if (!field->getValueForName(value_st, value)) {
                        std::cerr << "Warning: field value not found:: " << value_st << std::endl;
                        break;
                    }
                    this->where[field].insert((uint8_t) value);
                }
            }
        }
    }

    for (Field* field: this->specialized_schema.extra_fields) {
        if (set_group_by_fields.count(field) > 0) {
            this->group_by_fields.push_back(field);
        }
    }
}

void GeneralQueryDescription::dump(std::ostream &os) const {

    os << "GeneralQuery [" << std::endl
       << "  flags:   " << std::endl
       << "    region_flag:   " << this->region_flag   << std::endl
       << "    region_level:  " << this->region_level  << std::endl
       << "    region_x0:     " << this->region_x0     << std::endl
       << "    region_y0:     " << this->region_y0     << std::endl
       << "    region_x1:     " << this->region_x1     << std::endl
       << "    region_y1:     " << this->region_y1     << std::endl
       << std::endl
       << "    tile_flag:     " << this->tile_flag     << std::endl
       << "    tile_level:    " << this->tile_level    << std::endl
       << "    tile_x:        " << this->tile_x        << std::endl
       << "    tile_y:        " << this->tile_y        << std::endl
       << "    tile_offset:   " << this->tile_offset   << std::endl
       << std::endl
       << "    tseries_flag:  " << this->tseries_flag     << std::endl
       << "    tseries_bin0:  " << this->tseries_bin0    << std::endl
       << "    tseries_incr:  " << this->tseries_incr        << std::endl
       << "    tseries_count: " << this->tseries_count        << std::endl
       << std::endl
       << "  group_by fields: " << std::endl;

    for (Field *f: this->group_by_fields) {
       os << "    field: " << f->name << std::endl;
    }

    os << std::endl;
    os << "  where: " << std::endl;

    for (auto it: this->where) {

        Field *f = it.first;
        std::unordered_set<uint8_t> &values = it.second;


        os << "    " << f->name << " = ";
        bool first = true;
        for (uint8_t value: values) {
            if (!first) {
                os << " | ";
            }
            first = false;
            os << f->getNameForValue(value) << "(value=" << (int)value << ")";
        }
        os << std::endl;
    }
    os << "]" << std::endl;
}


//---------------------------------------------------------------------------
// AddFlatTreeTraversalScheme
//---------------------------------------------------------------------------

template <bool Flag=true>
struct AddFlatTreeTraversalScheme {

    template <typename Index, int I>
    static void add(GeneralQueryDescription &general_query_description, typename Index::QueryType &query) {

        typedef typename Index::STreeTypesList Types;
        typedef typename boost::mpl::at_c<Types,I+1>::type FlatTreeType;
        typedef typename FlatTreeType::AddressType FlatTreeAddressType;

        Field *field = general_query_description.specialized_schema.getExtraFieldUtil(I);
        if (!field) {
            return;
        }

        if (general_query_description.isGroupBy(field)) {
            FlatTreeAddressType addr;
            query.add(addr,1); // dive one level deep on root node of flat tree.
        }
        else if (general_query_description.fieldIsConstrainedByWhereClause(field)) {
            for (uint8_t value: general_query_description.where[field]) {
                FlatTreeAddressType addr(value);
                query.add(addr,0); // dive one level deep on root node of flat tree.
            }
        }
        else {
            FlatTreeAddressType addr;
            query.add(addr,0); // dive one level deep on root node of flat tree.
        }
    }

};

template <>
struct AddFlatTreeTraversalScheme<false> {

    template <typename Index, int I>
    static void add(GeneralQueryDescription &/*general_query_description*/, typename Index::QueryType &/*query*/) {
    }
};


//--------------------------------------------------------------------------------
// Hash
//--------------------------------------------------------------------------------

struct Hash {
    template <typename T>
    size_t operator()(const T &obj) const
    {
        return obj.hash();
    }
};


//---------------------------------------------------------------------------
// Group (on group by)
//---------------------------------------------------------------------------

struct GroupByKey {

    GroupByKey()
    {}

    GroupByKey(std::vector<uint8_t> values):
        values(values)
    {}

    bool operator==(const GroupByKey &other) const {
        if (values.size() != other.values.size()) {
            return false;
        }
        for (size_t i=0;i<values.size();i++) {
            if (values[i] != other.values[i]) {
                return false;
            }
        }
        return true;
    }

    size_t hash() const {
        size_t hash = 0L;
        for (size_t i=0;i<values.size();i++) {
            hash ^= (((size_t)values[i]) << (i % 8));
        }
        return hash;
    }

    std::vector<uint8_t> values;
};

std::ostream &operator<<(std::ostream &os, const GroupByKey &key) {
    os << "GroupByKey[";
    bool first = true;
    for (uint8_t v: key.values) {
        if (!first) {
            os << ", ";
        }
        first = false;
        os << (int) v;
    }
    os << "]";
    return os;
}

//---------------------------------------------------------------------------
// GetFlagTreeAddressValue
//---------------------------------------------------------------------------

template <bool Flag=true>
struct GetFlagTreeAddressValue {
    template <typename Query, int I>
    static bool get(Query &query, uint8_t &value) {
        typedef typename mpl::at_c<typename Query::AddressTypeList, I+1>::type AddressType;
        AddressType addr;
        addr = query.template getCurrentAddress<AddressType>();
        value = addr.singleton_path_element;
        return true;
    }
};

template <>
struct GetFlagTreeAddressValue<false> {
    template <typename Query, int I>
    static bool get(Query &query, uint8_t &value) {
        return false;
    }
};

//---------------------------------------------------------------------------
// GeneralQueryResultsCollector
//---------------------------------------------------------------------------

template <typename Index>
struct GeneralQueryResultsCollector {

    // use the address of the front
    typedef Index                                              IndexType;
    typedef typename stree::Query<typename Index::STreeType>   QueryType;
    typedef typename Index::TimeSeriesType                     TimeSeriesType;
    typedef TSeries<typename Index::EntryType::TimeType>       TSeriesType;

    // typedef typename Index::FrontType::AddressType    TBAddr;
    // typedef Tile<TBAddr, PixelType>                   TBTile;

public:

    GeneralQueryResultsCollector(const GeneralQueryDescription &general_query_description);
    ~GeneralQueryResultsCollector();


    // TODO make STree dispatch "changeBaseAddress" signals
    void changeBaseAddress(int dimension, QueryType &query);

    void visit(TimeSeriesType &timeseries, QueryType &query);

    TSeriesType* getTSeriesCreateIfNeeded(GroupByKey &key);

public:

    const GeneralQueryDescription &general_query_description;

    int                  num_extra_dimensions;
    std::vector<Field*>  used_fields;
    std::vector<bool>    group_by_flags;
    std::vector<uint8_t> current_address_values;

    std::unordered_map<GroupByKey, TSeriesType*, Hash> result;
};

template <typename Index>
GeneralQueryResultsCollector<Index>::GeneralQueryResultsCollector(const GeneralQueryDescription &general_query_description):
    general_query_description(general_query_description)
{
    num_extra_dimensions = general_query_description.specialized_schema.getNumExtraFieldsUtil();
    group_by_flags.resize(num_extra_dimensions);
    used_fields.resize(num_extra_dimensions);
    current_address_values.resize(num_extra_dimensions);
    for (int i=0;i<num_extra_dimensions;i++) {
        used_fields[i]    = general_query_description.specialized_schema.getExtraFieldUtil(i);
        group_by_flags[i] = general_query_description.isGroupBy(used_fields[i]);
    }
}

template <typename Index>
GeneralQueryResultsCollector<Index>::~GeneralQueryResultsCollector() {
    // delete all time series collected
    for (auto it: result) {
        delete it.second;
    }
}


template <typename Index>
void GeneralQueryResultsCollector<Index>::changeBaseAddress(int dimension, QueryType &query) {

    // if (group_by_flags[dimension]) {
        // TBAddr base_addr = query.template getCurrentBaseAddress<TBAddr>();
        // std::cout << "changed group-by address: " << dimension << std::endl;
    // }

//    if (dimension == 0)
//    {
//        TBAddr base_addr = query.template getCurrentBaseAddress<TBAddr>();
//        // std::cout << "TileBuilder::changeBaseAddress " << base_addr << std::endl;
//        new_base_address = true;
//    }

}

template <typename Index>
void GeneralQueryResultsCollector<Index>::visit(TimeSeriesType &timeseries, QueryType &query) {
    // std::cout << "visit" << std::endl;

    uint8_t value;

    static const bool flag_0 = Index::Dimensions > 1;
    if (GetFlagTreeAddressValue<flag_0>::template get<QueryType,0>(query, value)) {
        current_address_values[0] = value;
    }

    static const bool flag_1 = Index::Dimensions > 2;
    if (GetFlagTreeAddressValue<flag_1>::template get<QueryType,1>(query, value)) {
        current_address_values[1] = value;
    }

    static const bool flag_2 = Index::Dimensions > 3;
    if (GetFlagTreeAddressValue<flag_2>::template get<QueryType,2>(query, value)) {
        current_address_values[2] = value;
    }

    static const bool flag_3 = Index::Dimensions > 4;
    if (GetFlagTreeAddressValue<flag_3>::template get<QueryType,3>(query, value)) {
        current_address_values[3] = value;
    }

    static const bool flag_4 = Index::Dimensions > 5;
    if (GetFlagTreeAddressValue<flag_4>::template get<QueryType,4>(query, value)) {
        current_address_values[4] = value;
    }

    static const bool flag_5 = Index::Dimensions > 6;
    if (GetFlagTreeAddressValue<flag_5>::template get<QueryType,5>(query, value)) {
        current_address_values[5] = value;
    }

#if 0
    for (int i=0;i<this->num_extra_dimensions;i++) {
        Field  *field = this->used_fields[i];
        uint8_t value = current_address_values[i];
        std::string value_name = (value != 255) ? field->getNameForValue(value) : std::string("empty");
        std::cout << "field: " << field->name << " = " << (int) value
                  << " or "
                  << value_name << std::endl;
    }
#endif

    GroupByKey key;
    for (int i=0;i<this->num_extra_dimensions;i++) {
        if (this->group_by_flags[i]) {
            key.values.push_back(current_address_values[i]);
        }
    }

    TSeriesType &tseries = *this->getTSeriesCreateIfNeeded(key);

    // std::cout << key << " address:    " << (void*) &tseries << " timeseries: " << &timeseries << std::endl;

    typedef typename Index::EntryType::TimeType  TimeType;
    typedef typename Index::EntryType::CountType CountType;

    TimeType first = this->general_query_description.tseries_bin0;
    TimeType incr  = this->general_query_description.tseries_incr;
    TimeType count = this->general_query_description.tseries_count;

    typedef typename TSeriesType::CountType TSeriesCountType;

    for (int i=0;i<count;i++) {
        TimeType a = first + i * incr;
        TimeType b = a + incr;
        TSeriesCountType count_i = static_cast<TSeriesCountType>(timeseries.getWindowTotal(a, b));
        // std::cout << "    from bin: " << a << " to " << b << ": " << count_i << std::endl;
        if (count_i > 0) {
            tseries.data[i] += count_i;
        }
    }
}

template <typename Index>
typename GeneralQueryResultsCollector<Index>::TSeriesType *GeneralQueryResultsCollector<Index>::getTSeriesCreateIfNeeded(GroupByKey &key)
{
    auto it = result.find(key);
    if (it == result.end()) {
        TSeriesType *tseries = new TSeriesType(general_query_description.tseries_bin0,
                                               general_query_description.tseries_incr,
                                               general_query_description.tseries_count);
        result[key] = tseries;
        return tseries;
    }
    else {
        return (*it).second;
    }
}

//------------------------------------------------------------------------
// getTypeName
//------------------------------------------------------------------------

#include <cxxabi.h>

template <typename T>
std::string getTypeName()
{
    char   buffer[10000];
    size_t length = 10000;
    int    status;
    abi::__cxa_demangle(typeid(T).name(),
                        buffer,
                        &length,
                        &status);
    buffer[length] = 0;
    return std::string(buffer);
}

//---------------------------------------------------------------------------
// StringBuf
//---------------------------------------------------------------------------

template <class Type>
std::stringbuf& operator<<(std::stringbuf& buf, const Type& var)
{
    buf.sputn(reinterpret_cast<const char*>(&var), sizeof var);
    return buf;
}

template <class Type>
std::stringbuf& operator>>(std::stringbuf& buf, Type& var)
{
    buf.sgetn(reinterpret_cast<char*>(&var), sizeof(var));
    return buf;
}

//---------------------------------------------------------------------------
// Tile
//---------------------------------------------------------------------------

typedef uint16_t PixelSize;
typedef uint8_t  PixelCoord;
// stypedef float    PixelType;

template <typename PixelType>
struct Record
{
    static const int OFF_I     = 0;
    static const int OFF_J     = sizeof(PixelCoord);
    static const int OFF_VALUE = sizeof(PixelCoord) + sizeof(PixelCoord);

    Record() {
        std::fill((char*) &data[0], (char*) &data[0] + sizeof(data), 0);
    }
    Record(PixelCoord i, PixelCoord j, PixelType value) {
        std::copy((char*)&i,     (char*)&i      + sizeof(PixelCoord), (char*) &data[OFF_I]);
        std::copy((char*)&j,     (char*)&j      + sizeof(PixelCoord), (char*) &data[OFF_J]);
        std::copy((char*)&value, (char*)&value  + sizeof(PixelType),  (char*) &data[OFF_VALUE]);
    }

    PixelCoord getI() const {
        PixelCoord result;
        std::copy((char*) &data[OFF_I], (char*) &data[OFF_I] + sizeof(PixelCoord), (char*) &result);
        // std::cout << "getI(): " << (int) result << std::endl;
        return result;
    }

    PixelCoord getJ() const {
        PixelCoord result;
        std::copy((char*) &data[OFF_J], (char*) &data[OFF_J] + sizeof(PixelCoord), (char*) &result);
        // std::cout << "getJ(): " << (int) result << std::endl;
        return result;
    }

    PixelType getValue() const {
        PixelType result;
        std::copy((char*) &data[OFF_VALUE], (char*) &data[OFF_VALUE] + sizeof(PixelType), (char*) &result);
        // std::cout << "getValue(): " << result << std::endl;
        return result;
    }

    uint8_t data[sizeof(PixelCoord) + sizeof(PixelCoord) + sizeof(PixelType)];
};


template <typename TileID, typename PixelType>
struct Tile
{
    Tile(TileID id);
    ~Tile();

    void  write(PixelCoord i, PixelCoord j, PixelType value);

    const void* getPtr() const;
    int         getSize() const;

    TileID                         id;
    std::vector<Record<PixelType>> data;
};


//---------------------------------------------------------------------------
// Output
//---------------------------------------------------------------------------

template <typename TileID, typename PixelType>
std::ostream& operator<<(std::ostream &os, const Tile<TileID, PixelType>& tile)
{
    int line = 1;
    for (auto &r: tile.data)
    {
        os << "[" << std::setw(5) << std::right << line << "]"
           << " i: " << std::setw(3) << std::right << (int) r.getI()
           << " j: " << std::setw(3) << std::right << (int) r.getJ()
           << " value: " << std::setw(7) << std::right << r.getValue() << std::endl;
        line++;
    }
    return os;
}

//-------------------------------------------------------------------------------
// Tile Implementations
//-------------------------------------------------------------------------------

template <typename TileID, typename PixelType>
Tile<TileID, PixelType>::Tile(TileID id):
    id(id)
{}

template <typename TileID, typename PixelType>
Tile<TileID, PixelType>::~Tile()
{}

template <typename TileID, typename PixelType>
void Tile<TileID, PixelType>::write(PixelCoord i, PixelCoord j, PixelType value)
{
    // std::cout << "sizeof Record: " << sizeof(Record<PixelType>) << std::endl;

    data.push_back(Record<PixelType>(i,j,value));

    // data << i << j << value;
    // data.pubsync();

    // std::cout << "orig:  " << (int) i << " / " << (int) j << " / " << value << std::endl;
    // std::cout << "check: " << (int) data.back().getI() << " / " << (int) data.back().getJ() << " / " << (int) data.back().getValue() << std::endl;
}

template <typename TileID, typename PixelType>
const void *Tile<TileID, PixelType>::getPtr() const
{
    return (const void*) &data[0];
}

template <typename TileID, typename PixelType>
int Tile<TileID, PixelType>::getSize() const
{
    return data.size() * sizeof(Record<PixelType>);
}

//---------------------------------------------------------------------------
// TileBuilder
//---------------------------------------------------------------------------

template <typename Index, typename Summary, typename PixelType>
struct TileBuilder
{

    // use the address of the front
    typedef Index                                     TBIndex;
    typedef typename Index::FrontType::AddressType    TBAddr;
    typedef typename stree::Query<Index>              TBQuery;
    typedef Tile<TBAddr, PixelType>                   TBTile;

    TileBuilder();
    ~TileBuilder();

    // TODO make STree dispatch "changeBaseAddress" signals
    void changeBaseAddress(int dimension, TBQuery &query);
    void visit(Summary &timeseries, TBQuery &query);

public:

    bool                  new_base_address;
    TBTile               *current_tile;
    std::vector<TBTile*>  tiles;


    Entry::TimeType       time_start;
    Entry::TimeType       time_end;
};



//--------------------------------------------------------------------------
// output time series
//--------------------------------------------------------------------------

template <typename TimeType>
std::ostream &operator<<(std::ostream &os , const TSeries<TimeType> &time_series);

//---------------------------------------------------------------------------
// TimeSeriesBuilder
//---------------------------------------------------------------------------

template <typename Index, typename Summary, typename EntryType>
struct TimeSeriesBuilder
{

    // use the address of the front
    typedef Index                                     TSBIndex;
    typedef typename Index::FrontType::AddressType    TSBAddr;
    typedef typename stree::Query<Index>              TSBQuery;
    typedef typename EntryType::TimeType              TimeType;
    typedef typename EntryType::CountType             CountType;
    typedef TSeries<TimeType>                         TSBTimeSeries;

    TimeSeriesBuilder();
    ~TimeSeriesBuilder();

    // TODO make STree dispatch "changeBaseAddress" signals
    void changeBaseAddress(int dimension, TSBQuery &query);
    void visit(Summary &timeseries, TSBQuery &query);

public:

    bool                         new_base_address;
    TSBTimeSeries               *current_time_series;
    std::vector<TSBTimeSeries*>  time_series_list;

    TimeType first;     // [first,...,first + incr); [bin + incr, ..., bin + 2 x incr); ... [bin + incr, ..., bin + count x incr);
    TimeType incr;
    TimeType count;
};


//--------------------------------------------------------------------------
// BaseIndex
//--------------------------------------------------------------------------

struct BaseIndex {
    virtual ~BaseIndex() {}
};

//--------------------------------------------------------------------------
// Here we are constrained to a fixed set of STrees:
//    - all have a QuadTree address space in the first dimension
//    - FlatTree extra dimensions (eg. 3 for
//      the mts dataset with Weekday, Hour, Category).
//    - all strees carry a time series as a final address content
//--------------------------------------------------------------------------

template <int NumFlatTrees, char QuadTreeLevels, typename Entry>
struct Index: public BaseIndex
{

    typedef Entry                                                        EntryType;

private: // Metafunctions to help produce the actual STree type

    template <typename Content, int K>
    struct FlatTreeFold
    {
        typedef flattree::FlatTree< typename FlatTreeFold< Content, K-1 >::type > type;
    };

    template <typename Content>
    struct FlatTreeFold<Content, 0>
    {
        typedef Content type;
    };

    //
    // STreeTypeListFold: auxiliar function to produce
    //     mpl::vector< Q<F^K<Content>, F^K<Content>, ..., F^1<Content> >
    //

    template <typename List, typename Content, int K>    /* Inductive Case */
    struct STreeTypeListFold
    {
        typedef typename boost::mpl::push_back<List, typename FlatTreeFold<Content, K>::type>::type AppendedList;
        typedef typename STreeTypeListFold<AppendedList, Content, K-1>::type type;
    };

    template <typename List, typename Content>
    struct STreeTypeListFold<List, Content, 1>
    {
        typedef typename boost::mpl::push_back<List, flattree::FlatTree<Content>>::type type;
    };

    template <typename List, typename Content>
    struct STreeTypeListFold<List, Content, 0>
    {
        typedef List type;
    };
public:

    // Dimensions before TimeSeries summary
    static const int Dimensions = NumFlatTrees + 1;

    // QuadTree Number of Levels
    static const int Levels = QuadTreeLevels;

    // TimeSeries
    typedef timeseries::TimeSeries<Entry> TimeSeriesType;

    // Initial Type: QuadTree
    typedef quadtree::QuadTree<
        QuadTreeLevels,
        typename FlatTreeFold<TimeSeriesType, NumFlatTrees>::type > QuadTreeType;

private:

    //
    // STreeTypeList
    //

    template <typename Content, int K>
    struct STreeTypeList
    {
        typedef boost::mpl::vector< QuadTreeType > InitialTypeList;
        typedef typename boost::mpl::if_c< K != 0,
            typename STreeTypeListFold<InitialTypeList, Content, K>::type,
            InitialTypeList>::type type;
    };

public:

    // STree Type

    typedef typename STreeTypeList<TimeSeriesType, NumFlatTrees>::type   STreeTypesList;

    typedef stree::STree<STreeTypesList , typename EntryType::TimeType > STreeType;

    typedef typename QuadTreeType::AddressType                           QuadTreeAddressType;
    typedef typename STreeType::AddressType                              STreeAddressType;

    typedef stree::Query<STreeType>                                      QueryType;

private:

    void _serveTimeSeries(Request &request, bool binary);
    void _serveTimeSeriesRange(Request &request, bool binary);

public:

    // register services into server
    void registerServices(Server &server);

    void serveTile(Request &request);

    void serveSchema(Request &request);

    void serveSchemaJson(Request &request);

    void serveTimeSeriesBinary(Request &request);

    void serveTimeSeriesText(Request &request);

    void serveTimeSeriesRangeText(Request &request);

    void serveTimeSeriesRangeBinary(Request &request);

    void serveShutdown(Request &request);

    void serveVersion(Request &request);

    void serveStartTiming(Request &request);

    void serveStopTiming(Request &request);

    void serveTimeBinFunction(Request &request);

    void serveStatus(Request &request);

    void serveRegister(Request &request);

    void serveBuildLog(Request &request);

    void serveQuery(Request &request);

    void dumpCounts(std::ostream &os, bool header_only=false);

    void dumpSchema(std::ostream &os);

    void dumpStatus(std::ostream &os);

    Index(const SpecializedSchema &specialized_schema, TimeBinFunction time_bin_function);

    ~Index();

public: // attributes

    STreeType                 stree;

    const SpecializedSchema  &specialized_schema;

    TimeBinFunction           time_bin_function;

    uint64_t                  count_points_added;

    int                       time_to_build_in_seconds;

    std::stringstream         build_log;

    mercator::TileCoordinate     min_lat;
    mercator::TileCoordinate     max_lat;
    mercator::TileCoordinate     min_lng;
    mercator::TileCoordinate     max_lng;
    uint32_t                     min_time_bin;
    uint32_t                     max_time_bin;
};


//-----------------------------------------------------------------------------
// TileServeHelper
//-----------------------------------------------------------------------------

template <typename Query, typename STreeTypesList, int index, bool flag=true>
struct TileServeHelper {
    static void add(Query &query, uint8_t value) {
        typename boost::mpl::at_c<STreeTypesList, index+1>::type::AddressType addr_aux(value);
        query.add(addr_aux, 0);
    }
};

template <typename Query, typename STreeTypesList, int index>
struct TileServeHelper<Query, STreeTypesList, index, false> {
    static void add(Query &query, uint8_t value)
    {}
};

//---------------------------------------------------------------------------
// Index Impl.
//---------------------------------------------------------------------------

template <int NumFlatTrees, char QuadTreeLevels, typename EntryType>
Index<NumFlatTrees, QuadTreeLevels, EntryType>::Index(const SpecializedSchema &specialized_schema,
                                                                  TimeBinFunction time_bin_function):
    BaseIndex(),
    specialized_schema(specialized_schema),
    time_bin_function(time_bin_function),
    count_points_added(0)
{}

template <int NumFlatTrees, char QuadTreeLevels, typename EntryType>
Index<NumFlatTrees, QuadTreeLevels, EntryType>::~Index()
{
    //std::cerr << "~Index<...> " << std::endl;
}

template <int NumFlatTrees, typename STreeTypesList, typename QueryType>
void addFlatTreeConstraints(QueryType &query,
                            const SpecializedSchema &specialized_schema,
                            const std::string &extra_constraints)
{
    // kind=Block_Call|Data_Cant_Connect;hour=0h|1h|2h

    boost::char_separator<char>            sep(";");
    tokenizer<boost::char_separator<char>> tokens(extra_constraints, sep);
    for ( auto it = tokens.begin(); it != tokens.end(); ++it)
    {
        std::string block_st(*it);
        if (block_st.size() == 0)
            continue;

        boost::char_separator<char>            sep2("=|");
        tokenizer<boost::char_separator<char>> tokens2(block_st, sep2);

        std::vector<std::string> block_tokens;
        for ( auto it2 = tokens2.begin(); it2 != tokens2.end(); ++it2)
        {
            std::string block_token_st(*it2);
            if (block_token_st.size() == 0)
                continue;
            block_tokens.push_back(block_token_st);
        }

        if (block_tokens.size() < 1)
            continue;

        int extra_field_index;
        Field *f = specialized_schema.getExtraFieldUtil(block_tokens[0], extra_field_index);
        // extra_field_index = f->

        if (f == nullptr) {
            std::cout << "Warning: could not find field named " << block_tokens[0] << std::endl;
            continue;
        }

        for (size_t i=1;i<block_tokens.size();i++) {
            try {
                int value;
                if (!f->getValueForName(block_tokens[i], value)) {
                    throw std::exception();
                }

                //std::cout << "-----> " << block_tokens[i] << " has value " << value <<std::endl;

                if (extra_field_index  == 0) {
                    static const bool flag = 0 < NumFlatTrees;
                    TileServeHelper<QueryType, STreeTypesList, 0, flag>::add(query, (uint8_t) value);
                } else if (extra_field_index  == 1) {
                    static const bool flag = 1 < NumFlatTrees;
                    TileServeHelper<QueryType, STreeTypesList, 1, flag>::add(query, (uint8_t) value);
                } else if (extra_field_index  == 2) {
                    static const bool flag = 2 < NumFlatTrees;
                    TileServeHelper<QueryType, STreeTypesList, 2, flag>::add(query, (uint8_t) value);
                } else if (extra_field_index  == 3) {
                    static const bool flag = 3 < NumFlatTrees;
                    TileServeHelper<QueryType, STreeTypesList, 3, flag>::add(query, (uint8_t) value);
                } else if (extra_field_index  == 4) {
                    static const bool flag = 4 < NumFlatTrees;
                    TileServeHelper<QueryType, STreeTypesList, 4, flag>::add(query, (uint8_t) value);
                }
            }
            catch (...) {
                std::cout << "Warning: could not find value names " << block_tokens[i] << " for field " << f->name;
                continue;
            }
        }
    }
}







template <int NumFlatTrees, char QuadTreeLevels, typename EntryType>
void Index<NumFlatTrees, QuadTreeLevels, EntryType>::serveTile(Request &request)
{

    //std::cout << "Index::serveTile" << std::endl;

    // tile request is of the form:
    //
    //    z x y t0 t1 <constraints>
    //
    // assume it will search 8 levels deeper on the quadtree
    // a matrix of 256x256 entries is the answer

    //
    // constraints language:
    //
    // <field_name>=(<field_value>)*
    //

    // 5 required parameters z/x/y/t0/t1
    int max_params = 6; // the 5 stands for z x y t0 t1
    int values[max_params];

    auto it = request.params.begin() + 2; // first entry in params is the url (empty right now)
                                          // second entry is the handler key, starting from
                                          // index 2 are the parameters;

    // initialize values with required parameters
    for (int i=0;i<6;i++) {
        if (it == request.params.end()) {
            request.respondJson(std::string("ERROR Query needs to have at least params: z x y t0 t1"));
        return;
        }
        values[i] = atoi(it->c_str());
        ++it;
    }

    // initialize required part of the query
    QueryType query;

    // OFF needs to be a number <= 8 because of its coordinates
    enum { Z=0, OFF=1, X=2, Y=3, T0=4, T1=5 };

    QuadTreeAddressType qaddr;
    qaddr.setLevelCoords(values[X], values[Y], values[Z]);
    query.add(qaddr, values[OFF]); // fixed offset of 8 levels (256x256 spatial bins)


    // std::cout << "query: " << values[Z] << "/" << values[OFF]
    //              << "/" << values[X] << "/" << values[Y] << "/"
    //              << values[T0] << "/" << values[T1] << std::endl;

    // the extra addresses
    if (it != request.params.end()) {

        std::string extra_constraints(*it);

        // add flattree constraints on the query object
        addFlatTreeConstraints<NumFlatTrees, STreeTypesList, QueryType>(query, specialized_schema, extra_constraints);
    }

    // clip time
    Entry::clipTime(values[T0]);
    Entry::clipTime(values[T1]);

    //
    TileBuilder<STreeType,
            TimeSeriesType,
            double> tile_builder;
    tile_builder.time_start = values[T0];
    tile_builder.time_end   = values[T1];

    //
    stree.visit(query, tile_builder);

    //
    if (tile_builder.current_tile)
    {
        request.respondOctetStream(tile_builder.current_tile->getPtr(), tile_builder.current_tile->getSize());
    }
    else
    {
        request.respondOctetStream((void *) 0, 0);
    }


//    std::stringstream ss;
//    if (tile_builder.current_tile != 0)
//    {
//        ss << *tile_builder.current_tile << std::endl;
//    }

    // request.respondOctetStream(tile_builder.current_tile->data.str());

    // request.respondJson(ss.str());

    // request.respondOctetStream((void *) &tile_builder.current_tile->data[0], );

}

template <int NumFlatTrees, char QuadTreeLevels, typename EntryType>
void Index<NumFlatTrees, QuadTreeLevels, EntryType>::serveTimeSeriesText(Request &request)
{
    this->_serveTimeSeries(request, false);
}

template <int NumFlatTrees, char QuadTreeLevels, typename EntryType>
void Index<NumFlatTrees, QuadTreeLevels, EntryType>::serveTimeSeriesBinary(Request &request)
{
    this->_serveTimeSeries(request, true);
}

template <int NumFlatTrees, char QuadTreeLevels, typename EntryType>
void Index<NumFlatTrees, QuadTreeLevels, EntryType>::_serveTimeSeries(Request &request, bool binary)
{

    //std::cout << "Index::serveTimeSeries" << std::endl;

    // tile request is of the form:
    //
    //    z x y t0 t1 <constraints>
    //
    // assume it will search 8 levels deeper on the quadtree
    // a matrix of 256x256 entries is the answer

    //
    // constraints language:
    //
    // <field_name>=(<field_value>)*
    //

    // 5 required parameters z/x/y/t0/t1
    int max_params = 6; // the 5 stands for z x y t0 t1
    int values[max_params];

    auto it = request.params.begin() + 2; // first entry in params is the url (empty right now)
                                          // second entry is the handler key, starting from
                                          // index 2 are the parameters;

    // initialize values with required parameters
    for (int i=0;i<6;i++) {
        if (it == request.params.end()) {
            request.respondJson(std::string("ERROR Query needs to have at least params: z x y bin_0 bin_incr bin_count"));
            return;
        }
        values[i] = atoi(it->c_str());
        ++it;
    }

    // initialize required part of the query
    QueryType query;

    enum { Z=0, X=1, Y=2, BIN_0=3, BIN_INCR=4, BIN_COUNT=5 };

    QuadTreeAddressType qaddr;
    qaddr.setLevelCoords(values[X], values[Y], values[Z]);
    query.add(qaddr, 0); // fixed offset of 8 levels (256x256 spatial bins)

    // std::cout << "query: "
    //           << values[Z]     << "/" << values[X]        << "/" << values[Y] << "/"
    //           << values[BIN_0] << "/" << values[BIN_INCR] << "/" << values[BIN_COUNT] << std::endl;

    // the extra addresses
    if (it != request.params.end()) {

        std::string extra_constraints(*it);

        // add flattree constraints on the query object
        addFlatTreeConstraints<NumFlatTrees, STreeTypesList, QueryType>(query, specialized_schema, extra_constraints);

    }

    //
    TimeSeriesBuilder<STreeType, TimeSeriesType, EntryType> time_series_builder;
    time_series_builder.first      = values[BIN_0];
    time_series_builder.incr       = values[BIN_INCR];
    time_series_builder.count      = values[BIN_COUNT];

    //
    stree.visit(query, time_series_builder);

//    std::cout << "time_series_builder.current_time_series: " << time_series_builder.current_time_series << std::endl;
//    std::cout << "time_series_builder.time_series_list.front: " << time_series_builder.time_series_list.front() << std::endl;

    //
    if (time_series_builder.current_time_series)
    {
        if (binary) {
            request.respondOctetStream(time_series_builder.current_time_series->data,
                                       time_series_builder.current_time_series->getSizeInBytes());
//            request.respondJson(std::string("Nothing"));
        }
        else { // text mode

            std::stringstream ss;
            ss << *time_series_builder.current_time_series;
            request.respondJson(ss.str());
        }
        // request.respondOctetStream(tile_builder.current_tile->getPtr(), tile_builder.current_tile->getSize());
    }
    else
    {
        request.respondOctetStream((void *) 0, 0);
    }
}


template <int NumFlatTrees, char QuadTreeLevels, typename EntryType>
void Index<NumFlatTrees, QuadTreeLevels, EntryType>::serveTimeSeriesRangeText(Request &request)
{
    this->_serveTimeSeriesRange(request, false);
}

template <int NumFlatTrees, char QuadTreeLevels, typename EntryType>
void Index<NumFlatTrees, QuadTreeLevels, EntryType>::serveTimeSeriesRangeBinary(Request &request)
{
    this->_serveTimeSeriesRange(request, true);
}


template <int Level>
bool clip(int &x, int &y) {

    bool clipped = false;

    static const int max = 1 << Level;
    if (x < 0) {
        x = 0;
        clipped = true;
    }
    else if (x > max) {
        x = max;
        clipped = true;
    }

    if (y < 0) {
        y = 0;
        clipped = true;
    }
    else if (y > max) {
        y = max;
        clipped = true;
    }
    return clipped;
}


template <int NumFlatTrees, char QuadTreeLevels, typename EntryType>
void Index<NumFlatTrees, QuadTreeLevels, EntryType>::_serveTimeSeriesRange(Request &request, bool binary)
{

    // std::cout << "Index::serveTimeSeriesRange" << std::endl;

    // tile request is of the form:
    //
    //    z x y t0 t1 <constraints>
    //
    // assume it will search 8 levels deeper on the quadtree
    // a matrix of 256x256 entries is the answer

    //
    // constraints language:
    //
    // <field_name>=(<field_value>)*
    //

    // 5 required parameters z/x/y/t0/t1
    int max_params = 8; // the 5 stands for z x y t0 t1
    int values[max_params];

    auto it = request.params.begin() + 2; // first entry in params is the url (empty right now)
    // second entry is the handler key, starting from
    // index 2 are the parameters;

    // initialize values with required parameters
    for (int i=0;i<8;i++) {
        if (it == request.params.end()) {
            request.respondJson(std::string("ERROR Query needs to have at least params: level x0 y0 x1 y1 bin_0 bin_incr bin_count"));
            return;
        }
        values[i] = atoi(it->c_str());
        ++it;
    }

    // initialize required part of the query
    QueryType query;

    enum { LEVEL=0, X0=1, Y0=2, X1=3, Y1=4, BIN_0=5, BIN_INCR=6, BIN_COUNT=7 };

    // std::cout << "query: "
    //           << values[LEVEL]     << "/"
    //           << values[X0]        << "/" << values[Y0] << "/"
    //           << values[X1]        << "/" << values[Y1] << "/"
    //           << values[BIN_0]     << "/" << values[BIN_INCR] << "/" << values[BIN_COUNT] << std::endl;


    if (values[LEVEL] > QuadTreeLevels) {
        int shift = values[LEVEL] - QuadTreeLevels;
        values[LEVEL] = QuadTreeLevels;
        values[X0] >>= shift;
        values[X1] >>= shift;
        values[Y0] >>= shift;
        values[Y1] >>= shift;

        // std::cout << "   query adjusted to: "
        //           << values[LEVEL]     << "/"
        //           << values[X0]        << "/" << values[Y0] << "/"
        //           << values[X1]        << "/" << values[Y1] << "/"
        //           << values[BIN_0]     << "/" << values[BIN_INCR] << "/" << values[BIN_COUNT] << std::endl;
    }

    // clip rectangle coordinates to valid ones
    bool clipped = false;
    clipped |= clip<QuadTreeLevels>(values[X0], values[Y0]);
    clipped |= clip<QuadTreeLevels>(values[X1], values[Y1]);

    if (clipped) {
        // std::cout << "clipped query: "
        //           << values[LEVEL]     << "/"
        //           << values[X0]        << "/" << values[Y0] << "/"
        //           << values[X1]        << "/" << values[Y1] << "/"
        //           << values[BIN_0]     << "/" << values[BIN_INCR] << "/" << values[BIN_COUNT] << std::endl;
    }

    static const int MAX_BIN_COUNT = 1 << 16;
    if (values[BIN_COUNT] > MAX_BIN_COUNT) {
        std::cout << "[WARNING] number of bins clipped from " << values[BIN_COUNT] << " to 2^16" << std::endl;
        values[BIN_COUNT] = MAX_BIN_COUNT;
    }
    else if (values[BIN_COUNT] < 0) {
        std::cout << "[WARNING] number of bins clipped from " << values[BIN_COUNT] << " to 0" << std::endl;
        values[BIN_COUNT] = 0;
    }



    QuadTreeAddressType qaddr_min, qaddr_max;
    qaddr_min.setLevelCoords(values[X0], values[Y0], values[LEVEL]);
    qaddr_max.setLevelCoords(values[X1], values[Y1], values[LEVEL]);
    query.add(qaddr_min, qaddr_max); // fixed offset of 8 levels (256x256 spatial bins)

    // the extra addresses
    if (it != request.params.end()) {

        std::string extra_constraints(*it);

        // add flattree constraints on the query object
        addFlatTreeConstraints<NumFlatTrees, STreeTypesList, QueryType>(query, specialized_schema, extra_constraints);

    }



    //
    TimeSeriesBuilder<STreeType, TimeSeriesType, EntryType> time_series_builder;
    time_series_builder.first      = values[BIN_0];
    time_series_builder.incr       = values[BIN_INCR];
    time_series_builder.count      = values[BIN_COUNT];

    //
    stree.visit(query, time_series_builder);

    //    std::cout << "time_series_builder.current_time_series: " << time_series_builder.current_time_series << std::endl;
    //    std::cout << "time_series_builder.time_series_list.front: " << time_series_builder.time_series_list.front() << std::endl;

    //
    if (time_series_builder.current_time_series)
    {
        if (binary) {
            request.respondOctetStream(time_series_builder.current_time_series->data,
                                       time_series_builder.current_time_series->getSizeInBytes());
            //            request.respondJson(std::string("Nothing"));
        }
        else { // text mode

            std::stringstream ss;
            ss << *time_series_builder.current_time_series;
            request.respondJson(ss.str());
        }
        // request.respondOctetStream(tile_builder.current_tile->getPtr(), tile_builder.current_tile->getSize());
    }
    else
    {
        request.respondOctetStream((void *) 0, 0);
    }
}

template <int NumFlatTrees, char QuadTreeLevels, typename EntryType>
void Index<NumFlatTrees, QuadTreeLevels, EntryType>::serveQuery(Request &request)
{

    //std::cout << "Index::serveQuery" << std::endl;

    //
    // tile request is of the form:
    //    TILE     -> tile/<level>/<x>/<y>/<level>/<offset>
    //    TSERIES  -> tseries/<bin0>/<incr>/<count>
    //    LABEL    -> label/<label_name>
    //    REGION   -> region/<level>/<x0>/<y0>/<x1>/<y1>
    //    WHERE    -> where/(<label_name>=<value>(|<value>)*)*
    //
    // assume it will search 8 levels deeper on the quadtree
    // a matrix of 256x256 entries is the answer
    //

    std::stringstream ss;
    auto it = request.params.begin() + 2; // first entry in params is the url (empty right now)
                                          // second entry is the handler key, starting from
                                          // index 2 are the parameters;

    bool first = true;
    while (it != request.params.end()) {
        if (!first) {
            ss << "/";
        }
        first = false;
        ss << *it;
        it++;
    }
    GeneralQueryDescription general_query_description(specialized_schema, ss.str());

    if (general_query_description.tseries_flag == false) {
        general_query_description.tseries_bin0  = 0;
        general_query_description.tseries_incr  = 65535;
        general_query_description.tseries_count = 1;
    }

    if (general_query_description.tile_flag == true) {
        request.respondJson("'tile' option not implemented through 'query' interface yet");
        return;
    }

    if (general_query_description.region_level > QuadTreeLevels) {
        request.respondJson("region exceeds quadtree level");
        return;
    }

    QueryType query;

    // set spatial scheme of traversal (range xor root node)...
    if (general_query_description.region_flag) {
        // ... range

        int level = general_query_description.region_level;
        int x0    = general_query_description.region_x0;
        int y0    = general_query_description.region_y0;
        int x1    = general_query_description.region_x1;
        int y1    = general_query_description.region_y1;

        if (level > QuadTreeLevels) {
            int shift = level - QuadTreeLevels;
            x0 >>= shift;
            y0 >>= shift;
            x1 >>= shift;
            y1 >>= shift;
        }

        bool clipped = false;
        clipped |= clip<QuadTreeLevels>(x0, y0);
        clipped |= clip<QuadTreeLevels>(x1, y1);

        if (x0 > x1) {
            std::swap(x0,x1);
        }

        if (y0 > y1) {
            std::swap(y0,y1);
        }

        QuadTreeAddressType qaddr_min, qaddr_max;
        qaddr_min.setLevelCoords(x0, y0, level);
        qaddr_max.setLevelCoords(x1, y1, level);

        // add spatial constraing to query
        query.add(qaddr_min, qaddr_max); // fixed offset of 8 levels (256x256 spatial bins)

    }
    else {
        // ... root node

        // no spatial contraint: all space
        QuadTreeAddressType qaddr_root;
        query.add(qaddr_root); // fixed offset of 8 levels (256x256 spatial bins)

    }

    // set traversal constraints to query
    static const bool flag_0 = NumFlatTrees>0;
    AddFlatTreeTraversalScheme< flag_0 >::template add<Index,0>(general_query_description, query);
    static const bool flag_1 = NumFlatTrees>1;
    AddFlatTreeTraversalScheme< flag_1 >::template add<Index,1>(general_query_description, query);
    static const bool flag_2 = NumFlatTrees>2;
    AddFlatTreeTraversalScheme< flag_2 >::template add<Index,2>(general_query_description, query);
    static const bool flag_3 = NumFlatTrees>3;
    AddFlatTreeTraversalScheme< flag_3 >::template add<Index,3>(general_query_description, query);
    static const bool flag_4 = NumFlatTrees>4;
    AddFlatTreeTraversalScheme< flag_4 >::template add<Index,4>(general_query_description, query);
    static const bool flag_5 = NumFlatTrees>5;
    AddFlatTreeTraversalScheme< flag_5 >::template add<Index,5>(general_query_description, query);


    typedef GeneralQueryResultsCollector<Index> Collector;
    typedef typename Collector::TSeriesType     TSeriesType;

    Collector collector(general_query_description);
    stree.visit(query, collector);

    ss.str("");
    ss << "[" << std::endl;
    bool first_2 = true;
    for (auto it: collector.result) {

        GroupByKey        key     = it.first;
        TSeriesType      &tseries = *it.second;

        // std::cout << key << std::endl;

        if (!first_2) {
            ss << ", " << std::endl;
        }
        first_2 = false;


        ss << "{";
        bool first = true;
        for (size_t i=0;i<key.values.size();i++) {
            Field *field = general_query_description.group_by_fields[i];
            if (!first) {
                ss << ", ";
            }
            first = false;
            ss << "\"" << field->name << "\":\"" << field->getNameForValue(key.values[i]) << "\"";
        }

        if (key.values.size() > 0) {
            ss << ", ";
        }

        ss << "\"values\":[";
        first = true;
        for (int i=0;i<tseries.count;i++) {
            if (!first) {
                ss << ", ";
            }
            first = false;
            ss << tseries.data[i];
        }
        ss << "]";
        ss << "}";
    }
    ss  << std::endl << "]";


    // ss.str("traversal done... need to implement the result collecting algorithm.");
    // general_query_description.dump(ss);
    request.respondJson(ss.str());

}

template <bool Flag=true>
struct PrintFlatTreeStats {
    template <typename Types, int index>
    static void print_flat_tree_counts(std::ostream &os, Field *field) {
        typedef typename boost::mpl::at_c<Types, index>::type Type;
        os << "FlatTrees on Field '" << field->name << "': " << Type::count_new << std::endl;
        os << "   Total Children: " << Type::count_entries << std::endl;
        // os << "   new:    " << Type::count_new       << std::endl;
        // os << "   delete: " << Type::count_delete    << std::endl;
        os << std::endl;
    }
};

template <>
struct PrintFlatTreeStats<false> {
    template <typename Types, int index>
    static void print_flat_tree_counts(std::ostream &/*os*/, Field */*field*/) {
    }
};

template <int NumFlatTrees, char QuadTreeLevels, typename EntryType>
void Index<NumFlatTrees, QuadTreeLevels, EntryType>::dumpStatus(std::ostream &os) {

    os << "Number of Points: " << this->count_points_added  << std::endl;

    os << "Time to Index:    " << this->time_to_build_in_seconds  << "s." << std::endl;

    os << std::endl;

    os << "Resident Memory: " << memory_util::MemInfo::get().res_B()   << "B"  << std::endl;
    os << "Resident Memory: " << memory_util::MemInfo::get().res_MB()  << "MB" << std::endl;
    os << "Virtual  Memory: " << memory_util::MemInfo::get().virt_B()  << "B"  << std::endl;
    os << "Virtual  Memory: " << memory_util::MemInfo::get().virt_MB() << "MB" << std::endl;

    os << std::endl;

    os << "Number of TimeSeries:        " << TimeSeriesType::count_new       << std::endl;
    os << "Number of TimeSeries entries:   " << TimeSeriesType::count_used_bins << std::endl;
    os << "Number of TimeSeries adds:      " << TimeSeriesType::count_num_adds  << std::endl;

    os << std::endl;

    static const bool flag_1 = NumFlatTrees>0;
    PrintFlatTreeStats< flag_1 >::template print_flat_tree_counts<Index::STreeTypesList,1>(os, specialized_schema.getExtraFieldUtil(0));
    static const bool flag_2 = NumFlatTrees>1;
    PrintFlatTreeStats< flag_2 >::template print_flat_tree_counts<Index::STreeTypesList,2>(os, specialized_schema.getExtraFieldUtil(1));
    static const bool flag_3 = NumFlatTrees>2;
    PrintFlatTreeStats< flag_3 >::template print_flat_tree_counts<Index::STreeTypesList,3>(os, specialized_schema.getExtraFieldUtil(2));
    static const bool flag_4 = NumFlatTrees>3;
    PrintFlatTreeStats< flag_4 >::template print_flat_tree_counts<Index::STreeTypesList,4>(os, specialized_schema.getExtraFieldUtil(3));
    static const bool flag_5 = NumFlatTrees>4;
    PrintFlatTreeStats< flag_5 >::template print_flat_tree_counts<Index::STreeTypesList,5>(os, specialized_schema.getExtraFieldUtil(4));
    static const bool flag_6 = NumFlatTrees>5;
    PrintFlatTreeStats< flag_6 >::template print_flat_tree_counts<Index::STreeTypesList,6>(os, specialized_schema.getExtraFieldUtil(5));

    os << "QuadTree Stats: " << std::endl;
    quadtree::Stats<QuadTreeType> stats;
    stats.initialize(stree.root);
    stats.dumpReport(os);

    os << std::endl;

    os << "Schema" << std::endl;

    this->dumpSchema(os);
    // this->specialized_schema.schema.

    os << "Mongoose threads: " << g_server->mongoose_threads << std::endl;
}

template <bool Flag=true>
struct CountFlatTreeNodes {
    template <typename Types, int index>
    static void count(uint64_t &count) {
        typedef typename boost::mpl::at_c<Types, index>::type Type;
        count += Type::count_new + Type::count_entries;
        // os << "FlatTrees on Field '" << field->name << "': " << Type::count_new << std::endl;
        // os << "   Total Children: " << Type::count_entries << std::endl;
        // os << "   new:    " << Type::count_new       << std::endl;
        // os << "   delete: " << Type::count_delete    << std::endl;
        // os << std::endl;
    }
};

template <>
struct CountFlatTreeNodes<false> {
    template <typename Types, int index>
    static void count(uint64_t &count) {
    }
};



template <int NumFlatTrees, char QuadTreeLevels, typename EntryType>
void Index<NumFlatTrees, QuadTreeLevels, EntryType>::dumpCounts(std::ostream &os, bool header_only) {

    quadtree::Stats<QuadTreeType> stats;
    stats.initialize(stree.root);

    uint64_t quad_tree_max_level  = stats.N;
    uint64_t num_quad_tree_leaves = stats.getNumNodesByLevel(stats.N);
    uint64_t num_quad_tree_nodes  = stats.totalNodes;

    uint64_t leaf_entries      = TimeSeriesType::count_used_bins;
    uint64_t key_count         = STreeType::count_proper_leaf_unique_adds;
    int multiplicity           = (quad_tree_max_level+1) * (1 << NumFlatTrees);
    double actual_multiplicity = leaf_entries / (double) key_count;

    uint64_t fnodes = 0; // flat tree nodes

    static const bool flag_1 = NumFlatTrees>0;
    CountFlatTreeNodes< flag_1 >::template count<Index::STreeTypesList,1>(fnodes);
    static const bool flag_2 = NumFlatTrees>1;
    CountFlatTreeNodes< flag_2 >::template count<Index::STreeTypesList,2>(fnodes);
    static const bool flag_3 = NumFlatTrees>2;
    CountFlatTreeNodes< flag_3 >::template count<Index::STreeTypesList,3>(fnodes);
    static const bool flag_4 = NumFlatTrees>3;
    CountFlatTreeNodes< flag_4 >::template count<Index::STreeTypesList,4>(fnodes);
    static const bool flag_5 = NumFlatTrees>4;
    CountFlatTreeNodes< flag_5 >::template count<Index::STreeTypesList,5>(fnodes);
    static const bool flag_6 = NumFlatTrees>5;
    CountFlatTreeNodes< flag_6 >::template count<Index::STreeTypesList,6>(fnodes);

    uint64_t inodes = fnodes + num_quad_tree_nodes;

    double cardinality_log_2 = 0.0;
    double leaf_cardinality_log_2 = 0.0;
    cardinality_log_2       += log2((pow(4,quad_tree_max_level+1)-1.0)/3.0);
    leaf_cardinality_log_2  += log2(pow(4,quad_tree_max_level));
    for (Field *f: this->specialized_schema.extra_fields) {
        if (f->isExcluded()) {
            cardinality_log_2      += log2(1.0 + f->name_to_value.size());
            leaf_cardinality_log_2 += log2(f->name_to_value.size());
        }
    }

    uint64_t size = TimeSeriesType::count_used_bins + inodes;

    std::string dataset_name = this->specialized_schema.schema.name;

    std::stringstream ss;
    ss << dataset_name
       << "_q" << (quad_tree_max_level + 1);
    for (Field *f: this->specialized_schema.extra_fields) {
        if (f->isActive()) {
            ss << "_" << f->name << f->name_to_value.size();
        }
    }
    std::string name = ss.str();

    if (header_only) {
        os << "name points keycount size mult actualmult leafcardlogtwo cardlogtwo time dataset resmem nodes dim qmaxlevel qleaves qnodes leaves lentries ladds inodes" << std::endl;
        return;
    }

    std::string sep(" ");
    os << name << sep;                                     // name:            name of the dataset
    os << this->count_points_added << sep;                 // points:          number of points indexed
    os << STreeType::count_proper_leaf_unique_adds << sep; // keycount:        number of unique elements added
    os << size << sep;                                     // size:            inodes + lentries
    os << multiplicity << sep;                             // mult:            multiplicity of schema
    os << actual_multiplicity << sep;                      // actualmult:      lentries/keycount
    os << leaf_cardinality_log_2 << sep;                   // leafcardlogtwo:  leaves cardinality log ten
    os << cardinality_log_2 << sep;                        // cardlogtwo:      cardinality log ten
    os << this->time_to_build_in_seconds << sep;           // time:            time to build the index in seconds
    os << dataset_name << sep;                             // dataset:         name of the dataset
    os << memory_util::MemInfo::get().res_B() << sep;      // resmem:          resident memory
    os << inodes + TimeSeriesType::count_new << sep;       // nodes:           inodes + leaves (circles and rectangles in the figure of the paper)
    os << Dimensions   << sep;                             // dim:             dimension of schema
    os << quad_tree_max_level << sep;                      // qmaxlevel:       quadtree max level label (levels go from 0 to qmaxlevel, they count qmaxlevel+1)
    os << num_quad_tree_leaves << sep;                     // qleaves:         quadtree leaves
    os << num_quad_tree_nodes << sep;                      // qnodes:          total nodes on the quadtree
    os << TimeSeriesType::count_new << sep;                // leaves:          number of time series
    os << TimeSeriesType::count_used_bins << sep;          // lentries:        number of time series entriesleaf entries
    os << TimeSeriesType::count_num_adds << sep;           // ladds:           number of add calls to a timeseries
    os << inodes;                                          // inodes:          internal nodes of the quadtree (circles in the figure of the paper)
}

template <int NumFlatTrees, char QuadTreeLevels, typename EntryType>
void Index<NumFlatTrees, QuadTreeLevels, EntryType>::serveStatus(Request &request)
{
    std::stringstream ss;

    this->dumpStatus(ss);

    request.respondJson(ss.str());
}

template <int NumFlatTrees, char QuadTreeLevels, typename EntryType>
void Index<NumFlatTrees, QuadTreeLevels, EntryType>::serveRegister(Request &request)
{
    std::stringstream ss;
    ss << "Registering nanocube with database on " << gs_nanodb << std::endl;

    std::stringstream nano_up;
    nano_up << "curl http://" << gs_nanodb << "/online/"
            << gs_hname
            << "/" << gs_hport
            << "/" << gs_schema_name
            << "/" << "0.0.1";
    std::string system_call = nano_up.str();
    system(system_call.c_str());

    request.respondJson(ss.str());
}

template <int NumFlatTrees, char QuadTreeLevels, typename EntryType>
void Index<NumFlatTrees, QuadTreeLevels, EntryType>::serveBuildLog(Request &request)
{
    request.respondJson(this->build_log.str());
}

template <int NumFlatTrees, char QuadTreeLevels, typename EntryType>
void Index<NumFlatTrees, QuadTreeLevels, EntryType>::serveShutdown(Request &request)
{
    std::string passcode = "bogus";
    if (request.params.size() >= 3)
        passcode = request.params[2];

    if (passcode == g_server->passcode() || passcode == "nownownownow") {
        std::cout << "Index::serveShutdown in 3 seconds" << std::endl;
        std::stringstream ss;
        ss << "Going to shutdown stree_serve in 3 seconds";
        request.respondJson(ss.str());

        sleep(3);
        g_server->stop();
    } else {
        std::cout << "Index::serveShutdown unauthorizied request ignored" << std::endl;
        std::stringstream ss;
        ss << "Ignoring unauthorized server shutdown";
        request.respondJson(ss.str());
    }
}

template <int NumFlatTrees, char QuadTreeLevels, typename EntryType>
void Index<NumFlatTrees, QuadTreeLevels, EntryType>::serveVersion(Request &request)
{
    std::stringstream ss;
    //ss << "\"VERSION\"";
    ss << "\"" << VERSION << "\"";
    request.respondJson(ss.str());
}

template <int NumFlatTrees, char QuadTreeLevels, typename EntryType>
void Index<NumFlatTrees, QuadTreeLevels, EntryType>::serveStartTiming(Request &request)
{
    //std::cout << "Index::serveStartTiming" << std::endl;

    std::stringstream ss;

    if (g_server->toggleTiming(true)) {
        ss << "Requests are now being timed";
    } else
        ss << "Problem starting the timing service... Sorry";

    request.respondJson(ss.str());
}

template <int NumFlatTrees, char QuadTreeLevels, typename EntryType>
void Index<NumFlatTrees, QuadTreeLevels, EntryType>::serveStopTiming(Request &request)
{
    //std::cout << "Index::serveStopTiming" << std::endl;

    std::stringstream ss;
    if (g_server->toggleTiming(false)) {
        ss << "Requests are no longer being timed";
    } else {
        ss << "Problem stoping the timing service... Sorry";
    }
    request.respondJson(ss.str());
}


template <int NumFlatTrees, char QuadTreeLevels, typename EntryType>
void Index<NumFlatTrees, QuadTreeLevels, EntryType>::dumpSchema(std::ostream &os)
{
    os << this->specialized_schema.schema;

    if (specialized_schema.getNumExtraFields() != specialized_schema.getNumExtraFieldsUtil()) {
        os << "<WARNING> THESE FIELDS WERE EXCLUDED FROM THE INDEX:" << std::endl;
        for (Field* f: specialized_schema.schema.fields) {
            if (f->isExcluded()) {
                os << f->name << std::endl;
            }
        }
    }

}


template <int NumFlatTrees, char QuadTreeLevels, typename EntryType>
void Index<NumFlatTrees, QuadTreeLevels, EntryType>::serveSchema(Request &request)
{
    //std::cout << "Index::serveSchema" << std::endl;

    std::stringstream ss;
    this->dumpSchema(ss);

    request.respondJson(ss.str());
}

template <int NumFlatTrees, char QuadTreeLevels, typename EntryType>
void Index<NumFlatTrees, QuadTreeLevels, EntryType>::serveSchemaJson(Request &request)
{

    std::stringstream ss;

    bool first = true;

    ss << "{" << std::endl;

    ss << "\"name\": \""  << specialized_schema.schema.name << "\"," << std::endl;
    ss << "\"tbin\": \""  << this->time_bin_function.getSpecificationString() << "\"," << std::endl;
    ss << "\"sbin\": "  << Index::Levels << "," << std::endl;
    ss << "\"minlat\": " << this->min_lat << "," << std::endl;
    ss << "\"maxlat\": " << this->max_lat << "," << std::endl;
    ss << "\"minlng\": " << this->min_lng << "," << std::endl;
    ss << "\"maxlng\": " << this->max_lng << "," << std::endl;
    ss << "\"mintbin\": " << this->min_time_bin << "," << std::endl;
    ss << "\"maxtbin\": " << this->max_time_bin << "," << std::endl;
    ss << "\"fields\": [" << std::endl;
    for (Field* field: specialized_schema.extra_fields) {
        if (field->isExcluded())
            continue;

        if (!first) {
            ss << ",";
        }
        first = false;

        ss << "{ \"name\": \"" << field->name << "\", \"values\": [";

        bool _first = true;
        for (auto it: field->value_to_name) {
            if (!_first) {
                ss << ",";
            }
            _first = false;
            ss << "\"" << it.second << "\"";
        }
        ss << "]}" << std::endl;
    }
    ss << "]}" << std::endl;

    request.respondJson(ss.str());
}

template <int NumFlatTrees, char QuadTreeLevels, typename EntryType>
void Index<NumFlatTrees, QuadTreeLevels, EntryType>::serveTimeBinFunction(Request &request)
{
    //std::cout << "Index::serveTimeBinFunction" << std::endl;
    std::string st = time_bin_function.getSpecificationString();
    request.respondJson(st);
}

// register services into server
template <int NumFlatTrees, char QuadTreeLevels, typename EntryType>
void Index<NumFlatTrees, QuadTreeLevels, EntryType>::registerServices(Server &server)
{
    {
        RequestHandler handler = std::bind(&Index::serveTile, this, std::placeholders::_1);
        server.registerHandler("tile", handler);
    }

    {
        RequestHandler handler = std::bind(&Index::serveSchema, this, std::placeholders::_1);
        server.registerHandler("schema", handler);
    }

    {
        RequestHandler handler = std::bind(&Index::serveTimeSeriesText, this, std::placeholders::_1);
        server.registerHandler("tseries_text", handler);
    }

    {
        RequestHandler handler = std::bind(&Index::serveTimeSeriesBinary, this, std::placeholders::_1);
        server.registerHandler("tseries", handler);
    }

    {
        RequestHandler handler = std::bind(&Index::serveTimeSeriesRangeBinary, this, std::placeholders::_1);
        server.registerHandler("rtseries", handler);
    }

    {
        RequestHandler handler = std::bind(&Index::serveTimeSeriesRangeText, this, std::placeholders::_1);
        server.registerHandler("rtseries_text", handler);
    }

    {
        RequestHandler handler = std::bind(&Index::serveShutdown, this, std::placeholders::_1);
        server.registerHandler("shutdown", handler);
    }

    {
        RequestHandler handler = std::bind(&Index::serveVersion, this, std::placeholders::_1);
        server.registerHandler("version", handler);
    }

    {
        RequestHandler handler = std::bind(&Index::serveStartTiming, this, std::placeholders::_1);
        server.registerHandler("start_timing", handler);
    }

    {
        RequestHandler handler = std::bind(&Index::serveStopTiming, this, std::placeholders::_1);
        server.registerHandler("stop_timing", handler);
    }

    {
        RequestHandler handler = std::bind(&Index::serveTimeBinFunction, this, std::placeholders::_1);
        server.registerHandler("tbin", handler);
    }

    {
        RequestHandler handler = std::bind(&Index::serveStatus, this, std::placeholders::_1);
        server.registerHandler("status", handler);
    }

    {
        RequestHandler handler = std::bind(&Index::serveRegister, this, std::placeholders::_1);
        server.registerHandler("register", handler);
    }

    {
        RequestHandler handler = std::bind(&Index::serveSchemaJson, this, std::placeholders::_1);
        server.registerHandler("schema_json", handler);
    }

    {
        RequestHandler handler = std::bind(&Index::serveQuery, this, std::placeholders::_1);
        server.registerHandler("query", handler);
    }

    {
        RequestHandler handler = std::bind(&Index::serveBuildLog, this, std::placeholders::_1);
        server.registerHandler("buildlog", handler);
    }
}

//---------------------------------------------------------------------------
// TileBuilder Impl.
//---------------------------------------------------------------------------

template <typename Index, typename Summary, typename PixelType>
TileBuilder<Index, Summary, PixelType>::TileBuilder():
    new_base_address(true),
    current_tile(nullptr)
{}

template <typename Index, typename Summary, typename PixelType>
TileBuilder<Index, Summary, PixelType>::~TileBuilder()
{
    current_tile = nullptr;
    for (auto t: tiles)
        delete t;
}

// TODO make STree signal this
template <typename Index, typename Summary, typename PixelType>
void TileBuilder<Index, Summary, PixelType>::changeBaseAddress(int dimension, TBQuery &query)
{
    if (dimension == 0)
    {
        TBAddr base_addr = query.template getCurrentBaseAddress<TBAddr>();
        // std::cout << "TileBuilder::changeBaseAddress " << base_addr << std::endl;
        new_base_address = true;
    }
}

template <typename Index, typename Summary, typename PixelType>
void TileBuilder<Index, Summary, PixelType>::visit(Summary &timeseries, TBQuery &query)
{
    TBAddr addr = query.template getCurrentAddress<TBAddr>();

    static const int Levels = Index::FrontType::AddressSize;

    if (new_base_address)
    {
        TBAddr base_addr = query.template getCurrentBaseAddress<TBAddr>();

        // std::cout << "TileBuilder::visit's base address " << base_addr << std::endl;

        // tile size
        PixelSize sideSize = 1 << (addr.level - base_addr.level);

        assert (sideSize <= 256);

        current_tile = new TBTile(base_addr);
        new_base_address = false;
        tiles.push_back(current_tile);
    }

    // calculate relative address i and j
    TBTile &t = *current_tile;

    int i = (addr.y - current_tile->id.y) >> (Levels - addr.level);
    int j = (addr.x - current_tile->id.x) >> (Levels - addr.level);

    assert (i < 256 && j < 256);

    // t.sideSize-1 - addr.getLevelYCoord(); // - t.id.x;
    // PixelCoord j = addr.getLevelXCoord(); // - t.id.y;

    PixelType count = static_cast<PixelType>(timeseries.getWindowTotal(this->time_start, this->time_end));
    // auto it0 = lower_bound(timeseries.entries.

//    std::cout << "Last Time Bin: " << timeseries.entries.back().time << std::endl;
//    std::cout << "Time Start   : " << time_start << std::endl;
//    std::cout << "Time End     : " << time_end << std::endl;

    if (count > 0) {
        t.write((PixelCoord) i, (PixelCoord) j, count);
        // std::cout << "i: " << i << "   j: " << j << "  count: " << count << std::endl;
    }

    // t(i, j) += count;

    // std::cout << "Address: " << addr << std::endl;
    // std::cout << "Tile: "    << i << ", " << j << std::endl;
    // std::cout << "Visiting timeseries: " << std::endl;
    // timeseries.dump(std::cout);

}

//---------------------------------------------------------------------------
// TimeSeries Output Impl.
//---------------------------------------------------------------------------

template <typename TimeType>
std::ostream &operator<<(std::ostream &os , const TSeries<TimeType> &time_series) {
    for (int i=0;i<time_series.count;i++) {
        os << "["
           << (time_series.first + i * time_series.incr) << ", "
           << time_series.first + (i+1) * time_series.incr << ") = " << time_series.data[i] << std::endl;
    }
    return os;
}

//---------------------------------------------------------------------------
// TimeSeriesBuilder Impl.
//---------------------------------------------------------------------------

template <typename Index, typename Summary, typename EntryType>
TimeSeriesBuilder<Index, Summary, EntryType>::TimeSeriesBuilder():
    new_base_address(true),
    current_time_series(nullptr)
{}

template <typename Index, typename Summary, typename EntryType>
TimeSeriesBuilder<Index, Summary, EntryType>::~TimeSeriesBuilder()
{
//    std::cout << "~TimeSeriesBuilder" << std::endl;
//    std::cout << "  current_time_series: " << current_time_series << std::endl;
//    std::cout << "  time_series_list.front: " << (time_series_list.size() > 0 ? time_series_list.front() : nullptr) << std::endl;

    current_time_series = nullptr;
    for (TSBTimeSeries *t: time_series_list)
        delete t;
}

// TODO make STree signal this
template <typename Index, typename Summary, typename EntryType>
void TimeSeriesBuilder<Index, Summary, EntryType>::changeBaseAddress(int dimension, TSBQuery &query)
{
#if 1
    if (dimension == 0)
    {
//        TSBAddr base_addr = query.template getCurrentBaseAddress<TSBAddr>();
//        std::cout << "TileBuilder::changeBaseAddress " << base_addr << std::endl;
        new_base_address = true;
    }
#endif
}

template <typename Index, typename Summary, typename EntryType>
void TimeSeriesBuilder<Index, Summary, EntryType>::visit(Summary &timeseries, TSBQuery &query)
{
    TSBAddr addr = query.template getCurrentAddress<TSBAddr>();

    // static const int Levels = Index::FrontType::AddressSize;

    if (new_base_address)
    {
        TSBAddr base_addr = query.template getCurrentBaseAddress<TSBAddr>();

//        std::cout << "TileBuilder::visit's base address " << base_addr << std::endl;

        // tile size
        current_time_series = new TSBTimeSeries(first, incr, count);
        time_series_list.push_back(current_time_series);

//        std::cout << "current_time_series: " << current_time_series << std::endl;
//        std::cout << "time_series_list.front: " << time_series_list.front() << std::endl;

        new_base_address = false;
    }

    assert (current_time_series);

    // calculate relative address i and j
    TSBTimeSeries &t = *current_time_series;

    typedef typename TSBTimeSeries::CountType TSeriesCountType;

    for (int i=0;i<t.count;i++) {
        TimeType a = first + i * incr;
        TimeType b = a + incr;
        TSeriesCountType count_i = static_cast<TSeriesCountType>(timeseries.getWindowTotal(a, b));
//        std::cout << "from bin: " << a << " to " << b << ": " << count_i << std::endl;
        if (count_i > 0) {
            t.data[i] += count_i;
        }
    }
}

//-----------------------------------------------------------------------------
// Aux
//-----------------------------------------------------------------------------

template <typename Address, typename STreeTypesList, int index, bool flag=true>
struct Aux {
    static void add(Address &addr, uint8_t value) {
        typename boost::mpl::at_c<STreeTypesList, index+1>::type::AddressType addr_aux(value);
        addr.set(addr_aux);
    }
};

template <typename Address, typename STreeTypesList, int index>
struct Aux<Address, STreeTypesList, index, false> {
    static void add(Address &addr, uint8_t value)
    {}
};

//-----------------------------------------------------------------------------
// Region
//-----------------------------------------------------------------------------

struct Region {

    inline bool contains(float lat, float lon) const{
        return lat0 <= lat && lat <= lat1 && lon0 <= lon && lon <= lon1;
    }

    float lat0;
    float lat1;
    float lon0;
    float lon1;
};

// Get current date/time, format is YYYY-MM-DD.HH:mm:ss
const std::string currentDateTime() {
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    // Visit http://www.cplusplus.com/reference/clibrary/ctime/strftime/
    // for more information about date/time format
    strftime(buf, sizeof(buf), "%Y-%m-%d_%X", &tstruct);

    return buf;
}

//-----------------------------------------------------------------------------
// TimeDerivedFields
//-----------------------------------------------------------------------------

struct TimeDerivedFields {

    enum Field {HOUR_OF_DAY=0x1, DAY_OF_WEEK=0x2, MONTH=0x4};

    static std::uint64_t  cached_hour_bin;
    static std::uint8_t   cached_hour;
    static std::uint8_t   cached_day_of_week;
    static std::uint8_t   cached_month;

    static void extract(std::time_t t, uint8_t &hour, uint8_t &day_of_week, uint8_t &month) {
        std::tm *timeinfo = localtime(&t);
        month        = timeinfo->tm_mon + 1;
        hour         = timeinfo->tm_hour;
        day_of_week  = timeinfo->tm_wday; // 0 is Sun, 1 is Mon, 2 is Tue ... 6 is Sat
        if (day_of_week == 0)
            day_of_week = 7; // 1 is Mon ... 7 is Sun
    }

    static void init() {
        uint8_t hour, day_of_week, month;
        extract(0, hour, day_of_week, month);

        cached_hour_bin    = 0;
        cached_hour        = hour;
        cached_day_of_week = day_of_week;
        cached_month       = month;
    }

    inline static void cache_extract(std::time_t t, uint8_t &hour, uint8_t &day_of_week, uint8_t &month) {

        uint64_t hour_bin = t / 3600;
        if (hour_bin == cached_hour_bin) {
            hour        = cached_hour;
            day_of_week = cached_day_of_week;
            month       = cached_month;
        }
        else {
            std::tm *timeinfo = localtime(&t);
            month        = timeinfo->tm_mon + 1;
            hour         = timeinfo->tm_hour;
            day_of_week  = timeinfo->tm_wday; // 0 is Sun, 1 is Mon, 2 is Tue ... 6 is Sat
            if (day_of_week == 0)
                day_of_week = 7; // 1 is Mon ... 7 is Sun

            // save in cache
            cached_hour_bin = hour_bin;
            cached_hour = hour;
            cached_day_of_week = day_of_week;
            cached_month = month;
        }
    }

    struct Set {

        Set():
            bitset(0),
            size(0)
        {}

        void insert(std::string st) {
            if (st.compare("h") == 0 || st.compare("hour") == 0) {
                if (!(bitset & HOUR_OF_DAY)) {
                    bitset |= HOUR_OF_DAY;
                    size++;
                }
            }
            else if (st.compare("d") == 0 || st.compare("dayofweek") == 0) {
                if (!(bitset & DAY_OF_WEEK)) {
                    bitset |= DAY_OF_WEEK;
                    size++;
                }
            }
            else if (st.compare("m") == 0 || st.compare("month") == 0) {
                if (!(bitset & MONTH)) {
                    bitset |= MONTH;
                    size++;
                }
            }
        }

        bool contains(Field f) {
            return (f & bitset);
        }

        uint32_t bitset;
        int size;
    };

};

std::uint64_t  TimeDerivedFields::cached_hour_bin = 0;
std::uint8_t   TimeDerivedFields::cached_hour = 0;
std::uint8_t   TimeDerivedFields::cached_day_of_week = 0;
std::uint8_t   TimeDerivedFields::cached_month = 0;

//-----------------------------------------------------------------------------
// buildIndex
//-----------------------------------------------------------------------------

template <int NumFlatTrees, int QuadTreeLevels>
BaseIndex *buildIndex(const SpecializedSchema &specialized_schema,
                      Server &server,
                      NumberOfPoints max_points,
                      TimeBinFunction time_bin_function,
                      NumberOfPoints report_frequency,
                      std::vector<Region> &regions,
                      TimeDerivedFields::Set attach_set) {

    typedef typename Entry::TimeType  TimeType;
    typedef typename Entry::CountType CountType;

    typedef Index<NumFlatTrees, QuadTreeLevels, Entry> IndexType;
    IndexType *index = new IndexType(specialized_schema, time_bin_function);

    int record_size = specialized_schema.schema.record_size;

    char buffer[record_size];

    uint8_t extra_field_values[NumFlatTrees];

    NumberOfPoints total = 0;
    NumberOfPoints count = 0;
    NumberOfPoints count_problems = 0;
    NumberOfPoints count_outside_region = 0;

    stopwatch::Stopwatch     stopwatch;
    stopwatch.start();

    bool filter_regions = regions.size() > 0;

    index->min_lat = 1 << QuadTreeLevels;
    index->max_lat = 0;
    index->min_lng = 1 << QuadTreeLevels;
    index->max_lng = 0;
    index->min_time_bin = 1 << (8 * sizeof(TimeType));
    index->max_time_bin = 0;

    std::string counts_filename;
    {
        std::stringstream ss;
        ss << "nanocube-count-" << currentDateTime() << ".txt";
        counts_filename = ss.str();
    }
//    std::ofstream counts_file(counts_filename);
//    index->dumpCounts(counts_file,true);
    index->dumpCounts(index->build_log,true);

    while (std::cin.read(&buffer[0], record_size))
    {

        if (report_frequency != 0 && total > 0 && total % report_frequency == 0) {

            index->time_to_build_in_seconds = stopwatch.timeInSeconds();

            std::cout << "total: " << total
                      << " res. mem: "
                      << memory_util::MemInfo::get().res_B() << "B "
                      << memory_util::MemInfo::get().res_MB() << "MB "
                      << "time: " << index->time_to_build_in_seconds
                      << " valid "     << count
                      << "     problems " << count_problems
                      << " outside region " << (count_outside_region)
                      <<  std::endl;

#ifdef COLLECT_MEMUSAGE
            // dump to counts file
//            index->dumpCounts(counts_file);
//            counts_file << std::endl;

            index->dumpCounts(index->build_log);
            index->build_log << std::endl;
#endif // COLLECT_MEMUSAGE
        }
        total++;


        float  latitude  = *((float*)  (&buffer[0] + specialized_schema.field_latitude->record_offset));
        float  longitude = *((float*)  (&buffer[0] + specialized_schema.field_longitude->record_offset));
        time_t time      = *((time_t*) (&buffer[0] + specialized_schema.field_time->record_offset));
        uint64_t weight  = specialized_schema.weightField() ? *((uint64_t*) (&buffer[0] + specialized_schema.field_weight->record_offset)) : 1;

//        std::cout << "lat,lon: " << latitude << " " << longitude << std::endl;
//        std::cout << "time:    " << time << std::endl;

        int i = 0;
        for (Field* f: specialized_schema.extra_fields) {

            if (f->isExcluded()) {
                continue;
            }

            if (f->flag == Field::USER)  {
                uint8_t value = *((uint8_t*) (&buffer[0] + f->record_offset));
                extra_field_values[i++] = value;
            }
            else if (f->flag == Field::ATTACHED) {
                // when we find the first attached index we add its entries and exit loop
                // these fields must be the last ones

                uint8_t hour, day_of_week, month;

                TimeDerivedFields::cache_extract(time, hour, day_of_week, month);

                if (attach_set.contains(TimeDerivedFields::HOUR_OF_DAY)) {
                    extra_field_values[i++] = hour;
                }
                if (attach_set.contains(TimeDerivedFields::DAY_OF_WEEK)) {
                    extra_field_values[i++] = day_of_week;
                }
                if (attach_set.contains(TimeDerivedFields::MONTH)) {
                    extra_field_values[i++] = month;
                }
                break;
            }
        }

        // identify tile
        mercator::TileCoordinate tx, ty;
        mercator::MercatorProjection::tileOfLongitudeLatitude(longitude,
                                                              latitude,
                                                              QuadTreeLevels,
                                                              tx, ty);

        if (tx >= (1 << QuadTreeLevels) || (tx < 0) || ty >= (1 << QuadTreeLevels) || (ty < 0)) {
            count_problems++;
            continue; // point outside bounds
        }

        // if point is not inside any region do not consider it
        if (filter_regions) {
            bool filter_out = true;
            for (const Region &region: regions) {
                if (region.contains(latitude, longitude)) {
                    filter_out = false;
                    break;
                }
            }

            if (filter_out) {
                count_outside_region++;
                continue;
            }
        }

        if (tx < index->min_lng) index->min_lng = tx;
        if (tx > index->max_lng) index->max_lng = tx;
        if (ty < index->min_lat) index->min_lat = ty;
        if (ty > index->max_lat) index->max_lat = ty;

        // calendar
        // std::tm time_record = *gmtime(&base_record->time);
        typedef typename IndexType::STreeAddressType Address;
        typedef typename IndexType::STreeTypesList   STreeTypesList;

        Address addr;

        // set addr
        typename IndexType::QuadTreeAddressType qaddr(tx, ty, QuadTreeLevels);
        addr.set(qaddr);

        if (NumFlatTrees > 0) {
            static const bool flag = NumFlatTrees > 0;
            Aux<Address, STreeTypesList, 0, flag >::add(addr, extra_field_values[0]);
        }

        if (NumFlatTrees > 1) {
            static const bool flag = NumFlatTrees > 1;
            Aux<Address, STreeTypesList, 1, flag>::add(addr, extra_field_values[1]);
        }

        if (NumFlatTrees > 2) {
            static const bool flag = NumFlatTrees > 2;
            Aux<Address, STreeTypesList, 2, flag>::add(addr, extra_field_values[2]);
        }

        if (NumFlatTrees > 3) {
            static const bool flag = NumFlatTrees > 3;
            Aux<Address, STreeTypesList, 3, flag>::add(addr, extra_field_values[3]);
        }

        if (NumFlatTrees > 4) {
            static const bool flag = NumFlatTrees > 4;
            Aux<Address, STreeTypesList, 4, flag>::add(addr, extra_field_values[4]);
        }

        // tile is
#ifdef DEBUG_STREE_SERVER
        std::cout << "coord: "
                  << latitude << "  "
                  << longitude << std::endl;
        std::cout << "tile: " << tx << "  " << ty;
        for (int j=0;j<NumFlatTrees;j++) {
            std::cout << " " << (int)extra_field_values[j];
        }
        std::cout << std::endl;
#endif

        // std::cout << "Add: " << qaddr << std::endl;

        // set address
//        typename STreeTS::AddressType address;
//        address.set(a0);

        TimeType time_bin = 0;
        // get timestamp
        if (time != 0) {
            time_bin = time_bin_function.getBin(time);

//            static const int timeBinSizeInHours = 1;
//            static const std::time_t t0 = mkTimestamp(2010,1,1,0,0,0);
//            static const std::time_t timeBinSizeInSeconds = timeBinSizeInHours * 60 * 60; // this is one day unit
//            time_bin = static_cast<TimeBin>((time - t0)/timeBinSizeInSeconds);
        }

        if (time_bin < index->min_time_bin) index->min_time_bin = time_bin;
        if (time_bin > index->max_time_bin) index->max_time_bin = time_bin;

        // add timestamp to stree
        if ((max_points > 0) && ((count + weight) > max_points)) {
            weight -= ((count + weight) - max_points);
        }
        count += weight;
        index->count_points_added += weight;

        index->stree.add(addr, time_bin, weight);

//        if ((hitS.getNumAdds() % showProgressStep) == 0)
//        {
//            showProgress();
//            // std::clog << memory_util::MemInfo::get();
//            // std::clog << hitS.report;
//        }
        // std::cout << "hitS: " << hitS.getNumAdds() << std::endl;

        if (max_points > 0 && count == max_points) {
            break;
        }
    }

    index->time_to_build_in_seconds = stopwatch.timeInSeconds();

#ifdef COLLECT_MEMUSAGE

    // save status on a file
    try {
//        std::stringstream ss;
//        ss << "nanocube-info-" << currentDateTime() << ".txt";
//        std::string filename = ss.str();
//        std::ofstream of(filename);
//        index->dumpStatus(of);
    }
    catch (...) {
        std::cout << "Could not save status file after building the index" << std::endl;
    }

    // save status on a file
    try {
        // index->dumpCounts(counts_file);
        index->dumpCounts(index->build_log);
    }
    catch (...) {
        std::cout << "Could not save count file after building the index" << std::endl;
    }

#endif // COLLECT_MEMUSAGE

    // add records to the index
    std::cout << "Added " << count << " records to the index in "  << stopwatch.timeInSeconds() <<  std::endl;

    // register services
    index->registerServices(server);

    return index;
}

//-----------------------------------------------------------------------------
// signal handlers
//-----------------------------------------------------------------------------
static int exit_status = EXIT_SUCCESS;

void exitFunction(void)
{
    std::cerr << "Shuting down..." << std::endl;

    std::stringstream nano_down;
    nano_down << "curl http://" << gs_nanodb << "/offline/"
              << gs_hname
              << "/" << gs_hport
              << "/" << exit_status;
    std::string system_call = nano_down.str();
    system(system_call.c_str());
}

void signalHandler(int signum)
{
    std::cerr << "Caught signal " << signum << std::endl;
    exit_status = signum;
    exit(signum);
}

void setupSignalHandlers()
{
    // If SIGHUP was being ignored, keep it that way...
    void (*previousSignalHandler)(int);
    previousSignalHandler = signal(SIGHUP, signalHandler);
    if (previousSignalHandler == SIG_IGN) {
        std::cout << "Ignoring SIGHUP as before" << std::endl;
        signal(SIGHUP, SIG_IGN);
    }

    signal(SIGINT,  signalHandler);
    signal(SIGQUIT, signalHandler);
    signal(SIGILL,  signalHandler);
    signal(SIGTRAP, signalHandler);
    signal(SIGABRT, signalHandler);
    signal(SIGFPE,  signalHandler);
    signal(SIGSEGV, signalHandler);
    signal(SIGTERM, signalHandler);

    atexit(exitFunction);
}

std::string getHostname()
{
    struct addrinfo hints, *result;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags    = AI_CANONNAME;

    std::string ret;

    char hname[256];
    if (gethostname(hname, 256) != 0) {
        sprintf(hname, "Unknown");
    }

    // could use http, ssh, login, echo, ...
    if (getaddrinfo(hname, "ssh", &hints, &result) != 0) {
        // If this fails, fall back to gethostname()
        ret = hname;
    } else {
        ret = result->ai_canonname;
    }
    return ret;
}


//-----------------------------------------------------------------------------
// random string
//-----------------------------------------------------------------------------
static std::string random_string() {
    static const char charset[] =
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789";
    static bool first_time = true;

    if (first_time) {
        srand(time(NULL));
        first_time = false;
    }
    const int length = 8;
    char s[length+1];

    for (int i = 0; i < length; ++i) {
        s[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    s[length] = '\0';
    return s;
}

//-----------------------------------------------------------------------------
// main
//-----------------------------------------------------------------------------
int main(int argc, char** argv)
{
    std::string compile_options = "";

#ifdef FAST_COMPILE
    compile_options += "FAST_COMPILE ";
#endif

#ifdef COLLECT_MEMUSAGE
    compile_options += "COLLECT_MEMUSAGE ";
#endif

    std::cout << "nanocube (updated at 09:38am on Jun 3, 2013)" << std::endl;
    std::cout << "   compile options: " << compile_options << std::endl;

    int quadtree_bits = -1;

    // create a server
    g_server = new Server;
    Server &server = *g_server;
    int port = g_server->port;
    NumberOfPoints max_points = 0;
    NumberOfPoints report_frequency  = 1000000;
    int mongoose_threads = 10;

    bool help = false;

    bool noserve = false;

    TimeBinFunction time_bin_function;
    bool time_bin_flag = false;

    std::vector<Region> regions;

    std::unordered_set<std::string> excluded_dimensions;

    TimeDerivedFields::Set attach_set;

    // find type of dump
    try {
        std::string       type_st("");
        for (int i=1;i<argc;i++) {
            std::vector<std::string> tokens;
            split(std::string(argv[i]),'=',tokens);
            if (tokens.size() < 1) {
                continue;
            }
            if (tokens[0].compare("--help") == 0) {
                help = true;
            }
            if (tokens[0].compare("--noserve") == 0) {
                noserve = true;
            }
            if (tokens.size() < 2) {
                continue;
            }
            if (tokens[0].compare("--port") == 0) {
                port = std::stoi(tokens[1]);
            }
            if (tokens[0].compare("--threads") == 0) {
                mongoose_threads = std::stoi(tokens[1]);
                if (mongoose_threads < 0 ||  mongoose_threads > 20)
                    mongoose_threads = 10;
            }
            if (tokens[0].compare("--nanodb") == 0) {
                std::vector<std::string> vec;
                split(tokens.at(1),':',vec);
                if (vec.size() != 2) {
                    std::cerr << "Unexpected format for nanodb.  Expecting --nanodb=hostname:port" << std::endl;
                    exit(1);
                } else {
                    gs_nanodb = tokens[1];
                    std::cout << "gs_nanodb=" << gs_nanodb << std::endl;
                }
            }
            if (tokens[0].compare("--levels") == 0) {
                quadtree_bits = std::stoi(tokens[1]);
            }
            if (tokens[0].compare("--max") == 0) {
                max_points = std::stoull(tokens[1]);
            }
            if (tokens[0].compare("--tbin") == 0) {
                time_bin_function.init(tokens[1]);
                time_bin_flag = true;
            }
            if (tokens[0].compare("--rf") == 0) { // report frequency
                report_frequency = std::stoull(tokens[1]);
            }
            if (tokens[0].compare("--exclude") == 0) {
                std::vector<std::string> vec;
                split(tokens.at(1),',',vec);
                for (auto st: vec) {
                    excluded_dimensions.insert(st);
                }
            }
            if (tokens[0].compare("--attach") == 0) {
                std::vector<std::string> vec;
                split(tokens.at(1),',',vec);
                for (auto st: vec) {
                    attach_set.insert(st);
                }
            }
            if (tokens[0].compare("--regions") == 0) {
                std::vector<std::string> vec;
                split(tokens.at(1),',',vec);
                size_t k = 0;
                if (k+4 <= vec.size()) {
                    Region region;
                    region.lat0 = std::stof(vec.at(k));
                    region.lat1 = std::stof(vec.at(k+1));
                    region.lon0 = std::stof(vec.at(k+2));
                    region.lon1 = std::stof(vec.at(k+3));
                    regions.push_back(region);
                    k+=4;
                }
            }
        }
    }
    catch (...)
    {}

    if (help) {
        std::stringstream ss;
        ss << "stree_serve [options] < <dmp_file>"                                       << std::endl
           << "cat <dmp_file> | stree_serve [options]"                                   << std::endl
           << "LD_PRELOAD=$TCMALLOC stree_serve [options] < <dmp_file>"                  << std::endl
           << ""                                                                         << std::endl
           << "options:"                                                                 << std::endl
           << "   --max=n"                                                               << std::endl
           << "         build index with the first n points"                             << std::endl
           << ""                                                                         << std::endl
           << "   --rf=n"                                                                << std::endl
           << "         report progress every n points added"                            << std::endl
           << ""                                                                         << std::endl
           << "   --exclude=field1([,field])*"                                           << std::endl
           << "         do not index these fields"                                       << std::endl
           << ""                                                                         << std::endl
           << "   --tbin=n  (e.g. 2010_1h, 2010-06_1d)"                                  << std::endl
           << "         time binning scheme"                                             << std::endl
           << "         2010_1h"                                                         << std::endl
           << "             [2010-01-01 00:00:00, 2010-01-01 01:00:00) -> 0"             << std::endl
           << "             [2010-01-01 01:00:00, 2010-01-01 02:00:00) -> 1"             << std::endl
           << "             [2010-01-01 02:00:00, 2010-01-01 03:00:00) -> 2"             << std::endl
           << "             ..."                                                         << std::endl
           << "         2010-06_1d"                                                      << std::endl
           << "             [2010-06-01 00:00:00, 2010-07-01 00:00:00) -> 0"             << std::endl
           << "             [2010-07-01 00:00:00, 2010-08-01 00:00:00) -> 1"             << std::endl
           << "             [2010-08-01 00:00:00, 2010-09-01 00:00:00) -> 2"             << std::endl
           << "             ..."                                                         << std::endl
           << ""                                                                         << std::endl
           << "   --levels=n  (default: 20)"                                             << std::endl
           << "         spatial subdivisions: 0,5,10,15,17,20,25"                        << std::endl
           << ""                                                                         << std::endl
           << "   --port=n    (default: 29512)"                                          << std::endl
           << "         service port"                                                    << std::endl
           << ""                                                                         << std::endl
           << "   --threads=n    (default: 10)"                                          << std::endl
           << "         number of mongoose threads"                                      << std::endl
           << ""                                                                         << std::endl
           << "   --regions=lat0,lat1,lon0,lon1([,lat0,lat1,lon0,lon1])*"                << std::endl
           << "         only index points in the union of these regions"                 << std::endl
           << ""                                                                         << std::endl
           << "   --noserve"                                                             << std::endl
           << "         do not start server"                                             << std::endl
           << ""                                                                         << std::endl
           << "   --nanodb=hostname:port   (default: dt-jklosow.research.att.com:29999)" << std::endl
           << "         register nanocube with hostname at port"                         << std::endl
           << ""                                                                         << std::endl
           << "   --help"                                                                << std::endl
           << "         help message"                                                    << std::endl
           << ""                                                                         << std::endl
           << "   --attach=field(,field)*"                                               << std::endl
           << "         attach field based on time field where field is one"             << std::endl
           << "         of the of the followiong:"                                       << std::endl
           << "            d or dayofweek (1 Mon to 7 Sun)"                              << std::endl
           << "            h or hour (0 to 24)"                                          << std::endl
           << "            m or month (1 to 12)"                                         << std::endl
           << "" << std::endl;
        std::cout << ss.str();
        exit(0);
    }

    if (!time_bin_flag) {
        std::cout << "--tbin option is required" << std::endl;
        std::cout << "   e.g. --tbin=2010_1h or --tbin=2010-06_10h or --tbin=2013-01-01_15m" << std::endl;
        std::cout << "   note that time_t on the records should map to non-negative numbers up to 65535)" << std::endl;
        exit(1);
    }

    if (quadtree_bits < 0) {
        std::cout << "--levels option is required" << std::endl;
        std::cout << "   e.g. --levels=20" << std::endl;
        exit(1);
    }

    // (-74.2042 , 21.078) (-73.7862 , 21.161)

    // read schema of the stream of records
    Schema schema;
    std::cin >> schema;

    for (std::string field_name: excluded_dimensions) {
        Field* field = schema.getField(field_name);
        if (field) {
            field->status = Field::EXCLUDED;
        }
    }

    // init time derived fields cache
    TimeDerivedFields::init();

    // attach extra columns to schema
    if (attach_set.size > 0) {
        if (attach_set.contains(TimeDerivedFields::HOUR_OF_DAY)) {
            Field *field = schema.addField("hour",UINT8, Field::ATTACHED);
            field->flag = Field::ATTACHED;
            for (int i=0;i<24;i++) {
                field->addValueName(i,std::to_string(i));
            }
        }
        if (attach_set.contains(TimeDerivedFields::DAY_OF_WEEK)) {
            Field *field = schema.addField("dayofweek",UINT8, Field::ATTACHED);
            field->addValueName(1,"Mon");
            field->addValueName(2,"Tue");
            field->addValueName(3,"Wed");
            field->addValueName(4,"Thr");
            field->addValueName(5,"Fri");
            field->addValueName(6,"Sat");
            field->addValueName(7,"Sun");
        }
        if (attach_set.contains(TimeDerivedFields::MONTH)) {
            Field *field = schema.addField("month",UINT8, Field::ATTACHED);
            field->addValueName(1,"Jan");
            field->addValueName(2,"Feb");
            field->addValueName(3,"Mar");
            field->addValueName(4,"Apr");
            field->addValueName(5,"May");
            field->addValueName(6,"Jun");
            field->addValueName(7,"Jul");
            field->addValueName(8,"Aug");
            field->addValueName(9,"Sep");
            field->addValueName(10,"Oct");
            field->addValueName(11,"Nov");
            field->addValueName(12,"Dec");
        }
    }
    std::cout << schema;

    // try creating a specialized schema (separates the required dimensions and extra dimensions)
    SpecializedSchema specialized_schema(schema);

    BaseIndex *index;

    // number of extra files in the schema (not excluded)
    int num_extra_fields = specialized_schema.getNumExtraFieldsUtil();

#define INSTANCE(EXTRA_FIELDS, QUADTREE_BITS)                                                                                                  \
    else if (num_extra_fields==(EXTRA_FIELDS) && quadtree_bits==(QUADTREE_BITS)) {       \
        index = buildIndex<(EXTRA_FIELDS),(QUADTREE_BITS)>(specialized_schema, server, max_points, time_bin_function, report_frequency, regions, attach_set); \
    }
#define MACRO(z,EXTRA_FIELDS,QUADTREE_BITS) INSTANCE(EXTRA_FIELDS,QUADTREE_BITS)
#define QUADTREE_INSTANCES(QUADTREE_BITS) BOOST_PP_REPEAT_FROM_TO(0,6,MACRO,QUADTREE_BITS)

#ifdef FAST_COMPILE
#warning FAST_COMPILE

    if (0) {}
    QUADTREE_INSTANCES(20)
    QUADTREE_INSTANCES(25)

#else

    if (0) {}
    QUADTREE_INSTANCES(0)
    QUADTREE_INSTANCES(5)
    QUADTREE_INSTANCES(10)
    QUADTREE_INSTANCES(15)
    QUADTREE_INSTANCES(17)
    QUADTREE_INSTANCES(20)
    QUADTREE_INSTANCES(25)

#endif // FAST_COMPILE


    gs_hname = getHostname();
    gs_hport = port;
    gs_schema_name = schema.name;
    setupSignalHandlers();

    if (!noserve) {
        // update port
        server.port = port;

        std::stringstream nano_up;
        nano_up << "curl http://" << gs_nanodb << "/online/"
                << gs_hname
                << "/" << port
                << "/" << schema.name
                << "/" << "0.0.1";
        std::string system_call = nano_up.str();
        system(system_call.c_str());

        // create authentication code
        std::string auth_code = random_string();
        std::cout << "Authorization=" << auth_code << std::endl;
        server.passcode(auth_code);

        // start http server
        server.start(mongoose_threads);
    }

    delete g_server;
    delete index;
}
