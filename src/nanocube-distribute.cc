
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>

#include <boost/asio.hpp>

#include "tclap/CmdLine.h"


#include "Master.hh"
#include "NanoCubeSchema.hh"
#include "DumpFile.hh"


struct Options {
    Options(std::vector<std::string>& args);

    TCLAP::CmdLine cmd_line { "Nanocube Distribute - distribute process", ' ', "2.3", true };

    // -h or --hosts
    TCLAP::ValueArg<std::string> hosts_file {  
            "h",              // flag
            "hosts",         // name
            "Hosts file, containing address:port [q] [X], where [q] specifies a query-only host and [X] is a float number in the interval [0, 1.0], with the dataset porcentage going  to that host.", // description
            true,            // required
            "",               // value
            "hosts-filename" // type description
            };

    // -b or --block
    TCLAP::ValueArg<int> block_size {  
            "b",              // flag
            "block",         // name
            "Block size, to be sent to each host.", // description
            false,            // required
            10000,               // value
            "block-size" // type description
            };

    // -q or --query
    TCLAP::ValueArg<int> query_port {  
            "q",              // flag
            "query",         // name
            "Query port.", // description
            false,            // required
            29512,               // value
            "query-port" // type description
            };

    // -o or --query-only
    TCLAP::SwitchArg query_only {  
            "o",              // flag
            "query-only",         // name
            "Only offer query.", // description
            false               // value
            };
    
    TCLAP::ValueArg<int> no_mongoose_threads {
        "t",              // flag
        "threads",        // name
        "Number of threads for querying (mongoose)",     // description
        false,                                 // required
        10,                                   // value
        "threads"                         // type description
    };


};

Options::Options(std::vector<std::string>& args) {
    cmd_line.add(hosts_file); // add command option
    cmd_line.add(block_size);
    cmd_line.add(query_port);
    cmd_line.add(query_only);
    cmd_line.add(no_mongoose_threads);
    cmd_line.parse(args);
}


namespace {

    std::istream* input_stream = nullptr;

}


dumpfile::DumpFileDescription input_file_description;

//------------------------------------------------------------------------------
// wakeSlave
// Steps:
// 1 - Connect to deamon
// 2 - Send schema
// 3 - Wait for slave port
// 4 - Connect to slave
// 5 - Ready to send data
//------------------------------------------------------------------------------

void wakeSlave(Slave& slave, dumpfile::DumpFileDescription schema)
{
    // //1 - Connect to deamon
    std::cerr << "(distribute) Connecting to deamon " << slave.address << ":" << slave.deamon_port << std::endl;

    boost::asio::io_service io_service;
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(slave.address), slave.deamon_port);
    boost::asio::ip::tcp::socket socket(io_service);

    socket.connect(endpoint);

    //2 - Send schema
    std::cerr << "(distribute) Sending schema to deamon... " <<  std::endl;
    std::ostringstream os;
    os << schema;
    boost::asio::write(socket, boost::asio::buffer(os.str()));
    socket.close();

    // Do I need to close a socket and then re-connect it to make sure the deamon is not blocked at recv waiting for more?
    socket.connect(endpoint);

    //3 - Wait for slave port
    std::cerr << "(distribute) Waiting for deamon to send ncserve query and insert ports... " <<  std::endl;
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

    //Parse response
    int pos0 = aux.find(":");
    std::string status = aux.substr(0, pos0);

    int pos1 = aux.find(":", pos0+1);
    int insert_port = atoi(aux.substr(pos0+1, pos1-pos0).c_str());

    int query_port = atoi(aux.substr(pos1+1, aux.length()-pos1).c_str());

    if(status.compare("ERROR") == 0)
    {
        std::cerr << "(distribute) Deamon returned an error." << std::endl;
        return;
    }

    std::cerr << "(distribute) ncserve insert port: " << insert_port <<  ", query port: " << query_port << std::endl;
    slave.insert_port = insert_port;
    slave.query_port = query_port;
}

//------------------------------------------------------------------------------
// sendToSlave
//------------------------------------------------------------------------------
bool sendToSlave(Slave& slave, int batch_size)
{

    //remove
    //slave.slave_insert_port = port;

    //1 - Connect to slave
    std::cerr << "(distribute) Connecting to slave " << slave.address << ":" << slave.insert_port << std::endl;

    boost::asio::io_service io_service;
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(slave.address), slave.insert_port);
    boost::asio::ip::tcp::socket socket(io_service);

    socket.connect(endpoint);

    //2 - Send data
    std::cerr << "(distribute) Sending data to slave... " <<  std::endl;

    std::istream &is = *input_stream;
    
    int records_sent = 0;
    bool finished_input = false;
    auto record_size = input_file_description.record_size;
    auto num_bytes_per_batch = record_size * batch_size;
    char buffer[num_bytes_per_batch];
    //while (1) {
        is.read(buffer,num_bytes_per_batch);
        //std::cout << buffer << std::endl;
        if (!is) {
            auto gcount = is.gcount();
            if (gcount > 0) {
                boost::asio::write(socket, boost::asio::buffer(buffer, gcount));
            }
            finished_input = true;
            //break;
        }
        else {
            boost::asio::write(socket, boost::asio::buffer(buffer, num_bytes_per_batch));
        }
        records_sent += batch_size;
    //}

    std::cerr << "(distribute) Records sent: " << records_sent << std::endl;

    socket.close();

    std::cerr << "(distribute) Finished" <<  std::endl;

    return finished_input;
}

//------------------------------------------------------------------------------
// initScatter
//------------------------------------------------------------------------------
void initScatter(Options& options, std::vector<Slave>& slaves)
{

    std::cerr << "(distribute) Initializing scattering..." << std::endl;

    //Read schema
    //std::cout << std::cin.rdbuf();

    std::istream &is = *input_stream;
    
    is >> input_file_description;
    //nanocube::Schema nanocube_schema(input_file_description);

    int i=0;
    for(i=0; i<slaves.size(); i++)
    {
        wakeSlave(slaves[i], input_file_description);
    }

    sleep(5);

    //Send data
    int block_size = options.block_size.getValue();
    bool finished_input = false;
    i = 0;
    for(;;)
    {
        finished_input = sendToSlave(slaves[i], block_size * slaves[i].load);
	      i++;
	      if(i >= slaves.size())
	          i = 0;
        if(finished_input)
            break;
    }


    std::cerr << "(distribute) Scattering finished" << std::endl;

}


//------------------------------------------------------------------------------
// initGather
//------------------------------------------------------------------------------
void initGather(Options& options, std::vector<Slave>& slaves)
{

    std::cerr << "(distribute) Initializing gathering..." << std::endl;

    Master master(slaves);
    int current_port = options.query_port.getValue();

    int tentative=0;
    while (tentative < 100) {
        tentative++;
        try {
            std::cerr << "(distribute) Starting MasterServer on port " << current_port << std::endl;
            master.start(options.no_mongoose_threads.getValue(), current_port);
        }
        catch (MasterException &e) {
            std::cerr << e.what() << std::endl;
            current_port++;
        }
    }

    std::cerr << "(distribute) Gathering finished" << std::endl;


}

int parseHostFile(Options& options, std::vector<Slave>& slaves)
{
    //Read hosts file
    std::ifstream file;
    file.open(options.hosts_file.getValue());
    if(file.is_open())
    {
        std::string line;
        while ( std::getline (file,line) )
        {
            int sep0 = line.find(":");
            int sep1 = line.find(" ", sep0+1);
            int sep2 = line.find(" ", sep1+1);
            
            std::string address = line.substr(0, sep0);
            std::string type = "d";
            int port = 0.0;
            float load = 1.0;
            Slave newslave(address);

            if(sep1 == std::string::npos && sep2 == std::string::npos)
            {
                //address:port
                port = atoi(line.substr(sep0+1).c_str());
            }
            else if(sep2 == std::string::npos)
            {
                //address:port q or address:port X
                port = atoi(line.substr(sep0+1, sep1-sep0-1).c_str());
                std::string aux = line.substr(sep1+1);

                if(aux.compare("q") == 0)
                {
                    type = "q";
                }
                else
                {
                   load = atof(aux.c_str());
                }
            }
            else
            {
                //address:port q X 
                port = atoi(line.substr(sep0+1, sep1-sep0-1).c_str());
                type = line.substr(sep1+1, sep2-sep1-1);
                load = atof(line.substr(sep2+1).c_str());
            }

            if(type.compare("q") == 0)
            {
                newslave.query_port = port;
            }
            else
            {
                newslave.deamon_port = port;
            }

            newslave.load = load;

            slaves.push_back(newslave);

            std::cerr << "(distribute) Node: " << type << ", " << address << ":" << port << ", Load: " << load << std::endl;
        }

        file.close();
    }
    else
    {
        std::cerr << "(distribute) Unable to open file\n"; 
        return 0;
    }

    return 1;

}


//------------------------------------------------------------------------------
// main
// file format:
// distribution rules:
// address:port [q] [X]
// address:port - host address and port
// q: query only host
// X: dataset porcentage going to that host (0.0 <= X <= 1.0)
//------------------------------------------------------------------------------

int main(int argc, char *argv[])
{

#if 0
    std::ifstream fixed_file("/Users/llins/projects/nanocube/scripts/crime50k.nano");
    input_stream = &fixed_file;
#else
    input_stream = &std::cin;
#endif
    
    std::vector<std::string> args(argv, argv + argc);
    Options options(args);     

    std::vector<Slave> slaves;

    int success = parseHostFile(options, slaves);

    if(success == 0)
    {
        exit(1);
    }

    if(!options.query_only.getValue())
        initScatter(options, slaves);

    initGather(options, slaves);

    return 0;
    
}
