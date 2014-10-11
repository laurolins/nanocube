#pragma once

#include <string>
#include <iostream>
#include <vector>

#include <cstdlib>

#include "Common.hh"

#include "ContentHolder.hh"
#include "TaggedPointer.hh"

namespace quadtree
{

typedef unsigned char NumChildren;
typedef uint64_t Count;

static const bool PROPER_FLAG = false;
static const bool SHARED_FLAG = true;

static const uint16_t PROPER = 0;
static const uint16_t SHARED = 1;


//
// Child names are 0,1,2,3 with a fixed subdivision interpretation:
//
//     (x-bit, y-bit) = x-bit * 2^0 + y-bit * 2^1
//
//                   little-endian
//  ---------        -----------------
//  | 2 | 3 |        | (0,1) | (1,1) |
//  ---------   or   -----------
//  | 0 | 1 |        | (0,0) | (1,0) |
//  ---------        -----------------
//
//  ^ y
//  |
//  *--> x
//
typedef int           ChildName;


//
// An entry index is a value in {0, 1, 2, 3} and corresponds to the
// memory offset on the childrens vector. Note that the child stored
// at entry index "i" might not be named "i"
//
typedef int           ChildEntryIndex;


//
// 16 differnt kinds of nodes: from 0 to 15 all subsets of 4 elements
// (4 child nodes)
//
typedef unsigned char NodeKey;

#include "QuadTreeNode_autogen.hh"

//-----------------------------------------------------------------------------
// ContentHolder
//-----------------------------------------------------------------------------
using contentholder::ContentHolder;

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------

template <typename Content>
struct Node;

template <typename Content, typename K>
struct ScopedNode;

template <typename Content>
struct NodePointer;

//-----------------------------------------------------------------------------
// static maps for actual indices and children indices based on node type
//-----------------------------------------------------------------------------

// if
//
//     nodeActualIndices[i] = j
//
// then the i-th entry in the children memory array correspond to the child
// named "j"
extern ChildName childEntryIndexToName[16][4];

//
// if
//
//     nodeChildrenIndices[i] = j
//
// then the child named "i" is in the j-th entry of the children array
//
extern ChildName childNameToEntryIndex[16][4];

//-----------------------------------------------------------------------------
// Node
//-----------------------------------------------------------------------------

template <typename Content>
struct Node: public ContentHolder<Content>
{
    Node(NodeKey key);

    ~Node();

    void setContentAndChildrenToNull();

    inline Node *getChild(ChildName index) const;
    inline Node<Content>* getChildAndShareFlag(ChildName index, bool &shared) const;

    inline bool isSharedPointer(ChildName index) const;
    inline bool isProperPointer(ChildName index) const;

    // inline NodePointer<Content> getChildPointer(ChildIndex index) const;

    inline NodePointer<Content> const* getChildrenArray() const {
        const ScopedNode<Content, NodeType1111> &node =
            reinterpret_cast<const ScopedNode<Content, NodeType1111>&>(*this);
        return node.children;
    }

    inline NodePointer<Content>* getChildrenArray() {
        ScopedNode<Content, NodeType1111> &node =
            reinterpret_cast<ScopedNode<Content, NodeType1111>&>(*this);
        return node.children;
    }

    void  setChild(Node* child, ChildName index, bool shared);

    inline Count    getNumChildren() const;

    Count    getMemoryUsage() const;

    bool     isLeaf() const;

    bool     hasChildSlot(ChildName index) const;

    Node*    copyWithAddedChild(Node* child, ChildName index, bool shared);

    Node*    makeLazyCopy() const;

    inline NodeKey key() const;

    ChildName getChildActualIndex(int index) const;

    CountRecord count();

private:

    template <NodeKey key>
    void  _setChild(Node* child, ChildName index, bool shared);

    template <NodeKey key>
    inline Count _getNumChildren() const;

    template <NodeKey key>
    Count _getMemoryUsage() const;

private:

    static Node* _newNode(NodeKey key);

};


//-----------------------------------------------------------------------------
// TaggedNodePointer
//-----------------------------------------------------------------------------

template <typename Content>
struct NodePointer
{
    NodePointer();
    NodePointer(Node<Content> *ptr, bool shared);

    inline bool isShared() const {
        return tag_ptr.getTag() == SHARED;
    }

    inline bool isProper() const {
        return tag_ptr.getTag() == PROPER;
    }

    inline Node<Content>* getNode() const {
        return tag_ptr.getPointer();
    }

    inline void setNode(Node<Content>* ptr, bool shared) {
        tag_ptr.setPointer(ptr);
        tag_ptr.setTag(shared ? SHARED : PROPER);
    }

    // space efficiently but not necessarely cpu efficient
    tagged_pointer::TaggedPointer<Node<Content>> tag_ptr;
};


template <typename Content>
NodePointer<Content>::NodePointer():
    tag_ptr(nullptr, SHARED)
{}

template <typename Content>
NodePointer<Content>::NodePointer(Node<Content> *ptr, bool shared):
    tag_ptr(ptr, shared ? SHARED : PROPER)
{}


//-----------------------------------------------------------------------------
// ScopedNode
//-----------------------------------------------------------------------------

template <typename Content, typename NodeType>
struct ScopedNode: public Node<Content>
{
    ScopedNode();

    NodePointer<Content> children[NodeType::numChildren];
       // <-- get value N computed at compile time from key
       //     the interpretation is also stored on key
};






//-----------------------------------------------------------------------------
// Node Implementation
//-----------------------------------------------------------------------------

template <typename Content>
Node<Content>::Node(NodeKey key):
    ContentHolder<Content>()
{
    // TODO: check if we can move this to the parent class constructor (more efficient)
    this->setUserData(key);

    // assert (this->getContent() == nullptr);
    //std::cerr << "Node " << this << std::endl;
}


template <typename Content>
Node<Content>::~Node()
{
    // delete children
    NumChildren       num_children = getNumChildren();
    const NodePointer<Content> *children_pointers = getChildrenArray();
    for (int i=0;i<num_children;i++)
    {
        const NodePointer<Content> &child_pointer = children_pointers[i];
        if (child_pointer.isProper()) {
            delete child_pointer.getNode();
        }
    }

    // delete content if it is proper
    if (this->contentIsProper()) {
        Content* content = this->getContent();
        delete content;
    }
}

template <typename Content>
void Node<Content>::setContentAndChildrenToNull()
{
    // delete children
    NumChildren           num_children = getNumChildren();
    NodePointer<Content> *children = getChildrenArray();

    for (int i=0;i<num_children;i++) {
        children[i].setNode(nullptr, true);
    }

    // delete content if it is proper
    if (this->contentIsProper()) {
        this->setProperContent(nullptr);
    }
}

template <typename Content>
template <NodeKey K>
inline Count
Node<Content>::_getNumChildren() const
{
    return NodeKeyToNodeType<K>::type::numChildren;
}



template <typename Content>
template <NodeKey K>
inline Count
Node<Content>::_getMemoryUsage() const
{
    return sizeof(ScopedNode<Content, typename NodeKeyToNodeType<K>::type>);
}

template <typename Content>
inline bool
Node<Content>::isLeaf() const
{
    return this->key() == 0;
}

template <typename Content>
template <NodeKey K>
inline void
Node<Content>::_setChild(Node<Content>* child, ChildName child_name, bool shared)
{
    typedef typename NodeKeyToNodeType<K>::type CurrentNodeType;

    int child_index = CurrentNodeType::childrenIndices[child_name];

    if (child_index == -1)
        throw std::string("incompatible type");

    NodePointer<Content> &node_pointer = this->getChildrenArray()[child_index];

    node_pointer.setNode(child, shared);
}


template <typename Content>
Node<Content>*
Node<Content>::_newNode(NodeKey key)
{
    if (key == 7)
    {
        return new ScopedNode<Content, typename NodeKeyToNodeType<7>::type>();
    }
    else if (key < 7)
    {
        if (key == 3)
            return new ScopedNode<Content, typename NodeKeyToNodeType<3>::type>();
        else if (key < 3)
        {
            if (key == 1)
                return new ScopedNode<Content, typename NodeKeyToNodeType<1>::type>();
            else if (key == 0)
                return new ScopedNode<Content, typename NodeKeyToNodeType<0>::type>();
            else if (key == 2)
                return new ScopedNode<Content, typename NodeKeyToNodeType<2>::type>();
        }
        else {
            if (key == 5)
                return new ScopedNode<Content, typename NodeKeyToNodeType<5>::type>();
            else if (key == 4)
                return new ScopedNode<Content, typename NodeKeyToNodeType<4>::type>();
            else if (key == 6)
                return new ScopedNode<Content, typename NodeKeyToNodeType<6>::type>();
        }
    }
    else { // key > 7
        if (key == 11)
            return new ScopedNode<Content, typename NodeKeyToNodeType<11>::type>();
        else if (key < 11)
        {
            if (key == 9)
                return new ScopedNode<Content, typename NodeKeyToNodeType<9>::type>();
            else if (key == 8)
                return new ScopedNode<Content, typename NodeKeyToNodeType<8>::type>();
            else if (key == 10)
                return new ScopedNode<Content, typename NodeKeyToNodeType<10>::type>();
        }
        else // key > 13
        {
            if (key == 13)
                return new ScopedNode<Content, typename NodeKeyToNodeType<13>::type>();
            else if (key == 12)
                return new ScopedNode<Content, typename NodeKeyToNodeType<12>::type>();
           else if (key == 15)
                return new ScopedNode<Content, typename NodeKeyToNodeType<15>::type>();
            else if (key == 14)
                return new ScopedNode<Content, typename NodeKeyToNodeType<14>::type>();
        }
    }

    // invalid key
    throw std::string("Invalid child index");

}

template <typename Content>
inline bool
Node<Content>::hasChildSlot(ChildName index) const
{
    return ((this->key & (1 << index)) != 0);
}


template <typename Content>
NodeKey
inline Node<Content>::key() const
{
    return this->getUserData();
}

template <typename Content>
ChildName
Node<Content>::getChildActualIndex(int index) const
{
    return childEntryIndexToName[this->key()][index];
}

template <typename Content>
CountRecord Node<Content>::count()
{
    CountRecord mine;
    return mine;
}

template <typename Content>
Node<Content>*
Node<Content>::copyWithAddedChild(Node<Content>* child, ChildName new_child_name, bool shared)
{
    NodeKey key = this->key();
    NodeKey new_key = key | (1 << new_child_name);

    if (new_key == key)
        throw std::string("Should be a different key");

    Node<Content> *copy = Node<Content>::_newNode(new_key);
    copy->copyContentAndProperFlag(*this);

    // set child
    copy->setChild(child, new_child_name, shared);

    NumChildren            num_children   = getNumChildren();
    ChildName             *children_names = childEntryIndexToName[key];
    NodePointer<Content>  *children       = getChildrenArray();

    for (int i=0;i<num_children;i++)
    {
        const NodePointer<Content> &ci = children[i];
        copy->setChild(ci.getNode(), children_names[i], ci.isShared());
    }

    return copy;
}

template <typename Content>
Node<Content>*
Node<Content>::makeLazyCopy() const
{
    Node<Content> *copy = Node<Content>::_newNode(this->key());
    NumChildren            num_children = getNumChildren();
    NodePointer<Content>  const *source_children_pointers = getChildrenArray();
    NodePointer<Content>  *target_children_pointers = copy->getChildrenArray();
    for (int i=0;i<num_children;i++)
    {
        const NodePointer<Content> &source_pointer = source_children_pointers[i];
        NodePointer<Content> &target_pointer = target_children_pointers[i];
        target_pointer.setNode(source_pointer.getNode(), SHARED_FLAG); // make everything shared
    }
    copy->setSharedContent(this->getContent());
    return copy;
}

template <typename Content>
void
Node<Content>::setChild(Node<Content>* child, ChildName child_name, bool shared)
{
    // NodeKey K = this->key;
    NodeKey key = this->key();

    if (key == 7)
    {
        _setChild<7>(child, child_name, shared);
    }
    else if (key < 7)
    {
        if (key == 3)
            _setChild<3>(child, child_name, shared);
        else if (key < 3)
        {
            if (key == 1)
                _setChild<1>(child, child_name, shared);
            else if (key == 0)
                _setChild<0>(child, child_name, shared);
            else if (key == 2)
                _setChild<2>(child, child_name, shared);
        }
        else {
            if (key == 5)
                _setChild<5>(child, child_name, shared);
            else if (key == 4)
                _setChild<4>(child, child_name, shared);
            else if (key == 6)
                _setChild<6>(child, child_name, shared);
        }
    }
    else { // key > 7
        if (key == 11)
            _setChild<11>(child, child_name, shared);
        else if (key < 11)
        {
            if (key == 9)
                _setChild<9>(child, child_name, shared);
            else if (key == 8)
                _setChild<8>(child, child_name, shared);
            else if (key == 10)
                _setChild<10>(child, child_name, shared);
        }
        else // key > 13
        {
            if (key == 13)
                _setChild<13>(child, child_name, shared);
            else if (key == 12)
                _setChild<12>(child, child_name, shared);
            else if (key == 15)
                _setChild<15>(child, child_name, shared);
            else if (key == 14)
                _setChild<14>(child, child_name, shared);
        }
    }

    if (key <16)
        return;

    // invalid key
    throw std::string("Invalid child index");

}

//template <typename Content>
//inline NodePointer<Content>
//Node<Content>::getChildPointer(ChildIndex index) const
//{
//    NodeKey key = this->key();
//    const NodePointer<Content> *children_begin = getChildrenArray();
//    int i = nodeChildrenIndices[key][index];
//    if (i == -1) return nullptr;
//    return children_begin[i]; //
//}

template <typename Content>
inline Node<Content>*
Node<Content>::getChild(ChildName child_name) const
{
    NodeKey key = this->key();
    const NodePointer<Content> *children_begin = getChildrenArray();
    int i = childNameToEntryIndex[key][child_name];
    if (i == -1) return nullptr;
    return children_begin[i].getNode();
}

template <typename Content>
inline Node<Content>*
Node<Content>::getChildAndShareFlag(ChildName child_name, bool &shared) const
{
    NodeKey key = this->key();
    const NodePointer<Content> *children_begin = getChildrenArray();
    int i = childNameToEntryIndex[key][child_name];
    if (i == -1) {

        // std::cout << "child_name is NOT part of this node storage" << std::endl;

        return nullptr;
    }
    else {
        shared = children_begin[i].isShared();

        // std::cout << "child_name IS part of this node storage: " << children_begin[i].getNode() << " shared:" << shared << std::endl;

        return children_begin[i].getNode();
    }
}


template <typename Content>
bool
Node<Content>::isSharedPointer(ChildName child_name) const
{
    NodeKey key = this->key();
    const NodePointer<Content> *children_begin = getChildrenArray();
    int i = childNameToEntryIndex[key][child_name];
    if (i == -1) throw std::exception();
    return children_begin[i].isShared();
}

template <typename Content>
bool
Node<Content>::isProperPointer(ChildName child_name) const
{
    NodeKey key = this->key();
    const NodePointer<Content> *children_begin = getChildrenArray();
    int i = childNameToEntryIndex[key][child_name];
    if (i == -1) throw std::exception();
    return children_begin[i].isProper();
}

template <typename Content>
Count
Node<Content>::getMemoryUsage() const
{
    // NodeKey K = this->key;
    NodeKey key = this->key();

    if (key == 7)
    {
        return _getMemoryUsage<7>();
    }
    else if (key < 7)
    {
        if (key == 3)
            return _getMemoryUsage<3>();
        else if (key < 3)
        {
            if (key == 1)
                return _getMemoryUsage<1>();
            else if (key == 0)
                return _getMemoryUsage<0>();
            else if (key == 2)
                return _getMemoryUsage<2>();
        }
        else {
            if (key == 5)
                return _getMemoryUsage<5>();
            else if (key == 4)
                return _getMemoryUsage<4>();
            else if (key == 6)
                return _getMemoryUsage<6>();
        }
    }
    else { // key > 7
        if (key == 11)
            return _getMemoryUsage<11>();
        else if (key < 11)
        {
            if (key == 9)
                return _getMemoryUsage<9>();
            else if (key == 8)
                return _getMemoryUsage<8>();
            else if (key == 10)
                return _getMemoryUsage<10>();
        }
        else // key > 11
        {
            if (key == 13)
                return _getMemoryUsage<13>();
            else if (key == 12)
                return _getMemoryUsage<12>();
            else if (key == 15)
                return _getMemoryUsage<15>();
            else if (key == 14)
                return _getMemoryUsage<14>();
        }
    }

    // invalid key
    throw std::string("key needs to be [0,15]");

}

template <typename Content>
inline Count
Node<Content>::getNumChildren() const
{
    NodeKey key = this->key();
    static const int v[16] = {0, 1, 1, 2,
                              1, 2, 2, 3,
                              1, 2, 2, 3,
                              2, 3, 3, 4};
    return v[key];
}


//
// ScopedNode Implementation
//

template <typename Content, typename NodeType>
ScopedNode<Content, NodeType>::ScopedNode(): Node<Content>(NodeType::key)
{}

} // end namespace quadtree
