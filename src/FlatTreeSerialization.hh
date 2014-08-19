#pragma once

#include <FlatTree.hh>

#include <cassert>

#include <sstream>

#include <serialization/BasicTypes.hpp>
#include <serialization/CoreSchema.hpp>
#include <serialization/Deserializer.hpp>
#include <serialization/FieldType.hpp>
#include <serialization/Schema.hpp>
#include <serialization/Schema.hpp>
#include <serialization/SchemaLog.hpp>
#include <serialization/SchemaSerialization.hpp>
#include <serialization/Serializer.hpp>
#include <serialization/Type.hpp>
#include <serialization/Util.hpp>

namespace serialization
{

using namespace flattree;
using namespace serialization::util;

template <typename Content>
Schema &operator<<(Schema &schema, flattree::FlatTree<Content> &)
{
    typedef flattree::FlatTree<Content> FTree;
    typedef flattree::Link<Content>     FTreeLink;

    // create new custom type
    serialization::CustomType *type =
            serialization::retrieveOrCreateCustomType<FTree>(schema, getTypeName<FTree>());

    // assert it is an empty custom type
    if (!(type->getNumFields() == 0))
        throw std::runtime_error("Invalid Path Size");

    { // tagged pointer
        type->addField(serialization::FieldType(CoreSchema::ID_tagged_pointer, FieldType::TAGGED_POINTER), "content");
    }

    { // links
        CustomType *field_type =
            serialization::retrieveOrCreateCustomType<FTreeLink>(schema, getTypeName<FTreeLink>());
        type->addField(serialization::FieldType(field_type, FieldType::VALUE, FieldType::REPEAT_16), "links");

    }
    return schema;
}

template <typename Content>
Schema &operator<<(Schema &schema, flattree::Link<Content> &)
{
    typedef flattree::Link<Content>     FTreeLink;

    // create new custom type
    serialization::CustomType *type =
            serialization::retrieveOrCreateCustomType<FTreeLink>(schema, getTypeName<FTreeLink>());

    // assert it is an empty custom type
    assert(type->getNumFields() == 0);

    { // tagged pointer
        type->addField(serialization::FieldType(serialization::CoreSchema::ID_tagged_pointer,
                                                serialization::FieldType::TAGGED_POINTER), "content");
    }

    { // links
        type->addField(serialization::FieldType(serialization::CoreSchema::ID_uint8), "label");
    }
    return schema;
}


template <typename Content>
Serializer &operator<<(Serializer &s, flattree::FlatTree<Content> &ft)
{
    //
    typedef serialization::TaggedPointer<Content> TagPtrContent;

    // recast custom tagged pointer as serialization tagged pointer
    // assuming the client know what he is doing.
    const TagPtrContent &tagged_pointer = reinterpret_cast<const TagPtrContent&>(ft.data);

    s << tagged_pointer; // save as a pointer for now

    // serialize list of links
    uint16 size = ft.links.size();
    s << size; // save as a pointer for now
    for (uint16 i=0;i<size;i++)
        s << ft.links[i];

    return s;
}

template <typename Content>
Serializer &operator<<(Serializer &s, flattree::Link<Content> &link)
{
    //m
    typedef serialization::TaggedPointer<Content> TagPtrContent;

    // recast custom tagged pointer as serialization tagged pointer
    // assuming the client know what he is doing.
    const TagPtrContent &tagged_pointer =
            reinterpret_cast<const TagPtrContent&>(link.data);

    s << tagged_pointer; // save as a pointer for now

    // now serialie the PathElement
    uint8 label = link.label;
    s << label;

    //
    return s;
}

template <typename Content>
Deserializer &operator>>(Deserializer &d, flattree::FlatTree<Content> &ft)
{
    //m
    typedef serialization::TaggedPointer<Content>    TagPtrContent;
    typedef serialization::TaggedPointerRef<Content> TagPtrContentRef;

    TagPtrContent    &tagged_pointer = reinterpret_cast<TagPtrContent&>(ft.data);
    TagPtrContentRef  tagged_pointer_ref(tagged_pointer);

    d >> tagged_pointer_ref;

    // deserialize list of links
    uint16 size;
    d >> size;
    ft.links.resize(size);
    for (uint16 i=0;i<size;i++)
        d >> ft.links[i];

    //
    return d;
}


template <typename Content>
Deserializer &operator>>(Deserializer &d, flattree::Link<Content> &link)
{
    typedef serialization::TaggedPointer<Content>    TagPtrContent;
    typedef serialization::TaggedPointerRef<Content> TagPtrContentRef;

    TagPtrContent    &tagged_pointer = reinterpret_cast<TagPtrContent&>(link.data);
    TagPtrContentRef  tagged_pointer_ref(tagged_pointer);

    d >> tagged_pointer_ref;

    // now serialie the PathElement
    uint8 label;
    d >> label;

    link.label = label;

    //
    return d;
}

template <typename Content>
void registerFlatTreeObjectFactories(Schema &schema, Deserializer &deserializer)
{
    addObjectFactory<FlatTree<Content>>(schema, deserializer);
    addObjectFactory<flattree::Link<Content>>(schema, deserializer);
}

template <typename Content>
void registerFlatTreeToSchema(Schema &schema)
{
    flattree::FlatTree<Content> ft;
    flattree::Link<Content>     link;
    schema << ft << link;
}

}



