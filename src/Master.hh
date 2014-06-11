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

#include <boost/mpi/environment.hpp>
#include <boost/mpi/communicator.hpp>
#include <boost/mpi/nonblocking.hpp>

#include "mongoose.h"

//-------------------------------------------------------------------------
// Request
//-------------------------------------------------------------------------

struct MasterRequest {

    enum Type { JSON_OBJECT=0, OCTET_STREAM=1};

    MasterRequest(mg_connection *conn, const std::vector<std::string> &params);

    void respondJson(std::string msg);

    void respondOctetStream(const void *ptr, int size);

    void printInfo();

    char* getURI();

    const char* getHeader(char* headername);

private:

    mg_connection *conn;

public:

    const std::vector<std::string> &params;

    int response_size;

};

//-------------------------------------------------------------------------
// MasterRequestHandler
//-------------------------------------------------------------------------

typedef std::function<void (MasterRequest&)> MasterRequestHandler;

//-------------------------------------------------------------------------
// MasterException
//-------------------------------------------------------------------------

struct MasterException: public std::runtime_error {
public:
    MasterException(const std::string &message);
};

//-------------------------------------------------------------------------
// Slave
//-------------------------------------------------------------------------

struct Slave {
public:
    Slave(std::string address, int port);

    std::string address;
    int port;
};


//-------------------------------------------------------------------------
// Master
//-------------------------------------------------------------------------

struct Master {

    Master(std::vector<Slave> slaves);

    void start(int mongoose_threads);
    void stop();
    bool toggleTiming(bool b);
    bool isTiming() const;
    const std::string currentDateTime();

    void requestSlave(MasterRequest &request, Slave &slave);
    void processSlave(MasterRequest &request);
    void requestAllSlaves(MasterRequest &request);
    void *mg_callback(mg_event event, mg_connection *conn);


    int port;
    int mongoose_threads;
private:

    bool done;
    bool is_timing;
    std::ofstream timing_of;
    std::vector<Slave> slaves;
};
