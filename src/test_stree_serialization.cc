#include <iostream>
#include <cstdint>

#include <boost/mpl/vector.hpp>
#include <boost/mpl/reverse.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/begin_end.hpp>
#include <boost/mpl/back.hpp>
#include <boost/mpl/front.hpp>
#include <boost/type_traits/is_same.hpp>

// some string manipulation
#include <boost/algorithm/string.hpp>

#include <TimeSeries.hh>
#include <FlatTree.hh>
#include <STree.hh>
#include <QuadTree.hh>
#include <QuadTreeUtil.hh>

#include <TimeSeriesSerialization.hh>
#include <FlatTreeSerialization.hh>
#include <QuadTreeSerialization.hh>
#include <STreeSerialization.hh>


static const quadtree::BitSize Levels = 25;

typedef uint32_t TSTime;
typedef uint32_t TSCount;
typedef timeseries::TimeSeries<TSTime, TSCount> TS;

typedef flattree::FlatTree<TS>      HTree; // hour tree (content is a timeseries)
typedef flattree::FlatTree<HTree>   WTree; // weekday tree (content is an hour tree)

typedef quadtree::QuadTree<Levels,WTree>   QTree; // quad tree (content is a wtree)

typedef typename HTree::AddressType  HAddr; // hour address
typedef typename WTree::AddressType  WAddr; // week address
typedef typename QTree::AddressType  QAddr; // week address

typedef boost::mpl::vector<QTree, WTree, HTree> STreeTypeList;
typedef stree::STree<STreeTypeList, TSTime> STreeType;

int main()
{

    STreeType st;

    {
        WAddr waddr(2);
        HAddr haddr(2);
        QAddr qaddr(0,0,25);
        st.setAddress(qaddr);
        st.setAddress(waddr);
        st.setAddress(haddr);
        st.add(2);
    }

    {
        WAddr waddr(3);
        HAddr haddr(3);
        QAddr qaddr(1,1,25);
        st.setAddress(qaddr);
        st.setAddress(waddr);
        st.setAddress(haddr);
        st.add(3);
    }



    { // serialize quadtree

        serialization::Schema schema;
        serialization::CoreSchema::initCoreSchema(schema);

        serialization::registerTimeSeriesToSchema<TSTime, TSCount>(schema);
        serialization::registerFlatTreeToSchema<TS>(schema);
        serialization::registerFlatTreeToSchema<HTree>(schema);
        serialization::registerQuadTreeToSchema<Levels,WTree>(schema);
        serialization::registerSTreeToSchema<STreeTypeList, TSTime>(schema);

        serialization::Serializer serializer(schema, "/tmp/stree.bin");

        serializer.scheduleSerialization(serialization::Pointer<serialization::Schema>(&schema));
        serializer.run();

        serializer.scheduleSerialization(serialization::Pointer<STreeType>(&st));
        serializer.run();

    }

#if 1
    { // deserialize quadtree

        serialization::Deserializer deserializer("/tmp/stree.bin");

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
        serialization::registerFlatTreeObjectFactories<TS>(schema, deserializer);
        serialization::registerFlatTreeObjectFactories<HTree>(schema, deserializer);
        serialization::registerQuadTreeObjectFactories<Levels, WTree>(schema, deserializer);
        serialization::registerSTreeObjectFactories<STreeTypeList, TSTime>(schema, deserializer);

        // deserialize user data
        deserializer.run(serialization::CoreSchema::FIRST_USER_TYPEID,
                         serialization::CoreSchema::MAXIMUM_USER_TYPEID);

        std::vector<STreeType*> STs;
        deserializer.collectDeserializedObjectsByType(schema.getTypeByName(serialization::util::getTypeName<STreeType>())->getTypeID(), STs);
        STreeType& st_restored = *STs[0];

        // qt_restored.

        {
            std::cout << "---------------- Original STree -----------------" << std::endl;
            quadtree::Stats<QTree> stats;
            stats.initialize(st.root);
            stats.dumpReport(std::cout);
        }
        {
            std::cout << "---------------- Restored STree -----------------" << std::endl;
            quadtree::Stats<QTree> stats;
            stats.initialize(st_restored.root);
            stats.dumpReport(std::cout);
        }
    }
#endif


    return 0;



}

