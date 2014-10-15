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
    else if(params[1].compare("tbin")==0){
        uri_original = TBIN;
        uri_translated = TBIN;
        
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
        uri_original      = BIN_QUERY;
        uri_translated    = BIN_QUERY;
        uri_stroriginal   = uri;
        uri_strtranslated = uri;
    }
    else if(params[1].compare("bintquery")==0){
        uri_original      = BIN_TQUERY;
        uri_translated    = BIN_TQUERY;
        uri_stroriginal   = uri;
        uri_strtranslated = uri;
    }
}


static std::string _content_type[] {
    std::string("application/json")         /* 0 */,
    std::string("application/octet-stream") /* 1 */
};

void MasterRequest::respondJson(std::string msg_content)
{
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
}

void MasterRequest::respondOctetStream(const void *ptr, int size)
{

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

Slave::Slave(std::string paddress):
    deamon_port(-1),
    query_port(-1),
    insert_port(-1),
    load(1.0)
{
    //Get ip address
    boost::asio::io_service io_service;
    boost::asio::ip::tcp::resolver resolver(io_service);
    boost::asio::ip::tcp::resolver::query query(paddress, "");
    boost::asio::ip::tcp::endpoint end = *resolver.resolve(query);
    this->address = end.address().to_string();
}

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
  mongoose_threads(10),
  done(false),
  is_timing(false),
  slaves(slaves)
{}

void Master::writeSlave(std::string uri, boost::asio::ip::tcp::socket& socket)
{

    char buffer[1024];
    //std::string uri = request.uri_strtranslated;
    sprintf(buffer, "GET %s HTTP/1.1\n\n", uri.c_str());
    
    boost::asio::write(socket, boost::asio::buffer(buffer));
}

void Master::readSlave(boost::asio::ip::tcp::socket& socket, std::vector<char>* content)
{


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

        response.insert(response.end(), buf, buf+len);
    }

    std::string aux(response.begin(), response.end());
    auto pos = aux.find("\r\n\r\n");

    *content = std::vector<char>(aux.begin()+pos+4,aux.end());

}

void Master::requestAllSlaves(MasterRequest &request)
{

    try
    {
        if(request.uri_translated == MasterRequest::SCHEMA
            || request.uri_translated == MasterRequest::TQUERY
            || request.uri_translated == MasterRequest::TBIN
            || request.uri_translated == MasterRequest::BIN_TQUERY)
        {
            std::vector<char> content;
            boost::asio::io_service io_service;
            boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(slaves[0].address), slaves[0].query_port);
            boost::asio::ip::tcp::socket socket(io_service);

            socket.connect(endpoint);
            writeSlave(request.uri_strtranslated, socket);
            readSlave(socket, &content);

            if(request.uri_translated == MasterRequest::SCHEMA)
            {
                auto schema_st = std::string(content.begin(), content.end());
                std::cerr << schema_st << std::endl;
                request.respondJson(schema_st);
            }
            else if(request.uri_translated == MasterRequest::TQUERY)
            {
                request.respondJson(std::string(content.begin(), content.end()));
            }
            else if(request.uri_translated == MasterRequest::TBIN)
            {
                request.respondJson(std::string(content.begin(), content.end()));
            }
            else if (request.uri_translated == MasterRequest::BIN_TQUERY)
            {
                request.respondOctetStream(&content[0], content.size());
            }

            socket.close();
            return;
        }

        //Instead of storing in each Slave object, we need to do this, because of Mongoose threads
        //(Slave objects are shared between all Mongoose threads)
        std::vector<boost::asio::ip::tcp::socket*> sockets;
        std::vector<boost::asio::ip::tcp::endpoint*> endpoints;
        std::vector<boost::asio::io_service*> io_services;
        std::vector<std::vector<char>*> contents;
        int i = 0;
        for(i = 0; i<slaves.size(); i++)
        {
            boost::asio::io_service* io_service = new boost::asio::io_service();
            boost::asio::ip::tcp::endpoint* endpoint = new boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(slaves[i].address), slaves[i].query_port);
            boost::asio::ip::tcp::socket* socket = new boost::asio::ip::tcp::socket(*io_service);

            sockets.push_back(socket);
            endpoints.push_back(endpoint);
            io_services.push_back(io_service);
        }

#if 0
        //Parallel write/read

        //Scatter
        for(i = 0; i < slaves.size(); i++)
        {
            sockets[i]->connect(*endpoints[i]);
            writeSlave(request.uri_strtranslated, *sockets[i]);
        }

        //Gather threads
        std::vector<std::thread> gather_threads;
        
        for(i = 0; i < slaves.size(); i++)
        {
            std::vector<char>* content = new std::vector<char>();
            contents.push_back(content);

            std::cout << "Gather " << i << std::endl;
            gather_threads.push_back(std::thread(&Master::readSlave, this, *sockets[i], contents[i]));
        }
        
        
        //Join gather threads
        for(i = 0; i < slaves.size(); i++)
        {
            std::cout << "Join " << i << std::endl;
            gather_threads[i].join();
            sockets[i]->close();
        }
#else
        //Serial write/read

        for(i = 0; i < slaves.size(); i++)
        {
            std::vector<char>* content = new std::vector<char>();

            sockets[i]->connect(*endpoints[i]);
            writeSlave(request.uri_strtranslated, *sockets[i]);
            readSlave(*sockets[i], content);
            sockets[i]->close();

            contents.push_back(content);
        }
#endif

        //Aggregate results
        vector::Vector aggregatedVector;
        bool isAggregating = false;
        for(i = 0; i < slaves.size(); i++)
        {
            //auto content = readSlave(i);
            auto content = *contents[i];
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

        //Clean pointers
        for(i = 0; i<slaves.size(); i++)
        {
            delete contents[i];
            delete sockets[i];
            delete endpoints[i];
            delete io_services[i];
        }

        
        
        //
        if(request.uri_original == MasterRequest::QUERY)
        {

            //Convert vector to JSON and respond.
            dumpfile::DumpFileDescription dummydump;
            nanocube::Schema dummyschema(dummydump);
            nanocube::QueryResult queryresult(aggregatedVector, dummyschema);

            std::ostringstream os;
            queryresult.json(os);

            request.respondJson(os.str());
        }
        else if(request.uri_original == MasterRequest::TILE)
        {
            //
            ::query::QueryDescription query_description;

            try {
                std::cerr << request.query_params << std::endl;
                parse(request.query_params, query_description);
            }
            catch (::query::parser::QueryParserException &e) {
                request.respondJson(e.what());
                return;
            }
            std::cerr << query_description.getFirstAnchoredTarget() << std::endl;

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

            using Edge  = ::vector::Edge;
            using Value = ::vector::Value;
            using INode = ::vector::InternalNode;

            uint32_t written_bytes = 0;
            std::ostringstream os;
            if (aggregatedVector.root) {
                INode *inode = aggregatedVector.root->asInternalNode();
                for (auto it: inode->children) {
                    Edge &e = it.second;

                    maps::Tile subtile(e.label);
                    maps::Tile relative_tile = tile.relativeTile(subtile);

                    Value value = e.node->asLeafNode()->value;
                    uint8_t ii = relative_tile.getY();
                    uint8_t jj = relative_tile.getX();

                    // i, j, value
                    os.write((char*) &ii, sizeof(uint8_t));
                    os.write((char*) &jj, sizeof(uint8_t));
                    os.write((char*) &value, sizeof(Value));

                    written_bytes += sizeof(uint8_t) + sizeof(uint8_t) + sizeof(Value);

                } // transfering data to main matrix
            }
        
            request.respondOctetStream(os.str().c_str(), written_bytes);

        }
        else if(request.uri_original == MasterRequest::BIN_QUERY)
        {
            std::stringstream ss;
            vector::serialize(aggregatedVector, ss);
            std::string result = ss.str();

//            std::ofstream data("/tmp/results",std::fstream::out|std::fstream::app);
//            data << request.uri_strtranslated << std::endl;
//            data << "result" << std::endl;
//            for (auto ch: result) {
//                data << std::hex << (unsigned int) ch << " ";
//            }
//            data << std::endl;
            
            request.respondOctetStream(result.c_str(), result.size());
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

    auto &schema = *this->schema.get();

    for (::query::parser::Dimension *dimension: parser.dimensions) {
        try {

            int index = schema.getDimensionIndex(dimension->name);
            bool anchored = dimension->anchored;
            query_description.setAnchor(index, anchored);
            ::query::parser::TargetExpression *target = dimension->target;
            target->updateQueryDescription(index, query_description);

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
            std::cerr << "(master) (Warning) Dimension " << dimension->name << " not found. Disconsidering it." << std::endl;
            continue;
        }
    }
}

void Master::requestSchema()
{

    std::vector<char> strschema;
    boost::asio::io_service io_service;
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(slaves[0].address), slaves[0].query_port);
    boost::asio::ip::tcp::socket socket(io_service);

    // wait 1 second
    sleep(10);

    socket.connect(endpoint);
    writeSlave("/binschema", socket);
    readSlave(socket, &strschema);
    socket.close();

    // read input file description
    std::istringstream iss(std::string(strschema.begin(), strschema.end()));
    iss >> schema_dump_file_descriptions;
    
    // create nanocube_schema from input_file_description
    schema.reset(new nanocube::Schema(schema_dump_file_descriptions));
    
    
    
    //std::cout << "--------- schema -----------" << std::endl;
    //std::cout << schema.get()->dump_file_description << std::endl;
    //std::cout << "----------------------------" << std::endl;
    
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

        std::cerr << "(master) Request URI: " << uri << std::endl;

        

        // tokenize on slahses: first should be the address,
        // second the requested function name, and from
        // third on parameters to the functions
        std::vector<std::string> tokens;
        boost::split(tokens, uri, boost::is_any_of("/"));

        MasterRequest request(conn, tokens, uri);

        if (tokens.size() == 0) {
            std::stringstream ss;
            ss << "Master: bad URL: " << uri;
            request.respondJson(ss.str());
            return  (void*) ""; // mark as processed
        }
        else if (tokens.size() == 1) {
            std::stringstream ss;
            ss << "Master: no handler name was provided on " << uri;
            request.respondJson(ss.str());
            return (void*) ""; // mark as processed
        }

        std::string handler_name = tokens[1];


        if (is_timing) {
            //handlers[handler_name](request);
            //requestSlaves(request);
            requestAllSlaves(request);
            auto t1 = std::chrono::high_resolution_clock::now();
            uint64_t elapsed_nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(t1-t0).count();
            timing_of << currentDateTime() << " " << uri
                      << " " << elapsed_nanoseconds << " ns"
                      << " input " << uri.length()
                      << " output " << request.response_size
                      << std::endl;
        } else {
            std::cerr << "(master) Request URI: " << uri << std::endl;
            std::cerr << "(master) Request handler: " << handler_name << std::endl;
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

void Master::start(int mongoose_threads, int mongoose_port) // blocks current thread
{

    //boost::mpi::communicator mpicom;
    //std::cout << "**Master: I am process " << mpicom.rank() << " of " << mpicom.size() << "." << std::endl;

    //char p[256];
    //sprintf(p,"%d",port);
    //std::string port_st = p;
    this->mongoose_threads = mongoose_threads;
    this->mongoose_port = mongoose_port;

    // port_st.c_str();
    _master = this; // single master
    // auto callback = std::bind(&Server::mg_callback, this, std::placeholders::_1, std::placeholders::_2);

    std::string mongoose_string = std::to_string(mongoose_threads);
    std::string port_string = std::to_string(mongoose_port);
    struct mg_context *ctx;
    const char *options[] = {"listening_ports", port_string.c_str(), "num_threads", mongoose_string.c_str(), NULL};
    ctx = mg_start(&__mg_master_callback, NULL, options);

    if (!ctx) {
        throw MasterException("Couldn't create mongoose context... exiting!");
    }

    // ctx = mg_start(&callback, NULL, options);

    requestSchema();

    std::cerr << "(master) server on port " << mongoose_port << std::endl;
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

