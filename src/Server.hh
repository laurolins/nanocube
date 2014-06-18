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


#include "mongoose.h"

//-------------------------------------------------------------------------
// Request
//-------------------------------------------------------------------------

struct Request {

    enum Type { JSON_OBJECT=0, OCTET_STREAM=1};

    Request(mg_connection *conn, const std::vector<std::string> &params);

    void respondJson(std::string msg_content);

    void respondText(std::string msg_content);

    void respondOctetStream(const void *ptr, int size);

private:

    mg_connection *conn;

public:

    const std::vector<std::string> &params;

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

    Server();

    void registerHandler(std::string name, const RequestHandler &handler);

    void start(int mongoose_threads);
    void stop();
    bool toggleTiming(bool b);
    bool isTiming() const;
    const std::string currentDateTime();

    void *mg_callback(mg_event event, mg_connection *conn);

    int port { 29512 };
    int mongoose_threads { 10 };
private:

    std::unordered_map<std::string, RequestHandler> handlers;

    struct mg_context *ctx { nullptr };
    bool is_timing { false };
    std::ofstream timing_of;
};
