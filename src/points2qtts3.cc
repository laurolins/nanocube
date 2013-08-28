#include <iostream>
#include <random>
#include <chrono>
#include <sstream>
#include <iomanip>

#include <set>
#include <stdio.h>
#include <string.h>

#include <cmath>
#include <cstdlib>
#include <ctime>
#include <cstdio>

#include <unordered_map>

#include <string>
#include <fstream>
#include <sstream>

#include <datatiles/Stopwatch.hh>
#include <datatiles/TimeSeries.hh>
#include <datatiles/QuadTree.hh>
#include <datatiles/QuadTreeUtil.hh>
#include <datatiles/MercatorProjection.hh>

#include <datatiles/STree.hh>
#include <datatiles/FlatTree.hh>

// boost::mpl Metaprogramming Template
#include <boost/mpl/reverse.hpp>
#include <boost/mpl/vector.hpp>
#include <boost/mpl/size.hpp>
#include <boost/mpl/begin_end.hpp>
#include <boost/mpl/back.hpp>
#include <boost/mpl/front.hpp>
#include <boost/type_traits/is_same.hpp>

// some string manipulation
#include <boost/algorithm/string.hpp>

#include <datatiles/MemoryUtil.hh>

#include <datatiles/TimeSeriesSerialization.hh>
#include <datatiles/FlatTreeSerialization.hh>
#include <datatiles/QuadTreeSerialization.hh>
#include <datatiles/STreeSerialization.hh>

// #include <boost/mpl/next_prior.hpp>
// #include <boost/mpl/for_each.hpp>
// #include <boost/mpl/range_c.hpp>

using namespace std;

typedef uint32_t Int;

typedef float RealCoordinate;

using quadtree::Coordinate;
using quadtree::Count;
using quadtree::BitSize;
using quadtree::QuadTree;
using quadtree::Address;
using quadtree::Node;
using quadtree::Stats; // from QuadTreeUtil

using timeseries::TimeSeries;
using timeseries::TimeSeriesStatistics;

//
// Timestamp
//

typedef uint64_t Timestamp;

Timestamp mkTimestamp(string st, string format="%Y-%m-%d %H:%M:%S")
{
    struct tm tm;
    memset(&tm, 0, sizeof(struct tm));
    strptime(st.c_str(), format.c_str(), &tm);
    return (Timestamp) mktime(&tm);
}

Timestamp mkTimestamp(int year, int month, int day, int hour, int min, int sec)
{
    struct tm timeinfo;
    memset(&timeinfo, 0, sizeof(struct tm));
    timeinfo.tm_year = year	 - 1900;
    timeinfo.tm_mon  = month - 1;
    timeinfo.tm_mday = day;
    timeinfo.tm_hour = hour;
    timeinfo.tm_min  = min;
    timeinfo.tm_sec  = sec;

    /* call mktime: timeinfo->tm_wday will be set */
    Timestamp	timestamp = (Timestamp) mktime ( &timeinfo );

    //
    return timestamp;
}

//
// Tokenizer: copied from stack overflow solution from Evan Teran
//

std::vector<std::string> &split(const std::string &s, char delim, std::vector<std::string> &elems)
{
    std::stringstream ss(s);
    std::string item;
    while(std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

//
// Auxiliar Text Formatting stuff
//

std::string fl(std::string st, int n=12)
{
    int l = st.size();
    if (l > n)
        return st;
    return st + std::string(n-l,' ');
}

std::string fr(std::string st, int n=12)
{
    int l = st.size();
    if (l > n)
        return st;
    return std::string(n-l,' ') + st;
}

template <class T>
inline std::string str(const T& t)
{
    std::stringstream ss;
    ss << std::setprecision(1) << std::fixed << t ;
    return ss.str();
}

template<typename Structure>
std::ostream& operator<<(std::ostream &os, const typename Structure::AddressType& addr)
{
    os << "Addr[x: "  << addr.x
       << ", y: "     << addr.y
       << ", level: " << addr.level
       << "]";
    return os;
}

class NullContent
{};

struct Point
{
    float    latitude;
    float    longitude;
    uint64_t timestamp;
};

//
// Small allocation TimeSeries
//


Count b2mb(Count b)
{
    Count mb = (Count) (b / (double) (1<<20));
    return mb;
}





enum Level
{
    L05 = 5,
    L10 = 10,
    L15 = 15,
    L20 = 20,
    L25 = 25
};

template <Level L>
struct LevelToType
{
    enum {value = L};
};

template <Level LevelBits>
void worker(int timeBinSizeInHours, std::string output_filename)
{

    typedef uint32_t EventCount;
    typedef uint32_t EventTime;
    typedef TimeSeries<EventTime, EventCount> EventTimeSeries;

    // day of week and time of day (periodic summaries)
    typedef flattree::FlatTree<EventTimeSeries>      TypeDim2;
    typedef flattree::FlatTree<TypeDim2>             TypeDim1;
    typedef quadtree::QuadTree<LevelBits, TypeDim1>  TypeDim0;


    typedef flattree::FlatTree<EventTimeSeries>      HTree;
    typedef flattree::FlatTree<TypeDim2>             WTree;
    typedef quadtree::QuadTree<LevelBits, TypeDim1>  QTree;

    // quadtree TS
    typedef TypeDim0 QuadTreeTS;

    typedef typename TypeDim0::AddressType QAddr;
    typedef typename TypeDim1::AddressType WAddr;
    typedef typename TypeDim2::AddressType HAddr;

    // nested trees for STree
    typedef mpl::vector<TypeDim0, TypeDim1, TypeDim2> STreeTypes;

    // STree for TimeSeries
    typedef stree::STree<STreeTypes, EventTime> STreeTS;

    //
    STreeTS    &stree = *(new STreeTS());
    QuadTreeTS &hitS = stree.root;

    std::clog << "sizeof(flattree::FlatTree<EventTimeSeries>): " << sizeof(TypeDim2) << std::endl;
    std::clog << "sizeof(flattree::Node<EventTimeSeries>): " << sizeof(flattree::Node<EventTimeSeries>) << std::endl;
    std::clog << "sizeof(EventTimeSeries): " << sizeof(EventTimeSeries) << std::endl;

    //
    // dumpTypeSizes(std::clog);

    std::clog << "Time Bin Size in Hours: " << timeBinSizeInHours << std::endl;
    std::clog << "QuadTree Levels:        " << LevelBits << std::endl;

    //
    Stopwatch    stopwatch;
    stopwatch.start();
    // Milliseconds timeAddAll;

    // show header of report
    char sep = '|';
    std::clog
        << sep
        << fl("Add Calls ")     << sep // number of add calls
        << fl("Num Nodes ")  << sep // num nodes in quad tree
        << fl("Num Leaves") << sep //
        << fl("Nodes/Leaf")         << sep //
        << fl("Num FT1   ")         << sep //
        << fl("Num FT2   ")         << sep //
        << fl("Num TS    ")         << sep //
        << fl("QT    (MB)") << sep
        << fl("FT1   (MB)") << sep
        << fl("FT2   (MB)") << sep
        << fl("TS    (MB)") << sep
        << fl("Total (MB)") << sep
        << fl("RES   (MB)") << sep
        << fl("VIRT  (MB)") << sep
        << fl("Time (sec)") << sep << std::endl;


    auto showProgress = [&] ()
        {
            // all in MB
            Count mem_qt  = b2mb(hitS.getMemoryUsage());
            Count mem_ft1 = b2mb(TypeDim1::mem());
            Count mem_ft2 = b2mb(TypeDim2::mem());
            Count mem_ts  = b2mb(EventTimeSeries::mem());
            Count mem_total = mem_qt + mem_ft1 + mem_ft2 + mem_ts;

            Count num_adds      = hitS.getNumAdds();
            Count num_qt_nodes  = hitS.getNumNodes();
            Count num_qt_leaves = hitS.getNumLeaves();
            Count num_ft1       = TypeDim1::num();
            Count num_ft2       = TypeDim2::num();
            Count num_ts        = EventTimeSeries::num();
            Count num_total     = num_qt_nodes + num_ft1 + num_ft2 + num_qt_nodes;

            double nodes_per_leaf = static_cast<double>(num_qt_nodes)/static_cast<double>(num_qt_leaves);

            memory_util::MemInfo memInfo = memory_util::MemInfo::get();
            Count mem_res     = memInfo.res_MB();
            Count mem_virt    = memInfo.virt_MB();

            double time_span = stopwatch.time()/1000.0;

            // report
            std::clog
            << sep
            << fr(str(num_adds))       << sep         // Add Calls
            << fr(str(num_qt_nodes))   << sep   // Num Nodes NN
            << fr(str(num_qt_leaves))  << sep   // Num Leaves NL
            << fr(str(nodes_per_leaf)) << sep   // NodesPerLeaf
            << fr(str(num_ft1))        << sep   // NN/NL
            << fr(str(num_ft2))        << sep   // NN/NL
            << fr(str(num_ts))         << sep   // NN/NL

            << fr(str(mem_qt))         << sep   // QT Size  (MB)
            << fr(str(mem_ft1))        << sep   // FT1 Size (MB)
            << fr(str(mem_ft2))        << sep   // FT2 Size (MB)
            << fr(str(mem_ts))         << sep   // TS Size  (MB)
            << fr(str(mem_total))      << sep   // Total    (MB)
            << fr(str(mem_res))        << sep   // RES      (MB)
            << fr(str(mem_virt))       << sep   // VIRT     (MB)

            << fr(str(time_span))      << sep << std::endl; // time to add all
        };

    Count showProgressStep = 1000000;

    Count count = 0;
    Count countProblems = 0;

    Point point;



//    ifstream is("/Users/lauro/projects/geo_pointset_tiles/build/x");
    FILE *filein = stdin;
    while (fread((void*) &point, sizeof(Point), 1, filein))
    {
        count++;

//        if (count == 10)
//            break;

         // if ((count % 1) == 0)
//         {
//             std::clog << "Reading point: " << count << " problems: " << countProblems << " adds: " << hitS.getNumAdds() << std::endl;
//             std::clog << "   point lat, long: " << point.latitude << ", " << point.longitude << std::endl;

//             // unsigned char* ptr = (unsigned char*) &point;
//             // fprintf(stderr,"\n------------> RECEIVED %3ld:   ", count);
//             // for (int i=0;i<sizeof(Point);i++)
//             //     fprintf(stderr,"%x ",ptr[i]);
//             // fprintf(stderr,"\n\n");

//         }

        mercator::TileCoordinate tx, ty;
        mercator::MercatorProjection::tileOfLongitudeLatitude(point.longitude,
                                                              point.latitude,
                                                              LevelBits,
                                                              tx, ty);
        // tile is
        // std::cout << "coord: "
        //           << coords.longitude << "  "
        //           << coords.latitude << std::endl;
        // std::cout << "tile: " << tx << "  " << ty << std::endl;

        if (tx >= (1 << LevelBits) || (tx < 0) || ty >= (1 << LevelBits) || (ty < 0))
        {
            countProblems++;
            continue; // point outside bounds
        }

        time_t ttt = static_cast<time_t>(point.timestamp);
        tm time_record = *gmtime(&ttt);

        QAddr  a0((Coordinate) tx, (Coordinate) ty, LevelBits);
        WAddr  a1 = WAddr(time_record.tm_wday); // spatial address
        HAddr  a2 = HAddr(time_record.tm_hour); // 10 o'clock

        // set address
        stree.setAddress(a0);
        stree.setAddress(a1);
        stree.setAddress(a2);

        // get timestamp
        static const Timestamp t0 = mkTimestamp(2010,1,1,0,0,0);
        static const Timestamp timeBinSizeInSeconds = timeBinSizeInHours * 60 * 60; // this is one day unit
        EventTime t = static_cast<EventTime>((point.timestamp - t0)/timeBinSizeInSeconds);

        // add timestamp to stree
        stree.add(t);

        if ((hitS.getNumAdds() % showProgressStep) == 0)
        {
            showProgress();
            // std::clog << memory_util::MemInfo::get();
            // std::clog << hitS.report;
        }

        // std::cout << "hitS: " << hitS.getNumAdds() << std::endl;

    }

    // last report with totals
    showProgress();
    // std::clog << memory_util::MemInfo::get();
    // std::clog << hitS.report;

    // computing statistics
    Stats<QuadTreeTS> stats;
    stats.initialize(hitS);
    stats.dumpReport(std::clog);

    // dump report on timeseries
    TypeDim1::dump_ftlist(std::clog);
    TypeDim2::dump_ftlist(std::clog);
    EventTimeSeries::dump_tslist(std::clog);



    //
    if (output_filename.size() > 0)
    {
        serialization::Schema schema;
        serialization::CoreSchema::initCoreSchema(schema);

        serialization::registerTimeSeriesToSchema<Int, Int>(schema);
        serialization::registerFlatTreeToSchema<EventTimeSeries>(schema);
        serialization::registerFlatTreeToSchema<HTree>(schema);
        serialization::registerQuadTreeToSchema<LevelBits,WTree>(schema);
        serialization::registerSTreeToSchema<STreeTypes, Int>(schema);

        serialization::Serializer serializer(schema, output_filename);

        serializer.scheduleSerialization(serialization::Pointer<serialization::Schema>(&schema));
        serializer.run();

        serializer.scheduleSerialization(serialization::Pointer<STreeTS>(&stree));
        serializer.run();
    }



    // FIXME reimplement this

//    TimeSeriesStatisticsVisitor<LevelBits> statsVisitor;

//    { // run a count session
//        TimeSeriesStatistics::CountSessionRAII countSession(
//            statsVisitor.stats);

//        // get statistics of levels and node content storage
//        hitS.visitSubnodes(Address<LevelBits>(), -1, statsVisitor);

//    } // guarded call to reset and consolidate

//    statsVisitor.stats.dumpReport(std::clog);
//    statsVisitor.stats.histogram.dumpReport(std::clog);


//    // print time series of node 0
//    hitS.root->getContent()->dump(std::clog);

}


//
// main
//

int main(int argc, char** argv)
{
    //
    Timestamp timeBinSizeInHours = 0;

    Level     quadTreeLevels = L25;

    std::string output_filename;

    for (int i=1;i<argc;i++)
    {
        std::string st_i(argv[i]);
        std::string st_upper_i = st_i;
        boost::to_upper(st_upper_i);

        int n = st_i.size();

        std::string prefix = st_upper_i.substr(0,2);
        std::string suffix = st_i.substr(2,n-2);

//        cout << "prefix: " << prefix << std::endl;
//        cout << "suffix: " << suffix << std::endl;

        // -L25 indicates 25 divisions (26 levels on the spatial quadtree)
        if (prefix.compare("-L") == 0)
            quadTreeLevels = static_cast<Level>(atoi(suffix.c_str()));

        // -H1 timebin size in hours
        if (prefix.compare("-H") == 0)
            timeBinSizeInHours = atoi(suffix.c_str());

        // -O for filename
        if (prefix.compare("-O") == 0)
            output_filename = suffix;
    }

    // default
    if (timeBinSizeInHours == 0)
        timeBinSizeInHours = 1;

//    cout << "timeBinSizeInHours: " << timeBinSizeInHours << std::endl;
//    cout << "quadTreeLevels:     " << quadTreeLevels << std::endl;

    switch (quadTreeLevels)
    {
    case L05: worker<L05>(timeBinSizeInHours, output_filename); break;
    case L10: worker<L10>(timeBinSizeInHours, output_filename); break;
    case L15: worker<L15>(timeBinSizeInHours, output_filename); break;
    case L20: worker<L20>(timeBinSizeInHours, output_filename); break;
    case L25: worker<L25>(timeBinSizeInHours, output_filename); break;
    default:  worker<L25>(timeBinSizeInHours, output_filename); break;
    }

}
