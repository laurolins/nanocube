
//------------------------------------------------------------------------------
// Code to read .dmp files
//------------------------------------------------------------------------------

static const char *ndmp_load_info_tokens[] = {
	"field",
	"nc_dim_quadtree_",
	"nc_dim_cat_",
	"nc_dim_time_",
	"nc_var_uint_"
};

// static const int TOKEN_TABLE_BEGIN = 0;
#define ndmp_TOKEN_TABLE_END 5
#define ndmp_TOKEN_TYPE_BEGIN 1
#define ndmp_TOKEN_TYPE_END 5
#define ndmp_TOKEN_FIELD 0
#define ndmp_TOKEN_QUADTREE 1
#define ndmp_TOKEN_CATEGORICAL 2
#define ndmp_TOKEN_TIME 3
#define ndmp_TOKEN_VAR_UINT 4

typedef struct {
	int formats[64];
	int resolutions[64];
	int variable_size;
	int record_size;
	int num_dimensions;
	int depth;	// total depth of the hierarchies
	u64 length; // length of the load_info in bytes (data starts at this offset)
} ndmp_LoadInfo;

typedef enum {
	ndmp_LOAD_INFO_OK = 0,
	ndmp_LOAD_INFO_NO_FIELD_NAME = 1,
	ndmp_LOAD_INFO_UNKNOWN_FIELD_TYPE = 2,
	ndmp_LOAD_INFO_INVALID_FIELD_RESOLUTION= 3,
	ndmp_LOAD_INFO_FIELD_RESOLUTION_NOT_SUPPORTED=4,
	ndmp_LOAD_INFO_VARIABLE_NOT_LAST_FIELD= 5,
	ndmp_LOAD_INFO_UNRECOGNIZED_TYPE = 6
} ndmp_LoadInfo_Error;

// read input load_info from file
static ndmp_LoadInfo_Error
ndmp_LoadInfo_init(ndmp_LoadInfo* load_info, nv_Nanocube* nanocube, MemoryBlock *filename)
{

	Assert(nanocube->initialized
	       && nanocube->num_index_dimensions==0
	       && nanocube->index_initialized==0);

	load_info->num_dimensions = 0;
	load_info->depth = 0;
	load_info->variable_size = 0;
	load_info->record_size = 0;

	// assumes separator consists of two lines

	char buffer[Kilobytes(4)];

	pt_File input_file = platform.open_file(filename->begin, filename->end, pt_FILE_READ);

	nu_FileTokenizer ftok;
	nu_FileTokenizer_init(&ftok, '\n', buffer, buffer + sizeof(buffer), &input_file);

	// char token[Kilobytes(1)];

	nu_BufferTokenizer btok;

	while (nu_FileTokenizer_next(&ftok)) {

		if (ftok.token_empty) // signal that data is coming
			break;

		//		fprintf(stderr, "%s\n", ftok.buffer + ftok.token_begin);

		// @todo add mechanism to skip empty tokens
		nu_BufferTokenizer_init(&btok, ' ',
					ftok.token.begin,
					ftok.token.end);

		if (!nu_BufferTokenizer_next(&btok)) { continue; }

		// log
		{
			//auto n = BufferTokenizer_copy_token(&btok, token, sizeof(token)-1);
			//token[n] = 0;
			//fprintf(stderr, "---> %s	[%d] \n", token, btok.overflow);
		}

		s64 tok_index =
			nu_first_match_on_cstr_prefix_table(ndmp_load_info_tokens,
							    ndmp_TOKEN_FIELD,
							    ndmp_TOKEN_FIELD + 1,
							    btok.token.begin,
							    btok.token.end);

		if (tok_index != ndmp_TOKEN_FIELD) {
			continue;
		}

		// read next non empty token
		if (!nu_BufferTokenizer_next(&btok)) {
			//	       fprintf(stderr,"[Error] No field name?\n");
			return(ndmp_LOAD_INFO_NO_FIELD_NAME);
		}

		MemoryBlock field_name = btok.token;

		if (!nu_BufferTokenizer_next(&btok)) {
			//	       fprintf(stderr,"[Error] No field type!\n");
			return(ndmp_LOAD_INFO_NO_FIELD_NAME);
		}

		MemoryBlock field_type = btok.token;

		s64 type_index =
			nu_first_match_on_cstr_prefix_table(
							    ndmp_load_info_tokens,
							    ndmp_TOKEN_TYPE_BEGIN,
							    ndmp_TOKEN_TYPE_END,
							    field_type.begin,
							    field_type.end);

		if (type_index == -1) {
			//	       fprintf(stderr,"[Error] Unkown field type\n");
			return(ndmp_LOAD_INFO_UNKNOWN_FIELD_TYPE);
		}

		// check resolution of type for input purposes
		nu_BufferTokenizer rtok; // resolution tokenizer
		nu_BufferTokenizer_init(&rtok, '_',
					field_type.begin,
					field_type.end);

		MemoryBlock resolution_text = rtok.token;
		while (nu_BufferTokenizer_next(&rtok)) { // annotate last token
			resolution_text = rtok.token;
		}

		if (resolution_text.begin == resolution_text.end ||
		    resolution_text.end-resolution_text.begin > 9) {
			//	       fprintf(stderr,"[Error] Could not decode resolution of field type!\n");
			return(ndmp_LOAD_INFO_INVALID_FIELD_RESOLUTION);
		}

		// @todo library call to covert decimal str into a number
		s32 resolution;
		if (!pt_parse_s32(resolution_text.begin, resolution_text.end,
				  &resolution)
		    || resolution < 1
		    || resolution > 127) {
			//	       fprintf(stderr,"[Error] Field resolution not supported\n");
			return(ndmp_LOAD_INFO_FIELD_RESOLUTION_NOT_SUPPORTED);
		}

		if (type_index == ndmp_TOKEN_VAR_UINT
		    && load_info->variable_size == 0) {
			load_info->variable_size = resolution;
			load_info->record_size  += resolution;

			char measure_name[] = "count";

			nv_Nanocube_insert_measure_dimension
				(nanocube, nv_NUMBER_STORAGE_UNSIGNED_32,
				 measure_name, cstr_end(measure_name));

			continue; // should be the last dimension
		} else if (load_info->variable_size > 0) {
			//	       fprintf(stderr,"[error] variable needs to be the last field.\n");
			return(ndmp_LOAD_INFO_VARIABLE_NOT_LAST_FIELD);
		}

		// annotate everything
		int index = load_info->num_dimensions;
		load_info->formats[index] = (int) type_index;
		load_info->resolutions[index] = resolution;

		int num_levels = 0;
		int bits_per_level = 0;

		if (type_index == ndmp_TOKEN_QUADTREE) {
			num_levels = resolution;
			bits_per_level = 2;
			load_info->record_size += 8; // +8 bites
		} else if (type_index == ndmp_TOKEN_CATEGORICAL) {
			num_levels = 1;
			bits_per_level = 8;
			load_info->record_size += 1;
		} else if (type_index == ndmp_TOKEN_TIME) {
			num_levels = resolution * 8;
			bits_per_level = 1;
			load_info->record_size += resolution;
		} else {
			//	       fprintf(stderr,"[Error] Type not recognized.\n");
			return(ndmp_LOAD_INFO_UNRECOGNIZED_TYPE);
		}

		nv_Nanocube_insert_index_dimension(nanocube,
						   (u8) bits_per_level,
						   (u8) num_levels,
						   field_name.begin,
						   field_name.end);

		load_info->depth += num_levels;
		++load_info->num_dimensions;

		//	   fprintf(stderr,"%s\n",ftok.buffer + ftok.token_begin);

	}


	//     fprintf(stderr,"%lld\n",ftok.file_offset);

	load_info->length = ftok.file_offset;

	platform.close_file(&input_file);

	return ndmp_LOAD_INFO_OK;
}

