//----------------------------------------------------------------------------
// FILE: s_passage.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifdef __S_PASSAGE_H_
//#pragma message ("    HINT: " __FILE__ " included multiple times in this translation unit")
#else
#define __S_PASSAGE_H_

//----------------------------------------------------------------------------
// Forward Declarations
//----------------------------------------------------------------------------
struct UNIT;

//----------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------

void s_PassageEnter(
	UNIT *pPlayer,
	UNIT *pPassage);

void s_PassageExit(
	UNIT *pPlayer,
	UNIT *pPassage);

#endif