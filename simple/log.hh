#pragma once

#include <ostream>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <fstream>

namespace logging {

using NodeID   = uint64_t;
using NodeName = int;
using Label    = int;
using Object   = int;
using Color    = int;

struct Action;

static const bool SHARED = true;
static const bool PROPER = false;

enum ActionType { UNDEFINED=0,
                  ACTION_NEW_NODE=1,
                  ACTION_SET_CHILD_LINK=2,
                  ACTION_SET_CONTENT_LINK=3,
                  ACTION_PUSH=4,
                  ACTION_POP=5,
                  ACTION_STORE=6,
                  ACTION_HIGHLIGHT_NODE=7,
                  ACTION_HIGHLIGHT_CONTENT_LINK=8,
                  ACTION_HIGHLIGHT_CHILD_LINK=9 };

std::string getActionName(ActionType type);

//-----------------------------------------------------------------------------
// Log
//-----------------------------------------------------------------------------

struct Log {

    Log();
    virtual ~Log();

    NodeName getNodeName(NodeID node);

    void newNode(NodeID node, int dimension, int layer);
    void setChildLink  (NodeID parent, NodeID child,   Label label, bool shared);
    void setContentLink(NodeID node,   NodeID content, bool shared);

    void store(NodeID node, Object obj);

    void highlightNode(NodeID node, Color color=0);
    void highlightChildLink(NodeID node, Label label, Color color=0);
    void highlightContentLink(NodeID node, Color color=0);

    void dump_json_actions(std::ostream &os) const;

    void dump_last_action();


//    void callInsert(void *node, int data, void *parallel_content);
//    void callTraceProperPath(void *node, int data, void *parallel_content);

    void clear();

    void pushMessage(std::string msg);
    void popMessage();

    std::unordered_map<NodeID, NodeName> map;
    std::vector<Action*> actions;

    bool          ostream_empty { true };
    std::ofstream ostream;

};

//-----------------------------------------------------------------------------
// Action
//-----------------------------------------------------------------------------

struct Action {
    Action() = default;
    Action(ActionType action_type);

    virtual ~Action() {};

    virtual void json(std::ostream &os) const = 0;

    ActionType type { UNDEFINED };
};

//-----------------------------------------------------------------------------
// ActionStore
//-----------------------------------------------------------------------------

struct ActionStore: public Action {
    ActionStore() = default;
    ActionStore(NodeName node_name, Object obj);

    void json(std::ostream &os) const;

    NodeName node_name {  0 };
    Object   object    { -1 };
};

//-----------------------------------------------------------------------------
// ActionNewNode
//-----------------------------------------------------------------------------

struct ActionNewNode: public Action {
    ActionNewNode(NodeName node, int dimension, int layer);

    void json(std::ostream &os) const;

    NodeName node;
    int dimension;
    int layer;
};

//-----------------------------------------------------------------------------
// ActionSetChildLink
//-----------------------------------------------------------------------------

struct ActionSetChildLink: public Action {
    ActionSetChildLink(NodeName parent, NodeName child, Label label, bool shared);

    void json(std::ostream &os) const;

    NodeName parent;
    NodeName child;
    Label    label;
    bool     shared;
};

//-----------------------------------------------------------------------------
// ActionSetContentLink
//-----------------------------------------------------------------------------

struct ActionSetContentLink: public Action {
    ActionSetContentLink(NodeName node, NodeName content, bool shared);

    void json(std::ostream &os) const;

    NodeName node;
    NodeName content;
    bool     shared;
};

//-----------------------------------------------------------------------------
// ActionPush
//-----------------------------------------------------------------------------

struct ActionPush: public Action {
    ActionPush(std::string message);

    void json(std::ostream &os) const;

    std::string message;
};

//-----------------------------------------------------------------------------
// ActionPop
//-----------------------------------------------------------------------------

struct ActionPop: public Action {
    ActionPop();

    void json(std::ostream &os) const;
};

//-----------------------------------------------------------------------------
// ActionHighlightNode
//-----------------------------------------------------------------------------

struct ActionHighlightNode: public Action {
    ActionHighlightNode() = default;
    ActionHighlightNode(NodeName node_name, Color color);

    void json(std::ostream &os) const;

    NodeName node_name { 0 };
    Color    color     { 0 }; // if color == 0 consider a clear highlight
};

//-----------------------------------------------------------------------------
// ActionHighlightContentLink
//-----------------------------------------------------------------------------

struct ActionHighlightContentLink: public Action {
    ActionHighlightContentLink() = default;
    ActionHighlightContentLink(NodeName node_name, Color color);

    void json(std::ostream &os) const;

    NodeName node_name { 0 };
    Color      color   { 0 };
};

//-----------------------------------------------------------------------------
// ActionHighlightChildLink
//-----------------------------------------------------------------------------

struct ActionHighlightChildLink: public Action {
    ActionHighlightChildLink() = default;
    ActionHighlightChildLink(NodeName node_name, Label label, Color color);

    void json(std::ostream &os) const;

    NodeName node_name { 0 };
    Label    label     { 0 };
    Color    color     { 0 };
};



Log &getLog();

std::string name(NodeID node_id);

}
