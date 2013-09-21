#include <NanoCubeQueryResult.hh>

#include <tuple>

#include <QuadTree.hh>


typedef ::quadtree::Address<29,int> QAddr;

namespace nanocube {

QueryResult::QueryResult(query::result::Vector &result, ::nanocube::Schema &schema):
    result(result),
    schema(schema)
{}

namespace {

enum ContextType { LIST, DICT, PLAIN };

struct JsonWriter;

//-----------------------------------------------------------------------------
// Context
//-----------------------------------------------------------------------------

struct Context {
public:
    Context() = default;
    Context(JsonWriter *writer, ContextType type);

    Context(const Context& other) = delete;            // copy is
    Context& operator=(const Context& other) = delete; // forbidden

    Context(Context&& other);
    Context& operator=(Context&& other);

    ~Context();

    auto operator<<(const std::string &st) -> Context&;

public:
    JsonWriter  *writer  { nullptr };
    std::string  start_st;
    std::string  end_st;
    std::string  sep_st;
    bool         use_sep { false };
};

struct JsonWriter {

    JsonWriter(std::ostream &os):
        os(os)
    {
        context_stack.push(Context(this, PLAIN));
    }

    JsonWriter(const JsonWriter& other) = delete;
    JsonWriter& operator=(const JsonWriter& other) = delete;

    JsonWriter& plain() {
        *this << ""; // make sure current context separators are triggered
        context_stack.push(Context(this, PLAIN));
        return *this;
    }

    JsonWriter& dict(std::string name="") {
        *this << ""; // make sure current context separators are triggered
        if (name.size() > 0)
            os << "\"" << name << "\":";
        context_stack.push(Context(this, DICT));
        return *this;
    }

    JsonWriter& list(std::string name="") {
        *this << ""; // make sure current context separators are triggered
        if (name.size() > 0)
            os << "\"" << name << "\":";
        context_stack.push(Context(this, LIST));
        return *this;
    }

    JsonWriter& pop() {
        context_stack.pop();
        return *this;
    }

    ~JsonWriter() {
        while (!context_stack.empty())
            context_stack.pop();
    }

    auto operator<<(const std::string &st) -> JsonWriter& {
        Context &context = context_stack.top();
        context << st;
        return *this;
    }

    std::ostream &os;
    std::stack<Context> context_stack;
};

//-----------------------------------------------------------------------------
// Context Implementation
//-----------------------------------------------------------------------------

Context::Context(Context&& other) {
    *this = std::move(other);
}

Context& Context::operator=(Context&& other) {
    std::swap(writer,   other.writer);
    std::swap(start_st, other.start_st);
    std::swap(end_st,   other.end_st);
    std::swap(sep_st,   other.sep_st);
    std::swap(use_sep,  other.use_sep);
    return *this;
}

Context::Context(JsonWriter *writer, ContextType type):
    writer(writer)
{
    if (type == LIST) {
        start_st = "[ ";
        end_st   = " ]";
        sep_st   = ", ";
    }
    else if (type == DICT) {
        start_st = "{ ";
        end_st   = " }";
        sep_st   = ", ";
    }
    writer->os << start_st;
}


Context::~Context() {
    if (writer)
        writer->os << end_st;
}

auto Context::operator<<(const std::string &st) -> Context& {
    if (writer) {
        if (use_sep)
            writer->os << sep_st;
        else
            use_sep = true;
        writer->os << st;
    }
    return *this;
}


struct ContextGuard {

    ContextGuard() = default;

    ContextGuard(JsonWriter *writer):
        writer(writer)
    {}

    ContextGuard(JsonWriter &writer):
        writer(&writer)
    {}

    ContextGuard(ContextGuard&& other) {
        *this = std::move(other);
    }

    ContextGuard& operator=(ContextGuard&& other) {
        std::swap(writer,other.writer);
        return *this;
    }

    ~ContextGuard()
    {
        if (writer)
            writer->context_stack.pop();
    }

    JsonWriter *writer { nullptr };
};

}

void QueryResult::json(std::ostream &os) {

    // empty result
    if (!result.root) {
        return;
    }

    // int indent = 0;

    // auto S = [&indent]() -> std::string { return std::string(" ",3*indent); };
    // auto process_node = [&os, &]


    enum Op {BEGIN, END};

    using Guard     = ContextGuard;

    using Node      = ::query::result::ResultNode;
    using Edge      = ::query::result::Edge;
    using Leaf      = ::query::result::LeafNode;
    using Internal  = ::query::result::InternalNode;
    using Label     = ::query::result::Label;

    using Item      = std::tuple<Node*, Label, Op>;

    JsonWriter writer(os);

    std::stack<Item> stack;

    auto process = [&writer, &stack] (Node *node) {
        if (!node)
            return;

        else if (node->isLeafNode()) {
            Leaf *leaf = node->asLeafNode();
            writer << std::string("\"value\":" + std::to_string(leaf->value));
        }
        else if (node->isInternalNode()) {
            Internal *internal = node->asInternalNode();
            writer.list("children");
#ifdef USE_VECTOR
            for (Edge &e: internal->children) {
                stack.push(Item(e.node, e.label, END));
                stack.push(Item(e.node, e.label, BEGIN));
            }
#else
            for (auto it: internal->children) {
                Edge &e = it.second;
                stack.push(Item(e.node, e.label, END));
                stack.push(Item(e.node, e.label, BEGIN));
            }
#endif
        }
    };

    Guard g = writer.dict();
    {
        Guard g2 = writer.list("levels");
        for (std::string level_name: result.level_names) {
            writer << std::string("\"" + level_name + "\"");
        }
    }

    stack.push(Item(result.root.get(), 0, END));
    stack.push(Item(result.root.get(), 0, BEGIN));

    while (!stack.empty()) {

        Item item = std::move(stack.top());
        stack.pop();

        Node* node  = std::get<0>(item);
        Label label = std::get<1>(item);
        Op    op    = std::get<2>(item);

        if (op == BEGIN) {
            writer.dict(); // pushes dict context
            writer << std::string("\"addr\":" + std::to_string(label));
            process(node);
        }
        else if (op == END) {
            if (node->isInternalNode())
                writer.pop(); // pop list of children
            writer.pop(); // pop node dictionary
        }
    }

//    stack.push(Item(result.root, 0, Gu));
//    {
//        Guard g2 = writer.list("root");
//        for (std::string level_name: result.level_names) {
//            writer << std::string("\"" + level_name + "\"");
//        }
//    }
//    Block output(os, Block::DICT);
//    {
//        Block block(output, Block);
//        os << S() << "\"levels\":";
//        {
//            Guard guard_list(os, "[", "]");
//            Comma comma(os);
//            for (std::string level_name: result.level_names) {
//                comma << "\"" << level_name << "\"";
//            }
//        }
//    }
//    if (result.getNumLevels() > 0) {
//        ::query::result::InternalNode *root = result.root->asInternalNode();
//        for (auto it=root.children.rbegin(); it!=root.rend(); it++) {
//            ::query::result::Edge &edge = *it;
//            stack.push(Item(edge.node,
//                            Item::OPEN_FLAG,
//                            it == (root.children.rend()-1) ) );
//        }
//    }

}







} // nanocube namespace



#if 0
struct Item {

    static const bool OPEN_FLAG  = true;
    static const bool CLOSE_FLAG = false;

    Item(const ::query::result::ResultNode &node, bool flag, bool first):
        node(node), flag(flag), first(first)
    {}

    bool open() const {
        return flag == OPEN_FLAG;
    }

    bool close() const {
        return flag == CLOSE_FLAG;
    }

    const ::query::result::ResultNode &node;
    bool  flag;
    bool  first;

};

std::stack<Item> stack;
// stack.push(Item(result.root, Item::OPEN_FLAG));

// TODO: reverse this
if (result.getNumLevels() > 0) {
    ::query::result::InternalNode *root = result.root->asInternalNode();
    for (auto it=root.children.rbegin(); it!=root.rend(); it++) {
        ::query::result::Edge &edge = *it;
        stack.push(Item(edge.node,
                        Item::OPEN_FLAG,
                        it == (root.children.rend()-1) ) );
    }
}

//    for (auto it=result.root.children.rbegin(); it!=result.root.children.rend(); it++) {
//        ::query::result::ResultNode &node = *(*it);
//        stack.push(Item(node, Item::OPEN_FLAG, it == (result.root.children.rend()-1) ) );
//    }

os << "[ ";

// bool first = true;

while (!stack.empty()) {

    Item item = stack.top();
    stack.pop();

    const ::query::result::ResultNode &u = item.node;

    if (item.open()) { // open item

        if (!item.first) {
            os << ", ";
        }

        os << "{ ";

        // dimension name
        std::string dimension_name = schema.getDimensionName(u.dimension);
        std::string dimension_id = schema.dimension_keys[u.dimension];
        std::string dimension_address;

        dimension_address = std::to_string(u.address);
        os << "\"dimension\":\"" << dimension_name << "\", ";

        if (dimension_id[0] == 'q') {
            QAddr qaddr(u.address);
            dimension_address =
                    std::to_string(qaddr.getLevelXCoord()) + "," +
                    std::to_string(qaddr.getLevelYCoord()) + "," +
                    std::to_string(qaddr.level);
            os << "\"address\":\"" << dimension_address << "\"";
        }
        else {
            os << "\"address\":\"" << dimension_address << "\", ";
        }

        if (u.children.size() == 0) {
            os << ", " << "\"value\":" << u.value << " }";
        }
        else {
            os << ", \"children\":[";
            stack.push(Item(u,Item::CLOSE_FLAG,false));
            for (auto it = u.children.rbegin(); it!= u.children.rend();it++) {
                ::query::result::ResultNode &v = *(*it);
                stack.push(Item(v, Item::OPEN_FLAG, it==u.children.rend()-1));
            }
        }

    }
    else { // close item

        os << "] }";

    }
}

os << " ]";

#endif
