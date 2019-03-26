#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
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


#if CHECK_ASSERTIONS
#define Assert(Expression) if(!(Expression)) {*(int *)0 = 0;}
#else
#define Assert(Expression)
#endif

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

// internal, local_persist, global_variable
#if !defined(internal)
#define internal static
#endif
#define local_persist static
#define global_variable static

//---------------------------
// offseted pointers
//---------------------------

#define OffsetedPointer(ptr,offset) ((void*) ((char*) ptr + offset))
#define RightOffsetedPointer(ptr,size,offset) ((void*) ((char*) ptr + size - offset))

//---------------------------
// min and max
//---------------------------

#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))
#define RALIGN(a,b) (b*((a+b-1)/b))
#define LALIGN(a,b) (b*(a/b))

//---------------------------
// math stuff
//---------------------------

#define pt_abs_s32(x)     abs(x)
#define pt_abs_s64(x)     labs(x)
#define pt_abs_f32(x)     fabsf(x)
#define pt_abs_f64(x)     fabs(x)
#define pt_floor_f32(x)   floorf(x)
#define pt_floor_f64(x)   floor(x)
#define pt_log_f64(x)     log(x)
#define pt_sqrt_f64(x)    sqrt(x)
#define pt_tan_f64(x)     tan(x)
#define pt_sin_f64(x)     sin(x)
#define pt_cos_f64(x)     cos(x)
#define pt_sqrt_f64(x)    sqrt(x)
#define pt_atan2_f64(x,y) atan2(x,y)
#define pt_PI 3.14159265358979323846

//---------------------------
// limits
//---------------------------

#define pt_MAX_U64 ULLONG_MAX;

/* Allow for functions below to create entries on profile report */
#include "profile.c"

//------------------------------------------------------------------------------
// intrinsics
//------------------------------------------------------------------------------
#if 0
#if !defined(COMPILER_MSVC)
#define COMPILER_MSVC 0
#endif
#if !defined(COMPILER_LLVM)
#define COMPILER_LLVM 0
#endif
#if !COMPILER_MSVC && !COMPILER_LLVM
#if _MSC_VER
#undef COMPILER_MSVC
#define COMPILER_MSVC 1
#else
// TODO(casey): Moar compilerz!!!
#undef COMPILER_LLVM
#define COMPILER_LLVM 1
#endif
#endif

#if COMPILER_MSVC
#include <intrin.h>
#elif COMPILER_LLVM
#include <x86intrin.h>
#else
#error SEE/NEON optimizations are not available for this compiler yet!!!!
#endif
#endif

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

//------------------------------------------------------------------------------
// Print
//------------------------------------------------------------------------------

typedef struct {
	char *begin;
	char *end;
	char *capacity;
	u64   written: 63;
	u64   overflow: 1;
	u64   initialized: 1;
} Print;

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

typedef struct {
	MemoryBlock memblock;
	void*       handle;
} pt_Memory;

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
// Ptr
//------------------------------------------------------------------------------

//
// 6 bytes Offset pointer to enable position independent
// data structures
//
typedef struct {
	u16 data[3];
} Ptr;

#define PTR_SPECIALIZED_TYPE(name) typedef struct { Ptr ptr; } name ;

//
// check macro on nanocube_platform.c to simplify the use
// of specialized pointers:
//
//     PTR_SPECIALIZED_TYPE(name)
//     PTR_SPECIALIZED_SERVICES(name)
//


//------------------------------------------------------------------------------
// platform independent file path struct
//------------------------------------------------------------------------------

#define MAX_FILE_PATH_SIZE 1023
typedef struct {
	char  full_path[MAX_FILE_PATH_SIZE+1];
	char* name;
	char* extension; // extension start last . in name
	char* end;
} FilePath;

//------------------------------------------------------------------------------
// work queue
//------------------------------------------------------------------------------

typedef struct pt_WorkQueue pt_WorkQueue;

//------------------------------------------------------------------------------
// platform server abstraction
//------------------------------------------------------------------------------

// the status when creating a listen port or a client socket
// to connect to a server use this numbers.
#define pt_TCP_SOCKET_ACTIVATING 0
#define pt_TCP_SOCKET_OK 1
#define pt_TCP_SOCKET_ERROR -1

#define pt_TCP_LISTEN_SOCKET 1
#define pt_TCP_CLIENT_SOCKET 2
#define pt_TCP_SERVER_SOCKET 3

typedef struct {
	s32   type;
	void* user_data;
	void* handle;
} pt_TCP_Socket;

typedef struct {
	void *handle;
} pt_TCP;

typedef struct {
	s32           status;
	pt_TCP_Socket socket;
} pt_TCP_Feedback;

#define PLATFORM_TCP_CALLBACK(name) void name(pt_TCP_Socket *socket, char *buffer, u64 length)
typedef PLATFORM_TCP_CALLBACK(PlatformTCPCallback);

//------------------------------------------------------------------------------
// Platform API
//------------------------------------------------------------------------------

// memory

#define PLATFORM_ALLOCATE_MEMORY(name) pt_Memory name(u64 size, u8 alignment, u64 preferred)
typedef PLATFORM_ALLOCATE_MEMORY(PlatformAllocateMemory);

#define PLATFORM_RESIZE_MEMORY(name) b8 name(pt_Memory *mem, u64 new_size, u8 alignment)
typedef PLATFORM_RESIZE_MEMORY(PlatformResizeMemory);

#define PLATFORM_FREE_MEMORY(name) void name(pt_Memory *pm)
typedef PLATFORM_FREE_MEMORY(PlatformFreeMemory);

#define PLATFORM_COPY_MEMORY(name) void name(void *dest, void *src, u64 count)
typedef PLATFORM_COPY_MEMORY(PlatformCopyMemory);

// file handling

#define PLATFORM_OPEN_READ_FILE(name) pt_File name(const char* file_begin, const char* file_end)
typedef PLATFORM_OPEN_READ_FILE(PlatformOpenReadFile);

#define PLATFORM_OPEN_WRITE_FILE(name) pt_File name(const char* file_begin, const char* file_end)
typedef PLATFORM_OPEN_WRITE_FILE(PlatformOpenWriteFile);

#define PLATFORM_READ_NEXT_FILE_CHUNK(name) void name(pt_File *pfh, char *buffer_begin, char* buffer_end)
typedef PLATFORM_READ_NEXT_FILE_CHUNK(PlatformReadNextFileChunk);

#define PLATFORM_WRITE_TO_FILE(name) b8 name(pt_File *pfh, char *begin, char* end)
typedef PLATFORM_WRITE_TO_FILE(PlatformWriteToFile);

#define PLATFORM_SEEK_FILE(name) void name(pt_File *pfh, u64 offset)
typedef PLATFORM_SEEK_FILE(PlatformSeekFile);

#define PLATFORM_CLOSE_FILE(name) void name(pt_File *pfh)
typedef PLATFORM_CLOSE_FILE(PlatformCloseFile);

#define PLATFORM_EXECUTABLE_PATH(name) void name(FilePath *fp)
typedef PLATFORM_EXECUTABLE_PATH(PlatformExecutablePath);

#define PLATFORM_RESIZE_FILE(name) b8 name(char* file_begin, char* file_end, u64 new_size)
typedef PLATFORM_RESIZE_FILE(PlatformResizeFile);

// // tcp server
//
// #define PLATFORM_SERVER_CREATE(name) pt_Server name(u32 port, void *user_data, PlatformServerRequestHandler *handler)
// typedef PLATFORM_SERVER_CREATE(PlatformServerCreate);
//
// #define PLATFORM_SERVER_START(name) void name(pt_Server *server, pt_WorkQueue *work_queue)
// typedef PLATFORM_SERVER_START(PlatformServerStart);
//
// #define PLATFORM_SERVER_RESPOND(name) void name(pt_ServerConnection *connection, char *begin, char *end)
// typedef PLATFORM_SERVER_RESPOND(PlatformServerRespond);

// tcp

#define PLATFORM_TCP_CREATE(name) pt_TCP name()
typedef PLATFORM_TCP_CREATE(PlatformTCPCreate);

#define PLATFORM_TCP_DESTROY(name) void name(pt_TCP tcp)
typedef PLATFORM_TCP_DESTROY(PlatformTCPDestroy);

#define PLATFORM_TCP_SERVE(name) void name(pt_TCP tcp, s32 port, void *user_data, PlatformTCPCallback *callback, pt_TCP_Feedback *feedback)
typedef PLATFORM_TCP_SERVE(PlatformTCPServe);

#define PLATFORM_TCP_CLIENT(name) void name(pt_TCP tcp, s32 port, char *hostname, void *user_data, PlatformTCPCallback *callback, pt_TCP_Feedback *feedback)
typedef PLATFORM_TCP_CLIENT(PlatformTCPClient);

#define PLATFORM_TCP_WRITE(name) void name(pt_TCP_Socket *socket, char *buffer, s64 length)
typedef PLATFORM_TCP_WRITE(PlatformTCPWrite);

// process current events (if incoming data on client and server sockets,
// use the work queue to dispatch the data)
#define PLATFORM_TCP_PROCESS_EVENTS(name) void name(pt_TCP tcp, pt_WorkQueue *work_queue)
typedef PLATFORM_TCP_PROCESS_EVENTS(PlatformTCPProcessEvents);

//
/* run the event loop indefinitely */
// #define PLATFORM_TCP_CYCLE(name) void name(pt_TCP *server, pt_WorkQueue *work_queue)
//
// #define PLATFORM_TCP_CONNECT(name) pt_Connection name(char *hostname, s32 port, pt_ReceiveHandler *receive_handler)
//
// #define PLATFORM_TCP_SEND(name) void name(pt_Connection *connection, char *buffer_begin, char *buffer_end)
//
// #define PLATFORM_TCP_SERVE(name) void name(pt_TCP *tcp, s32 port, void *user_data, pt_ReceiveHandler *receive_handler)
//


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

typedef struct PlatformAPI {

	PlatformAllocateMemory           *allocate_memory;
	PlatformResizeMemory             *resize_memory;
	PlatformFreeMemory               *free_memory;
	PlatformCopyMemory               *copy_memory;
	PlatformOpenReadFile             *open_read_file;
	PlatformOpenWriteFile            *open_write_file;
	PlatformReadNextFileChunk        *read_next_file_chunk;
	PlatformSeekFile                 *seek_file;
	PlatformCloseFile                *close_file;
	PlatformWriteToFile              *write_to_file;
	PlatformExecutablePath           *executable_path;

#if 0
	PlatformServerCreate             *server_create;
	PlatformServerStart              *server_start;
	PlatformServerRespond            *server_respond;
#else
	PlatformTCPCreate                *tcp_create;
	PlatformTCPDestroy               *tcp_destroy;
	PlatformTCPServe                 *tcp_serve;
	PlatformTCPWrite                 *tcp_write;
	PlatformTCPProcessEvents         *tcp_process_events;
	PlatformTCPClient                *tcp_client;
#endif

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

} PlatformAPI;

// anyone who includes platform.c (should only be included once)
// will have this global platform variable
global_variable PlatformAPI platform;

/*
 *
 * functions
 *
 */

internal f64
pt_nan_f64()
{
	return NAN;
}

internal b8
pt_is_nan_f64(f64 value)
{
	return (b8) (value != value);
}

internal inline u8
pt_safe_s32_u8(s32 i)
{
	Assert(i >= 0 && i < 256);
	return (u8) i;
}

internal inline u8
pt_safe_s64_u8(s64 i)
{
	Assert(i >= 0 && i < 256);
	return (u8) i;
}

internal inline u32
pt_safe_s64_u32(s64 i)
{
	Assert(i >= 0 && i < 0x100000000ll);
	return (u32) i;
}

internal inline u16
pt_safe_s64_u16(s64 i)
{
	Assert(i >= 0 && i < 0x10000ll);
	return (u16) i;
}

internal inline u32
pt_safe_s32_u32(s32 i)
{
	Assert(i >= 0);
	return (u32) i;
}

internal inline u64
pt_safe_s64_u64(s64 i)
{
	Assert(i >= 0);
	return (u64) i;
}

internal inline u64
pt_next_multiple(u64 value, u64 base)
{
	return base * ((value + base - 1)/base);
}

internal void pt_copy_bytesn(const char* src, char* dst, u64 n)
{
	for (u64 i=0;i<n;++i) {
		*dst++ = *src++;
	}
}

internal s32 pt_copy_bytes(const char* src_begin, const char* src_end,
			   char* dst_begin, char* dst_end)
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

inline internal void
pt_fill(char *begin, char *end, char ch)
{
	while (begin != end) {
		*begin = ch;
		++begin;
	}
}

inline internal void
pt_filln(char *begin, u64 count, char ch)
{
	for (u64 i=0;i<count;++i) {
		*begin = ch;
		++begin;
	}
}

internal void
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
internal void
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
	internal void                              \
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

internal u32
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
internal u32
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
internal void
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
internal void
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

internal void
pt_BitStream_init(pt_BitStream *self, void *data, u64 data_length, u64 bit_offset)
{
	self->data = data;
	self->data_length = data_length;
	self->bit_offset = bit_offset;
}

internal u8
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

internal void
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
}

internal void
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
internal void
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
// MemoryBlock
//------------------------------------------------------------------------------

internal s64
MemoryBlock_length(MemoryBlock *self)
{
	Assert(self->begin < self->end);
	return self->end - self->begin;
}

internal MemoryBlock
MemoryBlock_aligned(MemoryBlock block, u64 block_size)
{
	char *begin = (char*) RALIGN((u64) block.begin,block_size);
	char *end   = (char*) LALIGN((u64) block.end,  block_size);
	if (begin <= end) {
		return (MemoryBlock) { .begin = begin, .end = end };
	} else {
		return (MemoryBlock) { .begin = 0, .end = 0 };
	}
}

internal MemoryBlock
MemoryBlock_partition_part(MemoryBlock block, s32 index, s32 parts)
{
	s64 n = block.end - block.begin;
	s64 block_size = n / parts;
	char *begin = block.begin + block_size * index;
	return (MemoryBlock) { .begin = begin, .end = begin + block_size };
}

//------------------------------------------------------------------------------
//  string utility
//------------------------------------------------------------------------------

internal char*
cstr_end(char *begin)
{
	while(*begin != 0)
		++begin;
	return begin;
}

internal s64
cstr_len(char *begin)
{
	char *it = begin;
	while(*it != 0)
		++it;
	return it - begin;
}

internal s64
pt_compare_memory(char* b1, char* e1,
		  char* b2, char* e2)
{
	char *i1 = b1;
	char *i2 = b2;
	// abs value will be 1 + index of where differs
	// signal is negative if first differing byte is smaller
	// on
	while (i1 != e1 && i2 != e2)
	{
		s64 diff = (s64) *i1 - (s64) *i2;
		if (diff < 0) { return -(1 + (i1-b1)); }
		else if (diff > 0) { return (1 + (i1-b1)); }
		++i1; ++i2;
	}
	if (i1 != e1) { return (1 + (i1-b1)); }
	else if (i2 != e2) { return -(1 + (i1-b1)); }
	return 0;
}

internal s64
pt_compare_memory_n(char* b1, char* e1, char* b2, char* e2, s64 n)
{
	if (e1 - b1 > n)
		e1 = b1 + n;
	if (e2 - b2 > n)
		e2 = b2 + n;
	return pt_compare_memory(b1, e1, b2, e2);
}

internal inline s64
pt_compare_memory_cstr(
		       char* b1,
		       char* e1,
		       char* cstr)
{
	return pt_compare_memory(b1, e1, cstr, cstr_end(cstr));
}

internal inline s64
pt_compare_memory_n_cstr(char* b1, char* e1, char* cstr, s64 n)
{
	// compare at most n characters
	return pt_compare_memory_n(b1, e1, cstr, cstr_end(cstr), n);
}

//------------------------------------------------------------------------------
// Parse integers
//------------------------------------------------------------------------------

internal b8
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

internal b8
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

internal b8
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

internal b8
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

internal b8
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

internal b8
pt_parse_f32(char *begin, char *end, f32 *output)
{
	// @TODO(llins): make this more efficient
	// expecting a cstr is not a good idea
	char buffer[32];
	s32 len = pt_copy_bytes(begin, end, buffer, buffer+31);
	buffer[len] = 0;
	*output = (f32) atof(buffer);
	return 1; //(errno == 0 ? 1 : 0);
}


internal char*
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

internal void
pt_swap_s32(s32 *a, s32 *b)
{
	s32 aux = *a;
	*a = *b;
	*b = aux;
}

//------------------------------------------------------------------------------
// MemoryBlock
//------------------------------------------------------------------------------

internal s64
pt_MemoryBlock_find_first_match(MemoryBlock *begin, MemoryBlock *end, char *content_begin, char *content_end)
{
	Assert(begin < end);
	MemoryBlock *it = begin;
	while (it != end) {
		if (pt_compare_memory(it->begin, it->end, content_begin, content_end) != 0) {
			++it;
		} else {
			return (it - begin);
		}
	}
	return -1;
}

//------------------------------------------------------------------------------
// Print
//------------------------------------------------------------------------------

internal void
Print_init(Print *ps, char *begin, char *capacity)
{
	Assert(begin <= capacity);
	ps->begin = ps->end = begin;
	ps->capacity = capacity;
	ps->written = 0;
	ps->overflow = 0;
	ps->initialized = 1;
}

internal u64
Print_length(Print *self)
{
	Assert(self->begin <= self->end);
	return self->end - self->begin;
}

internal void
Print_clear(Print *ps)
{
	ps->end = ps->begin;
	ps->written = 0;
	ps->overflow = 0;
	ps->initialized = 1;
}

internal char*
Print_checkpoint(Print *self)
{
	return self->end;
}

internal void
Print_rewind(Print *self, char *checkpoint)
{
	Assert(self->begin <= checkpoint && checkpoint <= self->capacity);
	self->end = checkpoint;
}


internal void
Print_u64(Print *ps, u64 x)
{
	Assert(ps->initialized);
	if (x == 0) {
		if (ps->end < ps->capacity) {
			*ps->end = '0';
			++ps->end;
			ps->written = 1;
			ps->overflow = 0;
		}
		else {
			ps->overflow = 1;
			ps->written = 0;
		}
	}
	else {
		char *it = ps->end;
		while (x > 0) {
			if (it == ps->capacity) {
				// overflow
				ps->overflow = 1;
				ps->written = 0;
				return;
			}
			u64 d = x % 10;
			x = x/10;
			*it = (char)('0' + d);
			++it;
		}
		// reverse
		int i = (int) (it - ps->end)/2 - 1;
		while (i >= 0) {
			char c = *(ps->end + i);
			*(ps->end + i) = *(it - 1 - i);
			*(it - 1 - i) = c;
			--i;
		}
		ps->written = it - ps->end;
		ps->overflow = 0;
		ps->end += ps->written;
	}
}


internal void
Print_s64(Print *ps, s64 x)
{
	Assert(ps->initialized);


	if (x == 0) {
		if (ps->end < ps->capacity) {
			*ps->end = '0';
			++ps->end;
			ps->written = 1;
			ps->overflow = 0;
		} else {
			ps->overflow = 1;
			ps->written = 0;
		}
		return;
	} else {

		char *begin = ps->end;

		if (x < 0) {
			if (begin < ps->capacity) {
				*begin = '-';
				++begin;
				ps->written = 1;
				ps->overflow = 0;
				x = -x;
			} else {
				ps->overflow = 1;
				ps->written = 0;
				return;
			}
		}

		char *it = begin;
		while (x > 0) {
			if (it == ps->capacity) {
				// overflow
				ps->overflow = 1;
				ps->written = 0;
				return;
			}
			u64 d = x % 10;
			x = x/10;
			*it = (char)('0' + d);
			++it;
		}
		// reverse
		int i = (int) (it - begin)/2 - 1;
		while (i >= 0) {
			char c = *(begin + i);
			*(begin + i) = *(it - 1 - i);
			*(it - 1 - i) = c;
			--i;
		}
		ps->written = it - ps->end;
		ps->overflow = 0;
		ps->end += ps->written;
	}
}


internal void
Print_cstr_safe(Print *self, char *begin, char *end)
{
	Assert(self->initialized);
	char* it = self->end;
	char* src_it = begin;
	while (*src_it != 0 && src_it != end) {
		if (it == self->capacity) {
			// overflow
			self->overflow = 1;
			self->written = 0;
			return;
		}
		*it = *src_it;
		++it;
		++src_it;
	}
	self->written = it - self->end;
	self->overflow = 0;
	self->end += self->written;
}

internal void
Print_cstr(Print *ps, const char* s)
{
	Assert(ps->initialized);

	char* it = ps->end;
	while (*s != 0) {
		if (it == ps->capacity) {
			// overflow
			ps->overflow = 1;
			ps->written = 0;
			return;
		}
		*it = *s;
		++it;
		++s;
	}
	ps->written = it - ps->end;
	ps->overflow = 0;
	ps->end += ps->written;
}

internal void
Print_char(Print *ps, char c)
{
	Assert(ps->initialized);
	if (ps->end != ps->capacity) {
		*ps->end = c;
		++ps->end;
		ps->written = 1;
		ps->overflow = 0;
	}
	else {
		ps->overflow = 1;
		ps->written = 0;
	}
}

internal void
Print_nchar(Print *ps, char c, u64 n)
{
	Assert(ps->initialized);
	if (n <= (u64) (ps->capacity - ps->end)) {
		for (u64 i=0;i<n;++i) {
			*ps->end = c;
			++ps->end;
		}
		ps->written = n;
		ps->overflow = 0;
	} else {
		ps->overflow = 1;
		ps->written = 0;
	}
}

internal void
Print_str(Print *ps, const char* begin, const char *end)
{
	Assert(ps->initialized);
	u64 n = end - begin;
	if (n <= (u64)(ps->capacity - ps->end)) {
		for (u64 i=0;i<n;++i) {
			*ps->end = *(begin + i);
			++ps->end;
		}
		ps->written = n;
		ps->overflow = 0;
	}
	else {
		ps->overflow = 1;
		ps->written = 0;
	}
}

typedef union f64_bits
{
	u64 data;
	f64 value;
	struct {
		u64 mantissa: 52;
		u64 exponent: 11;
		u64 sign: 1;
	};
}
f64_bits;

internal void
Print_f64(Print *ps, f64 value)
{
	char buffer[32];
	sprintf(buffer, "%e", value);
	Print_cstr(ps, buffer);
}

/*
 * Assume last print started at position "pos".
 * can be used to align a series of smaller
 * prints.
 */
internal void
Print_fake_last_print(Print *self, char *pos)
{
	Assert(self->begin <= pos && pos <= self->end
	       && !self->overflow && self->initialized);
	self->written = pt_safe_s64_u64(self->end - pos);
}


internal void
Print_align(Print *ps, u64 len, s8 align, char space_filler)
{
	Assert(ps->initialized);
	if (len <= ps->written)
		return;

	u64 extra = len - ps->written;
	if (extra <= (u64)(ps->capacity - ps->end)) {

		char* begin = ps->end - ps->written;

		u64 left = align < 0
			? 0
			: (align > 0
			   ? (len-extra)
			   : (len - (extra+1)/2));

		for (u64 i=0;i<extra;++i) {
			*ps->end = space_filler;
			++ps->end;
		}

		pt_rotate(begin, begin + left, ps->end);

		ps->written  = len;
		ps->overflow = 0;
	}
	else {
		ps->overflow = 1;
		ps->written = 0;
	}
}

internal void
Print_bin_u64(Print *self, u64 x)
{
	char *mark = self->end;

	// NOTE(llins): super inefficient
	b8 game_on = 0;
	for (s32 i=63;i>=0;--i) {
		b8 on = ((x & (1ULL << i)) != 0);
		if (on) {
			Print_char(self, '1');
			game_on = 1;
		} else if (game_on) {
			Print_char(self, '0');
		}
	}
	if (!game_on) {
		Print_char(self, '0');
	}

	Print_fake_last_print(self, mark);
}


// use the elegant stb library from Jeff Roberts and Sean Barret
// https://raw.githubusercontent.com/nothings/stb/master/stb_sprintf.h
#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"

internal void
Print_format(Print *self, char const * format, ...)
{
	va_list argp;
	va_start(argp, format);
	s32 result = stbsp_vsnprintf(self->end, self->capacity - self->end, format, argp);
	va_end(argp);
	self->end += result;
	self->written = result;
	self->overflow = 0;
// 	char *it = self->end;
// 	/* expensive loop, if we could access the number of characters written */
// 	while (it != self->capacity && *it != 0)
// 		++it;
// 	self->end = it;
}

//------------------------------------------------------------------------------
// platform independent file path services
//------------------------------------------------------------------------------

internal void
FilePath_init(FilePath *self, char *begin, char *end)
{
	s64 n = end - begin;
	Assert(n <= MAX_FILE_PATH_SIZE);
	self->end = self->full_path + n;
	*self->end = 0;
	self->full_path[n+1] = 0; // make sure it is a cstr
	pt_copy_bytes(begin, end, self->full_path, self->end);
	self->end  = self->full_path + n;
	self->name = self->full_path;

	// find last /
	char *it = self->full_path;
	while (it != self->end) {
		if (*it == '/' || *it == '\\') {
			self->name = it + 1;
			self->extension = self->end;
		} else if (*it == '.') {
			self->extension = it;
		}
		++it;
	}
}

internal void
FilePath_copy(FilePath* self, FilePath *other)
{
	char *dst = self->full_path;
	for (char *it=other->full_path;it<other->end;++it) {
		*dst++ = *it;
	}
	self->end  = dst;
	*dst = 0; // make it cstr
	self->name = self->full_path + (other->name - other->full_path);
	self->extension = self->full_path
		+ (other->extension - other->full_path);
}

internal void
FilePath_set_name(FilePath* self, char *begin, char *end)
{
	s64 n = end - begin;

	Assert((self->name - self->full_path) + n <= MAX_FILE_PATH_SIZE);

	self->end = self->name + n;
	*self->end = 0;
	pt_copy_bytes(begin, end, self->name, self->end);
	self->extension = self->end;

	// find last /
	self->extension = self->end;
	char *it = self->full_path;
	while (it != self->end)
	{
		if (*it == '.')
		{
			self->extension = it;
		}
		++it;
	}
}

internal inline void
FilePath_set_name_cstr(FilePath* fp, char *cstr)
{
	FilePath_set_name(fp, cstr, cstr_end(cstr));
}


//------------------------------------------------------------------------------
// LinearAllocator
//------------------------------------------------------------------------------

internal void
LinearAllocator_init(LinearAllocator* self, char *begin, char *end, char *capacity)
{
	self->begin = begin;
	self->end   = end;
	self->capacity = capacity;
}

internal char*
LinearAllocator_alloc(LinearAllocator* self, u64 num_bytes)
{
	Assert(self->end + num_bytes <= self->capacity);
	void *result = self->end;
	self->end += num_bytes;
	return result;
}

internal char*
LinearAllocator_alloc_if_available(LinearAllocator* self, u64 num_bytes)
{
	if (self->end + num_bytes > self->capacity) {
		return 0;
	}
	void *result = self->end;
	self->end += num_bytes;
	return result;
}



internal u64
LinearAllocator_free_space(LinearAllocator* self)
{
	return (self->capacity - self->end);
}

internal char*
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

internal void
LinearAllocator_clear(LinearAllocator* self)
{
	self->end = self->begin;
}

internal void
LinearAllocator_pop(LinearAllocator* self, u64 num_bytes)
{
	Assert(self->begin + num_bytes <= self->end);
	self->end -= num_bytes;
}

internal LinearAllocatorCheckpoint
LinearAllocator_checkpoint(LinearAllocator *self)
{
	LinearAllocatorCheckpoint result;
	result.checkpoint = self->end;
	return result;
}

internal void
LinearAllocator_rewind(LinearAllocator *self, LinearAllocatorCheckpoint cp)
{
	Assert(cp.checkpoint <= self->end);
	self->end = cp. checkpoint;
}


//------------------------------------------------------------------------------
// BilinearAllocator
//------------------------------------------------------------------------------

internal void
BilinearAllocator_init(BilinearAllocator* self, char *begin, char *capacity)
{
	// [begin,end_left)
	// [end_right,capcity)
	self->begin = begin;
	self->end_left = begin;
	self->end_right = capacity;
	self->capacity = capacity;
}

internal char*
BilinearAllocator_alloc_left(BilinearAllocator* self, u64 num_bytes)
{
	Assert(self->end_left + num_bytes <= self->end_right);
	void *result = self->end_left;
	self->end_left += num_bytes;
	return result;
}

internal char*
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

internal MemoryBlock
BilinearAllocator_free_memblock(BilinearAllocator* self)
{
	return (MemoryBlock) { .begin = self->end_left, .end = self->end_right };
}

internal u64
BilinearAllocator_free_space(BilinearAllocator* self)
{
	return (self->end_right - self->end_left);
}

internal char*
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

internal void
BilinearAllocator_clear(BilinearAllocator* self)
{
	self->end_left  = self->begin;
	self->end_right = self->capacity;
}

internal void
BilinearAllocator_clear_left(BilinearAllocator* self)
{
	self->end_left  = self->begin;
}

internal void
BilinearAllocator_clear_right(BilinearAllocator* self)
{
	self->end_right  = self->capacity;
}

internal void
BilinearAllocator_pop_left(BilinearAllocator* self, u64 num_bytes)
{
	Assert(self->begin + num_bytes <= self->end_left);
	self->end_left -= num_bytes;
}

internal void
BilinearAllocator_pop_right(BilinearAllocator* self, u64 num_bytes)
{
	Assert(self->end_right + num_bytes <= self->capacity);
	self->end_right += num_bytes;
}


internal BilinearAllocatorCheckpoint
BilinearAllocator_left_checkpoint(BilinearAllocator *self)
{
	BilinearAllocatorCheckpoint result;
	result.checkpoint = self->end_left;
	result.left = 1;
	return result;
}

internal BilinearAllocatorCheckpoint
BilinearAllocator_right_checkpoint(BilinearAllocator *self)
{
	BilinearAllocatorCheckpoint result;
	result.checkpoint = self->end_right;
	result.left = 0;
	return result;
}

internal void
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

//------------------------------------------------------------------------------
// Ptr
//------------------------------------------------------------------------------

internal void
Ptr_reset(Ptr *self, s64 offset)
{
	if (offset > 0) {
		// *((u32*) &self->data)     = *  ( (u32*) &offset     );
	        u16 *ptr = (u16*) &offset;
		self->data[0] = *( ptr + 0 );
		self->data[1] = *( ptr + 1 );
		self->data[2] = *( ptr + 2 );
	}
	else {
		offset = -offset;
	        u16 *ptr = (u16*) &offset;
		self->data[0] = *( ptr + 0 );
		self->data[1] = *( ptr + 1 );
		self->data[2] = *( ptr + 2 ) + (u16) 0x8000;
// 		*((u32*) &self->data)     = *  ( (u32*) &offset     );
// 		self->data[2] = *( (u16*) &offset + 2 ) + (u16) 0x8000;
	}
}

internal void
Ptr_set_null(Ptr *self)
{
	self->data[0] = 1;
	self->data[1] = 0;
	self->data[2] = 0;
}

internal void
Ptr_set(Ptr *self, void* p)
{
	if (p) {
		s64 offset = (s64) ((char*) p - (char*) self);
		Ptr_reset(self, offset);
	} else {
		Ptr_set_null(self);
	}
}


// NOTE(llins): 2017-06-20T16:20
// How can we make a 6-bytes pointer more efficient
// in some experiments this func was taking on average
// 71 sycles per hit
internal s64
Ptr_offset(Ptr *self)
{

	// pf_BEGIN_BLOCK("Ptr_offset");

	/* this is always a positiver number */
	s64 offset = ((s64)self->data[0]) +
		     ((s64)self->data[1] << 16) +
		     ((s64)self->data[2] << 32);

	/* but if bit 48 */
	if (offset & 0x800000000000ll) {
		offset = -(offset & 0x7fffffffffffll);
	}

	// pf_END_BLOCK();

	return offset;
}

internal void*
Ptr_get_not_null(Ptr *self)
{
	return ((char*) self + Ptr_offset(self));
}


internal b8
Ptr_is_null(Ptr *self)
{
	return self->data[0] == 1 && self->data[1] == 0 && self->data[2] == 0;
}

internal b8
Ptr_is_not_null(Ptr *self)
{
	return !Ptr_is_null(self);
}

internal void*
Ptr_get(Ptr *self)
{
	return Ptr_is_not_null(self) ? Ptr_get_not_null(self) : 0;
}

internal void
Ptr_copy(Ptr *self, Ptr *other)
{
	Ptr_set(self, Ptr_get(other));
}

internal void
Ptr_swap(Ptr *self, Ptr *other)
{
	void *a = Ptr_get(self);
	void *b = Ptr_get(other);
	Ptr_set(self, b);
	Ptr_set(other, a);
}

// trick gcc -E -CC x.c to preserve comments and see the macro in multiple lines
#define PTR_SPECIALIZED_SERVICES(name, base) /*
*/ void name##_reset(name *self, s64 offset) { Ptr_reset(&self->ptr, offset); } /*
*/ void name##_set_null(name *self) { Ptr_set_null(&self->ptr); } /*
*/ void name##_set(name *self, base *p) { Ptr_set(&self->ptr, p); } /*
*/ b8 name##_is_not_null(name *self) { return Ptr_is_not_null(&self->ptr); } /*
*/ b8 name##_is_null(name *self) { return Ptr_is_null(&self->ptr); } /*
*/ base * name##_get(name *self) { return ((base *) Ptr_get(&self->ptr)); } /*
*/ base * name##_get_not_null(name *self) { return (base *) Ptr_get_not_null(&self->ptr); } /*
*/ void name##_copy(name *self, name *other) { Ptr_copy(&self->ptr, &other->ptr); } /*
*/ void name##_swap(name *self, name *other) { Ptr_swap(&self->ptr, &other->ptr); }

internal void
pt_rotate_Ptr(Ptr *begin, Ptr *middle, Ptr *end)
{
	Ptr* next = middle;
	while (begin != next) {
		Ptr_swap(begin,next);
		++begin;
		++next;
		if (next == end) {
			next = middle;
		} else if (begin == middle) {
			middle = next;
		}
	}
}


#define pt_DEFINE_ARRAY(name, base) \
typedef struct { \
	base *begin; \
	base *end; \
	base *capacity; \
} name;

#define pt_DEFINE_ARRAY_SERVICES(name, base) \
internal void \
name ## _init(name *self, base *begin, base *capacity) { \
	self->begin = begin; \
	self->end   = begin; \
	self->capacity = capacity; \
} \
internal void \
name ## _append(name *self, base value) { \
	Assert(self->end != self->capacity); \
	*self->end = value; \
	++self->end; \
} \
internal base \
name ## _get(name *self, u64 index) { \
	Assert(self->begin + index < self->end); \
	return self->begin[index]; \
} \
internal void \
name ## _set(name *self, u64 index, base value) { \
	Assert(self->begin + index < self->end); \
	self->begin[index] = value; \
} \
internal void \
name ## _clear(name *self) { \
	self->end = self->begin; \
} \
internal u64 \
name ## _size(name *self) { \
	return (u64) (self->end - self->begin); \
}

/* define ArrayMemoryBlock */
pt_DEFINE_ARRAY(ArrayMemoryBlock, MemoryBlock)
pt_DEFINE_ARRAY_SERVICES(ArrayMemoryBlock, MemoryBlock)

/* define ArrayMemoryBlock */
pt_DEFINE_ARRAY(Array_u64, u64)
pt_DEFINE_ARRAY_SERVICES(Array_u64, u64)

/* define ArrayMemoryBlock */
pt_DEFINE_ARRAY(Array_u128, u128)
pt_DEFINE_ARRAY_SERVICES(Array_u128, u128)















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




