#include <chrono>
#include "Stopwatch.hh"

namespace stopwatch {

//-----------------------------------------------------------------------------
// Stopwatch
//-----------------------------------------------------------------------------

Stopwatch::Stopwatch():
    started(false)
{}

void Stopwatch::start()
{
    started = true;
    t0 = t1 = std::chrono::system_clock::now();
}

void Stopwatch::stop()
{
    started = false;
    t1 = std::chrono::system_clock::now();
}

Milliseconds Stopwatch::time()
{
    std::chrono::time_point<std::chrono::system_clock> aux_t1 = started ? std::chrono::system_clock::now() : t1;
    return static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>
            (aux_t1-t0).count());
}

int Stopwatch::timeInSeconds()
{
    std::chrono::time_point<std::chrono::system_clock> aux_t1 = started ? std::chrono::system_clock::now() : t1;
    return static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>
            (aux_t1-t0).count());
}

//-----------------------------------------------------------------------------
// StartStop: RAII
//-----------------------------------------------------------------------------

StartStop::StartStop(Stopwatch &stopwatch, Milliseconds &result):
    stopwatch(stopwatch),
    result(result)
{
    stopwatch.start();
}

StartStop::~StartStop()
{
    stopwatch.stop();
    result = stopwatch.time();
}

}
