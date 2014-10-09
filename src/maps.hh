#pragma once

#include <string>
#include <vector>
#include <stdexcept>
#include "geometry.hh"

namespace maps {
    

using Address = uint64_t; // raw version of a x,y,z address

using MercatorPoint     = geometry::Point<geometry::units::mercator>;
using DegreesPoint      = geometry::Point<geometry::units::degrees>;

using MercatorRectangle = geometry::Rectangle<geometry::units::mercator>;
using ScreenRectangle   = geometry::IntegralRectangle<geometry::units::screen>;

using Zoom              = geometry::IntegralLength<geometry::units::zoom>;
using TileCoord         = geometry::IntegralLength<geometry::units::tile>;

//------------------------------------------------------------------------------
// MapsException
//------------------------------------------------------------------------------

struct MapsException: public std::runtime_error {
public:
    MapsException(const std::string &message);
};

//----------------------------------------------------------------------------
// Tile
//----------------------------------------------------------------------------

struct Tile {

    Tile();
    Tile(Address address);
    Tile(int x, int y, int z);
    Tile(const MercatorPoint &point, Zoom zoom);
    Tile(const DegreesPoint &point, Zoom zoom);

    bool operator==(const Tile &other) const;

    bool operator<(const Tile &other) const;

    bool operator<=(const Tile &other) const;

    bool isValid() const;

    auto relativeTile(const Tile &subtile) const -> Tile;

    auto raw() const -> Address;

    auto bounds() const -> MercatorRectangle;

    auto center() const -> MercatorPoint;

    inline int getX() const { return x.quantity; }

    inline int getY() const { return y.quantity; }

    inline int getZoom() const { return zoom.quantity; }

    bool inBetween(Tile min_tile, Tile max_tile) const;

public: // data members

    TileCoord x;
    TileCoord y;
    Zoom      zoom;

};

//----------------------------------------------------------------------------
// TileRangeIterator
//----------------------------------------------------------------------------

struct TileRangeIterator {
public:
    TileRangeIterator(Tile min_tile, Tile max_tile);
    void reset();
    bool next();
    auto getTile() const -> maps::Tile;
public:
    Tile min_tile;
    Tile max_tile;
    int x, min_x, max_x;
    int y, min_y, max_y;
};


//----------------------------------------------------------------------------
// bestOpenStreeMapsZoomFor
//----------------------------------------------------------------------------

auto osmZoomFor(const ScreenRectangle &screen_rect, const MercatorRectangle &camera_rect) -> Zoom;

auto osmTilesFor(const ScreenRectangle &screen_rect, const MercatorRectangle &camera_rect) -> std::vector<Tile>;

namespace io {
std::ostream &operator<<(std::ostream &os, const Tile &tile);
} // util namespace


} // maps namespace

