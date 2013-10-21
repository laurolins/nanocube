#pragma once

#include <vector>
#include <unordered_map>
#include <cstdint>

namespace log {

using NodeID   = uint64_t;
using NodeName = int;

struct Action;

enum ActionType { UNDEFINED, ACTION_NEW_NODE, ACTION_SET_CHILD_LINK, ACTION_SET_CONTENT_LINK };

struct Log {

    ~Log();

    NodeName getNodeName(NodeID node);

    void newNode(NodeID node, int dimension, int layer);
    void setChildLink  (NodeID parent, NodeID child,   bool shared);
    void setContentLink(NodeID node,   NodeID content, bool shared);

//    void callInsert(void *node, int data, void *parallel_content);
//    void callTraceProperPath(void *node, int data, void *parallel_content);

    void clear();

    std::unordered_map<NodeID, NodeName> map;
    std::vector<Action*> actions;

};

struct Action {
    Action() = default;
    Action(ActionType action_type);
    ActionType type { UNDEFINED };
};

struct ActionNewNode: public Action {
    ActionNewNode(NodeName node, int dimension, int layer);
    NodeName node;
    int dimension;
    int layer;
};

struct ActionSetChildLink: public Action {
    ActionSetChildLink(NodeName parent, NodeName child, bool shared);
    NodeName parent;
    NodeName child;
    bool shared;
};

struct ActionSetContentLink: public Action {
    ActionSetContentLink(NodeName node, NodeName content, bool shared);
    NodeName node;
    NodeName content;
    bool shared;
};

Log &getLog();

}
