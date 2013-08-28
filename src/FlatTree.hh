#pragma once

#include <algorithm>
#include <vector>
#include <cassert>
#include <iostream>
#include <stack>
#include <cstdint>

#ifndef FLATTREE_VECTOR
#include <small_vector.hh>
#endif

#include <ContentHolder.hh>

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

typedef unsigned char PathSize;
typedef unsigned char PathIndex;
typedef unsigned char PathElement;

typedef unsigned char NumChildren;

typedef int32_t       Level;
typedef uint64_t      Count;

using contentholder::ContentHolder;

//-----------------------------------------------------------------------------
// Forward Declarations
//-----------------------------------------------------------------------------

template <typename Content>
struct Node;

template <typename Content>
struct FlatTree;

//-----------------------------------------------------------------------------
// Address
//-----------------------------------------------------------------------------

template <typename Structure>
struct Address
{
    typedef Structure StructureType;

    // this is a special PathElement value used to indicate
    // an empty path (path of the root)
    static const PathElement EMPTY_PATH = (PathElement) 0xff;

    Address();
    Address(PathElement p);
    PathSize getPathSize() const;
    bool     isEmpty() const;

    PathElement operator[](PathIndex index) const;

    bool operator<(const Address &addr) const;
    bool operator==(const Address &addr) const;

    size_t hash() const;

    //! a path in this context of flattree contains at most one element
    PathElement singleton_path_element;

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

    Node<Content>* trailProperPath(AddressType addr, NodeStackType &stack);

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
// Output
//-----------------------------------------------------------------------------

template <typename Content>
std::ostream& operator<<(std::ostream &o, const FlatTree<Content>& ft);

template<typename Content>
std::ostream& operator<<(std::ostream &o, const Link<Content>& ts);





//-----------------------------------------------------------------------------
// Impl. Address Template Members
//-----------------------------------------------------------------------------

template <typename Structure>
Address<Structure>::Address():
    singleton_path_element(EMPTY_PATH)
{}


template <typename Structure>
Address<Structure>::Address(PathElement p):
    singleton_path_element(p)
{}

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

//
// Node
//

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
    if (getNodeType() == Node<Content>::LINK)
        return 0;
    else // flattree
        return (reinterpret_cast<FlatTree<Content>*>
                    (const_cast<Node<Content>*>(this)))->links.size();
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
// The idea is that a message is going to be
// sent to all the contents of the given path.
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
        Node<Content> *child = this->getLink(addr[0], true);
        stack.push_back(child); // add root
        return child;
    }

    return this;

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
        Node<Content> *child = this->getLink(addr[0], false);
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

#if 1
    count_entries += links.size();

    copy->links.resize(links.size());
    std::copy(links.begin(), links.end(), copy->links.begin());
    for (auto &link: copy->links)
        link.setSharedContent(link.getContent()); // mark as shared instead of proper
#else
    for (auto &link: links)
    {
        copy->links.push_back(link);
        (*copy->links.rbegin()).setSharedContent(link.getContent());
//        copy->back().set
//        link.setSharedContent(link.getContent()); // mark as shared instead of proper
    }
#endif

    copy->setSharedContent(this->getContent()); // TODO: check the semantics of the lazy copy
                             // in regards to the content
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
