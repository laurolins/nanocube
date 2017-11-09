#pragma once

#include <unordered_set>

#include "labeled_tree.hh"

namespace polycover {
    
    namespace bounds {
        
        //------
        // Node
        //------

        using labeled_tree::Node;

        
        //-----------
        // Direction
        //-----------
        
        enum EdgeType  { HORIZONTAL=0, VERTICAL=1      };
        enum Direction { LEFT=0,BOTTOM=1,RIGHT=2,TOP=3 };
        
        Direction opposite(Direction d);
        
        using EdgeID = std::uint64_t;
        
        struct Cell;
        struct Edge;
        struct Vertex;
        
        //-------------------------------------------------------------
        // Cell
        //-------------------------------------------------------------
        
        struct Cell {
            Cell(): id(0) {};
            Cell(int x, int y, int level);
            Cell child(int child) const;
            Cell vertex() const;
            Edge edge(Direction d);
            std::size_t hash() const;
            bool operator==(const Cell& other) const;
            union {
                struct {
                    std::uint64_t x: 29;
                    std::uint64_t y: 29;
                    std::uint64_t level: 6;
                };
                uint64_t id;
            };
        };
        
        //-------------------------------------------------------------
        // Edge
        //-------------------------------------------------------------
        
        struct Edge {
            Edge() = default;
            
            Edge(int x, int y, int level, bool vertical);
            
            std::size_t hash() const;
            
            bool operator==(const Edge& other) const;
            
            Edge parent(int level_offset=1) const;
            
            Cell cell() const;
            
            Edge child(int level_offset) const;
            
            Edge adjacent(int delta) const;
            
            std::uint64_t coord(int dim) const;
            
            void sumMod2(const Edge& e, std::vector<Edge> &output);
            
            // 29 bits for x
            // 29 bits for y
            //  5 bits for level
            //  1 bit  for orientaion (horiz/vertical)
            union {
                struct {
                    std::uint64_t x: 29;
                    std::uint64_t y: 29;
                    std::uint64_t level: 5;
                    std::uint64_t vertical: 1;
                };
                std::uint64_t id;
            };
        };
        
        //---------------------------------------------------------------
        // HashCell
        //---------------------------------------------------------------
        
        struct HashCell {
            inline const std::size_t operator()(const Cell& c) const {
                return c.hash();
            }
        };
        
        //---------------------------------------------------------------
        // HashEdge
        //---------------------------------------------------------------
        
        struct HashEdge {
            inline const std::size_t operator()(const Edge& e) const {
                return e.hash();
            }
        };
        
        //---------------------------------------------------------------
        // EdgeSet
        //---------------------------------------------------------------
        
        struct EdgeSet {
        public:
            EdgeSet() = default;
            void insert(const Edge& e);
            std::vector<std::vector<Cell>>  generateContours();
        public:
            std::vector<Edge> buffer;
            std::unordered_set<Edge, HashEdge> edges;
        };
        
        
        
        struct NormalizedPoint {
            NormalizedPoint() = default;
            NormalizedPoint(double x, double y);
            double x { 0.0 };
            double y { 0.0 };
        };

        using Boundary = std::vector<std::vector<NormalizedPoint>>;

        
        /*!
         * Compute the boundary (rectilinear) polygons from a cell
         * decomposition
         */
        Boundary boundary(const Node& node, int max_level=-1);
        
        
    } // namespace boundary
    
} // polycover namespace
