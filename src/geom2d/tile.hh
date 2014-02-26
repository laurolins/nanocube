#pragma once

#include <vector>
#include <memory>
#include <iostream>

#include <stdexcept>

#include "point.hh"
#include "boundingbox.hh"

namespace geom2d {

//------------------------------------------------------------------------------
// Corner
//------------------------------------------------------------------------------

enum Corner { BOTTOM_LEFT=0, BOTTOM_RIGHT=1, TOP_RIGHT=3, TOP_LEFT=2 };

//------------------------------------------------------------------------------
// Tile
//------------------------------------------------------------------------------

struct Tile {
    Tile() = default;
    Tile(int x, int y, int z);
    Tile(uint64_t raw);

    auto index() const -> int;

    auto corner(Corner corner) const -> Point ;

    auto center() const -> Point;
    auto size()   const -> Point;

    auto getBoundingBox() const -> BoundingBox;
    int x { 0 };
    int y { 0 };
    int z { 0 };
};

auto operator+(const Tile &tile, int index) -> Tile;

} // geom2d

//-----------------------------------------------------------------------------
// IO
//-----------------------------------------------------------------------------

namespace geom2d {

namespace io {

std::ostream& operator<<(std::ostream &os, const Tile& tile);

} // io

} // geom2d

