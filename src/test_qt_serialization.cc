#include <QuadTreeUtil.hh>
#include <QuadTreeSerialization.hh>

static const int Levels = 25;

typedef int NullContent;

typedef quadtree::QuadTree<Levels,NullContent> QTree;
typedef typename QTree::AddressType QAddr;
typedef typename QTree::NodeStackType QStack;

int main()
{
    QTree qt;
    QStack stack;
    qt.trailProperPath(QAddr(0,1,25),       stack);
    qt.trailProperPath(QAddr(123,312,25),   stack);
    qt.trailProperPath(QAddr(1123,1312,25), stack);

    { // serialize quadtree

        serialization::Schema schema;
        serialization::CoreSchema::initCoreSchema(schema);
        serialization::registerQuadTreeToSchema<Levels, NullContent>(schema);

        serialization::Serializer serializer(schema, "/tmp/qt.bin");

        serializer.scheduleSerialization(serialization::Pointer<serialization::Schema>(&schema));
        serializer.run();

        serializer.scheduleSerialization(serialization::Pointer<QTree>(&qt));
        serializer.run();

    }


    { // deserialize quadtree

        serialization::Deserializer deserializer("/tmp/qt.bin");

        // deserialize schema
        deserializer.run(0,serialization::CoreSchema::LAST_BUILTIN_TYPEID);

        // get schema
        std::vector<serialization::Schema*> schemas;
        deserializer.collectDeserializedObjectsByType(serialization::CoreSchema::ID_Schema, schemas);
        serialization::Schema &schema = *schemas[0];

        // log
        std::cout << "Retrieved Schema:" << std::endl;
        std::cout << schema << std::endl;

        serialization::registerQuadTreeObjectFactories<Levels,NullContent>(schema, deserializer);

        // deserialize user data
        deserializer.run(serialization::CoreSchema::FIRST_USER_TYPEID, 0xFFFFFF);

        std::vector<QTree*> qtrees;
        deserializer.collectDeserializedObjectsByType(
                    schema.getTypeByName(serialization::util::getTypeName<QTree>())->getTypeID(),
                    qtrees);
        QTree& qt_restored = *qtrees[0];

        // qt_restored.

        {
            quadtree::Stats<QTree> stats;
            stats.initialize(qt);
            std::cout << "---------------- Original QuadTree Stats-----------------" << std::endl;
            stats.dumpReport(std::cout);
        }

        {
            quadtree::Stats<QTree> stats;
            stats.initialize(qt_restored);
            std::cout << "---------------- Restored QuadTree Stats-----------------" << std::endl;
            stats.dumpReport(std::cout);
        }
    }

    return 0;

}
