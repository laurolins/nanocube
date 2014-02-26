#include "NanoCubeSchema.hh"

namespace nanocube {

//------------------------------------------------------------------------------
// Schema
//------------------------------------------------------------------------------

Schema::Schema(dumpfile::DumpFileDescription &dump_file_description):
    dump_file_description(dump_file_description)
{
    // figure out dimension and variable keys
    for (dumpfile::Field *field: dump_file_description.fields) {
        std::string field_type_name = field->field_type.name;
        if (field_type_name.find("nc_dim_cat_") == 0) {
            auto pos = field_type_name.begin() + field_type_name.rfind('_');
            int num_bytes = std::stoi(std::string(pos+1,field_type_name.end()));
            this->dimension_keys.push_back("c" + std::to_string(num_bytes));
        }
        else if (field_type_name.find("nc_dim_quadtree_") == 0) {
            auto pos = field_type_name.begin() + field_type_name.rfind('_');
            int num_levels = std::stoi(std::string(pos+1,field_type_name.end()));
            this->dimension_keys.push_back("q" + std::to_string(num_levels));
        }
        else if (field_type_name.find("nc_dim_time_") == 0) {
            auto pos = field_type_name.begin() + field_type_name.rfind('_');
            int num_bytes = std::stoi(std::string(pos+1,field_type_name.end()));
            this->variable_keys.push_back("u" + std::to_string(num_bytes));
        }
        else if (field_type_name.find("nc_var_uint_") == 0) {
            auto pos = field_type_name.begin() + field_type_name.rfind('_');
            int num_bytes = std::stoi(std::string(pos+1,field_type_name.end()));
            this->variable_keys.push_back("u" + std::to_string(num_bytes));
        }
    }
}

int Schema::getDimensionIndex(std::string name) const
{
    return dump_file_description.getFieldIndex(name);
}

std::string Schema::getDimensionName(int index) const
{
    return dump_file_description.fields[index]->name;
}


RawAddress Schema::getRawAddress(int dimension_index, const std::string &val_name)
{
    ::dumpfile::Field *field = dump_file_description.fields[dimension_index];
    return static_cast<RawAddress>(field->getValueFromValName(val_name));
}

} // nanocube namespace
