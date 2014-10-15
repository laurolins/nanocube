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
#include <memory>
#include <thread>

#include <boost/asio.hpp>
// #include <boost/mpi/environment.hpp>
// #include <boost/mpi/communicator.hpp>
// #include <boost/mpi/nonblocking.hpp>

#include "NanoCubeSchema.hh"

#include "mongoose.h"

//-------------------------------------------------------------------------
// Request
//-------------------------------------------------------------------------

struct MasterRequest {

    enum Type { JSON_OBJECT=0, OCTET_STREAM=1};
    enum Uri {SCHEMA=0, QUERY=1, BIN_QUERY=2, TILE=3, TQUERY=4, TBIN=5, BIN_TQUERY=6 };

    MasterRequest(mg_connection *conn, const std::vector<std::string> &params, std::string uri);

    void respondJson(std::string msg);

    void respondOctetStream(const void *ptr, int size);

    const char* getHeader(char* headername);

private:

    mg_connection *conn;

public:

    const std::vector<std::string> &params;

    int response_size;

    Uri uri_translated; //uri from the master to the slaves
    Uri uri_original; //uri from the browser to the master

    std::string uri_strtranslated;
    std::string uri_stroriginal;
    std::string query_params;

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
    Slave(std::string address);

    //void setResponse(std::string response);
    std::string content_type;
    int content_length;

    std::string address;
    int deamon_port;
    int insert_port;
    int query_port;
    float load;
 

private:
    std::string response;
};


//-------------------------------------------------------------------------
// Master
//-------------------------------------------------------------------------

struct Master {

    Master(std::vector<Slave> slaves);

    void start(int mongoose_threads, int mongoose_port);
    void stop();
    bool toggleTiming(bool b);
    bool isTiming() const;
    const std::string currentDateTime();

    void writeSlave(std::string uri, boost::asio::ip::tcp::socket& socket);
    void readSlave(boost::asio::ip::tcp::socket& socket, std::vector<char>* content);
    void requestAllSlaves(MasterRequest &request);
    void requestSchema();
    void *mg_callback(mg_event event, mg_connection *conn);
    void parse(std::string query_st, ::query::QueryDescription &query_description);

    int mongoose_port;
    int mongoose_threads;

private:

    bool done;
    bool is_timing;
    std::ofstream timing_of;
    std::vector<Slave> slaves;
    
    dumpfile::DumpFileDescription     schema_dump_file_descriptions;
    std::unique_ptr<nanocube::Schema> schema;
};
