#include "nanocube_platform.h"
#include "nanocube_alloc.h"

/* replace nx default payload before loading index */
#define NX_PAYLOAD

typedef struct nx_PayloadAggregate {
	u64 count;
} nx_PayloadAggregate;

internal void
nx_PayloadAggregate_init(nx_PayloadAggregate* self,
			 void *payload_context)
{
	self->count = 0;
}

internal void
nx_PayloadAggregate_insert(nx_PayloadAggregate* self,
			   void *unit,
			   void *payload_context)
{
	u32 value = *((u32*) unit);
	self->count += value;
}

internal void
nx_PayloadAggregate_share(nx_PayloadAggregate* self,
			  nx_PayloadAggregate *other,
			  void *payload_context)
{
	self->count = other->count;
}


#include "nanocube_index.h"

#include "nanocube_platform.c"
#include "nanocube_alloc.c"
#include "nanocube_index.c"

static PlatformAPI platform;

#include "nanocube_util.c"

#include <stdio.h>
#include <stdlib.h>

#include <winsock2.h>
#include <windows.h>
#include "win32_nanocube_platform.c"

typedef struct {
	int  num_bits;
	int  max;
	char value[1024];
	int  length;
	int  pos;
	int  offset;
} Counter;

inline void
Counter_reset(Counter* self)
{
	self->pos = self->offset;
	self->value[self->pos] = -1;
}

internal void
Counter_init(Counter* self, int b, int l, int offset)
{
	Assert(l < 1024);
	self->num_bits=b;
	self->length= l;
	self->offset = offset;
	self->max = (1 << b) - 1;
	for (int i = 0; i < offset; ++i)
		self->value[i] = 0;
	Counter_reset(self);
}

internal b8
Counter_next(Counter* self)
{
	if (self->pos < self->offset) return 0;
	while (self->pos >= self->offset) {
		++self->value[self->pos];
		if (self->value[self->pos] > self->max) {
			--self->pos;
		}
		else if (self->pos < self->length - 1) {
			++self->pos;
			self->value[self->pos] = -1;
		}
		else { // if (pos == length-1)
			return 1;
		}
	}
	return 0;
}


internal void
Counter_print(Counter *self, Print *print)
{
    for (s32 i = 0; i < self->length; ++i) {
	    Print_u64(print, (u64) self->value[i]);
    }
}

typedef struct {
	int  p[1024];
	char data[1024];
	int  length;
} Permutation;

internal void
Permutation_init(Permutation *self, int length)
{
	self->length = length;
	for (int i = 0; i < length; ++i) {
		self->p[i] = i;
	}
	for (int i = length; i > 1; --i) {
		int  index = (rand() % i);
		int  a = self->p[i-1];
		self->p[i - 1] = self->p[index];
		self->p[index] = a;
	}
}

internal void
Permutation_update(Permutation* self, char* raw)
{
	for (int i = 0; i < self->length; ++i) {
		self->data[i] = *(raw + self->p[i]);
	}
}

internal void
Permutation_print(Permutation *self, Print *print)
{
    for (s32 i = 0; i < self->length; ++i) {
	    Print_u64(print, (u64) self->data[i]);
    }
}


nu_PRINT_PAYLOAD(test_print_payload)
{
	Print_u64(print, payload->count);
// 	b8 first = 1;
// 	for (u32 i=0;i<64;++i) {
// 		if (payload->bitset & (1ull << i)) {
// 			if (!first)
// 				Print_cstr(print,",");
// 			Print_u64(print, i);
// 			first = 0;
// 		}
// 	}
}

static PlatformFileHandle pfh_stderr;
static Print              print_obj;
static Print              *print;
static char               print_buffer[Kilobytes(2)];
internal void
init()
{
	/* initialize platform */
	win32_init_platform(&platform);

	print = &print_obj;
	Print_init(print, print_buffer, print_buffer + sizeof(print_buffer));

	pfh_stderr.open = 1;
	pfh_stderr.write = 1;
	pfh_stderr.read = 0;
	pfh_stderr.last_seek_success = 0;
	pfh_stderr.last_read = 0;
	pfh_stderr.handle = stderr;
}

internal void
msg(Print *p)
{
// 	const char name[] = "w:/tmp/last.txt";
// 	PlatformFileHandle pfh = platform.open_write_file(name, name
// 							  + sizeof(name));
	platform.write_to_file(&pfh_stderr, p->begin, p->end);
// 	platform.close_file(&pfh_stderr);
}

internal void
test_old()
{
	// mmap 1GB of virtual memory in this process
	// page aligned memory block
	PlatformMemory data_memory = platform.allocate_memory(Gigabytes(1), 12,
							      Terabytes(2));
	PlatformMemory insert_memory = platform.allocate_memory(Megabytes(32),
								0, 0);

	// try finding minimal examples to stress the code
	// considering all examples with 1, 1, 1
	//
	// two levels deep and one bit

	const int num_points      = 10;
	const u8  num_dimensions  = 2;
	const u8  num_levels      = 5;
	const u8  num_bits        = 2;

	// array with number of bits in each dimension
	u8 b[nx_MAX_NUM_DIMENSIONS];
	for (u8 i=0; i<num_dimensions; ++i) {
		*(b + i) = num_bits;
	}

	nx_Array bb = nx_Array_build(b, num_dimensions);

	int depth_per_record = (int) num_levels * num_dimensions;
	Counter counter;
	Counter_init(&counter, (int)num_bits,
		     depth_per_record * num_points,
		     depth_per_record); // fixing one point

	nx_LabelArrayList address[10]; // some max dimensions
	Assert(num_dimensions <= 10);
	for (int i=0;i<(int) num_dimensions-1;++i)
		address[i].next = &address[i+1];
	address[num_dimensions-1].next = 0;

	Permutation permutation;
	Permutation_init(&permutation, depth_per_record * num_points);

	// char buffer[128];

	const float PRINT_PROBABILITY = 0.0f;

	int count = 0;
	while (Counter_next(&counter)) {
		++count;
		al_Allocator* allocator =
			al_Allocator_new(data_memory.memblock);
		al_Cache* nanocube_index_cache =
			al_Allocator_create_cache(allocator,
						  "NanocubeIndex",
						  sizeof(nx_NanocubeIndex));

		nx_NanocubeIndex* nanocube_index = (nx_NanocubeIndex*)
			al_Cache_alloc(nanocube_index_cache);
		nx_NanocubeIndex_init(nanocube_index, allocator, bb); // bits per level

		b8 save_experiment = ((float) (rand()) /(float) (RAND_MAX))
			< PRINT_PROBABILITY;

		Permutation_update(&permutation, counter.value);
		nx_Label* p_labels = (nx_Label*) &permutation.data;

		if (count % 10000 == 0) {
			Print_clear(print);
			Print_cstr(print,"problem: ");
			Print_u64(print, count);
			Print_align(print, 10, 1);
			Print_cstr(print,"  ");
			Permutation_print(&permutation, print);
			Print_cstr(print,"\n");
			msg(print);
			// fprintf(stderr, "problem: %5d\n  ", count);
			// fprintf(stderr, "problem: %5d\n", count);
		}


		for (int i=0;i<(int) num_points;++i) {
// 			snprintf(buffer,sizeof(buffer),
// 				 "/tmp/test_%04d_p%d-%d_%s_b%d_d%d_l%d.dot",
// 				 count,(i+1),num_points,
// 				 permutation.values().c_str(), // "_____", // p.values().c_str(),
// 				 num_bits,num_dimensions,
// 				 num_levels);

			// set address
			for (u32 j=0;j<num_dimensions;++j) {
				nx_Label *labels_j = p_labels
					+ i * num_dimensions * num_levels
					+ j * num_levels;
				nx_Array_init(&address[j].labels, labels_j,
					      (u8) num_levels);
			}
			u32 payload_unit = (u32) i;
			nx_NanocubeIndex_insert (nanocube_index, &address[0],
						 &payload_unit, 0,
						 insert_memory.memblock);

		}
	}
}



#define print_variable(name) \
			Print_cstr(print,#name); \
			Print_align(print,40,1); \
			Print_u64(print,name); \
			Print_align(print,16,1); \
			Print_u64(print,100u*name/total); \
			Print_align(print,16,1); \
			Print_cstr(print,"%\n");

internal void
test_complete_tree()
{

	u32 bits       = 2;
	u32 levels     = 7;
	u32 dimensions = 2;
	u32 depth      = levels * dimensions;

	/* create a complete nanocube with the spec above */
	u32 n = (1u << (bits * levels * dimensions));

	Assert(n <= (1u << 31));

	f64 t0 = platform.get_time();

	/* prepare canonical input from 0 to n-1 */
	PlatformMemory input_memory =
		platform.allocate_memory(n * sizeof(u32), 12, 0);
	u32 *input_begin = (u32*) input_memory.memblock.begin;
	u32 *input_end   = (u32*) input_memory.memblock.end;
	for (u32 i=0;i<n;++i) {
		*(input_begin + i) = i;
	}


	Print_clear(print);
	Print_cstr(print,"Allocated array with ");
	Print_u64(print,n);
	Print_cstr(print," x (4B) slots and initialized it in ");
	Print_f64(print,platform.get_time() - t0);
	Print_cstr(print," s.\n");
	msg(print);

#if 1
	
	t0 = platform.get_time();

	/* permute input */
	u32 length = n;
	for (u32 i = length; i > 1; --i) {
		u32  index = (rand() % i);
		u32  a = *(input_begin + i - 1);
		*(input_begin + i - 1) = *(input_begin + index);
		*(input_begin + index) = a;
	}

	Print_clear(print);
	Print_cstr(print,"Shuffled records in ");
	Print_f64(print,platform.get_time() - t0);
	Print_cstr(print," s.\n");
	msg(print);

#endif



	t0 = platform.get_time();
	
	/* input path storage space */
	PlatformMemory paths_memory =
		platform.allocate_memory(depth, 12, 0);
	nx_Label *labels_begin = (nx_Label*) paths_memory.memblock.begin;
	nx_Label *labels_end   = (nx_Label*) paths_memory.memblock.end;

	/* insert all points in the order of the input */
	Assert(dimensions <= 10);
	nx_LabelArrayList address[10]; // some max dimensions
	for (u32 i=0;i<dimensions;++i) {
		address[i].next = &address[i+1];
		nx_Array_init(&address[i].labels,
			      labels_begin + i * levels,
			      (u8) levels);
	}
	address[dimensions-1].next = 0;



	/* prepare an empty nanocube with the right setup */
	PlatformMemory data_memory =
		platform.allocate_memory(Gigabytes(6), 12, Terabytes(2));

	PlatformMemory insert_memory =
		platform.allocate_memory(Megabytes(32), 0, 0);

	al_Allocator* allocator = al_Allocator_new(data_memory.memblock);

	al_Cache* nanocube_cache =
		al_Allocator_create_cache(allocator, "NC",
					  sizeof(nx_NanocubeIndex));

	nx_NanocubeIndex* nanocube_index = (nx_NanocubeIndex*)
		al_Cache_alloc(nanocube_cache);

	/* prepare nx_Array with bits per dimension */
	u8 b[10];
	for (u8 i=0; i<dimensions; ++i) {
		*(b + i) = bits;
	}
	nx_Array bb = nx_Array_build(b, dimensions);

	nx_NanocubeIndex_init(nanocube_index, allocator, bb);

	f64 delta  = 0.01;
	f64 target = delta;
	
	for (u32 i=0;i<n;++i) {
		/* fill in all of labels with value of input[i] */




		u32 paths = *(input_begin + i);

		char *src_begin = (char*) &paths;
		char *src_end   = src_begin + sizeof(paths);

		nx_Label lbl;
		char *dst_begin = (char*) &lbl;
		char *dst_end   = dst_begin + sizeof(lbl);

		for (u32 j=0;j<depth;++j) {
			lbl = 0;
			pt_read_bits(src_begin, (depth - 1 - j) * bits,
				     bits, dst_begin);
			*(labels_begin + j) = lbl;
		}

		u32 payload_unit = (u32) 1;
		nx_NanocubeIndex_insert (nanocube_index, &address[0],
				&payload_unit, 0, insert_memory.memblock);

#if 0
		/* create .dot file for visual inspection */
		Print_clear(print);
		Print_cstr(print,"test_");
		Print_u64(print,i);
		Print_cstr(print,".dot");
		nu_save_dot_file(nanocube_index, print->begin,
				 print->end, test_print_payload);
#endif

		f64 ratio = i/(f64)(n-1);
		if (ratio >= target) {
			target += delta;
			Print_clear(print);
			Print_cstr(print,"Completed ");
			Print_u64(print,(u64) (ratio * 100));
			Print_cstr(print,"% in ");
			Print_f64(print,platform.get_time() - t0);
			Print_cstr(print," s.");
			Print_cstr(print,"  ");
			Print_u64(print, (u64)(allocator->used_pages *
						al_PAGE_SIZE)/Megabytes(1));
			Print_cstr(print," MB\n");

			u64 total = nx_INSERT_BRANCH + nx_INSERT_EXACT
				+ nx_INSERT_SPLIT + nx_INSERT_SHARED_SUFFIX_WAS_SPLIT
				+ nx_INSERT_SHARED_SPLIT + nx_INSERT_SHARED_NO_SPLIT;

			print_variable(nx_INSERT_BRANCH);
			print_variable(nx_INSERT_EXACT);
			print_variable(nx_INSERT_SPLIT);
			print_variable(nx_INSERT_SHARED_SUFFIX_WAS_SPLIT);
			print_variable(nx_INSERT_SHARED_NO_SPLIT);
			print_variable(nx_INSERT_SHARED_SPLIT);
			print_variable(total);
			msg(print);
		}


	}

	Print_clear(print);
	Print_cstr(print,"No crash after inserting ");
	Print_u64(print,n);
	Print_cstr(print," records in ");
	Print_f64(print,platform.get_time() - t0);
	Print_cstr(print," s.\n");
	msg(print);


}



int
main(int argc, char *argv[])
{
	init();
	test_complete_tree();
	return 0;
}

