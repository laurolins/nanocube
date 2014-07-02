/*


Test runs fine:
./nanocube-sliding -w 10000000 -n 1 -q 5200 < ../scripts/crime50k.nano

Test has problem:
./nanocube-sliding -w 10000000 -n 2 -q 5200 < ../scripts/crime50k.nano
(the second process does not answer any query. It never starts up)
(notice that there is only one query-port and insert-port on the terminal)


If the window is smaller, then the results are more consistent (but I don't know if right)
./nanocube-sliding -w 100 -n 2 -q 5200 < ../scripts/crime50k.nano




*/

#include <unistd.h>    /* for fork */
#include <sys/types.h> /* for pid_t */
#include <sys/wait.h>  /* for wait */

#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>
#include <thread>
#include <chrono>
#include <random>
#include <vector>

#include <boost/asio.hpp>

#include "tclap/CmdLine.h"

#include "NanoCubeSchema.hh"
#include "DumpFile.hh"
#include "Master.hh"


struct Options {
    Options(std::vector<std::string>& args);

    TCLAP::CmdLine cmd_line { "Nanocube Sliding Window", ' ', "2.3", false };

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

    // -w or --window-size
    TCLAP::ValueArg<int> window_size {  
        "w",              // flag
        "window-size",         // name
        "Window size, in hours.", // description
        false ,              // required
        24, //value
        "window-size"
    };
    
    // -n or --num-windows
    TCLAP::ValueArg<int> num_windows {  
        "n",              // flag
        "num",         // name
        "Number of windows.", // description
        false,               // required
        2,
        "num-windows"
    };

    // -t or --threads
    TCLAP::ValueArg<int> no_mongoose_threads {
        "t",              // flag
        "threads",        // name
        "Number of threads for querying (mongoose)",     // description
        false,                                 // required
        10,                                   // value
        "threads"                         // type description
    };

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
    cmd_line.add(block_size);
    cmd_line.add(query_port);
    cmd_line.add(query_only);
    cmd_line.add(no_mongoose_threads);
    cmd_line.add(window_size);
    cmd_line.add(num_windows);
    cmd_line.add(schema);
    cmd_line.parse(args);
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


//------------------------------------------------------------------------------
// Window
//------------------------------------------------------------------------------
struct Window {

    Window(NanoCubeSchema nc_schema, std::string schema_filename, int windowid, int query_port);
    void runProcess();
    void openStream();
    void writeStream(const void * ptr, size_t size, size_t count);
    void closeStream();
    void initialize();
    void killProcess();

    NanoCubeSchema nc_schema;
    std::string schema_filename;
    int pipe_read_file_descriptor;
    int pipe_write_file_descriptor;
    int pid;
    int windowid;
    FILE* stream;
    int query_port;

};

Window::Window(NanoCubeSchema nc_schema, std::string schema_filename, int windowid, int query_port):
    nc_schema(nc_schema),
    schema_filename(schema_filename),
    windowid(windowid),
    query_port(query_port)
{}

void Window::initialize()
{
    int pipe_fds[2];
    if(pipe(pipe_fds) == -1)
    {
        std::cerr << "Couldn't create pipe!" << std::endl;
        exit(127);
    }

    pipe_read_file_descriptor  = pipe_fds[0];
    pipe_write_file_descriptor = pipe_fds[1];
    
    pid_t newpid = fork();
    pid = newpid;
    if(newpid == 0) /* Child process */
    {
        close(pipe_write_file_descriptor); //child process does not need this end of pipe
        if(pipe_read_file_descriptor != STDIN_FILENO) //if: http://webdocs.cs.ualberta.ca/~tony/C379/C379Labs/Lab2/exec.html
            dup2(pipe_read_file_descriptor, STDIN_FILENO); //std::cin becomes the pipe read channel
        
        runProcess();
    }
    else /* Parent process */
    {
        close(pipe_read_file_descriptor);
    }
}

void Window::killProcess()
{
    //kill process
    closeStream();
    std::cout << "Killing process " << pid << std::endl;
    kill(pid, SIGINT);
}


void Window::runProcess()
{

    //std::cout << getpid() << std::endl;
    //pid = getpid();

    std::cout << "Child process " << getpid() << " is creating a nanocube leaf. Query: " << query_port << std::endl;
    
    //execlp("nanocube-leaf", "nanocube-leaf", "-q", std::to_string(query_port).c_str(), "-b", "1", "-s", schema_filename.c_str(), (char *)NULL);
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
    execlp(program_name.c_str(), program_name.c_str(), "-q", std::to_string(query_port).c_str(), "-b", "1", "-s", schema_filename.c_str(), (char *)NULL);

    // failed to execute program
    std::cout << "Could not create leaf process." << std::endl;;

    exit(-1); /* only if execv fails */
}

void Window::openStream()
{
    //return;
    //std::cout << "1 " << pipe_write_file_descriptor << std::endl;
    stream = fdopen(pipe_write_file_descriptor, "w");
    //std::cout << "2 " << pipe_write_file_descriptor << std::endl;
    //std::cout << "3 " << stream << std::endl;
}


void Window::writeStream(const void * ptr, size_t size, size_t count)
{

    //std::cout << "Writing..." << std::endl;
    //std::cout << pipe_write_file_descriptor << std::endl;
    //std::cout << size << std::endl;
    //std::cout << count << std::endl;
    //stream = fdopen(pipe_write_file_descriptor, "w");
    //std::cout << "Open" << std::endl;
    //std::cout << stream << std::endl;
    int ret = fwrite(ptr, size, count, stream);
    //fflush(stream);
    //fclose(stream);
    //std::cout << "Closing..." << std::endl;

    if(ret != count)
    {
        std::cout << "Error in writeStream. Wrote " << ret << ". Should be " << count << std::endl;
        exit(-1);
    }
}

void Window::closeStream()
{
    close(pipe_write_file_descriptor);
    fflush(stream);
    fclose(stream);
}


//------------------------------------------------------------------------------
// initGather
//------------------------------------------------------------------------------
void initGather(Options& options, std::vector<Slave>& slaves)
{

    std::cout << "Initializing gathering..." << std::endl;

    Master master(slaves);
    int current_port = options.query_port.getValue();

    int tentative=0;
    while (tentative < 100) {
        tentative++;
        try {
            std::cout << "Starting MasterServer on port " << current_port << std::endl;
            master.start(options.no_mongoose_threads.getValue(), current_port);
        }
        catch (MasterException &e) {
            std::cout << e.what() << std::endl;
            current_port++;
        }
    }

    std::cout << "Gathering finished" << std::endl;

}


//------------------------------------------------------------------------------
// main
// sliding will be responsible for starting two leaf processes, each one for a window slice
// sliding will receive a record, and forward to the right slice according to its timestamp
//------------------------------------------------------------------------------

int main(int argc, char *argv[])
{

    
    std::vector<std::string> args(argv, argv + argc);
    
    // read options
    Options options(args);

    // options are valid
    std::string schema_filename = options.schema.getValue();

    // figure out the schema of the nanocube leaf
    dumpfile::DumpFileDescription input_file_description;


    if (schema_filename.size()) {
        std::ifstream ifs(schema_filename);
        ifs >> input_file_description;
    }
    else {
        std::cin >> input_file_description;
        
        std::stringstream ss;
        ss << input_file_description;
        ss << std::endl;

        schema_filename = "schema.tmp";

        //Save schema to a temp file
        FILE* file = fopen(schema_filename.c_str(),"w+");
        fprintf(file,"%s", ss.str().c_str());
        fclose(file);
        
    }

    // read schema
    NanoCubeSchema nc_schema(input_file_description);

    //std::cout << nc_schema.dump_file_description.record_size << std::endl;

    // windows
    std::vector<Window> windows;
    std::vector<Slave> slaves;
    int i = 0;
    for(i = 0; i<options.num_windows.getValue(); i++)
    {

        // Seed with a real random value, if available
        std::random_device rd;
        // target port range 50100 - 59999
        std::default_random_engine random_engine(rd());
        std::uniform_int_distribution<int> uniform_dist(50100, 59999);
        int query_port = uniform_dist(random_engine);

        Window newwindow = Window(nc_schema, schema_filename, i, query_port);
        windows.push_back(newwindow);
        windows[i].initialize();
        windows[i].openStream();

        Slave newslave("127.0.0.1");
        newslave.query_port = windows[i].query_port;
        slaves.push_back(newslave);

        //sleep(10);
    }

    //send schema
    /*
    std::stringstream ss;
    ss << nc_schema.dump_file_description;
    ss << std::endl;
    for(i = 0; i<options.num_windows.getValue(); i++)
    {
        windows[i].writeStream(ss.str().c_str(), 1, ss.str().size());
    }
    */
    // create a master (query-only) to handle queries
    pid_t newpid = fork();
    if(newpid == 0) /* Child process */
    {
        initGather(options, slaves);
    }
    else
    {
        std::cout << "started redirecting stdin content to write-channel of parent-child pipe of process " << getpid() << std::endl;

        // write everything coming from stdin to child process
        int batch_size = 1;//options.batch_size.getValue();
        int record_size = nc_schema.dump_file_description.record_size;
        int num_bytes_per_batch = record_size * batch_size;
        char buffer[num_bytes_per_batch];    

        int time_dimension_index = nc_schema.dump_file_description.getTimeFieldIndex();
        dumpfile::Field* time_dimension_field = nc_schema.dump_file_description.fields[time_dimension_index];

        // std::cout << "offset_inside_record: " << time_dimension_field->offset_inside_record << std::endl;
        // std::cout << "first_token_index: " << time_dimension_field->first_token_index << std::endl;
        // std::cout << "field_type: " << time_dimension_field->field_type << std::endl;
        // std::cout << "name: " << time_dimension_field->field_type.name << std::endl;
        // std::cout << "num_bytes: " << time_dimension_field->field_type.num_bytes << std::endl;
        // std::cout << "num_tokens: " << time_dimension_field->field_type.num_tokens << std::endl;

        sleep(10);

        int start_time     = -1;
        int window_size    = options.window_size.getValue();
        int current_window = 0;
        int window_end     = -1;
        int num_points     = 0;
        while (1)
        {
            std::cin.read(buffer,num_bytes_per_batch);

            //write buffer
            //std::cout << record_size << std::endl;
            int offset = time_dimension_field->offset_inside_record;
            int num_bytes = time_dimension_field->field_type.num_bytes;
            int first_token_index = time_dimension_field->first_token_index;
            long timestamp = 0;
            std::copy(buffer+offset, buffer+offset+num_bytes, (char*)&timestamp);

            // first record initialize time initial boundaries
            if (start_time == -1) { 
                start_time = timestamp;
                window_end = start_time + window_size;
            }

            //std::cout << timestamp << std::endl;

            if(timestamp >= window_end)
            {
                std::cout << "Old window: " << current_window << " Last timestamp: " << window_end << std::endl;

                window_end += window_size;
                current_window++;
                if(current_window >= windows.size())
                {
                    current_window = 0;
                }
                windows[current_window].killProcess();
                windows[current_window].initialize();
                windows[current_window].openStream();

                std::cout << "    New window: " << current_window << " Last timestamp: " << window_end << std::endl;
            }


            if (!std::cin) {
                auto gcount = std::cin.gcount();
                if (gcount > 0) {
                    //fwrite((void*) buffer, 1, gcount, f);
                    // std::cout << "@ Writing to window " << current_window << ", query: " << windows[current_window].query_port << std::endl;
                    windows[current_window].writeStream((void*) buffer, 1, gcount);
                    num_points++;
                }
                break;
            }
            else {
                // std::cout << "@ Writing to window " << current_window << ", query: " << windows[current_window].query_port << std::endl;
                //fwrite((void*) buffer, 1, BUFFER_SIZE, f);
                windows[current_window].writeStream((void*) buffer, 1, num_bytes_per_batch);
                num_points++;
            }

            
            
        }

        std::cout << "num_points: " << num_points << std::endl;
        std::cout << "closing redirections" << std::endl;

        // clear
        //int aux = EOF;
        //windows[current_window].writeStream((void*) aux, 1, 1);
        for(i = 0; i < windows.size(); i++)
        {
            windows[i].closeStream();
        }

        waitpid(newpid, 0, 0);

    }

    

}
