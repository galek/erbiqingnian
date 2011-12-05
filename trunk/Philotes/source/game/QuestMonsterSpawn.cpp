#include "stdafx.h"
#include "QuestMonsterSpawn.h"
#include "Quest.h"	//Quest system
#include "units.h"
#include "picker.h"
#include "monsters.h"
#include "room_path.h"
#include "sublevel.h"

//this gets called via a callback when the monster is getting created.
static void sPendingMonsterSpawnedCallback(
	UNIT* pMonster,
	void* pCallbackData)
{
	MONSTER_QUEST_SPAWN_PARAMS *params = (MONSTER_QUEST_SPAWN_PARAMS *)pCallbackData;
	//lets set the monsters unique name
	if( params->m_MonsterUniqueName != INVALID_ID )
	{
		UnitSetNameOverride( pMonster, params->m_MonsterUniqueName );
		//MonsterProperNameSet( pMonster, params->m_MonsterUniqueName, TRUE );
	}
	
}


//spawns a monster in a given room.
bool s_SpawnMonstersInRoom( GAME *pGame,
							QUEST_TASK_DEFINITION_TUGBOAT* pQuestTask,
							int qIndex,
							ROOM *pRoom )
{
	ASSERTX_RETFALSE( pRoom, "Quest trying to add monster to NULL room." );
	//monster quality
	int nMonsterQuality = ( pQuestTask->nQuestMonstersToSpawnQuality[qIndex] >= 0 )?pQuestTask->nQuestMonstersToSpawnQuality[qIndex]:-1;
	int nTries = 256;	//number of tries to spawn
	SPAWN_LOCATION tSpawnLocation;	//spawn location
	int nNumSpawned( 0 );
	MONSTER_QUEST_SPAWN_PARAMS paramsToPassCreatedMonster;
	VECTOR Position;
	Position.fX = pRoom->pHull->aabb.center.fX + RandGetFloat( pGame, -pRoom->pHull->aabb.halfwidth.fX, pRoom->pHull->aabb.halfwidth.fX );
	Position.fY = pRoom->pHull->aabb.center.fY + RandGetFloat( pGame, -pRoom->pHull->aabb.halfwidth.fY, pRoom->pHull->aabb.halfwidth.fY );
	Position.fZ = .1f;
	ROOM* pSavedRoom = pRoom;
	pRoom = RoomGetFromPosition( pGame, pRoom, &Position );
	if( !pRoom )
		pRoom = pSavedRoom;

	while (nNumSpawned == 0 && nTries-- > 0)
	{
		if( !pRoom )
			pRoom = pSavedRoom;
		ROOM_PATH_NODE * pPathNode = RoomGetNearestFreePathNode( pGame, RoomGetLevel(pRoom), Position, &pRoom, NULL );
		if( !pPathNode )
		{
			continue;
		}
		Position = RoomPathNodeGetExactWorldPosition(pGame, pRoom, pPathNode);

		pRoom = RoomGetFromPosition( pGame, pRoom, &Position );					
		tSpawnLocation.vSpawnPosition = Position;
		tSpawnLocation.vSpawnUp = VECTOR( 0, 0, 1 );
		tSpawnLocation.vSpawnDirection = VECTOR( 1, 0, 0 );
		tSpawnLocation.fRadius = 0.0f;
		VectorDirectionFromAngleRadians( tSpawnLocation.vSpawnDirection, TWOxPI * RandGetFloat(pGame) );
				
		// create a spawn entry just for this single target
		SPAWN_ENTRY tSpawn;		
		tSpawn.eEntryType = SPAWN_ENTRY_TYPE_MONSTER;
		tSpawn.nIndex = pQuestTask->nQuestMonstersIDsToSpawn[qIndex];
		tSpawn.codeCount = (PCODE)CODE_SPAWN_ENTRY_SINGLE_COUNT;
			
		paramsToPassCreatedMonster.m_MonsterUniqueName = pQuestTask->nQuestMonstersUniqueNames[ qIndex ];
		if( tSpawn.nIndex  >= 0 )
		{
			nNumSpawned = s_RoomSpawnMonsterByMonsterClass( 
				pGame, 
				&tSpawn, 
				nMonsterQuality, 
				pRoom, 
				&tSpawnLocation,
				sPendingMonsterSpawnedCallback, 
				(void*)&paramsToPassCreatedMonster);	
		}
		else
		{
			nNumSpawned++;
		}
	}
	if( nNumSpawned == 0 )
    {
		ASSERT_MSG( "Spawning monster for quest failed. (s_SpawnMonstersInRoom )" );
    }

	return (nNumSpawned != 0 );
}

//Spawns the quest items in this level( dungeon )
void s_SpawnMonstersForQuestOnLevelAdd( GAME *pGame,
									    LEVEL *pLevelCreated )
{
	if( IS_CLIENT( pGame ) )  
		return;	//server only
	ASSERTX( pLevelCreated, "Quest trying to add monster to NULL level." );
	//this has got to be one of the slowest ways of doing this.
	int QuestCount = ExcelGetCount(EXCEL_CONTEXT(pGame), DATATABLE_QUESTS_TASKS_FOR_TUGBOAT );
	//setup some flags
	DWORD dwFlags = 0;
	SETBIT( dwFlags, RRF_PLAYERLESS_BIT );

	// we will hold the rooms we can pick here
	#define MAX_FAILS 25
	ROOM *pRoomsAvailable[ MAX_RANDOM_ROOMS ];
	int nAvailableRoomCount = 0;
	BOOL bTooManyRooms = FALSE;
	
	// add rooms to the picker	
	PickerStart( pGame, picker );	
	ROOM *room = LevelGetFirstRoom( pLevelCreated );
	room = LevelGetNextRoom( room );
	for ( room; room; room = LevelGetNextRoom( room ))
	{
		if (nAvailableRoomCount < MAX_RANDOM_ROOMS )
		{			
			if( RoomAllowsMonsterSpawn( room ) )
			{
				// save available room			
				pRoomsAvailable[ nAvailableRoomCount ] = room;							
				// add to picker
				PickerAdd(MEMORY_FUNC_FILELINE(pGame, nAvailableRoomCount, 1));				
				nAvailableRoomCount++;
			}
		}
		else
		{
			bTooManyRooms = TRUE;
		}
	}
	if( room &&
		nAvailableRoomCount == 0 )
	{
		// save available room			
		pRoomsAvailable[ nAvailableRoomCount ] = room;							
		// add to picker
		PickerAdd(MEMORY_FUNC_FILELINE(pGame, nAvailableRoomCount, 1));				
		nAvailableRoomCount++;
	}
	// warn of too many rooms
	WARNX( bTooManyRooms == FALSE, "Too many rooms in level to pick random room, increase MAX_RANDOM_ROOMS" );
	
	// if we had rooms that passed the pick filter, pick one
	if (nAvailableRoomCount <= 0 )
	{
		ASSERTX( pLevelCreated, "No rooms found that can be choosen randomly in Quest spawn Monster." );
		return;
	}
	
	for( int i = 0; i < QuestCount; i++ )
	{
		QUEST_TASK_DEFINITION_TUGBOAT* pQuestTaskDefinition = (QUEST_TASK_DEFINITION_TUGBOAT*)ExcelGetData( pGame, DATATABLE_QUESTS_TASKS_FOR_TUGBOAT, i );
		ASSERT_CONTINUE(pQuestTaskDefinition);
		int count( 0 );
		while( count < MAX_MONSTERS_TO_SPAWN &&
			   pQuestTaskDefinition->nQuestDungeonsToSpawnMonsters[ count ] != INVALID_ID )
		{
			//check to see if we are in the right dungeon
			if( pQuestTaskDefinition->nQuestDungeonsToSpawnMonsters[ count ] == LevelGetLevelAreaID( pLevelCreated ) )
			{
				//check to seee if we are on the correct floor
				//-1 only on last level		
				if( ( pQuestTaskDefinition->nQuestLevelsToSpawnMonstersIn[ count ] == -1 &&
					  pLevelCreated->m_LevelAreaOverrides.bIsLastLevel ) ||	//last level only
					pQuestTaskDefinition->nQuestLevelsToSpawnMonstersIn[ count ] == pLevelCreated->m_nDepth )
				{	
					int nFailCount( 0 );
					//find out how many packs to create per level
					for( int nPacksCreate = 0; nPacksCreate < pQuestTaskDefinition->nQuestPacksCountToSpawn[ count ]; nPacksCreate++ )
					{
						//Choose a random room
						int nPick = PickerChoose( pGame );
						ASSERTX_CONTINUE( nPick >= 0 && nPick < nAvailableRoomCount, "Invalid room picked" );		
						ROOM *pRoom = pRoomsAvailable[ nPick ];
						ASSERTX_CONTINUE( pRoom, "Invalid room pointer for picked room" );
						SUBLEVEL *pSubLevel = RoomGetSubLevel( pRoom );
						//spawn monsters per pack
						for( int nMonsterCreate = 0; nMonsterCreate < pQuestTaskDefinition->nQuestNumOfMonstersPerPack[ count ]; nMonsterCreate++ )
						{
							if( !s_SpawnMonstersInRoom( pGame, 
													   pQuestTaskDefinition,
													   count,
													   pRoom ) )
							{
								nFailCount++;
								ASSERTX( nFailCount < MAX_FAILS, "failed to spawn a monster in SpawnMonstersForQuest more then MAX_FAILS allowed." );		
								nPacksCreate--;	//we failed. So lets try another room
								break;	//we break out and try again with a different room
							}
							else
							{
								pSubLevel->nMonsterCount++;
							}
						}
					}

				}
			}
			count++;
		}
	}
}

