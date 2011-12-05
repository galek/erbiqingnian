//----------------------------------------------------------------------------
// BASIC TYPES
//----------------------------------------------------------------------------
// type				str						vmi offset
DEF(TYPE_VOID,		"void",					0)
DEF(TYPE_CHAR,		"char",					0)
DEF(TYPE_BYTE,		"unsigned char",		1)
DEF(TYPE_SHORT,		"short",				2)
DEF(TYPE_WORD,		"unsigned short",		3)
DEF(TYPE_INT,		"int",					4)
DEF(TYPE_DWORD,		"unsigned int",			5)
DEF(TYPE_INT64,		"int64",				6)
DEF(TYPE_QWORD,		"unsigned int64",		7)
DEF(TYPE_FLOAT,		"float",				8)
DEF(TYPE_DOUBLE,	"double",				9)
DEF(TYPE_LDOUBLE,	"long double",			10)
DEF(TYPE_BOOL,		"bool",					4)
DEF(TYPE_ENUM,		"enum",					4)
DEF(TYPE_PTR,		"ptr",					11)
DEF(TYPE_FUNC,		"func",					0)
DEF(TYPE_STRUCT,	"struct",				0)
DEF(TYPE_TYPEDEF,	"typedef",				0)
// extern types
DEF(TYPE_GAME,		"GAME",					11)
DEF(TYPE_UNIT,		"UNIT",					11)
DEF(TYPE_STATS,		"STATS",				11)
