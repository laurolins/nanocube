
//------------------------------------------------------------------------------
// Options
//------------------------------------------------------------------------------

typedef struct op_Parameter {

	MemoryBlock value;

	struct op_Parameter *next;

} op_Parameter;


typedef struct op_NamedParameter {

	MemoryBlock name;

	op_Parameter *first_parameter;
	op_Parameter *last_parameter;

	struct op_NamedParameter *next;

} op_NamedParameter;

/* positional and named parameters */
typedef struct {

	op_NamedParameter *first_named_parameter;
	op_NamedParameter *last_named_parameter;

	op_Parameter *first_positioned_parameter;
	op_Parameter *last_positioned_parameter;

	LinearAllocator allocator;

} op_Options;

/*
 * Parameter
 */

internal void
op_Parameter_init(op_Parameter *self, char *value_begin, char *value_end)
{
	self->value.begin = value_begin;
	self->value.end   = value_end;
	self->next        = 0;
}

internal u32
op_Parameter_count(op_Parameter *self)
{
	u32 count = 0;
	op_Parameter *it = self;
	while (it) {
		++count;
		it = it->next;
	}
	return count;
}

internal b8
op_Parameter_value_u64(op_Parameter *self, u64 *output)
{
	if (!self) {
		return 0;
	}
	if (!pt_parse_u64(self->value.begin, self->value.end, output)) {
		return 0;
	} else {
		return 1;
	}
}

internal b8
op_Parameter_value_f32(op_Parameter *self, f32 *output)
{
	if (!self) {
		return 0;
	}
	if (!pt_parse_f32(self->value.begin, self->value.end, output)) {
		return 0;
	} else {
		return 1;
	}
}

internal b8
op_Parameter_value_s32(op_Parameter *self, s32 *output)
{
	if (!self) {
		return 0;
	}
	if (!pt_parse_s32(self->value.begin, self->value.end, output)) {
		return 0;
	} else {
		return 1;
	}
}


internal op_Parameter*
op_Parameter_at(op_Parameter *self, u32 index)
{
	u32 i = 0;
	op_Parameter *it = self;
	while (it) {
		if (i == index) {
			return it;
		} else {
			++i;
			it=it->next;
		}
	}
	return 0;
}

/*
 * NamedParameter
 */

internal void
op_NamedParameter_init(op_NamedParameter *self, char *name_begin, char *name_end)
{
	self->name.begin      = name_begin;
	self->name.end        = name_end;
	self->first_parameter = 0;
	self->last_parameter  = 0;
	self->next            = 0;
}

internal op_NamedParameter*
op_NamedParameter_at(op_NamedParameter *self, u32 index)
{
	u32 i = 0;
	op_NamedParameter *it = self;
	while (it) {
		if (i == index) {
			return it;
		} else {
			++i;
			it = it->next;
		}
	}
	return 0;
}

internal op_NamedParameter*
op_NamedParameter_find(op_NamedParameter *self, char *name_begin, char *name_end)
{
	op_NamedParameter *it = self;
	while (it) {
		if (pt_compare_memory(name_begin, name_end, it->name.begin, it->name.end) == 0) {
			return it;
		} else {
			it = it->next;
		}
	}
	return 0;
}

internal u32
op_NamedParameter_count(op_NamedParameter *self)
{
	u32 count = 0;
	op_NamedParameter *it = self;
	while (it) {
		++count;
		it = it->next;
	}
	return count;
}

/*
 * Options
 */

internal void
op_Options_init(op_Options *self, char *buffer_begin, char *buffer_end)
{
	self->first_named_parameter = 0;
	self->last_named_parameter  = 0;

	self->first_positioned_parameter = 0;
	self->last_positioned_parameter  = 0;

	LinearAllocator_init(&self->allocator, buffer_begin, buffer_begin, buffer_end);
}

internal void
op_Options_reset(op_Options *self)
{
	self->first_named_parameter = 0;
	self->last_named_parameter  = 0;

	self->first_positioned_parameter = 0;
	self->last_positioned_parameter  = 0;

	LinearAllocator_clear(&self->allocator);
}

internal u32
op_Options_num_positioned_parameters(op_Options *self)
{
	return op_Parameter_count(self->first_positioned_parameter);
}

internal u32
op_Options_num_named_parameters(op_Options *self)
{
	return op_NamedParameter_count(self->first_named_parameter);
}

internal b8
op_Options_u64(op_Options *self, u32 index, u64 *output)
{
	op_Parameter *parameter = op_Parameter_at(self->first_positioned_parameter, index);
	if (!parameter) {
		return 0;
	} else {
		return op_Parameter_value_u64(parameter, output);
	}
}

internal b8
op_Options_f32(op_Options *self, u32 index, f32 *output)
{
	op_Parameter *parameter = op_Parameter_at(self->first_positioned_parameter, index);
	if (!parameter) {
		return 0;
	} else {
		return op_Parameter_value_f32(parameter, output);
	}
}

internal b8
op_Options_s32(op_Options *self, u32 index, s32 *output)
{
	op_Parameter *parameter = op_Parameter_at(self->first_positioned_parameter, index);
	if (!parameter) {
		return 0;
	} else {
		return op_Parameter_value_s32(parameter, output);
	}
}

internal b8
op_Options_str(op_Options *self, u32 index, MemoryBlock *memblock)
{
	op_Parameter *parameter = op_Parameter_at(self->first_positioned_parameter, index);
	if (!parameter) {
		return 0;
	} else {
		*memblock = parameter->value;
		return 1;
	}
}

internal op_NamedParameter*
op_Options_named_parameter_at(op_Options *self, u32 index)
{
	return op_NamedParameter_at(self->first_named_parameter, index);
}

internal op_NamedParameter*
op_Options_find(op_Options *self, char *name_begin, char *name_end)
{
	return op_NamedParameter_find(self->first_named_parameter, name_begin, name_end);
}

internal op_NamedParameter*
op_Options_find_cstr(op_Options *self, char *name)
{
	return op_Options_find(self, name, cstr_end(name));
}

internal b8
op_Options_named_u64(op_Options *self, char *name_begin, char *name_end, u32 value_index, u64 *output)
{
	op_NamedParameter *named_parameter = op_Options_find(self, name_begin, name_end);
	if (named_parameter) {
		op_Parameter *parameter = op_Parameter_at(named_parameter->first_parameter, value_index);
		if (parameter) {
			return op_Parameter_value_u64(parameter, output);
		} else {
			return 0;
		}
	} else {
		return 0;
	}
}

internal b8
op_Options_named_f32(op_Options *self, char *name_begin, char *name_end, u32 value_index, f32 *output)
{
	op_NamedParameter *named_parameter = op_Options_find(self, name_begin, name_end);
	if (named_parameter) {
		op_Parameter *parameter = op_Parameter_at(named_parameter->first_parameter, value_index);
		if (parameter) {
			return op_Parameter_value_f32(parameter, output);
		} else {
			return 0;
		}
	} else {
		return 0;
	}
}

internal b8
op_Options_named_str(op_Options *self, char *name_begin, char *name_end, u32 value_index, MemoryBlock *output)
{
	op_NamedParameter *named_parameter = op_Options_find(self, name_begin, name_end);
	if (named_parameter) {
		op_Parameter *parameter = op_Parameter_at(named_parameter->first_parameter, value_index);
		if (parameter) {
			*output = parameter->value;
			return 1;
		} else {
			return 0;
		}
	} else {
		return 0;
	}
}

internal b8
op_Options_named_str_cstr(op_Options *self, char *name, u32 value_index, MemoryBlock *output)
{
	return op_Options_named_str(self, name, cstr_end(name), value_index, output);
}

internal b8
op_Options_named_u64_cstr(op_Options *self, char *name, u32 value_index, u64 *output)
{
	return op_Options_named_u64(self, name, cstr_end(name), value_index, output);
}

internal b8
op_Options_named_f32_cstr(op_Options *self, char *name, u32 value_index, f32 *output)
{
	return op_Options_named_f32(self, name, cstr_end(name), value_index, output);
}

internal op_NamedParameter*
op_Options_insert_named_parameter(op_Options *self, char *name_begin, char *name_end)
{
	// fprintf(stderr,"append named parameter  %.*s\n", (s32) (name_end - name_begin), name_begin);
	op_NamedParameter *named_parameter = (op_NamedParameter*) LinearAllocator_alloc(&self->allocator, sizeof(op_NamedParameter));
	op_NamedParameter_init(named_parameter, name_begin, name_end);
	if (!self->last_named_parameter) {
		self->first_named_parameter = named_parameter;
		self->last_named_parameter  = named_parameter;
	} else {
		self->last_named_parameter->next = named_parameter;
		self->last_named_parameter       = named_parameter;
	}
	return named_parameter;
}

internal op_Parameter*
op_Options_append_parameter(op_Options *self, char *value_begin, char *value_end)
{
	// fprintf(stderr,"append parameter %.*s\n", (s32) (value_end - value_begin), value_begin);
	op_Parameter *parameter = (op_Parameter*) LinearAllocator_alloc(&self->allocator, sizeof(op_Parameter));
	op_Parameter_init(parameter, value_begin, value_end);
	if (!self->last_positioned_parameter) {
		self->first_positioned_parameter = parameter;
		self->last_positioned_parameter  = parameter;
	} else {
		self->last_positioned_parameter->next = parameter;
		self->last_positioned_parameter       = parameter;
	}
	return parameter;
}

internal op_Parameter*
op_Options_append_to_named_parameter(op_Options *self, op_NamedParameter *named_parameter, char *value_begin, char *value_end)
{
	// fprintf(stderr,"append parameter to named parameter %.*s\n", (s32) (value_end - value_begin), value_begin);
	op_Parameter *parameter = (op_Parameter*) LinearAllocator_alloc(&self->allocator, sizeof(op_Parameter));
	op_Parameter_init(parameter, value_begin, value_end);
	if (!named_parameter->last_parameter) {
		named_parameter->first_parameter = parameter;
		named_parameter->last_parameter  = parameter;
	} else {
		named_parameter->last_parameter->next = parameter;
		named_parameter->last_parameter = parameter;
	}
	return parameter;
}




#define	op_TOKEN_NAME               2
#define	op_TOKEN_EXPECT_NAMED_PARAM 3
#define	op_TOKEN_SEPARATOR          4
#define	op_TOKEN_TEXT               5
#define	op_TOKEN_STRING             6

#define op_init_charset(name) \
	nt_CharSet name; \
	nt_CharSet_init(& name, st_ ## name, cstr_end(st_ ## name) , 0)

#define op_init_not_charset(name) \
	nt_CharSet not_ ## name; \
	nt_CharSet_init(&not_ ## name, st_ ## name, cstr_end(st_ ## name) , 1)

#define op_init_any_charset(name) \
	nt_CharSet name; \
	nt_CharSet_init_any(& name)

#define op_transition_c0m(source, target, symbols) \
	nt_Tokenizer_add_transition(tokenizer, source, & symbols, target, MOVE_RIGHT, nt_ACTION_BEGIN_TOKEN, 0)

#define op_transition_00m(state, symbols) \
	nt_Tokenizer_add_transition(tokenizer, state, & symbols, state, MOVE_RIGHT, nt_ACTION_DO_NOTHING, 0)

#define op_transition_ce0(source, target, symbols, token) \
	nt_Tokenizer_add_transition(tokenizer, source, & symbols, target, DONT_MOVE, \
				    nt_ACTION_EMIT_TOKEN, op_TOKEN_ ## token)

#define op_transition_0em(state, symbols, token) \
	nt_Tokenizer_add_transition(tokenizer, state, & symbols, state, MOVE_RIGHT, \
				    nt_ACTION_EMIT_SINGLETON, op_TOKEN_ ## token)

#define op_transition_cem(source, target, symbols, token) \
	nt_Tokenizer_add_transition(tokenizer, source, & symbols, target, MOVE_RIGHT, \
				    nt_ACTION_EMIT_TOKEN, op_TOKEN_ ## token)

#define op_transition_cEm(source, target, symbols, token) \
	nt_Tokenizer_add_transition(tokenizer, source, & symbols, target, MOVE_RIGHT, \
				    nt_ACTION_EMIT_TOKEN_INC, op_TOKEN_ ## token)

/*
 * program -named1=x1,x2  pos1 pos2 -named2 -named3 -named4=a,b,c
 */

internal void
op_initialize_tokenizer(nt_Tokenizer *tokenizer)
{
	nt_Tokenizer_init(tokenizer);

	nt_Tokenizer_reset_text(tokenizer, 0, 0);
// 	nt_Tokenizer_insert_skip_token(tokenizer, op_TOKEN_SKIP);
	nt_Tokenizer_insert_skip_token(tokenizer, op_TOKEN_SEPARATOR);

	const b8 MOVE_RIGHT = 1;
	const b8 DONT_MOVE  = 0;

	static char st_minus[]       = "-";
	static char st_separator[]   = " \t\n\r";
	static char st_equal[]       = "=";
	static char st_comma[]       = ",";
	static char st_name[]        = "-_abcdefghijklmnopqrstuvxywzABCDEFGHIJKLMNOPQRSTUVXYWZ0123456789.";
	static char st_openbrace[]   = "{";
	static char st_closebrace[]  = "}";
// 	static char st_backslash[]   = "\\";
// 	static char st_singlequote[] = "'";

	op_init_charset(minus);
	op_init_charset(separator);
	op_init_charset(equal);
	op_init_charset(name);
	op_init_charset(comma);
	op_init_charset(openbrace);
	op_init_charset(closebrace);
// 	op_init_charset(singlequote);
	op_init_any_charset(any);

	nt_CharSet eof;
	nt_CharSet_init_eof(&eof);

	// three characters
	// <state>:    change or 0
	// <emit>:     emit or 0
	// <movement>: move or 0

	op_transition_0em(0,equal,EXPECT_NAMED_PARAM);
	op_transition_0em(0,comma,EXPECT_NAMED_PARAM);
	op_transition_c0m(0,1,minus);
	op_transition_c0m(0,2,separator);
	op_transition_c0m(0,4,openbrace);
	op_transition_c0m(0,3,any);

	op_transition_00m(1,name);
	op_transition_ce0(1,0,any,NAME);

	op_transition_00m(2,separator);
	op_transition_ce0(2,0,any,SEPARATOR);

	op_transition_ce0(3,0,separator,TEXT);
	op_transition_ce0(3,0,comma,TEXT);
	op_transition_ce0(3,0,eof,TEXT);
	op_transition_00m(3,any);

	op_transition_cEm(4,0,closebrace,STRING);
	op_transition_00m(4,any);


}


#define op_Parser_BUFFER_SIZE        10
#define op_Parser_LOOKAHEAD           3
#define op_Parser_STACK_CAPACITY   1024
#define op_Parser_ERROR_LOG_SIZE   4096

#define op_Parser_NEXT_RESULT_OPEN        1
#define op_Parser_NEXT_RESULT_CLOSE       2
#define op_Parser_NEXT_RESULT_OPEN_CLOSE  3
#define op_Parser_NEXT_RESULT_ERROR       4

typedef struct {

	nt_Tokenizer    tokenizer;
	nt_Token        buffer[op_Parser_BUFFER_SIZE];
	nt_Token        *tkbegin;
	nt_Token        *tkend;
	b8              eof:1;
	b8              error: 1;

	op_Options        *options;
	op_NamedParameter *named_parameter;
	b8                ready_for_named_parameter;

	/* A log with op_Parser_ERROR_LOG_SIZE */
	char            log_buffer[op_Parser_ERROR_LOG_SIZE];
	Print           log;

	// memory for the AST
	char            *buffer_begin;
	char            *buffer_end;

} op_Parser;


internal void
op_Parser_reset(op_Parser *self, char *text_begin, char *text_end)
{
	self->eof = 0;
	self->error = 0;
	self->tkbegin = self->buffer;
	self->tkend = self->buffer;
	self->named_parameter = 0;
	self->ready_for_named_parameter = 0;
	op_Options_reset(self->options);
	nt_Tokenizer_reset_text(&self->tokenizer, text_begin, text_end);
}

internal void
op_Parser_init(op_Parser *self, op_Options *options)
{
	op_initialize_tokenizer(&self->tokenizer);
	Print_init(&self->log,
		   self->log_buffer,
		   self->log_buffer + sizeof(self->log_buffer));
	self->options = options;
	op_Parser_reset(self, 0, 0);
}

internal void
op_Parser_fill_buffer(op_Parser *self)
{
	if (self->eof || self->error) return;
	if (self->tkbegin != self->buffer) {
		nt_Token *dst = self->buffer;
		nt_Token *src = self->tkbegin;
		while (src != self->tkend) {
			*dst++ = *src++;
		}
		self->tkbegin = self->buffer;
		self->tkend = dst;
	}
	nt_Token *end = self->buffer + op_Parser_LOOKAHEAD;
	while (self->tkend != end) {
		if (nt_Tokenizer_next(&self->tokenizer)) {
			*self->tkend++ = self->tokenizer.token;
		} else {
			if (self->tokenizer.next_result_detail == nt_TOKENIZER_NEXT_RESULT_INVALID_INPUT) {
				self->error = 1;
				break;
			} else {
				self->eof = 1;
				break;
			}
		}
	}
}

internal void
op_Parser_consume_tokens(op_Parser *self, u32 n)
{
	Assert(self->tkend - self->tkbegin >= n);
	self->tkbegin += n;
	if (self->tkend - self->tkbegin < op_Parser_LOOKAHEAD) {
		op_Parser_fill_buffer(self);
	}
}

internal b8
op_Parser_compare(op_Parser* self, int index, nt_TokenType type)
{
	return (self->tkbegin + index < self->tkend) && (self->tkbegin + index)->type == type;
}

internal b8
op_Parser_compare_next(op_Parser* self, nt_TokenType type)
{
	return op_Parser_compare(self, 0, type);
}

internal void
op_Parser_log_context(op_Parser *self)
{
	b8 no_token = self->tkbegin == self->tkend;
	char *pos = 0;
	if (no_token) {
		pos = self->tokenizer.it;
	} else {
		pos = self->tkbegin->begin;
	}

	char *begin = self->tokenizer.text_begin;
	char *end   = self->tokenizer.text_end;

	Assert(begin <= pos && pos <= end);

	char *context_begin;
	char *context_end;

	{
		char *it = pos;
		while (it != begin && *it != '\n') {
			--it;
		}
		if (*it == '\n') ++it;
		context_begin = it;
	}

	{
		char *it = pos;
		while (it != end && *it != '\n') {
			++it;
		}
		context_end = it;
	}

	Print *print = &self->log;

	Print_cstr(print,"[Context]");
	if (no_token) {
		Print_cstr(print, " No valid token available. line: ");
		Print_u64(print, self->tokenizer.line);
		Print_cstr(print, " column: ");
		Print_u64(print, self->tokenizer.column);
		Print_cstr(print, "\n");
	} else {
		Print_cstr(print, "\n");
	}
	Print_str(print, context_begin, context_end);
	Print_cstr(print,"\n");
	Print_cstr(print,"^");
	Print_align(print,pos - context_begin + 1, 1, ' ');
	Print_cstr(print,"\n");
}

internal nt_Token*
op_Parser_token(op_Parser *self, s64 index)
{
	Assert(index < self->tkend - self->tkbegin);
	return self->tkbegin + index;
}

internal b8 op_Parser_run(op_Parser *self, char *text_begin, char *text_end)
{
	op_Parser_reset(self, text_begin, text_end);
	op_Parser_fill_buffer(self);
	for (;;) {
		if (op_Parser_compare_next(self, op_TOKEN_TEXT)) {
			nt_Token *token = op_Parser_token(self, 0);
			if (self->named_parameter == 0) {
				op_Options_append_parameter(self->options, token->begin, token->end);
			} else {
				if (self->ready_for_named_parameter) {
					op_Options_append_to_named_parameter(self->options, self->named_parameter, token->begin, token->end);
					self->ready_for_named_parameter = 0;
				} else {
					op_Options_append_parameter(self->options, token->begin, token->end);
					self->named_parameter = 0;
				}
			}
			op_Parser_consume_tokens(self, 1);
		} else if (op_Parser_compare_next(self, op_TOKEN_STRING)) {
			nt_Token *token = op_Parser_token(self, 0);
			if (self->named_parameter == 0) {
				op_Options_append_parameter(self->options, token->begin+1, token->end-1);
			} else if (self->named_parameter) {
				if (self->ready_for_named_parameter) {
					op_Options_append_to_named_parameter(self->options, self->named_parameter, token->begin+1, token->end-1);
					self->ready_for_named_parameter = 0;
				} else {
					op_Options_append_parameter(self->options, token->begin+1, token->end-1);
					self->named_parameter = 0;
				}
			}
			op_Parser_consume_tokens(self, 1);
		} else if (op_Parser_compare_next(self, op_TOKEN_NAME)) {
			nt_Token *token = op_Parser_token(self, 0);
			if (self->named_parameter) {
				if (self->ready_for_named_parameter) {
					return 0;
				} else {
					self->named_parameter = op_Options_insert_named_parameter(self->options, token->begin, token->end);
					self->ready_for_named_parameter = 0;
				}
			} else {
				self->named_parameter = op_Options_insert_named_parameter(self->options, token->begin, token->end);
				self->ready_for_named_parameter = 0;
			}
			op_Parser_consume_tokens(self, 1);
		} else if (op_Parser_compare_next(self, op_TOKEN_EXPECT_NAMED_PARAM)) {
			if (self->named_parameter == 0) {
				return 0;
			} else if (self->ready_for_named_parameter) {
				return 0;
			} else {
				self->ready_for_named_parameter = 1;
			}
			op_Parser_consume_tokens(self, 1);
		} else if (op_Parser_compare_next(self, nt_TOKEN_EOF)) {
			return 1;
		} else {
			return 0;
		}
	}
}

