#include <iostream>
#include <memory>
#include <cmath>

#include <stack>
#include <tuple>

#include <vector.hh>

//------------------------------------------------------------------------------
// VectorException
//------------------------------------------------------------------------------

vector::VectorException::VectorException(const std::string &message):
    std::runtime_error(message)
{}

//------------------------------------------------------------------------------
// Node Impl.
//------------------------------------------------------------------------------

vector::Node::Node()
{
#ifdef DEBUG_VECTOR
    std::cout << "Node: canonical constructor: " << ((uint64_t) this) % 10001 << std::endl;
#endif
}

vector::Node::~Node()
{
#ifdef DEBUG_VECTOR
    std::cout << "Node: destructor: " << ((uint64_t) this) % 10001 << std::endl;
#endif
}

auto vector::Node::asLeafNode() const -> const LeafNode* {
    return nullptr;
}

auto vector::Node::asLeafNode() -> LeafNode * {
    return nullptr;
}

auto vector::Node::asInternalNode() const -> const InternalNode *{
    return nullptr;
}

auto vector::Node::asInternalNode() -> InternalNode* {
    return nullptr;
}

bool vector::Node::isLeafNode() const {
    return this->asLeafNode() != nullptr;
}

bool vector::Node::isInternalNode() const {
    return this->asInternalNode() != nullptr;
}

//------------------------------------------------------------------------------
// Edge Impl.
//------------------------------------------------------------------------------

vector::Edge::Edge():
    node(nullptr),
    label(0)
{}

vector::Edge::Edge(Node *node, Label label):
    node(node), 
    label(label)
{}

bool vector::Edge::operator<(const Edge &e) const {
    return label < e.label;
}

bool vector::Edge::operator==(const Edge &e) const {
    return label == e.label;
}

//------------------------------------------------------------------------------
// InternalNode Impl.
//------------------------------------------------------------------------------

vector::InternalNode::InternalNode()
{
#ifdef DEBUG_VECTOR
    std::cout << "InternalNode: canonical constructor" << std::endl;
#endif
}

vector::InternalNode::~InternalNode() { // dtor
#ifdef DEBUG_VECTOR
    std::cout << "InternalNode: destructor" << std::endl;
#endif

#ifdef USE_VECTOR
    for (Edge &e: children) {
        delete e.node;
    }
#else
    for (auto it: children) {
        delete it.second.node;
    }
#endif
}

vector::InternalNode::InternalNode(const InternalNode &other): // copy ctor
    InternalNode()
{
#ifdef DEBUG_VECTOR
    std::cout << "InternalNode: copy constructor" << std::endl;
#endif

#ifdef USE_VECTOR
    children.reserve(other.children.size());
    for (const Edge &e: other.children) {
        children.push_back(Edge(e.node->clone(), e.label));
    }
#else
    for (auto it: children) {
        delete it.second.node;
    }
#endif
}

auto vector::InternalNode::operator=(const InternalNode &other) -> InternalNode& // copy assign
{
#ifdef DEBUG_VECTOR
    std::cout << "InternalNode: copy assignment" << std::endl;
#endif

#ifdef USE_VECTOR
    children.reserve(other.children.size());
    for (const Edge &e: other.children) {
        children.push_back(Edge(e.node->clone(), e.label));
    }
#else
    for (const auto it: other.children) {
        const Edge &e = it.second;
        children[e.label] = Edge(e.node->clone(), e.label);
    }
#endif
    return *this;
}

vector::InternalNode::InternalNode(InternalNode &&other): // move ctor
    InternalNode()
{
#ifdef DEBUG_VECTOR
    std::cout << "InternalNode: move constructor" << std::endl;
#endif

    std::swap(this->children, other.children);
}

auto vector::InternalNode::operator=(InternalNode &&other) -> InternalNode&  // move assign
{
#ifdef DEBUG_VECTOR
    std::cout << "InternalNode: move assignment" << std::endl;
#endif

    std::swap(this->children, other.children);
    return *this;
}

auto vector::InternalNode::getChild(Label label) const -> Node* {
#ifdef USE_VECTOR
    auto it = std::lower_bound(children.begin(), children.end(), Edge(nullptr, label));
    if (it != children.end() && it->label == label) {
        return it->node;
    }
    else {
        return nullptr;
    }
#else
    auto it = children.find(label);
    if (it == children.end()) {
        return nullptr;
    }
    else {
        return it->second.node;
    }
#endif
}

auto vector::InternalNode::getOrCreateChild(Label label, bool leaf_child, bool &created) -> Node*
{
#ifdef USE_VECTOR
    auto it = std::lower_bound(children.begin(), children.end(), Edge(nullptr, label));
    if (it != children.end() && it->label == label) {
        created = false;
        return it->node;
    }
    else {
        Node* child;
        if (leaf_child) {
            child = new LeafNode();
        } else {
            child = new InternalNode();
        }
        children.insert(it,Edge(child,label));
        created = true;
        return child;
    }
#else
    auto it = children.find(label);
    if (it == children.end()) {
        Node* child;
        if (leaf_child) {
            child = new LeafNode();
        } else {
            child = new InternalNode();
        }
        children[label] = Edge(child,label);
        created = true;
        return child;
    }
    else {
        return it->second.node;
    }
#endif
}

void vector::InternalNode::deleteChild(Label label)
{
#ifdef USE_VECTOR
    auto it = std::lower_bound(children.begin(), children.end(), Edge(nullptr, label));
    if (it != children.end() && it->label == label) {
        delete it->node;
        children.erase(it);
    }
    else {
        throw VectorException("VectorBuilder::deleteChild() ... child doesn't exist");
    }
#else
    auto it = children.find(label);
    if (it != children.end()) {
        delete it->second.node;
        children.erase(it);
    }
    else {
        throw VectorException("VectorBuilder::deleteChild() ... child doesn't exist");
    }
#endif
}

auto vector::InternalNode::clone() const -> Node*
{
    return new InternalNode(*this); // copy constructor
}

auto vector::InternalNode::asInternalNode() const -> const InternalNode* {
    return this;
}

auto vector::InternalNode::asInternalNode() -> InternalNode* {
    return this;
}

//------------------------------------------------------------------------------
// LeafNode Impl.
//------------------------------------------------------------------------------

vector::LeafNode::LeafNode():
    value(0.0)
{
#ifdef DEBUG_VECTOR
    std::cout << "LeafNode: canonical constructor" << std::endl;
#endif
}

vector::LeafNode::~LeafNode()
{
#ifdef DEBUG_VECTOR
    std::cout << "LeafNode: destructor" << std::endl;
#endif
}

//LeafNode::LeafNode(Value value):
//    Node(),
//    value(value)
//{
//}

void vector::LeafNode::setValue(Value value)
{
    this->value = value;
}

vector::LeafNode::LeafNode(const LeafNode &other):
    LeafNode()
{
#ifdef DEBUG_VECTOR
    std::cout << "LeafNode: copy constructor" << std::endl;
#endif
    this->value = other.value;
}

auto vector::LeafNode::operator =(const LeafNode &other) -> LeafNode& 
{
#ifdef DEBUG_VECTOR
    std::cout << "LeafNode: copy assignment" << std::endl;
#endif
    this->value = other.value;
    return *this;
}

vector::LeafNode::LeafNode(LeafNode&& other):
    LeafNode()
{
#ifdef DEBUG_VECTOR
    std::cout << "LeafNode: move constructor" << std::endl;
#endif
    std::swap(this->value, other.value);
}

auto vector::LeafNode::operator =(LeafNode&& other) -> LeafNode&
{
    std::cout << "LeafNode: move assignment" << std::endl;
    std::swap(this->value, other.value);
    return *this;
}

auto vector::LeafNode::clone() const -> Node*
{
    return new LeafNode(*this); // copy constructor
}

auto vector::LeafNode::asLeafNode() const -> const LeafNode*
{
    return this;
}

auto vector::LeafNode::asLeafNode() -> LeafNode*
{
    return this;
}


//------------------------------------------------------------------------------
// Vector Impl.
//------------------------------------------------------------------------------

vector::Vector::Vector(double v):
    Vector(0)
{
    this->createRoot();
    this->root.get()->asLeafNode()->value = v;
}

vector::Vector::Vector(int num_levels)

{
    for (int i=0;i<num_levels;i++) {
        this->level_names.push_back("L" + std::to_string(i));
    }
}

vector::Vector::~Vector() {}

bool vector::Vector::empty() const {
    return root.get() == nullptr;
}

auto vector::Vector::createRoot() -> Node* 
{
    if (!empty())
        throw VectorException("Vector::setRoot must be an empty vector");

    if (this->getNumLevels() > 0)
        root.reset(new InternalNode());
    else
        root.reset(new LeafNode());

    return root.get();
}

void vector::Vector::clear() {
    delete root.release();
}

int vector::Vector::getNumLevels() const
{
    return this->level_names.size();
}

void vector::Vector::setLevelName(int level, std::string name)
{
    // assert(level < this->getNumLevels());
    this->level_names[level] = name;
}

//            if (node->isLeafNode()) {
//                LeafNode *leaf_node = node->asLeafNode();
//                os.write(&STORE_COMMAND, sizeof(STORE_COMMAND));

//                double value = leaf_node->value;
//                os.write(static_cast<char*>(&value), sizeof(double));
//            }
//            else {
//                InternalNode *inode = node->asInternalNode();
//                for (auto it = inode->children.rbegin();
//                     it != inode->children.rend(); ++it) {
//                    stack.push(Item(it->node, PUSH, it->label));
//                    stack.push(Item(it->node, POP , it->label));
//                }
//            }

vector::Vector::Vector(const Vector &other) // copy ctor
{
#ifdef DEBUG_VECTOR
    std::cout << "Vector: copy constructor" << std::endl;
#endif
    if (other.root)
        this->root.reset(other.root->clone());
    this->level_names = other.level_names;
}

auto vector::Vector::operator =(const Vector &other) -> Vector& // copy assign
{
#ifdef DEBUG_VECTOR
    std::cout << "Vector: copy assignment" << std::endl;
#endif
    if (other.root)
        this->root.reset(other.root->clone());
    this->level_names = other.level_names;
    return *this;
}

vector::Vector::Vector(Vector &&other):  // move ctor
    Vector()
{
#ifdef DEBUG_VECTOR
    std::cout << "Vector: move constructor" << std::endl;
#endif
    std::swap(this->root,other.root);
    this->level_names.swap(other.level_names);
}

auto vector::Vector::operator =(Vector &&other) -> Vector&// move assign
{
#ifdef DEBUG_VECTOR
    std::cout << "Vector: move assignment" << std::endl;
#endif
    std::swap(this->root,other.root);
    this->level_names.swap(other.level_names);
    return *this;
}

namespace vector {

auto inline product(const Vector &v1,
                    const Vector &v2,
                    StoreOp op) -> Vector;

auto inline product(const Vector &v1,
                    Value scalar,
                    StoreOp op) -> Vector;

auto inline product(Value scalar,
                    const Vector &v2,
                    StoreOp op) -> Vector;

}


auto inline vector::product(
        const Vector &v1,
        const Vector &v2,
        StoreOp op) -> Vector
{
    // is it a scalar with a non-scalar vector?
    if (v1.getNumLevels() != v2.getNumLevels()) {
        if (v2.getNumLevels() == 0) {
            double value = v2.empty() ? 0.0 : v2.root.get()->asLeafNode()->value;
            return product(v1, value, op);
        }
        else if (v1.getNumLevels() == 0) {
            double value = v2.empty() ? 0.0 : v2.root->asLeafNode()->value;
            return product(value, v2, op);
        }
        else {
            throw VectorException("Promotion different from scalar to vector is not available");
        }
    }

//    if (!(v1.getNumLevels() == v2.getNumLevels() || v1.getNumLevels() == 0 || v2.getNumLevels() == 0))
//        throw VectorException("v1 + v2 only defined to vectors with same depth");

//    if (v1.empty()) {
//        return v2; // should trigger a copy and then a move
//    }
//    else if (v2.empty()) {
//        return v1;
//    }

    Vector result = v1; // copy

    InstructionIterator it(v2);

    VectorBuilder vb(result);
    while (it.next()) {

        const Instruction &instruction = it.getInstruction();
        if (instruction.type == Instruction::PUSH) {
            vb.push(instruction.label);
        }
        else if (instruction.type == Instruction::POP) {
            vb.pop();
        }
        else if (instruction.type == Instruction::STORE) {
            vb.store(instruction.value, op);
        }
    }

    return result; // move constructor please!
}

auto inline vector::product(const Vector& v1,
                            Value   scalar,
                            StoreOp op) -> vector::Vector
{
    if (v1.empty()) {
        return Vector(v1.getNumLevels());
        // return Vector(0.0);
        // throw new VectorException("For now we don't support empty vector product with scalar");
    }

    Vector result = v1; // copy

    InstructionIterator it(v1);

    VectorBuilder vb(result);
    while (it.next()) {

        const Instruction &instruction = it.getInstruction();
        if (instruction.type == Instruction::PUSH) {
            vb.push(instruction.label);
        }
        else if (instruction.type == Instruction::POP) {
            vb.pop();
        }
        else if (instruction.type == Instruction::STORE) {
            vb.store(scalar, op);
        }
    }

    return result; // move constructor please!
}

auto inline vector::product(Value          scalar,
                            const Vector&  v2,
                            StoreOp        op) -> vector::Vector
{
    if (v2.empty()) {
        return Vector(v2.getNumLevels());
    }

    // throw new VectorException("For now we don't support empty vector product with scalar");

    Vector result = v2; // copy

    InstructionIterator it(v2);

    VectorBuilder vb(result);
    while (it.next()) {

        const Instruction &instruction = it.getInstruction();
        if (instruction.type == Instruction::PUSH) {
            vb.push(instruction.label);
        }
        else if (instruction.type == Instruction::POP) {
            vb.pop();
        }
        else if (instruction.type == Instruction::STORE) {
            vb.store(scalar, op, INVERTED); // inverse of op
        }
    }

    return result; // move constructor please!
}


auto vector::operator +(const Vector &v1, const Vector &v2) -> Vector {
    return product(v1, v2, ADD);
}

auto vector::operator -(const Vector &v1, const Vector &v2) -> Vector {
    return product(v1, v2, SUB);
}

auto vector::operator *(const Vector &v1, const Vector &v2) -> Vector {
    return product(v1, v2, MUL);
}

auto vector::operator /(const Vector &v1, const Vector &v2) -> Vector {
    return product(v1, v2, DIV);
}

auto vector::operator >(const Vector &v1, const Vector &v2) -> Vector {
    return product(v1, v2, GT);
}

auto vector::operator >=(const Vector &v1, const Vector &v2) -> Vector {
    return product(v1, v2, GEQ);
}

auto vector::operator <(const Vector &v1, const Vector &v2) -> Vector {
    return product(v1, v2, LE);
}

auto vector::operator <=(const Vector &v1, const Vector &v2) -> Vector {
    return product(v1, v2, LEQ);
}

auto vector::operator !=(const Vector &v1, const Vector &v2) -> Vector {
    return product(v1, v2, NEQ);
}

auto vector::operator ==(const Vector &v1, const Vector &v2) -> Vector {
    return product(v1, v2, EQ);
}

auto vector::operator +(const Vector &v1, Value scalar) -> Vector {
    return product(v1, scalar, ADD);
}

auto vector::operator -(const Vector &v1, Value scalar) -> Vector {
    return product(v1, scalar, SUB);
}

auto vector::operator *(const Vector &v1, Value scalar) -> Vector {
    return product(v1, scalar, MUL);
}

auto vector::operator /(const Vector &v1, Value scalar) -> Vector {
    return product(v1, scalar, DIV);
}

auto vector::operator >(const Vector &v1, Value scalar) -> Vector {
    return product(v1, scalar, GT);
}

auto vector::operator >=(const Vector &v1, Value scalar) -> Vector {
    return product(v1, scalar, GEQ);
}

auto vector::operator <(const Vector &v1, Value scalar) -> Vector {
    return product(v1, scalar, LE);
}

auto vector::operator <=(const Vector &v1, Value scalar) -> Vector {
    return product(v1, scalar, LEQ);
}

auto vector::operator !=(const Vector &v1, Value scalar) -> Vector {
    return product(v1, scalar, NEQ);
}

auto vector::operator ==(const Vector &v1, Value scalar) -> Vector {
    return product(v1, scalar, EQ);
}

namespace {
static const char STORE_COMMAND = 0xBB;
static const char PUSH_COMMAND  = 0xAA;
static const char POP_COMMAND   = 0xCC;
}

void vector::serialize(const Vector &vector, std::ostream &os)
{
    enum Instruction { PUSH, POP, ROOT };
    using Item = std::tuple<Node*, Label, Instruction>;

    // four bytes for number of levels
    uint16_t no_levels = static_cast<uint16_t>(vector.level_names.size());
    os.write(reinterpret_cast<char*>(&no_levels), sizeof(uint16_t));
    for (auto name: vector.level_names) {
        os.write(&name[0],name.size());
        // end of name marker:
        char zero = 0;
        os.write(&zero,sizeof(char));
    }

    // serialization of an empty vector is empty
    if (vector.root == nullptr)
        return;

    // push, pop and store instructions
    std::stack<Item> stack;
    stack.push(Item(vector.root.get(),0,ROOT));

    auto process = [&os, &stack](Node* node, Label label) {
        if (node->isLeafNode()) {
            LeafNode *leaf_node = node->asLeafNode();
            os.write(&STORE_COMMAND, sizeof(STORE_COMMAND));

            double value = leaf_node->value;
            os.write(reinterpret_cast<char*>(&value), sizeof(double));
        }
        else {
            InternalNode *inode = node->asInternalNode();

#ifdef USE_VECTOR
            for (auto it = inode->children.rbegin();
                 it != inode->children.rend(); ++it) {
                stack.push(Item(it->node, it->label, POP));
                stack.push(Item(it->node, it->label, PUSH));
            }
#else
            for (auto it: inode->children) {
                Edge &e = it.second;
                stack.push(Item(e.node, e.label, POP));
                stack.push(Item(e.node, e.label, PUSH));
            }
#endif
        }
    };

    while (!stack.empty()) {

        Item item = stack.top();

        stack.pop();

        Node*       node        = std::get<0>(item);
        Label       label       = std::get<1>(item);
        Instruction instruction = std::get<2>(item);

        if (instruction == ROOT) {
            process(node, label);
        }
        else if (instruction == PUSH) {
            os.write(&PUSH_COMMAND, sizeof(PUSH_COMMAND));
            os.write(reinterpret_cast<char*>(&label), sizeof(label));
            process(node, label);
        }
        else if (instruction == POP) {
            os.write(&POP_COMMAND, sizeof(POP_COMMAND));
        }
    }
}

auto vector::deserialize(std::istream &is) -> Vector
{
    uint16_t no_levels;
    is.read(reinterpret_cast<char*>(&no_levels), sizeof(uint16_t));

    Vector v(no_levels);
    char buffer[256];
    for (int i=0;i<no_levels;i++) {
        is.getline(buffer,256,'\0');
        v.setLevelName(i, std::string(buffer));
    }

    VectorBuilder vb(v);
    char ch;
    while (true) {
        is.read(&ch,1);
        if (!is)
            break;

        if (ch == STORE_COMMAND) {
            double value;
            is.read(reinterpret_cast<char*>(&value),sizeof(double));
            if (!is) {
                throw VectorException("deserialize problem");
            }
            vb.store(value);
        }

        else if (ch == PUSH_COMMAND) {
            Label label;
            is.read(reinterpret_cast<char*>(&label),sizeof(Label));
            if (!is) {
                throw VectorException("deserialize problem");
            }
            vb.push(label);
        }

        else if (ch == POP_COMMAND) {
            vb.pop();
        }
    }

    return v;

}











namespace vector {
    struct Item {
        Item(){}
        Item(const Node *node, Label label, int level):
            node(node),
            label(label),
            level(level)
        {}
        const Node* node;
        Label label;
        int level;
    };
}

std::ostream& vector::operator<<(std::ostream &os, const Vector &v) {

    std::stack<Item> stack;
    stack.push(Item(v.root.get(),0,0));

    while (!stack.empty()) {
        Item item = stack.top();
        stack.pop();

//        std::cout << "node: " << ((uint64_t) item.node) % 10001 << std::endl;
//        std::cout << "node is leaf?     " << item.node->isLeafNode() << std::endl;
//        std::cout << "node is internal? " << item.node->isInternalNode() << std::endl;

        os << std::string(3*item.level,' ');
        if (item.node->asLeafNode()) { // all leaves
            os << "addr: "  << item.label << std::endl;
            os << std::string(3*item.level,' ');
            os << "   value: " << item.node->asLeafNode()->value << std::endl;
        }
        else {
            if (item.level == 0) {
                os << "root" << std::endl;
            }
            else {
               os << "addr: " << item.label << std::endl;
            }
            const InternalNode *internal_node = item.node->asInternalNode();
#ifdef USE_VECTOR
            for (const Edge &e: internal_node->children) {
                stack.push(Item(e.node, e.label, item.level+1));
            }
#else
            for (const auto it: internal_node->children) {
                const Edge &e = it.second;
                stack.push(Item(e.node, e.label, item.level+1));
            }
#endif
        }
    }
    return os;
}

//-----------------------------------------------------------------------------
// Instruction
//-----------------------------------------------------------------------------

vector::Instruction::Instruction():
    node(nullptr)
{}

vector::Instruction::Instruction(Node *node, Label label, bool push):
    node(node),
    type(push ? PUSH : POP),
    label(label),
    value(0)
{}

vector::Instruction::Instruction(Node *node, Value value):
    node(node),
    type(STORE),
    label(0),
    value(value)
{}

std::ostream &vector::operator<<(std::ostream &os, const Instruction::Type& t) {

    switch(t) {
    case Instruction::PUSH:
        os << "PUSH";
        break;
    case Instruction::POP:
        os << "POP";
        break;
    case Instruction::STORE:
        os << "STORE";
        break;
    }
    return os;
}

std::ostream &vector::operator<<(std::ostream &os, const Instruction& i) {
    os << i.type
       << " "
       << ((i.type == Instruction::STORE) ? std::to_string(i.value) : std::to_string(i.label));
       // << "    label: " << i.label << " value: " << i.value;
    return os;
}

//-----------------------------------------------------------------------------
// InstructionIterator Impl.
//-----------------------------------------------------------------------------

const vector::Instruction& vector::InstructionIterator::getInstruction() const {
    return stack.back();
}

void vector::InstructionIterator::printStack() const
{
    int i=0;
    for (auto it = stack.rbegin();it!=stack.rend();it++) {
        std::cout << "stack[top-" << i++ << "] = " << *it << std::endl;
    }
}

vector::InstructionIterator::InstructionIterator(const Vector &vector):
    vector(vector)
{
    if (vector.empty())
        return;
    static const bool push = true;
    stack.push_back(Instruction(vector.root.get(),0,push));
}

bool vector::InstructionIterator::next() {
    if (stack.empty())
        return false;

    Instruction instruction = stack.back();
    stack.pop_back();

    if (instruction.type == Instruction::PUSH) {
        Node *node = instruction.node;
        if (node->asLeafNode()) {
            stack.push_back(Instruction(node,node->asLeafNode()->value)); // store instruction
            // std::cout << "   stack " << stack.back() << std::endl;
        }
        else {
            InternalNode *internal_node = node->asInternalNode();
#ifdef USE_VECTOR
            for (auto it=internal_node->children.rbegin();it!=internal_node->children.rend();it++) {
                Edge &e = *it;
#else
            for (auto it: internal_node->children) {
                Edge &e = it.second;
#endif
                // std::cout << "   e.label " << e.label << std::endl;

                static const bool push = true;
                static const bool pop  = false;

                stack.push_back(Instruction(e.node, e.label, pop));  // schedule a pop

                // std::cout << "   top pop label " << stack.back().label << " type " << stack.back().type << std::endl;
                // std::cout << "   top pop label " << stack.back() << std::endl;

                stack.push_back(Instruction(e.node, e.label, push)); // schedule a push
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
// VectorBuilder::Item Impl.
//------------------------------------------------------------------------------

vector::VectorBuilder::Item::Item():
    node(nullptr),
    label(0),
    deletable(false)
{}

vector::VectorBuilder::Item::Item(Node *node, Label label, bool deletable):
    node(node),
    label(label),
    deletable(deletable)
{}

//------------------------------------------------------------------------------
// VectorBuilder Impl.
//------------------------------------------------------------------------------

vector::VectorBuilder::VectorBuilder(Vector &vector):
    vector(vector),
    current_level(0)
{}

vector::VectorBuilder::~VectorBuilder()
{
    if (stack.size() > 0) {
        while (stack.size() > 1) {
            this->pop();
        }
        if (stack.back().deletable) {
            vector.clear();
        }
    }
}

auto vector::VectorBuilder::getCurrentNode() -> Node*
{
    if (stack.size() == 0) {
        bool deletable;
        if (vector.empty()) {
            vector.createRoot();
            deletable = true;
        }
        else {
            deletable = false;
        }
        stack.push_back(Item(vector.root.get(), 0, deletable)); // keep is false until something updates it
    }
    return stack.back().node;
}

void vector::VectorBuilder::push(Label label)
{
    Node *current_node = this->getCurrentNode();

    InternalNode *internal_node = current_node->asInternalNode();

    if (!internal_node)
        throw VectorException("VectorBuilder::push() ... out of bounds");

    bool leaf_child = current_level == this->getNumLevels()-1;
    bool child_was_created = false;
    Node *child = internal_node->getOrCreateChild(label, leaf_child, child_was_created);

    // might use the child_was_created flag to detect
    // deadends: paths that had nothing stored
    bool deletable = child_was_created;

    // std::cout << std::string(3*current_level, ' ') << "push addr: " << label << std::endl;

    stack.push_back(Item(child, label, deletable));
    current_level++;

}

void vector::VectorBuilder::pop()
{
    if (stack.size() <= 1)
        throw VectorException("VectorBuilder::pop() ... out of bounds");

    Item item = stack.back();

    stack.pop_back();
    current_level--;

    // std::cout << std::string(3*current_level, ' ') << "pop addr: " << item.label << std::endl;

    if (item.deletable) {
        // delete item on current node
        InternalNode *internal_node = this->getCurrentNode()->asInternalNode();
        internal_node->deleteChild(item.label);
    }
}

namespace vector {

inline Value eval_op(Value a, Value b, StoreOp op, StoreMode store_mode) {
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
        throw VectorException("Operation not implemented");
    }
}

} // complementary


void vector::VectorBuilder::store(Value value, StoreOp op, StoreMode store_mode)
{
    LeafNode* leaf_node = this->getCurrentNode()->asLeafNode();
    if (!leaf_node) {
        throw VectorException("VectorBuilder::store() ... can only store on last level");
    }
    leaf_node->setValue(eval_op(leaf_node->value, value, op, store_mode));

    // std::cout << std::string(3*current_level, ' ') << "store: " << value << std::endl;

    // current path is not deletable, since we stored some data into it
    auto it = stack.rbegin();
    while (it != stack.rend() && it->deletable == true) {
        it->deletable = false;
        it++;
    }
}

int vector::VectorBuilder::getNumLevels() const
{
    return vector.getNumLevels();
}

