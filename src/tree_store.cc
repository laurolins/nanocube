#include <iostream>
#include <memory>
#include <cmath>
#include <algorithm>

#include <stack>
#include <tuple>

#include "tree_store.hh"

//------------------------------------------------------------------------------
// TreeStoreException
//------------------------------------------------------------------------------

tree_store::TreeStoreException::TreeStoreException(const std::string &message):
    std::runtime_error(message)
{}


////////////////////////////////////////////////////////////////////////////////
// TODO: bring back code below
////////////////////////////////////////////////////////////////////////////////

#if 0


//------------------------------------------------------------------------------
// Label Impl.
//------------------------------------------------------------------------------

std::ostream& tree_store::operator<<(std::ostream& os, const Label& label) {
    bool first = true;
    os << "<";
    std::for_each(label.begin(), label.end(), [&first,&os](int lbl) {
            if (!first)
                os << ",";
            else
                first = false;
            os << lbl;
        });
    os << ">";
    return os;
}


namespace tree_store {

auto inline product(const TreeStore &v1,
                    const TreeStore &v2,
                    StoreOp op) -> TreeStore;

auto inline product(const TreeStore &v1,
                    Value scalar,
                    StoreOp op) -> TreeStore;

auto inline product(Value scalar,
                    const TreeStore &v2,
                    StoreOp op) -> TreeStore;

}


auto inline tree_store::product(
        const TreeStore &v1,
        const TreeStore &v2,
        StoreOp op) -> TreeStore
{
    // is it a scalar with a non-scalar tree_store?
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
            throw TreeStoreException("Promotion different from scalar to tree_store is not available");
        }
    }

//    if (!(v1.getNumLevels() == v2.getNumLevels() || v1.getNumLevels() == 0 || v2.getNumLevels() == 0))
//        throw TreeStoreException("v1 + v2 only defined to tree_stores with same depth");

//    if (v1.empty()) {
//        return v2; // should trigger a copy and then a move
//    }
//    else if (v2.empty()) {
//        return v1;
//    }

    TreeStore tree_store = v1; // copy

    InstructionIterator it(v2);

    TreeStoreBuilder vb(tree_store);
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

    return tree_store; // move constructor please!
}

auto inline tree_store::product(const TreeStore& v1,
                            Value   scalar,
                            StoreOp op) -> tree_store::TreeStore
{
    if (v1.empty()) {
        return TreeStore(v1.getNumLevels());
        // return TreeStore(0.0);
        // throw new TreeStoreException("For now we don't support empty tree_store product with scalar");
    }

    TreeStore tree_store = v1; // copy

    InstructionIterator it(v1);

    TreeStoreBuilder vb(tree_store);
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

    return tree_store; // move constructor please!
}

auto inline tree_store::product(Value          scalar,
                            const TreeStore&  v2,
                            StoreOp        op) -> tree_store::TreeStore
{
    if (v2.empty()) {
        return TreeStore(v2.getNumLevels());
    }

    // throw new TreeStoreException("For now we don't support empty tree_store product with scalar");

    TreeStore tree_store = v2; // copy

    InstructionIterator it(v2);

    TreeStoreBuilder vb(tree_store);
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

    return tree_store; // move constructor please!
}


auto tree_store::operator +(const TreeStore &v1, const TreeStore &v2) -> TreeStore {
    return product(v1, v2, ADD);
}

auto tree_store::operator -(const TreeStore &v1, const TreeStore &v2) -> TreeStore {
    return product(v1, v2, SUB);
}

auto tree_store::operator *(const TreeStore &v1, const TreeStore &v2) -> TreeStore {
    return product(v1, v2, MUL);
}

auto tree_store::operator /(const TreeStore &v1, const TreeStore &v2) -> TreeStore {
    return product(v1, v2, DIV);
}

auto tree_store::operator >(const TreeStore &v1, const TreeStore &v2) -> TreeStore {
    return product(v1, v2, GT);
}

auto tree_store::operator >=(const TreeStore &v1, const TreeStore &v2) -> TreeStore {
    return product(v1, v2, GEQ);
}

auto tree_store::operator <(const TreeStore &v1, const TreeStore &v2) -> TreeStore {
    return product(v1, v2, LE);
}

auto tree_store::operator <=(const TreeStore &v1, const TreeStore &v2) -> TreeStore {
    return product(v1, v2, LEQ);
}

auto tree_store::operator !=(const TreeStore &v1, const TreeStore &v2) -> TreeStore {
    return product(v1, v2, NEQ);
}

auto tree_store::operator ==(const TreeStore &v1, const TreeStore &v2) -> TreeStore {
    return product(v1, v2, EQ);
}

auto tree_store::operator +(const TreeStore &v1, Value scalar) -> TreeStore {
    return product(v1, scalar, ADD);
}

auto tree_store::operator -(const TreeStore &v1, Value scalar) -> TreeStore {
    return product(v1, scalar, SUB);
}

auto tree_store::operator *(const TreeStore &v1, Value scalar) -> TreeStore {
    return product(v1, scalar, MUL);
}

auto tree_store::operator /(const TreeStore &v1, Value scalar) -> TreeStore {
    return product(v1, scalar, DIV);
}

auto tree_store::operator >(const TreeStore &v1, Value scalar) -> TreeStore {
    return product(v1, scalar, GT);
}

auto tree_store::operator >=(const TreeStore &v1, Value scalar) -> TreeStore {
    return product(v1, scalar, GEQ);
}

auto tree_store::operator <(const TreeStore &v1, Value scalar) -> TreeStore {
    return product(v1, scalar, LE);
}

auto tree_store::operator <=(const TreeStore &v1, Value scalar) -> TreeStore {
    return product(v1, scalar, LEQ);
}

auto tree_store::operator !=(const TreeStore &v1, Value scalar) -> TreeStore {
    return product(v1, scalar, NEQ);
}

auto tree_store::operator ==(const TreeStore &v1, Value scalar) -> TreeStore {
    return product(v1, scalar, EQ);
}

namespace {
static const char STORE_COMMAND = 0xBB;
static const char PUSH_COMMAND  = 0xAA;
static const char POP_COMMAND   = 0xCC;
}

void tree_store::serialize(const TreeStore &tree_store, std::ostream &os)
{
    enum Instruction { PUSH, POP, ROOT };
    using Item = std::tuple<Node*, Label, Instruction>;

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
    stack.push(Item(tree_store.root.get(),{},ROOT));

    auto process = [&os, &stack](Node* node, Label label) {
        if (node->isLeafNode()) {
            LeafNode *leaf_node = node->asLeafNode();
            os.write(&STORE_COMMAND, sizeof(STORE_COMMAND));

            double value = leaf_node->value;
            os.write(reinterpret_cast<char*>(&value), sizeof(double));
        }
        else {
            InternalNode *inode = node->asInternalNode();

#ifdef USE_TREE_STORE
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

auto tree_store::deserialize(std::istream &is) -> TreeStore
{
    uint16_t no_levels;
    is.read(reinterpret_cast<char*>(&no_levels), sizeof(uint16_t));

    TreeStore v(no_levels);
    char buffer[256];
    for (int i=0;i<no_levels;i++) {
        is.getline(buffer,256,'\0');
        v.setLevelName(i, std::string(buffer));
    }

    TreeStoreBuilder vb(v);
    char ch;
    while (true) {
        is.read(&ch,1);
        if (!is)
            break;

        if (ch == STORE_COMMAND) {
            double value;
            is.read(reinterpret_cast<char*>(&value),sizeof(double));
            if (!is) {
                throw TreeStoreException("deserialize problem");
            }
            vb.store(value);
        }

        else if (ch == PUSH_COMMAND) {
            Label label;
            is.read(reinterpret_cast<char*>(&label),sizeof(Label));
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


std::ostream& tree_store::operator<<(std::ostream &os, const TreeStore &v) {

    
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
#ifdef USE_TREE_STORE
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


std::ostream &tree_store::operator<<(std::ostream &os, const Instruction::Type& t) {

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

std::ostream &tree_store::operator<<(std::ostream &os, const Instruction& i) {
    os << i.type
       << " ";
    if (i.type == Instruction::STORE)
        os << std::to_string(i.value);
    else
        os << i.label;
    return os;
}

#endif
