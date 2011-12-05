//----------------------------------------------------------------------------
// target.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _TARGET_H_
#define _TARGET_H_



//----------------------------------------------------------------------------
// ENUMERATIONS
//----------------------------------------------------------------------------
enum TARGET_TYPE
{
	TARGET_INVALID = -1,
	TARGET_FIRST = 0,
	
	TARGET_NONE = TARGET_FIRST,		// missiles, etc
	TARGET_GOOD,
	TARGET_BAD,
	TARGET_OBJECT,
	TARGET_DEAD,
	TARGET_MISSILE,

	NUM_TARGET_TYPES,

	TARGET_COLLISIONTEST_START = TARGET_GOOD,
};

enum
{
	TARGETMASK_GOOD =			MAKE_MASK(TARGET_GOOD),
	TARGETMASK_BAD =			MAKE_MASK(TARGET_BAD),
	TARGETMASK_OBJECT =			MAKE_MASK(TARGET_OBJECT),
	TARGETMASK_DEAD =			MAKE_MASK(TARGET_DEAD),
};


//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------
struct TARGET_NODE
{
	struct UNIT		*pPrev;
	struct UNIT		*pNext;
};


struct TARGET_LIST
{
	struct UNIT		*ppFirst[NUM_TARGET_TYPES];
};


//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------


#endif //_TARGET_H_