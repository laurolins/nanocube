#include "boundary.hh"

#include <numeric>
#include <deque>
#include <unordered_map>

namespace polycover {
    
    namespace bounds {
        
        
        Direction opposite(Direction d) {
            switch (d) {
                case LEFT:
                    return RIGHT;
                case RIGHT:
                    return LEFT;
                case BOTTOM:
                    return TOP;
                case TOP:
                    return BOTTOM;
                default:
                    throw std::runtime_error("ooops");
            }
        }
        
        
        //---------------------------------------------------------------
        // Cell Impl.
        //---------------------------------------------------------------
        
        Cell::Cell(int x, int y, int level):
        x(x),y(y),level(level)
        {}
        
        Cell Cell::child(int child) const {
            auto xbit = (child & 0x1) ? 1 : 0;
            auto ybit = (child & 0x2) ? 1 : 0;
            return Cell(2 * x + xbit, 2 * y + ybit, level+1);
        }
        
        Cell Cell::vertex() const {
            Cell result = *this;
            while (result.x % 2 == 0 && result.y % 2 == 0 && result.level > 0) {
                result.x >>= 1;
                result.y >>= 1;
                --result.level;
            }
            return result;
        }
        
        std::size_t Cell::hash() const {
            return id;
        }
        
        
        Edge Cell::edge(Direction d) {
            switch (d) {
                case LEFT:
                    return Edge(x,y,level,1);
                case RIGHT:
                    return Edge(x+1,y,level,1);
                case BOTTOM:
                    return Edge(x,y,level,0);
                case TOP:
                    return Edge(x,y+1,level,0);
                default:
                    throw std::runtime_error("oops");
            }
        }
        
        inline bool Cell::operator==(const Cell& other) const {
            return id == other.id;
        }
        
        //---------------------------------------------------------------
        // Edge Impl.
        //---------------------------------------------------------------
        
        Edge::Edge(int x, int y, int level, bool vertical):
        x(x),
        y(y),
        level(level),
        vertical(vertical)
        {}
        
        std::uint64_t Edge::coord(int dim) const {
            return (dim==0) ? x : y;
        }
        
        Edge Edge::adjacent(int delta) const {
            if (vertical) {
                return Edge(x,y+delta,level,vertical);
            }
            else {
                return Edge(x+delta,y,level,vertical);
            }
        }
        
        Edge Edge::child(int level_offset) const {
            return Edge(x << level_offset, y << level_offset, level + level_offset, vertical);
        }
        
        void Edge::sumMod2(const Edge& other, std::vector<Edge> &output) {
            output.clear();
            
            if (*this == other) {
                // result is empty
                return;
            }
            else if (vertical != other.vertical || level == other.level) {
                output.push_back(*this);
                output.push_back(other);
                return;
            }
            
            // different levels and same orientation
            
            auto &big   = (level < other.level) ? *this : other;
            auto &small = (level < other.level) ? other : *this;
            
            auto level_difference = small.level - big.level;
            
            auto big_segment0 = big.child(level_difference);
            auto big_segment1 = big.adjacent(1).child(level_difference);
            
            int odim = vertical ? 1 : 0; // orientation dim
            int pdim = 1 - odim;
            
            if ( big_segment0.coord(pdim) != small.coord(pdim) ){
                output.push_back(*this);
                output.push_back(other);
                return;
            }
            
            
            if (big_segment0.coord(odim) > small.coord(odim) || big_segment1.coord(odim) <= small.coord(odim)) {
                output.push_back(*this);
                output.push_back(other);
                return;
            }
            else {
                // bit by bit do the intersecion
                auto e = big;
                for (auto i=0;i<level_difference;++i) {
                    auto small_coarsened = small.parent(level_difference-i-1);
                    auto e0 = e.child(1);
                    if (small_coarsened.coord(odim) == e0.coord(odim)) {
                        output.push_back(e0.adjacent(1));
                        e = e0;
                    }
                    else {
                        output.push_back(e0);
                        e = e0.adjacent(1);
                    }
                }
            }
        }
        
        std::size_t Edge::hash() const {
            return id;
        }
        
        inline bool Edge::operator==(const Edge& other) const {
            return id == other.id;
        }
        
        inline Edge Edge::parent(int level_offset) const {
            return Edge(x >> level_offset,y >> level_offset,level-level_offset,vertical);
        }
        
        Cell Edge::cell() const {
            return Cell(x,y,level);
        }
        
        //
        // EdgeSet
        //
        
        void EdgeSet::insert(const Edge& e) {
            
            //
            // assume e.level <= max{x.level: x in current edges} + 1
            // for all edges "x" currently in the set.
            //
            // In words: e is as deep or deeper (by one) than the current
            // deepest edge stored.
            //
            
            // auto odim = (e.vertical) ? 1 : 0;
            auto pdim = (e.vertical) ? 0 : 1;
            
            auto ee    = e;
            int  level = e.level;
            while (level > 0) {
                if ((ee.coord(pdim) % 2) == 1) {
                    // check if the same edge is already in the set.
                    // that is the only possibility of erasing something
                    // from the set
                    if (edges.count(e)) {
                        edges.erase(e);
                    }
                    else {
                        edges.insert(e);
                    }
                    return;
                }
                
                auto parent = ee.parent();
                if (edges.count(parent)) {
                    edges.erase(parent);
                    parent.sumMod2(e,buffer);
                    for (auto e: buffer) {
                        edges.insert(e);
                    }
                    return;
                }
                ee = parent;
                --level;
            }
            
            // if we get here is because we should insert e
            edges.insert(e);
            
        }
        
        
        std::vector<std::vector<Cell>> EdgeSet::generateContours() {
            
            // Contours
            std::vector<std::vector<Cell>> contours;
            
            struct Vertex {
                Vertex() = default;
                Vertex(Cell name):
                name(name)
                {}
                
                inline Vertex* adj(Direction dir) {
                    return _adj[dir];
                }
                
                inline Vertex& adj(Direction dir, Vertex* vadj) {
                    _adj[dir] = vadj;
                    return *this;
                }
                
                inline bool anyNeighbor(Vertex* &vadj, Direction& dir) {
                    for (auto i=0;i<4;++i)
                        if (_adj[i]) {
                            vadj = _adj[i];
                            dir  = (Direction) i;
                            return true;
                        }
                    return false;
                }
                
                int deg() const {
                    return std::accumulate(&_adj[0], &_adj[4], 0, [](int curr, Vertex* v) { return (v) ? 1 : 0; });
                }
                
                Vertex* _adj[4] { nullptr, nullptr, nullptr, nullptr }; // follow the direction thing
                Cell name; // canonical name
            };
            
            struct Graph {
                Graph() = default;
                
                void removeEdge(Vertex *u, Direction dir) {
                    auto v = u->adj(dir);
                    if (!v)
                        throw std::runtime_error("ooops");
                    u->adj(dir,nullptr);
                    v->adj(opposite(dir), nullptr);
                }
                
                void insert(const Edge& e) {
                    auto name0 = e.cell().vertex();
                    auto name1 = e.adjacent(1).cell().vertex();
                    auto v0 = vmap[name0];
                    auto v1 = vmap[name1];
                    if (!v0) {
                        v0 = new Vertex(name0);
                        vertices.push_back(std::unique_ptr<Vertex>(v0));
                        vmap[name0] = v0;
                    }
                    if (!v1) {
                        v1 = new Vertex(name1);
                        vertices.push_back(std::unique_ptr<Vertex>(v1));
                        vmap[name1] = v1;
                    }
                    if (e.vertical) {
                        v0->adj(TOP,v1);
                        v1->adj(BOTTOM,v0);
                    }
                    else {
                        v0->adj(RIGHT,v1);
                        v1->adj(LEFT,v0);
                    }
                }
                std::vector<std::unique_ptr<Vertex>> vertices;
                std::unordered_map<Cell, Vertex*, HashCell> vmap;
            };
            
            // double memory
            Graph graph;
            for (auto e: edges) {
                graph.insert(e);
            }
            
            for (auto &it: graph.vertices) {
                auto u = it.get();
                while (true) {
                    Direction dir;
                    Vertex *v;
                    
                    if (!u->anyNeighbor(v, dir)) {
                        break;
                    }
                    
                    // new contour
                    contours.push_back({});
                    auto &contour = contours.back();
                    
                    // insert first contour vertex
                    contour.push_back(u->name);
                    graph.removeEdge(u,dir);
                    
                    // start a new contour at vertex v
                    while (v != u) {
                        contour.push_back(v->name);
                        
                        Vertex *w;
                        
                        if (!v->anyNeighbor(w,dir))
                            throw std::runtime_error("ooops");
                        
                        graph.removeEdge(v, dir);
                        v = w;
                        
                    }
                    
                } // vertex u loop
                
            } // vertices loop
            
            return contours;
            
        }
        

        //-----------------
        // NormalizedPoint
        //-----------------
        
        NormalizedPoint::NormalizedPoint(double x, double y):
        x(x),y(y)
        {}

        /*!
         * Compute the boundary (rectilinear) polygons from a cell
         * decomposition
         */
        Boundary boundary(const Node& node, int max_level) {
            // One idea is to just insert/remove cell edges
            EdgeSet set;
            
            // since at least the root exists,
            
            struct Item{
            public:
                Item() = default;
                Item(const Node* node, const Cell& cell):
                node(node), cell(cell)
                {}
            public:
                const Node* node { nullptr };
                Cell  cell;
            };
            
            // make it a bfs
            std::deque<Item> queue;
            queue.push_back(Item{&node, Cell{0, 0, 0}});
            while (queue.size()) {
                Item item = queue.front();
                queue.pop_front();
                
                // don't refine anymore
                if (max_level >=0 && item.cell.level >= max_level)
                    continue;
                
                // refine item's node
                // internal edges are easy
                
                if (item.node->isLeaf()) {
                    auto &cell = item.cell;
                    set.insert(cell.edge(LEFT));
                    set.insert(cell.edge(RIGHT));
                    set.insert(cell.edge(TOP));
                    set.insert(cell.edge(BOTTOM));
                }
                else {
                    for (auto i=0;i<4;++i) {
                        auto child = item.node->children[i].get();
                        if (child) {
                            queue.push_back({child,item.cell.child(i)});
                        }
                    }
                }
            }
            
            // refine tree as we go...
            // bfs update an edge set
            
            
            //            std::cout << "--------- edges ----------" << std::endl;
            //            for (auto e: set.edges) {
            //                std::cout << e.x << " " << e.y << " " << e.level << " " << (e.vertical ? '|' : '-') << std::endl;
            //            }
            
            
            
            Boundary result;
            
            auto contours = set.generateContours();
            auto i = 0;
            for (auto &contour: contours) {
                
                result.push_back({});
                auto &boundary_contour = result.back();
                
                //                std::cout << "--------- contour " << (i++) << "----------" << std::endl;
                for (auto cell: contour) {
                    // get mercator coords
                    auto mx = 2.0 * (double) cell.x / (1 << cell.level) - 1.0;
                    auto my = 2.0 * (double) cell.y / (1 << cell.level) - 1.0;
                    // std::cout << cell.x << " " << cell.y << " " << cell.level << std::endl;
                    //                    std::cout << mx << " " << my << std::endl;
                    boundary_contour.push_back({mx,my});
                }
            }
            return result;
        }
        
    } // namespace boundary
    
} // polycover namespace
