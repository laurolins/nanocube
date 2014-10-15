#pragma once

#include <iostream>
#include <algorithm>
#include <iomanip>
#include <unistd.h>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <fstream>
#include <string>
#include <exception>
#include <stdexcept>
#include <functional>
#include <mutex>

#include "mongoose.h"

//-------------------------------------------------------------------------
// Request
//-------------------------------------------------------------------------

struct Request {

    enum Type { JSON_OBJECT=0, OCTET_STREAM=1};

    Request(mg_connection *conn, const std::string& request_string);

    void respondJson(std::string msg_content);

    void respondText(std::string msg_content);

    void respondOctetStream(const void *ptr, std::size_t size);

private:

    mg_connection *conn;

public:

    const std::string request_string;

    int response_size;

};

//-------------------------------------------------------------------------
// RequestHandler
//-------------------------------------------------------------------------

typedef std::function<void (Request&)> RequestHandler;


//-------------------------------------------------------------------------
// ServerException
//-------------------------------------------------------------------------

struct ServerException: public std::runtime_error {
public:
    ServerException(const std::string &message);
};

//-------------------------------------------------------------------------
// Server
//-------------------------------------------------------------------------

struct Server {

    Server() = default;

    void init(int mongoose_threads);
    void run();
    
    void stop();
    
    bool toggleTiming(bool b);
    
    bool isTiming() const;
    
    const std::string currentDateTime();
    const std::string currentDateTime2();
    
    void setHandler(const RequestHandler& request_handler);

    void handle_request(Request &request);
    
public:
    
    int port { 29512 };

    int mongoose_threads { 10 };

    RequestHandler handler;

public:

    bool is_timing { false };
    
    bool keep_running { true };
    
    std::ofstream timing_of;
    
    std::mutex mutex; // mutual exclusion for writing into the timing file

};
