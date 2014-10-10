#pragma once

#include <set>
#include <random>
#include <iostream>
#include <stdexcept>
#include <functional>
#include <cmath>
#include <vector>
#include <iterator>
#include <memory>
#include <iomanip>
#include <sstream>
#include <map>

#include "infix_iterator.hh"
#include "labeled_tree.hh"
#include "mipmap.hh"

namespace polycover {

//------------------------------------------------------------------------------
// Vec2
//------------------------------------------------------------------------------

template <typename T>
struct Vec2 {
public:
    using coord_type = T;
public:
    Vec2() = default;

    Vec2(T x, T y): x(x), y(y)
    {}
    
    T max() {
        return std::max(x,y);
    }

    T min() {
        return std::min(x,y);
    }

    bool operator==(const Vec2& other) const {
        return (x == other.x && y == other.y);
    }

    bool operator!=(const Vec2& other) const {
        return (x != other.x || y != other.y);
    }

    bool operator<(const Vec2& other) const {
        return (x < other.x || (x == other.x && y < other.y));
    }

    T x;
    T y;
};

//------------------------------------------------------------------------------
// Box
//------------------------------------------------------------------------------

template <typename T>
struct Box {
public:
    Box() = default;
    Box(Vec2<T> pmin, Vec2<T> pmax);
    bool empty { true };
    void add(const Vec2<T> &p);
    void add(const Box<T> &b);
    T width() const;
    T height() const;
    Vec2<T> size() const;
public:
    Vec2<T> pmin;
    Vec2<T> pmax;
};

//------------------------------------------------------------------------------
// Specialized Vec2's and Boxes
//------------------------------------------------------------------------------

using RealPoint = Vec2<double>;
using RealVec = Vec2<double>;
using GridPoint = Vec2<int>;
using RealBox = Box<double>;
using GridBox = Box<int>;

//------------------------------------------------------------------------------
// Segment
//------------------------------------------------------------------------------

struct Segment {
    Segment() = default;
    Segment(RealPoint p1, RealPoint p2);
    RealPoint p1;
    RealPoint p2;
};

//------------------------------------------------------------------------------
// BoundaryType
//------------------------------------------------------------------------------

enum BoundaryType { LEFT, RIGHT, HORIZONTAL };

BoundaryType boundary(const Segment &s);

//------------------------------------------------------------------------------
// Chain
//------------------------------------------------------------------------------

struct Chain {
public:
    Chain() = default;
    Chain(int index, BoundaryType type);
    void grow(int n=1);
    
public:
    BoundaryType type;
    int index  { -1 };  // origin of first segment in the chain;
    int length { -1 }; // number of segments (cyclic)
};

//------------------------------------------------------------------------------
// ChainBoundary
//------------------------------------------------------------------------------

struct ChainBoundary {
public:
    struct Iterator {
    public:
        Iterator(const ChainBoundary& boundary);
        bool next();
        void get(int &x, int &y);
    public:
        const ChainBoundary& boundary;
        int current_y { -1 };
    };
    
public:
    enum Mode { MIN, MAX };
    static const int UNDEFINED = 0x7FFFFFF;
public:
    void reset(Mode mode);
    void insert(int x, int y);
    Iterator iterator();
public:
    Mode mode { MIN };
    int min_row { -1 };
    std::vector<int> rows;
};

//------------------------------------------------------------------------------
// Polygon
//------------------------------------------------------------------------------

struct Polygon {
public:
    
    Polygon() = default;
    Polygon(const std::vector<RealPoint> &points);
    Polygon(const Polygon &other, std::function<RealPoint(const RealPoint&)> transform);
    void add(RealPoint p);
    RealBox getBoundingBox() const; // as a segment: p1 is minx, miny and p2 is maxx, maxy
    RealPoint  getVertex(int i) const;
    Segment getSegment(int i) const;
    
    std::vector<Chain> chains() const;
    
    void normalizeToBox(RealBox box);
    
    void makeItCW();
    double area();

public:
    
    std::vector<RealPoint> points;

};

//------------------------------------------------------------------------------
// LLVertex
//------------------------------------------------------------------------------

struct LLVertex {
public:
    LLVertex() = default;
    LLVertex(RealPoint p);
public:
    RealPoint point;
    LLVertex *next { nullptr };
    LLVertex *prev { nullptr };
};

//------------------------------------------------------------------------------
// LLPolygon: Linked List Polygon
//------------------------------------------------------------------------------

struct LLPolygon {
public:
    LLPolygon() = default;
    LLPolygon(const std::vector<RealPoint> &points);
    
    LLPolygon(const LLPolygon& other) = delete;
    LLPolygon& operator=(const LLPolygon& other) = delete;

    LLPolygon(LLPolygon&& other);
    LLPolygon& operator=(LLPolygon&& other);

    ~LLPolygon();
    
    LLVertex* insert(const RealPoint &v, LLVertex* reference_vertex=nullptr);
public:
    LLVertex *entry_vertex { nullptr }; // first vertex
};

//------------------------------------------------------------------------------
// ColumnEntry
//------------------------------------------------------------------------------

enum GridCellEvent { BOUNDARY_HIT, START_INTERIOR, END_INTERIOR };

struct ColumnEntry {
public:
    ColumnEntry() = default;
    ColumnEntry(int column);
public:
    void update(GridCellEvent cell_event);
public:
    int column {-1};
    int boundary_hit    { 0 }; // boundary hit
    int start_interior  { 0 }; // start interior marker
    int end_interior    { 0 }; // end interior marker
};

//------------------------------------------------------------------------------
// Row
//------------------------------------------------------------------------------

struct Row {
public:
    Row(int row);
    void update(int column, GridCellEvent cell_event);
    ColumnEntry& getColumn(int column);
public: 
    int row;
    std::vector<ColumnEntry> entries;
};

//------------------------------------------------------------------------------
// Grid
//------------------------------------------------------------------------------

struct Grid {
public:
    Grid(int width, int height);
    Row& getRow(int row);
    void update(const GridPoint &p, GridCellEvent cell_event);
public:
    int width;
    int height;
    std::vector<std::unique_ptr<Row>> rows;
};

//------------------------------------------------------------------------------
// Reporter
//------------------------------------------------------------------------------

struct Reporter {

    void polygon(const Polygon& polygon) {
        polygons.push_back(polygon);
    }

    void grid_intersection_point(const RealPoint& p) {
        grid_intersection_points.push_back(p);
    }

    void grid_cell(const GridPoint& p) {
        grid_cells.push_back(p);
    }
    
    void start_interior(const GridPoint& p) {
        start_interior_cells.push_back(p);
    }

    void end_interior(const GridPoint& p) {
        end_interior_cells.push_back(p);
    }

    void segment(const Segment& segment) {
        segments.push_back(segment);
    }

    void decomposition(const std::string& st) {
        decompositions.push_back(st);
    }

    void outgoing_unit_segment(const Segment& segment) {
        outgoing_unit_segments.push_back(segment);
    }

    std::vector<Segment>   segments;
    std::vector<Polygon>   polygons;
    std::vector<GridPoint> grid_cells;
    std::vector<RealPoint> grid_intersection_points;
    std::vector<Segment>   outgoing_unit_segments;
    std::vector<GridPoint> start_interior_cells;
    std::vector<GridPoint> end_interior_cells;
    std::vector<std::string> decompositions;
    
};

//------------------------------------------------------------------------------
// Template Functions
//------------------------------------------------------------------------------

template <typename T>
inline constexpr T absolute(const T &v) {
    return (v < 0) ? -v : v;
}

inline constexpr int manhattan_distance(const GridPoint &p1, const GridPoint &p2) {
    return absolute(p1.x - p2.x) + absolute(p1.y - p2.y);
}

template <typename T>
inline constexpr T dot(const Vec2<T>& v1, const Vec2<T> &v2) {
    return v1.x * v2.x + v1.y * v2.y;
}

template <typename T>
inline constexpr Vec2<T> operator-(const Vec2<T>& v) {
    return Vec2<T>(-v.x, -v.y);
}

template <typename T>
inline constexpr Vec2<T> operator+(const Vec2<T>& v1, const Vec2<T> &v2) {
    return Vec2<T>(v1.x + v2.x, v1.y + v2.y);
}

template <typename T>
inline constexpr Vec2<T> operator-(const Vec2<T>& v1, const Vec2<T> &v2) {
    return Vec2<T>(v1.x - v2.x, v1.y - v2.y);
}

template <typename T>
inline constexpr Vec2<T> operator*(const Vec2<T>& u, const Vec2<T>& v) {
    return Vec2<T>(u.x * v.x, u.y * v.y);
}

template <typename T>
inline constexpr Vec2<T> operator/(const Vec2<T>& u, const Vec2<T>& v) {
    return Vec2<T>(u.x / v.x, u.y / v.y);
}

template <typename T>
inline constexpr Vec2<T> operator*(const Vec2<T>& v, const T& scalar) {
    return Vec2<T>(v.x * scalar, v.y * scalar);
}

template <typename T>
inline constexpr Vec2<T> operator*(const T& scalar, const Vec2<T>& v) {
    return Vec2<T>(v.x * scalar, v.y * scalar);
}

template <typename T>
inline constexpr Vec2<T> operator/(const Vec2<T>& v, const T& scalar) {
    return Vec2<T>(v.x / scalar, v.y / scalar);
}

template <typename T>
inline constexpr Vec2<T> operator/(const T& scalar, const Vec2<T>& v) {
    return Vec2<T>(v.x / scalar, v.y / scalar);
}


template <typename T1, typename T2>
inline constexpr Vec2<T2> cast(const Vec2<T1>& v, std::function< T2 (const T1&) > f) {
    return Vec2<T2>( f(v.x), f(v.y) );
}

//------------------------------------------------------------------------------
// Non Template Functions
//------------------------------------------------------------------------------

inline double length(const Vec2<double> &v) {
    return sqrt(v.x * v.x + v.y * v.y);
}

inline double length(const Vec2<int> &v) {
    return sqrt((double)(v.x * v.x + v.y * v.y));
}

inline Vec2<double> normalized(const Vec2<double> &v) {
    auto len = length(v);
    return Vec2<double>(v.x/len, v.y/len);
}

inline Vec2<double> perpendicular(const Vec2<double> &v) {
    // auto len = length(v);
    return Vec2<double>(-v.y, v.x);
}

inline Vec2<double> rotate(const Vec2<double> &v, double theta) {
    // auto len = length(v);
    auto sin_theta = sin(theta);
    auto cos_theta = cos(theta);
    return Vec2<double>( v.x * cos_theta - v.y * sin_theta,
                         v.x * sin_theta + v.y * cos_theta );
}


inline Vec2<double> fraction(const Vec2<double> &v) {
    return Vec2<double>(v.x - floor(v.x), v.y - floor(v.y));
}

inline GridPoint toGridPoint(const RealPoint &v) {
    return GridPoint((int)floor(v.x), (int) floor(v.y));
}

inline constexpr double sgn(double x) {
    return (x < 0.0 ? -1.0 : (x > 0.0 ? 1.0 : 0.0));
}

inline GridPoint sgn(const RealPoint &v) {
    return GridPoint((int)sgn(v.x), (int) sgn(v.y));
}

inline int toInt(const double &v) {
    return (int) floor(v);
};


template <typename T>
inline constexpr bool ccw(const Vec2<T> &p1, const Vec2<T> &p2, const Vec2<T> &p3) {
    return (p3.y-p1.y) * (p2.x-p1.x) > (p2.y-p1.y) * (p3.x-p1.x);
}

template <typename T>
inline constexpr bool intersect(const Vec2<T> &p1, const Vec2<T> &p2, const Vec2<T> &p3, const Vec2<T> &p4) {
    return ccw(p1,p3,p4) != ccw(p2,p3,p4) && ccw(p1,p2,p3) != ccw(p1,p2,p4);
}

//------------------------------------------------------------------------------
// IO
//------------------------------------------------------------------------------

template <typename T>
std::ostream& operator<<(std::ostream& os, const Vec2<T>& v) {
    os << "[" << std::setprecision(18) << v.x << "," << std::setprecision(18) << v.y << "]"; // compatible with json
    return os;
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const Box<T>& b) {
    os << b.pmin << "x" << b.pmax; // compatible with json
    return os;
}


std::ostream& operator<<(std::ostream& os, const Polygon& poly);

std::ostream& operator<<(std::ostream& os, const Segment& segment);

std::ostream& operator<<(std::ostream& os, const Grid& grid);

std::ostream &operator<<(std::ostream& os, const Reporter &reporter);


//------------------------------------------------------------------------------
// Box
//------------------------------------------------------------------------------

template <typename T>
Box<T>::Box(Vec2<T> pmin, Vec2<T> pmax):
    pmin(pmin),
    pmax(pmax)
{}

template <typename T>
void Box<T>::add(const Vec2<T> &p) {
    if (empty) {
        pmin = pmax = p;
        empty = false;
    }
    else {
        pmin.x = std::min(p.x, pmin.x);
        pmax.x = std::max(p.x, pmax.x);
        pmin.y = std::min(p.y, pmin.y);
        pmax.y = std::max(p.y, pmax.y);
    }
}

template <typename T>
void Box<T>::add(const Box<T> &b) {
    if (!b.empty) {
        this->add(b.pmin);
        this->add(b.pmax);
    }
}

template <typename T>
T Box<T>::width() const {
    if (empty)
        throw std::runtime_error("ooops");
    return pmax.x - pmin.x;
}

template <typename T>
T Box<T>::height() const {
    if (empty)
        throw std::runtime_error("ooops");
    return pmax.y - pmin.y;
}

template <typename T>
Vec2<T> Box<T>::size() const {
    if (empty)
        throw std::runtime_error("ooops");
    return pmax - pmin;
}

//------------------------------------------------------------------------------
// GridIntersectSegment
//------------------------------------------------------------------------------

struct GridIntersectSegment2 {
public:
    GridIntersectSegment2(RealPoint p1, RealPoint p2);
    bool next(RealPoint& next_point);
public:
    RealPoint p1;
    RealPoint p2;
    RealPoint current_point;
    RealPoint delta;
    bool      first { true };
};


//
// Intersect Polygon with Grid
//
LLPolygon intersect_grid(const Polygon& polygon);


//------------------------------------------------------------------------------
// CellVertex
//------------------------------------------------------------------------------

struct CellVertex {
public:
    enum Type { ENTRY, EXIT, CORNER, EXIT_ENTRY, INTERNAL };
    
public:
    CellVertex() = default;
    CellVertex(LLVertex* v, Type type, int index, int component);
    
    void tag();
    void untag();
    bool isTagged() const;
    
public:
    LLVertex* vertex { nullptr };
    Type      type;
    int       index;
    int       index_cw;
    int       component; 
    bool      tagged { false };
};

//------------------------------------------------------------------------------
// Cell
//------------------------------------------------------------------------------

// from a cell we should be able to spit one or more polygons
struct Cell {
    
    // a series of ENTER or EXIT vertices from a single polygon
    using CellComponent = std::vector<CellVertex*>;
    
    Cell() = default;
    Cell(GridPoint id);
    
    ~Cell();
    
    // TODO: erase release memory of these CellVertex
    
    void add(LLVertex* v, CellVertex::Type type, int component_id);
    std::vector<Polygon> polygons();
    
    GridPoint id; // cell id
    
    int latest_component_id { -1 };
    
    std::vector<CellComponent> components;
    std::vector<CellVertex*> internal_vertices;
};


//-----------------------------------------------------------------
// VertexType
//-----------------------------------------------------------------

struct VertexType {
public:
    enum Tag    { INTERNAL=0, HORIZONTAL_EDGE=1, VERTICAL_EDGE=2, CORNER=3, UNDEFINED=4 };
    enum Edge   { NO_EDGE,     LEFT_EDGE, TOP_EDGE, RIGHT_EDGE, BOTTOM_EDGE };
    enum CellID { BASE_CELL=0, LEFT_CELL=1, BELOW_CELL=2, LEFT_BELOW_CELL=3, NO_CELL=4 };
    enum Case   { DONT_ADD, EXIT_ENTER_VERTEX, ENTER_VERTEX, EXIT_VERTEX, INTERNAL_VERTEX };
public:
    VertexType(LLVertex *u);
    Case getCase(CellID) const;
    GridPoint getCellAddress(CellID cell_id) const;
public:
    LLVertex *u;
    Tag  tag { UNDEFINED };
    Edge edge { NO_EDGE };
    Case cell_cases[4] { DONT_ADD, DONT_ADD, DONT_ADD, DONT_ADD };
};

std::ostream& operator<<(std::ostream& os, const VertexType::Tag& tag);
std::ostream& operator<<(std::ostream& os, const VertexType::Edge& edge);
std::ostream& operator<<(std::ostream& os, const VertexType::CellID& cell_id);
std::ostream& operator<<(std::ostream& os, const VertexType::Case& c);
std::ostream& operator<<(std::ostream& os, const VertexType& v);

//------------------------------------------------------------------------------
// CellMap
//------------------------------------------------------------------------------

struct CellMap {
public:
    CellMap() = default;
    Cell& operator()(const GridPoint &p);
    void markCellsThatIntersectPolygon(LLPolygon &llpoly);
public:
    int current_component { 0 };
    void add(GridPoint p, LLVertex* v, CellVertex::Type type);
    std::map<GridPoint, Cell*> map;
};

//------------------------------------------------------------------------------
// Important Functions Decl.
//------------------------------------------------------------------------------

template <typename Reporter>
std::vector<GridPoint> raster_segment(RealPoint p1, RealPoint p2, Reporter *reporter);

std::vector<GridPoint> raster_segment(RealPoint p1, RealPoint p2);

template <typename Reporter>
Grid raster_polygon(const Polygon& polygon, Reporter *reporter= nullptr);

template <typename Reporter>
std::vector<Segment> find_outgoing_vectors(const Polygon& polygon, Reporter* reporter=nullptr);

//------------------------------------------------------------------------------
// Important Functions Definition
//------------------------------------------------------------------------------

template <typename Reporter>
std::vector<GridPoint> raster_segment(RealPoint p1, RealPoint p2, Reporter *reporter) {

    if (reporter)
        reporter->segment({p1, p2});

    std::function<int(const double&)> toInt = [] (const double &v) -> int {
        // std::cout << "toInt: " << std::setprecision(18) << v << " converted to " << (int) floor(v) << std::endl;
        return (int) floor(v);
    };

    RealVec delta  = normalized(p2 - p1);
    // RealVec delta_sign = RealVec(sgn(delta.x), sgn(delta.y));

    // GridPoint sign_i = cast(sign, toInt); 
    // std::cout << "p1:    " << p1 << std::endl;
    // std::cout << "p2:    " << p2 << std::endl;
    // std::cout << "sign:  " << sign << std::endl;
    // std::cout << "delta: " << delta << std::endl;
    // std::cout << "sign:  " << sign << std::endl;

    // if (!(delta_sign.x && sign.y))
    //     throw std::runtime_error("raster_segment cannot handle horizontal or vertical segments");

    bool horizontal = (delta.y == 0.0);
    bool vertical   = (delta.x == 0.0);

    std::vector<GridPoint> result;

    if (horizontal && vertical) {
        result.push_back(cast(p1, toInt));
    }
    else if (horizontal) {
        auto p1_grid = cast(p1, toInt);
        auto p2_grid = cast(p2, toInt);
        const auto y = p1.y;
        for (auto x=p1_grid.x; x<=p2_grid.x; ++x) {
            result.push_back(GridPoint(x,y));
        }
    }
    else if (vertical) {
        auto p1_grid = cast(p1, toInt);
        auto p2_grid = cast(p2, toInt);
        const auto x = p1.x;
        for (auto y=p1_grid.y; y<=p2_grid.y; ++y) {
            result.push_back(GridPoint(x,y));
        }
    }
    else { // general case

        auto iterate = [](const RealVec &p, const RealVec &delta) -> RealVec {

            auto next_coord = [](double v, double dv) {
                auto fraction = v - floor(v);
                if (fraction > 0.0)
                    return floor(v) + ((dv > 0.0) ? 1.0 : 0.0);
                else
                    return floor(v) + sgn(dv);
            };

            auto next_x = next_coord(p.x, delta.x);
            auto next_y = next_coord(p.y, delta.y);

            auto alpha_x = (next_x - p.x)/delta.x;
            auto alpha_y = (next_y - p.y)/delta.y;

            auto alpha = std::min(alpha_x, alpha_y);

            return p + alpha * delta;

        };

        auto truncate = [](const RealVec &p, const RealVec &delta) -> RealVec {

            auto p_frac = fraction(p);

            if (p_frac.x * p_frac.y == 0.0)
                return p;

            // std::cout << "p_frac: " << p_frac << std::endl;

            auto alpha_x = -p_frac.x / delta.x; // * (delta.x > 0.0 ? -1.0 : 1.0);
            auto alpha_y = -p_frac.y / delta.y; // * (delta.y > 0.0 ? -1.0 : 1.0);

            // std::cout << "alpha_x: " << alpha_x << std::endl;
            // std::cout << "alpha_y: " << alpha_y << std::endl;

            double alpha = (fabs(alpha_x) < fabs(alpha_y)) ? alpha_x : alpha_y;

            // p.x + delta.x * (p_frac.x / delta.x)      if delta.x < 0
            // p.x + delta.x * (-p_frac.x / delta.x)     if delta.x > 0

            // std::cout << "sim alpha_x: " << p + delta * alpha_x << std::endl;
            // std::cout << "sim alpha_y: " << p + delta * alpha_y << std::endl;

            return p + delta * alpha;
        };


        auto p1_adj = truncate(p1, delta);
        auto p2_adj = truncate(p2, delta);
        // auto p2_grid   = cast(p2_adj, toInt);

        // make it equal the previous point
        GridPoint p_prev_grid = cast(p1_adj, toInt);
    
        auto p = p1_adj;
        while (true) {

            auto p_grid = cast(p,toInt);

            if (reporter) {
                reporter->grid_intersection_point(p);
            }

            // std::cout << "Real Point: " << p << "  Integer Point: " << p_grid << std::endl;

            if (!result.size() || p_grid != result.back()) {

                // crosses grid
                // if (manhattan_distance(p_prev_grid, p_grid) > 1) {
                // }

                result.push_back(p_grid);

                if (reporter) {
                    reporter->grid_cell(p_grid);
                }


            }

            if (length(p - p2_adj) < 1.0e-8)
                break;

            p_prev_grid = p_grid;
                
            // if (length(p-p2_adj) < 1e-10)
            //     break;
            p = iterate(p, delta);
        } // end while

    }
    return result;
}



std::ostream& operator<<(std::ostream& os, const BoundaryType& btype);

std::ostream& operator<<(std::ostream& os, const Chain& chain);

template <typename Reporter>
Grid raster_polygon(const Polygon& polygon, Reporter *reporter) {

    
    auto bbox = polygon.getBoundingBox();
    
    if (bbox.pmin.x < 0.0 || bbox.pmin.y < 0.0)
        throw std::runtime_error("oooops");
    
    // find polygon bounds
    int rows    = (int) ceil(bbox.pmax.y) + 1;
    int columns = (int) ceil(bbox.pmax.x) + 1;
    Grid grid(columns,rows);
    
#if 1
    return grid;

#else
    
    // identify chains
    int first_chain = -1;
    std::vector<Chain> chains;
    for (int i=0;i<(int)polygon.vertices.size();++i) {
        Segment curr_segment = polygon.getSegment(i);
        Segment prev_segment = polygon.getSegment(i-1);
        
        auto curr_boundary = boundary(curr_segment); // is it left boundary or top boundary?
        
        if (curr_boundary != boundary(prev_segment)) {
            chains.push_back(Chain(i,curr_boundary));
            if (chains.size() == 1) {
                first_chain = i;
            }
        }
        else if (chains.size()){
            chains.back().grow();
        }
    }
    chains.back().grow(first_chain);
    
//    std::cout << polygon << std::endl;
//    std::cout << "[";
//    std::copy(chains.begin(), chains.end(), infix_ostream_iterator<Chain>(std::cout, ","));
//    std::cout << "]" << std::endl;
//    std::cout << std::endl;
   
    // toInt
    std::function<int(const double&)> toInt = [] (const double &v) -> int {
        // std::cout << "toInt: " << std::setprecision(18) << v << " converted to " << (int) floor(v) << std::endl;
        return (int) floor(v);
    };
    
    
    // rasterize chain
    auto rasterize_chain = [&] (const Chain &chain) {
        
//        std::string prefix(4,'.');
//        std::cerr << prefix << "processing chain " << chain << std::endl;
        
        bool      empty = true;
        GridPoint last_boundary_cell;
        GridPoint current_limit_cell;
        
        for (int s=0;s<chain.length;++s) {
            
            Segment segment = polygon.getSegment(chain.index + s);
            
            auto p1 = segment.v1.point;
            auto p2 = segment.v2.point;
            
            if (reporter)
                reporter->segment({p1, p2});
            
            // add_cell
            auto add_cell = [&] (const GridPoint &p) {
                if (empty) {
                    empty = false;
                    
                    grid.update(p, BOUNDARY_HIT);
                    if (reporter)
                        reporter->grid_cell(p);

                    last_boundary_cell = p;
                    current_limit_cell = p;
                }
                else if (last_boundary_cell != p) {
                    grid.update(p, BOUNDARY_HIT);
                    if (reporter)
                        reporter->grid_cell(p);
                    last_boundary_cell = p;

                    if (chain.type == LEFT) { // LEFT_BOUNDARY
                        if (current_limit_cell.y != p.y) {
                            grid.update(current_limit_cell, START_INTERIOR);
                            if (reporter)
                                reporter->start_interior(current_limit_cell);
                            
                            current_limit_cell = p;
                        }
                        else if (current_limit_cell.x > p.x) {
                            current_limit_cell = p;
                        }
                    }
                    else { // RIGHT_BOUNDARY
                        if (current_limit_cell.y != p.y) {
                            grid.update(current_limit_cell, END_INTERIOR);
                            if (reporter)
                                reporter->end_interior(current_limit_cell);
                            
                            current_limit_cell = p;
                        }
                        else if (current_limit_cell.x < p.x) {
                            current_limit_cell = p;
                        }
                    }
                } // point is different
            };
            
            auto intersection_point = [&](RealPoint p) {
                if (reporter)
                    reporter->grid_intersection_point(p);
            };
            
            GridIntersectSegment intersection_engine(p1,p2);
            add_cell(cast(p1,toInt));

            RealVec p;
            while (intersection_engine.next(p)) {
                add_cell(cast(p,toInt));
            }
            
            add_cell(cast(p2,toInt));
            
            
        } // end chain

        
        // add last limit cell of chain
        if (chain.type == LEFT) { // LEFT_BOUNDARY
            grid.update(current_limit_cell, START_INTERIOR);
            if (reporter)
                reporter->start_interior(current_limit_cell);
        }
        else { // RIGHT_BOUNDARY
            grid.update(current_limit_cell, END_INTERIOR);
            if (reporter)
                reporter->end_interior(current_limit_cell);
        }
        
       
        // return result;

    };
    

    // rasterize chains
    for (auto &chain: chains) {
        rasterize_chain(chain);
    }
    
    return grid;
#endif
    
}



//------------------------------------------------------------------------------
// raster_polygon2
//------------------------------------------------------------------------------

#define xLOG_RASTER_POLYGON_2

template <typename Reporter>
Grid raster_polygon2(const std::vector<Polygon> &polygons, Reporter *reporter) {
    
    RealBox bbox;
    for (auto &p: polygons) {
        bbox.add(p.getBoundingBox());
    }
    
    if (bbox.pmin.x < 0.0 || bbox.pmin.y < 0.0)
        throw std::runtime_error("oooops");
    
    // find polygon bounds
    int rows    = (int) ceil(bbox.pmax.y) + 1;
    int columns = (int) ceil(bbox.pmax.x) + 1;
    Grid grid(columns,rows);
    
    // every arc in the boundary of an (clockwise-oriented) simple polygon
    // induces an interior point. With this function we want to identify
    // which quadrant
    auto quadrant_of_interior_point = [](Vec2<int> reference_point, Vec2<int> direction) {
        RealPoint p    { 0.5 * reference_point.x, 0.5 * reference_point.y };
        RealPoint dir  { (double) direction.x, (double) direction.y };
        RealPoint right_quadrant_point = p + rotate(0.25 * normalized(dir), -22.5/180.0 * M_PI);
        auto quadrant = GridPoint { (int) floor(right_quadrant_point.x), (int) floor(right_quadrant_point.y)};
        return quadrant; // there is an interior point in the
    };

    for (auto &polygon: polygons) {
        
        ChainBoundary chain_boundary;
        for (auto &chain: polygon.chains()) {
            
#ifdef LOG_RASTER_POLYGON_2
            std::cout << std::string(4,'.') << "processing chain " << chain << std::endl;
#endif
            
            chain_boundary.reset(chain.type == LEFT ? ChainBoundary::MIN : ChainBoundary::MAX);
            
            // add all points in the chain
            // auto n = polygon.vertices.size();
            
            auto begin = chain.index;
            auto end   = begin + chain.length;
            for (auto i=begin;i<end;++i) {
                
                // don't start_interior and end_interior from horizontal segments
                bool horizontal_segment = boundary(polygon.getSegment(i)) == HORIZONTAL;
                
                auto v      = polygon.getVertex(i); // cyclic get vertex
                auto v_next = polygon.getVertex(i+1);
                
                if (v == v_next) // TODO: understand why this could happen when simple polygons are given
                    continue;    //       it is happening with the us_states.tt2 example
                
                auto out_vector = v_next - v;
                // auto in_vector  = v_prev.point - v.point;
                
                GridPoint out_direction = sgn(out_vector);
                // GridPoint in_direction  = sgn(in_vector);
                
                // bool p_flag   = v.poly_has_vertex;        // vertex is considered inside the polygon?
                // bool in_flag  = v_prev.poly_has_segment;  // segment from v to v_next is considered part of the polygon
                // bool out_flag = v.poly_has_segment;       // segment from v_prev to v is considered_part of the polygon
                
                // iterate through all grid intersections from p1 to p2
                GridIntersectSegment2 intersection_engine(v,v_next);
                
                // bool first = true;
                
#ifdef LOG_RASTER_POLYGON_2
                std::cout << std::string(8,'.') << "processing segment " << i << std::endl;
#endif
                
                RealVec p;
                while (intersection_engine.next(p)) {
                    
#ifdef LOG_RASTER_POLYGON_2
                    std::cout << std::string(12,'.') << "point " << p << std::endl;
#endif
                    if (reporter)
                        reporter->grid_intersection_point(p);
                    
                    if (horizontal_segment) // disconsider horizontal segments
                        continue;
                    
                    // check if
                    auto p_grid = toGridPoint(p);
                    
                    auto reference_point = sgn(fraction(p));
                    
                    auto quadrant = quadrant_of_interior_point(reference_point, out_direction);
                    
                    auto cell = p_grid + quadrant;
                    
                    //                grid.update(cell, BOUNDARY_HIT);
                    //
                    //                if (reporter)
                    //                    reporter->grid_cell(cell);
                    
                    chain_boundary.insert(cell.x, cell.y);
                    
                }
                
            } // end processing segments of a chain
            
            
            auto it = chain_boundary.iterator();
            auto flag = chain.type == LEFT ? START_INTERIOR : END_INTERIOR;
            while (it.next()) {
                int x,y;
                it.get(x,y);
                GridPoint cell(x,y);
                grid.update(cell, flag);
                
                if (reporter) {
                    if (flag == START_INTERIOR)
                        reporter->start_interior(cell);
                    else
                        reporter->end_interior(cell);
                }
            }
            
            
        } // end processing chain
        
    } // end processing polygons
    
    return grid;

}

template <typename Reporter>
std::vector<Segment> find_outgoing_vectors(const Polygon& polygon, Reporter* reporter) {

    // assuming clock-wise a rotation of 90 will get us to the exterior

    // find left-most point
    // auto it_left_most_point = polygon.points.cbegin();
    // for (auto it=polygon.points.cbegin() + 1; it!= polygon.points.cend(); ++it) {
    //     if (it_left_most_point->x > it->x ||
    //         it_left_most_point->x == it->x && left_most_point->y > it->y) {
    //         it_left_most_point = it;
    //     }
    // }

    // auto p_prev =  it_left_most_point == polygon.points.cbegin() ? polygon.points.back() : *(it_left_most_point-1);
    // auto p      = *it_left_most_point;
    // auto p_next =  it_left_most_point == polygon.points.cend()-1 ? polygon.points.front() : *(it_left_most_point+1);

    // // clock-wise and 90 degrees rotation should be all pointing out
    // auto delta = (p_next.y > p_prev.y) ? 1 : -1;


    std::vector<Segment> outgoing_unit_vectors;

    auto p0 = polygon.points.back();
    for (auto p1: polygon.points) {
        RealPoint base = (p0 + p1) * 0.5;
        RealPoint perp = perpendicular(normalized(p1 - p0));
        Segment segment(base, base + perp);
        outgoing_unit_vectors.push_back(segment);

        if (reporter)
            reporter->outgoing_unit_segment(segment);

        p0 = p1;
    }

    return outgoing_unit_vectors;
}


Polygon random_simple_grid_polygon(int width, int height, int num_sides, uint32_t seed=13);


struct TileCoverEngine {
public:
    TileCoverEngine(int max_level, int max_texture_level);
    labeled_tree::Node* computeCover(const std::vector<Polygon> &polygons);
public:
    std::unique_ptr<MipMap> mipmap_p;
    int max_level;
    int max_texture_level;
};

// ::labeled_tree::Node* compute_tree_decomposition(const std::vector<Polygon> &polygons, int max_level, int max_texture_level);

}

