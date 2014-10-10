#include "polycover.hh"

#include "maps.hh"
#include "labeled_tree.hh"
#include "mipmap.hh"
#include "area.hh"

#include <algorithm>
#include <deque>
#include <unordered_map>
#include <unordered_set>

namespace polycover {

//------------------------------------------------------------------------------
// Segment
//------------------------------------------------------------------------------

Segment::Segment(RealPoint p1, RealPoint p2):
    p1(p1), p2(p2)
{}

//------------------------------------------------------------------------------
// BoundaryType
//------------------------------------------------------------------------------

BoundaryType boundary(const Segment &s) {
    auto p = perpendicular(s.p2 - s.p1);
    if (p.x > 0) {
        return RIGHT;
    }
    else if (p.x < 0) {
        return LEFT;
    }
    else {
        return HORIZONTAL; // s.v1.point.x < s.v2.point.x ? LEFT : RIGHT;
    }
};

//------------------------------------------------------------------------------
// Chain
//------------------------------------------------------------------------------

Chain::Chain(int index, BoundaryType type):
    index(index),
    type(type),
    length(1)
{}

void Chain::grow(int n) {
    length += n;
}

std::ostream& operator<<(std::ostream& os, const BoundaryType& btype) {
    os << ((btype == LEFT) ? "\"LEFT_BOUNDARY\"" : "\"RIGHT_BOUNDARY\"");
    return os;
}

std::ostream& operator<<(std::ostream& os, const Chain& chain) {
    os << "{\"type\":\"chain\", \"index\":" << chain.index << ", \"length\":"<< chain.length << ", \"type\":" << chain.type << "}";
    return os;
}

//------------------------------------------------------------------------------
// ChainBoundary::Iterator Impl.
//------------------------------------------------------------------------------

ChainBoundary::Iterator::Iterator(const ChainBoundary& boundary):
boundary(boundary)
{}

bool ChainBoundary::Iterator::next() {
    if (boundary.min_row == -1)
        return false;
    if (current_y == -1) {
        current_y = boundary.min_row;
        while (current_y < boundary.rows.size() && boundary.rows[current_y] == UNDEFINED)
            ++current_y;
        if (current_y >= boundary.rows.size())
            return false;
        else
            return true;
    }
    else {
        ++current_y;
        while (current_y < boundary.rows.size() && boundary.rows[current_y] == UNDEFINED)
            ++current_y;
        if (current_y >= boundary.rows.size())
            return false;
        else
            return true;
    }
}

void ChainBoundary::Iterator::get(int &x, int &y) {
    y = current_y;
    x = boundary.rows[y];
}

//------------------------------------------------------------------------------
// ChainBoundary Impl.
//------------------------------------------------------------------------------

void ChainBoundary::reset(Mode mode) {
    this->mode = mode;
    min_row = -1;
    rows.clear();
}

void ChainBoundary::insert(int x, int y) {
    
    if (x < 0 || y < 0)
        throw std::runtime_error("ooops");
    
    auto old_rows_size = rows.size();
    if (y >= old_rows_size) {
        rows.resize(y+1);
        std::for_each(rows.begin() + old_rows_size, rows.end(), [](int &v) { v = UNDEFINED; });
        rows[y] = x;
    }
    else {
        if (rows[y] == UNDEFINED)
            rows[y] = x;
        else
            rows[y] = (mode == MIN) ? std::min(rows[y],x) : std::max(rows[y],x);
    }
    
    if (min_row == -1) {
        min_row = y;
    } else {
        min_row = std::min(min_row, y);
    }
    
}

auto ChainBoundary::iterator() -> Iterator {
    return Iterator(*this);
}


//------------------------------------------------------------------------------
// Polygon
//------------------------------------------------------------------------------

Polygon::Polygon(const std::vector<RealPoint> &points)
{
    for (auto p: points) {
        this->points.push_back(p);
    }
}
    
void Polygon::makeItCW() {
    double area = 0.0;
    auto prev = points.back();
    for (auto &current: points) {
        area += (current.x - prev.x) * (current.y + prev.y); //signed  area of a bar from 0 to the average point between heights
        prev = current;
    }
    if (area < 0) {
        std::reverse(points.begin(), points.end());
    }
}

double Polygon::area() {
    double area_twice = 0.0;
    auto prev = points.back();
    for (auto &current: points) {
        area_twice += (current.x - prev.x) * (current.y + prev.y); //signed  area of a bar from 0 to the average point between heights
        prev = current;
    }
    return area_twice / 2.0;
}

Polygon::Polygon(const Polygon &other, std::function<RealPoint(const RealPoint&)> transform) {
    for (auto p: other.points) {
        this->points.push_back(transform(p));
    }
}

void Polygon::add(RealPoint p) {
    points.push_back(p);
}


int modulo (int m, int n) { return m >= 0 ? m % n : ( n - abs ( m%n ) ) % n; }

RealPoint Polygon::getVertex(int i) const {
    int a = modulo(i,(int) points.size());
    return points[a];
}

Segment Polygon::getSegment(int i) const {
    int a = modulo(i,(int) points.size());
    int b = (a+1) % points.size();
    // std::cout << "a: " << a << "   b: " << b << std::endl;
    return Segment(points[a], points[b]);
}

void Polygon::normalizeToBox(RealBox box) {
    auto t = [&](RealPoint p) {
        auto pp = ((p - box.pmin)/box.size() * 2.0) - RealPoint{1.0, 1.0};
        return pp;
    };
    for (auto &p: points) {
        p = t(p);
    }
}

RealBox Polygon::getBoundingBox() const { // as a segment: p1 is minx, miny and p2 is maxx, maxy
    RealBox result;
    for (auto& p: points) {
        result.add(p);
    }
    return result;
}

//std::vector<Chain> Polygon::chains() const {
//    const Polygon& polygon = *this;
//    std::vector<Chain> chains;
//    for (int i=0;i<(int)polygon.vertices.size();++i) {
//        Segment curr_segment = polygon.getSegment(i);
//        auto curr_boundary = boundary(curr_segment); // is it left boundary or top boundary?
//
//        if (curr_boundary == HORIZONTAL)
//            continue;
//
//        Segment prev_segment = polygon.getSegment(i-1);
//        auto prev_boundary = boundary(prev_segment);
//        
//        if (chains.size() == 0 || curr_boundary != prev_boundary) {
//            chains.push_back(Chain(i,curr_boundary));
//        }
//        else {
//            chains.back().grow();
//        }
//    }
//    
//    // check for a merge between first chain and last chain
//    if (chains.size() > 1 && chains.front().type == chains.back().type && chains.front().index == 0) {
//        chains.back().grow(chains.front().length);
//        chains.erase(chains.begin());
//    }
//    return chains;
//}


std::vector<Chain> Polygon::chains() const {
    const Polygon& polygon = *this;
    int first_chain_index = -1;
    std::vector<Chain> chains;
    for (int i=0;i<(int)polygon.points.size();++i) {
        Segment curr_segment = polygon.getSegment(i);
        
        auto curr_boundary = boundary(curr_segment); // is it LEFT, RIGHT or HORIZONTAL
        // auto prev_boundary = boundary(prev_segment); // is it LEFT, RIGHT or HORIZONTAL

        if (curr_boundary == HORIZONTAL) {
            if (chains.size())
                chains.back().grow();
        }
        else {
            // curr_boundary is either RIGHT or LEFT
            if (!chains.size()) {
                chains.push_back(Chain(i,curr_boundary));
                first_chain_index = i;
            }
            else if (curr_boundary != chains.back().type) {
                chains.push_back(Chain(i,curr_boundary));
            }
            else {
                chains.back().grow();
            }
        }
    }
    chains.back().grow(first_chain_index); // HORIZONTAL segments are added to the last chain
    
    if (chains.size() > 1 && chains.back().type == chains.front().type) {
        auto new_length = chains.back().length + chains.front().length;
        auto new_index = chains.back().index;
        
        chains.front().index  = new_index;
        chains.front().length = new_length;
        
        chains.resize(chains.size()-1); // get rid of last element;
    }
    
    return chains;
}


//------------------------------------------------------------------------------
// LLVertex
//------------------------------------------------------------------------------

LLVertex::LLVertex(RealPoint p):
    point(p)
{}

//------------------------------------------------------------------------------
// LLPolygon: Linked List Polygon
//------------------------------------------------------------------------------

LLPolygon::LLPolygon(const std::vector<RealPoint> &point) {
    for (auto p: point)
        this->insert(p); // keeps appending vertices
}

LLPolygon::LLPolygon(LLPolygon&& other) {
    this->entry_vertex = other.entry_vertex;
    other.entry_vertex = nullptr;
}

LLPolygon& LLPolygon::operator=(LLPolygon&& other) {
    this->entry_vertex = other.entry_vertex;
    other.entry_vertex = nullptr;
    return *this;
}

LLPolygon::~LLPolygon() {
    auto v = this->entry_vertex;
    if (v) {
        if (v->next == v) {
            delete v;
        }
        else {
            v->prev->next = nullptr; // make the next of the last vertex empty
            while (v) {
                auto vv = v->next;
                delete v;
                v = vv;
            }
        }
    }
    this->entry_vertex = nullptr;
}

LLVertex* LLPolygon::insert(const RealPoint &p, LLVertex* reference_vertex)
{
    auto new_vertex = new LLVertex(p);
    if (!entry_vertex) {
        entry_vertex = new_vertex;
        new_vertex->prev = new_vertex;
        new_vertex->next = new_vertex; // loop
    }
    else {
        if (reference_vertex == nullptr)
            reference_vertex = entry_vertex->prev;

        auto reference_vertex_next = reference_vertex->next;

        reference_vertex->next = new_vertex;
        new_vertex->prev = reference_vertex;

        new_vertex->next = reference_vertex_next;
        reference_vertex_next->prev = new_vertex;
    }
    return new_vertex;
}

//------------------------------------------------------------------------------
// GridIntersectSegment2 Impl.
//------------------------------------------------------------------------------

GridIntersectSegment2::GridIntersectSegment2(RealPoint p1, RealPoint p2):
    p1(p1),
    p2(p2),
    current_point(p1),
    delta(normalized(p2 - p1)),
    first(true)
{}

bool GridIntersectSegment2::next(RealPoint& next_point) {
    
    if (first) {
        first = false;
        next_point = current_point;
        return true; // p1 will be the first point!
    }
    
    const double EPSILON = 1.0e-8;
    
    auto next_coord = [](double v, double dv) {
        if (dv == 0) {
            return floor(v);
        }
        
        auto fraction = v - floor(v);
        if (fraction > 0.0)
            return floor(v) + ((dv > 0.0) ? 1.0 : 0.0);
        else
            return floor(v) + sgn(dv);
    };
    
    auto next_x = next_coord(current_point.x, delta.x);
    auto next_y = next_coord(current_point.y, delta.y);
    
    
    auto alpha_x = delta.x != 0 ? (next_x - current_point.x)/delta.x : 0;
    auto alpha_y = delta.y != 0 ? (next_y - current_point.y)/delta.y : 0;
    
    
    // assuming one of the alphas is != 0
    bool round[2] = { true, false };
    auto alpha = alpha_x;
    if (alpha_x == 0.0 || (alpha_y != 0.0 && alpha_y < alpha_x)) {
        alpha = alpha_y;
        round[0] = false;
        round[1] = true;
    }
    else if (alpha_x == alpha_y) {
        round[1] = true;
    }
    
    auto result = current_point + alpha * delta;
    
    if (round[0])
        result.x = next_x;
    if (round[1])
        result.y = next_y;
    
    auto distance_to_p2 = length(p2 - result);
    
    // is we get close enough to p2 than we are done
    // else if the p2 is closer to current point than to next point
    // then p2 is alse the next and last point
    if (distance_to_p2 < EPSILON || length(result-current_point) >= length(p2 - current_point)) {
        next_point = p2;
        current_point = next_point;
        return false;
    }
    else {
        next_point = result;
        current_point = next_point;
        return true;
    }
}

//------------------------------------------------------------------------------
// intersect_grid
//------------------------------------------------------------------------------

LLPolygon intersect_grid(const Polygon& polygon) {
    LLPolygon llpoly;
    for (auto i=0;i<polygon.points.size();++i) {
        auto segment = polygon.getSegment(i);
        if (segment.p1 == segment.p2)
            continue;
        GridIntersectSegment2 gis(segment.p1, segment.p2);
        RealPoint p;
        while (gis.next(p)) {
            llpoly.insert(p);
        }
    }
    return llpoly;
}

//------------------------------------------------------------------------------
// CellVertex
//------------------------------------------------------------------------------

CellVertex::CellVertex(LLVertex* v, Type type, int index, int component):
    vertex(v),
    type(type),
    index(index),
    component(component)
{}

void CellVertex::tag() {
    tagged = true;
}

void CellVertex::untag() {
    tagged = false;
}

bool CellVertex::isTagged() const {
    return tagged;
}

//------------------------------------------------------------------------------
// Cell
//------------------------------------------------------------------------------

Cell::Cell(GridPoint id):
    id(id)
{}

Cell::~Cell() {
    for (auto v: this->internal_vertices) {
        delete v;
    }
    for (auto &c: this->components) {
        for (auto v: c) {
            delete v;
        }
    }
}

void Cell::add(LLVertex* v, CellVertex::Type type, int component_id)  {
    
    if (latest_component_id < component_id) {
        components.push_back(CellComponent());
        latest_component_id = component_id;
    }
    
    if (!components.size())
        throw std::runtime_error("ooops");
    
    int current_component = (int) components.size() - 1;
    if (type == CellVertex::INTERNAL) {

        //
        // keep internal vertices in a separate list
        // since they are easy to collect later:
        // each internal vertext is an entry point to
        // a complete internal polygon that has no edge
        // intersection with any other polygon
        //

        internal_vertices.push_back(new CellVertex(v, type, (int) internal_vertices.size(), current_component));
    }
    else {
        auto &component = components.back();
        component.push_back(new CellVertex(v, type, (int) component.size(), current_component));
    }
}

std::vector<Polygon> Cell::polygons() {
    
    
    //
    // A cell is associated with a unit square whose edges
    // are in integer coordinates.
    //
    // A cell also points to a set of vertices on its boundaries
    // that are tagged as either ENTER, EXIT or ENTER_EXIT
    //

    //
    // Attributes:
    //
    //     LLVertex* vertex;
    //     Type      type;
    //     int       index;
    //     int       index_cw; // clockwise index
    //     bool      tagged;
    //

    // how do we collect the polygonal pieces that intersect
    // this cell?
    
    //
    // original polygons that were all internal polygons
    // are collected as they are
    //
    
    //
    // the vertices that intersect the boundary
    //
    
    
    RealVec real_cell_id(this->id.x, this->id.y);
    
    RealVec corners[4] {
        { real_cell_id.x       , real_cell_id.y          },
        { real_cell_id.x       , real_cell_id.y   + 1.0  },
        { real_cell_id.x + 1.0 , real_cell_id.y   + 1.0  },
        { real_cell_id.x + 1.0 , real_cell_id.y          }};
    
    // check if all vertices are EXIT_ENTRY if so,
    // make one of the them the ENTRY
    
    enum Corners  { BOTTOM_LEFT=0, TOP_LEFT=1, TOP_RIGHT=2,  BOTTOM_RIGHT=3 };
    
    
    
    std::vector<Polygon> polygons; // result
    
    
    //--------------------------------------------------
    // easy case: collect internal polygons
    for (auto v: internal_vertices) {
        Polygon internal_polygon;
        auto u = v->vertex;
        do {
            internal_polygon.add(u->point);
            u = u->next;
        } while (u != v->vertex);
        polygons.push_back(internal_polygon);
    }
    //--------------------------------------------------
    
    auto process = [&](CellComponent &vertices) {

        bool used_corners[4] = { false, false, false, false };

        //    bool all_vertices_are_entry_exit = true;
        std::for_each(vertices.begin(), vertices.end(), [&corners, &used_corners](CellVertex *v) {
            //        if (v->type != CellVertex::EXIT_ENTRY)
            //            all_vertices_are_entry_exit = false;
            for (auto i=0;i<4;++i)
                if (v->vertex->point == corners[i])
                    used_corners[i] = true;
        });
        
        //
        // sort all vertices in the following manner
        //
        //
        //  |-- B --|
        //  A       C
        //  |-- D --|
        //
        //
        
        auto linear_coefficient = [&real_cell_id](const RealPoint &p) {
            auto delta = p - RealVec(real_cell_id.x, real_cell_id.y);
            if (delta.x == 0)
                return delta.y;
            else if (delta.y == 1)
                return 1 + delta.x;
            else if (delta.x == 1)
                return 2.0 + 1.0 - delta.y;
            else
                return 3.0 + 1.0 - delta.x;
        };
        
        //
        //    enum Side { LEFT=0, TOP=1, RIGHT=2, BOTTOM=3 };
        //
        //    RealPoint real_cell_id((double)id.x, (double)id.y);
        //    auto sideOf = [&real_cell_id](const RealPoint &p) -> Side {
        //        if (p.x == real_cell_id.x)
        //            return LEFT;
        //        else if (p.y == real_cell_id.y + 1.0)
        //            return TOP;
        //        else if (p.x == real_cell_id.x + 1.0)
        //            return RIGHT;
        //        else if (p.y == real_cell_id.y)
        //            return BOTTOM;
        //        else
        //            throw std::runtime_error("oops");
        //    };
        //
        auto compare_lt = [&linear_coefficient](const CellVertex *c1, const CellVertex *c2) {
            auto x1 = linear_coefficient(c1->vertex->point);
            auto x2 = linear_coefficient(c2->vertex->point);
            if (x1 < x2) {
                return true;
            }
            else if (x1 == x2) {
                if (c1->type == CellVertex::CORNER && c2->type != CellVertex::CORNER) {
                    return true;
                }
                else {
                    return false;
                }
            }
            return false;
        };
        
        LLVertex ll_vertices[4] {
            corners[0],
            corners[1],
            corners[2],
            corners[3] } ;
        
        CellVertex cell_vertices[4] {
            {&ll_vertices[0], CellVertex::CORNER, -1, -1},
            {&ll_vertices[1], CellVertex::CORNER, -1, -1},
            {&ll_vertices[2], CellVertex::CORNER, -1, -1},
            {&ll_vertices[3], CellVertex::CORNER, -1, -1} } ;
        
        // sort vertices in cw order
        std::vector<CellVertex*> vertices_cw;
        
        // untag all vertices
        std::for_each(vertices.begin(), vertices.end(), [&vertices_cw](CellVertex *v) {
            v->untag();
            vertices_cw.push_back(v);
        });
        for (auto i=0;i<4;i++)
            if (!used_corners[i])
                vertices_cw.push_back(&cell_vertices[i]);
        
        std::sort(vertices_cw.begin(), vertices_cw.end(), compare_lt);
        
        // set cw indices
        for (auto it = vertices_cw.begin(); it != vertices_cw.end(); ++it) {
            int index = (int) (it - vertices_cw.begin());
            (*it)->index_cw = index;
        }
        
        auto next_cw = [&vertices_cw](int index_cw) {
            return vertices_cw[(index_cw + 1) % vertices_cw.size()];
        };
        
        auto next = [&vertices](const CellVertex *v) {
            return vertices[(v->index + 1) % vertices.size()];
        };
        
        // now build the polygons...
        //    - every time we find an "entry" vertex, we follow its
        //      next list until we get to a next boundary vertex that
        //      should be the next to show up (in cyclic manner) in the
        //      this->vertices list and should be a "exit" vertex.
        
        for (auto u: vertices) {
            
            if (u->isTagged())
                continue;
            
            //
            // types of vertices tags
            //
            // INTERNAL
            // EXIT
            // ENTRY_EXIT
            // EXIT_ENTRY
            // CORNER
            //
            
            if (u->type != CellVertex::ENTRY) {
                continue;
            }
            
            Polygon poly;
            // std::cout << "poly: " << std::endl;
            
            while (true) {
                
                // u->v is a path inside the cell
                u->tag();
                auto v = next(u);
                
                // extract polygon
                auto it  = u->vertex; // entry vertex
                auto begin = it; // next vertex using polygon order to cross the boundary
                auto end   = v->vertex; // next vertex using polygon order to cross the boundary
                
                // std::cout << "... adding poly point: " << it->pos << std::endl;
                poly.add(it->point);
                it = it->next;
                while (it != end) {
                    // std::cout << "... adding poly point: " << it->pos << std::endl;
                    poly.add(it->point);
                    it = it->next;
                }
                if (begin != end) {
                    poly.add(it->point);
                }
                
                v->tag();
                
                if (begin == end) { // finished a polygon
                    polygons.push_back(std::move(poly));
                    break;
                }
                
                // got to exit vertex, now find next vertex 
                // on cw order ( or corner )
                
                auto w = next_cw(v->index_cw);
                
                if (w->isTagged()) { // done
                    polygons.push_back(std::move(poly));
                    break;
                }
                
                while (w->type == CellVertex::CORNER) {
                    poly.add(w->vertex->point);
                    // std::cout << "... adding poly point: " << w->vertex->pos << std::endl;
                    w = next_cw(w->index_cw);
                }
                
                //            if (w->type == CellVertex::EXIT)
                //                throw std::runtime_error("needs to be an entry point");
                
                if (!w->isTagged()) {
                    u = w;
                }
                else { // finished a polygon
                    polygons.push_back(std::move(poly));
                    break;
                }
            }
        }
    }; // end process

    // process each component separately
    for (auto &component: components) {
        process(component);
    }
    
#if 0
    // TODO: clean this HACK of removing repeated vertices
    for (auto &p: polygons) {
        auto it = p.vertices.begin();
        while (it != p.vertices.end()) {
            auto it2 = it + 1;
            if (it2 == p.vertices.end())
                it2 = p.vertices.begin();
            
            // here we are talking about grid points so
            auto len = length(it->point - it2->point);
            if (len < 1e-10)
                it = p.vertices.erase(it);
            else
                ++it;
        }
    }
#endif

    return polygons;

}

//------------------------------------------------------------------------------
// CellMap
//------------------------------------------------------------------------------

void CellMap::add(GridPoint cell_id, LLVertex* vertex, CellVertex::Type type)  {
    Cell* cell = nullptr;
    auto it = map.find(cell_id);
    if (it == map.end()) {
        cell = new Cell(cell_id);
        map[cell_id] = cell;
    }
    else {
        cell = it->second;
    }
    
    cell->add(vertex, type, current_component);
}

Cell& CellMap::operator()(const GridPoint &p) {
    auto it = map.find(p);
    if (it == map.end()) {
        throw std::runtime_error("ooops");
    }
    else {
        return *it->second;
    }
}

#define xLOG_MARK_CELLS_THAT_INTERSECT_POLYGON

void CellMap::markCellsThatIntersectPolygon(LLPolygon &llpoly) {
    
    ++current_component;
    
    CellMap &cell_map = *this;
    
    
    //
    // if
    //       llpoly is totally contained in a single cell
    // then
    //       add one and only one internal vertex of that polygon
    //       to the corresponding cell.
    // else
    //       all cells in the cell will be either ENTRY, EXIT or EXIT_ENTRY
    //
    
    LLVertex *u = llpoly.entry_vertex;
    
    // TODO: replace this with something more efficient
    std::set<GridPoint> cells_with_either_internal_or_enter_exit_vertices;
    std::set<GridPoint> cells_with_enter_exit_vertices;
    
    while (true) {
        
#ifdef LOG_MARK_CELLS_THAT_INTERSECT_POLYGON
        std::cerr << "processing vertex at position: " << u->vertex.point << std::endl;
#endif
        
        VertexType u_type(u);
        
        auto process = [&](VertexType::CellID cell_id) {
            auto c         = u_type.getCase(cell_id);
            auto cell_addr = u_type.getCellAddress(cell_id);
            
#ifdef LOG_MARK_CELLS_THAT_INTERSECT_POLYGON
            std::cerr << "....vertex at " << u_type.u->vertex.point << " on cell " << cell_addr << " is " << c << std::endl;
#endif
            
            switch (c) {
                case VertexType::ENTER_VERTEX:
                    cells_with_enter_exit_vertices.insert(cell_addr);
                    cell_map.add(cell_addr, u_type.u, CellVertex::ENTRY);
                    break;
                case VertexType::EXIT_VERTEX:
                    cells_with_enter_exit_vertices.insert(cell_addr);
                    cell_map.add(cell_addr, u_type.u, CellVertex::EXIT);
                    break;
                case VertexType::EXIT_ENTER_VERTEX:
                    cells_with_either_internal_or_enter_exit_vertices.insert(cell_addr);
                    //                    cell_map.add(u_type.getCellAddress(cell_id), u_type.u, CellVertex::EXIT_ENTRY);
                    //                    ++vertices_added;
                    break;
                case VertexType::INTERNAL_VERTEX:
                    cells_with_either_internal_or_enter_exit_vertices.insert(cell_addr);
                    // cell_map.add(u_type.getCellAddress(cell_id), u_type.u, CellVertex::INTERNAL);
                    // TODO: prove a lemma saying we don't need these vertices...
                    break;
                default:
                    break;
            }
        };
        
        process(VertexType::BASE_CELL);
        process(VertexType::BELOW_CELL);
        process(VertexType::LEFT_CELL);
        process(VertexType::LEFT_BELOW_CELL);
        
        u = u->next;
        
        if (u == llpoly.entry_vertex)
            break;
    }
    
    for (auto cell_addr: cells_with_either_internal_or_enter_exit_vertices) {
        if (!cells_with_enter_exit_vertices.count(cell_addr))
            cell_map.add(cell_addr, llpoly.entry_vertex, CellVertex::INTERNAL); // it can be considered internal
    }
    
}


//-----------------------------------------------------------------
// VertexType
//-----------------------------------------------------------------

//struct VertexType {
//public:
//    enum Tag  { UNDEFINED, INTERNAL, EDGE, CORNER };
//    enum Edge { NO_EDGE, LEFT_EDGE, TOP_EDGE, RIGHT_EDGE, BOTTOM_EDGE };
//    enum CellID { BASE_CELL=0, LEFT_CELL=1, BELOW_CELL=2, LEFT_BELOW_CELL=3 };
//    enum Case { DONT_ADD, ENTER_EXIT_VERTEX, ENTER_VERTEX, EXIT_VERTEX };
//public:

std::ostream& operator<<(std::ostream& os, const VertexType::Tag& tag) {
    switch(tag) {
        case VertexType::UNDEFINED:
            os << "\"UNDEFINED\"";
            break;
        case VertexType::INTERNAL:
            os << "\"INTERNAL\"";
            break;
        case VertexType::VERTICAL_EDGE:
            os << "\"VERTICAL_EDGE\"";
            break;
        case VertexType::HORIZONTAL_EDGE:
            os << "\"HORIZONTAL_EDGE\"";
            break;
        case VertexType::CORNER:
            os << "\"CORNER\"";
            break;
        default:
            throw std::runtime_error("ooops");
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const VertexType::Edge& edge) {
    switch(edge) {
        case VertexType::NO_EDGE:
            os << "\"NO_EDGE\"";
            break;
        case VertexType::LEFT_EDGE:
            os << "\"LEFT_EDGE\"";
            break;
        case VertexType::TOP_EDGE:
            os << "\"TOP_EDGE\"";
            break;
        case VertexType::RIGHT_EDGE:
            os << "\"RIGHT_EDGE\"";
            break;
        case VertexType::BOTTOM_EDGE:
            os << "\"BOTTOM_EDGE\"";
            break;
        default:
            throw std::runtime_error("ooops");
    }
    return os;
}

//    enum CellID { BASE_CELL=0, LEFT_CELL=1, BELOW_CELL=2, LEFT_BELOW_CELL=3 };


std::ostream& operator<<(std::ostream& os, const VertexType::CellID& cell_id) {
    switch(cell_id) {
        case VertexType::NO_CELL:
            os << "\"NO_CELL\"";
            break;
        case VertexType::BASE_CELL:
            os << "\"BASE_CELL\"";
            break;
        case VertexType::LEFT_CELL:
            os << "\"LEFT_CELL\"";
            break;
        case VertexType::BELOW_CELL:
            os << "\"BELOW_CELL\"";
            break;
        case VertexType::LEFT_BELOW_CELL:
            os << "\"LEFT_BELOW_CELL\"";
            break;
        default:
            throw std::runtime_error("ooops");
    }
    return os;
}

//    enum Case { DONT_ADD, ENTER_EXIT_VERTEX, ENTER_VERTEX, EXIT_VERTEX };

std::ostream& operator<<(std::ostream& os, const VertexType::Case& c) {
    switch(c) {
        case VertexType::DONT_ADD:
            os << "\"DONT_ADD\"";
            break;
        case VertexType::EXIT_ENTER_VERTEX:
            os << "\"EXIT_ENTER_VERTEX\"";
            break;
        case VertexType::ENTER_VERTEX:
            os << "\"ENTER_VERTEX\"";
            break;
        case VertexType::EXIT_VERTEX:
            os << "\"EXIT_VERTEX\"";
            break;
        case VertexType::INTERNAL_VERTEX:
            os << "\"INTERNAL_VERTEX\"";
            break;
        default:
            throw std::runtime_error("ooops");
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, const VertexType& v) {
    os << "{ "
    << "\"tag\":" << v.tag << ", "
    << "\"edge\":" << v.edge;
    
    for (int i=0;i<4;i++) {
        VertexType::CellID cell_id = (VertexType::CellID) i;
        os << ", " << cell_id << ":" << v.cell_cases[i];
    }
    os << " }";
    return os;
}

VertexType::Case VertexType::getCase(CellID cell_id) const {
    return cell_cases[(int) cell_id];
}

GridPoint VertexType::getCellAddress(CellID cell_id) const {
    GridPoint p = toGridPoint(u->point);
    if (cell_id == LEFT_BELOW_CELL) {
        return p + GridPoint(-1,-1);
    }
    else if (cell_id == LEFT_CELL) {
        return p + GridPoint(-1,0);
    }
    else if (cell_id == BELOW_CELL) {
        return p + GridPoint(0,-1);
    }
    else if (cell_id == BASE_CELL) {
        return p;
    }
    else {
        throw std::runtime_error("ooops");
    }
}

VertexType::VertexType(LLVertex *u):
    u(u)
{
    
    auto frac = fraction(u->point);

    auto t  = u->prev;
    auto v  = u->next;

    auto tu = u->point - t->point;
    auto uv = v->point - u->point;
    
    auto &that = *this;
    
    auto update_case = [&that] (CellID cell_id, Case new_case) {
        if (cell_id == NO_CELL)
            return;
        int index = (int) cell_id;
        auto old_case = that.cell_cases[index];
        if (old_case == EXIT_VERTEX && new_case == ENTER_VERTEX) {
            that.cell_cases[index] = EXIT_ENTER_VERTEX;
        }
        else if (old_case == DONT_ADD){
            that.cell_cases[index] = new_case;
        }
        else {
            throw std::runtime_error("case not expected");
        }
    };
    
    tag = UNDEFINED;
    if (frac.x > 0 && frac.y > 0) { // INTERNAL
        tag = INTERNAL;
    }
    else if (frac.x > 0 && frac.y == 0) {
        tag = HORIZONTAL_EDGE;
    }
    else if (frac.x == 0 && frac.y > 0) {
        tag = VERTICAL_EDGE;
    }
    else {
        tag = CORNER;
    }

    //
    // |----+---+---+---|   |---+----+----+---|
    // | L  | . | . | . |   |   |    |    |   |
    // | L  | . | . | . |   |   | VE | I  |   |
    // | L  | . | . | . |   |   | C  | HE |   |
    // | LB | B | B | B |   |   |    |    |   |
    // |----+---+---+---|   |---+----+----+---|
    //
    
    auto classify_vector = [](Tag tag, RealVec ab, bool inward) {
      
        const CellID S  = BELOW_CELL;
        const CellID W  = LEFT_CELL;
        const CellID SW = LEFT_BELOW_CELL;
        const CellID B  = BASE_CELL;
        const CellID Z  = NO_CELL;
        
        const CellID map_outward[6][6] = {
            { SW , SW , S  , S  , S  , S  },
            { W  , Z  , S  , B  , Z  , S  },
            { W  , B  , B  , B  , B  , B  },
            { W  , W  , B  , B  , B  , B  },
            { W  , Z  , B  , B  , Z  , B  },
            { W  , B  , B  , B  , B  , B  }
        };

        const CellID map_inward[6][6] = {
            { SW , S  , S  , S  , S  , S  },
            { SW , Z  , B  , S  , Z  , B  },
            { W  , W  , B  , B  , B  , B  },
            { W  , B  , B  , B  , B  , B  },
            { W  , Z  , B  , B  , Z  , B  },
            { W  , W  , B  , B  , B  , B  }
        };
        
        GridPoint tag2gridpoint[4] {
            { 4, 4 }, // INTERNAL
            { 4, 1 }, // HORIZONTAL_EDGE
            { 1, 4 }, // VERTICAL_EDGE
            { 1, 1 }  // CORNER
        };
        
        GridPoint map_reference_cell = tag2gridpoint[(int) tag];
        
        auto sign = [](RealVec v) {
            return GridPoint( v.x > 0 ? 1 : (v.x < 0 ? -1 : 0),
                              v.y > 0 ? 1 : (v.y < 0 ? -1 : 0) );
        };

        CellID cell = NO_CELL;
        
        if (inward) {
            GridPoint p = map_reference_cell + (-1 * sign(ab));
            auto row = p.y;
            auto col = p.x;
            cell = map_inward[row][col];
        }
        else {
            GridPoint p = map_reference_cell + sign(ab);
            auto row = p.y;
            auto col = p.x;
            cell = map_outward[row][col];
        }
        
        return cell;

    };
    
    if (tag == INTERNAL) {
        
        update_case(BASE_CELL, INTERNAL_VERTEX);
        
    }
    else {
        CellID inward_cell  = classify_vector( tag, tu, true  );
        CellID outward_cell = classify_vector( tag, uv, false );
        
        update_case( inward_cell,  EXIT_VERTEX  );
        update_case( outward_cell, ENTER_VERTEX );
    }

}


//------------------------------------------------------------------------------
// ColumnEntry
//------------------------------------------------------------------------------

ColumnEntry::ColumnEntry(int column):
    column(column)
{}

void ColumnEntry::update(GridCellEvent event) {
    switch (event) {
    case BOUNDARY_HIT:
        ++boundary_hit;
        break;
    case START_INTERIOR:
        ++start_interior;
        break;
    case END_INTERIOR:
        ++end_interior;
        break;
    default:
        break;
    }
}

//------------------------------------------------------------------------------
// Row
//------------------------------------------------------------------------------

Row::Row(int row):
    row(row)
{}

ColumnEntry& Row::getColumn(int column) {
    auto it = std::lower_bound(entries.begin(), entries.end(), column,
                               [](const ColumnEntry& col_entry, int column) {
                                   return col_entry.column < column;
                               });
    if (it == entries.end()) {
        entries.push_back(ColumnEntry(column));
        return entries.back();
    }
    else if (it->column == column) {
        return *it;
    }
    else {
        return *entries.insert(it,ColumnEntry(column));
    }
}

void Row::update(int column, GridCellEvent event) {
    ColumnEntry& col_entry = getColumn(column);
    col_entry.update(event);
}

//------------------------------------------------------------------------------
// Grid
//------------------------------------------------------------------------------

Grid::Grid(int width, int height):
    width(width),
    height(height)
{
    rows.resize(height);
}

Row& Grid::getRow(int row) {
    auto result = rows.at(row).get();
    if (!result) {
        result = new Row(row);
        rows.at(row).reset(result);
    }
    return *result;
}

void Grid::update(const GridPoint &p, GridCellEvent cell_event) {
    auto& row = getRow(p.y);
    row.update(p.x, cell_event);
}

//------------------------------------------------------------------------------
// IO
//------------------------------------------------------------------------------

std::ostream& operator<<(std::ostream& os, const Polygon& poly) {
    os << "{ " 
       << "\"type\":"   << "\"polygon\", "
       << "\"points\":" << "[";
    std::copy(poly.points.begin(), poly.points.end(), infix_ostream_iterator<RealPoint>(os, ","));
    os << "] }";
    return os;
}

std::ostream& operator<<(std::ostream& os, const Segment& segment) {
    os << "{ " 
       << "\"type\":"   << "\"segment\", "
       << "\"p1\":" << segment.p1 << ", "
       << "\"p2\":" << segment.p2 << " }";
    return os;
}


std::ostream& operator<<(std::ostream& os, const Grid& grid) {

    auto rep = [](std::string st, int k) -> std::string {
        std::stringstream ss;
        for (int i=0;i<k;++i)
            ss << st;
        return ss.str();
    };

    for (auto it=grid.rows.rbegin();it!=grid.rows.rend();++it) {
        auto row = it->get();
        if (!row) {
            os << rep("0 ",grid.width) << std::endl;
            continue;
        }
        int current_col = 0;
        for (const auto &col_entry: row->entries) {
            int k = col_entry.column - current_col;
            if (k > 0) {
                os << rep("0 ",k);
            }
            os << col_entry.boundary_hit << " ";
            current_col = col_entry.column + 1;
        }
        os << rep("0 ",grid.width - current_col);
        os << std::endl;
        
    }
    return os;
}

std::ostream &operator<<(std::ostream& os, const Reporter &reporter) {
    os << "{";
    os << "\"polygons\":" << "[";
    std::copy(reporter.polygons.begin(), 
              reporter.polygons.end(), 
              infix_ostream_iterator<Polygon>(os, ","));
    os << "], ";
    os << "\"segments\":" << "[";
    std::copy(reporter.segments.begin(), 
              reporter.segments.end(), 
              infix_ostream_iterator<Segment>(os, ","));
    os << "], ";
    os << "\"outgoing_unit_segments\":" << "[";
    std::copy(reporter.outgoing_unit_segments.begin(), 
              reporter.outgoing_unit_segments.end(), 
              infix_ostream_iterator<Segment>(os, ","));
    os << "], ";
    os << "\"intersection_points\":" << "[";
    std::copy(reporter.grid_intersection_points.begin(), 
              reporter.grid_intersection_points.end(), 
              infix_ostream_iterator<RealPoint>(os, ","));
    os << "], ";
    os << "\"start_interior_cells\":" << "[";
    std::copy(reporter.start_interior_cells.begin(), 
              reporter.start_interior_cells.end(), 
              infix_ostream_iterator<GridPoint>(os, ","));
    os << "], ";
    os << "\"end_interior_cells\":" << "[";
    std::copy(reporter.end_interior_cells.begin(), 
              reporter.end_interior_cells.end(), 
              infix_ostream_iterator<GridPoint>(os, ","));
    os << "], ";
    os << "\"decompositions\":" << "[";
    std::copy(reporter.decompositions.begin(),
              reporter.decompositions.end(),
              infix_ostream_iterator<std::string>(os, ","));
    os << "], ";
    os << "\"grid_cells\":" << "[";
    std::copy(reporter.grid_cells.begin(), 
              reporter.grid_cells.end(), 
              infix_ostream_iterator<GridPoint>(os, ","));
    os << "] }";
    return os;
}


//------------------------------------------------------------------------------
// raster_segment
//------------------------------------------------------------------------------

std::vector<GridPoint> raster_segment(RealPoint p1, RealPoint p2) {

    struct DummyReporter {
        void polygon(const Polygon &) {}
        void segment(const Segment &) {}
        void grid_intersection_point(const RealPoint &) {}
        void grid_cell(const GridPoint &) {}
    };

    DummyReporter *dummy_reporter = nullptr;

    return raster_segment(p1, p2, dummy_reporter);

}



Polygon random_simple_grid_polygon(int width, int height, int num_sides, uint32_t seed)
{
    std::default_random_engine generator(seed);

    std::uniform_real_distribution<double> width_distribution(0,width);
    std::uniform_real_distribution<double> height_distribution(0,height);

    std::set<RealPoint> point_set;
    while (point_set.size() < num_sides) {
        // auto old_size = point_set.size();
        auto p = RealPoint(width_distribution(generator), height_distribution(generator));
        point_set.insert(p);
        // if (point_set.size() > old_size)
        //     std::cout << p << std::endl;
    }

    std::vector<RealPoint> points(point_set.size());
    std::copy(point_set.begin(), point_set.end(), points.begin());

    int n = (int) point_set.size();

    // std::copy(points.begin(), 
    //           points.end(), 
    //           infix_ostream_iterator<GridPoint>(std::cout, ","));

    // total perimeter reduces every time
    while (true) {
        // auto i = 0;
        bool finish = true;
        for (int i=0;i<n;++i) {
            for (int j=i+2;j<n;++j) {
                if (intersect(points[i],
                              points[i+1],
                              points[j],
                              points[(j+1) % n])) {

                    // std::cout << "swap" << std::endl;
                    std::swap(points[i+1], points[j]);
                    i = n;
                    j = n;
                    finish = false;
                }
            }
        }
        if (finish)
            break;
    }

    // std::cout << "After Permutation: " << std::endl;
    // std::copy(points.begin(), 
    //           points.end(), 
    //           infix_ostream_iterator<GridPoint>(std::cout, ","));


    Polygon poly;
    for (auto &p: points) {
        // std::cout << "push" << p << std::endl;
        poly.points.push_back(RealPoint(p.x, p.y));
    }

    // std::cout << poly << std::endl;

    return poly;
}



//-------------------------------------------------------------------------------
// TileCoverEngine Impl.
//-------------------------------------------------------------------------------

TileCoverEngine::TileCoverEngine(int max_level, int max_texture_level):
    max_level(max_level),
    max_texture_level(max_texture_level)
{
    mipmap_p.reset(new MipMap(1,std::min(max_level,max_texture_level)));
}

#define xLOG_COMPUTE_TREE_DECOMPOSITION
#define LOG_COMPUTE_TREE_DECOMPOSITION_VERBOSITY 0

labeled_tree::Node* TileCoverEngine::computeCover(const std::vector<Polygon> &mercator_polygons) {

#ifdef LOG_COMPUTE_TREE_DECOMPOSITION
    std::cout << "[CALL] compute_tree_decomposition (max_level="  << max_level  << ")" << std::endl;
    std::string prefix(4,'.');
#endif
    
    RealBox bbox;
    for (auto p: mercator_polygons) {
        bbox.add(p.getBoundingBox());
    }
    
#ifdef LOG_COMPUTE_TREE_DECOMPOSITION
    std::cout << prefix << bbox.size() << std::endl;
#endif

    auto width  = bbox.width();
    auto height = bbox.height();

    //
    // width_level = \argmax_{k} \{ 2 / 2^k >= width \}
    // exact case:
    //
    //      k = \log_{2} { 2 / width }
    //
    auto width_level  = (int) floor(log2(2.0/width));
    auto height_level = (int) floor(log2(2.0/height));

    // how do we align with the quadtree grid???
    auto level = std::min(width_level, height_level);

    
#ifdef LOG_COMPUTE_TREE_DECOMPOSITION
    std::cout << prefix << "width_level: "  << width_level  << std::endl;
    std::cout << prefix << "height_level: " << height_level << std::endl;
    std::cout << prefix << "level: "        << level        << std::endl;
#endif

    
    //
    // What is the decomposition strategy:
    //
    // from [level to max_level]
    //
    // assume k is the maximum rasterization we do at one shot
    // (obs. there is an interesting tradeoff there).  
    //
    // If (max_level - level <= k) then we can rasterize in a
    // single shot.
    //
    // Else we will assume a simple strategy:
    //
    //     level
    //     (level + (max_level-level) mod k)
    //     ...
    //     (max_level - 3k)
    //     (max_level - 2k)
    //     (max_level - k)
    //
    // Example: k=8, level=7, max_level=25
    //
    //     7   
    //     9  = 25 - 16 
    //     17 = 25 - 8
    //

//    auto t0 = ::maps::Tile(::maps::MercatorPoint(bbox.pmin.x, bbox.pmin.y), level);
//    auto t1 = ::maps::Tile(::maps::MercatorPoint(bbox.pmax.x, bbox.pmax.y), level);
//    if (t0.x.quantity != t1.x.quantity || t0.y.quantity != t1.y.quantity) {
//        --level;
//    }
    
    int k = max_texture_level - 1;
    std::deque<int> levels;
    std::deque<int> depths;
    auto remaining = max_level - level;
    auto current_level = max_level;
    while (remaining > 0) {

        auto l = std::min(k, remaining);

        levels.push_front(current_level-l);
        depths.push_front(l);

        remaining     -= l;
        current_level -= l;

    }
    if (levels.size() == 0) {
        levels.push_back(level);
        depths.push_back(0);
    }

    // result structure shared by all polygon rasterization
    labeled_tree::CoverTreeEngine engine;

#ifdef LOG_COMPUTE_TREE_DECOMPOSITION
#if LOG_COMPUTE_TREE_DECOMPOSITION_VERBOSITY>1
    {
        std::cerr << "-------------------------------------------" << std::endl;
        std::cerr << "TREE_DECOMPOSITION" << std::endl;
        std::stringstream ss;
        ::labeled_tree::text(ss, *engine.root.get());
        std::cerr << ss.str() << std::endl;
        std::cerr << "-------------------------------------------" << std::endl;
    }
#endif
#endif

    
    
    // mipmap range
    auto &mipmap = *mipmap_p.get();
    // MipMap mipmap(1,k+1); // 50% of the time is being spent here

    Reporter reporter;
    
    using ProcessPolyType = std::function<void(const std::vector<Polygon>&, const RealBox&, int)>;
    
#ifdef LOG_COMPUTE_TREE_DECOMPOSITION
    int COUNT = -1;
#endif
    
    struct CountIteration {
        CountIteration(labeled_tree::CoverTreeEngine &engine):
            engine(engine)
        {
            engine.iteration_tag++;
        }
        ~CountIteration() {
            engine.iteration_tag--;
        }
        labeled_tree::CoverTreeEngine &engine;
    };
    
    // routine to process polygons recursively
    ProcessPolyType process = [&](const std::vector<Polygon> &polygons, const RealBox& bbox, int recursion_level) {
        CountIteration count_iteration(engine);
        

#ifdef LOG_COMPUTE_TREE_DECOMPOSITION
        int iteration_id = ++COUNT;
        std::string prefix = std::string(recursion_level * 4 + 4,'.');
        std::cerr << prefix << "[CALL] process (" << mercator_polygons.size() <<  " polygons with, recursion_level: " << recursion_level << ", id: " << iteration_id << ")" << std::endl;
        prefix = std::string(recursion_level * 4 + 8,'.');
        for (auto &p: mercator_polygons) {
            std::cerr << prefix << p << std::endl;
        }
#endif
        
        auto current_level = levels[recursion_level];
        auto current_depth = depths[recursion_level];

#if 0
        {
            Reporter aux_reporter;
            auto pcopy = mercator_polygon;
            for (auto &v: pcopy.vertices) {
                auto &p = v.point;
//                p = p * 8.0;
                p = ((p + RealPoint{1.0,1.0}) * 0.5) * 16.0;
            }
            aux_reporter.polygon(pcopy); // register polygon
            // reporter.polygon(polygon);
            std::ofstream ff("/tmp/report-polygons_l" + std::to_string(recursion_level) + "_p" + std::to_string(COUNT++) + ".json");
            ff << aux_reporter;
        }
#endif

        auto top_right_boundary_tile = [](double mx, double my, int zoom) {
            int tiles_by_side = 1 << zoom;
            
            // mercator
            auto x = ((1.0f + mx)/2.0f * tiles_by_side);
            auto y = ((1.0f + my)/2.0f * tiles_by_side);
            
            int xx = (int) x;
            int yy = (int) y;
            
            if (x == floor(x))
                --xx;
            if (y == floor(y))
                --yy;
            
            return maps::Tile(xx, yy, zoom);
        };

        auto t0 = maps::Tile(maps::MercatorPoint(bbox.pmin.x, bbox.pmin.y), current_level);
        auto t1 = top_right_boundary_tile(bbox.pmax.x, bbox.pmax.y, current_level);
        
        if (t0.x.quantity > t1.x.quantity)
            t1.x.quantity = t0.x.quantity;
        if (t0.y.quantity > t1.y.quantity)
            t1.y.quantity = t0.y.quantity;
        
        int x_blocks = t1.x.quantity - t0.x.quantity + 1;
        int y_blocks = t1.y.quantity - t0.y.quantity + 1;

        if (x_blocks > 1 || y_blocks > 1)
            ++current_depth;

#ifdef LOG_COMPUTE_TREE_DECOMPOSITION
        using namespace ::maps::io;
        std::cerr << prefix << "grid will be 2^" << current_depth << " x 2^" << current_depth << std::endl;
        std::cerr << prefix << "t0: " << t0 << std::endl;
        std::cerr << prefix << "t1: " << t1 << std::endl;
#endif
    
        RealBox gridbox;
        {
            auto pmin = t0.bounds().min;
            auto pmax = t1.bounds().max;
            gridbox.add({pmin.getX(), pmin.getY()});
            gridbox.add({pmax.getX(), pmax.getY()});
        }

        // what is the alignment?
        int grid_size = 1 << current_depth;
        auto transform = [&gridbox, grid_size](const RealPoint& p) {
            auto aux = ((p - gridbox.pmin)/gridbox.size().max());
            auto aux2 = aux * RealPoint(grid_size, grid_size);
            return aux2;
        };

        auto inverse_transform = [&gridbox, grid_size](const RealPoint& p) {
            auto aux = p / RealPoint(grid_size, grid_size);
            auto aux2 = (aux * gridbox.size().max()) + gridbox.pmin;
            return aux2;
        };
        
        std::vector<Polygon> grid_polygons;
        for (auto &p: polygons) {
            grid_polygons.push_back(Polygon(p, transform));
            auto &poly = grid_polygons.back();
            //
            // TODO: analysis of problems this can generate
            //
            // simplify grid_polygon by moving every cooridante that is
            // within EPSILON distance to a grid coord to the grid coord
            for (auto &p: poly.points) {
                double grid_coord_x = std::floor(p.x + 0.5);
                double grid_coord_y = std::floor(p.y + 0.5);
                if (fabs(p.x - grid_coord_x) < 1e-7) {
                    p.x = grid_coord_x;
                }
                if (fabs(p.y - grid_coord_y) < 1e-7) {
                    p.y = grid_coord_y;
                }
            }
            // TODO: check if we still have a simple polygon

            // TODO: a polygon tiny area on grid coords won't be
            // considered
            if (poly.area() < 1e-8) {
                grid_polygons.pop_back();
            }
        }

        // only tiny or empty polygons, no updates to the cover tree
        if (grid_polygons.size() == 0) {
            return;
        }
        
        // simplify grid polygon
        
#ifdef LOG_COMPUTE_TREE_DECOMPOSITION
        for (auto &p: grid_polygons) {
            std::cout << prefix << "grid_polygon: " << p << std::endl;
        }
#endif
        Reporter reporter;
        auto grid = raster_polygon2(grid_polygons, &reporter);
        
        //
        mipmap.clear();
        mipmap.setActiveMaxRes(current_depth);
    
        // engine.goTo(::labeled_tree::Path(t0));
        
        for (auto &row: grid.rows) {
            if (!row.get())
                continue;

            std::size_t  row_no = (std::size_t) row->row;
            
            auto sum = 0;
            
            bool first = true;
            std::size_t previous_column = 0;
            
            for (auto &col: row->entries) {
                
                std::size_t col_no = (std::size_t) col.column;

                if (!first && col_no - previous_column > 1 && (sum % 2) == 1) {
                    for (auto j=previous_column+1;j<col_no;++j) {
                        mipmap.turnOnMaxResPixel(j, row_no);
                        // std::cout << "...turn on: " << j << "," << row_no << std::endl;
                    }
                }
                
                auto    local_open  = col.start_interior;
                auto    local_close = col.end_interior;

                if (local_open != 0 || local_close != 0) {
                    mipmap.turnOnMaxResPixel(col_no, row_no);
                }
                
                sum += local_open - local_close;
                
                previous_column = col_no;
                
                first = false;
                
            }

            
            
#if 0
            int64_t      open_count    =  0;

            bool         has_active_col_no = false;
            std::size_t  active_col_no = 0;

//            int boundary_hit    { 0 }; // boundary hit
//            int start_interior  { 0 }; // start interior marker
//            int end_interior    { 0 }; // end interior marker
            for (auto &col: row->entries) {
                
                std::size_t col_no = (std::size_t) col.column;
                
                auto    local_open  = col.start_interior;
                auto    local_close = col.end_interior;
                int64_t local_delta = local_open - local_close;
                
                auto new_open_count = open_count + local_delta;

                if (new_open_count < 0) {
                    throw std::runtime_error("cannot close more than open at anytime");
                }
                else if (local_open > 0 && open_count == 0 && local_delta == 0) {
                    mipmap.turnOnMaxResPixel(col_no, row_no);
                }
                else if (local_close > 0 && new_open_count == 0 && has_active_col_no) {
                    // end of "on" interval from active_col_no
                    for (auto j=active_col_no;j<=col_no;++j) {
                        mipmap.turnOnMaxResPixel(j, row_no);
                        // std::cout << "...turn on: " << j << "," << row_no << std::endl;
                    }
                    has_active_col_no = false;
                }
                else if (local_delta > 0 && !has_active_col_no) {
                    active_col_no     =  col_no;
                    has_active_col_no = true;
                }
                open_count = new_open_count;
            }
#endif

            
        }
        
        // prepare labeled_tree: complication is that it might
        // have two columns or two rows (we might need to break
        // into pieces)
        
        
        struct Item {
            Item() = default;
            Item(int x, int y, int level, maps::Tile tile):
                x(x), y(y), level(level), tile(tile)
            {}
            int x;
            int y;
            int level;
            maps::Tile tile;
        };
        
        //
        std::vector<Item> stack;
        
        
//        auto push = [&stack](
        
        // auto t00 = ::maps::Tile(t0.x.quantity, t0.y.quantity, current_level);
        if (x_blocks > 1 || y_blocks > 1) {
#ifdef LOG_COMPUTE_TREE_DECOMPOSITION
            std::cerr << prefix << "case multiple tiles" << std::endl;
#endif

            if (x_blocks > 1 && y_blocks > 1) {
                stack.push_back(Item(0,0,1,maps::Tile(t0.x.quantity,   t0.y.quantity,   current_level)));
                stack.push_back(Item(1,0,1,maps::Tile(t0.x.quantity+1, t0.y.quantity,   current_level)));
                stack.push_back(Item(1,1,1,maps::Tile(t0.x.quantity+1, t0.y.quantity+1, current_level)));
                stack.push_back(Item(0,1,1,maps::Tile(t0.x.quantity,   t0.y.quantity+1, current_level)));
            }
            else if (x_blocks > 1 && y_blocks == 1) {
                stack.push_back(Item(0,0,1,maps::Tile(t0.x.quantity,   t0.y.quantity,   current_level)));
                stack.push_back(Item(1,0,1,maps::Tile(t0.x.quantity+1, t0.y.quantity,   current_level)));
            }
            else if (x_blocks == 1 && y_blocks > 1) {
                stack.push_back(Item(0,0,1,maps::Tile(t0.x.quantity,   t0.y.quantity,   current_level)));
                stack.push_back(Item(0,1,1,maps::Tile(t0.x.quantity,   t0.y.quantity+1, current_level)));
            }
            //            stack.push_back(Item(0,0,1,::maps::Tile(t0.x.quantity, t0.y.quantity, current_level)));
            //            stack.push_back(Item(1,0,1,::maps::Tile(t1.x.quantity, t0.y.quantity, current_level)));
            //            stack.push_back(Item(1,1,1,::maps::Tile(t1.x.quantity, t1.y.quantity, current_level)));
            //            stack.push_back(Item(0,1,1,::maps::Tile(t0.x.quantity, t1.y.quantity, current_level)));
        }
        else {
#ifdef LOG_COMPUTE_TREE_DECOMPOSITION
            std::cerr << prefix << "case single tile" << std::endl;
#endif
            stack.push_back(Item(0,0,1,t0.refined((maps::SubTileLabel) 0)));
            stack.push_back(Item(1,0,1,t0.refined((maps::SubTileLabel) 1)));
            stack.push_back(Item(0,1,1,t0.refined((maps::SubTileLabel) 2)));
            stack.push_back(Item(1,1,1,t0.refined((maps::SubTileLabel) 3)));
        }
        
        

        
        
#ifdef LOG_COMPUTE_TREE_DECOMPOSITION
        mipmap.save("/tmp/mipmap_" + std::to_string(iteration_id));
        std::cerr << prefix << "tagging global cover tree... " << std::endl;
#endif
        while (stack.size()) {
            
            Item item = stack.back();
            stack.pop_back();
            engine.goTo(labeled_tree::Path(item.tile));

#ifdef LOG_COMPUTE_TREE_DECOMPOSITION
#if LOG_COMPUTE_TREE_DECOMPOSITION_VERBOSITY>0
            std::cerr << prefix << "processing grid_cell: " << item.tile << "  path: " << ::labeled_tree::Path(item.tile) << "  iteration: " << engine.iteration_tag << std::endl;
#endif
#endif

            
            std::string p = std::string(item.level*4, '.') ;
            
            auto state = mipmap.state(item.level, item.x, item.y);
            if (state == MipMap::FULL) {

#ifdef LOG_COMPUTE_TREE_DECOMPOSITION
#if LOG_COMPUTE_TREE_DECOMPOSITION_VERBOSITY>0
                std::cerr << prefix << p << "full: " << item.tile  << " iteration: " << engine.iteration_tag << std::endl;
#endif
#endif

                engine.consolidate();
            }
            else if (state == MipMap::PARTIAL) {
#ifdef LOG_COMPUTE_TREE_DECOMPOSITION
#if LOG_COMPUTE_TREE_DECOMPOSITION_VERBOSITY>0
                std::cerr << prefix << p << "partial: " << item.tile << " iteration: " << engine.iteration_tag << std::endl;
#endif
#endif
                auto x = item.x << 1;
                auto y = item.y << 1;

                stack.push_back(Item(x,   y,   item.level+1, item.tile.refined((maps::SubTileLabel) 0)));
                stack.push_back(Item(x+1, y,   item.level+1, item.tile.refined((maps::SubTileLabel) 1)));
                stack.push_back(Item(x,   y+1, item.level+1, item.tile.refined((maps::SubTileLabel) 2)));
                stack.push_back(Item(x+1, y+1, item.level+1, item.tile.refined((maps::SubTileLabel) 3)));
            }
            else {
//               std::cout << prefix << p << "empty: " << item.tile << std::endl;
            }
        }

        
        
#ifdef LOG_COMPUTE_TREE_DECOMPOSITION
#if LOG_COMPUTE_TREE_DECOMPOSITION_VERBOSITY>1
        {
            std::cerr << "-------------------------------------------" << std::endl;
            std::cerr << "TREE_DECOMPOSITION" << std::endl;
            std::stringstream ss;
            ::labeled_tree::text(ss, *engine.root.get());
            std::cerr << ss.str() << std::endl;
            std::cerr << "-------------------------------------------" << std::endl;
        }
#endif
#endif
        
        
        // assume no recursion for now
        if (recursion_level + 1 < levels.size()) {
            
            // find all the cells that need

            CellMap cellmap;
            std::vector<LLPolygon> ll_polygons(grid_polygons.size());
            int i=-1;
            for (auto &grid_polygon: grid_polygons) {
                ++i;
                ll_polygons[i] = std::move(intersect_grid(grid_polygon));
                cellmap.markCellsThatIntersectPolygon(ll_polygons[i]);
            }
            
#ifdef LOG_COMPUTE_TREE_DECOMPOSITION
            std::cout << prefix <<  "cellmap size: " << cellmap.map.size() << std::endl;
#endif
            
            for (auto it: cellmap.map) {

                //---- BEGIN -----
                // TODO: clean this segment to split parent if necessary
                //
                // tiles that we are going to refine, need to be
                // separated from their parent tiles
                auto cell_id = it.first;
                auto d = current_depth;
                if (x_blocks > 1 || y_blocks > 1)
                    --d;
                auto tile_id = t0;
                for (int i=0;i<d;++i)
                    tile_id.refine(maps::LOWER_LEFT);
                tile_id.x.quantity += cell_id.x;
                tile_id.y.quantity += cell_id.y;

                
#ifdef LOG_COMPUTE_TREE_DECOMPOSITION
#if LOG_COMPUTE_TREE_DECOMPOSITION_VERBOSITY>0
                static int count_refinements = 0;

                std::cerr << prefix << "Refining tile " << tile_id << "    refinement index: " << count_refinements++ << std::endl;
                {
//                    std::cerr << *engine.root.get() << std::endl;
                    std::cerr << "-------------------------------------------" << std::endl;
                    std::cerr << "BEFORE SPLIT" << std::endl;
                    std::stringstream ss;
                    ::labeled_tree::text(ss, *engine.root.get());
                    std::cerr << ss.str() << std::endl;
                    std::cerr << "-------------------------------------------" << std::endl;
                }
#endif
#endif
                
                // make sure tile_id is an individual tile
                engine.goTo(tile_id);
                engine.current_node->split(); // split is an operation that returns true if parent node is
                                              // fixed and has no children. if it is not fixed, nothing is done

#ifdef LOG_COMPUTE_TREE_DECOMPOSITION
#if LOG_COMPUTE_TREE_DECOMPOSITION_VERBOSITY>0
                {
                    std::cerr << "-------------------------------------------" << std::endl;
                    std::cerr << "AFTER SPLIT" << std::endl;
                    std::stringstream ss;
                    ::labeled_tree::text(ss, *engine.root.get());
                    std::cerr << ss.str() << std::endl;
                    std::cerr << "-------------------------------------------" << std::endl;
                }
#endif
#endif
                //---- END -----
                
                
#ifdef LOG_COMPUTE_TREE_DECOMPOSITION
                std::cerr << prefix << "cell: " << it.first << std::endl;
#endif
                // int count = 0;
                auto cell_intersection_grid_polygons = it.second->polygons();
                
                RealBox bbox;
                std::vector<Polygon> cell_intersection_mercator_polygons;
                std::for_each(cell_intersection_grid_polygons.begin(),
                              cell_intersection_grid_polygons.end(),
                              [&cell_intersection_mercator_polygons, &inverse_transform,&bbox](const Polygon& p) {
                                  cell_intersection_mercator_polygons.push_back(Polygon(p,inverse_transform));
                                  bbox.add(cell_intersection_mercator_polygons.back().getBoundingBox());
                              });
                
                

#ifdef LOG_COMPUTE_TREE_DECOMPOSITION
                std::cerr << prefix << "recurse on grid polygons:" << std::endl;
                for (auto &p: cell_intersection_grid_polygons) {
                    std::cerr << prefix << std::string(4,'.') << p << std::endl;
                    // std::cerr << prefix << std::string(4,".") << "bounds: " << bbox.pmin << ", " << bbox.pmax << std::endl;
                }
#endif
                process(cell_intersection_mercator_polygons, bbox, recursion_level + 1);

#ifdef LOG_COMPUTE_TREE_DECOMPOSITION
#if LOG_COMPUTE_TREE_DECOMPOSITION_VERBOSITY>0
                {
                    std::cerr << "-------------------------------------------" << std::endl;
                    std::cerr << "AFTER REFINEMENT" << std::endl;
                    // std::cerr << *engine.root.get() << std::endl;
                    std::stringstream ss;
                    ::labeled_tree::text(ss, *engine.root.get());
                    std::cerr << ss.str() << std::endl;
                    std::cerr << "-------------------------------------------" << std::endl;
                }
#endif
#endif
            }
            
            // throw std::runtime_error("recursion is not ready yet");
        }
    };
    
    process(mercator_polygons, bbox, 0); //
    
    engine.goTo(labeled_tree::Path());
    auto labeled_tree = engine.root.get();
    labeled_tree->optimize();
    engine.root.release(); // engine does not own labeled tree anymore
    
#ifdef LOG_COMPUTE_TREE_DECOMPOSITION
    std::stringstream ss;
    ::labeled_tree::json(ss, *labeled_tree);
    std::cerr << ss.str() << std::endl;
    std::cerr << *labeled_tree << std::endl;
#endif
    
    return labeled_tree;

}

} // polycover namespace
