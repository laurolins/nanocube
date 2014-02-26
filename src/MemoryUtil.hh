#pragma once

#include <iostream>
#include <unistd.h>

#include <fstream>

#include <chrono>
#include <thread>

#include "Stopwatch.hh"

namespace memory_util
{

//-----------------------------------------------------------------------------
// MemInfo
//-----------------------------------------------------------------------------

struct MemInfo
{
    static MemInfo get();

    size_t res_MB() const;

    size_t res_KB() const;

    size_t res_B() const;

    size_t virt_MB() const;

    size_t virt_KB() const;

    size_t virt_B() const;

private:
    MemInfo();


private:
    size_t res;
    size_t virt;

};

std::ostream& operator<<(std::ostream &os, const MemInfo &memInfo);

//-----------------------------------------------------------------------------
// MemTracker
//-----------------------------------------------------------------------------

template <typename Progress>
struct MemTracker
{
    void run();

    MemTracker(Progress p0, std::string filename);
    std::string filename;
    Progress progress;
};

template <typename Progress>
void MemTracker<Progress>::run()
{
    stopwatch::Stopwatch    w;
    stopwatch::Milliseconds t;

    std::chrono::milliseconds sleep_interval( 1000 );

    w.start();

    std::ofstream os(filename);

    std::string sep = "\t";
    os << "time" << sep << "res" << sep << "virt" << sep << "progress" << std::endl;

    while (true)
    {
        MemInfo memInfo = MemInfo::get();
        t = w.time();
        os << t << sep
           << memInfo.res_MB()  << sep
           << memInfo.virt_MB() << sep
           << progress << std::endl;

        // sleep for 1 sec.
        std::this_thread::sleep_for(sleep_interval);
    }

}

template <typename Progress>
MemTracker<Progress>::MemTracker(Progress p0, std::string filename):
    progress(p0),
    filename(filename)
{
    // leak!
    // std::thread *t = new std::thread(&MemTracker<Progress>::run, this);
}


}



#if 0
int main(){

    int tSize = 0, resident = 0, share = 0;
    ifstream buffer("/proc/self/statm");
    buffer >> tSize >> resident >> share;
    buffer.close();

    long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024; // in case x86-64 is configured to use 2MB pages
    double rss = resident * page_size_kb;
    cout << "RSS - " << rss << " kB\n";

    double shared_mem = share * page_size_kb;
    cout << "Shared Memory - " << shared_mem << " kB\n";

    cout << "Private Memory - " << rss - shared_mem << "kB\n";
    return 0;
}
#endif
