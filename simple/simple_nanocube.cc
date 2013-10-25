#include <simple_nanocube.hh>

#include <iostream>
#include <functional>
#include <algorithm>
#include <iterator>
#include <sstream>

#include <log.hh>


#define LOG

//-----------------------------------------------------------------------------
// Summary
//-----------------------------------------------------------------------------

void Summary::insert(const Object &obj) {
    std::cout << "insert " << obj << std::endl;
    objects.insert(obj);
    // ++count;
}

std::string Summary::info() const {
    std::vector<Object> objects_list(objects.begin(), objects.end());
    std::sort(objects_list.begin(), objects_list.end());

    std::stringstream ss;
    std::copy(objects_list.begin(), objects_list.end(), std::ostream_iterator<Object>(ss, " "));

    return ss.str();
}


//-----------------------------------------------------------------------------
// ContentLink
//-----------------------------------------------------------------------------

ContentLink::ContentLink(void *content, LinkType link_type):
    content(content), link_type(link_type)
{}

//-----------------------------------------------------------------------------
// ParentChildLink
//-----------------------------------------------------------------------------

ParentChildLink::ParentChildLink(Label label, Node *child, LinkType link_type):
    label(label), child(child), link_type(link_type)
{}

//-----------------------------------------------------------------------------
// Nanocube
//-----------------------------------------------------------------------------

Nanocube::Nanocube(const std::vector<int> &levels):
    levels { levels },
    dimension { (int) levels.size() }
{}

Summary *Nanocube::query(const Address &addr)
{
    if (root == nullptr) {
        return nullptr;
    }

    //
    Node *node = root;
    for (size_t i=0;i<addr.size();i++) {
        for (size_t j=0;j<addr[i].size();j++) {
            node = node->getChild(addr[i][j]);
            if (node == nullptr)
                return nullptr;
        }
        if (i < addr.size()-1) {
            node = node->getContentAsNode();
        }
    }

    if (node) {
        return node->getContentAsSummary();
    }
    else {
        return nullptr;
    }
}

namespace highlight {
    static const int CLEAR    = 0;
    static const int MAIN     = 1;
    static const int PARALLEL = 2;
}


struct LogMsg {
    LogMsg(std::string msg) {
        logging::getLog().pushMessage(msg);
    }
    ~LogMsg() {
        logging::getLog().popMessage();
    }
};

static void log_new_node(void* node, int dim, int layer) {
    logging::getLog().newNode((logging::NodeID)node, dim, layer);
}

static void log_shallow_copy(Node* node, int dim, int layer) {

    logging::getLog().newNode((logging::NodeID)node, dim, layer);
    if (node->getContent() != nullptr) {
        logging::getLog().setContentLink((logging::NodeID) node, (logging::NodeID) node->getContent(), logging::SHARED);
    }
    for (auto &child_link: node->children) {
        logging::getLog().setChildLink((logging::NodeID) node, (logging::NodeID) child_link.child, child_link.label, logging::SHARED);
    }
}

static void log_set_child_link(Node* node, Label label) {
    LinkType link_type;
    Node* child = node->getChild(label,link_type);
    logging::getLog().setChildLink((logging::NodeID) node,
                                   (logging::NodeID) child,
                                   label,
                                   link_type == SHARED ? logging::SHARED : logging::PROPER);
}

static void log_set_content_link(Node* node) {
    LinkType link_type = node->getContentType();
    void* content = node->getContent();
    logging::getLog().setContentLink((logging::NodeID) node,
                                       (logging::NodeID) content,
                                       link_type == SHARED ? logging::SHARED : logging::PROPER);
}


static void log_shallow_copy(Summary* summary, int dim, int layer) {

    logging::getLog().newNode((logging::NodeID)summary, dim, layer);
    std::vector<logging::Object> objects(summary->objects.size());
    std::copy(summary->objects.begin(),summary->objects.end(),objects.begin());
    std::sort(objects.begin(),objects.end());
    for (auto obj: objects) {
        logging::getLog().store((logging::NodeID)summary, obj);
    }
}

static void log_update_content_link(Node* node) {
    logging::getLog().setContentLink((logging::NodeID)node,
                                     (logging::NodeID)node->getContent(),
                                     node->getContentType() == SHARED ? logging::SHARED : logging::PROPER);
}

static void log_highlight_node(const Node* node, logging::Color color) {
    logging::getLog().highlightNode((logging::NodeID)node, color);
}

static void log_highlight_child_link(const Node* node, Label label, logging::Color color) {
    logging::getLog().highlightChildLink((logging::NodeID)node,label,color);
}

static void log_highlight_content_link(const Node* node, logging::Color color) {
    logging::getLog().highlightContentLink((logging::NodeID)node,color);
}


void Nanocube::insert(const Address &addr, const Object &object)
{

#ifdef LOG
    std::stringstream ss;
    ss << "Insert obj: " << object << " addr: ";
    for (size_t i=0;i<addr.size();++i) {
        ss << "{";
        std::ostream_iterator<int> out_it (ss,",");
        std::copy ( addr[i].begin(), addr[i].end(), out_it );
        ss << "} ";
    }
    LogMsg log_msg(ss.str());
#endif

    // assert length of each DimAddress in Address matches
    // with the max levels of each dimension

//    std::vector<Node*> parallel_contents(dimension);
//    std::fill(parallel_contents.begin(), parallel_contents.end(),nullptr);

    std::vector<std::vector<Node*>> paths(dimension);
    std::vector<std::vector<const Node*>> parallel_paths(dimension);

    if (root == nullptr) {
        root = new Node();

#ifdef LOG
        logging::getLog().newNode((logging::NodeID)root, 0, 0);
#endif

    }

    std::function<void(Node*,int,const Node*)> insert_recursively;

    int dimension = this->dimension;

    insert_recursively = [&insert_recursively, &paths, &addr, &object, &dimension, &parallel_paths]
            (Node* current_root, int current_dim, const Node* child_updated_root) -> void {





#ifdef LOG
        std::stringstream ss;
        auto &log = logging::getLog();
        auto node_name = log.getNodeName((logging::NodeID) current_root);
        std::string parallel_node_name = "null";
        if (child_updated_root != nullptr) {
            parallel_node_name = std::to_string(log.getNodeName((logging::NodeID) child_updated_root));
        }
        ss << "insert_recursively on node " << node_name
           << " dim " << current_dim << " parallel node " << parallel_node_name;

        LogMsg log_msg2(ss.str());
#endif

        std::cout << "insert_recursively, dim:" << current_dim << std::endl;


        auto &dim_path = paths[current_dim];
        auto &dim_parallel_path = parallel_paths[current_dim];
        auto &dim_addr = addr[current_dim];

        dim_path.clear();
        dim_parallel_path.clear();

        // fill in dim_stacks with an outdated path plus (a null pointer or shared child)
        current_root->prepareOutdatedProperPath(child_updated_root, dim_addr, dim_path, dim_parallel_path, current_dim);

        Node *child = dim_path.back(); // by convention the top of the stack
                                       // is a node that is already updated or
                                       // a nullptr

        auto index = dim_path.size()-2;
        for (auto it=dim_path.rbegin()+1;it!=dim_path.rend();++it) {

            Node* parent = *it;
            const Node* parallel_parent = (index < dim_parallel_path.size() ? dim_parallel_path[index] : nullptr);

            {
                LogMsg msg("...parallel_path index: " + std::to_string(index) + "  size: " + std::to_string(dim_parallel_path.size()));
            }

            --index;


            if (parent->children.size() == 1) {
                parent->setContent(child->getContent(), SHARED); // sharing content with children

#ifdef LOG
                {
                    LogMsg msg_single_child("...detected single child, sharing its contents");
                    log_set_content_link(parent);
                }
#endif

            }
            else {
                if (current_dim == dimension-1) {
                    // last dimension: insert object into summary
                    if (parent->hasContent() == false) {
                        parent->setContent(new Summary(), PROPER); // create proper content

#ifdef LOG
                        {
                            LogMsg msg_single_child("...new summary");
                            log_new_node(parent->getContent(), current_dim+1, 0);
                            // logging::getLog().newNode((logging::NodeID) parent->getContent(), current_dim+1, 0);
                            log_update_content_link(parent);
                        }
#endif

                    }
                    else if (parent->getContentType() == SHARED) {
                        // since number of children is not one, we need to clone
                        // content and insert new object
                        Summary* old_content = parent->getContentAsSummary();
                        parent->setContent(new Summary(*parent->getContentAsSummary()), PROPER); // create proper content

#ifdef LOG
                        {
                            LogMsg msg("...shallow copy of summary: " + std::to_string(logging::getLog().getNodeName((logging::NodeID)old_content)));
                            log_shallow_copy(parent->getContentAsSummary(), current_dim+1, 0);
                            log_update_content_link(parent);
                        }
#endif


                    }

#ifdef LOG
                    {
                        LogMsg msg_single_child("...inserting " + std::to_string(object) + " into summary node");
                        logging::getLog().store((logging::NodeID)parent->getContent(), object);
                    }
#endif

                    parent->getContentAsSummary()->insert(object);
                }
                else {
                    // not last dimension
                    if (parent->hasContent() == false) {
                        parent->setContent(new Node(), PROPER); // create proper content

#ifdef LOG
                        {
                            LogMsg msg_single_child("...new content on dimension " + std::to_string(current_dim+1));
                            log_new_node(parent->getContent(), current_dim+1, 0);
                            // logging::getLog().newNode((logging::NodeID)parent->getContent(), current_dim+1, 0);
                            log_update_content_link(parent);
                        }
#endif



                    }
                    else if (parent->getContentType() == SHARED) {
                        // need to copy
                        Node* old_content = parent->getContentAsNode();
                        parent->setContent(parent->getContentAsNode()->shallowCopy(), PROPER); // create proper content

#ifdef LOG
                        {
                            LogMsg msg("...shared branch needs to be copied. Shallow copy of node: " + std::to_string(logging::getLog().getNodeName((logging::NodeID)old_content)));
                            log_shallow_copy(parent->getContentAsNode(), current_dim+1, 0);
                            log_update_content_link(parent);
                        }
#endif
                    }

//                    Node* parallel_content = parallel_contents[current_dim]; //nullptr;
//                    if (!parallel_content && child) {
//                        parallel_content = child->getContentAsNode();
//                    }

                    // here is the catch
                    const Node* parallel_content = nullptr;
                    if (parallel_parent) {
                        parallel_content = parallel_parent->getContentAsNode();
                    }
                    else if (child) {
                        parallel_content = child->getContentAsNode();
                    }

//                    else {
//                        parallel_content = parallel_contents[current_dim];
//                    }


#ifdef LOG
                    {
                        log_highlight_content_link(parent, highlight::MAIN);
                        if (parallel_parent) {
                            log_highlight_content_link(parallel_parent, highlight::PARALLEL);
                        }
                    }
#endif

                    insert_recursively(parent->getContentAsNode(), current_dim+1, parallel_content);

#ifdef LOG
                    {
                        log_highlight_content_link(parent, highlight::CLEAR);
                        if (parallel_parent) {
                            log_highlight_content_link(parallel_parent, highlight::CLEAR);
                        }
                    }
#endif


//                     update parallel content (for coarser contents on that dimension)
                    // parallel_contents[current_dim] = parent->getContentAsNode();
                    //}
                }
            }

#ifdef LOG
            {
                std::string st_msg = parallel_parent? "...moving up parent and parallel_parent" : "...moving up parent only";
                LogMsg msg(st_msg);
                log_highlight_node(parent, highlight::CLEAR);
                if (parent->proper_parent != nullptr) {
                    log_highlight_child_link(parent->proper_parent,
                                             parent->label,
                                             highlight::CLEAR);
                }

                if (parallel_parent) {
                    log_highlight_node(parallel_parent, highlight::CLEAR);
                    if (parallel_parent->proper_parent != nullptr) {
                        log_highlight_child_link(parallel_parent->proper_parent,
                                                 parallel_parent->label,
                                                 highlight::CLEAR);
                    }
                }
            }
#endif

            child = parent;
        }

    };


    // insert
    insert_recursively(root, 0, nullptr);


}

//-----------------------------------------------------------------------------
// Node Impl.
//-----------------------------------------------------------------------------

Node *Node::shallowCopy() const {
    Node* copy = new Node(*this);
    for (auto &child: copy->children) {
        child.link_type = SHARED; // redefine all children as shared links
    }
    copy->content_link.link_type = SHARED;
    return copy;
}

void* Node::getContent() const {
    return this->content_link.content;
}

void Node::setContent(void *content, LinkType type)
{
    this->content_link.content = content;
    this->content_link.link_type = type;
}

bool Node::hasContent() const
{
    return content_link.content != nullptr;
}

Summary *Node::getContentAsSummary() const
{
    return reinterpret_cast<Summary*>(content_link.content);
}

Node *Node::getContentAsNode() const
{
    return reinterpret_cast<Node*>(content_link.content);
}

void Node::setParentChildLink(Label label, Node *node, LinkType link_type, bool &created)
{
    created = false;
    auto comp = [](const ParentChildLink &e, Label lbl) {
        return e.label < lbl;
    };
    auto it = std::lower_bound(children.begin(), children.end(), label, comp);

    if (it != children.end() && it->label == label) {
        it->child = node;
        it->label = label;
        it->link_type = link_type;
    }
    else {
        children.insert(it, ParentChildLink(label, node, link_type));
        created = true;
    }

    if (link_type == PROPER) {
        node->setProperParent(this, label);
    }

}

Node* Node::getChild(Label label) const
{
    auto comp = [](const ParentChildLink &e, Label lbl) {
        return e.label < lbl;
    };
    auto it = std::lower_bound(children.begin(), children.end(), label, comp);

    if (it != children.end() && it->label == label) {
        return it->child;
    }
    else {
        return nullptr;
    }
}

Node* Node::getChild(Label label, LinkType &link_type) const
{
    auto comp = [](const ParentChildLink &e, Label lbl) {
        return e.label < lbl;
    };
    auto it = std::lower_bound(children.begin(), children.end(), label, comp);

    if (it != children.end() && it->label == label) {
        link_type = it->link_type;
        return it->child;
    }
    else {
        return nullptr;
    }
}

void Node::setProperParent(Node *parent, Label lbl)
{
    this->proper_parent = parent;
    this->label         = lbl;
}

LinkType Node::getContentType() const
{
    return content_link.link_type;
}

void Node::prepareOutdatedProperPath(const Node*          parallel_root,
                                     const DimAddress&    addr,
                                     std::vector<Node *>& result_path,
                                     std::vector<const Node *>& parallel_path,
                                     int   current_dim)
{
#ifdef LOG
    std::vector<const Node*> parallel_highlight_stack;
#endif


    size_t index = 0;

    auto attachNewProperChain = [&index, &addr, &result_path, &current_dim](Node* node) {
        Node* next = new Node(); // last node

#ifdef LOG
        log_new_node(next, current_dim, result_path.size());
#endif

        for (size_t i=addr.size()-1; i>index; --i) {
            Node* current = new Node();
            bool created = false;
            current->setParentChildLink(addr[i], next, PROPER, created);


#ifdef LOG
            log_new_node(current, current_dim, i);
            log_set_child_link(current, addr[i]);
#endif

            next = current;
        }

        { // join the given node with new chain
            bool created = false;
            node->setParentChildLink(addr[index], next, PROPER, created);

#ifdef LOG
            log_set_child_link(node, addr[index]);
#endif
        }

        { // update path
            Node *current = node;
            for (size_t i=index;i<addr.size();i++) {
                Node *child = current->getChild(addr[i]);
                result_path.push_back(child);
                current = child;
            }
            result_path.push_back(nullptr); // end marker
        }
    };




    // start with root node and previous node is nullptr
    Node*    current_node  = this;
    result_path.push_back(current_node);

    // pointer parallel to current_node in child_structure
    const Node* current_parallel_node  = parallel_root;
    if (current_parallel_node != nullptr) {
        parallel_path.push_back(current_parallel_node);
    }

//    std::cout << "current_address.level: " << current_address.level << std::endl;
//    std::cout << "address.level:         " << address.level         << std::endl;

#ifdef LOG
    {
        std::string st_msg = current_parallel_node ? "...start main and parallel frontier" : "...start main frontier only";
        LogMsg msg(st_msg);
        log_highlight_node(current_node, highlight::MAIN);
        if (current_parallel_node) {
            log_highlight_node(current_parallel_node, highlight::PARALLEL);
        }
    }
#endif


    // traverse the path from root address to address
    // inserting and stacking new proper nodes until
    // either we find a path-suffix that can be reused
    // or we traverse all the path
    while (index < addr.size())
    {

        // assuming address is contained in the current address
        Label label = addr[index];

        LinkType next_node_link_type;
        Node* next_node           = current_node->getChild(label, next_node_link_type);
        Node* next_parallel_node  = current_parallel_node ? current_parallel_node->getChild(label) : nullptr;

        // nextAddress index on current node
        // there is no next node
        if (!next_node)
        {
            // there is a parallel structure: update current node to
            // share the right child on the parallel structure
            if (next_parallel_node) {
                bool created   = true;
                current_node->setParentChildLink(label, next_parallel_node, SHARED, created);
                result_path.push_back(next_parallel_node); // append marker to the path

#ifdef LOG
                {
                    LogMsg msg("...sharing existing path on parallel branch");
                    log_set_child_link(current_node, label);
                }
#endif
                break;
            }
            else {

#ifdef LOG
                {
                    LogMsg msg("...creating new branch and advancing main frontier!");
#endif

                    attachNewProperChain(current_node);

#ifdef LOG
                    Node *aux = current_node;
                    for (size_t ii=index;ii<addr.size();ii++) {
                        aux = current_node->getChild(addr[ii]);
                        log_highlight_child_link(current_node, addr[ii], highlight::MAIN);
                        log_highlight_node(aux, highlight::MAIN);
                    }
                }
#endif

                break;
            }
        }
        else if (next_node_link_type == SHARED) {
            // assert(next_parallel_node);
            if (next_node == next_parallel_node) {
                result_path.push_back(next_node); // append end-marker

#ifdef LOG
                {
                    LogMsg msg("...main path already sharing with parallel path: done advancind frontier");
                }
#endif

                break;
            }
            else {


#if 0
                next_node = next_parallel_node;
                result_path.push_back(next_node); // append end-marker
                bool created = false;
                current_node->setParentChildLink(label, next_parallel_node, SHARED, created);
#ifdef LOG
                {
                    LogMsg msg("...switching shared child (we need to prove this!)");
                    log_set_child_link(current_node,label);
                }
#endif
                break;

#else

                // make lazy copy of next_node: its content is going to
                // change and cannot interfere on the original version
                next_node = next_node->shallowCopy();
                bool created = false;
                current_node->setParentChildLink(label, next_node, PROPER, created);

#ifdef LOG
                {

                    LogMsg msg("...main path sharing branch not in parallel path: shallow copy, continue");
                    log_shallow_copy(next_node,current_dim,index+1);

                    //                logging::getLog().newNode((logging::NodeID) next_node, current_dim, index+1);
                    //                for (auto &child_link: next_node->children) {
                    //                    logging::getLog().setChildLink((logging::NodeID) next_node, (logging::NodeID) child_link.child, child_link.label, logging::SHARED);
                    //                }
                    //                if (next_node->getContent() != nullptr) {
                    //                    logging::getLog().setContentLink((logging::NodeID) next_node, (logging::NodeID) next_node->getContent(), logging::SHARED);
                    //                }


                    log_set_child_link(current_node,addr[index]);

//                logging::getLog().setChildLink((logging::NodeID) current_node,
//                                               (logging::NodeID) next_node,
//                                               addr[index],
//                                               logging::PROPER);
                }

#endif

#endif

            }
        }


#ifdef LOG
        {
            std::string st_msg = next_parallel_node ? "...advancing main and parallel frontier" : "...advancing main frontier only";
            LogMsg msg(st_msg);

            log_highlight_child_link(current_node, addr[index], highlight::MAIN);
            if (next_parallel_node) {
                log_highlight_child_link(current_parallel_node, addr[index], highlight::PARALLEL);
            }
#endif
            // update current_node, previous_node and stack
            current_node    = next_node;

            // update current_parallel_node
            current_parallel_node = next_parallel_node;

            // push paths
            result_path.push_back(current_node);
            if (current_parallel_node != nullptr) {
                parallel_path.push_back(current_parallel_node);
            }

#ifdef LOG
            log_highlight_node(current_node, highlight::MAIN);
            if (current_parallel_node) {
                // parallel_highlight_stack.push_back(current_parallel_node);
                log_highlight_node(current_parallel_node, highlight::PARALLEL);
            }
        }
#endif

        ++index;

        if (index == addr.size()) {
            result_path.push_back(nullptr); // base case
        }

    }

#ifdef LOG
//    {
//        if (parallel_highlight_stack.size() > 0) {
//            LogMsg msg("Cleaning parallel path");
//            while (!parallel_highlight_stack.empty()) {
//                log_highlight_node(parallel_highlight_stack.back(), highlight::CLEAR);
//                parallel_highlight_stack.pop_back();
//            }
//        }
//    }
#endif








}

