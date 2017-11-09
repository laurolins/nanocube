#include "../base/platform.c"


//
// TODO(llins) document all functions, types, and defines
// we use from these includes
//
#include <arpa/inet.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
// #include <resolv.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/epoll.h>

#include <time.h>
// clock()

#include <unistd.h>

#ifdef OS_MAC
#include <mach-o/dyld.h>
#include "../platform_dependent/osx_platform.c"
#elif OS_LINUX
#include "../platform_dependent/linux_platform.c"
#endif

#define tcp_LOCAL_ADDR          "127.0.0.1"
#define tcp_PORT	        8888
#define tcp_MAX_CONNECTIONS	64
#define tcp_MAX_EVENTS		64
#define tcp_TIMEOUT             500

/* this call will block */
// PLATFORM_SERVER_START(osx_sever_start)
// {
// }

// char Buffer[2048]
// char *BufferBegin;
// char *BufferEnd;

// typedef union epoll_data {
//   void *ptr;
//   int fd;
//   uint32_t u32;
//   uint64_t u64;
// } epoll_data_t;
//
// struct epoll_event
// {
//   uint32_t events;	/* Epoll events */
//   epoll_data_t data;	/* User data variable */
// } __EPOLL_PACKED;

typedef struct tcp_Connection {
	int    file_descriptor;
	struct epoll_event ev;
	struct tcp_Connection *next;
} tcp_Connection;

typedef struct {
	int epoll_fd;
} tcp_State;

static tcp_State global_state;

internal void
tcp_Connection_init(tcp_Connection *self, int fd)
{
	self->file_descriptor = fd;
	self->ev.data.ptr = (void*) self;
	self->ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
	self->next = 0;
}

internal void
tcp_Connection_set_fd(tcp_Connection *self, int fd)
{
	self->file_descriptor = fd;
}

internal void
tcp_set_nonblocking_fd(int fd)
{
	int opts = fcntl(fd, F_GETFL);
	Assert(opts >= 0);
	opts = opts | O_NONBLOCK;
	int update_error = fcntl(fd, F_SETFL, opts);
	Assert(!update_error);
}

internal void
tcp_epoll_retrigger_connection(int epoll_fd, tcp_Connection *connection)
{
	epoll_ctl(epoll_fd, EPOLL_CTL_MOD, connection->file_descriptor, &connection->ev);
}

internal void
tcp_epoll_insert_connection(int epoll_fd, tcp_Connection *connection)
{
	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, connection->file_descriptor, &connection->ev);
}

internal
PLATFORM_WORK_QUEUE_CALLBACK(do_work_fd)
{
	tcp_Connection *connection = (tcp_Connection*) data;
	static char buf[1024];
	b8 done = 0;
	while (1) {
		/* this could be running in a work thread */
		int count = read(connection->file_descriptor, buf, sizeof(buf)-1);
		if (count == -1) {
			/* If errno == EAGAIN, that means we have read all
			   data. So go back to the main loop. */
			if (errno != EAGAIN) {
				done = 1;
			}
			break;
		} else if (count == 0) {
			/* End of file. The remote has closed the connection. */
			done = 1;
			break;
		} else {
			buf[count] = 0;
			printf("Input data form fd %d -> %s\n", connection->file_descriptor, buf);
		}
	}
	if (done) {
		printf("Closing Connection %d\n", connection->file_descriptor);
		// printf ("Closed connection on descriptor %d\n",events[i].data.fd);
		/* Closing the descriptor will make epoll remove it
		   from the set of descriptors which are monitored. */
		close(connection->file_descriptor);
	} else {
		/* process request */

		/* retrigger tcp_Connection */
		printf("Retriggering Connection %d\n", connection->file_descriptor);
		tcp_epoll_retrigger_connection(global_state.epoll_fd, connection);
	}
// 	// u64 tid;
// 	// pthread_threadid_np(0, &tid);
// 	// mach_port_t tid = pthread_mach_thread_np(pthread_self());
// 	// pthread_threadid_np(0, &tid);
// 	// pid_t tid = gettid();// pthread_id_np_t tid;
// 	// tid = pthread_getthreadid_np();
// 	printf("thread %lld: %s\n", (u64) pthread_self(), (char*) data);
// 	usleep(1000000);
}










int client()
{
	PlatformAPI platform;
	linux_init_platform(&platform);

	// initialize worker threads
	linux_ThreadInfo thread_info_queue[4];
	pt_WorkQueue work_queue;
	linux_init_work_queue(&work_queue, thread_info_queue, ArrayCount(thread_info_queue));

	/* create tcp server socket */
	int server_socket_fd = socket(PF_INET, SOCK_STREAM, 0);
	Assert(server_socket_fd >= 0);
	// tcp_set_nonblocking_fd(server_socket_fd);
	struct sockaddr_in server_socket_addr;
	server_socket_addr.sin_family = AF_INET;
	server_socket_addr.sin_port = htons(tcp_PORT);
	inet_aton(tcp_LOCAL_ADDR, &(server_socket_addr.sin_addr));
	int connect_status = connect(server_socket_fd, (struct sockaddr *)&server_socket_addr , sizeof(server_socket_addr));
	if (connect_status < 0) {
		puts("connect error");
		return 1;
	}

	/* send n http requests over the same connection */
	char *message = "GET /taxi HTTP/1.1\r\n\r\n";
	for (int i=0;i<3;++i) {
		int send_status = send(server_socket_fd, message, cstr_end(message) - message, 0);
		if (send_status < 0) {
			puts("send error");
			return 1;
		}
	}

	close(server_socket_fd);
	return 0;
}

int server()
{
	PlatformAPI platform;
	linux_init_platform(&platform);

	// initialize worker threads
	linux_ThreadInfo thread_info_queue[4];
	pt_WorkQueue work_queue;
	linux_init_work_queue(&work_queue, thread_info_queue, ArrayCount(thread_info_queue));

	/* create tcp server socket */
	int listen_socket_fd = socket(PF_INET, SOCK_STREAM, 0);
	Assert(listen_socket_fd >= 0);
	tcp_set_nonblocking_fd(listen_socket_fd);
	struct sockaddr_in listen_socket_addr;
	listen_socket_addr.sin_family = AF_INET;
	listen_socket_addr.sin_port = htons(tcp_PORT);
	inet_aton(tcp_LOCAL_ADDR, &(listen_socket_addr.sin_addr));
	int bind_error = bind(listen_socket_fd, (struct sockaddr*) &listen_socket_addr, sizeof(listen_socket_addr));
	Assert(!bind_error);
	int listen_error = listen(listen_socket_fd, tcp_MAX_CONNECTIONS);
	Assert(!listen_error);

	/* create epoll object */
	int epoll_fd = epoll_create(tcp_MAX_CONNECTIONS);
	global_state.epoll_fd = epoll_fd;

	/* register the server socket into the epoll */
	struct epoll_event ev;
	ev.data.ptr = &listen_socket_fd;
	ev.events = EPOLLIN | EPOLLET; // edge trigger
	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_socket_fd, &ev);

	struct epoll_event events[tcp_MAX_EVENTS];

	tcp_Connection connections[tcp_MAX_CONNECTIONS];
	tcp_Connection *free_connections;
	tcp_Connection *idle_connections;
	// tcp_Connection *active_connections;
	{
		// initialize lists
		for (s32 i=0;i<ArrayCount(connections)-1;++i) {
			tcp_Connection_init(connections + i, 0);
			connections[i].next = connections + i + 1;
		}
		tcp_Connection *last_connection = connections + ArrayCount(connections) - 1;
		tcp_Connection_init(last_connection, 0);
		last_connection->next = 0;
		free_connections = connections;
		idle_connections = 0;
		// active_connections = 0;
	}

	printf("Server Running on Port: %d\n", tcp_PORT);
	while (1) {

		int num_fd = epoll_wait(epoll_fd,  events, ArrayCount(events), tcp_TIMEOUT);

		for (int i=0; i<num_fd; ++i) {
			if (events[i].data.ptr == (void*) &listen_socket_fd) {

				struct sockaddr_in new_connection_addr;
				socklen_t new_connection_len;
				int new_connection_fd = accept(listen_socket_fd,
						(struct sockaddr*) &new_connection_addr,
						&new_connection_len);
				Assert(new_connection_fd >= 0);
				tcp_set_nonblocking_fd(new_connection_fd);
				printf("[SERVER] connect from %s \n", inet_ntoa(new_connection_addr.sin_addr));

				/* insert idle connection */
				Assert(free_connections);
				tcp_Connection *new_connection = free_connections;
				free_connections = free_connections->next;
				new_connection->next = idle_connections;
				idle_connections = new_connection;

				tcp_Connection_set_fd(new_connection, new_connection_fd);
				tcp_epoll_insert_connection(epoll_fd, new_connection);

			} else {
				/* expectation here is that there is a complete http request coming */
				platform.work_queue_add_entry(&work_queue, do_work_fd, events[i].data.ptr);
			}
		}
	}
}


int
main(int argc, char* *argv)
{
	if (argc == 1) {
		server();
	} else {
		client();
	}
}

#if 0
				// 	app_state.platform.work_queue_add_entry(app_state.work_queue,test_worker_task,"Load A1");
				// 	app_state.platform.work_queue_add_entry(app_state.work_queue,test_worker_task,"Load A2");


				static char buf[1024];
				while (1) {
					/* this could be running in a work thread */
					int count = read(events[i].data.fd, buf, sizeof(buf));
					if (count == -1) {
						/* If errno == EAGAIN, that means we have read all
						   data. So go back to the main loop. */
						if (errno == EAGAIN) {


						}
						if (errno != EAGAIN)
						{
							perror ("read");
							done = 1;
						}
						break;
					} else if (count == 0) {
						/* End of file. The remote has closed the
						   connection. */
						done = 1;
						break;
					}
					if (done) {
					}



				if (events[i].events & EPOLLIN) {
					printf("EPOLLIN fd: %d\n", events[i].data.fd);
				}
				if (events[i].events & EPOLLHUP) {
					printf("EPOLLHUP fd: %d\n", events[i].data.fd);
				}
				}

	epfd = epoll_create(256);
	listenfd = socket(PF_INET, SOCK_STREAM, 0);
	// set the descriptor as non-blocking
	setnonblocking(listenfd);
	// event related descriptor
	ev.data.fd = listenfd;
	// monitor in message, edge trigger
	ev.events = EPOLLIN | EPOLLET;
	// register epoll event
	epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev);

	bzero(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(SERV_PORT);
	bind(listenfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	listen(listenfd, LISTENQ);





	// nfds is number of events (number of returned fd)
	int i, /*maxi,*/ nfds;
	int listenfd, connfd;
	// read task threads
	pthread_t tid1, tid2;
	// write back threads
	pthread_t tid3, tid4;
	// task node
	struct task *new_task = NULL;
	socklen_t clilen;
	struct sockaddr_in clientaddr;
	struct sockaddr_in serveraddr;

	pthread_mutex_init(&r_mutex, NULL);
	pthread_cond_init(&r_condl, NULL);
	pthread_mutex_init(&w_mutex, NULL);
	pthread_cond_init(&w_condl, NULL);

	// threads for reading thread pool
	pthread_create(&tid1, NULL, readtask, NULL);
	pthread_create(&tid2, NULL, readtask, NULL);
	// threads for respond to client
	pthread_create(&tid3, NULL, writetask, NULL);
	pthread_create(&tid4, NULL, writetask, NULL);

	// epoll descriptor, for handling accept
	epfd = epoll_create(256);
	listenfd = socket(PF_INET, SOCK_STREAM, 0);
	// set the descriptor as non-blocking
	setnonblocking(listenfd);
	// event related descriptor
	ev.data.fd = listenfd;
	// monitor in message, edge trigger
	ev.events = EPOLLIN | EPOLLET;
	// register epoll event
	epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &ev);
#endif


#if 0
int old()
{
	int sockfd;
	struct sockaddr_in self;
	char buffer[MAX_BUF];

	/* Create streaming socket */
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Socket");
		exit(errno);
	}

	/* Initialize address/port structure */
	bzero(&self, sizeof(self));
	self.sin_family = AF_INET;
	/* htons make sure it is little endian encoding */
	self.sin_port = htons(MY_PORT);
	self.sin_addr.s_addr = INADDR_ANY;

	/* Assign a port number to the socket */
	if (bind(sockfd, (struct sockaddr*)&self, sizeof(self)) != 0) {
		perror("socket:bind()");
		exit(errno);
	}

	/* Make it a "listening socket". Limit to 20 connections */
	if (listen(sockfd, 20) != 0) {
		perror("socket:listen()");
		exit(errno);
	}


	/* max connections */




	/* Server run continuously */
	while (1) {
		int clientfd;
		struct sockaddr_in client_addr;
		socklen_t addrlen = sizeof(client_addr);

		// accept an incomming connection
		clientfd = accept(sockfd, (struct sockaddr*)&client_addr, &addrlen);
		printf("%s:%d connected\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

		// this busy wait
		size_t num_received_bytes = recv(clientfd, buffer, MAX_BUF, 0);
		send(clientfd, buffer, num_received_bytes, 0);

		/*
		 * Close data connection
		 */
		close(clientfd);
	}

	/** Clean up */
	close(sockfd);
	return 0;
}
#endif
