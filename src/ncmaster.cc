

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>

#include "mongoose.h"

#include "Master.hh"


int main(int argc, char **argv)
{

    //Read servers file
    if(argc <= 1)
    {
       std::cout << "Usage: ./ncmaster [file with servers]\n" << std::endl;
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

            Slave newslave(address, port);
            slaves.push_back(newslave);
        }

        file.close();
    }
    else
    {
        std::cout << "Unable to open file\n";
        return 0;
    }
    

    Master master(slaves);

    int tentative=0;
    while (tentative < 100) {
        tentative++;
        try {
            std::cout << "Starting MasterServer on port " << master.port << std::endl;
            master.start(20);
        }
        catch (MasterException &e) {
            std::cout << e.what() << std::endl;
            master.port++;
        }
    }

    return 0;


}
