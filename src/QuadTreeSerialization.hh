#pragma once

#include "QuadTree.hh"

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

#include <cxxabi.h>

//namespace quadtree
//{

namespace serialization
{

using namespace ::serialization;
using namespace ::serialization::util;

// using serialization::util;

//------------------------------------------------------------------------------
// serializeNodePointer (find type and reinterpret_cast to the right type)
//------------------------------------------------------------------------------

template <typename N, typename Content>
inline void snp(Serializer &serializer, quadtree::Node<Content> *node)
{
    typedef typename std::add_pointer<N>::type PN;
    serializer << serialization::Pointer<N>(reinterpret_cast<PN>(node));
}


template <typename Content>
Serializer& serializeNodePointer(Serializer &serializer, quadtree::Node<Content> *node)
{
    typedef quadtree::ScopedNode<Content, quadtree::NodeType0000> N00;
    typedef quadtree::ScopedNode<Content, quadtree::NodeType1000> N01;
    typedef quadtree::ScopedNode<Content, quadtree::NodeType0100> N02;
    typedef quadtree::ScopedNode<Content, quadtree::NodeType1100> N03;
    typedef quadtree::ScopedNode<Content, quadtree::NodeType0010> N04;
    typedef quadtree::ScopedNode<Content, quadtree::NodeType1010> N05;
    typedef quadtree::ScopedNode<Content, quadtree::NodeType0110> N06;
    typedef quadtree::ScopedNode<Content, quadtree::NodeType1110> N07;
    typedef quadtree::ScopedNode<Content, quadtree::NodeType0001> N08;
    typedef quadtree::ScopedNode<Content, quadtree::NodeType1001> N09;
    typedef quadtree::ScopedNode<Content, quadtree::NodeType0101> N10;
    typedef quadtree::ScopedNode<Content, quadtree::NodeType1101> N11;
    typedef quadtree::ScopedNode<Content, quadtree::NodeType0011> N12;
    typedef quadtree::ScopedNode<Content, quadtree::NodeType1011> N13;
    typedef quadtree::ScopedNode<Content, quadtree::NodeType0111> N14;
    typedef quadtree::ScopedNode<Content, quadtree::NodeType1111> N15;

    quadtree::NodeKey key = node->key();

    using serialization::Pointer;

    if (key == 7)
    {
        snp<N07>(serializer, node);
    }
    else if (key < 7)
    {
        if (key == 3)
            snp<N03>(serializer, node);
        else if (key < 3)
        {
            if (key == 1)
                snp<N01>(serializer, node);
            else if (key == 0)
                snp<N00>(serializer, node);
            else if (key == 2)
                snp<N02>(serializer, node);
        }
        else {
            if (key == 5)
                snp<N05>(serializer, node);
            else if (key == 4)
                snp<N04>(serializer, node);
            else if (key == 6)
                snp<N06>(serializer, node);
        }
    }
    else { // key > 7
        if (key == 11)
            snp<N11>(serializer, node);
        else if (key < 11)
        {
            if (key == 9)
                snp<N09>(serializer, node);
            else if (key == 8)
                snp<N08>(serializer, node);
            else if (key == 10)
                snp<N10>(serializer, node);
        }
        else // key > 13
        {
            if (key == 13)
                snp<N13>(serializer, node);
            else if (key == 12)
                snp<N12>(serializer, node);
            else if (key == 15)
                snp<N15>(serializer, node);
            else if (key == 14)
                snp<N14>(serializer, node);
        }
    }
    return serializer;
}

//------------------------------------------------------------------------------
// QuadTree<N, Content>
//------------------------------------------------------------------------------

template <quadtree::BitSize N, typename Content>
serialization::Serializer &operator<<(serialization::Serializer &serializer,
                                      quadtree::QuadTree<N, Content>  &quadtree)
{
    typedef typename quadtree::QuadTree<N, Content>::NodeType NodeType;

    // serialize root pointer and schedule serializtion of its content
    serializeNodePointer(serializer, quadtree.root);

    serializer << static_cast<serialization::uint64>(quadtree.numAdds)
               << quadtree.counters;
    return serializer;
}

template <quadtree::BitSize N, typename Content>
serialization::Deserializer &operator>>(serialization::Deserializer &deserializer,
                                        quadtree::QuadTree<N, Content>  &quadtree)
{
    typedef typename quadtree::QuadTree<N, Content>::NodeType NodeType;

    // serializer pointer with the right type
    deserializer >> PointerRef<NodeType>(quadtree.root);

    // read numAdds
    serialization::uint64 numAdds;
    deserializer >> numAdds;
    quadtree.numAdds = numAdds;

    deserializer >> quadtree.counters;
    return deserializer;
}


template <quadtree::BitSize N, typename Content>
void addTypeToSchema(serialization::Schema &schema, const quadtree::QuadTree<N, Content>  &)
{
    // types
    typedef quadtree::QuadTree<N,Content>     QuadTreeType;
    typedef typename QuadTreeType::NodeType   NodeType;
    typedef quadtree::MemUsage                MemUsage;

    // create new custom type
    serialization::CustomType *type = serialization::retrieveOrCreateCustomType<QuadTreeType>(schema, getTypeName<QuadTreeType>());

    // assert it is an empty custom type
    assert(type->getNumFields() == 0);

    { // root: Node<Content> pointer
        // serialization::CustomType *field_type = serialization::retrieveOrCreateCustomType<NodeType>(schema,getTypeName<NodeType>());
        // pointer
        type->addField(serialization::FieldType(CoreSchema::ID_pointer, serialization::FieldType::POINTER), "root");
    }

    { // numAdds: uint64
        type->addField(serialization::FieldType(CoreSchema::ID_uint64, serialization::FieldType::VALUE),"numAdds");
    }


    { // counters: MemUsage
        serialization::CustomType *field_type = serialization::retrieveOrCreateCustomType<MemUsage>(schema,getTypeName<MemUsage>());
        type->addField(serialization::FieldType(field_type, serialization::FieldType::VALUE), "counters");
    }
}


//------------------------------------------------------------------------------
// ScopedNode<Content, NodeType>
//------------------------------------------------------------------------------


template <typename Content, typename NodeType>
serialization::Serializer &operator<<(serialization::Serializer &serializer,
                                      quadtree::ScopedNode<Content, NodeType> &scopedNode)
{
    typedef serialization::TaggedPointer<Content>   TagPtrContent;
    typedef quadtree::Node<Content>                 NodeContentType;

    const TagPtrContent &tagged_pointer = reinterpret_cast<const TagPtrContent&>(scopedNode.data);

    serializer << tagged_pointer; // save as a pointer for now

    // children nodes
    for (int i=0;i<NodeType::numChildren;i++)
        serializeNodePointer(serializer, scopedNode.children[i]);

        // serializer << serialization::Pointer<NodeContentType>(scopedNode.children[i]); // Node<Content>*
    return serializer;
}

template <typename Content, typename NodeType>
serialization::Deserializer &operator>>(serialization::Deserializer &deserializer,
                                        quadtree::ScopedNode<Content, NodeType> &scopedNode)
{
    typedef serialization::TaggedPointer<Content>    TagPtrContent;
    typedef serialization::TaggedPointerRef<Content> TagPtrContentRef;

    TagPtrContent    &tagged_pointer = reinterpret_cast<TagPtrContent&>(scopedNode.data);
    TagPtrContentRef tagged_pointer_ref(tagged_pointer);

    deserializer >> tagged_pointer_ref;

    typedef quadtree::Node<Content> NodeContentType;

//    void *ptr;
//    deserializer >> ptr; // throwing away the content pointer for now; scopedNode.getContent(); // save as a pointer for now
    for (int i=0;i<NodeType::numChildren;i++)
        deserializer >> PointerRef<NodeContentType>(scopedNode.children[i]);

    return deserializer;
}


template <typename Content, typename NodeType>
void addTypeToSchema(serialization::Schema &schema,
                     const quadtree::ScopedNode<Content, NodeType> &)
{
    typedef quadtree::ScopedNode<Content, NodeType> ScopedNodeType;

    // create new custom type
    serialization::CustomType *type = serialization::retrieveOrCreateCustomType<ScopedNodeType>(schema, getTypeName<ScopedNodeType>());

    // assert it is an empty custom type
    assert(type->getNumFields() == 0);

    { // content
        type->addField(serialization::FieldType(serialization::CoreSchema::ID_tagged_pointer, serialization::FieldType::POINTER),"content");
    }

    // TODO: field is an array of fixed length; (prepare a type for that).
    { // totalCount

        for (int i=0;i<NodeType::numChildren;i++)
        {
            std::stringstream ss;
            ss << "child_";
            ss << i;
            type->addField(serialization::FieldType(serialization::CoreSchema::ID_pointer, serialization::FieldType::POINTER),ss.str());
        }
    }
}

//-----------------------------------------------------------------------------------------
// MemUsage
//-----------------------------------------------------------------------------------------

serialization::Serializer &operator<<(serialization::Serializer &serializer,
                                      const quadtree::MemUsage &memUsage);

serialization::Deserializer &operator>>(serialization::Deserializer &deserializer,
                                        quadtree::MemUsage &memUsage);

void addTypeToSchema(serialization::Schema &schema, const quadtree::MemUsage &);

//-----------------------------------------------------------------------------------------
// Registration Template Functions
//-----------------------------------------------------------------------------------------

template <quadtree::BitSize N, typename Content>
void registerQuadTreeToSchema(Schema &schema)
{
    quadtree::QuadTree<N, Content> qt;

    addTypeToSchema(schema, qt.counters);

    { quadtree::ScopedNode<Content, quadtree::NodeType0000> node; addTypeToSchema(schema, node); }
    { quadtree::ScopedNode<Content, quadtree::NodeType1000> node; addTypeToSchema(schema, node); }
    { quadtree::ScopedNode<Content, quadtree::NodeType0100> node; addTypeToSchema(schema, node); }
    { quadtree::ScopedNode<Content, quadtree::NodeType1100> node; addTypeToSchema(schema, node); }
    { quadtree::ScopedNode<Content, quadtree::NodeType0010> node; addTypeToSchema(schema, node); }
    { quadtree::ScopedNode<Content, quadtree::NodeType1010> node; addTypeToSchema(schema, node); }
    { quadtree::ScopedNode<Content, quadtree::NodeType0110> node; addTypeToSchema(schema, node); }
    { quadtree::ScopedNode<Content, quadtree::NodeType1110> node; addTypeToSchema(schema, node); }
    { quadtree::ScopedNode<Content, quadtree::NodeType0001> node; addTypeToSchema(schema, node); }
    { quadtree::ScopedNode<Content, quadtree::NodeType1001> node; addTypeToSchema(schema, node); }
    { quadtree::ScopedNode<Content, quadtree::NodeType0101> node; addTypeToSchema(schema, node); }
    { quadtree::ScopedNode<Content, quadtree::NodeType1101> node; addTypeToSchema(schema, node); }
    { quadtree::ScopedNode<Content, quadtree::NodeType0011> node; addTypeToSchema(schema, node); }
    { quadtree::ScopedNode<Content, quadtree::NodeType1011> node; addTypeToSchema(schema, node); }
    { quadtree::ScopedNode<Content, quadtree::NodeType0111> node; addTypeToSchema(schema, node); }
    { quadtree::ScopedNode<Content, quadtree::NodeType1111> node; addTypeToSchema(schema, node); }

    addTypeToSchema(schema, qt);
}

template <quadtree::BitSize N, typename Content>
void registerQuadTreeObjectFactories(Schema &schema, Deserializer &deserializer)
{
    typedef quadtree::QuadTree<N, Content> QuadTreeType;

    typedef quadtree::ScopedNode<Content, quadtree::NodeType0000> N00;
    typedef quadtree::ScopedNode<Content, quadtree::NodeType1000> N01;
    typedef quadtree::ScopedNode<Content, quadtree::NodeType0100> N02;
    typedef quadtree::ScopedNode<Content, quadtree::NodeType1100> N03;
    typedef quadtree::ScopedNode<Content, quadtree::NodeType0010> N04;
    typedef quadtree::ScopedNode<Content, quadtree::NodeType1010> N05;
    typedef quadtree::ScopedNode<Content, quadtree::NodeType0110> N06;
    typedef quadtree::ScopedNode<Content, quadtree::NodeType1110> N07;
    typedef quadtree::ScopedNode<Content, quadtree::NodeType0001> N08;
    typedef quadtree::ScopedNode<Content, quadtree::NodeType1001> N09;
    typedef quadtree::ScopedNode<Content, quadtree::NodeType0101> N10;
    typedef quadtree::ScopedNode<Content, quadtree::NodeType1101> N11;
    typedef quadtree::ScopedNode<Content, quadtree::NodeType0011> N12;
    typedef quadtree::ScopedNode<Content, quadtree::NodeType1011> N13;
    typedef quadtree::ScopedNode<Content, quadtree::NodeType0111> N14;
    typedef quadtree::ScopedNode<Content, quadtree::NodeType1111> N15;

    serialization::util::addObjectFactory<quadtree::MemUsage>(schema, deserializer);
    serialization::util::addObjectFactory<N00>(schema, deserializer);
    serialization::util::addObjectFactory<N01>(schema, deserializer);
    serialization::util::addObjectFactory<N02>(schema, deserializer);
    serialization::util::addObjectFactory<N03>(schema, deserializer);
    serialization::util::addObjectFactory<N04>(schema, deserializer);
    serialization::util::addObjectFactory<N05>(schema, deserializer);
    serialization::util::addObjectFactory<N06>(schema, deserializer);
    serialization::util::addObjectFactory<N07>(schema, deserializer);
    serialization::util::addObjectFactory<N08>(schema, deserializer);
    serialization::util::addObjectFactory<N09>(schema, deserializer);
    serialization::util::addObjectFactory<N10>(schema, deserializer);
    serialization::util::addObjectFactory<N11>(schema, deserializer);
    serialization::util::addObjectFactory<N12>(schema, deserializer);
    serialization::util::addObjectFactory<N13>(schema, deserializer);
    serialization::util::addObjectFactory<N14>(schema, deserializer);
    serialization::util::addObjectFactory<N15>(schema, deserializer);


    serialization::util::addObjectFactory<QuadTreeType>(schema, deserializer);
}

} // end serialization


