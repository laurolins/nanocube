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

#include <random>

#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <time.h>

#include "tclap/CmdLine.h"

#define NUM_TENTATIVES 10

struct Options {
    Options(std::vector<std::string>& args);
    
    TCLAP::CmdLine cmd_line { "Nanocube Daemon", ' ', "2.3", true };
    
    // -q or --query
    TCLAP::ValueArg<int> port {
        "p",              // flag
        "port", // name
        "Port where to listen for nanocube related requests",    // description
        false,      // required
        50005,      // value
        "port"      // type description
    };
    
};

Options::Options(std::vector<std::string>& args) {
    cmd_line.add(port);
    cmd_line.parse(args);
}

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

pid_t initializeNcserve(std::string message, int input, int query)
{

    //Save schema to a temp file
    FILE* file = fopen("schema.tmp","w+");
    fprintf(file,"%s", message.c_str()); /*writes*/ 
    fclose(file);

    // Spawn a child to run the program.
    pid_t pid = fork();
    if (pid == 0) { /* child process */

        char charinput[15];
        char charquery[15];
        sprintf(charinput, "%d", input);
        sprintf(charquery, "%d", query);

        //printf("PID 2: %d\n", getpid());

        //TODO: remove file before replacing process by nanocube-leaf

        // child process will be replaced by this other process
        // could we do thi in the main process? maybe
        //execlp(program_name.c_str(), program_name.c_str(), NULL);
        //execv(program_name.c_str(), argv);
        execlp("nanocube-leaf", "nanocube-leaf", "-s", "schema.tmp", "-q", charquery, "-i", charinput, (char *)NULL);

        // failed to execute program
        printf("(deamon) Could not open file\n");

        exit(127); /* only if execv fails */

    }

    //printf("PID 1: %d\n", pid);

    return pid;
}

int main(int argc, char **argv)
{
    std::vector<std::string> args(argv, argv + argc);
    Options options(args);
    
    int port = options.port.getValue();

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
        printf("(deamon) Socket creation error.");
        exit(1);
    }

    //ok to reuse ports
    int yes=1;
    if (setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
        printf("(deamon) setsockopt error");
        exit(1);
    }

    if(bind(listenfd, (struct sockaddr *)&local, sizeof(struct sockaddr)) == -1)
    {
        printf("(deamon) Bind error.\n");
        exit(1);
    }

    if(listen(listenfd, 5) == -1)
    {
        printf("(deamon) Listen error.\n");
        exit(1);
    }

    printf("(deamon) Waiting for connection on port %d...\n", port);


    // Seed with a real random value, if available
    std::random_device rd;

    // target port range 50100 - 59999
    std::default_random_engine random_engine(rd());
    std::uniform_int_distribution<int> uniform_dist(50100, 59999);

    /* The Big Loop */
    while (1) {

        /* Wait for master communication */
        size = sizeof(remote);
        connfd = accept(listenfd, (struct sockaddr*)&remote, &size);

        if(connfd == -1)
        {
            printf("(deamon) Connection error.\n");
            exit(1);
        }

        printf("(deamon) Connection accepted.\n");

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
        //printf("Data received:\n%s", message.c_str());

        //Initialize ncserve
        int input = 0;
        int query = 0;
        int i = 0;

        // struct timespec ts;
        // clock_gettime(CLOCK_MONOTONIC, &ts);
        // //Using nano-seconds instead of seconds
        // srand((time_t)ts.tv_nsec);

        for(i=0; i<NUM_TENTATIVES; i++)
        {
            int input = uniform_dist(random_engine);
            int query = uniform_dist(random_engine);
            pid_t pid = initializeNcserve(message, input, query);

            //char filepath[30];
            //sprintf(filepath, "/tmp/nanocube-leaf-%d", pid);
            //FILE* file = fopen(filepath, "r");
            int status = 0;
            pid_t result = waitpid(pid, &status, WNOHANG);

            if(result == 0)
            {
                printf("(deamon) Sending input port %d and query port %d to ncdistribute.\n", input, query);

                //Send insert and query port to ncdistribute
                connfd = accept(listenfd, (struct sockaddr*)&remote, &size);
                char aux[100];
                sprintf(aux, "OK:%d:%d", input, query);
                int bytes_sent = 0;
                int length = strlen(aux);
                while(1)
                {
                    bytes_sent += send(connfd, aux, strlen(aux), 0);

                    if(bytes_sent >= length)
                        break;
                }
                printf("(deamon) Closed\n");

                close(connfd);   
                break;
            }

            sleep(1);
        }

            
        sleep(1); /* wait 1 second */
    }

    //closelog();
    close(listenfd);
    exit(0);
}
