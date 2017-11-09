#include "bitvector.hh"

#include <algorithm>
#include <iomanip>
#include <cassert>

namespace bitvector {

    void BitVector::resize(const ChunksOf64Bits& chunks) {
        if (chunks.count() > 0) {
            // make sure capacity is a multiple of 64-bits
            auto new_byte_capacity = chunks.count() * 8;
            char *new_data = new char[new_byte_capacity];
            auto n = std::min(_capacity, new_byte_capacity);
            std::copy(_data, _data + n, new_data);
            std::fill(new_data + n, new_data + new_byte_capacity, 0);
            delete [] _data;
            _data = new_data;
            _capacity = new_byte_capacity;
        }
        else {
            delete [] _data;
            _data     = nullptr;
            _capacity = 0;
            return;
        }
    }


    bool BitVector::write(NumBits bit_offset, NumBits bit_length, const char* input) {
        if (bit_offset + bit_length >= bit_capacity())
            return false;

        // align a byte 64 bit unsigned int on the 
        std::uint64_t* dst        = (std::uint64_t*) (_data + (bit_offset / 8)); 
        std::uint64_t  dst_bitoff = bit_offset & 0xF; // which bit offset should I start writing on

        //
        // if dst_bitoff == 3 then
        //      mask_left  == 1110000...000000 (64 bits)
        //      mask_right == 0001111...111111 (64 bits)
        //
        auto mask_right = ~0ULL << dst_bitoff;   // erase 3 least significant bits and negate
        auto mask_left  = ~mask_right;

        const std::uint64_t *src = (const std::uint64_t*) input;

        //
        // consume the bits in data a chunk of 64 at a time
        // they are all byte aligned
        // when less than 64 bits are available write them
        //
        auto remaining_bits = bit_length;
        while (remaining_bits > 0) {
            
            if (remaining_bits < 64) {

                // copy last bytes from src into a local 64 bit storage space
                std::uint64_t src_copy = 0;
                auto n = (remaining_bits / 8) + ((bit_length % 8) ? 1 : 0);
                std::copy((char*) src, (char*) src + n, (char*) &src_copy);

                //
                // ones on the extremes 1111 000000000 1111
                // # zeros in the middle is equal to remaining_bits
                //
                auto mask_end = mask_left | ~(~0ULL >> (64 - remaining_bits - dst_bitoff));
                
                *dst = (*dst & mask_end) | (src_copy << dst_bitoff);

                remaining_bits = 0;
            }
            else {
                // assuming we still need to write 64 or more bits
                // from src to dst

                // write lower piece of the data into dst
                *dst = (*dst & mask_left) | (*src << dst_bitoff);

                ++dst; // done writing on this 64-bit block. go to next

                // is there a right part of the data
                if (dst_bitoff) {

                    //
                    // we still have to write dst_bitoff bits in the beginning of
                    // this new 64 bit block on dst
                    //
                
                    *dst = (*dst & mask_right) | (*src >> (64 - dst_bitoff));
                }
                remaining_bits -= 64;
            }
        }
        
        return true;

    }

    bool BitVector::read(NumBits bit_offset, NumBits bit_length, char* output) {

        if (bit_offset + bit_length >= bit_capacity())
            return false;

        // align a byte 64 bit unsigned int on the 
        std::uint64_t* src        = (std::uint64_t*) (_data + (bit_offset / 8)); 
        int            src_bitoff = bit_offset & 0xF; // which bit offset should I start writing on

        //
        // if src_bitoff == 3 then
        //      mask_left  == 1110000...000000 (64 bits)
        //      mask_right == 0001111...111111 (64 bits)
        //
        auto mask_right = ~0ULL << src_bitoff;   // erase 3 least significant bits and negate
        auto mask_left  = ~mask_right;

        std::uint64_t *dst = (std::uint64_t*) output;

        auto remaining_bits = bit_length;
        while (remaining_bits > 0) {
            if (remaining_bits < 64) {
                // copy last bytes from src into a local 64 bit storage space
                std::uint64_t src_copy = 0;
                auto n = (remaining_bits / 8) + ((bit_length % 8) ? 1 : 0);
                std::copy((char*) src, (char*) src + n, (char*) &src_copy);
                auto mask_end = ~(mask_left | ~(~0ULL >> (64 - remaining_bits - src_bitoff)));
                src_copy &= mask_end; // shift bit offset (<8)
                src_copy >>= src_bitoff;
                auto p = (char*) &src_copy;
                std::copy(p, p + n, (char*) dst); 
                remaining_bits = 0;
            }
            else {
                *dst = src_bitoff ? (*src >> src_bitoff) | ( (*(src+1) & mask_left) << (64 - src_bitoff) ) : *src;
                ++src;
                ++dst; // done writing on this 64-bit block. go to next
                remaining_bits -= 64;
            }
        }
        
        return true;
    }

    void BitVector::swap(NumBits offset_a, NumBits offset_b, NumBits length) {

        // check limits
        if (offset_a > offset_b) std::swap(offset_a, offset_b);

        assert(offset_b - offset_a >= length);

        NumBits remaining = length;
        NumBits offset    = 0;
        while (remaining) {
            uint64_t a = 0;
            uint64_t b = 0;
            auto chunk = std::min(64ull, remaining);

            read(offset_a + offset, chunk, (char*) &a);
            read(offset_b + offset, chunk, (char*) &b);

            write(offset_a + offset, chunk, (char*) &b);
            write(offset_b + offset, chunk, (char*) &a);

            offset += chunk;
            remaining -= chunk;
        }
    }

    void print_bitvector(std::ostream& os, const BitVector& bv, int bytes_per_line) {
        for (auto i=0;i<bv._capacity;++i) {
            auto  v = (int) *((unsigned char*) (bv._data + i));
            // std::cout << "[" << vv << "]" << std::endl;
            os << std::hex << std::setfill('0') << std::setw(2) << v << " ";
            if ((i % bytes_per_line) == bytes_per_line-1) {
                os << std::endl;
            }
        }
        if ((bv._capacity % bytes_per_line) != 0)
            os << std::endl;
    };

} // end bitvector namespace