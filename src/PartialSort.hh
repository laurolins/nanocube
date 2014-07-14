#pragma once

#include <iostream>
#include <unistd.h>
#include <algorithm>
#include <unordered_map>
#include <vector>
#include <string>

#include "WordsCommon.hh"

//-------------------------------------------------------------------------
// PartialSort
//-------------------------------------------------------------------------

struct PartialSort : public WordsHandler {

    PartialSort();

    void insert(Node n);
    void sort(int k);
    void merge(WordsHandler& h, int s);
    void getTopK(int k);

private:

    //-------------------------------------------------------------------------
    // Compare mode
    //-------------------------------------------------------------------------
    struct compare_node
    {
        bool operator()(const Node& n1, const Node& n2) const
        {
            return n1.count > n2.count;
        }
    };

    enum PartialState {DIRTY = 0, SORTED = 1};

    std::vector<Node> v;
    PartialState state;
    int sorted_up_to;
};
