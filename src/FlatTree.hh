#pragma once

#include <algorithm>
#include <vector>
#include <cassert>
#include <iostream>
#include <stack>
#include <cstdint>
#include <stdexcept>

#ifndef FLATTREE_VECTOR
#include "small_vector.hh"
#endif

#include "ContentHolder.hh"

#include "cache.hh"
#include "polycover/labeled_tree.hh"

//
// Needed Mechanisms
//
// 1. makeLazyCopy for a FlatTree
// 2. trailProperPath on a FlatTree
// 3. getContentCreateIfNeeded for a Node
// 4. check if the content of a Node is proper
// 5. after a lazy copy the content of all FlatTree nodes is not proper
//

namespace flattree
{
    
    using DimensionPath = std::vector<int>; // matching tree_store_nanocube.hh
    
    using Mask = polycover::labeled_tree::Node;
    
        using Cache = nanocube::Cache;

typedef unsigned char PathSize;
typedef unsigned char PathIndex;
typedef unsigned char PathElement;

typedef unsigned char NumChildren;

typedef int32_t       Level;
typedef uint64_t      Count;

typedef uint64_t      RawAddress;

using contentholder::ContentHolder;

//-----------------------------------------------------------------------------
// Forward Declarations
//-----------------------------------------------------------------------------

template <typename Content>
struct Node;

template <typename Content>
struct Iterator;

template <typename Content>
struct FlatTree;

//-----------------------------------------------------------------------------
// Address
//-----------------------------------------------------------------------------

template <typename Structure>
struct Address
{
public: // subtypes and constants

    typedef Structure StructureType;

    // this is a special PathElement value used to indicate
    // an empty path (path of the root)
    static const PathElement EMPTY_PATH = (PathElement) 0xff;

public: // constructors

    Address() = default;

    explicit Address(uint64_t raw_address);

    Address(PathElement p);

public:  // methods

    PathSize getPathSize() const;
    bool     isEmpty() const;

    uint64_t raw() const; // return raw address

    bool read(std::istream &is);

    PathElement operator[](PathIndex index) const;

    bool operator<(const Address &addr) const;
    bool operator==(const Address &addr) const;

    size_t hash() const;
    
    DimensionPath getDimensionPath() const;

public:  // data members

    //! a path in this context of flattree contains at most one element
    PathElement singleton_path_element { EMPTY_PATH };

};


template<typename Structure>
std::ostream& operator<<(std::ostream &os, const Address<Structure>& addr);

//-----------------------------------------------------------------------------
// Node
//-----------------------------------------------------------------------------

typedef uint8_t NodeType;

template <typename Content>
struct Node: public ContentHolder<Content>
{
    static const NodeType LINK     = 1;
    static const NodeType FLATTREE = 2;

    NumChildren getNumChildren() const;
    NodeType    getNodeType() const;

protected:
    Node(NodeType type); // node cannot be created
};

//-----------------------------------------------------------------------------
// Link
//-----------------------------------------------------------------------------

template <typename Content>
struct Link: public Node<Content>
{
    Link();
    Link(PathElement label);

    Node<Content> &asNode();

    PathElement    label;
};


//-----------------------------------------------------------------------------
// FlatTree
//-----------------------------------------------------------------------------

template <typename Content>
struct FlatTree: public Node<Content>
{
public:
    typedef FlatTree<Content>              Type;
    typedef Node<Content>                  NodeType;
    typedef Address<Type>                  AddressType;
    typedef Content                        ContentType;
    typedef std::vector<NodeType*>         NodeStackType;
    typedef Iterator<Content>              IteratorType;

#if 0
public: // static services for allocation and memory usage count

    static FlatTree* create(); // use this as a factory
    static void dump_ftlist(std::ostream &os);

    static Count mem(); // memory usage in bytes of all create flattrees
    static Count num(); // number of created FlatTrees

    // data
    static std::vector<FlatTree*> ftlist;
#endif

public:

    static void* operator new(size_t size);
    static void  operator delete(void *p);

    static uint64_t count_new;
    static uint64_t count_delete;
    static uint64_t count_entries; // number of total level 1 nodes across all flattrees

public:

    NumChildren getNumChildren() const;

    FlatTree   *makeLazyCopy() const;

    Count       getMemoryUsage() const;

    NodeType* getRoot();

    NodeType* trailProperPath(AddressType addr, NodeStackType &stack);

    void      prepareProperOutdatedPath(FlatTree*             parallel_structure,
                                        AddressType           address,
                                        std::vector<void*>&   parallel_replaced_nodes,
                                        NodeStackType&        stack);

    Node<Content>* find(AddressType &addr);

    void dump(std::ostream& os);

    // visit all subnodes of a certain node in the
    // requested target level.
    template <typename Visitor>
    void visitSubnodes(AddressType address, Level targetLevelOffset, Visitor &visitor);

    // visit all subnodes of a certain node in the
    // requested target level.
    template <typename Visitor>
    void visitRange(AddressType min_address, AddressType max_address, Visitor &visitor);

    // polygon visit (cache first preprocessing)
    template <typename Visitor>
    void visitSequence(const std::vector<RawAddress> &seq, Visitor &visitor, Cache& cache);

    template <typename Visitor>
    void visitExistingTreeLeaves(const Mask* mask, Visitor &visitor);

    FlatTree();
    ~FlatTree();

private:


    Link<Content>* getLink(PathElement e, bool create_if_not_found=false);

//    Node<Content>* addChild(PathElement label);
//    Node<Content>* getProperChildCreateIfNeed(PathElement e); //


public:

#ifndef FLATTREE_VECTOR
    small_vector::small_vector<Link<Content> > links;
#else
    std::vector<Link<Content> > links;
#endif

};

//-----------------------------------------------------------------------------
// Iterator
//-----------------------------------------------------------------------------

// Iterate through all parent-child relations
template <typename Content>
struct Iterator {

public: // constants
    static const bool SHARED = true;
    static const bool PROPER = false;

public: // subtypes
    typedef FlatTree<Content>                    tree_type;
    typedef typename FlatTree<Content>::NodeType node_type;

public: // constructor
    Iterator(const tree_type &tree);

public: // methods

    bool next();

    const node_type* getCurrentNode() {
        return current_node;
    }

    const node_type* getCurrentParentNode() {
        return current_parent_node;
    }

    int getCurrentLevel() const {
        return current_level;
    }

    std::string getLabel() const {
        return current_label;
    }

    bool isShared() const {
        return current_flag == SHARED;
    }

    bool isProper() const {
        return current_flag == PROPER;
    }

public:

    const tree_type &tree;

    const node_type *current_node        { nullptr };
    const node_type *current_parent_node { nullptr };

    bool current_flag                    { PROPER  }; // that is a property of flat trees

    int current_level                    {  0 };
    int current_index                    { -1 };

    std::string current_label;

};

//
// checkout default values on declaration they are
// important for sync purposes.
//
template <typename Content>
Iterator<Content>::Iterator(const tree_type& tree):
    tree(tree)
{}

template <typename Content>
bool Iterator<Content>::next() {
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
    auto num_links = tree.links.size();
    if (current_index >= num_links) {
        current_node = nullptr;
        return false;
    }
    else {
        // tree.links[current_index].
        auto &link = tree.links[current_index];
        current_label = std::to_string(link.label);
        current_node = &link;
        return true;
    }
}

//-----------------------------------------------------------------------------
// Output
//-----------------------------------------------------------------------------

template <typename Content>
std::ostream& operator<<(std::ostream &o, const FlatTree<Content>& ft);

template<typename Content>
std::ostream& operator<<(std::ostream &o, const Link<Content>& ts);

//-----------------------------------------------------------------------------
// Impl. Address Template Members
//-----------------------------------------------------------------------------

//template <typename Structure>
//Address<Structure>::Address():
//    singleton_path_element(EMPTY_PATH)
//{}

template <typename Structure>
Address<Structure>::Address(PathElement p):
    singleton_path_element(p)
{}

template <typename Structure>
Address<Structure>::Address(uint64_t raw_address):
    singleton_path_element((PathElement) raw_address)
{}

template <typename Structure>
uint64_t Address<Structure>::raw() const {
    return (uint64_t) singleton_path_element;
}
    
    template<typename Structure>
    DimensionPath Address<Structure>::getDimensionPath() const {
        DimensionPath result;
        if (singleton_path_element != EMPTY_PATH) {
            result.push_back((int)singleton_path_element);
        }
        return result;
    }


template <typename Structure>
bool Address<Structure>::read(std::istream &is)
{
    is.read((char*) &singleton_path_element,sizeof(PathElement));
    if (!is) {
        return false;
    }
    else {
        return true;
    }
}

template <typename Structure>
PathSize Address<Structure>::getPathSize() const
{
    return (singleton_path_element == EMPTY_PATH ? 0 : 1);
}

template <typename Structure>
bool Address<Structure>::isEmpty() const
{
    return (singleton_path_element == EMPTY_PATH);
}

template <typename Structure>
inline bool Address<Structure>::operator<(const Address<Structure> &addr) const
{
    return (singleton_path_element == EMPTY_PATH && addr.singleton_path_element != EMPTY_PATH) ||
            (singleton_path_element < addr.singleton_path_element);
}

template <typename Structure>
inline bool Address<Structure>::operator==(const Address<Structure> &addr) const
{
    return (singleton_path_element == addr.singleton_path_element);
}

template<typename Structure>
inline size_t Address<Structure>::hash() const
{
    return singleton_path_element;
}

template <typename Structure>
PathElement Address<Structure>::operator[](PathIndex index) const
{
    if (index == 0)
    {
        assert(singleton_path_element != EMPTY_PATH);
        return singleton_path_element;
    }
    assert(0);
}

//----------------------------------------------------------------------------
// Node Impl.
//----------------------------------------------------------------------------

template <typename Content>
Node<Content>::Node(NodeType type):
    ContentHolder<Content>()
{
    this->setUserData(type);
}

template <typename Content>
NumChildren
Node<Content>::getNumChildren() const
{
    using FlatTree = FlatTree<Content>;
    if (getNodeType() == Node<Content>::LINK)
        return 0;
    else // flattree
        return (reinterpret_cast<const FlatTree*>(this))->links.size();
}

template <typename Content>
NodeType
Node<Content>::getNodeType() const
{
    return this->getUserData();
}

//-----------------------------------------------------------------------------
// Impl. Link Template Memebers
//-----------------------------------------------------------------------------

template <typename Content>
Link<Content>::Link():
    Node<Content>(Node<Content>::LINK),
    label(0)
{}

template <typename Content>
Link<Content>::Link(PathElement label):
    Node<Content>(Node<Content>::LINK),
    label(label)
{}

template <typename Content>
Node<Content> &Link<Content>::asNode()
{
    return static_cast<Node<Content>&>(*this);
}

//-----------------------------------------------------------------------------
// Impl. FlatTree Template Members
//-----------------------------------------------------------------------------

template <typename Content>
uint64_t FlatTree<Content>::count_new = 0;

template <typename Content>
uint64_t FlatTree<Content>::count_delete = 0;

template <typename Content>
uint64_t FlatTree<Content>::count_entries = 0;

template <typename Content>
void* FlatTree<Content>::operator new(size_t size) {
    count_new++;
    return ::operator new(size);
}

template <typename Content>
void FlatTree<Content>::operator delete(void *p) {
    count_delete++;
    ::operator delete(p);
}

//
// The notion here is of preparing a path that
// is completely owned by the current flattree.
// The idea is that a "message" (new data point)
// is going to be sent to all the contents of the
// given path.
//
template <typename Content>
Node<Content>*
FlatTree<Content>::trailProperPath(AddressType addr, FlatTree::NodeStackType &stack)
{
    assert(addr.getPathSize()<=1);

    // add root
    stack.push_back(this);

    if (addr.getPathSize() == 1)
    {
        Node<Content> *child = this->getLink(addr.singleton_path_element, true);
        stack.push_back(child); // add root
        return child;
    }

    return this;

}


template <typename Content>
void
FlatTree<Content>::prepareProperOutdatedPath(FlatTree*                  parallel_structure,
                                             FlatTree::AddressType      address,
                                             std::vector<void*>&        parallel_replaced_nodes,
                                             FlatTree::NodeStackType&   stack)
{
    //std::cout << "FlatTree<Content>::prepareProperOutdatedPath(...): address == " << address << std::endl;
    
    // same implementation as trailProperPath
    // there is no gain on a flattree to share
    // child nodes.

    // needs to be a complete path
    if (address.getPathSize() != 1)
        throw std::runtime_error("Invalid Path Size");
    
    // std::cout << "FlatTree::prepareProperOutdatedPath(...): address == " << address << std::endl;

    // to get to this point at least the root needs
    // to be updated, otherwise it would have been
    // detected before
    stack.push_back(this);
    // parallel_replaced_nodes.push_back(this);


    if (parallel_structure) {
        auto parallel_child = parallel_structure->getLink(address.singleton_path_element, false);

        bool needs_to_update_child = true;

        // get child. maybe doesn't need to be updated...
        Node<Content> *child = this->getLink(address.singleton_path_element, false);
        if (child == nullptr) {
            child = this->getLink(address.singleton_path_element, true);
            child->setSharedContent(parallel_child->getContent());
            needs_to_update_child = false;
        }
        else if (parallel_child->getContent() == child->getContent()){
            // nothing to be done: content already updated
            needs_to_update_child = false;
        }

        // a third case might occur here:
        // parallel_child exists, but it is not the same as current child
        // in this case there is a need to update structure
        //
        // check when inserting third point on
        // a b c-- t count
        // 2 1 0 1 0 1
        // 0 0 0 1 1 1
        // 1 1 0 1 2 1
        //

        stack.push_back(child);
        if (needs_to_update_child) {
            stack.push_back(nullptr);
            // return child;
        }
//        else {
//             std::cout << "Special case: saving resources!!" << std::endl;
//             return this;
//        }
    }

    else {
        Node<Content> *child = this->getLink(address.singleton_path_element, true);
        stack.push_back(child);
        stack.push_back(nullptr);
        // return child;
    }
}

//
// The notion here is of preparing a path that
// is completely owned by the current flattree.
// The idea is that a message is going to be
// sent to all the contents of the given path.
//
template <typename Content>
Node<Content>*
FlatTree<Content>::find(AddressType &addr)
{
    assert(addr.getPathSize()<=1);

    // add root
    if (addr.getPathSize() == 0)
    {
        return this;
    }
    else // if (addr.getPathSize() == 1)
    {
        Node<Content> *child = this->getLink(addr.singleton_path_element, false);
        return child;
    }
}

template <typename Content>
template <typename Visitor>
void FlatTree<Content>::visitSubnodes(AddressType address, Level targetLevelOffset, Visitor &visitor)
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
    else if (address.isEmpty() && targetLevelOffset == 1) {
        // loop
        for (Link<Content> &link: this->links) {
            AddressType addr(link.label);
            visitor.visit(static_cast<NodeType*>(&link), addr);
        }
    }
}

#if 0
//
// Report each link once
//
template <typename Content>
template <typename Visitor>
void FlatTree<Content>::visitAllNodes(Visitor &visitor)
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
    else if (address.isEmpty() && targetLevelOffset == 1) {
        // loop
        for (Link<Content> &link: this->links) {
            AddressType addr(link.label);
            visitor.visit(static_cast<NodeType*>(&link), addr);
        }
    }
}
#endif


template <typename Content>
template <typename Visitor>
void FlatTree<Content>::visitRange(AddressType min_address, AddressType max_address, Visitor &visitor)
{
    for (PathElement e=min_address.singleton_path_element;e<=max_address.singleton_path_element;e++)
    {
        AddressType addr(e);
        NodeType *node = find(addr);
        if (node)
            visitor.visit(node, addr);
    }
}


// polygon visit (cache first preprocessing)
template <typename Content>
template <typename Visitor>
void FlatTree<Content>::visitSequence(const std::vector<RawAddress> &seq, Visitor &visitor, Cache& cache) {
    for (auto raw_address: seq) {
        this->visitSubnodes(AddressType(raw_address),0,visitor);
    }
}

    template<typename Content>
    template <typename Visitor>
    void FlatTree<Content>::visitExistingTreeLeaves(const Mask* mask, Visitor &visitor) {
        throw std::runtime_error("not available");
    }

//
// Node Implementation
//

template <typename Content>
FlatTree<Content>::FlatTree(): Node<Content>(Node<Content>::FLATTREE)
{}

template <typename Content>
FlatTree<Content>::~FlatTree()
{
    // delete content of children nodes
    for (Link<Content> &link: this->links) {
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

template <typename Content>
void FlatTree<Content>::dump(std::ostream& os)
{
    os << "FlatTree, tag: "
       << (int) this->data.getTag()
       << " content: "
       << static_cast<void*>(this->data.getPointer())
       << std::endl;

    for (auto &l: links)
        os << "   Link, label: "
           << (int) l.label
           << ", tag: "
           << (int) l.data.getTag()
           << " content: "
           << static_cast<void*>(l.data.getPointer())
           << std::endl;
}


template <typename Content>
FlatTree<Content>*
FlatTree<Content>::makeLazyCopy() const
{
    FlatTree<Content> *copy = new FlatTree<Content>();

    copy->setSharedContent(this->getContent()); // TODO: check the semantics of the lazy copy
                                                // in regards to the content

    count_entries += links.size();
    copy->links.resize(links.size());
    std::copy(links.begin(), links.end(), copy->links.begin());
    for (auto &link: copy->links)
        link.setSharedContent(link.getContent()); // mark as shared instead of proper

    return copy;
}

template <typename Content>
inline bool compare_links(const Link<Content> &a, const Link<Content> &b)
{
    return (a.label < b.label);
}

template <typename Content>
Link<Content> *
FlatTree<Content>::getLink(PathElement e, bool create_if_not_found)
{
    Link<Content> link(e);
    auto it = std::lower_bound(links.begin(),links.end(),link,compare_links<Content>);
    if (it != links.end() && it->label == e)
    {
        return const_cast<Link<Content>*>(&*it);
    }
    else
    {
        if (!create_if_not_found)
            return nullptr;
        else
        {
            count_entries++; // global count of nodes of level 1

            Link<Content> aux(e);
            auto it2 = links.insert(it, aux);
            return const_cast<Link<Content>*>(&*it2);

        }
    }
}

template <typename Content>
Count FlatTree<Content>::getMemoryUsage() const
{
    Count result = sizeof(FlatTree<Content>);
    result += links.size() * sizeof(Link<Content>);
//    for (auto &link: links)
//        if (link.proper)
//            result += link.node->getMemoryUsage();
    return result;
}

template <typename Content>
Node<Content> *FlatTree<Content>::getRoot()
{
    return this;
}

//-----------------------------------------------------------------------------
// Output
//-----------------------------------------------------------------------------

template <typename Content>
std::ostream& operator<<(std::ostream &o, const FlatTree<Content>& ft)
{
    o << "[flattree: " << ft.getNumChildren() << "] ";
    return o;
}

template<typename Content>
std::ostream& operator<<(std::ostream &o,
                         const Link<Content>& ts)
{
    o << "[Link: " << static_cast<int>(ts.label) << "] ";
    return o;
}

template<typename Structure>
std::ostream& operator<<(std::ostream &os, const Address<Structure>& addr)
{
    std::string st = (addr.isEmpty() ? std::string("empty") : std::to_string((int)addr.singleton_path_element));
    os << "Addr["  << st << "]";
    return os;
}

}
