/*
BEGIN_NOTES
# 2017-05-26T19:14
Queries are evaluated to nm_Table which has two parts,
one is common for all possible nanocubes: the nm_TableKeys
part. The other part is based on the payload specifics.
Note that a nm_Table is based on a low level encoding.
The query responses corresponding to a nm_Table, might
still be conveniently formatted to the user.
For example, on a calendar date relative query we might
want the low level binary tree path or an actual
calendar formatted date.

We would like a nice way to serialize a table based on
a common part and on a custom part (payload).

Should we include a FormattedTable. Seems inefficient
if we cannot stream from a nv_Table directly into
a serialized formatted table. We can do it:

# 2017-06-09T11:06:24
Watermark the version number into the index structure
so that we can always track which version created which
index.

END_NOTES
*/

//------------------------------------------------------------------------------
// NanocubeVector
//------------------------------------------------------------------------------

#define nv_MAX_NAME_LENGTH 31
#define nv_MAX_MEASURE_DIMENSIONS 32

typedef enum {
	nv_NUMBER_STORAGE_SIGNED_32,
	nv_NUMBER_STORAGE_SIGNED_64,
	nv_NUMBER_STORAGE_UNSIGNED_32,
	nv_NUMBER_STORAGE_UNSIGNED_64,
	nv_NUMBER_STORAGE_FLOAT_32,
	nv_NUMBER_STORAGE_FLOAT_64
} nv_NumberStorage;

typedef struct {
	u64  num_index_dimensions:   16;
	u64  num_measure_dimensions: 16;
	u64  payload_aggregate_size: 16;
	u64  initialized: 1;
	u64  index_initialized: 1;
	u64  key_value_store_initialized: 1;
	u64  padding: 13; /* without filling the full 64 bits, compiler might compress this */

	// would like to tag index dimensions with a hint of its intended use or context
	// will add this into the key value store
	struct {
		u8   bits_per_level[nx_MAX_NUM_DIMENSIONS];
		u8   num_levels[nx_MAX_NUM_DIMENSIONS];
		char names[nx_MAX_NUM_DIMENSIONS][nv_MAX_NAME_LENGTH+1];
	} index_dimensions;

	struct {
		nv_NumberStorage storage[nv_MAX_MEASURE_DIMENSIONS];
		char names[nx_MAX_NUM_DIMENSIONS][nv_MAX_NAME_LENGTH+1];
		u16 offsets[nv_MAX_MEASURE_DIMENSIONS];
	} measure_dimensions;

	nx_NanocubeIndex index;

	al_Ptr_Cache cache_payload;

	bt_BTree key_value_store;

	// @TODO(llins): include btree for annotations
} nv_Nanocube;

/*
 * nv_Selection holds a lisd of names that
 * should be related to measure dimension names
 *
 * taxirides.select("count","tip","fare")
 */
typedef struct {
	MemoryBlock *begin;
	MemoryBlock *end;
} nv_Selection;

/*
 * nv_TableValues: how do we represent query results coming
 * from a nv_Nanocube
 */
typedef struct {
	struct {
		f64 *begin;
		f64 *end;
	} values;
	struct {
		MemoryBlock *begin;
		MemoryBlock *end;
	} names;
	struct {
		u32 *begin;
		u32 *end;
	} src_indices;
	u32 columns;
	u32 rows;
	LinearAllocator *memsrc;
} nv_TableValues;

/*
 * @TODO(llins): bring closely tied services of the types above into a
 *  single file (make this .h become this new .c)
 */

//------------------------------------------------------------------------------
// nv_NumberStorage
//------------------------------------------------------------------------------

internal char *nv_storage_names[6] = { "s32", "s64", "u32", "u64", "f32", "f64" };

internal char*
nv_storage_name_cstr(nv_NumberStorage s)
{
	switch(s) {
	case nv_NUMBER_STORAGE_SIGNED_32:
		return nv_storage_names[0];
	case nv_NUMBER_STORAGE_SIGNED_64:
		return nv_storage_names[1];
	case nv_NUMBER_STORAGE_UNSIGNED_32:
		return nv_storage_names[2];
	case nv_NUMBER_STORAGE_UNSIGNED_64:
		return nv_storage_names[3];
	case nv_NUMBER_STORAGE_FLOAT_32:
		return nv_storage_names[4];
	case nv_NUMBER_STORAGE_FLOAT_64:
		return nv_storage_names[5];
	default:
		Assert(0);
		return 0;
	}
}

//------------------------------------------------------------------------------
// nv_Nanocube
//------------------------------------------------------------------------------

internal void
nv_Nanocube_init(nv_Nanocube* self)
{
	char *p = (char*) self;
	pt_fill(p, p + sizeof(nv_Nanocube), 0);

	self->num_index_dimensions = 0;
	self->num_measure_dimensions = 0;
	self->initialized = 1;
	self->index_initialized = 0;
	self->payload_aggregate_size = 0;
	self->key_value_store_initialized = 0;
}

internal al_Allocator*
nv_Nanocube_get_allocator(nv_Nanocube* self)
{
	al_Cache *cache = al_Ptr_Cache_get(&self->cache_payload);
	al_Allocator *allocator = al_Ptr_Allocator_get(&cache->allocator_p);
	return allocator;
}

internal void
nv_Nanocube_insert_index_dimension(nv_Nanocube *self, u8 bits_per_level,
				   u8 num_levels, char *name_begin, char *name_end)
{
	Assert(self->initialized && "NanocubeCount not initialized!");
	Assert(!self->index_initialized && "Cannot insert dimension when index is initialized");
	Assert(self->num_index_dimensions < nx_MAX_NUM_DIMENSIONS);

	u64 i = self->num_index_dimensions;
	self->index_dimensions.bits_per_level[i] = bits_per_level;
	self->index_dimensions.num_levels[i]     = num_levels;
	s32 name_len = pt_copy_bytes(name_begin, name_end, self->index_dimensions.names[i],
				     self->index_dimensions.names[i] + nv_MAX_NAME_LENGTH);
	self->index_dimensions.names[i][name_len] = 0;

	++self->num_index_dimensions;
}

// typedef struct nx_PayloadAggregate {
// 	al_Ptr_char data;
// } nx_PayloadAggregate;


internal f64
nv_Nanocube_get_value(nv_Nanocube *self, u32 index, nx_PayloadAggregate *agg)
{
	Assert(index < self->num_measure_dimensions);
	char *p = al_Ptr_char_get(&agg->data) +
		self->measure_dimensions.offsets[index];
	f64 result = 0;
	switch(self->measure_dimensions.storage[index]) {
	case nv_NUMBER_STORAGE_SIGNED_32: {
		result = (f64) *((s32*) p);
	} break;
	case nv_NUMBER_STORAGE_SIGNED_64: {
		result = (f64) *((s64*) p);
	} break;
	case nv_NUMBER_STORAGE_UNSIGNED_32: {
		result = (f64) *((u32*) p);
	} break;
	case nv_NUMBER_STORAGE_UNSIGNED_64: {
		result = (f64) *((u64*) p);
	} break;
	case nv_NUMBER_STORAGE_FLOAT_32: {
		result = (f64) *((f32*) p);
	} break;
	case nv_NUMBER_STORAGE_FLOAT_64: {
		result = (f64) *((f64*) p);
	} break;
	default: {
		Assert(0 && "unexpected type");
	} break;
	}
	return result;
}

internal void
nv_Nanocube_insert_measure_dimension(nv_Nanocube *self, nv_NumberStorage storage,
				     char *name_begin, char *name_end)
{
	Assert(self->initialized && "NanocubeCount not initialized!");
	Assert(!self->index_initialized && "Cannot insert dimension when index is initialized");
	Assert(self->num_measure_dimensions < nv_MAX_MEASURE_DIMENSIONS);

	u64 i = self->num_measure_dimensions;
	self->measure_dimensions.storage[i] = storage;
	s32 name_len =
		pt_copy_bytes(name_begin,
			      name_end,
			      self->measure_dimensions.names[i],
			      self->measure_dimensions.names[i]
			      + nv_MAX_NAME_LENGTH);
	self->measure_dimensions.names[i][name_len] = 0;

	self->measure_dimensions.offsets[i] = (u16) self->payload_aggregate_size;

	switch(storage) {
	case nv_NUMBER_STORAGE_SIGNED_32: {
		self->payload_aggregate_size += sizeof(s32);
	} break;
	case nv_NUMBER_STORAGE_SIGNED_64: {
		self->payload_aggregate_size += sizeof(s64);
	} break;
	case nv_NUMBER_STORAGE_UNSIGNED_32: {
		self->payload_aggregate_size += sizeof(s32);
	} break;
	case nv_NUMBER_STORAGE_UNSIGNED_64: {
		self->payload_aggregate_size += sizeof(s64);
	} break;
	case nv_NUMBER_STORAGE_FLOAT_32: {
		self->payload_aggregate_size += sizeof(f32);
	} break;
	case nv_NUMBER_STORAGE_FLOAT_64: {
		self->payload_aggregate_size += sizeof(f64);
	} break;
	default: {
		Assert(0 && "unexpected type");
	} break;
	}

	++self->num_measure_dimensions;
}

internal void
nv_Nanocube_init_index(nv_Nanocube *self, al_Allocator* allocator)
{
	Assert(self->initialized && self->num_index_dimensions > 0 && self->num_measure_dimensions > 0 && !self->index_initialized);

	nx_Array num_bits_array = nx_Array_build(self->index_dimensions.bits_per_level, (u8) self->num_index_dimensions);
	nx_NanocubeIndex_init(&self->index, allocator, num_bits_array);

	// initialize payload cache
	char *name = "nv_payload";
	u64 size = self->payload_aggregate_size;
	if (size < 8) {
		size = 8;
	}
	al_Ptr_Cache_set(&self->cache_payload, al_Allocator_create_cache (allocator,name,size));
	self->index_initialized = 1;
}

internal void
nv_Nanocube_init_key_value_store(nv_Nanocube *self, al_Allocator* allocator)
{
	bt_BTree_init(&self->key_value_store, allocator);
	self->key_value_store_initialized = 1;
}

internal s32
nv_Nanocube_measure_index_by_name(nv_Nanocube *self, char *measure_name_begin,
				  char *measure_name_end)
{
	u64 n = self->num_measure_dimensions;
	for (u64 i=0;i<n;++i) {
		char *name = self->measure_dimensions.names[i];
		if (pt_compare_memory(measure_name_begin, measure_name_end,
				      name, cstr_end(name)) == 0) {
			return (s32) i;
		}
	}
	return -1;
}


internal b8
nv_Nanocube_insert_key_value(nv_Nanocube *self,
			     char *key_begin,   char *key_end,
			     char *value_begin, char *value_end)
{
	bt_BTree_insert(&self->key_value_store, key_begin, key_end, value_begin, value_end);
	return 1;
}

// TODO(llins): try to replace dim_name with a unique index_dimension id in the parameters of this procedure
internal b8
nv_Nanocube_set_dimension_path_name(nv_Nanocube *self, char *dim_name_begin, char *dim_name_end, u8 *path, u8 path_length,
				    char *value_begin, char *value_end, Print *print)
{
	char *key_begin, *key_end;
	b8 ok;

	// set kv mapping
	key_begin = print->end;
	Print_str(print, dim_name_begin, dim_name_end);
	Print_cstr(print, ":kv:");
	for (s32 i=0;i<path_length;++i) {
		if (i > 0)
			Print_cstr(print, ":");
		Print_u64(print, path[i]);
	}
	key_end = print->end;
	ok = nv_Nanocube_insert_key_value(self, key_begin, key_end, value_begin, value_end);
	print->end = key_begin;
	if (!ok) {
		return 0;
	}

	// set vk mapping (reverse mapping: from name to value)
	key_begin = print->end;
	Print_str(print, dim_name_begin, dim_name_end);
	Print_cstr(print, ":vk:");
	Print_str(print, value_begin, value_end);
	key_end = print->end;

	// note: changing original value_begin and value_end since it won't be used anymore
	value_begin = key_end;
	for (s32 i=0;i<path_length;++i) {
		if (i > 0)
			Print_cstr(print, ":");
		Print_u64(print, path[i]);
	}
	value_end = print->end;

	ok = nv_Nanocube_insert_key_value(self, key_begin, key_end, value_begin, value_end);
	print->end = key_begin;
	if (!ok) {
		return 0;
	}

	return 1;
}

// TODO(llins): try to replace dim_name with a unique index_dimension id in the parameters of this procedure
internal MemoryBlock
nv_Nanocube_get_dimension_path_name(nv_Nanocube *self, char *dim_name_begin, char *dim_name_end, u8 *path, u8 path_length, Print *print)
{
	MemoryBlock mem = { .begin = 0, .end = 0 };

	char *key_begin, *key_end;

	// set kv mapping
	key_begin = print->end;
	Print_str(print, dim_name_begin, dim_name_end);
	Print_cstr(print, ":kv:");
	for (s32 i=0;i<path_length;++i) {
		if (i > 0)
			Print_cstr(print, ":");
		Print_u64(print, path[i]);
	}
	key_end = print->end;
	print->end = key_begin; // on return the print original content from begin to end should be the same

	// if nothing was found assuming mem was not touched
	bt_BTree_get_value(&self->key_value_store, key_begin, key_end, &mem);
	return mem;
}


// TODO(llins): try to replace dim_name with a unique index_dimension id in the parameters of this procedure
internal b8
nv_Nanocube_set_dimension_hint(nv_Nanocube *self, char *dim_name_begin, char *dim_name_end, char *value_begin, char *value_end, Print *print)
{
	char *key_begin, *key_end;
	b8 ok;

	// set kv mapping
	key_begin = print->end;
	Print_str(print, dim_name_begin, dim_name_end);
	Print_cstr(print, ":hint");
	key_end = print->end;
	ok = nv_Nanocube_insert_key_value(self, key_begin, key_end, value_begin, value_end);
	print->end = key_begin;
	if (!ok) {
		return 0;
	}

	return 1;
}

internal b8
nv_Nanocube_set_dimension_hint_cstr(nv_Nanocube *self, char *dim_name_begin, char *dim_name_end, char *value_cstr, Print *print)
{
	return	nv_Nanocube_set_dimension_hint(self, dim_name_begin, dim_name_end, value_cstr, cstr_end(value_cstr), print);
}


// TODO(llins): try to replace dim_name with a unique index_dimension id in the parameters of this procedure
internal MemoryBlock
nv_Nanocube_get_dimension_hint(nv_Nanocube *self, char *dim_name_begin, char *dim_name_end, Print *print)
{
	MemoryBlock mem = { .begin = 0, .end = 0 };

	char *key_begin, *key_end;

	// set kv mapping
	key_begin = print->end;
	Print_str(print, dim_name_begin, dim_name_end);
	Print_cstr(print, ":hint");
	key_end = print->end;
	print->end = key_begin;

	// if nothing was found assuming mem was not touched
	bt_BTree_get_value(&self->key_value_store, key_begin, key_end, &mem);
	return mem;
}

internal b8
nv_Nanocube_get_time_binning(nv_Nanocube *self, u8 dim_index, nm_TimeBinning *time_binning)
{
	Assert(dim_index < self->num_index_dimensions);
	Assert(self->index_dimensions.bits_per_level[dim_index] == 1);

	char buffer[32];
	Print print;
	Print_init(&print, buffer, buffer + sizeof(buffer));
	Print_cstr(&print, self->index_dimensions.names[dim_index]);
	Print_cstr(&print, ":time_binning");

	MemoryBlock mem;
	if (!bt_BTree_get_value(&self->key_value_store, print.begin, print.end, &mem)) {
		return 0;
	}

	Assert(mem.end - mem.begin == sizeof(nm_TimeBinning));
	pt_copy_bytes(mem.begin, mem.end, (char*) time_binning, (char*) time_binning + sizeof(nm_TimeBinning));

	return 1;
}

internal b8
nv_Nanocube_get_alias_path(nv_Nanocube *self, u8 dim_index, MemoryBlock alias, s32 output_buffer_capacity, u8 *output_buffer, s32 *output_length)
{
	// @todo implement this
	Assert(dim_index < self->num_index_dimensions);

	char buffer[Kilobytes(1)]; // it would be good to have a local temp storage more capable here
	Print print;
	Print_init(&print, buffer, buffer + sizeof(buffer));
	Print_cstr(&print, self->index_dimensions.names[dim_index]);
	Print_cstr(&print, ":vk:");
	Print_str(&print, alias.begin, alias.end);

	MemoryBlock mem;
	if (!bt_BTree_get_value(&self->key_value_store, print.begin, print.end, &mem)) {
		return 0;
	}

	// expecting 0:12:32:1:0 like strings
	s32 len = 0;
	char *it  = mem.begin;
	char *end = mem.end;
	while (it <= end) {
		char *sep = pt_find_char(it, end, ':');
		u32 val = 0;
		if (!pt_parse_u32(it, sep, &val)) {
			return 0;
		}
		Assert(val < 256);
		Assert(len < output_buffer_capacity);
		output_buffer[len] = (u8) val;
		++len;
		it = sep + 1;
	}
	*output_length = len;
	return 1;
}

internal MemoryBlock
nv_Nanocube_get_key_value(nv_Nanocube *self, char *key_begin, char *key_end)
{
	MemoryBlock mem = { .begin = 0, .end = 0 };
	// if nothing was found assuming mem was not touched
	bt_BTree_get_value(&self->key_value_store, key_begin, key_end, &mem);
	return mem;
}

internal void
nv_TableValues_print(nv_TableValues *self, Print *print, u32 record_index)
{
	Assert(record_index < self->rows);
	f64* it = self->values.begin + record_index * self->columns;
	for (u32 i=0;i<self->columns;++i) {
		Print_f64(print, *it);
		Print_align(print, 14, 1,' ');
		++it;
	}
}

internal void
nv_TableValues_print_header(nv_TableValues *self, Print *print)
{
	MemoryBlock* it = self->names.begin;
	for (u32 i=0;i<self->columns;++i) {
		Print_str(print, it->begin, it->end);
		Print_align(print, 14, 1,' ');
		++it;
	}
}

internal void
nv_TableValues_print_json(nv_TableValues *self, Print *print)
{
	Print_cstr(print, "[");
	u32 record_size = self->columns;
	for (u32 j=0;j<self->columns;++j) {
		if (j > 0) {
			Print_char(print, ',');
		}
		Print_cstr(print, "{\"name\":\"");
		MemoryBlock *name =self->names.begin + j;
		Print_str(print, name->begin, name->end);
		Print_cstr(print, "\",\"values\":[");
		f64* it = self->values.begin + j;
		for (u32 i=0;i<self->rows;++i) {
			if (i > 0) {
				Print_char(print, ',');
			}
			Print_f64(print, *it);
			it += record_size;
		}
		Print_cstr(print, "]}");
	}
	Print_cstr(print, "]");
}

internal void
nv_TableValues_init(nv_TableValues *self, LinearAllocator *memsrc)
{
	self->memsrc             = memsrc;
	self->values.begin       = 0;
	self->values.end         = 0;
	self->names.begin        = 0;
	self->names.end          = 0;
	self->src_indices.begin  = 0;
	self->src_indices.end    = 0;
	self->columns            = 0;
	self->rows               = 0;
}

internal void
nv_TableValues_init_column_names(nv_TableValues *self, MemoryBlock *begin, MemoryBlock *end)
{
	Assert(self->names.begin == 0);
	u32 n = pt_safe_s64_u32(end - begin);
	self->columns = n;
	MemoryBlock *names_copy= (MemoryBlock*) LinearAllocator_alloc(self->memsrc, n * sizeof(MemoryBlock));
	self->names.begin = names_copy;
	self->names.end   = names_copy + n;
	for (u32 i=0;i<n;++i) {
		MemoryBlock *name = begin + i;
		u64 name_len = pt_safe_s64_u64(name->end - name->begin);
		Assert(name_len > 0);
		MemoryBlock *name_copy = names_copy + i;
		name_copy->begin = (char*) LinearAllocator_alloc(self->memsrc, RALIGN(name_len,8));
		name_copy->end = name_copy->begin + name_len;
		pt_copy_bytes(name->begin, name->end, name_copy->begin, name_copy->end);
	}
}

internal void
nv_TableValues_init_column_names_uninitialized(nv_TableValues *self, u32 columns)
{
	Assert(self->names.begin == 0);
	self->columns = columns;
	MemoryBlock *names = (MemoryBlock*) LinearAllocator_alloc(self->memsrc, columns * sizeof(MemoryBlock));
	self->names.begin = names;
	self->names.end   = names + columns;
	for (u32 i=0;i<columns;++i) {
		MemoryBlock *name = names + i;
		name->begin = 0;
		name->end = 0;
	}
}

internal void
nv_TableValues_set_column_name(nv_TableValues *self, u32 index, char *name_begin, char *name_end)
{
	Assert(self->rows == 0);
	Assert(self->src_indices.begin == 0);
	u64 name_len = pt_safe_s64_u64(name_end - name_begin);
	Assert(name_len > 0);
	MemoryBlock *name_copy = self->names.begin + index;
	name_copy->begin = (char*)
		LinearAllocator_alloc(self->memsrc, RALIGN(name_len,8));
	name_copy->end = name_copy->begin + name_len;
	pt_copy_bytes(name_begin, name_end,
		      name_copy->begin,
		      name_copy->end);
}

internal void
nv_TableValues_init_src_indices(nv_TableValues *self, u32 *begin, u32 *end)
{
	Assert(self->rows == 0 && self->columns > 0);
	Assert(self->names.begin != 0);
	if (begin != 0) {
		Assert(end - begin == self->columns);
	}

	u32 *it_src = begin;
	u32 *it_dst = (u32*) LinearAllocator_alloc(self->memsrc,
					self->columns * sizeof(u32));

	self->src_indices.begin = it_dst;
	self->src_indices.end = it_dst + self->columns;

	while (it_src != end) {
		*it_dst = *it_src;
		++it_dst;
		++it_src;
	}
}

//------------------------------------------------------------------------------
// Payload Table Algebra
//------------------------------------------------------------------------------

#if 1

/* Assume a global linear allocator */
static u64 nv_TABLE_VALUE_MAX_SIZE=Megabytes(8);

internal nv_TableValues*
nv_new_table_values(LinearAllocator *nv_table_value_allocator)
{
	/*
	 * @TODO(llins): improve the current design of having a
	 * hard coded limit of 32MB per table. Ability to free
	 * this memory is also important
	 */
	Assert(nv_table_value_allocator);
	char *mem = LinearAllocator_alloc_aligned(nv_table_value_allocator, nv_TABLE_VALUE_MAX_SIZE, 8);
	LinearAllocator *memsrc = (LinearAllocator*) mem;
	mem += RALIGN(sizeof(LinearAllocator),8);
	LinearAllocator_init(memsrc, mem, mem, mem + nv_TABLE_VALUE_MAX_SIZE);

	/* init empty table values */
	nv_TableValues *result = (nv_TableValues*) LinearAllocator_alloc(memsrc, RALIGN(sizeof(nv_TableValues),8));
	nv_TableValues_init(result, memsrc);

	return result;
}

// #define nm_TABLE_VALUES_CREATE(name)
// 	nm_TableValuesHandle name(void *nanocube, void *payload_config, void *allocation_context)
nm_TABLE_VALUES_CREATE(nv_tv_create)
{
	nv_Nanocube  *cube = (nv_Nanocube*) nanocube;
	nv_Selection *selection = (nv_Selection*) payload_config;

	u32 n = selection
		? pt_safe_s64_u32(selection->end - selection->begin)
		: (u32) cube->num_measure_dimensions;

	nv_TableValues *result = nv_new_table_values((LinearAllocator*) allocation_context);
	Assert(result);

	/*
	 * create array of n*MemoryBlock and initialize it
	 * with name copies form the selection: self contained
	 * table
	 */
	if (selection) {
		nv_TableValues_init_column_names(result, selection->begin, selection->end);
	} else {
		nv_TableValues_init_column_names_uninitialized(result, n);
		for (u32 i=0;i<n;++i) {
			nv_TableValues_set_column_name(result, i, cube->measure_dimensions.names[i],
						       cstr_end(cube->measure_dimensions.names[i]));
		}
	}

	/*
	 * create array of measure indices in the payload aggregates
	 * of the nv_Nanocube
	 */
	nv_TableValues_init_src_indices(result, 0, 0);
	for (u32 i=0;i<n;++i) {
		MemoryBlock *name = result->names.begin + i;
		s32 index = nv_Nanocube_measure_index_by_name(cube, name->begin, name->end);
		Assert(index >= 0);
		*(result->src_indices.begin + i) = pt_safe_s32_u32(index);
	}

	return result;
}

nm_TABLE_VALUES_COPY_FORMAT(nv_tv_copy_format)
{
	nv_TableValues *table = (nv_TableValues*) handle;

	nv_TableValues *result = nv_new_table_values((LinearAllocator*) allocation_context);
	Assert(result);

	/* init column names */
	nv_TableValues_init_column_names(result, table->names.begin, table->names.end);

	/* init src indices */
	nv_TableValues_init_src_indices(result, table->src_indices.begin, table->src_indices.end);

	return result;
}

nm_TABLE_VALUES_COPY(nv_tv_copy)
{
	LinearAllocator *nv_table_value_allocator = (LinearAllocator*) allocation_context;

	nv_TableValues *table = (nv_TableValues*) handle;

	/* @TODO(llins): more memory than needed here */
	nv_TableValues *result = nv_new_table_values((LinearAllocator*) allocation_context);
	Assert(result);

	/* init column names */
	nv_TableValues_init_column_names(result, table->names.begin, table->names.end);

	/* init src indices */
	nv_TableValues_init_src_indices(result, table->src_indices.begin, table->src_indices.end);

	u32 n = table->columns;
	u32 entries = table->rows * table->columns;
	result->values.begin = (f64*) LinearAllocator_alloc(nv_table_value_allocator, entries * sizeof(f64));
	result->values.end = result->values.begin + entries;

	f64 *src = table->values.begin;
	f64 *dst = result->values.begin;

	if (permutation) {
		/* permuted rows copy */
		u32 i = 0;
		f64 *it_dst  = dst;
		f64 *end_dst = dst + entries;
		while (it_dst != end_dst) {
			u32 j = *(permutation->begin + i);
			f64 *it_src = src + j * n;
			for (u32 u=0;u<n;++u) {
				*it_dst = *it_src;
				++it_dst;
				++it_src;
			}
			++i;
		}
	} else {
		/* direct copy */
		f64 *it_dst  = dst;
		f64 *end_dst = dst + entries;
		f64 *it_src  = src;
		while (it_dst != end_dst) {
			*it_dst = *it_src;
			++it_dst;
			++it_src;
		}
	}

	result->rows = table->rows;

	return result;
}


// #define nm_TABLE_VALUES_APPEND(name)
// 	s32 name(nm_TableValuesHandle handle, nx_PayloadAggregate *data,
// 		 void *nanocube)
nm_TABLE_VALUES_APPEND(nv_tv_append)
{
	nv_Nanocube    *cube = (nv_Nanocube*) nanocube;
	nv_TableValues *table = (nv_TableValues*) handle;
	/* copy values as f64 numbers */
	f64 *values = (f64*) LinearAllocator_alloc_if_available(table->memsrc, table->columns * sizeof(f64));
	if (!values) { return nm_ERROR_PAYLOAD_OUT_OF_MEMORY; }
	if (table->rows == 0) {
		table->values.begin = values;
		table->values.end = values;
	}
	for (u32 i=0;i<table->columns;++i) {
		*(values + i) = nv_Nanocube_get_value(cube, *(table->src_indices.begin + i), data);
	}
	table->values.end += table->columns;
	++table->rows;
	return nm_OK;
}

nm_TABLE_VALUES_APPEND_COPYING(nv_tv_append_copying)
{
	nv_TableValues *dst = (nv_TableValues*) dst_handle;
	nv_TableValues *src = (nv_TableValues*) src_handle;
	Assert(src_index < src->rows);
	Assert(dst->columns == src->columns);
	f64 *it_dst = (f64*) LinearAllocator_alloc_if_available(dst->memsrc, dst->columns * sizeof(f64));
	if (!it_dst) { return nm_ERROR_PAYLOAD_OUT_OF_MEMORY; }
	if (dst->rows == 0) {
		dst->values.begin = it_dst;
		dst->values.end = it_dst;
	}
	f64 *it_src = src->values.begin + src_index * src->columns;
	for (u32 i=0;i<dst->columns;++i) {
		*it_dst = *it_src;
		++it_src;
		++it_dst;
	}
	dst->values.end = it_dst;
	++dst->rows;
	return nm_OK;
}


// #define TABLE_VALUES_COMBINE_ENTRY(name) void name(TableValuesHandle entry_handle, u32 entry_index, TableValuesHandle with_handle, u32 with_index, MeasureExpression_Operation_Type op, b8 entry_on_right)
nm_TABLE_VALUES_COMBINE_ENTRY(nv_tv_combine_entry)
{
	nv_TableValues *table_dst = (nv_TableValues*) entry_handle;
	nv_TableValues *table_src = (nv_TableValues*) with_handle;

	Assert(table_dst->columns = table_src->columns);

	f64 *src     = table_src->values.begin + table_src->columns * with_index;
	f64 *src_end = src + table_src->columns;
	f64 *dst     = table_dst->values.begin + table_dst->columns * entry_index;

	if (pt_is_nan_f64(*src) || pt_is_nan_f64(*dst)) {
		*dst = pt_nan_f64();
	} else {
		switch(op)
		{
		case nm_MEASURE_EXPRESSION_OPERATION_BINARY_ADD: {
#if 0
			b8 print_debug = 1;
#endif
			while (src != src_end) {
#if 0
				static s64 COMB=-1;
				if (print_debug) {
					++COMB;
					Print *print = &debug_request->print;
					Print_clear(print);
					Print_u64(print,COMB);
					Print_cstr(print," count: ");
					Print_f64(print, *src);
					Print_cstr(print," + ");
					Print_f64(print, *dst);
					Print_cstr(print," -> ");
					Print_f64(print, *dst + *src);
					Print_cstr(print,"\n");
					Request_print(debug_request, print);
					print_debug = 0;
				}
#endif
				*dst += *src;
				++src;
				++dst;
			}
		} break;
		case nm_MEASURE_EXPRESSION_OPERATION_BINARY_SUB: {
			if (entry_on_right) {
				while (src != src_end) {
					*dst = *src - *dst;
					++src;
					++dst;
				}
			}
			else {
				while (src != src_end) {
					*dst -= *src;
					++src;
					++dst;
				}
			}
		} break;
		case nm_MEASURE_EXPRESSION_OPERATION_BINARY_DIV: {
			if (entry_on_right) {
				while (src != src_end) {
					*dst = *src / *dst;
					++src;
					++dst;
				}
			}
			else {
				while (src != src_end) {
					*dst /= *src;
					++src;
					++dst;
				}
			}
		} break;
		case nm_MEASURE_EXPRESSION_OPERATION_BINARY_MUL: {
			while (src != src_end) {
				*dst *= *src;
				++src;
				++dst;
			}
		} break;
		default:
			break;
		}
	}
}

// #define TABLE_VALUES_PACK(name) void name(TableValuesHandle handle, TablePermutation* permutation, MemoryBlock repeat_flags)
nm_TABLE_VALUES_PACK(nv_tv_pack)
{
	Assert(permutation);

	nv_TableValues *table = (nv_TableValues*) handle;

	u32 n = table->rows;

	Assert(n == permutation->end - permutation->begin);
	Assert(n == repeat_flags.end - repeat_flags.begin);

	// initialize an empty table
	pt_Memory memory = platform.allocate_memory(n * table->columns * sizeof(f64), 3, 0);

	LinearAllocator	memsrc;
	nv_TableValues  target;

	LinearAllocator_init(&memsrc, memory.memblock.begin, memory.memblock.begin, memory.memblock.end);
	nv_TableValues_init(&target, &memsrc);
	target.columns = table->columns;

	/*
	 * leaving target partially initialized since it is
	 * a temporary object. Only needs to be consistent
	 * enough to run nv_tv_append_copying and
	 * nv_tv_combine_entry.
	 */

	u32 nn = 0;
	for (u32 i=0;i<n;++i) {
		u32 j = *(permutation->begin + i);
		if (*(repeat_flags.begin + i) == 0) {
			nv_tv_append_copying(&target, handle, j);
			++nn;
		} else {
			Assert(nn>0);
			nv_tv_combine_entry(&target, nn-1, handle, j, nm_MEASURE_EXPRESSION_OPERATION_BINARY_ADD, 0);
		}
	}
	table->values.end = table->values.begin + (nn * table->columns);
	table->rows = nn;
	pt_copy_bytes((char*) target.values.begin,
		      (char*) target.values.end,
		      (char*) table->values.begin,
		      (char*) table->values.end);
	table->memsrc->end = (char*) table->values.end;
	platform.free_memory(&memory);
}

nm_TABLE_VALUES_COMBINE_NUMBER(nv_tv_combine_number)
{
	nv_TableValues *table = (nv_TableValues*) handle;

	f64 *it  = table->values.begin;
	f64 *end = table->values.end;

	while (it != end) {
		switch(op) {
		case nm_MEASURE_EXPRESSION_OPERATION_BINARY_ADD: {
			*it += number;
		} break;
		case nm_MEASURE_EXPRESSION_OPERATION_BINARY_SUB: {
			if (number_on_right) {
				*it -= number;
			} else  {
				*it = number - *it;
			}
		} break;
		case nm_MEASURE_EXPRESSION_OPERATION_BINARY_DIV: {
			if (number_on_right) {
				*it /= number;
			} else  {
				*it = number / *it;
			}
		} break;
		case nm_MEASURE_EXPRESSION_OPERATION_BINARY_MUL: {
			*it *= number;
		} break;
		default:
			break;
		}
		++it;
	}
}

// #define TABLE_VALUES_CANT_COMBINE_ENTRY(name)
// 	void name(TableValuesHandle handle, u32 index,
//		  MeasureExpression_Operation_Type op, b8 value_on_right)
nm_TABLE_VALUES_CANT_COMBINE_ENTRY(nv_tv_cant_combine_entry)
{
	nv_TableValues *table_dst = (nv_TableValues*) handle;
	f64 *dst     = table_dst->values.begin + index * table_dst->columns;
	f64 *dst_end = dst + table_dst->columns;
	// assuming values that don't exist are implictly zero
	switch(op)
	{
	case nm_MEASURE_EXPRESSION_OPERATION_BINARY_SUB: {
		if (value_on_right) {
			while (dst != dst_end) {
				*dst = - (*dst);
				++dst;
			}
		}
	} break;
	case nm_MEASURE_EXPRESSION_OPERATION_BINARY_DIV: {
		if (value_on_right) {
			while (dst != dst_end) {
				*dst = 0.0;
				++dst;
			}
		} else  {
			while (dst != dst_end) {
				*dst = pt_nan_f64(); // 0.0;
				++dst;
			}
		}
	} break;
	case nm_MEASURE_EXPRESSION_OPERATION_BINARY_MUL: {
		while (dst != dst_end) {
			*dst = 0.0;
			++dst;
		}
	} break;
	default:
		break;
	}
}


// #define nm_get_time_binning(name) b8 name(void *handle, u8 dimension, nm_timebinning *output)
nm_GET_TIME_BINNING(nv_get_time_binning)
{
	return nv_Nanocube_get_time_binning((nv_Nanocube*) nanocube, dimension, output);
}

nm_GET_ALIAS_PATH(nv_get_alias_path)
{
	return nv_Nanocube_get_alias_path((nv_Nanocube*) nanocube, dimension, alias, output_buffer_capacity, output_buffer, output_length);
}

internal void
nv_payload_services_init(nm_Services *services)
{
	services->create = nv_tv_create;
	services->append = nv_tv_append;
	services->append_copying = nv_tv_append_copying;
	services->copy = nv_tv_copy;
	services->copy_format = nv_tv_copy_format;
	services->combine_entry = nv_tv_combine_entry;
	services->cant_combine_entry = nv_tv_cant_combine_entry;
	services->combine_number = nv_tv_combine_number;
	services->get_time_binning = nv_get_time_binning;
	services->get_alias_path = nv_get_alias_path;
	services->pack = nv_tv_pack;
}

#endif

#define nv_FORMAT_JSON   1
#define nv_FORMAT_TEXT   2
#define nv_FORMAT_BINARY 3

typedef struct {
	nm_Measure *measure;
} nv_Query;

typedef struct {
	u8         format;
} nv_Format;


#define nv_Poly_TYPE_INTERIOR_AND_BOUNDARY 0
#define nv_Poly_TYPE_INTERIOR_ONLY 1
#define nv_Poly_TYPE_BOUNDARY_ONLY 2

#define nv_MAX_LONGITUDE   180.0
#define nv_MIN_LONGITUDE  -180.0
#define nv_MIN_LATITUDE   -85.05113
#define nv_MAX_LATITUDE    85.05113

#define nv_Poly_OP_UNION 0
#define nv_Poly_OP_INTERSECTION 1
#define nv_Poly_OP_SYMDIFF 2
#define nv_Poly_OP_DIFF 3
#define nv_Poly_OP_COMPLEMENT 4

typedef struct nv_Poly {
	b8 is_poly : 1;
	b8 is_op : 1;
	union {
		struct {
			s32 num_points;
			s32 poly_type;
			f32 *coords;
		} poly;
		struct {
			s32 num_polys;
			s32 op_type;
			struct nv_Poly* *polys;
		} op;
	};
} nv_Poly;

internal void
nv_Poly_init_poly(nv_Poly *self, s32 poly_type, s32 num_points, f32 *coords)
{
	self->is_poly = 1;
	self->is_op   = 0;
	self->poly.num_points = num_points;
	self->poly.poly_type  = poly_type;
	self->poly.coords     = coords;
}

internal void
nv_Poly_init_op(nv_Poly *self, s32 op_type, s32 num_polys, nv_Poly* *polys)
{
	self->is_poly      = 0;
	self->is_op        = 1;
	self->op.num_polys = num_polys;
	self->op.op_type   = op_type;
	self->op.polys     = polys;
}

/*
 * Nanocube index and vector abstract syntax reduce rules
 */

typedef struct {
	np_TypeID number;
	np_TypeID string;
	np_TypeID path;
	np_TypeID measure;
	np_TypeID target;
	np_TypeID binding;
	np_TypeID selection;
	np_TypeID query;
	np_TypeID schema;
	np_TypeID format;
	np_TypeID version;
	np_TypeID poly; // polygon
} nv_CompilerTypes;

internal nv_CompilerTypes nv_compiler_types;

np_FUNCTION_HANDLER(nv_function_query)
{
	Assert(params_begin + 1 == params_end);
	np_TypeValue *measure_tv = params_begin;
	Assert(measure_tv->type_id  == nv_compiler_types.measure);
	nv_Query *result = (nv_Query*) np_Compiler_alloc(compiler, sizeof(nv_Query));
	*result = (nv_Query) { .measure = (nm_Measure*) measure_tv->value };
	return np_TypeValue_value(nv_compiler_types.query, result);
}

np_FUNCTION_HANDLER(nv_function_format)
{
	Assert(params_begin + 1 == params_end);
	np_TypeValue *format_name_tv = params_begin;
	Assert(format_name_tv->type_id == nv_compiler_types.string);
	MemoryBlock format_name = *((MemoryBlock*) format_name_tv->value);
	nv_Format *result = (nv_Format*) np_Compiler_alloc(compiler, sizeof(nv_Format));
	// log error message if format name is not json or text
	if (pt_compare_memory_cstr(format_name.begin, format_name.end, "json") == 0) {
		result->format = nv_FORMAT_JSON;
	} else if (pt_compare_memory_cstr(format_name.begin, format_name.end, "text") == 0) {
		result->format = nv_FORMAT_TEXT;
	} else if (pt_compare_memory_cstr(format_name.begin, format_name.end, "bin") == 0) {
		result->format = nv_FORMAT_BINARY;
	} else {
		char *error = "Invalid format (it needs to be either 'text', 'json', 'bin')\n";
		np_Compiler_log_custom_error(compiler, error, cstr_end(error));
		np_Compiler_log_ast_node_context(compiler);
		return np_TypeValue_error();
	}
	return np_TypeValue_value(nv_compiler_types.format, result);
}

np_FUNCTION_HANDLER(nv_function_schema)
{
	Assert(params_begin == params_end);
// 	np_TypeValue *query_tv = params_begin;
// 	np_TypeValue *format_tv = params_begin + 1;
// 	Assert(query_tv->type_id == nv_compiler_types.query);
// 	Assert(format_tv->type_id == nv_compiler_types.format);
// 	nv_Query *result = (nv_Query*) query_tv->value;
// 	if (query_tv->readonly) {
// 	nv_Schema *result = (nv_Schema*) query_tv->value;
// 	result  = (nv_Query*) np_Compiler_alloc(compiler, sizeof(nv_Query));
// 		*result = *((nv_Query*) query_tv->value);
// 	}
// 	result->format = ((nv_Format*)format_tv->value)->format;
	return np_TypeValue_value(nv_compiler_types.schema, 0);
}

np_FUNCTION_HANDLER(nv_function_version)
{
	Assert(params_begin == params_end);
// 	np_TypeValue *query_tv = params_begin;
// 	np_TypeValue *format_tv = params_begin + 1;
// 	Assert(query_tv->type_id == nv_compiler_types.query);
// 	Assert(format_tv->type_id == nv_compiler_types.format);
// 	nv_Query *result = (nv_Query*) query_tv->value;
// 	if (query_tv->readonly) {
// 	nv_Schema *result = (nv_Schema*) query_tv->value;
// 	result  = (nv_Query*) np_Compiler_alloc(compiler, sizeof(nv_Query));
// 		*result = *((nv_Query*) query_tv->value);
// 	}
// 	result->format = ((nv_Format*)format_tv->value)->format;
	return np_TypeValue_value(nv_compiler_types.version, 0);
}



np_FUNCTION_HANDLER(nv_function_select)
{
	//
	np_TypeValue *it;
	it = params_begin;
	while (it != params_end) {
		Assert(it->type_id == nv_compiler_types.string);
		++it;
	}

	s64 n = params_end - params_begin;

	nv_Selection *selection = (nv_Selection*) np_Compiler_alloc(compiler, sizeof(nv_Selection));

	MemoryBlock *names = (MemoryBlock*) np_Compiler_alloc(compiler, n * sizeof(MemoryBlock));

	selection->begin = names;
	selection->end   = names + n;

	it = params_begin;
	for (s32 i=0;i<n;++i) {
		*(names + i) = *((MemoryBlock*) it->value);
		++it;
	}

	return np_TypeValue_value(nv_compiler_types.selection, selection);
}

np_FUNCTION_HANDLER(nv_function_interval_sequence)
{
	Assert(params_end - params_begin == 3);

	np_TypeValue *base_value_type  = params_begin;
	np_TypeValue *width_value_type = params_begin + 1;
	np_TypeValue *count_value_type = params_begin + 2;

	// Make sure single parameter is a number
	Assert(base_value_type->type_id  == nv_compiler_types.number);
	Assert(width_value_type->type_id == nv_compiler_types.number);
	Assert(count_value_type->type_id == nv_compiler_types.number);

	// value should be an integer number
	s64 base   = (s64) *((f64*) base_value_type->value);
	u64 width  = (u64) *((f64*) width_value_type->value);
	u64 count  = (u64) *((f64*) count_value_type->value);
	u64 stride = width;
	u8  depth  = 0; // use max depth

	nm_Target *target = (nm_Target*) np_Compiler_alloc(compiler, sizeof(nm_Target));
	target->type = nm_TARGET_INTERVAL_SEQUENCE;
	target->anchor=0;
	target->loop=1;
	target->interval_sequence.base   = base;
	target->interval_sequence.width  = width;
	target->interval_sequence.count  = count;
	target->interval_sequence.depth  = depth;
	target->interval_sequence.stride = stride;

	return np_TypeValue_value(nv_compiler_types.target, target);
}

np_FUNCTION_HANDLER(nv_function_interval_sequence_with_stride)
{
	Assert(params_end - params_begin == 4);

	np_TypeValue *base_value_type   = params_begin;
	np_TypeValue *width_value_type  = params_begin + 1;
	np_TypeValue *count_value_type  = params_begin + 2;
	np_TypeValue *stride_value_type = params_begin + 3;

	// Make sure single parameter is a number
	Assert(base_value_type->type_id  == nv_compiler_types.number);
	Assert(width_value_type->type_id == nv_compiler_types.number);
	Assert(count_value_type->type_id == nv_compiler_types.number);
	Assert(stride_value_type->type_id == nv_compiler_types.number);

	// value should be an integer number
	s64 base   = (s64) *((f64*) base_value_type->value);
	u64 width  = (u64) *((f64*) width_value_type->value);
	u64 count  = (u64) *((f64*) count_value_type->value);
	u64 stride = (u64) *((f64*) stride_value_type->value);
	u8  depth  = 0; // use max depth

	nm_Target *target = (nm_Target*) np_Compiler_alloc(compiler, sizeof(nm_Target));
	target->type = nm_TARGET_INTERVAL_SEQUENCE;
	target->anchor=0;
	target->loop=1;
	target->interval_sequence.base   = base;
	target->interval_sequence.width  = width;
	target->interval_sequence.count  = count;
	target->interval_sequence.depth  = depth;
	target->interval_sequence.stride = stride;

	return np_TypeValue_value(nv_compiler_types.target, target);
}

np_FUNCTION_HANDLER(nv_function_interval_aggregate)
{
	Assert(params_end - params_begin == 2);

	np_TypeValue *begin_value_type  = params_begin;
	np_TypeValue *end_value_type    = params_begin + 1;

	// Make sure single parameter is a number
	Assert(begin_value_type->type_id  == nv_compiler_types.number);
	Assert(end_value_type->type_id    == nv_compiler_types.number);

	// value should be an integer number
	s64 begin   = (s64) *((f64*) begin_value_type->value);
	s64 end     = (s64) *((f64*) end_value_type->value);

	s64 base   = begin;
	u64 width  = end - begin;
	u64 count  = 1;
	u64 stride = width;
	u8  depth  = 0; // use max depth

	nm_Target *target = (nm_Target*) np_Compiler_alloc(compiler, sizeof(nm_Target));
	target->type = nm_TARGET_INTERVAL_SEQUENCE_AGGREGATE;
	target->anchor=0;
	target->loop=0;
	target->interval_sequence.base   = base;
	target->interval_sequence.width  = width;
	target->interval_sequence.count  = count;
	target->interval_sequence.depth  = depth;
	target->interval_sequence.stride = stride;

	return np_TypeValue_value(nv_compiler_types.target, target);
}

np_FUNCTION_HANDLER(nv_function_interval_sequence_aggregate)
{
	Assert(params_end - params_begin == 3);

	np_TypeValue *base_value_type  = params_begin;
	np_TypeValue *width_value_type = params_begin + 1;
	np_TypeValue *count_value_type = params_begin + 2;

	// Make sure single parameter is a number
	Assert(base_value_type->type_id  == nv_compiler_types.number);
	Assert(width_value_type->type_id == nv_compiler_types.number);
	Assert(count_value_type->type_id == nv_compiler_types.number);

	// value should be an integer number
	s64 base   = (s64) *((f64*) base_value_type->value);
	u64 width  = (u64) *((f64*) width_value_type->value);
	u64 count  = (u64) *((f64*) count_value_type->value);
	u64 stride = width;
	u8  depth  = 0; // use max depth

	nm_Target *target = (nm_Target*) np_Compiler_alloc(compiler, sizeof(nm_Target));
	target->type = nm_TARGET_INTERVAL_SEQUENCE_AGGREGATE;
	target->anchor=0;
	target->loop=0;
	target->interval_sequence.base   = base;
	target->interval_sequence.width  = width;
	target->interval_sequence.count  = count;
	target->interval_sequence.depth  = depth;
	target->interval_sequence.stride = stride;

	return np_TypeValue_value(nv_compiler_types.target, target);
}

np_FUNCTION_HANDLER(nv_function_time_series)
{
	Assert(params_end - params_begin == 4);

	np_TypeValue *base_value_type  = params_begin;
	np_TypeValue *width_value_type = params_begin + 1;
	np_TypeValue *count_value_type = params_begin + 2;
	np_TypeValue *stride_value_type = params_begin + 3;

	// Make sure single parameter is a number
	Assert(base_value_type->type_id  == nv_compiler_types.string);
	Assert(width_value_type->type_id == nv_compiler_types.number);
	Assert(count_value_type->type_id == nv_compiler_types.number);
	Assert(stride_value_type->type_id == nv_compiler_types.number);

	// value should be an integer number
	MemoryBlock *base = (MemoryBlock*) base_value_type->value;
	u64 width         = (u64) *((f64*) width_value_type->value);
	u64 count         = (u64) *((f64*) count_value_type->value);
	s64 stride        = (s64) *((f64*) stride_value_type->value);

	/* parse base date and time */
	ntp_Parser parser;
	ntp_Parser_init(&parser);
	if (!ntp_Parser_run(&parser, base->begin, base->end)) {
		char *error = "Invalid base date on timeseq\n";
		np_Compiler_log_custom_error(compiler, error, cstr_end(error));
		np_Compiler_log_ast_node_context(compiler);
		return np_TypeValue_error();
	}

	nm_Target *target = (nm_Target*) np_Compiler_alloc(compiler, sizeof(nm_Target));
	target->type = nm_TARGET_TIME_SERIES;
	target->anchor=0;
	target->loop=1;
	target->time_sequence.base       = parser.time;
	target->time_sequence.width      = width;
	target->time_sequence.count      = count;
	target->time_sequence.stride     = stride;
	target->time_sequence.cumulative = 0;

	return np_TypeValue_value(nv_compiler_types.target, target);
}

np_FUNCTION_HANDLER(nv_function_cumulative_time_series)
{
	Assert(params_end - params_begin == 4);

	np_TypeValue *base_value_type  = params_begin;
	np_TypeValue *width_value_type = params_begin + 1;
	np_TypeValue *count_value_type = params_begin + 2;
	np_TypeValue *stride_value_type = params_begin + 3;

	// Make sure single parameter is a number
	Assert(base_value_type->type_id  == nv_compiler_types.string);
	Assert(width_value_type->type_id == nv_compiler_types.number);
	Assert(count_value_type->type_id == nv_compiler_types.number);
	Assert(stride_value_type->type_id == nv_compiler_types.number);

	// value should be an integer number
	MemoryBlock *base = (MemoryBlock*) base_value_type->value;
	u64 width         = (u64) *((f64*) width_value_type->value);
	u64 count         = (u64) *((f64*) count_value_type->value);
	s64 stride        = (s64) *((f64*) stride_value_type->value);

	/* parse base date and time */
	ntp_Parser parser;
	ntp_Parser_init(&parser);
	if (!ntp_Parser_run(&parser, base->begin, base->end)) {
		char *error = "Invalid base date on timeseq\n";
		np_Compiler_log_custom_error(compiler, error, cstr_end(error));
		np_Compiler_log_ast_node_context(compiler);
		return np_TypeValue_error();
	}

	nm_Target *target = (nm_Target*) np_Compiler_alloc(compiler, sizeof(nm_Target));
	target->type = nm_TARGET_TIME_SERIES;
	target->anchor=0;
	target->loop=1;
	target->time_sequence.base       = parser.time;
	target->time_sequence.width      = width;
	target->time_sequence.count      = count;
	target->time_sequence.stride     = stride;
	target->time_sequence.cumulative = 1;

	return np_TypeValue_value(nv_compiler_types.target, target);
}



np_FUNCTION_HANDLER(nv_function_time_series_aggregate)
{
	Assert(params_end - params_begin == 4);

	np_TypeValue *base_value_type  = params_begin;
	np_TypeValue *width_value_type = params_begin + 1;
	np_TypeValue *count_value_type = params_begin + 2;
	np_TypeValue *stride_value_type = params_begin + 3;

	// Make sure single parameter is a number
	Assert(base_value_type->type_id  == nv_compiler_types.string);
	Assert(width_value_type->type_id == nv_compiler_types.number);
	Assert(count_value_type->type_id == nv_compiler_types.number);
	Assert(stride_value_type->type_id == nv_compiler_types.number);

	// value should be an integer number
	MemoryBlock *base = (MemoryBlock*) base_value_type->value;
	u64 width         = (u64) *((f64*) width_value_type->value);
	u64 count         = (u64) *((f64*) count_value_type->value);
	s64 stride        = (s64) *((f64*) stride_value_type->value);

	/* parse base date and time */
	ntp_Parser parser;
	ntp_Parser_init(&parser);
	if (!ntp_Parser_run(&parser, base->begin, base->end)) {
		char *error = "Invalid base date on timeseq\n";
		np_Compiler_log_custom_error(compiler, error, cstr_end(error));
		np_Compiler_log_ast_node_context(compiler);
		return np_TypeValue_error();
	}

	nm_Target *target = (nm_Target*) np_Compiler_alloc(compiler, sizeof(nm_Target));
	target->type = nm_TARGET_TIME_SERIES_AGGREGATE;
	target->anchor=0;
	target->loop=0;
	target->time_sequence.base   = parser.time;
	target->time_sequence.width  = width;
	target->time_sequence.count  = count;
	target->time_sequence.stride = stride;

	return np_TypeValue_value(nv_compiler_types.target, target);
}

np_FUNCTION_HANDLER(nv_function_dive_1)
{
	Assert(params_end - params_begin == 1);
	np_TypeValue *depth_value_type = params_begin;

	// Make sure single parameter is a number
	Assert(depth_value_type->type_id == nv_compiler_types.number);

	// value should be an integer number
	s32 depth_value = (s32) *((f64*) depth_value_type->value);

	Assert(depth_value >= 0);

	nm_Target *target = (nm_Target*) np_Compiler_alloc(compiler, sizeof(nm_Target));
	target->type = nm_TARGET_FIND_DIVE;
	target->anchor=1;
	target->find_dive.path.by_alias = 0;
	target->find_dive.path.array.begin  = 0;
	target->find_dive.path.array.length = 0;
	target->find_dive.depth = (u8) depth_value;

	return np_TypeValue_value(nv_compiler_types.target, target);
}

np_FUNCTION_HANDLER(nv_function_dive_list)
{
	Assert(params_end - params_begin >= 1);

	// should be an array of labels!
	s64 len = params_end - params_begin;

	nm_Dive *begin = 0;
	nm_Dive *end = 0;
	if (len > 0) {
		// check if every object is a dive object
		np_TypeValue *it = params_begin;
		while (it != params_end) {
			Assert(it->type_id == nv_compiler_types.target);
			nm_Target *target = it->value;
			if (target->type != nm_TARGET_FIND_DIVE) {
				char *error = "dive_list must contain only dive objects\n";
				np_Compiler_log_custom_error(compiler, error, cstr_end(error));
				np_Compiler_log_ast_node_context(compiler);
				return np_TypeValue_error();
			}
			++it;
		}

		nm_Dive* dive_list = (nm_Dive*) np_Compiler_alloc(compiler,sizeof(nm_Dive)*len);
		begin = dive_list;
		end   = dive_list + len;
		s64 i = 0;
		it = params_begin;
		while (it != params_end) {
			nm_Target *target = it->value;
			dive_list[i++] = target->find_dive;
			++it;
		}

	}
	nm_Target* target = (nm_Target*) np_Compiler_alloc(compiler,sizeof(nm_Target));
	target->type = nm_TARGET_FIND_DIVE_LIST;
	target->find_dive_list.begin = begin;
	target->find_dive_list.end = end;
	target->anchor=1;
	target->loop=0;

	return np_TypeValue_value(nv_compiler_types.target, target);
}

np_FUNCTION_HANDLER(nv_function_dive_2)
{
	Assert(params_end - params_begin == 2);
	np_TypeValue *path_tv = params_begin;
	np_TypeValue *depth_value_type = params_begin + 1;

	// Make sure single parameter is a number
	Assert(path_tv->type_id == nv_compiler_types.path);
	Assert(depth_value_type->type_id == nv_compiler_types.number);

	// value should be an integer number
	nm_Path *path = (nm_Path*) path_tv->value;
	s32 depth_value = (s32) *((f64*) depth_value_type->value);

	Assert(depth_value >= 0);

	nm_Target *target = (nm_Target*) np_Compiler_alloc(compiler, sizeof(nm_Target));

	target->type = nm_TARGET_FIND_DIVE;
	target->anchor=1;
	target->loop = 0;
	target->find_dive.path = *path;
	target->find_dive.depth = (u8) depth_value;

	return np_TypeValue_value(nv_compiler_types.target, target);
}

np_FUNCTION_HANDLER(nv_function_dive_by_alias)
{
	Assert(params_end - params_begin == 2);
	np_TypeValue *alias_tv = params_begin;
	np_TypeValue *depth_value_type = params_begin + 1;

	// Make sure single parameter is a number
	Assert(alias_tv->type_id == nv_compiler_types.string);
	Assert(depth_value_type->type_id == nv_compiler_types.number);

	// value should be an integer number
	s32 depth_value = (s32) *((f64*) depth_value_type->value);

	Assert(depth_value >= 0);

	nm_Target *target = (nm_Target*) np_Compiler_alloc(compiler, sizeof(nm_Target));

	target->type = nm_TARGET_FIND_DIVE;
	target->anchor=1;
	target->loop = 0;
	target->find_dive.path.by_alias = 1;
	target->find_dive.path.alias = *((MemoryBlock*) alias_tv->value);
	target->find_dive.depth = (u8) depth_value;

	return np_TypeValue_value(nv_compiler_types.target, target);
}



np_FUNCTION_HANDLER(nv_function_mask)
{
	Assert(params_end - params_begin == 1);
	np_TypeValue *mask_tv = params_begin;

	// Make sure single parameter is a number
	Assert(mask_tv->type_id == nv_compiler_types.string);

	nm_Target *target = (nm_Target*) np_Compiler_alloc(compiler, sizeof(nm_Target));
	target->type   = nm_TARGET_MASK;
	target->anchor = 0;
	target->loop   = 0;
	target->mask   = *((MemoryBlock*) mask_tv->value);

	return np_TypeValue_value(nv_compiler_types.target, target);
}

internal nm_Binding*
nv_copy_binding(np_Compiler *compiler, nm_Binding *binding)
{
	nm_Binding *result = 0;
	nm_Binding *prev_copy = 0;
	nm_Binding *it = binding;
	while (it) {
		nm_Binding *it_copy = (nm_Binding*) np_Compiler_alloc(compiler, sizeof(nm_Binding));
		it_copy->dimension_name = it->dimension_name;
		it_copy->target		= it->target;
		it_copy->hint           = it->hint;
		it_copy->next		= 0;
		if (prev_copy) {
			prev_copy->next = it_copy;
		}
		if (!result) {
			result = it_copy;
		}
		prev_copy = it_copy;
		it = it->next;
	}
	return result;
}

internal nm_MeasureSourceBinding*
nv_copy_measure_source_binding(BilinearAllocator *memory, nm_MeasureSourceBinding *msb)
{
	nm_MeasureSourceBinding *result = 0;
	nm_MeasureSourceBinding *prev_copy = 0;
	nm_MeasureSourceBinding *it = msb;
	while (it) {
		nm_MeasureSourceBinding *it_copy = (nm_MeasureSourceBinding*) BilinearAllocator_alloc_left(memory, sizeof(nm_MeasureSourceBinding));
		it_copy->dimension = it->dimension;
		it_copy->target    = it->target;
		it_copy->next	   = 0;
		if (prev_copy) {
			prev_copy->next = it_copy;
		}
		if (!result) {
			result = it_copy;
		}
		prev_copy = it_copy;
		it = it->next;
	}
	return result;
}

internal nm_MeasureExpression*
nv_copy_measure_expression(BilinearAllocator *memory, nm_MeasureExpression *measure_expression)
{
	nm_MeasureExpression *copy = (nm_MeasureExpression*) BilinearAllocator_alloc_left(memory, sizeof(nm_MeasureExpression));
	*copy = *measure_expression;
	if (copy->is_unary_op) {
		copy->op.left = nv_copy_measure_expression(memory, copy->op.left);
	}
	if (copy->is_binary_op) {
		copy->op.left  = nv_copy_measure_expression(memory, copy->op.left);
		copy->op.right = nv_copy_measure_expression(memory, copy->op.right);
	}
	return copy;
}

internal nm_Measure*
nv_copy_measure(np_Compiler *compiler, nm_Measure *measure)
{
	nm_Measure *measure_copy = (nm_Measure*) np_Compiler_alloc(compiler, sizeof(nm_Measure));

	pt_fill((char*) measure_copy, (char*) measure_copy + sizeof(nm_Measure), 0);

	measure_copy->memory      = measure->memory; // same allocator
	measure_copy->num_sources = measure->num_sources;
	for (u32 i=0;i<measure->num_sources;++i)
	{
		measure_copy->sources[i] = measure->sources[i];
		measure_copy->num_bindings[i] = measure->num_bindings[i];
		measure_copy->bindings[i] = nv_copy_measure_source_binding(measure_copy->memory, measure->bindings[i]);
		measure_copy->payload_config[i] = measure->payload_config[i];
	}

	measure_copy->expression = nv_copy_measure_expression(measure_copy->memory, measure->expression);

	return measure_copy;
}

np_FUNCTION_HANDLER(nv_function_binding_target)
{
	Assert(params_end - params_begin >= 2);

	np_TypeValue *dimension_name_tv = params_begin;
	np_TypeValue *target_tv	    = params_begin + 1;

	Assert(dimension_name_tv->type_id == nv_compiler_types.string);
	Assert(target_tv->type_id == nv_compiler_types.target);

	nm_Binding *binding = (nm_Binding*) np_Compiler_alloc(compiler, sizeof(nm_Binding));
	binding->dimension_name = *((MemoryBlock*) dimension_name_tv->value);
	binding->target = (nm_Target*) target_tv->value;
	binding->next   = 0;

	// search for the "text" keyword on the tag list
	binding->hint = (nm_BindingHint) { .hint_id = 0 };
	np_TypeValue *it = params_begin + 2;
	while (it != params_end) {
		MemoryBlock *tag = (MemoryBlock*) it->value;
		// check if
		if (pt_compare_memory_n_cstr(tag->begin, tag->end, "img", 3) == 0) {
			u64 n = 0;
			if (!pt_parse_u64(tag->begin + 3, tag->end, &n)) {
				char *error = "Invalid img type. Expecting img<NUMBER> (eg. img8, img25), couldn't parse number";
				np_Compiler_log_custom_error(compiler, error, cstr_end(error));
				np_Compiler_log_ast_node_context(compiler);
				return np_TypeValue_error();
			}
			// assume suffix is a number store in void* slot
			binding->hint = (nm_BindingHint) { .hint_id = nm_BINDING_HINT_IMG, .param = (void*) n };
		} else if (pt_compare_memory_cstr(tag->begin, tag->end, "name") == 0) {
			// NOTE(llins) will only know the source of the mapping from ID to NAMEs when we bind a source
			binding->hint = (nm_BindingHint) { .hint_id = nm_BINDING_HINT_NAME, .param = 0 };
		} else if (pt_compare_memory_cstr(tag->begin, tag->end, "time") == 0) {
			binding->hint = (nm_BindingHint) { .hint_id = nm_BINDING_HINT_TIME, .param = 0 };
		}
		++it;
	}

	return np_TypeValue_value(nv_compiler_types.binding, binding);
}

np_FUNCTION_HANDLER(nv_function_binding_path)
{
	Assert(params_end - params_begin == 2);

	np_TypeValue *dimension_name_tv = params_begin;
	np_TypeValue *path_tv	        = params_begin + 1;

	Assert(dimension_name_tv->type_id == nv_compiler_types.string);
	Assert(path_tv->type_id	== nv_compiler_types.path);

	nm_Path *path = (nm_Path*) path_tv->value;

	nm_Target *target = (nm_Target*) np_Compiler_alloc(compiler, sizeof(nm_Target));
	target->type = nm_TARGET_FIND_DIVE;
	target->anchor = 0;
	target->loop   = 0;
	target->find_dive.path.by_alias  = 0;
	target->find_dive.path.array.begin  = path->array.begin;
	target->find_dive.path.array.length = path->array.length;
	target->find_dive.depth = 0;

	nm_Binding *binding = (nm_Binding*) np_Compiler_alloc(compiler, sizeof(nm_Binding));
	binding->dimension_name = *((MemoryBlock*) dimension_name_tv->value);
	binding->target = target;
	binding->next   = 0;

	return np_TypeValue_value(nv_compiler_types.binding, binding);
}

np_FUNCTION_HANDLER(nv_function_binding_alias)
{
	Assert(params_end - params_begin == 2);

	np_TypeValue *dimension_name_tv = params_begin;
	np_TypeValue *alias_tv	        = params_begin + 1;

	Assert(dimension_name_tv->type_id == nv_compiler_types.string);
	Assert(alias_tv->type_id	  == nv_compiler_types.string);

	nm_Target *target = (nm_Target*) np_Compiler_alloc(compiler, sizeof(nm_Target));
	target->type = nm_TARGET_FIND_DIVE;
	target->anchor = 0;
	target->loop   = 0;
	target->find_dive.path.by_alias  = 1;
	target->find_dive.path.alias = *((MemoryBlock*) alias_tv->value);
	target->find_dive.depth = 0;

	nm_Binding *binding = (nm_Binding*) np_Compiler_alloc(compiler, sizeof(nm_Binding));
	binding->dimension_name = *((MemoryBlock*) dimension_name_tv->value);
	binding->target = target;
	binding->next   = 0;

	return np_TypeValue_value(nv_compiler_types.binding, binding);
}

np_FUNCTION_HANDLER(nv_function_poly)
{
	Assert(params_end - params_begin == 1);

	np_TypeValue *coords_tv = params_begin;

	Assert(coords_tv->type_id == nv_compiler_types.string);

	// try to parse all the ',' into lat/lon pairs of f32
	MemoryBlock st = *((MemoryBlock*) coords_tv->value);

	// count occurrences of ','
	s32 coords_upper_bound = 1;
	char *it = st.begin;
	while (it != st.end) {
		if (*it == ',') {
			++coords_upper_bound;
		}
		++it;
	}

	f32 *coords_begin    = (f32*) np_Compiler_alloc(compiler, coords_upper_bound * sizeof(f32));
	f32 *coords_capacity = coords_begin + coords_upper_bound;

	f32  *coords_end = coords_begin;
	it = st.begin;
	u8   parity = 0;
	while (it < st.end) {
		char *sep = pt_find_char(it,st.end,',');
		if (coords_end == coords_capacity) {
			char *error = "Unexpected error: not enough memory to handle poly coords";
			np_Compiler_log_custom_error(compiler, error, cstr_end(error));
			np_Compiler_log_ast_node_context(compiler);
			return np_TypeValue_error();
		}
		f32 coord;
		if (pt_parse_f32(it, sep, &coord)){
			if (parity == 0) {
				if  (coord < nv_MIN_LATITUDE || coord > nv_MAX_LATITUDE) {
					char *error = "Invalid latitude: expected lat needs to be -85.05113 <= lat <= 85.05113.";
					np_Compiler_log_custom_error(compiler, error, cstr_end(error));
					np_Compiler_log_ast_node_context(compiler);
					return np_TypeValue_error();
				}
			} else {
				if  (coord < nv_MIN_LONGITUDE || coord > nv_MAX_LONGITUDE) {
					char *error = "Invalid latitude: expected lon needs to be -180 <= lon <= 180.";
					np_Compiler_log_custom_error(compiler, error, cstr_end(error));
					np_Compiler_log_ast_node_context(compiler);
					return np_TypeValue_error();
				}
			}
			*coords_end = coord;
			++coords_end;
			it = sep+1;
			parity = 1 - parity;
		} else {
			char *error = "Couldn't parse number in the 'poly' function string parameter: expecting a comma separated list of lat,lon";
			np_Compiler_log_custom_error(compiler, error, cstr_end(error));
			np_Compiler_log_ast_node_context(compiler);
			return np_TypeValue_error();
		}
	}

	if (parity == 1) {
		char *error = "Odd number of coords provided to poly function, expected lat,lon pairs";
		np_Compiler_log_custom_error(compiler, error, cstr_end(error));
		np_Compiler_log_ast_node_context(compiler);
		return np_TypeValue_error();
	}

	nv_Poly *poly = (nv_Poly*) np_Compiler_alloc(compiler, sizeof(nv_Poly));
	s32 num_points = (s32) (coords_end - coords_begin) / 2;
	nv_Poly_init_poly(poly, nv_Poly_TYPE_INTERIOR_AND_BOUNDARY, num_points, coords_begin);
	return np_TypeValue_value(nv_compiler_types.poly, poly);
}


internal np_TypeValue
nv_function_poly_combine(np_Compiler* compiler, np_TypeValue *params_begin, np_TypeValue *params_end, s32 op_type)
{
	s32 num_polys = (s32) (params_end - params_begin);
	for (s32 i=0;i<num_polys;++i) {
		Assert(params_begin[i].type_id == nv_compiler_types.poly);
	}
	nv_Poly* *list = (nv_Poly**) np_Compiler_alloc(compiler, num_polys * sizeof(nv_Poly*));
	for (s32 i=0;i<num_polys;++i) {
		list[i] = (nv_Poly*) params_begin[i].value;
	}
	nv_Poly* poly_combine = (nv_Poly*) np_Compiler_alloc(compiler, sizeof(nv_Poly));
	nv_Poly_init_op(poly_combine, op_type, num_polys, list);
	return np_TypeValue_value(nv_compiler_types.poly, poly_combine);
}

np_FUNCTION_HANDLER(nv_function_poly_complement)
{
	s32 num_polys = (s32) (params_end - params_begin);
	Assert(num_polys == 1);
	nv_Poly* *list = (nv_Poly**) np_Compiler_alloc(compiler, num_polys * sizeof(nv_Poly*));
	for (s32 i=0;i<num_polys;++i) {
		list[i] = (nv_Poly*) params_begin[i].value;
	}
	nv_Poly* poly_complement = (nv_Poly*) np_Compiler_alloc(compiler, sizeof(nv_Poly));
	nv_Poly_init_op(poly_complement, nv_Poly_OP_COMPLEMENT, num_polys, list);
	return np_TypeValue_value(nv_compiler_types.poly, poly_complement);
}

np_FUNCTION_HANDLER(nv_function_poly_union)
{
	return nv_function_poly_combine(compiler, params_begin, params_end, nv_Poly_OP_UNION);
}

np_FUNCTION_HANDLER(nv_function_poly_diff)
{
	return nv_function_poly_combine(compiler, params_begin, params_end, nv_Poly_OP_DIFF);
}

np_FUNCTION_HANDLER(nv_function_poly_symdiff)
{
	return nv_function_poly_combine(compiler, params_begin, params_end, nv_Poly_OP_SYMDIFF);
}

np_FUNCTION_HANDLER(nv_function_poly_intersection)
{
	return nv_function_poly_combine(compiler, params_begin, params_end, nv_Poly_OP_INTERSECTION);
}

#ifdef POLYCOVER

internal polycover_Shape
nv_shape_combine(polycover_Shape shape1, polycover_Shape shape2, s32 op_type)
{
	PolycoverAPI *polycover = &global_app_state->polycover;
	switch(op_type) {
		case nv_Poly_OP_DIFF: {
			return polycover->get_difference(shape1, shape2);
		}
		case nv_Poly_OP_SYMDIFF: {
			return polycover->get_symmetric_difference(shape1, shape2);
		}
		case nv_Poly_OP_INTERSECTION: {
			return polycover->get_intersection(shape1, shape2);
		}
		case nv_Poly_OP_UNION: {
			return polycover->get_union(shape1, shape2);
		}
		default: {
			Assert(0 && "not expected");
		}
	}
	return (polycover_Shape) { .handle = 0 };
}

internal polycover_Shape
nv_compute_poly_shape(nv_Poly *poly, s32 level)
{
	PolycoverAPI *polycover = &global_app_state->polycover;
	if (poly->is_poly) {
		return polycover->new_shape(poly->poly.coords, poly->poly.num_points, level);
	} else {
		if (poly->op.op_type == nv_Poly_OP_COMPLEMENT) {
			Assert(poly->op.num_polys == 1);
			polycover_Shape shape  = nv_compute_poly_shape(poly->op.polys[0], level);
			polycover_Shape result = polycover->get_complement(shape);
			polycover->free_shape(shape);
			return result;
		} else {
// 		case nv_Poly_OP_DIFF:
// 		case nv_Poly_OP_SYMDIFF:
// 		case nv_Poly_OP_INTERSECTION:
// 		case nv_Poly_OP_UNION: {
			Assert(poly->op.num_polys >= 1);
			if (poly->op.num_polys == 1) {
				return nv_compute_poly_shape(poly->op.polys[0], level);
			} else {
				// set result as the first poly shape
				polycover_Shape a = nv_compute_poly_shape(poly->op.polys[0], level);
				for (s32 i=1;i<poly->op.num_polys;++i) {
					polycover_Shape b = nv_compute_poly_shape(poly->op.polys[i], level);
					polycover_Shape aa = nv_shape_combine(a, b, poly->op.op_type);
					polycover->free_shape(a);
					polycover->free_shape(b);
					a = aa;
				}
				return a;
			}
		}
	}
}

internal char*
nv_compute_poly_mask(nv_Poly *poly, s32 level, char *buffer_begin, char *buffer_end)
{
	PolycoverAPI *polycover = &global_app_state->polycover;
	polycover_Shape shape = nv_compute_poly_shape(poly, level);
	Assert(shape.handle != 0);
	s32 code_size = 0;
	s32 ok = polycover->get_code(shape, buffer_begin, buffer_end, &code_size);
	polycover->free_shape(shape);
	if (ok) {
		return buffer_begin + code_size;
	} else {
		return 0;
	}
}

#endif

np_FUNCTION_HANDLER(nv_function_region)
{
	Assert(params_end - params_begin == 2);
	np_TypeValue *level_tv = params_begin;
	np_TypeValue *poly_tv  = params_begin + 1;
#ifdef POLYCOVER
	// compute a mask using polycover
	s32 level = (s32) *((f64*)level_tv->value);
	nv_Poly *poly = (nv_Poly*) poly_tv->value;
	MemoryBlock free_memblock = np_Compiler_free_memblock(compiler);
	char *end = nv_compute_poly_mask(poly, level, free_memblock.begin, free_memblock.end);
	if (end == 0) {
		char *error = "Error on nv_compute_poly_mask";
		np_Compiler_log_custom_error(compiler, error, cstr_end(error));
		np_Compiler_log_ast_node_context(compiler);
		return np_TypeValue_error();
	} else {
		// a bit hacky, but should be a correct way to get the code
		char *code = np_Compiler_alloc(compiler, end - free_memblock.begin);
		nm_Target *target = (nm_Target*) np_Compiler_alloc(compiler, sizeof(nm_Target));
		target->type   = nm_TARGET_MASK;
		target->anchor = 0;
		target->loop   = 0;
		target->mask   = (MemoryBlock) { .begin = code, .end = end };
		return np_TypeValue_value(nv_compiler_types.target, target);
	}
#else
	char *error = "POLYCOVER support is not enabled. You cannot use the region filter.";
	np_Compiler_log_custom_error(compiler, error, cstr_end(error));
	np_Compiler_log_ast_node_context(compiler);
	return np_TypeValue_error();
#endif
}

np_FUNCTION_HANDLER(nv_function_alias)
{
	// should be an array of labels!
	s64 len = params_end - params_begin;

	Assert(len == 1);

	// assert that all the params are paths
	np_TypeValue *alias_tv = params_begin;
	Assert(alias_tv->type_id == nv_compiler_types.string);

	nm_Path *path = (nm_Path*) np_Compiler_alloc(compiler, sizeof(nm_Path));
	path->by_alias = 1;
	path->alias = *((MemoryBlock*) alias_tv->value);

	return np_TypeValue_value(nv_compiler_types.path, path);
}


np_FUNCTION_HANDLER(nv_function_path)
{
	// should be an array of labels!
	s64 len = params_end - params_begin;

	Assert(len >= 0 && len < 256);

	u8 *labels = 0;
	if (len) {
		labels = (u8*) np_Compiler_alloc(compiler,(u64) len);
		for (s64 i=0;i<len;++i) {
			np_TypeValue* tv = params_begin + i;
			Assert(tv->type_id == nv_compiler_types.number);
			u8 lbl = (u8) (*((f64*) tv->value));
			labels[i] = lbl;
		}
	}

	nm_Path *path = (nm_Path*) np_Compiler_alloc(compiler, sizeof(nm_Path));
	path->by_alias = 0;
	path->array.begin  = labels;
	path->array.length = (u8) len;

	return np_TypeValue_value(nv_compiler_types.path, path);
}

np_FUNCTION_HANDLER(nv_function_path_list_aggregation)
{
	// should be an array of labels!
	s64 len = params_end - params_begin;

	// assert that all the params are paths
	np_TypeValue *it = params_begin;
	while (it != params_end) {
		Assert(it->type_id == nv_compiler_types.path);
		++it;
	}

	// allocate an array of path pointers
	nm_Path* *begin = (nm_Path**) np_Compiler_alloc(compiler,(u64) len * sizeof(nm_Path*));
	nm_Path* *end   = begin + len;
	for (s32 i=0;i<len;++i) {
		begin[i] = (nm_Path*) params_begin[i].value;
	}

	nm_Target* target = (nm_Target*) np_Compiler_alloc(compiler,sizeof(nm_Target));
	target->type = nm_TARGET_PATH_LIST;
	target->anchor=0;
	target->loop=0;
	target->path_list.begin = begin;
	target->path_list.end   = end;

	return np_TypeValue_value(nv_compiler_types.target, target);
}

np_FUNCTION_HANDLER(nv_function_path_list_aggregation_from_aliases)
{
	// should be an array of labels!
	s64 len = params_end - params_begin;

	// assert that all the params are paths
	np_TypeValue *it = params_begin;
	while (it != params_end) {
		Assert(it->type_id == nv_compiler_types.string);
		++it;
	}

	// allocate an array of path pointers
	nm_Path* *begin = (nm_Path**) np_Compiler_alloc(compiler,(u64) len * sizeof(nm_Path*));
	nm_Path* *end   = begin + len;
	for (s32 i=0;i<len;++i) {
		nm_Path* path = (nm_Path*) np_Compiler_alloc(compiler, sizeof(nm_Path));
		path->by_alias = 1;
		path->alias = *((MemoryBlock*) params_begin[i].value);
		begin[i] = path;
	}

	nm_Target* target = (nm_Target*) np_Compiler_alloc(compiler,sizeof(nm_Target));
	target->type = nm_TARGET_PATH_LIST;
	target->anchor=0;
	target->loop=0;
	target->path_list.begin = begin;
	target->path_list.end   = end;

	return np_TypeValue_value(nv_compiler_types.target, target);
}

np_FUNCTION_HANDLER(nv_function_tile2d)
{
	Assert(params_end - params_begin == 3);

	np_TypeValue *level = params_begin;
	np_TypeValue *x     = params_begin + 1;
	np_TypeValue *y     = params_begin + 2;

	Assert(level->type_id  == nv_compiler_types.number);
	Assert(x->type_id      == nv_compiler_types.number);
	Assert(y->type_id      == nv_compiler_types.number);

	// value should be an integer number
	s32 level_value = (s32) *((f64*) level->value);
	s32 x_value     = (s32) *((f64*) x->value);
	s32 y_value     = (s32) *((f64*) y->value);

	if (level_value < 0 || x_value < 0 || y_value < 0) {
		return np_TypeValue_error();
	}

	u8 *labels = (u8*) np_Compiler_alloc(compiler,(u64) level_value * sizeof(u8));
	for (s32 i=0;i<level_value;++i) {
		labels[i] = ((x_value & (1 << (level_value - 1 - i))) ? 1 : 0) +
			    ((y_value & (1 << (level_value - 1 - i))) ? 2 : 0);

	}
	nm_Path *path = (nm_Path*) np_Compiler_alloc(compiler, sizeof(nm_Path));
	path->by_alias = 0;
	path->array.begin  = labels;
	path->array.length = level_value;
	return np_TypeValue_value(nv_compiler_types.path, path);
}

np_FUNCTION_HANDLER(nv_function_img2d)
{
	Assert(params_end - params_begin == 3);

	np_TypeValue *level = params_begin;
	np_TypeValue *x     = params_begin + 1;
	np_TypeValue *y     = params_begin + 2;

	Assert(level->type_id  == nv_compiler_types.number);
	Assert(x->type_id      == nv_compiler_types.number);
	Assert(y->type_id      == nv_compiler_types.number);

	// value should be an integer number
	s32 level_value = (s32) *((f64*) level->value);
	s32 x_value     = (s32) *((f64*) x->value);
	s32 y_value     = (s32) *((f64*) y->value);

	if (level_value < 0 || x_value < 0 || y_value < 0) {
		return np_TypeValue_error();
	}

	y_value = (1 << level_value) - 1 - y_value;

	u8 *labels = (u8*) np_Compiler_alloc(compiler,(u64) level_value * sizeof(u8));
	for (s32 i=0;i<level_value;++i) {
		labels[i] = ((x_value & (1 << (level_value - 1 - i))) ? 1 : 0) +
			    ((y_value & (1 << (level_value - 1 - i))) ? 2 : 0);

	}
	nm_Path *path = (nm_Path*) np_Compiler_alloc(compiler, sizeof(nm_Path));
	path->by_alias = 0;
	path->array.begin  = labels;
	path->array.length = level_value;
	return np_TypeValue_value(nv_compiler_types.path, path);
}



np_FUNCTION_HANDLER(nv_function_binding_chain)
{
	Assert(params_end - params_begin == 2);

	np_TypeValue *left = params_begin;
	np_TypeValue *right = params_begin + 1;

	Assert(left->type_id  == nv_compiler_types.binding);
	Assert(right->type_id == nv_compiler_types.binding);

	nm_Binding *left_binding  = (nm_Binding*) left->value;
	nm_Binding *right_binding = (nm_Binding*) right->value;

	np_TypeValue result = *left;

	if (result.readonly)
	{
		left_binding = nv_copy_binding(compiler, left_binding);
		result.readonly = 0;
		result.value = left_binding;
	}

	nm_Binding *it = left_binding;
	while (it->next != 0) {
		it = it->next;
	}
	it->next = right_binding;

	return result;
}

np_FUNCTION_HANDLER(nv_function_measure_binding)
{
	Assert(params_end - params_begin == 2);

	np_TypeValue *left = params_begin;
	np_TypeValue *right = params_begin + 1;

	Assert(left->type_id  == nv_compiler_types.measure);
	Assert(right->type_id == nv_compiler_types.binding);


	nm_Measure *measure = (nm_Measure*) left->value;
	nm_Binding *binding = (nm_Binding*) right->value;

	np_TypeValue result = *left;

	if (result.readonly) {
		measure		= nv_copy_measure(compiler, measure);
		result.readonly = 0;
		result.value	= measure;
	}

	if (right->readonly) {
		binding = nv_copy_binding(compiler, binding);
	}

	nm_Measure_bind(measure,
			binding->dimension_name.begin,
			binding->dimension_name.end,
			*binding->target,
			binding->hint);

	return result;
}

np_FUNCTION_HANDLER(nv_function_measure_selection)
{
	Assert(params_end - params_begin == 2);

	np_TypeValue *left  = params_begin;
	np_TypeValue *right = params_begin + 1;

	Assert(left->type_id  == nv_compiler_types.measure);
	Assert(right->type_id == nv_compiler_types.selection);

	nm_Measure   *measure   = (nm_Measure*) left->value;
	/*
	 * selection is a more specific concept. it is opaque to
	 * nm_ (nanocube measure) layer and related to the
	 * payload of nanocube vectors.
	 */
	nv_Selection *selection = (nv_Selection*) right->value;

	np_TypeValue result = *left;

	if (result.readonly) {
		measure		= nv_copy_measure(compiler, measure);
		result.readonly = 0;
		result.value	= measure;
	}

	u8 error_code = nm_Measure_set_payload_config(measure,selection);

	if (error_code) {
		char *error = "Multiple selects on the same measure source.\n";
		np_Compiler_log_custom_error(compiler, error, cstr_end(error));
		np_Compiler_log_ast_node_context(compiler);
		return np_TypeValue_error();
	} else {
		return result;
	}
}

internal np_TypeValue
nv_function_measure_op_measure(np_Compiler *compiler,
		np_TypeValue* params_begin,
		np_TypeValue *params_end,
		nm_MeasureExpression_Operation_Type op)
{
	Assert(params_end - params_begin == 2);

	np_TypeValue *left	= params_begin;
	np_TypeValue *right = params_begin + 1;

	Assert(left->type_id  == nv_compiler_types.measure);
	Assert(right->type_id == nv_compiler_types.measure);

	nm_Measure *left_measure  = (nm_Measure*) left->value;
	nm_Measure *right_measure = (nm_Measure*) right->value;

	np_TypeValue result = *left;

	if (result.readonly) {
		left_measure    = nv_copy_measure(compiler, left_measure);
		result.readonly = 0;
		result.value	= left_measure;
	}

	nm_Measure_combine_measure(left_measure, right_measure, op);

	return result;
}

internal np_TypeValue
nv_function_measure_op_number(np_Compiler *compiler,
		np_TypeValue* params_begin,
		np_TypeValue *params_end,
		nm_MeasureExpression_Operation_Type op)
{
	Assert(params_end - params_begin == 2);

	np_TypeValue *left	= params_begin;
	np_TypeValue *right = params_begin + 1;

	Assert(left->type_id  == nv_compiler_types.measure);
	Assert(right->type_id == nv_compiler_types.number);

	nm_Measure *measure = (nm_Measure*) left->value;
	f64     *number  = (f64*) right->value;

	np_TypeValue result = *left;

	if (result.readonly) {
		measure         = nv_copy_measure(compiler, measure);
		result.value	= measure;
		result.readonly = 0;
	}

	nm_Measure_combine_number(measure, *number, op, 1);

	return result;
}

internal np_TypeValue
nv_function_number_op_measure(np_Compiler *compiler,
		np_TypeValue* params_begin,
		np_TypeValue *params_end,
		nm_MeasureExpression_Operation_Type op)
{
	Assert(params_end - params_begin == 2);

	np_TypeValue *left = params_begin;
	np_TypeValue *right = params_begin + 1;

	Assert(left->type_id  == nv_compiler_types.number);
	Assert(right->type_id == nv_compiler_types.measure);

	nm_Measure *measure = (nm_Measure*) right->value;
	f64     *number  = (f64*)	  left->value;

	np_TypeValue result = *right;

	if (result.readonly) {
		measure		= nv_copy_measure(compiler, measure);
		result.value	= measure;
		result.readonly = 0;
	}

	nm_Measure_combine_number(measure, *number, op, 0);

	return result;
}

// measure op measure

np_FUNCTION_HANDLER(nv_function_measure_sub_measure)
{
	return nv_function_measure_op_measure(compiler,
			params_begin, params_end,
			nm_MEASURE_EXPRESSION_OPERATION_BINARY_SUB);
}

np_FUNCTION_HANDLER(nv_function_measure_add_measure)
{
	return nv_function_measure_op_measure(compiler,
			params_begin, params_end,
			nm_MEASURE_EXPRESSION_OPERATION_BINARY_ADD);
}

np_FUNCTION_HANDLER(nv_function_measure_mul_measure)
{
	return nv_function_measure_op_measure(compiler,
			params_begin, params_end,
			nm_MEASURE_EXPRESSION_OPERATION_BINARY_MUL);
}

np_FUNCTION_HANDLER(nv_function_measure_div_measure)
{
	return nv_function_measure_op_measure(compiler,
			params_begin, params_end,
			nm_MEASURE_EXPRESSION_OPERATION_BINARY_DIV);
}

// measure op number

np_FUNCTION_HANDLER(nv_function_measure_sub_number)
{
	return nv_function_measure_op_number(compiler,
			params_begin, params_end,
			nm_MEASURE_EXPRESSION_OPERATION_BINARY_SUB);
}

np_FUNCTION_HANDLER(nv_function_measure_add_number)
{
	return nv_function_measure_op_number(compiler,
			params_begin, params_end,
			nm_MEASURE_EXPRESSION_OPERATION_BINARY_ADD);
}

np_FUNCTION_HANDLER(nv_function_measure_mul_number)
{
	return nv_function_measure_op_number(compiler,
			params_begin, params_end,
			nm_MEASURE_EXPRESSION_OPERATION_BINARY_MUL);
}

np_FUNCTION_HANDLER(nv_function_measure_div_number)
{
	return nv_function_measure_op_number(compiler,
			params_begin, params_end,
			nm_MEASURE_EXPRESSION_OPERATION_BINARY_DIV);
}

// number op measure

np_FUNCTION_HANDLER(nv_function_number_sub_measure)
{
	return nv_function_number_op_measure(compiler,
			params_begin, params_end,
			nm_MEASURE_EXPRESSION_OPERATION_BINARY_SUB);
}

np_FUNCTION_HANDLER(nv_function_number_add_measure)
{
	return nv_function_number_op_measure(compiler,
			params_begin, params_end,
			nm_MEASURE_EXPRESSION_OPERATION_BINARY_ADD);
}

np_FUNCTION_HANDLER(nv_function_number_mul_measure)
{
	return nv_function_number_op_measure(compiler,
			params_begin, params_end,
			nm_MEASURE_EXPRESSION_OPERATION_BINARY_MUL);
}

np_FUNCTION_HANDLER(nv_function_number_div_measure)
{
	return nv_function_number_op_measure(compiler,
			params_begin, params_end,
			nm_MEASURE_EXPRESSION_OPERATION_BINARY_DIV);
}

internal void
nv_Compiler_init(np_Compiler *compiler)
{
	nv_compiler_types.number    = compiler->number_type_id;
	nv_compiler_types.string    = compiler->string_type_id;
	nv_compiler_types.path      = np_Compiler_insert_type_cstr(compiler, "Path")->id;
	nv_compiler_types.measure   = np_Compiler_insert_type_cstr(compiler, "Measure")->id;
	nv_compiler_types.target    = np_Compiler_insert_type_cstr(compiler, "Target")->id;
	nv_compiler_types.binding   = np_Compiler_insert_type_cstr(compiler, "Binding")->id;
	nv_compiler_types.selection = np_Compiler_insert_type_cstr(compiler, "Selection")->id;
	nv_compiler_types.query     = np_Compiler_insert_type_cstr(compiler, "Query")->id;
	nv_compiler_types.format    = np_Compiler_insert_type_cstr(compiler, "Format")->id;
	nv_compiler_types.schema    = np_Compiler_insert_type_cstr(compiler, "Schema")->id;
	nv_compiler_types.version   = np_Compiler_insert_type_cstr(compiler, "Version")->id;
	nv_compiler_types.poly      = np_Compiler_insert_type_cstr(compiler, "Poly")->id;

	// and operators
	np_TypeID parameter_types[4];

	// p: number* -> path
	parameter_types[0] = nv_compiler_types.number;
	np_Compiler_insert_function_cstr
		(compiler, "p", nv_compiler_types.path, 0,
		 0, 1, nv_compiler_types.number, nv_function_path);

	// a: strign -> path
	parameter_types[0] = nv_compiler_types.string;
	np_Compiler_insert_function_cstr
		(compiler, "a", nv_compiler_types.path,
		 parameter_types, parameter_types + 1, 0, 0,
		 nv_function_alias);

	{
		char *equivalent_names[] = { "pathagg", "agg" };
		for (s32 i=0;i<2;++i) {
			// pathagg: path* -> target
			parameter_types[0] = nv_compiler_types.path;
			np_Compiler_insert_function_cstr
				(compiler, equivalent_names[i], nv_compiler_types.target,
				 0, 0, 1, nv_compiler_types.path,
				 nv_function_path_list_aggregation);

			parameter_types[0] = nv_compiler_types.string;
			np_Compiler_insert_function_cstr
				(compiler, equivalent_names[i], nv_compiler_types.target,
				 parameter_types, parameter_types+1,
				 1, nv_compiler_types.string,
				 nv_function_path_list_aggregation_from_aliases);
		}
	}

	// tile2d: number, number, number -> path
	parameter_types[0] = nv_compiler_types.number;
	parameter_types[1] = nv_compiler_types.number;
	parameter_types[2] = nv_compiler_types.number;
	np_Compiler_insert_function_cstr
		(compiler, "tile2d", nv_compiler_types.path,
		 parameter_types, parameter_types + 3, 0, 0,
		 nv_function_tile2d);

	// tile2d: number, number, number -> path
	parameter_types[0] = nv_compiler_types.number;
	parameter_types[1] = nv_compiler_types.number;
	parameter_types[2] = nv_compiler_types.number;
	np_Compiler_insert_function_cstr
		(compiler, "img2d", nv_compiler_types.path,
		 parameter_types, parameter_types + 3, 0, 0,
		 nv_function_img2d);

	// dive: number, number, number -> target
	parameter_types[0] = nv_compiler_types.number;
	parameter_types[1] = nv_compiler_types.number;
	parameter_types[2] = nv_compiler_types.number;
	np_Compiler_insert_function_cstr
		(compiler, "intseq", nv_compiler_types.target,
		 parameter_types, parameter_types + 3, 0, 0,
		 nv_function_interval_sequence);

	// dive: number, number, number -> target
	parameter_types[0] = nv_compiler_types.number;
	parameter_types[1] = nv_compiler_types.number;
	parameter_types[2] = nv_compiler_types.number;
	parameter_types[3] = nv_compiler_types.number;
	np_Compiler_insert_function_cstr
		(compiler, "intseq", nv_compiler_types.target,
		 parameter_types, parameter_types + 4, 0, 0,
		 nv_function_interval_sequence_with_stride);

	// dive: number, number, number -> target
	parameter_types[0] = nv_compiler_types.number;
	parameter_types[1] = nv_compiler_types.number;
	parameter_types[2] = nv_compiler_types.number;
	np_Compiler_insert_function_cstr
		(compiler, "intseqagg", nv_compiler_types.target,
		 parameter_types, parameter_types + 3, 0, 0,
		 nv_function_interval_sequence_aggregate);

	// dive: number, number, number -> target
	parameter_types[0] = nv_compiler_types.number;
	parameter_types[1] = nv_compiler_types.number;
	np_Compiler_insert_function_cstr
		(compiler, "interval", nv_compiler_types.target,
		 parameter_types, parameter_types + 2, 0, 0,
		 nv_function_interval_aggregate);

	// timeseq: string, number, number -> target
	parameter_types[0] = nv_compiler_types.string;
	parameter_types[1] = nv_compiler_types.number;
	parameter_types[2] = nv_compiler_types.number;
	parameter_types[3] = nv_compiler_types.number;
	np_Compiler_insert_function_cstr
		(compiler, "timeseries", nv_compiler_types.target,
		 parameter_types, parameter_types + 4, 0, 0,
		 nv_function_time_series);

	// timeseq: string, number, number -> target
	parameter_types[0] = nv_compiler_types.string;
	parameter_types[1] = nv_compiler_types.number;
	parameter_types[2] = nv_compiler_types.number;
	parameter_types[3] = nv_compiler_types.number;
	np_Compiler_insert_function_cstr
		(compiler, "ctimeseries", nv_compiler_types.target,
		 parameter_types, parameter_types + 4, 0, 0,
		 nv_function_cumulative_time_series);

	// timeseq: string, number, number -> target
	parameter_types[0] = nv_compiler_types.string;
	parameter_types[1] = nv_compiler_types.number;
	parameter_types[2] = nv_compiler_types.number;
	parameter_types[3] = nv_compiler_types.number;
	np_Compiler_insert_function_cstr
		(compiler, "timeseriesagg", nv_compiler_types.target,
		 parameter_types, parameter_types + 4, 0, 0,
		 nv_function_time_series_aggregate);

	{
		char *equivalent_names[] = { "dive", "branch" };
		for (s32 i=0;i<2;++i) {
			// dive: number -> target
			parameter_types[0] = nv_compiler_types.number;
			np_Compiler_insert_function_cstr
				(compiler, equivalent_names[i], nv_compiler_types.target,
				 parameter_types, parameter_types + 1, 0, 0,
				 nv_function_dive_1);

			// dive: path x number -> target
			parameter_types[0] = nv_compiler_types.path;
			parameter_types[1] = nv_compiler_types.number;
			np_Compiler_insert_function_cstr
				(compiler, equivalent_names[i], nv_compiler_types.target,
				 parameter_types, parameter_types + 2, 0, 0,
				 nv_function_dive_2);

			// dive: path x number -> target
			parameter_types[0] = nv_compiler_types.string;
			parameter_types[1] = nv_compiler_types.number;
			np_Compiler_insert_function_cstr
				(compiler, equivalent_names[i], nv_compiler_types.target,
				 parameter_types, parameter_types + 2, 0, 0,
				 nv_function_dive_by_alias);
		}
	}

	// dive_list -> target that is a dive list
	//
	// because of the assumption that result table fields have to have a fixed
	// depth, there needs to be at least one entry to dive list from which
	// we can derive the depth
	//
	parameter_types[0] = nv_compiler_types.target;
	np_Compiler_insert_function_cstr
		(compiler, "dive_list", nv_compiler_types.target,
		 parameter_types, parameter_types+1, 1, nv_compiler_types.target,
		 // 0, 0, 1, nv_compiler_types.target,
		 nv_function_dive_list);

	// poly: string -> poly
	parameter_types[0] = nv_compiler_types.string;
	np_Compiler_insert_function_cstr
		(compiler, "poly", nv_compiler_types.poly,
		 parameter_types, parameter_types + 1, 0, 0,
		 nv_function_poly);

	// poly_union: poly...poly -> poly
	parameter_types[0] = nv_compiler_types.poly;
	np_Compiler_insert_function_cstr
		(compiler, "poly_union", nv_compiler_types.poly,
		 parameter_types, parameter_types + 1, 1, nv_compiler_types.poly,
		 nv_function_poly_union);

	// poly_intersection: poly...poly -> poly
	parameter_types[0] = nv_compiler_types.poly;
	np_Compiler_insert_function_cstr
		(compiler, "poly_intersection", nv_compiler_types.poly,
		 parameter_types, parameter_types + 1, 1, nv_compiler_types.poly,
		 nv_function_poly_intersection);

	// poly_symdiff: poly...poly -> poly
	parameter_types[0] = nv_compiler_types.poly;
	np_Compiler_insert_function_cstr
		(compiler, "poly_symdiff", nv_compiler_types.poly,
		 parameter_types, parameter_types + 1, 1, nv_compiler_types.poly,
		 nv_function_poly_symdiff);

	// poly_diff: poly...poly -> poly
	parameter_types[0] = nv_compiler_types.poly;
	parameter_types[1] = nv_compiler_types.poly;
	np_Compiler_insert_function_cstr
		(compiler, "poly_diff", nv_compiler_types.poly,
		 parameter_types, parameter_types + 2, 0, 0,
		 nv_function_poly_diff);

	// poly_union: poly -> poly
	parameter_types[0] = nv_compiler_types.poly;
	np_Compiler_insert_function_cstr
		(compiler, "poly_complement", nv_compiler_types.poly,
		 parameter_types, parameter_types + 1, 0, 0,
		 nv_function_poly_complement);

	// region: string x target -> binding
	parameter_types[0] = nv_compiler_types.number;
	parameter_types[1] = nv_compiler_types.poly;
	np_Compiler_insert_function_cstr
		(compiler, "region", nv_compiler_types.target,
		 parameter_types, parameter_types + 2,
		 0, 0,
		 nv_function_region);

	// mask: string -> target
	parameter_types[0] = nv_compiler_types.string;
	np_Compiler_insert_function_cstr
		(compiler, "mask", nv_compiler_types.target,
		 parameter_types, parameter_types + 1, 0, 0,
		 nv_function_mask);

	// b: string x target -> binding
	parameter_types[0] = nv_compiler_types.string;
	parameter_types[1] = nv_compiler_types.target;
	np_Compiler_insert_function_cstr
		(compiler, "b", nv_compiler_types.binding,
		 parameter_types, parameter_types + 2, 1,
		 nv_compiler_types.string, nv_function_binding_target);

	// b: string x path -> binding
	parameter_types[0] = nv_compiler_types.string;
	parameter_types[1] = nv_compiler_types.path;
	np_Compiler_insert_function_cstr
		(compiler, "b", nv_compiler_types.binding,
		 parameter_types, parameter_types + 2, 0, 0,
		 nv_function_binding_path);

	// b: string x string -> binding
	parameter_types[0] = nv_compiler_types.string;
	parameter_types[1] = nv_compiler_types.string;
	np_Compiler_insert_function_cstr
		(compiler, "b", nv_compiler_types.binding,
		 parameter_types, parameter_types + 2, 0, 0,
		 nv_function_binding_alias);

	// .: binding x binding -> binding
	parameter_types[0] = nv_compiler_types.binding;
	parameter_types[1] = nv_compiler_types.binding;
	np_Compiler_insert_function_cstr
		(compiler, ".", nv_compiler_types.binding,
		 parameter_types, parameter_types + 2, 0, 0,
		 nv_function_binding_chain);

	// .: measure x binding -> measure
	parameter_types[0] = nv_compiler_types.measure;
	parameter_types[1] = nv_compiler_types.binding;
	np_Compiler_insert_function_cstr
		(compiler, ".", nv_compiler_types.measure,
		 parameter_types, parameter_types + 2, 0, 0,
		 nv_function_measure_binding);

	/* select: string* -> selection */
	np_Compiler_insert_function_cstr
		(compiler, "select", nv_compiler_types.selection,
		 0, 0, 1, nv_compiler_types.string,
		 nv_function_select);

	/* .: measure x selection -> measure */
	parameter_types[0] = nv_compiler_types.measure;
	parameter_types[1] = nv_compiler_types.selection;
	np_Compiler_insert_function_cstr
		(compiler, ".", nv_compiler_types.measure,
		 parameter_types, parameter_types + 2, 0, 0,
		 nv_function_measure_selection);

	// +: mesaure x measure -> measure
	parameter_types[0] = nv_compiler_types.measure;
	parameter_types[1] = nv_compiler_types.measure;
	np_Compiler_insert_function_cstr
		(compiler, "+", nv_compiler_types.measure,
		 parameter_types, parameter_types + 2, 0, 0,
		 nv_function_measure_add_measure);

	// *: mesaure x measure -> measure
	np_Compiler_insert_function_cstr
		(compiler, "*", nv_compiler_types.measure,
		 parameter_types, parameter_types + 2, 0, 0,
		 nv_function_measure_mul_measure);

	// /: mesaure x measure -> measure
	np_Compiler_insert_function_cstr
		(compiler, "/", nv_compiler_types.measure,
		 parameter_types, parameter_types + 2, 0, 0,
		 nv_function_measure_div_measure);

	// -: mesaure x measure -> measure
	np_Compiler_insert_function_cstr
		(compiler, "-", nv_compiler_types.measure,
		 parameter_types, parameter_types + 2, 0, 0,
		 nv_function_measure_sub_measure);


	// +: mesaure x number -> measure
	parameter_types[0] = nv_compiler_types.measure;
	parameter_types[1] = nv_compiler_types.number;
	np_Compiler_insert_function_cstr
		(compiler, "+", nv_compiler_types.measure,
		 parameter_types, parameter_types + 2, 0, 0,
		 nv_function_measure_add_number);

	// *: mesaure x measure -> measure
	np_Compiler_insert_function_cstr
		(compiler, "*", nv_compiler_types.measure,
		 parameter_types, parameter_types + 2, 0, 0,
		 nv_function_measure_mul_number);

	// /: mesaure x measure -> measure
	np_Compiler_insert_function_cstr
		(compiler, "/", nv_compiler_types.measure,
		 parameter_types, parameter_types + 2, 0, 0,
		 nv_function_measure_div_number);

	// -: mesaure x measure -> measure
	np_Compiler_insert_function_cstr
		(compiler, "-", nv_compiler_types.measure,
		 parameter_types, parameter_types + 2, 0, 0,
		 nv_function_measure_sub_number);

	// +: mesaure x number -> measure
	parameter_types[0] = nv_compiler_types.number;
	parameter_types[1] = nv_compiler_types.measure;
	np_Compiler_insert_function_cstr
		(compiler, "+", nv_compiler_types.measure,
		 parameter_types, parameter_types + 2, 0, 0,
		 nv_function_number_add_measure);

	// *: mesaure x measure -> measure
	np_Compiler_insert_function_cstr
		(compiler, "*", nv_compiler_types.measure,
		 parameter_types, parameter_types + 2, 0, 0,
		 nv_function_number_mul_measure);

	// /: mesaure x measure -> measure
	np_Compiler_insert_function_cstr
		(compiler, "/", nv_compiler_types.measure,
		 parameter_types, parameter_types + 2, 0, 0,
		 nv_function_number_div_measure);

	// -: mesaure x measure -> measure
	np_Compiler_insert_function_cstr
		(compiler, "-", nv_compiler_types.measure,
		 parameter_types, parameter_types + 2, 0, 0,
		 nv_function_number_sub_measure);

	// q: query -> measure
	parameter_types[0] = nv_compiler_types.measure;
	np_Compiler_insert_function_cstr
		(compiler, "q", nv_compiler_types.query,
		 parameter_types, parameter_types + 1, 0, 0,
		 nv_function_query);

	// format: string -> format
	parameter_types[0] = nv_compiler_types.string;
	np_Compiler_insert_function_cstr
		(compiler, "format", nv_compiler_types.format,
		 parameter_types, parameter_types + 1, 0, 0,
		 nv_function_format);

	// info goes totally on the type here
	// schema: void -> 0 of type schema
	np_Compiler_insert_function_cstr
		(compiler, "schema", nv_compiler_types.schema,
		 0, 0, 0, 0,
		 nv_function_schema);

	// info goes totally on the type here
	// schema: void -> 0 of type schema
	np_Compiler_insert_function_cstr
		(compiler, "version", nv_compiler_types.version,
		 0, 0, 0, 0,
		 nv_function_version);


}


//---------------------------------------------------
//
// from nm_Table with nv_TableValues to nvr_Table
//
//---------------------------------------------------

//
// @note the function below depends on nanocube_vector_table.c
//

//
// there is an implicit assumption here that nm_Table has an
// nv_TableValue payload
//
internal nvr_Table*
nvr_new_table(void *buffer, u64 size, nm_Table *input_table)
{
	Assert( LALIGN((u64) buffer, 8) == (u64) buffer );
	Assert(size >= sizeof(nvr_Table));

	nm_TableKeys   *table_keys   = &input_table->table_keys;
	nv_TableValues *table_values = input_table->table_values_handle;

	nvr_Table *table = buffer;
	table->left = sizeof(nvr_Table);
	table->capacity = size;
	table->rows = table_keys->rows;
	table->num_index_columns = table_keys->columns;
	table->num_value_columns = table_values->columns;

	// reserve and prepare index column info
	table->index_columns_info = nvr_Table_alloc(table, table->num_index_columns * sizeof(nvr_IndexColumnInfo));

	// index column info
	nvr_IndexColumnInfo *index_column_info_array = nvr_Table_deref(table, table->index_columns_info.offset);
	s32 record_offset = 0;
	for (s32 i=0;i<table->num_index_columns;++i) {
		nvr_IndexColumnInfo    *dst_info = index_column_info_array + i;
		nm_TableKeysColumnType *src_info = table_keys->type->begin + i;

		dst_info->index = i;

		// copy name
		s64 name_length = MemoryBlock_length(&src_info->name);
		dst_info->name = nvr_Table_alloc(table, name_length + 1); // reserve space for null termination
		char *dst_name= nvr_Table_deref(table, dst_info->name.offset);
		pt_copy_bytesn(src_info->name.begin, dst_name, name_length);
		dst_name[name_length] = 0;

		// initialize description of index structure
		if (src_info->loop_column) {
			dst_info->num_levels = 1;
			dst_info->bits_per_level = 32;
			dst_info->byte_offset = record_offset;
			dst_info->num_bytes = 4;
		} else {
			dst_info->num_levels = src_info->levels;
			dst_info->bits_per_level = src_info->bits;
			dst_info->byte_offset = record_offset;
			dst_info->num_bytes = RALIGN(src_info->levels*src_info->bits,8)/8;
		}
		record_offset += dst_info->num_bytes;
	}
	record_offset = RALIGN(record_offset,8);

	// value column info
	table->value_columns_info = nvr_Table_alloc(table, table->num_value_columns * sizeof(nvr_ValueColumnInfo));
	nvr_ValueColumnInfo *value_column_info_array = nvr_Table_deref(table, table->value_columns_info.offset);
	for (s32 i=0;i<table->num_value_columns;++i) {
		nvr_ValueColumnInfo    *dst_info = value_column_info_array + i;
		// nm_TableKeysColumnType *src_info = table_values->type->begin + i;
		dst_info->index = i;

		MemoryBlock src_name = table_values->names.begin[i];
		u32 name_length = (u32) MemoryBlock_length(&src_name);
		dst_info->name = nvr_Table_alloc(table, name_length + 1); // reserve space for null termination
		char *dst_name= nvr_Table_deref(table, dst_info->name.offset);
		pt_copy_bytesn(src_name.begin, dst_name, name_length);
		dst_name[name_length] = 0;

		MemoryBlock *begin;
		MemoryBlock *end;
		dst_info->byte_offset = record_offset;
		dst_info->num_bytes = 8;
		dst_info->padding = 0;

		record_offset += 8; // all are f64 numbers little endian
	}

	table->record_size = record_offset;

	// assuming it will fit
	table->data = nvr_Table_alloc(table, table->record_size * table->rows);

	char *dst = nvr_Table_deref(table, table->data.offset);
	char *src_key   = table_keys->keys.begin;
	char *src_value = (char*) table_values->values.begin;
	for (s32 i=0;i<table->rows;++i) {
		for (s32 j=0;j<table->num_index_columns;++j) {
			nvr_IndexColumnInfo *info = index_column_info_array + j;
			pt_copy_bytesn(src_key + info->byte_offset, dst + info->byte_offset, info->num_bytes);
		}
		for (s32 j=0;j<table->num_value_columns;++j) {
			nvr_ValueColumnInfo *info = value_column_info_array + j;
			pt_copy_bytesn(src_value + (j * info->num_bytes), dst + info->byte_offset, info->num_bytes);
		}
		dst += table->record_size;
		src_key += table_keys->row_length;
		src_value += table_values->columns * sizeof(f64);
	}

	return table;

}


//-------------------------------------------
// nanocube vector schema
//-------------------------------------------

#include "nanocube_vector_schema.c"

static nvs_Schema*
nv_prepare_nvs_schema(nv_Nanocube *nanocube, char *name, s32 name_length, void *buffer, s32 buffer_length)
{
	nvs_Schema *schema = nvs_init_schema(buffer, buffer_length, name, name_length);

	// schema doesn't fit on the given buffer_length
	if (!schema) {
		return 0;
	}

	for (s32 i=0;i<nanocube->num_index_dimensions;++i) {
		// nvs_Schema_push_index_dimension(nvs_Schema *self, char *name, u8 bits_per_level, u8 num_levels, u8 hint)
		// @todo write the hint instead of zero
		nvs_Schema_push_index_dimension(schema,
						nanocube->index_dimensions.names[i],
						nanocube->index_dimensions.bits_per_level[i],
						nanocube->index_dimensions.num_levels[i],
						0);
	}

	for (s32 i=0;i<nanocube->num_measure_dimensions;++i) {
		nvs_Schema_push_measure_dimension(schema,
						nanocube->measure_dimensions.names[i],
						(u8) nanocube->measure_dimensions.storage[i]);
	}
	return schema;
}


//-------------------------------------------
// print
//-------------------------------------------

typedef struct {
	ut_PrintStack print_stack;
	nv_Format     format;
	s32           objects_printed;
} nv_ResultStream;

internal void
nv_ResultStream_init(nv_ResultStream *self, Print *print, nv_Format format)
{
	self->format = format;
	self->objects_printed = 0;
	switch(self->format.format) {
	case nv_FORMAT_JSON: {
		ut_PrintStack_init(&self->print_stack, print, "\t", "\n");
	} break;
	case nv_FORMAT_TEXT: {
		ut_PrintStack_init(&self->print_stack, print, "\t", "\n");
	} break;
	case nv_FORMAT_BINARY: {
		ut_PrintStack_init(&self->print_stack, print, "", "");
	} break;
	default: {
	} break;
	}
}

internal void
nv_ResultStream_begin(nv_ResultStream *self)
{
	switch(self->format.format) {
	case nv_FORMAT_JSON: {
		ut_PrintStack_push(&self->print_stack, "[", "]\n");
	} break;
	case nv_FORMAT_TEXT: {
		ut_PrintStack_push(&self->print_stack, 0, 0);
	} break;
	case nv_FORMAT_BINARY: {
	} break;
	default: {
	} break;
	}
}

internal void
nv_ResultStream_end(nv_ResultStream *self)
{
	switch(self->format.format) {
	case nv_FORMAT_JSON: {
		ut_PrintStack_pop(&self->print_stack);
	} break;
	case nv_FORMAT_TEXT: {
		ut_PrintStack_pop(&self->print_stack);
	} break;
	case nv_FORMAT_BINARY: {
	} break;
	default: {
	} break;
	}
}

internal void
nv_ResultStream_sep(nv_ResultStream *self)
{
	if (self->objects_printed > 0) {
		switch (self->format.format) {
		case nv_FORMAT_JSON: {
			ut_PrintStack_append_cstr(&self->print_stack,",");
		} break;
		case nv_FORMAT_TEXT: {
		} break;
		case nv_FORMAT_BINARY: {
		} break;
		default: {
		} break;
		}
	}
}


// TODO(llins): fill in linear result using relative offset
typedef struct {
} nv_BinaryResult;

typedef struct {
	u32 begin;
	u32 end;
} nv_BinaryResult_Text;

typedef struct {
} nv_BinaryResult_Column;

//
// @TODO(llins) 2017-06-22T11:48
// Sort aliases by path. Should help integrating it later.
//

internal void
nv_ResultStream_version(nv_ResultStream *self, char *api, char *executable)
{
	Print         *print       = self->print_stack.print;
	ut_PrintStack *print_stack = &self->print_stack;
	// separate from previous pring if necessary
	nv_ResultStream_sep(self);
	++self->objects_printed;
	switch (self->format.format) {
	case nv_FORMAT_JSON: {
		ut_PrintStack_push(print_stack, "{", "}");
		{
			ut_PrintStack_print(print_stack, "\"type\":\"version\",");
			ut_PrintStack_print(print_stack, "\"api\":\"");
			ut_PrintStack_append_cstr(print_stack, api);
			ut_PrintStack_append_cstr(print_stack, "\",");
			ut_PrintStack_print(print_stack, "\"executable\":\"");
			ut_PrintStack_append_cstr(print_stack, executable);
			ut_PrintStack_append_cstr(print_stack, "\"");
		}
		ut_PrintStack_pop(print_stack);
	} break;
	case nv_FORMAT_TEXT: {
	{
		// TODO(llins) print aliases on the text schema?
		Print_format(print, "# version\napi: %s\nexecutable: %s", api, executable);
	}
	} break;
	case nv_FORMAT_BINARY: {
		// TODO(llins): send as a table
	} break;
	}
}

internal void
nv_ResultStream_schema(nv_ResultStream *self, MemoryBlock name, nv_Nanocube *nanocube)
{
	Print         *print       = self->print_stack.print;
	ut_PrintStack *print_stack = &self->print_stack;
	// separate from previous pring if necessary
	nv_ResultStream_sep(self);
	++self->objects_printed;
	switch (self->format.format) {
	case nv_FORMAT_JSON: {
		ut_PrintStack_push(print_stack, "{", "}");
		{
			ut_PrintStack_print(print_stack, "\"type\":\"schema\",");
			ut_PrintStack_print(print_stack, "\"name\":\"");
			ut_PrintStack_append_str(print_stack, name.begin, name.end);
			ut_PrintStack_append_cstr(print_stack, "\",");

			ut_PrintStack_push(print_stack, "\"index_dimensions\":[", "],");
			for (u32 i=0;i<nanocube->num_index_dimensions;++i) {
				if (i > 0) {
					ut_PrintStack_append_cstr(print_stack, ",");
				}
				ut_PrintStack_push(print_stack, "{", "}");
				ut_PrintStack_print_formatted(print_stack, "\"index\":%d,", i);
				ut_PrintStack_print_formatted(print_stack, "\"name\":\"%s\",",nanocube->index_dimensions.names[i]);
				ut_PrintStack_print_formatted(print_stack, "\"bits_per_level\":%d,", nanocube->index_dimensions.bits_per_level[i]);
				ut_PrintStack_print_formatted(print_stack, "\"num_levels\":%d,", nanocube->index_dimensions.num_levels[i]);
				MemoryBlock hint = nv_Nanocube_get_dimension_hint(nanocube,
										  nanocube->index_dimensions.names[i],
										  cstr_end(nanocube->index_dimensions.names[i]),
										  print);
				ut_PrintStack_print(print_stack, "\"hint\":\"");
				ut_PrintStack_append_str(print_stack, hint.begin, hint.end);
				ut_PrintStack_append_cstr(print_stack, "\",");

				// dictionary with key -> alias
				//
				{
					ut_PrintStack_push(print_stack, "\"aliases\":{", "}");
					{
						// hacky way to get aliases. Iterate through all the path aliases of all dimensions
						// to the the ones for the dimension we want
						bt_Iter iter;
						bt_Iter_init(&iter, &nanocube->key_value_store);
						bt_Hash     alias_hash;
						MemoryBlock alias_key;
						MemoryBlock alias_value;
						char  buffer[128];
						Print print_prefix;
						Print_init(&print_prefix, buffer, buffer + sizeof(buffer));
						Print_format(&print_prefix,"%s:kv:",nanocube->index_dimensions.names[i]);
						u64 prefix_length = Print_length(&print_prefix);
						s32 num_aliases = 0;
						while (bt_Iter_next(&iter, &alias_hash, &alias_key, &alias_value))  {
							if (MemoryBlock_length(&alias_key) < prefix_length) {
								continue;
							}
							if (pt_compare_memory(alias_key.begin, alias_key.begin + prefix_length, print_prefix.begin, print_prefix.end)==0) {
								if (num_aliases > 0) {
									ut_PrintStack_append_cstr(print_stack, ",");
								}
								ut_PrintStack_print(print_stack, "\"");
								ut_PrintStack_append_str(print_stack, alias_key.begin + prefix_length, alias_key.end);
								ut_PrintStack_append_cstr(print_stack, "\":\"");
								ut_PrintStack_append_escaped_json_str(print_stack, alias_value.begin, alias_value.end);
								ut_PrintStack_append_cstr(print_stack, "\"");
								++num_aliases;
							}
						}
					}
					ut_PrintStack_pop(print_stack);
				}

				ut_PrintStack_pop(print_stack);
			}
			ut_PrintStack_pop(print_stack);

			ut_PrintStack_push(print_stack, "\"measure_dimensions\":[", "]");
			for (u32 i=0;i<nanocube->num_measure_dimensions;++i) {
				if (i > 0) {
					ut_PrintStack_append_cstr(print_stack, ",");
				}
				ut_PrintStack_push(print_stack, "{", "}");
				ut_PrintStack_print_formatted(print_stack, "\"order\":%d,", i+1);
				ut_PrintStack_print_formatted(print_stack, "\"name\":\"%s\",",nanocube->measure_dimensions.names[i]);
				ut_PrintStack_print_formatted(print_stack, "\"type\":\"%s\"", nv_storage_name_cstr(nanocube->measure_dimensions.storage[i]));
				ut_PrintStack_pop(print_stack);
			}
			ut_PrintStack_pop(print_stack);
		}
		ut_PrintStack_pop(print_stack);
	} break;
	case nv_FORMAT_TEXT: {
	{

		// TODO(llins) print aliases on the text schema?

		Print_cstr(print, "# schema: ");
		Print_str(print, name.begin, name.end);
		Print_char(print,'\n');
		Print_char(print,'#');
		Print_align(print, 53, -1, '#');
		Print_char(print,'\n');
		Print_format(print,"%1s %32s %10s %7s %s\n", "#", "index_dimension", "bits", "levels", "hint");
		Print_char(print,'#');
		Print_align(print, 53, -1, '#');
		Print_char(print,'\n');
		for (u32 i=0;i<nanocube->num_index_dimensions;++i) {
			MemoryBlock hint = nv_Nanocube_get_dimension_hint(nanocube,
									  nanocube->index_dimensions.names[i],
									  cstr_end(nanocube->index_dimensions.names[i]),
									  print);
			Print_format(print,
				     "%1d %32s %10d %7d %s\n",
				     i+1,
				     nanocube->index_dimensions.names[i],
				     nanocube->index_dimensions.bits_per_level[i],
				     nanocube->index_dimensions.num_levels[i],
				     hint.begin);
		}
		Print_char(print,'#');
		Print_align(print, 53, -1, '#');
		Print_char(print,'\n');
		Print_format(print, "%1s %32s %10s\n", "#", "measure_dimension", "format");
		Print_char(print,'#');
		Print_align(print, 53, -1, '#');
		Print_char(print,'\n');
		for (u32 i=0;i<nanocube->num_measure_dimensions;++i) {
			Print_format(print,
				     "%1d %32s %10s\n",
				     i+1,
				     nanocube->measure_dimensions.names[i],
				     nv_storage_name_cstr(nanocube->measure_dimensions.storage[i]));
		}
	}
	} break;
	case nv_FORMAT_BINARY: {
		//
		// @todo if multiple schemas are printed, there is no way the client will
		// be able to know it. For the moment, assume a single nanocube per
		// server is being served for clients using the binary schema.
		//
		Assert(((u64)print->begin % 8)==0);
		void *buffer = print->end;
		s64 buffer_length = print->capacity - print->end;
		buffer_length = LALIGN(buffer_length, 8);
		// Assert(print->begin == print->end);
		nvs_Schema *schema = nv_prepare_nvs_schema(nanocube, name.begin, (s32) MemoryBlock_length(&name), buffer, buffer_length);
		if (!schema) {
			fputs("[fatal] Not enough memory to write binary schema", stderr);
			Assert(0 && "[fatal] Not enough memory to write binary schema");
			exit(-1);
		}
		nvs_pack(schema, nvs_smallest_size(schema));
		print->end = print->end + schema->size;
	} break;
	}
}

internal void
nv_ResultStream_table(nv_ResultStream *self, nm_Table *table)
{
	Print         *print       = self->print_stack.print;
	ut_PrintStack *print_stack = &self->print_stack;
	// separate from previous pring if necessary
	nv_ResultStream_sep(self);
	++self->objects_printed;

	switch (self->format.format) {
		case nv_FORMAT_BINARY: {
			// assuming that the result stream can fit the result
			Assert(print->begin == print->end);
			char *buffer = print->end;
			s64   buffer_size = print->capacity - print->end;
			//
			// @todo: fallback mechanism when buffer doesn't have enough capacity
			// @todo: what about multiple tables per query? doesn't work with binary data
			//
			nvr_Table *nvr_table = nvr_new_table(buffer, buffer_size, table);
			nvr_table->capacity = nvr_table->left;
			print->end = buffer + nvr_table->left;
		} break;
		case nv_FORMAT_JSON: {
		ut_PrintStack_push(print_stack, "{", "}");
		{
			nm_TableKeys *table_keys = &table->table_keys;
			ut_PrintStack_print(print_stack, "\"type\":\"table\",");

			s32 rows = table_keys->rows;
			ut_PrintStack_print_formatted(print_stack, "\"numrows\":%d,", rows);

			ut_PrintStack_push(print_stack, "\"index_columns\":[", "],");
			// index columns...
			u32 record_size   = table_keys->row_length;
			u32 column_offset = 0;
			for (u32 i=0;i<table_keys->columns;++i) {

				nm_TableKeysColumnType *it_coltype = table_keys->type->begin + i;

				if (it_coltype != table_keys->type->begin) {
					ut_PrintStack_append_cstr(print_stack, ",");
				}

				ut_PrintStack_push(print_stack, "{", "}");
				ut_PrintStack_print(print_stack, "\"name\":\"");
				ut_PrintStack_append_str(print_stack, it_coltype->name.begin, it_coltype->name.end);
				ut_PrintStack_append_cstr(print_stack, "\",");

				u8 levels = it_coltype->levels;
				u8 bits   = it_coltype->bits;

				switch(it_coltype->hint.hint_id) {
				case nm_BINDING_HINT_NONE: {
					ut_PrintStack_print(print_stack, "\"hint\":\"none\",");
					s32 values_per_row;
					if (it_coltype->loop_column) {
						values_per_row = 1;
					} else {
						values_per_row = levels;
					}
					ut_PrintStack_print_formatted(print_stack, "\"values_per_row\":%d,", values_per_row);

					ut_PrintStack_push(print_stack, "\"values\":[", "]");
					if (it_coltype->loop_column) {
						char *it = table_keys->keys.begin + column_offset;
						if (rows) {
							ut_PrintStack_print_formatted(print_stack, "%d", ((u32*)it)[0]);
							for (u32 i=1;i<rows;++i) {
								it += record_size;
								ut_PrintStack_append_cstr(print_stack, ",");
								ut_PrintStack_append_formatted(print_stack, "%d", ((u32*)it)[0]);
							}
						}
					} else {
						char *it = table_keys->keys.begin + column_offset;
						if (rows) {
							ut_PrintStack_print(print_stack, "");
							for (u32 i=0;i<rows;++i) {
								for (u32 j=0;j<levels;++j) {
									nx_Label label = 0;
									pt_read_bits2(it, bits * (levels - 1 - j), bits, (char*) &label);
									if (i > 0 || j > 0) {
										ut_PrintStack_append_cstr(print_stack, ",");
									}
									ut_PrintStack_append_formatted(print_stack, "%llu", (u64) label);
								}
								it += record_size;
							}
						}
					}
					ut_PrintStack_pop(print_stack);

				} break;
				case nm_BINDING_HINT_IMG: {
					if (it_coltype->loop_column) {
						ut_PrintStack_print(print_stack, "\"error\":\"expected a path, received a loop column\"");
					} else if (it_coltype->bits != 2) {
						ut_PrintStack_print(print_stack, "\"error\":\"expected a quadtree path\"");
					} else {
						u32 param = (u32) ((u64) it_coltype->hint.param);
						ut_PrintStack_print_formatted(print_stack, "\"hint\":\"img%llu\",",param);
						ut_PrintStack_print(print_stack, "\"values_per_row\":2,");
						// values will be listed as x, y
						ut_PrintStack_push(print_stack, "\"values\":[", "]");
						char *it = table_keys->keys.begin + column_offset;
						if (rows) {
							ut_PrintStack_print(print_stack, "");
							for (u32 i=0;i<rows;++i) {
								u64 x = 0;
								u64 y = 0;
								u32 j0 = (param < levels) ? (levels - param) : 0;
								for (u32 j=j0;j<levels;++j) {
									x = x * 2;
									y = y * 2;
									nx_Label label = 0;
									pt_read_bits2(it, bits * (levels - 1 - j), bits, (char*) &label);
									if (label & 0x1) {
										x += 1;
									}
									if (label & 0x2) {
										y += 1;
									}
								}
								it += record_size;
								if (i > 0) {
									ut_PrintStack_append_cstr(print_stack, ",");
								}
								ut_PrintStack_append_formatted(print_stack, "%llu,%llu", x, y);
							}
						}
						ut_PrintStack_pop(print_stack);
					}
				} break;
				case nm_BINDING_HINT_NAME: {
					if (it_coltype->loop_column) {
						ut_PrintStack_print(print_stack, "\"error\":\"expected a path, received a loop column\"");
					} else {
						// with the column name and the source, try to find the column names
						Assert(table->source->num_nanocubes > 0);
						nv_Nanocube *nanocube = (nv_Nanocube*) table->source->nanocubes[0];

						// temporarily
						ut_PrintStack_print(print_stack, "\"hint\":\"name\",");
						s32 values_per_row = 1;
						ut_PrintStack_print_formatted(print_stack, "\"values_per_row\":%d,", values_per_row);

						ut_PrintStack_push(print_stack, "\"values\":[", "]");
						char *it = table_keys->keys.begin + column_offset;
						if (rows) {
							ut_PrintStack_print(print_stack, "");
							for (u32 i=0;i<rows;++i) {

								// NOTE(llins) we use the print object buffer to retrieve
								// node path to query for the path name
								u8 *path = (u8*) print->end;
								print->end += levels;

								for (u32 j=0;j<levels;++j) {
									nx_Label label = 0;
									pt_read_bits2(it, bits * (levels - 1 - j), bits, (char*) &label);
									path[j] = label;
								}

								it += record_size;

								MemoryBlock label =
									nv_Nanocube_get_dimension_path_name(nanocube,
													    it_coltype->name.begin, it_coltype->name.end,
													    path, (u8) levels, print);
								print->end = (char*) path;

// 								char *key_begin = print->end;
// 								Print_str(print, it_coltype->name.begin, it_coltype->name.end);
// 								Print_cstr(print, ":kv:");
// 								Print_u64(print, value);
// 								// nv_Nanocube_insert_key_value(nanocube,print->begin, print->end, cat_text->begin, cat_text->end);
// 								MemoryBlock label = nv_Nanocube_get_dimension_path_name(nanocube, key_begin, print->end);
// 								print->end = key_begin;

								if (i > 0)
									ut_PrintStack_append_cstr(print_stack, ",");
								ut_PrintStack_append_cstr(print_stack, "\"");
								ut_PrintStack_append_str(print_stack, label.begin, label.end);
								ut_PrintStack_append_cstr(print_stack, "\"");
							}
						}
						ut_PrintStack_pop(print_stack);
					}
				} break;
				case nm_BINDING_HINT_TIME: {
				} break;
				default:
					break;
				} // switch hint
				ut_PrintStack_pop(print_stack);

				column_offset += nm_TableKeysColumnType_bytes(it_coltype);

			} // iterate through index columns
			ut_PrintStack_pop(print_stack);

// 			nm_TableKeys *table_keys = &table->table_values_handle;
// 			ut_PrintStack_print(print_stack, "\"type\":\"table\",");
// 			internal void nv_TableValues_print_json(nv_TableValues *self, Print *print);

			nv_TableValues *table_values = (nv_TableValues*) table->table_values_handle;
			ut_PrintStack_push(print_stack, "\"measure_columns\":[", "]");
			record_size = table_values->columns;
			for (u32 j=0;j<table_values->columns;++j) {
				if (j > 0) {
					ut_PrintStack_append_cstr(print_stack, ",");
				}
				MemoryBlock *name =table_values->names.begin + j;

				ut_PrintStack_push(print_stack, "{", "}");
				{
					ut_PrintStack_print(print_stack, "\"name\":\"");
					ut_PrintStack_append_str(print_stack, name->begin, name->end);
					ut_PrintStack_append_cstr(print_stack, "\",");

					ut_PrintStack_push(print_stack, "\"values\":[", "]");
					f64* it = table_values->values.begin + j;
					ut_PrintStack_print(print_stack, "");
					for (u32 i=0;i<table_values->rows;++i) {
						if (i > 0) {
							ut_PrintStack_append_cstr(print_stack, ",");
						}
						ut_PrintStack_append_formatted(print_stack, "%f", *it);
						// Print_f64(print, *it);
						it += record_size;
					}
					ut_PrintStack_pop(print_stack);
				}
				ut_PrintStack_pop(print_stack);
			}
			ut_PrintStack_pop(print_stack);

			// close column arrays
		} // end table object
		ut_PrintStack_pop(print_stack);
	} break;
	case nv_FORMAT_TEXT: {

		//
		// going to use the print object directly since we don't
		// nest anything in the text output
		//

		nm_TableKeys   *table_keys   = &table->table_keys;
		nv_TableValues *table_values = (nv_TableValues*) table->table_values_handle;

		s32 rows = table_keys->rows;
		Assert(rows == table_values->rows);

		static const s32 index_col_fixed_width = 50;
		static const s32 value_col_fixed_width = 32;

		// print header
		{
			{ // index part
				nm_TableKeysColumnType *it = table_keys->type->begin;
				while (it != table_keys->type->end) {
					Print_str(print, it->name.begin, it->name.end);
					Print_align(print, index_col_fixed_width, 1, ' ');
					++it;
				}
			}
			{ // value part
				MemoryBlock *it = table_values->names.begin;
				while (it != table_values->names.end) {
					Print_str(print, it->begin, it->end);
					Print_align(print, value_col_fixed_width, 1, ' ');
					++it;
				}
			}
			Print_char(print, '\n');
		}
		// print rows
		for (u32 row=0;row<rows;++row) {
#if 1
			{ // index columns
				char *it = table_keys->keys.begin + table_keys->row_length * row;
				u32 column_offset = 0;
				u32 record_size   = table_keys->row_length;
				for (u32 i=0;i<table_keys->columns;++i) {

					nm_TableKeysColumnType *it_coltype = table_keys->type->begin + i;

					char *check_point = print->end;

					switch(it_coltype->hint.hint_id) {
					case nm_BINDING_HINT_NONE: {
						if (it_coltype->loop_column) {
							u32 val = *((u32*) it);
							Print_u64(print, (u64) val);
							it += sizeof(u32);
						} else {
							u32 bits   = it_coltype->bits;
							u32 levels = it_coltype->levels;
							u32 bytes  = (it_coltype->bits * it_coltype->levels + 7)/8;

							// it_coltype specific stuff

							for (u32 j=0;j<levels;++j) {
								nx_Label label = 0;
								pt_read_bits2(it, bits * (levels - 1 - j), bits, (char*) &label);
								if (j > 0) {
									Print_cstr(print, ",");
								}
								Print_u64(print, (u64) label);
							}
							it += bytes;
						}
					} break;
					case nm_BINDING_HINT_IMG: {
						// TODO(llins): generalize to 2d
						if (it_coltype->loop_column) {
							Print_cstr(print, "error: expected a path, received a loop column");
						} else if (it_coltype->bits != 2) {
							Print_cstr(print, "error: expected a quadtree path");
						} else {
							u32 bytes  = (it_coltype->bits * it_coltype->levels + 7)/8;
							u32 bits   = it_coltype->bits;
							u32 levels = it_coltype->levels;
							u32 param = (u32) ((u64) it_coltype->hint.param);
							u64 x = 0;
							u64 y = 0;
							u32 j0 = (param < levels) ? (levels - param) : 0;
							for (u32 j=j0;j<levels;++j) {
								x = x * 2;
								y = y * 2;
								nx_Label label = 0;
								pt_read_bits2(it, bits * (levels - 1 - j), bits, (char*) &label);
								if (label & 0x1) {
									x += 1;
								}
								if (label & 0x2) {
									y += 1;
								}
							}
							it += bytes;
							if (i > 0) {
								Print_char(print, ',');
							}
							Print_format(print,"%llu,%llu", x, y);
						}
					} break;
					case nm_BINDING_HINT_NAME: {
						if (it_coltype->loop_column) {
							Print_cstr(print, "error: expected a path, received a loop column");
						} else {
							// with the column name and the source, try to find the column names
							Assert(table->source->num_nanocubes > 0);
							nv_Nanocube *nanocube = (nv_Nanocube*) table->source->nanocubes[0];

							u32 bits   = it_coltype->bits;
							u32 levels = it_coltype->levels;

							u8 *path = (u8*) print->end;
							print->end += levels;

							for (u32 j=0;j<levels;++j) {
								nx_Label label = 0;
								pt_read_bits2(it, bits * (levels - 1 - j), bits, (char*) &label);
								path[j] = label;
							}
							it += record_size;
							MemoryBlock label = nv_Nanocube_get_dimension_path_name(nanocube,
												    it_coltype->name.begin, it_coltype->name.end,
												    path, (u8) levels, print);
							print->end = (char*) path;
							Print_str(print, label.begin, label.end);
						}
					}
					default:{
					}break;
					}

					// align stuff that was printed
					Print_fake_last_print(print, check_point);
					Print_align(print, index_col_fixed_width, 1, ' ');

					column_offset += nm_TableKeysColumnType_bytes(it_coltype);
				}
			}
#endif
			{ // value columns
				f64 *it = table_values->values.begin + table_values->columns * row;
				for (u32 j=0;j<table_values->columns;++j) {
					f64 value = *(it + j);
					Print_format(print, "%f", value);
					Print_align(print, value_col_fixed_width, 1, ' ');
				}
			}
			Print_char(print, '\n');

		} // end row printing loop
	} break;
	default: break;
	} // end switch
}

internal void
nv_Compiler_insert_nanocube(np_Compiler *compiler, nv_Nanocube* cube, char *name_begin, char *name_end)
{
	np_Symbol* symbol = np_SymbolTable_find_variable(&compiler->symbol_table, name_begin, name_end);
	if (symbol) {
		Assert(symbol->is_variable);
		Assert(symbol->variable.type_id == nv_compiler_types.measure);
		nm_Measure *measure = (nm_Measure*) symbol->variable.value;
		Assert(measure->num_sources == 1);

		// TODO(llins): check that the cube is compatible with previously inserted cubes
		nm_MeasureSource_insert_nanocube(measure->sources[0], &cube->index, cube);
	} else {

		u32 n = (u32) cube->num_index_dimensions;
		// reserve space for MeasureSource names and link them
		MemoryBlock *names= (MemoryBlock*) np_Compiler_alloc(compiler, sizeof(MemoryBlock) * n);
		for (u32 i=0;i<n;++i) {
			names[i].begin = cube->index_dimensions.names[i];
			names[i].end   = cstr_end(cube->index_dimensions.names[i]);
			// 		printf("%d %p %p ----------> [",(s32)*names[i].begin, names[i].begin, names[i].end);
			// 		fwrite(names[i].begin, 1, 20, stdout);
			// 		printf("]\n");
		}

		nm_MeasureSource *source= (nm_MeasureSource*) np_Compiler_alloc(compiler, sizeof(nm_MeasureSource));
		nm_MeasureSource_init(source, names, names+n, cube->index_dimensions.num_levels, cube->index_dimensions.num_levels + n);
		nm_MeasureSource_insert_nanocube(source, &cube->index, cube);

		nm_Measure *measure= (nm_Measure*) np_Compiler_alloc(compiler, sizeof(nm_Measure));
		nm_Measure_init(measure, source, compiler->memory); // this measure should be copied and
		// a buffer added before it can be combined

		np_Compiler_insert_variable(compiler, name_begin, name_end, nv_compiler_types.measure, measure);
	}
}

// replace the linked nanocube on variable named [name_begin,name_end)
// returns the old nanocube pointer
internal nv_Nanocube*
nv_Compiler_update_singleton_nanocube_symbol(np_Compiler *compiler, nv_Nanocube* cube, char *name_begin, char *name_end)
{
	// no allocation on the compiler memory should happend
	// here. this is a lightweight update on the measure
	// beign pointed by the symbol named [name_begin,name_end).
	np_Symbol* symbol = np_SymbolTable_find_variable(&compiler->symbol_table, name_begin, name_end);
	Assert(symbol);
	Assert(symbol->is_variable);
	Assert(symbol->variable.type_id == nv_compiler_types.measure);
	nm_Measure *measure = (nm_Measure*) symbol->variable.value;
	Assert(measure->num_sources == 1);

	nm_MeasureSource *source = measure->sources[0];

	nv_Nanocube *old_nanocube = (nv_Nanocube*) source->nanocubes[0];

	// the names don't change, but the levels should point
	// into the new cube
	u32 n = (u32) cube->num_index_dimensions;
	MemoryBlock *names = source->names.begin;
	Assert(names + n == source->names.end);
	for (u32 i=0;i<n;++i) {
		names[i].begin = cube->index_dimensions.names[i];
		names[i].end   = cstr_end(cube->index_dimensions.names[i]);
		// 		printf("%d %p %p ----------> [",(s32)*names[i].begin, names[i].begin, names[i].end);
		// 		fwrite(names[i].begin, 1, 20, stdout);
		// 		printf("]\n");
	}
	nm_MeasureSource_init(source, names, names + n, cube->index_dimensions.num_levels, cube->index_dimensions.num_levels + n);
	nm_MeasureSource_insert_nanocube(source, &cube->index, cube);
	Assert(source->num_nanocubes == 1);
	return old_nanocube;
}

internal void
nv_Compiler_insert_nanocube_cstr(np_Compiler *compiler, nv_Nanocube* cube, char *name_cstr)
{
	nv_Compiler_insert_nanocube(compiler, cube, name_cstr, cstr_end(name_cstr));
}

//------------------------------------------------------------------------------
//
// Custom payload: a vector
//
//------------------------------------------------------------------------------

internal void
nx_PayloadAggregate_init(nx_PayloadAggregate *self, void *payload_context)
{
	nv_Nanocube *nanocube = (nv_Nanocube*) payload_context;
	al_Ptr_char_set_null(&self->data);

	al_Cache *payload_cache = al_Ptr_Cache_get(&nanocube->cache_payload);
	char *it = (char*) al_Cache_alloc(payload_cache);

	al_Ptr_char_set(&self->data, it);

	// clear all the numbers
	pt_fill(it, it + nanocube->payload_aggregate_size, 0);
}

internal void
nx_PayloadAggregate_share(nx_PayloadAggregate* self, nx_PayloadAggregate* other, void *payload_context)
{
	nv_Nanocube *nanocube = (nv_Nanocube*) payload_context;
// 	al_Cache *payload_cache = al_Ptr_Cache_get(&nanocube->cache_payload);

	char *it_src = al_Ptr_char_get(&other->data);
	Assert(!al_Ptr_char_is_null(&self->data));
	char *it_dst = al_Ptr_char_get(&self->data); // (char*) al_Cache_alloc(payload_cache);

	al_Ptr_char_set(&self->data, it_dst);
	pt_copy_bytes(it_src, it_src + nanocube->payload_aggregate_size,
			it_dst, it_dst + nanocube->payload_aggregate_size);
}

internal void
nx_PayloadAggregate_insert(nx_PayloadAggregate* self, void* payload_unit, void* payload_context)
{
	nv_Nanocube *nanocube = (nv_Nanocube*) payload_context;
	char *it_in  = (char*) payload_unit;
	char *it_out = al_Ptr_char_get(&self->data);
	for(u64 i=0;i<nanocube->num_measure_dimensions;++i) {
		switch(nanocube->measure_dimensions.storage[i]) {
		case nv_NUMBER_STORAGE_SIGNED_32: {
			s32 *in  = (s32*) it_in;
			s32 *out = (s32*) it_out;
			*out += *in;
			it_in  += sizeof(s32);
			it_out += sizeof(s32);
		} break;
		case nv_NUMBER_STORAGE_SIGNED_64: {
			s64 *in  = (s64*) it_in;
			s64 *out = (s64*) it_out;
			*out += *in;
			it_in  += sizeof(s64);
			it_out += sizeof(s64);
		} break;
		case nv_NUMBER_STORAGE_UNSIGNED_32: {
			u32 *in  = (u32*) it_in;
			u32 *out = (u32*) it_out;
			*out += *in;
			it_in  += sizeof(u32);
			it_out += sizeof(u32);
		} break;
		case nv_NUMBER_STORAGE_UNSIGNED_64: {
			u64 *in  = (u64*) it_in;
			u64 *out = (u64*) it_out;
			*out += *in;
			it_in  += sizeof(u64);
			it_out += sizeof(u64);
		} break;
		case nv_NUMBER_STORAGE_FLOAT_32: {
			f32 *in  = (f32*) it_in;
			f32 *out = (f32*) it_out;
			*out += *in;
			it_in  += sizeof(f32);
			it_out += sizeof(f32);
		} break;
		case nv_NUMBER_STORAGE_FLOAT_64: {
			f64 *in  = (f64*) it_in;
			f64 *out = (f64*) it_out;
			*out += *in;
			it_in  += sizeof(f64);
			it_out += sizeof(f64);
		} break;
		default: {
			Assert(0 && "unexpected type");
		} break;
		}
	}
}



