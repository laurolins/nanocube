#pragma once

#include <unordered_map>
#include <memory>
#include "qtfilter.hh"

namespace nanocube {

// Cache will only be used
struct Cache {
    qtfilter::Node* getMask(void* key);
    void insertMask(void* key, qtfilter::Node* mask);
public:
    std::unordered_map<void*, std::unique_ptr<qtfilter::Node>> polygon_masks; //
};

}
