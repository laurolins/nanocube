/* include services with common impl. on linux and osx */
#include "osx_linux_platform.c"

// void name(FilePath *fp)
PLATFORM_EXECUTABLE_PATH(osx_executable_path)
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

/************* Work Queue Support ************/

typedef struct {
	PlatformWorkQueueCallback *callback;
	void *data;
} pt_WorkQueueEntry;

#include <dispatch/dispatch.h>
// dispatch_semaphore_t semaphore;
// semaphore = dispatch_semaphore_create(1); // init with value of 1

struct pt_WorkQueue {
	u32 volatile completion_goal;
	u32 volatile completion_count;

	u32 volatile next_entry_to_write;
	u32 volatile next_entry_to_read;

	/* platform dependent semaphore primitive */
	dispatch_semaphore_t semaphore_handle;

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

	self->semaphore_handle = dispatch_semaphore_create(0);

	self->done        = 0;
	self->num_threads = 0;

	// s32 err = sem_init(&self->semaphore_handle_storage, 0, 0);
	// self->semaphore_handle = &self->semaphore_handle_storage;
	// self->semaphore_handle = sem_open("/workqueue", O_EXCL); // exclusive O_CREAT); // , 700, 0);
	// self->semaphore_handle = sem_open("/nanocube_workqueue", O_CREAT, 700, 0); // exclusive O_CREAT); // , 700, 0);
	// return self->semaphore_handle != SEM_FAILED;
	return 1;  // err == 0;
}

// #define COMPILER_BARRIER() asm volatile("" ::: "memory")
// #define INTERLOCKED_COMPARE_EXCHANGE(a,b,c) asm volatile("" ::: "memory")

PLATFORM_WORK_QUEUE_ADD_ENTRY(osx_work_queue_add_entry)
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

	dispatch_semaphore_signal(queue->semaphore_handle);
// 	sem_post(queue->semaphore_handle);
	/* ReleaseSemaphore(queue->semaphore_handle, 1, 0); */
}

internal b8
osx_work_queue_work_on_next_entry(pt_WorkQueue *queue)
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
PLATFORM_WORK_QUEUE_COMPLETE_ALL_WORK(osx_work_queue_complete_all_work)
{
	while(queue->completion_goal != queue->completion_count) {
		osx_work_queue_work_on_next_entry(queue);
	}
	queue->completion_goal  = 0;
	queue->completion_count = 0;
}

internal
PLATFORM_WORK_QUEUE_FINISHED(osx_work_queue_finished)
{
	return queue->completion_goal == queue->completion_count;
}

PLATFORM_WORK_QUEUE_CALLBACK(osx_work_queue_thread_stop)
{
	pt_WorkQueue *queue2 = (pt_WorkQueue*) data;
	pt_atomic_sub_u32(&queue2->num_threads,1);
	pthread_exit(0);
}

global_variable pthread_key_t osx_thread_index_key;
global_variable b8 osx_app_state_interrupted;

/* should be called by the main thread */
internal void
osx_init()
{
	osx_app_state_interrupted = 0;
	pthread_key_create(&osx_thread_index_key,0);
}

internal void*
osx_work_queue_thread_run(void* data)
{
	pt_WorkQueue *queue = (pt_WorkQueue*) data;

	// increment
	u32 index = pt_atomic_add_u32(&queue->num_threads,1);

	pthread_setspecific(osx_thread_index_key, (void*) ((u64) index));

	// u32 TestThreadID = GetThreadID();
	// Assert(TestThreadID == GetCurrentThreadId());
	for(;;) {
		if(osx_work_queue_work_on_next_entry(queue)) {
			dispatch_semaphore_wait(queue->semaphore_handle, DISPATCH_TIME_FOREVER);
			// sem_wait(queue->semaphore_handle);
			// WaitForSingleObjectEx(Queue->SemaphoreHandle, INFINITE, FALSE);
		}
	}
	return 0;
	// return(0);
}

//
// internal void*
// osx_thread_procedure(void* data)
// {
// 	osx_ThreadInfo *thread_info = (osx_ThreadInfo*) data;
// 	pt_WorkQueue *queue = thread_info->queue;
// 	// u32 TestThreadID = GetThreadID();
// 	// Assert(TestThreadID == GetCurrentThreadId());
// 	for(;;) {
// 		if(osx_WorkQueue_work_on_next_entry(queue)) {
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
PLATFORM_WORK_QUEUE_CREATE(osx_work_queue_create)
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
		s32 error = pthread_create(&thread, 0, osx_work_queue_thread_run, queue);
		Assert(!error);
		pthread_detach(thread);
	}

	while (queue->num_threads != num_threads) {
		usleep(10);
	}

	return queue;
}

// internal b8
// osx_init_work_queue(pt_WorkQueue *queue, osx_ThreadInfo *thread_begin, u32 thread_count)
// {
// 	if (!pt_WorkQueue_init(queue, thread_count))
// 		return 0;
//
// 	for (u32 i=0;i<thread_count;++i) {
// 		osx_ThreadInfo *thread_info = thread_begin + i;
// 		thread_info->queue = queue;
//
// 		// 		pthread_attr_t attr;
// 		// 		pthread_attr_init(&attr);
// 		// 		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
// 		// 		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
//
// 		pthread_t thread;
//
// 		s32 error = pthread_create(&thread, 0, osx_thread_procedure, thread_info);
// 		Assert(!error);
// 		pthread_detach(thread);
// 	}
// 	return 1;
// }
//

internal
PLATFORM_GET_THREAD_INDEX(osx_get_thread_index)
{
	return (u64) pthread_getspecific(osx_thread_index_key);
}

internal
PLATFORM_THREAD_SLEEP(osx_thread_sleep)
{
	usleep(millisecs);
}

/* this should be called fro the same thread that called start */
internal
PLATFORM_WORK_QUEUE_DESTROY(osx_work_queue_destroy)
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
		osx_work_queue_add_entry(queue, osx_work_queue_thread_stop, queue);
	}
	while (queue->num_threads > 0) {
		usleep(10);
	}

	dispatch_release(queue->semaphore_handle);
	free(queue);
}







/************* End Work Queue Support ************/


internal void
osx_init_platform(PlatformAPI* p)
{
	/* initialize common services */
	osx_linux_init_platform(p);
	p->executable_path          = osx_executable_path;
	p->work_queue_add_entry     = osx_work_queue_add_entry;
	p->work_queue_complete_work = osx_work_queue_complete_all_work;
	p->work_queue_create        = osx_work_queue_create;
	p->work_queue_destroy       = osx_work_queue_destroy;
	p->work_queue_finished      = osx_work_queue_finished;
	p->get_thread_index         = osx_get_thread_index;
	p->thread_sleep             = osx_thread_sleep;

	/* tcp support is not implemented on OSX */
	p->tcp_create               = 0;
	p->tcp_serve                = 0;
	p->tcp_write                = 0;
	p->tcp_process_events       = 0;
	p->tcp_client               = 0;
}
