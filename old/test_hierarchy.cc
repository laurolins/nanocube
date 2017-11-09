#include <iostream>
#include <sstream>
#include <string>
#include <algorithm>
#include <iostream>
#include <vector>
#include <cassert>

#include <gtest/gtest.h>

#include "hierarchy.hh"
#include "mmap.hh"

using namespace hierarchy;
using namespace alloc;

std::string code(Hierarchy& h) {
    auto n = h.code();
    std::vector<char> buf(n);
    h.code(&buf[0],buf.size());
    return std::string(&buf[0]);
}

LabelArray la(const std::vector<Label>& values) {
    static std::vector<Label> a;
    a = values;
    return LabelArray(&a[0],a.size());
}

static const std::vector<std::string> expected_codes  {
    ".0-0-0<",
    ".0-0.0<.1<<",
    ".0.0.0<.1<<.1<<",
    ".0.0.0<.1<<.1-1<<",
    ".0.0.0<.1<<.1.0<.1<<<",
    ".0.0.0<.1<<.1.0<.1<<<"
};

static const std::vector<std::vector<Label>> paths {
    {0, 0, 0},
    {0, 0, 1},
    {0, 1},
    {0, 1, 1},
    {0, 1, 0},
    {0, 1}
};

TEST(hierarchy, basic) {

    // mmap 1TB of virtual memory in this process
    alloc::memory_map::MMap mmap(1ULL<<30);
    auto& allocator = alloc::slab::allocator(mmap.memory_block());
    auto hierarchy_cache = allocator.cache_create("Hierarchy", sizeof(Hierarchy));
    auto &hierarchy = *(new (hierarchy_cache->alloc()) Hierarchy(allocator,2));

    auto i = 0;
    for (auto &p: paths) {
        hierarchy.insert(la(p));
        EXPECT_STREQ(code(hierarchy).c_str(),expected_codes[i++].c_str());
    }
}

TEST(hierarchy, duplicate) {

    // mmap 1TB of virtual memory in this process
    alloc::memory_map::MMap mmap(1ULL<<30);

    alloc::ConstMemoryBlock memblock;
    
    { // fill in mmap with data
        auto& allocator = alloc::slab::allocator(mmap.memory_block());
        auto hierarchy_cache = allocator.cache_create("Hierarchy", sizeof(Hierarchy));
        auto &hierarchy = *(new (hierarchy_cache->alloc()) Hierarchy(allocator,2));
        allocator.root(&hierarchy);
        
        // insert paths
        for (auto &p: paths) { hierarchy.insert(la(p)); }
        
        // memory block
        memblock = allocator.memory_block();
    }

    { // copy object to dupbuffer
        std::vector<char> dupbuffer(memblock.size());
        std::copy((const char*)memblock.base(),
                  (const char*)memblock.base() + memblock.size(),
                  &dupbuffer[0]);

        auto &allocator = *((alloc::slab::Allocator*) &dupbuffer[0]);
        auto &hierarchy = *((Hierarchy*)allocator.root());
        
        EXPECT_STREQ(code(hierarchy).c_str(),expected_codes.back().c_str());
    }
}