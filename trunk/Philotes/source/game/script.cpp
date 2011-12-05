//----------------------------------------------------------------------------
// script.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "excel_private.h"
#include "filepaths.h"
#include "script.h"
#include "script_priv.h"
#include "units.h"
#include "stats.h"
#include "prime.h"
#include "pakfile.h"
#include "skills.h"
#include "crc.h"
#if ISVERSION(SERVER_VERSION)
#include "script.cpp.tmh"
#endif


//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
BOOL sgbScriptExecuteTrace = FALSE;
BOOL sgbScriptParseTrace = FALSE;
BOOL sgbScriptSymbolTrace = FALSE;


//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
#define STRING_TABLE_HASH_SIZE				256
#define SYMBOL_TABLE_HASH_SIZE				256
#define SVALUE_STACK_SIZE					64

#define BYTE_STREAM_BUFFER_SIZE				32
#define PARSE_STACK_BUFFER_SIZE				16

#define PARSEFLAG_BOF						0x00000001
#define PARSEFLAG_BOL						0x00000002
#define PARSEFLAG_EOL						0x00000004

#define DEFAULT_VM_GLOBAL_SIZE				4096
#define DEFAULT_VM_LOCAL_SIZE				4096
#define DEFAULT_VM_STACK_SIZE				512

#define MAX_FUNCTION_PARAMS					64

#define SCRIPT_MAGIC						'hscr'
#define SCRIPT_VERSION						(3 + GLOBAL_FILE_VERSION)
#define SCRIPT_FILENAME						"script.bin"

#define SCRIPT_STRING_MAGIC					'hsst'
#define SCRIPT_STRING_VERSION				(1 + GLOBAL_FILE_VERSION)

#define SCRIPT_SYMBOL_TABLE_MAGIC			'hsyt'
#define SCRIPT_SYMBOL_TABLE_VERSION			(1 + GLOBAL_FILE_VERSION)

#define SCRIPT_SYMBOL_MAGIC					'hsym'
#define SCRIPT_SYMBOL_VERSION				(2 + GLOBAL_FILE_VERSION)
#define SCRIPT_SYMBOL_END_MAGIC				'hend'


//----------------------------------------------------------------------------
// ERROR MACROS
//----------------------------------------------------------------------------
#define SASSERT(exp, str)					{ if (!(exp)) { ScriptParseError(parse, str); } }
#define SASSERT_RETFALSE(exp, str)			{ if (!(exp)) { ScriptParseError(parse, str); return FALSE;} }
#define SASSERT_RETNULL(exp, str)			{ if (!(exp)) { ScriptParseError(parse, str); return NULL;} }
#define SASSERT_RETZERO(exp, str)			{ if (!(exp)) { ScriptParseError(parse, str); return 0;} }


//----------------------------------------------------------------------------
#define SYM_HASH_KEY(key)					((key) % SYMBOL_TABLE_HASH_SIZE)


#define SCRIPT_ERROR(str)					{ trace("%s\n", str); return FALSE; }
#define RETURN_IF_FALSE(x)					if (!(x)) return FALSE;
#define BREAK_IF_FALSE(x)					if (!(x)) break;

#define NEXT()								{ if (!GetToken(parse)) return FALSE; }
#define SKIP(t)								{ if (!(parse.token.tok == t)) { ScriptParseError(parse, "EXPECTED: " #t); return FALSE; } NEXT(); }
#define PARSE_SKIP(t)						{ GetToken(parse); SKIP(t); }


//----------------------------------------------------------------------------
enum VMI
{
#define VMI_DEF_SING(x)						x
#define VMI_DEF_TYIN(x)						x, x##_CH = x, x##_UC, x##_SH, x##_US, x##_IN, x##_UI, x##_LL, x##_UL
#define VMI_DEF_TYNP(x)						x, x##_CH = x, x##_UC, x##_SH, x##_US, x##_IN, x##_UI, x##_LL, x##_UL, x##_FL, x##_DB, x##_LD
#define VMI_DEF_TYPE(x)						x, x##_CH = x, x##_UC, x##_SH, x##_US, x##_IN, x##_UI, x##_LL, x##_UL, x##_FL, x##_DB, x##_LD, x##_POINTER
#include "vmi.h"
};


enum ETOKEN
{
#define DEF(id, str, prec, assoc, op, intonly, assign, vmi)	\
											id,
  #include "tokens.h"
#undef DEF

	TOK_LAST,

#define DEF(id, str)						id,
  #include "keywords.h"
#undef DEF

	TOK_LASTKEYWORD,
};


enum
{
#define DEF(id, str, vmi)					id,
  #include "basictypes.h"
#undef DEF
	TYPE_BASICTYPE =		0x1f,			// mask for basic type
	TYPE_ARRAY =			0x20,			// array flag (also has TYPE_PTR)
	TYPE_CONSTANT =			0x40,			// const
	TYPE_BITFIELD =			0x80,			// bitfield flag
};

enum EIPARAM_TYPE
{
	PARAM_NORMAL,							// normal passed parameter
	PARAM_INDEX,							// table index
	PARAM_CONTEXT,							// ptr to entire script context
	PARAM_CONTEXT_VMSTATE,					// vmstate
	PARAM_CONTEXT_GAME,						// game
	PARAM_CONTEXT_UNIT,						// unit
	PARAM_CONTEXT_STATS,					// statslist
	PARAM_STAT_PARAM,						// param for a stat, determined by stat
};

enum
{
	STAT_SOURCE_NONE,						// 0
	STAT_SOURCE_STATS,						// 1
	STAT_SOURCE_UNIT,						// 2
	STAT_SOURCE_OBJECT,						// 3
	STAT_SOURCE_STACK_UNIT,					// 4
	STAT_SOURCE_RIDER,						// 5
};

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------
typedef void (*FP_PARSE_EXPR)(struct PARSE & parse);


//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------
// file we're parsing
struct PARSE_FILE
{
	DWORD					flags;
	const char *			filename;
	const char *			buf;				// buffer we're parsing
	const char *			cur;				// current position in buffer
	unsigned int			line;				// line number of current position
	BOOL					free_buf;			// free the buffer?
	PARSE_FILE *			prev;				// link to previous file on stack
	int						if_level;			// recurse level for #if, #ifdef, #ifndef, etc
};


// table definition of token
struct TOKEN_DEF
{
	char *					str;				// string for token
	int						precedence;			// precedence 0 = highest
	int						associativity;		// 0 = Left-to-Right, 1 = Right-to-Left
	int						operandcount;		// number of operands
	int						intop;				// operator is integer only
	int						assign;				// operand assigns (expect l-value for op1)
	int						vmi;				// base instruction (_CH or non-typed)
};


// table definition of type
struct TYPE_DEF
{
	char *					str;				// string for type
	int						vmi_offset;			// offset from base vmi
};


// entry in string hash
struct STRING_NODE						
{
	char *					str;				// string
	int						len;				// length of string
	DWORD					hash;				// hash value of string
	STRING_NODE *			next;				// next in hash
};


// string table
struct STRING_TABLE2
{
	STRING_NODE *			hash[STRING_TABLE_HASH_SIZE];
	unsigned int			count;
};


// type
struct TYPE
{
	int						type;				// type enumeration
	struct SSYMBOL *		sym;				// symbol defining the type
};


// token
struct TOKEN
{
	TYPE					type;
	ETOKEN					tok;				// token enum
	VALUE					value;
	int						index;				// count from 0
};


// parse node
struct PARSENODE
{
	TOKEN					token;
	PARSENODE *				next;				// next node in stack or list
	PARSENODE *				parent;
	int						childcount;
	PARSENODE **			children;			// operands if tok is non-terminal

	union
	{
		int					localstack;			// local stack size of block
		int					offset;				// offset into array or structure
		struct
		{
			int				paramcount;			// function parameter count 
			int				curparam;			// current param being parsed (either for a function call or for a stat's param list
		};
	};
};


// parse stack
struct PARSESTACK
{
	PARSENODE *				top;
	int						count;
};


// parse list
struct PARSELIST
{
	PARSENODE *				first;
	PARSENODE *				last;
	int						count;
};


// function
struct FUNCTION
{
	int					index;				// index in function table
	SSYMBOL *			symbol;				// symbol
	SSYMBOL **			parameters;			// parameter list
	int					parameter_count;	// number of parameters
	int					min_param_count;	// minimum number of parameters (minus context parameters & default parameters)
	BOOL				varargs;			// variable number of arguments
	union
	{
		BYTE_STREAM *	stream;				// emitted code (or replacement in event of macro_func)
		void *			fptr;				// fptr for imported functions
	};
};


// symbol
struct SSYMBOL
{
	ETOKEN				tok;				// token enum
	TYPE				type;				// type (variables)
	char *				str;				// string
	DWORD				key;				// hash key made from str

	SSYMBOL *			prev;				// list order, for scoping
	SSYMBOL *			next;
	SSYMBOL *			hashprev;			// symbol hash
	SSYMBOL *			hashnext;

	int					block_level;		// block level at which this symbol was defined
	int					type_size;			// size of type for symbol

	union
	{
		BYTE_STREAM *	replacement;		// replacement stream for macros/defines
		FUNCTION *		function;			// function
	};
	union
	{
		int				index;				// index for stats or index of param
		int				value;				// value of enum
		int				offset;				// ptr to memory location for global or local variable
	};
	union
	{
		int				arraysize;			// size of array
		struct
		{
			SSYMBOL **	fields;				// fields in a struct
			int			fieldcount;			// number of fields in struct
		};
		struct
		{
			EIPARAM_TYPE	parameter_type;	// for imported function parameters, type of parameter
			int				index_table;	// table to do lookup in for index params
			int				index_index;	// index of table to use
		};
	};

};


// symbol table
struct SYMBOL_TABLE
{
	SSYMBOL *			first;				// first symbol in scope list
	SSYMBOL *			last;				// last symbol in scope list
	SSYMBOL *			hash[SYMBOL_TABLE_HASH_SIZE];
};


// macro expansion
struct MACRONODE
{
	SSYMBOL *			symbol;
	BYTE_STREAM			cursor;
	BYTE_STREAM *		params;
	int					paramcount;
};


struct MACROSTACK
{
	MACRONODE **		stack;				// stack of macros currently being expanded
	int					stacksize;
	int					stacktop;
};


// current parse state
struct PARSE
{
	DWORD				error;				// error enum
	int					errorlog;			// error log
	const char *		errorstr;			// error str
	const char *		text;				// original text being parsed
	const char *		curtext;			// current text position in .text
	const char *		lasttoken;			// last token read
	unsigned int		line;				// line #
	PARSE_FILE *		file;				// top file being parsed
	TOKEN				prev_token;			// previous token
	TOKEN				token;				// last token read
	char				tokstr[4096];		// temporary buffer for last token
	int					token_count;		// number of tokens processed so far
	BOOL				bConstWanted;		// constant desired from expression parser
	BOOL				bStopAtEOL;			// stop processing expression at EOL
	BOOL				bParamDeclWanted;	// param desired from decl parser
	BOOL				bParamWanted;		// param desired, stop on commas
	BOOL				bNoInitializer;		// disallow variable initialization after decl

	BOOL				bLastSymbolWasTerm;	// last symbol parsed was a terminator
	int					block_level;		// block level (0 = global)

	int					cur_local_offset;	// current offset into local memory space

	PARSESTACK			operatorstack;		// stack of operators in expression
	PARSESTACK			operandstack;		// stack of operands in expression
	PARSELIST			blocklist;			// fifo list of blocks in script

	MACROSTACK			macrostack;			// stack of macros currently being expanded

	struct VMSTATE *	vm;
};


// process global vm data -- this usually only changes during startup, though it might occur console command processing
struct GLOBALVM
{
	CCriticalSection	cs;

	unsigned int		globalcur;			// current offset into global memory space
	unsigned int		globalsize;			// allocated size

	SYMBOL_TABLE *		SymbolTable;		// symbol table

	FUNCTION **			FunctionTable;		// functions
	unsigned int		function_count;		// number of functions

	unsigned int		dwCrcImportFunctions;	// CRC for import functions

	STRING_TABLE2 *		StringTable;		// string table

	VMGAME *			vm;					// used for global parsing
};


//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
char isalnum_table[256];					// maps character to 0|1 if alnum
char isident_table[256];					// maps character to 0|1 if ident

// strings for basic types
TYPE_DEF BasicTypeDef[] =
{
#define DEF(id, str, vmi)					{ str, vmi },
  #include "basictypes.h"
#undef DEF
};

TYPE int_type = {TYPE_INT | TYPE_CONSTANT};

// strings for operator tokens
TOKEN_DEF TokenDef[] =
{
#define DEF(id, str, prec, assoc, op, intop, assign, vmi)	\
											{ str, prec, assoc, op, intop, assign, vmi },
  #include "tokens.h"
#undef DEF
};

// strings for keyword tokens
static char * KeywordStr[] = 
{
#define DEF(id, str)					str,
  #include "keywords.h"
#undef DEF
};

GLOBALVM		gVM;						// global VM (symbol, function, string tables)

struct EXTERN_TYPE_DEF				// externally defined pointer types
{
	const char *	str;
	int				type;
};

EXTERN_TYPE_DEF ExternTypeDef[] =
{
	{ "GAME",		TYPE_GAME	},
	{ "UNIT",		TYPE_UNIT	},
	{ "STATS",		TYPE_STATS	},
};

struct CONTEXT_DEF					// context pointers
{
	const char *	str;
	int				offset;
	int				statsource;
	int				type;
};

CONTEXT_DEF ContextDef[] =
{
	// variable			offset
	{ "unit",			OFFSET(SCRIPT_CONTEXT, unit),			2,	TYPE_UNIT		},
	{ "object",			OFFSET(SCRIPT_CONTEXT, object),			3,	TYPE_UNIT		},
	{ "source",			OFFSET(SCRIPT_CONTEXT, object),			3,	TYPE_UNIT		},
	{ "statslist",		OFFSET(SCRIPT_CONTEXT, stats),			1,	TYPE_STATS		},
	{ "skill",			OFFSET(SCRIPT_CONTEXT, nSkill),			0,	TYPE_INT		},
	{ "stateId",		OFFSET(SCRIPT_CONTEXT, nState),			0,	TYPE_INT		},
	{ "sklvl",			OFFSET(SCRIPT_CONTEXT, nSkillLevel),	0,	TYPE_INT		},
	{ "param1",			OFFSET(SCRIPT_CONTEXT, nParam1),		0,	TYPE_INT		},
	{ "param2",			OFFSET(SCRIPT_CONTEXT, nParam2),		0,	TYPE_INT		},
};


//----------------------------------------------------------------------------
// IMPORT FUNCTION HEADERS
//----------------------------------------------------------------------------
#include "imports.h"


//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------
BOOL GetTokenNoMacro(
	PARSE & parse,
	BOOL bSkipEOL = TRUE);

BOOL GetToken(
	PARSE & parse);

BOOL ParseType(
	PARSE & parse,
	TYPE & type,
	TOKEN & ident,
	SSYMBOL * structure);

PARSENODE * ParseExpr(
	PARSE & parse,
	int operatortop = 0,
	int operandtop = 0);

BOOL ParseDecl(
	PARSE & parse,
	PARSENODE * block,
	SSYMBOL * structure);

PARSENODE * ParseBlock(
	PARSE & parse);

PARSENODE * ParseBlockList(
	PARSE & parse,
	TOKEN * function = NULL);

BOOL EmitNode(
	PARSE & parse,
	BYTE_STREAM * stream,
	PARSENODE * node);


//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void ScriptExecTrace(
	const char * format,
	...)
{
#if !ISVERSION(RETAIL_VERSION)
	if (!sgbScriptExecuteTrace)
	{
		return;
	}

	va_list args;
	va_start(args, format);
	vtrace(format, args);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void ScriptParseTrace(
	const char * format,
	...)
{
#if !ISVERSION(RETAIL_VERSION)
	if (!sgbScriptParseTrace)
	{
		return;
	}

	va_list args;
	va_start(args, format);
	vtrace(format, args);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define SCRIPT_ASSERT_ON_PARSE_ERROR
static void ScriptParseError(
	PARSE & parse,
	const char * errorstr)
{
#if !ISVERSION(RETAIL_VERSION)
#ifdef SCRIPT_ASSERT_ON_PARSE_ERROR
	ASSERTV_MSG("SCRIPT PARSE ERROR (%s) %s   LINE: %d\n   TOKEN: %s   AT: %s", STR_ARG(errorstr), STR_ARG(parse.errorstr), parse.line, STR_ARG(parse.lasttoken), parse.curtext);
#endif
#else
	REF(parse);
	REF(errorstr);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline BOOL isalphanum(
	int c)
{
	return isalnum_table[c];
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline BOOL isidentchar(
	int c)
{
	return isident_table[c];
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline BOOL isRstat(
	const PARSENODE * p)
{
	return ( p->token.tok == TOK_RSTAT    || p->token.tok == TOK_RSTAT_BASE    || p->token.tok == TOK_RSTAT_MOD    ||
	         p->token.tok == TOK_RSTAT_US || p->token.tok == TOK_RSTAT_US_BASE || p->token.tok == TOK_RSTAT_US_MOD );
}


//----------------------------------------------------------------------------
// compare two strings, where length of str2 is given
//----------------------------------------------------------------------------
static inline int icstrncmp(
	const char * str1,
	const char * str2,
	int len2)
{
	const char * end2 = str2 + len2;
	while (str2 < end2)
	{
		if (*str2 > *str1)
		{
			return 1;
		}
		if (*str1 > *str2)
		{
			return -1;
		}
		str1++;
		str2++;
	}
	if (*str1 != 0)
	{
		return -1;
	}
	return 0;
}


//----------------------------------------------------------------------------
// TYPE
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline int TypeGetBasicType(
	const TYPE & type)
{
	return (type.type & TYPE_BASICTYPE);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline int TypeIsBasicType(
	const TYPE & type)
{
	return TypeGetBasicType(type) < TYPE_PTR;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void TypeSet(
	TYPE & type,
	int type_flags)
{
	type.type = type_flags;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline int TypeGetVMIOffset(
	const TYPE & type)
{
	int basictype = TypeGetBasicType(type);
	ASSERT(basictype > 0 && basictype < arrsize(BasicTypeDef));
	return BasicTypeDef[basictype].vmi_offset;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline int TypeGetSize(
	const TYPE & type)
{
	switch (TypeGetBasicType(type))
	{
	case VT_VOID:
		return 0;
	case TYPE_CHAR:
		return sizeof(char);
	case TYPE_BYTE:
		return sizeof(unsigned char);
	case TYPE_SHORT:
		return sizeof(short);
	case TYPE_WORD:
		return sizeof(unsigned short);
	case TYPE_INT:
		return sizeof(int);
	case TYPE_DWORD:
		return sizeof(unsigned int);
	case TYPE_INT64:
		return sizeof(INT64);
	case TYPE_QWORD:
		return sizeof(UINT64);
	case TYPE_FLOAT:
		return sizeof(float);
	case TYPE_DOUBLE:
		return sizeof(double);
	case TYPE_LDOUBLE:
		return sizeof(long double);
	case TYPE_PTR:
	case TYPE_GAME:
	case TYPE_UNIT:
	case TYPE_STATS:
		{
			if (type.type & TYPE_ARRAY)
			{
				ASSERT_RETZERO(0);
			}
			else
			{
				return sizeof (int * POINTER_64);
			}
		}
	case TYPE_ENUM:
		return sizeof(int);
	case TYPE_FUNC:
		ASSERT_RETZERO(0);
	case TYPE_STRUCT:
		ASSERT_RETZERO(0);
	case TYPE_BOOL:
		return sizeof(BOOL);
	default:
		ASSERT_RETZERO(0);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline int TypeGetDeclSize(
	const TYPE & type)
{
	switch (TypeGetBasicType(type))
	{
	case VT_VOID:
		return 0;
	case TYPE_CHAR:
		return sizeof(char);
	case TYPE_BYTE:
		return sizeof(unsigned char);
	case TYPE_SHORT:
		return sizeof(short);
	case TYPE_WORD:
		return sizeof(unsigned short);
	case TYPE_INT:
		return sizeof(int);
	case TYPE_DWORD:
		return sizeof(unsigned int);
	case TYPE_INT64:
		return sizeof(INT64);
	case TYPE_QWORD:
		return sizeof(UINT64);
	case TYPE_FLOAT:
		return sizeof(float);
	case TYPE_DOUBLE:
		return sizeof(double);
	case TYPE_LDOUBLE:
		return sizeof(long double);
	case TYPE_PTR:
	case TYPE_GAME:
	case TYPE_UNIT:
	case TYPE_STATS:
		{
			if (type.type & TYPE_ARRAY)
			{
				ASSERT_RETZERO(type.sym);
				return type.sym->type_size;
			}
			else
			{
				return sizeof (int * POINTER_64);
			}
		}
	case TYPE_ENUM:
		return sizeof(int);
	case TYPE_FUNC:
		ASSERT_RETZERO(0);
	case TYPE_STRUCT:
		{
			ASSERT_RETZERO(type.sym);
			return type.sym->type_size;
		}
	case TYPE_BOOL:
		return sizeof(BOOL);
	default:
		ASSERT_RETZERO(0);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL TypeSetUnsigned(
	TYPE & type)
{
	switch (TypeGetBasicType(type))
	{
	case TYPE_CHAR:		type.type &= ~TYPE_CHAR;	type.type |= TYPE_BYTE;		break;
	case TYPE_BYTE:		break;
	case TYPE_SHORT:	type.type &= ~TYPE_SHORT;	type.type |= TYPE_WORD;		break;
	case TYPE_WORD:		break;
	case TYPE_INT:		type.type &= ~TYPE_INT;		type.type |= TYPE_DWORD;	break;
	case TYPE_DWORD:	break;
	case TYPE_INT64:	type.type &= ~TYPE_INT64;	type.type |= TYPE_QWORD;	break;
	case TYPE_QWORD:	break;
	default:			ASSERT_RETFALSE(0);
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL TypeIsConstant(
	const TYPE & type)
{
	return (type.type & TYPE_CONSTANT);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL TypeIsIntegral(
	const TYPE & type)
{
	switch (TypeGetBasicType(type))
	{
	case TYPE_CHAR:
	case TYPE_BYTE:
	case TYPE_SHORT:
	case TYPE_WORD:
	case TYPE_INT:
	case TYPE_DWORD:
	case TYPE_INT64:
	case TYPE_QWORD:
	case TYPE_ENUM:
		return TRUE;
	default:
		return FALSE;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int ValueGetAsInteger(
	const VALUE & value,
	const TYPE & type)
{
	switch (TypeGetBasicType(type))
	{
	case TYPE_CHAR:		return value.c;
	case TYPE_BYTE:		return value.uc;
	case TYPE_SHORT:	return value.s;
	case TYPE_WORD:		return value.us;
	case TYPE_INT:		return value.i;
	case TYPE_DWORD:	return value.ui;
	case TYPE_INT64:	return (int)value.ll;
	case TYPE_QWORD:	return (int)value.ull;
	case TYPE_ENUM:		return value.i;
	default:			ASSERT_RETZERO(0);
	}
}


//----------------------------------------------------------------------------
// Parse File functions
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ParseFilePush(
	PARSE & parse,
	const char * filename,
	const char * buf,
	BOOL free_buf)
{
	PARSE_FILE * file = (PARSE_FILE *)MALLOCZ(NULL, sizeof(PARSE_FILE));
	file->filename = filename;
	file->buf = buf;
	file->cur = buf;
	file->free_buf = free_buf;
	file->line = 1;
	file->flags = PARSEFLAG_BOF | PARSEFLAG_BOL;
	file->prev = parse.file;
	parse.file = file;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ParseFilePop(
	PARSE & parse)
{
	PARSE_FILE * file = parse.file;
	ASSERT_RETURN(file);

	parse.file = file->prev;
	if (file->free_buf)
	{
		FREE(NULL, file->buf);
	}
	FREE(NULL, file);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BOOL ParseFileTestFlags(
	PARSE & parse,
	DWORD flags)
{
	ASSERT_RETFALSE(parse.file);
	return ((parse.file->flags & flags) == flags);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void ParseFileSetFlags(
	PARSE & parse,
	DWORD flags)
{
	ASSERT_RETURN(parse.file);
	parse.file->flags |= flags;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void ParseFileClearFlags(
	PARSE & parse,
	DWORD flags)
{
	ASSERT_RETURN(parse.file);
	parse.file->flags &= ~flags;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline int ParseFileGetLine(
	PARSE & parse)
{
	ASSERT_RETZERO(parse.file);
	return parse.file->line;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void ParseFileIncrementLine(
	PARSE & parse)
{
	ASSERT_RETURN(parse.file);
	parse.file->line++;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void ParseFileIncrementIfLevel(
	PARSE & parse)
{
	ASSERT_RETURN(parse.file);
	parse.file->if_level++;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void ParseFileDecrementIfLevel(
	PARSE & parse)
{
	ASSERT_RETURN(parse.file);
	ASSERT_RETURN(parse.file->if_level > 0);
	parse.file->if_level--;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline int ParseFileGetIfLevel(
	PARSE & parse)
{
	ASSERT_RETZERO(parse.file);
	return parse.file->if_level;
}


//----------------------------------------------------------------------------
// String Table functions
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
STRING_TABLE2 * ScriptStringTableInitEx(
	void)
{
	STRING_TABLE2 * tbl = (STRING_TABLE2 *)MALLOCZ(NULL, sizeof(STRING_TABLE2));
	return tbl;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ScriptStringTableFree(
	STRING_TABLE2 * tbl)
{
	ASSERT_RETURN(tbl);
	for (int ii = 0; ii < STRING_TABLE_HASH_SIZE; ii++)
	{
		STRING_NODE * node = tbl->hash[ii];
		while (node)
		{
			STRING_NODE * next = node->next;
			ASSERT(node->str);
			if (node->str)
			{
				FREE(NULL, node->str);
			}
			FREE(NULL, node);
			node = next;
		}
	}
	FREE(NULL, tbl);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline char * sStringTableFindOrAddString(
	char * str,
	int len,
	DWORD hash)
{
	ASSERT_RETNULL(str);

	STRING_TABLE2 * tbl = gVM.StringTable;
	ASSERT_RETNULL(tbl);

	DWORD key = hash % STRING_TABLE_HASH_SIZE;

	STRING_NODE * node = tbl->hash[key];
	while (node)
	{
		if (node->hash == hash && node->len == len && PStrCmp(node->str, str, len) == 0)
		{
			return node->str;
		}
		node = node->next;
	}

	node = (STRING_NODE *)MALLOC(NULL, sizeof(STRING_NODE));
	node->str = (char *)MALLOC(NULL, len + 1);
	MemoryCopy(node->str, len + 1, str, len);
	node->str[len] = 0;
	node->len = len;
	node->hash = hash;
	node->next = tbl->hash[key];
	tbl->hash[key] = node;
	tbl->count++;
	return node->str;
}


//----------------------------------------------------------------------------
// add a string to the string table, or return existing string
//----------------------------------------------------------------------------
char * StringTableAddEx(
	char * str)
{
	char * ret = NULL;

	ASSERT_RETNULL(str);
	DWORD hash = 0;
	char * p = str;
	while (*p)
	{
		hash = hash * 253 + *p;
		p++;
	}
	int len = (int)(p - str);

	CSAutoLock autolock(&gVM.cs);

	ret = sStringTableFindOrAddString(str, len, hash);

	return ret;
}


//----------------------------------------------------------------------------
// add a string to the string table, or return existing string
//----------------------------------------------------------------------------
char * StringTableAddEx(
	char * str,
	int len,
	DWORD hash)
{
	return sStringTableFindOrAddString(str, len, hash);
}


//----------------------------------------------------------------------------
// Byte stream functions
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ByteStreamFree(
	BYTE_STREAM & str)
{
	if (str.buf && str.bDynamic)
	{
		FREE(str.pool, str.buf);
	}
	structclear(str);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ByteStreamInit(
	struct MEMORYPOOL * pool,
	BYTE_STREAM & str,
	BYTE * buf,
	int size)
{
	structclear(str);
	str.pool = pool;
	if (buf)
	{
		str.buf = buf;
		str.size = size;
	}
	else
	{
		str.bDynamic = TRUE;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BYTE_STREAM * ByteStreamInit(
	struct MEMORYPOOL * pool)
{
	BYTE_STREAM * str = (BYTE_STREAM *)MALLOCZ(pool, sizeof(BYTE_STREAM));
	str->pool = pool;
	str->bDynamic = TRUE;
	return str;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ByteStreamFree(
	BYTE_STREAM * str)
{
	if (!str)
	{
		return;
	}
	struct MEMORYPOOL * pool = str->pool;
	ByteStreamFree(*str);
	FREE(pool, str);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ByteStreamReset(
	BYTE_STREAM * str)
{
	ASSERT_RETURN(str);
	str->size = str->cur;
	str->cur = 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ByteStreamResize(
	BYTE_STREAM * str,
	int size)
{
	ASSERT_RETFALSE(str);
	if (str->size > str->cur + size)
	{
		return TRUE;
	}
	ASSERT_RETFALSE(str->bDynamic);
	int newsize = str->size + MAX(size, BYTE_STREAM_BUFFER_SIZE);
	str->buf = (BYTE *)REALLOC(str->pool, str->buf, newsize);
	ASSERT(str->buf);
	if (!str->buf)
	{
		str->size = 0;
		return FALSE;
	}
	str->size = newsize;
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int ByteStreamGetPosition(
	BYTE_STREAM * str)
{
	ASSERT_RETZERO(str);
	return (str->cur);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ByteStreamAddByte(
	BYTE_STREAM * str,
	int value)
{
	ASSERT_RETFALSE(str);
	ASSERT_RETFALSE(value >= 0 && value <= 255);
	RETURN_IF_FALSE(ByteStreamResize(str, sizeof(BYTE)));
	*(BYTE *)(str->buf + str->cur) = (BYTE)value;
	str->cur += sizeof(BYTE);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ByteStreamAddWord(
	BYTE_STREAM * str,
	int value)
{
	ASSERT_RETFALSE(str);
	ASSERT_RETFALSE(value >= 0 && value <= USHRT_MAX);
	RETURN_IF_FALSE(ByteStreamResize(str, sizeof(WORD)));
	*(WORD *)(str->buf + str->cur) = (WORD)value;
	str->cur += sizeof(WORD);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ByteStreamAddInt(
	BYTE_STREAM * str,
	int value)
{
	ASSERT_RETFALSE(str);
	RETURN_IF_FALSE(ByteStreamResize(str, sizeof(int)));
	*(int *)(str->buf + str->cur) = (int)value;
	str->cur += sizeof(int);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ByteStreamAddInt64(
	BYTE_STREAM * str,
	UINT64 value)
{
	ASSERT_RETFALSE(str);
	RETURN_IF_FALSE(ByteStreamResize(str, sizeof(UINT64)));
	*(UINT64 *)(str->buf + str->cur) = (UINT64)value;
	str->cur += sizeof(UINT64);
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ByteStreamAddFloat(
	BYTE_STREAM * str,
	float value)
{
	ASSERT_RETFALSE(str);
	RETURN_IF_FALSE(ByteStreamResize(str, sizeof(float)));
	*(float *)(str->buf + str->cur) = (float)value;
	str->cur += sizeof(float);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ByteStreamAddDouble(
	BYTE_STREAM * str,
	double value)
{
	ASSERT_RETFALSE(str);
	RETURN_IF_FALSE(ByteStreamResize(str, sizeof(double)));
	*(double *)(str->buf + str->cur) = (double)value;
	str->cur += sizeof(double);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ByteStreamAddLDouble(
	BYTE_STREAM * str,
	long double value)
{
	ASSERT_RETFALSE(str);
	RETURN_IF_FALSE(ByteStreamResize(str, sizeof(long double)));
	*(long double *)(str->buf + str->cur) = (long double)value;
	str->cur += sizeof(long double);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ByteStreamAddPtr(
	BYTE_STREAM * str,
	void * ptr)
{
	ASSERT_RETFALSE(str);
	RETURN_IF_FALSE(ByteStreamResize(str, sizeof(void *)));
	*(void **)(str->buf + str->cur) = ptr;
	str->cur += sizeof(void *);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ByteStreamAddValue(
	BYTE_STREAM * str,
	VALUE* value)
{
	ASSERT_RETFALSE(str);
	RETURN_IF_FALSE(ByteStreamResize(str, sizeof(VALUE)));
	memcpy(str->buf + str->cur, value, sizeof(VALUE));
	str->cur += sizeof(VALUE);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ByteStreamAdd(
	BYTE_STREAM * str,
	void * value,
	unsigned int size)
{
	ASSERT_RETFALSE(str);
	RETURN_IF_FALSE(ByteStreamResize(str, size));
	memcpy(str->buf + str->cur, value, size);
	str->cur += size;
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ByteStreamSet(
	BYTE_STREAM * str,
	int value,
	unsigned int size)
{
	ASSERT_RETFALSE(str);
	RETURN_IF_FALSE(ByteStreamResize(str, size));
	memset(str->buf + str->cur, value, size);
	str->cur += size;
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ByteStreamAddToken(
	BYTE_STREAM * str,
	TOKEN & token)
{
	return ByteStreamAdd(str, &token, sizeof(TOKEN));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ByteStreamAddVMINum(
	BYTE_STREAM * stream,
	const TYPE & type,
	const VALUE & value)
{
	switch (TypeGetBasicType(type))
	{
	case TYPE_CHAR:
		ByteStreamAddInt(stream, VMI_CONST_CH);
		ByteStreamAddByte(stream, value.c);
		break;
	case TYPE_BYTE:
		ByteStreamAddInt(stream, VMI_CONST_UC);
		ByteStreamAddByte(stream, value.uc);
		break;
	case TYPE_SHORT:
		ByteStreamAddInt(stream, VMI_CONST_SH);
		ByteStreamAddWord(stream, value.s);
		break;
	case TYPE_WORD:
		ByteStreamAddInt(stream, VMI_CONST_US);
		ByteStreamAddWord(stream, value.us);
		break;
	case TYPE_INT:
		ByteStreamAddInt(stream, VMI_CONST_IN);
		ByteStreamAddInt(stream, value.i);
		break;
	case TYPE_DWORD:
		ByteStreamAddInt(stream, VMI_CONST_UI);
		ByteStreamAddInt(stream, value.ui);
		break;
	case TYPE_INT64:
		ByteStreamAddInt(stream, VMI_CONST_LL);
		ByteStreamAddInt64(stream, value.ll);
		break;
	case TYPE_QWORD:
		ByteStreamAddInt(stream, VMI_CONST_UL);
		ByteStreamAddInt64(stream, value.ull);
		break;
	case TYPE_FLOAT:
		ByteStreamAddInt(stream, VMI_CONST_FL);
		ByteStreamAddFloat(stream, value.f);
		break;
	case TYPE_DOUBLE:
		ByteStreamAddInt(stream, VMI_CONST_DB);
		ByteStreamAddDouble(stream, value.d);
		break;
	case TYPE_LDOUBLE:
		ByteStreamAddInt(stream, VMI_CONST_LD);
		ByteStreamAddLDouble(stream, value.ld);
		break;
	case TYPE_BOOL:
		ByteStreamAddInt(stream, VMI_CONST_IN);
		ByteStreamAddInt(stream, value.i);
		break;
	case TYPE_ENUM:
		{
			ASSERT_RETFALSE(value.sym);
			ByteStreamAddInt(stream, VMI_CONST_IN);
			ByteStreamAddInt(stream, value.sym->value);
		}
		break;
	default:
		ASSERT_RETFALSE(0);
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ByteStreamAddNum(
	BYTE_STREAM * stream,
	const TYPE & type,
	const VALUE & value)
{
	switch (TypeGetBasicType(type))
	{
	case TYPE_CHAR:
		ByteStreamAddByte(stream, value.c);
		break;
	case TYPE_BYTE:
		ByteStreamAddByte(stream, value.uc);
		break;
	case TYPE_SHORT:
		ByteStreamAddWord(stream, value.s);
		break;
	case TYPE_WORD:
		ByteStreamAddWord(stream, value.us);
		break;
	case TYPE_INT:
		ByteStreamAddInt(stream, value.i);
		break;
	case TYPE_DWORD:
		ByteStreamAddInt(stream, value.ui);
		break;
	case TYPE_INT64:
		ByteStreamAddInt64(stream, value.ll);
		break;
	case TYPE_QWORD:
		ByteStreamAddInt64(stream, value.ull);
		break;
	case TYPE_FLOAT:
		ByteStreamAddFloat(stream, value.f);
		break;
	case TYPE_DOUBLE:
		ByteStreamAddDouble(stream, value.d);
		break;
	case TYPE_LDOUBLE:
		ByteStreamAddLDouble(stream, value.ld);
		break;
	case TYPE_BOOL:
		ByteStreamAddInt(stream, value.i);
		break;
	case TYPE_ENUM:
		{
			ASSERT_RETFALSE(value.sym);
			ByteStreamAddInt(stream, value.sym->value);
		}
		break;
	default:
		ASSERT_RETFALSE(0);
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline int StrAddValue(
	void * str,
	const TYPE & type,
	const VALUE & value)
{
	switch (TypeGetBasicType(type))
	{
	case TYPE_CHAR:
		*(char *)str = value.c;
		return sizeof(char);
	case TYPE_BYTE:
		*(unsigned char *)str = value.uc;
		return sizeof(unsigned char);
	case TYPE_SHORT:
		*(short *)str = value.s;
		return sizeof(short);
	case TYPE_WORD:
		*(unsigned short *)str = value.us;
		return sizeof(unsigned short);
	case TYPE_INT:
		*(int *)str = value.i;
		return sizeof(int);
	case TYPE_DWORD:
		*(unsigned int *)str = value.ui;
		return sizeof(unsigned int);
	case TYPE_INT64:
		*(INT64 *)str = value.ll;
		return sizeof(INT64);
	case TYPE_QWORD:
		*(UINT64 *)str = value.ull;
		return sizeof(UINT64);
	case TYPE_FLOAT:
		*(float *)str = value.f;
		return sizeof(float);
	case TYPE_DOUBLE:
		*(double *)str = value.d;
		return sizeof(double);
	case TYPE_LDOUBLE:
		*(long double *)str = value.d;
		return sizeof(long double);
	case TYPE_BOOL:
		*(int *)str = value.i;
		return sizeof(int);
	case TYPE_ENUM:
		{
			ASSERT_RETZERO(value.sym);
			*(int *)str = value.sym->value;
			return sizeof(int);
		}
		break;
	default:
		ASSERT_RETZERO(0);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ByteStreamPatchInt(
	BYTE_STREAM * str,
	int value,
	unsigned int loc)
{
	ASSERT_RETFALSE(str);
	ASSERT_RETFALSE(str->cur >= loc + sizeof(int));
	*(int *)(str->buf + loc) = (int)value;
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ByteStreamGetChar(
	BYTE_STREAM * str,
	char& value)
{
	ASSERT_RETFALSE(str);
	ASSERT_RETFALSE(str->size - str->cur >= sizeof(char));
	value = *(char *)(str->buf + str->cur);
	str->cur += sizeof(char);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ByteStreamGetByte(
	BYTE_STREAM * str,
	BYTE& value)
{
	ASSERT_RETFALSE(str);
	ASSERT_RETFALSE(str->size - str->cur >= sizeof(BYTE));
	value = *(BYTE *)(str->buf + str->cur);
	str->cur += sizeof(BYTE);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ByteStreamGetShort(
	BYTE_STREAM * str,
	short& value)
{
	ASSERT_RETFALSE(str);
	ASSERT_RETFALSE(str->size - str->cur >= sizeof(short));
	value = *(short *)(str->buf + str->cur);
	str->cur += sizeof(short);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ByteStreamGetWord(
	BYTE_STREAM * str,
	WORD& value)
{
	ASSERT_RETFALSE(str);
	ASSERT_RETFALSE(str->size - str->cur >= sizeof(WORD));
	value = *(WORD *)(str->buf + str->cur);
	str->cur += sizeof(WORD);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ByteStreamGetInt(
	BYTE_STREAM * str,
	int & value)
{
	ASSERT_RETFALSE(str);
	ASSERT_RETFALSE(str->size - str->cur >= sizeof(int));
	value = *(int *)(str->buf + str->cur);
	str->cur += sizeof(int);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ByteStreamGetDword(
	BYTE_STREAM * str,
	unsigned int & value)
{
	ASSERT_RETFALSE(str);
	ASSERT_RETFALSE(str->size - str->cur >= sizeof(unsigned int));
	value = *(unsigned int *)(str->buf + str->cur);
	str->cur += sizeof(unsigned int);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ByteStreamGetPtr(
	BYTE_STREAM * str,
	DWORD_PTR& value)
{
	ASSERT_RETFALSE(str);
	ASSERT_RETFALSE(str->size - str->cur >= sizeof(DWORD_PTR));
	value = *(DWORD_PTR*)(str->buf + str->cur);
	str->cur += sizeof(DWORD_PTR);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ByteStreamGetInt64(
	BYTE_STREAM * str,
	INT64& value)
{
	ASSERT_RETFALSE(str);
	ASSERT_RETFALSE(str->size - str->cur >= sizeof(INT64));
	value = *(INT64 *)(str->buf + str->cur);
	str->cur += sizeof(INT64);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ByteStreamGetUInt64(
	BYTE_STREAM * str,
	UINT64& value)
{
	ASSERT_RETFALSE(str);
	ASSERT_RETFALSE(str->size - str->cur >= sizeof(UINT64));
	value = *(UINT64 *)(str->buf + str->cur);
	str->cur += sizeof(UINT64);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ByteStreamGetFloat(
	BYTE_STREAM * str,
	float& value)
{
	ASSERT_RETFALSE(str);
	ASSERT_RETFALSE(str->size - str->cur >= sizeof(float));
	value = *(float *)(str->buf + str->cur);
	str->cur += sizeof(float);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ByteStreamGetDouble(
	BYTE_STREAM * str,
	double& value)
{
	ASSERT_RETFALSE(str);
	ASSERT_RETFALSE(str->size - str->cur >= sizeof(double));
	value = *(double *)(str->buf + str->cur);
	str->cur += sizeof(double);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ByteStreamGetLDouble(
	BYTE_STREAM * str,
	long double& value)
{
	ASSERT_RETFALSE(str);
	ASSERT_RETFALSE(str->size - str->cur >= sizeof(long double));
	value = *(long double *)(str->buf + str->cur);
	str->cur += sizeof(long double);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ByteStreamGetValue(
	BYTE_STREAM * str,
	VALUE* value)
{
	ASSERT_RETFALSE(str);
	ASSERT_RETFALSE(str->size - str->cur >= sizeof(VALUE));
	memcpy(value, str->buf + str->cur, sizeof(VALUE));
	str->cur += sizeof(VALUE);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ByteStreamGet(
	BYTE_STREAM * str,
	void * value,
	int unsigned size)
{
	ASSERT_RETFALSE(str);
	ASSERT_RETFALSE(str->size - str->cur >= size);
	memcpy(value, str->buf + str->cur, size);
	str->cur += size;
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ByteStreamGetToken(
	BYTE_STREAM * str,
	TOKEN & token)
{
	structclear(token);
	return ByteStreamGet(str, &token, sizeof(TOKEN));
}


//----------------------------------------------------------------------------
// token functions
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
TOKEN * MakeToken(
	ETOKEN tok)
{
	TOKEN * token = (TOKEN *)MALLOCZ(NULL, sizeof(TOKEN));
	token->tok = tok;
	return token;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
char * TokenGetIdentName(
	TOKEN * token)
{
	ASSERT_RETNULL(token);
	switch (token->tok)
	{
	case TOK_IDENT:
		ASSERT_RETNULL(token->value.sym);
		return token->value.sym->str;
	default:
		ASSERT_RETNULL(0);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline int TokenGetPrecedence(
	TOKEN & token)
{
	if (token.tok < TOK_LAST)
	{
		return TokenDef[token.tok].precedence;
	}
	return 99;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline int TokenGetAssociativity(
	TOKEN & token)
{
	if (token.tok < TOK_LAST)
	{
		return TokenDef[token.tok].associativity;
	}
	return 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline int TokenGetOperandCount(
	TOKEN & token)
{
	if (token.tok < TOK_LAST)
	{
		return TokenDef[token.tok].operandcount;
	}
	return 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline int TokenGetIntegerOp(
	TOKEN & token)
{
	if (token.tok < TOK_LAST)
	{
		return TokenDef[token.tok].intop;
	}
	return 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline int TokenGetAssign(
	TOKEN & token)
{
	if (token.tok < TOK_LAST)
	{
		return TokenDef[token.tok].assign;
	}
	return 0;
}


//----------------------------------------------------------------------------
// symbol table functions
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void IdentGetLenAndKey(
	const char * str,
	int * len,
	DWORD * key)
{
	const char * p = str;
	*key = STR_HASH_ADDC(0, *p);
	p++;
	while (isidentchar(*p))
	{
		*key = STR_HASH_ADDC(*key, *p);
		p++;
	}
	*len = (int)(p - str);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void StrGetLenAndKey(
	const char * str,
	int * len,
	DWORD * key)
{
	const char * p = str;
	*key = STR_HASH_ADDC(0, *p);
	p++;
	while (*p)
	{
		*key = STR_HASH_ADDC(*key, *p);
		p++;
	}
	*len = (int)(p - str);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SYMBOL_TABLE * ScriptSymbolTableInit(
	void)
{
	SYMBOL_TABLE * tbl = (SYMBOL_TABLE *)MALLOCZ(NULL, sizeof(SYMBOL_TABLE));
	return tbl;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SymbolFree(
	SSYMBOL * symbol)
{
	ASSERT_RETURN(symbol);
	if (symbol->str)
	{
		FREE(NULL, symbol->str);
	}
	if (symbol->tok == TOK_STRUCT || symbol->tok == TOK_FIELD)
	{
		if (symbol->fieldcount > 0)
		{
			FREE(NULL, symbol->fields);
		}
	}
	FREE(NULL, symbol);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ScriptSymbolTableFree(
	SYMBOL_TABLE * tbl)
{
	ASSERT_RETURN(tbl);
	SSYMBOL * sym = tbl->first;
	while (sym)
	{
		SSYMBOL * next = sym->next;
		SymbolFree(sym);
		sym = next;
	}
	FREE(NULL, tbl);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SSYMBOL * SymbolMakeNew(
	ETOKEN tok,
	const char * str,
	int len,
	DWORD key)
{
	SSYMBOL * sym = (SSYMBOL *)MALLOCZ(NULL, sizeof(SSYMBOL));
	if (len)
	{
		ASSERT_RETNULL(str);
		sym->str = (char *)MALLOC(NULL, len + 1);
		memcpy(sym->str, str, len);
		sym->str[len] = 0;
	}
	sym->key = key;
	sym->tok = tok;

	return sym;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SSYMBOL * SymbolMakeNew(
	ETOKEN tok,
	const char * str)
{
	ASSERT_RETNULL(str);

	int len;
	DWORD key;
	StrGetLenAndKey(str, &len, &key);

	return SymbolMakeNew(tok, str, len, key);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SymbolTableAdd(
	SYMBOL_TABLE * tbl,
	SSYMBOL * symbol)
{
	ASSERT_RETURN(tbl);
	ASSERT_RETURN(symbol);
	ASSERT_RETURN(symbol->prev == NULL);
	ASSERT_RETURN(symbol->next == NULL);
	ASSERT_RETURN(symbol->hashprev == NULL);
	ASSERT_RETURN(symbol->hashnext == NULL);

	symbol->next = NULL;
	symbol->prev = tbl->last;
	tbl->last = symbol;
	if (symbol->prev)
	{
		symbol->prev->next = symbol;
	}
	else
	{
		tbl->first = symbol;
	}

	if (!symbol->str)
	{
		return;
	}

	DWORD hash = SYM_HASH_KEY(symbol->key);
	symbol->hashprev = NULL;
	symbol->hashnext = tbl->hash[hash];
	if (symbol->hashnext)
	{
		symbol->hashnext->hashprev = symbol;
	}
	tbl->hash[hash] = symbol;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SSYMBOL * SymbolTableAdd(
	SYMBOL_TABLE * tbl,
	ETOKEN tok,
	const char * str,
	int len,
	DWORD key)
{
	ASSERT_RETNULL(tbl);
	SSYMBOL * sym = SymbolMakeNew(tok, str, len, key);
	ASSERT_RETNULL(sym);

	SymbolTableAdd(tbl, sym);
	return sym;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SSYMBOL * SymbolTableAdd(
	SYMBOL_TABLE * tbl,
	ETOKEN tok,
	const char * str)
{
	ASSERT_RETNULL(str);

	int len;
	DWORD key;
	StrGetLenAndKey(str, &len, &key);

	return SymbolTableAdd(tbl, tok, str, len, key);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SSYMBOL * SymbolTableFind(
	SYMBOL_TABLE * tbl,
	const char * str,
	int len,
	DWORD key)
{
	ASSERT_RETNULL(tbl);
	ASSERT_RETNULL(len > 0);

	DWORD hash = SYM_HASH_KEY(key);
	SSYMBOL * sym = tbl->hash[hash];
	while (sym)
	{
		if (sym->key == key)
		{
			if (icstrncmp(sym->str, str, len) == 0)
			{
				return sym;
			}
		}
		sym = sym->hashnext;
	}
	return NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SSYMBOL * SymbolTableFind(
	SYMBOL_TABLE * tbl,
	const char * str)
{
	const char * p = str;
	DWORD key = STR_HASH_ADDC(0, *p);
	p++;
	while (*p)
	{
		key = STR_HASH_ADDC(key, *p);
		p++;
	}
	int len = (int)(p - str);
	return SymbolTableFind(tbl, str, len, key);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SSYMBOL * SymbolTableFindPrev(
	SYMBOL_TABLE * tbl,
	SSYMBOL * sym,
	ETOKEN tok)
{
	ASSERT_RETNULL(tbl && sym);
	SSYMBOL * curr = sym->hashnext;
	while (curr)
	{
		if ((tok >= TOK_LAST || curr->tok == tok) && PStrCmp(sym->str, curr->str) == 0)
		{
			return curr;
		}
		curr = curr->hashnext;
	}
	return NULL;
}
	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SSYMBOL * SymbolTableGetLast(
	SYMBOL_TABLE * tbl)
{
	ASSERT_RETNULL(tbl);
	return tbl->last;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SymbolTableRemove(
	SYMBOL_TABLE * tbl,
	SSYMBOL * sym)
{
	if (sym->next)
	{
		ASSERT(tbl->last != sym);
		ASSERT(sym->next->prev == sym);
		sym->next->prev = sym->prev;
	}
	else
	{
		ASSERT(tbl->last == sym);
		tbl->last = sym->prev;
	}
	if (sym->prev)
	{
		ASSERT(tbl->first != sym);
		ASSERT(sym->prev->next == sym);
		sym->prev->next = sym->next;
	}
	else
	{
		ASSERT(tbl->first == sym);
		tbl->first = sym->next;
	}
	sym->next = sym->prev = NULL;

	if (!sym->str)
	{
		return;
	}
	if (sym->hashnext)
	{
		ASSERT(sym->hashnext->hashprev == sym);
		sym->hashnext->hashprev = sym->hashprev;
	}
	if (sym->hashprev)
	{
		ASSERT(sym->hashprev->hashnext == sym);
		sym->hashprev->hashnext = sym->hashnext;
	}
	else
	{
		DWORD hash = SYM_HASH_KEY(sym->key);
		ASSERT(tbl->hash[hash] == sym);
		tbl->hash[hash] = sym->hashnext;
	}
	sym->hashnext = sym->hashprev = NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SSYMBOL * StructFindField(
	SSYMBOL * structure,
	const char * fieldname,
	DWORD key)
{
	ASSERT_RETNULL(structure);
	ASSERT_RETNULL(structure->type.type == TYPE_STRUCT);
	for (int ii = 0; ii < structure->fieldcount; ii++)
	{
		SSYMBOL * field = structure->fields[ii];
		if (field->key == key && PStrCmp(fieldname, field->str) == 0)
		{
			return field;
		}
	}
	return NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL StructAddField(
	SSYMBOL * structure,
	SSYMBOL * field)
{
	ASSERT_RETNULL(structure);
	ASSERT_RETNULL(field);
	structure->fields = (SSYMBOL **)REALLOC(NULL, structure->fields, (structure->fieldcount + 1) * sizeof(SSYMBOL *));
	structure->fields[structure->fieldcount] = field;
	structure->fieldcount++;

	field->offset = structure->type_size;
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SymbolTrace(
	SSYMBOL * sym,
	unsigned int index)
{
	trace("%3d: %s\n", index, sym->str);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SymbolTableTrace(
	SYMBOL_TABLE * tbl)
{
	if (!sgbScriptSymbolTrace)
	{
		return;
	}

	trace("symbol table:\n");
	ASSERT_RETURN(tbl);
	SSYMBOL * sym = tbl->first;
	unsigned int index = 0;
	while (sym)
	{
		SSYMBOL * next = sym->next;
		SymbolTrace(sym, index);
		++index;
		sym = next;
	}
	trace("function table:\n");
	for (unsigned int ii = 0; ii < gVM.function_count; ii++)
	{
		FUNCTION * function = gVM.FunctionTable[ii];
		ASSERT(function);
		trace("%3d: %s\n", ii, function->symbol->str);
	}
}


//----------------------------------------------------------------------------
// function table functions
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline FUNCTION * FunctionTableAdd(
	SSYMBOL * functionsym)
{
	ASSERT_RETNULL(functionsym->function == NULL);

	FUNCTION * function = (FUNCTION *)MALLOCZ(NULL, sizeof(FUNCTION));
	ASSERT_RETNULL(function);
	function->symbol = functionsym;
	functionsym->function = function;

	CSAutoLock autolock(&gVM.cs);

	function->index = gVM.function_count;
	gVM.FunctionTable = (FUNCTION **)REALLOC(NULL, gVM.FunctionTable, sizeof(FUNCTION *) * (gVM.function_count + 1));
	gVM.FunctionTable[gVM.function_count++] = function;

	return function;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline FUNCTION * FunctionTableGet(
	int index)
{
	FUNCTION * function = NULL;

	CSAutoLock autolock(&gVM.cs);

	ASSERT(index >= 0 && index < (int)gVM.function_count);
	if (index >= 0 && index < (int)gVM.function_count)
	{
		function = gVM.FunctionTable[index];
	}

	return function;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BOOL FunctionAddParameter(
	FUNCTION * function,
	SSYMBOL * parameter,
	BOOL bRequired)
{
	ASSERT_RETFALSE(function);
	ASSERT_RETFALSE(parameter);

	function->parameters = (SSYMBOL **)REALLOC(NULL, function->parameters, (function->parameter_count + 1) * sizeof(SSYMBOL *));
	function->parameters[function->parameter_count++] = parameter;
	parameter->function = function;
	if (bRequired)
	{
		function->min_param_count++;
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline int FunctionGetParameterCount(
	FUNCTION * function)
{
	ASSERT_RETZERO(function);
	return function->parameter_count;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline SSYMBOL * FunctionGetParameter(
	FUNCTION * function,
	int index)
{
	ASSERT_RETNULL(function);
	ASSERT_RETNULL(index >= 0 && index < function->parameter_count);
	return function->parameters[index];
}


//----------------------------------------------------------------------------
// parse stack functions
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
#define ParseNodeNew(t)		ParseNodeNewDbg(t, __FILE__, __LINE__)
PARSENODE * ParseNodeNewDbg(
	TOKEN * token,
	const char * file,
	unsigned int line)
#else
PARSENODE * ParseNodeNew(
	TOKEN * token)
#endif
{
#if ISVERSION(DEVELOPMENT)
	PARSENODE * node = (PARSENODE *)MALLOCZFL(NULL, sizeof(PARSENODE), file, line);
#else
	PARSENODE * node = (PARSENODE *)MALLOCZ(NULL, sizeof(PARSENODE));
#endif
	node->token = *token;
	return node;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
#define ParseNodeNew(t)		ParseNodeNewDbg(t, __FILE__, __LINE__)
PARSENODE * ParseNodeNewDbg(
	ETOKEN tok,
	const char * file,
	unsigned int line)
#else
PARSENODE * ParseNodeNew(
	ETOKEN tok)
#endif
{
#if ISVERSION(DEVELOPMENT)
	PARSENODE * node = (PARSENODE *)MALLOCZFL(NULL, sizeof(PARSENODE), file, line);
#else
	PARSENODE * node = (PARSENODE *)MALLOCZ(NULL, sizeof(PARSENODE));
#endif
	node->token.tok = tok;
	return node;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PARSENODE * ParseNodeAddChild(
	PARSENODE * parent,
	PARSENODE * child)
{
	parent->children = (PARSENODE **)REALLOC(NULL, parent->children, (parent->childcount + 1) * sizeof(PARSENODE *));
	parent->children[parent->childcount] = child;
	parent->childcount++;
	return parent;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PARSENODE * ParseNodeAddChildFront(
	PARSENODE * parent,
	PARSENODE * child)
{
	parent->children = (PARSENODE **)REALLOC(NULL, parent->children, (parent->childcount + 1) * sizeof(PARSENODE *));
	MemoryMove(parent->children + 1, (parent->childcount) * sizeof(PARSENODE*), parent->children + 0, parent->childcount * sizeof(PARSENODE *));
	parent->children[0] = child;
	parent->childcount++;
	return parent;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PARSENODE * ParseNodeRemoveChild(
	PARSENODE * parent,
	int child_index)
{
	ASSERT_RETNULL(parent);
	ASSERT_RETNULL(child_index >= 0 && child_index < parent->childcount);

	PARSENODE * child = parent->children[child_index];
	if (child_index > 0 && child_index < parent->childcount - 1)
	{
		MemoryMove(parent->children + child_index + 1, (parent->childcount - child_index - 1) * sizeof(PARSENODE*), parent->children + child_index, (parent->childcount - child_index - 1) * sizeof(PARSENODE *));
	}
	parent->childcount--;
	return child;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ParseNodeFree(
	PARSENODE * node)
{
	for (int ii = 0; ii < node->childcount; ii++)
	{
		if (node->children[ii])
		{
			ParseNodeFree(node->children[ii]);
		}
	}
	if (node->children)
	{
		FREE(NULL, node->children);
	}
	FREE(NULL, node);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ParseStackPush(
	PARSESTACK & stack,
	PARSENODE * node)
{
	ASSERT(!node->next);
	node->next = stack.top;
	stack.top = node;
	stack.count++;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PARSENODE * ParseStackPop(
	PARSESTACK & stack)
{
	if (!stack.top)
	{
		return NULL;
	}
	PARSENODE * node = stack.top;
	stack.top = node->next;
	stack.count--;
	node->next = NULL;
	return node;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PARSENODE * ParseStackPeek(
	PARSESTACK & stack,
	int offset = 0)
{
	if (stack.count <= offset)
	{
		return NULL;
	}
	PARSENODE * node = stack.top;
	while (offset > 0)
	{
		node = node->next;
	}
	return node;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PARSENODE * ParseStackPeekFunction(
	PARSE & parse,
	PARSESTACK & stack)
{
	PARSENODE * node = stack.top;
	while (node)
	{
		switch (node->token.tok)
		{
		case TOK_LPAREN:
			return NULL;
		case TOK_FUNCTION:
		case TOK_IFUNCTION:
			return node;
		}
		node = node->next;
	}
	return node;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ParseStackFree(
	PARSESTACK & stack)
{
	for (PARSENODE * node = ParseStackPop(stack); node;)
	{
		ParseNodeFree(node);
	}
	ASSERT(stack.count == 0);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ParseListPush(
	PARSELIST & list,
	PARSENODE * node)
{
	ASSERT(!node->next);
	if (list.last)
	{
		list.last->next = node;
	}
	else
	{
		list.first = list.last = node;
	}
	list.count++;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PARSENODE * ParseListPop(
	PARSELIST & list)
{
	if (!list.first)
	{
		return NULL;
	}
	PARSENODE * node = list.first;
	list.first = node->next;
	if (!list.first)
	{
		list.last = NULL;
	}
	list.count--;
	return list.first;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ParseListFree(
	PARSELIST & list)
{
	for (PARSENODE * node = ParseListPop(list); node;)
	{
		ParseNodeFree(node);
	}
	ASSERT(list.count == 0);
}


//----------------------------------------------------------------------------
// lexer functions
//----------------------------------------------------------------------------
static const char * sParseStatSuffixes(
	PARSE & parse,
	TOKEN & token,
	const char * tmp)
{
	int pos = 0;

	if (*tmp == '\'')
	{
		token.tok = TOK_RSTAT_US;
		pos++;
	}
	else
	{
		BOOL bUnshifted = FALSE;
		BOOL bBase = FALSE;
		BOOL bMod = FALSE;

		while (tmp[pos] == '$')
		{
			int startPos = ++pos;

			if (tmp[pos++]=='u' && tmp[pos++]=='s')
			{
				bUnshifted = TRUE;
				continue;
			}

			pos = startPos;
			if (tmp[pos++]=='b' && tmp[pos++]=='a' && tmp[pos++]=='s' && tmp[pos++]=='e')
			{
				bBase = TRUE;
				continue;
			}

			pos = startPos;
			if (tmp[pos++]=='m' && tmp[pos++]=='o' && tmp[pos++]=='d')
			{
				bMod = TRUE;
				continue;
			}

			pos = startPos;
			ASSERTV_MSG(0, "invalid expression suffix '%s'", tmp[startPos]);
			break;
		}

		if (bUnshifted)
		{
			if (bBase)
			{
				token.tok = TOK_RSTAT_US_BASE;
			}
			else if (bMod)
			{
				token.tok = TOK_RSTAT_US_MOD;
			}
			else
			{
				token.tok = TOK_RSTAT_US;
			}
		}
		else
		{
			if (bBase)
			{
				token.tok = TOK_RSTAT_BASE;
			}
			else if (bMod)
			{
				token.tok = TOK_RSTAT_MOD;
			}
			else
			{
				token.tok = TOK_RSTAT;
			}
		}
	}

	return tmp + pos;
}

//----------------------------------------------------------------------------
// lexer reads in an identifier, and we need to tokenize it
// gets complicated due to context-sensitive params for stats, etc.
//----------------------------------------------------------------------------
const char * ReadIdentifier(
	PARSE & parse,
	const char * str)
{
	int len;
	DWORD key;
	IdentGetLenAndKey(str, &len, &key);

	if (parse.prev_token.tok == TOK_PERIOD)
	{
		PARSENODE * prev = ParseStackPeek(parse.operandstack);
		ASSERT_RETFALSE(prev);
		if (prev->token.type.type == TYPE_STRUCT)		// struct member
		{
			parse.token.tok = TOK_FIELDSTR;
			parse.token.value.str = (char *)MALLOC(NULL, len + 1);
			PStrCopy(parse.token.value.str, str, len+1);
			parse.token.value.str[len] = 0;
			return str + len;
		}
		else if (isRstat(prev))	// this identifier is a param for a stat
		{
			int stat = STAT_GET_STAT(prev->token.value.i);
			const STATS_DATA * stat_data = StatsGetData(NULL, stat);
			ASSERT_RETFALSE(stat_data);
			if (stat_data->m_nParamTable[0] != INVALID_LINK)
			{
				char tmp[256];
				ASSERT_RETFALSE(len < 256);
				PStrCopy(tmp, str, len+1);
				tmp[len] = 0;
				int param = ExcelGetLineByStringIndex(NULL, (EXCELTABLE)stat_data->m_nParamTable[0], tmp);
				if (param >= 0)
				{
					parse.token.tok = TOK_SPARAM;
					parse.token.value.i = param;
					return sParseStatSuffixes(parse, prev->token, str + len);
				}
			}
		}
		else if (prev->token.tok == TOK_PERIOD && prev->childcount == 2 && isRstat(prev->children[1]))		// again, param for a stat, but stat is hidden under a period in cases like attacker.damage=5
		{
			int stat = STAT_GET_STAT(prev->children[1]->token.value.i);
			const STATS_DATA * stat_data = StatsGetData(NULL, stat);
			ASSERT_RETFALSE(stat_data);
			if (stat_data->m_nParamTable[0] != INVALID_LINK)
			{
				char tmp[256];
				ASSERT_RETFALSE(len < 256);
				PStrCopy(tmp, str, len+1);
				tmp[len] = 0;
				int param = ExcelGetLineByStringIndex(NULL, (EXCELTABLE)stat_data->m_nParamTable[0], tmp);
				if (param >= 0)
				{
					parse.token.tok = TOK_SPARAM;
					parse.token.value.i = param;
					return sParseStatSuffixes(parse, prev->children[1]->token, str + len);
				}
			}
		}
	}
	else if (parse.prev_token.tok == TOK_COLON)		// resolves to the row # for a datatable field, for example monsters:fellbeast => 37 or whatever
	{
		PARSENODE * prev = ParseStackPeek(parse.operandstack);
		if (prev && prev->token.tok == TOK_DATATABLE)
		{
			ASSERT_RETFALSE(prev->token.value.sym);
			char tmp[256];
			PStrCopy(tmp, str, len+1);
			ASSERT_RETFALSE(len < 256);
			tmp[len] = 0;
			ParseStackPop(parse.operandstack);
			parse.bLastSymbolWasTerm = FALSE;
			int idx = ExcelGetLineByStringIndex(NULL, (EXCELTABLE)prev->token.value.sym->index, tmp);
			ParseNodeFree(prev);
			parse.token.tok = TOK_NUM;
			parse.token.type = int_type;
			parse.token.value.i = idx;
			return str + len;
		}
	}

	// if top operator on operator stack is a paramlist operator & top operand is a RSTAT or RSTAT_US then we might be a linked param
	{
		PARSENODE * topop = ParseStackPeek(parse.operatorstack);
		if (topop && topop->token.tok == TOK_PARAMLIST)
		{
			PARSENODE * prev = ParseStackPeek(parse.operandstack);
			ASSERT_RETFALSE(prev);
			if (isRstat(prev))
			{
				int stat = STAT_GET_STAT(prev->token.value.i);
				const STATS_DATA * stat_data = StatsGetData(NULL, stat);
				ASSERT_RETFALSE(stat_data);

				unsigned int curparamidx = prev->curparam;
				ASSERT_RETFALSE(curparamidx < stat_data->m_nParamCount);

				if (stat_data->m_nParamTable[curparamidx] != INVALID_LINK)
				{
					char tmp[256];
					ASSERT_RETFALSE(len < 256);
					PStrCopy(tmp, str, len+1);
					tmp[len] = 0;
					int param = ExcelGetLineByStringIndex(NULL, (EXCELTABLE)stat_data->m_nParamTable[curparamidx], tmp);
					if (param >= 0)
					{
						parse.token.tok = TOK_NUM;
						parse.token.type = int_type;
						parse.token.value.i = param;
						return str + len;
					}
				}
			}
			else if (prev->token.tok == TOK_PERIOD && prev->childcount == 2 && isRstat(prev->children[1]))
			{
				int stat = STAT_GET_STAT(prev->children[1]->token.value.i);
				const STATS_DATA * stat_data = StatsGetData(NULL, stat);
				ASSERT_RETFALSE(stat_data);

				unsigned int curparamidx = prev->curparam;
				ASSERT_RETFALSE(curparamidx < stat_data->m_nParamCount);

				if (stat_data->m_nParamTable[curparamidx] != INVALID_LINK)
				{
					char tmp[256];
					ASSERT_RETNULL(len < 256);
					PStrCopy(tmp, str, len+1);
					tmp[len] = 0;
					int param = ExcelGetLineByStringIndex(NULL, (EXCELTABLE)stat_data->m_nParamTable[curparamidx], tmp);
					if (param >= 0)
					{
						parse.token.tok = TOK_NUM;
						parse.token.type = int_type;
						parse.token.value.i = param;
						return str + len;
					}
				}
			}
		}
	}
/*
	// see if we have a function on the operator stack
	if (!parse.bParamDeclWanted)
	{
		PARSENODE * funcnode = ParseStackPeekFunction(parse, parse.operatorstack);
		if (funcnode)
		{
			SSYMBOL * func = funcnode->token.value.sym;
			ASSERT_RETNULL(func);
			FUNCTION * function = func->function;
			ASSERT_RETNULL(function);
			int stat = INVALID_LINK;
			for (int ii = funcnode->curparam; ii < function->parameter_count; ii++)
			{
				SSYMBOL * param = FunctionGetParameter(function, ii);
				switch (param->parameter_type)
				{
				case PARAM_NORMAL:
				case PARAM_INDEX:
					{
						if (param->parameter_type == PARAM_INDEX)
						{
							char tmp[256];
							ASSERT_RETNULL(len < 256);
							strncpy(tmp, str, len);
							tmp[len] = 0;
							int idx = ExcelGetLineByStringIndex(NULL, (EXCELTABLE)param->index_table, param->index_index, tmp);
							if (idx >= 0)
							{
								if (param->index_table == DATATABLE_STATS && param->index_index == 0)
								{
									stat = idx;
								}
								parse.token.tok = TOK_NUM;
								parse.token.type = int_type;
								parse.token.value.i = idx;
								funcnode->curparam++;
								return str + len;
							}
						}
						break;
					}
					break;
				case PARAM_STAT_PARAM:
					{
						if (str[0] >= '0' && str[0] <= '9' || str[0] == '-')
						{
							char tmp[256];
							ASSERT_RETNULL(len < 256);
							strncpy(tmp, str, len);
							tmp[len] = 0;
							parse.token.tok = TOK_NUM;
							parse.token.type = int_type;
							parse.token.value.i = PStrToInt(tmp);
							funcnode->curparam++;
							return str + len;
						}
						if (stat > INVALID_LINK)
						{
							const STATS_DATA * stat_data = StatsGetData(NULL, stat);
							ASSERT_RETNULL(stat_data);
							if (stat_data->nParamTable > INVALID_LINK)
							{
								char tmp[256];
								ASSERT_RETNULL(len < 256);
								strncpy(tmp, str, len);
								tmp[len] = 0;
								int param = ExcelGetLineByStringIndex(NULL, (EXCELTABLE)stat_data->nParamTable, tmp);
								if (param >= 0)
								{
									parse.token.tok = TOK_SPARAM;
									parse.token.type = int_type;
									parse.token.value.i = param;
									funcnode->curparam++;
									return str + len;
								}
							}
						}	
					}
					break;
				default:
					break;
				}
				funcnode->curparam++;
			}
		}
	}
*/
	SSYMBOL * symbol = SymbolTableFind(gVM.SymbolTable, str, len, key);
	if (!symbol)
	{
		symbol = SymbolTableAdd(gVM.SymbolTable, TOK_IDENT, str, len, key);
		ASSERT_RETNULL(symbol);
	}

	parse.token.tok = symbol->tok;
	parse.token.type = symbol->type;

	switch (symbol->tok)
	{
		case TOK_RSTAT:
		{
			parse.token.value.i = symbol->index;
			return sParseStatSuffixes(parse, parse.token, str + len);
		}
		case TOK_CONTEXTVAR:
		{
			parse.token.value.i = symbol->index;
			break;
		}
		default:
		{
			parse.token.value.sym = symbol;
			break;
		}
	}

	return str + len;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const char * ReadString(
	PARSE & parse,
	const char * p,
	BOOL iswchar)
{
	const char sep = *p;
	p++;
	char * buf = parse.tokstr;

	while (*p)
	{
		// need to process escape-sequences here

		if (*p == sep)
		{
			*buf = 0;

			parse.token.value.str = StringTableAddEx(parse.tokstr);
			parse.token.tok = TOK_STR;
			return p + 1;
		}
		*buf++ = *p++;
	}
	// error condition
	return NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline const char * ReadToken1(
	PARSE & parse,
	const char * p,
	ETOKEN tok1)
{
	parse.token.tok = tok1;
	return p + 1;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline const char * ReadToken2(
	PARSE & parse,
	const char * p,
	ETOKEN tok1,
	char tok2c,
	ETOKEN tok2)
{
	char next = p[1];

	if (next == tok2c)
	{
		parse.token.tok = tok2;
		return p + 2;
	}
	parse.token.tok = tok1;
	return p + 1;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline const char * ReadToken3(
	PARSE & parse,
	const char * p,
	ETOKEN tok1,
	char tok2c,
	ETOKEN tok2,
	char tok3c,
	ETOKEN tok3)
{
	char next = p[1];

	if (next == tok2c)
	{
		parse.token.tok = tok2;
		return p + 2;
	}
	if (next == tok3c)
	{
		parse.token.tok = tok3;
		return p + 2;
	}
	parse.token.tok = tok1;
	return p + 1;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline const char * ReadComment(
	PARSE & parse,
	const char * p)
{
	while (*p)
	{
		if (p[0] == '*' && p[1] == '/')
		{
			return p + 2;
		}
		p++;
	}
	return NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline const char * ReadLineComment(
	PARSE & parse,
	const char * p)
{
	while (*p)
	{
		if (p[0] == '\n')
		{
			++parse.line;
			return p + 1;
		}
		p++;
	}
	return NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ReadToEOL(
   PARSE & parse)
{
	if (parse.token.tok == TOK_LINEFEED || parse.token.tok == TOK_EOF)
	{
		return TRUE;
	}
	do
	{
		int result = GetTokenNoMacro(parse, FALSE);
		if (!result)
		{
			SCRIPT_ERROR("unexpected end of buffer while parsing macro.");
		}
	} while (parse.token.tok != TOK_LINEFEED && parse.token.tok != TOK_EOF);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ParseConstantExpr(
	PARSE & parse,
	TOKEN & token,
	BOOL bStopAtEOL = FALSE)
{
	parse.bConstWanted = TRUE;
	parse.bStopAtEOL = bStopAtEOL;
	PARSENODE * expr = ParseExpr(parse, parse.operatorstack.count, parse.operandstack.count);
	parse.bStopAtEOL = FALSE;
	parse.bConstWanted = FALSE;
	RETURN_IF_FALSE(expr);
	RETURN_IF_FALSE(expr->token.tok == TOK_NUM);
	RETURN_IF_FALSE(TypeIsConstant(expr->token.type));
	token = expr->token;
	ParseNodeFree(expr);
	return TRUE;
}


//----------------------------------------------------------------------------
// parse after #define
//----------------------------------------------------------------------------
BOOL ParseDefine(
	PARSE & parse)
{
	ASSERT_RETFALSE(parse.token.tok == TOK_IDENT);

	SSYMBOL * macrosymbol = parse.token.value.sym;
	ASSERT_RETFALSE(macrosymbol);
	macrosymbol->tok = TOK_MACRO_OBJ;

	char c = parse.file->cur[0];
	if (c == '(')
	{
		FUNCTION * func = FunctionTableAdd(macrosymbol);
		ASSERT_RETFALSE(func);
		macrosymbol->tok = TOK_MACRO_FUNC;

		BOOL result = GetTokenNoMacro(parse);
		if (!result || parse.token.tok != TOK_LPAREN)
		{
			SCRIPT_ERROR("unexpected end of buffer while parsing macro.");
		}
		ASSERT_RETFALSE(GetTokenNoMacro(parse));
		while (parse.token.tok != TOK_RPAREN)
		{
			if (func->varargs)
			{
				SCRIPT_ERROR("badly punctuated parameter list for macro.");
			}
			if (parse.token.tok == TOK_IDENT)
			{
				SSYMBOL * param = parse.token.value.sym;
				ASSERT_RETFALSE(param);
				param->tok = TOK_MACRO_PARAM;
				ASSERT_RETFALSE(FunctionAddParameter(func, param, TRUE));
				ASSERT_RETFALSE(GetTokenNoMacro(parse));
				if (parse.token.tok == TOK_COMMA)
				{
					ASSERT_RETFALSE(GetTokenNoMacro(parse));
				}
				continue;
			}
			else if (parse.token.tok == TOK_DOTS)
			{
				func->varargs = TRUE;
			}
			else
			{
				SCRIPT_ERROR("badly punctuated parameter list for macro.");
			}
			ASSERT_RETFALSE(GetTokenNoMacro(parse));
		}
	}

	BYTE_STREAM * stream = NULL;
	if (!GetTokenNoMacro(parse, FALSE))
	{
		return FALSE;
	}
	while (parse.token.tok != TOK_LINEFEED && parse.token.tok != TOK_EOF)
	{
		if (!stream)
		{
			stream = ByteStreamInit(NULL);
		}
		ByteStreamAddToken(stream, parse.token);
		if (!GetTokenNoMacro(parse, FALSE))
		{
			ByteStreamFree(stream);
			return FALSE;
		}
	}
	if (macrosymbol->tok == TOK_MACRO_OBJ)
	{
		macrosymbol->replacement = stream;
	}
	else
	{
		FUNCTION * func = macrosymbol->function;

		ASSERT_RETFALSE(func);
		func->stream = stream;

		int paramcount = FunctionGetParameterCount(func);
		for (int ii = 0; ii < paramcount; ii++)
		{
			SSYMBOL * parameter = FunctionGetParameter(func, ii);
			SymbolTableRemove(gVM.SymbolTable, parameter);
		}
	}

	return TRUE;
}


//----------------------------------------------------------------------------
// skip ifdef'd out section
// mode: 0-skip until we get an else, 1-skip until we get to the end 
//----------------------------------------------------------------------------
BOOL PreprocessSkip(
	PARSE & parse,
	int mode)
{
	ASSERT_RETFALSE(parse.file);
	int target_if_level = ParseFileGetIfLevel(parse);

	if (ParseFileTestFlags(parse, PARSEFLAG_EOL))
	{
		ParseFileClearFlags(parse, PARSEFLAG_EOL);
		ParseFileSetFlags(parse, PARSEFLAG_BOL);
	}

	const char * p = parse.file->cur;
	while (TRUE)
	{
		parse.curtext = p;
		char c = *p;
		switch (c)
		{
		case 0:
			parse.file->cur = p;
			return FALSE;
		case ' ':
		case '\t':
		case '\f':
		case '\v':
		case '\r':
			p++;
			continue;
		case '/':
			{
				char next = p[1];
				if (next == '/')
				{
					p = ReadLineComment(parse, p);
					ParseFileSetFlags(parse, PARSEFLAG_BOL);
					continue;
				}
				if (next == '*')
				{
					p = ReadComment(parse, p);
					break;
				}
				p++;
			}
			break;
		case '\n':
			p++;
			parse.line++;
			ParseFileSetFlags(parse, PARSEFLAG_BOL);
			continue;
		case '#':
			if (!ParseFileTestFlags(parse, PARSEFLAG_BOL))
			{
				p++;
				break;
			}
			parse.file->cur = p + 1;
			ASSERT_RETFALSE(GetTokenNoMacro(parse, FALSE));
			ASSERT_RETFALSE(parse.file);
			p = parse.file->cur;
			switch (parse.token.tok)
			{
			case TOK_IF:
				ParseFileIncrementIfLevel(parse);
				break;
			case TOK_IFDEF:
				ParseFileIncrementIfLevel(parse);
				break;
			case TOK_IFNDEF:
				ParseFileIncrementIfLevel(parse);
				break;
			case TOK_ELSE:
				if (mode == 1)
				{
					break;
				}
				if (ParseFileGetIfLevel(parse) == target_if_level)
				{
					ASSERT_RETFALSE(ReadToEOL(parse));
					return TRUE;
				}
				break;
			case TOK_ELIF:
				if (mode == 1)
				{
					break;
				}
				if (ParseFileGetIfLevel(parse) == target_if_level)
				{
					NEXT();
					TOKEN token;
					ASSERT_RETFALSE(ParseConstantExpr(parse, token, TRUE));
					if (ValueGetAsInteger(token.value, token.type))
					{
						ASSERT_RETFALSE(ReadToEOL(parse));
						return TRUE;
					}
					ASSERT_RETFALSE(parse.file);
					p = parse.file->cur;
				}
				break;
			case TOK_ENDIF:
				ParseFileDecrementIfLevel(parse);
				if (ParseFileGetIfLevel(parse) < target_if_level)
				{
					parse.file->cur = p;
					return ReadToEOL(parse);
				}
				break;
			}
			break;
		default:
			p++;
			break;
		}
		ParseFileClearFlags(parse, PARSEFLAG_BOL);
		parse.file->cur = p;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL Preprocess(
	PARSE & parse)
{
	switch (parse.token.tok)
	{
	case TOK_DEFINE:
		{
			ASSERT_RETFALSE(GetTokenNoMacro(parse));
			if (!ParseDefine(parse))
			{
				return FALSE;
			}
			ASSERT_RETFALSE(ReadToEOL(parse));
			return TRUE;
		}
		break;
	case TOK_INCLUDE:
		{
			NEXT();
			ASSERT_RETFALSE(parse.token.tok == TOK_STR);
			char * filename = parse.token.value.str;
			ASSERT_RETFALSE(ReadToEOL(parse));

			DWORD dwBytesread;
			char * buf = (char *)FileLoad(MEMORY_FUNC_FILELINE(NULL, filename, &dwBytesread));
			if (!buf)
			{
				TraceError( "ERROR: Script include failed: can't find %s", filename);
				return FALSE;
			}

			ParseFilePush(parse, filename, buf, TRUE);
			return TRUE;
		}
		break;
	case TOK_IFDEF:
		{
			NEXT();
			ParseFileIncrementIfLevel(parse);
			if (parse.token.tok != TOK_IDENT)
			{
				ASSERT_RETFALSE(ReadToEOL(parse));
				return TRUE;
			}
			else
			{
				return PreprocessSkip(parse, 0);
			}
		}
		break;
	case TOK_IFNDEF:
		{
			NEXT();
			ParseFileIncrementIfLevel(parse);
			if (parse.token.tok == TOK_IDENT)
			{
				ASSERT_RETFALSE(ReadToEOL(parse));
				return TRUE;
			}
			else
			{
				return PreprocessSkip(parse, 0);
			}
		}
		break;
	case TOK_IF:
		{
			NEXT();
			ParseFileIncrementIfLevel(parse);
			TOKEN token;
			ASSERT_RETFALSE(ParseConstantExpr(parse, token, TRUE));
			if (ValueGetAsInteger(token.value, token.type))
			{
				ASSERT_RETFALSE(ReadToEOL(parse));
				return TRUE;
			}
			else
			{
				return PreprocessSkip(parse, 0);
			}
		}
		break;
	case TOK_ELSE:
	case TOK_ELIF:
		{
			ASSERT_RETFALSE(ReadToEOL(parse));
			return PreprocessSkip(parse, 1);
		}
		break;
	case TOK_ENDIF:
		ParseFileDecrementIfLevel(parse);
		ASSERT_RETFALSE(ReadToEOL(parse));
		return TRUE;
	case TOK_UNDEF:
		{
			RETURN_IF_FALSE(!GetTokenNoMacro(parse));
			if (parse.token.tok == TOK_MACRO_OBJ || parse.token.tok == TOK_MACRO_FUNC)
			{
				SymbolTableRemove(gVM.SymbolTable, parse.token.value.sym);
			}
			ASSERT_RETFALSE(ReadToEOL(parse));
			return TRUE;
		}
		break;
	case TOK_ERROR:
		ASSERT_RETFALSE(0);
		break;
	case TOK_WARNING:
		ASSERT_RETFALSE(0);
		break;
	case TOK_PRAGMA:
		ASSERT_RETFALSE(0);
		break;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL ParseNumberSetBaseIntegralType(
	TOKEN * token,
	UINT64 n,
	const char * str)
{
	token->type.type = 0;
    if ((n & 0xffffffff00000000LL) != 0) 
	{
        if ((n >> 63) != 0)
		{
            token->type.type = TYPE_CONSTANT | TYPE_QWORD;
		}
        else
		{
            token->type.type = TYPE_CONSTANT | TYPE_INT64;
		}
    } 
	else if (n > 0x7fffffffLL) 
	{
        token->type.type = TYPE_CONSTANT | TYPE_DWORD;
    } 
	else 
	{
        token->type.type = TYPE_CONSTANT | TYPE_INT;
    }
	if (*str == 'U' || *str == 'u')
	{
		RETURN_IF_FALSE(TypeSetUnsigned(token->type));
		str++;
	}
	if (*str == 'L' || *str == 'l')
	{
		str++;
		if (*str == 'L' || *str == 'l')
		{
            if (token->type.type == (TYPE_CONSTANT | TYPE_INT))
			{
                token->type.type = TYPE_CONSTANT | TYPE_INT64;
			}
            else if (token->type.type == (TYPE_CONSTANT | TYPE_DWORD))
			{
                token->type.type = TYPE_CONSTANT | TYPE_QWORD;
			}
		}
	}

	switch (TypeGetBasicType(token->type))
	{
	case TYPE_INT:		token->value.i = (int)n;				break;
	case TYPE_DWORD:	token->value.ui = (unsigned int)n;	break;
	case TYPE_INT64:	token->value.ll = (INT64)n;			break;
	case TYPE_QWORD:	token->value.ull = (UINT64)n;		break;
	default:			ASSERT_RETFALSE(0);
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL ParseNumberSetBaseFloatingPointType(
	TOKEN * token,
	long double ld,
	const char * str)
{
	if (*str == 'F' || *str == 'f')
	{	// need to handle overflow
		token->type.type = TYPE_CONSTANT | TYPE_FLOAT;
		token->value.f = (float)ld;
	}
	else if (*str == 'L' || *str == 'l')
	{
		token->type.type = TYPE_CONSTANT | TYPE_LDOUBLE;
		token->value.ld = ld;
	}
	else
	{
		token->type.type = TYPE_CONSTANT | TYPE_DOUBLE;
		token->value.d = (double)ld;
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ParseNumberHex(
	TOKEN * token,
	const char * str)
{
	UINT64 n = 0;
	int ch = *str++;
	while (1)
	{
		if (ch >= 'a' && ch <= 'f')
		{
			n = n * 16 + (10 + ch - 'a');
		}
		else if (ch >= 'A' && ch <= 'F')
		{
			n = n * 16 + (10 + ch - 'A');
		}
		else if (ch >= '0' && ch <= '9')
		{
			n = n * 16 + (ch - '0');
		}
		else
		{
			break;
		}
		ch = *str++;
	}
	ASSERT_RETFALSE(ch != '.');
	return ParseNumberSetBaseIntegralType(token, n, str);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ParseNumberBin(
	TOKEN * token,
	const char * str)
{
	UINT64 n = 0;
	int ch = *str++;
	while (1)
	{
		if (ch == '0' || ch == '1')
		{
			n = n * 2 + (ch - '0');
		}
		else
		{
			break;
		}
		ch = *str++;
	}
	ASSERT_RETFALSE(ch != '.');
	return ParseNumberSetBaseIntegralType(token, n, str);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ParseNumberOct(
	TOKEN * token,
	const char * str)
{
	UINT64 n = 0;
	int ch = *str++;

	while (1)
	{
		if (ch >= '0' && ch <= '7')
		{
			n = n * 8 + (ch - '0');
		}
		else
		{
			break;
		}
		ch = *str++;
	}
	ASSERT_RETFALSE(ch != '.');
	return ParseNumberSetBaseIntegralType(token, n, str);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ParseNumberDec(
	TOKEN * token,
	const char * str)
{
	UINT64 n = 0;
	int ch = *str++;

	while (1)
	{
		if (ch >= '0' && ch <= '9')
		{
			n = n * 10 + (ch - '0');
		}
		else
		{
			break;
		}
		ch = *str++;
	}
	if (ch != '.')
	{
		return ParseNumberSetBaseIntegralType(token, n, str);
	}

	int div = 1;
    while (1) 
	{
		int ch = *str;
		if (ch == 0)
		{
			break;
		}
		if (ch >= '0' && ch <= '9')
		{
			n = n * 10 + (ch - '0');
		}
		else
		{
			return FALSE;
		}
		str++;
		div *= 10;
    }

	long double ld = (long double)n;
	if (div > 1)
	{
		ld /= (long double)div;
	}

	if (ch == 'e' || ch == 'E')
	{
		int exp_val = 0;
		int sign = 1;
		ch = *str++;
        if (ch == '+') 
		{
            ch = *str++;
        } 
		else if (ch == '-') 
		{
            sign = -1;
            ch = *str++;
        }
        if (ch < '0' || ch > '9')
		{
			return FALSE;
		}
        while (ch >= '0' && ch <= '9') 
		{
            exp_val = exp_val * 10 + ch - '0';
            ch = *str++;
        }
		exp_val = exp_val * sign;
	    ld = ldexp(ld, exp_val);
	}

	return ParseNumberSetBaseFloatingPointType(token, ld, str);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ParseNumber(
	TOKEN * tok,
	const char * str)
{
	int t = str[0];
	if (t == 0)
	{
		return FALSE;
	}
	int ch = str[1];

	if (t == '0')
	{
		if (ch == 'x' || ch == 'X')
		{
			return ParseNumberHex(tok, str + 2);
		}
		else if (ch == 'b' || ch == 'B')
		{
			return ParseNumberBin(tok, str + 2);
		}
		else if (ch >= '0' && ch <= '7')
		{
			return ParseNumberOct(tok, str + 1);
		}
		else
		{
			return ParseNumberDec(tok, str);
		}
	}
	else if (t == '.' || (t > '0' && t <= '9'))
	{
		return ParseNumberDec(tok, str);
	}
	ASSERT_RETFALSE(0);
}


//----------------------------------------------------------------------------
// return next token w/o macro substitution
//----------------------------------------------------------------------------
BOOL GetTokenNoMacro(
	PARSE & parse,
	BOOL bSkipEOL)
{
	if (!parse.file)
	{
		parse.token.tok = TOK_EOF;
		parse.token.index = parse.token_count++;
		return TRUE;
	}

	parse.prev_token = parse.token;
	structclear(parse.token);
	const char * p = parse.file->cur;
	while (1)
	{
		if (ParseFileTestFlags(parse, PARSEFLAG_EOL))
		{
			ParseFileClearFlags(parse, PARSEFLAG_EOL);
			ParseFileSetFlags(parse, PARSEFLAG_BOL);
		}
		if (!p)
		{
			return FALSE;
		}

		parse.curtext = p;
		char c = *p;
		switch (c)
		{
		case 0:
			if (parse.file)
			{
				ParseFilePop(parse);
				if (parse.file)
				{
					p = parse.file->cur;
					continue;
				}
			}
			parse.token.tok = TOK_EOF;
			parse.token.index = parse.token_count++;
			return TRUE;
		case ' ':
		case '\t':
		case '\f':
		case '\v':
		case '\r':
			p++;
			continue;
		case '\n':
			ParseFileSetFlags(parse, PARSEFLAG_EOL);
			parse.line++;
			p++;
			ParseFileIncrementLine(parse);
			if (!bSkipEOL)
			{
				parse.token.tok = TOK_LINEFEED;
				break;
			}
			continue;
		case '#':
			if (ParseFileTestFlags(parse, PARSEFLAG_BOL))
			{
				parse.file->cur = p + 1;
				ASSERT_RETFALSE(GetTokenNoMacro(parse));
				if (!Preprocess(parse))
				{
					return FALSE;
				}
				if (!parse.file)
				{
					parse.token.tok = TOK_EOF;
					parse.token.index = parse.token_count++;
					return TRUE;
				}
				p = parse.file->cur;
				ParseFileClearFlags(parse, PARSEFLAG_EOL);
				ParseFileSetFlags(parse, PARSEFLAG_BOL);
				continue;
			}
			p = ReadToken2(parse, p, TOK_SHARP, '#', TOK_TWOSHARPS);
			break;
		case 'a': case 'b': case 'c': case 'd':
		case 'e': case 'f': case 'g': case 'h':
		case 'i': case 'j': case 'k': case 'l':
		case 'm': case 'n': case 'o': case 'p':
		case 'q': case 'r': case 's': case 't':
		case 'u': case 'v': case 'w': case 'x':
		case 'y': case 'z': 
		case 'A': case 'B': case 'C': case 'D':
		case 'E': case 'F': case 'G': case 'H':
		case 'I': case 'J': case 'K': 
		case 'M': case 'N': case 'O': case 'P':
		case 'Q': case 'R': case 'S': case 'T':
		case 'U': case 'V': case 'W': case 'X':
		case 'Y': case 'Z': 
		case '_':
			p = ReadIdentifier(parse, p);
			break;
		case 'L':		// wide-char indicator?
			{
				char c2 = p[1];
				if (c2 == '\'' || c2 == '\"')
				{
					p = ReadString(parse, p + 1, TRUE); 
					break;
				}
				p = ReadIdentifier(parse, p);
				break;
			}
			break;
		case '0': case '1': case '2': case '3':
		case '4': case '5': case '6': case '7':
		case '8': case '9':
			{
				char * str = parse.tokstr;
				char prev = *p;
				*str++ = *p++;
				while (isalphanum(*p) || *p == '.' ||
                  ((*p == '+' || *p == '-') && 
                   (prev == 'e' || prev == 'E' || prev == 'p' || prev == 'P')))
				{
					prev = *p;
					*str++ = *p++;
				}
				*str = 0;
				parse.token.tok = TOK_NUM;
				if (!ParseNumber(&parse.token, parse.tokstr))
				{
					return FALSE;
				}
			}
			break;
		case '\'':
		case '\"':
			p = ReadString(parse, p, FALSE);
			break;
		case '.':
			if (p[1] == '.' && p[2] == '.')
			{
				parse.token.tok = TOK_DOTS;
				p += 3;
			}
			else if (p[1] == '{')
			{
				parse.token.tok = TOK_PARAMLIST;
				p += 2;
			}
			else
			{
				parse.token.tok = TOK_PERIOD;
				p++;
			}
			break;
		case '<':
			{
				if (p[1] == '<')
				{
					p = ReadToken2(parse, p + 1, TOK_LSHIFT, '=', TOK_A_LSHIFT);
				}
				else
				{
					p = ReadToken2(parse, p, TOK_LT, '=', TOK_LTE);
				}
				break;
			}
			break;
		case '>':
			{
				if (p[1] == '>')
				{
					p = ReadToken2(parse, p + 1, TOK_RSHIFT, '=', TOK_A_RSHIFT);
				}
				else
				{
					p = ReadToken2(parse, p, TOK_GT, '=', TOK_GTE);
				}
				break;
			}
			break;
		case '&':
			p = ReadToken3(parse, p, TOK_AND, '&', TOK_LAND, '=', TOK_A_AND);
			break;
		case '|':
			p = ReadToken3(parse, p, TOK_OR, '|', TOK_LOR, '=', TOK_A_OR);
			break;
		case '+':
			if (parse.bLastSymbolWasTerm)
			{
				p = ReadToken3(parse, p, TOK_ADD, '+', TOK_POSTINC, '=', TOK_A_ADD);
			}
			else
			{
				p = ReadToken3(parse, p, TOK_POS, '+', TOK_PREINC, '=', TOK_A_ADD);
			}
			break;
		case '-':
			if (parse.bLastSymbolWasTerm)
			{
				p = ReadToken3(parse, p, TOK_SUB, '+', TOK_POSTDEC, '=', TOK_A_SUB);
			}
			else
			{
				p = ReadToken3(parse, p, TOK_NEG, '+', TOK_PREDEC, '=', TOK_A_SUB);
			}
			break;
		case '!':
			p = ReadToken2(parse, p, TOK_NOT, '=', TOK_NE);
			break;
		case '=':
			p = ReadToken2(parse, p, TOK_ASSIGN, '=', TOK_EQ);
			break;
		case '*':
			p = ReadToken2(parse, p, TOK_MUL, '=', TOK_A_MUL);
			break;
		case '/':
			{
				char next = p[1];
				if (next == '/')
				{
					p = ReadLineComment(parse, p);
					continue;
				}
				if (next == '*')
				{
					p = ReadComment(parse, p);
					continue;
				}
				p = ReadToken2(parse, p, TOK_DIV, '=', TOK_A_DIV);
			}
			break;
		case '%':
			p = ReadToken2(parse, p, TOK_MOD, '=', TOK_A_MOD);
			break;
		case '^':
			p = ReadToken3(parse, p, TOK_XOR, '^', TOK_POWER, '=', TOK_A_XOR);
			break;
		case '(':
			p = ReadToken1(parse, p, TOK_LPAREN);
			break;
		case ')':
			p = ReadToken1(parse, p, TOK_RPAREN);
			break;
		case '[':
			p = ReadToken1(parse, p, TOK_LBRACKET);
			break;
		case ']':
			p = ReadToken1(parse, p, TOK_RBRACKET);
			break;
		case '{':
			p = ReadToken1(parse, p, TOK_LCURLY);
			break;
		case '}':
			p = ReadToken1(parse, p, TOK_RCURLY);
			break;
		case ',':
			p = ReadToken1(parse, p, TOK_COMMA);
			break;
		case ';':
			p = ReadToken1(parse, p, TOK_SEMICOLON);
			break;
		case ':':
			p = ReadToken1(parse, p, TOK_COLON);
			break;
		case '?':
			p = ReadToken1(parse, p, TOK_QUESTION);
			break;
		case '~':
			p = ReadToken1(parse, p, TOK_TILDE);
			break;
		default:
			// unrecognized character
			return FALSE;
		}

		if (parse.token.tok == TOK_IDENT)
		{
			parse.lasttoken = parse.token.value.sym->str;
		}

		parse.token.index = parse.token_count++;
		parse.file->cur = p;
		parse.curtext = p;
		ParseFileClearFlags(parse, PARSEFLAG_BOF | PARSEFLAG_BOL);
		return TRUE;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ReadExprStream(
	PARSE & parse,
	BYTE_STREAM * stream)
{
	int paren_level = 0;
	while (TRUE)
	{
		switch (parse.token.tok)
		{
		case TOK_LPAREN:
			paren_level++;
			break;
		case TOK_RPAREN:
			if (paren_level == 0)
			{
				return TRUE;
			}
			paren_level--;
			break;
		case TOK_COMMA:
			if (paren_level == 0)
			{
				return TRUE;
			}
			break;
		case TOK_EOF:
			return FALSE;
		default:
			break;
		}
		RETURN_IF_FALSE(ByteStreamAddToken(stream, parse.token));
		NEXT();
	}

	return TRUE;
}


//----------------------------------------------------------------------------
// return next token with macro substitution
//----------------------------------------------------------------------------
BOOL GetToken(
	PARSE & parse)
{
	// if we have a macro in process, return the next token
	BOOL gottoken = FALSE;
	while (parse.macrostack.stacktop > 0)
	{
		MACRONODE* macronode = parse.macrostack.stack[parse.macrostack.stacktop - 1];

		if (macronode->cursor.cur < macronode->cursor.size &&
			ByteStreamGetToken(&macronode->cursor, parse.token))
		{
			gottoken = TRUE;
			break;
		}
		// pop & free top macronode
		if (macronode->params)
		{
			FREE(NULL, macronode->params);
		}
		FREE(NULL, macronode);
		parse.macrostack.stack[parse.macrostack.stacktop - 1] = NULL;
		parse.macrostack.stacktop--;
	}
	if (!gottoken)
	{
		RETURN_IF_FALSE(GetTokenNoMacro(parse, !parse.bStopAtEOL));
	}

	if (parse.token.tok == TOK_MACRO_OBJ || parse.token.tok == TOK_MACRO_FUNC)
	{
		SSYMBOL * symbol = parse.token.value.sym;
		ASSERT_RETFALSE(symbol);
		if (!symbol->replacement)
		{
			return GetToken(parse);
		}

		MACRONODE * macronode = (MACRONODE *)MALLOCZ(NULL, sizeof(MACRONODE));
		macronode->symbol = symbol;
		macronode->cursor = *symbol->replacement;
		macronode->cursor.cur = 0;

		if (parse.token.tok == TOK_MACRO_FUNC)
		{
			FUNCTION * function = symbol->function;
			ASSERT_RETFALSE(function);

			PARSENODE * sentry = ParseNodeNew(&parse.token);
			ASSERT_RETFALSE(sentry);
			ParseStackPush(parse.operatorstack, sentry);

			macronode->cursor = *function->stream;
			macronode->cursor.cur = 0;

			// get parameters
			NEXT();
			SASSERT_RETFALSE(parse.token.tok == TOK_LPAREN, "Expected left parentheses after macro name.");
			NEXT();

			while (parse.token.tok != TOK_RPAREN)
			{
				macronode->params = (BYTE_STREAM *)REALLOC(NULL, macronode->params, (macronode->paramcount+1) * sizeof(BYTE_STREAM));
				memclear(macronode->params + macronode->paramcount, sizeof(BYTE_STREAM));
				ByteStreamInit(NULL, macronode->params[macronode->paramcount]);

				ASSERT_RETFALSE(ReadExprStream(parse, macronode->params + macronode->paramcount));
				macronode->paramcount++;

				if (parse.token.tok == TOK_COMMA)
				{
					NEXT();
				}
			}

			while (TRUE)
			{
				PARSENODE * seek = ParseStackPop(parse.operatorstack);
				ASSERT_RETFALSE(seek);
				if (seek == sentry)
				{
					ParseNodeFree(seek);
					break;
				}
				ParseNodeFree(seek);
			}
		}

		if (parse.macrostack.stacktop >= parse.macrostack.stacksize)
		{
			parse.macrostack.stacksize++;
			parse.macrostack.stack = (MACRONODE**)REALLOC(NULL, parse.macrostack.stack, sizeof(MACRONODE*) * parse.macrostack.stacksize);
			ASSERT_RETFALSE(parse.macrostack.stack);
		}
		parse.macrostack.stack[parse.macrostack.stacktop++] = macronode;

		return GetToken(parse);
	}
	else if (parse.token.tok == TOK_MACRO_PARAM)
	{
		SSYMBOL * param = parse.token.value.sym;
		ASSERT_RETFALSE(param);
		ASSERT_RETFALSE(parse.macrostack.stacktop > 0);
		MACRONODE* macronode = parse.macrostack.stack[parse.macrostack.stacktop - 1];
		ASSERT_RETFALSE(macronode);
		FUNCTION * function= param->function;
		ASSERT_RETFALSE(function);
		int paramindex = -1;
		int parametercount = FunctionGetParameterCount(function);
		for (int ii = 0; ii < parametercount; ii++)
		{
			SSYMBOL * seek = FunctionGetParameter(function, ii);
			ASSERT_RETFALSE(seek);
			if (seek->key == param->key && PStrCmp(seek->str, param->str) == 0)
			{
				paramindex = ii;
				break;
			}
		}
		ASSERT_RETFALSE(paramindex >= 0);
		ASSERT_RETFALSE(paramindex < macronode->paramcount);

		MACRONODE* parammacro = (MACRONODE*)MALLOCZ(NULL, sizeof(MACRONODE));
		parammacro->symbol = param;
		parammacro->cursor = macronode->params[paramindex];
		parammacro->cursor.cur = 0;
		
		if (parse.macrostack.stacktop >= parse.macrostack.stacksize)
		{
			parse.macrostack.stacksize++;
			parse.macrostack.stack = (MACRONODE**)REALLOC(NULL, parse.macrostack.stack, sizeof(MACRONODE*) * parse.macrostack.stacksize);
			ASSERT_RETFALSE(parse.macrostack.stack);
		}
		parse.macrostack.stack[parse.macrostack.stacktop++] = parammacro;

		return GetToken(parse);
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PrintType(
	TYPE & type)
{
	if (type.type == 0)
	{
		return;
	}
	if (type.type & TYPE_BASICTYPE)
	{
		int basictype = TypeGetBasicType(type);
		if (basictype < arrsize(BasicTypeDef))
		{
			ScriptParseTrace("%s", BasicTypeDef[basictype].str);
		}
		else
		{
			ASSERT(0);
		}
	}
	else
	{
		ASSERT(0);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PrintToken(
	PARSE & parse,
	TOKEN * token,
	int offset = 0)
{
	if (token->type.type)
	{
		ScriptParseTrace("(");
		PrintType(token->type);
		ScriptParseTrace(") ");
	}

	switch (token->tok)
	{
	case TOK_NUM:
		ASSERT_RETURN(TypeIsConstant(token->type));
		switch (TypeGetBasicType(token->type))
		{
		case TYPE_CHAR:		ScriptParseTrace("%d\n", token->value.c);		break;
		case TYPE_BYTE:		ScriptParseTrace("%d\n", token->value.uc);		break;
		case TYPE_SHORT:	ScriptParseTrace("%d\n", token->value.s);		break;
		case TYPE_WORD:		ScriptParseTrace("%d\n", token->value.us);		break;
		case TYPE_INT:		ScriptParseTrace("%d\n", token->value.i);		break;
		case TYPE_DWORD:	ScriptParseTrace("%d\n", token->value.ui);		break;
		case TYPE_INT64:	ScriptParseTrace("%d\n", token->value.ll);		break;
		case TYPE_QWORD:	ScriptParseTrace("%d\n", token->value.ull);		break;
		case TYPE_FLOAT:	ScriptParseTrace("%.3f\n", token->value.f);		break;
		case TYPE_DOUBLE:	ScriptParseTrace("%.3f\n", token->value.d);		break;
		case TYPE_LDOUBLE:	ScriptParseTrace("%.3lf\n", token->value.ld);	break;
		default:			ASSERT_RETURN(0);
		}
		break;
	case TOK_IDENT:
		ASSERT(token->value.sym);
		if (token->tok < TOK_LAST)
		{
			ScriptParseTrace("%s  '%s'\n", TokenDef[token->tok].str, token->value.sym->str);
		}
		else
		{
			ScriptParseTrace("%s\n", token->value.sym->str);
		}
		break;
	case TOK_GLOBALVAR:
	case TOK_GLOBALADR:
	case TOK_LOCALVAR:
	case TOK_LOCALADR:
		ASSERT_BREAK(token->value.sym);
		if (token->tok < TOK_LAST)
		{
			ScriptParseTrace("%s  '%s'  @%d", TokenDef[token->tok].str, token->value.sym->str, token->value.sym->offset);
		}
		else
		{
			ScriptParseTrace("%s  @%d", token->value.sym->str, token->value.sym->offset);
		}
		if (offset)
		{
			ScriptParseTrace("  offset:%d", offset);			
		}
		ScriptParseTrace("\n");
		break;
	case TOK_FUNCTION:
	case TOK_IFUNCTION:
		{
			ASSERT_BREAK(token->value.sym);
			ScriptParseTrace("%s: %s(", TokenDef[token->tok].str, token->value.sym->str);
			FUNCTION * func = token->value.sym->function;
			ASSERT_BREAK(func);
			int param_count = FunctionGetParameterCount(func);
			for (int ii = 0; ii < param_count; ii++)
			{
				SSYMBOL * param = FunctionGetParameter(func, ii);
				ASSERT_BREAK(param);
				PrintType(param->type);
				ScriptParseTrace(" %s", param->str);
				if (ii < param_count - 1)
				{
					ScriptParseTrace(", ");
				}
			}
			ScriptParseTrace(")\n");
		}
		break;
	case TOK_PARAM:
		{
			ASSERT_BREAK(token->value.sym);
			ScriptParseTrace("%s: %s\n", TokenDef[token->tok].str, token->value.sym->str);
		}
		break;
	case TOK_FIELDSTR:
		{
			ASSERT_BREAK(token->value.str);
			ScriptParseTrace("%s: %s\n", TokenDef[token->tok].str, token->value.str);
		}
		break;
	case TOK_LSTAT:
	case TOK_RSTAT:
	case TOK_RSTAT_BASE:
	case TOK_RSTAT_MOD:
		{
			int stat = STAT_GET_STAT(token->value.i);
			const STATS_DATA * stat_data = StatsGetData(NULL, stat);
			ASSERT_BREAK(stat_data);
			ScriptParseTrace("%cstat: %s\n", (token->tok == TOK_LSTAT ? 'l' : 'r'), stat_data->m_szName);
		}
		break;
	case TOK_LSTAT_US:
	case TOK_RSTAT_US:
	case TOK_RSTAT_US_BASE:
	case TOK_RSTAT_US_MOD:
		{
			int stat = STAT_GET_STAT(token->value.i);
			const STATS_DATA * stat_data = StatsGetData(NULL, stat);
			ASSERT_BREAK(stat_data);
			ScriptParseTrace("%cstat: %s\n", (token->tok == TOK_LSTAT_US ? 'l' : 'r'), stat_data->m_szName);
		}
		break;
	case TOK_SPARAM:
		ScriptParseTrace("statparam: %d\n", token->value.i);
		break;
	case TOK_CONTEXTVAR:
		{
			int var = token->value.i;
			ASSERT_BREAK(var >= 0 && var < arrsize(ContextDef));
			ScriptParseTrace("context var: %s\n", ContextDef[var].str);
		}
		break;
	default:
		if (token->tok < TOK_LAST)
		{
			ScriptParseTrace("%s\n", TokenDef[token->tok].str);
			break;
		}
		else if (token->tok < TOK_LASTKEYWORD)
		{
			ScriptParseTrace("%s\n", KeywordStr[token->tok - TOK_LAST - 1]);
			break;
		}
		ASSERT(0);
		break;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PrintNode(
	PARSE & parse,
	PARSENODE * node)
{
	for (int ii = 0; ii < node->childcount; ii++)
	{
		PrintNode(parse, node->children[ii]);
	}
	PrintToken(parse, &node->token, node->offset);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PushOperand(
	PARSE & parse)
{
	PARSENODE * node = ParseNodeNew(&parse.token);
	ParseStackPush(parse.operandstack, node);
	parse.bLastSymbolWasTerm = TRUE;
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PushOperator(
	PARSE & parse)
{
	PARSENODE * node = ParseNodeNew(&parse.token);
	ParseStackPush(parse.operatorstack, node);
	parse.bLastSymbolWasTerm = FALSE;
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PushOperator(
	PARSE & parse,
	int paramcount)
{
	PARSENODE * node = ParseNodeNew(&parse.token);
	node->paramcount = paramcount;
	ParseStackPush(parse.operatorstack, node);
	parse.bLastSymbolWasTerm = FALSE;
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL TypeConversion(
	TOKEN & token,
	int type)
{
	ASSERT_RETFALSE(TypeIsConstant(token.type));

	int basictype = TypeGetBasicType(token.type);
	int desttype = type;
	if (desttype == basictype)
	{
		return TRUE;
	}
	switch (desttype)
	{
	case TYPE_CHAR:
		switch (basictype)
		{
		case TYPE_BYTE:		ASSERT_RETFALSE(token.value.uc <= SCHAR_MAX);
							token.type.type = (TYPE_CONSTANT | TYPE_CHAR); token.value.c = (char)token.value.uc; break;
		case TYPE_SHORT:	ASSERT_RETFALSE(token.value.s >= SCHAR_MIN && token.value.s <= SCHAR_MAX);
							token.type.type = (TYPE_CONSTANT | TYPE_CHAR); token.value.c = (char)token.value.s; break;
		case TYPE_WORD:		ASSERT_RETFALSE(token.value.us <= SCHAR_MAX);
							token.type.type = (TYPE_CONSTANT | TYPE_CHAR); token.value.c = (char)token.value.us; break;
		case TYPE_INT:		ASSERT_RETFALSE(token.value.i >= SCHAR_MIN && token.value.i <= SCHAR_MAX);
							token.type.type = (TYPE_CONSTANT | TYPE_CHAR); token.value.c = (char)token.value.i; break;
		case TYPE_DWORD:	ASSERT_RETFALSE(token.value.ui <= SCHAR_MAX);
							token.type.type = (TYPE_CONSTANT | TYPE_CHAR); token.value.c = (char)token.value.ui; break;
		case TYPE_INT64:	ASSERT_RETFALSE(token.value.ll >= SCHAR_MIN && token.value.ll <= SCHAR_MAX);
							token.type.type = (TYPE_CONSTANT | TYPE_CHAR); token.value.c = (char)token.value.ll; break;
		case TYPE_QWORD:	ASSERT_RETFALSE(token.value.ull <= SCHAR_MAX);
							token.type.type = (TYPE_CONSTANT | TYPE_CHAR); token.value.c = (char)token.value.ull; break;
		case TYPE_FLOAT:	ASSERT_RETFALSE(token.value.f >= SCHAR_MIN && token.value.f <= SCHAR_MAX);
							token.type.type = (TYPE_CONSTANT | TYPE_CHAR); token.value.c = (char)token.value.f; break;
		case TYPE_DOUBLE:	ASSERT_RETFALSE(token.value.d >= SCHAR_MIN && token.value.d <= SCHAR_MAX);
							token.type.type = (TYPE_CONSTANT | TYPE_CHAR); token.value.c = (char)token.value.d; break;
		case TYPE_LDOUBLE:	ASSERT_RETFALSE(token.value.ld >= SCHAR_MIN && token.value.ld <= SCHAR_MAX);
							token.type.type = (TYPE_CONSTANT | TYPE_CHAR); token.value.c = (char)token.value.ld; break;
		case TYPE_BOOL:
		case TYPE_ENUM:		ASSERT_RETFALSE(token.value.i >= SCHAR_MIN && token.value.i <= SCHAR_MAX);
							token.type.type = (TYPE_CONSTANT | TYPE_CHAR); token.value.c = (char)token.value.i; break;
		default:			ASSERT_RETFALSE(0);		
		}
		break;
	case TYPE_BYTE:
		switch (basictype)
		{
		case TYPE_CHAR:		ASSERT_RETFALSE(token.value.c >= 0);
							token.type.type = (TYPE_CONSTANT | TYPE_BYTE); token.value.uc = (unsigned char)token.value.c; break;
		case TYPE_SHORT:	ASSERT_RETFALSE(token.value.s >= 0 && token.value.s <= UCHAR_MAX);
							token.type.type = (TYPE_CONSTANT | TYPE_BYTE); token.value.uc = (unsigned char)token.value.s; break;
		case TYPE_WORD:		ASSERT_RETFALSE(token.value.us <= UCHAR_MAX);
							token.type.type = (TYPE_CONSTANT | TYPE_BYTE); token.value.uc = (unsigned char)token.value.us; break;
		case TYPE_INT:		ASSERT_RETFALSE(token.value.i >= 0 && token.value.i <= UCHAR_MAX);
							token.type.type = (TYPE_CONSTANT | TYPE_BYTE); token.value.uc = (unsigned char)token.value.i; break;
		case TYPE_DWORD:	ASSERT_RETFALSE(token.value.ui <= UCHAR_MAX);
							token.type.type = (TYPE_CONSTANT | TYPE_BYTE); token.value.uc = (unsigned char)token.value.ui; break;
		case TYPE_INT64:	ASSERT_RETFALSE(token.value.ll >= 0 && token.value.ll <= UCHAR_MAX);
							token.type.type = (TYPE_CONSTANT | TYPE_BYTE); token.value.uc = (unsigned char)token.value.ll; break;
		case TYPE_QWORD:	ASSERT_RETFALSE(token.value.ull <= UCHAR_MAX);
							token.type.type = (TYPE_CONSTANT | TYPE_BYTE); token.value.uc = (unsigned char)token.value.ull; break;
		case TYPE_FLOAT:	ASSERT_RETFALSE(token.value.f >= 0 && token.value.f <= UCHAR_MAX);
							token.type.type = (TYPE_CONSTANT | TYPE_BYTE); token.value.uc = (unsigned char)token.value.f; break;
		case TYPE_DOUBLE:	ASSERT_RETFALSE(token.value.d >= 0 && token.value.d <= UCHAR_MAX);
							token.type.type = (TYPE_CONSTANT | TYPE_BYTE); token.value.uc = (unsigned char)token.value.d; break;
		case TYPE_LDOUBLE:	ASSERT_RETFALSE(token.value.ld >= 0 && token.value.ld <= UCHAR_MAX);
							token.type.type = (TYPE_CONSTANT | TYPE_BYTE); token.value.uc = (unsigned char)token.value.ld; break;
		case TYPE_BOOL:
		case TYPE_ENUM:		ASSERT_RETFALSE(token.value.i >= 0 && token.value.i <= UCHAR_MAX);
							token.type.type = (TYPE_CONSTANT | TYPE_BYTE); token.value.uc = (unsigned char)token.value.i; break;
		default:			ASSERT_RETFALSE(0);		
		}
		break;
	case TYPE_SHORT:
		switch (basictype)
		{
		case TYPE_CHAR:		token.type.type = (TYPE_CONSTANT | TYPE_SHORT); token.value.s = (short)token.value.c; break;
		case TYPE_BYTE:		token.type.type = (TYPE_CONSTANT | TYPE_SHORT); token.value.s = (short)token.value.uc; break;
		case TYPE_WORD:		ASSERT_RETFALSE(token.value.us <= SHRT_MAX);
							token.type.type = (TYPE_CONSTANT | TYPE_SHORT); token.value.s = (short)token.value.us; break;
		case TYPE_INT:		ASSERT_RETFALSE(token.value.i >= SHRT_MIN && token.value.i <= SHRT_MAX);
							token.type.type = (TYPE_CONSTANT | TYPE_SHORT); token.value.s = (short)token.value.i; break;
		case TYPE_DWORD:	ASSERT_RETFALSE(token.value.ui <= SHRT_MAX);
							token.type.type = (TYPE_CONSTANT | TYPE_SHORT); token.value.s = (short)token.value.ui; break;
		case TYPE_INT64:	ASSERT_RETFALSE(token.value.ll >= SHRT_MIN && token.value.ll <= SHRT_MAX);
							token.type.type = (TYPE_CONSTANT | TYPE_SHORT); token.value.s = (short)token.value.ll; break;
		case TYPE_QWORD:	ASSERT_RETFALSE(token.value.ull <= SHRT_MAX);
							token.type.type = (TYPE_CONSTANT | TYPE_SHORT); token.value.s = (short)token.value.ull; break;
		case TYPE_FLOAT:	ASSERT_RETFALSE(token.value.f >= SHRT_MIN && token.value.f <= SHRT_MAX);
							token.type.type = (TYPE_CONSTANT | TYPE_SHORT); token.value.s = (short)token.value.f; break;
		case TYPE_DOUBLE:	ASSERT_RETFALSE(token.value.d >= SHRT_MIN && token.value.d <= SHRT_MAX);
							token.type.type = (TYPE_CONSTANT | TYPE_SHORT); token.value.s = (short)token.value.d; break;
		case TYPE_LDOUBLE:	ASSERT_RETFALSE(token.value.ld >= SHRT_MIN && token.value.ld <= SHRT_MAX);
							token.type.type = (TYPE_CONSTANT | TYPE_SHORT); token.value.s = (short)token.value.ld; break;
		case TYPE_BOOL:
		case TYPE_ENUM:		ASSERT_RETFALSE(token.value.i >= SHRT_MIN && token.value.i <= SHRT_MAX);
							token.type.type = (TYPE_CONSTANT | TYPE_SHORT); token.value.s = (short)token.value.i; break;
		default:			ASSERT_RETFALSE(0);		
		}
		break;
	case TYPE_WORD:
		switch (basictype)
		{
		case TYPE_CHAR:		ASSERT_RETFALSE(token.value.c >= 0);
							token.type.type = (TYPE_CONSTANT | TYPE_WORD); token.value.us = (unsigned short)token.value.c; break;
		case TYPE_BYTE:		token.type.type = (TYPE_CONSTANT | TYPE_WORD); token.value.us = (unsigned short)token.value.uc; break;
		case TYPE_SHORT:	ASSERT_RETFALSE(token.value.us >= 0);
							token.type.type = (TYPE_CONSTANT | TYPE_WORD); token.value.us = (unsigned short)token.value.s; break;
		case TYPE_INT:		ASSERT_RETFALSE(token.value.i >= 0 && token.value.i <= USHRT_MAX);
							token.type.type = (TYPE_CONSTANT | TYPE_WORD); token.value.us = (unsigned short)token.value.i; break;
		case TYPE_DWORD:	ASSERT_RETFALSE(token.value.ui <= USHRT_MAX);
							token.type.type = (TYPE_CONSTANT | TYPE_WORD); token.value.us = (unsigned short)token.value.ui; break;
		case TYPE_INT64:	ASSERT_RETFALSE(token.value.ll >= 0 && token.value.ll <= USHRT_MAX);
							token.type.type = (TYPE_CONSTANT | TYPE_WORD); token.value.us = (unsigned short)token.value.ll; break;
		case TYPE_QWORD:	ASSERT_RETFALSE(token.value.ull <= USHRT_MAX);
							token.type.type = (TYPE_CONSTANT | TYPE_WORD); token.value.us = (unsigned short)token.value.ull; break;
		case TYPE_FLOAT:	ASSERT_RETFALSE(token.value.f >= 0 && token.value.f <= USHRT_MAX);
							token.type.type = (TYPE_CONSTANT | TYPE_WORD); token.value.us = (unsigned short)token.value.f; break;
		case TYPE_DOUBLE:	ASSERT_RETFALSE(token.value.d >= 0 && token.value.d <= USHRT_MAX);
							token.type.type = (TYPE_CONSTANT | TYPE_WORD); token.value.us = (unsigned short)token.value.d; break;
		case TYPE_LDOUBLE:	ASSERT_RETFALSE(token.value.ld >= 0 && token.value.ld <= USHRT_MAX);
							token.type.type = (TYPE_CONSTANT | TYPE_WORD); token.value.us = (unsigned short)token.value.ld; break;
		case TYPE_BOOL:
		case TYPE_ENUM:		ASSERT_RETFALSE(token.value.i >= 0 && token.value.i <= USHRT_MAX);
							token.type.type = (TYPE_CONSTANT | TYPE_WORD); token.value.us = (unsigned short)token.value.i; break;
		default:			ASSERT_RETFALSE(0);		
		}
		break;
	case TYPE_INT:
		switch (basictype)
		{
		case TYPE_CHAR:		token.type.type = (TYPE_CONSTANT | TYPE_INT); token.value.i = (int)token.value.c; break;
		case TYPE_BYTE:		token.type.type = (TYPE_CONSTANT | TYPE_INT); token.value.i = (int)token.value.uc; break;
		case TYPE_SHORT:	token.type.type = (TYPE_CONSTANT | TYPE_INT); token.value.i = (int)token.value.s; break;
		case TYPE_WORD:		token.type.type = (TYPE_CONSTANT | TYPE_INT); token.value.i = (int)token.value.us; break;
		case TYPE_DWORD:	ASSERT_RETFALSE(token.value.ui <= INT_MAX);
							token.type.type = (TYPE_CONSTANT | TYPE_INT); token.value.i = (int)token.value.ui; break;
		case TYPE_INT64:	ASSERT_RETFALSE(token.value.ll >= INT_MIN && token.value.ll <= INT_MAX);
							token.type.type = (TYPE_CONSTANT | TYPE_INT); token.value.i = (int)token.value.ll; break;
		case TYPE_QWORD:	ASSERT_RETFALSE(token.value.ull <= INT_MAX);
							token.type.type = (TYPE_CONSTANT | TYPE_INT); token.value.i = (int)token.value.ull; break;
		case TYPE_FLOAT:	ASSERT_RETFALSE(token.value.f >= INT_MIN && token.value.f <= INT_MAX);
							token.type.type = (TYPE_CONSTANT | TYPE_INT); token.value.i = (int)token.value.f; break;
		case TYPE_DOUBLE:	ASSERT_RETFALSE(token.value.d >= INT_MIN && token.value.d <= INT_MAX);
							token.type.type = (TYPE_CONSTANT | TYPE_INT); token.value.i = (int)token.value.d; break;
		case TYPE_LDOUBLE:	ASSERT_RETFALSE(token.value.ld >= INT_MIN && token.value.ld <= INT_MAX);
							token.type.type = (TYPE_CONSTANT | TYPE_INT); token.value.i = (int)token.value.ld; break;
		case TYPE_BOOL:
		case TYPE_ENUM:		token.type.type = (TYPE_CONSTANT | TYPE_INT); break;
		default:			ASSERT_RETFALSE(0);		
		}
		break;
	case TYPE_DWORD:
		switch (basictype)
		{
		case TYPE_CHAR:		ASSERT_RETFALSE(token.value.c >= 0);
							token.type.type = (TYPE_CONSTANT | TYPE_DWORD); token.value.ui = (unsigned int)token.value.c; break;
		case TYPE_BYTE:		token.type.type = (TYPE_CONSTANT | TYPE_DWORD); token.value.ui = (unsigned int)token.value.uc; break;
		case TYPE_SHORT:	ASSERT_RETFALSE(token.value.s >= 0);
							token.type.type = (TYPE_CONSTANT | TYPE_DWORD); token.value.ui = (unsigned int)token.value.s; break;
		case TYPE_WORD:		token.type.type = (TYPE_CONSTANT | TYPE_DWORD); token.value.ui = (unsigned int)token.value.us; break;
		case TYPE_INT:		ASSERT_RETFALSE(token.value.i >= 0);
							token.type.type = (TYPE_CONSTANT | TYPE_DWORD); token.value.ui = (unsigned int)token.value.i; break;
		case TYPE_INT64:	ASSERT_RETFALSE(token.value.ll >= 0 && token.value.ll <= UINT_MAX);
							token.type.type = (TYPE_CONSTANT | TYPE_DWORD); token.value.ui = (unsigned int)token.value.ll; break;
		case TYPE_QWORD:	ASSERT_RETFALSE(token.value.ull <= UINT_MAX);
							token.type.type = (TYPE_CONSTANT | TYPE_DWORD); token.value.ui = (unsigned int)token.value.ull; break;
		case TYPE_FLOAT:	ASSERT_RETFALSE(token.value.f >= 0 && token.value.f <= UINT_MAX);
							token.type.type = (TYPE_CONSTANT | TYPE_DWORD); token.value.ui = (unsigned int)token.value.f; break;
		case TYPE_DOUBLE:	ASSERT_RETFALSE(token.value.d >= 0 && token.value.d <= UINT_MAX);
							token.type.type = (TYPE_CONSTANT | TYPE_DWORD); token.value.ui = (unsigned int)token.value.d; break;
		case TYPE_LDOUBLE:	ASSERT_RETFALSE(token.value.ld >= 0 && token.value.ld <= UINT_MAX);
							token.type.type = (TYPE_CONSTANT | TYPE_DWORD); token.value.ui = (unsigned int)token.value.ld; break;
		case TYPE_BOOL:
		case TYPE_ENUM:		ASSERT_RETFALSE(token.value.i >= 0);
							token.type.type = (TYPE_CONSTANT | TYPE_DWORD); token.value.ui = (unsigned int)token.value.i; break;
		default:			ASSERT_RETFALSE(0);		
		}
		break;
	case TYPE_INT64:
		switch (basictype)
		{
		case TYPE_CHAR:		token.type.type = (TYPE_CONSTANT | TYPE_INT64); token.value.ll = (INT64)token.value.c; break;
		case TYPE_BYTE:		token.type.type = (TYPE_CONSTANT | TYPE_INT64); token.value.ll = (INT64)token.value.uc; break;
		case TYPE_SHORT:	token.type.type = (TYPE_CONSTANT | TYPE_INT64); token.value.ll = (INT64)token.value.s; break;
		case TYPE_WORD:		token.type.type = (TYPE_CONSTANT | TYPE_INT64); token.value.ll = (INT64)token.value.us; break;
		case TYPE_INT:		token.type.type = (TYPE_CONSTANT | TYPE_INT64); token.value.ll = (INT64)token.value.i; break;
		case TYPE_DWORD:	token.type.type = (TYPE_CONSTANT | TYPE_INT64); token.value.ll = (INT64)token.value.ui; break;
		case TYPE_QWORD:	ASSERT_RETFALSE((token.value.ull >> 63) == 0);
							token.type.type = (TYPE_CONSTANT | TYPE_INT64); token.value.ll = (INT64)token.value.ull; break;
		case TYPE_FLOAT:	token.type.type = (TYPE_CONSTANT | TYPE_INT64); token.value.ll = (INT64)token.value.f; break;
		case TYPE_DOUBLE:	token.type.type = (TYPE_CONSTANT | TYPE_INT64); token.value.ll = (INT64)token.value.d; break;
		case TYPE_LDOUBLE:	token.type.type = (TYPE_CONSTANT | TYPE_INT64); token.value.ll = (INT64)token.value.ld; break;
		case TYPE_BOOL:
		case TYPE_ENUM:		token.type.type = (TYPE_CONSTANT | TYPE_INT64); token.value.ll = (INT64)token.value.i; break;
		default:			ASSERT_RETFALSE(0);		
		}
		break;
	case TYPE_QWORD:
		switch (basictype)
		{
		case TYPE_CHAR:		ASSERT_RETFALSE(token.value.c >= 0);
							token.type.type = (TYPE_CONSTANT | TYPE_QWORD); token.value.ull = (UINT64)token.value.c; break;
		case TYPE_BYTE:		token.type.type = (TYPE_CONSTANT | TYPE_QWORD); token.value.ull = (UINT64)token.value.uc; break;
		case TYPE_SHORT:	ASSERT_RETFALSE(token.value.s >= 0);
							token.type.type = (TYPE_CONSTANT | TYPE_QWORD); token.value.ull = (UINT64)token.value.s; break;
		case TYPE_WORD:		token.type.type = (TYPE_CONSTANT | TYPE_QWORD); token.value.ull = (UINT64)token.value.us; break;
		case TYPE_INT:		ASSERT_RETFALSE(token.value.i >= 0);
							token.type.type = (TYPE_CONSTANT | TYPE_QWORD); token.value.ull = (UINT64)token.value.i; break;
		case TYPE_DWORD:	token.type.type = (TYPE_CONSTANT | TYPE_QWORD); token.value.ull = (UINT64)token.value.ui; break;
		case TYPE_INT64:	ASSERT_RETFALSE(token.value.ll >= 0);
							token.type.type = (TYPE_CONSTANT | TYPE_QWORD); token.value.ull = (UINT64)token.value.ll; break;
		case TYPE_FLOAT:	ASSERT_RETFALSE(token.value.f >= 0.0f);
							token.type.type = (TYPE_CONSTANT | TYPE_QWORD); token.value.ull = (UINT64)token.value.f; break;
		case TYPE_DOUBLE:	ASSERT_RETFALSE(token.value.d >= 0.0f);
							token.type.type = (TYPE_CONSTANT | TYPE_QWORD); token.value.ull = (UINT64)token.value.d; break;
		case TYPE_LDOUBLE:	ASSERT_RETFALSE(token.value.ld >= 0.0f);
							token.type.type = (TYPE_CONSTANT | TYPE_QWORD); token.value.ull = (UINT64)token.value.ld; break;
		case TYPE_BOOL:
		case TYPE_ENUM:		ASSERT_RETFALSE(token.value.i >= 0);
							token.type.type = (TYPE_CONSTANT | TYPE_QWORD); token.value.ull = (UINT64)token.value.i; break;
		default:			ASSERT_RETFALSE(0);		
		}
		break;
	case TYPE_FLOAT:
		switch (basictype)
		{
		case TYPE_CHAR:		token.type.type = (TYPE_CONSTANT | TYPE_FLOAT); token.value.f = (float)token.value.c; break;
		case TYPE_BYTE:		token.type.type = (TYPE_CONSTANT | TYPE_FLOAT); token.value.f = (float)token.value.uc; break;
		case TYPE_SHORT:	token.type.type = (TYPE_CONSTANT | TYPE_FLOAT); token.value.f = (float)token.value.s; break;
		case TYPE_WORD:		token.type.type = (TYPE_CONSTANT | TYPE_FLOAT); token.value.f = (float)token.value.us; break;
		case TYPE_INT:		token.type.type = (TYPE_CONSTANT | TYPE_FLOAT); token.value.f = (float)token.value.i; break;
		case TYPE_DWORD:	token.type.type = (TYPE_CONSTANT | TYPE_FLOAT); token.value.f = (float)token.value.ui; break;
		case TYPE_INT64:	token.type.type = (TYPE_CONSTANT | TYPE_FLOAT); token.value.f = (float)token.value.ll; break;
		case TYPE_QWORD:	token.type.type = (TYPE_CONSTANT | TYPE_FLOAT); token.value.f = (float)token.value.ull; break;
		case TYPE_DOUBLE:	token.type.type = (TYPE_CONSTANT | TYPE_FLOAT); token.value.f = (float)token.value.d; break;
		case TYPE_LDOUBLE:	token.type.type = (TYPE_CONSTANT | TYPE_FLOAT); token.value.f = (float)token.value.ld; break;
		case TYPE_BOOL:
		case TYPE_ENUM:		token.type.type = (TYPE_CONSTANT | TYPE_FLOAT); token.value.f = (float)token.value.i; break;
		default:			ASSERT_RETFALSE(0);		
		}
		break;
	case TYPE_DOUBLE:
		switch (basictype)
		{
		case TYPE_CHAR:		token.type.type = (TYPE_CONSTANT | TYPE_DOUBLE); token.value.d = (double)token.value.c; break;
		case TYPE_BYTE:		token.type.type = (TYPE_CONSTANT | TYPE_DOUBLE); token.value.d = (double)token.value.uc; break;
		case TYPE_SHORT:	token.type.type = (TYPE_CONSTANT | TYPE_DOUBLE); token.value.d = (double)token.value.s; break;
		case TYPE_WORD:		token.type.type = (TYPE_CONSTANT | TYPE_DOUBLE); token.value.d = (double)token.value.us; break;
		case TYPE_INT:		token.type.type = (TYPE_CONSTANT | TYPE_DOUBLE); token.value.d = (double)token.value.i; break;
		case TYPE_DWORD:	token.type.type = (TYPE_CONSTANT | TYPE_DOUBLE); token.value.d = (double)token.value.ui; break;
		case TYPE_INT64:	token.type.type = (TYPE_CONSTANT | TYPE_DOUBLE); token.value.d = (double)token.value.ll; break;
		case TYPE_QWORD:	token.type.type = (TYPE_CONSTANT | TYPE_DOUBLE); token.value.d = (double)token.value.ull; break;
		case TYPE_FLOAT:	token.type.type = (TYPE_CONSTANT | TYPE_DOUBLE); token.value.d = (double)token.value.f; break;
		case TYPE_LDOUBLE:	token.type.type = (TYPE_CONSTANT | TYPE_DOUBLE); token.value.d = (double)token.value.ld; break;
		case TYPE_BOOL:
		case TYPE_ENUM:		token.type.type = (TYPE_CONSTANT | TYPE_DOUBLE); token.value.d = (double)token.value.i; break;
		default:			ASSERT_RETFALSE(0);		
		}
		break;
	case TYPE_LDOUBLE:
		switch (basictype)
		{
		case TYPE_CHAR:		token.type.type = (TYPE_CONSTANT | TYPE_LDOUBLE); token.value.ld = (long double)token.value.c; break;
		case TYPE_BYTE:		token.type.type = (TYPE_CONSTANT | TYPE_LDOUBLE); token.value.ld = (long double)token.value.uc; break;
		case TYPE_SHORT:	token.type.type = (TYPE_CONSTANT | TYPE_LDOUBLE); token.value.ld = (long double)token.value.s; break;
		case TYPE_WORD:		token.type.type = (TYPE_CONSTANT | TYPE_LDOUBLE); token.value.ld = (long double)token.value.us; break;
		case TYPE_INT:		token.type.type = (TYPE_CONSTANT | TYPE_LDOUBLE); token.value.ld = (long double)token.value.i; break;
		case TYPE_DWORD:	token.type.type = (TYPE_CONSTANT | TYPE_LDOUBLE); token.value.ld = (long double)token.value.ui; break;
		case TYPE_INT64:	token.type.type = (TYPE_CONSTANT | TYPE_LDOUBLE); token.value.ld = (long double)token.value.ll; break;
		case TYPE_QWORD:	token.type.type = (TYPE_CONSTANT | TYPE_LDOUBLE); token.value.ld = (long double)token.value.ull; break;
		case TYPE_FLOAT:	token.type.type = (TYPE_CONSTANT | TYPE_LDOUBLE); token.value.ld = (long double)token.value.f; break;
		case TYPE_DOUBLE:	token.type.type = (TYPE_CONSTANT | TYPE_LDOUBLE); token.value.ld = (long double)token.value.d; break;
		case TYPE_BOOL:
		case TYPE_ENUM:		token.type.type = (TYPE_CONSTANT | TYPE_LDOUBLE); token.value.ld = (long double)token.value.i; break;
		default:			ASSERT_RETFALSE(0);		
		}
		break;
	default:
		ASSERT_RETFALSE(0);		
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL TypeConversion(
	PARSENODE *& node,
	TYPE & type)
{
	int basictype = TypeGetBasicType(type);
	if (TypeGetBasicType(node->token.type) == basictype)
	{
		if (basictype != TYPE_PTR)
		{
			return TRUE;
		}
	}
	if (TypeIsConstant(node->token.type))
	{
		return TypeConversion(node->token, TypeGetBasicType(type));
	}
	else
	{
		ASSERT_RETFALSE(type.type < TYPE_PTR);
		// apply typecast
		TOKEN cast_token;
		structclear(cast_token);
		cast_token.tok = TOK_TYPECAST;
		cast_token.type = type;
		PARSENODE * cast_node = ParseNodeNew(&cast_token);
		ParseNodeAddChild(cast_node, node);
		node = cast_node;
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL TypePromotion(
	TOKEN & op,
	PARSENODE *& a,
	PARSENODE *& b,
	TYPE & type)
{
	type.sym = NULL;

	int intop = TokenGetIntegerOp(op);
	int a_type = TypeGetBasicType(a->token.type);
	int b_type = TypeGetBasicType(b->token.type);

	if (!a_type)
	{
		return FALSE;
	}
	if (!b_type)
	{
		return FALSE;
	}
	if (a_type == TYPE_LDOUBLE || b_type == TYPE_LDOUBLE)
	{
		if (intop)
		{
			type.type = TYPE_INT64;
		}
		else
		{
			type.type = TYPE_LDOUBLE;
		}
	}
	else if (!intop && a_type == TYPE_DOUBLE || b_type == TYPE_DOUBLE)
	{
		if (intop)
		{
			type.type = TYPE_INT64;
		}
		else
		{
			type.type = TYPE_DOUBLE;
		}
	}
	else if (!intop && a_type == TYPE_FLOAT || b_type == TYPE_FLOAT)
	{
		if (intop)
		{
			type.type = TYPE_INT;
		}
		else
		{
			type.type = TYPE_FLOAT;
		}
	}
	else if (a_type == TYPE_QWORD || b_type == TYPE_QWORD)
	{
		type.type = TYPE_QWORD;
	}
	else if ((a_type == TYPE_INT64 && b_type == TYPE_DWORD) || 
		(b_type == TYPE_INT64 && a_type == TYPE_DWORD))
	{
		type.type = TYPE_QWORD;
	}
	else if (a_type == TYPE_INT64 || b_type == TYPE_INT64)
	{
		type.type = TYPE_INT64;
	}
	else if (a_type == TYPE_DWORD || b_type == TYPE_DWORD)
	{
		type.type = TYPE_DWORD;
	}
	else
	{
		type.type = TYPE_INT;
	}

	if (a_type != type.type)
	{
		RETURN_IF_FALSE(TypeConversion(a, type));
	}
	if (b_type != type.type)
	{
		RETURN_IF_FALSE(TypeConversion(b, type));
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL IsZero(
	TOKEN & token)
{
	if (!TypeIsConstant(token.type))
	{
		return FALSE;
	}
	int basictype = TypeGetBasicType(token.type);
	if (!basictype)
	{
		return FALSE;
	}
	switch (basictype)
	{
	case TYPE_CHAR:			return (token.value.c == 0);
	case TYPE_BYTE:			return (token.value.uc == 0);
	case TYPE_SHORT:		return (token.value.s == 0);
	case TYPE_WORD:			return (token.value.us == 0);
	case TYPE_INT:			return (token.value.i == 0);
	case TYPE_DWORD:		return (token.value.ui == 0);
	case TYPE_INT64:		return (token.value.ll == 0L);
	case TYPE_QWORD:		return (token.value.ll == 0UL);
	case TYPE_FLOAT:		return (token.value.f == 0.0f);
	case TYPE_DOUBLE:		return (token.value.f == 0.0);
	case TYPE_LDOUBLE:		return (token.value.f == 0.0l);
	case TYPE_BOOL:
	case TYPE_ENUM:			return (token.value.i == 0);
	default:				ASSERT_RETFALSE(0);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL IsOne(
	TOKEN & token)
{
	if (!TypeIsConstant(token.type))
	{
		return FALSE;
	}
	int basictype = TypeGetBasicType(token.type);
	if (!basictype)
	{
		return FALSE;
	}
	switch (basictype)
	{
	case TYPE_CHAR:			return (token.value.c == 1);
	case TYPE_BYTE:			return (token.value.uc == 1);
	case TYPE_SHORT:		return (token.value.s == 1);
	case TYPE_WORD:			return (token.value.us == 1);
	case TYPE_INT:			return (token.value.i == 1);
	case TYPE_DWORD:		return (token.value.ui == 1);
	case TYPE_INT64:		return (token.value.ll == 1L);
	case TYPE_QWORD:		return (token.value.ll == 1UL);
	case TYPE_FLOAT:		return (token.value.f == 1.0f);
	case TYPE_DOUBLE:		return (token.value.f == 1.0);
	case TYPE_LDOUBLE:		return (token.value.f == 1.0l);
	case TYPE_BOOL:
	case TYPE_ENUM:			return (token.value.i == 1);
	default:				ASSERT_RETFALSE(0);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SetZero(
	TOKEN & token)
{
	if (!TypeIsConstant(token.type))
	{
		return FALSE;
	}
	int basictype = TypeGetBasicType(token.type);
	if (!basictype)
	{
		return FALSE;
	}
	switch (basictype)
	{
	case TYPE_CHAR:			token.value.c = 0; break;
	case TYPE_BYTE:			token.value.uc = 0; break;
	case TYPE_SHORT:		token.value.s = 0; break;
	case TYPE_WORD:			token.value.us = 0; break;
	case TYPE_INT:			token.value.i = 0; break;
	case TYPE_DWORD:		token.value.ui = 0; break;
	case TYPE_INT64:		token.value.ll = 0L; break;
	case TYPE_QWORD:		token.value.ll = 0UL; break;
	case TYPE_FLOAT:		token.value.f = 0.0f; break;
	case TYPE_DOUBLE:		token.value.f = 0.0; break;
	case TYPE_LDOUBLE:		token.value.f = 0.0l; break;
	case TYPE_BOOL:
	case TYPE_ENUM:			token.value.i = 0; break;
	default:				ASSERT_RETFALSE(0);
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SetOne(
	TOKEN & token)
{
	if (!TypeIsConstant(token.type))
	{
		return FALSE;
	}
	int basictype = TypeGetBasicType(token.type);
	if (!basictype)
	{
		return FALSE;
	}
	switch (basictype)
	{
	case TYPE_CHAR:			token.value.c = 1; break;
	case TYPE_BYTE:			token.value.uc = 1; break;
	case TYPE_SHORT:		token.value.s = 1; break;
	case TYPE_WORD:			token.value.us = 1; break;
	case TYPE_INT:			token.value.i = 1; break;
	case TYPE_DWORD:		token.value.ui = 1; break;
	case TYPE_INT64:		token.value.ll = 1L; break;
	case TYPE_QWORD:		token.value.ll = 1UL; break;
	case TYPE_FLOAT:		token.value.f = 1.0f; break;
	case TYPE_DOUBLE:		token.value.f = 1.0; break;
	case TYPE_LDOUBLE:		token.value.f = 1.0l; break;
	case TYPE_BOOL:
	case TYPE_ENUM:			token.value.i = 1; break;
	default:				ASSERT_RETFALSE(0);
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void OpTilde_CH(VALUE & a)					{ a.c = ~a.c; };
static inline void OpTilde_UC(VALUE & a)					{ a.uc = ~a.uc; };
static inline void OpTilde_SH(VALUE & a)					{ a.s = ~a.s; };
static inline void OpTilde_US(VALUE & a)					{ a.us = ~a.us; };
static inline void OpTilde_IN(VALUE & a)					{ a.i = ~a.i; };
static inline void OpTilde_UI(VALUE & a)					{ a.ui = ~a.ui; };
static inline void OpTilde_LL(VALUE & a)					{ a.ll = ~a.ll; };
static inline void OpTilde_UL(VALUE & a)					{ a.ull = ~a.ull; };

static inline void OpNot_CH(VALUE & a)						{ a.c = !a.c; };
static inline void OpNot_UC(VALUE & a)						{ a.uc = !a.uc; };
static inline void OpNot_SH(VALUE & a)						{ a.s = !a.s; };
static inline void OpNot_US(VALUE & a)						{ a.us = !a.us; };
static inline void OpNot_IN(VALUE & a)						{ a.i = !a.i; };
static inline void OpNot_UI(VALUE & a)						{ a.ui = !a.ui; };
static inline void OpNot_LL(VALUE & a)						{ a.ll = !a.ll; };
static inline void OpNot_UL(VALUE & a)						{ a.ull = !a.ull; };

static inline void OpNeg_CH(VALUE & a)						{ a.c = -a.c; };
static inline void OpNeg_UC(VALUE & a)						{ /* should warn */ };
static inline void OpNeg_SH(VALUE & a)						{ a.s = -a.s; };
static inline void OpNeg_US(VALUE & a)						{ /* should warn */ };
static inline void OpNeg_IN(VALUE & a)						{ a.i = -a.i; };
static inline void OpNeg_UI(VALUE & a)						{ /* should warn */ };
static inline void OpNeg_LL(VALUE & a)						{ a.ll = -a.ll; };
static inline void OpNeg_UL(VALUE & a)						{ /* should warn */ };
static inline void OpNeg_FL(VALUE & a)						{ a.f = -a.f; };
static inline void OpNeg_DB(VALUE & a)						{ a.d = -a.d; };
static inline void OpNeg_LD(VALUE & a)						{ a.ld = -a.ld; };

static inline void OpPow_CH(VALUE & a, VALUE & b)			{ a.c = (char)((int)POWI((int)a.c, (int)b.c)); };
static inline void OpPow_UC(VALUE & a, VALUE & b)			{ a.uc = (unsigned char)((int)POWI((int)a.uc, (int)b.uc)); };
static inline void OpPow_SH(VALUE & a, VALUE & b)			{ a.s = (short)((int)POWI((int)a.s, (int)b.s)); };
static inline void OpPow_US(VALUE & a, VALUE & b)			{ a.us = (unsigned short)((int)POWI((int)a.us, (int)b.us)); };
static inline void OpPow_IN(VALUE & a, VALUE & b)			{ a.i = (int)POWI(a.i, b.i); };
static inline void OpPow_UI(VALUE & a, VALUE & b)			{ a.ui = (unsigned int)POWI(a.ui, b.ui); };
static inline void OpPow_LL(VALUE & a, VALUE & b)			{ a.ll = (INT64)POWI(a.ll, b.ll); };
static inline void OpPow_UL(VALUE & a, VALUE & b)			{ a.ull = (UINT64)POWI(a.ull, b.ull); };
static inline void OpPow_FL(VALUE & a, VALUE & b)			{ a.f = powf(a.f, b.f); };
static inline void OpPow_DB(VALUE & a, VALUE & b)			{ a.d = pow(a.d, b.d); };
static inline void OpPow_LD(VALUE & a, VALUE & b)			{ a.ld = pow(a.ld, b.ld); };

static inline void OpMul_CH(VALUE & a, VALUE & b)			{ a.c = (char)(a.c * b.c); };
static inline void OpMul_UC(VALUE & a, VALUE & b)			{ a.uc = (unsigned char)(a.uc * b.uc); };
static inline void OpMul_SH(VALUE & a, VALUE & b)			{ a.s = (short)(a.s * b.s); };
static inline void OpMul_US(VALUE & a, VALUE & b)			{ a.us = (unsigned short)(a.us * b.us); };
static inline void OpMul_IN(VALUE & a, VALUE & b)			{ a.i *= b.i; };
static inline void OpMul_UI(VALUE & a, VALUE & b)			{ a.ui *= b.ui; };
static inline void OpMul_LL(VALUE & a, VALUE & b)			{ a.ll *= b.ll; };
static inline void OpMul_UL(VALUE & a, VALUE & b)			{ a.ull *= b.ull; };
static inline void OpMul_FL(VALUE & a, VALUE & b)			{ a.f *= b.f; };
static inline void OpMul_DB(VALUE & a, VALUE & b)			{ a.d *= b.d; };
static inline void OpMul_LD(VALUE & a, VALUE & b)			{ a.ld *= b.ld; };

static inline void OpDiv_CH(VALUE & a, VALUE & b)			{ ASSERT(b.c != 0); a.c = ((b.c == 0) ? 0 : (char)(a.c / b.c)); };
static inline void OpDiv_UC(VALUE & a, VALUE & b)			{ ASSERT(b.uc != 0); a.uc = ((b.uc == 0) ? 0 : (unsigned char)(a.uc / b.uc)); };
static inline void OpDiv_SH(VALUE & a, VALUE & b)			{ ASSERT(b.s != 0); a.s = ((b.s == 0) ? 0 : (short)(a.s / b.s)); };
static inline void OpDiv_US(VALUE & a, VALUE & b)			{ ASSERT(b.us != 0); a.us = ((b.us == 0) ? 0 : (unsigned short)(a.us / b.us)); };
static inline void OpDiv_IN(VALUE & a, VALUE & b)			{ ASSERT(b.i != 0); a.i = ((b.i == 0) ? 0 : (a.i / b.i)); };
static inline void OpDiv_UI(VALUE & a, VALUE & b)			{ ASSERT(b.ui != 0); a.ui = ((b.ui == 0) ? 0 : (a.ui / b.ui)); };
static inline void OpDiv_LL(VALUE & a, VALUE & b)			{ ASSERT(b.ll != 0); a.ll = ((b.ll == 0) ? 0 : (a.ll / b.ll)); };
static inline void OpDiv_UL(VALUE & a, VALUE & b)			{ ASSERT(b.ull != 0); a.ull = ((b.ull == 0) ? 0 : (a.ull / b.ull)); };
static inline void OpDiv_FL(VALUE & a, VALUE & b)			{ ASSERT(b.f != 0); a.f = ((b.f == 0) ? 0 : (a.f / b.f)); };
static inline void OpDiv_DB(VALUE & a, VALUE & b)			{ ASSERT(b.d != 0); a.d = ((b.d == 0) ? 0 : (a.d / b.d)); };
static inline void OpDiv_LD(VALUE & a, VALUE & b)			{ ASSERT(b.ld != 0); a.ld = ((b.ld == 0) ? 0 : (a.ld / b.ld)); };

static inline void OpMod_CH(VALUE & a, VALUE & b)			{ ASSERT(b.c != 0); a.c = ((b.c == 0) ? 0 : (char)(a.c % b.c)); };
static inline void OpMod_UC(VALUE & a, VALUE & b)			{ ASSERT(b.uc != 0); a.uc = ((b.uc == 0) ? 0 : (unsigned char)(a.uc % b.uc)); };
static inline void OpMod_SH(VALUE & a, VALUE & b)			{ ASSERT(b.s != 0); a.s = ((b.s == 0) ? 0 : (short)(a.s % b.s)); };
static inline void OpMod_US(VALUE & a, VALUE & b)			{ ASSERT(b.us != 0); a.us = ((b.us == 0) ? 0 : (unsigned short)(a.us % b.us)); };
static inline void OpMod_IN(VALUE & a, VALUE & b)			{ ASSERT(b.i != 0); a.i = ((b.i == 0) ? 0 : (a.i % b.i)); };
static inline void OpMod_UI(VALUE & a, VALUE & b)			{ ASSERT(b.ui != 0); a.ui = ((b.ui == 0) ? 0 : (a.ui % b.ui)); };
static inline void OpMod_LL(VALUE & a, VALUE & b)			{ ASSERT(b.ll != 0); a.ll = ((b.ll == 0) ? 0 : (a.ll % b.ll)); };
static inline void OpMod_UL(VALUE & a, VALUE & b)			{ ASSERT(b.ull != 0); a.ull = ((b.ull == 0) ? 0 : (a.ull % b.ull)); };

static inline void OpAdd_CH(VALUE & a, VALUE & b)			{ a.c = (char)(a.c + b.c); };
static inline void OpAdd_UC(VALUE & a, VALUE & b)			{ a.uc = (unsigned char)(a.uc + b.uc); };
static inline void OpAdd_SH(VALUE & a, VALUE & b)			{ a.s = (short)(a.s + b.s); };
static inline void OpAdd_US(VALUE & a, VALUE & b)			{ a.us = (unsigned short)(a.us + b.us); };
static inline void OpAdd_IN(VALUE & a, VALUE & b)			{ a.i += b.i; };
static inline void OpAdd_UI(VALUE & a, VALUE & b)			{ a.ui += b.ui; };
static inline void OpAdd_LL(VALUE & a, VALUE & b)			{ a.ll += b.ll; };
static inline void OpAdd_UL(VALUE & a, VALUE & b)			{ a.ull += b.ull; };
static inline void OpAdd_FL(VALUE & a, VALUE & b)			{ a.f += b.f; };
static inline void OpAdd_DB(VALUE & a, VALUE & b)			{ a.d += b.d; };
static inline void OpAdd_LD(VALUE & a, VALUE & b)			{ a.ld += b.ld; };

static inline void OpSub_CH(VALUE & a, VALUE & b)			{ a.c = (char)(a.c - b.c); };
static inline void OpSub_UC(VALUE & a, VALUE & b)			{ a.uc = (unsigned char)(a.uc - b.uc); };
static inline void OpSub_SH(VALUE & a, VALUE & b)			{ a.s = (short)(a.s - b.s); };
static inline void OpSub_US(VALUE & a, VALUE & b)			{ a.us = (unsigned short)(a.us - b.us); };
static inline void OpSub_IN(VALUE & a, VALUE & b)			{ a.i -= b.i; };
static inline void OpSub_UI(VALUE & a, VALUE & b)			{ a.ui -= b.ui; };
static inline void OpSub_LL(VALUE & a, VALUE & b)			{ a.ll -= b.ll; };
static inline void OpSub_UL(VALUE & a, VALUE & b)			{ a.ull -= b.ull; };
static inline void OpSub_FL(VALUE & a, VALUE & b)			{ a.f -= b.f; };
static inline void OpSub_DB(VALUE & a, VALUE & b)			{ a.d -= b.d; };
static inline void OpSub_LD(VALUE & a, VALUE & b)			{ a.ld -= b.ld; };

static inline void OpLShift_CH(VALUE & a, VALUE & b)		{ a.c <<= b.c; };
static inline void OpLShift_UC(VALUE & a, VALUE & b)		{ a.uc <<= b.uc; };
static inline void OpLShift_SH(VALUE & a, VALUE & b)		{ a.s <<= b.s; };
static inline void OpLShift_US(VALUE & a, VALUE & b)		{ a.us <<= b.us; };
static inline void OpLShift_IN(VALUE & a, VALUE & b)		{ a.i <<= b.i; };
static inline void OpLShift_UI(VALUE & a, VALUE & b)		{ a.ui <<= b.ui; };
static inline void OpLShift_LL(VALUE & a, VALUE & b)		{ a.ll <<= b.ll; };
static inline void OpLShift_UL(VALUE & a, VALUE & b)		{ a.ull <<= b.ull; };

static inline void OpRShift_CH(VALUE & a, VALUE & b)		{ a.c >>= b.c; };
static inline void OpRShift_UC(VALUE & a, VALUE & b)		{ a.uc >>= b.uc; };
static inline void OpRShift_SH(VALUE & a, VALUE & b)		{ a.s >>= b.s; };
static inline void OpRShift_US(VALUE & a, VALUE & b)		{ a.us >>= b.us; };
static inline void OpRShift_IN(VALUE & a, VALUE & b)		{ a.i >>= b.i; };
static inline void OpRShift_UI(VALUE & a, VALUE & b)		{ a.ui >>= b.ui; };
static inline void OpRShift_LL(VALUE & a, VALUE & b)		{ a.ll >>= b.ll; };
static inline void OpRShift_UL(VALUE & a, VALUE & b)		{ a.ull >>= b.ull; };

static inline void OpLT_CH(VALUE & a, VALUE & b)			{ a.i = (a.c < b.c); };
static inline void OpLT_UC(VALUE & a, VALUE & b)			{ a.i = (a.uc < b.uc); };
static inline void OpLT_SH(VALUE & a, VALUE & b)			{ a.i = (a.s < b.s); };
static inline void OpLT_US(VALUE & a, VALUE & b)			{ a.i = (a.us < b.us); };
static inline void OpLT_IN(VALUE & a, VALUE & b)			{ a.i = (a.i < b.i); };
static inline void OpLT_UI(VALUE & a, VALUE & b)			{ a.i = (a.ui < b.ui); };
static inline void OpLT_LL(VALUE & a, VALUE & b)			{ a.i = (a.ll < b.ll); };
static inline void OpLT_UL(VALUE & a, VALUE & b)			{ a.i = (a.ull < b.ull); };
static inline void OpLT_FL(VALUE & a, VALUE & b)			{ a.i = (a.f < b.f); };
static inline void OpLT_DB(VALUE & a, VALUE & b)			{ a.i = (a.d < b.d); };
static inline void OpLT_LD(VALUE & a, VALUE & b)			{ a.i = (a.ld < b.ld); };

static inline void OpGT_CH(VALUE & a, VALUE & b)			{ a.i = (a.c > b.c); };
static inline void OpGT_UC(VALUE & a, VALUE & b)			{ a.i = (a.uc > b.uc); };
static inline void OpGT_SH(VALUE & a, VALUE & b)			{ a.i = (a.s > b.s); };
static inline void OpGT_US(VALUE & a, VALUE & b)			{ a.i = (a.us > b.us); };
static inline void OpGT_IN(VALUE & a, VALUE & b)			{ a.i = (a.i > b.i); };
static inline void OpGT_UI(VALUE & a, VALUE & b)			{ a.i = (a.ui > b.ui); };
static inline void OpGT_LL(VALUE & a, VALUE & b)			{ a.i = (a.ll > b.ll); };
static inline void OpGT_UL(VALUE & a, VALUE & b)			{ a.i = (a.ull > b.ull); };
static inline void OpGT_FL(VALUE & a, VALUE & b)			{ a.i = (a.f > b.f); };
static inline void OpGT_DB(VALUE & a, VALUE & b)			{ a.i = (a.d > b.d); };
static inline void OpGT_LD(VALUE & a, VALUE & b)			{ a.i = (a.ld > b.ld); };

static inline void OpLTE_CH(VALUE & a, VALUE & b)			{ a.i = (a.c <= b.c); };
static inline void OpLTE_UC(VALUE & a, VALUE & b)			{ a.i = (a.uc <= b.uc); };
static inline void OpLTE_SH(VALUE & a, VALUE & b)			{ a.i = (a.s <= b.s); };
static inline void OpLTE_US(VALUE & a, VALUE & b)			{ a.i = (a.us <= b.us); };
static inline void OpLTE_IN(VALUE & a, VALUE & b)			{ a.i = (a.i <= b.i); };
static inline void OpLTE_UI(VALUE & a, VALUE & b)			{ a.i = (a.ui <= b.ui); };
static inline void OpLTE_LL(VALUE & a, VALUE & b)			{ a.i = (a.ll <= b.ll); };
static inline void OpLTE_UL(VALUE & a, VALUE & b)			{ a.i = (a.ull <= b.ull); };
static inline void OpLTE_FL(VALUE & a, VALUE & b)			{ a.i = (a.f <= b.f); };
static inline void OpLTE_DB(VALUE & a, VALUE & b)			{ a.i = (a.d <= b.d); };
static inline void OpLTE_LD(VALUE & a, VALUE & b)			{ a.i = (a.ld <= b.ld); };

static inline void OpGTE_CH(VALUE & a, VALUE & b)			{ a.i = (a.c >= b.c); };
static inline void OpGTE_UC(VALUE & a, VALUE & b)			{ a.i = (a.uc >= b.uc); };
static inline void OpGTE_SH(VALUE & a, VALUE & b)			{ a.i = (a.s >= b.s); };
static inline void OpGTE_US(VALUE & a, VALUE & b)			{ a.i = (a.us >= b.us); };
static inline void OpGTE_IN(VALUE & a, VALUE & b)			{ a.i = (a.i >= b.i); };
static inline void OpGTE_UI(VALUE & a, VALUE & b)			{ a.i = (a.ui >= b.ui); };
static inline void OpGTE_LL(VALUE & a, VALUE & b)			{ a.i = (a.ll >= b.ll); };
static inline void OpGTE_UL(VALUE & a, VALUE & b)			{ a.i = (a.ull >= b.ull); };
static inline void OpGTE_FL(VALUE & a, VALUE & b)			{ a.i = (a.f >= b.f); };
static inline void OpGTE_DB(VALUE & a, VALUE & b)			{ a.i = (a.d >= b.d); };
static inline void OpGTE_LD(VALUE & a, VALUE & b)			{ a.i = (a.ld >= b.ld); };

static inline void OpEQ_CH(VALUE & a, VALUE & b)			{ a.i = (a.c == b.c); };
static inline void OpEQ_UC(VALUE & a, VALUE & b)			{ a.i = (a.uc == b.uc); };
static inline void OpEQ_SH(VALUE & a, VALUE & b)			{ a.i = (a.s == b.s); };
static inline void OpEQ_US(VALUE & a, VALUE & b)			{ a.i = (a.us == b.us); };
static inline void OpEQ_IN(VALUE & a, VALUE & b)			{ a.i = (a.i == b.i); };
static inline void OpEQ_UI(VALUE & a, VALUE & b)			{ a.i = (a.ui == b.ui); };
static inline void OpEQ_LL(VALUE & a, VALUE & b)			{ a.i = (a.ll == b.ll); };
static inline void OpEQ_UL(VALUE & a, VALUE & b)			{ a.i = (a.ull == b.ull); };
static inline void OpEQ_FL(VALUE & a, VALUE & b)			{ a.i = (a.f == b.f); };
static inline void OpEQ_DB(VALUE & a, VALUE & b)			{ a.i = (a.d == b.d); };
static inline void OpEQ_LD(VALUE & a, VALUE & b)			{ a.i = (a.ld == b.ld); };

static inline void OpNEQ_CH(VALUE & a, VALUE & b)			{ a.i = (a.c != b.c); };
static inline void OpNEQ_UC(VALUE & a, VALUE & b)			{ a.i = (a.uc != b.uc); };
static inline void OpNEQ_SH(VALUE & a, VALUE & b)			{ a.i = (a.s != b.s); };
static inline void OpNEQ_US(VALUE & a, VALUE & b)			{ a.i = (a.us != b.us); };
static inline void OpNEQ_IN(VALUE & a, VALUE & b)			{ a.i = (a.i != b.i); };
static inline void OpNEQ_UI(VALUE & a, VALUE & b)			{ a.i = (a.ui != b.ui); };
static inline void OpNEQ_LL(VALUE & a, VALUE & b)			{ a.i = (a.ll != b.ll); };
static inline void OpNEQ_UL(VALUE & a, VALUE & b)			{ a.i = (a.ull != b.ull); };
static inline void OpNEQ_FL(VALUE & a, VALUE & b)			{ a.i = (a.f != b.f); };
static inline void OpNEQ_DB(VALUE & a, VALUE & b)			{ a.i = (a.d != b.d); };
static inline void OpNEQ_LD(VALUE & a, VALUE & b)			{ a.i = (a.ld != b.ld); };

static inline void OpBAnd_CH(VALUE & a, VALUE & b)			{ a.c &= b.c; };
static inline void OpBAnd_UC(VALUE & a, VALUE & b)			{ a.uc &= b.uc; };
static inline void OpBAnd_SH(VALUE & a, VALUE & b)			{ a.s &= b.s; };
static inline void OpBAnd_US(VALUE & a, VALUE & b)			{ a.us &= b.us; };
static inline void OpBAnd_IN(VALUE & a, VALUE & b)			{ a.i &= b.i; };
static inline void OpBAnd_UI(VALUE & a, VALUE & b)			{ a.ui &= b.ui; };
static inline void OpBAnd_LL(VALUE & a, VALUE & b)			{ a.ll &= b.ll; };
static inline void OpBAnd_UL(VALUE & a, VALUE & b)			{ a.ull &= b.ull; };

static inline void OpBXor_CH(VALUE & a, VALUE & b)			{ a.c ^= b.c; };
static inline void OpBXor_UC(VALUE & a, VALUE & b)			{ a.uc ^= b.uc; };
static inline void OpBXor_SH(VALUE & a, VALUE & b)			{ a.s ^= b.s; };
static inline void OpBXor_US(VALUE & a, VALUE & b)			{ a.us ^= b.us; };
static inline void OpBXor_IN(VALUE & a, VALUE & b)			{ a.i ^= b.i; };
static inline void OpBXor_UI(VALUE & a, VALUE & b)			{ a.ui ^= b.ui; };
static inline void OpBXor_LL(VALUE & a, VALUE & b)			{ a.ll ^= b.ll; };
static inline void OpBXor_UL(VALUE & a, VALUE & b)			{ a.ull ^= b.ull; };

static inline void OpBOr_CH(VALUE & a, VALUE & b)			{ a.c |= b.c; };
static inline void OpBOr_UC(VALUE & a, VALUE & b)			{ a.uc |= b.uc; };
static inline void OpBOr_SH(VALUE & a, VALUE & b)			{ a.s |= b.s; };
static inline void OpBOr_US(VALUE & a, VALUE & b)			{ a.us |= b.us; };
static inline void OpBOr_IN(VALUE & a, VALUE & b)			{ a.i |= b.i; };
static inline void OpBOr_UI(VALUE & a, VALUE & b)			{ a.ui |= b.ui; };
static inline void OpBOr_LL(VALUE & a, VALUE & b)			{ a.ll |= b.ll; };
static inline void OpBOr_UL(VALUE & a, VALUE & b)			{ a.ull |= b.ull; };

static inline void OpLAnd_CH(VALUE & a, VALUE & b)			{ a.i = (a.c && b.c); };
static inline void OpLAnd_UC(VALUE & a, VALUE & b)			{ a.i = (a.uc && b.uc); };
static inline void OpLAnd_SH(VALUE & a, VALUE & b)			{ a.i = (a.s && b.s); };
static inline void OpLAnd_US(VALUE & a, VALUE & b)			{ a.i = (a.us && b.us); };
static inline void OpLAnd_IN(VALUE & a, VALUE & b)			{ a.i = (a.i && b.i); };
static inline void OpLAnd_UI(VALUE & a, VALUE & b)			{ a.i = (a.ui && b.ui); };
static inline void OpLAnd_LL(VALUE & a, VALUE & b)			{ a.i = (a.ll && b.ll); };
static inline void OpLAnd_UL(VALUE & a, VALUE & b)			{ a.i = (a.ull && b.ull); };
static inline void OpLAnd_FL(VALUE & a, VALUE & b)			{ a.i = (a.f && b.f); };
static inline void OpLAnd_DB(VALUE & a, VALUE & b)			{ a.i = (a.d && b.d); };
static inline void OpLAnd_LD(VALUE & a, VALUE & b)			{ a.i = (a.ld && b.ld); };

static inline void OpLOr_CH(VALUE & a, VALUE & b)			{ a.i = (a.c || b.c); };
static inline void OpLOr_UC(VALUE & a, VALUE & b)			{ a.i = (a.uc || b.uc); };
static inline void OpLOr_SH(VALUE & a, VALUE & b)			{ a.i = (a.s || b.s); };
static inline void OpLOr_US(VALUE & a, VALUE & b)			{ a.i = (a.us || b.us); };
static inline void OpLOr_IN(VALUE & a, VALUE & b)			{ a.i = (a.i || b.i); };
static inline void OpLOr_UI(VALUE & a, VALUE & b)			{ a.i = (a.ui || b.ui); };
static inline void OpLOr_LL(VALUE & a, VALUE & b)			{ a.i = (a.ll || b.ll); };
static inline void OpLOr_UL(VALUE & a, VALUE & b)			{ a.i = (a.ull || b.ull); };
static inline void OpLOr_FL(VALUE & a, VALUE & b)			{ a.i = (a.f || b.f); };
static inline void OpLOr_DB(VALUE & a, VALUE & b)			{ a.i = (a.d || b.d); };
static inline void OpLOr_LD(VALUE & a, VALUE & b)			{ a.i = (a.ld || b.ld); };


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void EmitNodeChildren(
	PARSE & parse,
	BYTE_STREAM * stream,
	PARSENODE * node)
{
	if (!node)
	{
		return;
	}
	for (int ii = 0; ii < node->childcount; ii++)
	{
		EmitNode(parse, stream, node->children[ii]);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BOOL EmitNodeNum(
	PARSE & parse, 
	BYTE_STREAM * stream,
	PARSENODE * node,
	BOOL bEmitVMI)
{
	ASSERT_RETFALSE(node->token.tok == TOK_NUM);
	if (bEmitVMI)
	{
		return ByteStreamAddVMINum(stream, node->token.type, node->token.value);
	}
	else
	{
		return ByteStreamAddNum(stream, node->token.type, node->token.value);
	}
}

//----------------------------------------------------------------------------
// emit and expression, returning either a type of register or constant
// (only if allowed).  if register, value is the register which contains
// the value of the expression emitted.  if constant, value is the constant
// value of the expression
//----------------------------------------------------------------------------
BOOL EmitNodeExpr(
	PARSE & parse,
	PARSENODE * node,
	int * value,
	int * type,
	BOOL allow_constant)
{
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL EmitNode(
	PARSE & parse,
	BYTE_STREAM * stream,
	PARSENODE * node)
{
	switch (node->token.tok)
	{
	case TOK_NUM:
		EmitNodeNum(parse, stream, node, TRUE);
		break;
	case TOK_GLOBALADR:
		{
			ASSERT_RETFALSE(node->token.value.sym);
			if (node->childcount == 0)
			{
				ByteStreamAddInt(stream, VMI_GLOBAL_PTR);
				ByteStreamAddInt(stream, node->offset);
			}
			else
			{
				ASSERT_RETFALSE(node->childcount == 1);
				EmitNode(parse, stream, node->children[0]);
				ByteStreamAddInt(stream, VMI_IGLOBAL_PTR);
				ByteStreamAddInt(stream, node->offset);
			}
		}
		break;
	case TOK_LOCALADR:
		{
			ASSERT_RETFALSE(node->token.value.sym);
			if (node->childcount == 0)
			{
				ByteStreamAddInt(stream, VMI_LOCAL_PTR);
				ByteStreamAddInt(stream, node->offset);
			}
			else
			{
				ASSERT_RETFALSE(node->childcount == 1);
				EmitNode(parse, stream, node->children[0]);
				ByteStreamAddInt(stream, VMI_ILOCAL_PTR);
				ByteStreamAddInt(stream, node->offset);
			}
		}
		break;
	case TOK_GLOBALVAR:
		{
			ASSERT_RETFALSE(node->token.value.sym);
			if (node->childcount == 0)
			{
				ByteStreamAddInt(stream, VMI_GLOBAL + TypeGetVMIOffset(node->token.type));
				ByteStreamAddInt(stream, node->offset);
			}
			else
			{
				ASSERT_RETFALSE(node->childcount == 1);
				EmitNode(parse, stream, node->children[0]);
				ByteStreamAddInt(stream, VMI_IGLOBAL + TypeGetVMIOffset(node->token.type));
				ByteStreamAddInt(stream, node->offset);
			}
		}
		break;
	case TOK_LOCALVAR:
	case TOK_PARAM:
		{
			ASSERT_RETFALSE(node->token.value.sym);
			if (node->childcount == 0)
			{
				ByteStreamAddInt(stream, VMI_LOCAL + TypeGetVMIOffset(node->token.type));
				ByteStreamAddInt(stream, node->offset);
			}
			else
			{
				ASSERT_RETFALSE(node->childcount == 1);
				EmitNode(parse, stream, node->children[0]);
				ByteStreamAddInt(stream, VMI_ILOCAL + TypeGetVMIOffset(node->token.type));
				ByteStreamAddInt(stream, node->offset);
			}
		}
		break;
	case TOK_GLOBAL_INIT:
	case TOK_LOCAL_INIT:
		{
			ASSERT_RETFALSE(node->childcount == 1);
			ByteStreamAddInt(stream, TokenDef[node->token.tok].vmi + TypeGetVMIOffset(node->token.type));
			ByteStreamAddInt(stream, node->offset);						// address
			EmitNodeNum(parse, stream, node->children[0], FALSE);		// value
		}
		break;
	case TOK_GLOBAL_INITM:
	case TOK_LOCAL_INITM:
		{
			ByteStreamAddInt(stream, TokenDef[node->token.tok].vmi);
			ByteStreamAddInt(stream, node->offset);						// address
			int locSize = ByteStreamGetPosition(stream);
			int size = 0;
			ByteStreamAddInt(stream, 0);								// patch size later
			for (int ii = 0; ii < node->childcount; ii++)
			{
				PARSENODE * child = node->children[ii];
				ASSERT_RETFALSE(child);
				switch (child->token.tok)
				{
				case TOK_NUM:
					size += TypeGetSize(child->token.type);
					EmitNodeNum(parse, stream, child, FALSE);			// values
					break;
				case TOK_FILLZERO:
					size += child->token.value.ui;
					ByteStreamSet(stream, 0, child->token.value.ui);
					break;
				default:
					ASSERT_RETFALSE(0);
				}
			}
			ByteStreamPatchInt(stream, size, locSize);
		}
		break;
	case TOK_NEG:
	case TOK_TILDE:
	case TOK_NOT:
		{
			ASSERT_RETFALSE(node->childcount == 1);
			EmitNodeChildren(parse, stream, node);
			ByteStreamAddInt(stream, TokenDef[node->token.tok].vmi + TypeGetVMIOffset(node->token.type));
		}
		break;
	case TOK_TYPECAST:
		{
			ASSERT_RETFALSE(node->childcount == 1);
			EmitNodeChildren(parse, stream, node);
			int vmi = TypeGetVMIOffset(node->children[0]->token.type);
			switch (TypeGetBasicType(node->token.type))
			{
			case TYPE_CHAR:		vmi += VMI_TYPECAST_TO_CH; break;
			case TYPE_BYTE:		vmi += VMI_TYPECAST_TO_UC; break;
			case TYPE_SHORT:	vmi += VMI_TYPECAST_TO_SH; break;
			case TYPE_WORD:		vmi += VMI_TYPECAST_TO_US; break;
			case TYPE_INT:		vmi += VMI_TYPECAST_TO_IN; break;
			case TYPE_DWORD:	vmi += VMI_TYPECAST_TO_UI; break;
			case TYPE_INT64:	vmi += VMI_TYPECAST_TO_LL; break;
			case TYPE_QWORD:	vmi += VMI_TYPECAST_TO_UL; break;
			case TYPE_FLOAT:	vmi += VMI_TYPECAST_TO_FL; break;
			case TYPE_DOUBLE:	vmi += VMI_TYPECAST_TO_DB; break;
			case TYPE_LDOUBLE:	vmi += VMI_TYPECAST_TO_LD; break;
			default:			
				ASSERT(0);
			}
			ByteStreamAddInt(stream, vmi);
		}
		break;
	case TOK_POWER:
	case TOK_MUL:
	case TOK_DIV:
	case TOK_MOD:
	case TOK_ADD:
	case TOK_SUB:
	case TOK_LSHIFT:
	case TOK_RSHIFT:
	case TOK_LT:
	case TOK_GT:
	case TOK_LTE:
	case TOK_GTE:
	case TOK_EQ:
	case TOK_NE:
	case TOK_AND:
	case TOK_XOR:
	case TOK_OR:
		{
			ASSERT_RETFALSE(node->childcount == 2);
			EmitNodeChildren(parse, stream, node);
			ByteStreamAddInt(stream, TokenDef[node->token.tok].vmi + TypeGetVMIOffset(node->token.type));
		}
		break;
	case TOK_LAND:
		{
			ASSERT_RETFALSE(node->childcount == 2);
			EmitNode(parse, stream, node->children[0]);
			ByteStreamAddInt(stream, VMI_LANDJMP + TypeGetVMIOffset(node->children[0]->token.type));
			int locA = ByteStreamGetPosition(stream);
			ByteStreamAddInt(stream, 0);
			EmitNode(parse, stream, node->children[1]);
			ByteStreamAddInt(stream, VMI_LOGEND + TypeGetVMIOffset(node->children[1]->token.type));
			ByteStreamPatchInt(stream, ByteStreamGetPosition(stream), locA);
		}
		break;
	case TOK_LOR:
		{
			ASSERT_RETFALSE(node->childcount == 2);
			EmitNode(parse, stream, node->children[0]);
			ByteStreamAddInt(stream, VMI_LORJMP + TypeGetVMIOffset(node->children[0]->token.type));
			int locA = ByteStreamGetPosition(stream);
			ByteStreamAddInt(stream, 0);
			EmitNode(parse, stream, node->children[1]);
			ByteStreamAddInt(stream, VMI_LOGEND + TypeGetVMIOffset(node->children[1]->token.type));
			ByteStreamPatchInt(stream, ByteStreamGetPosition(stream), locA);
		}
		break;
	case TOK_TERNARY:
		{
			ASSERT_RETFALSE(node->childcount == 3);
			EmitNode(parse, stream, node->children[0]);
			ByteStreamAddInt(stream, VMI_JMPZERO + TypeGetVMIOffset(node->children[0]->token.type));
			int locA = ByteStreamGetPosition(stream);
			ByteStreamAddInt(stream, 0);
			EmitNode(parse, stream, node->children[1]);
			ByteStreamAddInt(stream, VMI_JMP);
			int locB = ByteStreamGetPosition(stream);
			ByteStreamAddInt(stream, 0);
			ByteStreamPatchInt(stream, ByteStreamGetPosition(stream), locA);
			EmitNode(parse, stream, node->children[2]);
			ByteStreamPatchInt(stream, ByteStreamGetPosition(stream), locB);
		}
		break;
	case TOK_ASSIGN:
		{
			ASSERT_RETFALSE(node->childcount == 2);
			switch (node->children[0]->token.tok)
			{
			case TOK_GLOBALADR:
				{
					ASSERT_RETFALSE(node->children[0]->token.value.sym);
					if (node->children[1]->token.tok == TOK_NUM)
					{
						ByteStreamAddInt(stream, VMI_ASSIGN_GLOBAL_CONST + TypeGetVMIOffset(node->token.type));
						ByteStreamAddInt(stream, node->children[0]->offset);
						EmitNodeNum(parse, stream, node->children[1], FALSE);
					}
					else
					{
						EmitNodeChildren(parse, stream, node);
						ByteStreamAddInt(stream, VMI_ASSIGN_GLOBAL + TypeGetVMIOffset(node->token.type));
					}
				}
				break;
			case TOK_LOCALADR:
				{
					ASSERT_RETFALSE(node->children[0]->token.value.sym);
					if (node->children[1]->token.tok == TOK_NUM)
					{
						ByteStreamAddInt(stream, VMI_ASSIGN_LOCAL_CONST + TypeGetVMIOffset(node->token.type));
						ByteStreamAddInt(stream, node->children[0]->offset);
						ASSERT_RETFALSE(node->children[1]->token.tok == TOK_NUM);
						EmitNodeNum(parse, stream, node->children[1], FALSE);
					}
					else
					{
						EmitNodeChildren(parse, stream, node);
						ByteStreamAddInt(stream, VMI_ASSIGN_LOCAL + TypeGetVMIOffset(node->token.type));
					}
				}
				break;
			case TOK_CONTEXTVAR:
				{
					int var = node->children[0]->token.value.i;
					ASSERT_RETFALSE(var >= 0 && var < arrsize(ContextDef));
					EmitNodeChildren(parse, stream, node);
					ByteStreamAddInt(stream, VMI_ASSIGN_CONTEXTVAR);
					ByteStreamAddInt(stream, var);
				}
				break;
			case TOK_LSTAT:
				{
					EmitNode(parse, stream, node->children[1]);
					ByteStreamAddInt(stream, VMI_ASSIGN_STAT);
					ByteStreamAddInt(stream, node->children[0]->token.value.i);
				}
				break;
			case TOK_LSTAT_US:
				{
					EmitNode(parse, stream, node->children[1]);
					ByteStreamAddInt(stream, VMI_ASSIGN_STAT_US);
					ByteStreamAddInt(stream, node->children[0]->token.value.i);
				}
				break;
			case TOK_PERIOD:
				{
					PARSENODE * per = node->children[0];
					ASSERT_RETFALSE(per->childcount == 2);
					ASSERT_RETFALSE(per->children[1]->token.tok == TOK_LSTAT || per->children[1]->token.tok == TOK_LSTAT_US);
					EmitNode(parse, stream, node->children[1]);
					EmitNode(parse, stream, per->children[0]);
					ByteStreamAddInt(stream, VMI_SET_UNIT);
					if (per->children[1]->token.tok == TOK_LSTAT)
					{
						ByteStreamAddInt(stream, VMI_ASSIGN_STAT);
					}
					else
					{
						ByteStreamAddInt(stream, VMI_ASSIGN_STAT_US);
					}
					ByteStreamAddInt(stream, per->children[1]->token.value.i);
				}
				break;
			default:
				ASSERT_RETFALSE(0);
			}
		}
		break;
	case TOK_A_MUL:
		{
			ASSERT_RETFALSE(node->childcount == 2);
			switch (node->children[0]->token.tok)
			{
			case TOK_GLOBALADR:
				{
					ASSERT_RETFALSE(node->children[0]->token.value.sym);
					EmitNodeChildren(parse, stream, node);
					ByteStreamAddInt(stream, VMI_A_MUL_GLOBAL + TypeGetVMIOffset(node->token.type));
				}
				break;
			case TOK_LOCALADR:
				{
					ASSERT_RETFALSE(node->children[0]->token.value.sym);
					EmitNodeChildren(parse, stream, node);
					ByteStreamAddInt(stream, VMI_A_MUL_LOCAL + TypeGetVMIOffset(node->token.type));
				}
				break;
			case TOK_LSTAT:
				{
					EmitNode(parse, stream, node->children[1]);
					ByteStreamAddInt(stream, VMI_A_MUL_STAT);
					ByteStreamAddInt(stream, node->children[0]->token.value.i);
				}
				break;
			case TOK_LSTAT_US:
				{
					EmitNode(parse, stream, node->children[1]);
					ByteStreamAddInt(stream, VMI_A_MUL_STAT_US);
					ByteStreamAddInt(stream, node->children[0]->token.value.i);
				}
				break;
			case TOK_PERIOD:
				{
					PARSENODE * per = node->children[0];
					ASSERT_RETFALSE(per->childcount == 2);
					ASSERT_RETFALSE(per->children[1]->token.tok == TOK_LSTAT || per->children[1]->token.tok == TOK_LSTAT_US);
					EmitNode(parse, stream, node->children[1]);
					EmitNode(parse, stream, per->children[0]);
					ByteStreamAddInt(stream, VMI_SET_UNIT);
					ByteStreamAddInt(stream, per->children[1]->token.tok == TOK_LSTAT ? VMI_A_MUL_STAT : VMI_A_MUL_STAT_US);
					ByteStreamAddInt(stream, per->children[1]->token.value.i);
				}
				break;
			default:
				ASSERT_RETFALSE(0);
			}
		}
		break;
	case TOK_A_DIV:
		{
			ASSERT_RETFALSE(node->childcount == 2);
			switch (node->children[0]->token.tok)
			{
			case TOK_GLOBALADR:
				{
					ASSERT_RETFALSE(node->children[0]->token.value.sym);
					EmitNodeChildren(parse, stream, node);
					ByteStreamAddInt(stream, VMI_A_DIV_GLOBAL + TypeGetVMIOffset(node->token.type));
				}
				break;
			case TOK_LOCALADR:
				{
					ASSERT_RETFALSE(node->children[0]->token.value.sym);
					EmitNodeChildren(parse, stream, node);
					ByteStreamAddInt(stream, VMI_A_DIV_LOCAL + TypeGetVMIOffset(node->token.type));
				}
				break;
			case TOK_LSTAT:
				{
					EmitNode(parse, stream, node->children[1]);
					ByteStreamAddInt(stream, VMI_A_DIV_STAT);
					ByteStreamAddInt(stream, node->children[0]->token.value.i);
				}
				break;
			case TOK_LSTAT_US:
				{
					EmitNode(parse, stream, node->children[1]);
					ByteStreamAddInt(stream, VMI_A_DIV_STAT_US);
					ByteStreamAddInt(stream, node->children[0]->token.value.i);
				}
				break;
			case TOK_PERIOD:
				{
					PARSENODE * per = node->children[0];
					ASSERT_RETFALSE(per->childcount == 2);
					ASSERT_RETFALSE(per->children[1]->token.tok == TOK_LSTAT || per->children[1]->token.tok == TOK_LSTAT_US);
					EmitNode(parse, stream, node->children[1]);
					EmitNode(parse, stream, per->children[0]);
					ByteStreamAddInt(stream, VMI_SET_UNIT);
					ByteStreamAddInt(stream, per->children[1]->token.tok == TOK_LSTAT ? VMI_A_DIV_STAT : VMI_A_DIV_STAT_US);
					ByteStreamAddInt(stream, per->children[1]->token.value.i);
				}
				break;
			default:
				ASSERT_RETFALSE(0);
			}
		}
		break;
	case TOK_A_MOD:
		{
			ASSERT_RETFALSE(node->childcount == 2);
			switch (node->children[0]->token.tok)
			{
			case TOK_GLOBALADR:
				{
					ASSERT_RETFALSE(node->children[0]->token.value.sym);
					EmitNodeChildren(parse, stream, node);
					ByteStreamAddInt(stream, VMI_A_MOD_GLOBAL + TypeGetVMIOffset(node->token.type));
				}
				break;
			case TOK_LOCALADR:
				{
					ASSERT_RETFALSE(node->children[0]->token.value.sym);
					EmitNodeChildren(parse, stream, node);
					ByteStreamAddInt(stream, VMI_A_MOD_LOCAL + TypeGetVMIOffset(node->token.type));
				}
				break;
			case TOK_LSTAT:
				{
					EmitNode(parse, stream, node->children[1]);
					ByteStreamAddInt(stream, VMI_A_MOD_STAT);
					ByteStreamAddInt(stream, node->children[0]->token.value.i);
				}
				break;
			case TOK_LSTAT_US:
				{
					EmitNode(parse, stream, node->children[1]);
					ByteStreamAddInt(stream, VMI_A_MOD_STAT_US);
					ByteStreamAddInt(stream, node->children[0]->token.value.i);
				}
				break;
			case TOK_PERIOD:
				{
					PARSENODE * per = node->children[0];
					ASSERT_RETFALSE(per->childcount == 2);
					ASSERT_RETFALSE(per->children[1]->token.tok == TOK_LSTAT || per->children[1]->token.tok == TOK_LSTAT_US);
					EmitNode(parse, stream, node->children[1]);
					EmitNode(parse, stream, per->children[0]);
					ByteStreamAddInt(stream, VMI_SET_UNIT);
					ByteStreamAddInt(stream, per->children[1]->token.tok == TOK_LSTAT ? VMI_A_MOD_STAT : VMI_A_MOD_STAT_US);
					ByteStreamAddInt(stream, per->children[1]->token.value.i);
				}
				break;
			default:
				ASSERT_RETFALSE(0);
			}
		}
		break;
	case TOK_A_ADD:
		{
			ASSERT_RETFALSE(node->childcount == 2);
			switch (node->children[0]->token.tok)
			{
			case TOK_GLOBALADR:
				{
					ASSERT_RETFALSE(node->children[0]->token.value.sym);
					EmitNodeChildren(parse, stream, node);
					ByteStreamAddInt(stream, VMI_A_ADD_GLOBAL + TypeGetVMIOffset(node->token.type));
				}
				break;
			case TOK_LOCALADR:
				{
					ASSERT_RETFALSE(node->children[0]->token.value.sym);
					EmitNodeChildren(parse, stream, node);
					ByteStreamAddInt(stream, VMI_A_ADD_LOCAL + TypeGetVMIOffset(node->token.type));
				}
				break;
			case TOK_LSTAT:
				{
					EmitNode(parse, stream, node->children[1]);
					ByteStreamAddInt(stream, VMI_A_ADD_STAT);
					ByteStreamAddInt(stream, node->children[0]->token.value.i);
				}
				break;
			case TOK_LSTAT_US:
				{
					EmitNode(parse, stream, node->children[1]);
					ByteStreamAddInt(stream, VMI_A_ADD_STAT_US);
					ByteStreamAddInt(stream, node->children[0]->token.value.i);
				}
				break;
			case TOK_PERIOD:
				{
					PARSENODE * per = node->children[0];
					ASSERT_RETFALSE(per->childcount == 2);
					ASSERT_RETFALSE(per->children[1]->token.tok == TOK_LSTAT || per->children[1]->token.tok == TOK_LSTAT_US);
					EmitNode(parse, stream, node->children[1]);
					EmitNode(parse, stream, per->children[0]);
					ByteStreamAddInt(stream, VMI_SET_UNIT);
					ByteStreamAddInt(stream, per->children[1]->token.tok == TOK_LSTAT ? VMI_A_ADD_STAT : VMI_A_ADD_STAT_US);
					ByteStreamAddInt(stream, per->children[1]->token.value.i);
				}
				break;
			default:
				ASSERT_RETFALSE(0);
			}
		}
		break;
	case TOK_A_SUB:
		{
			ASSERT_RETFALSE(node->childcount == 2);
			switch (node->children[0]->token.tok)
			{
			case TOK_GLOBALADR:
				{
					ASSERT_RETFALSE(node->children[0]->token.value.sym);
					EmitNodeChildren(parse, stream, node);
					ByteStreamAddInt(stream, VMI_A_SUB_GLOBAL + TypeGetVMIOffset(node->token.type));
				}
				break;
			case TOK_LOCALADR:
				{
					ASSERT_RETFALSE(node->children[0]->token.value.sym);
					EmitNodeChildren(parse, stream, node);
					ByteStreamAddInt(stream, VMI_A_SUB_LOCAL + TypeGetVMIOffset(node->token.type));
				}
				break;
			case TOK_LSTAT:
				{
					EmitNode(parse, stream, node->children[1]);
					ByteStreamAddInt(stream, VMI_A_SUB_STAT);
					ByteStreamAddInt(stream, node->children[0]->token.value.i);
				}
				break;
			case TOK_LSTAT_US:
				{
					EmitNode(parse, stream, node->children[1]);
					ByteStreamAddInt(stream, VMI_A_SUB_STAT_US);
					ByteStreamAddInt(stream, node->children[0]->token.value.i);
				}
				break;
			case TOK_PERIOD:
				{
					PARSENODE * per = node->children[0];
					ASSERT_RETFALSE(per->childcount == 2);
					ASSERT_RETFALSE(per->children[1]->token.tok == TOK_LSTAT || per->children[1]->token.tok == TOK_LSTAT_US);
					EmitNode(parse, stream, node->children[1]);
					EmitNode(parse, stream, per->children[0]);
					ByteStreamAddInt(stream, VMI_SET_UNIT);
					ByteStreamAddInt(stream, per->children[1]->token.tok == TOK_LSTAT ? VMI_A_SUB_STAT : VMI_A_SUB_STAT_US);
					ByteStreamAddInt(stream, per->children[1]->token.value.i);
				}
				break;
			default:
				ASSERT_RETFALSE(0);
			}
		}
		break;
	case TOK_A_LSHIFT:
		{
			ASSERT_RETFALSE(node->childcount == 2);
			switch (node->children[0]->token.tok)
			{
			case TOK_GLOBALADR:
				{
					ASSERT_RETFALSE(node->children[0]->token.value.sym);
					EmitNodeChildren(parse, stream, node);
					ByteStreamAddInt(stream, VMI_A_LSHIFT_GLOBAL + TypeGetVMIOffset(node->token.type));
				}
				break;
			case TOK_LOCALADR:
				{
					ASSERT_RETFALSE(node->children[0]->token.value.sym);
					EmitNodeChildren(parse, stream, node);
					ByteStreamAddInt(stream, VMI_A_LSHIFT_LOCAL + TypeGetVMIOffset(node->token.type));
				}
				break;
			case TOK_LSTAT:
				{
					EmitNode(parse, stream, node->children[1]);
					ByteStreamAddInt(stream, VMI_A_LSHIFT_STAT);
					ByteStreamAddInt(stream, node->children[0]->token.value.i);
				}
				break;
			case TOK_LSTAT_US:
				{
					EmitNode(parse, stream, node->children[1]);
					ByteStreamAddInt(stream, VMI_A_LSHIFT_STAT_US);
					ByteStreamAddInt(stream, node->children[0]->token.value.i);
				}
				break;
			case TOK_PERIOD:
				{
					PARSENODE * per = node->children[0];
					ASSERT_RETFALSE(per->childcount == 2);
					ASSERT_RETFALSE(per->children[1]->token.tok == TOK_LSTAT || per->children[1]->token.tok == TOK_LSTAT_US);
					EmitNode(parse, stream, node->children[1]);
					EmitNode(parse, stream, per->children[0]);
					ByteStreamAddInt(stream, VMI_SET_UNIT);
					ByteStreamAddInt(stream, per->children[1]->token.tok == TOK_LSTAT ? VMI_A_LSHIFT_STAT : VMI_A_LSHIFT_STAT_US);
					ByteStreamAddInt(stream, per->children[1]->token.value.i);
				}
				break;
			default:
				ASSERT_RETFALSE(0);
			}
		}
		break;
	case TOK_A_RSHIFT:
		{
			ASSERT_RETFALSE(node->childcount == 2);
			switch (node->children[0]->token.tok)
			{
			case TOK_GLOBALADR:
				{
					ASSERT_RETFALSE(node->children[0]->token.value.sym);
					EmitNodeChildren(parse, stream, node);
					ByteStreamAddInt(stream, VMI_A_RSHIFT_GLOBAL + TypeGetVMIOffset(node->token.type));
				}
				break;
			case TOK_LOCALADR:
				{
					ASSERT_RETFALSE(node->children[0]->token.value.sym);
					EmitNodeChildren(parse, stream, node);
					ByteStreamAddInt(stream, VMI_A_RSHIFT_LOCAL + TypeGetVMIOffset(node->token.type));
				}
				break;
			case TOK_LSTAT:
				{
					EmitNode(parse, stream, node->children[1]);
					ByteStreamAddInt(stream, VMI_A_RSHIFT_STAT);
					ByteStreamAddInt(stream, node->children[0]->token.value.i);
				}
				break;
			case TOK_LSTAT_US:
				{
					EmitNode(parse, stream, node->children[1]);
					ByteStreamAddInt(stream, VMI_A_RSHIFT_STAT_US);
					ByteStreamAddInt(stream, node->children[0]->token.value.i);
				}
				break;
			case TOK_PERIOD:
				{
					PARSENODE * per = node->children[0];
					ASSERT_RETFALSE(per->childcount == 2);
					ASSERT_RETFALSE(per->children[1]->token.tok == TOK_LSTAT || per->children[1]->token.tok == TOK_LSTAT_US);
					EmitNode(parse, stream, node->children[1]);
					EmitNode(parse, stream, per->children[0]);
					ByteStreamAddInt(stream, VMI_SET_UNIT);
					ByteStreamAddInt(stream, per->children[1]->token.tok == TOK_LSTAT ? VMI_A_RSHIFT_STAT : VMI_A_RSHIFT_STAT_US);
					ByteStreamAddInt(stream, per->children[1]->token.value.i);
				}
				break;
			default:
				ASSERT_RETFALSE(0);
			}
		}
		break;
	case TOK_A_AND:
		{
			ASSERT_RETFALSE(node->childcount == 2);
			switch (node->children[0]->token.tok)
			{
			case TOK_GLOBALADR:
				{
					ASSERT_RETFALSE(node->children[0]->token.value.sym);
					EmitNodeChildren(parse, stream, node);
					ByteStreamAddInt(stream, VMI_A_AND_GLOBAL + TypeGetVMIOffset(node->token.type));
				}
				break;
			case TOK_LOCALADR:
				{
					ASSERT_RETFALSE(node->children[0]->token.value.sym);
					EmitNodeChildren(parse, stream, node);
					ByteStreamAddInt(stream, VMI_A_AND_LOCAL + TypeGetVMIOffset(node->token.type));
				}
				break;
			case TOK_LSTAT:
				{
					EmitNode(parse, stream, node->children[1]);
					ByteStreamAddInt(stream, VMI_A_AND_STAT);
					ByteStreamAddInt(stream, node->children[0]->token.value.i);
				}
				break;
			case TOK_LSTAT_US:
				{
					EmitNode(parse, stream, node->children[1]);
					ByteStreamAddInt(stream, VMI_A_AND_STAT_US);
					ByteStreamAddInt(stream, node->children[0]->token.value.i);
				}
				break;
			case TOK_PERIOD:
				{
					PARSENODE * per = node->children[0];
					ASSERT_RETFALSE(per->childcount == 2);
					ASSERT_RETFALSE(per->children[1]->token.tok == TOK_LSTAT || per->children[1]->token.tok == TOK_LSTAT_US);
					EmitNode(parse, stream, node->children[1]);
					EmitNode(parse, stream, per->children[0]);
					ByteStreamAddInt(stream, VMI_SET_UNIT);
					ByteStreamAddInt(stream, per->children[1]->token.tok == TOK_LSTAT ? VMI_A_AND_STAT : VMI_A_AND_STAT_US);
					ByteStreamAddInt(stream, per->children[1]->token.value.i);
				}
				break;
			default:
				ASSERT_RETFALSE(0);
			}
		}
		break;
	case TOK_A_XOR:
		{
			ASSERT_RETFALSE(node->childcount == 2);
			switch (node->children[0]->token.tok)
			{
			case TOK_GLOBALADR:
				{
					ASSERT_RETFALSE(node->children[0]->token.value.sym);
					EmitNodeChildren(parse, stream, node);
					ByteStreamAddInt(stream, VMI_A_XOR_GLOBAL + TypeGetVMIOffset(node->token.type));
				}
				break;
			case TOK_LOCALADR:
				{
					ASSERT_RETFALSE(node->children[0]->token.value.sym);
					EmitNodeChildren(parse, stream, node);
					ByteStreamAddInt(stream, VMI_A_XOR_LOCAL + TypeGetVMIOffset(node->token.type));
				}
				break;
			case TOK_LSTAT:
				{
					EmitNode(parse, stream, node->children[1]);
					ByteStreamAddInt(stream, VMI_A_XOR_STAT);
					ByteStreamAddInt(stream, node->children[0]->token.value.i);
				}
				break;
			case TOK_LSTAT_US:
				{
					EmitNode(parse, stream, node->children[1]);
					ByteStreamAddInt(stream, VMI_A_XOR_STAT_US);
					ByteStreamAddInt(stream, node->children[0]->token.value.i);
				}
				break;
			case TOK_PERIOD:
				{
					PARSENODE * per = node->children[0];
					ASSERT_RETFALSE(per->childcount == 2);
					ASSERT_RETFALSE(per->children[1]->token.tok == TOK_LSTAT || per->children[1]->token.tok == TOK_LSTAT_US);
					EmitNode(parse, stream, node->children[1]);
					EmitNode(parse, stream, per->children[0]);
					ByteStreamAddInt(stream, VMI_SET_UNIT);
					ByteStreamAddInt(stream, per->children[1]->token.tok == TOK_LSTAT ? VMI_A_XOR_STAT : VMI_A_XOR_STAT_US);
					ByteStreamAddInt(stream, per->children[1]->token.value.i);
				}
				break;
			default:
				ASSERT_RETFALSE(0);
			}
		}
		break;
	case TOK_A_OR:
		{
			ASSERT_RETFALSE(node->childcount == 2);
			switch (node->children[0]->token.tok)
			{
			case TOK_GLOBALADR:
				{
					ASSERT_RETFALSE(node->children[0]->token.value.sym);
					EmitNodeChildren(parse, stream, node);
					ByteStreamAddInt(stream, VMI_A_OR_GLOBAL + TypeGetVMIOffset(node->token.type));
				}
				break;
			case TOK_LOCALADR:
				{
					ASSERT_RETFALSE(node->children[0]->token.value.sym);
					EmitNodeChildren(parse, stream, node);
					ByteStreamAddInt(stream, VMI_A_OR_LOCAL + TypeGetVMIOffset(node->token.type));
				}
				break;
			case TOK_LSTAT:
				{
					EmitNode(parse, stream, node->children[1]);
					ByteStreamAddInt(stream, VMI_A_OR_STAT);
					ByteStreamAddInt(stream, node->children[0]->token.value.i);
				}
				break;
			case TOK_LSTAT_US:
				{
					EmitNode(parse, stream, node->children[1]);
					ByteStreamAddInt(stream, VMI_A_OR_STAT_US);
					ByteStreamAddInt(stream, node->children[0]->token.value.i);
				}
				break;
			case TOK_PERIOD:
				{
					PARSENODE * per = node->children[0];
					ASSERT_RETFALSE(per->childcount == 2);
					ASSERT_RETFALSE(per->children[1]->token.tok == TOK_LSTAT || per->children[1]->token.tok == TOK_LSTAT_US);
					EmitNode(parse, stream, node->children[1]);
					EmitNode(parse, stream, per->children[0]);
					ByteStreamAddInt(stream, VMI_SET_UNIT);
					ByteStreamAddInt(stream, per->children[1]->token.tok == TOK_LSTAT ? VMI_A_OR_STAT : VMI_A_OR_STAT_US);
					ByteStreamAddInt(stream, per->children[1]->token.value.i);
				}
				break;
			default:
				ASSERT_RETFALSE(0);
			}
		}
		break;
	case TOK_COMMA:
		EmitNodeChildren(parse, stream, node);
		break;
	case TOK_BLOCKLIST:
		parse.cur_local_offset += node->localstack;
		EmitNodeChildren(parse, stream, node);
		parse.cur_local_offset -= node->localstack;
		break;
	case TOK_IF:
		{
			ASSERT_RETFALSE(node->childcount >= 2 && node->childcount <= 3);
			EmitNode(parse, stream, node->children[0]);
			ByteStreamAddInt(stream, VMI_JMPZERO + TypeGetVMIOffset(node->children[0]->token.type));
			int locA = ByteStreamGetPosition(stream);
			ByteStreamAddInt(stream, 0);
			EmitNode(parse, stream, node->children[1]);

			if (node->childcount == 3)
			{
				ByteStreamAddInt(stream, VMI_JMP);
				int locB = ByteStreamGetPosition(stream);
				ByteStreamAddInt(stream, 0);
				ByteStreamPatchInt(stream, ByteStreamGetPosition(stream), locA);
				EmitNode(parse, stream, node->children[2]);
				ByteStreamPatchInt(stream, ByteStreamGetPosition(stream), locB);
			}
			else
			{
				ByteStreamPatchInt(stream, ByteStreamGetPosition(stream), locA);
			}						
		}
		break;
	case TOK_WHILE:
		{
			ASSERT_RETFALSE(node->childcount == 2);
			int locA = ByteStreamGetPosition(stream);
			EmitNode(parse, stream, node->children[0]);
			ByteStreamAddInt(stream, VMI_JMPZERO + TypeGetVMIOffset(node->children[0]->token.type));
			int locB = ByteStreamGetPosition(stream);
			ByteStreamAddInt(stream, 0);
			EmitNode(parse, stream, node->children[1]);
			ByteStreamAddInt(stream, VMI_JMP);
			ByteStreamAddInt(stream, locA);
			ByteStreamPatchInt(stream, ByteStreamGetPosition(stream), locB);
		}
		break;
	case TOK_FUNCTION:
		{
			ASSERT_RETFALSE(node->token.value.sym);
			FUNCTION * func = node->token.value.sym->function;
			ASSERT_RETFALSE(func);
			ASSERT_RETFALSE(func->parameter_count == node->childcount);		// change for ...

			EmitNodeChildren(parse, stream, node);
			ByteStreamAddInt(stream, VMI_CALL);
			ByteStreamAddInt(stream, func->index);
//			ByteStreamAddInt(stream, node->childcount);						// only emit if varargs
			ByteStreamAddInt(stream, parse.cur_local_offset);
		}
		break;
	case TOK_IFUNCTION:
		{
			ASSERT_RETFALSE(node->token.value.sym);
			FUNCTION * func = node->token.value.sym->function;
			ASSERT_RETFALSE(func);
			ASSERT_RETFALSE(node->childcount >= func->min_param_count && node->childcount <= func->parameter_count);		// change for ...

			// to do: emit parameters of function, including CONTEXT ones
			for (int curparam = 0, curchild = 0; curparam < func->parameter_count; curparam++)
			{
				SSYMBOL * param = FunctionGetParameter(func, curparam);
				ASSERT_RETFALSE(param);

				switch (param->parameter_type)
				{
				case PARAM_NORMAL:
					EmitNode(parse, stream, node->children[curchild]);
					curchild++;
					break;
				case PARAM_INDEX:
				case PARAM_STAT_PARAM:
					// need to do a conversion from RSTAT to index lookup
					{
						PARSENODE * param = node->children[curchild];
						switch (param->token.tok)
						{
						case TOK_NUM:
							break;
						default:
							ASSERT_RETFALSE(0);
							break;
						}
						EmitNode(parse, stream, param);
						curchild++;
					}
					break;
				case PARAM_CONTEXT:
					ByteStreamAddInt(stream, VMI_CONTEXT);
					break;
				case PARAM_CONTEXT_VMSTATE:
					ByteStreamAddInt(stream, VMI_VMSTATE);
					break;
				case PARAM_CONTEXT_GAME:
					ByteStreamAddInt(stream, VMI_GAME);
					break;
				case PARAM_CONTEXT_UNIT:
					ByteStreamAddInt(stream, VMI_UNIT);
					break;
				case PARAM_CONTEXT_STATS:
					ByteStreamAddInt(stream, VMI_STATS);
					break;
				default:
					ASSERT_RETFALSE(0);
					break;
				}
			}

			ByteStreamAddInt(stream, VMI_ICALL);
			ByteStreamAddInt(stream, func->index);
		}
		break;
	case TOK_RSTAT:
		{
			ByteStreamAddInt(stream, VMI_RSTAT);
			ByteStreamAddInt(stream, node->token.value.i);
		}
		break;
	case TOK_RSTAT_BASE:
		{
			ByteStreamAddInt(stream, VMI_RSTAT_BASE);
			ByteStreamAddInt(stream, node->token.value.i);
		}
		break;
	case TOK_RSTAT_MOD:
		{
			ByteStreamAddInt(stream, VMI_RSTAT_MOD);
			ByteStreamAddInt(stream, node->token.value.i);
		}
		break;
	case TOK_RSTAT_US:
		{
			ByteStreamAddInt(stream, VMI_RSTAT_US);
			ByteStreamAddInt(stream, node->token.value.i);
		}
		break;
	case TOK_RSTAT_US_BASE:
		{
			ByteStreamAddInt(stream, VMI_RSTAT_US_BASE);
			ByteStreamAddInt(stream, node->token.value.i);
		}
		break;
	case TOK_RSTAT_US_MOD:
		{
			ByteStreamAddInt(stream, VMI_RSTAT_US_MOD);
			ByteStreamAddInt(stream, node->token.value.i);
		}
		break;
	case TOK_RSTATPARAM:		// stat with an expression param  (damage.[rand(1,4)])
		{
			ASSERT_RETFALSE(node->childcount == 1);
			EmitNodeChildren(parse, stream, node);	// emit param value
			ByteStreamAddInt(stream, VMI_RSTATPARAM);
			ByteStreamAddInt(stream, node->token.value.i);
		}
		break;
	case TOK_RSTATPARAM_US:		// unshifted stat with an expression param  (damage.[rand(1,4)])
		{
			ASSERT_RETFALSE(node->childcount == 1);
			EmitNodeChildren(parse, stream, node);	// emit param value
			ByteStreamAddInt(stream, VMI_RSTATPARAM_US);
			ByteStreamAddInt(stream, node->token.value.i);
		}
		break;
	case TOK_PERIOD:			// unit.stat or object.stat, etc.
		{
			ASSERT_RETFALSE(node->childcount == 2);
			ASSERT_RETFALSE(node->children[0]->token.tok == TOK_CONTEXTVAR);
			ASSERT_RETFALSE(isRstat(node->children[1]));
			EmitNode(parse, stream, node->children[0]);
			ByteStreamAddInt(stream, VMI_SET_UNIT);
			EmitNode(parse, stream, node->children[1]);
		}
		break;
	case TOK_RETURN:
		{
			ASSERT_RETFALSE(node->childcount == 1);
			EmitNodeChildren(parse, stream, node);
			ByteStreamAddInt(stream, VMI_RETURN);
		}
		break;
	case TOK_CONTEXTVAR:
		{
			int var = node->token.value.i;
			ASSERT_RETFALSE(var >= 0 && var < arrsize(ContextDef));
			ByteStreamAddInt(stream, VMI_CONTEXTVAR + TypeGetVMIOffset(node->token.type));
			ByteStreamAddInt(stream, var);
		}
		break;
	default:
		ASSERT_RETFALSE(0);
		break;
	}
	return TRUE;
}


//----------------------------------------------------------------------------
// write a function call to the code stream
//----------------------------------------------------------------------------
BOOL EmitFunction(
	PARSE & parse,
	PARSENODE * function)
{
	ASSERT_RETFALSE(function->token.tok == TOK_FUNCTION);
	ASSERT_RETFALSE(function->token.value.sym);

	FUNCTION * func = function->token.value.sym->function;
	ASSERT_RETFALSE(func);

	// allocate a new function in the vm's function list
	func->stream = ByteStreamInit(NULL);

	ASSERT_RETFALSE(function->childcount == 1);
	ASSERT_RETFALSE(function->children[0]->token.tok == TOK_BLOCKLIST);
	if (!EmitNode(parse, func->stream, function->children[0]))
	{
		func->stream->cur = 0;
		return FALSE;
	}
	if (func->stream->size >= 0)
	{
		ByteStreamAddInt(func->stream, VMI_EOF);
	}
	ByteStreamReset(func->stream);

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BOOL ParseNodeIsConstant(
	PARSENODE * node)
{
	return (node->token.tok == TOK_NUM && TypeIsConstant(node->token.type) && TypeGetBasicType(node->token.type));
}


//----------------------------------------------------------------------------
// pop a unary 
//----------------------------------------------------------------------------
inline BOOL PopUnaryOperator(
	PARSE & parse,
	PARSENODE *& a,
	int & basic_type,
	BOOL & is_constant)
{
	a = ParseStackPop(parse.operandstack);
	ASSERT_RETFALSE(a);

	basic_type = TypeGetBasicType(a->token.type);
	is_constant = ParseNodeIsConstant(a);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void PushUnaryOperator(
	PARSE & parse,
	PARSENODE * node,
	PARSENODE * a)
{
	ParseNodeAddChild(node, a);
	node->token.type = a->token.type;
	ParseStackPush(parse.operandstack, node);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BOOL PopBinaryOperator(
	PARSE & parse,
	PARSENODE *& a,
	int & a_basic_type,
	BOOL & a_is_constant,
	PARSENODE *& b,
	int & b_basic_type,
	BOOL & b_is_constant)
{
	b = ParseStackPop(parse.operandstack);
	ASSERT_RETFALSE(b);
	a = ParseStackPop(parse.operandstack);
	ASSERT_RETFALSE(a);

	// note: in some cases we should only be setting is_constant if the other node has no side effects!!!
	a_basic_type = TypeGetBasicType(a->token.type);
	a_is_constant = ParseNodeIsConstant(a);

	b_basic_type = TypeGetBasicType(b->token.type);
	b_is_constant = ParseNodeIsConstant(b);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void PushBinaryConstant(
	PARSE & parse,
	PARSENODE * node,
	PARSENODE * a,
	PARSENODE * b)
{
	ParseNodeFree(node);
	ParseNodeFree(b);
	ParseStackPush(parse.operandstack, a);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void PushBinaryOperator(
	PARSE & parse,
	PARSENODE * node,
	PARSENODE * a,
	PARSENODE * b,
	const TYPE & op_type)
{
	ParseNodeAddChild(node, a);
	ParseNodeAddChild(node, b);
	node->token.type = op_type;
	ParseStackPush(parse.operandstack, node);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BOOL ConvertToLValue(
	PARSE & parse,
	PARSENODE * node,
	BOOL is_constant)
{
	ASSERT_RETFALSE(!is_constant);
	if (node->token.tok == TOK_GLOBALVAR)
	{
		node->token.tok = TOK_GLOBALADR;
	}
	else if (node->token.tok == TOK_LOCALVAR)
	{
		node->token.tok = TOK_LOCALADR;
	}
	else if (node->token.tok == TOK_RSTAT)
	{
		node->token.tok = TOK_LSTAT;
	}
	else if (node->token.tok == TOK_RSTAT_US)
	{
		node->token.tok = TOK_LSTAT_US;
	}
	else if (node->token.tok == TOK_CONTEXTVAR)
	{
		node->token.tok = TOK_CONTEXTVAR;
	}
	else if (node->token.tok == TOK_PERIOD)
	{
		ASSERT_RETFALSE(node->childcount > 0);
		ConvertToLValue(parse, node->children[node->childcount-1], is_constant);
	}
	else
	{
		ASSERT_RETFALSE(0);
	}
	return TRUE;
}


//----------------------------------------------------------------------------
// pop operators/operands from their stacks until we reach either the marker
// operator (typically a paren or bracket) or a higher precedence operator
// popped operators are placed with their operands in tree form back on to
// the operand stack
//----------------------------------------------------------------------------
BOOL PopParseNodes(
	PARSE & parse,
	int precedence,
	int associativity,
	ETOKEN stop1 = TOK_EOF,
	ETOKEN stop2 = TOK_EOF)
{
	while (TRUE)
	{
		PARSENODE * node = ParseStackPeek(parse.operatorstack);
		if (!node)
		{
			return TRUE;
		}
		int p = TokenGetPrecedence(node->token);
		if (node->token.tok == stop1 || node->token.tok == stop2)
		{
			return TRUE;
		}
		if (p > precedence)
		{
			return TRUE;
		}
		if (associativity == 1 && p == precedence)	// right-to-left associativity
		{
			return TRUE;
		}
		ParseStackPop(parse.operatorstack);

		PARSENODE * a = NULL;
		PARSENODE * b = NULL;
		int a_basic_type, b_basic_type;
		BOOL a_is_constant, b_is_constant;
		TYPE op_type;

		switch (node->token.tok)
		{
		case TOK_NEG:
			{
				RETURN_IF_FALSE(PopUnaryOperator(parse, a, a_basic_type, a_is_constant));
				if (a_is_constant)
				{
					switch (a_basic_type)
					{
					case TYPE_CHAR:						OpNeg_CH(a->token.value); break;
					case TYPE_BYTE:						OpNeg_UC(a->token.value); break;
					case TYPE_SHORT:					OpNeg_SH(a->token.value); break;
					case TYPE_INT:						OpNeg_IN(a->token.value); break;
					case TYPE_DWORD:					OpNeg_UI(a->token.value); break;
					case TYPE_INT64:					OpNeg_LL(a->token.value); break;
					case TYPE_QWORD:					OpNeg_UL(a->token.value); break;
					case TYPE_FLOAT:					OpNeg_FL(a->token.value); break;
					case TYPE_DOUBLE:					OpNeg_DB(a->token.value); break;
					case TYPE_LDOUBLE:					OpNeg_LD(a->token.value); break;
					case TYPE_BOOL:						a->token.type.type = TYPE_CONSTANT | TYPE_INT; OpNeg_IN(a->token.value); break;
					default:							ASSERT_RETFALSE(0);
					}
					ParseNodeFree(node);
					ParseStackPush(parse.operandstack, a);
				}
				else
				{
					PushUnaryOperator(parse, node, a);
				}
			}
			break;
		case TOK_TILDE:
			{
				RETURN_IF_FALSE(PopUnaryOperator(parse, a, a_basic_type, a_is_constant));
				if (a_is_constant)
				{
					switch (a_basic_type)
					{
					case TYPE_CHAR:						OpTilde_CH(a->token.value); break;
					case TYPE_BYTE:						OpTilde_UC(a->token.value); break;
					case TYPE_SHORT:					OpTilde_SH(a->token.value); break;
					case TYPE_INT:						OpTilde_IN(a->token.value); break;
					case TYPE_DWORD:					OpTilde_UI(a->token.value); break;
					case TYPE_INT64:					OpTilde_LL(a->token.value); break;
					case TYPE_QWORD:					OpTilde_UL(a->token.value); break;
					case TYPE_BOOL:						a->token.type.type = TYPE_CONSTANT | TYPE_INT; OpTilde_IN(a->token.value); break;
					default:							ASSERT_RETFALSE(0);
					}
					ParseNodeFree(node);
					ParseStackPush(parse.operandstack, a);
				}
				else
				{
					PushUnaryOperator(parse, node, a);
				}
			}
			break;
		case TOK_NOT:
			{
				RETURN_IF_FALSE(PopUnaryOperator(parse, a, a_basic_type, a_is_constant));
				if (a_is_constant)
				{
					switch (a_basic_type)
					{
					case TYPE_CHAR:						OpNot_CH(a->token.value); break;
					case TYPE_BYTE:						OpNot_UC(a->token.value); break;
					case TYPE_SHORT:					OpNot_SH(a->token.value); break;
					case TYPE_INT:						OpNot_IN(a->token.value); break;
					case TYPE_DWORD:					OpNot_UI(a->token.value); break;
					case TYPE_INT64:					OpNot_LL(a->token.value); break;
					case TYPE_QWORD:					OpNot_UL(a->token.value); break;
					case TYPE_BOOL:						a->token.type.type = TYPE_CONSTANT | TYPE_INT; OpNot_IN(a->token.value); break;
					default:							ASSERT_RETFALSE(0);
					}
					ParseNodeFree(node);
					ParseStackPush(parse.operandstack, a);
				}
				else
				{
					PushUnaryOperator(parse, node, a);
				}
			}
			break;
		case TOK_POWER:
			{
				RETURN_IF_FALSE(PopBinaryOperator(parse, a, a_basic_type, a_is_constant, b, b_basic_type, b_is_constant));
				RETURN_IF_FALSE(TypePromotion(node->token, a, b, op_type));
				if (b_is_constant)
				{
					if (IsZero(b->token))
					{
						SetOne(b->token);
						PushBinaryConstant(parse, node, b, a);
						break;
					}
					if (IsOne(b->token))
					{
						PushBinaryConstant(parse, node, a, b);
						break;
					}
				}
				if (a_is_constant)
				{
					if (IsZero(a->token))
					{
						SetZero(a->token);
						PushBinaryConstant(parse, node, a, b);
						break;
					}
					if (IsOne(a->token))
					{
						PushBinaryConstant(parse, node, a, b);
						break;
					}
				}
				if (a_is_constant && b_is_constant)
				{
					ASSERT(a->token.type.type == b->token.type.type);
					switch (a_basic_type)
					{
					case TYPE_CHAR:						OpPow_CH(a->token.value, b->token.value); break;
					case TYPE_BYTE:						OpPow_UC(a->token.value, b->token.value); break;
					case TYPE_SHORT:					OpPow_SH(a->token.value, b->token.value); break;
					case TYPE_INT:						OpPow_IN(a->token.value, b->token.value); break;
					case TYPE_DWORD:					OpPow_UI(a->token.value, b->token.value); break;
					case TYPE_INT64:					OpPow_LL(a->token.value, b->token.value); break;
					case TYPE_QWORD:					OpPow_UL(a->token.value, b->token.value); break;
					case TYPE_FLOAT:					OpPow_FL(a->token.value, b->token.value); break;
					case TYPE_DOUBLE:					OpPow_DB(a->token.value, b->token.value); break;
					case TYPE_LDOUBLE:					OpPow_LD(a->token.value, b->token.value); break;
					default:							ASSERT_RETFALSE(0);
					}
					PushBinaryConstant(parse, node, a, b);
					break;
				}
				PushBinaryOperator(parse, node, a, b, op_type);
			}
			break;
		case TOK_MUL:
			{
				RETURN_IF_FALSE(PopBinaryOperator(parse, a, a_basic_type, a_is_constant, b, b_basic_type, b_is_constant));
				RETURN_IF_FALSE(TypePromotion(node->token, a, b, op_type));
				if (a_is_constant || b_is_constant)
				{
					if (IsZero(a->token) || IsZero(b->token))
					{
						SetZero(a->token);
						PushBinaryConstant(parse, node, a, b);
						break;
					}
					if (IsOne(a->token))
					{
						PushBinaryConstant(parse, node, b, a);
						break;
					}
					if (IsOne(b->token))
					{
						PushBinaryConstant(parse, node, a, b);
						break;
					}
				}
				if (a_is_constant && b_is_constant)
				{
					ASSERT(a->token.type.type == b->token.type.type);
					switch (a_basic_type)
					{
					case TYPE_CHAR:						OpMul_CH(a->token.value, b->token.value); break;
					case TYPE_BYTE:						OpMul_UC(a->token.value, b->token.value); break;
					case TYPE_SHORT:					OpMul_SH(a->token.value, b->token.value); break;
					case TYPE_INT:						OpMul_IN(a->token.value, b->token.value); break;
					case TYPE_DWORD:					OpMul_UI(a->token.value, b->token.value); break;
					case TYPE_INT64:					OpMul_LL(a->token.value, b->token.value); break;
					case TYPE_QWORD:					OpMul_UL(a->token.value, b->token.value); break;
					case TYPE_FLOAT:					OpMul_FL(a->token.value, b->token.value); break;
					case TYPE_DOUBLE:					OpMul_DB(a->token.value, b->token.value); break;
					case TYPE_LDOUBLE:					OpMul_LD(a->token.value, b->token.value); break;
					default:							ASSERT_RETFALSE(0);
					}
					PushBinaryConstant(parse, node, a, b);
					break;
				}
				PushBinaryOperator(parse, node, a, b, op_type);
			}
			break;
		case TOK_DIV:
			{
				RETURN_IF_FALSE(PopBinaryOperator(parse, a, a_basic_type, a_is_constant, b, b_basic_type, b_is_constant));
				RETURN_IF_FALSE(TypePromotion(node->token, a, b, op_type));
				if (b_is_constant)
				{
					if (IsZero(b->token))
					{
						ASSERT_RETFALSE(0);
					}
					if (IsOne(b->token))
					{
						PushBinaryConstant(parse, node, a, b);
						break;
					}
				}
				if (a_is_constant && IsZero(a->token))
				{
					SetZero(a->token);
					PushBinaryConstant(parse, node, a, b);
					break;
				}
				if (a_is_constant && b_is_constant)
				{
					ASSERT(a->token.type.type == b->token.type.type);
					switch (a_basic_type)
					{
					case TYPE_CHAR:						OpDiv_CH(a->token.value, b->token.value); break;
					case TYPE_BYTE:						OpDiv_UC(a->token.value, b->token.value); break;
					case TYPE_SHORT:					OpDiv_SH(a->token.value, b->token.value); break;
					case TYPE_INT:						OpDiv_IN(a->token.value, b->token.value); break;
					case TYPE_DWORD:					OpDiv_UI(a->token.value, b->token.value); break;
					case TYPE_INT64:					OpDiv_LL(a->token.value, b->token.value); break;
					case TYPE_QWORD:					OpDiv_UL(a->token.value, b->token.value); break;
					case TYPE_FLOAT:					OpDiv_FL(a->token.value, b->token.value); break;
					case TYPE_DOUBLE:					OpDiv_DB(a->token.value, b->token.value); break;
					case TYPE_LDOUBLE:					OpDiv_LD(a->token.value, b->token.value); break;
					default:							ASSERT_RETFALSE(0);
					}
					PushBinaryConstant(parse, node, a, b);
					break;
				}
				PushBinaryOperator(parse, node, a, b, op_type);
			}
			break;
		case TOK_MOD:
			{
				RETURN_IF_FALSE(PopBinaryOperator(parse, a, a_basic_type, a_is_constant, b, b_basic_type, b_is_constant));
				RETURN_IF_FALSE(TypePromotion(node->token, a, b, op_type));
				if (b_is_constant)
				{
					if (IsZero(b->token))
					{
						ASSERT_RETFALSE(0);
					}
					if (IsOne(b->token))
					{
						SetZero(b->token);
						PushBinaryConstant(parse, node, b, a);
						break;
					}
				}
				if (a_is_constant)
				{
					if (IsZero(a->token))
					{
						SetZero(a->token);
						PushBinaryConstant(parse, node, a, b);
						break;
					}
					if (IsOne(a->token))
					{
						if (IsOne(b->token))
						{
							SetZero(a->token);
						}
						else
						{
							SetOne(a->token);
						}
						PushBinaryConstant(parse, node, a, b);
						break;
					}
				}
				if (a_is_constant && b_is_constant)
				{
					ASSERT(a->token.type.type == b->token.type.type);
					switch (a_basic_type)
					{
					case TYPE_CHAR:						OpMod_CH(a->token.value, b->token.value); break;
					case TYPE_BYTE:						OpMod_UC(a->token.value, b->token.value); break;
					case TYPE_SHORT:					OpMod_SH(a->token.value, b->token.value); break;
					case TYPE_INT:						OpMod_IN(a->token.value, b->token.value); break;
					case TYPE_DWORD:					OpMod_UI(a->token.value, b->token.value); break;
					case TYPE_INT64:					OpMod_LL(a->token.value, b->token.value); break;
					case TYPE_QWORD:					OpMod_UL(a->token.value, b->token.value); break;
					default:							ASSERT_RETFALSE(0);
					}
					PushBinaryConstant(parse, node, a, b);
					break;
				}
				PushBinaryOperator(parse, node, a, b, op_type);
			}
			break;
		case TOK_ADD:
			{
				RETURN_IF_FALSE(PopBinaryOperator(parse, a, a_basic_type, a_is_constant, b, b_basic_type, b_is_constant));
				RETURN_IF_FALSE(TypePromotion(node->token, a, b, op_type));
				if (a_is_constant && IsZero(a->token))
				{
					PushBinaryConstant(parse, node, b, a);
					break;
				}
				if (b_is_constant && IsZero(b->token))
				{
					PushBinaryConstant(parse, node, a, b);
					break;
				}
				if (a_is_constant && b_is_constant)
				{
					ASSERT(a->token.type.type == b->token.type.type);
					switch (a_basic_type)
					{
					case TYPE_CHAR:						OpAdd_CH(a->token.value, b->token.value); break;
					case TYPE_BYTE:						OpAdd_UC(a->token.value, b->token.value); break;
					case TYPE_SHORT:					OpAdd_SH(a->token.value, b->token.value); break;
					case TYPE_INT:						OpAdd_IN(a->token.value, b->token.value); break;
					case TYPE_DWORD:					OpAdd_UI(a->token.value, b->token.value); break;
					case TYPE_INT64:					OpAdd_LL(a->token.value, b->token.value); break;
					case TYPE_QWORD:					OpAdd_UL(a->token.value, b->token.value); break;
					case TYPE_FLOAT:					OpAdd_FL(a->token.value, b->token.value); break;
					case TYPE_DOUBLE:					OpAdd_DB(a->token.value, b->token.value); break;
					case TYPE_LDOUBLE:					OpAdd_LD(a->token.value, b->token.value); break;
					default:							ASSERT_RETFALSE(0);
					}
					PushBinaryConstant(parse, node, a, b);
					break;
				}
				PushBinaryOperator(parse, node, a, b, op_type);
			}
			break;
		case TOK_SUB:
			{
				RETURN_IF_FALSE(PopBinaryOperator(parse, a, a_basic_type, a_is_constant, b, b_basic_type, b_is_constant));
				RETURN_IF_FALSE(TypePromotion(node->token, a, b, op_type));
				if (b_is_constant && IsZero(b->token))
				{
					PushBinaryConstant(parse, node, a, b);
					break;
				}
				if (a_is_constant && b_is_constant)
				{
					ASSERT(a->token.type.type == b->token.type.type);
					switch (a_basic_type)
					{
					case TYPE_CHAR:						OpSub_CH(a->token.value, b->token.value); break;
					case TYPE_BYTE:						OpSub_UC(a->token.value, b->token.value); break;
					case TYPE_SHORT:					OpSub_SH(a->token.value, b->token.value); break;
					case TYPE_INT:						OpSub_IN(a->token.value, b->token.value); break;
					case TYPE_DWORD:					OpSub_UI(a->token.value, b->token.value); break;
					case TYPE_INT64:					OpSub_LL(a->token.value, b->token.value); break;
					case TYPE_QWORD:					OpSub_UL(a->token.value, b->token.value); break;
					case TYPE_FLOAT:					OpSub_FL(a->token.value, b->token.value); break;
					case TYPE_DOUBLE:					OpSub_DB(a->token.value, b->token.value); break;
					case TYPE_LDOUBLE:					OpSub_LD(a->token.value, b->token.value); break;
					default:							ASSERT_RETFALSE(0);
					}
					PushBinaryConstant(parse, node, a, b);
					break;
				}
				PushBinaryOperator(parse, node, a, b, op_type);
			}
			break;
		case TOK_LSHIFT:
			{
				RETURN_IF_FALSE(PopBinaryOperator(parse, a, a_basic_type, a_is_constant, b, b_basic_type, b_is_constant));
				RETURN_IF_FALSE(TypePromotion(node->token, a, b, op_type));
				if (b_is_constant)
				{
					if (IsZero(b->token))
					{
						PushBinaryConstant(parse, node, a, b);
						break;
					}
					int b_value = ValueGetAsInteger(b->token.value, b->token.type);
					if (a_basic_type == TYPE_INT64 || a_basic_type == TYPE_QWORD)
					{
						if (b_value >= 64)
						{
							SetZero(b->token);
							PushBinaryConstant(parse, node, b, a);
							break;
						}
					}
					else if (b_value >= 32)
					{
						SetZero(b->token);
						PushBinaryConstant(parse, node, b, a);
						break;
					}
				}
				if (a_is_constant && b_is_constant)
				{
					ASSERT(a->token.type.type == b->token.type.type);
					switch (a_basic_type)
					{
					case TYPE_CHAR:						OpLShift_CH(a->token.value, b->token.value); break;
					case TYPE_BYTE:						OpLShift_UC(a->token.value, b->token.value); break;
					case TYPE_SHORT:					OpLShift_SH(a->token.value, b->token.value); break;
					case TYPE_INT:						OpLShift_IN(a->token.value, b->token.value); break;
					case TYPE_DWORD:					OpLShift_UI(a->token.value, b->token.value); break;
					case TYPE_INT64:					OpLShift_LL(a->token.value, b->token.value); break;
					case TYPE_QWORD:					OpLShift_UL(a->token.value, b->token.value); break;
					default:							ASSERT_RETFALSE(0);
					}
					PushBinaryConstant(parse, node, a, b);
					break;
				}
				PushBinaryOperator(parse, node, a, b, op_type);
			}
			break;
		case TOK_RSHIFT:
			{
				RETURN_IF_FALSE(PopBinaryOperator(parse, a, a_basic_type, a_is_constant, b, b_basic_type, b_is_constant));
				RETURN_IF_FALSE(TypePromotion(node->token, a, b, op_type));
				if (b_is_constant)
				{
					if (IsZero(b->token))
					{
						PushBinaryConstant(parse, node, a, b);
						break;
					}
					int b_value = ValueGetAsInteger(b->token.value, b->token.type);
					if (a_basic_type == TYPE_INT64 || a_basic_type == TYPE_QWORD)
					{
						if (b_value >= 64)
						{
							SetZero(b->token);
							PushBinaryConstant(parse, node, b, a);
							break;
						}
					}
					else if (b_value >= 32)
					{
						SetZero(b->token);
						PushBinaryConstant(parse, node, b, a);
						break;
					}
				}
				if (a_is_constant && b_is_constant)
				{
					ASSERT(a->token.type.type == b->token.type.type);
					switch (a_basic_type)
					{
					case TYPE_CHAR:						OpRShift_CH(a->token.value, b->token.value); break;
					case TYPE_BYTE:						OpRShift_UC(a->token.value, b->token.value); break;
					case TYPE_SHORT:					OpRShift_SH(a->token.value, b->token.value); break;
					case TYPE_INT:						OpRShift_IN(a->token.value, b->token.value); break;
					case TYPE_DWORD:					OpRShift_UI(a->token.value, b->token.value); break;
					case TYPE_INT64:					OpRShift_LL(a->token.value, b->token.value); break;
					case TYPE_QWORD:					OpRShift_UL(a->token.value, b->token.value); break;
					default:							ASSERT_RETFALSE(0);
					}
					PushBinaryConstant(parse, node, a, b);
					break;
				}
				PushBinaryOperator(parse, node, a, b, op_type);
			}
			break;
		case TOK_LT:
			{
				RETURN_IF_FALSE(PopBinaryOperator(parse, a, a_basic_type, a_is_constant, b, b_basic_type, b_is_constant));
				RETURN_IF_FALSE(TypePromotion(node->token, a, b, op_type));
				if (a_is_constant && b_is_constant)
				{
					ASSERT(a->token.type.type == b->token.type.type);
					switch (a_basic_type)
					{
					case TYPE_CHAR:						OpLT_CH(a->token.value, b->token.value); break;
					case TYPE_BYTE:						OpLT_UC(a->token.value, b->token.value); break;
					case TYPE_SHORT:					OpLT_SH(a->token.value, b->token.value); break;
					case TYPE_INT:						OpLT_IN(a->token.value, b->token.value); break;
					case TYPE_DWORD:					OpLT_UI(a->token.value, b->token.value); break;
					case TYPE_INT64:					OpLT_LL(a->token.value, b->token.value); break;
					case TYPE_QWORD:					OpLT_UL(a->token.value, b->token.value); break;
					case TYPE_FLOAT:					OpLT_FL(a->token.value, b->token.value); break;
					case TYPE_DOUBLE:					OpLT_DB(a->token.value, b->token.value); break;
					case TYPE_LDOUBLE:					OpLT_LD(a->token.value, b->token.value); break;
					default:							ASSERT_RETFALSE(0);
					}
					TypeSet(a->token.type, TYPE_CONSTANT | TYPE_INT);
					PushBinaryConstant(parse, node, a, b);
					break;
				}
				PushBinaryOperator(parse, node, a, b, op_type);
			}
			break;
		case TOK_GT:
			{
				RETURN_IF_FALSE(PopBinaryOperator(parse, a, a_basic_type, a_is_constant, b, b_basic_type, b_is_constant));
				RETURN_IF_FALSE(TypePromotion(node->token, a, b, op_type));
				if (a_is_constant && b_is_constant)
				{
					ASSERT(a->token.type.type == b->token.type.type);
					switch (a_basic_type)
					{
					case TYPE_CHAR:						OpGT_CH(a->token.value, b->token.value); break;
					case TYPE_BYTE:						OpGT_UC(a->token.value, b->token.value); break;
					case TYPE_SHORT:					OpGT_SH(a->token.value, b->token.value); break;
					case TYPE_INT:						OpGT_IN(a->token.value, b->token.value); break;
					case TYPE_DWORD:					OpGT_UI(a->token.value, b->token.value); break;
					case TYPE_INT64:					OpGT_LL(a->token.value, b->token.value); break;
					case TYPE_QWORD:					OpGT_UL(a->token.value, b->token.value); break;
					case TYPE_FLOAT:					OpGT_FL(a->token.value, b->token.value); break;
					case TYPE_DOUBLE:					OpGT_DB(a->token.value, b->token.value); break;
					case TYPE_LDOUBLE:					OpGT_LD(a->token.value, b->token.value); break;
					default:							ASSERT_RETFALSE(0);
					}
					TypeSet(a->token.type, TYPE_CONSTANT | TYPE_INT);
					PushBinaryConstant(parse, node, a, b);
					break;
				}
				PushBinaryOperator(parse, node, a, b, op_type);
			}
			break;
		case TOK_LTE:
			{
				RETURN_IF_FALSE(PopBinaryOperator(parse, a, a_basic_type, a_is_constant, b, b_basic_type, b_is_constant));
				RETURN_IF_FALSE(TypePromotion(node->token, a, b, op_type));
				if (a_is_constant && b_is_constant)
				{
					ASSERT(a->token.type.type == b->token.type.type);
					switch (a_basic_type)
					{
					case TYPE_CHAR:						OpLTE_CH(a->token.value, b->token.value); break;
					case TYPE_BYTE:						OpLTE_UC(a->token.value, b->token.value); break;
					case TYPE_SHORT:					OpLTE_SH(a->token.value, b->token.value); break;
					case TYPE_INT:						OpLTE_IN(a->token.value, b->token.value); break;
					case TYPE_DWORD:					OpLTE_UI(a->token.value, b->token.value); break;
					case TYPE_INT64:					OpLTE_LL(a->token.value, b->token.value); break;
					case TYPE_QWORD:					OpLTE_UL(a->token.value, b->token.value); break;
					case TYPE_FLOAT:					OpLTE_FL(a->token.value, b->token.value); break;
					case TYPE_DOUBLE:					OpLTE_DB(a->token.value, b->token.value); break;
					case TYPE_LDOUBLE:					OpLTE_LD(a->token.value, b->token.value); break;
					default:							ASSERT_RETFALSE(0);
					}
					TypeSet(a->token.type, TYPE_CONSTANT | TYPE_INT);
					PushBinaryConstant(parse, node, a, b);
					break;
				}
				PushBinaryOperator(parse, node, a, b, op_type);
			}
			break;
		case TOK_GTE:
			{
				RETURN_IF_FALSE(PopBinaryOperator(parse, a, a_basic_type, a_is_constant, b, b_basic_type, b_is_constant));
				RETURN_IF_FALSE(TypePromotion(node->token, a, b, op_type));
				if (a_is_constant && b_is_constant)
				{
					ASSERT(a->token.type.type == b->token.type.type);
					switch (a_basic_type)
					{
					case TYPE_CHAR:						OpGTE_CH(a->token.value, b->token.value); break;
					case TYPE_BYTE:						OpGTE_UC(a->token.value, b->token.value); break;
					case TYPE_SHORT:					OpGTE_SH(a->token.value, b->token.value); break;
					case TYPE_INT:						OpGTE_IN(a->token.value, b->token.value); break;
					case TYPE_DWORD:					OpGTE_UI(a->token.value, b->token.value); break;
					case TYPE_INT64:					OpGTE_LL(a->token.value, b->token.value); break;
					case TYPE_QWORD:					OpGTE_UL(a->token.value, b->token.value); break;
					case TYPE_FLOAT:					OpGTE_FL(a->token.value, b->token.value); break;
					case TYPE_DOUBLE:					OpGTE_DB(a->token.value, b->token.value); break;
					case TYPE_LDOUBLE:					OpGTE_LD(a->token.value, b->token.value); break;
					default:							ASSERT_RETFALSE(0);
					}
					TypeSet(a->token.type, TYPE_CONSTANT | TYPE_INT);
					PushBinaryConstant(parse, node, a, b);
					break;
				}
				PushBinaryOperator(parse, node, a, b, op_type);
			}
			break;
		case TOK_EQ:
			{
				RETURN_IF_FALSE(PopBinaryOperator(parse, a, a_basic_type, a_is_constant, b, b_basic_type, b_is_constant));
				RETURN_IF_FALSE(TypePromotion(node->token, a, b, op_type));
				if (a_is_constant && b_is_constant)
				{
					ASSERT(a->token.type.type == b->token.type.type);
					switch (a_basic_type)
					{
					case TYPE_CHAR:						OpEQ_CH(a->token.value, b->token.value); break;
					case TYPE_BYTE:						OpEQ_UC(a->token.value, b->token.value); break;
					case TYPE_SHORT:					OpEQ_SH(a->token.value, b->token.value); break;
					case TYPE_INT:						OpEQ_IN(a->token.value, b->token.value); break;
					case TYPE_DWORD:					OpEQ_UI(a->token.value, b->token.value); break;
					case TYPE_INT64:					OpEQ_LL(a->token.value, b->token.value); break;
					case TYPE_QWORD:					OpEQ_UL(a->token.value, b->token.value); break;
					case TYPE_FLOAT:					OpEQ_FL(a->token.value, b->token.value); break;
					case TYPE_DOUBLE:					OpEQ_DB(a->token.value, b->token.value); break;
					case TYPE_LDOUBLE:					OpEQ_LD(a->token.value, b->token.value); break;
					default:							ASSERT_RETFALSE(0);
					}
					TypeSet(a->token.type, TYPE_CONSTANT | TYPE_INT);
					PushBinaryConstant(parse, node, a, b);
					break;
				}
				PushBinaryOperator(parse, node, a, b, op_type);
			}
			break;
		case TOK_NE:
			{
				RETURN_IF_FALSE(PopBinaryOperator(parse, a, a_basic_type, a_is_constant, b, b_basic_type, b_is_constant));
				RETURN_IF_FALSE(TypePromotion(node->token, a, b, op_type));
				if (a_is_constant && b_is_constant)
				{
					ASSERT(a->token.type.type == b->token.type.type);
					switch (a_basic_type)
					{
					case TYPE_CHAR:						OpNEQ_CH(a->token.value, b->token.value); break;
					case TYPE_BYTE:						OpNEQ_UC(a->token.value, b->token.value); break;
					case TYPE_SHORT:					OpNEQ_SH(a->token.value, b->token.value); break;
					case TYPE_INT:						OpNEQ_IN(a->token.value, b->token.value); break;
					case TYPE_DWORD:					OpNEQ_UI(a->token.value, b->token.value); break;
					case TYPE_INT64:					OpNEQ_LL(a->token.value, b->token.value); break;
					case TYPE_QWORD:					OpNEQ_UL(a->token.value, b->token.value); break;
					case TYPE_FLOAT:					OpNEQ_FL(a->token.value, b->token.value); break;
					case TYPE_DOUBLE:					OpNEQ_DB(a->token.value, b->token.value); break;
					case TYPE_LDOUBLE:					OpNEQ_LD(a->token.value, b->token.value); break;
					default:							ASSERT_RETFALSE(0);
					}
					TypeSet(a->token.type, TYPE_CONSTANT | TYPE_INT);
					PushBinaryConstant(parse, node, a, b);
					break;
				}
				PushBinaryOperator(parse, node, a, b, op_type);
			}
			break;
		case TOK_AND:
			{
				RETURN_IF_FALSE(PopBinaryOperator(parse, a, a_basic_type, a_is_constant, b, b_basic_type, b_is_constant));
				RETURN_IF_FALSE(TypePromotion(node->token, a, b, op_type));
				if (b_is_constant && IsZero(b->token))
				{
					SetZero(b->token);
					PushBinaryConstant(parse, node, b, a);
					break;
				}
				if (a_is_constant && IsZero(a->token))
				{
					SetZero(a->token);
					PushBinaryConstant(parse, node, a, b);
					break;
				}
				if (a_is_constant && b_is_constant)
				{
					ASSERT(a->token.type.type == b->token.type.type);
					switch (a_basic_type)
					{
					case TYPE_CHAR:						OpBAnd_CH(a->token.value, b->token.value); break;
					case TYPE_BYTE:						OpBAnd_UC(a->token.value, b->token.value); break;
					case TYPE_SHORT:					OpBAnd_SH(a->token.value, b->token.value); break;
					case TYPE_INT:						OpBAnd_IN(a->token.value, b->token.value); break;
					case TYPE_DWORD:					OpBAnd_UI(a->token.value, b->token.value); break;
					case TYPE_INT64:					OpBAnd_LL(a->token.value, b->token.value); break;
					case TYPE_QWORD:					OpBAnd_UL(a->token.value, b->token.value); break;
					default:							ASSERT_RETFALSE(0);
					}
					PushBinaryConstant(parse, node, a, b);
					break;
				}
				PushBinaryOperator(parse, node, a, b, op_type);
			}
			break;
		case TOK_XOR:
			{
				RETURN_IF_FALSE(PopBinaryOperator(parse, a, a_basic_type, a_is_constant, b, b_basic_type, b_is_constant));
				RETURN_IF_FALSE(TypePromotion(node->token, a, b, op_type));
				if (a_is_constant && b_is_constant)
				{
					ASSERT(a->token.type.type == b->token.type.type);
					switch (a_basic_type)
					{
					case TYPE_CHAR:						OpBXor_CH(a->token.value, b->token.value); break;
					case TYPE_BYTE:						OpBXor_UC(a->token.value, b->token.value); break;
					case TYPE_SHORT:					OpBXor_SH(a->token.value, b->token.value); break;
					case TYPE_INT:						OpBXor_IN(a->token.value, b->token.value); break;
					case TYPE_DWORD:					OpBXor_UI(a->token.value, b->token.value); break;
					case TYPE_INT64:					OpBXor_LL(a->token.value, b->token.value); break;
					case TYPE_QWORD:					OpBXor_UL(a->token.value, b->token.value); break;
					default:							ASSERT_RETFALSE(0);
					}
					PushBinaryConstant(parse, node, a, b);
					break;
				}
				PushBinaryOperator(parse, node, a, b, op_type);
			}
			break;
		case TOK_OR:
			{
				RETURN_IF_FALSE(PopBinaryOperator(parse, a, a_basic_type, a_is_constant, b, b_basic_type, b_is_constant));
				RETURN_IF_FALSE(TypePromotion(node->token, a, b, op_type));
				if (a_is_constant && IsZero(a->token))
				{
					PushBinaryConstant(parse, node, b, a);
					break;
				}
				if (b_is_constant && IsZero(b->token))
				{
					PushBinaryConstant(parse, node, a, b);
					break;
				}
				if (a_is_constant && b_is_constant)
				{
					ASSERT(a->token.type.type == b->token.type.type);
					switch (a_basic_type)
					{
					case TYPE_CHAR:						OpBOr_CH(a->token.value, b->token.value); break;
					case TYPE_BYTE:						OpBOr_UC(a->token.value, b->token.value); break;
					case TYPE_SHORT:					OpBOr_SH(a->token.value, b->token.value); break;
					case TYPE_INT:						OpBOr_IN(a->token.value, b->token.value); break;
					case TYPE_DWORD:					OpBOr_UI(a->token.value, b->token.value); break;
					case TYPE_INT64:					OpBOr_LL(a->token.value, b->token.value); break;
					case TYPE_QWORD:					OpBOr_UL(a->token.value, b->token.value); break;
					default:							ASSERT_RETFALSE(0);
					}
					PushBinaryConstant(parse, node, a, b);
					break;
				}
				PushBinaryOperator(parse, node, a, b, op_type);
			}
			break;
		case TOK_LAND:
			{
				RETURN_IF_FALSE(PopBinaryOperator(parse, a, a_basic_type, a_is_constant, b, b_basic_type, b_is_constant));
				RETURN_IF_FALSE(TypePromotion(node->token, a, b, op_type));
				if (a_is_constant && IsZero(a->token))
				{
					TypeSet(a->token.type, TYPE_CONSTANT | TYPE_INT);
					SetZero(a->token);
					PushBinaryConstant(parse, node, a, b);
					break;
				}
				if (b_is_constant && IsZero(b->token))
				{
					TypeSet(b->token.type, TYPE_CONSTANT | TYPE_INT);
					SetZero(b->token);
					PushBinaryConstant(parse, node, b, a);
				}
				if (a_is_constant && b_is_constant)
				{
					ASSERT(a->token.type.type == b->token.type.type);
					switch (a_basic_type)
					{
					case TYPE_CHAR:						OpLAnd_CH(a->token.value, b->token.value); break;
					case TYPE_BYTE:						OpLAnd_UC(a->token.value, b->token.value); break;
					case TYPE_SHORT:					OpLAnd_SH(a->token.value, b->token.value); break;
					case TYPE_INT:						OpLAnd_IN(a->token.value, b->token.value); break;
					case TYPE_DWORD:					OpLAnd_UI(a->token.value, b->token.value); break;
					case TYPE_INT64:					OpLAnd_LL(a->token.value, b->token.value); break;
					case TYPE_QWORD:					OpLAnd_UL(a->token.value, b->token.value); break;
					case TYPE_FLOAT:					OpLAnd_FL(a->token.value, b->token.value); break;
					case TYPE_DOUBLE:					OpLAnd_DB(a->token.value, b->token.value); break;
					case TYPE_LDOUBLE:					OpLAnd_LD(a->token.value, b->token.value); break;
					default:							ASSERT_RETFALSE(0);
					}
					TypeSet(a->token.type, TYPE_CONSTANT | TYPE_INT);
					PushBinaryConstant(parse, node, a, b);
					break;
				}
				PushBinaryOperator(parse, node, a, b, op_type);
			}
			break;
		case TOK_LOR:
			{
				RETURN_IF_FALSE(PopBinaryOperator(parse, a, a_basic_type, a_is_constant, b, b_basic_type, b_is_constant));
				RETURN_IF_FALSE(TypePromotion(node->token, a, b, op_type));
				if (a_is_constant && !IsZero(a->token))
				{
					TypeSet(a->token.type, TYPE_CONSTANT | TYPE_INT);
					SetOne(a->token);
					PushBinaryConstant(parse, node, a, b);
					break;
				}
				if (b_is_constant && !IsZero(b->token))
				{
					TypeSet(b->token.type, TYPE_CONSTANT | TYPE_INT);
					SetOne(b->token);
					PushBinaryConstant(parse, node, b, a);
					break;
				}
				if (a_is_constant && b_is_constant)
				{
					ASSERT(a->token.type.type == b->token.type.type);
					switch (a_basic_type)
					{
					case TYPE_CHAR:						OpLOr_CH(a->token.value, b->token.value); break;
					case TYPE_BYTE:						OpLOr_UC(a->token.value, b->token.value); break;
					case TYPE_SHORT:					OpLOr_SH(a->token.value, b->token.value); break;
					case TYPE_INT:						OpLOr_IN(a->token.value, b->token.value); break;
					case TYPE_DWORD:					OpLOr_UI(a->token.value, b->token.value); break;
					case TYPE_INT64:					OpLOr_LL(a->token.value, b->token.value); break;
					case TYPE_QWORD:					OpLOr_UL(a->token.value, b->token.value); break;
					case TYPE_FLOAT:					OpLOr_FL(a->token.value, b->token.value); break;
					case TYPE_DOUBLE:					OpLOr_DB(a->token.value, b->token.value); break;
					case TYPE_LDOUBLE:					OpLOr_LD(a->token.value, b->token.value); break;
					default:							ASSERT_RETFALSE(0);
					}
					TypeSet(a->token.type, TYPE_CONSTANT | TYPE_INT);
					PushBinaryConstant(parse, node, a, b);
					break;
				}
				PushBinaryOperator(parse, node, a, b, op_type);
			}
			break;
		case TOK_ASSIGN:
			{
				RETURN_IF_FALSE(PopBinaryOperator(parse, a, a_basic_type, a_is_constant, b, b_basic_type, b_is_constant));
				op_type = a->token.type;
				RETURN_IF_FALSE(TypeConversion(b, op_type));
				RETURN_IF_FALSE(ConvertToLValue(parse, a, a_is_constant));
				PushBinaryOperator(parse, node, a, b, op_type);
			}
			break;
		case TOK_A_MUL:
			{
				RETURN_IF_FALSE(PopBinaryOperator(parse, a, a_basic_type, a_is_constant, b, b_basic_type, b_is_constant));
				if (b_is_constant && IsOne(b->token))
				{
					PushBinaryConstant(parse, node, a, b);
					break;
				}
				op_type = a->token.type;
				RETURN_IF_FALSE(TypeConversion(b, op_type));
				RETURN_IF_FALSE(ConvertToLValue(parse, a, a_is_constant));
				PushBinaryOperator(parse, node, a, b, op_type);
			}
			break;
		case TOK_A_DIV:
			{
				RETURN_IF_FALSE(PopBinaryOperator(parse, a, a_basic_type, a_is_constant, b, b_basic_type, b_is_constant));
				if (b_is_constant)
				{
					if (IsZero(b->token))
					{
						ASSERT_RETFALSE(0);
					}
					if (IsOne(b->token))
					{
						PushBinaryConstant(parse, node, a, b);
						break;
					}
				}
				op_type = a->token.type;
				RETURN_IF_FALSE(TypeConversion(b, op_type));
				RETURN_IF_FALSE(ConvertToLValue(parse, a, a_is_constant));
				PushBinaryOperator(parse, node, a, b, op_type);
			}
			break;
		case TOK_A_MOD:
			{
				RETURN_IF_FALSE(PopBinaryOperator(parse, a, a_basic_type, a_is_constant, b, b_basic_type, b_is_constant));
				if (b_is_constant)
				{
					if (IsZero(b->token))
					{
						ASSERT_RETFALSE(0);
					}
					if (IsOne(b->token))
					{
						SetZero(b->token);
						PushBinaryConstant(parse, node, b, a);
						break;
					}
				}
				op_type = a->token.type;
				RETURN_IF_FALSE(TypeConversion(b, op_type));
				RETURN_IF_FALSE(ConvertToLValue(parse, a, a_is_constant));
				PushBinaryOperator(parse, node, a, b, op_type);
			}
			break;
		case TOK_A_ADD:
			{
				RETURN_IF_FALSE(PopBinaryOperator(parse, a, a_basic_type, a_is_constant, b, b_basic_type, b_is_constant));
				if (b_is_constant && IsZero(b->token))
				{
					PushBinaryConstant(parse, node, a, b);
					break;
				}
				op_type = a->token.type;
				RETURN_IF_FALSE(TypeConversion(b, op_type));
				RETURN_IF_FALSE(ConvertToLValue(parse, a, a_is_constant));
				PushBinaryOperator(parse, node, a, b, op_type);
			}
			break;
		case TOK_A_SUB:
			{
				RETURN_IF_FALSE(PopBinaryOperator(parse, a, a_basic_type, a_is_constant, b, b_basic_type, b_is_constant));
				if (b_is_constant && IsZero(b->token))
				{
					PushBinaryConstant(parse, node, a, b);
					break;
				}
				op_type = a->token.type;
				RETURN_IF_FALSE(TypeConversion(b, op_type));
				RETURN_IF_FALSE(ConvertToLValue(parse, a, a_is_constant));
				PushBinaryOperator(parse, node, a, b, op_type);
			}
			break;
		case TOK_A_LSHIFT:
			{
				RETURN_IF_FALSE(PopBinaryOperator(parse, a, a_basic_type, a_is_constant, b, b_basic_type, b_is_constant));
				if (b_is_constant && IsZero(b->token))
				{
					PushBinaryConstant(parse, node, a, b);
					break;
				}
				op_type = a->token.type;
				RETURN_IF_FALSE(TypeConversion(b, op_type));
				RETURN_IF_FALSE(ConvertToLValue(parse, a, a_is_constant));
				PushBinaryOperator(parse, node, a, b, op_type);
			}
			break;
		case TOK_A_RSHIFT:
			{
				RETURN_IF_FALSE(PopBinaryOperator(parse, a, a_basic_type, a_is_constant, b, b_basic_type, b_is_constant));
				if (b_is_constant && IsZero(b->token))
				{
					PushBinaryConstant(parse, node, a, b);
					break;
				}
				op_type = a->token.type;
				RETURN_IF_FALSE(TypeConversion(b, op_type));
				RETURN_IF_FALSE(ConvertToLValue(parse, a, a_is_constant));
				PushBinaryOperator(parse, node, a, b, op_type);
			}
			break;
		case TOK_A_AND:
			{
				RETURN_IF_FALSE(PopBinaryOperator(parse, a, a_basic_type, a_is_constant, b, b_basic_type, b_is_constant));
				if (b_is_constant && IsZero(b->token))
				{
					SetZero(b->token);
					PushBinaryConstant(parse, node, b, a);
					break;
				}
				op_type = a->token.type;
				RETURN_IF_FALSE(TypeConversion(b, op_type));
				RETURN_IF_FALSE(ConvertToLValue(parse, a, a_is_constant));
				PushBinaryOperator(parse, node, a, b, op_type);
			}
			break;
		case TOK_A_XOR:
			{
				RETURN_IF_FALSE(PopBinaryOperator(parse, a, a_basic_type, a_is_constant, b, b_basic_type, b_is_constant));
				op_type = a->token.type;
				RETURN_IF_FALSE(TypeConversion(b, op_type));
				RETURN_IF_FALSE(ConvertToLValue(parse, a, a_is_constant));
				PushBinaryOperator(parse, node, a, b, op_type);
			}
			break;
		case TOK_A_OR:
			{
				RETURN_IF_FALSE(PopBinaryOperator(parse, a, a_basic_type, a_is_constant, b, b_basic_type, b_is_constant));
				if (b_is_constant && IsZero(b->token))
				{
					PushBinaryConstant(parse, node, a, b);
					break;
				}
				op_type = a->token.type;
				RETURN_IF_FALSE(TypeConversion(b, op_type));
				RETURN_IF_FALSE(ConvertToLValue(parse, a, a_is_constant));
				PushBinaryOperator(parse, node, a, b, op_type);
			}
			break;
		case TOK_PERIOD:
			{
				RETURN_IF_FALSE(PopBinaryOperator(parse, a, a_basic_type, a_is_constant, b, b_basic_type, b_is_constant));
				if (a->token.type.type == TYPE_UNIT && isRstat(b))
				{
					op_type = b->token.type;
					PushBinaryOperator(parse, node, a, b, op_type);
					break;
				}
				// period.1 for example
				if (isRstat(a))
				{
					int stat = STAT_GET_STAT(a->token.value.i);
					const STATS_DATA * stat_data = StatsGetData(NULL, stat);
					ASSERT_RETFALSE(stat_data);

					if (b_is_constant)
					{
						// modify a, free node && b, push a on operand stack
						ASSERT_RETFALSE(a->curparam == 0);
						ASSERT_RETFALSE(b->token.value.i >= 0 && ((b->token.value.i & (int)stat_data->m_nParamGetMask[0]) == b->token.value.i));
						a->token.value.i = MAKE_STAT(STAT_GET_STAT(a->token.value.i), b->token.value.i);
						PushBinaryConstant(parse, node, a, b);		// treating a as a constant, push a
						break;
					}
					// handle non-constant stat param
					ASSERT(0);
					break;
				}
				ASSERT(0);
			}
			break;
		case TOK_COMMA:
			{
				PARSENODE * funcnode = ParseStackPeekFunction(parse, parse.operatorstack);
				if (funcnode)
				{
					funcnode->curparam++;
					ParseNodeFree(node);
					break;
				}
				RETURN_IF_FALSE(PopBinaryOperator(parse, a, a_basic_type, a_is_constant, b, b_basic_type, b_is_constant));
				if (a_is_constant)
				{
					PushBinaryConstant(parse, node, b, a);
					break;
				}
				op_type = b->token.type;
				PushBinaryOperator(parse, node, a, b, op_type);
			}
			break;
		case TOK_TERNARY:
			{
				// need to change to not evaluate else path
				PARSENODE * c = ParseStackPop(parse.operandstack);
				ASSERT_RETFALSE(c);
				PARSENODE * b = ParseStackPop(parse.operandstack);
				ASSERT_RETFALSE(b);
				PARSENODE * a = ParseStackPop(parse.operandstack);
				ASSERT_RETFALSE(a);

				// type promotion
				TYPE op_type;
				ASSERT_RETFALSE(TypePromotion(node->token, b, c, op_type));

				int a_isconstant = ParseNodeIsConstant(a);

				if (a_isconstant)
				{
					ParseNodeFree(node);
					if (!IsZero(a->token))
					{
						ParseNodeFree(c);
						ParseStackPush(parse.operandstack, b);
					}
					else
					{
						ParseNodeFree(b);
						ParseStackPush(parse.operandstack, c);
					}
					break;
				}

				ParseNodeAddChild(node, a);
				ParseNodeAddChild(node, b);
				ParseNodeAddChild(node, c);
				node->token.type = op_type;
				ParseStackPush(parse.operandstack, node);
			}
			break;
		default:
			SASSERT_RETFALSE(0, "Unrecognized token");
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ParseFunctionMatch(
	PARSE & parse,
	PARSENODE * funcnode,
	FUNCTION * function,
	BOOL & match)
{
	match = FALSE;

	ASSERT_RETFALSE(funcnode);
	ASSERT_RETFALSE(function);
	ASSERT_RETFALSE(funcnode->token.tok == TOK_FUNCTION || funcnode->token.tok == TOK_IFUNCTION);
	ASSERT_RETFALSE(funcnode->token.value.sym);
	int actual_paramcount = parse.operandstack.count - funcnode->paramcount;
	ASSERT_RETFALSE(actual_paramcount >= 0);

	if (actual_paramcount > function->parameter_count)
	{
		return TRUE;
	}

	PARSENODE * params[MAX_FUNCTION_PARAMS];
	int total_paramcount = 0;
	int paramcount = 0;
	int stat = INVALID_LINK;
	match = TRUE;

	for (int ii = function->parameter_count - 1; ii >= 0; ii--)
	{
		SSYMBOL * paramsym = FunctionGetParameter(function, ii);
		BOOL matchparam = FALSE;
		switch (paramsym->parameter_type)
		{
		default:
			matchparam = TRUE;
			break;
		case PARAM_NORMAL:
			{
				if (actual_paramcount <= 0)
				{
					break;
				}
				PARSENODE * param = ParseStackPop(parse.operandstack);
				ASSERT_RETFALSE(param);
				params[paramcount++] = param;
				actual_paramcount--;
				matchparam = TRUE;
			}
			break;
		case PARAM_INDEX:
			{
				PARSENODE * param = ParseStackPop(parse.operandstack);
				ASSERT_RETFALSE(param);
				params[paramcount++] = param;
				actual_paramcount--;

				if (param->token.tok == TOK_NUM)
				{
					ASSERT_RETFALSE(param->token.type.type == int_type.type);
					ASSERT_RETFALSE(param->token.value.i >= 0 && param->token.value.i < (int)ExcelGetCount(NULL, (EXCELTABLE)paramsym->index_table));
					if (paramsym->index_table == DATATABLE_STATS && paramsym->index_index == 0)
					{
						stat = param->token.value.i;
					}
					matchparam = TRUE;;
				}
				else if (param->token.tok == TOK_IDENT)
				{
					ASSERT_RETFALSE(param->token.value.sym);
					int idx = ExcelGetLineByStringIndex(NULL, (EXCELTABLE)paramsym->index_table, paramsym->index_index, param->token.value.sym->str);
					ASSERT_RETFALSE(idx != INVALID_LINK);
					if (idx > INVALID_LINK)
					{
						if (paramsym->index_table == DATATABLE_STATS && paramsym->index_index == 0)
						{
							stat = idx;
						}
						matchparam = TRUE;;
					}
				}
				else if (isRstat(param) && paramsym->index_table == DATATABLE_STATS && paramsym->index_index == 0)
				{
					stat = STAT_GET_STAT(param->token.value.i);
					matchparam = TRUE;
				}
			}
			break;
		case PARAM_STAT_PARAM:
			{
				PARSENODE * param = ParseStackPop(parse.operandstack);
				ASSERT_RETFALSE(param);
				params[paramcount++] = param;
				actual_paramcount--;

				if (param->token.tok == TOK_NUM)
				{
					matchparam = TRUE;
					break;
				}
				if (stat <= INVALID_LINK)
				{
					break;
				}
				const STATS_DATA * stat_data = StatsGetData(NULL, stat);
				ASSERT_RETFALSE(stat_data);
				if (stat_data->m_nParamTable[0] <= INVALID_LINK)
				{
					break;
				}
				if (param->token.tok == TOK_IDENT)
				{
					ASSERT_RETFALSE(param->token.value.sym);
					int idx = ExcelGetLineByStringIndex(NULL, (EXCELTABLE)stat_data->m_nParamTable[0], param->token.value.sym->str);
					if (idx > INVALID_LINK)
					{
						matchparam = TRUE;
					}
				}
				else if (isRstat(param) && stat_data->m_nParamTable[0] == DATATABLE_STATS)
				{
					matchparam = TRUE;
				}
			}
			break;
		}
		if (!matchparam)
		{
			match = FALSE;
			break;
		}
		total_paramcount++;
	}

	if (!match || function->parameter_count != total_paramcount || actual_paramcount != 0)
	{
		for (int ii = paramcount - 1; ii >= 0; ii--)
		{
			ParseStackPush(parse.operandstack, params[ii]);
		}
		return TRUE;
	}

	stat = INVALID_LINK;
	actual_paramcount = paramcount;
	paramcount = 0;
	for (int ii = function->parameter_count - 1; ii >= 0; ii--)
	{
		SSYMBOL * paramsym = FunctionGetParameter(function, ii);
		switch (paramsym->parameter_type)
		{
		case PARAM_NORMAL:
			{
				ASSERT_RETFALSE(paramcount < actual_paramcount);
				PARSENODE * param = params[paramcount++];
				ASSERT_RETFALSE(param);

				ParseNodeAddChildFront(funcnode, param);
			}
			break;
		case PARAM_INDEX:
			{
				ASSERT_RETFALSE(paramcount < actual_paramcount);
				PARSENODE * param = params[paramcount++];
				ASSERT_RETFALSE(param);

				if (param->token.tok == TOK_NUM)
				{
					ASSERT_RETFALSE(param->token.value.i >= 0 && param->token.value.i < (int)ExcelGetCount(NULL, (EXCELTABLE)paramsym->index_table));
					ASSERT_RETFALSE(param->token.type.type == int_type.type);
					if (paramsym->index_table == DATATABLE_STATS && paramsym->index_index == 0)
					{
						stat = param->token.value.i;
					}
				}
				else if (param->token.tok == TOK_IDENT)
				{
					ASSERT_RETFALSE(param->token.value.sym);
					int idx = ExcelGetLineByStringIndex(NULL, (EXCELTABLE)paramsym->index_table, paramsym->index_index, param->token.value.sym->str);
					if (paramsym->index_table == DATATABLE_STATS && paramsym->index_index == 0)
					{
						stat = idx;
					}
					param->token.tok = TOK_NUM;
					param->token.type = int_type;
					param->token.value.i = idx;
				}
				else if (isRstat(param) && paramsym->index_table == DATATABLE_STATS && paramsym->index_index == 0)
				{
					stat = STAT_GET_STAT(param->token.value.i);
					param->token.tok = TOK_NUM;
					param->token.type = int_type;
					param->token.value.i = stat;
				}
				else
				{
					ASSERT_RETFALSE(0);
				}
				ParseNodeAddChildFront(funcnode, param);
			}
			break;
		case PARAM_STAT_PARAM:
			{
				ASSERT_RETFALSE(paramcount < actual_paramcount);
				PARSENODE * param = params[paramcount++];
				ASSERT_RETFALSE(param);

				if (param->token.tok == TOK_NUM)
				{
					ParseNodeAddChildFront(funcnode, param);
					break;
				}
				ASSERT_RETFALSE(stat > INVALID_LINK);
				const STATS_DATA * stat_data = StatsGetData(NULL, stat);
				ASSERT_RETFALSE(stat_data);
				ASSERT_RETFALSE(stat_data->m_nParamTable[0] > INVALID_LINK);
				if (param->token.tok == TOK_IDENT)
				{
					ASSERT_RETFALSE(param->token.value.sym);
					int idx = ExcelGetLineByStringIndex(NULL, (EXCELTABLE)stat_data->m_nParamTable[0], param->token.value.sym->str);
					ASSERT_RETFALSE(idx > INVALID_LINK);
					param->token.tok = TOK_NUM;
					param->token.type = int_type;
					param->token.value.i = idx;
				}
				else if (isRstat(param) && stat_data->m_nParamTable[0] == DATATABLE_STATS)
				{
					stat = STAT_GET_STAT(param->token.value.i);
					param->token.tok = TOK_NUM;
					param->token.type = int_type;
					param->token.value.i = stat;
				}
				ParseNodeAddChildFront(funcnode, param);
			}
			break;
		default:
			break;
		}
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ParseFunctionCall(
	PARSE & parse)
{
	PARSENODE * funcnode = ParseStackPop(parse.operatorstack);
	ASSERT_RETFALSE(funcnode);
	ASSERT_RETFALSE(funcnode->token.tok == TOK_FUNCTION || funcnode->token.tok == TOK_IFUNCTION);
	int paramcount = parse.operandstack.count - funcnode->paramcount;
	ASSERT_RETFALSE(paramcount >= 0);
	// should do parameter matching here
	ASSERT_RETFALSE(funcnode->token.value.sym);
	SSYMBOL * funcsym = funcnode->token.value.sym;
	while (funcsym)
	{
		BOOL match = FALSE;
		if (!ParseFunctionMatch(parse, funcnode, funcsym->function, match))
		{
			break;
		}
		if (match)
		{
			funcnode->token.value.sym = funcsym;
			ParseStackPush(parse.operandstack, funcnode);
			parse.bLastSymbolWasTerm = TRUE;
			return TRUE;
		}
		funcsym = SymbolTableFindPrev(gVM.SymbolTable, funcsym, TOK_LAST);
	}

	ParseNodeFree(funcnode);
	SASSERT_RETFALSE(0, "Parameter Mismatch in function call");
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PopParens(
	PARSE & parse)
{
	PopParseNodes(parse, TokenDef[TOK_RPAREN].precedence, 0, TOK_LPAREN);
	PARSENODE * node = ParseStackPeek(parse.operatorstack);
	if (!node)
	{
		return FALSE;
	}
	if (node->token.tok == TOK_LPAREN)
	{
		ParseStackPop(parse.operatorstack);
		ParseNodeFree(node);
		return TRUE;
	}
	if (node->token.tok == TOK_FUNCTION || node->token.tok == TOK_IFUNCTION)
	{
		return ParseFunctionCall(parse);
	}
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PopBracket(
	PARSE & parse)
{
	PopParseNodes(parse, TokenDef[TOK_RPAREN].precedence, 0, TOK_LBRACKET);
	PARSENODE * node = ParseStackPeek(parse.operatorstack);
	if (!node)
	{
		return FALSE;
	}
	if (node->token.tok == TOK_LBRACKET)
	{
		ParseStackPop(parse.operatorstack);
		ParseNodeFree(node);

		PARSENODE * index = ParseStackPop(parse.operandstack);
		ASSERT_RETFALSE(index);
		ASSERT_RETFALSE(TypeIsIntegral(index->token.type));
		int index_isconstant = ParseNodeIsConstant(index);

		PARSENODE * pointer = ParseStackPop(parse.operandstack);
		ASSERT_RETFALSE(pointer);
		ASSERT_RETFALSE(TypeGetBasicType(pointer->token.type) == TYPE_PTR);
		ASSERT_RETFALSE(pointer->token.type.sym);

		int arraysize = pointer->token.type.sym->arraysize;
		int typesize = TypeGetDeclSize(pointer->token.type);
		pointer->token.type = pointer->token.type.sym->type;

		if (index_isconstant)
		{
			int ii = ValueGetAsInteger(index->token.value, index->token.type);
			ASSERT_RETFALSE(ii >= 0 && ii < arraysize);
			pointer->offset += ii * typesize;
			ParseNodeFree(index);
		}
		else
		{
			RETURN_IF_FALSE(TypeConversion(index, int_type));

			PARSENODE * mul_op = ParseNodeNew(TOK_MUL);
			ASSERT_RETFALSE(mul_op);
			mul_op->token.type.type = TYPE_INT;
			ParseNodeAddChild(mul_op, index);

			PARSENODE * size_op = ParseNodeNew(TOK_NUM);
			ASSERT_RETFALSE(size_op);
			size_op->token.type = int_type;
			size_op->token.value.i = typesize;
			ParseNodeAddChild(mul_op, size_op);

			if (pointer->childcount == 0)
			{
				ParseNodeAddChild(pointer, mul_op);
			}
			else
			{
				PARSENODE * add_op = ParseNodeNew(TOK_ADD);
				ASSERT_RETFALSE(add_op);
				add_op->token.type.type = TYPE_INT;

				PARSENODE * a = ParseNodeRemoveChild(pointer, 0);
				ASSERT_RETFALSE(a);
				ParseNodeAddChild(add_op, a);
				ParseNodeAddChild(add_op, mul_op);
				ParseNodeAddChild(pointer, add_op);
			}
		}
		ParseStackPush(parse.operandstack, pointer);

		return TRUE;
	}
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ParseExprEnd(
	PARSE & parse,
	PARSENODE *& expr)
{
	ScriptParseTrace("\nParseTree:\n");
	int prectopop = 100;
	if (parse.bParamWanted)
	{
		prectopop = TokenDef[TOK_LPAREN].precedence - 1;
	}
	ASSERT_RETFALSE(PopParseNodes(parse, prectopop, 0));
	expr = ParseStackPop(parse.operandstack);
	if (expr)
	{
		PrintNode(parse, expr);
	}
	ScriptParseTrace("  end.\n");
	return (expr != NULL);
}


//----------------------------------------------------------------------------
// top-level expression parser
//----------------------------------------------------------------------------
BOOL ParseExprEx(
	PARSE & parse,
	PARSENODE *& expr)
{
	parse.bLastSymbolWasTerm = FALSE;
	ScriptParseTrace("Parse Expr:\n");
	while (TRUE)
	{
		PrintToken(parse, &parse.token);
		switch (parse.token.tok)
		{
		case TOK_PERIOD:
			PopParseNodes(parse, TokenGetPrecedence(parse.token), TokenGetAssociativity(parse.token));
			PushOperator(parse);
			break;
		case TOK_LBRACKET:
			ASSERT_RETFALSE(parse.bLastSymbolWasTerm);
			PushOperator(parse);
			break;
		case TOK_RBRACKET:
			if (PopBracket(parse))
			{
				break;
			}
			return ParseExprEnd(parse, expr);
		case TOK_NEG:
			if (parse.bLastSymbolWasTerm)
			{
				parse.token.tok = TOK_SUB;
			}
			PopParseNodes(parse, TokenGetPrecedence(parse.token), TokenGetAssociativity(parse.token));
			PushOperator(parse);
			break;
		case TOK_POS:
			if (parse.bLastSymbolWasTerm)
			{
				parse.token.tok = TOK_ADD;
			}
			PopParseNodes(parse, TokenGetPrecedence(parse.token), TokenGetAssociativity(parse.token));
			PushOperator(parse);
			break;
		case TOK_TILDE:
		case TOK_NOT:
		case TOK_POWER:
		case TOK_MUL:
		case TOK_DIV:
		case TOK_MOD:
		case TOK_ADD:
		case TOK_SUB:
		case TOK_LSHIFT:
		case TOK_RSHIFT:
		case TOK_LT:
		case TOK_GT:
		case TOK_LTE:
		case TOK_GTE:
		case TOK_EQ:
		case TOK_NE:
		case TOK_AND:
		case TOK_XOR:
		case TOK_OR:
		case TOK_LAND:
		case TOK_LOR:
		case TOK_QUESTION:
		case TOK_ASSIGN:
		case TOK_A_MUL:
		case TOK_A_DIV:
		case TOK_A_MOD:
		case TOK_A_ADD:
		case TOK_A_SUB:
		case TOK_A_LSHIFT:
		case TOK_A_RSHIFT:
		case TOK_A_AND:
		case TOK_A_XOR:
		case TOK_A_OR:
			PopParseNodes(parse, TokenGetPrecedence(parse.token), TokenGetAssociativity(parse.token));
			PushOperator(parse);
			break;
		case TOK_COLON:
			{
				PopParseNodes(parse, TokenGetPrecedence(parse.token), TokenGetAssociativity(parse.token));
				// should scan for ? on stack!
				PARSENODE * node = ParseStackPeek(parse.operatorstack, 0);
				if (!node)
				{
					return FALSE;
				}  
				ASSERT(node->token.tok == TOK_QUESTION);
				node->token.tok = TOK_TERNARY;
				parse.bLastSymbolWasTerm = FALSE;
			}
			break;
		case TOK_COMMA:
			PopParseNodes(parse, TokenGetPrecedence(parse.token), TokenGetAssociativity(parse.token));
			if (parse.operatorstack.count == 0 && (parse.bConstWanted || parse.bParamWanted) )
			{
				return ParseExprEnd(parse, expr);
			}

			// phu - if the top of the operator stack is a .[ then we need to pop an operand (a param), assert that it's a constant,
			// pop a second operand (rstat), add the param to the rstat,
			// update the param count of the rstat, push the rstat as an operand, and break
			{
				PARSENODE * node = ParseStackPeek(parse.operatorstack);
				if (node && node->token.tok == TOK_PARAMLIST)
				{
					PARSENODE * paramnode = ParseStackPop(parse.operandstack);
					ASSERT_RETFALSE(paramnode);
					ASSERT_RETFALSE(ParseNodeIsConstant(paramnode));
					int param = ValueGetAsInteger(paramnode->token.value, paramnode->token.type);
					ParseNodeFree(paramnode);

					PARSENODE * statnode = ParseStackPeek(parse.operandstack);
					ASSERT_RETFALSE(statnode);
					ASSERT_RETFALSE(isRstat(statnode));
					int stat = STAT_GET_STAT(statnode->token.value.i);
					const STATS_DATA * stat_data = StatsGetData(NULL, stat);
					ASSERT_RETFALSE(stat_data);

					unsigned int curparamidx = statnode->curparam;
					ASSERT_RETFALSE(curparamidx < stat_data->m_nParamCount);
					ASSERT_RETFALSE((param & stat_data->m_nParamGetMask[curparamidx]) == (unsigned int)param);

					PARAM oldparam = STAT_GET_PARAM(statnode->token.value.i);
					statnode->token.value.i = MAKE_STAT(stat, oldparam + ((param & stat_data->m_nParamGetMask[curparamidx]) << stat_data->m_nParamGetShift[curparamidx]));
					statnode->curparam++;

					parse.bLastSymbolWasTerm = FALSE;
					break;
				}
			}

			PushOperator(parse);
			break;
		case TOK_LPAREN:
			ASSERT_RETFALSE(!parse.bLastSymbolWasTerm);
			PushOperator(parse);
			break;
		case TOK_RPAREN:
			if (PopParens(parse))
			{
				break;
			}
			return ParseExprEnd(parse, expr);
		case TOK_NUM:
			ASSERT_RETFALSE(!parse.bLastSymbolWasTerm);
			PushOperand(parse);
			break;
		case TOK_GLOBALVAR:
		case TOK_GLOBALADR:
		case TOK_LOCALVAR:
		case TOK_LOCALADR:
		case TOK_PARAM:
			ASSERT_RETFALSE(!parse.bLastSymbolWasTerm);
			ASSERT_RETFALSE(parse.token.value.sym);
			if (parse.bConstWanted)
			{
				return FALSE;
			}
			else
			{
				PushOperand(parse);
				PARSENODE * node = ParseStackPeek(parse.operandstack);
				ASSERT_RETFALSE(node);
				node->offset = parse.token.value.sym->offset;
			}
			break;
		case TOK_FUNCTION:
			{
				ASSERT_RETFALSE(!parse.bLastSymbolWasTerm);
				ASSERT_RETFALSE(parse.token.value.sym);
				PushOperator(parse, parse.operandstack.count);
				NEXT();
				SASSERT_RETFALSE(parse.token.tok == TOK_LPAREN, "Expected left parentheses after function name.");
			}
			break;
		case TOK_IFUNCTION:
			{
				ASSERT_RETFALSE(parse.token.value.sym);
				ASSERT_RETFALSE(parse.token.value.sym);
				PushOperator(parse, parse.operandstack.count);
				NEXT();
				SASSERT_RETFALSE(parse.token.tok == TOK_LPAREN, "Expected left parentheses after function name.");
			}
			break;
		case TOK_FIELDSTR:
			{
				ASSERT_RETFALSE(!parse.bLastSymbolWasTerm);
				ASSERT_RETFALSE(parse.token.value.str);
				PARSENODE * op = ParseStackPop(parse.operatorstack);
				ASSERT_RETFALSE(op);
				ASSERT_RETFALSE(op->token.tok == TOK_PERIOD);
				PARSENODE * struc = ParseStackPop(parse.operandstack);
				ASSERT_RETFALSE(struc);
				ASSERT_RETFALSE(struc->token.type.type == TYPE_STRUCT);
				// find the field
				int len;
				DWORD key;
				StrGetLenAndKey(parse.token.value.str, &len, &key);
				SSYMBOL * field = StructFindField(struc->token.type.sym, parse.token.value.str, key);
				ASSERT_RETFALSE(field);
				FREE(NULL, parse.token.value.str);
				struc->token.value.sym = field;
				struc->token.type = field->type;
				struc->offset += field->offset;
				ParseNodeFree(op);
				ParseStackPush(parse.operandstack, struc);
				parse.bLastSymbolWasTerm = TRUE;
				parse.token = struc->token;
			}
			break;
		case TOK_PARAMLIST:				// .{ operator for stat param lists
			{
				ASSERT_RETFALSE(parse.bLastSymbolWasTerm);
				PARSENODE * node = ParseStackPeek(parse.operandstack);
				ASSERT_RETFALSE(node);
				ASSERT_RETFALSE(isRstat(node));
				PushOperator(parse);
			}
			break;
		case TOK_RCURLY:
			{
				PARSENODE * node = ParseStackPeek(parse.operatorstack);
				if (node && node->token.tok == TOK_PARAMLIST)
				{
					PARSENODE * paramnode = ParseStackPop(parse.operandstack);
					ASSERT_RETFALSE(paramnode);
					ASSERT_RETFALSE(ParseNodeIsConstant(paramnode));
					int param = ValueGetAsInteger(paramnode->token.value, paramnode->token.type);
					ASSERT_RETFALSE(param >= 0);
					ParseNodeFree(paramnode);

					PARSENODE * statnode = ParseStackPeek(parse.operandstack);
					ASSERT_RETFALSE(statnode);
					ASSERT_RETFALSE(isRstat(statnode));
					int stat = STAT_GET_STAT(statnode->token.value.i);
					const STATS_DATA * stat_data = StatsGetData(NULL, stat);
					ASSERT_RETFALSE(stat_data);

					unsigned int curparamidx = statnode->curparam;
					ASSERT_RETFALSE(curparamidx < stat_data->m_nParamCount);
					ASSERT_RETFALSE((param & stat_data->m_nParamGetMask[curparamidx]) == (unsigned int)param);

					PARAM oldparam = STAT_GET_PARAM(statnode->token.value.i);
					statnode->token.value.i = MAKE_STAT(stat, oldparam + ((param & stat_data->m_nParamGetMask[curparamidx]) << stat_data->m_nParamGetShift[curparamidx]));
					statnode->curparam++;

					parse.bLastSymbolWasTerm = TRUE;

					node = ParseStackPop(parse.operatorstack);
					ASSERT_RETFALSE(node && node->token.tok == TOK_PARAMLIST);
					ParseNodeFree(node);
					break;
				}
			}
			return ParseExprEnd(parse, expr);
		case TOK_SEMICOLON:
		case TOK_LINEFEED:
		case TOK_EOF:
			return ParseExprEnd(parse, expr);
		case TOK_RSTAT:
		case TOK_RSTAT_BASE:
		case TOK_RSTAT_MOD:
		case TOK_RSTAT_US:
		case TOK_RSTAT_US_BASE:
		case TOK_RSTAT_US_MOD:
			{
				ASSERT_RETFALSE(!parse.bLastSymbolWasTerm);
				ASSERT_RETFALSE(!parse.bConstWanted);
				PushOperand(parse);
			}
			break;
		case TOK_SPARAM:
			{
				ASSERT_RETFALSE(!parse.bLastSymbolWasTerm);
				PARSENODE * op = ParseStackPop(parse.operatorstack);
				ASSERT_RETFALSE(op);
				ASSERT_RETFALSE(op->token.tok == TOK_PERIOD);
				PARSENODE * stat = ParseStackPop(parse.operandstack);
				ASSERT_RETFALSE(stat);
				if (isRstat(stat))
				{
					// damage_min.fire
					ParseNodeFree(op);
					stat->token.value.i = MAKE_STAT(STAT_GET_STAT(stat->token.value.i), parse.token.value.i);
					ParseStackPush(parse.operandstack, stat);
					parse.bLastSymbolWasTerm = TRUE;
					parse.token = stat->token;
				}
				else if (stat->token.tok == TOK_PERIOD && stat->childcount == 2 && isRstat(stat->children[1]))
				{
					// unit.damage_min.fire
					ParseNodeFree(op);
					stat->children[1]->token.value.i = MAKE_STAT(STAT_GET_STAT(stat->children[1]->token.value.i), parse.token.value.i);
					ParseStackPush(parse.operandstack, stat);
					parse.bLastSymbolWasTerm = TRUE;
					parse.token = stat->token;
				}
				else
				{
					ASSERT_RETFALSE(0);
				}
			}
			break;
		case TOK_CONTEXTVAR:
			{
				ASSERT_RETFALSE(!parse.bLastSymbolWasTerm);
				ASSERT_RETFALSE(!parse.bConstWanted);
				PushOperand(parse);
			}
			break;
		case TOK_DATATABLE:
			{
				ASSERT_RETFALSE(!parse.bLastSymbolWasTerm);
				PushOperand(parse);
				NEXT();
				SASSERT_RETFALSE(parse.token.tok == TOK_COLON, "Datatable not followed by :");
			}
			break;
		case TOK_IDENT:
			{
				PARSENODE * node = ParseStackPeek(parse.operatorstack);
				SASSERT_RETFALSE(node && (node->token.tok == TOK_COMMA || node->token.tok == TOK_FUNCTION || node->token.tok == TOK_IFUNCTION), "Unrecognized token");
				PushOperand(parse);
			}
			break;
		default:
			SASSERT_RETFALSE(0, "Unrecognized token");
			break;
		}
		RETURN_IF_FALSE(GetToken(parse));
	}
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PARSENODE * ParseExpr(
	PARSE & parse,
	int operatortop,
	int operandtop)
{
	PARSENODE * expr;

	BOOL result = ParseExprEx(parse, expr);

	ASSERT(parse.operandstack.count >= operandtop);
	while (parse.operandstack.count > operandtop)
	{
		PARSENODE * node = ParseStackPop(parse.operandstack);
		ParseNodeFree(node);
	}
	ASSERT(parse.operatorstack.count >= operatortop);
	while (parse.operatorstack.count > operatortop)
	{
		PARSENODE * node = ParseStackPop(parse.operatorstack);
		ParseNodeFree(node);
	}
	if (!result)
	{
		return NULL;
	}
	return expr;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PARSENODE * PushBlock(
	PARSE & parse)
{
	PARSENODE * node = ParseNodeNew(&parse.token);
	ParseListPush(parse.blocklist, node);
	return node;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ParseBlockEx(
	PARSE & parse,
	PARSENODE *& block)
	// pass token stream
	// pass break point stack
	// pass continue point stack
{
	ScriptParseTrace("\nParse Block:\n");
	PrintToken(parse, &parse.token);
	parse.bConstWanted = FALSE;

	switch (parse.token.tok)
	{
	case TOK_IF:
		{
			block = ParseNodeNew(&parse.token);
			ASSERT_RETFALSE(block);
			NEXT();

			SKIP(TOK_LPAREN);
			PARSENODE * expr = ParseExpr(parse);
			RETURN_IF_FALSE(expr);
			ParseNodeAddChild(block, expr);
			SKIP(TOK_RPAREN);

			PARSENODE * trueblock = ParseBlock(parse);
			RETURN_IF_FALSE(trueblock);
			ParseNodeAddChild(block, trueblock);

			if (parse.token.tok == TOK_ELSE)
			{
				NEXT();
				PARSENODE * falseblock = ParseBlock(parse);
				RETURN_IF_FALSE(falseblock);
				ParseNodeAddChild(block, falseblock);
			}
		}
		break;
	case TOK_WHILE:
		{
			block = ParseNodeNew(&parse.token);
			ASSERT_RETFALSE(block);

			NEXT();
			SKIP(TOK_LPAREN);
			PARSENODE * expr = ParseExpr(parse);
			RETURN_IF_FALSE(expr);
			ParseNodeAddChild(block, expr);
			SKIP(TOK_RPAREN);									// on a continue do a jump to here

			PARSENODE * repeat = ParseBlock(parse);
			RETURN_IF_FALSE(repeat);
			ParseNodeAddChild(block, repeat);
		}
		break;
	case TOK_LCURLY:
		{
			NEXT();
			block = ParseBlockList(parse);
			RETURN_IF_FALSE(block);
			SKIP(TOK_RCURLY);
		}
		break;
	case TOK_RETURN:
		{
			block = ParseNodeNew(&parse.token);
			ASSERT_RETFALSE(block);

			NEXT();
			PARSENODE * expr = ParseExpr(parse);
			RETURN_IF_FALSE(expr);
			ParseNodeAddChild(block, expr);
		}
		break;
	case TOK_BREAK:
		// pop break point & jump
		PARSE_SKIP(TOK_SEMICOLON);
		break;
	case TOK_CONTINUE:
		// pop continue point & jump
		PARSE_SKIP(TOK_SEMICOLON);
		break;
	case TOK_FOR:
		PARSE_SKIP(TOK_LPAREN);
		if (parse.token.tok != TOK_SEMICOLON)
		{
//			ParseExpr(parse);
		}
		SKIP(TOK_SEMICOLON);
		if (parse.token.tok != TOK_SEMICOLON)
		{
			// save loc A & push continue point
//			ParseExpr(parse);
			// put JMP_IF_ZERO B & save loc B 
			// put JMP C & save loc C
		}
		SKIP(TOK_SEMICOLON);
		if (parse.token.tok != TOK_RPAREN)
		{
//			ParseExpr(parse);
			// put JMP A
		}
		// patch loc C
//		ParseBlock(parse);
		// patch loc B & patch break point
		// pop continue point
		break;
	case TOK_DO:
		// save loc A & push continue point
//		ParseBlock(parse);
		SKIP(TOK_WHILE);
		PARSE_SKIP(TOK_LPAREN);
//		ParseExpr(parse);
		// put JMP_IFN_ZERO A & patch break point
		SKIP(TOK_RPAREN);
		PARSE_SKIP(TOK_SEMICOLON);
		break;
	case TOK_SWITCH:
		PARSE_SKIP(TOK_LPAREN);
//		ParseExpr(parse);
		// check integer		
		SKIP(TOK_RPAREN);
		break;
	case TOK_CASE:
		break;
	case TOK_DEFAULT:
		break;
	case TOK_GOTO:
		RETURN_IF_FALSE(GetToken(parse));
		break;
	default:
		// if it's a label do one thing else...
		if (parse.token.tok != TOK_SEMICOLON)
		{
			// vpop?
			block = ParseExpr(parse);
			RETURN_IF_FALSE(block);
		}
		break;
	}
	
	if (parse.token.tok == TOK_SEMICOLON)
	{
		NEXT();
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PARSENODE * ParseBlock(
	PARSE & parse)
{
	PARSENODE * block = NULL;
	if (!ParseBlockEx(parse, block))
	{
		if (block)
		{
			ParseNodeFree(block);
		}
		return NULL;
	}
	return block;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ParseDeclFunc(
	PARSE & parse,
	SSYMBOL * function)
{
	FUNCTION * func = FunctionTableAdd(function);
	ASSERT_RETFALSE(func);

	// read parameter list
	while (parse.token.tok != TOK_RPAREN)
	{
		parse.bParamDeclWanted = TRUE;
		TOKEN ident;
		TYPE btype;
		int result = ParseType(parse, btype, ident, NULL);
		parse.bParamDeclWanted = FALSE;
		ASSERT_RETFALSE(result);
		ASSERT_RETFALSE(ident.tok == TOK_PARAM);

		SSYMBOL * param = ident.value.sym;
		ASSERT_RETFALSE(param);

		// remove the param from the symbol table & add to parameter list
		SymbolTableRemove(gVM.SymbolTable, param);
		FunctionAddParameter(func, param, TRUE);

		if (parse.token.tok == TOK_COMMA)
		{
			NEXT();
		}
		else if (parse.token.tok != TOK_RPAREN)
		{
			return FALSE;
		}
	}
	NEXT();
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ParseTokIsIdentifier(
	PARSE & parse)
{
	switch (parse.token.tok)
	{
	case TOK_IDENT:
	case TOK_GLOBALVAR:
	case TOK_LOCALVAR:
	case TOK_PARAM:
	case TOK_FUNCTION:
	case TOK_IFUNCTION:
		return TRUE;		// can possibly map to variable name declaration
	default:
		return FALSE;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SymbolSetName(
	PARSE & parse,
	SSYMBOL * symbol,
	char * fieldname)
{
	ASSERT_RETFALSE(symbol);
	ASSERT_RETFALSE(symbol->str == NULL);
	int len;
	DWORD key;
	StrGetLenAndKey(fieldname, &len, &key);
	symbol->str = (char *)MALLOC(NULL, len + 1);
	memcpy(symbol->str, fieldname, len);
	symbol->str[len] = 0;
	symbol->key = key;
	return TRUE;
}


//----------------------------------------------------------------------------
// parse identifier declaration
//----------------------------------------------------------------------------
BOOL ParseDeclIdent(
	PARSE & parse,
	const TYPE & type,
	TOKEN & ident,
	SSYMBOL * structure)
{
	if (parse.token.tok == TOK_SEMICOLON)
	{
		return TRUE;
	}
	ASSERT_RETFALSE(ParseTokIsIdentifier(parse));

	ASSERT_RETFALSE(parse.token.value.sym);
	if (structure)
	{
		char * fieldname = parse.token.value.sym->str;
		// check that field name isn't already defined for this struct
		if (StructFindField(structure, fieldname, parse.token.value.sym->key))
		{
			ASSERT_RETFALSE(0);
		}
		SSYMBOL * field = SymbolTableAdd(gVM.SymbolTable, TOK_FIELD, NULL, 0, 0);
		ASSERT_RETFALSE(field);
		SymbolSetName(parse, field, fieldname);
		field->type = type;
		ASSERT_RETFALSE(StructAddField(structure, field));
		parse.token.tok = TOK_FIELD;
		parse.token.value.sym = field;
	}
	else if (parse.token.tok != TOK_IDENT)
	{
		RETURN_IF_FALSE(parse.token.value.sym->block_level != parse.block_level);
		SSYMBOL * symbol = SymbolTableAdd(gVM.SymbolTable, TOK_IDENT, parse.token.value.sym->str);
		ASSERT_RETFALSE(symbol);
		symbol->block_level = parse.block_level;
		parse.token.tok = TOK_IDENT;
		parse.token.value.sym = symbol;
	}
	SSYMBOL * symbol = parse.token.value.sym;
	NEXT();

	BOOL isfunction = FALSE;
	if (structure)
	{
		;
	}
	else if (parse.bParamDeclWanted)
	{
		symbol->tok = TOK_PARAM;
	}
	else if (parse.token.tok == TOK_LPAREN)
	{
		ASSERT_RETFALSE(!structure);
		NEXT();

		isfunction = TRUE;
		symbol->tok = TOK_FUNCTION;
		symbol->block_level = 0;
		parse.block_level++;
		int result = ParseDeclFunc(parse, symbol);
		parse.block_level--;
		if (!result)
		{
			return FALSE;
		}
	}
	else if (parse.block_level == 1)
	{
		symbol->tok = TOK_GLOBALVAR;
		symbol->offset = gVM.globalcur;
	}
	else
	{
		symbol->tok = TOK_LOCALVAR;
		symbol->offset = parse.cur_local_offset;
	}
	symbol->type = type;
	symbol->type_size = TypeGetDeclSize(type);

	SSYMBOL * prev = symbol;
	if (!isfunction)
	{
		while (parse.token.tok == TOK_LBRACKET)	// array
		{
			NEXT();
			int n = -1;
			if (parse.token.tok != TOK_RBRACKET)
			{
				TOKEN token;
				ASSERT_RETFALSE(ParseConstantExpr(parse, token, FALSE));
				n = ValueGetAsInteger(token.value, token.type);
				ASSERT_RETFALSE(n > 0);
			}
			SKIP(TOK_RBRACKET);

			int type_size = TypeGetDeclSize(prev->type);

			SSYMBOL * arrayof = SymbolTableAdd(gVM.SymbolTable, TOK_ARRAY, NULL, 0, 0);
			arrayof->type = prev->type;
			arrayof->type_size = type_size;

			prev->type.type = TYPE_PTR | TYPE_ARRAY;
			arrayof->arraysize = n;

			// multiply type sizes
			{
				SSYMBOL * cur = symbol;
				while (cur)
				{
					cur->type_size *= n;
					cur = cur->type.sym;
				}
			}

			prev->type.sym = arrayof;
			prev = arrayof;
		}
	}
	if (structure)
	{
		structure->type_size += symbol->type_size;
	}
	else if (symbol->tok == TOK_GLOBALVAR)
	{
		gVM.globalcur += MAX(4, symbol->type_size);
	}
	else if (symbol->tok == TOK_LOCALVAR)
	{
		parse.cur_local_offset += MAX(4, symbol->type_size);
	}

	ident.tok = symbol->tok;
	ident.type = symbol->type;
	ident.value.sym = symbol;
	ScriptParseTrace("ParseDecl: ");
	PrintToken(parse, &ident);

    return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ParseStruct(
	PARSE & parse,
	TYPE & type,
	TOKEN & ident,
	SSYMBOL * structure)
{
	NEXT();											// read type identifier (or field name if structure != NULL)

	SSYMBOL * symbol = NULL;

	if (parse.token.tok == TOK_LCURLY)
	{
		if (structure)								// for structures, fields are added directly to parent structure
		{
			symbol = structure;
			ASSERT_RETFALSE(symbol->type.type == TYPE_STRUCT);
		}
		else
		{
			symbol = SymbolTableAdd(gVM.SymbolTable, TOK_STRUCT, NULL, 0, 0);
			ASSERT_RETFALSE(symbol);
			symbol->type.type = TYPE_STRUCT;
			symbol->type.sym = symbol;
		}
		ScriptParseTrace("ParseDecl: struct\n");
	}
	else
	{
		if (structure)								// the token becomes the field name
		{
			ASSERT_RETFALSE(ParseTokIsIdentifier(parse));
			ASSERT_RETFALSE(parse.token.value.sym);
			char * fieldname = parse.token.value.sym->str;
			if (StructFindField(structure, fieldname, parse.token.value.sym->key))
			{
				ASSERT_RETFALSE(0);
			}
			symbol = SymbolTableAdd(gVM.SymbolTable, TOK_FIELD, NULL, 0, 0);
			ASSERT_RETFALSE(symbol);
			SymbolSetName(parse, symbol, fieldname);
			ASSERT_RETFALSE(StructAddField(structure, symbol));
		}
		else
		{
			ASSERT_RETFALSE(parse.token.tok == TOK_IDENT);

			symbol = parse.token.value.sym;
			ASSERT_RETFALSE(symbol);
		}
		symbol->type.type = TYPE_STRUCT;
		symbol->type.sym = symbol;
		ScriptParseTrace("ParseDecl: struct %s\n", symbol->str);
		NEXT();
	}

	SKIP(TOK_LCURLY);

	while (parse.token.tok != TOK_RCURLY)
	{
		ASSERT_RETFALSE(ParseDecl(parse, NULL, symbol));
	}
	if (structure && structure != symbol)
	{
		structure->type_size += symbol->type_size;
	}

	SKIP(TOK_RCURLY);

	type = symbol->type;
	ident.tok = symbol->tok;
	ident.type = type;
	ident.type.sym = symbol;
	ident.value.sym = symbol;

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ParseEnum(
	PARSE & parse,
	TYPE & type)
{
	NEXT();
	type.type |= TYPE_ENUM;

	ScriptParseTrace("ParseDecl: enum ");
	// insert typedef

	ScriptParseTrace("= { ");
	SKIP(TOK_LCURLY);
	int value = 0;
	while (parse.token.tok != TOK_RCURLY)
	{
		if (parse.token.tok != TOK_IDENT)
		{
			return FALSE;
		}
		SSYMBOL * enumeration = parse.token.value.sym;
		ASSERT_RETFALSE(enumeration);
		NEXT();

		enumeration->type.type = TYPE_ENUM | TYPE_CONSTANT;
		enumeration->tok = TOK_NUM;
		if (parse.token.tok == TOK_ASSIGN)
		{
			NEXT();
			TOKEN token;
			ASSERT_RETFALSE(ParseConstantExpr(parse, token));
			value = ValueGetAsInteger(token.value, token.type);
		}
		enumeration->value = value++;
		ScriptParseTrace("%s (%d)", enumeration->str, enumeration->value);

		if (parse.token.tok == TOK_COMMA)
		{
			ScriptParseTrace(", ");
			NEXT();
		}
		else if (parse.token.tok == TOK_RCURLY)
		{
			ScriptParseTrace("}\n");
			NEXT();
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}
	NEXT();
	return TRUE;
}


//----------------------------------------------------------------------------
// return the type and skip, or return 0 if not a type declaration
//----------------------------------------------------------------------------
BOOL ParseType(
	PARSE & parse,
	TYPE & type,
	TOKEN & ident,
	SSYMBOL * structure)
{
	structclear(type);
	structclear(ident);

	BOOL is_unsigned = FALSE;
	switch (parse.token.tok)
	{
	case TOK_UNSIGNED:
		NEXT();
		is_unsigned = TRUE;;
		break;
	case TOK_ENUM:
		ASSERT_RETFALSE(!parse.bParamDeclWanted);	// for now
		ASSERT_RETFALSE(!structure);
		return ParseEnum(parse, type);
	case TOK_STRUCT:
		ASSERT_RETFALSE(!parse.bParamDeclWanted);	// for now
		if (parse.token.type.type != 0)				// structure has been defined already
		{
			break;
		}
		ASSERT_RETFALSE(ParseStruct(parse, type, ident, structure));
		if (structure)								// structure declaration in another struct declares field name first, so return
		{
			return TRUE;
		}
		return ParseDeclIdent(parse, type, ident, structure);
	default:
		break;
	}

	BOOL is_long = FALSE;
	switch (parse.token.tok)
	{
	case TOK_LONG:
		NEXT();
		is_long = TRUE;
		break;
	case TOK_SHORT:
		NEXT();
		type.type = TYPE_SHORT;
		break;
	}
	switch (parse.token.tok)
	{
    case TOK_BOOL:
		RETURN_IF_FALSE(!is_unsigned);
		RETURN_IF_FALSE(!is_long);
		RETURN_IF_FALSE(type.type != TYPE_SHORT);
		NEXT();
		type.type = TYPE_BOOL;
		break;
    case TOK_CHAR:
		RETURN_IF_FALSE(!is_long);
		RETURN_IF_FALSE(type.type != TYPE_SHORT);
		NEXT();
		type.type = TYPE_CHAR;
		break;
    case TOK_VOID:
		RETURN_IF_FALSE(!is_unsigned);
		RETURN_IF_FALSE(!is_long);
		RETURN_IF_FALSE(type.type != TYPE_SHORT);
		NEXT();
		type.type = TYPE_VOID;
		break;
    case TOK_SHORT:
		RETURN_IF_FALSE(!is_unsigned);
		RETURN_IF_FALSE(!is_long);
		RETURN_IF_FALSE(type.type != TYPE_SHORT);
		NEXT();
		type.type = TYPE_SHORT;
		break;
    case TOK_INT:
		NEXT();
		if (is_long)
		{
			type.type = TYPE_INT64;
		}
		else if (type.type & TYPE_SHORT)
		{
			;
		}
		else
		{
			type.type = TYPE_INT;
		}
        break;
    case TOK_FLOAT:
		RETURN_IF_FALSE(!is_unsigned);
		RETURN_IF_FALSE(!is_long);
		RETURN_IF_FALSE(type.type != TYPE_SHORT);
		NEXT();
		type.type = TYPE_FLOAT;
		break;
    case TOK_DOUBLE:
		RETURN_IF_FALSE(!is_unsigned);
		RETURN_IF_FALSE(type.type != TYPE_SHORT);
		NEXT();
		if (is_long)
		{
			type.type = TYPE_LDOUBLE;
		}
		else
		{
			type.type = TYPE_DOUBLE;
		}
		break;
	case TOK_STRUCT:
		RETURN_IF_FALSE(!is_unsigned);
		RETURN_IF_FALSE(type.type != TYPE_SHORT);
		type = parse.token.type;
		NEXT();
		break;
	case TOK_TYPEDEF:
		RETURN_IF_FALSE(!is_unsigned);
		RETURN_IF_FALSE(!is_long);
		type = parse.token.type;
		NEXT();
		break;
	default:
		if (is_long)
		{
			type.type = TYPE_INT;
			break;
		}
		if (type.type == TYPE_SHORT)
		{
			break;
		}
		// should read typedef'd types
		return FALSE;
	}
	if (is_unsigned)
	{
		RETURN_IF_FALSE(TypeSetUnsigned(type));
	}

	return ParseDeclIdent(parse, type, ident, structure);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ParseInitStruct(
	PARSE & parse,
	TYPE & type,
	PARSENODE * init);


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ParseInitArray(
	PARSE & parse,
	TYPE & type,
	PARSENODE * init)
{
	ASSERT_RETFALSE(init);
	ASSERT_RETFALSE(type.type & TYPE_ARRAY);
	SSYMBOL * type_sym = type.sym;
	ASSERT_RETNULL(type_sym);

	int count = 0;
	while (TRUE)
	{
		if (parse.token.tok == TOK_RCURLY)
		{
			if (count < type_sym->arraysize)
			{
				PARSENODE * fill = ParseNodeNew(TOK_FILLZERO);
				ASSERT_RETNULL(fill);
				fill->token.value.ui = TypeGetDeclSize(type_sym->type) * (type_sym->arraysize - count);
				ParseNodeAddChild(init, fill);
			}
			NEXT();
			return TRUE;
		}

		if (type_sym->type.type & TYPE_ARRAY)
		{
			ASSERT_RETNULL(parse.token.tok == TOK_LCURLY);
			NEXT();
			ASSERT_RETNULL(ParseInitArray(parse, type_sym->type, init));
		}
		else if (type_sym->type.type == TYPE_STRUCT)
		{
			ASSERT_RETNULL(parse.token.tok == TOK_LCURLY);
			NEXT();
			ASSERT_RETNULL(ParseInitStruct(parse, type_sym->type, init));
		}
		else
		{
			ASSERT_RETNULL(TypeIsBasicType(type_sym->type));
			TOKEN token;
			ASSERT_RETFALSE(ParseConstantExpr(parse, token));
			PARSENODE * value = ParseNodeNew(&token);
			ASSERT_RETNULL(value);
			ParseNodeAddChild(init, value);
		}
		count++;
		ASSERT_RETFALSE(count <= type_sym->arraysize);

		if (parse.token.tok == TOK_COMMA)
		{
			NEXT();
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ParseInitStruct(
	PARSE & parse,
	TYPE & type,
	PARSENODE * init)
{
	ASSERT_RETFALSE(init);
	ASSERT_RETFALSE(type.type == TYPE_STRUCT);
	SSYMBOL * type_sym = type.sym;
	ASSERT_RETFALSE(type_sym);

	int count = 0;
	while (TRUE)
	{
		SSYMBOL * field = NULL;
		if (count < type_sym->fieldcount)
		{
			field = type_sym->fields[count];
		}

		if (parse.token.tok == TOK_RCURLY)
		{
			if (count < type_sym->fieldcount)
			{
				ASSERT_RETFALSE(field);
				PARSENODE * fill = ParseNodeNew(TOK_FILLZERO);
				ASSERT_RETFALSE(fill);
				fill->token.value.ui = TypeGetDeclSize(type_sym->type) - field->offset;
				ParseNodeAddChild(init, fill);
			}
			NEXT();
			return TRUE;
		}

		ASSERT_RETFALSE(field);
		if (field->type.type & TYPE_ARRAY)
		{
			ASSERT_RETFALSE(parse.token.tok == TOK_LCURLY);
			NEXT();
			ASSERT_RETFALSE(ParseInitArray(parse, field->type, init));
		}
		else if (field->type.type == TYPE_STRUCT)
		{
			ASSERT_RETFALSE(parse.token.tok == TOK_LCURLY);
			NEXT();
			ASSERT_RETFALSE(ParseInitStruct(parse, field->type, init));
		}
		else
		{
			ASSERT_RETFALSE(TypeIsBasicType(field->type));
			TOKEN token;
			ASSERT_RETFALSE(ParseConstantExpr(parse, token));
			PARSENODE * value = ParseNodeNew(&token);
			ASSERT_RETFALSE(value);
			ParseNodeAddChild(init, value);
		}
		count++;
		ASSERT_RETFALSE(count <= type_sym->fieldcount);

		if (parse.token.tok == TOK_COMMA)
		{
			NEXT();
		}
	}
}


//----------------------------------------------------------------------------
// initialization, leave semicolon after we're done
//----------------------------------------------------------------------------
PARSENODE * ParseInitializer(
	PARSE & parse,
	TOKEN & ident)
{
	if (TypeIsBasicType(ident.type))
	{
		ASSERT_RETNULL(ident.value.sym);

		TOKEN token;
		ASSERT_RETFALSE(ParseConstantExpr(parse, token));
		TypeConversion(token, TypeGetBasicType(ident.type));

		PARSENODE * init = ParseNodeNew((parse.block_level == 1) ? TOK_GLOBAL_INIT : TOK_LOCAL_INIT);
		ASSERT_RETNULL(init);
		init->offset = ident.value.sym->offset;
		init->token.type = ident.type;

		PARSENODE * value = ParseNodeNew(&token);
		ASSERT_RETNULL(value);

		ParseNodeAddChild(init, value);
		return init;
	}
	else if (parse.token.tok == TOK_LCURLY)
	{
		NEXT();

		ASSERT_RETNULL(ident.value.sym);

		PARSENODE * init = ParseNodeNew((parse.block_level == 1) ? TOK_GLOBAL_INITM : TOK_LOCAL_INITM);
		ASSERT_RETNULL(init);
		init->offset = ident.value.sym->offset;

		// array or structure initializer
		if (ident.type.type & TYPE_ARRAY)
		{
			ASSERT_RETNULL(ParseInitArray(parse, ident.type, init));
			return init;
		}
		else if (ident.type.type == TYPE_STRUCT)
		{
			ASSERT_RETNULL(ParseInitStruct(parse, ident.type, init));
			return init;
		}
		else
		{
			ASSERT_RETNULL(0);
		}
	}
	else 
	{
		ASSERT_RETNULL(0);
	}
	return NULL;
}


//----------------------------------------------------------------------------
// pass a block OR a structure
//----------------------------------------------------------------------------
BOOL ParseDecl(
	PARSE & parse,
	PARSENODE * block,
	SSYMBOL * structure)
{
	ASSERT((block && !structure) || (structure && !block));

	TOKEN ident;
	TYPE btype;
	if (!ParseType(parse, btype, ident, structure))
	{
		return FALSE;
	}

	int basictype = TypeGetBasicType(btype);
	if (basictype != TYPE_ENUM &&
		basictype != TYPE_STRUCT)
	{
		if (parse.token.tok == TOK_LCURLY)
		{
			// function declaration
			ASSERT_RETFALSE(ident.tok == TOK_FUNCTION);
			ASSERT_RETFALSE(ident.value.sym != NULL);
			NEXT();

			FUNCTION * func = ident.value.sym->function;
			ASSERT_RETFALSE(func);

			PARSENODE * funcnode = ParseNodeNew(&ident);
			ASSERT_RETFALSE(funcnode);

			PARSENODE * body = ParseBlockList(parse, &ident);
			RETURN_IF_FALSE(body);
			SKIP(TOK_RCURLY);
			ParseNodeAddChild(funcnode, body);

			EmitFunction(parse, funcnode);
			ParseNodeFree(funcnode);
			return TRUE;
		}
	}

	while (TRUE)
	{
		if (parse.token.tok == TOK_SEMICOLON)
		{
			NEXT();
			return TRUE;
		}
		else if (parse.token.tok == TOK_ASSIGN && block)
		{
			NEXT();
			PARSENODE * init = ParseInitializer(parse, ident);
			RETURN_IF_FALSE(init);

			ParseNodeAddChild(block, init);
		}
		else if (parse.token.tok == TOK_COMMA)
		{
			NEXT();

			RETURN_IF_FALSE(ParseDeclIdent(parse, btype, ident, structure));
		}
		else
		{
			ASSERT_RETFALSE(0);
		}
	}
}


//----------------------------------------------------------------------------
// generate the top value in the value stack
//----------------------------------------------------------------------------
BOOL GenValue(
	PARSE & parse)
{
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PARSENODE * ParseBlockList(
	PARSE & parse,
	TOKEN * function)
{
	PARSENODE * block = ParseNodeNew(TOK_BLOCKLIST);
	ASSERT_RETFALSE(block);

	parse.block_level++;

	// push local variables
	if (function)
	{
		FUNCTION * func = function->value.sym->function;
		ASSERT_RETNULL(func);

		int param_count = FunctionGetParameterCount(func);
		for (int ii = 0; ii < param_count; ii++)
		{
			SSYMBOL * param = FunctionGetParameter(func, ii);
			ASSERT_BREAK(param);
			SymbolTableAdd(gVM.SymbolTable, param);
			param->offset = parse.cur_local_offset;
			parse.cur_local_offset += MAX(4, TypeGetSize(param->type));
		}
	}

	while (parse.token.tok != TOK_EOF)
	{
		parse.bParamDeclWanted = FALSE;
		while (ParseDecl(parse, block, FALSE))
		{
			;
		}

		PARSENODE * subblock = ParseBlock(parse);
		BREAK_IF_FALSE(subblock);
		ParseNodeAddChild(block, subblock);
		if (parse.token.tok == TOK_EOF ||
			(parse.block_level > 1 && parse.token.tok == TOK_RCURLY))
		{
			break;
		}
	}

	// pop symbols until block level < current
	if (parse.block_level > 1)
	{
		int old_local_cur = parse.cur_local_offset;
		while (SSYMBOL * sym = SymbolTableGetLast(gVM.SymbolTable))
		{
			if (sym->block_level < parse.block_level)
			{
				break;
			}
			SymbolTableRemove(gVM.SymbolTable, sym);
			if (sym->tok == TOK_LOCALVAR)
			{
				parse.cur_local_offset -= MAX(4, TypeGetSize(sym->type));
				ASSERT(parse.cur_local_offset >= 0);
			}
			SymbolFree(sym);
		}

		if (function)
		{
			FUNCTION * func = function->value.sym->function;
			ASSERT_RETNULL(func);

			int param_count = FunctionGetParameterCount(func);
			for (int ii = 0; ii < param_count; ii++)
			{
				SSYMBOL * param = FunctionGetParameter(func, ii);
				ASSERT_BREAK(param);
				SymbolTableRemove(gVM.SymbolTable, param);
				parse.cur_local_offset -= MAX(4, TypeGetSize(param->type));
				ASSERT(parse.cur_local_offset >= 0);
			}
		}

		int localsize = old_local_cur - parse.cur_local_offset;
		block->localstack = localsize;
	}
	parse.block_level--;

	return block;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// forward decl
struct VMSTATE * VMGetVMState(
	struct VMGAME * vm);

void VMReleaseState(
	struct VMSTATE * vmstate);

struct VMAutoReleaseState
{
	VMSTATE *	vmstate;
	VMAutoReleaseState(
		VMSTATE * _vmstate) : vmstate(_vmstate)
	{
	}
	~VMAutoReleaseState(
		void)
	{
		if (vmstate)
		{
			VMReleaseState(vmstate);
			vmstate = NULL;
		}
	}
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int ScriptParseEx(
	VMGAME * vm,
	BYTE_STREAM * stream,
	const char * str,
	int errorlog,
	const char * errorstr)
{
#define EXCEL_SCRIPT_BREAK 0
#if defined(_DEBUG) && EXCEL_SCRIPT_BREAK
	const char * pszDebugString = "shield_overload_pct = 1";
	if(PStrICmp(str, pszDebugString) == 0)
	{
		DebugBreak();
	}
#endif
	int len = 0;
	ASSERT_RETZERO(vm);

	VMSTATE * vmstate = VMGetVMState(vm);
	ASSERT_RETZERO(vmstate);

	VMAutoReleaseState autolock(vmstate);

	CSAutoLock csautolock(&gVM.cs);

	PARSE parse;
	structclear(parse);

	ParseFilePush(parse, "console", str, FALSE);
	parse.vm = vmstate;
	parse.errorlog = errorlog;
	parse.errorstr = errorstr;
	parse.text = str;

	GetToken(parse);

	PARSENODE * block = ParseBlockList(parse);
	if (!block)
	{
		return 0;
	}

	parse.cur_local_offset = 0;
	EmitNode(parse, stream, block);
	ParseNodeFree(block);

	if (stream->size >= 0)
	{
		ByteStreamAddInt(stream, VMI_EOF);
	}
	len = stream->cur;
	ByteStreamReset(stream);

	if (parse.macrostack.stack)
	{
		FREE(NULL, parse.macrostack.stack);
	}

	while (parse.file)
	{
		ParseFilePop(parse);
	}

	ASSERT(parse.operandstack.top == NULL);
	ASSERT(parse.operatorstack.top == NULL);

	return (len <= 4 ? 0 : len);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BYTE * ScriptParseEx(
	GAME * game, 
	const char * str,
	BYTE * buf,
	int len,
	int errorlog,
	const char * errorstr)
{
	VMGAME * vm = gVM.vm;
	if (game)
	{
		vm = game->m_vm;
	}

	BYTE_STREAM stream;
	ByteStreamInit(NULL, stream, buf, len);
	int result = ScriptParseEx(vm, &stream, str, errorlog, errorstr);
	if (!result)
	{
		return NULL;
	}
	return buf + result;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SSYMBOL * AddImportFunction(
	const char * str, 
	void * fp,
	int ret_type)
{
	SSYMBOL * symbol = SymbolTableAdd(gVM.SymbolTable, TOK_IFUNCTION, str);
	ASSERT_RETNULL(symbol);
	symbol->type.type = ret_type;
	FUNCTION * function = FunctionTableAdd(symbol);
	function->fptr = fp;

	gVM.dwCrcImportFunctions = CRC_STRING(gVM.dwCrcImportFunctions, str);
	gVM.dwCrcImportFunctions = CRC(gVM.dwCrcImportFunctions, ret_type);

	return symbol;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void EndImportFunction(
	SSYMBOL * sym)
{
	ASSERT_RETURN(sym);
	ASSERT_RETURN(sym->str);
	FUNCTION * function = sym->function;
	ASSERT_RETURN(function);

	// construct a hash value out of the function name and parameters
	DWORD hash = CRC_STRING(0, sym->str);
	CRC(hash, function->min_param_count);
	CRC(hash, function->parameter_count);
	CRC(hash, function->varargs);
	for (unsigned int ii = 0; ii < (unsigned int)function->parameter_count; ++ii)
	{
		SSYMBOL * parameter = function->parameters[ii];
		ASSERT_CONTINUE(parameter && parameter->str);
		CRC_STRING(hash, parameter->str);
		CRC(hash, parameter->parameter_type);
		CRC(hash, parameter->index_table);
		CRC(hash, parameter->index_index);
		CRC(hash, parameter->type.type);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SSYMBOL * AddImportVMStateParam(
	SSYMBOL * sym)
{
	ASSERT_RETNULL(sym);
	FUNCTION * function = sym->function;
	ASSERT_RETNULL(function);
	
	SSYMBOL * param = SymbolMakeNew(TOK_PARAM, "game");
	param->parameter_type = PARAM_CONTEXT_VMSTATE;
	FunctionAddParameter(function, param, FALSE);

	gVM.dwCrcImportFunctions = CRC(gVM.dwCrcImportFunctions, TOK_PARAM);
	gVM.dwCrcImportFunctions = CRC(gVM.dwCrcImportFunctions, PARAM_CONTEXT_VMSTATE);

	return param;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SSYMBOL * AddImportContext(
	SSYMBOL * sym)
{
	ASSERT_RETNULL(sym);
	FUNCTION * function = sym->function;
	ASSERT_RETNULL(function);
	
	SSYMBOL * param = SymbolMakeNew(TOK_PARAM, "context");
	param->parameter_type = PARAM_CONTEXT;
	FunctionAddParameter(function, param, FALSE);

	gVM.dwCrcImportFunctions = CRC(gVM.dwCrcImportFunctions, TOK_PARAM);
	gVM.dwCrcImportFunctions = CRC(gVM.dwCrcImportFunctions, PARAM_CONTEXT);

	return param;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SSYMBOL * AddImportGameParam(
	SSYMBOL * sym)
{
	ASSERT_RETNULL(sym);
	FUNCTION * function = sym->function;
	ASSERT_RETNULL(function);
	
	SSYMBOL * param = SymbolMakeNew(TOK_PARAM, "game");
	param->parameter_type = PARAM_CONTEXT_GAME;
	FunctionAddParameter(function, param, FALSE);

	gVM.dwCrcImportFunctions = CRC(gVM.dwCrcImportFunctions, TOK_PARAM);
	gVM.dwCrcImportFunctions = CRC(gVM.dwCrcImportFunctions, PARAM_CONTEXT_GAME);

	return param;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SSYMBOL * AddImportCntxParam(
	SSYMBOL * sym,
	const char * str,
	int type)
{
	ASSERT_RETNULL(sym);
	FUNCTION * function = sym->function;
	ASSERT_RETNULL(function);
	
	SSYMBOL * param = SymbolMakeNew(TOK_PARAM, str);
	param->parameter_type = (EIPARAM_TYPE)type;
	FunctionAddParameter(function, param, FALSE);

	gVM.dwCrcImportFunctions = CRC_STRING(gVM.dwCrcImportFunctions, str);
	gVM.dwCrcImportFunctions = CRC(gVM.dwCrcImportFunctions, TOK_PARAM);
	gVM.dwCrcImportFunctions = CRC(gVM.dwCrcImportFunctions, type);

	return param;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SSYMBOL * AddImportIdxParam(
	SSYMBOL * sym,
	int table,
	const char * str,
	int type)
{
	ASSERT_RETNULL(sym);
	FUNCTION * function = sym->function;
	ASSERT_RETNULL(function);
	
	SSYMBOL * param = SymbolMakeNew(TOK_PARAM, str);
	param->parameter_type = PARAM_INDEX;
	param->index_table = table;
	param->index_index = 0;
	param->type.type = type;
	FunctionAddParameter(function, param, TRUE);

	gVM.dwCrcImportFunctions = CRC_STRING(gVM.dwCrcImportFunctions, str);
	gVM.dwCrcImportFunctions = CRC(gVM.dwCrcImportFunctions, TOK_PARAM);
	gVM.dwCrcImportFunctions = CRC(gVM.dwCrcImportFunctions, table);
	gVM.dwCrcImportFunctions = CRC(gVM.dwCrcImportFunctions, PARAM_INDEX);
	gVM.dwCrcImportFunctions = CRC(gVM.dwCrcImportFunctions, type);

	return param;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SSYMBOL * AddImportIdxParam(
	SSYMBOL * sym,
	int table,
	int index,
	const char * str,
	int type)
{
	ASSERT_RETNULL(sym);
	FUNCTION * function = sym->function;
	ASSERT_RETNULL(function);
	
	SSYMBOL * param = SymbolMakeNew(TOK_PARAM, str);
	param->parameter_type = PARAM_INDEX;
	param->index_table = table;
	param->index_index = index;
	param->type.type = type;
	FunctionAddParameter(function, param, TRUE);

	gVM.dwCrcImportFunctions = CRC_STRING(gVM.dwCrcImportFunctions, str);
	gVM.dwCrcImportFunctions = CRC(gVM.dwCrcImportFunctions, TOK_PARAM);
	gVM.dwCrcImportFunctions = CRC(gVM.dwCrcImportFunctions, table);
	gVM.dwCrcImportFunctions = CRC(gVM.dwCrcImportFunctions, PARAM_INDEX);
	gVM.dwCrcImportFunctions = CRC(gVM.dwCrcImportFunctions, index);
	gVM.dwCrcImportFunctions = CRC(gVM.dwCrcImportFunctions, type);

	return param;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SSYMBOL * AddImportParamParam(
	SSYMBOL * sym,
	const char * str,
	int type)
{
	ASSERT_RETNULL(sym);
	FUNCTION * function = sym->function;
	ASSERT_RETNULL(function);
	
	SSYMBOL * param = SymbolMakeNew(TOK_PARAM, str);
	param->parameter_type = PARAM_STAT_PARAM;
	param->type.type = type;
	FunctionAddParameter(function, param, TRUE);

	gVM.dwCrcImportFunctions = CRC_STRING(gVM.dwCrcImportFunctions, str);
	gVM.dwCrcImportFunctions = CRC(gVM.dwCrcImportFunctions, TOK_PARAM);
	gVM.dwCrcImportFunctions = CRC(gVM.dwCrcImportFunctions, PARAM_STAT_PARAM);
	gVM.dwCrcImportFunctions = CRC(gVM.dwCrcImportFunctions, type);

	return param;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SSYMBOL * AddImportParam(
	SSYMBOL * sym,
	const char * str,
	int type)
{
	ASSERT_RETNULL(sym);
	FUNCTION * function = sym->function;
	ASSERT_RETNULL(function);
	
	SSYMBOL * param = SymbolMakeNew(TOK_PARAM, str);
	param->parameter_type = PARAM_NORMAL;
	param->type.type = type;
	FunctionAddParameter(function, param, TRUE);

	gVM.dwCrcImportFunctions = CRC_STRING(gVM.dwCrcImportFunctions, str);
	gVM.dwCrcImportFunctions = CRC(gVM.dwCrcImportFunctions, TOK_PARAM);
	gVM.dwCrcImportFunctions = CRC(gVM.dwCrcImportFunctions, PARAM_NORMAL);
	gVM.dwCrcImportFunctions = CRC(gVM.dwCrcImportFunctions, type);

	return param;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InitializeImportTable(
	void)
{
#define IMPORT_TABLE
	#include "imports.h"
#undef  IMPORT_TABLE

	return TRUE;
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ScriptInitEx(
	void)
{
	for (int ii = 0; ii < 256; ii++)
	{
		if (isalnum(ii))
		{
			isalnum_table[ii] = 1;
			isident_table[ii] = 1;
		}
		else
		{
			isalnum_table[ii] = 0;
			isident_table[ii] = (ii == '_' ? 1 : 0);
		}
	}

	memclear(&gVM, sizeof(gVM));

	gVM.cs.Init();
	
	CSAutoLock autolock(&gVM.cs);

	gVM.globalsize = DEFAULT_VM_GLOBAL_SIZE;
	gVM.StringTable = ScriptStringTableInitEx();
	gVM.SymbolTable = ScriptSymbolTableInit();
	for (int ii = 0; ii < arrsize(KeywordStr); ii++)
	{
		int len;
		DWORD key;
		const char * str = KeywordStr[ii];
		StrGetLenAndKey(str, &len, &key);
		ASSERT_CONTINUE(SymbolTableFind(gVM.SymbolTable, str, len, key) == NULL);
		SymbolTableAdd(gVM.SymbolTable, (ETOKEN)(TOK_LAST + 1 + ii), str, len, key);
	}
	for (int ii = 0; ii < arrsize(ExternTypeDef); ii++)
	{
		int len;
		DWORD key;
		const char * str = ExternTypeDef[ii].str;
		StrGetLenAndKey(str, &len, &key);
		ASSERT_CONTINUE(SymbolTableFind(gVM.SymbolTable, str, len, key) == NULL);
		SSYMBOL * sym = SymbolTableAdd(gVM.SymbolTable, TOK_TYPEDEF, str, len, key);
		sym->index = ii;
		sym->type.type = ExternTypeDef[ii].type;
	}
	for (int ii = 0; ii < arrsize(ContextDef); ii++)
	{
		int len;
		DWORD key;
		const char * str = ContextDef[ii].str;
		StrGetLenAndKey(str, &len, &key);
		ASSERT_CONTINUE(SymbolTableFind(gVM.SymbolTable, str, len, key) == NULL);
		SSYMBOL * sym = SymbolTableAdd(gVM.SymbolTable, TOK_CONTEXTVAR, str, len, key);
		sym->index = ii;
		sym->type.type = ContextDef[ii].type;
	}
	InitializeImportTable();

	gVM.vm = VMInitGame(NULL);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ScriptRegisterTableNames(
	void)
{
	for (int ii = 0; ii < NUM_DATATABLES; ii++)
	{
		int len;
		DWORD key;
		const char * str = ExcelGetTableName((EXCELTABLE)ii);
		if (!str)
		{
			continue;
		}
		StrGetLenAndKey(str, &len, &key);
		SSYMBOL * sym = SymbolTableFind(gVM.SymbolTable, str, len, key);
		if (sym)
		{
			ASSERT_CONTINUE(sym->tok == TOK_DATATABLE);
		}
		else
		{
			sym = SymbolTableAdd(gVM.SymbolTable, TOK_DATATABLE, str, len, key);
		}
		sym->index = ii;
		sym->type.type = TYPE_INT;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ScriptRegisterStatSymbols(
	EXCEL_TABLE * table)
{
	ASSERT_RETFALSE(table);
	int statcount = table->m_Data.Count();
	for (int ii = 0; ii < statcount; ii++)
	{
		const STATS_DATA * stats_data = (const STATS_DATA *)ExcelGetDataPrivate(table, ii);
		if (stats_data)
		{
			int len;
			DWORD key;
			StrGetLenAndKey(stats_data->m_szName, &len, &key);
			ASSERTX_CONTINUE(SymbolTableFind(gVM.SymbolTable, stats_data->m_szName, len, key) == NULL, "Duplicate entry found in script symbol table");
			SSYMBOL * sym = SymbolTableAdd(gVM.SymbolTable, TOK_RSTAT, stats_data->m_szName, len, key);
			sym->index = MAKE_STAT(ii, 0);
			sym->type.type = TYPE_INT;
		}
	}
	SSYMBOL * symbol = SymbolTableAdd(gVM.SymbolTable, TOK_PAKMARKER, NULL, 0, 0);
	ASSERT_RETFALSE(symbol);
	symbol->index = -1;
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ScriptFreeEx(
	void)
{
	// CHB 2006.09.05 - Would cause an access violation during
	// abort of initialization.
	bool bInitialized = (gVM.vm != 0);

	if (bInitialized)
	{
		gVM.cs.Enter();
	}

	if (gVM.vm != 0)
	{
		VMFreeGame(gVM.vm);
		gVM.vm = NULL;
	}

	if (gVM.FunctionTable)
	{
		for (unsigned int ii = 0; ii < gVM.function_count; ii++)
		{
			FUNCTION * function = gVM.FunctionTable[ii];
			if (function->symbol->tok == TOK_FUNCTION && function->stream)
			{
				ByteStreamFree(function->stream);
			}
			if (function->parameters)
			{
				int paramcount = FunctionGetParameterCount(function);
				for (int ii = 0; ii < paramcount; ii++)
				{
					SSYMBOL * param = FunctionGetParameter(function, ii);
					if (param)
					{
						SymbolFree(param);
					}
				}
				FREE(NULL, function->parameters);
			}
			FREE(NULL, function);
		}
		FREE(NULL, gVM.FunctionTable);
	}
	ScriptStringTableFree(gVM.StringTable);
	ScriptSymbolTableFree(gVM.SymbolTable);

	if (bInitialized)
	{
		gVM.cs.Free();
	}

	memclear(&gVM, sizeof(gVM));
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sScriptWriteStringTableToPak(
	WRITE_BUF_DYNAMIC & fbuf)
{
	STRING_TABLE2 * tbl = gVM.StringTable;
	ASSERT_RETFALSE(tbl);

	// write header
	FILE_HEADER hdr;
	hdr.dwMagicNumber = SCRIPT_STRING_MAGIC;
	hdr.nVersion = SCRIPT_STRING_VERSION;
	ASSERT_RETFALSE(fbuf.PushInt(hdr.dwMagicNumber));
	ASSERT_RETFALSE(fbuf.PushInt(hdr.nVersion));

	// write count
	ASSERT_RETFALSE(fbuf.PushInt(tbl->count));

	unsigned int written = 0;
	for (unsigned int ii = 0; ii < STRING_TABLE_HASH_SIZE; ii++)
	{
		STRING_NODE * node;
		STRING_NODE * next = tbl->hash[ii];
		while ((node = next) != NULL)
		{
			next = node->next;

			ASSERT_RETFALSE(node->str);
			ASSERT_RETFALSE(fbuf.PushInt(node->hash));
			ASSERT_RETFALSE(fbuf.PushInt(node->len));
			ASSERT_RETFALSE(fbuf.PushBuf(node->str, node->len));
			written++;
		}
	}
	ASSERT_RETFALSE(written == tbl->count);
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ScriptReadStringTableFromPak(
	BYTE_BUF & buf)
{
	FILE_HEADER hdr;
	ASSERT_RETFALSE(buf.PopBuf(&hdr, sizeof(hdr)));
	if (hdr.dwMagicNumber != SCRIPT_STRING_MAGIC || hdr.nVersion != SCRIPT_STRING_VERSION)
	{
		return FALSE;
	}

	int count;
	ASSERT_RETFALSE(buf.PopBuf(&count, sizeof(count)));
	for (int ii = 0; ii < count; ii++)
	{
		DWORD hash;
		ASSERT_RETFALSE(buf.PopBuf(&hash, sizeof(hash)));
		int len;
		ASSERT_RETFALSE(buf.PopBuf(&len, sizeof(len)));
		char str[4096];
		ASSERT(len < 4096);
		ASSERT_RETFALSE(buf.PopBuf(str, len));
		str[len] = 0;

		StringTableAddEx(str, len, hash);
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sScriptWriteSymbolToPak(
	WRITE_BUF_DYNAMIC & fbuf,
	SSYMBOL * sym)
{
	ASSERT_RETFALSE(sym);
	ASSERT_RETFALSE(sym->str);

	if (sym->tok == TOK_IDENT)
	{
		return TRUE;
	}
	// header
	FILE_HEADER hdr;
	hdr.dwMagicNumber = SCRIPT_SYMBOL_MAGIC;
	hdr.nVersion = SCRIPT_SYMBOL_VERSION;
	ASSERT_RETFALSE(fbuf.PushInt(hdr.dwMagicNumber));
	ASSERT_RETFALSE(fbuf.PushInt(hdr.nVersion));

	int len;
	DWORD key;
	StrGetLenAndKey(sym->str, &len, &key);
	ASSERT(key == sym->key);
	ASSERT_RETFALSE(fbuf.PushInt(len));
	ASSERT_RETFALSE(fbuf.PushBuf(sym->str, len));
	ASSERT_RETFALSE(fbuf.PushInt(sym->key));
	ASSERT_RETFALSE(fbuf.PushInt(sym->tok));
	ASSERT_RETFALSE(fbuf.PushInt(sym->type.type));
	ASSERT(sym->type.sym == NULL);
	ASSERT_RETFALSE(fbuf.PushInt(sym->block_level));
	ASSERT_RETFALSE(fbuf.PushInt(sym->type_size));
	ASSERT_RETFALSE(fbuf.PushInt(sym->index));
	if (sym->type.type & TYPE_ARRAY)
	{
		ASSERT_RETFALSE(fbuf.PushInt(sym->arraysize));
	}
	else if (sym->type.type == TYPE_STRUCT)
	{
		ASSERT_RETFALSE(fbuf.PushInt(sym->fieldcount));
		ASSERT(sym->fields == NULL);
	}
	else if (sym->tok == TOK_PARAM)
	{
		ASSERT_RETFALSE(fbuf.PushInt(sym->parameter_type));
		if (sym->parameter_type == PARAM_INDEX)
		{
			ASSERT_RETFALSE(fbuf.PushInt(sym->index_table));
			ASSERT_RETFALSE(fbuf.PushInt(sym->index_index));
		}
	}
	if (sym->tok == TOK_FUNCTION)
	{
		FUNCTION * function = sym->function;
		ASSERT_RETFALSE(function);
		ASSERT_RETFALSE(function->stream);
		ASSERT_RETFALSE(fbuf.PushInt(function->index));
		ASSERT_RETFALSE(fbuf.PushInt(function->parameter_count));
		ASSERT_RETFALSE(fbuf.PushInt(function->min_param_count));
		ASSERT_RETFALSE(fbuf.PushInt(function->varargs));
		for (int ii = 0; ii < function->parameter_count; ii++)
		{
			ASSERT_RETFALSE(sScriptWriteSymbolToPak(fbuf, function->parameters[ii]));
		}
		ASSERT(function->stream->cur == 0);
		ASSERT_RETFALSE(fbuf.PushInt(function->stream->size));
		ASSERT_RETFALSE(fbuf.PushBuf(function->stream->buf, function->stream->size));
	}
	else if (sym->tok == TOK_MACRO_OBJ)
	{
		ASSERT(sym->replacement->cur == 0);
		ASSERT_RETFALSE(fbuf.PushInt(sym->replacement->size));
		ASSERT_RETFALSE(fbuf.PushBuf(sym->replacement->buf, sym->replacement->size));
	}
	else if (sym->tok != TOK_PARAM)
	{
		ASSERT(sym->replacement == NULL);
		ASSERT(sym->function == NULL);
/*
		BYTE_STREAM *	replacement;		// replacement stream for macros/defines
		FUNCTION *		function;			// function
*/
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SSYMBOL * ScriptReadSymbolFromPak(
	BYTE_BUF & buf)
{
	char str[4096];
	int len;
	DWORD key;
	ETOKEN tok;

	ASSERT_RETFALSE(buf.PopBuf(&len, sizeof(len)));
	ASSERT_RETFALSE(len < 4096);
	ASSERT_RETFALSE(buf.PopBuf(str, len)); str[len] = 0;
	ASSERT_RETFALSE(buf.PopBuf(&key, sizeof(key)));
	ASSERT_RETFALSE(buf.PopBuf(&tok, sizeof(tok)));

	SSYMBOL * sym = SymbolMakeNew(tok, str, len, key);
	ASSERT_RETNULL(sym);

	ONCE
	{
		ASSERT_BREAK(buf.PopBuf(&sym->type.type, sizeof(sym->type.type)));
		ASSERT_BREAK(buf.PopBuf(&sym->block_level, sizeof(sym->block_level)));
		ASSERT_BREAK(buf.PopBuf(&sym->type_size, sizeof(sym->type_size)));
		ASSERT_BREAK(buf.PopBuf(&sym->index, sizeof(sym->index)));

		if (sym->type.type & TYPE_ARRAY)
		{
			ASSERT_BREAK(buf.PopBuf(&sym->arraysize, sizeof(sym->arraysize)));
		}
		else if (sym->type.type == TYPE_STRUCT)
		{
			ASSERT_BREAK(buf.PopBuf(&sym->fieldcount, sizeof(sym->fieldcount)));
		}
		else if (sym->tok == TOK_PARAM)
		{
			ASSERT_BREAK(buf.PopBuf(&sym->parameter_type, sizeof(sym->parameter_type)));
			if (sym->parameter_type == PARAM_INDEX)
			{
				ASSERT_BREAK(buf.PopBuf(&sym->index_table, sizeof(sym->index_table)));
				ASSERT_BREAK(buf.PopBuf(&sym->index_index, sizeof(sym->index_index)));
			}
		}
		if (sym->tok == TOK_FUNCTION)
		{
			FUNCTION * function = FunctionTableAdd(sym);
			ASSERT_BREAK(function);

			int index, param_count, min_param_count;
			ASSERT_BREAK(buf.PopBuf(&index, sizeof(param_count)));
			ASSERT_BREAK(index == function->index);
			ASSERT_BREAK(buf.PopBuf(&param_count, sizeof(param_count)));
			ASSERT_BREAK(buf.PopBuf(&min_param_count, sizeof(min_param_count)));
			ASSERT_BREAK(buf.PopBuf(&function->varargs, sizeof(function->varargs)));

			int ii;
			for (ii = 0; ii < param_count; ii++)
			{
				FILE_HEADER hdr;
				ASSERT_BREAK(buf.PopBuf(&hdr, sizeof(hdr)));
				ASSERT_BREAK(hdr.dwMagicNumber == SCRIPT_SYMBOL_MAGIC && hdr.nVersion == SCRIPT_SYMBOL_VERSION);
				SSYMBOL * param = ScriptReadSymbolFromPak(buf);
				ASSERT_BREAK(FunctionAddParameter(function, param, ii < min_param_count));
				ASSERT_BREAK(param);
			}
			if (ii < param_count)
			{
				function->symbol = NULL;
				break;
			}

			function->stream = ByteStreamInit(NULL);
			ASSERT_BREAK(function->stream);
			unsigned int size;
			ASSERT_BREAK(buf.PopBuf(&size, sizeof(size)));
			ASSERT(size < 16384);
			ASSERT_BREAK(ByteStreamResize(function->stream, size));
			ASSERT_BREAK(buf.PopBuf(function->stream->buf, size));
			function->stream->cur += size;
		}
		else if (sym->tok == TOK_MACRO_OBJ)
		{
			sym->replacement = ByteStreamInit(NULL);
			ASSERT_BREAK(sym->replacement);
			BOOL result = FALSE;
			ONCE
			{
				ASSERT_BREAK(buf.PopBuf(&sym->replacement->size, sizeof(sym->replacement->size)));
				unsigned int size;
				ASSERT_BREAK(buf.PopBuf(&size, sizeof(size)));
				ASSERT(size < 16384);
				ASSERT_BREAK(ByteStreamResize(sym->replacement, size));
				ASSERT_BREAK(buf.PopBuf(sym->replacement->buf, size));
				sym->replacement->cur += size;
				result = TRUE;
			}
			if (!result)
			{
				FREE(NULL, sym->replacement);
				break;
			}
		}
		return sym;
	}

	FREE(NULL, sym);
	return NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sScriptWriteSymbolTableToPak(
	unsigned int marker,
	WRITE_BUF_DYNAMIC & fbuf)
{
	SYMBOL_TABLE * tbl = gVM.SymbolTable;
	ASSERT_RETFALSE(tbl);
	SSYMBOL * sym;
	SSYMBOL * prev = tbl->last;
	while ((sym = prev) != NULL)
	{
		prev = sym->prev;

		if (sym->tok == TOK_PAKMARKER && sym->index == (int)marker)
		{
			break;
		}
	}
	ASSERT_RETFALSE(sym);

	// write header
	{
		FILE_HEADER hdr;
		hdr.dwMagicNumber = SCRIPT_SYMBOL_TABLE_MAGIC;
		hdr.nVersion = SCRIPT_SYMBOL_TABLE_VERSION;
		ASSERT_RETFALSE(fbuf.PushInt(hdr.dwMagicNumber));
		ASSERT_RETFALSE(fbuf.PushInt(hdr.nVersion));
	}
	
	SSYMBOL * next = sym->next;
	while ((sym = next) != NULL)
	{
		next = sym->next;

		ASSERT_RETFALSE(sScriptWriteSymbolToPak(fbuf, sym));
	}

	// write footer
	{
		FILE_HEADER hdr;
		hdr.dwMagicNumber = SCRIPT_SYMBOL_END_MAGIC;
		hdr.nVersion = 0;
		ASSERT_RETFALSE(fbuf.PushInt(hdr.dwMagicNumber));
		ASSERT_RETFALSE(fbuf.PushInt(hdr.nVersion));
	}
	
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ScriptReadSymbolTableFromPak(
	BYTE_BUF & fbuf)
{
	FILE_HEADER hdr;
	ASSERT_RETFALSE(fbuf.PopInt(&hdr.dwMagicNumber, sizeof(hdr.dwMagicNumber)));
	ASSERT_RETFALSE(hdr.dwMagicNumber == SCRIPT_SYMBOL_TABLE_MAGIC);
	ASSERT_RETFALSE(fbuf.PopInt(&hdr.nVersion, sizeof(hdr.nVersion)));
	ASSERT_RETFALSE(hdr.nVersion == SCRIPT_SYMBOL_TABLE_VERSION);

	while (TRUE)
	{
		ASSERT_RETFALSE(fbuf.PopInt(&hdr.dwMagicNumber, sizeof(hdr.dwMagicNumber)));
		ASSERT_RETFALSE(fbuf.PopInt(&hdr.nVersion, sizeof(hdr.nVersion)));

		if (hdr.dwMagicNumber == SCRIPT_SYMBOL_END_MAGIC)
		{
			SymbolTableTrace(gVM.SymbolTable);
			return TRUE;
		}
		ASSERT_RETFALSE(hdr.dwMagicNumber == SCRIPT_SYMBOL_MAGIC);
		ASSERT_RETFALSE(hdr.nVersion == SCRIPT_SYMBOL_VERSION);
		SSYMBOL * sym = ScriptReadSymbolFromPak(fbuf);
		ASSERT_RETFALSE(sym);
		SymbolTableAdd(gVM.SymbolTable, sym);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD ScriptGetImportCRC(
	void)
{
	return gVM.dwCrcImportFunctions;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ScriptWriteGlobalVMToPak(
	void)
{
	if (!AppCommonUsePakFile())
	{
		return TRUE;
	}
	
	WRITE_BUF_DYNAMIC fbuf;

	// write header
	FILE_HEADER hdr;
	hdr.dwMagicNumber = SCRIPT_MAGIC;
	hdr.nVersion = SCRIPT_VERSION;
	ASSERT_RETFALSE(fbuf.PushInt(hdr.dwMagicNumber));
	ASSERT_RETFALSE(fbuf.PushInt(hdr.nVersion));

	{
		CSAutoLock autolock(&gVM.cs);

		ASSERT_RETFALSE(sScriptWriteStringTableToPak(fbuf));
		ASSERT_RETFALSE(sScriptWriteSymbolTableToPak((unsigned int)-1, fbuf));
	}

	UINT64 gentime = 0;
	if (!fbuf.Save(SCRIPT_FILENAME, FILE_PATH_SCRIPT, &gentime))
	{
		return FALSE;
	}

	TCHAR filename[MAX_PATH];
	PStrPrintf(filename, MAX_PATH, _T("%s%s"), FILE_PATH_SCRIPT, SCRIPT_FILENAME);

	// history debugging
	FileDebugCreateAndCompareHistory(filename);
		
	// add cooked file to the pak file
	DECLARE_LOAD_SPEC(spec, filename);
	spec.buffer = (void *)fbuf.GetBuf();
	spec.size = fbuf.GetPos();
	spec.gentime = gentime;
	PakFileAddFile(spec);

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ScriptReadGlobalVMFromPak(
	void)
{
	if (!AppCommonUsePakFile())
	{
		return FALSE;
	}

	char szFilename[MAX_PATH];
	PStrPrintf(szFilename, arrsize(szFilename), "%s%s", FILE_PATH_SCRIPT, SCRIPT_FILENAME);

	DECLARE_LOAD_SPEC(spec, szFilename);
	BYTE * buffer = (BYTE *)PakFileLoadNow(spec);
	if (!buffer)
	{
		return FALSE;
	}
	AUTOFREE autofree(NULL, buffer);

	BYTE_BUF buf(buffer, spec.bytesread + 1);

	CSAutoLock autolock(&gVM.cs);

	FILE_HEADER hdr;
	ASSERT_RETFALSE(buf.PopBuf(&hdr, sizeof(hdr)));
	if (hdr.dwMagicNumber != SCRIPT_MAGIC || hdr.nVersion != SCRIPT_VERSION)
	{
		return FALSE;
	}
	ASSERT_RETFALSE(ScriptReadStringTableFromPak(buf));
	ASSERT_RETFALSE(ScriptReadSymbolTableFromPak(buf));

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ScriptAddMarker(
	unsigned int marker)
{
	SSYMBOL * symbol = SymbolTableAdd(gVM.SymbolTable, TOK_PAKMARKER, NULL, 0, 0);
	ASSERT_RETFALSE(symbol);
	symbol->index = marker;
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ScriptWriteToBuffer(
	unsigned int marker,
	WRITE_BUF_DYNAMIC & fbuf)
{
	// write header
	FILE_HEADER hdr;
	hdr.dwMagicNumber = SCRIPT_MAGIC;
	hdr.nVersion = SCRIPT_VERSION;
	ASSERT_RETFALSE(fbuf.PushInt(hdr.dwMagicNumber));
	ASSERT_RETFALSE(fbuf.PushInt(hdr.nVersion));

	CSAutoLock autolock(&gVM.cs);

	ASSERT_RETFALSE(sScriptWriteSymbolTableToPak(marker, fbuf));

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ScriptReadFromBuffer(
	BYTE_BUF & fbuf)
{
	CSAutoLock autolock(&gVM.cs);

	FILE_HEADER hdr;
	ASSERT_RETFALSE(fbuf.PopInt(&hdr.dwMagicNumber, sizeof(hdr.dwMagicNumber)));
	ASSERT_RETFALSE(hdr.dwMagicNumber == SCRIPT_MAGIC);
	ASSERT_RETFALSE(fbuf.PopInt(&hdr.nVersion, sizeof(hdr.nVersion)));
	ASSERT_RETFALSE(hdr.nVersion == SCRIPT_VERSION);
	ASSERT_RETFALSE(ScriptReadSymbolTableFromPak(fbuf));

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
VMSTATE * VMInitContext(
	struct MEMORYPOOL * pool)
{
	VMSTATE * vmstate = (VMSTATE *)MALLOCZ(pool, sizeof(VMSTATE));
	vmstate->m_MemPool = pool;
	vmstate->localsize = DEFAULT_VM_LOCAL_SIZE;
	vmstate->Locals = (BYTE *)MALLOCZ(vmstate->m_MemPool, vmstate->localsize);

	vmstate->stacksize = DEFAULT_VM_STACK_SIZE;
	vmstate->Stack = (VALUE*)MALLOCZ(vmstate->m_MemPool, vmstate->stacksize * sizeof(VALUE));

	return vmstate;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void VMFreeContext(
	VMSTATE * vmstate)
{
	ASSERT_RETURN(vmstate);
	if (vmstate->Stack)
	{
		FREE(vmstate->m_MemPool, vmstate->Stack);
	}
	if (vmstate->Locals)
	{
		FREE(vmstate->m_MemPool, vmstate->Locals);
	}
	if (vmstate->CallStack)
	{
		for (unsigned int ii = 0; ii < vmstate->callstacktop; ii++)
		{
			ByteStreamFree(vmstate->CallStack[ii]);
		}
		FREE(vmstate->m_MemPool, vmstate->CallStack);
	}

	FREE(vmstate->m_MemPool, vmstate);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void VMInitState(
	VMSTATE * vm)
{
	vm->pre_stat_source = vm->cur_stat_source = STAT_SOURCE_NONE;
	if (vm->context.stats)
	{
		if (StatsIsRider(vm->context.stats) && vm->context.unit)
		{
			vm->pre_stat_source = vm->cur_stat_source = STAT_SOURCE_RIDER;
		}
		else
		{
			vm->pre_stat_source = vm->cur_stat_source = STAT_SOURCE_STATS;
		}
	}
	else if (vm->context.unit)
	{
		vm->pre_stat_source = vm->cur_stat_source = STAT_SOURCE_UNIT;
	}
	else if (vm->context.object)
	{
		vm->pre_stat_source = vm->cur_stat_source = STAT_SOURCE_OBJECT;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct VMGAME * VMInitGame(
	struct MEMORYPOOL * pool)
{
	VMGAME * vm = (VMGAME *)MALLOCZ(pool, sizeof(VMGAME));
	vm->m_MemPool = pool;
	vm->m_Globals = (BYTE *)MALLOCZ(vm->m_MemPool, gVM.globalsize);
	return vm;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void VMFreeGame(
	struct VMGAME * vm)
{
	ASSERT_RETURN(vm);
	if (vm->m_VMStateStack)
	{
		for (unsigned int ii = 0; ii < vm->m_VMStateStackSize; ++ii)
		{
			ASSERT_CONTINUE(vm->m_VMStateStack[ii]);
			VMFreeContext(vm->m_VMStateStack[ii]);
		}
		FREE(vm->m_MemPool, vm->m_VMStateStack);
	}
	FREE(vm->m_MemPool, vm->m_Globals);
	FREE(vm->m_MemPool, vm);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct VMSTATE * VMGetVMState(
	struct VMGAME * vm)
{
	ASSERT_RETNULL(vm);
	if (vm->m_VMStateStackCur >= vm->m_VMStateStackSize)
	{
		vm->m_VMStateStack = (VMSTATE **)REALLOCZ(vm->m_MemPool, vm->m_VMStateStack, (vm->m_VMStateStackSize + 1) * sizeof(VMSTATE *));
		vm->m_VMStateStack[vm->m_VMStateStackSize] = VMInitContext(vm->m_MemPool);
		++vm->m_VMStateStackSize;
	}
	VMSTATE * vmstate = vm->m_VMStateStack[vm->m_VMStateStackCur];
	ASSERT_RETNULL(vmstate);
	++vm->m_VMStateStackCur;

	// clear values
	vmstate->m_vm = vm;
	vmstate->Globals = vm->m_Globals;
	vmstate->stacktop = 0;
	vmstate->stackthis = 0;
	vmstate->localcur = 0;
	vmstate->retval.i = 0;
	vmstate->callstacktop = 0;
	memclear(&vmstate->context, sizeof(SCRIPT_CONTEXT));

	return vmstate;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void VMReleaseState(
	struct VMSTATE * vmstate)
{
	ASSERT_RETURN(vmstate);
	ASSERT_RETURN(vmstate->m_vm);
	ASSERT_RETURN(vmstate->m_vm->m_VMStateStackCur > 0);
	--vmstate->m_vm->m_VMStateStackCur;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#ifdef _WIN64
	// For the x64 platform these functions exist in script_x64.asm.
	// At the moment, VMCallStdCall and GetST0 have not been ported to the x64
	// platform because we do not make use of them.

	extern "C" DWORD VMCallCDecl(VMSTATE*, const void*, size_t, void*);
	//extern "C" DWORD VMCallStdCall(VMSTATE*, const void*, size_t, void*);
	//extern "C" DWORD GetST0(void);
#else
	DWORD VMCallCDecl(
					  VMSTATE * vm,
					  const void * args,
					  size_t sz,
					  void * func)
	{
		DWORD rc;
		__asm
		{
			mov ecx, sz			// size of args buffer
				mov esi, args		// buffer
				sub esp, ecx		// allocate stack space
				mov edi, esp		// start of dest stack frame
				shr ecx, 2			// size of args in dwords
				rep movsd			// copy params to stack
				call [func]			// call function
			mov rc, eax			// save retval
				add esp, sz			// restore stack pointer
		}
		return rc;
	}

	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	DWORD VMCallStdCall(
						VMSTATE * vm,
						const void * args,
						size_t sz,
						void * func)
	{
		DWORD rc;
		__asm
		{
			mov ecx, sz			// size of args buffer
				mov esi, args		// buffer
				sub esp, ecx		// allocate stack space
				mov edi, esp		// start of dest stack frame
				shr ecx, 2			// size of args in dwords
				rep movsd			// copy params to stack
				call [func]			// call function
			mov rc, eax			// save retval
		}
		return rc;
	}

	//----------------------------------------------------------------------------
	//----------------------------------------------------------------------------
	__declspec (naked) DWORD GetST0(void)
	{
		DWORD f;
		__asm
		{
			fstp dword ptr [f]			// pop STO into f
			mov eax, dword ptr [f]		// copy int eax
			ret							// done
		};
	}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline int fp_VMI_RESET(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	vm->stacktop = vm->stackthis;
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_CALL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int func;
	ASSERT_RETZERO(ByteStreamGetDword(&str, func));

	FUNCTION * function = FunctionTableGet(func);
	ASSERT_RETZERO(function);
	ASSERT_RETZERO(function->symbol);
	ASSERT_RETZERO(function->symbol->tok == TOK_FUNCTION);

	unsigned int localoffset;
	ASSERT_RETZERO(ByteStreamGetDword(&str, localoffset));

	unsigned int paramcount = function->parameter_count;

	ScriptExecTrace("call: %s\n", function->symbol->str);

	// add current byte stream to call stack
	if (vm->callstacktop >= vm->callstacksize)
	{
		vm->callstacksize++;
		vm->CallStack = (BYTE_STREAM *)REALLOC(vm->m_MemPool, vm->CallStack, sizeof(BYTE_STREAM) * (vm->callstacksize));
		ASSERT_RETZERO(vm->CallStack);
	}
	vm->CallStack[vm->callstacktop] = str;
	vm->CallStack[vm->callstacktop].localstack = localoffset;

	// put parameters on local stack
	vm->localcur += localoffset;
	int localcur = vm->localcur;
	for (unsigned int ii = 0; ii < paramcount; ii++)
	{
		SSYMBOL * parameter = function->parameters[ii];
		ASSERT_RETZERO(parameter);

		VALUE value = vm->Stack[vm->stacktop-paramcount+ii];
		localcur += StrAddValue(vm->Locals + localcur, parameter->type, value);
	}
	vm->stacktop -= paramcount;
	ASSERT_RETZERO(vm->stacktop >= 0);
	vm->CallStack[vm->callstacktop].valuestack = vm->stacktop;
	vm->stackthis = vm->stacktop;

	vm->callstacktop++;
	// set stream to function to call
	BYTE_STREAM * funcstream = function->stream;
	ASSERT_RETZERO(funcstream);
	ASSERT_RETZERO(funcstream->size > 0);
	str = *funcstream;
	str.cur = 0;
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ICALL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int func;
	ASSERT_RETZERO(ByteStreamGetDword(&str, func));

	FUNCTION * function = FunctionTableGet(func);
	ASSERT_RETZERO(function);
	ASSERT_RETZERO(function->symbol);
	ASSERT_RETZERO(function->symbol->tok == TOK_IFUNCTION);
	ASSERT_RETZERO(function->fptr);

	unsigned int paramcount = function->parameter_count;

#ifdef _WIN64
	QWORD paramstack[10];
#else
	DWORD paramstack[10];
#endif
	ASSERT_RETZERO(paramcount < 10);
	ASSERT_RETZERO(vm->stacktop >= paramcount);
	// put parameters on stack
	for (unsigned int ii = 0; ii < paramcount; ii++)
	{
		VALUE value = vm->Stack[vm->stacktop-paramcount+ii];
#ifdef _WIN64
		paramstack[ii] = value.ull;
#else
		paramstack[ii] = value.ui;
#endif
	}
	vm->stacktop -= paramcount;

	ScriptExecTrace("call: %s\n", function->symbol->str);
	int result = VMCallCDecl(vm, paramstack, paramcount * sizeof(paramstack[0]) , function->fptr);
	
	vm->Stack[vm->stacktop++].i = result;
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_JMP(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	int offset;
	ASSERT_RETZERO(ByteStreamGetInt(&str, offset));
	ScriptExecTrace("jump (addr:%d)\n", offset);
	str.cur = offset;
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_GLOBAL_PTR(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int offset; 
	ASSERT_RETZERO(ByteStreamGetDword(&str, offset));
	vm->Stack[vm->stacktop++].ui = offset; 
	ScriptExecTrace("push global address = %u  type = unsigned int\n", offset);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_LOCAL_PTR(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int offset; 
	ASSERT_RETZERO(ByteStreamGetDword(&str, offset));
	vm->Stack[vm->stacktop++].ui = offset; 
	ScriptExecTrace("push local address = %u  type = unsigned int\n", offset);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_IGLOBAL_PTR(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	unsigned int value;
	ASSERT_RETZERO(ByteStreamGetDword(&str, value));
	vm->Stack[vm->stacktop-1].ui += value; 
	ScriptExecTrace("push global address = %u  type = unsigned int\n", vm->Stack[vm->stacktop-1].ui);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ILOCAL_PTR(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	unsigned int value;
	ASSERT_RETZERO(ByteStreamGetDword(&str, value));
	vm->Stack[vm->stacktop-1].ui += value; 
	ScriptExecTrace("push local address = %u  type = unsigned int\n", vm->Stack[vm->stacktop-1].ui);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_RETURN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ScriptExecTrace("return\n");
	ASSERT_RETZERO(vm->callstacktop > 0);
	str = vm->CallStack[vm->callstacktop-1];
	ASSERT_RETZERO(vm->localcur >= str.localstack);
	vm->localcur -= str.localstack;
	ASSERT_RETZERO(vm->stacktop > str.valuestack);
	VALUE value = vm->Stack[vm->stacktop-1];
	vm->stacktop = str.valuestack;
	vm->Stack[vm->stacktop++] = value;
	vm->callstacktop--;
	if (vm->callstacktop > 0) 
	{ 
		vm->stackthis = vm->CallStack[vm->callstacktop-1].valuestack; 
	}
	else 
	{ 
		vm->stackthis = 0; 
	}
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_EOF(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	if (vm->callstacktop > 0)
	{
		ScriptExecTrace("return void\n");
		str = vm->CallStack[vm->callstacktop-1];
		ASSERT_RETZERO(vm->localcur >= str.localstack);
		vm->localcur -= str.localstack;
		vm->stacktop = str.valuestack;
		vm->callstacktop--;
		if (vm->callstacktop > 0) 
		{ 
			vm->stackthis = vm->CallStack[vm->callstacktop-1].valuestack; 
		}
		else 
		{ 
			vm->stackthis = 0; 
		}
		return 1;
	}
	if (!(vm->stacktop > 0))
	{
		return 0;
	}
	vm->retval = vm->Stack[vm->stacktop-1];
	return 0;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_JMPZERO_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1);
	char a = vm->Stack[vm->stacktop-1].c;
	if (a)
	{ 
		str.cur += sizeof(int); 
		ScriptExecTrace("jmpzero  offset = ---  type = char\n");
	}
	else 
	{ 
		unsigned int offset; 
		ASSERT_RETZERO(ByteStreamGetDword(&str, offset)); 
		str.cur = offset; 
		ScriptExecTrace("jmpzero  offset = %u  type = char\n", offset);
	}
	vm->stacktop -= 1;
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_JMPZERO_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned char a = vm->Stack[vm->stacktop-1].uc;
	if (a)
	{ 
		str.cur += sizeof(int); 
		ScriptExecTrace("jmpzero  offset = ---  type = unsigned char\n");
	}
	else 
	{ 
		unsigned int offset; 
		ASSERT_RETZERO(ByteStreamGetDword(&str, offset)); 
		str.cur = offset; 
		ScriptExecTrace("jmpzero  offset = %u  type = unsigned char\n", offset);
	}
	vm->stacktop -= 1;
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_JMPZERO_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1);
	short a = vm->Stack[vm->stacktop-1].s;
	if (a)
	{ 
		str.cur += sizeof(int); 
		ScriptExecTrace("jmpzero  offset = ---  type = short\n");
	}
	else 
	{ 
		unsigned int offset; 
		ASSERT_RETZERO(ByteStreamGetDword(&str, offset)); 
		str.cur = offset; 
		ScriptExecTrace("jmpzero  offset = %u  type = short\n", offset);
	}
	vm->stacktop -= 1;
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_JMPZERO_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned short a = vm->Stack[vm->stacktop-1].us;
	if (a)
	{ 
		str.cur += sizeof(int); 
		ScriptExecTrace("jmpzero  offset = ---  type = unsigned short\n");
	}
	else 
	{ 
		unsigned int offset; 
		ASSERT_RETZERO(ByteStreamGetDword(&str, offset)); 
		str.cur = offset; 
		ScriptExecTrace("jmpzero  offset = %u  type = unsigned short\n", offset);
	}
	vm->stacktop -= 1;
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_JMPZERO_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1);
	int a = vm->Stack[vm->stacktop-1].i;
	if (a)
	{
		str.cur += sizeof(int); 
		ScriptExecTrace("jmpzero  offset = ---  type = int\n");
	}
	else 
	{ 
		unsigned int offset; 
		ASSERT_RETZERO(ByteStreamGetDword(&str, offset)); 
		str.cur = offset; 
		ScriptExecTrace("jmpzero  offset = %u  type = int\n", offset);
	}
	vm->stacktop -= 1;
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_JMPZERO_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned int a = vm->Stack[vm->stacktop-1].ui;
	if (a)
	{ 
		str.cur += sizeof(int); 
		ScriptExecTrace("jmpzero  offset = ---  type = unsigned int\n");
	}
	else 
	{ 
		unsigned int offset; 
		ASSERT_RETZERO(ByteStreamGetDword(&str, offset)); 
		str.cur = offset; 
		ScriptExecTrace("jmpzero  offset = %u  type = unsigned int\n", offset);
	}
	vm->stacktop -= 1;
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_JMPZERO_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1);
	INT64 a = vm->Stack[vm->stacktop-1].ll;
	if (a)
	{ 
		str.cur += sizeof(int); 
		ScriptExecTrace("jmpzero  offset = ---  type = int64\n");
	}
	else 
	{ 
		unsigned int offset; 
		ASSERT_RETZERO(ByteStreamGetDword(&str, offset)); 
		str.cur = offset; 
		ScriptExecTrace("jmpzero  offset = %u  type = int64\n", offset);
	}
	vm->stacktop -= 1;
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_JMPZERO_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1);
	UINT64 a = vm->Stack[vm->stacktop-1].ull;
	if (a)
	{ 
		str.cur += sizeof(int); 
		ScriptExecTrace("jmpzero  offset = ---  type = unsigned int64\n");
	}
	else 
	{ 
		unsigned int offset; 
		ASSERT_RETZERO(ByteStreamGetDword(&str, offset)); 
		str.cur = offset; 
		ScriptExecTrace("jmpzero  offset = %u  type = unsigned int64\n", offset);
	}
	vm->stacktop -= 1;
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_JMPZERO_FL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1);
	float a = vm->Stack[vm->stacktop-1].f;
	if (a)
	{ 
		str.cur += sizeof(int); 
		ScriptExecTrace("jmpzero  offset = ---  type = float\n");
	}
	else 
	{ 
		unsigned int offset; 
		ASSERT_RETZERO(ByteStreamGetDword(&str, offset)); 
		str.cur = offset; 
		ScriptExecTrace("jmpzero  offset = %u  type = float\n", offset);
	}
	vm->stacktop -= 1;
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_JMPZERO_DB(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1);
	double a = vm->Stack[vm->stacktop-1].d;
	if (a)
	{ 
		str.cur += sizeof(int); 
		ScriptExecTrace("jmpzero  offset = ---  type = double\n");
	}
	else 
	{ 
		unsigned int offset; 
		ASSERT_RETZERO(ByteStreamGetDword(&str, offset)); 
		str.cur = offset; 
		ScriptExecTrace("jmpzero  offset = %u  type = double\n", offset);
	}
	vm->stacktop -= 1;
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_JMPZERO_LD(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1);
	long double a = vm->Stack[vm->stacktop-1].ld;
	if (a)
	{ 
		str.cur += sizeof(int); 
		ScriptExecTrace("jmpzero  offset = ---  type = long double\n");
	}
	else 
	{ 
		unsigned int offset; 
		ASSERT_RETZERO(ByteStreamGetDword(&str, offset)); 
		str.cur = offset; 
		ScriptExecTrace("jmpzero  offset = %u  type = long double\n", offset);
	}
	vm->stacktop -= 1;
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_JMPZERO_POINTER(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1);
	void * a = vm->Stack[vm->stacktop-1].ptr;
	if (a)
	{ 
		str.cur += sizeof(int); 
		ScriptExecTrace("jmpzero  offset = ---  type = pointer\n");
	}
	else 
	{ 
		unsigned int offset; 
		ASSERT_RETZERO(ByteStreamGetDword(&str, offset)); 
		str.cur = offset; 
		ScriptExecTrace("jmpzero  offset = %u  type = pointer\n", offset);
	}
	vm->stacktop -= 1;
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_CONST_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	char value;
	ASSERT_RETZERO(ByteStreamGetChar(&str, value));
	ASSERT_RETZERO(vm->stacktop < vm->stacksize);
	vm->Stack[vm->stacktop++].c = value; 
	ScriptExecTrace("push constant value = %d  type = char\n", value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_CONST_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned char value;
	ASSERT_RETZERO(ByteStreamGetByte(&str, value));
	ASSERT_RETZERO(vm->stacktop < vm->stacksize);
	vm->Stack[vm->stacktop++].uc = value; 
	ScriptExecTrace("push constant value = %u  type = unsigned char\n", value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_CONST_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	short value;
	ASSERT_RETZERO(ByteStreamGetShort(&str, value));
	ASSERT_RETZERO(vm->stacktop < vm->stacksize);
	vm->Stack[vm->stacktop++].s = value; 
	ScriptExecTrace("push constant value = %d  type = short\n", value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_CONST_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned short value;
	ASSERT_RETZERO(ByteStreamGetWord(&str, value));
	ASSERT_RETZERO(vm->stacktop < vm->stacksize);
	vm->Stack[vm->stacktop++].us = value; 
	ScriptExecTrace("push constant value = %u  type = unsigned short\n", value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_CONST_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	int value;
	ASSERT_RETZERO(ByteStreamGetInt(&str, value));
	ASSERT_RETZERO(vm->stacktop < vm->stacksize);
	vm->Stack[vm->stacktop++].i = value; 
	ScriptExecTrace("push constant value = %d  type = int\n", value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_CONST_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int value;
	ASSERT_RETZERO(ByteStreamGetDword(&str, value));
	ASSERT_RETZERO(vm->stacktop < vm->stacksize);
	vm->Stack[vm->stacktop++].ui = value; 
	ScriptExecTrace("push constant value = %u  type = unsigned int\n", value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_CONST_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	INT64 value;
	ASSERT_RETZERO(ByteStreamGetInt64(&str, value));
	ASSERT_RETZERO(vm->stacktop < vm->stacksize);
	vm->Stack[vm->stacktop++].ll = value; 
	ScriptExecTrace("push constant value = %I64d  type = int64\n", value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_CONST_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	UINT64 value;
	ASSERT_RETZERO(ByteStreamGetUInt64(&str, value));
	ASSERT_RETZERO(vm->stacktop < vm->stacksize);
	vm->Stack[vm->stacktop++].ull = value; 
	ScriptExecTrace("push constant value = %I64u  type = unsigned int64\n", value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_CONST_FL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	float value;
	ASSERT_RETZERO(ByteStreamGetFloat(&str, value));
	ASSERT_RETZERO(vm->stacktop < vm->stacksize);
	vm->Stack[vm->stacktop++].f = value; 
	ScriptExecTrace("push constant value = %f  type = float\n", value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_CONST_DB(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	double value;
	ASSERT_RETZERO(ByteStreamGetDouble(&str, value));
	ASSERT_RETZERO(vm->stacktop < vm->stacksize);
	vm->Stack[vm->stacktop++].d = value; 
	ScriptExecTrace("push constant value = %f  type = double\n", value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_CONST_LD(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	long double value;
	ASSERT_RETZERO(ByteStreamGetLDouble(&str, value));
	ASSERT_RETZERO(vm->stacktop < vm->stacksize);
	vm->Stack[vm->stacktop++].ld = value; 
	ScriptExecTrace("push constant value = %f  type = long double\n", value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_CONST_POINTER(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	VALUE value;
	ASSERT_RETZERO(ByteStreamGetValue(&str, &value));
	ASSERT_RETZERO(vm->stacktop < vm->stacksize);
	vm->Stack[vm->stacktop++] = value; 
	ScriptExecTrace("push constant value = %x  type = pointer\n", value.ptr);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_GLOBAL_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int offset; 
	ASSERT_RETZERO(ByteStreamGetDword(&str, offset));
	char value = *(char *)(vm->Globals + offset); 
	vm->Stack[vm->stacktop++].c = value;
	ScriptExecTrace("push global variable at offset %u  value = %d  type = char\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_GLOBAL_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int offset; 
	ASSERT_RETZERO(ByteStreamGetDword(&str, offset));
	unsigned char value = *(unsigned char *)(vm->Globals + offset); 
	vm->Stack[vm->stacktop++].uc = value;
	ScriptExecTrace("push global variable at offset %u  value = %u  type = unsigned char\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_GLOBAL_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int offset; 
	ASSERT_RETZERO(ByteStreamGetDword(&str, offset));
	short value = *(short *)(vm->Globals + offset); 
	vm->Stack[vm->stacktop++].s = value;
	ScriptExecTrace("push global variable at offset %u  value = %d  type = short\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_GLOBAL_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int offset; 
	ASSERT_RETZERO(ByteStreamGetDword(&str, offset));
	unsigned short value = *(unsigned short *)(vm->Globals + offset); 
	vm->Stack[vm->stacktop++].us = value;
	ScriptExecTrace("push global variable at offset %u  value = %u  type = unsigned short\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_GLOBAL_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int offset; 
	ASSERT_RETZERO(ByteStreamGetDword(&str, offset));
	int value = *(int *)(vm->Globals + offset); 
	vm->Stack[vm->stacktop++].i = value;
	ScriptExecTrace("push global variable at offset %u  value = %d  type = int\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_GLOBAL_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int offset; 
	ASSERT_RETZERO(ByteStreamGetDword(&str, offset));
	unsigned int value = *(unsigned int *)(vm->Globals + offset); 
	vm->Stack[vm->stacktop++].ui = value;
	ScriptExecTrace("push global variable at offset %u  value = %u  type = unsigned int\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_GLOBAL_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int offset; 
	ASSERT_RETZERO(ByteStreamGetDword(&str, offset));
	INT64 value = *(INT64 *)(vm->Globals + offset); 
	vm->Stack[vm->stacktop++].ll = value;
	ScriptExecTrace("push global variable at offset %u  value = %I64d  type = int64\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_GLOBAL_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int offset; 
	ASSERT_RETZERO(ByteStreamGetDword(&str, offset));
	UINT64 value = *(UINT64 *)(vm->Globals + offset); 
	vm->Stack[vm->stacktop++].ull = value;
	ScriptExecTrace("push global variable at offset %u  value = %I64u  type = unsigned int64\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_GLOBAL_FL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int offset; 
	ASSERT_RETZERO(ByteStreamGetDword(&str, offset));
	float value = *(float *)(vm->Globals + offset); 
	vm->Stack[vm->stacktop++].f = value;
	ScriptExecTrace("push global variable at offset %u  value = %f  type = float\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_GLOBAL_DB(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int offset; 
	ASSERT_RETZERO(ByteStreamGetDword(&str, offset));
	double value = *(double *)(vm->Globals + offset); 
	vm->Stack[vm->stacktop++].d = value;
	ScriptExecTrace("push global variable at offset %u  value = %f  type = double\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_GLOBAL_LD(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int offset; 
	ASSERT_RETZERO(ByteStreamGetDword(&str, offset));
	long double value = *(long double *)(vm->Globals + offset); 
	vm->Stack[vm->stacktop++].ld = value;
	ScriptExecTrace("push global variable at offset %u  value = %f  type = long double\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_GLOBAL_POINTER(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int offset; 
	ASSERT_RETZERO(ByteStreamGetDword(&str, offset));
	void * value = *(void **)(vm->Globals + offset); 
	vm->Stack[vm->stacktop++].ptr = value;
	ScriptExecTrace("push global variable at offset %u  value = %x  type = pointer\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_LOCAL_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int offset; 
	ASSERT_RETZERO(ByteStreamGetDword(&str, offset));
	char value = *(char *)(vm->Locals + vm->localcur + offset); 
	vm->Stack[vm->stacktop++].c = value;
	ScriptExecTrace("push local variable at offset %u  value = %d  type = char\n", vm->localcur + offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_LOCAL_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int offset; 
	ASSERT_RETZERO(ByteStreamGetDword(&str, offset));
	unsigned char value = *(unsigned char *)(vm->Locals + vm->localcur + offset); 
	vm->Stack[vm->stacktop++].uc = value;
	ScriptExecTrace("push local variable at offset %u  value = %u  type = unsigned char\n", vm->localcur + offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_LOCAL_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int offset; 
	ASSERT_RETZERO(ByteStreamGetDword(&str, offset));
	short value = *(short *)(vm->Locals + vm->localcur + offset); 
	vm->Stack[vm->stacktop++].s = value;
	ScriptExecTrace("push local variable at offset %u  value = %d  type = short\n", vm->localcur + offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_LOCAL_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int offset; 
	ASSERT_RETZERO(ByteStreamGetDword(&str, offset));
	unsigned short value = *(unsigned short *)(vm->Locals + vm->localcur + offset); 
	vm->Stack[vm->stacktop++].us = value;
	ScriptExecTrace("push local variable at offset %u  value = %u  type = unsigned short\n", vm->localcur + offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_LOCAL_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int offset; 
	ASSERT_RETZERO(ByteStreamGetDword(&str, offset));
	int value = *(int *)(vm->Locals + vm->localcur + offset); 
	vm->Stack[vm->stacktop++].i = value;
	ScriptExecTrace("push local variable at offset %u  value = %d  type = int\n", vm->localcur + offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_LOCAL_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int offset; 
	ASSERT_RETZERO(ByteStreamGetDword(&str, offset));
	unsigned int value = *(unsigned int *)(vm->Locals + vm->localcur + offset); 
	vm->Stack[vm->stacktop++].ui = value;
	ScriptExecTrace("push local variable at offset %u  value = %u  type = unsigned int\n", vm->localcur + offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_LOCAL_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int offset; 
	ASSERT_RETZERO(ByteStreamGetDword(&str, offset));
	INT64 value = *(INT64 *)(vm->Locals + vm->localcur + offset); 
	vm->Stack[vm->stacktop++].ll = value;
	ScriptExecTrace("push local variable at offset %u  value = %I64d  type = int64\n", vm->localcur + offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_LOCAL_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int offset; 
	ASSERT_RETZERO(ByteStreamGetDword(&str, offset));
	UINT64 value = *(UINT64 *)(vm->Locals + vm->localcur + offset); 
	vm->Stack[vm->stacktop++].ull = value;
	ScriptExecTrace("push local variable at offset %u  value = %I64u  type = unsigned int64\n", vm->localcur + offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_LOCAL_FL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int offset; 
	ASSERT_RETZERO(ByteStreamGetDword(&str, offset));
	float value = *(float *)(vm->Locals + vm->localcur + offset); 
	vm->Stack[vm->stacktop++].f = value;
	ScriptExecTrace("push local variable at offset %u  value = %f  type = float\n", vm->localcur + offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_LOCAL_DB(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int offset; 
	ASSERT_RETZERO(ByteStreamGetDword(&str, offset));
	double value = *(double *)(vm->Locals + vm->localcur + offset); 
	vm->Stack[vm->stacktop++].d = value;
	ScriptExecTrace("push local variable at offset %u  value = %f  type = double\n", vm->localcur + offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_LOCAL_LD(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int offset; 
	ASSERT_RETZERO(ByteStreamGetDword(&str, offset));
	long double value = *(long double *)(vm->Locals + vm->localcur + offset); 
	vm->Stack[vm->stacktop++].ld = value;
	ScriptExecTrace("push local variable at offset %u  value = %f  type = long double\n", vm->localcur + offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_LOCAL_POINTER(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int offset; 
	ASSERT_RETZERO(ByteStreamGetDword(&str, offset));
	void * value = *(void **)(vm->Locals + vm->localcur + offset); 
	vm->Stack[vm->stacktop++].ptr = value;
	ScriptExecTrace("push local variable at offset %u  value = %x  type = pointer\n", vm->localcur + offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_IGLOBAL_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	unsigned int offset; 
	ASSERT_RETFALSE(ByteStreamGetDword(&str, offset));
	offset += vm->Stack[vm->stacktop-1].ui;
	ASSERT_RETZERO(offset <= gVM.globalcur + sizeof(char));
	char value = *(char *)(vm->Globals + offset); 
	vm->Stack[vm->stacktop-1].c = value;
	ScriptExecTrace("push global (indirect) variable at offset %u  value = %d  type = char\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_IGLOBAL_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	unsigned int offset; 
	ASSERT_RETFALSE(ByteStreamGetDword(&str, offset));
	offset += vm->Stack[vm->stacktop-1].ui;
	ASSERT_RETZERO(offset <= gVM.globalcur + sizeof(unsigned char));
	unsigned char value = *(unsigned char *)(vm->Globals + offset); 
	vm->Stack[vm->stacktop-1].uc = value;
	ScriptExecTrace("push global (indirect) variable at offset %u  value = %u  type = unsigned char\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_IGLOBAL_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	unsigned int offset; 
	ASSERT_RETFALSE(ByteStreamGetDword(&str, offset));
	offset += vm->Stack[vm->stacktop-1].ui;
	ASSERT_RETZERO(offset <= gVM.globalcur + sizeof(short));
	short value = *(short *)(vm->Globals + offset); 
	vm->Stack[vm->stacktop-1].s = value;
	ScriptExecTrace("push global (indirect) variable at offset %u  value = %d  type = short\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_IGLOBAL_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	unsigned int offset; 
	ASSERT_RETFALSE(ByteStreamGetDword(&str, offset));
	offset += vm->Stack[vm->stacktop-1].ui;
	ASSERT_RETZERO(offset <= gVM.globalcur + sizeof(unsigned short));
	unsigned short value = *(unsigned short *)(vm->Globals + offset); 
	vm->Stack[vm->stacktop-1].us = value;
	ScriptExecTrace("push global (indirect) variable at offset %u  value = %u  type = unsigned short\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_IGLOBAL_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	unsigned int offset; 
	ASSERT_RETFALSE(ByteStreamGetDword(&str, offset));
	offset += vm->Stack[vm->stacktop-1].ui;
	ASSERT_RETZERO(offset <= gVM.globalcur + sizeof(int));
	int value = *(int *)(vm->Globals + offset); 
	vm->Stack[vm->stacktop-1].i = value;
	ScriptExecTrace("push global (indirect) variable at offset %u  value = %d  type = int\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_IGLOBAL_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	unsigned int offset; 
	ASSERT_RETFALSE(ByteStreamGetDword(&str, offset));
	offset += vm->Stack[vm->stacktop-1].ui;
	ASSERT_RETZERO(offset <= gVM.globalcur + sizeof(unsigned int));
	unsigned int value = *(unsigned int *)(vm->Globals + offset); 
	vm->Stack[vm->stacktop-1].ui = value;
	ScriptExecTrace("push global (indirect) variable at offset %u  value = %u  type = unsigned int\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_IGLOBAL_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	unsigned int offset; 
	ASSERT_RETFALSE(ByteStreamGetDword(&str, offset));
	offset += vm->Stack[vm->stacktop-1].ui;
	ASSERT_RETZERO(offset <= gVM.globalcur + sizeof(INT64));
	INT64 value = *(INT64 *)(vm->Globals + offset); 
	vm->Stack[vm->stacktop-1].ll = value;
	ScriptExecTrace("push global (indirect) variable at offset %u  value = %I64d  type = int64\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_IGLOBAL_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	unsigned int offset; 
	ASSERT_RETFALSE(ByteStreamGetDword(&str, offset));
	offset += vm->Stack[vm->stacktop-1].ui;
	ASSERT_RETZERO(offset <= gVM.globalcur + sizeof(UINT64));
	UINT64 value = *(UINT64 *)(vm->Globals + offset); 
	vm->Stack[vm->stacktop-1].ull = value;
	ScriptExecTrace("push global (indirect) variable at offset %u  value = %I64u  type = unsigned int64\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_IGLOBAL_FL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	unsigned int offset; 
	ASSERT_RETFALSE(ByteStreamGetDword(&str, offset));
	offset += vm->Stack[vm->stacktop-1].ui;
	ASSERT_RETZERO(offset <= gVM.globalcur + sizeof(float));
	float value = *(float *)(vm->Globals + offset); 
	vm->Stack[vm->stacktop-1].f = value;
	ScriptExecTrace("push global (indirect) variable at offset %u  value = %f  type = float\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_IGLOBAL_DB(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	unsigned int offset; 
	ASSERT_RETFALSE(ByteStreamGetDword(&str, offset));
	offset += vm->Stack[vm->stacktop-1].ui;
	ASSERT_RETZERO(offset <= gVM.globalcur + sizeof(double));
	double value = *(double *)(vm->Globals + offset); 
	vm->Stack[vm->stacktop-1].d = value;
	ScriptExecTrace("push global (indirect) variable at offset %u  value = %f  type = double\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_IGLOBAL_LD(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	unsigned int offset; 
	ASSERT_RETFALSE(ByteStreamGetDword(&str, offset));
	offset += vm->Stack[vm->stacktop-1].ui;
	ASSERT_RETZERO(offset <= gVM.globalcur + sizeof(long double));
	long double value = *(long double *)(vm->Globals + offset); 
	vm->Stack[vm->stacktop-1].ld = value;
	ScriptExecTrace("push global (indirect) variable at offset %u  value = %f  type = long double\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_IGLOBAL_POINTER(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	unsigned int offset; 
	ASSERT_RETFALSE(ByteStreamGetDword(&str, offset));
	offset += vm->Stack[vm->stacktop-1].ui;
	ASSERT_RETZERO(offset <= gVM.globalcur + sizeof(void *));
	void * value = *(void **)(vm->Globals + offset); 
	vm->Stack[vm->stacktop-1].ptr = value;
	ScriptExecTrace("push global (indirect) variable at offset %u  value = %x  type = long double\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ILOCAL_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	unsigned int offset; 
	ASSERT_RETFALSE(ByteStreamGetDword(&str, offset));
	offset += vm->Stack[vm->stacktop-1].ui;
	ASSERT_RETZERO(offset <= vm->localcur + sizeof(char));
	char value = *(char *)(vm->Locals + offset); 
	vm->Stack[vm->stacktop-1].c = value;
	ScriptExecTrace("push local (indirect) variable at offset %u  value = %d  type = char\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ILOCAL_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	unsigned int offset; 
	ASSERT_RETFALSE(ByteStreamGetDword(&str, offset));
	offset += vm->Stack[vm->stacktop-1].ui;
	ASSERT_RETZERO(offset <= vm->localcur + sizeof(unsigned char));
	unsigned char value = *(unsigned char *)(vm->Locals + offset); 
	vm->Stack[vm->stacktop-1].uc = value;
	ScriptExecTrace("push local (indirect) variable at offset %u  value = %u  type = unsigned char\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ILOCAL_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	unsigned int offset; 
	ASSERT_RETFALSE(ByteStreamGetDword(&str, offset));
	offset += vm->Stack[vm->stacktop-1].ui;
	ASSERT_RETZERO(offset <= vm->localcur + sizeof(short));
	short value = *(short *)(vm->Locals + offset); 
	vm->Stack[vm->stacktop-1].s = value;
	ScriptExecTrace("push local (indirect) variable at offset %u  value = %d  type = short\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ILOCAL_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	unsigned int offset; 
	ASSERT_RETFALSE(ByteStreamGetDword(&str, offset));
	offset += vm->Stack[vm->stacktop-1].ui;
	ASSERT_RETZERO(offset <= vm->localcur + sizeof(unsigned short));
	unsigned short value = *(unsigned short *)(vm->Locals + offset); 
	vm->Stack[vm->stacktop-1].us = value;
	ScriptExecTrace("push local (indirect) variable at offset %u  value = %u  type = unsigned short\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ILOCAL_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	unsigned int offset; 
	ASSERT_RETFALSE(ByteStreamGetDword(&str, offset));
	offset += vm->Stack[vm->stacktop-1].ui;
	ASSERT_RETZERO(offset <= vm->localcur + sizeof(int));
	int value = *(int *)(vm->Locals + offset); 
	vm->Stack[vm->stacktop-1].i = value;
	ScriptExecTrace("push local (indirect) variable at offset %u  value = %d  type = int\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ILOCAL_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	unsigned int offset; 
	ASSERT_RETFALSE(ByteStreamGetDword(&str, offset));
	offset += vm->Stack[vm->stacktop-1].ui;
	ASSERT_RETZERO(offset <= vm->localcur + sizeof(unsigned int));
	unsigned int value = *(unsigned int *)(vm->Locals + offset); 
	vm->Stack[vm->stacktop-1].ui = value;
	ScriptExecTrace("push local (indirect) variable at offset %u  value = %u  type = unsigned int\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ILOCAL_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	unsigned int offset; 
	ASSERT_RETFALSE(ByteStreamGetDword(&str, offset));
	offset += vm->Stack[vm->stacktop-1].ui;
	ASSERT_RETZERO(offset <= vm->localcur + sizeof(INT64));
	INT64 value = *(INT64 *)(vm->Locals + offset); 
	vm->Stack[vm->stacktop-1].ll = value;
	ScriptExecTrace("push local (indirect) variable at offset %u  value = %I64d  type = int64\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ILOCAL_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	unsigned int offset; 
	ASSERT_RETFALSE(ByteStreamGetDword(&str, offset));
	offset += vm->Stack[vm->stacktop-1].ui;
	ASSERT_RETZERO(offset <= vm->localcur + sizeof(UINT64));
	UINT64 value = *(UINT64 *)(vm->Locals + offset); 
	vm->Stack[vm->stacktop-1].ull = value;
	ScriptExecTrace("push local (indirect) variable at offset %u  value = %I64u  type = unsigned int64\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ILOCAL_FL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	unsigned int offset; 
	ASSERT_RETFALSE(ByteStreamGetDword(&str, offset));
	offset += vm->Stack[vm->stacktop-1].ui;
	ASSERT_RETZERO(offset <= vm->localcur + sizeof(float));
	float value = *(float *)(vm->Locals + offset); 
	vm->Stack[vm->stacktop-1].f = value;
	ScriptExecTrace("push local (indirect) variable at offset %u  value = %f  type = float\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ILOCAL_DB(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	unsigned int offset; 
	ASSERT_RETFALSE(ByteStreamGetDword(&str, offset));
	offset += vm->Stack[vm->stacktop-1].ui;
	ASSERT_RETZERO(offset <= vm->localcur + sizeof(double));
	double value = *(double *)(vm->Locals + offset); 
	vm->Stack[vm->stacktop-1].d = value;
	ScriptExecTrace("push local (indirect) variable at offset %u  value = %f  type = double\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ILOCAL_LD(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	unsigned int offset; 
	ASSERT_RETFALSE(ByteStreamGetDword(&str, offset));
	offset += vm->Stack[vm->stacktop-1].ui;
	ASSERT_RETZERO(offset <= vm->localcur + sizeof(long double));
	long double value = *(long double *)(vm->Locals + offset); 
	vm->Stack[vm->stacktop-1].ld = value;
	ScriptExecTrace("push local (indirect) variable at offset %u  value = %f  type = long double\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ILOCAL_POINTER(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	unsigned int offset; 
	ASSERT_RETFALSE(ByteStreamGetDword(&str, offset));
	offset += vm->Stack[vm->stacktop-1].ui;
	ASSERT_RETZERO(offset <= vm->localcur + sizeof(void *));
	void * value = *(void **)(vm->Locals + offset); 
	vm->Stack[vm->stacktop-1].ptr = value;
	ScriptExecTrace("push local (indirect) variable at offset %u  value = %x  type = long double\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_GLOBAL_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	char value = vm->Stack[vm->stacktop-1].c;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(char) <= gVM.globalsize);
	*(char *)(vm->Globals + offset) = value; 
	vm->Stack[vm->stacktop-2].c = value;
	vm->stacktop--;
	ScriptExecTrace("assign global variable at offset %u  value = %d  type = char\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_GLOBAL_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned char value = vm->Stack[vm->stacktop-1].uc;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned char) <= gVM.globalsize);
	*(unsigned char *)(vm->Globals + offset) = value; 
	vm->Stack[vm->stacktop-2].uc = value;
	vm->stacktop--;
	ScriptExecTrace("assign global variable at offset %u  value = %u  type = unsigned char\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_GLOBAL_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	short value = vm->Stack[vm->stacktop-1].s;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(short) <= gVM.globalsize);
	*(short *)(vm->Globals + offset) = value; 
	vm->Stack[vm->stacktop-2].s = value;
	vm->stacktop--;
	ScriptExecTrace("assign global variable at offset %u  value = %d  type = short\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_GLOBAL_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned short value = vm->Stack[vm->stacktop-1].us;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned short) <= gVM.globalsize);
	*(unsigned short *)(vm->Globals + offset) = value; 
	vm->Stack[vm->stacktop-2].us = value;
	vm->stacktop--;
	ScriptExecTrace("assign global variable at offset %u  value = %u  type = unsigned short\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_GLOBAL_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	int value = vm->Stack[vm->stacktop-1].i;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(int) <= gVM.globalsize);
	*(int *)(vm->Globals + offset) = value; 
	vm->Stack[vm->stacktop-2].i = value;
	vm->stacktop--;
	ScriptExecTrace("assign global variable at offset %u  value = %d  type = int\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_GLOBAL_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned int value = vm->Stack[vm->stacktop-1].ui;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned int) <= gVM.globalsize);
	*(unsigned int *)(vm->Globals + offset) = value; 
	vm->Stack[vm->stacktop-2].ui = value;
	vm->stacktop--;
	ScriptExecTrace("assign global variable at offset %u  value = %u  type = unsigned int\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_GLOBAL_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	INT64 value = vm->Stack[vm->stacktop-1].ll;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(INT64) <= gVM.globalsize);
	*(INT64 *)(vm->Globals + offset) = value; 
	vm->Stack[vm->stacktop-2].ll = value;
	vm->stacktop--;
	ScriptExecTrace("assign global variable at offset %u  value = %I64d  type = int64\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_GLOBAL_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	UINT64 value = vm->Stack[vm->stacktop-1].ull;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(UINT64) <= gVM.globalsize);
	*(UINT64 *)(vm->Globals + offset) = value; 
	vm->Stack[vm->stacktop-2].ull = value;
	vm->stacktop--;
	ScriptExecTrace("assign global variable at offset %u  value = %I64u  type = unsigned int64\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_GLOBAL_FL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	float value = vm->Stack[vm->stacktop-1].f;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(float) <= gVM.globalsize);
	*(float *)(vm->Globals + offset) = value; 
	vm->Stack[vm->stacktop-2].f = value;
	vm->stacktop--;
	ScriptExecTrace("assign global variable at offset %u  value = %f  type = float\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_GLOBAL_DB(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	double value = vm->Stack[vm->stacktop-1].d;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(double) <= gVM.globalsize);
	*(double *)(vm->Globals + offset) = value; 
	vm->Stack[vm->stacktop-2].d = value;
	vm->stacktop--;
	ScriptExecTrace("assign global variable at offset %u  value = %f  type = double\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_GLOBAL_LD(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	long double value = vm->Stack[vm->stacktop-1].ld;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(long double) <= gVM.globalsize);
	*(long double *)(vm->Globals + offset) = value; 
	vm->Stack[vm->stacktop-2].ld = value;
	vm->stacktop--;
	ScriptExecTrace("assign global variable at offset %u  value = %f  type = long double\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_GLOBAL_POINTER(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	void * value = vm->Stack[vm->stacktop-1].ptr;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(void *) <= gVM.globalsize);
	*(void **)(vm->Globals + offset) = value; 
	vm->Stack[vm->stacktop-2].ptr = value;
	vm->stacktop--;
	ScriptExecTrace("assign global variable at offset %u  value = %x  type = pointer\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_LOCAL_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	char value = vm->Stack[vm->stacktop-1].c;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(char) <= vm->localsize);
	*(char *)(vm->Locals + vm->localcur + offset) = value; 
	vm->Stack[vm->stacktop-2].c = value;
	vm->stacktop--;
	ScriptExecTrace("assign local variable at offset %u  value = %d  type = char\n", vm->localcur + offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_LOCAL_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned char value = vm->Stack[vm->stacktop-1].uc;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned char) <= vm->localsize);
	*(unsigned char *)(vm->Locals + vm->localcur + offset) = value; 
	vm->Stack[vm->stacktop-2].uc = value;
	vm->stacktop--;
	ScriptExecTrace("assign local variable at offset %u  value = %u  type = unsigned char\n", vm->localcur + offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_LOCAL_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	short value = vm->Stack[vm->stacktop-1].s;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(short) <= vm->localsize);
	*(short *)(vm->Locals + vm->localcur + offset) = value; 
	vm->Stack[vm->stacktop-2].s = value;
	vm->stacktop--;
	ScriptExecTrace("assign local variable at offset %u  value = %d  type = short\n", vm->localcur + offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_LOCAL_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned short value = vm->Stack[vm->stacktop-1].us;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned short) <= vm->localsize);
	*(unsigned short *)(vm->Locals + vm->localcur + offset) = value; 
	vm->Stack[vm->stacktop-2].us = value;
	vm->stacktop--;
	ScriptExecTrace("assign local variable at offset %u  value = %u  type = unsigned short\n", vm->localcur + offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_LOCAL_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	int value = vm->Stack[vm->stacktop-1].i;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(int) <= vm->localsize);
	*(int *)(vm->Locals + vm->localcur + offset) = value; 
	vm->Stack[vm->stacktop-2].i = value;
	vm->stacktop--;
	ScriptExecTrace("assign local variable at offset %u  value = %d  type = int\n", vm->localcur + offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_LOCAL_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned int value = vm->Stack[vm->stacktop-1].ui;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned int) <= vm->localsize);
	*(unsigned int *)(vm->Locals + vm->localcur + offset) = value; 
	vm->Stack[vm->stacktop-2].ui = value;
	vm->stacktop--;
	ScriptExecTrace("assign local variable at offset %u  value = %u  type = unsigned int\n", vm->localcur + offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_LOCAL_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	INT64 value = vm->Stack[vm->stacktop-1].ll;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(INT64) <= vm->localsize);
	*(INT64 *)(vm->Locals + vm->localcur + offset) = value; 
	vm->Stack[vm->stacktop-2].ll = value;
	vm->stacktop--;
	ScriptExecTrace("assign local variable at offset %u  value = %I64d  type = int64\n", vm->localcur + offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_LOCAL_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	UINT64 value = vm->Stack[vm->stacktop-1].ull;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(UINT64) <= vm->localsize);
	*(UINT64 *)(vm->Locals + vm->localcur + offset) = value; 
	vm->Stack[vm->stacktop-2].ull = value;
	vm->stacktop--;
	ScriptExecTrace("assign local variable at offset %u  value = %I64u  type = unsigned int64\n", vm->localcur + offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_LOCAL_FL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	float value = vm->Stack[vm->stacktop-1].f;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(float) <= vm->localsize);
	*(float *)(vm->Locals + vm->localcur + offset) = value; 
	vm->Stack[vm->stacktop-2].f = value;
	vm->stacktop--;
	ScriptExecTrace("assign local variable at offset %u  value = %f  type = float\n", vm->localcur + offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_LOCAL_DB(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	double value = vm->Stack[vm->stacktop-1].d;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(double) <= vm->localsize);
	*(double *)(vm->Locals + vm->localcur + offset) = value; 
	vm->Stack[vm->stacktop-2].d = value;
	vm->stacktop--;
	ScriptExecTrace("assign local variable at offset %u  value = %f  type = double\n", vm->localcur + offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_LOCAL_LD(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	long double value = vm->Stack[vm->stacktop-1].ld;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(long double) <= vm->localsize);
	*(long double *)(vm->Locals + vm->localcur + offset) = value; 
	vm->Stack[vm->stacktop-2].ld = value;
	vm->stacktop--;
	ScriptExecTrace("assign local variable at offset %u  value = %f  type = long double\n", vm->localcur + offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_LOCAL_POINTER(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	void * value = vm->Stack[vm->stacktop-1].ptr;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(void *) <= vm->localsize);
	*(void **)(vm->Locals + vm->localcur + offset) = value; 
	vm->Stack[vm->stacktop-2].ptr = value;
	vm->stacktop--;
	ScriptExecTrace("assign local variable at offset %u  value = %x  type = pointer\n", vm->localcur + offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_GLOBAL_CONST_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int offset;
	ASSERT_RETZERO(ByteStreamGetDword(&str, offset));
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(char) <= gVM.globalsize); 
	char value;
	ASSERT_RETZERO(ByteStreamGetChar(&str, value));
	*(char *)(vm->Globals + offset) = value;
	vm->Stack[vm->stacktop++].c = value;
	ScriptExecTrace("assign global variable at offset %u  value = %d  type = char\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_GLOBAL_CONST_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int offset;
	ASSERT_RETZERO(ByteStreamGetDword(&str, offset));
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned char) <= gVM.globalsize); 
	unsigned char value;
	ASSERT_RETZERO(ByteStreamGetByte(&str, value));
	*(unsigned char *)(vm->Globals + offset) = value;
	vm->Stack[vm->stacktop++].uc = value;
	ScriptExecTrace("assign global variable at offset %u  value = %u  type = unsigned char\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_GLOBAL_CONST_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int offset;
	ASSERT_RETZERO(ByteStreamGetDword(&str, offset));
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(short) <= gVM.globalsize); 
	short value;
	ASSERT_RETZERO(ByteStreamGetShort(&str, value));
	*(short *)(vm->Globals + offset) = value;
	vm->Stack[vm->stacktop++].s = value;
	ScriptExecTrace("assign global variable at offset %u  value = %d  type = short\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_GLOBAL_CONST_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int offset;
	ASSERT_RETZERO(ByteStreamGetDword(&str, offset));
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned short) <= gVM.globalsize); 
	unsigned short value;
	ASSERT_RETZERO(ByteStreamGetWord(&str, value));
	*(unsigned short *)(vm->Globals + offset) = value;
	vm->Stack[vm->stacktop++].us = value;
	ScriptExecTrace("assign global variable at offset %u  value = %u  type = unsigned short\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_GLOBAL_CONST_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int offset;
	ASSERT_RETZERO(ByteStreamGetDword(&str, offset));
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(int) <= gVM.globalsize); 
	int value;
	ASSERT_RETZERO(ByteStreamGetInt(&str, value));
	*(int *)(vm->Globals + offset) = value;
	vm->Stack[vm->stacktop++].i = value;
	ScriptExecTrace("assign global variable at offset %u  value = %d  type = int\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_GLOBAL_CONST_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int offset;
	ASSERT_RETZERO(ByteStreamGetDword(&str, offset));
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned int) <= gVM.globalsize); 
	unsigned int value;
	ASSERT_RETZERO(ByteStreamGetDword(&str, value));
	*(unsigned int *)(vm->Globals + offset) = value;
	vm->Stack[vm->stacktop++].ui = value;
	ScriptExecTrace("assign global variable at offset %u  value = %u  type = unsigned int\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_GLOBAL_CONST_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int offset;
	ASSERT_RETZERO(ByteStreamGetDword(&str, offset));
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(INT64) <= gVM.globalsize); 
	INT64 value;
	ASSERT_RETZERO(ByteStreamGetInt64(&str, value));
	*(INT64 *)(vm->Globals + offset) = value;
	vm->Stack[vm->stacktop++].ll = value;
	ScriptExecTrace("assign global variable at offset %u  value = %I64d  type = int64\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_GLOBAL_CONST_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int offset;
	ASSERT_RETZERO(ByteStreamGetDword(&str, offset));
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(UINT64) <= gVM.globalsize); 
	UINT64 value;
	ASSERT_RETZERO(ByteStreamGetUInt64(&str, value));
	*(UINT64 *)(vm->Globals + offset) = value;
	vm->Stack[vm->stacktop++].ull = value;
	ScriptExecTrace("assign global variable at offset %u  value = %I64u  type = unsigned int64\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_GLOBAL_CONST_FL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int offset;
	ASSERT_RETZERO(ByteStreamGetDword(&str, offset));
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(float) <= gVM.globalsize); 
	float value;
	ASSERT_RETZERO(ByteStreamGetFloat(&str, value));
	*(float *)(vm->Globals + offset) = value;
	vm->Stack[vm->stacktop++].f = value;
	ScriptExecTrace("assign global variable at offset %u  value = %f  type = float\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_GLOBAL_CONST_DB(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int offset;
	ASSERT_RETZERO(ByteStreamGetDword(&str, offset));
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(double) <= gVM.globalsize); 
	double value;
	ASSERT_RETZERO(ByteStreamGetDouble(&str, value));
	*(double *)(vm->Globals + offset) = value;
	vm->Stack[vm->stacktop++].d = value;
	ScriptExecTrace("assign global variable at offset %u  value = %f  type = double\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_GLOBAL_CONST_LD(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int offset;
	ASSERT_RETZERO(ByteStreamGetDword(&str, offset));
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(long double) <= gVM.globalsize); 
	long double value;
	ASSERT_RETZERO(ByteStreamGetLDouble(&str, value));
	*(long double *)(vm->Globals + offset) = value;
	vm->Stack[vm->stacktop++].ld = value;
	ScriptExecTrace("assign global variable at offset %u  value = %f  type = long double\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_GLOBAL_CONST_POINTER(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int offset;
	ASSERT_RETZERO(ByteStreamGetDword(&str, offset));
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(void *) <= gVM.globalsize); 
	VALUE value;
	ASSERT_RETZERO(ByteStreamGetValue(&str, &value));
	*(void **)(vm->Globals + offset) = value.ptr;
	vm->Stack[vm->stacktop++] = value;
	ScriptExecTrace("assign global variable at offset %u  value = %x  type = pointer\n", offset, value.ptr);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_LOCAL_CONST_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int offset;
	ASSERT_RETZERO(ByteStreamGetDword(&str, offset));
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(char) <= vm->localsize); 
	char value;
	ASSERT_RETZERO(ByteStreamGetChar(&str, value));
	*(char *)(vm->Locals + vm->localcur + offset) = value;
	vm->Stack[vm->stacktop++].c = value;
	ScriptExecTrace("assign local variable at offset %u  value = %d  type = char\n", vm->localcur + offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_LOCAL_CONST_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int offset;
	ASSERT_RETZERO(ByteStreamGetDword(&str, offset));
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned char) <= vm->localsize); 
	unsigned char value;
	ASSERT_RETZERO(ByteStreamGetByte(&str, value));
	*(unsigned char *)(vm->Locals + vm->localcur + offset) = value;
	vm->Stack[vm->stacktop++].uc = value;
	ScriptExecTrace("assign local variable at offset %u  value = %u  type = unsigned char\n", vm->localcur + offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_LOCAL_CONST_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int offset;
	ASSERT_RETZERO(ByteStreamGetDword(&str, offset));
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(short) <= vm->localsize); 
	short value;
	ASSERT_RETZERO(ByteStreamGetShort(&str, value));
	*(short *)(vm->Locals + vm->localcur + offset) = value;
	vm->Stack[vm->stacktop++].s = value;
	ScriptExecTrace("assign local variable at offset %u  value = %d  type = short\n", vm->localcur + offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_LOCAL_CONST_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int offset;
	ASSERT_RETZERO(ByteStreamGetDword(&str, offset));
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned short) <= vm->localsize); 
	unsigned short value;
	ASSERT_RETZERO(ByteStreamGetWord(&str, value));
	*(unsigned short *)(vm->Locals + vm->localcur + offset) = value;
	vm->Stack[vm->stacktop++].us = value;
	ScriptExecTrace("assign local variable at offset %u  value = %u  type = unsigned short\n", vm->localcur + offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_LOCAL_CONST_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int offset;
	ASSERT_RETZERO(ByteStreamGetDword(&str, offset));
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(int) <= vm->localsize); 
	int value;
	ASSERT_RETZERO(ByteStreamGetInt(&str, value));
	*(int *)(vm->Locals + vm->localcur + offset) = value;
	vm->Stack[vm->stacktop++].i = value;
	ScriptExecTrace("assign local variable at offset %u  value = %d  type = int\n", vm->localcur + offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_LOCAL_CONST_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int offset;
	ASSERT_RETZERO(ByteStreamGetDword(&str, offset));
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned int) <= vm->localsize); 
	unsigned int value;
	ASSERT_RETZERO(ByteStreamGetDword(&str, value));
	*(unsigned int *)(vm->Locals + vm->localcur + offset) = value;
	vm->Stack[vm->stacktop++].ui = value;
	ScriptExecTrace("assign local variable at offset %u  value = %u  type = unsigned int\n", vm->localcur + offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_LOCAL_CONST_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int offset;
	ASSERT_RETZERO(ByteStreamGetDword(&str, offset));
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(INT64) <= vm->localsize); 
	INT64 value;
	ASSERT_RETZERO(ByteStreamGetInt64(&str, value));
	*(INT64 *)(vm->Locals + vm->localcur + offset) = value;
	vm->Stack[vm->stacktop++].ll = value;
	ScriptExecTrace("assign local variable at offset %u  value = %I64d  type = int64\n", vm->localcur + offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_LOCAL_CONST_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int offset;
	ASSERT_RETZERO(ByteStreamGetDword(&str, offset));
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(UINT64) <= vm->localsize); 
	UINT64 value;
	ASSERT_RETZERO(ByteStreamGetUInt64(&str, value));
	*(UINT64 *)(vm->Locals + vm->localcur + offset) = value;
	vm->Stack[vm->stacktop++].ull = value;
	ScriptExecTrace("assign local variable at offset %u  value = %I64u  type = unsigned int64\n", vm->localcur + offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_LOCAL_CONST_FL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int offset;
	ASSERT_RETZERO(ByteStreamGetDword(&str, offset));
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(float) <= vm->localsize); 
	float value;
	ASSERT_RETZERO(ByteStreamGetFloat(&str, value));
	*(float *)(vm->Locals + vm->localcur + offset) = value;
	vm->Stack[vm->stacktop++].f = value;
	ScriptExecTrace("assign local variable at offset %u  value = %f  type = float\n", vm->localcur + offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_LOCAL_CONST_DB(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int offset;
	ASSERT_RETZERO(ByteStreamGetDword(&str, offset));
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(double) <= vm->localsize); 
	double value;
	ASSERT_RETZERO(ByteStreamGetDouble(&str, value));
	*(double *)(vm->Locals + vm->localcur + offset) = value;
	vm->Stack[vm->stacktop++].d = value;
	ScriptExecTrace("assign local variable at offset %u  value = %f  type = double\n", vm->localcur + offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_LOCAL_CONST_LD(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int offset;
	ASSERT_RETZERO(ByteStreamGetDword(&str, offset));
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(long double) <= vm->localsize); 
	long double value;
	ASSERT_RETZERO(ByteStreamGetLDouble(&str, value));
	*(long double *)(vm->Locals + vm->localcur + offset) = value;
	vm->Stack[vm->stacktop++].ld = value;
	ScriptExecTrace("assign local variable at offset %u  value = %f  type = long double\n", vm->localcur + offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_LOCAL_CONST_POINTER(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int offset;
	ASSERT_RETZERO(ByteStreamGetDword(&str, offset));
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(void *) <= vm->localsize); 
	VALUE value;
	ASSERT_RETZERO(ByteStreamGetValue(&str, &value));
	*(void **)(vm->Locals + vm->localcur + offset) = value.ptr;
	vm->Stack[vm->stacktop++] = value;
	ScriptExecTrace("assign local variable at offset %u  value = %x  type = pointer\n", vm->localcur + offset, value.ptr);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_GLOBAL_MEM(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int offset;
	ASSERT_RETZERO(ByteStreamGetDword(&str, offset));
	unsigned int size;
	ASSERT_RETZERO(ByteStreamGetDword(&str, size));
	ASSERT_RETZERO(offset + size <= gVM.globalcur);
	ASSERT_RETZERO(ByteStreamGet(&str, vm->Globals + offset, size));
	ScriptExecTrace("initialize global variable at offset %u  bytes %u\n", offset, size);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_LOCAL_MEM(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int offset;
	ASSERT_RETZERO(ByteStreamGetDword(&str, offset));
	unsigned int size;
	ASSERT_RETZERO(ByteStreamGetDword(&str, size));
	ASSERT_RETZERO(vm->localcur + offset + size <= vm->localsize);
	ASSERT_RETZERO(ByteStreamGet(&str, vm->Locals + vm->localcur + offset, size));
	ScriptExecTrace("initialize global variable at offset %u  bytes %u\n", vm->localcur + offset, size);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_MUL_GLOBAL_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	char value = vm->Stack[vm->stacktop-1].c;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned char) < gVM.globalsize);
	char * ptr = (char *)(vm->Globals + offset);
	value = (char)(*ptr * value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].uc = value;
	vm->stacktop--;
	ScriptExecTrace("*= global variable at offset %u  value = %u  type = unsigned char\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_MUL_GLOBAL_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned char value = vm->Stack[vm->stacktop-1].uc;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned char) < gVM.globalsize);
	unsigned char * ptr = (unsigned char *)(vm->Globals + offset);
	value = (unsigned char)(*ptr * value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].uc = value;
	vm->stacktop--;
	ScriptExecTrace("*= global variable at offset %u  value = %u  type = unsigned char\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_MUL_GLOBAL_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	short value = vm->Stack[vm->stacktop-1].s;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(short) < gVM.globalsize);
	short * ptr = (short *)(vm->Globals + offset);
	value = (short)(*ptr * value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].s = value;
	vm->stacktop--;
	ScriptExecTrace("*= global variable at offset %u  value = %d  type = short\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_MUL_GLOBAL_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned short value = vm->Stack[vm->stacktop-1].us;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned short) < gVM.globalsize);
	unsigned short * ptr = (unsigned short *)(vm->Globals + offset);
	value = (unsigned short)(*ptr * value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].us = value;
	vm->stacktop--;
	ScriptExecTrace("*= global variable at offset %u  value = %u  type = unsigned short\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_MUL_GLOBAL_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	int value = vm->Stack[vm->stacktop-1].i;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(int) < gVM.globalsize);
	int * ptr = (int *)(vm->Globals + offset);
	value = (int)(*ptr * value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].i = value;
	vm->stacktop--;
	ScriptExecTrace("*= global variable at offset %u  value = %d  type = int\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_MUL_GLOBAL_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned int value = vm->Stack[vm->stacktop-1].ui;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned int) < gVM.globalsize);
	unsigned int * ptr = (unsigned int *)(vm->Globals + offset);
	value = (unsigned int)(*ptr * value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ui = value;
	vm->stacktop--;
	ScriptExecTrace("*= global variable at offset %u  value = %u  type = unsigned int\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_MUL_GLOBAL_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	INT64 value = vm->Stack[vm->stacktop-1].ll;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(INT64) < gVM.globalsize);
	INT64* ptr = (INT64 *)(vm->Globals + offset);
	value = (INT64)(*ptr * value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ll = value;
	vm->stacktop--;
	ScriptExecTrace("*= global variable at offset %u  value = %I64d  type = int64\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_MUL_GLOBAL_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	UINT64 value = vm->Stack[vm->stacktop-1].ull;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(UINT64) < gVM.globalsize);
	UINT64 * ptr = (UINT64 *)(vm->Globals + offset);
	value = (UINT64)(*ptr * value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ull = value;
	vm->stacktop--;
	ScriptExecTrace("*= global variable at offset %u  value = %I64u  type = unsigned int64\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_MUL_GLOBAL_FL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	float value = vm->Stack[vm->stacktop-1].f;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(float) < gVM.globalsize);
	float * ptr = (float *)(vm->Globals + offset);
	value = (float)(*ptr * value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].f = value;
	vm->stacktop--;
	ScriptExecTrace("*= global variable at offset %u  value = %f  type = float\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_MUL_GLOBAL_DB(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	double value = vm->Stack[vm->stacktop-1].d;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(double) < gVM.globalsize);
	double * ptr = (double *)(vm->Globals + offset);
	value = (double)(*ptr * value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].d = value;
	vm->stacktop--;
	ScriptExecTrace("*= global variable at offset %u  value = %f  type = double\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_MUL_GLOBAL_LD(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	long double value = vm->Stack[vm->stacktop-1].ld;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(long double) < gVM.globalsize);
	long double * ptr = (long double *)(vm->Globals + offset);
	value = (long double)(*ptr * value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ld = value;
	vm->stacktop--;
	ScriptExecTrace("*= global variable at offset %u  value = %f  type = long double\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_MUL_LOCAL_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	char value = vm->Stack[vm->stacktop-1].c;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned char) < vm->localsize);
	char * ptr = (char *)(vm->Locals + offset);
	value = (char)(*ptr * value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].uc = value;
	vm->stacktop--;
	ScriptExecTrace("*= local variable at offset %u  value = %u  type = unsigned char\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_MUL_LOCAL_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned char value = vm->Stack[vm->stacktop-1].uc;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned char) < vm->localsize);
	unsigned char * ptr = (unsigned char *)(vm->Locals + offset);
	value = (unsigned char)(*ptr * value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].uc = value;
	vm->stacktop--;
	ScriptExecTrace("*= local variable at offset %u  value = %u  type = unsigned char\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_MUL_LOCAL_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	short value = vm->Stack[vm->stacktop-1].s;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(short) < vm->localsize);
	short * ptr = (short *)(vm->Locals + offset);
	value = (short)(*ptr * value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].s = value;
	vm->stacktop--;
	ScriptExecTrace("*= local variable at offset %u  value = %d  type = short\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_MUL_LOCAL_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned short value = vm->Stack[vm->stacktop-1].us;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned short) < vm->localsize);
	unsigned short * ptr = (unsigned short *)(vm->Locals + offset);
	value = (unsigned short)(*ptr * value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].us = value;
	vm->stacktop--;
	ScriptExecTrace("*= local variable at offset %u  value = %u  type = unsigned short\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_MUL_LOCAL_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	int value = vm->Stack[vm->stacktop-1].i;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(int) < vm->localsize);
	int * ptr = (int *)(vm->Locals + offset);
	value = (int)(*ptr * value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].i = value;
	vm->stacktop--;
	ScriptExecTrace("*= local variable at offset %u  value = %d  type = int\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_MUL_LOCAL_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned int value = vm->Stack[vm->stacktop-1].ui;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned int) < vm->localsize);
	unsigned int * ptr = (unsigned int *)(vm->Locals + offset);
	value = (unsigned int)(*ptr * value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ui = value;
	vm->stacktop--;
	ScriptExecTrace("*= local variable at offset %u  value = %u  type = unsigned int\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_MUL_LOCAL_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	INT64 value = vm->Stack[vm->stacktop-1].ll;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(INT64) < vm->localsize);
	INT64* ptr = (INT64 *)(vm->Locals + offset);
	value = (INT64)(*ptr * value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ll = value;
	vm->stacktop--;
	ScriptExecTrace("*= local variable at offset %u  value = %I64d  type = int64\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_MUL_LOCAL_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	UINT64 value = vm->Stack[vm->stacktop-1].ull;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(UINT64) < vm->localsize);
	UINT64 * ptr = (UINT64 *)(vm->Locals + offset);
	value = (UINT64)(*ptr * value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ull = value;
	vm->stacktop--;
	ScriptExecTrace("*= local variable at offset %u  value = %I64u  type = unsigned int64\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_MUL_LOCAL_FL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	float value = vm->Stack[vm->stacktop-1].f;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(float) < vm->localsize);
	float * ptr = (float *)(vm->Locals + offset);
	value = (float)(*ptr * value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].f = value;
	vm->stacktop--;
	ScriptExecTrace("*= local variable at offset %u  value = %f  type = float\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_MUL_LOCAL_DB(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	double value = vm->Stack[vm->stacktop-1].d;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(double) < vm->localsize);
	double * ptr = (double *)(vm->Locals + offset);
	value = (double)(*ptr * value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].d = value;
	vm->stacktop--;
	ScriptExecTrace("*= local variable at offset %u  value = %f  type = double\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_MUL_LOCAL_LD(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	long double value = vm->Stack[vm->stacktop-1].ld;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(long double) < vm->localsize);
	long double * ptr = (long double *)(vm->Locals + offset);
	value = (long double)(*ptr * value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ld = value;
	vm->stacktop--;
	ScriptExecTrace("*= local variable at offset %u  value = %f  type = long double\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_DIV_GLOBAL_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	char value = vm->Stack[vm->stacktop-1].c;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned char) < gVM.globalsize);
	char * ptr = (char *)(vm->Globals + offset);
	value = (char)(*ptr / value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].uc = value;
	vm->stacktop--;
	ScriptExecTrace("/= global variable at offset %u  value = %u  type = unsigned char\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_DIV_GLOBAL_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned char value = vm->Stack[vm->stacktop-1].uc;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned char) < gVM.globalsize);
	unsigned char * ptr = (unsigned char *)(vm->Globals + offset);
	value = (unsigned char)(*ptr / value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].uc = value;
	vm->stacktop--;
	ScriptExecTrace("/= global variable at offset %u  value = %u  type = unsigned char\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_DIV_GLOBAL_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	short value = vm->Stack[vm->stacktop-1].s;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(short) < gVM.globalsize);
	short * ptr = (short *)(vm->Globals + offset);
	value = (short)(*ptr / value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].s = value;
	vm->stacktop--;
	ScriptExecTrace("/= global variable at offset %u  value = %d  type = short\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_DIV_GLOBAL_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned short value = vm->Stack[vm->stacktop-1].us;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned short) < gVM.globalsize);
	unsigned short * ptr = (unsigned short *)(vm->Globals + offset);
	value = (unsigned short)(*ptr / value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].us = value;
	vm->stacktop--;
	ScriptExecTrace("/= global variable at offset %u  value = %u  type = unsigned short\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_DIV_GLOBAL_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	int value = vm->Stack[vm->stacktop-1].i;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(int) < gVM.globalsize);
	int * ptr = (int *)(vm->Globals + offset);
	value = (int)(*ptr / value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].i = value;
	vm->stacktop--;
	ScriptExecTrace("/= global variable at offset %u  value = %d  type = int\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_DIV_GLOBAL_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned int value = vm->Stack[vm->stacktop-1].ui;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned int) < gVM.globalsize);
	unsigned int * ptr = (unsigned int *)(vm->Globals + offset);
	value = (unsigned int)(*ptr / value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ui = value;
	vm->stacktop--;
	ScriptExecTrace("/= global variable at offset %u  value = %u  type = unsigned int\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_DIV_GLOBAL_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	INT64 value = vm->Stack[vm->stacktop-1].ll;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(INT64) < gVM.globalsize);
	INT64* ptr = (INT64 *)(vm->Globals + offset);
	value = (INT64)(*ptr / value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ll = value;
	vm->stacktop--;
	ScriptExecTrace("/= global variable at offset %u  value = %I64d  type = int64\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_DIV_GLOBAL_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	UINT64 value = vm->Stack[vm->stacktop-1].ull;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(UINT64) < gVM.globalsize);
	UINT64 * ptr = (UINT64 *)(vm->Globals + offset);
	value = (UINT64)(*ptr / value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ull = value;
	vm->stacktop--;
	ScriptExecTrace("/= global variable at offset %u  value = %I64u  type = unsigned int64\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_DIV_GLOBAL_FL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	float value = vm->Stack[vm->stacktop-1].f;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(float) < gVM.globalsize);
	float * ptr = (float *)(vm->Globals + offset);
	value = (float)(*ptr / value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].f = value;
	vm->stacktop--;
	ScriptExecTrace("/= global variable at offset %u  value = %f  type = float\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_DIV_GLOBAL_DB(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	double value = vm->Stack[vm->stacktop-1].d;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(double) < gVM.globalsize);
	double * ptr = (double *)(vm->Globals + offset);
	value = (double)(*ptr / value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].d = value;
	vm->stacktop--;
	ScriptExecTrace("/= global variable at offset %u  value = %f  type = double\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_DIV_GLOBAL_LD(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	long double value = vm->Stack[vm->stacktop-1].ld;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(long double) < gVM.globalsize);
	long double * ptr = (long double *)(vm->Globals + offset);
	value = (long double)(*ptr / value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ld = value;
	vm->stacktop--;
	ScriptExecTrace("/= global variable at offset %u  value = %f  type = long double\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_DIV_LOCAL_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	char value = vm->Stack[vm->stacktop-1].c;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned char) < vm->localsize);
	char * ptr = (char *)(vm->Locals + offset);
	value = (char)(*ptr / value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].uc = value;
	vm->stacktop--;
	ScriptExecTrace("/= local variable at offset %u  value = %u  type = unsigned char\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_DIV_LOCAL_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned char value = vm->Stack[vm->stacktop-1].uc;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned char) < vm->localsize);
	unsigned char * ptr = (unsigned char *)(vm->Locals + offset);
	value = (unsigned char)(*ptr / value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].uc = value;
	vm->stacktop--;
	ScriptExecTrace("/= local variable at offset %u  value = %u  type = unsigned char\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_DIV_LOCAL_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	short value = vm->Stack[vm->stacktop-1].s;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(short) < vm->localsize);
	short * ptr = (short *)(vm->Locals + offset);
	value = (short)(*ptr / value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].s = value;
	vm->stacktop--;
	ScriptExecTrace("/= local variable at offset %u  value = %d  type = short\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_DIV_LOCAL_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned short value = vm->Stack[vm->stacktop-1].us;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned short) < vm->localsize);
	unsigned short * ptr = (unsigned short *)(vm->Locals + offset);
	value = (unsigned short)(*ptr / value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].us = value;
	vm->stacktop--;
	ScriptExecTrace("/= local variable at offset %u  value = %u  type = unsigned short\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_DIV_LOCAL_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	int value = vm->Stack[vm->stacktop-1].i;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(int) < vm->localsize);
	int * ptr = (int *)(vm->Locals + offset);
	value = (int)(*ptr / value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].i = value;
	vm->stacktop--;
	ScriptExecTrace("/= local variable at offset %u  value = %d  type = int\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_DIV_LOCAL_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned int value = vm->Stack[vm->stacktop-1].ui;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned int) < vm->localsize);
	unsigned int * ptr = (unsigned int *)(vm->Locals + offset);
	value = (unsigned int)(*ptr / value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ui = value;
	vm->stacktop--;
	ScriptExecTrace("/= local variable at offset %u  value = %u  type = unsigned int\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_DIV_LOCAL_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	INT64 value = vm->Stack[vm->stacktop-1].ll;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(INT64) < vm->localsize);
	INT64* ptr = (INT64 *)(vm->Locals + offset);
	value = (INT64)(*ptr / value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ll = value;
	vm->stacktop--;
	ScriptExecTrace("/= local variable at offset %u  value = %I64d  type = int64\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_DIV_LOCAL_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	UINT64 value = vm->Stack[vm->stacktop-1].ull;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(UINT64) < vm->localsize);
	UINT64 * ptr = (UINT64 *)(vm->Locals + offset);
	value = (UINT64)(*ptr / value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ull = value;
	vm->stacktop--;
	ScriptExecTrace("/= local variable at offset %u  value = %I64u  type = unsigned int64\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_DIV_LOCAL_FL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	float value = vm->Stack[vm->stacktop-1].f;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(float) < vm->localsize);
	float * ptr = (float *)(vm->Locals + offset);
	value = (float)(*ptr / value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].f = value;
	vm->stacktop--;
	ScriptExecTrace("/= local variable at offset %u  value = %f  type = float\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_DIV_LOCAL_DB(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	double value = vm->Stack[vm->stacktop-1].d;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(double) < vm->localsize);
	double * ptr = (double *)(vm->Locals + offset);
	value = (double)(*ptr / value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].d = value;
	vm->stacktop--;
	ScriptExecTrace("/= local variable at offset %u  value = %f  type = double\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_DIV_LOCAL_LD(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	long double value = vm->Stack[vm->stacktop-1].ld;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(long double) < vm->localsize);
	long double * ptr = (long double *)(vm->Locals + offset);
	value = (long double)(*ptr / value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ld = value;
	vm->stacktop--;
	ScriptExecTrace("/= local variable at offset %u  value = %f  type = long double\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_MOD_GLOBAL_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	char value = vm->Stack[vm->stacktop-1].c;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned char) < gVM.globalsize);
	char * ptr = (char *)(vm->Globals + offset);
	value = (char)(*ptr % value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].uc = value;
	vm->stacktop--;
	ScriptExecTrace("%= global variable at offset %u  value = %u  type = unsigned char\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_MOD_GLOBAL_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned char value = vm->Stack[vm->stacktop-1].uc;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned char) < gVM.globalsize);
	unsigned char * ptr = (unsigned char *)(vm->Globals + offset);
	value = (unsigned char)(*ptr % value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].uc = value;
	vm->stacktop--;
	ScriptExecTrace("%= global variable at offset %u  value = %u  type = unsigned char\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_MOD_GLOBAL_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	short value = vm->Stack[vm->stacktop-1].s;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(short) < gVM.globalsize);
	short * ptr = (short *)(vm->Globals + offset);
	value = (short)(*ptr % value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].s = value;
	vm->stacktop--;
	ScriptExecTrace("%= global variable at offset %u  value = %d  type = short\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_MOD_GLOBAL_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned short value = vm->Stack[vm->stacktop-1].us;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned short) < gVM.globalsize);
	unsigned short * ptr = (unsigned short *)(vm->Globals + offset);
	value = (unsigned short)(*ptr % value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].us = value;
	vm->stacktop--;
	ScriptExecTrace("%= global variable at offset %u  value = %u  type = unsigned short\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_MOD_GLOBAL_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	int value = vm->Stack[vm->stacktop-1].i;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(int) < gVM.globalsize);
	int * ptr = (int *)(vm->Globals + offset);
	value = (int)(*ptr % value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].i = value;
	vm->stacktop--;
	ScriptExecTrace("%= global variable at offset %u  value = %d  type = int\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_MOD_GLOBAL_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned int value = vm->Stack[vm->stacktop-1].ui;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned int) < gVM.globalsize);
	unsigned int * ptr = (unsigned int *)(vm->Globals + offset);
	value = (unsigned int)(*ptr % value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ui = value;
	vm->stacktop--;
	ScriptExecTrace("%= global variable at offset %u  value = %u  type = unsigned int\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_MOD_GLOBAL_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	INT64 value = vm->Stack[vm->stacktop-1].ll;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(INT64) < gVM.globalsize);
	INT64* ptr = (INT64 *)(vm->Globals + offset);
	value = (INT64)(*ptr % value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ll = value;
	vm->stacktop--;
	ScriptExecTrace("%= global variable at offset %u  value = %I64d  type = int64\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_MOD_GLOBAL_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	UINT64 value = vm->Stack[vm->stacktop-1].ull;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(UINT64) < gVM.globalsize);
	UINT64 * ptr = (UINT64 *)(vm->Globals + offset);
	value = (UINT64)(*ptr % value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ull = value;
	vm->stacktop--;
	ScriptExecTrace("%= global variable at offset %u  value = %I64u  type = unsigned int64\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_MOD_LOCAL_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	char value = vm->Stack[vm->stacktop-1].c;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned char) < vm->localsize);
	char * ptr = (char *)(vm->Locals + offset);
	value = (char)(*ptr % value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].uc = value;
	vm->stacktop--;
	ScriptExecTrace("%= local variable at offset %u  value = %u  type = unsigned char\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_MOD_LOCAL_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned char value = vm->Stack[vm->stacktop-1].uc;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned char) < vm->localsize);
	unsigned char * ptr = (unsigned char *)(vm->Locals + offset);
	value = (unsigned char)(*ptr % value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].uc = value;
	vm->stacktop--;
	ScriptExecTrace("%= local variable at offset %u  value = %u  type = unsigned char\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_MOD_LOCAL_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	short value = vm->Stack[vm->stacktop-1].s;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(short) < vm->localsize);
	short * ptr = (short *)(vm->Locals + offset);
	value = (short)(*ptr % value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].s = value;
	vm->stacktop--;
	ScriptExecTrace("%= local variable at offset %u  value = %d  type = short\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_MOD_LOCAL_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned short value = vm->Stack[vm->stacktop-1].us;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned short) < vm->localsize);
	unsigned short * ptr = (unsigned short *)(vm->Locals + offset);
	value = (unsigned short)(*ptr % value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].us = value;
	vm->stacktop--;
	ScriptExecTrace("%= local variable at offset %u  value = %u  type = unsigned short\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_MOD_LOCAL_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	int value = vm->Stack[vm->stacktop-1].i;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(int) < vm->localsize);
	int * ptr = (int *)(vm->Locals + offset);
	value = (int)(*ptr % value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].i = value;
	vm->stacktop--;
	ScriptExecTrace("%= local variable at offset %u  value = %d  type = int\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_MOD_LOCAL_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned int value = vm->Stack[vm->stacktop-1].ui;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned int) < vm->localsize);
	unsigned int * ptr = (unsigned int *)(vm->Locals + offset);
	value = (unsigned int)(*ptr % value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ui = value;
	vm->stacktop--;
	ScriptExecTrace("%= local variable at offset %u  value = %u  type = unsigned int\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_MOD_LOCAL_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	INT64 value = vm->Stack[vm->stacktop-1].ll;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(INT64) < vm->localsize);
	INT64* ptr = (INT64 *)(vm->Locals + offset);
	value = (INT64)(*ptr % value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ll = value;
	vm->stacktop--;
	ScriptExecTrace("%= local variable at offset %u  value = %I64d  type = int64\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_MOD_LOCAL_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	UINT64 value = vm->Stack[vm->stacktop-1].ull;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(UINT64) < vm->localsize);
	UINT64 * ptr = (UINT64 *)(vm->Locals + offset);
	value = (UINT64)(*ptr % value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ull = value;
	vm->stacktop--;
	ScriptExecTrace("%= local variable at offset %u  value = %I64u  type = unsigned int64\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_ADD_GLOBAL_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	char value = vm->Stack[vm->stacktop-1].c;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned char) < gVM.globalsize);
	char * ptr = (char *)(vm->Globals + offset);
	value = (char)(*ptr + value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].uc = value;
	vm->stacktop--;
	ScriptExecTrace("+= global variable at offset %u  value = %u  type = unsigned char\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_ADD_GLOBAL_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned char value = vm->Stack[vm->stacktop-1].uc;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned char) < gVM.globalsize);
	unsigned char * ptr = (unsigned char *)(vm->Globals + offset);
	value = (unsigned char)(*ptr + value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].uc = value;
	vm->stacktop--;
	ScriptExecTrace("+= global variable at offset %u  value = %u  type = unsigned char\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_ADD_GLOBAL_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	short value = vm->Stack[vm->stacktop-1].s;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(short) < gVM.globalsize);
	short * ptr = (short *)(vm->Globals + offset);
	value = (short)(*ptr + value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].s = value;
	vm->stacktop--;
	ScriptExecTrace("+= global variable at offset %u  value = %d  type = short\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_ADD_GLOBAL_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned short value = vm->Stack[vm->stacktop-1].us;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned short) < gVM.globalsize);
	unsigned short * ptr = (unsigned short *)(vm->Globals + offset);
	value = (unsigned short)(*ptr + value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].us = value;
	vm->stacktop--;
	ScriptExecTrace("+= global variable at offset %u  value = %u  type = unsigned short\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_ADD_GLOBAL_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	int value = vm->Stack[vm->stacktop-1].i;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(int) < gVM.globalsize);
	int * ptr = (int *)(vm->Globals + offset);
	value = (int)(*ptr + value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].i = value;
	vm->stacktop--;
	ScriptExecTrace("+= global variable at offset %u  value = %d  type = int\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_ADD_GLOBAL_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned int value = vm->Stack[vm->stacktop-1].ui;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned int) < gVM.globalsize);
	unsigned int * ptr = (unsigned int *)(vm->Globals + offset);
	value = (unsigned int)(*ptr + value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ui = value;
	vm->stacktop--;
	ScriptExecTrace("+= global variable at offset %u  value = %u  type = unsigned int\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_ADD_GLOBAL_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	INT64 value = vm->Stack[vm->stacktop-1].ll;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(INT64) < gVM.globalsize);
	INT64* ptr = (INT64 *)(vm->Globals + offset);
	value = (INT64)(*ptr + value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ll = value;
	vm->stacktop--;
	ScriptExecTrace("+= global variable at offset %u  value = %I64d  type = int64\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_ADD_GLOBAL_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	UINT64 value = vm->Stack[vm->stacktop-1].ull;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(UINT64) < gVM.globalsize);
	UINT64 * ptr = (UINT64 *)(vm->Globals + offset);
	value = (UINT64)(*ptr + value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ull = value;
	vm->stacktop--;
	ScriptExecTrace("+= global variable at offset %u  value = %I64u  type = unsigned int64\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_ADD_GLOBAL_FL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	float value = vm->Stack[vm->stacktop-1].f;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(float) < gVM.globalsize);
	float * ptr = (float *)(vm->Globals + offset);
	value = (float)(*ptr + value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].f = value;
	vm->stacktop--;
	ScriptExecTrace("+= global variable at offset %u  value = %f  type = float\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_ADD_GLOBAL_DB(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	double value = vm->Stack[vm->stacktop-1].d;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(double) < gVM.globalsize);
	double * ptr = (double *)(vm->Globals + offset);
	value = (double)(*ptr + value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].d = value;
	vm->stacktop--;
	ScriptExecTrace("+= global variable at offset %u  value = %f  type = double\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_ADD_GLOBAL_LD(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	long double value = vm->Stack[vm->stacktop-1].ld;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(long double) < gVM.globalsize);
	long double * ptr = (long double *)(vm->Globals + offset);
	value = (long double)(*ptr + value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ld = value;
	vm->stacktop--;
	ScriptExecTrace("+= global variable at offset %u  value = %f  type = long double\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_ADD_LOCAL_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	char value = vm->Stack[vm->stacktop-1].c;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned char) < vm->localsize);
	char * ptr = (char *)(vm->Locals + offset);
	value = (char)(*ptr + value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].uc = value;
	vm->stacktop--;
	ScriptExecTrace("+= local variable at offset %u  value = %u  type = unsigned char\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_ADD_LOCAL_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned char value = vm->Stack[vm->stacktop-1].uc;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned char) < vm->localsize);
	unsigned char * ptr = (unsigned char *)(vm->Locals + offset);
	value = (unsigned char)(*ptr + value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].uc = value;
	vm->stacktop--;
	ScriptExecTrace("+= local variable at offset %u  value = %u  type = unsigned char\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_ADD_LOCAL_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	short value = vm->Stack[vm->stacktop-1].s;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(short) < vm->localsize);
	short * ptr = (short *)(vm->Locals + offset);
	value = (short)(*ptr + value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].s = value;
	vm->stacktop--;
	ScriptExecTrace("+= local variable at offset %u  value = %d  type = short\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_ADD_LOCAL_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned short value = vm->Stack[vm->stacktop-1].us;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned short) < vm->localsize);
	unsigned short * ptr = (unsigned short *)(vm->Locals + offset);
	value = (unsigned short)(*ptr + value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].us = value;
	vm->stacktop--;
	ScriptExecTrace("+= local variable at offset %u  value = %u  type = unsigned short\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_ADD_LOCAL_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	int value = vm->Stack[vm->stacktop-1].i;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(int) < vm->localsize);
	int * ptr = (int *)(vm->Locals + offset);
	value = (int)(*ptr + value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].i = value;
	vm->stacktop--;
	ScriptExecTrace("+= local variable at offset %u  value = %d  type = int\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_ADD_LOCAL_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned int value = vm->Stack[vm->stacktop-1].ui;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned int) < vm->localsize);
	unsigned int * ptr = (unsigned int *)(vm->Locals + offset);
	value = (unsigned int)(*ptr + value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ui = value;
	vm->stacktop--;
	ScriptExecTrace("+= local variable at offset %u  value = %u  type = unsigned int\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_ADD_LOCAL_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	INT64 value = vm->Stack[vm->stacktop-1].ll;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(INT64) < vm->localsize);
	INT64* ptr = (INT64 *)(vm->Locals + offset);
	value = (INT64)(*ptr + value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ll = value;
	vm->stacktop--;
	ScriptExecTrace("+= local variable at offset %u  value = %I64d  type = int64\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_ADD_LOCAL_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	UINT64 value = vm->Stack[vm->stacktop-1].ull;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(UINT64) < vm->localsize);
	UINT64 * ptr = (UINT64 *)(vm->Locals + offset);
	value = (UINT64)(*ptr + value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ull = value;
	vm->stacktop--;
	ScriptExecTrace("+= local variable at offset %u  value = %I64u  type = unsigned int64\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_ADD_LOCAL_FL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	float value = vm->Stack[vm->stacktop-1].f;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(float) < vm->localsize);
	float * ptr = (float *)(vm->Locals + offset);
	value = (float)(*ptr + value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].f = value;
	vm->stacktop--;
	ScriptExecTrace("+= local variable at offset %u  value = %f  type = float\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_ADD_LOCAL_DB(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	double value = vm->Stack[vm->stacktop-1].d;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(double) < vm->localsize);
	double * ptr = (double *)(vm->Locals + offset);
	value = (double)(*ptr + value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].d = value;
	vm->stacktop--;
	ScriptExecTrace("+= local variable at offset %u  value = %f  type = double\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_ADD_LOCAL_LD(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	long double value = vm->Stack[vm->stacktop-1].ld;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(long double) < vm->localsize);
	long double * ptr = (long double *)(vm->Locals + offset);
	value = (long double)(*ptr + value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ld = value;
	vm->stacktop--;
	ScriptExecTrace("+= local variable at offset %u  value = %f  type = long double\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_SUB_GLOBAL_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	char value = vm->Stack[vm->stacktop-1].c;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned char) < gVM.globalsize);
	char * ptr = (char *)(vm->Globals + offset);
	value = (char)(*ptr - value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].uc = value;
	vm->stacktop--;
	ScriptExecTrace("-= global variable at offset %u  value = %u  type = unsigned char\n", offset, value);
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_SUB_GLOBAL_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned char value = vm->Stack[vm->stacktop-1].uc;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned char) < gVM.globalsize);
	unsigned char * ptr = (unsigned char *)(vm->Globals + offset);
	value = (unsigned char)(*ptr - value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].uc = value;
	vm->stacktop--;
	ScriptExecTrace("-= global variable at offset %u  value = %u  type = unsigned char\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_SUB_GLOBAL_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	short value = vm->Stack[vm->stacktop-1].s;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(short) < gVM.globalsize);
	short * ptr = (short *)(vm->Globals + offset);
	value = (short)(*ptr - value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].s = value;
	vm->stacktop--;
	ScriptExecTrace("-= global variable at offset %u  value = %d  type = short\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_SUB_GLOBAL_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned short value = vm->Stack[vm->stacktop-1].us;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned short) < gVM.globalsize);
	unsigned short * ptr = (unsigned short *)(vm->Globals + offset);
	value = (unsigned short)(*ptr - value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].us = value;
	vm->stacktop--;
	ScriptExecTrace("-= global variable at offset %u  value = %u  type = unsigned short\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_SUB_GLOBAL_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	int value = vm->Stack[vm->stacktop-1].i;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(int) < gVM.globalsize);
	int * ptr = (int *)(vm->Globals + offset);
	value = (int)(*ptr - value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].i = value;
	vm->stacktop--;
	ScriptExecTrace("-= global variable at offset %u  value = %d  type = int\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_SUB_GLOBAL_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned int value = vm->Stack[vm->stacktop-1].ui;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned int) < gVM.globalsize);
	unsigned int * ptr = (unsigned int *)(vm->Globals + offset);
	value = (unsigned int)(*ptr - value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ui = value;
	vm->stacktop--;
	ScriptExecTrace("-= global variable at offset %u  value = %u  type = unsigned int\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_SUB_GLOBAL_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	INT64 value = vm->Stack[vm->stacktop-1].ll;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(INT64) < gVM.globalsize);
	INT64* ptr = (INT64 *)(vm->Globals + offset);
	value = (INT64)(*ptr - value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ll = value;
	vm->stacktop--;
	ScriptExecTrace("-= global variable at offset %u  value = %I64d  type = int64\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_SUB_GLOBAL_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	UINT64 value = vm->Stack[vm->stacktop-1].ull;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(UINT64) < gVM.globalsize);
	UINT64 * ptr = (UINT64 *)(vm->Globals + offset);
	value = (UINT64)(*ptr - value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ull = value;
	vm->stacktop--;
	ScriptExecTrace("-= global variable at offset %u  value = %I64u  type = unsigned int64\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_SUB_GLOBAL_FL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	float value = vm->Stack[vm->stacktop-1].f;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(float) < gVM.globalsize);
	float * ptr = (float *)(vm->Globals + offset);
	value = (float)(*ptr - value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].f = value;
	vm->stacktop--;
	ScriptExecTrace("-= global variable at offset %u  value = %f  type = float\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_SUB_GLOBAL_DB(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	double value = vm->Stack[vm->stacktop-1].d;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(double) < gVM.globalsize);
	double * ptr = (double *)(vm->Globals + offset);
	value = (double)(*ptr - value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].d = value;
	vm->stacktop--;
	ScriptExecTrace("-= global variable at offset %u  value = %f  type = double\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_SUB_GLOBAL_LD(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	long double value = vm->Stack[vm->stacktop-1].ld;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(long double) < gVM.globalsize);
	long double * ptr = (long double *)(vm->Globals + offset);
	value = (long double)(*ptr - value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ld = value;
	vm->stacktop--;
	ScriptExecTrace("-= global variable at offset %u  value = %f  type = long double\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_SUB_LOCAL_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	char value = vm->Stack[vm->stacktop-1].c;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned char) < vm->localsize);
	char * ptr = (char *)(vm->Locals + offset);
	value = (char)(*ptr - value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].uc = value;
	vm->stacktop--;
	ScriptExecTrace("-= local variable at offset %u  value = %u  type = unsigned char\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_SUB_LOCAL_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned char value = vm->Stack[vm->stacktop-1].uc;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned char) < vm->localsize);
	unsigned char * ptr = (unsigned char *)(vm->Locals + offset);
	value = (unsigned char)(*ptr - value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].uc = value;
	vm->stacktop--;
	ScriptExecTrace("-= local variable at offset %u  value = %u  type = unsigned char\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_SUB_LOCAL_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	short value = vm->Stack[vm->stacktop-1].s;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(short) < vm->localsize);
	short * ptr = (short *)(vm->Locals + offset);
	value = (short)(*ptr - value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].s = value;
	vm->stacktop--;
	ScriptExecTrace("-= local variable at offset %u  value = %d  type = short\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_SUB_LOCAL_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned short value = vm->Stack[vm->stacktop-1].us;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned short) < vm->localsize);
	unsigned short * ptr = (unsigned short *)(vm->Locals + offset);
	value = (unsigned short)(*ptr - value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].us = value;
	vm->stacktop--;
	ScriptExecTrace("-= local variable at offset %u  value = %u  type = unsigned short\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_SUB_LOCAL_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	int value = vm->Stack[vm->stacktop-1].i;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(int) < vm->localsize);
	int * ptr = (int *)(vm->Locals + offset);
	value = (int)(*ptr - value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].i = value;
	vm->stacktop--;
	ScriptExecTrace("-= local variable at offset %u  value = %d  type = int\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_SUB_LOCAL_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned int value = vm->Stack[vm->stacktop-1].ui;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned int) < vm->localsize);
	unsigned int * ptr = (unsigned int *)(vm->Locals + offset);
	value = (unsigned int)(*ptr - value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ui = value;
	vm->stacktop--;
	ScriptExecTrace("-= local variable at offset %u  value = %u  type = unsigned int\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_SUB_LOCAL_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	INT64 value = vm->Stack[vm->stacktop-1].ll;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(INT64) < vm->localsize);
	INT64* ptr = (INT64 *)(vm->Locals + offset);
	value = (INT64)(*ptr - value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ll = value;
	vm->stacktop--;
	ScriptExecTrace("-= local variable at offset %u  value = %I64d  type = int64\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_SUB_LOCAL_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	UINT64 value = vm->Stack[vm->stacktop-1].ull;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(UINT64) < vm->localsize);
	UINT64 * ptr = (UINT64 *)(vm->Locals + offset);
	value = (UINT64)(*ptr - value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ull = value;
	vm->stacktop--;
	ScriptExecTrace("-= local variable at offset %u  value = %I64u  type = unsigned int64\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_SUB_LOCAL_FL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	float value = vm->Stack[vm->stacktop-1].f;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(float) < vm->localsize);
	float * ptr = (float *)(vm->Locals + offset);
	value = (float)(*ptr - value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].f = value;
	vm->stacktop--;
	ScriptExecTrace("-= local variable at offset %u  value = %f  type = float\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_SUB_LOCAL_DB(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	double value = vm->Stack[vm->stacktop-1].d;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(double) < vm->localsize);
	double * ptr = (double *)(vm->Locals + offset);
	value = (double)(*ptr - value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].d = value;
	vm->stacktop--;
	ScriptExecTrace("-= local variable at offset %u  value = %f  type = double\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_SUB_LOCAL_LD(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	long double value = vm->Stack[vm->stacktop-1].ld;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(long double) < vm->localsize);
	long double * ptr = (long double *)(vm->Locals + offset);
	value = (long double)(*ptr - value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ld = value;
	vm->stacktop--;
	ScriptExecTrace("-= local variable at offset %u  value = %f  type = long double\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_LSHIFT_GLOBAL_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	char value = vm->Stack[vm->stacktop-1].c;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned char) < gVM.globalsize);
	char * ptr = (char *)(vm->Globals + offset);
	value = (char)(*ptr << value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].uc = value;
	vm->stacktop--;
	ScriptExecTrace("<<= global variable at offset %u  value = %u  type = unsigned char\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_LSHIFT_GLOBAL_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned char value = vm->Stack[vm->stacktop-1].uc;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned char) < gVM.globalsize);
	unsigned char * ptr = (unsigned char *)(vm->Globals + offset);
	value = (unsigned char)(*ptr << value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].uc = value;
	vm->stacktop--;
	ScriptExecTrace("<<= global variable at offset %u  value = %u  type = unsigned char\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_LSHIFT_GLOBAL_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	short value = vm->Stack[vm->stacktop-1].s;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(short) < gVM.globalsize);
	short * ptr = (short *)(vm->Globals + offset);
	value = (short)(*ptr << value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].s = value;
	vm->stacktop--;
	ScriptExecTrace("<<= global variable at offset %u  value = %d  type = short\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_LSHIFT_GLOBAL_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned short value = vm->Stack[vm->stacktop-1].us;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned short) < gVM.globalsize);
	unsigned short * ptr = (unsigned short *)(vm->Globals + offset);
	value = (unsigned short)(*ptr << value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].us = value;
	vm->stacktop--;
	ScriptExecTrace("<<= global variable at offset %u  value = %u  type = unsigned short\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_LSHIFT_GLOBAL_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	int value = vm->Stack[vm->stacktop-1].i;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(int) < gVM.globalsize);
	int * ptr = (int *)(vm->Globals + offset);
	value = (int)(*ptr << value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].i = value;
	vm->stacktop--;
	ScriptExecTrace("<<= global variable at offset %u  value = %d  type = int\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_LSHIFT_GLOBAL_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned int value = vm->Stack[vm->stacktop-1].ui;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned int) < gVM.globalsize);
	unsigned int * ptr = (unsigned int *)(vm->Globals + offset);
	value = (unsigned int)(*ptr << value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ui = value;
	vm->stacktop--;
	ScriptExecTrace("<<= global variable at offset %u  value = %u  type = unsigned int\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_LSHIFT_GLOBAL_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	INT64 value = vm->Stack[vm->stacktop-1].ll;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(INT64) < gVM.globalsize);
	INT64* ptr = (INT64 *)(vm->Globals + offset);
	value = (INT64)(*ptr << value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ll = value;
	vm->stacktop--;
	ScriptExecTrace("<<= global variable at offset %u  value = %I64d  type = int64\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_LSHIFT_GLOBAL_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	UINT64 value = vm->Stack[vm->stacktop-1].ull;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(UINT64) < gVM.globalsize);
	UINT64 * ptr = (UINT64 *)(vm->Globals + offset);
	value = (UINT64)(*ptr << value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ull = value;
	vm->stacktop--;
	ScriptExecTrace("<<= global variable at offset %u  value = %I64u  type = unsigned int64\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_LSHIFT_LOCAL_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	char value = vm->Stack[vm->stacktop-1].c;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned char) < vm->localsize);
	char * ptr = (char *)(vm->Locals + offset);
	value = (char)(*ptr << value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].uc = value;
	vm->stacktop--;
	ScriptExecTrace("<<= local variable at offset %u  value = %u  type = unsigned char\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_LSHIFT_LOCAL_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned char value = vm->Stack[vm->stacktop-1].uc;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned char) < vm->localsize);
	unsigned char * ptr = (unsigned char *)(vm->Locals + offset);
	value = (unsigned char)(*ptr << value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].uc = value;
	vm->stacktop--;
	ScriptExecTrace("<<= local variable at offset %u  value = %u  type = unsigned char\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_LSHIFT_LOCAL_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	short value = vm->Stack[vm->stacktop-1].s;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(short) < vm->localsize);
	short * ptr = (short *)(vm->Locals + offset);
	value = (short)(*ptr << value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].s = value;
	vm->stacktop--;
	ScriptExecTrace("<<= local variable at offset %u  value = %d  type = short\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_LSHIFT_LOCAL_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned short value = vm->Stack[vm->stacktop-1].us;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned short) < vm->localsize);
	unsigned short * ptr = (unsigned short *)(vm->Locals + offset);
	value = (unsigned short)(*ptr << value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].us = value;
	vm->stacktop--;
	ScriptExecTrace("<<= local variable at offset %u  value = %u  type = unsigned short\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_LSHIFT_LOCAL_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	int value = vm->Stack[vm->stacktop-1].i;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(int) < vm->localsize);
	int * ptr = (int *)(vm->Locals + offset);
	value = (int)(*ptr << value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].i = value;
	vm->stacktop--;
	ScriptExecTrace("<<= local variable at offset %u  value = %d  type = int\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_LSHIFT_LOCAL_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned int value = vm->Stack[vm->stacktop-1].ui;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned int) < vm->localsize);
	unsigned int * ptr = (unsigned int *)(vm->Locals + offset);
	value = (unsigned int)(*ptr << value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ui = value;
	vm->stacktop--;
	ScriptExecTrace("<<= local variable at offset %u  value = %u  type = unsigned int\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_LSHIFT_LOCAL_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	INT64 value = vm->Stack[vm->stacktop-1].ll;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(INT64) < vm->localsize);
	INT64* ptr = (INT64 *)(vm->Locals + offset);
	value = (INT64)(*ptr << value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ll = value;
	vm->stacktop--;
	ScriptExecTrace("<<= local variable at offset %u  value = %I64d  type = int64\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_LSHIFT_LOCAL_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	UINT64 value = vm->Stack[vm->stacktop-1].ull;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(UINT64) < vm->localsize);
	UINT64 * ptr = (UINT64 *)(vm->Locals + offset);
	value = (UINT64)(*ptr << value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ull = value;
	vm->stacktop--;
	ScriptExecTrace("<<= local variable at offset %u  value = %I64u  type = unsigned int64\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_RSHIFT_GLOBAL_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	char value = vm->Stack[vm->stacktop-1].c;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned char) < gVM.globalsize);
	char * ptr = (char *)(vm->Globals + offset);
	value = (char)(*ptr >> value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].uc = value;
	vm->stacktop--;
	ScriptExecTrace(">>= global variable at offset %u  value = %u  type = unsigned char\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_RSHIFT_GLOBAL_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned char value = vm->Stack[vm->stacktop-1].uc;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned char) < gVM.globalsize);
	unsigned char * ptr = (unsigned char *)(vm->Globals + offset);
	value = (unsigned char)(*ptr >> value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].uc = value;
	vm->stacktop--;
	ScriptExecTrace(">>= global variable at offset %u  value = %u  type = unsigned char\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_RSHIFT_GLOBAL_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	short value = vm->Stack[vm->stacktop-1].s;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(short) < gVM.globalsize);
	short * ptr = (short *)(vm->Globals + offset);
	value = (short)(*ptr >> value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].s = value;
	vm->stacktop--;
	ScriptExecTrace(">>= global variable at offset %u  value = %d  type = short\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_RSHIFT_GLOBAL_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned short value = vm->Stack[vm->stacktop-1].us;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned short) < gVM.globalsize);
	unsigned short * ptr = (unsigned short *)(vm->Globals + offset);
	value = (unsigned short)(*ptr >> value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].us = value;
	vm->stacktop--;
	ScriptExecTrace(">>= global variable at offset %u  value = %u  type = unsigned short\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_RSHIFT_GLOBAL_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	int value = vm->Stack[vm->stacktop-1].i;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(int) < gVM.globalsize);
	int * ptr = (int *)(vm->Globals + offset);
	value = (int)(*ptr >> value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].i = value;
	vm->stacktop--;
	ScriptExecTrace(">>= global variable at offset %u  value = %d  type = int\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_RSHIFT_GLOBAL_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned int value = vm->Stack[vm->stacktop-1].ui;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned int) < gVM.globalsize);
	unsigned int * ptr = (unsigned int *)(vm->Globals + offset);
	value = (unsigned int)(*ptr >> value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ui = value;
	vm->stacktop--;
	ScriptExecTrace(">>= global variable at offset %u  value = %u  type = unsigned int\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_RSHIFT_GLOBAL_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	INT64 value = vm->Stack[vm->stacktop-1].ll;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(INT64) < gVM.globalsize);
	INT64* ptr = (INT64 *)(vm->Globals + offset);
	value = (INT64)(*ptr >> value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ll = value;
	vm->stacktop--;
	ScriptExecTrace(">>= global variable at offset %u  value = %I64d  type = int64\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_RSHIFT_GLOBAL_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	UINT64 value = vm->Stack[vm->stacktop-1].ull;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(UINT64) < gVM.globalsize);
	UINT64 * ptr = (UINT64 *)(vm->Globals + offset);
	value = (UINT64)(*ptr >> value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ull = value;
	vm->stacktop--;
	ScriptExecTrace(">>= global variable at offset %u  value = %I64u  type = unsigned int64\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_RSHIFT_LOCAL_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	char value = vm->Stack[vm->stacktop-1].c;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned char) < vm->localsize);
	char * ptr = (char *)(vm->Locals + offset);
	value = (char)(*ptr >> value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].uc = value;
	vm->stacktop--;
	ScriptExecTrace(">>= local variable at offset %u  value = %u  type = unsigned char\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_RSHIFT_LOCAL_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned char value = vm->Stack[vm->stacktop-1].uc;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned char) < vm->localsize);
	unsigned char * ptr = (unsigned char *)(vm->Locals + offset);
	value = (unsigned char)(*ptr >> value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].uc = value;
	vm->stacktop--;
	ScriptExecTrace(">>= local variable at offset %u  value = %u  type = unsigned char\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_RSHIFT_LOCAL_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	short value = vm->Stack[vm->stacktop-1].s;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(short) < vm->localsize);
	short * ptr = (short *)(vm->Locals + offset);
	value = (short)(*ptr >> value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].s = value;
	vm->stacktop--;
	ScriptExecTrace(">>= local variable at offset %u  value = %d  type = short\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_RSHIFT_LOCAL_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned short value = vm->Stack[vm->stacktop-1].us;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned short) < vm->localsize);
	unsigned short * ptr = (unsigned short *)(vm->Locals + offset);
	value = (unsigned short)(*ptr >> value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].us = value;
	vm->stacktop--;
	ScriptExecTrace(">>= local variable at offset %u  value = %u  type = unsigned short\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_RSHIFT_LOCAL_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	int value = vm->Stack[vm->stacktop-1].i;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(int) < vm->localsize);
	int * ptr = (int *)(vm->Locals + offset);
	value = (int)(*ptr >> value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].i = value;
	vm->stacktop--;
	ScriptExecTrace(">>= local variable at offset %u  value = %d  type = int\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_RSHIFT_LOCAL_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned int value = vm->Stack[vm->stacktop-1].ui;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned int) < vm->localsize);
	unsigned int * ptr = (unsigned int *)(vm->Locals + offset);
	value = (unsigned int)(*ptr >> value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ui = value;
	vm->stacktop--;
	ScriptExecTrace(">>= local variable at offset %u  value = %u  type = unsigned int\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_RSHIFT_LOCAL_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	INT64 value = vm->Stack[vm->stacktop-1].ll;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(INT64) < vm->localsize);
	INT64* ptr = (INT64 *)(vm->Locals + offset);
	value = (INT64)(*ptr >> value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ll = value;
	vm->stacktop--;
	ScriptExecTrace(">>= local variable at offset %u  value = %I64d  type = int64\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_RSHIFT_LOCAL_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	UINT64 value = vm->Stack[vm->stacktop-1].ull;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(UINT64) < vm->localsize);
	UINT64 * ptr = (UINT64 *)(vm->Locals + offset);
	value = (UINT64)(*ptr >> value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ull = value;
	vm->stacktop--;
	ScriptExecTrace(">>= local variable at offset %u  value = %I64u  type = unsigned int64\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_AND_GLOBAL_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	char value = vm->Stack[vm->stacktop-1].c;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned char) < gVM.globalsize);
	char * ptr = (char *)(vm->Globals + offset);
	value = (char)(*ptr & value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].uc = value;
	vm->stacktop--;
	ScriptExecTrace("&= global variable at offset %u  value = %u  type = unsigned char\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_AND_GLOBAL_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned char value = vm->Stack[vm->stacktop-1].uc;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned char) < gVM.globalsize);
	unsigned char * ptr = (unsigned char *)(vm->Globals + offset);
	value = (unsigned char)(*ptr & value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].uc = value;
	vm->stacktop--;
	ScriptExecTrace("&= global variable at offset %u  value = %u  type = unsigned char\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_AND_GLOBAL_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	short value = vm->Stack[vm->stacktop-1].s;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(short) < gVM.globalsize);
	short * ptr = (short *)(vm->Globals + offset);
	value = (short)(*ptr & value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].s = value;
	vm->stacktop--;
	ScriptExecTrace("&= global variable at offset %u  value = %d  type = short\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_AND_GLOBAL_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned short value = vm->Stack[vm->stacktop-1].us;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned short) < gVM.globalsize);
	unsigned short * ptr = (unsigned short *)(vm->Globals + offset);
	value = (unsigned short)(*ptr & value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].us = value;
	vm->stacktop--;
	ScriptExecTrace("&= global variable at offset %u  value = %u  type = unsigned short\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_AND_GLOBAL_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	int value = vm->Stack[vm->stacktop-1].i;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(int) < gVM.globalsize);
	int * ptr = (int *)(vm->Globals + offset);
	value = (int)(*ptr & value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].i = value;
	vm->stacktop--;
	ScriptExecTrace("&= global variable at offset %u  value = %d  type = int\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_AND_GLOBAL_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned int value = vm->Stack[vm->stacktop-1].ui;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned int) < gVM.globalsize);
	unsigned int * ptr = (unsigned int *)(vm->Globals + offset);
	value = (unsigned int)(*ptr & value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ui = value;
	vm->stacktop--;
	ScriptExecTrace("&= global variable at offset %u  value = %u  type = unsigned int\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_AND_GLOBAL_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	INT64 value = vm->Stack[vm->stacktop-1].ll;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(INT64) < gVM.globalsize);
	INT64* ptr = (INT64 *)(vm->Globals + offset);
	value = (INT64)(*ptr & value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ll = value;
	vm->stacktop--;
	ScriptExecTrace("&= global variable at offset %u  value = %I64d  type = int64\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_AND_GLOBAL_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	UINT64 value = vm->Stack[vm->stacktop-1].ull;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(UINT64) < gVM.globalsize);
	UINT64 * ptr = (UINT64 *)(vm->Globals + offset);
	value = (UINT64)(*ptr & value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ull = value;
	vm->stacktop--;
	ScriptExecTrace("&= global variable at offset %u  value = %I64u  type = unsigned int64\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_AND_LOCAL_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	char value = vm->Stack[vm->stacktop-1].c;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned char) < vm->localsize);
	char * ptr = (char *)(vm->Locals + offset);
	value = (char)(*ptr & value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].uc = value;
	vm->stacktop--;
	ScriptExecTrace("&= local variable at offset %u  value = %u  type = unsigned char\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_AND_LOCAL_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned char value = vm->Stack[vm->stacktop-1].uc;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned char) < vm->localsize);
	unsigned char * ptr = (unsigned char *)(vm->Locals + offset);
	value = (unsigned char)(*ptr & value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].uc = value;
	vm->stacktop--;
	ScriptExecTrace("&= local variable at offset %u  value = %u  type = unsigned char\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_AND_LOCAL_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	short value = vm->Stack[vm->stacktop-1].s;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(short) < vm->localsize);
	short * ptr = (short *)(vm->Locals + offset);
	value = (short)(*ptr & value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].s = value;
	vm->stacktop--;
	ScriptExecTrace("&= local variable at offset %u  value = %d  type = short\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_AND_LOCAL_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned short value = vm->Stack[vm->stacktop-1].us;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned short) < vm->localsize);
	unsigned short * ptr = (unsigned short *)(vm->Locals + offset);
	value = (unsigned short)(*ptr & value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].us = value;
	vm->stacktop--;
	ScriptExecTrace("&= local variable at offset %u  value = %u  type = unsigned short\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_AND_LOCAL_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	int value = vm->Stack[vm->stacktop-1].i;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(int) < vm->localsize);
	int * ptr = (int *)(vm->Locals + offset);
	value = (int)(*ptr & value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].i = value;
	vm->stacktop--;
	ScriptExecTrace("&= local variable at offset %u  value = %d  type = int\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_AND_LOCAL_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned int value = vm->Stack[vm->stacktop-1].ui;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned int) < vm->localsize);
	unsigned int * ptr = (unsigned int *)(vm->Locals + offset);
	value = (unsigned int)(*ptr & value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ui = value;
	vm->stacktop--;
	ScriptExecTrace("&= local variable at offset %u  value = %u  type = unsigned int\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_AND_LOCAL_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	INT64 value = vm->Stack[vm->stacktop-1].ll;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(INT64) < vm->localsize);
	INT64* ptr = (INT64 *)(vm->Locals + offset);
	value = (INT64)(*ptr & value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ll = value;
	vm->stacktop--;
	ScriptExecTrace("&= local variable at offset %u  value = %I64d  type = int64\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_AND_LOCAL_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	UINT64 value = vm->Stack[vm->stacktop-1].ull;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(UINT64) < vm->localsize);
	UINT64 * ptr = (UINT64 *)(vm->Locals + offset);
	value = (UINT64)(*ptr & value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ull = value;
	vm->stacktop--;
	ScriptExecTrace("&= local variable at offset %u  value = %I64u  type = unsigned int64\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_XOR_GLOBAL_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	char value = vm->Stack[vm->stacktop-1].c;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned char) < gVM.globalsize);
	char * ptr = (char *)(vm->Globals + offset);
	value = (char)(*ptr ^ value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].uc = value;
	vm->stacktop--;
	ScriptExecTrace("^= global variable at offset %u  value = %u  type = unsigned char\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_XOR_GLOBAL_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned char value = vm->Stack[vm->stacktop-1].uc;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned char) < gVM.globalsize);
	unsigned char * ptr = (unsigned char *)(vm->Globals + offset);
	value = (unsigned char)(*ptr ^ value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].uc = value;
	vm->stacktop--;
	ScriptExecTrace("^= global variable at offset %u  value = %u  type = unsigned char\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_XOR_GLOBAL_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	short value = vm->Stack[vm->stacktop-1].s;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(short) < gVM.globalsize);
	short * ptr = (short *)(vm->Globals + offset);
	value = (short)(*ptr ^ value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].s = value;
	vm->stacktop--;
	ScriptExecTrace("^= global variable at offset %u  value = %d  type = short\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_XOR_GLOBAL_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned short value = vm->Stack[vm->stacktop-1].us;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned short) < gVM.globalsize);
	unsigned short * ptr = (unsigned short *)(vm->Globals + offset);
	value = (unsigned short)(*ptr ^ value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].us = value;
	vm->stacktop--;
	ScriptExecTrace("^= global variable at offset %u  value = %u  type = unsigned short\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_XOR_GLOBAL_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	int value = vm->Stack[vm->stacktop-1].i;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(int) < gVM.globalsize);
	int * ptr = (int *)(vm->Globals + offset);
	value = (int)(*ptr ^ value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].i = value;
	vm->stacktop--;
	ScriptExecTrace("^= global variable at offset %u  value = %d  type = int\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_XOR_GLOBAL_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned int value = vm->Stack[vm->stacktop-1].ui;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned int) < gVM.globalsize);
	unsigned int * ptr = (unsigned int *)(vm->Globals + offset);
	value = (unsigned int)(*ptr ^ value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ui = value;
	vm->stacktop--;
	ScriptExecTrace("^= global variable at offset %u  value = %u  type = unsigned int\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_XOR_GLOBAL_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	INT64 value = vm->Stack[vm->stacktop-1].ll;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(INT64) < gVM.globalsize);
	INT64* ptr = (INT64 *)(vm->Globals + offset);
	value = (INT64)(*ptr ^ value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ll = value;
	vm->stacktop--;
	ScriptExecTrace("^= global variable at offset %u  value = %I64d  type = int64\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_XOR_GLOBAL_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	UINT64 value = vm->Stack[vm->stacktop-1].ull;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(UINT64) < gVM.globalsize);
	UINT64 * ptr = (UINT64 *)(vm->Globals + offset);
	value = (UINT64)(*ptr ^ value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ull = value;
	vm->stacktop--;
	ScriptExecTrace("^= global variable at offset %u  value = %I64u  type = unsigned int64\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_XOR_LOCAL_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	char value = vm->Stack[vm->stacktop-1].c;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned char) < vm->localsize);
	char * ptr = (char *)(vm->Locals + offset);
	value = (char)(*ptr ^ value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].uc = value;
	vm->stacktop--;
	ScriptExecTrace("^= local variable at offset %u  value = %u  type = unsigned char\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_XOR_LOCAL_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned char value = vm->Stack[vm->stacktop-1].uc;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned char) < vm->localsize);
	unsigned char * ptr = (unsigned char *)(vm->Locals + offset);
	value = (unsigned char)(*ptr ^ value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].uc = value;
	vm->stacktop--;
	ScriptExecTrace("^= local variable at offset %u  value = %u  type = unsigned char\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_XOR_LOCAL_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	short value = vm->Stack[vm->stacktop-1].s;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(short) < vm->localsize);
	short * ptr = (short *)(vm->Locals + offset);
	value = (short)(*ptr ^ value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].s = value;
	vm->stacktop--;
	ScriptExecTrace("^= local variable at offset %u  value = %d  type = short\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_XOR_LOCAL_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned short value = vm->Stack[vm->stacktop-1].us;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned short) < vm->localsize);
	unsigned short * ptr = (unsigned short *)(vm->Locals + offset);
	value = (unsigned short)(*ptr ^ value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].us = value;
	vm->stacktop--;
	ScriptExecTrace("^= local variable at offset %u  value = %u  type = unsigned short\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_XOR_LOCAL_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	int value = vm->Stack[vm->stacktop-1].i;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(int) < vm->localsize);
	int * ptr = (int *)(vm->Locals + offset);
	value = (int)(*ptr ^ value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].i = value;
	vm->stacktop--;
	ScriptExecTrace("^= local variable at offset %u  value = %d  type = int\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_XOR_LOCAL_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned int value = vm->Stack[vm->stacktop-1].ui;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned int) < vm->localsize);
	unsigned int * ptr = (unsigned int *)(vm->Locals + offset);
	value = (unsigned int)(*ptr ^ value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ui = value;
	vm->stacktop--;
	ScriptExecTrace("^= local variable at offset %u  value = %u  type = unsigned int\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_XOR_LOCAL_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	INT64 value = vm->Stack[vm->stacktop-1].ll;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(INT64) < vm->localsize);
	INT64* ptr = (INT64 *)(vm->Locals + offset);
	value = (INT64)(*ptr ^ value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ll = value;
	vm->stacktop--;
	ScriptExecTrace("^= local variable at offset %u  value = %I64d  type = int64\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_XOR_LOCAL_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	UINT64 value = vm->Stack[vm->stacktop-1].ull;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(UINT64) < vm->localsize);
	UINT64 * ptr = (UINT64 *)(vm->Locals + offset);
	value = (UINT64)(*ptr ^ value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ull = value;
	vm->stacktop--;
	ScriptExecTrace("^= local variable at offset %u  value = %I64u  type = unsigned int64\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_OR_GLOBAL_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	char value = vm->Stack[vm->stacktop-1].c;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned char) < gVM.globalsize);
	char * ptr = (char *)(vm->Globals + offset);
	value = (char)(*ptr | value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].uc = value;
	vm->stacktop--;
	ScriptExecTrace("|= global variable at offset %u  value = %u  type = unsigned char\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_OR_GLOBAL_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned char value = vm->Stack[vm->stacktop-1].uc;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned char) < gVM.globalsize);
	unsigned char * ptr = (unsigned char *)(vm->Globals + offset);
	value = (unsigned char)(*ptr | value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].uc = value;
	vm->stacktop--;
	ScriptExecTrace("|= global variable at offset %u  value = %u  type = unsigned char\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_OR_GLOBAL_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	short value = vm->Stack[vm->stacktop-1].s;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(short) < gVM.globalsize);
	short * ptr = (short *)(vm->Globals + offset);
	value = (short)(*ptr | value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].s = value;
	vm->stacktop--;
	ScriptExecTrace("|= global variable at offset %u  value = %d  type = short\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_OR_GLOBAL_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned short value = vm->Stack[vm->stacktop-1].us;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned short) < gVM.globalsize);
	unsigned short * ptr = (unsigned short *)(vm->Globals + offset);
	value = (unsigned short)(*ptr | value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].us = value;
	vm->stacktop--;
	ScriptExecTrace("|= global variable at offset %u  value = %u  type = unsigned short\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_OR_GLOBAL_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	int value = vm->Stack[vm->stacktop-1].i;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(int) < gVM.globalsize);
	int * ptr = (int *)(vm->Globals + offset);
	value = (int)(*ptr | value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].i = value;
	vm->stacktop--;
	ScriptExecTrace("|= global variable at offset %u  value = %d  type = int\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_OR_GLOBAL_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned int value = vm->Stack[vm->stacktop-1].ui;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned int) < gVM.globalsize);
	unsigned int * ptr = (unsigned int *)(vm->Globals + offset);
	value = (unsigned int)(*ptr | value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ui = value;
	vm->stacktop--;
	ScriptExecTrace("|= global variable at offset %u  value = %u  type = unsigned int\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_OR_GLOBAL_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	INT64 value = vm->Stack[vm->stacktop-1].ll;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(INT64) < gVM.globalsize);
	INT64* ptr = (INT64 *)(vm->Globals + offset);
	value = (INT64)(*ptr | value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ll = value;
	vm->stacktop--;
	ScriptExecTrace("|= global variable at offset %u  value = %I64d  type = int64\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_OR_GLOBAL_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	UINT64 value = vm->Stack[vm->stacktop-1].ull;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(UINT64) < gVM.globalsize);
	UINT64 * ptr = (UINT64 *)(vm->Globals + offset);
	value = (UINT64)(*ptr | value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ull = value;
	vm->stacktop--;
	ScriptExecTrace("|= global variable at offset %u  value = %I64u  type = unsigned int64\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_OR_LOCAL_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	char value = vm->Stack[vm->stacktop-1].c;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned char) < vm->localsize);
	char * ptr = (char *)(vm->Locals + offset);
	value = (char)(*ptr | value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].uc = value;
	vm->stacktop--;
	ScriptExecTrace("|= local variable at offset %u  value = %u  type = unsigned char\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_OR_LOCAL_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned char value = vm->Stack[vm->stacktop-1].uc;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned char) < vm->localsize);
	unsigned char * ptr = (unsigned char *)(vm->Locals + offset);
	value = (unsigned char)(*ptr | value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].uc = value;
	vm->stacktop--;
	ScriptExecTrace("|= local variable at offset %u  value = %u  type = unsigned char\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_OR_LOCAL_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	short value = vm->Stack[vm->stacktop-1].s;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(short) < vm->localsize);
	short * ptr = (short *)(vm->Locals + offset);
	value = (short)(*ptr | value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].s = value;
	vm->stacktop--;
	ScriptExecTrace("|= local variable at offset %u  value = %d  type = short\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_OR_LOCAL_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned short value = vm->Stack[vm->stacktop-1].us;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned short) < vm->localsize);
	unsigned short * ptr = (unsigned short *)(vm->Locals + offset);
	value = (unsigned short)(*ptr | value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].us = value;
	vm->stacktop--;
	ScriptExecTrace("|= local variable at offset %u  value = %u  type = unsigned short\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_OR_LOCAL_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	int value = vm->Stack[vm->stacktop-1].i;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(int) < vm->localsize);
	int * ptr = (int *)(vm->Locals + offset);
	value = (int)(*ptr | value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].i = value;
	vm->stacktop--;
	ScriptExecTrace("|= local variable at offset %u  value = %d  type = int\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_OR_LOCAL_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	unsigned int value = vm->Stack[vm->stacktop-1].ui;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(unsigned int) < vm->localsize);
	unsigned int * ptr = (unsigned int *)(vm->Locals + offset);
	value = (unsigned int)(*ptr | value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ui = value;
	vm->stacktop--;
	ScriptExecTrace("|= local variable at offset %u  value = %u  type = unsigned int\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_OR_LOCAL_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	INT64 value = vm->Stack[vm->stacktop-1].ll;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(INT64) < vm->localsize);
	INT64* ptr = (INT64 *)(vm->Locals + offset);
	value = (INT64)(*ptr | value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ll = value;
	vm->stacktop--;
	ScriptExecTrace("|= local variable at offset %u  value = %I64d  type = int64\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_A_OR_LOCAL_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int offset = vm->Stack[vm->stacktop-2].ui;
	UINT64 value = vm->Stack[vm->stacktop-1].ull;
	ASSERT_RETZERO(offset >= 0 && offset + sizeof(UINT64) < vm->localsize);
	UINT64 * ptr = (UINT64 *)(vm->Locals + offset);
	value = (UINT64)(*ptr | value);
	*ptr = value;
	vm->Stack[vm->stacktop-2].ull = value;
	vm->stacktop--;
	ScriptExecTrace("|= local variable at offset %u  value = %I64u  type = unsigned int64\n", offset, value);
	return 1;
}

//----------------------------------------------------------------------------
#pragma warning(push)
#pragma warning(disable : 4146)		// disable warning about - on unsigned values
static inline int fp_VMI_NEG_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	char a = vm->Stack[vm->stacktop-1].c;
	vm->Stack[vm->stacktop-1].c = -a;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_NEG_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	unsigned char a = vm->Stack[vm->stacktop-1].uc;
	vm->Stack[vm->stacktop-1].uc = -a;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_NEG_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	short a = vm->Stack[vm->stacktop-1].s;
	vm->Stack[vm->stacktop-1].s = -a;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_NEG_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	unsigned short a = vm->Stack[vm->stacktop-1].us;
	vm->Stack[vm->stacktop-1].us = -a;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_NEG_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	int a = vm->Stack[vm->stacktop-1].i;
	vm->Stack[vm->stacktop-1].i = -a;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_NEG_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	unsigned int a = vm->Stack[vm->stacktop-1].ui;
	vm->Stack[vm->stacktop-1].ui = -a;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_NEG_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	INT64 a = vm->Stack[vm->stacktop-1].ll;
	vm->Stack[vm->stacktop-1].ll = -a;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_NEG_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	UINT64 a = vm->Stack[vm->stacktop-1].ull;
	vm->Stack[vm->stacktop-1].ull = -a;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_NEG_FL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	float a = vm->Stack[vm->stacktop-1].f;
	vm->Stack[vm->stacktop-1].f = -a;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_NEG_DB(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	double a = vm->Stack[vm->stacktop-1].d;
	vm->Stack[vm->stacktop-1].d = -a;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_NEG_LD(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	long double a = vm->Stack[vm->stacktop-1].ld;
	vm->Stack[vm->stacktop-1].ld = -a;
	return 1;
}
#pragma warning(pop)

//----------------------------------------------------------------------------
static inline int fp_VMI_TILDE_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	char a = vm->Stack[vm->stacktop-1].c;
	vm->Stack[vm->stacktop-1].c = ~a;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_TILDE_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	unsigned char a = vm->Stack[vm->stacktop-1].uc;
	vm->Stack[vm->stacktop-1].uc = ~a;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_TILDE_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	short a = vm->Stack[vm->stacktop-1].s;
	vm->Stack[vm->stacktop-1].s = ~a;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_TILDE_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	unsigned short a = vm->Stack[vm->stacktop-1].us;
	vm->Stack[vm->stacktop-1].us = ~a;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_TILDE_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	int a = vm->Stack[vm->stacktop-1].i;
	vm->Stack[vm->stacktop-1].i = ~a;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_TILDE_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	unsigned int a = vm->Stack[vm->stacktop-1].ui;
	vm->Stack[vm->stacktop-1].ui = ~a;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_TILDE_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	INT64 a = vm->Stack[vm->stacktop-1].ll;
	vm->Stack[vm->stacktop-1].ll = ~a;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_TILDE_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	UINT64 a = vm->Stack[vm->stacktop-1].ull;
	vm->Stack[vm->stacktop-1].ull = ~a;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_NOT_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	char a = vm->Stack[vm->stacktop-1].c;
	vm->Stack[vm->stacktop-1].c = !a;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_NOT_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	unsigned char a = vm->Stack[vm->stacktop-1].uc;
	vm->Stack[vm->stacktop-1].uc = !a;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_NOT_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	short a = vm->Stack[vm->stacktop-1].s;
	vm->Stack[vm->stacktop-1].s = !a;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_NOT_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	unsigned short a = vm->Stack[vm->stacktop-1].us;
	vm->Stack[vm->stacktop-1].us = !a;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_NOT_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	int a = vm->Stack[vm->stacktop-1].i;
	vm->Stack[vm->stacktop-1].i = !a;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_NOT_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	unsigned int a = vm->Stack[vm->stacktop-1].ui;
	vm->Stack[vm->stacktop-1].ui = !a;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_NOT_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	INT64 a = vm->Stack[vm->stacktop-1].ll;
	vm->Stack[vm->stacktop-1].ll = !a;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_NOT_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1); 
	UINT64 a = vm->Stack[vm->stacktop-1].ull;
	vm->Stack[vm->stacktop-1].ull = !a;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_POWER_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	char a = vm->Stack[vm->stacktop-2].c;
	char b = vm->Stack[vm->stacktop-1].c;
	vm->Stack[vm->stacktop-2].c = (char)POWI(a, b);;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_POWER_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned char a = vm->Stack[vm->stacktop-2].uc;
	unsigned char b = vm->Stack[vm->stacktop-1].uc;
	vm->Stack[vm->stacktop-2].uc = (unsigned char)POWI(a, b);;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_POWER_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	short a = vm->Stack[vm->stacktop-2].s;
	short b = vm->Stack[vm->stacktop-1].s;
	vm->Stack[vm->stacktop-2].s = (short)POWI(a, b);;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_POWER_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned short a = vm->Stack[vm->stacktop-2].us;
	unsigned short b = vm->Stack[vm->stacktop-1].us;
	vm->Stack[vm->stacktop-2].us = (unsigned short)POWI(a, b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_POWER_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	int a = vm->Stack[vm->stacktop-2].i;
	int b = vm->Stack[vm->stacktop-1].i;
	vm->Stack[vm->stacktop-2].i = (int)POWI(a, b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_POWER_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	unsigned int a = vm->Stack[vm->stacktop-2].ui;
	unsigned int b = vm->Stack[vm->stacktop-1].ui;
	vm->Stack[vm->stacktop-2].ui = (unsigned int)POWI(a, b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_POWER_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	INT64 a = vm->Stack[vm->stacktop-2].ll;
	INT64 b = vm->Stack[vm->stacktop-1].ll;
	vm->Stack[vm->stacktop-2].ll = (INT64)POWI(a, b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_POWER_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	UINT64 a = vm->Stack[vm->stacktop-2].ull;
	UINT64 b = vm->Stack[vm->stacktop-1].ull;
	vm->Stack[vm->stacktop-2].ull = (UINT64)POWI(a, b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_POWER_FL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	float a = vm->Stack[vm->stacktop-2].f;
	float b = vm->Stack[vm->stacktop-1].f;
	vm->Stack[vm->stacktop-2].f = powf(a, b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_POWER_DB(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	double a = vm->Stack[vm->stacktop-2].d;
	double b = vm->Stack[vm->stacktop-1].d;
	vm->Stack[vm->stacktop-2].d = pow(a, b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_POWER_LD(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2); 
	long double a = vm->Stack[vm->stacktop-2].ld;
	long double b = vm->Stack[vm->stacktop-1].ld;
	vm->Stack[vm->stacktop-2].ld = pow(a, b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_MUL_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	char a = vm->Stack[vm->stacktop-2].c;
	char b = vm->Stack[vm->stacktop-1].c;
	vm->Stack[vm->stacktop-2].c = a * b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_MUL_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	unsigned char a = vm->Stack[vm->stacktop-2].uc;
	unsigned char b = vm->Stack[vm->stacktop-1].uc;
	vm->Stack[vm->stacktop-2].uc = a * b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_MUL_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	short a = vm->Stack[vm->stacktop-2].s;
	short b = vm->Stack[vm->stacktop-1].s;
	vm->Stack[vm->stacktop-2].s = a * b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_MUL_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	unsigned short a = vm->Stack[vm->stacktop-2].us;
	unsigned short b = vm->Stack[vm->stacktop-1].us;
	vm->Stack[vm->stacktop-2].us = a * b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_MUL_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	int a = vm->Stack[vm->stacktop-2].i;
	int b = vm->Stack[vm->stacktop-1].i;
	vm->Stack[vm->stacktop-2].i = a * b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_MUL_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	unsigned int a = vm->Stack[vm->stacktop-2].ui;
	unsigned int b = vm->Stack[vm->stacktop-1].ui;
	vm->Stack[vm->stacktop-2].ui = a * b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_MUL_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	INT64 a = vm->Stack[vm->stacktop-2].ll;
	INT64 b = vm->Stack[vm->stacktop-1].ll;
	vm->Stack[vm->stacktop-2].ll = a * b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_MUL_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	UINT64 a = vm->Stack[vm->stacktop-2].ull;
	UINT64 b = vm->Stack[vm->stacktop-1].ull;
	vm->Stack[vm->stacktop-2].ull = a * b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_MUL_FL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	float a = vm->Stack[vm->stacktop-2].f;
	float b = vm->Stack[vm->stacktop-1].f;
	vm->Stack[vm->stacktop-2].f = a * b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_MUL_DB(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	double a = vm->Stack[vm->stacktop-2].d;
	double b = vm->Stack[vm->stacktop-1].d;
	vm->Stack[vm->stacktop-2].d = a * b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_MUL_LD(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	long double a = vm->Stack[vm->stacktop-2].ld;
	long double b = vm->Stack[vm->stacktop-1].ld;
	vm->Stack[vm->stacktop-2].ld = a * b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_DIV_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	char a = vm->Stack[vm->stacktop-2].c;
	char b = vm->Stack[vm->stacktop-1].c;
	ASSERT_RETZERO(b != 0);
	vm->Stack[vm->stacktop-2].c = a / b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_DIV_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	unsigned char a = vm->Stack[vm->stacktop-2].uc;
	unsigned char b = vm->Stack[vm->stacktop-1].uc;
	ASSERT_RETZERO(b != 0);
	vm->Stack[vm->stacktop-2].uc = a / b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_DIV_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	short a = vm->Stack[vm->stacktop-2].s;
	short b = vm->Stack[vm->stacktop-1].s;
	ASSERT_RETZERO(b != 0);
	vm->Stack[vm->stacktop-2].s = a / b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_DIV_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	unsigned short a = vm->Stack[vm->stacktop-2].us;
	unsigned short b = vm->Stack[vm->stacktop-1].us;
	ASSERT_RETZERO(b != 0);
	vm->Stack[vm->stacktop-2].us = a / b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_DIV_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	int a = vm->Stack[vm->stacktop-2].i;
	int b = vm->Stack[vm->stacktop-1].i;
	if (b == 0) return 0;
	vm->Stack[vm->stacktop-2].i = a / b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_DIV_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	unsigned int a = vm->Stack[vm->stacktop-2].ui;
	unsigned int b = vm->Stack[vm->stacktop-1].ui;
	ASSERT_RETZERO(b != 0);
	vm->Stack[vm->stacktop-2].ui = a / b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_DIV_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	INT64 a = vm->Stack[vm->stacktop-2].ll;
	INT64 b = vm->Stack[vm->stacktop-1].ll;
	ASSERT_RETZERO(b != 0);
	vm->Stack[vm->stacktop-2].ll = a / b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_DIV_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	UINT64 a = vm->Stack[vm->stacktop-2].ull;
	UINT64 b = vm->Stack[vm->stacktop-1].ull;
	ASSERT_RETZERO(b != 0);
	vm->Stack[vm->stacktop-2].ull = a / b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_DIV_FL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	float a = vm->Stack[vm->stacktop-2].f;
	float b = vm->Stack[vm->stacktop-1].f;
	ASSERT_RETZERO(b != 0.0f);
	vm->Stack[vm->stacktop-2].f = a / b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_DIV_DB(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	double a = vm->Stack[vm->stacktop-2].d;
	double b = vm->Stack[vm->stacktop-1].d;
	ASSERT_RETZERO(b != 0.0);
	vm->Stack[vm->stacktop-2].d = a / b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_DIV_LD(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	long double a = vm->Stack[vm->stacktop-2].ld;
	long double b = vm->Stack[vm->stacktop-1].ld;
	ASSERT_RETZERO(b != 0.0);
	vm->Stack[vm->stacktop-2].ld = a / b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_MOD_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	char a = vm->Stack[vm->stacktop-2].c;
	char b = vm->Stack[vm->stacktop-1].c;
	ASSERT_RETZERO(b != 0);
	vm->Stack[vm->stacktop-2].c = a % b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_MOD_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	unsigned char a = vm->Stack[vm->stacktop-2].uc;
	unsigned char b = vm->Stack[vm->stacktop-1].uc;
	ASSERT_RETZERO(b != 0);
	vm->Stack[vm->stacktop-2].uc = a % b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_MOD_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	short a = vm->Stack[vm->stacktop-2].s;
	short b = vm->Stack[vm->stacktop-1].s;
	ASSERT_RETZERO(b != 0);
	vm->Stack[vm->stacktop-2].s = a % b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_MOD_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	unsigned short a = vm->Stack[vm->stacktop-2].us;
	unsigned short b = vm->Stack[vm->stacktop-1].us;
	ASSERT_RETZERO(b != 0);
	vm->Stack[vm->stacktop-2].us = a % b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_MOD_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	int a = vm->Stack[vm->stacktop-2].i;
	int b = vm->Stack[vm->stacktop-1].i;
	ASSERT_RETZERO(b != 0);
	vm->Stack[vm->stacktop-2].i = a % b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_MOD_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	unsigned int a = vm->Stack[vm->stacktop-2].ui;
	unsigned int b = vm->Stack[vm->stacktop-1].ui;
	ASSERT_RETZERO(b != 0);
	vm->Stack[vm->stacktop-2].ui = a % b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_MOD_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	INT64 a = vm->Stack[vm->stacktop-2].ll;
	INT64 b = vm->Stack[vm->stacktop-1].ll;
	ASSERT_RETZERO(b != 0);
	vm->Stack[vm->stacktop-2].ll = a % b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_MOD_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	UINT64 a = vm->Stack[vm->stacktop-2].ull;
	UINT64 b = vm->Stack[vm->stacktop-1].ull;
	ASSERT_RETZERO(b != 0);
	vm->Stack[vm->stacktop-2].ull = a % b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_ADD_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	char a = vm->Stack[vm->stacktop-2].c;
	char b = vm->Stack[vm->stacktop-1].c;
	vm->Stack[vm->stacktop-2].c = a + b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_ADD_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	unsigned char a = vm->Stack[vm->stacktop-2].uc;
	unsigned char b = vm->Stack[vm->stacktop-1].uc;
	vm->Stack[vm->stacktop-2].uc = a + b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_ADD_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	short a = vm->Stack[vm->stacktop-2].s;
	short b = vm->Stack[vm->stacktop-1].s;
	vm->Stack[vm->stacktop-2].s = a + b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_ADD_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	unsigned short a = vm->Stack[vm->stacktop-2].us;
	unsigned short b = vm->Stack[vm->stacktop-1].us;
	vm->Stack[vm->stacktop-2].us = a + b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_ADD_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	int a = vm->Stack[vm->stacktop-2].i;
	int b = vm->Stack[vm->stacktop-1].i;
	vm->Stack[vm->stacktop-2].i = a + b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_ADD_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	unsigned int a = vm->Stack[vm->stacktop-2].ui;
	unsigned int b = vm->Stack[vm->stacktop-1].ui;
	vm->Stack[vm->stacktop-2].ui = a + b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_ADD_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	INT64 a = vm->Stack[vm->stacktop-2].ll;
	INT64 b = vm->Stack[vm->stacktop-1].ll;
	vm->Stack[vm->stacktop-2].ll = a + b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_ADD_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	UINT64 a = vm->Stack[vm->stacktop-2].ull;
	UINT64 b = vm->Stack[vm->stacktop-1].ull;
	vm->Stack[vm->stacktop-2].ull = a + b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_ADD_FL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	float a = vm->Stack[vm->stacktop-2].f;
	float b = vm->Stack[vm->stacktop-1].f;
	vm->Stack[vm->stacktop-2].f = a + b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_ADD_DB(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	double a = vm->Stack[vm->stacktop-2].d;
	double b = vm->Stack[vm->stacktop-1].d;
	vm->Stack[vm->stacktop-2].d = a + b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_ADD_LD(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	long double a = vm->Stack[vm->stacktop-2].ld;
	long double b = vm->Stack[vm->stacktop-1].ld;
	vm->Stack[vm->stacktop-2].ld = a + b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_SUB_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	char a = vm->Stack[vm->stacktop-2].c;
	char b = vm->Stack[vm->stacktop-1].c;
	vm->Stack[vm->stacktop-2].c = a - b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_SUB_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	unsigned char a = vm->Stack[vm->stacktop-2].uc;
	unsigned char b = vm->Stack[vm->stacktop-1].uc;
	vm->Stack[vm->stacktop-2].uc = a - b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_SUB_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	short a = vm->Stack[vm->stacktop-2].s;
	short b = vm->Stack[vm->stacktop-1].s;
	vm->Stack[vm->stacktop-2].s = a - b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_SUB_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	unsigned short a = vm->Stack[vm->stacktop-2].us;
	unsigned short b = vm->Stack[vm->stacktop-1].us;
	vm->Stack[vm->stacktop-2].us = a - b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_SUB_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	int a = vm->Stack[vm->stacktop-2].i;
	int b = vm->Stack[vm->stacktop-1].i;
	vm->Stack[vm->stacktop-2].i = a - b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_SUB_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	unsigned int a = vm->Stack[vm->stacktop-2].ui;
	unsigned int b = vm->Stack[vm->stacktop-1].ui;
	vm->Stack[vm->stacktop-2].ui = a - b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_SUB_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	INT64 a = vm->Stack[vm->stacktop-2].ll;
	INT64 b = vm->Stack[vm->stacktop-1].ll;
	vm->Stack[vm->stacktop-2].ll = a - b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_SUB_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	UINT64 a = vm->Stack[vm->stacktop-2].ull;
	UINT64 b = vm->Stack[vm->stacktop-1].ull;
	vm->Stack[vm->stacktop-2].ull = a - b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_SUB_FL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	float a = vm->Stack[vm->stacktop-2].f;
	float b = vm->Stack[vm->stacktop-1].f;
	vm->Stack[vm->stacktop-2].f = a - b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_SUB_DB(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	double a = vm->Stack[vm->stacktop-2].d;
	double b = vm->Stack[vm->stacktop-1].d;
	vm->Stack[vm->stacktop-2].d = a - b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_SUB_LD(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	long double a = vm->Stack[vm->stacktop-2].ld;
	long double b = vm->Stack[vm->stacktop-1].ld;
	vm->Stack[vm->stacktop-2].ld = a - b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_LSHIFT_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	char a = vm->Stack[vm->stacktop-2].c;
	char b = vm->Stack[vm->stacktop-1].c;
	vm->Stack[vm->stacktop-2].c = a << b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_LSHIFT_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	unsigned char a = vm->Stack[vm->stacktop-2].uc;
	unsigned char b = vm->Stack[vm->stacktop-1].uc;
	vm->Stack[vm->stacktop-2].uc = a << b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_LSHIFT_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	short a = vm->Stack[vm->stacktop-2].s;
	short b = vm->Stack[vm->stacktop-1].s;
	vm->Stack[vm->stacktop-2].s = a << b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_LSHIFT_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	unsigned short a = vm->Stack[vm->stacktop-2].us;
	unsigned short b = vm->Stack[vm->stacktop-1].us;
	vm->Stack[vm->stacktop-2].us = a << b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_LSHIFT_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	int a = vm->Stack[vm->stacktop-2].i;
	int b = vm->Stack[vm->stacktop-1].i;
	vm->Stack[vm->stacktop-2].i = a << b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_LSHIFT_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	unsigned int a = vm->Stack[vm->stacktop-2].ui;
	unsigned int b = vm->Stack[vm->stacktop-1].ui;
	vm->Stack[vm->stacktop-2].ui = a << b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_LSHIFT_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	INT64 a = vm->Stack[vm->stacktop-2].ll;
	INT64 b = vm->Stack[vm->stacktop-1].ll;
	vm->Stack[vm->stacktop-2].ll = a << b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_LSHIFT_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	UINT64 a = vm->Stack[vm->stacktop-2].ull;
	UINT64 b = vm->Stack[vm->stacktop-1].ull;
	vm->Stack[vm->stacktop-2].ull = a << b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_RSHIFT_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	char a = vm->Stack[vm->stacktop-2].c;
	char b = vm->Stack[vm->stacktop-1].c;
	vm->Stack[vm->stacktop-2].c = a >> b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_RSHIFT_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	unsigned char a = vm->Stack[vm->stacktop-2].uc;
	unsigned char b = vm->Stack[vm->stacktop-1].uc;
	vm->Stack[vm->stacktop-2].uc = a >> b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_RSHIFT_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	short a = vm->Stack[vm->stacktop-2].s;
	short b = vm->Stack[vm->stacktop-1].s;
	vm->Stack[vm->stacktop-2].s = a >> b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_RSHIFT_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	unsigned short a = vm->Stack[vm->stacktop-2].us;
	unsigned short b = vm->Stack[vm->stacktop-1].us;
	vm->Stack[vm->stacktop-2].us = a >> b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_RSHIFT_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	int a = vm->Stack[vm->stacktop-2].i;
	int b = vm->Stack[vm->stacktop-1].i;
	vm->Stack[vm->stacktop-2].i = a >> b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_RSHIFT_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	unsigned int a = vm->Stack[vm->stacktop-2].ui;
	unsigned int b = vm->Stack[vm->stacktop-1].ui;
	vm->Stack[vm->stacktop-2].ui = a >> b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_RSHIFT_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	INT64 a = vm->Stack[vm->stacktop-2].ll;
	INT64 b = vm->Stack[vm->stacktop-1].ll;
	vm->Stack[vm->stacktop-2].ll = a >> b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_RSHIFT_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	UINT64 a = vm->Stack[vm->stacktop-2].ull;
	UINT64 b = vm->Stack[vm->stacktop-1].ull;
	vm->Stack[vm->stacktop-2].ull = a >> b;
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_LT_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	char a = vm->Stack[vm->stacktop-2].c;
	char b = vm->Stack[vm->stacktop-1].c;
	vm->Stack[vm->stacktop-2].i = (a < b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_LT_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	unsigned char a = vm->Stack[vm->stacktop-2].uc;
	unsigned char b = vm->Stack[vm->stacktop-1].uc;
	vm->Stack[vm->stacktop-2].i = (a < b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_LT_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	short a = vm->Stack[vm->stacktop-2].s;
	short b = vm->Stack[vm->stacktop-1].s;
	vm->Stack[vm->stacktop-2].i = (a < b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_LT_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	unsigned short a = vm->Stack[vm->stacktop-2].us;
	unsigned short b = vm->Stack[vm->stacktop-1].us;
	vm->Stack[vm->stacktop-2].i = (a < b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_LT_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	int a = vm->Stack[vm->stacktop-2].i;
	int b = vm->Stack[vm->stacktop-1].i;
	vm->Stack[vm->stacktop-2].i = (a < b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_LT_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	unsigned int a = vm->Stack[vm->stacktop-2].ui;
	unsigned int b = vm->Stack[vm->stacktop-1].ui;
	vm->Stack[vm->stacktop-2].i = (a < b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_LT_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	INT64 a = vm->Stack[vm->stacktop-2].ll;
	INT64 b = vm->Stack[vm->stacktop-1].ll;
	vm->Stack[vm->stacktop-2].i = (a < b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_LT_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	UINT64 a = vm->Stack[vm->stacktop-2].ull;
	UINT64 b = vm->Stack[vm->stacktop-1].ull;
	vm->Stack[vm->stacktop-2].i = (a < b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_LT_FL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	float a = vm->Stack[vm->stacktop-2].f;
	float b = vm->Stack[vm->stacktop-1].f;
	vm->Stack[vm->stacktop-2].i = (a < b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_LT_DB(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	double a = vm->Stack[vm->stacktop-2].d;
	double b = vm->Stack[vm->stacktop-1].d;
	vm->Stack[vm->stacktop-2].i = (a < b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_LT_LD(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	long double a = vm->Stack[vm->stacktop-2].ld;
	long double b = vm->Stack[vm->stacktop-1].ld;
	vm->Stack[vm->stacktop-2].i = (a < b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_GT_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	char a = vm->Stack[vm->stacktop-2].c;
	char b = vm->Stack[vm->stacktop-1].c;
	vm->Stack[vm->stacktop-2].i = (a > b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_GT_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	unsigned char a = vm->Stack[vm->stacktop-2].uc;
	unsigned char b = vm->Stack[vm->stacktop-1].uc;
	vm->Stack[vm->stacktop-2].i = (a > b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_GT_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	short a = vm->Stack[vm->stacktop-2].s;
	short b = vm->Stack[vm->stacktop-1].s;
	vm->Stack[vm->stacktop-2].i = (a > b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_GT_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	unsigned short a = vm->Stack[vm->stacktop-2].us;
	unsigned short b = vm->Stack[vm->stacktop-1].us;
	vm->Stack[vm->stacktop-2].i = (a > b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_GT_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	int a = vm->Stack[vm->stacktop-2].i;
	int b = vm->Stack[vm->stacktop-1].i;
	vm->Stack[vm->stacktop-2].i = (a > b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_GT_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	unsigned int a = vm->Stack[vm->stacktop-2].ui;
	unsigned int b = vm->Stack[vm->stacktop-1].ui;
	vm->Stack[vm->stacktop-2].i = (a > b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_GT_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	INT64 a = vm->Stack[vm->stacktop-2].ll;
	INT64 b = vm->Stack[vm->stacktop-1].ll;
	vm->Stack[vm->stacktop-2].i = (a > b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_GT_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	UINT64 a = vm->Stack[vm->stacktop-2].ull;
	UINT64 b = vm->Stack[vm->stacktop-1].ull;
	vm->Stack[vm->stacktop-2].i = (a > b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_GT_FL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	float a = vm->Stack[vm->stacktop-2].f;
	float b = vm->Stack[vm->stacktop-1].f;
	vm->Stack[vm->stacktop-2].i = (a > b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_GT_DB(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	double a = vm->Stack[vm->stacktop-2].d;
	double b = vm->Stack[vm->stacktop-1].d;
	vm->Stack[vm->stacktop-2].i = (a > b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_GT_LD(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	long double a = vm->Stack[vm->stacktop-2].ld;
	long double b = vm->Stack[vm->stacktop-1].ld;
	vm->Stack[vm->stacktop-2].i = (a > b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_LTE_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	char a = vm->Stack[vm->stacktop-2].c;
	char b = vm->Stack[vm->stacktop-1].c;
	vm->Stack[vm->stacktop-2].i = (a <= b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_LTE_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	unsigned char a = vm->Stack[vm->stacktop-2].uc;
	unsigned char b = vm->Stack[vm->stacktop-1].uc;
	vm->Stack[vm->stacktop-2].i = (a <= b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_LTE_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	short a = vm->Stack[vm->stacktop-2].s;
	short b = vm->Stack[vm->stacktop-1].s;
	vm->Stack[vm->stacktop-2].i = (a <= b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_LTE_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	unsigned short a = vm->Stack[vm->stacktop-2].us;
	unsigned short b = vm->Stack[vm->stacktop-1].us;
	vm->Stack[vm->stacktop-2].i = (a <= b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_LTE_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	int a = vm->Stack[vm->stacktop-2].i;
	int b = vm->Stack[vm->stacktop-1].i;
	vm->Stack[vm->stacktop-2].i = (a <= b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_LTE_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	unsigned int a = vm->Stack[vm->stacktop-2].ui;
	unsigned int b = vm->Stack[vm->stacktop-1].ui;
	vm->Stack[vm->stacktop-2].i = (a <= b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_LTE_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	INT64 a = vm->Stack[vm->stacktop-2].ll;
	INT64 b = vm->Stack[vm->stacktop-1].ll;
	vm->Stack[vm->stacktop-2].i = (a <= b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_LTE_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	UINT64 a = vm->Stack[vm->stacktop-2].ull;
	UINT64 b = vm->Stack[vm->stacktop-1].ull;
	vm->Stack[vm->stacktop-2].i = (a <= b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_LTE_FL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	float a = vm->Stack[vm->stacktop-2].f;
	float b = vm->Stack[vm->stacktop-1].f;
	vm->Stack[vm->stacktop-2].i = (a <= b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_LTE_DB(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	double a = vm->Stack[vm->stacktop-2].d;
	double b = vm->Stack[vm->stacktop-1].d;
	vm->Stack[vm->stacktop-2].i = (a <= b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_LTE_LD(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	long double a = vm->Stack[vm->stacktop-2].ld;
	long double b = vm->Stack[vm->stacktop-1].ld;
	vm->Stack[vm->stacktop-2].i = (a <= b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_GTE_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	char a = vm->Stack[vm->stacktop-2].c;
	char b = vm->Stack[vm->stacktop-1].c;
	vm->Stack[vm->stacktop-2].i = (a >= b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_GTE_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	unsigned char a = vm->Stack[vm->stacktop-2].uc;
	unsigned char b = vm->Stack[vm->stacktop-1].uc;
	vm->Stack[vm->stacktop-2].i = (a >= b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_GTE_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	short a = vm->Stack[vm->stacktop-2].s;
	short b = vm->Stack[vm->stacktop-1].s;
	vm->Stack[vm->stacktop-2].i = (a >= b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_GTE_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	unsigned short a = vm->Stack[vm->stacktop-2].us;
	unsigned short b = vm->Stack[vm->stacktop-1].us;
	vm->Stack[vm->stacktop-2].i = (a >= b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_GTE_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	int a = vm->Stack[vm->stacktop-2].i;
	int b = vm->Stack[vm->stacktop-1].i;
	vm->Stack[vm->stacktop-2].i = (a >= b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_GTE_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	unsigned int a = vm->Stack[vm->stacktop-2].ui;
	unsigned int b = vm->Stack[vm->stacktop-1].ui;
	vm->Stack[vm->stacktop-2].i = (a >= b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_GTE_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	INT64 a = vm->Stack[vm->stacktop-2].ll;
	INT64 b = vm->Stack[vm->stacktop-1].ll;
	vm->Stack[vm->stacktop-2].i = (a >= b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_GTE_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	UINT64 a = vm->Stack[vm->stacktop-2].ull;
	UINT64 b = vm->Stack[vm->stacktop-1].ull;
	vm->Stack[vm->stacktop-2].i = (a >= b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_GTE_FL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	float a = vm->Stack[vm->stacktop-2].f;
	float b = vm->Stack[vm->stacktop-1].f;
	vm->Stack[vm->stacktop-2].i = (a >= b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_GTE_DB(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	double a = vm->Stack[vm->stacktop-2].d;
	double b = vm->Stack[vm->stacktop-1].d;
	vm->Stack[vm->stacktop-2].i = (a >= b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_GTE_LD(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	long double a = vm->Stack[vm->stacktop-2].ld;
	long double b = vm->Stack[vm->stacktop-1].ld;
	vm->Stack[vm->stacktop-2].i = (a >= b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_EQ_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	char a = vm->Stack[vm->stacktop-2].c;
	char b = vm->Stack[vm->stacktop-1].c;
	vm->Stack[vm->stacktop-2].i = (a == b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_EQ_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	unsigned char a = vm->Stack[vm->stacktop-2].uc;
	unsigned char b = vm->Stack[vm->stacktop-1].uc;
	vm->Stack[vm->stacktop-2].i = (a == b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_EQ_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	short a = vm->Stack[vm->stacktop-2].s;
	short b = vm->Stack[vm->stacktop-1].s;
	vm->Stack[vm->stacktop-2].i = (a == b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_EQ_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	unsigned short a = vm->Stack[vm->stacktop-2].us;
	unsigned short b = vm->Stack[vm->stacktop-1].us;
	vm->Stack[vm->stacktop-2].i = (a == b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_EQ_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	int a = vm->Stack[vm->stacktop-2].i;
	int b = vm->Stack[vm->stacktop-1].i;
	vm->Stack[vm->stacktop-2].i = (a == b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_EQ_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	unsigned int a = vm->Stack[vm->stacktop-2].ui;
	unsigned int b = vm->Stack[vm->stacktop-1].ui;
	vm->Stack[vm->stacktop-2].i = (a == b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_EQ_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	INT64 a = vm->Stack[vm->stacktop-2].ll;
	INT64 b = vm->Stack[vm->stacktop-1].ll;
	vm->Stack[vm->stacktop-2].i = (a == b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_EQ_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	UINT64 a = vm->Stack[vm->stacktop-2].ull;
	UINT64 b = vm->Stack[vm->stacktop-1].ull;
	vm->Stack[vm->stacktop-2].i = (a == b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_EQ_FL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	float a = vm->Stack[vm->stacktop-2].f;
	float b = vm->Stack[vm->stacktop-1].f;
	vm->Stack[vm->stacktop-2].i = (a == b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_EQ_DB(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	double a = vm->Stack[vm->stacktop-2].d;
	double b = vm->Stack[vm->stacktop-1].d;
	vm->Stack[vm->stacktop-2].i = (a == b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_EQ_LD(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	long double a = vm->Stack[vm->stacktop-2].ld;
	long double b = vm->Stack[vm->stacktop-1].ld;
	vm->Stack[vm->stacktop-2].i = (a == b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_NE_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	char a = vm->Stack[vm->stacktop-2].c;
	char b = vm->Stack[vm->stacktop-1].c;
	vm->Stack[vm->stacktop-2].i = (a != b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_NE_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	unsigned char a = vm->Stack[vm->stacktop-2].uc;
	unsigned char b = vm->Stack[vm->stacktop-1].uc;
	vm->Stack[vm->stacktop-2].i = (a != b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_NE_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	short a = vm->Stack[vm->stacktop-2].s;
	short b = vm->Stack[vm->stacktop-1].s;
	vm->Stack[vm->stacktop-2].i = (a != b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_NE_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	unsigned short a = vm->Stack[vm->stacktop-2].us;
	unsigned short b = vm->Stack[vm->stacktop-1].us;
	vm->Stack[vm->stacktop-2].i = (a != b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_NE_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	int a = vm->Stack[vm->stacktop-2].i;
	int b = vm->Stack[vm->stacktop-1].i;
	vm->Stack[vm->stacktop-2].i = (a != b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_NE_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	unsigned int a = vm->Stack[vm->stacktop-2].ui;
	unsigned int b = vm->Stack[vm->stacktop-1].ui;
	vm->Stack[vm->stacktop-2].i = (a != b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_NE_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	INT64 a = vm->Stack[vm->stacktop-2].ll;
	INT64 b = vm->Stack[vm->stacktop-1].ll;
	vm->Stack[vm->stacktop-2].i = (a != b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_NE_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	UINT64 a = vm->Stack[vm->stacktop-2].ull;
	UINT64 b = vm->Stack[vm->stacktop-1].ull;
	vm->Stack[vm->stacktop-2].i = (a != b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_NE_FL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	float a = vm->Stack[vm->stacktop-2].f;
	float b = vm->Stack[vm->stacktop-1].f;
	vm->Stack[vm->stacktop-2].i = (a != b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_NE_DB(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	double a = vm->Stack[vm->stacktop-2].d;
	double b = vm->Stack[vm->stacktop-1].d;
	vm->Stack[vm->stacktop-2].i = (a != b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_NE_LD(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	long double a = vm->Stack[vm->stacktop-2].ld;
	long double b = vm->Stack[vm->stacktop-1].ld;
	vm->Stack[vm->stacktop-2].i = (a != b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_BAND_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	char a = vm->Stack[vm->stacktop-2].c;
	char b = vm->Stack[vm->stacktop-1].c;
	vm->Stack[vm->stacktop-2].c = (a & b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_BAND_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	unsigned char a = vm->Stack[vm->stacktop-2].uc;
	unsigned char b = vm->Stack[vm->stacktop-1].uc;
	vm->Stack[vm->stacktop-2].uc = (a & b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_BAND_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	short a = vm->Stack[vm->stacktop-2].s;
	short b = vm->Stack[vm->stacktop-1].s;
	vm->Stack[vm->stacktop-2].s = (a & b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_BAND_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	unsigned short a = vm->Stack[vm->stacktop-2].us;
	unsigned short b = vm->Stack[vm->stacktop-1].us;
	vm->Stack[vm->stacktop-2].us = (a & b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_BAND_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	int a = vm->Stack[vm->stacktop-2].i;
	int b = vm->Stack[vm->stacktop-1].i;
	vm->Stack[vm->stacktop-2].i = (a & b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_BAND_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	unsigned int a = vm->Stack[vm->stacktop-2].ui;
	unsigned int b = vm->Stack[vm->stacktop-1].ui;
	vm->Stack[vm->stacktop-2].ui = (a & b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_BAND_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	INT64 a = vm->Stack[vm->stacktop-2].ll;
	INT64 b = vm->Stack[vm->stacktop-1].ll;
	vm->Stack[vm->stacktop-2].ll = (a & b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_BAND_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	UINT64 a = vm->Stack[vm->stacktop-2].ull;
	UINT64 b = vm->Stack[vm->stacktop-1].ull;
	vm->Stack[vm->stacktop-2].ull = (a & b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_BXOR_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	char a = vm->Stack[vm->stacktop-2].c;
	char b = vm->Stack[vm->stacktop-1].c;
	vm->Stack[vm->stacktop-2].c = (a ^ b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_BXOR_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	unsigned char a = vm->Stack[vm->stacktop-2].uc;
	unsigned char b = vm->Stack[vm->stacktop-1].uc;
	vm->Stack[vm->stacktop-2].uc = (a ^ b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_BXOR_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	short a = vm->Stack[vm->stacktop-2].s;
	short b = vm->Stack[vm->stacktop-1].s;
	vm->Stack[vm->stacktop-2].s = (a ^ b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_BXOR_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	unsigned short a = vm->Stack[vm->stacktop-2].us;
	unsigned short b = vm->Stack[vm->stacktop-1].us;
	vm->Stack[vm->stacktop-2].us = (a ^ b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_BXOR_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	int a = vm->Stack[vm->stacktop-2].i;
	int b = vm->Stack[vm->stacktop-1].i;
	vm->Stack[vm->stacktop-2].i = (a ^ b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_BXOR_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	unsigned int a = vm->Stack[vm->stacktop-2].ui;
	unsigned int b = vm->Stack[vm->stacktop-1].ui;
	vm->Stack[vm->stacktop-2].ui = (a ^ b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_BXOR_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	INT64 a = vm->Stack[vm->stacktop-2].ll;
	INT64 b = vm->Stack[vm->stacktop-1].ll;
	vm->Stack[vm->stacktop-2].ll = (a ^ b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_BXOR_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	UINT64 a = vm->Stack[vm->stacktop-2].ull;
	UINT64 b = vm->Stack[vm->stacktop-1].ull;
	vm->Stack[vm->stacktop-2].ull = (a ^ b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_BOR_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	char a = vm->Stack[vm->stacktop-2].c;
	char b = vm->Stack[vm->stacktop-1].c;
	vm->Stack[vm->stacktop-2].c = (a | b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_BOR_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	unsigned char a = vm->Stack[vm->stacktop-2].uc;
	unsigned char b = vm->Stack[vm->stacktop-1].uc;
	vm->Stack[vm->stacktop-2].uc = (a | b);
	vm->stacktop--;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_BOR_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	short a = vm->Stack[vm->stacktop-2].s;
	short b = vm->Stack[vm->stacktop-1].s;
	vm->Stack[vm->stacktop-2].s = (a | b);
	vm->stacktop--;
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_BOR_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	unsigned short a = vm->Stack[vm->stacktop-2].us;
	unsigned short b = vm->Stack[vm->stacktop-1].us;
	vm->Stack[vm->stacktop-2].us = (a | b);
	vm->stacktop--;
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_BOR_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	int a = vm->Stack[vm->stacktop-2].i;
	int b = vm->Stack[vm->stacktop-1].i;
	vm->Stack[vm->stacktop-2].i = (a | b);
	vm->stacktop--;
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_BOR_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	unsigned int a = vm->Stack[vm->stacktop-2].ui;
	unsigned int b = vm->Stack[vm->stacktop-1].ui;
	vm->Stack[vm->stacktop-2].ui = (a | b);
	vm->stacktop--;
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_BOR_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	INT64 a = vm->Stack[vm->stacktop-2].ll;
	INT64 b = vm->Stack[vm->stacktop-1].ll;
	vm->Stack[vm->stacktop-2].ll = (a | b);
	vm->stacktop--;
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_BOR_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 2);
	UINT64 a = vm->Stack[vm->stacktop-2].ull;
	UINT64 b = vm->Stack[vm->stacktop-1].ull;
	vm->Stack[vm->stacktop-2].ull = (a | b);
	vm->stacktop--;
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_LANDJMP_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	char a = vm->Stack[vm->stacktop-1].c;
	if (!a)
	{ 
		vm->Stack[vm->stacktop-1].i = 0;
		unsigned int offset; 
		ASSERT_RETZERO(ByteStreamGetDword(&str, offset)); 
		str.cur = offset; 
		ScriptExecTrace("landjmp  offset = %u  type = char\n", offset);
	}
	else 
	{ 
		vm->stacktop--;
		str.cur += sizeof(int);
		ScriptExecTrace("landjmp  offset = ---  type = char\n");
	}
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_LANDJMP_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned char a = vm->Stack[vm->stacktop-1].uc;
	if (!a)
	{ 
		vm->Stack[vm->stacktop-1].i = 0;
		unsigned int offset; 
		ASSERT_RETZERO(ByteStreamGetDword(&str, offset)); 
		str.cur = offset; 
		ScriptExecTrace("landjmp  offset = %u  type = unsigned char\n", offset);
	}
	else 
	{ 
		vm->stacktop--;
		str.cur += sizeof(int);
		ScriptExecTrace("landjmp  offset = ---  type = unsigned char\n");
	}
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_LANDJMP_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	short a = vm->Stack[vm->stacktop-1].s;
	if (!a)
	{ 
		vm->Stack[vm->stacktop-1].i = 0;
		unsigned int offset; 
		ASSERT_RETZERO(ByteStreamGetDword(&str, offset)); 
		str.cur = offset; 
		ScriptExecTrace("landjmp  offset = %u  type = short\n", offset);
	}
	else 
	{ 
		vm->stacktop--;
		str.cur += sizeof(int);
		ScriptExecTrace("landjmp  offset = ---  type = short\n");
	}
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_LANDJMP_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned short a = vm->Stack[vm->stacktop-1].us;
	if (!a)
	{ 
		vm->Stack[vm->stacktop-1].i = 0;
		unsigned int offset; 
		ASSERT_RETZERO(ByteStreamGetDword(&str, offset)); 
		str.cur = offset; 
		ScriptExecTrace("landjmp  offset = %u  type = unsigned short\n", offset);
	}
	else 
	{ 
		vm->stacktop--;
		str.cur += sizeof(int);
		ScriptExecTrace("landjmp  offset = ---  type = unsigned short\n");
	}
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_LANDJMP_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	int a = vm->Stack[vm->stacktop-1].i;
	if (!a)
	{ 
		vm->Stack[vm->stacktop-1].i = 0;
		unsigned int offset; 
		ASSERT_RETZERO(ByteStreamGetDword(&str, offset)); 
		str.cur = offset; 
		ScriptExecTrace("landjmp  offset = %u  type = int\n", offset);
	}
	else 
	{ 
		vm->stacktop--;
		str.cur += sizeof(int);
		ScriptExecTrace("landjmp  offset = ---  type = int\n");
	}
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_LANDJMP_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned int a = vm->Stack[vm->stacktop-1].ui;
	if (!a)
	{ 
		vm->Stack[vm->stacktop-1].i = 0;
		unsigned int offset; 
		ASSERT_RETZERO(ByteStreamGetDword(&str, offset)); 
		str.cur = offset; 
		ScriptExecTrace("landjmp  offset = %u  type = unsigned int\n", offset);
	}
	else 
	{ 
		vm->stacktop--;
		str.cur += sizeof(int);
		ScriptExecTrace("landjmp  offset = ---  type = unsigned int\n");
	}
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_LANDJMP_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	INT64 a = vm->Stack[vm->stacktop-1].ll;
	if (!a)
	{ 
		vm->Stack[vm->stacktop-1].i = 0;
		unsigned int offset; 
		ASSERT_RETZERO(ByteStreamGetDword(&str, offset)); 
		str.cur = offset; 
		ScriptExecTrace("landjmp  offset = %u  type = INT64\n", offset);
	}
	else 
	{ 
		vm->stacktop--;
		str.cur += sizeof(int);
		ScriptExecTrace("landjmp  offset = ---  type = INT64\n");
	}
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_LANDJMP_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	UINT64 a = vm->Stack[vm->stacktop-1].ull;
	if (!a)
	{ 
		vm->Stack[vm->stacktop-1].i = 0;
		unsigned int offset; 
		ASSERT_RETZERO(ByteStreamGetDword(&str, offset)); 
		str.cur = offset; 
		ScriptExecTrace("landjmp  offset = %u  type = UINT64\n", offset);
	}
	else 
	{ 
		vm->stacktop--;
		str.cur += sizeof(int);
		ScriptExecTrace("landjmp  offset = ---  type = UINT64\n");
	}
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_LANDJMP_FL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	float a = vm->Stack[vm->stacktop-1].f;
	if (!a)
	{ 
		vm->Stack[vm->stacktop-1].i = 0;
		unsigned int offset; 
		ASSERT_RETZERO(ByteStreamGetDword(&str, offset)); 
		str.cur = offset; 
		ScriptExecTrace("landjmp  offset = %u  type = float\n", offset);
	}
	else 
	{ 
		vm->stacktop--;
		str.cur += sizeof(int);
		ScriptExecTrace("landjmp  offset = ---  type = float\n");
	}
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_LANDJMP_DB(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	double a = vm->Stack[vm->stacktop-1].d;
	if (!a)
	{ 
		vm->Stack[vm->stacktop-1].i = 0;
		unsigned int offset; 
		ASSERT_RETZERO(ByteStreamGetDword(&str, offset)); 
		str.cur = offset; 
		ScriptExecTrace("landjmp  offset = %u  type = double\n", offset);
	}
	else 
	{ 
		vm->stacktop--;
		str.cur += sizeof(int);
		ScriptExecTrace("landjmp  offset = ---  type = double\n");
	}
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_LANDJMP_LD(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	long double a = vm->Stack[vm->stacktop-1].ld;
	if (!a)
	{ 
		vm->Stack[vm->stacktop-1].i = 0;
		unsigned int offset; 
		ASSERT_RETZERO(ByteStreamGetDword(&str, offset)); 
		str.cur = offset; 
		ScriptExecTrace("landjmp  offset = %u  type = long double\n", offset);
	}
	else 
	{ 
		vm->stacktop--;
		str.cur += sizeof(int);
		ScriptExecTrace("landjmp  offset = ---  type = long double\n");
	}
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_LORJMP_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	char a = vm->Stack[vm->stacktop-1].c;
	if (a)
	{ 
		vm->Stack[vm->stacktop-1].i = 1;
		unsigned int offset; 
		ASSERT_RETZERO(ByteStreamGetDword(&str, offset)); 
		str.cur = offset; 
		ScriptExecTrace("lorjmp  offset = %u  type = char\n", offset);
	}
	else 
	{ 
		vm->stacktop--;
		str.cur += sizeof(int);
		ScriptExecTrace("lorjmp  offset = ---  type = char\n");
	}
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_LORJMP_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned char a = vm->Stack[vm->stacktop-1].uc;
	if (a)
	{ 
		vm->Stack[vm->stacktop-1].i = 1;
		unsigned int offset; 
		ASSERT_RETZERO(ByteStreamGetDword(&str, offset)); 
		str.cur = offset; 
		ScriptExecTrace("lorjmp  offset = %u  type = unsigned char\n", offset);
	}
	else 
	{ 
		vm->stacktop--;
		str.cur += sizeof(int);
		ScriptExecTrace("lorjmp  offset = ---  type = unsigned char\n");
	}
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_LORJMP_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	short a = vm->Stack[vm->stacktop-1].s;
	if (a)
	{ 
		vm->Stack[vm->stacktop-1].i = 1;
		unsigned int offset; 
		ASSERT_RETZERO(ByteStreamGetDword(&str, offset)); 
		str.cur = offset; 
		ScriptExecTrace("lorjmp  offset = %u  type = short\n", offset);
	}
	else 
	{ 
		vm->stacktop--;
		str.cur += sizeof(int);
		ScriptExecTrace("lorjmp  offset = ---  type = short\n");
	}
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_LORJMP_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned short a = vm->Stack[vm->stacktop-1].us;
	if (a)
	{ 
		vm->Stack[vm->stacktop-1].i = 1;
		unsigned int offset; 
		ASSERT_RETZERO(ByteStreamGetDword(&str, offset)); 
		str.cur = offset; 
		ScriptExecTrace("lorjmp  offset = %u  type = unsigned short\n", offset);
	}
	else 
	{ 
		vm->stacktop--;
		str.cur += sizeof(int);
		ScriptExecTrace("lorjmp  offset = ---  type = unsigned short\n");
	}
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_LORJMP_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	int a = vm->Stack[vm->stacktop-1].i;
	if (a)
	{ 
		vm->Stack[vm->stacktop-1].i = 1;
		unsigned int offset; 
		ASSERT_RETZERO(ByteStreamGetDword(&str, offset)); 
		str.cur = offset; 
		ScriptExecTrace("lorjmp  offset = %u  type = int\n", offset);
	}
	else 
	{ 
		vm->stacktop--;
		str.cur += sizeof(int);
		ScriptExecTrace("lorjmp  offset = ---  type = int\n");
	}
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_LORJMP_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned int a = vm->Stack[vm->stacktop-1].ui;
	if (a)
	{ 
		vm->Stack[vm->stacktop-1].i = 1;
		unsigned int offset; 
		ASSERT_RETZERO(ByteStreamGetDword(&str, offset)); 
		str.cur = offset; 
		ScriptExecTrace("lorjmp  offset = %u  type = unsigned int\n", offset);
	}
	else 
	{ 
		vm->stacktop--;
		str.cur += sizeof(int);
		ScriptExecTrace("lorjmp  offset = ---  type = unsigned int\n");
	}
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_LORJMP_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	INT64 a = vm->Stack[vm->stacktop-1].ll;
	if (a)
	{ 
		vm->Stack[vm->stacktop-1].i = 1;
		unsigned int offset; 
		ASSERT_RETZERO(ByteStreamGetDword(&str, offset)); 
		str.cur = offset; 
		ScriptExecTrace("lorjmp  offset = %u  type = INT64\n", offset);
	}
	else 
	{ 
		vm->stacktop--;
		str.cur += sizeof(int);
		ScriptExecTrace("lorjmp  offset = ---  type = INT64\n");
	}
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_LORJMP_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	UINT64 a = vm->Stack[vm->stacktop-1].ull;
	if (a)
	{ 
		vm->Stack[vm->stacktop-1].i = 1;
		unsigned int offset; 
		ASSERT_RETZERO(ByteStreamGetDword(&str, offset)); 
		str.cur = offset; 
		ScriptExecTrace("lorjmp  offset = %u  type = UINT64\n", offset);
	}
	else 
	{ 
		vm->stacktop--;
		str.cur += sizeof(int);
		ScriptExecTrace("lorjmp  offset = ---  type = UINT64\n");
	}
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_LORJMP_FL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	float a = vm->Stack[vm->stacktop-1].f;
	if (a)
	{ 
		vm->Stack[vm->stacktop-1].i = 1;
		unsigned int offset; 
		ASSERT_RETZERO(ByteStreamGetDword(&str, offset)); 
		str.cur = offset; 
		ScriptExecTrace("lorjmp  offset = %u  type = float\n", offset);
	}
	else 
	{ 
		vm->stacktop--;
		str.cur += sizeof(int);
		ScriptExecTrace("lorjmp  offset = ---  type = float\n");
	}
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_LORJMP_DB(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	double a = vm->Stack[vm->stacktop-1].d;
	if (a)
	{ 
		vm->Stack[vm->stacktop-1].i = 1;
		unsigned int offset; 
		ASSERT_RETZERO(ByteStreamGetDword(&str, offset)); 
		str.cur = offset; 
		ScriptExecTrace("lorjmp  offset = %u  type = double\n", offset);
	}
	else 
	{ 
		vm->stacktop--;
		str.cur += sizeof(int);
		ScriptExecTrace("lorjmp  offset = ---  type = double\n");
	}
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_LORJMP_LD(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	long double a = vm->Stack[vm->stacktop-1].ld;
	if (a)
	{ 
		vm->Stack[vm->stacktop-1].i = 1;
		unsigned int offset; 
		ASSERT_RETZERO(ByteStreamGetDword(&str, offset)); 
		str.cur = offset; 
		ScriptExecTrace("lorjmp  offset = %u  type = long double\n", offset);
	}
	else 
	{ 
		vm->stacktop--;
		str.cur += sizeof(int);
		ScriptExecTrace("lorjmp  offset = ---  type = long double\n");
	}
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_LOGEND_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	char a = vm->Stack[vm->stacktop-1].c;
	vm->Stack[vm->stacktop-1].i = (a ? 1 : 0);
	ScriptExecTrace("logend  type = char\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_LOGEND_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned char a = vm->Stack[vm->stacktop-1].uc;
	vm->Stack[vm->stacktop-1].i = (a ? 1 : 0);
	ScriptExecTrace("logend  type = unsigned char\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_LOGEND_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	short a = vm->Stack[vm->stacktop-1].s;
	vm->Stack[vm->stacktop-1].i = (a ? 1 : 0);
	ScriptExecTrace("logend  type = short\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_LOGEND_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned short a = vm->Stack[vm->stacktop-1].us;
	vm->Stack[vm->stacktop-1].i = (a ? 1 : 0);
	ScriptExecTrace("logend  type = unsigned short\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_LOGEND_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	int a = vm->Stack[vm->stacktop-1].i;
	vm->Stack[vm->stacktop-1].i = (a ? 1 : 0);
	ScriptExecTrace("logend  type = int\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_LOGEND_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned int a = vm->Stack[vm->stacktop-1].ui;
	vm->Stack[vm->stacktop-1].i = (a ? 1 : 0);
	ScriptExecTrace("logend  type = unsigned int\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_LOGEND_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	INT64 a = vm->Stack[vm->stacktop-1].ll;
	vm->Stack[vm->stacktop-1].i = (a ? 1 : 0);
	ScriptExecTrace("logend  type = INT64\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_LOGEND_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	UINT64 a = vm->Stack[vm->stacktop-1].ull;
	vm->Stack[vm->stacktop-1].i = (a ? 1 : 0);
	ScriptExecTrace("logend  type = UINT64\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_LOGEND_FL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	float a = vm->Stack[vm->stacktop-1].f;
	vm->Stack[vm->stacktop-1].i = (a ? 1 : 0);
	ScriptExecTrace("logend  type = float\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_LOGEND_DB(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	double a = vm->Stack[vm->stacktop-1].d;
	vm->Stack[vm->stacktop-1].i = (a ? 1 : 0);
	ScriptExecTrace("logend  type = double\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_LOGEND_LD(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	long double a = vm->Stack[vm->stacktop-1].ld;
	vm->Stack[vm->stacktop-1].i = (a ? 1 : 0);
	ScriptExecTrace("logend  type = long double\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_CH_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	char a = vm->Stack[vm->stacktop-1].c;
	vm->Stack[vm->stacktop-1].c = (char)a;
	ScriptExecTrace("typecast char to char\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_CH_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned char a = vm->Stack[vm->stacktop-1].uc;
	vm->Stack[vm->stacktop-1].c = (char)a;
	ScriptExecTrace("typecast unsigned char to char\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_CH_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	short a = vm->Stack[vm->stacktop-1].s;
	vm->Stack[vm->stacktop-1].c = (char)a;
	ScriptExecTrace("typecast short to char\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_CH_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned short a = vm->Stack[vm->stacktop-1].us;
	vm->Stack[vm->stacktop-1].c = (char)a;
	ScriptExecTrace("typecast unsigned short to char\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_CH_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	int a = vm->Stack[vm->stacktop-1].i;
	vm->Stack[vm->stacktop-1].c = (char)a;
	ScriptExecTrace("typecast int to char\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_CH_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned int a = vm->Stack[vm->stacktop-1].ui;
	vm->Stack[vm->stacktop-1].c = (char)a;
	ScriptExecTrace("typecast int to char\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_CH_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	INT64 a = vm->Stack[vm->stacktop-1].ll;
	vm->Stack[vm->stacktop-1].c = (char)a;
	ScriptExecTrace("typecast INT64 to char\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_CH_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	UINT64 a = vm->Stack[vm->stacktop-1].ull;
	vm->Stack[vm->stacktop-1].c = (char)a;
	ScriptExecTrace("typecast UINT64 to char\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_CH_FL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	float a = vm->Stack[vm->stacktop-1].f;
	vm->Stack[vm->stacktop-1].c = (char)a;
	ScriptExecTrace("typecast float to char\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_CH_DB(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	double a = vm->Stack[vm->stacktop-1].d;
	vm->Stack[vm->stacktop-1].c = (char)a;
	ScriptExecTrace("typecast double to char\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_CH_LD(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	long double a = vm->Stack[vm->stacktop-1].ld;
	vm->Stack[vm->stacktop-1].c = (char)a;
	ScriptExecTrace("typecast long double to char\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_UC_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	char a = vm->Stack[vm->stacktop-1].c;
	vm->Stack[vm->stacktop-1].uc = (unsigned char)a;
	ScriptExecTrace("typecast char to unsigned char\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_UC_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned char a = vm->Stack[vm->stacktop-1].uc;
	vm->Stack[vm->stacktop-1].uc = (unsigned char)a;
	ScriptExecTrace("typecast unsigned char to unsigned char\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_UC_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	short a = vm->Stack[vm->stacktop-1].s;
	vm->Stack[vm->stacktop-1].uc = (unsigned char)a;
	ScriptExecTrace("typecast short to unsigned char\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_UC_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned short a = vm->Stack[vm->stacktop-1].us;
	vm->Stack[vm->stacktop-1].uc = (unsigned char)a;
	ScriptExecTrace("typecast unsigned short to unsigned char\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_UC_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	int a = vm->Stack[vm->stacktop-1].i;
	vm->Stack[vm->stacktop-1].uc = (unsigned char)a;
	ScriptExecTrace("typecast int to unsigned char\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_UC_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned int a = vm->Stack[vm->stacktop-1].ui;
	vm->Stack[vm->stacktop-1].uc = (unsigned char)a;
	ScriptExecTrace("typecast int to unsigned char\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_UC_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	INT64 a = vm->Stack[vm->stacktop-1].ll;
	vm->Stack[vm->stacktop-1].uc = (unsigned char)a;
	ScriptExecTrace("typecast INT64 to unsigned char\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_UC_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	UINT64 a = vm->Stack[vm->stacktop-1].ull;
	vm->Stack[vm->stacktop-1].uc = (unsigned char)a;
	ScriptExecTrace("typecast UINT64 to unsigned char\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_UC_FL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	float a = vm->Stack[vm->stacktop-1].f;
	vm->Stack[vm->stacktop-1].uc = (unsigned char)a;
	ScriptExecTrace("typecast float to unsigned char\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_UC_DB(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	double a = vm->Stack[vm->stacktop-1].d;
	vm->Stack[vm->stacktop-1].uc = (unsigned char)a;
	ScriptExecTrace("typecast double to unsigned char\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_UC_LD(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	long double a = vm->Stack[vm->stacktop-1].ld;
	vm->Stack[vm->stacktop-1].uc = (unsigned char)a;
	ScriptExecTrace("typecast long double to unsigned char\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_SH_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	char a = vm->Stack[vm->stacktop-1].c;
	vm->Stack[vm->stacktop-1].s = (short)a;
	ScriptExecTrace("typecast char to short\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_SH_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned char a = vm->Stack[vm->stacktop-1].uc;
	vm->Stack[vm->stacktop-1].s = (short)a;
	ScriptExecTrace("typecast unsigned char to short\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_SH_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	short a = vm->Stack[vm->stacktop-1].s;
	vm->Stack[vm->stacktop-1].s = (short)a;
	ScriptExecTrace("typecast short to short\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_SH_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned short a = vm->Stack[vm->stacktop-1].us;
	vm->Stack[vm->stacktop-1].s = (short)a;
	ScriptExecTrace("typecast unsigned short to short\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_SH_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	int a = vm->Stack[vm->stacktop-1].i;
	vm->Stack[vm->stacktop-1].s = (short)a;
	ScriptExecTrace("typecast int to short\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_SH_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned int a = vm->Stack[vm->stacktop-1].ui;
	vm->Stack[vm->stacktop-1].s = (short)a;
	ScriptExecTrace("typecast int to short\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_SH_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	INT64 a = vm->Stack[vm->stacktop-1].ll;
	vm->Stack[vm->stacktop-1].s = (short)a;
	ScriptExecTrace("typecast INT64 to short\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_SH_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	UINT64 a = vm->Stack[vm->stacktop-1].ull;
	vm->Stack[vm->stacktop-1].s = (short)a;
	ScriptExecTrace("typecast UINT64 to short\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_SH_FL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	float a = vm->Stack[vm->stacktop-1].f;
	vm->Stack[vm->stacktop-1].s = (short)a;
	ScriptExecTrace("typecast float to short\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_SH_DB(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	double a = vm->Stack[vm->stacktop-1].d;
	vm->Stack[vm->stacktop-1].s = (short)a;
	ScriptExecTrace("typecast double to short\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_SH_LD(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	long double a = vm->Stack[vm->stacktop-1].ld;
	vm->Stack[vm->stacktop-1].s = (short)a;
	ScriptExecTrace("typecast long double to short\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_US_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	char a = vm->Stack[vm->stacktop-1].c;
	vm->Stack[vm->stacktop-1].us = (unsigned short)a;
	ScriptExecTrace("typecast char to unsigned short\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_US_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned char a = vm->Stack[vm->stacktop-1].uc;
	vm->Stack[vm->stacktop-1].us = (unsigned short)a;
	ScriptExecTrace("typecast unsigned char to unsigned short\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_US_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	short a = vm->Stack[vm->stacktop-1].s;
	vm->Stack[vm->stacktop-1].us = (unsigned short)a;
	ScriptExecTrace("typecast short to unsigned short\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_US_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned short a = vm->Stack[vm->stacktop-1].us;
	vm->Stack[vm->stacktop-1].us = (unsigned short)a;
	ScriptExecTrace("typecast unsigned short to unsigned short\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_US_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	int a = vm->Stack[vm->stacktop-1].i;
	vm->Stack[vm->stacktop-1].us = (unsigned short)a;
	ScriptExecTrace("typecast int to unsigned short\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_US_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned int a = vm->Stack[vm->stacktop-1].ui;
	vm->Stack[vm->stacktop-1].us = (unsigned short)a;
	ScriptExecTrace("typecast int to unsigned short\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_US_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	INT64 a = vm->Stack[vm->stacktop-1].ll;
	vm->Stack[vm->stacktop-1].us = (unsigned short)a;
	ScriptExecTrace("typecast INT64 to unsigned short\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_US_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	UINT64 a = vm->Stack[vm->stacktop-1].ull;
	vm->Stack[vm->stacktop-1].us = (unsigned short)a;
	ScriptExecTrace("typecast UINT64 to unsigned short\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_US_FL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	float a = vm->Stack[vm->stacktop-1].f;
	vm->Stack[vm->stacktop-1].us = (unsigned short)a;
	ScriptExecTrace("typecast float to unsigned short\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_US_DB(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	double a = vm->Stack[vm->stacktop-1].d;
	vm->Stack[vm->stacktop-1].us = (unsigned short)a;
	ScriptExecTrace("typecast double to unsigned short\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_US_LD(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	long double a = vm->Stack[vm->stacktop-1].ld;
	vm->Stack[vm->stacktop-1].us = (unsigned short)a;
	ScriptExecTrace("typecast long double to unsigned short\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_IN_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	char a = vm->Stack[vm->stacktop-1].c;
	vm->Stack[vm->stacktop-1].i = (int)a;
	ScriptExecTrace("typecast char to int\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_IN_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned char a = vm->Stack[vm->stacktop-1].uc;
	vm->Stack[vm->stacktop-1].i = (int)a;
	ScriptExecTrace("typecast unsigned char to int\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_IN_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	short a = vm->Stack[vm->stacktop-1].s;
	vm->Stack[vm->stacktop-1].i = (int)a;
	ScriptExecTrace("typecast short to int\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_IN_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned short a = vm->Stack[vm->stacktop-1].us;
	vm->Stack[vm->stacktop-1].i = (int)a;
	ScriptExecTrace("typecast unsigned short to int\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_IN_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	int a = vm->Stack[vm->stacktop-1].i;
	vm->Stack[vm->stacktop-1].i = (int)a;
	ScriptExecTrace("typecast int to int\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_IN_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned int a = vm->Stack[vm->stacktop-1].ui;
	vm->Stack[vm->stacktop-1].i = (int)a;
	ScriptExecTrace("typecast int to int\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_IN_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	INT64 a = vm->Stack[vm->stacktop-1].ll;
	vm->Stack[vm->stacktop-1].i = (int)a;
	ScriptExecTrace("typecast INT64 to int\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_IN_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	UINT64 a = vm->Stack[vm->stacktop-1].ull;
	vm->Stack[vm->stacktop-1].i = (int)a;
	ScriptExecTrace("typecast UINT64 to int\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_IN_FL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	float a = vm->Stack[vm->stacktop-1].f;
	vm->Stack[vm->stacktop-1].i = (int)a;
	ScriptExecTrace("typecast float to int\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_IN_DB(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	double a = vm->Stack[vm->stacktop-1].d;
	vm->Stack[vm->stacktop-1].i = (int)a;
	ScriptExecTrace("typecast double to int\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_IN_LD(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	long double a = vm->Stack[vm->stacktop-1].ld;
	vm->Stack[vm->stacktop-1].i = (int)a;
	ScriptExecTrace("typecast long double to int\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_UI_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	char a = vm->Stack[vm->stacktop-1].c;
	vm->Stack[vm->stacktop-1].ui = (unsigned int)a;
	ScriptExecTrace("typecast char to unsigned int\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_UI_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned char a = vm->Stack[vm->stacktop-1].uc;
	vm->Stack[vm->stacktop-1].ui = (unsigned int)a;
	ScriptExecTrace("typecast unsigned char to unsigned int\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_UI_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	short a = vm->Stack[vm->stacktop-1].s;
	vm->Stack[vm->stacktop-1].ui = (unsigned int)a;
	ScriptExecTrace("typecast short to unsigned int\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_UI_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned short a = vm->Stack[vm->stacktop-1].us;
	vm->Stack[vm->stacktop-1].ui = (unsigned int)a;
	ScriptExecTrace("typecast unsigned short to unsigned int\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_UI_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	int a = vm->Stack[vm->stacktop-1].i;
	vm->Stack[vm->stacktop-1].ui = (unsigned int)a;
	ScriptExecTrace("typecast int to unsigned int\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_UI_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned int a = vm->Stack[vm->stacktop-1].ui;
	vm->Stack[vm->stacktop-1].ui = (unsigned int)a;
	ScriptExecTrace("typecast int to unsigned int\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_UI_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	INT64 a = vm->Stack[vm->stacktop-1].ll;
	vm->Stack[vm->stacktop-1].ui = (unsigned int)a;
	ScriptExecTrace("typecast INT64 to unsigned int\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_UI_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	UINT64 a = vm->Stack[vm->stacktop-1].ull;
	vm->Stack[vm->stacktop-1].ui = (unsigned int)a;
	ScriptExecTrace("typecast UINT64 to unsigned int\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_UI_FL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	float a = vm->Stack[vm->stacktop-1].f;
	vm->Stack[vm->stacktop-1].ui = (unsigned int)a;
	ScriptExecTrace("typecast float to unsigned int\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_UI_DB(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	double a = vm->Stack[vm->stacktop-1].d;
	vm->Stack[vm->stacktop-1].ui = (unsigned int)a;
	ScriptExecTrace("typecast double to unsigned int\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_UI_LD(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	long double a = vm->Stack[vm->stacktop-1].ld;
	vm->Stack[vm->stacktop-1].ui = (unsigned int)a;
	ScriptExecTrace("typecast long double to unsigned int\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_LL_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	char a = vm->Stack[vm->stacktop-1].c;
	vm->Stack[vm->stacktop-1].ll = (INT64)a;
	ScriptExecTrace("typecast char to INT64\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_LL_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned char a = vm->Stack[vm->stacktop-1].uc;
	vm->Stack[vm->stacktop-1].ll = (INT64)a;
	ScriptExecTrace("typecast unsigned char to INT64\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_LL_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	short a = vm->Stack[vm->stacktop-1].s;
	vm->Stack[vm->stacktop-1].ll = (INT64)a;
	ScriptExecTrace("typecast short to INT64\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_LL_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned short a = vm->Stack[vm->stacktop-1].us;
	vm->Stack[vm->stacktop-1].ll = (INT64)a;
	ScriptExecTrace("typecast unsigned short to INT64\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_LL_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	int a = vm->Stack[vm->stacktop-1].i;
	vm->Stack[vm->stacktop-1].ll = (INT64)a;
	ScriptExecTrace("typecast int to INT64\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_LL_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned int a = vm->Stack[vm->stacktop-1].ui;
	vm->Stack[vm->stacktop-1].ll = (INT64)a;
	ScriptExecTrace("typecast int to INT64\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_LL_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	INT64 a = vm->Stack[vm->stacktop-1].ll;
	vm->Stack[vm->stacktop-1].ll = (INT64)a;
	ScriptExecTrace("typecast INT64 to INT64\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_LL_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	UINT64 a = vm->Stack[vm->stacktop-1].ull;
	vm->Stack[vm->stacktop-1].ll = (INT64)a;
	ScriptExecTrace("typecast UINT64 to INT64\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_LL_FL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	float a = vm->Stack[vm->stacktop-1].f;
	vm->Stack[vm->stacktop-1].ll = (INT64)a;
	ScriptExecTrace("typecast float to INT64\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_LL_DB(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	double a = vm->Stack[vm->stacktop-1].d;
	vm->Stack[vm->stacktop-1].ll = (INT64)a;
	ScriptExecTrace("typecast double to INT64\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_LL_LD(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	long double a = vm->Stack[vm->stacktop-1].ld;
	vm->Stack[vm->stacktop-1].ll = (INT64)a;
	ScriptExecTrace("typecast long double to INT64\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_UL_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	char a = vm->Stack[vm->stacktop-1].c;
	vm->Stack[vm->stacktop-1].ull = (UINT64)a;
	ScriptExecTrace("typecast char to UINT64\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_UL_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned char a = vm->Stack[vm->stacktop-1].uc;
	vm->Stack[vm->stacktop-1].ull = (UINT64)a;
	ScriptExecTrace("typecast unsigned char to UINT64\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_UL_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	short a = vm->Stack[vm->stacktop-1].s;
	vm->Stack[vm->stacktop-1].ull = (UINT64)a;
	ScriptExecTrace("typecast short to UINT64\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_UL_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned short a = vm->Stack[vm->stacktop-1].us;
	vm->Stack[vm->stacktop-1].ull = (UINT64)a;
	ScriptExecTrace("typecast unsigned short to UINT64\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_UL_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	int a = vm->Stack[vm->stacktop-1].i;
	vm->Stack[vm->stacktop-1].ull = (UINT64)a;
	ScriptExecTrace("typecast int to UINT64\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_UL_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned int a = vm->Stack[vm->stacktop-1].ui;
	vm->Stack[vm->stacktop-1].ull = (UINT64)a;
	ScriptExecTrace("typecast int to UINT64\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_UL_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	INT64 a = vm->Stack[vm->stacktop-1].ll;
	vm->Stack[vm->stacktop-1].ull = (UINT64)a;
	ScriptExecTrace("typecast INT64 to UINT64\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_UL_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	UINT64 a = vm->Stack[vm->stacktop-1].ull;
	vm->Stack[vm->stacktop-1].ull = (UINT64)a;
	ScriptExecTrace("typecast UINT64 to UINT64\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_UL_FL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	float a = vm->Stack[vm->stacktop-1].f;
	vm->Stack[vm->stacktop-1].ull = (UINT64)a;
	ScriptExecTrace("typecast float to UINT64\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_UL_DB(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	double a = vm->Stack[vm->stacktop-1].d;
	vm->Stack[vm->stacktop-1].ull = (UINT64)a;
	ScriptExecTrace("typecast double to UINT64\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_UL_LD(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	long double a = vm->Stack[vm->stacktop-1].ld;
	vm->Stack[vm->stacktop-1].ull = (UINT64)a;
	ScriptExecTrace("typecast long double to UINT64\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_FL_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	char a = vm->Stack[vm->stacktop-1].c;
	vm->Stack[vm->stacktop-1].f = (float)a;
	ScriptExecTrace("typecast char to float\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_FL_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned char a = vm->Stack[vm->stacktop-1].uc;
	vm->Stack[vm->stacktop-1].f = (float)a;
	ScriptExecTrace("typecast unsigned char to float\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_FL_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	short a = vm->Stack[vm->stacktop-1].s;
	vm->Stack[vm->stacktop-1].f = (float)a;
	ScriptExecTrace("typecast short to float\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_FL_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned short a = vm->Stack[vm->stacktop-1].us;
	vm->Stack[vm->stacktop-1].f = (float)a;
	ScriptExecTrace("typecast unsigned short to float\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_FL_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	int a = vm->Stack[vm->stacktop-1].i;
	vm->Stack[vm->stacktop-1].f = (float)a;
	ScriptExecTrace("typecast int to float\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_FL_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned int a = vm->Stack[vm->stacktop-1].ui;
	vm->Stack[vm->stacktop-1].f = (float)a;
	ScriptExecTrace("typecast int to float\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_FL_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	INT64 a = vm->Stack[vm->stacktop-1].ll;
	vm->Stack[vm->stacktop-1].f = (float)a;
	ScriptExecTrace("typecast INT64 to float\n");	
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_FL_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	UINT64 a = vm->Stack[vm->stacktop-1].ull;
	vm->Stack[vm->stacktop-1].f = (float)a;
	ScriptExecTrace("typecast UINT64 to float\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_FL_FL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	float a = vm->Stack[vm->stacktop-1].f;
	vm->Stack[vm->stacktop-1].f = (float)a;
	ScriptExecTrace("typecast float to float\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_FL_DB(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	double a = vm->Stack[vm->stacktop-1].d;
	vm->Stack[vm->stacktop-1].f = (float)a;
	ScriptExecTrace("typecast double to float\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_FL_LD(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	long double a = vm->Stack[vm->stacktop-1].ld;
	vm->Stack[vm->stacktop-1].f = (float)a;
	ScriptExecTrace("typecast long double to float\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_DB_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	char a = vm->Stack[vm->stacktop-1].c;
	vm->Stack[vm->stacktop-1].d = (double)a;
	ScriptExecTrace("typecast char to double\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_DB_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned char a = vm->Stack[vm->stacktop-1].uc;
	vm->Stack[vm->stacktop-1].d = (double)a;
	ScriptExecTrace("typecast unsigned char to double\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_DB_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	short a = vm->Stack[vm->stacktop-1].s;
	vm->Stack[vm->stacktop-1].d = (double)a;
	ScriptExecTrace("typecast short to double\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_DB_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned short a = vm->Stack[vm->stacktop-1].us;
	vm->Stack[vm->stacktop-1].d = (double)a;
	ScriptExecTrace("typecast unsigned short to double\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_DB_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	int a = vm->Stack[vm->stacktop-1].i;
	vm->Stack[vm->stacktop-1].d = (double)a;
	ScriptExecTrace("typecast int to double\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_DB_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned int a = vm->Stack[vm->stacktop-1].ui;
	vm->Stack[vm->stacktop-1].d = (double)a;
	ScriptExecTrace("typecast int to double\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_DB_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	INT64 a = vm->Stack[vm->stacktop-1].ll;
	vm->Stack[vm->stacktop-1].d = (double)a;
	ScriptExecTrace("typecast INT64 to double\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_DB_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	UINT64 a = vm->Stack[vm->stacktop-1].ull;
	vm->Stack[vm->stacktop-1].d = (double)a;
	ScriptExecTrace("typecast UINT64 to double\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_DB_FL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	float a = vm->Stack[vm->stacktop-1].f;
	vm->Stack[vm->stacktop-1].d = (double)a;
	ScriptExecTrace("typecast float to double\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_DB_DB(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	double a = vm->Stack[vm->stacktop-1].d;
	vm->Stack[vm->stacktop-1].d = (double)a;
	ScriptExecTrace("typecast double to double\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_DB_LD(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	long double a = vm->Stack[vm->stacktop-1].ld;
	vm->Stack[vm->stacktop-1].d = (double)a;
	ScriptExecTrace("typecast long double to double\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_LD_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	char a = vm->Stack[vm->stacktop-1].c;
	vm->Stack[vm->stacktop-1].ld = (long double)a;
	ScriptExecTrace("typecast char to long double\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_LD_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned char a = vm->Stack[vm->stacktop-1].uc;
	vm->Stack[vm->stacktop-1].ld = (long double)a;
	ScriptExecTrace("typecast unsigned char to long double\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_LD_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	short a = vm->Stack[vm->stacktop-1].s;
	vm->Stack[vm->stacktop-1].ld = (long double)a;
	ScriptExecTrace("typecast short to long double\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_LD_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned short a = vm->Stack[vm->stacktop-1].us;
	vm->Stack[vm->stacktop-1].ld = (long double)a;
	ScriptExecTrace("typecast unsigned short to long double\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_LD_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	int a = vm->Stack[vm->stacktop-1].i;
	vm->Stack[vm->stacktop-1].ld = (long double)a;
	ScriptExecTrace("typecast int to long double\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_LD_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned int a = vm->Stack[vm->stacktop-1].ui;
	vm->Stack[vm->stacktop-1].ld = (long double)a;
	ScriptExecTrace("typecast int to long double\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_LD_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	INT64 a = vm->Stack[vm->stacktop-1].ll;
	vm->Stack[vm->stacktop-1].ld = (long double)a;
	ScriptExecTrace("typecast INT64 to long double\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_LD_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	UINT64 a = vm->Stack[vm->stacktop-1].ull;
	vm->Stack[vm->stacktop-1].ld = (long double)a;
	ScriptExecTrace("typecast UINT64 to long double\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_LD_FL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	float a = vm->Stack[vm->stacktop-1].f;
	vm->Stack[vm->stacktop-1].ld = (long double)a;
	ScriptExecTrace("typecast float to long double\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_LD_DB(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	double a = vm->Stack[vm->stacktop-1].d;
	vm->Stack[vm->stacktop-1].ld = (long double)a;
	ScriptExecTrace("typecast double to long double\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_TYPECAST_TO_LD_LD(
	VMSTATE * vm,
	BYTE_STREAM & str)
{ 
	ASSERT_RETZERO(vm->stacktop >= 1);
	long double a = vm->Stack[vm->stacktop-1].ld;
	vm->Stack[vm->stacktop-1].ld = (long double)a;
	ScriptExecTrace("typecast long double to long double\n");
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_RSTAT(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int stat;
	ASSERT_RETZERO(ByteStreamGetDword(&str, stat));
	const STATS_DATA * stats_data = (const STATS_DATA *)ExcelGetData(NULL, DATATABLE_STATS, STAT_GET_STAT(stat));
	ASSERT_RETZERO(stats_data);
	int value;

	switch (vm->cur_stat_source)
	{
	case STAT_SOURCE_STATS:
		ASSERT(vm->context.stats);
		value = StatsGetStatShift(vm->context.game, vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		ScriptExecTrace("read list stat %s  value = %d\n", stats_data->m_szName, value);
		break;
	case STAT_SOURCE_UNIT:
		ASSERT(vm->context.unit);
		value = UnitGetStatShift(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		ScriptExecTrace("read unit stat %s  value = %d\n", stats_data->m_szName, value);
		break;
	case STAT_SOURCE_OBJECT:
		ASSERT(vm->context.object);
		value = UnitGetStatShift(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		ScriptExecTrace("read unit stat %s  value = %d\n", stats_data->m_szName, value);
		break;
	case STAT_SOURCE_STACK_UNIT:
		{
			value = 0;
			ASSERT_RETZERO(vm->stacktop >= 1);
			UNIT * unit = (UNIT *)(vm->Stack[--vm->stacktop].ptr);
			if (unit)
			{
				value = UnitGetStatShift(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			}
			vm->cur_stat_source = vm->pre_stat_source;
		}
		break;
	case STAT_SOURCE_RIDER:
		ASSERT(vm->context.stats && vm->context.unit);
		value = StatsGetCalculatedStatShift(vm->context.unit, UNITTYPE_NONE, vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		ScriptExecTrace("read rider stat %s  value = %d\n", stats_data->m_szName, value);
		break;
	default:
		value = 0;
		ASSERT(0);
		break;
	}
	vm->Stack[vm->stacktop++].i = value;
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_RSTAT_BASE(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int stat;
	ASSERT_RETZERO(ByteStreamGetDword(&str, stat));
	const STATS_DATA * stats_data = (const STATS_DATA *)ExcelGetData(NULL, DATATABLE_STATS, STAT_GET_STAT(stat));
	ASSERT_RETZERO(stats_data);
	int value;

	switch (vm->cur_stat_source)
	{
	case STAT_SOURCE_STATS:
		ASSERT(vm->context.stats);
		value = StatsGetStatShift(vm->context.game, vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		ScriptExecTrace("read list stat %s  value = %d\n", stats_data->m_szName, value);
		break;
	case STAT_SOURCE_UNIT:
		ASSERT(vm->context.unit);
		value = UnitGetBaseStatShift(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		ScriptExecTrace("read unit base stat %s  value = %d\n", stats_data->m_szName, value);
		break;
	case STAT_SOURCE_OBJECT:
		ASSERT(vm->context.object);
		value = UnitGetBaseStatShift(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		ScriptExecTrace("read unit base stat %s  value = %d\n", stats_data->m_szName, value);
		break;
	case STAT_SOURCE_STACK_UNIT:
		{
			value = 0;
			ASSERT_RETZERO(vm->stacktop >= 1);
			UNIT * unit = (UNIT *)(vm->Stack[--vm->stacktop].ptr);
			if (unit)
			{
				value = UnitGetBaseStatShift(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			}
			vm->cur_stat_source = vm->pre_stat_source;
		}
		break;
	case STAT_SOURCE_RIDER:
		ASSERT(vm->context.stats && vm->context.unit);
		value = StatsGetCalculatedStatShift(vm->context.unit, UNITTYPE_NONE, vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		ScriptExecTrace("read rider stat %s  value = %d\n", stats_data->m_szName, value);
		break;
	default:
		value = 0;
		ASSERT(0);
		break;
	}
	vm->Stack[vm->stacktop++].i = value;
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_RSTAT_MOD(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int stat;
	ASSERT_RETZERO(ByteStreamGetDword(&str, stat));
	const STATS_DATA * stats_data = (const STATS_DATA *)ExcelGetData(NULL, DATATABLE_STATS, STAT_GET_STAT(stat));
	ASSERT_RETZERO(stats_data);
	int value, baseValue;

	switch (vm->cur_stat_source)
	{
	case STAT_SOURCE_STATS:
		ASSERT(vm->context.stats);
		value = StatsGetStatShift(vm->context.game, vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		ScriptExecTrace("read list stat %s  value = %d\n", stats_data->m_szName, value);
		break;
	case STAT_SOURCE_UNIT:
		ASSERT(vm->context.unit);
		baseValue = UnitGetBaseStatShift(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		value = UnitGetStatShift(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat)) - baseValue;
		ScriptExecTrace("read unit base stat %s  value = %d\n", stats_data->m_szName, value);
		break;
	case STAT_SOURCE_OBJECT:
		ASSERT(vm->context.object);
		baseValue = UnitGetBaseStatShift(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		value = UnitGetStatShift(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat)) - baseValue;
		ScriptExecTrace("read unit base stat %s  value = %d\n", stats_data->m_szName, value);
		break;
	case STAT_SOURCE_STACK_UNIT:
		{
			value = 0;
			ASSERT_RETZERO(vm->stacktop >= 1);
			UNIT * unit = (UNIT *)(vm->Stack[--vm->stacktop].ptr);
			if (unit)
			{
				baseValue = UnitGetBaseStatShift(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
				value = UnitGetStatShift(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat)) - baseValue;
			}
			vm->cur_stat_source = vm->pre_stat_source;
		}
		break;
	case STAT_SOURCE_RIDER:
		ASSERT(vm->context.stats && vm->context.unit);
		value = StatsGetCalculatedStatShift(vm->context.unit, UNITTYPE_NONE, vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		ScriptExecTrace("read rider stat %s  value = %d\n", stats_data->m_szName, value);
		break;
	default:
		value = 0;
		ASSERT(0);
		break;
	}
	vm->Stack[vm->stacktop++].i = value;
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_STAT(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned int stat;
	ASSERT_RETZERO(ByteStreamGetDword(&str, stat));
	const STATS_DATA * stats_data = (const STATS_DATA *)ExcelGetData(NULL, DATATABLE_STATS, STAT_GET_STAT(stat));
	ASSERT_RETZERO(stats_data);
	int value = 0;

	switch (vm->cur_stat_source)
	{
	case STAT_SOURCE_STATS:
	case STAT_SOURCE_RIDER:
		ASSERT(vm->context.stats);
		StatsSetStatShift(vm->context.game, vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), vm->Stack[vm->stacktop-1].i);
		value = StatsGetStatShift(vm->context.game, vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		break;
	case STAT_SOURCE_UNIT:
		ASSERT(vm->context.unit);
		UnitSetStatShift(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), vm->Stack[vm->stacktop-1].i);
		value = UnitGetStatShift(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		break;
	case STAT_SOURCE_OBJECT:
		ASSERT(vm->context.object);
		UnitSetStatShift(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), vm->Stack[vm->stacktop-1].i);
		value = UnitGetStatShift(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		break;
	case STAT_SOURCE_STACK_UNIT:
		{
			value = 0;
			ASSERT_RETZERO(vm->stacktop >= 1);
			UNIT * unit = (UNIT *)(vm->Stack[--vm->stacktop].ptr);
			if (unit)
			{
				UnitSetStatShift(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), vm->Stack[vm->stacktop-1].i);
				value = UnitGetStatShift(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			}
			vm->cur_stat_source = vm->pre_stat_source;
		}
		break;
	default:
		value = 0;
		ASSERT(0);
		break;
	}
	vm->Stack[vm->stacktop-1].i = value;
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_MUL_STAT(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned int stat;
	ASSERT_RETZERO(ByteStreamGetDword(&str, stat));
	const STATS_DATA * stats_data = (const STATS_DATA *)ExcelGetData(NULL, DATATABLE_STATS, STAT_GET_STAT(stat));
	ASSERT_RETZERO(stats_data);
	int value = 0;

	switch (vm->cur_stat_source)
	{
	case STAT_SOURCE_STATS:
	case STAT_SOURCE_RIDER:
		{
			ASSERT(vm->context.stats);
			int val = StatsGetStatShift(vm->context.game, vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			val *= vm->Stack[vm->stacktop-1].i;
			StatsSetStatShift(vm->context.game, vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
			value = StatsGetStatShift(vm->context.game, vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_UNIT:
		{
			ASSERT(vm->context.unit);
			int val = UnitGetBaseStatShift(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			val *= vm->Stack[vm->stacktop-1].i;
			UnitSetStatShift(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
			value = UnitGetStatShift(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_OBJECT:
		{
			ASSERT(vm->context.object);
			int val = UnitGetBaseStatShift(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			val *= vm->Stack[vm->stacktop-1].i;
			UnitSetStatShift(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
			value = UnitGetStatShift(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_STACK_UNIT:
		{
			value = 0;
			ASSERT_RETZERO(vm->stacktop >= 1);
			UNIT * unit = (UNIT *)(vm->Stack[--vm->stacktop].ptr);
			if (unit)
			{
				int val = UnitGetBaseStatShift(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
				val *= vm->Stack[vm->stacktop-1].i;
				UnitSetStatShift(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
				value = UnitGetStatShift(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			}
			vm->cur_stat_source = vm->pre_stat_source;
		}
		break;
	default:
		value = 0;
		ASSERT(0);
		break;
	}
	vm->Stack[vm->stacktop-1].i = value;
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_DIV_STAT(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned int stat;
	ASSERT_RETZERO(ByteStreamGetDword(&str, stat));
	const STATS_DATA * stats_data = (const STATS_DATA *)ExcelGetData(NULL, DATATABLE_STATS, STAT_GET_STAT(stat));
	ASSERT_RETZERO(stats_data);
	int value = 0;

	switch (vm->cur_stat_source)
	{
	case STAT_SOURCE_STATS:
	case STAT_SOURCE_RIDER:
		{
			ASSERT(vm->context.stats);
			int val = StatsGetStatShift(vm->context.game, vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			val /= vm->Stack[vm->stacktop-1].i;
			StatsSetStatShift(vm->context.game, vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
			value = StatsGetStatShift(vm->context.game, vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_UNIT:
		{
			ASSERT(vm->context.unit);
			int val = UnitGetBaseStatShift(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			val /= vm->Stack[vm->stacktop-1].i;
			UnitSetStatShift(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
			value = UnitGetStatShift(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_OBJECT:
		{
			ASSERT(vm->context.object);
			int val = UnitGetBaseStatShift(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			val /= vm->Stack[vm->stacktop-1].i;
			UnitSetStatShift(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
			value = UnitGetStatShift(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_STACK_UNIT:
		{
			value = 0;
			ASSERT_RETZERO(vm->stacktop >= 1);
			UNIT * unit = (UNIT *)(vm->Stack[--vm->stacktop].ptr);
			if (unit)
			{
				int val = UnitGetBaseStatShift(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
				val /= vm->Stack[vm->stacktop-1].i;
				UnitSetStatShift(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
				value = UnitGetStatShift(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			}
			vm->cur_stat_source = vm->pre_stat_source;
		}
		break;
	default:
		value = 0;
		ASSERT(0);
		break;
	}
	vm->Stack[vm->stacktop-1].i = value;
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_MOD_STAT(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned int stat;
	ASSERT_RETZERO(ByteStreamGetDword(&str, stat));
	const STATS_DATA * stats_data = (const STATS_DATA *)ExcelGetData(NULL, DATATABLE_STATS, STAT_GET_STAT(stat));
	ASSERT_RETZERO(stats_data);
	int value = 0;

	switch (vm->cur_stat_source)
	{
	case STAT_SOURCE_STATS:
	case STAT_SOURCE_RIDER:
		{
			ASSERT(vm->context.stats);
			int val = StatsGetStatShift(vm->context.game, vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			val %= vm->Stack[vm->stacktop-1].i;
			StatsSetStatShift(vm->context.game, vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
			value = StatsGetStatShift(vm->context.game, vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_UNIT:
		{
			ASSERT(vm->context.unit);
			int val = UnitGetBaseStatShift(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			val %= vm->Stack[vm->stacktop-1].i;
			UnitSetStatShift(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
			value = UnitGetStatShift(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_OBJECT:
		{
			ASSERT(vm->context.object);
			int val = UnitGetBaseStatShift(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			val %= vm->Stack[vm->stacktop-1].i;
			UnitSetStatShift(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
			value = UnitGetStatShift(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_STACK_UNIT:
		{
			value = 0;
			ASSERT_RETZERO(vm->stacktop >= 1);
			UNIT * unit = (UNIT *)(vm->Stack[--vm->stacktop].ptr);
			if (unit)
			{
				int val = UnitGetBaseStatShift(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
				val %= vm->Stack[vm->stacktop-1].i;
				UnitSetStatShift(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
				value = UnitGetStatShift(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			}
			vm->cur_stat_source = vm->pre_stat_source;
		}
		break;
	default:
		value = 0;
		ASSERT(0);
		break;
	}
	vm->Stack[vm->stacktop-1].i = value;
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_ADD_STAT(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned int stat;
	ASSERT_RETZERO(ByteStreamGetDword(&str, stat));
	const STATS_DATA * stats_data = (const STATS_DATA *)ExcelGetData(NULL, DATATABLE_STATS, STAT_GET_STAT(stat));
	ASSERT_RETZERO(stats_data);
	int value = 0;

	switch (vm->cur_stat_source)
	{
	case STAT_SOURCE_STATS:
	case STAT_SOURCE_RIDER:
		{
			ASSERT(vm->context.stats);
			StatsChangeStatShift(vm->context.game, vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), vm->Stack[vm->stacktop-1].i);
			value = StatsGetStatShift(vm->context.game, vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_UNIT:
		{
			ASSERT(vm->context.unit);
			UnitChangeStatShift(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), vm->Stack[vm->stacktop-1].i);
			value = UnitGetStatShift(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_OBJECT:
		{
			ASSERT(vm->context.object);
			UnitChangeStatShift(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), vm->Stack[vm->stacktop-1].i);
			value = UnitGetStatShift(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_STACK_UNIT:
		{
			value = 0;
			ASSERT_RETZERO(vm->stacktop >= 1);
			UNIT * unit = (UNIT *)(vm->Stack[--vm->stacktop].ptr);
			if (unit)
			{
				UnitChangeStatShift(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), vm->Stack[vm->stacktop-1].i);
				value = UnitGetStatShift(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			}
			vm->cur_stat_source = vm->pre_stat_source;
		}
		break;
	default:
		value = 0;
		ASSERT(0);
		break;
	}
	vm->Stack[vm->stacktop-1].i = value;
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_SUB_STAT(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned int stat;
	ASSERT_RETZERO(ByteStreamGetDword(&str, stat));
	const STATS_DATA * stats_data = (const STATS_DATA *)ExcelGetData(NULL, DATATABLE_STATS, STAT_GET_STAT(stat));
	ASSERT_RETZERO(stats_data);
	int value = 0;

	switch (vm->cur_stat_source)
	{
	case STAT_SOURCE_STATS:
	case STAT_SOURCE_RIDER:
		{
			ASSERT(vm->context.stats);
			StatsChangeStatShift(vm->context.game, vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), -vm->Stack[vm->stacktop-1].i);
			value = StatsGetStatShift(vm->context.game, vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_UNIT:
		{
			ASSERT(vm->context.unit);
			UnitChangeStatShift(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), -vm->Stack[vm->stacktop-1].i);
			value = UnitGetStatShift(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_OBJECT:
		{
			ASSERT(vm->context.object);
			UnitChangeStatShift(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), -vm->Stack[vm->stacktop-1].i);
			value = UnitGetStatShift(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_STACK_UNIT:
		{
			value = 0;
			ASSERT_RETZERO(vm->stacktop >= 1);
			UNIT * unit = (UNIT *)(vm->Stack[--vm->stacktop].ptr);
			if (unit)
			{
				UnitChangeStatShift(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), -vm->Stack[vm->stacktop-1].i);
				value = UnitGetStatShift(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			}
			vm->cur_stat_source = vm->pre_stat_source;
		}
		break;
	default:
		value = 0;
		ASSERT(0);
		break;
	}
	vm->Stack[vm->stacktop-1].i = value;
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_LSHIFT_STAT(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned int stat;
	ASSERT_RETZERO(ByteStreamGetDword(&str, stat));
	const STATS_DATA * stats_data = (const STATS_DATA *)ExcelGetData(NULL, DATATABLE_STATS, STAT_GET_STAT(stat));
	ASSERT_RETZERO(stats_data);
	int value = 0;

	switch (vm->cur_stat_source)
	{
	case STAT_SOURCE_STATS:
	case STAT_SOURCE_RIDER:
		{
			ASSERT(vm->context.stats);
			int val = StatsGetStatShift(vm->context.game, vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			val <<= vm->Stack[vm->stacktop-1].i;
			StatsSetStatShift(vm->context.game, vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
			value = StatsGetStatShift(vm->context.game, vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_UNIT:
		{
			ASSERT(vm->context.unit);
			int val = UnitGetBaseStatShift(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			val <<= vm->Stack[vm->stacktop-1].i;
			UnitSetStatShift(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
			value = UnitGetStatShift(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_OBJECT:
		{
			ASSERT(vm->context.object);
			int val = UnitGetBaseStatShift(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			val <<= vm->Stack[vm->stacktop-1].i;
			UnitSetStatShift(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
			value = UnitGetStatShift(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_STACK_UNIT:
		{
			value = 0;
			ASSERT_RETZERO(vm->stacktop >= 1);
			UNIT * unit = (UNIT *)(vm->Stack[--vm->stacktop].ptr);
			if (unit)
			{
				int val = UnitGetBaseStatShift(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
				val <<= vm->Stack[vm->stacktop-1].i;
				UnitSetStatShift(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
				value = UnitGetStatShift(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			}
			vm->cur_stat_source = vm->pre_stat_source;
		}
		break;
	default:
		value = 0;
		ASSERT(0);
		break;
	}
	vm->Stack[vm->stacktop-1].i = value;
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_RSHIFT_STAT(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned int stat;
	ASSERT_RETZERO(ByteStreamGetDword(&str, stat));
	const STATS_DATA * stats_data = (const STATS_DATA *)ExcelGetData(NULL, DATATABLE_STATS, STAT_GET_STAT(stat));
	ASSERT_RETZERO(stats_data);
	int value = 0;

	switch (vm->cur_stat_source)
	{
	case STAT_SOURCE_STATS:
	case STAT_SOURCE_RIDER:
		{
			ASSERT(vm->context.stats);
			int val = StatsGetStatShift(vm->context.game, vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			val >>= vm->Stack[vm->stacktop-1].i;
			StatsSetStatShift(vm->context.game, vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
			value = StatsGetStatShift(vm->context.game, vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_UNIT:
		{
			ASSERT(vm->context.unit);
			int val = UnitGetBaseStatShift(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			val >>= vm->Stack[vm->stacktop-1].i;
			UnitSetStatShift(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
			value = UnitGetStatShift(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_OBJECT:
		{
			ASSERT(vm->context.object);
			int val = UnitGetBaseStatShift(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			val >>= vm->Stack[vm->stacktop-1].i;
			UnitSetStatShift(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
			value = UnitGetStatShift(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_STACK_UNIT:
		{
			value = 0;
			ASSERT_RETZERO(vm->stacktop >= 1);
			UNIT * unit = (UNIT *)(vm->Stack[--vm->stacktop].ptr);
			if (unit)
			{
				int val = UnitGetBaseStatShift(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
				val >>= vm->Stack[vm->stacktop-1].i;
				UnitSetStatShift(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
				value = UnitGetStatShift(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			}
			vm->cur_stat_source = vm->pre_stat_source;
		}
		break;
	default:
		value = 0;
		ASSERT(0);
		break;
	}
	vm->Stack[vm->stacktop-1].i = value;
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_AND_STAT(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned int stat;
	ASSERT_RETZERO(ByteStreamGetDword(&str, stat));
	const STATS_DATA * stats_data = (const STATS_DATA *)ExcelGetData(NULL, DATATABLE_STATS, STAT_GET_STAT(stat));
	ASSERT_RETZERO(stats_data);
	int value = 0;

	switch (vm->cur_stat_source)
	{
	case STAT_SOURCE_STATS:
	case STAT_SOURCE_RIDER:
		{
			ASSERT(vm->context.stats);
			int val = StatsGetStatShift(vm->context.game, vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			val &= vm->Stack[vm->stacktop-1].i;
			StatsSetStatShift(vm->context.game, vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
			value = StatsGetStatShift(vm->context.game, vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_UNIT:
		{
			ASSERT(vm->context.unit);
			int val = UnitGetBaseStatShift(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			val &= vm->Stack[vm->stacktop-1].i;
			UnitSetStatShift(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
			value = UnitGetStatShift(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_OBJECT:
		{
			ASSERT(vm->context.object);
			int val = UnitGetBaseStatShift(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			val &= vm->Stack[vm->stacktop-1].i;
			UnitSetStatShift(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
			value = UnitGetStatShift(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_STACK_UNIT:
		{
			value = 0;
			ASSERT_RETZERO(vm->stacktop >= 1);
			UNIT * unit = (UNIT *)(vm->Stack[--vm->stacktop].ptr);
			if (unit)
			{
				int val = UnitGetBaseStatShift(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
				val &= vm->Stack[vm->stacktop-1].i;
				UnitSetStatShift(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
				value = UnitGetStatShift(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			}
			vm->cur_stat_source = vm->pre_stat_source;
		}
		break;
	default:
		value = 0;
		ASSERT(0);
		break;
	}
	vm->Stack[vm->stacktop-1].i = value;
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_XOR_STAT(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned int stat;
	ASSERT_RETZERO(ByteStreamGetDword(&str, stat));
	const STATS_DATA * stats_data = (const STATS_DATA *)ExcelGetData(NULL, DATATABLE_STATS, STAT_GET_STAT(stat));
	ASSERT_RETZERO(stats_data);
	int value = 0;

	switch (vm->cur_stat_source)
	{
	case STAT_SOURCE_STATS:
	case STAT_SOURCE_RIDER:
		{
			ASSERT(vm->context.stats);
			int val = StatsGetStatShift(vm->context.game, vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			val ^= vm->Stack[vm->stacktop-1].i;
			StatsSetStatShift(vm->context.game, vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
			value = StatsGetStatShift(vm->context.game, vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_UNIT:
		{
			ASSERT(vm->context.unit);
			int val = UnitGetBaseStatShift(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			val ^= vm->Stack[vm->stacktop-1].i;
			UnitSetStatShift(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
			value = UnitGetStatShift(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_OBJECT:
		{
			ASSERT(vm->context.object);
			int val = UnitGetBaseStatShift(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			val ^= vm->Stack[vm->stacktop-1].i;
			UnitSetStatShift(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
			value = UnitGetStatShift(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_STACK_UNIT:
		{
			value = 0;
			ASSERT_RETZERO(vm->stacktop >= 1);
			UNIT * unit = (UNIT *)(vm->Stack[--vm->stacktop].ptr);
			if (unit)
			{
				int val = UnitGetBaseStatShift(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
				val ^= vm->Stack[vm->stacktop-1].i;
				UnitSetStatShift(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
				value = UnitGetStatShift(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			}
			vm->cur_stat_source = vm->pre_stat_source;
		}
		break;
	default:
		value = 0;
		ASSERT(0);
		break;
	}
	vm->Stack[vm->stacktop-1].i = value;
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_OR_STAT(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned int stat;
	ASSERT_RETZERO(ByteStreamGetDword(&str, stat));
	const STATS_DATA * stats_data = (const STATS_DATA *)ExcelGetData(NULL, DATATABLE_STATS, STAT_GET_STAT(stat));
	ASSERT_RETZERO(stats_data);
	int value = 0;

	switch (vm->cur_stat_source)
	{
	case STAT_SOURCE_STATS:
	case STAT_SOURCE_RIDER:
		{
			ASSERT(vm->context.stats);
			int val = StatsGetStatShift(vm->context.game, vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			val |= vm->Stack[vm->stacktop-1].i;
			StatsSetStatShift(vm->context.game, vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
			value = StatsGetStatShift(vm->context.game, vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_UNIT:
		{
			ASSERT(vm->context.unit);
			int val = UnitGetBaseStatShift(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			val |= vm->Stack[vm->stacktop-1].i;
			UnitSetStatShift(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
			value = UnitGetStatShift(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_OBJECT:
		{
			ASSERT(vm->context.object);
			int val = UnitGetBaseStatShift(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			val |= vm->Stack[vm->stacktop-1].i;
			UnitSetStatShift(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
			value = UnitGetStatShift(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_STACK_UNIT:
		{
			value = 0;
			ASSERT_RETZERO(vm->stacktop >= 1);
			UNIT * unit = (UNIT *)(vm->Stack[--vm->stacktop].ptr);
			if (unit)
			{
				int val = UnitGetBaseStatShift(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
				val |= vm->Stack[vm->stacktop-1].i;
				UnitSetStatShift(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
				value = UnitGetStatShift(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			}
			vm->cur_stat_source = vm->pre_stat_source;
		}
		break;
	default:
		value = 0;
		ASSERT(0);
		break;
	}
	vm->Stack[vm->stacktop-1].i = value;
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_RSTAT_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int stat;
	ASSERT_RETZERO(ByteStreamGetDword(&str, stat));
	const STATS_DATA * stats_data = (const STATS_DATA *)ExcelGetData(NULL, DATATABLE_STATS, STAT_GET_STAT(stat));
	ASSERT_RETZERO(stats_data);
	int value;

	switch (vm->cur_stat_source)
	{
	case STAT_SOURCE_STATS:
		ASSERT(vm->context.stats);
		value = StatsGetStat(vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		ScriptExecTrace("read list stat %s  value = %d\n", stats_data->m_szName, value);
		break;
	case STAT_SOURCE_UNIT:
		ASSERT(vm->context.unit);
		value = UnitGetStat(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		ScriptExecTrace("read unit stat %s  value = %d\n", stats_data->m_szName, value);
		break;
	case STAT_SOURCE_OBJECT:
		ASSERT(vm->context.object);
		value = UnitGetStat(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		ScriptExecTrace("read unit stat %s  value = %d\n", stats_data->m_szName, value);
		break;
	case STAT_SOURCE_STACK_UNIT:
		{
			value = 0;
			ASSERT_RETZERO(vm->stacktop >= 1);
			UNIT * unit = (UNIT *)(vm->Stack[--vm->stacktop].ptr);
			if (unit)
			{
				value = UnitGetStat(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			}
			vm->cur_stat_source = vm->pre_stat_source;
		}
		break;
	case STAT_SOURCE_RIDER:
		ASSERT(vm->context.stats && vm->context.unit);
		value = StatsGetCalculatedStat(vm->context.unit, UNITTYPE_NONE, vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		ScriptExecTrace("read rider stat %s  value = %d\n", stats_data->m_szName, value);
		break;
	default:
		value = 0;
		ASSERT(0);
		break;
	}
	vm->Stack[vm->stacktop++].i = value;
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_RSTAT_US_BASE(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int stat;
	ASSERT_RETZERO(ByteStreamGetDword(&str, stat));
	const STATS_DATA * stats_data = (const STATS_DATA *)ExcelGetData(NULL, DATATABLE_STATS, STAT_GET_STAT(stat));
	ASSERT_RETZERO(stats_data);
	int value;

	switch (vm->cur_stat_source)
	{
	case STAT_SOURCE_STATS:
		ASSERT(vm->context.stats);
		value = StatsGetStat(vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		ScriptExecTrace("read list stat %s  value = %d\n", stats_data->m_szName, value);
		break;
	case STAT_SOURCE_UNIT:
		ASSERT(vm->context.unit);
		value = UnitGetBaseStat(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		ScriptExecTrace("read unit stat %s  value = %d\n", stats_data->m_szName, value);
		break;
	case STAT_SOURCE_OBJECT:
		ASSERT(vm->context.object);
		value = UnitGetBaseStat(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		ScriptExecTrace("read unit stat %s  value = %d\n", stats_data->m_szName, value);
		break;
	case STAT_SOURCE_STACK_UNIT:
		{
			value = 0;
			ASSERT_RETZERO(vm->stacktop >= 1);
			UNIT * unit = (UNIT *)(vm->Stack[--vm->stacktop].ptr);
			if (unit)
			{
				value = UnitGetBaseStat(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			}
			vm->cur_stat_source = vm->pre_stat_source;
		}
		break;
	case STAT_SOURCE_RIDER:
		ASSERT(vm->context.stats && vm->context.unit);
		value = StatsGetCalculatedStat(vm->context.unit, UNITTYPE_NONE, vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		ScriptExecTrace("read rider stat %s  value = %d\n", stats_data->m_szName, value);
		break;
	default:
		value = 0;
		ASSERT(0);
		break;
	}
	vm->Stack[vm->stacktop++].i = value;
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_RSTAT_US_MOD(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int stat;
	ASSERT_RETZERO(ByteStreamGetDword(&str, stat));
	const STATS_DATA * stats_data = (const STATS_DATA *)ExcelGetData(NULL, DATATABLE_STATS, STAT_GET_STAT(stat));
	ASSERT_RETZERO(stats_data);
	int value, baseValue;

	switch (vm->cur_stat_source)
	{
	case STAT_SOURCE_STATS:
		ASSERT(vm->context.stats);
		value = StatsGetStat(vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		ScriptExecTrace("read list stat %s  value = %d\n", stats_data->m_szName, value);
		break;
	case STAT_SOURCE_UNIT:
		ASSERT(vm->context.unit);
		baseValue = UnitGetBaseStat(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		value = UnitGetStat(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat)) - baseValue;
		ScriptExecTrace("read unit stat %s  value = %d\n", stats_data->m_szName, value);
		break;
	case STAT_SOURCE_OBJECT:
		ASSERT(vm->context.object);
		baseValue = UnitGetBaseStat(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		value = UnitGetStat(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat)) - baseValue;
		ScriptExecTrace("read unit stat %s  value = %d\n", stats_data->m_szName, value);
		break;
	case STAT_SOURCE_STACK_UNIT:
		{
			value = 0;
			ASSERT_RETZERO(vm->stacktop >= 1);
			UNIT * unit = (UNIT *)(vm->Stack[--vm->stacktop].ptr);
			if (unit)
			{
				baseValue = UnitGetBaseStat(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
				value = UnitGetStat(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat)) - baseValue;
			}
			vm->cur_stat_source = vm->pre_stat_source;
		}
		break;
	case STAT_SOURCE_RIDER:
		ASSERT(vm->context.stats && vm->context.unit);
		value = StatsGetCalculatedStat(vm->context.unit, UNITTYPE_NONE, vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		ScriptExecTrace("read rider stat %s  value = %d\n", stats_data->m_szName, value);
		break;
	default:
		value = 0;
		ASSERT(0);
		break;
	}
	vm->Stack[vm->stacktop++].i = value;
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_STAT_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned int stat;
	ASSERT_RETZERO(ByteStreamGetDword(&str, stat));
	const STATS_DATA * stats_data = (const STATS_DATA *)ExcelGetData(NULL, DATATABLE_STATS, STAT_GET_STAT(stat));
	ASSERT_RETZERO(stats_data);
	int value = 0;

	switch (vm->cur_stat_source)
	{
	case STAT_SOURCE_STATS:
	case STAT_SOURCE_RIDER:
		ASSERT(vm->context.stats);
		StatsSetStat(vm->context.game, vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), vm->Stack[vm->stacktop-1].i);
		value = StatsGetStat(vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		break;
	case STAT_SOURCE_UNIT:
		ASSERT(vm->context.unit);
		UnitSetStat(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), vm->Stack[vm->stacktop-1].i);
		value = UnitGetStat(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		break;
	case STAT_SOURCE_OBJECT:
		ASSERT(vm->context.object);
		UnitSetStat(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), vm->Stack[vm->stacktop-1].i);
		value = UnitGetStat(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		break;
	case STAT_SOURCE_STACK_UNIT:
		{
			value = 0;
			ASSERT_RETZERO(vm->stacktop >= 1);
			UNIT * unit = (UNIT *)(vm->Stack[--vm->stacktop].ptr);
			if (unit)
			{
				UnitSetStat(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), vm->Stack[vm->stacktop-1].i);
				value = UnitGetStat(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			}
			vm->cur_stat_source = vm->pre_stat_source;
		}
		break;
	default:
		value = 0;
		ASSERT(0);
		break;
	}
	vm->Stack[vm->stacktop-1].i = value;
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_MUL_STAT_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned int stat;
	ASSERT_RETZERO(ByteStreamGetDword(&str, stat));
	const STATS_DATA * stats_data = (const STATS_DATA *)ExcelGetData(NULL, DATATABLE_STATS, STAT_GET_STAT(stat));
	ASSERT_RETZERO(stats_data);
	int value = 0;

	switch (vm->cur_stat_source)
	{
	case STAT_SOURCE_STATS:
	case STAT_SOURCE_RIDER:
		{
			ASSERT(vm->context.stats);
			int val = StatsGetStat(vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			val *= vm->Stack[vm->stacktop-1].i;
			StatsSetStat(vm->context.game, vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
			value = StatsGetStat(vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_UNIT:
		{
			ASSERT(vm->context.unit);
			int val = UnitGetBaseStat(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			val *= vm->Stack[vm->stacktop-1].i;
			UnitSetStat(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
			value = UnitGetStat(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_OBJECT:
		{
			ASSERT(vm->context.object);
			int val = UnitGetBaseStat(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			val *= vm->Stack[vm->stacktop-1].i;
			UnitSetStat(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
			value = UnitGetStat(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_STACK_UNIT:
		{
			value = 0;
			ASSERT_RETZERO(vm->stacktop >= 1);
			UNIT * unit = (UNIT *)(vm->Stack[--vm->stacktop].ptr);
			if (unit)
			{
				int val = UnitGetBaseStat(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
				val *= vm->Stack[vm->stacktop-1].i;
				UnitSetStat(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
				value = UnitGetStat(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			}
			vm->cur_stat_source = vm->pre_stat_source;
		}
		break;
	default:
		value = 0;
		ASSERT(0);
		break;
	}
	vm->Stack[vm->stacktop-1].i = value;
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_DIV_STAT_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned int stat;
	ASSERT_RETZERO(ByteStreamGetDword(&str, stat));
	const STATS_DATA * stats_data = (const STATS_DATA *)ExcelGetData(NULL, DATATABLE_STATS, STAT_GET_STAT(stat));
	ASSERT_RETZERO(stats_data);
	int value = 0;

	switch (vm->cur_stat_source)
	{
	case STAT_SOURCE_STATS:
	case STAT_SOURCE_RIDER:
		{
			ASSERT(vm->context.stats);
			int val = StatsGetStat(vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			val /= vm->Stack[vm->stacktop-1].i;
			StatsSetStat(vm->context.game, vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
			value = StatsGetStat(vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_UNIT:
		{
			ASSERT(vm->context.unit);
			int val = UnitGetBaseStat(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			val /= vm->Stack[vm->stacktop-1].i;
			UnitSetStat(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
			value = UnitGetStat(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_OBJECT:
		{
			ASSERT(vm->context.object);
			int val = UnitGetBaseStat(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			val /= vm->Stack[vm->stacktop-1].i;
			UnitSetStat(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
			value = UnitGetStat(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_STACK_UNIT:
		{
			value = 0;
			ASSERT_RETZERO(vm->stacktop >= 1);
			UNIT * unit = (UNIT *)(vm->Stack[--vm->stacktop].ptr);
			if (unit)
			{
				int val = UnitGetBaseStat(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
				val /= vm->Stack[vm->stacktop-1].i;
				UnitSetStat(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
				value = UnitGetStat(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			}
			vm->cur_stat_source = vm->pre_stat_source;
		}
		break;
	default:
		value = 0;
		ASSERT(0);
		break;
	}
	vm->Stack[vm->stacktop-1].i = value;
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_MOD_STAT_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned int stat;
	ASSERT_RETZERO(ByteStreamGetDword(&str, stat));
	const STATS_DATA * stats_data = (const STATS_DATA *)ExcelGetData(NULL, DATATABLE_STATS, STAT_GET_STAT(stat));
	ASSERT_RETZERO(stats_data);
	int value = 0;

	switch (vm->cur_stat_source)
	{
	case STAT_SOURCE_STATS:
	case STAT_SOURCE_RIDER:
		{
			ASSERT(vm->context.stats);
			int val = StatsGetStat(vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			val %= vm->Stack[vm->stacktop-1].i;
			StatsSetStat(vm->context.game, vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
			value = StatsGetStat(vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_UNIT:
		{
			ASSERT(vm->context.unit);
			int val = UnitGetBaseStat(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			val %= vm->Stack[vm->stacktop-1].i;
			UnitSetStat(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
			value = UnitGetStat(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_OBJECT:
		{
			ASSERT(vm->context.object);
			int val = UnitGetBaseStat(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			val %= vm->Stack[vm->stacktop-1].i;
			UnitSetStat(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
			value = UnitGetStat(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_STACK_UNIT:
		{
			value = 0;
			ASSERT_RETZERO(vm->stacktop >= 1);
			UNIT * unit = (UNIT *)(vm->Stack[--vm->stacktop].ptr);
			if (unit)
			{
				int val = UnitGetBaseStat(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
				val %= vm->Stack[vm->stacktop-1].i;
				UnitSetStat(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
				value = UnitGetStat(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			}
			vm->cur_stat_source = vm->pre_stat_source;
		}
		break;
	default:
		value = 0;
		ASSERT(0);
		break;
	}
	vm->Stack[vm->stacktop-1].i = value;
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_ADD_STAT_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned int stat;
	ASSERT_RETZERO(ByteStreamGetDword(&str, stat));
	const STATS_DATA * stats_data = (const STATS_DATA *)ExcelGetData(NULL, DATATABLE_STATS, STAT_GET_STAT(stat));
	ASSERT_RETZERO(stats_data);
	int value = 0;

	switch (vm->cur_stat_source)
	{
	case STAT_SOURCE_STATS:
	case STAT_SOURCE_RIDER:
		{
			ASSERT(vm->context.stats);
			StatsChangeStat(vm->context.game, vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), vm->Stack[vm->stacktop-1].i);
			value = StatsGetStat(vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_UNIT:
		{
			ASSERT(vm->context.unit);
			UnitChangeStat(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), vm->Stack[vm->stacktop-1].i);
			value = UnitGetStat(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_OBJECT:
		{
			ASSERT(vm->context.object);
			UnitChangeStat(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), vm->Stack[vm->stacktop-1].i);
			value = UnitGetStat(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_STACK_UNIT:
		{
			value = 0;
			ASSERT_RETZERO(vm->stacktop >= 1);
			UNIT * unit = (UNIT *)(vm->Stack[--vm->stacktop].ptr);
			if (unit)
			{
				UnitChangeStat(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), vm->Stack[vm->stacktop-1].i);
				value = UnitGetStat(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			}
			vm->cur_stat_source = vm->pre_stat_source;
		}
		break;
	default:
		value = 0;
		ASSERT(0);
		break;
	}
	vm->Stack[vm->stacktop-1].i = value;
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_SUB_STAT_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned int stat;
	ASSERT_RETZERO(ByteStreamGetDword(&str, stat));
	const STATS_DATA * stats_data = (const STATS_DATA *)ExcelGetData(NULL, DATATABLE_STATS, STAT_GET_STAT(stat));
	ASSERT_RETZERO(stats_data);
	int value = 0;

	switch (vm->cur_stat_source)
	{
	case STAT_SOURCE_STATS:
	case STAT_SOURCE_RIDER:
		{
			ASSERT(vm->context.stats);
			StatsChangeStat(vm->context.game, vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), -vm->Stack[vm->stacktop-1].i);
			value = StatsGetStat(vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_UNIT:
		{
			ASSERT(vm->context.unit);
			UnitChangeStat(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), -vm->Stack[vm->stacktop-1].i);
			value = UnitGetStat(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_OBJECT:
		{
			ASSERT(vm->context.object);
			UnitChangeStat(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), -vm->Stack[vm->stacktop-1].i);
			value = UnitGetStat(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_STACK_UNIT:
		{
			value = 0;
			ASSERT_RETZERO(vm->stacktop >= 1);
			UNIT * unit = (UNIT *)(vm->Stack[--vm->stacktop].ptr);
			if (unit)
			{
				UnitChangeStat(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), -vm->Stack[vm->stacktop-1].i);
				value = UnitGetStat(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			}
			vm->cur_stat_source = vm->pre_stat_source;
		}
		break;
	default:
		value = 0;
		ASSERT(0);
		break;
	}
	vm->Stack[vm->stacktop-1].i = value;
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_LSHIFT_STAT_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned int stat;
	ASSERT_RETZERO(ByteStreamGetDword(&str, stat));
	const STATS_DATA * stats_data = (const STATS_DATA *)ExcelGetData(NULL, DATATABLE_STATS, STAT_GET_STAT(stat));
	ASSERT_RETZERO(stats_data);
	int value = 0;

	switch (vm->cur_stat_source)
	{
	case STAT_SOURCE_STATS:
	case STAT_SOURCE_RIDER:
		{
			ASSERT(vm->context.stats);
			int val = StatsGetStat(vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			val <<= vm->Stack[vm->stacktop-1].i;
			StatsSetStat(vm->context.game, vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
			value = StatsGetStat(vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_UNIT:
		{
			ASSERT(vm->context.unit);
			int val = UnitGetBaseStat(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			val <<= vm->Stack[vm->stacktop-1].i;
			UnitSetStat(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
			value = UnitGetStat(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_OBJECT:
		{
			ASSERT(vm->context.object);
			int val = UnitGetBaseStat(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			val <<= vm->Stack[vm->stacktop-1].i;
			UnitSetStat(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
			value = UnitGetStat(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_STACK_UNIT:
		{
			value = 0;
			ASSERT_RETZERO(vm->stacktop >= 1);
			UNIT * unit = (UNIT *)(vm->Stack[--vm->stacktop].ptr);
			if (unit)
			{
				int val = UnitGetBaseStat(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
				val <<= vm->Stack[vm->stacktop-1].i;
				UnitSetStat(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
				value = UnitGetStat(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			}
			vm->cur_stat_source = vm->pre_stat_source;
		}
		break;
	default:
		value = 0;
		ASSERT(0);
		break;
	}
	vm->Stack[vm->stacktop-1].i = value;
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_RSHIFT_STAT_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned int stat;
	ASSERT_RETZERO(ByteStreamGetDword(&str, stat));
	const STATS_DATA * stats_data = (const STATS_DATA *)ExcelGetData(NULL, DATATABLE_STATS, STAT_GET_STAT(stat));
	ASSERT_RETZERO(stats_data);
	int value = 0;

	switch (vm->cur_stat_source)
	{
	case STAT_SOURCE_STATS:
	case STAT_SOURCE_RIDER:
		{
			ASSERT(vm->context.stats);
			int val = StatsGetStat(vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			val >>= vm->Stack[vm->stacktop-1].i;
			StatsSetStat(vm->context.game, vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
			value = StatsGetStat(vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_UNIT:
		{
			ASSERT(vm->context.unit);
			int val = UnitGetBaseStat(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			val >>= vm->Stack[vm->stacktop-1].i;
			UnitSetStat(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
			value = UnitGetStat(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_OBJECT:
		{
			ASSERT(vm->context.object);
			int val = UnitGetBaseStat(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			val >>= vm->Stack[vm->stacktop-1].i;
			UnitSetStat(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
			value = UnitGetStat(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_STACK_UNIT:
		{
			value = 0;
			ASSERT_RETZERO(vm->stacktop >= 1);
			UNIT * unit = (UNIT *)(vm->Stack[--vm->stacktop].ptr);
			if (unit)
			{
				int val = UnitGetBaseStat(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
				val >>= vm->Stack[vm->stacktop-1].i;
				UnitSetStat(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
				value = UnitGetStat(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			}
			vm->cur_stat_source = vm->pre_stat_source;
		}
		break;
	default:
		value = 0;
		ASSERT(0);
		break;
	}
	vm->Stack[vm->stacktop-1].i = value;
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_AND_STAT_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned int stat;
	ASSERT_RETZERO(ByteStreamGetDword(&str, stat));
	const STATS_DATA * stats_data = (const STATS_DATA *)ExcelGetData(NULL, DATATABLE_STATS, STAT_GET_STAT(stat));
	ASSERT_RETZERO(stats_data);
	int value = 0;

	switch (vm->cur_stat_source)
	{
	case STAT_SOURCE_STATS:
	case STAT_SOURCE_RIDER:
		{
			ASSERT(vm->context.stats);
			int val = StatsGetStat(vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			val &= vm->Stack[vm->stacktop-1].i;
			StatsSetStat(vm->context.game, vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
			value = StatsGetStat(vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_UNIT:
		{
			ASSERT(vm->context.unit);
			int val = UnitGetBaseStat(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			val &= vm->Stack[vm->stacktop-1].i;
			UnitSetStat(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
			value = UnitGetStat(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_OBJECT:
		{
			ASSERT(vm->context.object);
			int val = UnitGetBaseStat(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			val &= vm->Stack[vm->stacktop-1].i;
			UnitSetStat(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
			value = UnitGetStat(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_STACK_UNIT:
		{
			value = 0;
			ASSERT_RETZERO(vm->stacktop >= 1);
			UNIT * unit = (UNIT *)(vm->Stack[--vm->stacktop].ptr);
			if (unit)
			{
				int val = UnitGetBaseStat(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
				val &= vm->Stack[vm->stacktop-1].i;
				UnitSetStat(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
				value = UnitGetStat(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			}
			vm->cur_stat_source = vm->pre_stat_source;
		}
		break;
	default:
		value = 0;
		ASSERT(0);
		break;
	}
	vm->Stack[vm->stacktop-1].i = value;
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_XOR_STAT_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned int stat;
	ASSERT_RETZERO(ByteStreamGetDword(&str, stat));
	const STATS_DATA * stats_data = (const STATS_DATA *)ExcelGetData(NULL, DATATABLE_STATS, STAT_GET_STAT(stat));
	ASSERT_RETZERO(stats_data);
	int value = 0;

	switch (vm->cur_stat_source)
	{
	case STAT_SOURCE_STATS:
	case STAT_SOURCE_RIDER:
		{
			ASSERT(vm->context.stats);
			int val = StatsGetStat(vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			val ^= vm->Stack[vm->stacktop-1].i;
			StatsSetStat(vm->context.game, vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
			value = StatsGetStat(vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_UNIT:
		{
			ASSERT(vm->context.unit);
			int val = UnitGetBaseStat(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			val ^= vm->Stack[vm->stacktop-1].i;
			UnitSetStat(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
			value = UnitGetStat(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_OBJECT:
		{
			ASSERT(vm->context.object);
			int val = UnitGetBaseStat(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			val ^= vm->Stack[vm->stacktop-1].i;
			UnitSetStat(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
			value = UnitGetStat(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_STACK_UNIT:
		{
			value = 0;
			ASSERT_RETZERO(vm->stacktop >= 1);
			UNIT * unit = (UNIT *)(vm->Stack[--vm->stacktop].ptr);
			if (unit)
			{
				int val = UnitGetBaseStat(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
				val ^= vm->Stack[vm->stacktop-1].i;
				UnitSetStat(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
				value = UnitGetStat(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			}
			vm->cur_stat_source = vm->pre_stat_source;
		}
		break;
	default:
		value = 0;
		ASSERT(0);
		break;
	}
	vm->Stack[vm->stacktop-1].i = value;
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_A_OR_STAT_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1);
	unsigned int stat;
	ASSERT_RETZERO(ByteStreamGetDword(&str, stat));
	const STATS_DATA * stats_data = (const STATS_DATA *)ExcelGetData(NULL, DATATABLE_STATS, STAT_GET_STAT(stat));
	ASSERT_RETZERO(stats_data);
	int value = 0;

	switch (vm->cur_stat_source)
	{
	case STAT_SOURCE_STATS:
	case STAT_SOURCE_RIDER:
		{
			ASSERT(vm->context.stats);
			int val = StatsGetStat(vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			val |= vm->Stack[vm->stacktop-1].i;
			StatsSetStat(vm->context.game, vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
			value = StatsGetStat(vm->context.stats, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_UNIT:
		{
			ASSERT(vm->context.unit);
			int val = UnitGetBaseStat(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			val |= vm->Stack[vm->stacktop-1].i;
			UnitSetStat(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
			value = UnitGetStat(vm->context.unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_OBJECT:
		{
			ASSERT(vm->context.object);
			int val = UnitGetBaseStat(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			val |= vm->Stack[vm->stacktop-1].i;
			UnitSetStat(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
			value = UnitGetStat(vm->context.object, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
		}
		break;
	case STAT_SOURCE_STACK_UNIT:
		{
			value = 0;
			ASSERT_RETZERO(vm->stacktop >= 1);
			UNIT * unit = (UNIT *)(vm->Stack[--vm->stacktop].ptr);
			if (unit)
			{
				int val = UnitGetBaseStat(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
				val |= vm->Stack[vm->stacktop-1].i;
				UnitSetStat(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat), val);
				value = UnitGetStat(unit, STAT_GET_STAT(stat), STAT_GET_PARAM(stat));
			}
			vm->cur_stat_source = vm->pre_stat_source;
		}
		break;
	default:
		value = 0;
		ASSERT(0);
		break;
	}
	vm->Stack[vm->stacktop-1].i = value;
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_RSTATPARAM(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1);						// make sure param is on operand stack
	unsigned int param = vm->Stack[vm->stacktop-1].ui;		// get the param

	unsigned int stat;
	ASSERT_RETZERO(ByteStreamGetDword(&str, stat));			// this is the stat portion

	const STATS_DATA * stats_data = (const STATS_DATA *)ExcelGetData(NULL, DATATABLE_STATS, STAT_GET_STAT(stat));
	ASSERT_RETZERO(stats_data);

	int value;

	switch (vm->cur_stat_source)
	{
	case STAT_SOURCE_STATS:
		ASSERT(vm->context.stats);
		value = StatsGetStatShift(vm->context.game, vm->context.stats, STAT_GET_STAT(stat), param);
		ScriptExecTrace("read list stat %s  value = %d\n", stats_data->m_szName, value);
		break;
	case STAT_SOURCE_UNIT:
		ASSERT(vm->context.unit);
		value = UnitGetStatShift(vm->context.unit, STAT_GET_STAT(stat), param);
		ScriptExecTrace("read unit stat %s  value = %d\n", stats_data->m_szName, value);
		break;
	case STAT_SOURCE_OBJECT:
		ASSERT(vm->context.object);
		value = UnitGetStatShift(vm->context.object, STAT_GET_STAT(stat), param);
		ScriptExecTrace("read unit stat %s  value = %d\n", stats_data->m_szName, value);
		break;
	case STAT_SOURCE_STACK_UNIT:
		{
			value = 0;
			ASSERT_RETZERO(vm->stacktop >= 1);
			UNIT * unit = (UNIT *)(vm->Stack[--vm->stacktop].ptr);
			if (unit)
			{
				value = UnitGetStatShift(unit, STAT_GET_STAT(stat), param);
			}
			vm->cur_stat_source = vm->pre_stat_source;
		}
		break;
	case STAT_SOURCE_RIDER:
		ASSERT(vm->context.stats && vm->context.unit);
		value = StatsGetCalculatedStatShift(vm->context.unit, UNITTYPE_NONE, vm->context.stats, STAT_GET_STAT(stat), param);
		ScriptExecTrace("read rider stat %s  value = %d\n", stats_data->m_szName, value);
		break;
	default:
		value = 0;
		ASSERT(0);
		break;
	}
	vm->Stack[vm->stacktop-1].i = value;
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_RSTATPARAM_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 1);						// make sure param is on operand stack
	unsigned int param = vm->Stack[vm->stacktop-1].ui;		// get the param

	unsigned int stat;
	ASSERT_RETZERO(ByteStreamGetDword(&str, stat));			// this is the stat portion

	const STATS_DATA * stats_data = (const STATS_DATA *)ExcelGetData(NULL, DATATABLE_STATS, STAT_GET_STAT(stat));
	ASSERT_RETZERO(stats_data);

	int value;

	switch (vm->cur_stat_source)
	{
	case STAT_SOURCE_STATS:
		ASSERT(vm->context.stats);
		value = StatsGetStat(vm->context.stats, STAT_GET_STAT(stat), param);
		ScriptExecTrace("read list stat %s  value = %d\n", stats_data->m_szName, value);
		break;
	case STAT_SOURCE_UNIT:
		ASSERT(vm->context.unit);
		value = UnitGetStat(vm->context.unit, STAT_GET_STAT(stat), param);
		ScriptExecTrace("read unit stat %s  value = %d\n", stats_data->m_szName, value);
		break;
	case STAT_SOURCE_OBJECT:
		ASSERT(vm->context.object);
		value = UnitGetStat(vm->context.object, STAT_GET_STAT(stat), param);
		ScriptExecTrace("read unit stat %s  value = %d\n", stats_data->m_szName, value);
		break;
	case STAT_SOURCE_STACK_UNIT:
		{
			value = 0;
			ASSERT_RETZERO(vm->stacktop >= 1);
			UNIT * unit = (UNIT *)(vm->Stack[--vm->stacktop].ptr);
			if (unit)
			{
				value = UnitGetStat(unit, STAT_GET_STAT(stat), param);
			}
			vm->cur_stat_source = vm->pre_stat_source;
		}
		break;
	case STAT_SOURCE_RIDER:
		ASSERT(vm->context.stats && vm->context.unit);
		value = StatsGetCalculatedStat(vm->context.unit, UNITTYPE_NONE, vm->context.stats, STAT_GET_STAT(stat), param);
		ScriptExecTrace("read rider stat %s  value = %d\n", stats_data->m_szName, value);
		break;
	default:
		value = 0;
		ASSERT(0);
		break;
	}
	vm->Stack[vm->stacktop-1].i = value;
	return 1;
}


//----------------------------------------------------------------------------
static inline int fp_VMI_CONTEXTVAR_CH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int var; 
	ASSERT_RETFALSE(ByteStreamGetDword(&str, var));
	ASSERT_RETFALSE(var < arrsize(ContextDef));
	char value = *(char *)((BYTE *)(&vm->context) + ContextDef[var].offset);
	vm->Stack[vm->stacktop++].c = value;
	ScriptExecTrace("push context variable %s value = %d  type = char\n", ContextDef[var].str, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_CONTEXTVAR_UC(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int var; 
	ASSERT_RETFALSE(ByteStreamGetDword(&str, var));
	ASSERT_RETFALSE(var < arrsize(ContextDef));
	unsigned char value = *(unsigned char *)((BYTE *)(&vm->context) + ContextDef[var].offset);
	vm->Stack[vm->stacktop++].uc = value;
	ScriptExecTrace("push context variable %s value = %u  type = unsigned char\n", ContextDef[var].str, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_CONTEXTVAR_SH(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int var; 
	ASSERT_RETFALSE(ByteStreamGetDword(&str, var));
	ASSERT_RETFALSE(var < arrsize(ContextDef));
	short value = *(short *)((BYTE *)(&vm->context) + ContextDef[var].offset);
	vm->Stack[vm->stacktop++].s = value;
	ScriptExecTrace("push context variable %s value = %d  type = short\n", ContextDef[var].str, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_CONTEXTVAR_US(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int var; 
	ASSERT_RETFALSE(ByteStreamGetDword(&str, var));
	ASSERT_RETFALSE(var < arrsize(ContextDef));
	unsigned short value = *(unsigned short *)((BYTE *)(&vm->context) + ContextDef[var].offset);
	vm->Stack[vm->stacktop++].us = value;
	ScriptExecTrace("push context variable %s value = %u  type = unsigned short\n", ContextDef[var].str, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_CONTEXTVAR_IN(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int var; 
	ASSERT_RETFALSE(ByteStreamGetDword(&str, var));
	ASSERT_RETFALSE(var < arrsize(ContextDef));
	int value = *(int *)((BYTE *)(&vm->context) + ContextDef[var].offset);
	vm->Stack[vm->stacktop++].i = value;
	ScriptExecTrace("push context variable %s value = %d  type = int\n", ContextDef[var].str, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_CONTEXTVAR_UI(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int var; 
	ASSERT_RETFALSE(ByteStreamGetDword(&str, var));
	ASSERT_RETFALSE(var < arrsize(ContextDef));
	unsigned int value = *(unsigned int *)((BYTE *)(&vm->context) + ContextDef[var].offset);
	vm->Stack[vm->stacktop++].ui = value;
	ScriptExecTrace("push context variable %s value = %u  type = unsigned int\n", ContextDef[var].str, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_CONTEXTVAR_LL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int var; 
	ASSERT_RETFALSE(ByteStreamGetDword(&str, var));
	ASSERT_RETFALSE(var < arrsize(ContextDef));
	INT64 value = *(INT64 *)((BYTE *)(&vm->context) + ContextDef[var].offset);
	vm->Stack[vm->stacktop++].ll = value;
	ScriptExecTrace("push context variable %s value = %I64d  type = INT64\n", ContextDef[var].str, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_CONTEXTVAR_UL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int var; 
	ASSERT_RETFALSE(ByteStreamGetDword(&str, var));
	ASSERT_RETFALSE(var < arrsize(ContextDef));
	UINT64 value = *(UINT64 *)((BYTE *)(&vm->context) + ContextDef[var].offset);
	vm->Stack[vm->stacktop++].ull = value;
	ScriptExecTrace("push context variable %s value = %I64u  type = UINT64\n", ContextDef[var].str, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_CONTEXTVAR_FL(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int var; 
	ASSERT_RETFALSE(ByteStreamGetDword(&str, var));
	ASSERT_RETFALSE(var < arrsize(ContextDef));
	float value = *(float *)((BYTE *)(&vm->context) + ContextDef[var].offset);
	vm->Stack[vm->stacktop++].f = value;
	ScriptExecTrace("push context variable %s value = %f  type = float\n", ContextDef[var].str, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_CONTEXTVAR_DB(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int var; 
	ASSERT_RETFALSE(ByteStreamGetDword(&str, var));
	ASSERT_RETFALSE(var < arrsize(ContextDef));
	double value = *(double *)((BYTE *)(&vm->context) + ContextDef[var].offset);
	vm->Stack[vm->stacktop++].d = value;
	ScriptExecTrace("push context variable %s value = %f  type = double\n", ContextDef[var].str, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_CONTEXTVAR_LD(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int var; 
	ASSERT_RETFALSE(ByteStreamGetDword(&str, var));
	ASSERT_RETFALSE(var < arrsize(ContextDef));
	long double value = *(long double *)((BYTE *)(&vm->context) + ContextDef[var].offset);
	vm->Stack[vm->stacktop++].ld = value;
	ScriptExecTrace("push context variable %s value = %f  type = double\n", ContextDef[var].str, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_CONTEXTVAR_POINTER(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	unsigned int var; 
	ASSERT_RETFALSE(ByteStreamGetDword(&str, var));
	ASSERT_RETFALSE(var < arrsize(ContextDef));
	void * value = *(void **)((BYTE *)(&vm->context) + ContextDef[var].offset);
	vm->Stack[vm->stacktop++].ptr = value;
	ScriptExecTrace("push context variable %s value = %x  type = pointer\n", ContextDef[var].str, value);
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_VMSTATE(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	vm->Stack[vm->stacktop++].ptr = (void *)vm;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_CONTEXT(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	vm->Stack[vm->stacktop++].ptr = (void *)&vm->context;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_GAME(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	vm->Stack[vm->stacktop++].ptr = (void *)vm->context.game;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_UNIT(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	vm->Stack[vm->stacktop++].ptr = (void *)vm->context.unit;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_STATS(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	vm->Stack[vm->stacktop++].ptr = (void *)vm->context.stats;
	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_ASSIGN_CONTEXTVAR(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 0);

	unsigned int var; 
	ASSERT_RETZERO(ByteStreamGetDword(&str, var));
	ASSERT_RETZERO(var < arrsize(ContextDef));
	switch (ContextDef[var].type)
	{
	case TYPE_CHAR:
		*(BYTE *)((BYTE *)(&vm->context) + ContextDef[var].offset) = vm->Stack[vm->stacktop-1].c;
		ScriptExecTrace("assign context variable %s value = %d  type = char\n", ContextDef[var].str, vm->Stack[vm->stacktop-1].c);
		break;
	case TYPE_BYTE:
		*(BYTE *)((BYTE *)(&vm->context) + ContextDef[var].offset) = vm->Stack[vm->stacktop-1].uc;
		ScriptExecTrace("assign context variable %s value = %u  type = byte\n", ContextDef[var].str, vm->Stack[vm->stacktop-1].uc);
		break;
	case TYPE_SHORT:
		*(short *)((BYTE *)(&vm->context) + ContextDef[var].offset) = vm->Stack[vm->stacktop-1].s;
		ScriptExecTrace("assign context variable %s value = %d  type = short\n", ContextDef[var].str, vm->Stack[vm->stacktop-1].s);
		break;
	case TYPE_WORD:
		*(WORD *)((BYTE *)(&vm->context) + ContextDef[var].offset) = vm->Stack[vm->stacktop-1].us;
		ScriptExecTrace("assign context variable %s value = %u  type = word\n", ContextDef[var].str, vm->Stack[vm->stacktop-1].us);
		break;
	case TYPE_INT:
		*(int *)((BYTE *)(&vm->context) + ContextDef[var].offset) = vm->Stack[vm->stacktop-1].i;
		ScriptExecTrace("assign context variable %s value = %d  type = int\n", ContextDef[var].str, vm->Stack[vm->stacktop-1].i);
		break;
	case TYPE_DWORD:
		*(DWORD *)((BYTE *)(&vm->context) + ContextDef[var].offset) = vm->Stack[vm->stacktop-1].ui;
		ScriptExecTrace("assign context variable %s value = %u  type = dword\n", ContextDef[var].str, vm->Stack[vm->stacktop-1].ui);
		break;
	case TYPE_INT64:
		*(INT64 *)((BYTE *)(&vm->context) + ContextDef[var].offset) = vm->Stack[vm->stacktop-1].ll;
		ScriptExecTrace("assign context variable %s value = %I64d  type = int64\n", ContextDef[var].str, vm->Stack[vm->stacktop-1].ll);
		break;
	case TYPE_QWORD:
		*(UINT64 *)((BYTE *)(&vm->context) + ContextDef[var].offset) = vm->Stack[vm->stacktop-1].ull;
		ScriptExecTrace("assign context variable %s value = %I64u  type = uint64\n", ContextDef[var].str, vm->Stack[vm->stacktop-1].ull);
		break;
	case TYPE_FLOAT:
		*(float *)((BYTE *)(&vm->context) + ContextDef[var].offset) = vm->Stack[vm->stacktop-1].f;
		ScriptExecTrace("assign context variable %s value = %f  type = float\n", ContextDef[var].str, vm->Stack[vm->stacktop-1].f);
		break;
	case TYPE_DOUBLE:
		*(double *)((BYTE *)(&vm->context) + ContextDef[var].offset) = vm->Stack[vm->stacktop-1].d;
		ScriptExecTrace("assign context variable %s value = %f  type = float\n", ContextDef[var].str, vm->Stack[vm->stacktop-1].d);
		break;
	case TYPE_LDOUBLE:
		*(long double *)((BYTE *)(&vm->context) + ContextDef[var].offset) = vm->Stack[vm->stacktop-1].ld;
		ScriptExecTrace("assign context variable %s value = %f  type = float\n", ContextDef[var].str, vm->Stack[vm->stacktop-1].ld);
		break;
	case TYPE_PTR:
	case TYPE_GAME:
	case TYPE_UNIT:
	case TYPE_STATS:
		*(void **)((BYTE *)(&vm->context) + ContextDef[var].offset) = vm->Stack[vm->stacktop-1].ptr;
		ScriptExecTrace("assign context variable %s value = %IX  type = pointer\n", ContextDef[var].str, vm->Stack[vm->stacktop-1].ptr);
		break;
	default:
		ASSERT_RETZERO(0);
		break;
	}
	VMInitState(vm);

	return 1;
}

//----------------------------------------------------------------------------
static inline int fp_VMI_SET_UNIT(
	VMSTATE * vm,
	BYTE_STREAM & str)
{
	ASSERT_RETZERO(vm->stacktop >= 0);
	vm->cur_stat_source = STAT_SOURCE_STACK_UNIT;
	return 1;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
typedef int FP_VMI(VMSTATE * vm, BYTE_STREAM & str);
struct VMI_TBL
{
	int			vmi;
	FP_VMI *	fptr;
};

VMI_TBL gVMITable[] =
{
#undef  VMI_DEF_SING
#undef  VMI_DEF_TYIN
#undef  VMI_DEF_TYNP
#undef  VMI_DEF_TYPE
#define VMI_DEF_SING(x)		{ x, fp_##x }
#define VMI_DEF_TYIN(x)		{ x##_CH, fp_##x##_CH }, { x##_UC, fp_##x##_UC }, { x##_SH, fp_##x##_SH }, { x##_US, fp_##x##_US }, { x##_IN, fp_##x##_IN }, { x##_UI, fp_##x##_UI }, \
								{ x##_LL, fp_##x##_LL }, { x##_UL, fp_##x##_UL }
#define VMI_DEF_TYNP(x)		{ x##_CH, fp_##x##_CH }, { x##_UC, fp_##x##_UC }, { x##_SH, fp_##x##_SH }, { x##_US, fp_##x##_US }, { x##_IN, fp_##x##_IN }, { x##_UI, fp_##x##_UI }, \
								{ x##_LL, fp_##x##_LL }, { x##_UL, fp_##x##_UL }, { x##_FL, fp_##x##_FL }, { x##_DB, fp_##x##_DB }, { x##_LD, fp_##x##_LD }
#define VMI_DEF_TYPE(x)		{ x##_CH, fp_##x##_CH }, { x##_UC, fp_##x##_UC }, { x##_SH, fp_##x##_SH }, { x##_US, fp_##x##_US }, { x##_IN, fp_##x##_IN }, { x##_UI, fp_##x##_UI }, \
								{ x##_LL, fp_##x##_LL }, { x##_UL, fp_##x##_UL }, { x##_FL, fp_##x##_FL }, { x##_DB, fp_##x##_DB }, { x##_LD, fp_##x##_LD }, { x##_POINTER, fp_##x##_POINTER }

#include "vmi.h"
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline int sVMExecI(
	VMSTATE * vmstate,
	BYTE_STREAM * stream)
{
	if (!stream->size)
	{
		return 0;
	}

	VMInitState(vmstate);

	BYTE_STREAM str = *stream;
	str.cur = 0;
	
	while (TRUE)
	{
		int op;
		if (!ByteStreamGetInt(&str, op))
		{
			return 0;
		}
		VMI instruction = (VMI)op;
		ASSERT_RETZERO(instruction >= 0 && instruction < arrsize(gVMITable));
		if (!gVMITable[instruction].fptr(vmstate, str))
		{
			return vmstate->retval.i;
		}
	}

	ASSERT(vmstate->callstacktop == 0);
	vmstate->callstacktop = 0;
	return 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMExecI(
	VMGAME * vm,
	BYTE_STREAM * stream)
{
	VMSTATE * vmstate = VMGetVMState(vm);
	ASSERT_RETZERO(vmstate);
	VMAutoReleaseState autolock(vmstate);
	vmstate->context.nState = INVALID_ID;
	vmstate->context.nSkill = INVALID_ID;
	vmstate->context.nSkillLevel = INVALID_SKILL_LEVEL;

	return sVMExecI(vmstate, stream);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMExecI(
	GAME * game,
	UNIT * unit,
	BYTE_STREAM * stream)
{
	ASSERT_RETFALSE(game);
	VMSTATE * vmstate = VMGetVMState(game->m_vm);
	ASSERT_RETFALSE(vmstate);
	VMAutoReleaseState autolock(vmstate);

	vmstate->context.game = game;
	vmstate->context.unit = unit;
	vmstate->context.nState = INVALID_ID;
	vmstate->context.nSkill = INVALID_ID;
	vmstate->context.nSkillLevel = INVALID_SKILL_LEVEL;
	return sVMExecI(vmstate, stream);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMExecI(
	GAME * game,
	BYTE * buf,
	int len,
	const char * errorstr)
{
	const char * default_errorstr = "";

	ASSERT_RETFALSE(game);
	VMSTATE * vmstate = VMGetVMState(game->m_vm);
	ASSERT_RETFALSE(vmstate);
	VMAutoReleaseState autolock(vmstate);

	vmstate->context.game = game;
	vmstate->context.nState = INVALID_ID;
	vmstate->context.nSkill = INVALID_ID;
	vmstate->context.nSkillLevel = INVALID_SKILL_LEVEL;
	vmstate->errorstr = errorstr ? errorstr : default_errorstr;

	BYTE_STREAM str;
	ByteStreamInit(NULL, str, buf, len);
	return sVMExecI(vmstate, &str);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMExecI(
	GAME * game,
	UNIT * unit,
	BYTE * buf,
	int len,
	const char * errorstr)
{
	const char * default_errorstr = "";

	ASSERT_RETFALSE(game);
	VMSTATE * vmstate = VMGetVMState(game->m_vm);
	ASSERT_RETFALSE(vmstate);
	VMAutoReleaseState autolock(vmstate);

	vmstate->context.game = game;
	vmstate->context.unit = unit;
	vmstate->context.nState = INVALID_ID;
	vmstate->context.nSkill = INVALID_ID;
	vmstate->context.nSkillLevel = INVALID_SKILL_LEVEL;
	vmstate->errorstr = errorstr ? errorstr : default_errorstr;

	BYTE_STREAM str;
	ByteStreamInit(NULL, str, buf, len);
	return sVMExecI(vmstate, &str);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMExecI(
	GAME * game,
	UNIT * unit,
	int nParam1,
	int nParam2,
	int nSkill,
	int nSkillLevel,
	int nState,
	BYTE * buf,
	int len,
	const char * errorstr)
{
	const char * default_errorstr = "";

	ASSERT_RETFALSE(game);
	VMSTATE * vmstate = VMGetVMState(game->m_vm);
	ASSERT_RETFALSE(vmstate);
	VMAutoReleaseState autolock(vmstate);

	vmstate->context.game = game;
	vmstate->context.unit = unit;
	vmstate->context.nParam1 = nParam1;
	vmstate->context.nParam2 = nParam2;
	vmstate->context.nSkill = nSkill;
	vmstate->context.nSkillLevel = nSkillLevel;
	vmstate->context.nState = nState;	
	vmstate->errorstr = errorstr ? errorstr : default_errorstr;

	BYTE_STREAM str;
	ByteStreamInit(NULL, str, buf, len);
	return sVMExecI(vmstate, &str);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMExecI(
	GAME * game,
	UNIT * unit,
	struct STATS * statslist,
	BYTE * buf,
	int len,
	const char * errorstr)
{
	const char * default_errorstr = "";

	ASSERT_RETFALSE(game);
	VMSTATE * vmstate = VMGetVMState(game->m_vm);
	ASSERT_RETFALSE(vmstate);
	VMAutoReleaseState autolock(vmstate);

	vmstate->context.game = game;
	vmstate->context.unit = unit;
	vmstate->context.stats = statslist;
	vmstate->context.nState = INVALID_ID;
	vmstate->context.nSkillLevel = INVALID_SKILL_LEVEL;
	vmstate->context.nSkill = INVALID_ID;
	vmstate->errorstr = errorstr ? errorstr : default_errorstr;

	BYTE_STREAM str;
	ByteStreamInit(NULL, str, buf, len);
	return sVMExecI(vmstate, &str);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMExecI(
	GAME * game,
	UNIT * unit,
	struct STATS * statslist,
	int nParam1,
	int nParam2,
	BYTE * buf,
	int len,
	const char * errorstr)
{
	const char * default_errorstr = "";

	ASSERT_RETFALSE(game);
	VMSTATE * vmstate = VMGetVMState(game->m_vm);
	ASSERT_RETFALSE(vmstate);
	VMAutoReleaseState autolock(vmstate);

	vmstate->context.game = game;
	vmstate->context.unit = unit;
	vmstate->context.stats = statslist;
	vmstate->context.nParam1 = nParam1;
	vmstate->context.nParam2 = nParam2;
	vmstate->context.nState = INVALID_ID;
	vmstate->context.nSkill = INVALID_ID;
	vmstate->context.nSkillLevel = INVALID_SKILL_LEVEL;
	vmstate->errorstr = errorstr ? errorstr : default_errorstr;

	BYTE_STREAM str;
	ByteStreamInit(NULL, str, buf, len);
	return sVMExecI(vmstate, &str);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMExecI(
	GAME * game,
	UNIT * unit,
	struct STATS * statslist,
	int nParam1,
	int nParam2,
	int nSkill,
	int nSkillLevel,
	int nState,
	BYTE * buf,
	int len,
	const char * errorstr)
{
	const char * default_errorstr = "";

	ASSERT_RETFALSE(game);
	VMSTATE * vmstate = VMGetVMState(game->m_vm);
	ASSERT_RETFALSE(vmstate);
	VMAutoReleaseState autolock(vmstate);

	vmstate->context.game = game;
	vmstate->context.unit = unit;
	vmstate->context.stats = statslist;
	vmstate->context.nParam1 = nParam1;
	vmstate->context.nParam2 = nParam2;
	vmstate->context.nSkill = nSkill;
	vmstate->context.nSkillLevel = nSkillLevel;
	vmstate->context.nState = nState;
	vmstate->errorstr = errorstr ? errorstr : default_errorstr;

	BYTE_STREAM str;
	ByteStreamInit(NULL, str, buf, len);
	return sVMExecI(vmstate, &str);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMExecI(
	GAME * game,
	UNIT * unit,
	UNIT * object,
	struct STATS * statslist,
	int nParam1,
	int nParam2,
	int nSkill,
	int nSkillLevel,
	int nState,
	BYTE * buf,
	int len,
	const char * errorstr)
{
	const char * default_errorstr = "";

	ASSERT_RETFALSE(game);
	VMSTATE * vmstate = VMGetVMState(game->m_vm);
	ASSERT_RETFALSE(vmstate);
	VMAutoReleaseState autolock(vmstate);

	vmstate->context.game = game;
	vmstate->context.unit = unit;
	vmstate->context.object = object;
	vmstate->context.stats = statslist;
	vmstate->context.nParam1 = nParam1;
	vmstate->context.nParam2 = nParam2;
	vmstate->context.nSkill = nSkill;
	vmstate->context.nSkillLevel = nSkillLevel;
	vmstate->context.nState = nState;
	vmstate->errorstr = errorstr ? errorstr : default_errorstr;

	BYTE_STREAM str;
	ByteStreamInit(NULL, str, buf, len);
	return sVMExecI(vmstate, &str);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMExecI(
	GAME * game,
	UNIT * subject,
	UNIT * object,
	struct STATS * statslist,
	BYTE * buf,
	int len,
	const char * errorstr)
{
	const char * default_errorstr = "";

	if (!game)
	{
		ASSERT_RETFALSE(subject == NULL && object == NULL);
		return VMExecI(statslist, buf, len, errorstr);
	}
	VMSTATE * vmstate = VMGetVMState(game->m_vm);
	ASSERT_RETFALSE(vmstate);
	VMAutoReleaseState autolock(vmstate);

	vmstate->context.game = game;
	vmstate->context.unit = subject;
	vmstate->context.object = object;
	vmstate->context.stats = statslist;
	vmstate->context.nSkill = INVALID_ID;
	vmstate->context.nSkillLevel = INVALID_SKILL_LEVEL;
	vmstate->context.nState = INVALID_ID;
	vmstate->errorstr = errorstr ? errorstr : default_errorstr;

	BYTE_STREAM str;
	ByteStreamInit(NULL, str, buf, len);
	return sVMExecI(vmstate, &str);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMExecI(
	struct STATS * statslist,
	BYTE * buf,
	int len,
	const char * errorstr)
{
	const char * default_errorstr = "";

	VMSTATE * vmstate = VMGetVMState(gVM.vm);
	ASSERT_RETFALSE(vmstate);
	VMAutoReleaseState autolock(vmstate);

	vmstate->context.stats = statslist;
	vmstate->context.nSkill = INVALID_ID;
	vmstate->context.nSkillLevel = INVALID_SKILL_LEVEL;
	vmstate->context.nState = INVALID_ID;
	vmstate->errorstr = errorstr ? errorstr : default_errorstr;

	BYTE_STREAM str;
	ByteStreamInit(NULL, str, buf, len);
	return sVMExecI(vmstate, &str);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMExecI(
	struct STATS * statslist,
	int nParam1,
	int nParam2,
	BYTE * buf,
	int len,
	const char * errorstr)
{
	const char * default_errorstr = "";

	VMSTATE * vmstate = VMGetVMState(gVM.vm);
	ASSERT_RETFALSE(vmstate);
	VMAutoReleaseState autolock(vmstate);

	vmstate->context.stats = statslist;
	vmstate->context.nSkill = INVALID_ID;
	vmstate->context.nParam1 = nParam1;
	vmstate->context.nParam2 = nParam2;
	vmstate->context.nSkillLevel = INVALID_SKILL_LEVEL;
	vmstate->context.nState = INVALID_ID;
	vmstate->errorstr = errorstr ? errorstr : default_errorstr;

	BYTE_STREAM str;
	ByteStreamInit(NULL, str, buf, len);
	return sVMExecI(vmstate, &str);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int VMExecIUnitObjectBuffer(
	struct GAME * game,
	struct UNIT * unit,
	struct UNIT * object,
	BYTE * buf,
	int len,
	const char * errorstr /*= NULL*/)
{
	const char * default_errorstr = "";

	ASSERT_RETFALSE(game);
	VMSTATE * vmstate = VMGetVMState(game->m_vm);
	ASSERT_RETFALSE(vmstate);
	VMAutoReleaseState autolock(vmstate);

	vmstate->context.game = game;
	vmstate->context.unit = unit;
	vmstate->context.object = object;
	vmstate->context.nState = INVALID_ID;
	vmstate->context.nSkill = INVALID_ID;
	vmstate->context.nSkillLevel = INVALID_SKILL_LEVEL;
	vmstate->errorstr = errorstr ? errorstr : default_errorstr;

	BYTE_STREAM str;
	ByteStreamInit(NULL, str, buf, len);
	return sVMExecI(vmstate, &str);

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int ClientControlUnitVMExecI(
	BYTE * buf,
	int len)
{
	GAME * game = AppGetCltGame();
	if(!game)
		return FALSE;

	UNIT * player = GameGetControlUnit(game);
	if(!player)
		return FALSE;

	return VMExecI(game, player, buf, len);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int ScriptTest(
	void)
{
/*
	while (TRUE)
	{
		char input[4096];
		trace("> ");
		gets(input);
		if (stricmp(input, "quit") == 0 
			|| stricmp(input, "q") == 0
			|| stricmp(input, "exit") == 0)
		{
			break;
		}
		BYTE_STREAM * stream = ByteStreamInit(NULL);
		if (ScriptParseEx(vm, stream, input))
		{
			trace("%d\n", VMExecI(vm, stream));
		}
		ByteStreamFree(stream);
	}
*/
	return 0;
}

