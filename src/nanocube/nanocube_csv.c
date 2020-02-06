/*
 * cm_ indicates the csv-mapping language
 *
 * dependencies
 *     np_ parser
 *     nt_ tokenizer
 *     tm_ time
 *     ntp_ time parser
 */

typedef enum {
	cm_INDEX_DIMENSION,
	cm_MEASURE_DIMENSION
} cm_DimensionType;

// @TODO
// these names are in the cvs to nanocube mapping
// language
static char *cm_number_storage_names[] = {"s32", "s64", "u32", "u64", "f32", "f64" };

typedef struct {
	u32 is_index:1;
	u32 is_name:1;
	u32 index:30;
	MemoryBlock name;
} cm_ColumnRef;

static void
cm_ColumnRef_init(cm_ColumnRef *self, MemoryBlock *name)
{
	self->is_index = 0;
	self->is_name  = 1;
	self->index    = 0;
	self->name = *name;
}

static void
cm_ColumnRef_init_index(cm_ColumnRef *self, u32 index)
{
	self->is_index = 1;
	self->is_name  = 0;
	self->index    = index;
	self->name     = (MemoryBlock) { 0 };
}

typedef struct {
	cm_ColumnRef *begin;
	cm_ColumnRef *end;
} cm_ColumnRefArray;

typedef enum {
	cm_INDEX_MAPPING_LATLON_MERCATOR_QUADTREE,
	cm_INDEX_MAPPING_TIME,
	cm_INDEX_MAPPING_WEEKDAY,
	cm_INDEX_MAPPING_HOUR,
	cm_INDEX_MAPPING_CATEGORICAL,
	cm_INDEX_MAPPING_IP_HILBERT,
	cm_INDEX_MAPPING_XY_QUADTREE,
	cm_INDEX_MAPPING_NUMERICAL
} cm_IndexMappingType;

typedef enum {
	/* assume input fields are scalars (can repeat those) and multiply those */
	cm_MEASURE_MAPPING_PRODUCT,
	/* assumes two input fields containing calendar dates copute the difference */
	cm_MEASURE_MAPPING_ROW_BITSET,
	cm_MEASURE_MAPPING_TIME_DURATION,
	cm_MEASURE_MAPPING_TIME_DURATION_SQUARED
} cm_MeasureMappingType;

typedef enum {
	cm_INDEX_DIMENSION_MAPPING,
	cm_MEASURE_DIMENSION_MAPPING
} cm_MappingType;

#define cm_TIME_INPUT_TYPE_DATE      0
#define cm_TIME_INPUT_TYPE_UNIX_TIME 1
#define cm_TIME_INPUT_TYPE_BIN       2

typedef struct {
	cm_MappingType mapping_type;
	union {
		struct {
			cm_IndexMappingType type;
			struct {
				u8  depth;
				b8  top_down;
			} xy;
			struct {
				u8  bits;
				nm_Numerical spec;
			} numerical;
			struct {
				u8 depth;
			} latlon;
			struct {
				u8 depth;
			} ip_hilbert;
			struct {
				u8	       depth;
				nm_TimeBinning time_binning;
				/*
				 * when reading a date add this number of minutes to it.
				 * Example, the nytaxi dataset records show local time,
				 * (-05:00), but since no timezone tag is available, we
				 * read those datetimes as UTC. To fix that in this case
				 * we should add 5 * 60 minutes.
				 */
				s64            minutes_to_add;
				u8             time_input_type;
			} time;
			struct {
				/* one level of bits: values from 0 to 2^bits - 1 */
				u8          bits;
				u8          levels;
				b8          is_file; // indicate the the content of labels_table is actually a filename
				MemoryBlock labels_table; /* if empty */
			} categorical;
		} index_mapping;
		struct {
			cm_MeasureMappingType type;
			nv_NumberStorage      storage_type;
			/* time duration unit in seconds */
			u64                   time_unit_in_seconds;
		} measure_mapping;
	};
} cm_Mapping;

typedef struct {
	cm_DimensionType   type;
	MemoryBlock        name;
	cm_ColumnRefArray  input_columns;
	cm_Mapping         mapping_spec;
	void               *user_data;
} cm_Dimension;

// CVS to Schema Mapping Specification
typedef struct {
	cm_Dimension *index_dimensions[nx_MAX_NUM_DIMENSIONS];
	cm_Dimension *measure_dimensions[nx_MAX_NUM_DIMENSIONS];
	u8            num_index_dimensions;
	u8            num_measure_dimensions;
} cm_Spec;

static void
cm_Spec_init(cm_Spec *self)
{
	pt_fill((char*) self, (char*) self + sizeof(cm_Spec), 0);
	self->num_index_dimensions = 0;
	self->num_measure_dimensions = 0;
}

static b8
cm_Spec_is_valid(cm_Spec *self)
{
	return (self->num_index_dimensions > 0
		&& self->num_measure_dimensions > 0);
}

static s32
cm_Spec_dimensions(cm_Spec *self)
{
	return (s32) self->num_index_dimensions
		+ (s32) self->num_measure_dimensions;
}

static cm_Dimension*
cm_Spec_get_dimension(cm_Spec *self, s32 index)
{
	Assert(index < cm_Spec_dimensions(self));
	if (index >= self->num_index_dimensions) {
		return self->measure_dimensions[index -
			self->num_index_dimensions];
	}
	else {
		return self->index_dimensions[index];
	}
}

// @TODO(llins): check for repeated dimension names and report
// an error if same dimension name
static void
cm_Spec_insert_dimension(cm_Spec *self, cm_Dimension *dim)
{
	if (dim->type == cm_INDEX_DIMENSION) {
		Assert(self->num_index_dimensions < nx_MAX_NUM_DIMENSIONS);
		self->index_dimensions[self->num_index_dimensions] = dim;
		++self->num_index_dimensions;
	}
	else if (dim->type == cm_MEASURE_DIMENSION) {
		Assert(self->num_measure_dimensions < nx_MAX_NUM_DIMENSIONS);
		self->measure_dimensions[self->num_measure_dimensions] = dim;
		++self->num_measure_dimensions;
	}
}

static void
cm_Mapping_numerical(cm_Mapping *self, u8 bits, f64 a, f64 b, s32 to_int_method_code)
{
	self[0] = (cm_Mapping) { 0 };
	self->mapping_type	           = cm_INDEX_DIMENSION_MAPPING;
	self->index_mapping.type           = cm_INDEX_MAPPING_NUMERICAL;
	self->index_mapping.numerical.bits = bits;
	self->index_mapping.numerical.spec = (nm_Numerical) {
		.a = a,
		.b = b,
		.to_int_method = to_int_method_code
	};
}

static void
cm_Mapping_latlon(cm_Mapping *self, u8 depth)
{
	pt_fill((char*) self, (char*) self + sizeof(cm_Mapping), 0);
	self->mapping_type	         = cm_INDEX_DIMENSION_MAPPING;
	self->index_mapping.type         = cm_INDEX_MAPPING_LATLON_MERCATOR_QUADTREE;
	self->index_mapping.latlon.depth = depth;
}

static void
cm_Mapping_xy(cm_Mapping *self, u8 depth, b8 top_down)
{
	pt_fill((char*) self, (char*) self + sizeof(cm_Mapping), 0);
	self->mapping_type	         = cm_INDEX_DIMENSION_MAPPING;
	self->index_mapping.type         = cm_INDEX_MAPPING_XY_QUADTREE;
	self->index_mapping.xy.depth     = depth;
	self->index_mapping.xy.top_down  = top_down;
}

static void
cm_Mapping_ip_hilbert(cm_Mapping *self, u8 depth)
{
	pt_fill((char*) self, (char*) self + sizeof(cm_Mapping), 0);
	self->mapping_type	             = cm_INDEX_DIMENSION_MAPPING;
	self->index_mapping.type             = cm_INDEX_MAPPING_IP_HILBERT;
	self->index_mapping.ip_hilbert.depth = depth;
}

static void
cm_Mapping_categorical(cm_Mapping *self, u8 bits, u8 levels, MemoryBlock labels_table, b8 is_file)
{
	pt_fill((char*) self, (char*) self + sizeof(cm_Mapping), 0);

	self->mapping_type = cm_INDEX_DIMENSION_MAPPING;

	self->index_mapping.type = cm_INDEX_MAPPING_CATEGORICAL;
	self->index_mapping.categorical.bits = bits;
	self->index_mapping.categorical.levels = levels;
	self->index_mapping.categorical.labels_table = labels_table;
	self->index_mapping.categorical.is_file = is_file;
}

static b8
cm_Mapping_categorical_defined_with_labels_table(cm_Mapping *self)
{
	return self->index_mapping.categorical.labels_table.begin != 0;
}

static void
cm_Mapping_hour(cm_Mapping *self)
{
	pt_fill((char*) self, (char*) self + sizeof(cm_Mapping), 0);

	self->mapping_type       = cm_INDEX_DIMENSION_MAPPING;

	self->index_mapping.type = cm_INDEX_MAPPING_HOUR;
}

static void
cm_Mapping_weekday(cm_Mapping *self)
{
	pt_fill((char*) self, (char*) self + sizeof(cm_Mapping), 0);

	self->mapping_type       = cm_INDEX_DIMENSION_MAPPING;

	self->index_mapping.type = cm_INDEX_MAPPING_WEEKDAY;
}

static void
cm_Mapping_time(cm_Mapping *self, u8 depth, tm_Time base_time,
		s64 default_minute_offset, u64 bin_width, s64 minutes_to_add, u8 time_input_type)
{
	pt_fill((char*) self, (char*) self + sizeof(cm_Mapping), 0);

	self->mapping_type                      = cm_INDEX_DIMENSION_MAPPING;

	self->index_mapping.type	        = cm_INDEX_MAPPING_TIME;
	self->index_mapping.time.depth          = depth;
	self->index_mapping.time.minutes_to_add = minutes_to_add;
	self->index_mapping.time.time_input_type= time_input_type;

	nm_TimeBinning *time_binning = &self->index_mapping.time.time_binning;
	nm_TimeBinning_init(time_binning, base_time, default_minute_offset, bin_width);
}

static void
cm_Mapping_product(cm_Mapping *self, nv_NumberStorage storage_type)
{
	pt_fill((char*) self, (char*) self + sizeof(cm_Mapping), 0);

	self->mapping_type                      = cm_MEASURE_DIMENSION_MAPPING;

	self->measure_mapping.type	        = cm_MEASURE_MAPPING_PRODUCT;
	self->measure_mapping.storage_type      = storage_type;
}

static void
cm_Mapping_time_duration(cm_Mapping *self, nv_NumberStorage storage_type, u64 time_unit_in_secs)
{
	pt_fill((char*) self, (char*) self + sizeof(cm_Mapping), 0);

	self->mapping_type                         = cm_MEASURE_DIMENSION_MAPPING;

	self->measure_mapping.type	           = cm_MEASURE_MAPPING_TIME_DURATION;
	self->measure_mapping.storage_type         = storage_type;
	self->measure_mapping.time_unit_in_seconds = time_unit_in_secs;
}

static void
cm_Mapping_row_bitset(cm_Mapping *self)
{
	pt_fill((char*) self, (char*) self + sizeof(cm_Mapping), 0);
	self->mapping_type                         = cm_MEASURE_DIMENSION_MAPPING;
	self->measure_mapping.type	           = cm_MEASURE_MAPPING_ROW_BITSET;
	self->measure_mapping.storage_type         = nv_NUMBER_STORAGE_UNSIGNED_64;
}

static void
cm_Mapping_time_duration_squared(cm_Mapping *self, nv_NumberStorage storage_type, u64 time_unit_in_secs)
{
	pt_fill((char*) self, (char*) self + sizeof(cm_Mapping), 0);

	self->mapping_type                         = cm_MEASURE_DIMENSION_MAPPING;

	self->measure_mapping.type	           = cm_MEASURE_MAPPING_TIME_DURATION_SQUARED;
	self->measure_mapping.storage_type         = storage_type;
	self->measure_mapping.time_unit_in_seconds = time_unit_in_secs;
}

static void
cm_Dimension_init(cm_Dimension* self, MemoryBlock *name, cm_ColumnRefArray *cols, cm_Mapping *spec)
{
	self->type = spec->mapping_type == cm_MEASURE_DIMENSION_MAPPING ? cm_MEASURE_DIMENSION : cm_INDEX_DIMENSION;
	self->name          = *name;
	self->input_columns = *cols;
	self->mapping_spec  = *spec;
	self->user_data     = 0;
}

// cm_ compiler stuff

typedef struct {
	np_TypeID number_type_id;      // points to f64
	np_TypeID string_type_id;      // points to MemoryBlock
	np_TypeID column_ref_array_id; // points to cm_ColumnRefArray
	np_TypeID mapping_spec_id;     // points to cm_Mapping
	np_TypeID dimension_id;	       // points to cm_Dimension
	np_TypeID number_storage_id;   // points to nv_NumberStorage
	np_TypeID filename_id;         // points to MemoryBlock with a file name
} cm_CompilerTypes;

static cm_CompilerTypes cm_compiler_types;

static void
cm_compiler_register_number_storage_types(np_Compiler *compiler)
{
	// typedef enum
	// {
	//	   nv_NUMBER_STORAGE_SIGNED_32,
	//	   nv_NUMBER_STORAGE_SIGNED_64,
	//	   nv_NUMBER_STORAGE_UNSIGNED_32,
	//	   nv_NUMBER_STORAGE_UNSIGNED_64,
	//	   nv_NUMBER_STORAGE_FLOAT_32,
	//	   nv_NUMBER_STORAGE_FLOAT_64
	// }
	// nv_NumberStorage;

	// assuming cm_number_storage_names is aligned with the enum nv_NumberStorage
	nv_NumberStorage *storages = (nv_NumberStorage*) np_Compiler_alloc(compiler, sizeof(nv_NumberStorage) * 6);

	*(storages + 0) = nv_NUMBER_STORAGE_SIGNED_32;
	*(storages + 1) = nv_NUMBER_STORAGE_SIGNED_64;
	*(storages + 2) = nv_NUMBER_STORAGE_UNSIGNED_32;
	*(storages + 3) = nv_NUMBER_STORAGE_UNSIGNED_64;
	*(storages + 4) = nv_NUMBER_STORAGE_FLOAT_32;
	*(storages + 5) = nv_NUMBER_STORAGE_FLOAT_64;

	for (s32 i=0;i<6;++i) {
		np_Compiler_insert_variable
			(compiler,
			 cm_number_storage_names[i],
			 cstr_end(cm_number_storage_names[i]),
			 cm_compiler_types.number_storage_id,
			 storages + i);
	}
}

// input: (string_type_id)* -> column_reference_
np_FUNCTION_HANDLER(cm_compiler_func_input)
{
	{ // check if all types are string
		np_TypeValue *it = params_begin;
		while (it != params_end) {
			Assert(it->type_id == cm_compiler_types.string_type_id);
			++it;
		}
	}

	s64 n = params_end - params_begin;

	cm_ColumnRef* colref_begin = 0;
	cm_ColumnRef* colref_end   = 0;
	if (n > 0) {
		colref_begin = (cm_ColumnRef*) np_Compiler_alloc(compiler, n * sizeof(cm_ColumnRef));
		colref_end   = colref_begin + n;
		np_TypeValue *it_str = params_begin;
		cm_ColumnRef *it_col = colref_begin;
		while (it_str != params_end) {
			cm_ColumnRef_init(it_col, (MemoryBlock*) it_str->value);
			++it_str;
			++it_col;
		}
	}

	// allocate a contiguous sequence of
	cm_ColumnRefArray *result = (cm_ColumnRefArray*) np_Compiler_alloc(compiler, sizeof(cm_ColumnRefArray));
	result->begin = colref_begin;
	result->end   = colref_end;

	return np_TypeValue_value(cm_compiler_types.column_ref_array_id, result);
}


// input: (string_type_id)* -> column_reference_
np_FUNCTION_HANDLER(cm_compiler_func_input_indices)
{
	{ // check if all types are string
		np_TypeValue *it = params_begin;
		while (it != params_end) {
			Assert(it->type_id == cm_compiler_types.number_type_id);
			++it;
		}
	}

	s64 n = params_end - params_begin;

	cm_ColumnRef* colref_begin = 0;
	cm_ColumnRef* colref_end   = 0;
	if (n > 0) {
		colref_begin = (cm_ColumnRef*) np_Compiler_alloc(compiler, n * sizeof(cm_ColumnRef));
		colref_end   = colref_begin + n;
		np_TypeValue *it_str = params_begin;
		cm_ColumnRef *it_col = colref_begin;
		while (it_str != params_end) {
			u32 index = (u32) (((f64*) it_str->value)[0]);
			cm_ColumnRef_init_index(it_col, index - 1);
			++it_str;
			++it_col;
		}
	}

	// allocate a contiguous sequence of
	cm_ColumnRefArray *result = (cm_ColumnRefArray*) np_Compiler_alloc(compiler, sizeof(cm_ColumnRefArray));
	result->begin = colref_begin;
	result->end   = colref_end;

	return np_TypeValue_value(cm_compiler_types.column_ref_array_id, result);
}

/* latlon */
np_FUNCTION_HANDLER(cm_compiler_func_latlon)
{
	s64 n = params_end - params_begin;
	Assert(n == 1);

	np_TypeValue *number = params_begin;

	Assert(number->type_id = cm_compiler_types.number_type_id);

	f64 depth_float = *((f64*) number->value);

	if (depth_float < 1 || depth_float > 127) {
		static char *error = "latlon function expects resolution in {1,2,...,127}\n";
		np_Compiler_log_custom_error(compiler, error, cstr_end(error));
		np_Compiler_log_ast_node_context(compiler);
		return np_TypeValue_error();
	}
	u8 depth = (u8) depth_float;

	cm_Mapping *mapping_spec = (cm_Mapping*) np_Compiler_alloc(compiler, sizeof(cm_Mapping));
	cm_Mapping_latlon(mapping_spec, depth);

	return np_TypeValue_value(cm_compiler_types.mapping_spec_id, mapping_spec);
}

/* xy */
np_FUNCTION_HANDLER(cm_compiler_func_xy)
{
	s64 n = params_end - params_begin;
	Assert(n == 1);

	np_TypeValue *number = params_begin;

	Assert(number->type_id = cm_compiler_types.number_type_id);

	f64 depth_float = *((f64*) number->value);

	if (depth_float < 1 || depth_float > 127) {
		char *error = "xy function expects resolution in {1,2,...,127}\n";
		np_Compiler_log_custom_error(compiler, error, cstr_end(error));
		np_Compiler_log_ast_node_context(compiler);
		return np_TypeValue_error();
	}
	u8 depth = (u8) depth_float;

	cm_Mapping *mapping_spec = (cm_Mapping*) np_Compiler_alloc(compiler, sizeof(cm_Mapping));
	cm_Mapping_xy(mapping_spec, depth, 0);

	return np_TypeValue_value(cm_compiler_types.mapping_spec_id, mapping_spec);
}

/* slippy */
np_FUNCTION_HANDLER(cm_compiler_func_xy_slippy)
{
	s64 n = params_end - params_begin;
	Assert(n == 1);

	np_TypeValue *number = params_begin;

	Assert(number->type_id = cm_compiler_types.number_type_id);

	f64 depth_float = *((f64*) number->value);

	if (depth_float < 1 || depth_float > 127) {
		char *error = "xy_slippy function expects resolution in {1,2,...,127}\n";
		np_Compiler_log_custom_error(compiler, error, cstr_end(error));
		np_Compiler_log_ast_node_context(compiler);
		return np_TypeValue_error();
	}
	u8 depth = (u8) depth_float;

	cm_Mapping *mapping_spec = (cm_Mapping*) np_Compiler_alloc(compiler, sizeof(cm_Mapping));
	cm_Mapping_xy(mapping_spec, depth, 1);

	return np_TypeValue_value(cm_compiler_types.mapping_spec_id, mapping_spec);
}

/* ip */
np_FUNCTION_HANDLER(cm_compiler_func_ip_hilbert)
{
	s64 n = params_end - params_begin;
	Assert(n == 1);

	np_TypeValue *number = params_begin;

	Assert(number->type_id = cm_compiler_types.number_type_id);

	f64 depth_float = *((f64*) number->value);

	if (depth_float < 1 || depth_float > 16) {
		char *error = "ip_hilbert function expects resolution in {1,2,...,16}\n";
		np_Compiler_log_custom_error(compiler, error, cstr_end(error));
		np_Compiler_log_ast_node_context(compiler);
		return np_TypeValue_error();
	}
	u8 depth = (u8) depth_float;

	cm_Mapping *mapping_spec = (cm_Mapping*) np_Compiler_alloc(compiler, sizeof(cm_Mapping));
	cm_Mapping_ip_hilbert(mapping_spec, depth);

	return np_TypeValue_value(cm_compiler_types.mapping_spec_id, mapping_spec);
}

static np_TypeValue
cm_compiler_funct_time_base(np_Compiler* compiler, np_TypeValue *params_begin, np_TypeValue *params_end, b8 time_input_type)
{
	/*
	 * time(16,"2015-01-01",1*HOUR,0)
	 */
	s64 n = params_end - params_begin;
	Assert(n == 4);

	np_TypeValue *number         = params_begin;
	np_TypeValue *base_date      = params_begin + 1;
	np_TypeValue *bin_width      = params_begin + 2;
	np_TypeValue *minutes_to_add = params_begin + 3;

	f64 depth_float = *((f64*) number->value);
	if (depth_float < 1 || depth_float > 127) {
		char *error = "time function expects resolution in {1,2,...,127}\n";
		np_Compiler_log_custom_error(compiler, error, cstr_end(error));
		np_Compiler_log_ast_node_context(compiler);
		return np_TypeValue_error();
	}
	u8 depth = (u8) depth_float;

	s64 minadd = (s64) *((f64*) minutes_to_add->value);

	MemoryBlock *base_date_text = (MemoryBlock*) base_date->value;

	ntp_Parser parser;
	ntp_Parser_init(&parser);

	if (!ntp_Parser_run(&parser, base_date_text->begin, base_date_text->end)) {
		char *error = "time function expects date as second parameter.\n";
		np_Compiler_log_custom_error(compiler, error, cstr_end(error));
		np_Compiler_log_ast_node_context(compiler);
		return np_TypeValue_error();
	}

	f64 bin_width_float = *((f64*) bin_width->value);
	u64 binw = (u64) bin_width_float;

	cm_Mapping *mapping_spec = (cm_Mapping*) np_Compiler_alloc(compiler, sizeof(cm_Mapping));
	cm_Mapping_time(mapping_spec, depth, parser.time, (s64) parser.label.offset_minutes, (u64) binw, minadd, time_input_type);

	return np_TypeValue_value(cm_compiler_types.mapping_spec_id, mapping_spec);
}

static np_TypeValue
cm_compiler_funct_time_base_short(np_Compiler* compiler, np_TypeValue *params_begin, np_TypeValue *params_end, u8 time_input_type)
{
	/*
	 * time(16,"2015-01-01",1*HOUR,0)
	 */
	s64 n = params_end - params_begin;
	Assert(n == 2);

	np_TypeValue *base_date      = params_begin + 0;
	np_TypeValue *bin_width      = params_begin + 1;

	u8 depth = 16;
	s64 minadd = 0;

	MemoryBlock *base_date_text = (MemoryBlock*) base_date->value;

	ntp_Parser parser;
	ntp_Parser_init(&parser);

	if (!ntp_Parser_run(&parser, base_date_text->begin, base_date_text->end)) {
		char *error = "time function expects date as second parameter.\n";
		np_Compiler_log_custom_error(compiler, error, cstr_end(error));
		np_Compiler_log_ast_node_context(compiler);
		return np_TypeValue_error();
	}

	f64 bin_width_float = *((f64*) bin_width->value);
	u64 binw = (u64) bin_width_float;

	cm_Mapping *mapping_spec = (cm_Mapping*) np_Compiler_alloc(compiler, sizeof(cm_Mapping));
	cm_Mapping_time(mapping_spec, depth, parser.time, (s64) parser.label.offset_minutes, (u64) binw, minadd, time_input_type);

	return np_TypeValue_value(cm_compiler_types.mapping_spec_id, mapping_spec);
}

/* time */
np_FUNCTION_HANDLER(cm_compiler_func_time)
{
	return cm_compiler_funct_time_base(compiler, params_begin, params_end, cm_TIME_INPUT_TYPE_DATE);
}

np_FUNCTION_HANDLER(cm_compiler_func_time_short)
{
	return cm_compiler_funct_time_base_short(compiler, params_begin, params_end, cm_TIME_INPUT_TYPE_DATE);
}

/* btime */
np_FUNCTION_HANDLER(cm_compiler_func_btime)
{
	return cm_compiler_funct_time_base(compiler, params_begin, params_end, cm_TIME_INPUT_TYPE_BIN);
}

np_FUNCTION_HANDLER(cm_compiler_func_btime_short)
{
	return cm_compiler_funct_time_base_short(compiler, params_begin, params_end, cm_TIME_INPUT_TYPE_BIN);
}

/* unix time */
np_FUNCTION_HANDLER(cm_compiler_func_unix_time)
{
	return cm_compiler_funct_time_base(compiler, params_begin, params_end, cm_TIME_INPUT_TYPE_UNIX_TIME);
}

np_FUNCTION_HANDLER(cm_compiler_func_unix_time_short)
{
	return cm_compiler_funct_time_base_short(compiler, params_begin, params_end, cm_TIME_INPUT_TYPE_UNIX_TIME);
}

/* weekday */
np_FUNCTION_HANDLER(cm_compiler_func_hour)
{
	/*
	 * hour()
	 */
	s64 n = params_end - params_begin;
	Assert(n == 0);

	cm_Mapping *mapping_spec = (cm_Mapping*) np_Compiler_alloc(compiler, sizeof(cm_Mapping));
	cm_Mapping_hour(mapping_spec);

	return np_TypeValue_value(cm_compiler_types.mapping_spec_id, mapping_spec);
}

/* hour */
np_FUNCTION_HANDLER(cm_compiler_func_weekday)
{
	/*
	 * weekday()
	 */
	s64 n = params_end - params_begin;
	Assert(n == 0);

	cm_Mapping *mapping_spec = (cm_Mapping*) np_Compiler_alloc(compiler, sizeof(cm_Mapping));
	cm_Mapping_weekday(mapping_spec);

	return np_TypeValue_value(cm_compiler_types.mapping_spec_id, mapping_spec);
}


/* categorical */
np_FUNCTION_HANDLER(cm_compiler_func_categorical_short)
{
	s64 n = params_end - params_begin;
	Assert(n == 0);

	u8 bits = 8;
	u8 levels = 1;

	cm_Mapping *mapping_spec = (cm_Mapping*) np_Compiler_alloc(compiler, sizeof(cm_Mapping));
	cm_Mapping_categorical(mapping_spec, bits, levels, (MemoryBlock) { .begin=0, .end=0 }, 0);

	return np_TypeValue_value(cm_compiler_types.mapping_spec_id, mapping_spec);
}

/* categorical */
np_FUNCTION_HANDLER(cm_compiler_func_categorical_short_with_labels)
{
	s64 n = params_end - params_begin;
	Assert(n == 1);

	np_TypeValue *labels_tv = params_begin;

	Assert(labels_tv->type_id == cm_compiler_types.string_type_id);

	MemoryBlock labels_table = *((MemoryBlock*) labels_tv->value);

	u8 bits   = (u8) 8;
	u8 levels = (u8) 1;

	cm_Mapping *mapping_spec = (cm_Mapping*) np_Compiler_alloc(compiler, sizeof(cm_Mapping));
	cm_Mapping_categorical(mapping_spec, bits, levels, labels_table, 0);

	return np_TypeValue_value(cm_compiler_types.mapping_spec_id, mapping_spec);
}

/* categorical */
np_FUNCTION_HANDLER(cm_compiler_func_categorical)
{
	s64 n = params_end - params_begin;
	Assert(n == 2);

	np_TypeValue *bits_tv   = params_begin;
	np_TypeValue *levels_tv = params_begin+1;

	Assert(bits_tv->type_id   == cm_compiler_types.number_type_id);
	Assert(levels_tv->type_id == cm_compiler_types.number_type_id);

	f64 bits_float   = *((f64*) bits_tv->value);
	f64 levels_float = *((f64*) levels_tv->value);

	if (bits_float< 1 || bits_float > 8) {
		char *error = "categorical function expects bit resolution in {1,2,...,8}\n";
		np_Compiler_log_custom_error(compiler, error, cstr_end(error));
		np_Compiler_log_ast_node_context(compiler);
		return np_TypeValue_error();
	}
	u8 bits   = (u8) bits_float;
	u8 levels = (u8) levels_float;

	cm_Mapping *mapping_spec = (cm_Mapping*) np_Compiler_alloc(compiler, sizeof(cm_Mapping));
	cm_Mapping_categorical(mapping_spec, bits, levels, (MemoryBlock) { .begin=0, .end=0 }, 0);

	return np_TypeValue_value(cm_compiler_types.mapping_spec_id, mapping_spec);
}

/* categorical */
np_FUNCTION_HANDLER(cm_compiler_func_numerical)
{
	s64 n = params_end - params_begin;
	Assert(n == 4);

	np_TypeValue *bits_tv = params_begin;
	np_TypeValue *a_tv    = params_begin+1;
	np_TypeValue *b_tv    = params_begin+2;
	np_TypeValue *to_int_method_code_tv = params_begin+3;

	s32 bits = (s32) *((f64*) bits_tv->value);
	f64 a = *((f64*) a_tv->value);
	f64 b = *((f64*) b_tv->value);
	s32 to_int_method_code = (s32) *((f64*) to_int_method_code_tv->value);

	if (bits< 1 || bits> 128) {
		char *error = "categorical function expects bit resolution in {1,2,...,128}\n";
		np_Compiler_log_custom_error(compiler, error, cstr_end(error));
		np_Compiler_log_ast_node_context(compiler);
		return np_TypeValue_error();
	}

	if (to_int_method_code < 0 || to_int_method_code > 3) {
		char *error = "invalid to_int_method_code, it can be either 0,1,2 or 3\n";
		np_Compiler_log_custom_error(compiler, error, cstr_end(error));
		np_Compiler_log_ast_node_context(compiler);
		return np_TypeValue_error();
	}

	cm_Mapping *mapping_spec = (cm_Mapping*) np_Compiler_alloc(compiler, sizeof(cm_Mapping));
	cm_Mapping_numerical(mapping_spec, bits, a, b, to_int_method_code);

	return np_TypeValue_value(cm_compiler_types.mapping_spec_id, mapping_spec);
}

/* categorical */
np_FUNCTION_HANDLER(cm_compiler_func_categorical_with_labels)
{
	s64 n = params_end - params_begin;
	Assert(n == 3);

	np_TypeValue *bits_tv   = params_begin;
	np_TypeValue *levels_tv = params_begin+1;
	np_TypeValue *labels_tv = params_begin+2;

	Assert(bits_tv->type_id   == cm_compiler_types.number_type_id);
	Assert(levels_tv->type_id == cm_compiler_types.number_type_id);
	Assert(labels_tv->type_id == cm_compiler_types.string_type_id);

	f64         bits_float   = *((f64*) bits_tv->value);
	f64         levels_float = *((f64*) levels_tv->value);
	MemoryBlock labels_table = *((MemoryBlock*) labels_tv->value);

	if (bits_float< 1 || bits_float > 8) {
		char *error = "categorical function expects bit resolution in {1,2,...,8}\n";
		np_Compiler_log_custom_error(compiler, error, cstr_end(error));
		np_Compiler_log_ast_node_context(compiler);
		return np_TypeValue_error();
	}
	u8 bits   = (u8) bits_float;
	u8 levels = (u8) levels_float;

	cm_Mapping *mapping_spec = (cm_Mapping*) np_Compiler_alloc(compiler, sizeof(cm_Mapping));
	cm_Mapping_categorical(mapping_spec, bits, levels, labels_table, 0);

	return np_TypeValue_value(cm_compiler_types.mapping_spec_id, mapping_spec);
}

/* categorical */
np_FUNCTION_HANDLER(cm_compiler_func_categorical_with_file)
{
	s64 n = params_end - params_begin;
	Assert(n == 3);

	np_TypeValue *bits_tv   = params_begin;
	np_TypeValue *levels_tv = params_begin+1;
	np_TypeValue *filename_tv = params_begin+2;

	Assert(bits_tv->type_id   == cm_compiler_types.number_type_id);
	Assert(levels_tv->type_id == cm_compiler_types.number_type_id);
	Assert(filename_tv->type_id == cm_compiler_types.filename_id);

	f64         bits_float   = *((f64*) bits_tv->value);
	f64         levels_float = *((f64*) levels_tv->value);
	MemoryBlock labels_table = *((MemoryBlock*) filename_tv->value); // <--- filename string should be here

	if (bits_float< 1 || bits_float > 8) {
		char *error = "categorical function expects bit resolution in {1,2,...,8}\n";
		np_Compiler_log_custom_error(compiler, error, cstr_end(error));
		np_Compiler_log_ast_node_context(compiler);
		return np_TypeValue_error();
	}
	u8 bits   = (u8) bits_float;
	u8 levels = (u8) levels_float;

	cm_Mapping *mapping_spec = (cm_Mapping*) np_Compiler_alloc(compiler, sizeof(cm_Mapping));
	cm_Mapping_categorical(mapping_spec, bits, levels, labels_table, 1);

	return np_TypeValue_value(cm_compiler_types.mapping_spec_id, mapping_spec);
}


/* file: tags a string as being a filename */
np_FUNCTION_HANDLER(cm_compiler_func_file)
{
	s64 n = params_end - params_begin;
	Assert(n == 1);
	np_TypeValue *filename_tv   = params_begin;
	Assert(filename_tv->type_id == cm_compiler_types.string_type_id);
	return np_TypeValue_value(cm_compiler_types.filename_id, (MemoryBlock*) filename_tv->value);
}

// input: (string_type_id)* -> column_reference_
np_FUNCTION_HANDLER(cm_compiler_func_index_dimension)
{
	s64 n = params_end - params_begin;

	Assert(n == 3);
	Assert(params_begin->type_id == cm_compiler_types.string_type_id);
	Assert((params_begin+1)->type_id == cm_compiler_types.column_ref_array_id);
	Assert((params_begin+2)->type_id == cm_compiler_types.mapping_spec_id);

	MemoryBlock       *name = (MemoryBlock*) (params_begin+0)->value;
	cm_ColumnRefArray *cols = (cm_ColumnRefArray*) (params_begin+1)->value;
	cm_Mapping    *spec = (cm_Mapping*)    (params_begin+2)->value;

	if (spec->mapping_type != cm_INDEX_DIMENSION_MAPPING) {
		char *error = "expected a measure dimension spec (eg. product, measure).\n";
		np_Compiler_log_custom_error(compiler, error, cstr_end(error));
		np_Compiler_log_ast_node_context(compiler);
		return np_TypeValue_error();
	}

	cm_Dimension *result = (cm_Dimension*) np_Compiler_alloc(compiler,sizeof(cm_Dimension));
	cm_Dimension_init(result, name, cols, spec);

	return np_TypeValue_value(cm_compiler_types.dimension_id, result);
}

np_FUNCTION_HANDLER(cm_compiler_func_measure_dimension_from_storage_type)
{
	s64 n = params_end - params_begin;

	Assert(n == 3);
	Assert((params_begin+0)->type_id == cm_compiler_types.string_type_id);
	Assert((params_begin+1)->type_id == cm_compiler_types.column_ref_array_id);
	Assert((params_begin+2)->type_id == cm_compiler_types.number_storage_id);

	MemoryBlock       *name = (MemoryBlock*) (params_begin+0)->value;
	cm_ColumnRefArray *cols = (cm_ColumnRefArray*) (params_begin+1)->value;
	nv_NumberStorage  *numb = (nv_NumberStorage*)  (params_begin+2)->value;

	cm_Mapping product_mapping;
	cm_Mapping_product(&product_mapping, *numb);

	cm_Dimension *result = (cm_Dimension*) np_Compiler_alloc(compiler, sizeof(cm_Dimension));
	cm_Dimension_init(result, name, cols, &product_mapping);

	return np_TypeValue_value(cm_compiler_types.dimension_id, result);
}

np_FUNCTION_HANDLER(cm_compiler_func_row_bitset)
{
	s64 n = params_end - params_begin;
	Assert(n == 0);
	cm_Mapping*result = (cm_Mapping*) np_Compiler_alloc(compiler, sizeof(cm_Mapping));
	cm_Mapping_row_bitset(result);
	return np_TypeValue_value(cm_compiler_types.mapping_spec_id, result);
}


np_FUNCTION_HANDLER(cm_compiler_func_time_duration)
{
	s64 n = params_end - params_begin;

	Assert(n == 2);
	Assert((params_begin+0)->type_id == cm_compiler_types.number_storage_id);
	Assert((params_begin+1)->type_id == cm_compiler_types.number_type_id);

	nv_NumberStorage  storage_type      = *((nv_NumberStorage*)  (params_begin+0)->value);
	u64               time_unit_in_secs = (u64) *((f64*)(params_begin+1)->value);

	cm_Mapping product_mapping;
	cm_Mapping_product(&product_mapping, storage_type);

	cm_Mapping*result = (cm_Mapping*) np_Compiler_alloc(compiler, sizeof(cm_Mapping));
	cm_Mapping_time_duration(result, storage_type,  time_unit_in_secs);

	return np_TypeValue_value(cm_compiler_types.mapping_spec_id, result);
}

np_FUNCTION_HANDLER(cm_compiler_func_time_duration_squared)
{
	s64 n = params_end - params_begin;

	Assert(n == 2);
	Assert((params_begin+0)->type_id == cm_compiler_types.number_storage_id);
	Assert((params_begin+1)->type_id == cm_compiler_types.number_type_id);

	nv_NumberStorage  storage_type      = *((nv_NumberStorage*)  (params_begin+0)->value);
	u64               time_unit_in_secs = (u64) *((f64*)(params_begin+1)->value);

	cm_Mapping product_mapping;
	cm_Mapping_product(&product_mapping, storage_type);

	cm_Mapping*result = (cm_Mapping*) np_Compiler_alloc(compiler, sizeof(cm_Mapping));
	cm_Mapping_time_duration_squared(result, storage_type,  time_unit_in_secs);

	return np_TypeValue_value(cm_compiler_types.mapping_spec_id, result);
}

np_FUNCTION_HANDLER(cm_compiler_func_measure_dimension)
{
	s64 n = params_end - params_begin;

	Assert(n == 3);
	Assert((params_begin+0)->type_id == cm_compiler_types.string_type_id);
	Assert((params_begin+1)->type_id == cm_compiler_types.column_ref_array_id);
	Assert((params_begin+2)->type_id == cm_compiler_types.mapping_spec_id);

	MemoryBlock       *name = (MemoryBlock*)       (params_begin+0)->value;
	cm_ColumnRefArray *cols = (cm_ColumnRefArray*) (params_begin+1)->value;
	cm_Mapping        *spec = (cm_Mapping*)        (params_begin+2)->value;

	if (spec->mapping_type != cm_MEASURE_DIMENSION_MAPPING) {
		char *error = "expected a measure dimension spec (eg. product, measure).\n";
		np_Compiler_log_custom_error(compiler, error, cstr_end(error));
		np_Compiler_log_ast_node_context(compiler);
		return np_TypeValue_error();
	}

	cm_Dimension *result = (cm_Dimension*) np_Compiler_alloc(compiler, sizeof(cm_Dimension));
	cm_Dimension_init(result, name, cols, spec);

	return np_TypeValue_value(cm_compiler_types.dimension_id, result);
}

static void
cm_init_compiler_csv_mapping_infrastructure(np_Compiler *compiler)
{
	cm_compiler_types.number_type_id  = compiler->number_type_id;
	cm_compiler_types.string_type_id  = compiler->string_type_id;

	// register new csv-schema mapping types and copy id
	cm_compiler_types.column_ref_array_id = np_Compiler_insert_type_cstr(compiler,"cm_ColumnRefArray")->id;
	cm_compiler_types.mapping_spec_id     = np_Compiler_insert_type_cstr(compiler,"cm_Mapping")->id;
	cm_compiler_types.dimension_id        = np_Compiler_insert_type_cstr(compiler,"cm_Dimension")->id;
	cm_compiler_types.number_storage_id   = np_Compiler_insert_type_cstr(compiler,"nv_NumberStorage")->id;
	cm_compiler_types.filename_id         = np_Compiler_insert_type_cstr(compiler,"cm_Filename")->id;

	// and operators
	np_TypeID parameter_types[4];

	// input: string* -> column_ref_array
	np_Compiler_insert_function_cstr
		(compiler, "input", cm_compiler_types.column_ref_array_id,
		 0, 0, 1, cm_compiler_types.string_type_id,
		 cm_compiler_func_input);

	// input: int* -> column_ref_array
	np_Compiler_insert_function_cstr
		(compiler, "input", cm_compiler_types.column_ref_array_id,
		 0, 0, 1, cm_compiler_types.number_type_id,
		 cm_compiler_func_input_indices);

	// latlon: number -> mapping_spec
	// read latlon in degrees convert to mercator convert to quadtree
	parameter_types[0] = cm_compiler_types.number_type_id;
	np_Compiler_insert_function_cstr
		(compiler, "latlon", cm_compiler_types.mapping_spec_id,
		 parameter_types, parameter_types+1, 0, 0,
		 cm_compiler_func_latlon);

	// latlon: number -> mapping_spec
	// read latlon in degrees convert to mercator convert to quadtree
	parameter_types[0] = cm_compiler_types.number_type_id;
	np_Compiler_insert_function_cstr
		(compiler, "xy", cm_compiler_types.mapping_spec_id,
		 parameter_types, parameter_types+1, 0, 0,
		 cm_compiler_func_xy);

	// latlon: number -> mapping_spec
	// read latlon in degrees convert to mercator convert to quadtree
	parameter_types[0] = cm_compiler_types.number_type_id;
	np_Compiler_insert_function_cstr
		(compiler, "xy_slippy", cm_compiler_types.mapping_spec_id,
		 parameter_types, parameter_types+1, 0, 0,
		 cm_compiler_func_xy_slippy);

	// latlon: number -> mapping_spec
	// read latlon in degrees convert to mercator convert to quadtree
	parameter_types[0] = cm_compiler_types.number_type_id;
	np_Compiler_insert_function_cstr
		(compiler, "ip", cm_compiler_types.mapping_spec_id,
		 parameter_types, parameter_types+1, 0, 0,
		 cm_compiler_func_ip_hilbert);


	// categorical: string -> mapping_spec
	// read mapping from string
	parameter_types[0] = cm_compiler_types.string_type_id;
	np_Compiler_insert_function_cstr
		(compiler, "categorical", cm_compiler_types.mapping_spec_id,
		 parameter_types, parameter_types+1, 0, 0,
		 cm_compiler_func_categorical_short_with_labels);

	// categorical: none -> mapping_spec
	// read mapping from string
	np_Compiler_insert_function_cstr
		(compiler, "categorical", cm_compiler_types.mapping_spec_id,
		 0, 0, 0, 0,
		 cm_compiler_func_categorical_short);

	// categorical: number -> mapping_spec
	// read latlon in degrees convert to mercator convert to quadtree
	parameter_types[0] = cm_compiler_types.number_type_id;
	parameter_types[1] = cm_compiler_types.number_type_id;
	np_Compiler_insert_function_cstr
		(compiler, "categorical", cm_compiler_types.mapping_spec_id,
		 parameter_types, parameter_types+2, 0, 0,
		 cm_compiler_func_categorical);

	// categorical: number -> mapping_spec
	// read latlon in degrees convert to mercator convert to quadtree
	parameter_types[0] = cm_compiler_types.number_type_id;
	parameter_types[1] = cm_compiler_types.number_type_id;
	parameter_types[2] = cm_compiler_types.string_type_id;
	np_Compiler_insert_function_cstr
		(compiler, "categorical", cm_compiler_types.mapping_spec_id,
		 parameter_types, parameter_types+3, 0, 0,
		 cm_compiler_func_categorical_with_labels);

	// numerical: (number,number,number,number) -> mapping_spec
	// read latlon in degrees convert to mercator convert to quadtree
	parameter_types[0] = cm_compiler_types.number_type_id;
	parameter_types[1] = cm_compiler_types.number_type_id;
	parameter_types[2] = cm_compiler_types.number_type_id;
	parameter_types[3] = cm_compiler_types.number_type_id;
	np_Compiler_insert_function_cstr
		(compiler, "numerical", cm_compiler_types.mapping_spec_id,
		 parameter_types, parameter_types+4, 0, 0,
		 cm_compiler_func_numerical);

	// categorical: number -> mapping_spec
	// read latlon in degrees convert to mercator convert to quadtree
	parameter_types[0] = cm_compiler_types.number_type_id;
	parameter_types[1] = cm_compiler_types.number_type_id;
	parameter_types[2] = cm_compiler_types.filename_id;
	np_Compiler_insert_function_cstr
		(compiler, "categorical", cm_compiler_types.mapping_spec_id,
		 parameter_types, parameter_types+3, 0, 0,
		 cm_compiler_func_categorical_with_file);

	// categorical: number -> mapping_spec
	// read latlon in degrees convert to mercator convert to quadtree
	parameter_types[0] = cm_compiler_types.string_type_id;
	np_Compiler_insert_function_cstr
		(compiler, "file", cm_compiler_types.mapping_spec_id,
		 parameter_types, parameter_types+1, 0, 0,
		 cm_compiler_func_file);

	// time
	parameter_types[0] = cm_compiler_types.number_type_id;
	parameter_types[1] = cm_compiler_types.string_type_id;
	parameter_types[2] = cm_compiler_types.number_type_id;
	parameter_types[3] = cm_compiler_types.number_type_id;
	np_Compiler_insert_function_cstr
		(compiler, "time", cm_compiler_types.mapping_spec_id,
		 parameter_types, parameter_types+4, 0, 0,
		 cm_compiler_func_time);

	// time
	parameter_types[0] = cm_compiler_types.string_type_id;
	parameter_types[1] = cm_compiler_types.number_type_id;
	np_Compiler_insert_function_cstr
		(compiler, "time", cm_compiler_types.mapping_spec_id,
		 parameter_types, parameter_types+2, 0, 0,
		 cm_compiler_func_time_short);


	// btime
	parameter_types[0] = cm_compiler_types.number_type_id;
	parameter_types[1] = cm_compiler_types.string_type_id;
	parameter_types[2] = cm_compiler_types.number_type_id;
	parameter_types[3] = cm_compiler_types.number_type_id;
	np_Compiler_insert_function_cstr
		(compiler, "btime", cm_compiler_types.mapping_spec_id,
		 parameter_types, parameter_types+4, 0, 0,
		 cm_compiler_func_btime);

	// btime
	parameter_types[0] = cm_compiler_types.string_type_id;
	parameter_types[1] = cm_compiler_types.number_type_id;
	np_Compiler_insert_function_cstr
		(compiler, "btime", cm_compiler_types.mapping_spec_id,
		 parameter_types, parameter_types+2, 0, 0,
		 cm_compiler_func_btime_short);





	// latlon: number -> mapping_spec
	// read latlon in degrees convert to mercator convert to quadtree
	parameter_types[0] = cm_compiler_types.number_type_id;
	parameter_types[1] = cm_compiler_types.string_type_id;
	parameter_types[2] = cm_compiler_types.number_type_id;
	parameter_types[3] = cm_compiler_types.number_type_id;
	np_Compiler_insert_function_cstr
		(compiler, "unixtime", cm_compiler_types.mapping_spec_id,
		 parameter_types, parameter_types+4, 0, 0,
		 cm_compiler_func_unix_time);


	parameter_types[0] = cm_compiler_types.string_type_id;
	parameter_types[1] = cm_compiler_types.number_type_id;
	np_Compiler_insert_function_cstr
		(compiler, "unixtime", cm_compiler_types.mapping_spec_id,
		 parameter_types, parameter_types+2, 0, 0,
		 cm_compiler_func_unix_time_short);


	// hour
	np_Compiler_insert_function_cstr(compiler, "hour", cm_compiler_types.mapping_spec_id, 0, 0, 0, 0,
					 cm_compiler_func_hour);

	// hour
	np_Compiler_insert_function_cstr(compiler, "weekday", cm_compiler_types.mapping_spec_id, 0, 0, 0, 0,
					 cm_compiler_func_weekday);

	// index_dimension: string, colrefarray, mappingspec -> dimension
	parameter_types[0] = cm_compiler_types.string_type_id;
	parameter_types[1] = cm_compiler_types.column_ref_array_id;
	parameter_types[2] = cm_compiler_types.mapping_spec_id;
	np_Compiler_insert_function_cstr
		(compiler, "index_dimension",
		 cm_compiler_types.mapping_spec_id,
		 parameter_types, parameter_types+3, 0, 0,
		 cm_compiler_func_index_dimension);

	// index_dimension: string, colrefarray, mappingspec -> dimension
	parameter_types[0] = cm_compiler_types.string_type_id;
	parameter_types[1] = cm_compiler_types.column_ref_array_id;
	parameter_types[2] = cm_compiler_types.number_storage_id;
	np_Compiler_insert_function_cstr
		(compiler, "measure_dimension",
		 cm_compiler_types.mapping_spec_id, parameter_types,
		 parameter_types+3, 0, 0,
		 cm_compiler_func_measure_dimension_from_storage_type);

	// index_dimension: string, colrefarray, mappingspec -> dimension
	parameter_types[0] = cm_compiler_types.string_type_id;
	parameter_types[1] = cm_compiler_types.column_ref_array_id;
	parameter_types[2] = cm_compiler_types.mapping_spec_id;
	np_Compiler_insert_function_cstr
		(compiler, "measure_dimension",
		 cm_compiler_types.mapping_spec_id, parameter_types,
		 parameter_types+3, 0, 0,
		 cm_compiler_func_measure_dimension);

	// index_dimension: string, colrefarray, mappingspec -> dimension
	parameter_types[0] = cm_compiler_types.number_storage_id;
	parameter_types[1] = cm_compiler_types.number_type_id;
	np_Compiler_insert_function_cstr
		(compiler, "duration",
		 cm_compiler_types.mapping_spec_id, parameter_types,
		 parameter_types+2, 0, 0,
		 cm_compiler_func_time_duration);

	// index_dimension: string, colrefarray, mappingspec -> dimension
	parameter_types[0] = cm_compiler_types.number_storage_id;
	parameter_types[1] = cm_compiler_types.number_type_id;
	np_Compiler_insert_function_cstr
		(compiler, "duration2",
		 cm_compiler_types.mapping_spec_id, parameter_types,
		 parameter_types+2, 0, 0,
		 cm_compiler_func_time_duration_squared);

	// row_bitset: mappingspec
	np_Compiler_insert_function_cstr
		(compiler, "row_bitset",
		 cm_compiler_types.mapping_spec_id,
		 0, 0, 0, 0,
		 cm_compiler_func_row_bitset);
}

typedef b8 (cm_SnappingLatLonFunction)(f32 *lat, f32 *lon);

#define cm_MAX_LONGITUDE   180.0
#define cm_MIN_LONGITUDE  -180.0
#define cm_MIN_LATITUDE   -85.05113
#define cm_MAX_LATITUDE    85.05113
static b8
cm_mapping_latlon(cm_Dimension *dim, nx_Array *array, MemoryBlock *tokens_begin, cm_SnappingLatLonFunction *snap)
{
	// Print *print = &request->print;

	Assert(dim->mapping_spec.index_mapping.type == cm_INDEX_MAPPING_LATLON_MERCATOR_QUADTREE);

	u8 levels = dim->mapping_spec.index_mapping.latlon.depth;

	Assert(levels == array->length);

	cm_ColumnRef *basecol = dim->input_columns.begin;

	Assert(dim->input_columns.end - dim->input_columns.begin == 2);

	u32 latidx = (basecol+0)->index;
	u32 lonidx = (basecol+1)->index;

	MemoryBlock *lat_text = tokens_begin + latidx;
	MemoryBlock *lon_text = tokens_begin + lonidx;

	f32 lat, lon;

	if (!pt_parse_f32(lat_text->begin, lat_text->end, &lat)) {
		return 0;
	}

	if (!pt_parse_f32(lon_text->begin, lon_text->end, &lon)) {
		return 0;
	}

	if (snap) {
		b8 snap_success = (*snap)(&lat, &lon);
		if (!snap_success) {
			/* did not find a snap point for this lat lon */
			return 0;
		}
	}

	if (lon < cm_MIN_LONGITUDE
	    || lon > cm_MAX_LONGITUDE
	    || lat < cm_MIN_LATITUDE
	    || lat > cm_MAX_LATITUDE) {
		return 0;
	}


	// mercator from degrees
	f64 mercator_x = lon / 180.0;

	// @TODO(llins): simplify to /360.0
	f64 mercator_y
		= pt_log_f64(
			 pt_tan_f64(
				(lat * pt_PI/180.0)/2.0
				+ pt_PI/4.0
				)
			)
		/ pt_PI;


	// @TODO(llins): check limiting cases (e.g. lon==180);
	u64 bins = 1ull << levels;
	u64 cell_x = (u64) ((mercator_x + 1.0)/2.0 * bins);
	u64 cell_y = (u64) ((mercator_y + 1.0)/2.0 * bins);

	// quadtree path
// 	Print_clear(print);
// 	Print_cstr(print, "lat: ");
// 	Print_f64(print,lat);
// 	Print_align(print,24,1);
// 	Print_cstr(print, "   lon: ");
// 	Print_f64(print,lon);
// 	Print_align(print,24,1);
// 	Print_cstr(print, "   mx: ");
// 	Print_f64(print,mercator_x);
// 	Print_align(print,24,1);
// 	Print_cstr(print, "   my: ");
// 	Print_f64(print,mercator_y);
// 	Print_align(print,24,1);
// 	Print_cstr(print, "   cellx: ");
// 	Print_u64(print,cell_x);
// 	Print_align(print,24,1);
// 	Print_cstr(print, "   celly: ");
// 	Print_u64(print,cell_y);
// 	Print_align(print,24,1);
// 	Print_cstr(print, "\n");
// 	Request_print(request,print);

	for (u8 i=0;i<levels;i++) {
		u8 label = ((cell_x & 1) ? 1 : 0) + ((cell_y & 1) ? 2 : 0);
		cell_x >>= 1;
		cell_y >>= 1;
		nx_Array_set(array, levels - 1 - i, label);
	}

// 	Print_clear(print);
// 	Print_cstr(print, "path: ");
// 	for (u8 i=0;i<levels;i++) {
// 		u8 label = nx_Array_get(array,i);
// 		Print_u64(print, (u64) label);
// 	}
// 	Print_cstr(print, "\n");
// 	Request_print(request,print);

	return 1;
}

static b8
cm_mapping_xy(cm_Dimension *dim, nx_Array *array, MemoryBlock *tokens_begin)
{
	// Print *print = &request->print;

	Assert(dim->mapping_spec.index_mapping.type == cm_INDEX_MAPPING_XY_QUADTREE);

	u8 levels   = dim->mapping_spec.index_mapping.xy.depth;
	b8 top_down = dim->mapping_spec.index_mapping.xy.top_down;

	Assert(levels == array->length);

	cm_ColumnRef *basecol = dim->input_columns.begin;

	Assert(dim->input_columns.end - dim->input_columns.begin == 2);

	u32 x_idx = (basecol+0)->index;
	u32 y_idx = (basecol+1)->index;

	MemoryBlock *x_text = tokens_begin + x_idx;
	MemoryBlock *y_text = tokens_begin + y_idx;

	u64 cell_x=0, cell_y=0;

	if (!pt_parse_u64(x_text->begin, x_text->end, &cell_x)) {
		return 0;
	}

	if (!pt_parse_u64(y_text->begin, y_text->end, &cell_y)) {
		return 0;
	}

	u64 cells_per_side = 1ull << levels;
	if (cell_x >= cells_per_side || cell_y >= cells_per_side) {
		return 0;
	}

	if (top_down) {
		cell_y = cells_per_side - 1 - cell_y;
	}

	// quadtree path
// 	Print_clear(print);
// 	Print_cstr(print, "lat: ");
// 	Print_f64(print,lat);
// 	Print_align(print,24,1);
// 	Print_cstr(print, "   lon: ");
// 	Print_f64(print,lon);
// 	Print_align(print,24,1);
// 	Print_cstr(print, "   mx: ");
// 	Print_f64(print,mercator_x);
// 	Print_align(print,24,1);
// 	Print_cstr(print, "   my: ");
// 	Print_f64(print,mercator_y);
// 	Print_align(print,24,1);
// 	Print_cstr(print, "   cellx: ");
// 	Print_u64(print,cell_x);
// 	Print_align(print,24,1);
// 	Print_cstr(print, "   celly: ");
// 	Print_u64(print,cell_y);
// 	Print_align(print,24,1);
// 	Print_cstr(print, "\n");
// 	Request_print(request,print);

	for (u8 i=0;i<levels;i++) {
		u8 label = ((cell_x & 1) ? 1 : 0) + ((cell_y & 1) ? 2 : 0);
		cell_x >>= 1;
		cell_y >>= 1;
		nx_Array_set(array, levels - 1 - i, label);
	}

// 	Print_clear(print);
// 	Print_cstr(print, "path: ");
// 	for (u8 i=0;i<levels;i++) {
// 		u8 label = nx_Array_get(array,i);
// 		Print_u64(print, (u64) label);
// 	}
// 	Print_cstr(print, "\n");
// 	Request_print(request,print);

	return 1;
}

//rotate/flip a quadrant appropriately
static void
cm_hilbert_rot(u32 n, u32 *x, u32 *y, u32 rx, u32 ry) {
	if (ry == 0) {
		if (rx == 1) {
			*x = n-1 - *x;
			*y = n-1 - *y;
		}

		//Swap x and y
		u32 t = *x;
		*x = *y;
		*y =  t;
	}
}

//convert (x,y) to d
static u32
cm_hilbert_xy2d (u32 n, u32 x, u32 y) {
	u32 rx, ry, s, d=0;
	for (s=n/2; s>0; s/=2) {
		rx = (x & s) > 0;
		ry = (y & s) > 0;
		d += s * s * ((3 * rx) ^ ry);
		cm_hilbert_rot(s, &x, &y, rx, ry);
	}
	return d;
}

//convert d to (x,y)
void cm_hilbert_d2xy(u32 n, u32 d, u32 *x, u32 *y) {
	u32 rx, ry, s, t=d;
	*x = *y = 0;
	for (s=1; s<n; s*=2) {
		rx = 1 & (t/2);
		ry = 1 & (t ^ rx);
		cm_hilbert_rot(s, x, y, rx, ry);
		*x += s * rx;
		*y += s * ry;
		t /= 4;
	}
}

//
// @generalize 2017-06-28T16:47
// Make this procedure support deeper hierarchies maybe reading from an IPv6. or so...
// Right now it is limited to u32 numbers.
//
static b8
cm_mapping_ip_hilbert(cm_Dimension *dim, nx_Array *array, MemoryBlock *tokens_begin)
{
	// Print *print = &request->print;
	Assert(dim->mapping_spec.index_mapping.type == cm_INDEX_MAPPING_IP_HILBERT);

	u8 levels = dim->mapping_spec.index_mapping.ip_hilbert.depth;

	Assert(levels == array->length);
	Assert(levels>0 && levels <= 16);

	cm_ColumnRef *basecol = dim->input_columns.begin;

	Assert(dim->input_columns.end - dim->input_columns.begin == 1);

	MemoryBlock text = *(tokens_begin + basecol->index);

	// parse the four numbers separated by '.' into text
	u32 ip = 0;
	char *it = text.begin;
	s32 numbers_to_parse = 4;
	Assert(numbers_to_parse > 0);
	for (;;) {
		char *it_next = pt_find_char(it, text.end, '.');

		u32 num = 0;
		if (!pt_parse_u32(it,it_next,&num) || num > 255) {
			return 0;
		}
		ip = (ip << 8) + num;

		--numbers_to_parse;
		if (numbers_to_parse == 0) {
			if (it_next != text.end) {
				// there should be no more content to parse
				return 0;
			} else {
				// done
				break;
			}
		}

		it = it_next+1; // m
	}

	// consider only the top 2xlevel bits
	ip >>= (32 - 2*levels);

	// if we got to this point we have the for bytes
	// of the IPv4
	u32 x, y;
	cm_hilbert_d2xy(1u << levels, ip, &x, &y);
	y = (1 << levels) - 1 - y;
	for (u8 i=0;i<levels;i++) {
		u8 label = ((x & 1) ? 1 : 0) + ((y & 1) ? 2 : 0);
		x >>= 1;
		y >>= 1;
		nx_Array_set(array, levels - 1 - i, label);
	}

	return 1;
}

static b8
cm_mapping_numerical(cm_Dimension *dim, nx_Array *array, MemoryBlock *tokens_begin)
{
	Assert(dim->mapping_spec.index_mapping.type == cm_INDEX_MAPPING_NUMERICAL);

	cm_Mapping *mapping = &dim->mapping_spec;

	u8 levels = mapping->index_mapping.numerical.bits;

	Assert(levels == array->length);

	cm_ColumnRef *basecol = dim->input_columns.begin;

	Assert(dim->input_columns.end - dim->input_columns.begin == 1);

	u32 timeidx = (basecol+0)->index;

	MemoryBlock *number_text = tokens_begin + timeidx;

	f64 x = 0;
	s32 ok = 0;
	// losing the time fraction part of the data
	if (!pt_parse_f64(number_text->begin, number_text->end, &x)) {
		return 0;
	}

	f64 y = mapping->index_mapping.numerical.spec.a * x + mapping->index_mapping.numerical.spec.b;

	s64 bin = 0;
	switch(mapping->index_mapping.numerical.spec.to_int_method) {
	case nm_NUMERICAL_TO_INT_METHOD_TRUNCATE: {
		bin = (s64) pt_trunc_f64(y);
	} break;
	case nm_NUMERICAL_TO_INT_METHOD_FLOOR: {
		bin = (s64) pt_floor_f64(y);
	} break;
	case nm_NUMERICAL_TO_INT_METHOD_CEIL: {
		bin = (s64) pt_ceil_f64(y);
	} break;
	case nm_NUMERICAL_TO_INT_METHOD_ROUND: {
		bin = (s64) pt_round_f64(y);
	} break;
	default: {
		return 0;
	} break;
	};

	if (bin < 0 || bin >= (((s64)1) << levels)) {
		return 0;
	}

	for (u8 i=0;i<levels;i++) {
		u8 label = (bin & 1) ? 1 : 0;
		bin >>= 1;
		nx_Array_set(array, levels - 1 - i, label);
	}
	return 1;
}







static b8
cm_mapping_time(cm_Dimension *dim, nx_Array *array, MemoryBlock *tokens_begin, ntp_Parser *parser)
{
	Assert(dim->mapping_spec.index_mapping.type == cm_INDEX_MAPPING_TIME);

	cm_Mapping *mapping = &dim->mapping_spec;

	u8 levels = mapping->index_mapping.time.depth;

	Assert(levels == array->length);

	cm_ColumnRef *basecol = dim->input_columns.begin;

	Assert(dim->input_columns.end - dim->input_columns.begin == 1);

	u32 timeidx = (basecol+0)->index;

	MemoryBlock *time_text = tokens_begin + timeidx;

	nm_TimeBinning *time_binning = &mapping->index_mapping.time.time_binning;

	s64 time = 0;
	switch(dim->mapping_spec.index_mapping.time.time_input_type) {
	case cm_TIME_INPUT_TYPE_UNIX_TIME: {
		s32 ok = 0;
		// losing the time fraction part of the data
		if (!pt_parse_s64(time_text->begin, time_text->end, &time)) {
			char *it=time_text->begin;
			while (it != time_text->end) {
				if (*it == '.') {
					break;
				}
				++it;
			}
			if (it == time_text->end) {
				return 0;
			} else if (!pt_parse_s64(time_text->begin, it, &time)) {
				return 0;
			}
		}
		tm_Time tm_time;
		tm_Time_init_from_unix_time(&tm_time, time);
		time = tm_time.time;
		// adjustment if it is coming as a calendar based representation
		time += mapping->index_mapping.time.minutes_to_add * tm_SECONDS_PER_MINUTE;
	}break;
	case cm_TIME_INPUT_TYPE_DATE: {
		if (!ntp_Parser_run(parser, time_text->begin, time_text->end)) {
			return 0;
		}
		time = parser->time.time;
		// adjustment if it is coming as a calendar based representation
		time += mapping->index_mapping.time.minutes_to_add * tm_SECONDS_PER_MINUTE;
	} break;
	case cm_TIME_INPUT_TYPE_BIN: {
		if (!pt_parse_s64(time_text->begin, time_text->end, &time)) {
			return 0;
		}
		time = time_binning->base_time.time + time_binning->bin_width * time;
	} break;
	};

	if (time < time_binning->base_time.time) {
		// @maybe 2017-06-27T17:12
		// report the reason why we didn't succeed on this record
		return 0;
	}

	u64 offset = (u64) (time - time_binning->base_time.time) / time_binning->bin_width;

	for (u8 i=0;i<levels;i++) {
		u8 label = (offset & 1) ? 1 : 0;
		offset >>= 1;
		nx_Array_set(array, levels - 1 - i, label);
	}

// 	Print_clear(print);
// 	Print_cstr(print, "path: ");
// 	for (u8 i=0;i<levels;i++) {
// 		u8 label = nx_Array_get(array,i);
// 		Print_u64(print, (u64) label);
// 	}
// 	Print_cstr(print, "\n");
// 	Request_print(request,print);

	return 1;
}

static b8
cm_mapping_hour(cm_Dimension *dim, nx_Array *array, MemoryBlock *tokens_begin, ntp_Parser *parser)
{
	Assert(dim->mapping_spec.index_mapping.type == cm_INDEX_MAPPING_HOUR);

	u8 levels = 1;

	Assert(levels == array->length);

	cm_ColumnRef *basecol = dim->input_columns.begin;

	Assert(dim->input_columns.end - dim->input_columns.begin == 1);

	u32 timeidx = (basecol+0)->index;

	MemoryBlock *time_text = tokens_begin + timeidx;
	if (!ntp_Parser_run(parser, time_text->begin, time_text->end)) {
		return 0;
	}

	nx_Array_set(array, 0, (u8) parser->label.hour);

	return 1;
}

static b8
cm_mapping_weekday(cm_Dimension *dim, nx_Array *array, MemoryBlock *tokens_begin, ntp_Parser *parser)
{
	Assert(dim->mapping_spec.index_mapping.type == cm_INDEX_MAPPING_WEEKDAY);

	u8 levels = 1;

	Assert(levels == array->length);

	cm_ColumnRef *basecol = dim->input_columns.begin;

	Assert(dim->input_columns.end - dim->input_columns.begin == 1);

	u32 timeidx = (basecol+0)->index;

	MemoryBlock *time_text = tokens_begin + timeidx;
	if (!ntp_Parser_run(parser, time_text->begin, time_text->end)) {
		return 0;
	}
	u8 weekday = tm_weekday(parser->label.year, parser->label.month, parser->label.day);

	nx_Array_set(array, 0, weekday);

	return 1;
}


static b8
cm_mapping_categorical(cm_Dimension *dim, nx_Array *array, MemoryBlock *tokens_begin)
{
	Assert(dim->mapping_spec.index_mapping.type == cm_INDEX_MAPPING_CATEGORICAL);
	// Need a map "string" -> "id" mapping
	return 0;
}

static void
cm_mapping_measure_write_number(nv_NumberStorage storage, char **it_payload, f64 value) {
	switch (storage) {
	case nv_NUMBER_STORAGE_UNSIGNED_32: {
		u32 *number = (u32*) *it_payload;
		*number = (u32) value;
		*it_payload += sizeof(u32);
	} break;
	case nv_NUMBER_STORAGE_UNSIGNED_64: {
		u64 *number = (u64*) *it_payload;
		*number = (u64) value;
		*it_payload += sizeof(u64);
	} break;
	case nv_NUMBER_STORAGE_FLOAT_32: {
		f32 *number = (f32*) *it_payload;
		*number = (f32) value;
		*it_payload += sizeof(f32);
	} break;
	case nv_NUMBER_STORAGE_FLOAT_64: {
		f64 *number = (f64*) *it_payload;
		*number = (f64) value;
		*it_payload += sizeof(f64);
	} break;
	case nv_NUMBER_STORAGE_SIGNED_32: {
		s32 *number = (s32*) *it_payload;
		*number = (s32) value;
		*it_payload += sizeof(s32);
	} break;
	case nv_NUMBER_STORAGE_SIGNED_64: {
		s64 *number = (s64*) *it_payload;
		*number = (s64) value;
		*it_payload += sizeof(s64);
	} break;
	default: {
		Assert(0 && "problem!");
	} break;
	}
}

static b8
cm_mapping_measure(cm_Dimension *dim, u64 offset, char **it_payload, MemoryBlock *tokens_begin, ntp_Parser *parser)
{
	Assert(dim->type == cm_MEASURE_DIMENSION);

	cm_Mapping *mapping = &dim->mapping_spec;

	Assert(mapping->mapping_type == cm_MEASURE_DIMENSION_MAPPING);

	s64 cols = dim->input_columns.end - dim->input_columns.begin;
	nv_NumberStorage storage = mapping->measure_mapping.storage_type;

	switch(mapping->measure_mapping.type) {
	case cm_MEASURE_MAPPING_PRODUCT: {
		f64 result = 1.0;
		cm_ColumnRef *it  = dim->input_columns.begin;
		cm_ColumnRef *end = dim->input_columns.end;
		while (it != end) {
			/* @TODO(llins): simple impl. but inefficient if we repeat columns */
			MemoryBlock *text = tokens_begin + (it->index);
			f64 value = 0.0;
			if (!pt_parse_f64(text->begin, text->end, &value)) {
				return 0;
			}
			result *= value;
			++it;
		}
		/* advances the it_payload pointer */
		cm_mapping_measure_write_number(storage, it_payload, result);
	} break;
	case cm_MEASURE_MAPPING_TIME_DURATION:
	case cm_MEASURE_MAPPING_TIME_DURATION_SQUARED: {
		if (cols != 2) {
			return 0;
		}

		MemoryBlock *time_text_a = tokens_begin + (dim->input_columns.begin->index);
		MemoryBlock *time_text_b = tokens_begin + ((dim->input_columns.begin+1)->index);

		s64 secs_from_epoch_a = 0;
		s64 secs_from_epoch_b = 0;

		if (!ntp_Parser_run(parser, time_text_a->begin, time_text_a->end)) {
			return 0;
		} else {
			secs_from_epoch_a = parser->time.time;
		}

		if (!ntp_Parser_run(parser, time_text_b->begin, time_text_b->end)) {
			return 0;
		} else {
			secs_from_epoch_b = parser->time.time;
		}

		f64 value = (secs_from_epoch_b - secs_from_epoch_a) / (f64) mapping->measure_mapping.time_unit_in_seconds;

		if (mapping->measure_mapping.type == cm_MEASURE_MAPPING_TIME_DURATION_SQUARED) {
			value = value * value;
		}

		/* advances the it_payload pointer */
		cm_mapping_measure_write_number(storage, it_payload, value);
	} break;
	case cm_MEASURE_MAPPING_ROW_BITSET: {
		u64 result = 1ull << offset;
		// NOTE(llins): only 52 rows will be all right, more then that overflows
		// the f64 mantissa
		cm_mapping_measure_write_number(storage, it_payload, (f64) result);
	} break;
	default: {
		return 0;
	} break;
	}

	return 1;

}

