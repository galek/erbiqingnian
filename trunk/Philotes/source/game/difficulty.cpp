//----------------------------------------------------------------------------
// FILE: difficulty.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "difficulty.h"
#include "excel.h"
#include "units.h"
#include "statssave.h"
#include "globalindex.h"

//----------------------------------------------------------------------------
// Definitions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
const DIFFICULTY_DATA *DifficultyGetData(
	int nDifficulty)
{
	ASSERTX_RETNULL( nDifficulty != INVALID_LINK, "Expected difficulty link" );
	ASSERTX_RETNULL( nDifficulty > INVALID_LINK && nDifficulty < NUM_DIFFICULTIES, "Difficulty out of range" );
	return (const DIFFICULTY_DATA *)ExcelGetData( NULL, DATATABLE_DIFFICULTY, nDifficulty );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int DifficultyGetCurrent(
	UNIT *pPlayer)
{
	ASSERTX_RETINVALID( pPlayer, "Expected unit" );
	ASSERTX_RETINVALID( UnitIsPlayer( pPlayer ), "Expected player" );
	return UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void DifficultySetCurrent(
	UNIT *pPlayer,
	int nDifficulty)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );
	ASSERTX_RETURN( nDifficulty != INVALID_LINK, "Expected difficulty" );
	ASSERTX_RETURN( DifficultyGetMax( pPlayer ) >= nDifficulty, "Cannot set current difficulty beyond the max allowed for this player" );
	return UnitSetStat( pPlayer, STATS_DIFFICULTY_CURRENT, nDifficulty );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int DifficultyGetMax(
	UNIT *pPlayer)
{
	ASSERTX_RETINVALID( pPlayer, "Expected unit" );
	ASSERTX_RETINVALID( UnitIsPlayer( pPlayer ), "Expected player" );
	return UnitGetStat( pPlayer, STATS_DIFFICULTY_MAX );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void DifficultySetMax(
	UNIT *pPlayer,
	int nDifficulty)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );
	ASSERTX_RETURN( nDifficulty != INVALID_LINK, "Expected difficulty" );
	return UnitSetStat( pPlayer, STATS_DIFFICULTY_MAX, nDifficulty );
}

//----------------------------------------------------------------------------
// Validate whether we can be at a given difficulty level.  If you
// provide a unit, this validates it against the unit's maximums and
// minimums.  Otherwise, it validates it against what is globally allowed.
//
// Modifies the given difficulty to be within allowed tolerances.
//----------------------------------------------------------------------------
BOOL DifficultyValidateValue(
	int &nDifficulty,
	UNIT *pPlayer /*= NULL*/,
	GAMECLIENT *pClient /* = NULL */)
{
	if(nDifficulty < GlobalIndexGet(GI_DIFFICULTY_DEFAULT) ||
		nDifficulty > GlobalIndexGet(GI_DIFFICULTY_HELL))
	{
		nDifficulty = GlobalIndexGet(GI_DIFFICULTY_DEFAULT);
		return FALSE;
	}
	if(pPlayer && nDifficulty > DifficultyGetMax(pPlayer) )
	{
		nDifficulty = DifficultyGetMax(pPlayer);
		return FALSE;
	}
	if(pClient && ClientHasBadge(pClient, ACCT_TITLE_TRIAL_USER)
		&& nDifficulty != GlobalIndexGet(GI_DIFFICULTY_DEFAULT))
	{	//Trial users can't use difficulties other than the default
		nDifficulty = GlobalIndexGet(GI_DIFFICULTY_DEFAULT);
		return FALSE;		
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
STAT_VERSION_RESULT s_VersionStatDifficulty(
	STATS_FILE &tStatsFile,
	STATS_VERSION &nStatsVersion,
	UNIT *pUnit)
{
	int nStat = tStatsFile.tDescription.nStat;

	if(nStatsVersion <= STATS_VERSION_DIFFICULTY_TOO_HIGH)
	{
		switch(nStat)
		{
		case STATS_DIFFICULTY_CURRENT:
		case STATS_DIFFICULTY_MAX:
			// there's a chance that the max was set to hell previously
			tStatsFile.tValue.nValue = PIN(int(tStatsFile.tValue.nValue), int(DIFFICULTY_NORMAL), int(DIFFICULTY_NIGHTMARE));
			break;

		default:
			break;
		}
	}

	nStatsVersion = STATS_CURRENT_VERSION;

	return SVR_OK;
		
}