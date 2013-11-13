#include <simple_nanocube.hh>

#include <iostream>
#include <functional>
#include <algorithm>
#include <iterator>
#include <sstream>
#include <cassert>

#include <log.hh>


#define LOG

//-----------------------------------------------------------------------------
// Content
//-----------------------------------------------------------------------------

void Content::setOwner(Node *owner) {
    this->owner = owner;
}

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
// Address
//-----------------------------------------------------------------------------

Address::Address(RawAddress data):
    data(data)
{}

Address& Address::appendDimension()
{
    data.push_back(DimAddress());
    return *this;
}

Address& Address::appendLabel(Label label)
{
    data.back().push_back(label);
    return *this;
}

size_t Address::size() const
{
    return data.size();
}

size_t Address::size(int index) const
{
    return data[index].size();
}

DimAddress &Address::operator[](size_t index)
{
    return data[index];
}

const DimAddress &Address::operator[](size_t index) const
{
    return data[index];
}

//-----------------------------------------------------------------------------
// Address IO
//-----------------------------------------------------------------------------

std::ostream& operator<<(std::ostream& os, const Address& addr) {
    os << "Addr[";
    for (size_t i=0;i<addr.size();++i) {
        if (i > 0) {
            os << ",";
        }
        os << "{";
        std::ostream_iterator<int> out_it (os,",");
        std::copy ( addr[i].begin(), addr[i].end()-1, out_it );
        if (addr[i].begin() != addr[i].end()) {
            os << addr[i].back();
        }
        os << "}";
    }
    os << "]";
    return os;
}


//-----------------------------------------------------------------------------
// ContentLink
//-----------------------------------------------------------------------------

ContentLink::ContentLink(Content *content, LinkType link_type):
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

Nanocube::Nanocube(Nanocube &&nc)
{
    std::swap(nc.root,this->root);
    std::swap(nc.levels, this->levels);
    std::swap(nc.dimension, this->dimension);
}

Nanocube& Nanocube::operator=(Nanocube &&nc) {
    std::swap(nc.root,this->root);
    std::swap(nc.levels, this->levels);
    std::swap(nc.dimension, this->dimension);
    return *this;
}

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

Node *Nanocube::getNode(const Address &addr)
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
    return node;
}

namespace highlight {
    static const int CLEAR          = 0;
    static const int MAIN           = 1;
    static const int PARALLEL       = 2;
    static const int UPSTREAM_CHECK = 3;
}


struct LogMsg {
    LogMsg(std::string msg) {
        logging::getLog().pushMessage(msg);
    }
    ~LogMsg() {
        logging::getLog().popMessage();
    }
};

std::string log_name(Content* content) {
    return logging::name((logging::NodeID) content);
}

//std::string log_name(Summary* summary) {
//    return logging::name((logging::NodeID) summary);
//}

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

static void log_update_child_link(Node* node, Label label) {
    LinkType link_type;
    Node* child = node->getChild(label,link_type);
    logging::getLog().setChildLink((logging::NodeID) node,
                                   (logging::NodeID) child,
                                   label,
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

//static void log_update_content_link(Node* node) {
//    LinkType link_type = node->getContentType();
//    void* content = node->getContent();
//    logging::getLog().setContentLink((logging::NodeID) node,
//                                       (logging::NodeID) content,
//                                       link_type == SHARED ? logging::SHARED : logging::PROPER);
//}

static void log_highlight_node(const Node* node, logging::Color color) {
    logging::getLog().highlightNode((logging::NodeID)node, color);
}

static void log_highlight_child_link(const Node* node, Label label, logging::Color color) {
    logging::getLog().highlightChildLink((logging::NodeID)node,label,color);
}

static void log_highlight_content_link(const Node* node, logging::Color color) {
    logging::getLog().highlightContentLink((logging::NodeID)node,color);
}





//-----------------------------------------------------------------------------
// UpstreamThread
//-----------------------------------------------------------------------------

struct UpstreamThread {
public:
    UpstreamThread(Node *node);
    virtual ~UpstreamThread();
    Node* getNext() const;
    Node *getNext(bool &is_content) const;
    void  advance();
    void  rewind();
    void  pop();
    Node* top();

    Address getAddress() const;
public:
    std::vector<Node*> stack;
};

//-----------------------------------------------------------------------------
// UpstreamThread Impl.
//-----------------------------------------------------------------------------

UpstreamThread::UpstreamThread(Node *node)
{
    stack.push_back(node);
#ifdef LOG
    log_highlight_node(node, highlight::UPSTREAM_CHECK);
#endif
}

UpstreamThread::~UpstreamThread() {
    while (!stack.empty()) {
        rewind();
    }
}

Node* UpstreamThread::getNext(bool &is_content) const {
    Node* top = stack.back();
    if (top->proper_parent != nullptr) {
        is_content = false;
        return top->proper_parent;
    }
    else {
        is_content = true;
        return top->owner;
    }
}

Node* UpstreamThread::getNext() const {
    bool is_content;
    return getNext(is_content);
}

void UpstreamThread::advance() {
    bool is_content;
    stack.push_back(this->getNext(is_content));

#ifdef LOG
    Node* child  = *(stack.rbegin()+1);
    Node* parent = stack.back();
    if (is_content) {
        log_highlight_content_link(parent, highlight::UPSTREAM_CHECK);
    }
    else {
        log_highlight_child_link(parent, child->label, highlight::UPSTREAM_CHECK);
    }
    log_highlight_node(parent, highlight::UPSTREAM_CHECK);
#endif
}

void UpstreamThread::rewind() {
#ifdef LOG
    Node* parent = stack.back();
    log_highlight_node(parent, highlight::CLEAR);
    if (stack.size() > 1) {
        Node* child  = *(stack.rbegin()+1);
        if (child->proper_parent != nullptr) {
            log_highlight_child_link(parent, child->label, highlight::CLEAR);
        }
        else {
            log_highlight_content_link(parent, highlight::CLEAR);
        }
    }
#endif
    stack.pop_back();
}

Address UpstreamThread::getAddress() const
{
    Address result;

    std::vector<Label> labels; // assuming -1 is not a label (used as a separator)
    for (Node* node: stack) {
        if (node->proper_parent != nullptr) {
            labels.push_back(node->label);
        }
        else if (node->owner != nullptr) {
            labels.push_back(-1);
        }
    }

    if (labels.size() > 0) {
        result.appendDimension();
        for (auto it = labels.rbegin();it!= labels.rend();++it) {
            if (*it == -1) {
                result.appendDimension();
            }
            else {
                result.appendLabel(*it);
            }
        }
    }

    return result;
}

//-----------------------------------------------------------------------------
// Thread
//-----------------------------------------------------------------------------

struct Thread {
public:
    enum ThreadType { UNDEFINED, MAIN, PARALLEL };
    enum ItemType   { ROOT, CHILD, CONTENT };
public:
    struct Item {
        Item() = default;
        Item(Node* node, ItemType item_type, int dim, int layer);
        Node*    node      { nullptr };
        ItemType item_type { ROOT };
        int      dim       { 0 };
        int      layer     { 0 };
    };
private:
    void flagTopNode(bool b);
public:
    Thread() = default;
    Thread(ThreadType thread_type);

    Address getAddress() const;

    void  start(Node *node, int dim, int layer);
    void  advanceContent();
    void  advanceChild(Label label);
    void  rewind();
    int   getCurrentDimension() const;
    int   getCurrentLayer() const;
    Node* top() const;
public:
    ThreadType        thread_type;
    std::vector<Item> stack;
};

//-----------------------------------------------------------------------------
// Thread::Item Impl.
//-----------------------------------------------------------------------------

Thread::Item::Item(Node *node, ItemType item_type, int dim, int layer):
    node(node),
    item_type(item_type),
    dim(dim),
    layer(layer)
{}

//-----------------------------------------------------------------------------
// Thread Impl.
//-----------------------------------------------------------------------------

Thread::Thread(Thread::ThreadType thread_type):
    thread_type(thread_type)
{}

void Thread::flagTopNode(bool b) {
    if (b) {
        if (thread_type == MAIN) {
            top()->setFlag(Node::IN_MAIN_PATH);
        }
        else if (thread_type == PARALLEL) {
            top()->setFlag(Node::IN_PARALLEL_PATH);
        }
    }
    else {
        top()->setFlag(Node::NONE);
    }
}

int Thread::getCurrentDimension() const {
    return stack.back().dim;
}

int Thread::getCurrentLayer() const {
    return stack.back().layer;
}

void Thread::start(Node* node, int dim, int layer) {
    assert(stack.empty());
    stack.push_back(Item(node,ROOT,dim,layer));
    flagTopNode(true);

#ifdef LOG
    log_highlight_node(node, thread_type == MAIN ? highlight::MAIN : highlight::PARALLEL);
#endif

}

void Thread::advanceContent() {
    assert(!stack.empty());

    // assuming content is a node
    Node* content = this->top()->getContentAsNode();

    assert (content);

#ifdef LOG
    log_highlight_content_link(this->top(), thread_type == MAIN ? highlight::MAIN : highlight::PARALLEL);
    log_highlight_node(content, thread_type == MAIN ? highlight::MAIN : highlight::PARALLEL);
#endif

    stack.push_back(Item(content, CONTENT, getCurrentDimension()+1, 0));
    flagTopNode(true);


}

void Thread::advanceChild(Label label) {
    assert(!stack.empty());

    // assuming content is a node
    Node* child = this->top()->getChild(label);
    assert (child);

#ifdef LOG
    log_highlight_child_link(this->top(), label, thread_type == MAIN ? highlight::MAIN : highlight::PARALLEL);
    log_highlight_node(child, thread_type == MAIN ? highlight::MAIN : highlight::PARALLEL);
#endif

    stack.push_back(Item(child, CHILD, getCurrentDimension(), getCurrentLayer()+1));
    flagTopNode(true);
}

Node* Thread::top() const {
    return stack.back().node;
}

void Thread::rewind() {
    flagTopNode(false); // unflag
    Item item = stack.back();
    stack.pop_back();

#ifdef LOG
    log_highlight_node(item.node, highlight::CLEAR);
    if (item.item_type == CONTENT) {
        log_highlight_content_link(top(), highlight::CLEAR);
    }
    else if (item.item_type == CHILD) {
        log_highlight_child_link(top(), item.node->label, highlight::CLEAR);
    }
#endif
}

Address Thread::getAddress() const {
    Address result;
    for (const Item& it: stack) {
        if (it.item_type == ROOT) {
            result.appendDimension();
        }
        else if (it.item_type == CHILD) {
            result.appendLabel(it.node->label);
        }
        else if (it.item_type == CONTENT) {
            result.appendDimension();
        }
    }
    return result;
}

//-----------------------------------------------------------------------------
// ThreadStack
//-----------------------------------------------------------------------------

struct ThreadStack {
public:
    ThreadStack(Thread::ThreadType thread_type);
    void pop();
    void rewind();
    void push(Node* root, int dimension, int layer);
    void advanceContent();
    void advanceChild(Label label);
    Node* getFirstProperChild(Label label) const;
    Content* getAnyContent() const;
    Summary* getFirstSummary() const;
    Thread& top();
public:
    Thread::ThreadType thread_type;
    std::vector<Thread> stack;
};

//-----------------------------------------------------------------------------
// ThreadStack Impl.
//-----------------------------------------------------------------------------

ThreadStack::ThreadStack(Thread::ThreadType thread_type):
    thread_type(thread_type)
{}

void ThreadStack::pop() {
    stack.pop_back();
}

void ThreadStack::push(Node* root, int dimension, int layer)
{
    stack.push_back(Thread(thread_type));
    stack.back().start(root, dimension, layer);
}

void ThreadStack::advanceChild(Label label)
{
    for (auto &th: stack) {
        th.advanceChild(label);
    }
}

void ThreadStack::rewind()
{
    for (auto &th: stack) {
        th.rewind();
    }
}

Node *ThreadStack::getFirstProperChild(Label label) const
{
    LinkType link_type;
    for (auto &th: stack) {
        Node* child = th.top()->getChild(label, link_type);
        if (link_type == PROPER) {
            return child;
        }
    }
    return nullptr;
}

Content* ThreadStack::getAnyContent() const
{
    for (const Thread &th: stack) {
        Content* content = th.top()->getContent();
        return content;
    }
    return nullptr;
}

Summary* ThreadStack::getFirstSummary() const
{
    for (const Thread &th: stack) {
        return th.top()->getContentAsSummary();
    }
    return nullptr;
}

Thread &ThreadStack::top()
{
    return stack.back();
}

void ThreadStack::advanceContent()
{
    for (auto &th: stack) {
        th.advanceContent();
    }
}





//-----------------------------------------------------------------------------
// special function
//-----------------------------------------------------------------------------

bool can_switch(const Address& main_address,
              const Address &shared_address,
              const Address& object_address,
              Address& switch_address) {

    // be hypothesis here, main_address is a coarser version of
    // both shared and object address. We want to know if we can
    // add one constraint (one more label in one dimension)

    int n = main_address.size();

    assert(main_address.size() == shared_address.size());

    // check which dimension we can add a constraint
    bool result = false;
    for (int i=0;i<n-1;i++) {
        int j = main_address.size(i);
        if (main_address.size(i) < shared_address.size(i) && object_address[i][j] == shared_address[i][j]) {
            result = true;
            switch_address = main_address;
            switch_address[i].push_back(object_address[i][j]);
        }
    }
    return result;
}




//-----------------------------------------------------------------------------
// Nanocube Insert
//-----------------------------------------------------------------------------

void Nanocube::insert(const Address &addr, const Object &object)
{

#ifdef LOG
    std::stringstream ss;
    ss << "Insert obj: " << object << " addr: " << addr;
    logging::getLog().pushMessage(ss.str());
#endif

    Thread       main_path(Thread::MAIN);
    ThreadStack  mf_paths(Thread::PARALLEL); // minimally finer paths


    if (root == nullptr) {
        root = new Node();
#ifdef LOG
        logging::getLog().newNode((logging::NodeID)root, 0, 0);
#endif
    }

    std::function<void(Thread&, ThreadStack&)> insert_recursively;

    int dimension = this->dimension;

    Nanocube &nanocube = *this;

    //
    // Retrieve address of a node and check if it is contained in
    // a path in the minimally finer set of paths (mf_paths) for
    // the current address
    //
    auto find_switch_node = [&addr, &nanocube](Node* shared_child, const Address &child_addr) -> Node* {

        UpstreamThread upstream_thread(shared_child);
        while (upstream_thread.getNext()) {
            upstream_thread.advance();
        }
        Address child_proper_addr = upstream_thread.getAddress();

        std::stringstream ss;
        ss << "COMPLEX CASE... shared child addr: " << child_addr << "   child proper addr: " << child_proper_addr;
        LogMsg msg(ss.str());

        Node* switch_node = nullptr;

        Address switch_address;

        if ( can_switch(child_addr, child_proper_addr, addr, switch_address) ) {
            // find node

            ss << "... switch_address: " << switch_address << "   parallel: " << child_addr;
            LogMsg msg(ss.str());

            switch_node = nanocube.getNode(switch_address);
            if (switch_node == nullptr) {
                throw std::runtime_error("OOopps");
            }

#ifdef LOG
            {
                std::stringstream ss;
                ss << "Switch node on address " << switch_address << " is node " << log_name(switch_node);
                LogMsg msg(ss.str());
            }
#endif
        }

        return switch_node;
    };

    //
    // Recursive Insertion Function
    //

    insert_recursively = [&nanocube, &insert_recursively, &addr, &object, &dimension, &find_switch_node]
            (Thread& main_path, ThreadStack& mf_paths) -> void {

#ifdef LOG
        std::stringstream ss;
        auto &log = logging::getLog();
        ss << "insert_recursively on node " << log.getNodeName((logging::NodeID) main_path.top()) << " dim " << main_path.getCurrentDimension();
        LogMsg log_insert_recursively(ss.str());
#endif

        //
        // Part 1: Advance main and parallel threads until:
        //
        // (a) the top of the main thread has a shared child that
        //     can be proven to be contained in one of the parallel
        //     threads. In this case switch the shared child accordingly
        //     and stop.
        //
        // (b) top of the main thread has no child, in which case
        //     we can share from a proper parallel child or in case
        //     it doesn't exist, create a new child on the main branch
        //

        auto &dim_addr = addr[main_path.getCurrentDimension()];

        int index = 0;
        while (index < (int) dim_addr.size()) {

            Label label  = dim_addr[index];

            Node* parent = main_path.top();

            LinkType link_type;
            Node* child = parent->getChild(label, link_type);

            if (child) {

                if (link_type == SHARED) {

                    // check if we can end Part 1 by preserving or reswitching child node.

                    Address child_addr = main_path.getAddress().appendLabel(label);

                    Node* switch_node = find_switch_node(child, child_addr);

                    if (switch_node) {
                        Node* new_child = switch_node; // parallel_threads.getFirstProperChild(label);
                        if (!new_child) {
                            throw std::runtime_error("oops");
                        }
                        if (new_child != child) {
                            parent->setParentChildLink(label, new_child, SHARED);
#ifdef LOG
                            log_update_child_link(parent,label);
#endif
                        }
                        break;
                    }
                    else {
                        Node *new_child = child->shallowCopy();
#ifdef LOG
                        {
                            LogMsg msg("...cannot switch; shallow copy of " + log_name(child) +" into " + log_name(new_child));
                            log_shallow_copy(new_child,main_path.getCurrentDimension(),main_path.getCurrentLayer()+1);
                        }
#endif
                        parent->setParentChildLink(label, new_child, PROPER);
#ifdef LOG
                        {
                            LogMsg msg("...setting shallow copied node as child " + std::to_string(label) + " of node " + log_name(parent));
                            log_update_child_link(parent,label);
                        }
#endif
                    }
                }

                // if it is a proper child, just continue

            } // is there a child?
            else {
                // there is no child

                // find parallel proper child, share it and end Part 1

                //
                // Proof: this branch doesn't exist, but it need to exist in all
                // parallel paths. And it is the exact branch we want.
                //

                Node* new_child = mf_paths.getFirstProperChild(label);
                if (new_child) {
                    parent->setParentChildLink(label, new_child, SHARED);
#ifdef LOG
                    log_update_child_link(parent,label);
#endif
                    break;
                }
                else {
                    // no parallel branch, so we need a new node
                    parent->setParentChildLink(label, new Node(), PROPER);
#ifdef LOG
                    log_new_node(parent->getChild(label),main_path.getCurrentDimension(),main_path.getCurrentLayer()+1);
                    log_update_child_link(parent,label);
#endif
                }
            }

            main_path.advanceChild(label);
            mf_paths.advanceChild(label);

            ++index;
        }


        // Last node dimension
        bool last_node_dim = (dimension-1 == main_path.getCurrentDimension());

        // Part 2: check the content of the main thread
        for (;index >= 0;--index) {

            std::stringstream ss;
            ss << "index: " << index
               << "  dim: " << main_path.getCurrentDimension()
               << "  layer: " << main_path.getCurrentLayer()
               << "  last_node_dim: " << last_node_dim;

            std::cout << ss.str() << std::endl;

            LogMsg msg(ss.str());

            Node* parent = main_path.top();

            // general case
            if (parent->getNumChildren() == 1) {
                Label label = dim_addr[index];
                Node* child = parent->getChild(label);
                parent->setContent(child->getContent(), SHARED); // sharing content with children
#ifdef LOG
                log_update_content_link(parent);
#endif

                // done updating
            }

            else if (parent->hasContent() == false) {
                Content* shared_content = mf_paths.getAnyContent();
                if (shared_content) {
                    parent->setContent(shared_content, SHARED); // create proper content
#ifdef LOG
                    log_update_content_link(parent);
#endif
                }
                else {
                    Content *new_content = (last_node_dim ? (Content*) new Summary() : (Content*) new Node());
                    parent->setContent(new_content, PROPER); // create proper content

#ifdef LOG
                    log_new_node(parent->getContentAsNode(),main_path.getCurrentDimension()+1,0);
                    log_update_content_link(parent);
#endif
                }
            }
            else if (parent->getContentType() == SHARED) {

                // check if we can switch content for one on mf_paths
                Node* switch_node = nullptr;
                if (!last_node_dim) {
                    Node*   shared_content = parent->getContentAsNode();
                    Address shared_content_address = main_path.getAddress().appendDimension();
                    switch_node = find_switch_node(shared_content, shared_content_address);
                    if (switch_node) {
                        parent->setContent(switch_node, SHARED); // create proper content
#ifdef LOG
                        log_update_content_link(parent);
#endif
                    }
                    else {
                        Node* old_content = parent->getContentAsNode();
                        parent->setContent(old_content->shallowCopy(), PROPER); // create proper content
#ifdef LOG
                        log_shallow_copy(parent->getContentAsNode(),main_path.getCurrentDimension()+1,0);
                        log_update_content_link(parent);
#endif
                    }
                }

                else { // last dimension

                    Node*   shared_content_parent = parent->getContent()->owner;
                    Address shared_content_parent_address = main_path.getAddress();
                    switch_node = find_switch_node(shared_content_parent, shared_content_parent_address);
                    if (switch_node) {
                        Summary* shared_summary = switch_node->getContentAsSummary();
                        parent->setContent(shared_summary, SHARED); // create proper content
#ifdef LOG
                        log_update_content_link(parent);
#endif
                    }
                    else {
                        // shallow copy old summary
                        Summary* old_summary = parent->getContentAsSummary();
                        parent->setContent(new Summary(*old_summary), PROPER); // create proper content
#ifdef LOG
                        {
                            LogMsg msg("...shallow copy of " +  log_name(old_summary)
                                       + " into " + log_name(parent->getContentAsSummary()));
                            log_shallow_copy(parent->getContentAsSummary(),main_path.getCurrentDimension()+1,0);
                        }
                        {
                            LogMsg msg("...setting content link of node " +  log_name(parent));
                            log_update_content_link(parent);
                        }
#endif
                    }
                }
            }



            // recurse or solve it if in the last dimension
            if (parent->getContentType() == PROPER) {
                if (last_node_dim) {
                    parent->getContentAsSummary()->insert(object);

#ifdef LOG
                    {
                        LogMsg msg("...storing object into " +  log_name(parent->getContent()));
                        logging::getLog().store((logging::NodeID)parent->getContent(), object);
                    }
#endif
                }
                else {
                    // recurse
                    if (parent->getNumChildren() > 0) {
                        Label label = dim_addr[index];
                        Node* child = parent->getChild(label);
                        mf_paths.push(child, main_path.getCurrentDimension(), main_path.getCurrentLayer()+1);
                    }

                    // go to next dimension
                    main_path.advanceContent();
                    mf_paths.advanceContent();

                    insert_recursively(main_path, mf_paths);

                    //parallel_threads.rewind();
                    //main_thread.rewind();

                    if (parent->getNumChildren() > 0) {
                        mf_paths.top().rewind();
                        mf_paths.pop();
                    }
                }
            }

            mf_paths.rewind();
            main_path.rewind();

        }

    }; // end lambda function insert_recursively

    // start main thread
    main_path.start(root,0,0);

    // insert
    insert_recursively(main_path, mf_paths);

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

Content* Node::getContent() const {
    return this->content_link.content;
}

void Node::setContent(Content *content, LinkType type)
{
    this->content_link.content = content;
    this->content_link.link_type = type;
    if (type == PROPER) {
        content->setOwner(this);
    }
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

int Node::getNumChildren() const
{
    return children.size();
}

void Node::setParentChildLink(Label label, Node *node, LinkType link_type) {
    bool created;
    this->setParentChildLink(label,node,link_type,created);
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

void Node::setFlag(Node::Flag flag) const
{
    this->flag = flag;
}

LinkType Node::getContentType() const
{
    return content_link.link_type;
}






