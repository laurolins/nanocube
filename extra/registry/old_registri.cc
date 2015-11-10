#include <iostream>
#include <random>
#include <chrono>
#include <deque>
#include <sstream>
#include <iomanip>

#include <set>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/time.h>

#include <cmath>
#include <cstdlib>
#include <ctime>
#include <cstdio>

#include <unordered_map>
#include <unordered_set>

#include <string>
#include <fstream>

#include <Server.hh>
Server *g_server;

#include <unordered_map>
#include <functional>

//-----------------------------------------------------------------------------
// Split routines
//-----------------------------------------------------------------------------

std::vector<std::string> &split(const std::string &s, char delim,
                                std::vector<std::string> &elems)
{
    std::stringstream ss(s);
    std::string item;
    while(std::getline(ss, item, delim)) {
         elems.push_back(item);
    }
    return elems;
}

void deque_split(const std::string &s, char delim,
                  std::deque<std::string> &elems)
{
    std::stringstream ss(s);
    std::string item;
    while(std::getline(ss, item, delim)) {
         elems.push_back(item);
    }
}

struct Index
{
public:
    Index();
    ~Index();
    void registerServices(Server &server);
    void serveOnline(Request &request);
    void serveOffline(Request &request);
    void serveStatus(Request &request);
    void serveHistory(Request &request);
    void serveLookup(Request &request);
    void dumpStatus(std::ostream &os, bool history);
    std::string getCurrentTime();
private:
    bool searchForDupEntry(Request &request);
    bool searchForDupAlias(Request &request);

    std::vector<std::string> m_times;
    std::vector<bool> m_online;
    std::vector<std::string> m_hostnames;
    std::vector<int> m_ports;
    std::vector<std::string> m_names;
    std::vector<std::string> m_versions;
    std::vector<int> m_codes;
    size_t m_online_nanocubes;
};

Index *g_index;

//---------------------------------------------------------------------------
// Index Impl.
//---------------------------------------------------------------------------
Index::Index():
    m_online_nanocubes(0)
{}

Index::~Index()
{}

void Index::dumpStatus(std::ostream &os, bool history)
{
    if (m_online_nanocubes == 0 && !history) {
        // no nanocubes are online and we are not interested in overall history
        os << "No nanocubes are known to be online";
    } else {
        // If we want history, but there have never been any registered..
        if (m_hostnames.size() == 0) {
            os << "No nanocubes have ever been registered";
        } else {
            // Otherwise, give online or complete history based on request
            os << "alias|status|time|hostname|port|version|error" << std::endl;

            for (size_t i = 0; i < m_hostnames.size(); ++i) {
                if (m_online[i] || history) {
                    os << m_names[i];
                    if (m_online[i])
                        os << "|on";
                    else
                        os << "|off";
                    os << "|" << m_times[i];
                    os << "|" << m_hostnames[i]
                       << "|" << m_ports[i]
                       << "|" << m_versions[i] << "|";

                    switch (m_codes[i]) {
                    case EXIT_SUCCESS:
                        os << "None";
                        break;
                    case SIGINT:
                        os << "SIGINT";
                        break;
                    case SIGQUIT:
                        os << "SIGQUIT";
                        break;
                    case SIGILL:
                        os << "SIGILL";
                        break;
                    case SIGTRAP:
                        os << "SIGTRAP";
                        break;
                    case SIGABRT:
                        os << "SIGABRT";
                        break;
                    case SIGFPE:
                        os << "SIGFPE";
                        break;
                    case SIGSEGV:
                        os << "SIGSEGV";
                        break;
                    case SIGTERM:
                        os << "SIGTERM";
                        break;
                    default:
                        os << "UNKNOWN";
                    }
                    os << std::endl;
                }
            }
        }
    }
}

void Index::serveStatus(Request &request)
{
    std::stringstream ss;
    this->dumpStatus(ss, false);
    request.respondJson(ss.str());
}

void Index::serveHistory(Request &request)
{
    std::stringstream ss;
    this->dumpStatus(ss, true);
    request.respondJson(ss.str());
}


std::string Index::getCurrentTime()
{
    char buf[256];
    struct timeval tv;
    time_t current_time;
    gettimeofday(&tv, NULL);
    current_time = tv.tv_sec;
    strftime(buf, 256, "%a %b %d %T %Z %Y", localtime(&current_time));
    return std::string(buf);
}

void Index::serveLookup(Request &request)
{
    std::stringstream ss;

    if (request.params.size() < 3) {
        ss << "lookup bad size = " << request.params.size();
    } else {

        bool found = false;
        for (size_t i = 0; i < m_names.size(); ++i) {
            // Do a reverse order lookup for the last entry
            // with this alias in case multiple nanocubes
            // had the same alias
            size_t j = m_names.size() - i - 1;
            if (request.params[2] == m_names[j]) {
                found = true;
                if (m_online[j])
                    ss << m_hostnames[j] << ":" << m_ports[j];
                else
                    ss << "Nanocube " << m_names[j] << " was running on "
                       << m_hostnames[j] << ":" << m_ports[j] << " but is now offline";
                break;
            }
        }
        if (!found)
            ss << "Unknown nanocube alias specified: " << request.params[2];
    }
    request.respondJson(ss.str());
}

bool Index::searchForDupEntry(Request &request)
{
    bool found = false;
    for (size_t i = 0; i < m_hostnames.size(); ++i) {
        if (m_online[i]) {
            if (request.params[2] == m_hostnames[i]) {
                if ((std::stoi(request.params[3])) == m_ports[i]) {
                    found = true;
                    break;
                }
            }
        }
    }
    return found;
}

bool Index::searchForDupAlias(Request &request)
{
    bool found = false;
    for (size_t i = 0; i < m_names.size(); ++i) {
        if (m_online[i] && (request.params[4] == m_names[i])) {
            found = true;
            break;
        }
    }
    return found;
}

void Index::serveOnline(Request &request)
{
    std::stringstream ss;

    if (request.params.size() != 6) {
        ss << "1:online bad size = " << request.params.size();
    } else {

        // search for duplicate entry (hosthame/port)
        if (searchForDupEntry(request)) {
            ss << "2:registrtion: fail duplicate nanocube server:port "
               << request.params[2] << ":" << request.params[3];
        } else if (searchForDupAlias(request)) {
            ss << "3:registration: fail duplicate nanocube alias " << request.params[4];
        } else {
            m_times.push_back(getCurrentTime());
            m_online.push_back(true);
            m_hostnames.push_back(request.params[2]);
            m_ports.push_back(std::stoi(request.params[3]));
            m_names.push_back(request.params[4]);
            m_versions.push_back(request.params[5]);
            m_codes.push_back(0);
            ss << "0:registration: " << m_names[m_names.size()-1];
            m_online_nanocubes++;
        }
    }
    request.respondJson(ss.str());
}

void Index::serveOffline(Request &request)
{
    std::stringstream ss;

    if (request.params.size() != 5) {
        ss << "offline bad size = " << request.params.size();
    } else {

        bool found = false;
        for (size_t i = 0; i < m_hostnames.size(); ++i) {
            if (m_online[i]) {
                if (request.params[2] == m_hostnames[i]) {
                    if ((std::stoi(request.params[3])) == m_ports[i]) {
                        found = true;
                        ss << "Taking nanocube " << m_names[i] << " offline on "
                           << m_hostnames[i]
                           << " at port "
                           << m_ports[i];
                        m_times[i] = getCurrentTime();
                        m_online[i] = false;
                        m_codes[i] = std::stoi(request.params[4]);
                        break;
                    }
                }
            }
        }
        if (found)
            m_online_nanocubes--;
        else
            ss << "Unknown nanocube specified: " << request.params[2] << " on port " << request.params[3];
    }
    request.respondJson(ss.str());
}


// register services into server
void Index::registerServices(Server &server)
{
    {
        RequestHandler handler = std::bind(&Index::serveOnline, this, std::placeholders::_1);
        server.registerHandler("online", handler);
    }

    {
        RequestHandler handler = std::bind(&Index::serveOffline, this, std::placeholders::_1);
        server.registerHandler("offline", handler);
    }

    {
        RequestHandler handler = std::bind(&Index::serveStatus, this, std::placeholders::_1);
        server.registerHandler("status", handler);
    }

    {
        RequestHandler handler = std::bind(&Index::serveHistory, this, std::placeholders::_1);
        server.registerHandler("history", handler);
    }

    {
        RequestHandler handler = std::bind(&Index::serveLookup, this, std::placeholders::_1);
        server.registerHandler("lookup", handler);
    }
}


//-----------------------------------------------------------------------------
// signal handlers
//-----------------------------------------------------------------------------
static int exit_status = EXIT_SUCCESS;
static char gs_hname[256];
static int  gs_hport;

void exitFunction(void)
{
    std::cerr << "Shuting down..." << std::endl;

    std::cerr << std::endl;
    std::cerr << std::endl;
    std::cerr << "****************************" << std::endl;
    std::cerr << "* Current Nanocube History *" << std::endl;
    std::cerr << "****************************" << std::endl;
    std::cerr << std::endl;
    const int rc = system("date");
    std::cerr << std::endl;

    std::stringstream db_down;
    g_index->dumpStatus(db_down, true);
    std::cerr << db_down.str();

    std::cerr << std::endl;
    std::cerr << std::endl;
}

void signalHandler(int signum)
{
    std::cerr << "Caught signal " << signum << std::endl;
    exit_status = signum;
    exit(signum);
}

void setupSignalHandlers()
{
    // If SIGHUP was being ignored, keep it that way...
    void (*previousSignalHandler)(int);
    previousSignalHandler = signal(SIGHUP, signalHandler);
    if (previousSignalHandler == SIG_IGN) {
        std::cout << "Ignoring SIGHUP as before" << std::endl;
        signal(SIGHUP, SIG_IGN);
    }

    signal(SIGINT,  signalHandler);
    signal(SIGQUIT, signalHandler);
    signal(SIGILL,  signalHandler);
    signal(SIGTRAP, signalHandler);
    signal(SIGABRT, signalHandler);
    signal(SIGFPE,  signalHandler);
    signal(SIGSEGV, signalHandler);
    signal(SIGTERM, signalHandler);

    atexit(exitFunction);
}

//-----------------------------------------------------------------------------
// main
//-----------------------------------------------------------------------------
int main(int argc, char** argv)
{
    // create a server
    g_server = new Server;
    Server &server = *g_server;
    int port = g_server->port;
    int mongoose_threads = 10;

    bool help = false;

    // find type of dump
    try {
        std::string       type_st("");
        for (int i=1;i<argc;i++) {
            std::vector<std::string> tokens;
            split(std::string(argv[i]),'=',tokens);
            if (tokens.size() < 1) {
                continue;
            }
            if (tokens[0].compare("--help") == 0) {
                help = true;
            }
            if (tokens.size() < 2) {
                continue;
            }
            if (tokens[0].compare("--port") == 0) {
                port = std::stoi(tokens[1]);
            }
            if (tokens[0].compare("--threads") == 0) {
                mongoose_threads = std::stoi(tokens[1]);
                if (mongoose_threads < 0 ||  mongoose_threads > 20)
                    mongoose_threads = 10;
            }
        }
    }
    catch (...)
    {}

    if (help) {
        std::stringstream ss;
        ss << "registry [options]"                                  << std::endl
           << ""                                                    << std::endl
           << "options:"                                            << std::endl
           << "   --port=n    (default: 29999)"                     << std::endl
           << "         service port"                               << std::endl
           << ""                                                    << std::endl
           << "   --threads=n    (default: 10)"                     << std::endl
           << "         number of mongoose threads"                 << std::endl
           << ""                                                    << std::endl
           << "   --help"                                           << std::endl
           << "         help message"                               << std::endl
           << ""                                                    << std::endl;
        std::cout << ss.str();
        exit(0);
    }

    g_index = new Index;
    g_index->registerServices(server);

    // update port
    server.port = port;

    setupSignalHandlers();

    // start http server (does not return until server is shutting down)
    server.start(mongoose_threads);

    delete g_server;
}
