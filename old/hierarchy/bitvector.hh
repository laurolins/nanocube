#pragma once

#include <cstdint>
#include <iostream>

namespace bitvector {

    using NumBits  = std::uint64_t;
    using NumBytes = std::uint64_t;

    // multiple of 64 bits
    struct ChunksOf64Bits {
        ChunksOf64Bits() = default;
        ChunksOf64Bits(std::uint64_t count): _count(count){};
        ChunksOf64Bits operator *(double scale) { return ChunksOf64Bits((std::uint64_t) (_count * scale)); }
        std::uint64_t count() const { return _count; }
        std::uint64_t _count { 0 };
    };

    //
    // raw bit vector: capacity grows in multiples of 8 bytes
    //

    struct BitVector {
        
        BitVector() = default;
        ~BitVector() { delete [] _data; }

        bool write(NumBits bit_offset, NumBits bit_length, const char* input);

        bool read(NumBits bit_offset, NumBits bit_length, char* output);

        // preserve all the bits that are available
        void     resize(const ChunksOf64Bits& chunks);

        NumBits  bit_capacity() const { return _capacity * 8; }

        void swap(NumBits offset_a, NumBits offset_b, NumBits length);

        ChunksOf64Bits  chunk_capacity() const { return ChunksOf64Bits(_capacity/8); }

        std::uint64_t _capacity { 0 }; // capacity in bytes
        char*         _data     { nullptr };

    };

    void print_bitvector(std::ostream& os, const BitVector& bv, int bytes_per_line=8);

} // end bitvector namespace

