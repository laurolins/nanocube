#pragma once

#include <cstdint>
#include <vector>
#include <map>
#include <unordered_map>
#include <sstream>
#include <iomanip>

#include <Util.hh>

#ifdef OPTIMIZE_FOR_SPEED
#define TIMESERIES_VECTOR
#endif

#ifndef TIMESERIES_VECTOR
#include <small_vector.hh>
#endif

namespace timeseries {

typedef int32_t  Level; // this goes up 25 (17 zooms + 8 resolution)
typedef uint32_t Coordinate;
typedef uint32_t ChildIndex;
typedef uint64_t Count;

//
typedef uint8_t  BitSize;
typedef uint8_t  BitIndex; // this goes up 25 (17 zooms + 8 resolution)

typedef uint64_t Timestamp;
typedef uint32_t NumBins;

//-----------------------------------------------------------------------------
// TimeSeries
//-----------------------------------------------------------------------------

template<typename Entry>
struct TimeSeries
{

    typedef Entry                     EntryType;
    typedef typename Entry::TimeType  TimeType;
    typedef typename Entry::CountType CountType;

#if 0
public: // static services
    static TimeSeries* create(); // use this as a factory
    static void dump_tslist(std::ostream &os);

    static Count mem();
    static Count num();

    // data
    static std::vector<TimeSeries*> tslist;
#endif

public:
    // explicit TimeSeries(Key &key);
    // void setKey(Key &key);
    // Key key;

    inline Count add(TimeType timeStamp, uint64_t weight);

    TimeSeries* makeLazyCopy() const;

    Count getMemoryUsage() const;

    CountType getWindowTotal(TimeType a, TimeType b) const;

    void dump(std::ostream &os) const;

public: // change back to public because of the ObjectFactory deseralization need

    TimeSeries();

    ~TimeSeries();

    static void* operator new(size_t size);
    static void  operator delete(void *p);

    static uint64_t count_new;
    static uint64_t count_delete;
    static uint64_t count_used_bins;
    static uint64_t count_num_adds;

public: // attributes


    // optimize for TimeSeries of element

#ifndef TIMESERIES_VECTOR
    small_vector::small_vector<Entry> entries;
#else
    std::vector<EntryType> entries;
#endif
};

//-----------------------------------------------------------------------------
// TimeSeriesStatistics
//-----------------------------------------------------------------------------

struct TimeSeriesStatistics
{
    struct CountSessionRAII {
        CountSessionRAII(TimeSeriesStatistics &timeSeriesStatistics);
        ~CountSessionRAII();

        TimeSeriesStatistics &timeSeriesStatistics;
    };


    struct LevelInfo {
        Level level;
        Count numNodes;
        Count memoryUsage;
        LevelInfo();
    };

    TimeSeriesStatistics();

    void addLevelNodeInfo(Level level, Count memoryUsage);
    void reset();
    void consolidate();

    void dumpReport(std::ostream &os);

    std::unordered_map<Level, LevelInfo> map;
    Count memoryUsage;
    Count numNodes;

    datatiles::util::Histogram<NumBins> histogram;
};



//-----------------------------------------------------------------------------
// TimeSeries Implementation of Templates
//-----------------------------------------------------------------------------

#if 0
// reserve space for this
template<typename Entry>
std::vector<TimeSeries<Entry>*> TimeSeries<Entry>::tslist;
#endif

//-----------------------------------------------------------------------------
// TimeSeries
//-----------------------------------------------------------------------------

template<typename Entry>
uint64_t TimeSeries<Entry>::count_new = 0;

template<typename Entry>
uint64_t TimeSeries<Entry>::count_delete = 0;

template<typename Entry>
uint64_t TimeSeries<Entry>::count_used_bins = 0;

template<typename Entry>
uint64_t TimeSeries<Entry>::count_num_adds = 0;

template<typename Entry>
void* TimeSeries<Entry>::operator new(size_t size) {
    count_new++;
    return ::operator new(size);
}

template<typename Entry>
void TimeSeries<Entry>::operator delete(void *p) {
    count_delete++;
    ::operator delete(p);
}

// we can keep the key here if we like
template<typename Entry>
TimeSeries<Entry>::TimeSeries()
{
    // In the ~8M points MTS dataset this
    // extra entry of (16 bytes) had an effect of
    // adding the timeseries total storage from
    // 3243.6 MB to 3996 MB: ~ 753 MB.
    // we should avoid this extra storage.
    // entries.push_back(Entry(0, 0));
    //std::cerr << "TimeSeries " << this << std::endl;

}

template<typename Entry>
TimeSeries<Entry>::~TimeSeries()
{
    //std::cerr << "~TimeSeries " << this << std::endl;
}

template<typename Entry>
inline Count // return increase on the actual used memory
TimeSeries<Entry>::add(TimeType time, uint64_t weight)
{
    // std::cout << "TimeSeries<Entry>::add( " << time << " ) " << std::endl;
#ifdef COLLECT_MEMUSAGE
    count_num_adds++; // not thread safe (use atomic later)
#endif
    // assuming we are adding in order for now
    if (entries.size() == 0)
    {
#ifdef COLLECT_MEMUSAGE
        count_used_bins++; // not thread safe (use atomic later)
#endif
        entries.push_back(EntryType(time, weight));
        return sizeof(EntryType);
    }
    else if (entries.back().time() != time)
    {
#ifdef COLLECT_MEMUSAGE
        count_used_bins++; // not thread safe (use atomic later)
#endif
        entries.push_back(EntryType(time, entries.back().count() + weight));
        return sizeof(EntryType);
    }
    else
    {
        entries.back().increment_count(weight);
        return 0;
    }
}

template<typename Entry>
TimeSeries<Entry>*
TimeSeries<Entry>::makeLazyCopy() const
{
    TimeSeries<Entry> *copy = new TimeSeries<Entry>();

    count_used_bins += entries.size(); // not thread safe (use atomic later)

    copy->entries.resize(entries.size());
    std::copy(entries.begin(), entries.end(), copy->entries.begin());
    return copy;
}

template<typename Entry>
Count
TimeSeries<Entry>::getMemoryUsage() const
{
    // Note: counting with entries.size not entries.capacity;
    return sizeof(TimeSeries<Entry>) +
            sizeof(typename TimeSeries<Entry>::EntryType) * entries.size();
}


//
// Interval is [a, b)
//    closed on a and open on b.
//
template<typename Entry>
typename Entry::CountType
TimeSeries<Entry>::getWindowTotal(TimeType a, TimeType b) const
{
    EntryType ea(a,0);
    auto it_a = std::lower_bound(entries.begin(), entries.end(), ea);
    if (it_a == entries.end()) {
        return 0;
    }
    CountType count_a = (it_a == entries.begin() ? 0 : (it_a-1)->count());


    EntryType eb(b,0);
    auto it_b = std::lower_bound(entries.begin(), entries.end(), eb);
    if (it_b == entries.begin()) {
        return 0;
    }
    CountType count_b = (it_b-1)->count();
    return count_b - count_a;
}

template<typename Entry>
void TimeSeries<Entry>::dump(std::ostream &os) const
{
    os << "Time Series with " << entries.size() << "  entries." << std::endl;
    os << "[";
    for (auto it=entries.begin();it!=entries.end();it++)
    {
        os << "(" << it->time << "," << it->count << "), ";
    }
    os << "]";
    os << std::endl;
}

template<typename Entry>
std::ostream& operator<<(std::ostream &o,
                         const TimeSeries<Entry>& ts)
{
    o << "[timeseries: " << ts.getMemoryUsage() << "] ";
    return o;
}



} // end namespace timeseries
