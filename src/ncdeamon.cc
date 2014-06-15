#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <sstream>
#include <string>
#include <iostream>

#include <sys/socket.h>
#include <netinet/in.h>

#include "NanoCubeSchema.hh"
#include "DumpFile.hh"

void deamonize()
{
    /* Our process ID and Session ID */
    pid_t pid, sid;
    
    /* Fork off the parent process */
    pid = fork();
    if (pid < 0) {
        exit(EXIT_FAILURE);
    }
    /* If we got a good PID, then
       we can exit the parent process. */
    if (pid > 0) {
        exit(EXIT_SUCCESS);
    }

    /* Change the file mode mask */
    umask(0);
            
    /* Open any logs here */  
    openlog("nc", LOG_PID|LOG_CONS, LOG_USER);     
            
    /* Create a new SID for the child process */
    sid = setsid();
    if (sid < 0) {
        /* Log the failure */
        syslog(LOG_INFO, "Could not create a new SID for the child process.");
        exit(EXIT_FAILURE);
    }
    

    
    /* Change the current working directory */
    if ((chdir("/")) < 0) {
        /* Log the failure */
        syslog(LOG_INFO, "Could not change the current working directory.");
        exit(EXIT_FAILURE);
    }
    
    /* Close out the standard file descriptors */
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

//
// Here is the entity responsible to translating dump file
// field type names into a meaningful nanocube schema
//
// (from ncserve.cc)
struct NanoCubeSchema {

    NanoCubeSchema(dumpfile::DumpFileDescription &dump_file_description):
        dump_file_description(dump_file_description)
    {
        std::stringstream ss_dimensions_spec;
        std::stringstream ss_variables_spec;
        for (dumpfile::Field *field: dump_file_description.fields) {
            std::string field_type_name = field->field_type.name;
            std::cout << field_type_name << std::endl;
            if (field_type_name.find("nc_dim_cat_") == 0) {
                auto pos = field_type_name.begin() + field_type_name.rfind('_');
                int num_bytes = std::stoi(std::string(pos+1,field_type_name.end()));
                std::cout << "categorical dimension with " << num_bytes << " bytes" << std::endl;
                ss_dimensions_spec << "_c" << num_bytes;
            }
            else if (field_type_name.find("nc_dim_quadtree_") == 0) {
                auto pos = field_type_name.begin() + field_type_name.rfind('_');
                int num_levels = std::stoi(std::string(pos+1,field_type_name.end()));
                std::cout << "quadtree dimension with " << num_levels << " levels" << std::endl;
                ss_dimensions_spec << "_q" << num_levels;
            }
            else if (field_type_name.find("nc_dim_time_") == 0) {
                auto pos = field_type_name.begin() + field_type_name.rfind('_');
                int num_bytes = std::stoi(std::string(pos+1,field_type_name.end()));
                std::cout << "time dimension with " << num_bytes << " bytes" << std::endl;
                ss_variables_spec << "_u" << num_bytes;
            }
            else if (field_type_name.find("nc_var_uint_") == 0) {
                auto pos = field_type_name.begin() + field_type_name.rfind('_');
                int num_bytes = std::stoi(std::string(pos+1,field_type_name.end()));
                std::cout << "time dimension with " << num_bytes << " bytes" << std::endl;
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

void initializeNcserve(std::string message)
{
    dumpfile::DumpFileDescription input_file_description;

    std::istringstream iss (message);
    iss >> input_file_description;

    NanoCubeSchema nc_schema(input_file_description);
    std::cout << "Dimensions: " << nc_schema.dimensions_spec << std::endl;
    std::cout << "Variables:  " << nc_schema.time_and_variables_spec << std::endl;
    std::cout << "Executable: nc" << nc_schema.dimensions_spec << nc_schema.time_and_variables_spec << std::endl;

    // pipe
    int pipe_one[2];
    int &pipe_read_file_descriptor  = pipe_one[0];
    int &pipe_write_file_descriptor = pipe_one[1];

    // create pipe
    if(pipe(pipe_one) == -1)
    {
        std::cout << "Couldn't create pipe!" << std::endl;
        // perror("ERROR");
        exit(127);
    }

    // Spawn a child to run the program.
    pid_t pid = fork();
    if (pid == 0) { /* child process */

        /* Close unused pipe */
        close(pipe_write_file_descriptor);

        /* both file descriptors refer to pipe_read_file_descriptor */
        /* std::cin becomes the pipe read channel */
        dup2(pipe_read_file_descriptor, STDIN_FILENO);

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
        execlp(program_name.c_str(), program_name.c_str(), NULL);
        //execv(program_name.c_str(), argv);

        // failed to execute program
        std::cout << "Could not find program: " << program_name << std::endl;

        exit(127); /* only if execv fails */

    }
}

int main(int argc, char *argv[])
{


    int port = 29512;
    if(argc == 2)
    {
        port = atoi(argv[1]);
    }

    //Links:
    //http://ubuntuforums.org/showthread.php?t=2045538
    //http://www.enderunix.org/docs/eng/daemon.php
    //http://www.cs.dartmouth.edu/~campbell/cs60/socketprogramming.html

    //deamonize();
    
    /* Daemon-specific initialization goes here */
    //Socket initialization
    int listenfd, connfd;
    struct sockaddr_in local, remote;
    socklen_t size;

    memset(&local, 0, sizeof(local));
    local.sin_family = AF_INET;
    local.sin_port = htons(port);
    local.sin_addr.s_addr = htonl(INADDR_ANY);

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    
    if (listenfd == -1) {
        printf("Socket creation error.");
        exit(1);
    }

    //ok to reuse ports
    int yes=1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        printf("setsockopt error");
        exit(1);
    }

    if(bind(listenfd, (struct sockaddr *)&local, sizeof(struct sockaddr)) == -1)
    {
        printf("Bind error.\n");
        exit(1);
    }

    if(listen(listenfd, 5) == -1)
    {
        printf("Listen error.\n");
        exit(1);
    }

    printf("Waiting for connection on port %d...\n", port);

    /* The Big Loop */
    while (1) {

        /* Wait for master communication */
        size = sizeof(remote);
        connfd = accept(listenfd, (struct sockaddr*)&remote, &size);

        if(connfd == -1)
        {
            printf("Connection error.\n");
            exit(1);
        }

        printf("Connection accepted.\n");

        int bytes_read = 0;
        char buf[1024];
        std::string message;
        while(1)
        {
            bytes_read = recv(connfd, buf, 1024, 0);

            if(bytes_read == 0)
                break;

            message.insert(message.end(), buf, buf+bytes_read);
        }

        close(connfd);
        printf("Data received:\n%s", message.c_str());

        //Initialize ncserve
        initializeNcserve(message);

        //Send insert and query port to ncdistribute
        connfd = accept(listenfd, (struct sockaddr*)&remote, &size);
        char* aux = "OK:123:12345";
        int bytes_sent = 0;
        int length = 12;
        while(1)
        {
            bytes_sent += send(connfd, aux, strlen(aux), 0);

            if(bytes_sent >= length)
                break;
        }

        printf("Closed\n");


        close(connfd);       
        sleep(1); /* wait 1 second */
    }

    //closelog();
    close(listenfd);
    exit(0);
}