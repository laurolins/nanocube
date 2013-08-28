#pragma once

#include <iostream>
#include <cstddef>
#include <stdint.h>
#include <cassert>
#include <sstream>
#include <stack>

#include <QuadTreeNode.hh>

namespace quadtree
{

//-----------------------------------------------------------------------------
// typedefs
//-----------------------------------------------------------------------------

typedef int32_t  Level; // this goes up 25 (17 zooms + 8 resolution)
typedef uint32_t Coordinate;
typedef int      ChildIndex;
typedef uint64_t Count;

typedef uint8_t  BitSize;
typedef uint8_t  BitIndex; // this goes up 25 (17 zooms + 8 resolution)

//-----------------------------------------------------------------------------
// Forward Decl.
//-----------------------------------------------------------------------------

template<BitSize N, typename Structure>
class Address;

template<BitSize N, typename Content>
class QuadTree;

//-----------------------------------------------------------------------------
// MemUsage
//-----------------------------------------------------------------------------

struct MemUsage
{

    template <typename Content>
    void add(Node<Content> *node);

    template <typename Content>
    void remove(Node<Content> *node);

    Count getMemUsage()  const;
    Count getMemUsage(NumChildren n)  const;
    Count getCount() const;
    Count getCount(NumChildren n)  const;

    MemUsage();
    ~MemUsage();

    Count totalMem;
    Count totalCount;
    std::vector<Count> memByNumChildren;
    std::vector<Count> countByNumChildren;
};


//-----------------------------------------------------------------------------
// StackItem
//-----------------------------------------------------------------------------

template <BitSize N, typename Content, typename Structure>
struct StackItem
{
    StackItem();
    StackItem(Node<Content>* node, Address<N, Structure> address);

    Node<Content>             *node;
    Address<N, Structure>      address;
};

//-----------------------------------------------------------------------------
// QuadTree
//-----------------------------------------------------------------------------

template<BitSize N, typename Content>
class QuadTree
{
public:


    typedef QuadTree<N,Content>              Type;
    typedef Node<Content>                    NodeType;
    typedef Address<N, Type>                 AddressType;
    typedef Content                          ContentType;

    typedef std::vector<NodeType*>            NodeStackType;

    static const BitSize AddressSize = N;

    // public:
    //     typedef QuadTreeNode<N> NodeType;

public:

    QuadTree(); // up to 32 levels right now
    ~QuadTree();

    template<typename QuadTreeAddPolicy, typename Point>
    void add(AddressType address, Point &point, QuadTreeAddPolicy &addPolicy);

    // returns the deepest node that was added
    NodeType*  trailProperPath(AddressType address, NodeStackType &stack);

    NodeType* find(AddressType address) const;

    bool isEmpty() const;

    // visit all subnodes of a certain node in the
    // requested target level.
    template <typename Visitor>
    void visitSubnodes(AddressType address, Level targetLevelOffset, Visitor &visitor);

    // visit all subnodes of a certain node in the
    // requested target level.
    template <typename Visitor>
    void visitRange(AddressType min_address, AddressType max_address, Visitor &visitor);

    Count  getNumNodes()    const;
    Count  getNumLeaves()   const;
    Count  getMemoryUsage() const; // in bytes
    Count  getNumAdds()     const;

private:

    Node<Content> *_createPath(AddressType current_address, AddressType target_address);

public:

    NodeType* root; // the root always exist by definition

    Count  numAdds;

    MemUsage counters;

};

//-----------------------------------------------------------------------------
// Address
//-----------------------------------------------------------------------------

typedef unsigned char PathSize;
typedef unsigned char PathElement;
typedef unsigned char PathIndex;

enum AddressRangeRelation { CONTAINED, DISJOINT, INTERSECTS_BUT_NOT_CONTAINED };

template<BitSize N, typename Structure>
class Address
{
public:

    typedef Structure StructureType; // type of the structure addressed by this Address

public:

    Address(): x(0), y(0), level(0) {};
    Address(Coordinate x, Coordinate y, Level level):
        x(x), y(y), level(level) {};

    Address nextAddressTowards(const Address<N, Structure> &target) const;

    void eraseHigherBits();

    BitIndex levelBitIndex(Level level) const;
    Coordinate levelCoordinateMask(Level level) const;

    Address    childAddress(ChildIndex index) const;
    ChildIndex indexOnParent() const;

    inline Coordinate xbit(Level level) const;
    inline Coordinate ybit(Level level) const;

    inline Coordinate xbit() const;
    inline Coordinate ybit() const;

    bool isEmpty() const;

    bool operator==(const Address &addr) const;

    bool operator<(const Address &addr) const;

    void range(Address& min_address, Address &max_address, Level level) const;

    AddressRangeRelation getRangeRelation(const Address &min_addr, const Address &max_addr) const;

    size_t hash() const;

    void setLevelCoords(Coordinate x, Coordinate y, Level level);
    Coordinate getLevelXCoord() const;
    Coordinate getLevelYCoord() const;

    // new stuff (to enable composition on a nested LOD structure)
    PathElement operator[](PathIndex index) const;
    PathSize    getPathSize() const;

public:

    Coordinate x, y;
    Level      level;

};


template<BitSize N, typename Structure>
std::ostream& operator<<(std::ostream &os, const Address<N, Structure>& addr)
{
    os << "Addr[x: "  << addr.getLevelXCoord()
       << ", y: "     << addr.getLevelYCoord()
       << ", level: " << addr.level
       << "]";
    return os;
}


//-----------------------------------------------------------------------------
// Implmentation of QuadTree Template members
//-----------------------------------------------------------------------------

template<BitSize N, typename Content>
QuadTree<N, Content>::QuadTree():
    root(nullptr),
    numAdds(0)
{
    // std::cout << "constructing quadtree " << (unsigned long) this << std::endl;
    // std::cout << "    root " << (unsigned long) root << std::endl;
}


template<BitSize N, typename Content>
QuadTree<N, Content>::~QuadTree()
{
    // std::cout << "deleting quadtree " << (unsigned long) this << std::endl;
    if (root)
    {
        // std::cout << "   deleting quadtree root" << (unsigned long) root << std::endl;
        delete root;
    }
//    std::cout << "~QuadTree " << this << std::endl;
}

template<BitSize N, typename Content>
inline Count QuadTree<N, Content>::getNumNodes() const
{
    return counters.getCount(); // count all nodes
}

template<BitSize N, typename Content>
inline Count QuadTree<N, Content>::getNumAdds() const
{
    return numAdds;
}

template<BitSize N, typename Content>
inline Count QuadTree<N, Content>::getNumLeaves() const
{
    return counters.getCount(0); // only nodes with zero children
}

template<BitSize N, typename Content>
inline Count
QuadTree<N, Content>::getMemoryUsage() const
{
    return counters.getMemUsage(); // not including the QuadTree record itself.
}

template<BitSize N, typename Content>
inline bool
QuadTree<N, Content>::isEmpty() const
{
    return root == nullptr;
}


template<BitSize N, typename Content>
Node<Content>*
QuadTree<N, Content>::_createPath(AddressType current_address, AddressType target_address)
{

    // create nodes bottom up of the right kind;
    int n = target_address.level - current_address.level; // read all these bits

    assert (n > 0);

    // leaf
    Node<Content> *current = new ScopedNode<Content, NodeType0000>();
#ifdef COLLECT_MEMUSAGE
    counters.add(current);
#endif

    for (int i=target_address.level;i>current_address.level+1;i--) {

        ChildIndex xoff = target_address.xbit(i) ? 1 : 0;
        ChildIndex yoff = target_address.ybit(i) ? 1 : 0 ;
        ChildIndex index = (yoff << 1) | xoff;

        Node<Content> *next_node = nullptr;
        if (index == 0) {
            ScopedNode<Content, NodeType1000> *aux = new ScopedNode<Content, NodeType1000>();
            aux->children[0] = current;
            next_node = aux;
        } else if (index == 1) {
            ScopedNode<Content, NodeType0100> *aux = new ScopedNode<Content, NodeType0100>();
            aux->children[0] = current;
            next_node = aux;
        } else if (index == 2) {
            ScopedNode<Content, NodeType0010> *aux = new ScopedNode<Content, NodeType0010>();
            aux->children[0] = current;
            next_node = aux;
        } else if (index == 3) {
            ScopedNode<Content, NodeType0001> *aux = new ScopedNode<Content, NodeType0001>();
            aux->children[0] = current;
            next_node = aux;
        }

        //
#ifdef COLLECT_MEMUSAGE
        counters.add(next_node);
#endif
        current=next_node;
    }

    return current;
}

template<BitSize N, typename Content>
template<typename QuadTreeAddPolicy, typename Point>
void
QuadTree<N, Content>::add(AddressType address, Point &point, QuadTreeAddPolicy &addPolicy)
{
    numAdds++; // increment Add counter

    AddressType    currentAddress(0,0,0);
    NodeType* currentNode  = root;
    NodeType* previousNode = nullptr;

    if (isEmpty())
    {
        // new leaf nodes
        currentNode = new ScopedNode<Content,NodeType0000>(); // _newNode with nullptr: new root
#ifdef COLLECT_MEMUSAGE
        counters.add(currentNode);
#endif
        root = currentNode;
        // leaf node

        // When reported of a new address in the quadtree the policy
        // might add a new Content to the node just created
        // (e.g. associate a TimeSeries object as the content
        // of the new node)
        addPolicy.newNode(currentNode, currentAddress, address);
    }

    //
    //        Context<Content> context;
    //        context.push(root,0,currentAddress);

    while (currentAddress.level <= address.level)
    {

        addPolicy.addPoint(point, currentNode, currentAddress, address);

        // add point to all collections in the trail
        // currentNode->collection.add(element); (create a more
        // general signaling mechanism where a user can accomplish
        // the our goal)

        if (currentAddress == address)
        {
            return; // node has been created
        }

        // assuming address is contained in the current address
        AddressType nextAddress = currentAddress.nextAddressTowards(address);

        // nextAddress index on current node
        ChildIndex index = nextAddress.indexOnParent();

        NodeType* nextNode = currentNode->getChild(index);

        if (!nextNode)
        {
            // new leaf nodes
            // nextNode = new ScopedNode<Content,NodeType0000>(); // _newNode with nullptr: new root
            nextNode = _createPath(currentAddress,address); // new ScopedNode<Content,NodeType0000>(); // _newNode with nullptr: new root

#ifdef COLLECT_MEMUSAGE
            counters.remove(currentNode);
#endif

            // create copy of currentNode with updated type
            NodeType* currentNodeUpdated = currentNode->copyWithAddedChild(nextNode, index);

#ifdef COLLECT_MEMUSAGE
            counters.add(currentNodeUpdated);
#endif

            //
            if (previousNode)
                previousNode->setChild(currentNodeUpdated, currentAddress.indexOnParent());
            else if (currentNode == root)
                root = currentNodeUpdated;

            // delete current node
            currentNode->setContentAndChildrenToNull();
            delete currentNode;
            currentNode = currentNodeUpdated;

            // new address
            addPolicy.newNode(nextNode, nextAddress, address);
        }

        previousNode   = currentNode;
        currentNode    = nextNode;
        currentAddress = nextAddress;
    }

    // Shouldn't get here. Should exit
    // when currentAddr == address
    assert(false);

}





template<BitSize N, typename Content>
Node<Content>*
QuadTree<N, Content>::trailProperPath(AddressType address, NodeStackType &stack)
{
    numAdds++; // increment Add counter

    AddressType     currentAddress(0,0,0);
    NodeType* currentNode  = root;
    NodeType* previousNode = nullptr;

    if (isEmpty())
    {
        // new leaf nodes
        currentNode = new ScopedNode<Content,NodeType0000>(); // _newNode with nullptr: new root
#ifdef COLLECT_MEMUSAGE
        counters.add(currentNode);
#endif
        root = currentNode;
        // leaf node
    }

    while (currentAddress.level <= address.level)
    {
        stack.push_back(currentNode);

        // add point to all collections in the trail
        // currentNode->collection.add(element); (create a more
        // general signaling mechanism where a user can accomplish
        // the our goal)

        if (currentAddress == address)
        {
            return currentNode; // node has been created
        }

        // assuming address is contained in the current address
        AddressType nextAddress = currentAddress.nextAddressTowards(address);

        // nextAddress index on current node
        ChildIndex index = nextAddress.indexOnParent();

        NodeType* nextNode = currentNode->getChild(index);

        if (!nextNode)
        {
            // new leaf nodes
            nextNode = _createPath(currentAddress,address);

#ifdef COLLECT_MEMUSAGE
            counters.remove(currentNode);
#endif

            // create copy of currentNode with updated type
            NodeType* currentNodeUpdated = currentNode->copyWithAddedChild(nextNode, index);

#ifdef COLLECT_MEMUSAGE
            counters.add(currentNodeUpdated);
#endif
            if (previousNode)
                previousNode->setChild(currentNodeUpdated, currentAddress.indexOnParent());
            else if (currentNode == root)
                root = currentNodeUpdated;

            // delete current node
            currentNode->setContentAndChildrenToNull();
            delete currentNode;
            currentNode = currentNodeUpdated;

            stack.back() = currentNode;
        }

        previousNode   = currentNode;
        currentNode    = nextNode;
        currentAddress = nextAddress;
    }

    // Shouldn't get here. Should exit
    // when currentAddr == address
    return currentNode; // when add root

}

template<BitSize N, typename Content>
Node<Content>*
QuadTree<N, Content>::find(AddressType address)  const
{
    if (isEmpty())
        return nullptr;

    AddressType currentAddress(0,0,0);
    NodeType*   currentNode = root;

    while (currentAddress.level <= address.level)
    {
        if (currentAddress == address)
            return currentNode;

        // assuming address is contained in the current address
        AddressType nextAddress = currentAddress.nextAddressTowards(address);

        // nextAddress index on current node
        ChildIndex index = nextAddress.indexOnParent();

        NodeType* nextNode = currentNode->getChild(index);

        if (!nextNode)
            break; // there is no next node

        currentNode    = nextNode;
        currentAddress = nextAddress;
    }

    return nullptr;
}

template<BitSize N, typename Content>
template <typename Visitor>
void
QuadTree<N, Content>::visitSubnodes(AddressType address, Level targetLevelOffset, Visitor &visitor)
{
    Level targetLevel = address.level + targetLevelOffset;

    NodeType* baseNode = this->find(address);

    if (!baseNode)
        return; // there is no node

    std::stack<StackItem<N, Content, QuadTree<N, Content>>> stack;
    stack.push(StackItem<N, Content, QuadTree<N, Content>>(baseNode, address));

    while (!stack.empty())
    {
        StackItem<N, Content, QuadTree<N, Content>> &topItem = stack.top();
        NodeType*   node = topItem.node;
        AddressType addr = topItem.address;
        stack.pop();

        if (targetLevel < 0 || addr.level == targetLevel)
            visitor.visit(node, addr);

        if (targetLevel < 0 || addr.level < targetLevel)
        {
            NumChildren num_children = node->getNumChildren();
            const ChildIndex *actual_indices = nodeActualIndices[node->key()];
            NodeType**  children = node->getChildrenArray();

            for (int i=0;i<num_children;i++)
            {
                NodeType*   childNode = const_cast<NodeType*>(children[i]);
                AddressType childAddr = addr.childAddress(actual_indices[i]);
                stack.push(StackItem<N, Content, QuadTree<N, Content>>(childNode, childAddr));
            }
        }
    }
}

// visit all subnodes of a certain node in the
// requested target level.
template<BitSize N, typename Content>
template <typename Visitor>
void QuadTree<N,Content>::visitRange(AddressType min_address, AddressType max_address, Visitor &visitor)
{
    if (this->isEmpty()) // empty
        return;

    assert (min_address.level == max_address.level &&
            min_address.x     <= max_address.x     &&
            min_address.y     <= max_address.y);

    std::stack<StackItem<N, Content, QuadTree<N, Content>>> stack;
    stack.push(StackItem<N, Content, QuadTree<N, Content>>(root, AddressType()));

    while (!stack.empty())
    {
        StackItem<N, Content, QuadTree<N, Content>> &topItem = stack.top();
        NodeType*   node = topItem.node;
        AddressType addr = topItem.address;
        stack.pop();

        // std::cout << "Testing address: " << addr  << std::endl;

        // prepare range of current address
        AddressRangeRelation rel = addr.getRangeRelation(min_address, max_address);

        // std::cout << "   Relation was: " << rel << "  ";

        if (rel == CONTAINED) {
            // if range spanned by addr is contained in range min_address, max_address
            // then visit
            // std::cout << "   contained!" << std::endl;
            visitor.visit(node, addr);
        }
        else if (rel == INTERSECTS_BUT_NOT_CONTAINED)
        {
            NumChildren num_children = node->getNumChildren();
            const ChildIndex *actual_indices = nodeActualIndices[node->key()];
            NodeType**  children = node->getChildrenArray();

            for (int i=0;i<num_children;i++)
            {
                NodeType*   childNode = const_cast<NodeType*>(children[i]);
                AddressType childAddr = addr.childAddress(actual_indices[i]);
                stack.push(StackItem<N, Content, QuadTree<N, Content>>(childNode, childAddr));
            }
        }
        else {
            // std::cout << "   disjoint!" << std::endl;
        }

        // else nothing needs to be done.
    }
}






//-----------------------------------------------------------------------------
// Implmentation of Address Template members
//-----------------------------------------------------------------------------

// template<BitSize N, typename Structure>
// Address<N, Structure>::Address():
//     x(0), y(0), level(0)
// {}

// template<BitSize N, typename Structure>
// Address<N, Structure>::Address(Coordinate x, Coordinate y, Level level):
//     x(x), y(y), level(level)
// {}

template<BitSize N, typename Structure>
inline BitIndex
Address<N, Structure>::levelBitIndex(Level level) const
{
    // 0 is the least significant bit and corresponds to the decision
    // from level N-1 to level N (levels are counted from zero).
    return N - level;
}

template<BitSize N, typename Structure>
inline Coordinate
Address<N, Structure>::levelCoordinateMask(Level level) const
{
    if (level == 0) // on level zero the mask is zero
        return 0;

    // Coordinate with only the given level bit on
    return 1 << levelBitIndex(level);
}

template<BitSize N, typename Structure>
void Address<N, Structure>::eraseHigherBits()
{
    int k = 32 - level;
    Coordinate mask = (((0xFFFFFFFFU) << k) >> k);

    // std::cout << "mask:" << mask << std::endl;

    x = x & mask;
    y = y & mask;
}

template<BitSize N, typename Structure>
inline Coordinate Address<N, Structure>::xbit(Level level) const
{
    return levelCoordinateMask(level) & x;
}

template<BitSize N, typename Structure>
inline Coordinate Address<N, Structure>::ybit(Level level) const
{
    return levelCoordinateMask(level) & y;
}

template<BitSize N, typename Structure>
inline Coordinate Address<N, Structure>::xbit() const
{
    return xbit(level);
}

template<BitSize N, typename Structure>
inline Coordinate Address<N, Structure>::ybit() const
{
    return ybit(level);
}

template<BitSize N, typename Structure>
inline bool Address<N, Structure>::operator==(const Address<N, Structure> &addr) const
{
    return (x == addr.x && y == addr.y && level == addr.level);
}

template<BitSize N, typename Structure>
inline bool Address<N, Structure>::operator<(const Address<N, Structure> &addr) const
{
    return (level < addr.level) ||
            (level == addr.level && y < addr.y) ||
            (level == addr.level && y == addr.y && x < addr.x);
}

template<BitSize N, typename Structure>
inline AddressRangeRelation Address<N, Structure>::getRangeRelation(const Address<N, Structure> &min_addr,
                                                                    const Address<N, Structure> &max_addr) const
{
    //
    int target_level = min_addr.level;

    // make sure levels match requirements
    assert (max_addr.level == target_level && target_level >= this->level);

    // get range of addresses of this addres
    Address<N, Structure> this_min_addr, this_max_addr;
    this->range(this_min_addr, this_max_addr, target_level);

#if 0
    std::cout << "      range of " << *this << std::endl;
    std::cout << "          min: " << this_min_addr << std::endl;
    std::cout << "          max: " << this_max_addr << std::endl;
#endif

    if (min_addr.x <= this_min_addr.x && min_addr.y <= this_min_addr.y &&
            this_max_addr.x <= max_addr.x && this_max_addr.y <= max_addr.y) {
        return CONTAINED;
    }
    else if (min_addr.x > this_max_addr.x || max_addr.x < this_min_addr.x ||
             min_addr.y > this_max_addr.y || max_addr.y < this_min_addr.y) {
        return DISJOINT;
    }
    else {
        return INTERSECTS_BUT_NOT_CONTAINED;
    }
}


#if 0
static std::string bin(uint32_t x, int length) {
    std::stringstream ss;
    uint32_t aux = x;
    int i = 0;
    while (aux > 0) {
        // std::cout << aux << std::endl;
        ss << (int) (0x1 & aux);
        aux >>= 1;
        i++;
    }

    while(i < length) {
        ss << "0";
        i++;
    }

    return ss.str();
}
#endif


template<BitSize N, typename Structure>
void Address<N, Structure>::range(Address &min_address, Address &max_address, Level target_level) const
{


    min_address.x = x;
    min_address.y = y;
    min_address.level = target_level;


    if (target_level == level) {

        max_address = min_address;

    }
    else {

        // assuming target_level > this->level

        static const int S = sizeof(Coordinate) * 8; // no bits of Coordinate

#if 0
        std::cout
            << "S:            " << (int) S << std::endl
            << "N:            " << (int) N << std::endl
            << "level:        " << level << std::endl
            << "target_level: " << target_level << std::endl;
        //
        Coordinate mask_lower_bits = ~0;
        std::cout  << "mask_lower_bits step 1:        " << bin(mask_lower_bits, 32) << std::endl;
        mask_lower_bits >>= S - (target_level - level);
        std::cout  << "mask_lower_bits step 2:        " << bin(mask_lower_bits, 32) << std::endl;
        mask_lower_bits <<= N - target_level;
        std::cout  << "mask_lower_bits step 3:        " << bin(mask_lower_bits, 32) << std::endl;

    //    Coordinate mask_lower_bits_compl = ~mask_lower_bits; // we have N-level

    //    min_address.x |= mask_lower_bits_compl;
    //    min_address.y |= mask_lower_bits_compl;
    //    min_address.level = target_level;


        std::cout
            << "00000000001111111111222222222233" << std::endl
            << "01234567890123456789012345678901" << std::endl
            << bin(mask_lower_bits, 32) << std::endl;
#else
        Coordinate mask_lower_bits = ~0;
        mask_lower_bits >>= S - (target_level - level);
        mask_lower_bits <<= N - target_level;
#endif

        max_address.x = x | mask_lower_bits;
        max_address.y = y | mask_lower_bits;
        max_address.level = target_level;
    }



}

template<BitSize N, typename Structure>
inline size_t Address<N, Structure>::hash() const
{
    return ((size_t) x << 16) | (size_t) y | ((size_t)level << 24);
}

template<BitSize N, typename Structure>
void Address<N, Structure>::setLevelCoords(Coordinate x, Coordinate y, Level level)
{
    this->level = level;
    this->x = x << (N-level);
    this->y = y << (N-level);
}

template<BitSize N, typename Structure>
Coordinate Address<N, Structure>::getLevelXCoord() const
{
    return x >> (N - level);
}

template<BitSize N, typename Structure>
Coordinate Address<N, Structure>::getLevelYCoord() const
{
    return y >> (N - level);
}

template<BitSize N, typename Structure>
Address<N, Structure> Address<N, Structure>::childAddress(ChildIndex index) const
{
    int xbit = (index & 0x1) ? levelCoordinateMask(level+1) : 0;
    int ybit = (index & 0x2) ? levelCoordinateMask(level+1) : 0;

    return Address<N, Structure>(x | xbit, y | ybit, level + 1);
}

template<BitSize N, typename Structure>
inline ChildIndex Address<N, Structure>::indexOnParent() const
{
    ChildIndex xoff = xbit() ? 1 : 0;
    ChildIndex yoff = ybit() ? 1 : 0 ;
    ChildIndex index = (yoff << 1) | xoff;
    return index;
}

template<BitSize N, typename Structure>
Address<N, Structure> Address<N, Structure>::nextAddressTowards(const Address<N, Structure>& address) const
{
    // assuming prefix of this is from bit 0..level is
    // the same as in the target address, and that
    // this address's bits from [level+1..32] are all
    // zero (by "bits" here we mean in both for "x"
    // and "y" coordinates.
    return Address<N, Structure>(
                x | address.xbit(level + 1),
                y | address.ybit(level + 1),
                level+1);
}


template<BitSize N, typename Structure>
PathElement Address<N, Structure>::operator[](PathIndex index) const
{
    assert (level > 0);
    assert (index < level);
    PathElement e = (xbit(1+index) ? 1 : 0) + ((ybit(1+index) ? 1 : 0) << 1);
    return e;
}


template<BitSize N, typename Structure>
PathSize    Address<N, Structure>::getPathSize() const
{
    return level;
}

//-----------------------------------------------------------------------------
// Implementation of MemUsage Template Methods
//-----------------------------------------------------------------------------

#ifdef COLLECT_MEMUSAGE

template <typename Content>
void MemUsage::add(Node<Content> *node)
{
    Count mu = node->getMemoryUsage();
    NumChildren n = node->getNumChildren();

    memByNumChildren[n]   += mu;
    totalMem              += mu;
    countByNumChildren[n] += 1;
    totalCount            += 1;
}

template <typename Content>
void MemUsage::remove(Node<Content> *node)
{
    Count mu = node->getMemoryUsage();
    NumChildren n = node->getNumChildren();

    if (mu > memByNumChildren[n])
        throw std::string("Negative mem children is not valid");
    if (countByNumChildren[n] == 0)
        throw std::string("Negative num children is not valid");

    memByNumChildren[n]   -= mu;
    totalMem              -= mu;
    countByNumChildren[n] -= 1;
    totalCount            -= 1;
}

#else

template <typename Content> void MemUsage::add(Node<Content> *node) {}
template <typename Content> void MemUsage::remove(Node<Content> *node) {}

#endif


//-----------------------------------------------------------------------------
// Implementation of StackItem Templates
//-----------------------------------------------------------------------------

template <BitSize N, typename Content, typename Structure>
StackItem<N,Content,Structure>::StackItem()
{}

template <BitSize N, typename Content, typename Structure>
StackItem<N,Content,Structure>::StackItem(Node<Content>* node, Address<N, Structure> address):
    node(node), address(address)
{}


} // end namespace quadtree
