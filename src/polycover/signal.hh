#pragma once

#include <memory>
#include <vector>
#include <map>

namespace polycover {

namespace sig {

using ConnectionId = uint64_t;

//
// Make sure objects connected to a signal out-live the signal
// or, at least, any emission of that signal. Disconnect
// connected objects before deleting them.
//

template <typename... Args>
struct Signal {
public: // subtypes
    using F = std::function<void(Args...)>;
public: // methods
    Signal();
    void trigger(Args... args);
    auto connect(F f) -> ConnectionId;
    void disconnect(ConnectionId connection_id);
private:
    std::map<ConnectionId, F> functions;
    ConnectionId next_connection_id;
};

} // signal namespace


//-----------------------------------------------------------------------------
// sig::Signal Impl.
//-----------------------------------------------------------------------------

template <typename... Args>
sig::Signal<Args...>::Signal():
    next_connection_id(0)
{}

template <typename... Args>
void sig::Signal<Args...>::trigger(Args... args) {
    for (auto it: functions) {
        it.second(args...);
    }
}

template <typename... Args>
auto sig::Signal<Args...>::connect(F f) -> ConnectionId
{
    ConnectionId connection_id = next_connection_id++;
    functions[connection_id] = f;
    return connection_id;
}

template <typename... Args>
void sig::Signal<Args...>::disconnect(ConnectionId connection_id) {
    functions.erase(connection_id);
}


} // end polycover namespace 
