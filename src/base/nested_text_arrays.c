//
// nested text arrays (nta_)
//
// A file format for describing nested text arrays where each array
// entry might also be tagged with an identifier.
//
// @todo(llins): utf-8
// @todo(llins): carriage return is not being an accepted character
//
// ASCII table
//
// Char                           Dec  Char     Dec  Char     Dec  Char
// ---------                           ---------     ---------     ----------
// 0   NUL (null)                      32  SPACE     64  @         96  `
// 1   SOH (start of heading)          33  !         65  A         97  a
// 2   STX (start of text)             34  "         66  B         98  b
// 3   ETX (end of text)               35  #         67  C         99  c
// 4   EOT (end of transmission)       36  $         68  D        100  d
// 5   ENQ (enquiry)                   37  %         69  E        101  e
// 6   ACK (acknowledge)               38  &         70  F        102  f
// 7   BEL (bell)                      39  '         71  G        103  g
// 8   BS  (backspace)                 40  (         72  H        104  h
// 9   TAB (horizontal tab)            41  )         73  I        105  i
// 10  LF  (NL line feed, new line)    42  *         74  J        106  j
// 11  VT  (vertical tab)              43  +         75  K        107  k
// 12  FF  (NP form feed, new page)    44  ,         76  L        108  l
// 13  CR  (carriage return)           45  -         77  M        109  m
// 14  SO  (shift out)                 46  .         78  N        110  n
// 15  SI  (shift in)                  47  /         79  O        111  o
// 16  DLE (data link escape)          48  0         80  P        112  p
// 17  DC1 (device control 1)          49  1         81  Q        113  q
// 18  DC2 (device control 2)          50  2         82  R        114  r
// 19  DC3 (device control 3)          51  3         83  S        115  s
// 20  DC4 (device control 4)          52  4         84  T        116  t
// 21  NAK (negative acknowledge)      53  5         85  U        117  u
// 22  SYN (synchronous idle)          54  6         86  V        118  v
// 23  ETB (end of trans. block)       55  7         87  W        119  w
// 24  CAN (cancel)                    56  8         88  X        120  x
// 25  EM  (end of medium)             57  9         89  Y        121  y
// 26  SUB (substitute)                58  :         90  Z        122  z
// 27  ESC (escape)                    59  ;         91  [        123  {
// 28  FS  (file separator)            60  <         92  \        124  |
// 29  GS  (group separator)           61  =         93  ]        125  }
// 30  RS  (record separator)          62  >         94  ^        126  ~
// 31  US  (unit separator)            63  ?         95  _        127  DEL

#ifdef nested_text_arrays_UNIT_TEST

#include "platform.c"

// platform util assumes a platform global variable
// is available
global_variable PlatformAPI platform;

#include "arena.c"
#include "cstr.c"
#include "print.c"

#endif

#define nta_TYPE_ARRAY 1
#define nta_TYPE_TEXT 2
#define nta_TYPE_UNDEFINED 2

typedef struct {
	s32 offset;
} nta_PtrElement;

typedef struct {
	s32 offset;
	s32 length;
} nta_Block;

//
// limit to 2G file
//
// could run a first pass reserving space for big the arrays are,
// and later make a pass inserting the actual contents
//
//
// typedef struct {
// 	s32      type;
// 	nta_Block tag;
// 	union {
// 		struct {
// 			s32 size;
// 			nta_PtrElement begin;
// 		} array;
// 		nta_Block scalar;
// 	};
// } nta_Element;


#define nta_TOKEN_INVALID 0
#define nta_TOKEN_COMMENT_LINE 1
#define nta_TOKEN_TAG 2
#define nta_TOKEN_TEXT 3
#define nta_TOKEN_BEGIN_ARRAY 4
#define nta_TOKEN_END_ARRAY 5
#define nta_TOKEN_END_OF_FILE 6
#define nta_TOKEN_MULTILINE_TEXT 7

#define nta_CHAR_NEW_LINE '\n'
#define nta_CHAR_TAB      '\t'
#define nta_CHAR_SPACE    ' '
#define nta_CHAR_PERIOD   '.'
#define nta_CHAR_UNDERSCORE '_'

#define nta_FIRST_VISIBLE_ASCII 33
#define nta_LAST_VISIBLE_ASCII 126

typedef struct {
	s32 type;
	nta_Block block;
	s32 line;
	s32 column;
} nta_Token;

static inline s32
nta_char_is_visible_ascii(char ch)
{
	return ch >= nta_FIRST_VISIBLE_ASCII && ch <= nta_LAST_VISIBLE_ASCII;
}

static inline s32
nta_char_is_skip(char ch)
{
	return ch == nta_CHAR_SPACE || ch == nta_CHAR_TAB;
}

static inline s32
nta_char_is_tag_char(char ch)
{
	return (ch == nta_CHAR_UNDERSCORE || ch == nta_CHAR_PERIOD || (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9'));
}

static inline s32
nta_is_new_line(char ch)
{
	return ch == nta_CHAR_NEW_LINE;
}

#define nta_Tokenizer_STATUS_OK 0
#define nta_Tokenizer_STATUS_ERROR_COMMENT 1
#define nta_Tokenizer_STATUS_ERROR_ARRAY_BEGIN 2
#define nta_Tokenizer_STATUS_ERROR_ARRAY_END 3
#define nta_Tokenizer_STATUS_ERROR_TEXT 4
#define nta_Tokenizer_STATUS_ERROR_BEGIN_MULTILINE_TEXT 5
#define nta_Tokenizer_STATUS_ERROR_END_MULTILINE_TEXT 6
#define nta_Tokenizer_STATUS_ERROR_MULTILINE_TEXT 7
#define nta_Tokenizer_STATUS_ERROR_UNTERMINATED_MUTILINE_TEXT 8

typedef struct {
	char     *begin;
	char     *end;
	char     *it;

	s32      line;
	s32      column;

	nta_Token token;  // token of the last call to next
	s32      status;
} nta_Tokenizer;

static void
nta_Tokenizer_init(nta_Tokenizer *self, char *begin, char *end)
{
	self->begin = begin;
	self->end = end;
	self->it = begin;
	self->line = 1;
	self->column = 1;
	self->token = (nta_Token) { .type = nta_TOKEN_INVALID, .block = { .offset = 0, .length = 0 }};
	self->status = nta_Tokenizer_STATUS_OK;
}

static inline void
nta_Tokenizer_start_token(nta_Tokenizer *self, s32 type)
{
	self->token.type = type;
	self->token.block.offset = (s32) (self->it - self->begin);
	self->token.block.length = 0;
	self->token.line = self->line;
	self->token.column = self->column;
}

static inline void
nta_Tokenizer_end_token(nta_Tokenizer *self)
{
	self->token.block.length = ((s32) (self->it - self->begin)) - self->token.block.offset;
}

static inline void
nta_Tokenizer_skip(nta_Tokenizer *self)
{
	while (self->it != self->end && nta_char_is_skip(*self->it)) {
		++self->column;
		++self->it;
	}
}

static inline void
nta_Tokenizer_advance(nta_Tokenizer *self, s32 num_bytes)
{
	Assert(self->it + num_bytes <= self->end);
	while (num_bytes > 0) {
		if (nta_is_new_line(*self->it)) {
			++self->line;
			self->column = 1;
		} else {
			++self->column;
		}
		++self->it;
		--num_bytes;
	}
}

static inline s32
nta_Tokenizer_eof(nta_Tokenizer *self)
{
	return self->it == self->end;
}

static inline s32
nta_Tokenizer_goto_eol(nta_Tokenizer *self)
{
	while (!nta_Tokenizer_eof(self)  && (nta_char_is_visible_ascii(*self->it) || nta_char_is_skip(*self->it))) {
		++self->column;
		++self->it;
	}
	if (nta_Tokenizer_eof(self) || nta_is_new_line(*self->it)) {
		return 1;
	} else {
		return 0;
	}
}

static inline void
nta_Tokenizer_goto_end_of_tag(nta_Tokenizer *self)
{
	while (!nta_Tokenizer_eof(self) && (nta_char_is_tag_char(*self->it))) {
		++self->column;
		++self->it;
	}
}

static inline s32
nta_Tokenizer_compare_current_pattern(nta_Tokenizer *self, char *target)
{
	char *it_src = self->it;
	char *it_tgt= target;
	while (it_src != self->end && *it_tgt != 0 && *it_src == *it_tgt) {
		++it_src;
		++it_tgt;
	}
	return *it_tgt == 0;
}

static s32
nta_Tokenizer_text(nta_Tokenizer *self)
{
	// skip until first visible ascii and then go forward
	// until new line or oef
	nta_Tokenizer_skip(self);
	nta_Tokenizer_start_token(self, nta_TOKEN_TEXT);
	s32 ok = nta_Tokenizer_goto_eol(self);
	if (!ok) {
		// write parsing error message
		self->status = nta_Tokenizer_STATUS_ERROR_TEXT;
		return 0;
	}
	nta_Tokenizer_end_token(self);
	return 1;
}

//
// @todo track line and column to be able to write
// meaningful error messages
//
static s32
nta_Tokenizer_next(nta_Tokenizer *self, s32 can_be_tag_or_comment)
{
	// skip == space or tab
	for (;;) {
		nta_Tokenizer_skip(self);
		if (nta_is_new_line(*self->it)) {
			self->column = 1;
			++self->line;
			++self->it;
		} else {
			break;
		}
	}

	if (self->it == self->end) {
		nta_Tokenizer_start_token(self, nta_TOKEN_END_OF_FILE);
		nta_Tokenizer_end_token(self);
	} else if (can_be_tag_or_comment && *self->it == '#') {
		// search for next new line
		nta_Tokenizer_start_token(self, nta_TOKEN_COMMENT_LINE);
		nta_Tokenizer_advance(self, 1);
		s32 ok = nta_Tokenizer_goto_eol(self);
		if (!ok) {
			self->status = nta_Tokenizer_STATUS_ERROR_COMMENT;
			// write parsing error message
			return 0;
		}
	} else if (can_be_tag_or_comment && *self->it == '.') {
		// search for next new line
		nta_Tokenizer_start_token(self, nta_TOKEN_TAG);
		nta_Tokenizer_advance(self, 1);
		nta_Tokenizer_goto_end_of_tag(self);
		nta_Tokenizer_end_token(self);
	} else if (*self->it == '{') {
		// search for next new line
		nta_Tokenizer_start_token(self, nta_TOKEN_BEGIN_ARRAY);
		nta_Tokenizer_advance(self, 1);
		nta_Tokenizer_skip(self);
		// make sure we are at either eof or eol
		if (!(nta_Tokenizer_eof(self) || nta_is_new_line(*self->it))) {
			// this is not how we open an array
			self->status = nta_Tokenizer_STATUS_ERROR_ARRAY_BEGIN;
			return 0;
		}
		nta_Tokenizer_end_token(self);
	} else if (*self->it == '}') {
		// search for next new line
		nta_Tokenizer_start_token(self, nta_TOKEN_END_ARRAY);
		nta_Tokenizer_advance(self, 1);
		nta_Tokenizer_skip(self);
		// make sure we are at either eof or eol
		if (!(nta_Tokenizer_eof(self) || nta_is_new_line(*self->it))) {
			// this is not how to close an array
			self->status = nta_Tokenizer_STATUS_ERROR_ARRAY_END;
			return 0;
		}
		nta_Tokenizer_end_token(self);
	} else if (nta_Tokenizer_compare_current_pattern(self, "'''")) {
		nta_Tokenizer_advance(self, 3);
		nta_Tokenizer_skip(self);
		// make sure we are at either eof or eol
		if (!(nta_Tokenizer_eof(self) || nta_is_new_line(*self->it))) {
			// this is not how to close an array
			self->status = nta_Tokenizer_STATUS_ERROR_BEGIN_MULTILINE_TEXT;
			return 0;
		}
		nta_Tokenizer_advance(self, 1);
		nta_Tokenizer_start_token(self, nta_TOKEN_MULTILINE_TEXT);
		char *multiline_text_begin = self->it;
		char *multiline_text_end   = self->it;
		s32 done = 0;
		while (!nta_Tokenizer_eof(self)) {
			nta_Tokenizer_skip(self);
			if (nta_Tokenizer_compare_current_pattern(self, "'''")) {
				// the end of line was triggered
				nta_Tokenizer_advance(self, 3);
				nta_Tokenizer_skip(self);
				// make sure we are at either eof or eol
				if (!(nta_Tokenizer_eof(self) || nta_is_new_line(*self->it))) {
					// this is not how to close an array
					self->status = nta_Tokenizer_STATUS_ERROR_END_MULTILINE_TEXT;
					return 0;
				}
				nta_Tokenizer_advance(self, 1);

				// length will be what it will be
				//
				done = 1;
				self->token.block.length = multiline_text_end - multiline_text_begin;
				break;
			} else {
				s32 ok = nta_Tokenizer_goto_eol(self);
				if (!ok) {
					self->status = nta_Tokenizer_STATUS_ERROR_MULTILINE_TEXT;
					// write parsing error message
					return 0;
				}
				multiline_text_end = self->it;
				nta_Tokenizer_advance(self, 1);
			}
		}
		if (!done) {
			self->status = nta_Tokenizer_STATUS_ERROR_UNTERMINATED_MUTILINE_TEXT;
			return 0;
		}
	} else {
		// assumes it is text
		s32 ok = nta_Tokenizer_text(self);
		if (!ok) {
			return 0;
		}
	}
	return 1;
}

//
// A simple to parse format of nested arrays of tagged text values
//
// NL == new line
// SP == space
// HT == horizontal tab
// SKIP == *(SP | HT)
// TAG == *(LETTER | UNDERSCORE | PERIOD | DIGIT)
//
// document :: HASH comment_line
//             '.' tagged_value
//             value
//
// tagged_value :: TAG value
//
// value :: '{' NL nested_value
//
// nested_value :: '{' NL nested_value
//
// In memory we will keep a total copy verbatim of the configuration
// and ranges of where the values are located.
//

//
// one pass element, not the final format but format where parser
// pushes the data
//
typedef struct nta_ParseElement nta_ParseElement;
struct nta_ParseElement {
	s32      type;
	nta_Block tag;
	union {
		nta_ParseElement *array_begin;
		nta_Block text;
	};
	nta_ParseElement *next;
};


#define nta_PARSE_STATUS_OK 0
#define nta_PARSE_STATUS_TOKEN_ERROR 1
#define nta_PARSE_STATUS_GRAMMAR_ERROR 2

typedef struct {
	a_Arena       *arena;
	nta_Tokenizer tokenizer;
	nta_Token     tokens_buffer[2];
	s32           tokens_available;
	struct {
		nta_ParseElement* *begin;
		nta_ParseElement* *end;
		nta_ParseElement* *capacity;
	} stack;
	s32          last_parse_status;
	Print        *error_log;
} nta_Parser;

//
// assuming all intermediate stuff will be stored in the given arena
//
static nta_Parser*
nta_new_parser(a_Arena *arena, s32 max_depth)
{
	nta_Parser *parser = a_push(arena, sizeof(nta_Parser), 8, 0);
	nta_Tokenizer_init(&parser->tokenizer, 0, 0);
	parser->stack.begin = a_push(arena, sizeof(nta_ParseElement*) * max_depth, 8, 0);
	parser->stack.end = parser->stack.begin;
	parser->stack.capacity = parser->stack.begin + max_depth;
	parser->arena = arena;
	return parser;
}

static nta_ParseElement*
nta_Parser_new_array_element(nta_Parser *self)
{
	nta_ParseElement *arr = a_push(self->arena, sizeof(nta_ParseElement), 8, 0);
	arr->type = nta_TYPE_ARRAY;
	arr->tag = (nta_Block) { .offset = 0, .length = 0 }; // emtpy tag
	arr->array_begin = 0;
	arr->next = 0;
	return arr;
}

static nta_ParseElement*
nta_Parser_new_undefined_element(nta_Parser *self)
{
	nta_ParseElement *arr = a_push(self->arena, sizeof(nta_ParseElement), 8, 0);
	arr->type = nta_TYPE_UNDEFINED;
	arr->tag = (nta_Block) { .offset = 0, .length = 0 }; // emtpy tag
	arr->array_begin= 0;
	arr->next = 0;
	return arr;
}

static nta_ParseElement*
nta_Parser_stack_top(nta_Parser *self)
{
	Assert(self->stack.begin != self->stack.end);
	return *(self->stack.end - 1);
}

static void
nta_Parser_push(nta_Parser *self, nta_ParseElement *element)
{
	Assert(self->stack.end != self->stack.capacity);
	*self->stack.end = element;
	++self->stack.end;
}

static void
nta_Parser_pop(nta_Parser *self)
{
	Assert(self->stack.end != self->stack.capacity);
	--self->stack.end;
	*self->stack.end = 0;
}

static s32
nta_Parser_produce_token(nta_Parser *self, s32 can_be_comment_or_tag)
{
	Assert(self->tokens_available < 2);
	s32 ok = nta_Tokenizer_next(&self->tokenizer, can_be_comment_or_tag);
	if (!ok) {
		return 0;
	}
	self->tokens_buffer[self->tokens_available] = self->tokenizer.token;
	++self->tokens_available;
	return 1;
}

static void
nta_Parser_consume_token(nta_Parser *self)
{
	Assert(self->tokens_available > 0);
	if (self->tokens_available > 1) {
		self->tokens_buffer[0] = self->tokens_buffer[1];
	}
	--self->tokens_available;
}

static s32
nta_Parser_next_token(nta_Parser *self, s32 can_be_comment_or_tag, nta_Token *result)
{
	Assert(result);
	if (self->tokens_available == 0) {
		s32 ok = nta_Parser_produce_token(self, can_be_comment_or_tag);
		if (!ok) {
			return 0;
		}
	}
	Assert(self->tokens_available > 0);
	*result = self->tokens_buffer[0];
	return 1;
}

//
// A simple to parse format of nested arrays of tagged ASCII text values
//
// NL == new line
// SP == space
// HT == horizontal tab
// SKIP == *(SP | HT)
// TAG == *(LETTER | UNDERSCORE | PERIOD | DIGIT)
//
// entry :: HASH comment_line
//             '.' tagged_value
//             value
//
// tagged_value :: TAG value
//
// value :: '{' NL nested_value
//
// nested_value :: '{' NL nested_value
//
// In memory we will keep a total copy verbatim of the configuration
// and ranges of where the values are located.
//

static void
nta_print_location_and_context(Print *print, char *text_begin, char *text_end, char *pos, s32 line, s32 column)
{
	Assert(text_begin != text_end);
	s32 error_at_eof = (pos == text_end);

	// try finding the line before eof
	if (error_at_eof) {
		--pos;
	}

	print_format(print, "line %d column %d\n", line, column);
	// try finding previous new line
	char *prev = pos;
	while (text_begin != prev) {
		if (nta_is_new_line(*prev)) {
			++prev;
			break;
		} else {
			--prev;
		}
	}

	// try finding next new line
	char *next= pos + 1;
	while (next != text_end) {
		if (nta_is_new_line(*next)) {
			break;
		} else {
			++next;
		}
	}

	print_str(print, prev, next);
	print_char(print, '\n');
	print_char(print, '^');
	print_align(print, column, 1, '.');
	print_char(print, '\n');
}

static void
nta_Parser_tokenizer_error(nta_Parser *self)
{
	Print *print = self->error_log;
	self->last_parse_status = nta_PARSE_STATUS_TOKEN_ERROR;
	Assert(self->tokenizer.status != nta_Tokenizer_STATUS_OK);
	if (print) {
		print_cstr(print, "Parsing Error on Tokenizer while parsing ");
		switch(self->tokenizer.status) {
		case nta_Tokenizer_STATUS_ERROR_COMMENT: {
			print_cstr(print, "a comment\n");
			print_cstr(print, "(1) could be a non-ASCII character between '#' and new line\n");
		} break;
		case nta_Tokenizer_STATUS_ERROR_ARRAY_BEGIN: {
			print_cstr(print, "the beginning of an array\n");
			print_cstr(print, "(1) could be a non skip between '{' and new line\n");
		} break;
		case nta_Tokenizer_STATUS_ERROR_ARRAY_END: {
			print_cstr(print, "the end of an array\n");
			print_cstr(print, "(1) could be a non skip between '}' and new line\n");
		} break;
		case nta_Tokenizer_STATUS_ERROR_TEXT: {
			print_cstr(print, "a text\n");
			print_cstr(print, "(1) could be a non-ASCII character before new line\n");
		} break;
		case nta_Tokenizer_STATUS_ERROR_BEGIN_MULTILINE_TEXT: {
			print_cstr(print, "a multi-line text\n");
			print_cstr(print, "(1) after between the multi line begin text marker ''' and end of line there can only be skip characters\n");
		} break;
		case nta_Tokenizer_STATUS_ERROR_END_MULTILINE_TEXT: {
			print_cstr(print, "a multi-line text\n");
			print_cstr(print, "(1) after between the multi line end text marker ''' and end of line there can only be skip characters\n");
		} break;
		case nta_Tokenizer_STATUS_ERROR_MULTILINE_TEXT: {
			print_cstr(print, "a multi-line text\n");
			print_cstr(print, "(1) could be a non-ASCII character inside a multi-line text\n");
		} break;
		case nta_Tokenizer_STATUS_ERROR_UNTERMINATED_MUTILINE_TEXT: {
			print_cstr(print, "a multi-line text\n");
			print_cstr(print, "(1) multi-line was not closed. there should be a new line + skip + ''' + new line or eof\n");
		} break;
		default: {
		} break;
		}
		nta_print_location_and_context(print, self->tokenizer.begin, self->tokenizer.end, self->tokenizer.it,
					      self->tokenizer.line, self->tokenizer.column);
	}
}

static void
nta_Parser_grammar_error(nta_Parser *self, nta_Token token, char *message)
{
	self->last_parse_status = nta_PARSE_STATUS_GRAMMAR_ERROR;
	Print *print = self->error_log;
	if (print) {
		print_cstr(print, message);
		print_cstr(print, "\n");
		nta_print_location_and_context(print, self->tokenizer.begin, self->tokenizer.end,
					      self->tokenizer.begin + token.block.offset,
					      token.line, token.column);
	}
}



static s32
nta_Parser_grammar_array_entries(nta_Parser *self)
{
	nta_ParseElement *parent = nta_Parser_stack_top(self);
	nta_ParseElement *last = 0;

	Assert(parent->type == nta_TYPE_ARRAY);
	Assert(parent->array_begin == 0);

	nta_Token token;
	s32 ok;

	// read all entries in this level
	for (;;) {

		{
			ok = nta_Parser_next_token(self, 1, &token);
			if (!ok) {
				nta_Parser_tokenizer_error(self);
				return 0;
			}
		}

		if (token.type == nta_TOKEN_COMMENT_LINE) {
			nta_Parser_consume_token(self);
			// discard
		} else if (token.type == nta_TOKEN_TAG || token.type == nta_TOKEN_TEXT || token.type == nta_TOKEN_BEGIN_ARRAY) {


			// if it is not a comment line it
			nta_ParseElement *new_element = nta_Parser_new_undefined_element(self);
			if (token.type == nta_TOKEN_TAG) {
				nta_Parser_consume_token(self);

				// initialize an element
				new_element->tag = token.block;

				// next thing can only be TEXT or EOF or ARRAY
				s32 ok = nta_Parser_next_token(self, 0, &token);
				if (!ok) {
					nta_Parser_tokenizer_error(self);
					return 0;
				}
			}

			if (last == 0) {
				parent->array_begin = new_element;
			} else {
				last->next = new_element;
			}
			last = new_element;

			// append element as the tail of current parent
			if (token.type == nta_TOKEN_BEGIN_ARRAY) {
				nta_Parser_consume_token(self);

				new_element->type = nta_TYPE_ARRAY;
				nta_Parser_push(self, new_element);

				// parse other entries
				ok = nta_Parser_grammar_array_entries(self);
				if (!ok) {
					// precise error message was already generated
					nta_Parser_tokenizer_error(self);
					return 0;
				}

				ok = nta_Parser_next_token(self, 0, &token);
				if (!ok) {
					// @todo check tokenizer status and write up error msg
					nta_Parser_tokenizer_error(self);
					return 0;
				}

				if (token.type != nta_TOKEN_END_ARRAY) {
					nta_Parser_grammar_error(self, token, "Expected the closing of array using '}'");
					return 0;
				}
				nta_Parser_consume_token(self);
				nta_Parser_pop(self);

			} else if (token.type == nta_TOKEN_TEXT) {
				nta_Parser_consume_token(self);
				new_element->type = nta_TYPE_TEXT;
				new_element->text = token.block;
			} else if (token.type == nta_TOKEN_MULTILINE_TEXT) {
				nta_Parser_consume_token(self);
				new_element->type = nta_TYPE_TEXT;
				new_element->text = token.block;
			} else {
				nta_Parser_grammar_error(self, token, "Expected either text or array");
				return 0;
			}
		} else {
			break;
		}
	}
	return 1;
}

static s32
nta_Parser_parse(nta_Parser *self, char *text_begin, char *text_end, Print *error_log)
{
	self->error_log = error_log;

	// the root parse element is simply an untagged array
	nta_Tokenizer_init(&self->tokenizer, text_begin, text_end);

	// push new element
	nta_ParseElement *root = nta_Parser_new_array_element(self);
	nta_Parser_push(self, root);

	//
	// read array entries (recursively)
	//
	{
		s32 ok = nta_Parser_grammar_array_entries(self);
		if (!ok) {
			// @todo check tokenizer status and write up error msg
			return 0;
		}
	}

	// check if the next available token is an EOF
	{
		nta_Token token;
		s32 ok = nta_Parser_next_token(self, 1, &token);
		if (!ok) {
			nta_Parser_tokenizer_error(self);
			return 0;
		}
		if (token.type != nta_TOKEN_END_OF_FILE) {
			nta_Parser_grammar_error(self, token, "Expected end of file");
			return 0;
		} else {
			return 1;
		}
	}
}


// typedef struct nta_ParseElement nta_ParseElement;
// struct nta_ParseElement {
// 	s32      type;
// 	nta_Block tag;
// 	union {
// 		nta_ParseElement *array_begin;
// 		nta_Block text;
// 	};
// 	nta_ParseElement *next;
// };
//

static void
nta_ParseElement_print(nta_ParseElement *self, char *text, Print *print, s32 level)
{

#define nta_local_print_block(block) \
	print_str(print, text + block.offset, text + block.offset + block.length)

	if (self->type == nta_TYPE_ARRAY) {
		char *chkpt = print->end;
		print_cstr(print,"ARRAY [");
		nta_local_print_block(self->tag);
		print_cstr(print,"]");
		print_fake_last_print(print, chkpt);
		s64 len = (print->end - chkpt) + 4*level;
		print_align(print, len, 1, '.');
		print_char(print,'\n');
		nta_ParseElement *it = self->array_begin;
		while (it != 0) {
			nta_ParseElement_print(it, text, print, level + 1);
			it = it->next;
		}
	} else if (self->type == nta_TYPE_TEXT) {
		char *chkpt = print->end;
		print_cstr(print,"TEXT [");
		nta_local_print_block(self->tag);
		print_cstr(print,"] ");
		nta_local_print_block(self->text);
		print_fake_last_print(print, chkpt);
		s64 len = (print->end - chkpt) + 4*level;
		print_align(print, len, 1, '.');
		print_char(print,'\n');
	}

#undef nta_local_print_block

}

#ifdef nested_text_arrays_UNIT_TEST

#include "../platform_dependent/nix_platform.c"

// returns 0 if couldn't open the file
static char*
read_file_into_memory_arena_cstr(Arena *arena, char *filename, u64 *output_file_size)
{
	// read list of laccids we are interested, should be in memory
	FILE *fp = fopen(filename,"r");
	if (!fp) {
		return 0;
	}
	// read whole file into memory
	fseek(fp, 0L, SEEK_END);
	u64 file_size = ftell(fp);
	fseek(fp, 0L, SEEK_SET);
	char *c_str = Arena_push(arena, file_size + 1, 8, 0);
	u64 bytes_read = fread(c_str, file_size, 1, fp);
	fclose(fp);
	c_str[file_size] = 0;
	if (output_file_size) {
		*output_file_size = file_size;
	}
	return c_str;
}

s32
main(int argc, char *argv[])
{
	if (argc != 2) {

		fprintf(stdout, "Usage: %s TEXTDATA\n", argv[0]);
		exit(0);
	}

	nix_init_platform(&platform);

	// scratch arena
	Arena arena = { 0 };

	nta_Parser *parser = nta_new_parser(&arena, 128);

	u64   text_size = 0;
	char *text = read_file_into_memory_arena_cstr(&arena, argv[1], &text_size);
	if (text == 0) {
		fprintf(stdout, "File not found\n");
		exit(-1);
	}


	Print *error_log= print_new(&arena, Megabytes(1));
	s32 ok = nta_Parser_parse(parser, text, text + text_size, error_log);
	if (ok) {
		Print *print = print_new(&arena, Megabytes(1));
		nta_ParseElement_print(nta_Parser_stack_top(parser), text, print, 0);
		fputs(print->begin, stdout);
		fprintf(stdout, "Parsing SUCCEEDED\n");
	} else {
		fputs(error_log->begin, stdout);
		fprintf(stdout, "Parsing FAILED\n");
	}

	return 0;
}

#endif

