//------------------------------------------------------------------------------
// Tokenization
//------------------------------------------------------------------------------

typedef s32 nt_TokenType;

#define nt_TOKEN_EOF 0

typedef enum {
	nt_ACTION_DO_NOTHING,
	nt_ACTION_BEGIN_TOKEN,
	nt_ACTION_EMIT_SINGLETON,
	nt_ACTION_EMIT_TOKEN,
	nt_ACTION_EMIT_TOKEN_INC
} nt_Action;

typedef struct nt_StateTransition nt_StateTransition;

typedef struct {
	u32                index:31;
	u32                valid:1;
	nt_StateTransition *begin_transition;
	nt_StateTransition *end_transition;
} nt_State;

typedef struct {
	char *begin;
	char *end;
	b8    contiguous:1;
	b8    any: 1;
	b8    not: 1;
	b8    eof: 1;
	b8    others: 4;
} nt_CharSet;

struct nt_StateTransition {
	nt_CharSet    symbols;     // what is on the tape (null means match nothing) int        move;        // 0 or 1
	b8            move;
	nt_Action     action;      // emit
	nt_TokenType  toktype;     // if action is emit token, this is the type
	nt_State*     next_state;  // emit
};

typedef struct {
	char         *begin;
	char         *end;
	nt_TokenType type;
} nt_Token;

typedef enum {
	nt_TOKENIZER_NEXT_RESULT_FOUND_TOKEN   = 0,
	nt_TOKENIZER_NEXT_RESULT_DONE          = 1,
	nt_TOKENIZER_NEXT_RESULT_INVALID_INPUT = 2
} nt_NextResult;


#define nt_TRANSITION_CAPACITY  256
#define nt_STATE_CAPACITY       64
#define nt_SKIP_TOKENS_CAPACITY 16

typedef struct {

	char                   *text_begin;
	char                   *text_end;
	char                   *it;

	nt_Token               token;

	nt_NextResult          next_result_detail;

	u32                    line;
	u32                    column;

	b8                     eof; // emit an EOF token when hit eof

	s32                    num_transitions;
	s32                    num_states;
	s32                    num_skip_tokens;

	nt_State               states[nt_STATE_CAPACITY];
	nt_StateTransition     transitions[nt_TRANSITION_CAPACITY];
	nt_TokenType           skip_tokens[nt_SKIP_TOKENS_CAPACITY];

	nt_State*              state;

} nt_Tokenizer;








/*
 * Implementation (maybe move to a .c)
 */

internal void
nt_CharSet_init_eof(nt_CharSet* self)
{
	self->any        = 0;
	self->eof        = 1;
	self->begin      = 0;
	self->end        = 0;
	self->contiguous = 0;
	self->not        = 0;
}

internal void
nt_CharSet_init_any(nt_CharSet* self)
{
	self->any        = 1;
	self->eof        = 1;
	self->begin      = 0;
	self->end        = 0;
	self->contiguous = 0;
	self->not        = 0;
}

internal void
nt_CharSet_init(nt_CharSet* self, char *begin, char *end, b8 not)
{
	Assert(begin != 0 && begin < end);
	self->any   = 0;
	self->eof   = 0;
	self->begin = begin;
	self->end   = end;
	self->not   = not;
	self->contiguous = 0;
	if (begin < end) {
		self->contiguous = 1;
		u8 *it = (u8*) begin + 1;
		while (it != (u8*) end) {
			if (*(it-1) + 1 != *it) {
				self->contiguous = 0;
				break;
			}
			++it;
		}
	}
}

internal inline b8
nt_CharSet_match(nt_CharSet *self, char ch)
{
	if (self->any && ch != 0) return 1;

	if (self->eof && ch == 0) return 1;

	if (ch == 0) return 0; // never match zero in a rule that is not any

	if (self->contiguous) {
		b8 cond = (u8) *self->begin <= (u8) ch && (u8) ch <= (u8) *(self->end-1);
		return self->not ? !cond : cond;
	} else {
		char *it = self->begin;
		while (it != self->end) {
			if (*it == ch)
				return self->not ? 0 : 1;
			++it;
		}
		return self->not ? 1 : 0;
	}
}

internal char*
nt_NextResult_cstr(nt_NextResult tnr)
{
	static char *arr[] =
	{
		"TOKENIZER_NEXT_RESULT_FOUND_TOKEN"   ,
		"TOKENIZER_NEXT_RESULT_DONE"          ,
		"TOKENIZER_NEXT_RESULT_INVALID_INPUT"
	};
	return arr[(s32)tnr];
}

internal void
nt_Tokenizer_add_transition(nt_Tokenizer* self, s32 state, nt_CharSet* symbols, s32 next_state,
			    b8 move, nt_Action action, nt_TokenType toktype)
{
	Assert(state < sizeof(self->states)/sizeof(nt_State));
	Assert(next_state < sizeof(self->states)/sizeof(nt_State));
	Assert(self->num_transitions < sizeof(self->transitions)/sizeof(nt_StateTransition));

	nt_State *src = self->states + state;
	nt_State *dst = self->states + next_state;

	nt_StateTransition *t = self->transitions + self->num_transitions;
	t->symbols            = *symbols;
	t->move               = move;
	t->action             = action;
	t->toktype            = toktype;
	t->next_state         = dst;
	++self->num_transitions;

	if (src->valid) {
		if (src->begin_transition != src->end_transition) {
			Assert(t == src->end_transition);
			++src->end_transition;
		} else {
			src->begin_transition = t;
			src->end_transition   = t + 1;
		}
	} else {
		++self->num_states;
		src->index            = state;
		src->valid            = 1;
		src->begin_transition = t;
		src->end_transition   = t + 1;
	}

	if (!dst->valid) {
		++self->num_states;
		dst->index            = next_state;
		dst->valid            = 1;
		dst->begin_transition = dst->end_transition = 0;
	}
}

internal MemoryBlock
nt_Tokenizer_token_memblock(nt_Tokenizer *self)
{
	MemoryBlock result;
	result.begin = self->token.begin;
	result.end   = self->token.end;
	return result;
}

internal void
nt_Tokenizer_reset_text(nt_Tokenizer *self, char *text_begin, char *text_end)
{
	self->eof        = 0;
	self->line       = 1; /* specifically tied to \n */
	self->column     = 1;

	self->token.begin = 0;
	self->token.end   = 0;
	self->next_result_detail = nt_TOKENIZER_NEXT_RESULT_FOUND_TOKEN;

	self->text_begin = text_begin;
	self->text_end   = text_end;
	self->it         = text_begin;

	self->state      = self->states;
}

internal void
nt_Tokenizer_init(nt_Tokenizer *self)
{
	/* an empty tokenizer */
	self->num_transitions = 0;
	self->num_states      = 0;
	self->num_skip_tokens = 0;
	nt_Tokenizer_reset_text(self, 0, 0);

	// clean up all states
	char *p = (char*) self->states;
	pt_fill((char*) p, p + sizeof(self->states), 0);
}

#define nt_TOKEN_CANONICAL_SEPARATOR 1
#define nt_TOKEN_CANONICAL_TEXT 2

internal void
nt_Tokenizer_insert_skip_token(nt_Tokenizer *self, nt_TokenType toktype)
{
	Assert(self->num_skip_tokens < nt_SKIP_TOKENS_CAPACITY);
	self->skip_tokens[self->num_skip_tokens] = toktype;
	++self->num_skip_tokens;
}


/* make sure that str_begin and str_end won't chage throughout tokenizer usage */
internal void
nt_Tokenizer_init_canonical(nt_Tokenizer *self, char *sep_begin, char *sep_end)
{
	nt_Tokenizer_init(self);

	nt_CharSet any, separators, not_separators;
	nt_CharSet_init(&separators, sep_begin, sep_end, 0);
	nt_CharSet_init(&not_separators, sep_begin, sep_end, 1);

	const b8 DONT_MOVE   = 0;
	const b8 MOVE_RIGHT  = 1;

	nt_CharSet_init_any(&any);
	nt_Tokenizer_add_transition(self, 0, &separators, 1, MOVE_RIGHT, nt_ACTION_BEGIN_TOKEN,0);
	nt_Tokenizer_add_transition(self, 0, &any,        2, MOVE_RIGHT, nt_ACTION_BEGIN_TOKEN,0);

	nt_Tokenizer_add_transition(self, 1, &separators, 1, MOVE_RIGHT, nt_ACTION_DO_NOTHING, 0);
	nt_Tokenizer_add_transition(self, 1, &any,        0, DONT_MOVE,  nt_ACTION_EMIT_TOKEN,     nt_TOKEN_CANONICAL_SEPARATOR);

	nt_Tokenizer_add_transition(self, 2, &not_separators, 2, MOVE_RIGHT, nt_ACTION_DO_NOTHING, 0);
	nt_Tokenizer_add_transition(self, 2, &any,            0, DONT_MOVE,  nt_ACTION_EMIT_TOKEN, nt_TOKEN_CANONICAL_TEXT);

	nt_Tokenizer_insert_skip_token(self, nt_TOKEN_CANONICAL_SEPARATOR);
	nt_Tokenizer_insert_skip_token(self, nt_TOKEN_EOF);
}

internal b8
nt_Tokenizer_is_skip_token(nt_Tokenizer *self, nt_TokenType toktype)
{
	for (s32 i=0;i<self->num_skip_tokens;++i)
		if (toktype == self->skip_tokens[i])
			return 1;
	return 0;
}

internal b8
nt_Tokenizer_next(nt_Tokenizer *self)
{
	Assert(self->num_states);
	b8 check_eof = 1;
	for (;;) {

		if (check_eof) {
			if (self->it == self->text_end) {
				if (self->eof) {
					self->next_result_detail = nt_TOKENIZER_NEXT_RESULT_DONE;
					return 0;
				} else {
					self->token.begin = self->it;
					self->token.end   = self->it;
					self->token.type  = nt_TOKEN_EOF;
					self->eof = 1;
					if (nt_Tokenizer_is_skip_token(self, nt_TOKEN_EOF)) {
						self->next_result_detail = nt_TOKENIZER_NEXT_RESULT_DONE;
						return 0;
					} else {
						self->next_result_detail = nt_TOKENIZER_NEXT_RESULT_FOUND_TOKEN;
						return 1;
					}
				}
			}
		}

		nt_State* state = self->state;

		char ch = (self->it != self->text_end) ? *self->it : 0; // 0 marks end of file

		nt_StateTransition *transition = state->begin_transition;
		while (transition != state->end_transition) {
			if (nt_CharSet_match(&transition->symbols, ch))
				break;
			++transition;
		}

		if (transition == state->end_transition) {
			self->next_result_detail = nt_TOKENIZER_NEXT_RESULT_INVALID_INPUT;
			return 0;
		}

		if (transition->action == nt_ACTION_BEGIN_TOKEN) {
			self->token.begin = self->it;
			self->token.end   = self->it;
		} else if (transition->action == nt_ACTION_EMIT_SINGLETON) {
			self->token.begin = self->it;
			self->token.end   = self->it+1;
			self->token.type  = transition->toktype;
		} else if (transition->action == nt_ACTION_EMIT_TOKEN_INC) {
			self->token.end   = self->it+1;
			self->token.type  = transition->toktype;
		} else if (transition->action == nt_ACTION_EMIT_TOKEN) {
			self->token.end   = self->it;
			self->token.type  = transition->toktype;
		}

		if (transition->move) {
			++self->it;
			if (ch == '\n') {
				++self->line;
				self->column = 1;
			} else {
				++self->column;
			}
		}

		self->state = transition->next_state;

		/*
		 * Special treatment of the skip token: don't emit it to the user.
		 * Treat as if a new next is being called.
		 */
		switch(transition->action) {
		case nt_ACTION_EMIT_TOKEN:
		case nt_ACTION_EMIT_SINGLETON:
		case nt_ACTION_EMIT_TOKEN_INC: {
			if (nt_Tokenizer_is_skip_token(self, transition->toktype)) {
				check_eof = 1;
			} else {
				self->next_result_detail = nt_TOKENIZER_NEXT_RESULT_FOUND_TOKEN;
				return 1;
			}
		} break;
		default: {
			check_eof = 0;
		} break;
		}
	}
}

