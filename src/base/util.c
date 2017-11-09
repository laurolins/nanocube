/* parse storage size (no unit is bytes) 1 1b 1B 1k 2K 1m 4M 1g 1G */
#define ut_TOKEN_PLAIN     1
#define ut_TOKEN_BYTES     2
#define ut_TOKEN_KILOBYTES 3
#define ut_TOKEN_MEGABYTES 4
#define ut_TOKEN_GIGABYTES 5
internal s64
ut_parse_storage_size(char *begin, char *end)
{
	static char st_digits[]    = "0123456789";
	static char st_megabytes[] = "mM";
	static char st_kilobytes[] = "kK";
	static char st_bytes[]     = "bB";
	static char st_gigabytes[] = "gG";

	nt_CharSet digits, megabytes, kilobytes,
		   bytes, gigabytes, eof;

	nt_CharSet_init(&digits,    st_digits, cstr_end(st_digits) , 0);
	nt_CharSet_init(&bytes,     st_bytes, cstr_end(st_bytes) , 0);
	nt_CharSet_init(&kilobytes, st_kilobytes, cstr_end(st_kilobytes) , 0);
	nt_CharSet_init(&megabytes, st_megabytes, cstr_end(st_megabytes) , 0);
	nt_CharSet_init(&gigabytes, st_gigabytes, cstr_end(st_gigabytes) , 0);

	const b8 MOVE_RIGHT = 1;
	const b8 DONT_MOVE  = 0;

	nt_CharSet_init_eof(&eof);

	nt_Tokenizer tokenizer;
	nt_Tokenizer_init(&tokenizer);

	nt_Tokenizer_add_transition(&tokenizer, 0, &digits , 1, MOVE_RIGHT, nt_ACTION_BEGIN_TOKEN,    0);

	nt_Tokenizer_add_transition(&tokenizer, 1, &digits     , 1, MOVE_RIGHT, nt_ACTION_DO_NOTHING, 0);
	nt_Tokenizer_add_transition(&tokenizer, 1, &bytes      , 2, MOVE_RIGHT, nt_ACTION_DO_NOTHING, 0);
	nt_Tokenizer_add_transition(&tokenizer, 1, &kilobytes  , 3, MOVE_RIGHT, nt_ACTION_DO_NOTHING, 0);
	nt_Tokenizer_add_transition(&tokenizer, 1, &megabytes  , 4, MOVE_RIGHT, nt_ACTION_DO_NOTHING, 0);
	nt_Tokenizer_add_transition(&tokenizer, 1, &gigabytes  , 5, MOVE_RIGHT, nt_ACTION_DO_NOTHING, 0);
	nt_Tokenizer_add_transition(&tokenizer, 1, &eof        , 0, DONT_MOVE,  nt_ACTION_EMIT_TOKEN, ut_TOKEN_PLAIN);

	nt_Tokenizer_add_transition(&tokenizer, 2, &eof  , 0, DONT_MOVE, nt_ACTION_EMIT_TOKEN, ut_TOKEN_BYTES);

	nt_Tokenizer_add_transition(&tokenizer, 3, &eof  , 0, DONT_MOVE, nt_ACTION_EMIT_TOKEN, ut_TOKEN_KILOBYTES);

	nt_Tokenizer_add_transition(&tokenizer, 4, &eof  , 0, DONT_MOVE, nt_ACTION_EMIT_TOKEN, ut_TOKEN_MEGABYTES);

	nt_Tokenizer_add_transition(&tokenizer, 5, &eof  , 0, DONT_MOVE, nt_ACTION_EMIT_TOKEN, ut_TOKEN_GIGABYTES);

	s64 result = 0;

	nt_Tokenizer_reset_text(&tokenizer, begin, end);
	if (nt_Tokenizer_next(&tokenizer)) {
		nt_Token tok = tokenizer.token;
		s32 end_offset = -1;
		if (tok.type == ut_TOKEN_PLAIN)
			end_offset = 0;
		if (!pt_parse_s64(tok.begin, tok.end + end_offset, &result)) {
			return -1;
		}
		switch (tok.type) {
		case ut_TOKEN_PLAIN:
		case ut_TOKEN_BYTES: {
			return result;
		} break;
		case ut_TOKEN_KILOBYTES: {
			return Kilobytes(result);
		} break;
		case ut_TOKEN_MEGABYTES: {
			return Megabytes(result);
		} break;
		case ut_TOKEN_GIGABYTES: {
			return Gigabytes(result);
		} break;
		default: {
			return -1;
		}
		}
	} else {
		return -1;
	}
}


////////////////////////////////////////////////////////////////////////////////
// BasicAllocator
////////////////////////////////////////////////////////////////////////////////

typedef struct {
	pt_Memory *begin;
	pt_Memory *end;
	pt_Memory *capacity;
	pt_Memory buffer;
	u64 used_memory;
} BasicAllocator;

#define CLEAR(ptr) pt_filln((char*) ptr, sizeof(*ptr), '\0')
#define BasicAllocator_INITIAL_CAPACITY 16

internal void
BasicAllocator_init(BasicAllocator *self)
{
	CLEAR(self);
}

internal void
BasicAllocator_grow(BasicAllocator *self)
{
	if (self->begin == 0) {
		self->buffer   = platform.allocate_memory(sizeof(pt_Memory) * BasicAllocator_INITIAL_CAPACITY, 3, 0);
		self->begin    = (pt_Memory*) self->buffer.memblock.begin;
		self->end      = self->begin;
		self->capacity = self->begin + BasicAllocator_INITIAL_CAPACITY;
	} else {
		s64 new_capacity = 2 * (self->capacity - self->begin);
		s64 current_usage = self->end - self->begin;
		pt_Memory new_buffer = platform.allocate_memory(new_capacity * sizeof(pt_Memory), 3, 0);
		pt_Memory *new_begin = (pt_Memory*) new_buffer.memblock.begin;
		for (s32 i=0;i<current_usage;++i) {
			new_begin[i] = self->begin[i];
		}
		platform.free_memory(&self->buffer);
		self->buffer = new_buffer;
		self->begin  = new_begin;
		self->end    = new_begin + current_usage;
		self->capacity = new_begin + new_capacity;
	}
}

internal void*
BasicAllocator_alloc(BasicAllocator *self, u64 size, u8 alignment)
{
	pt_Memory result = platform.allocate_memory(size, alignment, 0);
	if (self->end == self->capacity) {
		BasicAllocator_grow(self);
	}
	*self->end = result;
	++self->end;
	self->used_memory += size; // we are missing the aligment bytes here
	return result.memblock.begin;
}

internal MemoryBlock
BasicAllocator_alloc_memblock(BasicAllocator *self, u64 size, u8 alignment)
{
	pt_Memory result = platform.allocate_memory(size, alignment, 0);
	if (self->end == self->capacity) {
		BasicAllocator_grow(self);
	}
	*self->end = result;
	++self->end;
	self->used_memory += size; // we are missing the aligment bytes here
	return result.memblock;
}

internal b8
BasicAllocator_free(BasicAllocator *self, void *ptr)
{
	s64 n = self->end - self->begin;
	for (s32 i=0;i<n;++i) {
		if ((void*) self->begin[i].memblock.begin == ptr) {

			self->used_memory -= MemoryBlock_length(&self->begin[i].memblock);

			platform.free_memory(self->begin + i);
			self->begin[i] = self->begin[n-1];
			--self->end;
			return 1;
		}
	}
	Assert(!"Didn't free any memory");
	return 0;
}

internal void
BasicAllocator_free_all(BasicAllocator *self)
{
	if (self->begin) {
		s64 n = self->end - self->begin;
		for (s32 i=0;i<n;++i) {
			platform.free_memory(self->begin + i);
		}
		platform.free_memory(&self->buffer);
		self->used_memory = 0;
	}
}


////////////////////////////////////////////////////////////////////////////////
// convert_uri_to_ascii
////////////////////////////////////////////////////////////////////////////////


// TODO(llins): Denormalize HTTP request-target in a more general way (%00-%FF <-> ascii)
// GET /taxi.b("pickup_location",dive(10)); HTTP/1.1
// method SP request-tar get SP HTTP-version CRLF

//
internal inline s32
ut_hex_digit(char ch)
{
// 	ascii code	48	0	(number zero)
// 	ascii code	65	A	(Capital A)
// 	ascii code	97	a	(Lowercase a)
	if (ch < 48 || ch >= 97 + 6) {
		return -1;
	} else if (ch < 48 + 10) {
		return ch - 48;
	} else if (ch < 65) {
		return -1;
	} else if (ch < 65 + 6) {
		return 10 + (ch - 65);
	} else if (ch < 97) {
		return -1;
	} else { // if (ch < 97 + 6) {
		return 10 + (ch - 97);
	}
}


internal MemoryBlock
ut_convert_uri_to_ascii(MemoryBlock uri)
{
	char *dst = uri.begin;
	char *src = uri.begin;
	MemoryBlock result;
	result.begin = dst;

	// no %DD possible output is the same as input
	if (uri.end - src < 3) {
		return uri;
	}
	char *max_src = uri.end - 3;

loop:
	if (src[0] == '%') {
		s32 h = ut_hex_digit(src[1]);
		if (h < 0)
			goto normal_advance;
		s32 l = ut_hex_digit(src[2]);
		if (l < 0)
			goto normal_advance;
		*dst = (char) (h * 16 + l);
		++dst;
		src += 3;
	} else {
normal_advance:
		*dst = *src;
		++dst;
		++src;
	}
	if (src <= max_src)
		goto loop;

	while (src <= uri.end) {
		*dst = *src;
		++dst;
		++src;
	}

	result.end = dst;
	return result;
}


internal MemoryBlock
ut_convert_uri_to_ascii_old(MemoryBlock uri)
{
	char *dst = uri.begin;
	char *src = uri.begin;

	MemoryBlock result;
	result.begin = dst;
	while (src != uri.end) {
		if (*src == '%' && (uri.end - src > 2)
		    && (*(src+1) == '2' && *(src+2) == '2')) {
			*dst = '"';
			src += 3;
		} else {
			*dst = *src;
			++src;
		}
		++dst;
	}
	result.end = dst;
	return result;
}




//
// ut_PrintStack
// Schedule print strings
//



#define ut_PrintStack_CAPACITY 10

typedef struct {
	char  *stack[ut_PrintStack_CAPACITY];
	s32   num_items;
	char  *level_text;
	char  *line_feed;
	Print *print;
} ut_PrintStack;

internal void
ut_PrintStack_init(ut_PrintStack *self, Print *print, char *level_text, char *line_feed)
{
	self->num_items = 0;
	self->print = print;
	self->level_text = level_text;
	self->line_feed = line_feed;
}

internal inline void
ut_PrintStack_margin(ut_PrintStack *self)
{
	for (s32 i=0;i<self->num_items;++i) {
		Print_cstr(self->print, self->level_text);
	}
}

internal void
ut_PrintStack_print(ut_PrintStack *self, char *text)
{
	if (self->print->begin < self->print->end)
		Print_cstr(self->print, self->line_feed);
	ut_PrintStack_margin(self);
	Print_cstr(self->print, text);
}

internal void
ut_PrintStack_append_cstr(ut_PrintStack *self, char *text)
{
	Print_cstr(self->print, text);
}

internal void
ut_PrintStack_append_str(ut_PrintStack *self, char *begin, char *end)
{
	Print_str(self->print, begin, end);
}

//
// JSON string
//
// Backspace is replaced with \b
// Form feed is replaced with \f
// Newline is replaced with \n
// Carriage return is replaced with \r
// Tab is replaced with \t
// Double quote is replaced with
// Backslash is replaced with
//

internal void
ut_PrintStack_append_escaped_json_str(ut_PrintStack *self, char *begin, char *end)
{
	Print *print = self->print;
	for (char *it=begin;it!=end;++it) {
		if ((*it >= 'a' && *it <= 'z') || (*it >= 'A' && *it <= 'Z') || (*it >= '0' && *it <= '9')) {
			Print_char(print, *it);
		} else if (*it == '\\') {
			Print_cstr(print,"\\\\");
		} else if (*it == 0) {
			Print_cstr(print,"\\0");
		} else if (*it == '"') {
			Print_cstr(print,"\\\"");
		} else if (*it == '\r') {
			Print_cstr(print,"\\r");
		} else if (*it == '\t') {
			Print_cstr(print,"\\t");
		} else if (*it == '\n') {
			Print_cstr(print,"\\n");
		} else if (*it == '\v') {
			Print_cstr(print,"\\v");
		} else if (*it == '\b') {
			Print_cstr(print,"\\b");
		} else {
			Print_char(print, *it);
		}
	}
}

internal void
ut_PrintStack_align(ut_PrintStack *self, s32 len, s32 alignment, char sep)
{
	Print_align(self->print, len, alignment, sep);
}

internal void
ut_PrintStack_push(ut_PrintStack *self, char *open_text, char *close_text)
{
	Assert(self->num_items < ut_PrintStack_CAPACITY);
	if (open_text)
		ut_PrintStack_print(self, open_text);
	self->stack[self->num_items] = close_text;
	++self->num_items;
}

internal inline void
ut_PrintStack_pop(ut_PrintStack *self)
{
	Assert(self->num_items > 0);
	--self->num_items;
	if (self->stack[self->num_items]) {
		ut_PrintStack_print(self, "");
		Print_cstr(self->print, self->stack[self->num_items]);
	}
}

#define ut_PrintStack_push_formatted(self, close_text, open_text_format, ...) \
	ut_PrintStack_print((self), ""); \
	Print_format((self)->print, open_text_format, __VA_ARGS__); \
	(self)->stack[(self)->num_items] = close_text; \
	++(self)->num_items;

#define ut_PrintStack_print_formatted(self, text_format, ...) \
	ut_PrintStack_print((self), ""); \
	Print_format((self)->print, text_format, __VA_ARGS__);

#define ut_PrintStack_append_formatted(self, text_format, ...) \
	Print_format((self)->print, text_format, __VA_ARGS__);





