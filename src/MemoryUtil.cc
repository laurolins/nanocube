#include "MemoryUtil.hh"
#include "Stopwatch.hh"

#include <iostream>
#include <fstream>
#include <unistd.h>


#ifdef __linux__
# include <sys/sysinfo.h>
#endif

#ifdef __APPLE__
# include <mach/task.h>
# include <mach/mach_init.h>
#endif

#ifdef _WINDOWS
# include <windows.h>
#else
# include <sys/resource.h>
#endif

namespace memory_util
{

//-----------------------------------------------------------------------------
// Output
//-----------------------------------------------------------------------------

/// The amount of memory currently being used by this process, in bytes.
/// By default, returns the full virtual arena, but if resident=true,
/// it will report just the resident set in RAM (if supported on that OS).
static size_t memory_used (bool resident=false)
{
#if defined(__linux__)
    // Ugh, getrusage doesn't work well on Linux.  Try grabbing info
    // directly from the /proc pseudo-filesystem.  Reading from
    // /proc/self/statm gives info on your own process, as one line of
    // numbers that are: virtual mem program size, resident set size,
    // shared pages, text/code, data/stack, library, dirty pages.  The
    // mem sizes should all be multiplied by the page size.
    size_t size = 0;
    FILE *file = fopen("/proc/self/statm", "r");
    if (file) {
        unsigned long vm = 0;
        unsigned long res = 0;
        int filled = fscanf (file, "%lu %lu", &vm, &res);  // Just need the first num: vm size
        fclose (file);

        size = getpagesize() * (size_t) (resident ? res : vm);
    }
    return size;
#endif

#if defined(__APPLE__)
    // Inspired by:
    // http://miknight.blogspot.com/2005/11/resident-set-size-in-mac-os-x.html
    struct task_basic_info t_info;
    mach_msg_type_number_t t_info_count = TASK_BASIC_INFO_COUNT;
    task_info(current_task(), TASK_BASIC_INFO, (task_info_t)&t_info, &t_info_count);
    size_t size = (resident ? t_info.resident_size : t_info.virtual_size);
    return size;
#endif

#if defined(_WINDOWS)
    // According to MSDN...
    PROCESS_MEMORY_COUNTERS counters;
    if (GetProcessMemoryInfo (GetCurrentProcess(), &count, sizeof (count)))
        return count.PagefileUsage;
    else return 0;

#else
    // No idea what platform this is
    return 0;   // Punt
#endif
}

//-----------------------------------------------------------------------------
// MemInfo Impl.
//-----------------------------------------------------------------------------

MemInfo MemInfo::get()
{
    MemInfo result;
    result.res = memory_used(true);
    result.virt = memory_used(false);
    return result;
}

size_t MemInfo::res_MB() const
{ return res/(1<<20); }

size_t MemInfo::res_KB() const
{ return res/(1<<10); }

size_t MemInfo::res_B() const
{ return res; }

size_t MemInfo::virt_MB() const
{ return virt/(1<<20); }

size_t MemInfo::virt_KB() const
{ return virt/(1<<10); }

size_t MemInfo::virt_B() const
{ return virt; }

MemInfo::MemInfo():
    res(0), virt(0)
{}


//-----------------------------------------------------------------------------
// Output
//-----------------------------------------------------------------------------

std::ostream& operator<<(std::ostream &os, const MemInfo &memInfo)
{
    os << "MemInfo (MB.): resident: " << memInfo.res_MB() << "   virtual: " << memInfo.virt_MB() << std::endl;
    return os;
}



//MemTracker::MemTracker(std::string filename)
//{
//    std::thread *t = new std::thread(track, filename);
//}



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
