#include <algorithm>
#include <functional>
#include <cctype>
#include <locale>
#include <sstream>
#include <deque>

#include "infix_iterator.hh"
#include "labeled_tree.hh"

#include "tokenizer.hh"

namespace polycover {

//-----------------------------------------------------------------------------
// tree
//-----------------------------------------------------------------------------

namespace labeled_tree {

//-----------------------------------------------------------------------
// Iterator::Action
//-----------------------------------------------------------------------

Iterator::Action::Action():
    type(POP)
{}

Iterator::Action::Action(const Node* parent,
                         const Node* child,
                         const ChildLabel label):
    type(PUSH),
    parent(parent),
    child(child),
    label(label)
{}

Iterator::Iterator(const Node& node) {
    // stack.push_back(Action()); // pop action
    stack.push_back(Action(nullptr, &node, NONE));
}

Iterator::Action* Iterator::next() {
    if (stack.size()) {
        Action a = stack.back();
        stack.pop_back();
        auto node = a.child;
        if (a.type == Action::PUSH) {
            for (int i=3;i>=0;--i) {
                ChildLabel child_label = (ChildLabel) i;
                Node* child = node->children[i].get();
                if (child) {
                    stack.push_back(Action()); // stack a pop action
                    stack.push_back(Action(node, child, child_label));
                }
            }
        }
        last_action = a;
        return &last_action;
    }
    else {
        return nullptr;
    }
}

//-----------------------------------------------------------------------
// io
//-----------------------------------------------------------------------
    
std::ostream& operator<<(std::ostream& os, const Path& path);


std::ostream& operator<<(std::ostream& os, const Node& root) {
    // std::stringstream ss;
    Iterator it(root);
    Iterator::Action::Type previous_type;// = Iterator::Action::POP;
    Iterator::Action *a;
    it.next(); // get rid of root
    while ((a = it.next())) {
        if (a->type == Iterator::Action::POP) {
            os << "<";
        }
        else {
            os << (int) a->label;
        }
        previous_type = a->type;
    }
    return os; // .str();
}
    
std::ostream& json(std::ostream& os, const Node& root) {
    os << "{ \"type\":\"decomposition\", \"code\":\"";
    // std::stringstream ss;
    Iterator it(root);
    Iterator::Action::Type previous_type;// = Iterator::Action::POP;
    Iterator::Action *a;
    it.next(); // get rid of root
    while ((a = it.next())) {
        if (a->type == Iterator::Action::POP) {
            os << "<";
        }
        else {
            os << (int) a->label;
        }
        previous_type = a->type;
    }
    os << "\"}";
    return os; // .str();
}

std::ostream& text(std::ostream& os, const Node& root) {
    int level = 0;
    std::function<void(const Node&)> print = [&os, &level, &print](const Node& u) {
        std::string prefix(4 * level, '.');
        os << prefix << u.path() << ", tag:" << u.tag << std::endl;
        ++level;
        for (auto &c: u.children) {
            if (c.get()) {
                print(*c.get());
            }
        }
        --level;
    };
        
    print(root);
    return os;
}
    
//-----------------------------------------------------------------------
// io
//-----------------------------------------------------------------------
    
std::istream& operator>>(std::istream& is, Node& root) {
    std::vector<Node*> stack;
    stack.push_back(&root);
    while (true) {
        char ch;
        is >> ch;
        if (!is.good())
            break;
        if (ch == '0' || ch == '1' || ch == '2' || ch == '3') {
            ChildLabel lbl = (int) ch - (int) '0';
            stack.push_back(stack.back()->advance(lbl));
        }
        else if (ch == '<') {
            stack.pop_back();
        }
        else {
            break;
        }
    }
    return is;
}

//-----------------------------------------------------------------------
// Tag Impl.
//-----------------------------------------------------------------------
    
Tag::Tag(Type type, int iteration):
    tag_type(type), iteration(iteration)
{}

void Tag::update(const Tag& tag) {
    *this = tag;

//        if (tag_type == NOT_FIXED) {
//            *this = tag;
//        }
//        else if (iteration < tag.iteration) {
//            *this = tag;
//        }
//        else if (iteration == tag.iteration) {
//            if (tag_type == FIXED || tag.tag_type == FIXED) {
//                tag_type = FIXED;
//            }
//        }
//        else if (iteration < tag.iteration) {
//            // throw std::runtime_error("ooops");
//        }
}

std::ostream &operator<<(std::ostream& os, const Tag::Type& ttype) {
    switch (ttype) {
    case Tag::FIXED:
        os << "\"FIXED\"";
        break;
    case Tag::FIXED_ANCESTOR:
        os << "\"FIXED_ANCESTOR\"";
        break;
    case Tag::NOT_FIXED:
        os << "\"NOT_FIXED\"";
        break;
    default:
        break;
    }
    return os;
}

    
std::ostream &operator<<(std::ostream& os, const Tag& tag) {
    os << "{ \"type\":" << tag.tag_type << ", \"iter\":" << tag.iteration << "}";
    return os;
}

    
//-----------------------------------------------------------------------
// Node Impl.
//-----------------------------------------------------------------------

Node::Node(Node* parent, ChildLabel label_to_parent):
    parent(parent),
    label_to_parent(label_to_parent)
{}
    
Node::~Node() {
    children[0].reset();
    children[1].reset();
    children[2].reset();
    children[3].reset();
}

Path Node::path() const {
    Path node_path;
    auto u = this;
    while (u->parent) {
        node_path.data.push_back(u->label_to_parent);
        u = u->parent;
    }
    node_path.reverse();
    return node_path;
}
    
    void Node::trim(int layers_to_go) {
        if (layers_to_go <= 0)
            throw std::runtime_error("Cannot trim to 0 layers");
        
        if (layers_to_go == 1) { // this will be the last layer
            children[0].reset();
            children[1].reset();
            children[2].reset();
            children[3].reset();
        }
        else {
            for (int i=0;i<4;++i) {
                if (children[i].get()) {
                    children[i].get()->trim(layers_to_go-1);
                }
            }
        }
    }
    
Node *Node::advance(ChildLabel child_label)
{
    if (child_label == 1 && path().equalTo(Path({1,2})))
        child_label = (child_label + 1) - 1;
        
        
    Node* child = nullptr;
    if (child_label == NONE)
        throw std::runtime_error("oops");
    child = children[child_label].get();
    if (!child) {
        children[child_label].reset((child = new Node(this, child_label)));
    }
    return child;
}

void Node::deleteChild(ChildLabel child_label)
{
    if (child_label == NONE)
        throw std::runtime_error("oops");
    children[child_label].reset();
}

void Node::deleteAllChildren()
{
    children[0].reset();
    children[1].reset();
    children[2].reset();
    children[3].reset();
}

bool Node::split() {
        
//        if (parent && parent->path().equalTo(Path({1,2})))
//            tag.iteration += 1 - 1;
        
    // make current node be a leaf on a tree
    std::vector<Node*> path;
    auto n = this;
    while (n && n->tag.tag_type == Tag::NOT_FIXED) {
        path.push_back(n);
        n = n->parent;
    }
        
    if (path.size()) {
        auto iter = tag.iteration;
        // assume p is fixed
        for (auto it = path.rbegin(); it != path.rend(); ++it) {
            auto p = (*it)->parent;
            if (p->tag.tag_type == Tag::FIXED_ANCESTOR)
                break;
                
            if (p->tag.tag_type != Tag::FIXED)
                throw std::runtime_error("Needs to be a fixed vertex!");
                
            if (p->getNumChildren() > 1)
                throw std::runtime_error("Can only split fixed vertex if no children exist");

            p->advance(0)->updateTag(Tag(Tag::FIXED,iter));
            p->advance(1)->updateTag(Tag(Tag::FIXED,iter));
            p->advance(2)->updateTag(Tag(Tag::FIXED,iter));
            p->advance(3)->updateTag(Tag(Tag::FIXED,iter));
            p->updateTag(Tag(Tag::FIXED_ANCESTOR,iter));
        }
        return true;
    }
    else {
        return false;
    }

//        if (tag.tag_type == Tag::FIXED) {
//            if (this->getNumChildren() > 0)
//                throw std::runtime_error("Can only split fixed vertex if no children exist");
//            auto iter = tag.iteration;
//            this->advance(0)->updateTag(Tag(Tag::FIXED,iter));
//            this->advance(1)->updateTag(Tag(Tag::FIXED,iter));
//            this->advance(2)->updateTag(Tag(Tag::FIXED,iter));
//            this->advance(3)->updateTag(Tag(Tag::FIXED,iter));
//            this->tag = Tag(Tag::FIXED_ANCESTOR,iter);
//            return true;
//        }
//        else {
//            return false;
//        }
}
    
int Node::getNumChildren() const {
    return
        ((children[0].get() != nullptr) ? 1 : 0) +
        ((children[1].get() != nullptr) ? 1 : 0) +
        ((children[2].get() != nullptr) ? 1 : 0) +
        ((children[3].get() != nullptr) ? 1 : 0);
}
    
int Node::optimize() { // returns 1 if it was optimized to be a leaf, 0 otherwise
        
    // if node has a FIXED tag, then get rid
    // of all its children. Assuming a node
    // that was fixed and then updated with
    // a larger iteration number as an FIXED_ANCESTOR
    // becomes FIXED_ANCESTOR
    if (tag.tag_type == Tag::FIXED) {
        for (auto &ptr: children)
            ptr.reset();
        return 1; // it is a leaf now
    }
    else if (tag.tag_type == Tag::FIXED_ANCESTOR) {
        int children_count  = 0;
        int children_leaves = 0;
        for (int i=0;i<4;++i) {
            if (children[i].get()) {
                ++children_count;
                children_leaves += children[i].get()->optimize();
            }
        }
        if (children_leaves == 4) {
            for (int i=0;i<4;++i)
                children[i].reset(); // get rid of children
            children_count = 0;
        }
        return children_count == 0 ? 1 : 0;
    }
    else {
        throw std::runtime_error("oooops"); // expecting all nodes to be NOT_FIXED
    }
}
    
Summary Node::getSummary() const {
    return Summary(*this);
}

void Node::updateTag(Tag tag) { // make is a fixed node
        
#if 0
    std::deque<int> labels;
    auto u = this;
    while (u->parent != nullptr) {
        labels.push_front((int) u->label_to_parent);
        u = u->parent;
    }
    std::stringstream ss;
    ss << "...update tag of path [ ";
    std::copy(labels.begin(),
              labels.end(),
              infix_ostream_iterator<int>(ss, ","));
    ss << " ] from " << this->tag;
#endif
    this->tag.update(tag);
#if 0
    ss << " to " << this->tag;
    ss << std::endl;
    std::cerr << ss.str();
#endif
}
    
bool Node::isFixed() const {
    return this->tag.tag_type != Tag::NOT_FIXED;
}

//    void Node::fix(int iteration) {
//        
//        //
//        // iteration is positive when it is a proper
//        // fix (meaning whole subtree is contained)
//        //
//        // iteration is negative when it is being
//        // fixed because of a child node
//        //
//        // Note that a positive fix has precedence
//
//        auto current_iteration = abs(fix_iteration);
//        auto new_iteration = abs(iteration);
//        if (new_iteration > current_iteration) {
//            fix_iteration = new_iteration;
//        }
//        else if (new_iteration == current_iteration && iteration > 0) {
//            fix_iteration = iteration; // a proper fix operation has priority
//        }
//        else {
//            throw std::runtime_error("ooops");
//        }
//
//    }

//-----------------------------------------------------------------------
// Summary Impl.
//-----------------------------------------------------------------------
    
struct Item {
    Item() = default;
    Item(int level, const Node* node):
        level(level), node(node)
    {}
    int level;
    const Node *node;
};
    
Summary::Summary(const Node& tree) {
    this->nodes_per_num_children.resize(5,0);
        
    std::vector<Item> stack;
    stack.push_back({0, &tree});
    while (!stack.empty()) {
        Item item = stack.back();
        stack.pop_back();
            
        ++num_nodes;
        nodes_per_num_children[item.node->getNumChildren()]++;
            
        if (nodes_per_level.size() <= item.level) {
            nodes_per_level.resize(item.level+1, 0);
        }
        ++nodes_per_level[item.level];
            
        for (int i=3;i>=0;--i) {
            const Node* child = item.node->children[i].get();
            if (child)
                stack.push_back({item.level+1, child});
        }
    }
}
    
std::size_t Summary::getNumLevels() const {
    return nodes_per_level.size();
}

std::size_t Summary::getNumLeaves() const {
    return nodes_per_num_children[0];
}

std::size_t Summary::getNumNodes()  const {
    return num_nodes;
}

std::size_t Summary::getFirstLevelWithTwoOrMoreNodes() const {
    for (auto i=0; i<nodes_per_level.size(); ++i) {
        if (nodes_per_level[i] > 1)
            return i;
    }
    return nodes_per_level.size();
}

    
std::ostream& operator<<(std::ostream& os, const Summary& summary) {
    os << "...cover" << std::endl;
    os << "......size:       " << summary.getNumNodes()  << std::endl;
    os << "......leaves:     " << summary.getNumLeaves() << std::endl;
    os << "......levels:     "
       << "[" << summary.getFirstLevelWithTwoOrMoreNodes()-1
       << "," << summary.getNumLevels()-1 << "]" << std::endl;
    os << "......num levels: " << summary.getNumLevels();
    return os;
}

    
    
    
    
    
    
    
//-----------------------------------------------------------------------
// Path Impl.
//-----------------------------------------------------------------------

Path::Path(const maps::Tile& tile) {
    for (auto i=0;i<tile.getZoom();++i) {
        data.push_back(tile.getSubTileLabel(i));
    }
}
    
Path::Path(const std::vector<ChildLabel> &labels) {
    data = labels;
}
    
void Path::setTile(const maps::Tile& tile) {
    auto x = tile.x.quantity;
    auto y = tile.y.quantity;
    auto z = tile.getZoom();

    data.resize(z);

    auto mask = 1 << (z - 1);
    for (auto i=0;i<z;++i) {
        data[i] = ((x & mask) != 0 ? 1 : 0) + ((y & mask) != 0 ? 2 : 0);
        mask >>= 1; // shift right one bit
    }
//        auto x = this->x.quantity;
//        auto y = this->y.quantity;
//        auto num_transitions = this->zoom.quantity;
//        if (index >= num_transitions)
//            throw MapsException("ooops");
//        auto bit_x = (x >> (num_transitions - 1 - index)) & 0x1;
//        auto bit_y = (y >> (num_transitions - 1 - index)) & 0x1;
//        return (SubTileLabel) (bit_x | (bit_y << 1));
//            data.push_back(tile.getSubTileLabel(i));
//        }
}

void Path::pop() {
    data.pop_back();
}

void Path::push(ChildLabel child) {
    data.push_back(child);
}

void Path::reverse() {
    std::reverse(data.begin(),data.end());
}

bool Path::equalTo(const Path& path) const {
    if (path.length() != length())
        return false;
    for (auto i=0;i<length();++i) {
        if (path[i] != (*this)[i])
            return false;
    }
    return true;
}
    
    
std::size_t Path::length() const {
    return data.size();
}

std::size_t Path::getLengthOfCommonPrefix(const Path& other) const {
    std::size_t result = 0;
    auto it1 = data.cbegin();
    auto it2 = other.data.cbegin();
    while (it1 != data.cend() && it2 != other.data.cend() && *it1 == *it2) {
        ++result;
        ++it1;
        ++it2;
    }
    return result;
}

ChildLabel& Path::operator[](std::size_t index) {
    return data[index];
}

const ChildLabel& Path::operator[](std::size_t index) const {
    return data[index];
}
    
//-----------------------------------------------------------------------
// Path IO
//-----------------------------------------------------------------------
    
std::ostream& operator<<(std::ostream& os, const Path& path) {
    os << "[";
    for (auto i: path.data) {
        os << " " << (int) i;
    }
    os << " ]";
    return os;
}
    
//-----------------------------------------------------------------------
// CoverTreeEngine Impl.
//-----------------------------------------------------------------------

CoverTreeEngine::CoverTreeEngine():
    root(new Node()),
    current_node(root.get())
{}

    
#define xLOG_COVER_TREE_ENGINE
    
    
void CoverTreeEngine::goTo(const Path &path) {
    auto common_prefix_size = current_path.getLengthOfCommonPrefix(path);

#ifdef LOG_COVER_TREE_ENGINE
    std::cout << "   goto path: " << path << "  from: " << current_path << "  common_prefix_size: " << common_prefix_size << std::endl;
#endif

    auto i = current_path.length() - common_prefix_size;
    while (i > 0) {
        rewind();
        --i;
    }
    for (auto i=common_prefix_size;i<path.length();++i) {
        advance(path[i]);
    }
}

void CoverTreeEngine::rewind() {
    if (current_path.length() == 0)
        throw std::runtime_error("ooops");

    // when rewinding from a NOT_FIXED node release memory
    Node* parent       = current_node->parent;
    auto  child_label  = current_node->label_to_parent;
        
    if (current_node->tag.tag_type == Tag::NOT_FIXED) {
        if (parent) {
            parent->deleteChild(child_label);
        }
    }
    else if (current_node->tag.tag_type == Tag::FIXED) {
        current_node->deleteAllChildren();
    }
        
    if (parent && parent->getNumChildren() > 0) {
        parent->updateTag(Tag(Tag::FIXED_ANCESTOR,this->iteration_tag)); // if there is one fixed child
        // then consolidate parent
    }

    current_node = parent;
    current_path.pop();
}

    
void CoverTreeEngine::advance(ChildLabel child_label) {
    current_node = current_node->advance(child_label);
    current_path.push(child_label);
}

void CoverTreeEngine::consolidate() {

#ifdef LOG_COVER_TREE_ENGINE
    std::cout << "...consolidate " << current_path << std::endl;
#endif
        
    current_node->updateTag(Tag(Tag::FIXED, this->iteration_tag));
    current_node->deleteAllChildren();
}
    
namespace {
        
// trim from start
inline std::string &ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
}
        
// trim from end
inline std::string &rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
}
        
// trim from both ends
inline std::string &trim(std::string &s) {
    return ltrim(rtrim(s));
}
        
}
    
//-----------------------------------------------------------------
// Parser
//-----------------------------------------------------------------
    
void Parser::push(std::string st) {
    // check if st starts with a pound #
    if (st.length() == 0 || st.at(0) == '#')
        return;
        
    if (state == EMPTY) {
        this->description = st;
        state = WITH_DESCRIPTION;
    }
    else if (state == WITH_DESCRIPTION) {
        // parse code from string
        Node node;
        std::stringstream ss(st);
        ss >> node;
        signal.trigger(description, node);
        state = EMPTY;
        ++trigger_count;
    }
}
    
bool Parser::run(std::istream& is, std::size_t max) {
        
    std::size_t base = this->trigger_count;
        
    Parser &parser = *this;
        
    tokenizer::Tokenizer lines(is, '\n', true);
    for (auto it=lines.begin();it!=lines.end();++it) {
        std::string line(trim(*it));
        parser.push(line);
        if (max != 0 && (trigger_count - base) == max) {
            return false;
        }
    }
    return true;
}
    
    //-----------------------------------------------------------------
    // load_from_code
    //-----------------------------------------------------------------
    
    Node* load_from_code(const std::string &code) {
        
        // TODO: limit depth??
        
        CoverTreeEngine engine;
        for (auto ch: code) {
            switch (ch) {
                case '<':
                    engine.rewind();
                    break;
                case '0':
                    engine.advance(0);
                    engine.consolidate();
                    break;
                case '1':
                    engine.advance(1);
                    engine.consolidate();
                    break;
                case '2':
                    engine.advance(2);
                    engine.consolidate();
                    break;
                case '3':
                    engine.advance(3);
                    engine.consolidate();
                    break;
                default:
                    throw std::runtime_error("ooops");
            }
        }
        auto result = engine.root.get();
        engine.root.release();
        return result;
    }
    
} // labaled_tree namespace

} // polycover
