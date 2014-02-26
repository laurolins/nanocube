#include "base.hh"

#include <cassert>
#include <algorithm>


//------------------------------------------------------------------------------
// Geom2DException Impl.
//------------------------------------------------------------------------------

geom2d::Geom2DException::Geom2DException(const std::string &message):
    std::runtime_error(message)
{}
