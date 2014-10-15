#include "DumpFile.hh"

#include <iostream>
#include <sstream>
#include <fstream>
#include <exception>

// some help from boost help
#include <boost/tokenizer.hpp>
#include <boost/algorithm/string.hpp>

// DumpFile description

namespace dumpfile {

//------------------------------------------------------------------------------
// DumpFileException Impl.
//------------------------------------------------------------------------------

DumpFileException::DumpFileException(const std::string &message):
    std::runtime_error(message)
{}

//------------------------------------------------------------------------------
// FieldType
//------------------------------------------------------------------------------

FieldType::FieldType(std::string name, int num_bytes, int num_tokens):
    name(name),
    num_bytes(num_bytes),
    num_tokens(num_tokens)
{}

bool FieldType::operator==(const FieldType &other) const {
    return other.name.compare(name) == 0;
}

//-------------------------------------------------------------------------------
// FieldTypesList
//-------------------------------------------------------------------------------

FieldTypesList *FieldTypesList::singleton = nullptr;

FieldTypesList::FieldTypesList():
    field_types({
        { "uint8",  1, 1 },
        { "uint16", 2, 1 },
        { "uint24", 3, 1 },
        { "uint32", 4, 1 },
        { "uint40", 5, 1 },
        { "uint48", 6, 1 },
        { "uint56", 7, 1 },
        { "uint64", 8, 1 },

        { "time",   8, 1 },

        { "float",  4, 1 },
        { "double", 8, 1 },

        // nanocube related fields

        // var: variable
        { "nc_var_uint_1", 1, 1 },
        { "nc_var_uint_2", 2, 1 },
        { "nc_var_uint_3", 3, 1 },
        { "nc_var_uint_4", 4, 1 },
        { "nc_var_uint_5", 5, 1 },
        { "nc_var_uint_6", 6, 1 },
        { "nc_var_uint_7", 7, 1 },
        { "nc_var_uint_8", 8, 1 },

        // nc: categorical dimensions
        { "nc_dim_cat_1", 1, 1 },
        { "nc_dim_cat_2", 2, 1 },
        { "nc_dim_cat_3", 3, 1 },
        { "nc_dim_cat_4", 4, 1 },
        { "nc_dim_cat_5", 5, 1 },
        { "nc_dim_cat_6", 6, 1 },
        { "nc_dim_cat_7", 7, 1 },
        { "nc_dim_cat_8", 8, 1 },

        // nc: time dimension (uint but with the name maps to the nanocube semantic)
        { "nc_dim_time_1", 1, 1 },
        { "nc_dim_time_2", 2, 1 },
        { "nc_dim_time_3", 3, 1 },
        { "nc_dim_time_4", 4, 1 },
        { "nc_dim_time_5", 5, 1 },
        { "nc_dim_time_6", 6, 1 },
        { "nc_dim_time_7", 7, 1 },
        { "nc_dim_time_8", 8, 1 },

        // nc: quadtree dimension (uint but with the name maps to the nanocube semantic)
        { "nc_dim_quadtree_0",  8, 2 },
        { "nc_dim_quadtree_1",  8, 2 },
        { "nc_dim_quadtree_2",  8, 2 },
        { "nc_dim_quadtree_3",  8, 2 },
        { "nc_dim_quadtree_4",  8, 2 },
        { "nc_dim_quadtree_5",  8, 2 },
        { "nc_dim_quadtree_6",  8, 2 },
        { "nc_dim_quadtree_7",  8, 2 },
        { "nc_dim_quadtree_8",  8, 2 },
        { "nc_dim_quadtree_9",  8, 2 },
        { "nc_dim_quadtree_10", 8, 2 },
        { "nc_dim_quadtree_11", 8, 2 },
        { "nc_dim_quadtree_12", 8, 2 },
        { "nc_dim_quadtree_13", 8, 2 },
        { "nc_dim_quadtree_14", 8, 2 },
        { "nc_dim_quadtree_15", 8, 2 },
        { "nc_dim_quadtree_16", 8, 2 },
        { "nc_dim_quadtree_17", 8, 2 },
        { "nc_dim_quadtree_18", 8, 2 },
        { "nc_dim_quadtree_19", 8, 2 },
        { "nc_dim_quadtree_20", 8, 2 },
        { "nc_dim_quadtree_21", 8, 2 },
        { "nc_dim_quadtree_22", 8, 2 },
        { "nc_dim_quadtree_23", 8, 2 },
        { "nc_dim_quadtree_24", 8, 2 },
        { "nc_dim_quadtree_25", 8, 2 },
        { "nc_dim_quadtree_26", 8, 2 },
        { "nc_dim_quadtree_27", 8, 2 },
        { "nc_dim_quadtree_28", 8, 2 },
        { "nc_dim_quadtree_29", 8, 2 },
        { "nc_dim_quadtree_30", 8, 2 },
        
        // nc: quadtree dimension (uint but with the name maps to the nanocube semantic)
        { "nc_dim_bintree_0",  1, 1 },
        { "nc_dim_bintree_1",  1, 1 },
        { "nc_dim_bintree_2",  1, 1 },
        { "nc_dim_bintree_3",  1, 1 },
        { "nc_dim_bintree_4",  1, 1 },
        { "nc_dim_bintree_5",  1, 1 },
        { "nc_dim_bintree_6",  1, 1 },
        { "nc_dim_bintree_7",  1, 1 },
        { "nc_dim_bintree_8",  1, 1 },
        { "nc_dim_bintree_9",  2, 1 },
        { "nc_dim_bintree_10", 2, 1 },
        { "nc_dim_bintree_11", 2, 1 },
        { "nc_dim_bintree_12", 2, 1 },
        { "nc_dim_bintree_13", 2, 1 },
        { "nc_dim_bintree_14", 2, 1 },
        { "nc_dim_bintree_15", 2, 1 },
        { "nc_dim_bintree_16", 2, 1 },
        { "nc_dim_bintree_17", 3, 1 },
        { "nc_dim_bintree_18", 3, 1 },
        { "nc_dim_bintree_19", 3, 1 },
        { "nc_dim_bintree_20", 3, 1 },
        { "nc_dim_bintree_21", 3, 1 },
        { "nc_dim_bintree_22", 3, 1 },
        { "nc_dim_bintree_23", 3, 1 },
        { "nc_dim_bintree_24", 3, 1 },
        { "nc_dim_bintree_25", 4, 1 },
        { "nc_dim_bintree_26", 4, 1 },
        { "nc_dim_bintree_27", 4, 1 },
        { "nc_dim_bintree_28", 4, 1 },
        { "nc_dim_bintree_29", 4, 1 },
        { "nc_dim_bintree_30", 4, 1 },
        { "nc_dim_bintree_31", 4, 1 },
        { "nc_dim_bintree_32", 4, 1 }
        })
{
    for (FieldType &ft: field_types) {
        // std::cout << ft << " addr: " << (void*) &ft << std::endl;
        field_types_map[ft.name] = &ft;
    }
}

const FieldType &FieldTypesList::getFieldType(std::string name)
{
    FieldTypesList &field_types_list = FieldTypesList::instance();
    auto it = field_types_list.field_types_map.find(name);
    if (it == field_types_list.field_types_map.end()) {
        std::cerr << "(ERROR) Couldn't find type names: " << name << std::endl;
        throw DumpFileException("(ERROR) Couldn't find type names: " + name);
    }

//    std::cout << "FieldTypesList::getFieldType(" << name << ") = "
//              << (void*) it->second << std::endl;

    return *(it->second);
}

FieldTypesList &FieldTypesList::instance()
{
    if (singleton == nullptr) {
        singleton = new FieldTypesList();
    }
    return *singleton;
}


//------------------------------------------------------------------------------
// Field
//------------------------------------------------------------------------------

Field::Field(std::string name, const FieldType &field_type, int offset_inside_record, int first_token_index):
    name(name),
    field_type(field_type),
    offset_inside_record(offset_inside_record),
    first_token_index(first_token_index)
{}

int Field::getNumBytes() const {
    return field_type.num_bytes;
}

int Field::getNumTokens() const {
    return field_type.num_tokens;
}

void Field::addValueName(uint64_t value, std::string valname) {
    if (map_value_to_valname.find(value) != map_value_to_valname.end()) {
        std::string old_name = map_value_to_valname[value];
        map_valname_to_value.erase(old_name);
    }
    map_value_to_valname[value]   = valname;
    map_valname_to_value[valname] = value;
}

uint64_t Field::getValueFromValName(std::string valname) const {
    auto it = map_valname_to_value.find(valname);
    if (it == map_valname_to_value.end()) {
        throw std::exception();
    }
    return it->second;
}

std::string Field::getValNameFromValue(uint64_t value) const {
    auto it = map_value_to_valname.find(value);
    if (it == map_value_to_valname.end()) {
        throw std::exception();
    }
    return it->second;
}

void Field::copyValNames(const Field &field) {
    map_valname_to_value = field.map_valname_to_value;
    map_value_to_valname = field.map_value_to_valname;
}


//------------------------------------------------------------------------------
// DumpFileDescription
//------------------------------------------------------------------------------

DumpFileDescription::DumpFileDescription():
    record_size(0),
    num_tokens(0)
{}

Field* DumpFileDescription::addField(std::string name, const FieldType &field_type) {
    auto it = map_name_to_field.find(name);
    if (it != map_name_to_field.end()) {
        std::cerr << "(ERROR) Field name already exists: " << name << std::endl;
        throw std::exception();
    }
    Field *field = new Field(name, field_type, record_size, num_tokens);
    fields.push_back(field);
    map_name_to_field[name] = field;

    record_size += field->getNumBytes();
    num_tokens  += field->getNumTokens();

    return field;
}

Field* DumpFileDescription::getFieldByName(std::string name) const {
    auto it = map_name_to_field.find(name);
    if (it != map_name_to_field.end()) {
        return it->second;
    }
    throw std::exception();
}

int DumpFileDescription::getFieldIndex(std::string name) const
{
    int i =0;
    for (Field *field: fields) {
        if (field->name.compare(name) == 0) {
            return i;
        }
        else {
            i++;
        }
    }
    throw DumpFileException("Dimension Not Found");
}

int DumpFileDescription::getTimeFieldIndex() const
{
    int i =0;
    static const std::string prefix = "nc_dim_time";
    for (Field *field: fields) {
        if (field->field_type.name.compare(0, prefix.length(), prefix) == 0) {
            return i;
        }
        else {
            i++;
        }
    }
    throw std::exception();
}

int DumpFileDescription::getNumFieldsByType(const FieldType &field_type) const {
    int count = 0;
    for (Field *f: fields) {
        if (f->field_type == field_type) {
            count++;
        }
    }
    return count;
}
    
    
    
    
void DumpFileDescription::writeRecordTextualDescription(std::ostream &os, char* data, int len) {
    // assuming a binary record!
    if (len < record_size) {
        throw DumpFileException("Not enough bytes for a record");
    }
    
    os << "{";
    std::size_t offset = 0;
    std::size_t value;
    bool first = true;
    for (auto &f: this->fields) {
        auto num_bytes = f->getNumBytes();
        value = 0;
        std::copy(data + f->offset_inside_record,
                  data + f->offset_inside_record + num_bytes,
                  (char*) &value);
        if (!first)
            os << " ";
        os << f->name << ":" << num_bytes << ":" << value;
        first = false;
        offset += num_bytes;
    }
    os << "}";
}

std::istream &operator>>(std::istream &is, DumpFileDescription &dump_file) {

    char buffer[1000];

    bool header_ok = true;
    
    // read line
    while (1)
    {
        is.getline(buffer, 1000);
        
        if (!is) {
            header_ok = false;
            break;
        }
        
        std::string line(buffer);

        // std::cout << line << std::endl;

        if (!line.size()) {
            break; // clean exit
        }

        boost::char_separator<char> sep(", ");
        boost::tokenizer<boost::char_separator<char>> tokens(line, sep);

        std::vector<std::string> tok;
        for ( auto it = tokens.begin(); it != tokens.end(); ++it)
        {
            std::string st(*it);
            if (st.size() > 0)
                tok.push_back(st);
        }

        if (tok.at(0).compare("name:") == 0) {
            dump_file.name = tok.at(1);
        }

        else if (tok.at(0).compare("encoding:") == 0) {
            std::string encoding_st(tok.at(1));
            if (encoding_st.compare("binary") == 0) {
                dump_file.encoding = DumpFileDescription::binary;
            }
            else if (encoding_st.compare("text") == 0) {
                dump_file.encoding = DumpFileDescription::text;
            }
            else {
                throw std::exception();
            }
        }

        else if (tok.at(0).compare("metadata:") == 0) {
            std::string key_st(tok.at(1));
            std::string value_st(tok.at(2));
            dump_file.metadata[key_st] = value_st;
        }

        else if (tok.at(0).compare("field:") == 0) {
            std::string field_name(tok.at(1));
            std::string field_type_st(tok.at(2));
            try {
                const FieldType &field_type = FieldTypesList::getFieldType(field_type_st);
                dump_file.addField(field_name, field_type);
            }
            catch (std::exception &e) {
                throw std::exception();
            }
        }
        else if (tok.at(0).compare("valname:") == 0) {
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

            try {
                Field *field = dump_file.getFieldByName(field_name);
                field->addValueName(value, name);
            }
            catch (std::exception &e) {
                std::cerr << "<ERROR> Field not found when setting valname: " << field_name << std::endl;
                throw e;
            }
        }
    }

    if (!header_ok) {
        throw DumpFileException("Invalid header specification");
    }

    return is;

}

std::ostream &operator<<(std::ostream &os, const FieldType &field_type) {
    os << field_type.name;
    return os;
}

std::ostream &operator<<(std::ostream &os, const Field &field)
{
    os << "field: " << field.name << " " << field.field_type << std::endl;

    // sort values
    for (auto it: field.map_value_to_valname) {
        uint64_t value = it.first;
        std::string valname = it.second;
        os << "valname: " << field.name << " " << value << " " << valname << std::endl;
    }
    return os;
}

std::ostream &operator<<(std::ostream &os, const DumpFileDescription &dump_file)
{
    os << "name: " << dump_file.name << std::endl;
    os << "encoding: " << (dump_file.encoding == DumpFileDescription::binary ? "binary" : "text")  << std::endl;

    // metadata
    for (auto it: dump_file.metadata) {
        os << "metadata: " << it.first << " " << it.second << std::endl;
    }

    // fields
    for (Field *f: dump_file.fields) {
        os << *f;
    }

    return os;
}

}
