//----------------------------------------------------------------------------
// FILE: s_adventure.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifdef __S_ADVENTURE_H_
//#pragma message ("    HINT: " __FILE__ " included multiple times in this translation unit")
#else
#define __S_ADVENTURE_H_

//----------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------

void s_AdventurePopulate(
	LEVEL *pLevel,
	UNIT *pPlayerCreator);

int s_AdventureGetPassageDest( 
	UNIT *pPassage);

int s_AdventurePassageGetTreasureClass( 
	UNIT *pWarpToDestLevel);

#endif