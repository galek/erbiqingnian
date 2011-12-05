//----------------------------------------------------------------------------
// FILE: quest_broken.cpp
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
#include "quest_broken.h"
#include "quests.h"
#include "dialog.h"
#include "unit_priv.h" // also includes units.h
#include "objects.h"
#include "s_quests.h"
#include "stringtable.h"
#include "c_sound.h"
#include "player.h"
#include "states.h"
#include "monsters.h"
#include "treasure.h"
#include "items.h"
#include "level.h"
#include "room_layout.h"
#include "faction.h"

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct QUEST_DATA_BROKEN
{
	int		nMurmur;
	int		nLevelHolborn;
};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_BROKEN *sQuestDataGet(
	QUEST *pQuest)
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return (QUEST_DATA_BROKEN *)pQuest->pQuestData;
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
	QUEST_DATA_BROKEN * pQuestData = sQuestDataGet( pQuest );

	// quest inactive
	if ( pQuest->eStatus == QUEST_STATUS_INACTIVE )
	{
		int current_level = UnitGetLevelDefinitionIndex( pPlayer );
		if ( current_level == pQuestData->nLevelHolborn )
		{
			if (UnitIsMonsterClass( pSubject, pQuestData->nMurmur ))
			{
				s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_BROKEN_TALK_MURMUR );
				return QMR_NPC_TALK;
			}
		}
	}

	return QMR_OK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageTalkStopped(
	QUEST *pQuest,
	const QUEST_MESSAGE_TALK_STOPPED *pMessage)
{
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pTalkingTo = UnitGetById( pGame, pMessage->idTalkingTo );
	REF(pTalkingTo);
	int nDialogStopped = pMessage->nDialog;

	switch( nDialogStopped )
	{
	
		//----------------------------------------------------------------------------
		case DIALOG_BROKEN_TALK_MURMUR:
		{
			if ( s_QuestActivate( pQuest ) )
			{
				QuestStateSet( pQuest, QUEST_STATE_BROKEN_TALK_MURMUR, QUEST_STATE_COMPLETE );
				s_QuestComplete( pQuest );
			}
			break;
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
	}

	return QMR_OK;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestFreeBroken(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	GAME *pGame = QuestGetGame( pQuest );
	
	// free quest data
	ASSERTX_RETURN( pQuest->pQuestData, "Expected quest data" );
	GFREE( pGame, pQuest->pQuestData );
	pQuest->pQuestData = NULL;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sQuestDataBroken(
	QUEST *pQuest,
	QUEST_DATA_BROKEN * pQuestData )
{
	pQuestData->nMurmur			= QuestUseGI( pQuest, GI_MONSTER_MURMUR );
	pQuestData->nLevelHolborn	= QuestUseGI( pQuest, GI_LEVEL_HOLBORN_STATION );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestInitBroken(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	
	GAME *pGame = QuestGetGame( pQuest );
	
	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		
	
	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	QUEST_DATA_BROKEN * pQuestData = ( QUEST_DATA_BROKEN * )GMALLOCZ( pGame, sizeof( QUEST_DATA_BROKEN ) );
	sQuestDataBroken( pQuest, pQuestData );
	pQuest->pQuestData = pQuestData;
}
