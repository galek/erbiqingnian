#include "StdAfx.h"
#include "QuestObjectSpawn.h"
#include "target.h"
#include "excel.h"
#include "objects.h"
#include "monsters.h"
#include "states.h"
#include "s_QuestServer.h"
#include "room_path.h"
#include "QuestProperties.h"
#include "items.h"
#include "globalindex.h"
#include "gameglobals.h"
using namespace TBQuestObjectCreate;

/*
The most confusing thing about Object Create is that objects must be placed in 
each room. Each object can be a boss or an interactable object.  We have to save the 
states for these objects so we know if they have
been interacted with or not. This is done by the stats:

STATS_QUEST_TASK_OBJECTS_TUGBOAT
STATS_QUEST_TASK_MONSTERS_TUGBOAT
STATS_QUEST_TASK_MONSTERXP_TUGBOAT

*/

//Checks to see if the monster has already given the player XP
BOOL s_QuestMonsterXPAwardedAlready( UNIT *pMonster,
									 UNIT *pPlayer )
{
	ASSERTX_RETTRUE( pMonster, "Monster must be passed in for XP check for quest" );
	ASSERTX_RETTRUE( pPlayer, "Player must be passed in for XP check for quest" );
	if( UnitGetGenus( pMonster ) != GENUS_MONSTER )
		return FALSE; 	
	if( QuestUnitIsQuestUnit( pMonster ) == FALSE )
		return FALSE; //we can only save info on quest units
	//int taskIndex = QuestUnitGetTaskIndex( pMonster );
	int arrayIndex = QuestUnitGetIndex( pMonster );
	const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask = QuestUnitGetTask( pMonster );
	ASSERT_RETFALSE( pQuestTask );
	if( QuestIsQuestTaskComplete( pPlayer, pQuestTask ) )
		return TRUE;
	int nQuestQueueIndex = MYTHOS_QUESTS::QuestGetQuestQueueIndex( pPlayer, pQuestTask );
	if( nQuestQueueIndex == INVALID_ID )
	{
		return TRUE; //you can get to this state if you abandon the quest
	}
	ASSERT_RETFALSE( nQuestQueueIndex != INVALID_ID );
	int TaskMask = UnitGetStat( pPlayer, STATS_QUEST_TASK_MONSTERXP_TUGBOAT, nQuestQueueIndex );
	return ((TaskMask & ( 1 <<  arrayIndex )  ) != 0 );

}
//sets that the monster has given XP to the player
void s_QuestMonsterSetXPAwarded( UNIT *pMonster,
								 UNIT *pPlayer )
{
	ASSERTX_RETURN( pMonster, "Monster must be passed in for XP check for quest" );
	ASSERTX_RETURN( pPlayer, "Player must be passed in for XP check for quest" );
	if( UnitGetGenus( pMonster ) != GENUS_MONSTER )
		return; 	
	if( QuestUnitIsQuestUnit( pMonster ) == FALSE )
		return; //we can only save info on quest units
	//int taskIndex = QuestUnitGetTaskIndex( pMonster );
	int arrayIndex = QuestUnitGetIndex( pMonster );
	const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask = QuestUnitGetTask( pMonster );
	ASSERT_RETURN( pQuestTask );
	int nQuestQueueIndex = MYTHOS_QUESTS::QuestGetQuestQueueIndex( pPlayer, pQuestTask );
	if( nQuestQueueIndex == INVALID_ID )
	{
		return; //you can get this if you abandon a quest in the middile of a dungeon
	}
	ASSERT_RETURN( nQuestQueueIndex != INVALID_ID );
	int TaskMask = UnitGetStat( pPlayer, STATS_QUEST_TASK_MONSTERXP_TUGBOAT, nQuestQueueIndex );
	TaskMask = TaskMask | ( 1 << arrayIndex );
	//set the mask complete
	UnitSetStat( pPlayer, STATS_QUEST_TASK_MONSTERXP_TUGBOAT, nQuestQueueIndex, TaskMask );
}


//All tasks in quest are in an array. I'm using the array index as the mask.
//this works great as long as we the array isn't bigger then the stat( 16 bits )
//Monsters are the second half of the stat.
BOOL s_IsTaskObjectCompleted( GAME *pGame,
							 UNIT *playerUnit,
							 const QUEST_TASK_DEFINITION_TUGBOAT* pQuestTask,
							 int ObjectIndex,
							 BOOL Monster,
							 BOOL bIgnoreBossNotNeededToCompleteQuest )
{
	ASSERT_RETFALSE(pQuestTask);
	if( !bIgnoreBossNotNeededToCompleteQuest &&
		Monster &&
		MYTHOS_QUESTS::QuestGetBossNotNeededToCompleteQuest( playerUnit, pQuestTask, ObjectIndex ) )
	{
		return TRUE;
	}
	if( QuestIsQuestTaskComplete( playerUnit, pQuestTask ) )
		return TRUE;
	STATS_TYPE useStat = ((Monster)?STATS_QUEST_TASK_MONSTERS_TUGBOAT:STATS_QUEST_TASK_OBJECTS_TUGBOAT);
	int nQuestQueueIndex = MYTHOS_QUESTS::QuestGetQuestQueueIndex( playerUnit, pQuestTask );
	ASSERT_RETFALSE( nQuestQueueIndex != INVALID_ID );
	int	TaskMask = UnitGetStat( playerUnit, useStat, nQuestQueueIndex );
	return ((TaskMask & ( 1 << ObjectIndex ) ) != 0 );

}



//this checks to see if all the objects in a task have been completed or not
BOOL QuestObjectSpawnIsComplete( UNIT *pPlayer,
								 const QUEST_TASK_DEFINITION_TUGBOAT* pQuestTask )
{
	if( pPlayer == NULL )
		return TRUE;
	GAME *pGame = UnitGetGame( pPlayer );
	for( int i = 0; i < MAX_OBJECTS_TO_SPAWN; i++ )
	{
		int objectSpawn = MYTHOS_QUESTS::QuestGetObjectIDToSpawnByIndex( pQuestTask, i );
		int monsterSpawn = MYTHOS_QUESTS::QuestGetBossIDToSpawnByIndex( pPlayer, pQuestTask, i );
		if( objectSpawn != INVALID_LINK )
		{
			if( s_IsTaskObjectCompleted( pGame, pPlayer, pQuestTask, i, FALSE, FALSE ) == FALSE )
			{
				return FALSE;
			}
		}
		if( monsterSpawn != INVALID_LINK )
		{
			const UNIT_DATA *monster = UnitGetData( UnitGetGame( pPlayer ), GENUS_MONSTER, monsterSpawn );
			if( monster != NULL &&
				UnitDataTestFlag(monster, UNIT_DATA_FLAG_IS_GOOD))
			{
				//actually if it's a good monster(NPC) we don't care.
			}
			else if( s_IsTaskObjectCompleted( pGame, pPlayer, pQuestTask, i, TRUE, FALSE ) == FALSE )
			{
				return FALSE;
			}			
		}
	}
	return TRUE;
}


//Get random unit
inline UNIT * s_GetUnit( GAME *pGame,
						 EQUEST_OBJECTS_CREATABLES eObjectType,
						 CQuestObjectParams *params )
{
	ASSERTX_RETNULL( params->m_ObjectsCount[ eObjectType ] > 0, "Trying to spawn a quest object but there are either no or not enough quest_interact objects in rooms" );
	int index = RandGetNum( pGame, 0, params->m_ObjectsCount[ eObjectType ] - 1 );	
	UNIT *unit = params->m_ObjectsFoundArray[ eObjectType ][ index ];
	ASSERTX_RETNULL( unit, "Quest object trying to spawn from a NULL unit" );
	//we are removing the unit
	params->m_ObjectsCount[ eObjectType ]--;
	//now lets just go head and remove the unit so we don't use it again.
	params->m_ObjectsFoundArray[ eObjectType ][ index ] = params->m_ObjectsFoundArray[ eObjectType ][ params->m_ObjectsCount[ eObjectType ] ];
	return unit;
}

inline ROOM * GetRandomRoom( LEVEL *pLevel )
{
	GAME *pGame = LevelGetGame( pLevel );
	ASSERT_RETVAL( pGame, NULL );
	ROOM* pRoom = NULL;
	s_LevelGetRandomSpawnRooms(pGame, pLevel, &pRoom, 1);
	return pRoom;
}

//Spawns Units
inline UNIT * s_SpawnObject( LEVEL *pLevel,
							 UNIT *unitToSpawnFrom,
						     int unitIDToSpawn,
						     CQuestObjectParams *params )
{
	UNIT *returnUnit( NULL );
	//if( !unitToSpawnFrom  )
	//	return returnUnit;
	if( unitIDToSpawn == INVALID_ID )
		return returnUnit;
	
	GAME *pGame = LevelGetGame( pLevel );
	ASSERT_RETVAL( pGame, NULL );

	//Why spawn it from the room? It has some nice things attached to it's
	//creation. such as spawning a monster.
	SPAWN_LOCATION location;
	ROOM *pRoom( NULL );
	if( unitToSpawnFrom )
	{
		location.vSpawnDirection = UnitGetFaceDirection( unitToSpawnFrom, true );
		location.vSpawnPosition = UnitGetPosition( unitToSpawnFrom );
		location.vSpawnUp = UnitGetUpDirection( unitToSpawnFrom );
		pRoom = UnitGetRoom( unitToSpawnFrom );
	}
	else
	{
		
		VECTOR position;				
		ROOM *roomReturned( NULL );		
		ROOM_PATH_NODE *pathNode( NULL );
		for( int nRoomTries = 0; nRoomTries < 100; nRoomTries++ )
		{
			pRoom = GetRandomRoom( pLevel );
			ASSERT_CONTINUE( pRoom );
			pathNode = RoomGetNearestFreePathNode( pGame, pLevel, position, &roomReturned, NULL, 0, 0, pRoom );		
			if( pathNode != NULL )
				break;			
		}
		ASSERT_RETVAL( pathNode, NULL );
		if( pathNode )
		{
			location.vSpawnDirection = VECTOR( 1, 0, 0 );
			location.vSpawnUp = VECTOR( 0, 0, 1 );
			location.vSpawnPosition = RoomPathNodeGetWorldPosition( pGame, pRoom, pathNode );			
		}
		
	}

	const UNIT_DATA *unitData = ObjectGetData( pGame, unitIDToSpawn );
	if( unitData)
	{
		returnUnit = s_RoomSpawnObject( pGame,
									   pRoom,
									   &location,
									   unitData->wCode );		
	}	
	return returnUnit;
}

//Spawns Units
inline UNIT * s_SpawnMonsterObject( UNIT *pPlayer,
								    LEVEL *pLevel,
								    UNIT *unitToSpawnFrom,
									int monsterIDToSpawn,
									const QUEST_TASK_DEFINITION_TUGBOAT* pQuestTask,
									int index )
{
	UNIT *returnUnit( NULL );
	if( monsterIDToSpawn == INVALID_ID )
		return returnUnit;

	VECTOR position;
	ROOM *pRoom = NULL;	
	MONSTER_SPEC tSpec;
	GAME *pGame = LevelGetGame( pLevel );
	ASSERT_RETNULL( pGame );
	if( unitToSpawnFrom )
	{
		pRoom = UnitGetRoom( unitToSpawnFrom );
		tSpec.vPosition= unitToSpawnFrom->vPosition;
		tSpec.vDirection = UnitGetFaceDirection( unitToSpawnFrom, true );		
	}
	else
	{
		
		
		ROOM *roomReturned( NULL );
		ROOM_PATH_NODE *pathNode( NULL );
		for( int nRoomTries = 0; nRoomTries < 100; nRoomTries++ )
		{
			pRoom = GetRandomRoom( pLevel );
			ASSERT_CONTINUE( pRoom );
			pathNode = RoomGetNearestFreePathNode( pGame, pLevel, position, &roomReturned, NULL, 0, 0, pRoom );		
			if( pathNode != NULL )
				break;			
		}
		ASSERT_RETVAL( pathNode, NULL );
		pRoom = roomReturned;
		tSpec.vPosition= RoomPathNodeGetWorldPosition( pGame, pRoom, pathNode );
		tSpec.vDirection = VECTOR( 1, 0, 0 );			
	}
	
	// what monster quality to use for this spawn
	int nMonsterQualityForSpawn = MYTHOS_QUESTS::QuestGetBossQualityIDToSpawnByIndex( pPlayer, pQuestTask, index );
	
	tSpec.nClass = monsterIDToSpawn;
	tSpec.nExperienceLevel = RoomGetMonsterExperienceLevel( pRoom );
	tSpec.nMonsterQuality = nMonsterQualityForSpawn;
	tSpec.pRoom = pRoom;
	tSpec.nWeaponClass = INVALID_ID;
				
	
	returnUnit = s_MonsterSpawn( pGame, tSpec );
	// We're going to make all quest mobs respawn now. Why not?
	UnitSetFlag( returnUnit, UNITFLAG_SHOULD_RESPAWN );
	UnitSetFlag( returnUnit, UNITFLAG_RESPAWN_EXACT );
	if( returnUnit )
	{
		s_SetUnitAsQuestUnit( returnUnit, NULL, pQuestTask, index ); 
		s_RoomSpawnMinions( UnitGetGame( returnUnit ), returnUnit, UnitGetRoom( returnUnit ), &tSpec.vPosition );
	}
	//lets set the unique name
	if( isQuestRandom( pQuestTask ) )
	{
		UnitSetStat( returnUnit, STATS_MONSTER_UNIQUE_NAME, MYTHOS_QUESTS::QuestGetBossPrefixNameByIndex( pPlayer, pQuestTask, 0 )  );
	}
	else if( returnUnit && pQuestTask )
	{
		int nUniqueName = MYTHOS_QUESTS::QuestGetBossUniqueNameByIndex( pPlayer, pQuestTask, index );
		if( nUniqueName != INVALID_ID )
		{
			UnitSetNameOverride( returnUnit, nUniqueName );
			UnitSetStat( returnUnit, STATS_MONSTER_UNIQUE_NAME, MONSTER_UNIQUE_NAME_INVALID );
		}
		
	}

	return returnUnit;
}


//this is the call back for iterating the units in a room.
static ROOM_ITERATE_UNITS sUnitFoundInLevel( UNIT *pUnit,
											 void *pCallbackData)
{
	CQuestObjectParams *params = ( CQuestObjectParams*)pCallbackData;
	//Add up the unit types found in the level
	if( UnitIsA( pUnit, UNITTYPE_QUEST_OBJECT ) )
	{
		for( int t = 0; t < KQUEST_OBJECT_COUNT; t++ )
		{
			if( UnitIsA( pUnit, KQUEST_OBJECT_UNITTYPES_ARRAY[ t ] ) &&
				!LevelPositionIsInSpawnFreeZone( UnitGetLevel( pUnit ),
												 &UnitGetPosition( pUnit ) ) )
			{
				params->m_ObjectsFoundArray[ t ][ params->m_ObjectsCount[ t ] ] = pUnit;
				params->m_ObjectsCount[ t ]++;
				if( params->m_ObjectsCount[ t ] >= MAX_OBJECTS_PER_ARRAY )
				{
					ASSERT_MSG( "Error Occured in Spawn Objects ( tugboat ). To many spawn points found" );
					return RIU_STOP;	//to many spawn points found
				}
			}
		}
	}
	return RIU_CONTINUE;
}

UNIT * s_GetQuestObjectInLevel( LEVEL *pLevel, TBQuestObjectCreate::EQUEST_OBJECTS_CREATABLES nType )
{
	ASSERTX_RETNULL( pLevel, "No level passed in" );
	//first thing is we need to find all the different quest object spawns
	CQuestObjectParams params;	
	TARGET_TYPE eTargetTypes[] = { TARGET_OBJECT, TARGET_INVALID };
	LevelIterateUnits( pLevel, eTargetTypes, sUnitFoundInLevel, (void *)&params );
	UNIT *unitSpawningFrom = s_GetUnit( LevelGetGame( pLevel ), nType, &params );
	ASSERTX_RETNULL( unitSpawningFrom, "Unit not found." );
	return unitSpawningFrom;
}

void s_SpawnObjectsUsingQuestPoints( int nObjectID,									 
									 LEVEL *pLevel )
{

	ASSERTX_RETURN( pLevel, "No level passed in" );
	if( nObjectID == INVALID_ID )
	{
		return;
	}
	//first thing is we need to find all the different quest object spawns
	CQuestObjectParams params;	
	TARGET_TYPE eTargetTypes[] = { TARGET_OBJECT, TARGET_INVALID };
	LevelIterateUnits( pLevel, eTargetTypes, sUnitFoundInLevel, (void *)&params );
	UNIT *unitSpawningFrom = s_GetUnit( LevelGetGame( pLevel ), KQUEST_OBJECT_INTERACT, &params );
	//ASSERTX_RETURN( unitSpawningFrom, "Could not find any objects to spawn object from" );	
	UNIT *unit = s_SpawnObject( pLevel, unitSpawningFrom, nObjectID, &params );
	UnitSetFlag( unit, UNITFLAG_SHOULD_RESPAWN );
	UnitSetFlag( unit, UNITFLAG_RESPAWN_EXACT );
	ASSERTX_RETURN( unit, "Unit not created." );
	
}
//this turned ugly real quick
UNIT * sCreateUnitAtObjectInLevel( LEVEL *pLevel, 
								   UNIT **pPlayerArray,
								   int nSpecificMonsterIndex, 
								   const QUEST_TASK_DEFINITION_TUGBOAT **pQuestTaskArray, 
								   int nQuestTaskArraySize, 
								   BOOL bCreateObjects, 
								   BOOL bCreateMonsters,
								   BOOL bRegisterUnits,
								   const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTaskToForce )
{
	if( nQuestTaskArraySize <= 0 )
		return NULL;	
	GAME *pGame = LevelGetGame( pLevel );
	//first thing is we need to find all the different quest object spawns
	CQuestObjectParams params;	
	TARGET_TYPE eTargetTypes[] = { TARGET_OBJECT, TARGET_INVALID };
	LevelIterateUnits( pLevel, eTargetTypes, sUnitFoundInLevel, (void *)&params );
	int nStartLoop = 0;
	int nEndLoop = MAX_OBJECTS_TO_SPAWN;
	UNIT *unit( NULL );

	if( nSpecificMonsterIndex >= 0 )
	{
		nStartLoop = nSpecificMonsterIndex;
		nEndLoop = nSpecificMonsterIndex + 1; //will make unique monster or object
	}
	//Now lets go through all the quest that use this level and populate the level
	for( int i = 0; i < nQuestTaskArraySize; i++ )
	{
		UNIT *pPlayer( NULL );
		if( pPlayerArray )
			pPlayer = pPlayerArray[ i ];
		ASSERT_CONTINUE( pPlayer );
		const QUEST_TASK_DEFINITION_TUGBOAT * pQuestTask = pQuestTaskArray[i];
		BOOL bForceDungeon( pQuestTaskToForce != NULL && pQuestTaskToForce == pQuestTask );
		//number of objects that can spawn per task
		for( int t = nStartLoop; t < nEndLoop; t++ ) 
		{
			//Unit ID's to create
			int objectIDToSpawn = MYTHOS_QUESTS::QuestGetObjectIDToSpawnByIndex( pQuestTask, t ); 
			BOOL bValidFloor = MYTHOS_QUESTS::QuestGetObjectSpawnDungeonFloorByIndex( pQuestTask, t ) == pLevel->m_nDepth;
			bValidFloor = bValidFloor || ( MYTHOS_QUESTS::QuestGetObjectSpawnDungeonFloorByIndex( pQuestTask, t ) == -1 && pLevel->m_LevelAreaOverrides.bIsLastLevel );
			if( objectIDToSpawn != INVALID_ID &&				
				( !bValidFloor ||
				  (pPlayer && MYTHOS_QUESTS::QuestGetObjectSpawnDungeonByIndex( pPlayer, pQuestTask, t ) != LevelGetLevelAreaID( pLevel ) ) ) &&
				!bForceDungeon )
			{
				objectIDToSpawn = INVALID_ID;				
			}
			if( bCreateObjects == FALSE )
				objectIDToSpawn = NULL;

			int bossIDToSpawn = MYTHOS_QUESTS::QuestGetBossIDToSpawnByIndex( pPlayer, pQuestTask, t );
			bValidFloor = MYTHOS_QUESTS::QuestGetBossSpawnDungeonFloorByIndex( pQuestTask, t ) == pLevel->m_nDepth;
			bValidFloor = bValidFloor || ( MYTHOS_QUESTS::QuestGetBossSpawnDungeonFloorByIndex( pQuestTask, t ) == -1 && pLevel->m_LevelAreaOverrides.bIsLastLevel );
			if( !bForceDungeon &&
				bossIDToSpawn != INVALID_ID &&
				( !bValidFloor ||
				  ( pPlayer && MYTHOS_QUESTS::QuestGetBossSpawnDungeonByIndex( pPlayer, pQuestTask, t ) != LevelGetLevelAreaID( pLevel ) ) ) )
			{
				bossIDToSpawn = INVALID_ID;
			}
			if( bCreateMonsters == FALSE )
				bossIDToSpawn = NULL;
			//Make sure we have a valid ID or one of them
			if( objectIDToSpawn != INVALID_ID ||
				bossIDToSpawn != INVALID_ID )
			{
				UNIT *unitSpawningFrom( NULL );
				//Get unit from Boss point.
				EQUEST_OBJECTS_CREATABLES createFrom = KQUEST_OBJECT_INTERACT;
				if( objectIDToSpawn == INVALID_ID )
				{
					//This is simply getting the unittype of the object to spawn the monster from in the level
					for( int spFrom = 0; spFrom < KQUEST_OBJECT_COUNT; spFrom++ )
					{
						if( KQUEST_OBJECT_UNITTYPES_ARRAY[ spFrom ] == pQuestTask->nQuestBossSpawnFromUnitType[t] )
						{
							createFrom = (EQUEST_OBJECTS_CREATABLES)spFrom;
							break;
						}
					}					
				}
				
				//Make sure we have a unit to spawn from
				//ASSERT_CONTINUE( unitSpawningFrom );				//we now allow for this
				//spawn object if we need to(spawn item)
				if( objectIDToSpawn != INVALID_ID )
				{

					unitSpawningFrom = s_GetUnit( pGame, createFrom, &params );
					unit = s_SpawnObject( pLevel, unitSpawningFrom, objectIDToSpawn, &params );
					//set the stats so we know it's a quest object
					if( unit )
					{
						UnitSetFlag( unit, UNITFLAG_SHOULD_RESPAWN );
						UnitSetFlag( unit, UNITFLAG_RESPAWN_EXACT );

						s_SetUnitAsQuestUnit( unit, NULL, pQuestTask, t );
						if( bRegisterUnits )
						{
							s_QuestAddTrackableQuestUnit( unit );
						}
						UnitSetFlag(unit, UNITFLAG_DONT_DEPOPULATE, TRUE);

						if( pQuestTask->nQuestObjectRemoveItemWhenUsed[ t ] != INVALID_ID &&
							pQuestTask->nQuestObjectRequiresItemWhenUsed[ t ] > 0 )
						{
							ObjectSetOperateByUnitClass( unit, pQuestTask->nQuestObjectRemoveItemWhenUsed[ t ], t );							
						}

					}
				}
				//spawn boss if we need to (spawn monster)
				if( bossIDToSpawn != INVALID_ID  )
				{
					switch( createFrom )
					{
					case KQUEST_OBJECT_INTERACT:
					case KQUEST_OBJECT_MINI_BOSS:						
					case KQUEST_OBJECT_BOSS:	
						unitSpawningFrom = s_GetUnit( pGame, createFrom, &params );						
						break;
					default:
						unitSpawningFrom = NULL;	//choose random spot					
						break;
					}		

					unit = NULL;
					//if we force the dungeon, we need to spawn the monster at the player. So that is why Player is passed in.
					if( bForceDungeon )
					{
						unit = s_SpawnMonsterObject( pPlayer, pLevel, pPlayer, bossIDToSpawn, pQuestTask, t );					
					}
					else
					{
						unit = s_SpawnMonsterObject( pPlayer, pLevel, unitSpawningFrom, bossIDToSpawn, pQuestTask, t );					
					}
					ASSERT_CONTINUE( unit );
					UnitSetFlag( unit, UNITFLAG_DESTROY_DEAD_NEVER, TRUE );
					UnitSetFlag(unit, UNITFLAG_DONT_DEPOPULATE, TRUE);
					//set the stats so we know it's a quest object
					s_SetUnitAsQuestUnit( unit, NULL, pQuestTask, t );										
					if( bRegisterUnits )
					{
						s_QuestAddTrackableQuestUnit( unit );
					}

				}
			}
		}
	}
	return unit;
}


/*
	When a room gets reactivated we might need to respawn a boss or other things.
*/
void s_QuestRoomActivatedRespawnFromObjects( ROOM *pRoom )
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
	QUEST_LEVEL_INFO_TUGBOAT *pQuestInfo = &pLevel->m_QuestTugboatInfo;	
	if( pQuestInfo->nUnitsTracking == 0 )  //this level isn't tracking any monsters
		return;
	int nIndex = -1;
	for( int t = 0; t < pQuestInfo->nUnitsTracking; t++ )
	{
		if( pQuestInfo->pRoomOfUnit[ t ] == pRoom )
		{
			nIndex = t;
			break;
		}
	}
	if( nIndex == -1 )
		return;
	
	//check respawn on Unit
	UNIT *pUnit = UnitGetById( pGame, pQuestInfo->nUnitIDsTracking[ nIndex ] );
	if( pUnit != NULL )
	{
		s_UnitRespawnFromCorpse( pUnit );
		return; //if the unit isn't null ( it hasn't been destroyed and if it's not dead or dying then we don't need to respawn it
	}

	//we now need to create a new unit
	pUnit = sCreateUnitAtObjectInLevel( pLevel, 
										NULL,
										pQuestInfo->nQuestIndexOfUnit[nIndex ],
										&pQuestInfo->pQuestTaskDefForUnit[ nIndex ],
										1,
										( pQuestInfo->nQuestTypeInQuest[ nIndex ] == (int)GENUS_OBJECT ),
										( pQuestInfo->nQuestTypeInQuest[ nIndex ] == (int)GENUS_MONSTER ),
										FALSE,
										NULL );
	ASSERT_RETURN( pUnit );
	pQuestInfo->nUnitIDsTracking[ nIndex ] = UnitGetId( pUnit ); //now have to reset the id of the unit tracking
	//we're done
	*/
#endif

}

//spawns items when the level is created
//NOTE: An important realization is that everything that gets created is based off of the level area... this is because
//multiple people in the party might need an item to complete a quest or kill a boss and so on. The quests are smart enough
//to know when a player has already killed a boss and won't reward them any cool items and it's smart enough not to quest items they
//already have.
//HOWEVER with random quests the player has to be involved so RANDOM quests will create items based off the player entering the level.
//this is probably ok since the level is a very specialized instance. !THis is NOT OK - What about People in Parties. And the first person
//on the last level doesn't have the quest!
void s_SpawnObjectsForQuestOnLevelAdd( GAME *pGame,
									   LEVEL *pLevelCreated,
									   UNIT *pPlayer,
									   const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTaskToForce )									   
{
	#define 	nMaxArraySizes	50
	const QUEST_TASK_DEFINITION_TUGBOAT	*nQuestTask[nMaxArraySizes];
	UNIT								*pPlayerOwningTask[ nMaxArraySizes ];
	int			nNumberOfQuestTask( 0 );
	UNIT *pPlayersInGame[ nMaxArraySizes ];
	ZeroMemory( nQuestTask, sizeof( QUEST_TASK_DEFINITION_TUGBOAT * ) * nMaxArraySizes );	
	ZeroMemory( pPlayerOwningTask, sizeof( UNIT * ) * nMaxArraySizes );
	ZeroMemory( pPlayersInGame, sizeof( UNIT * ) * nMaxArraySizes );
	
	int nPlayersInGame( 0 );
	pPlayersInGame[ nPlayersInGame++ ] = pPlayer;
	for (GAMECLIENT *pClient = ClientGetFirstInGame( pGame );
		pClient;
		pClient = ClientGetNextInGame( pClient ))
	{
		UNIT *pTeamMember = ClientGetPlayerUnit( pClient );	
		if( pTeamMember != pPlayersInGame[ 0 ] )
		{
			pPlayersInGame[ nPlayersInGame++ ] = pTeamMember;
		}
	}

	//this has got to be one of the slowest ways of doing this.
	int QuestCount = ExcelGetCount(EXCEL_CONTEXT(pGame), DATATABLE_QUESTS_TASKS_FOR_TUGBOAT);
	for( int i = 0; i < QuestCount; i++ )
	{
		const QUEST_TASK_DEFINITION_TUGBOAT * pQuestTaskDefinition = (const QUEST_TASK_DEFINITION_TUGBOAT*)ExcelGetData(EXCEL_CONTEXT(pGame), DATATABLE_QUESTS_TASKS_FOR_TUGBOAT, i );
		
		for( int nPlayerIndex = 0; nPlayerIndex < nPlayersInGame; nPlayerIndex++ )
		{
			pPlayer = pPlayersInGame[ nPlayerIndex ];
			int count( 0 );
			while( count < MAX_OBJECTS_TO_SPAWN &&
				   ( MYTHOS_QUESTS::QuestGetObjectSpawnDungeonByIndex( pPlayer, pQuestTaskDefinition, count ) != INVALID_ID ||
					 MYTHOS_QUESTS::QuestGetBossSpawnDungeonByIndex( pPlayer, pQuestTaskDefinition, count ) != INVALID_ID ) )
			{
				BOOL bForceDungeon( pQuestTaskToForce != NULL && pQuestTaskToForce == pQuestTaskDefinition );
				//check to see if we are in the right dungeon
				if( MYTHOS_QUESTS::QuestGetObjectSpawnDungeonByIndex( pPlayer, pQuestTaskDefinition, count ) == LevelGetLevelAreaID( pLevelCreated ) ||
					MYTHOS_QUESTS::QuestGetBossSpawnDungeonByIndex( pPlayer, pQuestTaskDefinition, count ) == LevelGetLevelAreaID( pLevelCreated ) ||
					bForceDungeon )
				{
					//check to seee if we are on the correct floor
					//-1 means only last level			
					if( bForceDungeon ||
						( MYTHOS_QUESTS::QuestGetObjectSpawnDungeonFloorByIndex( pQuestTaskDefinition, count ) == -1 &&
						MYTHOS_QUESTS::QuestGetBossSpawnDungeonFloorByIndex( pQuestTaskDefinition, count ) == -1 &&
						  pLevelCreated->m_LevelAreaOverrides.bIsLastLevel &&
						  pLevelCreated->m_pLevelDefinition->bSafe == false ) ||	//spawn on last level
						MYTHOS_QUESTS::QuestGetObjectSpawnDungeonFloorByIndex( pQuestTaskDefinition, count ) == pLevelCreated->m_nDepth ||
						MYTHOS_QUESTS::QuestGetBossSpawnDungeonFloorByIndex( pQuestTaskDefinition, count ) == pLevelCreated->m_nDepth)
					{
						
						nQuestTask[ nNumberOfQuestTask ] = pQuestTaskDefinition;										
						pPlayerOwningTask[ nNumberOfQuestTask ] = pPlayer;
						nNumberOfQuestTask++;
						nPlayerIndex = nPlayersInGame;
						ASSERTX_RETURN( nNumberOfQuestTask < nMaxArraySizes, "Error Occured in Spawn Objects ( tugboat ). To many quests referancing a dungeon" );		
						break;
					}				
				}
				count++;
			}
		}
	}

	//if we have no quest for this dungeon just leave
	if( nNumberOfQuestTask == 0 )
		return;
	//actually create the objects
	sCreateUnitAtObjectInLevel( pLevelCreated, &pPlayerOwningTask[0], -1, &nQuestTask[0], nNumberOfQuestTask, TRUE, TRUE, TRUE, NULL );


}

BOOL sSetMonsterInteractable(
	GAME * game,
	UNIT * unit,
	const struct EVENT_DATA & event_data)
{
	//ok so the task is complete and we need to show a dialog. So lets make the monster clickable	
	UnitChangeTargetType( unit, TARGET_GOOD );
	UnitSetInteractive( unit, UNIT_INTERACTIVE_ENABLED );
	UNIT *pPlayer = UnitGetById( game, (UNITID)event_data.m_Data1 );
	if( pPlayer )
	{		
		s_QuestUpdateQuestNPCs( game, pPlayer, unit );
	}
	return FALSE;
}

//this spawns items if need be.
BOOL s_QuestSpawnItemFromCreatedObject( UNIT *unitSpawning,
									    UNIT *pPlayer,	
										UNIT *pClientPlayer,
									    BOOL monster )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	if( !unitSpawning ||
		//!pPlayer ||
		!pClientPlayer )
	{
		return FALSE;
	}	
	//check and make sure the unit is a Quest unit
	if( !QuestUnitIsQuestUnit( unitSpawning ) )
		return INVALID_ID;

	if( monster )
	{		
		s_StateSet( unitSpawning, unitSpawning, STATE_NO_RESPAWN_ALLOWED, BOSS_RESPAWN_DELAY * GAME_TICKS_PER_SECOND / MSECS_PER_SEC );		
	}

	int taskID = UnitGetStat( unitSpawning, STATS_QUEST_TASK_REF );
	int index = UnitGetStat( unitSpawning, STATS_QUEST_ID_REF );

	//must be a quest created from an object
	GAME *pGame = UnitGetGame( unitSpawning );
	const QUEST_TASK_DEFINITION_TUGBOAT *pQuestTask = QuestTaskDefinitionGet( taskID ); 
	if( pQuestTask == NULL )
		return FALSE;



	//lets make sure they have the parent quest. We don't want to do task here
	//because you might have completed a task that is further down in the overall 
	//quest. There shouldn't be multiple quest per dungeon. That could lead to 
	//some bad mojo.

	if( QuestPlayerHasTask( pClientPlayer, pQuestTask ) == FALSE )
	{
		return INVALID_ID; //we won't spawn the item.
	}

	if( s_IsTaskObjectCompleted( pGame,
								 pClientPlayer,
								 pQuestTask,
								 index,
								 monster,
								 TRUE ) )
	{
		return INVALID_ID; //quest item has already been created and player has object
	}

	int itemID( INVALID_ID );
	BOOL bInteractedIndex( FALSE );
	if( monster )
	{
		itemID = MYTHOS_QUESTS::QuestGetBossIDSpawnsItemIndex( pClientPlayer, pQuestTask, index );
	}
	else
	{
		itemID = MYTHOS_QUESTS::QuestGetObjectSpawnItemToSpawnByIndex( pQuestTask, index );
	}
	
	if( itemID != INVALID_LINK )
	{

		BOOL bGiveToPlayer = ( MYTHOS_QUESTS::QuestGetObjectSpawnItemDirectToPlayerByIndex(pQuestTask, index) > 0 );
		//spawn the item
		UNIT *itemCreated = s_SpawnItemFromUnit( unitSpawning,
												 pClientPlayer,												 
												 itemID );
		if( itemCreated )
		{
			//set stats for it - so we know it's a quest item
			s_SetUnitAsQuestUnit( itemCreated,
				                  pClientPlayer,
								  pQuestTask,
								  index );

			if( bGiveToPlayer )
			{
				UnitSetRestrictedToGUID( itemCreated, UnitGetGuid( pPlayer ) );
				//this could fail but it would just put the item on the ground which is ok.
				s_ItemPickup( pPlayer, itemCreated );
			}

		}
		bInteractedIndex = TRUE; 
	}
	else if( monster )
	{		
		//ONLY IF IT'S A MONSTER AND IT DOESN'T DROP AN ITEM
		//MAKE IT NOT APPEAR AGAIN						
		s_SetTaskObjectCompleted( pGame,
								 pClientPlayer,
								 pQuestTask,
								 index,
								 monster );
								 
		bInteractedIndex = TRUE;

	}

	//Make it so if there is a dialog that it will appear. - if a monster
	if( monster &&		
		pQuestTask->nQuestDialogOnTaskCompleteAnywhere != INVALID_LINK )
	{
		//we don't want the monster clickable right off the bat. This causes the dialog to pop right up. Lets wait half a second
		UnitRegisterEventTimed( unitSpawning, sSetMonsterInteractable, EVENT_DATA( UnitGetId( pClientPlayer ) ), GAME_TICKS_PER_SECOND/2);		
	}
	if( unitSpawning && monster && bInteractedIndex )
	{
		s_QuestSendMessageBossDefeated( pPlayer, unitSpawning, pQuestTask );
			
	}
	return bInteractedIndex;
#else
	return FALSE;
#endif
}



