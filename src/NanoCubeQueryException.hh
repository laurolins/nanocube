#pragma once

#include <string>
#include <stdexcept>

namespace nanocube {

namespace query {

//------------------------------------------------------------------------------
// QueryException
//------------------------------------------------------------------------------

struct QueryException: public std::runtime_error {
public:
    QueryException(const std::string &message);
};

} // query

} // nanocube
