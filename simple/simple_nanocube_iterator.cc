#include <simple_nanocube_iterator.hh>

//-----------------------------------------------------------------------------
// Iterator::Item
//-----------------------------------------------------------------------------

Iterator::Item::Item(Node* node, Node* parent_node, Label label, int dimension, int level,
                     LinkType link_type):
    node { node },
    parent_node { parent_node },
    label { label },
    dimension { dimension },
    level { level },
    link_type {link_type }
{}

//-----------------------------------------------------------------------------
// Iterator
//-----------------------------------------------------------------------------

Iterator::Iterator(const Nanocube &nc):
    nanocube(nc)
{
    if (!nanocube.root)
        return; // nothing to iterate on
    stack.push_back(Item(nc.root,nullptr,0,0,0,PROPER));
}

bool Iterator::next()
{
    if (stack.empty()) {
        current_item_ptr = nullptr;
        return false;
    }
    current_item = stack.back();
    current_item_ptr = &current_item;

    stack.pop_back();

    // if level == 0 and link_type == PROPER
    if (current_item.link_type == PROPER) {

        // add content
        if (current_item.dimension < nanocube.dimension-1) {
            stack.push_back(Item(current_item.node->getContentAsNode(),
                                 current_item.node,
                                 0, // label is not applicable here
                                 current_item.dimension+1,
                                 0,
                                 current_item.node->getContentType()));
        }

        // add children
        for (auto &pcl: current_item.node->children) {
            stack.push_back(Item(pcl.child,
                                 current_item.node,
                                 pcl.label,
                                 current_item.dimension,
                                 current_item.level+1,
                                 pcl.link_type));
        }
    }
    return true;
}

bool Iterator::lastDimension() const
{
    if (current_item_ptr) {
        return current_item.dimension == nanocube.dimension-1;// && current_item.level == nanocube.levels.back()-1;
    }
    else {
        return false;
    }
}
