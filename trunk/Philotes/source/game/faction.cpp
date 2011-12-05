//----------------------------------------------------------------------------
// FILE: faction.cpp
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "console.h"
#include "excel.h"
#include "faction.h"
#include "globalindex.h"
#include "level.h"
#include "stats.h"
#include "stringtable.h"
#include "units.h"
#include "unit_priv.h"
#include "s_quests.h"
#include "objects.h"
#if ISVERSION(SERVER_VERSION)
#include "faction.cpp.tmh"
#endif
#include "excel_private.h"


//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct FACTION_GLOBALS
{
	int nFactionStandingMax;		// the best standing you can have
};
static FACTION_GLOBALS tFactionGlobals;

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static FACTION_GLOBALS *sFactionGlobalsGet(
	void)
{
	return &tFactionGlobals;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL FactionExcelPostProcess(
	EXCEL_TABLE * table)
{
	ASSERT_RETFALSE(table);
	ASSERTX_RETFALSE(ExcelGetCountPrivate(table) < FACTION_MAX_FACTIONS, "Too many factions, increase FACTION_MAX_FACTIONS\n");

	return TRUE;	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL FactionStandingExcelPostProcess(
	EXCEL_TABLE * table)
{
	ASSERT_RETFALSE(table);

	int nMaxScore = 0;
	int nStandingMax = INVALID_LINK;
	for (unsigned int ii = 0; ii < ExcelGetCountPrivate(table); ++ii)
	{
		const FACTION_STANDING_DEFINITION * faction_standing = (const FACTION_STANDING_DEFINITION *)ExcelGetDataPrivate(table, ii);

		// is this the highest score found so far
		if (nStandingMax == INVALID_LINK || faction_standing->nMaxScore > nMaxScore)
		{
			nStandingMax = ii;
			nMaxScore = faction_standing->nMaxScore;
		}
	}
	
	// assign the best standing
	FACTION_GLOBALS * faction_globals = sFactionGlobalsGet();
	ASSERT_RETFALSE(faction_globals);
	faction_globals->nFactionStandingMax = nStandingMax;

	return TRUE;
}

	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const WCHAR *FactionGetDisplayName(
	int nFaction)
{
	const FACTION_DEFINITION *pFactionDef = FactionGetDefinition( nFaction );
	ASSERT_RETVAL(pFactionDef != NULL, L"");
	return StringTableGetStringByIndex( pFactionDef->nDisplayString );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const WCHAR *FactionStandingGetDisplayName(
	int nFactionStanding)
{
	const FACTION_STANDING_DEFINITION *pFactionStandingDef = FactionStandingGetDefinition( nFactionStanding );
	ASSERT_RETVAL(pFactionStandingDef != NULL, L"");
	return StringTableGetStringByIndex( pFactionStandingDef->nDisplayString );
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int FactionGetNumFactions(
	void)
{
	return ExcelGetNumRows( NULL, DATATABLE_FACTION );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const FACTION_DEFINITION *FactionGetDefinition(
	int nFaction)
{
	return (FACTION_DEFINITION *)ExcelGetData( NULL, DATATABLE_FACTION, nFaction );
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int FactionGetNumStandings(
	void)
{
	return ExcelGetNumRows( NULL, DATATABLE_FACTION_STANDING );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const FACTION_STANDING_DEFINITION *FactionStandingGetDefinition(
	int nFactionStanding)
{
	return (FACTION_STANDING_DEFINITION *)ExcelGetData( NULL, DATATABLE_FACTION_STANDING, nFactionStanding );
}
		
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const WCHAR * FactionScoreToDecoratedStandingName(
	int nFactionScore)
{

	// go through faction standing table
	int nNumStandings = FactionGetNumStandings();
	for (int i = 0; i < nNumStandings; ++i)
	{
		const FACTION_STANDING_DEFINITION *pFactionStandingDef = FactionStandingGetDefinition( i );
		ASSERT_CONTINUE(pFactionStandingDef);
		
		// compare the score ranges (note that -1 means unbounded)
		if (nFactionScore >= pFactionStandingDef->nMinScore &&
			(nFactionScore <= pFactionStandingDef->nMaxScore || pFactionStandingDef->nMaxScore == -1))
		{
			return StringTableGetStringByIndex(pFactionStandingDef->nDisplayString);
		}
	}
	
	ASSERTX_RETVAL( 0, L"", "Unable to match faction score with any standing" );
	
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sFactionScoreToStanding(
	int nFactionScore)
{

	// go through faction standing table
	int nNumStandings = FactionGetNumStandings();
	for (int i = 0; i < nNumStandings; ++i)
	{
		const FACTION_STANDING_DEFINITION *pFactionStandingDef = FactionStandingGetDefinition( i );
		ASSERT_CONTINUE(pFactionStandingDef);
		
		// compare the score ranges (note that -1 means unbounded)
		if (nFactionScore >= pFactionStandingDef->nMinScore &&
			(nFactionScore <= pFactionStandingDef->nMaxScore || pFactionStandingDef->nMaxScore == -1))
		{
			return i;
		}
	}
	
	ASSERTX_RETVAL( 0, INVALID_LINK, "Unable to match faction score with any standing" );
	
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int FactionGetStanding(
	UNIT *pUnit,
	int nFaction)
{
	ASSERTX_RETINVALID( pUnit, "Expected unit" );
	ASSERTX_RETINVALID( nFaction >= 0 && nFaction < FactionGetNumFactions(), "Invalid faction" );
	
	// get the faction score for this faction on the unit
	int nFactionScore = FactionScoreGet( pUnit, nFaction );
	if (nFactionScore == FACTION_SCORE_NONE)
	{
		return INVALID_LINK;
	}
		
	// translate the raw faction score into a standing
	return sFactionScoreToStanding( nFactionScore );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
FACTION_STANDING_MOOD FactionGetMood(
	UNIT *pUnit,
	int nFaction)
{
	ASSERTX_RETVAL( pUnit, FSM_INVALID, "Expected unit" );
	ASSERTX_RETVAL( nFaction >= 0 && nFaction < FactionGetNumFactions(), FSM_INVALID, "Invalid faction" );

	// get standing with faction
	int nFactionStanding = FactionGetStanding( pUnit, nFaction );

	// get mood of standing
	return FactionStandingGetMood( nFactionStanding );

}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL FactionStandingIsBetterThan(
	int nFactionStanding,
	int nFactionStandingOther)
{

	// check for no standings at all
	if (nFactionStanding == INVALID_LINK || nFactionStandingOther == INVALID_LINK)
	{
		return FALSE;
	}

	// get definitions	
	const FACTION_STANDING_DEFINITION *pFactionStandingDef = FactionStandingGetDefinition( nFactionStanding );
	const FACTION_STANDING_DEFINITION *pFactionStandingOtherDef = FactionStandingGetDefinition( nFactionStandingOther );
	ASSERT_RETFALSE(pFactionStandingDef);
	ASSERT_RETTRUE(pFactionStandingOtherDef);

	// check better than	
	if (pFactionStandingDef->nMinScore > pFactionStandingOtherDef->nMaxScore)
	{
		return TRUE;
	}
	
	return FALSE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL FactionStandingIsWorseThan(
	int nFactionStanding,
	int nFactionStandingOther)
{

	// check for no standings at all
	if (nFactionStanding == INVALID_LINK || nFactionStandingOther == INVALID_LINK)
	{
		return FALSE;
	}

	// get definitions
	const FACTION_STANDING_DEFINITION *pFactionStandingDef = FactionStandingGetDefinition( nFactionStanding );
	const FACTION_STANDING_DEFINITION *pFactionStandingOtherDef = FactionStandingGetDefinition( nFactionStandingOther );
	ASSERT_RETTRUE(pFactionStandingDef);
	ASSERT_RETFALSE(pFactionStandingOtherDef);

	// check worse than	
	if (pFactionStandingDef->nMaxScore < pFactionStandingOtherDef->nMinScore)
	{
		return TRUE;
	}
	
	return FALSE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
FACTION_STANDING_MOOD FactionStandingGetMood(
	int nFactionStanding)
{

	// sanity
	if (nFactionStanding == INVALID_LINK)
	{
		return FSM_MOOD_NEUTRAL;
	}

	const FACTION_STANDING_DEFINITION *pFactionStandingDef = FactionStandingGetDefinition( nFactionStanding );
	ASSERT_RETVAL(pFactionStandingDef, FSM_MOOD_NEUTRAL);
	return pFactionStandingDef->eMood;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sFactionGetMaxScore(
	void)
{
	const FACTION_GLOBALS *pFactionGlobals = sFactionGlobalsGet();
	ASSERT_RETZERO(pFactionGlobals);
	const FACTION_STANDING_DEFINITION *pFactionStandingDef = FactionStandingGetDefinition( pFactionGlobals->nFactionStandingMax );
	ASSERT_RETZERO(pFactionStandingDef);
	return pFactionStandingDef->nMaxScore;	
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sFactionScoreSet(
	UNIT *pUnit,
	int nFaction,
	int nScoreAbsolute)
{
	
	// do not allow to go below zero
	if (nScoreAbsolute < 0)
	{
		nScoreAbsolute = 0;
	}

	// cap at the max
	int nFactionMaxScore = sFactionGetMaxScore();
	if (nScoreAbsolute > nFactionMaxScore)
	{
		nScoreAbsolute = nFactionMaxScore;
	}
	
	// set the stat explicitly
	UnitSetStat( pUnit, STATS_FACTION_SCORE, nFaction, nScoreAbsolute );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sFactionScoreChange(
	UNIT *pUnit,
	int nFaction,
	int nScoreDelta)
{

	// get current faction score
	int nFactionScore = FactionScoreGet( pUnit, nFaction );
	
	// what is their new faction score
	int nFactionScoreNew = nFactionScore + nScoreDelta;
		
	// set new value
	sFactionScoreSet( pUnit, nFaction, nFactionScoreNew );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int FactionScoreGet(
	UNIT *pUnit,
	int nFaction)
{
	return UnitGetStat( pUnit, STATS_FACTION_SCORE, nFaction );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void FactionScoreChange(
	UNIT *pUnit,
	int nFaction,
	int nScoreDelta)
{

	// change score with this faction
	sFactionScoreChange( pUnit, nFaction, nScoreDelta );

/*	drb - removed auto-decrease of other factions
	// when gaining faction points, we will also lose points with the other factions
	if (nScoreDelta > 0)
	{
	
		// how many other factions are there
		int nNumFactions = FactionGetNumFactions();
		if (nNumFactions > 1)
		{
		
			// split the points evenly among all other factions		
			int nScoreDeltaOther = -nScoreDelta / (nNumFactions - 1);

			// change the score of all other factions
			for (int nFactionOther = 0; nFactionOther < nNumFactions; ++nFactionOther)
			{
				if (nFactionOther != nFaction)
				{
					sFactionScoreChange( pUnit, nFactionOther, nScoreDeltaOther );
				}
			}

		}
			
	}*/

	// quest availability may have changed as a result of the player's faction change, update NPC icons
	if ( AppIsHellgate() && IS_SERVER( UnitGetGame( pUnit ) ) )
	{
		s_QuestsUpdateAvailability( pUnit );
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void FactionGiveInitialScores( 
	UNIT *pUnit)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
		
	int nNumFactions = FactionGetNumFactions();
	for (int nFaction = 0; nFaction < nNumFactions; ++nFaction)
	{
		const FACTION_DEFINITION *pFactionDef = FactionGetDefinition( nFaction );

		if (!pFactionDef)
		{
			// not in this version_package
			continue;
		}

		// init the score
		int nInitScore = FACTION_SCORE_NONE;
			
		// go through all faction init sets
		for (int j = 0; j < FACTION_MAX_INIT; ++j)
		{
			const FACTION_INIT *pFactionInit = &pFactionDef->tFactionInit[ j ];
			ASSERT_CONTINUE(pFactionInit);
			
			BOOL bGiveStanding = FALSE;
			if (pFactionInit->nUnitType != INVALID_LINK)
			{
				if (pFactionInit->nFactionStanding != INVALID_LINK)
				{
					if (UnitIsA( pUnit, pFactionInit->nUnitType ))
					{
						if (pFactionInit->nLevelDef != INVALID_LINK)
						{
							LEVEL *pLevel = UnitGetLevel( pUnit );
							if (pLevel)
							{
								if (LevelGetDefinitionIndex( pLevel ) == pFactionInit->nLevelDef)
								{
									bGiveStanding = TRUE;
								}
							}
						}
						else
						{
							bGiveStanding = TRUE;
						}
												
					}

				}
								
			}

			if (bGiveStanding == TRUE)
			{
				const FACTION_STANDING_DEFINITION *pStandingDef = FactionStandingGetDefinition( pFactionInit->nFactionStanding );
				ASSERT_CONTINUE(pStandingDef);
				if (pStandingDef->nMinScore > nInitScore)
				{
					nInitScore = pStandingDef->nMinScore;
				}
			}
			
		}
		
		// set the score
		if (nInitScore != FACTION_SCORE_NONE)
		{
			UnitSetStat( pUnit, STATS_FACTION_SCORE, nFaction, nInitScore );
		}
		
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void FactionDebugString(
	int nFaction,
	int nScore,
	WCHAR *puszBuffer,
	int nBufferSize)
{
	ASSERTX_RETURN( puszBuffer, "Expected buffer" );
	ASSERTX_RETURN( nBufferSize, "Invalid buffer size" );
	ASSERTX_RETURN( nFaction != INVALID_LINK, "Invalid faction" );

	// get standing	
	int nStanding = sFactionScoreToStanding( nScore );
	
	// print debug string
	PStrPrintf( 
		puszBuffer, 
		nBufferSize, 			
		L"%s - %s (%d)", 
		FactionGetDisplayName( nFaction ),
		FactionStandingGetDisplayName( nStanding ),
		nScore);
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_FactionScoreChanged( 
	UNIT *pUnit,
	int nFaction, 
	int nOldValue,
	int nNewValue)
{
	int nScoreDelta = nNewValue - nOldValue;

	const int MESSAGE_SIZE = 2048;
	WCHAR uszMessage[ MESSAGE_SIZE ];
	PStrPrintf( 
		uszMessage, 
		MESSAGE_SIZE,
		GlobalStringGet( GS_FACTION_SCORE_CHANGED ),
		FactionGetDisplayName( nFaction ),
		nScoreDelta,
		nNewValue,
		FactionStandingGetDisplayName( FactionGetStanding( pUnit, nFaction ) ));
		
	// display to console
	ConsoleString( CONSOLE_SYSTEM_COLOR, uszMessage );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int FactionGetTitle(
	UNIT * unit )
{
	GLOBAL_INDEX eFactionIndex = GI_INVALID;
	GLOBAL_STRING eGoodString = GS_INVALID;
	GLOBAL_STRING eKindlyString = GS_INVALID;
	if ( UnitIsA( unit, UNITTYPE_TEMPLAR ) )
	{
		eFactionIndex = GI_FACTION_TEMPLAR;
		eGoodString = GS_TEMPLAR_GOOD_TITLE;
		eKindlyString = GS_TEMPLAR_KINDLY_TITLE;
	}
	else if ( UnitIsA( unit, UNITTYPE_CABALIST ) )
	{
		eFactionIndex = GI_FACTION_CABALIST;
		eGoodString = GS_CABALIST_GOOD_TITLE;
		eKindlyString = GS_CABALIST_KINDLY_TITLE;
	}
	else if ( UnitIsA( unit, UNITTYPE_HUNTER ) )
	{
		eFactionIndex = GI_FACTION_HUNTER;
		eGoodString = GS_HUNTER_GOOD_TITLE;
		eKindlyString = GS_HUNTER_KINDLY_TITLE;
	}

	if ( eFactionIndex != GI_INVALID )
	{
		int index = GlobalIndexGet( eFactionIndex );
		int standing = FactionGetStanding( unit, index );
		if ( standing >= GlobalIndexGet( GI_FACTION_STANDING_KINDLY ) )
			return GlobalStringGetStringIndex( eKindlyString );
		if ( standing >= GlobalIndexGet( GI_FACTION_STANDING_GOOD ) )
			return GlobalStringGetStringIndex( eGoodString );
	}

	return INVALID_LINK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

OPERATE_RESULT FactionCanOperateObject(
	UNIT * unit,
	UNIT * object )
{
	ASSERT_RETVAL( unit, OPERATE_RESULT_FORBIDDEN );
	ASSERT_RETVAL( object, OPERATE_RESULT_FORBIDDEN );

	GLOBAL_INDEX eFactionIndex = GI_INVALID;
	if ( UnitIsA( unit, UNITTYPE_TEMPLAR ) )
	{
		eFactionIndex = GI_FACTION_TEMPLAR;
	}
	else if ( UnitIsA( unit, UNITTYPE_CABALIST ) )
	{
		eFactionIndex = GI_FACTION_CABALIST;
	}
	else if ( UnitIsA( unit, UNITTYPE_HUNTER ) )
	{
		eFactionIndex = GI_FACTION_HUNTER;
	}

	if ( eFactionIndex != GI_INVALID )
	{
		int index = GlobalIndexGet( eFactionIndex );
		int standing = FactionGetStanding( unit, index );
		if ( standing >= GlobalIndexGet( GI_FACTION_STANDING_GOOD ) )
			return OPERATE_RESULT_OK;
	}

	return OPERATE_RESULT_FORBIDDEN;
}
