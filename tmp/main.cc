#include <iostream>
#include <sstream>
#include <cstdlib>
#include <csignal>
#include <cmath>
#include <set>
#include <map>
#include <boost/unordered_set.hpp>
#include <boost/unordered_map.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include "event.h"
#include "mongoose.h"
#include "tile_key.h"

using namespace std;

const int max_level = 17;

inline void latlong_to_normal_mercator(double &x, double &y, double lat, double lon)
{
    lon = lon * (M_PI / 180.0);
    lat = lat * (M_PI / 180.0);
    x = (lon + M_PI) / (M_PI * 2);
    y = (log(tan(M_PI / 4.0 + lat / 2.0)) + M_PI) / (M_PI * 2);
}

boost::unordered_map<TileKey, EventSet> tiles;

/******************************************************************************/
// mongoose wrapper

typedef void (*Handler)(const vector<string> &params, struct mg_connection *conn);

map<string, Handler> handlers;

void error(const string &str, struct mg_connection *conn)
{
    mg_printf(conn, "HTTP/1.1 501 Not Implemented\r\nContent-Type: text/plain\r\nContent-Length: %d\r\n\r\n%s",
              (int)str.size(), str.c_str());
}

void *mg_callback(enum mg_event event, struct mg_connection *conn)
{
    if (event == MG_NEW_REQUEST) {
        const struct mg_request_info *request_info = mg_get_request_info(conn);
        string uri = string(request_info->uri);
        vector<string> strs;
        boost::split(strs, uri, boost::is_any_of("/"));
        if (!strs.size()) {
            error(string("bad URL: ") + uri, conn);
            return (void*)"";
        }
        strs.erase(strs.begin());
        if (!strs.size()) {
            error(string("bad URL: ") + uri, conn);
            return (void*)"";
        }
        if (handlers.find(strs[0]) == handlers.end()) {
            error(string("no handler for: ") + uri + string(" (request key: ") + strs[0] + string(")"), conn);
            return (void*)"";
        }
        handlers[strs[0]](strs, conn);
        return (void*)"";
    }
    return NULL;
}

/******************************************************************************/

void build_tiles()
{
    double lat, lon, x, y;
    int type;
    int c = 0;
    while (cin >> type >> lat >> lon) {
        if (++c % 100000 == 0)
            cerr << "Read " << c << "...\n";
        if (lat > 90) break;
        latlong_to_normal_mercator(x, y, lat, lon);
        uint32_t tile_x = (uint32_t) (x * (1 << (max_level + 8)));
        uint32_t tile_y = (uint32_t) ((1 << (max_level + 8)) - (y * (1 << (max_level + 8))) - 1);
        
        for (int i=0; i<max_level; ++i) {
            TileKey tile_k;
            tile_k.x_tile = tile_x >> (max_level + 8 - i);
            tile_k.y_tile = tile_y >> (max_level + 8 - i);
            tile_k.zoom   = i;

            EventKey event_k;
            event_k.x = tile_x >> (max_level - i);
            event_k.y = tile_y >> (max_level - i);

            boost::unordered_map<TileKey, EventSet>::iterator f = tiles.find(tile_k);

            if (f == tiles.end()) {
                EventSet es(3);
                es.add(event_k, type);
                tiles[tile_k] = es;
            } else                
                f->second.add(event_k, type);
        }
    }

    cerr << "done! " << tiles.size() << endl;
}

void ascii_tile(const vector<string> &args, struct mg_connection *conn)
{
    if (args.size() != 4) {
        error("tile expects three parameters; zoom, x, and y", conn);
        return;
    }
    int x = -1, y = -1, zoom = -1;
    zoom = atoi(args[1].c_str());
    x    = atoi(args[2].c_str());
    y    = atoi(args[3].c_str());
    if (x < 0 || y < 0 || zoom < 0) {
        error(string("tile expects non-negative parameters, got ") +
              args[1] + string("/") + args[2] + string("/") + args[3], conn);
        return;
    }

    TileKey tile_k(x, y, zoom);
    boost::unordered_map<TileKey, EventSet>::iterator f = tiles.find(tile_k);
    stringstream ss;
    if (f != tiles.end()) {
        const EventSet &event = f->second;
        for (size_t i=0; i<event.keys.size(); ++i) {
            ss << (int)event.keys[i].x << " " << (int)event.keys[i].y;
            for (size_t j=0; j<event.num_event_types; ++j) {
                ss << " " << event.counts[event.num_event_types * i + j];
            }
            ss << endl;
        }
    }
    string msg = ss.str();
    mg_printf(conn, "HTTP/1.1 200 OK\r\nContent-Type: text/plain\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: %d\r\n\r\n%s",
              (int)msg.size(), msg.c_str());
}

void json_tile(const vector<string> &args, struct mg_connection *conn)
{
    if (args.size() != 4) {
        error("tile expects three parameters; zoom, x, and y", conn);
        return;
    }
    int x = -1, y = -1, zoom = -1;
    zoom = atoi(args[1].c_str());
    x    = atoi(args[2].c_str());
    y    = atoi(args[3].c_str());
    if (x < 0 || y < 0 || zoom < 0) {
        error(string("tile expects non-negative parameters, got ") +
              args[1] + string("/") + args[2] + string("/") + args[3], conn);
        return;
    }

    TileKey tile_k(x, y, zoom);
    boost::unordered_map<TileKey, EventSet>::iterator f = tiles.find(tile_k);
    stringstream ss;
    if (f != tiles.end()) {
        const EventSet &event = f->second;
        ss << "[[";
        for (size_t i=0; i<event.keys.size(); ++i) {
            ss << (i>0?",":"") << (int)event.keys[i].x;
        }
        ss << "],[";
        for (size_t i=0; i<event.keys.size(); ++i) {
            ss << (i>0?",":"") << (int)event.keys[i].y;
        }
        ss << "],[";
        for (size_t i=0; i<event.counts.size(); ++i) {
            ss << (i>0?",":"") << event.counts[i];
        }
        ss << "]] ";
    } else {
        ss << "[[],[],[]]";
    }
    string msg = ss.str();
    mg_printf(conn, "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: %d\r\n\r\n%s",
              (int)msg.size(), msg.c_str());
}

/******************************************************************************/
// on SIGINT, set flag that's checked by loop
int has_intterupted = 0;
void sigint_handler(int s) {
    cout << "Caught signal " << s << endl;
    has_intterupted = 1;
}

int main(int argc, char **argv)
{
    /**************************************************************************/
    // load data
    build_tiles();

    /**************************************************************************/
    // set http handlers
    handlers[string("ascii_tile")] = &ascii_tile;
    handlers[string("json_tile")] = &json_tile;

    /**************************************************************************/
    // set sigint handler to kill server
    struct sigaction action;
    action.sa_handler = sigint_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    sigaction(SIGINT, &action, NULL);
    
    /**************************************************************************/
    // init mongoose
    struct mg_context *ctx;
    const char *options[] = {"listening_ports", "29512", NULL};
    ctx = mg_start(&mg_callback, NULL, options);

    while (!has_intterupted)
        sleep(1);

    mg_stop(ctx);
    return 0;
    // for (map<TileKey, int>::iterator b = tiles.begin(), e = tiles.end();
    //      b !=e; ++b) {
    //     cout << b->second << " " << b->first.x_tile << " " << b->first.y_tile << " " << b->first.zoom << endl;
    //     // counts[b->zoom]++;
    // }
    // for (map<int, int>::iterator b = counts.begin(), e = counts.end();
    //      b != e; ++b) {
    //     cout << b->first << " " << b->second << endl;
    // }
}
