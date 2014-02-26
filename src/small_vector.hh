#pragma once

#include <iostream>
#include <cstdint>
#include <cassert>
#include <cmath>
#include <limits>
#include <vector>

#include <iterator>

#include "TaggedPointer.hh"

namespace small_vector {

using tagged_pointer::TaggedPointer;

namespace ptr {

template<typename Pointer, typename Reference, typename Container>
class iterator
{
public:

    typedef std::random_access_iterator_tag iterator_category;
    typedef typename Container::value_type value_type;
    typedef typename Container::difference_type difference_type;
    typedef Reference reference;
    typedef Pointer pointer;

    iterator(pointer ptr):
        ptr(ptr)
    {}

    reference
    operator*() const
    { return *ptr; }

    pointer
    operator->() const
    { return ptr; }

    iterator&
    operator++()
    {
        ++ptr;
        return *this;
    }

    iterator
    operator++(int)
    { return iterator(ptr++); }


    iterator&
    operator--()
    {
        --ptr;
        return *this;
    }

    iterator
    operator--(int)
    { return iterator(ptr--); }


    reference
    operator[](const difference_type& __n) const
    { return ptr[__n]; }

    iterator&
    operator+=(const difference_type& __n)
    { ptr += __n; return *this; }

    iterator
    operator+(const difference_type& __n) const
    { return iterator(ptr + __n); }

    iterator&
    operator-=(const difference_type& __n)
    { ptr -= __n; return *this; }

    iterator
    operator-(const difference_type& __n) const
    { return iterator(ptr - __n); }

    const pointer
    base() const
    { return ptr; }

protected:

    pointer ptr;

};

template<typename Pointer, typename Reference, typename _Container>
inline bool operator==(const iterator<Pointer, Reference, _Container>& __lhs,
                       const iterator<Pointer, Reference, _Container>& __rhs)
{ return __lhs.base() == __rhs.base(); }

template<typename Pointer, typename Reference, typename _Container>
inline bool operator!=(const iterator<Pointer, Reference, _Container>& __lhs,
                       const iterator<Pointer, Reference, _Container>& __rhs)
{ return __lhs.base() != __rhs.base(); }

template<typename Pointer, typename Reference, typename _Container>
inline typename iterator<Pointer, Reference, _Container>::difference_type
operator-(const iterator<Pointer, Reference, _Container>& __lhs,
          const iterator<Pointer, Reference, _Container>& __rhs)
{ return __lhs.base() - __rhs.base(); }

}


template <typename T>
struct small_vector
{
    typedef small_vector type;

    typedef uint16_t actual_size_type;

    typedef uint64_t size_type;
    typedef int64_t  difference_type;

    typedef T value_type;
    typedef value_type* pointer;
    typedef const value_type* const_pointer;
    typedef value_type& reference;
    typedef const value_type& const_reference;

    typedef ptr::iterator<pointer, reference, type> iterator;
    typedef ptr::iterator<const_pointer, const_reference, type> const_iterator;
    typedef std::reverse_iterator<iterator> reverse_iterator;

//    typedef typename std::vector<T>::const_iterator const_iterator;
//    typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

    small_vector():
        data(0, 0)
    {}

    ~small_vector()
    {
      //std::cout << "delete small_vector" << std::endl;
        if (this->size() > 0)
            delete [] data.getPointer();
    }

    void assert_capacity(size_type c)
    {
        if (c > capacity())
        {
            size_type s = size();

            size_type new_capacity = capacityFor(c);

            T* buffer = data.getPointer();
            T* new_buffer = new T[new_capacity];

            std::copy(buffer, buffer + s, new_buffer);

            if (buffer)
                delete [] buffer;

            data.setPointer(new_buffer);
        }
    }

    void resize(size_type s)
    {
        assert_capacity(s);
        this->data.setTag(s);
    }

    void push_back(const T& obj)
    {
        size_type size = this->size();
        // size_type capacity = this->capacity();

        // assert (size < std::numeric_limits<actual_size_type>::max()-1);

        resize(size+1); // unitialized resize
        (*this)[size] = obj;
    }

    reference operator[](size_type index)
    {
        T *ptr = data.getPointer() + index;
        return *ptr;
    }

    const reference operator[](size_type index) const
    {
        T *ptr = data.getPointer() + index;
        return *ptr;
    }

    reference back()
    {
        return *rbegin();
    }

    reference front()
    {
        return *begin();
    }

    inline size_type capacityFor(size_type s) const
    {
        if (s <= 2)
            return s;
        else { // inefficient, but sufficient for now
            return static_cast<size_type>(pow(2,ceil(log2(static_cast<double>(s)))));
        }
    }

    size_type capacity() const
    {
        return capacityFor(size());
    }

    size_type size() const
    {
        return data.getTag();
    }

    //
    // iterators
    //

    iterator begin()
    { return iterator(data.getPointer()); }

    iterator end() noexcept
    { return iterator(data.getPointer() + size()); }

    const_iterator begin() const
    { return const_iterator(data.getPointer()); }

    const_iterator end() const
    { return const_iterator(data.getPointer() + size()); }


    reverse_iterator rbegin()
    { return reverse_iterator(end()); }

    reverse_iterator rend()
    { return reverse_iterator(begin()); }


    iterator insert(iterator __position, const value_type& __x)
    {
        difference_type index = __position - begin();

        difference_type move_block_size = size() - index;

        resize(size() + 1);

        // std::cout << move_block_size << std::endl;

        //rbegin() + move_block_size
        std::copy(rbegin() + 1,
                  rbegin() + 1 + move_block_size,
                  rbegin());
        (*this)[index] = __x;


        //this->data.setTag(size()+1);

        // dump(std::clog);


        return iterator(begin() + index);

        // assert_capacity(size() + 1);
        // if (size())

    }

//    iterator insert(iterator __position, value_type&& __x)
//    { return emplace(__position, std::move(__x)); }

    void dump(std::ostream &os)
    {
        os << "[small_vector, size: " << size() << "]: ";
        for (size_type i=0;i<size();i++)
            os << (*this)[i] << " ";
        os << std::endl;
    }

    // tag will be the current size of the list
    TaggedPointer<T> data;
};


}
