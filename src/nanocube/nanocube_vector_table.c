//------------------------------------------------------------------------------
//
// nvr_Table
//
// Nanocube Vector Result: this is a binary format for the result of
// Nanocube Vector queries.
//
// Other projects should import this file
//
// @todo Create an easy to read object that encodes an nm_Table
// with nv_TableValue
//
// Maybe support lat/lon or text representation of tables when
// using a hint mechanism
//
// the problem with text is that it doesnt' align well
//
// keep it simple for now. with fixed record size.
//
//------------------------------------------------------------------------------


#ifndef NANOCUBE_VECTOR_RESULT
#define NANOCUBE_VECTOR_RESULT

//
// offset cannot be zero in any object
//

typedef struct {
	u32 offset;
	u32 length;
} nvr_Block;

typedef struct {
	nvr_Block name;
	u8  index;
	u8  bits_per_level;
	u16 num_levels;
	u16 byte_offset; // inside a record
	u16 num_bytes;
} nvr_IndexColumnInfo;

typedef struct {
	nvr_Block name;
	u16 index;
	u16 byte_offset;
	u16 num_bytes;
	u16 padding;
} nvr_ValueColumnInfo;

// there is a natural LOD when using NC
// it is implicit the capacity of an nv_SeriaTable
// just shift everything to the left
typedef struct {
	u32 rows;
	u32 record_size;
	u32 num_index_columns;
	u32 num_value_columns;
	nvr_Block index_columns_info;
	nvr_Block value_columns_info;
	nvr_Block data;
	s64 left;
	s64 capacity;
} nvr_Table;

internal void* nvr_Table_deref(nvr_Table *self, u32 offset);
internal nvr_Block nvr_Table_alloc(nvr_Table *self, u64 size);

#ifdef NANOCUBE_VECTOR_TABLE_IMPLEMENTATION
internal void*
nvr_Table_deref(nvr_Table *self, u32 offset)
{
	return ((char*) self) + offset;
}

internal nvr_Block
nvr_Table_alloc(nvr_Table *self, u64 size)
{
	// assuming left is a multiple of 8 bytes
	Assert( LALIGN((u64) self->left, 8) == (u64) self->left );
	u64 alloc_size = RALIGN(size,8);
	Assert(self->left + alloc_size <= self->capacity);
	nvr_Block result = { .offset = self->left, .length = size };
	self->left += alloc_size;
	return result;
}
#endif

#endif
