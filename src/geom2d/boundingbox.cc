#include "boundingbox.hh"

#include "base.hh"

#include <cassert>
#include <algorithm>

//------------------------------------------------------------------------------
// BoundingBox
//------------------------------------------------------------------------------

void geom2d::BoundingBox::add(geom2d::Point p)
{
    if (!empty) {
        min.x = std::min(p.x,min.x);
        min.y = std::min(p.y,min.y);
        max.x = std::max(p.x,max.x);
        max.y = std::max(p.y,max.y);
    }
    else {
        empty = false;
        min = max = p;
    }
}


void geom2d::BoundingBox::merge(const geom2d::BoundingBox &bbox)
{
    if (!empty && !bbox.empty) {
        min.x = std::min(bbox.min.x,min.x);
        min.y = std::min(bbox.min.y,min.y);
        max.x = std::max(bbox.max.x,max.x);
        max.y = std::max(bbox.max.y,max.y);
    }
    else {
        *this = bbox;
    }
}

bool geom2d::BoundingBox::contains(geom2d::Point p) const
{
    return (min.x <= p.x && p.x < max.x && min.y <= p.y && p.y < max.y);
}

bool geom2d::BoundingBox::contains(const BoundingBox &bbox) const {
    if (empty || bbox.empty)
        throw geom2d::Geom2DException("ooops");
    if (min.x <= bbox.min.x && bbox.max.x <= max.x && min.y <= bbox.min.y && bbox.max.y <= max.y) {
        return true;
    }
    else {
        return false;
    }
}

bool geom2d::BoundingBox::contained(const BoundingBox &bbox) const {
    if (empty || bbox.empty)
        throw geom2d::Geom2DException("ooops");
    if (bbox.min.x <= min.x && max.x <= bbox.max.x && bbox.min.y <= min.y && max.y <= bbox.max.y) {
        return true;
    }
    else {
        return false;
    }
}

bool geom2d::BoundingBox::disjoint(const BoundingBox &bbox) const {
    if (empty || bbox.empty)
        throw geom2d::Geom2DException("ooops");
    if (max.x <= bbox.min.x || max.y <= bbox.min.y || bbox.max.x <= min.x || bbox.max.y <= min.y) {
        return true;
    }
    else {
        return false;
    }
}

bool geom2d::BoundingBox::intersects(const BoundingBox &bbox) const {
    return !disjoint(bbox);
}


auto geom2d::BoundingBox::operator[](size_t index) const -> Point {
    if (index == 0)
        return min;
    else if (index == 1)
        return Point(max.x, min.y);
    else if (index == 2)
        return max;
    else if (index == 3)
        return Point(min.x, max.y);
    else
        throw geom2d::Geom2DException("oooops");
}


auto geom2d::BoundingBox::compare(const BoundingBox &bbox) const -> BoundingBoxRelation
{
    if (empty || bbox.empty)
        throw geom2d::Geom2DException("ooops");
    if (disjoint(bbox)) {
        return DISJOINT;
    }
    else if (contains(bbox)) {
        return CONTAINS;
    }
    else if (contained(bbox)) {
        return CONTAINED;
    }
    else {
        return INTERSECTS_BUT_NEITHER_CONTAINED_OR_CONTAINS;
    }
}

void geom2d::BoundingBox::clear()
{
    empty = true;
    min = { 0, 0 };
    max = { 0, 0 };
}

//------------------------------------------------------------------------------
// io
//------------------------------------------------------------------------------

std::ostream& geom2d::io::operator<<(std::ostream &os, const BoundingBox& bbox) {
    os << "{ " << bbox.min << ", " << bbox.max << " } ";
    return os;
}

