//----------------------------------------------------------------------------
// FILE: s_passage.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "level.h"
#include "s_passage.h"
#include "units.h"
#include "warp.h"

//----------------------------------------------------------------------------
// Function Definitions
//----------------------------------------------------------------------------
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sPassageInit(
	UNIT *pPassage,
	UNIT *pPlayer)
{
	
	// what is the destination of this passageway
	LEVELID idLevel = UnitGetStat( pPassage, STATS_WARP_DEST_LEVEL_ID );
	if (idLevel == INVALID_ID)
	{

		// init this passage warp by just adding it to the level
		LevelAddWarp( pPassage, UnitGetId( pPlayer ), WRP_ON_USE );
				
	}

	// must have a level at this point
	idLevel = UnitGetStat( pPassage, STATS_WARP_DEST_LEVEL_ID );	
	ASSERTX_RETURN( idLevel != INVALID_ID, "Expected level id" );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_PassageEnter(
	UNIT *pPlayer,
	UNIT *pPassage)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( pPassage, "Expected unit" );
	ASSERTX_RETURN( UnitIsA( pPassage, UNITTYPE_PASSAGE_ENTER ), "Expected passage entrance" );
	
	// init passage if we need to
	sPassageInit( pPassage, pPlayer );
		
	// passageways are really just warps
	s_WarpEnterWarp( pPlayer, pPassage );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_PassageExit(
	UNIT *pPlayer,
	UNIT *pPassage)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( pPassage, "Expected unit" );
	ASSERTX_RETURN( UnitIsA( pPassage, UNITTYPE_PASSAGE_EXIT ), "Expected passage exit" );

	// init passage if we need to
	sPassageInit( pPassage, pPlayer );

	// again, a passage exit is just a warp
	s_WarpEnterWarp( pPlayer, pPassage );
		
}
