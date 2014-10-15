#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>

namespace dumpfile {

//------------------------------------------------------------------------------
// DumpFileException
//------------------------------------------------------------------------------

struct DumpFileException: public std::runtime_error {
public:
    DumpFileException(const std::string &message);
};

//-------------------------------------------------------------------------------
// FieldType
//-------------------------------------------------------------------------------

struct FieldType {

    FieldType(std::string name, int num_bytes, int num_tokens);
    bool operator==(const FieldType &other) const;

    std::string name;
    int         num_bytes;  // on binary encoding
    int         num_tokens; // on text encoding
};

//-------------------------------------------------------------------------------
// FieldTypesList
//-------------------------------------------------------------------------------

struct FieldTypesList {

    static const FieldType& getFieldType(std::string name);

private:

    static FieldTypesList &instance();

    FieldTypesList();

    static FieldTypesList* singleton;

    std::vector<FieldType>                      field_types;
    std::unordered_map<std::string, FieldType*> field_types_map;
};

//-----------------------------------------------------------------------------
// Field
//-----------------------------------------------------------------------------

struct Field {

    Field(std::string name, const FieldType &field_type, int offset_inside_record, int first_token_index);

    int getNumBytes() const;

    int getNumTokens() const;

    void addValueName(uint64_t value, std::string valname);

    uint64_t getValueFromValName(std::string valname) const;

    std::string getValNameFromValue(uint64_t value) const;

    void copyValNames(const Field &field);

    std::string name;

    const FieldType &field_type;

    int         offset_inside_record; // if in a binary schema this is the byte offset,
    int         first_token_index;    // first token index

    std::unordered_map<uint64_t, std::string> map_value_to_valname;
    std::unordered_map<std::string, uint64_t> map_valname_to_value;

};

//-----------------------------------------------------------------------------
// DumpFielDescription
//-----------------------------------------------------------------------------

struct DumpFileDescription {

    enum Encoding { binary, text, unknown };

    DumpFileDescription();

    Field* addField(std::string name, const FieldType &field_type);

    Field* getFieldByName(std::string name) const;

    int    getFieldIndex(std::string name) const;

    int    getTimeFieldIndex() const;

    int getNumFieldsByType(const FieldType &field_type) const;

    inline bool isBinary() const { return encoding == binary; }

    inline bool isText() const { return encoding == text; }
    
    void writeRecordTextualDescription(std::ostream &os, char* data, int len);

    std::string name;

    Encoding encoding { unknown };

    int record_size;

    int num_tokens;

    std::vector<Field*> fields;
    std::unordered_map<std::string, Field*> map_name_to_field;
    std::unordered_map<std::string, std::string> metadata;
};


std::istream &operator>>(std::istream &is, DumpFileDescription &schema);

std::ostream &operator<<(std::ostream &os, const FieldType &field_type);
std::ostream &operator<<(std::ostream &os, const Field &dim);
std::ostream &operator<<(std::ostream &os, const DumpFileDescription &schema);

}















//enum FieldType {

//    field_uint8,

//    field_float,

//    field_uint64,

//    field_time,

//    //
//    // nanocube related types
//    //

//    // variable
//    field_nc_var_uint_1,
//    field_nc_var_uint_2,
//    field_nc_var_uint_3,
//    field_nc_var_uint_4,
//    field_nc_var_uint_5,
//    field_nc_var_uint_6,
//    field_nc_var_uint_7,
//    field_nc_var_uint_8,

//    // dimensions

//    // nc_dim_cat_N
//    //
//    //    uint of N bytes
//    //    a different name is used with nc prefix as a signal
//    //    to the nanocube program
//    field_nc_dim_cat_1,
//    field_nc_dim_cat_2,
//    field_nc_dim_cat_3,
//    field_nc_dim_cat_4,
//    field_nc_dim_cat_5,
//    field_nc_dim_cat_6,
//    field_nc_dim_cat_7,
//    field_nc_dim_cat_8,

//    //
//    // time essentially an unsigned int, but the name time is an important
//    // annotation for nanocube program
//    //
//    field_nc_dim_time_1,
//    field_nc_dim_time_2,
//    field_nc_dim_time_3,
//    field_nc_dim_time_4,
//    field_nc_dim_time_5,
//    field_nc_dim_time_6,
//    field_nc_dim_time_7,
//    field_nc_dim_time_8,

//    //
//    // quadtree
//    //    in binary format assumes two 32-bit integers:
//    //    x and y coordinates on a 2^(num_levels) x 2^(num_levels) grid
//    //
//    field_nc_dim_quadtree_0,
//    field_nc_dim_quadtree_1,
//    field_nc_dim_quadtree_2,
//    field_nc_dim_quadtree_3,
//    field_nc_dim_quadtree_4,
//    field_nc_dim_quadtree_5,
//    field_nc_dim_quadtree_6,
//    field_nc_dim_quadtree_7,
//    field_nc_dim_quadtree_8,
//    field_nc_dim_quadtree_9,
//    field_nc_dim_quadtree_10,
//    field_nc_dim_quadtree_11,
//    field_nc_dim_quadtree_12,
//    field_nc_dim_quadtree_13,
//    field_nc_dim_quadtree_14,
//    field_nc_dim_quadtree_15,
//    field_nc_dim_quadtree_16,
//    field_nc_dim_quadtree_17,
//    field_nc_dim_quadtree_18,
//    field_nc_dim_quadtree_19,
//    field_nc_dim_quadtree_20,
//    field_nc_dim_quadtree_21,
//    field_nc_dim_quadtree_22,
//    field_nc_dim_quadtree_23,
//    field_nc_dim_quadtree_24,
//    field_nc_dim_quadtree_25,
//    field_nc_dim_quadtree_26,
//    field_nc_dim_quadtree_27,
//    field_nc_dim_quadtree_28,
//    field_nc_dim_quadtree_29,
//    field_nc_dim_quadtree_30

//};
