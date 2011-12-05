//----------------------------------------------------------------------------
// waypoint.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "waypoint.h"
#include "prime.h"
#include "game.h"
#include "definition.h"
#include "config.h"
#include "gameconfig.h"
#include "level.h"
#include "unit_priv.h"
#include "stats.h"
#include "objects.h"
#include "stringtable.h"
#include "uix.h"
#include "c_message.h"
#include "warp.h"
#include "uix_components_complex.h"
#include "excel_private.h"
#include "statspriv.h"
#include "difficulty.h"
//#include "..\data\excel\difficulty_hdr.h"


//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------
struct WAYPOINT_SYSTEM
{
	SORTED_ARRAY<int, 4>		m_LevelList;
};


//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
WAYPOINT_SYSTEM gWaypointSystem;


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL WaypointSystemInit(
	struct EXCEL_TABLE * table)
{
	ASSERT_RETFALSE(table);

	gWaypointSystem.m_LevelList.Init();
	unsigned int nNumLevels = ExcelGetCountPrivate( table );
	for (unsigned int ii = 0; ii < nNumLevels; ++ii)
	{
		const LEVEL_DEFINITION * level_data = LevelDefinitionGet( ii );
		if (level_data)
		{
			if (level_data->nWaypointMarker != INVALID_LINK)
			{
				gWaypointSystem.m_LevelList.Insert(NULL, ii);
			}
		}
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void WaypointSystemFree(
	void)
{
	gWaypointSystem.m_LevelList.Destroy(NULL);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
unsigned int WaypointSystemGetCount(
	void)
{
	return gWaypointSystem.m_LevelList.Count();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
unsigned int WaypointSystemGetLevelByIndex(
	unsigned int index)
{
	ASSERT_RETINVALID(index >= 0 && index < WaypointSystemGetCount());
	return gWaypointSystem.m_LevelList[index];
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
unsigned int WaypointSystemGetIndexByMarker(
	GAME * game,
	unsigned int marker)
{
	if (marker == INVALID_LINK)
	{
		return (unsigned int)INVALID_LINK;
	}
	unsigned int count = WaypointSystemGetCount();
	for (unsigned int ii = 0; ii < count; ii++)
	{
		unsigned int waypoint_level = gWaypointSystem.m_LevelList[ii];
		const LEVEL_DEFINITION * level_data = LevelDefinitionGet(waypoint_level);
		ASSERT_CONTINUE(level_data);
		if ((unsigned int)level_data->nWaypointMarker == (unsigned int)marker)
		{
			return ii;
		}
	}
	return (unsigned int)INVALID_LINK;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL WaypointIsMarked( 
	GAME * game, 
	UNIT * player, 
	int level_definition,
	int nDifficulty)
{
	ASSERT_RETFALSE(level_definition != INVALID_LINK);
	ASSERT_RETFALSE(nDifficulty != INVALID_LINK);
	ASSERT_RETFALSE(player);

	int index = gWaypointSystem.m_LevelList.FindExactIndex(level_definition);
	if (index == INVALID_LINK)
	{
		return FALSE;
	}
	
	int flag_idx = index/32;
	DWORD flags = UnitGetStat(player, STATS_WAYPOINT_FLAGS, flag_idx, nDifficulty);
	
	return TESTBIT(flags, index % 32);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sUnitSetWaypoint(
	GAME * game,
	UNIT * unit,
	int waypoint)
{
	ASSERT_RETFALSE(game && IS_SERVER(game));
	ASSERT_RETFALSE(unit);
	ASSERT_RETFALSE(waypoint >= 0 && (unsigned int)waypoint < WaypointSystemGetCount());

	unsigned int waypoint_block = waypoint / 32;
	DWORD flags = UnitGetStat(unit, STATS_WAYPOINT_FLAGS, waypoint_block, UnitGetStat(unit, STATS_DIFFICULTY_CURRENT));
	if (TESTBIT(flags, waypoint - waypoint_block * 32))
	{
		return FALSE;
	}
	SETBIT(flags, waypoint - waypoint_block * 32);
	UnitSetStat(unit, STATS_WAYPOINT_FLAGS, waypoint_block, UnitGetStat(unit, STATS_DIFFICULTY_CURRENT), flags);
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void WaypointGive(
	GAME * game,
	UNIT * player,
	const LEVEL_DEFINITION *pLevelDef)
{
	unsigned int waypoint = WaypointSystemGetIndexByMarker(game, pLevelDef->nWaypointMarker);
	if (waypoint != INVALID_LINK)
	{
		sUnitSetWaypoint(game, player, waypoint);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_PlayerInitWaypointFlags(
	GAME * game,
	UNIT * player)
{
	ASSERT_RETFALSE(game && IS_SERVER(game));
	ASSERT_RETFALSE(player);

	unsigned int waypoint_count = WaypointSystemGetCount();

	for (unsigned int ii = 0; ii < waypoint_count; ++ii)
	{
		unsigned int waypoint_level = gWaypointSystem.m_LevelList[ii];
		const LEVEL_DEFINITION * level_data = LevelDefinitionGet(waypoint_level);
		ASSERT_CONTINUE(level_data);
		if (level_data->bAutoWaypoint)		
		{
			BOOL bGiveWaypoint = TRUE;
			if (level_data->bMultiplayerOnly == TRUE && AppIsMultiplayer() == FALSE)
			{
				bGiveWaypoint = FALSE;
			}
			if (bGiveWaypoint)
			{
				sUnitSetWaypoint(game, player, ii);
			}
		}
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_LevelSetWaypoint(
	GAME * game,
	UNIT * player,
	UNIT * marker)
{
	ASSERT_RETFALSE(game && IS_SERVER(game));
	ASSERT_RETFALSE(player);
	ASSERT_RETFALSE(marker);

	unsigned int waypoint = WaypointSystemGetIndexByMarker(game, UnitGetClass(marker));
	if (waypoint == INVALID_LINK)
	{
		return FALSE;
	}
	return sUnitSetWaypoint(game, player, waypoint);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_WaypointsClearAll(
	UNIT* player,
	int nDifficulty)
{
	ASSERT_RETFALSE(player);
	if(!ISVERSION(SERVER_VERSION))
		return FALSE;
	unsigned int waypoint_count = WaypointSystemGetCount();
	unsigned int waypoint_flag_count = DWORD_FLAG_SIZE(waypoint_count);

	// iterate waypoint blocks (of 32)	
	for (unsigned int jj = 0; jj < waypoint_flag_count; ++jj)
	{
		UnitSetStat(player, STATS_WAYPOINT_FLAGS, jj, nDifficulty, 0x0);
		//UnitClearStat(player, STATS_WAYPOINT_FLAGS, jj, nDifficulty);
	}
	UnitClearStat(player, STATS_LEVEL_DEF_START, 0);
	UnitClearStat(player, STATS_LEVEL_DEF_RETURN, 0);
	return TRUE;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
BOOL s_LevelSetAllWaypoints(
	GAME * game,
	UNIT * player)
{
	ASSERT_RETFALSE(game && IS_SERVER(game));
	ASSERT_RETFALSE(player);

	unsigned int waypoint_count = WaypointSystemGetCount();
	unsigned int waypoint_flag_count = DWORD_FLAG_SIZE(waypoint_count);

	// iterate waypoint blocks (of 32)	
	for (unsigned int jj = 0; jj < waypoint_flag_count; ++jj)
	{
		UnitSetStat(player, STATS_WAYPOINT_FLAGS, jj, UnitGetStat(player, STATS_DIFFICULTY_CURRENT), 0xffffffff);
	}
	return TRUE;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sPlayerCanWaypointToLevelDef( 
	UNIT *pPlayer,
	int nLevelDef)
{
	ASSERTX_RETFALSE( pPlayer, "Expected unit" );
	ASSERTV_RETFALSE( UnitIsPlayer( pPlayer ), "Expected player, got '%s'", UnitGetDevName( pPlayer ) );

	// no level def
	if (nLevelDef == INVALID_LINK)
	{
		return FALSE;
	}
	
	// get level definition
	const LEVEL_DEFINITION *pLevelDef = LevelDefinitionGet( nLevelDef );
	
	// if player is hardcore dead, this level might not allow them
	if (PlayerIsHardcoreDead( pPlayer ) && pLevelDef->bHardcoreDeadCanVisit == FALSE)
	{
		return FALSE;
	}
	
	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void WaypointCreateListForPlayer( 
	GAME * game, 
	UNIT * player, 
	const WCHAR ** level_names, 
	int * table_index, 
	unsigned int * list_length)
{
	ASSERT_RETURN(list_length);
	*list_length = 0;
	
	ASSERT_RETURN(game);
	ASSERT_RETURN(player);
	
	LEVEL * level = UnitGetLevel(player);
	const LEVEL_DEFINITION * player_level_data = LevelDefinitionGet(level);
	
	unsigned int waypoint_count = WaypointSystemGetCount();
	unsigned int waypoint_flag_count = DWORD_FLAG_SIZE(waypoint_count);

	// iterate waypoint blocks (of 32)	
	for (unsigned int jj = 0; jj < waypoint_flag_count; ++jj)
	{
		DWORD flags = UnitGetStat(player, STATS_WAYPOINT_FLAGS, jj, UnitGetStat(player, STATS_DIFFICULTY_CURRENT));
		if (!flags)
		{
			continue;
		}
		// iterate waypoint flags in block
		unsigned int waypoint_bit_count = MIN((unsigned int)32, waypoint_count - jj * 32);
		for (unsigned int ii = 0; ii < waypoint_bit_count; ++ii)
		{
			if (TESTBIT(flags, ii))
			{
				unsigned int waypoint_level = WaypointSystemGetLevelByIndex(jj * 32 + ii);
				const LEVEL_DEFINITION * level_data = LevelDefinitionGet(waypoint_level);
				if (level_data == player_level_data)
				{
					continue;
				}
				
				// some levels hardcore dead players cannot go to
				if (sPlayerCanWaypointToLevelDef( player, waypoint_level ) == FALSE)
				{
					continue;
				}
				
				ASSERT_CONTINUE(level_data->nWaypointMarker != INVALID_LINK);
				const UNIT_DATA * marker = ObjectGetData(game, level_data->nWaypointMarker);
				ASSERT_CONTINUE(marker);
				if (table_index)
				{
					table_index[*list_length] = jj * 32 + ii;
				}
				level_names[*list_length] = StringTableGetStringByIndex(marker->nString);
				(*list_length)++;				
			}
		}
	}
}

//----------------------------------------------------------------------------
// handle message from client to waypoint warp
// TUGBOAT SECURITY HOLE!
// TODO: Validate tugboat levelarea waypoint warp.  
// But is this ever used in tugboat?
//----------------------------------------------------------------------------
BOOL s_LevelWaypointWarp(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit,	
	int level_definition,
	int levelarea )			//Tugboat only
{
	if (level_definition < 0 || (AppIsHellgate() && (unsigned int)level_definition >= ExcelGetCount(EXCEL_CONTEXT(game), DATATABLE_LEVEL) ))
	{
		return FALSE;
	}
	
	// does the client actually have the waypoint marked?
	int nDifficulty = DifficultyGetCurrent(unit);
	if (!WaypointIsMarked(game, unit, level_definition, nDifficulty))
	{
		return FALSE;
	}

	// can't do this if you're dead
	if (IsUnitDeadOrDying( unit ))
	{
		return FALSE;
	}

	// if level is not available, forget it
	if (LevelDefIsAvailableToUnit( unit, level_definition ) != WRR_OK)
	{
		return FALSE;
	}

	// player must be able to use this waypoint
	if (sPlayerCanWaypointToLevelDef( unit, level_definition ) == FALSE)
	{
		return FALSE;
	}
		
	// todo: is the client standing near the waypoint thinger	
	
	// setup warp spec
	WARP_SPEC tSpec;
	tSpec.nLevelDefDest = level_definition;
	SETBIT( tSpec.dwWarpFlags, WF_ARRIVE_AT_START_LOCATION_BIT );
	tSpec.nLevelAreaDest = levelarea;
	tSpec.nLevelDepthDest = level_definition;  // this seems wrong, but I'm converting as is for now -Colin
	
	// warp!
	return s_WarpToLevelOrInstance( unit, tSpec );
	
}


#if !ISVERSION(SERVER_VERSION)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_WaypointTryOperate(
	GAME * game,
	UNIT * player,
	unsigned int clicked)
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(player);
	
	// create list of levels I can warp to
	const WCHAR * level_names[MAX_WAYPOINT_BLOCKS];
	int table_index[MAX_WAYPOINT_BLOCKS];
	unsigned int list_length = 0;
	WaypointCreateListForPlayer(game, player, &level_names[0], &table_index[0], &list_length);

	if (clicked >= list_length)
	{
		return;
	}

	MSG_CCMD_OPERATE_WAYPOINT msg;
	msg.nLevelDefinition = WaypointSystemGetLevelByIndex(table_index[clicked]);
	// TRAVIS: broken - need to work on waypoints
	msg.nLevelArea = (AppIsTugboat())?1:msg.nLevelArea;
	c_SendMessage(CCMD_OPERATE_WAYPOINT, &msg);
}


//----------------------------------------------------------------------------
// pressed the waypoint object... start the menu
//----------------------------------------------------------------------------
void c_LevelOpenWaypointUI(
	GAME * game)
{
	ASSERT_RETURN(game && IS_CLIENT(game));
	UNIT * player = GameGetControlUnit(game);
	ASSERT_RETURN(player);

	//// create list of levels I can warp to
	//const WCHAR * level_names[MAX_WAYPOINT_BLOCKS];
	//unsigned int list_length = 0;
	//WaypointCreateListForPlayer(game, player, &level_names[0], NULL, &list_length);
	//
	UIShowWaypointsScreen(/*&level_names[0], list_length*/);
}

#endif //!SERVER_VERSION

//----------------------------------------------------------------------------
//versioning stuff
//----------------------------------------------------------------------------
STAT_VERSION_RESULT s_VersionStatWaypointFlag(
	 STATS_FILE &tStatsFile,
	 STATS_VERSION &nStatsVersion,
	 UNIT *pUnit)
{
	//int nStat = tStatsFile.tDescription.nStat;

	// quest save state stats
	if (nStatsVersion < 4) // Waypoint flag was saved w/o a param
	{
		DWORD waypoint_block = 0;
		ASSERT(tStatsFile.tDescription.nParamCount == 0);
		STATS_PARAM_DESCRIPTION* pParamDesc = &tStatsFile.tDescription.tParamDescriptions[tStatsFile.tDescription.nParamCount];
		//pParamDesc->eType = PARAM_TYPE_BY_CODE; // All this desc stuff gets blown away, but its clearer to put it here anyway...
		//pParamDesc->idParamTable = ;
		//pParamDesc->nParamBitsFile = ;
		//pParamDesc->nParamOffs = ;
		//pParamDesc->nParamShift = ;

		STATS_FILE_PARAM_VALUE* pValue = &tStatsFile.tParamValue[tStatsFile.tDescription.nParamCount];
		pValue->nValue = waypoint_block;
		//tStatsFile.tParamValue[ 0 ].nValue = dwDifficultyCode;

		// we are now next version
		tStatsFile.tDescription.nParamCount++;

		//DWORD dwDifficultyCode = DIFFICULTY_NORMAL;
		DWORD dwDifficultyCode = ExcelGetCode( NULL, DATATABLE_DIFFICULTY, DIFFICULTY_NORMAL );
		ASSERT(tStatsFile.tDescription.nParamCount == 1);
		pParamDesc = &tStatsFile.tDescription.tParamDescriptions[tStatsFile.tDescription.nParamCount];
		pValue = &tStatsFile.tParamValue[tStatsFile.tDescription.nParamCount];
		pValue->nValue = dwDifficultyCode;

		//DWORD dwDiffcultyTable = ExcelGetCode( NULL, DATATABLE_QUEST_STATE, pState->nQuestState );
		pParamDesc->idParamTable = DATATABLE_DIFFICULTY;
		tStatsFile.tDescription.nParamCount++;
		nStatsVersion = 4;
	}	
	//Insert further versioning checks here...

	nStatsVersion = STATS_CURRENT_VERSION;
	
	return SVR_OK;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
STAT_VERSION_RESULT s_VersionStatStartLevel(
	STATS_FILE &tStatsFile,
	STATS_VERSION &nStatsVersion,
	UNIT *pUnit)
{
	//int nStat = tStatsFile.tDescription.nStat;

	// quest save state stats
	if (nStatsVersion < 4) // Waypoint flag was saved w/o a param
	{
		DWORD dwDifficulty = DIFFICULTY_NORMAL;
		ASSERT(tStatsFile.tDescription.nParamCount == 0);
		STATS_PARAM_DESCRIPTION* pParamDesc = &tStatsFile.tDescription.tParamDescriptions[tStatsFile.tDescription.nParamCount];

		STATS_FILE_PARAM_VALUE* pValue = &tStatsFile.tParamValue[tStatsFile.tDescription.nParamCount];
		pValue->nValue = dwDifficulty;
		pParamDesc->idParamTable = DATATABLE_DIFFICULTY;
		// we are now next version
		tStatsFile.tDescription.nParamCount++;
	}	
	//Insert further versioning checks here...

	nStatsVersion = STATS_CURRENT_VERSION;
	
	return SVR_OK;
	
}

