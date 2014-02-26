#include <iostream>
#include <random>
#include <chrono>
#include <cmath>
#include <sstream>
#include <iomanip>

#include "QuadTree.hh"

using namespace quadtree;

typedef uint32_t Int;
typedef std::uniform_int_distribution<Coordinate> CoordinateRandomVariable;

//
// AddressRandomVariable
//

struct CoordinatePrefix
{
    Int        numBits;
    Coordinate prefix;
    CoordinatePrefix(Int numBits, Coordinate prefix):
        numBits(numBits), prefix(prefix)
    {
        int k = 31 - numBits;
        Coordinate mask = (Coordinate) (((0xFFFFFFFFU) << k) >> k);
        prefix = prefix & mask;
    }
};

template<BitSize N>
struct AddressRandomVariable
{
    AddressRandomVariable(Level max_zoom, Level resolution, CoordinatePrefix xPrefix, CoordinatePrefix yPrefix);
    ~AddressRandomVariable();
    Address<N> sampleOne();
private:
    Level                        max_zoom;      // 17 = max zoom levels 0..17
    Level                        resolution;    // 8 bits (in each dimension)
    std::mt19937                 rng;           // mersenne random number generator
    CoordinateRandomVariable    *coordRV;       // coordinate random variable
    CoordinatePrefix             xPrefix;
    CoordinatePrefix             yPrefix;
};

template<BitSize N>
AddressRandomVariable<N>::AddressRandomVariable(Level max_zoom, Level resolution,
                                             CoordinatePrefix xPrefix, CoordinatePrefix yPrefix):
    max_zoom(max_zoom),
    resolution(resolution),
    xPrefix(xPrefix),
    yPrefix(yPrefix)
{
    rng.seed(1); // the sample sequence is deterministic
    coordRV = new CoordinateRandomVariable(0, 1 << (max_zoom + resolution));
}

template<BitSize N>
AddressRandomVariable<N>::~AddressRandomVariable()
{
    delete coordRV;
}

template<BitSize N>
Address<N> AddressRandomVariable<N>::sampleOne()
{
    // first bit needs to be 0 always (level 0 is only x=0, y=0)
    Coordinate x = (*coordRV)(rng);
    Coordinate y = (*coordRV)(rng);

    Coordinate xmask = (0xFFFFFFFFU >> xPrefix.numBits);
    Coordinate ymask = (0xFFFFFFFFU >> yPrefix.numBits);

    x = (x & xmask) | xPrefix.prefix;
    y = (y & ymask) | yPrefix.prefix;

    // \Note there might be garbage on the most significant bits
    // of this address after the max_zoom + resolution bit
    Address<N> addr(x, y, max_zoom + resolution);
    addr.eraseHigherBits();
    return addr;
}


//
// Stopwatch
//

typedef int Milliseconds;
struct Stopwatch
{
    Stopwatch()
    {}
    void start()
    {
        t0 = t1 = std::chrono::system_clock::now();
    }
    void stop()
    {
        t1 = std::chrono::system_clock::now();
    }
    Milliseconds time()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>
            (t1-t0).count();
    }

    std::chrono::time_point<std::chrono::system_clock> t0, t1;
};



//
// StartStop: RAII
//

struct StartStop
{
    StartStop(Stopwatch &stopwatch):
    stopwatch(stopwatch)
    {
        stopwatch.start();
    }

    ~StartStop()
    {
        stopwatch.stop();
    }
    Stopwatch &stopwatch;
};


template <BitSize N>
std::ostream& operator<<(std::ostream &os, const Address<N>& addr)
{
    os << "Addr[x: "  << addr.x
       << ", y: "     << addr.y
       << ", level: " << addr.level
       << "]";
    return os;
}

template <BitSize N>
struct Visitor
{
    static Count count;
    static void visit(Address<N> addr)
    {
        // if ((count++ % 10000) == 0)
        //     std::clog << addr << std::endl;
    }
};


template<BitSize N>
Count Visitor<N>::count = 0;


typedef uint32_t Timestamp;

struct Event
{
    double x;
    double y;
    Timestamp  t;
};

template <BitSize N>
struct TimeSeries
{
    // we can keep the address here if we like
    TimeSeries()
    {}

    void setAddress(Address<N> &address)
    {
        this->address = address;
    }

    void add(Event &e)
    {
        // insert event info in the right order
    }

    Address<N> address;
};

template <class T>
inline std::string str(const T& t)
{
    std::stringstream ss;
    ss << std::setprecision(1) << std::fixed << t ;
    return ss.str();
}

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

int main()
{
    // S is the set of all possible spatial bins.
    // |S| = 2^(max_level + resolution)

    // hitS is a quadtree that stores the of S where there
    // is at least one event in the MarkThisSpot dataset

    int kw = 4;

    const BitSize LevelBits = 17 + 8; // 17 zoom subdivisions + 8 fragment subdivisions

    char sep = '|';
    std::clog
        << sep
        << fl("k",kw)          << sep
        << fl("Add Calls")     << sep // number of add calls
        << fl("Num Nodes NN")  << sep // num nodes in quad tree
        << fl("Num Leaves NL") << sep //
        << fl("NN/NL")         << sep //
        << fl("Mem. Size (MB)")     << sep
        << fl("Time Add (ms)")      << sep
        << fl("Time Find (ms)")     << sep << std::endl;

    for (int k=0;k<6;k++)
    {
        QuadTree<LevelBits, TimeSeries<LevelBits>, Event> hitS;

        //
        CoordinatePrefix xPrefix = CoordinatePrefix(24,0);
        CoordinatePrefix yPrefix = CoordinatePrefix(24,0);
        AddressRandomVariable<LevelBits> addressRandomVariable(17,8,xPrefix,yPrefix);

        // number of points
        Int N = static_cast<Int>(pow(10,k));

        //
        Stopwatch stopwatch;
        Milliseconds timeAddAll;
        Milliseconds timeFindAll;

        { // add N random addresses into QuadTree
            StartStop ss(stopwatch);

            for (Int i=0;i<N;i++)
            {
                Address<LevelBits> addr = addressRandomVariable.sampleOne();
                // if ((i % 100000) == 0)
                //     std::clog << addr << std::endl;
                Event dummy;
                hitS.add(addr, dummy);
            }
        }
        // report
        timeAddAll = stopwatch.time();

        { // add N random addresses into QuadTree

            StartStop ss(stopwatch);

            hitS.visitSubnodes<Visitor<LevelBits>>(Address<LevelBits>(),25);

        }
        // report
        timeFindAll = stopwatch.time();


        // report
        std::clog
            << sep
            << fr(str(k),kw)                  << sep
            << fr(str(N))                     << sep // number of add calls
            << fr(str(hitS.getNumNodes()))    << sep // num nodes in quad tree
            << fr(str(hitS.getNumLeaves()))   << sep // num nodes in quad tree
            << fr(str((double)hitS.getNumNodes()/(double)hitS.getNumLeaves())) << sep // num nodes in quad tree
            << fr(str(hitS.getMemoryUsage()/(double)(1<<20))) << sep // num nodes in quad tree
            << fr(str(timeAddAll))            << sep // time to add all
            << fr(str(timeFindAll))           << sep << std::endl; // time to find all

    }
}
