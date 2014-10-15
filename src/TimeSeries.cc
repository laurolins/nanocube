#include <map>
#include "TimeSeries.hh"
#include <iomanip>

namespace timeseries {


//------------------------------------------------------------------------------------
// TimeSeriesStatistics::LevelInfo
//------------------------------------------------------------------------------------

TimeSeriesStatistics::LevelInfo::LevelInfo():
    level(-1), numNodes(0), memoryUsage(0)
{}

//------------------------------------------------------------------------------------
// TimeSeriesStatistics::CountSessionRAII
//------------------------------------------------------------------------------------

TimeSeriesStatistics::CountSessionRAII::CountSessionRAII(TimeSeriesStatistics &timeSeriesStatistics):
    timeSeriesStatistics(timeSeriesStatistics)
{
    timeSeriesStatistics.reset();
}

TimeSeriesStatistics::CountSessionRAII::~CountSessionRAII()
{
    timeSeriesStatistics.consolidate();
}

//------------------------------------------------------------------------------------
// TimeSeriesStatistics
//------------------------------------------------------------------------------------

TimeSeriesStatistics::TimeSeriesStatistics():
    memoryUsage(0),
    numNodes(0)
{}

void TimeSeriesStatistics::addLevelNodeInfo(Level level, Count memoryUsage)
{
    if (map.find(level) == map.end())
    {
        map[level] = LevelInfo();
        map[level].level = level;
    }
    LevelInfo &levelInfo   = map[level];
    levelInfo.memoryUsage += memoryUsage;
    levelInfo.numNodes    += 1;
}

void TimeSeriesStatistics::reset()
{
    map.clear();
    numNodes    = 0;
    memoryUsage = 0;
}

void TimeSeriesStatistics::consolidate()
{
    for (auto &item: map)
    {
        LevelInfo &info = item.second;
        numNodes    += info.numNodes;
        memoryUsage += info.memoryUsage;
    }
}

namespace {

std::string fl(std::string st, int n=14)
{
    int l = (int) st.size();
    if (l > n)
        return st;
    return st + std::string(n-l,' ');
}

std::string fr(std::string st, int n=14)
{
    int l = (int) st.size();
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

}

void TimeSeriesStatistics::dumpReport(std::ostream &os)
{
    throw std::runtime_error("removed");
    
#if 0
    char sep = '|';


    // Statistics of Content Associated with Nodes of the QuadTree
    os  << "-------------------------------------------------------------------" << std::endl;
    os  << "------------------- TimeSeriesStatistics Report -------------------" << std::endl;
    os  << "-------------------------------------------------------------------" << std::endl;

    // Statistics of Content Associated with Nodes of the QuadTree
    os
            << sep
            << fl("Level")         << sep // number of add calls
            << fl("Num Nodes")     << sep // num nodes in quad tree
            << fl("Num Nodes (%)")     << sep // num nodes in quad tree
            << fl("Memory Usage (MB)")  << sep //
            << fl("Memory Usage (%)") << sep << std::endl;

    Count totalMemoryUsage = memoryUsage;
    Count totalNumNodes    = numNodes;

    for (size_t i=0;i<map.size();i++)
    {
        TimeSeriesStatistics::LevelInfo &levelInfo = map[i];

        os
                << sep
                << fr(str(levelInfo.level))         << sep // number of add calls
                << fr(str(levelInfo.numNodes))     << sep // num nodes in quad tree
                << fr(str(100.0 * (double)levelInfo.numNodes/totalNumNodes)) << sep // num nodes in quad tree
                << fr(str(levelInfo.memoryUsage/(double)(1 <<20)))  << sep //
                << fl(str(100.0 * (double)levelInfo.memoryUsage/totalMemoryUsage)) << sep << std::endl;
    }

    os
            << sep
            << fl("Sum")         << sep // number of add calls
            << fr(str(totalNumNodes))     << sep // num nodes in quad tree
            << fr(str(100.0 * (double)totalNumNodes/totalNumNodes)) << sep // num nodes in quad tree
            << fr(str(totalMemoryUsage/((double) (1 << 20)))) << sep //
            << fl(str(100.0 * (double)totalMemoryUsage/totalMemoryUsage)) << sep << std::endl;
#endif

}

} // end namespace timeseries
