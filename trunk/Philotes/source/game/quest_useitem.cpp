//----------------------------------------------------------------------------
// FILE: quest_talk.h
//
// This is a template quest. The player first talks to "Cast Giver" (in quest.xls),
// who gives them quest items from "Starting Items Treasure Class" to use 
// (right-click) on "Objective Monster" or "Objective Object". The player must
// use the item within "MonitorApproachRadius" of the monster or object, unless
// it was not specified. When they use the item, the "Mostly Complete Function" 
// is called, if specified, to do cool stuff.
// Finally, the player must go speak with Cast Rewarder. 
// 
// author: Chris March, 3-7-2007
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "quest_useitem.h"
#include "quests.h"
#include "s_quests.h"
#include "npc.h"

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct QUEST_DATA_USEITEM
{
	UNITID idApproachedUnit;
};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_USEITEM * sQuestDataGet( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return (QUEST_DATA_USEITEM *)pQuest->pQuestData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sQuestDataInit(
	QUEST *pQuest,
	QUEST_DATA_USEITEM *pQuestData)
{
	pQuestData->idApproachedUnit = INVALID_ID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageInteract( 
	QUEST *pQuest,
	const QUEST_MESSAGE_INTERACT *pMessage)
{
#if !ISVERSION( CLIENT_ONLY_VERSION )
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pPlayer = UnitGetById( pGame, pMessage->idPlayer );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );
	ASSERTX_RETVAL( pPlayer, QMR_OK, "Invalid player" );
	ASSERTX_RETVAL( pSubject, QMR_OK, "Invalid NPC" );	
	const QUEST_DEFINITION *pQuestDefinition = QuestGetDefinition( pQuest );

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
		if ( QuestStateGetValue( pQuest, QUEST_STATE_USEITEMTEMPLATE_RETURN ) == QUEST_STATE_ACTIVE )
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
				// Incomplete: tell the player to go get it done
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
static float sGetMonitorApproachRadius( QUEST *pQuest )
{
	// check the level for thing to use item on
	GAME * pGame = UnitGetGame( QuestGetPlayer( pQuest ) );
	const QUEST_DEFINITION * pQuestDef = pQuest->pQuestDef;
	if ( pQuestDef->nObjectiveObject != INVALID_LINK )
	{
		const UNIT_DATA * pUnitToUseNearData = 
			UnitGetData(pGame, GENUS_OBJECT, pQuestDef->nObjectiveObject );

		ASSERTV_RETZERO( pUnitToUseNearData, "quest \"%s\": Objective Object is in the wrong version_package", pQuestDef->szName );
		return pUnitToUseNearData->flMonitorApproachRadius;	
	}
	else
	{
		ASSERTV_RETVAL( pQuestDef->nObjectiveMonster != INVALID_LINK, QMR_OK, "quest \"%s\": valid Objective Object or Objective Monster must be specified for UseItem template quest", pQuestDef->szName );
		const UNIT_DATA * pUnitToUseNearData = 
			UnitGetData(pGame, GENUS_MONSTER, pQuestDef->nObjectiveMonster );

		ASSERTV_RETZERO( pUnitToUseNearData, "quest \"%s\": Objective Monster is in the wrong version_package", pQuestDef->szName );
		return pUnitToUseNearData->flMonitorApproachRadius;	
	}
}

static void sAttemptToSpawnMissingUnits( QUEST * pQuest, LEVEL *pLevel )
{
	const QUEST_DEFINITION *pQuestDef = pQuest->pQuestDef;
	if ( !pQuestDef->bDisableSpawning &&
		 ( pQuest->pQuestDef->nLevelDefDestinations[0] == INVALID_LINK ||
		   pQuest->pQuestDef->nLevelDefDestinations[0] == LevelGetDefinitionIndex( pLevel ) ) &&
		!s_QuestAttemptToSpawnMissingObjectiveObjects( pQuest, pLevel ) &&
		 pQuestDef->nObjectiveMonster != INVALID_LINK &&
		 !LevelCountMonsters( pLevel, pQuestDef->nObjectiveMonster ) )
	{
		ASSERT_RETURN(pQuestDef->nObjectiveMonster != INVALID_LINK);
		const UNIT_DATA * pMonsterData = 
			UnitGetData(UnitGetGame(QuestGetPlayer(pQuest)), GENUS_MONSTER, pQuestDef->nObjectiveMonster);
		ASSERTV_RETURN( pMonsterData != NULL, "%s quest: NULL UNIT_DATA for Objective Monster", pQuestDef->szName );
		int nMonsterQuality = pMonsterData->nMonsterQualityRequired;
		
		s_LevelSpawnMonsters( 
			QuestGetGame( pQuest ), 
			pLevel, 
			pQuestDef->nObjectiveMonster, 
			1,
			nMonsterQuality);
	}
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageTalkStopped(
	QUEST *pQuest,
	const QUEST_MESSAGE_TALK_STOPPED *pMessage )
{
	int nDialogStopped = pMessage->nDialog;
	const QUEST_DEFINITION *pQuestDefinition = QuestGetDefinition( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pTalkingTo = UnitGetById( pGame, pMessage->idTalkingTo );

	if ( nDialogStopped != INVALID_LINK )
	{
		if ( nDialogStopped == pQuestDefinition->nDescriptionDialog )
		{
			if ( s_QuestIsGiver( pQuest, pTalkingTo ) &&
				 s_QuestCanBeActivated( pQuest ) )
			{
				// activate quest
				if ( s_QuestActivate( pQuest ) )
				{
					if ( pQuestDefinition->nLevelDefDestinations[0] != INVALID_LINK &&
						 UnitGetLevelDefinitionIndex( pTalkingTo ) != pQuestDefinition->nLevelDefDestinations[0] )
					{
						QuestStateSet( pQuest, QUEST_STATE_USEITEMTEMPLATE_TRAVEL, QUEST_STATE_ACTIVE );
					}
					else
					{
						QuestStateSet( pQuest, QUEST_STATE_USEITEMTEMPLATE_TRAVEL, QUEST_STATE_COMPLETE );

						if ( sGetMonitorApproachRadius( pQuest ) > 0.0f )
						{
							QuestStateSet( pQuest, QUEST_STATE_USEITEMTEMPLATE_FIND, QUEST_STATE_ACTIVE );
						}
						else
						{
							QuestStateSet( pQuest, QUEST_STATE_USEITEMTEMPLATE_FIND, QUEST_STATE_COMPLETE );
							QuestStateSet( pQuest, QUEST_STATE_USEITEMTEMPLATE_USEITEM, QUEST_STATE_ACTIVE );
						}
						sAttemptToSpawnMissingUnits( pQuest, UnitGetLevel( QuestGetPlayer( pQuest ) ) );

					}
				}
			}
		}
		else if ( nDialogStopped == pQuestDefinition->nCompleteDialog )
		{
			if ( QuestStateGetValue( pQuest, QUEST_STATE_USEITEMTEMPLATE_RETURN ) == QUEST_STATE_ACTIVE &&
				 s_QuestIsRewarder( pQuest, pTalkingTo ) )
			{	
				// complete quest
				QuestStateSet( pQuest, QUEST_STATE_USEITEMTEMPLATE_RETURN, QUEST_STATE_COMPLETE );

				if ( s_QuestComplete( pQuest ) )
				{								
					// offer reward
					if ( s_QuestOfferReward( pQuest, pTalkingTo, FALSE ) )
					{					
						return QMR_OFFERING;
					}
				}					
			}
		}
	}

	return QMR_OK;

}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessagePlayerApproach(
	QUEST *pQuest,
	const QUEST_MESSAGE_PLAYER_APPROACH *pMessage )
{

	if ( QuestStateGetValue( pQuest, QUEST_STATE_USEITEMTEMPLATE_FIND ) == QUEST_STATE_ACTIVE )
	{
		const QUEST_DEFINITION* pQuestDef = pQuest->pQuestDef;
		UNIT *pTarget = UnitGetById( QuestGetGame( pQuest ), pMessage->idTarget );

		if (UnitIsGenusClass( pTarget, GENUS_OBJECT, pQuestDef->nObjectiveObject ) ||
			UnitIsMonsterClass( pTarget, pQuestDef->nObjectiveMonster ))
		{
			QuestStateSet( pQuest, QUEST_STATE_USEITEMTEMPLATE_FIND, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_USEITEMTEMPLATE_USEITEM, QUEST_STATE_ACTIVE );
			QUEST_DATA_USEITEM * pQuestData = sQuestDataGet( pQuest );
			pQuestData->idApproachedUnit = pMessage->idTarget;
		}
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageCanUseItem(
	QUEST *pQuest,
	const QUEST_MESSAGE_CAN_USE_ITEM * pMessage)
{
	if ( QuestStateGetValue( pQuest, QUEST_STATE_USEITEMTEMPLATE_USEITEM ) == QUEST_STATE_ACTIVE )
	{
		UNIT *pPlayer = QuestGetPlayer( pQuest );
		GAME * pGame = UnitGetGame( pPlayer );		
		UNIT *pItem = UnitGetById( pGame, pMessage->idItem );
		if ( pItem )
		{
			int nDifficulty = UnitGetStat(pPlayer, STATS_DIFFICULTY_CURRENT);
			if (  UnitGetStat( pItem, STATS_QUEST_STARTING_ITEM, nDifficulty ) == pQuest->nQuest )
			{
				return QMR_USE_OK;
			}
		}
		return QMR_USE_FAIL;
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageUseItem(
	QUEST *pQuest,
	const QUEST_MESSAGE_USE_ITEM * pMessage)
{
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pItem = UnitGetById( pGame, pMessage->idItem );
	if (pItem)
	{
		UNIT* player = QuestGetPlayer(pQuest);
		ASSERTX(player, "NULL player for quest on MessageUseItem");
		int nDifficulty = UnitGetStat(player, STATS_DIFFICULTY_CURRENT);
		if (  UnitGetStat( pItem, STATS_QUEST_STARTING_ITEM, nDifficulty ) == pQuest->nQuest )
		{
			QuestStateSet( pQuest, QUEST_STATE_USEITEMTEMPLATE_USEITEM, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_USEITEMTEMPLATE_RETURN, QUEST_STATE_ACTIVE );
			return QMR_USE_OK;
		}
	}

	return QMR_OK;			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT s_sQuestMessageEnterLevel( 
	GAME* pGame,
	QUEST* pQuest,
	const QUEST_MESSAGE_ENTER_LEVEL *pMessageEnterLevel )
{
#if !ISVERSION( CLIENT_ONLY_VERSION )

	if ( pQuest->pQuestDef->nLevelDefDestinations[0] != INVALID_LINK )
	{
		if ( pQuest->pQuestDef->nLevelDefDestinations[0] == pMessageEnterLevel->nLevelNewDef )
		{		
			// arrived at the level destination
			if ( QuestStateGetValue( pQuest, QUEST_STATE_USEITEMTEMPLATE_TRAVEL ) == 
				 QUEST_STATE_ACTIVE )
			{
				QuestStateSet( pQuest, QUEST_STATE_USEITEMTEMPLATE_TRAVEL, QUEST_STATE_COMPLETE );
				if ( sGetMonitorApproachRadius( pQuest ) > 0.0f )
				{
					QuestStateSet( pQuest, QUEST_STATE_USEITEMTEMPLATE_FIND, QUEST_STATE_ACTIVE );
				}
				else
				{
					QuestStateSet( pQuest, QUEST_STATE_USEITEMTEMPLATE_FIND, QUEST_STATE_COMPLETE );
					QuestStateSet( pQuest, QUEST_STATE_USEITEMTEMPLATE_USEITEM, QUEST_STATE_ACTIVE );
				}
				sAttemptToSpawnMissingUnits( pQuest, UnitGetLevel( QuestGetPlayer( pQuest ) ) );
			}
		}
		else
		{
			// arrived at a level that is not the destination
			if ( QuestStateGetValue( pQuest, QUEST_STATE_USEITEMTEMPLATE_FIND ) == QUEST_STATE_ACTIVE ||
				 QuestStateGetValue( pQuest, QUEST_STATE_USEITEMTEMPLATE_USEITEM ) == QUEST_STATE_ACTIVE )
			{
				// reset to hidden to hack around the "complete can only be set to hidden, not active" limitation
				QuestStateSet( pQuest, QUEST_STATE_USEITEMTEMPLATE_TRAVEL, QUEST_STATE_HIDDEN );			
				QuestStateSet( pQuest, QUEST_STATE_USEITEMTEMPLATE_FIND, QUEST_STATE_HIDDEN );
				QuestStateSet( pQuest, QUEST_STATE_USEITEMTEMPLATE_USEITEM, QUEST_STATE_HIDDEN );

				QuestStateSet( pQuest, QUEST_STATE_USEITEMTEMPLATE_TRAVEL, QUEST_STATE_ACTIVE );			
			}
		}
	}

#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )
	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT s_sQuestMessageEnterRoom( 
	QUEST* pQuest )
{
#if !ISVERSION( CLIENT_ONLY_VERSION )

	UNIT *pPlayer = QuestGetPlayer( pQuest );
	if (UnitIsInLimbo( pPlayer ))
		return QMR_OK;

	// check if the object or monster to use the item on is still in 
	// range, after it has been approached
	
	//reset the unit id in the data, and then set only if still within range
	QUEST_DATA_USEITEM *pQuestData = sQuestDataGet( pQuest );
	if ( pQuestData->idApproachedUnit != INVALID_ID )
	{
		UNIT *pApproachedUnit = 
			UnitGetById ( UnitGetGame( pPlayer ), pQuestData->idApproachedUnit );
		pQuestData->idApproachedUnit = INVALID_ID;
		if ( pApproachedUnit )
		{
			const UNIT_DATA *pUnitData = UnitGetData( pApproachedUnit );
			if ( pUnitData )
			{
				float use_dist = pUnitData->flMonitorApproachRadius;
				if ( use_dist == 0.0f || 
					VectorDistanceSquaredXY( pPlayer->vPosition, pApproachedUnit->vPosition ) <= ( use_dist * use_dist ) )
				{
					pQuestData->idApproachedUnit = UnitGetId( pApproachedUnit );
				}
			}
		}
	}

	if ( pQuestData->idApproachedUnit == INVALID_ID &&
		 QuestStateGetValue( pQuest, QUEST_STATE_USEITEMTEMPLATE_USEITEM ) == QUEST_STATE_ACTIVE )
	{
		// player went out of range from target
		QuestStateSet( pQuest, QUEST_STATE_USEITEMTEMPLATE_USEITEM, QUEST_STATE_HIDDEN );
		QuestStateSet( pQuest, QUEST_STATE_USEITEMTEMPLATE_FIND, QUEST_STATE_HIDDEN );
		QuestStateSet( pQuest, QUEST_STATE_USEITEMTEMPLATE_FIND, QUEST_STATE_ACTIVE );
	}

#endif //!ISVERSION( CLIENT_ONLY_VERSION )

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
			return sQuestMessageInteract( pQuest, pInteractMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_TALK_STOPPED:
		{
			const QUEST_MESSAGE_TALK_STOPPED *pTalkStoppedMessage = (const QUEST_MESSAGE_TALK_STOPPED *)pMessage;
			return sQuestMessageTalkStopped( pQuest, pTalkStoppedMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_PLAYER_APPROACH:
		{
			const QUEST_MESSAGE_PLAYER_APPROACH *pPlayerApproachMessage = (const QUEST_MESSAGE_PLAYER_APPROACH *)pMessage;
			return sQuestMessagePlayerApproach( pQuest, pPlayerApproachMessage );
		}

		//----------------------------------------------------------------------------
		case QM_CAN_USE_ITEM:
		{
			const QUEST_MESSAGE_CAN_USE_ITEM *pCanUseItemMessage = ( const QUEST_MESSAGE_CAN_USE_ITEM * )pMessage;
			return sQuestMessageCanUseItem( pQuest, pCanUseItemMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_USE_ITEM:
		{
			const QUEST_MESSAGE_USE_ITEM *pUseItemMessage = ( const QUEST_MESSAGE_USE_ITEM * )pMessage;
			return sQuestMessageUseItem( pQuest, pUseItemMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_ENTER_LEVEL:
		{
			const QUEST_MESSAGE_ENTER_LEVEL * pMessageEnterLevel = ( const QUEST_MESSAGE_ENTER_LEVEL * )pMessage;
			return s_sQuestMessageEnterLevel( 
				UnitGetGame( QuestGetPlayer( pQuest ) ), 
				pQuest, 
				pMessageEnterLevel );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_ENTER_ROOM:
		{
			return s_sQuestMessageEnterRoom( pQuest );
		}
		break;

		//----------------------------------------------------------------------------
		case QM_CLIENT_MONSTER_INIT:
		{
			QUEST_MESSAGE_MONSTER_INIT *pMessageMonsterInit = (QUEST_MESSAGE_MONSTER_INIT *)pMessage;
			UNIT *pMonster = pMessageMonsterInit->pMonster;
			if ( QuestIsActive( pQuest ) &&
				 pQuest->pQuestDef->nLevelDefDestinations[0] == UnitGetLevelDefinitionIndex( pMonster ) &&
				 UnitGetClass( pMonster ) == pQuest->pQuestDef->nObjectiveMonster )
			{
				UnitSetFlag( pMonster, UNITFLAG_SHOW_LABEL_WHEN_DEAD, TRUE );
			}
			return QMR_OK;
		}
		break;

	}

	return QMR_OK;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestFreeUseItem(
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
void QuestInitUseItem(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	
	
	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		
			
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	QUEST_DATA_USEITEM * pQuestData = ( QUEST_DATA_USEITEM * )GMALLOCZ( QuestGetGame( pQuest ), sizeof( QUEST_DATA_USEITEM ) );
	sQuestDataInit( pQuest, pQuestData );
	pQuest->pQuestData = pQuestData;

	// quest message flags
	pQuest->dwQuestMessageFlags |= MAKE_MASK( QSMF_APPROACH_RADIUS );

#if ISVERSION(DEVELOPMENT)
	const QUEST_DEFINITION *pQuestDef = pQuest->pQuestDef;
	// sanity check data
	if (pQuestDef->nObjectiveObject != INVALID_LINK )
	{
		const UNIT_DATA *pObjectiveObjectData = UnitGetData( UnitGetGame( pQuest->pPlayer ), GENUS_OBJECT, pQuestDef->nObjectiveObject);
		ASSERTV_RETURN( pObjectiveObjectData , "quest \"%s\": the \"Objective Object\" in quest.xls has the wrong version_package", pQuestDef->szName );
		ASSERTV_RETURN( UnitDataTestFlag( pObjectiveObjectData , UNIT_DATA_FLAG_MONITOR_PLAYER_APPROACH ), "quest object \"%s\" must have MonitorPlayerApproach in objects.xls set to 1", pObjectiveObjectData->szName );
		ASSERTV_RETURN( pObjectiveObjectData->flMonitorApproachRadius > 0.f, "quest object \"%s\" must have MonitorPlayerRadius in objects.xls set to 7 (example for an object of radius 3)", pObjectiveObjectData->szName );
	}
	else
	{
		ASSERTV_RETURN( pQuestDef->nObjectiveMonster, "quest \"%s\": requiers either an Objective Object or an Objective Monster in quest.xls is not a valid item", pQuestDef->szName );
		const UNIT_DATA *pObjectiveMonsterData = UnitGetData( UnitGetGame( pQuest->pPlayer ), GENUS_MONSTER, pQuestDef->nObjectiveMonster);
		ASSERTV_RETURN( pObjectiveMonsterData , "quest \"%s\": the \"Objective Monster\" in quest.xls has the wrong version_package", pQuestDef->szName );
		ASSERTV_RETURN( !UnitDataTestFlag( pObjectiveMonsterData , UNIT_DATA_FLAG_SPAWN_IN_AIR), "quest monster \"%s\" is not allowed for quest \"%s\", because it is flying (\"in air\" in monsters.xls)", pObjectiveMonsterData->szName, pQuestDef->szName );
		ASSERTV( UnitDataTestFlag( pObjectiveMonsterData , UNIT_DATA_FLAG_MONITOR_PLAYER_APPROACH ), "quest monster \"%s\" must have MonitorPlayerApproach in monsters.xls set to 1", pObjectiveMonsterData->szName );
		ASSERTV( UnitDataTestFlag( pObjectiveMonsterData , UNIT_DATA_FLAG_NEVER_DESTROY_DEAD ), "quest monster \"%s\" must have \"never destroy dead\" in monsters.xls set to 1", pObjectiveMonsterData->szName );
		ASSERTV( UnitDataTestFlag( pObjectiveMonsterData , UNIT_DATA_FLAG_SELECTABLE_DEAD ), "quest monster \"%s\" must have \"selectable dead or dying\" in monsters.xls set to 1", pObjectiveMonsterData->szName );
		ASSERTV( UnitDataTestFlag( pObjectiveMonsterData , UNIT_DATA_FLAG_INFORM_QUESTS_ON_INIT ), "quest monster \"%s\" must have \"Inform quests on init\" in monsters.xls set to 1", pObjectiveMonsterData->szName );
		ASSERTV( pObjectiveMonsterData->flMonitorApproachRadius > 0.f, "quest monster \"%s\" must have MonitorPlayerRadius in monsters.xls set to 7 (example for a monster of radius 3)", pObjectiveMonsterData->szName );
	}

	// this doesn't seem to be used ASSERTV_RETURN( UnitDataTestFlag( pCollectItemData, UNIT_DATA_FLAG_ASK_QUESTS_FOR_PICKUP ), "quest item \"%s\" must have AskQuestsForPickup in items.xls set to 1", pCollectItemData->szName );
#endif //ISVERSION(DEVELOPMENT)

}
