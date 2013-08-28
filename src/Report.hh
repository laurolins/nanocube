#pragma once

#include <unordered_map>
#include <algorithm>
#include <vector>

#include<iostream>

namespace report {

//------------------------------------------------------------------------------
// Forward declarations
//------------------------------------------------------------------------------

struct LayerId;
struct Layer;
struct LayerId;
struct Layer;
struct Link;
struct Node;
struct Report;
struct InternalNode;
struct LeafNode;

//------------------------------------------------------------------------------
// LayerKey
//------------------------------------------------------------------------------

struct LayerId {

public: // methods

    LayerId(int dimension, int level):
        dimension(dimension),
        level(level)
    {}

    bool operator==(const LayerId &k) const {
        return (dimension == k.dimension && level == k.level);
    }

    bool operator<(const LayerId &k) const {
        return (dimension < k.dimension || (dimension == k.dimension && level < k.level));
    }

public: // data members

    int dimension;

    int level;

};

//------------------------------------------------------------------------------
// Layer
//------------------------------------------------------------------------------

struct Layer {

public: // methods

    Layer(LayerId key):
        layer_id(key),
        index(-1),
        num_nodes(0),
        num_proper_children(0),
        num_shared_children(0),
        num_shared_content(0),
        num_proper_content(0)
    {}

    inline bool operator==(const Layer& layer) const {
        return (layer_id == layer.layer_id);
    }

    inline bool operator<(const Layer& layer) const {
        return (layer_id < layer.layer_id);
    }

public: // data members

    LayerId layer_id;

    int     index;

    std::vector<Node*> nodes;

    uint64_t num_nodes;

    uint64_t num_proper_children;
    uint64_t num_shared_children;

    uint64_t num_shared_content;
    uint64_t num_proper_content;

};

std::ostream& operator<<(std::ostream &os, const Layer& layer);

//------------------------------------------------------------------------------
// Link
//------------------------------------------------------------------------------

struct Link {

public: // subtypes and constants

    typedef char Flags;

    static const char SHARED_PROPER_MASK = 0x1;
    static const char SHARED = 0x1;
    static const char PROPER = 0x0;

    static const char CHILD_CONTENT_MASK = 0x2;
    static const char CHILD   = 0x0;
    static const char CONTENT = 0x2;

public: // data members

    Node* source_node;
    Node* target_node;

    Flags flags;

    std::string label;


public: // methods

    Link():
        source_node(nullptr),
        target_node(nullptr),
        flags(0)
    {}

    Link(Node* source_node, Node *target_node, Flags flags, std::string label):
        source_node(source_node),
        target_node(target_node),
        flags(flags),
        label(label)
    {}

    inline bool isContentLink() const {
        return (flags & CHILD_CONTENT_MASK) == CONTENT;
    }

    inline bool isChildLink() const {
        return (flags & CHILD_CONTENT_MASK) == CHILD;
    }

    inline bool isProper() const {
        return (flags & SHARED_PROPER_MASK) == PROPER;
    }

    inline bool isShared() const {
        return (flags & SHARED_PROPER_MASK) == SHARED;
    }

    bool operator==(const Link &link) const;
    bool operator<(const Link &link) const;
};


//------------------------------------------------------------------------------
// Node
//------------------------------------------------------------------------------

struct Node {

public: // subtypes

    enum Type { INTERNAL_NODE, LEAF_NODE };

public:

    Node(Type type, Layer *layer, uint64_t key):
        type(type),
        layer(layer),
        key(key)
    {
        layer->nodes.push_back(this);
    }

    virtual void insertChildLink(Node *child, bool shared, std::string label)
    {}

    virtual void setContent(Node *content, bool shared)
    {}

    virtual std::string getInfo() const
    { return ""; }

    virtual void setInfo(std::string info)
    {}

    virtual InternalNode* asInternalNode() {
        return nullptr;
    }

    virtual LeafNode* asLeafNode() {
        return nullptr;
    }

    bool operator==(const Node &node) const {
        return (key == node.key);
    }

    bool operator<(const Node &node) const {
        return (key < node.key);
    }

public: // data members

    Type type;

    Layer *layer;

    uint64_t key;

};

std::ostream& operator<<(std::ostream &os, const Node& node);



//------------------------------------------------------------------------------
// InternalNode
//------------------------------------------------------------------------------

struct InternalNode: public Node {

public: // methods

    InternalNode(Layer *layer, uint64_t key);

    virtual void insertChildLink(Node *child, bool shared, std::string label);

    virtual void setContent(Node *content, bool shared);

    virtual InternalNode* asInternalNode() {
        return this;
    }

public: // data members

    std::vector<Link> children;

    Link content;

};

//------------------------------------------------------------------------------
// Leaf
//------------------------------------------------------------------------------

struct LeafNode: public Node {

public: // methods

    LeafNode(Layer* layer, uint64_t key);

    virtual std::string getInfo() const;

    virtual void setInfo(std::string st);

    virtual LeafNode* asLeafNode() {
        return this;
    }

public: // data members

    std::string info;

};


//------------------------------------------------------------------------------
// Graph
//------------------------------------------------------------------------------

struct Report {

public: // data members

    std::unordered_map<uint64_t, Node*> node_map;

    std::vector<Layer*> layers; // keep these layers sorted

    int dimensions; // this includes the time series dimension (leaves)

private:

    Layer *getLayer(LayerId key);

public: // methods

    Report(int dimensions);

    void updateToHumanReadableKeys();

    void updateLayerIndices();

    Node* insertNode(uint64_t node_key, int dimension, int level);

    Node *getNode(uint64_t node_key) const;

};


//
void report_python(std::ostream &os, Report &report);
void report_graphviz(std::ostream &os, Report &report);


} // end report namespace
