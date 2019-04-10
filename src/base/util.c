/* parse storage size (no unit is bytes) 1 1b 1B 1k 2K 1m 4M 1g 1G */
#define ut_TOKEN_PLAIN     1
#define ut_TOKEN_BYTES     2
#define ut_TOKEN_KILOBYTES 3
#define ut_TOKEN_MEGABYTES 4
#define ut_TOKEN_GIGABYTES 5
static s64
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

static void
ut_PrintStack_init(ut_PrintStack *self, Print *print, char *level_text, char *line_feed)
{
	self->num_items = 0;
	self->print = print;
	self->level_text = level_text;
	self->line_feed = line_feed;
}

static inline void
ut_PrintStack_margin(ut_PrintStack *self)
{
	for (s32 i=0;i<self->num_items;++i) {
		print_cstr(self->print, self->level_text);
	}
}

static void
ut_PrintStack_print(ut_PrintStack *self, char *text)
{
	if (self->print->begin < self->print->end)
		print_cstr(self->print, self->line_feed);
	ut_PrintStack_margin(self);
	print_cstr(self->print, text);
}

static void
ut_PrintStack_append_cstr(ut_PrintStack *self, char *text)
{
	print_cstr(self->print, text);
}

static void
ut_PrintStack_append_str(ut_PrintStack *self, char *begin, char *end)
{
	print_str(self->print, begin, end);
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

static void
ut_PrintStack_append_escaped_json_str(ut_PrintStack *self, char *begin, char *end)
{
	Print *print = self->print;
	for (char *it=begin;it!=end;++it) {
		if ((*it >= 'a' && *it <= 'z') || (*it >= 'A' && *it <= 'Z') || (*it >= '0' && *it <= '9')) {
			print_char(print, *it);
		} else if (*it == '\\') {
			print_cstr(print,"\\\\");
		} else if (*it == 0) {
			print_cstr(print,"\\0");
		} else if (*it == '"') {
			print_cstr(print,"\\\"");
		} else if (*it == '\r') {
			print_cstr(print,"\\r");
		} else if (*it == '\t') {
			print_cstr(print,"\\t");
		} else if (*it == '\n') {
			print_cstr(print,"\\n");
		} else if (*it == '\v') {
			print_cstr(print,"\\v");
		} else if (*it == '\b') {
			print_cstr(print,"\\b");
		} else {
			print_char(print, *it);
		}
	}
}

static void
ut_PrintStack_align(ut_PrintStack *self, s32 len, s32 alignment, char sep)
{
	print_align(self->print, len, alignment, sep);
}

static void
ut_PrintStack_push(ut_PrintStack *self, char *open_text, char *close_text)
{
	Assert(self->num_items < ut_PrintStack_CAPACITY);
	if (open_text)
		ut_PrintStack_print(self, open_text);
	self->stack[self->num_items] = close_text;
	++self->num_items;
}

static inline void
ut_PrintStack_pop(ut_PrintStack *self)
{
	Assert(self->num_items > 0);
	--self->num_items;
	if (self->stack[self->num_items]) {
		ut_PrintStack_print(self, "");
		print_cstr(self->print, self->stack[self->num_items]);
	}
}

#define ut_PrintStack_push_formatted(self, close_text, open_text_format, ...) \
	ut_PrintStack_print((self), ""); \
	print_format((self)->print, open_text_format, __VA_ARGS__); \
	(self)->stack[(self)->num_items] = close_text; \
	++(self)->num_items;

#define ut_PrintStack_print_formatted(self, text_format, ...) \
	ut_PrintStack_print((self), ""); \
	print_format((self)->print, text_format, __VA_ARGS__);

#define ut_PrintStack_append_formatted(self, text_format, ...) \
	print_format((self)->print, text_format, __VA_ARGS__);





