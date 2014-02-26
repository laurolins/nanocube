#pragma once

#include <iostream>
#include <cmath>

#include "point.hh"

namespace geom2d {

//------------------------------------------------------------------------------
// BoundingBox
//------------------------------------------------------------------------------

enum BoundingBoxRelation { DISJOINT, CONTAINED, CONTAINS, INTERSECTS_BUT_NEITHER_CONTAINED_OR_CONTAINS };

//------------------------------------------------------------------------------
// BoundingBox
//------------------------------------------------------------------------------

struct BoundingBox {

    BoundingBox() = default;

    void add(Point p);
    void merge(const BoundingBox& bbox);

    bool contains(Point p) const;

    bool disjoint(const BoundingBox &bbox) const;
    bool contains(const BoundingBox &bbox) const;
    bool contained(const BoundingBox &bbox) const;
    bool intersects(const BoundingBox &bbox) const;

    auto compare(const BoundingBox& bbox) const -> BoundingBoxRelation;

    void clear();

    auto operator[](size_t index) const -> Point;

    bool empty { true };
    Point min;
    Point max;

};

}

//-----------------------------------------------------------------------------
// IO
//-----------------------------------------------------------------------------

namespace geom2d {

namespace io {

std::ostream& operator<<(std::ostream &os, const BoundingBox& bbox);

} // io

} // geom2d


