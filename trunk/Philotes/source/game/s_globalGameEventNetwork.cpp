#include "stdafx.h"
#include "svrstd.h"
#include "prime.h"
#include "Currency.h"
#include "ServerSuite\GlobalGameEventServer\GlobalGameEventServerRequestMsg.h"
#include "ServerSuite\GlobalGameEventServer\GlobalGameEventServerResponseMsg.h"
#include "c_message.h"
#include "ehm.h"
#include "gamelist.h"
#include "s_globalGameEventNetwork.h"
#if ISVERSION(SERVER_VERSION)
#include "s_globalGameEventNetwork.cpp.tmh"
#endif

//----------------------------------------------------------------------------
// Pick an event.  We actually pick an event every time we donate.  The
// final donation's event is the one that actually happens.  This is hacky,
// but also cool since the final donation could theoretically influence our
// choice.
//----------------------------------------------------------------------------
int s_GlobalGameEventGetRandomEventType(
	GAME *game)
{
	//Here we should pick a random event, for example by excel script
	///Grr...our picker chooser needs a game, so now, its time to implement a piece of the programming test~ -bmanegold
	//// P.S. bill, this funciton now has a game, if you want to use the picker :) -rjd
	int nRows = ExcelGetNumRows(NULL, DATATABLE_DONATION_REWARDS);
	int nMax = 0;
	int ii;
	for (ii = 0; ii < nRows; ii++)
	{
		int weight = ((DONATION_REWARDS_DATA*)ExcelGetData(NULL, DATATABLE_DONATION_REWARDS, ii))->m_nWeight; 
		nMax += (weight != INVALID_ID ? weight : 1);
	}
	int nChoice = RandGetNum(game, 0, nMax);
	int nVal = 0;
	for (ii = 0; ii < nRows && nVal < nMax; ii++)
	{
		int weight = ((DONATION_REWARDS_DATA*)ExcelGetData(NULL, DATATABLE_DONATION_REWARDS, ii))->m_nWeight; 
		nVal += (weight != INVALID_ID ? weight : 1);
		if(nVal > nChoice)
			break;
	}

	ASSERT_DO(ii != nRows)
	{ //This should never happen, but just in case...
		ii = 0;
	}

	return ii;
}

//----------------------------------------------------------------------------
// Currently, we assume the GLOBAL_GAME_EVENT_SERVER is a singleton with
// SVRINSTANCE 0.  Being global it pretty much has to be a singleton.
//----------------------------------------------------------------------------
BOOL GameServerToGlobalGameEventServerSendMessage(
	MSG_STRUCT * msg,
	NET_CMD		 command)
{
#if ISVERSION(SERVER_VERSION)
	return SvrNetSendRequestToService(
		ServerId(GLOBAL_GAME_EVENT_SERVER, 0), msg, command) == SRNET_SUCCESS;
#else
	UNREFERENCED_PARAMETER(msg);
	UNREFERENCED_PARAMETER(command);
	return FALSE;
#endif
}

//----------------------------------------------------------------------------
// We should look at a better way to serialize currency, for example into
// blob form rather than messaging components.
//----------------------------------------------------------------------------
BOOL s_GlobalGameEventSendDonate(
	UNIT * unit,
	cCurrency nMoney,
	int nDesiredEvent)
{
	MSG_EVENT_REQUEST_DONATE_MONEY msg;
	msg.nMoney = nMoney.GetValue(KCURRENCY_VALUE_INGAME);
	msg.nRealWorldMoney = nMoney.GetValue(KCURRENCY_VALUE_REALWORLD);
	msg.nDonationEventTypePreferred = nDesiredEvent;
	UnitGetName(unit, msg.szDonatorCharacterName, MAX_CHARACTER_NAME, 0);

	return GameServerToGlobalGameEventServerSendMessage(
		&msg, EVENT_REQUEST_DONATE_MONEY);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_GlobalGameEventSendSetDonationCost(
	UINT64 nInGameMoney,
	cCurrency nAdditionalMoney,
	int nAnnouncementThreshhold,
	int nEventMax  /*= DONATION_INFINITY*/)
{
	MSG_EVENT_REQUEST_SET_DONATION_TOTAL_COST msg;
	msg.nMoney = nInGameMoney + nAdditionalMoney.GetValue(KCURRENCY_VALUE_INGAME);
	msg.nRealWorldMoney = nAdditionalMoney.GetValue(KCURRENCY_VALUE_REALWORLD);
	msg.nAnnouncementThreshhold = nAnnouncementThreshhold;
	msg.nEventCount = nEventMax;
	
	return GameServerToGlobalGameEventServerSendMessage(
		&msg, EVENT_REQUEST_SET_DONATION_TOTAL_COST);
}

//----------------------------------------------------------------------------
// HANDLERS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sEventCmdDonationEvent(
	__notnull LPVOID svrContext,
	SVRID sender,
	__notnull MSG_STRUCT * msgdata )
{
	BOOL bRet = TRUE;
	MSG_EVENT_RESPONSE_DONATION_EVENT *msg = 
		(MSG_EVENT_RESPONSE_DONATION_EVENT *)msgdata;

	MSG_APPCMD_DONATION_EVENT gameMsg;

	GameServerContext * pServerContext = (GameServerContext*)svrContext; 
	CBRA(msg);
	CBRA(pServerContext);

	TraceInfo(TRACE_FLAG_GAME_NET, "Got global event %d", msg->nDonationEventType);
	
	gameMsg.nDonationEventType = msg->nDonationEventType;
#if !ISVERSION(CLIENT_ONLY_VERSION)
	GameListMessageAllGames(
		pServerContext->m_GameList,
		ServiceUserId(USER_SERVER,0,INVALID_SVRCONNECTIONID),
		&gameMsg,
		APPCMD_DONATION_EVENT);
#endif
Error:
	if(!bRet && msg)
	{
		TraceError("Error posting global event %d", msg->nDonationEventType);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sEventCmdAnnouncement(
	__notnull LPVOID svrContext,
	SVRID sender,
	__notnull MSG_STRUCT * msgdata )
{
	BOOL bRet = TRUE;
	MSG_EVENT_RESPONSE_ANNOUCEMENT *msg = 
		(MSG_EVENT_RESPONSE_ANNOUCEMENT *)msgdata;

	MSG_SCMD_GLOBAL_LOCALIZED_ANNOUNCEMENT gameMsg;
	CBRA(msg);

	gameMsg.tLocalizedParamString = msg->tLocalizedParamString;
	ClientSystemSendMessageToAll(AppGetClientSystem(), INVALID_NETCLIENTID64, 
		SCMD_GLOBAL_LOCALIZED_ANNOUNCEMENT, &gameMsg);

Error:
	if(!bRet)
	{
		TraceGameInfo("Error in global announcement");
	}
}


//----------------------------------------------------------------------------
// Command Table
//----------------------------------------------------------------------------
FP_NET_RESPONSE_COMMAND_HANDLER sGlobalGameEventToGameResponseHandlers[] =
{
	sEventCmdDonationEvent,
	sEventCmdAnnouncement
};