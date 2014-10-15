#pragma once

#include <algorithm>
#include <iostream>
#include <vector>

#include "ContentHolder.hh"

#include "cache.hh"

#include "polycover/labeled_tree.hh"

//-----------------------------------------------------------------------------
// DECLARATIONS
//-----------------------------------------------------------------------------

namespace flattree_n {
    
    using DimensionPath = std::vector<int>; // matching tree_store_nanocube.hh
    
    using Mask = polycover::labeled_tree::Node;
    
    using Cache = nanocube::Cache;

using PathSize   = uint8_t;
using Level      = int32_t;

using NumBytes   = uint8_t;
using RawAddress = uint64_t; // covers all cases and it is never stored

//--------------------------------------------------------------------
// Address
//--------------------------------------------------------------------

template <typename Structure>
struct Address {
public:
    static const RawAddress Root = ~0UL >> (8 - Structure::Size) * 8;
public:

    Address();

    explicit Address(uint64_t raw_address);


    PathSize getPathSize() const;
    bool     isEmpty()     const;

    uint64_t raw()         const; // return raw address

    bool read(std::istream &is);

    DimensionPath getDimensionPath() const;
    
    
//    bool isEmpty() const;

//    bool operator==(const Address &addr) const;
//    bool operator< (const Address &addr) const;

    // new stuff (to enable composition on a nested LOD structure)
    // PathElement operator[](PathIndex index) const;
    // PathSize    getPathSize() const;
    // size_t hash() const;

    RawAddress raw_address;

};

//--------------------------------------------------------------------
// Iterator
//--------------------------------------------------------------------

// Iterate through all parent-child relations
template <typename Structure>
struct Iterator {

public: // constants
    static const bool SHARED = true;
    static const bool PROPER = false;

public: // subtypes
    using tree_type = Structure;
    using node_type = typename Structure::NodeType;

public: // constructor
    Iterator(const tree_type &tree);

public: // methods

    bool next();

    auto getCurrentNode() const -> const node_type*;
    auto getCurrentParentNode() const -> const node_type*;

    std::string getLabel() const;

    bool isShared() const;
    bool isProper() const;

    int getCurrentLevel() const;

public:

    const tree_type &tree;

    const node_type *current_node         { nullptr };
    const node_type *current_parent_node  { nullptr };

    bool current_flag { PROPER };

    int current_level {  0 };
    int current_index { -1 };

    std::string current_label;

};


//--------------------------------------------------------------------
// Node
//--------------------------------------------------------------------

using NumChildren = uint32_t;

enum NodeType { LINK=1, FLATTREE=2 };

template <NumBytes N, typename Content>
struct Node: public contentholder::ContentHolder<Content> {
    NumChildren getNumChildren() const;
    NodeType    getNodeType() const;
protected:
    Node(NodeType type); // node cannot be created
};

//--------------------------------------------------------------------
// Link
//--------------------------------------------------------------------

template <typename Structure>
struct Link: public Node<Structure::Size, typename Structure::ContentType>
{
    using NodeType = Node<Structure::Size, typename Structure::ContentType>;

    Link();
    Link(RawAddress addr);
    // Node<Content> &asNode();

    uint64_t getLabel() const {
        uint64_t label = 0;
        std::copy(&raw_address[0], &raw_address[Structure::Size], (char*) &label);
        return label;
    }

    inline RawAddress getRawAddress() const;

    char raw_address[Structure::Size]; // raw bytes encoding label
};


//--------------------------------------------------------------------
// FlatTree
//--------------------------------------------------------------------

template <NumBytes N, typename Content>
struct FlatTree: public Node<N, Content> {

public: // constants

    static const int Size = N;


public: // subtypes

    using ContentType   = Content;
    using NodeType      = Node<Size, ContentType>;
    using LinkType      = Link<FlatTree>;
    using AddressType   = Address<FlatTree>;
    using NodeStackType = std::vector<NodeType*>;
    using IteratorType  = Iterator<FlatTree>;

public: // methods

    FlatTree();

    ~FlatTree();

    auto getRoot() -> NodeType*;

    auto getLink(RawAddress raw_address, bool create_if_not_found) -> LinkType*;

    auto makeLazyCopy() const -> FlatTree*;

    auto find(const AddressType &addr) -> NodeType*;

    void prepareProperOutdatedPath(FlatTree*             parallel_structure,
                                   AddressType           address,
                                   std::vector<void*>&   parallel_replaced_nodes,
                                   NodeStackType&        stack);


    //
    void dump(std::ostream& os);

    // visit all subnodes of a certain node in the requested target level.
    template <typename Visitor>
    void visitSubnodes(AddressType address, Level targetLevelOffset, Visitor &visitor);

    // visit all subnodes of a certain node in the requested range.
    template <typename Visitor>
    void visitRange(AddressType min_address, AddressType max_address, Visitor &visitor);

    // polygon visit (cache first preprocessing)
    template <typename Visitor>
    void visitSequence(const std::vector<RawAddress> &seq, Visitor &visitor, Cache& cache);
    
    template <typename Visitor>
    void visitExistingTreeLeaves(const Mask* mask, Visitor &visitor);


    std::vector<LinkType> links; // TODO: replace with something more space efficient (3 pointers in here)

};

} //











//-----------------------------------------------------------------------------
// DEFINITIONS
//-----------------------------------------------------------------------------

// separate the files of the declaration and definition
// #include <FlatTreeN_impl.hh>

namespace flattree_n {

//-----------------------------------------------------------------------------
// Address Impl.
//-----------------------------------------------------------------------------

template<typename Structure>
Address<Structure>::Address():
    raw_address { Root }
{}

template<typename Structure>
Address<Structure>::Address(uint64_t raw_address):
    raw_address { raw_address }
{}

template<typename Structure>
PathSize Address<Structure>::getPathSize() const
{
    return raw_address == Root ? 0 : 1;
}

template<typename Structure>
uint64_t Address<Structure>::raw() const
{
    return raw_address;
}

template<typename Structure>
bool Address<Structure>::isEmpty() const
{
    return raw_address == Root;
}

template<typename Structure>
bool Address<Structure>::read(std::istream &is)
{
    raw_address = 0;
    is.read((char*) &raw_address, Structure::Size);
    if (!is) {
        return false;
    }
    else {
        return true;
    }
}

    template<typename Structure>
    DimensionPath Address<Structure>::getDimensionPath() const {
        DimensionPath result;
        if (raw_address != Root) {
            result.push_back((int)raw_address);
        }
        return result;
    }

//-----------------------------------------------------------------------------
// Node Impl.
//-----------------------------------------------------------------------------

template<NumBytes N, typename Content>
Node<N, Content>::Node(NodeType type):
    contentholder::ContentHolder<Content>()
{
    this->setUserData(type);
}

template <NumBytes N, typename Content>
auto Node<N,Content>::getNumChildren() const -> NumChildren
{
    using FlatTree = FlatTree<N,Content>;

    if (getNodeType() == LINK)
        return 0;
    else // flattree
        return (reinterpret_cast<const FlatTree*>(this))->links.size();
}

template <NumBytes N, typename Content>
auto Node<N, Content>::getNodeType() const -> NodeType
{
    return (NodeType) this->getUserData();
}

//-----------------------------------------------------------------------------
// Link Impl.
//-----------------------------------------------------------------------------

template <typename Structure>
Link<Structure>::Link():
    NodeType(LINK)
{}

template <typename Structure>
Link<Structure>::Link(RawAddress addr):
    NodeType(LINK)
{
    char* ptr = (char*) &addr; // (char*) &raw_address[0];
    std::copy(ptr, ptr + Structure::Size, (char*) &raw_address);
}

template <typename Structure>
RawAddress Link<Structure>::getRawAddress() const
{
    RawAddress result = 0;
    char* ptr = (char*) &raw_address[0];
    std::copy(ptr, ptr + Structure::Size, (char*) &result);
    return result;
}

//-----------------------------------------------------------------------------
// FlatTree Impl.
//-----------------------------------------------------------------------------

template<NumBytes N, typename Content>
FlatTree<N, Content>::FlatTree():
    NodeType(FLATTREE)
{}

template<NumBytes N, typename Content>
auto FlatTree<N, Content>::find(const FlatTree::AddressType &addr) -> NodeType*
{
    if (addr.isEmpty()) {
        return this;
    }
    else {
        return this->getLink(addr.raw_address, false);
    }
}

template<NumBytes N, typename Content>
void FlatTree<N, Content>::prepareProperOutdatedPath(FlatTree*                parallel_structure,
                                                     FlatTree::AddressType    address,
                                                     std::vector<void *>&     parallel_replaced_nodes,
                                                     FlatTree::NodeStackType& stack)
{
    // same implementation as trailProperPath
    // there is no gain on a flattree to share
    // child nodes.

    // needs to be a complete path
    if (address.getPathSize() != 1)
        throw std::runtime_error("Invalid Path Size");

    // to get to this point at least the root needs
    // to be updated, otherwise it would have been
    // detected before
    stack.push_back(this);
    // parallel_replaced_nodes.push_back(this);


    if (parallel_structure) {
        auto parallel_child = parallel_structure->getLink(address.raw_address, false);

        bool needs_to_update_child = true;

        // get child. maybe doesn't need to be updated...
        NodeType *child = this->getLink(address.raw_address, false);
        if (child == nullptr) {
            child = this->getLink(address.raw_address, true);
            child->setSharedContent(parallel_child->getContent());
            needs_to_update_child = false;
        }
        else if (parallel_child->getContent() == child->getContent()){
            // nothing to be done: content already updated
            needs_to_update_child = false;
        }

        stack.push_back(child);
        if (needs_to_update_child) {
            stack.push_back(nullptr);
//            return child;
        }
//        else {
//            std::cout << "Special case: saving resources!!" << std::endl;
//            return this;
//        }
    }

    else {
        NodeType *child = this->getLink(address.raw_address, true);
        stack.push_back(child);
        stack.push_back(nullptr);
//        return child;
    }

//    // all nodes in the flattree_n implementation
//    // are proper, so it is a simple matter of
//    // pushing the root and a (possibly new) child node.

//    assert(address.getPathSize()<=1);

//    // add root
//    stack.push_back(this);

//    if (address.getPathSize() == 1)
//    {
//        NodeType* child = this->getLink(address.raw_address, true);
//        stack.push_back(child); // add root
//        return child;
//    }

//    return this;
}



template <NumBytes N, typename Content>
template <typename Visitor>
void FlatTree<N, Content>::visitSubnodes(AddressType address, Level targetLevelOffset, Visitor &visitor)
{
//    Level targetLevel = (address.isEmpty() ? 0 : 1) + targetLevelOffset;
//    assert(targetLevel <= 1);

    NodeType *node = find(address);
    if (!node) {
        return; // no node fits the bill
    }

    if (targetLevelOffset == 0) {
        visitor.visit(node, address);
    }
    else if (targetLevelOffset == 1) {
        // loop
        for (LinkType &link: this->links) {
            visitor.visit(static_cast<NodeType*>(&link), AddressType(link.getRawAddress()));
        }
    }
}

template <NumBytes N, typename Content>
template <typename Visitor>
void FlatTree<N, Content>::visitRange(AddressType min_address, AddressType max_address, Visitor &visitor)
{
    for (RawAddress e=min_address.raw();e<=max_address.raw();e++)
    {
        // Inefficient: search for the first in the range then iterate
        // but I guess this query is not being used anyway...
        auto addr = AddressType(e);
        NodeType *node = find(addr);
        if (node)
            visitor.visit(node, addr);
    }
}

template <NumBytes N, typename Content>
template <typename Visitor>
void FlatTree<N, Content>::visitSequence(const std::vector<RawAddress> &seq, Visitor &visitor, Cache& cache)
{
    for (auto raw_address: seq) {
        this->visitSubnodes(AddressType(raw_address),0,visitor);
    }
    // throw std::runtime_error("visitSequence not supported");
}
    
    
    template<NumBytes N, typename Content>
    template <typename Visitor>
    void FlatTree<N,Content>::visitExistingTreeLeaves(const Mask* mask, Visitor &visitor) {
        throw std::runtime_error("not available");
    }


template<NumBytes N, typename Content>
auto FlatTree<N, Content>::getLink(RawAddress raw_address, bool create_if_not_found) -> LinkType*
{
    auto compare = [](const Link<FlatTree>& link, RawAddress raw_address) {
        return (link.getRawAddress() < raw_address);
    };

    auto it = std::lower_bound(links.begin(),links.end(),raw_address,compare);
    if (it != links.end() && it->getRawAddress() == raw_address)
    {
        return const_cast<LinkType*>(&*it);
    }
    else
    {
        if (!create_if_not_found)
            return nullptr;
        else
        {
            // count_entries++; // global count of nodes of level 1
            auto it2 = links.insert(it, LinkType(raw_address));
            return const_cast<LinkType*>(&*it2);
        }
    }
}

template <NumBytes N, typename Content>
auto FlatTree<N, Content>::makeLazyCopy() const -> FlatTree*
{
    FlatTree *copy = new FlatTree();

    copy->links.resize(links.size());
    std::copy(links.begin(), links.end(), copy->links.begin());
    for (auto &link: copy->links)
        link.setSharedContent(link.getContent()); // mark as shared instead of proper

    copy->setSharedContent(this->getContent());

    return copy;
}


template <NumBytes N, typename Content>
FlatTree<N, Content>::~FlatTree()
{
    // delete content of children nodes
    for (LinkType &link: this->links) {
        if (link.contentIsProper()) {
            delete link.getContent();
        }
    }

    // delete self content
    if (this->contentIsProper()) {
        delete this->getContent();
    }

    //    std::cerr << "~FlatTree " << this << std::endl;
}

template <NumBytes N, typename Content>
auto FlatTree<N, Content>::getRoot() -> NodeType*
{
    return this;
}

template <NumBytes N, typename Content>
void FlatTree<N, Content>::dump(std::ostream& os)
{
    os << "FlatTree, tag: "
       << (int) this->data.getTag()
       << " content: "
       << static_cast<void*>(this->data.getPointer())
       << std::endl;

    for (auto &l: links)
        os << "   Link, label: "
           << (RawAddress) l.getRawAddress()
           << ", tag: "
           << (int) l.data.getTag()
           << " content: "
           << static_cast<void*>(l.data.getPointer())
           << std::endl;
}



//-----------------------------------------------------------------------------
// Iterator
//-----------------------------------------------------------------------------

template <typename Structure>
Iterator<Structure>::Iterator(const tree_type& tree):
    tree(tree)
{}

template <typename Structure>
bool Iterator<Structure>::next() {
    current_index++;
    if (current_level == 0) {
        if  (current_index==0) {
            current_node = &tree;
            return true;
        }
        else if (current_index == 1) {
            current_parent_node = &tree;
            current_level = 1;
            current_index = 0;
        }
    }

    // only gets here if it is on level 1
    const int num_links = tree.links.size();
    if (current_index >= num_links) {
        current_node = nullptr;
        return false;
    }
    else {
        // tree.links[current_index].
//        current_label = std::to_string(current_index);
//        current_node = &tree.links[current_index];
        auto &link = tree.links[current_index];
        current_label = std::to_string(link.getLabel());
        current_node = &link;
        return true;
    }
}

template <typename Structure>
auto Iterator<Structure>::getCurrentNode() const -> const node_type* {
    return current_node;
}

template <typename Structure>
auto Iterator<Structure>::getCurrentParentNode() const -> const node_type* {
    return current_parent_node;
}

template <typename Structure>
std::string Iterator<Structure>::getLabel() const {
    return current_label;
}

template <typename Structure>
int Iterator<Structure>::getCurrentLevel() const {
    return current_level;
}

template <typename Structure>
bool Iterator<Structure>::isShared() const {
    return current_flag == SHARED;
}

template <typename Structure>
bool Iterator<Structure>::isProper() const {
    return current_flag == PROPER;
}

} // flattree_n
