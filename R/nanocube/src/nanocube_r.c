#define USE_RINTERNALS 1
#define R_NO_REMAP

#include <R.h>
#include <Rinternals.h>

#include <stdlib.h>
#include <stdio.h>

#include "src/base/platform.c"
#include "src/base/alloc.c"
#include "src/base/time.c"
#include "src/base/tokenizer.c"
#include "src/base/time_parser.c"
#include "src/base/profile.c"
#include "src/nanocube/nanocube_btree.c"
#include "src/nanocube/nanocube_vector_payload.h"
#include "src/nanocube/nanocube_index.c"
#include "src/nanocube/nanocube_parser.c"
#include "src/nanocube/nanocube_measure.c"

// platform util assumes a platform global variable
// is available
global_variable PlatformAPI platform;
global_variable b8          g_platform_initialized = 0;

#include "src/nanocube/nanocube_vector.c"

#ifdef WINDOWS_PLATFORM
#define export_procedure __declspec(dllimport)
#define init_platform win32_init_platform
/* maybe add some options to trigger the correct OS */
#include <windows.h>

#include "src/platform_dependent/win32_platform.c"

#else

/* no export tag for osx and linux */
#define export_procedure

/* common headers on osx and linux */
#include <dlfcn.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include <semaphore.h>
#include <pthread.h>

#if OSX_PLATFORM

/* osx specific headers*/
#define init_platform osx_init_platform

/* #include <mach-o/dyld.h> */
#include "src/platform_dependent/osx_platform.c"

#elif LINUX_PLATFORM

#define init_platform linux_init_platform

/* linux specific headers*/

#include "src/platform_dependent/linux_platform.c"

#endif

#endif

internal PlatformAPI*
ncr_get_platform()
{
	if (!g_platform_initialized) {
		init_platform(&platform);
		g_platform_initialized = 1;
	}
	return &platform;
}


#define ncr_ENGINE_MAX_BLOCKS            1024
#define ncr_ENGINE_LOG_MAX_SIZE          1024
#define ncr_ENGINE_CLASS         "ncr_Engine"
#define ncr_ENGINE_MEM_COMPILER            32
#define ncr_ENGINE_MEM_TABLE_INDEX_COLUMNS 64
#define ncr_ENGINE_MEM_TABLE_VALUE_COLUMNS 64


/* the result of loading one or multiple nanocubes is an engine object */
typedef struct {
	pt_Memory             blocks[ncr_ENGINE_MAX_BLOCKS];
	s32                   num_blocks;

	pt_MappedFile         mapped_files[ncr_ENGINE_MAX_BLOCKS];
	s32                   num_mapped_files;

	LinearAllocator       compiler_allocator;
	np_CompilerCheckpoint compiler_checkpoint;
	np_Compiler           compiler;
	b8                    ok;

	LinearAllocator       table_index_columns_allocator;
	LinearAllocator       table_value_columns_allocator;

	char                  log_buffer[ncr_ENGINE_LOG_MAX_SIZE];
	Print                 log;
} ncr_Engine;

internal MemoryBlock
ncr_Engine_allocate_memory(ncr_Engine *self, u64 size, u8 alignment, u64 preferred)
{
	PlatformAPI *pt = ncr_get_platform();
	Assert(self->num_blocks < ncr_ENGINE_MAX_BLOCKS-1);
	self->blocks[self->num_blocks] = pt->allocate_memory(size, alignment, preferred);
	MemoryBlock result = self->blocks[self->num_blocks].memblock;
	++self->num_blocks;
	return result;
}

internal void
ncr_Engine_init(ncr_Engine *self)
{
	/* initialize nx_ module */
	nx_initialize();

	self->num_blocks       = 0;
	self->num_mapped_files = 0;
	self->ok               = 1;

	Print_init(&self->log, self->log_buffer, self->log_buffer + sizeof(self->log_buffer));

	/* one block as the nanocube query compiler memory */
	MemoryBlock mem_compiler            = ncr_Engine_allocate_memory(self, Megabytes(ncr_ENGINE_MEM_COMPILER), 3, 0);
	MemoryBlock mem_table_index_columns = ncr_Engine_allocate_memory(self, Megabytes(ncr_ENGINE_MEM_TABLE_INDEX_COLUMNS), 3, 0);
	MemoryBlock mem_table_value_columns = ncr_Engine_allocate_memory(self, Megabytes(ncr_ENGINE_MEM_TABLE_VALUE_COLUMNS), 3, 0);

	LinearAllocator_init(&self->compiler_allocator,
			     mem_compiler.begin,
			     mem_compiler.begin,
			     mem_compiler.end);
	LinearAllocator_init(&self->table_index_columns_allocator,
			     mem_table_index_columns.begin,
			     mem_table_index_columns.begin,
			     mem_table_index_columns.end);
	LinearAllocator_init(&self->table_value_columns_allocator,
			     mem_table_value_columns.begin,
			     mem_table_value_columns.begin,
			     mem_table_value_columns.end);

	/* basic nanocube compiler initializetion */
	np_Compiler_init(&self->compiler, &self->compiler_allocator);

	/* nanocube vector compiler initialization */
	nv_Compiler_init(&self->compiler);
}

internal void
ncr_Engine_release(ncr_Engine *self)
{
	// printf("releasing memory...\n");
	PlatformAPI *pt = ncr_get_platform();
	LinearAllocator_clear(&self->compiler_allocator);
	for (s32 i=0;i<self->num_blocks;++i) {
		pt->free_memory(self->blocks + i);
	}
	for (s32 i=0;i<self->num_mapped_files;++i) {
		/* should be mapped to be released */
		pt->close_mmap_file(self->mapped_files + i);
	}
	self->num_blocks       = 0;
	self->num_mapped_files = 0;
}

internal void
ncr_Engine_log_cstr(ncr_Engine *self, char *str)
{
	Print_cstr(&self->log, str);
}

internal b8
ncr_Engine_load_and_register_nanocube(ncr_Engine *self,
				      char *alias_begin,
				      char *alias_end,
				      char *filename_begin,
				      char *filename_end,
				      b8 cache)
{
	/*
	 * if cache is true, allocate a whole block
	 * of memory and load file into memory
	 */
	char *block_begin = 0;
	if (cache) {
		pt_File nanocube_file = platform.open_read_file(filename_begin, filename_end);
		if (!nanocube_file.open) {
			ncr_Engine_log_cstr(self, "[serve] couldn't open source file.");
			self->ok = 0;
			return 0;
		}
		/* bring all file content to memory */
		pt_Memory mem = platform.allocate_memory(nanocube_file.size, 12, 0);
		if (!mem.handle) {
			ncr_Engine_log_cstr(self, "[serve] couldn't allocate memory for one of the anchored sources.");
			self->ok = 0;
			return 0;
		}
		platform.read_next_file_chunk(&nanocube_file, mem.memblock.begin, mem.memblock.end);
		platform.close_file(&nanocube_file);

		self->blocks[self->num_blocks] = mem;
		++self->num_blocks;

		block_begin = mem.memblock.begin;
	} else {
		/* mmap file */
		self->mapped_files[self->num_mapped_files] =
			platform.open_mmap_file(filename_begin, filename_end, 1, 0);

		pt_MappedFile *mapped_file = self->mapped_files + self->num_mapped_files;
		if (!mapped_file->mapped) {
			ncr_Engine_log_cstr(self, "[serve] couldn't mmap source file.");
			self->ok = 0;
			return 0;
		}
		/* if mapped count as mapped */
		++self->num_mapped_files;
		block_begin = mapped_file->begin;
	}

	al_Allocator*  allocator = (al_Allocator*)  block_begin;
	nv_Nanocube*   cube      = (nv_Nanocube*)   al_Ptr_char_get(&allocator->root_p);
	nv_Compiler_insert_nanocube(&self->compiler, cube, alias_begin, alias_end);
	self->compiler_checkpoint = np_Compiler_checkpoint(&self->compiler);
	return 1;
}

internal b8
ncr_is_engine(SEXP x) {
    return Rf_inherits(x, ncr_ENGINE_CLASS);
}

internal void
ncr_free_engine(SEXP x) {
	if (ncr_is_engine(x)) {
		ncr_Engine *engine = (ncr_Engine*) R_ExternalPtrAddr(x);
		ncr_Engine_release(engine);
		free(engine);
	}
}

export_procedure SEXP
ncr_messages(SEXP sEngine)
{
	if (!ncr_is_engine(sEngine))
		return R_NilValue;
	ncr_Engine *engine = (ncr_Engine*) R_ExternalPtrAddr(sEngine);
	SEXP res = PROTECT(Rf_allocVector(STRSXP, 1));
	SET_STRING_ELT(res, 0, Rf_mkCharLen(engine->log.begin, (s32)(engine->log.end - engine->log.begin)));
	UNPROTECT(1);
	return res;
}


/*
 * Expects an array of strings: each string is either "<filename>"
 * or "<handle>=<filename>". Returns a handle that can be used
 * to query one or multiple data sources.
 */
export_procedure SEXP
ncr_load(SEXP sFiles)
{
	/* assumes sFiles is a vector of strings */

	ncr_Engine *engine = (ncr_Engine*) malloc(sizeof(ncr_Engine));

	// printf("loading engine...\n");

	ncr_Engine_init(engine);

	// char **names = VHAR(sFiles);
	s32  length  = LENGTH(sFiles);

	b8 ok = 1;

	nt_Tokenizer tok;
	char sep[] = ",";
	nt_Tokenizer_init_canonical(&tok, sep, cstr_end(sep));

	for (s32 i=0;i<length;++i) {

		/* removes const */
		char *st_begin = (char*) CHAR(STRING_ELT(sFiles,i));
		char *st_end   = cstr_end(st_begin);

		char *eq = pt_find_char(st_begin, st_end, '=');
		b8 alias = eq != st_end;
		char *alias_begin = st_begin;
		char *alias_end   = eq;
		if (!alias || alias_begin == alias_end) {
			ncr_Engine_log_cstr(engine, "[error] usage is <alias>=<NC1>(\",\"<NC>)*. (alias cannot be empty)\n");
			ncr_Engine_log_cstr(engine, st_begin);
			ncr_Engine_log_cstr(engine, "\n");
			engine->ok = 0;
			break;
		}

		nt_Tokenizer_reset_text(&tok, eq+1, st_end);
		s32 num_nanocubes = 0;
		while (nt_Tokenizer_next(&tok)) {

			char *filename_begin = tok.token.begin;
			char *filename_end   = tok.token.end;

			if (filename_begin == filename_end) {
				continue;
			}

			b8 cache = 0;
			if (*filename_begin == '@') {
				cache = 1;
				++filename_begin;
			}

			if (filename_begin == filename_end) {
				continue;
			}

			b8 ok = ncr_Engine_load_and_register_nanocube(engine, alias_begin, alias_end,
								      filename_begin, filename_end,
								      cache);
			if (!ok) {
				engine->ok = 0;
				break;
			}
			++num_nanocubes;
		}
		if (num_nanocubes == 0) {
			ncr_Engine_log_cstr(engine, "[error] alias without nanocube associated: ");
			ncr_Engine_log_cstr(engine, st_begin);
			ncr_Engine_log_cstr(engine, "\n");
			engine->ok = 0;
			break;
		}
	}

	if (!engine->ok) {
		// printf("releasing engine...\n");
		/* release resources, but engine is still valid (contains error messages) */
		ncr_Engine_release(engine);
		// printf("returning nil...\n");
	}

	SEXP res = PROTECT(R_MakeExternalPtr(engine, R_NilValue, R_NilValue));
	R_RegisterCFinalizerEx(res, ncr_free_engine, TRUE);
	Rf_setAttrib(res, Rf_install("class"), Rf_mkString(ncr_ENGINE_CLASS));
	UNPROTECT(1);
	return res;
}

#define print_size_of(name) \
	printf("%-32s%d\n",#name,(s32)sizeof(name));

export_procedure SEXP ncr_query(SEXP sEngine, SEXP sQuery)
{
	if (!ncr_is_engine(sEngine))
		return R_NilValue;

	ncr_Engine *engine    = (ncr_Engine*) R_ExternalPtrAddr(sEngine);
	np_Compiler *compiler = &engine->compiler;
	np_Compiler_goto_checkpoint(compiler, engine->compiler_checkpoint);

	Print *print = &engine->log;
	Print_clear(print);

	char *query_begin = (char*) CHAR(STRING_ELT(sQuery,0));
	char *query_end   = cstr_end(query_begin);

// 	print_size_of(nv_Nanocube);
// 	print_size_of(nx_NanocubeIndex);
// 	print_size_of(nm_Measure);
// 	printf("query: %s\n", query_begin);

	// treat current token as the query
	nt_Tokenizer scanner;
	np_initialize_tokenizer(&scanner, query_begin, query_end);
	np_Parser parser;
	np_Parser_init(&parser, &scanner, compiler->memory);
	b8 ok = np_Parser_run(&parser);

	if (!ok) {
		Print_cstr(print, "104 Syntax Error.\n");
		return R_NilValue;
	}

	np_TypeValue last_statement;
	last_statement = np_Compiler_reduce(compiler, parser.ast_first, query_begin, query_end);

	if (!compiler->reduce.success) {
		Print_cstr(print, "105 Reduce Error\n");
		Print_str(print, compiler->reduce.log.begin, compiler->reduce.log.end);
		return R_NilValue;
	}

	/* @TODO(llins): check what is the type of the result. */
	Assert(!last_statement.error);

	if (last_statement.type_id != nv_compiler_types.measure) {
		Print_cstr(print, "106 Expected last term to be a measure expression\n");
		Print_str(print, compiler->reduce.log.begin, compiler->reduce.log.end);
		return R_NilValue;
	}

	nm_Measure *measure = (nm_Measure*) last_statement.value;

	nm_Services nv_payload_services;
	nv_payload_services_init(&nv_payload_services);

	/* reset pre-reserved memory for preparing response */
	LinearAllocator_clear(&engine->table_index_columns_allocator);
	LinearAllocator_clear(&engine->table_value_columns_allocator);

	nm_Table *table_begin = nm_Measure_eval(measure,
						&engine->table_index_columns_allocator,
						&nv_payload_services,
						&engine->table_value_columns_allocator);
	nm_Table *table_end   = table_begin + 1; //  measure->num_sources;
	nm_Table *table       = table_begin;

	// buffer for key printing
	char buffer[64];
	Print print_key;

	if (table) {

		/* data frame */
		nm_TableKeys   *table_keys   = &table->table_keys;
		nv_TableValues *table_values = (nv_TableValues*) table->table_values_handle;

		s32 columns = (s32) table_keys->columns + (s32) table_values->columns;
		u32 rows    = table_keys->rows;

// 		printf("columns:     %d\n",columns);
// 		printf("row_length:  %d\n",table_keys->row_length);

		/* allocate a data frame with u64 bits and floating point vectors */
		SEXP res = PROTECT(Rf_allocVector(VECSXP, columns));
		{
			u32 col = 0;
			u32 key_record_size = table_keys->row_length;
			u8  key_offset = 0;
			for (u32 j=0;j<table_keys->columns;++j) {
				nm_TableKeysColumnType *coltype = table_keys->type->begin + col;
				if (coltype->loop_column || (coltype->flags & nm_BINDING_FLAG_TEXT) == 0) {
					// standard way of encoding (in R try to squeeze path into a linear number (up to 52 bits))
					f64  *it_dst = REAL(SET_VECTOR_ELT(res, col, Rf_allocVector(REALSXP, rows)));
					char *it_src = table_keys->keys.begin + key_offset;
					if (coltype->loop_column) {
						for (u32 i=0;i<rows;++i) {
							u32 val = *((u32*) it_src);
							*it_dst = (f64) val;
							it_dst += 1;
							it_src += key_record_size;
						}
						key_offset += sizeof(u32);
					} else {
						u32 bits   = coltype->bits;
						u32 levels = coltype->levels;
						u32 bytes  = (coltype->bits * coltype->levels + 7)/8;

						/* assume no loss for now */
						Assert(bits * levels < 53);
						for (u32 i=0;i<rows;++i) {
							u64 value = 0;
							char* p = (char*) &value;
							pt_copy_bytes(it_src, it_src + bytes, (char*) p, p + bytes);
							*it_dst = (f64) value;
							it_dst += 1;
							it_src += key_record_size;
						}
						key_offset += bytes;
					}
					++col;
				} else {
					// more expensive way to write out result: search for the
					// text representation into the btree of the nanocube
					SEXP names   = SET_VECTOR_ELT(res, col, Rf_allocVector(STRSXP, rows));
					char *it_src = table_keys->keys.begin + key_offset;

					u32 bits   = coltype->bits;
					u32 levels = coltype->levels;
					u32 bytes  = (coltype->bits * coltype->levels + 7)/8;

					Print_init(&print_key, buffer, buffer + ArrayCount(buffer));
					Print_str(&print_key, coltype->name.begin, coltype->name.end);
					Print_cstr(&print_key,":kv:");
					char *print_key_chkpt = Print_checkpoint(&print_key);

					/* assume no loss for now */
					Assert(bits * levels < 53);
					for (u32 i=0;i<rows;++i) {
						u64 value = 0;
						char* p = (char*) &value;
						pt_copy_bytes(it_src, it_src + bytes, (char*) p, p + bytes);

						Assert(table->source != 0);
						Assert(table->source->num_nanocubes > 0);

						/* force the way down */
						nv_Nanocube *nanocube = (nv_Nanocube*) table->source->nanocubes[0];

						// search for keyword on the right cube... which one
						// is the right cube?
						Print_rewind(&print_key, print_key_chkpt);
						Print_u64(&print_key, value);
						MemoryBlock text = nv_Nanocube_get_key_value(nanocube, print_key.begin, print_key.end);
						SET_STRING_ELT(names, i, Rf_mkCharLen(text.begin, (s32)(text.end-text.begin)));

						it_src += key_record_size;
					}
					key_offset += bytes;
					++col;
				}
			}
			for (u32 j=0;j<table_values->columns;++j) {
				f64 *it_dst = REAL(SET_VECTOR_ELT(res, col, Rf_allocVector(REALSXP, rows)));
				f64* it_src = table_values->values.begin + j;
				for (u32 i=0;i<rows;++i) {
					*it_dst = *it_src;
					it_src += table_values->columns;
					++it_dst;
				}
				++col;
			}
		}
		/* reserve name vector for columns */
		{
			Rf_setAttrib(res, R_NamesSymbol, Rf_allocVector(STRSXP, columns));
			SEXP names = Rf_getAttrib(res, R_NamesSymbol);
			u32 col = 0;
			for (u32 j=0;j<table_keys->columns;++j) {
				char *begin = (table_keys->type->begin + j)->name.begin;
				char *end   = (table_keys->type->begin + j)->name.end;
				SET_STRING_ELT(names, col, Rf_mkCharLen(begin, (s32)(end-begin)));
				++col;
			}
			for (u32 j=0;j<table_values->columns;++j) {
				char *begin = (table_values->names.begin + j)->begin;
				char *end   = (table_values->names.begin + j)->end;
				SET_STRING_ELT(names, col, Rf_mkCharLen(begin, (s32)(end-begin)));
				++col;
			}
		}
// 		{
// 			Rf_setAttrib(res, R_DimSymbol, Rf_allocVector(INTSXP, 2));
// 			SEXP dims = Rf_getAttrib(res, R_DimSymbol);
// 			INTEGER(dims)[0] = rows;
// 			INTEGER(dims)[1] = columns;
// 		}
#if 1
		{
			Rf_setAttrib(res, R_RowNamesSymbol, Rf_allocVector(INTSXP, rows));
			SEXP numbers = Rf_getAttrib(res, R_RowNamesSymbol);
			for (u32 i=0;i<rows;++i) {
				INTEGER(numbers)[i] = (s32) (i + 1);
			}
		}
		Rf_setAttrib(res, R_ClassSymbol, Rf_mkString("data.frame"));
#endif
		UNPROTECT(1);
		return res;
	} else {
		Print_cstr(print, "107 Problem evaluating measure\n");
		return R_NilValue;
	}

// 	/* TODO(llins): too much text being pushed into Print object */
// 	static char txt[] = "txt";
// 	while (table != table_end) {
// 		np_Symbol *sym_is_text = np_SymbolTable_find_variable(&compiler->symbol_table, txt, cstr_end(txt));
// 		if (sym_is_text) {
// 			if (sym_is_text->variable.type_id == compiler->number_type_id) {
// 				s64 value = (s64) *((f64*)sym_is_text->variable.value);
// 				if (value) {
// 					nv_print_table(print, table);
// 				} else {
// 					nv_print_table_json(print, table);
// 				}
// 			}
// 		} else {
// 			nv_print_table_json(print, table);
// 		}
// 		++table;
// // 		nv_print_table(print, table);
// // 		++table;
// 	}
//
// 	SEXP res = PROTECT(Rf_allocVector(STRSXP, 1));
// 	SET_STRING_ELT(res, 0,
// 		       Rf_mkCharLen(print->begin, (s32)(print->end - print->begin)));
// 	UNPROTECT(1);
// 	return res;
}

