#pragma once

#include <algorithm>
#include <iostream>
#include <cstddef>
#include <stdint.h>
#include <cassert>
#include <sstream>
#include <stack>

#include <unordered_map>

#include "QuadTreeNode.hh"

#include "cache.hh"

#include "qtfilter.hh" // used in visitSequence to create a quadtree
                       // filter based on a sequence of raw addresses

#include "geom2d/point.hh"
#include "geom2d/polygon.hh"

#include "polycover/labeled_tree.hh"

namespace quadtree
{
    
    using DimensionPath = std::vector<int>; // matching tree_store_nanocube.hh
    
    using Mask = polycover::labeled_tree::Node;

    using Cache = nanocube::Cache;

//-----------------------------------------------------------------------------
// typedefs
//-----------------------------------------------------------------------------

using RawAddress = uint64_t;

typedef int32_t  Level; // this goes up 25 (17 zooms + 8 resolution)
typedef uint32_t Coordinate;
typedef int      ChildName;
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

template<BitSize N, typename Content>
struct Iterator;

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
struct StackItemTemplate
{
    StackItemTemplate();
    StackItemTemplate(Node<Content>* node, Address<N, Structure> address);

    Node<Content>             *node;
    Address<N, Structure>      address;
};

//-----------------------------------------------------------------------------
// QuadTree
//-----------------------------------------------------------------------------

// Idea: merge quadtree and quadtree node?
// one exrea pointer per quadtree. Bad in the case
// of nested quadtrees!

template<BitSize N, typename Content>
class QuadTree
{
public:


    typedef QuadTree<N,Content>              Type;
    typedef Node<Content>                    NodeType;
    typedef Address<N, Type>                 AddressType;
    typedef Iterator<N, Content>             IteratorType;
    typedef Content                          ContentType;

    typedef std::vector<NodeType*>           NodeStackType;

    static const BitSize AddressSize = N;

private:

    typedef StackItemTemplate<N, Content, QuadTree>  _StackItemType;
    typedef std::stack<_StackItemType>       _StackType;

public:

    QuadTree(); // up to 32 levels right now
    ~QuadTree();

#if 0
    template<typename QuadTreeAddPolicy, typename Point>
    void add(AddressType address, Point &point, QuadTreeAddPolicy &addPolicy);
#endif

    // returns the deepest node that was added
    NodeType* trailProperPath(AddressType address, NodeStackType &stack);

    //
    // A path is outdated from the latest added object.
    // Make a lazy copy of this path up to the point where
    // there is a proof we can reuse part of the childs path.
    //
    void prepareProperOutdatedPath(QuadTree*             parallel_structure,
                                   AddressType           address,
                                   std::vector<void*>&   parallel_replaced_nodes,
                                   NodeStackType&        stack);

    NodeType* find(AddressType address) const;

    QuadTree* makeLazyCopy() const;

    bool isEmpty() const;

    NodeType* getRoot();

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

    // polygon visit (cache first preprocessing)
    template <typename Visitor>
    void visitExistingTreeLeaves(const Mask* mask, Visitor &visitor);

//    // mask querying
//    // template <typename Visitor>
//    void visitExistingTreeLeaves(<#const Mask *mask#>); // Visitor &visitor); //

    // visit all nodes following only proper parent-child relations
    template <typename Visitor>
    void scan(Visitor &visitor);

private:

    Node<Content> *_createPath(AddressType current_address, AddressType target_address, bool include_current_address);

public:

    NodeType* root; // the root always exist by definition

    // Collect these somewhere else!
    // Count  numAdds;
    // MemUsage counters;

};


//-----------------------------------------------------------------------------
// Iterator
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Iterator
//-----------------------------------------------------------------------------

// Iterate through all parent-child relations
template <BitSize N, typename Content>
struct Iterator {

public: // constants
    static const bool SHARED = false;
    static const bool PROPER = true;

public: // subtypes
    typedef QuadTree<N, Content>                       tree_type;
    typedef typename QuadTree<N,Content>::NodeType     node_type;
    typedef typename QuadTree<N,Content>::AddressType  address_type;
    typedef NodePointer<Content>                       node_pointer_type;

public: // constructor
    Iterator(const tree_type &tree);

public: // methods

    bool next();

    const node_type* getCurrentNode() {
        return current_item.node;
    }

    const node_type* getCurrentParentNode() {
        return current_item.parent_node;
    }

    int getCurrentLevel() const {
        return current_item.level;
    }

    std::string getLabel() const {
        return current_item.label;
    }

    bool isShared() const {
        return current_item.isShared();
    }

    bool isProper() const {
        return current_item.isProper();
    }

private:

    struct Item {
    public:

        Item():
            node(nullptr),
            parent_node(nullptr)
        {}

        Item(const node_type *node,
             const node_type *parent_node,
             std::string      label,
             int              level,
             bool             shared):
            node(node),
            parent_node(parent_node),
            label(label),
            level(level),
            shared(shared)
        {}

        bool isProper() const {
            return !shared;
        }

        bool isShared() const {
            return shared;
        }

    public: // data memebers
        const node_type *node;
        const node_type *parent_node;
        std::string label;
        int         level;
        bool        shared;
    };

    std::stack<Item> stack;

public:

    const tree_type &tree;

    Item current_item;

};

template <BitSize N, typename Content>
Iterator<N,Content>::Iterator(const tree_type& tree):
    tree(tree)
{
    stack.push(Item(tree.root,
                    nullptr,
                    "",
                    0,
                    false));
}

template <BitSize N, typename Content>
bool Iterator<N,Content>::next() {

    if (stack.empty()) {
        return false;
    }

    current_item = stack.top();
    stack.pop();

    const node_type *node = current_item.node;

    if (current_item.isProper()) {

        NumChildren num_children = node->getNumChildren();

        const ChildName *actual_indices = childEntryIndexToName[node->key()];

        const node_pointer_type *children = node->getChildrenArray();

        for (int i=0;i<num_children;i++)
        {
            const node_pointer_type &ci = children[i];

            node_type*   child_node = ci.getNode();
            std::string label = std::to_string(actual_indices[i]);
            stack.push(Item(child_node, node, label, current_item.level+1, ci.isShared()));
        }
    }

    return true;

}








//-----------------------------------------------------------------------------
// Address
//-----------------------------------------------------------------------------

typedef unsigned char PathSize;
typedef unsigned char PathElement;
typedef unsigned char PathIndex;

enum AddressRangeRelation { CONTAINED, DISJOINT, INTERSECTS_BUT_NOT_CONTAINED };

static const bool FLAG_HIGH_LEVEL_COORDS = true;
static const bool FLAG_LOW_LEVEL_COORDS = false;

template<BitSize N, typename Structure>
class Address
{
public:

    typedef Structure StructureType; // type of the structure addressed by this Address

public: // constructors

    Address(): x(0), y(0), level(0)
    {}

    // Python Code (mask of 29 bits):
    //
    //    >>> "0b" + "".join(["1"] * 29)
    //    '0b11111111111111111111111111111'
    //    >>> "%x" % eval("0b" + "".join(["1"] * 29))
    //    '1fffffff'
    //

    Address(uint64_t raw)
    {
        Coordinate x = raw & 0x1fffffffL;           // [0, 28] represent x
        Coordinate y = (raw >> 29L) & 0x1fffffffL;  // [29,57] represent y
        Level      level = (raw >> 58L) & 0x3fL;    // [58,63] represent level
        this->setLevelCoords(x,y,level);
    }

    Address(Coordinate x, Coordinate y, Level level, bool flag_level_coords=FLAG_LOW_LEVEL_COORDS):
        x(x), y(y), level(level) {
        if (flag_level_coords == FLAG_HIGH_LEVEL_COORDS) { // if coords come in high level
            this->level = level;
            this->x = x << (N-level);
            this->y = y << (N-level);
        }
    }

public: // methods

    bool read(std::istream &is);

    DimensionPath getDimensionPath() const;
    
    uint64_t raw() const;

    Address nextAddressTowards(const Address<N, Structure> &target) const;

    void eraseHigherBits();

    BitIndex levelBitIndex(Level level) const;
    Coordinate levelCoordinateMask(Level level) const;

    Address    childAddress(ChildName index) const;

    ChildName nameOnParent() const;
    void      getNamePath(std::vector<ChildName> &path) const;

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
    root(nullptr)
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
inline bool
QuadTree<N, Content>::isEmpty() const
{
    return root == nullptr;
}

template<BitSize N, typename Content>
Node<Content>* QuadTree<N,Content>::getRoot()
{
    return root;
}
    
    
    template<BitSize N, typename Content>
    template <typename Visitor>
    void QuadTree<N,Content>::visitExistingTreeLeaves(const Mask* mask, Visitor &visitor)
    {
        using StackItem = StackItemTemplate<N, Content, QuadTree<N, Content>>;
        
        std::stack< StackItem > stack;
        stack.push( StackItem(this->root, AddressType()) );
        
        // stack and mask go in sync
        std::vector<const Mask*> mask_stack;
        mask_stack.push_back(mask);
        
        while (!stack.empty())
        {
            StackItemTemplate<N, Content, QuadTree<N, Content>> &topItem = stack.top();
            NodeType*   node = topItem.node;
            AddressType addr = topItem.address;
            stack.pop();
            
            //
            auto mask_node = mask_stack.back();
            mask_stack.pop_back();
            
            if (mask_node->getNumChildren() == 0) {
                visitor.visit(node, addr);
            }
            else { // schedule next visit
                
                NumChildren num_children = node->getNumChildren();
                const ChildName *actual_indices = childEntryIndexToName[node->key()];
                NodePointer<Content>*  children = node->getChildrenArray();
                
                for (int i=0;i<num_children;i++)
                {
                    const NodePointer<Content>& ci        = children[i];
                    NodeType*                   childNode = ci.getNode();
                    // NodeType*   childNode = const_cast<NodeType*>(children[i]);
                    
                    auto actual_child_index = actual_indices[i];
                    
                    auto mask_child_node = mask_node->children[actual_child_index].get();
                    
                    if (mask_child_node != nullptr) {
                        
                        AddressType childAddr = addr.childAddress(actual_indices[i]);
                        
                        stack.push(StackItemTemplate<N, Content, QuadTree<N, Content>>(childNode, childAddr));
                        mask_stack.push_back(mask_child_node);
                        
                    }
                }
            }
        }
    } // visitExistingTreeLeaves
    

template<BitSize N, typename Content>
Node<Content>*
QuadTree<N, Content>::_createPath(AddressType current_address, AddressType target_address, bool include_current_address)
{

//    // create nodes bottom up of the right kind;
//    int n = target_address.level - current_address.level; // read all these bits

    // create a node for each bit in [min_bit, max_bit]
    const int max_level = target_address.level;
    const int min_level = current_address.level + (include_current_address ? 0 : 1);

    int n = max_level - min_level + 1;

    if (! (n > 0)) {
        throw std::string("n <= 0 on QuadTree<N, Content>::_createPath(...)");
    }

    // leaf
    Node<Content> *current = new ScopedNode<Content, NodeType0000>();

    for (int i=max_level;i>min_level;i--) {

        ChildName xoff = target_address.xbit(i) ? 1 : 0;
        ChildName yoff = target_address.ybit(i) ? 1 : 0 ;
        ChildName index = (yoff << 1) | xoff;

        Node<Content> *next_node = nullptr;
        if (index == 0) {
            ScopedNode<Content, NodeType1000> *aux = new ScopedNode<Content, NodeType1000>();
            aux->children[0].setNode(current,PROPER_FLAG);
            next_node = aux;
        } else if (index == 1) {
            ScopedNode<Content, NodeType0100> *aux = new ScopedNode<Content, NodeType0100>();
            aux->children[0].setNode(current,PROPER_FLAG);
            next_node = aux;
        } else if (index == 2) {
            ScopedNode<Content, NodeType0010> *aux = new ScopedNode<Content, NodeType0010>();
            aux->children[0].setNode(current,PROPER_FLAG);
            next_node = aux;
        } else if (index == 3) {
            ScopedNode<Content, NodeType0001> *aux = new ScopedNode<Content, NodeType0001>();
            aux->children[0].setNode(current,PROPER_FLAG);
            next_node = aux;
        }

        //
        current = next_node;
    }

    return current;
}

#if 0
template<BitSize N, typename Content>
template<typename QuadTreeAddPolicy, typename Point>
void
QuadTree<N, Content>::add(AddressType address, Point &point, QuadTreeAddPolicy &addPolicy)
{
    AddressType    current_address(0,0,0);
    NodeType* current_node  = root;
    NodeType* previous_node = nullptr;

    if (isEmpty())
    {
        // new leaf nodes
        current_node = new ScopedNode<Content,NodeType0000>(); // _newNode with nullptr: new root

        root = current_node;
        // leaf node

        // When reported of a new address in the quadtree the policy
        // might add a new Content to the node just created
        // (e.g. associate a TimeSeries object as the content
        // of the new node)
        addPolicy.newNode(current_node, current_address, address);
    }

    //
    //        Context<Content> context;
    //        context.push(root,0,currentAddress);

    while (current_address.level <= address.level)
    {

        addPolicy.addPoint(point, current_node, current_address, address);

        // add point to all collections in the trail
        // currentNode->collection.add(element); (create a more
        // general signaling mechanism where a user can accomplish
        // the our goal)

        if (current_address == address)
        {
            return; // node has been created
        }

        // assuming address is contained in the current address
        AddressType next_address = current_address.nextAddressTowards(address);

        // nextAddress index on current node
        ChildName next_node_name = next_address.nameOnParent();

        bool pointer_to_next_node_is_shared = false;
        NodeType* next_node = current_node->getChildAndShareFlag(next_node_name, pointer_to_next_node_is_shared);

        if (!next_node)
        {
            // new leaf nodes
            // nextNode = new ScopedNode<Content,NodeType0000>(); // _newNode with nullptr: new root
            next_node = _createPath(current_address,address); // new ScopedNode<Content,NodeType0000>(); // _newNode with nullptr: new root


            // create copy of currentNode with updated type
            NodeType* current_node_updated = current_node->copyWithAddedChild(next_node, next_node_name);

            //
            if (previous_node)
                previous_node->setChild(current_node_updated, current_address.nameOnParent());
            else if (current_node == root)
                root = current_node_updated;

            // delete current node
            current_node->setContentAndChildrenToNull();
            delete current_node;
            current_node = current_node_updated;

            // new address
            addPolicy.newNode(next_node, next_address, address);
        }
        else if (pointer_to_next_node_is_shared) {

            NodeType* proper_next_node = next_node->makeLazyCopy();
            current_node->setChild(proper_next_node, next_node_name, PROPER_FLAG);
            next_node = proper_next_node;

        }

        previous_node   = current_node;
        current_node    = next_node;
        current_address = next_address;
    }


    // Shouldn't get here. Should exit
    // when currentAddr == address
    throw std::string("ooops");

}
#endif


/*!
 * preparePath
 *
 * This is a special service required by nanocubes. It assumes parallel_structure (if not null)
 * was just updated with a new data point, and we are preparing to add this data point to
 * this quadtree in a resource-smart way (use of shared resources). It turns out that
 * child structure is "contained" in this quadtree except, maybe for a (suffix) path
 * containing the new data point just added in child_strucutre.
 */
template<BitSize N, typename Content>
void
QuadTree<N, Content>::prepareProperOutdatedPath(QuadTree<N, Content> *parallel_structure,
                                                AddressType           address,
                                                std::vector<void*>   &parallel_replaced_nodes,
                                                NodeStackType        &stack)
{

#if 0
    {
        std::vector<ChildName> path;
        address.getNamePath(path);
        std::cout << "prepareProperOutdatedPath: ";
        for (int child_name: path) {
            std::cout << child_name << " ";
        }
        std::cout << std::endl;
    }
#endif



    // start from root address
    AddressType  current_address(0,0,0);

    // if quadtree is empty
    if (isEmpty())
    {
        // we assume there cannot be a parallel structure if this
        // quadtree is empty, otherwise this quadtree shoudln't
        // have been created since the parent-child nodes in the
        // previous dimension could both point to parallel
        // structure
        assert (parallel_structure == nullptr);
        root = this->_createPath(current_address, address, true);
    }

    // start with root node and previous node is nullptr
    NodeType*    previous_node = nullptr;
    NodeType*    current_node  = root;
    stack.push_back(current_node);

    // pointer parallel to current_node in child_structure
    NodeType*    current_parallel_node  = (parallel_structure ? parallel_structure->root : nullptr);

    //

    // traverse the path from root address to address
    // inserting and stacking new proper nodes until
    // either we find a path-suffix that can be reused
    // or we traverse all the path
    while (current_address.level < address.level)
    {

//        std::cout << "current_address.level: " << current_address << std::endl;
//        std::cout << "address.level:         " << address         << std::endl;

        // assuming address is contained in the current address
        AddressType next_address = current_address.nextAddressTowards(address);

        // nextAddress index on current node
        ChildName next_node_name = next_address.nameOnParent();

        // get next node in the current structure and if its share flag
        bool pointer_to_next_node_is_shared = false;
        NodeType* next_node = current_node->getChildAndShareFlag(next_node_name, pointer_to_next_node_is_shared);

        // get pointer to next parallel node
        bool pointer_to_next_parallel_node_is_shared = false;
        NodeType* next_parallel_node = parallel_structure ?
                    current_parallel_node->getChildAndShareFlag(next_node_name, pointer_to_next_parallel_node_is_shared):
                    nullptr;

        // there is no next node
        if (!next_node)
        {

            // there is a parallel structure: update current node to
            // share the right child on the parallel structure
            bool shared_flag;
            if (parallel_structure) {
                next_node = next_parallel_node;
                shared_flag = true;
            }
            else {
                next_node = this->_createPath(current_address, address, false); // false == do not include current address
                shared_flag = false;
            }

            // create copy of currentNode with updated type and new child
            NodeType* current_node_updated =
                    current_node->copyWithAddedChild(next_node, next_node_name, shared_flag);

            // next_node was added to current node which was updated
            // we need to update its parent too
            if (previous_node) {
                ChildName name_on_parent  = current_address.nameOnParent();
                bool current_share_status = previous_node->isSharedPointer(name_on_parent);
                previous_node->setChild(current_node_updated, name_on_parent, current_share_status);
            }
            else if (current_node == root) {
                root = current_node_updated;
            }

            // this node might be shared by some upstream summary
            // and detecting that it was replaced is important
            parallel_replaced_nodes.push_back(current_node);

            // delete current node
            current_node->setContentAndChildrenToNull();
            delete current_node;

            // update current node
            current_node = current_node_updated;
            stack.back() = current_node;

            // if parallel structure exists we are done. only the things
            // on the stack need to have its content updated with
            // the new point.
            if (parallel_structure) {

                stack.push_back(next_node); // stack has always one node more

                break;

            }

        }
        else if (pointer_to_next_node_is_shared) {

            if (parallel_structure == nullptr) {
                std::cout << "ooops" << std::endl;
            }
            // if there is a shared parent-child link in
            // the current structure then the parallel structure
            // needs to exist
            assert(parallel_structure);

            if (next_node == next_parallel_node) {

                // not sure if this case can occur
                stack.push_back(next_node); // stack has always one node more (can be nullptr)

                break;

            }
            else if (std::count(parallel_replaced_nodes.begin(),
                                parallel_replaced_nodes.end(),
                                static_cast<void*>(next_node))) {

                //
                // is an outdated node on the parallel structure
                // means that it is actually the next_parallel_node
                // before it got updated. just keep it.
                //

                //
                // Follow the intuition here. Assume a shared parent-child
                // is needed for the new content than we should just copy
                // it from the parallel structure.
                //

                // create copy of currentNode with updated type and new child
                current_node->setChild(next_parallel_node, next_node_name, SHARED_FLAG);

                stack.push_back(next_parallel_node); // stack has always one node more (can be nullptr)

                break;
            }

            else {

                // make lazy copy of next_node: its content is going to
                // change and cannot interfere on the original version

                next_node = next_node->makeLazyCopy();

                current_node->setChild(next_node, next_node_name, PROPER_FLAG);

            }

        }

        // update current_parallel_node
        current_parallel_node = next_parallel_node;

        // update current_node, previous_node and stack
        previous_node   = current_node;
        current_node    = next_node;
        stack.push_back(current_node);

        // update current_address
        current_address = next_address;

        if (current_address.level == address.level) {

            stack.push_back(nullptr); // base case

        }

    }


#if 0
    { // CHECK IF KEY CORRESPONDS TO STORED CHILDREN

        int _key = current_node->key();
        std::cout << "updated current node key: " << _key << std::endl;

        for (int _i=0;_i<4;_i++) {
            ChildName _child_name = _i;
            if (((1 << _child_name) & _key) != 0) {
                std::cout << "   testing child: " << _child_name << std::endl;
                bool _shared;
                NodeType* _c = current_node->getChildAndShareFlag(_child_name, _shared);
                assert (_c != nullptr);
            }
        }

    } // CHECK IF KEY CORRESPONDS TO STORED CHILDREN
#endif

    // Shouldn't get here. Should exit
    // when currentAddr == address
    // return current_node; // when add root

}

template<BitSize N, typename Content>
Node<Content>*
QuadTree<N, Content>::trailProperPath(AddressType address, NodeStackType &stack)
{
    AddressType  current_address(0,0,0);
    NodeType*    current_node  = root;
    NodeType*    previous_node = nullptr;

    if (isEmpty())
    {
        // new leaf nodes
        current_node = new ScopedNode<Content,NodeType0000>(); // _newNode with nullptr: new root

        root = current_node;
        // leaf node
    }

    while (current_address.level <= address.level)
    {
        stack.push_back(current_node);

        // add point to all collections in the trail
        // currentNode->collection.add(element); (create a more
        // general signaling mechanism where a user can accomplish
        // the our goal)

        if (current_address == address)
        {
            return current_node; // node has been created
        }

        // assuming address is contained in the current address
        AddressType next_address = current_address.nextAddressTowards(address);

        // nextAddress index on current node
        ChildName next_node_name = next_address.nameOnParent();

        bool pointer_to_next_node_is_shared = false;
        NodeType* next_node = current_node->getChildAndShareFlag(next_node_name, pointer_to_next_node_is_shared);

        if (!next_node)
        {
            // new leaf nodes
            next_node = _createPath(current_address,address);

            // create copy of currentNode with updated type
            NodeType* current_node_updated = current_node->copyWithAddedChild(next_node, next_node_name, PROPER_FLAG);

            // next_node was added to current node which was updated
            // we need to update its parent too
            if (previous_node) {
                int  index_on_parent  = current_address.nameOnParent();
                bool current_share_status = previous_node->isSharedPointer(index_on_parent);
                previous_node->setChild(current_node_updated, index_on_parent, current_share_status);
            }
            else if (current_node == root) {
                root = current_node_updated;
            }

            // delete current node
            current_node->setContentAndChildrenToNull();
            delete current_node;

            // update current node
            current_node = current_node_updated;
            stack.back() = current_node;

        }
        else if (pointer_to_next_node_is_shared) {

            NodeType* proper_next_node = next_node->makeLazyCopy();
            current_node->setChild(proper_next_node, next_node_name, PROPER_FLAG);
            next_node = proper_next_node;

        }

        {
            bool shared;
            assert (current_node->getChildAndShareFlag(next_node_name, shared) != nullptr);
        }



        previous_node   = current_node;
        current_node    = next_node;
        current_address = next_address;
    }

    // Shouldn't get here. Should exit
    // when currentAddr == address
    return current_node; // when add root

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
        ChildName index = nextAddress.nameOnParent();

        NodeType* nextNode = currentNode->getChild(index);

        if (!nextNode)
            break; // there is no next node

        currentNode    = nextNode;
        currentAddress = nextAddress;
    }

    return nullptr;
}

template<BitSize N, typename Content>
QuadTree<N,Content> *QuadTree<N, Content>::makeLazyCopy() const
{
    QuadTree<N,Content> *lazy_copy = new QuadTree<N,Content>();
    if (root) {
        lazy_copy->root = root->makeLazyCopy();
    }
    return lazy_copy;
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

    std::stack<StackItemTemplate<N, Content, QuadTree<N, Content>>> stack;
    stack.push(StackItemTemplate<N, Content, QuadTree<N, Content>>(baseNode, address));

    while (!stack.empty())
    {
        StackItemTemplate<N, Content, QuadTree<N, Content>> &topItem = stack.top();
        NodeType*   node = topItem.node;
        AddressType addr = topItem.address;
        stack.pop();

        if (targetLevel < 0 || addr.level == targetLevel)
            visitor.visit(node, addr);

        if (targetLevel < 0 || addr.level < targetLevel)
        {
            NumChildren num_children = node->getNumChildren();
            const ChildName *actual_indices = childEntryIndexToName[node->key()];
            NodePointer<Content>*  children = node->getChildrenArray();

            for (int i=0;i<num_children;i++)
            {
                const NodePointer<Content> &ci = children[i];
                NodeType*   childNode = ci.getNode();
                // NodeType*   childNode = const_cast<NodeType*>(children[i]);
                AddressType childAddr = addr.childAddress(actual_indices[i]);
                stack.push(StackItemTemplate<N, Content, QuadTree<N, Content>>(childNode, childAddr));
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

    if (!(min_address.level == max_address.level &&
          min_address.x     <= max_address.x     &&
          min_address.y     <= max_address.y) ) {
        throw std::string("Invalid range addresses");
    }
        
    std::stack<StackItemTemplate<N, Content, QuadTree<N, Content>>> stack;
    stack.push(StackItemTemplate<N, Content, QuadTree<N, Content>>(root, AddressType()));

    while (!stack.empty())
    {
        StackItemTemplate<N, Content, QuadTree<N, Content>> &topItem = stack.top();
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
            const ChildName *actual_indices = childEntryIndexToName[node->key()];

            NodePointer<Content>  *children = node->getChildrenArray();

            for (int i=0;i<num_children;i++)
            {
                const NodePointer<Content> &ci = children[i];
                NodeType*   childNode = ci.getNode();
                AddressType childAddr = addr.childAddress(actual_indices[i]);
                stack.push(StackItemTemplate<N, Content, QuadTree<N, Content>>(childNode, childAddr));
            }
        }
        else {
            // std::cout << "   disjoint!" << std::endl;
        }

        // else nothing needs to be done.
    }
}


// static std::unordered_map<void*, qtfilter::Node*> cache;

template<BitSize N, typename Content>
template <typename Visitor>
void QuadTree<N,Content>::visitSequence(const std::vector<RawAddress> &seq,
                                        Visitor &visitor, Cache& cache)
{
    // preprocess and cache sequence (assuming the vector won't change)
    // be careful with multi-threading (receive a cache then)
    
    //
    // check if traversal mask is on cache
    // if it is reuse the mask, otherwise compute
    // a new mask and cache it
    //
    
    void* key = (void*) &seq; // not elegant, but should work
    auto mask = cache.getMask(key);
    if (!mask) {
        
        // assert that the raw addresses are at the same level
        // make the max intersection level be
        
        // prepare quadtree filter
        
        int level = N;
        for (RawAddress a: seq) {
            AddressType addr(a);
            level = std::min(level,addr.level);
        }
        
        geom2d::Polygon polygon;
        
        for (RawAddress a: seq) {
            geom2d::Tile tile(a);
            geom2d::Point p = tile.center();
            
            // std::cout << tile.x << ", " << tile.y << ", " <<  tile.z << std::endl;
            // std::cout << p.x    << ", " << p.y    << std::endl;
            
            polygon.add(p);
        }
        
        // polygon.save("/tmp/query.poly");
        
        // std::cout << "Polgon sides: " << polygon.size() << std::endl;
        
        //        {
        //            using namespace geom2d::io;
        //            std::cout << polygon << std::endl;
        //        }
        
        // compute the filter quadtree
        mask = qtfilter::intersect(polygon, level, true);
        cache.insertMask(key, mask);
    }
    
//    std::unique_ptr<qtfilter::Node> mask_root_node_uptr(qtfilter::intersect(polygon, level, true));
//    qtfilter::Node *mask_root_node = mask_root_node_uptr.get();
    
//        cache[(void*) &seq] = reinterpret_cast<void*>(mask_root_node);
//    }
//    else {
//        mask_root_node = reinterpret_cast<qtfilter::Node*>(it->second);
//    }
//    if (mask_root_node == nullptr)
//        return;
//    std::cout << "cached polygons: " << cache.size() << std::endl;

    // node has the filter

    // Level targetLevel = address.level + targetLevelOffset;

    // NodeType* baseNode = this->find(address);

    // if (!baseNode)
    //    return; // there is no node

    using StackItem = StackItemTemplate<N, Content, QuadTree<N, Content>>;


    std::stack< StackItem > stack;
    stack.push( StackItem(this->root, AddressType()) );

    // stack and mask go in sync
    std::vector<qtfilter::Node*> mask_stack;
    mask_stack.push_back(mask);

    while (!stack.empty())
    {
        StackItemTemplate<N, Content, QuadTree<N, Content>> &topItem = stack.top();
        NodeType*   node = topItem.node;
        AddressType addr = topItem.address;
        stack.pop();

        //
        qtfilter::Node* mask_node = mask_stack.back();
        mask_stack.pop_back();

        if (mask_node->isLeaf()) {
            visitor.visit(node, addr);
        }
        else { // schedule next visit

            NumChildren num_children = node->getNumChildren();
            const ChildName *actual_indices = childEntryIndexToName[node->key()];
            NodePointer<Content>*  children = node->getChildrenArray();

            for (int i=0;i<num_children;i++)
            {
                const NodePointer<Content>& ci        = children[i];
                NodeType*                   childNode = ci.getNode();
                // NodeType*   childNode = const_cast<NodeType*>(children[i]);

                auto actual_child_index = actual_indices[i];

                qtfilter::Node* mask_child_node = mask_node->getChildren(actual_child_index);

                if (mask_child_node != nullptr) {

                    AddressType childAddr = addr.childAddress(actual_indices[i]);

                    stack.push(StackItemTemplate<N, Content, QuadTree<N, Content>>(childNode, childAddr));
                    mask_stack.push_back(mask_child_node);

                }
            }

        } // internal node
        
    } // stack is emtpy?
    
} // visit sequence

    
    
    

    
// visit all subnodes of a certain node in the
// requested target level.
template<BitSize N, typename Content>
template <typename Visitor>
void QuadTree<N,Content>::scan(Visitor &visitor)
{
    if (this->isEmpty()) // empty
        return;

    _StackType stack;
    stack.push(_StackItemType(root, AddressType()));

    while (!stack.empty())
    {
        _StackItemType &topItem = stack.top();
        NodeType*   node = topItem.node;
        AddressType addr = topItem.address;
        stack.pop();

        visitor.visit(node, addr);

        NumChildren num_children = node->getNumChildren();
        const ChildName           *actual_indices = childEntryIndexToName[node->key()];
        const NodePointer<Content> *children_pointers = node->getChildrenArray();

        for (int i=0;i<num_children;i++)
        {
            const NodePointer<Content> &child_pointer = children_pointers[i];
            if (child_pointer.isShared()) {
                continue; // do not push shared nodes (only proper ones)
            }
            NodeType*   childNode = child_pointer.getNode();
            AddressType childAddr = addr.childAddress(actual_indices[i]);
            stack.push(_StackItemType(childNode, childAddr));
        }
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
    DimensionPath Address<N, Structure>::getDimensionPath() const {
        DimensionPath result;
        result.reserve(this->level);
        for (int i=0;i<this->level;++i) {

            int xx = xbit(i+1) ? 1 : 0;
            int yy = ybit(i+1) ? 1 : 0;
            
            int label = xx + 2 * yy;
        
            result.push_back(label);

        }
        return result;
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
    if (! (max_addr.level == target_level && target_level >= this->level) ) {
        
        throw std::string("Invalid range on Address::getRangeRelation()");

    }

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
Address<N, Structure> Address<N, Structure>::childAddress(ChildName child_name) const
{
    int xbit = (child_name & 0x1) ? levelCoordinateMask(level+1) : 0;
    int ybit = (child_name & 0x2) ? levelCoordinateMask(level+1) : 0;

    return Address<N, Structure>(x | xbit, y | ybit, level + 1);
}

template<BitSize N, typename Structure>
inline ChildName Address<N, Structure>::nameOnParent() const
{
    ChildName xoff = xbit() ? 1 : 0;
    ChildName yoff = ybit() ? 1 : 0 ;
    ChildName index = (yoff << 1) | xoff;
    return index;
}


template<BitSize N, typename Structure>
inline void Address<N, Structure>::getNamePath(std::vector<ChildName> &path) const
{
    path.clear();
    for (Level i=0;i<level;i++) {
        ChildName xoff = xbit(i) ? 1 : 0;
        ChildName yoff = ybit(i) ? 1 : 0 ;
        ChildName name = (yoff << 1) | xoff;
        path.push_back(name);
    }
}


template<BitSize N, typename Structure>
uint64_t Address<N, Structure>::raw() const {
    uint64_t ll = level;
    uint64_t xx = getLevelXCoord();
    uint64_t yy = getLevelYCoord();
    return  xx | (yy << 29) | (ll << 58);
}

template<BitSize N, typename Structure>
bool Address<N, Structure>::read(std::istream &is)
{
    Coordinate coords[2];
    is.read((char*) &coords[0],2 * sizeof(Coordinate));
    if (!is) {
        return false;
    }
    else {
        this->level = N;
        this->x = coords[0];
        this->y = coords[1];
        return true;
    }
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
    if (!(level > 0))
        throw std::string("level <= 0 on Address::operator[]");
    if (!(index < level))
        throw std::string("index >= level on Address::operator[]");
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
StackItemTemplate<N,Content,Structure>::StackItemTemplate()
{}

template <BitSize N, typename Content, typename Structure>
StackItemTemplate<N,Content,Structure>::StackItemTemplate(Node<Content>* node, Address<N, Structure> address):
    node(node), address(address)
{}


} // end namespace quadtree
