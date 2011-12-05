//----------------------------------------------------------------------------
// FILE: quest_dark.cpp
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
#include "quest_dark.h"
#include "quests.h"
#include "dialog.h"
#include "unit_priv.h" // also includes units.h
#include "objects.h"
#include "level.h"
#include "stringtable.h"
#include "ai.h"
#include "monsters.h"
#include "room_layout.h"
#include "s_quests.h"
#include "s_trade.h"
#include "offer.h"

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct QUEST_DATA_DARK
{
	int		nDarkTemplar;
	int		nDarkTech1;
	int		nDarkTech2;
	int		nDarkTechAI;
	int		nDarkDeadTech;
	int		nDarkDRLG;
	int		nLightDRLG;
	int		nEndObject;
	int		nReturnPortal;
	int		nWarpToDark;
};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_DARK * sQuestDataGet( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return (QUEST_DATA_DARK *)pQuest->pQuestData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sStartDark( QUEST * pQuest )
{
	ASSERT_RETURN( pQuest );
	QUEST_DATA_DARK * pQuestData = sQuestDataGet( pQuest );
	REF(pQuestData);
	ASSERT_RETURN( IS_SERVER( UnitGetGame( pQuest->pPlayer ) ) );

	// activate quest
	if ( s_QuestActivate( pQuest ) )
	{
		// add additional activation code if needed
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDarkUpdateFind3( QUEST * pQuest )
{
	// mark quest for everyone
	if ( QuestStateGetValue( pQuest, QUEST_STATE_DARK_TECH1_FOUND ) == QUEST_STATE_ACTIVE )
	{
		s_QuestStateSetForAll( pQuest, QUEST_STATE_DARK_TECH1_FOUND, QUEST_STATE_COMPLETE );
		s_QuestStateSetForAll( pQuest, QUEST_STATE_DARK_TECH2_FOUND, QUEST_STATE_ACTIVE );
	}
	else if ( QuestStateGetValue( pQuest, QUEST_STATE_DARK_TECH2_FOUND ) == QUEST_STATE_ACTIVE )
	{
		s_QuestStateSetForAll( pQuest, QUEST_STATE_DARK_TECH2_FOUND, QUEST_STATE_COMPLETE );
	}
	else
	{
		s_QuestStateSetForAll( pQuest, QUEST_STATE_DARK_FIND_TECHS, QUEST_STATE_COMPLETE );
		s_QuestStateSetForAll( pQuest, QUEST_STATE_DARK_RETURN, QUEST_STATE_ACTIVE );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sDarkChangeTechAI( QUEST * pQuest, UNIT * npc, int nAI, BOOL bClear )
{
	QUEST_DATA_DARK * pQuestData = sQuestDataGet( pQuest );
	UNIT * player = pQuest->pPlayer;
	ASSERT_RETURN( player );
	GAME * game = UnitGetGame( player );
	ASSERT_RETURN( game );
	ASSERT_RETURN( npc );

	ASSERT_RETURN( ( UnitGetClass( npc ) == pQuestData->nDarkTech1 ) || ( UnitGetClass( npc ) == pQuestData->nDarkTech2 ) );

	int ai = UnitGetStat( npc, STATS_AI );
	if ( ai == nAI )
		return;

	// found!
	AI_Free( game, npc );
	UnitSetStat( npc, STATS_AI, nAI );
	if ( bClear )
		UnitSetAITarget(npc, INVALID_ID, TRUE);
	else
		UnitSetAITarget(npc, player, TRUE);
	AI_Init( game, npc );

	sDarkUpdateFind3( pQuest );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define MAX_DARKTECH_LOCATIONS				32

static void sDarkCreateTechs(
	GAME *pGame,
	QUEST * pQuest,
	LEVEL *pLevel)
{
	ASSERTX_RETURN( pLevel, "Expected level" );

	QUEST_DATA_DARK * pQuestData = sQuestDataGet( pQuest );

	VECTOR vPosition[ MAX_DARKTECH_LOCATIONS ];
	VECTOR vDirection[ MAX_DARKTECH_LOCATIONS ];
	ROOM * pRoomList[ MAX_DARKTECH_LOCATIONS ];
	int nNumLocations = 0;
		
	for ( ROOM * room = LevelGetFirstRoom( pLevel ); room; room = LevelGetNextRoom( room ) )
	{
		// check to see if spawned
		for ( UNIT * test = room->tUnitList.ppFirst[TARGET_GOOD]; test; test = test->tRoomNode.pNext )
		{
			if ( UnitGetClass(test) == pQuestData->nDarkTech1 )
			{
				// found one already
				return;
			}
		}

		// save the nodes
		if ( nNumLocations < MAX_DARKTECH_LOCATIONS )
		{
			ROOM_LAYOUT_SELECTION_ORIENTATION * pOrientationNodeSet = NULL;
			ROOM_LAYOUT_GROUP * nodeset = RoomGetLabelNode( room, "Techs", &pOrientationNodeSet );
			if ( nodeset && pOrientationNodeSet )
			{
				for ( int i = 0; i < nodeset->nGroupCount; i++ )
				{
					ROOM_LAYOUT_SELECTION_ORIENTATION * pOrientationNode = NULL;
					ROOM_LAYOUT_GROUP * node = s_QuestNodeSetGetNode( room, nodeset, &pOrientationNode, i+1 );
					ASSERT_RETURN( node );
					ASSERT_RETURN( pOrientationNode );
					s_QuestNodeGetPosAndDir( room, pOrientationNode, &vPosition[ nNumLocations ], &vDirection[ nNumLocations ], NULL, NULL );
					pRoomList[ nNumLocations ] = room;
					nNumLocations++;
				}
			}
		}
	}

	ASSERT( nNumLocations >= 3 );

	for ( int i = 0; i < 3; i++ )
	{
		// choose a location
		int index = RandGetNum( pGame->rand, nNumLocations );

		// and spawn the techs
		MONSTER_SPEC tSpec;
		if ( i == 0 )
			tSpec.nClass = pQuestData->nDarkDeadTech;
		else if ( i == 1 )
			tSpec.nClass = pQuestData->nDarkTech1;
		else
			tSpec.nClass = pQuestData->nDarkTech2;
		tSpec.nExperienceLevel = QuestGetMonsterLevel( pQuest, pLevel, tSpec.nClass, 1 );
		tSpec.pRoom = pRoomList[ index ];
		tSpec.vPosition = vPosition[ index ];
		tSpec.vDirection = VECTOR( 0.0f, 0.0f, 0.0f );
		tSpec.dwFlags = MSF_DONT_DEACTIVATE_WITH_ROOM | MSF_DONT_DEPOPULATE;
		s_MonsterSpawn( pGame, tSpec );

		// remove/replace in array
		nNumLocations--;
		pRoomList[index] = pRoomList[nNumLocations];
		vPosition[index] = vPosition[nNumLocations];
		vDirection[index] = vDirection[nNumLocations];
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sSpawnEndObject( QUEST * pQuest )
{
	ASSERT_RETURN( pQuest );
	QUEST_DATA_DARK * pQuestData = sQuestDataGet( pQuest );
	GAME * game = UnitGetGame( pQuest->pPlayer );
	ASSERT_RETURN( IS_SERVER( game ) );

	// create the end object...
	LEVEL * level = UnitGetLevel( pQuest->pPlayer );
	UNIT * next_portal = NULL;
	ROOM * portal_room = NULL;
	for ( ROOM * room = LevelGetFirstRoom(level); room; room = LevelGetNextRoom(room) )
	{
		for ( UNIT * test = room->tUnitList.ppFirst[TARGET_OBJECT]; test; test = test->tRoomNode.pNext )
		{
			int test_class = UnitGetClass( test );
			if ( pQuestData->nEndObject == test_class )
			{
				// already exists!
				return;
			}
			if ( pQuestData->nReturnPortal == test_class )
			{
				next_portal = test;
				portal_room = room;
			}
		}
	}

	ASSERT_RETURN( next_portal );
	
	OBJECT_SPEC tSpec;
	tSpec.nClass = pQuestData->nEndObject;
	tSpec.pRoom = portal_room;
	tSpec.vPosition = next_portal->vPosition;
	tSpec.pvFaceDirection = &next_portal->vFaceDirection;
	s_ObjectSpawn( game, tSpec );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHasMonocle(
	void *pUserData)
{
	QUEST *pQuest = (QUEST *)pUserData;
	
	QuestStateSet( pQuest, QUEST_STATE_DARK_FIND_TEMPLAR, QUEST_STATE_COMPLETE );
	QuestStateSet( pQuest, QUEST_STATE_DARK_FIND_TECHS, QUEST_STATE_ACTIVE );
	QuestStateSet( pQuest, QUEST_STATE_DARK_TECH1_FOUND, QUEST_STATE_ACTIVE );
	QuestStateSet( pQuest, QUEST_STATE_DARK_TECH2_FOUND, QUEST_STATE_ACTIVE );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageInteract(
	QUEST *pQuest,
	const QUEST_MESSAGE_INTERACT *pMessage )
{
	QUEST_DATA_DARK * pData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pPlayer = UnitGetById( pGame, pMessage->idPlayer );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );
	ASSERTX_RETVAL( pPlayer, QMR_OK, "Invalid player" );
	ASSERTX_RETVAL( pSubject, QMR_OK, "Invalid NPC" );	
	
	if ( QuestGetStatus( pQuest ) == QUEST_STATUS_ACTIVE )
	{

		if (UnitIsMonsterClass( pSubject, pData->nDarkTemplar ))
		{
				
			if ( QuestStateGetValue( pQuest, QUEST_STATE_DARK_FIND_TEMPLAR ) == QUEST_STATE_ACTIVE )
			{
				s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK, DIALOG_DARK_FOUND, DIALOG_DARK_MONOCLE_REWARD );		
				return QMR_NPC_TALK;
			}
			if ( QuestStateGetValue( pQuest, QUEST_STATE_DARK_FIND_TECHS ) == QUEST_STATE_ACTIVE )
			{
				s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK, DIALOG_DARK_ACTIVE );
				return QMR_NPC_TALK;
			}
			if ( QuestStateGetValue( pQuest, QUEST_STATE_DARK_RETURN ) == QUEST_STATE_ACTIVE )
			{
				s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_DARK_COMPLETE, DIALOG_DARK_REWARD );
				return QMR_NPC_TALK;
			}
			
		}

		if (UnitIsMonsterClass( pSubject, pData->nDarkTech1 ) || 
			UnitIsMonsterClass( pSubject, pData->nDarkTech2 ))
		{
			if ( QuestStateGetValue( pQuest, QUEST_STATE_DARK_TECH2_FOUND ) == QUEST_STATE_ACTIVE )
				s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK, DIALOG_DARK_FIND_TECH2 );
			else
				s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK, DIALOG_DARK_FIND_TECH1 );
			return QMR_NPC_TALK;
		}

		if (UnitIsMonsterClass( pSubject, pData->nDarkDeadTech ))
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK, DIALOG_DARK_FIND_DEADTECH );
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
	QUEST_DATA_DARK * pQuestData = sQuestDataGet( pQuest );
	
	switch( nDialogStopped )
	{
		//----------------------------------------------------------------------------
		case DIALOG_DARK_INIT:
		{
			sStartDark( pQuest );
			break;
		}

		//----------------------------------------------------------------------------
		case DIALOG_DARK_FIND_TECH1:
		case DIALOG_DARK_FIND_TECH2:
		{
			UnitSetInteractive( pTalkingTo, UNIT_INTERACTIVE_RESTRICED );
			sDarkChangeTechAI( pQuest, pTalkingTo, pQuestData->nDarkTechAI, FALSE );
			break;
		}

		//----------------------------------------------------------------------------
		case DIALOG_DARK_FOUND:
		{
		
			// setup actions
			OFFER_ACTIONS tActions;
			tActions.pfnAllTaken = sHasMonocle;
			tActions.pAllTakenUserData = pQuest;
			
			// offer to player
			s_OfferGive( pQuest->pPlayer, pTalkingTo, OFFERID_QUEST_DARK_MONOCLES, tActions );
				
			return QMR_OFFERING;
			
		}
		
		//----------------------------------------------------------------------------
		case DIALOG_DARK_FIND_DEADTECH:
		{
			// turn off interaction
			UnitSetInteractive( pTalkingTo, UNIT_INTERACTIVE_RESTRICED );
			sDarkUpdateFind3( pQuest );
			break;
		}

		//----------------------------------------------------------------------------
		case DIALOG_DARK_COMPLETE:
		{
			// complete the quest
			QuestStateSet( pQuest, QUEST_STATE_DARK_RETURN, QUEST_STATE_COMPLETE );
			if ( s_QuestComplete( pQuest ) )
			{
			
				// offer reward
				if (s_QuestOfferReward( pQuest, pTalkingTo ) == TRUE)
				{
					return QMR_OFFERING;
				}
			}
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
	QUEST_STATUS eStatus = QuestGetStatus( pQuest );

	if ( eStatus == QUEST_STATUS_ACTIVE )
	{

		QUEST_DATA_DARK * pQuestData = sQuestDataGet( pQuest );
		GAME *pGame = QuestGetGame( pQuest );
		UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );

		if ( UnitIsMonsterClass( pSubject, pQuestData->nDarkTemplar ))
		{
			if ( QuestStateGetValue( pQuest, QUEST_STATE_DARK_FIND_TECHS ) == QUEST_STATE_ACTIVE )
			{
				return QMR_NPC_INFO_WAITING;
			}
			return QMR_NPC_INFO_RETURN;
		}

		UNIT *pPlayer = QuestGetPlayer( pQuest );
		if (UnitIsMonsterClass( pSubject, pQuestData->nDarkTech1 ) || 
			UnitIsMonsterClass( pSubject, pQuestData->nDarkTech2 ))
		{
			if ( ( QuestStateGetValue( pQuest, QUEST_STATE_DARK_FIND_TECHS ) == QUEST_STATE_ACTIVE ) && 
				   UnitCanInteractWith( pSubject, pPlayer ) )
			{
				return QMR_NPC_INFO_RETURN;				
			}
			return QMR_OK;
		}

		if (UnitIsMonsterClass( pSubject, pQuestData->nDarkDeadTech ))
		{
			if ( ( QuestStateGetValue( pQuest, QUEST_STATE_DARK_FIND_TECHS ) == QUEST_STATE_ACTIVE ) && 
				   UnitCanInteractWith( pSubject, pPlayer ) )
			{
				return QMR_NPC_INFO_RETURN;
			}
			return QMR_OK;
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
	if ( AppIsDemo() )
		return QMR_OK;

	UNIT * player = pQuest->pPlayer;
	GAME * game = UnitGetGame( player );
	ASSERT_RETVAL( IS_SERVER( game ), QMR_OK );

	QUEST_DATA_DARK * pQuestData = sQuestDataGet( pQuest );
	if ( pMessage->nLevelNewDRLGDef == pQuestData->nDarkDRLG )
	{
		if ( !QuestIsActive( pQuest ) )
			return QMR_OK;

		// spawn the techs
		LEVEL *pLevel = UnitGetLevel( player );
		sDarkCreateTechs( game, pQuest, pLevel );
	}

	if ( pMessage->nLevelNewDRLGDef == pQuestData->nLightDRLG )
	{
		if ( QuestGetStatus( pQuest ) == QUEST_STATUS_INACTIVE )
		{
			// check the level for warp to the dark
			UNIT * pPlayer = pQuest->pPlayer;
			LEVEL * pLevel = UnitGetLevel( pPlayer );
			TARGET_TYPE eTargetTypes[] = { TARGET_OBJECT, TARGET_INVALID };
			UNIT * warp =  LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_OBJECT, pQuestData->nWarpToDark );
			if ( warp )
			{
				// send video
				s_NPCVideoStart( game, player, GI_MONSTER_DARK_TEMPLAR, NPC_VIDEO_INCOMING, DIALOG_DARK_INIT );
			}
		}
	}

	return QMR_OK;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestCanRunDRLGRule( 
	QUEST * pQuest,
	const QUEST_MESSAGE_CAN_RUN_DRLG_RULE * pMessage)
{
	QUEST_DATA_DARK * pData = sQuestDataGet( pQuest );

	const LEVEL_DEFINITION * pLevelDef = LevelDefinitionGet( pMessage->nLevelDefId );
	if ( pLevelDef->nSequenceNumber < 10 )
	{
		// dont run in act 1
		return QMR_DRLG_RULE_FORBIDDEN;
	}

	if ( ( pData->nLightDRLG == pMessage->nDRLGDefId ) && ( QuestGetStatus( pQuest ) >= QUEST_STATUS_COMPLETE ) )
	{
		return QMR_DRLG_RULE_FORBIDDEN;
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
		case QM_SERVER_CAN_RUN_DRLG_RULE:
		{
			const QUEST_MESSAGE_CAN_RUN_DRLG_RULE * pRuleData = (const QUEST_MESSAGE_CAN_RUN_DRLG_RULE *)pMessage;
			return sQuestCanRunDRLGRule( pQuest, pRuleData );
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
void QuestFreeDark(
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
	QUEST_DATA_DARK * pQuestData)
{
	pQuestData->nDarkTemplar	= QuestUseGI( pQuest, GI_MONSTER_DARK_TEMPLAR );
	pQuestData->nDarkTech1		= QuestUseGI( pQuest, GI_MONSTER_DARK_TECHSMITH1 );
	pQuestData->nDarkTech2		= QuestUseGI( pQuest, GI_MONSTER_DARK_TECHSMITH2 );
	pQuestData->nDarkTechAI		= QuestUseGI( pQuest, GI_AI_TECH_FOLLOW );
	pQuestData->nDarkDeadTech	= QuestUseGI( pQuest, GI_MONSTER_DARK_DEADTECH );
	pQuestData->nDarkDRLG		= QuestUseGI( pQuest, GI_DRLG_MINIROOMSDARK );
	pQuestData->nLightDRLG		= QuestUseGI( pQuest, GI_DRLG_MINIROOMS );
	pQuestData->nEndObject		= QuestUseGI( pQuest, GI_OBJECT_ESCORT_END );
	pQuestData->nReturnPortal	= QuestUseGI( pQuest, GI_OBJECT_ONERETURN );
	pQuestData->nWarpToDark		= QuestUseGI( pQuest, GI_OBJECT_WARP_TO_DARK );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestInitDark(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	
	
	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		
		
	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_DARK * pQuestData = ( QUEST_DATA_DARK * )GMALLOCZ( pGame, sizeof( QUEST_DATA_DARK ) );
	sQuestDataInit( pQuest, pQuestData );
	pQuest->pQuestData = pQuestData;

	// add additional cast members
	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nDarkTemplar );
	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nDarkTech1 );
	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nDarkTech2 );
	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nDarkDeadTech );

}
