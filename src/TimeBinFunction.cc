#include "TimeBinFunction.hh"

#include <string>
#include <cstring>
#include <algorithm>
#include <sstream>
#include <iostream>

using namespace std;


#include <vector>
#include <string>
using namespace std;

void split(const char *str, const char *separators, std::vector<std::string> &result)
{
    int count = 0;
    while (*str)
    {

//        std::cout << "processing: " << count << ": " << *str << std::endl;

        const char *begin = str;

        while(*str) {

            bool is_separator = false;

            { // check if current char is a separator
                const char *it_sep = separators;
                while(*it_sep) {

//                    std::cout << "   is " << *it_sep << "?" << std::endl;

                    if (*it_sep == *str) {
                        is_separator = true;
                        break;
                    }
                    it_sep++;
                }
            }

            if (is_separator) {
                break;
            }
            else {
                str++;
                count++;
            }
        }

        if (begin < str) {
            std::string token(string(begin, str));
            result.push_back(token);

//            std::cout << "   found word token " << count << std::endl;
//            std::cout << "       " << token << std::endl;
        }

        if (*str) {
            str++;
            count++;
        }
        else {
            break;
        }

    }
}

std::time_t parseDate(std::string spec)
{
    std::vector<std::string> tokens;
    split(spec.c_str(), "-:_", tokens);

    if (tokens.size() < 2)
        throw std::string("no tokens");

    //
    std::tm timeinfo;

    // year
    if (tokens.size() > 0) {
        std::istringstream(tokens[0]) >> timeinfo.tm_year;
        timeinfo.tm_year -= 1900;
    }
    else {
        timeinfo.tm_year = 0;
    }

    // month
    if (tokens.size() > 1) {
        std::istringstream(tokens[1]) >> timeinfo.tm_mon;
        timeinfo.tm_mon -= 1;
    }
    else {
        timeinfo.tm_mon = 0;
    }

    // day
    if (tokens.size() > 2) {
        std::istringstream(tokens[2]) >> timeinfo.tm_mday;
    }
    else {
        timeinfo.tm_mday = 1;
    }

    // hour
    if (tokens.size() > 3) {
        std::istringstream(tokens[3]) >> timeinfo.tm_hour;
    }
    else {
        timeinfo.tm_hour = 0;
    }

    // minute
    if (tokens.size() > 4) {
        std::istringstream(tokens[4]) >> timeinfo.tm_min;
    }
    else {
        timeinfo.tm_min = 0;
    }

    // minute
    if (tokens.size() > 5) {
        std::istringstream(tokens[5]) >> timeinfo.tm_sec;
    }
    else {
        timeinfo.tm_sec = 0;
    }

    timeinfo.tm_wday  = 0;
    timeinfo.tm_yday  = 0;
    timeinfo.tm_isdst = -1;

    // time "t"
    std::time_t t = mktime(&timeinfo);

    return t;
}


//-----------------------------------------------------------------------------
// TimeBinFunction
//-----------------------------------------------------------------------------

TimeBinFunction::TimeBinFunction():
    reference_time(chrono::system_clock::from_time_t(0)),
    bin_size(chrono::seconds(1))
{}

TimeBinFunction::TimeBinFunction(
    chrono::system_clock::time_point reference_time,
    chrono::seconds                  bin_size):
    reference_time(reference_time),
    bin_size(bin_size)
{}

TimeBinFunction::TimeBinFunction(std::string spec)
{
    this->init(spec);
}

void TimeBinFunction::init(
    chrono::system_clock::time_point reference_time,
    chrono::seconds                  bin_size)
{
    this->reference_time = reference_time;
    this->bin_size       = bin_size;
}

void TimeBinFunction::init(std::string spec)
{
    std::vector<std::string> tokens;
    split(spec.c_str(), "-:_", tokens);

    if (tokens.size() < 2)
        throw std::string("no tokens");

    // last number should have a unit: w,d,h,m,s
    std::string bin_size_st = tokens.back();
    char bin_size_unit = bin_size_st.back();
    int  count         = std::stoi(std::string(bin_size_st.begin(),bin_size_st.end()-1));

    int unit_to_seconds = 1;
    switch(bin_size_unit) {
    case 'd':
        unit_to_seconds = 24 * 60 * 60;
        break;
    case 'm':
        unit_to_seconds = 60;
        break;
    case 'h':
        unit_to_seconds = 60 * 60;
        break;
    case 'w':
        unit_to_seconds = 7 * 24 * 60 * 60;
        break;
    case 's':
        unit_to_seconds = 1;
        break;
    default:
        throw std::string("unknown unit");
    }
    int  bin_size_in_seconds = count * unit_to_seconds;

    // erase last token
    tokens.erase(tokens.end()-1);

    //
    std::tm timeinfo;

    // year
    if (tokens.size() > 0) {
        std::istringstream(tokens[0]) >> timeinfo.tm_year;
        timeinfo.tm_year -= 1900;
    }
    else {
        timeinfo.tm_year = 0;
    }

    // month
    if (tokens.size() > 1) {
        std::istringstream(tokens[1]) >> timeinfo.tm_mon;
        timeinfo.tm_mon -= 1;
    }
    else {
        timeinfo.tm_mon = 0;
    }

    // day
    if (tokens.size() > 2) {
        std::istringstream(tokens[2]) >> timeinfo.tm_mday;
    }
    else {
        timeinfo.tm_mday = 1;
    }

    // hour
    if (tokens.size() > 3) {
        std::istringstream(tokens[3]) >> timeinfo.tm_hour;
    }
    else {
        timeinfo.tm_hour = 0;
    }

    // minute
    if (tokens.size() > 4) {
        std::istringstream(tokens[4]) >> timeinfo.tm_min;
    }
    else {
        timeinfo.tm_min = 0;
    }

    // seconds
    if (tokens.size() > 5) {
        std::istringstream(tokens[5]) >> timeinfo.tm_sec;
    }
    else {
        timeinfo.tm_sec = 0;
    }

    timeinfo.tm_wday  = 0;
    timeinfo.tm_yday  = 0;
    timeinfo.tm_isdst = -1;

    // time "t"
    std::time_t t = mktime(&timeinfo);

    this->reference_time = chrono::system_clock::from_time_t(t);
    this->bin_size       = chrono::seconds(bin_size_in_seconds);
}

std::string TimeBinFunction::getSpecificationString() const {
    char        buffer[100];
    std::time_t ref = chrono::system_clock::to_time_t(reference_time);
    std::tm    *timeinfo = localtime(&ref);
    std::strftime(buffer,100,"%Y-%m-%d_%H:%M:%S",timeinfo);
    std::string ref_st(buffer);

    enum {WEEK, DAY, HOUR, MINUTE, SECONDS };
    static const int units[] = {7 * 24 * 60 * 60, 24 * 60 * 60, 60 * 60, 60, 1};

    std::string bin_st;
    if (bin_size.count() % units[WEEK] == 0) {
        bin_st = std::to_string(bin_size.count() / units[WEEK]) + std::string("w");
    } else if (bin_size.count() % units[DAY] == 0) {
        bin_st = std::to_string(bin_size.count() / units[DAY]) + std::string("d");
    } else if (bin_size.count() % units[HOUR] == 0) {
        bin_st = std::to_string(bin_size.count() / units[HOUR]) + std::string("h");
    } else if (bin_size.count() % units[MINUTE] == 0) {
        bin_st = std::to_string(bin_size.count() / units[MINUTE]) + std::string("m");
    } else {
        bin_st = std::to_string(bin_size.count()) + std::string("s");
    }
    return ref_st + std::string("_") + bin_st;
}

int TimeBinFunction::getBin(std::time_t t) const {
    chrono::system_clock::time_point tp = chrono::system_clock::from_time_t(t);
    chrono::seconds delta_t_in_secs =
        chrono::duration_cast<chrono::seconds>(tp - reference_time);

    // bins are closed on the right open on the left
    float bin_index = (float) delta_t_in_secs.count() / bin_size.count();

    return (int) floor(bin_index);
}

int TimeBinFunction::getBin(std::string st) const {
    return getBin(parseDate(st));
}

std::time_t TimeBinFunction::getTimeOfFractionalBin(float fractional_bin) const {
    chrono::duration<float, std::ratio<1>> fractional_seconds_offset(fractional_bin * bin_size.count());

    // chrono::s
    chrono::system_clock::duration   dura = chrono::duration_cast<chrono::system_clock::duration>(fractional_seconds_offset);
    chrono::system_clock::time_point target = reference_time + dura;

    return chrono::system_clock::to_time_t(target);
}

HistogramSchema TimeBinFunction::getHistogramSchema(time_t t0, time_t t1, int target_pixels) const
{
    HistogramSchema result;

    // bin
    // bucket
    // pixel

    // density is measured in pixels / bucket

    int bin0 = getBin(t0);
    int bin1 = getBin(t1);
    if (bin0 > bin1)
        std::swap(bin0, bin1);
    int total_bins_in_range = bin1 - bin0 + 1;

    static const int target_density = 20;
    static const std::vector<int> possible_bucket_sizes { 1, 6, 24, 7*24, 4*7*24, 12*7*24, 24*7*24, 48*7*24, 52*7*24, 104*7*24 }; // unit is bins

    int result_bucket_size = 1;
    for (auto it=possible_bucket_sizes.rbegin(); it != possible_bucket_sizes.rend(); it++) {
        int b = *it;
        float no_bucket = (float) total_bins_in_range / b;
        if (no_bucket * target_density >= target_pixels) {
            result_bucket_size = b;
            break;
        }
    }


    result.first = bin0 - (bin0 % result_bucket_size);
    result.incr  = result_bucket_size;
    result.count = 1 + (int) ceil((float) (bin1 - bin0) /result.incr);
    result.time_bin_function = this;

//}
//    else {
//        result.first = bin0 - (bin0 % 24);
//        result.incr  = 24;
//        result.count = (int) ceil((float) (bin1 - bin0) /result.incr);
//    }

//    result.first = bin0;
//    result.incr  = (bin1 - bin0 + 1) / target_bins;
//    if (result.incr == 0)
//        result.incr = 1;

//    result.count = (int) ceil((float) (bin1 - bin0) /result.incr);
//    result.time_bin_function = this;

//    if (result.count == 0)
//        result.count = 1;

    return result;
}




std::time_t TimeBinFunction::getLocalTime(int year, int month, int day, int hour, int min, int sec, int isdst)
{
    std::tm timeinfo;
    memset(&timeinfo, 0, sizeof(std::tm));
    timeinfo.tm_year   = year	 - 1900;
    timeinfo.tm_mon    = month - 1;
    timeinfo.tm_mday   = day;
    timeinfo.tm_hour   = hour;
    timeinfo.tm_min    = min;
    timeinfo.tm_sec    = sec;
    timeinfo.tm_isdst  = isdst;

    // tm_yday are tm_wday not set, since they are ignored by mktime
    return mktime(&timeinfo);
}

//-----------------------------------------------------------------------------
// HistogramSchema
//-----------------------------------------------------------------------------

HistogramSchema::HistogramSchema():
    time_bin_function(nullptr),
    first(0),
    incr(0),
    count(0)
{}

std::time_t HistogramSchema::getTimeOfBucketCenter(int index) const
{
    if(!time_bin_function)
        throw std::string("ooops");

    float fractional_bin = first + index * incr + incr/2.0;
    return time_bin_function->getTimeOfFractionalBin(fractional_bin);
}


std::time_t HistogramSchema::getStartTimeOfBucket(int index) const
{
    if(!time_bin_function)
        throw std::string("ooops");

    float fractional_bin = first + index * incr;
    return time_bin_function->getTimeOfFractionalBin(fractional_bin);
}





















