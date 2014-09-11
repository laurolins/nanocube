#pragma once

#include "address.hh"

#include <vector>
#include <cmath>
#include <stdexcept>

namespace nanocube {
    
//------------------------------------------------------------------------------
// Tile
//------------------------------------------------------------------------------

struct Tile {
public:
    Tile() = default;
    Tile(DimAddress &addr);
    Tile(int x, int y, int level);
    void advance(int label) ;
    DimAddress toAddress() const;
public:
    int x { 0 };
    int y { 0 };
    int level { 0 };
};
            
//------------------------------------------------------------------------------
// TileRange
//------------------------------------------------------------------------------

struct TileRange {
public:
    enum Relation { DISJOINT, CONTAINS, INTERSECTS };
public:
    TileRange(Tile a, Tile b);
    TileRange(Tile tile, int depth);
    inline int level() const {
        return a.level;
    }
    Relation relation(const TileRange& other);

public:
    Tile a;
    Tile b;
};

//------------------------------------------------------------------------------
// Tile
//------------------------------------------------------------------------------

struct Tile1d {
public:
    Tile1d() = default;
    Tile1d(DimAddress &addr);
    Tile1d(int x, int level);
    void advance(int label) ;
    DimAddress toAddress() const;
public:
    int x { 0 };
    int level { 0 };
};

//------------------------------------------------------------------------------
// TileRange1d
//------------------------------------------------------------------------------

struct TileRange1d {
public:
    enum Relation { DISJOINT, CONTAINS, INTERSECTS };
public:
    TileRange1d(Tile1d a, Tile1d b);
    TileRange1d(Tile1d tile, int depth);
    inline int level() const {
        return a.level;
    }
    Relation relation(const TileRange1d& other);

public:
    Tile1d a;
    Tile1d b;
};



}


