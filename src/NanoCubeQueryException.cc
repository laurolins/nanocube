#include "NanoCubeQueryException.hh"

//------------------------------------------------------------------------------
// QueryException Impl.
//------------------------------------------------------------------------------

nanocube::query::QueryException::QueryException(const std::string &message):
    std::runtime_error(message)
{}
