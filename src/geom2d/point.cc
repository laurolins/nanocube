#include <geom2d/point.hh>

#include <cassert>
#include <algorithm>

//------------------------------------------------------------------------------
// io
//------------------------------------------------------------------------------

std::ostream& geom2d::io::operator<<(std::ostream &os, const Point& point) {
    os << "{ " << point.x << ", " << point.y << " } ";
    return os;
}
