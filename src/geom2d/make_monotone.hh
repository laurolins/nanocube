#pragma once

#include <vector>
#include <memory>
#include <iostream>

#include <stdexcept>

#include "point.hh"
#include "polygon.hh"

namespace geom2d {

//------------------------------------------------------------------------------
// makeMonotone
//------------------------------------------------------------------------------

// Partition a simple polygon into y-monotone sub polygons (with
// the same set of vertices).

auto makeMonotone(const Polygon &poly) -> std::vector<Polygon>;
auto convexPartition(const Polygon &poly) -> std::vector<Polygon>;
auto triangulateMonotonePolygon(const Polygon &poly) -> std::vector<Polygon>;

void test_make_monotone();

} // geom2d
