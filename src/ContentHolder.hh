#pragma once

#include "TaggedPointer.hh"

namespace contentholder {

using tagged_pointer::TaggedPointer;

typedef uint16_t UserData; // the first seven bits of CustomData
typedef uint16_t Flag; // the first seven bits of CustomData

//-----------------------------------------------------------------------------
// UserDataAndFlag
//-----------------------------------------------------------------------------

struct UserDataAndFlag
{
    UserDataAndFlag(uint16_t tag)
    {
        proper_content = (tag & 0x8000) ? 1 : 0;
        user_data      = tag & 0x7FFF;
    }
    uint16_t tag()
    {
        return (proper_content << 15) | user_data;
    }
    uint16_t proper_content: 1;
    uint16_t user_data: 15;
};

//-----------------------------------------------------------------------------
// ContentHolder
//-----------------------------------------------------------------------------

template <typename Content>
struct ContentHolder
{
    ContentHolder();

    void        setProperContent(Content *content);

    void        setSharedContent(Content *content);

    Content*    getContent() const;

    Content*    getProperContentCreateIfNeeded();

    bool        contentIsProper() const;

    bool        contentIsShared() const;

    void        copyContentAndProperFlag(const ContentHolder& c);

private:

    UserDataAndFlag userDataAndFlag() const;

public: // reuse the 7 bits of the flag byte indicating that
        // content is proper.

    void        setUserData(UserData d);

    inline UserData getUserData() const;

public:

    TaggedPointer<Content> data;

//    Content*         content;
//    Flag             proper_content: 1;
//    UserData         user_data     : 7;
};

//UserDataAndFlag tag2udf(Tag tag)
//{
//}

//
// Content Holder
//

//template <typename Content>
//UserDataAndFlag ContentHolder<Content>::userDataAndFlag() const
//{
//    return UserDataAndFlag(data.getTag());
//}

template <typename Content>
ContentHolder<Content>::ContentHolder()
{}

template <typename Content>
bool ContentHolder<Content>::contentIsProper() const
{
    return UserDataAndFlag(data.getTag()).proper_content == 1;
}

template <typename Content>
bool ContentHolder<Content>::contentIsShared() const
{
    return UserDataAndFlag(data.getTag()).proper_content == 0;
}

template <typename Content>
void ContentHolder<Content>::copyContentAndProperFlag(const ContentHolder &c)
{
    data.setPointer(c.data.getPointer());
    UserDataAndFlag udf(data.getTag());
    UserDataAndFlag cudf(c.data.getTag());
    udf.proper_content = cudf.proper_content;
    data.setTag(udf.tag());

//    this->content = c.content;
//    this->proper_content = c.proper_content;
}

template <typename Content>
Content* ContentHolder<Content>::getContent() const
{
    return data.getPointer();
}

template <typename Content>
void ContentHolder<Content>::setProperContent(Content *content)
{
    data.setPointer(content);
    UserDataAndFlag udf(data.getTag());
    udf.proper_content = 1;
    data.setTag(udf.tag());

    //this->content = content;
    //this->proper_content = 1;
}

template <typename Content>
void ContentHolder<Content>::setSharedContent(Content *content)
{
    data.setPointer(content);
    UserDataAndFlag udf(data.getTag());
    udf.proper_content = 0;
    data.setTag(udf.tag());

    // this->content = content;
    // this->proper_content = 0;
}

template <typename Content>
Content* ContentHolder<Content>::getProperContentCreateIfNeeded()
{
    if (!data.getPointer())
    {
        //
        this->setProperContent(new Content());

        // std::cout << "new content: " << ((uint64_t) this->getContent()) % 100001L << std::endl;

        // std::cout << "getProperContentCreateIfNeeded() -> new content " << data.getPointer() << std::endl;
    }
    else if (!contentIsProper())
    {
        // Content *shared_content = data.getPointer();
        // std::cout << "lazy copying content: " << ((uint64_t) shared_content) % 100001L << std::endl;

        // int shared_count = shared_content->entries.back().count;
        this->setProperContent(data.getPointer()->makeLazyCopy());
        // int this_count = shared_content->entries.back().count;

//        std::cout << "getProperContentCreateIfNeeded() -> content lazy copied from "
//                  << shared_content << " to " << data.getPointer()  << std::endl;
    }
    else {
//        std::cout << "getProperContentCreateIfNeeded() -> content was already proper" << std::endl;
    }
    return data.getPointer();
}

template <typename Content>
void ContentHolder<Content>::setUserData(UserData d)
{
    UserDataAndFlag udf(data.getTag());
    udf.user_data = d;
    data.setTag(udf.tag());

    // user_data = d;
}

template <typename Content>
inline UserData ContentHolder<Content>::getUserData() const
{
    // apparently this constructor wasn't getting optimized away...
    // at least I see a 3% difference in overall runtime when replacing
    // this call by the other one.

    return data.getTag() & 0x7FFF;
    // return UserDataAndFlag(data.getTag()).user_data;
    // return user_data;
}

}
