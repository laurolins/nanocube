#include <cstdint>
#include <TimeSeries.hh>
#include <TimeSeriesSerialization.hh>


typedef uint32_t TSTime;
typedef uint32_t TSCount;
typedef timeseries::TimeSeries<TSTime, TSCount> TS;
typedef timeseries::Entry<TSTime, TSCount> TSEntry;

int main()
{
    TS ts;

    ts.add(1);
    ts.add(1);
    ts.add(2);
    ts.add(2);
    ts.add(2);
    ts.add(3);

    { // serialize quadtree

        TSEntry e;

        serialization::Schema schema;
        serialization::CoreSchema::initCoreSchema(schema);
        schema << ts << e; // register

        serialization::Serializer serializer(schema, "/tmp/ts.bin");

        serializer.scheduleSerialization(serialization::Pointer<serialization::Schema>(&schema));
        serializer.run();

        serializer.scheduleSerialization(serialization::Pointer<TS>(&ts));
        serializer.run();

    }

#if 1
    { // deserialize quadtree

        serialization::Deserializer deserializer("/tmp/ts.bin");

        // deserialize schema
        deserializer.run(0,serialization::CoreSchema::LAST_BUILTIN_TYPEID);

        // get schema
        std::vector<serialization::Schema*> schemas;
        deserializer.collectDeserializedObjectsByType(serialization::CoreSchema::ID_Schema, schemas);
        serialization::Schema &schema = *schemas[0];

        // log
        std::cout << "Retrieved Schema:" << std::endl;
        std::cout << schema << std::endl;

        serialization::registerTimeSeriesObjectFactories<TSTime, TSCount>(schema, deserializer);

        // deserialize user data
        deserializer.run(serialization::CoreSchema::FIRST_USER_TYPEID,
                         serialization::CoreSchema::MAXIMUM_USER_TYPEID);

        std::vector<TS*> TSs;
        deserializer.collectDeserializedObjectsByType(schema.getTypeByName(serialization::getTypeName<TS>())->getTypeID(), TSs);
        TS& ts_restored = *TSs[0];

        // qt_restored.

        {
            std::cout << "---------------- Original TimeSeries -----------------" << std::endl;
            ts.dump(std::cout);
        }
        {
            std::cout << "---------------- Restored TimeSeries -----------------" << std::endl;
            ts_restored.dump(std::cout);
        }
    }
#endif
    return 0;

}
