//----------------------------------------------------------------------------
// FILE: quest_collect.h
//
// The collect quest is a template quest, in which the player must collect 
// items from a specified treasure class. You can also specify a class of 
// monster to get the drops from, and a level to find them in.
//
// Code is adapted from the forge and urgent collect tasks, in tasks.cpp.
// 
// author: Chris March, 1-29-2007
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "c_quests.h"
#include "game.h"
#include "globalindex.h"
#include "npc.h"
#include "quest_collect.h"
#include "quest_template.h"
#include "quests.h"
#include "dialog.h"
#include "offer.h"
#include "level.h"
#include "s_quests.h"
#include "Quest.h"
#include "items.h"
#include "treasure.h"
#include "monsters.h"
#include "states.h"
#include "picker.h"

#define MAX_QUEST_COLLECT_MONSTER_SPAWN_PER_ROOM	(6.0f)

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_COLLECT * sQuestDataGet( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return (QUEST_DATA_COLLECT *)pQuest->pQuestData;
}		

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestCollectDataInit(
	QUEST *pQuest,
	QUEST_DATA_COLLECT *pQuestData)
{
	ASSERT_RETURN( pQuestData != NULL );
	ZeroMemory( pQuestData, sizeof(QUEST_DATA_COLLECT) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sPlayerUpdateCollectedCount(
	UNIT *pPlayer,
	GAME* pGame,
	QUEST* pQuest)
{
	// what are we looking for
	SPECIES spItem = MAKE_SPECIES( GENUS_ITEM, pQuest->pQuestDef->nCollectItem );

	// it must actually be on their person
	DWORD dwFlags = 0;
	SETBIT( dwFlags, IIF_ON_PERSON_SLOTS_ONLY_BIT );
	SETBIT( dwFlags, IIF_IGNORE_EQUIP_SLOTS_BIT );

	int nNumCollectItemsInInventory = 
		UnitInventoryCountUnitsOfType( pPlayer, spItem, dwFlags );
	UnitSetStat( pPlayer, STATS_QUEST_NUM_COLLECTED_ITEMS, pQuest->nQuest, 0, UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT ), nNumCollectItemsInInventory ); // TODO cmarch multiple item classes?
	return nNumCollectItemsInInventory;
}

//-------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sCompareInts( const void * pFirst, const void * pSecond )
{
	int *p1 = (int *) pFirst;
	int *p2 = (int *) pSecond;
	if ( *p1 > *p2 )
		return 1;
	else if ( *p1 < *p2 )
		return -1;
	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
QUEST_MESSAGE_RESULT s_QuestCollectMessageEnterLevel( 
	GAME* pGame,
	QUEST* pQuest,
	const QUEST_MESSAGE_ENTER_LEVEL *pMessageEnterLevel,
	QUEST_DATA_COLLECT *pData )
{
#if !ISVERSION( CLIENT_ONLY_VERSION )

	const QUEST_DEFINITION *pQuestDef = pQuest->pQuestDef;
	ASSERTX_RETVAL( pQuestDef, QMR_OK, "expected QUEST_DEFINITION*" );

	if ( pQuestDef->nLevelDefDestinations[0] != INVALID_LINK &&
		 pQuestDef->nLevelDefDestinations[0] != 
				pMessageEnterLevel->nLevelNewDef )
		return QMR_OK;

	ASSERTV_RETVAL( pData, QMR_OK, "quest \"%s\": expected quest data", pQuestDef->szName );
	if (pData->bDataCalcAttempted)
		return QMR_OK;

	pData->bDataCalcAttempted = TRUE;

	// calculate spawn and drop data
	
	// calculate the max number of monsters that must be killed to spawn all the quest items
	float fMaxMonsterSpawnCount = 
		(pQuestDef->fCollectDropRate <= 0.0f || pQuestDef->nObjectiveCount <= 0) ? 
			0.0f : 
			ceilf( float( pQuestDef->nObjectiveCount )/pQuestDef->fCollectDropRate );
	if ( fMaxMonsterSpawnCount > MAX_QUEST_COLLECT_MONSTER_SPAWN )
	{
		ASSERTV( FALSE, "quest \"%s\": \"Objective Count\" divided by \"Collect Spawn Rate\" is too large! please decrease \"Objective Count\" and/or increase \"Collect Spawn Rate\"", pQuestDef->szName );
		fMaxMonsterSpawnCount = MAX_QUEST_COLLECT_MONSTER_SPAWN;
	}
	int nMaxMonsterSpawnCount = PrimeFloat2Int(fMaxMonsterSpawnCount);
	
	// calculate the max number of spawn rooms

	UNIT *pPlayer = QuestGetPlayer( pQuest );
	ASSERTV_RETVAL( pPlayer, QMR_OK, "quest \"%s\": expected player", pQuestDef->szName );
	LEVEL *pLevel = UnitGetLevel( pPlayer );
	ASSERTV_RETVAL( pLevel, QMR_OK, "quest \"%s\": expected level", pQuestDef->szName );

	// pick spawn rooms
	{
		PickerStart( pGame, spawnRoomPicker );
		for (ROOM *pRoom = LevelGetFirstRoom( pLevel ); pRoom; pRoom = LevelGetNextRoom( pRoom ))
		{
			ASSERTV_BREAK( pRoom, "quest \"%s\": expected room", pQuestDef->szName );
			if (RoomIsInDefaultSubLevel( pRoom ) && 
				RoomAllowsMonsterSpawn( pRoom ) &&
				!RoomTestFlag( pRoom, ROOM_HAS_NO_SPAWN_NODES_BIT ))
			{
				PickerAdd(MEMORY_FUNC_FILELINE(pGame, RoomGetId( pRoom ), 1));
			}
		}

		// no spawn rooms found, relax rules
		if (PickerGetCount( pGame) <= 0)
			for (ROOM *pRoom = LevelGetFirstRoom( pLevel ); pRoom; pRoom = LevelGetNextRoom( pRoom ))
			{
				ASSERTV_BREAK( pRoom, "quest \"%s\": expected room", pQuestDef->szName );
				if (RoomIsInDefaultSubLevel( pRoom ) && 
					RoomAllowsMonsterSpawn( pRoom ))
				{
					PickerAdd(MEMORY_FUNC_FILELINE(pGame, RoomGetId( pRoom ), 1));
				}
			}

		// no spawn rooms found, relax rules
		if (PickerGetCount( pGame) <= 0)
			for (ROOM *pRoom = LevelGetFirstRoom( pLevel ); pRoom; pRoom = LevelGetNextRoom( pRoom ))
			{
				ASSERTV_BREAK( pRoom, "quest \"%s\": expected room", pQuestDef->szName );
				if (RoomAllowsMonsterSpawn( pRoom ))
				{
					PickerAdd(MEMORY_FUNC_FILELINE(pGame, RoomGetId( pRoom ), 1));
				}
			}

		// no spawn rooms found, relax rules
		if (PickerGetCount( pGame) <= 0)
			for (ROOM *pRoom = LevelGetFirstRoom( pLevel ); pRoom; pRoom = LevelGetNextRoom( pRoom ))
			{
				ASSERTV_BREAK( pRoom, "quest \"%s\": expected room", pQuestDef->szName );
				PickerAdd(MEMORY_FUNC_FILELINE(pGame, RoomGetId( pRoom ), 1));
			}

		int nMaxSpawnRooms = 
			MIN(MAX_QUEST_COLLECT_SPAWN_ROOMS, PickerGetCount( pGame ));

		// calculate the max number of monsters to spawn in a spawn room
		float fMaxMonsterSpawnsPerRoom = (nMaxSpawnRooms <= 0) ? 0 : ceilf(fMaxMonsterSpawnCount/float(nMaxSpawnRooms));
		if (fMaxMonsterSpawnsPerRoom > 0.0f)
		{
			if (fMaxMonsterSpawnsPerRoom > MAX_QUEST_COLLECT_MONSTER_SPAWN_PER_ROOM)
			{
				// don't overwhelm the player with a ton of spawns, quest data should probably be adjusted, but make sure here
				fMaxMonsterSpawnsPerRoom = MAX_QUEST_COLLECT_MONSTER_SPAWN_PER_ROOM;
				fMaxMonsterSpawnCount = fMaxMonsterSpawnsPerRoom * float(nMaxSpawnRooms);
				nMaxMonsterSpawnCount = PrimeFloat2Int(fMaxMonsterSpawnCount);
				ASSERTV(FALSE, "quest \"%s\": increasing quest item drop rate, to limit quest monster spawns. Please increase \"Collect Item Drop Rate\" or decrease \"Objective Count\"", pQuestDef->szName);
			}

			// reduce the number of spawn rooms so we don't go overboard by a ton
			nMaxSpawnRooms = 
				MIN(nMaxSpawnRooms, FLOAT_TO_INT_CEIL(fMaxMonsterSpawnCount/fMaxMonsterSpawnsPerRoom));
		}
		pData->nMaxMonsterSpawnsPerRoom = PrimeFloat2Int(fMaxMonsterSpawnsPerRoom);

		ASSERTV_RETVAL( pData->nSpawnRoomCount == 0, QMR_OK, "quest \"%s\": spawn room count to start at zero", pQuestDef->szName );
		ASSERTV_RETVAL( nMaxSpawnRooms <= arrsize( pData->idSpawnRooms ), QMR_OK, "quest \"%s\": too many spawn rooms calculated", pQuestDef->szName );
		while ( pData->nSpawnRoomCount < nMaxSpawnRooms && PickerGetCount( pGame ) > 0 )
		{
			pData->idSpawnRooms[ pData->nSpawnRoomCount++ ] = PickerChooseRemove( pGame );
		}
	}

	// pick the kill events that will spawn a quest item drop
	{
		PickerStart( pGame, killDropPicker );
		for ( int i = 1; i <= nMaxMonsterSpawnCount; ++i)
		{
			PickerAdd(MEMORY_FUNC_FILELINE(pGame, i, 1));
		}
		
		int nKillArrayDataCount = MIN( pQuestDef->nObjectiveCount, MAX_QUEST_COLLECT_OBJECTIVE_COUNT );
		for ( int i = 0; i < nKillArrayDataCount && PickerGetCount( pGame ) > 0; ++i)
		{
			pData->nKillCountsForDrops[ i ] = PickerChooseRemove( pGame );
		}

		qsort( pData->nKillCountsForDrops, nKillArrayDataCount, sizeof(int), sCompareInts );
	}

#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )
	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int s_sQuestCollectAttemptToSpawnQuestMonsters(
	GAME *pGame,
	QUEST *pQuest,
	LEVEL *pLevel,
	UNIT *pPlayer,
	int nMax, /* = 0 */
	ROOM *pRoom /* = NULL */ )
{
	const QUEST_DEFINITION *pQuestDef = QuestGetDefinition( pQuest );
	if ( !pQuest->pQuestDef->bDisableSpawning  &&
		 ( pQuestDef->nLevelDefDestinations[0] == INVALID_LINK || 
		   pQuestDef->nLevelDefDestinations[0] == LevelGetDefinitionIndex( pLevel ) ) )
	{
		// wait a bit after a unique is killed for a drop before respawning another,
		// they may have been in the process of picking up the drop
		int nMonsterQuality = INVALID_LINK;
		BOOL bMonsterIsUnique = FALSE;
		if ( pQuestDef->nObjectiveMonster != INVALID_LINK )
		{		
			const UNIT_DATA * pMonsterData = 
				UnitGetData(pGame, GENUS_MONSTER, pQuestDef->nObjectiveMonster);
			ASSERTV_RETZERO( pMonsterData != NULL, "%s quest: NULL UNIT_DATA for collect Objective Monster", pQuestDef->szName );
			nMonsterQuality = pMonsterData->nMonsterQualityRequired;
			QUEST_DATA_COLLECT *pQuestData = sQuestDataGet( pQuest );
			bMonsterIsUnique = MonsterQualityGetType( nMonsterQuality ) >= MQT_UNIQUE;
			if ( bMonsterIsUnique &&
				 pQuestData->bDidKill &&
				 AppCommonGetCurTime() - pQuestData->tiLastKillTime <= 300000 ) // wait 5 minutes
			{
				return 0;
			}
		}

		int nCollected = UnitGetStat( pPlayer, STATS_QUEST_NUM_COLLECTED_ITEMS, pQuest->nQuest, 0, UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT ) ); // TODO cmarch multiple item classes?

		// need to kill more? make sure there are enough in the level
		if (pQuestDef->fCollectDropRate <= 0.0f)
			return 0;

		int nKillsLeft = 
			(CEILF( float( pQuestDef->nObjectiveCount - nCollected ) / pQuestDef->fCollectDropRate ) );
		int nDesiredMonstersInLevel = nKillsLeft;
		nDesiredMonstersInLevel = MIN( nDesiredMonstersInLevel, int(MAX_QUEST_COLLECT_MONSTER_SPAWN) );
		int nMonstersInLevel = 
			LevelCountMonsters( pLevel, pQuestDef->nObjectiveMonster, pQuestDef->nObjectiveUnitType );
		if ( nMonstersInLevel < nDesiredMonstersInLevel )
		{
			// spawn more.
			// cap the amount to spawn at one time, to limit any 
			// performance problems. Plus, the player might get better than
			// average luck on the drops, and not have to kill as many
			// as usual. Uniques should probably just spawn 1, unless the
			// quest data says otherwise
			int nMonstersToSpawn = 
					( nMax > 0 ) ? 
						nMax : 
						( bMonsterIsUnique ? 
							MIN( ( nDesiredMonstersInLevel - nMonstersInLevel ), int( RandGetNum( pGame, 3, 4 ) ) ) : 
							int( RandGetNum( pGame, 5, 6 ) ) ); 
			return s_LevelSpawnMonsters( 
				pGame, 
				pLevel, 
				pQuestDef->nObjectiveMonster, 
				nMonstersToSpawn,
				nMonsterQuality, 
				pRoom,
				FALSE); 
		}
	}
	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
QUEST_MESSAGE_RESULT s_QuestCollectMessageRoomsActivated( 
	GAME* pGame,
	QUEST* pQuest,
	QUEST_DATA_COLLECT *pData)
{
	UNIT *pPlayer = QuestGetPlayer( pQuest );
	ASSERTX_RETVAL( pPlayer, QMR_OK, "expected quest player" );
	const QUEST_DEFINITION *pQuestDef = QuestGetDefinition( pQuest );
	ASSERTX_RETVAL( pQuestDef, QMR_OK, "expected quest definition" );

	if ( ( pQuestDef->nLevelDefDestinations[0] == INVALID_LINK || 
		   pQuestDef->nLevelDefDestinations[0] == 
				UnitGetLevelDefinitionIndex( pPlayer ) ) )
	{
		ASSERTX_RETVAL( pQuest->pQuestDef, QMR_OK, "quest \"%s\": expected quest def");
		ASSERTV_RETVAL( pData, QMR_OK, "quest \"%s\": expected quest data", pQuest->pQuestDef->szName );
		QUEST_GLOBALS *pQuestGlobals = QuestGlobalsGet( pPlayer );
		LEVEL *pLevel = UnitGetLevel( pPlayer );
		ASSERTV_RETVAL( pLevel, QMR_OK, "quest \"%s\": expected level", pQuest->pQuestDef->szName );
		// only spawn in the already picked spawn rooms, when they are first activated
		for (int i = 0; i < pData->nSpawnRoomCount; ++i )
		{
			if (pData->idSpawnRooms[i] != INVALID_ID)
			{
				ROOM *pRoom = RoomGetByID( pGame, pData->idSpawnRooms[i] );
				if (pRoom)
				{
					ASSERTX_CONTINUE( pRoom->nIndexInLevel >= 0 && pRoom->nIndexInLevel < pLevel->m_nRoomCount, "quest collect: room index out of range" );
					if (TESTBIT(pQuestGlobals->dwRoomsActivated, pRoom->nIndexInLevel))
					{
						if (s_sQuestCollectAttemptToSpawnQuestMonsters(
								pGame,
								pQuest,
								UnitGetLevel( pPlayer ),
								pPlayer,
								pData->nMaxMonsterSpawnsPerRoom,
								pRoom) >= pData->nMaxMonsterSpawnsPerRoom)
						{
							pData->idSpawnRooms[i] = INVALID_ID; // don't try to spawn for this room again
						}
					}
				}
			}
		}
	}
	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sUnitIsACollectItem(
	UNIT *pItem,
	QUEST *pQuest)
{
	if (UnitIsA( pItem, UNITTYPE_ITEM ) &&
		pQuest->pQuestDef->nCollectItem == UnitGetClass( pItem ))
	{
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT s_sQuestCollectMessagePickup( 
	GAME* pGame,
	QUEST* pQuest,
	const QUEST_MESSAGE_PICKUP* pMessage)
{
	UNIT* pPlayer = pMessage->pPlayer;
	ASSERTX_RETVAL( pPlayer == QuestGetPlayer( pQuest ), QMR_OK, "Player should be the owner of this quest" );

	// check for completing the collection
	if ( QuestStateGetValue( pQuest, QUEST_STATE_COLLECTTEMPLATE_COLLECT ) == QUEST_STATE_ACTIVE &&
		 sUnitIsACollectItem( pMessage->pPickedUp, pQuest ) )
	{
		int nNumCollected = UnitGetStat( pPlayer, STATS_QUEST_NUM_COLLECTED_ITEMS, pQuest->nQuest, 0, UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT ));
		++nNumCollected;
		UnitSetStat( pPlayer, STATS_QUEST_NUM_COLLECTED_ITEMS, pQuest->nQuest, 0, UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT ), nNumCollected );

		if ( nNumCollected == 1 )
			QuestAutoTrack( pQuest );

		if ( nNumCollected >= pQuest->pQuestDef->nObjectiveCount ) // TODO cmarch multiple item classes?
		{
			QuestStateSet( pQuest, QUEST_STATE_COLLECTTEMPLATE_COLLECT, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_COLLECTTEMPLATE_COLLECT_COUNT, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_COLLECTTEMPLATE_RETURN, QUEST_STATE_ACTIVE );
		}
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT s_sQuestCollectMessageDroppingItem( 
	GAME* pGame,
	QUEST* pQuest,
	const QUEST_MESSAGE_DROPPED_ITEM* pMessage)
{
	UNIT* pPlayer = pMessage->pPlayer;
	ASSERTX_RETVAL( pPlayer == QuestGetPlayer( pQuest ), QMR_OK, "Player should be the owner of this quest" );

	// check for completing the collection
	if ( QuestIsActive( pQuest )  &&
		 pQuest->pQuestDef->nCollectItem == pMessage->nItemClass )
	{
		int nNumCollected = sPlayerUpdateCollectedCount( pPlayer, pGame, pQuest );
		if ( nNumCollected < pQuest->pQuestDef->nObjectiveCount &&
			 QuestStateGetValue( pQuest, QUEST_STATE_COLLECTTEMPLATE_RETURN ) >= QUEST_STATE_ACTIVE )
		{
			QuestStateSet( pQuest, QUEST_STATE_COLLECTTEMPLATE_COLLECT, QUEST_STATE_HIDDEN );
			QuestStateSet( pQuest, QUEST_STATE_COLLECTTEMPLATE_COLLECT_COUNT, QUEST_STATE_HIDDEN );
			QuestStateSet( pQuest, QUEST_STATE_COLLECTTEMPLATE_RETURN, QUEST_STATE_HIDDEN );

			QuestStateSet( pQuest, QUEST_STATE_COLLECTTEMPLATE_COLLECT, QUEST_STATE_ACTIVE );
			QuestStateSet( pQuest, QUEST_STATE_COLLECTTEMPLATE_COLLECT_COUNT, QUEST_STATE_ACTIVE );
		}

		if ( nNumCollected < 1 )
			s_QuestTrack(pQuest, FALSE);
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//When a unit is killed, call this function. It will spawn any items needed to be spawned
//----------------------------------------------------------------------------
QUEST_MESSAGE_RESULT s_QuestCollectMessagePartyKill( 
	QUEST *pQuest,
	const QUEST_MESSAGE_PARTY_KILL *pMessage,
	QUEST_DATA_COLLECT *pQuestData)
{
	UNIT *pPlayer = QuestGetPlayer( pQuest );
	UNIT *pVictim =pMessage->pVictim;
	UNIT *pAttacker = pMessage->pKiller;

	if( pAttacker == NULL ||
		pVictim == NULL ||
		pPlayer == NULL ||
		pQuest == NULL )
		return QMR_OK;

	// check if quest is active, and monster and level requirements are met
	
	// must match the unit type if specified, else match the monster 
	// class (but not if checking unit type, since multiple monster 
	// class hierarchies can match same unit type)
	const QUEST_DEFINITION *pQuestDef = pQuest->pQuestDef;
	if ( QuestIsActive( pQuest ) &&
		 ( pQuestDef->nLevelDefDestinations[0] == INVALID_LINK || 
		   pQuestDef->nLevelDefDestinations[0] == 
				UnitGetLevelDefinitionIndex( pVictim ) ) )
	{
		BOOL bMonsterMatches = FALSE;
		// must match the unit type if specified, else match the monster 
		// class (but not if checking unit type, since multiple monster 
		// class hierarchies can match same unit type)
		if ( pQuestDef->nObjectiveUnitType != INVALID_LINK )
		{
			if ( UnitIsA( pVictim, pQuestDef->nObjectiveUnitType ) )
				bMonsterMatches = TRUE;
		}
		else if ( UnitIsA( pVictim, UNITTYPE_MONSTER ) &&			// no destructibles
				  ( pQuestDef->nObjectiveMonster == INVALID_LINK ||
					UnitIsInHierarchyOf( 
						GENUS_MONSTER, 
						UnitGetClass( pVictim ),
						&pQuestDef->nObjectiveMonster,
						1 ) ) )
		{
			bMonsterMatches = TRUE;
		}

		if (bMonsterMatches &&
			UnitGetStat( 
				pPlayer, 
				STATS_QUEST_NUM_COLLECTED_ITEMS, 
				pQuest->nQuest, 
				0, 
				UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT ) ) < pQuestDef->nObjectiveCount) // TODO cmarch multiple item classes?
		{
			// reset spawn timer
			pQuestData->tiLastKillTime = AppCommonGetCurTime();
			pQuestData->bDidKill = TRUE;
			++pQuestData->nKillCount;

			// Spawn a quest item on the predetermined kill events until all the drops
			// are spawned. If the player hasn't collected all the items after the
			// required number are dropped, keep dropping them with a small chance
			// The player may have missed some and continued on.
			ASSERTV_RETVAL( pQuestData->nDropCount >= 0, QMR_OK, "quest \"%s\" unexpected negative drop count", pQuestDef->szName );
			if( ( pQuestData->nDropCount < pQuestDef->nObjectiveCount &&
					( pQuestData->nDropCount >= arrsize( pQuestData->nKillCountsForDrops ) ||
					  pQuestData->nKillCount >= pQuestData->nKillCountsForDrops[ pQuestData->nDropCount ] ) ) ||
				( pQuestData->nDropCount >= pQuestDef->nObjectiveCount &&
					RandGetFloat( UnitGetGame( pPlayer ) ) <= 0.01f ) )

			{
				//spawns the item from the defender
				UNIT *pCreated = 
					s_SpawnItemFromUnit( 
						pVictim,
						pPlayer,
						pQuestDef->nCollectItem);

				ASSERTV( pCreated != NULL, "quest \"%s\": failed to collection item", pQuestDef->szName );
				if( pCreated != NULL )
				{
					// Allow the player to pick it up, but not the party.
					// A monster kill by a party member can result in a quest
					// items spawned for another party member.
					UnitSetRestrictedToGUID( 
						pCreated, 
						UnitGetGuid( pPlayer ) );
					UnitSetStat(pCreated, STATS_QUEST_ITEM, UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT ), pQuest->nQuest );
					++pQuestData->nDropCount;
				}
			}
		}
	}
	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestCollectOnEnterGame(
	const QUEST_FUNCTION_PARAM &tParam)
{
	UNIT *pPlayer = tParam.pPlayer;
	QUEST *pQuest = tParam.pQuest;

	// reset item count stat
	GAME *pGame = UnitGetGame( pPlayer );
	if (!QuestIsActive(pQuest))
	{
		s_QuestTrack( pQuest, FALSE );
		return;
	}

	int nNumCollected = sPlayerUpdateCollectedCount( pPlayer, pGame, pQuest );
	if ( nNumCollected < pQuest->pQuestDef->nObjectiveCount )
	{
		// don't have all the items yet
		if (QuestStateGetValue( pQuest, QUEST_STATE_COLLECTTEMPLATE_RETURN ) >= QUEST_STATE_ACTIVE )
		{
			s_QuestTrack( pQuest, (nNumCollected > 0) );

			// roll back from return state
			QuestStateSet( pQuest, QUEST_STATE_COLLECTTEMPLATE_COLLECT, QUEST_STATE_HIDDEN );
			QuestStateSet( pQuest, QUEST_STATE_COLLECTTEMPLATE_COLLECT_COUNT, QUEST_STATE_HIDDEN );
			QuestStateSet( pQuest, QUEST_STATE_COLLECTTEMPLATE_RETURN, QUEST_STATE_HIDDEN );

			QuestStateSet( pQuest, QUEST_STATE_COLLECTTEMPLATE_COLLECT, QUEST_STATE_ACTIVE );
			QuestStateSet( pQuest, QUEST_STATE_COLLECTTEMPLATE_COLLECT_COUNT, QUEST_STATE_ACTIVE );
		}
	}
	else
	{
		// got all the items
		if (QuestStateGetValue( pQuest, QUEST_STATE_COLLECTTEMPLATE_RETURN ) < QUEST_STATE_ACTIVE )
		{
			s_QuestTrack( pQuest, TRUE );

			// set to return state
			QuestStateSet( pQuest, QUEST_STATE_COLLECTTEMPLATE_COLLECT, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_COLLECTTEMPLATE_COLLECT_COUNT, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_COLLECTTEMPLATE_RETURN, QUEST_STATE_ACTIVE );
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT s_sQuestMessageNPCInteract(
	QUEST *pQuest,
	const QUEST_MESSAGE_INTERACT *pMessage )
{
#if !ISVERSION( CLIENT_ONLY_VERSION )
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pPlayer = UnitGetById( pGame, pMessage->idPlayer );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );
	ASSERTX_RETVAL( pPlayer, QMR_OK, "Invalid player" );
	ASSERTX_RETVAL( pSubject, QMR_OK, "Invalid NPC" );	
	const QUEST_DEFINITION *pQuestDefinition = QuestGetDefinition( pQuest->nQuest );
	ASSERT_RETVAL( pQuestDefinition, QMR_OK );

	// Quest giver
	if (QuestIsInactive( pQuest ))
	{
		if (s_QuestIsGiver( pQuest, pSubject ) &&
			s_QuestCanBeActivated( pQuest ))
		{
			// Activate the quest			
			s_QuestDisplayDialog(
				pQuest,
				pSubject,
				NPC_DIALOG_OK_CANCEL,
				pQuestDefinition->nDescriptionDialog );

			return QMR_NPC_TALK;
		}
	}
	else if (QuestIsActive( pQuest ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_COLLECTTEMPLATE_COLLECT ) == QUEST_STATE_COMPLETE )
		{
			if ( s_QuestIsRewarder( pQuest,  pSubject ) )
			{
				// Reward: tell the player good job, and offer rewards
				s_QuestDisplayDialog(
					pQuest,
					pSubject,
					NPC_DIALOG_OK_CANCEL,
					pQuestDefinition->nCompleteDialog,
					pQuestDefinition->nRewardDialog );

				return QMR_NPC_TALK;
			}
		}
		else
		{
			if ( s_QuestIsRewarder( pQuest,  pSubject ) )
			{
				// Incomplete: tell the player to go kill the monster
				s_QuestDisplayDialog(
					pQuest,
					pSubject,
					NPC_DIALOG_OK,
					pQuestDefinition->nIncompleteDialog );

				return QMR_NPC_TALK;
			}
		}
	}
#endif			
	return QMR_OK;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT s_sQuestMessageTalkStopped(
	QUEST *pQuest,
	const QUEST_MESSAGE_TALK_STOPPED *pMessage )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	int nDialogStopped = pMessage->nDialog;
	const QUEST_DEFINITION *pQuestDefinition = QuestGetDefinition( pQuest );
	UNIT *pPlayer = QuestGetPlayer( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pTalkingTo = UnitGetById( pGame, pMessage->idTalkingTo );

	if ( nDialogStopped != INVALID_LINK )
	{
		if ( nDialogStopped == pQuestDefinition->nDescriptionDialog )
		{
			if ( QuestIsInactive( pQuest ) &&
				 s_QuestIsGiver( pQuest, pTalkingTo ) &&
				 s_QuestCanBeActivated( pQuest ) )
			{
				// activate quest
				if ( s_QuestActivate( pQuest ) )
				{
					QuestStateSet( pQuest, QUEST_STATE_COLLECTTEMPLATE_COLLECT, QUEST_STATE_ACTIVE );
					QuestStateSet( pQuest, QUEST_STATE_COLLECTTEMPLATE_COLLECT_COUNT, QUEST_STATE_ACTIVE );

					// reset item count
					GAME *pGame = UnitGetGame( pPlayer );
					
					// check for immediate completion
					int nNumCollected = sPlayerUpdateCollectedCount( pPlayer, pGame, pQuest );
					if (nNumCollected > 0)
						s_QuestTrack( pQuest, TRUE );

					if ( nNumCollected >= pQuestDefinition->nObjectiveCount ) // TODO cmarch multiple item classes?
					{
						QuestStateSet( pQuest, QUEST_STATE_COLLECTTEMPLATE_COLLECT, QUEST_STATE_COMPLETE );
						QuestStateSet( pQuest, QUEST_STATE_COLLECTTEMPLATE_COLLECT_COUNT, QUEST_STATE_COMPLETE );
						QuestStateSet( pQuest, QUEST_STATE_COLLECTTEMPLATE_RETURN, QUEST_STATE_ACTIVE );
					}
					else 
					{
						// starting in the quest destination? spawn monsters in the rooms already activated by the player
						s_QuestCollectMessageRoomsActivated(UnitGetGame( pPlayer ), pQuest, sQuestDataGet( pQuest ) );
					}
				}
			}
		}
		else if ( nDialogStopped == pQuestDefinition->nCompleteDialog )
		{
			if ( QuestStateGetValue( pQuest, QUEST_STATE_COLLECTTEMPLATE_RETURN ) == QUEST_STATE_ACTIVE &&
				 s_QuestIsRewarder( pQuest, pTalkingTo ) )
			{
				// complete quest
				QuestStateSet( pQuest, QUEST_STATE_COLLECTTEMPLATE_RETURN, QUEST_STATE_COMPLETE );

				if ( s_QuestComplete( pQuest ) )
				{			
					s_QuestRemoveQuestItems( pPlayer, pQuest );

					// offer reward
					if ( s_QuestOfferReward( pQuest, pTalkingTo, FALSE ) )
					{					
						return QMR_OFFERING;
					}
				}	
			}
		}
	}
#endif // !ISVERSION(CLIENT_ONLY_VERSION)

	return QMR_OK;		
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
QUEST_MESSAGE_RESULT s_QuestCollectMessageCreateLevel(
	QUEST *pQuest,
	const QUEST_MESSAGE_CREATE_LEVEL *pMessageCreateLevel,
	QUEST_DATA_COLLECT *pData )
{
	if ( pQuest->pQuestDef->nLevelDefDestinations[0] == INVALID_LINK ||
		LevelGetDefinitionIndex( pMessageCreateLevel->pLevel ) == pQuest->pQuestDef->nLevelDefDestinations[0] )
	{
		// the level is being (re)created, reset all the quest data
		QuestCollectDataInit( pQuest, pData );
	}
	return QMR_OK;		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageHandler(
	QUEST *pQuest,
	QUEST_MESSAGE_TYPE eMessageType,
	const void *pMessage)
{
	switch (eMessageType)
	{
		//----------------------------------------------------------------------------
		case QM_SERVER_INTERACT:
		{
			const QUEST_MESSAGE_INTERACT *pInteractMessage = (const QUEST_MESSAGE_INTERACT *)pMessage;
			return s_sQuestMessageNPCInteract( pQuest, pInteractMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_TALK_STOPPED:
		{
			const QUEST_MESSAGE_TALK_STOPPED *pTalkStoppedMessage = (const QUEST_MESSAGE_TALK_STOPPED *)pMessage;
			return s_sQuestMessageTalkStopped( pQuest, pTalkStoppedMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_CREATE_LEVEL:
		{
			const QUEST_MESSAGE_CREATE_LEVEL * pMessageCreateLevel = ( const QUEST_MESSAGE_CREATE_LEVEL * )pMessage;
			return s_QuestCollectMessageCreateLevel( pQuest, pMessageCreateLevel, sQuestDataGet( pQuest ) );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_ENTER_LEVEL:
		{
			const QUEST_MESSAGE_ENTER_LEVEL * pMessageEnterLevel = ( const QUEST_MESSAGE_ENTER_LEVEL * )pMessage;
			if ( QuestStateGetValue( pQuest, QUEST_STATE_COLLECTTEMPLATE_COLLECT ) == QUEST_STATE_ACTIVE )
				return s_QuestCollectMessageEnterLevel( 
							UnitGetGame( QuestGetPlayer( pQuest ) ), 
							pQuest, 
							pMessageEnterLevel, 
							sQuestDataGet( pQuest ) );

			return QMR_OK;
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_ROOMS_ACTIVATED:
		{
			// "message data" is in the quest globals
			// check if a spawn room was activated, spawn baddies
			if ( QuestStateGetValue( pQuest, QUEST_STATE_COLLECTTEMPLATE_COLLECT ) == QUEST_STATE_ACTIVE )
				return s_QuestCollectMessageRoomsActivated( 
							UnitGetGame( QuestGetPlayer( pQuest ) ), 
							pQuest,
							sQuestDataGet( pQuest ) );
			return QMR_OK;
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_PARTY_KILL:
		{
			const QUEST_MESSAGE_PARTY_KILL *pPartyKillMessage = (const QUEST_MESSAGE_PARTY_KILL *)pMessage;
			if ( QuestStateGetValue( pQuest, QUEST_STATE_COLLECTTEMPLATE_COLLECT ) == QUEST_STATE_ACTIVE )
				return s_QuestCollectMessagePartyKill( pQuest, pPartyKillMessage, sQuestDataGet( pQuest ) );
			return QMR_OK;
		}

		//----------------------------------------------------------------------------
		case QM_CLIENT_ATTEMPTING_PICKUP:
		case QM_SERVER_ATTEMPTING_PICKUP: // check client and server, because of latency issues
		{
			const QUEST_MESSAGE_ATTEMPTING_PICKUP *pMessagePickup = (const QUEST_MESSAGE_ATTEMPTING_PICKUP*)pMessage;
			// if the item was spawned for this quest, but the collection state is not active, disallow pickup
			if ( pMessagePickup->nQuestIndex == pQuest->nQuest &&
				 QuestStateGetValue( pQuest, QUEST_STATE_COLLECTTEMPLATE_COLLECT ) != QUEST_STATE_ACTIVE )
			{
				return QMR_PICKUP_FAIL;
			}

			return QMR_OK;
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_PICKUP:
		{
			const QUEST_MESSAGE_PICKUP *pMessagePickup = (const QUEST_MESSAGE_PICKUP*)pMessage;
			return s_sQuestCollectMessagePickup( 
						UnitGetGame( QuestGetPlayer( pQuest ) ), 
						pQuest, 
						pMessagePickup);
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_DROPPED_ITEM:
		{
			const QUEST_MESSAGE_DROPPED_ITEM *pMessageDropping = (const QUEST_MESSAGE_DROPPED_ITEM*)pMessage;
			return s_sQuestCollectMessageDroppingItem( 
						UnitGetGame( QuestGetPlayer( pQuest ) ), 
						pQuest, 
						pMessageDropping);
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_QUEST_STATUS:
			if ( !QuestIsActive( pQuest ) && !QuestIsOffering( pQuest ) )
			{
				UNIT *pPlayer = QuestGetPlayer( pQuest );
				UnitClearStat( pPlayer, STATS_QUEST_NUM_COLLECTED_ITEMS, StatParam( STATS_QUEST_NUM_COLLECTED_ITEMS, pQuest->nQuest, 0, UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT ) ) );
			}
			// fall through

		case QM_CLIENT_QUEST_STATUS:
		{
			const QUEST_MESSAGE_STATUS * pMessageStatus = ( const QUEST_MESSAGE_STATUS * )pMessage;
			return QuestTemplateMessageStatus( pQuest, pMessageStatus );
		}
	}

	return QMR_OK;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestCollectFree(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	
	// free quest data
	ASSERTX_RETURN( pQuest->pQuestData, "Expected quest data" );
	GFREE( UnitGetGame( pQuest->pPlayer ), pQuest->pQuestData );
	pQuest->pQuestData = NULL;	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestCollectInit(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	
	
	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		

	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_COLLECT * pQuestData = ( QUEST_DATA_COLLECT * )GMALLOCZ( pGame, sizeof( QUEST_DATA_COLLECT ) );
	pQuest->pQuestData = pQuestData;
	QuestCollectDataInit( pQuest, pQuestData );

	// chance to drop quest items on kills by party members
	SETBIT( pQuest->dwQuestFlags, QF_PARTY_KILL );

#if ISVERSION(DEVELOPMENT)
	const QUEST_DEFINITION *pQuestDef = pQuest->pQuestDef;
	// sanity check data
	ASSERTV_RETURN( pQuestDef->nCollectItem != INVALID_LINK, "quest \"%s\": the \"Collect Item\" in quest.xls is not a valid item", pQuestDef->szName );
	ASSERTV_RETURN( pQuestDef->fCollectDropRate >= 0.0f, "quest \"%s\": the \"Collect Drop Rate\" in quest.xls must be positive or zero, it is %.3f", pQuestDef->szName, pQuestDef->fCollectDropRate );
	ASSERTV_RETURN( pQuestDef->nObjectiveCount > 0, "quest \"%s\": the \"Objective Count\" in quest.xls must be larger than zero, it is %i", pQuestDef->szName, pQuestDef->nObjectiveCount );
	if (pQuestDef->fCollectDropRate > 0.0f)
	{
		ASSERTV_RETURN( FLOAT_TO_INT_CEIL(float(pQuestDef->nObjectiveCount )/pQuestDef->fCollectDropRate) < MAX_QUEST_COLLECT_MONSTER_SPAWN, "quest \"%s\": \"Objective Count\" divided by \"Collect Spawn Rate\" is too large! please increase \"Objective Count\" and/or increase \"Collect Spawn Rate\"", pQuestDef->szName );
	}
	const UNIT_DATA *pCollectItemData = UnitGetData( pGame, GENUS_ITEM, pQuestDef->nCollectItem );
	ASSERTV_RETURN( pCollectItemData, "quest \"%s\": the \"Collect Item\" in quest.xls has the wrong version_package", pQuestDef->szName );
	ASSERTV_RETURN( UnitDataTestFlag( pCollectItemData, UNIT_DATA_FLAG_INFORM_QUESTS_ON_PICKUP ), "quest item \"%s\" must have InformQuestsOnPickup in items.xls set to 1", pCollectItemData->szName );
	ASSERTV_RETURN( UnitDataTestFlag( pCollectItemData, UNIT_DATA_FLAG_NO_TRADE), "quest item \"%s\" must have \"no trade\" in items.xls set to 1", pCollectItemData->szName );
	ASSERTV_RETURN( !UnitDataTestFlag( pCollectItemData, UNIT_DATA_FLAG_NO_DROP ), "quest item \"%s\" must not have NoDrop in items.xls set (all side quest items should be droppable)", pCollectItemData->szName );
	ASSERTV( pQuestDef->nObjectiveCount <= MAX_QUEST_COLLECT_OBJECTIVE_COUNT, "quest \"%s\": Objective Count is %i, code should be updated, since it expects it to be less than %i", pQuestDef->szName, pQuestDef->nObjectiveCount, MAX_QUEST_COLLECT_OBJECTIVE_COUNT );
	
	// this doesn't seem to be used ASSERTV_RETURN( UnitDataTestFlag( pCollectItemData, UNIT_DATA_FLAG_ASK_QUESTS_FOR_PICKUP ), "quest item \"%s\" must have AskQuestsForPickup in items.xls set to 1", pCollectItemData->szName );
#endif //ISVERSION(DEVELOPMENT)
}
