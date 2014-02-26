#include "Report.hh"

#include <unordered_set>
#include <cassert>
#include <stdexcept>

namespace report {

//------------------------------------------------------------------------------
// Layer Impl.
//------------------------------------------------------------------------------

std::ostream& operator<<(std::ostream &os, const Layer& layer) {
    os << "report::Layer["
       << " dim=" << layer.layer_id.dimension
       << ", level=" << layer.layer_id.level
       << ", num_nodes=" << layer.num_nodes
       << ", proper_children=" << layer.num_proper_children
       << ", shared_children=" << layer.num_shared_children
       << ", proper_content=" << layer.num_proper_content
       << ", shared_content=" << layer.num_shared_content
       << " ]";
    return os;
}

//------------------------------------------------------------------------------
// Node Impl.
//------------------------------------------------------------------------------

bool Link::operator==(const Link &link) const {
    Node &src = *source_node;
    Node &tgt = *target_node;
    Node &link_src = *link.source_node;
    Node &link_tgt = *link.target_node;

    return (src == link_src) &&
           (tgt == link_tgt);
}

bool Link::operator<(const Link &link) const {
    return (*source_node < *link.source_node) ||
            (*source_node == *link.source_node && *target_node < *link.target_node);
}


std::ostream& operator<<(std::ostream &os, const Node& node) {
    os << "report::Node["
       << " key=" << (node.key % 1017013)
       << ", dim=" << node.layer->layer_id.dimension
       << ", lev=" << node.layer->layer_id.level
       << ", type=" << (node.type == Node::INTERNAL_NODE ? "internal" : "leaf")
       << " ] @" << (((uint64_t) &node) % 10013);
    return os;
}

//------------------------------------------------------------------------------
// Internal Node Impl.
//------------------------------------------------------------------------------

InternalNode::InternalNode(Layer *layer, uint64_t key):
    Node(Node::INTERNAL_NODE, layer, key)
{}

void InternalNode::insertChildLink(Node *child_node, bool shared, std::string label) {

    assert(child_node->layer->layer_id.dimension == layer->layer_id.dimension &&
           child_node->layer->layer_id.level - 1 == layer->layer_id.level);

    children.push_back(Link( this, child_node, (shared ? Link::SHARED : Link::PROPER) + Link::CHILD , label));

    if (shared) {
        layer->num_shared_children++;
    }
    else {
        layer->num_proper_children++;
    }

}

void InternalNode::setContent(Node *content_node, bool shared) {

    content = Link(this, content_node, (shared ? Link::SHARED : Link::PROPER) + Link::CONTENT , "");

    if (shared) {
        layer->num_shared_content++;
    }
    else {
        layer->num_proper_content++;
    }
}

//------------------------------------------------------------------------------
// LeafNode Impl.
//------------------------------------------------------------------------------

LeafNode::LeafNode(Layer *layer, uint64_t key):
    Node(Node::LEAF_NODE, layer, key),
    info("")
{}

void LeafNode::setInfo(std::string info) {
    this->info = info;
}

std::string LeafNode::getInfo() const {
    return info;
}

//------------------------------------------------------------------------------
// Report Impl.
//------------------------------------------------------------------------------

bool compare(const Layer* a, const Layer* b) {
    return (*a < *b);
}

Layer *Report::getLayer(LayerId key) {

    Layer query_layer(key);

    auto it = std::lower_bound(layers.begin(), layers.end(), &query_layer, compare);

    Layer *layer = nullptr;
    if (it == layers.end() || !((*it)->layer_id == query_layer.layer_id)) {
        layer = new Layer(key);
        layers.insert(it, layer);
    }
    else {
        layer = *it;
    }

    return layer;

}

Report::Report(int dimensions):
    dimensions(dimensions)
{}


Node* Report::getNode(uint64_t node_key) const {

    // key cannot exist on node_map
    auto it = node_map.find(node_key);
    if (it != node_map.end()) {
        return it->second;
    }
    else {
        return nullptr;
    }
}


Node* Report::insertNode(uint64_t node_key, int dimension, int level) {

    // key cannot exist on node_map
    auto it = node_map.find(node_key);
    assert(it == node_map.end());

    Layer *layer = this->getLayer(LayerId(dimension,level));
    layer->num_nodes++;

    Node *node;
    if (dimension == this->dimensions - 1) {
        node = new LeafNode(layer, node_key);
    }
    else {
        node = new InternalNode(layer, node_key);
    }

    node_map[node_key] = node;

    // std::cout << "   " << *node << std::endl;

    return node;

}

/*
void Report::insertParentChildLink(Node *node, uint64_t child_key, bool shared, std::string label) {

    auto it = node_map.find(child_key);

    Node *child_node = nullptr;
    if (it == node_map.end()) {

        LayerKey layer_key(node->layer->key.dimension, node->layer->key.level+1);

        Layer *child_layer = getLayer(layer_key);

        Node *child_node = new InternalNode(child_layer);

        node_map[child_key] = child_node;

    }
    else {
        child_node = it->second;
    }

    node->insertChildLink(child_node, shared, label);

}

void Report::setContentLink(Node *node, uint64_t content_key, bool shared) {

    auto it = node_map.find(content_key);

    Node *content_node = nullptr;
    if (it == node_map.end()) {

        content_node = this->insertNode(content_key, node->layer->key.dimension+1);

        node_map[content_key] = content_node;

    }
    else {
        content_node = it->second;
    }

    node->setContent(content_node, shared);
}
*/

bool compare_node_pointers(Node *a, Node *b) {
    return (a->layer->layer_id < b->layer->layer_id ||
                (a->layer->layer_id == b->layer->layer_id && a->key < b->key));
}

struct Map {

    void insert(Node* node) {
        node_set.insert(node);
    }

    void consolidate() {
        std::vector<Node*> nodes(node_set.begin(),node_set.end());
        std::sort(nodes.begin(),nodes.end(),compare_node_pointers);
        for (Node* node: nodes) {
            // std::cout << "Key: " << node->key << " -> " << keys.size() << std::endl;
            keys.push_back(node->key);
        }
    }

    uint64_t operator[](uint64_t old_key) {
        // std::cout << "Querying Node: " << old_key << std::endl;

        auto it = std::find(keys.begin(), keys.end(), old_key);
        if (it == keys.end()) {
            throw std::exception();
        }
        return (it - keys.begin());
    }

    std::vector<uint64_t> keys;
    std::unordered_set<Node*> node_set;
};

bool compare_nodes(Node* a, Node *b) {
    return *a < *b;
}

bool compare_links(Link* a, Link *b) {
    return *a < *b;
}

void Report::updateToHumanReadableKeys()
{
    Map map;
    for (auto it: node_map) {
        Node *node = it.second;
        map.insert(node);
    }
    map.consolidate();
    // std::cout << "Number of Nodes: " << map.node_set.size() << std::endl;

    // create nodes vector
    for (auto it: node_map) {
        Node *node = it.second;
        node->key = map[node->key];
    }
}

void Report::updateLayerIndices()
{
    int i=0;
    for (Layer *layer: this->layers) {
        layer->index = i++;
    }
}

void report_python(std::ostream &os, Report &report) {

    os << "#" << std::endl;
    os << "# g.insertNode(<key>, <dimension>, <level>, <info>)" << std::endl;
    os << "# g.insertLink(<src_key>, <dst_key>, <label>, <content-or-child>, <shared-or-proper>)" << std::endl;
    os << "#" << std::endl;

    os << std::endl;


    std::vector<Node*> nodes;
    std::vector<Link*> links;

    // create nodes vector
    for (auto it: report.node_map) {
        Node *node = it.second;
        nodes.push_back(node);
    }

    // create nodes vector
    for (auto it: report.node_map) {
        Node *node = it.second;
        InternalNode *internal_node = node->asInternalNode();
        if (internal_node) {
            for (Link &link: internal_node->children) {
                links.push_back(&link);
            }
            {
                Link &link = internal_node->content;
                links.push_back(&link);
            }
        }
    }

    // sort nodes and links
    std::sort(nodes.begin(), nodes.end(), compare_nodes);
    std::sort(links.begin(), links.end(), compare_links);


    os << "g = Graph()" << std::endl;
    for (Node *node: nodes) {
        os << "g.insertNode("
           << node->key << ", "
           << node->layer->layer_id.dimension << ", "
           << node->layer->layer_id.level << ", "
           << "\"" << node->getInfo() << "\"" << ")" << std::endl;
    }

    for (Link *link: links) {
        os << "g.insertLink("
           << link->source_node->key << ", "
           << link->target_node->key << ", "
           << "\"" << link->label << "\"" << ", "
           << link->isContentLink() << ", "
           << link->isShared() << ")" << std::endl;
    }

}


void report_graphviz(std::ostream &os, Report &report) {
    std::vector<Node*> nodes;
    std::vector<Link*> links;

    // create nodes vector
    for (auto it: report.node_map) {
        Node *node = it.second;
        nodes.push_back(node);
    }

    // create nodes vector
    for (auto it: report.node_map) {
        Node *node = it.second;
        InternalNode *internal_node = node->asInternalNode();
        if (internal_node) {

            // std::cout << *internal_node->layer << std::endl;

            // children
            for (Link &link: internal_node->children) {
//                if (link.source_node == nullptr || link.target_node == nullptr )
//                    throw std::runtime_error("link needs to have its endpoints right");

                links.push_back(&link);
            }


            { // content
                Link &link = internal_node->content;

                // if (link.source_node == nullptr || link.target_node == nullptr )
                //    throw std::runtime_error("link needs to have its endpoints right");

                links.push_back(&link);
            }
        }
    }

    // sort nodes and links
    std::sort(nodes.begin(), nodes.end(), compare_nodes);
    std::sort(links.begin(), links.end(), compare_links);


    os << "digraph G {"          << std::endl;
    os << "   size = \"10,10\";" << std::endl;

    for (Layer *layer: report.layers) {

        os << "{ rank=same; " << std::endl;

        for (Node *node: layer->nodes) {
            os << "   " << node->key << " ";
            if (node->type == Node::INTERNAL_NODE) {
                os << "[label=\""
                   << node->layer->layer_id.dimension
                   << "-"
                   << node->layer->layer_id.level << "\", "
                   << "style=filled, "
                   << "color=" << ((node->layer->layer_id.dimension % 2 == 0) ? "lightgoldenrod3" : "lightpink1")
                   << "];";
            }
            else {
                os << "[style=filled, color=lightgray, shape=box, label=\""
                   << node->getInfo() << "\"" << "];";
            }
            os << std::endl;
        }

        os << "}" << std::endl;
    }

    for (Link *link: links) {
        os << "   "
           << link->source_node->key << " -> "
           << link->target_node->key << " ";
        os << "[ color=" << (link->isContentLink() ? (link->isShared() ? "lightblue3" : "blue") :
                                                     (link->isShared() ? "gray" : "black")) << ", "
           << "style=" << (link->isProper() ? "solid" : "dashed") << ","
           << "arrowhead=" << (link->isContentLink() ? "normal" : "none") << ","
           << "label=\"" << link->label << "\"]";
        os << std::endl;
    }
    os << "}" << std::endl;
}


} // end report namespace
