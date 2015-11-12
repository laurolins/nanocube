#include <ctime>
#include <iostream>
#include <thread>
#include <functional>
#include <vector>
#include <cassert>
#include <map>
#include <string>
#include <sstream>
#include <mutex>

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
    KEY_NOT_FOUND=4,
    NOT_IMPLEMENTED=5,
    NEXT_TOKEN_EMPTY=6,
    INSERT_REQUEST_INVALID=7,
    GET_REQUEST_INVALID=8,
    REMOVE_REQUEST_INVALID=9
};

static const char *status_messages[] {
    "OK",
        "ALIAS_ALREADY_EXISTS",
        "ALIAS_DOESNT_EXIST"
        "UTF8_PARSING_ERROR",
        "KEY_NOT_FOUND",
        "NOT_IMPLEMENTED",
        "NEXT_TOKEN_EMPTY",
        "INSERT_REQUEST_INVALID",
        "GET_REQUEST_INVALID",
        "REMOVE_REQUEST_INVALID"
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
    
    struct Iter {
        using iter_t = typename std::map<Key,Value>::const_iterator;
        Iter() = default;
        Iter(const Item& i);
        bool next();
        const Key& key() const { return _result->first; }
        const Value& value() const { return _result->second; }
        iter_t _result;
        iter_t _current;
        iter_t _end;
    };

    Item() = default;
    Item(const Alias& alias): _alias(alias) {}

    Timestamp insertion_time() const { return _insertion_time; }
    Timestamp removal_time() const { return _removal_time; }

    Item& insertion_time(Timestamp t) { _insertion_time=t; return *this; }
    Item& removal_time(Timestamp t) { _removal_time=t; return *this; }

    Expected<Value> get(const Key& key) noexcept;
    void set(const Key& key, const Value& value) noexcept;

    Iter iter() const { return Iter(*this); }

    const Alias& alias() const { return _alias; }
    
    Alias       _alias;
    Timestamp   _insertion_time { 0 }; // we assume 0 is undefined for now
    Timestamp   _removal_time   { 0 }; 

    std::map<Key, Value> _dictionary;

};

Item::Iter::Iter(const Item& item):
    _current(item._dictionary.cbegin()),
    _end(item._dictionary.cend())
{}

bool Item::Iter::next() {
    if (_current == _end)
        return false;
    else {
        _result = _current;
        ++_current;
        return true;
    }
}

Expected<Value> Item::get(const Key& key) noexcept {
    auto it = _dictionary.find(key);
    if (it != _dictionary.end()) {
        return Expected<Value>(it->second);
    }
    else {
        return Expected<Value>(KEY_NOT_FOUND);
    }
}

void Item::set(const Key& key, const Value& value) noexcept {
    _dictionary[key] = value;
}


//------------------------------------------------------------------------------
// Collection
//------------------------------------------------------------------------------

//
// Collection of dictionaries with an alias identifier
//

struct Collection {

    struct Iter {
        using iter_t = typename std::map<Alias,std::unique_ptr<Item>>::const_iterator;
        Iter() = default;
        Iter(const Collection& c);
        const Item* next();
        iter_t _current;
        iter_t _end;
    };

    
    Expected<Item*> insert(const Alias& alias);

    bool            remove(const Alias& alias);

    Item*           get(const Alias& alias);

    Iter            iter() { return Iter(*this); }
    
    // doesn't need to be very efficient
    std::map<Alias, std::unique_ptr<Item>> _current_items;
    std::vector<std::unique_ptr<Item>>     _removed_items;


    std::mutex mutex;
};

Collection::Iter::Iter(const Collection& col):
    _current(col._current_items.cbegin()),
    _end(col._current_items.cend())
{}

const Item* Collection::Iter::next() {
    if (_current == _end)
        return nullptr;
    else {
        auto result = _current->second.get();
        ++_current;
        return result;
    }
}

bool Collection::remove(const Alias& alias) {
    auto it = _current_items.find(alias);
    if (it == _current_items.end()) {
        return false;
    }
    else {
        _current_items.erase(it);
        return true;
    }
}

Expected<Item*> Collection::insert(const Alias& alias) {
    auto it = _current_items.find(alias);
    if (it == _current_items.end()) {
        auto new_item = new Item(alias);
        new_item->insertion_time(std::time(0)); // get current timestamp
        _current_items[alias] = std::unique_ptr<Item>(new_item);
        return Expected<Item*>(new_item);
    }
    else {
        return Expected<Item*>(ALIAS_ALREADY_EXISTS);
    }
}

Item* Collection::get(const Alias& alias) {
    auto it = _current_items.find(alias);
    if (it == _current_items.end()) {
        return nullptr;
    }
    else {
        return it->second.get();
    }
}


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


enum RequestType { REQUEST_NONE=0, REQUEST_INSERT=1, REQUEST_REMOVE=2, REQUEST_GET=3 };
static const char *request_strings[] { "none", "insert", "remove", "get" };
static const int MIN_REQUEST = 1;
static const int MAX_REQUEST = 3;

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

// This is strongly coupled with the next_token function.
struct Token {
    Token() = default;
    Token(const char* begin, const char* end, const char* next):
        _begin(begin), _end(end), _next(next)
    {}

    //
    // assuming a null terminated string in st
    // compare with the string from [_begin,_end)
    //
    bool operator==(const char* st) const;

    Token& lstrip(utf8::CodePoint p); // left strip

    const char* begin() const { return _begin; };
    const char* end()   const { return _end;   };
    const char* next()  const { return _next;  };

    const char* _begin { nullptr };
    const char* _end   { nullptr };
    const char* _next  { nullptr };
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
    if (begin == end)
        return Expected<Token>(NEXT_TOKEN_EMPTY);
    
    auto it = begin;
    utf8::Next n;
    while ( (n = utf8::next(it,end)) ) {
        if (*n == delim) {
            return Expected<Token>(Token(begin,it,it+1));
        }
        else {
            it = n.next();
        }
    }
    if (n.done()) {
        return Expected<Token>(Token(begin,it,end));
    }
    else {
        return Expected<Token>(UTF8_PARSING_ERROR);
    }
}

std::vector<Token> tokenize(const char* begin, const char* end, utf8::CodePoint delim) {
    std::vector<Token> tokens;
    auto it = begin;
    while (true) {
        auto et = next_token(it,end,'|');
        if (et) {
            auto &tok = et.get();
            tokens.push_back(tok);
            it = tok.next();
        }
        else {
            break;
        }
    }
    return tokens;
}


//------------------------------------------------------------------------------
// parse request
//------------------------------------------------------------------------------

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

using Handler = std::function<void(const MemoryBlock& params, std::ostream& ostream)>;

void write_status_and_statusmsg(std::ostream &os, Status status) {
    os << status << '|' << status_messages[status];
}

void none_handler(const MemoryBlock& params, std::ostream& ostream) {
    write_status_and_statusmsg(ostream, NOT_IMPLEMENTED);
}

void remove_handler(const MemoryBlock& params, std::ostream& ostream)  {
    auto tokens = tokenize(params.begin(), params.end(), '|');

    // parse alias name
    if (tokens.size() == 1) {
        auto &col = collection();
        std::lock_guard<std::mutex> lock(col.mutex);
        auto &tok = tokens.at(0);
        auto ok = col.remove(std::string(tok.begin(),tok.end()));
        if (ok) {
            write_status_and_statusmsg(ostream, OK);
        }
        else {
            write_status_and_statusmsg(ostream, ALIAS_DOESNT_EXIST);
        }
    }
    else {
        write_status_and_statusmsg(ostream, REMOVE_REQUEST_INVALID);
    }
}

void insert_handler(const MemoryBlock& params, std::ostream& ostream) { 
    auto tokens = tokenize(params.begin(), params.end(), '|');

    // parse alias name
    if (tokens.size()) {
        auto &col = collection();

        std::lock_guard<std::mutex> lock(col.mutex);

        auto &token = tokens.at(0);
        auto eitem = col.insert(std::string(token.begin(),token.end()));
        if (eitem) {
            auto item = *eitem;
            for (auto i=1;i<tokens.size()-1;i+=2) {
                auto &tkey = tokens.at(i);
                auto &tval = tokens.at(i+1);
                item->set(std::string(tkey.begin(),tkey.end()),
                          std::string(tval.begin(),tval.end()));
            }
            write_status_and_statusmsg(ostream, OK);
            return;
        }
        else {
            write_status_and_statusmsg(ostream, eitem.status());
            return;
        }
    }
    write_status_and_statusmsg(ostream, INSERT_REQUEST_INVALID);
}

void get_handler(const MemoryBlock& params, std::ostream& ostream) {

    auto tokens = tokenize(params.begin(), params.end(), '|');

    //
    // get
    // get|latency
    //
    // return all the current elements of the collection:
    //
    // alias1|key1_1|value1_1|key1_2|value1_2|...|key1_n|value1_n\n
    // alias2|key2_1|value2_1|key2_2|value2_2|...|key2_n|value2_n\n
    // ...
    // alias2|key2_1|value2_1|key2_2|value2_2|...|key2_n|value2_n\n
    //

    if (tokens.size() == 0) {
        auto &col = collection();

        // @todo replace with shared mutex
        std::lock_guard<std::mutex> lock(col.mutex);

        ostream << OK << '|';

        auto iter = collection().iter();
        const Item* item = nullptr;
        while( (item = iter.next())) {
            ostream << item->alias();
            auto iter_dict = item->iter();
            while (iter_dict.next()) {
                ostream << '|' << iter_dict.key() << '|' << iter_dict.value();
            }
            ostream << '\n';
        }
    }
    else if (tokens.size() == 1) {
        auto &col = collection();

        // @todo replace with shared mutex
        std::lock_guard<std::mutex> lock(col.mutex);

        auto alias = tokens.at(0);
        auto item  = col.get(std::string(alias.begin(),alias.end()));
        if (item) {
            ostream << OK << '|';

            ostream << item->alias();
            auto iter_dict = item->iter();
            while (iter_dict.next()) {
                ostream << '|' << iter_dict.key() << '|' << iter_dict.value();
            }
            ostream << '\n';
            return;
        }
        else {
            write_status_and_statusmsg(ostream,ALIAS_DOESNT_EXIST);
            return;
        }
    }
    else {
        write_status_and_statusmsg(ostream,GET_REQUEST_INVALID);
    }
}

std::vector<Handler> _handlers { none_handler, insert_handler, remove_handler, get_handler };

// worker thread to process requests in "parallel"
void worker(int worker_id)
{
    auto socket   = nn_socket(AF_SP, NN_REP); 
    assert(socket>= 0);
    auto endpoint = nn_connect(socket, "inproc://test"); 
    assert(endpoint >= 0);

    char buf [1024*1024]; // 1M per thread (don't expect messages larger than 1M)

    std::stringstream ss;

    while (1) {

        auto received_bytes = nn_recv (socket, buf, sizeof(buf), 0);
        assert(received_bytes >= 0);

        // parse reqeuest
        auto request = parse_request_type(MemoryBlock(buf,buf + received_bytes));

        //
        // @todo: to avoid copying...
        // move the back-end to a custom implementation
        //
        ss.str("");
        ss.clear();
        
        std::cerr << "[" << worker_id << "] received request '" << std::string(buf,buf+received_bytes) << "'  ---> " << request.type_string() << std::endl; 

        // process request
        _handlers[request.type()](request.params(), ss);

        // inneficient!!
        auto st = ss.str();
        std::cerr << "[" << worker_id << "] response: '" << st << "'" << std::endl; 

        auto sent_bytes = nn_send(socket, st.c_str(), st.length(), 0);
        assert(sent_bytes == st.length());
    }
    nn_shutdown(socket,endpoint);
}

// redirect requests to worker threads
int main(int argc, char** argv) {

    int port = 29999;
    if (argc > 1) {
        try {
            port = std::stoi(argv[1]);
        }
        catch(...) {
            assert(0 && "invalid port number");
        }
    }

    std::stringstream ss;
    ss << "tcp://*:" << port;
    auto server = ss.str();
    
    auto frontend_socket   = nn_socket(AF_SP_RAW, NN_REP);  
    assert(frontend_socket >= 0);

    auto frontend_endpoint = nn_bind(frontend_socket, server.c_str()); 
    assert(frontend_endpoint >= 0);

    auto backend_socket    = nn_socket(AF_SP_RAW, NN_REQ);
    assert(backend_socket >= 0);
    auto backend_endpoint  = nn_bind(backend_socket, "inproc://test");
    assert(backend_endpoint >= 0);

    // start three worker threads
    std::thread t1(worker, 1);
    std::thread t2(worker, 2);
    std::thread t3(worker, 3);

    std::string version = "0.0.1-2015.11.10";
    std::cerr << "nanocube-registry v." << version << std::endl;
    std::cerr << "Listening on port " << port << "..." << std::endl;
    
    auto exit_status = nn_device(frontend_socket, backend_socket);

    nn_shutdown(frontend_socket,frontend_endpoint);
    nn_shutdown(backend_socket,backend_endpoint);

    return 0;
}


