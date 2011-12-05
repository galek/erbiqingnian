//----------------------------------------------------------------------------
// FILE: quest_default.cpp
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "globalindex.h"
#include "level.h"
#include "offer.h"
#include "quest_default.h"
#include "quests.h"
#include "s_quests.h"
#include "unit_priv.h"
#include "Quest.h"
#include "npc.h"
#include "inventory.h"

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT s_sQuestMessageNPCHasInfo( 
	QUEST *pQuest,
	const QUEST_MESSAGE_INTERACT_INFO *pMessage)
{
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );

	// check givers
	if (s_QuestIsGiver( pQuest, pSubject ))
	{
		if (s_QuestCanBeActivated( pQuest ))
		{
			// there is new info available, a new quest				
			return ( pQuest->pQuestDef->eStyle == QS_STORY ) ?
						QMR_NPC_STORY_NEW :
						QMR_NPC_INFO_NEW;
		}
		else if (QuestIsInactive( pQuest ) &&
				 pQuest->pQuestDef->nUnavailableDialog != INVALID_LINK)
		{
			return QMR_NPC_INFO_UNAVAILABLE;
		}
	}

	// check rewarders
	if (s_QuestIsRewarder( pQuest, pSubject ))
	{
		if (QuestIsActive( pQuest ))
		{
			if (s_QuestAllStatesComplete( pQuest, TRUE ))
			{
				// this quest is complete, the npc is awating your return
				return ( pQuest->pQuestDef->eStyle == QS_STORY ) ?
							QMR_NPC_STORY_RETURN :
							QMR_NPC_INFO_RETURN;
			}
			else
			{
				// the quest is active, the npc is waiting for you to do the quest tasks				
				return QMR_NPC_INFO_WAITING;
			}
		}
		else if (QuestIsOffering( pQuest ))
		{
			return QMR_NPC_INFO_WAITING;
		}						
		
	}
	
	return QMR_OK;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT s_sQuestTransitionLevel( 
	QUEST *pQuest,
	const QUEST_MESSAGE_ENTER_LEVEL *pMessage)
{
	const QUEST_DEFINITION *pQuestDef = pQuest->pQuestDef;
	int nLevelDefNew = pMessage->nLevelNewDef;
	
	// auto activate if inactive and going to the level that activates it
	if (QuestGetStatus( pQuest ) ==	QUEST_STATUS_INACTIVE &&
		pQuestDef->nLevelDefAutoActivate == nLevelDefNew)
	{
		if ( s_QuestActivate( pQuest ) )
		{
		}
	}
	
	return QMR_OK;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageSubLevelTransition(
	QUEST *pQuest,
	const QUEST_MESSAGE_SUBLEVEL_TRANSITION *pMessage)
{
	UNIT *pPlayer = QuestGetPlayer( pQuest );
	SUBLEVEL *pSubLevelNew = pMessage->pSubLevelNew;
	SUBLEVEL_TYPE eType = SubLevelGetType( pSubLevelNew );
	const QUEST_DEFINITION *pQuestDef = QuestGetDefinition( pQuest );

	if (pQuestDef->eSubLevelTypeTruthOld != ST_INVALID && eType == pQuestDef->eSubLevelTypeTruthOld)
	{
		int nQuestStateToAdvanceTo = pQuestDef->nQuestStateAdvanceToAtSubLevelTruthOld;
		s_QuestTruthRoomEnter( pPlayer, pSubLevelNew, pQuest, nQuestStateToAdvanceTo );	
	}
	else if (pQuestDef->eSubLevelTypeTruthNew != ST_INVALID && eType == pQuestDef->eSubLevelTypeTruthNew)
	{
		s_QuestTruthRoomEnter( pPlayer, pSubLevelNew, NULL, INVALID_LINK );	
	}
							
	return QMR_OK;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT s_sQuestMessageNPCInteract(
	QUEST *pQuest,
	const QUEST_MESSAGE_INTERACT *pMessage )
{
#if !ISVERSION( CLIENT_ONLY_VERSION )
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );


	// if the quest is in an offering state, and this is the rewarder, offer the reward again
	if (QuestIsOffering( pQuest ) && s_QuestIsRewarder( pQuest, pSubject ) == TRUE)
	{

		const QUEST_DEFINITION *pQuestDefinition = pQuest->pQuestDef;
		// Reward: tell the player good job, and offer rewards
		s_QuestDisplayDialog(
			pQuest,
			pSubject,
			NPC_DIALOG_OK_CANCEL,
			pQuestDefinition->nCompleteDialog,
			pQuestDefinition->nRewardDialog );

		return QMR_OFFERING;
				
	}
#endif // CLIENT_ONLY_VERSION

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT s_sQuestMessageKillDropGiverItem( 
	QUEST *pQuest,
	const QUEST_MESSAGE_KILL_DROP_GIVER_ITEM *pMessage)
{
	UNIT *pVictim = pMessage->pVictim;
	const QUEST_DEFINITION *pQuestDef = pQuest->pQuestDef;
	UNIT *pQuestPlayer = QuestGetPlayer( pQuest );
	GAME *pGame = UnitGetGame( pQuestPlayer );
	ASSERTV_RETVAL( pQuestDef->nGiverItem != INVALID_LINK, QMR_OK, "quest \"%s\": expected nGiverItem for kill drop giver item message, check s_QuestEventMonsterKill", pQuestDef->szName );
	ASSERTV_RETVAL( s_QuestCanBeActivated( pQuest ), QMR_OK, "quest \"%s\": expected quest to be able to be activated if receiving kill drop giver item message, check s_QuestEventMonsterKill", pQuestDef->szName );
	if ( ( pQuestDef->nGiverItemMonster == INVALID_LINK ||
		   pQuestDef->nGiverItemMonster == UnitGetClass( pVictim ) ) &&
		( pQuestDef->nGiverItemLevelDef == INVALID_LINK || 
		  pQuestDef->nGiverItemLevelDef == 
		  UnitGetLevelDefinitionIndex( pVictim ) ) )
	{
		if ( RandGetFloat( pGame ) <= pQuestDef->fGiverItemDropRate )
		{
			// don't spawn the item if it is in the player's inventory, or
			// in the level
			if ( !s_QuestCheckForItem( pQuestPlayer, pQuestDef->nGiverItem ) )
			{
				UNIT *pExisting = 
					LevelFindFirstUnitOf(
						UnitGetLevel( pVictim ),
						NULL, // TODO
						GENUS_ITEM,
						pQuestDef->nGiverItem);

				if ( pExisting == NULL )
				{
					//spawns the item from the defender
					UNIT *pCreated = 
						s_SpawnItemFromUnit( 
							pVictim,
							pQuestPlayer,		// Allow only the killing player to pick it up
							pQuestDef->nGiverItem );
					ASSERTV_RETVAL( pCreated != NULL, QMR_OK, "quest \"%s\": failed to create quest giving item on monster kill", pQuestDef->szName );
					UNREFERENCED_PARAMETER( pCreated );
				}
			}
		}
	}
	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
QUEST_MESSAGE_RESULT sQuestMessageCanUseItem(
	QUEST *pQuest,
	const QUEST_MESSAGE_CAN_USE_ITEM * pMessage)
{
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pItem = UnitGetById( pGame, pMessage->idItem );
	const QUEST_DEFINITION *pQuestDef = pQuest->pQuestDef;
	if ( pItem )
	{
		if ( pQuestDef->nGiverItem != INVALID_LINK &&
			pQuestDef->nGiverItem == UnitGetClass( pItem ) )
		{
			if ( s_QuestCanBeActivated( pQuest ) )
			{
				return QMR_USE_OK;
			}
			return QMR_USE_FAIL;
		}
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
QUEST_MESSAGE_RESULT s_sQuestMessageUseItem(
	QUEST *pQuest,
	const QUEST_MESSAGE_USE_ITEM * pMessage)
{
#if !ISVERSION( CLIENT_ONLY_VERSION )

	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pItem = UnitGetById( pGame, pMessage->idItem );
	const QUEST_DEFINITION *pQuestDef = pQuest->pQuestDef;
	if ( pItem )
	{
		if ( pQuestDef->nGiverItem != INVALID_LINK &&
			pQuestDef->nGiverItem == UnitGetClass( pItem ) )
		{
			if ( s_QuestCanBeActivated( pQuest ) )
			{
				s_QuestDisplayDialog(
					pQuest,
					pItem,
					NPC_DIALOG_OK_CANCEL,
					pQuestDef->nDescriptionDialog);
				return QMR_USE_OK;
			}
			return QMR_USE_FAIL;
		}
	}
#endif
	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
QUEST_MESSAGE_RESULT s_sQuestMessageTogglePortal(
	QUEST *pQuest,
	const QUEST_MESSAGE_TOGGLE_PORTAL * pMessage)
{
#if !ISVERSION( CLIENT_ONLY_VERSION )

	const QUEST_DEFINITION *pQuestDef = pQuest->pQuestDef;
	// side quests require the portal to their destination level to be
	// opened for the prereqs to be considered done.
	if ( pQuestDef->nLevelDefDestinations[0] == pMessage->nLevelDefDestination &&
		 pQuestDef->eStyle != QS_STORY )
	{
		s_QuestUpdateCastInfo( pQuest );
	}
#endif
	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static QUEST_MESSAGE_RESULT sQuestMessageCanOperateObject( 
	QUEST *pQuest,
	const QUEST_MESSAGE_CAN_OPERATE_OBJECT *pMessage)
{
	ASSERT_RETVAL( pQuest, QMR_OK );
	GAME *pGame = QuestGetGame( pQuest );
	ASSERT_RETVAL( pGame, QMR_OK );
	UNIT *pObject = UnitGetById( pGame, pMessage->idObject );
	if ( !pObject )
		return QMR_OK;

	if ( UnitGetGenus( pObject ) != GENUS_OBJECT )
		return QMR_OK;

	int nClass = UnitGetClass( pObject );
	if ( ( pQuest->pQuestDef->nWarpToOpenActivate != INVALID_ID ) && ( pQuest->pQuestDef->nWarpToOpenActivate == nClass ) )
	{
		if ( QuestIsInactive( pQuest ) )
			return QMR_OPERATE_FORBIDDEN;

		return QMR_OPERATE_OK;
	}

	if ( ( pQuest->pQuestDef->nWarpToOpenComplete != INVALID_ID ) && ( pQuest->pQuestDef->nWarpToOpenComplete == nClass ) )
	{
		if ( QuestGetStatus( pQuest ) >= QUEST_STATUS_COMPLETE )
			return QMR_OPERATE_OK;

		return QMR_OPERATE_FORBIDDEN;
	}
	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
QUEST_MESSAGE_RESULT s_QuestsDefaultMessageHandler(
	QUEST *pQuest,
	QUEST_MESSAGE_TYPE eMessageType,
	const void *pMessage)
{

	switch (eMessageType)
	{
	
		//----------------------------------------------------------------------------
		case QM_SERVER_INTERACT_INFO:
		{
			const QUEST_MESSAGE_INTERACT_INFO *pHasInfoMessage = (const QUEST_MESSAGE_INTERACT_INFO *)pMessage;
			return s_sQuestMessageNPCHasInfo( pQuest, pHasInfoMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_ENTER_LEVEL:
		{
			const QUEST_MESSAGE_ENTER_LEVEL *pLevelMessage = (const QUEST_MESSAGE_ENTER_LEVEL *)pMessage;
			return s_sQuestTransitionLevel( pQuest, pLevelMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_SUB_LEVEL_TRANSITION:
		{
			const QUEST_MESSAGE_SUBLEVEL_TRANSITION *pSubLevelMessage = (const QUEST_MESSAGE_SUBLEVEL_TRANSITION *)pMessage;
			return sQuestMessageSubLevelTransition( pQuest, pSubLevelMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_INTERACT:
		{
			const QUEST_MESSAGE_INTERACT *pInteractMessage = (const QUEST_MESSAGE_INTERACT *)pMessage;
			return s_sQuestMessageNPCInteract( pQuest, pInteractMessage );		
		}
		
		//----------------------------------------------------------------------------
		case QM_SERVER_KILL_DROP_GIVER_ITEM:
		{
			const QUEST_MESSAGE_KILL_DROP_GIVER_ITEM *pKillDropGiverItemMessage = ( const QUEST_MESSAGE_KILL_DROP_GIVER_ITEM * )pMessage;
			return s_sQuestMessageKillDropGiverItem( pQuest, pKillDropGiverItemMessage );
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
			return s_sQuestMessageUseItem( pQuest, pUseItemMessage );
		}

		//----------------------------------------------------------------------------
		case QM_CAN_OPERATE_OBJECT:
		{
			const QUEST_MESSAGE_CAN_OPERATE_OBJECT *pCanOperateObjectMessage = (const QUEST_MESSAGE_CAN_OPERATE_OBJECT *)pMessage;
			return sQuestMessageCanOperateObject( pQuest, pCanOperateObjectMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_TOGGLE_PORTAL:
		{
			const QUEST_MESSAGE_TOGGLE_PORTAL *pOpenPortalMessage = ( const QUEST_MESSAGE_TOGGLE_PORTAL * )pMessage;
			return s_sQuestMessageTogglePortal( pQuest, pOpenPortalMessage );
		}
		
	}
	
	return QMR_OK;
	
}