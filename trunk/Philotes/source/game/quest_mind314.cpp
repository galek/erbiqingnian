//----------------------------------------------------------------------------
// FILE: quest_mind314.cpp
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
#include "quest_mind314.h"
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
#include "room_layout.h"
#include "offer.h"
#include "e_portal.h"
#include "quest_truth_b.h"
#include "quest_demoend.h"

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------
static const int MAX_314_EGOS = 3;

//----------------------------------------------------------------------------
struct QUEST_DATA_MIND314
{
	int		nLucious;
	int		nTechsmith314;
	int		nMindWarp;

	int		nDRLG;
	int		nLevelMindOf314;

	int		nImaginationPOI;

	int		nUltra314;
	int		nUltra314Versions[ MAX_314_EGOS ];
	int		nTruthMinion;
	int		nTruthPortal;

	int				nFatherOld;
	int				nFatherNew;
	int				nArphaun;

};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_MIND314 * sQuestDataGet( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return (QUEST_DATA_MIND314 *)pQuest->pQuestData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSpawnUltra314(
	GAME * pGame,
	QUEST * pQuest,
	ROOM * pSpawnRoom )
{
	QUEST_DATA_MIND314 * pQuestData = sQuestDataGet( pQuest );

	// check the level for truth demon here already
	LEVEL * pLevel = UnitGetLevel( pQuest->pPlayer );
	TARGET_TYPE eTargetTypes[] = { TARGET_BAD, TARGET_INVALID };
	UNIT *pDemon = LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_MONSTER, pQuestData->nUltra314 );
	if ( pDemon )
		return;

	// spawn the demon
	int nNodeIndex = s_RoomGetRandomUnoccupiedNode( pGame, pSpawnRoom, 0 );
	if( nNodeIndex < 0 )
		return;

	// init location
	SPAWN_LOCATION tLocation;
	SpawnLocationInit( &tLocation, pSpawnRoom, nNodeIndex );

	MONSTER_SPEC tSpec;
	tSpec.nClass = pQuestData->nUltra314;
	tSpec.nExperienceLevel = QuestGetMonsterLevel( pQuest, pLevel, tSpec.nClass, 17 );
	tSpec.pRoom = pSpawnRoom;
	tSpec.vPosition = tLocation.vSpawnPosition;
	tSpec.vDirection = cgvNone;
	tSpec.dwFlags = MSF_DONT_DEACTIVATE_WITH_ROOM | MSF_DONT_DEPOPULATE;
	s_MonsterSpawn( pGame, tSpec );

	// spawn in his minions =)
	int nMinionLevel = QuestGetMonsterLevel( pQuest, pLevel, pQuestData->nTruthMinion, 16 );
	for ( int spawns = 0; spawns < pQuest->pQuestDef->nSpawnCount; spawns++ )
	{
		MONSTER_SPEC tSpec;
		tSpec.nClass = pQuestData->nTruthMinion;
		tSpec.nExperienceLevel = nMinionLevel;
		tSpec.pRoom = pSpawnRoom;
		tSpec.vPosition = tLocation.vSpawnPosition;
		tSpec.vDirection = cgvNone;
		tSpec.dwFlags = MSF_DONT_DEACTIVATE_WITH_ROOM | MSF_DONT_DEPOPULATE;
		s_MonsterSpawn( pGame, tSpec );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageUnitHasInfo( 
	QUEST *pQuest,
	const QUEST_MESSAGE_INTERACT_INFO *pMessage)
{
	QUEST_STATUS eStatus = QuestGetStatus( pQuest );
	QUEST_DATA_MIND314 * pQuestData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );

	// quest active
	if (UnitIsMonsterClass( pSubject, pQuestData->nLucious ))
	{
		if (QuestIsInactive( pQuest ))
		{
			if (s_QuestIsPrereqComplete( pQuest ))
			{
				return QMR_NPC_STORY_NEW;
			}
			return QMR_OK;
		}
		else if (QuestIsActive( pQuest ))
		{
			if (QuestStateIsActive( pQuest, QUEST_STATE_MIND314_TALK_LUCIOUS ))
			{
				return QMR_NPC_STORY_RETURN;
			}

			if (QuestStateIsActive( pQuest, QUEST_STATE_MIND314_TALK_314LUCIOUS ))
			{
				if ( UnitGetLevelDefinitionIndex( pSubject ) == pQuestData->nLevelMindOf314 )
				{
					return QMR_NPC_STORY_RETURN;
				}			
			}
			return QMR_OK;
		}
		return QMR_OK;
	}

	// quest not active
	if ( eStatus != QUEST_STATUS_ACTIVE )
	{
		return QMR_OK;
	}

	if (UnitIsMonsterClass( pSubject, pQuestData->nFatherOld ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_MIND314_TRUTH_B_TALK_FATHER1 ) == QUEST_STATE_ACTIVE )
		{
			return QMR_NPC_STORY_RETURN;
		}
		return QMR_OK;
	}

	if (UnitIsMonsterClass( pSubject, pQuestData->nFatherNew ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_MIND314_TRUTH_B_TALK_FATHER2 ) == QUEST_STATE_ACTIVE )
		{
			return QMR_NPC_STORY_RETURN;
		}
		return QMR_OK;
	}

	if (UnitIsMonsterClass( pSubject, pQuestData->nArphaun ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_MIND314_TRUTH_B_RETURN_LORD ) == QUEST_STATE_ACTIVE )
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
	QUEST_DATA_MIND314 * pQuestData = sQuestDataGet( pQuest );
	QUEST_STATUS eStatus = QuestGetStatus( pQuest );

	// quest activate
	if (UnitIsMonsterClass( pSubject, pQuestData->nLucious ))
	{
		if ( ( eStatus == QUEST_STATUS_INACTIVE ) && s_QuestIsPrereqComplete( pQuest ) )
		{
			s_QuestActivate( pQuest );
			eStatus = QuestGetStatus( pQuest );	// reset status (now that it has been activated...)
		}		
	}

	if (UnitIsMonsterClass( pSubject, pQuestData->nFatherOld ))
	{

		// force to this state in the quest
		s_QuestInactiveAdvanceTo( pQuest, QUEST_STATE_MIND314_TRUTH_B_TALK_FATHER1 );				
		
		s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK, DIALOG_TRUTH_B_FATHER_1 );
		return QMR_NPC_TALK;
		
	}

	if (UnitIsMonsterClass( pSubject, pQuestData->nFatherNew ))
	{
	
		// force to this state in the quest
		s_QuestInactiveAdvanceTo( pQuest, QUEST_STATE_MIND314_TRUTH_B_TALK_FATHER2 );
	
		if ( QuestStateGetValue( pQuest, QUEST_STATE_MIND314_TRUTH_B_TALK_FATHER2 ) == QUEST_STATE_ACTIVE )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK, DIALOG_TRUTH_B_FATHER_2 );
			return QMR_NPC_TALK;
		}

		return QMR_OK;
	}

	if ( eStatus != QUEST_STATUS_ACTIVE )
	{
		return QMR_OK;
	}

	if (UnitIsMonsterClass( pSubject, pQuestData->nLucious ))
	{
		if (QuestStateIsActive( pQuest, QUEST_STATE_MIND314_TALK_LUCIOUS ))
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK, DIALOG_MIND314_LUCIOUS, INVALID_LINK, GI_MONSTER_TECHSMITH_314 );
			return QMR_NPC_TALK;
		}
		if ( QuestStateGetValue( pQuest, QUEST_STATE_MIND314_TALK_314LUCIOUS ) == QUEST_STATE_ACTIVE )
		{
			if ( UnitGetLevelDefinitionIndex( pSubject ) != pQuestData->nLevelMindOf314 )
				return QMR_OK;

			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK, DIALOG_MIND314_LUCIOUS_INMIND );
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
	UNIT * player = pQuest->pPlayer;
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pTalkingTo = UnitGetById( pGame, pMessage->idTalkingTo );

	switch( nDialogStopped )
	{

		//----------------------------------------------------------------------------
		case DIALOG_MIND314_LUCIOUS:
		{
			QuestStateSet( pQuest, QUEST_STATE_MIND314_TALK_LUCIOUS, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_MIND314_ENTER_MIND, QUEST_STATE_ACTIVE );
			QUEST_DATA_MIND314 * pQuestData = sQuestDataGet( pQuest );
			s_QuestOpenWarp( UnitGetGame( player ), player, pQuestData->nMindWarp );
			break;
		}

		//----------------------------------------------------------------------------
		case DIALOG_MIND314_LUCIOUS_INMIND:
		{
			s_QuestStateSetForAll( pQuest, QUEST_STATE_MIND314_TALK_314LUCIOUS, QUEST_STATE_COMPLETE );
			s_QuestStateSetForAll( pQuest, QUEST_STATE_MIND314_DEFEAT_314, QUEST_STATE_ACTIVE );
			sSpawnUltra314( UnitGetGame( player ), pQuest, UnitGetRoom( pTalkingTo ) );
			break;
		}
		
		//----------------------------------------------------------------------------
		case DIALOG_TRUTH_B_FATHER_1:
		{
			QuestStateSet( pQuest, QUEST_STATE_MIND314_TRUTH_B_TALK_FATHER1, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_MIND314_TRUTH_B_TALK_FATHER2, QUEST_STATE_ACTIVE );
			s_QuestTransportToNewTruthRoom( player );			
			break;
		}

		//----------------------------------------------------------------------------
		case DIALOG_TRUTH_B_FATHER_2:
		{
			QuestStateSet( pQuest, QUEST_STATE_MIND314_TRUTH_B_TALK_FATHER2, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_MIND314_TRUTH_B_RETURN_LORD, QUEST_STATE_ACTIVE );
			break;
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
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pObject = UnitGetById( pGame, pMessage->idObject );
	QUEST_DATA_MIND314 * pQuestData = sQuestDataGet( pQuest );

	if ( UnitIsObjectClass( pObject, pQuestData->nMindWarp ) )
	{
		if ( QuestGetStatus( pQuest ) != QUEST_STATUS_ACTIVE )
			return QMR_OPERATE_FORBIDDEN;

		if ( QuestStateGetValue( pQuest, QUEST_STATE_MIND314_TRUTH_B_RETURN_LORD ) >= QUEST_STATE_ACTIVE )
			return QMR_OPERATE_FORBIDDEN;

		if ( QuestStateGetValue( pQuest, QUEST_STATE_MIND314_ENTER_MIND ) >= QUEST_STATE_ACTIVE )
			return QMR_OK;

		return QMR_OPERATE_FORBIDDEN;
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageIsUnitVisible( 
	QUEST *pQuest,
	const QUEST_MESSAGE_IS_UNIT_VISIBLE * pMessage)
{
	QUEST_DATA_MIND314 * pQuestData = sQuestDataGet( pQuest );

	if ( pMessage->nUnitClass == pQuestData->nMindWarp )
	{
		if ( QuestGetStatus( pQuest ) == QUEST_STATUS_ACTIVE )
		{
			if ( QuestStateGetValue( pQuest, QUEST_STATE_MIND314_TRUTH_B_RETURN_LORD ) >= QUEST_STATE_ACTIVE )
				return QMR_UNIT_HIDDEN;

			if ( QuestStateGetValue( pQuest, QUEST_STATE_MIND314_ENTER_MIND ) >= QUEST_STATE_ACTIVE )
				return QMR_UNIT_VISIBLE;
		}

		return QMR_UNIT_HIDDEN;
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

	if (nQuestState == QUEST_STATE_MIND314_TALK_LUCIOUS &&
		eValue == QUEST_STATE_COMPLETE)
	{
		QUEST_DATA_MIND314 * pQuestData = sQuestDataGet( pQuest );
		// make the portal visible
		LEVEL * pLevel = UnitGetLevel( pQuest->pPlayer );
		ASSERT_RETVAL( pLevel, QMR_OK );
		TARGET_TYPE eTargetTypes[] = { TARGET_OBJECT, TARGET_INVALID };
		UNIT * warp =  LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_OBJECT, pQuestData->nMindWarp );
		// may not have warp if you abandon the quest in the mind of 314 level
		if ( warp )
		{
			c_UnitSetNoDraw(warp, FALSE, FALSE);
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
	QUEST_DATA_MIND314* pData = sQuestDataGet( pQuest );

	if ( pMessage->nDRLGDefId != pData->nDRLG )
		return QMR_OK;

	if ( pMessage->nLevelDefId != pData->nLevelMindOf314 )
		return QMR_DRLG_RULE_FORBIDDEN;

	if ( QuestGetStatus( pQuest ) >= QUEST_STATUS_COMPLETE )
	{
		return QMR_DRLG_RULE_FORBIDDEN;
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

	QUEST_DATA_MIND314 * pQuestData = sQuestDataGet( pQuest );

	if ( pMessage->nLevelNewDef == pQuestData->nLevelMindOf314 )
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_MIND314_ENTER_MIND ) == QUEST_STATE_ACTIVE )
		{
			QuestStateSet( pQuest, QUEST_STATE_MIND314_ENTER_MIND, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_MIND314_FIND_LIMIT, QUEST_STATE_ACTIVE );
		}
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessagePlayerApproach(
	QUEST *pQuest,
	const QUEST_MESSAGE_PLAYER_APPROACH *pMessage )
{

	if ( !QuestIsActive( pQuest ) )
		return QMR_OK;

	QUEST_DATA_MIND314 * pQuestData = sQuestDataGet( pQuest );
	UNIT *pTarget = UnitGetById( QuestGetGame( pQuest ), pMessage->idTarget );

	if ( UnitIsGenusClass( pTarget, GENUS_OBJECT, pQuestData->nImaginationPOI ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_MIND314_FIND_LIMIT ) == QUEST_STATE_ACTIVE )
		{
			QuestStateSet( pQuest, QUEST_STATE_MIND314_FIND_LIMIT, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_MIND314_TALK_314LUCIOUS, QUEST_STATE_ACTIVE );
		}
	}

	if (UnitIsMonsterClass( pTarget, pQuestData->nArphaun ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_MIND314_TRUTH_B_RETURN_LORD ) == QUEST_STATE_ACTIVE )
		{
			QuestStateSet( pQuest, QUEST_STATE_MIND314_TRUTH_B_RETURN_LORD, QUEST_STATE_COMPLETE );
			s_QuestComplete( pQuest );
			// auto complete end of beta quest if needed
			s_QuestDemoEndForceComplete( pQuest->pPlayer );
		}
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sIsSplitEgo(
	QUEST_DATA_MIND314 *pQuestData,
	UNIT *pUnit)
{
	ASSERTX_RETFALSE( pQuestData, "Expected quest data" );
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	
	for (int i = 0; i < MAX_314_EGOS; ++i)
	{
		if (UnitIsMonsterClass( pUnit, pQuestData->nUltra314Versions[ i ] ))
		{
			return TRUE;
		}
	}
	
	return FALSE;
	
}

//----------------------------------------------------------------------------
struct EGO_CALLBACK_DATA
{
	QUEST_DATA_MIND314 *pQuestData;
	BOOL bAllDead;
	
	EGO_CALLBACK_DATA::EGO_CALLBACK_DATA( void )
		:	pQuestData( NULL ),
			bAllDead( TRUE )
	{ }
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static ROOM_ITERATE_UNITS sCountEgos(
	UNIT *pUnit,
	void *pCallbackData)
{
	EGO_CALLBACK_DATA *pEgoCallbackData = (EGO_CALLBACK_DATA *)pCallbackData;
	if (sIsSplitEgo( pEgoCallbackData->pQuestData, pUnit ))
	{
		if (IsUnitDeadOrDying( pUnit ) == FALSE)
		{
			pEgoCallbackData->bAllDead = FALSE;
			return RIU_STOP;  //  no need to keep searching
		}
	}
	return RIU_CONTINUE;
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sAllSplitEgosAreDead(
	QUEST_DATA_MIND314 *pQuestData,
	LEVEL *pLevel)
{
	ASSERTX_RETFALSE( pQuestData, "Expected quest data" );
	ASSERTX_RETFALSE( pLevel, "Expected level" );
	
	// setup callback data
	EGO_CALLBACK_DATA tCallbackData;
	tCallbackData.pQuestData = pQuestData;
	tCallbackData.bAllDead = TRUE;
	
	// scan the level for any alive split egos
	TARGET_TYPE eTargetTypes[] = { TARGET_BAD, TARGET_INVALID };
	LevelIterateUnits( pLevel, eTargetTypes, sCountEgos, &tCallbackData );
	
	return tCallbackData.bAllDead;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageMonsterKill( 
	QUEST *pQuest,
	const QUEST_MESSAGE_MONSTER_KILL *pMessage)
{
	QUEST_DATA_MIND314 * pQuestData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pVictim = UnitGetById( pGame, pMessage->idVictim );
	UNIT *pKiller = UnitGetById( pGame, pMessage->idKiller );

	// killed 314's ego
	if (sIsSplitEgo( pQuestData, pVictim ) == TRUE)
	{
		if (sAllSplitEgosAreDead( pQuestData, UnitGetLevel( pVictim ) ))
		{
			s_QuestStateSetForAll( pQuest, QUEST_STATE_MIND314_DEFEAT_314, QUEST_STATE_COMPLETE );
			s_QuestSpawnObject( pQuestData->nTruthPortal, pVictim, pKiller );
			s_QuestStateSetForAll( pQuest, QUEST_STATE_MIND314_TRUTH_B_TALK_FATHER1, QUEST_STATE_ACTIVE );						
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
		case QM_CAN_OPERATE_OBJECT:
		{
			const QUEST_MESSAGE_CAN_OPERATE_OBJECT *pCanOperateObjectMessage = (const QUEST_MESSAGE_CAN_OPERATE_OBJECT *)pMessage;
			return sQuestMessageCanOperateObject( pQuest, pCanOperateObjectMessage );
		}

		//----------------------------------------------------------------------------
		case QM_IS_UNIT_VISIBLE:
		{
			const QUEST_MESSAGE_IS_UNIT_VISIBLE *pVisibleMessage = (const QUEST_MESSAGE_IS_UNIT_VISIBLE *)pMessage;
			return sQuestMessageIsUnitVisible( pQuest, pVisibleMessage );
		}

		//----------------------------------------------------------------------------
		case QM_CLIENT_QUEST_STATE_CHANGED:
		{
			const QUEST_MESSAGE_STATE_CHANGED *pStateMessage = (const QUEST_MESSAGE_STATE_CHANGED *)pMessage;
			return sQuestMesssageStateChanged( pQuest, pStateMessage );		
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

		//----------------------------------------------------------------------------
		case QM_SERVER_PLAYER_APPROACH:
		{
			const QUEST_MESSAGE_PLAYER_APPROACH *pPlayerApproachMessage = (const QUEST_MESSAGE_PLAYER_APPROACH *)pMessage;
			return sQuestMessagePlayerApproach( pQuest, pPlayerApproachMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_MONSTER_KILL:
		{
			const QUEST_MESSAGE_MONSTER_KILL *pMonsterKillMessage = (const QUEST_MESSAGE_MONSTER_KILL *)pMessage;
			return sQuestMessageMonsterKill( pQuest, pMonsterKillMessage );
		}

	}

	return QMR_OK;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestFreeMind314(
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
	QUEST_DATA_MIND314 * pQuestData)
{
	pQuestData->nLucious				= QuestUseGI( pQuest, GI_MONSTER_LUCIOUS );
	pQuestData->nTechsmith314			= QuestUseGI( pQuest, GI_MONSTER_TECHSMITH_314 );
	pQuestData->nMindWarp				= QuestUseGI( pQuest, GI_OBJECT_MIND314WARP );
	pQuestData->nDRLG					= QuestUseGI( pQuest, GI_DRLG_HELLDUNGEON );
	pQuestData->nLevelMindOf314 		= QuestUseGI( pQuest, GI_LEVEL_MIND314 );
	pQuestData->nTruthMinion			= QuestUseGI( pQuest, GI_MONSTER_TRUTH_MINION );
	pQuestData->nImaginationPOI 		= QuestUseGI( pQuest, GI_OBJECT_IMAGINATION_POI );
	pQuestData->nUltra314				= QuestUseGI( pQuest, GI_MONSTER_ULTRA_314 );
	pQuestData->nUltra314Versions[ 0 ]	= QuestUseGI( pQuest, GI_MONSTER_ULTRA_314_TEMPLAR );
	pQuestData->nUltra314Versions[ 1 ]	= QuestUseGI( pQuest, GI_MONSTER_ULTRA_314_HUNTER );
	pQuestData->nUltra314Versions[ 2 ]	= QuestUseGI( pQuest, GI_MONSTER_ULTRA_314_CABALIST );
	pQuestData->nTruthPortal			= QuestUseGI( pQuest, GI_OBJECT_TRUTH_B_OLD_ENTER );
	pQuestData->nFatherOld				= QuestUseGI( pQuest, GI_MONSTER_TRUTH_B_SAGE_OLD );
	pQuestData->nFatherNew				= QuestUseGI( pQuest, GI_MONSTER_TRUTH_B_SAGE_NEW );
	pQuestData->nArphaun				= QuestUseGI( pQuest, GI_MONSTER_LORD_ARPHAUN );		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestInitMind314(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	
	
	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		
			
	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_MIND314 * pQuestData = ( QUEST_DATA_MIND314 * )GMALLOCZ( pGame, sizeof( QUEST_DATA_MIND314 ) );
	sQuestDataInit( pQuest, pQuestData );
	pQuest->pQuestData = pQuestData;

	// add additional cast members
	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nArphaun );
	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nFatherOld );
	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nFatherNew );	

	// quest message flags
	pQuest->dwQuestMessageFlags |= MAKE_MASK( QSMF_APPROACH_RADIUS ) | MAKE_MASK( QSMF_IS_UNIT_VISIBLE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestMind314Version(
	const QUEST_FUNCTION_PARAM &tParam)
{
/*	UNIT *pPlayer = tParam.pPlayer;
	QUEST *pQuest = tParam.pQuest;
	int nVersion = QuestGetVersion( pQuest );
	
	// this quest was combined with the truthB quest and if truthb is not completed, 
	// then mind needs to be reopened
	if (nVersion < 1)
	{
		if (QuestIsComplete( pQuest ))
		{
			QUEST *pQuestTruthB = QuestGetQuest( pPlayer, QUEST_TRUTH_B );
			if (pQuestTruthB)
			{
				if (QuestIsActive( pQuestTruthB ))
				{
					s_QuestDeactivate( pQuest );
					s_QuestActivate( pQuest );
				}
			}
		}
	}*/
}