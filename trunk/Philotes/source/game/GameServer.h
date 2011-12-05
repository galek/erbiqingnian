//----------------------------------------------------------------------------
// GameServer.h
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _GAMESERVER_H_
#define _GAMESERVER_H_


//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
#define GAMEID_INDEX_START					0
#define GAMEID_INDEX_BITS					16												// GAMEID is a 64 bit value, of which bits 0..15 is the game index
#define GAMEID_ITER_START					GAMEID_INDEX_BITS
#define GAMEID_ITER_BITS					16												// ITER is stored in bits 16..31
#define GAMEID_SVRID_START					(GAMEID_ITER_START + GAMEID_ITER_BITS)			// shift to put SVRID in low bits
#define GAMEID_SVRID_BITS					32												// SVRID is stored in bits 32..63
#define GAMEID_TOTAL_BITS					(GAMEID_SVRID_START + GAMEID_SVRID_BITS)


//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------
// return the game index from the GAMEID
inline UINT GameIdGetIndex(
	GAMEID id)				
{ 
	return (UINT)MASK_NBIT(GAMEID, id, GAMEID_INDEX_START, GAMEID_INDEX_BITS);
}

// return the ITER part from the GAMEID (iter is assigned as a 1-up value for each new game of the same index
inline UINT GameIdGetIter(
	GAMEID id)				
{ 
	return (UINT)MASK_NBIT(GAMEID, id, GAMEID_ITER_START, GAMEID_ITER_BITS);
}

inline UINT32 GameIdGetSvrId(
	GAMEID id)	
{
	return (UINT32)MASK_NBIT(GAMEID, id, GAMEID_SVRID_START, GAMEID_SVRID_BITS);
}

GAMEID GameIdNew(
	UINT index,
	UINT iter);

UINT64 GameServerGenerateGUID(
	void);


#if !ISVERSION(CLIENT_ONLY_VERSION)
void GameServerNetClientRemove(
	NETCLIENTID64);
#endif

#if ISVERSION(SERVER_VERSION)
BOOL GameServerSendSwitchMessageUnsafe(
	NETCLIENTID64 idNetClient, 
	int nServerInstance,
	const WCHAR * szPlayerName,
	struct GAME *pGameCurrent = NULL);
#endif

#if ISVERSION(SERVER_VERSION)
BOOL GameServerSwitchInstance(
	NETCLIENTID64 idNetClient, 
	int nServerInstance,
	const WCHAR * szPlayerName,
	struct WARP_SPEC * pWarpSpec,
	GAMEID idGame);
#endif

#if !ISVERSION(CLIENT_ONLY_VERSION)
NETCLIENTID64 GameServerGetNetClientId64FromAccountId(
	UNIQUE_ACCOUNT_ID idAccount);
#endif

#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL GameServerBootAccountId(
	struct GameServerContext * pServerContext,
	UNIQUE_ACCOUNT_ID idAccount);
#endif

#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL GameServerGagAccountId(
	struct GameServerContext * pServerContext,
	UNIQUE_ACCOUNT_ID idAccount,
	enum GAG_ACTION eGagAction);
#endif

#if ISVERSION(SERVER_VERSION)
UNIQUE_ACCOUNT_ID GameServerGetAuctionHouseAccountID(
	GameServerContext* pSvrCtx);

PGUID GameServerGetAuctionHouseCharacterID(
	GameServerContext* pSvrCtx);

APPCLIENTID GameServerGetAuctionHouseAppClientID(
	GameServerContext* pSvrCtx);
#else
inline UNIQUE_ACCOUNT_ID GameServerGetAuctionHouseAccountID(
	GameServerContext* pSvrCtx) { REF( pSvrCtx ); return INVALID_UNIQUE_ACCOUNT_ID;}

inline PGUID GameServerGetAuctionHouseCharacterID(
	GameServerContext* pSvrCtx) { REF( pSvrCtx ); return INVALID_GUID; }

inline APPCLIENTID GameServerGetAuctionHouseAppClientID(
	GameServerContext* pSvrCtx) { REF( pSvrCtx ); return INVALID_CLIENTID; }
#endif

//----------------------------------------------------------------------------
// GAME SERVER TO SERVER COMMUNICATION FUNCTIONS
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL GameServerToChatServerSendMessage(
	MSG_STRUCT * msg,
	NET_CMD	command);

BOOL GameServerToAuctionServerSendMessage(
	MSG_STRUCT * msg,
	NET_CMD		 command);

BOOL GameServerToBillingProxySendMessage(
	MSG_STRUCT * msg,
	NET_CMD		 command,
	SVRINSTANCE billingInstance);
#endif
//----------------------------------------------------------------------------
// FUNCTION SHARED FOR TESTING
//----------------------------------------------------------------------------
#ifdef TEST_GAME_SERVER
void sServerToGameServerResponseCmdAssert(
	LPVOID svrContext,
	SVRID sender, 
	MSG_STRUCT * msg);
#endif


#endif //_GAMESERVER_H_
