//
// A memory-layout way to represent the schema of a nanocube vector
// independent of details. This should be used in other projects
// that need to understand what is available in a nanocube.
//
// nvs_ stands for Nanocube Vector Schema
//

//
// [schema][idx_1...idx_n][m_1...m_n][a_{1,1} ... a_{1,k}][a_{2,1} ... a_{2,k}]----->....<-----[names]
//

typedef struct {
	u32 name; // right offset to cstr
	u8  bits_per_level;
	u8  num_levels;
	u16 hint;
	u32 alias_offset; // it is a left offset after the
	u32 alias_count;  // number of aliases
} nvs_IndexDimension;

typedef struct {
	u32 name; // right offset to cstr
	u32 storage;
} nvs_MeasureDimension;

typedef struct {
	u32 left;
	u32 right;
} nvs_CheckPoint;

//
// [nvs_Schema] [ index_dimensions ] [ measure_dimensions ] ---> ...  <--- [ strings ]
//
typedef struct {
	u16 num_index_dimensions;
	u16 num_measure_dimensions;
	u32 name; // name of the nanocube
	u32 left;
	u32 right;
	u32 size;
	u32 flags; // to be used by client applications as they wish
} nvs_Schema;

_Static_assert((sizeof(nvs_MeasureDimension)%8)==0, "sizeof(nvs_MeasureDimension) is not a multiple of 8");
_Static_assert((sizeof(nvs_IndexDimension)%8)==0, "sizeof(nvs_IndexDimension) is not a multiple of 8");
_Static_assert((sizeof(nvs_Schema)%8)==0, "sizeof(nvs_Schema) is not a multiple of 8");

static nvs_IndexDimension*
nvs_Schema_index_dimension_at(nvs_Schema *self, s32 index)
{
	Assert(index < self->num_index_dimensions);
	nvs_IndexDimension *array = OffsetedPointer(self, sizeof(nvs_Schema));
	return array + index;
}

static nvs_CheckPoint
nvs_Schema_checkpoint(nvs_Schema *self)
{
	return (nvs_CheckPoint) {
		.left = self->left,
		.right = self->right
	};
}

static char*
nvs_Schema_cstr(nvs_Schema *self, s32 right_offset)
{
	return (char*) RightOffsetedPointer(self, self->size, right_offset);
}

static void
nvs_Schema_restore(nvs_Schema *self, nvs_CheckPoint cp)
{
	self->left = cp.left;
	self->right = cp.right;
}

static nvs_MeasureDimension*
nvs_Schema_measure_dimension_at(nvs_Schema *self, s32 index)
{
	Assert(index < self->num_measure_dimensions);
	nvs_MeasureDimension *array = OffsetedPointer(self, sizeof(nvs_Schema) + self->num_index_dimensions * sizeof(nvs_IndexDimension));
	return array + index;
}

static s32
nvs_Schema_push_cstr(nvs_Schema *self, char *name)
{
	// right the cstring at the end
	s32 name_len = cstr_len(name);
	s32 right_requirement = RAlign(name_len+1,8);
	if (self->left + right_requirement > self->right) {
		return 0; // note that any valid string would have a positive number
	}
	self->right -= right_requirement;
	char *dst = OffsetedPointer(self,self->right);
	platform.copy_memory(dst, name, name_len);
	dst[name_len] = 0;
	// return the right offset
	return self->size - self->right;
}

static s32
nvs_Schema_push_str(nvs_Schema *self, char *name, s32 name_len)
{
	// right the cstring at the end
	s32 right_requirement = RAlign(name_len+1,8);
	if (self->left + right_requirement > self->right) {
		return 0; // note that any valid string would have a positive number
	}
	self->right -= right_requirement;
	char *dst = OffsetedPointer(self,self->right);
	platform.copy_memory(dst, name, name_len);
	dst[name_len] = 0;
	// return the right offset
	return self->size - self->right;
}

static nvs_IndexDimension*
nvs_Schema_push_index_dimension_(nvs_Schema *self)
{
	if (self->left + sizeof(nvs_IndexDimension) > self->right)
		return 0;
	++self->num_index_dimensions;
	self->left += sizeof(nvs_IndexDimension);
	return nvs_Schema_index_dimension_at(self, self->num_index_dimensions-1);
}

static nvs_MeasureDimension*
nvs_Schema_push_measure_dimension_(nvs_Schema *self)
{
	if (self->left + sizeof(nvs_MeasureDimension) > self->right)
		return 0;
	++self->num_measure_dimensions;
	self->left += sizeof(nvs_MeasureDimension);
	return nvs_Schema_measure_dimension_at(self, self->num_measure_dimensions-1);
}

//
// ready for a linear scan by adding length to current
// alias address
//
typedef struct {
	u16 value_offset;
	u16 length;
	char key[];
} nvs_Alias;

//
// @warning this push_alias mechanism will only work if the
// aliases are inserted in contiguous fashion per dimension,
// otherwise it will be messed up without detection
//
static s32
nvs_push_alias(nvs_Schema *self, s32 index_dimension, char *key_begin, char *key_end, char *value_begin, char *value_end)
{
	Assert(index_dimension < self->num_index_dimensions);
	nvs_IndexDimension *idx_dim = nvs_Schema_index_dimension_at(self, index_dimension);

	u16 key_length   = key_end - key_begin;
	u16 value_length = value_end - value_begin;

	u16 length = RAlign(sizeof(nvs_Alias) + key_length + value_length + 2, 8);

	if (self->left + length > self->right) { return 0; }

	nvs_Alias *alias = OffsetedPointer(self, self->left);

	if (idx_dim->alias_offset == 0) {
		Assert(idx_dim->alias_count == 0);
		idx_dim->alias_offset = self->left;
	}

	self->left += length;


	char *dst_key   = &alias->key[0];
	char *dst_value = dst_key + key_length + 1;

	alias->value_offset = sizeof(nvs_Alias) + key_length + 1;
	alias->length = length;

	platform.copy_memory(dst_key, key_begin, key_length);
	dst_key[key_length] = 0;
	platform.copy_memory(dst_value, value_begin, value_length);
	dst_value[value_length] = 0;

	++idx_dim->alias_count;

	return 1;
}

static s32
nvs_Schema_push_index_dimension(nvs_Schema *self, char *name, u8 bits_per_level, u8 num_levels, u8 hint)
{
	Assert(self->num_measure_dimensions==0);

	nvs_CheckPoint cp = nvs_Schema_checkpoint(self);

	s32 name_right_offset = nvs_Schema_push_cstr(self, name);

	if (!name_right_offset) {
		nvs_Schema_restore(self, cp);
		return 0;
	}

	nvs_IndexDimension *dim = nvs_Schema_push_index_dimension_(self);
	if (!dim) {
		nvs_Schema_restore(self, cp);
		return 0;
	}

	*dim = (nvs_IndexDimension) {
		.name = name_right_offset,
		.bits_per_level = bits_per_level,
		.num_levels = num_levels,
		.hint = hint,
		.alias_offset = 0,
		.alias_count = 0
	};

	return 1;
}

static s32
nvs_Schema_push_measure_dimension(nvs_Schema *self, char *name, u8 storage)
{
	nvs_CheckPoint cp = nvs_Schema_checkpoint(self);

	s32 name_right_offset = nvs_Schema_push_cstr(self, name);

	if (!name_right_offset) {
		nvs_Schema_restore(self, cp);
		return 0;
	}

	nvs_MeasureDimension *dim = nvs_Schema_push_measure_dimension_(self);
	if (!dim) {
		nvs_Schema_restore(self, cp);
		return 0;
	}

	*dim = (nvs_MeasureDimension) {
		.name = name_right_offset,
		.storage = storage
	};

	return 1;
}

static nvs_Schema*
nvs_init_schema(void *buffer, s32 size, char *name, s32 name_len)
{
	Assert(size > 0);
	Assert(size == RAlign(size,8));
	Assert(size > sizeof(nvs_Schema));
	nvs_Schema *schema = buffer;
	*schema = (nvs_Schema) {
		.num_index_dimensions = 0,
		.num_measure_dimensions = 0,
		.name = 0,
		.left = sizeof(nvs_Schema),
		.right = size,
		.size = size,
		.flags = 0
	};

	s32 name_index = nvs_Schema_push_str(schema, name, name_len);
	if (!name_index) {
		return 0;
	}
	schema->name = name_index;
	return schema;
}
// should call this function with a size that fits
static void
nvs_pack(nvs_Schema *self, s32 new_size)
{
	// pre-conditions
	Assert(new_size == RAlign(new_size,8));
	Assert(new_size < self->size);
	Assert(new_size >= self->size - (self->right - self->left));

	s32 right_data_length = self->size - self->right;
	s32 new_right = new_size - right_data_length;

	void *dst = OffsetedPointer(self,new_right);
	void *src = OffsetedPointer(self,self->right);
	platform.copy_memory(dst, src, right_data_length);

	self->right = new_right;
	self->size  = new_size;
}

static s32
nvs_smallest_size(nvs_Schema *self)
{
	s32 new_size = self->size - (self->right - self->left);
	Assert((new_size%8)==0);
	return new_size;
}

// static nvs_Schema*
// nvs_new_schema(s32 size, char *name)
// {
// 	Assert(size > 0);
// 	Assert(size == RALIGN(size,Kilobytes(4)));
// 	void *buffer = platform.allocate_memory_raw(size,0);
// 	nvs_Schema *schema = nvs_init_schema(buffer, size, name);
// 	if (!schema) {
// 		platform.free_memory_raw(schema);
// 		return 0;
// 	}
// 	return schema;
// }

// // should call this function with a size that fits
// static nvs_Schema*
// nvs_resize(nvs_Schema *self, s32 new_size)
// {
// 	// pre-conditions:
// 	// (1) new_size is a multiple of 8
// 	Assert(new_size == RALIGN(new_size,8));
// 	// (2) it fits the self content
// 	Assert(new_size >= self->size - (self->right - self->left));
//
// 	// copy left and right
// 	void *buffer = platform.allocate_memory_raw(new_size,0);
// 	platform.memory_copy(buffer, self, self->left);
//
// 	nvs_Schema copy = buffer;
// 	s32 right_data_length = self->size - self->right;
// 	copy->size  = new_size;
// 	copy->right = new_size - right_data_length;
// 	platform.memory_copy(RightOffsetedPointer(copy,copy->size,copy->right),
// 			     RightOffsetedPointer(self,self->size,self->right),
// 			     right_data_length);
//
// 	platform.free_memory_raw(self);
//
// 	return copy;
// }



