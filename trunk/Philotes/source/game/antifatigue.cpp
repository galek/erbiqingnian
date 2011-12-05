//----------------------------------------------------------------------------
// FILE: antifatigue.h
//
// System which encourages minors not to play the game too much. Required by
// China.
//
// author: Chris March
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "antifatigue.h"
#include "units.h"
#include "s_message.h"
#include "quests.h"
#include "uix.h"
#include "states.h"
#include "clients.h"

#include "console.h"


//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

#define DO_TEST_CODE (ISVERSION(DEVELOPMENT) && 0)

//----------------------------------------------------------------------------
// ANTI_FATIGUE_STATUS_DATA contains the message and/or game limitations to 
// enforce when the account has reached a given amount of online time. 
// Game messages and limitations are specfied in "English anti fatigue criteria.doc"
//----------------------------------------------------------------------------
struct ANTI_FATIGUE_STATUS_DATA
{
	int nOnlineTimeFirstMessageInMinutes;
	int nOnlineTimeLastMessageInMinutes;
	int nMessageRepeatTimeInMinutes;
	GLOBAL_STRING eMessageStringGlobalIndex;
	int nExperienceAndDropRatePenaltyPct;
	ESTATE  ePenaltyState;
};

// I could make an excel spreadsheet for this, if it seems the data will be changing
static const ANTI_FATIGUE_STATUS_DATA sAntiFatigueStatusData[] =
{
#if DO_TEST_CODE
																								
	1,		1,			-1, GS_ANTIFATIGUE_HOUR1,			0,		STATE_NONE,					
	2,		2,			-1, GS_ANTIFATIGUE_HOUR2,			0,		STATE_NONE,					

	3,		3,			-1, GS_ANTIFATIGUE_HOUR3,			50,		STATE_ANTIFATIGUE_PENALTY1,	

	4,		6,			1, GS_ANTIFATIGUE_50PCT_PENALTY,	50,		STATE_ANTIFATIGUE_PENALTY1,	

	7,		INT_MAX,	2, GS_ANTIFATIGUE_100PCT_PENALTY,	100,	STATE_ANTIFATIGUE_PENALTY2,	
#else
	60,		60,			-1, GS_ANTIFATIGUE_HOUR1,			0,		STATE_NONE,					
	120,	120,		-1, GS_ANTIFATIGUE_HOUR2,			0,		STATE_NONE,					

	179,	179,		-1, GS_ANTIFATIGUE_HOUR3,			0,		STATE_NONE,	

	180,	299,		30, GS_ANTIFATIGUE_50PCT_PENALTY,	50,		STATE_ANTIFATIGUE_PENALTY1,	

	300,	INT_MAX,	15, GS_ANTIFATIGUE_100PCT_PENALTY,	100,	STATE_ANTIFATIGUE_PENALTY2,	
#endif
};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Updates the Anti-Fatigue system, displaying on-screen messages, and setting
// player stats as necessary. (unit timed event with low frequency)
//----------------------------------------------------------------------------
static BOOL s_sAntiFatigueSystemUpdate(
	GAME* pGame,
	UNIT* pPlayer,
	const struct EVENT_DATA& event_data)	
{
	GAMECLIENT *pClient = UnitGetClient( pPlayer );
	ASSERT_RETFALSE(pClient);

	APPCLIENTID idAppClient = ClientGetAppId(pClient);
	ASSERT_RETFALSE(idAppClient != INVALID_ID);

	int nCurrentOnlineTimeInMinutes = ClientGetFatigueOnlineSecs(AppGetClientSystem(), idAppClient) / 60;
	if (nCurrentOnlineTimeInMinutes <= 0) 
	{
		// the player might turn 18 during a play session, and no longer be subject to this system. Happy Birthday!
#if !DO_TEST_CODE
		// disable XP/drop penalties
		StateClearAllOfType( pPlayer, STATE_ANTIFATIGUE_PENALTY );
		UnitSetStat( pPlayer, STATS_ANTIFATIGUE_XP_DROP_PENALTY_PCT, 0 );
		return TRUE;
#endif //!DO_TEST_CODE
	}

	int nDataIndex = (int)event_data.m_Data1;
	int nRepeatsLeft = (int)event_data.m_Data2;
	BOOL bNoMessages = (BOOL)event_data.m_Data3;

	ASSERT_RETFALSE(nDataIndex >= 0 && nDataIndex < arrsize( sAntiFatigueStatusData )); 
	const ANTI_FATIGUE_STATUS_DATA *pData = sAntiFatigueStatusData + nDataIndex;
	if (pData)
	{
		// status changed. display message and update stats
		if (pData->eMessageStringGlobalIndex != GS_INVALID && !bNoMessages)
		{
			// send message to client to display anti fatigue message
			MSG_SCMD_UISHOWMESSAGE tMessage;
			tMessage.bType = (BYTE)QUIM_GLOBAL_STRING;
			tMessage.nDialog = pData->eMessageStringGlobalIndex;
			tMessage.bFlags = (BYTE)UISMS_FADEOUT;
			tMessage.nParam1 = 30 * GAME_TICKS_PER_SECOND; // duration

			// send message
			GAMECLIENTID idClient = UnitGetClientId( pPlayer );
			s_SendMessage( UnitGetGame( pPlayer ), idClient, SCMD_UISHOWMESSAGE, &tMessage );
		}

		StateClearAllOfType( pPlayer, STATE_ANTIFATIGUE_PENALTY );
		if (pData->ePenaltyState != STATE_NONE)
			s_StateSet(pPlayer, pPlayer, pData->ePenaltyState, 0);

		UnitSetStat( pPlayer, STATS_ANTIFATIGUE_XP_DROP_PENALTY_PCT, pData->nExperienceAndDropRatePenaltyPct );
	}

	if (pData->nOnlineTimeLastMessageInMinutes < INT_MAX)
		--nRepeatsLeft;

	if (nRepeatsLeft > 0)	
	{
		UnitRegisterEventTimed(pPlayer, s_sAntiFatigueSystemUpdate, EVENT_DATA(nDataIndex, nRepeatsLeft), pData->nMessageRepeatTimeInMinutes * 60 * GAME_TICKS_PER_SECOND);
	}
	return TRUE;
}

//----------------------------------------------------------------------------
// Updates the Anti-Fatigue system, displaying on-screen messages, and setting
// player stats as necessary. (unit timed event with low frequency)
//----------------------------------------------------------------------------
void s_AntiFatigueSystemPlayerInit(
	GAME* pGame,
	UNIT* pPlayer )
{
	GAMECLIENT *pClient = UnitGetClient( pPlayer );
	
	if (pClient == NULL)
	{
		return;
	}

	APPCLIENTID idAppClient = ClientGetAppId(pClient);
	ASSERT_RETURN(idAppClient != INVALID_ID);

	int nCurrentOnlineTimeInMinutes = ClientGetFatigueOnlineSecs(AppGetClientSystem(), idAppClient) / 60;

#if DO_TEST_CODE
	nCurrentOnlineTimeInMinutes = 12;
#endif //DO_TEST_CODE

	// register timed events for each repeating message
	for (int i = 0; i < arrsize( sAntiFatigueStatusData ); ++i )
	{
		const ANTI_FATIGUE_STATUS_DATA *pData = (sAntiFatigueStatusData + i);
		int nRepeatsLeft = 0;
		int nMinutesTillFirstEvent = -1;

		if (pData->nOnlineTimeLastMessageInMinutes >= nCurrentOnlineTimeInMinutes)
		{
			// schedule events
			nRepeatsLeft = 
				( pData->nMessageRepeatTimeInMinutes <= 0 ) ? 
					0 :
					( pData->nOnlineTimeLastMessageInMinutes == INT_MAX ) ? 
						INT_MAX :
						(1 + ((pData->nOnlineTimeLastMessageInMinutes - MAX(nCurrentOnlineTimeInMinutes, pData->nOnlineTimeFirstMessageInMinutes))/pData->nMessageRepeatTimeInMinutes));

			if ( pData->nOnlineTimeFirstMessageInMinutes >= nCurrentOnlineTimeInMinutes ) 
			{
				// first event time is yet to come
				nMinutesTillFirstEvent = pData->nOnlineTimeFirstMessageInMinutes - nCurrentOnlineTimeInMinutes; 
			}
			else if ( pData->nOnlineTimeFirstMessageInMinutes < nCurrentOnlineTimeInMinutes ) 
			{
				if (nRepeatsLeft)
				{
					// figure out time to next repeat event
					nMinutesTillFirstEvent = 
						( ( nCurrentOnlineTimeInMinutes - pData->nOnlineTimeFirstMessageInMinutes ) % pData->nMessageRepeatTimeInMinutes );
					if ( nMinutesTillFirstEvent )
						nMinutesTillFirstEvent = pData->nMessageRepeatTimeInMinutes - nMinutesTillFirstEvent;
				}
			}

			if (nMinutesTillFirstEvent == 0)
			{
				// do the event now
				s_sAntiFatigueSystemUpdate(
					pGame,
					pPlayer,
					EVENT_DATA(i, nRepeatsLeft, FALSE));
			}
			else if (nMinutesTillFirstEvent > 0)
			{
				UnitRegisterEventTimed(
					pPlayer, 
					s_sAntiFatigueSystemUpdate, 
					EVENT_DATA(i, nRepeatsLeft, FALSE), 
					nMinutesTillFirstEvent * 60 * GAME_TICKS_PER_SECOND);
			}
		}

		if (pData->nOnlineTimeFirstMessageInMinutes < nCurrentOnlineTimeInMinutes &&
			nMinutesTillFirstEvent != 0)
		{
			// activate any penalties for passed events, if an event wasn't taken care of this frame
			s_sAntiFatigueSystemUpdate(
				pGame,
				pPlayer,
				EVENT_DATA(i, 0, FALSE));	// just show the UI message, could opt not too, but the feedback seems like a good idea
		}
	}
}
