//----------------------------------------------------------------------------
// FILE: npc.cpp
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "prime.h"
#include "clients.h"
#include "c_message.h"
#include "c_tasks.h"
#include "c_ui.h"
#include "console.h"
#include "game.h"
#include "hireling.h"
#include "interact.h"
#include "inventory.h"
#include "items.h"
#include "monsters.h"
#include "movement.h"
#include "pets.h"
#include "room.h"
#include "quests.h"
#include "s_message.h"
#include "s_trade.h"
#include "offer.h"
#include "states.h"
#include "stats.h"
#include "stringtable.h"
#include "dialog.h"
#include "tasks.h"
#include "trade.h"
#include "unit_priv.h" // also includes units.h
#include "unitmodes.h"
#include "uix.h"
#include "c_input.h"
#include "npc.h"
#include "globalindex.h"
#include "fontcolor.h"
#include "c_sound.h"
#include "uix_money.h"
#include "c_quests.h"
#include "s_quests.h"
#include "gossip.h"
#include "characterclass.h"
#include "recipe.h"
#include "s_recipe.h"
#include "s_QuestServer.h"
#include "skills.h"
#include "chat.h"
#include "uix_graphic.h"
#include "uix_chat.h"
#include "c_sound_util.h"
#include "Currency.h"
#include "stash.h"
#include "stringreplacement.h"
#include "GameServer.h"

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
enum GREETING_TYPE
{
	GREETING,
	GOODBYE,
};

//----------------------------------------------------------------------------
struct GREETING_LOOKUP
{
	GREETING_TYPE eType;
	INTERACT_EVENT eEvent;
	UNITTYPE eRequiredType;
	GENDER eRequiredGender;
	FACTION_STANDING_MOOD eRequiredFactionMood;
};
static GREETING_LOOKUP sgtGreetingLookup[] = 
{
	// type		choice						required class			req gender		req faction mood
	{ GREETING,	IE_HELLO_GENERIC,			UNITTYPE_PLAYER,		GENDER_INVALID,	FSM_INVALID },
	{ GREETING,	IE_HELLO_TEMPLAR,			UNITTYPE_TEMPLAR,		GENDER_INVALID,	FSM_INVALID },
	{ GREETING,	IE_HELLO_CABALIST,			UNITTYPE_CABALIST,		GENDER_INVALID,	FSM_INVALID },
	{ GREETING,	IE_HELLO_HUNTER,			UNITTYPE_HUNTER,		GENDER_INVALID,	FSM_INVALID },
	{ GREETING,	IE_HELLO_MALE,				UNITTYPE_PLAYER,		GENDER_MALE,	FSM_INVALID },
	{ GREETING,	IE_HELLO_FEMALE,			UNITTYPE_PLAYER,		GENDER_FEMALE,	FSM_INVALID },
	{ GREETING,	IE_HELLO_FACTION_BAD,		UNITTYPE_PLAYER,		GENDER_INVALID,	FSM_MOOD_BAD },
	{ GREETING,	IE_HELLO_FACTION_NEUTRAL,	UNITTYPE_PLAYER,		GENDER_INVALID,	FSM_MOOD_NEUTRAL },
	{ GREETING,	IE_HELLO_FACTION_GOOD,		UNITTYPE_PLAYER,		GENDER_INVALID,	FSM_MOOD_GOOD },

	{ GOODBYE,	IE_GOODBYE_GENERIC,			UNITTYPE_PLAYER,		GENDER_INVALID,	FSM_INVALID },
	{ GOODBYE,	IE_GOODBYE_TEMPLAR,			UNITTYPE_TEMPLAR,		GENDER_INVALID,	FSM_INVALID },
	{ GOODBYE,	IE_GOODBYE_CABALIST,		UNITTYPE_CABALIST,		GENDER_INVALID,	FSM_INVALID },
	{ GOODBYE,	IE_GOODBYE_HUNTER,			UNITTYPE_HUNTER,		GENDER_INVALID,	FSM_INVALID },
	{ GOODBYE,	IE_GOODBYE_MALE,			UNITTYPE_PLAYER,		GENDER_MALE,	FSM_INVALID },
	{ GOODBYE,	IE_GOODBYE_FEMALE,			UNITTYPE_PLAYER,		GENDER_FEMALE,	FSM_INVALID },
	{ GOODBYE,	IE_GOODBYE_FACTION_BAD,		UNITTYPE_PLAYER,		GENDER_INVALID,	FSM_MOOD_BAD },
	{ GOODBYE,	IE_GOODBYE_FACTION_NEUTRAL,	UNITTYPE_PLAYER,		GENDER_INVALID,	FSM_MOOD_NEUTRAL },
	{ GOODBYE,	IE_GOODBYE_FACTION_GOOD,	UNITTYPE_PLAYER,		GENDER_INVALID,	FSM_MOOD_GOOD },	
};
static int sgnNumGreetingLookup = arrsize( sgtGreetingLookup );

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------
struct UI_TOOLTIP;

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const NPC_DATA *NPCGetData(
	GAME *pGame,
	UNIT *pUnit)
{
	ASSERTX_RETNULL( pGame, "Expected game" );
	ASSERTX_RETNULL( pUnit, "Expected unit" );
	
	const UNIT_DATA *pUnitData = UnitGetData( pUnit );
	if (pUnitData->nNPC != INVALID_LINK)
	{
		return (const NPC_DATA *)ExcelGetData( pGame, DATATABLE_NPC, pUnitData->nNPC );
	}
	
	return NULL;
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT *s_TalkGetTalkingTo( 
	UNIT *pPlayer)
{
	ASSERT_RETNULL( pPlayer );
	UNITID idTalkingTo = UnitGetStat( pPlayer, STATS_TALKING_TO );
	if (idTalkingTo != INVALID_ID)
	{
		return UnitGetById( UnitGetGame( pPlayer ), idTalkingTo );
	}
	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_TalkSetTalkingTo(
	UNIT *pPlayer,
	UNIT *pTalkingTo,
	int nDialog)
{			
	ASSERT_RETURN( pPlayer );

	if (pTalkingTo)
	{
	
		// save who we're talking to	
		UnitSetStat( pPlayer, STATS_TALKING_TO, UnitGetId( pTalkingTo ) );	

		// save what was being said
		UnitSetStat( pPlayer, STATS_DIALOG_ID, nDialog );
		
	}
	else
	{
		// clear talking to
		UnitClearStat( pPlayer, STATS_TALKING_TO, 0 );
		UnitClearStat( pPlayer, STATS_DIALOG_ID, 0 );
	}
	
	// when talking to npc, stop moving and face them
	if (pTalkingTo != NULL && UnitIsA( pTalkingTo, UNITTYPE_MONSTER ))
	{
		GAME *pGame = UnitGetGame( pPlayer );
		
		// stop me
		if (UnitOnStepList(pGame, pTalkingTo))
		{
		
			// this removes me from the step list and sends msg to client
			s_UnitSetMode( pTalkingTo, MODE_IDLE, TRUE );

			if ( UnitTestFlag( pTalkingTo, UNITFLAG_HASPATH ) )
			{
				int nIndex = UnitGetStat( pTalkingTo, STATS_AI_PATH_INDEX );
				int nEnd = UnitGetStat( pTalkingTo, STATS_AI_PATH_END );
				if ( nEnd == nIndex )
				{
					if ( nEnd )
						UnitSetStat( pTalkingTo, STATS_AI_PATH_INDEX, nIndex-1 );
					else
						UnitSetStat( pTalkingTo, STATS_AI_PATH_INDEX, nIndex+1 );
				}
				else if ( nEnd > nIndex )
					UnitSetStat( pTalkingTo, STATS_AI_PATH_INDEX, nIndex-1 );
				else
					UnitSetStat( pTalkingTo, STATS_AI_PATH_INDEX, nIndex+1 );
				UnitClearFlag( pTalkingTo, UNITFLAG_HASPATH );
			}
		}

	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_TalkIsBeingTalkedTo(
	UNIT *pUnit)
{
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	GAME *pGame = UnitGetGame( pUnit );
	
	// see if any clients are talking to us
	for (GAMECLIENT *pClient = ClientGetFirstInGame( pGame ); pClient; pClient = ClientGetNextInGame( pClient ))
	{
		UNIT *pPlayer = ClientGetControlUnit( pClient );
		if (pPlayer)
		{
			if (s_TalkGetTalkingTo( pPlayer ) == pUnit)
			{
				return TRUE;
			}
		}
	}

	return FALSE;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSendAudioDialog(
	GAME *pGame,
	UNIT *pPlayer,
	UNIT *pNPC,
	int nSoundDialog)
{

	// setup message
	MSG_SCMD_AUDIO_DIALOG_PLAY tMessage;
	tMessage.idSource = UnitGetId( pNPC );
	tMessage.nSound = nSoundDialog;

	// send interact only to player who did the interaction
	s_SendMessage( pGame, UnitGetClientId( pPlayer ), SCMD_AUDIO_DIALOG_PLAY, &tMessage );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sGetAvailableGreetings( 
	GAME *pGame,
	GREETING_TYPE eType, 
	UNIT *pPlayer, 
	UNIT *pNPC,
	int *pnSoundGreetings, 
	int nMaxGreetings)
{
	const NPC_DATA *pNPCData = NPCGetData( pGame, pNPC );
	if (pNPCData == NULL)
	{
		return 0;
	}

	// get player information
	GENDER eGender = UnitGetGender( pPlayer );
	
	// get npc FACTION information
	FACTION_STANDING_MOOD eMood = FSM_INVALID;
	int nFactionNPC = UnitGetPrimaryFaction( pNPC );
	if (nFactionNPC != INVALID_LINK)
	{
		eMood = FactionGetMood( pPlayer, nFactionNPC );
	}
		
	// go through the table
	int nNumAvailable = 0;
	for (int i = 0; i < sgnNumGreetingLookup; ++i)
	{
		const GREETING_LOOKUP *pLookup = &sgtGreetingLookup[ i ];
		
		// reject ones that aren't for the type we're looking for
		if (pLookup->eType != eType)
		{
			continue;
		}
		
		// check required unittype
		if(!UnitIsA(pPlayer, pLookup->eRequiredType))
		{
			continue;
		}

		// check required gender
		if (pLookup->eRequiredGender != GENDER_INVALID && pLookup->eRequiredGender != eGender)
		{
			continue;
		}
		
		// check required faction standing mood
		if (pLookup->eRequiredFactionMood != FSM_INVALID && pLookup->eRequiredFactionMood != eMood)
		{
			continue;
		}
		
		// add to array of available greetings (if room)
		if (nNumAvailable < nMaxGreetings - 1)
		{
			int nSound = pNPCData->nSound[ pLookup->eEvent ];
			if (nSound != INVALID_LINK)
			{
				pnSoundGreetings[ nNumAvailable++ ] = nSound;
			}
			
		}
		
	}
	
	return nNumAvailable;
	
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_NPCDoGreeting( 
	GAME *pGame, 
	UNIT *pNPC, 
	UNIT *pPlayer)
{
	const NPC_DATA *pNPCData = NPCGetData( pGame, pNPC );
	if (pNPCData == NULL)
	{
		return;
	}

	// get available greetings
	int nSoundGreetings[ IE_MAX_CHOICES];
	int nNumGreetings = sGetAvailableGreetings( pGame,
		GREETING, 
		pPlayer, 
		pNPC, 
		nSoundGreetings, 
		IE_MAX_CHOICES);
		
	if (nNumGreetings > 0)
	{
	
		// pick one of the greetings and send
		int nSoundPick = nSoundGreetings[ RandGetNum( pGame, 0, nNumGreetings - 1 ) ];
		sSendAudioDialog( pGame, pPlayer, pNPC, nSoundPick );

	}
					
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_NPCDoGoodbye( 
	GAME *pGame, 
	UNIT *pNPC, 
	UNIT *pPlayer)
{
	const NPC_DATA *pNPCData = NPCGetData( pGame, pNPC );
	if (pNPCData == NULL)
	{
		return;
	}

	// get available greetings
	int nSoundGreetings[ IE_MAX_CHOICES ];
	int nNumGreetings = sGetAvailableGreetings(	
		pGame,
		GOODBYE, 
		pPlayer, 
		pNPC, 
		nSoundGreetings, 
		IE_MAX_CHOICES);
		
	if (nNumGreetings > 0)
	{
	
		// pick one of the greetings and send
		int nSoundPick = nSoundGreetings[ RandGetNum( pGame, 0, nNumGreetings - 1 ) ];
		sSendAudioDialog( pGame, pPlayer, pNPC, nSoundPick );

	}					

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL NPCCanBeExamined(
	UNIT *pPlayer,
	UNIT *pNPC)
{
	ASSERTX_RETFALSE( pPlayer, "Expected unit" );
	ASSERTX_RETFALSE( pNPC, "Expected unit" );
	
	// get unit data
	const UNIT_DATA *pNPCData = UnitGetData( pNPC );
	if (pNPCData->nAnalyze != INVALID_LINK)
	{
		return TRUE;
	}
	
	// no examine
	return FALSE;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int x_NPCInteractGetAvailable(
	UNIT *pPlayer,
	UNIT *pNPC,
	INTERACT_MENU_CHOICE *ptBuffer,
	int nBufferSize)
{
	ASSERTX_RETZERO( pPlayer, "Expected player" );
	ASSERTX_RETZERO( pNPC, "Expected NPC" );
	ASSERTX_RETZERO( ptBuffer, "Expected buffer" );
	ASSERTX_RETZERO( nBufferSize > 0, "Invalid buffer size" );

	// init count
	int nCount = 0;

	// you can talk to everybody (hopefully everybody has something to say, like at least a greeting)
	InteractAddChoice( pPlayer, UNIT_INTERACT_TALK, TRUE, &nCount, ptBuffer, nBufferSize );

	// KCK: See if this is a subscriber only NPC
	BOOL	bSubscriberContentLocked = FALSE;
	const UNIT_DATA * pUnitData = UnitGetData( pNPC );
	if ( UnitDataTestFlag( pUnitData, UNIT_DATA_FLAG_SUBSCRIBER_ONLY) && !PlayerIsSubscriber(pPlayer))
		bSubscriberContentLocked = TRUE;

	// some are merchants
	if (UnitIsMerchant( pNPC ) == TRUE && !bSubscriberContentLocked)
	{
		InteractAddChoice( pPlayer, UNIT_INTERACT_TRADE, TRUE, &nCount, ptBuffer, nBufferSize );	
	}

	// auto identifiers
	if (UnitAutoIdentifiesInventory( pNPC ) == TRUE && !bSubscriberContentLocked)
	{
		InteractAddChoice( pPlayer, UNIT_INTERACT_IDENTIFY_INVENTORY, TRUE, &nCount, ptBuffer, nBufferSize );	
	}
	
	// task givers
	if (UnitIsTaskGiver( pNPC ) && !bSubscriberContentLocked)
	{
		InteractAddChoice( pPlayer, UNIT_INTERACT_TASK, TRUE, &nCount, ptBuffer, nBufferSize );	
	}
	
	// some you can examine
	if (NPCCanBeExamined( pPlayer, pNPC ))
	{
		InteractAddChoice( pPlayer, UNIT_INTERACT_EXAMINE, TRUE, &nCount, ptBuffer, nBufferSize );	
	}
	
	// some have gossip
	if (s_GossipHasGossip( pPlayer, pNPC ))
	{
		InteractAddChoice( pPlayer, UNIT_INTERACT_GOSSIP, TRUE, &nCount, ptBuffer, nBufferSize );	
	}
		
	// some can return you to where you died
	if (UnitIsGravekeeper( pNPC ))
	{
		InteractAddChoice( pPlayer, UNIT_INTERACT_RETURN_TO_CORPSE, TRUE, &nCount, ptBuffer, nBufferSize );	
	}

	////Marsh Added
	//if(eResult == NPC_INTERACT_IGNORED )
	//{
	//	NPCDoGreeting( pGame, pNPC, pPlayer );
	//	eResult = NPCCheckForQuest( pGame, pPlayer, pNPC );
	//}

	//if(eResult == NPC_INTERACT_IGNORED )
	//{
	//	NPCDoGreeting( pGame, pNPC, pPlayer );
	//	eResult = NPCTalkDungeonWarp( pPlayer, pNPC, NPC_DIALOG_OK_CANCEL, DIALOG_TASK_DUNGEON_WARP_LAST_LEVEL );
	//}

	//if(eResult == NPC_INTERACT_IGNORED )
	//{
	//	NPCDoGreeting( pGame, pNPC, pPlayer );
	//	eResult = NPCTalkGoblinSeller( pPlayer, pNPC, NPC_DIALOG_OK_CANCEL, DIALOG_TASK_BUY_GOBLIN_HIRELING );
	//}
	////Marsh end add

	return nCount;  // return how many we found
				
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION( CLIENT_ONLY_VERSION )
static void sSendNPCStartMessage( 
	UNIT *pPlayer,						
	UNIT *pNPC,
	NPC_DIALOG_TYPE eType,
	int nDialog,
	int nDialogReward,
	int nQuestId,
	GLOBAL_INDEX eSecondNPC )
{

	// setup message
	MSG_SCMD_TALK_START tMessage;
	tMessage.idTalkingTo = UnitGetId( pNPC );
	tMessage.nDialogType = (int)eType;
	tMessage.nDialog = nDialog;
	tMessage.nDialogReward = nDialogReward;
	tMessage.nQuestId = nQuestId;
	tMessage.nGISecondNPC = (int)eSecondNPC;
	
	// send to client
	GAME *pGame = UnitGetGame( pPlayer );
	GAMECLIENTID idClient = UnitGetClientId( pPlayer );
	s_SendMessage( pGame, idClient, SCMD_TALK_START, &tMessage );		
}
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_NPCTalkStart( 
	UNIT *pPlayer, 
	UNIT *pNPC,
	NPC_DIALOG_TYPE eType,
	int nDialog,
	int nDialogReward,
	GLOBAL_INDEX eSecondNPC,
	int nQuestID )
{
#if !ISVERSION( CLIENT_ONLY_VERSION )
	ASSERTX_RETURN( nDialog != INVALID_LINK, "Invalid dialog" );
	sSendNPCStartMessage( pPlayer, pNPC, eType, nDialog, nDialogReward, nQuestID, eSecondNPC );

	// mark this player as talking to this npc
	s_TalkSetTalkingTo( pPlayer, pNPC, nDialog );
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )
}

void s_NPCTalkStartToTeam( 
	UNIT *pPlayer, 
	UNIT *pNPC,
	NPC_DIALOG_TYPE eType,
	int nDialog,
	int nDialogReward,
	GLOBAL_INDEX eSecondNPC )
{
#if !ISVERSION( CLIENT_ONLY_VERSION )
	ASSERTX_RETURN( nDialog != INVALID_LINK, "Invalid dialog" );
	ASSERTX_RETURN( pPlayer, "Invalid pPlayer" );
	LEVEL *pLevel = UnitGetLevel( pPlayer );
	ASSERTX_RETURN( pLevel, "Invalid pLevel" );
	//This needs to be fixed up to only do your team. For now it'll do everybody in an array
	for (GAMECLIENT * client = ClientGetFirstInLevel(pLevel); client; client = ClientGetNextInLevel(client, pLevel))
	{
		UNIT * unit = ClientGetPlayerUnit(client);
		s_ConsoleDialogMessage( unit, nDialog );
		//sSendNPCStartMessage( unit, pNPC, eType, nDialog, nDialogReward, INVALID_ID, eSecondNPC );
	}


	// mark this player as talking to this npc
	s_TalkSetTalkingTo( pPlayer, pNPC, nDialog );
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void s_NPCQuestTalkStart( 
	QUEST * pQuest, 
	UNIT * pNPC,
	NPC_DIALOG_TYPE eType,
	int nDialog,
	int nDialogReward,
	GLOBAL_INDEX eSecondNPC /*= GI_INVALID*/, 
	BOOL bLeaveUIOpenOnOk /*= FALSE*/)
{
#if !ISVERSION( CLIENT_ONLY_VERSION )
	ASSERTX_RETURN( nDialog != INVALID_LINK, "Invalid dialog" );
	s_QuestDisplayDialog( pQuest, pNPC, eType, nDialog, nDialogReward, eSecondNPC, bLeaveUIOpenOnOk);
#endif //#if !ISVERSION( CLIENT_ONLY_VERSION )
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_NPCVideoStart( 
	GAME *pGame, 
	UNIT *pPlayer, 
	GLOBAL_INDEX eNPC,
	NPC_DIALOG_TYPE eType,
	int nDialog,
	int nDialogReward /*= INVALID_LINK*/, 
	GLOBAL_INDEX eInterruptNPC /*= GI_INVALID*/)
{	
	ASSERTX_RETURN( AppIsHellgate(), "Hellgate only" );
	
	// setup message(
	MSG_SCMD_VIDEO_START tMessage;
	tMessage.nGITalkingTo = (int)eNPC;
	tMessage.nGITalkingTo2 = (int)eInterruptNPC;
	tMessage.nDialogType = (int)eType;
	tMessage.nDialog = nDialog;
	tMessage.nDialogReward = nDialogReward;
	
	// send to client
	s_SendMessage( pGame, UnitGetClientId(pPlayer), SCMD_VIDEO_START, &tMessage );	
	
	// mark this player as talking to himself?!
	s_TalkSetTalkingTo( pPlayer, pPlayer, nDialog );
}
			
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_NPCTalkStop(
	GAME *pGame,
	UNIT *pPlayer,
	CONVERSATION_COMPLETE_TYPE eCompleteType /*= CCT_OK*/,
	int nRewardSelection /*= -1 */,
	BOOL bCancelOffers /*= TRUE*/)
{

	// sanity
	UNIT *pTalkingTo = s_TalkGetTalkingTo( pPlayer );
	
	// cancel any offers that were out there ... this is kind of a lame to put this,
	// because the talking and offering systems are separate, but not the ui no longer
	// calls the appropriate callbacks for when we cancel talking or a reward ui, it just
	// lumps them all together with an OK
	if (bCancelOffers)
	{
		s_OfferCancel( pPlayer );
	}
	
	// do cancel talk logic
	if (pTalkingTo)
	{

		// do the goodbye
		s_NPCDoGoodbye( pGame, pTalkingTo, pPlayer );
		
		// what was being said
		int nDialog = UnitGetStat( pPlayer, STATS_DIALOG_ID );
			
		// break the link now (this is necessary here because stopping talking may start
		// another conversation as part of the quest interaction)
		s_TalkSetTalkingTo( pPlayer, NULL, INVALID_LINK );
			
		// tell quest system that a conversation has stopped 
		if (eCompleteType == CCT_OK)
		{
			s_QuestEventTalkStopped( pPlayer, pTalkingTo, nDialog, nRewardSelection );
		}
		else if (eCompleteType == CCT_CANCEL || eCompleteType == CCT_FORCE_CANCEL)
		{
			s_QuestEventTalkCancelled( pPlayer, pTalkingTo, nDialog  );
		}
		s_QuestEventDialogClosed( pPlayer, pTalkingTo, nDialog );

		// when forcing conversation to be closed, tell the client
		if (eCompleteType == CCT_FORCE_CANCEL)
		{
			s_PlayerToggleUIElement( pPlayer, UIE_CONVERSATION, UIEA_CLOSE );
		}
					
	}
				
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_NPCEmoteStart(
	GAME *pGame, 
	UNIT *pPlayer, 
	int nDialogEmote)
{
	MSG_SCMD_EMOTE_START tMessage;
	tMessage.nDialog = nDialogEmote;

	// send to client
	s_SendMessage( pGame, UnitGetClientId(pPlayer), SCMD_EMOTE_START, &tMessage );	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_InteractInfoUpdate(
	UNIT *pPlayer,
	UNIT *pSubject,
	BOOL bUpdatingAllQuests /*=FALSE*/)
{
	ASSERTX_RETURN( pSubject, "Expected target unit" );
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );
	GAME *pGame = UnitGetGame( pSubject );
	
	// don't update interact info twice for the same UNIT during the same
	// loop of the quests update. It might be a good idea to allow updates
	// on the same tick from outside the main update, in case something changes
	UNITID idPlayer = UnitGetId( pPlayer );
	GAME_TICK tGameTick = GameGetTick( pGame );
	if ( bUpdatingAllQuests && 
		UnitGetStat( pSubject, STATS_INTERACT_INFO_UPDATE_TICK ) == (int)tGameTick &&
		UnitGetStat( pSubject, STATS_INTERACT_INFO_UPDATE_PLAYER_ID ) == (int)idPlayer )
		return;

	// setup info
	MSG_SCMD_INTERACT_INFO tMessage;
	tMessage.idSubject = UnitGetId( pSubject );
	tMessage.bInfo = (BYTE)s_QuestsGetInteractInfo( pPlayer, pSubject );

	// send to client
	s_SendMessage( pGame, UnitGetClientId(pPlayer), SCMD_INTERACT_INFO, &tMessage );	

	UnitSetStat( pSubject, STATS_INTERACT_INFO_UPDATE_TICK, tGameTick ); 
	UnitSetStat( pSubject, STATS_INTERACT_INFO_UPDATE_PLAYER_ID, idPlayer ); 
	
}	

//----------------------------------------------------------------------------
struct NPC_INFO_PRIORITY
{
	int nPriority;
	INTERACT_INFO eInfo;
};
static NPC_INFO_PRIORITY sgtNPCInfoPriorityLookup[] = 
{
	{ 5, INTERACT_INFO_UNAVAILABLE },
	{ 0, INTERACT_INFO_NONE },
	{ 30, INTERACT_INFO_NEW },
	{ 40, INTERACT_INFO_WAITING },
	{ 50, INTERACT_INFO_RETURN },
	{ 10, INTERACT_INFO_MORE },

	{ 80, INTERACT_STORY_NEW },
	{ 90, INTERACT_STORY_RETURN },
	{ 20, INTERACT_STORY_GOSSIP },

	{ 60, INTERACT_HOLIDAY_NEW },
	{ 70, INTERACT_HOLIDAY_RETURN },
	
	{ NONE, INTERACT_INFO_INVALID }	// keep this last	
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int s_NPCInfoPriority( 
	INTERACT_INFO eInfo)
{
	
	// find entry
	for (int i = 0; sgtNPCInfoPriorityLookup[ i ].eInfo != INTERACT_INFO_INVALID; ++i)
	{
		NPC_INFO_PRIORITY *pPriority = &sgtNPCInfoPriorityLookup[ i ];
		if (pPriority->eInfo == eInfo)
		{
			return pPriority->nPriority;
		}
		
	}
	
	return NONE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_NPCBuyHireling(
	UNIT *pPlayer,
	UNIT *pForeman,
	int nBuyIndex)
{
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( pForeman, "Expected unit" );
	ASSERTX_RETURN( UnitIsForeman( pForeman ), "Expected foreman" );

	// get the available hirelings at this foreman
	HIRELING_CHOICE tHirelings[ MAX_HIRELING_MESSAGE ];
	int nNumHirelings = s_HirelingGetAvailableHirelings( pPlayer, pForeman, tHirelings, MAX_HIRELING_MESSAGE );
	if (nNumHirelings > 0)
	{
	
		// the index they want to buy must be in range
		if (nBuyIndex >= 0 && nBuyIndex < nNumHirelings)
		{
			const HIRELING_CHOICE *pHirelingToBuy = &tHirelings[ nBuyIndex ];	
			cCurrency playerCurrency = UnitGetCurrency( pPlayer );
			cCurrency hirelingCost = cCurrency( pHirelingToBuy->nCost, 0 );			
			if ( playerCurrency  >= hirelingCost)
			{
				UNITLOG_ASSERTX_RETURN(pHirelingToBuy->nCost >= 0, "You can't be PAID money to buy a hireling!", pPlayer);			
				// take money from player
				UNITLOG_ASSERT_RETURN(UnitRemoveCurrencyValidated( pPlayer, hirelingCost ), pPlayer );				
						
				MATRIX matUnitWorld;
				UnitGetWorldMatrix( pPlayer, matUnitWorld);
				
				int nMonsterQuality = -1;
				int nMonsterClass = pHirelingToBuy->nMonsterClass;
				
				ROOM * pSpawnRoom = UnitGetRoom( pPlayer );	
				VECTOR vInitialPosition(0);	
				VECTOR vPosition = UnitGetPosition(pPlayer);
				ROOM_PATH_NODE *pSpawnPathNode = PathStructGetEstimatedLocation(pPlayer, pSpawnRoom);
				// figure out a direction to face
				VECTOR vDirection = UnitGetDirectionXY( pPlayer );

				MONSTER_SPEC tSpec;
				tSpec.nClass = nMonsterClass;
				tSpec.nExperienceLevel = UnitGetExperienceLevel( pPlayer );
				tSpec.nMonsterQuality = nMonsterQuality;
				tSpec.pRoom = pSpawnRoom;
				tSpec.pPathNode = pSpawnPathNode;
				tSpec.vPosition = vPosition;
				tSpec.vDirection = vDirection;
				tSpec.vDirection.fZ = 0.0f;  // only care about X/Y direction
				VectorNormalize( tSpec.vDirection );
				tSpec.pOwner = pPlayer;
				
				GAME *pGame = UnitGetGame( pPlayer );
				UNIT *pHireling = s_MonsterSpawn( pGame, tSpec );		
				if (!pHireling)	
				{
					return; //couldn't spawn monster
				}
				//Hirelings, unlike most monsters, need GUIDS since they're possessed and saved with players
				ASSERT_RETURN(UnitSetGuid(pHireling, GameServerGenerateGUID()));

				PetAdd( pHireling, pPlayer, INVLOC_HIRELING );	

			}
			
		}
		
	}
		
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_NPCTalkingTo( 
	UNITID idTalkingTo)
{
	// do stuff here maybe someday, seems like a nice hook for something
}
			
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
void c_NPCTalkStart(
	GAME *pGame,
	NPC_DIALOG_SPEC *pSpec)
{
	BOOL bCine = FALSE;
/*	if ( AppIsHellgate() && ( pSpec->nQuestDefId != INVALID_ID ) )
	{
		bCine = ( c_QuestGetStyle( pGame, pSpec->nQuestDefId ) == QS_STORY );
	}*/

	if ( bCine )
	{
		UIDisplayNPCStory( pGame, pSpec );
	}
	else
	{
		UIDisplayNPCDialog( pGame, pSpec );
	}
	
	if ( pSpec->nDialog == INVALID_LINK )
	{
		return;
	}

	int nSound = c_DialogGetSound( pSpec->nDialog );
	if ( nSound != INVALID_ID )
	{
		c_SoundPlay( nSound );
	}
	
	UNITMODE eUnitMode = c_DialogGetMode( pSpec->nDialog );
	if ( eUnitMode != INVALID_ID )
	{
		UNIT * pNPC = UnitGetById( pGame, pSpec->idTalkingTo );
		if ( pNPC )
		{
			BOOL bHasMode = FALSE;
			float fDuration = UnitGetModeDuration( pGame, pNPC, eUnitMode, bHasMode );
			if ( bHasMode )
				c_UnitSetMode( pNPC, eUnitMode, 0, fDuration );
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_NPCTalkForceCancel(
	void)
{
	// hide the UI
	UIHideNPCDialog(CCT_FORCE_CANCEL);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sReturnToHeadstoneAccept(
	void *pUserData, 
	DWORD dwCallbackData)
{

	GAME *pGame = AppGetCltGame();
	UNIT *pUnit = GameGetControlUnit( pGame );
	int nCost = CAST_PTR_TO_INT( pUserData );
	
	// if you have enough money, try to accept
	if (UnitGetCurrency( pUnit ) >= cCurrency( nCost, 0 ))
	{
	
		// tell the server that we accept their offer
		MSG_CCMD_TRY_RETURN_TO_HEADSTONE tMessage;
		tMessage.bAccept = TRUE;
		c_SendMessage( CCMD_TRY_RETURN_TO_HEADSTONE, &tMessage );

	}
	else
	{
	
		// init dialog spec
		NPC_DIALOG_SPEC tSpec;
		tSpec.idTalkingTo = UnitGetId( pUnit );
		tSpec.nDialog = DIALOG_HEADSTONE_CANNOT_AFFORD_RETURN;
		tSpec.eType = NPC_DIALOG_OK;
		
		// start talk	
		c_NPCTalkStart( pGame, &tSpec );
	
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_NPCHeadstoneInfo(
	UNIT *pUnit,
	UNIT *pAsker,
	BOOL bCanReturnNow,	
	int nCost,
	int nLevelDefDest)
{
	GAME *pGame = UnitGetGame( pUnit );

	if (bCanReturnNow)
	{
	
		// get money as localized string
		const int MAX_MONEY_STRING = 128;
		WCHAR uszMoney[ MAX_MONEY_STRING ];
		PStrPrintf( uszMoney, MAX_MONEY_STRING, L"%d", nCost );
		WCHAR uszMoneyLocalized[ MAX_MONEY_STRING ];
		UIMoneyLocalizeMoneyString( uszMoneyLocalized, MAX_MONEY_STRING, uszMoney );
		
		// format the money amount with text label
		WCHAR uszMoneyWithText[ MAX_STRING_ENTRY_LENGTH ];
		PStrPrintf( uszMoneyWithText, MAX_STRING_ENTRY_LENGTH, GlobalStringGet( GS_MONEY_WITH_TEXT ), uszMoneyLocalized );	
		
		// get dialog
		WCHAR uszText[ MAX_STRING_ENTRY_LENGTH ];
		int nDialog = DIALOG_ASK_RETURN_TO_CORPSE;
		const WCHAR *szTranslatedDialog = c_DialogTranslate( nDialog );
		ASSERT_RETURN(szTranslatedDialog);
		PStrCopy( uszText, szTranslatedDialog, MAX_STRING_ENTRY_LENGTH );

		// replace money value
		const WCHAR *uszMoneyToken = StringReplacementTokensGet( SR_MONEY );	
		PStrReplaceToken( uszText, MAX_STRING_ENTRY_LENGTH, uszMoneyToken, uszMoneyWithText );
		
		// replace return location
		const WCHAR *uszLevelToken = StringReplacementTokensGet( SR_LEVEL );
		const LEVEL_DEFINITION *pLevelDef = LevelDefinitionGet( nLevelDefDest );
		WCHAR uszLevelName[MAX_STRING_ENTRY_LENGTH] = L"\0";
		UIAddColorToString(uszLevelName, FONTCOLOR_GRAVEKEEPER_RETURN_LEVEL, MAX_STRING_ENTRY_LENGTH);
		PStrCat(uszLevelName, LevelDefinitionGetDisplayName( pLevelDef ), MAX_STRING_ENTRY_LENGTH);
		UIAddColorReturnToString(uszLevelName, MAX_STRING_ENTRY_LENGTH);
		PStrReplaceToken( 
			uszText, 
			MAX_STRING_ENTRY_LENGTH, 
			uszLevelToken, 
			uszLevelName);
		
		// init dialog spec
		NPC_DIALOG_SPEC tSpec;
		tSpec.idTalkingTo = UnitGetId( pAsker );
		tSpec.puszText = uszText;
		tSpec.nDialog = nDialog;
		tSpec.eType = NPC_DIALOG_OK_CANCEL;
		tSpec.tCallbackOK.pfnCallback = sReturnToHeadstoneAccept;
		tSpec.tCallbackOK.pCallbackData = CAST_TO_VOIDPTR( nCost );
		
		// start talk	
		c_NPCTalkStart( pGame, &tSpec );
	
	}
	else
	{
				
		// init dialog spec
		NPC_DIALOG_SPEC tSpec;
		tSpec.idTalkingTo = UnitGetId( pAsker );
		tSpec.nDialog = DIALOG_INFO_RETURN_TO_CORPSE;
		tSpec.eType = NPC_DIALOG_OK;
		
		// start talk	
		c_NPCTalkStart( pGame, &tSpec );
	
	}

}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDungeonWarpAccept(
	void *pUserData, 
	DWORD dwCallbackData)
{
	MSG_CCMD_RETURN_TO_LOWEST_LEVEL tMessage; //we don't need to add anything here.		
	c_SendMessage( CCMD_RETURN_TO_LOWEST_LEVEL, &tMessage );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sGuildCreateBegin(
							   void *pUserData, 
							   DWORD dwCallbackData)
{
	UI_COMPONENT * pDialog = UIComponentGetByEnum(UICOMP_GUILD_CREATE_DIALOG);
	ASSERT_RETURN(pDialog);
	UIComponentActivate(pDialog, TRUE);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRespecAccept(
							   void *pUserData, 
							   DWORD dwCallbackData)
{
	MSG_CCMD_RESPEC tMessage; //we don't need to add anything here.		
	c_SendMessage( CCMD_RESPEC, &tMessage );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sRespecCraftingAccept(
						  void *pUserData, 
						  DWORD dwCallbackData)
{
	MSG_CCMD_RESPEC tMessage; //we don't need to add anything here.		
	c_SendMessage( CCMD_RESPECCRAFTING, &tMessage );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_NPCDungeonWarpInfo( 
	UNIT *pPlayer, 
	UNIT *pWarper, 
	int nDepth,
	cCurrency nCost)

{
	GAME *pGame = UnitGetGame( pPlayer );
	WCHAR uszTextBuffer[ MAX_STRING_ENTRY_LENGTH ];					

	// init common dialog spec stuff	
	NPC_DIALOG_SPEC tSpec;
	tSpec.idTalkingTo = UnitGetId( pWarper );

	if (( nDepth - LevelGetDepth( UnitGetLevel( pWarper ) ) ) <= 0)
	{
		// nowhere to go
		tSpec.nDialog = DIALOG_TASK_NO_DUNGEON_VISITED;
		tSpec.eType = NPC_DIALOG_OK;
	}
	else
	{
		
		// does the player have enough money to do this
		cCurrency playerCurrency = UnitGetCurrency( pPlayer );
		
		if (playerCurrency >= nCost)
		{
			// player has enough money to do this
			const WCHAR *puszText = c_DialogTranslate( DIALOG_TASK_DUNGEON_WARP_LAST_LEVEL );
			PStrCopy( uszTextBuffer, puszText, MAX_STRING_ENTRY_LENGTH );
			tSpec.eType = NPC_DIALOG_OK_CANCEL;
			tSpec.tCallbackOK.pfnCallback = sDungeonWarpAccept;
		}
		else
		{
			// not enough money
			const WCHAR *puszText = c_DialogTranslate( DIALOG_TASK_DUNGEON_WARP_LAST_LEVEL );
			PStrCopy( uszTextBuffer, puszText, MAX_STRING_ENTRY_LENGTH );
			tSpec.eType = NPC_DIALOG_OK;
		}

		
		const int MAX_MONEY_STRING = 128;
		WCHAR uszMoney[ MAX_MONEY_STRING ];
		PStrPrintf( uszMoney, MAX_MONEY_STRING, L"%d", nCost.GetValue( KCURRENCY_VALUE_INGAME_RANK1 ) );		
		// replace any money token with the money value
		const WCHAR *puszGoldToken = StringReplacementTokensGet( SR_COPPER );
		PStrReplaceToken( uszTextBuffer, MAX_STRING_ENTRY_LENGTH, puszGoldToken, uszMoney );

		PStrPrintf( uszMoney, MAX_MONEY_STRING, L"%d", nCost.GetValue( KCURRENCY_VALUE_INGAME_RANK2 ) );		
		// replace any money token with the money value
		puszGoldToken = StringReplacementTokensGet( SR_SILVER );
		PStrReplaceToken( uszTextBuffer, MAX_STRING_ENTRY_LENGTH, puszGoldToken, uszMoney );

		PStrPrintf( uszMoney, MAX_MONEY_STRING, L"%d", nCost.GetValue( KCURRENCY_VALUE_INGAME_RANK3 ) );		
		// replace any money token with the money value
		puszGoldToken = StringReplacementTokensGet( SR_GOLD );
		PStrReplaceToken( uszTextBuffer, MAX_STRING_ENTRY_LENGTH, puszGoldToken, uszMoney );

		// use the text buffer for display
		tSpec.puszText = uszTextBuffer;
									    						
	}

	// start talking
	c_NPCTalkStart( pGame, &tSpec );

}	


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_NPCRespecInfo( 
		UNIT *pPlayer, 
		UNIT *pRespeccer, 
		cCurrency nCost,
		BOOL bCrafting)

{
	GAME *pGame = UnitGetGame( pPlayer );
	WCHAR uszTextBuffer[ MAX_STRING_ENTRY_LENGTH ];					

	// init common dialog spec stuff	
	NPC_DIALOG_SPEC tSpec;
	tSpec.idTalkingTo = UnitGetId( pRespeccer );



	// does the player have enough money to do this
	cCurrency playerCurrency = UnitGetCurrency( pPlayer );

	int nRespecs = bCrafting ? UnitGetStat( pPlayer, STATS_RESPECSCRAFTING ) : UnitGetStat( pPlayer, STATS_RESPECS );

	if( MAX_RESPECS != -1 && nRespecs >= MAX_RESPECS )
	{
		// Respec limit reached
		const WCHAR *puszText = bCrafting ? c_DialogTranslate( DIALOG_RESPEC_LIMIT_REACHED ) : c_DialogTranslate( DIALOG_RESPEC_LIMIT_REACHED );
		PStrCopy( uszTextBuffer, puszText, MAX_STRING_ENTRY_LENGTH );
		tSpec.eType = NPC_DIALOG_OK;	
	}
	else if( PlayerIsHardcore( pPlayer ) || PlayerIsElite( pPlayer ) )
	{
		// Incorrect player type
		const WCHAR *puszText = bCrafting ? c_DialogTranslate( DIALOG_RESPECCRAFTING_INCORRECT_PLAYERTYPE_DIALOG ) : c_DialogTranslate( DIALOG_RESPEC_INCORRECT_PLAYERTYPE_DIALOG );
		PStrCopy( uszTextBuffer, puszText, MAX_STRING_ENTRY_LENGTH );
		tSpec.eType = NPC_DIALOG_OK;	
	}
	else if (playerCurrency >= nCost)
	{
		// player has enough money to do this
		const WCHAR *puszText = bCrafting ? ( nCost == 0 ? c_DialogTranslate( DIALOG_RESPECCRAFTING_DIALOG_FREE ) : c_DialogTranslate( DIALOG_RESPECCRAFTING_DIALOG ) ) :
			( nCost == 0 ? c_DialogTranslate( DIALOG_RESPEC_DIALOG_FREE ) : c_DialogTranslate( DIALOG_RESPEC_DIALOG ) );
		PStrCopy( uszTextBuffer, puszText, MAX_STRING_ENTRY_LENGTH );
		tSpec.eType = NPC_DIALOG_OK_CANCEL;
		tSpec.tCallbackOK.pfnCallback = bCrafting ? sRespecCraftingAccept : sRespecAccept;
	}
	else
	{
		// not enough money
		const WCHAR *puszText = bCrafting ? c_DialogTranslate( DIALOG_RESPECCRAFTING_INSUFFICIENT_FUNDS_DIALOG ) : c_DialogTranslate( DIALOG_RESPEC_INSUFFICIENT_FUNDS_DIALOG );
		PStrCopy( uszTextBuffer, puszText, MAX_STRING_ENTRY_LENGTH );
		tSpec.eType = NPC_DIALOG_OK;
	}


	const int MAX_MONEY_STRING = 128;
	WCHAR uszMoney[ MAX_MONEY_STRING ];
	PStrPrintf( uszMoney, MAX_MONEY_STRING, L"%d", nCost.GetValue( KCURRENCY_VALUE_INGAME_RANK1 ) );		
	// replace any money token with the money value
	const WCHAR *puszGoldToken = StringReplacementTokensGet( SR_COPPER );
	PStrReplaceToken( uszTextBuffer, MAX_STRING_ENTRY_LENGTH, puszGoldToken, uszMoney );

	PStrPrintf( uszMoney, MAX_MONEY_STRING, L"%d", nCost.GetValue( KCURRENCY_VALUE_INGAME_RANK2 ) );		
	// replace any money token with the money value
	puszGoldToken = StringReplacementTokensGet( SR_SILVER );
	PStrReplaceToken( uszTextBuffer, MAX_STRING_ENTRY_LENGTH, puszGoldToken, uszMoney );

	PStrPrintf( uszMoney, MAX_MONEY_STRING, L"%d", nCost.GetValue( KCURRENCY_VALUE_INGAME_RANK3 ) );		
	// replace any money token with the money value
	puszGoldToken = StringReplacementTokensGet( SR_GOLD );
	PStrReplaceToken( uszTextBuffer, MAX_STRING_ENTRY_LENGTH, puszGoldToken, uszMoney );

	if( MAX_RESPECS != -1 )
	{
		PStrPrintf( uszMoney, MAX_MONEY_STRING, L"%d", MAX_RESPECS - nRespecs );		
		// replace any money token with the money value
		puszGoldToken = StringReplacementTokensGet( SR_VALUE );
		PStrReplaceToken( uszTextBuffer, MAX_STRING_ENTRY_LENGTH, puszGoldToken, uszMoney );
	}

	// use the text buffer for display
	tSpec.puszText = uszTextBuffer;

	// start talking
	c_NPCTalkStart( pGame, &tSpec );

}	


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_NPCCreateGuildInfo( 
	UNIT *pPlayer, 
	UNIT *pGuildmaster, 
	cCurrency nCost)

{
	GAME *pGame = UnitGetGame( pPlayer );
	WCHAR uszTextBuffer[ MAX_STRING_ENTRY_LENGTH ];					

	// init common dialog spec stuff	
	NPC_DIALOG_SPEC tSpec;
	tSpec.idTalkingTo = UnitGetId( pGuildmaster );



	// does the player have enough money to do this
	cCurrency playerCurrency = UnitGetCurrency( pPlayer );

	if( c_PlayerGetGuildChannelId() != INVALID_CHANNELID )
	{
		// already a guild member
		const WCHAR *puszText = c_DialogTranslate( DIALOG_GUILD_CREATE_ALREADY_GUILDMEMBER );
		PStrCopy( uszTextBuffer, puszText, MAX_STRING_ENTRY_LENGTH );
		tSpec.eType = NPC_DIALOG_OK;
	}
	else if (playerCurrency >= nCost)
	{
		// player has enough money to do this
		const WCHAR *puszText = c_DialogTranslate( DIALOG_GUILD_CREATE_DIALOG );
		PStrCopy( uszTextBuffer, puszText, MAX_STRING_ENTRY_LENGTH );
		tSpec.eType = NPC_DIALOG_OK_CANCEL;
		tSpec.tCallbackOK.pfnCallback = sGuildCreateBegin;
	}
	else
	{
		// not enough money
		const WCHAR *puszText = c_DialogTranslate( DIALOG_GUILD_CREATE_INSUFFICIENT_FUNDS_DIALOG );
		PStrCopy( uszTextBuffer, puszText, MAX_STRING_ENTRY_LENGTH );
		tSpec.eType = NPC_DIALOG_OK;
	}


	const int MAX_MONEY_STRING = 128;
	WCHAR uszMoney[ MAX_MONEY_STRING ];
	PStrPrintf( uszMoney, MAX_MONEY_STRING, L"%d", nCost.GetValue( KCURRENCY_VALUE_INGAME_RANK1 ) );		
	// replace any money token with the money value
	const WCHAR *puszGoldToken = StringReplacementTokensGet( SR_COPPER );
	PStrReplaceToken( uszTextBuffer, MAX_STRING_ENTRY_LENGTH, puszGoldToken, uszMoney );

	PStrPrintf( uszMoney, MAX_MONEY_STRING, L"%d", nCost.GetValue( KCURRENCY_VALUE_INGAME_RANK2 ) );		
	// replace any money token with the money value
	puszGoldToken = StringReplacementTokensGet( SR_SILVER );
	PStrReplaceToken( uszTextBuffer, MAX_STRING_ENTRY_LENGTH, puszGoldToken, uszMoney );

	PStrPrintf( uszMoney, MAX_MONEY_STRING, L"%d", nCost.GetValue( KCURRENCY_VALUE_INGAME_RANK3 ) );		
	// replace any money token with the money value
	puszGoldToken = StringReplacementTokensGet( SR_GOLD );
	PStrReplaceToken( uszTextBuffer, MAX_STRING_ENTRY_LENGTH, puszGoldToken, uszMoney );

	// use the text buffer for display
	tSpec.puszText = uszTextBuffer;

	// start talking
	c_NPCTalkStart( pGame, &tSpec );

}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_NPCTransporterWarpInfo( 
						  UNIT *pPlayer, 
						  UNIT *pWarper )

{
	GAME *pGame = UnitGetGame( pPlayer );
	WCHAR uszTextBuffer[ MAX_STRING_ENTRY_LENGTH ];					

	// init common dialog spec stuff	
	NPC_DIALOG_SPEC tSpec;
	tSpec.idTalkingTo = UnitGetId( pWarper );

	{

		const WCHAR *puszText = c_DialogTranslate( DIALOG_TRANSPORTER_WARP );
		PStrCopy( uszTextBuffer, puszText, MAX_STRING_ENTRY_LENGTH );
		tSpec.eType = NPC_LOCATION_LIST;
		tSpec.tCallbackOK.pfnCallback = NULL;

		// use the text buffer for display
		tSpec.puszText = uszTextBuffer;

	}

	// start talking
	c_NPCTalkStart( pGame, &tSpec );

}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sPvPSignUpAccept(
	void *pUserData, 
	DWORD dwCallbackData)
{
	MSG_CCMD_PVP_SIGN_UP_ACCEPT tMessage; //we don't need to add anything here.		
	c_SendMessage( CCMD_PVP_SIGN_UP_ACCEPT, &tMessage );
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_NPCPvPSignerUpperInfo( 
	UNIT * pPlayer, 
	UNIT * pPvPSignerUpper)
{

	GAME *pGame = UnitGetGame( pPlayer );

	// init common dialog spec stuff	
	NPC_DIALOG_SPEC tSpec;
	tSpec.idTalkingTo = UnitGetId( pPvPSignerUpper );

#if (0) 	// TODO cmarch: check prereqs for PvP?
	if (nDepth == 0)
	{
		// nowhere to go
		tSpec.nDialog = DIALOG_PVP_ARENA_ACCESS_DENIED;
		tSpec.eType = NPC_DIALOG_OK;
	}
	else
#endif
	{
		tSpec.nDialog = DIALOG_PVP_ARENA_WARP_CONFIRMATION;
		tSpec.eType = NPC_DIALOG_OK_CANCEL;
		tSpec.tCallbackOK.pfnCallback = sPvPSignUpAccept;
	}

	// start talking
	c_NPCTalkStart( pGame, &tSpec );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_NPCVideoStart(
	GAME * pGame,
	NPC_DIALOG_SPEC * pSpec )
{
	UIDisplayNPCVideoAndText( pGame, pSpec );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_NPCEmoteStart(
	GAME * pGame,
	int nDialogEmote)
{
	const WCHAR * pEmote = c_DialogTranslate( nDialogEmote );
	ASSERT_RETURN(pEmote);
	UIAddChatLine( CHAT_TYPE_QUEST, ChatGetTypeColor(CHAT_TYPE_EMOTE), pEmote );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_NPCFlagSoundsForLoad(
	GAME * pGame,
	int nNPC,
	BOOL bLoadNow)
{
	ASSERT_RETURN(pGame);
	ASSERT_RETURN(nNPC >= 0);
	const NPC_DATA * pNPCData = (const NPC_DATA *)ExcelGetData( pGame, DATATABLE_NPC, nNPC );
	if(pNPCData)
	{
		for(int i=0; i<IE_MAX_CHOICES; i++)
		{
			c_SoundFlagForLoad(pNPCData->nSound[i], bLoadNow);
		}
	}
}

#else
void c_NPCEmoteStart(
	GAME * pGame,
	int nDialog)
{
    ASSERT(FALSE);
}
#endif// !ISVERSION(SERVER_VERSION)
