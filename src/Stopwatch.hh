#pragma once

#include <chrono>

namespace stopwatch {

//
// Stopwatch
//

using Milliseconds = int;
    
struct Stopwatch
{
    Stopwatch();
    void start();
    void stop();
    Milliseconds time();
    int timeInSeconds();


    bool started;
    std::chrono::time_point<std::chrono::system_clock> t0, t1;
};

//
// StartStop: RAII
//

struct StartStop
{
    StartStop(Stopwatch &stopwatch, Milliseconds &result);
    ~StartStop();

    Stopwatch &stopwatch;
    Milliseconds &result;
};


}
