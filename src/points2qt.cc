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

#include <datatiles/QuadTree.hh>

#include <datatiles/QuadTreeUtil.hh>

#include <datatiles/MercatorProjection.hh>

using namespace std;

using quadtree::Coordinate;
using quadtree::Count;
using quadtree::BitSize;
using quadtree::QuadTree;
using quadtree::Address;
using quadtree::Node;
using quadtree::Stats; // from QuadTreeUtil

typedef uint32_t Int;

typedef float RealCoordinate;

//
// Timestamp
//

typedef uint64_t Timestamp;

Timestamp mkTimestamp(string st, string format="%Y-%m-%d %H:%M:%S")
{
    struct tm	tm = {0};
    strptime(st.c_str(), format.c_str(), &tm);
    return (Timestamp) mktime(&tm);
}

Timestamp mkTimestamp(int year, int month, int day, int hour, int min, int sec)
{
    struct tm timeinfo = {0};
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

std::string fl(std::string st, int n=14)
{
    int l = st.size();
    if (l > n)
        return st;
    return st + std::string(n-l,' ');
}

std::string fr(std::string st, int n=14)
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


const BitSize LevelBits = 25;
typedef quadtree::QuadTree<LevelBits, NullContent> QuadTreeTS;


//
typedef typename QuadTreeTS::AddressType QAddr;
typedef typename QuadTreeTS::NodeType    QNode;



template<typename Point>
struct NullPolicy
{
    // Signal that a node was added to a QuadTree at "address" while
    // in the process of adding "targetAddress" (targetAddress will
    // eventually be equal to address in this call)
    void newNode(QNode *node,
                 QAddr &address,
                 QAddr &)
    {}

    // point was added
    void addPoint(Point                        &point,
                  QNode                *node,
                  QAddr                   &,
                  QAddr                   &)
    {}
};


//
// main
//

int main()
{
    // reopen stdin as a binary stream
    stdin = freopen(0, "rb", stdin);

    const BitSize LevelBits = 25;
    QuadTree<LevelBits, NullContent> hitS;

    //
    Stopwatch    stopwatch;
    Milliseconds timeAddAll;

    // show header of report
    char sep = '|';
    std::clog
        << sep
        << fl("Add Calls")     << sep // number of add calls
        << fl("Num Nodes NN")  << sep // num nodes in quad tree
        << fl("Num Leaves NL") << sep //
        << fl("NN/NL")         << sep //
        << fl("Mem. Size (MB)")     << sep
        << fl("Time Add (ms)")      << sep << std::endl;


    auto showProgress = [&] ()
    {
        // report
        std::clog
        << sep
        << fr(str(hitS.getNumAdds()))     << sep // number of add calls
        << fr(str(hitS.getNumNodes()))    << sep // num nodes in quad tree
        << fr(str(hitS.getNumLeaves()))   << sep // num nodes in quad tree
        << fr(str((double)hitS.getNumNodes()/(double)hitS.getNumLeaves())) << sep // num nodes in quad tree
        << fr(str(hitS.getMemoryUsage()/(double)(1<<20))) << sep // num nodes in quad tree
        << fr(str(stopwatch.time()))      << sep << std::endl; // time to add all
    };

    Count showProgressStep = 1000000;

    Count count = 0;
    Count countProblems = 0;

    NullPolicy<int> nullPolicy;

    Point point;

    while (fread((void*) &point, sizeof(Point), 1, stdin))
    {
        count++;

        // if ((count % 1) == 0)
        // {
        //     std::clog << "Reading point: " << count << " problems: " << countProblems << " adds: " << hitS.getNumAdds() << std::endl;
        //     std::clog << "   point lat, long: " << point.latitude << ", " << point.longitude << std::endl;

        //     // unsigned char* ptr = (unsigned char*) &point;
        //     // fprintf(stderr,"\n------------> RECEIVED %3ld:   ", count);
        //     // for (int i=0;i<sizeof(Point);i++)
        //     //     fprintf(stderr,"%x ",ptr[i]);
        //     // fprintf(stderr,"\n\n");

        // }

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

        int point = 0;

        QAddr addr((Coordinate) tx, (Coordinate) ty, LevelBits);
        hitS.add(addr, point, nullPolicy);

        if ((hitS.getNumAdds() % showProgressStep) == 0)
            showProgress();

        // std::cout << "hitS: " << hitS.getNumAdds() << std::endl;

    }

    // last report with totals
    showProgress();

    // computing statistics
    Stats<QuadTreeTS> stats;
    stats.initialize(hitS);
    stats.dumpReport(std::clog);

}


