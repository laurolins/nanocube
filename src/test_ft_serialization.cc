#include <FlatTree.hh>
#include <FlatTreeSerialization.hh>

typedef int NullContent;

typedef flattree::FlatTree<NullContent> FTree;
typedef typename FTree::AddressType     FAddr;
typedef typename FTree::NodeStackType   FStack;

int main()
{
    FTree  &ft = *FTree::create();
    FStack stack;
    ft.trailProperPath(FAddr(1), stack);
    ft.trailProperPath(FAddr(5), stack);
    ft.trailProperPath(FAddr(7), stack);

    { // serialize quadtree

        flattree::Link<NullContent> link; // dummy link just to register into schema

        serialization::Schema schema;
        serialization::CoreSchema::initCoreSchema(schema);
        schema << ft << link; // register

        serialization::Serializer serializer(schema, "/tmp/ft.bin");

        serializer.scheduleSerialization(serialization::Pointer<serialization::Schema>(&schema));
        serializer.run();

        serializer.scheduleSerialization(serialization::Pointer<FTree>(&ft));
        serializer.run();

    }

#if 1
    { // deserialize quadtree

        serialization::Deserializer deserializer("/tmp/ft.bin");

        // deserialize schema
        deserializer.run(0,serialization::CoreSchema::LAST_BUILTIN_TYPEID);

        // get schema
        std::vector<serialization::Schema*> schemas;
        deserializer.collectDeserializedObjectsByType(serialization::CoreSchema::ID_Schema, schemas);
        serialization::Schema &schema = *schemas[0];

        // log
        std::cout << "Retrieved Schema:" << std::endl;
        std::cout << schema << std::endl;

        serialization::registerFlatTreeObjectFactories<NullContent>(schema, deserializer);

        // deserialize user data
        deserializer.run(serialization::CoreSchema::FIRST_USER_TYPEID,
                         serialization::CoreSchema::MAXIMUM_USER_TYPEID);

        std::vector<FTree*> ftrees;
        deserializer.collectDeserializedObjectsByType(schema.getTypeByName(serialization::getTypeName<FTree>())->getTypeID(), ftrees);
        FTree& ft_restored = *ftrees[0];

        // qt_restored.

        {
            std::cout << "---------------- Original FlatTree -----------------" << std::endl;
            ft.dump(std::cout);
        }
        {
            std::cout << "---------------- Restored FlatTree -----------------" << std::endl;
            ft_restored.dump(std::cout);
        }
    }
#endif
    return 0;

}
