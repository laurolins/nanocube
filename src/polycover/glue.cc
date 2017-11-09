#include "polycover.h"

#include <cmath>

#include "polycover.hh"
#include "tessellation.hh"
#include "cells.hh"
#include "boundary.hh"

using poly_t       = polycover::Polygon;
using tree_t       = polycover::labeled_tree::LabeledTree;
using node_t       = polycover::labeled_tree::Node;
using node_ptr_t   = std::unique_ptr<node_t>;
using cells_t      = polycover::cells::Cells;
using celliter_t   = polycover::cells::CellIterator;


static tree_t
polycover_compute_union_decomposition(const polycover::area::Area& area, polycover::TileCoverEngine& engine, tesselation::BoundaryEngine& boundary_engine)
{
    // simple areas
    auto adj_area = boundary_engine.run2(area, tesselation::WINDING_NONZERO);

    using iter_t = decltype(adj_area.contours.begin());

    std::function<tree_t(iter_t)> extract = [&engine](iter_t it) -> tree_t {
        polycover::area::Contour& contour = *it->get();
        polycover::Polygon poly;
        for (auto &p: contour.points) {
            poly.add({p.x, p.y});
        }
        poly.makeItCW(); // make sure all polygons are CW
        return engine.computeCover({ poly });
    };

    // using the union
    return polycover::labeled_tree::combine(adj_area.contours.begin(), adj_area.contours.end(), extract, polycover::labeled_tree::A_OR_B);
}

// lat,lon pairs
polycover_Shape
polycover_contour_in_degrees_to_shape(float *points, int num_points, int max_level)
{
	polycover::area::Area     area;
	polycover::area::Contour *contour = area.addContour();
	for (int i=0;i<num_points;++i) {
		float lon_degrees = points[2*i+1];
		float lat_degrees = points[2*i];
		float lon_mercator = lon_degrees / 180.0f;
		float lat_mercator = logf(tanf((lat_degrees * M_PI/180.0)/2 + M_PI/4))/M_PI;
		contour->add({lon_mercator,lat_mercator});
	}
	const int texture_level = 9;
	polycover::TileCoverEngine engine(max_level, texture_level);
	tesselation::BoundaryEngine boundary_engine;
	cells_t *result = new cells_t( std::move( polycover_compute_union_decomposition(area,engine,boundary_engine) ) );
	polycover_Shape shape;
	shape.handle = result;
	return shape;
}

void polycover_release_shape(polycover_Shape shape)
{
	cells_t *cells = (cells_t*) shape.handle;
	if (cells != 0) {
		delete cells;
	}
}

//
// if success returns 1, else if not enough memory returns zero.
// the code size input int is initialized regardless if result is zero or 1
// for success there needs to be at least code_size + 1 bytes in the input buffer
// since we want to force a \0 character at the end of the code word
//
int polycover_shape_code(polycover_Shape shape, char *begin, char *end, int *code_size)
{
	// assert(shape.handle);
	cells_t *cells = (cells_t*) shape.handle;
	auto code = cells->code();
	// write
	*code_size = code.size();
	auto buffer_size = end - begin;
	if (buffer_size < code.size() + 1) {
		return 0;
	} else {
		char *it = begin;
		for (auto ch: code) {
			*it = ch;
			++it;
		}
		*it = 0; // null-terminated string
		return 1;
	}
}

polycover_Shape polycover_complement_shape(polycover_Shape shape)
{
	cells_t *cells = (cells_t*) shape.handle;
	// - unary operator on labeled tree...
	cells_t *complement = new cells_t(-cells->tree());
	return (polycover_Shape) { .handle = complement };
}

polycover_Shape polycover_union(polycover_Shape shape1, polycover_Shape shape2)
{
	cells_t *cells1 = (cells_t*) shape1.handle;
	cells_t *cells2 = (cells_t*) shape2.handle;
	cells_t *result = new cells_t(polycover::labeled_tree::combine(cells1->tree(),cells2->tree(),polycover::labeled_tree::A_OR_B));
	return (polycover_Shape) { .handle = result };
}

polycover_Shape polycover_difference(polycover_Shape shape1, polycover_Shape shape2)
{
	cells_t *cells1 = (cells_t*) shape1.handle;
	cells_t *cells2 = (cells_t*) shape2.handle;
	cells_t *result = new cells_t(polycover::labeled_tree::combine(cells1->tree(),cells2->tree(),polycover::labeled_tree::A_MINUS_B));
	return (polycover_Shape) { .handle = result };
}

polycover_Shape polycover_symmetric_difference(polycover_Shape shape1, polycover_Shape shape2)
{
	cells_t *cells1 = (cells_t*) shape1.handle;
	cells_t *cells2 = (cells_t*) shape2.handle;
	cells_t *result = new cells_t(polycover::labeled_tree::combine(cells1->tree(),cells2->tree(),polycover::labeled_tree::A_SYMMDIFF_B));
	return (polycover_Shape) { .handle = result };
}

polycover_Shape polycover_intersection(polycover_Shape shape1, polycover_Shape shape2)
{
	cells_t *cells1 = (cells_t*) shape1.handle;
	cells_t *cells2 = (cells_t*) shape2.handle;
	cells_t *result = new cells_t(polycover::labeled_tree::combine(cells1->tree(),cells2->tree(),polycover::labeled_tree::A_AND_B));
	return (polycover_Shape) { .handle = result };
}

