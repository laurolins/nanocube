#pragma once

#include <iostream>
#include <mongoose.h>
#include <algorithm>
#include <iomanip>
#include <unistd.h>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <fstream>
#include <string>

#include <functional>

#include "Logger.hh"

//-------------------------------------------------------------------------
// Request
//-------------------------------------------------------------------------

struct Request {

    enum Type { JSON_OBJECT=0, OCTET_STREAM=1};

    Request(mg_connection *conn, const std::vector<std::string> &params, 
            Logger &logger);

    void respondJson(std::string msg_content, int return_code=200);
    void respondText(std::string msg_content, int return_code=200);
    void respondOctetStream(const void *ptr, int size, int return_code=200);

private:

    mg_connection *conn;
    Logger &logger;

public:

    int response_size;
    const std::vector<std::string> &params;
    int cache_max_age_in_seconds;
    std::string allowed_origin;
};

//-------------------------------------------------------------------------
// RequestHandler
//-------------------------------------------------------------------------

typedef std::function<void (Request&)> RequestHandler;

//-------------------------------------------------------------------------
// Server
//-------------------------------------------------------------------------

struct Server {

    Server();

    void registerHandler(std::string name, RequestHandler handler);

    void start(int mongoose_threads);
    void stop();
    bool toggleTiming(bool b);
    const std::string currentDateTime();

    // marginal security to present unauthorized shutdowns
    std::string passcode() { return m_passcode; }
    void passcode(std::string passcode) { m_passcode = passcode; }

    void *mg_callback(mg_event event, mg_connection *conn);


    int port;
    int mongoose_threads;
private:

    std::unordered_map<std::string, RequestHandler> handlers;
    std::string m_passcode;

    bool done;
    bool is_timing;
    std::ofstream timing_of;
    Logger logger;
};
