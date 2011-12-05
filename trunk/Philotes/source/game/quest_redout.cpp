//----------------------------------------------------------------------------
// FILE: quest_redout.cpp
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
#include "quest_redout.h"
#include "quests.h"
#include "dialog.h"
#include "unit_priv.h" // also includes units.h
#include "objects.h"
#include "level.h"
#include "s_quests.h"
#include "stringtable.h"
#include "ai.h"
#include "uix.h"
#include "states.h"
#include "monsters.h"
#include "room.h"
#include "room_path.h"
#include "offer.h"
#include "e_portal.h"
#include "c_camera.h"
#include "skills.h"
#include "inventory.h"

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct QUEST_DATA_REDOUT
{
	int		nLucious;
	int		nArphaun;

	int		nLevelRedout;
	int		nLevelCCStation;

	int		nComputer;

	int		nHunter1;
	int		nHunter2;
	int		nHunter3;
	int		nHunter4;
	int		nNumTroops;

	int		nBeelzebub;
	
	int		nStateRTSActive;
	int		nStateRTSComplete;
	
};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_REDOUT * sQuestDataGet( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return (QUEST_DATA_REDOUT *)pQuest->pQuestData;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageUnitHasInfo( 
	QUEST *pQuest,
	const QUEST_MESSAGE_INTERACT_INFO *pMessage)
{
	QUEST_STATUS eStatus = QuestGetStatus( pQuest );
	QUEST_DATA_REDOUT * pQuestData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );

	// quest active
	if (UnitIsMonsterClass( pSubject, pQuestData->nLucious ))
	{
		if ( ( eStatus == QUEST_STATUS_INACTIVE ) && s_QuestIsPrereqComplete( pQuest ) )
		{
			return QMR_NPC_STORY_NEW;
		}
		return QMR_OK;
	}

	// quest not active
	if ( eStatus != QUEST_STATUS_ACTIVE )
	{
		return QMR_OK;
	}

	if (UnitIsMonsterClass( pSubject, pQuestData->nArphaun ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_REDOUT_TALK_LORD ) == QUEST_STATE_ACTIVE )
		{
			return QMR_NPC_STORY_RETURN;
		}
		if ( QuestStateGetValue( pQuest, QUEST_STATE_REDOUT_TALK_LORD2 ) == QUEST_STATE_ACTIVE )
		{
			return QMR_NPC_STORY_RETURN;
		}
		return QMR_OK;
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageInteract( 
	QUEST *pQuest,
	const QUEST_MESSAGE_INTERACT *pMessage)
{
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pPlayer = UnitGetById( pGame, pMessage->idPlayer );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );
	ASSERTX_RETVAL( pPlayer, QMR_OK, "Invalid player" );
	ASSERTX_RETVAL( pSubject, QMR_OK, "Invalid NPC" );
	QUEST_DATA_REDOUT * pQuestData = sQuestDataGet( pQuest );
	QUEST_STATUS eStatus = QuestGetStatus( pQuest );

	// quest activate
	if (UnitIsMonsterClass( pSubject, pQuestData->nLucious ))
	{
		if ( ( eStatus == QUEST_STATUS_INACTIVE ) && s_QuestIsPrereqComplete( pQuest ) )
		{
			if ( s_QuestActivate( pQuest ) )
			{
				QuestStateSet( pQuest, QUEST_STATE_REDOUT_TALK_LUCIOUS, QUEST_STATE_ACTIVE );
				s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK, DIALOG_REDOUT_LUCIOUS, INVALID_LINK, GI_MONSTER_TECHSMITH_314 );
				return QMR_NPC_TALK;
			}
			return QMR_OK;
		}
	}

	if ( eStatus != QUEST_STATUS_ACTIVE )
	{
		return QMR_OK;
	}

	if (UnitIsMonsterClass(pSubject, pQuestData->nArphaun ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_REDOUT_TALK_LORD ) == QUEST_STATE_ACTIVE )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK, DIALOG_REDOUT_LORD );
			return QMR_NPC_TALK;
		}
		if ( QuestStateGetValue( pQuest, QUEST_STATE_REDOUT_TALK_LORD2 ) == QUEST_STATE_ACTIVE )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK, DIALOG_REDOUT_LORD_RETURN );
			return QMR_NPC_TALK;
		}
		return QMR_OK;
	}

	return QMR_OK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageTalkStopped(
	QUEST *pQuest,
	const QUEST_MESSAGE_TALK_STOPPED *pMessage)
{
	int nDialogStopped = pMessage->nDialog;
	
	switch( nDialogStopped )
	{

		//----------------------------------------------------------------------------
		case DIALOG_REDOUT_LUCIOUS:
		{
			QuestStateSet( pQuest, QUEST_STATE_REDOUT_TALK_LUCIOUS, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_REDOUT_TALK_LORD, QUEST_STATE_ACTIVE );
			break;
		}

		//----------------------------------------------------------------------------
		case DIALOG_REDOUT_LORD:
		{
			QuestStateSet( pQuest, QUEST_STATE_REDOUT_TALK_LORD, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_REDOUT_TRAVEL, QUEST_STATE_ACTIVE );
			// map it!
			s_QuestMapReveal( pQuest, GI_LEVEL_CRAVEN_ST );
			s_QuestMapReveal( pQuest, GI_LEVEL_EMBANKMENT_APPROACH );
			s_QuestMapReveal( pQuest, GI_LEVEL_EMBANKMENT_REDOUT );
			break;
		}

		//----------------------------------------------------------------------------
		case DIALOG_REDOUT_LORD_RETURN:
		{
			QuestStateSet( pQuest, QUEST_STATE_REDOUT_TALK_LORD2, QUEST_STATE_COMPLETE );
			s_QuestComplete( pQuest );
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
	if ( QuestGetStatus( pQuest ) != QUEST_STATUS_ACTIVE )
	{
		return QMR_OK;
	}

	QUEST_DATA_REDOUT * pQuestData = sQuestDataGet( pQuest );

	if ( pMessage->nLevelNewDef == pQuestData->nLevelRedout )
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_REDOUT_TRAVEL ) == QUEST_STATE_ACTIVE )
		{
			QuestStateSet( pQuest, QUEST_STATE_REDOUT_TRAVEL, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_REDOUT_COMMAND, QUEST_STATE_ACTIVE );
		}
		else if( QuestStateGetValue( pQuest, QUEST_STATE_REDOUT_VICTORY ) == QUEST_STATE_ACTIVE )
		{
			QuestStateSet( pQuest, QUEST_STATE_REDOUT_COMMAND, QUEST_STATE_HIDDEN );
			QuestStateSet( pQuest, QUEST_STATE_REDOUT_COMMAND, QUEST_STATE_ACTIVE );
			QuestStateSet( pQuest, QUEST_STATE_REDOUT_VICTORY, QUEST_STATE_INVALID );
		}
	}

	if ( pMessage->nLevelNewDef == pQuestData->nLevelCCStation )
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_REDOUT_RETURN ) == QUEST_STATE_ACTIVE )
		{
			QuestStateSet( pQuest, QUEST_STATE_REDOUT_RETURN, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_REDOUT_TALK_LORD2, QUEST_STATE_ACTIVE );
		}
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageCanOperateObject( 
	QUEST *pQuest,
	const QUEST_MESSAGE_CAN_OPERATE_OBJECT *pMessage)
{
	if ( !QuestIsActive( pQuest ) )
		return QMR_OK;

	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pObject = UnitGetById( pGame, pMessage->idObject );
	QUEST_DATA_REDOUT * pQuestData = sQuestDataGet( pQuest );

	// is this an object that this quest cares about
	if ( UnitIsObjectClass( pObject, pQuestData->nComputer ) )
	{	
		if ( QuestStateGetValue( pQuest, QUEST_STATE_REDOUT_COMMAND ) == QUEST_STATE_ACTIVE || 
				QuestStateGetValue( pQuest, QUEST_STATE_REDOUT_COMMAND ) == QUEST_STATE_FAIL)
		{
			return QMR_OPERATE_OK;
		}
		else
		{
			return QMR_OPERATE_FORBIDDEN;
		}
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static QUEST_MESSAGE_RESULT sQuestMessageObjectOperated( 
	QUEST *pQuest,
	const QUEST_MESSAGE_OBJECT_OPERATED *pMessage)
{
	// drb - I'm not really sure why the following lines were taking out. The history doesn't go back that far. I've put them back in...
	if (QuestGetStatus( pQuest ) != QUEST_STATUS_ACTIVE)
	{
		return QMR_OK;
	}

	QUEST_DATA_REDOUT * pQuestData = sQuestDataGet( pQuest );
	UNIT *pTarget = pMessage->pTarget;

	if ( UnitGetClass( pTarget ) == pQuestData->nComputer  )
	{
		s_StateSet( pTarget, pTarget, STATE_OPERATE_OBJECT_FX, 0 );
		QuestStateSet( pQuest, QUEST_STATE_REDOUT_COMMAND, QUEST_STATE_HIDDEN );
		QuestStateSet( pQuest, QUEST_STATE_REDOUT_VICTORY, QUEST_STATE_ACTIVE );
		UNIT * pPlayer = pQuest->pPlayer;
		GAME * pGame = UnitGetGame( pPlayer );
		if ( ! UnitHasState( pGame, pQuest->pPlayer, STATE_QUEST_RTS ))
		{
			DWORD dwFlags = SKILL_START_FLAG_INITIATED_BY_SERVER | SKILL_START_FLAG_SERVER_ONLY;
			SkillStartRequest( pGame, pPlayer, pTarget->pUnitData->nSkillWhenUsed, UnitGetId( pPlayer ), VECTOR(0), dwFlags, SkillGetNextSkillSeed( pGame ) );
			LevelNeedRoomUpdate( UnitGetLevel( pPlayer ), TRUE );
			pQuestData->nNumTroops = 4;
		}
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageMonsterDying( 
	QUEST *pQuest,
	const QUEST_MESSAGE_MONSTER_DYING *pMessage)
{
	if ( QuestGetStatus( pQuest ) != QUEST_STATUS_ACTIVE )
		return QMR_OK;

	QUEST_DATA_REDOUT * pQuestData = sQuestDataGet( pQuest );
	UNIT *pVictim = UnitGetById( QuestGetGame( pQuest ), pMessage->idVictim );
	int monster_class = UnitGetClass( pVictim );

	if ( ( monster_class == pQuestData->nHunter1 ) ||
		 ( monster_class == pQuestData->nHunter2 ) ||
		 ( monster_class == pQuestData->nHunter3 ) ||
		 ( monster_class == pQuestData->nHunter4 ) )
	{
	
		// check if it was my troop that died... NOTE that we're using the "DYING" message
		// here so that we can check the container ... since this message happens early
		// in the dying chain we are certain that the container is still in tact
		// (ie. these troops are pets which will be removed from their container via their
		// dying event, which is too late for us)
		UNIT * pMaster = UnitGetContainer( pVictim );
		ASSERT( pMaster );
		UNIT * pPlayer = pQuest->pPlayer;
		if ( pMaster && ( pMaster == pPlayer ) )
		{
			pQuestData->nNumTroops--;
			if ( pQuestData->nNumTroops <= 0 )
			{
				// quest failed!
				s_StateClear( pPlayer, UnitGetId( pPlayer ), STATE_QUEST_RTS_REDOUT, 0 );
			}
		}
	}

	return QMR_OK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageMonsterKill( 
	QUEST *pQuest,
	const QUEST_MESSAGE_MONSTER_KILL *pMessage)
{
	if ( QuestGetStatus( pQuest ) != QUEST_STATUS_ACTIVE )
		return QMR_OK;

	QUEST_DATA_REDOUT * pQuestData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pVictim = UnitGetById( pGame, pMessage->idVictim );

	if ( ( UnitIsMonsterClass( pVictim, pQuestData->nBeelzebub ) ) && 
		 ( QuestStateGetValue( pQuest, QUEST_STATE_REDOUT_VICTORY ) == QUEST_STATE_ACTIVE ) )
	{
		//s_QuestStateSetForAll( pQuest, QUEST_STATE_REDOUT_VICTORY, QUEST_STATE_COMPLETE );
		//s_QuestStateSetForAll( pQuest, QUEST_STATE_REDOUT_RETURN, QUEST_STATE_ACTIVE );
		//UNIT * pPlayer = pQuest->pPlayer;
		//s_StateClear( pPlayer, UnitGetId( pPlayer ), STATE_QUEST_RTS, 0 );
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

	if ( nQuestState == QUEST_STATE_REDOUT_VICTORY )
	{
		if ( eValue == QUEST_STATE_FAIL )
		{
			UIShowMessage( QUIM_DIALOG, DIALOG_QUEST_FAILED );
		}
		else if ( eValue == QUEST_STATE_COMPLETE )
		{
			UIShowMessage( QUIM_DIALOG, DIALOG_QUEST_VICTORY );
		}
	}

	return QMR_OK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageUnitStateChanged( 
	QUEST *pQuest,
	const QUEST_MESSAGE_UNIT_STATE_CHANGE *pMessage)
{
	if ( pMessage->pPlayer != pQuest->pPlayer )
		return QMR_OK;

	if ( QuestGetStatus( pQuest ) != QUEST_STATUS_ACTIVE )
		return QMR_OK;

	QUEST_DATA_REDOUT * pQuestRedout = sQuestDataGet( pQuest );
	if(!pQuestRedout)
		return QMR_OK;

	if(pQuestRedout->nStateRTSComplete == pMessage->nState)
	{
		if(pMessage->bStateSet)
		{
			QuestStateSet( pQuest, QUEST_STATE_REDOUT_VICTORY, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_REDOUT_RETURN, QUEST_STATE_ACTIVE );
			QuestStateSet( pQuest, QUEST_STATE_REDOUT_COMMAND, QUEST_STATE_COMPLETE );
		}
	}
	else if(pQuestRedout->nStateRTSActive == pMessage->nState)
	{
		if ( !pMessage->bStateSet && ( StateIsA( pMessage->nState, STATE_QUEST_RTS ) ) && ( QuestStateGetValue( pQuest, QUEST_STATE_REDOUT_VICTORY ) == QUEST_STATE_ACTIVE ) )
		{
			QuestStateSet( pQuest, QUEST_STATE_REDOUT_VICTORY, QUEST_STATE_INVALID );
			QuestStateSet( pQuest, QUEST_STATE_REDOUT_COMMAND, QUEST_STATE_ACTIVE );
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
		case QM_SERVER_INTERACT_INFO:
		{
			const QUEST_MESSAGE_INTERACT_INFO *pHasInfoMessage = (const QUEST_MESSAGE_INTERACT_INFO *)pMessage;
			return sQuestMessageUnitHasInfo( pQuest, pHasInfoMessage );
		}

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
		case QM_CAN_OPERATE_OBJECT:
		{
			const QUEST_MESSAGE_CAN_OPERATE_OBJECT *pCanOperateObjectMessage = (const QUEST_MESSAGE_CAN_OPERATE_OBJECT *)pMessage;
			return sQuestMessageCanOperateObject( pQuest, pCanOperateObjectMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_OBJECT_OPERATED:
		{
			const QUEST_MESSAGE_OBJECT_OPERATED *pObjectOperatedMessage = (const QUEST_MESSAGE_OBJECT_OPERATED *)pMessage;
			return sQuestMessageObjectOperated( pQuest, pObjectOperatedMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_MONSTER_DYING:
		{
			const QUEST_MESSAGE_MONSTER_DYING *pMonsterDyingMessage = (const QUEST_MESSAGE_MONSTER_DYING *)pMessage;
			return sQuestMessageMonsterDying( pQuest, pMonsterDyingMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_MONSTER_KILL:
		{
			const QUEST_MESSAGE_MONSTER_KILL *pMonsterKillMessage = (const QUEST_MESSAGE_MONSTER_KILL *)pMessage;
			return sQuestMessageMonsterKill( pQuest, pMonsterKillMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_UNIT_STATE_CHANGE:
		{
			const QUEST_MESSAGE_UNIT_STATE_CHANGE *pUnitStateChangeMessage = ( const QUEST_MESSAGE_UNIT_STATE_CHANGE * )pMessage;
			return sQuestMessageUnitStateChanged( pQuest, pUnitStateChangeMessage );
		}

#if !ISVERSION(SERVER_VERSION)
		//----------------------------------------------------------------------------
		case QM_SERVER_RELOAD_UI:
		{
			GAME *pGame = UnitGetGame( pQuest->pPlayer );
			if (pQuest->pPlayer && 
				IS_CLIENT(pGame) &&
				(QuestGetStatus( pQuest ) == QUEST_STATUS_ACTIVE) &&
				UnitHasState( pGame, pQuest->pPlayer, STATE_QUEST_RTS ))
			{														
				QUEST_DATA_REDOUT * pQuestData = sQuestDataGet( pQuest );
				if (UnitGetLevelDefinitionIndex( pQuest->pPlayer ) == pQuestData->nLevelRedout)
				{
					UIComponentActivate( UICOMP_RTS_ACTIVEBAR, TRUE );
					UIEnterRTSMode();
				}
			}
			return QMR_OK;
		}

		//----------------------------------------------------------------------------
		case QM_CLIENT_QUEST_STATE_CHANGED:
		{
			const QUEST_MESSAGE_STATE_CHANGED *pStateMessage = (const QUEST_MESSAGE_STATE_CHANGED *)pMessage;
			return sQuestMesssageStateChanged( pQuest, pStateMessage );		
		}
#endif// !ISVERSION(SERVER_VERSION)
	}

	return QMR_OK;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestFreeRedout(
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
	QUEST_DATA_REDOUT * pQuestData)
{
	pQuestData->nLucious 			= QuestUseGI( pQuest, GI_MONSTER_LUCIOUS );
	pQuestData->nArphaun 			= QuestUseGI( pQuest, GI_MONSTER_LORD_ARPHAUN );
	pQuestData->nLevelRedout		= QuestUseGI( pQuest, GI_LEVEL_EMBANKMENT_REDOUT );
	pQuestData->nLevelCCStation		= QuestUseGI( pQuest, GI_LEVEL_CHARING_CROSS_STATION );
	pQuestData->nComputer			= QuestUseGI( pQuest, GI_OBJECT_COMMAND_COMPUTER );
	pQuestData->nHunter1 			= QuestUseGI( pQuest, GI_MONSTER_RTS_HUNTER1 );
	pQuestData->nHunter2 			= QuestUseGI( pQuest, GI_MONSTER_RTS_HUNTER2 );
	pQuestData->nHunter3 			= QuestUseGI( pQuest, GI_MONSTER_RTS_HUNTER3 );
	pQuestData->nHunter4 			= QuestUseGI( pQuest, GI_MONSTER_RTS_HUNTER4 );
	pQuestData->nBeelzebub			= QuestUseGI( pQuest, GI_MONSTER_BEELZEBUB );
	pQuestData->nStateRTSActive		= QuestUseGI( pQuest, GI_STATE_QUEST_RTS );
	pQuestData->nStateRTSComplete	= QuestUseGI( pQuest, GI_STATE_QUEST_RTS_COMPLETE );
	pQuestData->nNumTroops = 0;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestInitRedout(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	
	
	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		
			
	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_REDOUT * pQuestData = ( QUEST_DATA_REDOUT * )GMALLOCZ( pGame, sizeof( QUEST_DATA_REDOUT ) );
	sQuestDataInit( pQuest, pQuestData );
	pQuest->pQuestData = pQuestData;

	// add additional cast members
	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nArphaun );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*
void s_QuestRestoreRedout(
	QUEST *pQuest,
	UNIT *pSetupPlayer)
{
}
*/
