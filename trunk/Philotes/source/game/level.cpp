//----------------------------------------------------------------------------
// level.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "act.h"
#include "prime.h"
#include "game.h"
#include "globalindex.h"
#include "collision.h"
#include "district.h"
#include "dungeon.h"
#include "level.h"
#include "levelareas.h"
#include "room.h"
#include "room_path.h"
#include "room_layout.h"
#include "drlg.h"
#include "drlgpriv.h"
#include "c_granny_rigid.h"
#ifdef HAVOK_ENABLED
	#include "havok.h"
#endif
#include "stringtable.h"
#include "script.h"
#include "s_message.h"
#include "units.h"
#include "unit_priv.h"
#include "monsters.h"
#include "items.h"
#include "states.h"
#include "objects.h"
#include "c_sound.h"
#include "c_sound_util.h"
#include "c_sound_memory.h"
#include "c_backgroundsounds.h"
#include "c_music.h"
#include "sound_definition.h"
#include "player.h"
#include "clients.h"
#include "ai.h"
#include "collision.h"
#include "console.h"
#include "skills.h"
#include "vector.h"
#include "filepaths.h"
#include "pakfile.h"
#include "c_particles.h"
#include "performance.h"
#include "config.h"
#include "c_message.h"
#include "gameconfig.h"
#include "picker.h"
#include "spawnclass.h"
#include "e_model.h"
#include "e_material.h"
#include "e_environment.h"
#include "e_definition.h"
#include "e_drawlist.h"
#include "e_effect_priv.h"
#include "uix.h"
#include "e_visibilitymesh.h"
#include "gameunits.h"
#include "wardrobe.h"
#include "waypoint.h"
#include "warp.h"
#ifdef HAVOKFX_ENABLED
#include "e_havokfx.h"
#endif
#include "s_townportal.h"
#include "e_region.h"
#include "lightmaps.h"
#include "automap.h"
#include "e_settings.h"		// CHB 2006.06.28 - for e_GetRenderFlag()
#include "e_budget.h"
#include "e_profile.h"
#include "s_chatNetwork.h"
#include "s_adventure.h"
#include "tasks.h"
#include "e_main.h"
#include "weather.h"
#include "Quest.h"
#include "s_QuestServer.h"
#include "s_quests.h"
#include "c_GameMapsClient.h"
#include "s_GameMapsServer.h"
#include "s_LevelAreasServer.h"
#include "LevelAreaLinker.h"
#include "chatserver.h"
#include "c_adclient.h"
#include "combat.h"
#include "language.h"
#include "sku.h"
#include "QuestProperties.h"
#if ISVERSION(SERVER_VERSION)
#include "level.cpp.tmh"
#endif
#include "excel_private.h"
#include "perfhier.h"
#include "treasure.h"
#include "achievements.h"
#include "global_themes.h"
#include "c_QuestClient.h"
#include "QuestStructs.h"
#include "metagame.h"
#include "fillpak.h"

//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
#if ISVERSION(CHEATS_ENABLED)
static float sgflChampionSpawnRateOverride = INVALID_LINK;
int gnMonsterLevelOverride = -1;
int gnItemLevelOverride = -1;
#endif

// for now we go through all rooms in the level and send them, we do this because
// it has been deemed unnacceptable to load the rooms one at a time as the clients
// need them because of gfx loading is too slow currently, when we have that fixed
// we can remove this
BOOL gbClientsHaveVisualRoomsForEntireLevel = TRUE;

LEVEL_TYPE gtLevelTypeAny;
static DWORD ROOM_UPDATE_DELAY_IN_TICKS = GAME_TICKS_PER_SECOND / 2;

//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ExcelLevelsPostProcess(
	struct EXCEL_TABLE * table)
{
	return WaypointSystemInit(table);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ExcelLevelsFree(
	struct EXCEL_TABLE * table)
{
	REF(table);
	WaypointSystemFree();
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelLevelFreeRow( 
	EXCEL_TABLE * table,
	BYTE * rowdata)
{
	ASSERT_RETURN(table);
	ASSERT_RETURN(rowdata);

	LEVEL_DEFINITION * pLevelDef = (LEVEL_DEFINITION *)rowdata;

	if (pLevelDef->pnAchievements)
	{
		FREE(NULL, pLevelDef->pnAchievements);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSpawnMemoryFree(
	GAME *pGame,
	LEVEL *pLevel)
{
	ASSERT_RETURN(pLevel);
	while (pLevel->m_pSpawnClassMemory)
	{
		SPAWN_CLASS_MEMORY *pNext = pLevel->m_pSpawnClassMemory->pNext;
		GFREE( pGame, pLevel->m_pSpawnClassMemory);
		pLevel->m_pSpawnClassMemory = pNext;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static SPAWN_CLASS_MEMORY *sFindSpawnClassMemory(
	GAME *pGame,
	LEVELID idLevel,
	int nSpawnClassIndex)
{
	LEVEL *pLevel = LevelGetByID(pGame, idLevel);
	ASSERT_RETNULL(pLevel);
	for (SPAWN_CLASS_MEMORY *pMemory = pLevel->m_pSpawnClassMemory; pMemory; pMemory = pMemory->pNext)
	{
		if (pMemory->nSpawnClassIndex == nSpawnClassIndex)
		{
			return pMemory;
		}
	}
	return NULL;	
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SPAWN_CLASS_MEMORY* LevelGetSpawnMemory(
	GAME *pGame,
	LEVELID idLevel,
	int nSpawnClass)
{
	return sFindSpawnClassMemory(pGame, idLevel, nSpawnClass);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRememberLevelSpawnClassPicks(	
	GAME *pGame,
	int nSpawnClassIndex,
	int nPicks[ MAX_SPAWN_CLASS_CHOICES ],
	int nNumPicks,
	void *pUserData)
{

	// we only care about entries that say they should "remember picks"
	const SPAWN_CLASS_DATA *pSpawnClassData = SpawnClassGetData( pGame, nSpawnClassIndex );
	ASSERT_RETURN(pSpawnClassData);
	if (pSpawnClassData->bLevelRemembersPicks == TRUE)
	{
	
		LEVEL *level = (LEVEL *)pUserData;
		SPAWN_CLASS_MEMORY *pMemory = sFindSpawnClassMemory( pGame, level->m_idLevel, nSpawnClassIndex );
		if (pMemory == NULL)
		{
		
			// allocate new memory
			pMemory = (SPAWN_CLASS_MEMORY *)GMALLOCZ( pGame, sizeof( SPAWN_CLASS_MEMORY ) );
			pMemory->pNext = level->m_pSpawnClassMemory;
			level->m_pSpawnClassMemory = pMemory;
			
			// save picks info
			pMemory->nSpawnClassIndex = nSpawnClassIndex;
			pMemory->nNumPicks = nNumPicks;
			for (int i = 0; i < nNumPicks; ++i)
			{
				pMemory->nPicks[ i ] = nPicks[ i ];
			}

		}
		
	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const char *LevelGetDevName(
	const LEVEL *pLevel)
{
	ASSERTX_RETNULL( pLevel, "Expected level" );
	return pLevel->m_szLevelName;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const WCHAR *LevelDefinitionGetDisplayName(
	const LEVEL_DEFINITION *pLevelDefinition)
{
	ASSERT_RETVAL(pLevelDefinition, L"");
	return StringTableGetStringByIndex( pLevelDefinition->nLevelDisplayNameStringIndex );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const WCHAR *LevelDefinitionGetDisplayName(
	int nLevelDef)
{
	const LEVEL_DEFINITION *ptLevelDef = LevelDefinitionGet( nLevelDef );
	return LevelDefinitionGetDisplayName( ptLevelDef );
}
		
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const WCHAR *LevelGetDisplayName(
	const LEVEL *pLevel)
{
	ASSERTX_RETNULL( pLevel, "Expected level" );
	const LEVEL_DEFINITION * pLevelDef = LevelDefinitionGet( pLevel );
	return LevelDefinitionGetDisplayName( pLevelDef );
}
		
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LevelResetSpawnMemory(
	GAME *pGame,
	LEVEL *pLevel)
{
	ASSERT_RETURN(pLevel);

	// free any memory (if present)
	if (pLevel->m_pSpawnClassMemory != NULL)
	{
		sSpawnMemoryFree( pGame, pLevel );
	}
	ASSERTX( pLevel->m_pSpawnClassMemory == NULL, "Expected empty spawn memory for level" );
	
	// evaluate the level spawning tree and remember all the picks we chose
	for (int i = 0; i < MAX_LEVEL_DISTRICTS; ++i)
	{
		int nSpawnClass = pLevel->m_nDistrictSpawnClass[ i ];		
		if (nSpawnClass != INVALID_LINK)
		{
		
			SpawnTreeEvaluateDecisions( 
				pGame, 
				pLevel,
				nSpawnClass, 
				sRememberLevelSpawnClassPicks, 
				pLevel );

		}
		
	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_LevelCreateSubLevelEntrance(
	LEVEL *pLevel, 
	SUBLEVEL *pSubLevel)
{
	ASSERTX_RETURN( pLevel, "Expected level" );
	ASSERTX_RETURN( pSubLevel, "Expected sub level" );

	ASSERTX_RETURN( pLevel, "Expected level" );
	GAME *pGame = LevelGetGame( pLevel );	

	// start picker
	PickerStart( pGame, tPicker );	
	for (ROOM *pRoom = LevelGetFirstRoom( pLevel ); pRoom; pRoom = LevelGetNextRoom( pRoom ))
	{

		// check room for possible sublevel locations
		if (s_RoomSelectPossibleSubLevelEntranceLocation( pGame, pRoom, pSubLevel ) == FALSE)
		{
			continue;
		}

		// add room to picker
		PickerAdd(MEMORY_FUNC_FILELINE(pGame, RoomGetId( pRoom ), 1));
		
	}

	// pick one of the rooms to have the entrance portal
	ROOMID idRoom = PickerChoose( pGame );
	ASSERTX_RETURN( idRoom != INVALID_ID, "Expected room" );
	
	// get the room
	ROOM *pSelectedRoom = RoomGetByID( pGame, idRoom );
	ASSERTX_RETURN( pSelectedRoom, "Expected room" );

	// create the entrance object here			
	const SUBLEVEL_DEFINITION *ptSubLevelDef = SubLevelGetDefinition( pSubLevel );
	int nObjectClass = ptSubLevelDef->nObjectEntrance;
	ASSERTX_RETURN( nObjectClass != INVALID_LINK, "Expected entrance object class" );
	
	VECTOR vDirection = RandGetVectorXY( pGame );
	OBJECT_SPEC tSpec;
	tSpec.nClass = nObjectClass;
	tSpec.pRoom = pSelectedRoom;
	tSpec.vPosition = pSelectedRoom->vPossibleSubLevelEntrance;
	tSpec.pvFaceDirection = &vDirection;
	UNIT *pObject = s_ObjectSpawn( pGame, tSpec );

	// check for errors
	ASSERTX_RETURN( pObject, "Unable to create sub level entrance room unit" );

	// set the entrance object
//	s_SubLevelSetDoorway( pSubLevel, pObject, SD_ENTRANCE );
						
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static ROOM_ITERATE_UNITS sDestroyUnusedSublevelDoorways(
	UNIT *pUnit,
	void *pCallbackData)
{
	UNITID idUnit = UnitGetId( pUnit );
	
	LEVEL *pLevel = UnitGetLevel( pUnit );
	ASSERTX_RETVAL( pLevel, RIU_CONTINUE, "Expected level" );
	
	// check each sublevel
	int nNumSubLevels = LevelGetNumSubLevels( pLevel );
	for (int i = 0; i < nNumSubLevels; ++i)
	{
		SUBLEVEL *pSubLevel = LevelGetSubLevelByIndex( pLevel, i );
		
		// must be an entrance or exit object for this sublevel
		const SUBLEVEL_DEFINITION *pSubLevelDef = SubLevelGetDefinition( pSubLevel );
		ASSERTX_CONTINUE( pSubLevelDef, "Expected sublevel def" );
		if (UnitIsGenusClass( pUnit, GENUS_OBJECT, pSubLevelDef->nObjectEntrance ) ||
			UnitIsGenusClass( pUnit, GENUS_OBJECT, pSubLevelDef->nObjectExit ))
		{
		
			// see if this doorway is actually in use by a sublevel in this level			
			BOOL bInUse = FALSE;			
			for (int j = 0; j < SD_NUM_DOORWAYS; ++j)
			{
				const SUBLEVEL_DOORWAY *pDoorway = s_SubLevelGetDoorway( pSubLevel, (SUBLEVEL_DOORWAY_TYPE)j );
				if (pDoorway->idDoorway == idUnit)
				{
					bInUse = TRUE;
					break;  // no need to keep searching
				}
				
			}
			
			// if not in use, destroy this unit
			if (bInUse == FALSE)
			{
				UnitFree( pUnit );
			}	
			
		}
		
	}

	return RIU_CONTINUE;  // keep searching
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sLevelDestroyUnselectedSubLevelDoorways( 
	LEVEL *pLevel)
{
	ASSERTX_RETURN( pLevel, "Expected level" );
	
	// iterate all objects on this level
	TARGET_TYPE eTargetTypes[] = { TARGET_OBJECT, TARGET_INVALID };
	LevelIterateUnits( pLevel, eTargetTypes, sDestroyUnusedSublevelDoorways, NULL );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sLevelSetupAllSublevelDoorways(
	LEVEL *pLevel)
{

	// go through all our sub levels
	int nNumSubLevels = LevelGetNumSubLevels( pLevel );
	for (int i = 0;  i < nNumSubLevels; ++i)
	{
		SUBLEVEL *pSubLevel = LevelGetSubLevelByIndex( pLevel, i );
		s_SubLevelSetupDoorways( pSubLevel );		
	}

	// destroy any leftover doorway objects
	s_sLevelDestroyUnselectedSubLevelDoorways( pLevel );
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int s_sLevelSpawnMonster( 
	LEVEL *pLevel,
	int nMonsterClass,
	int nMonsterQuality)
{
#if defined(_DEBUG) || ISVERSION(DEVELOPMENT) || defined(USERNAME_cbatson)	// CHB 2006.09.27 - Allow no monsters on release build.
	GLOBAL_DEFINITION * pGlobal = DefinitionGetGlobal();
	if ( (pGlobal->dwFlags & GLOBAL_FLAG_ABSOLUTELYNOMONSTERS) != 0 )
	{
		return 0;
	}
#endif

	ASSERTX_RETVAL( pLevel, 0, "Expected level" );
	GAME *pGame = LevelGetGame( pLevel );
	int nNumSpawned = 0;

	if ( nMonsterClass != INVALID_LINK )
	{
	
		// lets try this a few times
		int nTries = 32;
		while (nTries--)
		{
		
			// pick a room suitable for this monster
			DWORD dwRandomRoomFlags = 0;
			SETBIT( dwRandomRoomFlags, RRF_PATH_NODES_BIT );
			SETBIT( dwRandomRoomFlags, RRF_MUST_ALLOW_MONSTER_SPAWN_BIT );
			SETBIT( dwRandomRoomFlags, RRF_NO_ROOMS_WITH_NO_SPAWN_NODES_BIT );
			
			SUBLEVEL *pSubLevel = LevelGetPrimarySubLevel( pLevel );
			ROOM *pRoom = s_SubLevelGetRandomRoom( pSubLevel, dwRandomRoomFlags, NULL, NULL );
			ASSERTX_RETVAL( pRoom, 0, "Unable to get random room for unqiue to spawn in" );

			// get node to spawn monster at
			DWORD dwRandomNodeFlags = 0;
			SETBIT( dwRandomNodeFlags, RNF_MUST_ALLOW_SPAWN );
			int nNodeIndex = s_RoomGetRandomUnoccupiedNode( pGame, pRoom, dwRandomNodeFlags );	
			if (nNodeIndex != INVALID_INDEX )
			{
				ROOM_PATH_NODE *pNode = RoomGetRoomPathNode( pRoom, nNodeIndex );

				// create a spawn entry just for this single monster
				SPAWN_ENTRY tSpawn;
				tSpawn.eEntryType = SPAWN_ENTRY_TYPE_MONSTER;
				tSpawn.nIndex = nMonsterClass;
				tSpawn.codeCount = (PCODE)CODE_SPAWN_ENTRY_SINGLE_COUNT;

				// setup location to spawn at
				SPAWN_LOCATION tSpawnLocation;
				tSpawnLocation.vSpawnPosition = RoomPathNodeGetExactWorldPosition( pGame, pRoom, pNode );
				tSpawnLocation.vSpawnDirection = RandGetVectorXY( pGame );
				tSpawnLocation.vSpawnUp = cgvZAxis;
				tSpawnLocation.fRadius = 0.0f;
				// create it
				nNumSpawned = s_RoomSpawnMonsterByMonsterClass( 
					pGame, 
					&tSpawn, 
					nMonsterQuality, 
					pRoom, 
					&tSpawnLocation, 
					NULL, 
					NULL);
				if (nNumSpawned > 0)
				{
					break;  // exit while
				}

			}

		}
		
	}
	
	return nNumSpawned;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sLevelSpawnUniqueMonster( 
	LEVEL *pLevel)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)

#if defined(_DEBUG) || ISVERSION(DEVELOPMENT) || defined(USERNAME_cbatson)	// CHB 2006.09.27 - Allow no monsters on release build.
	GLOBAL_DEFINITION * pGlobal = DefinitionGetGlobal();
	if ( (pGlobal->dwFlags & GLOBAL_FLAG_ABSOLUTELYNOMONSTERS) != 0 )
	{
		return;
	}
#endif

	ASSERTX_RETURN( pLevel, "Expected level" );
	GAME *pGame = LevelGetGame( pLevel );

	// get the possible monsters for this level
	const int MAX_MONSTER_CLASSES = 2048;
	int nAvailableMonsterClasses[ MAX_MONSTER_CLASSES ];
	int nNumAvailableClasses = 0;

	// TODO check unique monster class overrides, when implemented

	// no unique monster override, pick from district spawn classes
	if (nNumAvailableClasses == 0)
	{
		nNumAvailableClasses = s_LevelGetPossibleRandomMonsters( 
			pLevel, 
			nAvailableMonsterClasses, 
			MAX_MONSTER_CLASSES, 
			0,
			TRUE);
	}

	if (nNumAvailableClasses > 0)
	{

		// setup picker
		PickerStart( pGame, tPicker );

		// scan through the monsters possible
		int nNumAvailableUniques = 0;		
		for (int i = 0 ;i < nNumAvailableClasses; ++i)
		{
			int nMonsterClass = nAvailableMonsterClasses[ i ];
			const UNIT_DATA *pMonsterData = MonsterGetData( NULL, nMonsterClass );
			if (pMonsterData->nMonsterClassAtUniqueQuality != INVALID_LINK)
			{
				PickerAdd(MEMORY_FUNC_FILELINE(pGame, i, 1));
				nNumAvailableUniques++;			
			}
		}

		// pick monster
		if (nNumAvailableUniques > 0)
		{

			// get monster class
			int nPick = PickerChoose( pGame );
			int nMonsterClass = nAvailableMonsterClasses[ nPick ];
			ASSERTX_RETURN( nMonsterClass != INVALID_LINK, "Expected monster class" );
			
			// get quality to spawn
			int nMonsterQuality = MonsterQualityPick( pGame, MQT_UNIQUE );

			// attempt to create it
			s_sLevelSpawnMonster( 
				pLevel, 
				nMonsterClass, 
				nMonsterQuality);

		}

	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sLevelAttemptSpawnNamedMonster( 
	LEVEL *pLevel)
{
#if defined(_DEBUG) || ISVERSION(DEVELOPMENT) || defined(USERNAME_cbatson)	// CHB 2006.09.27 - Allow no monsters on release build.
	GLOBAL_DEFINITION * pGlobal = DefinitionGetGlobal();
	if ( (pGlobal->dwFlags & GLOBAL_FLAG_ABSOLUTELYNOMONSTERS) != 0 )
	{
		return;
	}
#endif

	ASSERTX_RETURN( pLevel, "Expected level" );
	GAME *pGame = LevelGetGame( pLevel );

	// get the possible monsters for this level
	const int MAX_MONSTER_CLASSES = 64;
	int nAvailableMonsterClasses[ MAX_MONSTER_CLASSES ];
	float fAvailableMonsterChances[ MAX_MONSTER_CLASSES ];
	int nNumAvailableClasses = 0;

	PickerStart( pGame, tPicker );
	for (int i = 0; i < MAX_LEVEL_DISTRICTS; ++i)
	{
		int nSpawnClass = pLevel->m_nDistrictNamedMonsterClass[ i ];
		if (nSpawnClass != INVALID_ID)
		{
			ASSERTX_BREAK(nNumAvailableClasses < arrsize(nAvailableMonsterClasses), "more districts than expected, increase MAX_MONSTER_CLASSES");
			nAvailableMonsterClasses[nNumAvailableClasses] = nSpawnClass;
			fAvailableMonsterChances[nNumAvailableClasses] = 
						pLevel->m_fDistrictNamedMonsterChance[ i ];
			PickerAdd(MEMORY_FUNC_FILELINE(pGame, nNumAvailableClasses, 1));
			++nNumAvailableClasses;
		}
	}

	if (nNumAvailableClasses > 0)
	{
		// get monster class
		int nPick = PickerChoose( pGame );
		int nMonsterClass = nAvailableMonsterClasses[ nPick ];
		ASSERTX_RETURN( nMonsterClass != INVALID_LINK, "Expected monster class" );

		float fMonsterChance = fAvailableMonsterChances[ nPick ];

		if (fMonsterChance > 0.0f)
		{
			float flRoll = RandGetFloat( pGame, 0.0f, 100.0f );
			if (flRoll < fMonsterChance)
			{
				// get quality to spawn
				const UNIT_DATA *pMonsterData = MonsterGetData( NULL, nMonsterClass );
				ASSERTX_RETURN( pMonsterData != NULL, "unexpected NULL monster data for named monster to spawn");

				// attempt to create it
				s_sLevelSpawnMonster( 
					pLevel, 
					nMonsterClass, 
					pMonsterData->nRequiredQuality);
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sLevelPopulateAllSubLevels(
	LEVEL *pLevel,
	UNIT *pActivator)
{	
	ASSERTX_RETURN( pLevel, "Expected level" );
	
	// go through all sublevels
	int nNumSubLevels = LevelGetNumSubLevels( pLevel );
	for (int i = 0; i < nNumSubLevels; ++i)
	{
		SUBLEVEL *pSubLevel = SubLevelGetByIndex( pLevel, i );
		SUBLEVELID idSubLevel = SubLevelGetId( pSubLevel );
		s_SubLevelPopulate( pLevel, idSubLevel, pActivator );
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAddPossibleUnit( 
	LEVEL *pLevel, 
	int nClass,
	GENUS eGenus)
{
	ASSERTX_RETURN( pLevel, "Expected level" );
	ASSERTX_RETURN( eGenus >= 0 && eGenus < NUM_GENUS, "Invalid genus" );
	LEVEL_POSSIBLE_UNITS *pPossibleUnits = &pLevel->m_tPossibleUnits[ eGenus ];

	// search for existing entry (yah, slow, but only done once on level creation)
	for (int i = 0; i < pPossibleUnits->nCount; ++i)
	{
		if (pPossibleUnits->nClass[ i ] == nClass)
		{
			return;
		}
	}

	// add new entry
	ASSERTX_RETURN( pPossibleUnits->nCount <= LEVEL_MAX_POSSIBLE_UNITS - 1, "Too many possible units" );
	pPossibleUnits->nClass[ pPossibleUnits->nCount++ ] = nClass;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sLevelGetPossibleUnits( 
	LEVEL *pLevel)
{
	ASSERTX_RETURN( pLevel, "Expected level" );
	GAME *pGame = LevelGetGame( pLevel );
	int nLevelDef = LevelGetDefinitionIndex( pLevel );

	// setup buffer for possible monsters
	const int MAX_MONSTERS = 2048;
	int nMonsterClasses[ MAX_MONSTERS ];
	int nNumMonsters = 0;	
	
	//// first, add the possible random monsters
	//nNumMonsters = s_LevelGetPossibleRandomMonsters( 
	//	pLevel, 
	//	nMonsterClasses, 
	//	MAX_MONSTERS, 
	//	nNumMonsters, 
	//	TRUE);

	// add possible monsters from DRLG choices (random monsters)
	const LEVEL_DRLG_CHOICE *pDRLGChoice = DungeonGetLevelDRLGChoice( pLevel );
	LevelDRLGChoiceGetPossilbeMonsters(
		pGame,
		pDRLGChoice,
		nMonsterClasses,
		MAX_MONSTERS,
		&nNumMonsters,
		TRUE,
		pLevel);

	// add monster results to level struct
	for (int i = 0; i < nNumMonsters; ++i)
	{
		sAddPossibleUnit( pLevel, nMonsterClasses[ i ], GENUS_MONSTER );
	}			
	
	// TRAVIS: Mythos doesn't have this stuff -
	if( AppIsHellgate() )
	{
		// get the interesting quest units for quests that are concerned with this level
		for (int nQuest = 0; nQuest < NUM_QUESTS; ++nQuest)
		{
			const QUEST_DEFINITION *pQuestDef = QuestGetDefinition( nQuest );
			if (pQuestDef)
			{
			
				// search for this level in the quest def
				BOOL bPreloadQuestWithLevel = FALSE;
				for (int i = 0; i < QUEST_MAX_PRELOAD_LEVELS; ++i)
				{
					if (pQuestDef->nLevelDefPreloadedWith[ i ] == nLevelDef)
					{
						bPreloadQuestWithLevel = TRUE;
						break;
					}
				}
				
				// load if found
				if (bPreloadQuestWithLevel == TRUE)
				{
					const APP_QUEST_GLOBAL *pAppQuestGlobal = QuestGetAppQuestGlobal( nQuest );
					for (int j = 0; j < pAppQuestGlobal->nNumInterstingGI; ++j)
					{
						GLOBAL_INDEX eGlobalIndex = pAppQuestGlobal->eInterestingGI[ j ];
						EXCELTABLE eTable = GlobalIndexGetDatatable( eGlobalIndex );
						int nIndex = GlobalIndexGet( eGlobalIndex );
						
						// only care about stuff from a couple of tables
						switch (eTable)
						{
						
							//----------------------------------------------------------------------------				
							case DATATABLE_MONSTERS:
							{
								sAddPossibleUnit( pLevel, nIndex, GENUS_MONSTER );
								break;
							}
							//----------------------------------------------------------------------------
							case DATATABLE_OBJECTS:
							{
								sAddPossibleUnit( pLevel, nIndex, GENUS_OBJECT );
								break;
							}
							//----------------------------------------------------------------------------
							default:
							{
								break;
							}
							
						}
						
					}
					
				}
				
			}	
				
		}
	}
			
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static ROOM_ITERATE_UNITS sAddObjectsForTreasureSpawn(
	UNIT *pUnit,
	void *pCallbackData)
{
	CArrayList *arrayList = (CArrayList * )pCallbackData;
	if( pUnit &&
		pUnit->pUnitData->nUnitTypeSpawnTreasureOnLevel != INVALID_ID )
	{
		ArrayAddGetIndex(*arrayList, pUnit);
		//arrayList->Add( pUnit );
	}
	return RIU_CONTINUE;
}


static void s_LevelRegisterTreasureCreationOnUnitDeath( GAME *pGame,
													    LEVEL *pLevel,
														UNIT *pActivator )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERTX_RETURN( pLevel, "Expected level" );
	ASSERTX_RETURN( pGame, "Expected game" );
	ASSERTX_RETURN( pActivator, "Expected game" );
	//Ended up having to add it to an arraylist because the room gets iterated multiple times,
	//causing an assert.
	CArrayList ObjectsToCreate( AL_REFERENCE, sizeof( pActivator ) );
	TARGET_TYPE eTargetTypes[] = { TARGET_OBJECT, TARGET_INVALID };
	LevelIterateUnits( pLevel, eTargetTypes, sAddObjectsForTreasureSpawn, &ObjectsToCreate ); 
	for( UINT t = 0; t < ObjectsToCreate.Count(); t++ )
	{
		UNIT *pUnit = (UNIT *)ObjectsToCreate.GetPtr( t );
		if( pUnit )
		{
			ROOM *pRoom( NULL );
			if( UnitDataTestFlag( pUnit->pUnitData, UNIT_DATA_FLAG_LEVELTREASURE_SPAWN_BEFORE_UNIT ) )
				pRoom = UnitGetRoom( pUnit );
			s_LevelRegisterTreasureDropRandom( UnitGetGame( pUnit ),
											   UnitGetLevel( pUnit ),
											   pUnit->pUnitData->nUnitTypeSpawnTreasureOnLevel,
											   GlobalIndexGet( GI_OBJECT_LEVELTREASURE_FALLBACK ),
											   pRoom );

		}
	}
	ObjectsToCreate.Destroy();
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
static void s_sLevelUpdateRooms( 
							   LEVEL *pLevel)
{
	ASSERTX_RETURN( pLevel, "Expected level" );

	// we only if the level needs an room update anyway
	if (pLevel->m_bNeedRoomUpdate == TRUE)
	{

		// we only do this every once in a while
		GAME *pGame = LevelGetGame( pLevel );
		DWORD dwNow = GameGetTick( pGame );

		// for now just do this work brute force ... I'll make this more elegant soon -Colin
		if (pLevel->m_dwLastRoomUpdateTick < ROOM_UPDATE_DELAY_IN_TICKS ||
			dwNow - pLevel->m_dwLastRoomUpdateTick > ROOM_UPDATE_DELAY_IN_TICKS)
		{

			for (ROOM *pRoom = LevelGetFirstRoom( pLevel ); pRoom; pRoom = LevelGetNextRoom( pRoom ))
			{
				s_RoomUpdate( pRoom );
			}

			// update has been done
			LevelNeedRoomUpdate( pLevel, FALSE );
			pLevel->m_dwLastRoomUpdateTick = dwNow;

		}

	}	

}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
static ROOM_ITERATE_UNITS s_sLevelSetHellriftWarpsSetWarpDestination(
	UNIT *pUnit,
	void *pCallbackData )
{
	ASSERT_RETVAL(pUnit, RIU_CONTINUE);
	if(!UnitIsA(pUnit, UNITTYPE_HELLRIFT_PORTAL))
	{
		return RIU_CONTINUE;
	}

	LEVEL *pLevel = UnitGetLevel( pUnit );

	SUBLEVELID idSubLevel = s_ObjectGetOppositeSubLevel(pUnit);
	int nDRLGDefinition = SubLevelGetDRLG(pLevel, idSubLevel);
	ASSERT_RETVAL(nDRLGDefinition >= 0, RIU_CONTINUE);

	UnitSetStat(pUnit, STATS_WARP_DEST_LEVEL_DEF, nDRLGDefinition);
	return RIU_CONTINUE;
}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

static void s_sLevelSetHellriftWarps(
	LEVEL * pLevel)
{
#if !ISVERSION( CLIENT_ONLY_VERSION )
	TARGET_TYPE eTargetTypes[] = { TARGET_OBJECT, TARGET_INVALID };
	LevelIterateUnits( pLevel, eTargetTypes, s_sLevelSetHellriftWarpsSetWarpDestination, NULL );
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sLevelPopulate(
	LEVEL* pLevel,
	UNITID idActivator)
{
	ASSERTX_RETURN( pLevel, "Expected level" );
	GAME *pGame = LevelGetGame( pLevel );	
	UNIT *pActivator = UnitGetById( pGame, idActivator );
	const LEVEL_DEFINITION *ptLevelDef = LevelDefinitionGet( pLevel );
			
	// you cannot populate a level that is already populated
	if (LevelTestFlag( pLevel, LEVEL_POPULATED_BIT ))
	{
		return;
	}

	// debug
	trace( "Populating level '%s'\n", LevelGetDevName( pLevel ));
	
	// run DRLG	
	DRLGCreateLevel( pLevel->m_pGame, pLevel, pLevel->m_dwSeed, idActivator );

	// populate all sublevels (with things like warps etc.)
	s_sLevelPopulateAllSubLevels( pLevel, pActivator );

	if( AppIsTugboat() )
	{
		s_QuestLevelPopulate( pGame, pLevel, pActivator );
		MYTHOS_LEVELAREAS::s_LevelAreaPopulateLevel( pLevel );
		//Spawn classes for rooms turn
		ROOM *pRoom = pLevel->m_pRooms;
		int nCreated( 0 );
		while (pRoom)
		{
			const ROOM_INDEX *pRoomIndex = RoomGetRoomIndex( pGame, pRoom );
			for( int t = 0; t < MAX_SPAWN_CLASS_PER_ROOM; t++ )
			{
				if( pRoomIndex && pRoomIndex->nSpawnClass[t] != INVALID_ID )
				{
						nCreated += s_RoomCreateSpawnClass( pRoom, pRoomIndex->nSpawnClass[t], pRoomIndex->nSpawnClassRunXTimes[t] );
				}
			}
			pRoom = pRoom->pNextRoomInLevel;
		}
		REF( nCreated );
		
	}	

	if( AppIsHellgate() )
	{
		// setup any sublevel entrances and exits
		s_sLevelSetupAllSublevelDoorways( pLevel );

		// populate the level with random quests
		int nRoll = RandGetNum( pGame, 0, 100 );
		if (nRoll <= ptLevelDef->nAdventureChancePercent)
		{
			s_AdventurePopulate( pLevel, pActivator );
		}

	}

	// do unique monster spawn chances
	if (ptLevelDef->flUniqueSpawnChance > 0.0f)
	{
		float flRoll = RandGetFloat( pGame, 0.0f, 100.0f );
		if (flRoll < ptLevelDef->flUniqueSpawnChance)
		{
			s_sLevelSpawnUniqueMonster( pLevel );
		}
	}

	// named monster spawns		
	s_sLevelAttemptSpawnNamedMonster( pLevel );
	
	// Setup event callback for monsters that drop items for level
	s_LevelRegisterTreasureCreationOnUnitDeath( pGame, pLevel, pActivator );
	if( AppIsTugboat() )
	{
	
		MYTHOS_LEVELAREAS::s_LevelAreaRegisterItemCreationOnUnitDeath( pGame, pLevel, pActivator );
		s_QuestRegisterItemCreationOnUnitDeath( pGame, pLevel, pActivator );
	}

	// get and cache a list units that can occur in this level, we will send this
	// list to clients when they load the level so that they can preload the gfx for
	// all the units
	s_sLevelGetPossibleUnits( pLevel );
	
	// Set some stats on the hellrift warp units so that the clients can display the name properly
	s_sLevelSetHellriftWarps( pLevel );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int s_sSelectNumDistricts( 
	GAME* pGame,
	const DRLG_DEFINITION* pDRLGDefinition)
{
	int nNumDistricts = 1;
	// drb - forcing to 1 for now...
/*	if (pDRLGDefinition->codeNumDistricts != NULL_CODE)
	{
		int nCodeLength = 0;
		BYTE *pCode = ExcelGetScriptCode( pGame, DATATABLE_LEVEL_DRLGS, pDRLGDefinition->codeNumDistricts, &nCodeLength );
		if (pCode)
		{
			nNumDistricts = VMExecI( pGame, pCode, nCodeLength );
		}	
	}
*/
	// must have at least one	
	if (nNumDistricts <= 0)
	{
		nNumDistricts = 1;  // must have at least one district
	}

	// can't have over the max
	if (nNumDistricts > MAX_LEVEL_DISTRICTS)
	{
		nNumDistricts = MAX_LEVEL_DISTRICTS;
	}
			
	return nNumDistricts;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sLevelAllocateDistricts(
	GAME* pGame,
	LEVEL* pLevel)
{
	ASSERTX_RETURN( pGame, "Expected game" );
	ASSERTX_RETURN( pLevel, "Expected level" );
	ASSERTX_RETURN( s_LevelGetNumDistricts( pLevel ) == 0, "Level already has districts" );
	
	// pick how many districts we want to have	
	const DRLG_DEFINITION * pDRLGDefinition = LevelGetDRLGDefinition( pLevel );
	int nNumDistricts = s_sSelectNumDistricts( pGame, pDRLGDefinition );

	// create each district
	for (int i = 0; i < nNumDistricts; ++i)
	{
		DISTRICT* pDistrict = DistrictCreate( pGame, pLevel );
		ASSERTX_CONTINUE( pDistrict, "Unable to create district" );

		// assign to level
		ASSERTX_CONTINUE( pLevel->m_nNumDistricts < MAX_LEVEL_DISTRICTS, "Too many districts created for level" );
		pLevel->m_pDistrict[ pLevel->m_nNumDistricts++ ] = pDistrict;		

	}

	ASSERTX( s_LevelGetNumDistricts( pLevel ) > 0, "Level must have at least 1 district" );
	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int LevelGetStartNewGameLevelDefinition(
	void)
{
	int nLevelDef = INVALID_LINK;
	int nNumLevelDefs = ExcelGetNumRows( NULL, DATATABLE_LEVEL );	

	BOOL bCheatLevels = FALSE;

#if defined(_DEBUG) || ISVERSION(DEVELOPMENT) 
	GLOBAL_DEFINITION * pGlobals = DefinitionGetGlobal();
	if ( ( pGlobals->dwFlags & GLOBAL_FLAG_CHEAT_LEVELS ) != 0 )
		bCheatLevels = TRUE;
#endif
	// if we are *not* skipping the tutorial, find the tutorial level
/*	if (gbSkipTutorial == FALSE)
	{	

		// go through all level defs and find the first tutorial
		for (int i = 0 ; i < nNumLevelDefs; ++i)
		{
			const LEVEL_DEFINITION *pLevelDef = LevelDefinitionGet( i );
			if (pLevelDef->bTutorial == TRUE)
			{
				nLevelDef = i;
				break;
			}
			
		}
		
	}
	else*/
	{
	
		// go through all level defs and find the first level
		for (int i = 0 ; i < nNumLevelDefs; ++i)
		{
			const LEVEL_DEFINITION *pLevelDef = LevelDefinitionGet( i );
			if (! pLevelDef)
				continue;
			if ((bCheatLevels && pLevelDef->bFirstLevelCheating) ||
				(!bCheatLevels && pLevelDef->bFirstLevel) )
			{
				nLevelDef = i;
				break;
			} 			
		}
		
	}

	ASSERTX( nLevelDef != INVALID_LINK, "Unable to find first level of game" );		
	return nLevelDef;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL LevelIsActionLevelDefinition(
	const LEVEL_DEFINITION *pLevelDefinition)
{
	ASSERTX_RETFALSE( pLevelDefinition, "Expected level definition" );
	SRVLEVELTYPE eLevelType = pLevelDefinition->eSrvLevelType;
	switch (eLevelType)
	{
		case SRVLEVELTYPE_DEFAULT:		return TRUE;
		case SRVLEVELTYPE_TUTORIAL:		return TRUE;
		case SRVLEVELTYPE_MINITOWN:		return FALSE;
		case SRVLEVELTYPE_BIGTOWN:		return FALSE;
		case SRVLEVELTYPE_CUSTOM:		return TRUE;
		case SRVLEVELTYPE_PVP_PRIVATE:	return TRUE;
		case SRVLEVELTYPE_PVP_CTF:		return TRUE;
		case SRVLEVELTYPE_OPENWORLD:	return TRUE;
		default:						ASSERT_RETFALSE(0);
	}
	ASSERTX_RETTRUE( 0, "Unknown server level type" );	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL LevelIsActionLevel(
	LEVEL *pLevel)
{
	ASSERTX_RETFALSE( pLevel, "Expected level" );
	return LevelIsActionLevelDefinition( pLevel->m_pLevelDefinition );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL LevelAllowsUnpartiedPlayers(
	LEVEL *pLevel)
{
	ASSERTX_RETFALSE( pLevel, "Expected level" );
	return !LevelIsActionLevel( pLevel ) || pLevel->m_pLevelDefinition->eSrvLevelType == SRVLEVELTYPE_PVP_CTF ||
		pLevel->m_pLevelDefinition->eSrvLevelType == SRVLEVELTYPE_OPENWORLD;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LevelRemoveRoom(
	GAME *pGame,
	LEVEL *pLevel,
	ROOM *pRoom)
{
	ASSERTX_RETURN( pGame, "Expected game" );
	ASSERTX_RETURN( pGame->m_Dungeon, "Expected dungeon" );
	ASSERTX_RETURN( pLevel, "Expected level" );
	ASSERTX_RETURN( pRoom, "Expected room" );
	ASSERTX_RETURN( pLevel->m_nRoomCount < MAX_ROOMS_PER_LEVEL, "Too many rooms for level" );
	
	// remove from level list
	if( pLevel->m_pRooms == pRoom )	//special case the first room
	{
		pLevel->m_pRooms = pRoom->pNextRoomInLevel;
	}
	if( pRoom->pNextRoomInLevel )
		pRoom->pNextRoomInLevel->pPrevRoomInLevel = pRoom->pPrevRoomInLevel;
	if( pRoom->pPrevRoomInLevel )
		pRoom->pPrevRoomInLevel->pNextRoomInLevel = pRoom->pNextRoomInLevel;

	
	// keep running total of level area
	pLevel->m_flArea -= RoomGetArea( pRoom );
	if ( AppIsTugboat() )
	{
		BOUNDING_BOX BBox;
		BBox.vMin = pRoom->pHull->aabb.center - pRoom->pHull->aabb.halfwidth;
		BBox.vMax = pRoom->pHull->aabb.center + pRoom->pHull->aabb.halfwidth;
		BBox.vMax.fZ = 4.2f;
		BBox.vMin.fZ = 0;
		pLevel->m_pQuadtree->Remove(pRoom, BBox);
	}	
	// remove room from dungeon
	DUNGEON *pDungeon = pGame->m_Dungeon;
	DungeonRemoveRoom( pDungeon, pRoom );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DISTRICT* s_LevelGetDistrict(
	LEVEL* pLevel,
	int nDistrict)
{
	ASSERTX_RETNULL( pLevel, "Expected level" );
	int nNumDistricts = s_LevelGetNumDistricts( pLevel );
	ASSERTX_RETNULL( nDistrict >= 0 && nDistrict < nNumDistricts, "Invalid district number for level" );
	return pLevel->m_pDistrict[ nDistrict ];
}	


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LevelFree(
	GAME * game,
	LEVELID idLevel)
{
	LEVEL * level = LevelGetByID(game, idLevel);
	if (!level)
	{
		return;
	}
	
	//Destroy the level of Interest items
	level->m_UnitsOfInterest.Destroy();

#if !ISVERSION(CLIENT_ONLY_VERSION)
	// free chat channel
	if (IS_SERVER(game) && level->m_idChatChannel != INVALID_CHANNELID)
	{
		s_ChatNetReleaseChatChannel( level->m_idChatChannel );
	}

	if ((LevelIsTown(level) &&
		( GameGetSubType(game) == GAME_SUBTYPE_MINITOWN ||
		  GameGetSubType(game) == GAME_SUBTYPE_OPENWORLD ) &&
		level->m_idWarpCreator == INVALID_ID) ||
		(AppIsHellgate() && GameIsPVP(game)) )
	{
		if( AppIsHellgate() )
		{
			s_ChatNetInstanceRemoved(GameGetId(game), LevelGetDefinitionIndex(level), 0);
		}
		else
		{
			s_ChatNetInstanceRemoved(GameGetId(game), LevelGetLevelAreaID(level), LevelGetPVPWorld(level) ? 1 : 0 );
		}
	}
#endif

	// client logic
	if (IS_CLIENT(game))
	{
		// free automap stuff
		c_LevelFreeAutomapInfo(game, level);
	}
	
	// free districts
	for (int ii = 0; ii < level->m_nNumDistricts; ++ii)
	{
		DISTRICT * district = s_LevelGetDistrict(level, ii);
		DistrictFree(game, district);
	}
	level->m_nNumDistricts = 0;
	
	// free spawn memory
	sSpawnMemoryFree(game, level);

	// free spawn-free zones
	if(level->m_pSpawnFreeZones)
	{
		GFREE(game, level->m_pSpawnFreeZones);
		level->m_pSpawnFreeZones = NULL;
	}

	// free level collision	
	LevelCollisionFree(game, level);

	// remove rooms
	ROOM * room = level->m_pRooms;
	// we should not do this, we must be allowed to look at the rooms of a 
	// level as they are being destroyed so we can properly cleanup stuff, but it's
	// too big of a change today for TGS -Colin
	level->m_pRooms = NULL;	// <-- move this after rooms are free and update count as we free them
	while (room)
	{
		ROOM * next = room->pNextRoomInLevel;
		LevelRemoveRoom(game, level, room);
		RoomFree(game, room, TRUE);
		room = next;
	}
	level->m_nRoomCount = 0;

	// units' positions are recorded in level's prox map to reduce move msg spam (MYTHOS SERVER TOWN LEVELS ONLY)
	if (level->m_pUnitPosProxMap) {
		level->m_pUnitPosProxMap->Free();
		MEMORYPOOL_DELETE(GameGetMemPool(game), level->m_pUnitPosProxMap);
	}

#ifdef HAVOK_ENABLED
	// close havok world
	if ( AppIsHellgate() )
		HavokCloseWorld(&level->m_pHavokWorld);
#endif

	// remove level from dungeon
	DUNGEON * dungeon = game->m_Dungeon;
	ASSERT_RETURN(dungeon);	
	int index = level->m_idLevel;
	ASSERT_RETURN(dungeon->m_Levels[index] == level);
	dungeon->m_Levels[index] = NULL;

	// reset engine portals
	if (IS_CLIENT( game ))
	{
		CLT_VERSION_ONLY(e_PortalsReset());
	}
	
	if ( AppIsTugboat() )
	{
		if( level->m_pQuadtree )
		{
			level->m_pQuadtree->Free();
			GFREE( game, level->m_pQuadtree );
			level->m_pQuadtree = NULL;
		}
#if !ISVERSION(SERVER_VERSION)
		if( level->m_pVisibility )
		{
			delete( level->m_pVisibility );
			level->m_pVisibility = NULL;
			e_VisibilityMeshSetCurrent( level->m_pVisibility );
		}
#endif
	}

	// free level memory
	GFREE(game, level);
#if ISVERSION(SERVER_VERSION)
	if(game->pServerContext )PERF_ADD64(game->pServerContext->m_pPerfInstance,GameServer,GameServerLevelCount,-1);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sPaintNoSpawnZones(
	ROOM *pRoom,
	void *pUserData)
{
	const LEVEL_SPAWN_FREE_ZONE *pZone = (const LEVEL_SPAWN_FREE_ZONE *)pUserData;
	LEVEL *pLevel = RoomGetLevel( pRoom );
	GAME *pGame = LevelGetGame( pLevel );
	int nNodesInZone = 0;
	
	// go through all path nodes in this zoom
	ROOM_PATH_NODE_SET *pRoomPathNodes = RoomGetPathNodeSet( pRoom );	
	if (pRoomPathNodes != NULL)
	{
		for (int i = 0; i < pRoomPathNodes->nPathNodeCount; ++i)
		{
			ROOM_PATH_NODE *pNode = RoomGetRoomPathNode( pRoom, i );		
				
			// get world position
			VECTOR vPositionWorld = RoomPathNodeGetExactWorldPosition( pGame, pRoom, pNode );
			
			// check no spawn zone
			if (LevelPositionIsInSpecificSpawnFreeZone( pLevel, pZone, &vPositionWorld ) == TRUE)
			{
				PATH_NODE_INSTANCE *pNodeInstance = RoomGetPathNodeInstance( pRoom, pNode->nIndex );
				SETBIT( pNodeInstance->dwNodeInstanceFlags, NIF_NO_SPAWN );
				++nNodesInZone;
			}

		}

	}
			
	// if there is a node in the spawn free zone, set flag on room
	if (nNodesInZone > 0)
	{
		RoomSetFlag( pRoom, ROOM_HAS_NO_SPAWN_NODES_BIT, TRUE );

		if (nNodesInZone >= pRoomPathNodes->nPathNodeCount)
			RoomSetFlag( pRoom, ROOM_NO_SPAWN_BIT, TRUE );
	}
		
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LevelAddSpawnFreeZone(
	GAME *pGame,
	LEVEL *pLevel,
	const VECTOR &vPosition,
	const float flRadius,
	BOOL bMarkPathNodes)
{
	ASSERTX_RETURN( pLevel, "Expected level" );

	// add another zone
	pLevel->m_nNumSpawnFreeZones++;

	// reallocate memory	
	pLevel->m_pSpawnFreeZones = (LEVEL_SPAWN_FREE_ZONE*)GREALLOC(
		pGame, 
		pLevel->m_pSpawnFreeZones, 
		pLevel->m_nNumSpawnFreeZones * sizeof( LEVEL_SPAWN_FREE_ZONE ));
	
	// get last zone and init
	LEVEL_SPAWN_FREE_ZONE *pZone = &pLevel->m_pSpawnFreeZones[ pLevel->m_nNumSpawnFreeZones - 1 ];	
	pZone->vPosition = vPosition;
	pZone->fRadiusSquared = flRadius * flRadius;

	// iterate rooms and paint any path nodes in the no spawn flag
	if (bMarkPathNodes)
	{
		LevelIterateRooms( pLevel, sPaintNoSpawnZones, pZone );
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL LevelPositionIsInSpecificSpawnFreeZone(
	const LEVEL *pLevel,
	const LEVEL_SPAWN_FREE_ZONE *pZone,
	const VECTOR *pvPositionWorld)
{
	ASSERTX_RETFALSE( pLevel, "Expected level" );
	ASSERTX_RETFALSE( pZone, "Expected zone" );
	ASSERTX_RETFALSE( pvPositionWorld, "Expected position" );
	
	float fDistanceSquared = VectorDistanceSquared( *pvPositionWorld, pZone->vPosition );
	if (fDistanceSquared < pZone->fRadiusSquared)
	{
		return TRUE;
	}
	
	return FALSE;
	
}	
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL LevelPositionIsInSpawnFreeZone(
	const LEVEL *pLevel,
	const VECTOR *pvPositionWorld)
{
	ASSERTX_RETFALSE( pLevel, "Expected level" );
	
	// this could no doubt be optimized -Colin
	for (int i = 0; i < pLevel->m_nNumSpawnFreeZones; ++i)
	{
		const LEVEL_SPAWN_FREE_ZONE *pZone = &pLevel->m_pSpawnFreeZones[ i ];	
		if (LevelPositionIsInSpecificSpawnFreeZone( pLevel, pZone, pvPositionWorld ))
		{
			return TRUE;
		}
	}
	
	return FALSE;  // not in a spawn free zone
	
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
LEVEL* LevelGetByID(
	GAME* game,
	LEVELID idLevel)
{
	ASSERT_RETNULL(game && game->m_Dungeon);
	DUNGEON* dungeon = game->m_Dungeon;

	if ((unsigned int)idLevel >= dungeon->m_nLevelCount)
	{
		return NULL;
	}

	LEVEL* level = dungeon->m_Levels[idLevel];
	return level;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL LevelHasRooms(
	GAME* game,
	LEVELID idLevel)
{
	LEVEL* level = LevelGetByID(game, idLevel);
	ASSERT_RETFALSE(level);
	return (level->m_nRoomCount > 0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL LevelIsPopulated(
	LEVEL *pLevel)
{
	ASSERTX_RETFALSE( pLevel, "Expected level" );
	return TESTBIT( pLevel->m_dwFlags, LEVEL_POPULATED_BIT );
}

//----------------------------------------------------------------------------
// Returns percentage [0, 100] of rooms which support monsters spawns, and 
// have once been populated.
//----------------------------------------------------------------------------
int LevelGetMonstersSpawnedRoomsPercent(
	LEVEL *pLevel)
{
	ASSERT_RETNULL(pLevel);
	int nPopulatedOnceRooms = 0;
	int nMonsterSpawnRooms = 0;
	for ( ROOM * pRoom = LevelGetFirstRoom( pLevel ); pRoom; pRoom = LevelGetNextRoom( pRoom ) )
	{
		// figure out if a room *really* allows monster spawn... 
		if ( RoomAllowsMonsterSpawn( pRoom ) &&
			 pRoom->pLayoutSelections &&
			 pRoom->pLayoutSelections->pRoomLayoutSelections &&
			 pRoom->pLayoutSelections->pRoomLayoutSelections[ROOM_LAYOUT_ITEM_SPAWNPOINT].nCount > 0 )
		{
			++nMonsterSpawnRooms;
			if ( RoomTestFlag( pRoom, ROOM_POPULATED_ONCE_BIT ) )
				++nPopulatedOnceRooms;
		}
	}
	return nMonsterSpawnRooms == 0 ? 0 : ( ( nPopulatedOnceRooms * 100 ) / nMonsterSpawnRooms );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sActivateRoom(
	ROOM * room,
	void * data)
{
	UNIT * activator = (UNIT *)data;
	s_RoomActivate(room, activator);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
void s_DebugLevelActivateAllRooms(
	LEVEL * level,
	UNIT * activator)
{
	ASSERT_RETURN(level);
	ASSERT_RETURN(activator);
	LevelIterateRooms(level, sActivateRoom, activator);
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LevelAddRoom(
	GAME* game,
	LEVEL* level,
	ROOM* room)
{
	ASSERTX_RETURN(game, "Expected game");
	ASSERTX_RETURN(game->m_Dungeon, "Expected dungeon");
	ASSERTX_RETURN(level, "Expected level");
	ASSERTX_RETURN(room, "Expected room");
	ASSERTX_RETURN(level->m_nRoomCount < MAX_ROOMS_PER_LEVEL, "Too many rooms for level");

	// tie to level
	room->pNextRoomInLevel = level->m_pRooms;
	room->pPrevRoomInLevel = NULL;
	if (level->m_pRooms)
	{
		level->m_pRooms->pPrevRoomInLevel = room;
	}
	level->m_pRooms = room;
	room->nIndexInLevel = level->m_nRoomCount++;

	// keep running total of level area
	level->m_flArea += RoomGetArea( room );

	// tie to dungeon
	DungeonAddRoom(game->m_Dungeon, level, room);
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DRLG_LEVEL_DEFINITION* DRLGLevelDefinitionGet(
	const char* pszFolder,
	const char* pszFileName)
{
	if (pszFolder[0] == 0 || pszFileName[0] == 0)
	{
		return NULL;
	}

	char pszFileNameWithPath[DEFAULT_FILE_WITH_PATH_SIZE];
	PStrPrintf(pszFileNameWithPath, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s", pszFolder, pszFileName);

	if ( AppCommonIsAnyFillpak() && ! FillPakShouldLoad( pszFileName ) )
		return NULL;

	ConsoleDebugString( OP_LOW, LOADING_STRING(DRLG level definition), pszFileNameWithPath );

	if ( AppCommonIsFillpakInConvertMode() ) 
	{
		AppSetDebugFlag( ADF_FORCE_ASSERTS_ON, TRUE );
		AppSetDebugFlag( ADF_IN_CONVERT_STEP, TRUE );
	}

	e_GrannyUpdateDRLGFile(pszFileName, pszFileNameWithPath);

	if ( AppCommonIsFillpakInConvertMode() ) 
	{
		AppSetDebugFlag( ADF_FORCE_ASSERTS_ON, FALSE );
		AppSetDebugFlag( ADF_IN_CONVERT_STEP, FALSE );
	}

	char pszDRLGFileName[MAX_XML_STRING_LENGTH];
	PStrReplaceExtension(pszDRLGFileName, MAX_XML_STRING_LENGTH, pszFileNameWithPath, DRLG_FILE_EXTENSION);

	DECLARE_LOAD_SPEC( spec, pszDRLGFileName );
	spec.flags |= PAKFILE_LOAD_ADD_TO_PAK;
	BYTE* pbyFileStart = (BYTE*)PakFileLoadNow(spec);

	ASSERTV_RETNULL( pbyFileStart, "DRLG file not found!\n%s", pszDRLGFileName );

	FILE_HEADER* pFileHeader = (FILE_HEADER*)pbyFileStart;
	ASSERT(pFileHeader->dwMagicNumber == DRLG_FILE_MAGIC_NUMBER);
	if (pFileHeader->dwMagicNumber != DRLG_FILE_MAGIC_NUMBER)
	{
		FREE(NULL, pbyFileStart);
		return NULL;
	}
	ASSERT(pFileHeader->nVersion == DRLG_FILE_VERSION);
	if (pFileHeader->nVersion != DRLG_FILE_VERSION)
	{
		FREE(NULL, pbyFileStart);
		return NULL;
	}

	DRLG_LEVEL_DEFINITION* pDefinition = (DRLG_LEVEL_DEFINITION*)(pbyFileStart + sizeof(FILE_HEADER));
	pDefinition->tHeader.nId = INVALID_ID;
	PStrCopy(pDefinition->tHeader.pszName, pszDRLGFileName, MAX_XML_STRING_LENGTH);
	pDefinition->pbyFileStart = pbyFileStart;

	// convert offsets to pointers
	pDefinition->pTemplates = (DRLG_LEVEL_TEMPLATE *)(pbyFileStart + (int)CAST_PTR_TO_INT(pDefinition->pTemplates));
	for (int ii = 0; ii < pDefinition->nTemplateCount; ii++)
	{
		pDefinition->pTemplates[ii].pRooms = (DRLG_ROOM*)(pbyFileStart + (int)CAST_PTR_TO_INT(pDefinition->pTemplates[ii].pRooms));
	}
	if (pDefinition->pSubsitutionRules)
	{
		pDefinition->pSubsitutionRules = (DRLG_SUBSTITUTION_RULE*)(pbyFileStart + (int)CAST_PTR_TO_INT(pDefinition->pSubsitutionRules));
		for (int ii = 0; ii < pDefinition->nSubstitutionCount; ii++)
		{
			DRLG_SUBSTITUTION_RULE* pSub = pDefinition->pSubsitutionRules + ii;
			pSub->pRuleRooms = (DRLG_ROOM*)(pbyFileStart + (int)CAST_PTR_TO_INT(pSub->pRuleRooms));
			for (int jj = 0; jj < pSub->nReplacementCount; jj++)
			{
				pSub->ppReplacementRooms[jj] = (DRLG_ROOM*)( pbyFileStart + PtrToInt( pSub->__p64_ppReplacementRooms[jj] ) );
			}
		}
	}

	return pDefinition;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int DRLGGetDefinitionIndexByName(
	const char *pszName)
{
	ASSERTX_RETINVALID( pszName, "Expected name string" );
	return ExcelGetLineByStringIndex(EXCEL_CONTEXT(), DATATABLE_LEVEL_DRLGS, pszName);
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define MAX_RULE_LABELS		32

typedef struct
{
	char	szLabel[DEFAULT_INDEX_SIZE];
	int		nLine;
} RULE_LABEL;

static RULE_LABEL sgRuleLabelTbl[MAX_RULE_LABELS];
static int sgnNumRuleLabels = 0;


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAddRuleLabel(
	const char * pszLabel,
	int nLine)
{
	ASSERT_RETURN(pszLabel);
	ASSERT_RETURN(sgnNumRuleLabels < MAX_RULE_LABELS);
	PStrCopy(sgRuleLabelTbl[sgnNumRuleLabels].szLabel, pszLabel, DEFAULT_INDEX_SIZE);
	sgRuleLabelTbl[sgnNumRuleLabels].nLine = nLine;
	sgnNumRuleLabels++;
}


//----------------------------------------------------------------------------

int LevelRuleGetLine( char * pszLabel )
{
	for ( int i = 0; i < sgnNumRuleLabels; i++ )
	{
		if ( PStrCmp( sgRuleLabelTbl[i].szLabel, pszLabel ) == 0 )
			return sgRuleLabelTbl[i].nLine;
	}
	return -1;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sExcelLevelRulesPostProcessLoadLayouts(
	const struct ROOM_LAYOUT_GROUP *ptLayoutDef,
	void *pCallbackData)
{
	REF(pCallbackData);
	switch(ptLayoutDef->eType)
	{
	case ROOM_LAYOUT_ITEM_GRAPHICMODEL:
		{
			char pszFileNameWithPath[MAX_XML_STRING_LENGTH];
			PStrPrintf(pszFileNameWithPath, MAX_XML_STRING_LENGTH, "%s", ptLayoutDef->pszName);

			PropDefinitionGet(NULL, pszFileNameWithPath, FALSE, 0.0f );
		}
		break;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ExcelLevelRulesPostLoadAll(
	EXCEL_TABLE * table)
{
	BOOL QuickStartDesktopServerIsEnabled(void);
	if (AppGetType() != APP_TYPE_CLOSED_SERVER || AppTestDebugFlag(ADF_FILL_PAK_BIT) || QuickStartDesktopServerIsEnabled())
	{
		return TRUE;
	}

	for (unsigned int ii = 0; ii < ExcelGetCountPrivate(table); ++ii)
	{
		DRLG_PASS * def = (DRLG_PASS *)ExcelGetDataPrivate(table, ii);
		if ( ! def )// this can commonly happen when we have different package/versions
			continue;
		//ASSERTV_CONTINUE(def, "No def found for row %d of table %s.", ii+2, table->m_Name);

		OVERRIDE_PATH tOverride;
		OverridePathByLine(def->nPathIndex, &tOverride );
		ASSERT_CONTINUE(tOverride.szPath[0]);
		def->pDrlgDef = DRLGLevelDefinitionGet(tOverride.szPath, def->pszDrlgFileName);

		DRLGPassIterateLayouts(def, sExcelLevelRulesPostProcessLoadLayouts, NULL);
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ExcelLevelRulesPostProcess(
	EXCEL_TABLE * table)
{
	sgnNumRuleLabels = 0;
	for (unsigned int ii = 0; ii < ExcelGetCountPrivate(table); ++ii)
	{
		DRLG_PASS * def = (DRLG_PASS *)ExcelGetDataPrivate(table, ii);
		if(!def)
			continue;

		if (def->pszLabel && def->pszLabel[0])
		{
			sAddRuleLabel(def->pszLabel, ii);
		}
	}

	BOOL QuickStartDesktopServerIsEnabled(void);
	if (AppGetType() != APP_TYPE_CLOSED_SERVER || AppTestDebugFlag(ADF_FILL_PAK_BIT) || QuickStartDesktopServerIsEnabled())
	{
		return TRUE;
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelLevelRulesFreeRow( 
	EXCEL_TABLE * table,
	BYTE * rowdata)
{
	ASSERT_RETURN(table);
	ASSERT_RETURN(rowdata);

	DRLG_PASS * def = (DRLG_PASS *)rowdata;
	if (def->pDrlgDef)
	{
		FREE(NULL, def->pDrlgDef->pbyFileStart);	// warning, this frees pDrlgDef
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DRLG_LEVEL_DEFINITION* DrlgGetLevelRuleDefinition(
	DRLG_PASS *pPass )
{
	ASSERT_RETNULL( pPass );
	if (!pPass->pDrlgDef)
	{
		OVERRIDE_PATH tOverride;
		OverridePathByLine( pPass->nPathIndex, &tOverride );
		ASSERT( tOverride.szPath[ 0 ] );
		pPass->pDrlgDef = DRLGLevelDefinitionGet(
			tOverride.szPath, 
			pPass->pszDrlgFileName);
	}
	return pPass->pDrlgDef;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ExcelLevelFilePathPostProcessRow(
	EXCEL_TABLE * table,
	unsigned int row,
	BYTE * rowdata)
{
	ASSERT_RETFALSE(table);
	ASSERT_RETFALSE(rowdata);
	REF(row);

	LEVEL_FILE_PATH * level_file_path = (LEVEL_FILE_PATH *)rowdata;
	ASSERT_RETFALSE(level_file_path->pszPath);
	PStrForceBackslash(level_file_path->pszPath, DEFAULT_FILE_WITH_PATH_SIZE);

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ExcelRoomIndexPostProcess(
	EXCEL_TABLE * table)
{
	if( table->m_Index == DATATABLE_PROPS )
		return TRUE;
#if !ISVERSION(SERVER_VERSION)
	//if(!AppIsFillingPak())

	// We need to load these all the time now to ensure that they get frontloaded
	// in the fillpak.
	{
		for (unsigned int ii = 0; ii < ExcelGetCountPrivate(table); ++ii)
		{
			ROOM_INDEX * room_index = (ROOM_INDEX *)ExcelGetDataPrivate(table, ii);
			if ( ! room_index )
				continue;
			if (room_index->pszReverbFile)
			{
				room_index->pReverbDefinition = c_SoundMemoryLoadReverbDefinitionByFile(room_index->pszReverbFile);
			}
			ASSERTV(room_index->pReverbDefinition, "Room %s has no reverb file set or reverb file (%s) could not be loaded.", room_index->pszFile, room_index->pszReverbFile);
		}
	}
#endif // !ISVERSION(SERVER_VERSION)

	BOOL QuickStartDesktopServerIsEnabled(void);
	if (AppGetType() != APP_TYPE_CLOSED_SERVER || AppTestDebugFlag(ADF_FILL_PAK_BIT) || QuickStartDesktopServerIsEnabled())
	{
		return TRUE;
	}

	for (unsigned int ii = 0; ii < ExcelGetCountPrivate(table); ++ii)
	{
		ROOM_INDEX * room_index = (ROOM_INDEX *)ExcelGetDataPrivate(table, ii);
		if ( ! room_index )
			continue;
		room_index->pRoomDefinition = RoomDefinitionGet(NULL, ii, FALSE);

		char szFilePath[DEFAULT_FILE_WITH_PATH_SIZE];
		PStrPrintf(szFilePath, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s", room_index->pszFile, ROOM_PATH_NODE_SUFFIX);
		ROOM_PATH_NODE_DEFINITION * pPathNodeDef = (ROOM_PATH_NODE_DEFINITION *)DefinitionGetByName(DEFINITION_GROUP_ROOM_PATH_NODE, szFilePath);
		REF(pPathNodeDef);
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRoomDefinitionFree( 
	ROOM_DEFINITION * room_def)
{
#ifdef HAVOK_ENABLED
	if (AppIsHellgate())
	{
		if (room_def->tHavokShapeHolder.pHavokShape)
		{
			HavokReleaseShape(room_def->tHavokShapeHolder);
		}
		//if (room_def->ppHavokMultiShape) 
		//{
		//	for (int ii = 0; ii < room_def->nHavokShapeCount; ++ii)
		//	{
		//		ASSERT_CONTINUE(room_def->ppHavokMultiShape[ii]);
		//		HavokReleaseShape(room_def->ppHavokMultiShape[ii]);
		//	}
		//	FREE(NULL, room_def->ppHavokMultiShape);
		//	room_def->ppHavokMultiShape = NULL;
		//}
	}
#endif
	if (room_def->pbFileStart)
	{
		FREE(NULL, room_def->pbFileStart);	// warning this frees the room_def itself!
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ExcelRoomIndexFreeRow( 
	EXCEL_TABLE * table,
	BYTE * rowdata)
{
	ASSERT_RETURN(table);
	ASSERT_RETURN(rowdata);

	ROOM_INDEX * room_index = (ROOM_INDEX *)rowdata;

	if (room_index->pRoomDefinition)
	{
		sRoomDefinitionFree(room_index->pRoomDefinition);
	}
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
struct ROOM_SORT
{
	ROOM * pRoom;
	float fDistance;
};
//-------------------------------------------------------------------------------------------------
static int sCompareRoomSort( const void * pFirst, const void * pSecond )
{
	ROOM_SORT * p1 = (ROOM_SORT *) pFirst;
	ROOM_SORT * p2 = (ROOM_SORT *) pSecond;
	if ( p1->fDistance > p2->fDistance )
		return 1;
	else if ( p1->fDistance < p2->fDistance )
		return -1;
	return 0;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
struct ROOM_INDEX_SORT
{
	int nIndex;
	BYTE bDistance;
};
//-------------------------------------------------------------------------------------------------
static int sCompareRoomIndexSort( const void * pFirst, const void * pSecond )
{
	ROOM_INDEX_SORT * p1 = (ROOM_INDEX_SORT *) pFirst;
	ROOM_INDEX_SORT * p2 = (ROOM_INDEX_SORT *) pSecond;
	if ( p1->bDistance > p2->bDistance )
		return 1;
	else if ( p1->bDistance < p2->bDistance )
		return -1;
	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float s_LevelGetChampionSpawnRate(
	GAME *pGame,
	LEVEL *pLevel)
{
	ASSERTX_RETZERO( pLevel, "Expected level" );
	
	// check for cheating/development override
#if ISVERSION(CHEATS_ENABLED)
	if (sgflChampionSpawnRateOverride >= 0.0f)
	{
		return sgflChampionSpawnRateOverride;
	} 
	
#endif

	// check for override in the DRLG for this level
	const DRLG_DEFINITION * pDRLGDefinition = LevelGetDRLGDefinition( pLevel );
	ASSERTX_RETZERO( pDRLGDefinition, "Expected drlg definition" );
	if (pDRLGDefinition->flChampionSpawnRateOverride != -1.0f)
	{
		if( AppIsTugboat() )
		{
			return pDRLGDefinition->flChampionSpawnRateOverride * pLevel->m_LevelAreaOverrides.fChampionSpawnMult;
		}
		else
		{
			return pDRLGDefinition->flChampionSpawnRateOverride;
		}
	}

	// use the one defined in the level def
	const LEVEL_DEFINITION *ptLevelDef = LevelDefinitionGet( pLevel );
	ASSERTX_RETZERO( ptLevelDef, "Expected level definition" );
	if( AppIsTugboat() )
	{
		return ptLevelDef->flChampionSpawnRate * pLevel->m_LevelAreaOverrides.fChampionSpawnMult;
	}
	else
	{
		return ptLevelDef->flChampionSpawnRate;
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_LevelSetChampionSpawnRateOverride(
	float flRate)
{
#if ISVERSION(CHEATS_ENABLED)
	sgflChampionSpawnRateOverride = flRate;

	if (sgflChampionSpawnRateOverride >= 0.0f)
	{		
		ConsoleString( CONSOLE_SYSTEM_COLOR, L"Champion Spawn Rate Override = '%.02f'", sgflChampionSpawnRateOverride );
	}
	else
	{
		ConsoleString( CONSOLE_SYSTEM_COLOR, L"Champion Spawn Rate Override = 'Disabled'" );
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int s_LevelSpawnMonsters(
	GAME* pGame,
	LEVEL* pLevel,
	int nMonsterClass,
	int nMonsterCount,
	int nMonsterQuality,
	ROOM *pRoom /*= NULL*/,
	PFN_MONSTER_SPAWNED_CALLBACK pfnSpawnedCallback /*= NULL*/,
	void *pCallbackData /*= NULL*/)
{
	int nSpawnCount = 0;
#if !ISVERSION(CLIENT_ONLY_VERSION)

#if defined(_DEBUG) || ISVERSION(DEVELOPMENT) || defined(USERNAME_cbatson)	// CHB 2006.09.27 - Allow no monsters on release build.
	GLOBAL_DEFINITION * pGlobal = DefinitionGetGlobal();
	if ( (pGlobal->dwFlags & GLOBAL_FLAG_NOMONSTERS) != 0 )
	{
		return 0;
	}
#endif

	ASSERTX_RETZERO( nMonsterCount > 0, "No valid monsters to spawn specified");
	ASSERT_RETZERO( LevelTestFlag( pLevel, LEVEL_POPULATED_BIT ) );

	const int MAX_MONSTER_CLASSES = 2048;
		int pnMonsterClassesTemp[ MAX_MONSTER_CLASSES ];
	int nMonsterClassesTempCount = 1, nMonsterClassIndex = 0;
	if ( nMonsterClass == INVALID_LINK )
	{
		nMonsterClassesTempCount = 
			s_LevelGetPossibleRandomMonsters( 
				pLevel, 
				pnMonsterClassesTemp, 
				arrsize( pnMonsterClassesTemp ), 
				0,
				TRUE);
	}
	else
	{
		pnMonsterClassesTemp[0] = nMonsterClass;
	}

	// eliminate classes that do not match required quality
	int pnMonsterClasses[ MAX_MONSTER_CLASSES ];
	int nMonsterClassesCount = 0;
	for (int i = 0; i < nMonsterClassesTempCount; ++i)
	{
		int nMonsterClassTemp = pnMonsterClassesTemp[i];
		const UNIT_DATA * pMonsterData = 
			UnitGetData(pGame, GENUS_MONSTER, nMonsterClassTemp);
		if ( pMonsterData != NULL &&
			 UnitDataTestFlag( pMonsterData, UNIT_DATA_FLAG_SPAWN ) &&
			pMonsterData->nMonsterQualityRequired == nMonsterQuality)
		{
			pnMonsterClasses[nMonsterClassesCount++] = nMonsterClassTemp;
		}
	}

	if (nMonsterClassesCount <= 0)
		return 0;

	// pick rooms in the level to do spawning in
	ROOM* pSpawnRooms[MAX_RANDOM_ROOMS];
	int nSpawnRoomsCount;
	if ( pRoom )
	{
		nSpawnRoomsCount = 1;
		pSpawnRooms[0] = pRoom;
	}
	else
	{
		nSpawnRoomsCount = 
			s_LevelGetRandomSpawnRooms( 
				pGame, 
				pLevel, 
				pSpawnRooms, 
				MIN( nMonsterCount, int( arrsize( pSpawnRooms ) ) ) );
		if ( nSpawnRoomsCount <= 0 )
			return 0; 
	}

	// go through any spawnable units to create
	DWORD dwRandomNodeFlags = 0;
	SETBIT( dwRandomNodeFlags, RNF_MUST_ALLOW_SPAWN );
	int nMaxRoomSpawnCount = MAX( ( ( nMonsterCount / nSpawnRoomsCount ) + 1 ), 3 ); 	// Make sure enough will be spawned, but try not to overwhelm rooms. On the other hand, clumps are more fun. I'd rather kill a couple in one spot, instead of wandering around looking for one.
	int nMinRoomSpawnCount = nMaxRoomSpawnCount - 2; 	// Make sure enough will be spawned, but try not to overwhelm rooms. On the other hand, clumps are more fun. I'd rather kill a couple in one spot, instead of wandering around looking for one.
	WARNX( nMaxRoomSpawnCount * nSpawnRoomsCount >= nMonsterCount, "s_LevelSpawnMonsters: nMaxRoomSpawnCount is too small");
	
	int nSpawnRoomIndex = 0;
	for (int i = 0; i < nMonsterCount; ++i) 
	{
		ROOM *pRoom = pSpawnRooms[nSpawnRoomIndex];
		nSpawnRoomIndex = ( ( nSpawnRoomIndex + 1 ) % nSpawnRoomsCount );
		int nRoomSpawnCount = RandGetNum(pGame, nMinRoomSpawnCount, nMaxRoomSpawnCount);
		nMonsterClassIndex = 
			( ( nMonsterClassIndex + 1 ) % nMonsterClassesCount );

		const UNIT_DATA * pMonsterData = 
			UnitGetData(pGame, GENUS_MONSTER, pnMonsterClasses[nMonsterClassIndex]);
		ASSERTX_RETZERO( pMonsterData != NULL, "NULL UNIT_DATA for monster class");
		ASSERTV_RETZERO( UnitDataTestFlag( pMonsterData, UNIT_DATA_FLAG_SPAWN ), "monster \"%s\": cannot be spawned because the Spawn flag needs to be set in monsters.xls", pMonsterData->szName );

		if (pMonsterData->nMonsterQualityRequired != nMonsterQuality)
			continue;

		SPAWN_ENTRY tSpawn;
		tSpawn.eEntryType = SPAWN_ENTRY_TYPE_MONSTER;
		tSpawn.nIndex = pnMonsterClasses[nMonsterClassIndex];
		tSpawn.codeCount = (PCODE)CODE_SPAWN_ENTRY_SINGLE_COUNT;

		for (int j = 0; j < nRoomSpawnCount; ++j)
		{
			int nTries = 32;
			int nSpawnedThisAttemptCount = 0;

			// make sure it spawns
			do
			{
				// pick a random node to spawn at
				int nNodeIndex = s_RoomGetRandomUnoccupiedNode( pGame, pRoom, dwRandomNodeFlags );
				if ( nNodeIndex < 0 )
					continue;
				SPAWN_LOCATION tSpawnLocation;
				SpawnLocationInit( &tSpawnLocation, pRoom, nNodeIndex );

				// spawn monster
				nSpawnedThisAttemptCount = 
					s_RoomSpawnMonsterByMonsterClass( 
						pGame, 
						&tSpawn, 
						nMonsterQuality, 
						pRoom, 
						&tSpawnLocation,
						pfnSpawnedCallback,
						pCallbackData);
			} while (nSpawnedThisAttemptCount < 1 && --nTries > 0);
			WARNX( nSpawnedThisAttemptCount <= 1, "more monsters spawned than expected" );
			nSpawnCount += nSpawnedThisAttemptCount;
			if ( nSpawnCount >= nMonsterCount )
				return nSpawnCount;
		}
	}
#endif
	return nSpawnCount;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void s_sLevelAddAllPlayersToChannel(
	__notnull GAME * pGame,
	__notnull LEVEL * pLevel,
	CHANNELID idChannel )
{
	MSG_SCMD_SET_LEVEL_COMMUNICATION_CHANNEL setChannelMessage;
	setChannelMessage.idGame = GameGetId(pGame);
	setChannelMessage.idLevel = LevelGetID(pLevel);
	setChannelMessage.idChannel = idChannel;

	s_SendMessageToAllInLevel(
		pGame,
		pLevel,
		SCMD_SET_LEVEL_COMMUNICATION_CHANNEL,
		&setChannelMessage );

	for( GAMECLIENT * client = ClientGetFirstInLevel(pLevel);
		 client;
		 client = ClientGetNextInLevel(client,pLevel) )
	{
		UNIT * pUnit = ClientGetPlayerUnitForced(client);
		if( pUnit )
		{
			s_ChatNetAddPlayerToChannel(
				UnitGetGuid(pUnit),
				idChannel);
		}
	}
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void s_LevelClearCommunicationChannel(
	GAME * pGame,
	LEVEL * pLevel )
{
	ASSERT_RETURN(pGame && pLevel);

	pLevel->m_idChatChannel = INVALID_CHANNELID;
	pLevel->m_bChannelRequested = TRUE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL s_LevelSetCommunicationChannel(
	GAME * pGame,
	LEVEL * pLevel,
	CHANNELID idChannel )
{
	ASSERT_RETFALSE(pGame);
	ASSERT_RETFALSE(pLevel);
	ASSERT_RETTRUE(pLevel->m_idChatChannel != idChannel);
	ASSERT_RETFALSE(IS_SERVER(pGame));

	if( pLevel->m_nPlayerCount < 1 )
	{
		return FALSE;
	}

	if(pLevel->m_idChatChannel != INVALID_CHANNELID &&
	   pLevel->m_idChatChannel != idChannel)
	{
		ASSERTX_RETFALSE(
			pLevel->m_idChatChannel == INVALID_CHANNELID,
			"Setting level chat channel when one already exists.");
	}

	pLevel->m_idChatChannel = idChannel;
	pLevel->m_bChannelRequested = FALSE;

	s_sLevelAddAllPlayersToChannel(
		pGame,
		pLevel,
		idChannel );

	return TRUE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void s_LevelAddPlayer(
	LEVEL * level,
	UNIT * unit)
{
	ASSERT_RETURN(level);
	ASSERT_RETURN(unit);
	GAME * game = LevelGetGame(level);
	ASSERT_RETURN(game && IS_SERVER(game));

	level->m_nPlayerCount++;

	//  set that the player has visited area
	if (AppIsTugboat())
	{
		ASSERT_RETURN( UnitGetGenus( unit ) == GENUS_PLAYER );
		MYTHOS_LEVELAREAS::SetPlayerVisitedHub(unit, LevelGetLevelAreaID(level));
	}
	
	//	add them to our channel
	if (level->m_idChatChannel != INVALID_CHANNELID)
	{
		MSG_SCMD_SET_LEVEL_COMMUNICATION_CHANNEL setChannelMessage;
		setChannelMessage.idGame = GameGetId(game);
		setChannelMessage.idLevel = LevelGetID(level);
		setChannelMessage.idChannel = level->m_idChatChannel;

		s_SendMessage(game, UnitGetClientId(unit), SCMD_SET_LEVEL_COMMUNICATION_CHANNEL, &setChannelMessage);

		s_ChatNetAddPlayerToChannel(UnitGetGuid(unit), level->m_idChatChannel);
	}

	//	see if we need a channel
	if (level->m_idChatChannel == INVALID_CHANNELID && !level->m_bChannelRequested)
	{
		s_ChatNetRequestChatChannel(level);
		level->m_bChannelRequested = TRUE;
	}
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void s_LevelRemovePlayer(
	LEVEL * level,
	UNIT * unit)
{
	if (!level)
	{
		return;
	}
	ASSERT_RETURN(unit);
	GAME * game = LevelGetGame(level);
	ASSERT_RETURN(game && IS_SERVER(game));

	if (level->m_idChatChannel != INVALID_CHANNELID)
	{
		MSG_SCMD_SET_LEVEL_COMMUNICATION_CHANNEL setChannelMessage;
		setChannelMessage.idGame = GameGetId(game);
		setChannelMessage.idLevel = LevelGetID(level);
		setChannelMessage.idChannel = INVALID_CHANNELID;
		s_SendMessage(game, UnitGetClientId(unit), SCMD_SET_LEVEL_COMMUNICATION_CHANNEL, &setChannelMessage);
		s_ChatNetRemovePlayerFromChannel(UnitGetGuid(unit), level->m_idChatChannel);
	}

	ASSERT_RETURN(level->m_nPlayerCount > 0);
	level->m_nPlayerCount--;

	if (level->m_nPlayerCount == 0)
	{
		if (level->m_idChatChannel != INVALID_CHANNELID)
		{
			s_ChatNetReleaseChatChannel(level->m_idChatChannel);
			level->m_idChatChannel = INVALID_CHANNELID;
		}
	}
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int s_LevelGetPossibleRandomMonsters( 
	LEVEL* pLevel, 
	int *pnMonsterClasses,
	int nBufferSize,
	int nCurrentBufferCount,
	BOOL bUseLevelSpawnClassMemory)
{
	ASSERT( pLevel && pnMonsterClasses );
	
	GAME *pGame = LevelGetGame( pLevel );
	for (int i = 0; i < MAX_LEVEL_DISTRICTS; ++i)
	{
		int nSpawnClass = pLevel->m_nDistrictSpawnClass[ i ];
		s_SpawnClassGetPossibleMonsters( 
			pGame, 
			pLevel, 
			nSpawnClass, 
			pnMonsterClasses, 
			nBufferSize, 
			&nCurrentBufferCount,
			bUseLevelSpawnClassMemory);
	}

	return nCurrentBufferCount;
	
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
#define MIN_ROOM_INDEX_BITS		12
#define MIN_MONSTER_INDEX_BITS	16
#define ROOM_DISTANCE_DIVISOR   2.0f

#if !ISVERSION(CLIENT_ONLY_VERSION)
void s_LevelSendToClient( 
	GAME * pGame, 
	UNIT *pPlayer,
	GAMECLIENTID idClient,
	LEVELID idLevel)
{
	GAMECLIENT * pClient = ClientGetById( pGame, idClient );
	ASSERT_RETURN( pClient );	
	LEVEL* pLevel = LevelGetByID( pGame, idLevel );

	MSG_SCMD_SET_LEVEL msg;
	msg.idLevel = idLevel;
	msg.dwUID = pLevel->m_dwUID;
	msg.nLevelDefinition = LevelGetDefinitionIndex( pLevel );
	msg.nDRLGDefinition = DungeonGetLevelDRLG( pGame, pLevel );
	msg.nEnvDefinition = s_LevelGetEnvironmentIndex( pGame, pLevel );
	msg.nMusicRef = DungeonGetLevelMusicRef( pGame, pLevel );
	msg.nSpawnClass = DungeonGetLevelSpawnClass( pGame, pLevel );
	msg.nNamedMonsterClass = DungeonGetLevelNamedMonsterClass( pGame, pLevel );
	msg.fNamedMonsterChance = DungeonGetLevelNamedMonsterChance( pGame, pLevel );
	msg.vBoundingBoxMin = pLevel->m_BoundingBox.vMin;
	msg.vBoundingBoxMax = pLevel->m_BoundingBox.vMax;
	msg.nDepth = pLevel->m_nDepth;
	msg.nArea = LevelGetLevelAreaID( pLevel );	
	msg.CellWidth = pLevel->m_CellWidth;
	msg.nAreaDepth = INVALID_LINK;
	msg.dwPVPWorld = pLevel->m_bPVPWorld ? 1 : 0;
	
	if (AppIsTugboat())
	{
		msg.nAreaDepth = MYTHOS_LEVELAREAS::LevelAreaGetMaxDepth( msg.nArea );
		msg.nAreaDifficulty = LevelGetDifficulty( pLevel ); 
	}

	MemoryCopy(msg.nThemes, MAX_THEMES_PER_LEVEL_DESC * sizeof(DWORD), pLevel->m_pnThemes, MAX_THEMES_PER_LEVEL * sizeof(DWORD));
	
	ZeroMemory( &msg.tBuffer, sizeof( msg.tBuffer ) );
	BIT_BUF tBuffer( msg.tBuffer, MAX_SET_LEVEL_MSG_BUFFER );

	// init sublevels
	for (int i = 0; i < MAX_SUBLEVELS; ++i)
	{
		SUBLEVEL *pSubLevel = &pLevel->m_SubLevels[ i ];
		SUBLEVEL_DESC *pSubLevelDesc = &msg.tSubLevels[ i ];
		
		pSubLevelDesc->idSubLevel = pSubLevel->idSubLevel;
		pSubLevelDesc->nSubLevelDef = pSubLevel->nSubLevelDef;
		pSubLevelDesc->vPosition = pSubLevel->vPosition;		
		
	}
	
	// send which rooms are being used in the level
	ASSERT_RETURN( ExcelGetNumRows( pGame, DATATABLE_ROOM_INDEX ) < (1 << MIN_ROOM_INDEX_BITS) );
	ROOM * pRoom = pLevel->m_pRooms;
	ASSERT_RETURN( pRoom );
		
	VECTOR vStartLocation;
	VECTOR vStartDirection;

	DWORD dwWarpFlags = 0;
	SETBIT( dwWarpFlags, WF_ARRIVE_AT_START_LOCATION_BIT );
	
	LevelGetStartPosition(
		 pGame,
		 ((AppIsTugboat())?pClient->m_pControlUnit:NULL), //this should be ok for hellgate but...
		 idLevel, 
		 vStartLocation, 
		 vStartDirection, 
		 dwWarpFlags);

	// find the min 
	int nRoomIndexMin = pRoom->pDefinition->nRoomExcelIndex;
	int nRoomIndexMax  = pRoom->pDefinition->nRoomExcelIndex;
	while ( pRoom )
	{
		if ( pRoom->pDefinition->nRoomExcelIndex < nRoomIndexMin )
		{
			nRoomIndexMin = pRoom->pDefinition->nRoomExcelIndex;
		}
		if ( pRoom->pDefinition->nRoomExcelIndex > nRoomIndexMax )
		{
			nRoomIndexMax  = pRoom->pDefinition->nRoomExcelIndex;
		}
		pRoom = pRoom->pNextRoomInLevel;
	}
	ASSERT_RETURN( nRoomIndexMax - nRoomIndexMin <= (1 << MIN_ROOM_INDEX_BITS) );
	DWORD pdwRoomIndexMask[ (1 << MIN_ROOM_INDEX_BITS) / 32 ];
	ZeroMemory( pdwRoomIndexMask, sizeof( pdwRoomIndexMask ) );
	BYTE pbyRoomDistance[ (1 << MIN_ROOM_INDEX_BITS) ];
	ZeroMemory( pbyRoomDistance, sizeof( pbyRoomDistance ) );
	pRoom = pLevel->m_pRooms;
	while ( pRoom )
	{
		float fDistance = ConvexHullManhattenDistance( pRoom->pHull, & vStartLocation );
		BYTE byDistance = min( 128, (BYTE) (fDistance / ROOM_DISTANCE_DIVISOR) );
		BYTE * pbyDist = & pbyRoomDistance[ pRoom->pDefinition->nRoomExcelIndex - nRoomIndexMin ];

		if ( ! TESTBIT( pdwRoomIndexMask, pRoom->pDefinition->nRoomExcelIndex - nRoomIndexMin) || 
			byDistance < *pbyDist )
			*pbyDist = byDistance;

		SETBIT( pdwRoomIndexMask, pRoom->pDefinition->nRoomExcelIndex - nRoomIndexMin, TRUE );

		pRoom = pRoom->pNextRoomInLevel;
	}

	tBuffer.PushUInt( nRoomIndexMin, MIN_ROOM_INDEX_BITS );
	int nRoomIndexDelta = nRoomIndexMax - nRoomIndexMin + 1;
	ASSERT_RETURN( nRoomIndexDelta );
	tBuffer.PushUInt( nRoomIndexDelta, MIN_ROOM_INDEX_BITS );
	//tBuffer.PushBuf((BYTE*)pdwRoomIndexMask, nRoomIndexDelta);
	//*
	for ( int i = 0; i < nRoomIndexDelta / 32; i++ )
		tBuffer.PushUInt( pdwRoomIndexMask[ i ], 32 );
	if (nRoomIndexDelta % 32)
	{
		tBuffer.PushUInt( pdwRoomIndexMask[ nRoomIndexDelta / 32 ], nRoomIndexDelta % 32 );
	}
	// */

	for ( int i = 0; i < nRoomIndexDelta; i++ )
	{
		if ( TESTBIT( pdwRoomIndexMask, i ) )
		{
			tBuffer.PushUInt( pbyRoomDistance[ i ], 8 );
		}
	}

	// send which monsters can be used in this level so that clients can preload the gfx
	const LEVEL_POSSIBLE_UNITS *pPossibleMonsters = &pLevel->m_tPossibleUnits[ GENUS_MONSTER ];	
	int nMonsterClassCount = pPossibleMonsters->nCount;
	if( AppIsTugboat() && pLevel->m_pLevelDefinition->eSrvLevelType == SRVLEVELTYPE_OPENWORLD )
	{
		nMonsterClassCount = 0;
	}
	ASSERTX_RETURN( nMonsterClassCount <= (1 << MIN_MONSTER_INDEX_BITS), "Too many possible monsters" );
	tBuffer.PushUInt( nMonsterClassCount, MIN_MONSTER_INDEX_BITS );	
	for (int i = 0; i < nMonsterClassCount; ++i)
	{
		int nClass = pPossibleMonsters->nClass[ i ];
#ifdef _DEBUG
		const UNIT_DATA *pUnitData = MonsterGetData( NULL, nClass );
		trace( "Preloading '%s' for client '%I64d\n", pUnitData->szName );
#endif		
		ASSERTV( nClass <= (1 << MIN_MONSTER_INDEX_BITS), "Possible monster class is too high - [ class %d, bits %d ]", nClass, MIN_MONSTER_INDEX_BITS );
		tBuffer.PushUInt( nClass, MIN_MONSTER_INDEX_BITS );
	}

	MSG_SET_BBLOB_LEN(msg, tBuffer, tBuffer.GetLen());
	s_SendMessage(pGame, idClient, SCMD_SET_LEVEL, &msg);

	// for now we go through all rooms in the level and send them, we do this because
	// it has been deemed unnacceptable to load the rooms one at a time as the clients
	// need them because of gfx loading is too slow currently, when we have that fixed
	// we can remove this
	if (gbClientsHaveVisualRoomsForEntireLevel && !pLevel->m_pLevelDefinition->bClientDiscardRooms)
	{
		for (pRoom = LevelGetFirstRoom( pLevel ); pRoom; pRoom = LevelGetNextRoom( pRoom ))
		{
			s_RoomSendToClient( pRoom, pClient, FALSE );
		}
	}	

	if( AppIsTugboat() )
	{
		LevelGetAnchorMarkers( pLevel )->SendAnchorsToClient( pGame, idClient, idLevel );		

	}
}
#endif

//-------------------------------------------------------------------------------------------------
// Resend all state information as if they know nothing.  (For reconnection)
//-------------------------------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void s_LevelResendToClient( 
	GAME * pGame, 
	LEVEL *pLevel,
	GAMECLIENT *pClient,
	UNIT *pUnit)
{
	ASSERT_RETURN(pGame && pLevel && pClient && pUnit);
	ASSERT_RETURN(ClientGetPlayerUnitForced(pClient) == pUnit);

	GAMECLIENTID idClient = ClientGetId(pClient);

	s_LevelSendStartLoad( 
		pGame,
		idClient,
		pLevel->m_nLevelDef, 
		LevelGetDepth(pLevel), 
		LevelGetLevelAreaID( pLevel ),
		pLevel->m_nDRLGDef);

	s_LevelSendToClient(pGame, pUnit, idClient, LevelGetID(pLevel));
	ClientClearAllRoomsKnown(pClient, pLevel);
	ClientRemoveAllKnownUnits(pGame, pClient);
	RoomUpdateClient(pUnit);
	//s_SendUnitToClient(pUnit, pClient);

	MSG_SCMD_LOADCOMPLETE tMessageComplete;
	tMessageComplete.nLevelDefinition = pLevel->m_nLevelDef;
	tMessageComplete.nDRLGDefinition = pLevel->m_nDRLGDef;	
	tMessageComplete.nLevelId = LevelGetID( pLevel );	
	s_SendMessage( pGame, ClientGetId( pClient ), SCMD_LOADCOMPLETE, &tMessageComplete );
}
#endif
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void sRemoveAllRoomModelDefs()
{
	// get rid of all the room models
	int nRoomIndexCount = ExcelGetNumRows( NULL, DATATABLE_ROOM_INDEX );
	for ( int i = 0; i < nRoomIndexCount; i++ )
	{
		ROOM_INDEX * pRoomIndex = (ROOM_INDEX *) ExcelGetData( NULL, DATATABLE_ROOM_INDEX, i );
		if ( ! pRoomIndex )
			continue;
		if ( ! pRoomIndex->pRoomDefinition )
			continue;

		for ( int nLOD = 0; nLOD < LOD_COUNT; nLOD++ )
		{
			e_ModelDefinitionDestroy( pRoomIndex->pRoomDefinition->nModelDefinitionId, 
				nLOD, MODEL_DEFINITION_DESTROY_ALL, TRUE );
		}
		pRoomIndex->pRoomDefinition->nModelDefinitionId = INVALID_ID;
	}
}

//----------------------------------------------------------------------------
static int sgnLevelDefinition = INVALID_LINK;
static int sgnDRLGDefinition = INVALID_LINK;
static int sgnLevelDepth = 0;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDisplayLevelChange(
	void)
{		
	
	const LEVEL_DEFINITION * pLevelDefinition = LevelDefinitionGet( sgnLevelDefinition );
	ASSERT( pLevelDefinition );
	if ( AppIsTugboat() )
	{
		TCHAR szBuf[512];
		if( sgnLevelDepth == 0 )
		{
			PStrPrintf(szBuf, 512, "%s", pLevelDefinition->pszDisplayName );
		}
		else
		{
			PStrPrintf(szBuf, 512, "Level %d - %s", sgnLevelDepth, pLevelDefinition->pszDisplayName );
		}

		UIShowQuickMessage( szBuf, LEVELCHANGE_FADEOUTMULT_DEFAULT, szBuf );
	}
	else
	{
		const WCHAR *puszLevelName = StringTableGetStringByIndex( pLevelDefinition->nLevelDisplayNameStringIndex );
		const DRLG_DEFINITION * pDRLGDefinition = DRLGDefinitionGet( sgnDRLGDefinition );
		ASSERT_RETURN( pDRLGDefinition );
		if ( pDRLGDefinition->nDRLGDisplayNameStringIndex != INVALID_LINK )
		{
			const WCHAR *puszDRLGName = StringTableGetStringByIndex( pDRLGDefinition->nDRLGDisplayNameStringIndex );
			UIShowQuickMessage( puszLevelName, LEVELCHANGE_FADEOUTMULT_DEFAULT, puszDRLGName );
		}
		else
		{
			UIShowQuickMessage( puszLevelName, LEVELCHANGE_FADEOUTMULT_DEFAULT );
		}
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
#define LEVEL_READY_COUNT 100
#define LEVEL_READY_COUNT_MYTHOS 200

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
static BOOL sLevelLoadedFinalizeCheckMovie(
	GAME * game, 
	UNIT * unit, 
	const EVENT_DATA & event_data)
{
	int nMovie = (int)event_data.m_Data1;

	if(AppGetState() == APP_STATE_PLAYMOVIELIST)
	{
		GameEventRegister(game, sLevelLoadedFinalizeCheckMovie, NULL, NULL, EVENT_DATA(nMovie), 1);
		return TRUE;
	}

	// tell the server that we're totally loaded now
	MSG_CCMD_LEVELLOADED msg;
	c_SendMessage(CCMD_LEVELLOADED, &msg);

	UNIT * pControlUnit = GameGetControlUnit(game);

	// triger preloaded event
	UnitTriggerEvent(game, EVENT_PRELOADED, pControlUnit, NULL, NULL);

	//TRAVIS: Tugboat console is also in the chat window - this clears chat when going thru levels.
	if( AppIsHellgate() )
	{
		ConsoleClear();
	}

	return TRUE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static BOOL sLevelLoadedFinalizePlayMovie(
	GAME * game, 
	UNIT * unit, 
	const EVENT_DATA & event_data)
{
	int nMovie = (int)event_data.m_Data1;
	AppSwitchState(APP_STATE_PLAYMOVIELIST, nMovie);
	GameEventRegister(game, sLevelLoadedFinalizeCheckMovie, NULL, NULL, EVENT_DATA(nMovie), 1);
	return TRUE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void sLevelLoadedFinalize( GAME * pGame, UNIT * pControlUnit )
{
	//Uber scary and messy and ugly--this is getting hit on character
	//select, which means instant network error.
	ASSERT_RETURN(AppGetState() != APP_STATE_CHARACTER_CREATE);
	AppSwitchState(APP_STATE_IN_GAME);

	pGame->eGameState = GAMESTATE_RUNNING;

	int nMovie = UnitGetStat(pControlUnit, STATS_MOVIE_TO_PLAY_AFTER_LEVEL_LOAD);

	if(nMovie >= 0)
	{
		UIHideLoadingScreen();
		GameEventRegister(pGame, sLevelLoadedFinalizePlayMovie, NULL, NULL, EVENT_DATA(nMovie), 1);
	}
	else
	{
		sLevelLoadedFinalizeCheckMovie(pGame, pControlUnit, EVENT_DATA(nMovie));
	}
	if( AppIsTugboat() )
	{
		UIHideLoadingScreen();
	}
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static PRESULT sDummyFileLevelLoadedCallback(
	ASYNC_DATA & data)
{
	PAKFILE_LOAD_SPEC * spec = (PAKFILE_LOAD_SPEC *)data.m_UserData;
	ASSERT_RETINVALIDARG(spec);

	GAME * pGame = AppGetCltGame();
	WAIT_FOR_LOAD_CALLBACK_DATA *pCallbackData = (WAIT_FOR_LOAD_CALLBACK_DATA *)spec->callbackData;
	ASSERT_RETFAIL(pCallbackData);
	


	UNIT * pControlUnit = pGame ? GameGetControlUnit(pGame) : NULL;
	int nWardrobe = pControlUnit ? UnitGetWardrobe( pControlUnit ) : INVALID_ID;

	int nLevelReadyCount = LEVEL_READY_COUNT;
	// I'm not sure if Hellgate actually wants to try this -
	// waiting until the operations hit a basically idle state - no files requested or pending -
	// then we'll allow the counter to increase, to try to make double-sure that the level is actually loaded.
//	if( AppIsHellgate() || ( nOperations < 2 && nFinishedOperations < 2 ) )
//	{
		pCallbackData->nCount++;
//	}
#if ISVERSION( CLIENT_ONLY_VERSION )
	//else
	//{
	//	pCallbackData->nCount = 0;
	//}
	if( AppIsTugboat() )
	{
		nLevelReadyCount = LEVEL_READY_COUNT_MYTHOS;
	}
#endif

	if (pControlUnit && 
		pCallbackData->nCount >= nLevelReadyCount && 
		c_WardrobeIsUpdated( nWardrobe ) )
	{	
		sLevelLoadedFinalize( pGame, pControlUnit );
	}
	else
	{
		DECLARE_LOAD_SPEC(newspec, "");
		newspec.priority = ASYNC_PRIORITY_LOAD_READY;
		newspec.fpLoadingThreadCallback = sDummyFileLevelLoadedCallback;
		newspec.flags |= PAKFILE_LOAD_FREE_CALLBACKDATA | PAKFILE_LOAD_DUMMY_CALLBACK;
		CLEAR_MASK(spec->flags, PAKFILE_LOAD_FREE_CALLBACKDATA);
		newspec.callbackData = pCallbackData;  // keep callback data around
		PakFileLoad(newspec);
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_LevelWaitForLoaded(
	GAME *pGame,
	int nLevelDefinition,
	int nDRLGDefinition,
	ASYNC_FILE_CALLBACK pOverrideCallback /*=NULL*/ )
{
	WAIT_FOR_LOAD_CALLBACK_DATA *pCallbackData = (WAIT_FOR_LOAD_CALLBACK_DATA *)MALLOCZ( NULL, sizeof( WAIT_FOR_LOAD_CALLBACK_DATA ));
	pCallbackData->nCount = 0;
	pCallbackData->nLevelDefinition = nLevelDefinition;
	pCallbackData->nDRLGDefinition = nDRLGDefinition;
	
	TIMER_START("c_LevelWaitForLoaded -- Waiting for Load to Finish");
	DECLARE_LOAD_SPEC( spec, "" );
	spec.pool = NULL;
	spec.priority = ASYNC_PRIORITY_LOAD_READY;
	spec.flags |= PAKFILE_LOAD_FREE_CALLBACKDATA | PAKFILE_LOAD_DUMMY_CALLBACK;
	spec.fpLoadingThreadCallback = pOverrideCallback ? pOverrideCallback : sDummyFileLevelLoadedCallback;
	spec.callbackData = pCallbackData;
	PakFileLoad(spec);
}
#endif //!ISVERSION(SERVER_VERSION)

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static BOOL sLevelSetStateOnClients(
	GAME * pGame,
	UNIT * ,
	const EVENT_DATA& event_data)
{
	LEVEL * pLevel = LevelGetByID( pGame, (LEVELID)event_data.m_Data1 );
	if ( ! pLevel )
		return FALSE;

	const DRLG_DEFINITION * pDrlgDefinition = LevelGetDRLGDefinition( pLevel );
	ASSERT_RETFALSE( pDrlgDefinition );

	if ( pDrlgDefinition->nRandomState == INVALID_ID )
		return FALSE;

	for (GAMECLIENT *pClient = ClientGetFirstInGame( pGame ); 
		pClient; 
		pClient = ClientGetNextInGame( pClient ))	
	{
		UNIT *pControlUnit = ClientGetControlUnit( pClient );
		if (pControlUnit)
		{
			if (UnitGetLevel( pControlUnit ) == pLevel && !UnitTestFlag( pControlUnit, UNITFLAG_IN_LIMBO ))
			{
				s_StateSet( pControlUnit, NULL, pDrlgDefinition->nRandomState, pDrlgDefinition->nRandomStateDuration );
			}
		}
	}
	
	s_LevelStartEvents( pGame, pLevel );
	return TRUE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void s_LevelStartEvents( 
	GAME * pGame,
	LEVEL * pLevel )
{
	const DRLG_DEFINITION * pDrlgDefinition = LevelGetDRLGDefinition( pLevel );
	ASSERT_RETURN( pDrlgDefinition );

	if ( pDrlgDefinition->nRandomState == INVALID_ID )
		return;

	ASSERT_RETURN( pDrlgDefinition->nRandomStateRateMin && pDrlgDefinition->nRandomStateRateMax );
	ASSERT_RETURN( pDrlgDefinition->nRandomStateRateMin <= pDrlgDefinition->nRandomStateRateMax );
	int nTicksUntilState = RandGetNum( pGame, pDrlgDefinition->nRandomStateRateMin, pDrlgDefinition->nRandomStateRateMax );

	GameEventRegister( pGame, sLevelSetStateOnClients, NULL, NULL, EVENT_DATA( pLevel->m_idLevel ), nTicksUntilState );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSetSubLevelRegion(
	LEVEL *pLevel,
	SUBLEVEL *pSubLevel,
	void *pCallbackData)
{
	const SUBLEVEL_DEFINITION *ptSubLevelDef = SubLevelGetDefinition( pSubLevel );

	// add region to engine
	V( e_RegionAdd( pSubLevel->nEngineRegion ) );
	
	// add offset
	e_RegionSetOffset( pSubLevel->nEngineRegion, &pSubLevel->vPosition );
	
	// set drlg definition (if specified)
	if (ptSubLevelDef->nDRLGDef	!= INVALID_LINK)
	{
		GAME *pGame = LevelGetGame( pLevel );
		int nEnvironment = DungeonGetDRLGBaseEnvironment( ptSubLevelDef->nDRLGDef );
		c_SetEnvironmentDef( pGame, pSubLevel->nEngineRegion, nEnvironment, "", TRUE );
	}
				
}

#if !ISVERSION(SERVER_VERSION)
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static int c_sLevelSelectThemeWithHighestEnvironmentPriority(
	GAME * pGame,
	int * pnThemes)
{
	ASSERT_RETINVALID(pnThemes);

	PickerStart(pGame, picker);
	int nMaxPriority = -1;
	for(int i=0; i<MAX_THEMES_PER_LEVEL; i++)
	{
		if(pnThemes[i] < 0)
		{
			continue;
		}

		const LEVEL_THEME * pLevelTheme = (const LEVEL_THEME *)ExcelGetData(NULL, DATATABLE_LEVEL_THEMES, pnThemes[i]);
		ASSERT_CONTINUE(pLevelTheme);

		if(pLevelTheme->pszEnvironment[0] != '\0')
		{
			// Reset the picker and the max priority if we've found a more important theme
			if(pLevelTheme->nEnvironmentPriority > nMaxPriority)
			{
				PickerReset(PickerGetCurrent(pGame), FALSE);
				nMaxPriority = pLevelTheme->nEnvironmentPriority;
			}
			
			// If this is the same priority as the existing environment, then add it to the picker
			if(pLevelTheme->nEnvironmentPriority == nMaxPriority)
			{
				PickerAdd(MEMORY_FUNC_FILELINE(pGame, i, 1));
			}
		}
	}
	return PickerChoose(pGame);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_LevelSetAndLoad( 
	GAME *pGame, 
	MSG_SCMD_SET_LEVEL* msg )
{
	TIMER_START( "c_LevelSetAndLoad()" );

	const GLOBAL_DEFINITION* global_definition = DefinitionGetGlobal();
	ASSERT_RETURN(global_definition);
	
	UNIT * pControlUnit = GameGetControlUnit(pGame);

	if ( AppGetCurrentLevel() != (LEVELID)msg->idLevel )
	{
		c_SaveAutomap(pGame, AppGetCurrentLevel());

		if ( pControlUnit )
		{ // get the control unit out of the level before it is freed
			UnitRemoveFromWorld( pControlUnit, FALSE );
		}

#if !ISVERSION(SERVER_VERSION) && !defined(TUGBOAT)
		if(AppIsHellgate() && !GameGetGameFlag(pGame, GAMEFLAG_DISABLE_ADS))
		{
			if(AppGetCurrentLevel() >= 0)
			{
				LEVEL * pOldLevel = LevelGetByID(pGame, AppGetCurrentLevel());
				if(pOldLevel)
				{
					c_AdClientExitLevel(pGame, pOldLevel->m_nLevelDef);
				}
			}
		}
#endif

		LevelFree( pGame, AppGetCurrentLevel() );

		UnitFreeListFree( pGame );

		c_ClearVisibleRooms();

		sRemoveAllRoomModelDefs();

		c_SoundStopAll();
		c_SoundSetFlagPlayedSounds(TRUE);

		V( e_EnvironmentReleaseAll() );

		if ( 0 == ( global_definition->dwFlags & GLOBAL_FLAG_NO_CLEANUP_BETWEEN_LEVELS ) )
		{
			V( e_Cleanup( FALSE, TRUE ) );

#if !ISVERSION( SERVER_VERSION ) && ISVERSION( DEVELOPMENT )
			if( AppIsHellgate() )
			{
				LogMessage(PERF_LOG, "LEVEL CHANGE" );
				MEMORYSTATUSEX memstat;
				memstat.dwLength = sizeof( MEMORYSTATUSEX );
				GlobalMemoryStatusEx(&memstat);
				LogMessage( PERF_LOG, "AvailVirtual: %llu", memstat.ullAvailVirtual );
				LogMessage( PERF_LOG, "AvailExtendedVirtual: %llu", memstat.ullAvailExtendedVirtual );
				LogMessage( PERF_LOG, "AvailPhys: %llu", memstat.ullAvailPhys );
				LogMessage( PERF_LOG, "AvailPageFile: %llu", memstat.ullAvailPageFile );

				MemoryTraceAllFLIDX();
				MemoryDumpCRTStats();
				e_EngineMemoryDump( TRUE, NULL );
				e_ModelDumpList();
				e_TextureDumpList();
			}			
#endif
		}

		V( e_ReaperReset() );
		V( e_OptimizeManagedResources() );
	}

	BOUNDING_BOX tBoundingBox;
	tBoundingBox.vMin = msg->vBoundingBoxMin;
	tBoundingBox.vMax = msg->vBoundingBoxMax;

	AppSetCurrentLevel( msg->idLevel );

#if !ISVERSION(SERVER_VERSION) && !defined(TUGBOAT)
	if(AppIsHellgate() && !GameGetGameFlag(pGame, GAMEFLAG_DISABLE_ADS))
	{
		if(AppGetCurrentLevel() >= 0)
		{
			c_AdClientEnterLevel(pGame, msg->nLevelDefinition);
		}
	}
#endif

	// must be called before LevelAdd()
	DungeonUpdateOnSetLevelMsg( pGame, msg );

	// fill out common level spec stuff
	LEVEL_SPEC tSpec;
	tSpec.idLevel = msg->idLevel;
	tSpec.tLevelType.nLevelDef = msg->nLevelDefinition;
	tSpec.tLevelType.bPVPWorld = msg->dwPVPWorld != 0;
	tSpec.nDRLGDef = msg->nDRLGDefinition;
	tSpec.dwSeed = global_definition->dwSeed;
	tSpec.pBoundingBox = &tBoundingBox;
	tSpec.bPopulateLevel = TRUE;	
	
	LEVEL * pLevel = NULL;
	if ( AppIsHellgate() )
	{
		// add level
		pLevel = LevelAdd( pGame, tSpec );
	}
	else
	{
	
		// fill out tugboat specific information
		tSpec.tLevelType.nLevelArea = msg->nArea;
		tSpec.tLevelType.nLevelDepth = msg->nDepth;		
		tSpec.nDifficultyOffset = msg->nAreaDifficulty;
		tSpec.flCellWidth = msg->CellWidth;

		// add level 
		pLevel = LevelAdd( pGame, tSpec );
		
	}
	ASSERT_RETURN( pLevel );
	
	// apply sublevels to level
	SubLevelClearAll( pLevel );
	for (int i = 0; i < MAX_SUBLEVELS; ++i)
	{
		const SUBLEVEL_DESC *pSubLevelDesc = &msg->tSubLevels[ i ];
		if (pSubLevelDesc->idSubLevel != INVALID_ID)
		{
			SubLevelAdd( pLevel, pSubLevelDesc );
		}
	}
			
	pLevel->m_dwUID = msg->dwUID;

#ifdef HAVOK_ENABLED
	if ( AppIsHellgate() )
	{
		if (pLevel->m_pHavokWorld)
		{
			HavokCloseWorld( &pLevel->m_pHavokWorld );
		}
		const DRLG_DEFINITION *ptDRLGDefinition = LevelGetDRLGDefinition( pLevel );
		HavokInitWorld( &pLevel->m_pHavokWorld, IS_CLIENT(pGame), ptDRLGDefinition ? ptDRLGDefinition->bCanUseHavokFX : FALSE, &tBoundingBox, TRUE );
#ifdef _DEBUG
		c_HavokSetupVisualDebugger( pLevel->m_pHavokWorld );
#endif // _DEBUG
	}
#endif // HAVOK_ENABLED
	{
		TIMER_START( "cLevelInit()" );
		int nSelectedTheme = c_sLevelSelectThemeWithHighestEnvironmentPriority(pGame, (int*)msg->nThemes);
		c_LevelInit( pGame, msg->nLevelDefinition, msg->nEnvDefinition, ((nSelectedTheme >= 0) ? msg->nThemes[nSelectedTheme] : INVALID_ID) );
	}

	{
		BIT_BUF tBuffer( msg->tBuffer, MAX_SET_LEVEL_MSG_BUFFER );

		UINT nRoomIndexMin;
		tBuffer.PopUInt( &nRoomIndexMin, MIN_ROOM_INDEX_BITS );
		UINT nRoomIndexDelta;
		tBuffer.PopUInt( &nRoomIndexDelta, MIN_ROOM_INDEX_BITS );

		DWORD pdwRoomIndexMask[ (1 << MIN_ROOM_INDEX_BITS) / 32 ];
		//tBuffer.PopBuf((BYTE*)pdwRoomIndexMask, nRoomIndexDelta);
		//*
		for ( UINT i = 0; i < nRoomIndexDelta / 32; i++ )
			tBuffer.PopUInt( (UINT *)&pdwRoomIndexMask[ i ], 32 );
		if (nRoomIndexDelta % 32)
		{
			tBuffer.PopUInt( (UINT *)&pdwRoomIndexMask[ nRoomIndexDelta / 32 ], nRoomIndexDelta % 32 );
		}
		// */

		BYTE pbyRoomDistance[ (1 << MIN_ROOM_INDEX_BITS) ];
		ZeroMemory( pbyRoomDistance, sizeof( pbyRoomDistance ) );

		for ( UINT i = 0; i < nRoomIndexDelta; i++ )
		{
			if ( TESTBIT( pdwRoomIndexMask, i ) )
			{
				UINT nDistance;
				tBuffer.PopUInt( &nDistance, 8 );
				pbyRoomDistance[ i ] = (BYTE) nDistance;
			}
		}

		{
			TIMER_START( "Loading Rooms" )

			ROOM_INDEX_SORT pRoomIndexSort[ (1 << MIN_ROOM_INDEX_BITS) ];
			int nRoomIndexCount = 0;
			
			for ( UINT i = 0; i < nRoomIndexDelta; i++ )
			{
				if ( TESTBIT( pdwRoomIndexMask, i ) )
				{
					pRoomIndexSort[ nRoomIndexCount ].nIndex = nRoomIndexMin + i;
					pRoomIndexSort[ nRoomIndexCount ].bDistance = pbyRoomDistance[ i ];
					nRoomIndexCount++;
					ASSERT( nRoomIndexCount < (1 << MIN_ROOM_INDEX_BITS) );
				}
			}

			qsort( pRoomIndexSort, nRoomIndexCount, sizeof( ROOM_INDEX_SORT ), sCompareRoomIndexSort );
			if ( AppIsHellgate() )
			{
				for ( int i = 0; i < nRoomIndexCount; i++ )
				{
					float fDistance = pRoomIndexSort[ i ].bDistance * ROOM_DISTANCE_DIVISOR;
					ROOM_DEFINITION * pRoomDefinition = RoomDefinitionGet( pGame, pRoomIndexSort[ i ].nIndex, TRUE, fDistance ); // this loads the graphics
					if ( pRoomDefinition )
						pRoomDefinition->fInitDistanceToCamera = fDistance;
				}
			}
		}
		
		{
			TIMER_START( "Loading Monsters" )
			
#if defined(_DEBUG) || ISVERSION(DEVELOPMENT) || defined(USERNAME_cbatson)	// CHB 2006.09.27 - Allow no monsters on release build.
			GLOBAL_DEFINITION * pGlobals = DefinitionGetGlobal();
			if ( ( pGlobals->dwFlags & GLOBAL_FLAG_NOMONSTERS ) == 0 )
#endif
			{
				UINT nMonsterClassCount;;
				tBuffer.PopUInt( &nMonsterClassCount, MIN_MONSTER_INDEX_BITS );
				for ( UINT i = 0; i < nMonsterClassCount; i++ )
				{
					UINT nMonsterClass;
					tBuffer.PopUInt( &nMonsterClass, MIN_MONSTER_INDEX_BITS );
					UnitDataLoad( pGame, DATATABLE_MONSTERS, nMonsterClass );
					c_UnitDataFlagSoundsForLoad(pGame, DATATABLE_MONSTERS, nMonsterClass, FALSE);
				}
			}
		
		}
		
	}

	if(pControlUnit)
	{
		c_UnitFlagSoundsForLoad(pControlUnit);
		c_PlayerClearMusicInfo(pControlUnit);
	}

	c_CombatSystemFlagSoundsForLoad(pGame);
	c_StatesFlagSoundsForLoad(pGame);
	c_UnitDataAlwaysKnownFlagSoundsForLoad(pGame);

	// TRAVIS: FIXME - NO SEPARATE REGIONS FOR US RIGHT NOW - BUT MAYBE LATER! POCKET DUNGEONS!
	if ( AppIsHellgate() )
	{
		// set the regions
		SubLevelsIterate( pLevel, sSetSubLevelRegion, NULL );	
	}

	// CML 2007.09.14 - Moved to after the sublevel iteration so that the sublevel automaps have been created
	//   before the load call tries to populate them.
	// load automap info
	c_LoadAutomap(pGame, pLevel->m_idLevel);

	c_SoundUnloadAtLevelChange();
	c_SoundSetFlagPlayedSounds(FALSE);

	c_UnitDataAllUnflagSoundsForLoad(pGame);

	int nMusicRef = DungeonGetLevelMusicRef(pGame, pLevel);
	if(nMusicRef >= 0)
	{
		c_MusicPlay(nMusicRef);
	}

//	c_LevelWaitForLoaded( pGame, msg->nLevelDefinition, msg->nDRLGDefinition );

}
#endif //!ISVERSION(SERVER_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sLevelLinkInit( 
	LEVEL_LINK *pLink, 
	UNIT *pUnit,
	LEVELID idLevelDest)
{
	ASSERTX_RETURN( pLink, "Expected level link" );
	ASSERTX_RETURN( pUnit, "Expected unit" );
	
	pLink->idUnit = UnitGetId( pUnit );
	pLink->idLevel = idLevelDest;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sAddLevelLink(
	LEVEL *pLevel,
	UNIT *pUnit,
	LEVELID idLevelLink)
{				
	ASSERTX_RETFALSE( pLevel, "Expected level" );
	ASSERTX_RETFALSE( pUnit, "Expected unit" );

	GAME * game = LevelGetGame(pLevel);
	ASSERT_RETFALSE(game);

	// verify there isn't a link to this level already
	if( AppIsHellgate() )
	{
		for (int i = 0; i < pLevel->m_nNumLevelLinks; ++i)
		{
			const LEVEL_LINK *pLink = &pLevel->m_LevelLink[ i ];
			if (pLink->idLevel == idLevelLink)
			{
				// however if there is a link the dest level already from a different unit
				// it's gonna cause problems
				if( !( pLink->idLevel == INVALID_ID &&
					   ObjectWarpAllowsInvalidDestination( pUnit ) ) )
				{
					ASSERTV_RETFALSE( 
						pLink->idUnit == UnitGetId( pUnit ), 
						"sAddLevelLink: Trying to link to level '%s' via unit '%s' [id=%d], but link already exists from a different unit '%s' [id=%d]",
						LevelGetDevName( pLevel ),
						UnitGetDevName( pUnit ),
						UnitGetId( pUnit ),
						UnitGetDevName( UnitGetById( game, pLink->idUnit ) ),
						pLink->idUnit);
				}
				if( IS_CLIENT( game ) )
				{

					//on the client the object doesn't know what it's linked to unless it's part of the unit data
					//so if the ID's match then it's already been added - client only
					if( pLink->idUnit == UnitGetId( pUnit ) )
						return TRUE;
				}
				else
				{
					return TRUE;  // link already exists and is recorded ... don't add it again, but not an error
				}
			}
		}
	}
	else if( AppIsTugboat() )
	{
		//tugboat can have multiple entrances into a dungeon.  So lets just make sure the same warp isn't getting added.
		for (int i = 0; i < pLevel->m_nNumLevelLinks; ++i)
		{
			const LEVEL_LINK *pLink = &pLevel->m_LevelLink[ i ];
			if ( pLink->idUnit == UnitGetId( pUnit ) )
			{
				return TRUE;
			}
		}
	}
	// must be able to fit it
	ASSERTX_RETFALSE( pLevel->m_nNumLevelLinks < MAX_LEVEL_LINKS, "Exceeded level link storage" );	
	LEVEL_LINK *pLink = &pLevel->m_LevelLink[ pLevel->m_nNumLevelLinks++ ];
	
	// init level link node
	sLevelLinkInit( pLink, pUnit, idLevelLink );
	return TRUE;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL LevelAddWarp(
	UNIT *pWarp,
	UNITID idPlayer,
	WARP_RESOLVE_PASS eWarpResolvePass)
{

	// we only keep track of links for static level<->level warps
	if (ObjectIsWarp( pWarp ) == TRUE && 
		ObjectIsDynamicWarp( pWarp ) == FALSE &&
		ObjectIsSubLevelWarp( pWarp ) == FALSE )
	{

		// only resolve warps that resolve their links at this time
		const UNIT_DATA *pObjectData = UnitGetData( pWarp );
		if (pObjectData->eWarpResolvePass == eWarpResolvePass)		
		{
			LEVEL *pLevel = UnitGetLevel( pWarp );
			
			// tie warp to an actual level instance (this creates a level or links to an existing level)
			LEVELID idLevelDest = INVALID_ID;
			if( IS_SERVER( UnitGetGame( pWarp ) ) )
			{
				//only servers really know where the warp is Potentially going. 
				//Client side can only know location of door and if the unit has a specific level it's linking to( hard coded into the unit data )
				idLevelDest = s_WarpInitLink( pWarp, idPlayer, eWarpResolvePass );
				if( AppIsHellgate() )
				{
					if( ObjectWarpAllowsInvalidDestination( pWarp ) == FALSE )
					{
						ASSERTX_RETFALSE( idLevelDest != INVALID_ID, "Unable to initialize warp level link" );
					}
				}
			}

			
			return sAddLevelLink( pLevel, pWarp, idLevelDest );

		}
								
	}

	return TRUE;  // not an error, do not stop
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static ROOM_ITERATE_UNITS sStartLevelActiveSkill(
	UNIT *pUnit,
	void *pCallbackData)
{
	const UNIT_DATA *pUnitData = UnitGetData( pUnit );
	if (pUnitData->nSkillLevelActive!= INVALID_LINK)
	{
		GAME *pGame = UnitGetGame( pUnit );
		DWORD dwFlags = SKILL_START_FLAG_INITIATED_BY_SERVER;
		SkillStartRequest( 
			pGame, 
			pUnit, 
			pUnitData->nSkillLevelActive, 
			INVALID_ID, 
			cgvNone, 
			dwFlags, 
			SkillGetNextSkillSeed( pGame ));		
	}
	return RIU_CONTINUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static ROOM_ITERATE_UNITS sDoLevelFirstTimeActivation(
	UNIT *pUnit,
	void *pCallbackData)
{
	UNITID idActivator = *((UNITID *)pCallbackData);
	LevelAddWarp( pUnit, idActivator, WRP_LEVEL_FIRST_ACTIVATE );
	return RIU_CONTINUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sLevelFirstTimeActivation( 
	LEVEL *pLevel,
	UNITID idActivator)
{
	ASSERTX_RETURN( pLevel, "Expected level" );
	LevelIterateUnits( pLevel, NULL, sDoLevelFirstTimeActivation, &idActivator );	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static void s_sLevelActivateCreateRoomData(
	GAME * game,
	ROOM * room)
{
	RoomCreatePathEdgeNodeConnections(game, room, FALSE);
	RoomLayoutSelectionsRecreateSelectionData(game, room);
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static ROOM_ITERATE_UNITS s_sUpgradeToAppropriateLevel(
	UNIT *pUnit,
	void *pCallbackData)
{
	ASSERT_RETVAL(pUnit, RIU_CONTINUE);
	ASSERT_RETVAL(pCallbackData, RIU_STOP);

	if(!UnitIsA(pUnit, UNITTYPE_MONSTER))
	{
		return RIU_CONTINUE;
	}

	EVENT_DATA * pEventData = (EVENT_DATA *)pCallbackData;
	int nNewMonsterLevel = (int)pEventData->m_Data1;
	int nCurMonsterLevel = UnitGetExperienceLevel(pUnit);
	if(nNewMonsterLevel > nCurMonsterLevel)
	{
		MONSTER_SPEC tSpec;
		MonsterSpecInit( tSpec );
		tSpec.nExperienceLevel = nNewMonsterLevel;
		tSpec.nClass = UnitGetClass( pUnit );
		tSpec.nMonsterQuality = MonsterGetQuality( pUnit );
		tSpec.dwFlags = MSF_IS_LEVELUP;
		s_MonsterInitStats( UnitGetGame(pUnit), pUnit, tSpec );

		UNIT * pItem = UnitInventoryIterate(pUnit, NULL);
		while(pItem)
		{
			ITEM_SPEC tItemSpec;
			SETBIT(tItemSpec.dwFlags, ISF_MONSTER_INVENTORY_BIT);
			SETBIT(tItemSpec.dwFlags, ISF_SKIP_REQUIRED_AFFIXES_BIT);
			SETBIT(tItemSpec.dwFlags, ISF_SKIP_HAND_CRAFTED_AFFIXES_BIT);
			SETBIT(tItemSpec.dwFlags, ISF_RESTRICT_AFFIX_LEVEL_CHANGES_BIT);
			SETBIT(tItemSpec.dwFlags, ISF_SKIP_QUALITY_AFFIXES_BIT);
			SETBIT(tItemSpec.dwFlags, ISF_SKIP_PROPS);
			tItemSpec.nQuality = ItemGetQuality(pItem);
			s_ItemInitProperties(UnitGetGame(pUnit), pItem, pUnit, UnitGetClass(pItem), UnitGetData(pItem), &tItemSpec);
			pItem = UnitInventoryIterate(pUnit, pItem);
		}
	}
	return RIU_CONTINUE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static void s_sLevelUpgradeUnitsToAppropriateLevel(
	LEVEL * pLevel)
{
	ASSERT_RETURN(pLevel);
	const LEVEL_DEFINITION * pLevelDef = LevelDefinitionGet(pLevel);
	ASSERT_RETURN(pLevelDef);

	// We only do this for places where the monster level is supposed to come from the activator
	if(!pLevelDef->bMonsterLevelFromActivatorLevel)
	{
		return;
	}

	// If the override is zero, then there's no point
	if(pLevel->m_nMonsterLevelOverride == 0)
	{
		return;
	}

	TARGET_TYPE eTargetTypes[] = { TARGET_BAD, TARGET_INVALID };
	EVENT_DATA tEventData(pLevel->m_nMonsterLevelOverride);
	LevelIterateUnits(pLevel, eTargetTypes, s_sUpgradeToAppropriateLevel, &tEventData);
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static void s_sLevelActivate(
	GAME * game,
	LEVEL * level,
	UNIT * activator)
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(level);
	ASSERT_RETURN(activator);

	// in shared town games, safeguard against activating levels that are unnecessary
	if (GameAllowsActionLevels(game) == FALSE)
	{
		ASSERTV_RETURN(LevelIsActionLevel(level) == FALSE, "Level [%s] not allowed to be activated for game type", LevelGetDevName(level));
	}

	const LEVEL_DEFINITION *ptLevelDef = LevelDefinitionGet( level );
	ASSERT_RETURN( ptLevelDef );

	if ( ptLevelDef->bMonsterLevelFromActivatorLevel )
	{
		int nActivatorExperienceLevel = UnitGetExperienceLevel( activator );
		if(level->m_nMonsterLevelOverride <= 0 || nActivatorExperienceLevel > (level->m_nMonsterLevelOverride + LEVEL_MONSTER_UPGRADE_BUFFER))
		{
			level->m_nMonsterLevelOverride = nActivatorExperienceLevel + ptLevelDef->nMonsterLevelActivatorDelta;
		}
	} else {
		level->m_nMonsterLevelOverride = 0;
	}

	// do nothing if active already 
	if (LevelIsActive(level))
	{
		return;
	}

	// if level has not been populated, do so now
	if (LevelTestFlag(level, LEVEL_POPULATED_BIT) == FALSE)
	{
		s_sLevelPopulate(level, UnitGetId(activator));
	}

	LevelSetFlag(level, LEVEL_ACTIVE_BIT);

	// go through each room
	for (ROOM * room = LevelGetFirstRoom(level); room; room = LevelGetNextRoom(room))
	{
		s_sLevelActivateCreateRoomData(game, room);
	}

	// activate all rooms
	if( !(AppIsTugboat() && UnitTestFlag( activator, UNITFLAG_RETURN_FROM_PORTAL)))
	{
		for (ROOM * room = LevelGetFirstRoom(level); room; room = LevelGetNextRoom(room))
		{
			s_RoomActivate(room, activator);
		}
	}

	// notify task system
	s_TaskEventLevelActivated(level);
	s_QuestLevelActivated(game, activator, level);	// tugboat

	// start level active skills
	TARGET_TYPE eTargetTypes[] = { TARGET_OBJECT, TARGET_INVALID };
	LevelIterateUnits(level, eTargetTypes, sStartLevelActiveSkill, NULL);

	// do first time activation logic
	if (LevelTestFlag(level, LEVEL_ACTIVATED_ONCE_BIT) == FALSE)
	{
		sLevelFirstTimeActivation(level, UnitGetId(activator));
		LevelSetFlag(level, LEVEL_ACTIVATED_ONCE_BIT);
	}
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL s_sPlayersInArea(
	GAME * game,
	int area,
	unsigned int * PlayersInLevel)
{
	unsigned int numlevels = DungeonGetNumLevels(game);
	for (unsigned int ii = 0; ii < numlevels; ++ii)
	{
		LEVEL * level = LevelGetByID(game, (LEVELID)ii);
		if (!level)
		{
			continue;
		}
		if (LevelGetLevelAreaID(level) == area)
		{
			if (PlayersInLevel[level->m_idLevel] > 0)
			{
				return TRUE;
			}
		}
	}
	return FALSE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL s_sLevelCheckDestroy(
	GAME * game,
	LEVEL * & level,
	unsigned int * PlayersInLevel)
{
	BOOL bResult = FALSE;
	if (AppIsTugboat())
	{
		if( GameGetSubType(game) == GAME_SUBTYPE_PVP_CTF )
		{
			return FALSE;
		}

		int nCurrentArea = LevelGetLevelAreaID(level);
		ASSERT_DO(nCurrentArea >= 0)
		{
			DungeonRemoveLevel(game, level);
			level = NULL;
			return TRUE;
		}


		int nReservedArea = ClientSystemGetReservedArea( AppGetClientSystem(), GameGetId( game ) );

		GAME* pGame = LevelGetGame( level );
		// We Really have to go through all the levels for the game -
		// otherwise we'll get vestigial levels that were inactive when we left
		// the area, and won't be removed, as they weren't allowed to be pruned on
		// their initial deactivation.
		// Since this only happens when a level is being deactivated though,
		// it ought to be plenty cheap.
		// prune any levels that have no one in their area - we need to reroll 'em!
		for (int i = 0; i < DungeonGetNumLevels( pGame ); ++i)
		{
			LEVEL *pLevel = LevelGetByID( pGame, (LEVELID)i );
			if( pLevel )
			{

				int nLevelArea = LevelGetLevelAreaID(pLevel);
				if (LevelIsTown(pLevel))
				{
					continue;
				}

				// never throw away open worlds
				if( pLevel->m_pLevelDefinition->eSrvLevelType == SRVLEVELTYPE_OPENWORLD )
				{
					continue;
				}

				if( nLevelArea == nReservedArea )
				{
					continue;
				}

				if (s_sPlayersInArea(game, nLevelArea, PlayersInLevel))
				{
					continue;
				}

				if( ClientSystemIsGameReserved( AppGetClientSystem(), GameGetId( game ), nLevelArea ) )
				{
					continue;
				}



				DungeonRemoveLevel(game, pLevel);
				pLevel = NULL;
				// update the seed for this area ONLY!
			}
		}
	}
	level = NULL;
	return bResult;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static void s_sLevelDeactivateFreeRoomData(
	GAME * game,
	ROOM * room)
{
	s_RoomDeactivate(room, TRUE);
	RoomFreeEdgeNodeConnections(game, room);
	RoomLayoutSelectionsFreeSelectionData(game, room);
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static void s_sLevelDeactivate(
	GAME * game,
	LEVEL * & level,
	unsigned int * PlayersInLevel)
{
	if (s_sLevelCheckDestroy(game, level, PlayersInLevel))
	{
		return;
	}
	if (level == NULL)
	{
		return;
	}

	if (LevelIsActive(level) == FALSE)
	{
		return;
	}

	// clear active bit
	LevelClearFlag(level, LEVEL_ACTIVE_BIT);

	// go through each room
	for (ROOM * room = LevelGetFirstRoom(level); room; room = LevelGetNextRoom(room))
	{
		s_sLevelDeactivateFreeRoomData(game, room);
	}
}
#endif


//----------------------------------------------------------------------------
// this function is called during high-level game processing to
// update levels and rooms
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
void s_LevelsUpdate(
	GAME * game)
{
	ASSERT_RETURN(game);

	// no need to do this kind of polling every single frame
	DWORD tiCurTick = GameGetTick(game);
	DWORD tiElapsedTicks = tiCurTick - game->tiLastLevelUpdate;
	if (tiElapsedTicks < (DWORD)LEVEL_UPDATE_DELAY_IN_TICKS)
	{
		return;
	}	

	// save this tick as the last time we did an update
	game->tiLastLevelUpdate = tiCurTick;
		
	unsigned int PlayersInLevel[MAX_DUNGEON_LEVELS];
	memclear(PlayersInLevel, sizeof(PlayersInLevel));

	// figure out how many are clients on each level
	for (GAMECLIENT * client = ClientGetFirstInGame(game); client; client = ClientGetNextInGame(client))	
	{
		UNIT * unit = ClientGetControlUnit(client);
		if (!unit)
		{
			continue;
		}
		LEVEL * level = UnitGetLevel(unit);
		if (!level)
		{
			continue;
		}
		ASSERT_CONTINUE(level->m_idLevel < MAX_DUNGEON_LEVELS);
		PlayersInLevel[level->m_idLevel]++;
	}

	// go through all levels
	unsigned int numlevels = DungeonGetNumLevels(game);
	for (unsigned int ii = 0; ii < numlevels; ++ii)
	{
		LEVEL * level = LevelGetByID(game, (LEVELID)ii);
		if (!level)
		{
			continue;
		}
		
		// only care about active levels right now
		if (!LevelIsActive(level))
		{
			continue;
		}

		const LEVEL_DEFINITION * level_definition = LevelDefinitionGet(level);
		ASSERT_CONTINUE(level_definition);
		
		if (level_definition->bAlwaysActive == FALSE)
		{
			// if no players here, in this level, deactivate it
			if (level->m_idLevel < MAX_DUNGEON_LEVELS && PlayersInLevel[level->m_idLevel] == 0)
			{
				s_sLevelDeactivate(game, level, PlayersInLevel);
				if (!level)
				{
					continue;
				}
			}
		}
						
		// if level is still active, update the rooms
		if (LevelIsActive(level))
		{
			s_sLevelUpdateRooms(level);
		}
	}
}
#endif 


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static LEVELID sCreateChainedLevel(
	GAME *pGame,
	int nLevelDef,
	LEVEL *pLevelCreator,
	UNITID idActivator)
{

	// do nothing for no level
	if (nLevelDef == INVALID_LINK)
	{
		return INVALID_ID;
	}	

	// use existing level if we have one
	LEVEL_TYPE tLevelType;
	tLevelType.nLevelDef = nLevelDef;
	LEVEL *pLevelExisting = LevelFindFirstByType( pGame, tLevelType );
	if (pLevelExisting)
	{
		return LevelGetID( pLevelExisting );
	}
			
	// fill out spec
	LEVEL_SPEC tSpec;
	tSpec.tLevelType.nLevelDef = nLevelDef;
	tSpec.nDRLGDef = DungeonGetLevelDRLG( pGame, nLevelDef );
	tSpec.dwSeed = DefinitionGetGlobalSeed();
	tSpec.bPopulateLevel = FALSE;
	tSpec.pLevelCreator = pLevelCreator;
	tSpec.idActivator = idActivator;

	// create level
	LEVEL *pLevel = LevelAdd( pGame, tSpec );
	ASSERTX_RETINVALID( pLevel, "Unable to create chained level" );
		
	// return newly created level id
	return LevelGetID( pLevel );
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sLevelCreateChain( 
	LEVEL *pLevel,
	LEVEL *pLevelCreator,
	UNITID idActivator)
{
	ASSERTX_RETURN( pLevel, "Expected level" );
	GAME *pGame = LevelGetGame( pLevel );
	const LEVEL_DEFINITION *ptLevelDef = LevelDefinitionGet( pLevel );
	
	// save the creator of this level if needed, and continue creating chain
	if (ptLevelDef->nPrevLevel != INVALID_LINK || ptLevelDef->nNextLevel != INVALID_LINK)
	{
		
		// must have a creator
		ASSERTX( pLevelCreator, "Expected creator level" );
		int nLevelDefCreator = LevelGetDefinitionIndex( pLevelCreator );
		
		// is the creator our next level or previous level
		if (ptLevelDef->nPrevLevel == nLevelDefCreator)
		{
			pLevel->m_idLevelPrev = LevelGetID( pLevelCreator );
			pLevel->m_idLevelNext = sCreateChainedLevel( pGame, ptLevelDef->nNextLevel, pLevel, idActivator );
		}
		else if (ptLevelDef->nNextLevel == nLevelDefCreator)
		{
			pLevel->m_idLevelNext = LevelGetID( pLevelCreator );
			pLevel->m_idLevelPrev = sCreateChainedLevel( pGame, ptLevelDef->nPrevLevel, pLevel, idActivator );
		}
		else
		{
			ASSERTX_RETURN( 0, "Level has next or prev level, but creator matches neither of them" );
		}
				
	}
					
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
LEVEL *LevelAdd(
	GAME *pGame,
	const LEVEL_SPEC &tSpec)
{
	TIMER_STARTEX("LevelAddTimer", 5);
		
	ASSERT_RETNULL( pGame && pGame->m_Dungeon);
	DUNGEON *pDungeon = pGame->m_Dungeon;

	int nOldLevelCount = pDungeon->m_nLevelCount;

	LEVELID idLevel = tSpec.idLevel;
	if ( IS_SERVER(pGame) || tSpec.bClientOnlyLevel )
	{
		ASSERT_RETNULL(idLevel == INVALID_ID);
		idLevel = DungeonFindFreeId( pGame );
		if( idLevel >= (int)pDungeon->m_nLevelCount )
		{
			pDungeon->m_nLevelCount++;
		}
		//If the level add fails after this, YOU MUST DECREMENT the levelcount or it will crash.
	}
	else
	{
		ASSERT_RETNULL(idLevel != INVALID_ID);
		pDungeon->m_nLevelCount = MAX(pDungeon->m_nLevelCount, (unsigned int)(idLevel + 1));
	}

	LEVEL* pLevel = (LEVEL*)GMALLOCZ(pGame, sizeof(LEVEL));
	ASSERT_DO(pLevel != NULL)
	{
		if( (IS_SERVER(pGame) || tSpec.bClientOnlyLevel) && (int)pDungeon->m_nLevelCount > nOldLevelCount )
			pDungeon->m_nLevelCount--;
		return NULL;
	}
	//ASSERT_RETNULL(pLevel);
	pLevel->m_pQuadtree = NULL;
	pLevel->m_pVisibility = NULL;	
	pLevel->m_pGame = pGame;
	pLevel->m_idLevel = idLevel;
	pLevel->m_nLevelDef = INVALID_LINK;
	pLevel->m_nLevelAreaId = tSpec.tLevelType.nLevelArea;	
	pLevel->m_nDepth = tSpec.tLevelType.nLevelDepth;	
	pLevel->m_nDifficultyOffset = tSpec.nDifficultyOffset;
	pLevel->m_bPVPWorld = tSpec.tLevelType.bPVPWorld;
	pLevel->m_nNumLevelLinks = 0;
	pLevel->m_nPlayerCount = 0;
	pLevel->m_idChatChannel = INVALID_CHANNELID;
	pLevel->m_bChannelRequested = FALSE;
	pLevel->m_idWarpCreator = tSpec.idWarpCreator;
	pLevel->m_bVisited = FALSE;
	pLevel->m_nTreasureClassInLevel = tSpec.nTreasureClassInLevel;

	ZeroMemory( &pLevel->m_QuestTugboatInfo, sizeof(QUEST_LEVEL_INFO_TUGBOAT) );  //zero the entire struct out

	//Init the level of Interest items
	ArrayInit(pLevel->m_UnitsOfInterest, GameGetMemPool( pGame ), 5 );
	

	for (int i = 0; i < NUM_GENUS; ++i)
	{
		LEVEL_POSSIBLE_UNITS *pPossibleUnits = &pLevel->m_tPossibleUnits[ i ];
		pPossibleUnits->nCount = 0;
		for (int j = 0; j < LEVEL_MAX_POSSIBLE_UNITS; ++j)
		{
			pPossibleUnits->nClass[ j ] = INVALID_LINK;
		}
	}
		
	for(int i=0; i<MAX_THEMES_PER_LEVEL; i++)
	{
		pLevel->m_pnThemes[i] = INVALID_ID;
	}
	
	// init level links	
	pLevel->m_idLevelNext = INVALID_ID;
	pLevel->m_idLevelPrev = INVALID_ID;
	
	// assign level definition
	if (AppIsHellgate())
	{
		pLevel->m_nLevelDef = tSpec.tLevelType.nLevelDef;
	}
	else if (AppIsTugboat())
	{
		MYTHOS_LEVELAREAS::LevelAreaInitializeLevelAreaOverrides( pLevel );
		pLevel->m_nLevelDef = LevelDefinitionGetRandom( 
			pLevel,
			tSpec.tLevelType.nLevelArea, 
			tSpec.tLevelType.nLevelDepth);
		
#if !ISVERSION(SERVER_VERSION)
		e_VisibilityMeshSetCurrent( pLevel->m_pVisibility );
#endif
	}
	
	// must have a level definition
	ASSERT( pLevel->m_nLevelDef != INVALID_LINK );
	pLevel->m_pLevelDefinition = LevelDefinitionGet( pLevel->m_nLevelDef );
	ASSERT_DO(pLevel->m_pLevelDefinition != NULL)
	{
		if( (IS_SERVER(pGame) || tSpec.bClientOnlyLevel) && (int)pDungeon->m_nLevelCount > nOldLevelCount )
			pDungeon->m_nLevelCount--;
		return NULL;
	}

	if ( AppIsHellgate() )
	{
		pLevel->m_nPartySizeRecommended = pLevel->m_pLevelDefinition->nPartySizeRecommended;
	} else {
		pLevel->m_nPartySizeRecommended = pLevel->m_LevelAreaOverrides.nPartySizeRecommended;
	}

	//ASSERT_RETNULL( pLevel->m_pLevelDefinition );

	// units' positions are recorded in level's prox map to reduce move msg spam (MYTHOS SERVER TOWN LEVELS ONLY)
	pLevel->m_pUnitPosProxMap = 0;
#if ISVERSION(SERVER_VERSION)
	if( AppIsTugboat() 
			&& (pLevel->m_pLevelDefinition->eSrvLevelType == SRVLEVELTYPE_MINITOWN
				|| pLevel->m_pLevelDefinition->eSrvLevelType == SRVLEVELTYPE_BIGTOWN)) {
		pLevel->m_pUnitPosProxMap = MEMORYPOOL_NEW(GameGetMemPool(pGame)) PointMap;
		ASSERT_RETNULL(pLevel->m_pUnitPosProxMap);
		pLevel->m_pUnitPosProxMap->Init(GameGetMemPool(pGame));
	}
#endif // ISVERSION(SERVER_VERSION)

	const DRLG_DEFINITION *ptDRLGDef = NULL;
	int nDRLGDef = tSpec.nDRLGDef;
	if (nDRLGDef != INVALID_LINK)
	{
		ptDRLGDef = DRLGDefinitionGet( nDRLGDef );
		ASSERT_RETNULL( ptDRLGDef );
		pLevel->m_nDRLGDef = ExcelGetLineByStringIndex( pGame, DATATABLE_LEVEL_DRLGS, ptDRLGDef->pszName );
	}
	else
	{
		ASSERT( IS_SERVER( pGame ) );
		ASSERT( pLevel->m_nLevelDef != INVALID_LINK );
		pLevel->m_nDRLGDef = DungeonGetLevelDRLG( pGame, pLevel );
		ptDRLGDef = LevelGetDRLGDefinition( pLevel );
	}

	DWORD dwSeed = tSpec.dwSeed;
	while (dwSeed == 0 || dwSeed == 1000)
	{
		dwSeed = RandGetNum(pGame);
	}
#if ISVERSION(DEVELOPMENT)		// don't assert on live servers to reduce server spam
	if (IS_SERVER(pGame))
	{
		TraceGameInfo("DUNGEON SEED: %u", dwSeed);
	}
#endif
	pLevel->m_dwSeed = dwSeed;
	
	// set level name (it's nice to see this in the debugger)
	if (AppIsHellgate())
	{
		PStrPrintf( 
			pLevel->m_szLevelName, 
			LEVEL_NAME_SIZE, 
			"Def=%s [id=%d] [DRLG=%s] [Seed=%d]", 
			pLevel->m_pLevelDefinition->pszName,
			pLevel->m_idLevel,
			ptDRLGDef->pszName,
			pLevel->m_dwSeed);	
	
	}
	else if (AppIsTugboat())
	{
		PStrPrintf( 
			pLevel->m_szLevelName, 
			LEVEL_NAME_SIZE, 
			"Area=%s [Depth=%d] [id=%d] [Def=%s]", 
			MYTHOS_LEVELAREAS::LevelAreaGetDevName( LevelGetLevelAreaID( pLevel ) ),
			pLevel->m_nDepth,
			pLevel->m_idLevel,
			pLevel->m_pLevelDefinition->pszName);
	}

	// clear all sublevels
	SubLevelClearAll( pLevel );

	// create one sub level as the default
	int nSubLevelDef = LevelGetDefaultSubLevelDefinition( pLevel );
	SubLevelAdd( pLevel, nSubLevelDef );
	
	// drb - forcing to 1 district for now...
	// now associated with a level & random drlg choice
	pLevel->m_nDistrictSpawnClass[0] = DungeonGetLevelSpawnClass( pGame, pLevel );	
	pLevel->m_nDistrictNamedMonsterClass[0] = DungeonGetLevelNamedMonsterClass( pGame, pLevel );	
	pLevel->m_fDistrictNamedMonsterChance[0] = DungeonGetLevelNamedMonsterChance( pGame, pLevel );	
	for (int i = 1; i < MAX_LEVEL_DISTRICTS; ++i)
	{
		pLevel->m_nDistrictSpawnClass[ i ] = INVALID_LINK;
		pLevel->m_nDistrictNamedMonsterClass[ i ] = INVALID_LINK;
		pLevel->m_fDistrictNamedMonsterChance[ i ] = 0.0f;
	}

	//Maybe this should be someplace else. Seem like an ok place for it now....
	if( AppIsTugboat() &&
		pLevel->m_LevelAreaOverrides.bOverridesEnabled &&
		pLevel->m_LevelAreaOverrides.nSpawnClass != INVALID_ID )
	{
		pLevel->m_nDistrictSpawnClass[0] = pLevel->m_LevelAreaOverrides.nSpawnClass;
	}

	pLevel->m_nQuestSpawnClass = pLevel->m_pLevelDefinition->nQuestSpawnClass;
	pLevel->m_nInteractableSpawnClass = pLevel->m_pLevelDefinition->nInteractableSpawnClass;

	if (tSpec.pBoundingBox)
	{
		MemoryCopy(&pLevel->m_BoundingBox, sizeof(BOUNDING_BOX), tSpec.pBoundingBox, sizeof(BOUNDING_BOX));
	}
		
	pLevel->m_CellWidth = tSpec.flCellWidth;

	// init level quad tree
	if (AppIsTugboat())
	{
		if( IS_CLIENT( pGame ) )
		{ 
			if( pLevel->m_pQuadtree )
			{
				GFREE( pGame, pLevel->m_pQuadtree );
				pLevel->m_pQuadtree = NULL;
			}
			// quadtree for room visibility, now that we know the bounds and base room size
			
			pLevel->m_pQuadtree = (CQuadtree< ROOM*, CRoomBoundsRetriever>*)GMALLOC( pGame, sizeof(CQuadtree< ROOM*, CRoomBoundsRetriever>));
			CRoomBoundsRetriever boundsRetriever;
			pLevel->m_pQuadtree->Init( pLevel->m_BoundingBox, tSpec.flCellWidth, boundsRetriever, GameGetMemPool( pGame ) );
	#if !ISVERSION(SERVER_VERSION)		
			if( pLevel->m_pVisibility )
			{
				delete( pLevel->m_pVisibility );
			}
			// only meshes with actual visibility meshes need to bother creating one
			if( pLevel->m_pLevelDefinition->fVisibilityOpacity > 0 )
			{
				// quadtree for room visibility, now that we know the bounds and base room size
				pLevel->m_pVisibility = new CVisibilityMesh( *pLevel );
			}
			e_VisibilityMeshSetCurrent( pLevel->m_pVisibility );
	#endif
		}
	
	}
	
	RandInit(pLevel->m_randSeed, dwSeed);

	pLevel->m_dwUID = RandGetNum(pLevel->m_randSeed);		// overwritten later on client

	LevelGetAnchorMarkers( pLevel )->Init( pGame );

	if( AppIsTugboat() )
	{
		// we only reroll the dungeon seed if this is a fresh game,
		if( DungeonGetSeed( pGame ) == 0 ||
			DungeonGetSeed( pGame ) == 1000 )
		{
			DungeonSetSeed(pGame, dwSeed);
		}
	}
	else
	{
		DungeonSetSeed(pGame, dwSeed);
	}

	if (nOldLevelCount > idLevel)
	{
		LevelFree(pGame, idLevel);
		ASSERT(pDungeon->m_Levels[idLevel] == NULL);
	}

	if ((int)pDungeon->m_nLevelCount > nOldLevelCount)
	{
	
		pDungeon->m_Levels = (LEVEL**)GREALLOC(
			pGame, 
			pDungeon->m_Levels, 
			sizeof(LEVEL*) * pDungeon->m_nLevelCount);
			
		memclear(
			pDungeon->m_Levels + nOldLevelCount, 
			sizeof(LEVEL*) * (pDungeon->m_nLevelCount - nOldLevelCount));
	}

	pDungeon->m_Levels[pLevel->m_idLevel] = pLevel;

	// client does its here - server does it when it finalizes the DRLG.
	if( IS_CLIENT( pGame ) )
	{
		LevelCollisionInit(pGame, pLevel);
	}

	// allocate districts for this level (creates districts but does not place any rooms in them)
	sLevelAllocateDistricts( pGame, pLevel );

	// Select the weather that will be present on this level
	
	int nWeatherSet(0 );
	if( ptDRLGDef )
		nWeatherSet = ptDRLGDef->nWeatherSet;

	if( AppIsTugboat() )
	{
		int nNewWeatherSet = MYTHOS_LEVELAREAS::LevelAreaGetWeather( pLevel );
		nWeatherSet = ( nNewWeatherSet != INVALID_ID )?nNewWeatherSet:nWeatherSet;
	}
	if( nWeatherSet >= 0)
	{
		pLevel->m_pWeather = WeatherSelect(pGame, nWeatherSet, dwSeed);
	}

	if (IS_SERVER(pGame))
	{
#if !ISVERSION( CLIENT_ONLY_VERSION )
		// debug info
		trace( "Level '%s' [%d] created\n", LevelGetDevName( pLevel ), LevelGetID( pLevel ) );
		
		// reset any spawn memory for this level	
		LevelResetSpawnMemory(pGame, pLevel);	

		// when creating a level from a warp, make the link between the level that
		// the warp is on and this newly created level
		if (tSpec.idWarpCreator != INVALID_LINK)
		{
			UNIT *pWarpCreator = UnitGetById( pGame, tSpec.idWarpCreator );
			LEVEL *pLevelWarpCreator = UnitGetLevel( pWarpCreator );
			sAddLevelLink( pLevelWarpCreator, pWarpCreator, LevelGetID( pLevel ) );
		}
				
		// create any next of prev levels, we want to do this before we populate with rooms
		// so that any warps that are created during the population have the opportunity
		// to look at any next or prev levels that we're creating now
		sLevelCreateChain( pLevel, tSpec.pLevelCreator, tSpec.idActivator );
				
		// create level contents
		if (tSpec.bPopulateLevel)
		{
			s_sLevelPopulate(pLevel, tSpec.idActivator );
		}

		GAME_INSTANCE_TYPE	eInstanceType = GAME_INSTANCE_NONE;
		if (LevelIsTown(pLevel) &&
		   (GameGetSubType(pGame) == GAME_SUBTYPE_MINITOWN ||
			GameGetSubType(pGame) == GAME_SUBTYPE_OPENWORLD ) &&
			pLevel->m_idWarpCreator == INVALID_ID)
			eInstanceType = GAME_INSTANCE_TOWN;
		else if (AppIsHellgate() && GameIsPVP(pGame))
			eInstanceType = GAME_INSTANCE_PVP;

		if (eInstanceType != GAME_INSTANCE_NONE)
		{
			if( AppIsHellgate() )
			{
				s_ChatNetRegisterNewInstance(GameGetId(pGame), LevelGetDefinitionIndex(pLevel), (int)eInstanceType, 0);
			}
			else
			{
				s_ChatNetRegisterNewInstance(GameGetId(pGame), LevelGetLevelAreaID(pLevel), (int)eInstanceType, LevelGetPVPWorld(pLevel) ? 1 : 0 );
			}
		}
#endif
	}
	else
	{
		LevelSetFlag(pLevel, LEVEL_ACTIVE_BIT);
	}

#if ISVERSION(SERVER_VERSION)
	if(pGame->pServerContext )PERF_ADD64(pGame->pServerContext->m_pPerfInstance,GameServer,GameServerLevelCount,1);
#endif


	return pLevel;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int s_LevelGetPlayerCount(
	LEVEL *pLevel)
{
	ASSERTX_RETZERO( pLevel, "Expected level" );	
	int nCount = 0;
	
	GAME *pGame = LevelGetGame( pLevel );
	for (GAMECLIENT *pClient = ClientGetFirstInGame( pGame ); 
		 pClient; 
		 pClient = ClientGetNextInGame( pClient ))	
	{
		UNIT *pControlUnit = ClientGetControlUnit( pClient );
		if (pControlUnit)
		{
			if (UnitGetLevel( pControlUnit ) == pLevel)
			{
				nCount++;
			}
		}
	}
	
	return nCount;
	
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LevelSetFlag(
	LEVEL *pLevel,
	LEVEL_FLAG eFlag)
{
	ASSERTX_RETURN( pLevel, "Expected Level" );
	ASSERTX_RETURN( eFlag >= 0 && eFlag < NUM_LEVEL_FLAGS, "Invalid level flag" );
	SETBIT( pLevel->m_dwFlags, eFlag );	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LevelClearFlag(
	LEVEL *pLevel,
	LEVEL_FLAG eFlag)
{
	ASSERTX_RETURN( pLevel, "Expected Level" );
	ASSERTX_RETURN( eFlag >= 0 && eFlag < NUM_LEVEL_FLAGS, "Invalid level flag" );
	CLEARBIT( pLevel->m_dwFlags, eFlag );	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL LevelTestFlag(
	const LEVEL *pLevel,
	LEVEL_FLAG eBit)
{
	ASSERTX_RETFALSE( pLevel, "Expected Level" );
	ASSERTX_RETFALSE( eBit >= 0 && eBit < NUM_LEVEL_FLAGS, "Invalid level bit" );
	return TESTBIT( pLevel->m_dwFlags, eBit ) == TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
ROOM* LevelGetFirstRoom(
	const LEVEL *level)
{
	HITCOUNT(LEVEL_FIRST_ROOM);
	ASSERT_RETNULL(level);
	ASSERTX_RETNULL( IS_CLIENT( level->m_pGame ) || LevelTestFlag(level, LEVEL_POPULATED_BIT), "Server level must be populated with rooms first" );	
	return level->m_pRooms;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
ROOM* LevelGetNextRoom(
	ROOM* room)
{
	HITCOUNT(LEVEL_NEXT_ROOM);
	ASSERT_RETNULL(room);
	return room->pNextRoomInLevel;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int LevelGetMonsterExperienceLevel(
	const LEVEL* pLevel,
	int nDifficultyOverride /* = DIFFICULTY_INVALID */)
{	
	ASSERT_RETVAL( pLevel, 1 );
	
#if ISVERSION(CHEATS_ENABLED)
	if (gnMonsterLevelOverride!= -1)
	{
		return gnMonsterLevelOverride;
	}
#endif

	GAME* pGame = LevelGetGame( pLevel );	
	const LEVEL_DEFINITION *ptLevelDef = LevelDefinitionGet( pLevel );
	ASSERT_RETVAL( ptLevelDef, 1 );
	if (ptLevelDef->bMonsterLevelFromCreatorLevel)
	{
		UNITID idWarpCreator = pLevel->m_idWarpCreator;
		if (idWarpCreator != INVALID_LINK)
		{
			UNIT *pWarpCreator = UnitGetById( pGame, idWarpCreator );
			if (pWarpCreator)
			{
				LEVEL *pLevelCreator = UnitGetLevel( pWarpCreator );
				return LevelGetMonsterExperienceLevel( pLevelCreator );
			}
			ASSERTV( 
				0, 
				"Unable to find warp creator for level (%s) that needs it to get the monster xp level info",
				LevelGetDevName( pLevel ));
		}
		ASSERTV( 
			0, 
			"No warp creator exists level (%s) which wants it for monster xp level info",
			LevelGetDevName( pLevel ));		
	}

	if ( pLevel->m_nMonsterLevelOverride > 1 )
	{
		return pLevel->m_nMonsterLevelOverride;
	}

	int nDifficulty = (nDifficultyOverride == DIFFICULTY_INVALID) ? pGame->nDifficulty : nDifficultyOverride;
	if (nDifficulty < 0 || nDifficulty >= NUM_DIFFICULTIES)
	{
		ASSERTX(FALSE, "Illegal difficulty, defaulting to normal difficulty to determine monster experience level for level" );
		nDifficulty = DIFFICULTY_NORMAL;
	}

	int retval = ExcelEvalScript(
		pGame, 
		NULL, 
		NULL, 
		NULL, 
		DATATABLE_LEVEL, 
		OFFSET(LEVEL_DEFINITION, codeMonsterLevel[nDifficulty]), 
		LevelGetDefinitionIndex( pLevel ));

	if ( AppIsTugboat() )
	{
		// this calculation needs to happen in data, not here!
		/*int Depth = LevelGetDepth( pLevel ) + (int)FLOOR( (float)( LevelGetDifficultyOffset( pLevel ) ) * .75f );
		if( Depth < 0 )
		{
			Depth = 0;
		}*/
		int nDifficultyTugboat = MYTHOS_LEVELAREAS::LevelAreaGetDifficulty( pLevel );
		retval = max( 1, max( retval, nDifficultyTugboat ) );		
	}
	return retval;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct ROOM_MARK_UNITS_UNKNOWN_DATA
{
	GAME *				game;
	GAMECLIENT *		client;
	UNIT *				player;
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static ROOM_ITERATE_UNITS s_RoomMarkUnitsUnknownToClient(
	UNIT * unit,
	void * data)
{	
	ROOM_MARK_UNITS_UNKNOWN_DATA * cb = (ROOM_MARK_UNITS_UNKNOWN_DATA *)data;
	if (unit != cb->player &&
		!s_ClientAlwaysKnowsUnitInWorld( cb->client, unit ) )
	{
		ClientRemoveKnownUnit(cb->client, unit);
	}
	return RIU_CONTINUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_LevelMarkUnitsUnknownToClient(
	GAME * game, 
	GAMECLIENT * client,
	UNIT * player,
	LEVEL * level)
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(client);
	ASSERT_RETURN(player);
	ASSERT_RETURN(level);
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETURN(IS_SERVER(game));
	ASSERT_RETURN(UnitGetLevel(player) == level);

	ROOM_MARK_UNITS_UNKNOWN_DATA data;
	data.game = game;
	data.client = client;
	data.player = player;

	ROOM * curr = NULL;
	ROOM * next = level->m_pRooms;
	while ((curr = next) != NULL)
	{
		next = curr->pNextRoomInLevel;

		RoomIterateUnits(curr, NULL, s_RoomMarkUnitsUnknownToClient, (void *)&data);
	}
#endif
}


//----------------------------------------------------------------------------
enum LEVEL_WARP_COMMON_FLAGS
{
	LWCF_JOINING_GAME_BIT,
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTryJoinLog(
	BOOL bJoiningGame,
	GAME *pGame,
	UNIT *pPlayer,
	const char *pszFormat,
	...)
{	
	if (bJoiningGame && gbEnablePlayerJoinLog)
	{
		ASSERTX_RETURN( pPlayer, "Expected unit" );	
		
		const int MAX_MESSAGE = 2048;
		char szMessage[ MAX_MESSAGE ];
		va_list args;
		va_start( args, pszFormat );
		PStrVprintf( szMessage, MAX_MESSAGE, pszFormat, args );
		
		GAMECLIENT *pClient = UnitGetClient( pPlayer );
		s_PlayerJoinLog( pGame, pPlayer, pClient->m_idAppClient, szMessage );	
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
static BOOL s_sLevelWarpCommon(
	UNIT *pPlayer,
	WARP_SPEC &tWarpSpec,
	LEVELID idLevelLeaving,
	DWORD dwLevelWarpCommonFlags)
{
	ASSERTX_RETFALSE( UnitIsPlayer( pPlayer ), "Expected player" );	
	GAME *pGame = UnitGetGame( pPlayer );	
	GAMECLIENT *pClient = UnitGetClient( pPlayer );
	ASSERTX_RETFALSE( pClient, "Unable to get client onplayer warping levels" );
	GAMECLIENTID idClient = ClientGetId( pClient );
	BOOL bJoiningGame = TESTBIT( dwLevelWarpCommonFlags, LWCF_JOINING_GAME_BIT );

	sTryJoinLog( bJoiningGame, pGame, pPlayer, "Level warp common start" );

	if( AppIsTugboat() && tWarpSpec.nLevelAreaPrev == INVALID_ID && UnitGetLevel( pPlayer ) )
	{
		tWarpSpec.nLevelAreaPrev = UnitGetLevelAreaID( pPlayer );
	}
	// make sure we have a valid dungeon seed, or we'll get bitten later!
	DWORD dwSeed = DungeonGetSeed( pGame );
	if (tWarpSpec.nLevelAreaDest != INVALID_LINK)
	{
		dwSeed = DungeonGetSeed( pGame, tWarpSpec.nLevelAreaDest );
	}
	while (dwSeed == 0 || dwSeed == 1000)
	{
		dwSeed = RandGetNum( pGame );
		DungeonSetSeed( pGame, dwSeed );
	}

	// find the level instance we're going to
	if (tWarpSpec.idLevelDest == INVALID_ID)
	{
		sTryJoinLog( bJoiningGame, pGame, pPlayer, "level warp common - Invalid idLevelDest" );
		ASSERTX_RETFALSE( tWarpSpec.idLevelDest != INVALID_LINK, "Expected level id" );
	}
	LEVEL *pLevelDest = LevelGetByID( pGame, tWarpSpec.idLevelDest );	

	// we must have a destination level instance now
 	if (pLevelDest == NULL)
	{
		sTryJoinLog( bJoiningGame, pGame, pPlayer, "level warp common - Unable to find pLevelDest" );
		ASSERTX_RETFALSE( pLevelDest, "Expected destination level instance" );
	}
	LEVELID idLevelDest = LevelGetID( pLevelDest );
	int nLevelDefDest = LevelGetDefinitionIndex( pLevelDest );	
	int nDRLGDefDest = pLevelDest->m_nDRLGDef;

	// are we actually changing to a different level
	BOOL bChangingToDifferentLevel = TRUE;
	LEVEL * pPlayerLevel = UnitGetLevel(pPlayer);
	if(pPlayerLevel && idLevelDest == LevelGetID(pPlayerLevel))
	{
		bChangingToDifferentLevel = FALSE;
	}
	/*if( AppIsTugboat() &&
		pPlayerLevel && LevelGetLevelAreaID( pPlayerLevel ) != tWarpSpec.nLevelAreaDest )
	{
		// if we are warping to a new area, always close all old warps.
		s_TownPortalsClose( pPlayer );
	}*/

	// if we're joining a new game, the player should not have a level
	if (bJoiningGame)
	{
		ASSERTV( 
			pPlayerLevel == NULL,
			"s_sLevelWarpCommon() - Unit '%s' is joining game but is already on level '%s' in room '%s'",
			UnitGetDevName( pPlayer ),
			LevelGetDevName( pPlayerLevel ),
			RoomGetDevName( UnitGetRoom(  pPlayer ) ) );
			
		// we are having players stuck on the loading screen because they are considered
		// as not changing levels, even though they are joining the game for the first
		// time, force the changing to a new level logic
		bChangingToDifferentLevel = TRUE;
		
	}

	sTryJoinLog( bJoiningGame, pGame, pPlayer, "level warp common - bChangingToDifferentLevel=%d", bChangingToDifferentLevel );
		

	if (bChangingToDifferentLevel == TRUE)
	{
	
		// cancel all interaction, note that it's important to do this before we start messing
		// with the known rooms of this level so that we can get items put back
		// properly into inventories of players
		s_InteractCancelAll( pPlayer );

		// hardcore players can't go anywhere except levels that are portal and recall locations 
		// via a travel terminal
		if (PlayerIsHardcoreDead( pPlayer ))
		{
			const LEVEL_DEFINITION *pLevelDefDest = LevelDefinitionGet( pLevelDest );
			if (pLevelDefDest->bPortalAndRecallLoc == FALSE)
			{
				sTryJoinLog( bJoiningGame, pGame, pPlayer, "level warp common - Hardcore dead cannot join a level that can't be started at" );
				UNITLOG_WARNX( pLevelDefDest->bPortalAndRecallLoc == FALSE, "Hardcore dead player found a way to warp to an illegal level", pPlayer );
				return FALSE;
			}
		}

		sTryJoinLog( 
			bJoiningGame, 
			pGame, 
			pPlayer, 
			"level warp common - sending level start load, leveldef=%d, leveldepth=%d, levelarea=%d, drlgdef=%d",
			nLevelDefDest, 
			tWarpSpec.nLevelDepthDest, 
			tWarpSpec.nLevelAreaDest,
			nDRLGDefDest);

		// clean up stuff from the old level		
		if (pPlayerLevel)
		{
		
			// mark all units on the players previous level as unknown		
			s_LevelMarkUnitsUnknownToClient(pGame, pClient, pPlayer, pPlayerLevel);
			
			// any units that the client has been caching until level change must now be removed
			ClientRemoveCachedUnits( pClient, UKF_CACHED );

		}

		// tell client to start loading	this level
		s_LevelSendStartLoad( 
			pGame,
			idClient,
			nLevelDefDest, 
			tWarpSpec.nLevelDepthDest, 
			tWarpSpec.nLevelAreaDest,
			nDRLGDefDest);

		// tell the quests we are leaving this level...
		//if(AppIsHellgate())
		//{
		s_QuestEventLeaveLevel( pPlayer, pLevelDest );
		//}


		// put player in limbo
		// if we are returning to a headstone, we will be safe when we arrive so that
		// we will be invulnerable and ignored for a little	
		BOOL bSafeOnExitLimbo = LevelIsActionLevel( pLevelDest );// || LevelGetPVPWorld( pLevelDest ); // TESTBIT( tWarpSpec.dwWarpFlags, WF_ARRIVE_AT_HEADSTONE_BIT )
		s_UnitPutInLimbo( pPlayer, bSafeOnExitLimbo );

		// clear any previous rooms that they knew about before, this will stop this client
		// from receiving messages about anything that goes on in those rooms now
		ClientClearAllRoomsKnown(pClient, pPlayerLevel);
		
		// activate level (populate if necessary), but do *not* activate all sublevel rooms
		s_sLevelActivate(pGame, pLevelDest, pPlayer);	
		
		// send level to client
		s_LevelSendToClient( pGame, pPlayer, ClientGetId( pClient ), idLevelDest );
	}
	
	// upgrade any existing enemies to be the monster override level (if necessary)
	s_sLevelUpgradeUnitsToAppropriateLevel(pLevelDest);

	// warp into level ... this will cause the rooms around the player to be sent to them	
	BOOL bWarped = s_PlayerWarpLevels( 
		pPlayer, 
		idLevelDest, 
		idLevelLeaving,
		tWarpSpec);
	sTryJoinLog( bJoiningGame, pGame, pPlayer, "level warp common - player warp levels return value is '%d'", bWarped );

	if (bChangingToDifferentLevel == TRUE)
	{

		sTryJoinLog( bJoiningGame, pGame, pPlayer, "level warp common - sending level load complete" );
		
		// level load is done now
		MSG_SCMD_LOADCOMPLETE tMessageComplete;
		tMessageComplete.nLevelDefinition = nLevelDefDest;
		tMessageComplete.nDRLGDefinition = nDRLGDefDest;	
		tMessageComplete.nLevelId = LevelGetID( pLevelDest );	
		s_SendMessage( pGame, ClientGetId( pClient ), SCMD_LOADCOMPLETE, &tMessageComplete );
		
	}

	sTryJoinLog( bJoiningGame, pGame, pPlayer, "level warp common - complete with success" );
		
	return TRUE;
	
}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int LevelFindConnectedRootLevelDefinition( 
	int nLevelDef,
	LEVEL_CONNECTION_DIRECTION eLevelConnection)
{
	ASSERTX_RETINVALID( nLevelDef != INVALID_LINK, "Expected level definition index" );
	const LEVEL_DEFINITION *ptLevelDef = LevelDefinitionGet( nLevelDef );
	ASSERTX_RETINVALID( ptLevelDef, "Expected level definition" );

	// if this level has no next/prev levels, it is a root
	if (ptLevelDef->nNextLevel == INVALID_LINK && ptLevelDef->nPrevLevel == INVALID_LINK)
	{
		return nLevelDef;
	}
		
	// search next links
	int nLevelDefRoot = INVALID_LINK;
	if (ptLevelDef->nNextLevel != INVALID_LINK && eLevelConnection == LCD_NEXT)
	{
		nLevelDefRoot = LevelFindConnectedRootLevelDefinition( ptLevelDef->nNextLevel, LCD_NEXT );
		if (nLevelDefRoot != INVALID_LINK)
		{
			return nLevelDefRoot;
		}
	}
	
	// search prev links
	if (ptLevelDef->nPrevLevel != INVALID_LINK && eLevelConnection == LCD_PREV)
	{
		nLevelDefRoot = LevelFindConnectedRootLevelDefinition( ptLevelDef->nPrevLevel, LCD_PREV );
		if (nLevelDefRoot != INVALID_LINK)
		{
			return nLevelDefRoot;
		}
	}

	return INVALID_LINK;
		
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_LevelCreateRoot(
	GAME *pGame,
	int nLevelDef,
	UNITID idActivator)
{

	// tugboat doesn't do this ... at least, until I later discover that it needs it ;) -Colin
	if (AppIsTugboat())
	{
		return;
	}
	
	// find a connected root level, we don't care which one ... a root level
	// is a level that requires no parent to create and therefore has
	// no next or prev levels such as a station.  We do this in both directions
	// so that we can start anywhere
	int nLevelDefRoot = LevelFindConnectedRootLevelDefinition( nLevelDef, LCD_NEXT );
	if (nLevelDefRoot == INVALID_LINK)
	{
		nLevelDefRoot = LevelFindConnectedRootLevelDefinition( nLevelDef, LCD_PREV );
	}
	ASSERTX_RETURN( nLevelDefRoot != INVALID_LINK, "Expected root level defintition" );
	
	// if we already have a level of this type created, don't create another root
	LEVEL_TYPE tLevelTypeRoot;
	tLevelTypeRoot.nLevelDef = nLevelDefRoot;
	LEVEL *pLevelRoot = LevelFindFirstByType( pGame, tLevelTypeRoot );
	if (pLevelRoot)
	{
		// make sure it's active so any other chained levels are created off of it	
		s_sLevelPopulate( pLevelRoot, idActivator );
	}
	else
	{
			
		// fill out level spec		
		LEVEL_SPEC tSpec;
		tSpec.tLevelType = tLevelTypeRoot;
		tSpec.nDRLGDef = DungeonGetLevelDRLG( pGame, nLevelDefRoot );
		tSpec.dwSeed = DefinitionGetGlobalSeed();
		tSpec.bPopulateLevel = TRUE;	// populating a level will populate sublevels, and create warps, which create connected leves etc.
		tSpec.idActivator = idActivator;

		// add level				
		pLevelRoot = LevelAdd( pGame, tSpec );
		ASSERTX_RETURN( pLevelRoot, "Unable to create root level" );

	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static LEVELID s_sLevelGetConnectedInstanceOf(
	const LEVEL *pLevel,
	int nLevelDefConnected,
	int nLevelAreaPrev = INVALID_ID )
{
	ASSERTX_RETINVALID( pLevel, "Expected level" );
	GAME *pGame = LevelGetGame( pLevel );
	
	// check next/prev links
	if (pLevel->m_idLevelNext != INVALID_ID)
	{
		LEVEL *pLevelOther = LevelGetByID( pGame, pLevel->m_idLevelNext );
		if (LevelGetDefinitionIndex( pLevelOther ) == nLevelDefConnected)
		{
			return LevelGetID( pLevelOther );
		}
	}
	if (pLevel->m_idLevelPrev != INVALID_ID)
	{
		LEVEL *pLevelOther = LevelGetByID( pGame, pLevel->m_idLevelPrev );
		if (LevelGetDefinitionIndex( pLevelOther ) == nLevelDefConnected)
		{
			return LevelGetID( pLevelOther );
		}
	}
	
	// go through level connections
	LEVELID idLevelConnected = INVALID_ID;	
	for (int i = 0; i < pLevel->m_nNumLevelLinks; ++i)
	{
		const LEVEL_LINK *pLink = &pLevel->m_LevelLink[ i ];
		const LEVEL *pLevelOther = LevelGetByID( pGame, pLink->idLevel );
		if( AppIsHellgate() ||
			pLevelOther )	//Tugboat allows for level to be NULL.  For instances in hubs where links are used as a secondary starting point.
		{
			int nLevelDefOther = LevelGetDefinitionIndex( pLevelOther );
			if( AppIsHellgate() )
			{
				if (nLevelDefOther == nLevelDefConnected)
				{
				
					// warn us if we have more than one level instance that satisfies this query
					ASSERTX_CONTINUE( idLevelConnected == INVALID_LINK, "More than one level defs are connected to level, don't know which level instance you mean to get" );
					idLevelConnected = LevelGetID( pLevelOther );
					
					// do not break, keep searching so we can catch errors if we have them			
					
				}
			}
			else
			{
				if( pLevelOther &&  nLevelAreaPrev == LevelGetLevelAreaID( pLevelOther ) )
				{
					// warn us if we have more than one level instance that satisfies this query
					ASSERTX_CONTINUE( idLevelConnected == INVALID_LINK, "More than one level defs are connected to level, don't know which level instance you mean to get" );
					idLevelConnected = LevelGetID( pLevelOther );
				}
			}
		}
		
	}

	return idLevelConnected;
		
}
//returns the warp unit for a player to connect to
UNIT * LevelGetWarpConnectionUnit( GAME *pGame,
								   LEVEL *pLevelGoingTo,
								   int nLevelAreaLeaving,
								   int nLevelAreaLeavingDepth /* = 0 */)
{
	ASSERT_RETNULL( pGame );
	ASSERT_RETNULL( pLevelGoingTo );
	if( nLevelAreaLeaving == INVALID_ID )
		return NULL;
	

	
	int nLevelAreaGoingTo = LevelGetLevelAreaID( pLevelGoingTo );
	ASSERT_RETNULL( nLevelAreaGoingTo != INVALID_ID );
	if( MYTHOS_LEVELAREAS::LevelAreaIsRandom( nLevelAreaLeaving ) ) //level I'm leaving is random
	{
		int nCount( 0 );
		int nBaseLevelAreaForRandom = MYTHOS_LEVELAREAS::LevelAreaGetLevelAreaIDWithNoRandom( nLevelAreaLeaving );
		for (int i = 0; i < pLevelGoingTo->m_nNumLevelLinks; ++i)
		{
			const LEVEL_LINK *pLink = &pLevelGoingTo->m_LevelLink[ i ];
			UNIT *pWarp = UnitGetById( pGame, pLink->idUnit );
			ASSERT_RETNULL( pWarp );
			if( UnitGetWarpToLevelAreaID( pWarp ) == nBaseLevelAreaForRandom )
			{
				nCount++;
			}
		}	

		int nChoice = MYTHOS_LEVELAREAS::LevelAreaGetWarpIndexByWarpCount( nLevelAreaLeaving, nCount );
		UNIT *pReturnUnit( NULL );
		for (int i = 0; i < pLevelGoingTo->m_nNumLevelLinks; ++i)
		{
			const LEVEL_LINK *pLink = &pLevelGoingTo->m_LevelLink[ i ];
			UNIT *pWarp = UnitGetById( pGame, pLink->idUnit );
			if( UnitGetWarpToLevelAreaID( pWarp ) == nBaseLevelAreaForRandom )
			{
				pReturnUnit = pWarp;				
				if( nChoice == 0 )
					return pWarp;
				nChoice--;
			}
		}	
		return pReturnUnit;
		
	}
	else
	{
		for (int i = 0; i < pLevelGoingTo->m_nNumLevelLinks; ++i)

		{
			const LEVEL_LINK *pLink = &pLevelGoingTo->m_LevelLink[ i ];
			UNIT *pWarp = UnitGetById( pGame, pLink->idUnit );
			ASSERT_RETNULL( pWarp );
			if( UnitGetWarpToLevelAreaID( pWarp ) == nLevelAreaLeaving )
			{
				return pWarp;
			}

		}		
	}

	return NULL;
}

//----------------------------------------------------------------------------
// player is warping into this game for the first time or 
// is coming from another game instance	
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
BOOL s_LevelWarpIntoGame(
	UNIT *pPlayer,
	WARP_SPEC &tWarpSpec,
	int nLevelDefLeaving)
{
	ASSERTX_RETFALSE( pPlayer, "Expected unit" );
	ASSERTX_RETFALSE( UnitIsPlayer( pPlayer ), "Expected player" );
	GAME *pGame = UnitGetGame( pPlayer );
	GAMECLIENT *pClient = UnitGetClient( pPlayer );
	ASSERTX_RETFALSE( pClient, "Expected client" );

	s_PlayerJoinLog( pGame, pPlayer, pClient->m_idAppClient, "Warping player into game level" );

	// they are now allowed to have a room
	UnitSetFlag( pPlayer, UNITFLAG_ALLOW_PLAYER_ROOM_CHANGE );
		
	// the destination level should be blank here ... if it becomes possible or desired
	// to know the level id higher up than this function, then it's ok to remove
	// this assert, but note that we're creating or finding the destination level
	// in this function and that you would probably want to move that too
	if (tWarpSpec.idLevelDest != INVALID_ID)
	{
		s_PlayerJoinLog( pGame, pPlayer, pClient->m_idAppClient, "s_LevelWarpIntoGame() Warp spec has dest level id and should not" );
		ASSERTX_RETFALSE( tWarpSpec.idLevelDest == INVALID_ID, "Level instance must be invalid" );
	}
	LEVEL *pLevelDest = NULL;		

	// find any town portal belonging to this player, and link to their player instance coming in
	s_TownPortalFindAndLinkPlayerPortal( pPlayer );
	
	// alter the destination level when arriving at the unknown location of a specific GUID
	if (tWarpSpec.guidArriveAt != INVALID_GUID)
	{
		UNIT *pDestUnit = UnitGetByGuid( pGame, tWarpSpec.guidArriveAt );
		if (pDestUnit)
		{
			BOOL bValid = TRUE;
			
			// when warping to a party member, validate we can actually go where they are now
			if (TESTBIT( tWarpSpec.dwWarpFlags, WF_ARRIVE_VIA_PARTY_PORTAL_BIT ) == TRUE)
			{
				if (s_WarpValidatePartyMemberWarpTo( pPlayer, tWarpSpec.guidArriveAt, FALSE ) == FALSE)
				{
					bValid = FALSE;
				}
			}
		
			if (bValid)
			{
			
				// our destination level is the level that unit is on
				pLevelDest = UnitGetLevel( pDestUnit );		
				
				// alter the warp spec to have accurate information based on current location of dest unit		
				if (AppIsHellgate())
				{
					tWarpSpec.nLevelDefDest = LevelGetDefinitionIndex( pLevelDest );
				}
				else
				{
					tWarpSpec.nLevelAreaDest = UnitGetLevelAreaID( pDestUnit );
					tWarpSpec.nLevelDepthDest = UnitGetLevelDepth( pDestUnit );
					tWarpSpec.nPVPWorld = ( PlayerIsInPVPWorld( pPlayer ) ? 1 : 0 );

				}

			}
			else
			{
			
				tWarpSpec.guidArriveAt = INVALID_GUID;
				CLEARBIT( tWarpSpec.dwWarpFlags, WF_ARRIVE_VIA_PARTY_PORTAL_BIT );
				if (AppIsHellgate())
				{
					tWarpSpec.nLevelDefDest = UnitGetStartLevelDefinition( pPlayer );
				}
				else
				{
					int nLevelAreaStart = INVALID_ID;
					int nLevelDepthStart = 0;
					VECTOR vPosition;
					PlayerGetRespawnLocation( pPlayer, KRESPAWN_TYPE_RETURNPOINT, nLevelAreaStart, nLevelDepthStart, vPosition );
					//UnitGetStartLevelAreaDefinition( pPlayer, nLevelAreaStart, nLevelDepthStart );					
					tWarpSpec.nLevelAreaDest = nLevelAreaStart;//LevelDefinitionGetRandom( NULL, nLevelAreaStart, nLevelDepthStart );
					tWarpSpec.nLevelDepthDest = nLevelDepthStart;
					tWarpSpec.nPVPWorld = ( PlayerIsInPVPWorld( pPlayer ) ? 1 : 0 );
				}
			
			}
						
		}
		else
		{
			// no dest unit in this game, forget trying to do this anymore
			tWarpSpec.guidArriveAt = INVALID_GUID;
			CLEARBIT( tWarpSpec.dwWarpFlags, WF_ARRIVE_AT_GUID_BIT );
		}
		
	}

	// search for level if we don't have a specific one to join yet
	if (pLevelDest == NULL)
	{
			
		if (AppIsHellgate())
		{

			// for now, find the level that first matches ... we need to change this
			// to allow multiple level instances, for instance, if we were to randomly
			// create mini levels called "mini level a", and you could warp into the
			// game into on this level instance, if there is more than one level 
			// instances in the game of this type it will break -Colin
			LEVEL_TYPE tLevelType = WarpSpecGetLevelType( tWarpSpec );
			pLevelDest = LevelFindFirstByType( pGame, tLevelType );

			// create level if it's not here yet	
			if (!pLevelDest)
			{

				// find and create root level
				s_LevelCreateRoot( pGame, tLevelType.nLevelDef, UnitGetId( pPlayer ) );
				
				// now search for the level again
				pLevelDest = LevelFindFirstByType( pGame, tLevelType );
				if (pLevelDest == NULL)
				{
					s_PlayerJoinLog( pGame, pPlayer, pClient->m_idAppClient, "s_LevelWarpIntoGame() Unable to find pLevelDest" );
					ASSERTX_RETFALSE( pLevelDest, "Unable to find level to warp into game at" );
				}
				
			} 
			
		}
		else
		{
		
			// for now, find the level that first matches ... we need to change this
			// to allow multiple level instances, for instance, if we were to randomly
			// create mini levels called "mini level a", and you could warp into the
			// game into on this level instance, if there is more than one level 
			// instances in the game of this type it will break -Colin
			int nLevelDefDest = LevelDefinitionGetRandom( 		
				NULL,
				tWarpSpec.nLevelAreaDest, 
				tWarpSpec.nLevelDepthDest );
			
			// find first one that matches ... we  --- somebody should finish writing this comment
			LEVEL_TYPE tLevelType = WarpSpecGetLevelType( tWarpSpec );
			pLevelDest = LevelFindFirstByType( pGame, tLevelType );

			// if no level found, add a new one
			if (!pLevelDest)
			{			

				// TODO: this seems odd, why are we setting the dungeon difficulty here, seems
				// like something to do during LevelAdd() and store with the level or something -Colin
				//int nDifficultyOffset = DungeonGetDifficulty( pGame, tWarpSpec.nLevelAreaDest, pPlayer );
				/*
				if( nDifficultyOffset < 0 )
				{
					nDifficultyOffset = UnitGetDifficultyOffset( pPlayer );
					DungeonSetDifficulty( pGame, tWarpSpec.nLevelAreaDest, nDifficultyOffset );
				}
				*/
				// fill out level spec
				LEVEL_SPEC tLevelSpec;
				tLevelSpec.tLevelType = WarpSpecGetLevelType( tWarpSpec );
				tLevelSpec.nDRLGDef = DungeonGetLevelDRLG( pGame, nLevelDefDest );
				tLevelSpec.dwSeed = DefinitionGetGlobalSeed();
				tLevelSpec.nDifficultyOffset = DungeonGetDifficulty( pGame, tWarpSpec.nLevelAreaDest, pPlayer );
				tLevelSpec.bPopulateLevel = TRUE;
				tLevelSpec.idActivator = UnitGetId( pPlayer );

				// add level			
				pLevelDest = LevelAdd( pGame, tLevelSpec );
				
			} 
			
		}

	}
	
	// get destination level id
	if (pLevelDest == NULL)
	{
		s_PlayerJoinLog( pGame, pPlayer, pClient->m_idAppClient, "s_LevelWarpIntoGame() Unable to find pLevelDest" );
		ASSERTX_RETFALSE( pLevelDest, "Expected level" );	
	}
	LEVELID idLevelDest = LevelGetID( pLevelDest );

	// save in level spec
	tWarpSpec.idLevelDest = idLevelDest;
	
	// find a level that is connected to the destination level that matches
	// the level definition they are leaving (if possible)
	LEVELID idLevelLeaving = s_sLevelGetConnectedInstanceOf( pLevelDest, nLevelDefLeaving, tWarpSpec.nLevelAreaPrev );

	// warp
	DWORD dwLevelWarpCommonFlags = 0;
	SETBIT( dwLevelWarpCommonFlags, LWCF_JOINING_GAME_BIT );
	BOOL bSuccess = s_sLevelWarpCommon( pPlayer, tWarpSpec, idLevelLeaving, dwLevelWarpCommonFlags );

	s_PlayerJoinLog( pGame, pPlayer, pClient->m_idAppClient, "s_LevelWarpIntoGame() Complete with bSuccess=%d", bSuccess );
	
	return bSuccess;
			
}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
BOOL s_LevelWarp(
	UNIT *pPlayer,
	WARP_SPEC &tSpec)
{
	ASSERTX_RETFALSE( pPlayer, "Expected player" );	
	
	// where are they coming from
	LEVEL *pLevelCurrent = UnitGetLevel( pPlayer );
	LEVELID idLevelCurrent = LevelGetID( pLevelCurrent );
		
	// player is warping within this game instance from their current level
	// to the level instance specified
	return s_sLevelWarpCommon( pPlayer, tSpec, idLevelCurrent, 0 );
}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BOOL s_LevelRecall(
	GAME * game,
	UNIT * unit,
	DWORD dwLevelRecallFlags /*= 0*/)
{
#if !ISVERSION( CLIENT_ONLY_VERSION )
	ASSERT_RETFALSE(game && IS_SERVER(game));
	ASSERT_RETFALSE(unit);

	// you cannot recall on levels where town portal are not allowed
	if (TownPortalIsAllowed( unit ) == TRUE || TESTBIT( dwLevelRecallFlags, LRF_FORCE ) )
	{
	
		// if we're booting, we heal dead or dying player cause this MUST succeed
		BOOL bDeadCanTransport = FALSE;
		if (TESTBIT( dwLevelRecallFlags, LRF_BEING_BOOTED ) &&
			IsUnitDeadOrDying( unit ) == TRUE)
		{
			s_PlayerRestart( unit, PLAYER_RESPAWN_RESURRECT_FREE );
			ASSERTV( IsUnitDeadOrDying( unit ) == FALSE, "Unit not healed for booting" );
			bDeadCanTransport = TRUE;  // just incase it didn't work, we still need people booted
		}
		
		// forbid players from recalling when they are dying
		if (IsUnitDeadOrDying( unit ) && bDeadCanTransport == FALSE)
		{
			return FALSE;
		}

		// setup warp spec
		WARP_SPEC tSpec;
		
		// being booted
		SETBIT( tSpec.dwWarpFlags, WF_BEING_BOOTED_BIT, TESTBIT( dwLevelRecallFlags, LRF_BEING_BOOTED ) );
		
		GAMECLIENT * client = ClientGetById(game, UnitGetClientId(unit));
		ASSERT_RETFALSE(client);
		if ( AppIsHellgate() )
		{
			
			// setup warp spec
			tSpec.nLevelDefDest = UnitGetReturnLevelDefinition( unit );
			SETBIT( tSpec.dwWarpFlags, WF_ARRIVE_AT_START_LOCATION_BIT );
			
			// warp
			return s_WarpToLevelOrInstance( unit, tSpec );
			
		}
		else
		{
			LEVEL *pLevelCurrent = UnitGetLevel( unit );
			
			// setup warp spec
			tSpec.nLevelAreaDest = INVALID_ID;// pLevelCurrent ? LevelGetLevelAreaID( pLevelCurrent ) : 1;
			tSpec.nLevelDepthDest = 0;
			tSpec.nPVPWorld = ( PlayerIsInPVPWorld( unit ) ? 1 : 0 );

			// in the 'primary only' case, we are using an anchorstone, and don't want to touch
			// the secondary respawn locations
			if( TESTBIT( dwLevelRecallFlags, LRF_PRIMARY_ONLY ) )
			{
				SETBIT( tSpec.dwWarpFlags, WF_USED_RECALL );
				VECTOR vPosition;
				PlayerGetRespawnLocation( unit, KRESPAWN_TYPE_ANCHORSTONE, tSpec.nLevelAreaDest, tSpec.nLevelDepthDest, vPosition );
			}
			//if we aren't in dungeon these should be valid for a player
			if( tSpec.nLevelAreaDest == INVALID_ID )
			{
				tSpec.nLevelAreaDest = UnitGetStat( unit, STATS_RESPAWN_LEVELAREA );
				tSpec.nLevelDepthDest = UnitGetStat( unit, STATS_RESPAWN_LEVELDEPTH );
				tSpec.nPVPWorld = ( PlayerIsInPVPWorld( unit ) ? 1 : 0 );
			}
			if( tSpec.nLevelAreaDest == INVALID_ID )
			{
				VECTOR vPosition;
				PlayerGetRespawnLocation( unit, KRESPAWN_TYPE_ANCHORSTONE, tSpec.nLevelAreaDest, tSpec.nLevelDepthDest, vPosition );
			}

			//if we get to this, well it's probably a bad thing..
			ASSERT( tSpec.nLevelAreaDest != INVALID_ID );
			if( tSpec.nLevelAreaDest == INVALID_ID )
			{
				
				tSpec.nLevelAreaDest = pLevelCurrent ? LevelGetLevelAreaID( pLevelCurrent ) : 1;
				tSpec.nLevelDepthDest = 0;
				tSpec.nPVPWorld = ( PlayerIsInPVPWorld( unit ) ? 1 : 0 );
			}
			
			// warp!
			return s_WarpToLevelOrInstance( unit, tSpec );
			
		}

	}
	
	return FALSE;
#else
	return TRUE;	
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )
	
}

//----------------------------------------------------------------------------
// Here so we can do a delayed level recall, giving the player some time
// to savor his wins or making it hard to instant teleport by joining
// and leaving parties quickly.
//----------------------------------------------------------------------------
BOOL s_LevelRecallEvent(
	GAME * game,
	UNIT * unit,
	const EVENT_DATA &tData)
{
	return s_LevelRecall(game, unit, DWORD(tData.m_Data1) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int LevelGetFirstPortalAndRecallLocation(
	GAME *pGame)
{
	int nLevelDef = INVALID_LINK;
	int nLevelDefFallback = INVALID_LINK;
	
	int nNumLevels = ExcelGetNumRows( pGame, DATATABLE_LEVEL );
	for (int i = 0; i < nNumLevels; ++i)
	{
		const LEVEL_DEFINITION *pLevelDef = LevelDefinitionGet( i );
		if (pLevelDef)
		{
		
			// save level def that is marked as first portal and recall loc
			if (pLevelDef->bFirstPortalAndRecallLoc)
			{
				nLevelDef = i;
			}
			
			// as a fallback, we search for any available portal and recall loc
			if (nLevelDefFallback == INVALID_LINK)
			{
				if (pLevelDef->bPortalAndRecallLoc == TRUE)
				{
					nLevelDefFallback = i;
				}
			}
			
		}
	}
	
	// if we didn't find one, just pick the first one recall location found at all
	if (nLevelDef != INVALID_LINK)
	{
		return nLevelDef;
	}
	
	// return the default fallback found
	return nLevelDefFallback;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sIsLevelInArray(
	LEVEL *pLevel,
	LEVEL_CONNECTION *pBuffer,
	int nBufferCount)
{
	ASSERTX_RETFALSE( pLevel, "Expected level" );
	ASSERTX_RETFALSE( pBuffer, "Expected buffer" );	
	
	// search for level in buffer, do not add duplicates
	for (int i = 0; i < nBufferCount; ++i)
	{
		if (pBuffer[ i ].pLevel == pLevel)
		{
			return TRUE;
		}
	}
	
	return FALSE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sAddLevelToArray(
	LEVEL *pLevel,
	UNITID idWarpToLevel,
	int nCurrentCount,
	LEVEL_CONNECTION *pBuffer,
	int nBufferSize)
{
	ASSERTX_RETZERO( pLevel, "Expected level" );
	ASSERTX_RETZERO( pBuffer, "Expected level storage" );
	ASSERTX_RETZERO( nBufferSize > 0, "Invalid buffer size" );

	// search for level in buffer, do not add duplicates
	if (sIsLevelInArray( pLevel, pBuffer, nCurrentCount ) == TRUE)
	{
		return FALSE;
	}

	// must be able to fit
	ASSERTX_RETFALSE( nCurrentCount < nBufferSize, "Buffer too small" );
	pBuffer[ nCurrentCount ].pLevel = pLevel;
	pBuffer[ nCurrentCount ].idWarp = idWarpToLevel;
	
	// has been added
	return TRUE;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int s_sLevelGetConnectedChain( 
	LEVEL *pLevel, 
	UNITID idWarpToLevel,
	int nCurrentCount,
	LEVEL_CONNECTION *pConnectedLevels,
	int nMaxConnectedLevels)
{
	ASSERTX_RETZERO( pLevel, "Expected level" );
	ASSERTX_RETZERO( pConnectedLevels, "Expected level storage" );
	ASSERTX_RETZERO( nMaxConnectedLevels > 0, "Invalid buffer size" );
		
	// add this level
	if (!sAddLevelToArray( pLevel, idWarpToLevel, nCurrentCount, pConnectedLevels, nMaxConnectedLevels ))
	{
		return nCurrentCount;
	}

	// we have one more level
	nCurrentCount++;
		
	// go through level links
	GAME *pGame = LevelGetGame( pLevel );
	for (int i = 0; i < pLevel->m_nNumLevelLinks; ++i)
	{
		const LEVEL_LINK *pLink = &pLevel->m_LevelLink[ i ];
		LEVEL *pLevelConnected = LevelGetByID( pGame, pLink->idLevel );
		if (sIsLevelInArray( pLevelConnected, pConnectedLevels, nCurrentCount) == FALSE)
		{
			nCurrentCount = s_sLevelGetConnectedChain( 
				pLevelConnected,
				pLink->idUnit,
				nCurrentCount, 
				pConnectedLevels, 
				nMaxConnectedLevels);
		}
	}

	// return how many we found
	return nCurrentCount;
				
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int s_LevelGetConnectedLevels( 
	LEVEL* pLevel, 
	UNITID idWarpToLevel,
	LEVEL_CONNECTION* pConnectedLevels, 
	int nMaxConnectedLevels,
	BOOL bFollowEntireChain)
{
	ASSERTX_RETZERO( pLevel, "Expected Level" );
	ASSERTX_RETZERO( pConnectedLevels, "Invalid connected level params" );
	GAME *pGame = LevelGetGame( pLevel );
	int nNumConnected = 0;
		
	// do the whole level chain if desired
	if (bFollowEntireChain == TRUE)
	{
		nNumConnected = s_sLevelGetConnectedChain( 
			pLevel, 
			idWarpToLevel,
			nNumConnected, 
			pConnectedLevels, 
			nMaxConnectedLevels);
	}
	else
	{
	
		// add this level
		if (!sAddLevelToArray( pLevel, idWarpToLevel, nNumConnected, pConnectedLevels, nMaxConnectedLevels ))
		{
			return nNumConnected;  // no room for more, just stop where we are
		}

		// we now have one more level
		nNumConnected++;
				
		// go through each warp in this level
		for (int i = 0; i < pLevel->m_nNumLevelLinks; ++i)

		{
			const LEVEL_LINK *pLink = &pLevel->m_LevelLink[ i ];
			LEVEL *pLevelConnected = LevelGetByID( pGame, pLink->idLevel );
			if (!sAddLevelToArray( pLevelConnected, pLink->idUnit, nNumConnected, pConnectedLevels, nMaxConnectedLevels ))
			{
				break;  // no room for more, just stop where we are
			}		
			
			// we now have one more in the array
			nNumConnected++;
			
		}
	
	}
			
	// return number of connected levels
	return nNumConnected;
			
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int s_LevelGetNumDistricts( 
	const LEVEL* pLevel)
{
	return pLevel->m_nNumDistricts;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DISTRICT* s_LevelGetRandomDistrict( 
	GAME* pGame,
	const LEVEL* pLevel)
{
	ASSERTX_RETNULL( pLevel, "Expected level" );
	
	int nNumDistricts = s_LevelGetNumDistricts( pLevel );
	ASSERTX_RETNULL( nNumDistricts > 0, "Must have at least one district" );
	int nDistrict = RandGetNum( pGame, 0, nNumDistricts - 1 );
	return pLevel->m_pDistrict[ nDistrict ];
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_LevelOrganizeDistricts(
	GAME* pGame,
	LEVEL* pLevel)
{
	ASSERTX_RETURN( pGame, "Expected game" );
	ASSERTX_RETURN( pLevel, "Expected level" );	
	
	// approximately how much area will each district consist of
	int nNumDistricts = s_LevelGetNumDistricts( pLevel );
	ASSERTX_RETURN( nNumDistricts >= 1, "Level must have at least one district" );
	float flTotalLevelArea = LevelGetArea( pLevel );
	ASSERTX_RETURN( flTotalLevelArea > 0.0f, "Can't setup district, level has no area/rooms" );
	float flDistrictArea = flTotalLevelArea / (float)nNumDistricts;

	// grow each district
	for (int i = 0; i < nNumDistricts; ++i)
	{
		DISTRICT* pDistrict = pLevel->m_pDistrict[ i ];
		DistrictGrow( pGame, pDistrict, flDistrictArea );
	}
					
	// assign any leftover rooms to a random district
	for (ROOM* pRoom = LevelGetFirstRoom( pLevel ); pRoom; pRoom = LevelGetNextRoom( pRoom ))
	{
		if (RoomGetDistrict( pRoom ) == NULL)
		{
			DISTRICT* pDistrict = s_LevelGetRandomDistrict( pGame, pLevel );
			DistrictAddRoom( pDistrict, pRoom );
		}

		//DISTRICT* pDistrict = RoomGetDistrict( pRoom );		
		//int nSpawnClass  = DistrictGetSpawnClass( pDistrict );
		//trace(  "Room '%s' is in district with spawn class '%s'\n", pRoom->pDefinition->tHeader.pszName, ExcelGetStringIndexByLine( pGame, DATATABLE_SPAWN_CLASS, nSpawnClass ));
		
	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
typedef int (*FN_SELECT_RANDOM_SPAWNCLASS)(GAME * pGame, const LEVEL * pLevel);

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int s_sLevelSelectRandomSpawnableMonsterFromSpawnClass(
	GAME * pGame,
	const LEVEL * pLevel,
	SPAWN_CLASS_EVALUATE_FILTER pfnFilter,
	int nMonsterExperienceLevel,
	int nLevelDepth,
	int nSpawnClass,
	FN_SELECT_RANDOM_SPAWNCLASS pfnSelectRandomSpawnClass = NULL)
{
	int nMonsterClassToSpawn = INVALID_LINK;

	// what kind of monsters can be spawned on this level
	int nTries = 64;
	SPAWN_ENTRY tSpawns[MAX_SPAWNS_AT_ONCE];	
	int nSpawnCount = 0;
	do 
	{
		int nSpawnClassToUse = nSpawnClass;
		if(pfnSelectRandomSpawnClass)
		{
			nSpawnClassToUse = pfnSelectRandomSpawnClass(pGame, pLevel);
		}
		// evaluate a random spawn class in the level
		nSpawnCount = SpawnClassEvaluate( 
			pGame, 
			LevelGetID( pLevel ), 
			nSpawnClassToUse, 
			nMonsterExperienceLevel, 
			tSpawns, 
			pfnFilter,
			nLevelDepth);
	}
	while (nSpawnCount == 0 && nTries-- > 0);

	if (nSpawnCount > 0)
	{	
		// pick one of the monster classes available
		int nPick = RandGetNum( pGame, 0, nSpawnCount - 1 );
		ASSERTX( nPick >= 0 && nPick < nSpawnCount, "Invalid pick when creating task to kill special" );

		SPAWN_ENTRY* pSpawn = &tSpawns[ nPick ];		
		nMonsterClassToSpawn = pSpawn->nIndex;
	}

	return nMonsterClassToSpawn;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int s_LevelSelectRandomSpawnableInteractableMonster(
	GAME* pGame,
	const LEVEL* pLevel,
	SPAWN_CLASS_EVALUATE_FILTER pfnFilter)
{
	if(pLevel->m_nInteractableSpawnClass < 0)
		return INVALID_ID;

	// what monster level are monsters here
	int nMonsterExperienceLevel = LevelGetMonsterExperienceLevel( pLevel );	

	return s_sLevelSelectRandomSpawnableMonsterFromSpawnClass(pGame, pLevel, pfnFilter, nMonsterExperienceLevel, INVALID_ID, pLevel->m_nInteractableSpawnClass);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int s_sLevelSelectRandomSpawnableMonsterGetSpawnClass(
	GAME * pGame,
	const LEVEL * pLevel)
{
	DISTRICT* pDistrict = s_LevelGetRandomDistrict( pGame, pLevel );
	return DistrictGetSpawnClass( pDistrict );				
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int s_LevelSelectRandomSpawnableMonster(
	GAME* pGame,
	const LEVEL* pLevel,
	SPAWN_CLASS_EVALUATE_FILTER pfnFilter)
{
	// what monster level are monsters here
	int nMonsterExperienceLevel = LevelGetMonsterExperienceLevel( pLevel );	

	return s_sLevelSelectRandomSpawnableMonsterFromSpawnClass(pGame, pLevel, pfnFilter, nMonsterExperienceLevel, INVALID_ID, INVALID_ID, s_sLevelSelectRandomSpawnableMonsterGetSpawnClass);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int s_LevelSelectRandomSpawnableQuestMonster(
	GAME* pGame,
	const LEVEL* pLevel,
	int nMonsterExperienceLevel,
	int nLevelDepth,
	SPAWN_CLASS_EVALUATE_FILTER pfnFilter)
{
	return s_sLevelSelectRandomSpawnableMonsterFromSpawnClass(pGame, pLevel, pfnFilter, nMonsterExperienceLevel, nLevelDepth, pLevel->m_nQuestSpawnClass);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)

void c_SetEnvironmentDef( GAME * pGame, int nRegion, const LEVEL_ENVIRONMENT_DEFINITION * pEnvDefintion, const char * pszEnvSuffix, BOOL bForceSyncLoad /*= FALSE*/,BOOL bTransition /*= FALSE*/ )
{
	ASSERT_RETURN( pEnvDefintion );

	// Environment

	// load environment definition for this level
	OVERRIDE_PATH tOverride;
	char szFilePathBackup[ DEFAULT_FILE_WITH_PATH_SIZE ] = "";
	char szPath			 [ DEFAULT_FILE_WITH_PATH_SIZE ] = "";
	OverridePathByLine( pEnvDefintion->nPathIndex, &tOverride );
	ASSERT( tOverride.szPath[ 0 ] );
	PStrRemovePath( szPath, DEFAULT_FILE_WITH_PATH_SIZE, FILE_PATH_BACKGROUND, tOverride.szPath );

	char szFilePath[ DEFAULT_FILE_WITH_PATH_SIZE ] = "";	
	PStrPrintf( szFilePath, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s%s%s", szPath, pEnvDefintion->pszEnvironmentFile, pszEnvSuffix, ENVIRONMENT_SUFFIX );
	PStrPrintf( szFilePathBackup, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s%s", szPath, pEnvDefintion->pszEnvironmentFile, ENVIRONMENT_SUFFIX );
	
	int nEnvDef = INVALID_ID;
	if(pszEnvSuffix && pszEnvSuffix[0])
	{
		// It's okay not to have the override...
		nEnvDef = e_EnvironmentLoad( szFilePath, bForceSyncLoad, TRUE );
		if ( nEnvDef == INVALID_ID )
		{
			// ...but if we don't have the backup, then we're in trouble
			nEnvDef = e_EnvironmentLoad( szFilePathBackup, bForceSyncLoad, FALSE );
			ASSERT_RETURN(nEnvDef != INVALID_ID);
		}
	}
	else
	{
		nEnvDef = e_EnvironmentLoad( szFilePath, bForceSyncLoad );
	}

	c_SetEnvironmentDef( nRegion, szFilePath, szFilePathBackup, bTransition, bForceSyncLoad );

	if ( bForceSyncLoad )
	{
		ASSERTV( e_RegionGet( nRegion ), "Region %d doesn't exist!", nRegion );
		ASSERTV( e_GetCurrentEnvironmentDefID( nRegion ) != INVALID_ID, "Current environment ID for region %d invalid!", nRegion );
		ENVIRONMENT_DEFINITION * pEnvDef = e_GetCurrentEnvironmentDef( nRegion );
		ASSERTV( pEnvDef, "Environment sync load failed!\n\nFilepath: %s\nBackup:   %s\nRegion:   %d", szFilePath, szFilePathBackup, nRegion );
	}
	//e_SetLevelLocationRenderFlags( pEnvDef->nLocation );
	//e_LoadAllShadersOfType( pEnvDef->nLocation );  // done in RestoreEnvironmentDef()
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_SetEnvironmentDef( GAME * pGame, int nRegion, int nEnvDefinition, const char * pszEnvSuffix, BOOL bForceSyncLoad /*= FALSE*/, BOOL bTransition /*= FALSE*/ )
{
	const LEVEL_ENVIRONMENT_DEFINITION * pEnvDefintion = ( const LEVEL_ENVIRONMENT_DEFINITION * )ExcelGetData( pGame, DATATABLE_LEVEL_ENVIRONMENTS, nEnvDefinition );
	c_SetEnvironmentDef( pGame, nRegion, pEnvDefintion, pszEnvSuffix, bForceSyncLoad, bTransition );
}

//----------------------------------------------------------------------------
//when the level loads on the client it stores all the warps so they can be hidden or set visible
//----------------------------------------------------------------------------
void c_LevelFilloutLevelWarps( LEVEL *pLevel )
{
	ASSERT_RETURN( pLevel );
	ASSERTV_RETURN( pLevel->m_nNumLevelLinks == 0, "Level Warps already filled out" );
	GAME *pGame = LevelGetGame( pLevel );
	ASSERT_RETURN( pGame );
	UNIT *pPlayer = GameGetControlUnit( pGame );
	ASSERT_RETURN( pPlayer );
	for ( ROOM * pRoom = LevelGetFirstRoom(pLevel); pRoom; pRoom = LevelGetNextRoom(pRoom) )	
	{
		for ( UNIT * pWarp = pRoom->tUnitList.ppFirst[TARGET_OBJECT]; pWarp; pWarp = pWarp->tRoomNode.pNext )
		{
			LevelAddWarp( pWarp, UnitGetId( pPlayer ), pWarp->pUnitData->eWarpResolvePass );		
		}
	}
	

}

//----------------------------------------------------------------------------
//returns TRUE or FALSE if it is a warp and if it needs to be invisible or not - Default is true
//----------------------------------------------------------------------------
BOOL c_LevelEntranceVisible( UNIT *pWarp )
{
	ASSERT_RETFALSE( pWarp );
	if( !UnitIsA( pWarp, UNITTYPE_WARP ) )
		return TRUE;	//if it's not a entrance then it doesn't care if it's visible or not
	/*
	UNIT *pPlayer = GameGetControlUnit( UnitGetGame( pWarp ) );
	
	if( pWarp &&
		UnitGetWarpToLevelAreaID( pWarp ) != INVALID_ID)
	{
		//if the level always shows just always show the entrance or if not, the player has to have a map to it.		
		if( MYTHOS_LEVELAREAS::DoesLevelAreaAlwaysShow( UnitGetWarpToLevelAreaID( pWarp ) ) ||
			( pPlayer && MYTHOS_MAPS::PlayerHasMapForAreaByAreaID( pPlayer, UnitGetWarpToLevelAreaID( pWarp ) )) )
		{				
			return TRUE;
		}
		else
		{
			return FALSE;
		}
	}
	*/
	return TRUE;
}

//----------------------------------------------------------------------------
//when we enter a level dungeon entrances can be invisible. Only when a player
//has a map to that level will it show.
//----------------------------------------------------------------------------
void c_LevelResetDungeonEntranceVisibility( LEVEL *pLevel,
										   UNIT *pPlayer )
{
	
	if( !AppIsTugboat() ) //tugboat only
		return;

	ASSERT_RETURN( pLevel );
	ASSERT_RETURN( pPlayer );
	GAME *pGame = UnitGetGame( pPlayer );

	for( int t = 0; t < pLevel->m_nNumLevelLinks; t++ )
	{
		UNIT *pWarp = UnitGetById( pGame, pLevel->m_LevelLink[t].idUnit );
		if( pWarp )
		{
			if( c_LevelEntranceVisible( pWarp ) )
			{
				//it is visible
				c_UnitSetNoDraw( pWarp, FALSE ); 
			}
			else
			{
				//no map and it's not visible always
				c_UnitSetNoDraw( pWarp, TRUE );
			}
		}
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_LevelInit( 
	GAME *pGame, 
	int nLevelDefinition,
	int nEnvDefinition,
	int nRandomTheme)
{
	ASSERT( pGame );
	ASSERT( IS_CLIENT( pGame ) );

// #ifdef HAMMER
	if (AppIsHammer())
	{
		return;
	}
// #endif

	const LEVEL_DEFINITION * pLevelDefinition = LevelDefinitionGet( nLevelDefinition );
	ASSERT( pLevelDefinition );

	const LEVEL_THEME * pLevelTheme = NULL;
	if(nRandomTheme >= 0)
	{
		pLevelTheme = (const LEVEL_THEME *)ExcelGetData(pGame, DATATABLE_LEVEL_THEMES, nRandomTheme);
	}

	V( e_TextureBudgetUpdate() );

	// THIS NEEDS TO BE FIXED [-- how? better region specification?]
	V( e_SetRegion( 0 ) );
	c_SetEnvironmentDef( pGame, 0, nEnvDefinition, (pLevelTheme ? pLevelTheme->pszEnvironment : ""), TRUE );

	// check that the environment was sync loaded
	ASSERT( e_GetCurrentEnvironmentDef() );
	ASSERT( e_GetCurrentLevelLocation() != LEVEL_LOCATION_UNKNOWN );


	/*
#ifdef _DEBUG
	if ( ! GlobalNoPreload() )
#endif
	{
		PreloadQuestSounds(pGame, pLevelDefinition->pszName);
	}
	// */

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_SetEnvironmentDef( int nRegion, const char * pszFilePath, const char * pszFilePathBackup, BOOL bTransition, BOOL bForceSyncLoad /*= FALSE*/ )
{
	e_SetEnvironmentDef( nRegion, pszFilePath, pszFilePathBackup, bTransition, bForceSyncLoad );
}
#endif //!SERVER_VERSION

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// this is server only, because only the server has which drlg choice was made.
// the resuts get sent to the client, but the server has all of the info
int s_LevelGetEnvironmentIndex(
	GAME * pGame,
	LEVEL * pLevel )
{
	ASSERTX_RETVAL( pGame, INVALID_ID, "Expected Game" );
	ASSERTX_RETVAL( pLevel, INVALID_ID, "Expected Level" );
	ASSERTX_RETVAL( IS_SERVER( pGame ), INVALID_ID, "Server only" );

	int nLevelDefinition = pLevel->m_nLevelDef;

	DUNGEON * pDungeon = pGame->m_Dungeon;
	ASSERT_RETINVALID( pDungeon );

	ASSERT_RETINVALID( nLevelDefinition != INVALID_LINK );

	const LEVEL_DRLG_CHOICE *pDRLGChoice = &pDungeon->m_DungeonSelections[ nLevelDefinition ].m_tDLRGChoice;
	ASSERT_RETINVALID( pDRLGChoice );

	if ( pDRLGChoice->nEnvironmentOverride != INVALID_LINK )
		return pDRLGChoice->nEnvironmentOverride;

	const DRLG_DEFINITION * pDRLGDef = LevelGetDRLGDefinition( pLevel );
	ASSERT_RETINVALID( pDRLGDef );
	return pDRLGDef->nEnvironment;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
LEVEL_TYPE LevelGetType(
	const LEVEL *pLevel)
{
	ASSERTX_RETVAL( pLevel, gtLevelTypeAny, "Expected level" );
	LEVEL_TYPE tLevelType;
	if (AppIsHellgate())
	{
		tLevelType.nLevelDef = LevelGetDefinitionIndex( pLevel );
	}
	else if (AppIsTugboat())
	{
		tLevelType.nLevelArea = LevelGetLevelAreaID( pLevel );
		tLevelType.nLevelDepth = LevelGetDepth( pLevel );
		tLevelType.bPVPWorld = LevelGetPVPWorld( pLevel );
	}
	return tLevelType;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL LevelTypeIsValid(
	const LEVEL_TYPE &tLevelType)
{
	if (AppIsHellgate())
	{
		return tLevelType.nLevelDef != INVALID_LINK;
	}
	else if (AppIsTugboat())
	{
		return tLevelType.nLevelArea != INVALID_LINK &&
			   tLevelType.nLevelDepth >= 0; //&& 
			   //tLevelType.nLevelDepth <= MYTHOS_LEVELAREAS::LevelAreaGetLevelMaxDepth( tLevelType.nLevelArea );
	}
	return FALSE;
}
		
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int LevelGetDefinitionIndex(
	const LEVEL * level)
{
	ASSERT_RETINVALID(level);
	return level->m_nLevelDef;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
unsigned int LevelGetDefinitionCount(
	void)
{
	return ExcelGetNumRows(NULL, DATATABLE_LEVEL);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SRVLEVELTYPE LevelGetSrvLevelType(
	const LEVEL *pLevel)
{
	ASSERTX_RETVAL( pLevel, SVRLEVELTYPE_INVALID, "Expected level" );
	int nLevelDef = LevelGetDefinitionIndex( pLevel );
	return LevelDefinitionGetSrvLevelType( nLevelDef );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SRVLEVELTYPE LevelDefinitionGetSrvLevelType(
	int nLevelDef)
{
	ASSERTX_RETVAL( nLevelDef != INVALID_LINK, SVRLEVELTYPE_INVALID, "Expected level def" );
	const LEVEL_DEFINITION *pLevelDef = LevelDefinitionGet( nLevelDef );
	if (pLevelDef)
	{
		return pLevelDef->eSrvLevelType;
	}
	return SVRLEVELTYPE_INVALID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int LevelGetDefinitionIndex(
	const char * level_name)
{
	return ExcelGetLineByStringIndex(NULL, DATATABLE_LEVEL, level_name);
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int LevelDefinitionGetRandom(	
    LEVEL *pLevel,
	int nLevelAreaID,
	int nDepth )
{

	return MYTHOS_LEVELAREAS::LevelAreaGetDefRandom( pLevel, nLevelAreaID, nDepth );
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const LEVEL_DEFINITION * LevelDefinitionGet(
	int level_definition)
{
	if (level_definition == INVALID_LINK)
	{
		return NULL;
	}
	return (const LEVEL_DEFINITION *)ExcelGetData(NULL, DATATABLE_LEVEL, level_definition);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const LEVEL_DEFINITION * LevelDefinitionGet(
	const LEVEL * level)
{	
	ASSERT_RETNULL(level);
	int level_definition = LevelGetDefinitionIndex(level);
	return LevelDefinitionGet(level_definition);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const LEVEL_DEFINITION * LevelDefinitionGet(
	const char * level_name)
{
	ASSERT_RETNULL(level_name);
	return LevelDefinitionGet(LevelGetDefinitionIndex(level_name));
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const DRLG_DEFINITION* DRLGDefinitionGet(
	int nDRLGDefinition)
{
	ASSERTX_RETNULL((unsigned int)nDRLGDefinition < ExcelGetNumRows(EXCEL_CONTEXT(), DATATABLE_LEVEL_DRLGS ), "Invalid DRLGDefinitionGet() params");
	return (const DRLG_DEFINITION*)ExcelGetData( NULL, DATATABLE_LEVEL_DRLGS, nDRLGDefinition );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const DRLG_DEFINITION * LevelGetDRLGDefinition(
	const LEVEL * pLevel )
{
	ASSERT_RETNULL( pLevel );
	return DRLGDefinitionGet( pLevel->m_nDRLGDef );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int LevelGetDepth(
	const LEVEL* pLevel)
{
	ASSERT_RETINVALID(pLevel);
	return pLevel->m_nDepth;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int LevelGetPVPWorld(
	const LEVEL* pLevel)
{
	ASSERT_RETINVALID(pLevel);
	return pLevel->m_bPVPWorld;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int LevelGetDifficultyOffset(
	const LEVEL* pLevel)
{
	ASSERT_RETINVALID(pLevel);
	if( LevelIsSafe( pLevel ) )
	{
		return 0;
	}
	return pLevel->m_nDifficultyOffset;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int LevelGetDifficulty(
	const LEVEL* pLevel)
{
	ASSERT_RETINVALID(pLevel);
	return MYTHOS_LEVELAREAS::LevelAreaGetDifficulty( pLevel );
	//const LEVEL_AREA_DEFINITION* pLevelDef = LevelAreaDefinitionGet( LevelGetLevelArea( pLevel ) );
	//return 1 + (int)FLOOR( (float)( LevelGetDepth( pLevel ) - 1 ) * pLevelDef->fDifficultyScaleByLevel +  (float)( LevelGetDifficultyOffset( pLevel ) ) * .75f );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
GAME *LevelGetGame(
	const LEVEL* pLevel)
{
	ASSERTX_RETNULL( pLevel, "Expected level" );
	return pLevel->m_pGame;
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
LEVELID LevelGetID(
	const LEVEL *pLevel)
{
	ASSERTX_RETVAL( pLevel, INVALID_ID, "Expected Level" );
	return pLevel->m_idLevel;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)

int c_LevelCreatePaperdollLevel( 
	int nPlayerClass )
{
	const UNIT_DATA * pUnitData = PlayerGetData( NULL, nPlayerClass );
	if ( ! pUnitData )
		return INVALID_ID;

	GAME * pGame = AppGetCltGame();
	int nLevelDef = pUnitData->nPaperdollBackgroundLevel;
	int nDRLG = DungeonGetLevelDRLG( pGame, nLevelDef, TRUE );

	LEVEL_TYPE tLevelType;
	if( AppIsHellgate() )
	{
		tLevelType.nLevelDef = nLevelDef;
	}
	else if( AppIsTugboat() )
	{
		tLevelType.nLevelArea = 0;
		tLevelType.nLevelDepth = 0;
		tLevelType.bPVPWorld = FALSE;
	}
	LEVEL * pLevel = LevelFindFirstByType( pGame, tLevelType );
	if ( ! pLevel )
	{
		V( e_TextureBudgetUpdate() );
		// FIX THIS
		int nEnvironment = DungeonGetDRLGBaseEnvironment( nDRLG );
		REF( nEnvironment );
	
		c_SetEnvironmentDef( pGame, 0, nEnvironment, "", TRUE );
		if (AppIsHellgate())
		{
			e_SetRegion( 0 );	// this fixes the "stuck in loading screen due to NULL region 1 after restarting game" bug, 20902 -cmarch
		}

		// fill out common level spec information
		LEVEL_SPEC tSpec;
		tSpec.tLevelType.nLevelDef = nLevelDef;
		tSpec.nDRLGDef = nDRLG;
		tSpec.bClientOnlyLevel = TRUE;

		// tugboat specific info		
		if (AppIsTugboat())
		{
			tSpec.tLevelType.nLevelArea = 0;  // is this supposed to really be hardcoded?? -Colin
		}

		// create level
		pLevel = LevelAdd( pGame, tSpec );
		
		if ( pLevel )
		{
			DRLGCreateLevel( pLevel->m_pGame, pLevel, pLevel->m_dwSeed, INVALID_ID );
		}
		
	}

	return pLevel ? pLevel->m_idLevel : INVALID_ID;
}
#endif //!ISVERSION(SERVER_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LevelTypeInit(
	LEVEL_TYPE &tLevelType)
{
	tLevelType.nLevelDef = INVALID_LINK;
	tLevelType.nLevelArea = INVALID_LINK;
	tLevelType.nLevelDepth = 0;
	tLevelType.bPVPWorld = FALSE;
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
LEVEL *LevelFindFirstByType(
	GAME* pGame,
	const LEVEL_TYPE &tLevelType)
{

	int nNumLevels = DungeonGetNumLevels( pGame );
	for (int i = 0; i < nNumLevels; ++i)
	{
		LEVELID idLevelOther = (LEVELID)i;
		LEVEL *pLevelOther = LevelGetByID( pGame, idLevelOther );
		if( pLevelOther )
		{
			LEVEL_TYPE tLevelTypeOther = LevelGetType( pLevelOther );
			if (LevelTypesAreEqual( tLevelTypeOther, tLevelType ))
			{
				return pLevelOther;
			}			
		}		
	}
	
	return NULL;  // not found
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL LevelTypesAreEqual(
	const LEVEL_TYPE &tLevelType1,
	const LEVEL_TYPE &tLevelType2)
{
	if( AppIsHellgate() )
	{
		if ( tLevelType1.nLevelDef == tLevelType2.nLevelDef &&
			 tLevelType1.nLevelArea == tLevelType2.nLevelArea )
		{
			return TRUE;
		}
	}
	else if( AppIsTugboat() )
	{
		if ( tLevelType1.nLevelArea == tLevelType2.nLevelArea &&
			 tLevelType1.nLevelDepth == tLevelType2.nLevelDepth &&
			 tLevelType1.bPVPWorld == tLevelType2.bPVPWorld )
		{
			return TRUE;
		}
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL LevelGetLevelType(
	const LEVEL *pLevel,
	LEVEL_TYPE &tLevelType)
{
	ASSERTV_RETFALSE( pLevel, "Expected level" );
	
	// init result
	LevelTypeInit( tLevelType );
	
	if (AppIsHellgate())
	{
		tLevelType.nLevelDef = LevelGetDefinitionIndex( pLevel );
	}
	else
	{
		tLevelType.nLevelArea = LevelGetLevelAreaID( pLevel );
		tLevelType.nLevelDepth = LevelGetDepth( pLevel );
		tLevelType.nAreaLevel = pLevel->m_nAreaLevel;
		tLevelType.bPVPWorld = LevelGetPVPWorld( pLevel );
		//tLevelType.nItemKnowingArea = ?;
	}

	return TRUE;	
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float LevelGetArea( 
	LEVEL* pLevel)
{
	ASSERTX_RETZERO( pLevel, "Expected level" );
	return pLevel->m_flArea;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL LevelDepthGetDisplayName( 
	GAME* pGame,
	int nLevelArea,
	int nLevelDefDest,
	int nLevelDepth,
	WCHAR *puszBuffer,
	int nBufferSize,
	BOOL bIncludeQuests )
{	
	BOOL returnValue( FALSE );
	if( AppIsTugboat())
	{
		if ( MYTHOS_LEVELAREAS::LevelAreaDontDisplayDepth( nLevelArea, nLevelDepth ) == FALSE )
		{
			int nLevelNameOverride = MYTHOS_LEVELAREAS::LevelAreaGetDefRandom( NULL, nLevelArea, nLevelDepth );			
			const LEVEL_DEFINITION *pLevelDef = LevelDefinitionGet( nLevelNameOverride ); 
			ASSERT_RETFALSE( pLevelDef );
			if( nLevelDepth == MYTHOS_LEVELAREAS::LevelAreaGetMaxDepth( nLevelArea ) - 1 )
			{				
				if( pLevelDef->nFinalFloorSuffixName != INVALID_ID )
				{					
					PStrPrintf( puszBuffer, nBufferSize, StringTableGetStringByIndex( pLevelDef->nFinalFloorSuffixName ) );
				}
				else
				{
					const WCHAR *puszFormat = GlobalStringGet( GS_LEVEL_FINAL );
					PStrPrintf( puszBuffer, nBufferSize, puszFormat );
				}
				returnValue = TRUE;
			}
			else
			{
				int displayDepth = MYTHOS_LEVELAREAS::LevelAreaGetLevelDepthDisplay( nLevelArea, nLevelDepth );
				const WCHAR *puszFormat( NULL );
				if( pLevelDef->nFloorSuffixName != INVALID_ID )
				{					
					puszFormat = StringTableGetStringByIndex( pLevelDef->nFloorSuffixName );
				}
				else
				{
					puszFormat = GlobalStringGet( GS_LEVEL_WITH_INTEGER );
				}				
				returnValue = PStrLen( puszFormat ) > 0;
				PStrPrintf( puszBuffer, nBufferSize, puszFormat, displayDepth );
			}		

		}
		if( bIncludeQuests )
		{
			UNIT *pUnit = GameGetControlUnit( pGame );
			if( pUnit && nLevelArea >= 0)
			{			
				for( int d = 0; d < MAX_ACTIVE_QUESTS_PER_UNIT; d++ )
				{
					const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask = QuestGetUnitTaskByQuestQueueIndex(pUnit, d );

					if( pQuestTask &&
						QuestIsLevelAreaNeededByTask( pUnit, nLevelArea, pQuestTask ) )
					{
						int questTitleID = MYTHOS_QUESTS::QuestGetStringOrDialogIDForQuest( pUnit, pQuestTask, KQUEST_STRING_TITLE ); //QuestGetQuestTitle( pQuestTask->nParentQuest );							
						PStrPrintf( puszBuffer, nBufferSize, L"%s\n%s", puszBuffer, c_DialogTranslate( questTitleID ) );					
						returnValue = TRUE;
					}
				}			
			}
		}
	}
	return returnValue;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL LevelDefinitionIsSafe(
	const LEVEL_DEFINITION* pLevelDefinition)
{
 	return( pLevelDefinition->bTown || pLevelDefinition->bSafe );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL LevelIsSafe(
	const LEVEL* pLevel)
{
	if( LevelGetPVPWorld( pLevel ) )
	{
		return FALSE;
	}
	const LEVEL_DEFINITION* pLevelDefinition = LevelDefinitionGet( pLevel );	
	return LevelDefinitionIsSafe( pLevelDefinition );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL LevelIsTown(
	const LEVEL* pLevel)
{
	const LEVEL_DEFINITION* pLevelDefinition = LevelDefinitionGet( pLevel );	
	return pLevelDefinition->bTown;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL LevelIsRTS(
	const LEVEL* pLevel)
{
	const LEVEL_DEFINITION* pLevelDefinition = LevelDefinitionGet( pLevel );	
	return pLevelDefinition->bRTS;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL LevelPlayerOwnedCannotDie(
	const LEVEL *pLevel)
{
	ASSERTX_RETFALSE( pLevel, "Expected level" );
	const LEVEL_DEFINITION *pLevelDef = LevelDefinitionGet( pLevel );
	return pLevelDef->bPlayerOwnedCannotDie;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL LevelHasHellrift(	
	LEVEL *pLevel)
{
	ASSERTX_RETFALSE( pLevel, "Expected level" );
	
	// check all the sublevels
	int nNumSubLevels = LevelGetNumSubLevels( pLevel );
	for( int i = 0; i < nNumSubLevels; ++i)
	{
		const SUBLEVEL *pSubLevel = LevelGetSubLevelByIndex( pLevel, i );	
		if (SubLevelGetType( pSubLevel ) == ST_HELLRIFT)
		{
			return TRUE;
		}
	}
	return FALSE;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sLevelClearWarpDestination(
	VECTOR &vPosition, 
	VECTOR &vFaceDirection,
	int * pnRotation = NULL)
{
	vPosition = cgvNone;
	vFaceDirection = cgvNone;
	if(pnRotation)
	{
		*pnRotation = 0;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sLevelFindReturnWarp(
	LEVEL *pLevel, 
	ROOM **pRoomSelected, 
	VECTOR *pvPosition, 
	VECTOR *pvFaceDirection)
{
	ASSERTX_RETFALSE( pLevel, "Expected level" );
	
	// go through all rooms
	for (ROOM *pRoom = LevelGetFirstRoom( pLevel ); pRoom; pRoom = LevelGetNextRoom( pRoom ))
	{
	
		// reject rooms that are not on the default sublevel
		if (RoomIsInDefaultSubLevel( pRoom ) == FALSE)
		{
			continue;
		}
			
		// search all objects
		for (UNIT *pObject = pRoom->tUnitList.ppFirst[ TARGET_OBJECT ]; pObject; pObject = pObject->tRoomNode.pNext)
		{
			if (ObjectIsPortalFromTown( pObject ))
			{
				
				ASSERTX_RETFALSE( pRoomSelected, "Invalid params" );
				ASSERTX_RETFALSE( pvPosition, "Invalid params" );
				ASSERTX_RETFALSE( pvFaceDirection, "Invalid params" );
				
				*pRoomSelected = pRoom;
				TownPortalGetReturnWarpArrivePosition( 
					pObject, 
					pvFaceDirection, 
					pvPosition);
				return TRUE;
					
			}
			
		}	
			
	}	
	
	return FALSE;  // no return warp found in level
	
}	

//----------------------------------------------------------------------------
enum START_LOCATION_FACING_FLAGS
{
	SLFF_IN_FRONT_OF_UNIT_BIT,
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sGetStartLocationFacingUnit(
	UNIT *pUnit,
	float flMinDistance,
	float flMaxDistance,
	DWORD dwFacingFlags,
	ROOM **pRoomSelected, 
	VECTOR *pvPosition, 
	VECTOR *pvFaceDirection)
{	
	GAME *pGame = UnitGetGame( pUnit );

	// find a location around the unit
	NEAREST_NODE_SPEC tSpec;
	SETBIT( tSpec.dwFlags, NPN_CHECK_LOS );
	tSpec.fMinDistance = flMinDistance;
	tSpec.fMaxDistance = flMaxDistance;

	// if we can only pick in front of, set options for it
	VECTOR vDirection;
	if (TESTBIT( dwFacingFlags, SLFF_IN_FRONT_OF_UNIT_BIT ))
	{
		vDirection = UnitGetFaceDirection( pUnit , FALSE );
		tSpec.vFaceDirection = vDirection;
		SETBIT( tSpec.dwFlags, NPN_IN_FRONT_ONLY );
		SETBIT( tSpec.dwFlags, NPN_QUARTER_DIRECTIONALITY );
	}

	// find the location
	FREE_PATH_NODE tFreePathNode;
	int nNumNodes = RoomGetNearestPathNodes(pGame, UnitGetRoom(pUnit), UnitGetPosition(pUnit), 1, &tFreePathNode, &tSpec);
	if (nNumNodes > 0)
	{

		// save room
		*pRoomSelected = tFreePathNode.pRoom;

		// save position
		*pvPosition = RoomPathNodeGetWorldPosition( 
			pGame, 
			tFreePathNode.pRoom, 
			tFreePathNode.pNode);
			//dwFlags);			
		// make the direction facing the unit
		VectorSubtract( *pvFaceDirection, UnitGetPosition( pUnit ), *pvPosition );

	}
	else
	{

		// save room
		*pRoomSelected = UnitGetRoom( pUnit );

		// use the unit loc itself
		*pvPosition = UnitGetPosition( pUnit );

		// get direction
		*pvFaceDirection = UnitGetFaceDirection( pUnit, FALSE );

	}		
}

//----------------------------------------------------------------------------
#define MIN_HEADSTONE_DISTANCE (7.0f)
#define MAX_HEADSTONE_DISTANCE (30.0f)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sLevelFindMostRecentHeadstoneLocation(
	UNIT *pOwner,
	LEVEL *pLevel, 
	ROOM **pRoomSelected, 
	VECTOR *pvPosition, 
	VECTOR *pvFaceDirection)
{
	
	// find most recent one	
	UNIT *pHeadstoneLast = PlayerGetMostRecentHeadstone( pOwner );
	
	// if we found a headstone
	if (pHeadstoneLast)
	{

		// find a spot around the headstone
		sGetStartLocationFacingUnit( 
			pHeadstoneLast, 
			MIN_HEADSTONE_DISTANCE, 
			MAX_HEADSTONE_DISTANCE, 
			0,
			pRoomSelected, 
			pvPosition, 
			pvFaceDirection );
						
		return TRUE;  // we found a spot
		
	}
	
	return FALSE;  // no headstone found
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static ROOM_ITERATE_UNITS sFindGravekeeper(
	UNIT *pUnit,
	void *pCallbackData)
{
	UNITID *pidGravekeeper = (UNITID *)pCallbackData;
	ASSERTX_RETVAL( pidGravekeeper, RIU_STOP, "Expected storage for unit id" );
	if (UnitIsGravekeeper( pUnit ))
	{
		*pidGravekeeper = UnitGetId( pUnit );	
		return RIU_STOP;  // no need to continue searching
	}
	return RIU_CONTINUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static ROOM_ITERATE_UNITS sFindTransporter(
	UNIT *pUnit,
	void *pCallbackData)
{
	UNITID *pidGravekeeper = (UNITID *)pCallbackData;
	ASSERTX_RETVAL( pidGravekeeper, RIU_STOP, "Expected storage for unit id" );
	if (UnitIsTransporter( pUnit ))
	{
		*pidGravekeeper = UnitGetId( pUnit );	
		return RIU_STOP;  // no need to continue searching
	}
	return RIU_CONTINUE;
}
//----------------------------------------------------------------------------
#define MIN_GRAVEKEEPER_DISTANCE (3.0f)
#define MAX_GRAVEKEEPER_DISTANCE (18.0f)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sLevelFindGravekeeperRestartLocation(
	LEVEL *pLevel, 
	ROOM **pRoomSelected, 
	VECTOR *pvPosition, 
	VECTOR *pvFaceDirection)
{
	ASSERTX_RETFALSE( pLevel, "Expected level" );
	UNITID idGravekeeper = INVALID_ID;
		
	// go through all rooms
	for (ROOM *pRoom = LevelGetFirstRoom( pLevel ); pRoom; pRoom = LevelGetNextRoom( pRoom ))
	{
		RoomIterateUnits( pRoom, NULL, sFindGravekeeper, &idGravekeeper );			
	}	

	// did we find one
	if (idGravekeeper != INVALID_LINK)
	{
		GAME *pGame = LevelGetGame( pLevel );
		UNIT *pGravekeeper = UnitGetById( pGame, idGravekeeper );

		DWORD dwFlags = 0;
		SETBIT( dwFlags, SLFF_IN_FRONT_OF_UNIT_BIT );
		
		// get start location facing unit
		sGetStartLocationFacingUnit(
			pGravekeeper,
			MIN_GRAVEKEEPER_DISTANCE,
			MAX_GRAVEKEEPER_DISTANCE,
			dwFlags,
			pRoomSelected,
			pvPosition,
			pvFaceDirection);

		return TRUE;  // we found one maw!
						
	}
	else
	{	
		return FALSE;  // sowwy, wo gwavekeeper
	}

}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sLevelFindTransporterStartLocation(
	LEVEL *pLevel, 
	ROOM **pRoomSelected, 
	VECTOR *pvPosition, 
	VECTOR *pvFaceDirection)
{
	ASSERTX_RETFALSE( pLevel, "Expected level" );
	UNITID idTransporter = INVALID_ID;

	// go through all rooms
	for (ROOM *pRoom = LevelGetFirstRoom( pLevel ); pRoom; pRoom = LevelGetNextRoom( pRoom ))
	{
		RoomIterateUnits( pRoom, NULL, sFindTransporter, &idTransporter );			
	}	

	// did we find one
	if (idTransporter != INVALID_LINK)
	{
		GAME *pGame = LevelGetGame( pLevel );
		UNIT *pTransporter = UnitGetById( pGame, idTransporter );

		DWORD dwFlags = 0;
		SETBIT( dwFlags, SLFF_IN_FRONT_OF_UNIT_BIT );

		// get start location facing unit
		sGetStartLocationFacingUnit(
			pTransporter,
			MIN_GRAVEKEEPER_DISTANCE,
			MAX_GRAVEKEEPER_DISTANCE,
			dwFlags,
			pRoomSelected,
			pvPosition,
			pvFaceDirection);

		return TRUE;  // we found one 

	}
	else
	{	
		return FALSE;  
	}

}

//----------------------------------------------------------------------------
struct START_MARKER_DATA
{
	ROOM **pRoom;
	VECTOR *pvPosition;
	VECTOR *pvDirection;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static ROOM_ITERATE_UNITS sIsStartMarker(
	UNIT *pUnit,
	void *pCallbackData)
{
	if (UnitIsA( pUnit, UNITTYPE_START_LOCATION ))
	{
		START_MARKER_DATA *pStartMarkerData = (START_MARKER_DATA *)pCallbackData;
		*pStartMarkerData->pRoom = UnitGetRoom( pUnit );
		*pStartMarkerData->pvPosition = UnitGetPosition( pUnit );
		*pStartMarkerData->pvDirection = UnitGetFaceDirection( pUnit, FALSE );
		return RIU_STOP;
	}
	return RIU_CONTINUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static ROOM_ITERATE_UNITS sAddWarp(
	UNIT *pUnit,
	void *pCallbackData)
{
	if (UnitIsA( pUnit, UNITTYPE_WARP ))
	{
		CArrayList *arrayList = (CArrayList * )pCallbackData;
		ArrayAddGetIndex(*arrayList, pUnit);
	}
	return RIU_CONTINUE;
}

static UNIT * GetWarpConnectingLevels( LEVEL *pLevel,
									   LEVEL *pLastLevel,
									   int nLastLevelAreaID = INVALID_ID )
{
	CArrayList arrayValidObjects( AL_REFERENCE, sizeof( ROOM * ) );
	TARGET_TYPE eTargetTypes[] = { TARGET_OBJECT, TARGET_INVALID };
	LevelIterateUnits( pLevel, eTargetTypes, sAddWarp, &arrayValidObjects ); 
	for( UINT t = 0; t < arrayValidObjects.Count(); t++ )
	{
		UNIT *pObjectWarp = (UNIT * )arrayValidObjects.GetPtr( t );
		if( UnitGetWarpToLevelAreaID( pObjectWarp ) != INVALID_ID &&
			( ( pLastLevel && UnitGetWarpToLevelAreaID( pObjectWarp ) == LevelGetLevelAreaID( pLastLevel ) ) ||
			( nLastLevelAreaID != INVALID_ID && UnitGetWarpToLevelAreaID( pObjectWarp )== nLastLevelAreaID ) ) )
		{
			arrayValidObjects.Destroy();
			return pObjectWarp;
		}
	}
	arrayValidObjects.Destroy();
	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sLevelFindStartMarker( 
	LEVEL *pLevel, 
	LEVEL *pLastLevel,
	ROOM **pRoomDest, 
	VECTOR *pvPosition, 
	VECTOR *pvDirection)
{
	ASSERTX_RETFALSE( pLevel, "Expected level" );
	ASSERTX_RETFALSE( pRoomDest, "Expected room" );	
	ASSERTX_RETFALSE( pvPosition, "Expected position" );	
	ASSERTX_RETFALSE( pvDirection, "Expected direction" );	
	
	// init room found
	*pRoomDest = NULL;

	// setup callback data
	START_MARKER_DATA tCallbackData;
	tCallbackData.pRoom = pRoomDest;
	tCallbackData.pvPosition = pvPosition;
	tCallbackData.pvDirection = pvDirection;
	
	// go through all rooms in level	
	if( AppIsHellgate() ||
		pLastLevel == NULL )
	{
		for (ROOM *pRoom = LevelGetFirstRoom( pLevel ); pRoom; pRoom = LevelGetNextRoom( pRoom ))
		{
		
			// search objects in this room
			TARGET_TYPE eTargetTypes[] = { TARGET_OBJECT, TARGET_INVALID };
			RoomIterateUnits( pRoom, eTargetTypes, sIsStartMarker, &tCallbackData );
			
			// if we found one, our callback data will be set
			if (*tCallbackData.pRoom != NULL)
			{
				return TRUE;
			}
			
		}
	}
	else //I wonder if this is to slow....?
	{
		CArrayList arrayValidObjects( AL_REFERENCE, sizeof( ROOM * ) );
		TARGET_TYPE eTargetTypes[] = { TARGET_OBJECT, TARGET_INVALID };
		LevelIterateUnits( pLevel, eTargetTypes, sAddWarp, &arrayValidObjects ); 
		for( UINT t = 0; t < arrayValidObjects.Count(); t++ )
		{
			UNIT *pObjectWarp = GetWarpConnectingLevels( pLevel, pLastLevel );
			if( pObjectWarp )
			{
				//this is a warp to the level area so we'll just start here.
				*tCallbackData.pRoom = UnitGetRoom( pObjectWarp );
				*tCallbackData.pvPosition = UnitGetPosition( pObjectWarp );
				*tCallbackData.pvDirection = UnitGetFaceDirection( pObjectWarp, FALSE );
				arrayValidObjects.Destroy();
				return TRUE;
			}
		}
		
		//we couldn't find a place to start that linked to our last level. So we'll so about the old way...
		return sLevelFindStartMarker( pLevel, NULL, pRoomDest, pvPosition, pvDirection );		
	}

	// no start marker found
	return FALSE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sLevelFindMarker( 
	LEVEL *pLevel, 
	ROOM **pRoomDest, 
	VECTOR *pvPosition, 
	VECTOR *pvDirection,
	GLOBAL_INDEX eGIMarker)
{
	ASSERTX_RETFALSE( pLevel, "Expected level" );
	ASSERTX_RETFALSE( pRoomDest, "Expected room" );	
	ASSERTX_RETFALSE( pvPosition, "Expected position" );	
	ASSERTX_RETFALSE( pvDirection, "Expected direction" );	
	ASSERTX_RETFALSE( eGIMarker != GI_INVALID, "Expected global index" );
	
	// init result
	*pRoomDest = NULL;
	*pvPosition = cgvNone;
	*pvDirection = cgvNone;

	// find marker
	TARGET_TYPE eTargetTypes[] = { TARGET_OBJECT, TARGET_INVALID };	
	UNIT *pMarker = LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_OBJECT, GlobalIndexGet( eGIMarker ) );
	if (pMarker)
	{
		*pRoomDest = UnitGetRoom( pMarker );
		*pvPosition = UnitGetPosition( pMarker );
		*pvDirection = UnitGetFaceDirection( pMarker, FALSE );
		return TRUE;
	}
	
	return FALSE;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sLevelFindRandomSpawn( 
	LEVEL *pLevel, 
	ROOM **pRoomDest, 
	VECTOR *pvPosition, 
	VECTOR *pvDirection,
	UNIT *pUnit)
{
	ASSERTX_RETFALSE( pLevel, "Expected level" );
	ASSERTX_RETFALSE( pRoomDest, "Expected room" );	
	ASSERTX_RETFALSE( pvPosition, "Expected position" );	
	ASSERTX_RETFALSE( pvDirection, "Expected direction" );	
	ASSERTX_RETFALSE( pUnit, "Expected unit" );

	// init result
	*pRoomDest = NULL;
	*pvPosition = cgvNone;
	*pvDirection = cgvNone;

	TARGET_TYPE eTargetTypes[] = { TARGET_OBJECT, TARGET_INVALID };	
	UNIT *pMarker = LevelFindRandomUnitOf( pLevel, eTargetTypes, GENUS_OBJECT, GlobalIndexGet( GI_OBJECT_SPAWN_LOCATION ) );
	if (pMarker)
	{
		*pRoomDest = UnitGetRoom( pMarker );
		*pvPosition = UnitGetPosition( pMarker );
		*pvDirection = UnitGetFaceDirection( pMarker, FALSE );
		return TRUE;
	}

	return FALSE;

}

//----------------------------------------------------------------------------
struct FIND_OPPONENT_DATA
{
	UNIT *firstPlayer;
	UNIT *out_secondPlayer;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sFindOpponent( UNIT *unit, void *callbackData )
{
	FIND_OPPONENT_DATA *data = (FIND_OPPONENT_DATA *)callbackData;
	if (unit != data->firstPlayer)
		data->out_secondPlayer = unit;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sLevelFindFarthestSpawnFromOpponent( 
	LEVEL *pLevel, 
	ROOM **pRoomDest, 
	VECTOR *pvPosition, 
	VECTOR *pvDirection,
	UNIT *pUnit)
{
	ASSERTX_RETFALSE( pLevel, "Expected level" );
	ASSERTX_RETFALSE( pRoomDest, "Expected room" );	
	ASSERTX_RETFALSE( pvPosition, "Expected position" );	
	ASSERTX_RETFALSE( pvDirection, "Expected direction" );	
	ASSERTX_RETFALSE( pUnit, "Expected unit" );

	// init result
	*pRoomDest = NULL;
	*pvPosition = cgvNone;
	*pvDirection = cgvNone;

	// try to find an opponent in the level
	FIND_OPPONENT_DATA tData;
	tData.firstPlayer = pUnit;
	tData.out_secondPlayer = NULL;
	LevelIteratePlayers(pLevel, s_sFindOpponent, &tData);

	TARGET_TYPE eTargetTypes[] = { TARGET_OBJECT, TARGET_INVALID };	
	UNIT *pMarker = NULL;
	if (tData.out_secondPlayer)
	{
		pMarker = LevelFindFarthestUnitOf( pLevel, UnitGetPosition( tData.out_secondPlayer ), eTargetTypes, GENUS_OBJECT, GlobalIndexGet( GI_OBJECT_SPAWN_LOCATION ) );

	}
	else
	{
		pMarker = LevelFindRandomUnitOf( pLevel, eTargetTypes, GENUS_OBJECT, GlobalIndexGet( GI_OBJECT_SPAWN_LOCATION ) );
	}

	if (pMarker)
	{
		*pRoomDest = UnitGetRoom( pMarker );
		*pvPosition = UnitGetPosition( pMarker );
		*pvDirection = UnitGetFaceDirection( pMarker, FALSE );
		return TRUE;
	}

	return FALSE;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
ROOM * LevelGetStartPosition( 
	GAME * game,
	UNIT * pUnit,
	LEVELID idLevel,
	VECTOR & vPosition, 
	VECTOR & vFaceDirection, 
	DWORD dwWarpFlags)
{

	LEVEL * level = LevelGetByID(game, idLevel);
	if (!level)
	{
		sLevelClearWarpDestination(vPosition, vFaceDirection);
		return NULL;
	}

	const LEVEL_DEFINITION * level_definition = LevelDefinitionGet(level);
	ASSERT(level_definition);

	ROOM *pRoomDest = NULL;
	
	// if we're supposed to appear at a return portal, find it
	if (s_MetagameLevelGetStartPosition(pUnit, idLevel, &pRoomDest, &vPosition, &vFaceDirection))
	{
		return pRoomDest;
	}
	else if (TESTBIT( dwWarpFlags, WF_ARRIVE_AT_WARP_RETURN_BIT ))
	{
		if (sLevelFindReturnWarp(level, &pRoomDest, &vPosition, &vFaceDirection))
		{
			return pRoomDest;
		}
	}
	else if (TESTBIT( dwWarpFlags, WF_ARRIVE_AT_HEADSTONE_BIT ))
	{
		ASSERT( pUnit );
		if (sLevelFindMostRecentHeadstoneLocation(pUnit, level, &pRoomDest, &vPosition, &vFaceDirection))
		{
			// once you return to your headstone, you can't warp back to town and
			// do it again ... it becomes "used up" I guess
			PlayerSetHeadstoneLevelDef( pUnit, INVALID_LINK );
			return pRoomDest;
		}		
	}
	else if (TESTBIT( dwWarpFlags, WF_ARRIVE_AT_GRAVEKEEPER_BIT ))
	{
		if (sLevelFindGravekeeperRestartLocation(level, &pRoomDest, &vPosition, &vFaceDirection))
		{
			return pRoomDest;
		}					
	}
	else if (TESTBIT( dwWarpFlags, WF_ARRIVE_AT_TRANSPORTER_BIT ))
	{
		if (sLevelFindTransporterStartLocation(level, &pRoomDest, &vPosition, &vFaceDirection))
		{
			return pRoomDest;
		}					
	}
	else if (TESTBIT( dwWarpFlags, WF_ARRIVE_AT_START_LOCATION_BIT ))
	{
		LEVEL *pLastLevel = ( pUnit )?UnitGetLevel( pUnit ):NULL;
		if (sLevelFindStartMarker( level, pLastLevel, &pRoomDest, &vPosition, &vFaceDirection ))
		{
			return pRoomDest;
		}
	}
	else if (TESTBIT( dwWarpFlags, WF_ARRIVE_AT_ARENA_BIT ))
	{
		if (sLevelFindMarker( level, &pRoomDest, &vPosition, &vFaceDirection, GI_OBJECT_QUEST_GAME_START_LOCATION))
		{
			return pRoomDest;
		}
	}
	else if (TESTBIT( dwWarpFlags, WF_ARRIVE_AT_RANDOM_SPAWN_BIT ))
	{
		if (sLevelFindRandomSpawn( level, &pRoomDest, &vPosition, &vFaceDirection, pUnit ))
		{
			return pRoomDest;
		}
	}
	else if (TESTBIT( dwWarpFlags, WF_ARRIVE_AT_FARTHEST_PVP_SPAWN_BIT ))
	{
		if (sLevelFindFarthestSpawnFromOpponent( level, &pRoomDest, &vPosition, &vFaceDirection, pUnit ))
		{
			return pRoomDest;
		}
	}

	// the only 2 reasons this code runs is either from the console cheat or you are using recall/waypoints/tp
	// check "markers" first
	for (ROOM* room = LevelGetFirstRoom(level); room; room = LevelGetNextRoom(room))
	{
	
		// some rooms are disallowed to start a level
		if (s_RoomAllowedForStart( room, dwWarpFlags ) == FALSE)
		{
			continue;
		}
	
		for ( UNIT * test = room->tUnitList.ppFirst[TARGET_OBJECT]; test; test = test->tRoomNode.pNext )
		{
		
			if (s_ObjectAllowedAsStartingLocation( test, dwWarpFlags ))
			{
				if (UnitGetGenus(test) == GENUS_OBJECT)
				{
					const UNIT_DATA *objectdata = ObjectGetData(game, UnitGetClass(test));
					if ( objectdata && objectdata->nTriggerType != INVALID_LINK )
					{
						if (GlobalIndexGet( GI_OTRIGGER_MARKER ) == objectdata->nTriggerType)
						{
							// return info about pos
							vPosition = test->vPosition;
							vFaceDirection = test->vFaceDirection;
							return room;
						}
					}
				}
			}
		}
	}
	// then just start them from any warp point
	UNIT *pWarp = NULL;
	for (ROOM* room = LevelGetFirstRoom(level); room; room = LevelGetNextRoom(room))
	{
	
		// some rooms are disallowed to start a level
		if (s_RoomAllowedForStart( room, dwWarpFlags ) == FALSE)
		{
			continue;
		}
	
		for ( UNIT * test = room->tUnitList.ppFirst[TARGET_OBJECT]; test; test = test->tRoomNode.pNext )
		{
			if (ObjectIsWarp(test))
			{
			
				if (s_ObjectAllowedAsStartingLocation( test, dwWarpFlags ))
				{
			
					if (pWarp == NULL)
					{
						pWarp = test;
					}
					else
					{
						// if we've only found dynamic warps so far, prefer non dynamic ones
						if (ObjectIsDynamicWarp( pWarp ))
						{
							pWarp = test;
						}
					}
				}
			}				
		}
	}

	// return info about pos		
	if (pWarp != NULL)
	{
		return WarpGetArrivePosition( pWarp, &vPosition, &vFaceDirection, WD_FRONT );
	}

	ROOM *pRoomDefault = NULL;
	
	// just grab the first suitable room and translate a location in the middle of the floor
	for (ROOM* room = LevelGetFirstRoom(level); room; room = LevelGetNextRoom(room))	
	{

		if (s_RoomAllowedForStart( room, dwWarpFlags ) == FALSE)
		{
			continue;
		}
		
		pRoomDefault = room;
		break;
			
	}

	// super fallback, use first room
	if (pRoomDefault == NULL)
	{
		pRoomDefault = LevelGetFirstRoom(level);
	}

	ASSERTX_RETNULL( pRoomDefault, "No room found to start level" );
			
	vPosition.fX = pRoomDefault->pHull->aabb.center.fX;
	vPosition.fY = pRoomDefault->pHull->aabb.center.fY;
	vPosition.fZ = pRoomDefault->pHull->aabb.center.fZ;

	if(AppIsHellgate() )
	{
		VECTOR vNormal;
		VECTOR vRayDir = VECTOR( 0.0f, 0.0f, -1.0f );
		float fRayLength = pRoomDefault->pHull->aabb.halfwidth.fZ * 2.0f;
		float fDistance = LevelLineCollideLen( game, level, vPosition, vRayDir, fRayLength, &vNormal );

		int nRepeats = 0;
		while ( fDistance >= fRayLength - 0.1f )
		{
			vPosition.fX = pRoomDefault->pHull->aabb.center.fX + RandGetFloat( game, -pRoomDefault->pHull->aabb.halfwidth.fX, pRoomDefault->pHull->aabb.halfwidth.fX );
			vPosition.fY = pRoomDefault->pHull->aabb.center.fY + RandGetFloat( game, -pRoomDefault->pHull->aabb.halfwidth.fY, pRoomDefault->pHull->aabb.halfwidth.fY );
			fDistance = LevelLineCollideLen( game, level, vPosition, vRayDir, fRayLength, &vNormal );

			nRepeats++;
			ASSERTV_BREAK(
				nRepeats < 1000, 
				"Infinte loop looking for level start position - Room=%s, Level=%s",
				RoomGetDevName( pRoomDefault ),
				LevelGetDevName( level ));
		}
		vPosition.fZ -= fDistance;
	}
	else
		vPosition.fZ = 0.02f; //-= fDistance;
	
	
	vFaceDirection = VECTOR(0.0f, 1.0f, 0.0f);
	
	return pRoomDefault;
	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
ROOM * LevelGetWarpToPosition( 
	GAME * game,
	LEVELID idLevel,
	WARP_TO_TYPE eWarpToType,
	char * szTriggerString,
	VECTOR & vPosition, 
	VECTOR & vFaceDirection,
	int * pnRotation)
{
	LEVEL * level = LevelGetByID(game, idLevel);
	if (!level)
	{
		sLevelClearWarpDestination(vPosition, vFaceDirection, pnRotation);
		return NULL;
	}

	const LEVEL_DEFINITION * level_definition = NULL;
	if (level->m_nLevelDef != INVALID_LINK)
	{
		level_definition = LevelDefinitionGet(level);
	}
	ASSERT_RETNULL(level_definition);

	for (ROOM * room = LevelGetFirstRoom(level); room; room = LevelGetNextRoom(room))
	{
		for (UNIT * test = room->tUnitList.ppFirst[TARGET_OBJECT]; test; test = test->tRoomNode.pNext)
		{
			if ((UnitGetGenus(test) != GENUS_OBJECT))
			{
				continue;
			}
				
			if (!UnitTestFlag(test, UNITFLAG_TRIGGER))
			{
				continue;
			}

			BOOL bFound = FALSE;
			if (eWarpToType == WARP_TO_TYPE_PREV && UnitIsA( test, UNITTYPE_WARP_PREV_LEVEL ))
			{
				bFound = TRUE;
			}
			else if (eWarpToType == WARP_TO_TYPE_NEXT && UnitIsA( test, UNITTYPE_WARP_NEXT_LEVEL ))
			{
				bFound = TRUE;
			}
			else if (eWarpToType == WARP_TO_TYPE_TRIGGER && szTriggerString)
			{
				const UNIT_DATA * object_data = ObjectGetData(game, UnitGetClass(test));
				ASSERT_CONTINUE(object_data);
				ASSERT_CONTINUE(object_data->nTriggerType != INVALID_LINK);
				OBJECT_TRIGGER_DATA * triggerdata = (OBJECT_TRIGGER_DATA *)ExcelGetData(game, DATATABLE_OBJECTTRIGGERS, object_data->nTriggerType);
				if (triggerdata->bIsWarp)
				{
					if (PStrICmp(szTriggerString, object_data->szTriggerString1) == 0)
					{
						bFound = TRUE;
					}
					if (PStrICmp(szTriggerString, object_data->szTriggerString2) == 0)
					{
						bFound = TRUE;
					}
				}
			}

			if (!bFound)
			{
				continue;
			}

			return WarpGetArrivePosition( test, &vPosition, &vFaceDirection, WD_FRONT, pnRotation );
			
		}
		
	}

	ASSERT_RETNULL(0);
	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)

void c_LevelLoadComplete(
	int nLevelDefinition, 
	int nDRLGDefinition,
	int nLevelId,
	int nLevelDepth)
{
	GAME *pGame = AppGetCltGame();
	
	// save level info
	sgnLevelDefinition = nLevelDefinition;
	sgnDRLGDefinition = nDRLGDefinition;
	
	sgnLevelDepth = nLevelDepth;
	
	if ( AppIsTugboat() )
	{
		LEVEL * level = LevelGetByID(pGame, AppGetCurrentLevel());
		ROOM* room = NULL;
		ROOM* next = LevelGetFirstRoom(level);
		next = LevelGetFirstRoom(level);
		//marsh testing streaming
		if( level->m_pLevelDefinition->bClientDiscardRooms == FALSE )
		{
			while ( (room = next ) != NULL )
			{
				next = LevelGetNextRoom(room);
		
				RoomDoSetup(pGame, level, room );
				RoomCreatePathEdgeNodeConnections(pGame, room, FALSE);
			}
		}
		c_ClearVisibleRooms();
	}


	if (AsyncFileIsAsyncRead()) 
	{
		c_LevelWaitForLoaded( pGame, nLevelDefinition, nDRLGDefinition );
	}

	// CML 2006.8.24: moved gamestate set to running to load complete callback
}
#endif //!ISVERSION(SERVER_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL LevelIsVisible( LEVEL* pLevel,
				 const VECTOR& Position )	// position to check
{
#if !ISVERSION(SERVER_VERSION)
	if( pLevel == NULL ||
		pLevel->m_pVisibility == NULL )
	{
		return TRUE;
	}
	int TileX( pLevel->m_pVisibility->GetXTile( Position.fX ) );
	int TileY( pLevel->m_pVisibility->GetYTile( Position.fY ) );
	return pLevel->m_pVisibility->Visible( TileX, TileY );
#else
	return TRUE;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void LevelDirtyVisibility( LEVEL* pLevel )
{
	// Visibility not yet imported - uncomment this line once it's done
	if ( AppIsTugboat() )
		pLevel->m_pVisibility->SetCenter( VECTOR( 99,-99,-99 ) );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void LevelUpdateVisibility( GAME *pGame,
						    LEVEL* pLevel,
							const VECTOR& Position )	// position of light source
{
#if !ISVERSION(SERVER_VERSION)
	if( pLevel == NULL ||
		pLevel->m_pVisibility == NULL )
	{
		return;
	}
	int TileX( pLevel->m_pVisibility->GetXTile( Position.fX ) );
	int TileY( pLevel->m_pVisibility->GetYTile( Position.fY ) );
	
	VECTOR VisibilityCenter( (float)TileX, 0, (float)TileY );

	pLevel->m_pVisibility->SetCenterFloat( Position );

	if( VisibilityCenter == pLevel->m_pVisibility->Center() )
	{
		return;
	}
	pLevel->m_pVisibility->SetCenter( VisibilityCenter );


	// enable this for true line-of-sight
	pLevel->m_pVisibility->ClearVisibilityDataLocal();

	float fVisibilityRadius = (float)KMaxVisibilityRadius * pLevel->m_pLevelDefinition->fVisibilityDistanceScale;

	VECTOR MinBounds( Position );
	VECTOR MaxBounds( Position );
	MinBounds.x -= (float)fVisibilityRadius;
	MinBounds.y -= (float)fVisibilityRadius;
	MaxBounds.x += (float)fVisibilityRadius;
	MaxBounds.y += (float)fVisibilityRadius;

	int CastSteps( 48 );
	MATRIX mRotationMatrix;
	MatrixRotationAxis( mRotationMatrix, VECTOR( 0, 0, 1 ), DegreesToRadians( 360.0f / (float)CastSteps ) );

	VECTOR Ray = VECTOR( 0, 1, 0 );
	VECTOR Start( Position );
	VECTOR ImpactPoint;
	VECTOR ImpactNormal;

	Start.fZ = Position.fZ + 2.0f;
	VECTOR End( Start );
	float Radius = fVisibilityRadius;
	UNIT *pUnit = GameGetControlUnit(pGame);
	REF(pUnit);
	VECTOR UnitEnd;
	VECTOR UnitStart;
	UnitStart = Start;
	UnitStart.fZ = 0;
	BOUNDING_BOX BBox;
	BBox.vMin = MinBounds;
	BBox.vMax = MaxBounds;
	BBox.vMax.fZ += 4;
	BBox.vMin.fZ = 0;

	SIMPLE_DYNAMIC_ARRAY<ROOM*> RoomsList;
	ArrayInit(RoomsList, GameGetMemPool(pGame), 2);
	pLevel->m_pQuadtree->GetItems( BBox, RoomsList );
	DWORD dwMoveFlags = 0;
	dwMoveFlags |= MOVEMASK_DOORS;
	dwMoveFlags |= MOVEMASK_SOLID_MONSTERS;
	dwMoveFlags |= MOVEMASK_TEST_ONLY;
	dwMoveFlags |= MOVEMASK_NOT_WALLS;
	dwMoveFlags |= MOVEMASK_UPDATE_POSITION;
	COLLIDE_RESULT result;
	result.flags = 0;
	result.unit = NULL;
	TileX = pLevel->m_pVisibility->GetXTile( Start.fX );
	TileY = pLevel->m_pVisibility->GetYTile( Start.fY );
	pLevel->m_pVisibility->SetVisibility( TileX, TileY, TRUE );


	SIMPLE_DYNAMIC_ARRAY<unsigned int> SortedFaces;
	ArrayInit(SortedFaces, GameGetMemPool(pGame), 2);
	VECTOR Delta = MaxBounds - MinBounds;
	Delta.fZ = 0;
	float Dist = VectorLength( Delta );
	VectorNormalize( Delta );
	LevelSortFaces(pGame, pLevel, SortedFaces, MinBounds, Delta, Dist );

	for( int i = 0; i < CastSteps; i++ )
	{

		float Length = LevelLineCollideLen( pGame, pLevel, SortedFaces, Start + Ray * .1f, Ray, Radius, &ImpactNormal );
		
	
//		DWORD dwMoveResult = 0;

		UnitEnd = Start + Ray * Length;
		UnitEnd.fZ = 0;
		//dwMoveResult = RoomCollide(pUnit, &RoomsList, UnitStart, Ray, &UnitEnd, UnitGetCollisionRadius( pUnit ), 5.0f, dwMoveFlags);

		RoomCollide(pUnit, &RoomsList, UnitStart, Ray, &UnitEnd, UnitGetCollisionRadius( pUnit ), 5.0f, dwMoveFlags, &result );

		
		if( TESTBIT(&result.flags, MOVERESULT_COLLIDE_UNIT)  )
		{
			UnitEnd -= UnitStart;
			UnitEnd.fZ = 0;
			Length = VectorLength( UnitEnd ) + pLevel->m_pVisibility->PatchVisibilityWidth() * .5f;
		}
		int Steps = (int)FLOOR( Length / ( pLevel->m_pVisibility->PatchVisibilityWidth() * .5f ) + .5f ) + 1;
		ImpactPoint = Start;
		End = Ray * pLevel->m_pVisibility->PatchVisibilityWidth() * .5f;
		
		for( int j = 0; j < Steps + 1; j++ )
		{
			TileX = pLevel->m_pVisibility->GetXTile( ImpactPoint.fX );
			TileY = pLevel->m_pVisibility->GetYTile( ImpactPoint.fY );
			pLevel->m_pVisibility->SetVisibility( TileX, TileY, TRUE );
			
			TileX = pLevel->m_pVisibility->GetXTileHigh( ImpactPoint.fX );
			TileY = pLevel->m_pVisibility->GetYTileHigh( ImpactPoint.fY );
			pLevel->m_pVisibility->SetVisibility( TileX, TileY, TRUE );

			TileX = pLevel->m_pVisibility->GetXTile( ImpactPoint.fX );
			TileY = pLevel->m_pVisibility->GetYTileHigh( ImpactPoint.fY );
			pLevel->m_pVisibility->SetVisibility( TileX, TileY, TRUE );

			TileX = pLevel->m_pVisibility->GetXTileHigh( ImpactPoint.fX );
			TileY = pLevel->m_pVisibility->GetYTile( ImpactPoint.fY );
			pLevel->m_pVisibility->SetVisibility( TileX, TileY, TRUE );
			ImpactPoint += End;
		}
		MatrixMultiplyNormal( &Ray, &Ray, &mRotationMatrix );
}
	RoomsList.Destroy();
#endif
} // Level::UpdateVisiblity()

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LevelProjectToFloor(
	LEVEL *pLevel,
	const VECTOR &vPos,
	VECTOR &vPosOnFloor)
{
	ASSERTX_RETURN( pLevel, "Expected level" );
	GAME *pGame = LevelGetGame( pLevel );
	
	// we will project "down"
	VECTOR vDirection = cgvZAxis;
	vDirection.fZ = -vDirection.fZ;
	
	// offset a little bit in Z to allow units that project from their position 
	// already on the floor to be right underneath them
	VECTOR vSource = vPos;
	vSource.fZ += 0.1f;
	
	// project
	vPosOnFloor = vSource;
	vPosOnFloor.fZ -= LevelLineCollideLen( pGame, pLevel, vSource, vDirection, 9999999.9f );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
LEVELID s_LevelFindLevelOfTypeLinkingTo(
	GAME *pGame,
	const LEVEL_TYPE &tLevelTypeSource,
	LEVELID idLevelDest)
{
	ASSERTX_RETINVALID( idLevelDest != INVALID_ID, "Expected dest level id" );
	LEVELID idLevelLinkedToDest = INVALID_ID;
	
	// go through all levels
	for (LEVEL *pLevelOther = DungeonGetFirstLevel( pGame );
		 pLevelOther;
		 pLevelOther = DungeonGetNextLevel( pLevelOther ))
	{
		LEVELID idLevelOther = LevelGetID( pLevelOther );
		
		// ignore the destination level
		if (idLevelOther == idLevelDest)
		{
			continue;
		}

		// ignore levels that aren't of the right type (if specified)
		if (tLevelTypeSource.nLevelDef != INVALID_LINK)
		{
			int nLevelDefOther = LevelGetDefinitionIndex( pLevelOther );
			
			if (nLevelDefOther != tLevelTypeSource.nLevelDef)
			{
				continue;
			}
		}
		else if (tLevelTypeSource.nLevelArea != INVALID_LINK)
		{
			int nLevelAreaOther = LevelGetLevelAreaID( pLevelOther );
			int nLevelDepthOther = LevelGetDepth( pLevelOther );
			BOOL bPVPWorldOther = LevelGetPVPWorld( pLevelOther );
			
			if (nLevelAreaOther != tLevelTypeSource.nLevelArea ||
				nLevelDepthOther != tLevelTypeSource.nLevelDepth ||
				bPVPWorldOther != tLevelTypeSource.bPVPWorld )
			{
				continue;
			}
			
		}
				
		// check next and prev links
		if (pLevelOther->m_idLevelNext == idLevelDest ||
			pLevelOther->m_idLevelPrev == idLevelDest ||
			(AppIsTugboat() &&								//tugboat does this a bit different. It's ok for levelnext and levelprev to be invalid links.
			 (pLevelOther->m_idLevelNext == INVALID_LINK ||	
			  pLevelOther->m_idLevelPrev == INVALID_LINK) ) )
		{
			idLevelLinkedToDest = idLevelOther;
		}
		else
		{
		
			// does this level have a link to the dest level in question
			for (int i = 0; i < pLevelOther->m_nNumLevelLinks; ++i)
			{
				const LEVEL_LINK *pLink = &pLevelOther->m_LevelLink[ i ];
				if (pLink->idLevel == idLevelDest)
				{
													
					// if we've already found one, this is going to be an error and
					// we have to think about how we're linking levels together ... for now
					// we never have more than one way into a level from the same level
					ASSERTX( idLevelLinkedToDest == INVALID_ID, "Opposite level already found - unable to determine which level is desired" );
					idLevelLinkedToDest = idLevelOther;
																	
				}
				
			}

			// level areas of depth zero are all implicitly linked together (tugboat),
			// it's important to not have physical links tying the levels because tugboat
			// will destroy entire level area chains when there are no players at any
			// of the area level depths
			if (idLevelLinkedToDest == INVALID_ID &&
				tLevelTypeSource.nLevelDepth == 0 &&
				tLevelTypeSource.nLevelArea != INVALID_LINK)
			{
				idLevelLinkedToDest = idLevelOther;
			}
						
		}
						
	}

	return idLevelLinkedToDest;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_LevelGetArrivalPortalLocation( 
	GAME *pGame,
	LEVELID idLevelDest,
	LEVELID idLevelLeaving,
	int nPrevAreaID,
	ROOM **pRoom,
	VECTOR *pvPosition,
	VECTOR *pvDirection)
{
	ASSERTX_RETFALSE( pGame, "Expected game" );
	ASSERTX_RETFALSE( pRoom, "Expected room" );	
	ASSERTX_RETFALSE( pvPosition, "Expected position" );	
	ASSERTX_RETFALSE( pvDirection, "Expected direction" );	

	// get the level to search
	LEVEL *pLevelDest = LevelGetByID( pGame, idLevelDest );
	LEVEL *pLevelLeaving = LevelGetByID( pGame, idLevelLeaving );

	if( AppIsTugboat() )
	{
		UNIT *pWarpConnecting = GetWarpConnectingLevels( pLevelDest, pLevelLeaving, nPrevAreaID );
		if( pWarpConnecting != NULL )
		{
			*pRoom = UnitGetRoom( pWarpConnecting );
			*pvPosition = UnitGetPosition( pWarpConnecting );
			*pvDirection = UnitGetFaceDirection( pWarpConnecting, TRUE );
			return TRUE;
		}
	}
	// search all level connections for the level that the player was
	// leaving (note that the actual level that they were in may really
	// be in another game instance, but we have recreated on all game
	// instances the basic level layout so we can have something real
	// to refer to like this)
	UNITID idPortal = INVALID_ID;

	for (int i = 0; i < pLevelDest->m_nNumLevelLinks; ++i)
	{
		const LEVEL_LINK *pLink = &pLevelDest->m_LevelLink[ i ];
		if (idLevelLeaving != INVALID_ID && pLink->idLevel == idLevelLeaving)
		{
			ASSERTX_CONTINUE( idPortal == INVALID_ID, "More than one link leads to the level in question" );		
			
			// save portal
			idPortal = pLink->idUnit;
			
			// keep searching to help us catch bugs where we have more than
			// one way into the same level
			
		}
	}
	if( AppIsTugboat() )
	{
		
		int nAreaIDLeaving = nPrevAreaID;
		if( nAreaIDLeaving == INVALID_ID )
		{
			nAreaIDLeaving = pLevelLeaving ? LevelGetLevelAreaID( pLevelLeaving ) : INVALID_ID;
		}
		int nAreaIDArriving = pLevelDest ? LevelGetLevelAreaID( pLevelDest ) : INVALID_ID;
		const MYTHOS_LEVELAREAS::LEVELAREA_LINK *linker = MYTHOS_LEVELAREAS::GetLevelAreaLinkByAreaID( nAreaIDArriving );
		if( linker )
		{
			if( linker->m_LevelAreaIDNext == nAreaIDLeaving )
			{

				TARGET_TYPE eTargetTypes[] = { TARGET_OBJECT, TARGET_INVALID };

				UNIT *pMarker = LevelFindClosestUnitOfType( pLevelDest, cgvNone, eTargetTypes, UNITTYPE_WARP_NEXT_LEVEL );
				if( !pMarker )
					pMarker = LevelFindClosestUnitOfType( pLevelDest, cgvNone, eTargetTypes, UNITTYPE_STAIRS_DOWN );
				if (pMarker)
				{
					idPortal = UnitGetId( pMarker );
				}

			}
			else if( linker->m_LevelAreaIDPrevious == nAreaIDLeaving )
			{
				TARGET_TYPE eTargetTypes[] = { TARGET_OBJECT, TARGET_INVALID };

				UNIT *pMarker = LevelFindClosestUnitOfType( pLevelDest, cgvNone, eTargetTypes, UNITTYPE_WARP_PREV_LEVEL );
				if( !pMarker )
					pMarker = LevelFindClosestUnitOfType( pLevelDest, cgvNone, eTargetTypes, UNITTYPE_STAIRS_UP );
				if (pMarker)
				{
					idPortal = UnitGetId( pMarker );
				}
			}
		}
	}
	// This will place the player near exit portals on the hubs.
	/*
	if( AppIsTugboat() &&
		idPortal == INVALID_ID &&
		 pLevelDest->m_nNumLevelLinks > 0 &&
		( MYTHOS_LEVELAREAS::IsLevelAreaAHub( LevelGetLevelAreaID( pLevelDest ) ) || 
		  MYTHOS_LEVELAREAS::GetLevelAreaID_First_BetweenLevelAreas( LevelGetLevelAreaID( pLevelLeaving ), LevelGetLevelAreaID( pLevelDest ) ) != LevelGetLevelAreaID( pLevelLeaving ) ))
	{
		int nIndex = RandGetNum( pGame, 0, pLevelDest->m_nNumLevelLinks - 1 );
		idPortal = pLevelDest->m_LevelLink[ nIndex ].idUnit;
	}
	*/

	if (idPortal != INVALID_ID)
	{
		UNIT *pPortal = UnitGetById( pGame, idPortal );
		
		// the rule goes like this ... "put player in front of warps" ;)
		WARP_DIRECTION eDirection = WD_FRONT;
								
		// well, there is of course an exception, sometimes the level guys 
		// reuse rooms over again, and in one situation we want it to be reversed
		const UNIT_DATA *pObjectData = ObjectGetData( NULL, pPortal );
		if (pObjectData && UnitDataTestFlag(pObjectData, UNIT_DATA_FLAG_REVERSE_ARRIVE_DIRECTION))
		{
			eDirection = WD_BEHIND;
		}

		// get position and direction
		*pRoom = WarpGetArrivePosition( pPortal, pvPosition, pvDirection, eDirection );
		return TRUE;  // found it
		
	}

	return FALSE;  // not found			
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int LevelGetNumSubLevels(
	const LEVEL *pLevel)
{
	ASSERTX_RETZERO( pLevel, "Expected level" );
	return pLevel->m_nNumSubLevels;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SUBLEVEL *LevelGetSubLevelByIndex(
	LEVEL *pLevel,
	int nIndex)
{
	ASSERTX_RETNULL( pLevel, "Expected level" );
	ASSERTX_RETNULL( nIndex >= 0 && nIndex < LevelGetNumSubLevels( pLevel ), "Invalid index" );
	return &pLevel->m_SubLevels[ nIndex ];
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int LevelGetDefaultSubLevelDefinition( 
	const LEVEL *pLevel)
{
	ASSERTX_RETINVALID( pLevel, "Expected level" );
	const LEVEL_DEFINITION *ptLevelDef = LevelDefinitionGet( pLevel );
	ASSERTX_RETINVALID( ptLevelDef, "Expected level definition" );
	return ptLevelDef->nSubLevelDefault;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SUBLEVEL *LevelGetPrimarySubLevel(
	LEVEL *pLevel)
{
	ASSERTX_RETNULL( pLevel, "Expected level" );
	return SubLevelGetByIndex( pLevel, 0 );  // first sublevel is default/primary sublevel
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LevelIterateUnits( 
	LEVEL *pLevel, 
	TARGET_TYPE *peTargetTypes, 
	PFN_ROOM_ITERATE_UNIT_CALLBACK pfnCallback,
	void *pCallbackData)
{
	ASSERTX_RETURN( pLevel, "Expected level" );
	ASSERTX_RETURN( pfnCallback, "Expected callback" );
	
	// iterate rooms
	for (ROOM *pRoom = LevelGetFirstRoom( pLevel ); pRoom; pRoom = LevelGetNextRoom( pRoom ))
	{
		if (RoomIterateUnits( pRoom, peTargetTypes, pfnCallback, pCallbackData ) == RIU_STOP)
		{
			break;
		}
	}
	
}

//----------------------------------------------------------------------------
struct LEVEL_FIND_DATA
{
	SPECIES spSpecies;
	UNIT *pResult;
	
	LEVEL_FIND_DATA::LEVEL_FIND_DATA( void )
	{
		spSpecies = SPECIES_NONE;
		pResult = NULL;
	}
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static ROOM_ITERATE_UNITS sFindFirstOf(
	UNIT * unit,
	void * data)
{
	LEVEL_FIND_DATA * findData = (LEVEL_FIND_DATA *)data;
	
	if (UnitGetSpecies(unit) == findData->spSpecies)
	{
		findData->pResult = unit;
		return RIU_STOP;
	}
	return RIU_CONTINUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT *LevelFindFirstUnitOf(
	LEVEL *pLevel,
	TARGET_TYPE *peTargetTypes,
	GENUS eGenus,
	int nClass)
{
	ASSERTX_RETNULL( pLevel, "Expected level" );
	ASSERTX_RETNULL( eGenus != GENUS_NONE, "Expected genus" );
	ASSERTX_RETNULL( nClass != INVALID_LINK, "Expected class" );
	
	// search level
	LEVEL_FIND_DATA tData;
	tData.spSpecies = MAKE_SPECIES( eGenus, nClass );
	LevelIterateUnits( pLevel, peTargetTypes, sFindFirstOf, &tData );

	// return result	
	return tData.pResult;
	
}

//----------------------------------------------------------------------------
struct LEVEL_FIND_CLOSEST_DATA
{
	int nUnitType;
	UNIT *pResult;
	VECTOR vAnchor;
	float flDistSq;
	
	LEVEL_FIND_CLOSEST_DATA::LEVEL_FIND_CLOSEST_DATA( void )
	{
		nUnitType = INVALID_LINK;
		pResult = NULL;
		vAnchor = cgvNone;
		flDistSq = 0.0f;
	}
	
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static ROOM_ITERATE_UNITS sFindClosestOf(
	UNIT *pUnit,
	void *pCallbackData)
{
	LEVEL_FIND_CLOSEST_DATA *pFindData = (LEVEL_FIND_CLOSEST_DATA *)pCallbackData;
	
	if (UnitIsA( pUnit, pFindData->nUnitType ))
	{
		
		// get distance to unit
		float flDistanceSq = VectorDistanceSquared( UnitGetPosition( pUnit ), pFindData->vAnchor );
		if (pFindData->pResult == NULL || flDistanceSq < pFindData->flDistSq)
		{
			pFindData->pResult = pUnit;
			pFindData->flDistSq = flDistanceSq;
		}
	}
	return RIU_CONTINUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT *LevelFindClosestUnitOfType(
	LEVEL *pLevel,
	const VECTOR &vPosition,
	TARGET_TYPE *peTargetTypes,
	int nUnitType)
{
	ASSERTX_RETNULL( pLevel, "Expected level" );
	ASSERTX_RETNULL( nUnitType != INVALID_LINK, "Expected unit type" );

	// search level
	LEVEL_FIND_CLOSEST_DATA tData;
	tData.nUnitType = nUnitType;
	tData.vAnchor = vPosition;
	LevelIterateUnits( pLevel, peTargetTypes, sFindClosestOf, &tData );
	
	// return result
	return tData.pResult;
	
}

//----------------------------------------------------------------------------
struct LEVEL_FIND_FARTHEST_DATA
{
	SPECIES species;
	UNIT *pResult;
	VECTOR vAnchor;
	float flDistSq;

	LEVEL_FIND_FARTHEST_DATA::LEVEL_FIND_FARTHEST_DATA( void )
	{
		species = SPECIES_NONE;
		pResult = NULL;
		vAnchor = cgvNone;
		flDistSq = 0.0f;
	}

};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static ROOM_ITERATE_UNITS sFindFarthestOf(
	UNIT *pUnit,
	void *pCallbackData)
{
	LEVEL_FIND_FARTHEST_DATA *pFindData = (LEVEL_FIND_FARTHEST_DATA *)pCallbackData;

	if (UnitGetSpecies( pUnit ) == pFindData->species)
	{
		// get distance to unit
		float flDistanceSq = VectorDistanceSquared( UnitGetPosition( pUnit ), pFindData->vAnchor );
		if (pFindData->pResult == NULL || flDistanceSq > pFindData->flDistSq)
		{
			pFindData->pResult = pUnit;
			pFindData->flDistSq = flDistanceSq;
		}
	}
	return RIU_CONTINUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT *LevelFindFarthestUnitOf(
	LEVEL *pLevel,
	const VECTOR &vPosition,
	TARGET_TYPE *peTargetTypes,
	GENUS eGenus,
	int nClass)
{
	ASSERTX_RETNULL( pLevel, "Expected level" );
	ASSERTX_RETNULL( eGenus != GENUS_NONE, "Expected genus" );
	ASSERTX_RETNULL( nClass != INVALID_LINK, "Expected class" );

	// search level
	LEVEL_FIND_FARTHEST_DATA tData;
	tData.species = MAKE_SPECIES( eGenus, nClass );
	tData.vAnchor = vPosition;
	LevelIterateUnits( pLevel, peTargetTypes, sFindFarthestOf, &tData );

	// return result
	return tData.pResult;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static ROOM_ITERATE_UNITS sLevelAddSpeciesToPicker( UNIT *pUnit, void *pCallbackData )
{
	ASSERT_RETVAL( pUnit && pCallbackData, RIU_STOP );
	if (UnitGetSpecies( pUnit ) == *((SPECIES*) pCallbackData))
	{
		PickerAdd( MEMORY_FUNC_FILELINE( UnitGetGame( pUnit ), UnitGetId( pUnit ), 1));
	}
	return RIU_CONTINUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT *LevelFindRandomUnitOf(
	LEVEL *pLevel,
	TARGET_TYPE *peTargetTypes,
	GENUS eGenus,
	int nClass)
{
	ASSERTX_RETNULL( pLevel, "Expected level" );
	ASSERTX_RETNULL( eGenus != GENUS_NONE, "Expected genus" );
	ASSERTX_RETNULL( nClass != INVALID_LINK, "Expected class" );

	GAME *pGame = LevelGetGame( pLevel );
	PickerStart( pGame, picker );

	// find marker
	SPECIES species = MAKE_SPECIES( GENUS_OBJECT, nClass );
	LevelIterateUnits( 
		pLevel, 
		peTargetTypes, 
		sLevelAddSpeciesToPicker,
		&species);

	UNITID idUnit = PickerChoose( pGame );
	return (idUnit == INVALID_ID) ? NULL : UnitGetById( pGame, idUnit );
}

//----------------------------------------------------------------------------
// Returns the number of monsters in the level. If a monster class hierarchy
// is specified, only count those monsters which have monster class
// equal to, or are a child monster class of, nMonsterClassHierarchy.
//----------------------------------------------------------------------------
int LevelCountMonsters(
	LEVEL *pLevel,
	int nMonsterClassHierarchy,
	int nUnitType)
{
	int nMonsters = 0;
	for ( ROOM * room = LevelGetFirstRoom( pLevel ); room; room = LevelGetNextRoom( room ) )
	{
		for ( UNIT * test = room->tUnitList.ppFirst[TARGET_BAD]; test; test = test->tRoomNode.pNext )
		{
			// must match the unit type if specified, else match the monster 
			// class (but not if checking unit type, since multiple monster 
			// class hierarchies can match same unit type)
			if ( nUnitType != INVALID_LINK )
			{
				if ( UnitIsA( test, nUnitType ) )
					++nMonsters;
			}
			else if ( UnitIsA( test, UNITTYPE_MONSTER ) &&			// no destructibles
				      ( nMonsterClassHierarchy == INVALID_LINK ||
						UnitIsInHierarchyOf( 
							GENUS_MONSTER, 
							UnitGetClass( test ),
							&nMonsterClassHierarchy,
							1 ) ) )
			{
				++nMonsters;
			}
		}
	}
	return nMonsters;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int s_LevelCountMonstersUseEstimate(
	LEVEL *pLevel)
{
	int nMonsters = 0;
	TARGET_TYPE eTargetTypes[ ] = { TARGET_BAD, TARGET_INVALID };	
	for ( ROOM * room = LevelGetFirstRoom( pLevel ); room; room = LevelGetNextRoom( room ) )
	{
		if(RoomTestFlag(room, ROOM_POPULATED_BIT))
		{
			nMonsters += RoomGetMonsterCount( room, RMCT_DENSITY_VALUE_OVERRIDE, eTargetTypes );		
		}
		else
		{
			nMonsters += s_RoomGetEstimatedMonsterCount(room);
		}
	}
	return nMonsters;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LevelIteratePlayers( 
	LEVEL *pLevel, 
	PFN_ITERATE_PLAYERS pfnCallback,
	void *pCallbackData)
{
	ASSERTX_RETURN( pLevel, "Expected level" );
	ASSERTX_RETURN( pfnCallback, "Expected callback" );
	GAME *pGame = LevelGetGame( pLevel );

	// for now, we iterate clients in the game, if this gets too big we
	// will need to keep a shorter list in the level of the clients or something
	GAMECLIENT *pClientNext = NULL;
	for (GAMECLIENT *pClient = ClientGetFirstInGame( pGame );
		 pClient;
		 pClient = pClientNext)
	{
	
		// get next (allows delete if desired)
		pClientNext = ClientGetNextInGame( pClient );

		// get player		
		UNIT *pPlayer = ClientGetControlUnit( pClient );
		
		// check level
		LEVEL *pLevelPlayer = UnitGetLevel( pPlayer );
		if (pLevel == pLevelPlayer)
		{
			// do callback
			pfnCallback( pPlayer, pCallbackData );			
		}
				
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ThemeIsA(
	const int nLevelTheme,
	const int nTestTheme)
{
	if(nLevelTheme < 0 || nTestTheme < 0)
		return FALSE;

	if(nLevelTheme == nTestTheme)
		return TRUE;

	const LEVEL_THEME * pLevelTheme = (const LEVEL_THEME*)ExcelGetData(NULL, DATATABLE_LEVEL_THEMES, nLevelTheme);
	ASSERTX_RETFALSE(pLevelTheme, "Level Theme is not valid!");
	for(int i=0; i<MAX_LEVEL_THEME_ISA; i++)
	{
		if(pLevelTheme->nIsA[i] >= 0 && ThemeIsA(pLevelTheme->nIsA[i], nTestTheme))
		{
			return TRUE;
		}
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL DRLGThemeIsA(
	const DRLG_DEFINITION * pDRLGDef,
	const int nTestTheme)
{
	ASSERT_RETFALSE(pDRLGDef);
	for(int i=0; i<MAX_DRLG_THEMES; i++)
	{
		if(ThemeIsA(pDRLGDef->nThemes[i], nTestTheme))
			return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL LevelThemeIsA(
	const LEVEL * pLevel,
	const int nTestTheme)
{
	ASSERT_RETFALSE(pLevel);
	const DRLG_DEFINITION * pDRLGDef = LevelGetDRLGDefinition(pLevel);
	ASSERT_RETFALSE(pDRLGDef);
	return DRLGThemeIsA(pDRLGDef, nTestTheme);
}

//Client side only!
static ROOM_ITERATE_UNITS cUpdateVisibilityOfUnitsOnClient(
	UNIT *pUnit,
	void *pCallbackData)
{
	c_UnitSetVisibilityOnClientByThemesAndGameEvents( pUnit );
	return RIU_CONTINUE;
}

void sSetRoomsVisible( 
	ROOM *pRoom, 
	void *pCallbackData )
{
#if !ISVERSION(SERVER_VERSION)
	LEVEL *pLevel = RoomGetLevel( pRoom );
	int *pThemes( NULL );
	LevelGetSelectedThemes( pLevel, &pThemes );
	int nCount = pRoom->pLayoutSelections->pRoomLayoutSelections[ROOM_LAYOUT_ITEM_GRAPHICMODEL].nCount;
	for(int i=0; i<nCount; i++)
	{
		ROOM_LAYOUT_GROUP * pModel = pRoom->pLayoutSelections->pRoomLayoutSelections[ROOM_LAYOUT_ITEM_GRAPHICMODEL].pGroups[i];
		ASSERT_CONTINUE(pModel);
		if( pModel->nTheme > 0 )
		{
			
			BOOL bIsVisible( FALSE );			
			for( int nThemeIndex = 0; nThemeIndex < MAX_THEMES_PER_LEVEL; nThemeIndex++ )
			{
				if( pThemes[ nThemeIndex ] != INVALID_ID &&					
					ThemeIsA( pModel->nTheme, pThemes[ nThemeIndex ]) )
				{
					bIsVisible = TRUE;
					break;	//it's visible
				}
			}
			if( pModel->dwFlags & ROOM_LAYOUT_FLAG_NOT_THEME )
				bIsVisible = !bIsVisible;
			V( e_ModelSetFlagbit( pRoom->pLayoutSelections->pPropModelIds[i], MODEL_FLAGBIT_NODRAW, !bIsVisible ) );

		}

	}

	nCount = pRoom->pLayoutSelections->pRoomLayoutSelections[ROOM_LAYOUT_ITEM_ATTACHMENT].nCount;
	for(int i=0; i<nCount; i++)
	{
		ROOM_LAYOUT_GROUP * pLayoutGroup = pRoom->pLayoutSelections->pRoomLayoutSelections[ROOM_LAYOUT_ITEM_ATTACHMENT].pGroups[i];
		ASSERT_CONTINUE(pLayoutGroup);
		if( pLayoutGroup->nTheme > 0 )
		{
			ATTACHMENT * pAttachment = c_ModelAttachmentGet( RoomGetRootModel( pRoom ), pLayoutGroup->nLayoutId);			
			if( pAttachment ) 
			{
				BOOL bIsVisible( FALSE );	
				for( int nThemeIndex = 0; nThemeIndex < MAX_THEMES_PER_LEVEL; nThemeIndex++ )
				{
					if( pThemes[ nThemeIndex ] != INVALID_ID &&					
						ThemeIsA( pLayoutGroup->nTheme, pThemes[ nThemeIndex ]) )
					{
						bIsVisible = TRUE;
						break;	//it's visible
					}
				}
				c_AttachmentSetDraw( *pAttachment, bIsVisible );
			}			
		}

	}
		
#endif
}

//Client side only. This will set units visible or not 
//if a theme changes. 
void cLevelUpdateUnitsVisByThemes( LEVEL *pLevel )
{
#if !ISVERSION(SERVER_VERSION)
	ASSERT_RETURN( pLevel );
	ASSERT_RETURN( IS_CLIENT( LevelGetGame( pLevel ) ) );
	if( pLevel->m_LevelAreaOverrides.bAllowsDiffClientAndServerThemes == FALSE )
		return;	//no reason for doing this if the level doesn't allow it.
	pLevel->m_bThemesPopulated = FALSE;
	pLevel->m_nThemeIndexCount = 0;	
	LevelIterateUnits( pLevel, 0, cUpdateVisibilityOfUnitsOnClient, NULL );
	LevelIterateRooms( pLevel, sSetRoomsVisible, NULL );
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sLevelIsThemeValid(
	GAME * pGame,
	const LEVEL_THEME * pLevelTheme)
{
	if(pLevelTheme->nGlobalThemeRequired >= 0 && !GlobalThemeIsEnabled(pGame, pLevelTheme->nGlobalThemeRequired))
	{
		return FALSE;
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sLevelIsThemeValid(
	GAME * pGame,
	const int nLevelTheme)
{
	const LEVEL_THEME * pLevelTheme = (const LEVEL_THEME *)ExcelGetData(pGame, DATATABLE_LEVEL_THEMES, nLevelTheme);
	ASSERT_RETFALSE(pLevelTheme);
	return sLevelIsThemeValid(pGame, pLevelTheme);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL LevelHasThemeEnabled(
	LEVEL * pLevel,
	int nThemeToTest )
{
	ASSERT_RETFALSE( pLevel );

	int *pnThemesEnabled( NULL );
	int nThemeCount = LevelGetSelectedThemes( pLevel, &pnThemesEnabled );

	for ( int i = 0; i < nThemeCount; i++ )
	{
		if ( ThemeIsA( nThemeToTest, pnThemesEnabled[ i ] ) )
			return TRUE;
	}
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LevelThemeForce(
	LEVEL * pLevel,
	int nTheme,
	BOOL bForceOn )
{
	ASSERT_RETURN(pLevel);
	ASSERT_RETURN(nTheme != INVALID_ID);
	{
		int *pnThemesEnabled( NULL );
		LevelGetSelectedThemes( pLevel, &pnThemesEnabled );
	}

	for ( int i = 0; i < pLevel->m_nThemeIndexCount; i++ )
	{
		if ( pLevel->m_pnThemes[ i ] == nTheme )
		{
			if ( bForceOn )
				return;
			pLevel->m_pnThemes[ i ] = pLevel->m_pnThemes[ pLevel->m_nThemeIndexCount - 1 ];
			pLevel->m_nThemeIndexCount--;
			break;
		}
	}
	if ( bForceOn )
	{
		ASSERT_RETURN( pLevel->m_nThemeIndexCount < MAX_THEMES_PER_LEVEL );
		pLevel->m_pnThemes[ pLevel->m_nThemeIndexCount ] = nTheme;
		pLevel->m_nThemeIndexCount++;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int LevelGetSelectedThemes(
	LEVEL * pLevel,
	int ** pnThemes)
{
	ASSERT_RETINVALID(pLevel);
	ASSERT_RETINVALID(pnThemes);
	
	if(pLevel->m_bThemesPopulated)
	{
		*pnThemes = pLevel->m_pnThemes;
		return pLevel->m_nThemeIndexCount;
	}

	GAME * pGame = LevelGetGame(pLevel);
	const LEVEL_DEFINITION * pLevelDef = LevelDefinitionGet(pLevel);
	const DRLG_DEFINITION * pDRLGDef = LevelGetDRLGDefinition(pLevel);

	pLevel->m_bThemesPopulated = TRUE;
	for(int i=0; i<MAX_THEMES_PER_LEVEL; i++)
	{
		pLevel->m_pnThemes[i] = INVALID_ID;
	}

	pLevel->m_nThemeIndexCount = 0;
	if(pDRLGDef)
	{
		for(int i=0; i<MAX_DRLG_THEMES; i++)
		{
			if(pDRLGDef->nThemes[i] >= 0 && sLevelIsThemeValid(pGame, pDRLGDef->nThemes[i]))
			{
				pLevel->m_pnThemes[pLevel->m_nThemeIndexCount] = pDRLGDef->nThemes[i];
				pLevel->m_nThemeIndexCount++;
			}
		}
	}
	int nThemesFromWeatherCount = pLevel->m_pWeather ? MAX_THEMES_FROM_WEATHER : 0;
	for(int i=0; i<nThemesFromWeatherCount; i++)
	{
		if(pLevel->m_pWeather->nThemes[i] >= 0 && sLevelIsThemeValid(pGame, pLevel->m_pWeather->nThemes[i]))
		{
			pLevel->m_pnThemes[pLevel->m_nThemeIndexCount] = pLevel->m_pWeather->nThemes[i];
			pLevel->m_nThemeIndexCount++;
		}
	}

	// Note: We don't do the sLevelThemeIsValid() check on these themes.  Travis or Marsh, should we?
	if( AppIsTugboat() )
	{
		QuestGetThemes( pLevel );	//fill the level with quest themes
		MYTHOS_LEVELAREAS::LevelAreaFillLevelWithRandomTheme( pLevel,	//fill the level with level area themes.
															  pLevel->m_LevelAreaOverrides.pLevelArea );
		MYTHOS_LEVELAREAS::LevelAreaPopulateThemes( pLevel );

	}

	if(pGame && pLevelDef && pDRLGDef)
	{
		if(pLevelDef->nSelectRandomThemePct > 0 && RandGetNum(pGame, 100) < (DWORD)pLevelDef->nSelectRandomThemePct)
		{
			PickerStart(pGame, picker);

			// Add a "no theme" item
			PickerAdd(MEMORY_FUNC_FILELINE(pGame, INVALID_ID, 1));

			// Add all of the legitimate themes from the themes spreadsheet
			int nThemeCount = ExcelGetNumRows(pGame, DATATABLE_LEVEL_THEMES);
			for(int i=0; i<nThemeCount; i++)
			{
				const LEVEL_THEME * pLevelTheme = (const LEVEL_THEME *)ExcelGetData(pGame, DATATABLE_LEVEL_THEMES, i);
				ASSERT_CONTINUE(pLevelTheme);

				if(!sLevelIsThemeValid(pGame, pLevelTheme))
				{
					continue;
				}

				for(int j=0; j<MAX_LEVEL_THEME_ALLOWED_STYLES; j++)
				{
					if (pLevelTheme->nAllowedStyles[i] == pDRLGDef->nStyle)
					{
						PickerAdd(MEMORY_FUNC_FILELINE(pGame, i, 1));
						break;
					}
				}
			}
			int nRandomTheme = PickerChoose(pGame);
			if(nRandomTheme >= 0)
			{
				pLevel->m_pnThemes[pLevel->m_nThemeIndexCount] = nRandomTheme;
				pLevel->m_nThemeIndexCount++;
			}
		}
	}

	// As a last step, find out if any of the themes are Highlander, and eliminate all of the other ones
	for(int i=0; i<pLevel->m_nThemeIndexCount; i++)
	{
		int nTheme = pLevel->m_pnThemes[i];
		if(nTheme < 0)
			continue;
		
		const LEVEL_THEME * pLevelTheme = (const LEVEL_THEME *)ExcelGetData(pGame, DATATABLE_LEVEL_THEMES, nTheme);
		if(!pLevelTheme)
			continue;

		if(pLevelTheme->bHighlander)
		{
			pLevel->m_pnThemes[0] = nTheme;
			pLevel->m_nThemeIndexCount = 1;
			break;
		}
	}

	*pnThemes = pLevel->m_pnThemes;
	return pLevel->m_nThemeIndexCount;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LevelIterateRooms(
	LEVEL *pLevel,
	PFN_LEVEL_ITERATE_ROOMS pfnCallback,
	void *pUserData)
{
	ASSERTX_RETURN( pLevel, "Expected level" );
	ASSERTX_RETURN( pfnCallback, "Expected callback" );
	
	for (ROOM *pRoom = LevelGetFirstRoom( pLevel ); pRoom; pRoom = LevelGetNextRoom( pRoom ))
	{
		pfnCallback( pRoom, pUserData );
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct MONSTER_COUNT
{
	int nCount;
	const TARGET_TYPE * peTargetTypes;
	ROOM_MONSTER_COUNT_TYPE eCountType;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCountMonsters( 
	ROOM *pRoom, 
	void *pCallbackData )
{
	MONSTER_COUNT * pMCount = (MONSTER_COUNT *) pCallbackData;
	pMCount->nCount += RoomGetMonsterCount( pRoom, pMCount->eCountType, pMCount->peTargetTypes );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int LevelGetMonsterCount(
	LEVEL *pLevel,
	ROOM_MONSTER_COUNT_TYPE eCountType,
	const TARGET_TYPE * peTargetTypes)
{
	ASSERTX_RETZERO( pLevel, "Expected level" );
	MONSTER_COUNT tMonsterCount;
	tMonsterCount.nCount = 0;
	tMonsterCount.peTargetTypes = peTargetTypes;
	tMonsterCount.eCountType = eCountType;
	LevelIterateRooms( pLevel, sCountMonsters, &tMonsterCount );
	return tMonsterCount.nCount;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int LevelGetMonsterCountLivingAndBad(
	LEVEL *pLevel,
	ROOM_MONSTER_COUNT_TYPE eCountType /*= RMCT_ABSOLUTE*/)
{
	TARGET_TYPE peTargetTypes[] = { TARGET_BAD, TARGET_INVALID };
	return LevelGetMonsterCount( pLevel, eCountType, peTargetTypes );
}

//----------------------------------------------------------------------------
// Returns the number of rooms, picked randomly, which are available 
// for spawning of monsters or objects. Pass a ROOM* array and max desired
// rooms as pRooms and nMaxRooms.
//----------------------------------------------------------------------------
int s_LevelGetRandomSpawnRooms(
	GAME* pGame,
	LEVEL* pLevel,
	ROOM** pRooms,
	int nMaxRooms)
{
	// setup preferred flags
	DWORD dwFlags = 0;
	// required flags
	SETBIT( dwFlags, RRF_MUST_ALLOW_MONSTER_SPAWN_BIT );
	SETBIT( dwFlags, RRF_PATH_NODES_BIT );

	// optional flags
	SETBIT( dwFlags, RRF_SPAWN_POINTS_BIT );
	SETBIT( dwFlags, RRF_HAS_MONSTERS_BIT );
	SETBIT( dwFlags, RRF_PLAYERLESS_BIT );
	SETBIT( dwFlags, RRF_NO_ROOMS_WITH_NO_SPAWN_NODES_BIT );
	if (AppIsHellgate())
	{
		SETBIT( dwFlags, RRF_ACTIVE_BIT );
	}

	SUBLEVEL *pSubLevel = LevelGetPrimarySubLevel( pLevel );
	int nRooms = s_SubLevelGetRandomRooms( pSubLevel, pRooms, nMaxRooms, dwFlags, NULL, NULL );

	// try fall back room search options
	if (nRooms < nMaxRooms)
	{
		CLEARBIT( dwFlags, RRF_HAS_MONSTERS_BIT );
		nRooms = s_SubLevelGetRandomRooms( pSubLevel, pRooms, nMaxRooms, dwFlags, NULL, NULL  );
		if (nRooms < nMaxRooms)
		{
			CLEARBIT( dwFlags, RRF_PLAYERLESS_BIT );		
			nRooms = s_SubLevelGetRandomRooms( pSubLevel, pRooms, nMaxRooms, dwFlags, NULL, NULL );
			if (nRooms < nMaxRooms)
			{
				if (AppIsHellgate())
				{
					CLEARBIT( dwFlags, RRF_ACTIVE_BIT);		
					nRooms = s_SubLevelGetRandomRooms( pSubLevel, pRooms, nMaxRooms, dwFlags, NULL, NULL );
				}

				if (nRooms < nMaxRooms)
				{
					CLEARBIT( dwFlags, RRF_NO_ROOMS_WITH_NO_SPAWN_NODES_BIT );		
					nRooms = s_SubLevelGetRandomRooms( pSubLevel, pRooms, nMaxRooms, dwFlags, NULL, NULL );
					if (nRooms < nMaxRooms)
					{
						CLEARBIT( dwFlags, RRF_SPAWN_POINTS_BIT );		
						nRooms = s_SubLevelGetRandomRooms( pSubLevel, pRooms, nMaxRooms, dwFlags, NULL, NULL );
					}
				}
			}
		}
	}

	return nRooms;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LevelNeedRoomUpdate(
	LEVEL *pLevel,
	BOOL bNeedUpdate)
{
	ASSERTX_RETURN( pLevel, "Expected level" );
	pLevel->m_bNeedRoomUpdate = bNeedUpdate;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float LevelGetMaxTreasureDropRadius(
	LEVEL *pLevel)
{
	ASSERTX_RETZERO( pLevel, "Expected level" );
	const DRLG_DEFINITION *ptDRLGDef = LevelGetDRLGDefinition( pLevel );
	return ptDRLGDef ? ptDRLGDef->flMaxTreasureDropRadius : 0.0f;
}

//----------------------------------------------------------------------------
struct ADD_TEXTURE_DATA
{
	BOOL bError;
	PAK_ENUM ePakEnum;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAddTextureToPak( 
	const FILE_ITERATE_RESULT *pResult, 
	void *pUserData)
{
	ADD_TEXTURE_DATA *pAddTextureData = (ADD_TEXTURE_DATA *)pUserData;
	
	// add DDS file to pak
	DECLARE_LOAD_SPEC( tSpec, pResult->szFilenameRelativepath );
	tSpec.flags |= PAKFILE_LOAD_ADD_TO_PAK | PAKFILE_LOAD_FREE_BUFFER | PAKFILE_LOAD_ADD_TO_PAK_IMMEDIATE | PAKFILE_LOAD_IMMEDIATE;
	tSpec.pakEnum = pAddTextureData->ePakEnum;
	PakFileLoadNow( tSpec );

	// construct .tga name
	char szFilename[ DEFAULT_FILE_WITH_PATH_SIZE ];
	PStrReplaceExtension( szFilename, DEFAULT_FILE_WITH_PATH_SIZE, pResult->szFilenameRelativepath, "tga" );

	// construct path to definition	
	char szFileWithFullPath[ MAX_PATH ];
	FileGetFullFileName( szFileWithFullPath, szFilename, MAX_PATH );			
	if (FileExists( szFileWithFullPath ))
	{
		int nID = DefinitionGetIdByFilename( 
						DEFINITION_GROUP_TEXTURE, 
						szFileWithFullPath, 
						-1,
						TRUE,
						FALSE,
						pAddTextureData->ePakEnum);
		if (nID == INVALID_ID)
		{
			pAddTextureData->bError = TRUE;
		}
	}	
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sAddLocalizedTexturesToPak( 
	const LEVEL_FILE_PATH *pFilePath)
{

	ADD_TEXTURE_DATA tAddTextureData;
	tAddTextureData.bError = FALSE;
	tAddTextureData.ePakEnum = pFilePath->ePakfile;
	
	// construct path to directory
	char szDirectory[ MAX_PATH ];
	PStrPrintf( szDirectory, MAX_PATH, "%s%s", FILE_PATH_DATA, pFilePath->pszPath );
	
	// get absolute path to dir
	WCHAR szFullPath[ MAX_PATH ];	
	FileGetFullFileName( szFullPath, szDirectory, MAX_PATH );			
	
	// scan all files in this directory
	FilesIterateInDirectory( szFullPath, L"*.dds", sAddTextureToPak, &tAddTextureData, FALSE );

	return tAddTextureData.bError;
		
}
			
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_LevelLoadLocalizedTextures(
	GAME *pGame,
	int nLevelFilePath,
	int nSKU)
{

	// get file path info
	const LEVEL_FILE_PATH *pFilePath = LevelFilePathGetData( nLevelFilePath );

	// get the localized folders that this file path refers to
	int nLocalizedFolders[ LEVEL_FILE_PATH_MAX_LOCALIZED_FOLDERS ];
	int nNumLocalizedFolders = LevelFilePathGetLocalizedFolders( pFilePath, nLocalizedFolders, LEVEL_FILE_PATH_MAX_LOCALIZED_FOLDERS );
	if (nNumLocalizedFolders > 0)
	{
	
		// go through each folder
		for (int i = 0; i < nNumLocalizedFolders; ++i)
		{
			int nFilePath = nLocalizedFolders[ i ];
			const LEVEL_FILE_PATH *pFilePathLocalized = LevelFilePathGetData( nFilePath );
			if (pFilePathLocalized)
			{
			
				// what language is this folder localized in
				LANGUAGE eLanguage = pFilePathLocalized->eLanguage;
				
				// if any language in the SKU we're packing uses that language for its
				// background textures, we're going to use it
				if (SKUUsesBackgroundTextureLanguage( nSKU, eLanguage ))
				{
				
					// add contents of this folder
					sAddLocalizedTexturesToPak( pFilePathLocalized );
					
				}
				
			}

		}
		
	}

	return TRUE;
			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_LevelSendStartLoad( 
	GAME *pGame,
	GAMECLIENTID idClient,
	int nLevelDef, 
	int nLevelDepth, 
	int nLevelArea,
	int nDRLGDef)
{
	ASSERTX_RETURN( pGame, "Expected game" );
	ASSERTX_RETURN( IS_SERVER( pGame ), "Server only" );
	ASSERTX_RETURN( idClient != INVALID_CLIENTID, "Expected client id" );
	
	MSG_SCMD_START_LEVEL_LOAD tMessage;
	tMessage.nLevelDefinition = nLevelDef;
	tMessage.nLevelDepth = nLevelDepth;
	tMessage.nLevelArea = nLevelArea;
	tMessage.nDRLGDefinition = nDRLGDef;

	s_SendMessage( pGame, idClient, SCMD_START_LEVEL_LOAD, &tMessage );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LevelDRLGChoiceInit(
	LEVEL_DRLG_CHOICE &tDRLGChoice)
{

	tDRLGChoice.nDRLGWeight = 0;
	tDRLGChoice.nDRLG = INVALID_LINK;
	tDRLGChoice.nSpawnClass = INVALID_LINK;
	tDRLGChoice.nMusicRef = INVALID_LINK;
	tDRLGChoice.nNamedMonsterClass = INVALID_LINK;
	tDRLGChoice.fNamedMonsterChance = 0.0f;
	
}			

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LevelDRLGChoiceGetPossilbeMonsters(
	GAME *pGame,
	const LEVEL_DRLG_CHOICE *pDRLGChoice,
	int *pnMonsterClassBuffer,
	int nMaxBufferSize,
	int *pnCurrentBufferCount,
	BOOL bUseSpawnClassMemory,
	LEVEL *pLevel)
{						
	ASSERTX_RETURN( pGame, "Expected game" );
	ASSERTX_RETURN( pDRLGChoice, "Expected DRLG choice" );
	ASSERTX_RETURN( pnMonsterClassBuffer, "Expected monster class buffer" );
	ASSERTX_RETURN( nMaxBufferSize > 0, "Invalid buffer size" );
	ASSERTX_RETURN( pnCurrentBufferCount, "Expected current buffer count pointer" );
	
	// must have a level to use spawn class memory
	if (bUseSpawnClassMemory == TRUE)
	{
		ASSERTX_RETURN( pLevel, "Must have a level to use spawn class memory" );
	}
		
	// regular spawn classes
	s_SpawnClassGetPossibleMonsters( 
		pGame, 
		pLevel, 
		pDRLGChoice->nSpawnClass,
		pnMonsterClassBuffer, 
		nMaxBufferSize, 
		pnCurrentBufferCount,
		bUseSpawnClassMemory);

	// named monsters
	MonsterAddPossible( 
		pLevel,
		INVALID_LINK,
		pDRLGChoice->nNamedMonsterClass,
		pnMonsterClassBuffer, 
		nMaxBufferSize, 
		pnCurrentBufferCount);
					
	// drlg layouts, if we have a specific level in question we will check only the
	// actual layouts selected, otherwise we will get all the possibilities
	if (pLevel)
	{
		LevelGetLayoutMonsters( 
			pLevel, 
			bUseSpawnClassMemory, 
			pnMonsterClassBuffer, 
			nMaxBufferSize, 
			pnCurrentBufferCount);
	}
	else
	{
		DRLGGetPossibleMonsters(
			pGame,
			pDRLGChoice->nDRLG,
			pnMonsterClassBuffer, 
			nMaxBufferSize, 
			pnCurrentBufferCount);
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LevelDefGetPossibleMonsters(
	GAME *pGame,
	int nLevelDef,
	DWORD dwPossibleFlags,
	int *pnMonsterClassBuffer,
	int *pnCurrentBufferCount,
	int nMaxBufferSize)
{

	if (nLevelDef != INVALID_LINK)
	{	
		const LEVEL_DEFINITION *ptLevelDef = LevelDefinitionGet( nLevelDef );
		if (ptLevelDef)
		{

			// go through all DRLG choices
			if (TESTBIT( dwPossibleFlags, PF_DRLG_ALL_BIT ))
			{
				int nNumRows = ExcelGetNumRows(pGame, DATATABLE_LEVEL_DRLG_CHOICE);
				for(int j=0; j<nNumRows; j++)
				{
					const LEVEL_DRLG_CHOICE * pDRLGChoice = (const LEVEL_DRLG_CHOICE *)ExcelGetData(pGame, DATATABLE_LEVEL_DRLG_CHOICE, j);
					if(!pDRLGChoice)
						continue;

					if(pDRLGChoice->nLevel == nLevelDef)
					{
						LevelDRLGChoiceGetPossilbeMonsters(	
							pGame,
							pDRLGChoice,
							pnMonsterClassBuffer,
							nMaxBufferSize,
							pnCurrentBufferCount,
							FALSE,
							NULL);
					}
				}
			}

			// quest spawns
			s_SpawnClassGetPossibleMonsters( 
				pGame, 
				NULL,
				ptLevelDef->nQuestSpawnClass,
				pnMonsterClassBuffer, 
				nMaxBufferSize, 
				pnCurrentBufferCount,
				FALSE);

			// interact spawns
			s_SpawnClassGetPossibleMonsters( 
				pGame, 
				NULL,
				ptLevelDef->nInteractableSpawnClass,
				pnMonsterClassBuffer, 
				nMaxBufferSize, 
				pnCurrentBufferCount,
				FALSE);

		}

	}

}

//----------------------------------------------------------------------------
struct ROOM_LAYOUT_MONSTERS_DATA
{
	int *pnBuffer;
	int nMaxBuffer;
	int *pnCurrentBufferCount;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRoomLayoutGroupGetMonsterClasses(
	const ROOM_LAYOUT_GROUP *pLayout,
	ROOM_LAYOUT_MONSTERS_DATA *pRoomLayoutMonstersData)
{
	if (pLayout->eType == ROOM_LAYOUT_ITEM_SPAWNPOINT && pLayout->dwUnitType == ROOM_SPAWN_MONSTER)
	{
		
		int nMonsterClass = ExcelGetLineByCode( NULL, DATATABLE_MONSTERS, pLayout->dwCode );
		if (nMonsterClass != INVALID_LINK)
		{
			MonsterAddPossible(
				NULL,		// note we're not passing the level in so we don't check any spawning validity
				INVALID_LINK,
				nMonsterClass,
				pRoomLayoutMonstersData->pnBuffer,
				pRoomLayoutMonstersData->nMaxBuffer,
				pRoomLayoutMonstersData->pnCurrentBufferCount);
				
		}
	}			

	// do child groups
	for (int i = 0; i < pLayout->nGroupCount; ++i)
	{
		const ROOM_LAYOUT_GROUP *pChildGroup = &pLayout->pGroups[ i ];
		sRoomLayoutGroupGetMonsterClasses( pChildGroup, pRoomLayoutMonstersData );
	}
			
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRoomGetLayoutMonsterClasses(
	ROOM *pRoom,
	void *pCallbackData)
{
	ROOM_LAYOUT_MONSTERS_DATA *pRoomLayoutMonstersData = (ROOM_LAYOUT_MONSTERS_DATA *)pCallbackData;
	
	// get layout selection for this room	
	const ROOM_LAYOUT_SELECTION *pLayoutSelection = RoomGetLayoutSelection( pRoom, ROOM_LAYOUT_ITEM_SPAWNPOINT );	
	if (pLayoutSelection)
	{

		// check each layout group
		for (int i = 0; i < pLayoutSelection->nCount; ++i)
		{
			const ROOM_LAYOUT_GROUP *pSpawnPoint = pLayoutSelection->pGroups[ i ];
			ASSERTX_BREAK( pSpawnPoint->eType == ROOM_LAYOUT_ITEM_SPAWNPOINT, "Expected spawn point" );
			sRoomLayoutGroupGetMonsterClasses( pSpawnPoint, pRoomLayoutMonstersData );
		}
		
	}	
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LevelGetLayoutMonsters( 
	LEVEL *pLevel,
	BOOL bUseSpawnClassMemory, 
	int *pnMonsterClassBuffer, 
	int nMaxBufferSize, 
	int *pnCurrentBufferCount)
{
	ASSERTX_RETURN( pLevel, "Expected level" );
	ASSERTX_RETURN( pnMonsterClassBuffer, "Expected buffer" );
	ASSERTX_RETURN( nMaxBufferSize > 0, "Invalid buffer size" );
	ASSERTX_RETURN( pnCurrentBufferCount, "Expected current buffer count index" );

	// setup callback data
	ROOM_LAYOUT_MONSTERS_DATA tRoomLayoutMonstersData;
	tRoomLayoutMonstersData.pnBuffer = pnMonsterClassBuffer,
	tRoomLayoutMonstersData.nMaxBuffer = nMaxBufferSize;
	tRoomLayoutMonstersData.pnCurrentBufferCount = pnCurrentBufferCount;

	// iterate all rooms in level	
	LevelIterateRooms( pLevel, sRoomGetLayoutMonsterClasses, &tRoomLayoutMonstersData );
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LevelExecutePlayerEnterLvlScript(
    LEVEL *pLevel,
	UNIT *pUnit )
{
	ASSERT_RETURN( pLevel );
	ASSERT_RETURN( pUnit );
	if( pLevel->m_pLevelDefinition->codePlayerEnterLevel == INVALID_CODE )
		return;
	int code_len = 0;
	BYTE * code_ptr = ExcelGetScriptCode( LevelGetGame( pLevel ), DATATABLE_LEVEL, pLevel->m_pLevelDefinition->codePlayerEnterLevel, &code_len);		
	VMExecI( LevelGetGame( pLevel ),
			 pUnit, 
			 pUnit,
			 0,
			 INVALID_ID,
			 INVALID_LINK,
			 INVALID_ID,
			 INVALID_SKILL_LEVEL,
			 INVALID_LINK, 
			 code_ptr, 
			 code_len);

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int LevelDefinitionGetAct(
	int nLevelDef)
{
	ASSERTX_RETINVALID( nLevelDef != INVALID_LINK, "Expected level def index" );
	const LEVEL_DEFINITION *pLevelDef = LevelDefinitionGet( nLevelDef );
	ASSERTX_RETINVALID( pLevelDef, "Expected level def" );
	return pLevelDef->nAct;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int LevelGetAct(
	const LEVEL *pLevel)
{
	ASSERTX_RETINVALID( pLevel, "Expected level" );
	return LevelDefinitionGetAct( LevelGetDefinitionIndex( pLevel ) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sLevelDefinitionIsAvailableToGame(
	int nLevelDef)
{
	if (AppIsHellgate())
	{
		ASSERTX_RETFALSE( nLevelDef != INVALID_LINK, "Expected level def link" );
		int nAct = LevelDefinitionGetAct( nLevelDef );
		if (nAct != INVALID_LINK)
		{
			return ActIsAvailable( nAct );
		}
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL LevelIsAvailable(
	UNIT *pUnit,
	LEVEL *pLevel)
{
	ASSERTV_RETFALSE( pUnit, "Expected unit to ask if level is available" );
	ASSERTX_RETFALSE( pLevel, "Expected level" );
	int nLevelDef = LevelGetDefinitionIndex( pLevel );
	return LevelDefIsAvailableToUnit( pUnit, nLevelDef ) == WRR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int LevelDestinationValidate(
	UNIT *pPlayer,
	int nLevelDef)
{
	ASSERTV_RETZERO( pPlayer, "Expected unit" );
	ASSERTV_RETZERO( UnitIsPlayer( pPlayer ), "Expected player, got '%s'", UnitGetDevName( pPlayer ) );
	GAME *pGame = UnitGetGame( pPlayer );
	ASSERTV_RETZERO( pGame, "Expected game" );
	
	if (AppIsHellgate())
	{
	
		// must be available		
		if (LevelDefIsAvailableToUnit( pPlayer, nLevelDef ) != WRR_OK)
		{
			nLevelDef = LevelGetFirstPortalAndRecallLocation( pGame );
		}
		
		// super error checking
		if( LevelDefIsAvailableToUnit( pPlayer, nLevelDef ) != WRR_OK)
		{
			nLevelDef = LevelGetStartNewGameLevelDefinition();
		}

		ASSERTV( 
			LevelDefIsAvailableToUnit( pPlayer, nLevelDef ) == WRR_OK, 
			"Level def '%s' is not available to player '%s'", 
			nLevelDef,
			UnitGetDevName( pPlayer ));
		
	}
	
	return nLevelDef;
				
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
WARP_RESTRICTED_REASON LevelDefIsAvailableToUnit(
	UNIT *pUnit,
	int nLevelDef)
{
	ASSERTV_RETVAL( pUnit, WRR_UNKNOWN, "Expected unit" );
	WARP_RESTRICTED_REASON eRestrictedReason = WRR_OK;
	
	// the level act must be available to this player
	int nAct = LevelDefinitionGetAct( nLevelDef );
	eRestrictedReason = ActIsAvailableToUnit( pUnit, nAct );

	// the level def must be available in the game	
	if (eRestrictedReason == WRR_OK)
	{
	
		// the level def must be available in the game
		if (sLevelDefinitionIsAvailableToGame( nLevelDef ) == FALSE)
		{
			eRestrictedReason = WRR_LEVEL_NOT_AVAILABLE_TO_GAME;
		}
		
	}
	
	return eRestrictedReason;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
WARP_RESTRICTED_REASON LevelIsAvailableToUnit(
	UNIT *pUnit,
	LEVEL *pLevel)
{
	ASSERTV_RETVAL( pUnit, WRR_UNKNOWN, "Expected unit" );
	ASSERTV_RETVAL( pLevel, WRR_UNKNOWN, "Expected level" );
	int nLevelDef = LevelGetDefinitionIndex( pLevel );
	return LevelDefIsAvailableToUnit( pUnit, nLevelDef );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_LevelSendAllSubLevelStatus( 
	UNIT *pUnit)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	ASSERTV_RETURN( UnitIsPlayer( pUnit ), "Expected player, got '%s'", UnitGetDevName( pUnit ) );
	LEVEL *pLevel = UnitGetLevel( pUnit );
	ASSERTX_RETURN( pLevel, "Expected level" );
	
	// go through each sublevel
	int nNumSubLevels = LevelGetNumSubLevels( pLevel );
	for (int i = 0; i < nNumSubLevels; ++i)
	{
		SUBLEVEL *pSubLevel = LevelGetSubLevelByIndex( pLevel, i );
		ASSERTX_CONTINUE( pSubLevel, "Expected sublevel" );
		s_SubLevelSendStatus( pSubLevel, pUnit );
	}
		
}

//----------------------------------------------------------------------------
// this spawns something from a unit that died that the level requires
//----------------------------------------------------------------------------
static void sHandleSpawnTreasureOnDeathForLevel(
	GAME* pGame,
	UNIT* pUnit,
	UNIT* ,
	EVENT_DATA* pEventData,
	void*,
	DWORD dwId )
{

	ASSERT_RETURN( pEventData );
	int nTreasureClass = (int)pEventData->m_Data1;
	UNITID idOperator = (UNITID)UnitGetStat( pUnit, STATS_SPAWN_SOURCE );
	UNIT *pOperator = UnitGetById( pGame, idOperator );
#if !ISVERSION(CLIENT_ONLY_VERSION)
	s_TreasureSpawnInstanced( pUnit, pOperator, nTreasureClass );	
	UnitUnregisterEventHandler( pGame, pUnit, EVENT_UNITDIE_BEGIN, sHandleSpawnTreasureOnDeathForLevel );
#endif
}
//----------------------------------------------------------------------
//This is for levels that require a door to have a key that gets dropped by a random monster.
//----------------------------------------------------------------------
void s_LevelRegisterUnitToDropOnDie( GAME *pGame,
									 UNIT *pUnit, 
									 int nTreasureClass )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETURN( pGame );
	ASSERT_RETURN( pUnit );
	ASSERT_RETURN( nTreasureClass != INVALID_ID );
	UnitRegisterEventHandler( pGame, pUnit, EVENT_UNITDIE_BEGIN, sHandleSpawnTreasureOnDeathForLevel, &EVENT_DATA( nTreasureClass ));
#else
	REF( pGame );
	REF( pUnit );
	REF( nTreasureClass );
#endif
}

inline BOOL sRoomIsValidForTreasureSpawn( ROOM *pRoom,
										  ROOM *pBeforeRoom )
{
	return ( pRoom &&
			pRoom->nSpawnTreasureFromUnit.nTreasure == INVALID_ID &&
			( pBeforeRoom == NULL || pRoom->idRoom < pBeforeRoom->idRoom ) );
}

BOOL s_LevelRegisterTreasureDropRandom( GAME *pGame,
									    LEVEL *pLevel,
										int nTreasureClass,
										int nObjectClassFallbackSpawner,
										ROOM *pBeforeRoom ) //room going to spawn on treasure appears before this room
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETFALSE( pGame );	
	ASSERT_RETFALSE( pLevel );	
	//ASSERT_RETFALSE(LevelIsActive(pLevel) == TRUE);	
	ASSERT_RETFALSE( nTreasureClass != INVALID_ID );
	ASSERT_RETFALSE( nObjectClassFallbackSpawner != INVALID_ID );
	ROOM* pRoom = NULL;
	int count( 0 );
	CArrayList arrayValidRooms( AL_REFERENCE, sizeof( pRoom ) );
	CArrayList arrayValidRoomsWithMonsters( AL_REFERENCE, sizeof( pRoom ) );
	
	//ok so we couldn't find a room to spawn the treasure class. Lets just walk the rooms then....
	for ( pRoom = LevelGetFirstRoom( pLevel ); pRoom; pRoom = LevelGetNextRoom( pRoom ))
	{
		if( sRoomIsValidForTreasureSpawn( pRoom, pBeforeRoom ) )
		{
			if( s_RoomGetEstimatedMonsterCount( pRoom ) > 0 )
			{
				ArrayAddGetIndex( arrayValidRoomsWithMonsters, pRoom);				
			}
			ArrayAddGetIndex( arrayValidRooms, pRoom);
		}
	}
	if( arrayValidRoomsWithMonsters.Count() > 0 )
	{
		while( count < 100 )
		{
			int nIndex = RandGetNum( pGame, 0, arrayValidRoomsWithMonsters.Count() - 1 );
			pRoom = (ROOM *)arrayValidRoomsWithMonsters.GetPtr( nIndex );		
			//each room can only spawn 1 treasure for now - we need to do this check a second time.
			if( sRoomIsValidForTreasureSpawn( pRoom, pBeforeRoom ) )
			{
				arrayValidRooms.Destroy();
				arrayValidRoomsWithMonsters.Destroy();


				return s_RoomSetupTreasureSpawn( pRoom, nTreasureClass, TARGET_BAD, nObjectClassFallbackSpawner );

				
			}
			else
			{
				count++;
			}
		}
	}
	int nIndex = RandGetNum( pGame, 0, arrayValidRooms.Count() - 1 );
	pRoom = (ROOM *)arrayValidRooms.GetPtr( nIndex );		
	if( pRoom )
	{
		arrayValidRooms.Destroy();
		arrayValidRoomsWithMonsters.Destroy();
		if( RoomTestFlag( pRoom, ROOM_POPULATED_BIT ) == 0 )
		{
			s_RoomActivate( pRoom, NULL );
		}
		return s_RoomSetupTreasureSpawn( pRoom, nTreasureClass, TARGET_BAD, nObjectClassFallbackSpawner );
	}
	//ok so we couldn't find a room to spawn the treasure class. Lets just walk the rooms then....
	for ( pRoom = LevelGetFirstRoom( pLevel ); pRoom; pRoom = LevelGetNextRoom( pRoom ))
	{
		if( sRoomIsValidForTreasureSpawn( pRoom, pBeforeRoom ) )
		{
			arrayValidRooms.Destroy();

			return s_RoomSetupTreasureSpawn( pRoom, nTreasureClass, TARGET_BAD, nObjectClassFallbackSpawner ); 
		}
	}
	arrayValidRooms.Destroy();
	//we couldn't find a room. Bad news!
	ASSERT_RETFALSE( pRoom );
#else
	REF( pGame );
	REF( pLevel );
	REF( pBeforeRoom );
	REF( nTreasureClass );
	REF( nObjectClassFallbackSpawner );
#endif
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sLevelGetSingleEntranceLevel(
	int nLevelDef)
{
	ASSERTX_RETINVALID( nLevelDef != INVALID_LINK, "Expected level link" );
	const LEVEL_DEFINITION *pLevelDef = LevelDefinitionGet( nLevelDef );
	if (pLevelDef)
	{
	
		// easy case if this level is a level in a next/prev chain, the prev level is
		// always the only way into this level
		if (pLevelDef->nPrevLevel != INVALID_LINK)
		{
			return pLevelDef->nPrevLevel;
		}
		
		// Ok, this is a bit harder ... we take advantage of the fact that the map
		// is laid out with a main line path that leads into stations (levels with
		// many entrances and exists) so there is usually a level that has
		// it's "next" level as this level in question, in which case we will consider
		// that level as the "logical prev" level to this one
		int nNumLevels = ExcelGetNumRows( NULL, DATATABLE_LEVEL );
		int nLevelDefEntrance = INVALID_LINK;
		for (int nLevelDefOther = 0; nLevelDefOther < nNumLevels; ++nLevelDefOther)
		{
			const LEVEL_DEFINITION *pLevelDefOther = LevelDefinitionGet( nLevelDefOther );
			if (pLevelDefOther && pLevelDefOther->nNextLevel == nLevelDef)
			{
				// if we found one already, there are two ways into this level
				// so this level has no logical prev
				if (nLevelDefEntrance != INVALID_LINK)
				{
					return INVALID_LINK;
				}
				
				// record this level as the entrance, but keep searching for more
				nLevelDefEntrance = nLevelDefOther;
				
			}
			
		}

		// return entrnace if we found one and only one
		return nLevelDefEntrance;
		
	}
	
	return INVALID_LINK;
		
}

//----------------------------------------------------------------------------
enum LEVEL_MAP_OPERATION
{
	LMO_CLEAR_VISITED_STAT,
	LMO_SET_VISITED_STAT,
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sModifyPrevLevelVisitedStats(
	UNIT *pPlayer,
	int nLevelDef,
	LEVEL_MAP_OPERATION eOperation)
{

	// do the op
	int nDifficulty = UnitGetStat(pPlayer, STATS_DIFFICULTY_CURRENT);
	if (eOperation == LMO_CLEAR_VISITED_STAT)
	{
		const LEVEL_DEFINITION *pLevelDef = LevelDefinitionGet(nLevelDef);
		ASSERT_RETURN(pLevelDef);
		UnitSetStat( pPlayer, STATS_PLAYER_VISITED_LEVEL_BITFIELD, pLevelDef->nBitIndex, nDifficulty, 0);
	}
	else
	{
		LevelMarkVisited( pPlayer, nLevelDef, WORLDMAP_VISITED );
	}
	
	// do so for the previous level of this one too
	int nLevelDefPrev = sLevelGetSingleEntranceLevel( nLevelDef );
	if (nLevelDefPrev != INVALID_LINK)
	{
		sModifyPrevLevelVisitedStats( pPlayer, nLevelDefPrev, eOperation );
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL LevelCompressOrExpandMapStats(
	UNIT *pPlayer,
	LEVEL_MAP_EXPAND eExpand)
{
#if FALSE
	ASSERTX_RETTRUE( pPlayer, "Expected unit" );
	GAME *pGame = UnitGetGame( pPlayer );	
	static BOOL sbClientExpanding = FALSE;  // to solve UI repaint reentry issues
	
	// must be a hellate player
	if (UnitIsPlayer( pPlayer ) == FALSE || AppIsHellgate() == FALSE)
	{
		return TRUE;
	}

	if (IS_CLIENT( pGame ))
	{
		if (sbClientExpanding == TRUE && eExpand == LME_EXPAND)
		{
			return FALSE;  // in progress
		}
		sbClientExpanding = TRUE;
	}

	// go through all the levels
	int nNumLevels = ExcelGetNumRows( pGame, DATATABLE_LEVEL );
	for (int nLevelDef = 0; nLevelDef < nNumLevels; ++nLevelDef)
	{

		int nDifficulty = UnitGetStat(pPlayer, STATS_DIFFICULTY_CURRENT);			
		WORLD_MAP_STATE eMapState = (WORLD_MAP_STATE)UnitGetStat( pPlayer, STATS_PLAYER_VISITED_LEVEL_BITFIELD, nLevelDef, nDifficulty);
		if (eMapState != WORLDMAP_HIDDEN)
		{

			// once upon a time we were setting these stats for levels that didn't appear on
			// the world map at all, which is wasteful, so we're removing that mistake here
			if (LevelIsOnWorldMap( nLevelDef ) == FALSE)
			{
				UnitSetStat( pPlayer, STATS_PLAYER_VISITED_LEVEL_BITFIELD, nLevelDef, nDifficulty, 0 );
			}
			else if (eMapState == WORLDMAP_VISITED)
			{		
				// get the level tha was the bottleneck into this level (if any)
				int nLevelDefPrev = sLevelGetSingleEntranceLevel( nLevelDef );
				if (nLevelDefPrev != INVALID_LINK)
				{
					LEVEL_MAP_OPERATION eOperation = eExpand == LME_EXPAND ? LMO_SET_VISITED_STAT : LMO_CLEAR_VISITED_STAT;
					sModifyPrevLevelVisitedStats( pPlayer, nLevelDefPrev, eOperation );
				}
			}
			
		}

	}
	
	if (IS_CLIENT( pGame ) && eExpand == LME_EXPAND)
	{
		sbClientExpanding = FALSE;
	}
#endif
	return TRUE;
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL LevelIsOnWorldMap(
	int nLevelDef)
{
	ASSERTX_RETFALSE( nLevelDef != INVALID_LINK, "Expected level def link" );
	const LEVEL_DEFINITION *pLevelDef = LevelDefinitionGet( nLevelDef );
	if (pLevelDef)
	{
		if (pLevelDef->nMapRow != -1 && pLevelDef->nMapCol != -1)
		{
			return TRUE;
		}
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LevelMarkVisited(
	UNIT *pPlayer,
	int nLevelDef,
	WORLD_MAP_STATE eState,
	int nDifficultyOverride/*=-1*/)
{
	ASSERTX_RETURN( pPlayer, "Expected player" );
	ASSERTX_RETURN( nLevelDef != INVALID_LINK, "Expected level link" );
	int nDifficulty = UnitGetStat(pPlayer, STATS_DIFFICULTY_CURRENT);
	if(nDifficultyOverride != -1)
		nDifficulty = nDifficultyOverride;
	
	// make sure this level has a map representation at all


	const LEVEL_DEFINITION *pLevelDef = LevelDefinitionGet( nLevelDef );
	if (pLevelDef)
	{
		if (LevelIsOnWorldMap( nLevelDef ) == TRUE)
		{
			UnitSetStat( pPlayer, STATS_PLAYER_VISITED_LEVEL_BITFIELD, pLevelDef->nBitIndex, nDifficulty, eState);
		}
		if (pLevelDef && pLevelDef->pnAchievements)
		{
			for (int iAchievement = 0; iAchievement < pLevelDef->nNumAchievements; ++iAchievement)
			{
				s_AchievementsSendLevelEnter(pLevelDef->pnAchievements[iAchievement], pPlayer, nLevelDef);
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sPlayerQueueRoomUpdate(
	UNIT *pPlayer,
	void *pCallbackData)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTV_RETURN( UnitIsPlayer( pPlayer ), "Expected player, got '%s'", UnitGetDevName( pPlayer ));
	UNIT *pPlayerIgnore = (UNIT *)pCallbackData;
	if (pPlayer != pPlayerIgnore)
	{
		s_PlayerQueueRoomUpdate( pPlayer, 1 );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_LevelQueuePlayerRoomUpdate(
	LEVEL *pLevel,
	UNIT *pPlayerIgnore /*= NULL*/)
{
	ASSERTX_RETURN( pLevel, "Expected level" );
	LevelIteratePlayers( pLevel, sPlayerQueueRoomUpdate, pPlayerIgnore );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL LevelCanFormAutoParties(
	LEVEL *pLevel)
{
	ASSERTX_RETFALSE( pLevel, "Expected level" );
	const LEVEL_DEFINITION *pLevelDef = LevelDefinitionGet( pLevel );
	ASSERTX_RETFALSE( pLevelDef, "Expected level def" );
	return pLevelDef->bCanFormAutoParties;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL LevelIsOutdoors(
	LEVEL * pLevel)
{
	ASSERT_RETFALSE(pLevel);
	const DRLG_DEFINITION * pDRLGDef = LevelGetDRLGDefinition(pLevel);
	ASSERT_RETFALSE(pDRLGDef);
	return pDRLGDef->bIsOutdoors;
}


//---------------------------------------
//---------------------------------------
BOOL PlayerFixNontownWaypoints(
   UNIT* pPlayer )
{
	ASSERTX_RETFALSE( pPlayer, "Expected unit" );
	GAME *pGame = UnitGetGame( pPlayer );	

	// go through all the levels
	int nNumLevels = ExcelGetNumRows( pGame, DATATABLE_LEVEL );
	for(int nDifficulty = 0; nDifficulty <= UnitGetStat(pPlayer, STATS_DIFFICULTY_MAX); nDifficulty++)
	{
		for (int nLevelDef = 0; nLevelDef < nNumLevels; ++nLevelDef)
		{
			//int nDifficulty = UnitGetStat(pPlayer, STATS_DIFFICULTY_CURRENT);			
			WORLD_MAP_STATE eMapState = (WORLD_MAP_STATE)UnitGetStat( pPlayer, STATS_PLAYER_VISITED_LEVEL_BITFIELD, nLevelDef, nDifficulty);
			if (eMapState != WORLDMAP_HIDDEN)
			{

				// once upon a time we were setting these stats for levels that didn't appear on
				// the world map at all, which is wasteful, so we're removing that mistake here
				if (LevelIsOnWorldMap( nLevelDef ) == FALSE)
				{
					UnitSetStat( pPlayer, STATS_PLAYER_VISITED_LEVEL_BITFIELD, nLevelDef, nDifficulty, 0 );
				}
				else if (eMapState == WORLDMAP_VISITED)
				{		
					// get the level tha was the bottleneck into this level (if any)
					int nLevelDefPrev = sLevelGetSingleEntranceLevel( nLevelDef );
					if (nLevelDefPrev != INVALID_LINK)
					{
						LEVEL_MAP_OPERATION eOperation =  LMO_SET_VISITED_STAT ;
						sModifyPrevLevelVisitedStats( pPlayer, nLevelDefPrev, eOperation );
					}
				}

			}
		}
	}
	return TRUE;
}


//level of interest stuff
void s_LevelAddUnitOfInterest( LEVEL *pLevel, UNIT *pUnit )
{
	ASSERT_RETURN( pLevel );
	ASSERT_RETURN( pUnit );
	ASSERT_RETURN( IS_SERVER( LevelGetGame( pLevel ) ) );
	ArrayAddItem(pLevel->m_UnitsOfInterest, UnitGetId( pUnit ) );	
}

//level of interest stuff - returns the unit by Index
UNIT * s_LevelGetUnitOfInterestByIndex( LEVEL *pLevel, int nIndex )
{
	ASSERT_RETNULL( pLevel );
	ASSERT_RETNULL( IS_SERVER( LevelGetGame( pLevel ) ) );
	ASSERT_RETNULL(nIndex >= 0 && nIndex < (int)pLevel->m_UnitsOfInterest.Count());
	UNITID nUnitID = (UNITID)pLevel->m_UnitsOfInterest[ nIndex ];	
	return UnitGetById( LevelGetGame( pLevel ), nUnitID );
}

//level of interest stuff
int s_LevelGetUnitOfInterestCount( LEVEL *pLevel )
{
	ASSERT_RETZERO( pLevel );
	ASSERT_RETZERO( IS_SERVER( LevelGetGame( pLevel ) ) );
	return pLevel->m_UnitsOfInterest.Count();
}


void s_LevelSendPointsOfInterest( LEVEL *pLevel, UNIT *pPlayer )
{
	ASSERT_RETURN( pLevel );	
	ASSERT_RETURN( pPlayer );
	ASSERT_RETURN( UnitGetGenus( pPlayer ) == GENUS_PLAYER );
	if( pPlayer )
	{		
		MYTHOS_POINTSOFINTEREST::cPointsOfInterest *pPointsOfInterest = PlayerGetPointsOfInterest( pPlayer );
		ASSERT_RETURN( pPointsOfInterest );
		pPointsOfInterest->ClearAllPointsOfInterest();
		for( int t = 0; t < s_LevelGetUnitOfInterestCount( pLevel ); t++ )
		{
			UNIT *pUnit = s_LevelGetUnitOfInterestByIndex( pLevel, t );
			int nFlags = 0;
			if( UnitDataTestFlag( pUnit->pUnitData, UNIT_DATA_FLAG_IS_STATIC_POINT_OF_INTEREST ) )
			{
				nFlags |= MYTHOS_POINTSOFINTEREST::KPofI_Flag_Static;
				nFlags |= MYTHOS_POINTSOFINTEREST::KPofI_Flag_Display;
			}
			pPointsOfInterest->AddPointOfInterest( pUnit, INVALID_ID, nFlags );
		}
		pPointsOfInterest->ClientRefresh();
	}
}