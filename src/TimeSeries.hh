#pragma once

#include <cstdint>
#include <vector>
#include <map>
#include <unordered_map>
#include <sstream>
#include <iomanip>
#include <stdexcept>

#include <algorithm>

#include "Util.hh"

#ifdef OPTIMIZE_FOR_SPEED
#define TIMESERIES_VECTOR
#endif

#ifndef TIMESERIES_VECTOR
#include "small_vector.hh"
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

template <typename Entry, bool Small = true>
struct VectorType {
    using type = small_vector::small_vector<Entry>;
};

template <typename Entry>
struct VectorType<Entry,false> {
    using type = std::vector<Entry>;
};

template<typename Entry>
struct TimeSeries
{

    using EntryType = Entry;

    using VectorType = typename VectorType<Entry, Entry::front_size <= 2>::type;

public:
    // explicit TimeSeries(Key &key);
    // void setKey(Key &key);
    // Key key;

    inline Count add(Entry entry);

    TimeSeries* makeLazyCopy() const;

    Count getMemoryUsage() const;

    TimeSeries* getRoot();

    template <int VariableIndex>
    uint64_t getWindowTotal(Timestamp a, Timestamp b) const;

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

    VectorType entries;

    //    // optimize for TimeSeries of element
//#ifndef TIMESERIES_VECTOR
//    small_vector::small_vector<Entry> entries;
//#else
//    std::vector<EntryType> entries;
//#endif

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
{}

template<typename Entry>
TimeSeries<Entry>::~TimeSeries()
{}

template<typename Entry>
inline Count // return increase on the actual used memory
TimeSeries<Entry>::add(Entry entry)
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
        entries.push_back(entry);
        return sizeof(Entry);
    }
    else
    {
        uint64_t entry_time = entry.template get<0>();
        uint64_t current_entry_time = entries.back().template get<0>();

        // TODO sum the entries starting on the second entry...
        if (current_entry_time < entry_time) {

#ifdef COLLECT_MEMUSAGE
            count_used_bins++; // not thread safe (use atomic later)
#endif

            entry.accum(entries.back());
            entries.push_back(entry);
            return sizeof(Entry);
        }

        else if (current_entry_time == entry_time) {
            entries.back().accum(entry);
            return 0;
        }

        else { // out of order case: SLOW

            auto comp = [](const Entry &e, uint64_t time) -> bool {
                uint64_t e_time = e.template get<0>();
                return e_time < time;
            };

            // binary search
            auto it = std::lower_bound(entries.begin(), entries.end(), entry_time, comp);

            if (it == entries.end()) {
                throw std::runtime_error("Not expected");
            }

            uint64_t insertion_time = (*it).template get<0>();

            Count used_size_inc = 0;

            if (insertion_time != entry_time) {
                // insert new entry on time series
                it = entries.insert(it,entry);

                used_size_inc += sizeof(Entry);

#ifdef COLLECT_MEMUSAGE
                count_used_bins++; // not thread safe (use atomic later)
#endif

                if (it != entries.begin()) {
                    (*it).accum(*(it-1)); // accumulate last
                }
            }
            else {
                (*it).accum(entry);
            }

            // accumulate on suffix of (cumulative) time series
            for (auto it2=it+1;it2!=entries.end();++it2) {
                (*it2).accum(entry);
            }

            return used_size_inc;

        }

    }
//    else
//    {
//        entries.back().count()++;
//        return 0;
//    }
}

template<typename Entry>
TimeSeries<Entry>*
TimeSeries<Entry>::makeLazyCopy() const
{
    // std::cout << "TimeSeries<Entry>::makeLazyCopy() original size: " << entries.size() << std::endl;

    TimeSeries<Entry> *copy = new TimeSeries<Entry>();

    count_used_bins += entries.size(); // not thread safe (use atomic later)

    copy->entries.resize(entries.size());
    std::copy(entries.begin(), entries.end(), copy->entries.begin());

    // std::cout << "   copy size: " << copy->entries.size() << std::endl;

    assert(entries.size() == copy->entries.size());

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

template<typename Entry>
TimeSeries<Entry> *TimeSeries<Entry>::getRoot()
{
    return this;
}

//
// Interval is [a, b)
//    closed on a and open on b.
//

template<typename Entry>
template<int VariableIndex>
uint64_t TimeSeries<Entry>::getWindowTotal(Timestamp a, Timestamp b) const
{
    // EntryType ea(a,0);
    auto cmp = [](const Entry& e, Timestamp tb) -> bool{
        Timestamp ta = e.template get<0>();
        return (ta < tb);
    };

    auto it_a = std::lower_bound(entries.begin(), entries.end(), a, cmp);
    if (it_a == entries.end()) {
        return 0;
    }
    uint64_t count_a = (it_a == entries.begin() ? 0 : (it_a-1)->template get<VariableIndex>());

    auto it_b = std::lower_bound(entries.begin(), entries.end(), b, cmp);
    if (it_b == entries.begin()) {
        return 0;
    }
    uint64_t count_b = (it_b-1)->template get<VariableIndex>();
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
