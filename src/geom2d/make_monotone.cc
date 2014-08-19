#include "make_monotone.hh"

#include "base.hh"

#include <cassert>
#include <algorithm>

#include <iostream>
#include <iomanip>

#include "planegraph.hh"

namespace aux {

//------------------------------------------------------------------------------
// VertexType
//------------------------------------------------------------------------------

enum VertexType { START_VERTEX, END_VERTEX, SPLIT_VERTEX, MERGE_VERTEX, REGULAR_VERTEX };

//------------------------------------------------------------------------------
// Vertex
//------------------------------------------------------------------------------

struct Vertex {

    Vertex(geom2d::Point p, std::size_t index);

    auto getType() const -> VertexType;

    bool isAbove(const Vertex &v) const;
    bool isBelow(const Vertex &v) const;

    geom2d::Point coords;

    Vertex* next { nullptr };
    Vertex* prev{ nullptr };

    // auxiliar stuff
    std::size_t          index     { 0 };
    Vertex*              helper    { nullptr };

    void*                user_data { nullptr };
};

//------------------------------------------------------------------------------
// DoublyLinkedPolygon
//------------------------------------------------------------------------------

struct DoublyLinkedPolygon
{
    DoublyLinkedPolygon(const geom2d::Polygon &poly);

    auto operator[](std::size_t index) const -> Vertex*;

    size_t size() const;

    std::size_t  index(Vertex *v) const;

    std::vector<std::unique_ptr<Vertex>> vertices;
};

std::ostream& operator<<(std::ostream &os, const Vertex& vertex);
std::ostream& operator<<(std::ostream &os, const VertexType& vertex_type);

}





namespace aux {

//------------------------------------------------------------------------------
// io
//------------------------------------------------------------------------------

std::ostream& operator<<(std::ostream &os, const VertexType& vertex_type) {
    static std::vector<std::string> vertex_type_name { "START_VERTEX", "END_VERTEX", "SPLIT_VERTEX", "MERGE_VERTEX", "REGULAR_VERTEX"};
    int index = static_cast<int>(vertex_type);
    os << vertex_type_name.at(index);
    return os;
}

    std::ostream& operator<<(std::ostream &os, const Vertex& vertex) {
        os << "(" << std::setprecision(18) << vertex.coords.x << "," << std::setprecision(18) << vertex.coords.y << ")";
        return os;
    }

    
//------------------------------------------------------------------------------
// Vertex Impl.
//------------------------------------------------------------------------------

Vertex::Vertex(geom2d::Point p, std::size_t index):
    coords(p),
    index(index)
{}

bool Vertex::isAbove(const Vertex &v) const {
    return coords.y > v.coords.y || (coords.y == v.coords.y && coords.x < v.coords.x);
}

bool Vertex::isBelow(const Vertex &v) const {
    return !isAbove(v);
}

auto Vertex::getType() const -> VertexType {

    if (next == nullptr || prev == nullptr)
        throw std::runtime_error("Invalid Polygon");

//    {
//        // using namespace geom2d::io;
//        geom2d::io::operator<<(std::cout, prev->coords) << " <--- prev" << std::endl;
//        geom2d::io::operator<<(std::cout, coords) << " <--- curr" << std::endl;
//        geom2d::io::operator<<(std::cout, next->coords) << " <--- next" << std::endl;
//    }
    
    // interior angle is less than pi
    bool convex = geom2d::left(prev->coords, coords, next->coords);

    // interior angle is greater than pi
    bool reflex = geom2d::right(prev->coords, coords, next->coords);

    if (this->isAbove(*prev) && this->isAbove(*next)) {
        if (convex) {
            return START_VERTEX;
        }
        else if (reflex) {
            return SPLIT_VERTEX;
        }
        else throw geom2d::Geom2DException("Collinear vertex found. Can't handle it.");
    }
    else if (this->isBelow(*prev) && this->isBelow(*next))
    {
        if (convex) {
            return END_VERTEX;
        }
        else if (reflex) {
            return MERGE_VERTEX;
        }
        else throw geom2d::Geom2DException("Collinear vertex found. Can't handle it.");
    }
    else {
        return REGULAR_VERTEX;
    }

}

//------------------------------------------------------------------------------
// DoublyLinkedPolygon
//------------------------------------------------------------------------------

DoublyLinkedPolygon::DoublyLinkedPolygon(const geom2d::Polygon &poly) {
    size_t n = poly.size();
    vertices.resize(n);
    std::size_t index = 0;
    auto &vertices = this->vertices;
    std::for_each(poly.data().begin(), poly.data().end(), [&vertices,&index](geom2d::Point p) {
        vertices[index] = std::unique_ptr<Vertex>(new Vertex(p,index));
        if (index > 0) {
            vertices[index].get()->prev = vertices[index-1].get();
            vertices[index-1].get()->next = vertices[index].get();
        }
        ++index;
    });
    vertices[n-1].get()->next = vertices[0].get();
    vertices[0].get()->prev = vertices[n-1].get();

//
//    Vertex* last = new Vertex(poly.data().back());
//    last->index = n-1;
//
//    vertices[n-1] = std::unique_ptr<Vertex>(last);
//
//    Vertex* prev = last;
//    for (size_t i=0;i<n;i++) {
//        Vertex* curr = last;
//        if (i < n-1) {
//            curr = new Vertex(poly[i]);
//            curr->index = i;
//            vertices[i] = std::unique_ptr<Vertex>(curr);
//        }
//        curr->prev = prev;
//        prev->next = curr;
//        prev = curr;
//    }

}

auto DoublyLinkedPolygon::operator[](size_t index) const -> Vertex* {
    if (vertices[index].get() == nullptr)
        return nullptr;
    return vertices[index].get();
}

size_t DoublyLinkedPolygon::size() const {
    return vertices.size();
}

std::size_t DoublyLinkedPolygon::index(Vertex *v) const
{
    return v->index;
//    auto f = [v] (const std::unique_ptr<Vertex>& v_ptr) -> bool {
//        return v == v_ptr.get();
//    };
//    return std::find_if(vertices.begin(),vertices.end(),f) - vertices.begin();
}


// Ineficient for now
struct EdgeIndex {
    // interpret as the vertex->prev to vertex edge
    void insert(Vertex* vertex) {
        edges.push_back(vertex);
    }
    void remove(Vertex *e) {
        edges.erase(std::find(edges.begin(), edges.end(), e));
    }
    auto findRightmostLeftEdge(Vertex *vertex) const -> Vertex* {

        double  argument = 0.0;
        Vertex* result { nullptr };

        //
        double x = vertex->coords.x;
        double y = vertex->coords.y;
        for (Vertex* e: edges) {
            geom2d::Point p0 = e->coords;
            geom2d::Point p1 = e->next->coords;
            double coef = (y - p0.y)/(p1.y - p0.y);
            double xx = p0.x + (p1.x - p0.x) * coef;
            double candidate_argument = x - xx;
            if (candidate_argument > 0 && (result == nullptr || candidate_argument < argument)) {
                result = e;
                argument = candidate_argument;
            }
        }

        if (result == nullptr) {
            throw geom2d::Geom2DException("Empty result on findRightmostLeftEdge when one was expected");
        }

        return result;
    }
    std::vector<Vertex*> edges;
};

}


//
// Following algorithm described in Chapter 3 of
// Computational Geometry Algorithms and Applications
// Marc de Berg, Otfried Cheong, Marc van Kreveld, Mark Overmars
// Chapter 3
//

//------------------------------------------------------------------------------
// triangulateMonotonePolygon
//------------------------------------------------------------------------------

std::vector<geom2d::Polygon> geom2d::triangulateMonotonePolygon(const geom2d::Polygon &poly)
{
    using namespace aux;

    std::vector<Polygon> result;

    //
    DoublyLinkedPolygon dlpoly { poly };

    // create vertex priority queue simulating a top-down plane sweep
    // point. Left-most points with the same y-coordinate are considered
    // higher priority
    auto comp = [](const Vertex *v1, const Vertex *v2) {
        // less priority than
        return (v1->coords.y < v2->coords.y) || (v1->coords.y == v2->coords.y && v1->coords.x > v2->coords.x);
    };
    std::vector<Vertex*> priority_queue(dlpoly.size());
    for (size_t i=0;i<dlpoly.size();i++) {
        priority_queue[i] = dlpoly[i];
    }
    std::sort(priority_queue.begin(),priority_queue.end(),comp);

    // bottom and top indices
    std::size_t top_index    = priority_queue.front()->index;
    std::size_t bottom_index = priority_queue.back()->index;

    enum Chain { TOP, BOTTOM, LEFT, RIGHT };

    // from top_index bottom_index we have the left chain
    // from bottom_index to top_index we have the right chain
    auto chain = [top_index, bottom_index](Vertex *v) -> Chain {
        if (v->index == bottom_index) {
            return BOTTOM;
        }
        else if (v->index == top_index) {
            return TOP;
        }
        else if (bottom_index < top_index) {
            if (bottom_index < v->index && v->index < top_index) {
                return RIGHT;
            }
            else {
                return LEFT;
            }
        }
        else { // if (bottom_index > top_index) {
            if (top_index < v->index && v->index < bottom_index) {
                return LEFT;
            }
            else {
                return RIGHT;
            }
        }
    };

    using Diagonal = std::tuple<Vertex*, Vertex*>;

    std::vector<Diagonal> diagonals;

    //
    std::vector<Vertex*> stack;
    stack.push_back(priority_queue[0]);
    stack.push_back(priority_queue[1]);

    size_t n = poly.size();
    for (size_t j=2;j<n-1;j++) {
        Vertex* uj       = priority_queue[j];
        // Vertex* uj_prev  = priority_queue[j-1];
        Vertex* top      = stack.back();

        Chain chain_uj = chain(uj);

        if (chain_uj != BOTTOM && chain_uj != chain(top)) {
            for (size_t ii=1;ii<stack.size();ii++) {
                diagonals.push_back(Diagonal{ stack[ii], uj });
            }
            stack.erase(stack.begin(),stack.end()-1);
            stack.push_back(uj);
        }
        else {
            std::size_t k = stack.size() - 1;
            while (k > 0) {
                if (chain_uj == BOTTOM && chain(stack[k-1]) != TOP) {
                    k--;
                }
                else if (chain_uj == RIGHT && geom2d::area2(uj->coords, stack[k]->coords, stack[k-1]->coords) > 0) {
                    k--;
                }
                else if (chain_uj == LEFT  && geom2d::area2(uj->coords, stack[k-1]->coords, stack[k]->coords) > 0) {
                    k--;
                }
                else {
                    break;
                }
            }
            for (size_t ii=k;ii<stack.size()-1;ii++) {
                diagonals.push_back(Diagonal{ stack[ii], uj });
            }
            stack.erase(stack.begin()+k+1,stack.end());
            stack.push_back(uj);
        }
    }
    {
        Vertex* un = priority_queue[n-1];
        for (size_t ii=1;ii<stack.size()-1;ii++) {
            diagonals.push_back(Diagonal{ stack[ii], un });
        }
    }



    // collect polygons
    {
        size_t n = poly.size();
        geom2d::planegraph::PlaneGraph planegraph(poly);

#ifdef DEBUG_GEOM_2D
        {
            using namespace geom2d::planegraph;
            using namespace geom2d::planegraph::io;
            std::cout << "triangulation of polygon " << planegraph.getFace(1) << std::endl;
        }
#endif
        
        for (size_t i=0; i<n; i++) {
            Vertex* v = dlpoly[i];
            geom2d::planegraph::Vertex *vv = &planegraph.getVertex(i);
            v->user_data  = static_cast<void*>(vv);
        }

        //
        for (Diagonal &diagonal: diagonals) {
            Vertex* v1 = std::get<0>(diagonal);
            Vertex* v2 = std::get<1>(diagonal);
            geom2d::planegraph::Vertex& vv1 = *reinterpret_cast<geom2d::planegraph::Vertex*>(v1->user_data);
            geom2d::planegraph::Vertex& vv2 = *reinterpret_cast<geom2d::planegraph::Vertex*>(v2->user_data);

#ifdef DEBUG_GEOM_2D
            std::cout << "add diagonal (" << vv1.label << "," << vv2.label << ")" << std::endl;
#endif
            
            planegraph.subdivideInteriorFace(vv1,vv2);
        }

        // report
        for (size_t i=1;i<planegraph.faces.size();i++) {
            using namespace geom2d::planegraph;

#ifdef DEBUG_GEOM_2D
            using namespace geom2d::planegraph::io;
            std::cout << planegraph.getFace(i) << std::endl;
#endif

            Polygon poly_i;
            geom2d::planegraph::BigonIterator it(planegraph.getFace(i).entry_point,VERTEX,EDGE);
            while (it.next()) {
                poly_i.add(it.getCurrent()->vertex->coordinates);
                it.next();
            }
            result.push_back(std::move(poly_i));
        }
    }

    return result;

}



//------------------------------------------------------------------------------
// convexPartition
//------------------------------------------------------------------------------

std::vector<geom2d::Polygon> geom2d::convexPartition(const geom2d::Polygon &poly)
{
    using namespace aux;

    std::vector<Polygon> result;

    //
    DoublyLinkedPolygon dlpoly { poly };

    // create vertex priority queue simulating a top-down plane sweep
    // point. Left-most points with the same y-coordinate are considered
    // higher priority
    auto comp = [](const Vertex *v1, const Vertex *v2) {
        // less priority than
        return (v1->coords.y < v2->coords.y) || (v1->coords.y == v2->coords.y && v1->coords.x > v2->coords.x);
    };
    std::vector<Vertex*> priority_queue(dlpoly.size());
    for (size_t i=0;i<dlpoly.size();i++) {
        priority_queue[i] = dlpoly[i];
    }
    std::sort(priority_queue.begin(),priority_queue.end(),comp);

    // bottom and top indices
    std::size_t top_index    = priority_queue.front()->index;
    std::size_t bottom_index = priority_queue.back()->index;

    enum Chain { TOP, BOTTOM, LEFT, RIGHT };

    // from top_index bottom_index we have the left chain
    // from bottom_index to top_index we have the right chain
    auto chain = [top_index, bottom_index](Vertex *v) -> Chain {
        if (v->index == bottom_index) {
            return BOTTOM;
        }
        else if (v->index == top_index) {
            return TOP;
        }
        else if (bottom_index < top_index) {
            if (bottom_index < v->index && v->index < top_index) {
                return RIGHT;
            }
            else {
                return LEFT;
            }
        }
        else { // if (bottom_index > top_index) {
            if (top_index < v->index && v->index < bottom_index) {
                return LEFT;
            }
            else {
                return RIGHT;
            }
        }
    };

    using Diagonal = std::tuple<Vertex*, Vertex*>;

    std::vector<Diagonal> diagonals;

    //
    std::vector<Vertex*> stack;
    stack.push_back(priority_queue[0]);
    stack.push_back(priority_queue[1]);

    size_t n = poly.size();
    for (size_t j=2;j<n-1;j++) {
        Vertex* uj       = priority_queue[j];
        // Vertex* uj_prev  = priority_queue[j-1];
        Vertex* top      = stack.back();

        Chain chain_uj = chain(uj);

        if (chain_uj != BOTTOM && chain_uj != chain(top)) {
            for (size_t ii=1;ii<stack.size();ii++) {
                diagonals.push_back(Diagonal{ stack[ii], uj });
            }
            stack.erase(stack.begin(),stack.end()-1);
            stack.push_back(uj);
        }
        else {
            std::size_t k = stack.size() - 1;
            while (k > 0) {
                if (chain_uj == BOTTOM && chain(stack[k-1]) != TOP) {
                    k--;
                }
                else if (chain_uj == RIGHT && geom2d::area2(uj->coords, stack[k]->coords, stack[k-1]->coords) > 0) {
                    k--;
                }
                else if (chain_uj == LEFT  && geom2d::area2(uj->coords, stack[k-1]->coords, stack[k]->coords) > 0) {
                    k--;
                }
                else {
                    break;
                }
            }
            for (size_t ii=k;ii<stack.size()-1;ii++) {
                diagonals.push_back(Diagonal{ stack[ii], uj });
            }
            stack.erase(stack.begin()+k+1,stack.end());
            stack.push_back(uj);
        }
    }
    {
        Vertex* un = priority_queue[n-1];
        for (size_t ii=1;ii<stack.size()-1;ii++) {
            diagonals.push_back(Diagonal{ stack[ii], un });
        }
    }



    // collect polygons
    {
        size_t n = poly.size();
        geom2d::planegraph::PlaneGraph planegraph(poly);

#ifdef DEBUG_GEOM_2D
        {
            using namespace geom2d::planegraph;
            using namespace geom2d::planegraph::io;
            std::cout << "triangulation of polygon " << planegraph.getFace(1) << std::endl;
        }
#endif

        for (size_t i=0; i<n; i++) {
            Vertex* v = dlpoly[i];
            geom2d::planegraph::Vertex *vv = &planegraph.getVertex(i);
            v->user_data  = static_cast<void*>(vv);
        }

        //
        for (Diagonal &diagonal: diagonals) {
            Vertex* v1 = std::get<0>(diagonal);
            Vertex* v2 = std::get<1>(diagonal);
            geom2d::planegraph::Vertex& vv1 = *reinterpret_cast<geom2d::planegraph::Vertex*>(v1->user_data);
            geom2d::planegraph::Vertex& vv2 = *reinterpret_cast<geom2d::planegraph::Vertex*>(v2->user_data);


            geom2d::planegraph::Face &face = planegraph.findInteriorCommonFace(vv1,vv2);

            if (planegraph.isConvex(vv1, face) && planegraph.isConvex(vv2, face))
                continue;

#ifdef DEBUG_GEOM_2D
            std::cout << "add diagonal (" << vv1.label << "," << vv2.label << ")" << std::endl;
#endif
            
            planegraph.subdivideInteriorFace(vv1,vv2);
        }

        // report
        for (size_t i=1;i<planegraph.faces.size();i++) {
            using namespace geom2d::planegraph;
            using namespace geom2d::planegraph::io;

#ifdef DEBUG_GEOM_2D
            std::cout << planegraph.getFace(i) << std::endl;
#endif

            Polygon poly_i;
            geom2d::planegraph::BigonIterator it(planegraph.getFace(i).entry_point,VERTEX,EDGE);
            while (it.next()) {
                poly_i.add(it.getCurrent()->vertex->coordinates);
                it.next();
            }
            result.push_back(std::move(poly_i));
        }
    }

    return result;

}



//------------------------------------------------------------------------------
// makeMonotone
//------------------------------------------------------------------------------

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T*> &v) {
    bool first = true;
    std::for_each(v.cbegin(),
                  v.cend(),
                  [&os, &first] (const T* obj){
//                      if (!first)
//                          os << ", ";
                      os << *obj << std::endl;
                      first=false;
                  });
    return os;
}


std::vector<geom2d::Polygon> geom2d::makeMonotone(const geom2d::Polygon &poly)
{

#ifdef DEBUG_GEOM_2D
    std::cout << "makeMonotone(...)" << std::endl;
#endif
    
    using namespace aux;

    std::vector<Polygon> result;

//    // print priority queue
//    {
//        using namespace geom2d::io;
//        std::cout << poly << std::endl;
//    }


    //
    DoublyLinkedPolygon dlpoly { poly };

    // create vertex priority queue simulating a top-down plane sweep
    // point. Left-most points with the same y-coordinate are considered
    // higher priority
    auto comp = [](const Vertex *v1, const Vertex *v2) {
        // less priority than
        
        bool flag = (v1->coords.y < v2->coords.y) || (v1->coords.y == v2->coords.y && v1->coords.x > v2->coords.x);
        
        // std::cout << "v1:" << *v1 << " < " << " v2:" << *v2 << " ? " << flag << "            " << ((v1 == v2) ? "SAME_VERTEX" : "") << std::endl;
        
        return flag;
    };
    
    std::vector<Vertex*> priority_queue;
    for (auto &ptr: dlpoly.vertices) {
        priority_queue.push_back(ptr.get());
    }
    std::sort(priority_queue.begin(),priority_queue.end(),comp);

    using Diagonal = std::tuple<Vertex*, Vertex*>;

    std::vector<Diagonal> diagonals;

    EdgeIndex visible_edges;

    auto add_diagonal = [&diagonals](Vertex* v1, Vertex* v2) {
        diagonals.push_back(Diagonal{v1,v2});
    };
    
    
    // print priority queue
#ifdef DEBUG_GEOM_2D
    std::cout << priority_queue << std::endl;
#endif

    

    // create an empty binary search tree
    while (priority_queue.empty() == false) {

        // h = i-1

        Vertex* vi = priority_queue.back();
        priority_queue.pop_back();

#ifdef DEBUG_GEOM_2D
        std::cout << "procssing v" << (dlpoly.index(vi)+1) << std::endl;
#endif
        
        VertexType ti = vi->getType();

        if (ti == START_VERTEX) {

#ifdef DEBUG_GEOM_2D
            std::cout << "   START_VERTEX" << std::endl;
#endif

            vi->helper = vi;
            visible_edges.insert(vi);
        }
        else if (ti == END_VERTEX) {

#ifdef DEBUG_GEOM_2D
            std::cout << "   END_VERTEX" << std::endl;
#endif

            if (!vi->prev)
                throw std::runtime_error("ooops");
            
            if (vi->prev->helper->getType() == MERGE_VERTEX) {
                add_diagonal(vi,vi->prev->helper);
            }
            visible_edges.remove(vi->prev);
//            if helper(ei%1) is a merge vertex
//            then Insert the diagonal connecting vi to helper(ei%1) in D.
//            Delete ei%1 from T.
        }
        else if (ti == SPLIT_VERTEX) {

#ifdef DEBUG_GEOM_2D
            std::cout << "   SPLIT_VERTEX" << std::endl;
#endif
            Vertex *vj = visible_edges.findRightmostLeftEdge(vi);

            add_diagonal(vi,vj->helper);

            vj->helper = vi;
            vi->helper = vi;
            visible_edges.insert(vi);
//            HANDLESPLITVERTEX(vi)
//            1. Search in T to find the edge ej directly left of vi.
//            2. Insert the diagonal connecting vi to helper(e j ) in
//            3. helper(e j ) " vi
//            4. Insert ei in T and set helper(ei) to vi.
        }
        else if (ti == REGULAR_VERTEX) {

#ifdef DEBUG_GEOM_2D
            std::cout << "   REGULAR_VERTEX" << std::endl;
#endif
            // TODO: figure out this test
            bool interior_on_the_right = vi->isAbove(*vi->next);

            if (interior_on_the_right) {
                
                if (!vi->prev || !vi->prev->helper)
                    throw std::runtime_error("ooops");
                
                if (vi->prev->helper->getType() == MERGE_VERTEX) {
                    add_diagonal(vi,vi->prev->helper);
                }
                visible_edges.remove(vi->prev);
                vi->helper = vi;
                visible_edges.insert(vi);
            }
            else {
                Vertex *vj = visible_edges.findRightmostLeftEdge(vi);
                if (vj->helper->getType() == MERGE_VERTEX) {
                    add_diagonal(vi,vj->helper);
                }
                vj->helper = vi;
            }

//            if the interior of P lies to the right of vi then if helper(ei%1) is a merge vertex
//            then Insert the diagonal connecting vi to helper(ei%1) in D. Delete ei%1 from T.
//            Insert ei in T and set helper(ei) to vi.
//            else Search in T to find the edge ej directly left of vi. if helper(ej) is a merge vertex
//            then Insert the diagonal connecting vi to helper(ej) in D. helper(e j ) " vi
        }
        else if (ti == MERGE_VERTEX) {

#ifdef DEBUG_GEOM_2D
            std::cout << "   MERGE_VERTEX" << std::endl;
#endif

            if (vi->prev->helper->getType() == MERGE_VERTEX) {
                add_diagonal(vi,vi->prev->helper);
            }
            visible_edges.remove(vi->prev);
            Vertex *vj = visible_edges.findRightmostLeftEdge(vi);
            if (vj->helper->getType() == MERGE_VERTEX) {
                add_diagonal(vi,vj->helper);
            }
            vj->helper = vi;
//            HANDLEMERGEVERTEX(vi)
//            if helper(ei%1) is a merge vertex
//            then Insert the diagonal connecting vi to helper(ei%1) in D.
//            Delete ei%1 from T.
//            Search in T to find the edge ej directly left of vi. if helper(e j ) is a merge vertex
//            then Insert the diagonal connecting vi to helper(ej) in D. helper(e j ) " vi
        }

    }


#ifdef DEBUG_GEOM_2D
    for (Diagonal &diagonal: diagonals) {
        Vertex* v1 = std::get<0>(diagonal);
        Vertex* v2 = std::get<1>(diagonal);

        std::size_t i = dlpoly.index(v1) + 1;
        std::size_t j = dlpoly.index(v2) + 1;
        std::cout << "diagonal from (" << i << "," << j << ")" << std::endl;
    }
#endif

    // collect polygons
    {
        size_t n = poly.size();
        geom2d::planegraph::PlaneGraph planegraph(poly);
        for (size_t i=0; i<n; i++) {
            Vertex* v = dlpoly[i];
            geom2d::planegraph::Vertex *vv = &planegraph.getVertex(i);
            v->user_data  = static_cast<void*>(vv);
        }

        //
        for (Diagonal &diagonal: diagonals) {
            Vertex* v1 = std::get<0>(diagonal);
            Vertex* v2 = std::get<1>(diagonal);
            geom2d::planegraph::Vertex& vv1 = *reinterpret_cast<geom2d::planegraph::Vertex*>(v1->user_data);
            geom2d::planegraph::Vertex& vv2 = *reinterpret_cast<geom2d::planegraph::Vertex*>(v2->user_data);
            planegraph.subdivideInteriorFace(vv1,vv2);
        }

        // report
        for (size_t i=1;i<planegraph.faces.size();i++) {
            using namespace geom2d::planegraph;
            
#ifdef DEBUG_GEOM_2D
            using namespace geom2d::planegraph::io;
            std::cout << planegraph.getFace(i) << std::endl;
#endif
            
            Polygon poly_i;
            geom2d::planegraph::BigonIterator it(planegraph.getFace(i).entry_point,VERTEX,EDGE);
            while (it.next()) {
                poly_i.add(it.getCurrent()->vertex->coordinates);
                it.next();
            }
            result.push_back(std::move(poly_i));
        }
    }

    return result;

}


//-----------------------------------------------------------------------------
// test_make_monotone
//-----------------------------------------------------------------------------

void geom2d::test_make_monotone() {

    using namespace aux;
    using namespace geom2d::io;

    Polygon poly1 { {
    {165,  100},
    {135,  110},
    {125,  185},
    { 90,  170},
    { 70,  190},
    { 35,  165},
    { 60,  135},
    { 45,  100},
    { 20,  120},
    {  5,   55},
    { 40,   10},
    { 75,   35},
    {120,    5},
    {105,   70},
    {160,   50}
    } };

    Polygon poly2 { {
    {185,150},
    {135,130},
    {125,205},
    {90,190},
    {70,210},
    {35,185},
    {60,155},
    {45,120},
    {20,140},
    {5,75},
    {40,30},
    {75,55},
    {130,5},
    {105,90},
    {160,70}
    } };

    DoublyLinkedPolygon dpoly { poly2 };

    for (size_t i=0;i<dpoly.size();i++) {
        const Vertex& v = *dpoly[i];
        std::cout << "Vertex " << std::setw(2) << (i+1) << " with coords " << v.coords << " is a " << v.getType() << std::endl;
    }

    makeMonotone(poly2);

}


