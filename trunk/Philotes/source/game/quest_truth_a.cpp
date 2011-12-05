//----------------------------------------------------------------------------
// FILE: quest_truth_a.cpp
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "appcommon.h"
#include "dialog.h"
#include "globalindex.h"
#include "npc.h"
#include "objects.h"
#include "quest_truth_a.h"
#include "quest_wisdomchaos.h"
#include "quest_demoend.h"
#include "quests.h"
#include "room_path.h"
#include "s_quests.h"
#include "level.h"
#include "room.h"
#include "monsters.h"
#include "states.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------
	
//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct QUEST_DATA_TRUTH_A
{
	int		nMonsterSageOld;
	int		nMonsterSageNew;
	int		nObjectTruthAEnterPortal;
	
	int		nTruthDemon;
	
	int		nLevelCGStation;
	int		nBrandonLann;
};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
QUEST_DATA_TRUTH_A *sQuestDataGet(
	QUEST *pQuest)
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return (QUEST_DATA_TRUTH_A *)pQuest->pQuestData;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sIsQuestClassNPC(
	QUEST * pQuest,
	UNIT * npc )
{
	QUEST_DATA_TRUTH_A * pQuestData = sQuestDataGet( pQuest );
	return UnitIsMonsterClass( npc, pQuestData->nBrandonLann );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sFilterStone(
	UNIT *pUnit)
{
	if (UnitHasExactState( pUnit, STATE_STONE ))
	{
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UNIT *sFindBossStatue(
	UNIT *pPlayer,
	QUEST_DATA_TRUTH_A *pQuestData)
{
	ASSERTX_RETNULL( pPlayer, "Expected player" );
	ROOM *pRoom = UnitGetRoom( pPlayer );

	// search for boss statue inside this room
	TARGET_TYPE eTargetTypes[] = { TARGET_BAD, TARGET_INVALID };
	return RoomFindFirstUnitOf( pRoom, eTargetTypes, GENUS_MONSTER, pQuestData->nTruthDemon, sFilterStone );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSpawnTruthPortal(
	QUEST *pQuest,
	UNIT *pKiller,
	UNIT *pVictim)
{
	ASSERTX_RETURN( pQuest, "Expected quest" );
	QUEST_DATA_TRUTH_A * pQuestData = sQuestDataGet( pQuest );

	int nObjectClass = pQuestData->nObjectTruthAEnterPortal;
	s_QuestSpawnObject( nObjectClass, pVictim, pKiller );
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageInteract( 
	QUEST *pQuest,
	const QUEST_MESSAGE_INTERACT *pMessage)
{
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );
	QUEST_DATA_TRUTH_A *ptQuestData = sQuestDataGet( pQuest );
	
	if ( UnitIsMonsterClass( pSubject, ptQuestData->nMonsterSageOld ) )
	{
	
		// force to this state in the quest
		s_QuestInactiveAdvanceTo( pQuest, QUEST_STATE_TRUTH_A_TALK_SAGE_OLD );		
		
		s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK, DIALOG_TRUTH_A_SAGE );
		return QMR_NPC_TALK;

	}

	if ( UnitIsMonsterClass( pSubject, ptQuestData->nMonsterSageNew ) )
	{
	
		// force to this state in the quest
		s_QuestInactiveAdvanceTo( pQuest, QUEST_STATE_TRUTH_A_TALK_SAGE_NEW );		
	
		if (QuestStateGetValue( pQuest, QUEST_STATE_TRUTH_A_TALK_SAGE_NEW ) == QUEST_STATE_ACTIVE )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK, DIALOG_TRUTH_A_SAGE_NEW );
			return QMR_NPC_TALK;
		}
		
	}

	if ( sIsQuestClassNPC( pQuest, pSubject ) && ( QuestStateGetValue( pQuest, QUEST_STATE_TRUTH_A_TALK_CLASS) == QUEST_STATE_ACTIVE ) )
	{
		s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_TRUTH_A_LANN );
		return QMR_NPC_TALK;
	}
	return QMR_OK;
			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageTalkStopped(
	QUEST *pQuest,
	const QUEST_MESSAGE_TALK_STOPPED *pMessage)
{
	UNIT *pPlayer = QuestGetPlayer( pQuest );
	int nDialogStopped = pMessage->nDialog;
	
	switch( nDialogStopped )
	{
	
		//----------------------------------------------------------------------------
		case DIALOG_TRUTH_A_SAGE:
		{
			QuestStateSet( pQuest, QUEST_STATE_TRUTH_A_TALK_SAGE_OLD, QUEST_STATE_COMPLETE );
			s_QuestTransportToNewTruthRoom( 
				pPlayer, 
				pQuest, 
				GI_MONSTER_TRUTH_A_SAGE_NEW, 
				DIALOG_TRUTH_A_SAGE_NEW);
			QuestStateSet( pQuest, QUEST_STATE_TRUTH_A_TALK_SAGE_NEW, QUEST_STATE_ACTIVE );
			break;			
		}

		//----------------------------------------------------------------------------
		case DIALOG_TRUTH_A_SAGE_NEW:
		{
			QuestStateSet( pQuest, QUEST_STATE_TRUTH_A_TALK_SAGE_NEW, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_TRUTH_A_TRAVEL_CGS, QUEST_STATE_ACTIVE );		
			break;
		}
		
		//----------------------------------------------------------------------------
		case DIALOG_TRUTH_A_LANN:
		case DIALOG_TRUTH_A_ALAY:
		case DIALOG_TRUTH_A_NASIM:
		{
			QuestStateSet( pQuest, QUEST_STATE_TRUTH_A_TALK_CLASS, QUEST_STATE_COMPLETE );
			s_QuestComplete( pQuest );
			// auto complete end of beta quest if needed
			s_QuestDemoEndForceComplete( pQuest->pPlayer );
			break;
		}
		
	}
	return QMR_OK;
		
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageUnitHasInfo( 
	QUEST *pQuest,
	const QUEST_MESSAGE_INTERACT_INFO *pMessage)
{
	QUEST_DATA_TRUTH_A *ptQuestData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );

	if ( QuestIsInactive( pQuest ) && s_QuestIsPrereqComplete( pQuest ) )
	{
		if ( UnitIsMonsterClass( pSubject, ptQuestData->nMonsterSageOld ) )
			return QMR_NPC_STORY_NEW;

		if ( UnitIsMonsterClass( pSubject, ptQuestData->nMonsterSageNew ) )
			return QMR_NPC_STORY_NEW;
	}
	
	if (QuestIsActive( pQuest ))
	{
		if (UnitIsMonsterClass( pSubject, ptQuestData->nMonsterSageOld ))
		{
			if (QuestStateGetValue( pQuest, QUEST_STATE_TRUTH_A_TALK_SAGE_OLD ) == QUEST_STATE_ACTIVE)
			{
				return QMR_NPC_STORY_RETURN;
			}
		}
		
		if ( sIsQuestClassNPC( pQuest, pSubject ) )
		{
			if ( QuestStateGetValue( pQuest, QUEST_STATE_TRUTH_A_TALK_CLASS ) == QUEST_STATE_ACTIVE )
			{
				return QMR_NPC_STORY_RETURN;
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
	QUEST_DATA_TRUTH_A * pData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pVictim = UnitGetById( pGame, pMessage->idVictim );
	UNIT *pKiller = UnitGetById( pGame, pMessage->idKiller );

	// turn taint into interactive NPC
	if (UnitIsMonsterClass( pVictim, pData->nTruthDemon ))
	{
	
		// activate this quest if not active
		s_QuestActivate( pQuest );

		// skip the starting state and advance straight to use the obelisk
		s_QuestAdvanceTo( pQuest, QUEST_STATE_TRUTH_A_ENTER_PORTAL );

		// spawn portal to the truth room
		sSpawnTruthPortal( pQuest, pKiller, pVictim );
		
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
		return QMR_OK;

	QUEST_DATA_TRUTH_A * pQuestData = sQuestDataGet( pQuest );
	if ( pMessage->nLevelNewDef == pQuestData->nLevelCGStation )
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_TRUTH_A_TRAVEL_CGS ) == QUEST_STATE_ACTIVE )
		{
			QuestStateSet( pQuest, QUEST_STATE_TRUTH_A_TRAVEL_CGS, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_TRUTH_A_TALK_CLASS, QUEST_STATE_ACTIVE );
		}
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
struct FIND_BOSS_DATA
{
	int nMonsterClass;
	BOOL bBossActive;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static ROOM_ITERATE_UNITS sFindActiveBoss(
	UNIT *pUnit, 
	void *pCallbackData)
{
	FIND_BOSS_DATA *pFindBossData = (FIND_BOSS_DATA *)pCallbackData;
	if (UnitIsMonsterClass( pUnit, pFindBossData->nMonsterClass ))
	{
		if (IsUnitDeadOrDying( pUnit ) == FALSE)
		{
			if (UnitHasExactState( pUnit, STATE_STONE ) == FALSE)
			{
				pFindBossData->bBossActive = TRUE;
				return RIU_STOP;
			}
		}
	}
	return RIU_CONTINUE;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sBossIsActive(
	QUEST *pQuest)
{
	ASSERTX_RETFALSE( pQuest, "Expected quest" );
	QUEST_DATA_TRUTH_A *pQuestData = sQuestDataGet( pQuest );	
	UNIT *pPlayer = QuestGetPlayer( pQuest );
	LEVEL *pLevel = UnitGetLevel( pPlayer );

	// setup callback data
	FIND_BOSS_DATA tCallbackData;
	tCallbackData.bBossActive = FALSE;
	tCallbackData.nMonsterClass = pQuestData->nTruthDemon;
		
	// search units in level
	TARGET_TYPE eTargetTypes[] = { TARGET_BAD, TARGET_INVALID };
	BOOL bBossActive = FALSE;
	LevelIterateUnits( pLevel, eTargetTypes, sFindActiveBoss, &tCallbackData );
	return bBossActive;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessagePlayerApproach(
	QUEST *pQuest,
	const QUEST_MESSAGE_PLAYER_APPROACH *pMessage )
{

	// check for truth demon
	UNIT *pTarget = UnitGetById( QuestGetGame( pQuest ), pMessage->idTarget );
	QUEST_DATA_TRUTH_A *pQuestData = sQuestDataGet( pQuest );	
	if (UnitIsMonsterClass( pTarget, pQuestData->nTruthDemon ))
	{

		// only care about stone demons
		if (UnitHasExactState( pTarget, STATE_STONE ) && sBossIsActive( pQuest ) == FALSE)
		{

			// can we awaken this boss
			BOOL bAwaken = FALSE;
			if (QuestIsInactive( pQuest ))
			{
				if (s_QuestIsPrereqComplete( pQuest ))
				{
					bAwaken = TRUE;
				}
			}
			else if (QuestStateGetValue( pQuest, QUEST_STATE_TRUTH_A_SEARCH ) == QUEST_STATE_ACTIVE)
			{
				bAwaken = TRUE;
			}
				
			// awaken
			if (bAwaken)
			{

				// come to life
				s_StateClear( pTarget, UnitGetId( pTarget ), STATE_STONE, 0 );
				s_StateSet( pTarget, pTarget, STATE_STONE_AWAKENING_FAST, 0 );
				s_StateSet( pTarget, pTarget, STATE_MUSEUM_BOSS, 0 );
				
				// activate the the quest if needed
				if (QuestIsInactive( pQuest ))
				{
					s_QuestActivate( pQuest );
				}

				// advance quest to survive state
				s_QuestAdvanceTo( pQuest, QUEST_STATE_TRUTH_A_SURVIVE );
										
			}

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
		case QM_SERVER_INTERACT_INFO:
		{
			const QUEST_MESSAGE_INTERACT_INFO *pHasInfoMessage = (const QUEST_MESSAGE_INTERACT_INFO *)pMessage;
			return sQuestMessageUnitHasInfo( pQuest, pHasInfoMessage );
		}
				
		//----------------------------------------------------------------------------
		case QM_SERVER_MONSTER_KILL:
		{
			const QUEST_MESSAGE_MONSTER_KILL *pMonsterKillMessage = (const QUEST_MESSAGE_MONSTER_KILL *)pMessage;
			return sQuestMessageMonsterKill( pQuest, pMonsterKillMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_ENTER_LEVEL:
		{
			const QUEST_MESSAGE_ENTER_LEVEL * pTransitionMessage = ( const QUEST_MESSAGE_ENTER_LEVEL * )pMessage;
			return sQuestMessageTransitionLevel( pQuest, pTransitionMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_PLAYER_APPROACH:
		{
			const QUEST_MESSAGE_PLAYER_APPROACH *pPlayerApproachMessage = (const QUEST_MESSAGE_PLAYER_APPROACH *)pMessage;
			return sQuestMessagePlayerApproach( pQuest, pPlayerApproachMessage );
		}

	}

	return QMR_OK;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestFreeTruthA(
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
void QuestInitTruthA(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	
	
	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		

	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_TRUTH_A *pQuestData = (QUEST_DATA_TRUTH_A *)GMALLOCZ( pGame, sizeof( QUEST_DATA_TRUTH_A ) );
	pQuest->pQuestData = pQuestData;
	
	pQuestData->nMonsterSageOld				= QuestUseGI( pQuest, GI_MONSTER_TRUTH_A_SAGE_OLD );
	pQuestData->nMonsterSageNew				= QuestUseGI( pQuest, GI_MONSTER_TRUTH_A_SAGE_NEW );	
	pQuestData->nObjectTruthAEnterPortal	= QuestUseGI( pQuest, GI_OBJECT_TRUTH_A_OLD_ENTER );
	pQuestData->nTruthDemon					= QuestUseGI( pQuest, GI_MONSTER_TRUTH_DEMON );
	pQuestData->nLevelCGStation				= QuestUseGI( pQuest, GI_LEVEL_COVENT_GARDEN_STATION );
	pQuestData->nBrandonLann				= QuestUseGI( pQuest, GI_MONSTER_BRANDON_LANN );
				
	// add additional cast members
	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nMonsterSageOld );
	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nMonsterSageNew );
	
	// quest message flags
	pQuest->dwQuestMessageFlags |= MAKE_MASK( QSMF_APPROACH_RADIUS );
}
