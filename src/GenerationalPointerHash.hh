#pragma once
/******************************************************************************

THIS IS NOT CURRENTLY BEING USED

I'm keeping it in the repo because I might have done something slow
and stupid; maybe there's a way to fix it.

******************************************************************************/
#include <vector>
#include <utility>
#include <algorithm>

/******************************************************************************

Implements a linear-chaining hash table that can be cleared
generationally.  In other words, clear() takes O(1) time. This is
achieved by associating each sequence of additions, deletions and
lookups with a *generation*. Each slot in the table has a generation
number, and the table itself stores the current generation. Slots are
considered empty if the generation doesn't match. Clears are achieved
by simply bumping the generation number.

This is a useful feature when hash tables need to be created and
destroyed frequently enough that allocation becomes the bottleneck.

*******************************************************************************/

struct GenerationalPointerHashTable
{
    typedef void *Value;
    typedef std::pair<unsigned int, Value> TableSlot;

    GenerationalPointerHashTable(int initial_table_size=256) {
        m_table = std::vector<TableSlot>(initial_table_size, std::make_pair(0, nullptr));
        m_generation = 1;
    }

    inline uint32_t hash(Value k) const {
        return uint32_t((((long long) k) >> 3)) * uint32_t(2654435761);
        // // this is the FNV hash
        // unsigned int h = 2166136261;
        // for (int i=0; i<sizeof(Value); ++i) {
        //     h = (h * 16777619) ^ ((((long long) k) >> (8 * i)) & 255);
        // }
        // return h;
    }

    void expand() {
        std::vector<TableSlot> old_table = m_table;
        m_table = std::vector<TableSlot>(m_table.size() * 2, std::make_pair(m_generation+1, nullptr));
        m_size = 0;
        m_generation++;

        for (size_t i=0; i<old_table.size(); ++i) {
            if (old_table[i].first == m_generation-1)
                insert(old_table[i].second);
        }
    }

    void clear() {
        m_generation += 1;
        if (m_generation == (1 << 31)) {
            m_table = std::vector<TableSlot>(m_table.size(), std::make_pair(0, nullptr));
            m_generation = 1;
        }
        m_size = 0;
    }

    inline void insert(Value val) {
        unsigned int table_size = m_table.size();
        // max load factor 0.25 by default
        if (m_size * 4 > table_size) {
            expand();
            table_size = m_table.size();
        }
        unsigned int slot = hash(val) % table_size;
        while (m_table[slot].first == m_generation && 
               m_table[slot].second != nullptr)
            slot = (slot + 1) % table_size;
        m_table[slot] = std::make_pair(m_generation, val);
        m_size += 1;
    }

    bool find(Value val) {
        unsigned int table_size = m_table.size();
        unsigned int slot = hash(val) % table_size;
        int count=0;
        while (m_table[slot].first == m_generation &&
               m_table[slot].second != nullptr) {
            if (m_table[slot].second == val) {
                return true;
            }
            slot = (slot + 1) % table_size;
            count++;
        }
        return false;
    }

    void remove(Value val) {
        unsigned int table_size = m_table.size();
        unsigned int slot = hash(val) % table_size;
        while (m_table[slot].first == m_generation &&
               m_table[slot].second != nullptr) {
            if (m_table[slot].second == val) {
                m_table[slot].second = nullptr;
                return;
            }
            slot = (slot + 1) % table_size;
        }
        m_size -= 1;
    }

    std::vector<TableSlot> m_table;
    unsigned int m_generation;
    unsigned int m_size;
};
