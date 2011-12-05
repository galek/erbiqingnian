//----------------------------------------------------------------------------
// clients.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "gameclient.h"
#include "clients.h"
#include "game.h"
#include "gag.h"
#include "units.h"
#include "ServerRunnerAPI.h"
#include "s_message.h"
#include "s_reward.h"					// SREWARD_SESSION
#include "rjdmovementtracker.h"			// rjdMovementTracker
#include "events.h"						// event system is used to remove client from game
#include "GameServer.h"
#include "level.h"						// get clients in level
#include "pets.h"						// used for known
#include "inventory.h"					// used for known
#include "unit_priv.h"					// used for known
#include "objects.h"					// used for known
#include "stash.h"						// used for known
#include "minitown.h"
#include "player.h"
#if ISVERSION(SERVER_VERSION)
#include "gameclient.cpp.tmh"
#include <Watchdogclient_counter_definitions.h>
#include <PerfHelper.h>
#include "AuctionServerDefines.h"
#include "GameAuctionCommunication.h"
#endif 


//----------------------------------------------------------------------------
// DEBUGGING CONSTANTS
//----------------------------------------------------------------------------
#define	DEBUG_KNOWN_UNITS				0
#define TRACE_KNOWN(fmt, ...)			// trace(fmt, __VA_ARGS__)


//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
#define GAMECLIENTID_GAMEID_BITS		32
#define GAMECLIENTID_GAMEID_MASK		MAKE_NBIT_MASK(GAMECLIENTID, GAMECLIENTID_GAMEID_BITS)
#define GAMECLIENTID_GAMEID_SHIFT		0
#define GAMECLIENTID_ITER_BITS			16
#define GAMECLIENTID_ITER_MASK			MAKE_NBIT_MASK(GAMECLIENTID, GAMECLIENTID_ITER_BITS)
#define GAMECLIENTID_ITER_SHIFT			(GAMECLIENTID_GAMEID_SHIFT + GAMECLIENTID_GAMEID_BITS)
#define GAMECLIENTID_INDEX_BITS			16
#define GAMECLIENTID_INDEX_MASK			MAKE_NBIT_MASK(GAMECLIENTID, GAMECLIENTID_INDEX_BITS)
#define GAMECLIENTID_INDEX_SHIFT		(GAMECLIENTID_ITER_SHIFT + GAMECLIENTID_ITER_BITS)


//----------------------------------------------------------------------------
// GAME-LEVEL CLIENT STRUCTURES
// GAMECLIENT is defined in clients.h
//----------------------------------------------------------------------------
struct GAME_CLIENT_LIST
{
	GAMECLIENT **						m_pClients;
	unsigned int						m_nMaxClients;
	unsigned int						m_nFirstUnused;

	GAMECLIENT *						m_pClientsInGameHead;
	GAMECLIENT *						m_pClientsInGameTail;
	unsigned int						m_nNumClients;

	GAMECLIENT *						m_pFreeList;

	volatile LONG						m_nIter;					// iter value
};


//----------------------------------------------------------------------------
// GAMECLIENTID inline functions.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static GAMECLIENTID GenerateGameClientId(
	UINT index,
	LONG iter,
	GAMEID idGame)
{
	DBG_ASSERT((GAMECLIENTID(index) & GAMECLIENTID_INDEX_MASK) == GAMECLIENTID(index));
	DBG_ASSERT((GAMECLIENTID(iter) & GAMECLIENTID_ITER_MASK) == GAMECLIENTID(iter));

	return ((GAMECLIENTID(index) & GAMECLIENTID_INDEX_MASK) << GAMECLIENTID_INDEX_SHIFT) + 
		((GAMECLIENTID(iter) & GAMECLIENTID_ITER_MASK) << GAMECLIENTID_ITER_SHIFT) +
		((GAMECLIENTID(idGame) & GAMECLIENTID_GAMEID_MASK) << GAMECLIENTID_GAMEID_SHIFT);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UINT GameClientIdGetIndex(
	GAMECLIENTID id)				
{ 
	return (UINT)((id >> GAMECLIENTID_INDEX_SHIFT) & GAMECLIENTID_INDEX_MASK); 
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UINT GameClientIdGetIter(
	GAMECLIENTID id)
{
	return (UINT)((id >> GAMECLIENTID_ITER_SHIFT) & GAMECLIENTID_ITER_MASK); 
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL QueryGameClientIdMatchesGameId(
	GAMEID idGame,
	GAMECLIENTID idGameClient)
{
	return (idGame & GAMECLIENTID_GAMEID_MASK) == ((idGameClient >>  GAMECLIENTID_GAMEID_SHIFT) & GAMECLIENTID_GAMEID_MASK);
}


//----------------------------------------------------------------------------
// GAME-LEVEL CLIENT FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ClientListInit(
	GAME * game,
	unsigned int max_players)
{
	ASSERT_RETFALSE(game);
	GAME_CLIENT_LIST * client_list = game->m_pClientList = (GAME_CLIENT_LIST *)GMALLOCZ(game, sizeof(GAME_CLIENT_LIST));
	ASSERT_RETFALSE(client_list);
	client_list->m_nMaxClients = max_players;
	client_list->m_pClients = (GAMECLIENT **)GMALLOCZ(game, sizeof(GAMECLIENT *) * max_players);
	client_list->m_pFreeList = NULL;
	client_list->m_nFirstUnused = 0;

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline GAMECLIENT * ClientListGetClient(
	GAME_CLIENT_LIST * client_list,
	GAMECLIENTID idClient)
{
	if (GameClientIdGetIndex(idClient) >= client_list->m_nFirstUnused)
		return NULL;

	ASSERT_RETNULL(client_list->m_pClients);
	GAMECLIENT * client = client_list->m_pClients[GameClientIdGetIndex(idClient)];
	ASSERT(client);
	return client;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int ClientListGetClientCount(
	GAME_CLIENT_LIST * client_list)
{
	ASSERT_RETZERO(client_list);
	return client_list->m_nNumClients;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ClientListExitClients(
	GAME* game)
{
	// Send exit game messages to all clients in the game
	//
	GAMECLIENT* gameClient = game->m_pClientList->m_pClientsInGameHead;
	while(gameClient)
	{
		s_GameExit(game, gameClient);
		
		gameClient = gameClient->m_pNext;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline GAMECLIENT * GameListGetFreeClient(
	GAME * game,
	GAME_CLIENT_LIST * client_list)
{
	ASSERT_RETNULL(game);
	ASSERT_RETNULL(client_list);

	GAMECLIENT * client = client_list->m_pFreeList;
	if (client)
	{
		client_list->m_pFreeList = client->m_pNext;
		client->m_pNext = NULL;

		// assign unique GAMECLIENTID to client
		UINT index =  GameClientIdGetIndex(client->m_idGameClient);
		LONG iter = InterlockedIncrement(&(client_list->m_nIter)); //interlocked probably unnecessary since a single game is in a single thread.
		client->m_idGameClient = GenerateGameClientId(index, iter, GameGetId(game));
	}
	else
	{
		ASSERT_RETNULL(client_list->m_nFirstUnused < client_list->m_nMaxClients);
		ASSERT_RETNULL(client_list->m_pClients[client_list->m_nFirstUnused] == NULL);

		unsigned int index = client_list->m_nFirstUnused++;
		client = client_list->m_pClients[index] = (GAMECLIENT *)GMALLOCZ(game, sizeof(GAMECLIENT));

		LONG iter = InterlockedIncrement(&(client_list->m_nIter)); //interlocked probably unnecessary since a single game is in a single thread.
		client->m_idGameClient = GenerateGameClientId(index, iter, GameGetId(game));

		// allocate session info
		client->m_pRewardSession = (SREWARD_SESSION *)GMALLOCZ(game, sizeof(SREWARD_SESSION));
		s_RewardSessionInit(client->m_pRewardSession);
		client->m_pTradeSession = (STRADE_SESSION *)GMALLOCZ(game, sizeof(STRADE_SESSION));
		s_TradeSessionInit(client->m_pTradeSession);
		ASSERT_DO(client->m_pBadges == NULL)
		{
			GFREE(game, client->m_pBadges);
			client->m_pBadges = NULL;
		}

		client->m_pMovementTracker = (rjdMovementTracker *)GMALLOCZ(game, sizeof(rjdMovementTracker));
	}

	return client;
}


//----------------------------------------------------------------------------
// FORWARD DECLARATION
//----------------------------------------------------------------------------
static void ClientInitKnown(
	GAME * game,
	GAMECLIENT * client);


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
GAMECLIENT * ClientAddToGame(
	GAME * game,
	APPCLIENTID idAppClient)
{
	ASSERT_RETNULL(game && game->m_pClientList);
	GAME_CLIENT_LIST * client_list = game->m_pClientList;

	NETCLIENTID64 idNetClient = ClientGetNetId(AppGetClientSystem(), idAppClient);
	ASSERT_RETNULL(idNetClient != INVALID_NETCLIENTID64);

	GAMECLIENT * client = GameListGetFreeClient(game, client_list);
	if (!client)
	{	//In this error condition, we should dump everything we know about the ingame client list.
#if ISVERSION(SERVER_VERSION)
		TraceGameInfo("Failed to get free client in game %I64x with %d in-game clients, game subtype %d",
			GameGetId(game), ClientListGetClientCount(game->m_pClientList), game->eGameSubType); 
		//Todo: figure out how to get the minitown node and trace the info.

		int nLevelDef = INVALID_ID;
		for (GAMECLIENT *pClient = ClientGetFirstInGame( game ); 
			pClient; 
			pClient = ClientGetNextInGame( pClient ))
		{
			UNIT *pUnit = ClientGetPlayerUnit(pClient);
			if(pUnit) 
			{
				nLevelDef = UnitGetLevelDefinitionIndex(pUnit);
				TraceGameInfo("Client in level %d named %s", nLevelDef, ClientGetName(pClient));
			}
			else TraceGameInfo("Unitless client named %s", ClientGetName(pClient));
		}
		MiniTownStatTrace(nLevelDef);
#endif

		return NULL;
	}
	else
	{
		TraceGameInfo("Got client %I64x from game %I64x with %d in-game clients",
			ClientGetId(client), GameGetId(game), ClientListGetClientCount(game->m_pClientList));

	}

	ClientSetState(client, GAME_CLIENT_STATE_CONNECTED);
	client->m_idAppClient = idAppClient;
	client->m_idNetClient = idNetClient;
	client->m_pGame = game;
	client->m_pBadges = (PROTECTED_BADGE_COLLECTION *)GMALLOCZ(game, sizeof(PROTECTED_BADGE_COLLECTION));
	new (client->m_pBadges) PROTECTED_BADGE_COLLECTION(ClientGetBadges(AppGetClientSystem(), idAppClient));
#if !ISVERSION(CLIENT_ONLY_VERSION)
	new (&client->m_emailSendLimitTokenBucket) TokenBucket(0.5, 10);
	new (&client->m_emailGeneralActionTokenBucket) TokenBucket(10, 10);
#endif

	ClientInitKnown(game, client);

	client_list->m_nNumClients++;

	client->m_pPrev = client_list->m_pClientsInGameTail;
	if (client->m_pPrev)
	{
		client->m_pPrev->m_pNext = client;
	}
	else
	{
		client_list->m_pClientsInGameHead = client;
	}
	client_list->m_pClientsInGameTail = client;

	TraceGameInfo("ClientAddToGame() -- game:%lli  gameclient:%lli  appclient:%lli  netclient:%lli", 
		GameGetId(game), ClientGetId(client), idAppClient, idNetClient);

	return client;
}


//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------
static inline void sClientFreeBadges(
	GAME * game,
	GAMECLIENT * client);

#if !ISVERSION(CLIENT_ONLY_VERSION)
static void sClientClearMessageBuffer(
	GAME * game,
	GAMECLIENT * client);
#endif

static void ClientFreeKnown(
	GAME * game,
	GAMECLIENT * client);


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void ClientListRecycleClient(
	GAME * game,
	GAME_CLIENT_LIST * client_list,
	GAMECLIENT * client)
{
	ClientSetState(client, GAME_CLIENT_STATE_FREE);
	client->m_idAppClient = (APPCLIENTID)INVALID_CLIENTID;
	client->m_idNetClient = INVALID_NETCLIENTID64;
	s_RewardSessionInit(client->m_pRewardSession);
	s_TradeSessionInit(client->m_pTradeSession);
	client->m_pMovementTracker->Reset();
	client->m_tCMessageTracker.Reset();
	client->m_tSMessageTracker.Reset();
	client->m_nMissedPingCount = 0;
	sClientFreeBadges(game, client);
#if !ISVERSION(CLIENT_ONLY_VERSION)
	sClientClearMessageBuffer(game, client);
	client->m_wszGuildName[0] = 0;
	client->m_eGuildRank = (BYTE)GUILD_RANK_INVALID;
	client->m_guildRankName[0] = 0;
#endif
	ClientFreeKnown(game, client);
	client->m_pPrev = NULL;
	client->m_pNext = client_list->m_pFreeList;
	client_list->m_pFreeList = client;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL GameClientListValidate(
	GAME * game,
	GAMECLIENT * client)
{
	GAME_CLIENT_LIST * client_list = game->m_pClientList;
	ASSERT_RETFALSE(client_list);
	ASSERT_RETFALSE(client_list->m_nNumClients >= 0 && client_list->m_nNumClients <= client_list->m_nMaxClients);

	GAMECLIENT * table[256];
	unsigned int tablesize = 0;
	BOOL found = client ? FALSE : TRUE;

	GAMECLIENT * cur = client_list->m_pClientsInGameHead;
	while (cur)
	{
		ASSERT(ClientGetState(cur) != GAME_CLIENT_STATE_FREE);
		if (cur == client)
		{
			found = TRUE;
		}
		for (unsigned int ii = 0; ii < tablesize; ++ii)
		{
			ASSERT_RETFALSE(table[ii] != cur);
		}
		table[tablesize++] = cur;
		if (!cur->m_pNext)
		{
			ASSERT_RETFALSE(cur == client_list->m_pClientsInGameTail);
		}
		cur = cur->m_pNext;
	}

	ASSERT_RETFALSE(found);
	ASSERT_RETFALSE(tablesize == client_list->m_nNumClients);
	return TRUE;
}


//----------------------------------------------------------------------------
// FORWARD DECLARATION
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sClientFlushMessageBuffer(
	GAME * game,
	GAMECLIENT * client);
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void GameRemoveClient(
	GAME * game,
	GAMECLIENT * client)
{
	ASSERT_RETURN(game && game->m_pClientList);
	ASSERT_RETURN(client && ClientGetState(client) != GAME_CLIENT_STATE_FREE);

	TraceGameInfo("GameRemoveClient() -- game:%lli  client:%lli  appclientid:%d  netclient:%lli", GameGetId(game), ClientGetId(client), client->m_idAppClient, client->m_idNetClient);

#if !ISVERSION(CLIENT_ONLY_VERSION)
	sClientFlushMessageBuffer(game, client);
#endif

#ifdef TRACE_MESSAGE_RATE
	TraceMessageRateForClient(game, client);
#endif

	DBG_ASSERT(GameClientListValidate(game, client));

	GAME_CLIENT_LIST * client_list = game->m_pClientList;

	if (client->m_pPrev)
	{
		client->m_pPrev->m_pNext = client->m_pNext;
	}
	else
	{
		client_list->m_pClientsInGameHead = client->m_pNext;
	}
	if (client->m_pNext)
	{
		client->m_pNext->m_pPrev = client->m_pPrev;
	}
	else
	{
		client_list->m_pClientsInGameTail = client->m_pPrev;
	}

	NETCLIENTID64 idNetClient = ClientGetNetId(client);
	REF(idNetClient);
	
	ClientSystemClearClientGame(AppGetClientSystem(), client->m_idAppClient);

	ClientListRecycleClient(game, game->m_pClientList, client);
	game->m_pClientList->m_nNumClients--;

	DBG_ASSERT(GameClientListValidate(game, NULL));
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ClientListFree(
	GAME * game)
{
	ASSERT_RETURN(game && game->m_pClientList);
	GAME_CLIENT_LIST * client_list = game->m_pClientList;

	while (client_list->m_pClientsInGameHead)
	{
		GameRemoveClient(game, client_list->m_pClientsInGameHead);
	}

	// free trade session info and movement tracker
	for (unsigned int ii = 0; ii < client_list->m_nMaxClients; ++ii)
	{
		GAMECLIENT * client = client_list->m_pClients[ii];
		if (client)
		{
			GFREE(game, client->m_pRewardSession);
			client->m_pRewardSession = NULL;
			GFREE(game, client->m_pTradeSession);
			client->m_pTradeSession = NULL;
			GFREE(game, client->m_pMovementTracker);
			client->m_pMovementTracker = NULL;
			sClientFreeBadges(game, client);
#if !ISVERSION(CLIENT_ONLY_VERSION)
			sClientClearMessageBuffer(game, client);
#endif
			GFREE(game, client);
			client_list->m_pClients[ii] = NULL;
		}
	}

	GFREE(game, client_list->m_pClients);
	GFREE(game, client_list);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL ClientRemoveFromGameNow(
	GAME * game,
	GAMECLIENT * client,
	BOOL bSaveSend)
{
	ASSERT_RETFALSE(game);
	ASSERT_RETFALSE(client);

	if (ClientGetState(client) == GAME_CLIENT_STATE_FREE) 
	{
		return FALSE; // client has already been removed; 2 remove messages probably processed.
	}

	GAMECLIENTID idClient = ClientGetId(client);
	ASSERT_RETFALSE(QueryGameClientIdMatchesGameId(GameGetId(game), idClient));

	NETCLIENTID64 idNetClient = ClientGetNetId(client);
	APPCLIENTID idAppClient = ClientGetAppId(client);

	// save his player if he has one (it's possible to join a server and leave
	// before actually going into the game and being given a player)
	UNIT * player = ClientGetPlayerUnit(client, TRUE);
	if (player)
	{
		// cancel any trades so we can save.
		if (TradeIsTrading(player))
		{
			s_TradeCancel(player, FALSE);
		}
		
		// save
		if (bSaveSend)
		{

			AppPlayerSave(player);
		}

		// remove player
		UnitFree(player, UFF_SEND_TO_CLIENTS);
	}
	
	// send msg to disconnect client player (right now client shuts down upon getting this message)
	if (bSaveSend)
	{
		MSG_SCMD_PLAYERREMOVE msg;
		msg.id = idClient;
		s_SendMessage(game, idClient, SCMD_PLAYERREMOVE, &msg);
	}

	NetClientSetGameClientId(idNetClient, INVALID_GAMECLIENTID);

	GameRemoveClient(game, client);

	ClientSystemSetLogoutSaveCallback(AppGetClientSystem(), idAppClient);

	GameServerNetClientRemove(idNetClient);

	return TRUE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL ClientRemoveFromGameDo(
	GAME * game,
	UNIT * unit,
	const EVENT_DATA & event_data)
{
	if (unit != NULL) {
#if ISVERSION(SERVER_VERSION)
		//Also send a message to auction server
		GAME_AUCTION_REQUEST_LOGOUT_MEMBER_MSG msgAuction;
		msgAuction.MemberGuid = UnitGetGuid(unit);
		GameServerToAuctionServerSendMessage(&msgAuction, GAME_AUCTION_REQUEST_LOGOUT_MEMBER);
#endif
	}

	GAMECLIENTID idClient = (GAMECLIENTID)event_data.m_Data1 + 
							(GAMECLIENTID(event_data.m_Data2) << 32); //64-bit GameclientID split in two.
	ASSERT_RETFALSE(idClient != INVALID_ID);

	GAMECLIENT * client = ClientListGetClient(game->m_pClientList, idClient);
	if (!client || idClient != client->m_idGameClient) 
	{
		return FALSE;
	}

	return ClientRemoveFromGameNow(game, client, TRUE);
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL ClientRemoveFromGame(
	GAME * game,
	GAMECLIENT * client,
	DWORD msecDelay)
{
	ASSERT_RETFALSE(game && client);

	// set client as disconnecting, further requests to client get control unit will fail
	ClientSetState(client, GAME_CLIENT_STATE_DISCONNECTING);

	// register event to remove client
	EVENT_DATA event_data( DWORD(client->m_idGameClient & 0xffffffff), DWORD(client-> m_idGameClient >> 32 ));
	//64 bit gameclientid split in two.
	GameEventRegister(game, ClientRemoveFromGameDo, NULL, NULL, &event_data, msecDelay);

	return TRUE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int GameGetClientCount(
	GAME * game)
{
	ASSERT_RETZERO(game && game->m_pClientList);
	return game->m_pClientList->m_nNumClients;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int GameGetInGameClientCount(
	struct GAME * game)
{
	ASSERTV_RETZERO( game, "Expected game" );
	int nCount = 0;
	for (GAMECLIENT *pClient = ClientGetFirstInGame( game );
		 pClient;
		 pClient = ClientGetNextInGame( pClient ))
	{
		nCount++;
	}
	return nCount;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
GAMECLIENT * ClientGetById(
	GAME * game,
	GAMECLIENTID idClient)
{
	ASSERT_RETNULL(game && game->m_pClientList);
	if (idClient == INVALID_GAMECLIENTID)
	{
		return NULL;
	}
	GAMECLIENT * client = ClientListGetClient(game->m_pClientList, idClient);
	if (!client || ClientGetState(client) == GAME_CLIENT_STATE_FREE)
	{
		TraceGameInfo("tried to get client (id:%lli) that's already been freed from game (%lli)", idClient, GameGetId(game));
		return NULL;
	}
	return client;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ClientSetPlayerUnit(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit)
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(client);

	if (unit && UnitTestFlag(unit, UNITFLAG_FREED))
		unit = NULL;

	// save pointer to player unit
	client->m_pPlayerUnit = unit;
	// this client is now using this unit as its control unit, which will set the unit's client id
	ClientSetControlUnit(game, client, unit);

	if (unit)
	{
		WCHAR wszName[MAX_CHARACTER_NAME];
		UnitGetName(unit, wszName, arrsize(wszName), 0);
		PStrCvt(client->m_szName, wszName, arrsize(client->m_szName));
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ClientSetControlUnit(
	GAME * game,
	GAMECLIENT * client,
	UNIT * unit)
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(client);

	if (unit && UnitTestFlag(unit, UNITFLAG_FREED))
		unit = NULL;

	client->m_pControlUnit = unit;
	if (unit)
	{
		UnitSetClientID(unit, ClientGetId(client));
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
GAMECLIENTID ClientGetId(
	GAMECLIENT * client)
{
	ASSERT_RETINVALID(client);
	return (GAMECLIENTID)client->m_idGameClient;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ClientGetName(
	GAMECLIENT * client,
	WCHAR * wszName,
	unsigned int len)
{
	static const WCHAR * wszUnknown = L"Unknown";
	
	if (!client)
	{
		PStrCopy(wszName, wszUnknown, len);
		return FALSE;
	}
	
	UNIT * unit = ClientGetPlayerUnit(client, TRUE);
	if (!unit)
	{
		PStrCopy(wszName, wszUnknown, len);
		return FALSE;
	}
	
	return UnitGetName(unit, wszName, len, 0);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const char * ClientGetName(
	GAMECLIENT * client)
{
	static const char * szUnknown = "Unknown";
	
	if (!client)
	{
		return szUnknown;
	}
	
	UNIT * unit = ClientGetPlayerUnit(client, TRUE);
	if (!unit)
	{
		return szUnknown;
	}
	
	return client->m_szName;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
NETCLIENTID64 ClientGetNetId(
	GAME * game,
	GAMECLIENTID idClient)
{
	GAMECLIENT * client = ClientGetById(game, idClient);
	ASSERT_RETVAL(client, INVALID_NETCLIENTID);
	return ClientGetNetId(client);
}


//----------------------------------------------------------------------------
// TODO: check that client is active
//----------------------------------------------------------------------------
GAMECLIENT * GameGetNextClient(
	struct GAME * game,
	GAMECLIENT * client)
{
	ASSERT_RETNULL(game && game->m_pClientList);
	GAME_CLIENT_LIST * client_list = game->m_pClientList;

	if (!client)
	{
		return client_list->m_pClientsInGameHead;
	}
	return client->m_pNext;
}


//----------------------------------------------------------------------------
// This function uses "in game" to mean in a room in the world
//----------------------------------------------------------------------------
GAMECLIENT * ClientGetFirstInGame(
	GAME * game)
{	
	ASSERT_RETNULL(game);
	GAMECLIENT * client = NULL;
	while ((client = GameGetNextClient(game, client)) != NULL)	
	{
		if (ClientGetState(client) == GAME_CLIENT_STATE_INGAME)
		{
			return client;
		}
	}
	return NULL;
}


//----------------------------------------------------------------------------
// This function uses "in game" to mean in a room in the world
//----------------------------------------------------------------------------
GAMECLIENT * ClientGetNextInGame( 
	GAMECLIENT * client)
{
	ASSERT_RETNULL(client);

	GAME * game = client->m_pGame;
	ASSERT_RETNULL(game);

	while ((client = GameGetNextClient(game, client)) != NULL)	
	{
		if (ClientGetState(client) == GAME_CLIENT_STATE_INGAME)
		{
			return client;
		}
	}
	return NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
GAMECLIENT * ClientGetFirstInLevel(
	LEVEL * level)
{
	ASSERT_RETNULL(level);
	
	GAME * game = LevelGetGame(level);
	ASSERT_RETNULL(game);

	GAMECLIENT * client = ClientGetFirstInGame(game);

	while (client)
	{
		UNIT * unit = ClientGetPlayerUnitForced(client);
		if (unit)
		{
			LEVEL * levelUnit = UnitGetLevel(unit);
			if (levelUnit == level)
			{
				return client;
			}
		}
		
		client = ClientGetNextInGame(client);
	}
	
	return NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
GAMECLIENT * ClientGetNextInLevel(
	GAMECLIENT * client,
	LEVEL * level)
{
	ASSERT_RETNULL(client);
	ASSERT_RETNULL(level);
	
	while ((client = ClientGetNextInGame(client)) != NULL)	
	{
		UNIT * unit = ClientGetPlayerUnitForced(client);
		if (unit)
		{
			LEVEL * levelUnit = UnitGetLevel(unit);
			if (levelUnit == level)
			{
				return client;
			}
		}
	}
	return NULL;
}


//----------------------------------------------------------------------------
// BADGES
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void sClientFreeBadges(
	GAME * game,
	GAMECLIENT * client)
{
	ASSERT_RETURN(client);
	if (client->m_pBadges)
	{
		GFREE(game, client->m_pBadges);
		client->m_pBadges = NULL;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ClientGetBadges(
	GAMECLIENT * client,
	BADGE_COLLECTION & badges)
{
	ASSERT_RETFALSE(client);
	ASSERT_RETFALSE(client->m_pBadges);
	return client->m_pBadges->GetBadgeCollection(badges);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ClientHasBadge(
	GAMECLIENT * client,
	int badge)
{
	if (!client)
	{
		return FALSE;
	}

	BADGE_COLLECTION badges;

	if (!ClientGetBadges(client, badges))
	{
		return FALSE;
	}

	return badges.HasBadge(badge);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ClientHasPermission(
	GAMECLIENT * client,
	const BADGE_COLLECTION & permission)
{
	if (!client)
	{
		return FALSE;
	}

	BADGE_COLLECTION badges;

	if (!ClientGetBadges(client, badges))
	{
		return FALSE;
	}

	return badges.HasPermission(permission);
}


//----------------------------------------------------------------------------
// try to add badges to appclient and database, if successful add them to gameclient
//----------------------------------------------------------------------------
BOOL GameClientAddAccomplishmentBadge(
	GAMECLIENT * client,
	int badge)
{
	ASSERTX_RETFALSE(BadgeIsAccomplishment(badge), "Only accomplishment badges can be added by the game server directly.");
	ASSERT_RETFALSE(client);

	APPCLIENTID idAppClient = ClientGetAppId(client);
	ASSERT_RETFALSE(idAppClient != INVALID_ID);
	
	if (!AppClientAddAccomplishmentBadge(AppGetClientSystem(), idAppClient, badge))
	{
		return FALSE;
	}
	client->m_pBadges->AddAccomplishmentBadge(badge);
	return TRUE;
}


//----------------------------------------------------------------------------
// GAMECLIENT NETWORKING FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline void s_IncrementGameServerMessageCounter(
	void)
{
#if ISVERSION(SERVER_VERSION)
	GameServerContext * serverContext = (GameServerContext*)CurrentSvrGetContext();
	ASSERT_RETURN(serverContext);
	PERF_ADD64(serverContext->m_pPerfInstance, GameServer, GameServerMessagesSentPerSecond, 1);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sSendMessageBufferToClient(
	GAME * game,
	GAMECLIENT * client)
{
	ASSERT_RETFALSE(game);
	ASSERT_RETFALSE(client);

	ASSERT_RETFALSE(client->m_NetData.m_TransactionIndex == 0);		// can't send a buffer to the client while in the middle of a transaction
	ASSERT_RETFALSE(client->m_NetData.m_pHead);						// the first transaction buffer must always exist

	NETCLIENTID64 idNetClient = ClientGetNetId(game, ClientGetId(client));
	ASSERT_RETFALSE(idNetClient != INVALID_NETCLIENTID64);

	GAMECLIENT_MESSAGE_BUFFER * curr = NULL;
	GAMECLIENT_MESSAGE_BUFFER * next = client->m_NetData.m_pHead;

	BOOL retval = TRUE;
	unsigned int count = 0;

	while ((curr = next) != NULL)
	{
		next = curr->m_pNext;

		if (curr->m_ByteBufCur > 0)
		{
			if (SvrNetSendBufferToClient(idNetClient, curr->m_ByteBuf, curr->m_ByteBufCur) != SRNET_SUCCESS)
			{
				retval = FALSE;
				ASSERT(retval);
			}
		}
		if (curr == client->m_NetData.m_pHead)
		{
			curr->m_ByteBufCur = 0;
			curr->m_pNext = NULL;
		}
		else
		{
			GFREE(game, curr);
		}
		++count;
	}
	
	client->m_NetData.m_pTail = client->m_NetData.m_pHead;
	ASSERT(count == client->m_NetData.m_MsgBufCount);
	client->m_NetData.m_MsgBufCount = 1;

	return retval;
}
#endif


//----------------------------------------------------------------------------
// sends out all remaining buffered messages.
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sClientFlushMessageBuffer(
	GAME * game,
	GAMECLIENT * client)
{
	return sSendMessageBufferToClient(game, client);
}
#endif


//----------------------------------------------------------------------------
// resets buffered messages without sending (on client destroy)
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static void sClientClearMessageBuffer(
	GAME * game,
	GAMECLIENT * client)
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(client);

	ASSERT(client->m_NetData.m_TransactionIndex == 0);			// can't clear buffer to the client while in the middle of a transaction

	GAMECLIENT_MESSAGE_BUFFER * curr = NULL;
	GAMECLIENT_MESSAGE_BUFFER * next = client->m_NetData.m_pHead;

	while ((curr = next) != NULL)
	{
		next = curr->m_pNext;

		GFREE(game, curr);
	}

	structclear(client->m_NetData);
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static inline GAMECLIENT_MESSAGE_BUFFER * sSendMessageToClientAllocateBuffer(
	GAME * game, 
	GAMECLIENT * client)
{
	GAMECLIENT_MESSAGE_BUFFER * curr = (GAMECLIENT_MESSAGE_BUFFER *)GMALLOC(game, sizeof(GAMECLIENT_MESSAGE_BUFFER));
	curr->m_ByteBufCur = 0;
	curr->m_pNext = NULL;
	if (client->m_NetData.m_pTail)
	{
		ASSERT(client->m_NetData.m_pHead);
		client->m_NetData.m_pTail->m_pNext = curr;
	}
	else
	{
		client->m_NetData.m_pHead = curr;
	}
	client->m_NetData.m_pTail = curr;
	client->m_NetData.m_MsgBufCount++;
	return curr;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SendMessageTransactionBegin(
	GAME * game,
	GAMECLIENT * client)
{
	ASSERT_RETFALSE(game);
	ASSERT_RETFALSE(client);

#if !ISVERSION(CLIENT_ONLY_VERSION)
	GAME_TICK tick = GameGetTick(game);
	ASSERT_RETFALSE((client->m_NetData.m_TransactionIndex == 0 && client->m_NetData.m_tiTransactionTick == 0) || client->m_NetData.m_tiTransactionTick == tick);
	ASSERT_RETFALSE(client->m_NetData.m_TransactionIndex < MAX_MSGBUF_TRANSACTIONS);		// exceeded maximum number of nested transactions

	client->m_NetData.m_Transactions[client->m_NetData.m_TransactionIndex].m_BufferIndex = client->m_NetData.m_MsgBufCount - 1;
	if (client->m_NetData.m_pTail)
	{
		client->m_NetData.m_Transactions[client->m_NetData.m_TransactionIndex].m_BufferOffset = client->m_NetData.m_pTail->m_ByteBufCur;
	}
	else
	{
		client->m_NetData.m_Transactions[client->m_NetData.m_TransactionIndex].m_BufferOffset = 0;
	}
	++client->m_NetData.m_TransactionIndex;
	client->m_NetData.m_tiTransactionTick = tick;
#endif
	return TRUE;
}


//----------------------------------------------------------------------------
// delete the last msg buf
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static inline BOOL sSendMessageTransactionDeleteTail(
	GAME * game,
	GAMECLIENT * client)
{
	GAMECLIENT_MESSAGE_BUFFER * curr = client->m_NetData.m_pHead;
	ASSERT_RETFALSE(curr);
	ASSERT_RETFALSE(client->m_NetData.m_pTail);
	while (TRUE)
	{
		if (curr->m_pNext == client->m_NetData.m_pTail)
		{
			ASSERT(curr->m_pNext->m_pNext == NULL);
			curr->m_pNext = NULL;
			GFREE(game, client->m_NetData.m_pTail);
			client->m_NetData.m_pTail = curr;
			--client->m_NetData.m_MsgBufCount;
			break;
		}
		curr = curr->m_pNext;
	}
	return TRUE;
}
#endif


//----------------------------------------------------------------------------
// roll back before given transaction
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sSendMessageTransactionRollback(
	GAME * game,
	GAMECLIENT * client,
	unsigned int transactionidx)
{
	ASSERT_RETFALSE(game);
	ASSERT_RETFALSE(client);
	ASSERT_RETFALSE(transactionidx < client->m_NetData.m_TransactionIndex);
	ASSERT_RETFALSE(client->m_NetData.m_tiTransactionTick == GameGetTick(game));

	unsigned int ii = transactionidx - 1;
	unsigned int bufidx = client->m_NetData.m_Transactions[ii].m_BufferIndex;
	unsigned int bufoffs = client->m_NetData.m_Transactions[ii].m_BufferOffset;
	ASSERT_RETFALSE(bufidx < client->m_NetData.m_MsgBufCount);
	while (client->m_NetData.m_MsgBufCount > bufidx + 1)
	{
		sSendMessageTransactionDeleteTail(game, client);
	}
	ASSERT_RETFALSE(client->m_NetData.m_pTail || (bufidx == 0 && bufoffs == 0));
	if (client->m_NetData.m_pTail)
	{	
		client->m_NetData.m_pTail->m_ByteBufCur = bufoffs;
	}
	client->m_NetData.m_TransactionIndex = transactionidx;
	return TRUE;
}
#endif


//----------------------------------------------------------------------------
// roll back one level of transaction
//----------------------------------------------------------------------------
BOOL SendMessageTransactionRollback(
	GAME * game,
	GAMECLIENT * client)
{
	ASSERT_RETFALSE(game);
	ASSERT_RETFALSE(client);

#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETFALSE(client->m_NetData.m_TransactionIndex > 0);
	return sSendMessageTransactionRollback(game, client, client->m_NetData.m_TransactionIndex - 1);
#else
	return FALSE;
#endif
}


//----------------------------------------------------------------------------
// roll back all transactions
//----------------------------------------------------------------------------
BOOL SendMessageTransactionCancelAll(
	GAME * game,
	GAMECLIENT * client)
{
	ASSERT_RETFALSE(game);
	ASSERT_RETFALSE(client);
#if !ISVERSION(CLIENT_ONLY_VERSION)
	if (client->m_NetData.m_TransactionIndex <= 0)
	{
		return TRUE;
	}
	return sSendMessageTransactionRollback(game, client, 0);
#else
	return FALSE;
#endif
}


//----------------------------------------------------------------------------
// commits one level of transactions (still not finalized until all trnasactions
// have been comitted however
//----------------------------------------------------------------------------
BOOL SendMessageTransactionCommit(
	GAME * game,
	GAMECLIENT * client)
{
	ASSERT_RETFALSE(game);
	ASSERT_RETFALSE(client);
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETFALSE(client->m_NetData.m_tiTransactionTick == GameGetTick(game));
	ASSERT_RETFALSE(client->m_NetData.m_TransactionIndex > 0);

	--client->m_NetData.m_TransactionIndex;
#endif
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static inline BOOL sSendMessageToClient(
	GAME * game,
	GAMECLIENT * client,
	NET_CMD command,
	MSG_STRUCT * msgstruct)
{
	static NET_COMMAND_TABLE * cmdtbl = s_NetGetCommandTable();
	ASSERT_RETFALSE(cmdtbl);
	if (!game)
	{
		return TRUE;
	}

	ASSERT_RETFALSE(client);

	if (!msgstruct)
	{
		return FALSE;
	}

	unsigned int msgSize = NetCalcMessageSize(cmdtbl, command, msgstruct);
	ASSERT_RETFALSE(msgSize <= MAX_SMALL_MSG_SIZE);

	GAMECLIENT_MESSAGE_BUFFER * curr = client->m_NetData.m_pTail;
	if (NULL == curr)
	{
		ASSERT_RETFALSE(NULL == client->m_NetData.m_pHead);
		curr = sSendMessageToClientAllocateBuffer(game, client);
		ASSERT_RETFALSE(curr);
	}

	if (curr->m_ByteBufCur + msgSize > MAX_SMALL_MSG_SIZE)
	{
		if (client->m_NetData.m_Transactions == 0)			// either send the buffer if not in transaction mode, or allocate another buffer
		{
			sSendMessageBufferToClient(game, client);
			curr = client->m_NetData.m_pHead;
		}
		else
		{
			curr = sSendMessageToClientAllocateBuffer(game, client);
		}
		ASSERT_RETFALSE(curr);
	}

	BYTE * msgbuf = curr->m_ByteBuf + curr->m_ByteBufCur;
	UINT size = 0;
	BYTE_BUF_NET buf(msgbuf, MAX_SMALL_MSG_SIZE - curr->m_ByteBufCur);
	ASSERT_RETFALSE(NetTranslateMessageForSend(cmdtbl, command, msgstruct, size, buf));
	ASSERT(size == msgSize);
	curr->m_ByteBufCur += size;

	// track here.  Pray that BUFFER_GAME_MESSAGES is TRUE in s_message, or this is redundant.
	client->m_tSMessageTracker.AddMessage(GameGetTick(game), command, size);

	s_IncrementGameServerMessageCounter();

	return TRUE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InitClientMesssageTracking(
	void)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	sSendMessageToClient(NULL, NULL, NULL, NULL);
#endif
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SendClientMessages(
	GAME * game)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	for (GAMECLIENT * client = ClientGetFirstInGame(game); client; client = ClientGetNextInGame(client))
	{
		sSendMessageBufferToClient(game, client);
	}
#else
	REF(game);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SendMessageToClient(
	GAME * game,
	GAMECLIENT * client,
	NET_CMD command,
	MSG_STRUCT * msgstruct)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	return sSendMessageToClient(game, client, command, msgstruct);
#else
	REF(game);
	REF(client);
	REF(command);
	REF(msgstruct);
	return FALSE;
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL SendMessageToClient(
	GAME * game,
	GAMECLIENTID idClient,
	NET_CMD command,
	MSG_STRUCT * msgstruct)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	GAMECLIENT * client = ClientGetById(game, idClient);
	ASSERT_RETFALSE(client);
	return sSendMessageToClient(game, client, command, msgstruct);
#else
	REF(game);
	REF(idClient);
	REF(command);
	REF(msgstruct);
	return FALSE;
#endif
}


//----------------------------------------------------------------------------
// KNOWN
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sClientTestRoomKnown(
	GAMECLIENT * client,
	ROOM * room)
{
	unsigned int roomidx = RoomGetIndexInLevel(room);
	ASSERT_RETFALSE(roomidx < GetMaxRoomKnownFlagIndex());

	return TESTBIT(client->m_Known.m_pdwRoomKnownFlags, roomidx);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sClientRoomKnownTestLevel(
	GAMECLIENT * client,
	ROOM * room,
	BOOL bIgnoreClientPlayerState)
{
	ASSERT_RETFALSE(client);
	ASSERT_RETFALSE(room);

	UNIT * unit = ClientGetControlUnit(client, TRUE);
	if (unit == NULL)
	{
		return FALSE;
	}

	LEVEL * levelUnit = UnitGetLevel(unit);
	LEVEL * levelRoom = RoomGetLevel(room);
	if (!levelUnit || levelRoom != levelUnit)
	{
		return FALSE;  // on different levels, can't possibly know about room
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ClientSetRoomKnown(
	GAMECLIENT * client,
	ROOM * room,
	BOOL bKnown)
{
	unsigned int roomidx = RoomGetIndexInLevel(room);
	ASSERT_RETURN(roomidx < GetMaxRoomKnownFlagIndex());

	if (!sClientRoomKnownTestLevel(client, room, TRUE))
	{
		return;
	}
	if (bKnown)
	{
		if (!TESTBIT(client->m_Known.m_pdwRoomKnownFlags, RoomGetIndexInLevel(room)))
		{
			room->nKnownByClientRefCount++;
		}
		SETBIT(client->m_Known.m_pdwRoomKnownFlags, RoomGetIndexInLevel(room));
	}
	else
	{
		if (TESTBIT(client->m_Known.m_pdwRoomKnownFlags, RoomGetIndexInLevel(room)))
		{
			room->nKnownByClientRefCount--;
		}
		CLEARBIT(client->m_Known.m_pdwRoomKnownFlags, RoomGetIndexInLevel(room));
	}
}	


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ClientIsRoomKnown(	
	GAMECLIENT * client,
	ROOM * room,
	BOOL bIgnoreClientPlayerState /*= FALSE*/)
{
	ASSERT_RETFALSE(client);
	ASSERT_RETFALSE(room);

	if (!sClientRoomKnownTestLevel(client, room, bIgnoreClientPlayerState))
	{
		return FALSE;
	}
		
	unsigned int roomidx = RoomGetIndexInLevel(room);
	ASSERT_RETFALSE(roomidx < GetMaxRoomKnownFlagIndex());

	return TESTBIT(client->m_Known.m_pdwRoomKnownFlags, RoomGetIndexInLevel(room));
}	


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
ROOM * ClientGetFirstRoomKnown(
	GAMECLIENT * client,
	UNIT * unit)
{
	ASSERT_RETNULL(client);
	ASSERT_RETNULL(unit);
	ASSERT_RETNULL(UnitGetClient(unit) == client);

	LEVEL * level = RoomGetLevel(UnitGetRoom(unit));
	ASSERT_RETNULL(level);

	for (ROOM * room = LevelGetFirstRoom(level); room; room = LevelGetNextRoom(room))
	{
		if (sClientTestRoomKnown(client, room))
		{
			return room;
		}
	}
	return NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
ROOM * ClientGetNextRoomKnown(
	GAMECLIENT * client,
	UNIT * unit,
	ROOM * room)
{
	ASSERT_RETNULL(room);
	ASSERT_RETNULL(client);
	ASSERT_RETNULL(unit);
	ASSERT_RETNULL(UnitGetClient(unit) == client);

	LEVEL * level = RoomGetLevel(UnitGetRoom(unit));
	ASSERT_RETNULL(level);

	DBG_ASSERT(sClientTestRoomKnown(client, room));

	ROOM * next = LevelGetFirstRoom(level);
	while (next)
	{
		if (next == room)
		{
			next = LevelGetNextRoom(next);
			break;
		}
		next = LevelGetNextRoom(next);
	}

	while (next)
	{
		if (sClientTestRoomKnown(client, next))
		{
			return next;
		}
		next = LevelGetNextRoom(next);
	}

	return NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ClientClearAllRoomsKnown(
	GAMECLIENT * client,
	LEVEL * level)
{
	ASSERT_RETURN(client);
	
	if (level)
	{
		// go through each room
		for (ROOM * room = LevelGetFirstRoom(level); room; room = LevelGetNextRoom(room))
		{
			ASSERT_CONTINUE(RoomGetIndexInLevel(room) >= 0 && ((unsigned int) RoomGetIndexInLevel(room)) < GetMaxRoomKnownFlagIndex());
			if (TESTBIT(client->m_Known.m_pdwRoomKnownFlags, RoomGetIndexInLevel(room)))
			{
				--room->nKnownByClientRefCount;
			}
		}
	}
	memclear(client->m_Known.m_pdwRoomKnownFlags, DWORD_FLAG_SIZE(MAX_ROOMS_PER_LEVEL) * sizeof(DWORD));	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sUnitIsClientPet(
	UNIT * unit,
	UNIT * pet)
{
	if (!PetIsPet(pet))
	{
		return FALSE;
	}

	UNITID idOwner = PetGetOwner(pet);
	if (idOwner == INVALID_ID)
	{
		return FALSE;
	}

	UNIT * owner = UnitGetById(UnitGetGame(unit), idOwner);
	if (!owner)
	{
		return FALSE;
	}

	UNIT * ultimate_owner = UnitGetUltimateOwner(owner);
	if (!ultimate_owner)
	{
		return FALSE;
	}

	if (ultimate_owner != unit)
	{
		return FALSE;
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sUnitIsInClientInventory(
	UNIT * clientunit,
	UNIT * unit)
{
	UNIT * owner = UnitGetUltimateContainer(unit);
	if (!owner)
	{
		return FALSE;
	}

	if (clientunit == owner && UnitIsPhysicallyInContainer(unit))
	{
		return TRUE;
	}

	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_ClientAlwaysKnowsUnitInWorld( 
	UNIT * unit)
{
	ASSERT_RETFALSE(unit);

	GAME * game = UnitGetGame(unit);
	ASSERT_RETFALSE(game && IS_CLIENT(game));

	UNIT * clientunit = GameGetControlUnit(game);

	// must be in world
	if (UnitGetRoom(unit) == NULL)
	{
		return FALSE;
	}
	
	// clients always know about their control units
	if (clientunit == unit)
	{
		return TRUE;
	}

	// clients always know about their pets
	if (sUnitIsClientPet(clientunit, unit))
	{
		return TRUE;
	}
	
	// clients always know about things that they contain that are in the world
	if (sUnitIsInClientInventory(clientunit, unit))
	{
		return TRUE;
	}

	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL s_ClientAlwaysKnowsUnitInWorld( 
									GAMECLIENT* pClient,
									UNIT * unit)
{
	ASSERT_RETFALSE(unit);
	ASSERT_RETFALSE(pClient);

	UNIT * clientunit = ClientGetControlUnit(pClient);

	GAME * game = UnitGetGame(unit);
	ASSERT_RETFALSE(game && IS_SERVER(game));


	// must be in world
	if (UnitGetRoom(unit) == NULL)
	{
		return FALSE;
	}

	// clients always know about their control units
	if (clientunit == unit)
	{
		return TRUE;
	}

	// clients always know about their pets
	if (sUnitIsClientPet(clientunit, unit))
	{
		return TRUE;
	}

	// clients always know about things that they contain that are in the world
	if (sUnitIsInClientInventory(clientunit, unit))
	{
		return TRUE;
	}

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sUnitIsAvailableToPlayer(
	UNIT * player,
	UNIT * unit)
{
	ASSERT_RETFALSE(player);
	ASSERT_RETFALSE(unit);
	ASSERT_RETFALSE(UnitIsPlayer(player));

	// is this unit restricted to any guids
	PGUID guidRestrictedTo = UnitGetRestrictedToGUID(unit);
	if (guidRestrictedTo != INVALID_GUID)
	{
		if (UnitGetGuid(player) != guidRestrictedTo)
		{
			return FALSE;  // restricted to somebody else
		}		
	}

	// unit data flags
	const UNIT_DATA * unit_data = UnitGetData(unit);
	ASSERT_RETFALSE(unit_data);
	if (UnitDataTestFlag(unit_data, UNIT_DATA_FLAG_REQUIRES_CAN_OPERATE_TO_BE_KNOWN))
	{
		if (UnitIsA(unit, UNITTYPE_OBJECT))
		{
			if (ObjectCanOperate(player, unit) == FALSE)
			{
				return FALSE;
			}
		}
	}
		
	return TRUE;  // not restricted
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UNIT_KNOWN * ClientKnownFindByUnitId(
	GAMECLIENT * client,
	UNITID idUnit)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	GAMECLIENT_KNOWN_DATA & known = client->m_Known;

	unsigned int key = idUnit % KNOWN_HASH_SIZE;

	UNIT_KNOWN * next = known.m_hashUnitId[key];
	UNIT_KNOWN * curr = NULL;
	while ((curr = next) != NULL)
	{
		next = curr->nextInUnitIdHash;
		if (curr->idUnit == idUnit)
		{
			return curr;
		}
	}
#else
	REF(client);
	REF(idUnit);
#endif
	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsCachedOnClient(
	GAMECLIENT * client,
	UNIT * unit)
{
	ASSERT_RETFALSE(client);
	ASSERT_RETFALSE(unit);

#if ISVERSION(CLIENT_ONLY_VERSION)
	return FALSE;
#endif
	
	BOOL bIsNeverRemoved = FALSE;
	
	UNITID idUnit = UnitGetId( unit );
	UNIT_KNOWN *pKnown = ClientKnownFindByUnitId( client, idUnit );	
	if (pKnown)
	{
	
		// we never remove cached items
		if (TESTBIT( pKnown->dwUnitKnownFlags, UKF_CACHED ))
		{
			bIsNeverRemoved = TRUE;
		}
	}

	return bIsNeverRemoved;	
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ClientCanKnowUnit(	
	GAMECLIENT * client, 
	UNIT * unit,
	BOOL bIgnoreClientPlayerState /*= FALSE*/,
	BOOL bAllowStashWhenClosed /*= FALSE*/)
{
	ASSERT_RETFALSE(client);
	ASSERT_RETFALSE(unit);

	// if a unit is currently being remembered by a client, they always know about it
	if (UnitTestFlag(unit, UNITFLAG_FREED) == FALSE && UnitIsCachedOnClient( client, unit ))
	{
		return TRUE;
	}
	
	// check units that are restricted to a single GUID
	UNIT * player = ClientGetControlUnit(client, bIgnoreClientPlayerState);
	if (player)
	{
		if (sUnitIsAvailableToPlayer(player, unit) == FALSE)
		{
			return FALSE;
		}
	}
				
	UNIT * owner = UnitGetUltimateOwner(unit);

	// if client doesn't know about the room the unit is in, then the answer is always no
	ROOM * room = UnitGetRoom(unit);
	if (room == NULL)
	{		
		room = UnitGetRoom(owner);	// unit itself has no room, check the ultimate owner
	}
	if (room == NULL)
	{
		return FALSE;
	}
		
	// client must know about the room this unit is in
	if (ClientIsRoomKnown(client, room, bIgnoreClientPlayerState) == FALSE)
	{
		return FALSE;
	}
				
	// get container
	UNIT * container = UnitGetContainer(unit);
	if (container)
	{
		UNIT * ultimateContainer = UnitGetUltimateContainer(unit);

		// Not checking this yet ... Colin
		// is the unit contained in an inventory location that no clients can know about?
		if (InventoryIsInServerOnlyLocation(unit))
		{
			return FALSE;
		}

		// for items inside of a stash
		if (!bAllowStashWhenClosed &&
			StashIsItemIn(unit))
		{
			INVENTORY_LOCATION tInvLoc;
			UnitGetInventoryLocation(unit, &tInvLoc);
			if (InvLocIsOnlyKnownWhenStashOpen(container, tInvLoc.nInvLocation) == TRUE)
			{
				if (StashIsOpen(ultimateContainer) == FALSE)
				{
					return FALSE;
				}
			}
		}

		if (AppIsTugboat() &&
			UnitIsA(unit, UNITTYPE_PLAYER) &&
			container != player)
		{
			// in Tugboat, we tend to send the real inventory of players -
			// but we only want to send stuff they are trading or wearing.
			INVENTORY_LOCATION tInvLoc;
			UnitGetInventoryLocation(unit, &tInvLoc);
			if (UnitIsPhysicallyInContainer(unit) &&
				!InventoryIsTradeLocation(container, tInvLoc.nInvLocation) &&
				!ItemIsEquipped(unit, container) &&
				!ItemIsInWeaponSetLocation(container, unit))
			{
				return FALSE;
			}
		}

		if (UnitIsPhysicallyInContainer(unit))
		{
			// OK, we know about the room the unit or its owner is in, if the unit is in an inventory
			// of some kind, we need to ask if we're allowed to see the inventory of the owner
			if (ClientCanKnowInventoryOf(client, owner) == FALSE)
			{
				return FALSE;
			}
		}
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ClientCanKnowInventoryOf(
	GAMECLIENT * client,
	UNIT * unit,
	BOOL bIgnoreClientPlayerState /*= FALSE*/)
{
	ASSERT_RETFALSE(client);
	ASSERT_RETFALSE(unit);
		
	// everybody knows about the inventory of items (mods, recipe components etc) in the world
	if (UnitIsA(unit, UNITTYPE_ITEM) && UnitGetRoom(unit) != NULL)
	{
		return TRUE;
	}

	// everybody knows about the contents of decoy units
	if (UnitIsDecoy(unit))
	{
		return TRUE;
	}
			
	// check for client interactions
	UNIT * owner = UnitGetUltimateOwner(unit);	
	UNIT * control = ClientGetControlUnit(client, bIgnoreClientPlayerState);
	if (control)
	{
		// clients know about the complete inventory of things they own
		if (control == owner)
		{
			return TRUE;
		}
		
		// for now, we know the entire contents of shared stores and players when trading with them
		if (TradeIsTradingWith(control, unit))
		{
			if (UnitIsMerchant(unit))
			{
				// only know about inventory of merchants with shared inventories 
				// (as opposed to personal merchant stores)
				if (UnitMerchantSharesInventory(unit))
				{
					return TRUE;
				}
			}
			else if (UnitIsTradesman(unit))
			{
				// only know about inventory of tradesman with shared inventories 
				// (as opposed to personal tradesman stores)
				if (UnitTradesmanSharesInventory(unit))
				{
					return TRUE;
				}
			}
			else
			{
				// trading with a non merchant, know about all inventory (for now)
				return TRUE;
			}
		}
	}
	
	// check player interactions
	UNIT * player = ClientGetPlayerUnit(client, bIgnoreClientPlayerState);
	if (player)
	{
		// clients know about the complete inventory of things they own
		if (player == owner)
		{
			return TRUE;
		}
	}
	
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void ClientInitKnown(
	GAME * game,
	GAMECLIENT * client)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	GAMECLIENT_KNOWN_DATA & known = client->m_Known;

	structclear(known);	
	known.m_hashUnitId = (UNIT_KNOWN **)GMALLOCZ(game, sizeof(UNIT_KNOWN *) * KNOWN_HASH_SIZE);
	known.m_hashIndex = (UNIT_KNOWN **)GMALLOCZ(game, sizeof(UNIT_KNOWN *) * KNOWN_HASH_SIZE);
#else
	REF(game);
	REF(client);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void ClientFreeKnown(
	GAME * game,
	GAMECLIENT * client)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	GAMECLIENT_KNOWN_DATA & known = client->m_Known;

	if (known.m_hashUnitId)
	{
		for (unsigned int ii = 0; ii < KNOWN_HASH_SIZE; ++ii)
		{
			UNIT_KNOWN * next = known.m_hashUnitId[ii];
			UNIT_KNOWN * curr = NULL;
			while ((curr = next) != NULL)
			{
				next = curr->nextInUnitIdHash;
				GFREE(game, curr);
			}
		}
		GFREE(game, known.m_hashUnitId);
	}

	if (known.m_hashIndex)
	{
		GFREE(game, known.m_hashIndex);
	}

	UNIT_KNOWN * next = known.m_Garbage;
	UNIT_KNOWN * curr = NULL;
	while ((curr = next) != NULL)
	{
		next = curr->nextInIndexHash;
		GFREE(game, curr);
	}
	structclear(known);
#else
	REF(game);
	REF(client);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UNIT_KNOWN * ClientKnownFindByIndex(
	GAMECLIENT * client,
	unsigned int index)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	GAMECLIENT_KNOWN_DATA & known = client->m_Known;

	unsigned int key = index % KNOWN_HASH_SIZE;

	UNIT_KNOWN * next = known.m_hashIndex[key];
	UNIT_KNOWN * curr = NULL;
	while ((curr = next) != NULL)
	{
		next = curr->nextInIndexHash;
		if (curr->index == index)
		{
			return curr;
		}
	}
#else
	REF(client);
	REF(index);
#endif
	return NULL;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ClientAddKnownUnit(
	GAMECLIENT * client,
	UNIT * unit,
	DWORD dwUnitKnownFlags /*= 0*/)
{
	ASSERT_RETFALSE(client);
	ASSERT_RETFALSE(unit);

#if !ISVERSION(CLIENT_ONLY_VERSION)
	GAME * game = UnitGetGame(unit);
	ASSERT_RETFALSE(game);

	GAMECLIENT_KNOWN_DATA & known = client->m_Known;

	UNITID idUnit = UnitGetId(unit);
	unsigned int key = idUnit % KNOWN_HASH_SIZE;
	
	// return false if the unit is in the idUnit hash
	UNIT_KNOWN * node = ClientKnownFindByUnitId(client, idUnit);
	if (node)
	{
		return FALSE;
	}

	// get free node & index from garbage
	node = known.m_Garbage;
	if (!node)
	{
		node = (UNIT_KNOWN *)GMALLOCZ(game, sizeof(UNIT_KNOWN));
		node->index = known.m_Count++;
	}
	else
	{
		known.m_Garbage = node->nextInIndexHash;
		node->nextInIndexHash = NULL;
	}

	node->idUnit = idUnit;
	
	// save flags
	node->dwUnitKnownFlags = dwUnitKnownFlags;

	// add to unitid hash
	node->nextInUnitIdHash = known.m_hashUnitId[key];
	known.m_hashUnitId[key] = node;

	// add to index hash
	key = node->index % KNOWN_HASH_SIZE;
	node->nextInIndexHash = known.m_hashIndex[key];
	known.m_hashIndex[key] = node;

	TRACE_KNOWN("ClientAddKnownUnit():  CLIENT [%s]  UNIT [%d: %s]\n", ClientGetName(client), UnitGetId(unit), UnitGetDevName(unit));
#endif

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNITID ClientKnownIndexToUnitId(
	GAMECLIENT * client,
	unsigned int index)
{
	ASSERT_RETINVALID(client);
#if !ISVERSION(CLIENT_ONLY_VERSION)
	UNIT_KNOWN * known = ClientKnownFindByIndex(client, index);
	if (!known)
	{
		return (UNITID)INVALID_ID;
	}
	return known->idUnit;
#else
	REF(index);
	return (UNITID)INVALID_ID;
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
unsigned int ClientKnownUnitIdToIndex(
	GAMECLIENT * client,
	UNITID idUnit)
{
	ASSERT_RETINVALID(client);
#if !ISVERSION(CLIENT_ONLY_VERSION)
	UNIT_KNOWN * known = ClientKnownFindByUnitId(client, idUnit);
	if (!known)
	{
		return (UNITID)INVALID_ID;
	}
	return known->index;
#else
	REF(idUnit);
	return INVALID_ID;
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL UnitIsKnownByClient(
	GAMECLIENT * client,
	UNIT * unit)
{
	ASSERT_RETFALSE(client);
	ASSERT_RETFALSE(unit);

#if !ISVERSION(CLIENT_ONLY_VERSION)
	return (ClientKnownUnitIdToIndex(client, UnitGetId(unit)) != INVALID_INDEX);
#else
	return FALSE;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL ClientKnownRemoveUnitInventory(
	GAMECLIENT * client,
	UNIT * unit)
{
#if ISVERSION(CLIENT_ONLY_VERSION)
	REF(client);
	REF(unit);
#else
	UNIT * contained = NULL;
	while ((contained = UnitInventoryIterate(unit, contained)) != NULL)
	{
		// TRAVIS: if we remove client knowledge of pets, hirelings, and other noncontained
		// units, then on free, they will incorrectly be deemed unknown on other clients,
		// and thus their free will not be sent to those clients.
		if (UnitIsKnownByClient(client, contained) &&
			UnitIsPhysicallyInContainer( contained ) )
		{
			ClientRemoveKnownUnit(client, contained);
		}
	}
#endif
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ClientRemoveKnownUnit(
	GAMECLIENT * client,
	UNIT * unit)
{
	ASSERT_RETFALSE(client);
	ASSERT_RETFALSE(unit);

#if !ISVERSION(CLIENT_ONLY_VERSION)
	GAME * game = UnitGetGame(unit);
	ASSERT_RETFALSE(game);

	GAMECLIENT_KNOWN_DATA & known = client->m_Known;

	UNITID idUnit = UnitGetId(unit);
	unsigned int index = INVALID_ID;
	unsigned int key = idUnit % KNOWN_HASH_SIZE;
	
	UNIT_KNOWN * prev = NULL;
	UNIT_KNOWN * next = known.m_hashUnitId[key];
	UNIT_KNOWN * curr = NULL;
	while ((curr = next) != NULL)
	{
		next = curr->nextInUnitIdHash;
		if (curr->idUnit == idUnit)
		{
			if (prev)
			{
				prev->nextInUnitIdHash = curr->nextInUnitIdHash;
			}
			else
			{
				known.m_hashUnitId[key] = curr->nextInUnitIdHash;
			}
			index = curr->index;
			curr->idUnit = INVALID_ID;
			goto remove_from_index_hash;
		}
		prev = curr;
	}
	return FALSE;

remove_from_index_hash:
	ASSERT_RETFALSE(index != INVALID_ID);

	key = index % KNOWN_HASH_SIZE;

	prev = NULL;
	next = known.m_hashIndex[key];
	curr = NULL;
	while ((curr = next) != NULL)
	{
		next = curr->nextInIndexHash;
		if (curr->index == index)
		{
			if (prev)
			{
				prev->nextInIndexHash = curr->nextInIndexHash;
			}
			else
			{
				known.m_hashIndex[key] = curr->nextInIndexHash;
			}

			curr->nextInIndexHash = known.m_Garbage;
			known.m_Garbage = curr;

			TRACE_KNOWN("ClientRemoveKnownUnit():  CLIENT [%s]  UNIT [%d: %s]\n", ClientGetName(client), UnitGetId(unit), UnitGetDevName(unit));

			ClientKnownRemoveUnitInventory(client, unit);
			return TRUE;
		}
		prev = curr;
	}
	ASSERT_RETFALSE(0);
#else
	return TRUE;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ClientSetKnownFlag( 
	GAMECLIENT * client,
	UNIT * unit,
	UNIT_KNOWN_FLAG eFlag,
	BOOL bValue)
{
	ASSERTX_RETURN( client, "Expected client" );
	ASSERTX_RETURN( unit, "Expected unit" );
	UNITID idUnit = UnitGetId( unit );
	UNIT_KNOWN *pKnown = ClientKnownFindByUnitId( client, idUnit );
	if (pKnown)
	{
		SETBIT( pKnown->dwUnitKnownFlags, eFlag, bValue );
	}
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ClientTestKnownFlag(
	GAMECLIENT * client,
	UNIT * unit,
	UNIT_KNOWN_FLAG eFlag)
{
	ASSERTX_RETFALSE( client, "Expected client" );
	ASSERTX_RETFALSE( unit, "Expected unit" );
	UNITID idUnit = UnitGetId( unit );
	UNIT_KNOWN *pKnown = ClientKnownFindByUnitId( client, idUnit );
	if (pKnown)
	{
		return TESTBIT( pKnown->dwUnitKnownFlags, eFlag );
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ClientRemoveCachedUnits(
	GAMECLIENT * client,
	UNIT_KNOWN_FLAG eFlagToMatch)
{
	ASSERTX_RETURN( client, "Expected client" );
	
	// get known system
	GAMECLIENT_KNOWN_DATA &tKnownData = client->m_Known;
	ASSERTX_RETURN( tKnownData.m_hashUnitId, "Expected unitid hash for known data" );
	GAME *pGame = client->m_pGame;
	ASSERTX_RETURN( pGame, "Client has no game" );
	
	// go though the hash
	for (int i = 0; i < KNOWN_HASH_SIZE; ++i)
	{
		UNIT_KNOWN *pKnown = tKnownData.m_hashUnitId[ i ];
		while (pKnown)
		{
		
			// does this entry match the flags we're looking at
			if (TESTBIT( pKnown->dwUnitKnownFlags, eFlagToMatch ))
			{

				// keep next pointer
				UNIT_KNOWN *pNext = pKnown->nextInUnitIdHash;

				// clear the known bit we're matching so we can successfully remove the unit now
				CLEARBIT( pKnown->dwUnitKnownFlags, eFlagToMatch );
								
				// remove this unit from the client
				UNIT *pUnit = UnitGetById( pGame, pKnown->idUnit );
				if (pUnit)
				{
					s_RemoveUnitFromClient( pUnit, client, UFF_SEND_TO_CLIENTS );
				}
				
				// move to next entry
				pKnown = pNext;
				
			}
			else
			{
				pKnown = pKnown->nextInUnitIdHash;
			}
			
		}
		
	}
	
}

//----------------------------------------------------------------------------
// Super-simple implementation: 
// nuke the entire known structure and start fresh
//----------------------------------------------------------------------------
BOOL ClientRemoveAllKnownUnits(
	GAME * game,
	GAMECLIENT * client)
{
	ClientFreeKnown(game, client);
	ClientInitKnown(game, client);
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void s_ClientSetGuildAssociation(
	GAMECLIENT * client,
	const WCHAR * guildName,
	BYTE guildRank,
	const WCHAR * rankName )
{
	ASSERT_RETURN( client && guildName );

	PStrCopy(client->m_wszGuildName, guildName, MAX_GUILD_NAME);
	client->m_eGuildRank = guildRank;
	PStrCopy(client->m_guildRankName, rankName, MAX_CHARACTER_NAME);
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL s_ClientGetGuildAssociation(
	GAMECLIENT * client,
	WCHAR * guildName,
	DWORD guildNameLen,
	BYTE & guildRank,
	WCHAR * rankName,
	DWORD rankNameLen )
{
	ASSERT_RETFALSE( client && guildName && rankName );

	PStrCopy(guildName, guildNameLen, client->m_wszGuildName, MAX_GUILD_NAME);
	guildRank = client->m_eGuildRank;
	PStrCopy(rankName, rankNameLen, client->m_guildRankName, MAX_CHARACTER_NAME);

	return TRUE;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL s_ClientIsSendMailMessageAllowed(
	GAMECLIENT * client )
{
	ASSERT_RETFALSE(client);

	//	can't send if they're a trial user
	if (ClientHasBadge(client, ACCT_TITLE_TRIAL_USER))
		return FALSE;
	
	// check gagged status
	UNIT *pPlayer = ClientGetControlUnit( client );
	if (pPlayer)
	{
		if (s_GagIsGagged( pPlayer ))
		{
			return FALSE;		
		}
	}
		
	return client->m_emailSendLimitTokenBucket.isAllowed(1);
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL s_ClientIsGeneralEmailActionAllowed(
	GAMECLIENT * client )
{
	ASSERT_RETFALSE(client);
	return client->m_emailGeneralActionTokenBucket.isAllowed(1);
}
#endif
