//----------------------------------------------------------------------------
// FILE: gag.cpp
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "gag.h"
#include "globalindex.h"
#include "units.h"
#include "s_partyCache.h"
#include "uix_priv.h"

//----------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct GAG_ACTION_LOOKUP
{
	GAG_ACTION eAction;
	const WCHAR *puszName;
};
static const GAG_ACTION_LOOKUP sgtGagActionLookup[] = 
{
	{ GAG_1_DAYS,		L"GAG_1_DAYS" },
	{ GAG_3_DAYS,		L"GAG_3_DAYS" },
	{ GAG_7_DAYS,		L"GAG_7_DAYS" },
	{ GAG_PERMANENT,	L"GAG_PERMANENT" },
	{ GAG_UNGAG,		L"GAG_UNGAG" },	
	{ GAG_INVALID,		NULL },
};

//----------------------------------------------------------------------------
// Definitions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_GagEnable(
	UNIT *pPlayer,
	time_t timeGaggedUntil,
	BOOL bInformChatServer)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );
	
	// if no time, we treat that as a disable
	if (timeGaggedUntil <= 0)
	{
		s_GagDisable( pPlayer, bInformChatServer );
	}
	else
	{

		// get the game client
		GAMECLIENT *pClient = UnitGetClient( pPlayer );
		ASSERTX_RETURN( pClient, "Expected game client" );

		// set value
		CLIENTSYSTEM *pClientSystem = AppGetClientSystem();			
		ClientSystemSetGaggedUntilTime(	pClientSystem, pClient->m_idAppClient, timeGaggedUntil );

		// set stat
		DWORD dwHigh;
		DWORD dwLow;
		UTCTIME_SPLIT( timeGaggedUntil, dwHigh, dwLow );	
		UnitSetStat( pPlayer, STATS_GAGGED_UNTIL_TIME, SPLIT_VALUE_64_PARAM_LOW, dwLow );
		UnitSetStat( pPlayer, STATS_GAGGED_UNTIL_TIME, SPLIT_VALUE_64_PARAM_HIGH, dwHigh );
		//UnitSetStat( pPlayer, STATS_GAGGED, TRUE );

		// udpate the chat server with the new gagged status of this player
		if (bInformChatServer)
		{
			UpdateChatServerConnectionContext( pPlayer );
		}
		
	}
#endif			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_GagDisable(
	UNIT *pPlayer,
	BOOL bInformChatServer)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERTX_RETURN( pPlayer, "Expected unit" );
	ASSERTX_RETURN( UnitIsPlayer( pPlayer ), "Expected player" );

	// get the game client
	GAMECLIENT *pClient = UnitGetClient( pPlayer );
	ASSERTX_RETURN( pClient, "Expected game client" );

	// clear any in game gag restriction
	CLIENTSYSTEM *pClientSystem = AppGetClientSystem();			
	ClientSystemSetGaggedUntilTime(	pClientSystem, pClient->m_idAppClient, 0 );

	// set stat
	UnitSetStat( pPlayer, STATS_GAGGED_UNTIL_TIME, SPLIT_VALUE_64_PARAM_LOW, 0 );
	UnitSetStat( pPlayer, STATS_GAGGED_UNTIL_TIME, SPLIT_VALUE_64_PARAM_HIGH, 0 );	
	//UnitSetStat( pPlayer, STATS_GAGGED, FALSE );

	// udpate the chat server with the new gagged status of this player
	if (bInformChatServer)
	{
		UpdateChatServerConnectionContext( pPlayer );
	}
	
#endif		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_GagIsGagged(
	UNIT *pPlayer)
{
	ASSERTX_RETFALSE( pPlayer, "Expected unit" );
	ASSERTX_RETFALSE( UnitIsPlayer( pPlayer ), "Expected player" );
	
	// ask the client system if this client is gagged
	time_t timeGaggedUntil = s_GagGetGaggedUntil( pPlayer );
	if (timeGaggedUntil != 0)
	{
	
		// get the current time and compare against gagged until time
		time_t tTimeNow = time(0);
		return (timeGaggedUntil - tTimeNow) > 0;
		
	}

	return FALSE;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
time_t s_GagGetGaggedUntil(
	UNIT *pPlayer)
{
	ASSERTX_RETZERO( pPlayer, "Expected unit" );
	ASSERTX_RETZERO( UnitIsPlayer( pPlayer ), "Expected player" );

	// get the game client
	GAMECLIENT *pClient = UnitGetClient( pPlayer );
	ASSERTX_RETZERO( pClient, "Expected game client" );
	
	// ask the client system if this client is gagged
	CLIENTSYSTEM *pClientSystem = AppGetClientSystem();		
	return ClientSystemGetGaggedUntilTime( pClientSystem, pClient->m_idAppClient );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
GAG_ACTION GagActionParse(
	const WCHAR *puszAction)
{
	ASSERTX_RETVAL( puszAction, GAG_INVALID, "Expected string" );
	
	for (int i = 0; sgtGagActionLookup[ i ].eAction != GAG_INVALID; ++i)
	{
		const GAG_ACTION_LOOKUP *pLookup = &sgtGagActionLookup[ i ];
		if (PStrICmp( puszAction, pLookup->puszName ) == 0)
		{
			return pLookup->eAction;
		}
	}

	return GAG_INVALID;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
time_t GagGetTimeFromNow(
	GAG_ACTION eGagAction)
{

	// get current time
	time_t lTime;
	time( &lTime );
   
	// how long is the gag
	switch (eGagAction)
	{
		case GAG_1_DAYS:	UTCTimeAddDay( &lTime, 1 );		break;
		case GAG_3_DAYS:	UTCTimeAddDay( &lTime, 3 );		break;
		case GAG_7_DAYS:	UTCTimeAddDay( &lTime, 7 );		break;
		case GAG_PERMANENT:	UTCTimeAddDay( &lTime, 9999 );	break;
	}		

	return lTime;
				
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_GagIsGagged(
	UNIT *pPlayer)
{
	ASSERTX_RETFALSE( pPlayer, "Expected unit" );
	ASSERTX_RETFALSE( UnitIsPlayer( pPlayer ), "Expected player unit" );

	// check the stat that the server sent us	
	DWORD dwLow = UnitGetStat( pPlayer, STATS_GAGGED_UNTIL_TIME, SPLIT_VALUE_64_PARAM_LOW );
	DWORD dwHigh = UnitGetStat( pPlayer, STATS_GAGGED_UNTIL_TIME, SPLIT_VALUE_64_PARAM_HIGH );
	time_t timeGaggedUntil = UTCTIME_CREATE( dwLow, dwHigh );

	// check against now	
	time_t timeNow = time(0);
	return (timeGaggedUntil - timeNow) > 0;
	
	//return UnitGetStat( pPlayer, STATS_GAGGED ) == TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_GagDisplayGaggedMessage(
	void)
{
#if !ISVERSION(SERVER_VERSION)	
	UIShowGenericDialog( GlobalStringGet( GS_GAG_TITLE ), GlobalStringGet( GS_GAG_MESSAGE ) );
#endif	
}	
