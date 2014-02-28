#include <chrono>
#include <string>
#include <iostream>
#include <initializer_list>
#include <vector>
#include <deque>
#include <thread>
#include <mutex>

#include "signal.hh"

namespace timer {
    
    using Milliseconds   = std::chrono::milliseconds;
    using Timestamp = std::chrono::high_resolution_clock::time_point;
    
    using TickCallback = std::function<void()>;
    
    
    //------------------------------------------------------------------
    // FrameRateMeter
    //------------------------------------------------------------------
    
    struct Timer {
        
        Timer(Milliseconds tick_delay);
        
        ~Timer();
        
        void run();
        void stop();
        
        void subscribe(TickCallback callback);
        
    public: // data members
        Milliseconds                tick_delay;
        bool                        stop_flag { false };
        std::thread                 thread;
        sig::Signal<>               on_tick;
        
        std::vector<TickCallback>   to_subscribe;

        std::mutex                  mutex;
    };

//    namespace io {
//        std::ostream& operator<<(std::ostream &os, const Timer &timer);
//    }
    
    //
    // Usage:
    // {
    //    timer::Timer timer(1000}; // sleep for one second and tick
    //
    //    auto f = []() {
    //        std::cout << "tick" << std::endl;
    //    }
    //
    //    start;
    //
    // }
    //
    
    
} // fpsmeter namespace
