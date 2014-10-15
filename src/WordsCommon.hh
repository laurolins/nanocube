#pragma once

#include <string>

//-------------------------------------------------------------------------
// Hash
//-------------------------------------------------------------------------

// unsigned long hashstring(const char *str)
// {
//     unsigned long hash = 5381;
//     int c;
//
//     while (c = *str++)
//         hash = ((hash << 5) + hash) + c; //hash * 33 + c
//
//     return hash;
// }


// struct HashTable {
//
//     HashTable();
//     void insert(std::string);
//
// private:
//     //Alternatives: http://incise.org/hash-table-benchmarks.html
//     std::unordered_map<std::string, int> m;
// };

//-------------------------------------------------------------------------
// Node
//-------------------------------------------------------------------------

struct Node {

    Node(std::string w):
        word(w),
        count(1)
    { }

    std::string   word;
    int           count;
};

//-------------------------------------------------------------------------
// WordsHandler: Abstract class to be implemented
//-------------------------------------------------------------------------

struct WordsHandler {

    virtual void insert(Node n) = 0;
    virtual void merge(WordsHandler& h, int s) = 0;
    virtual void getTopK(int k) = 0;
};
