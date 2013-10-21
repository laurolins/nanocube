#include <log.hh>

namespace log {

//-----------------------------------------------------------------------------
// ActionNewNode
//-----------------------------------------------------------------------------

ActionNewNode::ActionNewNode(NodeName node, int layer, int dimension):
    Action(ACTION_NEW_NODE),
    node(node),
    dimension(dimension),
    layer(layer)
{}

//-----------------------------------------------------------------------------
// ActionSetChildLink
//-----------------------------------------------------------------------------

ActionSetChildLink::ActionSetChildLink(NodeName parent, NodeName child, bool shared):
    Action(ACTION_SET_CHILD_LINK),
    parent(parent),
    child(child),
    shared(shared)
{}

//-----------------------------------------------------------------------------
// ActionSetContentLink
//-----------------------------------------------------------------------------

ActionSetContentLink::ActionSetContentLink(NodeName node, NodeName content, bool shared):
    Action(ACTION_SET_CONTENT_LINK),
    node(node),
    content(content),
    shared (shared)
{}

//-----------------------------------------------------------------------------
// Action
//-----------------------------------------------------------------------------

Action::Action(log::ActionType action_type):
    type { action_type }
{}

//-----------------------------------------------------------------------------
// Log
//-----------------------------------------------------------------------------

NodeName Log::getNodeName(NodeID node)
{
    auto it = map.find(node);
    if (it == map.end()) {
        NodeName node_name = (int) map.size();
        map[node] = node_name;
        return node_name;
    }
    else return it->second;
}

Log::~Log() {
    clear();
}

void Log::clear()
{
    for (Action *a: actions) {
        delete a;
    }
    actions.resize(0);
}

void Log::newNode(NodeID node, int dimension, int layer)
{
    actions.push_back(new ActionNewNode(getNodeName(node), dimension, layer));
}

void Log::setChildLink(NodeID parent, NodeID child, bool shared)
{
    actions.push_back(new ActionSetChildLink(getNodeName(parent), getNodeName(child), shared));
}

void Log::setContentLink(NodeID node, NodeID content, bool shared)
{
    actions.push_back(new ActionSetContentLink(getNodeName(node), getNodeName(content), shared));
}

Log &getLog()
{
    static Log log;
    return log;
}

}

