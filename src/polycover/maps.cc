#include <cassert>
#include <algorithm>

#include "maps.hh"

namespace polycover {

//------------------------------------------------------------------------------
// MapsException
//------------------------------------------------------------------------------

maps::MapsException::MapsException(const std::string &message):
    std::runtime_error(message)
{}

//----------------------------------------------------------------------------
// Tile
//----------------------------------------------------------------------------

maps::Tile::Tile():
    x(0), y(0), zoom(0)
{}

maps::Tile::Tile(Address address)
{
    this->x.quantity    = address & 0x1fffffffL;           // [0, 28] represent x
    this->y.quantity    = (address >> 29L) & 0x1fffffffL;  // [29,57] represent y
    this->zoom.quantity = (address >> 58L) & 0x3fL;        // [58,63] represent level
}

bool maps::Tile::inBetween(Tile min_tile, Tile max_tile) const
{
    if (min_tile.getZoom() != zoom.quantity || max_tile.getZoom() != zoom.quantity)
        throw MapsException("inBetween defined only for tiles in the same zoom level");

    return min_tile.getX() <= x.quantity && x.quantity <= max_tile.getX() &&
            min_tile.getY() <= y.quantity && y.quantity <= max_tile.getY();
}

maps::SubTileLabel maps::Tile::getSubTileLabel(uint32_t index) const
{
    // if (zoom == 0)
    //    use most significant bit on x and y to get sublabel
    //
    // e.g. if num transitions
    //    == 0 then no subtiles att all
    //    == 1 then subtiles == {0}
    //    == 2 then subtiles == {0,1}
    // x >> (zoom - z - 1)
    //

    auto x = this->x.quantity;
    auto y = this->y.quantity;
    auto num_transitions = this->zoom.quantity;

    if (index >= num_transitions)
        throw MapsException("ooops");

    auto bit_x = (x >> (num_transitions - 1 - index)) & 0x1;
    auto bit_y = (y >> (num_transitions - 1 - index)) & 0x1;

    return (SubTileLabel) (bit_x | (bit_y << 1));

}


auto maps::Tile::relativeTile(const Tile& subtile) const -> Tile {
    // assume subtile is a subtile
    int no_bits = subtile.zoom.quantity - this->zoom.quantity;

    assert(no_bits >= 0);

    // TODO: check subtile

    Tile result;
    result.x.quantity = subtile.x.quantity % (1 << no_bits);
    result.y.quantity = subtile.y.quantity % (1 << no_bits);
    result.zoom.quantity = no_bits;

    return result;
}

maps::Tile::Tile(int x, int y, int z):
    x {x},
    y {y},
    zoom {z}
{}

bool maps::Tile::isValid() const {

    int yy = (1 << zoom.quantity) - 1 - y.quantity;
    return
            0 <= x.quantity &&
            x.quantity < (1 << zoom.quantity) &&
            0 <= yy &&
            yy < (1 << zoom.quantity);
}

auto maps::Tile::raw() const -> Address
{
    Address ll = zoom.quantity;
    Address xx = x.quantity;
    Address yy = y.quantity;
    Address addr = xx | (yy << 29) | (ll << 58);
    return addr;
}

bool maps::Tile::operator<=(const Tile &other) const {
    return (*this == other || *this < other);
}

bool maps::Tile::operator==(const Tile &other) const
{
    return x.quantity == other.x.quantity &&
           y.quantity == other.y.quantity &&
           zoom.quantity == other.zoom.quantity;
}
bool maps::Tile::operator!=(const Tile &other) const
{
    return !(*this == other);
}

bool maps::Tile::operator<(const Tile &other) const {
    return zoom.quantity < other.zoom.quantity ||
            (zoom.quantity == other.zoom.quantity && x.quantity < other.x.quantity) ||
            (zoom.quantity == other.zoom.quantity && x.quantity == other.x.quantity &&
             y.quantity < other.y.quantity);
}

maps::Tile::Tile(const DegreesPoint &p_deg, Zoom zoom):
    zoom(zoom)
{
    MercatorPoint p_mer = p_deg;
    int tiles_by_side = 1 << zoom.quantity;
    this->x = (int) ((1.0f + p_mer.x.quantity)/2.0f * tiles_by_side);
    this->y = (int) ((1.0f + p_mer.y.quantity)/2.0f * tiles_by_side);
    this->zoom = zoom;
}

maps::Tile::Tile(const MercatorPoint &p_mer, Zoom zoom):
    zoom(zoom)
{
    int tiles_by_side = 1 << zoom.quantity;
    this->x = (int) ((1.0f + p_mer.x.quantity)/2.0f * tiles_by_side);
    this->y = (int) ((1.0f + p_mer.y.quantity)/2.0f * tiles_by_side);
    this->zoom = zoom;
}





auto maps::Tile::bounds() const-> MercatorRectangle
{
    int   no_tiles_per_side = 1 << this->zoom.quantity;
    float tile_side_size = 2.0f / no_tiles_per_side;

    float x0 = -1.0f + this->x.quantity * tile_side_size;
    float y0 = -1.0f + this->y.quantity * tile_side_size;

    MercatorPoint min { x0, y0 };
    MercatorPoint max { x0 + tile_side_size, y0 + tile_side_size };

    return MercatorRectangle { min, max };
}

auto maps::Tile::center() const-> MercatorPoint
{
    int   no_tiles_per_side = 1 << this->zoom.quantity;
    float tile_side_size = 2.0f / no_tiles_per_side;

    float x0 = -1.0f + this->x.quantity * tile_side_size;
    float y0 = -1.0f + this->y.quantity * tile_side_size;

    return MercatorPoint { x0 + tile_side_size/2.0, y0 + tile_side_size/2.0 };
}

void maps::Tile::refine(maps::SubTileLabel label)
{
    x.quantity = (x.quantity << 1) | (  label     & 0x1 );
    y.quantity = (y.quantity << 1) | ( (label>>1) & 0x1 );
    zoom.quantity += 1;
}

maps::Tile maps::Tile::refined(maps::SubTileLabel label) const
{
    auto result = *this;
    result.refine(label);
    return result;
}

maps::Tile maps::Tile::parent() const {
    return Tile(x.quantity/2,y.quantity/2,zoom.quantity-1);
}

//----------------------------------------------------------------------------
// util
//----------------------------------------------------------------------------

auto maps::io::operator<<(std::ostream &os, const Tile &tile) -> std::ostream& {
    os << "Tile[x:" << tile.x.quantity << ",y:" << tile.y.quantity << ",z:" << tile.zoom.quantity << "]";
    return os;
}

//----------------------------------------------------------------------------
// bestOpenStreeMapsZoomFor Impl.
//----------------------------------------------------------------------------

auto maps::osmZoomFor(const ScreenRectangle &screen_rect,
                      const MercatorRectangle &camera_rect) -> Zoom
{

    //const Viewport &viewport, const Heatmap &heatmap
//    ScreenRectangle   screen_rect = viewport.getAdjustedRectangle();
//    MercatorRectangle camera_rect   = units_cast<units::world, units::mercator>(viewport.getCameraRectangle());

    // see which power of two matches best the x-dimension
    double camera_width   = camera_rect.width().quantity;
    double viewport_width = screen_rect.width().quantity;

    // z between
    // auto f_osm = [](double width, int z) { return width / (1L << z); }

    // viewport_width is in screen units
    //
    // 2^z is the number of tiles in a row for the world in level z
    //
    // camera_width / 2 is the width fraction of the whole world covered
    // by the camera.
    //
    // camera_width / 2 * 2^z is the number of z-level tiles covered by
    // the camera area.
    //
    // in Open Street Maps each tile texture has 256 pixels per row. We
    // want to get the difference of this number with the veiwport width
    //
    auto f = [viewport_width, camera_width](int z) {
        return 256 * (camera_width/2.0) * (1L << z) - viewport_width; // 256 pixels per tile
    };

    // initialize range

    // const static std::vector<int> z(34); // 0 to 30;
    // std::iota(std::begin(z), std::end(z), 0);

    // find smallest integer "z" in [left,right] that is zero or more
    int left = 0, right = 30;
    int z = 0;
    while (left < right) {
        z = (left + right) / 2;
        auto extra_pixels = f(z);
        if (extra_pixels < 0) {
            left = z + 1;
        }
        else if (extra_pixels > 0) {
            right = z;
        }
        else break;
    }

//    std::cout << "camera_width: " << camera_width   << std::endl;
//    std::cout << "vp width:     " << viewport_width << std::endl;
//    std::cout << "z:            " << z              << std::endl;
//    std::cout << "excess:       " << f(z)           << std::endl;

    if (z > 0) {
        if (abs(f(z-1)) < f(z))
            --z;
    }

    return Zoom(z);

}

auto maps::osmTilesFor(const ScreenRectangle   &screen_rect,
                       const MercatorRectangle &camera_rect) -> std::vector<Tile>
{
    Zoom zoom = osmZoomFor(screen_rect, camera_rect);

    std::vector<MercatorPoint> corners {
        camera_rect.corner(geometry::BOTTOM_LEFT),
        camera_rect.corner(geometry::BOTTOM_RIGHT),
        camera_rect.corner(geometry::TOP_RIGHT),
        camera_rect.corner(geometry::TOP_LEFT) };

    std::vector<Tile> corner_tiles;
    std::for_each(corners.begin(), corners.end(),
                  [&corner_tiles, zoom](maps::MercatorPoint p) { corner_tiles.push_back(maps::Tile(p, zoom)); });

    return corner_tiles;
}

//-----------------------------------------------------------------------------
// TileRangeIterator
//-----------------------------------------------------------------------------

maps::TileRangeIterator::TileRangeIterator(maps::Tile min_tile, maps::Tile max_tile):
    min_tile(min_tile),
    max_tile(max_tile)
{
    assert (min_tile <= max_tile);
    assert (min_tile.getZoom() == max_tile.getZoom());
    min_x = min_tile.getX();
    min_y = min_tile.getY();
    max_x = max_tile.getX();
    max_y = max_tile.getY();
    reset();
}

void maps::TileRangeIterator::reset()
{
    x = min_x - 1;
    y = min_y;
}

bool maps::TileRangeIterator::next()
{
    while (true) {
        x++;
        if (x > max_x) {
            x = min_x - 1;
            y++;
            continue;
        }
        if (y > max_y) {
            return false;
        }
        return true;
    }
}

auto maps::TileRangeIterator::getTile() const -> maps::Tile
{
    int z = min_tile.getZoom();
    return maps::Tile(x, y, z);

}

} // polycover
