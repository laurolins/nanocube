#include <tuple>

#include "NanoCubeQueryResult.hh"
#include "QuadTree.hh"
#include "json.hh"


typedef ::quadtree::Address<29,int> QAddr;

namespace nanocube {

QueryResult::QueryResult(TreeValue &result, ::nanocube::Schema &schema):
    result(result),
    schema(schema)
{}


void QueryResult::json(std::ostream &os) {
#if 0
    using namespace json;

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

    bool first = true;

    {
        stack.push(Item(result.root.get(), 0, END));
        stack.push(Item(result.root.get(), 0, BEGIN));

        while (!stack.empty()) {

            Item item = std::move(stack.top());
            stack.pop();

            Node* node  = std::get<0>(item);
            Label label = std::get<1>(item);
            Op    op    = std::get<2>(item);

            if (op == BEGIN) {
                if (first) {
                    writer.dict("root"); // pushes dict context
                    first = false;
                }
                else {
                    writer.dict();
                }

                std::stringstream ss;
                ss << "\"addr\":\"" << std::hex << label << "\"";
                writer << ss.str();
                process(node);
            }
            else if (op == END) {
                if (node->isInternalNode())
                    writer.pop(); // pop list of children
                writer.pop(); // pop node dictionary
            }
        }

    }
#endif
}

} // nanocube namespace
