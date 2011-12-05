//----------------------------------------------------------------------------
// FILE: c_quests.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "c_message.h"
#include "s_message.h"
#include "c_quests.h"
#include "console.h"
#include "dialog.h"
#include "fontcolor.h"
#include "globalindex.h"
#include "prime.h"
#include "language.h"
#include "stringtable.h"
#include "quests.h"
#include "uix.h"
#include "states.h"
#include "items.h"
#include "offer.h"
#include "s_reward.h"
#include "npc.h"
#include "monsters.h"
#include "LevelAreas.h"
#include "uix_money.h"
#include "uix_graphic.h"
#include "uix_chat.h"
#include "faction.h"
#include "chat.h"
#include "treasure.h"
#include "stringreplacement.h"

//----------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------

#define MAX_QUEST_MONSTER_INITS			32
static UNITID c_sgMonsterInitList[ MAX_QUEST_MONSTER_INITS ];
static int c_sgnNumMonsterInits = 0;

#define MAX_QUEST_VISIBLE_INITS			16
static UNITID c_sgQuestVisibleList[ MAX_QUEST_VISIBLE_INITS ];
static int c_sgnNumQuestVisibleInits = 0;

BOOL gbFullQuestLog = FALSE;

//----------------------------------------------------------------------------
// Definitions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
void c_QuestSetState(
	int nQuest,
	int nQuestState,
	QUEST_STATE_VALUE eValue,
	BOOL bRestore)
{
	GAME * pGame = AppGetCltGame();
	ASSERT_RETURN( pGame );
	UNIT * pPlayer = GameGetControlUnit( pGame );
	ASSERT_RETURN( pPlayer );

	// for now save client active quest
	QUEST_GLOBALS *pQuestGlobals = QuestGlobalsGet( pPlayer );
	ASSERT_RETURN(pQuestGlobals);
	pQuestGlobals->nActiveClientQuest = nQuest;
		
	QUEST *pQuest = QuestGetQuest( pPlayer, nQuest );
	if ( !pQuest )
		return;
		
	QUEST_STATE *pQuestState = QuestStateGetPointer( pQuest, nQuestState );	
	ASSERTX_RETURN( pQuestState, "Expected state" );
		
	// set new value
	QUEST_STATE_VALUE eValueOld = pQuestState->eValue;
	pQuestState->eValue = eValue;
	
	if ( !bRestore )
	{
	
		// send client side event
		c_QuestEventStateChanged( pQuest, nQuestState, eValueOld, eValue );

		// if this state has a full log, save it as the active full log
		const QUEST_STATE_DEFINITION *pStateDef = QuestStateGetDefinition( nQuestState );
		if (pStateDef->nDialogFullDesc != INVALID_LINK)
		{
			pQuest->nQuestStateActiveLog = nQuestState;
		}
		
	}
		
	// update quest log
	UIUpdateQuestLog( QUEST_LOG_UI_UPDATE_TEXT, nQuest );

}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sQuestTemplatePrintRewardStatsInfo(
	UNIT *pPlayer,
	QUEST *pQuest,
	WCHAR *puszBuffer,
	int nBufferSize)
{
	ASSERT(puszBuffer);

	// zero buffer
	puszBuffer[ 0 ] = 0;	
	WCHAR szStatReward[ 32 ];									

	int nExperience = QuestGetExperienceReward( pQuest );
	if ( nExperience > 0 )
	{
		PStrPrintf( szStatReward, 
			arrsize( szStatReward ), 
			GlobalStringGet( GS_EXPERIENCE_WITH_LABEL ), 
			nExperience );

		if ( puszBuffer[ 0 ] != 0 )
			PStrCat( puszBuffer, L"\n", nBufferSize );
		PStrCat( puszBuffer, szStatReward, nBufferSize );
	}			

	int nMoney = QuestGetMoneyReward( pQuest );
	if ( nMoney > 0 )
	{
		// get money as localized string
		const int MAX_MONEY_STRING = 128;
		WCHAR uszMoney[ MAX_MONEY_STRING ];
		PStrPrintf( uszMoney, MAX_MONEY_STRING, L"%d", nMoney );
		WCHAR uszMoneyLocalized[ MAX_MONEY_STRING ];
		UIMoneyLocalizeMoneyString( uszMoneyLocalized, MAX_MONEY_STRING, uszMoney );

		// format the money amount with text label
		WCHAR uszMoneyWithText[ MAX_MONEY_STRING ];
		PStrPrintf( 
			uszMoneyWithText, 
			MAX_MONEY_STRING, 
			GlobalStringGet( GS_MONEY_WITH_TEXT ), 
			uszMoneyLocalized );	

		if ( puszBuffer[ 0 ] != 0 )
			PStrCat( puszBuffer, L"\n", nBufferSize );
		PStrCat( puszBuffer, uszMoneyWithText, nBufferSize );
	}

	int nSkillPoints = QuestGetSkillPointReward( pQuest );
	if ( nSkillPoints > 0 )
	{
		PStrPrintf( szStatReward, 
			arrsize( szStatReward ), 
			GlobalStringGet( GS_SKILL_POINTS_WITH_LABEL ), 
			nSkillPoints );

		if ( puszBuffer[ 0 ] != 0 )
			PStrCat( puszBuffer, L"\n", nBufferSize );
		PStrCat( puszBuffer, szStatReward, nBufferSize );
	}

	int nStatPoints = QuestGetStatPointReward( pQuest );
	if ( nStatPoints > 0 )
	{
		PStrPrintf( szStatReward, 
			arrsize( szStatReward ), 
			GlobalStringGet( GS_STAT_POINTS_WITH_LABEL ), 
			nStatPoints );

		if ( puszBuffer[ 0 ] != 0 )
			PStrCat( puszBuffer, L"\n", nBufferSize );
		PStrCat( puszBuffer, szStatReward, nBufferSize );
	}

	int nFactionScoreDelta = pQuest->pQuestDef->nFactionAmountReward;
	int nFaction = pQuest->pQuestDef->nFactionTypeReward;
	if (nFactionScoreDelta != 0 && nFaction != INVALID_LINK)
	{
		const int MESSAGE_SIZE = 2048;
		WCHAR uszMessage[ MESSAGE_SIZE ];
		PStrPrintf( 
			uszMessage, 
			MESSAGE_SIZE,
			GlobalStringGet( GS_FACTION_SCORE_REWARD ),
			nFactionScoreDelta,
			FactionGetDisplayName( nFaction ));
			/*,
			FactionScoreGet( pPlayer, nFaction ),
			FactionStandingGetDisplayName( FactionGetStanding( pPlayer, nFaction ) ));
			*/

		if ( puszBuffer[ 0 ] != 0 )
			PStrCat( puszBuffer, L"\n", nBufferSize );
		PStrCat( puszBuffer, uszMessage, nBufferSize );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_QuestEventQuestStatus(
	int nQuest,
	int nDifficulty,
	QUEST_STATUS eStatus,
	BOOL bRestore )
{
	GAME * pGame = AppGetCltGame();
	ASSERT_RETURN( pGame );
	UNIT * pPlayer = GameGetControlUnit( pGame );
	ASSERTX_RETURN( pPlayer, "Expected control unit" );
	
	// for now save client active quest (when completing a quest, we clear the ui)
	QUEST_GLOBALS *pQuestGlobals = QuestGlobalsGet( pPlayer );
	ASSERT_RETURN(pQuestGlobals);

	pQuestGlobals->nLevelNextStoryDef = INVALID_ID;
	pQuestGlobals->bStoryQuestActive = FALSE;

	if (eStatus == QUEST_STATUS_COMPLETE)
	{
		if (AppTestDebugFlag( ADF_COLIN_BIT ))
		{
			QUEST *pQuest = QuestGetQuestByDifficulty( pPlayer, nQuest, nDifficulty );
			if (pQuest)
				ConsoleString( 
					CONSOLE_SYSTEM_COLOR, 
					GlobalStringGet( GS_QUEST_COMPLETE ),
					StringTableGetStringByIndex( pQuest->nStringKeyName ));
		}
		pQuestGlobals->nActiveClientQuest = INVALID_LINK;		
	}
	else
	{
		pQuestGlobals->nActiveClientQuest = nQuest;
	}

	// save status
	QUEST *pQuest = QuestGetQuestByDifficulty( pPlayer, nQuest, nDifficulty );
	if ( !pQuest )
		return;
	QUEST_STATUS eStatusOld = pQuest->eStatus;
	pQuest->eStatus = eStatus;

	const QUEST_DEFINITION * pQuestDef = pQuest->pQuestDef;
	ASSERT_RETURN( pQuestDef );
	
	// update logs
	if (eStatus == QUEST_STATUS_ACTIVE)
	{
		// if I get new quest, turn on the quest log
		UIUpdateQuestLog( QUEST_LOG_UI_VISIBLE, nQuest );

		// if it's a story quest, automatically start tracking it
		if (pQuestDef->bAutoTrackOnActivate && !bRestore)
		{
			QuestAutoTrack(pQuest);
		}
	}
	else
	{
		// show log for now
		UIUpdateQuestLog( QUEST_LOG_UI_UPDATE, nQuest );
	}

	// display complete message, unless the quest is being restored
	if ( eStatus == QUEST_STATUS_CLOSED && !bRestore && !pQuestDef->bSkipCompleteFanfare )
	{
		WCHAR szQuestComplete[MAX_STRING_ENTRY_LENGTH];
		if ( pQuestDef->nEndOfAct != 0 )
		{
			const WCHAR * puszEndOfActString = GlobalStringGet( GS_END_OF_ACT );
			PStrPrintf( szQuestComplete, MAX_STRING_ENTRY_LENGTH, puszEndOfActString, pQuestDef->nEndOfAct );
		}
		else
		{
			const WCHAR *puszCompleteString = GlobalStringGet( GS_QUEST_COMPLETE );
			const WCHAR *puszQuestName = StringTableGetStringByIndex( pQuest->nStringKeyName );
			PStrPrintf( szQuestComplete, MAX_STRING_ENTRY_LENGTH, puszCompleteString, puszQuestName );
		}

		WCHAR szStatRewards[MAX_STRING_ENTRY_LENGTH];
		szStatRewards[ 0 ] = 0;
		sQuestTemplatePrintRewardStatsInfo( pPlayer, pQuest, szStatRewards, arrsize( szStatRewards ) );

		UIShowQuickMessage( szQuestComplete, LEVELCHANGE_FADEOUTMULT_DEFAULT ); // "quest complete.."
		if ( szStatRewards[ 0 ] != 0 )
		{
			WCHAR szStatsAwarded[MAX_STRING_ENTRY_LENGTH];
			const WCHAR *puszRewardReceivedHeaderString = GlobalStringGet( GS_REWARD_RECEIVED_HEADER );
			PStrCopy( szStatsAwarded, puszRewardReceivedHeaderString, arrsize( szStatsAwarded ) );//, puszQuestName );
			PStrCat( szStatsAwarded, L"\n", arrsize( szStatsAwarded ) );
			PStrCat( szStatsAwarded, szStatRewards, arrsize( szStatsAwarded ) );
			UIAddChatLine( CHAT_TYPE_QUEST, ChatGetTypeColor(CHAT_TYPE_QUEST), szStatsAwarded, FALSE ); // "stat rewards, gold, xp..."
		}
	}

	// send client side quest event
	QUEST_MESSAGE_STATUS tMessage;
	tMessage.eStatusOld = eStatusOld;
	
	// send it only to this quest (we could send to others later if we want)
	ASSERTX( pQuest, "Expected quest" );
	QuestSendMessageToQuest( pQuest, QM_CLIENT_QUEST_STATUS, &tMessage );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_QuestTrack(
	QUEST * pQuest,
	BOOL bEnableTracking)
{
	ASSERT_RETURN(pQuest);

	MSG_CCMD_QUEST_TRACK msg;
	msg.nQuestID = pQuest->nQuest;
	msg.bTracking = bEnableTracking;
	c_SendMessage(CCMD_QUEST_TRACK, &msg);	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_AutoTrackQuests(
	BOOL bEnableTracking)
{
	MSG_CCMD_AUTO_TRACK_QUESTS msg;
	msg.bTracking = bEnableTracking;
	c_SendMessage(CCMD_AUTO_TRACK_QUESTS, &msg);	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_QuestEventOpenUI(
	UI_ELEMENT eElement)
{

	// setup message
	QUEST_MESSAGE_OPEN_UI tMessage;
	tMessage.eElement = eElement;
		
	// send to quests
	GAME *pGame = AppGetCltGame();
	if (pGame)
		MetagameSendMessage( GameGetControlUnit(pGame), QM_CLIENT_OPEN_UI, &tMessage );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_QuestEventUnitVisibleTest(
	UNIT * unit )
{
	ASSERT_RETURN( c_sgnNumQuestVisibleInits < MAX_QUEST_VISIBLE_INITS );
	c_sgQuestVisibleList[ c_sgnNumQuestVisibleInits++ ] = UnitGetId( unit );
	if( AppIsHellgate() )
	{
		c_QuestSetUnitVisibility( unit, FALSE );
	}
	//tugboat handles the visible Test differently. It sets the unit invisible at the init of the level.
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_QuestEventReloadUI(
	UNIT *pPlayer )
{
	ASSERTX_RETURN( pPlayer, "Invalid player" );

	if ( AppGetCltGame() )
	{
		// tell the server
		MSG_CCMD_UI_RELOAD msg;
		c_SendMessage(CCMD_UI_RELOAD, &msg);	

		// tell the client
		MetagameSendMessage( pPlayer, QM_SERVER_RELOAD_UI, NULL );
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_QuestSetUnitVisibility(
	UNIT * unit,
	BOOL bVisible )
{
	ASSERT_RETURN( unit );
	ASSERT_RETURN( IS_CLIENT( UnitGetGame( unit ) ) );

	if ( bVisible )
		c_StateClear( unit, UnitGetId( unit ), STATE_QUEST_HIDDEN, 0 );
	else
		c_StateSet( unit, unit, STATE_QUEST_HIDDEN, 0, 0, INVALID_ID );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_QuestEventSkillPoint(
	void)
{
	// send to quests
	GAME *pGame = AppGetCltGame();
	MetagameSendMessage( GameGetControlUnit(pGame), QM_CLIENT_SKILL_POINT, NULL );
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_QuestEventMonsterInit(
	UNIT * monster )
{
	ASSERT_RETURN( c_sgnNumMonsterInits < MAX_QUEST_MONSTER_INITS );
	c_sgMonsterInitList[ c_sgnNumMonsterInits++ ] = UnitGetId( monster );
}

//----------------------------------------------------------------------------
// we want to make sure we can pick up certain items.
//----------------------------------------------------------------------------
BOOL c_QuestEventCanPickup(
	UNIT *pPlayer,
	UNIT *pAttemptingToPickedUp )
{

	ASSERTX_RETFALSE( pPlayer , "Expected player" );
	ASSERTX_RETFALSE( pAttemptingToPickedUp, "Invalid picked up unit" );
	if( UnitIsA( pPlayer, UNITTYPE_PLAYER ) == FALSE )
		return TRUE;
	// setup message
	QUEST_MESSAGE_ATTEMPTING_PICKUP tMessage;
	tMessage.pPlayer = pPlayer;
	tMessage.pPickedUp = pAttemptingToPickedUp;
	tMessage.nQuestIndex = UnitGetStat( pAttemptingToPickedUp, STATS_QUEST_ITEM, UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT ) );

	// send to quests
	QUEST_MESSAGE_RESULT result = MetagameSendMessage( pPlayer, QM_CLIENT_ATTEMPTING_PICKUP, &tMessage );
	return result == QMR_OK;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_QuestsUpdate(
	GAME *pGame)
{
	ASSERTX_RETURN( IS_CLIENT( pGame ), "Client only" );

	if ( !c_sgnNumMonsterInits && !c_sgnNumQuestVisibleInits )
		return;

	UNIT *pPlayer = GameGetControlUnit( pGame );
	if ( !pPlayer )
		return;

	if ( UnitIsInLimbo( pPlayer ) )
		return;

	if ( c_sgnNumMonsterInits )
	{
		for ( int i = 0; i < c_sgnNumMonsterInits; i++ )
		{
			QUEST_MESSAGE_MONSTER_INIT tMessage;
			tMessage.pMonster = UnitGetById( pGame, c_sgMonsterInitList[i] );
			ASSERT_RETURN( tMessage.pMonster );

			MetagameSendMessage( pPlayer, QM_CLIENT_MONSTER_INIT, &tMessage );
		}
		c_sgnNumMonsterInits = 0;
	}

	if ( c_sgnNumQuestVisibleInits )
	{
		for ( int i = 0; i < c_sgnNumQuestVisibleInits; i++ )
		{
			UNIT * pUnit = UnitGetById( pGame, c_sgQuestVisibleList[i] );			
			if( AppIsHellgate() )
			{
				ASSERT_RETURN( pUnit );
				BOOL bVisible = QuestsUnitIsVisible( pPlayer, pUnit );
				c_QuestSetUnitVisibility( pUnit, bVisible );
			}
			else
			{
				if( pUnit )
				{
					//Tugboat handles units becoming visible by script. No need to force it here.
					QuestsUnitIsVisible( pPlayer, pUnit );
				}
			}
		}
		c_sgnNumQuestVisibleInits = 0;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_QuestMapGetIconForLevel(
	int nLevelDefID,
	char * szFrameNameBuf,
	int nFrameNameBufSize,
	DWORD& dwIconColor)
{
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_QuestEventEndQuestSetup(	
	void)
{
	GAME * pGame = AppGetCltGame();
	UNIT * pPlayer = GameGetControlUnit( pGame );
	QUEST_GLOBALS * pQuestGlobals = QuestGlobalsGet( pPlayer );
	ASSERT_RETURN(pQuestGlobals);
	
	// the server has sent us all quest status and states, set our quest log to
	// the first one that is active, or to none at all
	BOOL bFound = FALSE;
	for (int nQuest = 0; nQuest < NUM_QUESTS; ++nQuest)
	{
		QUEST * pQuest = QuestGetQuest( pPlayer, nQuest );
		if (!pQuest)
			continue;
		QUEST_STATUS eStatus = QuestGetStatus( pQuest );
		
		if (eStatus == QUEST_STATUS_ACTIVE)
		{
			pQuestGlobals->nActiveClientQuest = nQuest;
			bFound = TRUE;
			break;
		}
		
	}

	// set active quest to invalid
	if (bFound == FALSE)
	{
		pQuestGlobals->nActiveClientQuest = INVALID_LINK;
	}

	// mark that we have received all quest states from the server now
	pQuestGlobals->bAllInitialQuestStatesReceived = TRUE;	
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_QuestIsAllInitialQuestStateReceived(
	void)
{
	GAME * pGame = AppGetCltGame();
	UNIT * pPlayer = GameGetControlUnit( pGame );
	QUEST_GLOBALS *pQuestGlobals = QuestGlobalsGet( pPlayer );
	ASSERT_RETFALSE(pQuestGlobals);
	return pQuestGlobals->bAllInitialQuestStatesReceived;
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_QuestAbortCurrent( GAME * game, UNIT * player )
{
	ASSERT( IS_CLIENT( game ) );
	ASSERT( player );

	if ( UnitTestFlag( player, UNITFLAG_QUESTSTORY ) )
	{
		MSG_CCMD_ABORTQUEST msg;
		msg.idPlayer = UnitGetId( player );
		c_SendMessage(CCMD_ABORTQUEST, &msg);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_QuestIsVisibleInLog(
	QUEST *pQuest)
{

	if (QuestsAreDisabled( QuestGetPlayer( pQuest ) ) ||
		pQuest->pQuestDef->bHideQuestLog)
		return FALSE;

	// I'm making this a switch/case so that if we add another status it shows
	// up in a search on existing status values
	switch (QuestGetStatus( pQuest ))
	{
		case QUEST_STATUS_INVALID:		return FALSE;
		case QUEST_STATUS_INACTIVE:		return FALSE;
		case QUEST_STATUS_ACTIVE:		return TRUE;
		case QUEST_STATUS_OFFERING:		return TRUE;		
		case QUEST_STATUS_COMPLETE:		return FALSE;
		case QUEST_STATUS_CLOSED:		return FALSE;
		case QUEST_STATUS_FAIL:			return FALSE;
		default:						return FALSE;
	
	}	

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_QuestEventStateChanged(
	QUEST *pQuest,
	int nQuestState,
	QUEST_STATE_VALUE eValueOld,
	QUEST_STATE_VALUE eValueNew)
{
	// skip this if we are getting the inital states still...
	QUEST_GLOBALS * pQuestGlobals = QuestGlobalsGet( pQuest->pPlayer );
	if ( !pQuestGlobals->bAllInitialQuestStatesReceived )
		return;

	// setup message
	QUEST_MESSAGE_STATE_CHANGED tMessage;
	tMessage.nQuestState = nQuestState;
	tMessage.eValueOld = eValueOld;
	tMessage.eValueNew = eValueNew;

	// send it only to this quest (we could send to others later if we want)
	QuestSendMessageToQuest( pQuest, QM_CLIENT_QUEST_STATE_CHANGED, &tMessage );

}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const WCHAR *c_QuestStateGetLogText(
	UNIT *pPlayer,
	QUEST *pQuest,	
	const QUEST_STATE *pState,
	int *pnColor)
{
	ASSERTX_RETNULL( pState, "Expected quest state" );
	ASSERTX_RETNULL( pPlayer, "Expected unit" );
	ASSERTX_RETNULL( UnitIsA( pPlayer, UNITTYPE_PLAYER ), "Expected player" );

	// tugboat not using this right now
	if (AppIsTugboat())
	{
		return NULL;
	}

	// get state def	
	const QUEST_STATE_DEFINITION *pStateDef = QuestStateGetDefinition( pState->nQuestState );
	ASSERTX_RETNULL( pStateDef, "Expected quest state definition" );

	// get string index for log
	int nStringIndex = INVALID_LINK;
	for (int i = 0; i < MAX_QUEST_LOG_OPTIONS; ++i)
	{
		const QUEST_LOG_OPTION *ptOption = &pStateDef->tQuestLogOptions[ i ];
		if (ptOption->nString != INVALID_LINK)
		{
			if (ptOption->nUnitType == INVALID_LINK ||
				UnitIsA( pPlayer, ptOption->nUnitType))
			{
				nStringIndex = ptOption->nString;
				break;
			}
		}
	}

	// do they have a log string override?
	if ( pQuest->pQuestDef && ( pQuest->pQuestDef->nLogOverrideState == pState->nQuestState ) && ( pQuest->pQuestDef->nLogOverrideString != INVALID_LINK ) )
	{
		nStringIndex = pQuest->pQuestDef->nLogOverrideString;
	}

	// bail if there is no log		
	if (nStringIndex == INVALID_LINK)
	{
		return NULL;
	}

	QUEST_STATE_VALUE eStateValue = QuestStateGetValue( pQuest, pState );
	if ( ( eStateValue == QUEST_STATE_ACTIVE ) && !pStateDef->bShowLogOnActivate )
	{
		return NULL;
	}

	if ( ( eStateValue == QUEST_STATE_COMPLETE ) && !pStateDef->bShowLogOnComplete )
	{
		return NULL;
	}

	// assign color if wanted
	if (pnColor != NULL)
	{
		if (eStateValue == QUEST_STATE_ACTIVE)
		{
			*pnColor = FONTCOLOR_QUEST_LOG_STATE_ACTIVE;
		}
		else
		{
			*pnColor = FONTCOLOR_QUEST_LOG_STATE_OTHER;
		}
	}
	
	// translate
	return StringTableGetStringByIndex( nStringIndex );
		
}

//----------------------------------------------------------------------------
struct QUEST_DESCRIPTION_REPLACEMENT
{
	const WCHAR *puszToken;
	const WCHAR *puszReplacement;
	FONTCOLOR eColor;
};

//----------------------------------------------------------------------------
// Replaces tokens in a format string using the quest template data,
// return result in a buffer with specified size.
//----------------------------------------------------------------------------
void c_QuestReplaceStringTokens(
	UNIT *pPlayer,
	QUEST *pQuest,
	const WCHAR *puszFormat,
	WCHAR *puszBuffer,
	int nBufferSize )
{		
#define MAX_QUEST_ELEMENT_STRING_LENGTH (256)

	ASSERTX_RETURN( puszFormat != NULL && puszBuffer != NULL && pQuest != NULL && pPlayer != NULL, "unexpected NULL argument");
	GAME *pGame = AppGetCltGame();
	PStrCopy( puszBuffer, puszFormat, nBufferSize );

	const QUEST_DEFINITION *pQuestDefinition = QuestGetDefinition( pQuest->nQuest );
	ASSERT_RETURN( pQuestDefinition );

	int nDifficultyCurrent = UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT );

	// build string of targets
	WCHAR uszTargets[ MAX_QUEST_ELEMENT_STRING_LENGTH ];
	uszTargets[ 0 ] = 0;

	int nTargetUniqueName = UnitGetStat(pPlayer, STATS_SAVE_QUEST_HUNT_TARGET1_NAME, pQuest->nQuest, nDifficultyCurrent);
	int nObjectiveMonster = pQuestDefinition->nObjectiveMonster;
	if (nObjectiveMonster != INVALID_LINK)
	{
		if (nTargetUniqueName != MONSTER_UNIQUE_NAME_INVALID)
		{
			// look for unique names
			// for now we only support monster targets - this is just to remind me to fix this if we have other targets someday - colin

			// add to descriptive string of all our targets			
			MonsterGetUniqueNameWithTitle( 
				nTargetUniqueName, 
				nObjectiveMonster, 
				uszTargets,
				arrsize( uszTargets ) );
		}
		else
		{
			// no unique name added to monster, just use unit_data string
			const UNIT_DATA * pUnitData = UnitGetData(pGame, GENUS_MONSTER, nObjectiveMonster);
			if ( pUnitData != NULL )
				PStrCopy(
					uszTargets, 
					StringTableGetStringByIndex( pUnitData->nString ), 
					arrsize( uszTargets ) );
		}
	}
	
	// get level string
	WCHAR uszLevel[ MAX_QUEST_ELEMENT_STRING_LENGTH ];
	uszLevel[ 0 ] = 0;  // init to empty string	
	if ( pQuestDefinition->nLevelDefDestinations[0] == INVALID_LINK )
	{
		PStrCopy( 
			uszLevel, 
			GlobalStringGet( GS_ANY_AREA ), 
			MAX_QUEST_ELEMENT_STRING_LENGTH );			
	}
	else
	{
		const LEVEL_DEFINITION* pLevelDefinition = 
			LevelDefinitionGet( pQuestDefinition->nLevelDefDestinations[0] );
		ASSERTX( pLevelDefinition, "Unable to get level definition for quest" );
		PStrCopy( 
			uszLevel, 
			StringTableGetStringByIndex( pLevelDefinition->nLevelDisplayNameStringIndex ), 
			MAX_QUEST_ELEMENT_STRING_LENGTH );
	}

	// get level string
	WCHAR uszRewarderLevel[ MAX_QUEST_ELEMENT_STRING_LENGTH ];
	uszRewarderLevel[ 0 ] = 0;  // init to empty string	
	if ( pQuestDefinition->nLevelDefRewarderNPC != INVALID_LINK )
	{
		const LEVEL_DEFINITION* pLevelDefinition = 
			LevelDefinitionGet( pQuestDefinition->nLevelDefRewarderNPC );
		ASSERTX( pLevelDefinition, "Unable to get level definition for quest" );
		PStrCopy( 
			uszRewarderLevel, 
			StringTableGetStringByIndex( pLevelDefinition->nLevelDisplayNameStringIndex ), 
			arrsize( uszRewarderLevel ) );
	}

	// build string of things to collect
	WCHAR uszCollect[ MAX_QUEST_ELEMENT_STRING_LENGTH ];
	uszCollect[ 0 ] = 0;
	if (pQuestDefinition->nCollectItem != INVALID_LINK)
	{
		// get name
		DWORD dwFlags = 0;
		// SETBIT( dwFlags, INIF_MODIFY_NAME_BY_ANY_QUANTITY );
		ITEM_NAME_INFO tItemNameInfo;
		ItemGetNameInfo( tItemNameInfo, pQuestDefinition->nCollectItem, pQuestDefinition->nObjectiveCount, dwFlags );		
		PStrCopy( uszCollect, tItemNameInfo.uszName, MAX_QUEST_ELEMENT_STRING_LENGTH );
	}

#if (0)	// TODO cmarch: support mutiple collection item types?
	// how many different item types are there to collect
	int nNumCollectTypes = 0;
	for (int i = 0; i < arrsize( pQuestTemplate->tQuestCollect ); ++i)
	{
		const QUEST_COLLECT* pCollect = &pQuestTemplate->tQuestCollect[ i ];
		if (pCollect->nCount > 0)
		{
			nNumCollectTypes++;
		}	
	}

	// build string of things to collect
	WCHAR uszCollect[ MAX_QUEST_ELEMENT_STRING_LENGTH ];
	uszCollect[ 0 ] = 0;
	int nCurrentCollect = 0;
	for (int i = 0; i < arrsize( pQuestTemplate->tQuestCollect ); ++i)
	{
		const QUEST_COLLECT* pCollect = &pQuestTemplate->tQuestCollect[ i ];
		if (pCollect->nCount > 0)
		{
			// get name
			DWORD dwFlags = 0;
			// SETBIT( dwFlags, INIF_MODIFY_NAME_BY_ANY_QUANTITY );
			ITEM_NAME_INFO tItemNameInfo;
			ItemGetNameInfo( tItemNameInfo, pCollect->nItemClass, pCollect->nCount, dwFlags );		

			if (nCurrentCollect == 0)
			{
				// this is the first item in the list
				PStrCopy( uszCollect, tItemNameInfo.uszName, MAX_QUEST_ELEMENT_STRING_LENGTH );
			}
			else
			{
				// get the separator, this is the nth or the last item in the list
				const WCHAR *puszSep = NULL;
				if (nCurrentCollect == nNumCollectTypes - 1)
				{
					puszSep = GlobalStringGet( GS_ITEM_LIST_SEPARATOR_LAST );  // last collect
				}
				else
				{
					puszSep = GlobalStringGet( GS_ITEM_LIST_SEPARATOR );  // nth collect
				}

				// cat the separator
				PStrCat( uszCollect, puszSep, MAX_QUEST_ELEMENT_STRING_LENGTH );				   					   				

				// cat the name
				PStrCat( uszCollect, tItemNameInfo.uszName, MAX_QUEST_ELEMENT_STRING_LENGTH );
			}
			nCurrentCollect++;
		}
	}
#endif

#if (0) // TODO cmarch
	// build string for time limit
	WCHAR uszTimeLimit[ MAX_QUEST_ELEMENT_STRING_LENGTH ];
	uszTimeLimit[ 0 ] = 0;
	if (pQuestTemplate->nTimeLimitInMinutes > 0)
	{
		const WCHAR* puszMinutesFormat = GlobalStringGet( GS_TIME_MINUTES );
		PStrPrintf( uszTimeLimit, MAX_QUEST_ELEMENT_STRING_LENGTH, puszMinutesFormat, pQuestTemplate->nTimeLimitInMinutes );
	}

	// build string for trigger percent
	WCHAR uszTriggerPercent[ MAX_QUEST_ELEMENT_STRING_LENGTH ];
	uszTriggerPercent[ 0 ] = 0;	
	if( AppIsHellgate() )
	{
		PStrPrintf( uszTriggerPercent, MAX_QUEST_ELEMENT_STRING_LENGTH, L"%d", pQuestTemplate->nTriggerPercent );
	}
#endif

	// get player name
	WCHAR uszPlayerName[ MAX_CHARACTER_NAME ];
	PStrCopy( uszPlayerName, L"", arrsize(uszPlayerName) );
	UnitGetName( pPlayer, uszPlayerName, arrsize(uszPlayerName), 0 );

	const WCHAR * pszClassName = NULL;
	if (UnitIsA( pPlayer, UNITTYPE_TEMPLAR ))
	{
		pszClassName = GlobalStringGet( GS_TEMPLAR_CLASS );
	}
	else if (UnitIsA( pPlayer, UNITTYPE_CABALIST ))
	{
		pszClassName = GlobalStringGet( GS_CABALIST_CLASS );
	}
	else if (UnitIsA( pPlayer, UNITTYPE_HUNTER ))
	{
		pszClassName = GlobalStringGet( GS_HUNTER_CLASS );
	}

	WCHAR uszNPCGiver[128];
	uszNPCGiver[ 0 ] = 0;
	if ( pQuestDefinition->nCastGiver != INVALID_LINK )
	{
		SPECIES speciesGiver = QuestCastGetSpecies( pQuest, pQuestDefinition->nCastGiver );
		if ( speciesGiver != SPECIES_NONE )
		{
			GENUS eGenusGiver = (GENUS)GET_SPECIES_GENUS( speciesGiver );
			int nClassGiver = GET_SPECIES_CLASS( speciesGiver );
			const UNIT_DATA * pGiverData = UnitGetData(pGame, eGenusGiver, nClassGiver);
			if ( pGiverData != NULL )
			{
				PStrCopy(uszNPCGiver, StringTableGetStringByIndex( pGiverData->nString ), arrsize( uszNPCGiver ) );
				// end the string at the first newline, if any
				WCHAR *pszFirstNewline = PStrChr( uszNPCGiver, '\n' );
				if ( pszFirstNewline != NULL )
					*pszFirstNewline = 0;
			}
		}
	}

	WCHAR uszNPCRewarder[128];
	uszNPCRewarder[ 0 ] = 0;
	if ( pQuestDefinition->nCastRewarder != INVALID_LINK )
	{
		SPECIES speciesRewarder = QuestCastGetSpecies( pQuest, pQuestDefinition->nCastRewarder );
		if ( speciesRewarder != SPECIES_NONE )
		{
			GENUS eGenusRewarder = (GENUS)GET_SPECIES_GENUS( speciesRewarder );
			int nClassRewarder = GET_SPECIES_CLASS( speciesRewarder );
			const UNIT_DATA * pRewarderData = UnitGetData(pGame, eGenusRewarder, nClassRewarder);
			if ( pRewarderData != NULL )
			{
				PStrCopy(uszNPCRewarder, StringTableGetStringByIndex( pRewarderData->nString ), arrsize( uszNPCRewarder ) );
				// end the string at the first newline, if any
				WCHAR *pszFirstNewline = PStrChr( uszNPCRewarder, '\n' );
				if ( pszFirstNewline != NULL )
					*pszFirstNewline = 0;
			}
		}
	}

	// [OBJECTIVE MONSTER], singular noun
	WCHAR uszObjectiveMonster[128];
	uszObjectiveMonster[ 0 ] = 0;
	if ( pQuestDefinition->nObjectiveMonster  == INVALID_LINK )
	{
		PStrCopy( 
			uszObjectiveMonster, 
			GlobalStringGet( GS_ANY_DEMON ), 
			arrsize( uszObjectiveMonster ) );			
	}
	else
	{
		const UNIT_DATA * pMonsterData = 
			UnitGetData(pGame, GENUS_MONSTER, pQuestDefinition->nObjectiveMonster);
		if ( pMonsterData != NULL )
		{
			PStrCopy(
				uszObjectiveMonster, 
				StringTableGetStringByIndex( 
					pMonsterData->nString ), 
				arrsize( uszObjectiveMonster) );
		}
	}

	// [OBJECTIVE MONSTERS], singular or plural, depending on nObjectiveCount
	WCHAR uszObjectiveMonsters[128];
	uszObjectiveMonsters[ 0 ] = 0;
	if ( pQuestDefinition->nObjectiveMonster  == INVALID_LINK )
	{
		PStrCopy( 
			uszObjectiveMonsters, 
			GlobalStringGet( GS_ANY_DEMON ), 
			arrsize( uszObjectiveMonsters ) );			
	}
	else
	{
		const UNIT_DATA * pMonsterData = 
			UnitGetData(pGame, GENUS_MONSTER, pQuestDefinition->nObjectiveMonster);
		if ( pMonsterData != NULL )
		{
			// get the attributes to match for number
			DWORD dwAttributesToMatch = 
				StringTableGetGrammarNumberAttribute( 
					pQuestDefinition->nObjectiveCount > 1 ? 
					PLURAL : 
					SINGULAR );

			PStrCopy(
				uszObjectiveMonsters, 
				StringTableGetStringByIndexVerbose( 
				pMonsterData->nString, dwAttributesToMatch, 0, NULL, NULL ), 
				arrsize( uszObjectiveMonsters) );
		}
	}
	// [OPERATE OBJS], singular or plural, depending on nObjectiveCount
	WCHAR uszObjectiveObjects[128];
	uszObjectiveObjects[ 0 ] = 0;
	if ( pQuestDefinition->nObjectiveObject != INVALID_LINK )
	{
		const UNIT_DATA * pUnitData = 
			UnitGetData(pGame, GENUS_OBJECT, pQuestDefinition->nObjectiveObject);
		if ( pUnitData != NULL )
		{
			// get the attributes to match for number
			DWORD dwAttributesToMatch = 
				StringTableGetGrammarNumberAttribute( 
					pQuestDefinition->nObjectiveCount > 1 ? 
					PLURAL : 
					SINGULAR );

			PStrCopy(
				uszObjectiveObjects, 
				StringTableGetStringByIndexVerbose( 
					pUnitData->nString, dwAttributesToMatch, 0, NULL, NULL ), 
				arrsize( uszObjectiveObjects ) );
		}
	}

	// replace next area name
	const WCHAR * puszNextName = NULL;
	LEVEL * level = UnitGetLevel( pPlayer );
	if ( level )
	{
		const LEVEL_DEFINITION * leveldef_current = LevelDefinitionGet( level->m_nLevelDef );
		ASSERT_RETURN( leveldef_current );
		const LEVEL_DEFINITION * leveldef_next = LevelDefinitionGet( leveldef_current->nNextLevel);
		if ( leveldef_next )
		{
			puszNextName = StringTableGetStringByIndex( leveldef_next->nLevelDisplayNameStringIndex );
		}
	}


	// replace escort level destination name
	const WCHAR * puszEscortDestinationName = NULL;
	int nEscortLevelDest = UnitGetStat( pPlayer, STATS_QUEST_ESCORT_DESTINATION_LEVEL, pQuest->nQuest, nDifficultyCurrent );
	if (nEscortLevelDest != INVALID_ID)
	{
		const LEVEL_DEFINITION * leveldef_escortdest = LevelDefinitionGet( nEscortLevelDest );
		if ( leveldef_escortdest )
			puszEscortDestinationName = StringTableGetStringByIndex( leveldef_escortdest->nLevelDisplayNameStringIndex );
	}

	int nCollectObtained = UnitGetStat( pPlayer, STATS_QUEST_NUM_COLLECTED_ITEMS, pQuest->nQuest, 0, nDifficultyCurrent );
	nCollectObtained = MIN(nCollectObtained, pQuestDefinition->nObjectiveCount);
	WCHAR uszCollectObtained[ 16 ];
	uszCollectObtained[ 0 ] = 0;
		PStrPrintf( 
			uszCollectObtained, 
			arrsize( uszCollectObtained ), 
			L"%i", 
			nCollectObtained);
	
	const WCHAR *puszNameInLogOverride = NULL;
	int nStringKeyName = 
		( pQuestDefinition->nStringKeyNameInLogOverride == INVALID_LINK ) ?
			pQuestDefinition->nStringKeyNameOverride :
			pQuestDefinition->nStringKeyNameInLogOverride;
	if (nStringKeyName != INVALID_LINK)
	{
		puszNameInLogOverride = StringTableGetStringByIndex( nStringKeyName );
	}

	const WCHAR *puszObjectiveObjectOrMonster = puszNameInLogOverride;
	if ( puszObjectiveObjectOrMonster == NULL )
		puszObjectiveObjectOrMonster = ( pQuestDefinition->nObjectiveMonster != INVALID_LINK ) ? uszObjectiveMonster : uszObjectiveObjects;
	
	// [STARTING ITEM]
	const WCHAR *puszStartingItem = NULL;
	if ( pQuestDefinition->nStartingItemsTreasureClass != INVALID_LINK )
	{
		int nItemClass = 
			TreasureGetFirstItemClass( pGame, pQuestDefinition->nStartingItemsTreasureClass );
		if ( nItemClass != INVALID_LINK )
		{
			const UNIT_DATA * pUnitData = 
				UnitGetData(pGame, GENUS_ITEM, nItemClass );
			if ( pUnitData != NULL )
			{
				puszStartingItem = StringTableGetStringByIndex( pUnitData->nString );
			}
		}
	}

	WCHAR uszObjectiveCount[ 16 ];
	PStrPrintf( 
		uszObjectiveCount, 
		arrsize( uszObjectiveCount ), 
		L"%d", 
		pQuestDefinition->nObjectiveCount);

	WCHAR uszInfestationKills[ 16 ];
	PStrPrintf( 
		uszInfestationKills, 
		arrsize( uszInfestationKills ), 
		L"%d", 
		UnitGetStat( pPlayer, STATS_SAVE_QUEST_INFESTATION_KILLS, pQuest->nQuest, nDifficultyCurrent ) );

	WCHAR uszOperatedCount[ 16 ];
	PStrPrintf( 
		uszOperatedCount, 
		arrsize( uszOperatedCount ), 
		L"%d", 
		UnitGetStat( pPlayer, STATS_QUEST_OPERATED_OBJS, pQuest->nQuest, nDifficultyCurrent ) );

	const int MAX_BOSS_NAME = 256;
	WCHAR uszBoss[ MAX_BOSS_NAME ] = { 0 };
	if (pQuestDefinition->nMonsterBoss != INVALID_LINK)
	{
		const UNIT_DATA *pBossData = UnitGetData( pGame, GENUS_MONSTER, pQuestDefinition->nMonsterBoss );
		if (pBossData)
		{
			PStrPrintf( uszBoss, MAX_BOSS_NAME, L"%s", StringTableGetStringByIndex( pBossData->nString ) );
		}
	}

	const int MAX_CUSTOM_BOSS_NAME = 256;
	WCHAR uszCustomBoss[ MAX_CUSTOM_BOSS_NAME ] = { 0 };
	int nCustomBoss = UnitGetStat( pPlayer, STATS_QUEST_CUSTOM_BOSS_CLASS, pQuest->nQuest, nDifficultyCurrent );
	if (nCustomBoss != INVALID_LINK)
	{
		const UNIT_DATA *pBossData = UnitGetData( pGame, GENUS_MONSTER, nCustomBoss );
		if (pBossData)
		{
			PStrPrintf( uszCustomBoss, MAX_BOSS_NAME, L"%s", StringTableGetStringByIndex( pBossData->nString ) );
		}
	}
	// build other strings here
	// TODO: --->

	// build replacement table
	QUEST_DESCRIPTION_REPLACEMENT replacementTable[] = 
	{
		// token				replace with		color of replacement
		{ StringReplacementTokensGet( SR_QUEST_MONSTER_PROPER_NAME ),	uszTargets,						FONTCOLOR_INVALID },
		{ StringReplacementTokensGet( SR_QUEST_LEVEL ),					uszLevel,						FONTCOLOR_INVALID },
		{ StringReplacementTokensGet( SR_QUEST_REWARDER_LEVEL ),		uszRewarderLevel,				FONTCOLOR_INVALID },
		{ StringReplacementTokensGet( SR_CHARACTER_NAME ),				uszPlayerName,					FONTCOLOR_INVALID },
		{ StringReplacementTokensGet( SR_PLAYER_CLASS ),				pszClassName,					FONTCOLOR_INVALID },
		{ StringReplacementTokensGet( SR_QUEST_GIVER ),					uszNPCGiver,					FONTCOLOR_INVALID },
		{ StringReplacementTokensGet( SR_QUEST_REWARDER ),				uszNPCRewarder,					FONTCOLOR_INVALID },
		{ StringReplacementTokensGet( SR_NEXT_AREA ),					puszNextName,					FONTCOLOR_INVALID },
		{ StringReplacementTokensGet( SR_QUEST_ESCORT_DEST_LEVEL ),		puszEscortDestinationName,		FONTCOLOR_INVALID },
		{ StringReplacementTokensGet( SR_QUEST_COLLECT_OBJS ),			uszCollect,						FONTCOLOR_INVALID },
		{ StringReplacementTokensGet( SR_QUEST_COLLECT_OBTAINED ),		uszCollectObtained,				FONTCOLOR_INVALID },
		{ StringReplacementTokensGet( SR_INFESTATION_KILLS ),			uszInfestationKills,			FONTCOLOR_INVALID },
		{ StringReplacementTokensGet( SR_OBJECTIVE_COUNT ),				uszObjectiveCount,				FONTCOLOR_INVALID },
		{ StringReplacementTokensGet( SR_OBJECTIVE_MONSTER ),			uszObjectiveMonster,			FONTCOLOR_INVALID },
		{ StringReplacementTokensGet( SR_OBJECTIVE_MONSTERS ),			uszObjectiveMonsters,			FONTCOLOR_INVALID },
		{ StringReplacementTokensGet( SR_OPERATE_OBJECTS ),				uszObjectiveObjects,			FONTCOLOR_INVALID },
		{ StringReplacementTokensGet( SR_OBJECTIVE_OBJECT_OR_MONSTER ),	puszObjectiveObjectOrMonster,	FONTCOLOR_INVALID },
		{ StringReplacementTokensGet( SR_OPERATED_COUNT ),				uszOperatedCount,				FONTCOLOR_INVALID },
		{ StringReplacementTokensGet( SR_STARTING_ITEM ),				puszStartingItem ,				FONTCOLOR_INVALID },
		{ StringReplacementTokensGet( SR_NAME_IN_LOG_OVERRIDE ),		puszNameInLogOverride,			FONTCOLOR_INVALID },
		{ StringReplacementTokensGet( SR_BOSS ),						uszBoss,						FONTCOLOR_INVALID },
		{ StringReplacementTokensGet( SR_CUSTOM_BOSS ),					uszCustomBoss,					FONTCOLOR_INVALID },
		{ NULL,				NULL,				FONTCOLOR_INVALID }			// keep this last

	};

	// replace any tokens found with pieces we built (if we have them)
	FONTCOLOR eRegularColor = FONTCOLOR_WHITE;
	REF(eRegularColor);
	for (int i = 0; replacementTable[ i ].puszToken != NULL; ++i)
	{
		const QUEST_DESCRIPTION_REPLACEMENT *pReplacement = &replacementTable[ i ];
		if (pReplacement->puszReplacement != NULL)
		{

			// must have actual characters to replace
			if (PStrLen( pReplacement->puszReplacement ) > 0)
			{
				if (pReplacement->eColor != FONTCOLOR_INVALID)
				{
					WCHAR uszReplacement[MAX_QUEST_ELEMENT_STRING_LENGTH] = L"\0";
					UIAddColorToString( uszReplacement, pReplacement->eColor, arrsize(uszReplacement));
					PStrCat(uszReplacement, pReplacement->puszReplacement, arrsize(uszReplacement));
					UIAddColorReturnToString(uszReplacement, arrsize(uszReplacement));

					PStrReplaceToken( puszBuffer, nBufferSize, pReplacement->puszToken, uszReplacement);
				}
				else
				{
					// do replacement	
					PStrReplaceToken( puszBuffer, nBufferSize, pReplacement->puszToken, pReplacement->puszReplacement);
				}
			}

		}

	}
}

static void sQuestTemplateGetDescString(
	UNIT *pPlayer,
	QUEST *pQuest,
	WCHAR *puszDescBuffer,
	int nDescBufferSize )
{	
	const QUEST_DEFINITION *pQuestDefinition = QuestGetDefinition( pQuest->nQuest );
	ASSERT_RETURN( pQuestDefinition );

	// get description string
	const WCHAR *puszDescFormat = c_DialogTranslate( pQuestDefinition->nDescriptionDialog );
	c_QuestReplaceStringTokens( 
		pPlayer,
		pQuest,
		puszDescFormat,
		puszDescBuffer, 
		nDescBufferSize );
}

#if (0) // TODO cmarch
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sQuestGetDescString(
							   UNIT *pPlayer,
							   const QUEST_QUEST_DEFINITION_TUGBOAT *pQuestDefinition,
							   WCHAR *puszDescBuffer,
							   int nDescBufferSize )
{		
#define MAX_QUEST_ELEMENT_STRING_LENGTH (256)


	// get description string

	PStrCopy( puszDescBuffer, StringTableGetStringByIndex( c_QuestGetQuestDialogTextID( pPlayer, pQuestDefinition ) ), nDescBufferSize );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sQuestGetStatusString( 
								 const QUEST_TEMPLATE *pQuestTemplate,
								 QUEST_STATUS eStatus, 
								 DWORD dwAcceptedTick,
								 WCHAR *puszStatusBuffer, 
								 int nStatusBufferSize)
{	
	GAME *pGame = AppGetCltGame();

	BOOL bUseDefaultLabelStatus = TRUE;
	if (pQuestTemplate != NULL)
	{				
		int nExterminateGoal = QuestTemplateGetExterminateGoal( pQuestTemplate );

		// time limited quests display a timer instead of the status string			
		if ( pQuestTemplate->nTimeLimitInMinutes > 0 &&
			(eStatus == QUEST_STATUS_ACTIVE || eStatus == QUEST_STATUS_COMPLETE))
		{
			// update quest timer when we have an actual instance (if necessary)
			sQuestGetTimerStatusString( 
				pGame, 
				pQuestTemplate, 
				eStatus, 
				dwAcceptedTick,
				puszStatusBuffer, 
				nStatusBufferSize );
			bUseDefaultLabelStatus = FALSE;					
		}

		// extermination quests display the number left to exterminate instead 
		// of status string when the quest is active
		else if(eStatus == QUEST_STATUS_ACTIVE && nExterminateGoal > 0)
		{
			int nExterminateRemaining = nExterminateGoal - pQuestTemplate->nExterminatedCount;
			if (nExterminateRemaining < 0)
			{
				nExterminateRemaining = 0;
			}
			PStrPrintf( 
				puszStatusBuffer, 
				nStatusBufferSize, 
				GlobalStringGet( GS_NUMBER_REMAINING ), 
				nExterminateRemaining );
			bUseDefaultLabelStatus = FALSE;
		}

	}

	// set default status description
	if (bUseDefaultLabelStatus)
	{
		const WCHAR *puszStatusText = L"";
		QUEST_STATUS_LABEL *pQuestStatusLabel = sFindQuestStatusLabel( eStatus );
		if (pQuestStatusLabel && pQuestStatusLabel->pszStringKey != NULL)
		{
			puszStatusText = StringTableGetStringByKey( pQuestStatusLabel->pszStringKey );
		}
		PStrPrintf( puszStatusBuffer, nStatusBufferSize, puszStatusText);
	}

}
#endif



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sQuestTemplateGetRewardString(
	QUEST* pQuest,
	WCHAR *puszRewardBuffer,
	int nRewardBufferSize)
{
	// clear text
	puszRewardBuffer[ 0 ] = 0;		

	const QUEST_DEFINITION *pQuestDef = QuestGetDefinition( pQuest );
	ASSERT_RETURN( pQuestDef );

	// go through rewards
	UNIT* pItems[ MAX_OFFERINGS ];
	ITEMSPAWN_RESULT tOfferItems;
	tOfferItems.pTreasureBuffer = pItems;
	tOfferItems.nTreasureBufferSize = arrsize( pItems);
	
	QuestGetExistingRewardItems( pQuest, tOfferItems );

	for (int i = 0; i < tOfferItems.nTreasureBufferCount; ++i)
	{
		UNIT* pReward = tOfferItems.pTreasureBuffer[i];
		WCHAR uszItemTextDesc[ MAX_ITEM_NAME_STRING_LENGTH ];
		if ( AppIsHellgate() )
		{			
			if (i > 0)
				PStrCat( puszRewardBuffer, L" ", nRewardBufferSize );
			ItemGetEmbeddedTextModelString( pReward, uszItemTextDesc, arrsize(uszItemTextDesc), TRUE );
		}
		else
		{
			DWORD dwNameFlags = 0;			
			SETBIT( dwNameFlags, UNF_EMBED_COLOR_BIT );									
			UnitGetName( pReward, uszItemTextDesc, arrsize(uszItemTextDesc), dwNameFlags );
		}

		// append to reward descriptions
		PStrCat( puszRewardBuffer, uszItemTextDesc, nRewardBufferSize );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sQuestTemplatePrintRewardInfo(
	UNIT *pPlayer,
    QUEST *pQuest,
    WCHAR *puszBuffer,
    int nBufferSize)
{
	ASSERT(puszBuffer);

	// zero buffer
	puszBuffer[ 0 ] = 0;

	WCHAR szRewardStats[MAX_STRING_ENTRY_LENGTH];
	szRewardStats[ 0 ] = 0;
	sQuestTemplatePrintRewardStatsInfo( pPlayer, pQuest, szRewardStats, arrsize( szRewardStats ) );

	const QUEST_DEFINITION *pQuestDef = QuestGetDefinition( pQuest );
	ASSERT_RETURN( pQuestDef );

	if (pQuestDef->nOfferReward == INVALID_LINK && szRewardStats[ 0 ] == 0)
		return;	// no reward	

	PStrCat( puszBuffer, GlobalStringGet( GS_REWARD_HEADER ), nBufferSize );
	PStrCat( puszBuffer, L"\n", nBufferSize );
	if (szRewardStats[ 0 ] != 0)
	{
		PStrCat( puszBuffer, szRewardStats, nBufferSize );
		PStrCat( puszBuffer, L"\n", nBufferSize );
	}

	if (pQuestDef->nOfferReward  != INVALID_LINK)
	{
		const OFFER_DEFINITION *pOfferDef = 
			OfferGetDefinition( pQuestDef->nOfferReward );
		if ( pOfferDef )
		{
			// add line that says how many they can pick from if necessary
			if (pOfferDef->nNumAllowedTakes != REWARD_TAKE_ALL)
			{
				int nDifficulty = UnitGetStat( QuestGetPlayer( pQuest ), STATS_DIFFICULTY_CURRENT );

				int nNumTaken = 
					UnitGetStat( 
					pPlayer, STATS_OFFER_NUM_ITEMS_TAKEN, pQuestDef->nOfferReward, nDifficulty );

				int nNumChoicesLeft = 
					( pOfferDef->nNumAllowedTakes - nNumTaken);
				if (nNumChoicesLeft == 1)
				{
					PStrCat( 
						puszBuffer, 
						GlobalStringGet( GS_REWARD_INSTRUCTIONS_TAKE_ONE ), 
						nBufferSize );
				}
				else
				{
					const int MAX_MESSAGE = 128;
					WCHAR uszMessage[ MAX_MESSAGE ];
					PStrPrintf( 
						uszMessage, 
						MAX_MESSAGE, 
						GlobalStringGet( GS_REWARD_INSTRUCTIONS_TAKE_SOME ), 
						nNumChoicesLeft);
					PStrCat( puszBuffer, uszMessage, nBufferSize );
				}
				PStrCat( puszBuffer, L"\n", nBufferSize );
			}

			// get reward string
			WCHAR uszReward[ MAX_REWARD_DESCRIPTION_LENGTH ];
			sQuestTemplateGetRewardString( 
				pQuest, uszReward, MAX_REWARD_DESCRIPTION_LENGTH);

			PStrCat( puszBuffer, uszReward, nBufferSize );
			PStrCat( puszBuffer, L"\n", nBufferSize );
		}
	}
}

//----------------------------------------------------------------------------
// Displays a quest dialog, filling in strings with unit information. The
// reward will be displayed as an item, if s_QuestCreateRewardItems was
// called previously.
//----------------------------------------------------------------------------
void c_QuestDisplayDialog( 
   GAME *pGame,
   const MSG_SCMD_QUEST_DISPLAY_DIALOG *pMessage )
{
	UNITID idQuestGiver = pMessage->idQuestGiver;
	
	UNIT * pPlayer = GameGetControlUnit( pGame );
	ASSERT_RETURN( pPlayer );

	QUEST* pQuest = QuestGetQuest( pPlayer, pMessage->nQuest );
	if ( !pQuest )
		return;
	pQuest->tQuestTemplate = pMessage->tQuestTemplate;

	// get quest description string
	WCHAR uszDialogTextBuffer[ MAX_QUESTLOG_STRLEN ];
	// zero buffer
	uszDialogTextBuffer[ 0 ] = 0;

	// get format string
	WCHAR uszFormatTextBuffer[ MAX_QUESTLOG_STRLEN ];
	uszFormatTextBuffer[ 0 ] = 0;

	// get description string
	const WCHAR *puszDescFormat = NULL;
	if (pMessage->nDialog != INVALID_LINK)
	{
		puszDescFormat = c_DialogTranslate( pMessage->nDialog );
	}
	else if ( pMessage->nGossipString != INVALID_LINK )
	{
		// setup attributes of string to look for
		GENDER eGender = UnitGetGender( pPlayer );
		DWORD dwAttributes = StringTableGetGenderAttribute( eGender );
		// get string
		puszDescFormat = StringTableGetStringByIndexVerbose( pMessage->nGossipString, dwAttributes, NULL, NULL, NULL );
	}

	if ( puszDescFormat != NULL )
	{
		c_QuestReplaceStringTokens( 
			pPlayer,
			pQuest,
			puszDescFormat,
			uszFormatTextBuffer, 
			arrsize( uszFormatTextBuffer ) );
	}

	PStrCat( uszDialogTextBuffer, uszFormatTextBuffer, arrsize( uszDialogTextBuffer ) );
	// add newline
	if ( uszDialogTextBuffer[0] != 0 )
	{
		PStrCat( uszDialogTextBuffer, L"\n", arrsize( uszDialogTextBuffer ) );
	}	

	WCHAR uszRewardBuffer[ MAX_QUESTLOG_STRLEN ];
	uszRewardBuffer[ 0 ] = 0;

	//  BSP 3/29/07 - Disabled to make way for the rewards grid
	// get reward string
	//sQuestTemplatePrintRewardInfo( 
	//	pPlayer,
	//	pQuest, 
	//	uszRewardBuffer, 
	//	MAX_QUESTLOG_STRLEN );

	//// make sure it's null-terminated
	//uszRewardBuffer[MAX_QUESTLOG_STRLEN - 1] = L'\0';
	

	// setup dialog spec
	NPC_DIALOG_SPEC tSpec;
	tSpec.idTalkingTo = idQuestGiver;
	tSpec.puszText = uszDialogTextBuffer;
	tSpec.nDialog = pMessage->nDialog;		// we pass this along so we know where the text came from
	tSpec.puszRewardText = ( uszRewardBuffer[ 0 ] == 0) ? NULL : uszRewardBuffer;
	tSpec.eType = (NPC_DIALOG_TYPE)pMessage->nDialogType;
	tSpec.nQuestDefId = pQuest->nQuest;
	tSpec.eGITalkingTo = (GLOBAL_INDEX)pMessage->nGISecondNPC;
	tSpec.bLeaveUIOpenOnOk = pMessage->bLeaveUIOpenOnOk;
	
	// bring up dialog interface with a choice to accept or not
	c_NPCTalkStart( pGame, &tSpec );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_QuestTemplateRestore( 
	GAME *pGame, 
	const MSG_SCMD_QUEST_TEMPLATE_RESTORE *pMessage )
{
	UNIT * pPlayer = UnitGetById( pGame, pMessage->idPlayer );
	if ( pPlayer != NULL )
	{
		QUEST *pQuest = QuestGetQuest( pPlayer, pMessage->nQuest );
		if ( !pQuest )
			return;
		pQuest->tQuestTemplate = pMessage->tQuestTemplate;

		// update the quest log, so that keywords will resolve correctly
		UIUpdateQuestLog( QUEST_LOG_UI_UPDATE_TEXT );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
QUEST_STYLE c_QuestGetStyle(
	GAME * pGame, 
	int nQuestId )
{
	ASSERT_RETVAL( pGame, QS_INVALID );
	ASSERT_RETVAL( IS_CLIENT( pGame ), QS_INVALID );
	UNIT * pPlayer = GameGetControlUnit( pGame );
	ASSERT_RETVAL ( pPlayer, QS_INVALID );

	QUEST * pQuest = QuestGetQuest( pPlayer, nQuestId, FALSE );
	ASSERT_RETVAL( pQuest, QS_INVALID );
	ASSERT_RETVAL( pQuest->pQuestDef, QS_INVALID );

	return pQuest->pQuestDef->eStyle;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_QuestPlayerHasObjectiveInLevel(
	UNIT *pPlayer,
	int nLevelDef)
{
	if (AppIsTugboat())
	{
		return FALSE;
	}
	// do nothing if we havne't received all the setup from the server yet
	if ( AppIsTugboat() ||	
		 c_QuestIsAllInitialQuestStateReceived() == FALSE)
	{
		return FALSE;	
	}

	for (int nQuest = 0; nQuest < NUM_QUESTS; nQuest++)
	{		
		QUEST *pQuest = QuestGetQuest( pPlayer, nQuest );
		if (pQuest && c_QuestIsVisibleInLog( pQuest ) && pQuest->pQuestDef)
		{
			for (int i=0; i < MAX_QUEST_LEVEL_DEST; i++)
			{
				if (pQuest->pQuestDef->nLevelDefDestinations[i] == nLevelDef)
				{
					return TRUE;
				}
			}
		}
	}

	return FALSE;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_QuestClearAllStates(
	GAME * pGame,
	int nQuestId )
{
	ASSERT_RETURN( pGame );
	ASSERT_RETURN( IS_CLIENT( pGame ) );
	UNIT * pPlayer = GameGetControlUnit( pGame );
	ASSERT_RETURN( pPlayer );

	QUEST * pQuest = QuestGetQuest( pPlayer, nQuestId, FALSE );
	ASSERT_RETURN( pQuest );

	// set all states to beginning values
	for (int i = 0; i < pQuest->nNumStates; ++i)
	{
		QUEST_STATE *pState = QuestStateGetByIndex( pQuest, i );
		QuestStateInit( pState, pState->nQuestState );
	}

	// update the quest UI
	UIUpdateQuestLog( QUEST_LOG_UI_UPDATE_TEXT, nQuestId );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_QuestIsThisLevelNextStoryLevel(
	GAME * pGame,
	int nLevelDefIndex )
{
	ASSERT_RETFALSE( pGame );
	ASSERT_RETFALSE( IS_CLIENT( pGame ) );
	UNIT * pPlayer = GameGetControlUnit( pGame );
	ASSERT_RETFALSE( pPlayer );

	// NOTE!!! Quests spreadsheet data must remain in story order for this to work!
	int nDifficulty = UnitGetStat( pPlayer, STATS_DIFFICULTY_CURRENT );
	QUEST_GLOBALS * pQuestGlobals = QuestGlobalsGet( pPlayer );

	// if there is an active story quest, don't show a '!'
	if ( pQuestGlobals->bStoryQuestActive )
	{
		return FALSE;
	}
	// check cached result
	else if ( pQuestGlobals->nLevelNextStoryDef != INVALID_LINK )
	{
		return ( nLevelDefIndex == pQuestGlobals->nLevelNextStoryDef );
	}
	else
	{
		// find new result
		for ( int i = 0; i < NUM_QUESTS; i++ )
		{
			QUEST * pQuest = &pQuestGlobals->tQuests[ i ][ nDifficulty ];
			ASSERT_RETFALSE( pQuest );
			if ( !pQuest->pQuestDef )
				continue;

			if ( pQuest->pQuestDef->eStyle == QS_STORY )
			{
				if ( pQuest->eStatus == QUEST_STATUS_ACTIVE )
				{
					pQuestGlobals->bStoryQuestActive = TRUE;
					return FALSE;
				}
				else if ( pQuest->eStatus < QUEST_STATUS_ACTIVE )
				{
					pQuestGlobals->nLevelNextStoryDef = pQuest->pQuestDef->nLevelDefQuestStart;
					return ( nLevelDefIndex == pQuest->pQuestDef->nLevelDefQuestStart );
				}
			}
		}
	}

	return FALSE;
}

#endif //!ISVERSION(SERVER_VERSION)
