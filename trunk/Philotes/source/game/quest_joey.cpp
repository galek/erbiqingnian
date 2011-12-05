//----------------------------------------------------------------------------
// FILE: quest_joey.cpp
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "c_quests.h"
#include "game.h"
#include "globalindex.h"
#include "npc.h"
#include "quest_joey.h"
#include "quests.h"
#include "dialog.h"
#include "unit_priv.h" // also includes units.h
#include "objects.h"
#include "offer.h"
#include "level.h"
#include "s_quests.h"
#include "stringtable.h"
#include "items.h"
#include "treasure.h"
#include "faction.h"
#include "room_layout.h"
#include "monsters.h"
#include "states.h"

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct QUEST_DATA_JOEY
{
	int		nTottenhamCourt;
	int		nStation;
	int		nTaint;
	int		nJoey;
	int		nItemJoeyLeg;
	SPECIES spJoeyLeg;
};

//----------------------------------------------------------------------------
// LOCAL DATA
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_JOEY * sQuestDataGet( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return (QUEST_DATA_JOEY *)pQuest->pQuestData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sSpawnTaint( QUEST * pQuest )
{
	QUEST_DATA_JOEY * pQuestData = sQuestDataGet( pQuest );

	// check the level for taint
	UNIT * pPlayer = pQuest->pPlayer;
	LEVEL * pLevel = UnitGetLevel( pPlayer );
	TARGET_TYPE eTargetTypes[] = { TARGET_BAD, TARGET_GOOD, TARGET_DEAD, TARGET_INVALID };
	UNIT * taint =  LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_MONSTER, pQuestData->nTaint );
	if ( taint )
	{
		// if there is a taint and it is dead and my quest is active at this point, mark it as completed
		if ( IsUnitDeadOrDying( taint ) && ( QuestStateGetValue( pQuest, QUEST_STATE_JOEY_TAINT_DEAD ) != QUEST_STATE_COMPLETE ) )
		{
			UnitChangeTargetType( taint, TARGET_GOOD );
			UnitSetInteractive( taint, UNIT_INTERACTIVE_ENABLED );
			QuestStateSet( pQuest, QUEST_STATE_JOEY_TAINT_DEAD, QUEST_STATE_COMPLETE );
		}
		return;
	}


	// find spawn location
	VECTOR vPosition;
	VECTOR vDirection;
	BOOL bFound = FALSE;
	pLevel = UnitGetLevel( pQuest->pPlayer );
	ROOM * room = NULL;
	for ( room = LevelGetFirstRoom( pLevel ); room && !bFound; room = LevelGetNextRoom( room ) )
	{
		ROOM_LAYOUT_SELECTION_ORIENTATION * pOrientation;
		ROOM_LAYOUT_GROUP * node = RoomGetLabelNode( room, "Taint", &pOrientation );
		if ( node && pOrientation )
		{
			s_QuestNodeGetPosAndDir( room, pOrientation, &vPosition, &vDirection, NULL, NULL );
			bFound = TRUE;
		}
	}

	// spawn! =)
	if ( !bFound )
		return;

	MONSTER_SPEC tSpec;
	tSpec.nClass = pQuestData->nTaint;
	tSpec.nExperienceLevel = QuestGetMonsterLevel( pQuest, pLevel, tSpec.nClass, 2 );
	tSpec.pRoom = room;
	tSpec.vPosition = vPosition;
	tSpec.vDirection = VECTOR( 0.0f, 0.0f, 0.0f );
	tSpec.dwFlags = MSF_DONT_DEACTIVATE_WITH_ROOM | MSF_DONT_DEPOPULATE;
	s_MonsterSpawn( UnitGetGame( pQuest->pPlayer ), tSpec );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHasLeg(
	void *pUserData)

{
	QUEST *pQuest = (QUEST *)pUserData;
	
	// finished getting leg for now...
	QuestStateSet( pQuest, QUEST_STATE_JOEY_FIND_LEG, QUEST_STATE_COMPLETE );

	// activate rest of quest
	QuestStateSet( pQuest, QUEST_STATE_JOEY_RETURN_STATION, QUEST_STATE_ACTIVE );
	QuestStateSet( pQuest, QUEST_STATE_JOEY_TALK, QUEST_STATE_ACTIVE );
	
}			

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageInteract(
	QUEST *pQuest,
	const QUEST_MESSAGE_INTERACT *pMessage )
{
	QUEST_DATA_JOEY * pData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pPlayer = UnitGetById( pGame, pMessage->idPlayer );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );
	ASSERTX_RETVAL( pPlayer, QMR_OK, "Invalid player" );
	ASSERTX_RETVAL( pSubject, QMR_OK, "Invalid NPC" );	

	// JOEY
	if (UnitIsMonsterClass( pSubject, pData->nJoey ))
	{
		
		//----------------------------------------------------------------------------	
		if (QuestIsInactive( pQuest ))
		{
			if (s_QuestCanBeActivated( pQuest ))
			{
				s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_JOEY_INIT );
				return QMR_NPC_TALK;
			}
		}
		
		//----------------------------------------------------------------------------	
		else if (QuestIsActive( pQuest ))
		{
		
			// see if they player has his leg with them
			int nItemLeg = pData->nItemJoeyLeg;
			BOOL bBroughtLeg = s_QuestCheckForItem( pPlayer, nItemLeg );
			if (bBroughtLeg)
			{
				s_QuestAdvanceTo( pQuest, QUEST_STATE_JOEY_TALK );
				s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_JOEY_COMPLETE, DIALOG_JOEY_REWARD );
			}
			else
			{
				// tell player we want the leg
				s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK, DIALOG_JOEY_ACTIVE );
			}
			
			return QMR_NPC_TALK;
									
		}
		
	}
	
	// THE TAINT!
	if (UnitIsMonsterClass( pSubject, pData->nTaint ))
	{
	
		//----------------------------------------------------------------------------		
		if (QuestIsActive( pQuest ))
		{
			if ( QuestStateGetValue( pQuest, QUEST_STATE_JOEY_FIND_LEG ) != QUEST_STATE_COMPLETE )
			{
				
				// setup actions
				OFFER_ACTIONS tActions;
				tActions.pfnAllTaken = sHasLeg;
				tActions.pAllTakenUserData = pQuest;
								
				// offer to player
				s_OfferGive( pQuest->pPlayer, pSubject, OFFERID_QUEST_JOEY_LEG, tActions);
				return QMR_OFFERING;
				
			}
			else
			{
				s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK, DIALOG_JOEY_LOOT_DONE );
			}
			return QMR_NPC_TALK;		
		}
		
	}
				
	return QMR_OK;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageTalkStopped(
	QUEST *pQuest,
	const QUEST_MESSAGE_TALK_STOPPED *pMessage )
{
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pTalkingTo = UnitGetById( pGame, pMessage->idTalkingTo );
	int nDialogStopped = pMessage->nDialog;
	UNIT * pPlayer = pQuest->pPlayer;
	QUEST_DATA_JOEY *ptQuestData = sQuestDataGet( pQuest );
	
	switch( nDialogStopped )
	{
	
		//----------------------------------------------------------------------------
		case DIALOG_JOEY_INIT:
		{
			// activate quest
			if ( s_QuestActivate( pQuest ) )
			{
			}
			break;
		}

		//----------------------------------------------------------------------------
		case DIALOG_JOEY_COMPLETE:
		{
		
			// take no-drop leg
			int nItemLeg = ptQuestData->nItemJoeyLeg;
			if (s_QuestRemoveItem( pPlayer, nItemLeg ))
			{
			
				// complete last state
				QuestStateSet( pQuest, QUEST_STATE_JOEY_TALK, QUEST_STATE_COMPLETE );
				
				// complete quest
				if ( s_QuestComplete( pQuest ) )
				{
													
					// offer reward
					if (s_QuestOfferReward( pQuest, pTalkingTo ) == TRUE)
					{					
						return QMR_OFFERING;
					}
					
				}			
				
			}
		
			break;
			
		}
		
	}

	return QMR_OK;
		
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageTransitionLevel( 
	QUEST * pQuest,
	const QUEST_MESSAGE_ENTER_LEVEL * pMessage)
{
	QUEST_STATUS eStatus = QuestGetStatus( pQuest );

	if ( eStatus != QUEST_STATE_ACTIVE )
	{
		return QMR_OK;
	}

	QUEST_DATA_JOEY * pQuestData = sQuestDataGet( pQuest );
	if ( pMessage->nLevelNewDef == pQuestData->nTottenhamCourt )
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_JOEY_TRAVEL ) == QUEST_STATE_ACTIVE )
		{
			QuestStateSet( pQuest, QUEST_STATE_JOEY_TRAVEL, QUEST_STATE_COMPLETE );
		}
		if ( QuestStateGetValue( pQuest, QUEST_STATE_JOEY_TAINT_DEAD ) == QUEST_STATE_ACTIVE )
		{
			sSpawnTaint( pQuest );
		}
	}

	if ( ( QuestStateGetValue( pQuest, QUEST_STATE_JOEY_RETURN_STATION ) == QUEST_STATE_ACTIVE ) && ( pMessage->nLevelNewDef == pQuestData->nStation ) )
	{
		QuestStateSet( pQuest, QUEST_STATE_JOEY_RETURN_STATION, QUEST_STATE_COMPLETE );
	}

	return QMR_OK;	
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageMonsterKill( 
	QUEST *pQuest,
	const QUEST_MESSAGE_MONSTER_KILL *pMessage)
{
	QUEST_DATA_JOEY * pData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pVictim = UnitGetById( pGame, pMessage->idVictim );

	// turn taint into interactive NPC if player doesn't have leg already
	if (UnitIsMonsterClass( pVictim, pData->nTaint ) &&
		QuestStateGetValue( pQuest, QUEST_STATE_JOEY_TAINT_DEAD ) == QUEST_STATE_ACTIVE )
	{
		UnitChangeTargetType( pVictim, TARGET_GOOD );
		UnitSetInteractive( pVictim, UNIT_INTERACTIVE_ENABLED );
		QuestStateSet( pQuest, QUEST_STATE_JOEY_TAINT_DEAD, QUEST_STATE_COMPLETE );
	}

	return QMR_OK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageUnitHasInfo( 
	QUEST *pQuest,
	const QUEST_MESSAGE_INTERACT_INFO *pMessage)
{
	QUEST_STATUS eStatus = QuestGetStatus( pQuest );

	if ( eStatus == QUEST_STATUS_ACTIVE )
	{
		QUEST_DATA_JOEY *pQuestData = sQuestDataGet( pQuest );
		GAME *pGame = QuestGetGame( pQuest );
		UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );

		if (UnitIsMonsterClass( pSubject, pQuestData->nTaint ))
		{
			if ( ( QuestStateGetValue( pQuest, QUEST_STATE_JOEY_FIND_LEG ) != QUEST_STATE_COMPLETE ) &&
				 ( QuestStateGetValue( pQuest, QUEST_STATE_JOEY_TAINT_DEAD ) == QUEST_STATE_COMPLETE ) )
			{
				return QMR_NPC_INFO_RETURN;
			}
			return QMR_OK;
		}
		else if (UnitIsMonsterClass( pSubject, pQuestData->nJoey ))
		{
			if ( QuestStateGetValue( pQuest, QUEST_STATE_JOEY_FIND_LEG ) != QUEST_STATE_COMPLETE )
			{
				return QMR_NPC_INFO_WAITING;
			}
			else
			{
				return QMR_NPC_INFO_RETURN;
			}
		}
	}

	return QMR_OK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static QUEST_MESSAGE_RESULT sQuestMessageMonsterInit( 
	QUEST *pQuest,
	const QUEST_MESSAGE_MONSTER_INIT *pMessage)
{
	UNIT * monster = pMessage->pMonster;
/*	if ( IS_SERVER( UnitGetGame( monster ) ) )
	{
		return QMR_OK;
	}*/

	QUEST_DATA_JOEY * pData = sQuestDataGet( pQuest );

	if ( UnitGetClass( monster ) != pData->nJoey )
	{
		return QMR_OK;
	}

	// change his name and give him his leg
	if ( QuestGetStatus( pQuest ) < QUEST_STATUS_COMPLETE )
	{
		int nStringJoey = GlobalStringGetStringIndex( GS_JOEY );
		UnitSetNameOverride( monster, nStringJoey );
	}
	else
	{
		c_StateSet( monster, monster, STATE_JOEY_LEG, 0, 0 );
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMesssageStateChanged( 
	QUEST *pQuest,
	const QUEST_MESSAGE_STATE_CHANGED *pMessage)
{
	int nQuestState = pMessage->nQuestState;
	QUEST_STATE_VALUE eValue = 	pMessage->eValueNew;

	if (nQuestState == QUEST_STATE_JOEY_TALK &&
		eValue == QUEST_STATE_COMPLETE)
	{
		// change Joey's name to Wart =)
		QUEST_DATA_JOEY * pQuestData = sQuestDataGet( pQuest );
		TARGET_TYPE eTargetTypes[] = { TARGET_GOOD, TARGET_INVALID };
		LEVEL *pLevel = UnitGetLevel( pQuest->pPlayer );
		UNIT *pJoey = LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_MONSTER, pQuestData->nJoey );
		if (pJoey)
		{
			UnitSetNameOverride( pJoey, INVALID_LINK );
			c_StateSet( pJoey, pJoey, STATE_JOEY_LEG, 0, 0 );
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
		case QM_SERVER_ENTER_LEVEL:
		{
			const QUEST_MESSAGE_ENTER_LEVEL * pTransitionMessage = ( const QUEST_MESSAGE_ENTER_LEVEL * )pMessage;
			return sQuestMessageTransitionLevel( pQuest, pTransitionMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_MONSTER_KILL:
		{
			const QUEST_MESSAGE_MONSTER_KILL *pMonsterKillMessage = (const QUEST_MESSAGE_MONSTER_KILL *)pMessage;
			return sQuestMessageMonsterKill( pQuest, pMonsterKillMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_INTERACT_INFO:
		{
			const QUEST_MESSAGE_INTERACT_INFO *pHasInfoMessage = (const QUEST_MESSAGE_INTERACT_INFO *)pMessage;
			return sQuestMessageUnitHasInfo( pQuest, pHasInfoMessage );
		}

		//----------------------------------------------------------------------------
		case QM_CLIENT_MONSTER_INIT:
		{
			const QUEST_MESSAGE_MONSTER_INIT *pMonsterInitMessage = (const QUEST_MESSAGE_MONSTER_INIT *)pMessage;
			return sQuestMessageMonsterInit( pQuest, pMonsterInitMessage );
		}

		//----------------------------------------------------------------------------
		case QM_CLIENT_QUEST_STATE_CHANGED:
		{
			const QUEST_MESSAGE_STATE_CHANGED *pStateMessage = (const QUEST_MESSAGE_STATE_CHANGED *)pMessage;
			return sQuestMesssageStateChanged( pQuest, pStateMessage );		
		}
	}

	return QMR_OK;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestFreeJoey(
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
static void sQuestDataInit(
	QUEST *pQuest,
	QUEST_DATA_JOEY *pQuestData)
{
	pQuestData->nTottenhamCourt = QuestUseGI( pQuest, GI_LEVEL_TOTTENHAM_COURT );
	pQuestData->nStation		= QuestUseGI( pQuest, GI_LEVEL_HOLBORN_STATION );
	pQuestData->nTaint			= QuestUseGI( pQuest, GI_MONSTER_TAINT );
	pQuestData->nJoey			= QuestUseGI( pQuest, GI_MONSTER_JOEY );
	pQuestData->nItemJoeyLeg	= QuestUseGI( pQuest, GI_ITEM_JOEY_LEG_NO_DROP );
	pQuestData->spJoeyLeg = MAKE_SPECIES( GENUS_ITEM, pQuestData->nItemJoeyLeg );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestInitJoey(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	
	
	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		

	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_JOEY * pQuestData = ( QUEST_DATA_JOEY * )GMALLOCZ( pGame, sizeof( QUEST_DATA_JOEY ) );
	sQuestDataInit( pQuest, pQuestData );
	pQuest->pQuestData = pQuestData;
			
	// add additional cast members
	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nTaint );
	
}
