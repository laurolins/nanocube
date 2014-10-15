#pragma once

#include <vector>
#include <string>
#include <stack>
#include <exception>
#include <stdexcept>
#include <memory>
#include <unordered_map>
#include <map>
#include <iostream>
#include <sstream>
#include <tuple>
#include <cmath>
#include <functional>

// TODO: remove this dependency from here
#include "json.hh"

namespace tree_store {

////////////////////////////////////////////////////////////////////////////////
// Forward Declarations
////////////////////////////////////////////////////////////////////////////////

template <typename TreeStore>
struct InternalNode;

template <typename TreeStore>
struct Node;

template <typename TreeStore>
struct Edge;

template <typename TreeStore>
struct LeafNode;

template <typename Config>
struct TreeStore;

template <typename TreeStore>
struct TreeStoreBuilder;



////////////////////////////////////////////////////////////////////////////////
// Declarations
////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
// TreeStoreException
//------------------------------------------------------------------------------

struct TreeStoreException: public std::runtime_error {
public:
    TreeStoreException(const std::string &message);
};

//------------------------------------------------------------------------------
// Node
//------------------------------------------------------------------------------

template <typename TreeStore>
struct Node {
public:
    using treestore_type    = TreeStore;
    using label_type        = typename treestore_type::label_type;
    using value_type        = typename treestore_type::value_type;
    using leafnode_type     = typename treestore_type::leafnode_type;
    using internalnode_type = typename treestore_type::internalnode_type;
    using node_type         = typename treestore_type::node_type;
    using edge_type         = typename treestore_type::edge_type;
    using labelhash_type    = typename treestore_type::labelhash_type;

public: // Methods

    Node();

    Node(const Node&) = delete;             // no copy operations
    Node& operator=(const Node&) = delete;  //

    Node(const Node&&) = delete;            // no move operations
    Node& operator=(const Node&&) = delete; //

    virtual ~Node();

    virtual Node* clone() const = 0;

    virtual auto asLeafNode() const -> const leafnode_type*;
    virtual auto asLeafNode()       ->       leafnode_type*;

    virtual auto asInternalNode() const -> const internalnode_type*;
    virtual auto asInternalNode()       ->       internalnode_type*;

    bool isLeafNode() const;
    bool isInternalNode() const;

};

//------------------------------------------------------------------------------
// Edge
//------------------------------------------------------------------------------

template <typename TreeStore>
struct Edge {
public:
    using treestore_type    = TreeStore;
    using label_type        = typename treestore_type::label_type;
    using value_type        = typename treestore_type::value_type;
    using leafnode_type     = typename treestore_type::leafnode_type;
    using internalnode_type = typename treestore_type::internalnode_type;
    using node_type         = typename treestore_type::node_type;
    using edge_type         = Edge<TreeStore>;
    using labelhash_type    = typename treestore_type::labelhash_type;


public:

    Edge();
    Edge(node_type *node, label_type label);

    Edge(const Edge& other) = default;
    Edge& operator=(const Edge& other) = default;

    Edge(Edge&& other);
    Edge& operator=(Edge&& other);

    bool operator<(const Edge &e) const;
    bool operator==(const Edge &e) const;

public: // Value Members

    node_type  *node;
    label_type  label;

};

//------------------------------------------------------------------------------
// InternalNode
//------------------------------------------------------------------------------

template <typename TreeStore>
struct InternalNode: public Node<TreeStore> {
public:
    using treestore_type    = TreeStore;
    using label_type        = typename treestore_type::label_type;
    using value_type        = typename treestore_type::value_type;
    using leafnode_type     = typename treestore_type::leafnode_type;
    using internalnode_type = typename treestore_type::internalnode_type;
    using node_type         = typename treestore_type::node_type;
    using edge_type         = typename treestore_type::edge_type;
    using labelhash_type    = typename treestore_type::labelhash_type;

    using children_repository_type = std::unordered_map< label_type, edge_type, labelhash_type >;
    
    using func_relabel_type = std::function<label_type(const label_type&)>;

public:

    InternalNode();

    InternalNode(const InternalNode &e);             // copy constructor
    InternalNode &operator =(const InternalNode &e); // copy assignment

    InternalNode(InternalNode &&e);                  // move constructor
    InternalNode &operator =(InternalNode &&e);      // move assignment

    virtual ~InternalNode();                                 // destructor

    auto getChild(label_type label) const -> node_type*;
    auto getOrCreateChild(label_type label, bool leaf_child, bool &created) -> node_type*;

    void  deleteChild(label_type label);

    virtual auto clone() const -> node_type*;

    virtual auto asInternalNode() const -> const InternalNode*;
    virtual auto asInternalNode()       ->       InternalNode*;
    
    void relabelPathToChildren(func_relabel_type function);

public: // Value Members

    children_repository_type children;  // Keep these in order

};

//------------------------------------------------------------------------------
// LeafNode
//------------------------------------------------------------------------------

template <typename TreeStore>
struct LeafNode: public Node<TreeStore> {
public:
    using treestore_type    = TreeStore;
    using label_type        = typename treestore_type::label_type;
    using value_type        = typename treestore_type::value_type;
    using leafnode_type     = typename treestore_type::leafnode_type;
    using internalnode_type = typename treestore_type::internalnode_type;
    using node_type         = typename treestore_type::node_type;
    using edge_type         = typename treestore_type::edge_type;
    using labelhash_type    = typename treestore_type::labelhash_type;


public:
    LeafNode();

    virtual ~LeafNode();

    void setValue(value_type value);

    LeafNode(const LeafNode&);             // copy ctor
    LeafNode& operator=(const LeafNode&);  // copy assign

    LeafNode(LeafNode&&);            // move ctor
    LeafNode& operator=(LeafNode&&); // move assign

    virtual auto asLeafNode() const -> const LeafNode*;
    virtual auto asLeafNode()       ->       LeafNode*;

    virtual auto clone() const -> node_type*;

public: // Value Members
    value_type value;
};

//------------------------------------------------------------------------------
// TreeStore
//------------------------------------------------------------------------------

template <typename Config>
struct TreeStore {
public:

    using treestore_type    = TreeStore;
    using config_type       = Config;
    using label_type        = typename Config::label_type;
    using value_type        = typename Config::value_type;
    using labelhash_type    = Config;
    using node_type         = Node<treestore_type>;
    using edge_type         = Edge<treestore_type>;
    using leafnode_type     = LeafNode<treestore_type>;
    using internalnode_type = InternalNode<treestore_type>;
    
    // B declares A as a friend...
    friend struct TreeStoreBuilder<treestore_type>;


public: // Methods

    TreeStore() = default;

    explicit TreeStore(value_type value);
    explicit TreeStore(int num_levels);

    TreeStore(const TreeStore &e);             // copy constructor
    TreeStore &operator =(const TreeStore &e); // copy assignment

    TreeStore(TreeStore &&e);                  // move constructor
    TreeStore &operator =(TreeStore &&e);      // move assignment

    ~TreeStore();                              // destructor

    bool empty() const;
    int getNumLevels() const;

    void setLevelName(int level, std::string name);

    void clear();

private:

    auto createRoot() -> node_type*;

public: // Value Members
    std::unique_ptr<node_type>  root;
    std::vector<std::string>    level_names;
};

//------------------------------------------------------------------------------
// TreeStore Builder
//------------------------------------------------------------------------------

enum StoreMode { NORMAL, INVERTED };
enum StoreOp   { ADD, SUB, MUL, DIV, SET, POW, GEQ, LEQ, LE, GT, NEQ, EQ };

template <typename TreeStore>
struct TreeStoreBuilder {
public:
    using treestore_type    = TreeStore;
    using label_type        = typename treestore_type::label_type;
    using value_type        = typename treestore_type::value_type;
    using leafnode_type     = typename treestore_type::leafnode_type;
    using internalnode_type = typename treestore_type::internalnode_type;
    using node_type         = typename treestore_type::node_type;
    using edge_type         = typename treestore_type::edge_type;
    using labelhash_type    = typename treestore_type::labelhash_type;


public:

    struct Item {
        Item();
        Item(node_type *node, label_type label, bool keep);
    public:
        node_type *node;
        label_type label;
        bool       deletable;
    };

public: // Methods

    TreeStoreBuilder(treestore_type &tree_store);
    ~TreeStoreBuilder();

    void push(label_type label);
    void pop();

    void store(value_type value, StoreOp op=SET, StoreMode store_mode=NORMAL);

    int getNumLevels() const;

    void consolidateBranch();
    
    auto getCurrentNode() -> node_type*;

public: // Value Members

    treestore_type   &tree_store;
    int               current_level;
    std::vector<Item> stack;

};

//-----------------------------------------------------------------------------
// Instruction
//-----------------------------------------------------------------------------

template <typename TreeStore>
struct Instruction {
public:
    using treestore_type    = TreeStore;
    using label_type        = typename treestore_type::label_type;
    using value_type        = typename treestore_type::value_type;
    using leafnode_type     = typename treestore_type::leafnode_type;
    using internalnode_type = typename treestore_type::internalnode_type;
    using node_type         = typename treestore_type::node_type;
    using edge_type         = typename treestore_type::edge_type;
    using labelhash_type    = typename treestore_type::labelhash_type;

public:
    enum Type { PUSH, POP, STORE };

    Instruction();
    explicit Instruction(node_type *node, label_type label, bool push);
    explicit Instruction(node_type *node, value_type value);

public:

    node_type* node;
    Type  type;
    label_type label;
    value_type value;
};

//-----------------------------------------------------------------------------
// InstructionIterator
//-----------------------------------------------------------------------------

template <typename TreeStore>
struct InstructionIterator {
public:
    
    using treestore_type  = TreeStore;
    using instruction_type = Instruction<treestore_type>;
    
public:
    InstructionIterator(const TreeStore &tree_store);

    bool next();
    auto getInstruction() const -> const instruction_type&;

    void printStack() const;

public:
    const treestore_type &tree_store;
    std::vector<instruction_type> stack;
};

//-----------------------------------------------------------------------------
// TreeStoreIterator
//-----------------------------------------------------------------------------
    
    template <typename TreeStore>
    struct TreeStoreIterator {
    public:
        using treestore_type  = TreeStore;
        using node_type       = typename treestore_type::node_type;
        using edge_type       = typename treestore_type::edge_type;
    public:
        struct Item {
            Item() = default;
            Item(node_type *node, int layer);
        public:
            node_type *node { nullptr };
            int layer { 0 };
        };
    public:
        using item_type       = Item;
        
    public:
        TreeStoreIterator(node_type *root);
        TreeStoreIterator(treestore_type &treestore);
        bool next();
        item_type getCurrentItem() const;
    public:
        item_type current_item;
        node_type *root;
        std::vector<item_type> stack;
    };

    //-----------------------------------------------------------------------------
    // TreeStoreIterator Impl.
    //-----------------------------------------------------------------------------
    
    template <typename TreeStore>
    TreeStoreIterator<TreeStore>::Item::Item(node_type *node, int layer):
        node(node), layer(layer)
    {}

    template <typename TreeStore>
    TreeStoreIterator<TreeStore>::TreeStoreIterator(node_type *root):
        root(root)
    {
        if (!root)
            throw std::runtime_error("TreeStoreIterator with null root is invalid");
        stack.push_back({this->root, 0});
    }
    
    template <typename TreeStore>
    TreeStoreIterator<TreeStore>::TreeStoreIterator(treestore_type &treestore):
        root(treestore.root.get())
    {
        if (!root)
            throw std::runtime_error("TreeStoreIterator with null root is invalid");
        stack.push_back({this->root, 0});
    }
    
    template <typename TreeStore>
    bool TreeStoreIterator<TreeStore>::next()
    {
        if (stack.size()) {
            current_item = stack.back();
            stack.pop_back();

            if (current_item.node->isInternalNode()) {
                auto current_internal_node = current_item.node->asInternalNode();
                for (auto &it: current_internal_node->children)
                {
                    auto &e = it.second;
                    stack.push_back( {e.node, current_item.layer + 1 } );
                }
            }
            return true;
        }
        else {
            return false;
        }
    }
        
    template <typename TreeStore>
    auto TreeStoreIterator<TreeStore>::getCurrentItem() const -> item_type
    {
        return current_item;
    }
    
    
//-----------------------------------------------------------------------------
// io
//-----------------------------------------------------------------------------
    
template <typename Config>
std::ostream& operator<<(std::ostream &os, const TreeStore<Config> &v);
    
//-----------------------------------------------------------------------------
// Serialization
//-----------------------------------------------------------------------------

template <typename Config>
void serialize(const TreeStore<Config> &tree_store, std::ostream &os);

template <typename Config>
auto deserialize(std::istream &is) -> TreeStore<Config>;

//template <typename Config, typename Parameter>
//void json(const TreeStore<Config> &tree_store, std::ostream &os, const Parameter& parameter);
//
//template <typename Config, typename Parameter>
//void text(const TreeStore<Config> &tree_store, std::ostream &os, const Parameter& parameter);
    
    
////////////////////////////////////////////////////////////////////////////////
// Implementations
////////////////////////////////////////////////////////////////////////////////

//------------------------------------------------------------------------------
// Node
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
// Node Impl.
//------------------------------------------------------------------------------

template <typename T>
Node<T>::Node()
{
#ifdef DEBUG_TREE_STORE
    std::cout << "Node: canonical constructor: " << ((uint64_t) this) % 10001 << std::endl;
#endif
}

template <typename T>
Node<T>::~Node()
{
#ifdef DEBUG_TREE_STORE
    std::cout << "Node: destructor: " << ((uint64_t) this) % 10001 << std::endl;
#endif
}

template <typename T>
auto Node<T>::asLeafNode() const -> const leafnode_type* {
    return nullptr;
}

template <typename T>
auto Node<T>::asLeafNode() -> leafnode_type * {
    return nullptr;
}

template <typename T>
auto Node<T>::asInternalNode() const -> const internalnode_type *{
    return nullptr;
}

template <typename T>
auto Node<T>::asInternalNode() -> internalnode_type* {
    return nullptr;
}

template <typename T>
bool Node<T>::isLeafNode() const {
    return this->asLeafNode() != nullptr;
}

template <typename T>
bool Node<T>::isInternalNode() const {
    return this->asInternalNode() != nullptr;
}

//------------------------------------------------------------------------------
// Edge Impl.
//------------------------------------------------------------------------------

template <typename T>
Edge<T>::Edge():
    node(nullptr),
    label(0)
{}

template <typename T>
Edge<T>::Edge(node_type *node, label_type label):
    node(node), 
    label(label)
{}

    template <typename T>
Edge<T>::Edge(Edge&& other) {
    std::swap(node, other.node);
    label.swap(other.label);
}

    template <typename T>
    auto Edge<T>::operator=(Edge&& other) -> Edge& {
        std::swap(node, other.node);
        label.swap(other.label);
        return *this;
    }

    
    
template <typename T>
bool Edge<T>::operator<(const Edge &e) const {
    return label < e.label;
}

template <typename T>
bool Edge<T>::operator==(const Edge &e) const {
    return label == e.label;
}

//------------------------------------------------------------------------------
// InternalNode Impl.
//------------------------------------------------------------------------------

template <typename T>
InternalNode<T>::InternalNode()
{
#ifdef DEBUG_TREE_STORE
    std::cout << "InternalNode: canonical constructor" << std::endl;
#endif
}

template <typename T>
InternalNode<T>::~InternalNode() { // dtor
#ifdef DEBUG_TREE_STORE
    std::cout << "InternalNode: destructor" << std::endl;
#endif

    for (auto &it: children) {
        delete it.second.node;
    }
}

template <typename T>
InternalNode<T>::InternalNode(const InternalNode &other): // copy ctor
    InternalNode()
{
#ifdef DEBUG_TREE_STORE
    std::cout << "InternalNode: copy constructor" << std::endl;
#endif

    for (const auto &it: other.children) {
        const edge_type &e = it.second;
        children[e.label] = edge_type(e.node->clone(), e.label);
    }
}

template <typename T>
auto InternalNode<T>::operator=(const InternalNode &other) -> InternalNode& // copy assign
{
#ifdef DEBUG_TREE_STORE
    std::cout << "InternalNode: copy assignment" << std::endl;
#endif

    for (const auto &it: other.children) {
        const edge_type &e = it.second;
        children[e.label] = edge_type(e.node->clone(), e.label);
    }
    return *this;
}

template <typename T>
InternalNode<T>::InternalNode(InternalNode &&other): // move ctor
    InternalNode()
{
#ifdef DEBUG_TREE_STORE
    std::cout << "InternalNode: move constructor" << std::endl;
#endif

    std::swap(this->children, other.children);
}

template <typename T>
auto InternalNode<T>::operator=(InternalNode &&other) -> InternalNode&  // move assign
{
#ifdef DEBUG_TREE_STORE
    std::cout << "InternalNode: move assignment" << std::endl;
#endif

    std::swap(this->children, other.children);
    return *this;
}

template <typename T>
auto InternalNode<T>::getChild(label_type label) const -> node_type* {
    auto it = children.find(label);
    if (it == children.end()) {
        return nullptr;
    }
    else {
        return it->second.node;
    }
}

template <typename T>
auto InternalNode<T>::getOrCreateChild(label_type label, bool leaf_child, bool &created) -> node_type*
{
    using value_type = typename children_repository_type::value_type;
    
    edge_type e;
    
    auto status = children.emplace( value_type(label, e) );
    if (status.second) {
        node_type* child;
        if (leaf_child) {
            child = new leafnode_type();
        } else {
            child = new internalnode_type();
        }
        status.first->second.node  = child;
        status.first->second.label = label;
        created = true;
        return child;
    }
    else {
        created = false;
        return status.first->second.node;
    }
//    pair<iterator, bool> emplace ( Args&&... args );
//    auto it = children.find(label);
//    if (it == children.end()) {
//        node_type* child;
//        if (leaf_child) {
//            child = new leafnode_type();
//        } else {
//            child = new internalnode_type();
//        }
//        children[label] = edge_type(child,label);
//        created = true;
//        return child;
//    }
//    else {
//        return it->second.node;
//    }
}

template <typename T>
void InternalNode<T>::deleteChild(label_type label)
{
    auto it = children.find(label);
    if (it != children.end()) {
        delete it->second.node;
        children.erase(it);
    }
    else {
        throw TreeStoreException("TreeStoreBuilder::deleteChild() ... child doesn't exist");
    }
}

template <typename T>
auto InternalNode<T>::clone() const -> node_type*
{
    return new InternalNode(*this); // copy constructor
}

template <typename T>
auto InternalNode<T>::asInternalNode() const -> const InternalNode* {
    return this;
}

template <typename T>
auto InternalNode<T>::asInternalNode() -> InternalNode* {
    return this;
}

    template <typename T>
    void InternalNode<T>::relabelPathToChildren(func_relabel_type function) {

        using value_type = typename children_repository_type::value_type;

        children_repository_type new_map;
        new_map.reserve(children.size());
        for (auto &it: children) {
            auto &e = it.second;
            auto new_label = std::move(function(it.first));
            auto status = new_map.emplace( value_type ( new_label, edge_type( e.node, new_label ) ) );
        }
        this->children.swap(new_map);
    }

    
//------------------------------------------------------------------------------
// LeafNode Impl.
//------------------------------------------------------------------------------

template <typename T>
LeafNode<T>::LeafNode()
{
#ifdef DEBUG_TREE_STORE
    std::cout << "LeafNode: canonical constructor" << std::endl;
#endif
    value = T::config_type::default_value;
}

template <typename T>
LeafNode<T>::~LeafNode()
{
#ifdef DEBUG_TREE_STORE
    std::cout << "LeafNode: destructor" << std::endl;
#endif
}

template <typename T>
void LeafNode<T>::setValue(value_type value)
{
    this->value = value;
}

template <typename T>
LeafNode<T>::LeafNode(const LeafNode &other):
    LeafNode()
{
#ifdef DEBUG_TREE_STORE
    std::cout << "LeafNode: copy constructor" << std::endl;
#endif
    this->value = other.value;
}

template <typename T>
auto LeafNode<T>::operator =(const LeafNode &other) -> LeafNode& 
{
#ifdef DEBUG_TREE_STORE
    std::cout << "LeafNode: copy assignment" << std::endl;
#endif
    this->value = other.value;
    return *this;
}

template <typename T>
LeafNode<T>::LeafNode(LeafNode&& other):
    LeafNode()
{
#ifdef DEBUG_TREE_STORE
    std::cout << "LeafNode: move constructor" << std::endl;
#endif
    std::swap(this->value, other.value);
}

template <typename T>
auto LeafNode<T>::operator =(LeafNode&& other) -> LeafNode&
{
    std::cout << "LeafNode: move assignment" << std::endl;
    std::swap(this->value, other.value);
    return *this;
}

template <typename T>
auto LeafNode<T>::clone() const -> node_type*
{
    return new LeafNode(*this); // copy constructor
}

template <typename T>
auto LeafNode<T>::asLeafNode() const -> const LeafNode*
{
    return this;
}

template <typename T>
auto LeafNode<T>::asLeafNode() -> LeafNode*
{
    return this;
}


//------------------------------------------------------------------------------
// TreeStoreBuilder::Item Impl.
//------------------------------------------------------------------------------

template <typename T>
TreeStoreBuilder<T>::Item::Item():
    node(nullptr),
    label(0),
    deletable(false)
{}

template <typename T>
TreeStoreBuilder<T>::Item::Item(node_type *node, label_type label, bool deletable):
    node(node),
    label(label),
    deletable(deletable)
{}



//------------------------------------------------------------------------------
// TreeStoreBuilder Impl.
//------------------------------------------------------------------------------

template <typename T>
TreeStoreBuilder<T>::TreeStoreBuilder(treestore_type &tree_store):
    tree_store(tree_store),
    current_level(0)
{}

template <typename T>
TreeStoreBuilder<T>::~TreeStoreBuilder()
{
    if (stack.size() > 0) {
        while (stack.size() > 1) {
            this->pop();
        }
        if (stack.back().deletable) {
            tree_store.clear();
        }
    }
}

template <typename T>
auto TreeStoreBuilder<T>::getCurrentNode() -> node_type*
{
    if (stack.size() == 0) {
        bool deletable;
        if (tree_store.empty()) {
            tree_store.createRoot();
            deletable = true;
        }
        else {
            deletable = false;
        }
        stack.push_back(Item(tree_store.root.get(), label_type(), deletable)); // keep is false until something updates it
    }
    return stack.back().node;
}

template <typename T>
void TreeStoreBuilder<T>::push(label_type label)
{
    auto current_node = this->getCurrentNode();

    auto internal_node = current_node->asInternalNode();

    if (!internal_node)
        throw TreeStoreException("TreeStoreBuilder::push() ... out of bounds");

    bool leaf_child = current_level == this->getNumLevels()-1;
    bool child_was_created = false;
    auto child = internal_node->getOrCreateChild(label, leaf_child, child_was_created);

    // might use the child_was_created flag to detect
    // deadends: paths that had nothing stored
    bool deletable = child_was_created;

    // std::cout << std::string(3*current_level, ' ') << "push addr: " << label << std::endl;

    stack.push_back(Item(child, label, deletable));
    current_level++;

}

template <typename T>
void TreeStoreBuilder<T>::pop()
{
    if (stack.size() <= 1)
        throw TreeStoreException("TreeStoreBuilder::pop() ... out of bounds");

    Item item = stack.back();

    stack.pop_back();
    current_level--;

    // std::cout << std::string(3*current_level, ' ') << "pop addr: " << item.label << std::endl;

    if (item.deletable) {
        // delete item on current node
        auto internal_node = this->getCurrentNode()->asInternalNode();
        internal_node->deleteChild(item.label);
    }
}

template <typename T>
int TreeStoreBuilder<T>::getNumLevels() const
{
    return tree_store.getNumLevels();
}

    template <typename T>
    void TreeStoreBuilder<T>::consolidateBranch()
    {
        auto it = stack.rbegin();
        while (it != stack.rend() && it->deletable == true) {
            it->deletable = false;
            it++;
        }
    }
    
template <typename T>
void TreeStoreBuilder<T>::store(value_type value, StoreOp op, StoreMode store_mode)
{

    auto eval_op = [] (value_type a, value_type b, StoreOp op, StoreMode store_mode) -> value_type{
        if (store_mode == INVERTED)
            std::swap(a,b);

        switch (op) {
        case SET:
            return b;
        case ADD:
            return a + b;
        case SUB:
            return a - b;
        case MUL:
            return a * b;
        case DIV:
            return (b != 0 ? a/b : 0.0);
        case POW:
            return std::pow(a,b);
        case GEQ:
            return (a >= b ? 1.0 : 0.0);
        case LEQ:
            return (a <= b ? 1.0 : 0.0);
        case LE:
            return (a <  b ? 1.0 : 0.0);
        case GT:
            return (a >  b ? 1.0 : 0.0);
        case NEQ:
            return (a != b ? 1.0 : 0.0);
        case EQ:
            return (a == b ? 1.0 : 0.0);
        default:
            throw TreeStoreException("Operation not implemented");
        }
    };

    auto leaf_node = this->getCurrentNode()->asLeafNode();
    if (!leaf_node) {
        throw TreeStoreException("TreeStoreBuilder::store() ... can only store on last level");
    }
    leaf_node->setValue(eval_op(leaf_node->value, value, op, store_mode));

    // std::cout << std::string(3*current_level, ' ') << "store: " << value << std::endl;

    // current path is not deletable, since we stored some data into it
    
    consolidateBranch();
//    auto it = stack.rbegin();
//    while (it != stack.rend() && it->deletable == true) {
//        it->deletable = false;
//        it++;
//    }
}

//-----------------------------------------------------------------------------
// Instruction Impl.
//-----------------------------------------------------------------------------

template <typename T>
Instruction<T>::Instruction():
    node(nullptr)
{}

template <typename T>
Instruction<T>::Instruction(node_type *node, label_type label, bool push):
    node(node),
    type(push ? PUSH : POP),
    label(label),
    value(0)
{}

template <typename T>
Instruction<T>::Instruction(node_type *node, value_type value):
    node(node),
    type(STORE),
    label(0),
    value(value)
{}


//-----------------------------------------------------------------------------
// InstructionIterator Impl.
//-----------------------------------------------------------------------------

template <typename T>
auto InstructionIterator<T>::getInstruction() const -> const instruction_type& {
    return stack.back();
}

template <typename T>
void InstructionIterator<T>::printStack() const
{
    int i=0;
    for (auto it = stack.rbegin();it!=stack.rend();it++) {
        std::cout << "stack[top-" << i++ << "] = " << *it << std::endl;
    }
}

template <typename T>
InstructionIterator<T>::InstructionIterator(const treestore_type &tree_store):
    tree_store(tree_store)
{
    if (tree_store.empty())
        return;
    static const bool push = true;
    stack.push_back(instruction_type(tree_store.root.get(),{},push));
}

template <typename T>
bool InstructionIterator<T>::next() {
    if (stack.empty())
        return false;

    instruction_type instruction = stack.back();
    stack.pop_back();

    if (instruction.type == instruction_type::PUSH) {
        auto node = instruction.node;
        if (node->asLeafNode()) {
            stack.push_back(instruction_type(node,node->asLeafNode()->value)); // store instruction
            // std::cout << "   stack " << stack.back() << std::endl;
        }
        else {
            auto internal_node = node->asInternalNode();
            for (auto it: internal_node->children) {
                auto &e = it.second;
                // std::cout << "   e.label " << e.label << std::endl;

                static const bool push = true;
                static const bool pop  = false;

                stack.push_back(instruction_type(e.node, e.label, pop));  // schedule a pop

                // std::cout << "   top pop label " << stack.back().label << " type " << stack.back().type << std::endl;
                // std::cout << "   top pop label " << stack.back() << std::endl;

                stack.push_back(instruction_type(e.node, e.label, push)); // schedule a push
                // std::cout << "   top push label " << stack.back().label<< " type "  << stack.back().type  << std::endl;
                // std::cout << "   top push label " << stack.back() << std::endl;

                // std::cout << "   stack " << stack.top() << std::endl;
                // std::cout << "   stack " << stack.top() << std::endl;

            }
        }
    }

    // printStack();
    return !stack.empty();
}



//------------------------------------------------------------------------------
// TreeStore Impl.
//------------------------------------------------------------------------------

template <typename C>
TreeStore<C>::TreeStore(value_type v):
    TreeStore(0)
{
    this->createRoot();
    this->root.get()->asLeafNode()->value = v;
}

template <typename C>
TreeStore<C>::TreeStore(int num_levels)

{
    for (int i=0;i<num_levels;i++) {
        this->level_names.push_back("L" + std::to_string(i));
    }
}

template <typename C>
TreeStore<C>::~TreeStore() {}

template <typename C>
bool TreeStore<C>::empty() const {
    return root.get() == nullptr;
}

template <typename C>
auto TreeStore<C>::createRoot() -> node_type*
{
    if (!empty())
        throw TreeStoreException("TreeStore::setRoot must be an empty tree_store");

    if (this->getNumLevels() > 0)
        root.reset(new internalnode_type());
    else
        root.reset(new leafnode_type());

    return root.get();
}

template <typename C>
void TreeStore<C>::clear() {
    delete root.release();
}

template <typename C>
int TreeStore<C>::getNumLevels() const
{
    return (int) this->level_names.size();
}

template <typename C>
void TreeStore<C>::setLevelName(int level, std::string name)
{
    // assert(level < this->getNumLevels());
    this->level_names[level] = name;
}

template <typename C>
TreeStore<C>::TreeStore(const TreeStore &other) // copy ctor
{
#ifdef DEBUG_TREE_STORE
    std::cout << "TreeStore: copy constructor" << std::endl;
#endif
    if (other.root)
        this->root.reset(other.root->clone());
    this->level_names = other.level_names;
}

template <typename C>
auto TreeStore<C>::operator =(const TreeStore &other) -> TreeStore& // copy assign
{
#ifdef DEBUG_TREE_STORE
    std::cout << "TreeStore: copy assignment" << std::endl;
#endif
    if (other.root)
        this->root.reset(other.root->clone());
    this->level_names = other.level_names;
    return *this;
}

template <typename C>
TreeStore<C>::TreeStore(TreeStore &&other):  // move ctor
    TreeStore()
{
#ifdef DEBUG_TREE_STORE
    std::cout << "TreeStore: move constructor" << std::endl;
#endif
    std::swap(this->root,other.root);
    this->level_names.swap(other.level_names);
}

template <typename C>
auto TreeStore<C>::operator =(TreeStore &&other) -> TreeStore&// move assign
{
#ifdef DEBUG_TREE_STORE
    std::cout << "TreeStore: move assignment" << std::endl;
#endif
    std::swap(this->root,other.root);
    this->level_names.swap(other.level_names);
    return *this;
}

    
//--------------------------------------------------------------
// Serialization
//--------------------------------------------------------------
    
    template <typename C>
    std::ostream& operator<<(std::ostream &os, const TreeStore<C> &v) {
        
        using treestore_type    = TreeStore<C>;
        using config_type       = typename treestore_type::config_type;
        using label_type        = typename treestore_type::label_type;
        //using value_type        = typename treestore_type::value_type;
        //using leafnode_type     = typename treestore_type::leafnode_type;
        //using internalnode_type = typename treestore_type::internalnode_type;
        using node_type         = typename treestore_type::node_type;
        //using edge_type         = typename treestore_type::edge_type;
        //using labelhash_type    = typename treestore_type::labelhash_type;
        
        struct Item {
            Item() = default;
            Item(const node_type *node, label_type label, int level):
            node(node),
            label(label),
            level(level)
            {}
            const node_type* node;
            label_type label;
            int level;
        };
        
        config_type config;
        
        if (v.empty()) {
            os << "empty" << std::endl;
            return os;
        }
        
        using parameter_type = typename C::parameter_type;
        parameter_type parameter;
        
        std::stack<Item> stack;
        stack.push(Item(v.root.get(),{},0));
        
        while (!stack.empty()) {
            Item item = stack.top();
            stack.pop();
            
            //        std::cout << "node: " << ((uint64_t) item.node) % 10001 << std::endl;
            //        std::cout << "node is leaf?     " << item.node->isLeafNode() << std::endl;
            //        std::cout << "node is internal? " << item.node->isInternalNode() << std::endl;
            
            os << std::string(3*item.level,' ');
            if (item.node->asLeafNode()) { // all leaves
                os << "addr: ";
                config.print_label(os, item.label);
                os << std::endl;
                os << std::string(3*item.level,' ');
                os << "   value: ";
                config.print_value(os, item.node->asLeafNode()->value, parameter);
                os << std::endl;
            }
            else {
                if (item.level == 0) {
                    os << "root" << std::endl;
                }
                else {
                    os << "addr: ";
                    config.print_label(os, item.label);
                    os << std::endl;
                }
                const auto internal_node = item.node->asInternalNode();
#ifdef USE_TREE_STORE
                for (const Edge &e: internal_node->children) {
                    stack.push(Item(e.node, e.label, item.level+1));
                }
#else
                for (const auto it: internal_node->children) {
                    const auto &e = it.second;
                    stack.push(Item(e.node, e.label, item.level+1));
                }
#endif
            }
        }
        return os;
    }


    template <typename C, typename P>
    std::ostream& text(const TreeStore<C> &v, std::ostream &os, const P& parameter) {
        
        using treestore_type    = TreeStore<C>;
        using config_type       = typename treestore_type::config_type;
        using label_type        = typename treestore_type::label_type;
        //using value_type        = typename treestore_type::value_type;
        //using leafnode_type     = typename treestore_type::leafnode_type;
        //using internalnode_type = typename treestore_type::internalnode_type;
        using node_type         = typename treestore_type::node_type;
        //using edge_type         = typename treestore_type::edge_type;
        //using labelhash_type    = typename treestore_type::labelhash_type;
        
        struct Item {
            Item() = default;
            Item(const node_type *node, label_type label, int level):
            node(node),
            label(label),
            level(level)
            {}
            const node_type* node;
            label_type label;
            int level;
        };
        
        config_type config;
        
        if (v.empty()) {
            os << "empty" << std::endl;
            return os;
        }
        
        std::stack<Item> stack;
        stack.push(Item(v.root.get(),{},0));
        
        while (!stack.empty()) {
            Item item = stack.top();
            stack.pop();
            
            //        std::cout << "node: " << ((uint64_t) item.node) % 10001 << std::endl;
            //        std::cout << "node is leaf?     " << item.node->isLeafNode() << std::endl;
            //        std::cout << "node is internal? " << item.node->isInternalNode() << std::endl;
            
            os << std::string(3*item.level,' ');
            if (item.node->asLeafNode()) { // all leaves
                os << "addr: ";
                config.print_label(os, item.label);
                os << std::endl;
                os << std::string(3*item.level,' ');
                os << "   value: ";
                config.print_value(os, item.node->asLeafNode()->value, parameter);
                os << std::endl;
            }
            else {
                if (item.level == 0) {
                    os << "root" << std::endl;
                }
                else {
                    os << "addr: ";
                    config.print_label(os, item.label);
                    os << std::endl;
                }
                const auto internal_node = item.node->asInternalNode();
#ifdef USE_TREE_STORE
                for (const Edge &e: internal_node->children) {
                    stack.push(Item(e.node, e.label, item.level+1));
                }
#else
                for (const auto &it: internal_node->children) {
                    const auto &e = it.second;
                    stack.push(Item(e.node, e.label, item.level+1));
                }
#endif
            }
        }
        return os;
    }

    
    //-----------------------------------------------------------------------------
    // Serialize Impl.
    //-----------------------------------------------------------------------------

    namespace {
        static const char STORE_COMMAND = 0xBB;
        static const char PUSH_COMMAND  = 0xAA;
        static const char POP_COMMAND   = 0xCC;
    }
    
    template <typename C>
    void serialize(const TreeStore<C> &tree_store, std::ostream &os)
    {
        
        using treestore_type    = TreeStore<C>;
        using config_type       = typename treestore_type::config_type;
        using label_type        = typename treestore_type::label_type;
        //using value_type        = typename treestore_type::value_type;
        //using leafnode_type     = typename treestore_type::leafnode_type;
        //using internalnode_type = typename treestore_type::internalnode_type;
        using node_type         = typename treestore_type::node_type;
        //using edge_type         = typename treestore_type::edge_type;
        //using labelhash_type    = typename treestore_type::labelhash_type;

        
        config_type config;
        
        enum Instruction { PUSH, POP, ROOT };
        // using Item = std::tuple<node_type*, label_type, Instruction>;
        
        struct Item {
            Item() = default;
            
            Item(const Item& other) = default;
            Item& operator=(const Item& other) = default;

            Item(Item&& other) = default;
            Item& operator=(Item&& other) = default;

            Item(node_type *n, const label_type *l, Instruction i):
                node(n),
                label(l),
                instruction(i)
            {
//                std::cout << "---- construct ----" << std::endl;
//                std::cout << "node:        " << (void*) this->node << std::endl;
//                std::cout << "label:       " << (void*) this->label << std::endl;
//                std::cout << "label size:  " << this->label->size() << std::endl;
//                std::cout << "instruction: " << (int) this->instruction << std::endl;
            }

            void print(std::ostream &os) {
                os << "---- print ----" << (void*) this->node << std::endl;
                os << "node:        " << (void*) this->node << std::endl;
                os << "label:       " << (void*) &this->label << std::endl;
                os << "label size:  " << this->label->size() << std::endl;
                os << "instruction: " << (int) this->instruction << std::endl;
            }
            
            Item(node_type *node):
                node(node),
                instruction(ROOT)
            {}
            const node_type  *node   { nullptr };
            const label_type *label  { nullptr };
            Instruction instruction;
        };
        
        
        // four bytes for number of levels
        uint16_t no_levels = static_cast<uint16_t>(tree_store.level_names.size());
        os.write(reinterpret_cast<char*>(&no_levels), sizeof(uint16_t));
        for (auto name: tree_store.level_names) {
            os.write(&name[0],name.size());
            // end of name marker:
            char zero = 0;
            os.write(&zero,sizeof(char));
        }
        
        // serialization of an empty tree_store is empty
        if (tree_store.root == nullptr)
            return;
        
        // push, pop and store instructions
        std::stack<Item> stack;
        // , label_type *label
        auto process = [&os, &stack, &config](const node_type* node) {
            if (node->isLeafNode()) {
                const auto leaf_node = node->asLeafNode();
                os.write(&STORE_COMMAND, sizeof(STORE_COMMAND));
                config.serialize_value(os, leaf_node->value);
            }
            else {
                const auto inode = node->asInternalNode();
                for (const auto &it: inode->children) {
                    const auto &e = it.second;
                    stack.push(Item(e.node, &e.label, POP));
                    stack.push(Item(e.node, &e.label, PUSH));
                }
            }
        };
        
        stack.push(Item(tree_store.root.get()));
        while (!stack.empty()) {
            
            Item item = stack.top(); // this is a copy
            
            // item.print(std::cout);
            
            stack.pop();
            
            switch (item.instruction) {
                case ROOT:
                    process(item.node);
                    break;
                case PUSH:
                    os.write(&PUSH_COMMAND, sizeof(PUSH_COMMAND));
                    config.serialize_label(os, *item.label);
                    process(item.node);
                    break;
                case POP:
                    os.write(&POP_COMMAND, sizeof(POP_COMMAND));
                    break;
            }
        }
    }
    
    //---------------
    // deserialize
    //---------------
    
    template <typename C>
    auto deserialize(std::istream &is) -> TreeStore<C>
    {
        using treestore_type    = TreeStore<C>;
        using config_type       = typename treestore_type::config_type;
        using label_type        = typename treestore_type::label_type;
        using value_type        = typename treestore_type::value_type;
        //using leafnode_type     = typename treestore_type::leafnode_type;
        //using internalnode_type = typename treestore_type::internalnode_type;
        //using node_type         = typename treestore_type::node_type;
        //using edge_type         = typename treestore_type::edge_type;
        //using labelhash_type    = typename treestore_type::labelhash_type;

        using builder_type      = TreeStoreBuilder<treestore_type>;

        config_type config;
        
        uint16_t no_levels;
        is.read(reinterpret_cast<char*>(&no_levels), sizeof(uint16_t));
        
        treestore_type v(no_levels);
        char buffer[256];
        for (int i=0;i<no_levels;i++) {
            is.getline(buffer,256,'\0');
            v.setLevelName(i, std::string(buffer));
        }
        
        builder_type vb(v);
        char ch;
        while (true) {
            is.read(&ch,1);
            if (!is)
                break;
            
            if (ch == STORE_COMMAND) {
                value_type value;
                
                config.deserialize_value(is, value);
                // is.read(reinterpret_cast<char*>(&value),sizeof(double));

                if (!is) {
                    throw TreeStoreException("deserialize problem");
                }
                vb.store(value);
            }
            
            else if (ch == PUSH_COMMAND) {
                label_type label;
                config.deserialize_label(is, label);
                // is.read(reinterpret_cast<char*>(&label),sizeof(Label));
                if (!is) {
                    throw TreeStoreException("deserialize problem");
                }
                vb.push(label);
            }
            
            else if (ch == POP_COMMAND) {
                vb.pop();
            }
        }
        
        return v;
        
    }
    
    //---------------
    // json impl.
    //---------------
    
    template <typename C, typename P>
    struct Writer {
        using treestore_type    = TreeStore<C>;
        using config_type       = typename treestore_type::config_type;
        using label_type        = typename treestore_type::label_type;
        using leafnode_type     = typename treestore_type::leafnode_type;
        using internalnode_type = typename treestore_type::internalnode_type;
        using node_type         = typename treestore_type::node_type;
        using edge_type         = typename treestore_type::edge_type;
        using parameter_type    = P;
        using format_label_func = std::function<std::string(const label_type&)>;
        
    public:

        void setFormatLabelFunction(int result_layer, format_label_func f);
        
        format_label_func getLabelFormatFunction(int result_layer);

    public:
        
        void json(const TreeStore<C> &tree_store, std::ostream &os, const parameter_type& parameter);

    public:
        std::vector<format_label_func> fmt_label_functions;
    };

    
    
    template <typename C, typename P>
    void Writer<C,P>::setFormatLabelFunction(int result_layer, format_label_func f) {
        if (fmt_label_functions.size() <= result_layer)
            fmt_label_functions.resize(result_layer+1);
        fmt_label_functions[result_layer] = f;
    }
    
    template <typename C, typename P>
    auto Writer<C,P>::getLabelFormatFunction(int result_layer) -> format_label_func {
        format_label_func default_func = [](const label_type& label) {
            config_type config; // we need to send some parameters to this guy
            std::stringstream ss;
            ss << "\"path\":";
            config.print_label(ss, label);
            return ss.str();
        };
        
        format_label_func result;
        if (fmt_label_functions.size() > result_layer)
            result = fmt_label_functions[result_layer];
        
        if (result)
            return result;
        else
            return default_func;
    }
    
    template <typename C, typename P>
    void Writer<C,P>::json(const TreeStore<C> &tree_store, std::ostream &os, const parameter_type& parameter) {
        
        using namespace json;

        using Guard     = ContextGuard;
        
        // empty result
        if (!tree_store.root) {
            
            JsonWriter writer(os);
            Guard g = writer.dict();
            {
                {
                    Guard g2 = writer.list("layers");
                    for (std::string level_name: tree_store.level_names) {
                        writer << std::string("\"" + level_name + "\"");
                    }
                }
                {
                    Guard g2 = writer.dict("root");
                }
            }

            return;
        }
        
        // int indent = 0;
        
        // auto S = [&indent]() -> std::string { return std::string(" ",3*indent); };
        // auto process_node = [&os, &]
        
        
        enum Op {BEGIN, END};
        
        using treestore_type    = TreeStore<C>;
        using config_type       = typename treestore_type::config_type;
        using label_type        = typename treestore_type::label_type;
        // using value_type        = typename treestore_type::value_type;
        using leafnode_type     = typename treestore_type::leafnode_type;
        using internalnode_type = typename treestore_type::internalnode_type;
        using node_type         = typename treestore_type::node_type;
        using edge_type         = typename treestore_type::edge_type;
        // using labelhash_type    = typename treestore_type::labelhash_type;
        
//        using Item      = std::tuple<node_type*, label_type, Op>;
        
        struct Item {
            Item() = default;
            
            Item(node_type *node, label_type label, Op op, int layer):
                node(node), label(label), op(op), layer(layer)
            {}
            
            node_type  *node { nullptr };
            label_type  label;
            Op          op;
            int         layer { 0 };
        };
        
        
        JsonWriter writer(os);
        
        std::stack<Item> stack;
        
        config_type config; // we need to send some parameters to this guy
        
        auto process = [&writer, &stack, &config, &parameter] (node_type *node, int layer) {
            if (!node)
                return;
            
            else if (node->isLeafNode()) {
                leafnode_type *leaf = node->asLeafNode();
                std::stringstream ss;
                ss << "\"val\":";
                config.print_value(ss, leaf->value, parameter);
                writer << ss.str();
            }
            else if (node->isInternalNode()) {
                internalnode_type *internal = node->asInternalNode();
                writer.list("children");
                for (auto &it: internal->children) {
                    edge_type &e = it.second;
                    stack.push(Item(e.node, e.label, END, layer+1));
                    stack.push(Item(e.node, e.label, BEGIN, layer+1));
                }
            }
        };
        
        
        Guard g = writer.dict();
        {
            Guard g2 = writer.list("layers");
            for (std::string level_name: tree_store.level_names) {
                writer << std::string("\"" + level_name + "\"");
            }
        }
        
        bool first = true;
        
        {
            stack.push(Item(tree_store.root.get(), label_type(), END, 0));
            stack.push(Item(tree_store.root.get(), label_type(), BEGIN, 0));
            
            while (!stack.empty()) {
                
                Item item = std::move(stack.top());
                stack.pop();
                
                node_type* node  = item.node;
                label_type label = item.label;
                Op         op    = item.op;
                
                if (op == BEGIN) {
                    if (first) {
                        writer.dict("root"); // pushes dict context
                    }
                    else {
                        writer.dict();
                    }

                    if (!first) {
                        auto f = getLabelFormatFunction(item.layer);
                        writer << f(label);
                        
//                        std::stringstream ss;
//                        ss << "\"path\":";
//                        config.print_label(ss, label);
//                        writer << ss.str();
                    }
                    first = false;
                    process(node, item.layer);
                }
                else if (op == END) {
                    if (node->isInternalNode())
                        writer.pop(); // pop list of children
                    writer.pop(); // pop node dictionary
                }
            }
            
        }

    }

} // tree_store namespace

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
// //------------------------------------------------------------------------------
// // Label
// //------------------------------------------------------------------------------

// std::ostream& operator<<(std::ostream& os, const Label& label);

// template <typename Container> // we can make this generic for any container [1]
// struct hash_container {
//     using value_type = typename Container::value_type;
//     std::size_t operator()(Container const& c) const {
//         std::size_t seed = 0;
//         std::for_each(c.begin(), c.end(), [&seed](value_type v) {
//                 std::size_t vv = (std::size_t) v;
//                 seed ^= vv + 0x9e3779b9 + (seed << 6) + (seed >> 2);
//             });
//         return seed;
//     }
// };

////////////////////////////////////////////////////////////////////////////////
// TODO: bring back code below
////////////////////////////////////////////////////////////////////////////////

#if 0

std::ostream &operator<<(std::ostream &os, const Instruction::Type& t);
std::ostream &operator<<(std::ostream &os, const Instruction& i);

// output stream
std::ostream &operator<<(std::ostream &os, const TreeStore &v);

auto operator + (const TreeStore& v1, const TreeStore& v2) -> TreeStore;
auto operator - (const TreeStore& v1, const TreeStore& v2) -> TreeStore;
auto operator * (const TreeStore& v1, const TreeStore& v2) -> TreeStore;
auto operator / (const TreeStore& v1, const TreeStore& v2) -> TreeStore;
auto operator > (const TreeStore& v1, const TreeStore& v2) -> TreeStore;
auto operator >=(const TreeStore& v1, const TreeStore& v2) -> TreeStore;
auto operator < (const TreeStore& v1, const TreeStore& v2) -> TreeStore;
auto operator <=(const TreeStore& v1, const TreeStore& v2) -> TreeStore;
auto operator !=(const TreeStore& v1, const TreeStore& v2) -> TreeStore;
auto operator ==(const TreeStore& v1, const TreeStore& v2) -> TreeStore;

auto operator + (const TreeStore& v1, Value scalar) -> TreeStore;
auto operator - (const TreeStore& v1, Value scalar) -> TreeStore;
auto operator * (const TreeStore& v1, Value scalar) -> TreeStore;
auto operator / (const TreeStore& v1, Value scalar) -> TreeStore;
auto operator > (const TreeStore& v1, Value scalar) -> TreeStore;
auto operator >=(const TreeStore& v1, Value scalar) -> TreeStore;
auto operator < (const TreeStore& v1, Value scalar) -> TreeStore;
auto operator <=(const TreeStore& v1, Value scalar) -> TreeStore;
auto operator !=(const TreeStore& v1, Value scalar) -> TreeStore;
auto operator ==(const TreeStore& v1, Value scalar) -> TreeStore;


#endif


