/*
 *
 * ntp_ is time parser moduel
 *
 * depends on
 *
 *      tm_ time module
 *      nt_ tokenizer module
 */

#ifdef time_parser_UNIT_TEST
#include "platform.c"
#include "tokenizer.c"
#include "time.c"
#endif

#define	ntp_TOKEN_NUMBER         2
#define	ntp_TOKEN_SPACE          3
#define	ntp_TOKEN_T              4
#define	ntp_TOKEN_PLUS           5
#define	ntp_TOKEN_COLON          6
#define	ntp_TOKEN_MINUS          7
#define	ntp_TOKEN_SLASH          8
#define	ntp_TOKEN_Z              9
#define	ntp_TOKEN_AM            10
#define	ntp_TOKEN_PM            11
#define	ntp_TOKEN_AT            12
#define	ntp_TOKEN_PERIOD        13

#define ntp_LOOKAHEAD           10
#define ntp_ERROR_LOG_SIZE    1024
#define ntp_NUMBERS_CAPACITY     8

typedef struct {
	nt_Tokenizer    tokenizer;

	/* buffer multiple tokens */
	nt_Token        buffer[ntp_LOOKAHEAD];
	nt_Token        *tkbegin;
	nt_Token        *tkend;
	b8              eof:1;
	b8              tkerror: 1;

	b8  zone_has_negative_offset;
	s32 numbers[ntp_LOOKAHEAD];
	s32 num_date_numbers;
	s32 num_time_numbers;
	s32 num_zone_numbers;
	s32 num_numbers;
	b8  am_flag;
	b8  pm_flag;

	tm_Label label;
	tm_Time  time;

	/* A log with ntp_Parser_ERROR_LOG_SIZE */
	char            log_buffer[ntp_ERROR_LOG_SIZE];
	Print           log;

} ntp_Parser;


#define ntp_init_charset(name) \
	nt_CharSet_init(& name, st_ ## name, cstr_end(st_ ## name) , 0)

static void
ntp_initialize_tokenizer(nt_Tokenizer *tokenizer)
{
	nt_Tokenizer_init(tokenizer);

	nt_Tokenizer_reset_text(tokenizer, 0, 0);

	nt_Tokenizer_insert_skip_token(tokenizer, ntp_TOKEN_SPACE);

	const b8 MOVE_RIGHT = 1;
	const b8 DONT_MOVE  = 0;

	static char st_digits[]      = "0123456789";
	static char st_plus[]        = "+";
	static char st_minus[]       = "-";
	static char st_space[]       = " \t";
	static char st_colon[]       = ":";
	static char st_ts[]          = "tT";
	static char st_zs[]          = "zZ";
	static char st_slash[]       = "/";
	static char st_as[]          = "aA";
	static char st_ms[]          = "mM";
	static char st_ps[]          = "pP";
	static char st_at[]          = "@";
	static char st_period[]      = ".";

	nt_CharSet digits, plus, minus,
		   space, colon, ts, zs, slash,
		   any, as, ms, ps, at, period;

	ntp_init_charset(digits);
	ntp_init_charset(plus);
	ntp_init_charset(minus);
	ntp_init_charset(space);
	ntp_init_charset(colon);
	ntp_init_charset(ts);
	ntp_init_charset(zs);
	ntp_init_charset(slash);
	ntp_init_charset(as);
	ntp_init_charset(ms);
	ntp_init_charset(ps);
	ntp_init_charset(at);
	ntp_init_charset(period);

	/* charset that matches anything */
	nt_CharSet_init_any(&any);

	nt_Tokenizer_add_transition(tokenizer, 0, &digits , 1, MOVE_RIGHT, nt_ACTION_BEGIN_TOKEN,    0);
	nt_Tokenizer_add_transition(tokenizer, 0, &colon  , 0, MOVE_RIGHT, nt_ACTION_EMIT_SINGLETON, ntp_TOKEN_COLON);
	nt_Tokenizer_add_transition(tokenizer, 0, &plus   , 0, MOVE_RIGHT, nt_ACTION_EMIT_SINGLETON, ntp_TOKEN_PLUS);
	nt_Tokenizer_add_transition(tokenizer, 0, &minus  , 0, MOVE_RIGHT, nt_ACTION_EMIT_SINGLETON, ntp_TOKEN_MINUS);
	nt_Tokenizer_add_transition(tokenizer, 0, &space  , 2, MOVE_RIGHT, nt_ACTION_EMIT_SINGLETON, ntp_TOKEN_SPACE);
	nt_Tokenizer_add_transition(tokenizer, 0, &zs     , 0, MOVE_RIGHT, nt_ACTION_EMIT_SINGLETON, ntp_TOKEN_Z);
	nt_Tokenizer_add_transition(tokenizer, 0, &ts     , 0, MOVE_RIGHT, nt_ACTION_EMIT_SINGLETON, ntp_TOKEN_T);
	nt_Tokenizer_add_transition(tokenizer, 0, &at     , 0, MOVE_RIGHT, nt_ACTION_EMIT_SINGLETON, ntp_TOKEN_AT);
	nt_Tokenizer_add_transition(tokenizer, 0, &slash  , 0, MOVE_RIGHT, nt_ACTION_EMIT_SINGLETON, ntp_TOKEN_SLASH);
	nt_Tokenizer_add_transition(tokenizer, 0, &period , 0, MOVE_RIGHT, nt_ACTION_EMIT_SINGLETON, ntp_TOKEN_PERIOD);
	nt_Tokenizer_add_transition(tokenizer, 0, &as     , 3, MOVE_RIGHT, nt_ACTION_BEGIN_TOKEN,    0);
	nt_Tokenizer_add_transition(tokenizer, 0, &ps     , 4, MOVE_RIGHT, nt_ACTION_BEGIN_TOKEN,    0);

	nt_Tokenizer_add_transition(tokenizer, 1, &digits , 1, MOVE_RIGHT, nt_ACTION_DO_NOTHING,     0);
	nt_Tokenizer_add_transition(tokenizer, 1, &any    , 0, DONT_MOVE , nt_ACTION_EMIT_TOKEN,     ntp_TOKEN_NUMBER);

	nt_Tokenizer_add_transition(tokenizer, 2, &space  , 2, MOVE_RIGHT, nt_ACTION_DO_NOTHING,     0);
	nt_Tokenizer_add_transition(tokenizer, 2, &any    , 0, DONT_MOVE , nt_ACTION_EMIT_TOKEN,     ntp_TOKEN_SPACE);

	nt_Tokenizer_add_transition(tokenizer, 3, &ms     , 0, MOVE_RIGHT, nt_ACTION_EMIT_TOKEN,     ntp_TOKEN_AM);

	nt_Tokenizer_add_transition(tokenizer, 4, &ms     , 0, MOVE_RIGHT, nt_ACTION_EMIT_TOKEN,     ntp_TOKEN_PM);
}

static void
ntp_Parser_fill_buffer(ntp_Parser *self)
{
	if (self->eof || self->tkerror) return;
	if (self->tkbegin != self->buffer) {
		nt_Token *dst = self->buffer;
		nt_Token *src = self->tkbegin;
		while (src != self->tkend) {
			*dst++ = *src++;
		}
		self->tkbegin = self->buffer;
		self->tkend = dst;
	}
	nt_Token *end = self->buffer + ntp_LOOKAHEAD;
	while (self->tkend != end)
	{
		if (nt_Tokenizer_next(&self->tokenizer)) {
			*self->tkend++ = self->tokenizer.token;
		}
		else {
			if (self->tokenizer.next_result_detail == nt_TOKENIZER_NEXT_RESULT_INVALID_INPUT) {
				self->tkerror = 1;
				break;
			} else {
				self->eof = 1;
				break;
			}
		}
	}
}

static void
ntp_Parser_log_context(ntp_Parser *self)
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

	print_cstr(print,"[Context]");
	if (no_token) {
		print_cstr(print, " No valid token available. line: ");
		print_u64(print, self->tokenizer.line);
		print_cstr(print, " column: ");
		print_u64(print, self->tokenizer.column);
		print_cstr(print, "\n");
	} else {
		print_cstr(print, "\n");
	}
	print_str(print, context_begin, context_end);
	print_cstr(print,"\n");
	print_cstr(print,"^");
	print_align(print,pos - context_begin + 1, 1, ' ');
	print_cstr(print,"\n");
}

static void
ntp_Parser_reset(ntp_Parser *self, char *text_begin, char *text_end)
{
	self->num_numbers = 0;
	self->num_date_numbers = 0;
	self->num_time_numbers = 0;
	self->num_zone_numbers = 0;
	self->zone_has_negative_offset = 0;
	self->am_flag = 0;
	self->pm_flag = 0;

	self->tkbegin   = self->buffer;
	self->tkend     = self->buffer;
	self->eof       = 0;
	self->tkerror   = 0;

	nt_Tokenizer_reset_text(&self->tokenizer, text_begin, text_end);

	print_clear(&self->log);
}

static void
ntp_Parser_init(ntp_Parser* self)
{
	ntp_initialize_tokenizer(&self->tokenizer);
	print_init(&self->log, self->log_buffer, sizeof(self->log_buffer));
	ntp_Parser_reset(self,0,0);
}


static void
ntp_Parser_consume_tokens(ntp_Parser *self, u32 n)
{
	Assert(self->tkend - self->tkbegin >= n);
	self->tkbegin += n;
	if (self->tkend - self->tkbegin < ntp_LOOKAHEAD) {
		ntp_Parser_fill_buffer(self);
	}
}

static b8
ntp_Parser_compare(ntp_Parser* self, int index, nt_TokenType type)
{
	return (self->tkbegin + index < self->tkend) && (self->tkbegin + index)->type == type;
}

static b8
ntp_Parser_compare_next(ntp_Parser* self, nt_TokenType type)
{
	return ntp_Parser_compare(self, 0, type);
}

static void
ntp_Parser_push_number(ntp_Parser *self, s64 value)
{
	Assert(self->num_numbers < ntp_NUMBERS_CAPACITY);
	self->numbers[self->num_numbers] = value;
	++self->num_numbers;
}

static b8
ntp_Parser_consume_number(ntp_Parser *self)
{
	s64 value = 0;
	b8 ok = pt_parse_s64(self->tkbegin->begin, self->tkbegin->end, &value);
	if (!ok) {
		print_cstr(&self->log, "[parser error] Could not parse number\n");
		ntp_Parser_log_context(self);
		return 0;
	}
	ntp_Parser_push_number(self, value);
	ntp_Parser_consume_tokens(self, 1);
	return 1;
}

#define ntp_n(name) \
	ntp_Parser_compare_next(self, ntp_TOKEN_ ## name)


static b8
ntp_Parser_ZH(ntp_Parser *self)
{
	if (ntp_n(NUMBER)) {
		if (!ntp_Parser_consume_number(self)) {
			return 0;
		}
		++self->num_zone_numbers;
		if (ntp_n(COLON)) {
			if (self->num_zone_numbers == 2) {
				print_cstr(&self->log, "[ntp_Parser_ZH] zone offset with more than 2 numbers.\n");
				ntp_Parser_log_context(self);
				return 0;
			}
			ntp_Parser_consume_tokens(self,1);
			return ntp_Parser_ZH(self);
		} else if (ntp_Parser_compare_next(self,nt_TOKEN_EOF)) {
			return 1;
		} else {
			print_cstr(&self->log, "[ntp_Parser_ZH] error unexpected token.\n");
			ntp_Parser_log_context(self);
			return 0;
		}
	} else {
		print_cstr(&self->log, "[ntp_Parser_ZH] unexpected token while ntp_Parse_H.\n");
		ntp_Parser_log_context(self);
		return 0;
	}
}

static b8
ntp_Parser_Z(ntp_Parser *self)
{
	if (ntp_n(Z)) {
		ntp_Parser_push_number(self, 0);
		self->num_zone_numbers = 1;
		ntp_Parser_consume_tokens(self,1);
		if (ntp_Parser_compare_next(self,nt_TOKEN_EOF)) {
			return 1;
		} else {
			print_cstr(&self->log, "[parser error] unexpected token while ntp_Parse_Z.\n");
			ntp_Parser_log_context(self);
			return 0;
		}
	} else if (ntp_n(PLUS)) {
		ntp_Parser_consume_tokens(self,1);
		self->zone_has_negative_offset = 0;
		return ntp_Parser_ZH(self);
	} else if (ntp_n(MINUS)) {
		ntp_Parser_consume_tokens(self,1);
		self->zone_has_negative_offset = 1;
		return ntp_Parser_ZH(self);
	} else if (ntp_Parser_compare_next(self,nt_TOKEN_EOF)) {
		/* no timezone specified */
		return 1;
	} else {
		print_cstr(&self->log, "[parser error] unexpected token while ntp_Parse_H.\n");
		ntp_Parser_log_context(self);
		return 0;
	}
}

static b8
ntp_Parser_H(ntp_Parser *self)
{
	if (ntp_n(NUMBER)) {
		if (!ntp_Parser_consume_number(self)) {
			return 0;
		}
		++self->num_time_numbers;
		if (ntp_n(COLON)) {
			if (self->num_time_numbers == 3) {
				print_cstr(&self->log, "[parser error] hour with more than 3 numbers?.\n");
				ntp_Parser_log_context(self);
				return 0;
			}
			ntp_Parser_consume_tokens(self,1);
			return ntp_Parser_H(self);
		} else if (ntp_n(PM)) {
			self->pm_flag = 1;
			ntp_Parser_consume_tokens(self,1);
			return ntp_Parser_Z(self);
		} else if (ntp_n(AM)) {
			self->am_flag = 1;
			ntp_Parser_consume_tokens(self,1);
			return ntp_Parser_Z(self);
		} else if (ntp_n(MINUS) || ntp_n(PLUS) || ntp_n(Z)) {
			return ntp_Parser_Z(self);
		} else if (ntp_n(PERIOD)) {
			if (self->num_time_numbers != 3) {
				print_cstr(&self->log, "[parser error] fractional seconds need to come after seconds.\n");
				ntp_Parser_log_context(self);
				return 0;
			}
			ntp_Parser_consume_tokens(self,1);
			if (ntp_n(NUMBER)) {
				// discarding fractional seconds
				ntp_Parser_consume_tokens(self,1);
				return ntp_Parser_Z(self);
			} else {
				print_cstr(&self->log, "[parser error] fractional seconds needs to be a number.\n");
				ntp_Parser_log_context(self);
				return 0;
			}
		} else if (ntp_Parser_compare_next(self,nt_TOKEN_EOF)) {
			return 1;
		} else {
			print_cstr(&self->log, "[parser error] unexpected token while ntp_Parse_H.\n");
			ntp_Parser_log_context(self);
			return 0;
		}
	} else {
		print_cstr(&self->log, "[parser error] unexpected token while ntp_Parse_H.\n");
		ntp_Parser_log_context(self);
		return 0;
	}
}

static b8
ntp_Parser_D(ntp_Parser *self)
{
	/* needs to start with a number */
	if (ntp_n(NUMBER)) {
		if (!ntp_Parser_consume_number(self)) {
			return 0;
		}
		++self->num_date_numbers;
		if (ntp_n(SLASH) || ntp_n(MINUS)) {
			if (self->num_date_numbers == 3) {
				print_cstr(&self->log, "[parser error] date with more than 3 numbers?.\n");
				ntp_Parser_log_context(self);
				return 0;
			}
			ntp_Parser_consume_tokens(self,1);
			return ntp_Parser_D(self);
		} else if (ntp_n(T) || ntp_n(AT)) {
			ntp_Parser_consume_tokens(self,1);
			return ntp_Parser_H(self);
		} else if (ntp_n(NUMBER)) {
			return ntp_Parser_H(self);
		} else if (ntp_n(MINUS) || ntp_n(PLUS) || ntp_n(MINUS)) {
			return ntp_Parser_Z(self);
		} else if (ntp_Parser_compare_next(self,nt_TOKEN_EOF)) {
			return 1;
		} else {
			print_cstr(&self->log, "[parser error] D: expects a number.\n");
			ntp_Parser_log_context(self);
			return 0;
		}
	} else {
		print_cstr(&self->log, "[parser error] D: expects a number.\n");
		ntp_Parser_log_context(self);
		return 0;
	}
}


//
// grammar
//
// D = num ('/'|'-') D
//         ('T'|'@') H
//         num H         # note that num was not consumed here
//         ('+'|'-') Z
//




static b8
ntp_Parser_run(ntp_Parser *self, char *text_begin, char *text_end)
{
	ntp_Parser_reset(self, text_begin, text_end);

	ntp_Parser_fill_buffer(self);
	b8 result = ntp_Parser_D(self);
	if (!result)
		return 0;

	Assert(self->num_date_numbers > 0);
	s32 i= 0;

	self->label.year  = self->numbers[i++];
	self->label.month = (self->num_date_numbers > 1) ? self->numbers[i++] : 1;
	self->label.day   = (self->num_date_numbers > 2) ? self->numbers[i++] : 1;

	if (self->label.day >= 1000 && self->label.year <= 12) {
		/* day contains year, month contains day and day contains year */
		pt_swap_s32(&self->label.year, &self->label.day);
		pt_swap_s32(&self->label.day,  &self->label.month);
	}

	self->label.hour   = (self->num_time_numbers > 0) ? self->numbers[i++] : 0;
	self->label.minute = (self->num_time_numbers > 1) ? self->numbers[i++] : 0;
	self->label.second = (self->num_time_numbers > 2) ? self->numbers[i++] : 0;

	if (self->am_flag && self->pm_flag) {
		return 0;
	} else if (self->am_flag && self->label.hour > 12) {
		return 0; /* 13 am doesn't make any sense */
	} else if (self->pm_flag && self->label.hour > 12) {
		return 0;
	}

	if (self->pm_flag && self->label.hour < 12) {
		self->label.hour += 12;
	} else if (self->am_flag && self->label.hour == 12) {
		self->label.hour = 0;
	}

	self->label.offset_minutes  = (self->num_zone_numbers > 0) ? self->numbers[i++] * tm_MINUTES_PER_HOUR : 0;
	self->label.offset_minutes += (self->num_zone_numbers > 1) ? self->numbers[i++] : 0;

	if (self->label.offset_minutes && self->zone_has_negative_offset)
		self->label.offset_minutes = -self->label.offset_minutes;

	tm_Time_init_from_label(&self->time, &self->label);

	return 1;
}


#ifdef time_parser_UNIT_TEST

#include <stdio.h>

int
main()
{
	ntp_Parser parser;
	ntp_Parser_init(&parser);

	char *test1 = "2017-06-01 00:00:00.0";
	b8 ok = ntp_Parser_run(&parser, test1, cstr_end(test1));
	if (!ok) {
		printf("error:\n");
		write(stdout, parser.log.begin, parser.log.end-parser.log.begin);
	} else {
		printf("tm_Time: %lu\n", parser.time.time);
		char buffer[1024];
		Print print;
		Print_init(&print, buffer, buffer + sizeof(buffer));
		tm_Label_print(&parser.label, &print);
		Print_char(&print,0);
		printf("tm_Label: %s", print.begin);
		printf("\n");
	}
}

#endif


