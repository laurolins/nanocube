// Platform services for *nix-like platforms: -DOS_MAC, -DOS_LINUX

#if defined(OS_MAC)
#elif defined(OS_LINUX)
#else
#error "Invalid OS"
#endif

#include <arpa/inet.h>
#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <netdb.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h> // socket()
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h> // clock()
#include <unistd.h>
#include <x86intrin.h>

#if defined(OS_MAC)
// dispatch_semaphore_t semaphore;
// semaphore = dispatch_semaphore_create(1); // init with value of 1
#elif defined(OS_LINUX)
#endif

#if defined(OS_MAC)
#include <sys/event.h>           // kqueue, kevent
#include <dispatch/dispatch.h>   // dispatch_semaphore_create, etc
#include <mach-o/dyld.h>         // _NSGetExecutablePath
#elif defined(OS_LINUX)
#include <sys/epoll.h>           // epoll_*
#endif


PLATFORM_ALLOCATE_MEMORY(nix_allocate_memory)
{
	pt_Memory mem;
	u64 stride = (1ull << alignment);
	u64 aligned_size = size + stride - 1;
	mem.handle = malloc(aligned_size);

	Assert(mem.handle && "Could not allocate enough memory");

	mem.memblock.begin = (void*) RALIGN((u64) mem.handle, stride);
	mem.memblock.end   = mem.memblock.begin + size;

	return mem;
};

/* b8 name(pt_Memory* mem, u64 new_size, u8 alignment) */
PLATFORM_RESIZE_MEMORY(nix_resize_memory)
{
	u64 size   = mem->memblock.end - mem->memblock.begin;
	s64 stride = (1ull << alignment);
	s64 offset = mem->memblock.begin - (char*) mem->handle;
	u64 aligned_new_size = new_size + stride - 1;
	void *new_handle = realloc(mem->handle, aligned_new_size);
	if (!new_handle) {
		return 0;
	}
	if (alignment) {
		char *it_src = (char*) new_handle + offset;
		char *it_dst = (char*) RALIGN((u64) new_handle, stride);
		s64 delta = it_dst - it_src;
		s64 bytes_to_copy = MIN(size,new_size);
		Assert(bytes_to_copy);
		if (delta < 0) {
			char *it_dst_end = it_dst + bytes_to_copy;
			while (it_dst != it_dst_end) {
				*it_dst = *it_src;
				++it_src;
				++it_dst;
			}
		} else if (delta > 0) {
			char *it_dst_rev = it_dst + bytes_to_copy - 1;
			char *it_dst_rev_end = it_dst - 1;
			char *it_src_rev = it_src + bytes_to_copy - 1;
			while (it_dst_rev != it_dst_rev_end) {
				*it_dst_rev = *it_src_rev;
				--it_dst_rev;
				--it_src_rev;
			}
		}
		mem->handle         = new_handle;
		mem->memblock.begin = it_dst;
		mem->memblock.end   = mem->memblock.begin + new_size;
		Assert(((u64) it_dst % stride) == 0);
	} else {
		/* replace block into mem struct */
		mem->handle = new_handle;
		mem->memblock.begin = (char*) new_handle + offset;
		mem->memblock.end   = mem->memblock.begin + new_size;
	}
	return 1;
};

PLATFORM_FREE_MEMORY(nix_free_memory)
{
	/* should be robust to null pointer */
	free(pm->handle);
}

PLATFORM_COPY_MEMORY(nix_copy_memory)
{
	memcpy(dest, src, count);
}

PLATFORM_OPEN_MMAP_FILE(nix_open_mmap_file)
{
	struct stat   file_stat;

	char          file_name[1024];
	Assert(file_end - file_begin + 1 < 1024);
	u64 len = pt_copy_bytes(file_begin, file_end, file_name, file_name + 1023);
	file_name[len] = 0;

	pt_MappedFile mapped_file;
	pt_fill((char*) &mapped_file, (char*) &mapped_file + sizeof(pt_MappedFile), 0);


	if (read && !write) {

		s32 file_descriptor = open(file_name, O_RDONLY);

		if (file_descriptor == -1) {
			return mapped_file;
		}

		if (fstat(file_descriptor, &file_stat) == -1) {
			return mapped_file;
		}

		if (!S_ISREG(file_stat.st_mode)) {
			return mapped_file;
		}

		mapped_file.size  = file_stat.st_size;

		/* map file in a shared fashion for reading */
		mapped_file.handle = mmap (0, file_stat.st_size, PROT_READ, MAP_SHARED, file_descriptor, 0);
		if (mapped_file.handle == MAP_FAILED) {
			return mapped_file;
		}

		if (close(file_descriptor) == -1) {
			return mapped_file;
		}

		mapped_file.read   = 1;
		mapped_file.mapped = 1;
		mapped_file.begin  = (char*) mapped_file.handle;

	} else if (read && write) {

		s32 file_descriptor = open(file_name, O_RDWR);

		if (file_descriptor == -1) {
			return mapped_file;
		}

		if (fstat(file_descriptor, &file_stat) == -1) {
			return mapped_file;
		}

		if (!S_ISREG(file_stat.st_mode)) {
			return mapped_file;
		}

		mapped_file.size  = file_stat.st_size;

		/* map file in a shared fashion for reading */
		mapped_file.handle = mmap (0, file_stat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, file_descriptor, 0);
		if (mapped_file.handle == MAP_FAILED) {
			return mapped_file;
		}

		if (close(file_descriptor) == -1) {
			return mapped_file;
		}

		mapped_file.write  = 1;
		mapped_file.read   = 1;
		mapped_file.mapped = 1;
		mapped_file.begin  = (char*) mapped_file.handle;

		return mapped_file;

	}
	return mapped_file;
}

PLATFORM_RESIZE_FILE(nix_resize_file)
{
	char file_name[1024];
	Assert(file_end - file_begin + 1 < 1024);
	u64 len = pt_copy_bytes(file_begin, file_end, file_name, file_name + 1023);
	file_name[len] = 0;
	s32 file_descriptor = open(file_name, O_RDWR);
	s32 result = !ftruncate(file_descriptor, new_size);
	close(file_descriptor);
	return result;
}

PLATFORM_CLOSE_MMAP_FILE(nix_close_mmap_file)
{
	Assert(mapped_file->mapped);
	if (munmap(mapped_file->handle, mapped_file->size) == -1) {
		return;
	}
	mapped_file->mapped   = 0;
	mapped_file->unmapped = 1;
	mapped_file->handle   = 0;
	mapped_file->begin    = 0;
}

PLATFORM_OPEN_READ_FILE(nix_open_read_file)
{
	char *name = malloc(file_end - file_begin + 1);
	u64 len = file_end - file_begin;
	pt_copy_bytes(file_begin, file_end, name, name + len);
	*(name + len) = 0;

	FILE *fp = fopen(name, "rb");

	free(name);

	pt_File pfh;
	if (fp) {
		fseek(fp, 0L, SEEK_END);
		pfh.size = ftell(fp);
		fseek(fp, 0L, SEEK_SET);
	} else {
		pfh.size = 0;
	}
	pfh.open = fp ? 1 : 0;
	pfh.eof  = 0;
	pfh.last_read = 0;
	pfh.last_seek_success = 0;
	pfh.handle = fp;
	pfh.read = 1;
	pfh.write = 0;
	return pfh;
}

PLATFORM_OPEN_WRITE_FILE(nix_open_write_file)
{
	char *name = malloc(file_end - file_begin + 1);
	u64 len = file_end - file_begin;
	pt_copy_bytes(file_begin, file_end, name, name + len);
	*(name + len) = 0;

	FILE *fp = fopen(name, "wb");

	free(name);

	pt_File pfh;
	pfh.open = fp ? 1 : 0;
	pfh.eof  = 0;
	pfh.last_read = 0;
	pfh.last_seek_success = 0;
	pfh.handle = fp;
	pfh.read = 0;
	pfh.write = 1;
	pfh.size = 0;
	return pfh;
}

PLATFORM_READ_NEXT_FILE_CHUNK(nix_read_next_file_chunk)
{
	Assert(pfh->open && !pfh->eof && pfh->read);
	FILE* fp = (FILE*) pfh->handle;
	u64 buflen = buffer_end - buffer_begin;
	pfh->last_read = fread(buffer_begin, 1, buflen, fp);
	if (pfh->last_read < buflen) {
		// if read less than buffer size expect eof flag is on
		// and no error flag is set
		Assert(feof(fp) && !ferror(fp) && "FileTokenizer: error reading");
		pfh->eof = 1;
	}
}

// void name(PlatformFileHandle *pfh, u64 offset)
PLATFORM_SEEK_FILE(nix_seek_file)
{
	Assert(pfh->open);
	FILE* fp = (FILE*) pfh->handle;
	int err = fseek(fp, (s64) offset, SEEK_SET);
	pfh->last_seek_success = !err;
}

// void name(PlatformFileHandle *pfh)
PLATFORM_CLOSE_FILE(nix_close_file)
{
	Assert(pfh->open);
	FILE* fp = (FILE*) pfh->handle;
	fclose(fp);
	pfh->open = 0;
}

// b8 name(PlatformFileHandle *pfh, char *begin, char* end)
PLATFORM_WRITE_TO_FILE(nix_write_to_file)
{
	Assert(pfh->write && pfh->open);
	FILE* fp = (FILE*) pfh->handle;
	size_t size = (size_t) (end-begin);
	size_t written = fwrite(begin, 1, size, fp);
	fflush(fp);
	return written == size;
}

PLATFORM_GET_TIME(nix_get_time)
{
	return (f64) time(0); // clock() / (f64) CLOCKS_PER_SEC;
}


PLATFORM_CREATE_MUTEX(nix_create_mutex)
{
	pthread_mutex_t *raw_mutex = (pthread_mutex_t*) malloc(sizeof(pthread_mutex_t));
	return (pt_Mutex) { .handle = raw_mutex };
}

PLATFORM_RELEASE_MUTEX(nix_release_mutex)
{
	pthread_mutex_t *raw_mutex = (pthread_mutex_t*) mutex.handle;
	free(raw_mutex);
}

PLATFORM_LOCK_MUTEX(nix_lock_mutex)
{
	pthread_mutex_lock((pthread_mutex_t*) mutex.handle);
}

PLATFORM_UNLOCK_MUTEX(nix_unlock_mutex)
{
	pthread_mutex_unlock((pthread_mutex_t*) mutex.handle);
}

internal
PLATFORM_CYCLE_COUNT_CPU(nix_cycle_count_cpu)
{
	// defined in #include "x86intrin.h"
	return (u64) __rdtsc();
}

//
// BEGIN executable_path
//
#if defined(OS_MAC)

// void name(FilePath *fp)
PLATFORM_EXECUTABLE_PATH(nix_executable_path)
{
	u32 buf_size = MAX_FILE_PATH_SIZE;
	_NSGetExecutablePath( fp->full_path, &buf_size);
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

#elif defined(OS_LINUX)

PLATFORM_EXECUTABLE_PATH(nix_executable_path)
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

#endif
//
// END executable_path
//

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
#if defined(OS_MAC)
	dispatch_semaphore_t semaphore_handle;
#elif defined(OS_LINUX)
	sem_t *semaphore_handle;
	sem_t semaphore_handle_storage;
#endif

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

	s32 err = 0;
#if defined(OS_MAC)
	self->semaphore_handle = dispatch_semaphore_create(0);
	err = self->semaphore_handle == 0;
#elif defined(OS_LINUX)
	err = sem_init(&self->semaphore_handle_storage, 0, 0);
	self->semaphore_handle = &self->semaphore_handle_storage;
	// self->semaphore_handle = sem_open("/workqueue", O_EXCL); // exclusive O_CREAT); // , 700, 0);
	// self->semaphore_handle = sem_open("/nanocube_workqueue", O_CREAT, 700, 0); // exclusive O_CREAT); // , 700, 0);
	// return self->semaphore_handle != SEM_FAILED;
#endif

	self->done        = 0;
	self->num_threads = 0;

	return err == 0;
}

// #define COMPILER_BARRIER() asm volatile("" ::: "memory")
// #define INTERLOCKED_COMPARE_EXCHANGE(a,b,c) asm volatile("" ::: "memory")

PLATFORM_WORK_QUEUE_ADD_ENTRY(nix_work_queue_add_entry)
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

#if defined(OS_MAC)
	dispatch_semaphore_signal(queue->semaphore_handle);
#elif defined(OS_LINUX)
	sem_post(queue->semaphore_handle);
#endif
	/* ReleaseSemaphore(queue->semaphore_handle, 1, 0); */
}

internal b8
nix_work_queue_work_on_next_entry(pt_WorkQueue *queue)
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
PLATFORM_WORK_QUEUE_COMPLETE_ALL_WORK(nix_work_queue_complete_all_work)
{
	while(queue->completion_goal != queue->completion_count) {
		nix_work_queue_work_on_next_entry(queue);
	}
	queue->completion_goal  = 0;
	queue->completion_count = 0;
}

internal
PLATFORM_WORK_QUEUE_FINISHED(nix_work_queue_finished)
{
	return queue->completion_goal == queue->completion_count;
}

PLATFORM_WORK_QUEUE_CALLBACK(nix_work_queue_thread_stop)
{
	pt_WorkQueue *queue2 = (pt_WorkQueue*) data;
	pt_atomic_sub_u32(&queue2->num_threads,1);
	pthread_exit(0);
}

global_variable pthread_key_t nix_thread_index_key;
global_variable b8 nix_app_state_interrupted;

/* should be called by the main thread */
internal void
nix_init()
{
	nix_app_state_interrupted = 0;
	pthread_key_create(&nix_thread_index_key,0);
}

internal void*
nix_work_queue_thread_run(void* data)
{
	pt_WorkQueue *queue = (pt_WorkQueue*) data;

	// increment
	u32 index = pt_atomic_add_u32(&queue->num_threads,1);

	pthread_setspecific(nix_thread_index_key, (void*) ((u64) index));

	// u32 TestThreadID = GetThreadID();
	// Assert(TestThreadID == GetCurrentThreadId());
	for(;;) {
		if(nix_work_queue_work_on_next_entry(queue)) {
#if defined(OS_MAC)
			dispatch_semaphore_wait(queue->semaphore_handle, DISPATCH_TIME_FOREVER);
#elif defined(OS_LINUX)
			sem_wait(queue->semaphore_handle);
#endif
			// WaitForSingleObjectEx(Queue->SemaphoreHandle, INFINITE, FALSE);
		}
	}
	return 0;
	// return(0);
}

//
// internal void*
// nix_thread_procedure(void* data)
// {
// 	nix_ThreadInfo *thread_info = (nix_ThreadInfo*) data;
// 	pt_WorkQueue *queue = thread_info->queue;
// 	// u32 TestThreadID = GetThreadID();
// 	// Assert(TestThreadID == GetCurrentThreadId());
// 	for(;;) {
// 		if(nix_WorkQueue_work_on_next_entry(queue)) {
// 			dispatch_semaphore_wait(queue->semaphore_handle, DISPATCH_TIME_FOREVER);
// 			// sem_wait(queue->semaphore_handle);
// 			// WaitForSingleObjectEx(Queue->SemaphoreHandle, INFINITE, FALSE);
// 		}
// 	}
// 	return 0;
// 	// return(0);
// }
//

internal
PLATFORM_WORK_QUEUE_CREATE(nix_work_queue_create)
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
		s32 error = pthread_create(&thread, 0, nix_work_queue_thread_run, queue);
		Assert(!error);
		pthread_detach(thread);
	}

	while (queue->num_threads != num_threads) {
		usleep(10);
	}

	return queue;
}

// internal b8
// nix_init_work_queue(pt_WorkQueue *queue, nix_ThreadInfo *thread_begin, u32 thread_count)
// {
// 	if (!pt_WorkQueue_init(queue, thread_count))
// 		return 0;
//
// 	for (u32 i=0;i<thread_count;++i) {
// 		nix_ThreadInfo *thread_info = thread_begin + i;
// 		thread_info->queue = queue;
//
// 		// 		pthread_attr_t attr;
// 		// 		pthread_attr_init(&attr);
// 		// 		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
// 		// 		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
//
// 		pthread_t thread;
//
// 		s32 error = pthread_create(&thread, 0, nix_thread_procedure, thread_info);
// 		Assert(!error);
// 		pthread_detach(thread);
// 	}
// 	return 1;
// }
//

internal
PLATFORM_GET_THREAD_INDEX(nix_get_thread_index)
{
	return (u64) pthread_getspecific(nix_thread_index_key);
}

internal
PLATFORM_THREAD_SLEEP(nix_thread_sleep)
{
	usleep(millisecs);
}

/* this should be called fro the same thread that called start */
internal
PLATFORM_WORK_QUEUE_DESTROY(nix_work_queue_destroy)
{
	if (!queue)
		return;

	Assert(!queue->done);
	queue->done = 1;

	// schedule exit of the threads
	s32 num_threads = queue->num_threads;

	__sync_synchronize();

	// schedule exit of the threads
	for (s32 i=0;i<num_threads;++i) {
		nix_work_queue_add_entry(queue, nix_work_queue_thread_stop, queue);
	}
	while (queue->num_threads > 0) {
		usleep(10);
	}

#if defined(OS_MAC)
	dispatch_release(queue->semaphore_handle);
#elif defined(OS_LINUX)
	sem_destroy(queue->semaphore_handle);
#endif
	free(queue);

}

/************* End Work Queue Support ************/

/************* Simple HTTP Server over TCP ************/
//
// Include a notion of a complete request. Use a subset notion of HTTP requests.
// HTTP-request-message = request-line *(header-field CRLF) CRLF [message-body]
// How to recover from unknown input?
//

// #define nix_tcp_LOCAL_ADDR          "127.0.0.1"
#define nix_tcp_LOCAL_ADDR          "0.0.0.0"
#define nix_tcp_PORT	              8888
#define nix_tcp_MAX_CONNECTIONS     64
#define nix_tcp_MAX_SOCKETS         256
#define nix_tcp_MAX_EVENTS	      64
#define nix_tcp_TIMEOUT             100

#define nix_tcp_Socket_FREE          0
#define nix_tcp_Socket_ACTIVATIING   1
#define nix_tcp_Socket_INUSE         2
#define nix_tcp_Socket_CLOSED        3

#define nix_tcp_BUFFER_SIZE 4096
#define nix_tcp_MAX_SERVERS 16

// struct nix_tcp_Connection {
// 	int    file_descriptor;
// 	int    status;
// 	s32    index;
// 	struct kevent ev;
// 	struct nix_tcp_Connection *prev;
// 	struct nix_tcp_Connection *next;
// 	// when retriggering a connection for more data
// 	nix_tcp_Server *nix_server;
// 	f64    start_time;
// };
// A listen socket is associated with a port from which new client
// connections can be established. A server socket is one that gets
// data from a client socket and pushed this data through the handler

// #define nix_tcp_LISTEN_SOCKET 1
// #define nix_tcp_CLIENT_SOCKET 2
// #define nix_tcp_SERVER_SOCKET 3

typedef struct nix_tcp_Engine	nix_tcp_Engine;
typedef struct nix_tcp_Socket nix_tcp_Socket;

struct nix_tcp_Socket {
	int    file_descriptor;
	s32    type;
	s32    index;

	// struct to register into event signaling of this socket
#if defined(OS_MAC)
	struct kevent ev;
#elif defined(OS_LINUX)
	struct epoll_event ev;
#endif

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
			struct nix_tcp_Socket *listen_socket;
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
	struct nix_tcp_Socket *prev;
	struct nix_tcp_Socket *next;
	nix_tcp_Engine *tcp;
};

#define nix_tcp_TASK_QUEUE_SIZE 256
#define nix_tcp_TASK_QUEUE_MASK 0xff

#define nix_tcp_LISTEN_TASK 0x10
#define nix_tcp_CLIENT_TASK 0x11

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
} nix_tcp_Task;

struct nix_tcp_Engine {
#if defined(OS_MAC)
	int kqueue_fd;
	struct kevent     events[nix_tcp_MAX_EVENTS];
#elif defined(OS_LINUX)
	int epoll_fd;
	struct epoll_event  events[nix_tcp_MAX_EVENTS];
#endif
	nix_tcp_Socket    sockets[nix_tcp_MAX_SOCKETS];
	nix_tcp_Socket    *free_sockets;
	/* some active_connections might have had its file descriptor closed */
	/* retrieve closed connections once a new connection is requested */
	nix_tcp_Socket    *active_sockets;
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
		nix_tcp_Task queue[nix_tcp_TASK_QUEUE_SIZE];
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

global_variable nix_tcp_Engine nix_tcp_engines[nix_tcp_MAX_SERVERS];
global_variable u32 nix_tcp_num_engines = 0;

// OS set filedescriptor to be non-blocking

internal void
nix_tcp_set_nonblocking_fd(int fd)
{
	int opts = fcntl(fd, F_GETFL);
	Assert(opts >= 0);
	opts = opts | O_NONBLOCK;
	int update_error = fcntl(fd, F_SETFL, opts);
	Assert(!update_error);
}

// nix_tcp_Socket

internal void
nix_tcp_Socket_init_empty(nix_tcp_Socket *self, nix_tcp_Engine *tcp, s32 index)
{
	//TODO(llins): revise this one
	pt_filln((char*) self, sizeof(nix_tcp_Socket), 0);
	self->status = nix_tcp_Socket_FREE; // status;
	self->tcp = tcp;
	self->index = index;
	self->file_descriptor = -1; // fd;

#if defined(OS_MAC)
	EV_SET(&self->ev, 0, 0, 0, 0, 0, 0);
#elif defined(OS_LINUX)
	self->ev.data.ptr = 0;
	self->ev.events = 0;
#endif

	self->prev = 0;
	self->next = 0;
	self->start_time = 0;
	self->user_data = 0;
}

// the kevent call is different than the epoll one,
// we always pass the list of events we are interested
// in a contiguous array
internal void
nix_tcp_Socket_kqueue_retrigger(nix_tcp_Socket *self)
{
#if defined(OS_MAC)
	kevent(self->tcp->kqueue_fd, &self->ev, 1, 0, 0, 0);
#elif defined(OS_LINUX)
	epoll_ctl(self->tcp->epoll_fd, EPOLL_CTL_MOD, self->file_descriptor, &self->ev);
#endif
}

internal void
nix_tcp_Socket_kqueue_insert(nix_tcp_Socket *self)
{
#if defined(OS_MAC)
	kevent(self->tcp->kqueue_fd, &self->ev, 1, 0, 0, 0);
#elif defined(OS_LINUX)
	epoll_ctl(self->tcp->epoll_fd, EPOLL_CTL_ADD, self->file_descriptor, &self->ev);
#endif
}

internal void
nix_tcp_Socket_init_listen(nix_tcp_Socket *self, int file_descriptor, s32 port, void *user_data, PlatformTCPCallback *callback, u64 start_time)
{
	self->file_descriptor = file_descriptor;
	self->type = pt_TCP_LISTEN_SOCKET;

#if defined(OS_MAC)
	//
	// kevent we are interested
	//
	//     intptr_t ident;         /* identifier for this event */
	//     short     filter;       /* filter for event */
	//     u_short   flags;        /* action flags for kqueue */
	//     u_int     fflags;       /* filter flag value */
	//     intptr_t  data;         /* filter data value */
	//     void      *udata;       /* opaque user data identifier */
	//
	EV_SET(&self->ev, file_descriptor, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, self);
#elif defined(OS_LINUX)
	self->ev.data.ptr = self;
	self->ev.events = EPOLLIN | EPOLLET;
	// edge triggered (not oneshot for the listen socket)
#endif

	self->status = nix_tcp_Socket_INUSE;
	self->listen.port = port;
	self->callback = callback;
	self->user_data = user_data;
	self->start_time = start_time;
}

internal void
nix_tcp_Socket_init_server(nix_tcp_Socket *self, nix_tcp_Socket *listen_socket, int file_descriptor, void *user_data, PlatformTCPCallback *callback, u64 start_time)
{
	Assert(self->status == nix_tcp_Socket_FREE);
	self->file_descriptor = file_descriptor;
	self->type = pt_TCP_SERVER_SOCKET;

#if defined(OS_MAC)
	EV_SET(&self->ev, file_descriptor, EVFILT_READ, EV_ADD | EV_ENABLE | EV_ONESHOT, 0, 0, self);
#elif defined(OS_LINUX)
	self->ev.data.ptr = self;
	self->ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
#endif

	self->server.listen_socket = listen_socket;
	self->status = nix_tcp_Socket_INUSE;
	self->callback = callback;
	self->user_data = user_data;
	self->start_time = start_time;
}

internal void
nix_tcp_Socket_init_client(nix_tcp_Socket *self, int file_descriptor, void *user_data, s32 port, char *hostname, PlatformTCPCallback *callback, u64 start_time)
{
	Assert(self->status == nix_tcp_Socket_FREE);
	self->file_descriptor = file_descriptor;
	self->type = pt_TCP_CLIENT_SOCKET;

#if defined(OS_MAC)
	EV_SET(&self->ev, file_descriptor, EVFILT_READ, EV_ADD | EV_ONESHOT, 0, 0, self);
#elif defined(OS_LINUX)
	self->ev.data.ptr = self;
	self->ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
#endif

	self->status = nix_tcp_Socket_INUSE;
	self->callback = callback;
	self->user_data = user_data;
	self->start_time = start_time;
}

internal void
nix_tcp_Socket_close(nix_tcp_Socket *self)
{
	//TODO(llins): revise this one
	Assert(self->status == nix_tcp_Socket_INUSE);
	self->status = nix_tcp_Socket_CLOSED;
}

// nix_tcp_Engine

internal void
nix_tcp_Engine_init(nix_tcp_Engine *self)
{

#if defined(OS_MAC)
	/* create kqueue object */
	self->kqueue_fd = kqueue();
	Assert(self->kqueue_fd);
#elif defined(OS_LINUX)
	/* create epoll object */
	self->epoll_fd = epoll_create(nix_tcp_MAX_EVENTS);
	Assert(self->epoll_fd);
#endif

	nix_tcp_Socket *prev = 0;
	for (s32 i=0;i<ArrayCount(self->sockets);++i) {
		nix_tcp_Socket_init_empty(self->sockets + i, self, i);
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
nix_tcp_Engine_destroy(nix_tcp_Engine *self)
{
#if defined(OS_MAC)
	Assert(self->kqueue_fd);
#elif defined(OS_LINUX)
	Assert(self->epoll_fd);
#endif

	nix_tcp_Socket *it = self->active_sockets;
	while (it) {
		if (it->status == nix_tcp_Socket_INUSE) {
			close(it->file_descriptor);
			printf("[tcp_destroy] socket: %d  type: %d  status: %d was INUSE and is being closed\n", it->index, it->type, it->status);
		} else if (it->status == nix_tcp_Socket_CLOSED) {
			printf("[tcp_destroy] socket: %d  type: %d  status: %d was already CLOSED\n", it->index, it->type, it->status);
		} else {
			printf("[tcp_destroy] socket: %d  type: %d  status: %d is either FREE or ACTIVATING (not closing it)\n", it->index, it->type, it->status);
		}
		it = it->next;
	}

#if defined(OS_MAC)
	close(self->kqueue_fd);
#elif defined(OS_LINUX)
	close(self->epoll_fd);
#endif

	char *p = (char*) self;
	pt_fill(p, p +sizeof(nix_tcp_Engine), 0);
}

internal void
nix_tcp_Engine_release_socket(nix_tcp_Engine *self, nix_tcp_Socket *socket)
{
	Assert(socket->status==nix_tcp_Socket_INUSE || socket->status==nix_tcp_Socket_CLOSED);
	nix_tcp_Socket *prev = socket->prev;
	nix_tcp_Socket *next = socket->next;
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
	socket->status = nix_tcp_Socket_FREE;
}

internal void
nix_tcp_Engine_collect_closed_sockets(nix_tcp_Engine *self)
{
	nix_tcp_Socket *it = self->active_sockets;
	s32 count = 0;
	while (it) {
		nix_tcp_Socket *it_next = it->next;
		if (it->status == nix_tcp_Socket_CLOSED) {
			nix_tcp_Engine_release_socket(self, it);
			++count;
		}
		it = it_next;
	}
	printf("[SERVER] Collect Closed Socket %d\n", count);
}

internal nix_tcp_Socket*
nix_tcp_Engine_reserve_socket(nix_tcp_Engine *self)
{
	if (!self->free_sockets) {
		/* collect closed sockets from active sockets if any */
		nix_tcp_Engine_collect_closed_sockets(self);
	}
	Assert(self->free_sockets);
	nix_tcp_Socket *new_socket = self->free_sockets;
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
nix_tcp_Engine_produce_listen_task(nix_tcp_Engine *self, s32 port, void *user_data, PlatformTCPCallback *callback, pt_TCP_Feedback *feedback)
{
	// simply let it overflow
	u64 index = pt_atomic_add_u64(&self->tasks.end, 1);
	index &= nix_tcp_TASK_QUEUE_MASK;
	//
	// NOTE(llins): assumes we don't produce more tasks than the
	// queue size in a single cycle.
	//
	Assert(index != self->tasks.sentinel);
	nix_tcp_Task *task = self->tasks.queue + index;
	Assert(task->ready == 0);
	task->callback = callback;
	task->feedback = feedback;
	task->port = port;
	task->type = nix_tcp_LISTEN_TASK;
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
nix_tcp_Engine_produce_client_task(nix_tcp_Engine *self, s32 port, char *hostname, void *user_data, PlatformTCPCallback *callback, pt_TCP_Feedback *feedback)
{
	// simply let it overflow
	u64 index = pt_atomic_add_u64(&self->tasks.end, 1);
	index &= nix_tcp_TASK_QUEUE_MASK;
	//
	// NOTE(llins): assumes we don't produce more tasks than the
	// queue size in a single cycle.
	//
	Assert(index != self->tasks.sentinel);
	nix_tcp_Task *task = self->tasks.queue + index;
	Assert(task->ready == 0);
	task->callback = callback;
	task->client.hostname = hostname;
	task->feedback = feedback;
	task->port = port;
	task->type = nix_tcp_CLIENT_TASK;
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
nix_tcp_Engine_consume_listen_task(nix_tcp_Engine *self, nix_tcp_Task *task)
{
	Assert(task->type == nix_tcp_LISTEN_TASK);
	s32 next_status = pt_TCP_SOCKET_OK;
	nix_tcp_Socket *listen_socket = 0;
	/* create self server socket */
	int listen_socket_fd = socket(PF_INET, SOCK_STREAM, 0);
	if (listen_socket_fd < 0) {
		printf("pt_Engine_ERROR_SOCKET\n");
		goto error;
	}
	/* set as non-blocking */
	nix_tcp_set_nonblocking_fd(listen_socket_fd);
	struct sockaddr_in listen_socket_addr;
	listen_socket_addr.sin_family = AF_INET;
	listen_socket_addr.sin_port = htons(task->port);
	inet_aton(nix_tcp_LOCAL_ADDR, &(listen_socket_addr.sin_addr));
	int bind_error = bind(listen_socket_fd, (struct sockaddr*) &listen_socket_addr, sizeof(listen_socket_addr));
	if (bind_error) {
		printf("pt_Engine_ERROR_BIND\n");
		goto error;
	}
	/* listen */
	int listen_error = listen(listen_socket_fd, nix_tcp_MAX_CONNECTIONS);
	if (listen_error) {
		printf("pt_Engine_ERROR_LISTEN\n");
		goto error;
	}
	/* initialize listen socket report */
	f64 start_time = nix_get_time();
	listen_socket = nix_tcp_Engine_reserve_socket(self);
	if (!listen_socket) {
		printf("[nix_tcp_Engine_consume_tasks]: could not reserver socket\n");
		Assert(listen_socket_fd);
	}
	nix_tcp_Socket_init_listen(listen_socket, listen_socket_fd, task->port, task->user_data, task->callback, start_time);
	/* register into kqueue */
	nix_tcp_Socket_kqueue_insert(listen_socket);

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

internal void
nix_tcp_Engine_consume_client_task(nix_tcp_Engine *self, nix_tcp_Task *task)
{
	// TODO(llins): continue from here
	Assert(task->type == nix_tcp_CLIENT_TASK);
	s32 next_status = pt_TCP_SOCKET_OK;
	// TODO(llins): which one is it? a hostname or an ip address?
	// assume hostname

	int client_socket_fd = -1;
	f64 start_time = nix_get_time();

	// http://beej.us/guide/bgnet/output/html/singlepage/bgnet.html#getaddrinfo
	struct addrinfo hints = { 0 };
	hints.ai_family   = AF_UNSPEC;   // don't care IPV4 or IPV6
	hints.ai_socktype = SOCK_STREAM; // TCP stream socket
	hints.ai_flags    = AI_PASSIVE;  // fill in my IP for me
	char port_string[15];
	snprintf(port_string,15,"%d",task->port);
	struct addrinfo *servinfo;
	nix_tcp_Socket *client_socket = 0;

	int status = getaddrinfo(task->client.hostname, port_string, &hints, &servinfo);
	if (status != 0) {
		printf("[nix_tcp_Engine_consume_client_task] Error: getaddrinfo()\n");
		printf("     %s\n", gai_strerror(status));
		goto error;
	}
	s32 option = 0;
	for (struct addrinfo *p=servinfo; p!=0; p=p->ai_next) {
		++option;
		client_socket_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (client_socket_fd >= 0) {
			int connect_status = connect(client_socket_fd, p->ai_addr, p->ai_addrlen);
			if (connect_status != 0) {
				printf("[nix_tcp_Engine_consume_client_task] Could not connect(...) using addrinfo %d: connect(...)\n", option);
				close(client_socket_fd);
			} else {
				goto connect_ok;
			}
		} else {
			printf("[nix_tcp_Engine_consume_client_task] Could not socket(...) using addrinfo %d: connect(...)\n", option);
		}
	}
	goto error;

connect_ok:
	/* set client to be nonblocking */
	nix_tcp_set_nonblocking_fd(client_socket_fd);

	/* register client socket fd into the kqueue */
	client_socket = nix_tcp_Engine_reserve_socket(self);
	if (!client_socket) {
		printf("[nix_tcp_Engine_consume_client_task]: could not reserve socket\n");
		goto error;
	}
	// TODO(llins): register hostname and port on the client socket?
	nix_tcp_Socket_init_client(client_socket, client_socket_fd, task->user_data,
				   task->port, task->client.hostname, task->callback,
				   start_time);
	/* register into kqueue */
	nix_tcp_Socket_kqueue_insert(client_socket);
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
nix_tcp_Engine_consume_tasks(nix_tcp_Engine *self)
{
	// how to safely check if task queue is full
	u64 it  = (self->tasks.sentinel + 1) & nix_tcp_TASK_QUEUE_MASK;
	u64 end = self->tasks.end            & nix_tcp_TASK_QUEUE_MASK;
	while (it != end) {
		nix_tcp_Task *task = self->tasks.queue + it;
		if (task->ready) {
			/* listen task */
			if (task->type == nix_tcp_LISTEN_TASK) {
				nix_tcp_Engine_consume_listen_task(self, task);
			} else if (task->type == nix_tcp_CLIENT_TASK) {
				nix_tcp_Engine_consume_client_task(self, task);
			} else {
				printf("[nix_tcp_Engine_consume_tasks] task type not supported\n");
				Assert(0);
			}
		} else {
			// if i-th task is not ready leave remaining
			// tasks to next engine cycle
			break;
		}
		it = (it + 1) & nix_tcp_TASK_QUEUE_MASK;
	}
	if (it == 0) {
		self->tasks.sentinel = nix_tcp_TASK_QUEUE_SIZE - 1;
	} else {
		self->tasks.sentinel = it - 1;
	}
}

internal void
nix_tcp_read_from_connection_and_dispatch(nix_tcp_Socket *socket)
{
	char buf[nix_tcp_BUFFER_SIZE];
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
		f64 end_time = nix_get_time();
		printf("[SERVER] Closing Socket %d:%d on (active time: %fs)\n", socket->index, socket->file_descriptor, end_time - socket->start_time);
		// printf ("Closed socket on descriptor %d\n",events[i].data.fd);
		/* Closing the descriptor will make kqueue remove it
		   from the set of descriptors which are monitored. */
		close(socket->file_descriptor);
		nix_tcp_Socket_close(socket);
	} else {
		/* NOTE(llins): from what I read, system call here is thread safe */
		printf("[SERVER] Retriggering Socket %d:%d\n", socket->index, socket->file_descriptor);
		nix_tcp_Socket_kqueue_retrigger(socket);
	}
}

internal
PLATFORM_WORK_QUEUE_CALLBACK(nix_tcp_work_queue_callback)
{
	nix_tcp_Socket *connection = (nix_tcp_Socket*) data;
	nix_tcp_read_from_connection_and_dispatch(connection);
}

internal void
nix_tcp_Engine_process_events(nix_tcp_Engine *tcp, pt_WorkQueue *work_queue)
{
	/* register listen and client sockets coming as registered tasks */
	nix_tcp_Engine_consume_tasks(tcp);
	/* wait for events on sockets beign tracked for at most nix_tcp_TIMEOUT */
#if defined(OS_MAC)
	static const struct timespec timeout = { .tv_sec= 0, .tv_nsec = nix_tcp_TIMEOUT };
	int num_fd = kevent(tcp->kqueue_fd, 0, 0, tcp->events, ArrayCount(tcp->events), &timeout);
	struct kevent ev;
#elif defined(OS_LINUX)
	int num_fd = epoll_wait(tcp->epoll_fd, tcp->events, ArrayCount(tcp->events), nix_tcp_TIMEOUT);
#endif

	/* for each socket with some event available process all what
	 * is available (edge triggered)
	 *
	 * for events on listen sockets:
	 *     use the same thread to register new listen sockets to kqueue
	 *
	 * for events on client or server sockets:
	 *     use the same thread to run the receive callbacks if work_queue is null
	 *     dispatch callbacks to work queue
	 */
	for (int i=0; i<num_fd; ++i) {

#if defined(OS_MAC)
		nix_tcp_Socket *socket = (nix_tcp_Socket*) tcp->events[i].udata;
#elif defined(OS_LINUX)
		nix_tcp_Socket *socket = (nix_tcp_Socket*) tcp->events[i].data.ptr;
#endif

		if (socket->type == pt_TCP_LISTEN_SOCKET) {
			pf_BEGIN_BLOCK("accepting_connection");
			// try accepetping one or more incoming connections
			while (1) {
				struct sockaddr_in new_socket_addr;
				socklen_t new_socket_len = sizeof(struct sockaddr_in);
				int new_socket_fd = accept(socket->file_descriptor, (struct sockaddr*) &new_socket_addr, &new_socket_len);
				if (new_socket_fd == -1) {
					if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
						break;
					} else {
						printf("[SERVER] Fatal Error: problem when accepting socket\n");
						Assert(0 && "problem when accepting socket");
					}
				}
				Assert(new_socket_fd >= 0);
				nix_tcp_set_nonblocking_fd(new_socket_fd);
				nix_tcp_Socket *new_socket = nix_tcp_Engine_reserve_socket(tcp);
				f64 start_time = nix_get_time();
				printf("[SERVER] New Socket %d:%d from %s:%d\n",
				       new_socket->index,
				       new_socket_fd,
				       inet_ntoa(new_socket_addr.sin_addr),
				       (int) new_socket_addr.sin_port);
				nix_tcp_Socket_init_server(new_socket, socket, new_socket_fd, socket->user_data, socket->callback, start_time);
				nix_tcp_Socket_kqueue_insert(new_socket);
			}

			pf_END_BLOCK();

		} else {
			if (work_queue) {
				/* work threads */
				nix_work_queue_add_entry(work_queue, nix_tcp_work_queue_callback, socket);
			} else {
				/* single threaded */
				nix_tcp_read_from_connection_and_dispatch(socket);
			}
		}
	}
}

internal
PLATFORM_TCP_PROCESS_EVENTS(nix_tcp_process_events)
{
	nix_tcp_Engine_process_events((nix_tcp_Engine*) tcp.handle, work_queue);
}

internal
PLATFORM_TCP_WRITE(nix_tcp_write)
{
	if (length == 0)
		return;
	nix_tcp_Socket *nix_socket = (nix_tcp_Socket*) socket->handle;
	s64 offset = 0;
	while (1) {
		// https://stackoverflow.com/questions/108183/how-to-prevent-sigpipes-or-handle-them-properly
		// s64 count = write(nix_socket->file_descriptor, buffer + offset, length - offset);
		// MSG_NOSIGPIPE
#if defined(OS_MAC)
		s64 count = send(nix_socket->file_descriptor, buffer + offset, length - offset, SO_NOSIGPIPE);
#elif defined(OS_LINUX)
		s64 count = send(nix_socket->file_descriptor, buffer + offset, length - offset, MSG_NOSIGNAL );
#endif

		if (count == -1) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				// printf("[SERVER] received EAGAIN || EWOULDBLOCK with -1 result, will try again...\n");
				// usleep(1);
				continue;
			} else {
				printf("[SERVER] write count == -1 on socket %d:%d errorno %d is %s (exiting server respond)\n",
				       nix_socket->index, nix_socket->file_descriptor, errno,
				       strerror(errno));
				return;
			}
		} else if (count == 0) {
			printf("[SERVER] write count == 0 on socket %d:%d errorno %d is %s\n",
			       nix_socket->index, nix_socket->file_descriptor, errno,
			       strerror(errno));
			// TODO(llins): revise these cases. not sure what is the right approach.
			return;
		} else if (count > 0) {
			offset += count;
			if (offset == length) {
				printf("[SERVER] written %d bytes on socket (response done %d) %d:%d\n", (int) count,
				       (int) length, nix_socket->index, nix_socket->file_descriptor);
				break;
			} else {
				printf("[SERVER] written %d bytes on socket %d:%d\n", (int) count,
				       nix_socket->index, nix_socket->file_descriptor);
			}
		}
	}
}

internal
PLATFORM_TCP_CREATE(nix_tcp_create)
{
	pt_TCP result = { .handle = 0 };
	if (nix_tcp_num_engines == ArrayCount(nix_tcp_engines)) {
		return result;
	}

	/* osx server */
	nix_tcp_Engine *nix_tcp_engine = nix_tcp_engines + nix_tcp_num_engines;

	nix_tcp_Engine_init(nix_tcp_engine);

	/* osx server */
	++nix_tcp_num_engines;
	result.handle = (void*) nix_tcp_engine;
	return result;
}

internal
PLATFORM_TCP_DESTROY(nix_tcp_destroy)
{
	if (!tcp.handle) {
		return;
	}
	nix_tcp_Engine *tcp_engine = (nix_tcp_Engine*) tcp.handle;
	nix_tcp_Engine_destroy(tcp_engine);
}

// #define PLATFORM_TCP_SERVE(name) void name(pt_TCP *tcp, s32 port, void *user_data, PlatformTCPCallback *callback, s32 *status)
internal
PLATFORM_TCP_SERVE(nix_tcp_serve)
{
	nix_tcp_Engine *nix_tcp_engine = (nix_tcp_Engine*) tcp.handle;
	/* schedule tcp engine to create a listen socket on next process events cycle */
	nix_tcp_Engine_produce_listen_task(nix_tcp_engine, port, user_data, callback, feedback);
}

// #define PLATFORM_TCP_CLIENT(name) void name(pt_TCP *tcp, s32 port, char *hostname, void *user_data, PlatformTCPCallback *callback, s32 *status)
internal
PLATFORM_TCP_CLIENT(nix_tcp_client)
{
	nix_tcp_Engine *nix_tcp_engine = (nix_tcp_Engine*) tcp.handle;
	/* schedule tcp engine to create a client socket on next process events cycle */
	/* client socket should connect to host referred on hostname */
	// TODO(llins): should we copy hostname to a safe place? (client code might destroy it)
	nix_tcp_Engine_produce_client_task(nix_tcp_engine, port, hostname, user_data, callback, feedback);
}

// initialize osx platform to PlatformAPI

internal void
nix_init_platform(PlatformAPI* p)
{
	p->allocate_memory          = nix_allocate_memory;
	p->resize_memory            = nix_resize_memory;
	p->free_memory              = nix_free_memory;
	p->copy_memory              = nix_copy_memory;
	p->open_write_file          = nix_open_write_file;
	p->open_read_file           = nix_open_read_file;
	p->read_next_file_chunk     = nix_read_next_file_chunk;
	p->seek_file                = nix_seek_file;
	p->close_file               = nix_close_file;
	p->write_to_file            = nix_write_to_file;
	p->get_time                 = nix_get_time;
	p->open_mmap_file           = nix_open_mmap_file;
	p->close_mmap_file          = nix_close_mmap_file;
	p->resize_file              = nix_resize_file;
	p->cycle_count_cpu          = nix_cycle_count_cpu;

	// mutex
	p->create_mutex             = nix_create_mutex;
	p->release_mutex            = nix_release_mutex;
	p->lock_mutex               = nix_lock_mutex;
	p->unlock_mutex             = nix_unlock_mutex;

	p->executable_path          = nix_executable_path;
	p->work_queue_add_entry     = nix_work_queue_add_entry;
	p->work_queue_complete_work = nix_work_queue_complete_all_work;
	p->work_queue_create        = nix_work_queue_create;
	p->work_queue_destroy       = nix_work_queue_destroy;
	p->work_queue_finished      = nix_work_queue_finished;
	p->get_thread_index         = nix_get_thread_index;
	p->thread_sleep             = nix_thread_sleep;

	/* tcp support is not implemented on OSX */
	p->tcp_create               = nix_tcp_create;
	p->tcp_serve                = nix_tcp_serve;
	p->tcp_write                = nix_tcp_write;
	p->tcp_process_events       = nix_tcp_process_events;
	p->tcp_client               = nix_tcp_client;
}
