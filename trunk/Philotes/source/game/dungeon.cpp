//----------------------------------------------------------------------------
// FILE: dungeon.cpp
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "prime.h"
#include "dungeon.h"
#include "game.h"
#include "level.h"
#include "picker.h"
#include "room.h"
#include "excel.h"
#include "gameconfig.h"
#include "LevelAreas.h"
#include "c_adclient.h"
#include "difficulty.h"


//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
#define ROOMID_BITS					20
#define ROOMID_MASK					MAKE_NBIT_MASK(ROOMID, ROOMID_BITS)
#define ROOMID_SHIFT				0
#define LEVELID_MASK				MAKE_NBIT_MASK(ROOMID, 32 - ROOMID_BITS)
#define LEVELID_SHIFT				ROOMID_BITS

#define DEFAULT_DUNGEON_SEED		666


//----------------------------------------------------------------------------
// STATIC FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static unsigned int sRoomHashGetKey(
	ROOMID idRoom)
{
	return (idRoom % DUNGEON_ROOM_HASH_SIZE);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static ROOMID sDungeonMakeNewId(
	DUNGEON * dungeon,
	LEVEL * level)
{
	DWORD iter = ++dungeon->m_dwCurrentRoomId;
	ASSERT((iter & ROOMID_MASK) == iter);
	ASSERT((LEVELID)(level->m_idLevel & LEVELID_MASK) == level->m_idLevel);

//	return (((level->m_idLevel & LEVELID_MASK) << LEVELID_SHIFT) + ((iter & ROOMID_MASK) << ROOMID_SHIFT));
	return ((iter & ROOMID_MASK) << ROOMID_SHIFT);
}


//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL DungeonInit(
	GAME * game,
	DWORD dwSeed)
{
	ASSERT(game->m_Dungeon == NULL);
	DungeonFreeAds(game);
	DungeonFree(game);

	if (dwSeed == 0)
	{
		dwSeed = DEFAULT_DUNGEON_SEED;
	}

	DUNGEON * dungeon = (DUNGEON *)GMALLOCZ(game, sizeof(DUNGEON));

	for (unsigned int ii = 0; ii < MAX_DUNGEON_LEVELS; ++ii)
	{
		dungeon->m_DungeonSelections[ii].m_nAreaIDForDungeon = INVALID_ID;
		dungeon->m_DungeonSelections[ii].m_nAreaDifficulty = INVALID_ID;
	}
	game->m_Dungeon = dungeon;

	if (IS_SERVER(game))
	{
		dungeon->m_dwSeed = dwSeed;
		RandInit(dungeon->m_randSeed, dwSeed);
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void DungeonFreeAds(
	GAME * game)
{
	ASSERT_RETURN(game);
	if (!game->m_Dungeon)
	{
		return;
	}
	DUNGEON * dungeon = game->m_Dungeon;

	for (unsigned int ii = 0; ii < dungeon->m_nLevelCount; ++ii)
	{
#if !ISVERSION(SERVER_VERSION)
		if (AppIsHellgate() && IS_CLIENT(game) && !GameGetGameFlag(game, GAMEFLAG_DISABLE_ADS) && (unsigned int)AppGetCurrentLevel() == ii)
		{
			c_AdClientExitLevel(game, dungeon->m_Levels[ii]->m_nLevelDef);
		}
#endif
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void DungeonFree(
	GAME * game)
{
	ASSERT_RETURN(game);
	if (!game->m_Dungeon)
	{
		return;
	}
	DUNGEON * dungeon = game->m_Dungeon;

	for (unsigned int ii = 0; ii < dungeon->m_nLevelCount; ++ii)
	{
		LevelFree(game, (LEVELID)ii);
		GFREE(game, dungeon->m_Levels[ii]);
	}

	GFREE(game, dungeon->m_Levels);
	GFREE(game, dungeon);
	game->m_Dungeon = NULL;
}


inline int DungeonGetAreaIndexForDungeon( GAME* pGame,
										  int nArea )
{
	DUNGEON* pDungeon = pGame->m_Dungeon;	
	ASSERT_RETINVALID( nArea != INVALID_ID );
	for( int t = 0; t < MAX_DUNGEON_LEVELS; t++ )
	{
		if( pDungeon->m_DungeonSelections[t].m_nAreaIDForDungeon == nArea )
		{
			return t;			
		}
	}
	for( int t = 0; t < MAX_DUNGEON_LEVELS; t++ )
	{
		if( pDungeon->m_DungeonSelections[t].m_nAreaIDForDungeon == INVALID_ID )
		{
			pDungeon->m_DungeonSelections[t].m_nAreaIDForDungeon = nArea;
			return t;
		}
	}
	//this should never happen
	ASSERT_RETINVALID(  nArea == INVALID_ID );
	return -1;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void DungeonRemoveLevel(
	GAME * game,
	LEVEL * level)
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(level);

	DUNGEON * dungeon = game->m_Dungeon;
	ASSERT_RETURN(dungeon);

	for (unsigned int ii = 0; ii < dungeon->m_nLevelCount; ++ii)
	{
		if (dungeon->m_Levels[ii] == level)
		{
			if( AppIsTugboat() )
			{
				int nIndex = DungeonGetAreaIndexForDungeon( game, LevelGetLevelAreaID( level ) );
				if( nIndex != INVALID_ID )
				{
					dungeon->m_DungeonSelections[nIndex].m_nAreaIDForDungeon = INVALID_ID;
					dungeon->m_DungeonSelections[nIndex].m_nAreaDifficulty = INVALID_ID;
				}
			}
			LevelFree(game, (LEVELID)ii);
			GFREE(game, dungeon->m_Levels[ii]);
			dungeon->m_Levels[ii] = NULL;
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
LEVELID DungeonFindFreeId(
	GAME * game)
{
	ASSERT_RETINVALID(game);
	DUNGEON * dungeon = game->m_Dungeon;
	ASSERT_RETINVALID(dungeon);

	for (unsigned int ii = 0; ii < dungeon->m_nLevelCount; ii++)
	{
		if (dungeon->m_Levels[ii] == NULL)
		{
			return (LEVELID)ii;
		}
	}
	return (LEVELID)dungeon->m_nLevelCount;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD DungeonGetSeed(
	GAME* pGame)
{
	ASSERT_RETINVALID(pGame && pGame->m_Dungeon);
	DUNGEON* pDungeon = pGame->m_Dungeon;

	return pDungeon->m_dwSeed;
}




//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD DungeonGetSeed(
	GAME* pGame,
	int nLevelArea)
{
	ASSERT_RETINVALID(pGame && pGame->m_Dungeon);
	DUNGEON* pDungeon = pGame->m_Dungeon;
	if (AppIsHellgate())
	{
		// hellgate does not use level areas
		ASSERTX( nLevelArea == INVALID_LINK, "Expected no level area" );
		return DungeonGetSeed( pGame );
	}
	else
	{
		int nIndex = DungeonGetAreaIndexForDungeon( pGame, nLevelArea );
		ASSERT_RETZERO( nIndex != INVALID_INDEX );
		if( pDungeon->m_DungeonSelections[nIndex].m_dwAreaSeed == 0 ||
			pDungeon->m_DungeonSelections[nIndex].m_dwAreaSeed == INVALID_ID )
		{
			pDungeon->m_DungeonSelections[nIndex].m_dwAreaSeed = RandGetNum(pGame);
		}
		return pDungeon->m_DungeonSelections[nIndex].m_dwAreaSeed;
	}
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void DungeonSetSeed(
	GAME* pGame,
	DWORD dwSeed)
{
	ASSERT_RETURN(pGame && pGame->m_Dungeon);
	DUNGEON* pDungeon = pGame->m_Dungeon;

	pDungeon->m_dwSeed = dwSeed;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void DungeonSetDifficulty(
	GAME* pGame,
	UNIT *pPlayer,
	int nArea,
	int nDifficulty)
{
	
	ASSERT_RETURN(pGame && pGame->m_Dungeon);
	DUNGEON* pDungeon = pGame->m_Dungeon;	
	ASSERT_RETURN( nArea != INVALID_ID );
	int index = DungeonGetAreaIndexForDungeon( pGame, nArea );
	ASSERT_RETURN( index != INVALID_ID );
	int nMax = MYTHOS_LEVELAREAS::LevelAreaGetMaxDifficulty( nArea, pPlayer );
	int nMin = MYTHOS_LEVELAREAS::LevelAreaGetMinDifficulty( nArea, pPlayer );
	nDifficulty = MAX( nMin, nDifficulty );
	nDifficulty = MIN( nMax, nDifficulty );	
	pDungeon->m_DungeonSelections[index].m_nAreaDifficulty = nDifficulty;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int DungeonGetDifficulty(
	GAME* pGame,
	int nArea,
	UNIT *pPlayer )
{
	
	ASSERT(pGame && pGame->m_Dungeon);
	DUNGEON* pDungeon = pGame->m_Dungeon;
	int index = DungeonGetAreaIndexForDungeon( pGame, nArea );
	ASSERT_RETZERO( index != INVALID_ID );
	int nDifficultyOffset = pDungeon->m_DungeonSelections[index].m_nAreaDifficulty;
	if( nDifficultyOffset < 0 )
	{
		
		// this is shared -we don't own it.
		if( MYTHOS_LEVELAREAS::LevelAreaGetIsStaticWorld( nArea ) )
		{
			nDifficultyOffset = 1;
		}
		else
		{
			nDifficultyOffset = pPlayer ? UnitGetDifficultyOffset( pPlayer ) : 0;		
		}
		DungeonSetDifficulty( pGame, pPlayer, nArea, nDifficultyOffset );
		nDifficultyOffset = pDungeon->m_DungeonSelections[index].m_nAreaDifficulty;
	}
	return nDifficultyOffset;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void DungeonSetSeed(
	GAME* pGame,
	int nArea,
	DWORD dwSeed)
{
	ASSERT_RETURN(pGame && pGame->m_Dungeon);
	DUNGEON* pDungeon = pGame->m_Dungeon;

	int index = DungeonGetAreaIndexForDungeon( pGame, nArea );
	ASSERT_RETURN( index != INVALID_INDEX );
	pDungeon->m_DungeonSelections[index].m_dwAreaSeed = dwSeed;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int DungeonGetNumLevels(
	GAME* pGame)
{
	ASSERT_RETZERO(pGame && pGame->m_Dungeon);
	DUNGEON* pDungeon = pGame->m_Dungeon;

	return pDungeon->m_nLevelCount;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
LEVEL *DungeonGetFirstPopulatedLevel(
	GAME *pGame)
{
	ASSERTX_RETNULL( pGame, "Expected game" );
	int nNumLevels = DungeonGetNumLevels( pGame );
	for (int i = 0; i < nNumLevels; ++i)
	{
		LEVELID idLevel = (LEVELID)i;
		LEVEL *pLevel = LevelGetByID( pGame, idLevel );
		if ( pLevel && LevelIsPopulated( pLevel ) )
		{
			return pLevel;
		}
	}
	return NULL;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
LEVEL *DungeonGetFirstLevel(
	GAME *pGame)
{
	ASSERTX_RETNULL( pGame, "Expected game" );
	int nNumLevels = DungeonGetNumLevels( pGame );
	if (nNumLevels > 0)
	{
		return LevelGetByID( pGame, (LEVELID)0 );
	}
	return NULL;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
LEVEL *DungeonGetNextPopulatedLevel(
	LEVEL *pLevel)
{
	ASSERTX_RETNULL( pLevel, "Expected level" );
	GAME *pGame = LevelGetGame( pLevel );
	int nNumLevels = DungeonGetNumLevels( pGame );
	for (int i = (int)LevelGetID( pLevel ) + 1; i < nNumLevels; ++i)
	{
		LEVELID idLevel = (LEVELID)i;
		LEVEL *pLevelOther = LevelGetByID( pGame, idLevel );
		if ( pLevelOther &&  LevelIsPopulated( pLevelOther ))
		{
			return pLevelOther;
		}
	}
	return NULL;	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
LEVEL *DungeonGetNextLevel(
	LEVEL *pLevel)
{
	ASSERTX_RETNULL( pLevel, "Expected level" );
	GAME *pGame = LevelGetGame( pLevel );
	int nNumLevels = DungeonGetNumLevels( pGame );
	LEVELID idLevelNext = (LEVELID)((int)LevelGetID( pLevel ) + 1);
	if (idLevelNext < nNumLevels)
	{
		return LevelGetByID( pGame, idLevelNext );
	}
	return NULL;	
}	


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
ROOM * DungeonGetRoomByID(
	DUNGEON * dungeon,
	ROOMID idRoom)
{	
	ASSERT_RETNULL(dungeon);
	
	unsigned int key = sRoomHashGetKey(idRoom);
	ROOM * room = dungeon->m_RoomHash[key];
	while (room)
	{
		if (room->idRoom == idRoom)
		{
			return room;
		}
		room = room->m_pNextRoomInHash;
	}
	return NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void DungeonRemoveRoom(
	DUNGEON * dungeon,
	ROOM * room)
{	
	ASSERT_RETURN(dungeon);
	ASSERT_RETURN(room);
	
	if (room->m_pPrevRoomInHash)
	{
		room->m_pPrevRoomInHash->m_pNextRoomInHash = room->m_pNextRoomInHash;
	}
	else
	{
		unsigned int key = sRoomHashGetKey(room->idRoom);
		dungeon->m_RoomHash[key] = room->m_pNextRoomInHash;
	}
	if (room->m_pNextRoomInHash)
	{
		room->m_pNextRoomInHash->m_pPrevRoomInHash = room->m_pPrevRoomInHash;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void DungeonAddRoom(
	DUNGEON * dungeon,
	LEVEL * level,
	ROOM * room)
{	
	ASSERT_RETURN(dungeon);
	ASSERT_RETURN(level);
	ASSERT_RETURN(room);

	if (room->idRoom == INVALID_ID)
	{
		room->idRoom = sDungeonMakeNewId(dungeon, level);
	}
	unsigned int key = sRoomHashGetKey(room->idRoom);
	room->m_pNextRoomInHash = dungeon->m_RoomHash[key];
	dungeon->m_RoomHash[key] = room;
	if (room->m_pNextRoomInHash)
	{
		room->m_pNextRoomInHash->m_pPrevRoomInHash = room;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void DungeonPickDRLGs(
	GAME * game,
	int nDifficulty)
{
	DUNGEON * dungeon = game->m_Dungeon;
	ASSERT_RETURN( dungeon );

	int nNumRows = ExcelGetNumRows( game, DATATABLE_LEVEL );
	ASSERTX_RETURN( nNumRows < MAX_DUNGEON_LEVELS, "Too many levels in levels.xls, increase MAX_DUNGEON_LEVELS\n" );

	LEVEL_DRLG_CHOICE tDRLGChoices[MAX_LEVEL_DRLGS_CHOICES];
	game->nDifficulty = nDifficulty;
	// choose random drlgs for the game
	for ( int i = 0; i < nNumRows; i++ )
	{
		const LEVEL_DEFINITION * level_definition = LevelDefinitionGet( i );
		if(level_definition)
		{
			PickerStart( game, picker );
			int k = 0;
			UNIT* player ;
			if(IS_CLIENT(game))
				player = GameGetControlUnit(game);
			else
				player = NULL;
			int nNumDRLGChoices = ExcelGetNumRows(game, DATATABLE_LEVEL_DRLG_CHOICE);
			for ( int j = 0; ( j < nNumDRLGChoices ) && ( k < MAX_LEVEL_DRLGS_CHOICES ); j++ )
				//NOTE: this pretty much ignores the nNumDRLGS set in the excel, so we don't have to set a separate one for nightmare, hell, et.al. - wdm
			{
				LEVEL_DRLG_CHOICE* choice;
				choice = (LEVEL_DRLG_CHOICE*)ExcelGetData(game, DATATABLE_LEVEL_DRLG_CHOICE, j);
				if(choice && choice->nLevel == i && choice->nDifficulty == nDifficulty )
				{
					tDRLGChoices[k] = *choice;
					PickerAdd(MEMORY_FUNC_FILELINE(game, k, choice->nDRLGWeight));
					k++;
				}
			}
			int index = PickerChoose( game );
			const DIFFICULTY_DATA * pDifficultyData;
			pDifficultyData = (const DIFFICULTY_DATA *)ExcelGetData(game, DATATABLE_DIFFICULTY, nDifficulty);
			ASSERTV_CONTINUE(index != INVALID_INDEX, 
				"Couldn't find any matching DRLG choices for stage %s at difficulty level %s", 
				level_definition->pszName,
				pDifficultyData ? pDifficultyData->szName : "");

			if (index != -1)
			{
				dungeon->m_DungeonSelections[i].m_tDLRGChoice= tDRLGChoices[ index ];
				dungeon->m_DungeonSelections[i].m_nAreaDifficulty = INVALID_ID;
			}
		}
		else
		{
			LEVEL_DRLG_CHOICE &tDRLGChoice = dungeon->m_DungeonSelections[ i ].m_tDLRGChoice;
			LevelDRLGChoiceInit( tDRLGChoice );			
			dungeon->m_DungeonSelections[i].m_nAreaDifficulty = INVALID_ID;
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int DungeonGetLevelDRLG(
	GAME * game,
	int nLevelDefinition,
	BOOL bAllowNonServerGame )
{
	if ( !AppIsHammer() )
	{
		ASSERT( bAllowNonServerGame || IS_SERVER( game ) );
	}

	DUNGEON * dungeon = game->m_Dungeon;
	ASSERT_RETINVALID( dungeon );

	ASSERT_RETINVALID( nLevelDefinition != INVALID_LINK );

#ifdef _DEBUG
	const GAME_GLOBAL_DEFINITION* game_global_definition = DefinitionGetGameGlobal();
	ASSERT_RETINVALID(game_global_definition);
	if ( game_global_definition->nDRLGOverride != INVALID_ID )
	{
		return game_global_definition->nDRLGOverride;
	}
#endif

	const LEVEL_DRLG_CHOICE *pDLRGChoice = &dungeon->m_DungeonSelections[ nLevelDefinition ].m_tDLRGChoice;
	ASSERTX( pDLRGChoice->nDRLG != INVALID_LINK, "Expected DRLG def link" );
	return pDLRGChoice->nDRLG;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int DungeonGetLevelDRLG(
	GAME * game,
	LEVEL * level )
{
	ASSERT_RETINVALID( level );

	int index = level->m_nLevelDef;
	return DungeonGetLevelDRLG( game, index );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const struct LEVEL_DRLG_CHOICE *DungeonGetLevelDRLGChoice(
	GAME *pGame,
	int nLevelDef)
{
	ASSERTX_RETNULL( nLevelDef != INVALID_LINK && nLevelDef >= 0 && nLevelDef < MAX_DUNGEON_LEVELS, "Invalid level def" );
	ASSERTX_RETNULL( pGame, "Expected game" );
	DUNGEON *pDungeon = pGame->m_Dungeon;
	
	const LEVEL_DRLG_CHOICE *pDLRGChoice = &pDungeon->m_DungeonSelections[ nLevelDef ].m_tDLRGChoice;
	return pDLRGChoice;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const struct LEVEL_DRLG_CHOICE *DungeonGetLevelDRLGChoice(
	LEVEL *pLevel)
{
	ASSERTX_RETNULL( pLevel, "Expected level" );	
	return DungeonGetLevelDRLGChoice( LevelGetGame( pLevel ), LevelGetDefinitionIndex( pLevel ) );	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int DungeonGetLevelSpawnClass(
	GAME * game,
	int nLevelDefinition )
{
	DUNGEON * dungeon = game->m_Dungeon;
	ASSERT_RETINVALID( dungeon );

	ASSERT_RETINVALID( nLevelDefinition != INVALID_LINK );

	const LEVEL_DRLG_CHOICE *pDLRGChoice = &dungeon->m_DungeonSelections[ nLevelDefinition ].m_tDLRGChoice;
	ASSERTX( pDLRGChoice->nSpawnClass != INVALID_LINK, "Expected spawn class link" );
	return pDLRGChoice->nSpawnClass;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int DungeonGetLevelSpawnClass(
	GAME * game,
	LEVEL * level )
{
	ASSERT_RETINVALID( level );

	int index = level->m_nLevelDef;
	return DungeonGetLevelSpawnClass( game, index );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int DungeonGetLevelNamedMonsterClass(
	GAME * game,
	LEVEL * level )
{
	ASSERT_RETINVALID( level );

	int nLevelDefinition = level->m_nLevelDef;

	DUNGEON * dungeon = game->m_Dungeon;
	ASSERT_RETINVALID( dungeon );

	ASSERT_RETINVALID( nLevelDefinition != INVALID_LINK );

	const LEVEL_DRLG_CHOICE *pDLRGChoice = &dungeon->m_DungeonSelections[ nLevelDefinition ].m_tDLRGChoice;
	return pDLRGChoice->nNamedMonsterClass;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float DungeonGetLevelNamedMonsterChance(
	GAME * game,
	LEVEL * level )
{
	ASSERT_RETZERO( level );

	int nLevelDefinition = level->m_nLevelDef;

	DUNGEON * dungeon = game->m_Dungeon;
	ASSERT_RETZERO( dungeon );

	ASSERT_RETZERO( nLevelDefinition != INVALID_LINK );

	const LEVEL_DRLG_CHOICE *pDLRGChoice = &dungeon->m_DungeonSelections[ nLevelDefinition ].m_tDLRGChoice;
	return pDLRGChoice->fNamedMonsterChance;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int DungeonGetLevelMusicRef(
	GAME * game,
	LEVEL * level )
{
	ASSERT_RETINVALID( level );

	int nLevelDefinition = level->m_nLevelDef;

	DUNGEON * dungeon = game->m_Dungeon;
	ASSERT_RETINVALID( dungeon );

	ASSERT_RETINVALID( nLevelDefinition != INVALID_LINK );

	const LEVEL_DRLG_CHOICE *pDLRGChoice = &dungeon->m_DungeonSelections[ nLevelDefinition ].m_tDLRGChoice;
	return pDLRGChoice->nMusicRef;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// gets environment from the DRLG page, not from the override with level choices
int DungeonGetDRLGBaseEnvironment(
	int nDRLGDef )
{
	const DRLG_DEFINITION * pDRLGDef = DRLGDefinitionGet( nDRLGDef );
	ASSERTX_RETINVALID( pDRLGDef, "DRLG Definition not found" );
	return pDRLGDef->nEnvironment;
}


//----------------------------------------------------------------------------
// set on client with message from server
//----------------------------------------------------------------------------
void DungeonUpdateOnSetLevelMsg( 
	GAME * game, 
	const MSG_SCMD_SET_LEVEL *pMsg)
{
	int nLevelDefinition = pMsg->nLevelDefinition;

	ASSERT_RETURN( IS_CLIENT( game ) );

	DUNGEON * dungeon = game->m_Dungeon;
	ASSERT_RETURN( dungeon );

	ASSERT_RETURN( ( nLevelDefinition >= 0 ) && ( nLevelDefinition < MAX_DUNGEON_LEVELS ) );

	ASSERT( pMsg->nDRLGDefinition != INVALID_ID );
	ASSERT( pMsg->nSpawnClass != INVALID_ID );
	// cmarch: nNamedMonsterClass can be invalid, since it is optional 

	LEVEL_DRLG_CHOICE *pDLRGChoice = &dungeon->m_DungeonSelections[ nLevelDefinition ].m_tDLRGChoice;
	pDLRGChoice->nDRLG = pMsg->nDRLGDefinition;
	pDLRGChoice->nSpawnClass = pMsg->nSpawnClass;
	pDLRGChoice->nNamedMonsterClass = pMsg->nNamedMonsterClass;
	pDLRGChoice->fNamedMonsterChance = pMsg->fNamedMonsterChance;
	pDLRGChoice->nMusicRef = pMsg->nMusicRef;
	pDLRGChoice->nEnvironmentOverride = pMsg->nEnvDefinition;
}
