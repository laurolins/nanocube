#include "nanocube_platform.h"
#include "nanocube_alloc.h"
#include "nanocube_index.h"
#include "nanocube_draw.h"

// platform util assumes a platform global variable
// is available
global_variable PlatformAPI platform;

#include "nanocube_platform.c"
#include "nanocube_alloc.c"
#include "nanocube_index.c"
#include "nanocube_util.c"

// void name(ApplicationState* app_state, const char* req, u64 reqlen, PlatformFileHandle *pfh_stdout)
APPLICATION_PROCESS_REQUEST(application_process_request)
{
	platform = app_state->platform;

	//
	// make sure we don't accum. more than 4KB
	// before dumping print buffer
	//
	char print_buffer[Kilobytes(4)];
	Print print;
	Print_init(&print, print_buffer, print_buffer + sizeof(print_buffer));

	char* input_filename = request_begin; //"c:/work/compressed_nanocube/data/crimes_nc.dmp";

	PlatformMemory mem = platform.allocate_memory(Megabytes(1),0,0);
	Label* labels = (Label*) mem.memblock.begin; // allocate 1MB on the heap
	int labels_offset = 0;

	u8 dimensions = 0;
	u8 levels_per_dimension[MAX_NUM_DIMENSIONS];
	u8 bits_per_dimension[MAX_NUM_DIMENSIONS];
	int levels_sum = 0;

	u64 record_index    =  0;
	u8  dimension_index =  0;

	u64 line = 0;

	char read_buffer[Kilobytes(1)];

	FileTokenizer ftok;
	FileTokenizer_init(&ftok, '\n', read_buffer, read_buffer + sizeof(read_buffer), input_filename, cstr_end(input_filename));

	b8 parse_success = 1;

	BufferTokenizer btok;
	{
		while (FileTokenizer_next(&ftok)) {
			++line;

			// an empty line indicates
			// a new record should start
			if (ftok.token_empty) {

				if (record_index > 0 && dimension_index != dimensions) {
					Print_cstr(&print, "Insuficient dimensions for record on line ");
					Print_uint(&print, line);
					Print_cstr(&print, "\n");
					platform.write_to_file(pfh_stdout, print.begin, print.end);
					Print_clear(&print);
					parse_success = 0;
					break;
				}

				if (record_index == 0) {
					dimensions = dimension_index;
				}

				dimension_index = 0;
				++record_index;

			}
			else {

				BufferTokenizer_init(&btok,' ',ftok.token.begin, ftok.token.end);

				int max_label  = 0;
				int levels = 0;
				while (BufferTokenizer_next(&btok)) {
					// try converting token into an integer
					// needs to be null terminated
					int lbl;
					b8 ok = parse_int(btok.token.begin, btok.token.end, &lbl);
					if (!ok || lbl < 0 || lbl > 255)
					{
						Print_cstr(&print, "Could not parse non-negative int at line ");
						Print_uint(&print, line);
						Print_cstr(&print, "\n");
						platform.write_to_file(pfh_stdout, print.begin, print.end);
						Print_clear(&print);
						parse_success = 0;
						break;
					}
					if (lbl > max_label) max_label = lbl;
					++levels;
					labels[labels_offset++] = (Label) lbl;
				}

				if (record_index == 0) {
					levels_per_dimension[dimension_index] = (PathLength) levels;
					levels_sum += levels;
					u32 bits = msb32((u32) max_label);
					if ( (u32) max_label > (1u << bits)) ++bits;
					bits_per_dimension[dimension_index] = (u8) bits;
				}
				else {
					if (levels != levels_per_dimension[dimension_index]) {
						Print_cstr(&print, "Different number of levels for dimension ");
						Print_uint(&print, dimension_index);
						Print_cstr(&print, " on line ");
						Print_uint(&print, line);
						Print_cstr(&print, "\n");
						platform.write_to_file(pfh_stdout, print.begin, print.end);
						Print_clear(&print);
						parse_success = 0;
						break;
						//                         fprintf(stderr,"Different number of levels for dimension %d on line %d\n", dimension_index, line);
						//                         exit(-1);
					}
					u32 bits = msb32((u32) max_label);
					if ( (u32) max_label > (1u << bits)) ++bits;

					if (bits > bits_per_dimension[dimension_index])
						bits_per_dimension[dimension_index] = (u8) bits;
				}
				++dimension_index;
			}
		}

		if (record_index > 0 && dimension_index > 0 && dimension_index < dimensions) {
			Print_cstr(&print, "Last record is incomplete\n");
			platform.write_to_file(pfh_stdout, print.begin, print.end);
			Print_clear(&print);
			parse_success = 0;
		}
	}

	if (parse_success) {

		u64 records = record_index + 1;

		// prepare addresses
		LabelArrayList address[64]; // some max dimensions
		Assert(dimensions <= 64);
		for (int i=0;i<dimensions;++i) {
			address[i].next = (i < dimensions - 1) ? &address[i+1] : 0;
			// auto levels = nanocube_count->num_levels[i];
			// address[i].reset(paths + offset, levels);
			// offset += levels;
		}

		// mmap 1GB of virtual memory in this process
		// page aligned memory block
		PlatformMemory data_memory          = platform.allocate_memory(Gigabytes(1), 0, Terabytes(2));
		PlatformMemory insert_buffer_memory = platform.allocate_memory(Megabytes(32), 0, 0);

		Allocator*  allocator = Allocator_new(data_memory.memblock);
		Cache*  nanocube_index_cache = Allocator_create_cache(allocator, "NanocubeIndex", sizeof(NanocubeIndex));
		NanocubeIndex*  nanocube_index = (NanocubeIndex*) Cache_alloc(nanocube_index_cache);

		Array bit_array = Array_build(bits_per_dimension, (u8) dimensions);
		NanocubeIndex_init(nanocube_index, allocator, bit_array); // initialize nanocube (index is not initialized after this call)

		for (u64 i=0;i<records;++i) {

			Label* ptr = labels + i * levels_sum;
			for (int j=0;j<dimensions ;++j) {
				Array_init(&address[j].labels, ptr, levels_per_dimension[j]);
				ptr += levels_per_dimension[j];
			}

			// log records
			//             for (int j = 0; j < dimensions; ++j) {
			//                 fprintf(stderr, "[%d] -> ", j);
			//                 for (int k = 0; k < levels_per_dimension[j]; ++k) {
			//                     fprintf(stderr, "%d ", (int) Array_get(&address[j].labels,(u8) k) );
			//                 }
			//                 fprintf(stderr, "\n");
			//             }

			// assuming only one variable to be stored
			PayloadUnit payload_unit = (u32) i;

			// ready to insert record!
			NanocubeIndex_insert(nanocube_index, address, &payload_unit, insert_buffer_memory.memblock);

			// nanocube_index::print(nanocube_index);

			char filename[256];
			Print filename_print;
			Print_init(&filename_print, filename, filename + sizeof(filename) - 1);
			Print_cstr(&filename_print, input_filename);
			Print_char(&filename_print, '_');
			Print_uint(&filename_print, (u64) i);
			Print_cstr(&filename_print, ".dot");
			*filename_print.end++ = 0;
			save_dot_file(nanocube_index,filename_print.begin);

			//             if ((i % 10000) == 0) {
			//                 Print_cstr(&print, " Allocator usage after ");
			//                 Print_uint(&print, i);
			//                 Print_cstr(&print, " records in #pages is ");
			//                 Print_uint(&print, allocator->used_pages);
			//                 Print_char(&print, '\n');
			//                 platform.write_to_file(pfh_stdout, print.begin, print.end);
			//                 Print_clear(&print);
			//             }
		}

		log_memory(allocator, nanocube_index,&print);
		platform.write_to_file(pfh_stdout, print.begin, print.end);
		Print_clear(&print);

	}

	// nanocube_util::print(nanocube_index);
	platform.free_memory(mem.handle);

}
