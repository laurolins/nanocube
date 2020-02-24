//
// @todo
//
// - (DONE 2018-09-10T09:14) add a callback mechanism to trigger when
//   a server socket is created and destroyed. The goal is to use this
//   mechanism to let the user associate the server socket with
//   custom data (eg. http state machine).
//
//   idea: a simpler version could be a fresh bit indicating that the
//   server socket never received any data; this would solve the
//   creation of attaching custom data: the first time data is received
//   in the socket (client could detect and install a new custom
//   data), but the problem would be to reclaim the resource. A
//   server socket when destroyed would need to signal that the
//   custom resource is not needed. Maybe it is simpler to keep the
//   on_server_socket_creation and on_server_socket_destruction
//   events.
//
// - Assign unique number to a connection using atomic increments?
//
// - Independent closing of RD and WR channels of a socket. Keep
//   socket still alive if one direction is still open
//

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
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <dirent.h> // to get the filenames in directory
#include <fnmatch.h> // to get the filenames in directory

#include <sys/types.h>
#include <sys/socket.h> // socket()
#include <netdb.h>

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

typedef struct nix_Memory nix_Memory;

struct nix_Memory {
	pt_Memory block;
	nix_Memory *prev;
	nix_Memory *next;
};

// every malloc/free
static nix_Memory       *nix_entry_point_to_allocated_blocks = 0;
static pthread_mutex_t   nix_entry_point_to_allocated_blocks_mutex;

#define nix_PAGE_SIZE 4096

PLATFORM_TOTAL_ALLOCATED_MEMORY(nix_total_allocated_memory)
{
	// inefficient call
	pthread_mutex_lock(&nix_entry_point_to_allocated_blocks_mutex);
	u64 total = 0;
	nix_Memory *it = nix_entry_point_to_allocated_blocks;
	while (it != 0) {
		total += it->block.size;
		it = it->next;
	}
	pthread_mutex_unlock(&nix_entry_point_to_allocated_blocks_mutex);
	return total;
}

//
// @todo when out of memory, let the caller know about it instead of crashing
//
PLATFORM_ALLOCATE_MEMORY(nix_allocate_memory)
{
	b8 underflow_check = flags & pt_Memory_FLAG_CHECK_UNDERFLOW;
	b8 overflow_check  = flags & pt_Memory_FLAG_CHECK_OVERFLOW;

	// overflow and underflow cannot both be on
	Assert(!(underflow_check && overflow_check));

	// size request must be a multiple of page size
	// Assert((size % nix_PAGE_SIZE) == 0);

	u64 size_page_aligned = RAlign(size, nix_PAGE_SIZE);

	u64 extra_pages = 1;
	u64 offset = nix_PAGE_SIZE;
	if (underflow_check) {
		extra_pages = 2;
		offset      = 2 * nix_PAGE_SIZE;
	} else if (overflow_check) {
		extra_pages = 2;
		// right align so that we are at the boundary of a protected
		// page as soon as an extra byte is written there
		offset      = nix_PAGE_SIZE + size_page_aligned - size;
	}

	u64 total_size  = size_page_aligned + extra_pages * nix_PAGE_SIZE;
	void *platform_base = 0;
	s32 status = posix_memalign(&platform_base, nix_PAGE_SIZE, total_size);
	Assert(status == 0 && "Out of Memory");

	nix_Memory *new_block = (nix_Memory*) platform_base;
	pt_Memory  *result    = &new_block->block; // should be the same adress as new_block

	result->base  = (u8*) platform_base + offset;
	result->size  = size;
	result->used  = 0;
	result->flags = flags;
	result->prev  = 0;

	// protect pages
	if (flags & pt_Memory_FLAG_CHECK_UNDERFLOW) {
		s32 status = mprotect( (char*) platform_base + 1, nix_PAGE_SIZE, PROT_NONE);
		Assert(status == 0);
	}
	if (flags & pt_Memory_FLAG_CHECK_OVERFLOW) {
		s32 status = mprotect( result->base + result->size, nix_PAGE_SIZE, PROT_NONE);
		Assert(status == 0);
	}

	//
	// @todo(llins) needs to lock a mutex before touching/updating the
	// entry point of nix allocated memory blocks
	//
	pthread_mutex_lock(&nix_entry_point_to_allocated_blocks_mutex);

	new_block->next = nix_entry_point_to_allocated_blocks;
	new_block->prev = 0;
	if (nix_entry_point_to_allocated_blocks) {
		Assert(nix_entry_point_to_allocated_blocks->prev == 0);
		nix_entry_point_to_allocated_blocks->prev = new_block;
	}
	nix_entry_point_to_allocated_blocks = new_block;

	pthread_mutex_unlock(&nix_entry_point_to_allocated_blocks_mutex);

	return result;
};

PLATFORM_ALLOCATE_MEMORY_RAW(nix_allocate_memory_raw)
{
	pt_Memory* mem = nix_allocate_memory(size, flags);
	if (mem) {
		return mem->base;
	} else {
		return 0;
	}
};

/* b8 name(pt_Memory* mem, u64 new_size, u8 alignment) */
PLATFORM_RESIZE_MEMORY(nix_resize_memory)
{
	// dead end for now
	InvalidCodePath;
	return 0;
#if 0
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
		char *it_dst = (char*) RAlign((u64) new_handle, stride);
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
#endif
};

PLATFORM_FREE_MEMORY(nix_free_memory)
{
	nix_Memory *block = (nix_Memory*) memory;

	pthread_mutex_lock(&nix_entry_point_to_allocated_blocks_mutex);

	// mutex lock this section
	if (block->prev == 0) {
		Assert(nix_entry_point_to_allocated_blocks == block);
		nix_entry_point_to_allocated_blocks = nix_entry_point_to_allocated_blocks->next;
		if (nix_entry_point_to_allocated_blocks) {
			Assert(nix_entry_point_to_allocated_blocks->prev == block);
			nix_entry_point_to_allocated_blocks->prev = 0;
		}
	} else {
		nix_Memory *prev = block->prev;
		nix_Memory *next = block->next;
		prev->next = next;
		if (next) {
			next->prev = prev;
		}
	}

	pthread_mutex_unlock(&nix_entry_point_to_allocated_blocks_mutex);

	// free the block
	free(block);
}

PLATFORM_FREE_MEMORY_RAW(nix_free_memory_raw)
{
	pt_Memory* mem = (pt_Memory*) (LAlign((u64) raw,nix_PAGE_SIZE) - nix_PAGE_SIZE);
	nix_free_memory(mem);
};

static void
nix_free_all_memory()
{
	while (nix_entry_point_to_allocated_blocks) {
		nix_free_memory((pt_Memory*) nix_entry_point_to_allocated_blocks);
	}
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

PLATFORM_OPEN_FILE(nix_open_file)
{
	// big trouble if filename is larger than 1023 characters
	char filename[1024];
	s32 len = (s32) (file_end - file_begin);
	Assert(len + 1 <= sizeof(filename));
	platform.memory_copy(filename, (char*) file_begin, len);
	filename[len] = 0;

	pt_File result = { 0 };

	if (mode == pt_FILE_READ) {

		FILE *fp = fopen(filename, "rb");

		result = (pt_File) {
			.open = fp ? 1 : 0,
			.eof  = 0,
			.last_read = 0,
			.last_seek_success = 0,
			.handle = fp,
			.read = 1,
			.write = 0
		};

		if (fp) {
			fseek(fp, 0L, SEEK_END);
			result.size = ftell(fp);
			fseek(fp, 0L, SEEK_SET);
		} else {
			result.size = 0;
		}

	} else if (mode == pt_FILE_WRITE || mode == pt_FILE_APPEND) {
		char *mode_st = (mode == pt_FILE_WRITE) ? "wb" : "ab";

		FILE *fp = fopen(filename, mode_st);

		result = (pt_File) {
			.open = fp ? 1 : 0,
			.eof  = 0,
			.last_read = 0,
			.last_seek_success = 0,
			.handle = fp,
			.read = 0,
			.write = 1,
			.size = 0
		};

	} else {
		InvalidCodePath;
	}

	return result;
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
	s32 err = fseek(fp, (s64) offset, SEEK_SET);
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

static
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
	s32 failed = _NSGetExecutablePath(buffer, length);
	if (!failed) {
		*length = cstr_length(buffer);
	}
	return !failed;
//
//
// 	fp->end  = fp->full_path + buf_size;
// 	fp->name = fp->full_path;
// 	for(char *it=fp->full_path;*it;++it)
// 	{
// 		if(*it == '/')
// 		{
// 			fp->name = it + 1;
// 		}
// 	}
}

#elif defined(OS_LINUX)

PLATFORM_EXECUTABLE_PATH(nix_executable_path)
{
	char path[Kilobytes(1)];
	pid_t pid = getpid();
	sprintf(path, "/proc/%d/exe", pid);
// If successful, when bufsiz is greater than 0, readlink() returns the number of
// bytes placed in the buffer. When bufsiz is 0 and readlink() completes
// successfully, it returns the number of bytes contained in the symbolic link and
// the buffer is not changed.
//
// If the returned value is equal to bufsiz, you can determine the contents of the
// symbolic link with either lstat() or readlink(), with a 0 value for bufsiz.
//
// If unsuccessful, readlink() returns -1 and sets errno to one of the following
// values: ...
	ssize_t buf_size = readlink(path, buffer, *length);
	Assert(buf_size > 0);
	*length = buf_size;
	return 1;
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

static b8
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

static b8
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

static
PLATFORM_WORK_QUEUE_COMPLETE_ALL_WORK(nix_work_queue_complete_all_work)
{
	while(queue->completion_goal != queue->completion_count) {
		nix_work_queue_work_on_next_entry(queue);
	}
	queue->completion_goal  = 0;
	queue->completion_count = 0;
}

static
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

typedef union {
	u64  index;
	void *p;
} nix_QueueIndex;

/* should be called by the main thread */
static void
nix_init()
{
	nix_app_state_interrupted = 0;
	pthread_key_create(&nix_thread_index_key,0);

	// make the calling thread index information to be zero
	nix_QueueIndex idx = { 0 };
	pthread_setspecific(nix_thread_index_key, idx.p);
}

static void*
nix_work_queue_thread_run(void* data)
{
	pt_WorkQueue *queue = (pt_WorkQueue*) data;

	// increment
	nix_QueueIndex idx = { 0 };
	idx.index = pt_atomic_add_u32(&queue->num_threads,1);

	pthread_setspecific(nix_thread_index_key, idx.p);

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
// static void*
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

static
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

// static b8
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

static
PLATFORM_GET_THREAD_INDEX(nix_get_thread_index)
{
	return (u64) pthread_getspecific(nix_thread_index_key);
}

static
PLATFORM_THREAD_SLEEP(nix_thread_sleep)
{
	usleep(millisecs);
}

/* this should be called fro the same thread that called start */
static
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


#ifndef nix_tcp_DEBUG_LEVEL
#define nix_tcp_DEBUG_LEVEL 1
#endif

static char *nix_tcp_SOCKET_TYPE_NAME[] = {
	"UNDEFINED",
	"LISTEN",
	"CLIENT",
	"SERVER"
};

static char *nix_tcp_SOCKET_TYPE_INITIAL[] = {
	"u",
	"l",
	"c",
	"s"
};


// socket format
// socket print
#define nix_tcp_SF "%s%"PRIu64" (%d:%d) "
#define nix_tcp_SP(socket) nix_tcp_SOCKET_TYPE_INITIAL[socket->type], socket->id, (s32)socket->index, (s32)socket->file_descriptor

//
// let level 1 be only connection creation/destruction
//
#if nix_tcp_DEBUG_LEVEL > 0
#define nix_tcp_log(format, ...) fprintf(stderr, "[%s] " format, __FUNCTION__, ##  __VA_ARGS__)
#define nix_tcp_log_socket(socket, format, ...) fprintf(stderr, "[%s] " nix_tcp_SF format, __FUNCTION__, nix_tcp_SP(socket), ##  __VA_ARGS__)
#else
#define nix_tcp_log(format, ...)
#define nix_tcp_log_socket(socket, format, ...)
#endif

#if nix_tcp_DEBUG_LEVEL > 1
#define nix_tcp_log2(format, ...) fprintf(stderr, "[%s] " format, __FUNCTION__, ##  __VA_ARGS__)
#define nix_tcp_log_socket2(socket, format, ...) fprintf(stderr, "[%s] " nix_tcp_SF format, __FUNCTION__, nix_tcp_SP(socket), ##  __VA_ARGS__)
#else
#define nix_tcp_log2(format, ...)
#define nix_tcp_log_socket2(socket, format, ...)
#endif

#if nix_tcp_DEBUG_LEVEL > 2
#define nix_tcp_log3(format, ...) fprintf(stderr, "[%s] " format, __FUNCTION__, ##  __VA_ARGS__)
#define nix_tcp_log_socket3(socket, format, ...) fprintf(stderr, "[%s] " nix_tcp_SF format, __FUNCTION__, nix_tcp_SP(socket), ##  __VA_ARGS__)
#else
#define nix_tcp_log3(format, ...)
#define nix_tcp_log_socket3(socket, format, ...)
#endif

//
// Include a notion of a complete request. Use a subset notion of HTTP requests.
// HTTP-request-message = request-line *(header-field CRLF) CRLF [message-body]
// How to recover from unknown input?
//

// #define nix_tcp_LOCAL_ADDR          "127.0.0.1"
#define nix_tcp_LOCAL_ADDR          "0.0.0.0"
#define nix_tcp_PORT	            8888
#define nix_tcp_MAX_CONNECTIONS     64
#define nix_tcp_MAX_SOCKETS         256
#define nix_tcp_MAX_EVENTS	    64

#define nix_tcp_TIMEOUT_MILLISECONDS  0
#define nix_tcp_TIMEOUT_SECONDS       1

// #define nix_tcp_Socket_FREE          0
// #define nix_tcp_Socket_ACTIVATIING   1
// #define nix_tcp_Socket_INUSE         2
// #define nix_tcp_Socket_CLOSED        3

#define nix_tcp_BUFFER_SIZE Kilobytes(64)
#define nix_tcp_MAX_SERVERS 16

//
// let the status be either 0, 1, 2, or 3
//
// #define nix_tcp_Socket_FREE     0
// #define pt_TCP_READ   0x1
// #define pt_TCP_WRITE  0x2
// #define pt_TCP_READ_WRITE (pt_TCP_READ + pt_TCP_WRITE)

// struct nix_tcp_Connection {
// 	s32    file_descriptor;
// 	s32    status;
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

typedef struct nix_tcp_Engine nix_tcp_Engine;
typedef struct nix_tcp_Socket nix_tcp_Socket;

// #define pt_TCP_LISTEN_SOCKET 1
// #define pt_TCP_CLIENT_SOCKET 2
// #define pt_TCP_SERVER_SOCKET 3

struct nix_tcp_Socket {
	u64  id; // unique ID throughout the whole history of a tcp engine
	s32  file_descriptor;
	s32  type;
	s32  index;
	s32  status; // RD and WR or 0

	// struct to register into event signaling of this socket
#if defined(OS_MAC)
	struct kevent ev;
#elif defined(OS_LINUX)
	struct epoll_event ev;
#endif

	union {
		struct {
			s32 port;
			volatile u16 num_connections;
			u16 max_connections;
			PlatformTCPEventCallback *event_callback;
			void *context_event_callback;
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
	PlatformTCPDataCallback *data_callback;
	f64    start_time;
	void   *user_data;

	// which socket are we talking about
	struct nix_tcp_Socket *prev;
	struct nix_tcp_Socket *next;

	// socket has a back pointer to the engine
	nix_tcp_Engine *tcp;
};

#define nix_tcp_TASK_QUEUE_SIZE 256
#define nix_tcp_TASK_QUEUE_MASK 0xff

#define nix_tcp_LISTEN_TASK 0x10
#define nix_tcp_CLIENT_TASK 0x11

//
// make the main thread be responsible for closing
// the connections. we can make the connection
// immediately inactive by setting its status to zero
// but let the file descriptor and stuff be closed on
// the main processing of events
//
#define nix_tcp_CLOSE_TASK 0x12

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
			// NOTE(llins): it is simple to add a limit to
			// number of connections per port
			PlatformTCPEventCallback *event_callback;
			void *context_event_callback;
			u16 max_connections;
		} listen;
		struct {
			char *hostname;
		} client;
		struct {
			nix_tcp_Socket *socket;
			s32 type;
			u64 id;
		} close;
	};
	// use the pointer below when scheduling tasks
	// to indicate what happened
	s32  port;
	PlatformTCPDataCallback *data_callback;
	void *user_data;
	pt_TCP_Feedback *feedback;
} nix_tcp_Task;


struct nix_tcp_Engine {
#if defined(OS_MAC)
	s32 kqueue_fd;
	struct kevent     events[nix_tcp_MAX_EVENTS];
#elif defined(OS_LINUX)
	s32 epoll_fd;
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

	//
	// tag each socket with a unique ID
	//
	u64 next_listen_id;
	u64 next_server_id;
	u64 next_client_id;
};


global_variable nix_tcp_Engine nix_tcp_engines[nix_tcp_MAX_SERVERS];
global_variable u32 nix_tcp_num_engines = 0;

// OS set filedescriptor to be non-blocking

static void
nix_tcp_set_nonblocking_fd(s32 fd)
{
	s32 opts = fcntl(fd, F_GETFL);
	Assert(opts >= 0);
	opts = opts | O_NONBLOCK;
	s32 update_error = fcntl(fd, F_SETFL, opts);
	Assert(!update_error);
}

// the kevent call is different than the epoll one,
// we always pass the list of events we are interested
// in a contiguous array
static void
nix_tcp_retrigger_socket(nix_tcp_Socket *self)
{
#if defined(OS_MAC)
	kevent(self->tcp->kqueue_fd, &self->ev, 1, 0, 0, 0);
#elif defined(OS_LINUX)
	epoll_ctl(self->tcp->epoll_fd, EPOLL_CTL_MOD, self->file_descriptor, &self->ev);
#endif
}

static void
nix_tcp_socket_first_trigger(nix_tcp_Socket *self)
{
#if defined(OS_MAC)
	kevent(self->tcp->kqueue_fd, &self->ev, 1, 0, 0, 0);
#elif defined(OS_LINUX)
	epoll_ctl(self->tcp->epoll_fd, EPOLL_CTL_ADD, self->file_descriptor, &self->ev);
#endif
}


//
// this can be called from multiple threads
// but only when they are in use which means
// that those threads are already with the
// permission to access this socket
//
// the main tcp thread might check for the
// closed status and reclaim the socket
// from the active list ot the free list
//
// my main doubt about where to associate and
// reclaim the http state handler data for a socket
// (or any other higher level protocol over
// tcp on the client side) is that this is
// the right place to reclaim that resource
// but it is being run in a thread that is not
// the main tcp handling thread.
//
// maybe this would be ok if the release of
// an http state would only change a flag
// status on the custom data. the hardcore
// reclaiming of the buffers would be done
// by the main thread when needed. Similar
// to the socket processing.
//
// static void
// nix_tcp_close_socket_(nix_tcp_Socket *self, s32 flags)
// {
// }

static nix_tcp_Task*
nix_tcp_reserve_task_(nix_tcp_Engine *self)
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
	return task;
}

static void
nix_tcp_process_close_task_(nix_tcp_Engine *self, nix_tcp_Task *task)
{
	Assert(task->type == nix_tcp_CLOSE_TASK);

	//
	// this task doesn't ever fail
	// check if the id of the task is the one we scheduled it for
	//
	nix_tcp_Socket *socket = task->close.socket;

	if (!socket->id || socket->id != task->close.id || socket->type != task->close.type) {
		nix_tcp_log2("double scheduled closing of a socket disconsidered\n");
		return;
	}

	//
	// While receiving and sending data through a socket
	// one of the paths can have closed the socket,
	// while the other path just detected the channel
	// needed to be closed.
	//

	// remove from the event queue
	if (socket->type == pt_TCP_SERVER_SOCKET) {
		// get the listen socket associated with this sever socket
		nix_tcp_Socket *listen_socket = socket->server.listen_socket;

		pt_atomic_sub_u16(&listen_socket->listen.num_connections, 1);

		PlatformTCPEventCallback *event_callback = listen_socket->listen.event_callback;
		if (event_callback) {
			pt_TCP_Socket pt_socket = { .id = socket->id, .handle = socket };
			event_callback(pt_socket, pt_TCP_EVENT_SERVER_SOCKET_TERMINATION, listen_socket->listen.context_event_callback);
			// the user might have changes the user data
			// this is the key point of this mechanism,
			// let the user associate custom data to
			// new server sockets that will persist
			// throughout the socket usage
		}
	}

	nix_tcp_log_socket(socket,"closing...\n");

	// remove from the equeue
	// epoll_ctl(self->epoll_fd, EPOLL_CTL_DEL, socket->file_descriptor, &socket->ev);
	close(socket->file_descriptor);
	socket->id = 0;
	socket->status = 0;

	// task is ready to be used again
	task->ready = 0;
}

static void
nix_tcp_schedule_close_task(nix_tcp_Socket *self)
{
	nix_tcp_Engine *tcp = self->tcp;

	nix_tcp_Task *task = nix_tcp_reserve_task_(tcp);
	task->type = nix_tcp_CLOSE_TASK;
	task->close.socket = self;
	task->close.type = self->type; // type of socket
	task->close.id = self->id; // id of socket

	__sync_synchronize();
	// make sure everything above is ready before we set ready to 1
	task->ready = 1;
}

// this is executed by the main tcp thread

static void
nix_tcp_reclaim_sockets(nix_tcp_Engine *self)
{
	nix_tcp_Socket *it = self->active_sockets;
	s32 count = 0;
	while (it) {
		nix_tcp_Socket *it_next = it->next;
		if (!it->status) {
			nix_tcp_Socket *socket = it;

			// nix_tcp_Engine_release_socket(nix_tcp_Engine *self, nix_tcp_Socket *socket)
			// nix_tcp_Engine_release_socket(self, it);
			{
				// @understand why INUSE? can some thread switch it from
				// CLOSED to INUSE after the test above?
				// Assert(socket->status==nix_tcp_Socket_INUSE || socket->status==nix_tcp_Socket_CLOSED);
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
				// socket->status = nix_tcp_Socket_FREE;
				socket->id = 0;
			}
			++count;
		}
		it = it_next;
	}
	nix_tcp_log2("Collect Closed Socket %d\n", count);
}

static nix_tcp_Socket*
nix_tcp_reserve_socket(nix_tcp_Engine *self)
{
	if (!self->free_sockets) {
		/* collect closed sockets from active sockets if any */
		nix_tcp_reclaim_sockets(self);
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

static void
nix_tcp_schedule_listen_task(nix_tcp_Engine *self, s32 port, u16 max_connections, void *user_data, PlatformTCPDataCallback *data_callback,
	       PlatformTCPEventCallback *event_callback, void *context_event_callback, pt_TCP_Feedback *feedback)
{
	nix_tcp_Task *task = nix_tcp_reserve_task_(self);
	task->data_callback = data_callback;
	task->feedback = feedback;
	task->port = port;
	task->type = nix_tcp_LISTEN_TASK;
	task->user_data = user_data;

	task->listen.event_callback = event_callback;
	task->listen.context_event_callback = context_event_callback;
	task->listen.max_connections = max_connections;

	if (feedback) {
		feedback->status = pt_TCP_FEEDBACK_ACTIVATING;
		feedback->socket = (pt_TCP_Socket) { .id = 0, .handle = 0 };
	}
	__sync_synchronize();
	// make sure everything above is ready before we set ready to 1
	task->ready = 1;
}

//
// produce a client socket task to connect to a server
//
static void
nix_tcp_schedule_client_task(nix_tcp_Engine *self, s32 port, char *hostname, void *user_data, PlatformTCPDataCallback *data_callback, pt_TCP_Feedback *feedback)
{
	nix_tcp_Task *task = nix_tcp_reserve_task_(self);

	task->data_callback = data_callback;
	task->client.hostname = hostname;
	task->feedback = feedback;
	task->port = port;
	task->type = nix_tcp_CLIENT_TASK;
	task->user_data = user_data;
	if (feedback) {
		feedback->status = pt_TCP_FEEDBACK_ACTIVATING;
		feedback->socket = (pt_TCP_Socket) { .id = 0, .handle = 0 };
	}
	__sync_synchronize();
	// make sure everything above is ready before we set ready to 1
	task->ready = 1;
}

static void
nix_tcp_process_listen_task_(nix_tcp_Engine *self, nix_tcp_Task *task)
{
	Assert(task->type == nix_tcp_LISTEN_TASK);
	s32 next_status = pt_TCP_FEEDBACK_OK;
	nix_tcp_Socket *listen_socket = 0;
	/* create self server socket */
	s32 listen_socket_fd = socket(PF_INET, SOCK_STREAM, 0);
	if (listen_socket_fd < 0) {
		nix_tcp_log("pt_Engine_ERROR_SOCKET\n");
		goto error;
	}
	/* set as non-blocking */
	nix_tcp_set_nonblocking_fd(listen_socket_fd);
	struct sockaddr_in listen_socket_addr;
	listen_socket_addr.sin_family = AF_INET;
	listen_socket_addr.sin_port = htons(task->port);
	inet_aton(nix_tcp_LOCAL_ADDR, &(listen_socket_addr.sin_addr));
	s32 bind_error = bind(listen_socket_fd, (struct sockaddr*) &listen_socket_addr, sizeof(listen_socket_addr));
	if (bind_error) {
		nix_tcp_log("pt_Engine_ERROR_BIND\n");
		goto error;
	}
	/* listen */
	s32 listen_error = listen(listen_socket_fd, nix_tcp_MAX_CONNECTIONS);
	if (listen_error) {
		nix_tcp_log("pt_Engine_ERROR_LISTEN\n");
		goto error;
	}
	/* initialize listen socket report */
	f64 start_time = nix_get_time();
	listen_socket = nix_tcp_reserve_socket(self);
	if (!listen_socket) {
		nix_tcp_log("could not reserver socket\n");
		Assert(listen_socket_fd);
	}

	// nix_tcp_Socket_init_listen(listen_socket, listen_socket_fd, task->port, task->user_data, task->callback, start_time);
	// nix_tcp_Socket_init_listen(nix_tcp_Socket *self, s32 file_descriptor, s32 port, void *user_data, PlatformTCPCallback *callback, u64 start_time)
	{
		listen_socket->id = self->next_listen_id++; // assume only one thread creates the connections
		listen_socket->file_descriptor = listen_socket_fd;
		listen_socket->type = pt_TCP_LISTEN_SOCKET;

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
		EV_SET(&listen_socket->ev, listen_socket_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, listen_socket);
#elif defined(OS_LINUX)
		listen_socket->ev.data.ptr = listen_socket;
		listen_socket->ev.events = EPOLLIN | EPOLLET;
		// edge triggered (not oneshot for the listen socket)
#endif

		listen_socket->status = pt_TCP_READ; // set listen socket to be only read? just a convention
		listen_socket->listen.port = task->port;
		listen_socket->data_callback = task->data_callback;
		listen_socket->user_data = task->user_data;
		listen_socket->start_time = start_time;
		listen_socket->listen.event_callback = task->listen.event_callback;
		listen_socket->listen.context_event_callback = task->listen.context_event_callback;
		listen_socket->listen.max_connections = task->listen.max_connections;
	}

	/* register into kqueue */
	nix_tcp_socket_first_trigger(listen_socket);

	nix_tcp_log_socket(listen_socket, "opened\n");

	goto done;
error:
	next_status = pt_TCP_FEEDBACK_ERROR;
done:
	if (task->feedback) {
		task->feedback->status = next_status;
		if (listen_socket) {
			task->feedback->socket.handle = listen_socket;
			task->feedback->socket.id     = listen_socket->id;
		}
	}
	task->ready = 0;
}

static void
nix_tcp_tag_socket_to_send_reset_(s32 file_descriptor)
{
	//
	// @todo understand if this is needed
	//
	//  https://stackoverflow.com/questions/16590847/how-can-i-refuse-a-socket-connection-in-c
	//  If you want the client to see a TCP reset, most TCP stacks
	//  will trigger one if you enable the linger option with a
	//  timeout of 0.
	//
	struct linger lo = { 1, 0 };
	setsockopt(file_descriptor, SOL_SOCKET, SO_LINGER, &lo, sizeof(lo));
}


static void
nix_tcp_process_client_task_(nix_tcp_Engine *self, nix_tcp_Task *task)
{
	Assert(task->type == nix_tcp_CLIENT_TASK);
	s32 next_status = pt_TCP_FEEDBACK_OK;

	// TODO(llins): which one is it? a hostname or an ip address?
	// assume hostname
	s32 client_socket_fd = -1;
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

	s32 status = getaddrinfo(task->client.hostname, port_string, &hints, &servinfo);
	if (status != 0) {
		nix_tcp_log("Error: getaddrinfo()  %s\n", gai_strerror(status));
		goto error;
	}
	s32 option = 0;
	for (struct addrinfo *p=servinfo; p!=0; p=p->ai_next) {
		++option;
		client_socket_fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if (client_socket_fd >= 0) {
			s32 connect_status = connect(client_socket_fd, p->ai_addr, p->ai_addrlen);
			if (connect_status != 0) {
				nix_tcp_log("Could not connect(...) using addrinfo %d: connect(...)\n", option);
				close(client_socket_fd);
			} else {
				goto connect_ok;
			}
		} else {
			nix_tcp_log("Could not socket(...) using addrinfo %d: connect(...)\n", option);
		}
	}
	goto error;

connect_ok:
	/* set client to be nonblocking */
	nix_tcp_set_nonblocking_fd(client_socket_fd);

	/* register client socket fd into the kqueue */
	client_socket = nix_tcp_reserve_socket(self);
	if (!client_socket) {
		nix_tcp_log("could not reserve socket\n");
		goto error;
	}

	// @maybe register hostname and port on the client socket?
	{
		// nix_tcp_Socket_init_client(nix_tcp_Socket *self, s32 file_descriptor, void *user_data, s32 port, char *hostname, PlatformTCPCallback *callback, u64 start_time)
		// nix_tcp_Socket_init_client(client_socket, client_socket_fd, task->user_data, task->port, task->client.hostname, task->callback, start_time);

		Assert(!client_socket->status);
		client_socket->id = self->next_client_id++;
		client_socket->file_descriptor = client_socket_fd;
		client_socket->type = pt_TCP_CLIENT_SOCKET;

#if defined(OS_MAC)
		EV_SET(&client_socket->ev, client_socket_fd, EVFILT_READ, EV_ADD | EV_ONESHOT, 0, 0, client_socket);
#elif defined(OS_LINUX)
		client_socket->ev.data.ptr = client_socket;
		client_socket->ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
#endif

		client_socket->status = pt_TCP_READ_WRITE;
		client_socket->data_callback = task->data_callback;
		client_socket->user_data = task->user_data;
		client_socket->start_time = start_time;
	}

	//
	// once closed, send signal to client.
	// also do this on the client fd
	//
	nix_tcp_tag_socket_to_send_reset_(client_socket->file_descriptor);

	nix_tcp_log_socket(client_socket, "opened\n");

	/* register into kqueue */
	nix_tcp_socket_first_trigger(client_socket);
	goto done;
error:
	next_status = pt_TCP_FEEDBACK_ERROR;
done:
	if (task->feedback) {
		task->feedback->status = next_status;
		if (client_socket) {
			task->feedback->socket = (pt_TCP_Socket) { .id = client_socket->id, .handle = client_socket };
		}
	}
	task->ready = 0;
}

static void
nix_tcp_process_tasks_(nix_tcp_Engine *self)
{
	// how to safely check if task queue is full
	u64 it  = (self->tasks.sentinel + 1) & nix_tcp_TASK_QUEUE_MASK;
	u64 end = self->tasks.end            & nix_tcp_TASK_QUEUE_MASK;
	while (it != end) {
		nix_tcp_Task *task = self->tasks.queue + it;
		if (task->ready) {
			/* listen task */
			if (task->type == nix_tcp_LISTEN_TASK) {
				nix_tcp_process_listen_task_(self, task);
			} else if (task->type == nix_tcp_CLIENT_TASK) {
				nix_tcp_process_client_task_(self, task);
			} else if (task->type == nix_tcp_CLOSE_TASK) {
				nix_tcp_process_close_task_(self, task);
			} else {
				nix_tcp_log("task type not supported\n");
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

static void
nix_tcp_terminate(nix_tcp_Engine *self)
{
#if defined(OS_MAC)
	Assert(self->kqueue_fd);
#elif defined(OS_LINUX)
	Assert(self->epoll_fd);
#endif

	nix_tcp_Socket *it = self->active_sockets;
	while (it) {
		if (it->status) {
			nix_tcp_log_socket2(it,"status: %d is being closed\n", it->status);
			nix_tcp_schedule_close_task(it);
		}
		it = it->next;
	}

	nix_tcp_process_tasks_(self);

#if defined(OS_MAC)
	close(self->kqueue_fd);
#elif defined(OS_LINUX)
	close(self->epoll_fd);
#endif

	self[0] = (nix_tcp_Engine) { 0 };
}


//
// reads from a tcp socket and dispatch until
// nothing else is available for reading
// or the other side closes the connection.
// this can be done in multiple different
// threads.
//
static void
nix_tcp_read(nix_tcp_Socket *socket)
{
	//
	// ok, here is the culprit for having small chunks
	// being triggered to the tcp handler and breaking a
	// single http request into fragments
	//
	char buf[nix_tcp_BUFFER_SIZE];
	pt_TCP_Socket pt_socket = { .id = socket->id, .handle = socket };
	b8 done = 0;
	for (;;) {

		/* this could be running in a work thread */
		s32 count = read(socket->file_descriptor, buf, sizeof(buf)-1);
		if (count == -1) {
			/* If errno == EAGAIN, that means we have read all
			   data. So go back to the main loop. */
			if (errno != EAGAIN) {
				nix_tcp_log_socket2(socket, "read(...) == -1 (%d - %s) scheduling closing of connection\n", errno, strerror(errno));
				nix_tcp_schedule_close_task(socket);
			} else {
				nix_tcp_log_socket2(socket, "read(...) == -1 (%d - %s) exiting nix_tcp_read\n", errno, strerror(errno));
				nix_tcp_retrigger_socket(socket);
			}
			break;
		} else if (count == 0) {
			// this case indicates the closing of the other side of the connection?
			/* End of file. The remote has closed the socket. */
			nix_tcp_log_socket2(socket, "read(...) == 0 (%d - %s) scheduling closing of connection\n", errno, strerror(errno));
			nix_tcp_schedule_close_task(socket);
			break;
		} else {
			nix_tcp_log_socket3(socket, "read %d bytes and dispatched those to the callback\n", (s32) count);
			buf[count] = 0;
			socket->data_callback(pt_socket, buf, count);
		}
	}
}

static s32
nix_tcp_write(nix_tcp_Socket* socket, void *buffer, s64 length)
{
	if (length == 0) {
		return 1;
	}

	s64 offset = 0;
	for(;;) {
		if (socket->type == pt_TCP_SERVER_SOCKET) {
			nix_tcp_log_socket2(socket, "server socket sending %d bytes\n", (s32) (length-offset));
		}

		// https://stackoverflow.com/questions/108183/how-to-prevent-sigpipes-or-handle-them-properly
		// s64 count = write(socket->file_descriptor, buffer + offset, length - offset);
		// MSG_NOSIGPIPE
#if defined(OS_MAC)
		s64 count = send(socket->file_descriptor, OffsetedPointer(buffer,offset), length - offset, SO_NOSIGPIPE);
#elif defined(OS_LINUX)
		s64 count = send(socket->file_descriptor, OffsetedPointer(buffer,offset), length - offset, MSG_NOSIGNAL );
#endif

		if (count == -1) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				nix_tcp_log_socket3(socket, "send(...) == -1, (%d - %s), trying to write again...\n",  errno, strerror(errno));
				// usleep(1);
				continue;
			} else {
				nix_tcp_log_socket2(socket, "send(...) == -1, (%d - %s), scheduling the closing of the connection\n", errno, strerror(errno));
				nix_tcp_schedule_close_task(socket);
				break;
			}
		} else if (count == 0) {
			nix_tcp_log_socket2(socket, "send(...) == 0, (%d - %s), scheduling the closing of the connection\n", errno, strerror(errno));
			nix_tcp_schedule_close_task(socket);
			break;
		} else if (count > 0) {
			offset += count;
			nix_tcp_log_socket3(socket, "%d bytes written to the socket, done with nix_tcp_write\n", (s32)offset);
			if (offset == length) {
				if (socket->type == pt_TCP_CLIENT_SOCKET) {
					nix_tcp_log_socket2(socket, "client socket finished sending %d bytes\n", (s32) (length));
				}
				return 1;
			}
		}
	}
	return 0;
}

static
PLATFORM_WORK_QUEUE_CALLBACK(nix_tcp_work_queue_callback)
{
	nix_tcp_Socket *connection = (nix_tcp_Socket*) data;
	nix_tcp_read(connection);
}

static void
nix_tcp_process_events(nix_tcp_Engine *tcp, pt_WorkQueue *work_queue)
{
	/* register listen and client sockets as well as close socket coming as registered tasks */
	nix_tcp_process_tasks_(tcp);

	/* wait for events on sockets beign tracked for at most nix_tcp_TIMEOUT milliseconds */
#if defined(OS_MAC)
	// @note one second timeout
	static const struct timespec timeout = { .tv_sec= nix_tcp_TIMEOUT_SECONDS, .tv_nsec = 1000 * nix_tcp_TIMEOUT_MILLISECONDS };
	s32 num_fd = kevent(tcp->kqueue_fd, 0, 0, tcp->events, ArrayCount(tcp->events), &timeout);

	struct kevent ev; // @todo remove this line?

#elif defined(OS_LINUX)
	s32 num_fd = epoll_wait(tcp->epoll_fd, tcp->events, ArrayCount(tcp->events), nix_tcp_TIMEOUT_MILLISECONDS + 1000 * nix_tcp_TIMEOUT_SECONDS);
	if (num_fd == -1) {
		nix_tcp_log("epoll_wait returned error: %d -> %s\n", errno, strerror(errno));
	}
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
	for (s32 i=0; i<num_fd; ++i) {

		// here is where we map the socket file descriptor to the
		// higher level socket we manage
#if defined(OS_MAC)
		nix_tcp_Socket *socket = (nix_tcp_Socket*) tcp->events[i].udata;
#elif defined(OS_LINUX)
		nix_tcp_Socket *socket = (nix_tcp_Socket*) tcp->events[i].data.ptr;
#endif

		if (socket->type == pt_TCP_LISTEN_SOCKET) {
			pf_BEGIN_BLOCK("accepting_connection");
			// try accepetping one or more incoming connections
			for (;;) {
				struct sockaddr_in new_socket_addr;
				socklen_t new_socket_len = sizeof(struct sockaddr_in);
				s32 new_socket_fd = accept(socket->file_descriptor, (struct sockaddr*) &new_socket_addr, &new_socket_len);
				if (new_socket_fd == -1) {
					if ((errno == EAGAIN) || (errno == EWOULDBLOCK)) {
						break;
					} else {
						// @todo cleanup this (shouldn't crash)
						nix_tcp_log("Fatal Error: problem when accepting socket\n");
						Assert(0 && "problem when accepting socket");
					}
				}
				Assert(new_socket_fd >= 0);

				//
				// once closed, send signal to client.
				// also do this on the client fd
				//
				nix_tcp_tag_socket_to_send_reset_(new_socket_fd);

				//
				// too many connections already
				// close the lower level connection
				//
				if (socket->listen.num_connections == socket->listen.max_connections) {
					nix_tcp_log_socket2(socket, "closing accepted server socket: reached max connections (%d)\n", socket->listen.max_connections);
					close(new_socket_fd);
					continue;
				}

				nix_tcp_set_nonblocking_fd(new_socket_fd);

				//
				// @todo this new_socket should be tied to the same
				// http-state-processing-data always, and not be thread
				// specific
				//
				// here is ok to select the make the call to reserve the http
				// object, given that the tcp process is a single one
				//
				// this selection can fail which means that we should kill this
				// connection right away saying that we are out of resources
				//
				nix_tcp_Socket *new_socket = nix_tcp_reserve_socket(tcp);
				f64 start_time = nix_get_time();

				// increment number of connections
				pt_atomic_add_u16(&socket->listen.num_connections,1);

				// nix_tcp_Socket_init_server(new_socket, socket, new_socket_fd, socket->user_data, socket->callback, start_time);
				// nix_tcp_Socket_init_server(nix_tcp_Socket *self, nix_tcp_Socket *listen_socket, s32 file_descriptor, void *user_data,
				//                                                     PlatformTCPCallback *callback, u64 start_time)
				{
					// {
					Assert(!new_socket->status);

					new_socket->id = tcp->next_server_id++; // assume only one thread creates the connections
					new_socket->file_descriptor = new_socket_fd;
					new_socket->type = pt_TCP_SERVER_SOCKET;

#if defined(OS_MAC)
					EV_SET(&new_socket->ev, new_socket_fd, EVFILT_READ, EV_ADD | EV_ENABLE | EV_ONESHOT, 0, 0, new_socket);
#elif defined(OS_LINUX)
					new_socket->ev.data.ptr = new_socket;
					new_socket->ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT; // | EPOLLHUP | EPOLLRDHUP | EPOLLERR;
#endif

					new_socket->server.listen_socket = socket;
					new_socket->status = pt_TCP_READ_WRITE;
					new_socket->data_callback = socket->data_callback;
					new_socket->user_data = socket->user_data;
					new_socket->start_time = start_time;
					// }
				}

				if (socket->listen.event_callback) {
					// signal that a new socket was created
					pt_TCP_Socket pt_new_socket = { .id = new_socket->id, .handle = new_socket };
					socket->listen.event_callback(pt_new_socket, pt_TCP_EVENT_SERVER_SOCKET_INITIALIZATION, socket->listen.context_event_callback);

					// new_socket->user_data = pt_new_socket.user_data;

					// the user might have changes the user data
					// this is the key point of this mechanism,
					// let the user associate custom data to
					// new server sockets that will persist
					// throughout the socket usage
				}

				nix_tcp_socket_first_trigger(new_socket);

				nix_tcp_log_socket(new_socket, "opened on %s:%d\n", inet_ntoa(new_socket_addr.sin_addr), (s32) new_socket_addr.sin_port);
			}

			pf_END_BLOCK();

		} else {
			// nix_tcp_log_socket2(socket, "tcp event\n");

			if (work_queue) {
				/* work threads */
				nix_work_queue_add_entry(work_queue, nix_tcp_work_queue_callback, socket);
			} else {
				/* single threaded */
				nix_tcp_read(socket);
			}
		}
	}
}



static
PLATFORM_TCP_PROCESS_EVENTS(nix_tcp_process_events_)
{
	nix_tcp_process_events((nix_tcp_Engine*) tcp.handle, work_queue);
}

static
PLATFORM_TCP_WRITE(nix_tcp_write_)
{
	nix_tcp_Socket *nix_socket = (nix_tcp_Socket*) socket.handle;
	if (nix_socket->id == socket.id) {
		return nix_tcp_write(nix_socket, buffer, length);
	} else {
		return 0;
	}
}

//
// the glue functions will have a underscore appended to
// the core function
//

static
PLATFORM_TCP_CREATE(nix_tcp_create_)
{
	pt_TCP result = { .handle = 0 };
	if (nix_tcp_num_engines == ArrayCount(nix_tcp_engines)) {
		return result;
	}

	/* osx server */
	nix_tcp_Engine *nix_tcp_engine = nix_tcp_engines + nix_tcp_num_engines;

	{
		// nix_tcp_initialize(nix_tcp_Engine *self)
#if defined(OS_MAC)
		/* create kqueue object */
		nix_tcp_engine->kqueue_fd = kqueue();
		Assert(nix_tcp_engine->kqueue_fd);
#elif defined(OS_LINUX)
		/* create epoll object */
		nix_tcp_engine->epoll_fd = epoll_create(nix_tcp_MAX_EVENTS);
		Assert(nix_tcp_engine->epoll_fd);
#endif

		// initialize empty socket with the right index
		for (s32 i=0;i<ArrayCount(nix_tcp_engine->sockets);++i) {
			nix_tcp_Socket *socket = nix_tcp_engine->sockets + i;
			*socket = (nix_tcp_Socket) { 0 };
			socket->status = 0;
			socket->tcp = nix_tcp_engine;
			socket->index = i;
			socket->file_descriptor = -1;
			//
			socket->prev = (i > 0) ? (socket - 1) : 0;
			socket->next = (i<ArrayCount(nix_tcp_engine->sockets)-1) ? (socket + 1) : 0;
		}

		nix_tcp_engine->free_sockets = nix_tcp_engine->sockets;
		nix_tcp_engine->active_sockets = 0;
		/* initialize tasks infrastructure */
		nix_tcp_engine->tasks.sentinel = 0;
		nix_tcp_engine->tasks.end      = 1;
		for (s32 i=0;i<ArrayCount(nix_tcp_engine->tasks.queue);++i) {
			nix_tcp_engine->tasks.queue[i].ready = 0;
		}

		// reserve 0 for non-valid ids
		nix_tcp_engine->next_listen_id = 1;
		nix_tcp_engine->next_server_id = 1;
		nix_tcp_engine->next_client_id = 1;
	}

	/* osx server */
	++nix_tcp_num_engines;
	result.handle = (void*) nix_tcp_engine;
	return result;
}

static
PLATFORM_TCP_DESTROY(nix_tcp_destroy_)
{
	if (!tcp.handle) {
		return;
	}
	nix_tcp_Engine *tcp_engine = (nix_tcp_Engine*) tcp.handle;
	nix_tcp_terminate(tcp_engine);
}

// #define PLATFORM_TCP_SERVE(name) void name(pt_TCP *tcp, s32 port, void *user_data, PlatformTCPCallback *callback, s32 *status)
static
PLATFORM_TCP_LISTEN(nix_tcp_schedule_listen_task_)
{
	nix_tcp_Engine *nix_tcp_engine = (nix_tcp_Engine*) tcp.handle;
	/* schedule tcp engine to create a listen socket on next process events cycle */
	nix_tcp_schedule_listen_task(nix_tcp_engine, port, max_connections, user_data, data_callback, event_callback, context_event_callback, feedback);
}

// #define PLATFORM_TCP_CONNECT(name) void name(pt_TCP *tcp, s32 port, char *hostname, void *user_data, PlatformTCPCallback *callback, s32 *status)
static
PLATFORM_TCP_CONNECT(nix_tcp_connect_)
{
	nix_tcp_Engine *nix_tcp_engine = (nix_tcp_Engine*) tcp.handle;
	/* schedule tcp engine to create a client socket on next process events cycle */
	/* client socket should connect to host referred on hostname */
	// TODO(llins): should we copy hostname to a safe place? (client code might destroy it)
	nix_tcp_schedule_client_task(nix_tcp_engine, port, hostname, user_data, data_callback, feedback);
}

static
PLATFORM_TCP_CLOSE_SOCKET(nix_tcp_close_socket_)
{
	nix_tcp_Socket *nix_socket = (nix_tcp_Socket*) socket.handle;
	if (nix_socket->id == socket.id) {
		nix_tcp_schedule_close_task(nix_socket);
	}
}

static
PLATFORM_TCP_SOCKET_SET_CUSTOM_DATA(nix_tcp_socket_set_custom_data_)
{
	nix_tcp_Socket *nix_socket = (nix_tcp_Socket*) socket.handle;
	if (nix_socket->id == socket.id) {
		nix_socket->user_data = custom_data;
	}
}

static
PLATFORM_TCP_SOCKET_GET_CUSTOM_DATA(nix_tcp_socket_get_custom_data_)
{
	nix_tcp_Socket *nix_socket = (nix_tcp_Socket*) socket.handle;
	if (nix_socket->id == socket.id) {
		return nix_socket->user_data;
	} else {
		return 0;
	}
}


// read, read+write, write or zero
static
PLATFORM_TCP_SOCKET_STATUS(nix_tcp_socket_status_)
{
	nix_tcp_Socket *nix_socket = (nix_tcp_Socket*) socket.handle;
	if (nix_socket->id == socket.id) {
		return nix_socket->status;
	} else {
		return 0;
	}
}

// #define PLATFORM_MEMCOPY(name) void name(void *dst, void *src, u64 length)
PLATFORM_MEMORY_COPY(nix_memory_copy)
{
	memcpy(dst, src, length);
}

// #define PLATFORM_MEMCOPY(name) void name(void *dst, void *src, u64 length)
PLATFORM_MEMORY_MOVE(nix_memory_move)
{
	memmove(dst, src, length);
}

PLATFORM_MEMORY_SET(nix_memory_set)
{
	memset(dst, byte_value, length);
}

// #define PLATFORM_MEMCOPY(name) void name(void *dst, void *src, u64 length)
PLATFORM_MEMORY_COMPARE(nix_memory_compare)
{
	return memcmp(a, b, length);
}

static void
nix_get_filenames_in_directory_recursive_(Print *print, DIR *dir, PlatformGetFilenamesInDirectoryCallback *callback, void *user_data)
{
	struct stat stat_data;

	struct dirent *ent = 0;
	while ((ent = readdir(dir)) != 0) {
		if (cstr_match(ent->d_name,".") || cstr_match(ent->d_name,"..")) {
			continue;
		}
		char *p = print_checkpoint(print);
		print_format(print, "/%s", ent->d_name);
		s32 status = stat(print->begin, &stat_data);
		if (status != 0) {
			fprintf(stderr,"[Warning] Could not stat file %s (status: %d err: %s)\n",print->begin, status, strerror(errno));
			goto done;
		}
		if (S_ISDIR(stat_data.st_mode)) {
			DIR *dir2 = opendir(print->begin);
			if (!dir2) {
				fprintf(stderr,"[Warning] Could not open directory %s\n",print->begin);
				goto done;
			}
			// print_char(print,'/');
			nix_get_filenames_in_directory_recursive_(print, dir2, callback, user_data);

		} else {
			callback(print->begin, user_data);
		}
done:
		print_restore(print,p);
// 		if (ent->d_type == TD_DIR) {
// 			if (cstr_match(entry->d_name,".") || cstr_match(entry->d_name,"..")) {
// 				continue;
// 			} else {
// 			}
// 		} else {
// 			print_cstr(print,entry->d_name);
// 			char *p = print_checkpoint(print);
// 			callback(print->begin, user_data);
// 			print_restore(print,p);
// 		}
	}
}

// #define PLATFORM_GET_FILENAMES_IN_DIRECTORY_CALLBACK(name) void name(char *filename, s32 type, void *user_data)
// typedef PLATFORM_GET_FILENAMES_IN_DIRECTORY_CALLBACK(PlatformGetFilenamesInDirectoryCallback);
// #define PLATFORM_GET_FILENAMES_IN_DIRECTORY(name) s32 name(char *directory, s32 recursive, PlatformGetFilenamesInDirectoryCallback *callback, void *user_data)
PLATFORM_GET_FILENAMES_IN_DIRECTORY(nix_get_filenames_in_directory)
{
	DIR *dir = opendir(directory);
	if (!dir) {
		return 0;
	}
	if (!recursive) {
		struct dirent *ent = 0;
		while ((ent = readdir(dir)) != 0) {
			callback(ent->d_name, user_data);
		}
	} else {
		Print *print = print_new_raw(Kilobytes(4)); // longest filename
		if (directory[0] == 0) {
			print_char(print, '.');
		} else {
			print_format(print, directory);
		}
		nix_get_filenames_in_directory_recursive_(print, dir, callback, user_data);
		platform.free_memory_raw(print);
	}
	closedir (dir);
	return 1;
}

typedef struct {
	void *context;
	PlatformSortCompare *cmp;
} nix_SortCompareAdapter;

static s32
nix_adapted_compare(void *context, const void *a, const void *b)
{
	nix_SortCompareAdapter *adapter = context;
	return adapter->cmp(a, b, adapter->context);
}

// #define PLATFORM_SORT(name) void name(void *data, u64 count, u64 size, PlatformSortCompare *cmp)
PLATFORM_SORT(nix_sort)
{
#if defined(OS_MAC)
	//
	// bsd convention is different
	// @cleanup implement a single sort function instead of using the
	// platform one
	//
	nix_SortCompareAdapter adapter = {
		.context = context,
		.cmp = cmp
	};
	qsort_r(data, count, size, &adapter, nix_adapted_compare);
#elif defined(OS_LINUX)
	qsort_r(data, count, size, cmp, context);
#endif
}

// #define PLATFORM_MEMCOPY(name) void name(void *dst, void *src, u64 length)
PLATFORM_FILENAME_MATCH(nix_filename_match)
{
	return !fnmatch(pattern, text, FNM_PATHNAME);
}

PLATFORM_GETENV(nix_getenv)
{
	return getenv(variable_name);
}

PLATFORM_FAILED_ASSERTION(nix_failed_assertion)
{
	char time_buffer[32];
	time_t rawtime;
	struct tm * timeinfo;
	time ( &rawtime );
	timeinfo = localtime ( &rawtime );
	strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
	char buffer[Kilobytes(1)];
	Print print;
	print_init(&print, buffer, sizeof(buffer));
	u32 size = sizeof(buffer);
	nix_executable_path(buffer, &size);
	print.end = print.begin + cstr_length(buffer);
	print_cstr(&print,".log");
	char *exe_name = cstr_buffer_find_char('/',print.begin,print.end,0,1);
	if (exe_name) {
		++exe_name;
	} else {
		exe_name = buffer;
	}
	FILE *f = fopen(exe_name,"a");
	fprintf(f, "[%s] %s:%d: Assertion `%s` failed.\n", time_buffer, filename, line, expression);
	fflush(f);
	fclose(f);
	abort();
}

static void
nix_init_platform(PlatformAPI* p)
{
	nix_init();
	p->total_allocated_memory   = nix_total_allocated_memory;

	p->allocate_memory          = nix_allocate_memory;
	p->allocate_memory_raw      = nix_allocate_memory_raw;
	p->copy_memory              = nix_copy_memory;
	p->free_memory              = nix_free_memory;
	p->free_memory_raw          = nix_free_memory_raw;
	p->resize_memory            = nix_resize_memory;

	p->open_file                = nix_open_file;
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

	p->tcp_create                 = nix_tcp_create_;
	p->tcp_destroy                = nix_tcp_destroy_;
	p->tcp_listen                 = nix_tcp_schedule_listen_task_;
	p->tcp_write                  = nix_tcp_write_;
	p->tcp_process_events         = nix_tcp_process_events_;
	p->tcp_connect                = nix_tcp_connect_;
	p->tcp_close_socket           = nix_tcp_close_socket_;
	p->tcp_socket_get_custom_data = nix_tcp_socket_get_custom_data_;
	p->tcp_socket_set_custom_data = nix_tcp_socket_set_custom_data_;
	p->tcp_socket_status          = nix_tcp_socket_status_;

	p->memory_copy              = nix_memory_copy;
	p->memory_move              = nix_memory_move;
	p->memory_set               = nix_memory_set;
	p->memory_compare           = nix_memory_compare;

	p->get_filenames_in_directory = nix_get_filenames_in_directory;

	p->sort                     = nix_sort;

	p->filename_match           = nix_filename_match;
	p->getenv                   = nix_getenv;

	p->failed_assertion         = nix_failed_assertion;
}
