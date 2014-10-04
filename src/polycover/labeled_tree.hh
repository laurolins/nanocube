#pragma once

#include <memory>
#include <vector>
#include <iostream>

#include "signal.hh"

#include "maps.hh"

namespace polycover {

//
// Labels are numbers from 0 to 255
//

//-----------------------------------------------------------------------------
// tree
//-----------------------------------------------------------------------------

namespace labeled_tree {

using ChildLabel = uint8_t;

const uint8_t NONE = 255;
    
struct Summary;
struct Path;
    
//-----------------------------------------------------------------------
// Tag
//-----------------------------------------------------------------------
    
struct Tag {
public:
    enum Type { NOT_FIXED, FIXED, FIXED_ANCESTOR };
public:
    Tag() = default;
    Tag(Type type, int iteration);
    void update(const Tag& tag);
public:
    Type tag_type { NOT_FIXED };
    int  iteration { 0 };
};
    
std::ostream &operator<<(std::ostream& os, const Tag::Type& ttype);
std::ostream &operator<<(std::ostream& os, const Tag& tag);
    
//-----------------------------------------------------------------------
// Node
//-----------------------------------------------------------------------

struct Node {
public:
    Node() = default;
    ~Node();
        
    Node(Node* parent, ChildLabel label_to_parent);

    Node* advance(ChildLabel child_label);

    Path path() const;
        
    void deleteChild(ChildLabel child_label);

    void deleteAllChildren();
    
    void trim(int layers_to_go);
        
    bool split();

    int getNumChildren() const;
        
    int optimize();
        
    Summary getSummary() const;

    bool isFixed() const;
        
    void updateTag(Tag tag); // make is a fixed node

public:
    Node*                 parent            { nullptr   };
    ChildLabel            label_to_parent   { NONE      };
    Tag                   tag;
    std::unique_ptr<Node> children[4];
};


//-----------------------------------------------------------------------
// Summary
//-----------------------------------------------------------------------
    
struct Summary {
    Summary(const Node& tree);
        
    std::size_t getNumLevels() const;
    std::size_t getNumLeaves() const;
    std::size_t getNumNodes()  const;
        
    std::size_t getFirstLevelWithTwoOrMoreNodes() const;
        
    std::size_t num_nodes { 0 };
    std::vector<std::size_t> nodes_per_level;
    std::vector<std::size_t> nodes_per_num_children;
};
    
    
std::ostream& operator<<(std::ostream& os, const Summary& summary);
    
    
//-----------------------------------------------------------------------
// Node
//-----------------------------------------------------------------------

struct Iterator {
public:
    struct Action {
    public:
        enum Type { PUSH, POP };
    public:
        Action();
        Action(const Node* parent,
               const Node* child,
               ChildLabel label);
    public:
        Type         type;
        const Node* parent { nullptr };
        const Node* child  { nullptr };
        ChildLabel  label  { NONE };
    };

public:
    Iterator(const Node& node);
    Action *next(); // repeat until null
public:
    Action last_action;
    std::vector<Action> stack;
};


//-----------------------------------------------------------------------
// Write down the code of a tree
//-----------------------------------------------------------------------

std::ostream& operator<<(std::ostream& os, const Node& root);
    
    
std::ostream& json(std::ostream& os, const Node& root);
std::ostream& text(std::ostream& os, const Node& root);


//-----------------------------------------------------------------------
// Path
//-----------------------------------------------------------------------

struct Path {
    Path() = default;
    Path(const maps::Tile& tile);
    Path(const std::vector<ChildLabel> &labels);

    void setTile(const maps::Tile& tile);

    std::size_t length() const;
    void pop();
    void push(ChildLabel child);

    ChildLabel& operator[](std::size_t index);
    const ChildLabel& operator[](std::size_t index) const;

    void reverse();
    bool equalTo(const Path& path) const;

public:
    std::size_t getLengthOfCommonPrefix(const Path& other) const;
    std::vector<ChildLabel> data;
};

    
std::ostream& operator<<(std::ostream& os, const Path& path);

//-----------------------------------------------------------------------
// CoverTreeEngine
//-----------------------------------------------------------------------

struct CoverTreeEngine {
public:

    CoverTreeEngine();
        
    void goTo(const Path &path);

    void advance(ChildLabel child);

    void rewind();

    void consolidate();
        
    int                   iteration_tag { 0 }; // when consolidating and rewinding
    // use this tag
        
    std::unique_ptr<Node> root;
    Node*                 current_node { nullptr };
    Path                  current_path;
};

//-----------------------------------------------------------------
// Parser
//-----------------------------------------------------------------
    
struct Parser { // .ttt files (tile tree tiles)
        
    enum State { EMPTY, WITH_DESCRIPTION };
        
    // format:
    //     (
    //     code_string_description <line_feed>
    //     code_string <line_feed>
    //     )*
        
    // true if stream is finished, false if "max" interruption occurred
    bool run(std::istream& is, std::size_t max);
        
    void push(std::string st);   // start a new area
        
    State       state { EMPTY };
    std::string description;
        
    std::size_t trigger_count { 0 };
        
    sig::Signal<const std::string&, const labeled_tree::Node&> signal;
};
    
    Node* load_from_code(const std::string &code);

} // labeled_tree

} // polycover
