#pragma once

#include <cstdint>

#include "common.hh"

namespace alloc {
    
    namespace memory_map {
        
        //-----------------------------------------------------------------------------
        // MMap
        //-----------------------------------------------------------------------------
        
        struct MMap {
        public:
            MMap(NumBytes size, void *tentative_base=nullptr);
            MMap(const MemoryBlock &memory_block);
            MMap(const char* filename);
            ~MMap();
            MemoryBlock memory_block() const;
            operator bool() const;
         public:
            MemoryBlock _memory_block;
        };
        
    }
    
}
