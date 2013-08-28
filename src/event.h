#pragma once

#include <map>
#include <vector>
#include <boost/unordered_map.hpp>

struct EventKey
{
    unsigned char x, y;
};

inline bool operator==(const EventKey &k1, const EventKey &k2)
{
    return k1.x == k2.x && k1.y == k2.y;
}

inline size_t hash_value(const EventKey &k)
{
    return k.x + (((size_t) k.y) << 8);
}


inline bool operator<(const EventKey &e1, const EventKey &e2)
{
    if (e1.x < e2.x) return true;
    if (e1.x > e2.x) return false;
    if (e1.y < e2.y) return true;
    if (e1.y > e2.y) return false;
    return false;
}

struct EventSet
{
    boost::unordered_map<EventKey, size_t> index;
    std::vector<EventKey> keys;
    std::vector<unsigned short> counts;
    int num_event_types;
    
    EventSet() {};
    explicit EventSet(int n): num_event_types(n) {};

    void add(const EventKey &k, int event_type) {
        boost::unordered_map<EventKey, size_t>::iterator f = index.find(k);
        size_t v;
        if (f == index.end()) {
            v = keys.size();
            index[k] = v;
            keys.push_back(k);
            for (size_t i=0; i<num_event_types; ++i) {
                counts.push_back(0);
            }
        } else {
            v = f->second;
        }
        ++counts[v * num_event_types + event_type];
    };

    int tile_length() const {
        return keys.size() * (sizeof(EventKey) + sizeof(unsigned short));
    }

    // kinda dangerous, but I don't know how to do this better with mongoose's API
    void fill_buffer(char *v) {
        memcpy(v, &keys[0], keys.size() * sizeof(EventKey));
        memcpy(v + (keys.size() * sizeof(EventKey)), &counts[0], counts.size() * sizeof(unsigned short));
    }
};
