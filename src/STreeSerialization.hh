#pragma once

#include <STree.hh>

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

using namespace stree;

using namespace serialization::util;

template <typename TypeList, typename Data>
Schema &operator<<(Schema &schema, STree<TypeList, Data> &)
{
    typedef STree<TypeList, Data>  STreeType;
    typedef typename STreeType::FrontType FrontType;

    // create new custom type
    serialization::CustomType *type =
            serialization::retrieveOrCreateCustomType<STreeType>(schema, getTypeName<STreeType>());

    // assert it is an empty custom type
    assert(type->getNumFields() == 0);

    { // links
        CustomType *field_type =
            serialization::retrieveOrCreateCustomType<FrontType>(schema, getTypeName<FrontType>());

        type->addField(FieldType(field_type, FieldType::VALUE), "root");

    }
    return schema;
}

template <typename TypeList, typename Data>
Serializer &operator<<(Serializer &s, STree<TypeList, Data> &stree)
{
    s << stree.root;
    return s;
}

template <typename TypeList, typename Data>
Deserializer &operator>>(Deserializer &d, STree<TypeList, Data> &stree)
{
    d >> stree.root;
    return d;
}

template <typename TypeList, typename Data>
void registerSTreeObjectFactories(Schema &schema, Deserializer &deserializer)
{
    serialization::util::addObjectFactory<STree<TypeList, Data>>(schema, deserializer);
}

template <typename TypeList, typename Data>
void registerSTreeToSchema(Schema &schema)
{
    STree<TypeList, Data> stree;
    schema << stree;
}

}



