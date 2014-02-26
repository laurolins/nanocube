#pragma once

#include "Query.hh"

#include "DumpFile.hh"

using ::query::RawAddress;

namespace nanocube {

//------------------------------------------------------------------------------
// Schema
//------------------------------------------------------------------------------

struct Schema {

public: //

    Schema(::dumpfile::DumpFileDescription &dump_file_description);

public: // Methods

    std::string getDimensionName(int index) const;

    int getDimensionIndex(std::string name) const;

    RawAddress getRawAddress(int dimension_index, const std::string &val_name);

public: //

    ::dumpfile::DumpFileDescription &dump_file_description;

    std::vector<std::string> dimension_keys;

    std::vector<std::string> variable_keys;

};

} // nanocube namespace
