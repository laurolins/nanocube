#pragma once

#include <vector>
#include <memory>
#include <iostream>

#include <stdexcept>

namespace geom2d {

//------------------------------------------------------------------------------
// Geom2DException
//------------------------------------------------------------------------------

struct Geom2DException: public std::runtime_error {
public:
    Geom2DException(const std::string &message);
};

} // geom2d
