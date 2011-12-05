//----------------------------------------------------------------------------
// s_network.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifdef	_S_NETWORK_H_
//#pragma message ("    HINT: " __FILE__ " included multiple times in this translation unit")
#else
#define _S_NETWORK_H_


//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#ifndef _PRIME_H_
#error s_network.h requires prime.h be included first
#endif


//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
//----------------------------------------------------------------------------

void SrvProcessAppMessages(
	void);

GAMEID SrvNewGame(
	GAME_SUBTYPE eGameSubType,
	const GAME_SETUP * game_setup,
	BOOL bReserve,
	int nDifficulty = 0);

void SrvGameProcessMessages( 
	GAME * game);

BOOL SrvValidateMessageTable(
	void);

void SrvProcessDisconnectedNetclients();

void SrvProcessDirtyTownInstanceLists();

// CHB 2006.09.01 - consolecmd.cpp references this using ISVERSION(DEVELOPMENT)
#if ISVERSION(DEBUG_VERSION) || ISVERSION(DEVELOPMENT)
void sDebugTraceWeaponConfig(
	GAME * game, 
	UNIT * unit);
#endif

//----------------------------------------------------------------------------
#endif //#if !ISVERSION(CLIENT_ONLY_VERSION)
//----------------------------------------------------------------------------

// send a message to the server in local (single-player) mode
#if ISVERSION(DEBUG_VERSION)
#define SrvPostMessageToMailbox(g, clt, cmd, msg)		SrvPostMessageToMailboxDbg(g, clt, cmd, msg, __FILE__, __LINE__)
BOOL SrvPostMessageToMailboxDbg(
	GAMEID idGame,
	NETCLIENTID64 idClient,
	BYTE command,
	MSG_STRUCT * msg,
	const char * file,
	unsigned int line);
#else
BOOL SrvPostMessageToMailbox(
	GAMEID idGame,
	NETCLIENTID64 idClient,
	BYTE command,
	MSG_STRUCT * msg);
#endif

//this function seems to be in the wrong file
BOOL AppPostMessageToMailbox(
	NET_COMMAND_TABLE * cmd_tbl,
	int source,
	NETCLIENTID64 idClient,
	BYTE command,
	MSG_STRUCT * msg);

//this function seems to be in the wrong file
BOOL AppPostMessageToMailbox(
	NETCLIENTID64 idClient,
	BYTE command,
	MSG_STRUCT * msg);

BOOL s_NetworkJoinGameError(
	GAME * game,
	APPCLIENTID idAppClient,
	GAMECLIENT * client,
	UNIT * unit,
	int errorCode,
	const char * fmt,
	...);

#endif // _S_NETWORK_H_
