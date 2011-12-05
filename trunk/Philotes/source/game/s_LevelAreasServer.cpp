#include "stdafx.h"
#include "s_LevelAreasServer.h"
#include "game.h"
#include "level.h"
#include "drlg.h"
#include "LevelAreas.h"
#include "GameMaps.h"
#include "picker.h"
#include "monsters.h"
#include "room_path.h"
#include "QuestObjectSpawn.h"
#include "picker.h"
#include "LevelAreaLinker.h"
#include "sublevel.h"

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int MYTHOS_LEVELAREAS::s_LevelGetStartNewGameLevelAreaDefinition( BOOL bIntroComplete )
{
	int nLevelAreaDef = INVALID_LINK;
	int nNumAreaDefs = ExcelGetCount(NULL, DATATABLE_LEVEL_AREAS);	


	// go through all level defs and find the first tutorial
	for (int i = 0 ; i < nNumAreaDefs; ++i)
	{
		const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION *pLevelAreaDef = LevelAreaDefinitionGet( i );
		if ( pLevelAreaDef->bStartArea == TRUE &&
			 bIntroComplete )
		{
			nLevelAreaDef = i;
			break;
		}			
		if ( pLevelAreaDef->bIntroArea == TRUE &&
			 !bIntroComplete )
		{
			nLevelAreaDef = i;
			break;
		}			
	}
	return nLevelAreaDef;
}

//this gets called via a callback when the monster is getting created.
static void sPendingMonsterSpawnedCallback(
	UNIT* pMonster,
	void* pCallbackData)
{
	ASSERTX_RETURN( pCallbackData, "Callback data was not level" );
	//set the boss Unit	
	((LEVEL *)pCallbackData)->m_LevelAreaOverrides.nBossId = UnitGetId( pMonster );
	ROOM *pRoom = UnitGetRoom( pMonster );
	ASSERT_RETURN( pRoom );
	((LEVEL *)pCallbackData)->m_LevelAreaOverrides.nRoomId = RoomGetId( pRoom );
}


//s_SpawnObjectInRoom
static BOOL s_SpawnObjectOnLevel( int nObjectID,								   
								  LEVEL *pLevel )
{
	ASSERTX_RETFALSE( pLevel, "Level to spawn monster in is NULL." );	
	if( nObjectID == INVALID_ID )
		return FALSE;
	s_SpawnObjectsUsingQuestPoints( nObjectID, pLevel );
	return TRUE;
}
//spawns a monster in a given room.
static BOOL s_SpawnMonstersInRoom( GAME *pGame,	
								   int nMonsterClassID,
								   int nQuality,
								   ROOM *pRoom,
								   LEVEL *pLevel )
{
	ASSERTX_RETFALSE( pRoom, "Room to spawn monster is NULL." );
	//monster quality	
	int nTries = 256;	//number of tries to spawn
	SPAWN_LOCATION tSpawnLocation;	//spawn location
	int nNumSpawned( 0 );
	ROOM* pOriginalRoom = pRoom;

	while (nNumSpawned == 0 && nTries-- > 0)
	{
		VECTOR Position;
		Position.fX = pRoom->pHull->aabb.center.fX + RandGetFloat( pGame, -pRoom->pHull->aabb.halfwidth.fX, pRoom->pHull->aabb.halfwidth.fX );
		Position.fY = pRoom->pHull->aabb.center.fY + RandGetFloat( pGame, -pRoom->pHull->aabb.halfwidth.fY, pRoom->pHull->aabb.halfwidth.fY );
		Position.fZ = .1f;

		ROOM_PATH_NODE * pPathNode = RoomGetNearestFreePathNode( pGame, RoomGetLevel(pRoom), Position, &pRoom, NULL );
		if( !pPathNode )
		{
			continue;
		}
		Position = RoomPathNodeGetExactWorldPosition(pGame, pRoom, pPathNode);

		pRoom = RoomGetFromPosition( pGame, pRoom, &Position );		
		if( !pRoom )
		{
			pRoom = pOriginalRoom;
		}
		tSpawnLocation.vSpawnPosition = Position;
		tSpawnLocation.vSpawnUp = VECTOR( 0, 0, 1 );
		tSpawnLocation.vSpawnDirection = VECTOR( 1, 0, 0 );
		tSpawnLocation.fRadius = 0.0f;

				
		// create a spawn entry just for this single target
		SPAWN_ENTRY tSpawn;		
		tSpawn.eEntryType = SPAWN_ENTRY_TYPE_MONSTER;	
		tSpawn.nIndex = nMonsterClassID;
		tSpawn.codeCount = (PCODE)CODE_SPAWN_ENTRY_SINGLE_COUNT;
					
		nNumSpawned = s_RoomSpawnMonsterByMonsterClass( 
			pGame, 
			&tSpawn, 
			nQuality, 
			pRoom, 
			&tSpawnLocation,
			sPendingMonsterSpawnedCallback, 
			pLevel );	
	}

	return (nNumSpawned != 0 );
}


void MYTHOS_LEVELAREAS::s_LevelAreaRoomHasBeenReactivated( ROOM *pRoom )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	/*
	ASSERT_RETURN( pRoom );
	LEVEL *pLevel = RoomGetLevel( pRoom );
	ASSERT_RETURN( pLevel );
	if( pLevel->m_bPVPWorld == FALSE )  //monsters only respawn in PVP worlds
	{
		return;
	}
	
	GAME *pGame = LevelGetGame( pLevel );
	ASSERT_RETURN( pGame );
	if( pLevel->m_LevelAreaOverrides.nBossId == INVALID_ID )
		return;	//no boss monster on this level
	int nRoomID = RoomGetId( pRoom );	

	if( pLevel->m_LevelAreaOverrides.nRoomId == nRoomID )
	{
		UNIT *pBossMonster = UnitGetById( pGame, pLevel->m_LevelAreaOverrides.nBossId );
		if( pBossMonster )
		{
			s_UnitRespawnFromCorpse( pBossMonster );
			return;
		}
		//so the monster's corpse has been destroyed so we need to recreate it.
		const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION *pLevelArea = MYTHOS_LEVELAREAS::LevelAreaDefinitionGet( LevelGetLevelAreaID( pLevel ) );
		ASSERT_RETURN( pLevelArea );
		int nQuality = MYTHOS_LEVELAREAS::LevelAreaGetBossQuality( pLevel, pLevelArea,  pLevel->m_LevelAreaOverrides.bIsElite );
		s_SpawnMonstersInRoom( pGame, 
							   pLevel->m_LevelAreaOverrides.nBossesToCreate,
							   nQuality,
							   pRoom,
							   pLevel );
	}
	*/
#endif
}

//this creates the bosses and treasure
void MYTHOS_LEVELAREAS::s_LevelAreaPopulateLevel( LEVEL *pLevel )
{
	ASSERTX( pLevel, "level is null." );
	GAME *pGame = LevelGetGame( pLevel );
	if( IS_CLIENT( pGame ) )  
		return;	//server only	
	const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION *pLevelArea = LevelGetLevelAreaOverrides( pLevel )->pLevelArea;	
	if( !pLevelArea ||
		!pLevel->m_LevelAreaOverrides.bOverridesEnabled )
		return; //no boss monsters or objects to spawn

	//spawn any objects we need..
	if( pLevel->m_LevelAreaOverrides.nObjectSpawn != INVALID_ID )
	{
		s_SpawnObjectOnLevel( pLevel->m_LevelAreaOverrides.nObjectSpawn, pLevel );
	}

	//Now lets do the ugly boss monster code...
	pLevel->m_LevelAreaOverrides.nBossesToCreate = MYTHOS_LEVELAREAS::LevelAreaGetEndBossClassID( pLevel, pLevel->m_LevelAreaOverrides.bIsElite );
	if( pLevel->m_LevelAreaOverrides.nBossesToCreate == INVALID_ID )
		return; //no boss monsters or objects to spawn
	int nQuality = MYTHOS_LEVELAREAS::LevelAreaGetBossQuality( pLevel, pLevelArea,  pLevel->m_LevelAreaOverrides.bIsElite );
	//lets first try and get a quest object spawn
	UNIT *pQuestObjectSpawnBOSS = s_GetQuestObjectInLevel( pLevel, TBQuestObjectCreate::KQUEST_OBJECT_BOSS );
	if( pQuestObjectSpawnBOSS )
	{
		MONSTER_SPEC tSpec;		
		tSpec.vPosition= pQuestObjectSpawnBOSS->vPosition;
		tSpec.vDirection = UnitGetFaceDirection( pQuestObjectSpawnBOSS, true );		
		// what monster quality to use for this spawn			
		tSpec.nClass = pLevel->m_LevelAreaOverrides.nBossesToCreate;
		tSpec.pRoom = UnitGetRoom( pQuestObjectSpawnBOSS );
		tSpec.nExperienceLevel = RoomGetMonsterExperienceLevel( tSpec.pRoom );
		tSpec.nMonsterQuality = nQuality;
		
		tSpec.nWeaponClass = INVALID_ID;				
	
		UNIT *pMonster = s_MonsterSpawn( pGame, tSpec );
		if( pMonster )
		{
			sPendingMonsterSpawnedCallback( pMonster, pLevel );
			return;
		}

		if( tSpec.pRoom &&
			s_SpawnMonstersInRoom( pGame, 
								   pLevel->m_LevelAreaOverrides.nBossesToCreate,
								   nQuality,
								   tSpec.pRoom,
								   pLevel ) )
		{
			return;				
		}		
	}
	//setup some flags
	DWORD dwFlags = 0;
	SETBIT( dwFlags, RRF_PLAYERLESS_BIT );

	// we will hold the rooms we can pick here
	ROOM *pRoomsAvailable[ MAX_RANDOM_ROOMS ];
	int nAvailableRoomCount = 0;
	BOOL bTooManyRooms = FALSE;
	
	// add rooms to the picker	
	PickerStart( pGame, picker );	
	ROOM *room = LevelGetFirstRoom( pLevel );
	room = LevelGetNextRoom( room );
	for ( room; room; room = LevelGetNextRoom( room ))
	{
		if (nAvailableRoomCount < MAX_RANDOM_ROOMS)
		{			
			// save available room			
			pRoomsAvailable[ nAvailableRoomCount ] = room;							
			// add to picker
			PickerAdd(MEMORY_FUNC_FILELINE(pGame, nAvailableRoomCount, 1));				
			nAvailableRoomCount++;
		}
		else
		{
			bTooManyRooms = TRUE;
		}
	}
	
	// warn of too many rooms
	WARNX( bTooManyRooms == FALSE, "Too many rooms in level to pick random room, increase MAX_RANDOM_ROOMS" );
	
	// if we had rooms that passed the pick filter, pick one
	if (nAvailableRoomCount <= 0 )
	{
		ASSERTX( !pLevel, "No rooms found that can be choosen randomly to spawn Monster." );
		//TODO: Need to force a re-role of the level here
		return;
	}
	//we will try 25 different rooms for now
	
	for( int t = 0; t < 25; t++ )
	{
		//Choose a random room
		int nPick = PickerChoose( pGame );
		ASSERTX( nPick >= 0 && nPick < nAvailableRoomCount, "Invalid room picked" );		
		ROOM *pRoom = pRoomsAvailable[ nPick ];
		ASSERTX( pRoom, "Invalid room pointer for picked room" );
		
		if( s_SpawnMonstersInRoom( pGame, 
								   pLevel->m_LevelAreaOverrides.nBossesToCreate,
								   nQuality,
								   pRoom,
								   pLevel ) )
		{
			break;		
			
		}
	}		


}


void MYTHOS_LEVELAREAS::s_LevelAreaRegisterItemCreationOnUnitDeath( GAME *pGame,
											 LEVEL *pLevelCreated,
											 UNIT *pPlayer )
{
	ASSERT_RETURN( pGame );
	ASSERT_RETURN( pLevelCreated );
	int nLevelAreaID = LevelGetLevelAreaID( pLevelCreated );
	const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION *pLevelArea = LevelGetLevelAreaOverrides( pLevelCreated )->pLevelArea;
	ASSERT_RETURN( pLevelArea );
	if( pLevelArea->bRandomMonsterDropsLinkToObjectCreate &&		//link to objects created in dungeon
		pLevelCreated->m_LevelAreaOverrides.bHasObjectCreatedSomeWhereInDungeon == FALSE )
	{
		return;	//we aint doin' nothing since we aren't creating objects
	}
	int nMaxDepth = pLevelCreated->m_LevelAreaOverrides.nDungeonDepth;
	int nDepth = pLevelCreated->m_LevelAreaOverrides.nCurrentDungeonDepth;
	int nObjectOverRide = GlobalIndexGet( GI_OBJECT_LEVELTREASURE_FALLBACK );//MAKE_SPECIES( GENUS_OBJECT, GlobalIndexGet( GI_OBJECT_LEVELTREASURE_FALLBACK ) );	
	RAND rand;
	MYTHOS_LEVELAREAS::LevelAreaInitRandByLevelArea( &rand, nLevelAreaID );
	for( int t = 0; t < LVL_AREA_GIVE_ITEM; t++ )
	{
		if( pLevelArea->nRandomMonsterDropsTreasureClass[t] != INVALID_ID )
		{
			int nRandomFloor = -1;
			int count( 100 );
			while(nRandomFloor == -1 && count > 0)
			{
				count--;
				nRandomFloor = RandGetNum( rand, 0, nMaxDepth - 1 );
				int nSectionID = MYTHOS_LEVELAREAS::LevelAreaGetSectionByDepth( nLevelAreaID, nRandomFloor );
				if( nSectionID != INVALID_ID )
				{
					if( pLevelArea->m_Sections[ nSectionID ].bAllowLevelAreaTreasureSpawn == FALSE )
					{
						nRandomFloor = -1;
					}
				}				
			}
			ASSERTX_CONTINUE( nRandomFloor >= 0, "Unable to find floor that allows level area to spawn a treasure from a monster." );
			if( nRandomFloor == nDepth )
			{
				s_LevelRegisterTreasureDropRandom( pGame, pLevelCreated, pLevelArea->nRandomMonsterDropsTreasureClass[t], nObjectOverRide, NULL );
			}
		}
	}
	
}
