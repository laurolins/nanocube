#include "tile.hh"

#include <cassert>
#include <algorithm>

//------------------------------------------------------------------------------
// Tile Impl.
//------------------------------------------------------------------------------

geom2d::Tile::Tile(uint64_t raw)
{
    x = raw & 0x1fffffffL;           // [0, 28] represent x
    y = (raw >> 29L) & 0x1fffffffL;  // [29,57] represent y
    z = (raw >> 58L) & 0x3fL;        // [58,63] represent level
}

geom2d::Tile::Tile(int x, int y, int z):
    x{x}, y{y}, z{z}
{}

int geom2d::Tile::index() const
{
    return (x % 2) + ((y % 2) << 1);
}

auto geom2d::Tile::corner(Corner corner) const -> Point {
    double side = 2 * 1.0 / (1 << z);
    double xx = -1.0 + side * ( (0x1 & corner ? 1 : 0) + x);
    double yy = -1.0 + side * ( (0x2 & corner ? 1 : 0) + y);
    return Point(xx, yy);
}

geom2d::Point geom2d::Tile::center() const
{
    return (corner(BOTTOM_LEFT) + corner(TOP_RIGHT)) / 2.0;
}

geom2d::Point geom2d::Tile::size() const
{
    return corner(TOP_RIGHT) - corner(BOTTOM_LEFT);
}

geom2d::BoundingBox geom2d::Tile::getBoundingBox() const
{
    BoundingBox bbox;
    bbox.add(corner(BOTTOM_LEFT));
    bbox.add(corner(TOP_RIGHT));
    return bbox;
}

auto geom2d::operator+(const Tile &tile, int index) -> Tile {
    return Tile { 2*tile.x + (index & 0x1 ? 1 : 0),
                  2*tile.y + (index & 0x2 ? 1 : 0),
                  tile.z + 1 };
}

//------------------------------------------------------------------------------
// io
//------------------------------------------------------------------------------

std::ostream& geom2d::io::operator<<(std::ostream &os, const Tile& tile) {
    os << "Tile { " << tile.x << ", " << tile.y << ", " << tile.z << " } ";
    return os;
}
