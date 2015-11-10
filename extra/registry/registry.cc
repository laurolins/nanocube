#include <iostream>
#include <thread>
#include <functional>
#include <vector>
#include <cassert>
#include <map>

// nanomsg for tcp communication
#include <nanomsg/nn.h>
#include <nanomsg/reqrep.h>

// characters
#include "utf8.hh"

using Timestamp = std::time_t;
using Alias     = std::string;
using Key       = std::string;
using Value     = std::string;
using NumBytes  = std::uint64_t;

//------------------------------------------------------------------------------
// Status
//------------------------------------------------------------------------------

enum Status {
    OK=0,
    ALIAS_ALREADY_EXISTS=1,
    ALIAS_DOESNT_EXIST=2,
    UTF8_PARSING_ERROR=3,
};

static const char *status_messages[] {
    "OK",
    "ALIAS_ALREADY_EXISTS",
    "ALIAS_DOESNT_EXIST"
};

//------------------------------------------------------------------------------
// Entry
//------------------------------------------------------------------------------

template<typename T>
struct Expected {

    Expected() = default;
    Expected(T object): _object(object), _status(OK) {}
    Expected(Status not_ok_status): _status(not_ok_status) { assert(not_ok_status!=OK); }

    Status status       () const { return _status; }
    T&     get          ()       { assert(status() == OK); return _object; }
    T&     operator    *()       { return get(); }
           operator bool() const { return _status == OK; }

    T      _object;
    Status _status;
};

//------------------------------------------------------------------------------
// Item
//------------------------------------------------------------------------------

struct Item {

    // item

    Alias       _alias;
    Timestamp   _insertion_time;
    Timestamp   _removal_time;
    std::map<Key, Value> _dictionary;
};


//------------------------------------------------------------------------------
// Collection
//------------------------------------------------------------------------------

//
// Collection of dictionaries with an alias identifier
//

struct Collection {

    Expected<Item*> insert(const Alias& alias);

    Status          remove(const Alias& alias);

    Item*           get(const Alias& alias);
    
    // doesn't need to be very efficient
    std::map<Alias, std::unique_ptr<Item>> _service_map;

};

Collection& collection();

Collection& collection() {
    static Collection collection;
    return collection;
}


//------------------------------------------------------------------------------
// MemoryBlock
//------------------------------------------------------------------------------

struct MemoryBlock {
    MemoryBlock() = default;
    MemoryBlock(const char* begin, const char* end): _begin(begin), _end(end) {}
    
    NumBytes size() const { return _end - _begin; }

    const char* begin() const { return _begin; }
    const char* end() const { return _end; }

    const char* _begin { nullptr };
    const char* _end   { nullptr };
};

//------------------------------------------------------------------------------
// Request
//------------------------------------------------------------------------------


enum RequestType { REQUEST_NONE=0, REQUEST_INSERT=1, REQUEST_REMOVE=2, REQUEST_LIST=3, REQUEST_INFO=4 };
static const char *request_strings[] { "none", "insert", "remove", "list", "info" };
static const int MIN_REQUEST = 1;
static const int MAX_REQUEST = 4;

struct Request {
    Request() = default;
    Request(RequestType type, const MemoryBlock& params): _type(type), _params(params) {}
    RequestType type() const { return _type; }
    const char* type_string() const { return request_strings[_type]; }
    const MemoryBlock& params() const { return _params; }
    RequestType _type { REQUEST_NONE };
    MemoryBlock _params;
};

//------------------------------------------------------------------------------
// Token
//------------------------------------------------------------------------------

struct Token {
    Token() = default;
    Token(const char* begin, const char* end):
        _begin(begin), _end(end)
    {}

    //
    // assuming a null terminated string in st
    // compare with the string from [_begin,_end)
    //
    bool operator==(const char* st) const;

    Token& lstrip(utf8::CodePoint p); // left strip

    const char* begin() const { return _begin; };
    const char* end()   const { return _end; };

    const char* _begin { nullptr };
    const char* _end   { nullptr };
};

bool Token::operator==(const char* st) const {
    auto i = _begin;
    auto j = st; 
    while (i < _end && *j != 0 && *i == *j) {
        ++i;
        ++j;
    }
    return (*j == 0 && i == _end);
}

Token& Token::lstrip(utf8::CodePoint symbol) {
    auto it = _begin;
    utf8::Next n;
    while ( (n = utf8::next(_begin,_end)) ) {
        if (*n == symbol) {
            it = n.next();
        }
        else {
            break;
        }
    }
    _begin = it;
    return *this;
}

Expected<Token> next_token(const char* begin, const char* end, utf8::CodePoint delim) {
    auto it = begin;
    utf8::Next n;
    while ( (n = utf8::next(it,end)) ) {
        if (*n == delim) {
            return Expected<Token>(Token(begin,it));
        }
        else {
            it = n.next();
        }
    }
    if (n.done()) {
        return Expected<Token>(Token(begin,it));
    }
    else {
        return Expected<Token>(UTF8_PARSING_ERROR);
    }
}

Request parse_request_type(const MemoryBlock& block) {
    //
    // service request comes as 
    // <sevice-name>[|<params>]
    //
    auto et = next_token(block.begin(), block.end(), '|');
    if (et) {
        auto &t = *et;
        for (auto i=MIN_REQUEST;i<=MAX_REQUEST;++i) {
            if (t == request_strings[i]) {
                return Request( static_cast<RequestType>(i), 
                                MemoryBlock(t.end() == block.end() ? block.end() : t.end()+1, block.end()) );
            }
        }
    }
    return Request(REQUEST_NONE, block);
}


using Handler = std::function<bool(const MemoryBlock& params)>;

bool none_handler(const MemoryBlock& params) { return true; }

bool insert_handler(const MemoryBlock& params) { 

    std::vector<Token> tokens;

    // read 
    auto it = params.begin()
    while (true) {
        auto et = next_token(it,params.end(),'|');
        if (et) {
            tokens.push_back(*et);
            it = et.next();
        }
        // todo check et.problem()
    }
    
    // parse alias name
    if (tokens.size()) {
        auto &collection = getCollection();
        auto &token = tokens.at(0);
        auto eitem = collection.insert(std::string(token.begin(),token.end()));
        if (eitem) {
            auto item = *i;
            for (auto i=1;i<tokens.size()-1;++i) {
                item->
                
            }
        }
        


    }
    auto et = next_token(params.begin(),params.end(),'|');
    if (et) {
        auto &t = *et; // token

        auto &collection = getCollection();
        auto eitem = collection.insert(std::string(t.begin(),t.end()));
        if (eitem) {
            auto item = *eitem;

            // scan all key value pairs
            


        }
        
        
        


    }
    else {
        return false;
    }
    return true; 
}

bool remove_handler(const MemoryBlock& params) { return true; }
bool list_handler(const MemoryBlock& params) { return true; }
bool info_handler(const MemoryBlock& params) { return true; }

std::vector<Handler> _handlers { none_handler, insert_handler, remove_handler, list_handler, info_handler };

// worker thread to process requests in "parallel"
void worker(int worker_id)
{
    auto socket   = nn_socket(AF_SP, NN_REP); 
    assert(socket>= 0);
    auto endpoint = nn_connect(socket, "inproc://test"); 
    assert(endpoint >= 0);

    char buf [1024*1024]; // 1M per thread (don't expect messages larger than 1M)

    while (1) {

        auto received_bytes = nn_recv (socket, buf, sizeof(buf), 0);
        assert(received_bytes >= 0);

        // parse reqeuest
        auto request = parse_request_type(MemoryBlock(buf,buf + received_bytes));

        auto ok = _handlers[request.type()](request.params());

        std::cerr << "[" << worker_id << "] received request '" << std::string(buf,buf+received_bytes) << "'  ---> " << request.type_string() << std::endl; 
        auto msg = request.type_string();
        auto len = strlen(msg);
        auto sent_bytes = nn_send(socket, msg, len, 0);
        assert(sent_bytes == len);
    }
    nn_shutdown(socket,endpoint);
}

// redirect requests to worker threads
int main() {

    auto frontend_socket   = nn_socket(AF_SP_RAW, NN_REP);  
    assert(frontend_socket >= 0);
    auto frontend_endpoint = nn_bind(frontend_socket, "tcp://127.0.0.1:7777"); 
    assert(frontend_endpoint >= 0);

    auto backend_socket    = nn_socket(AF_SP_RAW, NN_REQ);
    assert(backend_socket >= 0);
    auto backend_endpoint  = nn_bind(backend_socket, "inproc://test");
    assert(backend_endpoint >= 0);

    // start three worker threads
    std::thread t1(worker, 1);
    std::thread t2(worker, 2);
    std::thread t3(worker, 3);

    std::cerr << "Registry is up on port 7777" << std::endl;
    
    auto exit_status = nn_device(frontend_socket, backend_socket);

    nn_shutdown(frontend_socket,frontend_endpoint);
    nn_shutdown(backend_socket,backend_endpoint);

    return 0;
}


