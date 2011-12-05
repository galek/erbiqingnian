//----------------------------------------------------------------------------
// FILE: commontypes.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// This file is included as part of the pre-compiled headers.  Therefore
// anything you do here will rebuild a whole lot of stuff.
//
// The purpose of this file is to house some basic typedefs so that we
// don't have to have headers including lots of other headers
//
//----------------------------------------------------------------------------
#ifndef _COMMONTYPES_H_
#define _COMMONTYPES_H_


//----------------------------------------------------------------------------
typedef unsigned __int64				QWORD;
typedef unsigned __int64				PTIME;


// An aligned, padded pointer that works for serialized structures created on x86 or x64.
#define SAFE_PTR(t, p) \
	/*DECLSPEC_ALIGN(8)*/ union \
	{ \
		t p; \
		t POINTER_64 __p64_##p; \
	}

#define SAFE_FUNC_PTR(t, p) \
	union \
	{ \
		t p; \
		QWORD __pad128_##p[2]; \
	}

#define EXCEL_STR(p)					SAFE_PTR(const char *, p)


// padded pointer used for structures saved to file
typedef int								LEVELID;						// id of level *instance* in the game
typedef int								SUBLEVELID;						// id of sublevel instance
typedef int								GAMETASKID;
typedef int								TASK_TEMPLATE_ID;


typedef DWORD							SPECIES;						// see macros GET_SPECIES_CLASS and GET_SPECIES_GENUS to get the class and genus out of this dword
#define MAKE_SPECIES(g, c)				(SPECIES)(((g) << 24) + c)
#define GET_SPECIES_CLASS(s)			((s) & 0x00ffffff)
#define GET_SPECIES_GENUS(s)			((s) >> 24)
#define INVALID_SPECIES_CLASS			(0x00ffffff)
typedef DWORD_PTR						HJOB;


//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
#define MSECS_PER_SEC					1000
#define SECONDS_PER_MINUTE				60

const int		NONE = -1;
const int		INVALID_LINK = -1;
const int		MONSTER_UNIQUE_NAME_INVALID = -1;
const int		INVALID_INDEX = -1;

const int		MAX_FUNCTION_STRING_LENGTH = 64;

const int		FACTION_MAX_FACTIONS = 16;	// arbitrary, increase as needed (I only put this here instead of faction.h because a network message header wants it)
const SPECIES	SPECIES_NONE = MAKE_SPECIES(-1, -1);

const int		GAMETASKID_INVALID = -1;
const int		TASKCODE_INVALID = -1;

const unsigned int INVALID_TEAM = (unsigned int)-1;

const int		MAX_ROOMS_PER_LEVEL	= 3200;


//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// used for model sorting in tugboat
enum MODEL_SORT_TYPE
{
	MODEL_STATIC,
	MODEL_DYNAMIC,
	MODEL_ICONIC,
	MODEL_NONSTATIC,
	MODEL_ALL
};


//----------------------------------------------------------------------------
// used for stringtables and language
enum GRAMMAR_NUMBER
{
	SINGULAR,
	PLURAL
};


//----------------------------------------------------------------------------
// unit genus
enum GENUS
{
	GENUS_NONE		= 0,	// saved in database, do not change value
	GENUS_PLAYER	= 1,	// saved in database, do not change value
	GENUS_MONSTER	= 2,	// saved in database, do not change value
	GENUS_MISSILE	= 3,	// saved in database, do not change value
	GENUS_ITEM		= 4,	// saved in database, do not change value
	GENUS_OBJECT	= 5,	// saved in database, do not change value
	
	NUM_GENUS,
};

//----------------------------------------------------------------------------
// write a stats list to a buffer
enum STATS_WRITE_METHOD
{
	STATS_WRITE_METHOD_INVALID = -1,
	
	STATS_WRITE_METHOD_FILE,
	STATS_WRITE_METHOD_NETWORK,
	STATS_WRITE_METHOD_PAK,
	STATS_WRITE_METHOD_DATABASE,
	
	NUM_STATS_WRITE_METHODS		// keep this last please
};


#endif