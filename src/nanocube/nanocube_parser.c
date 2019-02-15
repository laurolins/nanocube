/*
BEGIN_TODO
# 2017-03-11T04:46 llins

Move this code outside of nanocue specific stuff. Since we are
reusing this scripting language infra-structure in many different
contexts: NC from csv spec, NC query language, snap query language.

END_TODO
*/



//------------------------------------------------------------------------------
// Token Specific to the Simple Language Parser
//------------------------------------------------------------------------------

#define	np_TOKEN_SKIP               1
#define	np_TOKEN_INT                2
#define	np_TOKEN_FLOAT              3
#define	np_TOKEN_IDENTIFIER         4
#define	np_TOKEN_BINOP              5
#define	np_TOKEN_COMMA              6
#define	np_TOKEN_STRING             7
#define	np_TOKEN_SEMICOLON          8
#define	np_TOKEN_LPAREN             9
#define	np_TOKEN_RPAREN            10
#define	np_TOKEN_LBRACKET          11
#define	np_TOKEN_RBRACKET          12
#define	np_TOKEN_LBRACE            13
#define	np_TOKEN_RBRACE            14
#define	np_TOKEN_EQUAL             15
#define	np_TOKEN_EQUAL_EQUAL       16
#define	np_TOKEN_SLASH             17
#define	np_TOKEN_COMMENT           18
#define	np_TOKEN_HASH              19
#define	np_TOKEN_MULTILINE_STRING  20

//------------------------------------------------------------------------------
// AST
//------------------------------------------------------------------------------

typedef enum {
	np_AST_Node_Type_Number,
	np_AST_Node_Type_Function,
	np_AST_Node_Type_Binary_Operation,
	np_AST_Node_Type_Assignment,
	np_AST_Node_Type_Group, // parenthesis
	np_AST_Node_Type_Variable,
	np_AST_Node_Type_String
} np_AST_Node_Type;

// @todo add unary operator

typedef struct np_AST_Node {
	np_AST_Node_Type    type;
	struct np_AST_Node *next;  // next node in the same environment
	char            *begin;
	char            *end;   // input offset representing this node
	void            *detail;
} np_AST_Node;

typedef struct np_AST_Number {
	b8        is_integer;
	union {
		s64    ip; // integer precision
		f64 fp; // floating precision
	};
} np_AST_Number;

typedef struct {
	nt_Token name; // point to a safe region
} np_AST_Variable;

typedef struct {
	nt_Token str; // point to a safe region
} np_AST_String;

typedef struct np_AST_Function_Parameter
{
	np_AST_Node                      *node;
	struct np_AST_Function_Parameter *next;
}
np_AST_Function_Parameter;

typedef struct AST_Function
{
	nt_Token name; // point to a safe region
	int                     num_parameters;
	np_AST_Function_Parameter *first_parameter; // singly linked list
}
np_AST_Function;

typedef struct
{
	np_AST_Node *node;
}
np_AST_Group;

typedef struct
{
	nt_Token     name;
	np_AST_Node *left;
	np_AST_Node *right;
}
np_AST_Binary_Operation;

typedef struct
{
	nt_Token     name;
	np_AST_Node *node;
}
np_AST_Assignment;

//------------------------------------------------------------------------------
// Parser
//------------------------------------------------------------------------------

//
// simple program:
//
// theft = crimes . binding("kind","@2") . binding("time",interval(0,100));
//
// theft_percentage_50_crimes_plus = thefts/crimes * 100 * (crimes >= 100);
//
// theft_percentage_50_crimes_plus . binding("location",dive("@",8))
//

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

#define np_Parser_BUFFER_SIZE 10
#define np_Parser_LOOKAHEAD 2
#define np_Parser_STACK_CAPACITY 1024
#define np_Parser_ERROR_LOG_SIZE 4096

typedef np_AST_Node *np_AST_Node_Ptr;

typedef struct {

	nt_Tokenizer    *tokenizer;
	nt_Token        buffer[np_Parser_BUFFER_SIZE];
	nt_Token        *tkbegin;
	nt_Token        *tkend;
	b8              eof:1;
	b8              tkerror: 1;

	// TODO(llins): make the AST stack use the ast_memory,
	//              maybe the right side of the memory
	struct {
		//
		// end will be the linear allocator ast_memory->capacity
		// begin will be empty at first and should be always
		// greater than ast_memory->end
		//
		// once the parsing is done, the stack is useless and can be
		// overwritter
		//
		np_AST_Node_Ptr *begin;
		np_AST_Node_Ptr *end;
	} stack;

	np_AST_Node     *ast_first;
	np_AST_Node     *ast_last;

	/* A log with np_Parser_ERROR_LOG_SIZE */
	char            log_buffer[np_Parser_ERROR_LOG_SIZE];
	Print           log;

	// memory for the AST
	BilinearAllocator *ast_memory;

} np_Parser;

//------------------------------------------------------------------------------
// Type and TypeTable
//------------------------------------------------------------------------------

typedef u32 np_TypeID;

typedef struct {
	np_TypeID      id;
	MemoryBlock name;
} np_Type;

typedef struct {
	np_Type *begin;
	np_Type *end;
	np_Type *capacity;
} np_TypeTable;


//------------------------------------------------------------------------------
// Symbol and Symbol Table
//------------------------------------------------------------------------------

typedef u32 np_SymbolID;

typedef struct np_Compiler np_Compiler;

typedef struct {
	void         *value;
	np_TypeID     type_id;
	b8            readonly;
	b8            error;
} np_TypeValue;

static s32 np_s32(np_TypeValue *self) { return (s32) ((f64*) self->value)[0]; }
static s32 np_f32(np_TypeValue *self) { return (f32) ((f64*) self->value)[0]; }
static s32 np_f64(np_TypeValue *self) { return ((f64*) self->value)[0]; }
static s32 np_s64(np_TypeValue *self) { return (s64) ((f64*) self->value)[0]; }
static s32 np_u32(np_TypeValue *self) { return (u32) ((f64*) self->value)[0]; }

typedef np_TypeValue (*FunctionSymbolPtr)(np_Compiler*, np_TypeValue*,
					  np_TypeValue*);

#define np_FUNCTION_HANDLER(name) \
	internal np_TypeValue name(np_Compiler* compiler, \
	np_TypeValue *params_begin, np_TypeValue *params_end)

#define np_FUNCTION_HANDLER_FLAG(name) \
	internal np_TypeValue name(np_Compiler* compiler, \
	np_TypeValue *params_begin, np_TypeValue *params_end, s32 flag)

typedef struct {
	np_SymbolID id;
	MemoryBlock name;
	b8          is_variable:1;
	b8          is_function:1;
	union {
		struct {
			void   *value;
			np_TypeID  type_id;
		} variable;
		struct {
			FunctionSymbolPtr ptr;
			np_TypeID  return_type;
			np_TypeID *param_type_begin;
			np_TypeID *param_type_end;
			b8      var_params;      // accepts variable number of parameters
			np_TypeID  var_params_type; // if
		} function;
	};
} np_Symbol;

typedef struct {
	np_Symbol *begin;
	np_Symbol *end;
	np_Symbol *capacity;
} np_SymbolTable;

//------------------------------------------------------------------------------
// Compiler
//------------------------------------------------------------------------------

typedef struct np_TypeValueList {
	np_TypeValue             data;
	struct np_TypeValueList *next;
} np_TypeValueList;

typedef struct {
	BilinearAllocatorCheckpoint left_checkpoint;
	BilinearAllocatorCheckpoint right_checkpoint;
	s64                         num_symbols;
	s64                         num_types;
} np_CompilerCheckpoint;

#define np_Compiler_ERROR_LOG_SIZE 4096

#define np_TYPE_UNDEFINED 0
#define np_TYPE_NUMBER    1
#define np_TYPE_STRING    2

struct np_Compiler {
	BilinearAllocator *memory;

	np_TypeTable    type_table;
	np_SymbolTable  symbol_table;

	// primitive types of reductions
	np_TypeID       undefined_type_id;
	np_TypeID       number_type_id;
	np_TypeID       string_type_id;

	struct {

		/* a block of 4K is allocated on memory on _init */
		char   log_buffer[np_Compiler_ERROR_LOG_SIZE];
		Print  log;

		b8     success;

		char  *text_begin; // to print the context of an error these
		char  *text_end;   // pointers should be initialized at the
		                   // beginning of the reduce operation

		np_TypeValueList *statement_results;

		np_AST_Node* current_context;

	} reduce;
};



//------------------------------------------------------------------------------
// Tokenization procedures
//------------------------------------------------------------------------------

internal char*
nt_TokenType_cstr(nt_TokenType ntt)
{
	static char *arr[] =
	{
		"TOKEN_EOF"            ,
		"TOKEN_SKIP"           ,
		"TOKEN_INT"            ,
		"TOKEN_FLOAT"          ,
		"TOKEN_IDENTIFIER"     ,
		"TOKEN_BINOP"          ,
		"TOKEN_COMMA"          ,
		"TOKEN_STRING"         ,
		"TOKEN_SEMICOLON"      ,
		"TOKEN_LPAREN"         ,
		"TOKEN_RPAREN"         ,
		"TOKEN_LBRACKET"       ,
		"TOKEN_RBRACKET"       ,
		"TOKEN_LBRACE"         ,
		"TOKEN_RBRACE"         ,
		"TOKEN_EQUAL"          ,
		"TOKEN_EQUAL_EQUAL"    ,
		"TOKEN_SLASH"          ,
		"TOKEN_COMMENT"        ,
		"TOKEN_MULTILINE_STRING"
	};
	return arr[(int)ntt];
}

internal char*
np_AST_Node_Type_cstr(np_AST_Node_Type nt)
{
	static char *arr[] =
	{
		"AST_Node_Type_Number",
		"AST_Node_Type_Function",
		"AST_Node_Type_Binary_Operation",
		"AST_Node_Type_Assignment",
		"AST_Node_Type_Group",
		"AST_Node_Type_Variable",
		"AST_Node_Type_String"
	};
	return arr[(int)nt];
}


/* specific tokenizer for this parser */
internal void
np_initialize_tokenizer(nt_Tokenizer *tokenizer, char* text_begin, char *text_end)
{
	nt_Tokenizer_init(tokenizer);

	nt_Tokenizer_reset_text(tokenizer, text_begin, text_end);

	nt_Tokenizer_insert_skip_token(tokenizer, np_TOKEN_SKIP);
	nt_Tokenizer_insert_skip_token(tokenizer, np_TOKEN_COMMENT);

	const b8 MOVE_RIGHT = 1;
	const b8 DONT_MOVE  = 0;

	static char st_underscore[]  = "_";
	static char st_lletters[]    = "abcdefghijklmnopqrstuvxywz";
	static char st_uletters[]    = "ABCDEFGHIJKLMNOPQRSTUVXYWZ";
	static char st_digits[]      = "0123456789";
	static char st_exponent[]    = "eE";
	static char st_equal[]       = "=";
	static char st_sign[]        = "+-";
	static char st_skip[]        = " \t\n\r";
	static char st_newline[]     = "\n\r";
	static char st_period[]      = ".";
	static char st_lparen[]      = "(";
	static char st_rparen[]      = ")";
	static char st_lbracket[]    = "[";
	static char st_rbracket[]    = "]";
	static char st_lbrace[]      = "{";
	static char st_rbrace[]      = "}";
	static char st_semicolon[]   = ";";
	static char st_star[]        = "*";
	static char st_comma[]       = ",";
	// static char st_dquote[]      = "\"";
	static char st_squote[]      = "'";
	static char st_slash[]       = "/";
	static char st_hash[]        = "#";
	static char st_at[]          = "@";

	nt_CharSet digits, exponent, sign, skip, period,
		   any, lletters, uletters, underscore,
		   lparen, rparen, lbracket, rbracket,
		   lbrace, rbrace, semicolon, squote,
		   neg_squote, slash, newline, neg_newline,
		   equal, comma, star, hash, at;

	nt_CharSet_init(&comma,        st_comma,        cstr_end(st_comma)        , 0);
	nt_CharSet_init(&equal,        st_equal,        cstr_end(st_equal)        , 0);
	nt_CharSet_init(&slash,        st_slash,        cstr_end(st_slash)        , 0);
	nt_CharSet_init(&lparen,       st_lparen,       cstr_end(st_lparen)       , 0);
	nt_CharSet_init(&rparen,       st_rparen,       cstr_end(st_rparen)       , 0);
	nt_CharSet_init(&lbracket,     st_lbracket,     cstr_end(st_lbracket)     , 0);
	nt_CharSet_init(&rbracket,     st_rbracket,     cstr_end(st_rbracket)     , 0);
	nt_CharSet_init(&lbrace,       st_lbrace,       cstr_end(st_lbrace)       , 0);
	nt_CharSet_init(&rbrace,       st_rbrace,       cstr_end(st_rbrace)       , 0);
	nt_CharSet_init(&underscore,   st_underscore,   cstr_end(st_underscore)   , 0);
	nt_CharSet_init(&lletters,     st_lletters,     cstr_end(st_lletters)     , 0);
	nt_CharSet_init(&uletters,     st_uletters,     cstr_end(st_uletters)     , 0);
	nt_CharSet_init(&digits,       st_digits,       cstr_end(st_digits)       , 0);
	nt_CharSet_init(&sign,         st_sign,         cstr_end(st_sign)         , 0);
	nt_CharSet_init(&skip,         st_skip,         cstr_end(st_skip)         , 0);
	nt_CharSet_init(&period,       st_period,       cstr_end(st_period)       , 0);
	nt_CharSet_init(&exponent,     st_exponent,     cstr_end(st_exponent)     , 0);
	nt_CharSet_init(&semicolon,    st_semicolon,    cstr_end(st_semicolon)    , 0);
	nt_CharSet_init(&squote,       st_squote,       cstr_end(st_squote)       , 0);
	nt_CharSet_init(&neg_squote,   st_squote,       cstr_end(st_squote)       , 1);
	nt_CharSet_init(&newline,      st_newline,      cstr_end(st_newline)      , 0);
	nt_CharSet_init(&neg_newline,  st_newline,      cstr_end(st_newline)      , 1);
	nt_CharSet_init(&star,         st_star,         cstr_end(st_star)         , 0);
	nt_CharSet_init(&hash,         st_hash,         cstr_end(st_hash)         , 0);
	nt_CharSet_init(&at,           st_at,           cstr_end(st_at)           , 0);
	nt_CharSet_init_any(&any);

	nt_Tokenizer_add_transition(tokenizer,  0, &skip        ,   1, MOVE_RIGHT, nt_ACTION_BEGIN_TOKEN,    0);
	nt_Tokenizer_add_transition(tokenizer,  0, &sign        ,   2, MOVE_RIGHT, nt_ACTION_BEGIN_TOKEN,    0);
	nt_Tokenizer_add_transition(tokenizer,  0, &digits      ,   3, MOVE_RIGHT, nt_ACTION_BEGIN_TOKEN,    0);
	nt_Tokenizer_add_transition(tokenizer,  0, &period      ,   9, MOVE_RIGHT, nt_ACTION_BEGIN_TOKEN,    0);
	nt_Tokenizer_add_transition(tokenizer,  0, &underscore  ,  10, MOVE_RIGHT, nt_ACTION_BEGIN_TOKEN,    0);
	nt_Tokenizer_add_transition(tokenizer,  0, &lletters    ,  10, MOVE_RIGHT, nt_ACTION_BEGIN_TOKEN,    0);
	nt_Tokenizer_add_transition(tokenizer,  0, &uletters    ,  10, MOVE_RIGHT, nt_ACTION_BEGIN_TOKEN,    0);
	nt_Tokenizer_add_transition(tokenizer,  0, &squote      ,  11, MOVE_RIGHT, nt_ACTION_BEGIN_TOKEN,    0);
	nt_Tokenizer_add_transition(tokenizer,  0, &comma       ,   0, MOVE_RIGHT, nt_ACTION_EMIT_SINGLETON, np_TOKEN_COMMA);
	nt_Tokenizer_add_transition(tokenizer,  0, &lparen      ,   0, MOVE_RIGHT, nt_ACTION_EMIT_SINGLETON, np_TOKEN_LPAREN);
	nt_Tokenizer_add_transition(tokenizer,  0, &rparen      ,   0, MOVE_RIGHT, nt_ACTION_EMIT_SINGLETON, np_TOKEN_RPAREN);
	nt_Tokenizer_add_transition(tokenizer,  0, &lbracket    ,   0, MOVE_RIGHT, nt_ACTION_EMIT_SINGLETON, np_TOKEN_LBRACKET);
	nt_Tokenizer_add_transition(tokenizer,  0, &rbracket    ,   0, MOVE_RIGHT, nt_ACTION_EMIT_SINGLETON, np_TOKEN_RBRACKET);
	nt_Tokenizer_add_transition(tokenizer,  0, &lbrace      ,   0, MOVE_RIGHT, nt_ACTION_EMIT_SINGLETON, np_TOKEN_LBRACE);
	nt_Tokenizer_add_transition(tokenizer,  0, &rbrace      ,   0, MOVE_RIGHT, nt_ACTION_EMIT_SINGLETON, np_TOKEN_RBRACE);
	nt_Tokenizer_add_transition(tokenizer,  0, &semicolon   ,   0, MOVE_RIGHT, nt_ACTION_EMIT_SINGLETON, np_TOKEN_SEMICOLON);
	nt_Tokenizer_add_transition(tokenizer,  0, &star        ,   0, MOVE_RIGHT, nt_ACTION_EMIT_SINGLETON, np_TOKEN_BINOP);
	nt_Tokenizer_add_transition(tokenizer,  0, &slash       ,   0, MOVE_RIGHT, nt_ACTION_EMIT_SINGLETON, np_TOKEN_BINOP);
	nt_Tokenizer_add_transition(tokenizer,  0, &equal       ,  14, MOVE_RIGHT, nt_ACTION_BEGIN_TOKEN,    0);
	nt_Tokenizer_add_transition(tokenizer,  0, &hash        ,  13, MOVE_RIGHT, nt_ACTION_BEGIN_TOKEN,    0);
	nt_Tokenizer_add_transition(tokenizer,  0, &at          ,  15, MOVE_RIGHT, nt_ACTION_BEGIN_TOKEN,    0);

	nt_Tokenizer_add_transition(tokenizer,  1, &skip        ,   1, MOVE_RIGHT, nt_ACTION_DO_NOTHING,     0);
	nt_Tokenizer_add_transition(tokenizer,  1, &any         ,   0, DONT_MOVE , nt_ACTION_EMIT_TOKEN,     np_TOKEN_SKIP);

	nt_Tokenizer_add_transition(tokenizer,  2, &digits      ,   3, MOVE_RIGHT, nt_ACTION_DO_NOTHING,     0);
	nt_Tokenizer_add_transition(tokenizer,  2, &any         ,   0, DONT_MOVE , nt_ACTION_EMIT_TOKEN,     np_TOKEN_BINOP);

	nt_Tokenizer_add_transition(tokenizer,  3, &digits      ,   3, MOVE_RIGHT, nt_ACTION_DO_NOTHING,     0);
	nt_Tokenizer_add_transition(tokenizer,  3, &period      ,   4, MOVE_RIGHT, nt_ACTION_DO_NOTHING,     0);
	nt_Tokenizer_add_transition(tokenizer,  3, &exponent    ,   6, MOVE_RIGHT, nt_ACTION_DO_NOTHING,     0);
	nt_Tokenizer_add_transition(tokenizer,  3, &any         ,   0, DONT_MOVE , nt_ACTION_EMIT_TOKEN,     np_TOKEN_INT);

	nt_Tokenizer_add_transition(tokenizer,  4, &digits      ,   5, MOVE_RIGHT, nt_ACTION_DO_NOTHING,     0);
	nt_Tokenizer_add_transition(tokenizer,  4, &any         ,   0, DONT_MOVE,  nt_ACTION_EMIT_TOKEN,     np_TOKEN_FLOAT);

	nt_Tokenizer_add_transition(tokenizer,  5, &digits      ,   5, MOVE_RIGHT, nt_ACTION_DO_NOTHING,     0);
	nt_Tokenizer_add_transition(tokenizer,  5, &exponent    ,   6, MOVE_RIGHT, nt_ACTION_DO_NOTHING,     0);
	nt_Tokenizer_add_transition(tokenizer,  5, &any         ,   0, DONT_MOVE,  nt_ACTION_EMIT_TOKEN,     np_TOKEN_FLOAT);

	nt_Tokenizer_add_transition(tokenizer,  6, &sign        ,   7, MOVE_RIGHT, nt_ACTION_DO_NOTHING,     0);
	nt_Tokenizer_add_transition(tokenizer,  6, &digits      ,   8, MOVE_RIGHT, nt_ACTION_DO_NOTHING,     0);

	nt_Tokenizer_add_transition(tokenizer,  7, &digits      ,   8, MOVE_RIGHT, nt_ACTION_DO_NOTHING,     0);

	nt_Tokenizer_add_transition(tokenizer,  8, &digits      ,   8, MOVE_RIGHT, nt_ACTION_DO_NOTHING,     0);
	nt_Tokenizer_add_transition(tokenizer,  8, &any         ,   0, DONT_MOVE,  nt_ACTION_EMIT_TOKEN,     np_TOKEN_FLOAT);

	nt_Tokenizer_add_transition(tokenizer,  9, &digits      ,  5, MOVE_RIGHT,  nt_ACTION_DO_NOTHING,     0);
	nt_Tokenizer_add_transition(tokenizer,  9, &any         ,  0, DONT_MOVE,   nt_ACTION_EMIT_TOKEN,     np_TOKEN_BINOP);

	nt_Tokenizer_add_transition(tokenizer, 10, &lletters    ,  10, MOVE_RIGHT, nt_ACTION_DO_NOTHING,     0);
	nt_Tokenizer_add_transition(tokenizer, 10, &digits      ,  10, MOVE_RIGHT, nt_ACTION_DO_NOTHING,     0);
	nt_Tokenizer_add_transition(tokenizer, 10, &uletters    ,  10, MOVE_RIGHT, nt_ACTION_DO_NOTHING,     0);
	nt_Tokenizer_add_transition(tokenizer, 10, &underscore  ,  10, MOVE_RIGHT, nt_ACTION_DO_NOTHING,     0);
	nt_Tokenizer_add_transition(tokenizer, 10, &any         ,   0, DONT_MOVE , nt_ACTION_EMIT_TOKEN,     np_TOKEN_IDENTIFIER);

	nt_Tokenizer_add_transition(tokenizer, 11, &neg_squote  ,  11, MOVE_RIGHT, nt_ACTION_DO_NOTHING,     0);
	nt_Tokenizer_add_transition(tokenizer, 11, &squote      ,   0, MOVE_RIGHT, nt_ACTION_EMIT_TOKEN_INC, np_TOKEN_STRING);

	nt_Tokenizer_add_transition(tokenizer, 13, &neg_newline ,  13, MOVE_RIGHT, nt_ACTION_DO_NOTHING,     0);
	nt_Tokenizer_add_transition(tokenizer, 13, &any         ,   0, DONT_MOVE,  nt_ACTION_EMIT_TOKEN,     np_TOKEN_COMMENT);

	nt_Tokenizer_add_transition(tokenizer, 14, &equal       ,   0, MOVE_RIGHT, nt_ACTION_EMIT_TOKEN_INC, np_TOKEN_EQUAL_EQUAL);
	nt_Tokenizer_add_transition(tokenizer, 14, &any         ,   0, DONT_MOVE,  nt_ACTION_EMIT_TOKEN,     np_TOKEN_EQUAL);

	nt_Tokenizer_add_transition(tokenizer, 15, &newline     ,  16, MOVE_RIGHT, nt_ACTION_DO_NOTHING,    0);

	nt_Tokenizer_add_transition(tokenizer, 16, &newline     ,  17, MOVE_RIGHT, nt_ACTION_DO_NOTHING,    0);
	nt_Tokenizer_add_transition(tokenizer, 16, &any         ,  16, MOVE_RIGHT, nt_ACTION_DO_NOTHING,    0);

	nt_Tokenizer_add_transition(tokenizer, 17, &at          ,   0, MOVE_RIGHT, nt_ACTION_EMIT_TOKEN_INC, np_TOKEN_MULTILINE_STRING);
	nt_Tokenizer_add_transition(tokenizer, 17, &newline     ,  17, MOVE_RIGHT, nt_ACTION_DO_NOTHING,     0);
	nt_Tokenizer_add_transition(tokenizer, 17, &any         ,  16, MOVE_RIGHT, nt_ACTION_DO_NOTHING,     0);

}

//------------------------------------------------------------------------------
// Parser procedures
//------------------------------------------------------------------------------

internal void
np_Parser_init(np_Parser* self, nt_Tokenizer *tokenizer, BilinearAllocator *ast_memory)
{
	self->tokenizer = tokenizer;
	self->tkbegin   = self->buffer;
	self->tkend     = self->buffer;
	self->eof       = 0;
	self->tkerror   = 0;

	// ast memory
	self->ast_memory = ast_memory;

	// sneak in the ast stack into the end range of the ast_memory
	self->stack.end   = 0; // BilinearAllo(np_AST_Node_Ptr*) LALIGN((u64)self->ast_memory->capacity, 8);
	self->stack.begin = 0; // self->stack.end;

	self->ast_first = 0; // initially AST is empty
	self->ast_last  = 0;

	// TODO(llins): maybe set a general global flag VERY_CLEAN_INITIALIZATIONS
	// and clear memory that by design doesn't need to be cleaned. Might help
	// tracking bugs although it is slower.
	// for (u32 i=0;i<np_Parser_STACK_CAPACITY;++i)
	//	self->stack[i] = 0;

	/* Initialize log */
	Print_init(&self->log, self->log_buffer, self->log_buffer + sizeof(self->log_buffer));
}

// after a run, we should reset the tokenizer (new text) and the call
// this function to reset the parser
internal void
np_Parser_reset(np_Parser* self)
{
	self->tkbegin     = self->buffer;
	self->tkend       = self->buffer;
	self->eof         = 0;
	self->tkerror     = 0;
	self->stack.begin = 0;
	self->stack.end   = 0;
	self->ast_first   = 0; // initially AST is empty
	self->ast_last    = 0;
	Print_clear(&self->log);
}

/* return 0 if out of memory */
internal b8
np_Parser_push_ast_node(np_Parser *self, np_AST_Node *node)
{
	//
	// Assumes no other entity is messing up with the
	// biliner allocator right portion
	//
	if (BilinearAllocator_free_space(self->ast_memory) < sizeof(np_AST_Node*)) {
		return 0;
	}
	self->stack.begin = (np_AST_Node_Ptr*) BilinearAllocator_alloc_right(self->ast_memory, sizeof(np_AST_Node*));
	if (self->stack.end == 0) {
		// first time pushing a node
		self->stack.end   = self->stack.begin + 1;
	}
	*self->stack.begin = node;
	return 1;
}

internal void
np_Parser_pop_ast_nodes(np_Parser *self, u32 n)
{
	self->stack.begin += n;
	BilinearAllocator_pop_right(self->ast_memory, n * sizeof(np_AST_Node*));
//
// 	Assert(self->stack_size >= n);
// 	u32 i = self->stack_size;
// 	while (n > 0) {
// 		self->stack[--i] = 0;
// 		--n;
// 	}
// 	self->stack_size = i;
//
}

internal inline np_AST_Node*
np_Parser_ast_stack_top(np_Parser *self)
{
	Assert(self->stack.begin < self->stack.end);
	return self->stack.begin[0];
}

internal inline u32
np_Parser_ast_stack_top_index(np_Parser *self)
{
	Assert(self->stack.begin < self->stack.end);
	return (u32) (self->stack.end - self->stack.begin - 1);
}

internal inline np_AST_Node*
np_Parser_ast_stack_get(np_Parser *self, u32 index)
{
	np_AST_Node_Ptr *ptr = self->stack.end - index - 1;
	Assert(self->stack.begin <= ptr);
	return *ptr;
}

internal void
np_Parser_append_statement_and_clear_stack(np_Parser *self)
{
	Assert(self->stack.begin < self->stack.end);

	np_AST_Node* node = np_Parser_ast_stack_top(self);
	np_Parser_pop_ast_nodes(self, 1);

	if (self->ast_first) {
		self->ast_last->next = node;
		self->ast_last       = node;
	} else {
		self->ast_first = node;
		self->ast_last  = node;
	}
}

internal np_AST_Node*
np_Parser_ast_node(np_Parser *self, np_AST_Node_Type type, char *begin, char *end, void *detail)
{
	np_AST_Node *node = (np_AST_Node*) BilinearAllocator_alloc_left(self->ast_memory, sizeof(np_AST_Node));
	node->type     = type;
	node->begin    = begin;
	node->end      = end;
	node->detail   = detail;
	node->next     = 0;
	return node;
}

internal np_AST_Number*
np_Parser_ast_number_float(np_Parser *self, f64 value)
{
	np_AST_Number *number = (np_AST_Number*) BilinearAllocator_alloc_left(self->ast_memory, sizeof(np_AST_Number));

	number->is_integer = 0;
	number->fp         = value;
	return number;
}

internal np_AST_Number*
np_Parser_ast_number_int(np_Parser *self, s64 value)
{
	np_AST_Number *number = (np_AST_Number*) BilinearAllocator_alloc_left(self->ast_memory, sizeof(np_AST_Number));

	number->is_integer = 1;
	number->ip         = value;
	return number;
}

internal np_AST_Variable*
np_Parser_ast_variable(np_Parser *self, char *begin, char *end)
{
	np_AST_Variable *variable = (np_AST_Variable*) BilinearAllocator_alloc_left(self->ast_memory, sizeof(np_AST_Variable));

	variable->name.begin = begin;
	variable->name.end   = end;
	return variable;
}

internal np_AST_String*
np_Parser_ast_string(np_Parser *self, char *begin, char *end)
{
	np_AST_String *string = (np_AST_String*) BilinearAllocator_alloc_left(self->ast_memory, sizeof(np_AST_String));

	string->str.begin = begin;
	string->str.end   = end;
	return string;
}

internal np_AST_Group*
np_Parser_ast_group(np_Parser *self, np_AST_Node *node)
{
	np_AST_Group *group = (np_AST_Group*) BilinearAllocator_alloc_left(self->ast_memory, sizeof(np_AST_Group));

	group->node = node;
	return group;
}

internal np_AST_Function*
np_Parser_ast_function(np_Parser *self, char *begin, char *end)
{
	np_AST_Function *function = (np_AST_Function*) BilinearAllocator_alloc_left(self->ast_memory, sizeof(np_AST_Function));

	function->name.begin = begin;
	function->name.end   = end;
	function->num_parameters = 0;
	function->first_parameter = 0;
	return function;
}

internal np_AST_Function_Parameter*
np_Parser_ast_function_parameter(np_Parser *self, np_AST_Node *node, np_AST_Function_Parameter *next)
{
	np_AST_Function_Parameter *param = (np_AST_Function_Parameter*) BilinearAllocator_alloc_left(self->ast_memory, sizeof(np_AST_Function_Parameter));

	param->node = node;
	param->next = next;
	return param;
}

internal np_AST_Binary_Operation*
np_Parser_ast_binary_operation(np_Parser *self, char *begin, char *end, np_AST_Node *left, np_AST_Node *right)
{
	np_AST_Binary_Operation *binop = (np_AST_Binary_Operation*) BilinearAllocator_alloc_left(self->ast_memory, sizeof(np_AST_Binary_Operation));

	binop->name.begin = begin;
	binop->name.end   = end;
	binop->left       = left;
	binop->right      = right;
	return binop;
}

internal np_AST_Assignment*
np_Parser_ast_assignment(np_Parser *self, char *begin, char *end, np_AST_Node *node)
{
	np_AST_Assignment *assignment = (np_AST_Assignment*) BilinearAllocator_alloc_left(self->ast_memory, sizeof(np_AST_Assignment));

	assignment->name.begin = begin;
	assignment->name.end   = end;
	assignment->node       = node;
	return assignment;
}

internal u8
np_ast_precedence_from_name(np_AST_Binary_Operation *binop)
{
	char *begin = binop->name.begin;
	char *end   = binop->name.end;
	if (pt_compare_memory_cstr(begin, end, "+")==0) return 2;
	else if (pt_compare_memory_cstr(begin, end, "-")==0) return 2;
	else if (pt_compare_memory_cstr(begin, end, "/")==0) return 3;
	else if (pt_compare_memory_cstr(begin, end, "*")==0) return 3;
	else if (pt_compare_memory_cstr(begin, end, ".")==0) return 4;
	else return 10;
}

internal np_AST_Node*
np_ast_adjust_root(np_AST_Node* root)
{
	Assert(root->type == np_AST_Node_Type_Binary_Operation);

	np_AST_Binary_Operation *root_binop = (np_AST_Binary_Operation*) root->detail;

	b8 left_is_binop   = root_binop->left->type  == np_AST_Node_Type_Binary_Operation;
	b8 right_is_binop  = root_binop->right->type == np_AST_Node_Type_Binary_Operation;
	u8 root_precedence = np_ast_precedence_from_name(root_binop);

	b8 rotate_left  = 0;
	b8 rotate_right = 0;

	np_AST_Node *left  = 0;
	np_AST_Node *right = 0;
	np_AST_Binary_Operation *left_binop  = 0;
	np_AST_Binary_Operation *right_binop = 0;

	if (left_is_binop && right_is_binop) {
		left        = root_binop->left;
		right       = root_binop->right;
		left_binop  = (np_AST_Binary_Operation*) left->detail;
		right_binop = (np_AST_Binary_Operation*) right->detail;
		u8 left_precedence  = np_ast_precedence_from_name(left_binop);
		u8 right_precedence = np_ast_precedence_from_name(right_binop);
		if (right_precedence <= root_precedence && right_precedence <= left_precedence) {
			rotate_left = 1;
		} else if (left_precedence < root_precedence && left_precedence < right_precedence) {
			rotate_right = 1;
		}
	} else if (left_is_binop) {
		left        = root_binop->left;
		left_binop  = (np_AST_Binary_Operation*) left->detail;
		u8 left_precedence  = np_ast_precedence_from_name(left_binop);
		if (left_precedence < root_precedence) {
			rotate_right = 1;
		}
	} else if (right_is_binop) {
		right = root_binop->right;
		right_binop = (np_AST_Binary_Operation*) right->detail;
		u8 right_precedence = np_ast_precedence_from_name(right_binop);
		if (right_precedence <= root_precedence) {
			rotate_left = 1;
		}
	}

	if (rotate_left) {
		root_binop->right = right_binop->left;
		right_binop->left = np_ast_adjust_root(root);
		return right;
	} else if (rotate_right) {
		root_binop->left  = left_binop->right;
		left_binop->right = np_ast_adjust_root(root);
		return left;
	} else {
		return root;
	}

}

//
// rearrange AST to guarantee precedence and left assiaciativity
// of the binary operators
//
internal np_AST_Node*
np_ast_normalize(np_AST_Node* node)
{
	switch(node->type)
	{
	case np_AST_Node_Type_Number:
	case np_AST_Node_Type_Variable:
	case np_AST_Node_Type_String:
		return node;
	case np_AST_Node_Type_Assignment:
		{
			np_AST_Assignment *assignment = (np_AST_Assignment*) node->detail;
			assignment->node = np_ast_normalize(assignment->node);
		} return node;
	case np_AST_Node_Type_Group:
		{
			np_AST_Group *group = (np_AST_Group*) node->detail;
			group->node = np_ast_normalize(group->node);
		} return node;
	case np_AST_Node_Type_Function:
		{
			np_AST_Function *function = (np_AST_Function*) node->detail;
			np_AST_Function_Parameter *param = function->first_parameter;
			while (param) {
				param->node = np_ast_normalize(param->node);
				param = param->next;
			}
		} return node;
	case np_AST_Node_Type_Binary_Operation:
		{
			np_AST_Binary_Operation *binop = (np_AST_Binary_Operation*) node->detail;
			binop->left  = np_ast_normalize(binop->left);
			binop->right = np_ast_normalize(binop->right);
		} return np_ast_adjust_root(node);
	default:
		{
			Assert(0);
		} return node;
	}
}

internal void
np_Parser_fill_buffer(np_Parser *self)
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
	nt_Token *end = self->buffer + np_Parser_BUFFER_SIZE;
	while (self->tkend != end)
	{
		if (nt_Tokenizer_next(self->tokenizer)) {
			*self->tkend++ = self->tokenizer->token;
		}
		else {
			if (self->tokenizer->next_result_detail == nt_TOKENIZER_NEXT_RESULT_INVALID_INPUT) {
				self->tkerror = 1;
				break;
			} else {
				self->eof = 1;
				break;
			}
		}
	}
}

internal void
np_Parser_consume_tokens(np_Parser *self, u32 n)
{
	Assert(self->tkend - self->tkbegin >= n);
	self->tkbegin += n;
	if (self->tkend - self->tkbegin < np_Parser_LOOKAHEAD) {
		np_Parser_fill_buffer(self);
	}
}

internal inline b8
np_Parser_compare_next(np_Parser *self, nt_TokenType t)
{
	return self->tkbegin < self->tkend && self->tkbegin->type == t;
}

internal inline b8
np_Parser_compare_next2(np_Parser *self, nt_TokenType t1, nt_TokenType t2)
{
	return (self->tkbegin+1) < self->tkend &&
		self->tkbegin->type == t1 &&
		(self->tkbegin+1)->type == t2;
}

internal b8
np_Parser_E(np_Parser *self);

internal b8
np_Parser_EC(np_Parser *self);

internal b8
np_Parser_P(np_Parser *self);

internal b8
np_Parser_PS(np_Parser *self);

internal void
np_Parser_log_context(np_Parser *self)
{
	b8 no_token = self->tkbegin == self->tkend;
	char *pos = 0;
	if (no_token) {
		pos = self->tokenizer->it;
	} else {
		pos = self->tkbegin->begin;
	}

	char *begin = self->tokenizer->text_begin;
	char *end   = self->tokenizer->text_end;

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
		Print_u64(print, self->tokenizer->line);
		Print_cstr(print, " column: ");
		Print_u64(print, self->tokenizer->column);
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



//
//  P   -> )
//       | E PS
//
internal b8
np_Parser_P(np_Parser *self)
{
	if (np_Parser_compare_next(self, np_TOKEN_RPAREN)) {
		// function call
		np_Parser_consume_tokens(self,1);
		return 1;
	} else {
		if (!np_Parser_E(self)) return 0;
		if (!np_Parser_PS(self)) return 0;
		return 1;
	}
}

//
//  PS  -> )
//       | , E PS
//
internal b8
np_Parser_PS(np_Parser *self)
{
	if (np_Parser_compare_next(self, np_TOKEN_RPAREN)) {
		// function call
		np_Parser_consume_tokens(self,1);

		return 1;

	} else if (np_Parser_compare_next(self, np_TOKEN_COMMA)) {

		np_Parser_consume_tokens(self,1);

		if (!np_Parser_E(self)) return 0;

		if (!np_Parser_PS(self)) return 0;

		return 1;

	} else {

		/* error message */
		{
			Print_cstr(&self->log, "[parser error] Expecting either ')' or ',' on \n");
			np_Parser_log_context(self);
		}

		return 0;
	}
}

//
//  EC  -> - E
//       | + E
//       | / E
//       | * E
//       | . E
//       | \epsilon
//
internal b8
np_Parser_EC(np_Parser *self)
{
	if (np_Parser_compare_next(self, np_TOKEN_BINOP)) {

		{ // AST
			Assert(self->stack.begin < self->stack.end);

			// replace node on top of stack with binary op
			// node on top becomes left argument of binary op

			np_AST_Node *left = np_Parser_ast_stack_top(self);
			np_Parser_pop_ast_nodes(self, 1);

			b8 push_ok = np_Parser_push_ast_node(self,
						np_Parser_ast_node (self,
								    np_AST_Node_Type_Binary_Operation,
								    self->tkbegin->begin,
								    self->tkbegin->end,
								    np_Parser_ast_binary_operation(self,
												   self->tkbegin->begin,
												   self->tkbegin->end,
												   left,
												   0)
								   )
					       );
			if (!push_ok) {
				Print_cstr(&self->log, "[parser error] Not enough memory to stack AST nodes.\n");
				return 0;
			}

		} // AST

#ifdef CHECK_ASSERTIONS
		u32 binop_stack_index = np_Parser_ast_stack_top_index(self);
#endif

		// function call
		np_Parser_consume_tokens(self,1);

		if (!np_Parser_E(self)) return 0;

		{ // AST
			Assert(binop_stack_index + 1 == np_Parser_ast_stack_top_index(self));

			np_AST_Node *right = np_Parser_ast_stack_top(self);
			np_Parser_pop_ast_nodes(self, 1);

			np_AST_Binary_Operation *binop = (np_AST_Binary_Operation*) np_Parser_ast_stack_top(self)->detail;
			binop->right = right;

		} // AST

		return 1;
	}
	else {
		return 1;
	}
}


//
// TODO(llins): implement number continuation (numerical tokens that include signs)
// ex. 10+20
//

//
//  E   -> id ( P EC
//       | id EC
//       | num EC
//       | ( E ) EC
//       | - E
//
internal b8
np_Parser_E(np_Parser *self)
{
	if (np_Parser_compare_next2(self, np_TOKEN_IDENTIFIER, np_TOKEN_LPAREN)) {

		{ // AST
			b8 push_ok = np_Parser_push_ast_node(self,
						np_Parser_ast_node(self,
								   np_AST_Node_Type_Function,
								   self->tkbegin->begin,
								   self->tkbegin->end,
								   np_Parser_ast_function(self, self->tkbegin->begin,
											  self->tkbegin->end)));
			if (!push_ok) {
				Print_cstr(&self->log, "[parser error] Not enough memory to stack AST nodes.\n");
				return 0;
			}
		} // AST

		u32 function_stack_index = np_Parser_ast_stack_top_index(self);

		// function call
		np_Parser_consume_tokens(self,2);

		// parser parameters
		if (!np_Parser_P(self)) return 0;

		{ // AST

			// function
			np_AST_Function *function = (np_AST_Function*) np_Parser_ast_stack_get(self,function_stack_index)->detail;

			u32 current_stack_index = np_Parser_ast_stack_top_index(self);

			u32 num_parameters = 0;
			np_AST_Function_Parameter *first_parameter = 0;
			for (u32 i=current_stack_index; i>function_stack_index; --i)
			{
				++num_parameters;
				first_parameter = np_Parser_ast_function_parameter(self, np_Parser_ast_stack_get(self, i), first_parameter);
			}

			function->num_parameters  = num_parameters;
			function->first_parameter = first_parameter;

			np_Parser_pop_ast_nodes(self, num_parameters);
		} // AST

		if (!np_Parser_EC(self)) return 0;

		return 1;

	} else if (np_Parser_compare_next(self, np_TOKEN_IDENTIFIER)) {

		{ // AST
			b8 push_ok = np_Parser_push_ast_node(self,
						np_Parser_ast_node(self,
								   np_AST_Node_Type_Variable,
								   self->tkbegin->begin,
								   self->tkbegin->end,
								   np_Parser_ast_variable(self, self->tkbegin->begin, self->tkbegin->end)
								  )
					       );
			if (!push_ok) {
				Print_cstr(&self->log, "[parser error] Not enough memory to stack AST nodes.\n");
				return 0;
			}
		} // AST

		np_Parser_consume_tokens(self,1);

		if (!np_Parser_EC(self)) return 0;

		return 1;

	} else if (np_Parser_compare_next(self, np_TOKEN_INT)) {

		{ // AST_Node
			s64 value = 0;
			b8 ok = pt_parse_s64(self->tkbegin->begin, self->tkbegin->end, &value);
			if (!ok) {
				/* error message */
				{
					Print_cstr(&self->log, "[parser error] Could not parse number\n");
					np_Parser_log_context(self);
				}
				return 0;
			}

			np_AST_Number *num = np_Parser_ast_number_int(self, value);
			np_AST_Node *node = np_Parser_ast_node(self, np_AST_Node_Type_Number,
							       self->tkbegin->begin,
							       self->tkbegin->end, num);
			np_Parser_push_ast_node(self, node);
		} // AST_Node

		np_Parser_consume_tokens(self,1);

		if (!np_Parser_EC(self)) return 0;

		return 1;

	} else if (np_Parser_compare_next(self, np_TOKEN_FLOAT)) {

		{ // AST_Node
			f64 value = 0;
			b8 ok = pt_parse_f64(self->tkbegin->begin, self->tkbegin->end, &value);
			if (!ok) {
				/* error message */
				{
					Print_cstr(&self->log, "[parser error] Could not parse number\n");
					np_Parser_log_context(self);
				}
				return 0;
			}

			np_AST_Number *num = np_Parser_ast_number_float(self, value);
			np_AST_Node *node = np_Parser_ast_node(self, np_AST_Node_Type_Number,
							       self->tkbegin->begin,
							       self->tkbegin->end, num);
			b8 push_ok = np_Parser_push_ast_node(self, node);
			if (!push_ok) {
				Print_cstr(&self->log, "[parser error] Not enough memory to stack AST nodes.\n");
				return 0;
			}
		} // AST_Node

		np_Parser_consume_tokens(self,1);

		if (!np_Parser_EC(self)) return 0;

		return 1;

	} else if (np_Parser_compare_next(self, np_TOKEN_STRING)) {

		{ // AST
			np_AST_String *st = np_Parser_ast_string(self, self->tkbegin->begin+1, self->tkbegin->end-1);
			np_AST_Node *node = np_Parser_ast_node(self, np_AST_Node_Type_String,
							       self->tkbegin->begin, self->tkbegin->end, st);
			b8 push_ok = np_Parser_push_ast_node(self, node);
			if (!push_ok) {
				Print_cstr(&self->log, "[parser error] Not enough memory to stack AST nodes.\n");
				return 0;
			}
		} // AST

		np_Parser_consume_tokens(self,1);

		if (!np_Parser_EC(self)) return 0;

		return 1;

	} else if (np_Parser_compare_next(self, np_TOKEN_MULTILINE_STRING)) {

		{ // AST
			// forget the @\n   and the final \n@
			np_AST_String *st = np_Parser_ast_string(self, self->tkbegin->begin+2, self->tkbegin->end-2);
			np_AST_Node *node = np_Parser_ast_node(self, np_AST_Node_Type_String,
							       self->tkbegin->begin, self->tkbegin->end, st);
			b8 push_ok = np_Parser_push_ast_node(self, node);
			if (!push_ok) {
				Print_cstr(&self->log, "[parser error] Not enough memory to stack AST nodes.\n");
				return 0;
			}
		} // AST

		np_Parser_consume_tokens(self,1);

		if (!np_Parser_EC(self)) return 0;

		return 1;

	} else if (np_Parser_compare_next(self, np_TOKEN_LPAREN)) {

		char *begin = self->tkbegin->begin;

		np_Parser_consume_tokens(self,1);

		if (!np_Parser_E(self)) return 0;

		// semantic action: make symbols table include id etc
		if (!np_Parser_compare_next(self, np_TOKEN_RPAREN)) {
			/* error message */
			{
				Print_cstr(&self->log, "[parser error] Expected ')' not found.\n");
				np_Parser_log_context(self);
			}
			return 0;
		}

		char *end = self->tkbegin->end;

		{ // AST

			Assert(self->stack.begin < self->stack.end);
			np_AST_Node *node = np_Parser_ast_stack_top(self);
			np_Parser_pop_ast_nodes(self, 1);
			b8 push_ok = np_Parser_push_ast_node(self, np_Parser_ast_node(self, np_AST_Node_Type_Group,
									 begin, end, np_Parser_ast_group(self, node)));
			if (!push_ok) {
				Print_cstr(&self->log, "[parser error] Not enough memory to stack AST nodes.\n");
				return 0;
			}
		} // AST

		np_Parser_consume_tokens(self,1);

		if (!np_Parser_EC(self)) return 0;

		return 1;

	} else {
		/* error message */
		{
			Print_cstr(&self->log, "[parser error] Unexpected token.\n");
			np_Parser_log_context(self);
		}
		return 0;
	}
}

internal b8
np_Parser_S(np_Parser *self)
{
	if (np_Parser_compare_next2(self, np_TOKEN_IDENTIFIER, np_TOKEN_EQUAL)) {

		{ // AST
			b8 push_ok = np_Parser_push_ast_node(self,
							np_Parser_ast_node(self, np_AST_Node_Type_Assignment,
									   self->tkbegin->begin, self->tkbegin->end,
									   np_Parser_ast_assignment(self, self->tkbegin->begin,
												    self->tkbegin->end, 0)));
			if (!push_ok) {
				Print_cstr(&self->log, "[parser error] Not enough memory to stack AST nodes.\n");
				return 0;
			}
		} // AST

		u32 assignment_stack_index = np_Parser_ast_stack_top_index(self);

		np_Parser_consume_tokens(self,2);

		if (!np_Parser_E(self)) return 0;

		{ // AST

			Assert(assignment_stack_index+1 == np_Parser_ast_stack_top_index(self));

			np_AST_Assignment *assignment = (np_AST_Assignment*) np_Parser_ast_stack_get(self,assignment_stack_index)->detail;

			assignment->node = np_Parser_ast_stack_top(self);

			np_Parser_pop_ast_nodes(self, 1);

		} // AST

		{ // AST
			np_Parser_append_statement_and_clear_stack(self);
		} // AST

		// semantic action: make symbols table include id etc
		if (np_Parser_compare_next(self, np_TOKEN_SEMICOLON)) {

			np_Parser_consume_tokens(self,1);
			if (!np_Parser_S(self)) return 0;
			return 1;

		} else if (np_Parser_compare_next(self, nt_TOKEN_EOF)) {

			np_Parser_consume_tokens(self,1);
			return 1;

		} else {
			/* error message */
			{
				// this message doesn't bring the line number and cursor position
				Print_cstr(&self->log, "[parser error] Semi-colon or end-of-file expected.\n");
				np_Parser_log_context(self);
			}
			return 0;

		}

	} else if (np_Parser_compare_next(self, nt_TOKEN_EOF)) {

		return 1;

	} else {

		if (!np_Parser_E(self)) return 0;

		{ // AST
			np_Parser_append_statement_and_clear_stack(self);
		} // AST

		// semantic action: make symbols table include id etc
		if (np_Parser_compare_next(self, np_TOKEN_SEMICOLON)) {

			np_Parser_consume_tokens(self,1);
			if (!np_Parser_S(self)) return 0;
			return 1;

		} else if (np_Parser_compare_next(self, nt_TOKEN_EOF)) {

			np_Parser_consume_tokens(self,1);
			return 1;

		} else {
			{
				// this message doesn't bring the line number and cursor position
				Print_cstr(&self->log, "[parser error] Semi-colon or end-of-file expected.\n");
				np_Parser_log_context(self);
			}
			return 0;
		}

// 		// semantic action: make symbols table include id etc
// 		if (!np_Parser_compare_next(self, np_TOKEN_SEMICOLON)) { return 0; }
// 		np_Parser_consume_tokens(self,1);
// 		if (!np_Parser_S(self)) return 0;
// 		return 1;

	}
}

internal void
np_Parser_normalize_statements(np_Parser *self)
{
	np_AST_Node* prev = 0;
	np_AST_Node* it   = self->ast_first;
	while (it) {
		np_AST_Node *next = it->next;

		it->next = 0;

		it = np_ast_normalize(it);

		if (!prev) self->ast_first = it;
		else prev->next = it;

		prev = it;
		it   = next;
	}
	self->ast_last = prev;
}

internal b8
np_Parser_run(np_Parser *self)
{
	np_Parser_fill_buffer(self);
	b8 result = np_Parser_S(self);
	np_Parser_normalize_statements(self);
	return result;
}

//------------------------------------------------------------------------------
// Type and TypeTable
//------------------------------------------------------------------------------

internal np_Type*
np_TypeTable_insert(np_TypeTable *self, char* name_begin, char* name_end, BilinearAllocator *memory)
{
	Assert(self->end != self->capacity);

	np_TypeID id = (np_TypeID) (self->end - self->begin);

	np_Type* newtype = self->end;
	++self->end;

	u32 name_len = (u32) (name_end - name_begin);

	newtype->id = id;
	newtype->name.begin = (char*) BilinearAllocator_alloc_left(memory, name_len);
	newtype->name.end   = newtype->name.begin + name_len;

	pt_copy_bytes(name_begin, name_end, newtype->name.begin,
		      newtype->name.end);

	return newtype;
}

internal np_Type*
np_TypeTable_find(np_TypeTable *self, np_TypeID type_id)
{
	np_Type *it = self->begin;
	while (it != self->end)
	{
		if (it->id == type_id)
		{
			return it;
		}
		++it;
	}
	return 0;
}

internal void
np_TypeTable_init(np_TypeTable *self, np_Type *begin, np_Type *capacity)
{
	self->begin    = begin;
	self->end      = begin;
	self->capacity = capacity;
}



//------------------------------------------------------------------------------
// Symbol and Symbol Table
//------------------------------------------------------------------------------

internal void
np_SymbolTable_init(np_SymbolTable *self, np_Symbol *begin, np_Symbol *capacity)
{
	// reserve symbol capacity slots for the symbols array
	self->begin    = begin;
	self->end      = begin;
	self->capacity = capacity;
}

internal np_Symbol*
np_SymbolTable_insert_variable(np_SymbolTable *self, char *name_begin,
			       char *name_end, np_TypeID type_id, void *value,
			       BilinearAllocator *memory)
{
	Assert(self->end != self->capacity);

	np_SymbolID id = (np_SymbolID)
		(self->end - self->begin);
	np_Symbol *symbol = self->end;
	++self->end;

	// clear symbol memory
	for (int i=0;i<sizeof(np_Symbol);++i) {
		*((char*) symbol + i) = 0;
	}

	u32 name_len = (u32) (name_end - name_begin);

	symbol->id = id;
	symbol->name.begin = (char*) BilinearAllocator_alloc_left(memory, name_len);
	symbol->name.end   = symbol->name.begin + name_len;

	// copy name
	pt_copy_bytes(name_begin, name_end, symbol->name.begin, symbol->name.end);

	symbol->is_variable = 1;
	symbol->variable.type_id = type_id;
	symbol->variable.value   = value;

	return symbol;
}

internal np_Symbol*
np_SymbolTable_insert_function(np_SymbolTable *self,
			       char         *name_begin,
			       char         *name_end,
			       np_TypeID    return_type,
			       np_TypeID    *param_type_begin,
			       np_TypeID    *param_type_end,
			       b8           var_params,
			       np_TypeID    var_params_type,
			       FunctionSymbolPtr function_impl,
			       BilinearAllocator *memory)
{
	Assert(self->end != self->capacity);

	s64 num_params = param_type_end - param_type_begin;

	np_SymbolID id = (np_SymbolID) (self->end - self->begin);
	np_Symbol *symbol = self->end;
	++self->end;

	// clear symbol memory
	for (int i=0;i<sizeof(np_Symbol);++i) {
		*((char*) symbol + i) = 0;
	}

	u32 name_len = (u32) (name_end - name_begin);

	symbol->id = id;
	symbol->name.begin = (char*) BilinearAllocator_alloc_left(memory, name_len);
	symbol->name.end   = symbol->name.begin + name_len;

	// copy name
	pt_copy_bytes(name_begin, name_end,
		      symbol->name.begin, symbol->name.end);

	symbol->is_function = 1;
	symbol->function.return_type = return_type;

	np_TypeID *input_types = (np_TypeID*) BilinearAllocator_alloc_left(memory, num_params * sizeof(np_TypeID));
	for (int i=0;i<num_params;++i)
		*(input_types + i) = *(param_type_begin + i);

	symbol->function.param_type_begin = input_types;
	symbol->function.param_type_end   = input_types + num_params;
	symbol->function.var_params       = var_params;
	symbol->function.var_params_type  = var_params_type;

	symbol->function.ptr = function_impl;

	return symbol;

}

internal np_Symbol*
np_SymbolTable_find_function(np_SymbolTable *self,
			     char *name_begin,
			     char *name_end,
			     np_TypeID *param_type_begin,
			     np_TypeID *param_type_end)
{
	s64 num_input_params = param_type_end - param_type_begin;

	np_Symbol *symbol = self->begin;
	while (symbol != self->end)
	{
		if (symbol->is_function)
		{
			b8  same_name         = pt_compare_memory(name_begin, name_end, symbol->name.begin, symbol->name.end) == 0;
			s64 num_symbol_params = symbol->function.param_type_end - symbol->function.param_type_begin;

			if (same_name && num_symbol_params <= num_input_params) {

				b8 match = 1;

				if (num_symbol_params < num_input_params)
				{
					if (symbol->function.var_params) {
						// check if extra params match var params type
						np_TypeID *it_input  = param_type_begin + num_symbol_params;
						while (it_input != param_type_end) {
							if (*it_input != symbol->function.var_params_type)
							{
								match = 0;
								break;
							}
							++it_input;
						}
					}
					else { match = 0; }
				}

				if (match) {
					np_TypeID *it_symbol = symbol->function.param_type_begin;
					np_TypeID *it_symbol_end = symbol->function.param_type_end;
					np_TypeID *it_input  = param_type_begin;
					while (it_symbol != it_symbol_end)
					{
						if (*it_input != *it_symbol)
						{
							match = 0;
							break;
						}
						++it_input;
						++it_symbol;
					}
				}
				if (match)
					return symbol;
			}
		}
		++symbol;
	}

	return 0;
}

internal np_Symbol*
np_SymbolTable_find_variable(np_SymbolTable *self,
			     char *name_begin,
			     char *name_end)
{
	np_Symbol *symbol = self->begin;
	while (symbol != self->end) {
		if (symbol->is_variable &&
		    pt_compare_memory(name_begin,
				      name_end,
				      symbol->name.begin,
				      symbol->name.end) == 0) {
			return symbol;
		}
		++symbol;
	}
	return 0;
}

//------------------------------------------------------------------------------
// Compiler
//------------------------------------------------------------------------------

internal np_TypeValue
np_TypeValue_undefined()
{
	np_TypeValue error;
	error.value = 0;
	error.type_id = np_TYPE_UNDEFINED;
	error.readonly = 0;
	error.error = 0;
	return error;
}

internal np_TypeValue
np_TypeValue_error()
{
	np_TypeValue error;
	error.value = 0;
	error.type_id = 0;
	error.readonly = 0;
	error.error = 1;
	return error;
}

internal np_TypeValue
np_TypeValue_value(np_TypeID type_id, void *value)
{
	np_TypeValue result;
	result.type_id = type_id;
	result.value = value;
	result.error = 0;
	result.readonly = 0;
	return result;
}

internal np_TypeValue
np_TypeValue_readonly_value(np_TypeID type_id, void *value)
{
	np_TypeValue result;
	result.type_id = type_id;
	result.value = value;
	result.error = 0;
	result.readonly = 1;
	return result;
}

//------------------------------------------------------------------------------
// Compiler
//------------------------------------------------------------------------------

internal void
np_Compiler_clear_error_log(np_Compiler *self)
{
	Print_clear(&self->reduce.log);
}

internal np_Type*
np_Compiler_insert_type(np_Compiler *self, char *name_begin, char *name_end)
{
	return np_TypeTable_insert(&self->type_table, name_begin, name_end, self->memory);
}

internal np_Type*
np_Compiler_insert_type_cstr(np_Compiler *self, char *name_cstr)
{
	return np_TypeTable_insert(&self->type_table, name_cstr, cstr_end(name_cstr), self->memory);
}

internal np_Symbol*
np_Compiler_insert_function(np_Compiler *self,
			    char         *name_begin,
			    char         *name_end,
			    np_TypeID    return_type,
			    np_TypeID    *param_type_begin,
			    np_TypeID    *param_type_end,
			    b8           var_params,
			    np_TypeID    var_params_type,
			    FunctionSymbolPtr function_impl)
{
	return np_SymbolTable_insert_function
		(&self->symbol_table, name_begin, name_end,
		 return_type, param_type_begin, param_type_end,
		 var_params, var_params_type, function_impl,
		 self->memory);
}

internal np_Symbol*
np_Compiler_insert_function_cstr(np_Compiler *self,
			    char         *name_cstr,
			    np_TypeID    return_type,
			    np_TypeID    *param_type_begin,
			    np_TypeID    *param_type_end,
			    b8           var_params,
			    np_TypeID    var_params_type,
			    FunctionSymbolPtr function_impl)
{
	return np_SymbolTable_insert_function
		(&self->symbol_table, name_cstr, cstr_end(name_cstr),
		 return_type, param_type_begin, param_type_end,
		 var_params, var_params_type, function_impl,
		 self->memory);
}


internal np_Symbol*
np_Compiler_insert_variable(np_Compiler *self, char *name_begin,
			    char *name_end, np_TypeID type_id, void *value)
{
	return np_SymbolTable_insert_variable(&self->symbol_table, name_begin,
					      name_end, type_id, value,
					      self->memory);
}

internal np_Symbol*
np_Compiler_insert_variable_cstr(np_Compiler *self, char *name_cstr,
				 np_TypeID type_id, void *value)
{
	return np_SymbolTable_insert_variable(&self->symbol_table, name_cstr,
					      cstr_end(name_cstr), type_id,
					      value, self->memory);
}

internal char*
np_Compiler_alloc(np_Compiler *self, u64 num_bytes)
{
	return BilinearAllocator_alloc_left(self->memory, num_bytes);
}

internal MemoryBlock
np_Compiler_free_memblock(np_Compiler *self)
{
	return BilinearAllocator_free_memblock(self->memory);
}

internal np_TypeValue
np_function_add_numbers(np_Compiler *compiler, np_TypeValue* params_begin,
			np_TypeValue *params_end)
{
	Assert(params_end - params_begin == 2);
	np_TypeValue *left  = params_begin;
	np_TypeValue *right = params_begin + 1;

	f64 *val = (f64*) np_Compiler_alloc(compiler, sizeof(f64));

	*val = *((f64*) left->value) + *((f64*) right->value);

	return np_TypeValue_value( compiler->number_type_id, val );
}

internal np_TypeValue
np_function_mul_numbers(np_Compiler *compiler, np_TypeValue* params_begin,
			np_TypeValue *params_end)
{
	Assert(params_end - params_begin == 2);
	np_TypeValue *left  = params_begin;
	np_TypeValue *right = params_begin + 1;

	f64 *val = (f64*) np_Compiler_alloc(compiler, sizeof(f64));

	*val = *((f64*) left->value) * *((f64*) right->value);

	return np_TypeValue_value( compiler->number_type_id, val );
}

internal np_TypeValue
np_function_div_numbers(np_Compiler *compiler, np_TypeValue* params_begin,
			np_TypeValue *params_end)
{
	Assert(params_end - params_begin == 2);
	np_TypeValue *left  = params_begin;
	np_TypeValue *right = params_begin + 1;

	f64 *val = (f64*) np_Compiler_alloc(compiler, sizeof(f64));

	*val = *((f64*) left->value) / *((f64*) right->value);

	return np_TypeValue_value( compiler->number_type_id, val );
}

internal np_TypeValue
np_function_sub_numbers(np_Compiler *compiler, np_TypeValue* params_begin,
			np_TypeValue *params_end)
{
	Assert(params_end - params_begin == 2);
	np_TypeValue *left  = params_begin;
	np_TypeValue *right = params_begin + 1;

	f64 *val = (f64*) np_Compiler_alloc(compiler, sizeof(f64));

	*val = *((f64*) left->value) - *((f64*) right->value);

	return np_TypeValue_value( compiler->number_type_id, val );
}


internal void
np_Compiler_init(np_Compiler *self, BilinearAllocator *memory)
{

	self->memory = memory;

	// initialize reduce status and log area
	self->reduce.success = 0;
	Print_init(&self->reduce.log,
		   self->reduce.log_buffer,
		   self->reduce.log_buffer + sizeof(self->reduce.log_buffer));

	/* hard coded capacity for 16 types */
	const u32 type_capacity = 16;
	np_Type *type_begin    = (np_Type*) BilinearAllocator_alloc_left(memory, sizeof(np_Type) * type_capacity);
	np_Type *type_end = type_begin + type_capacity;
	np_TypeTable_init(&self->type_table, type_begin, type_end); // share the same memory

	/* hard coded symbols capacity for 128 */
	const u32 symbol_capacity = 128;
	np_Symbol *symbol_begin    = (np_Symbol*) BilinearAllocator_alloc_left(memory, sizeof(np_Symbol) * symbol_capacity);
	np_Symbol *symbol_end = symbol_begin + symbol_capacity;
	np_SymbolTable_init(&self->symbol_table, symbol_begin, symbol_end);

	// type 0 is special: it is Undefined
	self->undefined_type_id = np_Compiler_insert_type_cstr(self, "Undefined")->id;
	self->number_type_id    = np_Compiler_insert_type_cstr(self, "Number")->id;
	self->string_type_id    = np_Compiler_insert_type_cstr(self, "String")->id;

	Assert(np_TYPE_UNDEFINED == self->undefined_type_id);
	Assert(np_TYPE_NUMBER    == self->number_type_id);
	Assert(np_TYPE_STRING    == self->string_type_id);

	// and operators
	np_TypeID parameter_types[2];
	parameter_types[0] = self->number_type_id;
	parameter_types[1] = self->number_type_id;
	np_Compiler_insert_function_cstr
		(self, "+", self->number_type_id,
		 parameter_types, parameter_types + 2, 0, 0,
		 np_function_add_numbers);
	np_Compiler_insert_function_cstr
		(self, "*", self->number_type_id,
		 parameter_types, parameter_types + 2, 0, 0,
		 np_function_mul_numbers);
	np_Compiler_insert_function_cstr
		(self, "-", self->number_type_id,
		 parameter_types, parameter_types + 2, 0, 0,
		 np_function_sub_numbers);
	np_Compiler_insert_function_cstr
		(self, "/", self->number_type_id,
		 parameter_types, parameter_types + 2, 0, 0,
		 np_function_div_numbers);
}

internal np_CompilerCheckpoint
np_Compiler_checkpoint(np_Compiler *self)
{
	np_CompilerCheckpoint chkpt;
	chkpt.left_checkpoint  = BilinearAllocator_left_checkpoint(self->memory);
	chkpt.right_checkpoint = BilinearAllocator_right_checkpoint(self->memory);
	chkpt.num_symbols = self->symbol_table.end - self->symbol_table.begin;
	chkpt.num_types = self->type_table.end - self->type_table.begin;
	return chkpt;
}

internal void
np_Compiler_goto_checkpoint(np_Compiler *self, np_CompilerCheckpoint chkpt)
{
// 	Assert(self->memory->begin <= chkpt.memory_checkpoint
// 	       && self->memory->end <= chkpt.memory_checkpoint);
// 	Assert(chkpt.num_symbols
// 	       <= self->symbol_table.end - self->symbol_table.begin);
	Assert(chkpt.num_types <= self->type_table.end - self->type_table.begin);

	/* rewind types, symbols and memory */
	BilinearAllocator_rewind(self->memory, chkpt.left_checkpoint);
	BilinearAllocator_rewind(self->memory, chkpt.right_checkpoint);
	self->type_table.end = self->type_table.begin + chkpt.num_types;
	self->symbol_table.end = self->symbol_table.begin + chkpt.num_symbols;
}

internal void
np_Compiler_log_function_not_found_error(
					 np_Compiler    *self,
					 char           *name_begin,
					 char           *name_end,
					 np_TypeID      *param_begin,
					 np_TypeID      *param_end)
{
	Print *print = &self->reduce.log;

	Print_cstr(print,"[Compiler Reduce Error]\n");
	Print_cstr(print,"Couldn't find function named '");
	Print_str(print, name_begin, name_end);
	Print_cstr(print,"'\n");

	s64 nparams = param_end - param_begin;
	if (nparams > 0)
	{
		Print_cstr(print,"with parameter type");

		if (nparams > 1)
		{
			Print_cstr(print,"s");
		}

		Print_cstr(print,":\n");

		for (u32 i=0;i<nparams;++i) {
			Print_u64(print, (i+1));
			Print_cstr(print,". type_id: ");
			Print_u64(print,*(param_begin + i));
			Print_align(print,2,1,' ');
			Print_cstr(print,"   name: ");

			np_Type *type = np_TypeTable_find(&self->type_table, *(param_begin + i));

			Assert(type);

			Print_str(print, type->name.begin, type->name.end);
			Print_cstr(print,"\n");
		}
	}
}

internal void
np_Compiler_log_variable_not_found_error(
					 np_Compiler    *self,
					 char           *name_begin,
					 char           *name_end)
{
	Print *print = &self->reduce.log;
	Print_cstr(print,"[Compiler Reduce Error]\n");
	Print_cstr(print,"Couldn't find variable named '");
	Print_str(print, name_begin, name_end);
	Print_cstr(print,"'\n");
}

internal void
np_Compiler_log_custom_error(
			     np_Compiler    *self,
			     char           *error_begin,
			     char           *error_end)
{
	Print *print = &self->reduce.log;
	Print_cstr(print,"[Compiler Reduce Error]\n");
	Print_str(print, error_begin, error_end);
}


internal void
np_Compiler_log_ast_node_context(np_Compiler *self)
{
	np_AST_Node *node = self->reduce.current_context;
	Assert(node);

	char *begin = self->reduce.text_begin;
	char *end   = self->reduce.text_end;

	Assert(begin <= node->begin && node->end <= end);

	char *context_begin;
	char *context_end;

	{
		char *it = node->begin;
		while (it != begin && *it != '\n')
		{
			--it;
		}
		if (*it == '\n') ++it;
		context_begin = it;
	}

	{
		char *it = node->begin;
		while (it != end && *it != '\n')
		{
			++it;
		}
		context_end = it;
	}

	Print *print = &self->reduce.log;

	Print_cstr(print,"[Context]\n");

	Print_str(print, context_begin, context_end);
	Print_cstr(print,"\n");
	Print_cstr(print,"^");
	Print_align(print,node->begin - context_begin + 1,1,' ');
	Print_cstr(print,"\n");
}

#define np_Compiler_max_function_parameters 128

internal np_TypeValue
np_Compiler_reduce_node(np_Compiler *self, np_AST_Node* node)
{
	if (node->type == np_AST_Node_Type_Number) {

		np_AST_Number* ast_number = (np_AST_Number*) node->detail;

		f64 *val = (f64*) np_Compiler_alloc(self, sizeof(f64));

		*val = (ast_number->is_integer) ? (f64) ast_number->ip : ast_number->fp;

		return np_TypeValue_value(self->number_type_id, val);

	} else if (node->type == np_AST_Node_Type_Binary_Operation) {

		np_AST_Binary_Operation* ast_binop = (np_AST_Binary_Operation*) node->detail;

		np_TypeValue left_value  = np_Compiler_reduce_node(self, ast_binop->left);
		np_TypeValue right_value = np_Compiler_reduce_node(self, ast_binop->right);

		if (left_value.error || right_value.error)
			return np_TypeValue_error();

		self->reduce.current_context = node;

		// check if there is a compatible function
		// in the symbols table
		np_TypeID types[2];
		types[0] = left_value.type_id;
		types[1] = right_value.type_id;

		np_Symbol *symbol = np_SymbolTable_find_function(&self->symbol_table, ast_binop->name.begin, ast_binop->name.end,
								 types, types+2);

		if (symbol) {
			np_TypeValue params[2];
			params[0] = left_value;
			params[1] = right_value;
			return (*symbol->function.ptr)(self, params, params+2);
		} else {
			np_Compiler_log_function_not_found_error(self,
								 ast_binop->name.begin,
								 ast_binop->name.end,
								 types,
								 types+2);
			np_Compiler_log_ast_node_context(self);
			return np_TypeValue_error();
		}
	}
	else if (node->type == np_AST_Node_Type_Function) {
		np_AST_Function* ast_function = (np_AST_Function*) node->detail;

		/* NOTE(llins) inefficient two pass over linked list  */
		u32 num_params = 0;
		np_AST_Function_Parameter* it = ast_function->first_parameter;
		while (it) {
			++num_params;
			it = it->next;
		}

		// allocate arra
		np_TypeID    *types = 0;
		np_TypeValue *params = 0;
		BilinearAllocatorCheckpoint chkpt = { .checkpoint = 0, .left = 0 };
		if (num_params > 0) {
			if (BilinearAllocator_free_space(self->memory) < (sizeof(np_TypeID) + sizeof(np_TypeValue)) * num_params) {
				static char *error = "Not enough memory to reduce function call.";
				np_Compiler_log_custom_error(self,error, cstr_end(error));
				// np_Compiler_log_ast_node_context(self);
				return np_TypeValue_error();
			}
			chkpt  = BilinearAllocator_right_checkpoint(self->memory);
			types  = (np_TypeID*)    BilinearAllocator_alloc_right(self->memory, sizeof(np_TypeID)    * num_params);
			params = (np_TypeValue*) BilinearAllocator_alloc_right(self->memory, sizeof(np_TypeValue) * num_params);
		}

		it = ast_function->first_parameter;
		u32 index = 0;
		while (it) {
			np_TypeValue tv = np_Compiler_reduce_node(self, it->node);
			if (tv.error)
				return np_TypeValue_error();
			types[index]  = tv.type_id;
			params[index] = tv;
			++index;
			it = it->next;
		}

		self->reduce.current_context = node;

		np_Symbol *symbol = np_SymbolTable_find_function(&self->symbol_table, ast_function->name.begin, ast_function->name.end,
								 types, types+num_params);

		np_TypeValue result;
		if (symbol) {
			result = (*symbol->function.ptr)(self, params, params+num_params);
		} else {
			np_Compiler_log_function_not_found_error(self, ast_function->name.begin, ast_function->name.end,
								 types, types+num_params);
			np_Compiler_log_ast_node_context(self);
			result = np_TypeValue_error();
		}

		if (types) {
			BilinearAllocator_rewind(self->memory, chkpt);
		}
		return result;

	} else if (node->type == np_AST_Node_Type_Variable) {
		np_AST_Variable* ast_variable = (np_AST_Variable*) node->detail;

		self->reduce.current_context = node;

		np_Symbol *symbol = np_SymbolTable_find_variable
			(&self->symbol_table, ast_variable->name.begin,
			 ast_variable->name.end);

		if (symbol) {
			np_TypeValue result;
			result.type_id  = symbol->variable.type_id;
			result.value    = symbol->variable.value;
			result.readonly = 1;
			result.error    = 0;
			return result;
		} else {
			np_Compiler_log_variable_not_found_error
				(self, ast_variable->name.begin,
				 ast_variable->name.end);
			np_Compiler_log_ast_node_context(self);
			return np_TypeValue_error();
		}
	} else if (node->type == np_AST_Node_Type_String) {
		np_AST_String* ast_string = (np_AST_String*) node->detail;

		// allocate memory for the string
		u32 len = (u32) (ast_string->str.end - ast_string->str.begin);
		char *st_copy_begin = (char*) np_Compiler_alloc(self, len);
		char *st_copy_end   = st_copy_begin + len;
		pt_copy_bytes(ast_string->str.begin, ast_string->str.end,
			      st_copy_begin, st_copy_end);

		// string objects will be memory blocks
		MemoryBlock *mb = (MemoryBlock*)
			np_Compiler_alloc(self, sizeof(MemoryBlock));
		mb->begin = st_copy_begin;
		mb->end   = st_copy_end;

		np_TypeValue result;
		result.type_id  = self->string_type_id;
		result.value    = mb;
		result.readonly = 0;
		result.error    = 0;
		return result;

	} else if (node->type == np_AST_Node_Type_Assignment) {

		// check if variable already exists
		np_AST_Assignment* ast_assignment = (np_AST_Assignment*) node->detail;

		np_Symbol *symbol = np_SymbolTable_find_variable(
								 &self->symbol_table,
								 ast_assignment->name.begin,
								 ast_assignment->name.end);

		if (symbol) { return np_TypeValue_error(); } // variable already exists

		np_TypeValue result = np_Compiler_reduce_node(self, ast_assignment->node);

		if (result.error) {
			return np_TypeValue_error();
		}

		np_SymbolTable_insert_variable(&self->symbol_table,
					       ast_assignment->name.begin,
					       ast_assignment->name.end,
					       result.type_id,
					       result.value,
					       self->memory);

		return result;
	} else if (node->type == np_AST_Node_Type_Group) {

		np_AST_Group* ast_group = (np_AST_Group*) node->detail;
		return np_Compiler_reduce_node(self, ast_group->node);

	} else {
		return np_TypeValue_error();
	}
}


internal np_TypeValue
np_Compiler_reduce(np_Compiler *self, np_AST_Node* node, char *text_begin, char *text_end)
{
	Print_clear(&self->reduce.log);

	// input context (assumes node was extracted from this text)
	self->reduce.text_begin = text_begin;
	self->reduce.text_end   = text_end;

	self->reduce.statement_results = 0;

	np_TypeValueList *latest_result = 0;

	np_AST_Node* it = node;
	np_TypeValue result = np_TypeValue_undefined();
	while (it)
	{
		result = np_Compiler_reduce_node(self, it);

		if (result.error) {
			self->reduce.success = 0;
			return result;
		}

		if (latest_result == 0) {
			latest_result = (np_TypeValueList*) np_Compiler_alloc(self, sizeof(np_TypeValueList));
			latest_result->data = result;
			latest_result->next = 0;
			self->reduce.statement_results = latest_result;
		} else {
			latest_result->next = (np_TypeValueList*) np_Compiler_alloc(self, sizeof(np_TypeValueList));
			latest_result->next->data = result;
			latest_result->next->next = 0;
			latest_result = latest_result->next;
		}

		it = it->next;
	}

	self->reduce.success = !result.error;

	return result;
}


