#include <iomanip>
#include <algorithm>

#include "timer.hh"

//-----------------------------------------------------------------------------
// Timer
//-----------------------------------------------------------------------------

timer::Timer::Timer(Milliseconds tick_delay):
    tick_delay(tick_delay)
{
    thread = std::thread(&Timer::run, this);
}

timer::Timer::~Timer()
{
    this->stop();
}

void timer::Timer::stop() {
    if (!stop_flag) {
        stop_flag = true;
        thread.join();
    }
}

void timer::Timer::run() {
    while (!stop_flag) {
        
        if (to_subscribe.size() > 0) {
            std::lock_guard<std::mutex> lock(mutex);
            while (to_subscribe.size()) {
                on_tick.connect(to_subscribe.back());
                to_subscribe.pop_back();
            }
        }
        
        on_tick.trigger();
        std::this_thread::sleep_for(tick_delay);
    }
}

void timer::Timer::subscribe(TickCallback callback) {
    std::lock_guard<std::mutex> lock(mutex);
    to_subscribe.push_back(callback);
}

//std::ostream& timer::io::operator<<(std::ostream &os, const timerMeter &frmeter)
//{
//    bool first = true;
//    os << "fps(";
//    for (auto it=frmeter.time_windows.rbegin();it != frmeter.time_windows.rend();it++) {
//        if (!first) {
//            os << ",";
//        }
//        os << it->length.count();
//        first = false;
//    }
//    os << "s): ";
//    first = true;
//    for (auto it=frmeter.time_windows.rbegin();it != frmeter.time_windows.rend();it++) {
//        if (!first) {
//            os << " ";
//        }
//        os << std::fixed << std::setprecision(2) << it->frame_rate;
//        first = false;
//    }
//    return os;
//}

