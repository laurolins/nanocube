#include <unistd.h>    /* for fork */
#include <sys/types.h> /* for pid_t */
#include <sys/wait.h>  /* for wait */

#include <algorithm>
#include <iostream>
#include <vector>


#include <tclap/CmdLine.h> // templetized command line options

//-------------------------------------------------------------------------------
// Options
//-------------------------------------------------------------------------------

struct Options {
    Options(std::vector<std::string> &args);
    TCLAP::CmdLine cmd_line { "Nanocube front-end program", ' ', "2.3", "true" };
    TCLAP::UnlabeledValueArg<std::string> command { "command", "command is one of the following: "
            "leaf, sliding, composite, distributed, or deamon.\n"
            "sliding: start a local nanocube, with sliding windows\n"
            "leaf: start a local nanocube\ncomposite: start nanocube from existing nanocubes\n"
            "distribute: distribute nanocubes on hosts with a deamon process\n"
            "deamon: start nanocube deamon process", true, "", "command" };
};

Options::Options(std::vector<std::string> &args) {
    cmd_line.add(command); // add command option
    cmd_line.parse(args);
}

//-------------------------------------------------------------------------------
// main
//-------------------------------------------------------------------------------

int main(int argc, char** argv) {
    try {
        // this process only considers the first argument
        // which may be a command or --help or --version
        std::vector<std::string> args(argv, argv + std::min(argc,2));
        Options options(args);
        auto command = options.command.getValue();

        std::vector<std::string> valid_commands { { "leaf", "sliding", "composite", "distribute", "deamon" } };

        auto it = std::find(valid_commands.begin(), valid_commands.end(), command);
        if (it == valid_commands.end()) {
            std::stringstream ss;
            ss << "Invalid Command: " << command << std::endl;
            ss << "Try: leaf, sliding, composite, distribute, deamon";
            throw std::string(ss.str());
        }

        // pipe file descriptors
        int pipe_fds[2];
        if(pipe(pipe_fds) == -1)
        {
            std::cerr << "Couldn't create pipe!" << std::endl;
            exit(127);
        }

        // write output of pipe_write_file_descriptor becomes 
        // input  to pipe_read_file_descriptor
        int pipe_read_file_descriptor  = pipe_fds[0];
        int pipe_write_file_descriptor = pipe_fds[1];

        pid_t pid = fork();
        if (pid == 0) { /* child process */

            // child process doesn't write on pipe, just reads from it
            close(pipe_write_file_descriptor);

            // STDIN_FILENO now is the same as pipe_read_file_descriptor
            dup2(pipe_read_file_descriptor, STDIN_FILENO);

            // exec new process
            std::string program_name = std::string("nanocube-") + command;  // e.g. nanocube-leaf or 
                                                                            //      nanocube-deamon or 
                                                                            //      nanocube-distributed

            std::stringstream program_path;
            {

                const char* binaries_path_ptr = std::getenv("NANOCUBE_BIN");
                if (binaries_path_ptr) {
                    std::string binaries_path(binaries_path_ptr);
                    if (binaries_path.size() > 0 && binaries_path.back() != '/') {
                        binaries_path = binaries_path + "/";
                    }
                    program_path << binaries_path;
                }
                else {
                    program_path << "./";
                }
                program_path << program_name;
            }

            std::vector<std::string> params(argv+2, argv+argc);
            params.insert(params.begin(),program_name); // insert child program name

            std::vector<const char*> argv_forward;
            for (auto &st: params) {
                argv_forward.push_back(st.c_str());
            }
            argv_forward.push_back(0); // and of argv marker

#if 0
            std::cerr << "before exec" << std::endl;
            auto ptr = &argv_forward[0];
            while (ptr != nullptr) {
                std::cerr << *ptr << std::endl;
                ++ptr;
            }
#endif

            // child process will be replaced by this other process
            // could we do this in the main process? maybe
            //execlp(program_name.c_str(), program_name.c_str(), NULL);
            execv(program_path.str().c_str(), (char**) &argv_forward[0]);

            // failed to execute program
            std::cout << "Could not find program: " << program_name << std::endl;

            exit(127); /* only if execv fails */

        }
        else {

            // parent process doesn't read from pipe, just writes on it
            close(pipe_read_file_descriptor);

            FILE *f = fdopen(pipe_write_file_descriptor, "w");
        
            // write everything coming from stdin to child process
            const int BUFFER_SIZE = 4095;
            char buffer[BUFFER_SIZE + 1];
            while (1) {
                std::cin.read(buffer,BUFFER_SIZE);
                if (!std::cin) {
                    auto gcount = std::cin.gcount();
                    if (gcount > 0) {
                        fwrite((void*) buffer, 1, gcount, f);
                    }
                    break;
                }
                else {
                    fwrite((void*) buffer, 1, BUFFER_SIZE, f);
                }
            }

            // clear
            fflush(f);

            // std::cerr << "Waiting for child process to finish" << std::endl;
            // done writing on the pipe
            fclose(f);
            close(pipe_write_file_descriptor);

            //
            waitpid(pid,0,0); /* wait for child to exit */
        }

        // std::cout << "Command: " << options.command.getValue() << std::endl;

    } catch (TCLAP::ArgException &e) { 
        std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl; 
    } catch (std::string &st) { 
        std::cerr << st << std::endl; 
    }
}
