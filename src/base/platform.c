#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <limits.h>
#include <inttypes.h>

typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t   s8 ;
typedef int16_t  s16;
typedef int32_t  s32;
typedef int64_t  s64;

typedef float    f32;
typedef double   f64;

typedef u8       b8;
typedef u16      b16;
typedef u32      b32;
typedef u64      b64;

typedef struct {
	u64 low;
	u64 high;
} u128;


#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)
#define BytesToMegabytes(Value) ((f64)(Value)/Megabytes(1))

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#define PointerDifference(b,a) (((char*)(b))-((char*)(a)))
#define OffsetedPointer(ptr,offset) ((void*) ((char*) ptr + offset))
#define RightOffsetedPointer(ptr,size,offset) ((void*) ((char*) ptr + size - offset))

// static, local_persist, global_variable
// #if !defined(static)
// #define static static
// #endif
#define local_persist static
#define global_variable static

//---------------------------
// min and max
//---------------------------

#define Clamp(x,a,b) (((x)<(a)) ? (a) : (((x)>(b)) ? (b) : (x)))

#define Min(a,b) (((a)<(b))?(a):(b))
#define Max(a,b) (((a)>(b))?(a):(b))
#define RAlign(a,b) (b*((a+b-1)/b))
#define LAlign(a,b) (b*(a/b))
// https://stackoverflow.com/questions/3982348/implement-generic-swap-macro-in-c
#define Swap2(x, y, t) do { t SWAP = x; x = y; y = SWAP; } while (0)
#define Swap(x,y) do \
   { unsigned char swap_temp[sizeof(x) == sizeof(y) ? (s32)sizeof(x) : -1]; \
     memcpy(swap_temp,&y,sizeof(x)); \
     memcpy(&y,&x,       sizeof(x)); \
     memcpy(&x,swap_temp,sizeof(x)); \
    } while(0)



//---------------------------
// math stuff
//---------------------------
#define pt_trunc_f64(x)   trunc(x)
#define pt_trunc_f32(x)   truncf(x)
#define pt_modf_f32(x,i)  modff(x,i)
#define pt_modf_f64(x,i)  modf(x,i)
#define pt_abs_f32(x)     fabsf(x)
#define pt_abs_f64(x)     fabs(x)
#define pt_abs_s32(x)     abs(x)
#define pt_abs_s64(x)     labs(x)
#define pt_acos_f32(x)    acosf(x)
#define pt_acos_f64(x)    acos(x)
#define pt_atan2_f32(x,y) atan2f(x,y)
#define pt_atan2_f64(x,y) atan2(x,y)
#define pt_atan_f32(x)    atanf(x)
#define pt_atan_f64(x)    atan(x)
#define pt_cos_f32(x)     cosf(x)
#define pt_cos_f64(x)     cos(x)
#define pt_floor_f32(x)   floorf(x)
#define pt_floor_f64(x)   floor(x)
#define pt_ceil_f32(x)   ceilf(x)
#define pt_ceil_f64(x)   ceil(x)
#define pt_round_f32(x)   roundf(x)
#define pt_round_f64(x)   round(x)
#define pt_log_f32(x)     logf(x)
#define pt_log_f64(x)     log(x)
#define pt_pow_f32(b,e)   powf(b,e)
#define pt_sin_f32(x)     sinf(x)
#define pt_sin_f64(x)     sin(x)
#define pt_sinh_f32(x)    sinhf(x)
#define pt_sinh_f64(x)    sinh(x)
#define pt_sqrt_f32(x)    sqrtf(x)
#define pt_sqrt_f64(x)    sqrt(x)
#define pt_tan_f32(x)     tanf(x)
#define pt_tan_f64(x)     tan(x)
#define pt_PI 3.14159265358979323846

#define pt_PI_F32 3.14159265358979323846f
// the parenthesis might force the division to happen at compile time
#define pt_deg_to_rad_f32(x) (x)*(pt_PI_F32/180.0f)
#define pt_rad_to_deg_f32(x) (x)*(180.0f/pt_PI_F32)
#define pt_deg_to_rad_f64(x) (x)*(pt_PI/180.0)
#define pt_rad_to_deg_f64(x) (x)*(180.0/pt_PI)

//---------------------------
// limits
//---------------------------

#define pt_MAX_U64 ULLONG_MAX;

/* Allow for functions below to create entries on profile log */
#include "profile.c"

//------------------------------------------------------------------------------
// intrinsics
//------------------------------------------------------------------------------
// #if 0
// #if !defined(COMPILER_MSVC)
// #define COMPILER_MSVC 0
// #endif
// #if !defined(COMPILER_LLVM)
// #define COMPILER_LLVM 0
// #endif
// #if !COMPILER_MSVC && !COMPILER_LLVM
// #if _MSC_VER
// #undef COMPILER_MSVC
// #define COMPILER_MSVC 1
// #else
// // TODO(casey): Moar compilerz!!!
// #undef COMPILER_LLVM
// #define COMPILER_LLVM 1
// #endif
// #endif
// #if COMPILER_MSVC
// #include <intrin.h>
// #elif COMPILER_LLVM
// #include <x86intrin.h>
// #else
// #error SEE/NEON optimizations are not available for this compiler yet!!!!
// #endif
// #endif

//
// TODO(llins): add support for platforms that are not linux
//
#include <x86intrin.h>


static inline u64
pt_get_cpu_clock()
{
	return __rdtsc();
}

static inline u64
pt_atomic_sub_u64(u64 volatile *data, u64 decrement)
{
	u64 result = __sync_fetch_and_sub(data, decrement);
	return result;
}

static inline u64
pt_atomic_add_u64(u64 volatile *data, u64 increment)
{
	u64 result = __sync_fetch_and_add(data, increment);
	return result;
}

static inline u64
pt_atomic_exchange_u64(u64 volatile *data, u64 new)
{
	u64 result = __sync_lock_test_and_set(data, new);
	return result;
}

static inline u32
pt_atomic_sub_u32(u32 volatile *data, u32 decrement)
{
	u32 result = __sync_fetch_and_sub(data, decrement);
	return result;
}

static inline u32
pt_atomic_add_u32(u32 volatile *data, u32 increment)
{
	u32 result = __sync_fetch_and_add(data, increment);
	return result;
}

static inline u16
pt_atomic_add_u16(u16 volatile *data, u16 decrement)
{
	u32 result = __sync_fetch_and_add(data, decrement);
	return result;
}

static inline u16
pt_atomic_sub_u16(u16 volatile *data, u16 decrement)
{
	u32 result = __sync_fetch_and_sub(data, decrement);
	return result;
}


static inline u32
pt_atomic_exchange_u32(u32 volatile *data, u32 new)
{
	u32 result = __sync_lock_test_and_set(data, new);
	return result;
}

/*
 * if old val is not the same as the returned value than nothing happened
 * otherwise the new value is in the data location
 */
static inline u64
pt_atomic_cmp_and_swap_u64(u64 volatile *data, u64 old_val, u64 new_val)
{
	return __sync_val_compare_and_swap_8(data, old_val, new_val);
}

static inline void
pt_memory_barrier()
{
	__sync_synchronize();
}

static inline u32
pt_get_thread_id()
{
	u32 thread_id;
#if defined(__APPLE__) && defined(__x86_64__)
	__asm__("mov %%gs:0x00,%0" : "=r"(thread_id));
#elif defined(__i386__)
	asm("mov %%gs:0x08,%0" : "=r"(thread_id));
#elif defined(__x86_64__)
	__asm__("mov %%fs:0x10,%0" : "=r"(thread_id));
#else
#error Unsupported architecture
#endif
	return(thread_id);
}



typedef __m128 f32x4;

#define f32x4_at(a,i) ((f32*)&a)[i]

static f32x4
f32x4_haddamard(f32x4 a, f32x4 b)
{
	return _mm_mul_ps(a, b);
}

// http://threadlocalmutex.com/?p=8
static f32x4
f32x4_cross_3shuffles(f32x4 a, f32x4 b)
{
	f32x4 a_yzx = _mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 0, 2, 1));
	f32x4 b_yzx = _mm_shuffle_ps(b, b, _MM_SHUFFLE(3, 0, 2, 1));
	f32x4 c = _mm_sub_ps(_mm_mul_ps(a, b_yzx), _mm_mul_ps(a_yzx, b));
	return _mm_shuffle_ps(c, c, _MM_SHUFFLE(3, 0, 2, 1));
}

static f32x4
f32x4_cross_4shuffles(f32x4 a, f32x4 b)
{
	f32x4 a_yzx = _mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 0, 2, 1));
	f32x4 a_zxy = _mm_shuffle_ps(a, a, _MM_SHUFFLE(3, 1, 0, 2));
	f32x4 b_zxy = _mm_shuffle_ps(b, b, _MM_SHUFFLE(3, 1, 0, 1));
	f32x4 b_yzx = _mm_shuffle_ps(b, b, _MM_SHUFFLE(3, 0, 2, 1));
	return _mm_sub_ps(_mm_mul_ps(a_yzx, b_zxy), _mm_mul_ps(a_zxy, b_yzx));
}

//------------------------------------------------------------------------------
// MemoryBlock
//------------------------------------------------------------------------------

typedef struct {
	char *begin;
	char *end;
} MemoryBlock;

//------------------------------------------------------------------------------
// LinearAllocator
//------------------------------------------------------------------------------

typedef struct {
	char *begin;
	char *end;
	char *capacity;
} LinearAllocator;

typedef struct {
	char *checkpoint;
} LinearAllocatorCheckpoint;

//------------------------------------------------------------------------------
// BilinearAllocator
//------------------------------------------------------------------------------

typedef struct {
	char *begin;
	char *end_left;
	char *end_right;
	char *capacity;
} BilinearAllocator;

typedef struct {
	char *checkpoint;
	b8   left;
} BilinearAllocatorCheckpoint;

//------------------------------------------------------------------------------
// PlatformMemory
//------------------------------------------------------------------------------

#define pt_Memory_FLAG_CHECK_UNDERFLOW 0x1
#define pt_Memory_FLAG_CHECK_OVERFLOW  0x2

typedef struct pt_Memory pt_Memory;

// By design pt_Memory base pointer should always start at
// a page boundary and have a size that is a multiple of
// the pagesize. In this way we can check for small
// overflow and underflow.
struct pt_Memory{
	u8 *base;
	u64 size;

        // #define pt_Memory_FLAG_PROTECT_OVERFLOW  0x1
        // #define pt_Memory_FLAG_PROTECT_UNDERFLOW 0x2
	u64 flags;

	// field initialized to zero. might be used by an arena
	// to keep track of which offset is free for new data
	// to come in
	u64 used;

	// This field is initialized to zero every time the platform
	// allocates memory and its semantics will be defined by
	// the allocation. For Example: memory arenas might want to
	// link previous memory blocks together to be able to release all
	// the memory it allocated.
	pt_Memory   *prev;
};

//------------------------------------------------------------------------------
// PlatformMemory
//------------------------------------------------------------------------------

typedef struct {
	void*       handle;
} pt_Mutex;

//------------------------------------------------------------------------------
// pt_File
//------------------------------------------------------------------------------

typedef struct {
	u64         open: 1;
	u64         eof: 1;
	u64         last_seek_success: 1;
	u64         read: 1;
	u64         write: 1;
	u64         last_read;
	u64         size; // non-zero when open_read_file is called
	void*       handle;
} pt_File;

//------------------------------------------------------------------------------
// pt_MappedFIle
//------------------------------------------------------------------------------

typedef struct {
	u64 mapped:    1;
	u64 unmapped:  1;
	u64 read:      1;
	u64 write:     1;
	u64 size:     60;
	char *begin;
	void *handle;
} pt_MappedFile;

//------------------------------------------------------------------------------
// work queue
//------------------------------------------------------------------------------

typedef struct pt_WorkQueue pt_WorkQueue;

//------------------------------------------------------------------------------
// platform server abstraction
//------------------------------------------------------------------------------

// the status when creating a listen port or a client socket
// to connect to a server use this numbers.
#define pt_TCP_FEEDBACK_ACTIVATING 0
#define pt_TCP_FEEDBACK_OK 1
#define pt_TCP_FEEDBACK_ERROR -1

#define pt_TCP_LISTEN_SOCKET 1
#define pt_TCP_CLIENT_SOCKET 2
#define pt_TCP_SERVER_SOCKET 3

#define pt_TCP_READ  0x1
#define pt_TCP_WRITE 0x2
#define pt_TCP_READ_WRITE (pt_TCP_READ + pt_TCP_WRITE)

typedef struct {
	u64   id; // unique ID for the same tcp engine throughout the whole
	          // execution, unless we could count to u64 and this would
		  // cycle (many many many many many years)
// 	s32   type;
// 	void* user_data;
	void* handle; // this handle should be always available
	              // even if it is a different thing all together.
		      // the way to detect a socket that was already
		      // closed is that the ID doens't match the
		      // lower level ID anymore
} pt_TCP_Socket;

typedef struct {
	void *handle;
} pt_TCP;

typedef struct {
	s32           status;
	pt_TCP_Socket socket;
} pt_TCP_Feedback;

//
// incoming data through a tcp socket is available
//
#define PLATFORM_TCP_DATA_CALLBACK(name) void name(pt_TCP_Socket socket, char *buffer, u64 length)
typedef PLATFORM_TCP_DATA_CALLBACK(PlatformTCPDataCallback);

#define pt_TCP_EVENT_SERVER_SOCKET_INITIALIZATION 0xf1
#define pt_TCP_EVENT_SERVER_SOCKET_TERMINATION    0xf2

//
// the event callback is used to to signal creation and destruction
// of server sockets to a client app, note that on creationg we can
// use the s32 return code to indicate a problem with resources.
//
#define PLATFORM_TCP_EVENT_CALLBACK(name) void name(pt_TCP_Socket socket, s32 event, void *context)
typedef PLATFORM_TCP_EVENT_CALLBACK(PlatformTCPEventCallback);

//------------------------------------------------------------------------------
// Platform API
//------------------------------------------------------------------------------

// memory

#define PLATFORM_TOTAL_ALLOCATED_MEMORY(name) u64 name()
typedef PLATFORM_TOTAL_ALLOCATED_MEMORY(PlatformTotalAllocatedMemory);

#define PLATFORM_ALLOCATE_MEMORY(name) pt_Memory* name(u64 size, u64 flags)
typedef PLATFORM_ALLOCATE_MEMORY(PlatformAllocateMemory);

#define PLATFORM_FREE_MEMORY(name) void name(pt_Memory *memory)
typedef PLATFORM_FREE_MEMORY(PlatformFreeMemory);

#define PLATFORM_ALLOCATE_MEMORY_RAW(name) void* name(u64 size, u64 flags)
typedef PLATFORM_ALLOCATE_MEMORY_RAW(PlatformAllocateMemoryRaw);

#define PLATFORM_FREE_MEMORY_RAW(name) void name(void *raw)
typedef PLATFORM_FREE_MEMORY_RAW(PlatformFreeMemoryRaw);

//
// @todo deprecate this resize memory thing
//
#define PLATFORM_RESIZE_MEMORY(name) b8 name(pt_Memory *memory, u64 new_size)
typedef PLATFORM_RESIZE_MEMORY(PlatformResizeMemory);

#define PLATFORM_COPY_MEMORY(name) void name(void *dest, void *src, u64 count)
typedef PLATFORM_COPY_MEMORY(PlatformCopyMemory);

// file handling

#define pt_FILE_READ    0x1
#define pt_FILE_WRITE   0x2
#define pt_FILE_APPEND  0x4

#define PLATFORM_OPEN_FILE(name) pt_File name(const char* file_begin, const char* file_end, s32 mode)
typedef PLATFORM_OPEN_FILE(PlatformOpenFile);

#define PLATFORM_READ_NEXT_FILE_CHUNK(name) void name(pt_File *pfh, char *buffer_begin, char* buffer_end)
typedef PLATFORM_READ_NEXT_FILE_CHUNK(PlatformReadNextFileChunk);

#define PLATFORM_SEEK_FILE(name) void name(pt_File *pfh, u64 offset)
typedef PLATFORM_SEEK_FILE(PlatformSeekFile);

#define PLATFORM_CLOSE_FILE(name) void name(pt_File *pfh)
typedef PLATFORM_CLOSE_FILE(PlatformCloseFile);

#define PLATFORM_WRITE_TO_FILE(name) b8 name(pt_File *pfh, char *begin, char* end)
typedef PLATFORM_WRITE_TO_FILE(PlatformWriteToFile);

// returns 1 if success else return 0
// length should contain the size of the buffer at input
// at output it contains the length of the path regardless of success
#define PLATFORM_EXECUTABLE_PATH(name) s32 name(char *buffer, u32 *length)
typedef PLATFORM_EXECUTABLE_PATH(PlatformExecutablePath);

#define PLATFORM_RESIZE_FILE(name) b8 name(char* file_begin, char* file_end, u64 new_size)
typedef PLATFORM_RESIZE_FILE(PlatformResizeFile);

// tcp

#define PLATFORM_TCP_CREATE(name) pt_TCP name()
typedef PLATFORM_TCP_CREATE(PlatformTCPCreate);

#define PLATFORM_TCP_DESTROY(name) void name(pt_TCP tcp)
typedef PLATFORM_TCP_DESTROY(PlatformTCPDestroy);

#define PLATFORM_TCP_LISTEN(name) void name(pt_TCP tcp, s32 port, u16 max_connections, void *user_data, PlatformTCPDataCallback *data_callback, \
					    PlatformTCPEventCallback *event_callback, void *context_event_callback, pt_TCP_Feedback *feedback)
typedef PLATFORM_TCP_LISTEN(PlatformTCPListen);

#define PLATFORM_TCP_CONNECT(name) void name(pt_TCP tcp, s32 port, char *hostname, void *user_data, PlatformTCPDataCallback *data_callback, pt_TCP_Feedback *feedback)
typedef PLATFORM_TCP_CONNECT(PlatformTCPConnect);

//
// return 0 if write didn't go smoothly
//
#define PLATFORM_TCP_WRITE(name) s32 name(pt_TCP_Socket socket, void *buffer, s64 length)
typedef PLATFORM_TCP_WRITE(PlatformTCPWrite);

#define PLATFORM_TCP_CLOSE_SOCKET(name) void name(pt_TCP_Socket socket)
typedef PLATFORM_TCP_CLOSE_SOCKET(PlatformTCPCloseSocket);

//
// Figure out if a socket is still valid. Lower level events
// might have handled the socket invalid (communication was
// closed by the other side)
//
// returns either 0,1,2,3 which directions are still open
//
#define PLATFORM_TCP_SOCKET_STATUS(name) s32 name(pt_TCP_Socket socket)
typedef PLATFORM_TCP_SOCKET_STATUS(PlatformTCPSocketOK);

#define PLATFORM_TCP_SOCKET_SET_CUSTOM_DATA(name) void name(pt_TCP_Socket socket, void *custom_data)
typedef PLATFORM_TCP_SOCKET_SET_CUSTOM_DATA(PlatformTCPSocketSetCustomData);

#define PLATFORM_TCP_SOCKET_GET_CUSTOM_DATA(name) void *name(pt_TCP_Socket socket)
typedef PLATFORM_TCP_SOCKET_GET_CUSTOM_DATA(PlatformTCPSocketGetCustomData);

// process current events (if incoming data on client and server sockets,
// use the work queue to dispatch the data)
#define PLATFORM_TCP_PROCESS_EVENTS(name) void name(pt_TCP tcp, pt_WorkQueue *work_queue)
typedef PLATFORM_TCP_PROCESS_EVENTS(PlatformTCPProcessEvents);


// memory mapped files

#define PLATFORM_OPEN_MMAP_FILE(name) pt_MappedFile name(const char* file_begin, const char* file_end, b8 read, b8 write)
typedef PLATFORM_OPEN_MMAP_FILE(PlatformOpenMMapFile);

#define PLATFORM_CLOSE_MMAP_FILE(name) void name(pt_MappedFile *mapped_file)
typedef PLATFORM_CLOSE_MMAP_FILE(PlatformCloseMMapFile);

// multi-threading

#define PLATFORM_WORK_QUEUE_CALLBACK(name) void name(pt_WorkQueue *queue, void *data)
typedef PLATFORM_WORK_QUEUE_CALLBACK(PlatformWorkQueueCallback);

#define PLATFORM_WORK_QUEUE_ADD_ENTRY(name) void name(pt_WorkQueue *queue, PlatformWorkQueueCallback *callback, void *data)
typedef PLATFORM_WORK_QUEUE_ADD_ENTRY(PlatformWorkQueueAddEntry);

#define PLATFORM_WORK_QUEUE_COMPLETE_ALL_WORK(name) void name(pt_WorkQueue *queue)
typedef PLATFORM_WORK_QUEUE_COMPLETE_ALL_WORK(PlatformWorkQueueCompleteAllWork);

// non-blocking call that creates OS specific "threads" and hooks
// them up with the queue
#define PLATFORM_WORK_QUEUE_CREATE(name) pt_WorkQueue *name(u32 num_threads)
typedef PLATFORM_WORK_QUEUE_CREATE(PlatformWorkQueueCreate);

// blocking call sends a signal to all threads hooked to a previously
// started work queue to stop. Blocks until all threads are stopped.
#define PLATFORM_WORK_QUEUE_DESTROY(name) void name(pt_WorkQueue *queue)
typedef PLATFORM_WORK_QUEUE_DESTROY(PlatformWorkQueueDestroy);

#define PLATFORM_WORK_QUEUE_FINISHED(name) b8 name(pt_WorkQueue *queue)
typedef PLATFORM_WORK_QUEUE_FINISHED(PlatformWorkQueueFinished);

#define PLATFORM_GET_THREAD_INDEX(name) u64 name()
typedef PLATFORM_GET_THREAD_INDEX(PlatformGetThreadIndex);

#define PLATFORM_THREAD_SLEEP(name) void name(u64 millisecs)
typedef PLATFORM_THREAD_SLEEP(PlatformThreadSleep);

// measurements

#define PLATFORM_CYCLE_COUNT_CPU(name) u64 name()
typedef PLATFORM_CYCLE_COUNT_CPU(PlatformCycleCountCPU);

#define PLATFORM_GET_TIME(name) f64 name()
typedef PLATFORM_GET_TIME(PlatformGetTime);

// mutex

#define PLATFORM_CREATE_MUTEX(name) pt_Mutex name()
typedef PLATFORM_CREATE_MUTEX(PlatformCreateMutex);

#define PLATFORM_LOCK_MUTEX(name) void name(pt_Mutex mutex)
typedef PLATFORM_LOCK_MUTEX(PlatformLockMutex);

#define PLATFORM_UNLOCK_MUTEX(name) void name(pt_Mutex mutex)
typedef PLATFORM_UNLOCK_MUTEX(PlatformUnlockMutex);

#define PLATFORM_RELEASE_MUTEX(name) void name(pt_Mutex mutex)
typedef PLATFORM_RELEASE_MUTEX(PlatformReleaseMutex);

#define PLATFORM_MEMORY_COPY(name) void name(void *dst, void *src, u64 length)
typedef PLATFORM_MEMORY_COPY(PlatformMemoryCopy);

#define PLATFORM_MEMORY_MOVE(name) void name(void *dst, void *src, u64 length)
typedef PLATFORM_MEMORY_MOVE(PlatformMemoryMove);

#define PLATFORM_MEMORY_COMPARE(name) s32 name(void *a, void *b, u64 length)
typedef PLATFORM_MEMORY_COMPARE(PlatformMemoryCompare);

#define PLATFORM_MEMORY_SET(name) void name(void *dst, s32 byte_value, u64 length)
typedef PLATFORM_MEMORY_SET(PlatformMemorySet);

// platform access to get a list of filenames from a directory

// #define pt_FILE_TYPE_UNDEFINED    0
// #define pt_FILE_TYPE_REGULAR_FILE 1
// #define pt_FILE_TYPE_DIRECTORY    2
// #define pt_FILE_TYPE_LINK         4
// #define pt_FILE_TYPE_OTHER        8
// #define pt_FILE_TYPE_ALL          (1+2+4+8)

#define PLATFORM_GET_FILENAMES_IN_DIRECTORY_CALLBACK(name) void name(char *filename, void *user_data)
typedef PLATFORM_GET_FILENAMES_IN_DIRECTORY_CALLBACK(PlatformGetFilenamesInDirectoryCallback);

//
// @note maybe we someday need recursive traversal, for now a single folder will do it
//
// if return is 0, then directory could not be accessed
// otherwise it performed successfully
// set the input types to be file and directory or just directory of just files
//
#define PLATFORM_GET_FILENAMES_IN_DIRECTORY(name) s32 name(char *directory, s32 recursive, PlatformGetFilenamesInDirectoryCallback *callback, void *user_data)
typedef PLATFORM_GET_FILENAMES_IN_DIRECTORY(PlatformGetFilenamesInDirectory);

#define PLATFORM_SORT_COMPARE(name) s32 name(const void *a, const void *b, void *context)
typedef PLATFORM_SORT_COMPARE(PlatformSortCompare);

#define PLATFORM_SORT(name) void name(void *data, u64 count, u64 size, PlatformSortCompare *cmp, void *context)
typedef PLATFORM_SORT(PlatformSort);

#define PLATFORM_FILENAME_MATCH(name) s32 name(const char *pattern, const char *text)
typedef PLATFORM_FILENAME_MATCH(PlatformFilenameMatch);


#define PLATFORM_GETENV(name) char* name(void *variable_name)
typedef PLATFORM_GETENV(PlatformGetEnv);

#define PLATFORM_FAILED_ASSERTION(name) char* name(const char *expression, const char *filename, s32 line)
typedef PLATFORM_FAILED_ASSERTION(PlatformFailedAssertion);

typedef struct PlatformAPI {
	PlatformTotalAllocatedMemory     *total_allocated_memory;

	PlatformAllocateMemory           *allocate_memory;
	PlatformFreeMemory               *free_memory;

	PlatformAllocateMemoryRaw        *allocate_memory_raw;
	PlatformFreeMemoryRaw            *free_memory_raw;

	PlatformResizeMemory             *resize_memory;
	PlatformCopyMemory               *copy_memory;
	PlatformOpenFile                 *open_file;
	PlatformWriteToFile              *write_to_file;
	PlatformReadNextFileChunk        *read_next_file_chunk;
	PlatformSeekFile                 *seek_file;
	PlatformCloseFile                *close_file;
	PlatformExecutablePath           *executable_path;

	PlatformTCPCreate                *tcp_create;
	PlatformTCPDestroy               *tcp_destroy;
	PlatformTCPListen                *tcp_listen;
	PlatformTCPWrite                 *tcp_write;
	PlatformTCPProcessEvents         *tcp_process_events;
	PlatformTCPConnect               *tcp_connect;
	PlatformTCPCloseSocket           *tcp_close_socket;
	PlatformTCPSocketOK              *tcp_socket_status;
	PlatformTCPSocketSetCustomData   *tcp_socket_set_custom_data;
	PlatformTCPSocketGetCustomData   *tcp_socket_get_custom_data;

	PlatformGetTime                  *get_time;
	PlatformOpenMMapFile             *open_mmap_file;
	PlatformCloseMMapFile            *close_mmap_file;
	PlatformResizeFile               *resize_file;
	PlatformCycleCountCPU            *cycle_count_cpu;

	PlatformWorkQueueCreate          *work_queue_create;
	PlatformWorkQueueDestroy         *work_queue_destroy;
	PlatformWorkQueueAddEntry        *work_queue_add_entry;
	PlatformWorkQueueCompleteAllWork *work_queue_complete_work;
	PlatformWorkQueueFinished        *work_queue_finished;

	PlatformGetThreadIndex           *get_thread_index;
	PlatformThreadSleep              *thread_sleep;

	// mutex
	PlatformCreateMutex              *create_mutex;
	PlatformReleaseMutex             *release_mutex;
	PlatformUnlockMutex              *unlock_mutex;
	PlatformLockMutex                *lock_mutex;

	PlatformMemoryCopy               *memory_copy;
	PlatformMemoryMove               *memory_move;
	PlatformMemorySet                *memory_set;
	PlatformMemoryCompare            *memory_compare;

	PlatformGetFilenamesInDirectory  *get_filenames_in_directory;

	PlatformSort                     *sort;

	PlatformFilenameMatch            *filename_match;

	PlatformGetEnv                   *getenv;

	PlatformFailedAssertion          *failed_assertion;

} PlatformAPI;

/*
 * a static platform should be available
 */
global_variable PlatformAPI platform;

#if CHECK_ASSERTIONS
#define Assert(EX) (void) ((EX) || (platform.failed_assertion(#EX, __FILE__, __LINE__),0))
#else
#define Assert(Expression)
#endif
#define InvalidCodePath Assert(!"InvalidCodePath")
#define InvalidDefaultCase default: {InvalidCodePath;} break


#define StaticAssertAlignment_st_(a) #a
#define StaticAssertAlignment_cat_(a,b,c) StaticAssertAlignment_st_(a ## b ## c)
#define StaticAssertAlignment(type,alignment) \
	_Static_assert(sizeof(type) % alignment == 0, StaticAssertAlignment_cat_(type,_size_is_not_multiple_of_,alignment))

/*
 * functions
 */
static f64
pt_nan_f64()
{
	return NAN;
}

static b8
pt_is_nan_f64(f64 value)
{
	return (b8) (value != value);
}

static inline u8
pt_safe_s32_u8(s32 i)
{
	Assert(i >= 0 && i < 256);
	return (u8) i;
}

static inline u8
pt_safe_s64_u8(s64 i)
{
	Assert(i >= 0 && i < 256);
	return (u8) i;
}

static inline u32
pt_safe_s64_u32(s64 i)
{
	Assert(i >= 0 && i < 0x100000000ll);
	return (u32) i;
}

static inline u16
pt_safe_s64_u16(s64 i)
{
	Assert(i >= 0 && i < 0x10000ll);
	return (u16) i;
}

static inline u32
pt_safe_s32_u32(s32 i)
{
	Assert(i >= 0);
	return (u32) i;
}

static inline u64
pt_safe_s64_u64(s64 i)
{
	Assert(i >= 0);
	return (u64) i;
}

static inline u64
pt_next_multiple(u64 value, u64 base)
{
	return base * ((value + base - 1)/base);
}

// @todo convention here is different than the stdlib
static void
pt_copy_bytesn(const char* src, char* dst, u64 n)
{
	for (u64 i=0;i<n;++i) {
		*dst++ = *src++;
	}
}

static s32
pt_copy_bytes(const char* src_begin, const char* src_end, char* dst_begin, char* dst_end)
{
	//
	// @TODO(llins): make it more efficient by copying longer
	//               inputs using u64 pointers
	//
	Assert(dst_begin <= dst_end && src_begin <= src_end);
	s32 n = 0;
	while (src_begin != src_end && dst_begin != dst_end) {
		*dst_begin = *src_begin;
		++dst_begin;
		++src_begin;
		++n;
	}
	return n;
}

inline static void
pt_fill(char *begin, char *end, char ch)
{
	while (begin != end) {
		*begin = ch;
		++begin;
	}
}

inline static void
pt_filln(char *begin, u64 count, char ch)
{
	for (u64 i=0;i<count;++i) {
		*begin = ch;
		++begin;
	}
}

inline static void
pt_zero(void *base, u64 size)
{
	// @cleanup use the memset platform method
	u8 *it = (u8*) base;
	for (u64 i=0;i<size;++i) {
		*it = 0;
		++it;
	}
}

#define pt_clear(x) pt_zero(&x, sizeof(x))

static void
pt_reverse(char *begin, char *end)
{
	Assert(begin <= end);
	if (begin == end)
		return;
	char *left  = begin;
	char *right = end-1;
	while (left < right) {
		char ch = *left;
		*left = *right;
		*right = ch;
		++left;
		--right;
	}
}




//
// following the same spec as std algorithm library
// [begin,nbegin) and [nbegin,end) are valid ranges
//
// #define SWAP(x,y) do
//    { unsigned char swap_temp[sizeof(x) == sizeof(y) ? (s32)sizeof(x) : -1];
//      memcpy(swap_temp,&y,sizeof(x));
//      memcpy(&y,&x,       sizeof(x));
//      memcpy(&x,swap_temp,sizeof(x));
//     } while(0)

#define pt_ROTATE(type,begin,nbegin,end) \
{ \
	if (nbegin != end)  { \
		type *next = nbegin; \
		while (begin != next) {  \
			type tmp = *begin; \
			*begin   = *next; \
			*next    = tmp; \
			++begin; \
			++next; \
			if (next == end) { \
				next = nbegin; \
			} else if (begin == nbegin) { \
				nbegin = next; \
			} \
		} \
	} \
}

//
// following the same spec as std algorithm library
// [begin,nbegin) and [nbegin,end) are valid ranges
//
static void
pt_rotate(char *begin, char *nbegin, char *end)
{
	if (nbegin == end)
		return;
	char* next = nbegin;
	while (begin != next) {
		char tmp = *begin;
		*begin   = *next;
		*next    = tmp;

		++begin;
		++next;

		if (next == end) {
			next = nbegin;
		} else if (begin == nbegin) {
			nbegin = next;
		}
	}
}

#define ROTATE_SPECIALIZED(name, base)     \
	static void                              \
name(base *begin, base *nbegin, base *end) \
{                                          \
	base* next = nbegin;                   \
	while (begin != next) {                \
		base tmp = *begin;                 \
		*begin   = *next;                  \
		*next    = tmp;                    \
		++begin;                           \
		++next;                            \
		if (next == end) {                 \
			next = nbegin;                 \
		} else if (begin == nbegin) {      \
			nbegin = next;                 \
		}                                  \
	}                                      \
}

static u32
pt_msb32(u32 x)
{
	static const unsigned int bval[] =
	{ 0,1,2,2,3,3,3,3,4,4,4,4,4,4,4,4 };
	unsigned int r = 0;
	if (x & 0xFFFF0000) { r += 16 / 1; x >>= 16 / 1; }
	if (x & 0x0000FF00) { r += 16 / 2; x >>= 16 / 2; }
	if (x & 0x000000F0) { r += 16 / 4; x >>= 16 / 4; }
	return r + bval[x];
}

// Normalize on the two most significant bits
static u32
pt_normalize_msb2(u32 num_bytes) {
	// pf_BEGIN_BLOCK("pt_normalize_msb2");
	static const u32 class_bits = 2;
	u32 nobits = pt_msb32(num_bytes);
	u32 result;
	if (nobits > class_bits) {
		u32 class_shift = nobits - class_bits;
		u32 remainder = num_bytes % (1u << class_shift);
		result = ((num_bytes >> class_shift) + (remainder > 0u)) << class_shift;
	} else {
		result = num_bytes;
	}
	// pf_END_BLOCK();
	return result;
}

//
// Normalize on the three most significant bits
// |  x | msb2(x) | msb3(x) |
// |----+---------+---------|
// |  1 |       1 |       1 |
// |  2 |       2 |       2 |
// |  3 |       3 |       3 |
// |  4 |       4 |       4 |
// |  5 |       6 |       5 |
// |  6 |       6 |       6 |
// |  7 |       8 |       7 |
// |  8 |       8 |       8 |
// |  9 |      12 |      10 |
// | 10 |      12 |      10 |
// | 11 |      12 |      12 |
// | 12 |      12 |      12 |
// | 13 |      16 |      14 |
// | 14 |      16 |      14 |
// | 15 |      16 |      16 |
// | 16 |      16 |      16 |
//
//    static unsigned int normalize_msb3(unsigned int num_bytes) {
//        static const unsigned int class_bits = 3;
//        auto nobits = msb32(num_bytes);
//        if (nobits > class_bits) {
//            auto class_shift = nobits - class_bits;
//            auto remainder   = num_bytes % (1u << class_shift);
//            return ((num_bytes >> class_shift) + (remainder > 0u)) << class_shift;
//        }
//        else {
//            return num_bytes;
//        }
//    }


// offset on the input
#if 0
static void
pt_read_bits(const char* input, u32 offset, u32 bits, char* output)
{
	// pf_BEGIN_BLOCK("pt_read_bits");

	// align a 64 bits unsigned int with input array
	u64* src = (u64*) (input + (offset / 8));
	u64           src_bitoff = offset & 0x7u; // read
	//
	// if src_bitoff == 3 then
	//      mask_left  == 1110000...000000 (64 bits)
	//      mask_right == 0001111...111111 (64 bits)
	//
	u64 mask_right = ~0ULL << src_bitoff;   // erase 3 least significant bits and negate
	u64 mask_left = ~mask_right;
	u64 *dst = (u64*) output;
	u64 remaining_bits = bits;
	while (remaining_bits > 0) {
		if (remaining_bits < 64) {
			// copy last bytes from src into a local 64 bit storage space
			u64 src_copy = 0;
			u64 n = (remaining_bits + 7u) / 8u;
			pt_copy_bytes((char*)src, (char*)src + n, (char*)&src_copy, (char*)&src_copy + n);
			// std::copy((char*) src, (char*) src + n, (char*) &src_copy);
			u64 mask_end = ~(mask_left | ~(~0ULL >> (64 - remaining_bits - src_bitoff)));
			src_copy &= mask_end; // shift bit offset (<8)
			src_copy >>= src_bitoff;
			char* p = (char*)&src_copy;
			pt_copy_bytes(p, p + n, (char*)dst, (char*)dst + n);
			// std::copy(p, p + n, (char*) dst);
			remaining_bits = 0;
		} else {
			*dst = src_bitoff ? (*src >> src_bitoff) | ((*(src + 1) & mask_left) << (64 - src_bitoff)) : *src;
			++src;
			++dst; // done writing on this 64-bit block. go to next
			remaining_bits -= 64;
		}
	}

	// pf_END_BLOCK();
}

/* TODO(llins): this is messed up. We need to be careful at the
 *  when we see things don't align well. Using a u64* on a single
 *  byte.
 */
static void
pt_write_bits(const char *input,
	      u32 offset, // offset on the output
	      u32 bits,
	      char * output)
{
	// pf_BEGIN_BLOCK("pt_write_bits");

	// align a byte 64 bit unsigned int on the
	u64* dst = (u64*) (output + (offset / 8));
	u64  dst_bitoff = offset & 0x7u; // which bit offset should I start writing on
	//
	// if dst_bitoff == 3 then
	//      mask_left  == 1110000...000000 (64 bits)
	//      mask_right == 0001111...111111 (64 bits)
	//
	u64 mask_right = ~0ULL << dst_bitoff;   // erase 3 least significant bits and negate
	u64 mask_left = ~mask_right;
	const u64 *src = (const u64*) input;
	//
	// consume the bits in data a chunk of 64 at a time
	// they are all byte aligned
	// when less than 64 bits are available write them
	//
	u64 remaining_bits = bits;
	while (remaining_bits > 0) {
		if (remaining_bits < 64) {
			// copy last bytes from src into a local 64 bit storage space
			u64 src_copy = 0;
			u64 n = (remaining_bits + 7u) / 8u;
			pt_copy_bytes((char*)src, (char*)src + n, (char*)&src_copy, (char*)&src_copy + n);
			// std::copy((char*) src, (char*) src + n, (char*) &src_copy);
			//
			// ones on the extremes 1111 000000000 1111
			// # zeros in the middle is equal to remaining_bits
			//
			u64 mask_end = mask_left | ~(~0ULL >> (64 - remaining_bits - dst_bitoff));
			*dst = (*dst & mask_end) | (src_copy << dst_bitoff);
			remaining_bits = 0;
		} else {
			// assuming we still need to write 64 or more bits
			// from src to dst
			// write lower piece of the data into dst
			*dst = (*dst & mask_left) | (*src << dst_bitoff);
			++dst; // done writing on this 64-bit block. go to next
			// is there a right part of the data
			if (dst_bitoff) {
				// we still have to write dst_bitoff bits in the beginning of
				// this new 64 bit block on dst
				*dst = (*dst & mask_right) | (*src >> (64 - dst_bitoff));
			}
			remaining_bits -= 64;
		}
	}

	// pf_END_BLOCK();
}

#endif

typedef struct {
	u8  *data;
	u64 data_length;
	u64 bit_offset;
} pt_BitStream;

static void
pt_BitStream_init(pt_BitStream *self, void *data, u64 data_length, u64 bit_offset)
{
	self->data = data;
	self->data_length = data_length;
	self->bit_offset = bit_offset;
}

static u8
pt_BitStream_read(pt_BitStream *self, u8 bits)
{
	Assert(bits <= 8);
	static u8 LMASK[] = { 0x0, 0x1, 0x3, 0x7, 0xf, 0x1f, 0x3f, 0x7f, 0xff };

	u8  l           = self->bit_offset % 8;
	u64 byte_offset = self->bit_offset / 8;

	Assert(byte_offset < self->data_length);

	u8 lower_bits   = self->data[byte_offset] >> l;

	self->bit_offset += bits;
	if (l + bits <= 8) {
		return lower_bits & LMASK[bits];
	} else {
		Assert(byte_offset + 1 < self->data_length);
		u8 right_byte_bits = (l + bits) % 8;
		u8 higher_bits = self->data[byte_offset+1] & LMASK[right_byte_bits];
		return lower_bits + (higher_bits << (8-l));
	}
}

//
// base = 0xFF
// print "LMASKS"
// for i in xrange(8):
// mask = base >> (8-i)
// 	print bin(mask)
// 	print hex(mask)
//
// print "RMASKS"
// for i in xrange(8):
// 	mask = (base >> (8-i)) << (8-i)
// 	print bin(mask)
// 	print hex(mask)
//
// LMASKS     RMASKS
// 0b0        0b0
// 0x0        0x0
// 0b1        0b10000000
// 0x1        0x80
// 0b11       0b11000000
// 0x3        0xc0
// 0b111      0b11100000
// 0x7        0xe0
// 0b1111     0b11110000
// 0xf        0xf0
// 0b11111    0b11111000
// 0x1f       0xf8
// 0b111111   0b11111100
// 0x3f       0xfc
// 0b1111111  0b11111110
// 0x7f       0xfe
//

//
// 2019-06-15T11:35
// @TODO there is something weird with the write call on a bitstream
// where should it write? what is the semantics of the fields below
//
// 	u8  *data;
// 	u64 data_length;
// 	u64 bit_offset;
//
// when we call a pt_BitStream_write
//
// note that we don't upload the state of the bitstream's data_length
// or bit_offset, only its data.
//
static void
pt_BitStream_write(pt_BitStream *self, u8 value, u8 bits)
{
	Assert(bits <= 8);
	static u8 LMASK[] = { 0x00, 0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff };
	static u8 RMASK[] = { 0x00, 0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff };

	Assert(value < (1ull << bits));

	u8  l           = self->bit_offset % 8;
	u64 byte_offset = self->bit_offset / 8;
	Assert(byte_offset < self->data_length);

	if (l + bits <= 8) {
		// only one byte is touched
		u8 preserved_lower_bits  = self->data[byte_offset] & LMASK[l];
		u8 preserved_higher_bits = self->data[byte_offset] & RMASK[8-(l+bits)];
		u8 shifted_value         = value << l;
		self->data[byte_offset] = preserved_lower_bits + preserved_higher_bits + shifted_value;
	} else { // if (l + bits > 8) {
		{ // left part
			u8 preserved_lower_bits   = self->data[byte_offset] & LMASK[l];
			u8 shifted_value          = value << l;
			self->data[byte_offset]   = preserved_lower_bits + shifted_value;
		}
		{ // right part
			u8 a = l + bits - 8;
			u8 b = 8 - a;
			u8 preserved_higher_bits  = self->data[byte_offset+1] & RMASK[b];
			u8 shifted_value          = value >> (8-l);
			self->data[byte_offset+1] = shifted_value + preserved_higher_bits;
		}
	}
	// self->data_length += bits;
}

static void
pt_read_bits2(const char* input, u32 offset, u32 bits, char* output)
{
	pt_BitStream src;
	pt_BitStream_init(&src, (void*) input, 1ULL<<63, offset);

	pt_BitStream dst;
	pt_BitStream_init(&dst, (void*) output, 1ULL<<63, 0);

	while (bits >= 8) {
		u8 value = pt_BitStream_read(&src, 8);
		pt_BitStream_write(&dst, value, 8);
		bits -= 8;
	}
	if (bits) {
		u8 value = pt_BitStream_read(&src, bits);
		pt_BitStream_write(&dst, value, bits);
	}
}

//
// copy bits [0,bits-1]              from input
// into bits [offset, offset+bits-1] of   output
//
static void
pt_write_bits2(const char *input, u32 offset, u32 bits, char * output)
{
	pt_BitStream src;
	pt_BitStream_init(&src, (void*) input, 1ULL<<63, 0);

	pt_BitStream dst;
	pt_BitStream_init(&dst, (void*) output, 1ULL<<63, offset);

	while (bits >= 8) {
		u8 value = pt_BitStream_read(&src, 8);
		pt_BitStream_write(&dst, value, 8);
		bits -= 8;
	}
	if (bits) {
		u8 value = pt_BitStream_read(&src, bits);
		pt_BitStream_write(&dst, value, bits);
	}
}

//------------------------------------------------------------------------------
//
// sort
//
// sort using c's stdlib standard mechanism
//
//------------------------------------------------------------------------------

#define PLATFORM_SORT_COMPARE_CALLBACK(name) s32 name(const void *a, const void* b)
typedef PLATFORM_SORT_COMPARE_CALLBACK(pt_SortCompareCallback);

static void
pt_sort(void *base, u64 num_records, u64 record_size, pt_SortCompareCallback *compare)
{
	qsort(base, num_records, record_size, compare);
}

PLATFORM_SORT_COMPARE_CALLBACK(pt_sort_compare_f64)
{
	f64 aa = *((f64*)a);
	f64 bb = *((f64*)b);
	if (aa < bb) {
		return -1;
	} else if (aa > bb) {
		return 1;
	} else {
		return 0;
	}
}

static void
pt_sort_f64(f64 *base, u64 num_records)
{
	qsort(base, num_records, sizeof(f64), pt_sort_compare_f64);
}

//------------------------------------------------------------------------------
// MemoryBlock
//------------------------------------------------------------------------------

static s64
MemoryBlock_length(MemoryBlock *self)
{
	Assert(self->begin < self->end);
	return self->end - self->begin;
}

static MemoryBlock
MemoryBlock_aligned(MemoryBlock block, u64 block_size)
{
	char *begin = (char*) RAlign((u64) block.begin,block_size);
	char *end   = (char*) LAlign((u64) block.end,  block_size);
	if (begin <= end) {
		return (MemoryBlock) { .begin = begin, .end = end };
	} else {
		return (MemoryBlock) { .begin = 0, .end = 0 };
	}
}

static MemoryBlock
MemoryBlock_partition_part(MemoryBlock block, s32 index, s32 parts)
{
	s64 n = block.end - block.begin;
	s64 block_size = n / parts;
	char *begin = block.begin + block_size * index;
	return (MemoryBlock) { .begin = begin, .end = begin + block_size };
}

//------------------------------------------------------------------------------
// Parse integers
//------------------------------------------------------------------------------

static b8
pt_parse_s32(char *begin, char *end, s32 *output)
{
	int result = 0;

	if (begin >= end)
		return 0;

	int sign = 1;
	const char* it = begin;
	if (*it == '-') {
		sign = -1;
		++it;
	} else if (*it == '+') {
		++it;
	}

	while (it != end) {
		char ch = *it;
		if (ch >= '0' && ch <= '9') {
			result = 10 * result + ((int) ch - (int) '0');
		}
		else {
			return 0;
		}
		++it;
	}

	*output = result * sign;
	return 1;

}

static b8
pt_parse_s64(char *begin, char *end, s64 *output)
{
	s64 result = 0;

	if (begin >= end)
		return 0;

	int sign = 1;
	const char* it = begin;
	if (*it == '-') {
		sign = -1;
		++it;
	} else if (*it == '+') {
		++it;
	}

	while (it != end) {
		char ch = *it;
		if (ch >= '0' && ch <= '9') {
			result = 10 * result + ((int) ch - (int) '0');
		}
		else {
			return 0;
		}
		++it;
	}

	*output = result * sign;
	return 1;
}

static b8
pt_parse_u64(char *begin, char *end, u64 *output)
{
	u64 result = 0;

	if (begin >= end)
		return 0;

	const char* it = begin;
	while (it != end) {
		char ch = *it;
		if (ch >= '0' && ch <= '9') {
			result = 10 * result + ((int) ch - (int) '0');
		}
		else {
			return 0;
		}
		++it;
	}

	*output = result;
	return 1;
}

static b8
pt_parse_u32(char *begin, char *end, u32 *output)
{
	u32 result = 0;

	if (begin >= end)
		return 0;

	const char* it = begin;
	while (it != end) {
		char ch = *it;
		if (ch >= '0' && ch <= '9') {
			result = 10 * result + ((int) ch - (int) '0');
		}
		else {
			return 0;
		}
		++it;
	}

	*output = result;
	return 1;
}

static s32
pt_cstr_to_f32(char *cstr, f32 *output)
{
	char *end_ptr = 0;
	*output = strtof(cstr, &end_ptr);
	return (*end_ptr == 0);
}

static s32
pt_cstr_to_s32(char *cstr, s32 *output)
{
	char *end_ptr = 0;
	*output = strtol(cstr, &end_ptr, 10);
	return (*end_ptr == 0);
}

static s32
pt_cstr_to_f64(char *cstr, f64 *output)
{
	char *end_ptr = 0;
	*output = strtod(cstr, &end_ptr);
	return (*end_ptr == 0);
}

static s32
pt_cstr_to_u64(char *cstr, u64 *output)
{
	char *end_ptr = 0;
	*output = strtoull(cstr, &end_ptr, 10);
	return (*end_ptr == 0);
}

static s32
pt_str_to_s32(char *text, u32 len, s32 *output)
{
	if (len > 31) return 0;
	char buffer[32];
	pt_copy_bytesn(text, buffer, len);
	buffer[len] = 0;
	char *end_ptr = 0;
	*output = strtod(buffer, &end_ptr);
	return (*end_ptr == 0);
}

static s32
pt_str_to_u64(char *text, u32 len, u64 *output)
{
	if (len > 31) return 0;
	char buffer[32];
	pt_copy_bytesn(text, buffer, len);
	buffer[len] = 0;
	char *end_ptr = 0;
	*output = strtoull(buffer, &end_ptr, 10);
	return (*end_ptr == 0);
}

static s32
pt_str_to_f32(char *text, u32 len, f32 *output)
{
	if (len > 31) return 0;
	char buffer[32];
	pt_copy_bytesn(text, buffer, len);
	buffer[len] = 0;
	char *end_ptr = 0;
	*output = strtof(buffer, &end_ptr);
	return (*end_ptr == 0);
}

static s32
pt_str_to_f64(char *text, u32 len, f32 *output)
{
	if (len > 31) return 0;
	char buffer[32];
	pt_copy_bytesn(text, buffer, len);
	buffer[len] = 0;
	char *end_ptr = 0;
	*output = strtod(buffer, &end_ptr);
	return (*end_ptr == 0);
}

static s32
pt_u64_from_hex_str(char *text, u32 len, u64 *output)
{
	if (len > 31) return 0;
	char buffer[32];
	pt_copy_bytesn(text, buffer, len);
	buffer[len] = 0;
	char *end_ptr = 0;
	*output = strtoull(buffer, &end_ptr, 16);
	return (*end_ptr == 0);
}

//
// static s32
// parse_s32(char *cstr, s32 *output)
// {
// 	char *end_ptr;
// 	*output = strtol(cstr, &end_ptr, 10);
// 	return (*end_ptr == 0);
// }
//
// static s32
// parse_u64(char *cstr, u64 *output)
// {
// 	char *end_ptr;
// 	*output = strtoull(cstr, &end_ptr, 10);
// 	return (*end_ptr == 0);
// }
//

static b8
pt_parse_f64(char *begin, char *end, f64 *output)
{
	// @TODO(llins): make this more efficient
	// expecting a cstr is not a good idea
	char buffer[32];
	s32 len = pt_copy_bytes(begin, end, buffer, buffer+31);
	buffer[len] = 0;
	*output = atof(buffer);
	return 1; //(errno == 0 ? 1 : 0);
}

static b8
pt_parse_f32(char *begin, char *end, f32 *output)
{
	// @TODO(llins): make this more efficient
	// expecting a cstr is not a good idea
	char buffer[32];
	s32 len = pt_copy_bytes(begin, end, buffer, buffer+31);
	buffer[len] = 0;
	*output = (f32) atof(buffer);
	return 1; // (errno == 0 ? 1 : 0);
}


static char*
pt_find_char(char *begin, char *end, char ch)
{
	char *it = begin;
	while (it != end) {
		if (*it == ch)
			return it;
		++it;
	}
	return end;
}

static void
pt_swap_s32(s32 *a, s32 *b)
{
	s32 aux = *a;
	*a = *b;
	*b = aux;
}

//------------------------------------------------------------------------------
// LinearAllocator
//------------------------------------------------------------------------------

static void
LinearAllocator_init(LinearAllocator* self, char *begin, char *end, char *capacity)
{
	self->begin = begin;
	self->end   = end;
	self->capacity = capacity;
}

static char*
LinearAllocator_alloc(LinearAllocator* self, u64 num_bytes)
{
	Assert(self->end + num_bytes <= self->capacity);
	void *result = self->end;
	self->end += num_bytes;
	return result;
}

static char*
LinearAllocator_alloc_if_available(LinearAllocator* self, u64 num_bytes)
{
	if (self->end + num_bytes > self->capacity) {
		return 0;
	}
	void *result = self->end;
	self->end += num_bytes;
	return result;
}



static u64
LinearAllocator_free_space(LinearAllocator* self)
{
	return (self->capacity - self->end);
}

static char*
LinearAllocator_alloc_aligned(LinearAllocator* self, u64 num_bytes, u64 base_bytes)
{
	s64 offset = 0;
	u64 remainder = ((u64) self->end) % base_bytes;
	if (remainder > 0)
	{
		offset = base_bytes - remainder;
	}
	Assert(self->end + offset + num_bytes <= self->capacity);
	void *result = self->end + offset;
	self->end += offset + num_bytes;
	return result;
}

static void
LinearAllocator_clear(LinearAllocator* self)
{
	self->end = self->begin;
}

static void
LinearAllocator_pop(LinearAllocator* self, u64 num_bytes)
{
	Assert(self->begin + num_bytes <= self->end);
	self->end -= num_bytes;
}

static LinearAllocatorCheckpoint
LinearAllocator_checkpoint(LinearAllocator *self)
{
	LinearAllocatorCheckpoint result;
	result.checkpoint = self->end;
	return result;
}

static void
LinearAllocator_rewind(LinearAllocator *self, LinearAllocatorCheckpoint cp)
{
	Assert(cp.checkpoint <= self->end);
	self->end = cp. checkpoint;
}


//------------------------------------------------------------------------------
// BilinearAllocator
//------------------------------------------------------------------------------

static void
BilinearAllocator_init(BilinearAllocator* self, void *buffer, u64 buffer_size)
{
	// [begin,end_left)
	// [end_right,capcity)
	self->begin = buffer;
	self->end_left = buffer;
	self->end_right = (char*) buffer + buffer_size;
	self->capacity = self->end_right;
}

static char*
BilinearAllocator_alloc_left(BilinearAllocator* self, u64 num_bytes)
{
	Assert(self->end_left + num_bytes <= self->end_right);
	void *result = self->end_left;
	self->end_left += num_bytes;
	return result;
}

static char*
BilinearAllocator_alloc_right(BilinearAllocator* self, u64 num_bytes)
{
	// state is and num bytes is 4
	// b * * * * l  * * * r * * * * c
	// result
	// b * * * * lr * * * * * * * * c
	Assert(self->end_left + num_bytes <= self->end_right);
	self->end_right-= num_bytes;
	return self->end_right;
}

static MemoryBlock
BilinearAllocator_free_memblock(BilinearAllocator* self)
{
	return (MemoryBlock) { .begin = self->end_left, .end = self->end_right };
}

static u64
BilinearAllocator_free_space(BilinearAllocator* self)
{
	return (self->end_right - self->end_left);
}

static char*
BilinearAllocator_alloc_left_aligned(BilinearAllocator* self, u64 num_bytes, u64 base_bytes)
{
	s64 offset = 0;
	u64 remainder = ((u64) self->end_left) % base_bytes;
	if (remainder > 0) {
		offset = base_bytes - remainder;
	}
	Assert(self->end_left + offset + num_bytes <= self->capacity);
	void *result = self->end_left + offset;
	self->end_left += offset + num_bytes;
	return result;
}

// TODO(llins)
// BilinearAllocator_alloc_right_aligned(BilinearAllocator* self, u64 num_bytes, u64 base_bytes)

static void
BilinearAllocator_clear(BilinearAllocator* self)
{
	self->end_left  = self->begin;
	self->end_right = self->capacity;
}

static void
BilinearAllocator_clear_left(BilinearAllocator* self)
{
	self->end_left  = self->begin;
}

static void
BilinearAllocator_clear_right(BilinearAllocator* self)
{
	self->end_right  = self->capacity;
}

static void
BilinearAllocator_pop_left(BilinearAllocator* self, u64 num_bytes)
{
	Assert(self->begin + num_bytes <= self->end_left);
	self->end_left -= num_bytes;
}

static void
BilinearAllocator_pop_right(BilinearAllocator* self, u64 num_bytes)
{
	Assert(self->end_right + num_bytes <= self->capacity);
	self->end_right += num_bytes;
}


static BilinearAllocatorCheckpoint
BilinearAllocator_left_checkpoint(BilinearAllocator *self)
{
	BilinearAllocatorCheckpoint result;
	result.checkpoint = self->end_left;
	result.left = 1;
	return result;
}

static BilinearAllocatorCheckpoint
BilinearAllocator_right_checkpoint(BilinearAllocator *self)
{
	BilinearAllocatorCheckpoint result;
	result.checkpoint = self->end_right;
	result.left = 0;
	return result;
}

static void
BilinearAllocator_rewind(BilinearAllocator *self, BilinearAllocatorCheckpoint cp)
{
	if (cp.left) {
		Assert(self->begin <= cp.checkpoint);
		Assert(cp.checkpoint <= self->end_right);
		self->end_left = cp.checkpoint;
	} else {
		Assert(self->end_left <= cp.checkpoint);
		Assert(cp.checkpoint <= self->capacity);
		self->end_right = cp.checkpoint;
	}
}


//----------------------------------------------------------
// TESTS
//----------------------------------------------------------

#ifdef platform_UNIT_TEST

#include <stdlib.h>
#include <stdio.h>

void test_pt_write_bits2()
{
	u8 buffer[] = { 0xA, 0xB, 0x99};
	s32 target = 0x87654321;
	printf("target before: %x\n", target);
// 	pt_write_bits2((char*) buffer,      4, 4, (char*) &target);
// 	pt_write_bits2((char*) buffer + 1,  8, 4, (char*) &target);
	pt_write_bits2((char*) buffer + 2, 12, 8, (char*) &target);
	printf("target after:  %x\n", target);
}

int main(int argc, char *argv[])
{
	test_pt_write_bits2();
}


// // Use bash script utest.sh
//
// #include <stdlib.h>
// #include <stdio.h>
//
// int main(int argc, char *argv[])
// {
// 	char buffer[256];
// 	Print print;
// 	Print_init(&print, buffer, buffer + sizeof(buffer));
// 	{
// 		u8 buffer[] = { 0, 2 };
// 		u64 target = 0;
// 		pt_write_bits((char*) buffer,     0, 7, (char*) &target);
// 		pt_write_bits((char*) buffer + 1, 7, 7, (char*) &target);
// 		Print_clear(&print);
// 		Print_u64(&print, target);
// 		Print_char(&print, '-');
// 		Print_bin_u64(&print, target);
// 		Print_char(&print,0);
// 		printf("pt_write_bits:  %s\n", print.begin);
// 	}
// 	{
// 		u8 buffer[] = { 0, 2 };
// 		u64 target = 0;
// 		pt_write_bits2((char*) buffer,     0, 7, (char*) &target);
// 		pt_write_bits2((char*) buffer + 1, 7, 7, (char*) &target);
// 		Print_clear(&print);
// 		Print_u64(&print, target);
// 		Print_char(&print, '-');
// 		Print_bin_u64(&print, target);
// 		Print_char(&print,0);
// 		printf("pt_write_bits2: %s\n", print.begin);
// 	}
// }

#endif


// global_variable PlatformAPI platform;

