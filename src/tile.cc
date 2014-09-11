#include "tile.hh"

#include <stdexcept>

namespace nanocube {


//------------------------------------------------------------------------------
// Tile
//------------------------------------------------------------------------------

//
// 2d tile representation from and to path
//

Tile::Tile(DimAddress &addr) {
    x = 0;
    y = 0;
    level = 0;
    for (auto l: addr) {
        advance((int) l);
    }
}

Tile::Tile(int x, int y, int level):
    x(x), y(y), level(level)
{}
                
void Tile::advance(int label) {
    if (label < 0 || label > 3)
        throw std::runtime_error("ooooops!!");
                    
    x *= 2;
    y *= 2;
    level += 1;
    if (label & 0x1)
        x += 1;
    if (label & 0x2)
        y += 1;
}
    
DimAddress Tile::toAddress() const {
    DimAddress result;
    int mask = 1 << (level - 1);
    for (int i=level-1;i>=0;i--) {
        int xbit = (mask & x) ? 1 : 0;
        int ybit = (mask & y) ? 1 : 0;
        result.push_back(xbit + (ybit << 1));
        mask >>= 1;
    }
    return result;
}


//------------------------------------------------------------------------------
// TileRange
//------------------------------------------------------------------------------

TileRange::TileRange(Tile a, Tile b):
    a(a), b(b)
{
    int level = std::max(a.level, b.level);
    for (int i=0;i<level-a.level;i++)
        a.advance(0);
    for (int i=0;i<level-b.level;i++)
        b.advance(3);
}

TileRange::TileRange(Tile tile, int depth)
{
    a = tile;
    b = tile;
    for (int i=0;i<depth - tile.level;++i) {
        a.advance(0);
        b.advance(3);
    }
}
                
TileRange::Relation TileRange::relation(const TileRange& other) {
    if (level() != other.level()) {
        throw std::runtime_error("ooops");
    }
                    
    if (b.x < other.a.x || a.x > other.b.x ||
        b.y < other.a.y || a.y > other.b.y ) {
        return DISJOINT;
    }
    else if (a.x <= other.a.x && other.b.x <= b.x &&
             a.y <= other.a.y && other.b.y <= b.y) {
        return CONTAINS;
    }
    else return INTERSECTS;
}

//------------------------------------------------------------------------------
// Tile1d
//------------------------------------------------------------------------------

Tile1d::Tile1d(DimAddress &addr) {
    x = 0;
    level = 0;
    for (auto l: addr) {
        advance((int) l);
    }
}

Tile1d::Tile1d(int x, int level):
    x(x), level(level)
{}

void Tile1d::advance(int label) {
    if (label < 0 || label > 1)
        throw std::runtime_error("ooooops!!");
                    
    x *= 2;
    level += 1;
    if (label & 0x1)
        x += 1;
}

DimAddress Tile1d::toAddress() const {
    DimAddress result;
    int mask = 1 << (level - 1);
    for (int i=level-1;i>=0;i--) {
        int xbit = (mask & x) ? 1 : 0;
        result.push_back(xbit);
        mask >>= 1;
    }
    return result;
}


//------------------------------------------------------------------------------
// TileRange
//------------------------------------------------------------------------------

TileRange1d::TileRange1d(Tile1d a, Tile1d b):
    a(a), b(b)
{
    int level = std::max(a.level, b.level);
    for (int i=0;i<level-a.level;i++)
        a.advance(0);
    for (int i=0;i<level-b.level;i++)
        b.advance(1);
}

TileRange1d::TileRange1d(Tile1d tile, int depth)
{
    a = tile;
    b = tile;
    for (int i=0;i<depth - tile.level;++i) {
        a.advance(0);
        b.advance(1);
    }
}
                
TileRange1d::Relation TileRange1d::relation(const TileRange1d& other) {
    if (level() != other.level()) {
        throw std::runtime_error("ooops");
    }
                    
    if (b.x < other.a.x || a.x > other.b.x) {
        return DISJOINT;
    }
    else if (a.x <= other.a.x && other.b.x <= b.x) {
        return CONTAINS;
    }
    else return INTERSECTS;
}


                
}
