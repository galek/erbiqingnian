//----------------------------------------------------------------------------
// FILE: quest_operate.cpp
//
// The operate quest is a template quest, in which the player must interact
// with a number of objects of a specific class. A level can also be specified.
// Optionally, the objects can be spawned by the quest.
// 
// author: Chris March, 2-8-2007
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "quest_operate.h"
#include "quests.h"
#include "s_quests.h"
#include "npc.h"
#include "states.h"
#include "objects.h"
#include "room_layout.h"

#define STATS_QUEST_OPERATED_OBJ_UNIT_ID_LAST	STATS_QUEST_OPERATED_OBJ7_UNIT_ID
#define QUESTOPERATE_MAX_OPERATE_OBJECT_COUNT	(STATS_QUEST_OPERATED_OBJ_UNIT_ID_LAST - STATS_QUEST_OPERATED_OBJ0_UNIT_ID + 1)

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sQuestOperatedUnitIDStatsReset(
	QUEST *pQuest)
{
	UNIT *pPlayer = QuestGetPlayer( pQuest );
	int nDifficulty = UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT );
	for (int i=STATS_QUEST_OPERATED_OBJ0_UNIT_ID; 
		 i <= STATS_QUEST_OPERATED_OBJ_UNIT_ID_LAST; 
		 ++i)
		 UnitSetStat( pPlayer, i, pQuest->nQuest, nDifficulty, INVALID_ID );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT s_sQuestMessageEnterLevel( 
	GAME* pGame,
	QUEST* pQuest,
	const QUEST_MESSAGE_ENTER_LEVEL *pMessageEnterLevel )
{
#if !ISVERSION( CLIENT_ONLY_VERSION )

	if ( QuestStateGetValue( pQuest, QUEST_STATE_OPERATETEMPLATE_OPERATE_OBJS ) == 
			QUEST_STATE_ACTIVE )
	{
		if ( s_QuestAttemptToSpawnMissingObjectiveObjects( pQuest, UnitGetLevel( QuestGetPlayer( pQuest ) ) ) )
		{
			// reset partial completion on level entry, to support multiple operate 
			// objects of the same class, which are only unique by their UNITIDs,
			// which do not persist across runs of the game
			sQuestOperatedUnitIDStatsReset( pQuest );
		}
	}

#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )
	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestOperateComplete(
	const QUEST_FUNCTION_PARAM &tParam)
{
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageInteract(
	QUEST *pQuest,
	const QUEST_MESSAGE_INTERACT *pMessage )
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
		if ( QuestStateGetValue( pQuest, QUEST_STATE_OPERATETEMPLATE_OPERATE_OBJS ) == QUEST_STATE_COMPLETE )
		{
			if ( s_QuestIsRewarder( pQuest, pSubject ) )
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
			if ( s_QuestIsRewarder( pQuest, pSubject ) )
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
static QUEST_MESSAGE_RESULT sQuestMessageTalkStopped(
	QUEST *pQuest,
	const QUEST_MESSAGE_TALK_STOPPED *pMessage )
{
	int nDialogStopped = pMessage->nDialog;
	const QUEST_DEFINITION *pQuestDefinition = QuestGetDefinition( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pTalkingTo = UnitGetById( pGame, pMessage->idTalkingTo );
	UNIT *pPlayer = QuestGetPlayer( pQuest );

	if ( nDialogStopped != INVALID_LINK )
	{
		if ( nDialogStopped == pQuestDefinition->nDescriptionDialog )
		{
			if (QuestIsInactive( pQuest ) &&
				s_QuestIsGiver( pQuest, pTalkingTo ) &&
				s_QuestCanBeActivated( pQuest ))
			{
				// activate quest
				if ( s_QuestActivate( pQuest ) )
				{
					// activate all quest states for operating objects
					QuestStateSet( pQuest, QUEST_STATE_OPERATETEMPLATE_OPERATE_OBJS, QUEST_STATE_ACTIVE );

					s_QuestAttemptToSpawnMissingObjectiveObjects( pQuest, UnitGetLevel( pPlayer ));
				}
			}
		}
		else if ( nDialogStopped == pQuestDefinition->nCompleteDialog )
		{
			if ( QuestStateGetValue( pQuest, QUEST_STATE_OPERATETEMPLATE_RETURN ) == QUEST_STATE_ACTIVE &&
				 s_QuestIsRewarder( pQuest, pTalkingTo ) )
			{	
				// complete quest
				QuestStateSet( pQuest, QUEST_STATE_OPERATETEMPLATE_RETURN, QUEST_STATE_COMPLETE );

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

static BOOL sAlreadyOperated( QUEST *pQuest, UNIT *pObject )
{
	UNIT *pPlayer = QuestGetPlayer( pQuest );
	UNITID idObject = UnitGetId( pObject );
	for (int i = STATS_QUEST_OPERATED_OBJ0_UNIT_ID; 
		 i <= STATS_QUEST_OPERATED_OBJ_UNIT_ID_LAST; 
		 ++i)
		if ( ( (UNITID) UnitGetStat( pPlayer, i, pQuest->nQuest, UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT ) ) ) == idObject )
			return TRUE;

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageCanOperateObject( 
	QUEST *pQuest,
	const QUEST_MESSAGE_CAN_OPERATE_OBJECT *pMessage)
{
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pObject = UnitGetById( pGame, pMessage->idObject );
	UNIT *pPlayer = QuestGetPlayer( pQuest );
	const QUEST_DEFINITION *pQuestDef = pQuest->pQuestDef;

	if ( ( pQuestDef->nLevelDefDestinations[0] == INVALID_LINK || 
		   pQuestDef->nLevelDefDestinations[0] == UnitGetLevelDefinitionIndex( pObject ) ) &&
		UnitIsObjectClass( pObject, pQuestDef->nObjectiveObject ) )
	{		
		int nObjectsOperated = UnitGetStat( pPlayer, STATS_QUEST_OPERATED_OBJS, pQuest->nQuest, UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT ));
		if ( QuestStateGetValue( pQuest, QUEST_STATE_OPERATETEMPLATE_OPERATE_OBJS ) == QUEST_STATE_ACTIVE &&
			nObjectsOperated < pQuestDef->nObjectiveCount &&
			!sAlreadyOperated( pQuest, pObject ) )
		{
			return QMR_OPERATE_OK;
		}
		else
		{
			// quest isn't in a state that allows this quest object to be operated
			return QMR_OPERATE_FORBIDDEN;
		}
	}
	return QMR_OK;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestObjectOperated(
	QUEST *pQuest,
	const QUEST_MESSAGE_OBJECT_OPERATED *pMessage )
{
	UNIT *pObject = pMessage->pTarget;
	UNIT *pPlayer = pMessage->pOperator;
	const QUEST_DEFINITION *pQuestDef = pQuest->pQuestDef;
	
	if ( ( pQuestDef->nLevelDefDestinations[0] == INVALID_LINK || 
		   pQuestDef->nLevelDefDestinations[0] == 
				UnitGetLevelDefinitionIndex( pObject ) ) &&
		 UnitGetClass( pObject ) == pQuestDef->nObjectiveObject )
	{		
		int nObjectsOperated = UnitGetStat( pPlayer, STATS_QUEST_OPERATED_OBJS, pQuest->nQuest, UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT ));
		if ( QuestStateGetValue( pQuest, QUEST_STATE_OPERATETEMPLATE_OPERATE_OBJS ) == QUEST_STATE_ACTIVE &&
			 nObjectsOperated < pQuestDef->nObjectiveCount &&
			 !sAlreadyOperated( pQuest, pObject ) )
		{
			UnitSetStat( pPlayer, STATS_QUEST_OPERATED_OBJS, pQuest->nQuest, UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT ), nObjectsOperated+1 );
			if ( nObjectsOperated == 0 )
				QuestAutoTrack( pQuest );

			ASSERTX_RETVAL( nObjectsOperated + STATS_QUEST_OPERATED_OBJ0_UNIT_ID <= STATS_QUEST_OPERATED_OBJ_UNIT_ID_LAST, QMR_OPERATE_FORBIDDEN, "increase number of stats for operate object server to client transmission");
			UnitSetStat( pPlayer, nObjectsOperated + STATS_QUEST_OPERATED_OBJ0_UNIT_ID, pQuest->nQuest, UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT ), UnitGetId( pObject ) );
			s_StateSet( pObject, pObject, STATE_OPERATE_OBJECT_FX, 0 );

			if ( nObjectsOperated+1 >= pQuestDef->nObjectiveCount)
			{
				QuestStateSet( pQuest, QUEST_STATE_OPERATETEMPLATE_OPERATE_OBJS, QUEST_STATE_COMPLETE );
				QuestStateSet( pQuest, QUEST_STATE_OPERATETEMPLATE_RETURN, QUEST_STATE_ACTIVE );
			}
			
			return QMR_OPERATE_OK;
		}
		else
		{
			// quest isn't in a state that allows this quest object to be operated
			return QMR_OPERATE_FORBIDDEN;
		}
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
			return sQuestMessageInteract( pQuest, pInteractMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_TALK_STOPPED:
		{
			const QUEST_MESSAGE_TALK_STOPPED *pTalkStoppedMessage = (const QUEST_MESSAGE_TALK_STOPPED *)pMessage;
			return sQuestMessageTalkStopped( pQuest, pTalkStoppedMessage );
		}

		//----------------------------------------------------------------------------
		case QM_CAN_OPERATE_OBJECT:
		{
			const QUEST_MESSAGE_CAN_OPERATE_OBJECT *pCanObjectiveObjectMessage = (const QUEST_MESSAGE_CAN_OPERATE_OBJECT *)pMessage;
			return sQuestMessageCanOperateObject( pQuest, pCanObjectiveObjectMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_OBJECT_OPERATED:
		{
			const QUEST_MESSAGE_OBJECT_OPERATED *pObjectOperatedMessage = (const QUEST_MESSAGE_OBJECT_OPERATED *)pMessage;
			 return sQuestObjectOperated( pQuest, pObjectOperatedMessage );
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
		case QM_SERVER_QUEST_STATUS:
			if ( !QuestIsActive( pQuest ) && !QuestIsOffering( pQuest ) )
			{
				UNIT *pPlayer = QuestGetPlayer( pQuest );
				UnitClearStat( pPlayer, STATS_QUEST_OPERATED_OBJS, StatParam( STATS_QUEST_OPERATED_OBJS, pQuest->nQuest, UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT ) ) );
				sQuestOperatedUnitIDStatsReset( pQuest );
			}
			return QMR_OK;
	}

	return QMR_OK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestOperateFree(
	const QUEST_FUNCTION_PARAM &tParam)
{
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestOperateInit(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	

	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		

	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );

	sQuestOperatedUnitIDStatsReset( pQuest );
	ASSERTX_RETURN( pQuest->pQuestDef->nObjectiveCount <= QUESTOPERATE_MAX_OPERATE_OBJECT_COUNT, "Objective Count is too large for operate template quest, code needs to be updated to support this amount");

#if ISVERSION(DEVELOPMENT)
	const QUEST_DEFINITION *pQuestDef = pQuest->pQuestDef;
	// sanity check data
	ASSERTV_RETURN( pQuestDef->nObjectiveObject != INVALID_LINK, "quest \"%s\": the \"ObjectiveObject\" in quest.xls is not a valid object", pQuestDef->szName );
	const UNIT_DATA *pObjectiveObjectData = UnitGetData( UnitGetGame( QuestGetPlayer( pQuest ) ), GENUS_OBJECT, pQuestDef->nObjectiveObject );
	ASSERTV_RETURN( pObjectiveObjectData, "quest \"%s\": the \"ObjectiveObject\" in quest.xls has the wrong version_package", pQuestDef->szName );
	ASSERTV_RETURN( pObjectiveObjectData->fCollisionRadius <= 2.f, "quest object \"%s\" must have a CollisionRadius in objects.xls set to 2 or less, or it may fail to fit in a random location.", pObjectiveObjectData->szName );
	ASSERTV_RETURN( UnitDataTestFlag( pObjectiveObjectData, UNIT_DATA_FLAG_ASK_QUESTS_FOR_OPERATE ), "quest object \"%s\" must have AskQuestsForOperate in objects.xls set to 1", pObjectiveObjectData->szName );
	// this doesn't seem to be used ASSERTV_RETURN( UnitDataTestFlag( pCollectItemData, UNIT_DATA_FLAG_ASK_QUESTS_FOR_PICKUP ), "quest item \"%s\" must have AskQuestsForPickup in items.xls set to 1", pCollectItemData->szName );
#endif //ISVERSION(DEVELOPMENT)

}
