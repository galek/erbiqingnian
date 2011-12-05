//----------------------------------------------------------------------------
// KEYWORDS
//----------------------------------------------------------------------------

DEF(TOK_IF,			"if")
DEF(TOK_ELSE,		"else")
DEF(TOK_WHILE,		"while")
DEF(TOK_BREAK,		"break")
DEF(TOK_RETURN,		"return")
DEF(TOK_FOR,		"for")
DEF(TOK_GOTO,		"goto")
DEF(TOK_DO,			"do")
DEF(TOK_CONTINUE,	"continue")
DEF(TOK_SWITCH,		"switch")
DEF(TOK_CASE,		"case")

DEF(TOK_STATIC,		"static")
DEF(TOK_UNSIGNED,	"unsigned")
DEF(TOK_CONST1,		"const")
DEF(TOK_LONG,		"long")

DEF(TOK_VOID,		"void")
DEF(TOK_SHORT,		"short")
DEF(TOK_FLOAT,		"float")
DEF(TOK_DOUBLE,		"double")
DEF(TOK_BOOL,		"bool")
DEF(TOK_CHAR,		"char")
DEF(TOK_INT,		"int")

DEF(TOK_STRUCT,		"struct")
DEF(TOK_UNION,		"union")
DEF(TOK_TYPEDEF,	"typedef")
DEF(TOK_DEFAULT,	"default")
DEF(TOK_ENUM,		"enum")
DEF(TOK_SIZEOF,		"sizeof")
DEF(TOK_TYPEOF1,	"typeof")
DEF(TOK_LABEL,		"__label__")


//----------------------------------------------------------------------------
// the following are not keywords. They are included to ease parsing
// preprocessor
DEF(TOK_DEFINE,		"define")
DEF(TOK_INCLUDE,	"include")
DEF(TOK_IFDEF,		"ifdef")
DEF(TOK_IFNDEF,		"ifndef")
DEF(TOK_ELIF,		"elif")
DEF(TOK_ENDIF,		"endif")
DEF(TOK_DEFINED,	"defined")
DEF(TOK_UNDEF,		"undef")
DEF(TOK_ERROR,		"error")
DEF(TOK_WARNING,	"warning")
DEF(TOK_LINE,		"line")
DEF(TOK_PRAGMA,		"pragma")
DEF(TOK___LINE__,	"__LINE__")
DEF(TOK___FILE__,	"__FILE__")
DEF(TOK___DATE__,	"__DATE__")
DEF(TOK___TIME__,	"__TIME__")
DEF(TOK___FUNCTION__, "__FUNCTION__")
DEF(TOK___VA_ARGS__, "__VA_ARGS__")
