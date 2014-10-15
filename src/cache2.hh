#pragma once

#include <unordered_map>

#include "list.hh"

namespace cache2 {

//------------------------------------------------------------------------------
// Forward declarations
//------------------------------------------------------------------------------

template<typename Key, typename Object>
struct Item;

// template<typename Key, typename Object>
// struct Cache;

//------------------------------------------------------------------------------
// KeyWrapper
//------------------------------------------------------------------------------

template <typename Key>
struct KeyWrapper {
public:
    using key_type = Key;
    using key_wrapper_type = KeyWrapper<key_type>;

public:
    KeyWrapper() = default;
    KeyWrapper(const key_type *key_value);
public:
    const key_type *key { nullptr };
};

//------------------------------------------------------------------------------
// KeyWrapperHash Impl.
//------------------------------------------------------------------------------

template <typename Key, typename KeyHash = std::hash<Key>>
struct KeyWrapperHash {
public:
    using key_type = Key;
    using key_wrapper_type = KeyWrapper<key_type>;
    using key_hash_type = KeyHash;
public:
    std::size_t operator()(const key_wrapper_type &key_wrapper) const {
        if (key_wrapper.key) {
            return key_hash_type()(*key_wrapper.key);
        }
        else {
            throw std::runtime_error("hash of empty keywrapper is not defined");
        }
    }
};

//------------------------------------------------------------------------------
// KeyWrapperEqual Impl.
//------------------------------------------------------------------------------

template <typename Key, typename KeyEqual = std::equal_to<Key>>
struct KeyWrapperEqual {
public:
    using key_type = Key;
    using key_wrapper_type = KeyWrapper<key_type>;
    using key_equal_type = KeyEqual;
public:
    bool operator()(const key_wrapper_type &a,
                    const key_wrapper_type &b) const {
        if (a.key && b.key) {
            return key_equal_type()(*a.key, *b.key);
        }
        else {
            throw std::runtime_error("hash of empty keywrapper is not defined");
        }
    }
};


//------------------------------------------------------------------------------
// KeyWrapper Impl.
//------------------------------------------------------------------------------

template <typename Key>
KeyWrapper<Key>::KeyWrapper(const key_type *key):
    key(key)
{}
 
//------------------------------------------------------------------------------
// Item
//------------------------------------------------------------------------------

template <typename Key, typename Object>
struct Item {
public:
    using key_type = Key;
    using key_wrapper_type = KeyWrapper<key_type>;
    using object_type = Object;
    using item_type = Item<key_type, object_type>;
public:
    Item() = default;
    Item(const key_type &key, object_type* object);
    ~Item();
public:
    key_wrapper_type key_wrapper;
    object_type *object { nullptr };
    Item* next { nullptr };
    Item* prev { nullptr };
};

//------------------------------------------------------------------------------
// Item Impl.
//------------------------------------------------------------------------------

template <typename Key, typename Object>
Item<Key, Object>::Item(const key_type &key, object_type* object) {
    this->object = object;
    this->key_wrapper = key_wrapper_type(new key_type(key)); // dynamically allocate copy of key
}

template <typename Key, typename Object>
Item<Key, Object>::~Item() {
    delete key_wrapper.key; // key object is owned by item
    this->object = nullptr;
}


//     std::size_t hash() const;
// template <typename Key>
// std::size_t KeyWrapper<Key>::hash() const {
//     if (key_value) {
//         return std::hash(*key_value); // only std object for now
//     }
//     else {
//         return 0; // is this ok?
//     }
// }

// template <typename Key>
// bool KeyWrapper<Key>::equals(const KeyWrapper &other) const {
//     if (key_value) {
//         return std::equal_to(*key_value, other.key_value); // only std object for now
//     }
//     else {
//         return false; // is this ok?
//     }
// }


//------------------------------------------------------------------------------
// Cache
//------------------------------------------------------------------------------

template <typename Key, typename Object, typename KeyHash = std::hash<Key>, typename KeyEqual = std::equal_to<Key>> 
struct Cache {
public:
    using key_type = Key;
    using key_hash_type = KeyHash;
    using key_equal_type = KeyEqual;
    using object_type = Object;
    using key_wrapper_type = KeyWrapper<key_type>;
    using cache_type = Cache<key_type, object_type>;
    using item_type = Item<key_type, object_type>;
    using item_list_type = list::List<item_type>;
    using key_wrapper_hash_type = KeyWrapperHash<Key, KeyHash>;
    using key_wrapper_equal_type = KeyWrapperEqual<Key, KeyEqual>;
public:
    Cache(std::size_t budget=0);
    ~Cache();

    cache_type& insert(const std::string& key, object_type* object);
    object_type* operator[](const std::string& key);

    cache_type& enforce_budget(); // if budget > 0, keep only the "budget" that were most
                                  // recently used.
    
    void setBudget(std::size_t budget);
    
    std::size_t size() const;

public:
    std::size_t budget { 0 }; // number of items
    std::unordered_map<key_wrapper_type, item_type*, key_wrapper_hash_type, key_wrapper_equal_type> items;
    item_list_type mru_list; // most recently used items are in the front
                             // least recently used are in the back
};

template <typename Key, typename Object, typename KeyHash, typename EqualHash>
Cache<Key,Object,KeyHash,EqualHash>::Cache(std::size_t budget):
    budget(budget)
{}

template <typename Key, typename Object, typename KeyHash, typename EqualHash>
void Cache<Key,Object,KeyHash,EqualHash>::setBudget(std::size_t budget)
{
    this->budget = budget;
}

    template <typename Key, typename Object, typename KeyHash, typename EqualHash>
    std::size_t Cache<Key,Object,KeyHash,EqualHash>::size() const
    {
        return mru_list.getSize();
    }

    
template <typename Key, typename Object, typename KeyHash, typename EqualHash>
Cache<Key,Object,KeyHash,EqualHash>::~Cache()
{
    items.clear();
    while (mru_list.getSize()) {
        auto item = mru_list.back();

        // delete from Most Recently Used list
        mru_list.remove(item);

        // delete object from mru_list (this can be a lot of memory savings
        // depending on the kind of object)
        delete item->object;

        // delete item (the underlying key is also erased)
        delete item;
    }
}

template <typename Key, typename Object, typename KeyHash, typename EqualHash>
auto Cache<Key,Object,KeyHash,EqualHash>::insert(const std::string& key, object_type* object) -> cache_type& {
    // assume object doesn't exist yet
    auto new_item = new item_type(key, object); // a copy of the key is created

    key_wrapper_type key_wrapper(new_item->key_wrapper.key);
    items[key_wrapper] = new_item; // keys in unsorted map are lightweigh objects

    mru_list.push_front(new_item);
    return *this;
}

template <typename Key, typename Object, typename KeyHash, typename EqualHash>
auto Cache<Key,Object,KeyHash,EqualHash>::operator[](const std::string& key) -> object_type* {
    auto it = items.find(key_wrapper_type(&key));
    if (it == items.end())
        return nullptr;
    else {
        auto item = it->second;
        mru_list.remove(item);
        mru_list.push_front(item);
        return item->object;
    }
}

template <typename Key, typename Object, typename KeyHash, typename EqualHash>
auto Cache<Key,Object,KeyHash,EqualHash>::enforce_budget() -> cache_type& {
    while (mru_list.getSize() > budget) {
        auto item = mru_list.back();

        // delete from Most Recently Used list
        mru_list.remove(item);

        // delete from unordered_map
        items.erase(item->key_wrapper);

        // delete object from mru_list (this can be a lot of memory savings
        // depending on the kind of object)
        delete item->object;

        // delete item (the underlying key is also erased)
        delete item;
    }
    return *this;
}



} // cache2
