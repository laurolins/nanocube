#include <log.hh>

namespace logging {

//-----------------------------------------------------------------------------
// ActionNewNode
//-----------------------------------------------------------------------------

ActionNewNode::ActionNewNode(NodeName node, int dimension, int layer):
    Action(ACTION_NEW_NODE),
    node(node),
    dimension(dimension),
    layer(layer)
{}

void ActionNewNode::json(std::ostream &os) const
{
    os << "{ \"action\": \"" << getActionName(type) <<  "\""
       << ", \"node\":"  << node
       << ", \"dim\":"   << dimension
       << ", \"layer\":" << layer << " }";
}

//-----------------------------------------------------------------------------
// ActionSetChildLink
//-----------------------------------------------------------------------------

ActionSetChildLink::ActionSetChildLink(NodeName parent, NodeName child, Label label, bool shared):
    Action(ACTION_SET_CHILD_LINK),
    parent(parent),
    child(child),
    label(label),
    shared(shared)
{}

void ActionSetChildLink::json(std::ostream &os) const
{
    os << "{ \"action\": \"" << getActionName(type) <<  "\""
       << ", \"parent\":"  << parent
       << ", \"child\":"   << child
       << ", \"label\":"   << label
       << ", \"shared\":"  << (shared ? 1 : 0)
       << " }";
}

//-----------------------------------------------------------------------------
// ActionSetContentLink
//-----------------------------------------------------------------------------

ActionSetContentLink::ActionSetContentLink(NodeName node, NodeName content, bool shared):
    Action(ACTION_SET_CONTENT_LINK),
    node(node),
    content(content),
    shared (shared)
{}

void ActionSetContentLink::json(std::ostream &os) const
{
    os << "{ \"action\": \"" << getActionName(type) <<  "\""
       << ", \"node\":"      << node
       << ", \"content\":"   << content
       << ", \"shared\":"    << (shared ? 1 : 0)
       << " }";
}

//-----------------------------------------------------------------------------
// ActionStartInsert
//-----------------------------------------------------------------------------

ActionPush::ActionPush(std::string message):
    Action(ACTION_PUSH),
    message(message)
{}

void ActionPush::json(std::ostream &os) const
{
    os << "{ \"action\": \"" << getActionName(type) <<  "\""
       << ", \"msg\":\""  << message  <<  "\""
       << " }";
}

//-----------------------------------------------------------------------------
// ActionStartInsert
//-----------------------------------------------------------------------------

ActionPop::ActionPop():
    Action(ACTION_POP)
{}

void ActionPop::json(std::ostream &os) const
{
    os << "{ \"action\": \"" << getActionName(type) <<  "\"" << " }";
}

//-----------------------------------------------------------------------------
// Action
//-----------------------------------------------------------------------------

Action::Action(ActionType action_type):
    type { action_type }
{}


//-----------------------------------------------------------------------------
// ActionStore
//-----------------------------------------------------------------------------

ActionStore::ActionStore(NodeName node_name, Object object):
    Action(ACTION_STORE),
    node_name(node_name),
    object(object)
{}

void ActionStore::json(std::ostream &os) const
{
    os << "{ \"action\": \"" << getActionName(type) <<  "\""
       << ", \"node\":"  << node_name
       << ", \"obj\":"   << object
       << " }";
}

//-----------------------------------------------------------------------------
// ActionHighlightNode
//-----------------------------------------------------------------------------

ActionHighlightNode::ActionHighlightNode(NodeName node_name, Color color):
    Action(ACTION_HIGHLIGHT_NODE),
    node_name(node_name),
    color(color)
{}

void ActionHighlightNode::json(std::ostream &os) const
{
    os << "{ \"action\": \"" << getActionName(type) <<  "\""
       << ", \"node\":"  << node_name
       << ", \"color\":" << color
       << " }";
}

//-----------------------------------------------------------------------------
// ActionHighlightNode
//-----------------------------------------------------------------------------

ActionHighlightContentLink::ActionHighlightContentLink(NodeName node_name, Color color):
    Action(ACTION_HIGHLIGHT_CONTENT_LINK),
    node_name(node_name),
    color(color)
{}

void ActionHighlightContentLink::json(std::ostream &os) const
{
    os << "{ \"action\": \"" << getActionName(type) <<  "\""
       << ", \"node\":"  << node_name
       << ", \"color\":" << color
       << " }";
}

//-----------------------------------------------------------------------------
// ActionHighlightChildLink
//-----------------------------------------------------------------------------

ActionHighlightChildLink::ActionHighlightChildLink(NodeName node_name, Label label, Color color):
    Action(ACTION_HIGHLIGHT_CHILD_LINK),
    node_name(node_name),
    label(label),
    color(color)
{}

void ActionHighlightChildLink::json(std::ostream &os) const
{
    os << "{ \"action\": \"" << getActionName(type) <<  "\""
       << ", \"node\":"  << node_name
       << ", \"label\":" << label
       << ", \"color\":" << color
       << " }";
}

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

Log::Log():
    ostream("/tmp/actions.json")
{
    ostream << "[";
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

void Log::dump_last_action() {
    actions.back()->json(ostream);
    ostream << ", ";
    ostream.flush();
}

void Log::pushMessage(std::string msg)
{
    actions.push_back(new ActionPush(msg));
    dump_last_action();
}

void Log::popMessage()
{
    actions.push_back(new ActionPop());
    dump_last_action();
}

void Log::dump_json_actions(std::ostream &os) const
{
    bool first = true;
    os << "[";
    for (Action *a: actions) {
        if (!first) {
            os <<", ";
        }
        first = false;
        a->json(os);
    }
    os << "]";
}

void Log::newNode(NodeID node, int dimension, int layer)
{
    actions.push_back(new ActionNewNode(getNodeName(node), dimension, layer));
    dump_last_action();
}

void Log::setChildLink(NodeID parent, NodeID child, Label label, bool shared)
{
    actions.push_back(new ActionSetChildLink(getNodeName(parent), getNodeName(child), label, shared));
    dump_last_action();
}

void Log::setContentLink(NodeID node, NodeID content, bool shared)
{
    actions.push_back(new ActionSetContentLink(getNodeName(node), getNodeName(content), shared));
    dump_last_action();
}

void Log::store(NodeID node, Object obj)
{
    actions.push_back(new ActionStore(getNodeName(node), obj));
    dump_last_action();
}

void Log::highlightNode(NodeID node, Color color)
{
    actions.push_back(new ActionHighlightNode(getNodeName(node), color));
    dump_last_action();
}

void Log::highlightContentLink(NodeID node, Color color)
{
    actions.push_back(new ActionHighlightContentLink(getNodeName(node), color));
    dump_last_action();
}

void Log::highlightChildLink(NodeID node, Label label, Color color)
{
    actions.push_back(new ActionHighlightChildLink(getNodeName(node), label, color));
    dump_last_action();
}

//-----------------------------------------------------------------------------
// Free functions
//-----------------------------------------------------------------------------

Log &getLog()
{
    static Log log;
    return log;
}

std::string name(NodeID node_id) {
    return std::to_string(getLog().getNodeName(node_id));
}

std::string getActionName(ActionType type) {
    static std::vector<std::string> names {
        "undefined",
        "new_node",
        "set_child_link",
        "set_content_link",
        "push",
        "pop",
        "store",
        "highlight_node",
        "highlight_content_link",
        "highlight_child_link"
    };
    return names.at((int) type);
}


}

