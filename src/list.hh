#pragma once

namespace list {

//------------------------------------------------------------------------------
// CanonicalAdapter
//------------------------------------------------------------------------------

template <typename Item>
struct CanonicalAdapter {
public:
    using adapter_type = CanonicalAdapter<Item>;
    using item_type = Item;
public:
    // assume a field next and prev
    // on the Item class
    CanonicalAdapter(item_type* item);
    adapter_type& setNext(item_type* next);
    adapter_type& setPrev(item_type* prev);
    item_type* next() const;
    item_type* prev() const;
    item_type* operator()() const;
public:
    item_type* item;
};

//------------------------------------------------------------------------------
// CanonicalAdapter Impl.
//------------------------------------------------------------------------------

template <typename Item>
CanonicalAdapter<Item>::CanonicalAdapter(Item* item):
    item(item)
{}

template <typename Item>
auto CanonicalAdapter<Item>::setNext(Item* next) -> adapter_type& {
    item->next = next;
    return *this;
}

template <typename Item>
auto CanonicalAdapter<Item>::setPrev(Item* prev) -> adapter_type& {
    item->prev = prev;
    return *this;
}

template <typename Item>
auto CanonicalAdapter<Item>::prev() const -> item_type* {
    return item->prev;
}

template <typename Item>
auto CanonicalAdapter<Item>::next() const -> item_type* {
    return item->next;
}

template <typename Item>
auto CanonicalAdapter<Item>::operator()() const -> item_type* {
    return item;
}

//------------------------------------------------------------------------------
// List
//------------------------------------------------------------------------------

template <typename Item, typename  Adapter=CanonicalAdapter<Item>>
struct List {
public:
    using list_type = List<Item, Adapter>;
    using item_type = Item;
    using adapter_type = Adapter;
public:

    item_type* back() const;
    item_type* front() const;

    list_type& push_back(item_type *item);
    list_type& push_front(item_type *item);
    list_type& insert(item_type *item, item_type *insertion_point=nullptr);

    list_type& pop_back();
    list_type& pop_front();
    list_type& remove(item_type *item);

    std::size_t getSize() const;

public:
    std::size_t size { 0 };
    item_type* first { nullptr };
    item_type* last { nullptr };
};

//------------------------------------------------------------------------------
// List Impl.
//------------------------------------------------------------------------------

template <typename I, typename  A>
auto List<I,A>::back() const -> item_type* {
    return last;
}

template <typename I, typename  A>
auto List<I,A>::front() const -> item_type* {
    return first;
}

template <typename I, typename  A>
auto List<I,A>::push_back(item_type* item) -> list_type& {
    return insert(item, nullptr);
}

template <typename I, typename  A>
auto List<I,A>::push_front(item_type* item) -> list_type& {
    return insert(item, first);
}

template <typename I, typename  A>
auto List<I,A>::pop_back() -> list_type& {
    return remove(last);
}

template <typename I, typename  A>
auto List<I,A>::pop_front() -> list_type& {
    return remove(first);
}

template <typename I, typename  A>
auto List<I,A>::insert(item_type* item, item_type *insertion_point) -> list_type& {
    if (!first) { // empty list
        if (insertion_point)
            throw std::runtime_error("insert with an insertion_point on an empty list.");
        adapter_type u(item);
        u.setNext(nullptr).setPrev(nullptr);
        first = last = item;
    }
    else if (insertion_point) {
        // push back
        if (insertion_point != first) {
            adapter_type a_next(insertion_point);
            adapter_type a_prev(a_next.prev());
            adapter_type a_item(item);
            a_item.setPrev(a_prev()).setNext(a_next());
            a_prev.setNext(a_item());
            a_next.setPrev(a_item());
        }
        else {
            adapter_type a_first(insertion_point);
            adapter_type a_item(item);
            a_item.setPrev(nullptr).setNext(a_first());
            a_first.setPrev(a_item());
            first = a_item();
        }
    }
    else {
        adapter_type a_item(item);
        adapter_type a_last(last);
        a_item.setNext(nullptr).setPrev(last);
        a_last.setNext(item);
    }
    ++size;
    return *this;
}

template <typename I, typename  A>
auto List<I,A>::remove(item_type* item) -> list_type& {
    if (!first)
        throw std::runtime_error("cannot remove from empty list");

    if (first == item) {
        adapter_type a_first(item);
        first = a_first.next();
        a_first.setNext(nullptr);
    }
    else if (last == item) {
        adapter_type a_last(item);
        last = a_last.prev();
        a_last.setPrev(nullptr);
    }
    else {
        adapter_type a_item(item);
        adapter_type a_prev(a_item.prev());
        adapter_type a_next(a_item.next());
        a_prev.setNext(a_next());
        a_next.setPrev(a_prev());
        a_item.setPrev(nullptr).setNext(nullptr);
    }
    --size;
    return *this;
}

template <typename I, typename  A>
std::size_t List<I,A>::getSize() const {
    return size;
}

} // list namespace
