#include <chrono>

#include <thread>
#include <iostream>
#include <algorithm>
#include <iomanip>
#include <unistd.h>
#include <vector>
#include <unordered_map>
#include <sstream>
#include <string>

#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
// #include <boost/serialization/vector.hpp>
#include <boost/asio.hpp>

#include "mongoose.h"

#include "Master.hh"
#include "vector.hh"
#include "NanoCubeQueryResult.hh"
#include "NanoCubeSchema.hh"
#include "DumpFile.hh"
#include "maps.hh"
#include "QueryParser.hh"
#include "Query.hh"


//-------------------------------------------------------------------------
// Master Request Impl.
//-------------------------------------------------------------------------

template <typename Iter>
std::string join(Iter begin, Iter end, std::string const& separator)
{
    std::ostringstream result;
    if (begin != end)
        result << *begin++;
    while (begin != end)
        result << separator << *begin++;
    return result.str();
}

MasterRequest::MasterRequest(mg_connection *conn, const std::vector<std::string> &params, std::string uri):
    conn(conn), params(params), response_size(0)
{

    // std::string query_st = "src=[qaddr(0,0,2),qaddr(0,0,2)]/@dst=qaddr(1,0,1)+1";
    //std::string query_st = ss.str();

    // log
    // std::cout << query_st << std::endl;

//    ::query::QueryDescription query_description;
//    try {
//        this->parse(query_st, nanocube.schema, query_description);
//    }
//    catch (::query::parser::QueryParserException &e) {
//        request.respondJson(e.what());
//        return;
//    }

    query_params = join(params.begin()+2, params.end(), "/");


    if(params[1].compare("schema")==0){
        uri_original = SCHEMA;
        uri_translated = SCHEMA;

        uri_stroriginal   = uri;
        uri_strtranslated = uri;
    }
    else if(params[1].compare("tile")==0){
        uri_original = TILE;
        uri_translated = BIN_QUERY;

        uri_stroriginal = uri;
        uri_strtranslated = uri;
        uri_strtranslated.replace(1,4, "binquery");
    }
    else if(params[1].compare("tquery")==0){
        uri_original = TQUERY;
        uri_translated = TQUERY;

        uri_stroriginal = uri;
        uri_strtranslated = uri;
    }
    else if(params[1].compare("query")==0){
        uri_original = QUERY;
        uri_translated = BIN_QUERY;

        uri_stroriginal = uri;
        uri_strtranslated = uri;
        uri_strtranslated.replace(1,5, "binquery");
    }
    else if(params[1].compare("binquery")==0){
        uri_original = BIN_QUERY;
        uri_translated = BIN_QUERY;

        uri_stroriginal = uri;
        uri_strtranslated = uri;
        uri_strtranslated.replace(1,5, "binquery");
    }
}


static std::string _content_type[] {
    std::string("application/json")         /* 0 */,
    std::string("application/octet-stream") /* 1 */
};

void MasterRequest::respondJson(std::string msg_content)
{
    //std::cout << "===start MasterRequest::respondJson===" << std::endl;

    const std::string sep = "\r\n";

    std::stringstream ss;
    ss << "HTTP/1.1 200 OK"                << sep
       << "Content-Type: application/json" << sep
       << "Access-Control-Allow-Origin: *" << sep
       << "Content-Length: %d"             << sep << sep
       << "%s";

    response_size = 0 + (int) msg_content.size();
    //response_size = 106 + (int) size; // banchmark data transfer
    // check how a binary stream would work here
    mg_printf(conn, ss.str().c_str(), (int) msg_content.size(), msg_content.c_str());

    
    //std::cout << ss.str() << std::endl;
    //printf(ss.str().c_str(), (int) msg_content.size(), msg_content.c_str());
    //std::cout << "===end MasterRequest::respondJson===" << std::endl;
}

void MasterRequest::respondOctetStream(const void *ptr, int size)
{

    //std::cout << "===start MasterRequest::respondOctetStream===" << std::endl;

    const std::string sep = "\r\n";

    std::stringstream ss;
    ss << "HTTP/1.1 200 OK"                        << sep
       << "Content-Type: application/octet-stream" << sep
       << "Access-Control-Allow-Origin: *"         << sep
       << "Content-Length: %d"                     << sep << sep;

    response_size = 0;
    // check how a binary stream would work here
    mg_printf(conn, ss.str().c_str(), size);


    if (ptr && size > 0) { // unsage access to nullptr
        mg_write(conn, ptr, size);
        response_size += size;
    }

    //std::cout << "===end MasterRequest::respondOctetStream===" << std::endl;
}


const char* MasterRequest::getHeader(char* headername)
{
    return mg_get_header(conn, headername);
}


//-------------------------------------------------------------------------
// MasterException
//-------------------------------------------------------------------------

MasterException::MasterException(const std::string &message):
    std::runtime_error(message)
{}


//-------------------------------------------------------------------------
// Slave
//-------------------------------------------------------------------------

Slave::Slave(std::string address):
    address(address),
    deamon_port(-1),
    query_port(-1),
    insert_port(-1)
{}

/*
void Slave::setResponse(std::string res)
{
    response = res;

    //Parse response
    int pos0 = response.find("Content-Type");
    int pos1 = response.find("\r\n", pos0);
    content_type = response.substr(pos0+14, pos1-pos0-14);

    pos0 = response.find("Content-Length");
    pos1 = response.find("\r\n", pos0);
    content_length = std::stoi(response.substr(pos0+16, pos1-pos0-16));

    content = response.substr(response.length()-content_length, content_length);
}
*/

//-------------------------------------------------------------------------
// Master
//-------------------------------------------------------------------------

Master::Master(std::vector<Slave> slaves):
  port(29511),
  mongoose_threads(10),
  done(false),
  is_timing(false),
  slaves(slaves)
{}

void Master::processSlave(MasterRequest &request)
{
    //Client: Slave
    //Server: Master
}

std::vector<char> Master::requestSlave(std::string uri, Slave &slave)
{
    //Client: Master
    //Server: Slave

    
    std::cout << "Connecting to slave..." << std::endl;

    boost::asio::io_service io_service;
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(slave.address), slave.query_port);
    boost::asio::ip::tcp::socket socket(io_service);

    socket.connect(endpoint);

    char buffer[1024];
    //std::string uri = request.uri_strtranslated;
    sprintf(buffer, "GET %s HTTP/1.1\n\n", uri.c_str());
    
    boost::asio::write(socket, boost::asio::buffer(buffer));

    std::vector<char> response;
    for (;;)
    {
        char buf[1024];
        boost::system::error_code error;

        size_t len = socket.read_some(boost::asio::buffer(buf), error);

        if (error == boost::asio::error::eof)
            break; // Connection closed cleanly by peer.
        else if (error)
            throw boost::system::system_error(error); // Some other error.

        //std::cout.write(buf, len);
        response.insert(response.end(), buf, buf+len);
    }

    socket.close();
    std::cout << "Finished connection..." << std::endl;

    std::string aux(response.begin(), response.end());
    auto pos = aux.find("\r\n\r\n");

    return std::vector<char>(aux.begin()+pos+4,aux.end());

//    int pos0 = response.find("Content-Type");
//    int pos1 = response.find("\r\n", pos0);
//    content_type = response.substr(pos0+14, pos1-pos0-14);

//    pos0 = response.find("Content-Length");
//    pos1 = response.find("\r\n", pos0);
//    content_length = std::stoi(response.substr(pos0+16, pos1-pos0-16));

//    content = response.substr(response.length()-content_length, content_length);

//    auto pos = response.begin();
//    bool new_line = false;
//    while (pos != response.end()) {
//        if (new_line && *pos == '\n') {
//            ++pos;
//            break;
//        }
//        else if (*pos == '\n') {
//            new_line = true;
//        }
//        else {
//            new_line = false;
//        }
//        ++pos;
//    }

    //slave.setResponse(response);
    //return std::vector<char>(pos,response.end());

    // if(type.compare("application/json") == 0)
    // {
    //     request.respondJson(content);
    // }
    // else
    // {
    //     request.respondOctetStream(content.c_str(), content.size()); 
    // } 


}

void Master::requestAllSlaves(MasterRequest &request)
{

    try
    {
        int i=0;
        vector::Vector aggregatedVector;
        bool isAggregating = false;
        for(i = 0; i<slaves.size(); i++)
        {
            auto content = requestSlave(request.uri_strtranslated, slaves[i]);

            if(request.uri_translated == MasterRequest::SCHEMA)
            {
                auto schema_st = std::string(content.begin(), content.end());
                std::cerr << schema_st << std::endl;
                request.respondJson(schema_st);
                break;
            }
            else if(request.uri_translated == MasterRequest::TQUERY)
            {
                request.respondJson(std::string(content.begin(), content.end()));
                break;
            }
            else
            {
                //content is binary. Create vector and apply operations.
                std::istringstream is(std::string(content.begin(), content.end()));
                auto result = vector::deserialize(is);

                //First time
                if(isAggregating == false)
                {
                    aggregatedVector = result;
                    isAggregating = true;
                }
                else
                {
                    aggregatedVector = aggregatedVector + result;
                }
            }
        }

        if(request.uri_original == MasterRequest::BIN_QUERY)
        {
            // request.respondOctetStream(aggregatedVector);
        }
        else if(request.uri_original == MasterRequest::QUERY)
        {

            //Convert vector to JSON and respond.
            dumpfile::DumpFileDescription dummydump;
            nanocube::Schema dummyschema(dummydump);
            nanocube::QueryResult queryresult(aggregatedVector, dummyschema);

            std::ostringstream os;
            queryresult.json(os);

            //std::cout << os.str() << std::endl;

            request.respondJson(os.str());
        }
        else if(request.uri_original == MasterRequest::TILE)
        {


#if 1
            //
            ::query::QueryDescription query_description;

            try {
                std::cout << request.query_params << std::endl;
                parse(request.query_params, query_description);
            }
            catch (::query::parser::QueryParserException &e) {
                request.respondJson(e.what());
                return;
            }
            std::cout << query_description.getFirstAnchoredTarget() << std::endl;

            ::maps::Tile tile;
            auto *target = query_description.getFirstAnchoredTarget();
            if (target) {
                auto *find_and_dive_target = target->asFindAndDiveTarget();
                if (find_and_dive_target) {
                    ::query::RawAddress raw_address = find_and_dive_target->base;
                    tile = maps::Tile(raw_address);
                }
                else {
                    request.respondJson("problem1");
                    return;
                }
            }
            else {
                request.respondJson("problem2");
                return;
            }

            //
#endif


            using Edge  = ::vector::Edge;
            using Value = ::vector::Value;
            using INode = ::vector::InternalNode;

            uint32_t written_bytes = 0;
            std::ostringstream os;
            if (aggregatedVector.root) {
                INode *inode = aggregatedVector.root->asInternalNode();
                for (auto it: inode->children) {
                    Edge &e = it.second;

#if 1
                    maps::Tile subtile(e.label);
                    maps::Tile relative_tile = tile.relativeTile(subtile);

                    Value value = e.node->asLeafNode()->value;
                    uint8_t ii = relative_tile.getY();
                    uint8_t jj = relative_tile.getX();
#else

                    maps::Tile subtile(e.label);

                    Value value = e.node->asLeafNode()->value;
                    uint8_t ii = subtile.getY() - ((subtile.getY() >> 7) << 7);
                    uint8_t jj = subtile.getX() - ((subtile.getX() >> 7) << 7);
#endif


                    // i, j, value
                    os.write((char*) &ii, sizeof(uint8_t));
                    os.write((char*) &jj, sizeof(uint8_t));
                    os.write((char*) &value, sizeof(Value));

                    written_bytes += sizeof(uint8_t) + sizeof(uint8_t) + sizeof(Value);

                } // transfering data to main matrix
            }

            request.respondOctetStream(os.str().c_str(), written_bytes);
        }

    }
    catch (std::exception& e)
    {
        std::cerr << "Master exception: " << e.what() << std::endl;
    }
    
    
    // //open connection to slave and request data

    // //mpicom.barrier();
    // //std::cout << mpicom.size() << std::endl;
    // boost::mpi::communicator mpicom;
    // boost::mpi::request mpireqs[2];

    // //std::cout << mpicom.rank() << std::endl;

    // std::string outmsg = request.getURI();
    // std::cout << "Master: Requesting " << outmsg << std::endl;
    // mpireqs[0] = mpicom.send(1, 0, outmsg);
    // std::vector<char> inmsg;
    // inmsg.reserve(100000);
    // mpireqs[1] = mpicom.recv(1, 1, inmsg);
    // boost::mpi::wait_all(mpireqs, mpireqs+2);

    // char* data = inmsg.data();
    // ++data;
    // int size = inmsg.size()-1;
    // std::cout << "Master: Reply content " << data << std::endl;

    // if(inmsg.at(0) == '0')
    //     request.respondJson(data, size);
    // else
    //     request.respondOctetStream(data, size);

    //
    //mpicom.barrier();
    // try
    // {
    //     boost::asio::io_service io_service;
    //     tcp::resolver resolver(io_service);

    //     tcp::resolver::query query("localhost", "29512");
    //     tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

    //     tcp::socket socket(io_service);
    //     boost::asio::connect(socket, endpoint_iterator);

    //     //Send request
    //     acceptor.accept(socket); // 3
    //     std::string message("Hello from server\n");
    //     boost::asio::write(socket, boost::asio::buffer(message)); // 4
    //     socket.close(); // 5

    //     for (;;)
    //     {
    //         char buf[128];
    //         boost::system::error_code error;

    //         size_t len = socket.read_some(boost::asio::buffer(buf), error);

    //         if (error == boost::asio::error::eof)
    //             break; // Connection closed cleanly by peer.
    //         else if (error)
    //             throw boost::system::system_error(error); // Some other error.

    //         std::cout.write(buf, len);
    //     }
    // }
    // catch (std::exception& e)
    // {
    //     std::cerr << e.what() << std::endl;
    // }
        

    // //printf("Connection established.\n");

    // //request.printInfo();

    // //Send request to slave
    // char buffer[1024];
    // sprintf(buffer, "GET %s HTTP/1.1\n\n", request.getURI());
    // send(sockfd, buffer, strlen(buffer), 0);    
    // //std::cout << "Packet: " << buffer << std::endl;
    
    // std::string rec;
    // int bytes_read = 0;
    // int total_read = 0;
    // char type = 0; //0 (json), 1 (octet)
    // int size = 0;
    // do
    // {
    //     bzero(buffer, sizeof(buffer));
    //     bytes_read = recv(sockfd, buffer, sizeof(buffer), 0);
    //     rec.append(buffer, 0, bytes_read);
    //     //printf("%s\n", buffer);
    //     //printf("%d\n", bytes_read);

    //     total_read += bytes_read;

    //     //printf(".. %d\n", buffer);
    //     //printf("%02x %02x %02x %02x\n", buffer[0], buffer[1], buffer[2], buffer[3]);
    //     //printf(".... %d\n", total_read);

    //     if(total_read >= 1)
    //     {
    //         type = atoi(rec.substr(0, 1).c_str());
    //         printf("Type: %s\n", rec.substr(0, 1).c_str());
    //         printf("%s\n", buffer);
    //     }
    //     if(total_read >= 5)
    //     {
    //         printf("%d\n", rec.length());
    //         const char* bytes = rec.substr(1, 5).c_str();
    //         printf("Size: %d\n", bytes);
    //         size = ((unsigned char)bytes[0] << 24) |
    //          ((unsigned char)bytes[1] << 16) |
    //           ((unsigned char)bytes[2] << 8) |
    //            (unsigned char)bytes[3];
    //         printf("Size: %s\n", size);
    //         //printf("%s\n", buffer);
            
    //     }
    // }
    // while(bytes_read > 0);
    // //while ( buffer[bytes_read-1] != '\0' );

    // //rec = rec.substr(0, rec.length()-1);

    // printf("******************Start**************\n");
    // printf("%s\n", rec.c_str());
    // printf("******************End**************\n");

    // /*
    // int pos0 = rec.find("Content-Type");
    // int pos1 = rec.find("\r\n", pos0);
    // std::string type = rec.substr(pos0+14, pos1-pos0-14);
    // //printf("**** %s *****", type.c_str());

    // pos0 = rec.find("Content-Length");
    // pos1 = rec.find("\r\n", pos0);
    // int length = std::stoi(rec.substr(pos0+16, pos1-pos0-16));
    // //printf("!!!! %d !!!!!", length);

    // //pos0 = rec.find("\r\n\r\n");
    // //pos1 = rec.find("\n", pos0);
    // std::string content = rec.substr(rec.length()-length, length);
    // //printf("**** %s *****", content.c_str());

    // //pos0 = rec.find("Content-Type");
    // //pos1 = rec.find("\\n", pos1);
    // //std::string content = rec.substring();
    // */


    // //std::string type = rec.substr(0, 4);
    // //printf("**** %s *****", type.c_str());

    // std::string content = rec.substr(4, rec.length());
    // //printf("**** %s *****", content.c_str());

    // //Close connection
    // close(sockfd);
    // printf("Connection closed.\n");

    // //std::string aux ("{\"menu\": {\"id\": \"file\",\"value\": \"File\",\"popup\": {\"menuitem\": [{\"value\": \"New\", \"onclick\": \"CreateNewDoc()\"},{\"value\": \"Open\", \"onclick\": \"OpenDoc()\"},{\"value\": \"Close\", \"onclick\": \"CloseDoc()\"}]}}}");

    // if(type == 0)
    // {
    //     request.respondJson(content);
    // }
    // else
    // {
    //     printf("******************Start: %d**************\n", content.length());
    //     printf("%s\n", rec.c_str());
    //     printf("******************End: %d**************\n", content.length());
    //     request.respondOctetStream(content.c_str(), content.length());
    // }
    

}

void Master::parse(std::string              query_st,
                   ::query::QueryDescription &query_description)
{
    ::query::parser::QueryParser parser;
    parser.parse(query_st);

    //std::cout << ::query::parser::Print(parser) << std::endl;
    
    // std::cout << schema_dump_file_descriptions << std::endl;

    //return;

    auto &schema = *this->schema.get();

    for (::query::parser::Dimension *dimension: parser.dimensions) {
        try {

            // std::cout << dimension->name << std::endl;
            // std::cout << dimension->target << std::endl;

            int index = schema.getDimensionIndex(dimension->name);
            // std::cout << "Index of dimension " << dimension->name << " is " << index << std::endl;

            // std::cout << "3" << std::endl;

            bool anchored = dimension->anchored;

            // std::cout << "4" << std::endl;

            query_description.setAnchor(index, anchored);

            // std::cout << "5" << std::endl;

            ::query::parser::TargetExpression *target = dimension->target;

            // std::cout << "6" << std::endl;

            target->updateQueryDescription(index, query_description);

            // std::cout << "7" << std::endl;

            RawAddress replacement = 0ULL;
            { // replace raw addresses ~0ULL;
                auto field    = schema.dump_file_description.getFieldByName(dimension->name);
                // if it is a categorical field replacement will be 2^(bytes * 8) - 1;
                if (field->field_type.name.find("nc_dim_cat_") == 0) {
                    replacement = (1ULL << (field->getNumBytes() * 8)) - 1;
                }
            }
            
            query_description.targets[index]->replace(~0ULL, replacement);
            

        }
        catch(dumpfile::DumpFileException &e) {
            std::cout << "(Warning) Dimension " << dimension->name << " not found. Disconsidering it." << std::endl;
            continue;
        }
    }

    // std::cout << "10" << std::endl;
}

void Master::requestSchema()
{
    auto strschema = requestSlave("/binschema", slaves[0]);

    // read input file description
    std::istringstream iss(std::string(strschema.begin(), strschema.end()));
    iss >> schema_dump_file_descriptions;
    
    // create nanocube_schema from input_file_description
    schema.reset(new nanocube::Schema(schema_dump_file_descriptions));
    
    
    
    std::cout << "--------- schema -----------" << std::endl;
    std::cout << schema.get()->dump_file_description << std::endl;
    std::cout << "----------------------------" << std::endl;
    
}

void* Master::mg_callback(mg_event event, mg_connection *conn)
{ // blocks current thread

    if (event == MG_NEW_REQUEST) {

        std::chrono::time_point<std::chrono::high_resolution_clock> t0;
        if (is_timing) {
            t0 = std::chrono::high_resolution_clock::now();
        }

        const struct mg_request_info *request_info = mg_get_request_info(conn);

        std::string uri(request_info->uri);

        std::cout << "Master: Request URI: " << uri << std::endl;

        

        // tokenize on slahses: first should be the address,
        // second the requested function name, and from
        // third on parameters to the functions
        std::vector<std::string> tokens;
        boost::split(tokens, uri, boost::is_any_of("/"));

        MasterRequest request(conn, tokens, uri);

        if (tokens.size() == 0) {
            // std::cout << "Request URI: " << uri << std::endl;
            std::stringstream ss;
            ss << "Master: bad URL: " << uri;
            request.respondJson(ss.str());
            return  (void*) ""; // mark as processed
        }
        else if (tokens.size() == 1) {
            // std::cout << "Request URI: " << uri << std::endl;
            std::stringstream ss;
            ss << "Master: no handler name was provided on " << uri;
            request.respondJson(ss.str());
            return (void*) ""; // mark as processed
        }

        std::string handler_name = tokens[1];
        //std::cout << "Searching handler: " << handler_name << std::endl;


        if (is_timing) {
            //handlers[handler_name](request);
            //requestSlaves(request);
            auto t1 = std::chrono::high_resolution_clock::now();
            uint64_t elapsed_nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(t1-t0).count();
            timing_of << currentDateTime() << " " << uri
                      << " " << elapsed_nanoseconds << " ns"
                      << " input " << uri.length()
                      << " output " << request.response_size
                      << std::endl;
            // std::cout << "Request URI: " << uri << std::endl;
        } else {
            std::cout << "Master: Request URI: " << uri << std::endl;
            std::cout << "Master: Request handler: " << handler_name << std::endl;
            //handlers[handler_name](request);
            requestAllSlaves(request);
            //request.printInfo();
            //request.respondJson("blablabla");
        }
        
        return (void*) "";
    }
    return 0;
}

static Master *_master;
void* __mg_master_callback(mg_event event, mg_connection *conn)
{
    return _master->mg_callback(event, conn);
}

void Master::start(int mongoose_threads) // blocks current thread
{

    //Request schema
    requestSchema();

    //boost::mpi::communicator mpicom;
    //std::cout << "**Master: I am process " << mpicom.rank() << " of " << mpicom.size() << "." << std::endl;

    char p[256];
    sprintf(p,"%d",port);
    std::string port_st = p;
    this->mongoose_threads = mongoose_threads;

    // port_st.c_str();
    _master = this; // single master
    // auto callback = std::bind(&Server::mg_callback, this, std::placeholders::_1, std::placeholders::_2);

    std::string mongoose_string = std::to_string(mongoose_threads);
    struct mg_context *ctx;
    const char *options[] = {"listening_ports", port_st.c_str(), "num_threads", mongoose_string.c_str(), NULL};
    ctx = mg_start(&__mg_master_callback, NULL, options);

    if (!ctx) {
        throw MasterException("Couldn't create mongoose context... exiting!");
    }

    // ctx = mg_start(&callback, NULL, options);

    std::cout << "Master server on port " << port << std::endl;
    while (!done) // this thread will be blocked
        std::this_thread::sleep_for(std::chrono::seconds(1));

    // sleep(1);

    mg_stop(ctx);
}

void Master::stop()
{
  done = true;
}

// Get current date/time, format is YYYY-MM-DD.HH:mm:ss
const std::string Master::currentDateTime() {
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    // Visit http://www.cplusplus.com/reference/clibrary/ctime/strftime/
    // for more information about date/time format
    strftime(buf, sizeof(buf), "%Y-%m-%d_%X", &tstruct);

    return buf;
}

