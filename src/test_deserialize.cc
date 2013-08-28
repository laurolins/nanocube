#include <boost/mpl/vector.hpp>

#include <chrono>
#include <cstdint>

#include <datatiles/MemoryUtil.hh>

#include <datatiles/QuadTree.hh>
#include <datatiles/QuadTreeUtil.hh>
#include <datatiles/TimeSeries.hh>
#include <datatiles/FlatTree.hh>
#include <datatiles/STree.hh>

#include <algorithm>

#include <boost/mpl/vector.hpp>

#include <datatiles/TimeSeriesSerialization.hh>
#include <datatiles/FlatTreeSerialization.hh>
#include <datatiles/QuadTreeSerialization.hh>
#include <datatiles/STreeSerialization.hh>

#include <datatiles/Stopwatch.hh>



typedef serialization::Deserializer::Progress Progress;

static memory_util::MemTracker<Progress> *mem_tracker = 0;

void update_progress(Progress new_progress)
{
    mem_tracker->progress = new_progress;
}

//-------------------------------------------------------------------------------
// Load Index
//-------------------------------------------------------------------------------

void loadIndex_SH(const std::string filename)
{
    typedef uint32_t TimeBin; // timeseries time bin type
    typedef uint32_t TSCount; // timeseries count type

    typedef timeseries::TimeSeries<TimeBin, TSCount> EventTimeSeries;

    static const int Levels = 25;

    typedef flattree::FlatTree<EventTimeSeries> Dim1;
    typedef quadtree::QuadTree<Levels,Dim1>     Dim0;

    typedef Dim1 HTree;
    typedef Dim0 QTree;

    typedef boost::mpl::vector<Dim0, Dim1> PathListType;

    typedef typename Dim0::AddressType QAddr;
    typedef typename Dim1::AddressType HAddr;

    typedef stree::STree<PathListType, TimeBin> STreeType;

    typedef STreeType Index;

    serialization::Deserializer deserializer(filename);

    //
    serialization::Deserializer::ProgressCallback pc = &update_progress;
    deserializer.registerProgressCallback(&pc);

    // deserialize schema
    deserializer.run(0,serialization::CoreSchema::LAST_BUILTIN_TYPEID);

    // get schema
    std::vector<serialization::Schema*> schemas;
    deserializer.collectDeserializedObjectsByType(serialization::CoreSchema::ID_Schema, schemas);
    serialization::Schema &schema = *schemas[0];

    // log
    std::cout << "Retrieved Schema:" << std::endl;
    std::cout << schema << std::endl;

    serialization::registerTimeSeriesObjectFactories<TimeBin, TSCount>(schema, deserializer);
    serialization::registerFlatTreeObjectFactories<EventTimeSeries>(schema, deserializer);
    serialization::registerQuadTreeObjectFactories<Levels, HTree>(schema, deserializer);
    serialization::registerSTreeObjectFactories<PathListType, TimeBin>(schema, deserializer);

    // deserialize user data
    deserializer.run(serialization::CoreSchema::FIRST_USER_TYPEID,
                     serialization::CoreSchema::MAXIMUM_USER_TYPEID);

    std::vector<Index*> STs;
    deserializer.collectDeserializedObjectsByType(schema.getTypeByName(serialization::util::getTypeName<Index>())->getTypeID(), STs);

    // Index *st_restored = STs[0];

    // qt_restored.
    // return st_restored;
}



//-------------------------------------------------------------------------------
// Load Index
//-------------------------------------------------------------------------------

void loadIndex_S(const std::string filename)
{
    typedef uint32_t TimeBin; // timeseries time bin type
    typedef uint32_t TSCount; // timeseries count type

    typedef timeseries::TimeSeries<TimeBin, TSCount> EventTimeSeries;

    static const int Levels = 25;

    typedef quadtree::QuadTree<Levels,EventTimeSeries>     Dim0;

    typedef Dim0 QTree;

    typedef boost::mpl::vector<Dim0> PathListType;

    typedef typename Dim0::AddressType QAddr;

    typedef stree::STree<PathListType, TimeBin> STreeType;

    typedef STreeType Index;

    serialization::Deserializer deserializer(filename);

    //
    serialization::Deserializer::ProgressCallback pc = &update_progress;
    deserializer.registerProgressCallback(&pc);

    // deserialize schema
    deserializer.run(0,serialization::CoreSchema::LAST_BUILTIN_TYPEID);

    // get schema
    std::vector<serialization::Schema*> schemas;
    deserializer.collectDeserializedObjectsByType(serialization::CoreSchema::ID_Schema, schemas);
    serialization::Schema &schema = *schemas[0];

    // log
    std::cout << "Retrieved Schema:" << std::endl;
    std::cout << schema << std::endl;

    serialization::registerTimeSeriesObjectFactories<TimeBin, TSCount>(schema, deserializer);
    serialization::registerQuadTreeObjectFactories<Levels, EventTimeSeries>(schema, deserializer);
    serialization::registerSTreeObjectFactories<PathListType, TimeBin>(schema, deserializer);

    // deserialize user data
    deserializer.run(serialization::CoreSchema::FIRST_USER_TYPEID,
                     serialization::CoreSchema::MAXIMUM_USER_TYPEID);

    std::vector<Index*> STs;
    deserializer.collectDeserializedObjectsByType(schema.getTypeByName(serialization::util::getTypeName<Index>())->getTypeID(), STs);

    // Index *st_restored = STs[0];

    // qt_restored.
    // return st_restored;
}


using std::chrono::system_clock;

int main(int argc, char **argv)
{
    enum IndexType { SPATIAL, SPATIAL_HOUR, SPATIAL_WEEKDAY_HOUR, SPATIAL_WEEKDAY };

    IndexType index_type = SPATIAL;
    std::string filename("");

    for (int i=1;i<argc;i++)
    {
        std::string st_i(argv[i]);

        // int n = st_i.size();

//        std::string prefix = st_upper_i.substr(0,2);
//        std::string suffix = st_i.substr(2,n-2);

        // --type=SH
        if (st_i.find("--type=") == 0)
        {
            std::string type = st_i.substr(7);
            if (type.compare("S") == 0 || type.compare("SPATIAL") == 0)
            {
                index_type = SPATIAL;
            }
            else if (type.compare("SH") == 0 || type.compare("SPATIAL_HOUR") == 0)
            {
                index_type = SPATIAL_HOUR;
            }
            else if (type.compare("SW") == 0 || type.compare("SPATIAL_WEEKDAY") == 0)
            {
                index_type = SPATIAL_WEEKDAY;
            }
            else if (type.compare("SWH") == 0 || type.compare("SPATIAL_WEEKDAY_HOUR") == 0)
            {
                index_type = SPATIAL_WEEKDAY_HOUR;
            }
        }

        else
        {
            filename = st_i;
        }
    }

    assert(filename.size() > 0);
    assert(index_type == SPATIAL_HOUR || index_type == SPATIAL);

    //
    std::time_t now_c = system_clock::to_time_t(system_clock::now());
    char time_st[100];
    std::strftime(time_st,100,"%Y%m%d%H%M%S", std::localtime(&now_c));
    std::string timestamp(time_st);
    std::string mem_filename = "testdeserialize-"+timestamp+".tab";
    std::cout << mem_filename <<std::endl;


    //
    memory_util::MemTracker<Progress> memTracker(0L, mem_filename);
    mem_tracker = &memTracker;

    stopwatch::Stopwatch w;
    stopwatch::Milliseconds t;
    {
        stopwatch::StartStop s(w,t);
        if (index_type == SPATIAL) {
            std::cout << "Deserializing SPATIAL index: " << filename << std::endl;
            loadIndex_S(filename);
        }
        else if (index_type == SPATIAL_HOUR) {
            std::cout << "Deserializing SPATIAL_HOUR index: " << filename << std::endl;
            loadIndex_SH(filename);
        }
    }
    std::cout << "Time to load " << t/1000.0 << std::endl;
    return 0;
}




























#if 0


// find timeseries of everything
auto n0 = stree.root.find(quadtree::Address<1>());

flattree::Address emptyPath;

{
    auto n1 = n0->getContent()->find(emptyPath);
    auto n2 = n1->getContent()->find(emptyPath);
    EventTimeSeries &et = *n2->getContent();
    std::clog << "===================================================" << std::endl;
    std::clog << "No Weekday Constraint: " << std::endl << "   ";
    et.dump(std::clog);
}

for (flattree::PathElement weekday=0;weekday<7;weekday++)
{
    flattree::Address weekdayPath(weekday);
    auto n1 = n0->getContent()->find(weekdayPath);
    if (!n1)
        continue;
    auto n2 = n1->getContent()->find(emptyPath);
    EventTimeSeries &et = *n2->getContent();
    std::clog << "===================================================" << std::endl;
    std::clog << "Weekday: " << (int) weekday << std::endl << "   ";
    et.dump(std::clog);
}

for (flattree::PathElement hour=0;hour<24;hour++)
{
    flattree::Address hourPath(hour);
    auto n1 = n0->getContent()->find(emptyPath);
    if (!n1)
        continue;
    auto n2 = n1->getContent()->find(hourPath);
    if (!n2)
        continue;
    EventTimeSeries &et = *n2->getContent();
    std::clog << "===================================================" << std::endl;
    std::clog << "Hour: " << (int) hour << std::endl << "   ";
    et.dump(std::clog);
}


for (flattree::PathElement weekday=0;weekday<7;weekday++)
{
    flattree::Address weekdayPath(weekday);

    auto n1 = n0->getContent()->find(weekdayPath);
    if (!n1)
        continue;

    for (flattree::PathElement hour=0;hour<24;hour++)
    {
        flattree::Address hourPath(hour);

        auto n2 = n1->getContent()->find(hourPath);
        if (!n2)
            continue;

        EventTimeSeries &et = *n2->getContent();
        std::clog << "===================================================" << std::endl;
        std::clog << "Weekday: " << (int) weekday << " Hour: " << (int) hour << std::endl << "   ";
        et.dump(std::clog);
    }
}


// // computing statistics
// quadtree::Stats<25> stats;
// stats.initialize(sTree.root);
// stats.dumpReport(std::clog);

// dispatcher d;

// std::cout << typeid(types).name() << std::endl;
// std::cout << typeid(mpl::next<types>::type::item).name() << std::endl;
// std::cout << typeid(mpl::next<mpl::next<types>::type>::type).name() << std::endl;


#endif
