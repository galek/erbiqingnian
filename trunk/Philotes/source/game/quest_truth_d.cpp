//----------------------------------------------------------------------------
// FILE: quest_truth_d.cpp
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
#include "quest_truth_D.h"
#include "quests.h"
#include "dialog.h"
#include "unit_priv.h" // also includes units.h
#include "objects.h"
#include "level.h"
#include "s_quests.h"
#include "stringtable.h"
#include "monsters.h"
#include "offer.h"
#include "quest_triage.h"
#include "room_path.h"

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct QUEST_DATA_TRUTH_D
{
	int		nLordArphaun;
	int		nBrandonLann;
	int		nAeronAltair;
	int		nTechsmith99;

	int		nLevelExodus;
	int		nLevelTemplarBase;
	int		nLevelMonumentStation;

	int		nBoss;

	int		nSageOld;
	int		nSageNew;

	int		nTruthEnterPortal;
	int		nTruthSpawnMarker;
};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_TRUTH_D * sQuestDataGet( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return (QUEST_DATA_TRUTH_D *)pQuest->pQuestData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static UNIT * sFindTruthSpawnLocation( QUEST * pQuest )
{
	// check the level for a boss already
	UNIT * pPlayer = pQuest->pPlayer;
	LEVEL * pLevel = UnitGetLevel( pPlayer );
	TARGET_TYPE eTargetTypes[] = { TARGET_OBJECT, TARGET_INVALID };
	QUEST_DATA_TRUTH_D * pQuestData = sQuestDataGet( pQuest );
	return LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_OBJECT, pQuestData->nTruthSpawnMarker );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sSpawnBoss( QUEST * pQuest )
{
	// check the level for a boss already
	UNIT * pPlayer = pQuest->pPlayer;
	LEVEL * pLevel = UnitGetLevel( pPlayer );
	TARGET_TYPE eTargetTypes[] = { TARGET_BAD, TARGET_INVALID };
	QUEST_DATA_TRUTH_D * pQuestData = sQuestDataGet( pQuest );
	if ( LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_MONSTER, pQuestData->nBoss ) )
	{
		return;
	}

	UNIT * pMarker = sFindTruthSpawnLocation( pQuest );
	ASSERTX_RETURN( pMarker, "No place to spawn truth boss and event found" );

	GAME * pGame = UnitGetGame( pMarker );
	ROOM * pRoom = UnitGetRoom( pMarker );
	ROOM * pDestRoom;
	ROOM_PATH_NODE * pNode = RoomGetNearestPathNode( pGame, pRoom, UnitGetPosition( pMarker ), &pDestRoom );

	if ( !pNode )
		return;

	// init location
	SPAWN_LOCATION tLocation;
	SpawnLocationInit( &tLocation, pDestRoom, pNode->nIndex );

	MONSTER_SPEC tSpec;
	tSpec.nClass = pQuestData->nBoss;
	tSpec.nExperienceLevel = QuestGetMonsterLevel( pQuest, pLevel, tSpec.nClass, 25 );
	tSpec.pRoom = pDestRoom;
	tSpec.vPosition = tLocation.vSpawnPosition;
	tSpec.vDirection = VECTOR( 0.0f, 0.0f, 0.0f );
	tSpec.dwFlags = MSF_DONT_DEACTIVATE_WITH_ROOM | MSF_DONT_DEPOPULATE;
	s_MonsterSpawn( pGame, tSpec );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageUnitHasInfo( 
	QUEST *pQuest,
	const QUEST_MESSAGE_INTERACT_INFO *pMessage)
{
	QUEST_STATUS eStatus = QuestGetStatus( pQuest );
	if ( eStatus > QUEST_STATUS_ACTIVE )
		return QMR_OK;

	QUEST_DATA_TRUTH_D * pQuestData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );

	if ( eStatus == QUEST_STATUS_INACTIVE )
	{
		if ( UnitIsMonsterClass( pSubject, pQuestData->nTechsmith99 ) )
		{
			return QMR_NPC_STORY_NEW;
		}
		return QMR_OK;
	}

	if ( eStatus != QUEST_STATUS_ACTIVE )
		return QMR_OK;

	// quest active
	if (UnitIsMonsterClass( pSubject, pQuestData->nSageOld))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_TRUTH_D_TALK_BROTHERS ) == QUEST_STATE_ACTIVE )
		{
			return QMR_NPC_STORY_RETURN;
		}
		return QMR_OK;
	}

	if (UnitIsMonsterClass( pSubject, pQuestData->nSageNew))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_TRUTH_D_TALK_BROTHERS2 ) == QUEST_STATE_ACTIVE )
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

	// don't do this cause you can abandon the quest in the truth room
	//if ( pQuest->eStatus != QUEST_STATUS_ACTIVE )
	//{
	//	return QMR_OK;
	//}

	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pPlayer = UnitGetById( pGame, pMessage->idPlayer );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );
	ASSERTX_RETVAL( pPlayer, QMR_OK, "Invalid player" );
	ASSERTX_RETVAL( pSubject, QMR_OK, "Invalid NPC" );	
	QUEST_DATA_TRUTH_D * pQuestData = sQuestDataGet( pQuest );

	if ( UnitIsMonsterClass( pSubject, pQuestData->nTechsmith99 ) )
	{
		if ( QuestGetStatus( pQuest ) == QUEST_STATUS_INACTIVE )
		{
			s_QuestTruthDBegin( pQuest );
			return QMR_FINISHED;
		}
		return QMR_OK;
	}

	if (UnitIsMonsterClass( pSubject, pQuestData->nSageOld ))
	{
		
		// force to this state in the quest
		s_QuestInactiveAdvanceTo( pQuest, QUEST_STATE_TRUTH_D_TALK_BROTHERS );		

		s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_TRUTH_D_BROTHERS, INVALID_LINK, GI_MONSTER_TRUTH_D_SAGE_OLD );
		return QMR_NPC_TALK;

	}
	if (UnitIsMonsterClass( pSubject, pQuestData->nSageNew ))
	{
	
		// force to this state in the quest
		s_QuestInactiveAdvanceTo( pQuest, QUEST_STATE_TRUTH_D_TALK_BROTHERS2 );		
	
		if ( QuestStateGetValue( pQuest, QUEST_STATE_TRUTH_D_TALK_BROTHERS2 ) == QUEST_STATE_ACTIVE )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_TRUTH_D_BROTHERS2, INVALID_LINK, GI_MONSTER_TRUTH_D_SAGE_NEW );
			return QMR_NPC_TALK;
		}
		return QMR_OK;
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sVideoComplete( QUEST * pQuest )
{
	QuestStateSet( pQuest, QUEST_STATE_TRUTH_D_VIDEO, QUEST_STATE_COMPLETE );
	s_QuestComplete( pQuest );
	QUEST * pTriage = QuestGetQuest( pQuest->pPlayer, QUEST_TRIAGE );
	s_QuestTriageBegin( pTriage );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageTalkStopped(
	QUEST *pQuest,
	const QUEST_MESSAGE_TALK_STOPPED *pMessage )
{
	int nDialogStopped = pMessage->nDialog;
	
	switch( nDialogStopped )
	{
		//----------------------------------------------------------------------------
		case DIALOG_TRUTH_D_BROTHERS:
		{
			QuestStateSet( pQuest, QUEST_STATE_TRUTH_D_TALK_BROTHERS, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_TRUTH_D_TALK_BROTHERS2, QUEST_STATE_ACTIVE );
			
			// transport to new truth room
			UNIT *pPlayer = QuestGetPlayer( pQuest );
			s_QuestTransportToNewTruthRoom( pPlayer );
			
			break;
		}

		//----------------------------------------------------------------------------
		case DIALOG_TRUTH_D_BROTHERS2:
		{
			QuestStateSet( pQuest, QUEST_STATE_TRUTH_D_TALK_BROTHERS2, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_TRUTH_D_VIDEO, QUEST_STATE_ACTIVE );
			break;
		}

		//----------------------------------------------------------------------------
		case DIALOG_TRUTH_D_VIDEO:
		{
			sVideoComplete( pQuest );
			break;
		}
	}
	return QMR_OK;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageTalkCancelled(
	QUEST *pQuest,
	const QUEST_MESSAGE_TALK_CANCELLED *pMessage )
{
	if ( !QuestIsActive( pQuest ) )
		return QMR_OK;

	if ( pMessage->nDialog == DIALOG_TRUTH_D_VIDEO )
	{
		sVideoComplete( pQuest );
		return QMR_FINISHED;
	}
	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageMonsterKill( 
	QUEST *pQuest,
	const QUEST_MESSAGE_MONSTER_KILL *pMessage)
{
	QUEST_DATA_TRUTH_D * pQuestData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pVictim = UnitGetById( pGame, pMessage->idVictim );
	UNIT *pKiller = UnitGetById( pGame, pMessage->idKiller );
		
	if ( UnitIsMonsterClass( pVictim, pQuestData->nBoss ) )
	{
		if (QuestStateGetValue( pQuest, QUEST_STATE_TRUTH_D_KILL_BOSS ) == QUEST_STATE_ACTIVE)
		{
			QuestStateSet( pQuest, QUEST_STATE_TRUTH_D_KILL_BOSS, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_TRUTH_D_TALK_BROTHERS, QUEST_STATE_ACTIVE );
			s_QuestSpawnObject( pQuestData->nTruthEnterPortal, pVictim, pKiller );		
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
	{
		s_QuestTruthDBegin( pQuest );
		return QMR_OK;
	}

	UNIT * player = pQuest->pPlayer;
	GAME * game = UnitGetGame( player );
	ASSERT_RETVAL( IS_SERVER( game ), QMR_OK );

	QUEST_DATA_TRUTH_D * pQuestData = sQuestDataGet( pQuest );

	if ( pMessage->nLevelNewDef == pQuestData->nLevelExodus )
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_TRUTH_D_KILL_BOSS ) == QUEST_STATE_ACTIVE )
		{
			sSpawnBoss( pQuest );
			return QMR_OK;
		}
		else
		{
			// find spawn location unit
			UNIT * pLocation = sFindTruthSpawnLocation( pQuest );
			s_QuestSpawnObject( pQuestData->nTruthEnterPortal, pLocation, pQuest->pPlayer );
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
	if ( !QuestIsActive( pQuest ) )
		return QMR_OK;

	QUEST_DATA_TRUTH_D *ptQuestData = sQuestDataGet( pQuest );

	UNIT *pPlayer = QuestGetPlayer( pQuest );
	LEVEL *pLevel = UnitGetLevel( pPlayer );
	if (ptQuestData->nLevelExodus == LevelGetDefinitionIndex( pLevel ))
	{
		// exiting hellrift
		SUBLEVEL *pSubLevelOld = pMessage->pSubLevelOld;
		if (pSubLevelOld && SubLevelGetType( pSubLevelOld ) == ST_TRUTH_D_NEW)
		{
			// mark all old states as completed
			QuestStateSet( pQuest, QUEST_STATE_TRUTH_D_KILL_BOSS, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_TRUTH_D_TALK_BROTHERS, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_TRUTH_D_TALK_BROTHERS2, QUEST_STATE_COMPLETE );
			// finishing video
			s_NPCVideoStart( UnitGetGame( pPlayer ), pPlayer, GI_MONSTER_LORD_ARPHAUN, NPC_VIDEO_INSTANT_INCOMING, DIALOG_TRUTH_D_VIDEO );
			return QMR_OK;
		}
	}
	return QMR_OK;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageIsUnitVisible( 
	QUEST *pQuest,
	const QUEST_MESSAGE_IS_UNIT_VISIBLE * pMessage)
{
	QUEST_DATA_TRUTH_D * pQuestData = sQuestDataGet( pQuest );
	UNIT * pUnit = pMessage->pUnit;

	if ( UnitGetGenus( pUnit ) != GENUS_MONSTER )
		return QMR_OK;

	int nUnitClass = UnitGetClass( pUnit );
	int nLevelIndex = UnitGetLevelDefinitionIndex( pUnit );
	QUEST_STATUS eStatus = QuestGetStatus( pQuest );

	if ( nUnitClass == pQuestData->nLordArphaun )
	{
		if ( nLevelIndex == pQuestData->nLevelTemplarBase )
		{
			if ( eStatus >= QUEST_STATUS_COMPLETE )
				return QMR_UNIT_VISIBLE;
			else
				return QMR_UNIT_HIDDEN;
		}
		else if ( nLevelIndex == pQuestData->nLevelMonumentStation )
		{
			if ( eStatus >= QUEST_STATUS_COMPLETE )
				return QMR_UNIT_HIDDEN;
			else
				return QMR_UNIT_VISIBLE;
		}
	}

/*	if ( ( nUnitClass == pQuestData->nBrandonLann ) || ( nUnitClass == pQuestData->nAeronAltair ) )
	{
		if ( nLevelIndex == pQuestData->nLevelTemplarBase )
		{
			if ( eStatus >= QUEST_STATUS_COMPLETE )
				return QMR_UNIT_VISIBLE;
			else
				return QMR_UNIT_HIDDEN;
		}
	}*/

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
		case QM_SERVER_TALK_CANCELLED:
		{
			const QUEST_MESSAGE_TALK_CANCELLED *pTalkCancelledMessage = (const QUEST_MESSAGE_TALK_CANCELLED *)pMessage;
			return sQuestMessageTalkCancelled( pQuest, pTalkCancelledMessage );
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
		case QM_SERVER_SUB_LEVEL_TRANSITION:
		{
			const QUEST_MESSAGE_SUBLEVEL_TRANSITION *pSubLevelMessage = (const QUEST_MESSAGE_SUBLEVEL_TRANSITION *)pMessage;
			return sQuestMessageSubLevelTransition( pQuest, pSubLevelMessage );
		}

		//----------------------------------------------------------------------------
		case QM_IS_UNIT_VISIBLE:
		{
			const QUEST_MESSAGE_IS_UNIT_VISIBLE *pVisibleMessage = (const QUEST_MESSAGE_IS_UNIT_VISIBLE *)pMessage;
			return sQuestMessageIsUnitVisible( pQuest, pVisibleMessage );
		}
	}

	return QMR_OK;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestFreeTruthD(
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
	QUEST_DATA_TRUTH_D * pQuestData)
{
	pQuestData->nLordArphaun				= QuestUseGI( pQuest, GI_MONSTER_LORD_ARPHAUN );
	pQuestData->nAeronAltair				= QuestUseGI( pQuest, GI_MONSTER_AERON_ALTAIR );
	pQuestData->nBrandonLann				= QuestUseGI( pQuest, GI_MONSTER_BRANDON_LANN );
	pQuestData->nBoss						= QuestUseGI( pQuest, GI_MONSTER_TRUTH_D_BOSS );
	pQuestData->nTruthEnterPortal			= QuestUseGI( pQuest, GI_OBJECT_TRUTH_D_OLD_ENTER );
	pQuestData->nLevelExodus				= QuestUseGI( pQuest, GI_LEVEL_EXODUS );
	pQuestData->nLevelTemplarBase			= QuestUseGI( pQuest, GI_LEVEL_TEMPLAR_BASE );
	pQuestData->nLevelMonumentStation		= QuestUseGI( pQuest, GI_LEVEL_MONUMENT_STATION );
	pQuestData->nSageOld					= QuestUseGI( pQuest, GI_MONSTER_TRUTH_D_SAGE_OLD );
	pQuestData->nSageNew					= QuestUseGI( pQuest, GI_MONSTER_TRUTH_D_SAGE_NEW );
	pQuestData->nTruthSpawnMarker			= QuestUseGI( pQuest, GI_OBJECT_TRUTH_SPAWN_MARKER );
	pQuestData->nTechsmith99				= QuestUseGI( pQuest, GI_MONSTER_TECHSMITH_99 );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestInitTruthD(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	
	
	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		
			
	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_TRUTH_D * pQuestData = ( QUEST_DATA_TRUTH_D * )GMALLOCZ( pGame, sizeof( QUEST_DATA_TRUTH_D ) );
	sQuestDataInit( pQuest, pQuestData );
	pQuest->pQuestData = pQuestData;

	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nTechsmith99 );

	// quest message flags
	pQuest->dwQuestMessageFlags |= MAKE_MASK( QSMF_BACK_FROM_MOVIE ) | MAKE_MASK( QSMF_IS_UNIT_VISIBLE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
/*
void s_QuestRestoreTruthD(
	QUEST *pQuest,
	UNIT *pSetupPlayer)
{
}
*/

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void s_QuestTruthDBegin( QUEST * pQuest )
{
	ASSERTX_RETURN( IS_SERVER( UnitGetGame( pQuest->pPlayer ) ), "Server Only" );
	ASSERTX_RETURN( pQuest->nQuest == QUEST_TRUTH_D, "Wrong quest sent to function. Need Truth D" );

	if ( QuestIsComplete( pQuest ) )
		return;

	if ( !s_QuestIsPrereqComplete( pQuest ) )
		return;

	s_QuestActivate( pQuest );
	sSpawnBoss( pQuest );
}
