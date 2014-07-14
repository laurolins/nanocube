#pragma once

#include <iostream>
#include <unistd.h>
#include <algorithm>
#include <unordered_map>
#include <vector>
#include <string>

#include "WordsCommon.hh"

//-------------------------------------------------------------------------
// Heap
//-------------------------------------------------------------------------

struct Heap : public WordsHandler {

    Heap();

    void insert(Node n);
    void merge(WordsHandler& h, int s);
    void heapify();
    void getTopK(int k);

private:

    //-------------------------------------------------------------------------
    // Compare mode
    //-------------------------------------------------------------------------
    struct compare_node
    {
        bool operator()(const Node& n1, const Node& n2) const
        {
            return n1.count < n2.count;
        }
    };

    enum HeapState {DIRTY = 0, HEAP = 1};

    std::vector<Node> v;
    HeapState state;
    //boost::heap::fibonacci_heap<HeapNode, boost::heap::compare<compare_node>> heap;
};
