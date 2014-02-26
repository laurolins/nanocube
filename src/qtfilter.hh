#pragma once

#include <vector>
#include "geom2d/tile.hh"
#include "geom2d/polygon.hh"

namespace qtfilter {

//-----------------------------------------------------------------------------
// Node
//-----------------------------------------------------------------------------

struct Node {
    enum Flag { NONE, KEEP, KEEP_AND_FULL };

    Node();
    ~Node();

    auto getOrCreateChildren(int index) -> Node*;
    auto getChildren(int index) const -> Node*;
    auto operator[](int index) const -> Node*;

    void removeChildren(Flag flag);

    void clear();

    bool isLeaf() const;
    bool isFull() const;

    int  countChildren(Flag flag) const;

    int  getNumChildren() const;

private:
    Node* children[4]; // = { nullptr, nullptr, nullptr, nullptr };
public:
    Flag  flag { NONE };
};


//-----------------------------------------------------------------------------
// QuadTreeCreator
//-----------------------------------------------------------------------------

struct QuadTreeCreator {

    enum ActionType { NONE, TEST, POST_PROCESS };

    struct Action {
        Action() = default;
        Action(QuadTreeCreator *creator,
               Node* parent,
               geom2d::Tile  parent_tile,
               int   child_index,
               ActionType type);

        auto getTile() const -> geom2d::Tile;

        auto getNode() const -> Node*;

        QuadTreeCreator* creator     { nullptr };
        Node*            parent      { nullptr };
        geom2d::Tile     parent_tile { 0, 0, 0 };
        int              child_index { 0       };
        ActionType       type        { NONE    };
    };

    enum ToDo { KEEP, CONTINUE, SPLIT };

    bool next();

    Action& getAction();

    void split();
    void keep();

//    void push(int index);
//    void popAndKeep();
//    void popAndErase();
//    bool optimize { false };

    Node *root { nullptr };

    ToDo next_todo { CONTINUE };

    std::vector<Action> pending_actions;

};


//-----------------------------------------------------------------------------
// QuadTreeIterator
//-----------------------------------------------------------------------------

struct QuadTreeIterator {

    struct Item {
        Item() = default;
        Item(Node *node, geom2d::Tile tile):
            node {node},
            tile {tile}
        {}
        Node *node { nullptr };
        geom2d::Tile  tile;
    };

    QuadTreeIterator(Node* node, geom2d::Tile tile);

    bool next();

    Node*         getCurrentNode();
    geom2d::Tile  getCurrentTile();

    bool               only_leaves { false };
    Node*              root { nullptr };

    geom2d::Tile       root_tile;
    std::vector<Item>  stack;

};


//
//
//

auto intersect(const geom2d::Polygon &poly,
               int max_level,
               bool optimize) -> Node*;



}
