#include <tclap/CmdLine.h> // templetized command line options

struct Options {
    Options(std::vector<std::string>& args);
    TCLAP::CmdLine cmd_line { "Nanocube Leaf - local process", ' ', "2.3" };
};

Options::Options(std::vector<std::string>& args) {
    // cmd_line.add(command); // add command option
    cmd_line.parse(args);
}

int main(int argc, char** argv) 
{
    // std::cerr << "nanocube-leaf" << std::endl;
    try {

        std::vector<std::string> args(argv, argv + argc);
        
        Options options(args);

        std::cerr << "Read all options" << std::endl;

        // write everything coming from stdin to child process
        const int BUFFER_SIZE = 4095;
        char buffer[BUFFER_SIZE + 1];
        while (1) {
            std::cin.read(buffer,BUFFER_SIZE);
            if (!std::cin) {
                auto gcount = std::cin.gcount();
                if (gcount > 0) {
                    std::cout.write(buffer, gcount);
                }
                break;
            }
            else {
                std::cout.write(buffer, BUFFER_SIZE);
            }
        }
        std::cout.flush();

    } catch (TCLAP::ArgException &e) { 
        std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl; 
    } catch (std::string &st) { 
        std::cerr << st << std::endl; 
    }




    // std::cerr << "nanocube-leaf: " << argc << std::endl;
    // std::cerr << "nanocube-leaf... finished" << std::endl;
}
