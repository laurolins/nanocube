#pragma once

#include <cstdint>

namespace tagged_pointer {

template <typename T>
struct TaggedPointer
{
    typedef uint16_t Tag;
    typedef uint64_t UInt64;

    //
    static const UInt64 bit47 = (1UL << 47);

    TaggedPointer()
    {
        data.ptr = 0;
    }

    TaggedPointer(T *ptr, Tag tag)
    {
        data.ptr = static_cast<void*>(ptr);
        assert (data.aux.tag == 0 || data.aux.tag == 0xFF);
        data.aux.tag = tag;
    }

    inline T* operator()()
    {
        return getPointer();
    }

    inline T* getPointer() const
    {
        Data x = data;
        if (reinterpret_cast<UInt64>(data.ptr) & bit47)
            x.aux.tag = 0xFFFF;
        else
            x.aux.tag = 0;
        return static_cast<T*>(x.ptr);
    }

    inline void setPointer(T* ptr)
    {
        Tag tag = getTag();
        data.ptr = static_cast<void*>(ptr);
        assert (data.aux.tag == 0 || data.aux.tag == 0xFF);
        data.aux.tag = tag;
    }

    inline void setTag(Tag tag)
    {
        data.aux.tag = tag;
    }

    inline Tag getTag() const
    {
        return data.aux.tag;
    }

    struct Aux{
        Tag pad1;
        Tag pad2;
        Tag pad3;
        Tag tag;
    };

    union Data {
        void  *ptr;
        Aux    aux;
    } data;

};

}
