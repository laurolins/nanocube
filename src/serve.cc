#include <iostream>
#include <random>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <csignal>

#include <set>
#include <map>
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
#include <algorithm>

#include <Stopwatch.hh>
#include <TimeSeries.hh>
#include <QuadTree.hh>
#include <QuadTreeUtil.hh>
#include <MercatorProjection.hh>
#include "mongoose.h"

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

using namespace std;

/******************************************************************************/
// mongoose wrapper

typedef void (*Handler)(const vector<string> &params, struct mg_connection *conn);

map<string, Handler> handlers;

void error(const string &str, struct mg_connection *conn)
{
    mg_printf(conn, "HTTP/1.1 501 Not Implemented\r\nContent-Type: text/plain\r\nContent-Length: %d\r\n\r\n%s",
              (int)str.size(), str.c_str());
}

void *mg_callback(enum mg_event event, struct mg_connection *conn)
{
    if (event == MG_NEW_REQUEST) {
        const struct mg_request_info *request_info = mg_get_request_info(conn);
        string uri = string(request_info->uri);
        vector<string> strs;
        boost::split(strs, uri, boost::is_any_of("/"));
        if (!strs.size()) {
            error(string("bad URL: ") + uri, conn);
            return (void*)"";
        }
        strs.erase(strs.begin());
        if (!strs.size()) {
            error(string("bad URL: ") + uri, conn);
            return (void*)"";
        }
        if (handlers.find(strs[0]) == handlers.end()) {
            error(string("no handler for: ") + uri + string(" (request key: ") + strs[0] + string(")"), conn);
            return (void*)"";
        }
        handlers[strs[0]](strs, conn);
        return (void*)"";
    }
    return NULL;
}

////////////////////////////////////////////////////////////////////////////////

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
// Small allocation TimeSeries
//

typedef uint32_t EventCount;
typedef uint32_t EventTime;
typedef timeseries::TimeSeries<EventTime, EventCount> TS;
typedef timeseries::Entry<EventTime, EventCount> TSEntry;

const BitSize LevelBits = 25;
typedef QuadTree<LevelBits, TS> Tree;

//
typedef typename Tree::AddressType QAddr;
typedef typename Tree::NodeType    QNode;




/******************************************************************************/
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

//
// TimeSeries add and visit policy
//

template<BitSize N>
struct TimeSeriesAddPolicy
{


    // Signal that a node was added to a QuadTree at "address" while
    // in the process of adding "targetAddress" (targetAddress will
    // eventually be equal to address in this call)
    void newNode(QNode *node,
                 QAddr &,
                 QAddr &)
    {
        // add to all nodes found
        node->setProperContent(TS::create());
        memUsage += sizeof(TS);
    }

    // point was added
    void addPoint(Point                 &point,
                  QNode *node,
                  QAddr            &,
                  QAddr            &)
    {
        TS* ts = node->getContent();

        static const Timestamp t0 = mkTimestamp(2010,1,1,0,0,0);
        static const Timestamp timeBinSizeInSeconds = 24 * 60 * 60; // this is one day unit

        //
        EventTime t = static_cast<EventTime>((point.timestamp - t0)/timeBinSizeInSeconds);

        // truncate to integer unit of seconds

        memUsage += ts->add(t);

    }

    TimeSeriesAddPolicy():
        memUsage(0)
    {}

    Count memUsage;

    TimeSeriesStatistics stats;

};


/******************************************************************************/
// on SIGINT, set flag that's checked by loop
int has_interrupted = 0;
void sigint_handler(int s) {
    std::cerr << "Caught signal " << s << std::endl;
    has_interrupted = 1;
}
/******************************************************************************/

/******************************************************************************/
// report tile

inline bool compare_entry_time(const TSEntry &e0,
                               const TSEntry &e1)
{
    return e0.time < e1.time;
}



struct ReportTile
{
    Timestamp t0, t1;

    explicit ReportTile(Timestamp t0, Timestamp t1):
        t0(t0), t1(t1)
    {}

    void visit(QNode *node,
               const QAddr &addr)
    {
        auto ts = node->getContent();
        auto &e = ts->entries;
        TSEntry et0(t0, 0), et1(t1, 0);
        auto it0 = std::lower_bound(e.begin(), e.end(), et0, compare_entry_time) - 1;
        auto it1 = std::lower_bound(e.begin(), e.end(), et1, compare_entry_time) - 1;
        int count = it1->count - it0->count;
        int local_x = (addr.x >> (LevelBits - addr.level)) & 255, local_y = (addr.y >> (LevelBits - addr.level)) & 255;     // 255 because (1 << 8) - 1 = 255, and images are 1 << 8 wide
        if (count > 0) {
            x.push_back(local_x);
            y.push_back(local_y);
            counts.push_back(count);
        }
    }

    vector<int> x, y, counts;

};










Tree *hitSp;
void json_tile(const vector<string> &args, struct mg_connection *conn)
{
    if (args.size() != 6) {
        error("tile expects five parameters; zoom, x, y, t0 and t1", conn);
        return;
    }
    int x = -1, y = -1, zoom = -1;
    const string &d0 = args[4], &d1 = args[5];
    zoom = atoi(args[1].c_str());
    x    = atoi(args[2].c_str());
    y    = atoi(args[3].c_str());
    if (x < 0 || y < 0 || zoom < 0) {
        error(string("tile expects non-negative parameters, got ") +
              args[1] + string("/") + args[2] + string("/") + args[3], conn);
        return;
    }

    Timestamp t0 = mkTimestamp(d0, "%Y-%m-%d_%H:%M:%S");
    Timestamp t1 = mkTimestamp(d1, "%Y-%m-%d_%H:%M:%S");


    QAddr base(x << (LevelBits - zoom), y << (LevelBits - zoom), zoom);
    ReportTile tiler(t0, t1);
    hitSp->visitSubnodes(base, zoom+8, tiler);




    stringstream ss;
    ss << "[[";
    for (size_t i=0; i<tiler.x.size(); ++i) {
        ss << (i>0?",":"") << (int)tiler.x[i];
    }
    ss << "],[";
    for (size_t i=0; i<tiler.y.size(); ++i) {
        ss << (i>0?",":"") << (int)tiler.y[i];
    }
    ss << "],[";
    for (size_t i=0; i<tiler.counts.size(); ++i) {
        ss << (i>0?",":"") << (int)tiler.counts[i] << ",0,0";
    }
    ss << "]] ";
    string msg = ss.str();
    mg_printf(conn, "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: %d\r\n\r\n%s",
              (int)msg.size(), msg.c_str());
}

//
// main
//

int main(int argc, char **argv)
{
    Tree hitS;
    hitSp = &hitS;

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
            std::cerr
            << sep
            << fr(str(hitS.getNumAdds()))     << sep // number of add calls
            << fr(str(hitS.getNumNodes()))    << sep // num nodes in quad tree
            << fr(str(hitS.getNumLeaves()))   << sep // num nodes in quad tree
            << fr(str((double)hitS.getNumNodes()/(double)hitS.getNumLeaves())) << sep // num nodes in quad tree
            << fr(str(hitS.getMemoryUsage()/(double)(1<<20))) << sep // num nodes in quad tree
            << fr(str(stopwatch.time()))      << sep << std::endl; // time to add all
        };

    Count showProgressStep = 500000;

    Count count = 0;
    Count countProblems = 0;

    Point point;

    TimeSeriesAddPolicy<LevelBits> timeSeriesAddPolicy;

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
        ty = ((1 << LevelBits) - 1) - ty;

        QAddr addr((Coordinate) tx, (Coordinate) ty, LevelBits);


        hitS.add(addr, point, timeSeriesAddPolicy);


        if ((hitS.getNumAdds() % showProgressStep) == 0)
            showProgress();

        // std::cout << "hitS: " << hitS.getNumAdds() << std::endl;

    }
    cerr << "Done!" << endl;

    /**************************************************************************/
    // set http handlers
    handlers[string("json_tile")] = &json_tile;

    /**************************************************************************/
    // set sigint handler to kill server
    struct sigaction action;
    action.sa_handler = sigint_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGINT, &action, NULL);

    /**************************************************************************/
    // init mongoose
    struct mg_context *ctx;
    const char *options[] = {"listening_ports", "29512", NULL};
    ctx = mg_start(&mg_callback, NULL, options);

    while (!has_interrupted)
        sleep(1);

    mg_stop(ctx);
    return 0;

    // string d0, d1;
    // int x, y, z;
    // ifstream in(argv[1]);
    // while (in >> d0 >> d1 >> x >> y >> z) {
    //     Timestamp t0 = mkTimestamp(d0, "%Y-%m-%d_%H:%M:%S");
    //     Timestamp t1 = mkTimestamp(d1, "%Y-%m-%d_%H:%M:%S");
    //     Tree::SubnodeVisitor<CountRange> counter(&hitS);
    //     counter.init(t0, t1);
    //     counter.visit(QAddr(x << (LevelBits - z), y << (LevelBits - z), z), z+8);
    //     std::cout << x << " " << y << " " << z << " " << counter.total_count << std::endl;
    // }
}


