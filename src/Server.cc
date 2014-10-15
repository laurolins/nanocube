#include <chrono>

#include <thread>
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <unistd.h>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <string>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

#include "mongoose.h"

#include "Server.hh"

//-------------------------------------------------------------------------
// Request Impl.
//-------------------------------------------------------------------------

Request::Request(mg_connection *conn, const std::string &request_string):
    conn(conn), request_string(request_string), response_size(0)
{}


static std::string _content_type[] {
    std::string("application/json")         /* 0 */,
    std::string("application/octet-stream") /* 1 */,
    std::string("text/plain")         /* 2 */
};

void Request::respondText(std::string msg_content)
{
    const std::string sep = "\r\n";

    std::stringstream ss;
    ss << "HTTP/1.1 200 OK"                << sep
       << "Content-Type: text/plain"       << sep
       << "Access-Control-Allow-Origin: *" << sep
       << "Content-Length: %d"             << sep << sep
       << "%s";

    // response_size = 106 + (int) msg_content.size(); // banchmark data transfer
    response_size = (int) msg_content.size();
    // check how a binary stream would work here
    mg_printf(conn, ss.str().c_str(), (int) msg_content.size(), msg_content.c_str());
}

void Request::respondJson(std::string msg_content)
{
    const std::string sep = "\r\n";
    
    std::stringstream ss;
    ss << "HTTP/1.1 200 OK"             << sep
    << "Content-Type: application/json" << sep
    << "Access-Control-Allow-Origin: *" << sep
    << "Content-Length: %d"             << sep << sep
    << "%s";
    
    // response_size = 106 + (int) msg_content.size(); // banchmark data transfer
    response_size = (int) msg_content.size();
    // check how a binary stream would work here
    mg_printf(conn, ss.str().c_str(), (int) msg_content.size(), msg_content.c_str());
}

void Request::respondOctetStream(const void *ptr, std::size_t size)
{
    const std::string sep = "\r\n";

    std::stringstream ss;
    ss << "HTTP/1.1 200 OK"                        << sep
       << "Content-Type: application/octet-stream" << sep
       << "Access-Control-Allow-Origin: *"         << sep
       << "Content-Length: %d"                     << sep << sep;

    response_size = 0; // 114;
    // check how a binary stream would work here
    mg_printf(conn, ss.str().c_str(), size);

    if (ptr) { // unsage access to nullptr
        mg_write(conn, ptr, (int) size);
        response_size += size;
    }
    
}


//-------------------------------------------------------------------------
// ServerException
//-------------------------------------------------------------------------

ServerException::ServerException(const std::string &message):
    std::runtime_error(message)
{}

//-------------------------------------------------------------------------
// Server
//-------------------------------------------------------------------------

void Server::setHandler(const RequestHandler& rh)
{
    handler = rh;
}

static Server    *__server { nullptr };
static mg_server *__mongoose_server { nullptr };

int __mg_callback(struct mg_connection* c, enum mg_event e)
{
    if (e == MG_AUTH) {
        return MG_TRUE;   // Authorize all requests
    } else if (e == MG_REQUEST) {
        std::string uri(c->uri + 1);
        Request request(c, uri);
        __server->handle_request(request);
        return MG_TRUE;   // Mark as processed
        
    } else {
        return MG_FALSE;  // Rest of the events are not processed
    }
};

void Server::handle_request(Request &request) {
    std::chrono::time_point<std::chrono::high_resolution_clock> t0;

    bool was_timing = is_timing;
    if (is_timing) {
        t0 = std::chrono::high_resolution_clock::now();
    }

    handler(request);
    
    if (was_timing) {
        // std::cout << "Request URI: " << uri << std::endl;
        auto t1 = std::chrono::high_resolution_clock::now();
        uint64_t elapsed_nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(t1-t0).count();
        {
            std::lock_guard<std::mutex> lock(mutex);
            timing_of
            << currentDateTime() << "|"
            << elapsed_nanoseconds << "|"
            << request.response_size << "|"
            << request.request_string.size() << "|"
            << "'" << request.request_string << "'" << std::endl;
        }
    }
}

void Server::init(int mongoose_threads) // blocks current thread
{
    __server = this;
    
    mg_server *srv = mg_create_server(NULL, __mg_callback);
    mg_set_option(srv, "num_threads", std::to_string(mongoose_threads).c_str());      // Serve current directory
    mg_set_option(srv, "listening_port", std::to_string(port).c_str());  // Open port 8080
    
    __mongoose_server = srv;
    
    mg_poll_server(srv, 1000);   // Infinite loop, Ctrl-C to stop
    auto listening_socket = mg_get_listening_socket(srv);
    if (listening_socket == -1) { // -1 is INVALID_SOCKET on mongoose
        mg_destroy_server(&srv);
        throw ServerException("Problem starting mongoose server");
    }
}

void Server::run() {
    
    if (!__mongoose_server)
        throw ServerException("No mongoose server initialized");
    
    // add some flag for a clean shutdown of mongoose server
    for (;keep_running;) {
        mg_poll_server(__mongoose_server, 1000);   // Infinite loop, Ctrl-C to stop
        // std::cerr << "mg_poll_server result = " << code << std::endl;
    }
    mg_destroy_server(&__mongoose_server);
}

void Server::stop() // blocks current thread
{
    keep_running = false;
}

bool Server::toggleTiming(bool b)
{
    is_timing = b;
    if (is_timing) {
        if (timing_of.is_open()) {
            // already open, do nothing
        } else {
            // record requests and times in a file
            std::string filename = "nanocube-query-timing-" + currentDateTime2() + ".psv";
            timing_of.open(filename);
            if (!timing_of.is_open()) {
                std::cout << "[WARNING]: Could not record requests and times" << std::endl;
                is_timing = false;
                return false;
            }
            timing_of << "finish_time|latency_ns|output_bytes|input_bytes|query_string" << std::endl;
        }
    } else {
        if (timing_of.is_open()) {
            timing_of.close();
        }
    }
    return true;
}

bool Server::isTiming() const
{
    return is_timing;
}

// Get current date/time, format is YYYY-MM-DD.HH:mm:ss
const std::string Server::currentDateTime() {
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    // Visit http://www.cplusplus.com/reference/clibrary/ctime/strftime/
    // for more information about date/time format
    strftime(buf, sizeof(buf), "%Y-%m-%d_%X", &tstruct);
    return buf;
}


// Get current date/time, format is YYYYMMDDHHmmss
const std::string Server::currentDateTime2() {
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    // Visit http://www.cplusplus.com/reference/clibrary/ctime/strftime/
    // for more information about date/time format
    strftime(buf, sizeof(buf), "%Y%m%d%H%M%S", &tstruct);
    return buf;
}
