#pragma once

#include <cmath>
#include <ctime>
#include <chrono>
#include <string>

struct TimeBinFunction;

//-----------------------------------------------------------------------------
// HistogramSchema
//-----------------------------------------------------------------------------

struct HistogramSchema {

    HistogramSchema();
    std::time_t getTimeOfBucketCenter(int index) const;
    std::time_t getStartTimeOfBucket(int index) const;

public:

    const TimeBinFunction *time_bin_function;

    int first;
    int incr; // [first,first + incr), [first+incr, first + 2 x incr) ... [first+ (count-1) x incr, first + count x incr)
    int count;
public:
};

//-----------------------------------------------------------------------------
// TimeBinFunction
//-----------------------------------------------------------------------------

struct TimeBinFunction {

    TimeBinFunction();

    TimeBinFunction(std::string spec);

    TimeBinFunction(
        std::chrono::system_clock::time_point reference_time,
        std::chrono::seconds                  bin_size);

    void init(
        std::chrono::system_clock::time_point reference_time,
        std::chrono::seconds                  bin_size);

    void init(std::string);

    std::string getSpecificationString() const;

    int getBin(std::time_t t) const;
    int getBin(std::string t) const;

    HistogramSchema getHistogramSchema(time_t t0, time_t t1, int target_bins) const;

    std::time_t getTimeOfFractionalBin(float fractional_bin) const;

    static std::time_t getLocalTime(int year, int month, int day, int hour, int min, int sec, int isdst);

public:

    std::chrono::system_clock::time_point reference_time;
    std::chrono::seconds                  bin_size;
};


