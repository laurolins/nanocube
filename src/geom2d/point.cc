#include "point.hh"

#include <cassert>
#include <algorithm>
#include <iomanip>

//------------------------------------------------------------------------------
// io
//------------------------------------------------------------------------------

std::ostream& geom2d::io::operator<<(std::ostream &os, const Point& point) {
    os << "{ " << std::setprecision(18) << point.x << ", " << std::setprecision(18) << point.y << " } ";
    return os;
}
