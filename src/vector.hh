#pragma once

#include <vector>
#include <string>
#include <stack>
#include <exception>
#include <stdexcept>
#include <memory>

namespace vector {

//------------------------------------------------------------------------------
// Forward Declaration
//------------------------------------------------------------------------------

struct InternalNode;
struct LeafNode;
struct Node;
struct Edge;
struct Vector;
struct VectorBuilder;

//------------------------------------------------------------------------------
// Label
//------------------------------------------------------------------------------

typedef uint64_t Label;
typedef double   Value;

//------------------------------------------------------------------------------
// VectorException
//------------------------------------------------------------------------------

struct VectorException: public std::runtime_error {
public:
    VectorException(const std::string &message);
};

//------------------------------------------------------------------------------
// Node
//------------------------------------------------------------------------------

struct Node {
public: // Methods

    Node();

    Node(const Node&) = delete;             // no copy operations
    Node& operator=(const Node&) = delete;  //

    Node(const Node&&) = delete;            // no move operations
    Node& operator=(const Node&&) = delete; //

    virtual ~Node();

    virtual Node* clone() const = 0;

    virtual auto asLeafNode() const -> const LeafNode*;
    virtual auto asLeafNode() -> LeafNode*;

    virtual auto asInternalNode() const -> const InternalNode*;
    virtual auto asInternalNode() -> InternalNode*;

    bool isLeafNode() const;
    bool isInternalNode() const;

};

//------------------------------------------------------------------------------
// Edge
//------------------------------------------------------------------------------
struct Edge {

    Edge();
    Edge(Node *node, Label label);

    bool operator<(const Edge &e) const;

    bool operator==(const Edge &e) const;

public: // Data Members
    Node  *node;
    Label  label;
};

//------------------------------------------------------------------------------
// InternalNode
//------------------------------------------------------------------------------

struct InternalNode: public Node {
public:

    InternalNode();

    InternalNode(const InternalNode &e);             // copy constructor
    InternalNode &operator =(const InternalNode &e); // copy assignment

    InternalNode(InternalNode &&e);                  // move constructor
    InternalNode &operator =(InternalNode &&e);      // move assignment

    virtual ~InternalNode();                                 // destructor

    auto getChild(Label label) const -> Node*;
    auto getOrCreateChild(Label label, bool leaf_child, bool &created) -> Node*;

    void  deleteChild(Label label);

    virtual auto clone() const -> Node*;

    virtual auto asInternalNode() const -> const InternalNode*;
    virtual auto asInternalNode() -> InternalNode*;

public: // Data Members
    std::vector<Edge> children;  // Keep these in order 
    // (do not keep pointer to edges please)
};

//------------------------------------------------------------------------------
// LeafNode
//------------------------------------------------------------------------------

struct LeafNode: public Node {
public:

    LeafNode();
    // LeafNode(Value value);

    virtual ~LeafNode();

    void setValue(Value value);

    LeafNode(const LeafNode&);             // copy ctor
    LeafNode& operator=(const LeafNode&);  // copy assign

    LeafNode(LeafNode&&);            // move ctor
    LeafNode& operator=(LeafNode&&); // move assign

    virtual auto asLeafNode() const -> const LeafNode*;
    virtual auto asLeafNode() -> LeafNode*;

    virtual auto clone() const -> Node*;

public: // Data Members
    Value value;
};



//------------------------------------------------------------------------------
// Vector
//------------------------------------------------------------------------------

struct Vector {

    // B declares A as a friend...
    friend struct VectorBuilder;

public: // Methods

    Vector() = default;

    explicit Vector(double value);
    explicit Vector(int num_levels);

    Vector(const Vector &e);             // copy constructor
    Vector &operator =(const Vector &e); // copy assignment

    Vector(Vector &&e);                  // move constructor
    Vector &operator =(Vector &&e);      // move assignment

    ~Vector();                           // destructor

    bool empty() const;
    int getNumLevels() const;

    void setLevelName(int level, std::string name);

    void clear();

private:

    auto createRoot() -> Node*;

public: // Data Members
    std::unique_ptr<Node>    root;
    std::vector<std::string> level_names;
};

// output stream
std::ostream &operator<<(std::ostream &os, const Vector &v);

auto operator + (const Vector& v1, const Vector& v2) -> Vector;
auto operator - (const Vector& v1, const Vector& v2) -> Vector;
auto operator * (const Vector& v1, const Vector& v2) -> Vector;
auto operator / (const Vector& v1, const Vector& v2) -> Vector;
auto operator > (const Vector& v1, const Vector& v2) -> Vector;
auto operator >=(const Vector& v1, const Vector& v2) -> Vector;
auto operator < (const Vector& v1, const Vector& v2) -> Vector;
auto operator <=(const Vector& v1, const Vector& v2) -> Vector;
auto operator !=(const Vector& v1, const Vector& v2) -> Vector;
auto operator ==(const Vector& v1, const Vector& v2) -> Vector;

auto operator + (const Vector& v1, Value scalar) -> Vector;
auto operator - (const Vector& v1, Value scalar) -> Vector;
auto operator * (const Vector& v1, Value scalar) -> Vector;
auto operator / (const Vector& v1, Value scalar) -> Vector;
auto operator > (const Vector& v1, Value scalar) -> Vector;
auto operator >=(const Vector& v1, Value scalar) -> Vector;
auto operator < (const Vector& v1, Value scalar) -> Vector;
auto operator <=(const Vector& v1, Value scalar) -> Vector;
auto operator !=(const Vector& v1, Value scalar) -> Vector;
auto operator ==(const Vector& v1, Value scalar) -> Vector;

//-----------------------------------------------------------------------------
// Serialization
//-----------------------------------------------------------------------------

void serialize(const Vector &vector, std::ostream &os);
auto deserialize(std::istream &is) -> Vector;

//-----------------------------------------------------------------------------
// Instruction
//-----------------------------------------------------------------------------

struct Instruction {

    enum Type { PUSH, POP, STORE };

    Instruction();
    explicit Instruction(Node *node, Label label, bool push);
    explicit Instruction(Node *node, Value value);

    Node* node;
    Type  type;
    Label label;
    Value value;

};

std::ostream &operator<<(std::ostream &os, const Instruction::Type& t);
std::ostream &operator<<(std::ostream &os, const Instruction& i);

//-----------------------------------------------------------------------------
// InstructionIterator
//-----------------------------------------------------------------------------

struct InstructionIterator {

    InstructionIterator(const Vector &vector);

    bool next();
    auto getInstruction() const -> const Instruction&;

    void printStack() const;

public:
    const Vector &vector;
    std::vector<Instruction> stack;
};

enum StoreMode { NORMAL, INVERTED };
enum StoreOp { ADD, SUB, MUL, DIV, SET, POW, GEQ, LEQ, LE, GT, NEQ, EQ };

//------------------------------------------------------------------------------
// Vector Builder
//------------------------------------------------------------------------------

struct VectorBuilder {

    struct Item {
        Item();
        Item(Node *node, Label label, bool keep);
    public:
        Node *node;
        Label label;
        bool  deletable;
    };

public: // Methods

    VectorBuilder(Vector &vector);
    ~VectorBuilder();

    void push(Label label);
    void pop();

    void store(Value value, StoreOp op=SET, StoreMode store_mode=NORMAL);

    int getNumLevels() const;

private:

    auto getCurrentNode() -> Node*;

public: // Data Members

    Vector           &vector;

    int               current_level;
    std::vector<Item> stack;


};

} // vector namespace



