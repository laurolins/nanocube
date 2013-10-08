#pragma once

#include <vector>

#include <simple_nanocube.hh>

//-----------------------------------------------------------------------------
// Iterator
//-----------------------------------------------------------------------------

struct Iterator {
public:

    struct Item {
    public:
        Item() = default;
        Item(Node* node, Node* parent_node, Label label, int dimension, int level,
             LinkType link_type);

    public:
        Node*    node { nullptr };
        Node*    parent_node { nullptr };
        Label    label { 0 };
        int      dimension { -1 };
        int      level { 0 };
        LinkType link_type { PROPER }; // if level == 0 than this is
                                       // a "content" link type, otherwise
                                       // it is a parent child linkt type
    };

public:
    Iterator(const Nanocube &nc);
    bool next();

    bool lastDimension() const; // current item is in the last internal layer

public:
    const Nanocube &nanocube;
    Item  current_item;
    Item *current_item_ptr { nullptr };
    std::vector<Item> stack;
};











