#include "QueryResult.hh"

#include <cassert>
#include <algorithm>

#include "QuadTree.hh"


namespace query {

namespace result {

#if 0
//-----------------------------------------------------------------------------
// ResultNode
//-----------------------------------------------------------------------------

ResultNode::ResultNode(ResultNode *parent, int dimension, RawAddress address):
    parent(parent),
    dimension(dimension),
    address(address),
    value(0)
{}

ResultNode::ResultNode():
    parent(nullptr),
    dimension(BASE_DIMENSION),
    address(0),
    value(0)
{}

ResultNode* ResultNode::getChild(int dim, RawAddress addr) {

    assert(dim > this->dimension);

    auto it = std::find_if(children.begin(), children.end(),
                           [&dim, &addr](const ResultNode *p) {
        return p->address == addr && p->dimension == dim;
    });

    ResultNode* child = nullptr;
    if (it == children.end()) {
        child = new ResultNode(this, dim, addr);
        this->children.push_back(child);
    }
    else {
        child = *it;
    }
    return child;

}

std::ostream &operator<<(std::ostream &os, const ResultNode &node) {

    std::stack<const ResultNode*> stack;

    const ResultNode *u = &node;
    while (u->dimension != ResultNode::BASE_DIMENSION) {
        stack.push(u);
        u = u->parent;
    }

    typedef quadtree::Address<29,int> QAddr;

    while (!stack.empty()) {
        u = stack.top();
        stack.pop();

        os << "[dim=" << u->dimension << " addr=" << u->address << " qaddr=" << QAddr(u->address) << "] ";
    }

    os << " -> " << node.value;

    return os;
}

//-----------------------------------------------------------------------------
// Result
//-----------------------------------------------------------------------------

Result::Result():
    current_node(&root)
{}

void Result::push(int dimension, RawAddress addr) {
    ResultNode* child = current_node->getChild(dimension, addr);
    this->current_node = child;
}

void Result::pop() {
    current_node = current_node->parent;
    assert(current_node != nullptr);
}

void Result::accum(uint64_t value) {
    current_node->value += value;
}

std::ostream &operator<<(std::ostream &os, const Result &result) {

    std::stack<const ResultNode*> stack;
    stack.push(&result.root);

    while (!stack.empty()) {
        const ResultNode* u = stack.top();
        stack.pop();
        if (u->children.size() == 0) { // all leaves
            if (u->value > 0) {
                os << *u << std::endl;
            }
        }
        else {
            for (ResultNode* v: u->children) {
                stack.push(v);
            }
        }
    }
    return os;
}
#endif

} // namespace result

} // namespace query
