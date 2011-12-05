//----------------------------------------------------------------------------
// FILE: quest_wisdomchaos.cpp
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
#include "quest_chocolate.h"
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
#include "movement.h"
#include "inventory.h"

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct QUEST_DATA_CHOCOLATE
{
	int		nLevelCP;
	int		nLevelCCStation;
	int		nOracleItem;
	int		nLucious;
	int		nTech314;
	int		nRunaway314;
	int		nPitOfHappiness;
	int		nBrain;
	int		nMonsterOracleClient;
	int		nObjectWarpCCS_AA;
	int		nLevelGreenParkStation;
	int		nAiIdle;
	int		nAiTechFollow;
};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_CHOCOLATE * sQuestDataGet( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return (QUEST_DATA_CHOCOLATE *)pQuest->pQuestData;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static BOOL sTechsmithInteract( GAME * game, UNIT * unit, const struct EVENT_DATA& tEventData )
{
	ASSERT_RETFALSE( game && unit );

	// okay, you can catch the techsmith now!
	UnitSetInteractive( unit, UNIT_INTERACTIVE_ENABLED );

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define TECHSMITH_INTERACTIVE_DELAY_SECONDS		5

static void sTechsmith314Runaway( QUEST * pQuest )
{
	QUEST_DATA_CHOCOLATE * pQuestData = sQuestDataGet( pQuest );

	// check the level for techsmith 314
	UNIT * pPlayer = pQuest->pPlayer;
	LEVEL * pLevel = UnitGetLevel( pPlayer );
	TARGET_TYPE eTargetTypes[] = { TARGET_GOOD, TARGET_INVALID };
	UNIT * techsmith =  LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_MONSTER, pQuestData->nTech314 );
	if ( !techsmith )
	{
		return;
	}

	// remove and spawn the runaway tech
	// hold old location
	VECTOR vPosition = UnitGetPosition( techsmith );
	VECTOR vDirection = UnitGetDirectionXY( techsmith );
	ROOM * room = UnitGetRoom( techsmith );
	UnitFree( techsmith, UFF_SEND_TO_CLIENTS );

	MONSTER_SPEC tSpec;
	tSpec.nClass = pQuestData->nRunaway314;
	tSpec.nExperienceLevel = QuestGetMonsterLevel( pQuest, pLevel, tSpec.nClass, 1 );
	tSpec.pRoom = room;
	tSpec.vPosition = vPosition;
	tSpec.vDirection = vDirection;
	tSpec.dwFlags = MSF_DONT_DEACTIVATE_WITH_ROOM | MSF_DONT_DEPOPULATE;
	techsmith = s_MonsterSpawn( UnitGetGame( pPlayer ), tSpec );
	ASSERT_RETURN( techsmith );
	if ( techsmith )
	{
		// set event to become interactive
		UnitRegisterEventTimed( techsmith, sTechsmithInteract, NULL, GAME_TICKS_PER_SECOND * TECHSMITH_INTERACTIVE_DELAY_SECONDS );
	}
	AI_Register(techsmith, FALSE, 1);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChangeTechsmith314AI( QUEST * pQuest, int nNewAI, BOOL bSetPlayerTarget )
{
	QUEST_DATA_CHOCOLATE * pQuestData = sQuestDataGet( pQuest );

	// check the level for techsmith 314
	UNIT * pPlayer = pQuest->pPlayer;
	LEVEL * pLevel = UnitGetLevel( pPlayer );
	TARGET_TYPE eTargetTypes[] = { TARGET_GOOD, TARGET_INVALID };
	UNIT * techsmith =  LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_MONSTER, pQuestData->nRunaway314 );
	if ( !techsmith )
	{
		return;
	}

	// change his AI
	GAME * pGame = UnitGetGame( techsmith );
	int ai = UnitGetStat( techsmith, STATS_AI );
	if ( ai == nNewAI )
		return;
	AI_Free( pGame, techsmith );
	UnitSetStat( techsmith, STATS_AI, nNewAI );
	if ( bSetPlayerTarget )
		UnitSetAITarget(techsmith, pPlayer, TRUE);
	else
		UnitSetAITarget(techsmith, INVALID_ID, TRUE);
	AI_Init( pGame, techsmith );
	AI_Register(techsmith, FALSE, 1);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UNIT * sgOracleUnit = NULL;

static QUEST_MESSAGE_RESULT sClientQuestMesssageStateChanged( 
	QUEST *pQuest,
	const QUEST_MESSAGE_STATE_CHANGED *pMessage)
{
	int nQuestState = pMessage->nQuestState;
	QUEST_STATE_VALUE eValue = 	pMessage->eValueNew;
	QUEST_DATA_CHOCOLATE *pQuestData = sQuestDataGet( pQuest );

	if ( QuestGetStatus( pQuest ) != QUEST_STATUS_ACTIVE )
	{
		return QMR_OK;
	}

	if ( ( nQuestState == QUEST_STATE_CHOCOLATE_ACTIVATE ) && ( eValue == QUEST_STATE_COMPLETE ) )
	{
#if !ISVERSION(SERVER_VERSION)
		// close the inventory and spawn the demon
		UIComponentActivate(UICOMP_INVENTORY, FALSE);

		GAME * game = UnitGetGame( pQuest->pPlayer );
		ASSERT( IS_CLIENT( game ) );

		UNIT * player = GameGetControlUnit( game );

		sgOracleUnit = MonsterSpawnClient( game, pQuestData->nMonsterOracleClient, UnitGetRoom( player ), player->vPosition, player->vFaceDirection );
#endif
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
	QUEST_DATA_CHOCOLATE *pQuestData = sQuestDataGet( pQuest );

	if ( pQuest->eStatus >= QUEST_STATUS_COMPLETE )
		return QMR_OK;

	// is this an object that this quest cares about
	if ( UnitIsObjectClass( pObject, pQuestData->nObjectWarpCCS_AA ) )
	{
		if ( pQuest->eStatus != QUEST_STATUS_ACTIVE )
			return QMR_OPERATE_FORBIDDEN;

		return QMR_OPERATE_OK;
	}

	return QMR_OK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCheckPitActive( QUEST * pQuest )
{
	if ( QuestStateGetValue( pQuest, QUEST_STATE_CHOCOLATE_FIND_PIT ) < QUEST_STATE_COMPLETE )
	{
		QUEST_DATA_CHOCOLATE * pQuestData = sQuestDataGet( pQuest );
		UNIT * pPlayer = pQuest->pPlayer;
		LEVEL * pLevel = UnitGetLevel( pPlayer );
		TARGET_TYPE eTargetTypes[] = { TARGET_OBJECT, TARGET_INVALID };
		UNIT * pPit =  LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_OBJECT, pQuestData->nPitOfHappiness );
		if ( pPit )
		{
			// make sure trigger is active
			UnitSetFlag( pPit, UNITFLAG_TRIGGER, TRUE );
		}
	}
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

	QUEST_DATA_CHOCOLATE * pQuestData = sQuestDataGet( pQuest );

	if ( pMessage->nLevelNewDef == pQuestData->nLevelCP )
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_CHOCOLATE_TRAVEL_CP ) == QUEST_STATE_ACTIVE )
		{
			QuestStateSet( pQuest, QUEST_STATE_CHOCOLATE_TRAVEL_CP, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_CHOCOLATE_FIND_PIT, QUEST_STATE_ACTIVE );
		}
		sCheckPitActive( pQuest );
	}

	if ( pMessage->nLevelNewDef == pQuestData->nLevelCCStation )
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_CHOCOLATE_RETURN ) == QUEST_STATE_ACTIVE )
		{
			QuestStateSet( pQuest, QUEST_STATE_CHOCOLATE_RETURN, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_CHOCOLATE_UPDATE, QUEST_STATE_ACTIVE );
		}
	}

	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageCanUseItem(
	QUEST *pQuest,
	const QUEST_MESSAGE_CAN_USE_ITEM * pMessage)
{
	QUEST_DATA_CHOCOLATE * pQuestData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pItem = UnitGetById( pGame, pMessage->idItem );
	if (pItem)
	{
		int item_class = UnitGetClass( pItem );

		if ( item_class == pQuestData->nOracleItem )
		{
			if ( QuestGetStatus( pQuest ) == QUEST_STATUS_ACTIVE )
			{
				if ( QuestStateGetValue( pQuest, QUEST_STATE_CHOCOLATE_ACTIVATE ) == QUEST_STATE_ACTIVE )
				{
					LEVEL * pLevel = UnitGetLevel( pQuest->pPlayer );
					if ( pLevel && pLevel->m_nLevelDef == pQuestData->nLevelGreenParkStation )
					{
						return QMR_USE_OK;
					}
				}
			}
			return QMR_USE_FAIL;
		}
	}
	
	return QMR_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageUseItem(
	QUEST *pQuest,
	const QUEST_MESSAGE_USE_ITEM * pMessage)
{
	QUEST_DATA_CHOCOLATE * pQuestData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pItem = UnitGetById( pGame, pMessage->idItem );
	if (pItem)
	{
		int item_class = UnitGetClass( pItem );

		if ( item_class == pQuestData->nOracleItem )
		{
			if ( QuestGetStatus( pQuest ) == QUEST_STATUS_ACTIVE )
			{
				if ( QuestStateGetValue( pQuest, QUEST_STATE_CHOCOLATE_ACTIVATE ) == QUEST_STATE_ACTIVE )
				{
					s_QuestRemoveItem( pQuest->pPlayer, pQuestData->nOracleItem );
					QuestStateSet( pQuest, QUEST_STATE_CHOCOLATE_ACTIVATE, QUEST_STATE_COMPLETE );
					QuestStateSet( pQuest, QUEST_STATE_CHOCOLATE_TALK_LUCIOUS, QUEST_STATE_ACTIVE );
				}
				return QMR_OK;			
			}
			return QMR_USE_FAIL;
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
	QUEST_DATA_CHOCOLATE * pQuestData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );
	
	// quest not active
	if ( eStatus != QUEST_STATUS_ACTIVE )
	{
		return QMR_OK;
	}

	// quest active
	if ( UnitIsMonsterClass( pSubject, pQuestData->nLucious ))
	{
		int current_level = UnitGetLevelDefinitionIndex( pQuest->pPlayer );

		if ( ( current_level == pQuestData->nLevelGreenParkStation ) && ( QuestStateGetValue( pQuest, QUEST_STATE_CHOCOLATE_TALK_LUCIOUS ) == QUEST_STATE_ACTIVE ) )
		{
			return QMR_NPC_STORY_RETURN;
		}
		if ( current_level == pQuestData->nLevelCP )
		{
			if ( QuestStateGetValue( pQuest, QUEST_STATE_CHOCOLATE_TALK_LUCIOUS2 ) == QUEST_STATE_ACTIVE )
			{
				return QMR_NPC_STORY_RETURN;
			}
			if ( QuestStateGetValue( pQuest, QUEST_STATE_CHOCOLATE_TALK_LUCIOUS3 ) == QUEST_STATE_ACTIVE )
			{
				return QMR_NPC_STORY_RETURN;
			}
		}
		if ( ( current_level == pQuestData->nLevelCCStation ) && ( QuestStateGetValue( pQuest, QUEST_STATE_CHOCOLATE_UPDATE ) == QUEST_STATE_ACTIVE ) )
		{
			return QMR_NPC_STORY_RETURN;
		}
		return QMR_OK;
	}

	if (UnitIsMonsterClass( pSubject, pQuestData->nRunaway314 ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_CHOCOLATE_TAG_SMITH ) == QUEST_STATE_ACTIVE )
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
	if ( QuestGetStatus( pQuest ) >= QUEST_STATUS_COMPLETE )
		return QMR_OK;

	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pPlayer = UnitGetById( pGame, pMessage->idPlayer );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );
	ASSERTX_RETVAL( pPlayer, QMR_OK, "Invalid player" );
	ASSERTX_RETVAL( pSubject, QMR_OK, "Invalid NPC" );
	QUEST_DATA_CHOCOLATE * pQuestData = sQuestDataGet( pQuest );

	// quest inactive
	if ( QuestGetStatus( pQuest ) == QUEST_STATUS_INACTIVE )
	{
		if ( !s_QuestIsPrereqComplete( pQuest ) )
			return QMR_OK;

		if (UnitIsMonsterClass( pSubject, pQuestData->nLucious ))
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, DIALOG_CHOCOLATE_LUCIOUS_START );
			return QMR_NPC_TALK;
		}
		return QMR_OK;
	}

	if ( QuestGetStatus( pQuest ) != QUEST_STATUS_ACTIVE )
		return QMR_OK;

	// quest active
	if (UnitIsMonsterClass( pSubject, pQuestData->nLucious ))
	{
		int current_level = UnitGetLevelDefinitionIndex( pQuest->pPlayer );

		if ( ( current_level == pQuestData->nLevelGreenParkStation ) && ( QuestStateGetValue( pQuest, QUEST_STATE_CHOCOLATE_TALK_LUCIOUS ) == QUEST_STATE_ACTIVE ) )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK, DIALOG_CHOCOLATE_LUCIOUS_GOCP, INVALID_LINK, GI_MONSTER_TECHSMITH_314 );
			return QMR_NPC_TALK;
		}
		if ( current_level == pQuestData->nLevelCP )
		{
			if ( QuestStateGetValue( pQuest, QUEST_STATE_CHOCOLATE_TALK_LUCIOUS2 ) == QUEST_STATE_ACTIVE )
			{
				s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK, DIALOG_CHOCOLATE_LUCIOUS_INCP, INVALID_LINK, GI_MONSTER_TECHSMITH_314 );
				return QMR_NPC_TALK;
			}
			if ( QuestStateGetValue( pQuest, QUEST_STATE_CHOCOLATE_TALK_LUCIOUS3 ) == QUEST_STATE_ACTIVE )
			{
				s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK, DIALOG_CHOCOLATE_LUCIOUS_DUNK, INVALID_LINK, GI_MONSTER_TECHSMITH_314 );
				return QMR_NPC_TALK;
			}
		}
		if ( ( current_level == pQuestData->nLevelCCStation ) && ( QuestStateGetValue( pQuest, QUEST_STATE_CHOCOLATE_UPDATE ) == QUEST_STATE_ACTIVE ) )
		{
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK, DIALOG_CHOCOLATE_UPDATE, INVALID_LINK, GI_MONSTER_TECHSMITH_314 );
			return QMR_NPC_TALK;
		}
		return QMR_OK;
	}

	if (UnitIsMonsterClass( pSubject, pQuestData->nRunaway314 ))
	{
		if ( QuestStateGetValue( pQuest, QUEST_STATE_CHOCOLATE_TAG_SMITH ) == QUEST_STATE_ACTIVE )
		{
			UnitStopPathing( pSubject );
			sChangeTechsmith314AI( pQuest, pQuestData->nAiIdle, FALSE );
			s_NPCQuestTalkStart( pQuest, pSubject, NPC_DIALOG_OK, DIALOG_CHOCOLATE_314_RETURN );
			return QMR_NPC_TALK;
		}
		return QMR_OK;
	}

	return QMR_OK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHasOracle(
	void *pUserData)
{
	QUEST *pQuest = (QUEST *)pUserData;
	s_QuestActivate( pQuest );
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
	QUEST_DATA_CHOCOLATE *pQuestData = sQuestDataGet( pQuest );

	switch( nDialogStopped )
	{
		//----------------------------------------------------------------------------
		case DIALOG_CHOCOLATE_LUCIOUS_START:
		{
			// if we abandon this quest after we start, we will no longer have an oracle... give him one...
			DWORD dwFlags = 0;
			SETBIT( dwFlags, IIF_ON_PERSON_AND_STASH_ONLY_BIT );
			if ( s_QuestCheckForItem( pQuest->pPlayer, pQuestData->nOracleItem, dwFlags  ) == FALSE )
			{
				// setup actions
				OFFER_ACTIONS tActions;
				tActions.pfnAllTaken = sHasOracle;
				tActions.pAllTakenUserData = pQuest;
				
				// offer to player
				s_OfferGive( pQuest->pPlayer, pTalkingTo, OFFERID_QUEST_STITCH_ORACLE_COMPLETE, tActions );
				return QMR_OFFERING;
				
			}
			else
			{
				s_QuestActivate( pQuest );
			}
			break;
		}

		//----------------------------------------------------------------------------
		case DIALOG_CHOCOLATE_LUCIOUS_GOCP:
		{
			QuestStateSet( pQuest, QUEST_STATE_CHOCOLATE_TALK_LUCIOUS, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_CHOCOLATE_TRAVEL_CP, QUEST_STATE_ACTIVE );
			s_QuestMapReveal( pQuest, GI_LEVEL_ADMIRALTY_ARCH );
			s_QuestMapReveal( pQuest, GI_LEVEL_CHOCOLATE_PARK );
			break;
		}

		//----------------------------------------------------------------------------
		case DIALOG_CHOCOLATE_LUCIOUS_INCP:
		{
			s_QuestStateSetForAll( pQuest, QUEST_STATE_CHOCOLATE_TALK_LUCIOUS2, QUEST_STATE_COMPLETE );
			s_QuestStateSetForAll( pQuest, QUEST_STATE_CHOCOLATE_TAG_SMITH, QUEST_STATE_ACTIVE );
			sTechsmith314Runaway( pQuest );
			break;
		}

		//----------------------------------------------------------------------------
		case DIALOG_CHOCOLATE_314_RETURN:
		{
			s_QuestStateSetForAll( pQuest, QUEST_STATE_CHOCOLATE_TAG_SMITH, QUEST_STATE_COMPLETE );
			s_QuestStateSetForAll( pQuest, QUEST_STATE_CHOCOLATE_TALK_LUCIOUS3, QUEST_STATE_ACTIVE );
			sChangeTechsmith314AI( pQuest, pQuestData->nAiTechFollow, TRUE );
			break;
		}

		//----------------------------------------------------------------------------
		case DIALOG_CHOCOLATE_LUCIOUS_DUNK:
		{
			QuestStateSet( pQuest, QUEST_STATE_CHOCOLATE_TALK_LUCIOUS3, QUEST_STATE_COMPLETE );
			QuestStateSet( pQuest, QUEST_STATE_CHOCOLATE_RETURN, QUEST_STATE_ACTIVE );
			sChangeTechsmith314AI( pQuest, pQuestData->nAiIdle, FALSE );
			break;
		}

		//----------------------------------------------------------------------------
		case DIALOG_CHOCOLATE_UPDATE:
		{
			QuestStateSet( pQuest, QUEST_STATE_CHOCOLATE_UPDATE, QUEST_STATE_COMPLETE );
			s_QuestComplete( pQuest );
			break;
		}

	}

	return QMR_OK;

}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sSpawnBrain(
	GAME * pGame,
	QUEST * pQuest,
	UNIT * pPitObject )
{
	QUEST_DATA_CHOCOLATE * pQuestData = sQuestDataGet( pQuest );

	// check the level for a brain
	LEVEL * pLevel = UnitGetLevel( pQuest->pPlayer );
	TARGET_TYPE eTargetTypes[] = { TARGET_BAD, TARGET_INVALID };
	if (LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_MONSTER, pQuestData->nBrain ))
	{
		return;
	}
	
	// spawn near pit
	ROOM * pSpawnRoom = UnitGetRoom( pPitObject );

	// and spawn brain
	int nNodeIndex = s_RoomGetRandomUnoccupiedNode( pGame, pSpawnRoom, 0 );
	if (nNodeIndex < 0)
		return;

	// init location
	SPAWN_LOCATION tLocation;
	SpawnLocationInit( &tLocation, pSpawnRoom, nNodeIndex );

	MONSTER_SPEC tSpec;
	tSpec.nClass = pQuestData->nBrain;
	tSpec.nExperienceLevel = QuestGetMonsterLevel( pQuest, pLevel, tSpec.nClass, 12 );
	tSpec.pRoom = pSpawnRoom;
	tSpec.vPosition = tLocation.vSpawnPosition;
	tSpec.vDirection = VECTOR( 0.0f, 0.0f, 0.0f );
	tSpec.dwFlags = MSF_DONT_DEACTIVATE_WITH_ROOM | MSF_DONT_DEPOPULATE;
	s_MonsterSpawn( pGame, tSpec );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static QUEST_MESSAGE_RESULT sQuestMessageObjectOperated( 
	QUEST *pQuest,
	const QUEST_MESSAGE_OBJECT_OPERATED *pMessage)
{

	if (QuestGetStatus( pQuest ) != QUEST_STATUS_ACTIVE ||
		QuestStateGetValue( pQuest, QUEST_STATE_CHOCOLATE_FIND_PIT ) != QUEST_STATE_ACTIVE ||
		QuestStateGetValue( pQuest, QUEST_STATE_CHOCOLATE_DEFEAT_BOSS ) == QUEST_STATE_COMPLETE)
	{
		return QMR_OK;
	}

	QUEST_DATA_CHOCOLATE * pQuestData = sQuestDataGet( pQuest );
	UNIT *pTarget = pMessage->pTarget;

	if ( UnitGetClass( pTarget ) == pQuestData->nPitOfHappiness )
	{
		s_StateSet( pTarget, pTarget, STATE_OPERATE_OBJECT_FX, 0 );
		// deactivate trigger
		UnitSetFlag( pTarget, UNITFLAG_TRIGGER, FALSE );

		s_QuestStateSetForAll( pQuest, QUEST_STATE_CHOCOLATE_FIND_PIT, QUEST_STATE_COMPLETE );
		s_QuestStateSetForAll( pQuest, QUEST_STATE_CHOCOLATE_DEFEAT_BOSS, QUEST_STATE_ACTIVE );
		// spawn boss?!
		sSpawnBrain( UnitGetGame( pTarget ), pQuest, pTarget );
	}

	return QMR_OK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageMonsterKill( 
	QUEST *pQuest,
	const QUEST_MESSAGE_MONSTER_KILL *pMessage)
{
	QUEST_DATA_CHOCOLATE * pData = sQuestDataGet( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pVictim = UnitGetById( pGame, pMessage->idVictim );

	// turn taint into interactive NPC
	if ( UnitIsMonsterClass( pVictim, pData->nBrain ) &&
		 QuestStateGetValue( pQuest, QUEST_STATE_CHOCOLATE_DEFEAT_BOSS ) == QUEST_STATE_ACTIVE )
	{
		s_QuestStateSetForAll( pQuest, QUEST_STATE_CHOCOLATE_DEFEAT_BOSS, QUEST_STATE_COMPLETE );
		s_QuestStateSetForAll( pQuest, QUEST_STATE_CHOCOLATE_TALK_LUCIOUS2, QUEST_STATE_ACTIVE );
	}

	return QMR_OK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT c_sQuestMessageMonsterInit( 
	QUEST *pQuest,
	const QUEST_MESSAGE_MONSTER_INIT *pMessage)
{
	UNIT * monster = pMessage->pMonster;

	QUEST_DATA_CHOCOLATE * pQuestData = sQuestDataGet( pQuest );

	if ( UnitGetClass( monster ) != pQuestData->nTech314 &&
		 UnitGetClass( monster ) != pQuestData->nRunaway314 )
	{
		return QMR_OK;
	}

	// let's get down to face hugging, 
	// but only on clients that have completed the face hugging quest state
	if ( QuestStateGetValue( pQuest, QUEST_STATE_CHOCOLATE_ACTIVATE ) == QUEST_STATE_COMPLETE )
	{
		c_StateSet( monster, monster, STATE_ORACLE_ATTACHED, 0, 0, INVALID_ID );
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
		case QM_CLIENT_QUEST_STATE_CHANGED:
		{
			const QUEST_MESSAGE_STATE_CHANGED *pStateMessage = (const QUEST_MESSAGE_STATE_CHANGED *)pMessage;
			return sClientQuestMesssageStateChanged( pQuest, pStateMessage );		
		}

		//----------------------------------------------------------------------------
		case QM_CAN_OPERATE_OBJECT:
		{
			const QUEST_MESSAGE_CAN_OPERATE_OBJECT *pCanOperateObjectMessage = (const QUEST_MESSAGE_CAN_OPERATE_OBJECT *)pMessage;
			return sQuestMessageCanOperateObject( pQuest, pCanOperateObjectMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_ENTER_LEVEL:
		{
			const QUEST_MESSAGE_ENTER_LEVEL * pTransitionMessage = ( const QUEST_MESSAGE_ENTER_LEVEL * )pMessage;
			return sQuestMessageTransitionLevel( pQuest, pTransitionMessage );
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
			return sQuestMessageUseItem( pQuest, pUseItemMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_OBJECT_OPERATED:
		{
			const QUEST_MESSAGE_OBJECT_OPERATED *pObjectOperatedMessage = (const QUEST_MESSAGE_OBJECT_OPERATED *)pMessage;
			return sQuestMessageObjectOperated( pQuest, pObjectOperatedMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_MONSTER_KILL:
		{
			const QUEST_MESSAGE_MONSTER_KILL *pMonsterKillMessage = (const QUEST_MESSAGE_MONSTER_KILL *)pMessage;
			return sQuestMessageMonsterKill( pQuest, pMonsterKillMessage );
		}

		//----------------------------------------------------------------------------
		case QM_CLIENT_MONSTER_INIT:
		{
			const QUEST_MESSAGE_MONSTER_INIT *pMonsterInitMessage = (const QUEST_MESSAGE_MONSTER_INIT *)pMessage;
			return c_sQuestMessageMonsterInit( pQuest, pMonsterInitMessage );
		}


	}

	return QMR_OK;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestFreeChocolate(
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
	QUEST_DATA_CHOCOLATE * pQuestData)
{
	pQuestData->nLevelCP				= QuestUseGI( pQuest, GI_LEVEL_CHOCOLATE_PARK );
	pQuestData->nLevelCCStation			= QuestUseGI( pQuest, GI_LEVEL_CHARING_CROSS_STATION );
	pQuestData->nOracleItem				= QuestUseGI( pQuest, GI_ITEM_ORACLE );
	pQuestData->nLucious				= QuestUseGI( pQuest, GI_MONSTER_LUCIOUS );
	pQuestData->nTech314				= QuestUseGI( pQuest, GI_MONSTER_TECHSMITH_314 );
	pQuestData->nRunaway314				= QuestUseGI( pQuest, GI_MONSTER_TECHSMITH_314_RUNAWAY );
	pQuestData->nPitOfHappiness			= QuestUseGI( pQuest, GI_OBJECT_PIT_OF_HAPPINESS );
	pQuestData->nBrain					= QuestUseGI( pQuest, GI_MONSTER_BRAIN );
	pQuestData->nMonsterOracleClient	= QuestUseGI( pQuest, GI_MONSTER_ORACLE_CLIENT );
	pQuestData->nObjectWarpCCS_AA		= QuestUseGI( pQuest, GI_OBJECT_WARP_CCSTATION_ADMIRALTYARCH );
	pQuestData->nLevelGreenParkStation	= QuestUseGI( pQuest, GI_LEVEL_GREEN_PARK_STATION );
	pQuestData->nAiIdle					= QuestUseGI( pQuest, GI_AI_IDLE );
	pQuestData->nAiTechFollow			= QuestUseGI( pQuest, GI_AI_TECH_FOLLOW );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestInitChocolate(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	
	
	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		
			
	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_CHOCOLATE * pQuestData = ( QUEST_DATA_CHOCOLATE * )GMALLOCZ( pGame, sizeof( QUEST_DATA_CHOCOLATE ) );
	sQuestDataInit( pQuest, pQuestData );
	pQuest->pQuestData = pQuestData;

	// add additional cast members
	s_QuestAddCastMember( pQuest, GENUS_MONSTER, pQuestData->nRunaway314 );
}
