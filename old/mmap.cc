#include "mmap.hh"

#include <stdexcept>
#include <iostream>


#include <cstddef>
#include <cstdio>

#include <unistd.h>
#include <sys/mman.h>

#include <cstdio>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>



namespace alloc {
    
    namespace memory_map {
        
    MMap::MMap(NumBytes size, void *tentative_base)
    {
        auto base = mmap(tentative_base,
                         size,
                         PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANON | MAP_NORESERVE,
                         -1,
                         0);
            
        //    std::cout << "mmap base:           " << base << std::endl;
        //    std::cout << "mmap tentative base: " << tentative_base << std::endl;
            
        if (tentative_base && (base != tentative_base)) {
            throw std::runtime_error("mmap on tentative base address failed!");
        }
            
        if (!base) {
            throw std::runtime_error("mmap failed");
        }
            
        _memory_block = MemoryBlock{ base, size };
    }

    MMap::MMap(const char* filename) {
        // try to load
        auto size = 0ULL;
        {
            auto f = std::fopen(filename,"r");
            if (!f)
                return;
            std::fseek(f, 0, SEEK_END); // seek to end of file
            size = ftell(f); // get current file pointer
            std::fclose(f);
        }
        auto fdin = open(filename, O_RDONLY);
        auto p = mmap (0, size, PROT_READ, MAP_SHARED, fdin, 0);
        _memory_block = MemoryBlock{p,size};
    }
        
    MMap::MMap(const MemoryBlock &memory_block):
        _memory_block(memory_block)
    {}

    MMap::operator bool() const { 
        return _memory_block; 
    }
 
    MemoryBlock MMap::memory_block() const
    {
        return _memory_block;
    }
        
    MMap::~MMap() {
        if (_memory_block)
            munmap(_memory_block.base(), _memory_block.size());
    }
        
    }
    
}
