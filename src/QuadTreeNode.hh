#pragma once

#include <string>
#include <iostream>
#include <vector>

#include <cstdlib>

#include <Common.hh>

#include <ContentHolder.hh>

namespace quadtree
{

typedef unsigned char NumChildren;
typedef uint64_t Count;

//=========================================================================
// Part Automatically Generated. See tests/quad-tree-node/script.py
//=========================================================================

typedef unsigned char NodeKey;
typedef int           ChildIndex;

#include "QuadTreeNode_autogen.hh"

//=========================================================================
// Node Type Descriptions
//=========================================================================


//
// ContentHolder
//
using contentholder::ContentHolder;

//
// Node
//

template <typename Content>
struct Node;

template <typename Content, typename K>
struct ScopedNode;

//
// Node
//

extern ChildIndex nodeActualIndices[16][4];
extern ChildIndex nodeChildrenIndices[16][4];

template <typename Content>
struct Node: public ContentHolder<Content>
{
    Node(NodeKey key);

    ~Node();

    void setContentAndChildrenToNull();

    inline Node* getChild(ChildIndex index) const;

    inline Node* const* getChildrenArray() const {
        const ScopedNode<Content, NodeType1111> &node =
            reinterpret_cast<const ScopedNode<Content, NodeType1111>&>(*this);
        return node.children; 
    }

    inline Node** getChildrenArray() {
        ScopedNode<Content, NodeType1111> &node =
            reinterpret_cast<ScopedNode<Content, NodeType1111>&>(*this);
        return node.children; 
    }

    void  setChild(Node* child, ChildIndex index);

    inline Count    getNumChildren() const;

    Count    getMemoryUsage() const;

    bool     isLeaf() const;

    bool     hasChildSlot(ChildIndex index) const;

    Node*    copyWithAddedChild(Node<Content> *child, ChildIndex index);

    inline NodeKey key() const;

    CountRecord count();

private:

    template <NodeKey key>
    void  _setChild(Node* child, ChildIndex index);

    template <NodeKey key>
    inline Count _getNumChildren() const;

    template <NodeKey key>
    Count _getMemoryUsage() const;

private:

    static Node* _newNode(NodeKey key);

};

//
// ScopedNode
//

template <typename Content, typename NodeType>
struct ScopedNode: public Node<Content>
{
    ScopedNode();

    Node<Content>* children[NodeType::numChildren];
       // <-- get value N computed at compile time from key
       //     the interpretation is also stored on key
};


//=========================================================================
// Node Implementations
//=========================================================================

//
// Node Implementation
//

template <typename Content>
Node<Content>::Node(NodeKey key):
    ContentHolder<Content>()
{
    this->setUserData(key);

    //std::cerr << "Node " << this << std::endl;
}

template <typename Content>
Node<Content>::~Node()
{
    // delete children
    for (int i=0; i < 4; i++) {
        delete getChild(i);
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
    NumChildren       num_children = getNumChildren();
    Node<Content>**       children = getChildrenArray();
    
    for (int i=0;i<num_children;i++) {
        children[i] = nullptr;
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
Node<Content>::_setChild(Node<Content>* child, ChildIndex index)
{
    typedef typename NodeKeyToNodeType<K>::type CurrentNodeType;

    ChildIndex actualIndex = CurrentNodeType::childrenIndices[index];

    if (actualIndex == -1)
        throw std::string("incompatible type");

    this->getChildrenArray()[actualIndex] = child;
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
Node<Content>::hasChildSlot(ChildIndex index) const
{
    return ((this->key & (1 << index)) != 0);
}


template <typename Content>
NodeKey
Node<Content>::key() const
{
    return this->getUserData();
}

template <typename Content>
CountRecord Node<Content>::count()
{
    CountRecord mine;
    return mine;
}

template <typename Content>
Node<Content>*
Node<Content>::copyWithAddedChild(Node<Content> *child, ChildIndex index)
{
    NodeKey key = this->key();
    NodeKey new_key = key | (1 << index);

    if (new_key == key)
        throw std::string("Should be a different key");

    Node<Content> *copy = Node<Content>::_newNode(new_key);
    copy->copyContentAndProperFlag(*this);

    // set child
    copy->setChild(child, index);

    NumChildren       num_children = getNumChildren();
    ChildIndex       *actual_indices = nodeActualIndices[key];
    Node<Content>    **children = getChildrenArray();

    for (int i=0;i<num_children;i++)
    {
        Node<Content>*   ci = const_cast<Node<Content>*>(children[i]);
        copy->setChild(ci, actual_indices[i]);
    }

    return copy;
}


template <typename Content>
void
Node<Content>::setChild(Node<Content>* child, ChildIndex index)
{
    // NodeKey K = this->key;
    NodeKey key = this->key();

    if (key == 7)
    {
        _setChild<7>(child, index);
    }
    else if (key < 7)
    {
        if (key == 3)
            _setChild<3>(child, index);
        else if (key < 3)
        {
            if (key == 1)
                _setChild<1>(child, index);
            else if (key == 0)
                _setChild<0>(child, index);
            else if (key == 2)
                _setChild<2>(child, index);
        }
        else {
            if (key == 5)
                _setChild<5>(child, index);
            else if (key == 4)
                _setChild<4>(child, index);
            else if (key == 6)
                _setChild<6>(child, index);
        }
    }
    else { // key > 7
        if (key == 11)
            _setChild<11>(child, index);
        else if (key < 11)
        {
            if (key == 9)
                _setChild<9>(child, index);
            else if (key == 8)
                _setChild<8>(child, index);
            else if (key == 10)
                _setChild<10>(child, index);
        }
        else // key > 13
        {
            if (key == 13)
                _setChild<13>(child, index);
            else if (key == 12)
                _setChild<12>(child, index);
            else if (key == 15)
                _setChild<15>(child, index);
            else if (key == 14)
                _setChild<14>(child, index);
        }
    }

    if (key <16)
        return;

    // invalid key
    throw std::string("Invalid child index");

}

template <typename Content>
inline Node<Content>*
Node<Content>::getChild(ChildIndex index) const
{
    NodeKey key = this->key();
    Node<Content> *const *children_begin = getChildrenArray();
    int i = nodeChildrenIndices[key][index];
    if (i == -1) return nullptr;
    return children_begin[i];
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
