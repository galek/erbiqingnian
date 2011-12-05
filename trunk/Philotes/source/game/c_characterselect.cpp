//----------------------------------------------------------------------------
// c_characterselect.cpp
//
// The client side of the GameLoadBalance server.
// Handles character selection related messages from server.
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "prime.h"
#include "c_connmgr.h"
#include "ServerSuite/GameLoadBalance/GameLoadBalanceDef.h"
#include "ServerSuite/GameLoadBalance/GameLoadBalanceClientMsg.h"
#include "ServerSuite/GameLoadBalance/GameLoadBalanceServerMsg.h"
#include "uix_menus.h"
#include "uix_priv.h"
#include "c_connection.h"
#ifndef _BOT
#include "units.h"
#include "console.h"
#include "globalindex.h"
#include "uix.h"
#include "uix_priv.h"
#endif


//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
static BOOL gbWaitingInLoginQueue(FALSE);
static BOOL gbShowingCancelLogin(FALSE);


#if !ISVERSION(SERVER_VERSION)
//----------------------------------------------------------------------------
// Send a MSG_GAMELOADBALANCE_CCMD_LOGIN through the channel.
//----------------------------------------------------------------------------
BOOL CharacterSelectStartLogin(CHANNEL *pChannel)
{
	gbWaitingInLoginQueue = FALSE;
	gbShowingCancelLogin = FALSE;
	MSG_GAMELOADBALANCE_CCMD_LOGIN msg;
	return ConnectionManagerSend(pChannel, &msg, GAMELOADBALANCE_CCMD_LOGIN);
}

//----------------------------------------------------------------------------
// Pop off the GAMELOADBALANCE tables and mailslot, and set the channel to
// CHANNEL_STATE_CHARACTER_SELECTED
//----------------------------------------------------------------------------
static BOOL sCharacterSelectFinishLogin(CHANNEL *pChannel)
{
	if(AppGetType() == APP_TYPE_CLOSED_CLIENT)
	{
		NET_COMMAND_TABLE *pClientTable, *pServerTable;
		MAILSLOT *pMailSlot;

		ConnectionManagerChannelPopRequestTable(pChannel,pClientTable);
		ConnectionManagerChannelPopResponseTable(pChannel, pServerTable);
		ConnectionManagerChannelPopMailSlot(pChannel, pMailSlot);

		//Take out assertions for bot case, which does not use gApp.
#ifndef _BOT
		ASSERT(pClientTable == gApp.m_GameLoadBalanceClientCommandTable);
		ASSERT(pServerTable == gApp.m_GameLoadBalanceServerCommandTable);
#endif
	}
	
	extern void CharacterSelectDone(CHANNEL *pChannel, CHANNEL_STATE state);
	CharacterSelectDone(pChannel, CHANNEL_STATE_CHARACTER_SELECTED);

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCharacterSelectOnCancelQueueWait(
	void *pCallbackData,
	DWORD dwCallbackData )
{
	if (gbWaitingInLoginQueue && AppGetState() == APP_STATE_CHARACTER_CREATE)
		AppSwitchState(APP_STATE_RESTART);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL CharacterSelectWaitingInLoginQueue(
	void )
{
	return gbWaitingInLoginQueue;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CharacterSelectLoginQueueUiInit(void)
{
	gbWaitingInLoginQueue = FALSE;
	gbShowingCancelLogin = FALSE;
}

//----------------------------------------------------------------------------
// Returns TRUE if we should keep processing messages, FALSE if we should
// stop.  False isn't an error, merely a "pause" for the app to update.
//----------------------------------------------------------------------------
#ifndef _BOT
static BOOL sCharacterSelectProcessMessage(MSG_STRUCT *msg, BOOL &bDone)
#else
static BOOL sCharacterSelectProcessMessage(
	MSG_STRUCT *msg,
	BOOL &bDone,
	int &nCharTotal,
	int &nCharCount,
	int nPlayerSlots[MAX_CHARACTERS_PER_ACCOUNT],
	WCHAR szPlayerNames[MAX_CHARACTER_NAME][MAX_CHARACTERS_PER_ACCOUNT])
#endif
{
	gbWaitingInLoginQueue = FALSE;
	switch(msg->hdr.cmd)
	{
	case GAMELOADBALANCE_SCMD_LOGIN_RESPONSE:
		//TODO really check the response.
		{
#ifndef _BOT
			UIHideGenericDialog();
			CharacterSelectLoginQueueUiInit();
#endif

			MSG_GAMELOADBALANCE_SCMD_LOGIN_RESPONSE * responseMsg = 
				(MSG_GAMELOADBALANCE_SCMD_LOGIN_RESPONSE *)msg;
			if(responseMsg->status == LOGIN_STATUS_OK)
			{
				bDone = TRUE;
#ifndef _BOT
				if(responseMsg->nPlayersOnline > 1) //Don't display for invalid results
					ConsoleString(GlobalStringGet( GS_TOTAL_PLAYERS ),
					responseMsg->nPlayersOnline);
#endif
			}
			else if(responseMsg->status == GAME_LOGIN_STATUS_TEMPORARY_SETBACK)
			{//Display the error message and go back to character select state.

				const WCHAR *puszTitle = GlobalStringGet(GS_ERROR);
#ifndef _BOT
				if(responseMsg->eGlobalString == GS_INVALID)
				{
					UIShowGenericDialog(puszTitle, responseMsg->szReply);
				}
				else
				{
					UIShowGenericDialog(puszTitle, GlobalStringGet(GLOBAL_STRING(responseMsg->eGlobalString)));
				}
#else
				TraceError("Bot login setback in character select %d %ls", 
					responseMsg->eGlobalString, responseMsg->szReply);
#endif
			}
			else if(responseMsg->status == GAME_LOGIN_STATUS_FAILED)
			{
#ifndef _BOT
				//Kill their game channel.
				ConnectionManagerCloseChannel(AppGetGameChannel());
/*
				const WCHAR *puszTitle = GlobalStringGet(GS_ERROR);
				if(responseMsg->eGlobalString == GS_INVALID)
				{
					UIShowGenericDialog(puszTitle, responseMsg->szReply);
				}
				else
				{
					UIShowGenericDialog(puszTitle, GlobalStringGet(GLOBAL_STRING(responseMsg->eGlobalString)));
				}
*/
				UIComponentSetVisibleByEnum(UICOMP_CHARACTER_CREATE, FALSE);
				UIComponentSetVisibleByEnum(UICOMP_CHARACTER_SELECT, FALSE);
				UIComponentSetVisibleByEnum(UICOMP_DASHBOARD, FALSE);
				UI_COMPONENT * pCharSel = UIComponentGetByEnum(UICOMP_CHARACTER_SELECT);
				if (pCharSel) UICancelDelayedActivates(pCharSel);

				if(responseMsg->eGlobalString == GS_INVALID)
					AppSetErrorStateLocalized(
						GlobalStringGet(GS_ERROR), responseMsg->szReply,
						APP_STATE_RESTART);
				else
					AppSetErrorState(
						GS_ERROR, GLOBAL_STRING(responseMsg->eGlobalString),
						APP_STATE_RESTART);
#else
				TraceError("Bot login error in character select %d %ls", 
					responseMsg->eGlobalString, responseMsg->szReply);
#endif
			}
			return FALSE;
		}

	case GAMELOADBALANCE_SCMD_CHAR_COUNT:
		{
			MSG_GAMELOADBALANCE_SCMD_CHAR_COUNT * countMsg = 
				(MSG_GAMELOADBALANCE_SCMD_CHAR_COUNT *)msg;

			UI_MultiplayerCharCountHandler(countMsg->count);
#ifdef _BOT
			nCharTotal = countMsg->count;
			if (nCharTotal > MAX_CHARACTERS_PER_ACCOUNT)
				nCharTotal = MAX_CHARACTERS_PER_ACCOUNT;
#endif
			return FALSE;
		}
		break;
	case GAMELOADBALANCE_SCMD_CHAR_LIST:
		{
			MSG_GAMELOADBALANCE_SCMD_CHAR_LIST * listMsg = 
				(MSG_GAMELOADBALANCE_SCMD_CHAR_LIST *)msg;
			TraceDebugOnly("Got char %ls\n",listMsg->tCharacterInfo.szCharName);
			if(!UI_MultiplayerCharListHandler(listMsg->tCharacterInfo.szCharName,
				listMsg->tCharacterInfo.nPlayerSlot,
				listMsg->wardrobeBlob,
				MSG_GET_BLOB_LEN(listMsg,wardrobeBlob),
				listMsg->wszGuildName))
			{
				TraceError("MultiplayerCharListHandler failed for character %ls\n",listMsg->tCharacterInfo.szCharName);
				return FALSE;
			}
#ifdef _BOT
			if (nCharCount < nCharTotal)
			{
				PStrCopy(szPlayerNames[nCharCount], listMsg->tCharacterInfo.szCharName, MAX_CHARACTER_NAME);
				nPlayerSlots[nCharCount] = listMsg->tCharacterInfo.nPlayerSlot;

				nCharCount++;
			}
#endif
			return TRUE;
		}
	case GAMELOADBALANCE_SCMD_SECOND_LOGIN_WINS:
		{//They say we're already logged on and should auto-login with the same character.
		//Note that if we hack our clients and ignore this message, we will still log on 
			//with that same character because this is handled serverside.
#ifndef _BOT
			MSG_GAMELOADBALANCE_SCMD_SECOND_LOGIN_WINS * secondLoginMsg = 
				(MSG_GAMELOADBALANCE_SCMD_SECOND_LOGIN_WINS*)msg;

			//Error handling: if we haven't gotten the character yet.
			UNIT *pSelectedUnit = GameGetControlUnit(AppGetCltGame());
			ASSERTX_DO(pSelectedUnit && pSelectedUnit->szName && pSelectedUnit->szName[0], "Selected unit has no name")
			{
			//Note: this assert is known to happen in the "second login wins" case if the player is very new and has never saved, because the database can't send us his unit.  But this is ok, we handle that here.
				if(pSelectedUnit) UnitSetName(pSelectedUnit, secondLoginMsg->szCharNameAlreadyLoggedOn);
			}

			//Load the character
			UICharacterSelectLoadExistingCharacter();
#endif
			return FALSE;
		}
	case GAMELOADBALANCE_SCMD_QUEUE_POSITION:
		{
			gbWaitingInLoginQueue = TRUE;

			MSG_GAMELOADBALANCE_SCMD_QUEUE_POSITION * queuePosMsg = NULL;
			queuePosMsg = (MSG_GAMELOADBALANCE_SCMD_QUEUE_POSITION*)msg;

#ifndef _BOT
			if (gbShowingCancelLogin)
			{
				gbWaitingInLoginQueue = FALSE;
				UIHideGenericDialog();
				gbWaitingInLoginQueue = TRUE;
			}

			if (AppIsHellgate())
			{
				UIShowLoadingScreen(0, L"", L"", TRUE);
				WCHAR str[2048];
				PStrPrintf(str, 2048, GlobalStringGet(GS_LOGIN_QUEUE_TEXT), queuePosMsg->nQueuePossition, queuePosMsg->nQueueLength);

				DIALOG_CALLBACK callback;
				callback.pfnCallback = sCharacterSelectOnCancelQueueWait;
				UIShowGenericDialog(
					GlobalStringGet(GS_LOGIN_QUEUE_HEADER),
					str,
					TRUE,
					NULL,
					&callback,
					GDF_NO_OK_BUTTON);
				gbShowingCancelLogin = TRUE;
			}
			else
			{
				//	TODO: the above hellgate queue ui code works, but no dialog box shows up in tugboat.
				WCHAR str[2048];
				PStrPrintf(str, 2048, GlobalStringGet(GS_LOGIN_QUEUE_TEXT), queuePosMsg->nQueuePossition, queuePosMsg->nQueueLength);
				UIShowLoadingScreen(0, GlobalStringGet(GS_LOGIN_QUEUE_HEADER), str, TRUE);
				UIShowProgressChange(str);
			}
#endif

			return FALSE;
		}
	default:
		{
			ASSERT_MSG("Got unhandled CharacterSelect message!");
			return TRUE;
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#ifndef _BOT
void CharacterSelectProcessMessages(MAILSLOT *slot, CHANNEL *pChannel)
#else
void CharacterSelectProcessMessages(
	MAILSLOT *slot,
	CHANNEL *pChannel,
	int &nCharTotal,
	int &nCharCount,
	int nPlayerSlots[MAX_CHARACTERS_PER_ACCOUNT],
	WCHAR szPlayerNames[MAX_CHARACTER_NAME][MAX_CHARACTERS_PER_ACCOUNT])
#endif
{
	UNREFERENCED_PARAMETER(slot);
	UNREFERENCED_PARAMETER(pChannel);
	//grab the messages
	//for each message, call sCharacterCreateProcessMessage.
	MSG_BUF *msg = NULL,*head = NULL, *tail = NULL, *next = NULL;

	BOOL  bDone = FALSE;
	unsigned int count = MailSlotGetMessages(slot,
		head,
		tail);

	if(!count)
	{
		return;
	}

	next = head;

	while( (msg = next) != NULL)
	{
		next = msg->next;

		BYTE message[MAX_SMALL_MSG_STRUCT_SIZE];

		if(!ConnectionManagerTranslateMessageForRecv(pChannel,
			msg->msg,
			msg->size,
			(MSG_STRUCT*)message))
		{
			ASSERT_MSG("Could not translate message\n");
			break;
		}

#ifndef _BOT
		if(!sCharacterSelectProcessMessage((MSG_STRUCT*)message,bDone))
#else
		if(!sCharacterSelectProcessMessage(
			(MSG_STRUCT*)message,
			bDone,
			nCharTotal,
			nCharCount,
			nPlayerSlots,
			szPlayerNames))
#endif
		{
			if (bDone)
			{
				sCharacterSelectFinishLogin(pChannel);
			}
			break;
		}

	}
	if(msg != NULL && next)
	{
		MailSlotPushbackMessages(slot,next,tail);
		tail = msg;
	}
	MailSlotRecycleMessages(slot,head,tail);
}

//----------------------------------------------------------------------------
// Figure out the slot type from the slot number.  Currently, the first
// N slots are free, and the rest are subscription for Hellgate.
//----------------------------------------------------------------------------
CHARACTER_SLOT_TYPE CharacterSelectGetSlotType(int nCharacterSlot)
{
	// Hellgate no longer restricting non-subscribers

	//if(AppIsHellgate())
	//	return ((nCharacterSlot<MAX_CHARACTERS_PER_NON_SUBSCRIBER)?
	//	CHARACTER_SLOT_FREE:CHARACTER_SLOT_SUBSCRIPTION);
	//else 
		return CHARACTER_SLOT_FREE;
}
//----------------------------------------------------------------------------
// For closed client, send a message to the loadbalancer.  Otherwise, we're
// done with character selection.
//----------------------------------------------------------------------------
void CharacterSelectSetCharName(const WCHAR * szCharName,
								int nCharacterSlot,
								int nDifficulty,
								BOOL bNewPlayer )
{
#ifndef _BOT
	ConnectionStateSetCharacterName(szCharName, MAX_CHARACTER_NAME);
#endif

	if(AppGetType() == APP_TYPE_CLOSED_CLIENT)
	{
		if (nCharacterSlot < 0)
		{
			// this should mean that this is the first character on the account, i.e. the slot wasn't set because no button was pressed.
			//   So just set it to the first slot.
			nCharacterSlot = 0;
		}

		MSG_GAMELOADBALANCE_CCMD_CHAR_SELECT msg;

		wcscpy_s(msg.tCharacterInfo.szCharName,ARRAYSIZE(msg.tCharacterInfo.szCharName),szCharName);
		msg.tCharacterInfo.nPlayerSlot = nCharacterSlot;
		msg.tCharacterInfo.nDifficulty = nDifficulty;
		msg.bNewPlayer = (BYTE)bNewPlayer;
		msg.tCharacterInfo.eSlotType = CharacterSelectGetSlotType(nCharacterSlot);

		if(!ConnectionManagerSend(AppGetGameChannel(),&msg,GAMELOADBALANCE_CCMD_CHAR_SELECT))
		{
			TraceInfo(0,"send failed for character-select\n");
		}
	}
	else sCharacterSelectFinishLogin(AppGetGameChannel());
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CharacterSelectDeleteCharName(const WCHAR * szCharName )
{
	MSG_GAMELOADBALANCE_CCMD_CHAR_DELETE msg;

	wcscpy_s(msg.szCharName,ARRAYSIZE(msg.szCharName),szCharName);

	if(!ConnectionManagerSend(AppGetGameChannel(),&msg,GAMELOADBALANCE_CCMD_CHAR_DELETE))
	{
		TraceInfo(0,"send failed for character-delete\n");
	}

}
#endif //!SERVER_VERSION