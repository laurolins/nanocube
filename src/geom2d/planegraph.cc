#include <algorithm>

#include "base.hh"
#include "planegraph.hh"

//-----------------------------------------------------------------------------
// Entity
//-----------------------------------------------------------------------------

geom2d::planegraph::Entity::Entity(Label label):
    label { label }
{}

//-----------------------------------------------------------------------------
// Face
//-----------------------------------------------------------------------------

geom2d::planegraph::Face::Face(Label label):
    Entity{ label }
{}

//-----------------------------------------------------------------------------
// Edge
//-----------------------------------------------------------------------------

geom2d::planegraph::Edge::Edge(Label label):
    Entity{ label }
{}

//-----------------------------------------------------------------------------
// Vertex
//-----------------------------------------------------------------------------

geom2d::planegraph::Vertex::Vertex(Label label, Point coordinates):
    Entity      { label },
    coordinates { coordinates }
{}


//-----------------------------------------------------------------------------
// ThickVertex
//-----------------------------------------------------------------------------

geom2d::planegraph::ThickVertex::ThickVertex(Vertex &vertex, Edge &edge, Face &face):
    vertex { &vertex },
    edge   { &edge   },
    face   { &face   }
{}

auto geom2d::planegraph::ThickVertex::operator[](NeighborType type) const -> ThickVertex*
{
    return neighbors[type];
}

void geom2d::planegraph::ThickVertex::connect(ThickVertex& v1, ThickVertex& v2, NeighborType type)
{
    v1.neighbors[(int) type] = &v2;
    v2.neighbors[(int) type] = &v1;
}

//-----------------------------------------------------------------------------
// BigonIterator
//-----------------------------------------------------------------------------

geom2d::planegraph::BigonIterator::BigonIterator(ThickVertex *first, NeighborType t1, NeighborType t2):
    first { first },
    types { t1, t2 }
{}

bool geom2d::planegraph::BigonIterator::next()
{
    if (current == nullptr) {
        current = first;
        return true;
    }
    else {
        current = current->operator[](types[parity]);
        parity = (parity + 1) % 2;
        if (current == first) {
            return false;
        }
        else {
            return true;
        }
    }
}

auto geom2d::planegraph::BigonIterator::getCurrent() const -> ThickVertex*
{
    return current;
}

//-----------------------------------------------------------------------------
// PlaneGraph
//-----------------------------------------------------------------------------

geom2d::planegraph::PlaneGraph::PlaneGraph(const Polygon &polygon)
{

    size_t n = polygon.size();

    // exterior face will be labeled zero
    Face& exterior_face = this->newFace();

    // interior face will be labeled one
    Face& interior_face = this->newFace();

    // create n vertices and edges
    for (auto &p: polygon.data()) {
        this->newVertex(p);
        this->newEdge();
    }

    ThickVertex* prev_ui1_int_p = nullptr;
    ThickVertex* prev_ui1_ext_p = nullptr;

    auto connect_vertex_change = [&prev_ui1_int_p, &prev_ui1_ext_p](ThickVertex& ui0_int, ThickVertex& ui0_ext) {
        ThickVertex& prev_ui1_int = *prev_ui1_int_p;
        ThickVertex& prev_ui1_ext = *prev_ui1_ext_p;
        ThickVertex::connect(ui0_int, prev_ui1_int, VERTEX);
        ThickVertex::connect(ui0_ext, prev_ui1_ext, VERTEX);
    };

    for (size_t i=0;i<n;i++) {
        size_t curr = i;
        size_t prev = (i > 0 ? i-1 : n-1);

        Vertex& v_curr = this->getVertex(curr);

        Edge&   e_curr = this->getEdge(curr);
        Edge&   e_prev = this->getEdge(prev);

        // create four thick vertices
        ThickVertex& ui0_int = this->newThickVertex(v_curr, e_prev, interior_face, TARGET);
        ThickVertex& ui0_ext = this->newThickVertex(v_curr, e_prev, exterior_face, SOURCE);
        ThickVertex& ui1_int = this->newThickVertex(v_curr, e_curr, interior_face, SOURCE);
        ThickVertex& ui1_ext = this->newThickVertex(v_curr, e_curr, exterior_face, TARGET);

        // set entry point of current vertex and edge
        v_curr.entry_point = &ui0_int;
        e_curr.entry_point = &ui1_int;

        // connect square (EDGE and FACE changes)
        ThickVertex::connect(ui0_int, ui0_ext, FACE);
        ThickVertex::connect(ui1_int, ui1_ext, FACE);
        ThickVertex::connect(ui0_int, ui1_int, EDGE);
        ThickVertex::connect(ui0_ext, ui1_ext, EDGE);

        if (i == 0) {
            interior_face.entry_point = &ui1_int; // a SOURCE vertex
            exterior_face.entry_point = &ui0_ext; // a SOURCE vertex
        }

        if (i > 0) {
            connect_vertex_change(ui0_int, ui0_ext);
        }

        // prepare for next iteration
        prev_ui1_int_p = &ui1_int;
        prev_ui1_ext_p = &ui1_ext;
    }

    // we are missing two VERTEX connections from vertex n-1 to vertex 0
    if (n > 0) {
        // by construction
        Vertex& v0 = getVertex(0);

        ThickVertex& ui0_int = *v0.entry_point;
        ThickVertex& ui0_ext = *ui0_int[FACE];

        connect_vertex_change(ui0_int, ui0_ext);
    }
}

auto geom2d::planegraph::PlaneGraph::getFace(size_t index) const -> Face&
{
    return *faces.at(index).get();
}

auto geom2d::planegraph::PlaneGraph::getEdge(size_t index) const -> Edge&
{
    return *edges.at(index).get();
}

auto geom2d::planegraph::PlaneGraph::getVertex(size_t index) const -> Vertex&
{
    return *vertices.at(index).get();
}

namespace geom2d {
namespace planegraph {

bool findThickVerticesOnFace(Vertex&       vertex,
                             Face*         face,
                             ThickVertex* &u_src,
                             ThickVertex* &u_tgt)
{
    ThickVertex* first = vertex.entry_point;
    ThickVertex* u = first;

    u_tgt = u_src = nullptr;

    while (true) {
        if (u->face == face) {
            if (u->source) {
                u_src = u;
                u_tgt = u->operator[](EDGE);
            }
            else {
                u_src = u->operator[](EDGE);
                u_tgt = u;
            }
            return true;
        }
        u = u->operator[](FACE)->operator[](EDGE);
        if (u == first) {
            return false;
        }
    }
}

}
}


bool geom2d::planegraph::PlaneGraph::isConvex(geom2d::planegraph::Vertex &v, geom2d::planegraph::Face &f) const
{
    ThickVertex *u_src_p;
    ThickVertex *u_tgt_p;
    bool flag = findThickVerticesOnFace(v, &f, u_src_p, u_tgt_p);

    if (!flag)
        throw geom2d::Geom2DException("Vertex is not incident to face");

    // check the angle (assuming it needs to be on the left to be convex)
    Point a = u_tgt_p->operator[](VERTEX)->vertex->coordinates;
    Point b = v.coordinates;
    Point c = u_src_p->operator[](VERTEX)->vertex->coordinates;

    return geom2d::area2(a,b,c) >= 0;
}

bool geom2d::planegraph::PlaneGraph::isReflex(geom2d::planegraph::Vertex &v, geom2d::planegraph::Face &f) const
{
    return !isConvex(v,f);
}

auto geom2d::planegraph::PlaneGraph::newVertex(Point coordinates) -> Vertex&
{
    vertices.push_back(std::unique_ptr<Vertex>(new Vertex(this->next_vertex_label++, coordinates)));
    return *vertices.back().get();
}

auto geom2d::planegraph::PlaneGraph::newEdge() -> Edge&
{
    edges.push_back(std::unique_ptr<Edge>(new Edge(this->next_edge_label++)));
    return *edges.back().get();
}

auto geom2d::planegraph::PlaneGraph::newFace() -> Face&
{
    faces.push_back(std::unique_ptr<Face>(new Face(this->next_face_label++)));
    return *faces.back().get();
}

auto geom2d::planegraph::PlaneGraph::newThickVertex(Vertex &vertex, Edge &edge, Face &face, bool source) -> ThickVertex&
{
    thick_vertices.push_back(std::unique_ptr<ThickVertex>(new ThickVertex(vertex, edge, face)));
    ThickVertex& result = *thick_vertices.back().get();
    result.source = source;
    return result;
}

auto geom2d::planegraph::PlaneGraph::findInteriorCommonFace(geom2d::planegraph::Vertex &v1, geom2d::planegraph::Vertex &v2) const -> Face&
{
    std::vector<Face*> faces1;

    BigonIterator it1 { v1.entry_point, EDGE, FACE };
    while (it1.next()) {
        Face* face = it1.getCurrent()->face;
        if (face->label != getFace(0).label) {
            faces1.push_back(face);
        }
        it1.next(); // doesn't matter the next thick vertex (same face)
    }

    BigonIterator it2 { v2.entry_point, EDGE, FACE };
    while (it2.next()) {
        Face* face = it2.getCurrent()->face;
        if (face->label == getFace(0).label) {
            continue;
        }
        if (std::find(faces1.begin(),faces1.end(),face) != faces1.end()) {
            return *face;
        }
        it2.next(); // doesn't matter the next thick vertex (same face)
    }

    throw geom2d::Geom2DException("No Common Face Found");
}


auto geom2d::planegraph::PlaneGraph::subdivideInteriorFace(Vertex& v1, Vertex& v2) -> Face& {
    Face &face = findInteriorCommonFace(v1, v2);
    return this->subdivideFace(face, v1, v2);
}

auto geom2d::planegraph::PlaneGraph::subdivideFace(Face &face, Vertex& v1, Vertex& v2) -> Face&
{
    // find pair of thick vertices on v1 that are incident
    // to face f

    ThickVertex *v1_u0_p, *v1_u1_p, *v2_u0_p, *v2_u1_p;
    bool flag = findThickVerticesOnFace(v1, &face, v1_u1_p, v1_u0_p) && // u1 is a source, u0 is a target
         findThickVerticesOnFace(v2, &face, v2_u1_p, v2_u0_p);

    if (!flag) {
        throw geom2d::Geom2DException("couldn't find thick vertices incident to face");
    }

    ThickVertex& v1_u0 = *v1_u0_p;
    ThickVertex& v1_u1 = *v1_u1_p;
    ThickVertex& v2_u0 = *v2_u0_p;
    ThickVertex& v2_u1 = *v2_u1_p;

    // subdivide
    Face& existing_face = face;

    // create new face and new edge
    Face& new_face      = this->newFace();
    Edge& new_edge      = this->newEdge();

    // two new thick vertices on v1
    ThickVertex &v1_uu0 = this->newThickVertex(v1, new_edge, existing_face, SOURCE);
    ThickVertex &v1_uu1 = this->newThickVertex(v1, new_edge, new_face,      TARGET);

    // two new thick vertices on v2
    ThickVertex &v2_uu0 = this->newThickVertex(v2, new_edge, new_face,      SOURCE);
    ThickVertex &v2_uu1 = this->newThickVertex(v2, new_edge, existing_face, TARGET);

    // edge (angle) and face
    ThickVertex::connect(v1_u0,   v1_uu0, EDGE);
    ThickVertex::connect(v1_u1,   v1_uu1, EDGE);
    ThickVertex::connect(v1_uu0,  v1_uu1, FACE);

    // edge (angle) and face
    ThickVertex::connect(v2_u0,   v2_uu0, EDGE);
    ThickVertex::connect(v2_u1,   v2_uu1, EDGE);
    ThickVertex::connect(v2_uu0,  v2_uu1, FACE);

    // vertex
    ThickVertex::connect(v1_uu0,  v2_uu1, VERTEX);
    ThickVertex::connect(v2_uu0,  v1_uu1, VERTEX);

    //
    face.entry_point     = &v1_uu0;
    new_face.entry_point = &v1_u1;   // face label on thick vertices is outdated
    new_edge.entry_point = &v1_uu0;

    // update face thick vertices (this could be postponed for later,
    // by keeping a dirty flag on new_face).
    BigonIterator it(new_face.entry_point, VERTEX, EDGE); // no face change
    while (it.next()) {
        it.getCurrent()->face = &new_face;
    }

    return new_face;

}

std::ostream& geom2d::planegraph::io::operator<<(std::ostream &os, const geom2d::planegraph::Face &face)
{
    using namespace geom2d::planegraph;

//    ThickVertex *first = face.entry_point;
//    std::vector<NeighborType> x = {  };
//    if (first->source == TARGET) {
//        x = { EDGE, VERTEX };
//    }

    os << "Face [label:" << face.label << "] ";

    BigonIterator it(face.entry_point, VERTEX, EDGE);
    while (it.next()) {
        os << "v" << it.getCurrent()->vertex->label << " ";
        it.next(); // traverse edge (angle) without report
    }

//    ThickVertex *walker = first;
//    while (true) {
//        os << "v" << walker->vertex->label << " ";
//        walker = walker->operator[](x[0])->operator[](x[1]);
//        if (walker == first)
//            break;
//    }

    return os;
}



