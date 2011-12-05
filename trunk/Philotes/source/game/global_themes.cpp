//-------------------------------------------------------------------------------------------------
//
// Prime Global Themes v1.0
//
// by Tyler
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//
//-------------------------------------------------------------------------------------------------
// Global themes - like Halloween, Mod Tuesdays, The Week of Fire, etc....
//-------------------------------------------------------------------------------------------------

#include "stdafx.h"
#include "game.h"
#include "global_themes.h"
#include "excel.h"
#include "excel_private.h"
#include "datatables.h"
#include "stats.h"
#include "events.h"
#include "s_message.h"
#include "dungeon.h"
#include "level.h"
#include "room.h"
#include "room_layout.h"
#include "unit_priv.h"

#define MAX_FORCED_THEMES 24
struct FORCED_THEME
{
	char szName[ DEFAULT_INDEX_SIZE ]; // these are set by the commandline.  We can't look them up in Excel at that point.
	BOOL bForcedEnabled;
};
static FORCED_THEME sgptForcedThemes[ MAX_FORCED_THEMES ];
static int sgnForcedThemeCount = 0;
static DWORD sgdwThemesForcedEnabled[DWORD_FLAG_SIZE(MAX_GLOBAL_THEMES)];
static DWORD sgdwThemesForcedDisabled[DWORD_FLAG_SIZE(MAX_GLOBAL_THEMES)];

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void GlobalThemeForce(
	const char * pszTheme,
	BOOL bEnable )
{
	ASSERT_RETURN( sgnForcedThemeCount < MAX_FORCED_THEMES );
	FORCED_THEME & tForcedTheme = sgptForcedThemes[ sgnForcedThemeCount ];
	sgnForcedThemeCount++;
	PStrCopy( tForcedTheme.szName, pszTheme, DEFAULT_INDEX_SIZE );
	tForcedTheme.bForcedEnabled = bEnable;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BOOL ExcelGlobalThemesPostProcess( 
	EXCEL_TABLE * table)
{
	ZeroMemory( sgdwThemesForcedEnabled, sizeof( sgdwThemesForcedEnabled ) );
	ZeroMemory( sgdwThemesForcedDisabled, sizeof( sgdwThemesForcedDisabled ) );
	ASSERT_RETFALSE( sgnForcedThemeCount <= MAX_FORCED_THEMES );
	ASSERT_RETFALSE( ExcelGetNumRows( EXCEL_CONTEXT(), DATATABLE_GLOBAL_THEMES ) <= MAX_GLOBAL_THEMES );

	for ( int i = 0; i < sgnForcedThemeCount; i++ )
	{
		int nIndex = ExcelGetLineByStringIndexPrivate( table, sgptForcedThemes[ i ].szName );
		if ( nIndex != INVALID_ID )
		{
			if ( sgptForcedThemes[ i ].bForcedEnabled )
			{
				SETBIT( sgdwThemesForcedEnabled, nIndex );
			}
			else
			{
				SETBIT( sgdwThemesForcedDisabled, nIndex );
			}
		}
	}
	return TRUE;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static
void s_sSendGlobalThemeChange(
	GAME * pGame,
	int nGlobalTheme,
	BOOL bEnable,
	BOOL bSendToAll,
	GAMECLIENTID idClient )
{
	MSG_SCMD_GLOBAL_THEME_CHANGE tMsg;
	tMsg.wTheme = (WORD) nGlobalTheme;
	tMsg.bEnable = (BYTE) bEnable ? 1 : 0;
	if ( bSendToAll )
	{
		s_SendMessageToAll(pGame, SCMD_GLOBAL_THEME_CHANGE, &tMsg);	
	}
	else
	{
		s_SendMessage( pGame, idClient, SCMD_GLOBAL_THEME_CHANGE, &tMsg );
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void s_GlobalThemeSendAllToClient( 
	GAME * pGame,
	GAMECLIENTID idClient )
{
	ASSERT_RETURN( pGame );
	ASSERT_RETURN( IS_SERVER( pGame ) );

	int nThemeCount = ExcelGetNumRows( EXCEL_CONTEXT( pGame ), DATATABLE_GLOBAL_THEMES );

	for ( int i = 0; i < nThemeCount; i++ )
	{
		if ( GlobalThemeIsEnabled( pGame, i ) )
		{
			s_sSendGlobalThemeChange( pGame, i, TRUE, FALSE, idClient );
		}
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static
BOOL sGlobalThemeIsForceEnabled(
	int nTheme )
{
	ASSERT_RETFALSE( nTheme <= MAX_GLOBAL_THEMES );
	return TESTBIT( sgdwThemesForcedEnabled, nTheme );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static
BOOL sGlobalThemeIsForceDisabled(
	int nTheme )
{
	ASSERT_RETFALSE( nTheme <= MAX_GLOBAL_THEMES );
	return TESTBIT( sgdwThemesForcedDisabled, nTheme );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL GlobalThemeIsEnabled(
	GAME * pGame,
	int nGlobalTheme )
{
	ASSERT_RETFALSE( nGlobalTheme >= 0 && nGlobalTheme < MAX_GLOBAL_THEMES );
	ASSERT_RETFALSE( pGame );
	return TESTBIT( pGame->dwGlobalThemesEnabled, nGlobalTheme );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void GlobalThemeEnable(
	GAME * pGame,
	int nGlobalTheme )
{
	ASSERT_RETURN( nGlobalTheme >= 0 && nGlobalTheme < MAX_GLOBAL_THEMES );
	ASSERT_RETURN( pGame );

	if ( ! GlobalThemeIsEnabled( pGame, nGlobalTheme ) )
	{
		SETBIT( pGame->dwGlobalThemesEnabled, nGlobalTheme );

		if ( IS_SERVER( pGame ) )
		{
			s_sSendGlobalThemeChange( pGame, nGlobalTheme, TRUE, TRUE, INVALID_ID );
		}
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void GlobalThemeDisable(
	GAME * pGame,
	int nGlobalTheme )
{
	ASSERT_RETURN( nGlobalTheme >= 0 && nGlobalTheme < MAX_GLOBAL_THEMES );
	ASSERT_RETURN( pGame );
	if ( GlobalThemeIsEnabled( pGame, nGlobalTheme ) )
	{
		CLEARBIT( pGame->dwGlobalThemesEnabled, nGlobalTheme );

		if ( IS_SERVER( pGame ) )
		{
			s_sSendGlobalThemeChange( pGame, nGlobalTheme, FALSE, TRUE, INVALID_ID );
		}
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void GlobalThemeMonsterInit(
	GAME * pGame,
	UNIT * pUnit )
{
	ASSERT_RETURN( pGame );
	ASSERT_RETURN( pUnit );
	int nTotalThemes = ExcelGetNumRows( EXCEL_CONTEXT( pGame ), DATATABLE_GLOBAL_THEMES );
	for ( int i = 0; i < nTotalThemes; i++ )
	{
		if ( TESTBIT( pGame->dwGlobalThemesEnabled, i ) )
		{
			const GLOBAL_THEME_DATA * pGlobalThemeData = (const GLOBAL_THEME_DATA *) ExcelGetData( EXCEL_CONTEXT( pGame ), DATATABLE_GLOBAL_THEMES, i );
			if ( ! pGlobalThemeData )
				continue;

			int nTreasure = UnitGetStat( pUnit, STATS_TREASURE_CLASS );
			int nTreasureNew = INVALID_ID;
			if ( nTreasure != INVALID_ID )
			{
				for ( int j = 0; j < GLOBAL_THEME_MAX_TREASURECLASS_REPLACEMENTS; j++ )
				{
					if ( nTreasure == pGlobalThemeData->pnTreasureClassPreAndPost[ j ][ 0 ] )
						nTreasureNew = pGlobalThemeData->pnTreasureClassPreAndPost[ j ][ 1 ];
				}
				if ( nTreasureNew != INVALID_ID )
					UnitSetStat( pUnit, STATS_TREASURE_CLASS, nTreasureNew );
			}

		}
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static BOOL sGlobalThemesUpdateShouldMonsterTrySpawn(
	GAME * pGame,
	ROOM_LAYOUT_GROUP * pLayoutGroup)
{
	const UNIT_DATA * pUnitData = NULL;
	if(pLayoutGroup->dwUnitType == ROOM_SPAWN_MONSTER && pLayoutGroup->dwCode != 0xFFFFFFFF)
	{
		pUnitData = (const UNIT_DATA *)ExcelGetDataByCode(pGame, DATATABLE_MONSTERS, pLayoutGroup->dwCode);
	}
	else if(pLayoutGroup->dwUnitType == ROOM_SPAWN_OBJECT)
	{
		pUnitData = (const UNIT_DATA *)ExcelGetDataByCode(pGame, DATATABLE_OBJECTS, pLayoutGroup->dwCode);
	}
	if(pUnitData)
	{
		if(pUnitData->nGlobalThemeRequired >= 0 && GlobalThemeIsEnabled(pGame, pUnitData->nGlobalThemeRequired))
		{
			return TRUE;
		}
	}
	return FALSE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static void sGlobalThemesUpdateSpawnThemeMonstersInRoom(
	ROOM *pRoom,
	void *pCallbackData )
{
	ROOM_LAYOUT_SELECTION * pLayoutSelection = RoomGetLayoutSelection(pRoom, ROOM_LAYOUT_ITEM_SPAWNPOINT);
	if(!pLayoutSelection)
	{
		return;
	}
	if(pLayoutSelection->nCount > 0)
	{
		ASSERT_RETURN(pLayoutSelection->pGroups);
		ASSERT_RETURN(pLayoutSelection->pOrientations);
	}
	GAME * pGame = RoomGetGame(pRoom);
	ASSERT_RETURN(pGame);
	for(int i=0; i<pLayoutSelection->nCount; i++)
	{
		ROOM_LAYOUT_GROUP * pLayoutGroup = pLayoutSelection->pGroups[i];
		ASSERT_CONTINUE(pLayoutGroup);
		ROOM_LAYOUT_SELECTION_ORIENTATION * pOrientation = &pLayoutSelection->pOrientations[i];
		ASSERT_CONTINUE(pOrientation);
		if(sGlobalThemesUpdateShouldMonsterTrySpawn(pGame, pLayoutGroup))
		{
			s_RoomPopulateSpawnPoint(pGame, pRoom, pLayoutGroup, pOrientation, RPP_CONTENT);
		}
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static void sGlobalThemesUpdateSpawnThemeMonsters(
	GAME * pGame)
{
	LEVEL * pLevel = DungeonGetFirstLevel(pGame);
	while(pLevel)
	{
		if(LevelIsTown(pLevel) && LevelIsActive(pLevel))
		{
			LevelIterateRooms(pLevel, sGlobalThemesUpdateSpawnThemeMonstersInRoom, NULL);
		}
		pLevel = DungeonGetNextLevel(pLevel);
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static BOOL sUpdateGlobalThemes(
	GAME * game, 
	UNIT * unit, const 
	EVENT_DATA & event_data)
{
	s_GlobalThemesUpdate( game );
	return TRUE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void s_GlobalThemesUpdate(
	GAME * pGame )
{
	ASSERT_RETURN( pGame );
	ASSERT_RETURN( IS_SERVER( pGame ) );
	SYSTEMTIME	tTime;
	GetLocalTime( &tTime );

	int nNumGlobalThemes = ExcelGetNumRows( EXCEL_CONTEXT( pGame ), DATATABLE_GLOBAL_THEMES );
	for ( int i = 0; i < nNumGlobalThemes; i++ )
	{
		const GLOBAL_THEME_DATA * pGlobalThemeData = (const GLOBAL_THEME_DATA *) ExcelGetData( EXCEL_CONTEXT( pGame ), DATATABLE_GLOBAL_THEMES, i );
		if ( ! pGlobalThemeData )
			continue;

		if ( !pGlobalThemeData->bActivateByTime )
			continue;

		if ( sGlobalThemeIsForceDisabled( i ) )
		{

		}
		else
		if ( ( pGlobalThemeData->nStartMonth	== GTM_INVALID	|| ( pGlobalThemeData->nStartMonth		<= tTime.wMonth		&& pGlobalThemeData->nEndMonth		>= tTime.wMonth ) ) &&
			 ( pGlobalThemeData->nStartDay      == -1			|| ( pGlobalThemeData->nStartDay		<= tTime.wDay		&& pGlobalThemeData->nEndDay		>= tTime.wDay ) ) &&
			 ( pGlobalThemeData->nStartDayOfWeek== GTW_INVALID	|| ( pGlobalThemeData->nStartDayOfWeek	<= tTime.wDayOfWeek	&& pGlobalThemeData->nEndDayOfWeek	>= tTime.wDayOfWeek ) ) )
		{
			GlobalThemeEnable( pGame, i );
		} 
		else if ( sGlobalThemeIsForceEnabled( i ) )
		{
			GlobalThemeEnable( pGame, i );
		}
		else if ( GlobalThemeIsEnabled( pGame, i ) )
		{
			GlobalThemeDisable( pGame, i );
		}
	}

	//sGlobalThemesUpdateSpawnThemeMonsters(pGame);

	// Update again tomorrow
	int nSecondsUntilAfterMidnight = (((23 - tTime.wHour) * 60 + (59 - tTime.wMinute)) * 60 + (59 - tTime.wSecond));
	nSecondsUntilAfterMidnight += 60; // give us a buffer to make sure that we arrive tomorrow.
	int nGameTicksUntilAfterMidnight = nSecondsUntilAfterMidnight * GAME_TICKS_PER_SECOND;
	GameEventRegister( pGame, sUpdateGlobalThemes, NULL, NULL, EVENT_DATA(), nGameTicksUntilAfterMidnight );
}
