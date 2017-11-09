/* include services with common impl. on linux and osx */
#include "osx_linux_platform.c"

PLATFORM_EXECUTABLE_PATH(linux_executable_path)
{
	char path[MAX_FILE_PATH_SIZE];
	pid_t pid = getpid();
	sprintf(path, "/proc/%d/exe", pid);

	ssize_t buf_size = readlink(path, fp->full_path, MAX_FILE_PATH_SIZE);

	Assert(buf_size >= 0);

	{
		fp->end  = fp->full_path + buf_size;
		fp->name = fp->full_path;
		for(char *it=fp->full_path;*it;++it)
		{
			if(*it == '/')
			{
				fp->name = it + 1;
			}
		}
	}
}


/************* Work Queue Support ************/

typedef struct {
	PlatformWorkQueueCallback *callback;
	void *data;
} pt_WorkQueueEntry;

struct pt_WorkQueue {
	u32 volatile completion_goal;
	u32 volatile completion_count;

	u32 volatile next_entry_to_write;
	u32 volatile next_entry_to_read;

	/* platform dependent semaphore primitive */
	sem_t *semaphore_handle;
	sem_t semaphore_handle_storage;

	u32 num_threads;
	u32 done;

	pt_WorkQueueEntry entries[256];
};

internal b8
pt_WorkQueue_init(pt_WorkQueue *self, u32 semaphore_initial_value)
{
	self->completion_goal     = 0;
	self->completion_count    = 0;
	self->next_entry_to_write = 0;
	self->next_entry_to_read  = 0;
	s32 err = sem_init(&self->semaphore_handle_storage, 0, 0);
	self->semaphore_handle = &self->semaphore_handle_storage;
	// self->semaphore_handle = sem_open("/workqueue", O_EXCL); // exclusive O_CREAT); // , 700, 0);
	// self->semaphore_handle = sem_open("/nanocube_workqueue", O_CREAT, 700, 0); // exclusive O_CREAT); // , 700, 0);
	// return self->semaphore_handle != SEM_FAILED;

	self->done        = 0;
	self->num_threads = 0;

	return err == 0;
}


// #define COMPILER_BARRIER() asm volatile("" ::: "memory")
// #define INTERLOCKED_COMPARE_EXCHANGE(a,b,c) asm volatile("" ::: "memory")

PLATFORM_WORK_QUEUE_ADD_ENTRY(linux_work_queue_add_entry)
{
	// TODO(llins): Assuming single producer for now
	// TODO(casey): Switch to InterlockedCompareExchange eventually
	// so that any thread can add?
	u32 new_next_entry_to_write = (queue->next_entry_to_write + 1) % ArrayCount(queue->entries);
	Assert(new_next_entry_to_write != queue->next_entry_to_read);
	pt_WorkQueueEntry *entry = queue->entries + queue->next_entry_to_write;
	entry->callback = callback;
	entry->data = data;
	++queue->completion_goal;

	// _WriteBarrier();
	// OSMemoryBarrier();
	// COMPILER_BARRIER();
	__sync_synchronize();

	queue->next_entry_to_write = new_next_entry_to_write;

	sem_post(queue->semaphore_handle);
	/* ReleaseSemaphore(queue->semaphore_handle, 1, 0); */
}

internal b8
linux_work_queue_work_on_next_entry(pt_WorkQueue *queue)
{
	b8 we_should_sleep = 0;

	u32 original_next_entry_to_read = queue->next_entry_to_read;
	u32 new_next_entry_to_read = (original_next_entry_to_read + 1) % ArrayCount(queue->entries);
	if(original_next_entry_to_read != queue->next_entry_to_write) {
		u32 index = (u32) __sync_val_compare_and_swap_4((s32*) &queue->next_entry_to_read,
								original_next_entry_to_read,
								new_next_entry_to_read);
		// on win32
		// InterlockedCompareExchange((LONG volatile *)&queue->next_entryToRead, NewNextEntryToRead,
		// OriginalNextEntryToRead);
		if(index == original_next_entry_to_read) {
			pt_WorkQueueEntry entry = queue->entries[index];
			entry.callback(queue, entry.data);
			__sync_fetch_and_add_4(&queue->completion_count, 1);
			// InterlockedIncrement((LONG volatile *)&Queue->CompletionCount);
		}
	}
	else {
		we_should_sleep = 1;
	}

	return(we_should_sleep);
}

internal
PLATFORM_WORK_QUEUE_COMPLETE_ALL_WORK(linux_work_queue_complete_all_work)
{
	while(queue->completion_goal != queue->completion_count) {
		linux_work_queue_work_on_next_entry(queue);
	}
	queue->completion_goal  = 0;
	queue->completion_count = 0;
}

internal
PLATFORM_WORK_QUEUE_FINISHED(linux_work_queue_finished)
{
	return queue->completion_goal == queue->completion_count;
}

PLATFORM_WORK_QUEUE_CALLBACK(linux_work_queue_thread_stop)
{
	pt_WorkQueue *queue2 = (pt_WorkQueue*) data;
	pt_atomic_sub_u32(&queue2->num_threads,1);
	pthread_exit(0);
}

global_variable pthread_key_t linux_thread_index_key;
global_variable b8 linux_app_state_interrupted;

/* should be called by the main thread */
internal void
linux_init()
{
	linux_app_state_interrupted = 0;
	pthread_key_create(&linux_thread_index_key,0);
}

internal void*
linux_work_queue_thread_run(void* data)
{
	pt_WorkQueue *queue = (pt_WorkQueue*) data;

	// increment
	u32 index = pt_atomic_add_u32(&queue->num_threads,1);

	pthread_setspecific(linux_thread_index_key, (void*) ((u64) index));

	// u32 TestThreadID = GetThreadID();
	// Assert(TestThreadID == GetCurrentThreadId());
	for(;;) {
		if(linux_work_queue_work_on_next_entry(queue)) {
			sem_wait(queue->semaphore_handle);
			// WaitForSingleObjectEx(Queue->SemaphoreHandle, INFINITE, FALSE);
		}
	}
	return 0;
	// return(0);
}

internal
PLATFORM_WORK_QUEUE_CREATE(linux_work_queue_create)
{
	pt_WorkQueue *queue = (pt_WorkQueue*) malloc(sizeof(pt_WorkQueue));
	if (!pt_WorkQueue_init(queue, num_threads)) {
		free(queue);
		return 0;
	}

	for (u32 i=0;i<num_threads;++i) {
		// 		pthread_attr_t attr;
		// 		pthread_attr_init(&attr);
		// 		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		// 		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		pthread_t thread;
		s32 error = pthread_create(&thread, 0, linux_work_queue_thread_run, queue);
		Assert(!error);
		pthread_detach(thread);
	}

	while (queue->num_threads != num_threads) {
		usleep(10);
	}

	return queue;
}

internal
PLATFORM_GET_THREAD_INDEX(linux_get_thread_index)
{
	return (u64) pthread_getspecific(linux_thread_index_key);
}

internal
PLATFORM_THREAD_SLEEP(linux_thread_sleep)
{
	usleep(millisecs);
}

/* this should be called fro the same thread that called start */
internal
PLATFORM_WORK_QUEUE_DESTROY(linux_work_queue_destroy)
{
	if (!queue)
		return;

	Assert(!queue->done);
	queue->done = 1;

	// schedule exit of the threads
	s32 num_threads = queue->num_threads;

	__sync_synchronize();

	for (s32 i=0;i<num_threads;++i) {
		linux_work_queue_add_entry(queue, linux_work_queue_thread_stop, queue);
	}
	while (queue->num_threads > 0) {
		usleep(10);
	}
	// destroy the semaphore
	sem_destroy(queue->semaphore_handle);
	free(queue);
}

/************* End Work Queue Support ************/


/************* Simple HTTP Server over TCP ************/
//
// Include a notion of a complete request. Use a subset notion of HTTP requests.
// HTTP-request-message = request-line *(header-field CRLF) CRLF [message-body]
// How to recover from unknown input?
//

#include <arpa/inet.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/epoll.h>
#include <sys/mman.h>
#include <sys/socket.h> // socket()
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h> // clock()
#include <unistd.h>

#if 1

// #define linux_tcp_LOCAL_ADDR          "127.0.0.1"
#define linux_tcp_LOCAL_ADDR          "0.0.0.0"
#define linux_tcp_PORT	              8888
#define linux_tcp_MAX_CONNECTIONS     64
#define linux_tcp_MAX_SOCKETS         256
#define linux_tcp_MAX_EVENTS	      64
#define linux_tcp_TIMEOUT             100

#define linux_tcp_Socket_FREE          0
#define linux_tcp_Socket_ACTIVATIING   1
#define linux_tcp_Socket_INUSE         2
#define linux_tcp_Socket_CLOSED        3

#define linux_tcp_BUFFER_SIZE 4096
#define linux_tcp_MAX_SERVERS 16

// struct linux_tcp_Connection {
// 	int    file_descriptor;
// 	int    status;
// 	s32    index;
// 	struct epoll_event ev;
// 	struct linux_tcp_Connection *prev;
// 	struct linux_tcp_Connection *next;
// 	// when retriggering a connection for more data
// 	linux_tcp_Server *linux_server;
// 	f64    start_time;
// };
// A listen socket is associated with a port from which new client
// connections can be established. A server socket is one that gets
// data from a client socket and pushed this data through the handler

// #define linux_tcp_LISTEN_SOCKET 1
// #define linux_tcp_CLIENT_SOCKET 2
// #define linux_tcp_SERVER_SOCKET 3

typedef struct linux_tcp_Engine	linux_tcp_Engine;
typedef struct linux_tcp_Socket linux_tcp_Socket;

struct linux_tcp_Socket {
	int    file_descriptor;
	s32    type;
	s32    index;
	struct epoll_event ev;
	s32    status;
	union {
		struct {
			s32 port;
			u32 num_connections;
		} listen;
		//
		// One should not write on a server socket only
		// wait for data that will trigger the read handler
		// (in some arbitrary thread).
		// On the client socket one can write on it and
		// wait for the read handler to be triggered
		// when some data arrives.
		//
		struct {
			/* increase, decrease number of connections */
			struct linux_tcp_Socket *listen_socket;
		} server;
		struct {
			char *hostname; /* this is only valid if the client preserves it */
			s32 port;
		} client;
	};
	PlatformTCPCallback *callback;
	f64    start_time;
	void   *user_data;

	// which socket are we talking about
	struct linux_tcp_Socket *prev;
	struct linux_tcp_Socket *next;
	linux_tcp_Engine *tcp;
};

#define linux_tcp_TASK_QUEUE_SIZE 256
#define linux_tcp_TASK_QUEUE_MASK 0xff

#define linux_tcp_LISTEN_TASK 0x10
#define linux_tcp_CLIENT_TASK 0x11

typedef struct {
	s32 type;
	//
	// make sure ready is 0 always except when it is
	// ready to be processed (last step a producer
	// does is to set ready to 1 with a memory barrier
	// to everything before
	//
	s32 ready;
	union {
		struct {
			// NOTE(llins): it is simple to add a
			// limit to number of connections per port
		} listen;
		struct {
			char *hostname;
		} client;
	};
	// use the pointer below when scheduling tasks
	// to indicate what happened
	s32  port;
	void *callback;
	void *user_data;
	pt_TCP_Feedback *feedback;
} linux_tcp_Task;

struct linux_tcp_Engine {
	int epoll_fd;
	struct epoll_event  events[linux_tcp_MAX_EVENTS];
	linux_tcp_Socket    sockets[linux_tcp_MAX_SOCKETS];
	linux_tcp_Socket    *free_sockets;
	/* some active_connections might have had its file descriptor closed */
	/* retrieve closed connections once a new connection is requested */
	linux_tcp_Socket    *active_sockets;
	//
	// last thing that should be activated on a task
	// is its status (only when it is totally ready)
	//
	// tasks include: create a new listen port
	//                create a new connection
	//
	// implement a single consumer, multiple producer
	// thread safe mechanism
	//
	struct {
		// at one tcp engine cycle we don't expect to
		// get more than 256 tasks
		linux_tcp_Task queue[linux_tcp_TASK_QUEUE_SIZE];
		// empty config (only look at the least significant byte of the numbers below)
		//
		//  empty/initial:   [s] [c,p ] []     []  [] [] [] ... []
		//  after 1 produce: [s] [c/t1] [p]    []  [] [] [] ... []
		//  after 2 produce: [s] [c/t1] [t2]   [p] [] [] [] ... []
		//  after 1 consume: [ ] [s   ] [c/t2] [p] [] [] [] ... []
		//
		u64 sentinel; // to detect a full queue (consumer is 1 plus sentinel)
		u64 end;   // multi-threades atomic access
	} tasks;
};

global_variable linux_tcp_Engine linux_tcp_engines[linux_tcp_MAX_SERVERS];
global_variable u32 linux_tcp_num_engines = 0;



// OS set filedescriptor to be non-blocking

internal void
linux_tcp_set_nonblocking_fd(int fd)
{
	int opts = fcntl(fd, F_GETFL);
	Assert(opts >= 0);
	opts = opts | O_NONBLOCK;
	int update_error = fcntl(fd, F_SETFL, opts);
	Assert(!update_error);
}

// linux_tcp_Socket

internal void
liunx_tcp_Socket_init_empty(linux_tcp_Socket *self, linux_tcp_Engine *tcp, s32 index)
{
	//TODO(llins): revise this one
	pt_filln((char*) self, sizeof(linux_tcp_Socket), 0);
	self->status = linux_tcp_Socket_FREE; // status;
	self->tcp = tcp;
	self->index = index;
	self->file_descriptor = -1; // fd;
	self->ev.data.ptr = 0;
	self->ev.events = 0;
	self->prev = 0;
	self->next = 0;
	self->start_time = 0;
	self->user_data = 0;
}

internal void
linux_tcp_Socket_epoll_retrigger(linux_tcp_Socket *self)
{
	epoll_ctl(self->tcp->epoll_fd, EPOLL_CTL_MOD, self->file_descriptor, &self->ev);
}

internal void
linux_tcp_Socket_epoll_insert(linux_tcp_Socket *self)
{
	epoll_ctl(self->tcp->epoll_fd, EPOLL_CTL_ADD, self->file_descriptor, &self->ev);
}

internal void
linux_tcp_Socket_init_listen(linux_tcp_Socket *self, int file_descriptor, s32 port, void *user_data,
			     PlatformTCPCallback *callback, u64 start_time)
{
	self->file_descriptor = file_descriptor;
	self->type = pt_TCP_LISTEN_SOCKET;
	self->ev.data.ptr = self;
	self->ev.events = EPOLLIN | EPOLLET; // edge triggered (not oneshot for the listen socket)
	self->status = linux_tcp_Socket_INUSE;
	self->listen.port = port;
	self->callback = callback;
	self->user_data = user_data;
	self->start_time = start_time;
}
internal void
linux_tcp_Socket_init_server(linux_tcp_Socket *self, linux_tcp_Socket *listen_socket, int file_descriptor,
			     void *user_data, PlatformTCPCallback *callback, u64 start_time)
{
	Assert(self->status == linux_tcp_Socket_FREE);
	self->file_descriptor = file_descriptor;
	self->type = pt_TCP_SERVER_SOCKET;
	self->ev.data.ptr = self;
	self->ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
	self->server.listen_socket = listen_socket;
	self->status = linux_tcp_Socket_INUSE;
	self->callback = callback;
	self->user_data = user_data;
	self->start_time = start_time;
}

internal void
linux_tcp_Socket_init_client(linux_tcp_Socket *self, int file_descriptor, void *user_data,
			     s32 port, char *hostname, PlatformTCPCallback *callback, u64 start_time)
{
	Assert(self->status == linux_tcp_Socket_FREE);
	self->file_descriptor = file_descriptor;
	self->type = pt_TCP_CLIENT_SOCKET;
	self->ev.data.ptr = self;
	self->ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
	self->status = linux_tcp_Socket_INUSE;
	self->callback = callback;
	self->user_data = user_data;
	self->start_time = start_time;
}


// internal void
// linux_tcp_Socket_activate(linux_tcp_Socket *self, int fd, f64 start_time)
// {
// 	//TODO(llins): revise this one
// 	Assert(self->file_descriptor == 0);
// 	Assert(self->status == linux_tcp_Socket_FREE);
// 	self->file_descriptor = fd;
// 	self->start_time = start_time;
// 	self->status = linux_tcp_Socket_INUSE;
// }
//
internal void
linux_tcp_Socket_close(linux_tcp_Socket *self)
{
	//TODO(llins): revise this one
	Assert(self->status == linux_tcp_Socket_INUSE);
	self->status = linux_tcp_Socket_CLOSED;
}

// linux_tcp_Engine

internal void
linux_tcp_Engine_init(linux_tcp_Engine *self)
{
	/* create epoll object */
	self->epoll_fd = epoll_create(linux_tcp_MAX_EVENTS);
	Assert(self->epoll_fd);

	linux_tcp_Socket *prev = 0;
	for (s32 i=0;i<ArrayCount(self->sockets);++i) {
		liunx_tcp_Socket_init_empty(self->sockets + i, self, i);
		if (i < ArrayCount(self->sockets)-1) {
			self->sockets[i].next = self->sockets + i + 1;
		} else {
			self->sockets[i].next = 0;
		}
		self->sockets[i].prev = prev;
	}
	self->free_sockets = self->sockets;
	self->active_sockets = 0;
	/* initialize tasks infrastructure */
	self->tasks.sentinel = 0;
	self->tasks.end      = 1;
	for (s32 i=0;i<ArrayCount(self->tasks.queue);++i) {
		self->tasks.queue[i].ready = 0;
	}
}

internal void
linux_tcp_Engine_destroy(linux_tcp_Engine *self)
{
	Assert(self->epoll_fd);
	linux_tcp_Socket *it = self->active_sockets;
	while (it) {
		if (it->status == linux_tcp_Socket_INUSE) {
			close(it->file_descriptor);
			printf("[tcp_destroy] socket: %d  type: %d  status: %d was INUSE and is being closed\n", it->index, it->type, it->status);
		} else if (it->status == linux_tcp_Socket_CLOSED) {
			printf("[tcp_destroy] socket: %d  type: %d  status: %d was already CLOSED\n", it->index, it->type, it->status);
		} else {
			printf("[tcp_destroy] socket: %d  type: %d  status: %d is either FREE or ACTIVATING (not closing it)\n", it->index, it->type, it->status);
		}
		it = it->next;
	}
	close(self->epoll_fd);
	char *p = (char*) self;
	pt_fill(p, p +sizeof(linux_tcp_Engine), 0);
}

internal void
linux_tcp_Engine_release_socket(linux_tcp_Engine *self, linux_tcp_Socket *socket)
{
	Assert(socket->status==linux_tcp_Socket_INUSE || socket->status==linux_tcp_Socket_CLOSED);
	linux_tcp_Socket *prev = socket->prev;
	linux_tcp_Socket *next = socket->next;
	if (prev) {
		prev->next = next;
	}
	if (next) {
		next->prev = prev;
	}
	if (self->active_sockets == socket) {
		Assert(socket->prev == 0);
		self->active_sockets = next;
	}
	socket->next = self->free_sockets;
	socket->prev = 0;
	socket->file_descriptor = 0;
	self->free_sockets = socket;
	socket->status = linux_tcp_Socket_FREE;
}

internal void
linux_tcp_Engine_collect_closed_sockets(linux_tcp_Engine *self)
{
	linux_tcp_Socket *it = self->active_sockets;
	s32 count = 0;
	while (it) {
		linux_tcp_Socket *it_next = it->next;
		if (it->status == linux_tcp_Socket_CLOSED) {
			linux_tcp_Engine_release_socket(self, it);
			++count;
		}
		it = it_next;
	}
	printf("[SERVER] Collect Closed Socket %d\n", count);
}

internal linux_tcp_Socket*
linux_tcp_Engine_reserve_socket(linux_tcp_Engine *self)
{
	if (!self->free_sockets) {
		/* collect closed sockets from active sockets if any */
		linux_tcp_Engine_collect_closed_sockets(self);
	}
	Assert(self->free_sockets);
	linux_tcp_Socket *new_socket = self->free_sockets;
	self->free_sockets = self->free_sockets->next;
	if (self->free_sockets) {
		self->free_sockets->prev = 0;
	}
	new_socket->prev = 0;
	new_socket->next = self->active_sockets;
	if (self->active_sockets) {
		self->active_sockets->prev = new_socket;
	}
	self->active_sockets= new_socket;
	// socket status
	return new_socket;
}

internal void
linux_tcp_Engine_produce_listen_task(linux_tcp_Engine *self, s32 port, void *user_data, PlatformTCPCallback *callback, pt_TCP_Feedback *feedback)
{
	// simply let it overflow
	u64 index = pt_atomic_add_u64(&self->tasks.end, 1);
	index &= linux_tcp_TASK_QUEUE_MASK;
	//
	// NOTE(llins): assumes we don't produce more tasks than the
	// queue size in a single cycle.
	//
	Assert(index != self->tasks.sentinel);
	linux_tcp_Task *task = self->tasks.queue + index;
	Assert(task->ready == 0);
	task->callback = callback;
	task->feedback = feedback;
	task->port = port;
	task->type = linux_tcp_LISTEN_TASK;
	task->user_data = user_data;
	if (feedback) {
		feedback->status = pt_TCP_SOCKET_ACTIVATING;
		feedback->socket.handle    = 0;
		feedback->socket.type      = 0;
		feedback->socket.user_data = 0;
	}
	__sync_synchronize();
	// make sure everything above is ready before we set ready to 1
	task->ready = 1;
}

internal void
linux_tcp_Engine_produce_client_task(linux_tcp_Engine *self, s32 port, char *hostname, void *user_data, PlatformTCPCallback *callback, pt_TCP_Feedback *feedback)
{
	// simply let it overflow
	u64 index = pt_atomic_add_u64(&self->tasks.end, 1);
	index &= linux_tcp_TASK_QUEUE_MASK;
	//
	// NOTE(llins): assumes we don't produce more tasks than the
	// queue size in a single cycle.
	//
	Assert(index != self->tasks.sentinel);
	linux_tcp_Task *task = self->tasks.queue + index;
	Assert(task->ready == 0);
	task->callback = callback;
	task->client.hostname = hostname;
	task->feedback = feedback;
	task->port = port;
	task->type = linux_tcp_CLIENT_TASK;
	task->user_data = user_data;
	if (feedback) {
		feedback->status = pt_TCP_SOCKET_ACTIVATING;
		feedback->socket.handle    = 0;
		feedback->socket.type      = 0;
		feedback->socket.user_data = 0;
	}
	__sync_synchronize();
	// make sure everything above is ready before we set ready to 1
	task->ready = 1;
}

internal void
linux_tcp_Engine_consume_listen_task(linux_tcp_Engine *self, linux_tcp_Task *task)
{
	Assert(task->type == linux_tcp_LISTEN_TASK);
	s32 next_status = pt_TCP_SOCKET_OK;
	linux_tcp_Socket *listen_socket = 0;
	/* create self server socket */
	int listen_socket_fd = socket(PF_INET, SOCK_STREAM, 0);
	if (listen_socket_fd < 0) {
		printf("pt_Engine_ERROR_SOCKET\n");
		goto error;
	}
	/* set as non-blocking */
	linux_tcp_set_nonblocking_fd(listen_socket_fd);
	struct sockaddr_in listen_socket_addr;
	listen_socket_addr.sin_family = AF_INET;
	listen_socket_addr.sin_port = htons(task->port);
	inet_aton(linux_tcp_LOCAL_ADDR, &(listen_socket_addr.sin_addr));
	int bind_error = bind(listen_socket_fd, (struct sockaddr*) &listen_socket_addr, sizeof(listen_socket_addr));
	if (bind_error) {
		printf("pt_Engine_ERROR_BIND\n");
		goto error;
	}
	/* listen */
	int listen_error = listen(listen_socket_fd, linux_tcp_MAX_CONNECTIONS);
	if (listen_error) {
		printf("pt_Engine_ERROR_LISTEN\n");
		goto error;
	}
	/* initialize listen socket report */
	f64 start_time = osx_linux_get_time();
	listen_socket = linux_tcp_Engine_reserve_socket(self);
	if (!listen_socket) {
		printf("[linux_tcp_Engine_consume_tasks]: could not reserver socket\n");
		Assert(listen_socket_fd);
	}
	linux_tcp_Socket_init_listen(listen_socket, listen_socket_fd, task->port, task->user_data, task->callback, start_time);
	/* register into epoll */
	linux_tcp_Socket_epoll_insert(listen_socket);

	goto done;
error:
	next_status = pt_TCP_SOCKET_ERROR;
done:
	if (task->feedback) {
		task->feedback->status = next_status;
		if (listen_socket) {
			task->feedback->socket.handle = listen_socket;
			task->feedback->socket.type = pt_TCP_LISTEN_SOCKET;
			task->feedback->socket.user_data = task->user_data;
		}
	}
	task->ready = 0;
}

// int hostname_to_ip(char *hostname , char *ip)
// {
// 	int sockfd;
// 	struct addrinfo hints, *servinfo, *p;
// 	struct sockaddr_in *h;
// 	int rv;
// 	memset(&hints, 0, sizeof hints);
// 	hints.ai_family = AF_UNSPEC; // use AF_INET6 to force IPv6
// 	hints.ai_socktype = SOCK_STREAM;
// 	if ( (rv = getaddrinfo( hostname , "http" , &hints , &servinfo)) != 0)
// 	{
// 		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
// 		return 1;
// 	}
// 	// loop through all the results and connect to the first we can
// 	for(p = servinfo; p != NULL; p = p->ai_next)
// 	{
// 		h = (struct sockaddr_in *) p->ai_addr;
// 		strcpy(ip , inet_ntoa( h->sin_addr ) );
// 	}
// 	freeaddrinfo(servinfo); // all done with this structure
// 	return 0;
// }

internal void
linux_tcp_Engine_consume_client_task(linux_tcp_Engine *self, linux_tcp_Task *task)
{
	// TODO(llins): continue from here
	Assert(task->type == linux_tcp_CLIENT_TASK);
	s32 next_status = pt_TCP_SOCKET_OK;
	// TODO(llins): which one is it? a hostname or an ip address?
	// assume hostname

	int client_socket_fd = -1;
	f64 start_time = osx_linux_get_time();

	// http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html#getaddrinfo
	struct addrinfo hints;
	hints.ai_family   = AF_UNSPEC;   // don't care IPV4 or IPV6
	hints.ai_socktype = SOCK_STREAM; // TCP stream socket
	hints.ai_flags    = AI_PASSIVE;  // fill in my IP for me
	char port_string[15];
	snprintf(port_string,15,"%d",task->port);
	struct addrinfo *servinfo;
	linux_tcp_Socket *client_socket = 0;

	int status = getaddrinfo(task->client.hostname, port_string, &hints, &servinfo);
	if (status != 0) {
		printf("[linux_tcp_Engine_consume_client_task] Error: getaddrinfo()\n");
		goto error;
	}
	s32 option = 0;
	for (struct addrinfo *p=servinfo; p!=0; p=p->ai_next) {
		++option;
		client_socket_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (client_socket_fd >= 0) {
			int connect_status = connect(client_socket_fd, p->ai_addr, p->ai_addrlen);
			if (connect_status != 0) {
				printf("[linux_tcp_Engine_consume_client_task] Could not connect(...) using addrinfo %d: connect(...)\n", option);
				close(client_socket_fd);
			} else {
				goto connect_ok;
			}
		} else {
			printf("[linux_tcp_Engine_consume_client_task] Could not socket(...) using addrinfo %d: connect(...)\n", option);
		}
	}
	goto error;

connect_ok:
	/* set client to be nonblocking */
	linux_tcp_set_nonblocking_fd(client_socket_fd);

	/* register client socket fd into the epoll */
	client_socket = linux_tcp_Engine_reserve_socket(self);
	if (!client_socket) {
		printf("[linux_tcp_Engine_consume_client_task]: could not reserve socket\n");
		goto error;
	}
	// TODO(llins): register hostname and port on the client socket?
	linux_tcp_Socket_init_client(client_socket, client_socket_fd, task->user_data,
				     task->port, task->client.hostname, task->callback,
				     start_time);
	/* register into epoll */
	linux_tcp_Socket_epoll_insert(client_socket);
	goto done;
error:
	next_status = pt_TCP_SOCKET_ERROR;
done:
	if (task->feedback) {
		task->feedback->status = next_status;
		if (client_socket) {
			task->feedback->socket.handle = client_socket;
			task->feedback->socket.type = pt_TCP_CLIENT_SOCKET;
			task->feedback->socket.user_data = task->user_data;
		}
	}
	task->ready = 0;
}

internal void
linux_tcp_Engine_consume_tasks(linux_tcp_Engine *self)
{
	// how to safely check if task queue is full
	u64 it  = (self->tasks.sentinel + 1) & linux_tcp_TASK_QUEUE_MASK;
	u64 end = self->tasks.end            & linux_tcp_TASK_QUEUE_MASK;
	while (it != end) {
		linux_tcp_Task *task = self->tasks.queue + it;
		if (task->ready) {
			/* listen task */
			if (task->type == linux_tcp_LISTEN_TASK) {
				linux_tcp_Engine_consume_listen_task(self, task);
			} else if (task->type == linux_tcp_CLIENT_TASK) {
				linux_tcp_Engine_consume_client_task(self, task);
			} else {
				printf("[linux_tcp_Engine_consume_tasks] task type not supported\n");
				Assert(0);
			}
		} else {
			// if i-th task is not ready leave remaining
			// tasks to next engine cycle
			break;
		}
		it = (it + 1) & linux_tcp_TASK_QUEUE_MASK;
	}
	if (it == 0) {
		self->tasks.sentinel = linux_tcp_TASK_QUEUE_SIZE - 1;
	} else {
		self->tasks.sentinel = it - 1;
	}
}

internal void
linux_tcp_read_from_connection_and_dispatch(linux_tcp_Socket *socket)
{
	char buf[linux_tcp_BUFFER_SIZE];
	pt_TCP_Socket pt_socket = { .user_data = socket->user_data, .handle = socket };
	b8 done = 0;
	while (1) {
		/* this could be running in a work thread */
		int count = read(socket->file_descriptor, buf, sizeof(buf)-1);
		if (count == -1) {
			/* If errno == EAGAIN, that means we have read all
			   data. So go back to the main loop. */
			if (errno != EAGAIN) {
				done = 1;
			}
			break;
		} else if (count == 0) {
			/* End of file. The remote has closed the socket. */
			done = 1;
			break;
		} else {
			buf[count] = 0;
			printf("[SERVER] Incoming data on %d:%d (%d bytes)\n",
			       socket->index,
			       socket->file_descriptor,
			       count);
#if 0
			printf("====== Input data form fd %3d ======\n", socket->file_descriptor);
			printf("%s\n====== end =========================\n", buf);
#endif
			socket->callback(&pt_socket, buf, count);
		}
	}
	if (done) {
		f64 end_time = osx_linux_get_time();
		printf("[SERVER] Closing Socket %d:%d on (active time: %fs)\n", socket->index, socket->file_descriptor, end_time - socket->start_time);
		// printf ("Closed socket on descriptor %d\n",events[i].data.fd);
		/* Closing the descriptor will make epoll remove it
		   from the set of descriptors which are monitored. */
		close(socket->file_descriptor);
		linux_tcp_Socket_close(socket);
	} else {
		/* NOTE(llins): from what I read, system call here is thread safe */
		printf("[SERVER] Retriggering Socket %d:%d\n", socket->index, socket->file_descriptor);
		linux_tcp_Socket_epoll_retrigger(socket);
	}
}

internal
PLATFORM_WORK_QUEUE_CALLBACK(linux_tcp_work_queue_callback)
{
	linux_tcp_Socket *connection = (linux_tcp_Socket*) data;
	linux_tcp_read_from_connection_and_dispatch(connection);
}

internal void
linux_tcp_Engine_process_events(linux_tcp_Engine *tcp, pt_WorkQueue *work_queue)
{
	/* register listen and client sockets coming as registered tasks */
	linux_tcp_Engine_consume_tasks(tcp);
	/* wait for events on sockets beign tracked for at most linux_tcp_TIMEOUT */
	int num_fd = epoll_wait(tcp->epoll_fd, tcp->events, ArrayCount(tcp->events), linux_tcp_TIMEOUT);
	/* for each socket with some event available process all what
	 * is available (edge triggered)
	 *
	 * for events on listen sockets:
	 *     use the same thread to register new listen sockets to epoll
	 *
	 * for events on client or server sockets:
	 *     use the same thread to run the receive callbacks if work_queue is null
	 *     dispatch callbacks to work queue
	 */
	for (int i=0; i<num_fd; ++i) {
		linux_tcp_Socket *socket = (linux_tcp_Socket*) tcp->events[i].data.ptr;
		if (socket->type == pt_TCP_LISTEN_SOCKET) {
			pf_BEGIN_BLOCK("accepting_connection");
			// try accepetping one or more incoming connections
			while (1) {
				struct sockaddr_in new_socket_addr;
				socklen_t new_socket_len = sizeof(struct sockaddr_in);
				int new_socket_fd = accept(socket->file_descriptor,
							   (struct sockaddr*) &new_socket_addr,
							   &new_socket_len);
				if (new_socket_fd == -1) {
					if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
						break;
					} else {
						printf("[SERVER] Fatal Error: problem when accepting socket\n");
						Assert(0 && "problem when accepting socket");
					}
				}
				Assert(new_socket_fd >= 0);
				linux_tcp_set_nonblocking_fd(new_socket_fd);
				linux_tcp_Socket *new_socket = linux_tcp_Engine_reserve_socket(tcp);
				f64 start_time = osx_linux_get_time();
				printf("[SERVER] New Socket %d:%d from %s:%d\n",
				       new_socket->index,
				       new_socket_fd,
				       inet_ntoa(new_socket_addr.sin_addr),
				       (int) new_socket_addr.sin_port);
				linux_tcp_Socket_init_server(new_socket, socket, new_socket_fd, socket->user_data, socket->callback, start_time);
				linux_tcp_Socket_epoll_insert(new_socket);
			}

			pf_END_BLOCK();

		} else {
			if (work_queue) {
				/* work threads */
				linux_work_queue_add_entry(work_queue, linux_tcp_work_queue_callback, socket);
			} else {
				/* single threaded */
				linux_tcp_read_from_connection_and_dispatch(socket);
			}
		}
	}
}

internal
PLATFORM_TCP_PROCESS_EVENTS(linux_tcp_process_events)
{
	linux_tcp_Engine_process_events((linux_tcp_Engine*) tcp.handle, work_queue);
}

internal
PLATFORM_TCP_WRITE(linux_tcp_write)
{
	if (length == 0)
		return;
	linux_tcp_Socket *linux_socket = (linux_tcp_Socket*) socket->handle;
	s64 offset = 0;
	while (1) {
		// https://stackoverflow.com/questions/108183/how-to-prevent-sigpipes-or-handle-them-properly
		// s64 count = write(linux_socket->file_descriptor, buffer + offset, length - offset);
		s64 count = send(linux_socket->file_descriptor, buffer + offset, length - offset, MSG_NOSIGNAL );

		if (count == -1) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				// printf("[SERVER] received EAGAIN || EWOULDBLOCK with -1 result, will try again...\n");
				// usleep(1);
				continue;
			} else {
				printf("[SERVER] write count == -1 on socket %d:%d errorno %d is %s (exiting server respond)\n",
				       linux_socket->index, linux_socket->file_descriptor, errno,
				       strerror(errno));
				return;
			}
		} else if (count == 0) {
			printf("[SERVER] write count == 0 on socket %d:%d errorno %d is %s\n",
			       linux_socket->index, linux_socket->file_descriptor, errno,
			       strerror(errno));
			// TODO(llins): revise these cases. not sure what is the right approach.
			return;
		} else if (count > 0) {
			offset += count;
			if (offset == length) {
				printf("[SERVER] written %d bytes on socket (response done %d) %d:%d\n", (int) count,
				       (int) length, linux_socket->index, linux_socket->file_descriptor);
				break;
			} else {
				printf("[SERVER] written %d bytes on socket %d:%d\n", (int) count,
				       linux_socket->index, linux_socket->file_descriptor);
			}
		}
	}
}

internal
PLATFORM_TCP_CREATE(linux_tcp_create)
{
	pt_TCP result = { .handle = 0 };
	if (linux_tcp_num_engines == ArrayCount(linux_tcp_engines)) {
		return result;
	}

	/* linux server */
	linux_tcp_Engine *linux_tcp_engine = linux_tcp_engines + linux_tcp_num_engines;

	linux_tcp_Engine_init(linux_tcp_engine);

	/* linux server */
	++linux_tcp_num_engines;
	result.handle = (void*) linux_tcp_engine;
	return result;
}

internal
PLATFORM_TCP_DESTROY(linux_tcp_destroy)
{
	if (!tcp.handle) {
		return;
	}
	linux_tcp_Engine *tcp_engine = (linux_tcp_Engine*) tcp.handle;
	linux_tcp_Engine_destroy(tcp_engine);
}

// #define PLATFORM_TCP_SERVE(name) void name(pt_TCP *tcp, s32 port, void *user_data, PlatformTCPCallback *callback, s32 *status)
internal
PLATFORM_TCP_SERVE(linux_tcp_serve)
{
	linux_tcp_Engine *linux_tcp_engine = (linux_tcp_Engine*) tcp.handle;
	/* schedule tcp engine to create a listen socket on next process events cycle */
	linux_tcp_Engine_produce_listen_task(linux_tcp_engine, port, user_data, callback, feedback);
}


// #define PLATFORM_TCP_CLIENT(name) void name(pt_TCP *tcp, s32 port, char *hostname, void *user_data, PlatformTCPCallback *callback, s32 *status)
internal
PLATFORM_TCP_CLIENT(linux_tcp_client)
{
	linux_tcp_Engine *linux_tcp_engine = (linux_tcp_Engine*) tcp.handle;
	/* schedule tcp engine to create a client socket on next process events cycle */
	/* client socket should connect to host referred on hostname */
	// TODO(llins): should we copy hostname to a safe place? (client code might destroy it)
	linux_tcp_Engine_produce_client_task(linux_tcp_engine, port, hostname, user_data, callback, feedback);
}

//
//
//
// PLATFORM_SERVER_CREATE(linux_server_create)
// {
// 	Assert(linux_tcp_num_servers < linux_tcp_MAX_SERVERS - 1);
//
// 	pt_Engine server = { .handle = 0 };
//
// 	/* linux server */
// 	linux_tcp_Engine *linux_server = linux_tcp_servers + linux_tcp_num_servers;
//
// 	/* create tcp server socket */
// 	int listen_socket_fd = socket(PF_INET, SOCK_STREAM, 0);
// 	if (listen_socket_fd < 0) {
// 		printf("pt_Engine_ERROR_SOCKET\n");
// 		return server;
// 	}
//
// 	linux_tcp_set_nonblocking_fd(listen_socket_fd);
// 	struct sockaddr_in listen_socket_addr;
// 	listen_socket_addr.sin_family = AF_INET;
// 	// TODO(llins): use input port number
// 	listen_socket_addr.sin_port = htons(port); // linux_tcp_PORT);
// 	inet_aton(linux_tcp_LOCAL_ADDR, &(listen_socket_addr.sin_addr));
// 	int bind_error = bind(listen_socket_fd, (struct sockaddr*) &listen_socket_addr, sizeof(listen_socket_addr));
// 	if (bind_error) {
// 		printf("pt_Engine_ERROR_BIND\n");
// 		return server;
// 	}
// 	int listen_error = listen(listen_socket_fd, linux_tcp_MAX_CONNECTIONS);
// 	if (listen_error) {
// 		printf("pt_Engine_ERROR_LISTEN\n");
// 		return server;
// 	}
//
// 	/* create epoll object */
// 	int epoll_fd = epoll_create(linux_tcp_MAX_CONNECTIONS);
//
// 	/* register the server socket into the epoll */
// 	struct epoll_event ev;
// 	ev.data.ptr = &linux_server->listen_socket_fd;
// 	ev.events = EPOLLIN | EPOLLET; // edge trigger
// 	epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_socket_fd, &ev);
//
// 	/* linux server */
// 	++linux_tcp_num_servers;
//
// 	/* initialize linux_tcp_Engine */
// 	linux_tcp_Engine_init(linux_server, listen_socket_fd, epoll_fd, port);
// 	linux_server->user_data = user_data;
// 	linux_server->request_handler   = handler;
//
// 	server.handle = (void*) linux_server;
//
// 	return server;
// }
//
// //
// // if no work queue, single threaded server version this call blocks
// // loop using epoll and schedule or process incoming data
// //
// PLATFORM_SERVER_START(linux_server_start)
// {
// 	Assert(server->handle);
// 	linux_tcp_Engine *linux_server = (linux_tcp_Engine*) server->handle;
// 	printf("Engine Running on Port: %d\n", linux_server->port);
// 	// f64 t0 = osx_linux_get_time();
// 	while (1) {
//
// 		if (linux_app_state_interrupted)
// 			break;
//
// 		int num_fd = epoll_wait(linux_server->epoll_fd, linux_server->events, ArrayCount(linux_server->events), linux_tcp_TIMEOUT);
//
// 		for (int i=0; i<num_fd; ++i) {
// 			if (linux_server->events[i].data.ptr == (void*) &linux_server->listen_socket_fd) {
//
// 				pf_BEGIN_BLOCK("accepting_connection");
//
// 				// try accepetping one or more incoming connections
// 				while (1) {
// 					struct sockaddr_in new_connection_addr;
// 					socklen_t new_connection_len = sizeof(struct sockaddr_in);
//
//
// 					int new_connection_fd = accept(linux_server->listen_socket_fd,
// 								       (struct sockaddr*) &new_connection_addr,
// 								       &new_connection_len);
//
//
// 					if (new_connection_fd == -1) {
// 						if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
// 							break;
// 						} else {
// 							printf("[SERVER] Fatal Error: problem when accepting connection\n");
// 							Assert(0 && "problem when accepting connection");
// 						}
// 					}
//
// 					Assert(new_connection_fd >= 0);
// 					linux_tcp_set_nonblocking_fd(new_connection_fd);
// 					linux_tcp_Socket *new_connection = linux_tcp_Engine_new_connection(linux_server);
// 					f64 start_time = osx_linux_get_time();
// 					printf("[SERVER] New Socket %d:%d from %s:%d\n",
// 					       new_connection->index,
// 					       new_connection_fd,
// 					       inet_ntoa(new_connection_addr.sin_addr),
// 					       (int) new_connection_addr.sin_port);
// 					// TODO(llins): how to call the get_time
// 					linux_tcp_Socket_activate(new_connection, new_connection_fd, start_time);
// 					linux_tcp_epoll_insert_connection(new_connection);
// 				}
//
// 				pf_END_BLOCK();
//
// 			} else {
// 				if (work_queue) {
// 					/* work threads */
// 					linux_work_queue_add_entry(work_queue, linux_tcp_work_queue_callback, linux_server->events[i].data.ptr);
// 				} else {
// 					/* single threaded */
// 					linux_tcp_read_from_connection_and_dispatch((linux_tcp_Socket*) linux_server->events[i].data.ptr);
// 				}
// 			}
// 		}
// 	}
// }


#endif
// struct linux_tcp_Connection {
// 	int    file_descriptor;
// 	int    status;
// 	s32    index;
// 	struct epoll_event ev;
// 	struct linux_tcp_Connection *prev;
// 	struct linux_tcp_Connection *next;
// 	// when retriggering a connection for more data
// 	linux_tcp_Server *linux_server;
// 	f64    start_time;
// };


#if 0
typedef struct {
	s32 file_descriptor;
	s32 port;
	char *hostname;
} linux_tcp_OutgoingConnection;


internal linux_tcp_OutgoingConnection
linux_connect_to_server(char *hostname, s32 port)
{
	linux_tcp_OutgoingConnection conn;

	conn.file_descriptor = socket(AF_INET, SOCK_STREAM, 0);
	if (conn.file_descriptor < 0) {
		printf("[linux_connect_to_server] Error: socket(AF_INET, SOCK_STREAM, 0);\n");
		return conn;
	}

	struct hostent *server = gethostbyname(hostname);
	if (server == 0) {
		printf("[linux_connect_to_server] Error: failed on gethostbyname\n");
		goto close_socket;
	}

	struct sockaddr_in serveraddr;
	pt_filln((char *) &serveraddr, sizeof(serveraddr), 0);

	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons(port);
	pt_copy_bytesn((char*)server->h_addr, (char*)&serveraddr.sin_addr.s_addr, server->h_length);

	if (connect(conn.file_descriptor, &serveraddr, sizeof(serveraddr)) < 0) {
		conn.file_descriptor = -1;
		printf("[linux_connect_to_server] Error: failed to connect\n");
		goto close_socket;
	}

	goto done;

close_socket:
	Assert(conn.file_descriptor >= 0);
	close(conn.file_descriptor);
	conn.file_descriptor = -1;
done:
	return conn;




//
//
// 	int sockfd, portno, n;
// 	struct sockaddr_in serveraddr;
// 	struct hostent *server;
// 	char *hostname;
// 	char buf[BUFSIZE];
//
// 	/* socket: create the socket */
// 	sockfd = socket(AF_INET, SOCK_STREAM, 0);
// 	if (sockfd < 0)
// 		error("ERROR opening socket");
// //
// 	/* gethostbyname: get the server's DNS entry */
// 	server = gethostbyname(hostname);
// 	if (server == NULL) {
// 		fprintf(stderr,"ERROR, no such host as %s\n", hostname);
// 		exit(0);
// 	}
// //
// 	/* build the server's Internet address */
// 	bzero((char *) &serveraddr, sizeof(serveraddr));
// 	serveraddr.sin_family = AF_INET;
// 	bcopy((char *)server->h_addr,
// 	      (char *)&serveraddr.sin_addr.s_addr, server->h_length);
// 	serveraddr.sin_port = htons(portno);
// //
// 	/* connect: create a connection with the server */
// 	if (connect(sockfd, &serveraddr, sizeof(serveraddr)) < 0)
// 		error("ERROR connecting");
//
// 	//
// 	// NOTE(llins): once a connection is established we could
// 	// reuse it many times to perform serial requests. The first
// 	// function could end here
// 	//
// 	// once a request is sent it gets a serial number "i" (we need
// 	// HTTP logic to understand the logic of a "message" unit)
// 	// the i-th incoming response message will be the response
// 	// to this request.
// 	//
// 	// the same socket can be concurrently used by two different
// 	// threads to read and write
// 	//
// 	// on the client side, we can send a request async and wait
// 	// check for the response later
// 	//
// 	// connection = platform.connect(server)
// 	//
// 	// multiple connections are being managed and some threads
// 	// can be activated to read data
// 	//
// 	// response = platform.query(connection, message)
// 	// /* if sync, response comes with a ready flag on, otherwise user */
// 	//
// 	//
// 	// start as a server with a loop to keep track of the
// 	// incoming packages on the registered connections
// 	//
//
// //
// 	/* get message line from the user */
// 	printf("Please enter msg: ");
// 	bzero(buf, BUFSIZE);
// 	fgets(buf, BUFSIZE, stdin);
// //
// 	/* send the message line to the server */
// 	n = write(sockfd, buf, strlen(buf));
// 	if (n < 0)
// 		error("ERROR writing to socket");
// //
// 	/* print the server's reply */
// 	bzero(buf, BUFSIZE);
// 	n = read(sockfd, buf, BUFSIZE);
// 	if (n < 0)
// 		error("ERROR reading from socket");
// 	printf("Echo from server: %s", buf);
// 	close(sockfd);
// 	return 0;
// //
}

#endif






// initialize linux platform to PlatformAPI

internal void
linux_init_platform(PlatformAPI* p)
{
	/* initialize common services */
	osx_linux_init_platform(p);
	p->executable_path          = linux_executable_path;
	p->work_queue_add_entry     = linux_work_queue_add_entry;
	p->work_queue_complete_work = linux_work_queue_complete_all_work;
	p->work_queue_create        = linux_work_queue_create;
	p->work_queue_destroy       = linux_work_queue_destroy;
	p->work_queue_finished      = linux_work_queue_finished;
	p->get_thread_index         = linux_get_thread_index;
	p->thread_sleep             = linux_thread_sleep;

	/* tcp support */
	p->tcp_create               = linux_tcp_create;
	p->tcp_destroy              = linux_tcp_destroy;
	p->tcp_serve                = linux_tcp_serve;
	p->tcp_write                = linux_tcp_write;
	p->tcp_process_events       = linux_tcp_process_events;
	p->tcp_client               = linux_tcp_client;
}

