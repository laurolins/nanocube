// socket interface
#include <sys/socket.h>
// epoll interface
#include <sys/epoll.h>
// struct sockaddr_in
#include <netinet/in.h>
// IP addr convertion
#include <arpa/inet.h>
// File descriptor controller
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
// bzero()
#include <string.h>
// malloc(), free()
#include <stdlib.h>
#include <errno.h>
extern int errno;

// maximum received data byte
#define MAXBTYE     10
#define OPEN_MAX    100
#define LISTENQ     20
#define SERV_PORT   10012
#define INFTIM      1000
#define LOCAL_ADDR  "127.0.0.1"
#define TIMEOUT     500

// task item in thread pool
struct task
{
	// file descriptor or user_data
	epoll_data_t data;
	// next task
	struct task* next;
};

// for data transporting
struct user_data
{
	int fd;
	// real received data size
	unsigned int n_size;
	// content received
	char line[MAXBTYE];
};

void *readtask(void *args);
void *writetask(void *args);

// epoll descriptor from epoll_create()
int epfd;
// register epoll_ctl()
struct epoll_event ev;
// store queued events from epoll_wait()
struct epoll_event events[LISTENQ];
// mutex lock to protect readhead/readtail
pthread_mutex_t r_mutex;
pthread_cond_t r_condl;
// mutex lock to protect writehead/writetail
pthread_mutex_t w_mutex;
pthread_cond_t w_condl;
struct task *readhead = NULL, *readtail = NULL;
struct task *writehead = NULL, *writetail = NULL;

void setnonblocking(int sock)
{
	int opts;
	if ((opts = fcntl(sock, F_GETFL)) < 0)
		exit(-1); //errexit("GETFL %d failed", sock);
	opts = opts | O_NONBLOCK;
	if (fcntl(sock, F_SETFL, opts) < 0)
		exit(-1); 
		// errexit("SETFL %d failed", sock);
}

int main(int argc,char* argv[])
{
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

	bzero(&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	char *local_addr = LOCAL_ADDR;
	inet_aton(local_addr, &(serveraddr.sin_addr));
	serveraddr.sin_port = htons(SERV_PORT);
	bind(listenfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	listen(listenfd, LISTENQ);

	/* maxi = 0; */
	for(;;)
	{
		// waiting for epoll event
		nfds = epoll_wait(epfd, events, LISTENQ, TIMEOUT);
		// In case of edge trigger, must go over each event
		for (i = 0; i < nfds; ++i)
		{
			// Get new connection
			if (events[i].data.fd == listenfd)
			{
				// accept the client connection
				connfd = accept(listenfd, (struct sockaddr*)&clientaddr, &clilen);
				if (connfd < 0)
					exit(-1);
				setnonblocking(connfd);
				printf("[SERVER] connect from %s \n", inet_ntoa(clientaddr.sin_addr));
				ev.data.fd = connfd;
				// monitor in message, edge trigger
				ev.events = EPOLLIN | EPOLLET;
				// add fd to epoll queue
				epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ev);
			}
			// Received data
			else if (events[i].events & EPOLLIN)
			{
				if (events[i].data.fd < 0)
					continue;
				printf("[SERVER] put task %d to read queue\n", events[i].data.fd);
				new_task = malloc(sizeof(struct task));
				new_task->data.fd = events[i].data.fd;
				new_task->next = NULL;
				//printf("[SERVER] thread %d epollin before lock\n", pthread_self());
				// protect task queue (readhead/readtail)
				pthread_mutex_lock(&r_mutex);
				//printf("[SERVER] thread %d epollin after lock\n", pthread_self());
				// the queue is empty
				if (readhead == NULL)
				{
					readhead = new_task;
					readtail = new_task;
				}
				// queue is not empty
				else
				{
					readtail->next = new_task;
					readtail = new_task;
				}
				// trigger readtask thread
				pthread_cond_broadcast(&r_condl);
				//printf("[SERVER] thread %d epollin before unlock\n", pthread_self());
				pthread_mutex_unlock(&r_mutex);
				//printf("[SERVER] thread %d epollin after unlock\n", pthread_self());
			}
			// Have data to send
			else if (events[i].events & EPOLLOUT)
			{
				if (events[i].data.ptr == NULL)
					continue;
				printf("[SERVER] put task %d to write queue\n", ((struct task*)events[i].data.ptr)->data.fd);
				new_task = malloc(sizeof(struct task));
				new_task->data.ptr = (struct user_data*)events[i].data.ptr;
				new_task->next = NULL;
				//printf("[SERVER] thread %d epollout before lock\n", pthread_self());
				pthread_mutex_lock(&w_mutex);
				//printf("[SERVER] thread %d epollout after lock\n", pthread_self());
				// the queue is empty
				if (writehead == NULL)
				{
					writehead = new_task;
					writetail = new_task;
				}
				// queue is not empty
				else
				{
					writetail->next = new_task;
					writetail = new_task;
				}
				// trigger writetask thread
				pthread_cond_broadcast(&w_condl);
				//printf("[SERVER] thread %d epollout before unlock\n", pthread_self());
				pthread_mutex_unlock(&w_mutex);
				//printf("[SERVER] thread %d epollout after unlock\n", pthread_self());
			}
			else
			{
				printf("[SERVER] Error: unknown epoll event");
			}
		}
	}

	return 0;
}

void *readtask(void *args)
{
	int fd = -1;
	int n, i;
	struct user_data* data = NULL;
	while(1)
	{
		//printf("[SERVER] thread %d readtask before lock\n", pthread_self());
		// protect task queue (readhead/readtail)
		pthread_mutex_lock(&r_mutex);
		//printf("[SERVER] thread %d readtask after lock\n", pthread_self());
		while(readhead == NULL)
			// if condl false, will unlock mutex
			pthread_cond_wait(&r_condl, &r_mutex);

		fd = readhead->data.fd;
		struct task* tmp = readhead;
		readhead = readhead->next;
		free(tmp);

		//printf("[SERVER] thread %d readtask before unlock\n", pthread_self());
		pthread_mutex_unlock(&r_mutex);
		//printf("[SERVER] thread %d readtask after unlock\n", pthread_self());

		printf("[SERVER] readtask %d handling %d\n", (int) pthread_self(), fd);
		data = malloc(sizeof(struct user_data));
		data->fd = fd;
		if ((n = recv(fd, data->line, MAXBTYE, 0)) < 0)
		{
			if (errno == ECONNRESET)
				close(fd);
			printf("[SERVER] Error: readline failed: %s\n", strerror(errno));
			if (data != NULL)
				free(data);
		}
		else if (n == 0)
		{
			close(fd);
			printf("[SERVER] Error: client closed connection.\n");
			if (data != NULL)
				free(data);
		}
		else
		{
			data->n_size = n;
			for (i = 0; i < n; ++i)
			{
				if (data->line[i] == '\n' /*|| data->line[i] > 128*/)
				{
					data->line[i] = '\0';
					data->n_size = i + 1;
				}
			}
			printf("[SERVER] readtask %d received %d : [%d] %s\n", (int) pthread_self(), 
			       fd, data->n_size, data->line);
			if (data->line[0] != '\0')
			{
				// modify monitored event to EPOLLOUT,  wait next loop to send respond
				ev.data.ptr = data;
				// Modify event to EPOLLOUT
				ev.events = EPOLLOUT | EPOLLET;
				// modify moditored fd event
				epoll_ctl(epfd, EPOLL_CTL_MOD, fd, &ev);
			}
		}
	}
}

void *writetask(void *args)
{
	int n;
	// data to wirte back to client
	struct user_data *rdata = NULL;
	while(1)
	{
		//printf("[SERVER] thread %d writetask before lock\n", pthread_self());
		pthread_mutex_lock(&w_mutex);
		//printf("[SERVER] thread %d writetask after lock\n", pthread_self());
		while(writehead == NULL)
			// if condl false, will unlock mutex
			pthread_cond_wait(&w_condl, &w_mutex);

		rdata = (struct user_data*)writehead->data.ptr;
		struct task* tmp = writehead;
		writehead = writehead->next;
		free(tmp);

		//printf("[SERVER] thread %d writetask before unlock\n", pthread_self());
		pthread_mutex_unlock(&w_mutex);
		//printf("[SERVER] thread %d writetask after unlock\n", pthread_self());

		printf("[SERVER] writetask %d sending %d : [%d] %s\n", (int) pthread_self(), rdata->fd, rdata->n_size, rdata->line);
		// send responce to client
		if ((n = send(rdata->fd, rdata->line, rdata->n_size, 0)) < 0)
		{
			if (errno == ECONNRESET)
				close(rdata->fd);
			printf("[SERVER] Error: send responce failed: %s\n", strerror(errno));
		}
		else if (n == 0)
		{
			close(rdata->fd);
			printf("[SERVER] Error: client closed connection.");
		}
		else
		{
			// modify monitored event to EPOLLIN, wait next loop to receive data
			ev.data.fd = rdata->fd;
			// monitor in message, edge trigger
			ev.events = EPOLLIN | EPOLLET;
			// modify moditored fd event
			epoll_ctl(epfd, EPOLL_CTL_MOD, rdata->fd, &ev);
		}
		// delete data
		free(rdata);
	}
}

