// depends on "src/base/sort.c"

// TODO(llins): consolidate profile utility functions and move it to profile.c
#ifdef PROFILE

// assumes profile.h was already included

// #define pfc_PROFILE_TABLE_MEMORY_SIZE Megabytes(128)

//
// Infrastructure to collect result
//
// "./nanocube/nanocube_index.c" "|" "930" "|" "1" "|" "pt_read_bits"

#define pfc_Block_MAX_RECURSION_LEVEL 16

typedef struct {
	u64 hits[pfc_Block_MAX_RECURSION_LEVEL];
	u64 sum_cycles[pfc_Block_MAX_RECURSION_LEVEL];
	u64 min_cycles[pfc_Block_MAX_RECURSION_LEVEL];
	u64 max_cycles[pfc_Block_MAX_RECURSION_LEVEL];
	u32 levels_used;
	u32 recursion_level;
} pfc_Stats;

typedef struct {
	char *id;
	pfc_Stats *stats;
	u32 stats_index;
	u32 stats_capacity;
	pfc_Stats global_stats;
} pfc_Block;

typedef struct pfc_Node {
	pfc_Block *block;
	u64 cycles;
	u64 thread_id;
	struct pfc_Node *parent;
	struct pfc_Node *sibling;
	struct pfc_Node *children;
} pfc_Node;

typedef struct {
	/* blocks will be sorted by id (their name address) */
	pfc_Block *begin;
	pfc_Block *end;
	pfc_Block *capacity;
} pfc_BlockSet;

internal void
pfc_BlockSet_init(pfc_BlockSet *self, pfc_Block *begin, pfc_Block *capacity)
{
	self->begin = begin;
	self->end = begin;
	self->capacity = capacity;
}

internal void
pfc_Block_init(pfc_Block *self, char *id)
{
	self->id = id;
	self->stats = 0;
	self->stats_index = 0;
	self->stats_capacity = 0;
}

internal void
pfc_Block_initialize_stats(pfc_Block *self, pfc_Stats *stats, u32 count)
{
	Assert(self->stats_capacity == 0);
	self->stats = stats;
	self->stats_capacity = count;
	for (u32 i=0;i<count;++i) {
		pfc_Stats *stats_i = stats + i;
		stats_i->levels_used = 0;
		stats_i->recursion_level = 0;
		for (s32 j=0;j<pfc_Block_MAX_RECURSION_LEVEL;++j) {
			stats_i->hits[j] = 0;
			stats_i->sum_cycles[j] = 0;
			stats_i->min_cycles[j] = pt_MAX_U64;
			stats_i->max_cycles[j] = 0;
		}
	}
	self->stats_index = self->stats_capacity;
}

// return 1 when index is within range
// 0 when finished. After a return 0 next
// will cycle again
internal b8
pfc_Block_next_stats(pfc_Block *self)
{
	if (self->stats_index == self->stats_capacity) {
		self->stats_index = 0;
	} else {
		++self->stats_index;
	}
	return self->stats_index < self->stats_capacity;
}

internal void
pfc_Stats_insert(pfc_Stats *self, u64 cycles)
{
	Assert(self->recursion_level > 0);
	self->levels_used=Max(self->levels_used, self->recursion_level);
	u32 i = self->recursion_level-1;
	++self->hits[i];
	self->sum_cycles[i] += cycles;
	self->min_cycles[i] = Min(self->min_cycles[i], cycles);
	self->max_cycles[i] = Max(self->max_cycles[i], cycles);
}

internal void
pfc_Block_begin(pfc_Block *self)
{
	Assert(self->stats_index < self->stats_capacity);
	pfc_Stats *stats = self->stats + self->stats_index;
	++stats->recursion_level;
	++self->global_stats.recursion_level;
}

internal void
pfc_Block_end(pfc_Block *self, u64 cycles)
{
	pfc_Stats *stats = self->stats + self->stats_index;
	pfc_Stats_insert(stats, cycles);
	pfc_Stats_insert(&self->global_stats, cycles);
	--stats->recursion_level;
	--self->global_stats.recursion_level;
}

internal s64
pfc_BlockSet_indexof(pfc_BlockSet *self, char *id)
{
	s64 left  = 0;
	s64 right = self->end - self->begin;
	while (right - left > 2) {
		s64 middle = (left + right)/2;
		char *id_middle = self->begin[middle].id;
		if (id < id_middle) {
			right = middle;
		} else if (id > id_middle) {
			left = middle + 1;
		} else {
			return middle;
		}
	}
	Assert(right - left <= 2);

	/* solve problem for list of size at most 2 */
	if (left == right) {
		return -1;
	} else {
		char *id_left = self->begin[left].id;
		if (id < id_left) {
			return -left - 1;
		} else if (id == id_left) {
			return left;
		}

		if (left + 1 == right) {
			return -(left+1)-1;
		} else {
			char *id_left_plus_one = self->begin[left+1].id;
			if (id < id_left_plus_one) {
				return -(left+1)-1;
			} else if (id == id_left_plus_one) {
				return left+1;
			} else {
				return -(left+2)-1;
			}
		}
	}
}

//
// TODO(llins) allow for sorting using hits, avg.cy or min.cy also
// for now only cy is used
//
internal sort_Entry*
pfc_BlockSet_sort(pfc_BlockSet *self, BilinearAllocator *memory)
{
	s32 n = (s32) (self->end - self->begin);
	sort_Entry *perm_tmp = (sort_Entry*) BilinearAllocator_alloc_right(memory, n * sizeof(sort_Entry));
	sort_Entry *perm     = perm_tmp + n;
	for (s32 i=0;i<n;++i) {
		perm[i].index = i;
		/* reverse bits */
		perm[i].key   = ~self->begin[i].global_stats.sum_cycles[0];
	}
	sort_radixsort(perm, perm_tmp, n);
	BilinearAllocator_pop_right(memory, n * sizeof(sort_Entry));
	return perm;
}

internal pfc_Block*
pfc_BlockSet_find(pfc_BlockSet *self, char *id)
{
	s64 index = pfc_BlockSet_indexof(self, id);
	if (index < 0) {
		return 0;
	} else {
		return self->begin + index;
	}
}

internal u64
pfc_BlockSet_fit(pfc_BlockSet *self)
{
	u64 slots_freed = self->capacity - self->end;
	self->capacity = self->end;
	return slots_freed;
}


/* assuming insertion ids will come in order */
/* allows multiple insertion of the same id (if it is the largest one) */
internal void
pfc_BlockSet_insert(pfc_BlockSet *self, char *id)
{
	if (self->begin == self->end) {
		pfc_Block_init(self->end, id);
		++self->end;
	} else {
		char *last_id = (self->end-1)->id;
		Assert(last_id <= id);
		if (last_id < id) {
			pfc_Block_init(self->end, id);
			++self->end;
		}
	}
}

#define pfc_collect_MAX_THREADS 128

internal pfc_BlockSet*
pfc_collect(pf_Event *begin, u32 count, BilinearAllocator *memory)
{
	u32 n = count;

	/* allocate enough memory to radix sort events by id */
	sort_Entry *perm_tmp = (sort_Entry*) BilinearAllocator_alloc_right(memory, 2 * n * sizeof(sort_Entry));
	sort_Entry *perm = perm_tmp + n;

	// get permutation by id
	for (s64 i=0;i<n;++i) {
		perm[i].key = (u64) begin[i].id;
		perm[i].index = i;
	}
	sort_radixsort(perm, perm_tmp, n);

	/* create list of blocks */
	pfc_BlockSet *block_set = (pfc_BlockSet*) BilinearAllocator_alloc_left(memory, sizeof(pfc_BlockSet));
	{
		u64 c = BilinearAllocator_free_space(memory)/sizeof(pfc_Block);
		pfc_Block *blocks_begin = (pfc_Block*) BilinearAllocator_alloc_left(memory, c * sizeof(pfc_Block));
		pfc_BlockSet_init(block_set, blocks_begin, blocks_begin + c);
		for (u32 i=0;i<n;++i) {
			pf_Event *event = begin + perm[i].index;
			if (event->type == pf_BEGIN_EVENT) {
				pfc_BlockSet_insert(block_set, event->id);
			}
		}
		u64 freed_slots = pfc_BlockSet_fit(block_set);
		/* pop memory not used for blocks */
		BilinearAllocator_pop_left(memory, freed_slots * sizeof(pfc_Block));
	}

	// get permutation by thread_id (stable sort, so cycles are monotonous within block)
	for (s64 i=0;i<n;++i) {
		perm[i].key = (u64) begin[i].thread_id;
		perm[i].index = i;
	}
	sort_radixsort(perm, perm_tmp, n);

	BilinearAllocator_pop_right(memory, sizeof(sort_Entry) * n);
	u32 *cuts = (u32*) BilinearAllocator_alloc_right(memory, sizeof(u32) * (pfc_collect_MAX_THREADS + 1));
	s32 max_thread= 0;
	cuts[max_thread] = 0;
	for (s64 i=1;i<n;++i) {
		pf_Event *prev = begin + perm[i-1].index;
		pf_Event *curr = begin + perm[i].index;
		if (prev->thread_id < curr->thread_id) {
			Assert(max_thread < pfc_collect_MAX_THREADS-1);
			++max_thread;
			cuts[max_thread] = i;
		}
	}
	u32 num_threads = max_thread + 1;
	cuts[num_threads] = n;

	/* allocate stats space of block threads */
	pfc_Block *it = block_set->begin;
	while (it != block_set->end) {
		pfc_Stats *stats = (pfc_Stats*) BilinearAllocator_alloc_left(memory, sizeof(pfc_Stats) * num_threads);
		pfc_Block_initialize_stats(it, stats, num_threads);
		++it;
	}

	pf_Event *stack[128];
	for (u32 thread=0; thread < num_threads; ++thread) {
		u32 a = cuts[thread];
		u32 b = cuts[thread+1];

		pfc_Block *it = block_set->begin;
		while (it != block_set->end) {
			pfc_Block_next_stats(it);
			++it;
		}

		u32 stack_size = 0;
		for (u32 i=a;i<b;++i) {
			pf_Event *event = begin + perm[i].index;
			if (event->type == pf_BEGIN_EVENT) {
				Assert(stack_size < ArrayCount(stack));
				stack[stack_size] = event;
				pfc_Block *block = pfc_BlockSet_find(block_set, event->id);
				Assert(block);
				pfc_Block_begin(block);
				++stack_size;
			} else if (event->type == pf_END_EVENT) {
				Assert(stack_size > 0);
				--stack_size;
				pf_Event *begin_event = stack[stack_size];
				pfc_Block *block = pfc_BlockSet_find(block_set, begin_event->id);
				Assert(block);
				pfc_Block_end(block, event->clock - begin_event->clock);
			}
		}
		Assert(stack_size == 0);
	}

	return block_set;
}

internal inline void
pfc_clear_events()
{
	pt_atomic_exchange_u64(&global_profile_table->slot_event_index, 0);
}

#define pfc_ReportBuffers_collect_memory Megabytes(256)
#define pfc_ReportBuffers_print_memory   Megabytes(4)

//
// This part assumes memory arena is in context
// arena should be in context everytime we have
// included platform
//

typedef struct {
	a_Arena           arena;
	BilinearAllocator collect_memory;
	Print             print;
	nt_Tokenizer      tok;
} pfc_ReportBuffers;

global_variable pfc_ReportBuffers pfc_report;

internal void
pfc_begin()
{
	pt_clear(pfc_report);

	void *collect_buffer = a_push(&pfc_report.arena, pfc_ReportBuffers_collect_memory, 8, 1);
	void *print_buffer   = a_push(&pfc_report.arena, pfc_ReportBuffers_print_memory, 8, 1);

	BilinearAllocator_init(&pfc_report.collect_memory, collect_buffer, pfc_ReportBuffers_collect_memory);
	Print_init(&pfc_report.print, print_buffer, pfc_ReportBuffers_print_memory);

	static char sep[] = "|";
	nt_Tokenizer_init_canonical(&pfc_report.tok, sep, cstr_end(sep));
	pfc_clear_events();
}

internal void
pfc_end()
{
	a_clear(&pfc_report.arena);
}

internal void
pfc_generate_report_print_level(Print *print, pfc_Stats *stats, s32 level, u64 frame_cycles,
			       char *name_begin, char *name_end, s32 thread_index)
{
	// TODO(llins): Improve infrastructure to print. Use stb ideas.
	Print_str(print, name_begin, name_end);
	Print_align(print, 36, -1, ' ');

	// thread
	Print_cstr(print, " t");
	if (thread_index < 0) {
		Print_char(print, '*');
	} else {
		Print_u64(print, thread_index);
	}
	Print_align(print, 2, -1, ' ');

	Print_cstr(print," [");
	Print_u64(print, level);
	Print_align(print, 2, 1, ' ');
	Print_cstr(print,"]");

	Print_cstr(print," ht: ");
	Print_u64(print,stats->hits[0]);
	Print_align(print, 10, 1, ' ');

	Print_cstr(print,"  cy: ");
	Print_u64(print,stats->sum_cycles[0]);
	Print_align(print, 12, 1, ' ');

	Print_cstr(print," [");
	Print_u64(print,(100*stats->sum_cycles[0])/frame_cycles);
	Print_align(print, 3, 1, ' ');
	Print_cstr(print,"%]");

	Print_cstr(print,"  cy/ht{ avg: ");
	if (stats->hits[0] > 0) {
		Print_u64(print,stats->sum_cycles[0]/stats->hits[0]);
	} else {
		Print_u64(print,0);
	}
	Print_align(print, 8, 1, ' ');
	Print_cstr(print,"  min: ");
	Print_u64(print,stats->min_cycles[0]);
	Print_align(print, 8, 1, ' ');
	Print_cstr(print,"  max: ");
	Print_u64(print,stats->max_cycles[0]);
	Print_align(print, 8, 1, ' ');
	Print_cstr(print," }\n");
}

internal void
pfc_generate_log_events()
{
	Print *print = &pfc_report.print;

	u32 slot  = global_profile_table->slot_event_index >> 32;
	u32 count = global_profile_table->slot_event_index & 0xFFFFFFFF;

	pf_Event *events = global_profile_table->events[slot];

	Print_clear(print);
	Print_cstr(print,"clock|file|line|counter|name|thread_id|core_index|type\n");
	for (u32 i=0;i<count;++i) {
		pf_Event event = events[i];
		Print_u64(print, event.clock);
		Print_char(print, '|');
		Print_cstr(print, event.id);
		Print_char(print, '|');
		Print_u64(print, event.thread_id);
		Print_char(print, '|');
		Print_u64(print, event.core_index);
		Print_char(print, '|');
		Print_u64(print, (u64) event.type);
		Print_char(print, '\n');
	}
}



internal void
pfc_generate_report()
{
	u32 slot  = global_profile_table->slot_event_index >> 32;
	u32 count = global_profile_table->slot_event_index & 0xFFFFFFFF;

	u64 frame_cycles = 1;

	if (count == 0)
		return;

	BilinearAllocator_clear(&pfc_report.collect_memory);

	pfc_BlockSet *block_set = pfc_collect(global_profile_table->events[slot], count, &pfc_report.collect_memory);

	sort_Entry* order = pfc_BlockSet_sort(block_set, &pfc_report.collect_memory);
	u32 num_blocks = block_set->end - block_set->begin;

	/* sort by find max sum_cycles */
	nt_Tokenizer *tok = &pfc_report.tok;
	Print *print = &pfc_report.print;

	/* print a report */
	Print_clear(print);
	Print_cstr(print,"----------------\n");
	for (u32 i=0;i<num_blocks;++i) {
		u32 j = order[i].index;
		pfc_Block *it = block_set->begin + j;
		if (it->global_stats.levels_used == 0) {
			continue;
		}

		if (i == 0) {
			frame_cycles = it->global_stats.sum_cycles[0];
		}

		nt_Tokenizer_reset_text(tok, it->id, cstr_end(it->id));
		nt_Tokenizer_next(tok);
		// nt_Token filename = tok.token;
		nt_Tokenizer_next(tok);
		// nt_Token line = tok.token;
		nt_Tokenizer_next(tok);
		// nt_Token counter = tok.token;
		nt_Tokenizer_next(tok);
		nt_Token name = tok->token;

		// TODO(llins): Improve infrastructure to print. Use stb ideas.
		pfc_Stats *stats = &it->global_stats;
		for (u32 level=0;level<stats->levels_used;++level) {
			pfc_generate_report_print_level(print, stats, level, frame_cycles, name.begin, name.end, -1);
		}
		if (it->stats_capacity > 1) {
			for (s32 thread_id=0; thread_id < it->stats_capacity; ++thread_id) {
				stats = it->stats + thread_id;
				for (u32 level=0;level<stats->levels_used;++level) {
					pfc_generate_report_print_level(print, stats, level, frame_cycles,
								       name.begin, name.end, thread_id);
				}
			}
		}
	}
}

#endif

