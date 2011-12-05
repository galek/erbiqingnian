//----------------------------------------------------------------------------
// FILE: quest_truth_e.cpp
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "c_quests.h"
#include "game.h"
#include "gameglobals.h"
#include "globalindex.h"
#include "npc.h"
#include "quest_truth_e.h"
#include "quests.h"
#include "dialog.h"
#include "unit_priv.h" // also includes units.h
#include "objects.h"
#include "level.h"
#include "s_quests.h"
#include "stringtable.h"
#include "monsters.h"
#include "offer.h"
#include "gameclient.h"

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

/*
truth_e_talk_child
truth_e_talk_emmera

hellgate_talk_emmera


TRUTH_E_START
TRUTH_E_EMMERA
*/

//----------------------------------------------------------------------------
struct QUEST_DATA_TRUTH_E
{
	int		nLevelTestOfHope;

	int		nSageOld;
	int		nSageNew;

	int		nEmmera;
	int		nEmmeraOutside;

	int		nTruthEnterPortal;
	int		nTruthSpawnMarker;

	BOOL	bPlayedMovie;
};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_TRUTH_E * sQuestDataGet( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return (QUEST_DATA_TRUTH_E *)pQuest->pQuestData;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static UNIT * sFindTruthSpawnLocation( QUEST * pQuest )
{
	// check the level for a boss already
	UNIT * pPlayer = pQuest->pPlayer;
	LEVEL * pLevel = UnitGetLevel( pPlayer );
	TARGET_TYPE eTargetTypes[] = { TARGET_OBJECT, TARGET_INVALID };
	QUEST_DATA_TRUTH_E * pQuestData = sQuestDataGet( pQuest );
	return LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_OBJECT, pQuestData->nTruthSpawnMarker );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageUnitHasInfo( 
	QUEST *pQuest,
	const QUEST_MESSAGE_INTERACT_INFO *pMessage)
{
	QUEST_STATUS eStatus = QuestGetStatus( pQuest );
	if ( eStatus >= QUEST_STATUS_COMPLETE )
		return QMR_OK;

	QUEST_DATA_TRUTH_E * pQuestData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );

	if ( eStatus == QUEST_STATUS_INACTIVE )
	{
		if ( s_QuestIsPrereqComplete( pQuest ) && UnitIsMonsterClass( pSubject, pQuestData->nEmmeraOutside ) )
		{
			return QMR_NPC_STORY_NEW;
		}
		return QMR_OK;
	}

	if ( eStatus != QUEST_STATUS_ACTIVE )
		return QMR_OK;

	// quest active
	if ( UnitIsMonsterClass( pSubject, pQuestData->nSageOld ) )
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_TRUTH_E_TALK_CHILD ) == QUEST_STATE_ACTIVE )
		{
			return QMR_NPC_STORY_RETURN;
		}
		return QMR_OK;
	}

	if ( UnitIsMonsterClass( pSubject, pQuestData->nEmmera ) || UnitIsMonsterClass( pSubject, pQuestData->nEmmeraOutside ) )
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_TRUTH_E_TALK_EMMERA ) == QUEST_STATE_ACTIVE )
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
	QUEST_DATA_TRUTH_E * pQuestData = sQuestDataGet( pQuest );

	if ( UnitIsMonsterClass( pSubject, pQuestData->nSageOld ) )
	{
	
		// force to this state in the quest
		s_QuestInactiveAdvanceTo( pQuest, QUEST_STATE_TRUTH_E_TALK_CHILD );		

		s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_TRUTH_E_CHILD );
		return QMR_NPC_TALK;

	}

	if ( QuestIsInactive( pQuest ) && UnitIsMonsterClass( pSubject, pQuestData->nEmmeraOutside ) && s_QuestIsPrereqComplete( pQuest ) )
	{
		s_QuestTruthEBegin( pQuest );
		return QMR_FINISHED;
	}

	if ( !QuestIsActive( pQuest ) )
	{
		return QMR_OK;
	}
	
	if ( UnitIsMonsterClass( pSubject, pQuestData->nEmmera ) || 
		 UnitIsMonsterClass( pSubject, pQuestData->nEmmeraOutside ) )
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_TRUTH_E_TALK_EMMERA ) == QUEST_STATE_ACTIVE )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_TRUTH_E_EMMERA );
			return QMR_NPC_TALK;
		}
		return QMR_OK;
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static BOOL sPlayTruthMovie(
	GAME *pGame,
	UNIT *pPlayer,
	const struct EVENT_DATA &tEventData)
{
	const DIALOG_DATA *pDialogData = DialogGetData( DIALOG_TRUTH_E_CHILD2 );
	if ( !pDialogData )
		return TRUE;

	if ( pDialogData->nMovieListOnFinished == INVALID_LINK )
		return TRUE;

	MSG_SCMD_PLAY_MOVIELIST tMessage;
	tMessage.nMovieListIndex = pDialogData->nMovieListOnFinished;

	// send it
	GAMECLIENTID idClient = UnitGetClientId( pPlayer );
	s_SendMessage( pGame, idClient, SCMD_PLAY_MOVIELIST, &tMessage );

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#define DELAY_TIL_MOVIE			( GAME_TICKS_PER_SECOND * 3 )

static QUEST_MESSAGE_RESULT sQuestMessageTalkStopped(
	QUEST *pQuest,
	const QUEST_MESSAGE_TALK_STOPPED *pMessage )
{
	int nDialogStopped = pMessage->nDialog;
	
	switch( nDialogStopped )
	{
		//----------------------------------------------------------------------------
		case DIALOG_TRUTH_E_CHILD:
		{
			QuestStateSet( pQuest, QUEST_STATE_TRUTH_E_TALK_CHILD, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_TRUTH_E_TALK_EMMERA, QUEST_STATE_ACTIVE );
			
			// transport to new truth room
			UNIT *pPlayer = QuestGetPlayer( pQuest );
			s_QuestTransportToNewTruthRoom( pPlayer );
			QUEST_DATA_TRUTH_E * pQuestData = sQuestDataGet( pQuest );
			if ( ( QuestStateGetValue( pQuest, QUEST_STATE_TRUTH_E_TALK_EMMERA ) == QUEST_STATE_ACTIVE ) &&
				 ( pQuestData->bPlayedMovie == FALSE ) &&
				 ( UnitHasTimedEvent( pPlayer, sPlayTruthMovie ) == FALSE ) )
			{
				pQuestData->bPlayedMovie = TRUE;
				int nDurationInTicks = (int)(GAME_TICKS_PER_SECOND * TIME_FROM_TRUTH_FLASH_TILL_TRANSPORT_IN_SECONDS) + DELAY_TIL_MOVIE;
				UnitRegisterEventTimed( pPlayer, sPlayTruthMovie, NULL, nDurationInTicks );
			}
			break;
			
		}

		//----------------------------------------------------------------------------
		case DIALOG_TRUTH_E_EMMERA:
		{
			QuestStateSet( pQuest, QUEST_STATE_TRUTH_E_TALK_EMMERA, QUEST_STATE_COMPLETE );
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
	if ( !QuestIsActive( pQuest ) )
		return QMR_OK;

	UNIT * player = pQuest->pPlayer;
	GAME * game = UnitGetGame( player );
	ASSERT_RETVAL( IS_SERVER( game ), QMR_OK );

	QUEST_DATA_TRUTH_E * pQuestData = sQuestDataGet( pQuest );

	if ( pMessage->nLevelNewDef == pQuestData->nLevelTestOfHope )
	{
		// find spawn location unit
		UNIT * pLocation = sFindTruthSpawnLocation( pQuest );
		s_QuestSpawnObject( pQuestData->nTruthEnterPortal, pLocation, pQuest->pPlayer );
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
	}

	return QMR_OK;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestFreeTruthE(
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
	QUEST_DATA_TRUTH_E * pQuestData)
{
	pQuestData->nTruthEnterPortal			= QuestUseGI( pQuest, GI_OBJECT_TRUTH_E_OLD_ENTER );
	pQuestData->nLevelTestOfHope			= QuestUseGI( pQuest, GI_LEVEL_TEST_OF_HOPE );
	pQuestData->nSageOld					= QuestUseGI( pQuest, GI_MONSTER_TRUTH_E_SAGE_OLD );
	pQuestData->nSageNew					= QuestUseGI( pQuest, GI_MONSTER_TRUTH_E_SAGE_NEW );
	pQuestData->nEmmera						= QuestUseGI( pQuest, GI_MONSTER_EMMERA );
	pQuestData->nEmmeraOutside				= QuestUseGI( pQuest, GI_MONSTER_EMMERA_COMBAT );
	pQuestData->nTruthSpawnMarker			= QuestUseGI( pQuest, GI_OBJECT_TRUTH_SPAWN_MARKER );

	pQuestData->bPlayedMovie = FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestInitTruthE(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	
	
	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		
			
	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_TRUTH_E * pQuestData = ( QUEST_DATA_TRUTH_E * )GMALLOCZ( pGame, sizeof( QUEST_DATA_TRUTH_E ) );
	sQuestDataInit( pQuest, pQuestData );
	pQuest->pQuestData = pQuestData;

	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nSageOld );
	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nEmmera );
	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nEmmeraOutside );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*
void s_QuestRestoreTruthE(
	QUEST *pQuest,
	UNIT *pSetupPlayer)
{
}
*/

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void s_QuestTruthEBegin( QUEST * pQuest )
{
	ASSERTX_RETURN( IS_SERVER( UnitGetGame( pQuest->pPlayer ) ), "Server Only" );
	ASSERTX_RETURN( pQuest->nQuest == QUEST_TRUTH_E, "Wrong quest sent to function. Need Truth E" );

	if ( QuestIsComplete( pQuest ) )
		return;

	if ( !s_QuestIsPrereqComplete( pQuest ) )
		return;

	s_QuestActivate( pQuest );

	QUEST_DATA_TRUTH_E * pQuestData = sQuestDataGet( pQuest );
	UNIT * pLocation = sFindTruthSpawnLocation( pQuest );
	ASSERT_RETURN( pLocation );
	s_QuestSpawnObject( pQuestData->nTruthEnterPortal, pLocation, pQuest->pPlayer );
}
