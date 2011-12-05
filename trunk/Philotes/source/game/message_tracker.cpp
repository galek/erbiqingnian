//----------------------------------------------------------------------------
// message_tracker.cpp
// (C)Copyright 2007, Ping0. All rights reserved.
//
// Implements message tracking functions.
// Presently, just notes the message rate to help us with our bot user model.
//----------------------------------------------------------------------------

#include "stdafx.h"
#include "message_tracker.h"
//#include "prime.h"
#include "clients.h"
#include "level.h"
#include "c_message.h"
#if ISVERSION(SERVER_VERSION)
#include "message_tracker.cpp.tmh"
#endif

#define MAX_ALLOWED_PROPORTION 1.05f
#define INSTANT_BOOT_PROPORTION 2
#define TIME_LEEWAY 60*GAME_TICKS_PER_SECOND //They can put in an extra minute worth of commands!

const DWORD dwTicksMinAssertTime = 15*GAME_TICKS_PER_SECOND;
	//One speedhacking client only makes us spam the log once every 15 seconds.
static BOOL sbEnableMessageTracing = FALSE;

//----------------------------------------------------------------------------
// Static structure
//----------------------------------------------------------------------------
struct RateLimit
{
	unsigned int maxmessages[256];//For niceness reasons, we're using a fraction to denote this.
	unsigned int perticks[256];   //if perticks[cmd] is 0, you can have unlimited cmds.
};

RateLimit sCCMDRateLimit;
BOOL sbCCMDRateLimitInitialized = FALSE;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define CCMD_RATE(netcmd, msgs, ticks) { sCCMDRateLimit.maxmessages[netcmd] = msgs; sCCMDRateLimit.perticks[netcmd] = ticks;   }

#if ISVERSION(SERVER_VERSION)
static void sInitMessageTrackerForCCMDRates()
{
	CCMD_RATE(CCMD_PING,
		(CLIENT_PING_INTERVAL + SERVER_PING_INTERVAL)/GAME_TICKS_PER_SECOND,
		(CLIENT_PING_INTERVAL * SERVER_PING_INTERVAL)/GAME_TICKS_PER_SECOND) // 1/c_p_i + 1/p_p_i

	sbCCMDRateLimitInitialized = TRUE;
}
#endif

//----------------------------------------------------------------------------
// Protected
//----------------------------------------------------------------------------
BOOL MessageTracker::MessageRateIsAllowed(
	NET_CMD cmd, 
	float fProportionalTolerance, 
	DWORD dwTimeTolerance,
	const RateLimit &tRateLimit)
{
#if !ISVERSION(SERVER_VERSION)
	//We'd rather not have our detection formula compiled into the client.
	UNREFERENCED_PARAMETER(cmd);
	UNREFERENCED_PARAMETER(fProportionalTolerance);
	UNREFERENCED_PARAMETER(dwTimeTolerance);
	return TRUE;
#else
	if(tRateLimit.perticks[cmd] > 0 && 
		(messagecount[cmd]*tRateLimit.perticks[cmd] >
		fProportionalTolerance*tRateLimit.maxmessages[cmd]*
		(dwTickLast - dwTickStart + dwTimeTolerance)))
		return FALSE;
	else
		return TRUE;
	
#endif
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL MessageTracker::AddMessage(DWORD dwTick, NET_CMD cmd, unsigned int size)
{
	if(dwTickStart == 0) dwTickStart = dwTick;
	ASSERT_RETFALSE(dwTickLast - dwTickStart >= 0);

	dwTickLast = dwTick;

	ASSERT_RETFALSE(cmd >= 0 && cmd <= 255); //Guaranteed, it's a byte.
#if ISVERSION(RETAIL_VERSION) && !ISVERSION(SERVER_VERSION)
	UNREFERENCED_PARAMETER(cmd);
	UNREFERENCED_PARAMETER(size);
#else
	messagecount[cmd]++;
	sizecount[cmd] += size;
#endif
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void MessageTracker::TraceMessageRate(WCHAR * pszTypeString)
{
	float fTotalTime = float(dwTickLast - dwTickStart)/GAME_TICKS_PER_SECOND;
	if(fTotalTime == 0.0f) return;

	int nLastCommand = 256; // max possible netcmd
	TraceGameInfo("%lsTotal time %f", pszTypeString, fTotalTime);
	for(int i = 0; i < nLastCommand; i++)
	{
		if (messagecount[i] > 0)
			TraceGameInfo("%lsCommand %d used %f per second over %f seconds sending %f bytes per second",
				pszTypeString,
				i, messagecount[i]/fTotalTime,
				fTotalTime,
				sizecount[i]/fTotalTime);
	}

	return;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL MessageTracker::AddClientMessageToTrackerForCCMD(
	GAMECLIENT *pClient,
	DWORD dwTick,
	NET_CMD cmd,
	unsigned int size)
{
	ASSERT_RETFALSE(pClient);
	MessageTracker & tMessageTracker = pClient->m_tCMessageTracker;

	BOOL bRet = tMessageTracker.AddMessage(dwTick, cmd, size);
#if ISVERSION(SERVER_VERSION)
	if(!sbCCMDRateLimitInitialized) sInitMessageTrackerForCCMDRates();
	DWORD dwTickTime = tMessageTracker.dwTickLast - tMessageTracker.dwTickStart;

	if(!tMessageTracker.MessageRateIsAllowed(cmd, MAX_ALLOWED_PROPORTION, TIME_LEEWAY, sCCMDRateLimit))
	{
		if(dwTick > dwTicksMinAssertTime + tMessageTracker.dwTickAssert)
		{
			char szMessage[256];
			PStrPrintf(szMessage, 256, "SPEEDHACK! asserts: %d, cmd: %d msgs: %d ticks: %d excess: %f",
				tMessageTracker.nAsserts, cmd,
				tMessageTracker.messagecount[cmd], dwTickTime,
				float(tMessageTracker.messagecount[cmd]*sCCMDRateLimit.perticks[cmd])
				/float(sCCMDRateLimit.maxmessages[cmd]*dwTickTime));
#if !ISVERSION(DEVELOPMENT)
			UNITLOG_ASSERTX(FALSE, szMessage, ClientGetPlayerUnit(pClient));
#else
			TraceError("%s", szMessage);
#endif
			tMessageTracker.dwTickAssert = dwTick;
		}
		else
		{
			tMessageTracker.nAsserts++;
		}
	}
	if(!tMessageTracker.MessageRateIsAllowed(
		cmd, INSTANT_BOOT_PROPORTION, TIME_LEEWAY, sCCMDRateLimit))
	{
		bRet = FALSE;
	}
#endif
	
	return bRet;
}

//----------------------------------------------------------------------------
// Traces their level, dumps the message rate info, then resets the class.
//----------------------------------------------------------------------------
void TraceMessageRateForClient(GAME * pGame, GAMECLIENT *pClient)
{
	ASSERT_RETURN(pClient);

	BOOL bTown = (GameGetSubType(pGame) == GAME_SUBTYPE_BIGTOWN) || 
		(GameGetSubType(pGame) == GAME_SUBTYPE_MINITOWN);

	TraceGameInfo("Client %I64x exiting game %I64x.  bTown = %d",
		ClientGetId(pClient), 
		GameGetId(pGame),
		bTown);

	WCHAR szInfoString[32];
	if(sbEnableMessageTracing)
	{
		PStrPrintf(szInfoString, 32, L"%d C", bTown);
		pClient->m_tCMessageTracker.TraceMessageRate(szInfoString);
		TraceGameInfo("End CCMD trace.\n");
	}
	pClient->m_tCMessageTracker.Reset();
	if(sbEnableMessageTracing)
	{
		PStrPrintf(szInfoString, 32, L"%d S", bTown);
		pClient->m_tSMessageTracker.TraceMessageRate(szInfoString);
		TraceGameInfo("End SCMD trace.\n");
	}
	pClient->m_tSMessageTracker.Reset();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void EnableMessageTracing(BOOL bEnable)
{
	sbEnableMessageTracing = bEnable;
}