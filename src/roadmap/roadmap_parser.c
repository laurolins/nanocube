//
// CONSIDER(llins): if we consider finding all elements with a certain
// tag in multiple parallel threads, and assuming that
//
// 1. these elements with tag T cannot nest,
// 2. we cannot possible be in a string with the test <tag inside
//
// we can simply split a file into multiple segments, search for
// the first occurrence of "<tag" in each segment as the alignment
// anchor and start the standard .xml element parser we already
// have.
//

//------------------------------------------------------------------------------
// Token Specific to the Simple Language Parser
//------------------------------------------------------------------------------

#define	rp_TOKEN_SKIP             1
#define	rp_TOKEN_TAGOPEN          2
#define	rp_TOKEN_TAGCLOSE         3
#define	rp_TOKEN_TAGFINISH        4
#define	rp_TOKEN_EQUAL            5
#define rp_TOKEN_IDENTIFIER       6
#define rp_TOKEN_STRING           7
#define	rp_TOKEN_TAGFINISHOPEN    8


internal void
rp_print_token(Print *print,  nt_Token *token)
{
	switch(token->type) {
	case rp_TOKEN_SKIP: {
		Print_cstr(print,"SKIP");
	} break;
	case rp_TOKEN_TAGOPEN: {
		Print_cstr(print,"TAGOPEN");
	} break;
	case rp_TOKEN_TAGCLOSE: {
		Print_cstr(print,"TAGCLOSE");
	} break;
	case rp_TOKEN_TAGFINISH: {
		Print_cstr(print,"TAGFINISH");
	} break;
	case rp_TOKEN_TAGFINISHOPEN: {
		Print_cstr(print,"TAGFINISHOPEN");
	} break;
	case rp_TOKEN_EQUAL: {
		Print_cstr(print,"EQUAL");
	} break;
	case rp_TOKEN_IDENTIFIER: {
		Print_cstr(print,"INDENTIFIER(");
		Print_str(print, token->begin, token->end);
		Print_cstr(print,")");
	} break;
	case rp_TOKEN_STRING: {
		Print_cstr(print,"STRING(");
		Print_str(print, token->begin, token->end);
		Print_cstr(print,")");
	} break;
	default: break;
	}
}

#define rp_init_charset(name, symbols) \
	static char *st_ ## name = symbols; \
	nt_CharSet name; \
	nt_CharSet_init(& name, st_ ## name, cstr_end(st_ ## name) , 0)

#define rp_init_not_charset(name) \
	nt_CharSet not_ ## name; \
	nt_CharSet_init(&not_ ## name, st_ ## name, cstr_end(st_ ## name) , 1)

#define rp_init_any_charset(name) \
	nt_CharSet name; \
	nt_CharSet_init_any(& name)

#define rp_transition_c0m(source, target, symbols) \
	nt_Tokenizer_add_transition(tokenizer, source, & symbols, target, MOVE_RIGHT, nt_ACTION_BEGIN_TOKEN, 0)

#define rp_transition_00m(state, symbols) \
	nt_Tokenizer_add_transition(tokenizer, state, & symbols, state, MOVE_RIGHT, nt_ACTION_DO_NOTHING, 0)

#define rp_transition_ce0(source, target, symbols, token) \
	nt_Tokenizer_add_transition(tokenizer, source, & symbols, target, DONT_MOVE, \
				    nt_ACTION_EMIT_TOKEN, rp_TOKEN_ ## token)

#define rp_transition_0em(state, symbols, token) \
	nt_Tokenizer_add_transition(tokenizer, state, & symbols, state, MOVE_RIGHT, \
				    nt_ACTION_EMIT_SINGLETON, rp_TOKEN_ ## token)

#define rp_transition_cem(source, target, symbols, token) \
	nt_Tokenizer_add_transition(tokenizer, source, & symbols, target, MOVE_RIGHT, \
				    nt_ACTION_EMIT_TOKEN, rp_TOKEN_ ## token)

#define rp_transition_cEm(source, target, symbols, token) \
	nt_Tokenizer_add_transition(tokenizer, source, & symbols, target, MOVE_RIGHT, \
				    nt_ACTION_EMIT_TOKEN_INC, rp_TOKEN_ ## token)


//
// the pattern bellow occurs in west-virginia-latest osm file downloaded from
// geofabrik open street map datasets. It messes up the synchronization of the
// parsing procedure
//
//    <tag k="faa" v="W99\"/>
//
//



/* specific tokenizer for this parser */
internal void
rp_initialize_tokenizer(nt_Tokenizer *tokenizer, char* text_begin, char *text_end)
{
	nt_Tokenizer_init(tokenizer);

	nt_Tokenizer_reset_text(tokenizer, text_begin, text_end);
	nt_Tokenizer_insert_skip_token(tokenizer, rp_TOKEN_SKIP);
// 	nt_Tokenizer_insert_skip_token(tokenizer, rp_TOKEN_COMMENT);

	const b8 MOVE_RIGHT = 1;
	const b8 DONT_MOVE  = 0;

	rp_init_charset( backslash      , "\\"                                                    );
	rp_init_charset( digits         , "0123456789"                                            );
	rp_init_charset( doublequote    , "\""                                                    );
	rp_init_charset( equal          , "="                                                     );
	rp_init_charset( gt             , ">"                                                     );
	rp_init_charset( letters        , "abcdefghijklmnopqrstuvxywzABCDEFGHIJKLMNOPQRSTUVXYWZ"  );
	rp_init_charset( lt             , "<"                                                     );
	// rp_init_charset( newline        , "\n"                                                    );
	rp_init_charset( question       , "?"                                                     );
	rp_init_charset( singlequote    , "'"                                                     );
	rp_init_charset( skip           , " \t\n\r"                                               );
	rp_init_charset( slash          , "/"                                                     );
	rp_init_charset( underscore     , "_"                                                     );

// 	rp_init_not_charset(doublequote);
// 	rp_init_not_charset(singlequote);

	rp_init_any_charset(any);

	// three characters
	// <state>:    change or 0
	// <emit>:     emit or 0
	// <movement>: move or 0

	rp_transition_0em(0,equal,EQUAL);
	rp_transition_0em(0,gt,TAGCLOSE);
	rp_transition_c0m(0,6,lt);
	rp_transition_c0m(0,1,skip);
	rp_transition_c0m(0,4,doublequote);
	rp_transition_c0m(0,2,letters);
	rp_transition_c0m(0,3,slash);
	rp_transition_c0m(0,3,question);
	rp_transition_c0m(0,7,singlequote);

	rp_transition_00m(1,skip);
	rp_transition_ce0(1,0,any,SKIP);

	rp_transition_00m(2,letters);
	rp_transition_00m(2,digits);
	rp_transition_00m(2,underscore);
	rp_transition_ce0(2,0,any,IDENTIFIER);

	rp_transition_cEm(3,0,gt,TAGFINISH);

	/* can get in trouble with unicode characters inside strings */
	rp_transition_c0m(4,5,backslash);
	rp_transition_cEm(4,0,doublequote,STRING);
	rp_transition_00m(4,any);
	/* not a good idea */
	/* if a new line is found when reading a string, assume the string is done. */
	/* emit it, it might be crazy */
	// rp_transition_cEm(4,0,newline,STRING);
	rp_transition_c0m(5,4,any);

	rp_transition_cEm(6,0,slash,TAGFINISHOPEN);
	rp_transition_cEm(6,0,question,TAGOPEN);
	rp_transition_ce0(6,0,any,TAGOPEN);

	/* can get in trouble with unicode strings inside strings */
	rp_transition_c0m(7,8,backslash);
	rp_transition_cEm(7,0,singlequote,STRING);
	rp_transition_00m(7,any);
	// rp_transition_cEm(7,0,newline,STRING);

	rp_transition_c0m(8,7,any);

}




//
//  S   -> id = E; S
//       | $
//       | E; S
//
//  E   -> id ( P EC
//       | id EC
//       | num EC
//       | ( E ) EC
//       | - E
//
//  P   -> )
//       | E PS
//
//  PS  -> )
//       | , E PS
//
//  EC  -> - E
//       | + E
//       | / E
//       | * E
//       | . E
//       | \epsilon
//

typedef struct rp_KeyValue {
	MemoryBlock key;
	MemoryBlock value;
	struct rp_KeyValue *next;
} rp_KeyValue;

typedef struct rp_Element {
	MemoryBlock tag;

	// keys   on the even indices 0, 2, ...
	// values on the odd  indices 0, 2, ...
	rp_KeyValue *kv_first;
	rp_KeyValue *kv_last;

	/*
	 * position in the parser linear allocator
	 * where tag, keys and values of this element
	 * are safely stored
	 */
	LinearAllocatorCheckpoint memory_checkpoint;

} rp_Element;


#define rp_Parser_BUFFER_SIZE        10
#define rp_Parser_LOOKAHEAD           3
#define rp_Parser_STACK_CAPACITY   1024
#define rp_Parser_ERROR_LOG_SIZE   4096


#define rp_Parser_NEXT_RESULT_OPEN        1
#define rp_Parser_NEXT_RESULT_CLOSE       2
#define rp_Parser_NEXT_RESULT_OPEN_CLOSE  3
#define rp_Parser_NEXT_RESULT_ERROR       4

typedef struct {

	nt_Tokenizer    tokenizer;
	nt_Token        buffer[rp_Parser_BUFFER_SIZE];
	nt_Token        *tkbegin;
	nt_Token        *tkend;
	b8              eof:1;
	b8              error: 1;

	/* next call to next element should pop first */
	b8              pop: 1;
	b8              unused:5;
	u8              next_result;

	rp_Element      stack[rp_Parser_STACK_CAPACITY];
	u32             stack_size;

	/* A log with rp_Parser_ERROR_LOG_SIZE */
	char            log_buffer[rp_Parser_ERROR_LOG_SIZE];
	Print           log;

	// memory for the AST
	LinearAllocatorCheckpoint memory_checkpoint;
	LinearAllocator           *memory;

} rp_Parser;



/*
 * rp_Element
 */

internal void
rp_Element_init(rp_Element *self, char *tag_begin, char *tag_end)
{
	self->tag.begin = tag_begin;
	self->tag.end   = tag_end;
	self->kv_first  = 0;
	self->kv_last   = 0;

	/* not initialized */
	self->memory_checkpoint.checkpoint = 0;
}

internal void
rp_Element_insert_key_value(rp_Element *self, rp_KeyValue *kv)
{
	Assert(kv->next == 0);
	if (self->kv_last) {
		self->kv_last->next = kv;
		self->kv_last = kv;
	} else {
		self->kv_first = self->kv_last = kv;
	}
}

/*
 * rp_Parser
 */

internal void
rp_Parser_fill_buffer(rp_Parser *self)
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
	nt_Token *end = self->buffer + rp_Parser_LOOKAHEAD;
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
rp_Parser_reset(rp_Parser *self, char *text_begin, char *text_end)
{
	self->tkbegin = self->buffer;
	self->tkend   = self->buffer;
	self->eof     = 0;
	self->error   = 0;
	self->pop     = 0;
	self->unused  = 0;
	self->stack_size = 0;

	pt_fill((char*) self->stack,
		(char*) self->stack + sizeof(self->stack), 0);

	LinearAllocator_rewind(self->memory, self->memory_checkpoint);

	nt_Tokenizer_reset_text(&self->tokenizer, text_begin, text_end);

	Print_clear(&self->log);

	rp_Parser_fill_buffer(self);

}


internal void
rp_Parser_init(rp_Parser *self, LinearAllocator *memory)
{
	pt_fill((char*) self, (char*) self + sizeof(*self), 0);

	rp_initialize_tokenizer(&self->tokenizer, 0, 0);

	Print_init(&self->log,
		   self->log_buffer,
		   self->log_buffer + sizeof(self->log_buffer));

	self->memory = memory;
	self->memory_checkpoint = LinearAllocator_checkpoint(memory);

	rp_Parser_reset(self,0,0);
}


internal void
rp_Parser_consume_tokens(rp_Parser *self, u32 n)
{
	Assert(self->tkend - self->tkbegin >= n);
	self->tkbegin += n;
	if (self->tkend - self->tkbegin < rp_Parser_LOOKAHEAD) {
		rp_Parser_fill_buffer(self);
	}
}

internal b8
rp_Parser_compare(rp_Parser* self, int index, nt_TokenType type)
{
	return (self->tkbegin + index < self->tkend) && (self->tkbegin + index)->type == type;
}

internal b8
rp_Parser_compare_next(rp_Parser* self, nt_TokenType type)
{
	return rp_Parser_compare(self, 0, type);
}

internal void
rp_Parser_log_context(rp_Parser *self)
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

internal MemoryBlock
rp_Parser_make_copy(rp_Parser *self, char *begin, char *end)
{
	MemoryBlock result;
	u64  length = (u64) (end - begin);
	result.begin = LinearAllocator_alloc(self->memory, length);
	result.end   = result.begin + length;
	pt_copy_bytes(begin, end, result.begin, result.end);
	return result;
}

internal rp_Element*
rp_Parser_push_element(rp_Parser *self, char *tag_begin, char *tag_end)
{
	Assert(tag_begin <= tag_end);
	Assert(self->stack_size < rp_Parser_STACK_CAPACITY - 1);

	rp_Element *element = self->stack + self->stack_size;

	/* copy tag into memory */
	MemoryBlock tag_copy = rp_Parser_make_copy(self, tag_begin, tag_end);
	rp_Element_init(element, tag_copy.begin, tag_copy.end);

	element->memory_checkpoint = LinearAllocator_checkpoint(self->memory);

	++self->stack_size;

	return element;
}

internal rp_Element*
rp_Parser_top_element(rp_Parser *self)
{
	Assert(self->stack_size > 0);
	return self->stack + self->stack_size - 1;
}

internal void
rp_Parser_pop_element(rp_Parser *self)
{
	Assert(self->pop);
	Assert(self->stack_size > 0);
	self->pop = 0;
	--self->stack_size;

	if (self->stack_size == 0) {
		LinearAllocator_rewind(self->memory, self->memory_checkpoint);
	} else {
		rp_Element *top = rp_Parser_top_element(self);
		LinearAllocator_rewind(self->memory, top->memory_checkpoint);
	}

}

internal rp_KeyValue*
rp_Parser_insert_key_value(rp_Parser *self,
			   char *key_begin,   char *key_end,
			   char *value_begin, char *value_end)
{
	Assert(key_begin <= key_end && value_begin <= value_end);
	Assert(self->stack_size > 0);

	/* copy tag into memory */
	MemoryBlock key_copy   = rp_Parser_make_copy(self, key_begin,   key_end   );
	MemoryBlock value_copy = rp_Parser_make_copy(self, value_begin, value_end );

	rp_KeyValue *kv = (rp_KeyValue*)LinearAllocator_alloc(self->memory, sizeof(rp_KeyValue));
	kv->key   = key_copy;
	kv->value = value_copy;
	kv->next  = 0;

	rp_Element *element = rp_Parser_top_element(self);
	rp_Element_insert_key_value(element, kv);
	element->memory_checkpoint = LinearAllocator_checkpoint(self->memory);

	return kv;

}

internal nt_Token*
rp_Parser_token(rp_Parser *self, s64 index)
{
	Assert(index < self->tkend - self->tkbegin);
	return self->tkbegin + index;
}


// /* key values list */
// internal b8
// rp_Parser_C(rp_Parser *self)
// {
// 	if (rp_Parser_compare_next(self, 0, rp_TOKEN_TAGOPEN)
// 	    && rp_Parser_compare_next(self, 1, rp_TOKEN_IDENTIFIER)) {
// 	} else if (rp_Parser_compare_next(self, 0, rp_TOKEN_TAGFINISHOPEN)
// 		   && rp_Parser_compare_next(self, 1, rp_TOKEN_IDENTIFIER)
// 		   && rp_Parser_compare_next(self, 2, rp_TOKEN_TAGCLOSE)) {
// 	} else {
// 	}
// }


/* key values list */
internal b8
rp_Parser_E(rp_Parser *self)
{
	for (;;) {
		if (rp_Parser_compare(self, 0, rp_TOKEN_IDENTIFIER)
		    && rp_Parser_compare(self, 1, rp_TOKEN_EQUAL)
		    && rp_Parser_compare(self, 2, rp_TOKEN_STRING)) {

			/* ES again */
			nt_Token *key_token   = rp_Parser_token(self, 0);
			nt_Token *value_token = rp_Parser_token(self, 2);

			/* store key value without quotes */
			rp_Parser_insert_key_value(self,
						   key_token->begin, key_token->end,
						   value_token->begin+1, value_token->end-1);

			rp_Parser_consume_tokens(self, 3);
			continue;

		} else if (rp_Parser_compare_next(self, rp_TOKEN_TAGCLOSE)) {
			/* nest */
			rp_Parser_consume_tokens(self, 1);

			self->next_result = rp_Parser_NEXT_RESULT_OPEN;
			return 1;

		} else if (rp_Parser_compare_next(self, rp_TOKEN_TAGFINISH)) {
			rp_Parser_consume_tokens(self, 1);

			/* finish and pop */
			self->pop = 1;
			self->next_result = rp_Parser_NEXT_RESULT_OPEN_CLOSE;
			return 1;
		} else {
			self->next_result = rp_Parser_NEXT_RESULT_ERROR;
			self->error = 1;
			return 0;
		}
	}
}

internal b8
rp_Parser_next(rp_Parser *self)
{
	if (self->eof || self->error) {
		return 0;
	}

	for (;;) {

		if (self->pop) {
			rp_Parser_pop_element(self);
		}

		if (rp_Parser_compare_next(self, nt_TOKEN_EOF)) {
			self->eof = 1;
			self->next_result = rp_Parser_NEXT_RESULT_ERROR;
			return 0;
		} else if (rp_Parser_compare(self, 0, rp_TOKEN_TAGOPEN)
			 && rp_Parser_compare(self, 1, rp_TOKEN_IDENTIFIER)) {

			/* initialize memory */
			nt_Token *tag_token = rp_Parser_token(self, 1);
			rp_Parser_push_element(self, tag_token->begin, tag_token->end);

			/* consume first two tokens */
			rp_Parser_consume_tokens(self, 2);

			return rp_Parser_E(self);

		} else if (rp_Parser_compare(self, 0, rp_TOKEN_TAGFINISHOPEN)
			   && rp_Parser_compare(self, 1, rp_TOKEN_IDENTIFIER)
			   && rp_Parser_compare(self, 2, rp_TOKEN_TAGCLOSE)) {

			if (self->stack_size == 0) {
				self->error = 1;
				return 0;
			}

			rp_Element *element = rp_Parser_top_element(self);
			nt_Token   *ident   = rp_Parser_token(self, 1);

			if (pt_compare_memory(element->tag.begin, element->tag.end,
						ident->begin, ident->end) != 0) {
				self->error = 1;
				return 0;
			}

			/* consume first two tokens */
			rp_Parser_consume_tokens(self, 3);

			/* pop and continue loop! */
			self->pop = 1;
			self->next_result = rp_Parser_NEXT_RESULT_CLOSE;
			return 1;

		} else {
			self->error = 1;
			self->next_result = rp_Parser_NEXT_RESULT_ERROR;
			return 0;
		}
	}
}




















