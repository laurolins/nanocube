#include <Server.hh>

#include <chrono>

#include <iostream>
#include <mongoose.h>
#include <algorithm>
#include <iomanip>
#include <unistd.h>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <string>

#include "tokenizer.hh"

//-------------------------------------------------------------------------
// Request Impl.
//-------------------------------------------------------------------------

Request::Request(mg_connection *conn, const std::vector<std::string> &params):
    conn(conn), response_size(0), params(params)
{}

static std::string _content_type[] {
    std::string("application/json")         /* 0 */,
    std::string("application/octet-stream") /* 1 */
};

void Request::respondJson(std::string msg_content)
{
    const std::string sep = "\r\n";

    std::stringstream ss;
    ss << "HTTP/1.1 200 OK"                  << sep
       << "Content-Type: application/json"   << sep
       << "Content-Length: %d"               << sep << sep
       << "%s";

    response_size = 106 + (int) msg_content.size();
    // check how a binary stream would work here
    mg_printf(conn, ss.str().c_str(), (int) msg_content.size(), msg_content.c_str());
}

void Request::respondOctetStream(const void *ptr, int size)
{
    const std::string sep = "\r\n";

    std::stringstream ss;
    ss << "HTTP/1.1 200 OK"                        << sep
       << "Content-Type: application/octet-stream" << sep
       << "Content-Length: %d"                     << sep << sep;

    response_size = 114;
    // check how a binary stream would work here
    mg_printf(conn, ss.str().c_str(), size);


    if (ptr) { // unsage access to nullptr
        mg_write(conn, ptr, size);
        response_size += size;
    }
}


//-------------------------------------------------------------------------
// Server
//-------------------------------------------------------------------------

Server::Server():
  port(29999),
  mongoose_threads(10),
  done(false),
  is_timing(false)
{}

void Server::registerHandler(std::string name, RequestHandler handler)
{
    std::cout << "Registering handler: " << name << std::endl;
    handlers[name] = handler;
}

void* Server::mg_callback(mg_event event, mg_connection *conn)
{ // blocks current thread

    if (event == MG_NEW_REQUEST) {

        auto t0 = std::chrono::high_resolution_clock::now();

        const struct mg_request_info *request_info = mg_get_request_info(conn);

        std::string uri(request_info->uri);

        // std::cout << "---------------------------------------" << std::endl;

        // tokenize on slahses: first should be the address,
        // second the requested function name, and from
        // third on parameters to the functions
        tokenizer::Tokenizer tok(uri,'/','/');
        tok.update();
        std::vector<std::string> tokens;
        tokens.reserve(tok.tokens());
        for (auto i=0;i<tok.tokens();++i) {
            // std::cout << i << ":         " << "[" << tok.asStr(i) << "]" << std::endl;
            tokens.push_back(tok.asStr(i));
        }

        std::cout << "URI:       " << uri << std::endl;
        std::cout << "no tokens: " << tok.tokens() << std::endl;

        Request request(conn, tokens);

        if (tok.tokens() == 0) {
            std::cout << "Request URI: " << uri << std::endl;
            std::stringstream ss;
            ss << "bad URL: " << uri;
            request.respondJson(ss.str());
            return  (void*) ""; // mark as processed
        }
        else if (tok.tokens() == 1) {
            std::cout << "Request URI: " << uri << std::endl;
            std::stringstream ss;
            ss << "no handler name was provided on " << uri;
            request.respondJson(ss.str());
            return (void*) ""; // mark as processed
        }

        // std::cout << "tokens[1]: " << tokens[1] << std::endl;
        std::string handler_name = tokens[1];
        // std::cout << "Searching handler: " << handler_name << std::endl;

        if (handlers.find(handler_name) == handlers.end()) {
            std::cout << "Request URI: " << uri << std::endl;
            std::stringstream ss;
            ss << "no handler found for " << uri << " (request key: " << handler_name << ")";
            request.respondJson(ss.str());
            return (void*) ""; // mark as processed
        }

        if (is_timing) {
            handlers[handler_name](request);
            auto t1 = std::chrono::high_resolution_clock::now();
            uint64_t elapsed_nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(t1-t0).count();
            timing_of << currentDateTime() << " " << uri
                      << " " << elapsed_nanoseconds << " ns"
                      << " input " << uri.length()
                      << " output " << request.response_size
                      << std::endl;
            std::cout << "Request URI: " << uri << std::endl;
        } else {
            std::cout << "Request URI: " << uri << std::endl;
            handlers[handler_name](request);
        }

        return (void*) "";
    }
    return 0;
}

static Server *_server;
void* __mg_callback(mg_event event, mg_connection *conn)
{
    return _server->mg_callback(event, conn);
}

void Server::start(int mongoose_threads) // blocks current thread
{
    char p[256];
    sprintf(p,"%d",port);
    std::string port_st = p;
    this->mongoose_threads = mongoose_threads;

    _server = this; // single Server

    std::string mongoose_string = std::to_string(mongoose_threads);
    struct mg_context *ctx;
    const char *options[] = {"listening_ports", port_st.c_str(), "num_threads", mongoose_string.c_str(), NULL};
    ctx = mg_start(&__mg_callback, NULL, options);

    if (!ctx) {
        std::cout << "Couldn't create mongoose context... exiting!" << std::endl;
        return;
    }

    std::cout << "Server on port " << port << std::endl;
    while (!done) // this thread will be blocked
        sleep(1);

    mg_stop(ctx);
}

void Server::stop()
{
  done = true;
  handlers.clear();
}

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
