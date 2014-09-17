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

void Request::respondOctetStream(const void *ptr, int size)
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
        mg_write(conn, ptr, size);
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

//void* Server::mg_callback(struct mg_connection *conn, enum mg_event event)
//{ // blocks current thread
//    
//    
//    //    if (ev == MG_AUTH) {
//    //        return MG_TRUE;   // Authorize all requests
//    //    } else if (ev == MG_REQUEST && !strcmp(conn->uri, "/hello")) {
//    //        mg_printf_data(conn, "%s", "Hello world");
//    //        return MG_TRUE;   // Mark as processed
//    //    } else {
//    //        return MG_FALSE;  // Rest of the events are not processed
//    //    }
//    
//
//    if (event == MG_NEW_REQUEST && handler) {
//
//        std::chrono::time_point<std::chrono::high_resolution_clock> t0;
//        if (is_timing) {
//            t0 = std::chrono::high_resolution_clock::now();
//        }
//
//        const struct mg_request_info *request_info = mg_get_request_info(conn);
//
//        std::string uri(request_info->uri  + 1);
//        //std::cout << "Request URI: " << uri << std::endl;
//
//        // tokenize on slahses: first should be the address,
//        // second the requested function name, and from
//        // third on parameters to the functions
////        std::vector<std::string> tokens;
////        boost::split(tokens, uri, boost::is_any_of("/"));
//
//        Request request(conn, uri);
//
////        if (tokens.size() == 0) {
////            // std::cout << "Request URI: " << uri << std::endl;
////            std::stringstream ss;
////            ss << "bad URL: " << uri;
////            request.respondJson(ss.str());
////            return  (void*) ""; // mark as processed
////        }
////        else if (tokens.size() == 1) {
////            // std::cout << "Request URI: " << uri << std::endl;
////            std::stringstream ss;
////            ss << "no handler name was provided on " << uri;
////            request.respondJson(ss.str());
////            return (void*) ""; // mark as processed
////        }
//
////        std::string handler_name = tokens[1];
//        //std::cout << "Searching handler: " << handler_name << std::endl;
//
////        if (handlers.find(handler_name) == handlers.end()) {
////            // std::cout << "Request URI: " << uri << std::endl;
////            std::stringstream ss;
////            ss << "no handler found for " << uri << " (request key: " << handler_name << ")";
////            request.respondJson(ss.str());
////            return (void*) ""; // mark as processed
////        }
//
//        if (is_timing) {
//            handler(request);
//            auto t1 = std::chrono::high_resolution_clock::now();
//            uint64_t elapsed_nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(t1-t0).count();
//            timing_of << currentDateTime() << " " << uri
//                      << " " << elapsed_nanoseconds << " ns"
//                      << " input " << uri.length()
//                      << " output " << request.response_size
//                      << std::endl;
//            // std::cout << "Request URI: " << uri << std::endl;
//        } else {
//            // std::cout << "Request URI: " << uri << std::endl;
//            handler(request);
//        }
//
//        return (void*) "";
//    }
//    return 0;
//}
//    typedef int (*mg_handler_t)(struct mg_connection *, enum mg_event);

static Server *__server { nullptr };

int __mg_callback(struct mg_connection* c, enum mg_event e)
{
    if (e == MG_AUTH) {
        return MG_TRUE;   // Authorize all requests
    } else if (e == MG_REQUEST) {
        std::string uri(c->uri + 1);
        Request request(c, uri);
        __server->handler(request);
        return MG_TRUE;   // Mark as processed
        
    } else {
        return MG_FALSE;  // Rest of the events are not processed
    }
};


void Server::start(int mongoose_threads) // blocks current thread
{
    
    __server = this;
    
//    std::function<int(struct mg_connection*, enum mg_event)> mg_handler = [&that](struct mg_connection* c,
//                                                                                  enum mg_event e) {
//        if (e == MG_AUTH) {
//            return MG_TRUE;   // Authorize all requests
//        } else if (e == MG_REQUEST) {
//            std::string uri(c->uri + 1);
//            Request request(c, uri);
//            that.handler(request);
//            return MG_TRUE;   // Mark as processed
//        
//        } else {
//            return MG_FALSE;  // Rest of the events are not processed
//        }
//    };
    
    //
    // typedef int (*mg_handler_t)(struct mg_connection *, enum mg_event);
    // typedef void* function_t( void* ) ;
    // function_t** ptr_ptr_fun = func.target<function_t*>() ;
    //

    // typedef int (*mg_handler_t)(struct mg_connection *, enum mg_event);
//    typedef int (mg_handler_type)(struct mg_connection *, enum mg_event);
//    mg_handler_type** f = mg_handler.target<mg_handler_type*>();
    // std::cout << "target type: " << mg_handler. << std::endl;
    
    mg_server *srv = mg_create_server(NULL, __mg_callback);
    mg_set_option(srv, "num_threads", std::to_string(mongoose_threads).c_str());      // Serve current directory
    mg_set_option(srv, "listening_port", std::to_string(port).c_str());  // Open port 8080

    // add some flag for a clean shutdown of mongoose server
    for (;keep_running;) {
        mg_poll_server(srv, 1000);   // Infinite loop, Ctrl-C to stop
    }
    mg_destroy_server(&srv);
}

void Server::stop() // blocks current thread
{
    keep_running = false;
}

//void Server::stop()
//{
//  done = true;
//  handlers.clear();
//}

bool Server::toggleTiming(bool b)
{
    is_timing = b;
    if (is_timing) {
        if (timing_of.is_open()) {
            // already open, do nothing
        } else {
            // record requests and times in a file
            std::string filename = "nanocube-query-timing.txt";
            timing_of.open(filename);
            if (!timing_of.is_open()) {
                std::cout << "[WARNING]: Could not record requests and times" << std::endl;
                is_timing = false;
                return false;
            }
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



//            if (is_timing) {
//                that.handler(request);
//                auto t1 = std::chrono::high_resolution_clock::now();
//                uint64_t elapsed_nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(t1-t0).count();
//                timing_of << that.currentDateTime() << " " << uri
//                << " " << elapsed_nanoseconds << " ns"
//                << " input " << uri.length()
//                << " output " << request.response_size
//                << std::endl;
//                // std::cout << "Request URI: " << uri << std::endl;
//            } else {
// std::cout << "Request URI: " << uri << std::endl;
//            }


