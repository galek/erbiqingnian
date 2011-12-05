//----------------------------------------------------------------------------
// script_priv.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------
struct SCRIPT_CONTEXT
{
	struct GAME *					game;
	struct UNIT *					unit;
	struct UNIT *					object;
	struct STATS *					stats;
	int								nParam1;
	int								nParam2;
	int								nSkill;
	int								nSkillLevel;
	int								nState;
};

// value
union VALUE
{
	float							f;
	double							d;
	long double						ld;
	char							c;
	unsigned char					uc;
	short							s;
	unsigned short					us;
	int								i;
	unsigned int					ui;
	INT64							ll;
	UINT64							ull;

	void *							ptr;
	char *							str;				// string table entry
	struct SSYMBOL *				sym;				// symbol
};

// byte-stream
struct BYTE_STREAM
{
	MEMORYPOOL *					pool;				// memory pool
	BYTE *							buf;				// byte buffer
	unsigned int					cur;				// cursor position (length of buffer filled when writing)
	unsigned int					size;				// allocated size of buffer
	union
	{
		BOOL						bDynamic;			// can resize?  -- used only during parsing
		struct		
		{
			unsigned int			localstack;			// amount to subtract from localcur on return -- used only during execution
			unsigned int			valuestack;			// top of value stack before call
		};
	};
};


// current VM (execute) state
struct VMSTATE
{
	struct MEMORYPOOL *				m_MemPool;
	struct VMGAME *					m_vm;

	BYTE *							Globals;			// global variables

	BYTE *							Locals;				// local variables
	unsigned int					localcur;			// current size
	unsigned int					localsize;			// allocated size

	VALUE *							Stack;				// compute stack
	unsigned int					stacktop;			// top of stack
	unsigned int					stacksize;			// allocated size
	unsigned int					stackthis;			// local compute stack top for a function

	BYTE_STREAM *					CallStack;			// call stack
	unsigned int					callstacktop;		// current top
	unsigned int					callstacksize;		// allocated size

	VALUE							retval;				// return value
	const char *					errorstr;

	int								cur_stat_source;	// where to read & set stats (1 = context.stat, 2 = context.unit, 3 = context.object, 4 = unit off stack)
	int								pre_stat_source;	// stat source memory

	SCRIPT_CONTEXT					context;
};


// VM global state (per game)
struct VMGAME
{
	struct MEMORYPOOL *				m_MemPool;			// memory pool
	BYTE *							m_Globals;			// global variables
	VMSTATE	**						m_VMStateStack;
	unsigned int					m_VMStateStackCur;
	unsigned int					m_VMStateStackSize;	// max initialized index
};