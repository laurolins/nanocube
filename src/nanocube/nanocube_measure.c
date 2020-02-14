/*
 * nm_ nanocube measure
 */

typedef enum {
	nm_TARGET_ROOT,
	nm_TARGET_FIND_DIVE,
	nm_TARGET_MASK,
	/* loop, no anchor */
	nm_TARGET_INTERVAL_SEQUENCE,
	/* no loop, no anchor */
	nm_TARGET_INTERVAL_SEQUENCE_AGGREGATE,
	/* loop, no anchor */
	nm_TARGET_TIME_SERIES,
	/* no loop, no anchor */
	nm_TARGET_TIME_SERIES_AGGREGATE,
	nm_TARGET_PATH_LIST,
	nm_TARGET_FIND_DIVE_LIST,
	nm_TARGET_TILE2D_RANGE,
	nm_TARGET_MONTH_SERIES
} nm_TargetType;

typedef struct {
	s32         by_alias; // if by name, consider alias, otherwise consider array
	nx_Array    array;
	MemoryBlock alias;
} nm_Path;

typedef struct {
	nm_Path  path;
	u8       depth;
} nm_Dive;

typedef struct {
	tm_Time base;
	u64     width;
	u64     count;
	s64     stride;
	s64     cumulative; // fix the start point and move only the end point
} nm_TimeSequence;

typedef struct {
	nm_TargetType type;
	b8            anchor;
	b8            loop;
	union {
		nm_Dive find_dive;
		struct {
			nm_Dive *begin;
			nm_Dive *end;
		} find_dive_list;
		struct {
			nm_Path* *begin;
			nm_Path* *end;
		} path_list;
		MemoryBlock mask;
		struct {
			s64 base;
			u64 width;
			u64 count;
			s64 stride;
			u8  depth; // base, width, count and stride are numbers with max_depth digits (accirding base)
		} interval_sequence;
		nm_TimeSequence time_sequence;
		struct {
			u32 z;
			u32 x0;
			u32 y0;
			u32 x1;
			u32 y1;
		} tile2d_range;
	};
} nm_Target;

/*
 * A time binning representation that is associated with
 * queries of the type nm_TARGET_TIME_SERIES
 */
typedef struct {
	tm_Time base_time;
	/* minutes */
	s64     default_minute_offset;
	/* seconds */
	u64     bin_width;
} nm_TimeBinning;


#define nm_NUMERICAL_TO_INT_METHOD_TRUNCATE 0
#define nm_NUMERICAL_TO_INT_METHOD_FLOOR    1
#define nm_NUMERICAL_TO_INT_METHOD_CEIL     2
#define nm_NUMERICAL_TO_INT_METHOD_ROUND    3

/*
 * Numericl specification of the type
 *     A x + b with additional to int method
 */
typedef struct {
	f64 a;
	f64 b;
	s64 to_int_method;
} nm_Numerical;

/*
 * a flag that will be used to write the column value as text
 * instead of the path sequence of names
 */
#define nm_BINDING_HINT_NONE    0x0
#define nm_BINDING_HINT_IMG2D   0x1
#define nm_BINDING_HINT_TILE2D  0x2
#define nm_BINDING_HINT_NAME    0x3
#define nm_BINDING_HINT_TIME    0x4


typedef struct {
	s32 hint_id;
	//
	// hint number is used on hint_id:
	//	nm_BINDING_HINT_IMG2D
	//	nm_BINDING_HINT_TILE2D
	//
	s32 number;
	//
	// time sequence is used when timeseries or monthseries
	// columns are used
	//
	s16 has_time_sequence;
	s16 time_sequence_is_monthly;
	//
	// otherwise it should be reported as bins
	//
	nm_TimeSequence time_sequence;
} nm_BindingHint;

/*
 * This is the result of parsing 'b' function
 *
 * Note that it uses dimension name
 */
typedef struct nm_Binding {
	MemoryBlock       dimension_name;
	nm_Target         *target;
	nm_BindingHint    hint;
	struct nm_Binding *next;
} nm_Binding;


#define nm_MeasureSource_MAX_NANOCUBES_PER_SOURCE 1024

typedef struct {

	char *dim_name;
	s32 dim_levels;
	s32 dim_name_length;

	nx_NanocubeIndex *src_index;
	void *src_nanocube; // wrapper of the index

} nm_MeasureSourceItem;

typedef struct {
	u32 num_nanocubes;
	u32 num_dimensions; // index dimensions
	u32 left;
	u32 right; // if it is a pattern based source,
	           // write the pattern at the end
	u32 length;

	// each item slot has two parts that might or
	// might not be used dim and nanocube
	nm_MeasureSourceItem items[];
} nm_MeasureSource;

static s32
nm_measure_source_storage_needs(s32 num_dimensions, s32 num_nanocubes)
{
	return sizeof(nm_MeasureSource) + Max(num_dimensions,num_nanocubes) * sizeof(nm_MeasureSourceItem);
}

static nx_NanocubeIndex*
nm_measure_source_index(nm_MeasureSource* self, s32 index)
{
	Assert(index < self->num_nanocubes);
	return self->items[index].src_index;
}

static void*
nm_measure_source_nanocube(nm_MeasureSource* self, s32 index)
{
	Assert(index < self->num_nanocubes);
	return self->items[index].src_nanocube;
}

static MemoryBlock
nm_measure_source_dim_name(nm_MeasureSource* self, s32 index)
{
	Assert(index < self->num_dimensions);
	char *begin = self->items[index].dim_name;
	char *end   = begin + self->items[index].dim_name_length;
	return (MemoryBlock) {
		.begin = begin,
		.end = end
	};
}

static u8
nm_measure_source_dim_levels(nm_MeasureSource* self, s32 index)
{
	Assert(index < self->num_dimensions);
	return self->items[index].dim_levels;
}

MemoryBlock
nm_measure_source_get_pattern(nm_MeasureSource* self)
{
	if (self->right < self->length) {
		return (MemoryBlock) {
			.begin = OffsetedPointer(self, self->right),
			.end = OffsetedPointer(self, self->length)
		};
	} else {
		return (MemoryBlock) { 0 };
	}
}

static s32
nm_measure_source_set_pattern(nm_MeasureSource* self, char *pattern, s32 pattern_length)
{
	if (self->left + pattern_length > self->length) {
		return 0;
	}
	self->right = self->left - pattern_length;
	platform.copy_memory(OffsetedPointer(self, self->right), pattern, pattern_length);
	return 1;
}

static nm_MeasureSource*
nm_measure_source_init(void *buffer, u32 length)
{
	Assert(length > sizeof(nm_MeasureSource));
	nm_MeasureSource *result = buffer;
	result[0] = (nm_MeasureSource) {
		.num_nanocubes = 0,
		.num_dimensions = 0,
		.left = sizeof(nm_MeasureSource),
		.right = length,
		.length = length
	};
	return result;
}

static s32
nm_measure_source_insert_dimension(nm_MeasureSource *self, char *name, s32 name_length, u8 levels)
{
	s32 index = self->num_dimensions;
	if (index >= self->num_nanocubes) {
		if (self->left + sizeof(nm_MeasureSourceItem) > self->length) {
			return 0;
		}
		self->left += sizeof(nm_MeasureSourceItem);
		self->items[index] = (nm_MeasureSourceItem) { 0 };
	}
	self->items[index].dim_levels = levels;
	self->items[index].dim_name   = name;
	self->items[index].dim_name_length = name_length;
	++self->num_dimensions;
	return 1;
}

static s32
nm_measure_source_insert_nanocube(nm_MeasureSource *self, nx_NanocubeIndex *index, void *nanocube)
{
	s32 offset = self->num_nanocubes;
	if (offset >= self->num_dimensions) {
		if (self->left + sizeof(nm_MeasureSourceItem) > self->length) {
			return 0;
		}
		self->items[offset] = (nm_MeasureSourceItem) { 0 };
		self->left += sizeof(nm_MeasureSourceItem);
	}
	self->items[offset].src_index = index;
	self->items[offset].src_nanocube = nanocube;
	++self->num_nanocubes;
	return 1;
}

/*
 * This is a measure source binding.
 */
typedef struct nm_MeasureSourceBinding {
	u8                             dimension;
	nm_Target                      target;
	nm_BindingHint                 hint;
	struct nm_MeasureSourceBinding *next;
} nm_MeasureSourceBinding;

typedef enum {
	nm_MEASURE_EXPRESSION_OPERATION_BINARY_ADD,
	nm_MEASURE_EXPRESSION_OPERATION_BINARY_SUB,
	nm_MEASURE_EXPRESSION_OPERATION_BINARY_DIV,
	nm_MEASURE_EXPRESSION_OPERATION_BINARY_MUL,
	nm_MEASURE_EXPRESSION_OPERATION_UNARY_MINUS
} nm_MeasureExpression_Operation_Type;

typedef struct nm_MeasureExpression {
	b8 is_source:1;
	b8 is_number:1;
	b8 is_binary_op:1;
	b8 is_unary_op: 1;
	union {
		u32    source_index;
		double number;
		struct {
			nm_MeasureExpression_Operation_Type type;
			struct nm_MeasureExpression *left;
			struct nm_MeasureExpression *right;
		} op; // if unary op, only left should be set
	};
} nm_MeasureExpression;

#define nm_Measure_Max_Sources 64

/*
 *
 * An equivalence class notion for the opaque payload
 * config needs to be available:
 *
 * taxirides.select("fare")/taxirides.select("count")
 *
 */
typedef struct {
	u8                       num_sources;
	nm_MeasureSource         *sources[nm_Measure_Max_Sources]; // max of 64 sources
	nm_MeasureSourceBinding  *bindings[nm_Measure_Max_Sources];
	u8                       num_bindings[nm_Measure_Max_Sources];
	nm_MeasureExpression     *expression;
	BilinearAllocator        *memory;
	void                     *payload_config[nm_Measure_Max_Sources];
} nm_Measure;

//------------------------------------------------------------------------------
// TableKeys, TableKeysType,
//------------------------------------------------------------------------------

typedef struct {
	MemoryBlock    name;
	u8             levels;
	u8             bits;
	b8             loop_column;
	nm_BindingHint hint;
	// b8          flags;
} nm_TableKeysColumnType;

typedef struct {
	nm_TableKeysColumnType *begin;
	nm_TableKeysColumnType *end;
} nm_TableKeysType;

typedef struct {
	nm_TableKeysType *type;
	u32            rows;
	u32            row_length;
	u32            current_offset;
	u8             columns;
	u8             current_column;
	struct {
		char *begin;
		char *end;
	} keys;
	LinearAllocator *memsrc; // should be exclusive while not consolidated
} nm_TableKeys;

typedef struct {
	u32 *begin;
	u32 *end;
} nm_Permutation;

//------------------------------------------------------------------------------
// TableValues
//------------------------------------------------------------------------------

typedef void* nm_TableValuesHandle;

/*
 * TODO(llins): revise this two opaque objects interface;
 */
// allocation_context can be used as a reference to allocator infra-structure
#define nm_TABLE_VALUES_CREATE(name) nm_TableValuesHandle name(void *nanocube, void *payload_config, void *allocation_context)
typedef nm_TABLE_VALUES_CREATE(nm_TableValuesCreate);

#define nm_TABLE_VALUES_COPY(name) nm_TableValuesHandle name(nm_TableValuesHandle handle, nm_Permutation* permutation, void *allocation_context)
typedef nm_TABLE_VALUES_COPY(nm_TableValuesCopy);

#define nm_TABLE_VALUES_COPY_FORMAT(name) nm_TableValuesHandle name(nm_TableValuesHandle handle, void *allocation_context)
typedef nm_TABLE_VALUES_COPY_FORMAT(nm_TableValuesCopyFormat);

#define nm_TABLE_VALUES_APPEND(name) s32 name(nm_TableValuesHandle handle, nx_PayloadAggregate *data, void *nanocube)
typedef nm_TABLE_VALUES_APPEND(nm_TableValuesAppend);

#define nm_TABLE_VALUES_APPEND_COPYING(name) s32 name(nm_TableValuesHandle dst_handle, nm_TableValuesHandle src_handle, u32 src_index)
typedef nm_TABLE_VALUES_APPEND_COPYING(nm_TableValuesAppendCopying);

#define nm_TABLE_VALUES_PACK(name) void name(nm_TableValuesHandle handle, nm_Permutation* permutation, MemoryBlock repeat_flags)
typedef nm_TABLE_VALUES_PACK(nm_TableValuesPack);

#define nm_TABLE_VALUES_COMBINE_ENTRY(name) \
	void name(nm_TableValuesHandle entry_handle, u32 entry_index, \
		  nm_TableValuesHandle with_handle, u32 with_index, \
		  nm_MeasureExpression_Operation_Type op, b8 entry_on_right)
typedef nm_TABLE_VALUES_COMBINE_ENTRY(nm_TableValuesCombineEntry);

#define nm_TABLE_VALUES_CANT_COMBINE_ENTRY(name) \
	void name(nm_TableValuesHandle handle, u32 index, \
		  nm_MeasureExpression_Operation_Type op, b8 value_on_right)
typedef nm_TABLE_VALUES_CANT_COMBINE_ENTRY(nm_TableValuesCantCombineEntry);

#define nm_TABLE_VALUES_COMBINE_NUMBER(name) \
	void name(nm_TableValuesHandle handle, f64 number, \
		  nm_MeasureExpression_Operation_Type op, b8 number_on_right)
typedef nm_TABLE_VALUES_COMBINE_NUMBER(nm_TableValuesCombineNumber);

/*
 * function to retrieve the time binning info associated with a dimension
 * in the custom nanocube
 */
#define nm_GET_TIME_BINNING(name) b8 name(void *nanocube, u8 dimension, nm_TimeBinning *output)
typedef nm_GET_TIME_BINNING(nm_GetTimeBinning);


/*
 * function to retrieve path based on alias
 */
#define nm_GET_ALIAS_PATH(name) b8 name(void *nanocube, u8 dimension, MemoryBlock alias, s32 output_buffer_capacity, u8 *output_buffer, s32* output_length)
typedef nm_GET_ALIAS_PATH(nm_GetAliasPath);

//
// when evaluating a measure, TableValues is configurable
// by the specific application.
//

typedef struct {
	nm_TableValuesCreate           *create;
	nm_TableValuesAppend           *append;
	nm_TableValuesAppendCopying    *append_copying;
	nm_TableValuesCopy             *copy;
	nm_TableValuesCopyFormat       *copy_format;
	nm_TableValuesCombineEntry     *combine_entry;
	nm_TableValuesCantCombineEntry *cant_combine_entry;
	nm_TableValuesCombineNumber    *combine_number;
	nm_TableValuesPack             *pack;
	nm_GetTimeBinning              *get_time_binning;
	nm_GetAliasPath                *get_alias_path;
} nm_Services;

//------------------------------------------------------------------------------
// Table
//------------------------------------------------------------------------------

typedef struct {
	nm_TableKeys         table_keys;
	nm_TableValuesHandle table_values_handle;
	nm_MeasureSource     *source;
} nm_Table;

typedef union {
	struct {
		b8 is_different:1;
		b8 is_equal:1;
		b8 is_coarser:1;
		b8 is_finer:1;
	};
	b8 flags;
} nm_TableKeysColumnType_Relation;

// relation between table keys types
typedef nm_TableKeysColumnType_Relation TableKeys_Relation;

typedef union {
	struct {
		b8 is_ambiguous:1;
		b8 is_equal:1;
		b8 is_coarser:1;
		b8 is_finer:1;
		b8 is_unmatched:1;
		u8 corresponding_index;
	};
	u16 data;
} nm_TableKeysType_Column_Match;

typedef struct {
	nm_TableKeysType *t1;
	nm_TableKeysType *t2;

	nm_TableKeysColumnType_Relation relation;

	u8 c1;
	u8 c2;

	nm_TableKeysType_Column_Match *mapping1;
	nm_TableKeysType_Column_Match *mapping2;
} nm_TableKeysType_Alignment;

typedef struct {
	nm_Target              *target_begin;
	nm_Target              *target_end;
	nm_MeasureSource       *source;
	nm_TableKeys           *table_keys;
	nm_TableValuesHandle    table_values_handle;
	nm_Services            *nm_services;
} nm_MeasureSolveQueryData;

typedef struct {
	nm_Measure             *measure;
	LinearAllocator        *memsrc;
	nm_Services            *nm_services;

	nm_Table               *table_begin;
	nm_Table               *table_end;

	LinearAllocator        *allocation_context;
} nm_MeasureEvalContext;

typedef struct {
	union {
		struct {
			b8 is_table:1;
			b8 is_number:1;
			b8 error:1;
		};
		b8 flags;
	};
	union {
		nm_Table *table;
		f64 number;
	};
} nm_MeasureEvalExpressionResult;



/*
 * Data structure for finding and traversing nx hierarchies
 */

typedef struct {

	nx_NanocubeIndex *nci;
	nx_Node          *root;
	nx_Array         *find_path;
	s32              index;
	u8               target_depth;

	nx_List_u8       path;
	nx_Label         path_storage[nx_MAX_PATH_LENGTH];

	nx_NodeWrap      stack_nodes[nx_MAX_PATH_LENGTH];
	s32              stack_next_action[nx_MAX_PATH_LENGTH]; // if action is zero
	u8               stack_depth[nx_MAX_PATH_LENGTH];
	s32              stack_size;

} nm_NanocubeIndex_FindDive;

typedef struct {
	nx_NanocubeIndex  *nci;
	s32               index;
	u8                target_depth;

	b8                done;
	b8                num_next_calls_is_zero;

	nx_Node           *root;

	MemoryBlock       mask;
	char              *it_mask;

	nx_List_u8        path;
	nx_Label          path_storage[nx_MAX_PATH_LENGTH];

	nx_NodeWrap       cursor_nodes[nx_MAX_PATH_LENGTH];
	u8                cursor_depths[nx_MAX_PATH_LENGTH];
	u8                cursor_offsets[nx_MAX_PATH_LENGTH];
	u8                cursor_cum_depth;
	s32               cursor_num_nodes;
} nm_Mask;

//
// This might streamline range 2d queries on quadtree dimensions.
// Mask is the more general, but this one is more efficient for
// this specific case.
//

#define nm_Tile2D_Range_Iterator_test_DISJOINT   0
#define nm_Tile2D_Range_Iterator_test_CONTAINED  1
#define nm_Tile2D_Range_Iterator_test_INTERSECTS 2

typedef struct {
	nx_NodeWrap node;
	u32         z;
	u32         x;
	u32         y;
	s32         action;
	s32         test; // result of the test when we pushed this into the stack
} nm_Tile2D_Range_Iterator_Item;

typedef struct {

	nx_NanocubeIndex *nci;
	nx_Node          *root;
	s32              index;

	u32              z;
	u32              x0;
	u32              y0;
	u32              x1;
	u32              y1;

	nx_List_u8       path;
	nx_Label         path_storage[nx_MAX_PATH_LENGTH];

	nm_Tile2D_Range_Iterator_Item stack_items[nx_MAX_PATH_LENGTH];
	s32                           stack_size;

} nm_Tile2D_Range_Iterator;

//------------------------------------------------------------------------------
// error codes
//------------------------------------------------------------------------------

#define nm_OK 0
#define nm_ERROR_COULDNT_DERIVE_KEY_TYPES_FROM_MEASURE_SOURCE_AND_BINDINGS 1
#define nm_ERROR_PAYLOAD_OUT_OF_MEMORY 2
#define nm_ERROR_TIME_BIN_ALIGNMENT 3
#define nm_ERROR_INVALID_ALIAS 4
#define nm_ERROR_TYPE_INFERENCE_DIFFERENT_DIVE_DEPTHS 5
#define nm_ERROR_TYPE_INFERENCE_INVALID_ALIAS 6
#define nm_ERROR_INDEX_COLUMNS_OUT_OF_MEMORY 7

static char *nm_error_messages[] = {
	"nm_OK",
	"nm_ERROR_COULDNT_DERIVE_KEY_TYPES_FROM_MEASURE_SOURCE_AND_BINDINGS",
	"nm_ERROR_PAYLOAD_OUT_OF_MEMORY",
	"nm_ERROR_TIME_BIN_ALIGNMENT",
	"nm_ERROR_INVALID_ALIAS",
	"nm_ERROR_TYPE_INFERENCE_DIFFERENT_DIVE_DEPTHS",
	"nm_ERROR_TYPE_INFERENCE_INVALID_ALIAS",
	"nm_ERROR_INDEX_COLUMNS_OUT_OF_MEMORY"
};

//------------------------------------------------------------------------------
// nm_Path
//------------------------------------------------------------------------------

static b8
nm_Path_is_equal(nm_Path *self, nm_Path *other)
{
	if (self->by_alias != other->by_alias)
		return 0;
	if (self->by_alias) {
		return cstr_compare_memory(self->alias.begin, self->alias.end, other->alias.begin, other->alias.end) == 0;;
	} else {
		return nx_Array_is_equal(&self->array, &other->array);
	}
}


//------------------------------------------------------------------------------
// nm_Tile2D_Range_Iterator
//------------------------------------------------------------------------------

static s32
nm_Tile2D_Range_Iterator_test(nm_Tile2D_Range_Iterator *self, u32 z, u32 x, u32 y)
{
	if (z < self->z) {
		u32 gap = self->z - z;
		u32 xx0  = (x << gap);
		u32 yy0  = (y << gap);
		u32 xx1  = ((x + 1) << gap) - 1;
		u32 yy1  = ((y + 1) << gap) - 1;
		if (self->x1 < xx0 || self->y1 < yy0 || xx1 < self->x0 || yy1 < self->y0) {
			return nm_Tile2D_Range_Iterator_test_DISJOINT;
		} else if (self->x0 <= xx0 && xx1 <= self->x1 && self->y0 <= yy0 && yy1 <= self->y1) {
			return nm_Tile2D_Range_Iterator_test_CONTAINED;
		} else {
			return nm_Tile2D_Range_Iterator_test_INTERSECTS;
		}
	} else {
		u32 gap  = z - self->z;
		u32 xx0  = (x >> gap);
		u32 yy0  = (y >> gap);
		u32 xx1  = xx0;
		u32 yy1  = yy0;
		if (self->x1 < xx0 || self->y1 < yy0 || xx1 < self->x0 || yy1 < self->y0) {
			return nm_Tile2D_Range_Iterator_test_DISJOINT;
		} else {
			return nm_Tile2D_Range_Iterator_test_CONTAINED;
		}
	}
}

static void
nm_Tile2D_Range_Iterator_init(nm_Tile2D_Range_Iterator* self, nx_NanocubeIndex* nci, nx_Node* root, u32 index, u32 z, u32 x0, u32 x1, u32 y0, u32 y1)
{
	self[0] = (nm_Tile2D_Range_Iterator) { 0 };

	self->nci   = nci;
	self->root  = root;
	self->index = index;

	self->z  = z;
	self->x0 = x0;
	self->y0 = y0;
        self->x1 = x1;
	self->y1 = y1;

	nx_List_u8_init(&self->path,
			self->path_storage,
			self->path_storage,
			self->path_storage + nx_MAX_PATH_LENGTH);

	// check if root is disjoint from the 2d range
	// if so don't stack it, otherwise it is the root
	// of the stack

	nx_Node       *child              = root;
	nx_NodeWrap   child_w             = nx_NanocubeIndex_to_node(nci, child, index);
	u8            child_suffix_length = child->path_length;

	u32 x = 0;
	u32 y = 0;
	u32 zz = 0;
	for (u8 i=0; i<child->path_length; ++i) {
		nx_Label label = nx_Path_get(&child_w.path, i);

		// print a warning
		if (label > 3) fprintf(stderr, "[Warning] nm_Tile2D_Range_Iterator dealing with a non-quadtree hierarchy: result is invalid\n");
		x = (x << 1) + ((label & 1) ? 1 : 0);
		y = (y << 1) + ((label & 2) ? 1 : 0);
		++zz;
		nx_List_u8_push_back(&self->path, label);
	}

	// check if there is any intersection between the root
	// node and the target range

	s32 test_result = nm_Tile2D_Range_Iterator_test(self, zz, x, y);
	self->stack_items[0] = (nm_Tile2D_Range_Iterator_Item) { .node = child_w, .z = zz, .x = x, .y = y, .action = 0, .test = test_result };
	++self->stack_size;
}

static nx_Node*
nm_Tile2D_Range_Iterator_next(nm_Tile2D_Range_Iterator *self)
{
	while (self->stack_size > 0) {
		nm_Tile2D_Range_Iterator_Item *item = self->stack_items + self->stack_size - 1;
		if (item->action == 0) {
			if (item->test == nm_Tile2D_Range_Iterator_test_CONTAINED) {
				--self->stack_size;
				return item->node.raw_node;
			} else if (item->test == nm_Tile2D_Range_Iterator_test_INTERSECTS) {
				// needs to test its children
				if (item->node.children.length > 0) {
					nx_Child    *child_slot = nx_Children_get_child(&item->node.children, 0);
					nx_Node     *child      = nx_Child_get_node(child_slot);
					nx_NodeWrap  child_w    = nx_NanocubeIndex_to_node(self->nci, child, self->index);

					// make sure the path is truncated to the parent
					// depth before inserting new labels into it
					self->path.end = self->path.begin + item->z;
					Assert(self->path.end <= self->path.capacity);

					u8 child_offset = child->path_length - child_slot->suffix_length;

					u32 x = item->x;
					u32 y = item->y;
					u32 z = item->z;
					for (u8 i=child_offset; i<child->path_length; ++i) {
						nx_Label label = nx_Path_get(&child_w.path, i);
						if (label > 3) fprintf(stderr, "[Warning] nm_Tile2D_Range_Iterator dealing with a non-quadtree hierarchy: result is invalid\n");
						x = (x << 1) + ((label & 1) ? 1 : 0);
						y = (y << 1) + ((label & 2) ? 1 : 0);
						++z;
						nx_List_u8_push_back(&self->path, label);
					}

					s32 test_result = nm_Tile2D_Range_Iterator_test(self, z, x, y);
					++item->action;
					self->stack_items[self->stack_size] = (nm_Tile2D_Range_Iterator_Item) { .node = child_w, .z = z, .x = x, .y = y, .action = 0, .test = test_result };
					++self->stack_size;
				} else {
					--self->stack_size;
				}
			} else {
				--self->stack_size;
			}
		} else if (item->action < item->node.children.length) {
			nx_Child    *child_slot = nx_Children_get_child(&item->node.children, item->action);
			nx_Node     *child      = nx_Child_get_node(child_slot);
			nx_NodeWrap  child_w    = nx_NanocubeIndex_to_node(self->nci, child, self->index);

			// make sure the path is truncated to the parent
			// depth before inserting new labels into it
			self->path.end = self->path.begin + item->z;
			Assert(self->path.end <= self->path.capacity);

			u8 child_offset = child->path_length - child_slot->suffix_length;

			u32 x = item->x;
			u32 y = item->y;
			u32 z = item->z;
			for (u8 i=child_offset; i<child->path_length; ++i) {
				nx_Label label = nx_Path_get(&child_w.path, i);
				if (label > 3) fprintf(stderr, "[Warning] nm_Tile2D_Range_Iterator dealing with a non-quadtree hierarchy: result is invalid\n");
				x = (x << 1) + ((label & 1) ? 1 : 0);
				y = (y << 1) + ((label & 2) ? 1 : 0);
				++z;
				nx_List_u8_push_back(&self->path, label);
			}

			s32 test_result = nm_Tile2D_Range_Iterator_test(self, z, x, y);
			++item->action;
			self->stack_items[self->stack_size] = (nm_Tile2D_Range_Iterator_Item) { .node = child_w, .z = z, .x = x, .y = y, .action = 0, .test = test_result };
			++self->stack_size;
		} else {
			--self->stack_size;
		}
	}
	return 0;
}

//------------------------------------------------------------------------------
// find and dive iterator
//------------------------------------------------------------------------------

static void
nm_NanocubeIndex_FindDive_init(nm_NanocubeIndex_FindDive* self, nx_NanocubeIndex* nci, nx_Node* root, s32 index, nx_Array *find_path, u8 dive_depth)
{
	self->nci          = nci;
	self->root         = root;
	self->index        = index;
	self->find_path    = find_path;
	self->target_depth = find_path->length + dive_depth;

	nx_List_u8_init(&self->path,
			self->path_storage,
			self->path_storage,
			self->path_storage + nx_MAX_PATH_LENGTH);

	nx_Node       *child              = root;
	nx_NodeWrap   child_w             = nx_NanocubeIndex_to_node(nci, child, index);
	u8            child_suffix_length = child->path_length;
	u8 depth               = 0; // this is equiv to self->path length
	b8            valid               = 1;
	// find first node
	for (;;) {
		// check common prefix length
		u8       child_offset = child->path_length - child_suffix_length;

		for (u8 i=child_offset; i<child->path_length; ++i) {
			nx_Label existing_label  = nx_Path_get(&child_w.path, i);
			if (depth < find_path->length) {
				valid = existing_label == nx_Array_get(find_path, depth);
			}
			if (valid) {
				++depth;
				nx_List_u8_push_back(&self->path, existing_label);
			} else {
				break; // empty iterator!
			}
		}

		if (!valid)
			break;

		if (depth < find_path->length) {
			nx_Label input_next_label = nx_Array_get(find_path, depth);
			s32 pos = nx_Children_find(&child_w.children, input_next_label);
			if (pos >= 0) {
				nx_Child* child_slot = nx_Children_get_child(&child_w.children, pos);
				child                = nx_Child_get_node(child_slot); // update child
				child_suffix_length  = child_slot->suffix_length;
				child_w              = nx_NanocubeIndex_to_node(nci, child, index);
				// a child w_node can be followed (if shared break)
			} else {
				valid = 0;
				break;
			}
			// consumes labels of current w_node; more labels to go;
		} else {
			break;
		} // split on current w_node's labels (done)

	}

	if (valid) {
		self->stack_nodes[0] = child_w;
		self->stack_next_action[0] = 0;
		self->stack_depth[0] = depth;
		self->stack_size = 1;
	} else {
		self->stack_size = 0;
	}

}

static nx_Node*
nm_NanocubeIndex_FindDive_next(nm_NanocubeIndex_FindDive* self)
{
	while (self->stack_size > 0) {
		s32 top    = self->stack_size-1;
		s32 action = self->stack_next_action[top];
		if (action == 0) {
			// first time vistiting this node
			// is it deeper than the current
			nx_NodeWrap *top_node_w = &self->stack_nodes[top];
			if (self->stack_depth[top] >= self->target_depth) {
				// done: found place
				nx_Node *next = top_node_w->raw_node;
				--self->stack_size;
				return next;
			} else if (top_node_w->children.length > 0) {
				// stack first child of top node
				// and schedule second child
				++self->stack_next_action[top];

				nx_Child    *child_slot = nx_Children_get_child(&top_node_w->children, 0);
				nx_Node     *child      = nx_Child_get_node(child_slot);
				nx_NodeWrap  child_w    = nx_NanocubeIndex_to_node(self->nci, child, self->index);

				// make sure the path is truncated to the parent
				// depth before inserting new labels into it
				self->path.end = self->path.begin + self->stack_depth[top];
				Assert(self->path.end <= self->path.capacity);

				++top;
				++self->stack_size;
				u8 child_offset = child->path_length - child_slot->suffix_length;
				for (u8 i=child_offset; i<child->path_length; ++i) {
					nx_List_u8_push_back(&self->path, nx_Path_get(&child_w.path, i));
				}
				self->stack_next_action[top] = 0;
				self->stack_depth[top] = self->stack_depth[top-1] + child_slot->suffix_length;
				self->stack_nodes[top] = child_w;
			} else {
				--self->stack_size;
			}

		} else { // action > 0 (at least one child was pushed and processed)
			nx_NodeWrap *top_node_w = &self->stack_nodes[top];
			if (self->stack_depth[top] >= self->target_depth) {
				// done: found place
				nx_Node *next = top_node_w->raw_node;
				--self->stack_size;
				return next;
			} else if (action < top_node_w->children.length) {
				// stack first child of top node
				// and schedule second child
				++self->stack_next_action[top];
				nx_Child    *child_slot = nx_Children_get_child(&top_node_w->children, (u8) action);
				nx_Node     *child      = nx_Child_get_node(child_slot);
				nx_NodeWrap  child_w    = nx_NanocubeIndex_to_node(self->nci, child, self->index);

				// make sure the path is truncated to the parent
				// depth before inserting new labels into it
				self->path.end = self->path.begin + self->stack_depth[top];
				Assert(self->path.end <= self->path.capacity);

				++top;
				++self->stack_size;
				u8 child_offset = child->path_length - child_slot->suffix_length;
				for (u8 i=child_offset; i<child->path_length; ++i) {
					nx_List_u8_push_back(&self->path, nx_Path_get(&child_w.path, i));
				}
				self->stack_next_action[top] = 0;
				self->stack_depth[top] = self->stack_depth[top-1] + child_slot->suffix_length;
				self->stack_nodes[top] = child_w;
			} else {
				--self->stack_size;
			}
		}
	}
	return 0;
}


//
// Mask:
//
// A list of paths in a push-pop representation.
//
// 1. A path marker is a position containig a "pop" symbol
//    that is either the first position or the previous position
//    is a label symbol.
//
// 2. The path associated to a path marker is the cumulative
//    result of pushes and pops starting from an empty string.
//
// Examples:
//
// (E1) An empty string has no path marker
// (E2) The string "<" has one path marker with the empty path
//      associated.
//

static inline b8
nm_Mask_mask_next_path_marker(nm_Mask *self)
{
	while (self->it_mask != self->mask.end && *self->it_mask != '<') {
		nx_List_u8_push_back(&self->path, (u8) (*self->it_mask - '0'));
		++self->it_mask;
	}
	return self->it_mask != self->mask.end;
}

static inline void
nm_Mask_cursor_pop_back(nm_Mask *self)
{
	Assert(self->cursor_num_nodes > 0);

	u8 i = (u8) (self->cursor_num_nodes - 1);

	if (self->cursor_depths[i] > 1 || i==0) {
		--self->cursor_depths[i];
		--self->cursor_cum_depth;
	} else {
		// cursor_depth[i] == 1
		self->cursor_depths[i] = 0;
		--self->cursor_num_nodes;
		--self->cursor_cum_depth;
	}
}

static inline b8
nm_Mask_cursor_advance(nm_Mask *self, nx_Label lbl)
{
	Assert(self->cursor_num_nodes > 0);
	u8 i = (u8) (self->cursor_num_nodes - 1);

	nx_NodeWrap  *node          = self->cursor_nodes + i;
	u8 offset        = self->cursor_offsets[i];
	u8 current_depth = self->cursor_depths[i];
	u8 max_depth     = node->path.length - offset;

	/* consume up to the last label of the current node */
	if (current_depth < max_depth) {
		// check if there is a character match
		if (lbl == nx_Path_get(&node->path, offset + current_depth)) {
			++self->cursor_depths[i];
			++self->cursor_cum_depth;
			return 1;
		}
	} else {
		// check if there is a children node
		// that starts with the correct label
		s32 child_index= nx_Children_find(&node->children, lbl);
		if (child_index >= 0) {
			nx_Child* child = nx_Children_get_child(&node->children, child_index);
			nx_Node* child_node = nx_Child_get_node(child);
			self->cursor_nodes[i+1] = nx_NanocubeIndex_to_node(self->nci, child_node, self->index);
			if (child->shared) {
				/* sanity check: path label at position offset matches lbl */
				u8 offset = (u8) (child_node->path_length - child->suffix_length);
				Assert(nx_Path_get(&self->cursor_nodes[i+1].path, offset) == lbl);
				self->cursor_offsets[i+1] = offset;
			} else {
				self->cursor_offsets[i+1] = 0;
			}
			self->cursor_depths[i+1] = 1;
			++self->cursor_cum_depth;
			++self->cursor_num_nodes;
			return 1;
		}
	}
	return 0;
}

//
// Assume mask iterator is in a path marker
// Rewind until the current implicit path
// has length len
//
static inline b8
nm_Mask_mask_rewind(nm_Mask *self, u8 len)
{
	Assert(self->it_mask != self->mask.end);
	Assert(*self->it_mask == '<');
	u8 current_len = (u8) nx_List_u8_size(&self->path);

	Assert(len < current_len);

	while (current_len > len && self->it_mask != self->mask.end) {
		if (*self->it_mask == '<') {
			--current_len;
			++self->it_mask;
		} else {
			++current_len;
			++self->it_mask;
		}
	}
	self->path.end = self->path.begin + current_len;

	return current_len == len;
}

//
// once we pop the root, mask should be done.
//
static inline b8
nm_Mask_sync_rewind(nm_Mask *self)
{
	// cursor and current path are in sync
	while (self->it_mask != self->mask.end && *self->it_mask == '<') {
		if (nx_List_u8_size(&self->path) == 0)
			return 0;

		nx_List_u8_pop_back(&self->path);
		nm_Mask_cursor_pop_back(self);

		++self->it_mask;
	}
	return 1;
}

static b8
nm_Mask_rewind_from_leaf(nm_Mask *self)
{
	//
	// Assuming it_mask is at a path marker position
	// (ie mask.end or '<' right after a non '<')
	//
	if (self->it_mask == self->mask.end) return 0;

	while (self->it_mask != self->mask.end && *self->it_mask != '<')
	{
		nx_List_u8_push_back(&self->path, *self->it_mask);
		++self->it_mask;
	}
	return 1;
}

static inline b8
nm_Mask_try_to_match_cursor_to_path(nm_Mask *self)
{
	//
	// cursor and path are in sync up to:
	//      self->cursor_cum_depth;
	// try to advance up to the current path length
	//
	Assert(self->cursor_num_nodes > 0);
	// u8 i = (u8) (self->cursor_num_nodes - 1);
	u8 len = (u8) nx_List_u8_size(&self->path);
	b8 ok = 1;
	while (self->cursor_cum_depth < len) {
		ok = nm_Mask_cursor_advance(self, nx_List_u8_get(&self->path, self->cursor_cum_depth));
		if (!ok) {
			break;
		}
	}
	return ok;
}

static void
nm_Mask_init(nm_Mask *self, nx_NanocubeIndex *nci, nx_Node *root,
			   s32 index, MemoryBlock mask)
{
	pt_fill((char*) self, (char*) self + sizeof(nm_Mask), 0);

	self->nci    = nci;
	self->root   = root;
	self->index  = index;
	self->mask   = mask;
	self->done   = 1;
	self->num_next_calls_is_zero = 1;

	self->it_mask = mask.begin;

	// go to first leaf and let it be ready
	nx_List_u8_init(&self->path, self->path_storage, self->path_storage, self->path_storage + nx_MAX_PATH_LENGTH);

	self->cursor_cum_depth  = 0;
	self->cursor_num_nodes = 0;
	if (!root) return;

	self->cursor_nodes[0]   = nx_NanocubeIndex_to_node(nci, root, index);
	self->cursor_offsets[0] = 0;
	self->cursor_depths[0]  = 0;
	self->cursor_num_nodes  = 1;
	self->done              = 0;
}



static nx_Node*
nm_Mask_next(nm_Mask *self)
{
	if (self->done) return 0;

	if (!self->num_next_calls_is_zero) {
		// sync rewind and continue
		// keep rewinding in sync while on a '<' symbol
		b8 sync_rewind_ok = nm_Mask_sync_rewind(self);

		if (!sync_rewind_ok) {
			self->done = 1;
			return 0;
		}
	}

	self->num_next_calls_is_zero = 0;

	while (1)
	{
		// cursor and path are in sync
		b8 path_available = nm_Mask_mask_next_path_marker(self);

		if (!path_available) {
			self->done = 1;
			return 0;
		}

		b8 cursor_match = nm_Mask_try_to_match_cursor_to_path(self);

		if (cursor_match) {
			return self->cursor_nodes[self->cursor_num_nodes-1].raw_node;
		} else {
			b8 mask_rewind_ok = nm_Mask_mask_rewind(self, self->cursor_cum_depth);
			if (mask_rewind_ok) {
				// keep rewinding in sync while on a '<' symbol
				b8 sync_rewind_ok = nm_Mask_sync_rewind(self);

				if (!sync_rewind_ok) {
					self->done = 1;
					return 0;
				}

			} else {
				self->done = 1;
				return 0;
			}
		}
	}
}



//------------------------------------------------------------------------------
// Interval Iterator
//------------------------------------------------------------------------------

typedef struct
{
	nx_NodeWrap  node;
	s32       child_index;
	u8        depth;
	u64       begin;
	u64       end;
}
NanocubeIndex_Interval_Item;

typedef struct
{
	nx_NanocubeIndex *nci;
	nx_Node          *root;

	u64              begin;
	u64              end;

	s32              index;
	s32              stack_size;

	u8               bits_per_label;
	u8               depth;

	NanocubeIndex_Interval_Item stack[nx_MAX_PATH_LENGTH];
}
NanocubeIndex_Interval;

static void
NanocubeIndex_Interval_init(NanocubeIndex_Interval *self, nx_NanocubeIndex* nci, nx_Node* root, s32 index, u8  depth, u64 begin, u64 end)
{
#if 1
	// index is the dimension in the cube
	self[0] = (NanocubeIndex_Interval) {
		.nci    = nci,
		.root   = root,
		.index  = index,
		.begin  = begin,
		.end    = end,
		.depth  = depth,
		.stack_size = 0,
		.bits_per_label = nci->bits_per_label[index]
	};

	if (!self->root)
		return;

	// check precision for now
	Assert(self->bits_per_label * self->depth < 64);

	nx_NodeWrap root_wrap  = nx_NanocubeIndex_to_node(self->nci, self->root, self->index);
	u8          root_depth = self->root->path_length;

	// compute sequential number for the root
	u64 child_begin_suffix = 0;
	for (u8 i=0;i<root_depth;++i) {
		nx_Label label = nx_Path_get(&root_wrap.path, i);
		child_begin_suffix = (child_begin_suffix << self->bits_per_label) + label;
	}

	// depth is the target depth
	u8 shift_left = (u8) ((self->depth - root_depth) * self->bits_per_label);

	// sequential range in numbers with (depth * bits_per_label) bits starting at root
	u64 root_range_begin = child_begin_suffix << shift_left;
	u64 root_range_end   = root_range_begin + (1ull << shift_left);

	// simulate pushing first element into stack
	NanocubeIndex_Interval_Item *item = self->stack;
	item[0] = (NanocubeIndex_Interval_Item) {
		.node = root_wrap,
		.child_index = -1,
		.depth = root_depth,
		.begin = root_range_begin,
		.end   = root_range_end
	};
	self->stack_size = 1;

#else

	pt_fill((char*) self, (char*) self + sizeof(NanocubeIndex_Interval), 0);

	self->nci    = nci;
	self->root   = root;
	self->index  = index;
	self->begin  = begin;
	self->end    = end;
	self->depth  = depth;

	self->bits_per_label = nci->bits_per_label[index];

	if (!self->root)
		return;

	self->stack_size = 1;

	NanocubeIndex_Interval_Item *item = self->stack;

	item->node = nx_NanocubeIndex_to_node(self->nci, self->root, self->index);
	item->child_index = -1; // first hit on a node

	item->depth       = self->root->path_length;

	u64 child_begin_suffix = 0;
	for (u8 i=0;i<item->depth;++i) {
		nx_Label label = nx_Path_get(&item->node.path, i);
		child_begin_suffix = (child_begin_suffix << self->bits_per_label) + label;
	}

	// check precision for now
	Assert(self->bits_per_label * self->depth < 64);

	u8 shift_left = (u8) ((self->depth - item->depth) * self->bits_per_label);

	item->begin       = item->begin + (child_begin_suffix << shift_left);
	item->end         = item->begin + (1ull << shift_left);
	item->child_index = -1;

	//
	//     // numerical range of root (needs to fit u64 precision)
	//     self->stack[0].begin = 0;
	//     self->stack[0].end   = 1ull << (self->bits_per_label * self->depth);
#endif
}

typedef union
{
	b8 data;
	struct {
		b8 contained : 1;
		b8 partial_hit : 1;
		b8 disjoint: 1;
	};
}
NanocubeIndex_Interval_Hit;

static NanocubeIndex_Interval_Hit
NanocubeIndex_Interval_hit_test(NanocubeIndex_Interval *self, u64 node_begin, u64 node_end)
{
	NanocubeIndex_Interval_Hit result;
	result.data = 0;
	if (self->begin >= node_end || self->end <= node_begin) {
		result.disjoint = 1;
	}
	else if (self->begin <= node_begin && node_end <= self->end) {
		result.contained = 1;
	}
	else {
		result.partial_hit = 1;
	}
	return result;
}

static nx_Node*
NanocubeIndex_Interval_next(NanocubeIndex_Interval *self)
{
	if (!self->stack_size) return 0;

	while (self->stack_size > 0) {
		NanocubeIndex_Interval_Item *item = self->stack + self->stack_size - 1;

		if (item->child_index == -1) {
			// check if node interval is totally contained
			// into the requested interval
			NanocubeIndex_Interval_Hit hit = NanocubeIndex_Interval_hit_test(self, item->begin, item->end);

			if (hit.contained) {
				// found next node
				--self->stack_size;
				return item->node.raw_node;
			}
			else if (hit.partial_hit) {
				if (item->node.children.length) {
					// push next child
					nx_Child *child      = nx_Children_get_child(&item->node.children, 0);
					nx_Node  *child_node = nx_Child_get_node(child);

					// how many labels to append?
					u8 offset = 0;
					if (child->shared)
					{
						offset = (u8) (child_node->path_length - child->suffix_length);
					}

					u8 delta_depth = (u8) (child_node->path_length - offset);

					NanocubeIndex_Interval_Item *child_item = self->stack + self->stack_size;

					child_item->node  = nx_NanocubeIndex_to_node(self->nci, child_node, self->index);

					u64 child_begin_suffix = 0;
					for (u8 i=0;i<delta_depth;++i)
					{
						nx_Label label = nx_Path_get(&child_item->node.path, offset + i);
						child_begin_suffix = (child_begin_suffix << self->bits_per_label) + label;
					}

					child_item->depth = item->depth + delta_depth;

					u8 shift_left = (u8) ((self->depth - child_item->depth) * self->bits_per_label);

					child_item->begin = item->begin + (child_begin_suffix << shift_left);
					child_item->end   = child_item->begin + (1ull << shift_left);
					child_item->child_index = -1;

					item->child_index = 1; // next child index

					if ( !NanocubeIndex_Interval_hit_test(self, child_item->begin,
									      child_item->end).disjoint )
					{
						++self->stack_size;
					}
				}
				else {
					// nothing through this path
					--self->stack_size;
				}
			}
			else {
				--self->stack_size;
			}
		}
		else {
			if (item->child_index < item->node.children.length) {
				// push next child
				nx_Child *child      = nx_Children_get_child(&item->node.children,
									     (u8) item->child_index);
				nx_Node  *child_node = nx_Child_get_node(child);

				// how many labels to append?
				u8 offset = 0;
				if (child->shared) {
					offset = (u8) (child_node->path_length - child->suffix_length);
				}

				u8 delta_depth = (u8) (child_node->path_length - offset);

				NanocubeIndex_Interval_Item *child_item = self->stack + self->stack_size;

				child_item->node  = nx_NanocubeIndex_to_node(self->nci, child_node, self->index);
				child_item->depth = item->depth + delta_depth;

				u64 child_begin_suffix = 0;
				for (u8 i=0;i<delta_depth;++i) {
					nx_Label label = nx_Path_get(&child_item->node.path, offset + i);
					child_begin_suffix = (child_begin_suffix << self->bits_per_label) + label;
				}

				child_item->depth = item->depth + delta_depth;

				u8 shift_left = ((self->depth - child_item->depth) * self->bits_per_label);

				child_item->begin = item->begin + (child_begin_suffix << shift_left);
				child_item->end   = child_item->begin + (1ull << shift_left);
				child_item->child_index = -1;

				++item->child_index;

				if ( !NanocubeIndex_Interval_hit_test(self, child_item->begin,
								      child_item->end).disjoint ) {
					++self->stack_size;
				}
			}
			else {
				--self->stack_size;
			}
		}
	}

	return 0;

}



/*
 * nm_TimeBinning
 */
static void
nm_TimeBinning_init(nm_TimeBinning *self, tm_Time base_time, s64 default_minute_offset, u64 bin_width)
{
	self->base_time = base_time;
	self->default_minute_offset = default_minute_offset;
	self->bin_width = bin_width;
}

//------------------------------------------------------------------------------
// nm_Target
//------------------------------------------------------------------------------

static void
nm_Target_init(nm_Target *self) {
	char *m = (char*) self;
	pt_fill(m, m + sizeof(nm_Target), 0);
	self->type = nm_TARGET_ROOT;
	self->anchor = 0;
	self->loop = 0;
}

static s32
nm_Dive_match(nm_Dive *a, nm_Dive *b)
{
	return (a->depth == b->depth) && nm_Path_is_equal(&a->path, &b->path);
}

static b8
nm_Target_is_equal(nm_Target *self, nm_Target *other)
{
	if (self->type != other->type
	    || self->anchor != other->anchor
	    || self->loop != other->loop)
		return 0;

	if (self->type == nm_TARGET_ROOT) {
		return 1;
	} else if (self->type == nm_TARGET_FIND_DIVE) {
		return nm_Dive_match(&self->find_dive, &other->find_dive);
	} else if (self->type == nm_TARGET_FIND_DIVE_LIST) {
		s64 n0 = self->find_dive_list.end - self->find_dive_list.begin;
		s64 n1 = other->find_dive_list.end - other->find_dive_list.begin;
		if (n0 != n1) {
			return 0;
		} else {
			for (s64 i=0;i<n0;++i) {
				if (!nm_Dive_match(other->find_dive_list.begin + i, other->find_dive_list.begin + i)) {
					return 0;
				}
			}
			return 1;
		}
	} else if (self->type == nm_TARGET_MASK) {
		if (self->mask.begin == other->mask.begin
		    && self->mask.end == other->mask.end) {
			return 1;
		}
		else if (self->mask.end - self->mask.begin == other->mask.end - other->mask.begin) {
			if (cstr_compare_memory (self->mask.begin, self->mask.end,
					       other->mask.begin, other->mask.end) == 0) {
				return 1;
			}
		}
	} else if (self->type == nm_TARGET_TILE2D_RANGE) {
		if (self->tile2d_range.z == other->tile2d_range.z &&
		    self->tile2d_range.x0 == other->tile2d_range.x0 &&
		    self->tile2d_range.x1 == other->tile2d_range.x1 &&
		    self->tile2d_range.y0 == other->tile2d_range.y0 &&
		    self->tile2d_range.y1 == other->tile2d_range.y1) {
			return 1;
		}
	} else if (self->type == nm_TARGET_INTERVAL_SEQUENCE || self->type == nm_TARGET_INTERVAL_SEQUENCE_AGGREGATE) {
		if ((self->interval_sequence.base == other->interval_sequence.base)
		    && (self->interval_sequence.width == other->interval_sequence.width)
		    && (self->interval_sequence.count == other->interval_sequence.count)
		    && (self->interval_sequence.stride == other->interval_sequence.stride)
		    && (self->interval_sequence.depth == other->interval_sequence.depth)) {
			return 1;
		}
	} else if (self->type == nm_TARGET_TIME_SERIES || self->type == nm_TARGET_TIME_SERIES_AGGREGATE) {
		if ((self->time_sequence.base.time == other->time_sequence.base.time)
		    && (self->time_sequence.width == other->time_sequence.width)
		    && (self->time_sequence.count == other->time_sequence.count)
		    && (self->time_sequence.stride == other->time_sequence.stride)
		    && (self->time_sequence.cumulative == other->time_sequence.cumulative)) {
			return 1;
		}
	} else if (self->type == nm_TARGET_MONTH_SERIES) {
		if ((self->time_sequence.base.time == other->time_sequence.base.time)
		    && (self->time_sequence.width == other->time_sequence.width)
		    && (self->time_sequence.count == other->time_sequence.count)
		    && (self->time_sequence.stride == other->time_sequence.stride)
		    && (self->time_sequence.cumulative == other->time_sequence.cumulative)) {
			return 1;
		}
	} else if (self->type == nm_TARGET_PATH_LIST) {
		if (self->path_list.end - self->path_list.begin == other->path_list.end - other->path_list.begin) {
			nm_Path* *it_self  = self->path_list.begin;
			nm_Path* *it_other = other->path_list.begin;
			while (it_self != self->path_list.end) {
				if (!nm_Path_is_equal(*it_self, *it_other)) {
					break;
				}
				++it_self;
				++it_other;
				return 1;
			}
		}
	}
	return 0;
}

//------------------------------------------------------------------------------
// nm_MeasureSource
//------------------------------------------------------------------------------
//
/*
static void
nm_MeasureSource_init(nm_MeasureSource *self, MemoryBlock *names_begin, MemoryBlock *names_end, u8 *levels_begin, u8* levels_end)
{
	pt_fill((char*) self, (char*) self + sizeof(nm_MeasureSource), 0);
	self->num_nanocubes = 0;
	self->names.begin   = names_begin;
	self->names.end     = names_end;
	self->levels.begin  = levels_begin;
	self->levels.end    = levels_end;
}

// NOTE(llins): This assumes the nanocubes have the schema
static void
nm_MeasureSource_insert_nanocube(nm_MeasureSource *self, nx_NanocubeIndex *index, void *nanocube)
{
	Assert(self->num_nanocubes < nm_MeasureSource_MAX_NANOCUBES_PER_SOURCE);
	self->indices[self->num_nanocubes] = index;
	self->nanocubes[self->num_nanocubes] = nanocube;
	++self->num_nanocubes;
}
*/

static s32
nm_MeasureSource_dimension_by_name(nm_MeasureSource *self, char *name_begin, char *name_end)
{
	for (s32 i=0;i<self->num_dimensions;++i) {
		nm_MeasureSourceItem *item = &self->items[i];
		if (cstr_compare_memory(name_begin, name_end, item->dim_name, item->dim_name + item->dim_name_length) == 0) {
			return i;
		}
	}
	return -1;
}

//------------------------------------------------------------------------------
// nm_MeasureExpression
//------------------------------------------------------------------------------

static void
nm_MeasureExpression_init_source(nm_MeasureExpression *self, u32 index)
{
	for (s32 i=0;i<sizeof(nm_MeasureExpression);++i) {
		*((char*) self + i) = 0;
	}
	self->is_source = 1;
	self->source_index = index;
}

static void
nm_MeasureExpression_init_number(nm_MeasureExpression *self, double number)
{
	for (s32 i=0;i<sizeof(nm_MeasureExpression);++i) {
		*((char*) self + i) = 0;
	}
	self->is_number = 1;
	self->number = number;
}

static void
nm_MeasureExpression_init_binary_op(nm_MeasureExpression *self,
				    nm_MeasureExpression_Operation_Type type,
				    nm_MeasureExpression *left,
				    nm_MeasureExpression *right)
{
	for (s32 i=0;i<sizeof(nm_MeasureExpression);++i) {
		*((char*) self + i) = 0;
	}
	self->is_binary_op = 1;
	self->op.type  = type;
	self->op.left  = left;
	self->op.right = right;
}

static void
nm_MeasureExpression_init_unary_op(nm_MeasureExpression *self,
				   nm_MeasureExpression_Operation_Type type,
				   nm_MeasureExpression* left)
{
	for (s32 i=0;i<sizeof(nm_MeasureExpression);++i) {
		*((char*) self + i) = 0;
	}
	self->is_unary_op = 1;
	self->op.type  = type;
	self->op.left  = left;
	self->op.right = 0;
}


//------------------------------------------------------------------------------
// nm_MeasureSourceBinding
//------------------------------------------------------------------------------

static nm_MeasureSourceBinding*
nm_MeasureSourceBinding_dimension_binding(nm_MeasureSourceBinding *it, u8 dimension)
{
	while (it) {
		if (it->dimension == dimension)
			return it;
		it = it->next;
	}
	return 0;
}

//------------------------------------------------------------------------------
// nm_Measure
//------------------------------------------------------------------------------

// return number of sources that were successfully bound
static nm_MeasureSourceBinding*
nm_Measure_alloc_binding(nm_Measure *self)
{
	return (nm_MeasureSourceBinding*) BilinearAllocator_alloc_left(self->memory, sizeof(nm_MeasureSourceBinding));
}

static nm_MeasureExpression*
nm_Measure_alloc_expression(nm_Measure *self)
{
	return (nm_MeasureExpression*) BilinearAllocator_alloc_left(self->memory, sizeof(nm_MeasureExpression));
}

static void
nm_Measure_init(nm_Measure *self, nm_MeasureSource *source, BilinearAllocator* memory)
{
	self->memory = memory;
	for (s32 i=0;i<nm_Measure_Max_Sources;++i) {
		self->sources[i]      = 0;
		self->bindings[i]     = 0;
		self->num_bindings[i] = 0;
	}
	self->sources[0]      = source;
	self->num_sources     = 1;

	self->expression = nm_Measure_alloc_expression(self);
	nm_MeasureExpression_init_source(self->expression, 0);
}

static nm_MeasureSourceBinding*
nm_Measure_copy_binding_list(nm_Measure *self, nm_MeasureSourceBinding *it)
{
	nm_MeasureSourceBinding *begin = 0;
	nm_MeasureSourceBinding *last  = 0;
	while (it) {
		nm_MeasureSourceBinding *copy = nm_Measure_alloc_binding(self);
		copy->dimension = it->dimension;
		copy->target    = it->target;
		copy->hint      = it->hint;
		copy->next      = 0;

		if (begin) {
			last->next = copy;
			last = copy;
		} else {
			begin = copy;
			last  = copy;
		}

		it = it->next;
	}
	return begin;
}

/*
 * Search all the sources with free dimension
 * named 'name' and bind the target to those.
 * Returns number of actual bindings that happened.
 */
static u32
nm_Measure_bind(nm_Measure *self, char *name_begin, char *name_end, nm_Target target, nm_BindingHint hint)
{
	u32 result = 0;
	for (s32 i=0;i<self->num_sources;++i) {
		s32 dim = nm_MeasureSource_dimension_by_name(self->sources[i], name_begin, name_end);
		if (dim >= 0) {
			Assert(dim < 256);
			if (nm_MeasureSourceBinding_dimension_binding(self->bindings[i], (u8) dim) == 0) {
				++result;
				nm_MeasureSourceBinding *binding = nm_Measure_alloc_binding(self);
				binding->target = target;
				binding->dimension = (u8) dim;
				binding->next = self->bindings[i];
				binding->hint = hint;
				self->bindings[i] = binding;
				++self->num_bindings[i];
			}
		}
	}
	return result;
}

/*
 * Assumes the payload configuration of all sources
 * are undefined (==0).
 * error codes: 1 a source is already configured
 */
static u8
nm_Measure_set_payload_config(nm_Measure *self, void *payload_config)
{
	for (s32 i=0;i<self->num_sources;++i) {
		if (self->payload_config[i] != 0) {
			return 1;
		}
		// TODO(llins): create infrastructure to check if
		// source is compatible with the opaque payload config
		self->payload_config[i] = payload_config;
	}
	return 0;
}

static nm_MeasureExpression*
nm_Measure_copy_expression_adjust_source(nm_Measure *self, nm_MeasureExpression *expression, u32 *mapping)
{
	Assert(expression);

	nm_MeasureExpression *copy = nm_Measure_alloc_expression(self);
	if (expression->is_source) {
		nm_MeasureExpression_init_source(copy, *(mapping + expression->source_index));
	} else if (expression->is_number) {
		nm_MeasureExpression_init_number(copy, expression->number);
	} else if (expression->is_binary_op) {
		nm_MeasureExpression *left_copy = nm_Measure_copy_expression_adjust_source(self, expression->op.left,  mapping);
		nm_MeasureExpression *right_copy = nm_Measure_copy_expression_adjust_source(self, expression->op.right, mapping);
		nm_MeasureExpression_init_binary_op(copy, expression->op.type, left_copy, right_copy);
	} else if (expression->is_unary_op) {
		nm_MeasureExpression *left_copy = nm_Measure_copy_expression_adjust_source(self, expression->op.left,  mapping);
		nm_MeasureExpression_init_unary_op(copy, expression->op.type, left_copy);
	}
	else {
		Assert(0);
	}
	return copy;
}

static void
nm_Mesaure_combine_number(nm_Measure *self, double number, nm_MeasureExpression_Operation_Type op, b8 measure_is_left)
{
	Assert(self->expression);
	Assert(op != nm_MEASURE_EXPRESSION_OPERATION_UNARY_MINUS);

	nm_MeasureExpression *numexp = nm_Measure_alloc_expression(self);
	nm_MeasureExpression_init_number(numexp, number);


	nm_MeasureExpression *newexp = nm_Measure_alloc_expression(self);
	nm_MeasureExpression_init_binary_op
		(newexp, op, measure_is_left ? self->expression : numexp,
		 measure_is_left ? numexp : self->expression);

	self->expression = newexp;
}


static b8
nm_sources_are_the_same(nm_Measure *a, u32 a_src, nm_Measure *b, u32 b_src)
{
	Assert(a->sources[a_src]->num_nanocubes > 0);
	Assert(b->sources[b_src]->num_nanocubes > 0);

	if (a->sources[a_src]->items[0].src_index != b->sources[b_src]->items[0].src_index)
		return 0;

	if (a->num_bindings[a_src] != b->num_bindings[b_src])
		return 0;

	/* @TODO(llins): should insert a deep comparison here */
	if (a->payload_config[a_src] != b->payload_config[b_src])
		return 0;

	nm_MeasureSourceBinding *begin_b = b->bindings[b_src];
	nm_MeasureSourceBinding *it_a = a->bindings[a_src];
	while (it_a) {
		nm_MeasureSourceBinding *candidate_b = nm_MeasureSourceBinding_dimension_binding(begin_b, it_a->dimension);
		if (candidate_b == 0) {
			return 0;
		} else if (!nm_Target_is_equal(&it_a->target, &candidate_b->target)) {
			return 0;
		}
		it_a = it_a->next;
	}
	return 1;
}

static void
nm_Measure_combine_measure(nm_Measure *self, nm_Measure *other, nm_MeasureExpression_Operation_Type op)
{
	Assert(op != nm_MEASURE_EXPRESSION_OPERATION_UNARY_MINUS);

	u32 mapping[nm_Measure_Max_Sources];

	u32 free_index = self->num_sources;

	// @TODO(llins): cleanup this deep code;
	// consider if the payload_configs are compatible

	for (u32 i=0;i<other->num_sources;++i) {
		s32 match_index = -1;
		// see if there is a match
		for (s32 j=0;j<self->num_sources;++j) {
			b8 match = nm_sources_are_the_same(other,i,self,j);
			if (match) {
				match_index = j;
				break;
			}
		}

		if (match_index >= 0) {
			mapping[i] = match_index;
		} else {
			self->sources[free_index] = other->sources[i];
			self->num_bindings[free_index] = other->num_bindings[i];
			self->bindings[free_index] = nm_Measure_copy_binding_list(self, other->bindings[i]);
			self->payload_config[free_index] = other->payload_config[i];
			mapping[i] = free_index;
			++free_index;
			++self->num_sources;
		}
	}

	//
	// copy expression replacing the source indices
	// using the mapping array
	//
	nm_MeasureExpression *left  = self->expression;
	nm_MeasureExpression *right = nm_Measure_copy_expression_adjust_source (self, other->expression, mapping);
	self->expression = nm_Measure_alloc_expression(self);
	nm_MeasureExpression_init_binary_op(self->expression, op, left, right);
}

static void
nm_Measure_combine_number(nm_Measure *self, f64 number, nm_MeasureExpression_Operation_Type op, b8 number_on_right)
{
	Assert(op != nm_MEASURE_EXPRESSION_OPERATION_UNARY_MINUS);

	nm_MeasureExpression *previous_exp = self->expression;
	nm_MeasureExpression *number_exp = nm_Measure_alloc_expression(self);
	nm_MeasureExpression_init_number(number_exp, number);

	self->expression = nm_Measure_alloc_expression(self);
	if (number_on_right) {
		nm_MeasureExpression_init_binary_op(self->expression, op, previous_exp, number_exp);
	} else {
		nm_MeasureExpression_init_binary_op(self->expression, op, number_exp, previous_exp);
	}
}

//------------------------------------------------------------------------------
// Table Keys
//------------------------------------------------------------------------------

// MeasureSource + MeasureSourceBindings -> TableKeyType

static nm_TableKeysType*
nm_TableKeysType_from_measure_source_and_bindings(LinearAllocator *memsrc, nm_MeasureSource *src, nm_MeasureSourceBinding *binding, nm_Services *nm_services, s32 *error)
{
	*error = nm_OK;

	Assert(src->num_nanocubes > 0);

	// LinearAllocatorCheckpoint chkpt = LinearAllocator_checkpoint(memsrc);

	u8 dimension_of[nx_MAX_NUM_DIMENSIONS];
	u8 path_buffer[128];

	// respect the order of the dimensions in the src
	nm_TableKeysColumnType *tkct_begin = 0;
	nm_TableKeysColumnType *tkct_end   = 0;
	u8 *dim_it = dimension_of;
	nm_MeasureSourceBinding *it = binding;
	while (it) {
		if (it->target.anchor) {
			// found column
			nm_TableKeysColumnType *newcol = (nm_TableKeysColumnType*) LinearAllocator_alloc(memsrc, sizeof(nm_TableKeysColumnType));
			newcol->name = nm_measure_source_dim_name(src,it->dimension); // *(src->names.begin + it->dimension);

			// figure depth of target
			s32 depth = -1;
			{
				nm_Dive *begin = 0;
				nm_Dive *end = 0;
				if (it->target.type == nm_TARGET_FIND_DIVE) {
					begin = &it->target.find_dive;
					end   = begin + 1;
				} else if (it->target.type == nm_TARGET_FIND_DIVE_LIST) {
					begin = it->target.find_dive_list.begin;
					end   = it->target.find_dive_list.end;
				} else {
					Assert(0);
				}
				nm_Dive *it2 = begin;
				while (it2 != end) {
					// assume the path is not by_alias anymore
					if (it2->path.by_alias) {
						s32 candidate_depth = 0;
						if (nm_services->get_alias_path(src->items[0].src_nanocube, (u8) it->dimension, it2->path.alias, sizeof(path_buffer), path_buffer, &candidate_depth)) {
							candidate_depth += it2->depth;
							if (depth < 0) {
								depth = candidate_depth;
							} else if (depth != candidate_depth) {
								*error = nm_ERROR_TYPE_INFERENCE_DIFFERENT_DIVE_DEPTHS;
								return 0;
							}
						} else {
							*error = nm_ERROR_TYPE_INFERENCE_INVALID_ALIAS;
							return 0;
						}
					} else {
						s32 candidate_depth = it2->path.array.length;
						candidate_depth += it2->depth;
						if (depth < 0) {
							depth = candidate_depth;
						} else if (depth != candidate_depth) {
							*error = nm_ERROR_TYPE_INFERENCE_DIFFERENT_DIVE_DEPTHS;
							return 0;
						}
					}
					++it2;
				}
			}
			newcol->levels = depth; // it->target.find_dive.path.array.length + it->target.find_dive.depth;
			newcol->bits = src->items[0].src_index->bits_per_label[it->dimension];
			newcol->loop_column = 0;
			newcol->hint = it->hint;
			if (tkct_begin == 0) {
				tkct_begin = newcol;
			}
			tkct_end = newcol + 1;
			*dim_it = it->dimension;
			++dim_it;

		} else if (it->target.loop) {
			// found column
			nm_TableKeysColumnType *newcol = (nm_TableKeysColumnType*) LinearAllocator_alloc(memsrc, sizeof(nm_TableKeysColumnType));
			newcol->name   = nm_measure_source_dim_name(src, it->dimension);
			newcol->levels = 0; /* not well defined */
			newcol->bits   = 0;
			newcol->loop_column = 1;
			newcol->hint = it->hint;
			if (tkct_begin == 0) {
				tkct_begin = newcol;
			}
			tkct_end = newcol + 1;
			*dim_it = it->dimension;
			++dim_it;
		}
		it = it->next;
	}

	//
	// we have either a loop on a dimension or
	// same level anchored nodes
	//
	u8 columns = (u8) (tkct_end - tkct_begin);

	// insertion sort
	for (s32 i=1;i<columns;++i) {
		for (s32 j=0;j<i;++j) {
			Assert(dimension_of[i] != dimension_of[j]);
			if (dimension_of[i] < dimension_of[j]) {
				{
					u8 swp = dimension_of[i];
					dimension_of[i] = dimension_of[j];
					dimension_of[j] = swp;
				}
				{
					nm_TableKeysColumnType swp = *(tkct_begin + i);
					*(tkct_begin + i) = *(tkct_begin + j);
					*(tkct_begin + j) = swp;
				}
			}
		}
	}

	nm_TableKeysType *tkt = (nm_TableKeysType*) LinearAllocator_alloc(memsrc, sizeof(nm_TableKeysType));
	tkt->begin = tkct_begin;
	tkt->end   = tkct_end;

	return tkt;
}

static inline u32
nm_TableKeysColumnType_bytes(nm_TableKeysColumnType *self)
{
	return self->loop_column ? (u32) sizeof(u32) : (self->levels * self->bits + 7) / 8;
}

static nm_TableKeysColumnType_Relation
nm_TableKeysColumnType_compare(nm_TableKeysColumnType *c1, nm_TableKeysColumnType *c2)
{
	nm_TableKeysColumnType_Relation result;
	result.flags = 0;

	// names need to match
	s64 names_diff = cstr_compare_memory(c1->name.begin, c1->name.end,
					   c2->name.begin, c2->name.end);
	if (names_diff) {
		result.is_different = 1;
		return result;
	} else if (c1->loop_column != c2->loop_column) {
		result.is_different = 1;
		return result;
	} else if (c1->loop_column) {
		result.is_equal = 1;
		return result;
	} else if (c1->bits != c2->bits) {
		result.is_different = 1;
		return result;
	} else if (c1->levels < c2->levels) {
		result.is_coarser = 1;
		return result;
	} else if (c1->levels == c2->levels) {
		result.is_equal = 1;
		return result;
	} else { // if (c1->levels > c2->levels)
		result.is_finer = 1;
		return result;
	}
}

static nm_TableKeysType_Alignment*
nm_TableKeysType_align(nm_TableKeysType *t1, nm_TableKeysType *t2, LinearAllocator *memsrc)
{
	u8 c1 = pt_safe_s64_u8(t1->end - t1->begin);
	u8 c2 = pt_safe_s64_u8(t2->end - t2->begin);

	nm_TableKeysType_Alignment *alignment = (nm_TableKeysType_Alignment*)
		LinearAllocator_alloc
		(memsrc, sizeof(nm_TableKeysType_Alignment));

	alignment->relation.flags = 0;
	alignment->t1 = t1;
	alignment->t2 = t2;
	alignment->c1 = c1;
	alignment->c2 = c2;
	alignment->mapping1 = (nm_TableKeysType_Column_Match*)
		LinearAllocator_alloc(memsrc, sizeof(nm_TableKeysType_Column_Match) * c1);
	alignment->mapping2 = (nm_TableKeysType_Column_Match*)
		LinearAllocator_alloc(memsrc, sizeof(nm_TableKeysType_Column_Match) * c2);

	for (u8 i=0;i<c1;++i) {
		(alignment->mapping1 + i)->data = 0;
	}

	for (u8 i=0;i<c2;++i) {
		(alignment->mapping2 + i)->data = 0;
	}

	u8 unmatched1 = 0;
	u8 unmatched2 = 0;

	for (u8 i=0;i<c1;++i) {
		nm_TableKeysColumnType *col_type1 = t1->begin + i;
		nm_TableKeysType_Column_Match *mapping_i = alignment->mapping1 + i;
		for (u8 j=0;j<c2;++j)
		{
			nm_TableKeysColumnType *col_type2 = t2->begin + j;
			nm_TableKeysColumnType_Relation r =
				nm_TableKeysColumnType_compare
				(col_type1, col_type2);
			if (r.is_different) {
				continue;
			} else if (mapping_i->data != 0) {
				// ambiguity
				mapping_i->data = 0;
				mapping_i->is_ambiguous = 1;
				mapping_i->is_unmatched = 1;
			} else {
				mapping_i->corresponding_index = j;
				if (r.is_finer) {
					mapping_i->is_finer = 1;
				} else if (r.is_coarser) {
					mapping_i->is_coarser = 1;
				} else /* if (r.is_equal) */ {
					mapping_i->is_equal = 1;
				}
			}
		}

		if (mapping_i->data == 0)
		{
			mapping_i->is_unmatched = 1;
		}

		if (mapping_i->is_unmatched)
			++unmatched1;
	}

	for (u8 i=0;i<c2;++i) {
		nm_TableKeysColumnType *col_type2 = t2->begin + i;
		nm_TableKeysType_Column_Match *mapping_i = alignment->mapping2 + i;
		for (u8 j=0;j<c1;++j) {
			nm_TableKeysColumnType *col_type1 = t1->begin + j;
			nm_TableKeysColumnType_Relation r = nm_TableKeysColumnType_compare(col_type2, col_type1);
			if (r.is_different) {
				continue;
			} else if (mapping_i->data != 0) {
				// ambiguity
				mapping_i->data = 0;
				mapping_i->is_ambiguous = 1;
				mapping_i->is_unmatched = 1;
			} else {
				mapping_i->corresponding_index = j;
				if (r.is_finer) {
					mapping_i->is_finer = 1;
				} else if (r.is_coarser) {
					mapping_i->is_coarser = 1;
				}
				else /* if (r.is_equal) */ {
					mapping_i->is_equal = 1;
				}
			}
		}

		if (mapping_i->data == 0) {
			mapping_i->is_unmatched = 1;
		}

		if (mapping_i->is_unmatched)
			++unmatched2;
	}


	if (unmatched1 && unmatched2) {
		alignment->relation.is_different = 1;
	} else if (unmatched1) {
		// all columns on mapping2 should be coarser or equal
		// and the result will be is_finer, otherwise it is
		// is_different
		b8 is_finer = 1;
		for (u8 i=0;i<c2;++i) {
			nm_TableKeysType_Column_Match *mapping_i =
				alignment->mapping2 + i;
			if (!mapping_i->is_coarser && !mapping_i->is_equal) {
				is_finer = 0;
				break;
			}
		}
		if (is_finer) {
			alignment->relation.is_finer = 1;
		} else {
			alignment->relation.is_different = 1;
		}
	}
	else if (unmatched2) {
		// all columns on mapping2 should be coarser or equal
		// and the result will be is_finer, otherwise it is
		// is_different
		b8 is_coarser = 1;
		for (u8 i=0;i<c1;++i) {
			nm_TableKeysType_Column_Match *mapping_i =
				alignment->mapping1 + i;
			if (!mapping_i->is_finer && !mapping_i->is_equal) {
				is_coarser = 0;
				break;
			}
		}
		if (is_coarser) {
			alignment->relation.is_coarser = 1;
		} else {
			alignment->relation.is_different = 1;
		}
	}
	else if (c1) { // are there any columns?
		// all columns match. see if they are all equal
		// all coarser or all finer
		for (u8 i=1;i<c1;++i) {
			nm_TableKeysType_Column_Match *mapping_i =
				alignment->mapping1 + i;
			if ((mapping_i->is_coarser
			     && alignment->relation.is_finer)
			    || (mapping_i->is_finer
				&& alignment->relation.is_coarser)
			    ) {
				alignment->relation.flags = 0;
				alignment->relation.is_different = 1;
				break;
			}
			else if (mapping_i->is_coarser) {
				alignment->relation.is_coarser = 1;
			}
			else if (mapping_i->is_finer) {
				alignment->relation.is_finer = 1;
			}
		}
		if (alignment->relation.flags == 0) {
			alignment->relation.is_equal = 1;
		}
	}
	else {
		alignment->relation.is_equal = 1;
	}

	return alignment;

}


static void
nm_TableKeys_init(nm_TableKeys *self, nm_TableKeysType *type, LinearAllocator *memsrc)
{
	self->type = type;
	self->current_column = 0;
	self->memsrc = memsrc;
	self->keys.begin = 0;
	self->keys.end = 0;
	self->columns = (u8) (type->end - type->begin);
	self->rows = 0;
	self->current_offset = 0;
	// @todo key_size

	self->row_length = 0;
	for (s32 i=0;i<self->columns;++i) {
		nm_TableKeysColumnType *coltype = self->type->begin + i;
		if (coltype->loop_column) {
			self->row_length += sizeof(u32);
		} else {
			self->row_length += (coltype->bits * coltype->levels + 7)/8;
		}
	}

	self->keys.begin = LinearAllocator_alloc(self->memsrc, self->row_length);
	self->keys.end = self->keys.begin;
}

static s32
nm_TableKeys_push_anchor_column(nm_TableKeys *self, nx_Label *begin, nx_Label *end)
{
	if (self->current_column == self->columns) {
		return nm_ERROR_INDEX_COLUMNS_OUT_OF_MEMORY;
	}

	nm_TableKeysColumnType *coltype = self->type->begin + self->current_column;

	Assert(coltype->loop_column == 0);
	Assert(coltype->levels == (u8)(end - begin));

	u32 bits   = coltype->bits;
	u32 levels = coltype->levels;
	u32 bytes  = (coltype->bits * coltype->levels + 7)/8;

	char *col_value = self->keys.end + self->current_offset;

	pt_fill(col_value, col_value + bytes, 0);

	for (u32 i=0;i<levels;++i) {
		pt_write_bits2((char*) begin + i, bits * (levels - 1 - i), bits, col_value);
	}

	self->current_offset += bytes;
	++self->current_column;

	return nm_OK;
}

static void
TableKeys_push_loop_column(nm_TableKeys *self, u32 index)
{
	Assert(self->current_column < self->columns);

	nm_TableKeysColumnType *coltype =
		self->type->begin + self->current_column;

	Assert(coltype->loop_column == 1);

	u32 *col_value = (u32*) (self->keys.end + self->current_offset);

	*col_value = index;

	self->current_offset += sizeof(u32);
	++self->current_column;
}

static void
nm_TableKeys_pop_column(nm_TableKeys *self)
{
	Assert(self->current_column > 0);
	--self->current_column;

	nm_TableKeysColumnType *coltype =
		self->type->begin + self->current_column;

	if (coltype->loop_column) {
		self->current_offset -= sizeof(u32);
	}
	else {
		//         u32 bits   = coltype->bits;
		//         u32 levels = coltype->levels;
		u32 bytes  = (coltype->bits * coltype->levels + 7)/8;
		self->current_offset -= bytes;
	}
}

static s32
nm_TableKeys_commit_key(nm_TableKeys *self)
{
	Assert(self->current_column == self->columns);

	u32 len = self->row_length;

	// running record
	char *running_key = LinearAllocator_alloc_if_available(self->memsrc, self->row_length);
	if (!running_key)
		return nm_ERROR_INDEX_COLUMNS_OUT_OF_MEMORY;
	Assert(running_key == self->keys.end + len); // invariant: reserved space for one extra record

	pt_copy_bytes(self->keys.end, self->keys.end + len, running_key, running_key + len);

	self->keys.end = running_key;

	++self->rows;

	return nm_OK;
}

static void
nm_TableKeys_append_row_copying(nm_TableKeys *self, nm_TableKeys *src, u32 src_index)
{
	Assert(self->row_length == src->row_length);

	u32 len = self->row_length;
	pt_copy_bytes(src->keys.begin + src_index * len,
		      src->keys.begin + (src_index+1) * len,
		      self->keys.end,
		      self->keys.end + len );

	char *running_key = LinearAllocator_alloc(self->memsrc, self->row_length);
	Assert(running_key == self->keys.end + len); // invariant: reserved space for one extra record

	self->keys.end += len;
	++self->rows;
}

static void
nm_TableKeys_print_header(nm_TableKeys *self, Print *print)
{
	nm_TableKeysColumnType *it = self->type->begin;
	while (it != self->type->end) {
		print_str(print, it->name.begin, it->name.end);
		print_align(print, 50, 1, ' ');
		++it;
	}
}

static void
nm_TableKeys_print(nm_TableKeys *self, Print *print, u32 record_index)
{
	Assert(record_index < self->rows);
	char *it = self->keys.begin + self->row_length * record_index;
	for (u32 i=0;i<self->columns;++i) {
		nm_TableKeysColumnType *coltype = self->type->begin + i;

		char *check_point = print->end;
		if (coltype->loop_column) {
			u32 val = *((u32*) it);
			print_u64(print, (u64) val);
			it += sizeof(u32);
		} else {
			u32 bits   = coltype->bits;
			u32 levels = coltype->levels;
			u32 bytes  = (coltype->bits * coltype->levels + 7)/8;
			for (u32 j=0;j<levels;++j) {
				nx_Label label = 0;
				pt_read_bits2(it, bits * (levels - 1 - j), bits, (char*) &label);
				if (j > 0) {
					print_cstr(print, ",");
				}
				print_u64(print, (u64) label);
			}
			it += bytes;
		}
		print_fake_last_print(print, check_point);
		print_align(print, 50, 1, ' ');
	}
}

static void
nm_TableKeys_print_json(nm_TableKeys *self, Print *print)
{
	print_cstr(print, "[");
	nm_TableKeysColumnType *it_coltype = self->type->begin;
	u32 record_size   = self->row_length;
	u32 column_offset = 0;
	while (it_coltype!= self->type->end) {

		u8 levels = it_coltype->levels;
		u8 bits   = it_coltype->bits;

		if (it_coltype != self->type->begin)
			print_char(print, ',');

		print_cstr(print, "{ \"name\":\"");
		print_str(print, it_coltype->name.begin, it_coltype->name.end);
		print_cstr(print, "\", \"values_per_row\":");
		if (it_coltype->loop_column) {
			print_u64(print, 1);
		} else {
			print_u64(print, levels);
		}
		print_cstr(print, ", \"values\":[");
		if (it_coltype->loop_column) {
			char *it = self->keys.begin + column_offset;
			if (self->rows) {
				print_u64(print, *((u32*)(it + column_offset)));
				for (u32 i=1;i<self->rows;++i) {
					it += record_size;
					print_char(print, ',');
					print_u64(print, *((u32*)(it + column_offset)));
				}
			}
			column_offset += sizeof(u32);
		} else {
			char *it = self->keys.begin + column_offset;
			if (self->rows) {
				for (u32 j=0;j<it_coltype->levels;++j) {
					nx_Label label = 0;
					pt_read_bits2(it, bits * (levels - 1 - j), bits, (char*) &label);
					if (j > 0) {
						print_char(print, ',');
					}
					print_u64(print, (u64) label);
				}
				for (u32 i=1;i<self->rows;++i) {
					it += record_size;
					for (u32 j=0;j<it_coltype->levels;++j) {
						nx_Label label = 0;
						pt_read_bits2(it, bits * (levels - 1 - j), bits, (char*) &label);
						print_char(print, ',');
						print_u64(print, (u64) label);
					}
				}
			}
			column_offset += (it_coltype->bits * it_coltype->levels + 7)/8;
		}
		print_cstr(print, "]}");
		++it_coltype;
	}
	print_cstr(print, "]");
}

static void
nm_Permutation_init(nm_Permutation* self, u32 *begin, u32 *end)
{
	Assert(begin < end);
	self->begin = begin;
	self->end   = end;
	u32 *it = self->begin;
	u32 i = 0;
	while (it != self->end) {
		*it = i;
		++it;
		++i;
	}
}

static b8
nm_Permutation_is_identity(nm_Permutation *self)
{
	u32 i=0;
	u32 *it = self->begin;
	while (it != self->end) {
		if (*it != i) return 0;
		++i;
		++it;
	}
	return 1;
}

static b8
nm_TableKeys_less_than(nm_TableKeys *self, u32 rowi, u32 rowj)
{
	Assert(rowi < self->rows && rowj < self->rows);
	u8* bi = (u8*) self->keys.begin + rowi * self->row_length;
	u8* bj = (u8*) self->keys.begin + rowj * self->row_length;

	nm_TableKeysColumnType *col = self->type->begin;
	while (col != self->type->end) {
		if (col->loop_column) {
			/* TODO(llins): cleanup this */
			/* hard coded the u32 encoding of loop columns */
			u32 bytes = sizeof(u32);
			u32 a = *((u32*) bi);
			u32 b = *((u32*) bj);
			if (a < b) return 1;
			else if (a > b) return 0;
			bi += bytes;
			bj += bytes;

		} else {
			u32 bytes = (col->levels * col->bits + 7)/8;
			for (u32 i=0;i<bytes;++i) {
				u32 off = (bytes - 1 - i);
				u8 a = *(bi + off);
				u8 b = *(bj + off);
				if (a < b) return 1;
				else if (a > b) return 0;
			}
			bi += bytes;
			bj += bytes;
		}
		++col;
	}
	return 0; // equal
}

static b8
nm_TableKeys_is_totally_ordered(nm_TableKeys *self)
{
	for (u32 i=0;i<self->rows-1;++i) {
		if (!nm_TableKeys_less_than(self, i, i+1)) {
			return 0;
		}
	}
	return 1;
}

static b8
nm_TableKeys_equal_rows(nm_TableKeys *self, u32 rowi, u32 rowj)
{
	Assert(rowi < self->rows && rowj < self->rows);
	nm_TableKeysColumnType *col = self->type->begin;
	u8* bi = (u8*) self->keys.begin + rowi * self->row_length;
	u8* bj = (u8*) self->keys.begin + rowj * self->row_length;
	while (col != self->type->end) {

		if (col->loop_column) {
			/* TODO(llins): cleanup this */
			/* hard coded the u32 encoding of loop columns */
			u32 bytes = sizeof(u32);
			u32 a = *((u32*) bi);
			u32 b = *((u32*) bj);
			if (a < b) return 0;
			else if (a > b) return 0;
			bi += bytes;
			bj += bytes;
		} else {
			u32 bytes = (col->levels * col->bits + 7)/8;
			for (u32 i=0;i<bytes;++i) {
				u32 off = (bytes - 1 - i);
				u8 a = *(bi + off);
				u8 b = *(bj + off);
				if (a < b) return 0;
				else if (a > b) return 0;
			}
			bi += bytes;
			bj += bytes;
		}
		++col;
	}
	return 1; // equal
}

static void
nm_TableKeys_merge_sort(nm_TableKeys *self,
			u32 *permutation_begin,
			u32 *permutation_end,
			u32 *copy_area_begin,
			u32 *copy_area_end)
{
	u32 n = pt_safe_s64_u32(permutation_end - permutation_begin);
	Assert(n == pt_safe_s64_u32(copy_area_end - copy_area_begin));

	if (n == 1) {
		return;
	}

	u32 mid_point = n/2;
	u32 *permutation_middle = permutation_begin + mid_point;

	nm_TableKeys_merge_sort(self,
				permutation_begin,  permutation_middle,
				copy_area_begin, copy_area_begin + mid_point);
	nm_TableKeys_merge_sort(self,
				permutation_middle, permutation_end,
				copy_area_begin + mid_point, copy_area_end);

	// merge
	u32 *it_left  = permutation_begin;
	u32 *it_right = permutation_middle;
	u32 *it_copy  = copy_area_begin;
	while (it_left != permutation_middle && it_right != permutation_end) {
		if (nm_TableKeys_less_than(self, *it_right, *it_left)) {
			*it_copy = *it_right;
			++it_right;
		} else {
			*it_copy = *it_left;
			++it_left;
		}
		++it_copy;
	}
	while (it_left != permutation_middle) {
		*it_copy = *it_left;
		++it_left;
		++it_copy;
	}

	/*
	 * copy back region of permutation back to original memory
	 * note that the region
	 *      [permutation_begin + (copy_area_end - it_copy), permutation_end)
	 * is preserved in the old and new permutation.
	 */
	it_left     = permutation_begin;
	u32 *it_copyback = copy_area_begin;
	while (it_copyback != it_copy) {
		*it_left = *it_copyback;
		++it_left;
		++it_copyback;
	}
}

static nm_Permutation*
nm_TableKeys_order(nm_TableKeys *self, LinearAllocator *memsrc)
{
	nm_Permutation *permutation = (nm_Permutation*) LinearAllocator_alloc(memsrc, sizeof(nm_Permutation));

	u32 n = self->rows;
	u32 permutation_entries_size  = n * sizeof(u32);

	//
	// with -O3 optimization, the xmmx registers are used
	// they need to be 4-byte aligned to work
	//
	u32* permutation_begin = (u32*) LinearAllocator_alloc_aligned (memsrc, permutation_entries_size, 4);

	LinearAllocatorCheckpoint chkpt = LinearAllocator_checkpoint(memsrc);

	nm_Permutation_init(permutation, permutation_begin, permutation_begin + n);

	if (n == 1)
		return permutation;

	u32* copy_area_begin = (u32*) LinearAllocator_alloc_aligned(memsrc, permutation_entries_size, 4);

	nm_TableKeys_merge_sort(self,
				permutation_begin, permutation_begin + n,
				copy_area_begin, copy_area_begin + n);

	LinearAllocator_rewind(memsrc, chkpt);

	return permutation;
}

//
// allocates nrow bytes on memsrc and writes 0's and 1's
// there indicating at position i+1 if row[i+1] key is
// equal to row[i]
//
static MemoryBlock
nm_TableKeys_row_repeat_flags(nm_TableKeys *self, nm_Permutation *permutation,
			      LinearAllocator *memsrc)
{
	MemoryBlock result;

	u32 n = self->rows;

	Assert((s64) n == permutation->end - permutation->begin);

	result.begin = LinearAllocator_alloc(memsrc, n * sizeof(char));
	result.end   = result.begin + self->rows;

	if (n == 0) return result;

	char *it = result.begin;
	*it = 0;
	++it;

	u32 prev_pi = *permutation->begin;
	for (u32 i=1;i<n;++i) {
		u32 pi = *(permutation->begin + i);
		*it = nm_TableKeys_equal_rows(self, prev_pi, pi);
		++it;
		prev_pi = pi;
	}

	return result;
}


static void
nm_TableKeysType_permute(nm_TableKeysType *self,
			 nm_Permutation *permutation,
			 LinearAllocator *memsrc)
{
	Assert(permutation->end - permutation->begin
	       == self->end - self->begin);

	LinearAllocatorCheckpoint chkpt = LinearAllocator_checkpoint(memsrc);

	u32 m = pt_safe_s64_u32(self->end - self->begin);

	nm_TableKeysColumnType* buffer = (nm_TableKeysColumnType*)
		LinearAllocator_alloc
		(memsrc, sizeof(nm_TableKeysColumnType) * m);

	for (u32 i=0;i<m;++i) {
		u32 j = *(permutation->begin + i);
		*(buffer + i) = *(self->begin + j);
	}

	for (u32 i=0;i<m;++i) {
		*(self->begin + i) = *(buffer + i);
	}

	LinearAllocator_rewind(memsrc, chkpt);
}

static void
nm_TableKeys_permute_columns(nm_TableKeys *self,
			     nm_Permutation *permutation,
			     LinearAllocator *memsrc)
{
	Assert(permutation->end - permutation->begin
	       == self->type->end - self->type->begin);

	// if identity do nothing
	if (nm_Permutation_is_identity(permutation))
		return;

	LinearAllocatorCheckpoint chkpt = LinearAllocator_checkpoint(memsrc);
	// char *memsrc_checkpoint = memsrc->end;

	u32  m = self->columns;

	u32* offsets    = (u32*) LinearAllocator_alloc(memsrc, sizeof(u32) * m);
	u32* bytes      = (u32*) LinearAllocator_alloc(memsrc, sizeof(u32) * m);

	u32 off = 0;
	for (u32 i=0;i<m;++i) {
		nm_TableKeysColumnType *col = self->type->begin + i;
		u32 b = nm_TableKeysColumnType_bytes(col);
		*(offsets + i) = off;
		*(bytes   + i) = b;
		off += b;
	}

	char *row_buffer = LinearAllocator_alloc(memsrc,  sizeof(u8) * self->row_length);

	char *it_src = self->keys.begin;
	while (it_src != self->keys.end)
	{
		char *it_dst = row_buffer;
		for (u32 i=0;i<m;++i) {
			u32 j = *(permutation->begin + i);
			pt_copy_bytes(
				      it_src + offsets[j],
				      it_src + offsets[j] + bytes[j],
				      it_dst,
				      it_dst + bytes[j]);
			it_dst += bytes[j];
		}
		pt_copy_bytes(row_buffer, row_buffer + self->row_length,
			      it_src, it_src + self->row_length);
		it_src += self->row_length;
	}

	LinearAllocator_rewind(memsrc, chkpt);

	// it owns the type
	nm_TableKeysType_permute(self->type, permutation, memsrc);

}

static void
nm_TableKeys_permute_rows(nm_TableKeys *self,
			  nm_Permutation *permutation,
			  LinearAllocator *memsrc)
{
	u32 n = pt_safe_s64_u32(permutation->end - permutation->begin);

	Assert(n == self->rows);

	LinearAllocatorCheckpoint cp = LinearAllocator_checkpoint(memsrc);

	u32 len  = self->row_length;
	u32 nlen = n * len;

	char *buffer = LinearAllocator_alloc(memsrc, nlen);

	char *it = buffer;

	for (u32 i=0;i<n;++i) {
		u32 j = *(permutation->begin + i);
		char *src = self->keys.begin + j * len;
		pt_copy_bytes(src, src + self->row_length, it, it + len);
		it += len;
	}

	// copy back
	pt_copy_bytes(buffer, buffer + nlen,
		      self->keys.begin, self->keys.begin + nlen);

	LinearAllocator_rewind(memsrc, cp);
}

static void
nm_TableKeys_permute_rows_with_repetition(nm_TableKeys *self, nm_Permutation *permutation, MemoryBlock repeat_flags)
{
	Assert(self->memsrc);

	u32 n = pt_safe_s64_u32(permutation->end - permutation->begin);

	Assert(n == self->rows);
	Assert(n == repeat_flags.end - repeat_flags.begin);

	u32 len  = self->row_length;
	u32 nlen = n * len;

	// assuming memsrc is still in sync with table keys
	Assert(self->keys.end + len == self->memsrc->end);

	// this buffer is forgotten by the end of this procedure
	// when ajusting the memsrc->end pointer.
	// works because it is in sync.
	char *buffer = LinearAllocator_alloc(self->memsrc, nlen);

	char *it = buffer;
	u32   nn = 0; // new (equal or smaller) number of rows

	for (u32 i=0;i<n;++i) {
		if (*(repeat_flags.begin + i)) {
			/* make sure there it is the same keys */
#if 1
			u32 j = *(permutation->begin + i);
			Assert(cstr_compare_memory(buffer + (nn-1)*len,
						 buffer + nn*len,
						 self->keys.begin + j * len,
						 self->keys.begin + (j+1) * len) == 0);
#endif
			continue; // don't append key. equal to previous key.
		}
		u32 j = *(permutation->begin + i);
		char *src = self->keys.begin + j * len;
		pt_copy_bytes(src, src + len, it, it + len);
		it += len;
		++nn;
	}

	// copy back
	u32 nnlen = nn * len;
	pt_copy_bytes(buffer, buffer + nnlen, self->keys.begin, self->keys.begin + nnlen);

	self->rows     = nn;
	self->keys.end = self->keys.begin + nnlen;

	self->memsrc->end = self->keys.end + len; // space for one extra record
}

static nm_TableKeysType*
nm_TableKeysType_copy(nm_TableKeysType *self, LinearAllocator *memsrc)
{
	u32 m = pt_safe_s64_u32(self->end - self->begin);

	nm_TableKeysType* result = (nm_TableKeysType*)
		LinearAllocator_alloc(memsrc, sizeof(nm_TableKeysType));
	result->begin = (nm_TableKeysColumnType*)
		LinearAllocator_alloc
		(memsrc, m * sizeof(nm_TableKeysColumnType));
	result->end   = result->begin + m;

	nm_TableKeysColumnType *it_src = self->begin;
	nm_TableKeysColumnType *it_dst = result->begin;
	while (it_src != self->end) {
		*it_dst = *it_src;
		++it_src;
		++it_dst;
	}

	return result;
}

static void
nm_TableKeys_copy_to(nm_TableKeys *self,
		     nm_TableKeys *dst,
		     LinearAllocator *memsrc)
{
	nm_TableKeysType *type_copy = nm_TableKeysType_copy(self->type, memsrc);

	nm_TableKeys *result = dst; // (TableKeys*) LinearAllocator_alloc(memsrc, sizeof(TableKeysType));
	*result = *self;
	result->type = type_copy;

	char *keys_copy =
		LinearAllocator_alloc
		(memsrc, self->keys.end - self->keys.begin);
	result->keys.begin = keys_copy;
	result->keys.end   = keys_copy + (self->keys.end - self->keys.begin);

	pt_copy_bytes(self->keys.begin, self->keys.end,
		      result->keys.begin, result->keys.end);
}

//------------------------------------------------------------------------------
// Eval Measure
//------------------------------------------------------------------------------

static b8
nm_Measure_check_eval_type(nm_Measure *self)
{
	return 1;
}


#define nm_RUN_AND_CHECK_INIT s32 error_ = 0;
#define nm_RUN_AND_CHECK(Exp) \
	error_ = (Exp); \
	if (error_) return error_;

// @todo if return different from zero
static s32
nm_Measure_solve_query(nm_MeasureSolveQueryData *context, nx_Node *root, s32 index, s32 nanocube_index)
{
	Assert(root);

	nm_RUN_AND_CHECK_INIT;

	nm_Target* target = context->target_begin + index;
	b8 last_dimension = (context->target_begin + index + 1) == context->target_end;

	u8 path_buffer[128];

	if (target->type == nm_TARGET_ROOT) {
		if (!target->anchor) {
			if (last_dimension) {
				nx_PNode* pnode = (nx_PNode*) root;

				nm_RUN_AND_CHECK(
						 context->nm_services->append(context->table_values_handle, &pnode->payload_aggregate,
									      nm_measure_source_nanocube(context->source,nanocube_index))
						);

				nm_RUN_AND_CHECK(
						 nm_TableKeys_commit_key(context->table_keys)
						);
			} else {
				nx_INode* inode     = (nx_INode*) root;
				nx_Node*  next_root = nx_Ptr_Node_get(&inode->content_p);

				nm_RUN_AND_CHECK(
						 nm_Measure_solve_query(context, next_root, index + 1, nanocube_index)
						); // recursive call
			}
		} else {
			Assert(0 && "Case not implemented!");
		}

	} else if (target->type == nm_TARGET_FIND_DIVE_LIST) {

		Assert(target->anchor);

		nm_Dive *it_dive  = target->find_dive_list.begin;
		nm_Dive *end_dive = target->find_dive_list.end;

		while (it_dive != end_dive) {
			if (it_dive->path.by_alias) {
				s32 path_length = 0;
				if (context->nm_services->get_alias_path(nm_measure_source_nanocube(context->source,nanocube_index), (u8) index,
									 it_dive->path.alias, sizeof(path_buffer), path_buffer, &path_length)) {
					it_dive->path.array.begin  = path_buffer;
					it_dive->path.array.length = path_length;
				} else {
					// dead end: couldn't translate alias to path
					return nm_ERROR_INVALID_ALIAS;
				}
			}

			nx_Array *path_array  = &it_dive->path.array;
			u8  depth             = it_dive->depth;

			nm_NanocubeIndex_FindDive it;
			nm_NanocubeIndex_FindDive_init(&it, nm_measure_source_index(context->source,nanocube_index), root, index, path_array, depth);

			u8 target_depth = depth + path_array->length;
			nx_Node *target_node;
			while ((target_node = nm_NanocubeIndex_FindDive_next(&it)))
			{

				// add path_array to column on query result
				nx_Label* begin = it.path.begin;
				nx_Label* end   = begin + target_depth;

				nm_RUN_AND_CHECK(
					nm_TableKeys_push_anchor_column(context->table_keys, begin, end)
				);

				if (last_dimension)
				{
					nx_PNode* pnode = (nx_PNode*) target_node;

					// commit record
					nm_RUN_AND_CHECK(
							 context->nm_services->append(context->table_values_handle, &pnode->payload_aggregate, nm_measure_source_nanocube(context->source,nanocube_index))
							);

					nm_RUN_AND_CHECK(
							 nm_TableKeys_commit_key(context->table_keys)
							);
				} else {
					nx_INode *inode = (nx_INode*) target_node;
					nx_Node *next_root = nx_Ptr_Node_get(&inode->content_p);

					nm_RUN_AND_CHECK(
							 nm_Measure_solve_query (context, next_root, index + 1, nanocube_index)
							);
				}

				nm_TableKeys_pop_column(context->table_keys);
			}

			++it_dive;

		}
	} else if (target->type == nm_TARGET_FIND_DIVE) {
		if (target->anchor) {

			if (target->find_dive.path.by_alias) {
				s32 path_length = 0;
				if (context->nm_services->get_alias_path(nm_measure_source_nanocube(context->source,nanocube_index), (u8) index, target->find_dive.path.alias, sizeof(path_buffer), path_buffer, &path_length)) {
					target->find_dive.path.array.begin  = path_buffer;
					target->find_dive.path.array.length = path_length;
				} else {
					// dead end: couldn't translate alias to path
					return nm_ERROR_INVALID_ALIAS;
				}
			}

			nx_Array *path_array  = &target->find_dive.path.array;
			u8  depth = target->find_dive.depth;

			nm_NanocubeIndex_FindDive it;
			nm_NanocubeIndex_FindDive_init(&it, nm_measure_source_index(context->source,nanocube_index), root, index, path_array, depth);

			u8 target_depth = depth + path_array->length;
			nx_Node *target_node;
			while ((target_node = nm_NanocubeIndex_FindDive_next(&it)))
			{

				// add path_array to column on query result
				nx_Label* begin = it.path.begin;
				nx_Label* end   = begin + target_depth;

				nm_RUN_AND_CHECK(
					nm_TableKeys_push_anchor_column(context->table_keys, begin, end)
				);

				if (last_dimension)
				{
					nx_PNode* pnode = (nx_PNode*) target_node;

					// commit record
					nm_RUN_AND_CHECK(
						context->nm_services->append(context->table_values_handle, &pnode->payload_aggregate, nm_measure_source_nanocube(context->source,nanocube_index))
					);

					nm_RUN_AND_CHECK(
							 nm_TableKeys_commit_key(context->table_keys)
							);
				} else {
					nx_INode *inode = (nx_INode*) target_node;
					nx_Node *next_root = nx_Ptr_Node_get(&inode->content_p);
					nm_RUN_AND_CHECK(
							 nm_Measure_solve_query (context, next_root, index + 1, nanocube_index)
							);
				}

				nm_TableKeys_pop_column(context->table_keys);
			}
		} else if (target->find_dive.depth == 0) {
			if (target->find_dive.path.by_alias) {
				// @todo make sure we fill in the array
				s32 path_length = 0;
				if (context->nm_services->get_alias_path(nm_measure_source_nanocube(context->source,nanocube_index), (u8) index, target->find_dive.path.alias, sizeof(path_buffer), path_buffer, &path_length)) {
					target->find_dive.path.array.begin  = path_buffer;
					target->find_dive.path.array.length = path_length;
				} else {
					// dead end: couldn't translate alias to path
					return nm_ERROR_INVALID_ALIAS;
				}
			}


			nx_Array      *path_array  = &target->find_dive.path.array;
			u8  depth = 0;

			nm_NanocubeIndex_FindDive it;
			nm_NanocubeIndex_FindDive_init(&it, nm_measure_source_index(context->source,nanocube_index), root, index, path_array, depth);
			nx_Node *target_node;
			while ((target_node = nm_NanocubeIndex_FindDive_next(&it))) {
				if (last_dimension) {
					nx_PNode* pnode = (nx_PNode*) target_node;

					// commit
					nm_RUN_AND_CHECK(
							 context->nm_services->append(context->table_values_handle, &pnode->payload_aggregate, nm_measure_source_nanocube(context->source,nanocube_index))
							);
					nm_RUN_AND_CHECK(
							 nm_TableKeys_commit_key (context->table_keys)
							);
				} else {
					nx_INode* inode = (nx_INode*) target_node;
					nx_Node* next_root = nx_Ptr_Node_get(&inode->content_p);

					// recursive
					nm_RUN_AND_CHECK(
						nm_Measure_solve_query(context, next_root, index + 1, nanocube_index)
					);
				}
			}
		}
	} else if (target->type == nm_TARGET_PATH_LIST) {
		Assert(target->anchor == 0);
		Assert(target->loop   == 0);

		nm_Path* *it_path = target->path_list.begin;
		while (it_path != target->path_list.end) {

			nm_Path *path = *it_path;
			if (path->by_alias) {
				// @todo make sure we fill in the array
				s32 path_length = 0;
				if (context->nm_services->get_alias_path(nm_measure_source_nanocube(context->source,nanocube_index), (u8) index, path->alias, sizeof(path_buffer), path_buffer, &path_length)) {
					path->array.begin  = path_buffer;
					path->array.length = path_length;
				} else {
					return nm_ERROR_INVALID_ALIAS;
				}
			}

			nx_Array *path_array  = &path->array;
			u8  depth = 0;

			nm_NanocubeIndex_FindDive it;
			nm_NanocubeIndex_FindDive_init(&it, nm_measure_source_index(context->source,nanocube_index), root, index, path_array, depth);
			nx_Node *target_node;
			while ((target_node = nm_NanocubeIndex_FindDive_next(&it))) {
				if (last_dimension) {
					nx_PNode* pnode = (nx_PNode*) target_node;

					// commit
					nm_RUN_AND_CHECK(
							 context->nm_services->append(context->table_values_handle, &pnode->payload_aggregate, nm_measure_source_nanocube(context->source,nanocube_index))
							);

					nm_RUN_AND_CHECK(
							 nm_TableKeys_commit_key(context->table_keys)
							);

				} else {
					nx_INode* inode = (nx_INode*) target_node;
					nx_Node* next_root = nx_Ptr_Node_get(&inode->content_p);

					nm_RUN_AND_CHECK(
							 nm_Measure_solve_query(context, next_root, index + 1, nanocube_index)
							);
				}
			}
			++it_path;
		}
	} else if (target->type == nm_TARGET_MASK) {
		Assert(target->anchor == 0);

		nm_Mask it;
		nm_Mask_init(&it, nm_measure_source_index(context->source,nanocube_index), root, index, target->mask);

		nx_Node *target_node;
		while ( (target_node = nm_Mask_next(&it) ) )
		{
			if (last_dimension) {
				nx_PNode* pnode = (nx_PNode*) target_node;

				// commit
				nm_RUN_AND_CHECK(
						 context->nm_services->append(context->table_values_handle, &pnode->payload_aggregate, nm_measure_source_nanocube(context->source,nanocube_index))
						);

				nm_RUN_AND_CHECK(
						 nm_TableKeys_commit_key(context->table_keys)
						);
			} else {
				nx_INode* inode = (nx_INode*) target_node;
				nx_Node* next_root = nx_Ptr_Node_get(&inode->content_p);

				// recursive
				nm_RUN_AND_CHECK(
					nm_Measure_solve_query(context, next_root, index + 1, nanocube_index)
				);
			}
		}
	} else if (target->type == nm_TARGET_TILE2D_RANGE) {
		Assert(target->anchor == 0);
		Assert(target->loop == 0);

		nm_Tile2D_Range_Iterator it;
		nm_Tile2D_Range_Iterator_init(&it, nm_measure_source_index(context->source,nanocube_index), root, index,
					      target->tile2d_range.z,
					      target->tile2d_range.x0,
					      target->tile2d_range.y0,
					      target->tile2d_range.x1,
					      target->tile2d_range.y1
					     );
		nx_Node *target_node;
		while ( (target_node = nm_Tile2D_Range_Iterator_next(&it) ) )
		{
			if (last_dimension) {
				nx_PNode* pnode = (nx_PNode*) target_node;

				// commit
				nm_RUN_AND_CHECK(
						 context->nm_services->append(context->table_values_handle, &pnode->payload_aggregate, nm_measure_source_nanocube(context->source,nanocube_index))
						);

				nm_RUN_AND_CHECK(
						 nm_TableKeys_commit_key(context->table_keys)
						);
			} else {
				nx_INode* inode = (nx_INode*) target_node;
				nx_Node* next_root = nx_Ptr_Node_get(&inode->content_p);

				// recursive
				nm_RUN_AND_CHECK(
					nm_Measure_solve_query(context, next_root, index + 1, nanocube_index)
				);
			}
		}

	} else if (target->type == nm_TARGET_INTERVAL_SEQUENCE || target->type == nm_TARGET_INTERVAL_SEQUENCE_AGGREGATE) {

		if (target->type == nm_TARGET_INTERVAL_SEQUENCE) {
			Assert(target->anchor == 0);
			Assert(target->loop == 1);
		} else {
			Assert(target->anchor == 0);
			Assert(target->loop == 0);
		}

		b8 loop = target->loop;

		s64 begin_it = target->interval_sequence.base;

		u8 depth = target->interval_sequence.depth;
		if (depth == 0) {
			depth = nm_measure_source_dim_levels(context->source,index);
		}

		NanocubeIndex_Interval it;
		for (u32 i=0;i<target->interval_sequence.count;++i) {
			u64 begin = (begin_it >= 0) ? (u64) begin_it : 0ull;
			u64 end   = begin + target->interval_sequence.width;

			NanocubeIndex_Interval_init(&it, nm_measure_source_index(context->source,nanocube_index), root, index, depth, begin, end);

			if (loop) {
				TableKeys_push_loop_column(context->table_keys, i);
			}

			nx_Node *target_node;

			while ((target_node = NanocubeIndex_Interval_next(&it))) {
				if (last_dimension) {
					nx_PNode* pnode = (nx_PNode*) target_node;

					// commit
					nm_RUN_AND_CHECK(
						context->nm_services->append(context->table_values_handle, &pnode->payload_aggregate, nm_measure_source_nanocube(context->source,nanocube_index))
					);
					nm_RUN_AND_CHECK(
						nm_TableKeys_commit_key (context->table_keys)
					);
				} else {
					nx_INode* inode = (nx_INode*) target_node;
					nx_Node* next_root = nx_Ptr_Node_get (&inode->content_p);

					nm_RUN_AND_CHECK(
						nm_Measure_solve_query(context, next_root, index + 1, nanocube_index)
					);
				}
			}

			begin_it += target->interval_sequence.stride;

			if (loop) {
				nm_TableKeys_pop_column(context->table_keys);
			}
		}

	} else if (target->type == nm_TARGET_TIME_SERIES || target->type == nm_TARGET_TIME_SERIES_AGGREGATE) {

		if (target->type == nm_TARGET_TIME_SERIES) {
			Assert(target->anchor == 0);
			Assert(target->loop == 1);
		} else {
			Assert(target->anchor == 0);
			Assert(target->loop == 0);
		}

		b8 loop = target->loop;

		/* check if we can align the base date requested */
		nm_TimeBinning time_binning;
		if (!context->nm_services->get_time_binning(nm_measure_source_nanocube(context->source,nanocube_index), (u8) index, &time_binning)) {
			return nm_ERROR_TIME_BIN_ALIGNMENT;
		}

		u64 query_time0       = target->time_sequence.base.time;
		u64 query_bin_secs    = target->time_sequence.width;
		u64 query_stride_secs = target->time_sequence.stride;
		s32 cumulative        = target->time_sequence.cumulative;

		u64 scheme_time0      = time_binning.base_time.time;
		u64 scheme_bin_secs   = time_binning.bin_width;

		if ((query_bin_secs % scheme_bin_secs) != 0) {
			return nm_ERROR_TIME_BIN_ALIGNMENT;
		}

		if ((query_stride_secs % scheme_bin_secs) != 0) {
			return nm_ERROR_TIME_BIN_ALIGNMENT;
		}

		s64 offset = (s64) query_time0 - (s64) scheme_time0;

		if ((offset % scheme_bin_secs) != 0) {
			return nm_ERROR_TIME_BIN_ALIGNMENT;
		}

		/* convert to interval query */
		s64 bin_width = (s64) query_bin_secs / (s64) scheme_bin_secs;
		s64 stride  = (s64) query_stride_secs / (s64) scheme_bin_secs;
		s64 base = offset / (s64) scheme_bin_secs;

		u8 depth = nm_measure_source_dim_levels(context->source, index);//*(context->source->levels.begin + index);

		s64 begin = base;
		s64 end   = base + bin_width;

		NanocubeIndex_Interval it;
		for (u32 i=0;i<target->time_sequence.count;++i) {

			// only consider positive intervals
			s64 clamped_begin = Max(begin, 0);
			s64 clamped_end   = Max(end, 0);
			if (clamped_begin < clamped_end) {

				NanocubeIndex_Interval_init(&it, nm_measure_source_index(context->source,nanocube_index), root, index, depth, (u64) clamped_begin, (u64) clamped_end);

				if (loop) {
					TableKeys_push_loop_column(context->table_keys, i);
				}

				nx_Node *target_node;

				while ((target_node = NanocubeIndex_Interval_next(&it))) {
					if (last_dimension) {
						nx_PNode* pnode = (nx_PNode*) target_node;

						// commit
						nm_RUN_AND_CHECK(
							context->nm_services->append(context->table_values_handle, &pnode->payload_aggregate, nm_measure_source_nanocube(context->source,nanocube_index))
						);
						nm_RUN_AND_CHECK(
							nm_TableKeys_commit_key(context->table_keys)
						);
					} else {
						nx_INode* inode = (nx_INode*) target_node;
						nx_Node* next_root = nx_Ptr_Node_get(&inode->content_p);

						// recursive
						nm_RUN_AND_CHECK(
							nm_Measure_solve_query(context, next_root, index + 1, nanocube_index)
						);
					}
				}

				if (loop) {
					nm_TableKeys_pop_column(context->table_keys);
				}
			}

			if (!cumulative)
				begin += stride;
			end += stride;
		}
	} else if (target->type == nm_TARGET_MONTH_SERIES) {

		Assert(target->anchor == 0);
		Assert(target->loop == 1);

		b8 loop = target->loop;

		/* check if we can align the base date requested */
		nm_TimeBinning time_binning;
		if (!context->nm_services->get_time_binning(nm_measure_source_nanocube(context->source,nanocube_index), (u8) index, &time_binning)) {
			return nm_ERROR_TIME_BIN_ALIGNMENT;
		}

		u64 query_time0         = target->time_sequence.base.time;
		u64 query_bin_months    = target->time_sequence.width;
		u64 query_stride_months = target->time_sequence.stride;
		s32 cumulative          = target->time_sequence.cumulative;

		u64 scheme_time0        = time_binning.base_time.time;
		u64 scheme_bin_secs     = time_binning.bin_width;

		// the scheme bin in second must be less than one day...
		s64 seconds_in_a_day = 24 * 60 * 60;
		if (scheme_bin_secs > seconds_in_a_day) {
			return nm_ERROR_TIME_BIN_ALIGNMENT;
		}

		// it also needs to be a divisor of a day in seconds
		if ((seconds_in_a_day % scheme_bin_secs) != 0) {
			return nm_ERROR_TIME_BIN_ALIGNMENT;
		}

		// bins in a day
		s64 scheme_bins_in_a_day = seconds_in_a_day / scheme_bin_secs;

		u8 depth = nm_measure_source_dim_levels(context->source, index);
		// u8 depth = *(context->source->levels.begin + index);

		tm_Label time_label_0 = { 0 };
		tm_Label_init(&time_label_0, (tm_Time) { .time = query_time0 });
		s32 year_0  = time_label_0.year;
		s32 month_offset_0 = time_label_0.month-1; // make it 0-based

		NanocubeIndex_Interval it;
		for (u32 i=0;i<target->time_sequence.count;++i) {

			tm_Time  t_start = { 0 };
			tm_Time  t_finish = { 0 };

			{ // init start
				s32 month_offset_i = month_offset_0 + i * query_stride_months;
				s32 year_i = year_0 + month_offset_i / 12;
				s32 month_i = (month_offset_i % 12) + 1;
				tm_Label label_start = {.year=year_i, .month=month_i, .day=1, .hour=0, .minute=0, .second=0, .offset_minutes=0, .timezone = 0};
				tm_Time_init_from_label(&t_start, &label_start);
			}

			{ // init finish
				s32 month_offset_i = month_offset_0 + i * query_stride_months + query_bin_months;
				s32 year_i = year_0 + month_offset_i / 12;
				s32 month_i = (month_offset_i % 12) + 1;
				tm_Label label_finish= {.year=year_i, .month=month_i, .day=1, .hour=0, .minute=0, .second=0, .offset_minutes=0, .timezone = 0};
				tm_Time_init_from_label(&t_finish, &label_finish);
			}

			// not it is time to find out the interval, and if it is needed
			s64 bin_start  = (t_start.time - scheme_time0) / scheme_bin_secs;
			s64 bin_finish = (t_finish.time - scheme_time0) / scheme_bin_secs;

			// clamp to the actual valid interval
			bin_start  = Max(0,bin_start);
			bin_finish = Max(0,bin_finish);

			if (bin_start == bin_finish || bin_finish == 0) {
				continue;
			}

			NanocubeIndex_Interval_init(&it, nm_measure_source_index(context->source,nanocube_index), root, index, depth, (u64) bin_start, (u64) bin_finish);

			TableKeys_push_loop_column(context->table_keys, i);

			nx_Node *target_node;

			while ((target_node = NanocubeIndex_Interval_next(&it))) {
				if (last_dimension) {
					nx_PNode* pnode = (nx_PNode*) target_node;

					// commit
					nm_RUN_AND_CHECK(
							 context->nm_services->append(context->table_values_handle, &pnode->payload_aggregate, nm_measure_source_nanocube(context->source,nanocube_index))
							);
					nm_RUN_AND_CHECK(
							 nm_TableKeys_commit_key(context->table_keys)
							);
				} else {
					nx_INode* inode = (nx_INode*) target_node;
					nx_Node* next_root = nx_Ptr_Node_get(&inode->content_p);

					// recursive
					nm_RUN_AND_CHECK(
							 nm_Measure_solve_query(context, next_root, index + 1, nanocube_index)
							);
				}
			}

			nm_TableKeys_pop_column(context->table_keys);

		} // loop on the intervals
	} else {
		Assert(0 && "Case not implemented!");
	}

	return nm_OK; // or zero

}


static nm_MeasureEvalExpressionResult
nm_Measure_eval_table_op_table_coarser(nm_MeasureEvalContext *context,
				       nm_TableKeysType_Alignment *alignment,
				       nm_MeasureEvalExpressionResult left,
				       nm_MeasureEvalExpressionResult right,
				       nm_MeasureExpression_Operation_Type op)
{
	Assert(left.is_table && right.is_table);
	Assert(alignment->relation.is_coarser);

	nm_MeasureEvalExpressionResult result;
	result.flags = 0;

	//
	// find permutation of left relation
	// to match the right relation
	//
	nm_Permutation col_perm;
	col_perm.begin = (u32*) LinearAllocator_alloc(context->memsrc, alignment->c2 * sizeof(u32));
	col_perm.end   = col_perm.begin + alignment->c2;

	/* figure out permutation of columns in table1 */
	LinearAllocatorCheckpoint cp = LinearAllocator_checkpoint(context->memsrc);
	char *flags = LinearAllocator_alloc(context->memsrc, alignment->c2 * sizeof(b8));
	pt_fill(flags, flags + alignment->c2, 0);
	u32 *it = col_perm.begin;
	for (u32 i=0;i<alignment->c1;++i) {
		u8 j = (alignment->mapping1 + i)->corresponding_index;
		*(flags + j) = 1;
		*it = j;
		++it;
	}
	for (u32 i=0;i<alignment->c2;++i) {
		if (*(flags + i) == 0) {
			*it = i;
			++it;
		}
	}
	LinearAllocator_rewind(context->memsrc, cp);


	/* copy table1 with permuted columns */
	nm_Table *result_table = (nm_Table*) LinearAllocator_alloc(context->memsrc, sizeof(nm_Table));
	/* result table source is the same as the left one */
	result_table->source = left.table->source;
	nm_TableKeys_copy_to(&right.table->table_keys, &result_table->table_keys, context->memsrc);
	nm_TableKeys_permute_columns(&result_table->table_keys, &col_perm, context->memsrc);
	nm_Permutation* row_permutation = nm_TableKeys_order(&result_table->table_keys, context->memsrc);
	nm_TableKeys_permute_rows(&result_table->table_keys, row_permutation,
				  context->memsrc);
	result_table->table_values_handle = context->nm_services->copy(right.table->table_values_handle,
								       row_permutation,
								       context->allocation_context);

	// find permutation of rows on the keys
	// assumption all keys are different
	u32 len_result = result_table->table_keys.row_length;
	u32 len_left = left.table->table_keys.row_length;

	char *it_result = result_table->table_keys.keys.begin;
	char *it_left = left.table->table_keys.keys.begin;

	u32 result_index = 0;
	u32 left_index   = 0;

	u32 result_rows = result_table->table_keys.rows;
	u32 left_rows = left.table->table_keys.rows;

	while (result_index < result_rows
	       && left_index < left_rows) {

		s32 sign = 0; // -1 result is less than, 0 match, 1 result is larger than

		// compare the first c2 keys for a match
		char *col_result_value = it_result;
		char *col_left_value   = it_left;
		for (u32 i=0;i<alignment->c1;++i) {
			// check if columns are compatible
			nm_TableKeysColumnType *result_col = result_table->table_keys.type->begin + i;
			nm_TableKeysColumnType *left_col   = left.table->table_keys.type->begin + i;

			Assert(result_col->loop_column == left_col->loop_column);

			// check if all values are the same
			if (result_col->loop_column) {
				u32 a = *((u32*) col_result_value);
				u32 b = *((u32*) col_left_value);
				if (a < b) sign = -1;
				else if (a > b) sign = 1;
			} else {
				//                             u32 levels_result = result_col->levels;
				//                             u32 levels_left   = left_col->levels;
				for (u32 j=0;j<left_col->levels;++j) {
					u8 lbl_result_j = 0;
					u8 lbl_left_j   = 0;
					pt_read_bits2(col_result_value, ((result_col->levels - 1 - j) * result_col->bits), result_col->bits, (char*) &lbl_result_j);
					pt_read_bits2(col_left_value, ((left_col->levels - 1 - j) * left_col->bits), left_col->bits, (char*) &lbl_left_j);
					if (lbl_result_j < lbl_left_j) {
						sign = -1;
						break;
					} else if (lbl_result_j > lbl_left_j) {
						sign = 1;
						break;
					}
				}
			}

			if (sign != 0)
				break;

			col_result_value +=
				nm_TableKeysColumnType_bytes(result_col);
			col_left_value +=
				nm_TableKeysColumnType_bytes(left_col);
		}

		if (sign < 0) {
			context->nm_services->cant_combine_entry(result_table->table_values_handle,
									   result_index, op, 1);
			++result_index;
			it_result += len_result;
		} else if (sign > 0) {
			it_left += len_left;
			++left_index;
		} else {
			// match! merge the current summary on the result
			// table with the on on the right table
			context->nm_services-> combine_entry(result_table->table_values_handle,
								       result_index,
								       left.table->table_values_handle,
								       left_index, op, 1);
			it_result += len_result;
			++result_index;
		}
	}

	// finer rows with no match on coarser table
	while (result_index < result_rows) {
		context->nm_services->
			cant_combine_entry
			(result_table->table_values_handle,
			 result_index, op, 1);
		++result_index;
		it_result += len_result;
	}


	// iterate through result_table and right table
	// assuming the first alignment->c2 columns are all the same
	// MAX_NUM_DIMENSIONS

	result.is_table = 1;
	result.table = result_table;
	return result;

}


static nm_MeasureEvalExpressionResult
nm_Measure_eval_table_op_table_finer(nm_MeasureEvalContext *context,
				     nm_TableKeysType_Alignment *alignment,
				     nm_MeasureEvalExpressionResult left,
				     nm_MeasureEvalExpressionResult right,
				     nm_MeasureExpression_Operation_Type op)
{
	Assert(left.is_table && right.is_table);
	Assert(alignment->relation.is_finer);

	nm_MeasureEvalExpressionResult result;
	result.flags = 0;

	//
	// find permutation of left relation
	// to match the right relation
	//
	nm_Permutation col_perm;
	col_perm.begin = (u32*) LinearAllocator_alloc (context->memsrc, alignment->c1 * sizeof(u32));
	col_perm.end = col_perm.begin + alignment->c1;

	/* figure out permutation of columns in table1 */
	LinearAllocatorCheckpoint cp = LinearAllocator_checkpoint(context->memsrc);
	char *flags = LinearAllocator_alloc (context->memsrc, alignment->c1 * sizeof(b8));
	pt_fill(flags, flags + alignment->c1,
		0);
	u32 *it = col_perm.begin;
	for (u32 i=0;i<alignment->c2;++i) {
		u8 j = (alignment->mapping2 + i)->corresponding_index;
		*(flags + j) = 1;
		*it = j;
		++it;
	}
	for (u32 i=0;i<alignment->c1;++i) {
		if (*(flags + i) == 0) {
			*it = i;
			++it;
		}
	}
	LinearAllocator_rewind(context->memsrc, cp);
	/* end of: figure out permutation of columns in table1 */

	// copy table1 with permuted columns
	nm_Table *result_table = (nm_Table*) LinearAllocator_alloc(context->memsrc, sizeof(nm_Table));
	/* result table source is the same as the left one */
	result_table->source = left.table->source;
	nm_TableKeys_copy_to(&left.table->table_keys, &result_table->table_keys, context->memsrc);
	nm_TableKeys_permute_columns(&result_table->table_keys, &col_perm, context->memsrc);
	nm_Permutation* row_permutation = nm_TableKeys_order(&result_table->table_keys, context->memsrc);
	nm_TableKeys_permute_rows(&result_table->table_keys, row_permutation, context->memsrc);
	result_table->table_values_handle = context->nm_services->copy(left.table->table_values_handle,
								       row_permutation, context->allocation_context);

	// find permutation of rows on the keys
	// assumption all keys are different
	u32 len_result   = result_table->table_keys.row_length;
	u32 len_right    = right.table->table_keys.row_length;
	char *it_result  = result_table->table_keys.keys.begin;
	char *it_right   = right.table->table_keys.keys.begin;

	u32 result_index = 0;
	u32 right_index  = 0;
	u32 result_rows  = result_table->table_keys.rows;
	u32 right_rows   = right.table->table_keys.rows;

	while (result_index < result_rows && right_index < right_rows) {

		s32 sign = 0; // -1 result is less than, 0 match, 1 result is larger than

		// compare the first c2 keys for a match
		char *col_result_value = it_result;
		char *col_right_value  = it_right;
		for (u32 i=0;i<alignment->c2;++i) {
			// check if columns are compatible
			nm_TableKeysColumnType *result_col =
				result_table->table_keys.type->begin + i;
			nm_TableKeysColumnType *right_col  =
				right.table->table_keys.type->begin + i;

			Assert(result_col->loop_column == right_col->loop_column);

			// check if all values are the same
			if (result_col->loop_column) {
				u32 a = *((u32*) col_result_value);
				u32 b = *((u32*) col_right_value);
				if (a < b) sign = -1;
				else if (a > b) sign = 1;
			} else {
				//                             u32 levels_result = result_col->levels;
				//                             u32 levels_right  = right_col->levels;
				for (u32 j=0;j<right_col->levels;++j)
				{
					u8 lbl_result_j = 0;
					u8 lbl_right_j  = 0;
					pt_read_bits2(col_result_value, (result_col->levels - 1 - j) * result_col->bits, result_col->bits, (char*) &lbl_result_j);
					pt_read_bits2(col_right_value, (right_col->levels - 1 - j) * right_col->bits, right_col->bits, (char*) &lbl_right_j);
					if (lbl_result_j < lbl_right_j) {
						sign = -1;
						break;
					}
					else if (lbl_result_j > lbl_right_j) {
						sign = 1;
						break;
					}
				}
			}

			if (sign != 0)
				break;

			col_result_value +=
				nm_TableKeysColumnType_bytes(result_col);
			col_right_value  +=
				nm_TableKeysColumnType_bytes(right_col);
		}

		if (sign < 0)
		{
			context->nm_services->cant_combine_entry
				(result_table->table_values_handle,
				 result_index, op, 0);
			++result_index;
			it_result += len_result;
		} else if (sign > 0) {
			it_right  += len_right;
			++right_index;
		} else {
			// match! merge the current summary on the result
			// table with the on on the right table
			context->nm_services->combine_entry
				(result_table->table_values_handle,
				 result_index,
				 right.table->table_values_handle,
				 right_index,
				 op,
				 0);
			it_result += len_result;
			++result_index;
		}
	}

	while (result_index < result_rows) {
		context->nm_services->cant_combine_entry
			(result_table->table_values_handle,
			 result_index,
			 op,
			 0);
		++result_index;
		it_result += len_result;
	}


	// iterate through result_table and right table
	// assuming the first alignment->c2 columns are all the same
	// MAX_NUM_DIMENSIONS

	result.is_table = 1;
	result.table = result_table;
	return result;
}


static nm_MeasureEvalExpressionResult
nm_Measure_eval_table_op_table_equal(nm_MeasureEvalContext *context,
				     nm_TableKeysType_Alignment *alignment,
				     nm_MeasureEvalExpressionResult left,
				     nm_MeasureEvalExpressionResult right,
				     nm_MeasureExpression_Operation_Type op)
{
	Assert(left.is_table && right.is_table);
	Assert(alignment->relation.is_equal);

	// nm_MeasureEvalExpressionResult result;
	// result.flags = 0;

	// special case where there are no columns in either
	// relations

	// create an empty table
	nm_Table *result_table = (nm_Table*) LinearAllocator_alloc(context->memsrc, sizeof(nm_Table));
	/* result table source is the same as the left one */
	result_table->source = left.table->source;
	nm_TableKeysType *result_table_type = nm_TableKeysType_copy(left.table->table_keys.type, context->memsrc);
	nm_TableKeys_init(&result_table->table_keys, result_table_type, context->memsrc);
	/* create an empty values table with the same format as left */
	result_table->table_values_handle = context->nm_services->copy_format(left.table->table_values_handle, context->allocation_context);

	// run a merge loop
	// assume both tables are sorted by key
	u32 len_left    = left.table->table_keys.row_length;
	u32 len_right   = right.table->table_keys.row_length;
	char *it_left   = left.table->table_keys.keys.begin;
	char *it_right  = right.table->table_keys.keys.begin;

	u32 left_index  = 0;
	u32 right_index = 0;
	u32 left_rows   = left.table->table_keys.rows;
	u32 right_rows  = right.table->table_keys.rows;

	while (left_index < left_rows && right_index < right_rows) {
		s32 sign = 0; // -1 left is less than, 0 match, 1 left is larger than

		// compare the first c2 keys for a match
		char *col_left_value  = it_left;
		char *col_right_value = it_right;
		for (u32 i=0;i<left.table->table_keys.columns;++i) {
			// check if columns are compatible
			nm_TableKeysColumnType *left_col  = left.table->table_keys.type->begin  + i;
			nm_TableKeysColumnType *right_col = right.table->table_keys.type->begin + i;

			Assert(left_col->loop_column == right_col->loop_column);

			// check if all values are the same
			if (left_col->loop_column) {
				u32 a = *((u32*) col_left_value);
				u32 b = *((u32*) col_right_value);
				if (a < b) sign = -1;
				else if (a > b) sign = 1;
			} else {
				//                             u32 levels_left  = left_col->levels;
				//                             u32 levels_right = right_col->levels;
				for (u32 j=0;j<right_col->levels;++j) {
					u8 lbl_left_j  = 0;
					u8 lbl_right_j = 0;
					pt_read_bits2(col_left_value,  (left_col->levels - 1 - j) * left_col->bits,   left_col->bits,  (char*) &lbl_left_j);
					pt_read_bits2(col_right_value, (right_col->levels - 1 - j) * right_col->bits, right_col->bits, (char*) &lbl_right_j);
					if (lbl_left_j < lbl_right_j) {
						sign = -1;
						break;
					} else if (lbl_left_j > lbl_right_j) {
						sign = 1;
						break;
					}
				}
			}

			if (sign != 0)
				break;

			col_left_value  += nm_TableKeysColumnType_bytes(left_col);
			col_right_value += nm_TableKeysColumnType_bytes(right_col);
		}

		if (sign < 0) {

			nm_TableKeys_append_row_copying(&result_table->table_keys, &left.table->table_keys, left_index);
			context->nm_services->append_copying(result_table->table_values_handle,
							     left.table->table_values_handle, left_index);

			context->nm_services->cant_combine_entry(result_table->table_values_handle,
				 result_table->table_keys.rows - 1, op, 0);

			++left_index;
			it_left += len_left;

		} else if (sign > 0) {

			nm_TableKeys_append_row_copying(&result_table->table_keys, &right.table->table_keys,
							right_index);
			context->nm_services->append_copying(result_table->table_values_handle,
							     right.table->table_values_handle, right_index);
			context->nm_services->cant_combine_entry(result_table->table_values_handle,
								 result_table->table_keys.rows - 1, op, 1);

			++right_index;
			it_right += len_right;
		} else {
			nm_TableKeys_append_row_copying(&result_table->table_keys,
							&left.table->table_keys, left_index);
			context->nm_services->append_copying
				(result_table->table_values_handle,
				 left.table->table_values_handle,
				 left_index);

			// match! merge the current summary on the left
			// table with the on on the right table
			context->nm_services->combine_entry
				(result_table->table_values_handle,
				 result_table->table_keys.rows-1,  // last row
				 right.table->table_values_handle,
				 right_index, op, 0);

			it_left += len_left;
			++left_index;

			it_right += len_right;
			++right_index;
		}
	}

	if (left_index < left_rows) {
		while (left_index < left_rows) {
			nm_TableKeys_append_row_copying
				(&result_table->table_keys,
				 &left.table->table_keys, left_index);
			context->nm_services->append_copying
				(result_table->table_values_handle,
				 left.table->table_values_handle,
				 left_index);
			context->nm_services->cant_combine_entry
				(result_table->table_values_handle,
				 result_table->table_keys.rows - 1,
				 op,
				 0);
			++left_index;
			it_left += len_left;
		}
	} else if (right_index < right_rows) {
		while (right_index < right_rows) {
			nm_TableKeys_append_row_copying
				(&result_table->table_keys,
				 &right.table->table_keys,
				 right_index);
			context->nm_services->
				append_copying
				(result_table->table_values_handle,
				 right.table->table_values_handle,
				 right_index);
			context->nm_services->cant_combine_entry
				(result_table->table_values_handle,
				 result_table->table_keys.rows - 1,
				 op,
				 1);
			++right_index;
			it_right += len_right;
		}
	}

	// iterate through left_table and right table
	// assuming the first alignment->c2 columns are all the same
	// MAX_NUM_DIMENSIONS

	left.is_table = 1;
	left.table = result_table;
	return left;
}

static nm_MeasureEvalExpressionResult
nm_Measure_eval_expression(nm_MeasureEvalContext *context, nm_MeasureExpression *expression)
{
	nm_MeasureEvalExpressionResult result;
	result.flags = 0;

	if (expression->is_source) {
		result.is_table = 1;
		result.table = context->table_begin + expression->source_index;
		return result;
	} else if (expression->is_number) {
		result.is_number = 1;
		result.number = expression->number;
		return result;
	} else if (expression->is_binary_op) {
		nm_MeasureEvalExpressionResult left  = nm_Measure_eval_expression(context, expression->op.left);
		nm_MeasureEvalExpressionResult right = nm_Measure_eval_expression(context, expression->op.right);

		if (left.error || right.error) {
			result.error = 1;
			return result;
		}

		if (left.is_table && right.is_table) {
			nm_TableKeysType_Alignment *alignment = nm_TableKeysType_align(left.table->table_keys.type,
										       right.table->table_keys.type,
										       context->memsrc);
			if (alignment->relation.is_different) {
				result.error = 1;
				return result;
			} else if (alignment->relation.is_coarser) {
				return nm_Measure_eval_table_op_table_coarser(context, alignment, left, right, expression->op.type);
			} else if (alignment->relation.is_finer) {
				return nm_Measure_eval_table_op_table_finer(context, alignment, left, right, expression->op.type);
			} else if (alignment->relation.is_equal) {
				return nm_Measure_eval_table_op_table_equal(context, alignment, left, right, expression->op.type);
			}
		} else if (left.is_table && right.is_number) {
			// copy table1 with permuted columns
			nm_Table *result_table = (nm_Table*) LinearAllocator_alloc (context->memsrc, sizeof(nm_Table));
			// result table source is the same as the left table
			result_table->source = left.table->source;
			nm_TableKeys_copy_to (&left.table->table_keys, &result_table->table_keys, context->memsrc);
			result_table->table_values_handle = context->nm_services->copy(left.table->table_values_handle, 0,
										       context->allocation_context);

			context->nm_services->combine_number(result_table->table_values_handle, right.number, expression->op.type, 1);

			result.is_table = 1;
			result.table = result_table;
			return result;
		} else if (left.is_number && right.is_table) {
			// copy table1 with permuted columns
			nm_Table *result_table = (nm_Table*) LinearAllocator_alloc(context->memsrc, sizeof(nm_Table));
			// result table source is the same as the right table
			result_table->source = right.table->source;
			nm_TableKeys_copy_to (&right.table->table_keys, &result_table->table_keys, context->memsrc);
			result_table->table_values_handle = context->nm_services->copy(right.table->table_values_handle,
										       0, context->allocation_context);
			context->nm_services->combine_number(result_table->table_values_handle, left.number, expression->op.type, 0);

			result.is_table = 1;
			result.table = result_table;
			return result;
		}
	}
	result.error = 1;
	return result;
}


// @todo: bring in a log and allow for an error flow here
static nm_Table*
nm_Measure_eval(nm_Measure *self, LinearAllocator *memsrc, nm_Services *nm_services, void *allocation_context, s32 *error)
{
	*error = nm_OK; // no error at first

	// LinearAllocatorCheckpoint chkpt = LinearAllocator_checkpoint(memsrc);

	LinearAllocator memsrc_scratch;
	LinearAllocator memsrc_tables;
	s64 capacity = memsrc->capacity - memsrc->end;
	char* middle = memsrc->end + capacity / 2;
	LinearAllocator_init(&memsrc_tables, memsrc->end, memsrc->end, middle);
	LinearAllocator_init(&memsrc_scratch, middle, middle, memsrc->capacity);

	//
	// create table for each base query
	//

	nm_Target targets[nx_MAX_NUM_DIMENSIONS];

	nm_Table *begin = (nm_Table*) LinearAllocator_alloc(&memsrc_tables, sizeof(nm_Table) * self->num_sources);
	nm_Table *end   = begin + self->num_sources;

	nm_MeasureSolveQueryData query_context;

	for (s32 i=0;i<self->num_sources;++i)
	{
		nm_MeasureSource *source = self->sources[i];
		nm_MeasureSourceBinding *bindings = self->bindings[i];
		void *payload_config = self->payload_config[i];

		// prepare table
		nm_Table     *table      = begin + i;
		// associate table with source (note that the same table might have
		// multiple indices but we assume the same schema for all the indices)
		table->source = source;
		nm_TableKeys     *table_keys      = &table->table_keys;
		nm_TableKeysType *table_keys_type = nm_TableKeysType_from_measure_source_and_bindings(&memsrc_tables, source, bindings, nm_services, error);
		if (!table_keys_type) {
			return 0;
		}

		// memsrc shouldn't be touched until we finish
		// inserting all records into table_keys
		nm_TableKeys_init(table_keys, table_keys_type, &memsrc_tables);

		// @todo(llins): this is error prone
		Assert(source->num_nanocubes > 0);
		table->table_values_handle = nm_services->create(nm_measure_source_nanocube(source,0), payload_config, allocation_context);

		u8 num_dimensions = source->num_dimensions;

		// clear all the targets
		for (s32 j=0;j<num_dimensions;++j) {
			nm_Target_init(targets + j);
		}

		nm_MeasureSourceBinding *it = self->bindings[i];
		while (it) {
			targets[it->dimension] = it->target;
			it = it->next;
		}

		// @todo(llins): here is where we need to add all the results
		query_context.source              = source;
		query_context.target_begin        = targets;
		query_context.target_end          = targets + num_dimensions;
		query_context.table_keys          = table_keys;
		query_context.table_values_handle = table->table_values_handle;
		query_context.nm_services         = nm_services;

		for (s32 j=0;j<source->num_nanocubes;++j) {
			//nx_Node* root = nx_Ptr_Node_get(&  source->indices[j] ->root_p);
			nx_Node* root = nx_Ptr_Node_get(&nm_measure_source_index(source,j)->root_p);

			if (!root) {
				// empty cube
				continue;
			}

			// @verify did this come before multiple sources?
			// Assert(table_keys->current_column == 0 && table_keys->current_offset == 0);

			/* solve query for all indices within the measure source */
			*error = nm_Measure_solve_query(&query_context, root, 0, j);
			if (*error) { return 0; }

			// compress table
			if (table_keys->rows > 1)
			{
				LinearAllocator_clear(&memsrc_scratch);
				nm_Permutation* permutation = nm_TableKeys_order(table_keys, &memsrc_scratch);
				MemoryBlock repeat_flags = nm_TableKeys_row_repeat_flags(table_keys, permutation, &memsrc_scratch);
				nm_TableKeys_permute_rows_with_repetition(table_keys, permutation, repeat_flags);

#if 0
				/* check if keys are in order and there are no repetitions */
				Assert(nm_TableKeys_is_totally_ordered(table_keys));
#endif
				nm_services->pack(table->table_values_handle, permutation, repeat_flags);

				LinearAllocator_clear(&memsrc_scratch);
			}
			// clear scratch memsrc
		}
		table_keys->memsrc = 0; // non editable
	}

	// back to one linear allocator
	memsrc->end = memsrc_tables.end;

	// time to combine the tables
	nm_MeasureEvalContext eval_context;
	eval_context.measure     = self;
	eval_context.memsrc      = memsrc;
	eval_context.nm_services = nm_services;
	eval_context.table_begin = begin;
	eval_context.table_end   = end;
	eval_context.allocation_context = allocation_context;

	nm_MeasureEvalExpressionResult result = nm_Measure_eval_expression(&eval_context, self->expression);

	Assert(result.is_table);
	return result.table;

}



