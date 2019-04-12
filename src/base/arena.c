//------------------------------------------------------------------------------
// Memory a_Arena
// use the zero is initialization ZII
//------------------------------------------------------------------------------

#define a_DEBUG_LEVEL 0

#ifndef a_DEBUG_CHANNEL
#define a_DEBUG_CHANNEL stderr
#endif

#define a_print(st) fprintf(stderr, "%s", st)

#if a_DEBUG_LEVEL >= 0
#define a_log0(format, ...) fprintf(a_DEBUG_CHANNEL, "[%s] " format, __FUNCTION__, ##  __VA_ARGS__)
#else
#define a_log0(format, ...)
#endif

#if a_DEBUG_LEVEL > 0
#define a_log(format, ...) fprintf(a_DEBUG_CHANNEL, "[%s] " format, __FUNCTION__, ##  __VA_ARGS__)
#else
#define a_log(format, ...)
#endif

#if a_DEBUG_LEVEL > 1
#define a_log2(format, ...) fprintf(a_DEBUG_CHANNEL, "[%s] " format, __FUNCTION__, ##  __VA_ARGS__)
#else
#define a_log2(format, ...)
#endif

#if a_DEBUG_LEVEL > 2
#define a_log3(format, ...) fprintf(a_DEBUG_CHANNEL, "[%s] " format, __FUNCTION__, ##  __VA_ARGS__)
#else
#define a_log3(format, ...)
#endif

#ifndef a_DEFAULT_BLOCK_SIZE
#define a_DEFAULT_BLOCK_SIZE Megabytes(4)
#endif

typedef struct {
	void *base;
	u64 length;
} a_Block;

typedef struct {
	pt_Memory *current_block;
	u64 minimum_allocation_size;
	u64 allocation_flags;
} a_Arena;

typedef struct {
	u64 num_blocks;
	u64 used_memory;
	// total memory doesn't count platform specific space overhead
	// (overflow/underflow pages, pt_Memory*)
	u64 total_memory;
} a_Stats;

static void
a_init(a_Arena *self, u64 minimum_allocation_size, u64 allocation_flags)
{
	self->current_block = 0;
	self->minimum_allocation_size = minimum_allocation_size;
	self->allocation_flags = allocation_flags;
}

static void
a_free_last_block_(a_Arena *self)
{
	pt_Memory *free = self->current_block;
	if (free) {
		self->current_block = free->prev;
		a_log2("arena %p freeing more %.0fMB\n",self, BytesToMegabytes(free->size));
		platform.free_memory(free);
	}
}

static a_Stats
a_stats(a_Arena *self)
{
	a_Stats stats;
	pt_clear(stats);
	pt_Memory *it = self->current_block;
	if (it) {
		++stats.num_blocks;
		stats.used_memory += it->used;
		stats.total_memory += it->size;
		it = it->prev;
	}
	return stats;
}

static void
a_clear(a_Arena *self)
{
	while (self->current_block) {
		a_free_last_block_(self);
	}
}

// assumes alignment is a power of two
static u64
a_get_alignment_offset_pow2_(a_Arena *self, u64 alignment)
{
	u64 alignment_offset = 0;
	u64 current_used_offset_from_zero = (u64) self->current_block->base + self->current_block->used;
	// memory_index ResultPointer = (memory_index)a_Arena->CurrentBlock->Base + a_Arena->CurrentBlock->Used;
	u64 alignment_mask = alignment - 1; // expecting a number with only 11111 in its binary representation
	if(current_used_offset_from_zero & alignment_mask) {
		alignment_offset = alignment - (current_used_offset_from_zero & alignment_mask);
	}
	return(alignment_offset);
}

static void*
a_push(a_Arena* self, u64 size, u64 alignment, b8 zero_it)
{
	u64 alignment_offset = 0;
	u64 size_aligned= size;
	if (self->current_block) {
		alignment_offset = a_get_alignment_offset_pow2_(self, alignment);
		size_aligned = size + alignment_offset;
	}

	if (!self->current_block || (self->current_block->used + size_aligned > self->current_block->size)) {
		// default initialization
		if (self->minimum_allocation_size == 0) {
			self->minimum_allocation_size = a_DEFAULT_BLOCK_SIZE;
		}
		u64 allocation_size = Max(size_aligned, self->minimum_allocation_size);
		pt_Memory *new_block = platform.allocate_memory(allocation_size, self->allocation_flags);
		Assert(new_block);
		new_block->prev = self->current_block;
		self->current_block = new_block;

		alignment_offset = a_get_alignment_offset_pow2_(self, alignment);
		size_aligned = size + alignment_offset;

		a_log2("arena %p allocated more %.0fMB\n",self, BytesToMegabytes(allocation_size));
	}

	Assert(self->current_block);
	Assert(self->current_block->used + size_aligned <= self->current_block->size);

	void *result = self->current_block->base + self->current_block->used + alignment_offset;
	self->current_block->used += size_aligned;

	if (zero_it) {
		pt_zero(result, size);
	}

	return result;
}

static a_Block
a_push_block(a_Arena* self, u64 size, u64 alignment, b8 zero_it)
{
	return (a_Block) {
		.base = a_push(self, size, alignment, zero_it),
		.length = size
	};
}

#define a_push_array_(arena, type, count, alignment, zeroit) a_push(arena, sizeof(type) * count, alignment, zero_it)

// implement a checkpoint stacking mechanism on top of a memory arena
typedef struct {
	a_Arena *arena;
	pt_Memory *start_block;
	u64 start_block_used;
} a_Checkpoint;

static a_Checkpoint
a_checkpoint(a_Arena *self)
{
	return (a_Checkpoint) {
		.arena = self,
		.start_block = self->current_block,
		.start_block_used = self->current_block ? self->current_block->used : 0
	};
}

static void
a_pop(a_Checkpoint *self)
{
	while (self->arena->current_block != self->start_block) {
		a_free_last_block_(self->arena);
	}
	if (self->arena->current_block) {
		Assert(self->arena->current_block == self->start_block);
		self->arena->current_block->used = self->start_block_used;
	}
	// if we call free twice on a temp memory, it should crash
	// pt_clear(*self);
}



