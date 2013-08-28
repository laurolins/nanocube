#include <QuadTreeSerialization.hh>

namespace serialization {

//---------------------------------------------------------------------------------
// MemUsage
//---------------------------------------------------------------------------------

serialization::Serializer &operator<<(serialization::Serializer &serializer,
                                      const quadtree::MemUsage &memUsage)
{

    serialization::uint64 totalMem = static_cast<serialization::uint64>(memUsage.totalMem);
    serializer << totalMem;

    serialization::uint64 totalCount = static_cast<serialization::uint64>(memUsage.totalCount);
    serializer << totalCount;

    // serialize repeated list of numbers
    serialization::uint32 size = static_cast<serialization::uint32>(memUsage.memByNumChildren.size());
    serializer << size;
    for (quadtree::Count c: memUsage.memByNumChildren)
    {
        serialization::uint64 cc = static_cast<serialization::uint64>(c);
        serializer << cc;
    }

    // serialize repeated list of numbers
    serializer << static_cast<serialization::uint32>(memUsage.countByNumChildren.size());
    for (quadtree::Count c: memUsage.countByNumChildren)
    {
        serialization::uint64 cc = static_cast<serialization::uint64>(c);
        serializer << cc;
    }

    return serializer;
}

serialization::Deserializer &operator>>(serialization::Deserializer &deserializer,
                                        quadtree::MemUsage &memUsage)
{
    serialization::uint64 totalMem = static_cast<serialization::uint64>(memUsage.totalMem);
    deserializer >> totalMem;
    memUsage.totalMem = totalMem;

    serialization::uint64 totalCount = static_cast<serialization::uint64>(memUsage.totalCount);
    deserializer >> totalCount;
    memUsage.totalCount = totalCount;

    // serialize repeated list of numbers
    {
        serialization::uint32 size;
        deserializer >> size;
        memUsage.memByNumChildren.resize(size);
        for (size_t i=0;i<size;i++)
                 deserializer >> memUsage.memByNumChildren[i];
    }

    // serialize repeated list of numbers
    {
        serialization::uint32 size;
        deserializer >> size;
        memUsage.countByNumChildren.resize(size);
        for (size_t i=0;i<size;i++)
            deserializer >> memUsage.countByNumChildren[i];
    }

    return deserializer;
}

void addTypeToSchema(serialization::Schema &schema, const quadtree::MemUsage &)
{
    // create new custom type
    serialization::CustomType *type =
             serialization::retrieveOrCreateCustomType<quadtree::MemUsage>(schema,
                                                                          getTypeName<quadtree::MemUsage>());

    // assert it is an empty custom type
    assert(type->getNumFields() == 0);

    { // totalMem
        type->addField(serialization::FieldType(CoreSchema::ID_uint64, FieldType::VALUE),"totalMem");
    }

    { // totalCount
        type->addField(serialization::FieldType(CoreSchema::ID_uint64, FieldType::VALUE),"totalCount");
    }

    { // memByNumChildren
        type->addField(serialization::FieldType(CoreSchema::ID_uint64, FieldType::VALUE, FieldType::REPEAT_32),"memByNumChildren");
    }

    { // countByNumChildren
        type->addField(serialization::FieldType(CoreSchema::ID_uint64, FieldType::VALUE, FieldType::REPEAT_32),"countByNumChildren");
    }

}

} 
