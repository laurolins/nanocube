#include <iostream>
#include <random>
#include <chrono>
#include <sstream>
#include <iomanip>

#include <set>
#include <stdio.h>
#include <string.h>

#include <cmath>
#include <cstdlib>
#include <ctime>
#include <cstdio>

#include <unordered_map>
#include <unordered_set>

#include <string>
#include <fstream>
#include <sstream>

#include <boost/tokenizer.hpp>

// some string manipulation
#include <boost/algorithm/string.hpp>

#include <Stopwatch.hh>

#include <functional>

#define xDEBUG_STREE_SERVER

// #include <boost/mpl/next_prior.hpp>
// #include <boost/mpl/for_each.hpp>
// #include <boost/mpl/range_c.hpp>

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
    Field() {}
    Field(std::string name, FieldType type, int index, int record_offset):
        name(name), type(type), index(index), record_offset(record_offset)
    {}

    void addValueName(int value, std::string name) {
        value_to_name[value] = name;
        name_to_value[name]  = value;
    }

    int getValueForName(std::string name) const {
        auto it = name_to_value.find(name);
        if (it == name_to_value.end())
            throw std::string("value not found on Field::getValueName");
        else
            return it->second;
    }

    std::string getNameForValue(int value) const {
        auto it = value_to_name.find(value);
        if (it == value_to_name.end())
            throw std::string("value not found on Field::getValueName");
        else
            return it->second;
    }

    int getFieldSize() const {
        return getFieldTypeSize(type);
    }


    std::string name;
    FieldType   type;
    int         index;

    std::unordered_map<int, std::string> value_to_name;
    std::unordered_map<std::string, int> name_to_value;

    int         record_offset;
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

    void addField(std::string field_name, FieldType field_type) {
        Field *field = new Field(field_name, field_type, (int) this->fields.size(), record_size);
        record_size += getFieldTypeSize(field_type);

        fields.push_back(field);
        name_to_field[field->name] = field;
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
        boost::tokenizer<boost::char_separator<char>> tokens(line, sep);

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
            std::string name(tok.at(3));

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

        // check if required fields are present
        if ((field_latitude  == nullptr || field_latitude->type  != FLOAT) ||
            (field_longitude == nullptr || field_longitude->type != FLOAT) ||
            (field_time      == nullptr || field_time->type      != UINT64))
            throw std::string("Required fields not matched: latitude (float), longitude (float), time (uint64)");

        std::unordered_set<Field*> required_fields { field_latitude, field_longitude, field_time };

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

    int getNumExtraFields() const {
        return extra_fields.size();
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

    //
    std::vector<Field*> extra_fields;

    const Schema& schema;
};




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
// main
//-----------------------------------------------------------------------------

int main(int argc, char** argv)
{
    int max_points = 0;
    // int report_frequency  = 1000000;
    int parallel = 1;

    std::string output_filename("stree_dmp_out.dmp");
    std::string input_filename("-");

    // find type of dump
    try {
        std::string       type_st("");
        for (int i=1;i<argc;i++) {
            std::vector<std::string> tokens;
            split(std::string(argv[i]),'=',tokens);
            if (tokens.size() < 2) {
                continue;
            }
            if (tokens[0].compare("--max") == 0) {
                max_points = std::stoi(tokens[1]);
            }
            if (tokens[0].compare("--parallel") == 0) {
                parallel = std::stoi(tokens[1]);
            }
//            if (tokens[0].compare("--rf") == 0) { // report frequenc
//                report_frequency = std::stoi(tokens[1]);
//            }
            if (tokens[0].compare("--out") == 0) { // report frequenc
                output_filename = tokens[1];
            }
            if (tokens[0].compare("--in") == 0) { // report frequenc
                input_filename = tokens[1];
            }
        }
    }
    catch (...)
    {}

    //
    std::istream *input_stream_ptr = &std::cin;
    std::ifstream input_file_stream;
    if (input_filename.compare("-") != 0) {
        input_file_stream.open(input_filename);
        input_stream_ptr = &input_file_stream;
    }


    std::istream &input_stream = *input_stream_ptr;

    // read schema of the stream of records
    Schema schema;
    input_stream >> schema;

    // dump read schema to screen
    std::cout << schema;

    // try creating a specialized schema (separates the required dimensions and extra dimensions)
    SpecializedSchema specialized_schema(schema);

    // now read the point and write them as a big text file
    int record_size = specialized_schema.schema.record_size;
    char buffer[record_size];

    int count = 0;

    //
    std::string   tmp_original_filename = "/tmp/__stree_dmp_original.txt";
    std::string   tmp_sorted_filename   = "/tmp/__stree_dmp_sorted.txt";
    std::ofstream tmp_original_file(tmp_original_filename);

    stopwatch::Stopwatch     stopwatch;
    stopwatch.start();

    while (input_stream.read(&buffer[0], record_size))
    {
        count++;

        int index = 0;
        bool first = true;
        for (Field *f: schema.fields) {
            if (!first) {
                tmp_original_file << " ";
            }
            first = false;
            if (f->type == FLOAT) {
                tmp_original_file << *((float*) (&buffer[0] + index));
                index+=4;
            }
            else if (f->type == UINT64) {
                tmp_original_file << *((uint64_t*) (&buffer[0] + index));
                index+=8;
            }
            else if (f->type == UINT8) {
                tmp_original_file << (int) *((uint8_t*) (&buffer[0] + index));
                index+=1;
            }
        }
        tmp_original_file << std::endl;

        if (count == max_points) {
            break;
        }
    }

    // sort tmp file on the time column

    // add records to the index
    std::cout << "Added " << count << " records to the index in "  << stopwatch.timeInSeconds() <<  std::endl;

    // time offset
    int time_field_no = schema.getField("time")->index + 1;

    std::stringstream command;
    std::string st_parallel = (parallel > 1) ? std::string("--parallel=")+std::to_string(parallel) : std::string("");
    command << "pv " << tmp_original_filename << "| sort -S 50000000 " << st_parallel << " -n -k " << time_field_no << " -o" << tmp_sorted_filename;

    std::cout << "running command: " << std::endl << std::endl;
    std::cout << command.str() << std::endl << std::endl;

    int status = system(command.str().c_str());

    if (status != 0) {
        std::cout << "sort problem... exiting" << std::endl;
        exit(1);
    }

    // message
    std::cout << "file was sorted in " << tmp_sorted_filename << std::endl;

    // write to output file
    std::ofstream output_file(output_filename);

    // write schema to output files
    output_file << schema;
    output_file << std::endl; // mark of end of header

    // read tmp sorted file
    std::ifstream tmp_sorted_file(tmp_sorted_filename);
    if (tmp_sorted_file.is_open())
    {
        std::string line;
        std::vector<std::string> tokens;

        while ( tmp_sorted_file.good() )
        {
            getline(tmp_sorted_file,line);

            tokens.clear();

            split(line,' ',tokens);

            if (tokens.size() == 0) {
                break;
            }

//             std::cout  << " tokens size = " << tokens.size() << std::endl;
//            for (std::string st: tokens ) {
//                std::cout << st << "  " << std::endl;
//            }

            int i=0;
            int index = 0;
            for (Field *f: schema.fields) {
                if (f->type == FLOAT) {
//                     std::cout << i << "  ->  " << tokens[i] << std::endl;
                    float f = std::stof(tokens[i++]);
                    float *float_buffer = (float*) (&buffer[0] + index);
                    *float_buffer = f;
                    index += 4;
                }
                else if (f->type == UINT64) {
//                     std::cout << i << "  ->  " << tokens[i] << std::endl;
                    uint64_t l = std::stol(tokens[i++]);
                    uint64_t *long_buffer = (uint64_t*) (&buffer[0] + index);
                    *long_buffer = l;
                    index += 8;
                }
                else if (f->type == UINT8) {
//                     std::cout << i << "  ->  " << tokens[i] << std::endl;
                    int aux_int = std::stoi(tokens[i++]);
                    uint8_t *int_buffer = (uint8_t*) (&buffer[0] + index);
                    *int_buffer = (uint8_t) aux_int;
                    index += 1;
                }
                else {
                    throw std::exception();
                }
            }

            // write to output file
            output_file.write(buffer, record_size);
        }


    } // file is open

















}
