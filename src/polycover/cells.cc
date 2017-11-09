#include "cells.hh"

#include <sstream>

namespace polycover {
    
    namespace cells {
    
        using ActionIterator = labeled_tree::Iterator;
        
        //-------------------------------------
        // Cells
        //-------------------------------------
        
        Cells::Cells(Tree &&tree) {
            _tree.swap(tree);
            // compute the statistics by scanning all nodes once.
            if (!_tree.empty()) {
                ActionIterator it(*_tree.root());
                ActionIterator::Action *a;
                while ((a = it.next())) {
                    if (a->type == ActionIterator::Action::PUSH) {
                        if (a->child->getNumChildren() == 0) {
                            stats.depth = std::max(stats.depth, a->depth);
                            ++stats.num_leaves;
                        }
                        ++stats.num_nodes;
                    }
                }
            }
        }
        
        const Node* Cells::root() const {
            return _tree.root();
        }
        
        const labeled_tree::LabeledTree& Cells::tree() const {
            return _tree;
        }

        Code Cells::code() const {
            return _tree.code();
        }
        
        Area Cells::area(int level) const {
            Area result = 0;
            if (!_tree.empty()) {
                CellIterator iter(_tree.root(), level);
                CellIterator::Item* item;
                while ((item=iter.next())) {
                    result += 1ULL << (2 * (level - item->level));
                }
            }
            return result;
        }
        
        //----------------------
        // CellIterator::Item
        //----------------------
        
        CellIterator::Item::Item(const Node* node, int level, int x, int y):
            node(node), level(level), x(x), y(y)
        {}
        
        std::ostream& operator<<(std::ostream& os, const CellIterator::Item& item) {
            os << "[x:" << item.x << ",y:" << item.y << ",level:" << item.level << "]";
            return os;
        }

        //----------------------
        // CellIterator
        //----------------------
        
        CellIterator::CellIterator(const Node *root, int max_level, bool force_max_level):
        max_level(max_level),
        force_max_level(force_max_level)
        {
//            std::cout << "...maxlev:" << max_level << std::endl;
            if (root) {
                stack.push_back({root,0,0,0});
            }
        }
        
        CellIterator::Item* CellIterator::next() {
            while (stack.size()) {
                // starting from the top of the stack
                Item item = stack.back();
                stack.pop_back();
                
                // (reach max_level) or (not forcing max level but node is a leaf)
                if (item.level == max_level || (!force_max_level && item.node && item.node->isLeaf())) { // done
                    // std::cout << "...found leaf" << item << "   maxlev:" << max_level << std::endl;
                    // std::cout << current << std::endl;
                    this->current = item;
                    return &current;
                }
                // level < max_level
                else if (item.node && !item.node->isLeaf()) {
                    for (int i=3;i>=0;--i) {
                        //                        std::cout << "...child:" << i << std::endl;
                        auto &child = item.node->children[i];
                        if (child) {
                            auto xbit = (i & 1) ? 1 : 0;
                            auto ybit = (i & 2) ? 1 : 0;
                            stack.push_back({child.get(),
                                item.level + 1,
                                (item.x << 1) + xbit,
                                (item.y << 1) + ybit
                            });
                        }
                    }
                }
                // here the remaining possibilities are item.node and leaf and force_max_level or !item.node
                else {
                    for (int i=3;i>=0;--i) {
                        auto xbit = (i & 1) ? 1 : 0;
                        auto ybit = (i & 2) ? 1 : 0;
                        stack.push_back({nullptr,
                            item.level + 1,
                            (item.x << 1) + xbit,
                            (item.y << 1) + ybit
                        });
                    }
                }
            }
            return nullptr;
        }
    
    }
    
}