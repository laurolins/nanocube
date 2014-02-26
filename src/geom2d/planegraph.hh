#pragma once

#include <memory>
#include <vector>

#include "point.hh"
#include "polygon.hh"

//------------------------------------------------------------------------------
// Plane Graph
//------------------------------------------------------------------------------

namespace geom2d {

namespace planegraph {

static const bool SOURCE = true;
static const bool TARGET = false;

using  Label = int;

struct Vertex;
struct Face;
struct Edge;

struct ThickVertex; // ThickVertex is a vertex of the thick graph

using  Point   = geom2d::Point;
using  Polygon = geom2d::Polygon;

//-----------------------------------------------------------------------------
// Entity
//-----------------------------------------------------------------------------

struct Entity {
    Entity(Label label);

    Label        label;
    ThickVertex* entry_point { nullptr };
};

//-----------------------------------------------------------------------------
// Vertex
//-----------------------------------------------------------------------------

struct Vertex: public Entity {
    Vertex(Label label, Point coordinates);
    Point coordinates;
};

//-----------------------------------------------------------------------------
// Edge
//-----------------------------------------------------------------------------

struct Edge: public Entity {
    Edge(Label label);
};

//-----------------------------------------------------------------------------
// Face
//-----------------------------------------------------------------------------

struct Face: public Entity {
    Face(Label label);
};

//-----------------------------------------------------------------------------
// PlaneGraph
//-----------------------------------------------------------------------------

struct PlaneGraph {

    PlaneGraph(const Polygon &polygon);

private:

    auto newFace() -> Face&;
    auto newEdge() -> Edge&;
    auto newVertex(Point coordinates) -> Vertex&;
    auto newThickVertex(Vertex& vertex, Edge& edge, Face& face, bool source) -> ThickVertex&;

public:

    // assumes face zero is the external one
    auto findInteriorCommonFace(Vertex& v1, Vertex &v2) const -> Face&;

    auto subdivideFace(Face& face, Vertex& v1, Vertex& v2) -> Face&;
    auto subdivideInteriorFace(Vertex &v1, Vertex &v2) -> Face&;
    // auto subdivideFace(ThickVertex *v1, ThickVertex* v2)   -> Face*;

    auto getFace(size_t index) const -> Face&;
    auto getEdge(size_t index) const -> Edge&;
    auto getVertex(size_t index) const -> Vertex&;

    bool isConvex(Vertex& v, Face &f) const;
    bool isReflex(Vertex& v, Face &f) const;

    std::vector<std::unique_ptr<Face>>        faces;
    std::vector<std::unique_ptr<Vertex>>      vertices;
    std::vector<std::unique_ptr<Edge>>        edges;
    std::vector<std::unique_ptr<ThickVertex>> thick_vertices;

private:
    Label next_face_label   { 0 };
    Label next_edge_label   { 1 };
    Label next_vertex_label { 1 };
};


//-----------------------------------------------------------------------------
// ThickVertex
//-----------------------------------------------------------------------------

enum NeighborType { VERTEX=0, EDGE=1, FACE=2 };

struct ThickVertex {

public:

    ThickVertex(Vertex& vertex, Edge& edge, Face& face);

    auto operator[](NeighborType type) const -> ThickVertex*;


public: // static services

    static void connect(ThickVertex &v1, ThickVertex &v2, NeighborType type);

public:
    Vertex*      vertex       { nullptr };
    Edge*        edge         { nullptr };
    Face*        face         { nullptr };

    ThickVertex* neighbors[3] { nullptr, nullptr, nullptr };

    bool         source       { false }; // when traversing vertex is it a source?
};


//-----------------------------------------------------------------------------
// BigonIterator
//-----------------------------------------------------------------------------

struct BigonIterator {

    BigonIterator(ThickVertex *first, NeighborType t1, NeighborType t2);

    bool next();
    auto getCurrent() const -> ThickVertex*;

    ThickVertex* first;
    NeighborType types[2];
    int          parity  { 0 };
    ThickVertex* current { nullptr };
};


} // planegraph namespace

} // planegraph geom2d





//-----------------------------------------------------------------------------
// IO
//-----------------------------------------------------------------------------

namespace geom2d {
namespace planegraph {
namespace io {

std::ostream &operator<<(std::ostream& os, const Face& face);

} // io
} // planegraph
} // geom2d
