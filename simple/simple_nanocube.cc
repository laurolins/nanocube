#include <simple_nanocube.hh>

#include <iostream>
#include <functional>
#include <algorithm>
#include <iterator>
#include <sstream>

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

void Nanocube::insert(const Address &addr, const Object &object)
{

    // assert length of each DimAddress in Address matches
    // with the max levels of each dimension

    std::vector<std::vector<Node*>> paths(dimension);

    if (root == nullptr) {
        root = new Node();
    }

    std::function<void(Node*,int,Node*)> insert_recursively;

    int dimension = this->dimension;

    insert_recursively = [&insert_recursively, &paths, &addr, &object, &dimension]
            (Node* current_root, int current_dim, Node* child_updated_root) -> void {

        std::cout << "insert_recursively, dim:" << current_dim << std::endl;

        auto &dim_path = paths[current_dim];
        auto &dim_addr = addr[current_dim];

        dim_path.clear();

        // fill in dim_stacks with an outdated path plus (a null pointer or shared child)
        current_root->prepareOutdatedProperPath(child_updated_root, dim_addr, dim_path);

        Node *child = dim_path.back(); // by convention the top of the stack
                                       // is a node that is already updated or
                                       // a nullptr

        for (auto it=dim_path.rbegin()+1;it!=dim_path.rend();++it) {

            Node* parent = *it;

            if (parent->children.size() == 1) {
                parent->setContent(child->getContent(), SHARED); // sharing content with children
            }
            else {
                if (parent->content_link.link_type == SHARED && child &&
                        parent->getContent() == child->getContent()) {
                    std::cout << "Entering strange case" << std::endl;
                    // don't know which case is this one.
                }
                if (current_dim == dimension-1) {
                    // last dimension: insert object into summary
                    if (parent->hasContent() == false) {
                        parent->setContent(new Summary(), PROPER); // create proper content
                    }
                    else if (parent->getContentType() == SHARED) {
                        parent->setContent(new Summary(*parent->getContentAsSummary()), PROPER); // create proper content
                    }
                    parent->getContentAsSummary()->insert(object);
                }
                else {
                    // last dimension: insert object into summary
                    if (parent->hasContent() == false) {
                        parent->setContent(new Node(), PROPER); // create proper content
                    }
                    else if (parent->getContentType() == SHARED) {
                        parent->setContent(parent->getContentAsNode()->shallowCopy(), PROPER); // create proper content
                    }
                    insert_recursively(parent->getContentAsNode(), current_dim+1, child ? child->getContentAsNode() : nullptr);
                }
            }
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

Summary *Node::getContentAsSummary()
{
    return reinterpret_cast<Summary*>(content_link.content);
}

Node *Node::getContentAsNode()
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

LinkType Node::getContentType() const
{
    return content_link.link_type;
}

void Node::prepareOutdatedProperPath(const Node*          child_updated_path,
                                     const DimAddress&    addr,
                                     std::vector<Node *>& result_path)
{
    size_t index = 0;

    auto attachNewProperChain = [&index, &addr, &result_path](Node* node) {
        Node* next = new Node(); // last node

        for (size_t i=addr.size()-1; i>index; --i) {
            Node* current = new Node();
            bool created = false;
            current->setParentChildLink(addr[i], next, PROPER, created);
            next = current;
        }

        { // join the given node with new chain
            bool created = false;
            node->setParentChildLink(addr[index], next, PROPER, created);
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
    const Node* current_parallel_node  = child_updated_path;

//    std::cout << "current_address.level: " << current_address.level << std::endl;
//    std::cout << "address.level:         " << address.level         << std::endl;

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
                break;
            }
            else {
                attachNewProperChain(current_node);
                break;
            }
        }
        else if (next_node_link_type == SHARED) {
            // assert(next_parallel_node);
            if (next_node == next_parallel_node) {
                result_path.push_back(next_node); // append end-marker
                break;
            }
            else {
                // make lazy copy of next_node: its content is going to
                // change and cannot interfere on the original version
                next_node = next_node->shallowCopy();

                bool created = false;
                current_node->setParentChildLink(label, next_node, PROPER, created);
            }
        }

        // update current_parallel_node
        current_parallel_node = next_parallel_node;

        // update current_node, previous_node and stack
        current_node    = next_node;
        result_path.push_back(current_node);

        ++index;

        if (index == addr.size()) {
            result_path.push_back(nullptr); // base case
        }

    }
}

