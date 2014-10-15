#include <unistd.h>    /* for fork */
#include <sys/types.h> /* for pid_t */
#include <sys/wait.h>  /* for wait */

#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>
#include <thread>
#include <chrono>

#include <tclap/CmdLine.h> // templetized command line options

#include "DumpFile.hh"


void message() {

    std::cout << "Usage: frontend <number>" << std::endl;

}

//
// Here is the entity responsible to translating dump file
// field type names into a meaningful nanocube schema
//
struct NanoCubeSchema {

    NanoCubeSchema(dumpfile::DumpFileDescription &dump_file_description):
        dump_file_description(dump_file_description)
    {
        std::stringstream ss_dimensions_spec;
        std::stringstream ss_variables_spec;
        for (dumpfile::Field *field: dump_file_description.fields) {
            std::string field_type_name = field->field_type.name;
//            std::cout << field_type_name << std::endl;
            if (field_type_name.find("nc_dim_cat_") == 0) {
                auto pos = field_type_name.begin() + field_type_name.rfind('_');
                int num_bytes = std::stoi(std::string(pos+1,field_type_name.end()));
//                std::cout << "categorical dimension with " << num_bytes << " bytes" << std::endl;
                ss_dimensions_spec << "_c" << num_bytes;
            }
            else if (field_type_name.find("nc_dim_quadtree_") == 0) {
                auto pos = field_type_name.begin() + field_type_name.rfind('_');
                int num_levels = std::stoi(std::string(pos+1,field_type_name.end()));
//                std::cout << "quadtree dimension with " << num_levels << " levels" << std::endl;
                ss_dimensions_spec << "_q" << num_levels;
            }
            else if (field_type_name.find("nc_dim_time_") == 0) {
                auto pos = field_type_name.begin() + field_type_name.rfind('_');
                int num_bytes = std::stoi(std::string(pos+1,field_type_name.end()));
//                std::cout << "time dimension with " << num_bytes << " bytes" << std::endl;
                ss_variables_spec << "_u" << num_bytes;
            }
            else if (field_type_name.find("nc_var_uint_") == 0) {
                auto pos = field_type_name.begin() + field_type_name.rfind('_');
                int num_bytes = std::stoi(std::string(pos+1,field_type_name.end()));
//                std::cout << "time dimension with " << num_bytes << " bytes" << std::endl;
                ss_variables_spec << "_u" << num_bytes;
            }

            this->dimensions_spec         = ss_dimensions_spec.str();
            this->time_and_variables_spec = ss_variables_spec.str();
        }
    }

    std::string dimensions_spec;

    std::string time_and_variables_spec;

    dumpfile::DumpFileDescription &dump_file_description;

};

//
// d stands for dimension
//
//    cX = categorical, X = num bytes
//    qX = quadtree,    X = num levels
//    tX = time,        X = num bytes
//
// v stands for variable
//
//    uX = unsigned int of X bytes
//

struct Options {
    Options(std::vector<std::string>& args);

    TCLAP::CmdLine cmd_line { "Nanocube Leaf - local process", ' ', "2.3", true };

    // -s or --schema
    TCLAP::ValueArg<std::string> schema {  
            "s",              // flag
            "schema",         // name
            "Nanocube schema file (if not coming from stdin)", // description
            false,            // required
            "",               // value
            "schema-filename" // type description
            };
};

Options::Options(std::vector<std::string>& args) {
    cmd_line.add(schema); // add command option
    cmd_line.parse(args);
}

#define xDEBUG_NANOCUBE_LEAF_PROCESS

int main(int argc, char **argv)
{

    std::cout << "VERSION: " << NANOCUBE_VERSION << std::endl;

    try {

        // only interested in --schema or -s to figure out the
        // the right nanocube process to call
        std::vector<std::string> args(argv, argv + argc);
        for (auto it=args.begin()+1;it!=args.end();) {
            if (it->compare("--schema")==0 || it->compare("-s")==0) {
                if (it + 1 != args.end())
                    it+=2;
                else
                    it = args.erase(it);
            }
            else if (it->compare("--help") == 0) {
                it = it + 1;
            }
            else {
                it = args.erase(it);
            }
        }
        

        // read options
        // std::vector<std::string> args(argv, argv + argc);
        Options options(args);

        // options are valid
        auto schema_filename = options.schema.getValue();

        // figure out the schema of the nanocube leaf
        dumpfile::DumpFileDescription input_file_description;

        if (schema_filename.size()) {
            std::ifstream ifs(schema_filename);
            ifs >> input_file_description;
        }
        else {
            std::cin >> input_file_description;
        }

        // read schema
        NanoCubeSchema nc_schema(input_file_description);

#if 0
        std::cerr << "Dimensions: " << nc_schema.dimensions_spec << std::endl;
        std::cerr << "Variables:  " << nc_schema.time_and_variables_spec << std::endl;
#endif
        //
        // TODO: run some tests
        //
        //    (1) is there a single time column
        //    (2) are all field types starting with nc_ prefix
        //
        
        // pipe
        int pipe_fds[2];
        if(pipe(pipe_fds) == -1)
        {
            std::cerr << "Couldn't create pipe!" << std::endl;
            exit(127);
        }

        int pipe_read_file_descriptor  = pipe_fds[0];
        int pipe_write_file_descriptor = pipe_fds[1];

        
        
        // create pipe

        // Spawn a child to run the program.
        pid_t pid = fork();

        if (pid != 0) { /* parent process */
            close(pipe_read_file_descriptor);


            std::stringstream ss;
            if (schema_filename.size() == 0) // if schema came from stdin,
            {                                // we have to send it again.
                ss << input_file_description;
                ss << std::endl;
            }
            
            bool finish_redirect = false;
            bool redirect_error = false;
            
            auto redirect = [&finish_redirect,&redirect_error](std::string initial_content, int fd_write) {

                FILE *f = fdopen(fd_write, "w");
                
                if (initial_content.size() > 0) {

                    fwrite((void*) initial_content.c_str(), 1 , initial_content.size(), f);

                    if (ferror(f)) {
                        redirect_error = true;
                        fclose(f);
                        return; // problem writing to pipe
                    }
                    
                }
                
                // write everything coming from stdin to child process
                const int BUFFER_SIZE = 4095;
                char buffer[BUFFER_SIZE + 1];
                while (!finish_redirect) {
                    std::cin.read(buffer,BUFFER_SIZE);
                    if (!std::cin) {
                        auto gcount = std::cin.gcount();
                        if (gcount > 0) {
                            fwrite((void*) buffer, 1, gcount, f);

                            if (ferror(f)) {
                                redirect_error = true;
                                fclose(f);
                                return; // problem writing to pipe
                            }
                        
                        }
                        break;
                    }
                    else {
                        fwrite((void*) buffer, 1, BUFFER_SIZE, f);

                        if (ferror(f)) {
                            redirect_error = true;
                            fclose(f);
                            return; // problem writing to pipe
                        }

                    }
                }
                
                // clear
                fflush(f);
                fclose(f);
                
            };
            
            // start thread to redirect
            std::thread redirect_thread(redirect, ss.str(), pipe_write_file_descriptor);
            
#ifdef DEBUG_NANOCUBE_LEAF_PROCESS
            std::cerr << "[nanocube-leaf] parent process is waiting for child process to finish..." << std::endl;
#endif

            // std::this_thread::sleep_for(std::chrono::seconds(1));
            waitpid(pid,0,0); /* wait for child to exit */
            
#ifdef DEBUG_NANOCUBE_LEAF_PROCESS
            std::cerr << "[nanocube-leaf] finishing the redirect procedure..." << std::endl;
#endif

            finish_redirect = true;
            redirect_thread.join();
            
#ifdef DEBUG_NANOCUBE_LEAF_PROCESS
            std::cerr << "[nanocube-leaf] parent process done..." << std::endl;
#endif

            
        }
        else {
            close(pipe_write_file_descriptor);
            
            /* both file descriptors refer to pipe_read_file_descriptor */
            /* std::cin becomes the pipe read channel */
            dup2(pipe_read_file_descriptor, STDIN_FILENO);
            close(pipe_read_file_descriptor);
            
            // exec new process
            std::string program_name;
            {
                std::stringstream ss;

                const char* binaries_path_ptr = std::getenv("NANOCUBE_BIN");
                if (binaries_path_ptr) {
                    std::string binaries_path(binaries_path_ptr);
                    if (binaries_path.size() > 0 && binaries_path.back() != '/') {
                        binaries_path = binaries_path + "/";
                    }
                    ss << binaries_path;
                }
                else {
                    ss << "./";
                }

                ss << "nc" << nc_schema.dimensions_spec << nc_schema.time_and_variables_spec;
                program_name = ss.str();
            }
            
            // child process will be replaced by this other process
            // could we do thi in the main process? maybe
            // execlp(program_name.c_str(), program_name.c_str(), NULL);
            execv(program_name.c_str(), argv);

            // failed to execute program
            std::cout << "Could not find program: " << program_name << std::endl;
        
            exit(-1); /* only if execv fails */

        }

        return 0;


    } catch (dumpfile::DumpFileException &e) {
        std::cerr << "[Problem] A problem happened (possible cause: bad schema description on .dmp header)" << std::endl;
    } catch (TCLAP::ArgException &e) {
        std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
    } catch (std::string &st) {
        std::cerr << st << std::endl;
    }

}
