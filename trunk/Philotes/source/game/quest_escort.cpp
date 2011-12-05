//----------------------------------------------------------------------------
// FILE: quest_escort.cpp
//
// A template quest. Escort the Cast Giver to the level warp to the next 
// area. Then talk to him to finish the quest.
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
#include "quest_escort.h"
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
#include "clients.h"

//----------------------------------------------------------------------------
// LOCAL DATA
//----------------------------------------------------------------------------
static const int ESCORT_VALIDATE_RATE_IN_TICKS = GAME_TICKS_PER_SECOND * 60 * 1;

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------
static BOOL sValidateEscort( GAME *pGame, UNIT *pUnit, const struct EVENT_DATA &tEventData );

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct QUEST_DATA_ESCORT
{
	int		nTemplarAI;
	int		nEndObject;
	int		nIdleAI;
	int		nShield;
	int		nDRLG;
};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_DATA_ESCORT * sQuestDataGet( QUEST *pQuest )
{
	ASSERT_RETNULL( pQuest && pQuest->pQuestData );
	return (QUEST_DATA_ESCORT *)pQuest->pQuestData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSpawnEndObject( QUEST * pQuest )
{
	ASSERT_RETURN( pQuest );
	QUEST_DATA_ESCORT * pQuestData = sQuestDataGet( pQuest );
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
			if (UnitIsA( test, UNITTYPE_WARP_NEXT_LEVEL ))
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
static void s_sEscortResetEscortNPC(
	UNIT *pEscorting)
{
	ASSERTX_RETURN( pEscorting, "Expected unit" );

	int nIdleAI = GlobalIndexGet( GI_AI_IDLE );	
	if (UnitGetStat( pEscorting, STATS_AI ) != nIdleAI)
	{
	
		// restore ai
		GAME *pGame = UnitGetGame( pEscorting );
		AI_Free( pGame, pEscorting);
		UnitSetStat( pEscorting, STATS_AI, nIdleAI);
		AI_Init( pGame, pEscorting );

		// make invulnerable
		s_StateSet( pEscorting, pEscorting, STATE_NPC_GOD, 0 );
			
	}			

	// this NPC is interactive again
	UnitSetInteractive( pEscorting, UNIT_INTERACTIVE_ENABLED );

	// remove any validation event in progress
	UnitUnregisterTimedEvent( pEscorting, sValidateEscort );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sValidateEscort(
	GAME *pGame,
	UNIT *pUnit,
	const struct EVENT_DATA &tEventData)
{
	BOOL bRegisterAnotherEvent = TRUE;
	UNIT *pPlayer = (UNIT*)tEventData.m_Data1;
	ASSERT_RETFALSE(pPlayer);
	if (UnitCanInteractWith( pUnit, pPlayer ) == FALSE)
	{
		
		// if this level is no longer active (happens when players leave a level) then
		// we'll forget it and reset ourselves
		LEVEL *pLevel = UnitGetLevel( pUnit );		
		if (LevelIsActive(pLevel) == FALSE)
		{
			
			// return to idle (don't warp back to start position or anything, the players
			// might come back ... they might have portaled to town or something)
			s_sEscortResetEscortNPC( pUnit );			
			bRegisterAnotherEvent = FALSE;
			
		}		
		
	}

	// register another validation event
	if (bRegisterAnotherEvent)
	{
		UnitRegisterEventTimed( pUnit, sValidateEscort, &tEventData, ESCORT_VALIDATE_RATE_IN_TICKS );
	}
	
	return TRUE;
		
}		

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sStartEscort( 
	QUEST * pQuest, 
	UNIT * pEscortNPC )
{
	ASSERT_RETURN( pQuest && pEscortNPC );
	QUEST_DATA_ESCORT * pQuestData = sQuestDataGet( pQuest );
	GAME * game = UnitGetGame( pEscortNPC );
	ASSERT_RETURN( IS_SERVER( game ) );

	ASSERT_RETURN( s_QuestIsGiver( pQuest, pEscortNPC ) );

	// activate quest
	s_QuestActivateForAll( pQuest );

	// change the AI on the escort templar if needed
	if ( UnitGetStat( pEscortNPC, STATS_AI ) != pQuestData->nTemplarAI )
	{
	
		AI_Free( game, pEscortNPC );
		UnitSetStat( pEscortNPC, STATS_AI, pQuestData->nTemplarAI );
		s_StateClear( pEscortNPC, UnitGetId( pEscortNPC ), STATE_NPC_GOD, 0 );
		AI_Init( game, pEscortNPC );
		sSpawnEndObject( pQuest );
		
		// this NPC is not interactive anymore
		UnitSetInteractive( pEscortNPC, UNIT_INTERACTIVE_RESTRICED );

		// start an event going to check the validity of who we're following
		UnitRegisterEventTimed( pEscortNPC, sValidateEscort, &EVENT_DATA( (DWORD_PTR)QuestGetPlayer( pQuest ) ), ESCORT_VALIDATE_RATE_IN_TICKS );
		
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void c_sChangeEscortNPCName( QUEST * pQuest )
{
	ASSERT_RETURN( pQuest );
	ASSERT_RETURN( IS_CLIENT( UnitGetGame( pQuest->pPlayer ) ) );

	const QUEST_DEFINITION *pQuestDef = pQuest->pQuestDef;
	SPECIES spEscortNPC = QuestCastGetSpecies( pQuest, pQuestDef->nCastGiver );
	ASSERTV_RETURN( spEscortNPC != SPECIES_NONE, "quest \"%s\": no species for escort NPC", pQuestDef->szName );
	int nClassEscortNPC = GET_SPECIES_CLASS( spEscortNPC );

	TARGET_TYPE eTargetTypes[] = { TARGET_GOOD, TARGET_INVALID };
	LEVEL *pLevel = UnitGetLevel( QuestGetPlayer( pQuest ) );
	UNIT * pEscortNPC = LevelFindFirstUnitOf( pLevel, eTargetTypes, GENUS_MONSTER, nClassEscortNPC );
	if ( !pEscortNPC )
		return;

	const WCHAR * name = StringTableGetStringByIndex( pQuestDef->nStringKeyNameOverride ); 
	UnitSetName( pEscortNPC, name );	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCanActivateEscort(
	QUEST *pQuest,
	UNIT *pNPC)
{
	ASSERTX_RETFALSE( pQuest, "Expected quest" );
	ASSERTX_RETFALSE( pNPC, "Expected unit" );
	
	// see if this NPC can activate this quest
	if (s_QuestIsGiver( pQuest, pNPC ) && 
		s_QuestCanBeActivated( pQuest ) &&
		UnitGetStat( pNPC, STATS_ESCORT_COMPLETED ) == FALSE &&
		UnitIsInteractive( pNPC ) == TRUE)
	{
		return TRUE;
	}

	return FALSE;
			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageInteract(
	QUEST *pQuest,
	const QUEST_MESSAGE_INTERACT *pMessage )
{
#if !ISVERSION( CLIENT_ONLY_VERSION )
	const QUEST_DEFINITION * pQuestDef = pQuest->pQuestDef;
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pPlayer = UnitGetById( pGame, pMessage->idPlayer );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );
	ASSERTX_RETVAL( pPlayer, QMR_OK, "Invalid player" );
	ASSERTX_RETVAL( pSubject, QMR_OK, "Invalid NPC" );	
	
	switch (pQuest->eStatus)
	{
	//----------------------------------------------------------------------------
	case QUEST_STATUS_INACTIVE:
		if ( sCanActivateEscort( pQuest, pSubject ))
		{
			// set the destination level stat for all players with the quest
			// Set a stat for everyone in the game
			ASSERT_RETVAL( IS_SERVER( pGame ), QMR_OK );

			int nQuest = pQuest->nQuest;
			int nLevelNext = INVALID_ID;
			LEVEL * level = UnitGetLevel( pPlayer );
			if ( level )
			{
				const LEVEL_DEFINITION * leveldef_current = LevelDefinitionGet( level->m_nLevelDef );
				if ( leveldef_current )
					nLevelNext = leveldef_current->nNextLevel;
			}

			if (AppIsSinglePlayer() || UnitGetPartyId( pPlayer ) == INVALID_ID)
			{
				UnitSetStat( pPlayer, STATS_QUEST_ESCORT_DESTINATION_LEVEL, nQuest, UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT ), nLevelNext );
			}
			else
			{
				PARTYID idParty = UnitGetPartyId( pPlayer );

				for (GAMECLIENT *pClient = ClientGetFirstInGame( pGame );
					pClient;
					pClient = ClientGetNextInGame( pClient ))
				{
					UNIT *pUnit = ClientGetControlUnit( pClient );
					if (UnitGetPartyId( pUnit ) == idParty)
					{
						QUEST *pQuestOther = QuestGetQuest( pUnit, pQuest->nQuest );
						if ( pQuestOther )
							UnitSetStat( pUnit, STATS_QUEST_ESCORT_DESTINATION_LEVEL, nQuest, UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT ), nLevelNext );
					}

				}

			}	

			s_QuestDisplayDialog( pQuest, pSubject, NPC_DIALOG_OK_CANCEL, pQuestDef->nDescriptionDialog );
			return QMR_NPC_TALK;
		}
		break;

	//----------------------------------------------------------------------------
	case QUEST_STATUS_ACTIVE:
		if ( QuestStateGetValue( pQuest, QUEST_STATE_ESCORTTEMPLATE_FIND_EXIT ) == QUEST_STATE_COMPLETE )
		{
			if ( s_QuestIsRewarder( pQuest,  pSubject ) )
			{
				// Reward: tell the player good job, and offer rewards
				s_QuestDisplayDialog(
					pQuest,
					pSubject,
					NPC_DIALOG_OK_CANCEL,
					pQuestDef->nCompleteDialog,
					pQuestDef->nRewardDialog );

				return QMR_NPC_TALK;
			}
		}
		else
		{
			if ( s_QuestIsRewarder( pQuest,  pSubject ) )
			{
			
				// start escorting if not currently escorting
				s_sStartEscort( pQuest, pSubject );
				
				// Incomplete: tell the player to go kill the monster
				s_QuestDisplayDialog(
					pQuest,
					pSubject,
					NPC_DIALOG_OK,
					pQuestDef->nIncompleteDialog );

				return QMR_NPC_TALK;
			}
		}
		break;		
	}
#endif
	return QMR_OK;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT sQuestMessageTalkStopped(
	QUEST *pQuest,
	const QUEST_MESSAGE_TALK_STOPPED *pMessage )
{
	UNIT *pPlayer = QuestGetPlayer( pQuest );
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pTalkingTo = UnitGetById( pGame, pMessage->idTalkingTo );
	int nDialogStopped = pMessage->nDialog;
	QUEST_DATA_ESCORT * pQuestData = sQuestDataGet( pQuest );
	const QUEST_DEFINITION * pQuestDef = pQuest->pQuestDef;

	if ( nDialogStopped != INVALID_LINK )
	{
		if ( nDialogStopped == pQuestDef->nDescriptionDialog )
		{
			if ( QuestIsInactive( pQuest ) &&
				 s_QuestIsGiver( pQuest, pTalkingTo ) &&
				 s_QuestCanBeActivated( pQuest ) )
			{
				s_sStartEscort( pQuest, pTalkingTo );
			}
		}
		else if ( nDialogStopped == pQuestDef->nCompleteDialog )
		{
			if ( QuestStateGetValue( pQuest, QUEST_STATE_ESCORTTEMPLATE_FIND_EXIT ) == QUEST_STATE_COMPLETE &&
				 s_QuestIsRewarder( pQuest, pTalkingTo ) )
			{	
				// finish  up the quest
				QuestStateSet( pQuest, QUEST_STATE_ESCORTTEMPLATE_FOUND, QUEST_STATE_COMPLETE );
				if ( s_QuestComplete( pQuest ) )
				{
				
					// for templars, if the NPC has a shiled still we take it away
					// like they are giving it to the player
					if (UnitIsA( pPlayer, UNITTYPE_TEMPLAR ))
					{
						if ( s_QuestCheckForItem( pTalkingTo, pQuestData->nShield ) )
						{
							s_QuestRemoveItem( pTalkingTo, pQuestData->nShield );
						}
					}
									
					// give reward
					if (s_QuestOfferReward( pQuest, pTalkingTo ))
					{
						return QMR_OFFERING;
					}		
				}	
			}
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
	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pSubject = UnitGetById( pGame, pMessage->idSubject );

	if ( s_QuestIsGiver( pQuest, pSubject ) )
	{
		if ( eStatus == QUEST_STATUS_INACTIVE )
		{
			if ( s_QuestCanBeActivated( pQuest ) )
				return QMR_NPC_INFO_NEW;
		}
		else if ( eStatus == QUEST_STATUS_ACTIVE )
		{
			if ( QuestStateGetValue( pQuest, QUEST_STATE_ESCORTTEMPLATE_FIND_EXIT ) == QUEST_STATE_ACTIVE )
			{
				return QMR_OK;
			}
			return QMR_NPC_INFO_RETURN;
		}
	}

	return QMR_OK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static QUEST_MESSAGE_RESULT s_sQuestMessageObjectOperated( 
	QUEST *pQuest,
	const QUEST_MESSAGE_OBJECT_OPERATED *pMessage)
{
	if ( QuestGetStatus( pQuest ) != QUEST_STATUS_ACTIVE )
	{
		return QMR_OK;
	}

	QUEST_DATA_ESCORT * pQuestData = sQuestDataGet( pQuest );
	UNIT *pPlayer = pQuest->pPlayer;
	REF(pPlayer);
	UNIT *pTarget = pMessage->pTarget;
	int nTargetClass = UnitGetClass( pTarget );

	if ( nTargetClass == pQuestData->nEndObject )
	{
		// clear trigger
		UnitSetFlag( pTarget, UNITFLAG_TRIGGER, FALSE );
		// set states
		s_QuestStateSetForAll( pQuest, QUEST_STATE_ESCORTTEMPLATE_FIND_EXIT, QUEST_STATE_COMPLETE );
		s_QuestStateSetForAll( pQuest, QUEST_STATE_ESCORTTEMPLATE_FOUND, QUEST_STATE_ACTIVE );
		
		// reset templar AI
		UNIT *pEscorting = s_QuestGetGiver( pQuest );
		if (pEscorting)
		{
		
			// reset AI back to normal
			s_sEscortResetEscortNPC( pEscorting );
			
			// cannot be escored again
			UnitSetStat( pEscorting, STATS_ESCORT_COMPLETED, TRUE );
			
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
	{
		return QMR_OK;
	}

	GAME *pGame = QuestGetGame( pQuest );
	UNIT *pVictim = UnitGetById( pGame, pMessage->idVictim );
	if ( s_QuestIsGiver( pQuest, pVictim ) )
	{
		// quest fail!
		QuestStateSet( pQuest, QUEST_STATE_ESCORTTEMPLATE_FIND_EXIT, QUEST_STATE_FAIL );
		s_QuestFail( pQuest );
	}

	return QMR_OK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static QUEST_MESSAGE_RESULT c_sQuestMesssageStateChanged( 
	QUEST *pQuest,
	const QUEST_MESSAGE_STATE_CHANGED *pMessage)
{
	int nQuestState = pMessage->nQuestState;
	QUEST_STATE_VALUE eValue = 	pMessage->eValueNew;

	if ( nQuestState == QUEST_STATE_ESCORTTEMPLATE_FIND_EXIT )
	{
		if ( eValue == QUEST_STATE_ACTIVE )
		{
			ASSERT( QuestGetStatus( pQuest ) == QUEST_STATUS_ACTIVE );
			c_sChangeEscortNPCName( pQuest );
		}
		else if ( eValue == QUEST_STATE_FAIL )
		{
			UIShowMessage( QUIM_DIALOG, DIALOG_ESCORTTEMPLATE_FAIL );
		}
		else if ( eValue == QUEST_STATE_COMPLETE )
		{
			UIShowMessage( QUIM_DIALOG, DIALOG_ESCORTTEMPLATE_REACHED );
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
	QUEST_DATA_ESCORT * pData = sQuestDataGet( pQuest );
	ASSERTX_RETVAL(pMessage && pMessage->nLevelDefId != INVALID_ID, QMR_OK, "expected valid quest message data" );
	ASSERTX_RETVAL(pData, QMR_OK, "expected quest data" );

	const LEVEL_DEFINITION *pLevelDef = LevelDefinitionGet( pMessage->nLevelDefId );
	if ( pLevelDef == NULL || pLevelDef->nNextLevel == INVALID_LINK )
	{
		return QMR_DRLG_RULE_FORBIDDEN;
	}

	if ( ( pData->nDRLG == pMessage->nDRLGDefId ) && ( QuestGetStatus( pQuest ) >= QUEST_STATUS_COMPLETE ) )
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
		case QM_SERVER_OBJECT_OPERATED:
		{
			const QUEST_MESSAGE_OBJECT_OPERATED *pObjectOperatedMessage = (const QUEST_MESSAGE_OBJECT_OPERATED *)pMessage;
			return s_sQuestMessageObjectOperated( pQuest, pObjectOperatedMessage );
		}

		//----------------------------------------------------------------------------
		case QM_SERVER_MONSTER_KILL:
		{
			const QUEST_MESSAGE_MONSTER_KILL *pUIKillMessage = (const QUEST_MESSAGE_MONSTER_KILL *)pMessage;
			return sQuestMessageMonsterKill( pQuest, pUIKillMessage );
		}		

		//----------------------------------------------------------------------------
		case QM_SERVER_CAN_RUN_DRLG_RULE:
		{
			const QUEST_MESSAGE_CAN_RUN_DRLG_RULE * pRuleData = (const QUEST_MESSAGE_CAN_RUN_DRLG_RULE *)pMessage;
			return sQuestCanRunDRLGRule( pQuest, pRuleData );
		}

		//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
		case QM_CLIENT_QUEST_STATE_CHANGED:
		{
			const QUEST_MESSAGE_STATE_CHANGED *pStateMessage = (const QUEST_MESSAGE_STATE_CHANGED *)pMessage;
			return c_sQuestMesssageStateChanged( pQuest, pStateMessage );		
		}
#endif// !ISVERSION(SERVER_VERSION)
	}

	return QMR_OK;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestEscortFree(
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
	QUEST_DATA_ESCORT * pQuestData)
{
	pQuestData->nTemplarAI	= QuestUseGI( pQuest, GI_AI_TEMPLAR_ESCORT );
	pQuestData->nEndObject	= QuestUseGI( pQuest, GI_OBJECT_ESCORT_END );
	pQuestData->nIdleAI		= QuestUseGI( pQuest, GI_AI_IDLE );
	pQuestData->nShield		= QuestUseGI( pQuest, GI_TREASURE_LOST_REWARD_TEMPLAR );
	pQuestData->nDRLG		= QuestUseGI( pQuest, GI_DRLG_BLOODMOON );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void QuestEscortInit(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;
	ASSERTX_RETURN( pQuest, "Invalid quest" );	
	
	// message handler
	pQuest->pfnMessageHandler = sQuestMessageHandler;		
			
	// allocate quest specific data
	ASSERTX( pQuest->pQuestData == NULL, "Expected NULL quest data" );
	GAME * pGame = UnitGetGame( pQuest->pPlayer );
	QUEST_DATA_ESCORT * pQuestData = ( QUEST_DATA_ESCORT * )GMALLOCZ( pGame, sizeof( QUEST_DATA_ESCORT ) );
	sQuestDataInit( pQuest, pQuestData );
	pQuest->pQuestData = pQuestData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_QuestEscortOnEnterGame(
	const QUEST_FUNCTION_PARAM &tParam)
{
	QUEST *pQuest = tParam.pQuest;

	if ( TESTBIT( tParam.dwQuestParamFlags, QPF_FIRST_GAME_JOIN_BIT ) &&
		 QuestIsActive( pQuest ) &&
		 QuestStateGetValue( pQuest, QUEST_STATE_ESCORTTEMPLATE_FIND_EXIT ) == QUEST_STATE_ACTIVE )
	{
		// reset escort, DRLG may have changed
		s_QuestDeactivate( pQuest );
	}
}