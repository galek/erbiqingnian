//----------------------------------------------------------------------------
// FILE: quest_demo.cpp
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
#include "quest_demoend.h"
#include "quests.h"
#include "dialog.h"
#include "unit_priv.h" // also includes units.h
#include "objects.h"
#include "level.h"
#include "s_quests.h"
#include "stringtable.h"
#include "monsters.h"
#include "offer.h"

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct QUEST_DATA_DEMOEND
{
	int		nMurmur;
};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_DEMOEND * sQuestDataGet( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return (QUEST_DATA_DEMOEND *)pQuest->pQuestData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageInteract( 
	QUEST *pQuest,
	const QUEST_MESSAGE_INTERACT *pMessage)
{
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );
	QUEST_DATA_DEMOEND * pQuestData = sQuestDataGet( pQuest );

	if ( UnitIsMonsterClass( pSubject, pQuestData->nMurmur ) )
	{
		if ( QuestIsActive( pQuest ) )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK, DIALOG_DEMOEND_DESCRIPTION );
			return QMR_NPC_TALK;
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
	if ( QuestGetStatus( pQuest ) >= QUEST_STATUS_COMPLETE )
		return QMR_OK;

	// complete no matter what for beta
	s_QuestDemoEndForceComplete( pQuest->pPlayer );

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
void QuestFreeDemoEnd(
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
	GAME * pGame,
	QUEST_DATA_DEMOEND * pQuestData)
{
	pQuestData->nMurmur = GlobalIndexGet( GI_MONSTER_MURMUR );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestInitDemoEnd(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	
	
	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		
			
	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_DEMOEND * pQuestData = ( QUEST_DATA_DEMOEND * )GMALLOCZ( pGame, sizeof( QUEST_DATA_DEMOEND ) );
	sQuestDataInit( pGame, pQuestData );
	pQuest->pQuestData = pQuestData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// This quest blocks progress in the game if it is a beta version
// auto start end of beta quest
void s_QuestDemoEndForceComplete( UNIT * pPlayer )
{
	// comment out these two lines to move past act 1 in beta (open act 2)
	//if ( AppIsBeta() )
	//	return;

/*	QUEST * pFirstBetaQuest = QuestGetQuest( pPlayer, QUEST_FIRSTBETAEND );
	ASSERT_RETURN( pFirstBetaQuest );
	s_QuestActivate( pFirstBetaQuest );
	QuestStateSet( pFirstBetaQuest, QUEST_STATE_FIRSTBETAEND_END, QUEST_STATE_COMPLETE );
	s_QuestComplete( pFirstBetaQuest );

	// comment out these two lines to move past act 2 in beta (open act 3)
	if ( AppIsBeta() )
		return;

	QUEST * pSecondBetaQuest = QuestGetQuest( pPlayer, QUEST_DEMOEND );
	ASSERT_RETURN( pSecondBetaQuest );
	s_QuestActivate( pSecondBetaQuest );
	s_QuestComplete( pSecondBetaQuest );*/
}
