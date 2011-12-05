//----------------------------------------------------------------------------
// FILE: gameconfig.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef _GAMECONFIG_H_
#define _GAMECONFIG_H_

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#ifndef _DEFINITION_COMMON_H_
#include "definition_common.h"
#endif


//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
typedef struct GAME_GLOBAL_DEFINITION 
{
	DEFINITION_HEADER	tHeader;
	int					nLevelDefinition;		// index of row in levels.xls
	int					nDRLGOverride;
	int					nRoomOverride;
} GAME_GLOBAL_DEFINITION;

#endif // _GAMECONFIG_H_