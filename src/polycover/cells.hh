#pragma once

#include <string>
#include <memory>

#include "labeled_tree.hh"

namespace polycover {
    
    namespace cells {
    
        using Tree  = labeled_tree::LabeledTree;
        using Node  = labeled_tree::Node;
        using Count = std::uint64_t;
        using Depth = int;
        using Code  = std::string;
        
        using Area  = std::uint64_t;

        //--------
        // Cells
        //--------
        
        struct Cells {
        public:
            Cells() = default;
            Cells(int x, int y, int level);
            Cells(Tree &&tree); // move tree
            Code code() const;
            const Node* root() const;

            Count num_leaves() const { return stats.num_leaves; }
            Count num_nodes() const { return stats.num_nodes; }
            Count depth() const { return stats.depth; }
            
            const labeled_tree::LabeledTree& tree() const;
            
            Area area(int level=25) const;
            
        public:
            struct {
                Count num_leaves { 0 };
                Count num_nodes  { 0 };
                Depth depth      { 0 };
            } stats;
            labeled_tree::LabeledTree _tree;
        };
        
        //--------------
        // CellIterator
        //--------------

        struct CellIterator {

        public:

            struct Item {
            public:
                Item() = default;
                Item(const Node* node, int level, int x, int y);
            public:
                const Node* node  { nullptr };
                int   level       { 0 };
                int   x           { 0 };
                int   y           { 0 };
            };
            
        public:
            
            CellIterator(const Node *node, int max_level=-1, bool force_max_level=false);
            Item* next();
            
        public:
            
            Item              current;
            std::vector<Item> stack;
            int               max_level;
            bool              force_max_level;
        
        };
        
    }
    
}