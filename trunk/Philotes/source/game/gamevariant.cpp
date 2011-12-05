//----------------------------------------------------------------------------
// FILE: gamevariant.cpp
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "difficulty.h"
#include "game.h"
#include "gamevariant.h"
#include "player.h"
#include "units.h"

//----------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// The following game variant flags are set on characters and can never be unset
//----------------------------------------------------------------------------
static GAME_VARIANT_TYPE sgeStaticVariantTypeLookup[] = 
{
	GV_HARDCORE,
	GV_ELITE,
	GV_LEAGUE,
};
static const int sgnNumSavedVariantTypes = arrsize( sgeStaticVariantTypeLookup );

//----------------------------------------------------------------------------
// Function Definitions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void GameVariantInit(
	GAME_VARIANT &tGameVariant,
	UNIT *pUnit)
{

	// set to defaults
	tGameVariant.nDifficultyCurrent = GlobalIndexGet( GI_DIFFICULTY_DEFAULT );
	tGameVariant.dwGameVariantFlags = 0;
	
	// init if we have a unit from unit data
	if (pUnit)
	{

		tGameVariant.nDifficultyCurrent = DifficultyGetCurrent( pUnit );
		ASSERTX( tGameVariant.nDifficultyCurrent != INVALID_LINK, "Expected current difficulty for unit" );	
		
		// set game variant information
		SETBIT( tGameVariant.dwGameVariantFlags, GV_HARDCORE, PlayerIsHardcore( pUnit ));
		SETBIT( tGameVariant.dwGameVariantFlags, GV_ELITE, PlayerIsElite( pUnit ));
		SETBIT( tGameVariant.dwGameVariantFlags, GV_LEAGUE, PlayerIsLeague( pUnit ));
		SETBIT( tGameVariant.dwGameVariantFlags, GV_PVPWORLD, PlayerIsInPVPWorld( pUnit ));

	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD s_GameVariantFlagsGetFromNewPlayerFlags(
	APPCLIENTID idAppClient,
	DWORD dwNewPlayerFlags)
{
	DWORD dwGameVariantFlags = 0;
	
	if (dwNewPlayerFlags & NPF_HARDCORE)
	{
		SETBIT( dwGameVariantFlags, GV_HARDCORE );
	}
	if (dwNewPlayerFlags & NPF_PVPONLY)
	{
		SETBIT( dwGameVariantFlags, GV_PVPWORLD );
	}
	if (dwNewPlayerFlags & NPF_LEAGUE)
	{
		SETBIT( dwGameVariantFlags, GV_LEAGUE );
	}
	if (dwNewPlayerFlags & NPF_ELITE)
	{
		SETBIT( dwGameVariantFlags, GV_ELITE );
	}

	// maybe validate that this appclient can actually do it here?
	
	return dwGameVariantFlags;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL GameVariantsAreEqual(
	const GAME_VARIANT &tVariant1,
	const GAME_VARIANT &tVariant2,
	DWORD dwGameVariantCompareFlags /*= GVC_ALL*/,
	DWORD *pdwDifferences /*= NULL*/)
{

	// init differences (if present)
	if (pdwDifferences)
	{
		*pdwDifferences = 0;
	}

	// check difficulty if requested
	if (dwGameVariantCompareFlags & GVC_DIFFICULTY)
	{
		// difficulty must match	
		if (tVariant1.nDifficultyCurrent != tVariant2.nDifficultyCurrent)
		{
			return FALSE;
		}
	}

	// check variant flags if requested
	if (dwGameVariantCompareFlags & GVC_VARIANT_TYPE)
	{
		
		// elite must match
		if (TESTBIT( tVariant1.dwGameVariantFlags, GV_ELITE ) !=
			TESTBIT( tVariant2.dwGameVariantFlags, GV_ELITE ))
		{
			if (pdwDifferences)
			{
				SETBIT(*pdwDifferences, GV_ELITE, TRUE);
			}
			return FALSE;
		}

		// league must match
		if (TESTBIT( tVariant1.dwGameVariantFlags, GV_LEAGUE ) !=
			TESTBIT( tVariant2.dwGameVariantFlags, GV_LEAGUE ))
		{
			if (pdwDifferences)
			{
				SETBIT(*pdwDifferences, GV_LEAGUE, TRUE);
			}	
			return FALSE;
		}

		// hardcore must match
		if (TESTBIT( tVariant1.dwGameVariantFlags, GV_HARDCORE ) !=
			TESTBIT( tVariant2.dwGameVariantFlags, GV_HARDCORE ))
		{
			if (pdwDifferences)
			{
				SETBIT(*pdwDifferences, GV_HARDCORE, TRUE);
			}		
			return FALSE;
		}

	}
		
	return TRUE;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD GameVariantFlagsGet(
	UNIT *pPlayer)
{
	ASSERTX_RETZERO( pPlayer, "Expected unit" );
	ASSERTX_RETZERO( UnitIsPlayer( pPlayer ), "Expected player" );

	// get the game variant for this player	
	GAME_VARIANT tVariant;
	GameVariantInit( tVariant, pPlayer );
	return tVariant.dwGameVariantFlags;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD GameVariantFlagsGetStaticUnit(
		UNIT *pPlayer)
{
	ASSERTX_RETZERO( pPlayer, "Expected unit" );
	ASSERTX_RETZERO( UnitIsPlayer( pPlayer ), "Expected player" );

	// get the game variant for this player	
	GAME_VARIANT tVariant;
	GameVariantInit( tVariant, pPlayer );
	return GameVariantFlagsGetStatic( tVariant.dwGameVariantFlags );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD GameVariantFlagsGetStatic(
	DWORD dwGameVariantFlags)
{
	
	// not all game variant flags are permanent values that should be saved, only
	// set bits that are in the table and the variant itself
	DWORD dwGameVariantFlagsStatic = 0;
	for (int i = 0; i < sgnNumSavedVariantTypes; ++i)
	{
		GAME_VARIANT_TYPE eVariant = sgeStaticVariantTypeLookup[ i ];
		if (TESTBIT( dwGameVariantFlags, eVariant ))
		{
			SETBIT( dwGameVariantFlagsStatic, eVariant );
		}
	}

	// return the modifed bits
	return dwGameVariantFlagsStatic;
		
}
