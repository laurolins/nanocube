
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>

#include <boost/asio.hpp>

#include "NanoCubeSchema.hh"
#include "DumpFile.hh"


struct Slave {
    std::string deamon_address;
    int deamon_port;
    std::string slave_address;
    int slave_insert_port;
    int slave_query_port;
};


//------------------------------------------------------------------------------
// wakeSlave
// Steps:
// 1 - Connect to deamon
// 2 - Send schema
// 3 - Wait for slave port
// 4 - Connect to slave
// 5 - Ready to send data
//------------------------------------------------------------------------------

void wakeSlave(Slave slave, dumpfile::DumpFileDescription schema)
{
    // //1 - Connect to deamon
    std::cout << "Connecting to deamon " << slave.deamon_address << ":" << slave.deamon_port << std::endl;

    boost::asio::io_service io_service;
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string(slave.deamon_address), slave.deamon_port);
    boost::asio::ip::tcp::socket socket(io_service);

    socket.connect(endpoint);

    //2 - Send schema
    std::cout << "Sending schema to deamon... " <<  std::endl;
    std::ostringstream os;
    os << schema;
    boost::asio::write(socket, boost::asio::buffer(os.str()));
    socket.close();

    // Do I need to close a socket and then re-connect it to make sure the deamon is not blocked at recv waiting for more?
    socket.connect(endpoint);

    //3 - Wait for slave port
    std::cout << "Waiting for deamon to send ncserve query and insert ports... " <<  std::endl;
    std::vector<char> response;
    for (;;)
    {
        char buf[1024];
        boost::system::error_code error;

        size_t len = socket.read_some(boost::asio::buffer(buf), error);

        std::cout << "Received: " << len << std::endl;

        if (error == boost::asio::error::eof)
            break; // Connection closed cleanly by peer.
        else if (error)
            throw boost::system::system_error(error); // Some other error.

        //std::cout.write(buf, len);
        response.insert(response.end(), buf, buf+len);
    }

    std::string aux(response.begin(), response.end());
    std::cout << "Received message from deamon: " << aux << std::endl;

    //Parse response
    int pos0 = aux.find(":");
    std::string status = aux.substr(0, pos0);

    int pos1 = aux.find(":", pos0+1);
    int insert_port = atoi(aux.substr(pos0+1, pos1-pos0).c_str());

    int query_port = atoi(aux.substr(pos1+1, aux.length()-pos1).c_str());

    if(status.compare("ERROR") == 0)
    {
        std::cout << "Deamon returned an error." << std::endl;
        return;
    }

    std::cout << "ncserve insert port: " << insert_port <<  ", query port: " << query_port << std::endl;

    //4 - Connect to slave


    //5 - Ready to send data
}


//------------------------------------------------------------------------------
// main
// file format:
// address:port
//------------------------------------------------------------------------------

int main(int argc, char *argv[])
{

    //Read servers file
    if(argc <= 1)
    {
       std::cout << "Usage: ./ncdistribute [file with servers]\n" << std::endl;
       return 1;
    }

    std::vector<Slave> slaves;
    
    std::ifstream file;
    file.open(argv[1]);
    if(file.is_open())
    {
        std::string line;
        while ( std::getline (file,line) )
        {
            std::cout << "Node: " << line << std::endl;
            auto sep = line.find(":");
            std::string address = line.substr(0, sep);
            int port = atoi(line.substr(sep+1).c_str());

            Slave newslave;
            newslave.deamon_address = address;
            newslave.deamon_port = port;
            slaves.push_back(newslave);
        }

        file.close();
    }
    else
    {
        std::cout << "Unable to open file\n"; 
        return 0;
    }

    //Read schema
    //std::cout << std::cin.rdbuf();

    dumpfile::DumpFileDescription input_file_description;
    std::cin >> input_file_description;
    //nanocube::Schema nanocube_schema(input_file_description);

    int i=0;
    for(i=0; i<slaves.size(); i++)
    {
        wakeSlave(slaves[i], input_file_description);
    }


    return 0;
    
}