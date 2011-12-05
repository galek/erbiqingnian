//----------------------------------------------------------------------------
// gameclient.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifdef	_GAMECLIENT_H_
//#pragma message ("    HINT: " __FILE__ " included multiple times in this translation unit")
#else
#define _GAMECLIENT_H_


//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#ifndef _PRIME_H_
#include "prime.h"
#endif

#ifndef _MESSAGE_TRACKER_H_
#include "message_tracker.h"
#endif

#ifndef _ACCOUNTBADGES_H_
#include "accountbadges.h"
#endif

#if !ISVERSION(CLIENT_ONLY_VERSION)
#include "TokenBucket.h"
#endif


//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
#define									MAX_MSGBUF_TRANSACTIONS					4		// maximum number of nested transactions for GAMECLIENT_MESSAGE_BUFFER
#define									KNOWN_HASH_SIZE							128


//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------
enum GAME_CLIENT_STATE
{
	GAME_CLIENT_STATE_FREE,
	GAME_CLIENT_STATE_DISCONNECTING,
	GAME_CLIENT_STATE_SWITCHINGINSTANCE,
	GAME_CLIENT_STATE_CONNECTED,		// added to game, but unit is not in a room yet
	GAME_CLIENT_STATE_INGAME,
};


//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// aggregate message buffer + aggregate transaction buffer
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
struct GAMECLIENT_MESSAGE_BUFFER
{
	GAMECLIENT_MESSAGE_BUFFER *			m_pNext;								// pointer to next buffer in singly-linked list
	BYTE								m_ByteBuf[SMALL_MSG_BUF_SIZE];			// aggregate message buffer
	unsigned int						m_ByteBufCur;							// end of buffer
};
#endif


//----------------------------------------------------------------------------
// cursor to start of transaction
// TRANSACTION: allows us to roll back messages
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
struct GAMECLIENT_TRANSACT_NODE
{
	unsigned int						m_BufferIndex;							// index in GAMECLIENT_MESSAGE_BUFFER list where transaction began
	unsigned int						m_BufferOffset;							// offset in the buffer where transaction began
};
#endif


//----------------------------------------------------------------------------
// each game client has one of these to store consolidated message buffers,
// compression info, etc. for network
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
struct GAMECLIENT_NETWORK_DATA
{
	GAMECLIENT_MESSAGE_BUFFER *			m_pHead;								// first aggregate message buffer in singly linked list
	GAMECLIENT_MESSAGE_BUFFER *			m_pTail;								// last aggregate message buffer in singly linked list
	GAME_TICK							m_tiTransactionTick;					// game tick for transactions (transactions aren't allowed to span ticks)
	GAMECLIENT_TRANSACT_NODE			m_Transactions[MAX_MSGBUF_TRANSACTIONS];// array of transaction cursors
	unsigned int						m_TransactionIndex;						// length of m_Transactions
	unsigned int						m_MsgBufCount;							// # of items in message_buffer list
};
#endif

//----------------------------------------------------------------------------
enum UNIT_KNOWN_FLAG
{
	UKF_INVALID = -1,
	
	UKF_CACHED,				// this is a cached item, we will not remove this item from this client until this flag is cleared somehow
	UKF_ALLOW_CLOSED_STASH,	// send an item in a stash inventory location even if the stash is not currently open
	UKF_ALLOW_INSPECTION,	// send an item that is needed upon a player inspection.
	
	UFK_NUM_FLAGS			// keep this last please
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct UNIT_KNOWN
{
	UNITID								idUnit;
	unsigned int						index;
	UNIT_KNOWN *						nextInIndexHash;
	UNIT_KNOWN *						nextInUnitIdHash;
	DWORD								dwUnitKnownFlags;  // see UNIT_KNOWN_FLAG
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct GAMECLIENT_KNOWN_DATA
{
	// rooms known
	DWORD								m_pdwRoomKnownFlags[DWORD_FLAG_SIZE(MAX_ROOMS_PER_LEVEL)];

	// units known: assume a client knows about 500 units (300 items and 200 monsters)
	UNIT_KNOWN **						m_hashUnitId;
	UNIT_KNOWN **						m_hashIndex;
	UNIT_KNOWN *						m_Garbage;
	unsigned int						m_Count;
};

//----------------------------------------------------------------------------
inline unsigned int GetMaxRoomKnownFlagIndex(
	void)
{
	return MAX_ROOMS_PER_LEVEL;
}

//----------------------------------------------------------------------------
enum GAMECLIENT_FLAG
{
	GCF_CANNOT_RESERVE_GAME,			// this client cannot reserve a game
};

//----------------------------------------------------------------------------
// client data stored by the GAME
//----------------------------------------------------------------------------
struct GAMECLIENT
{
	GAMECLIENTID						m_idGameClient;
	APPCLIENTID							m_idAppClient;
	NETCLIENTID64						m_idNetClient;
	struct GAME *						m_pGame;
	char								m_szName[MAX_CHARACTER_NAME];
	
	struct UNIT	*						m_pPlayerUnit;					// this is the unit we save/load
	struct UNIT	*						m_pControlUnit;					// we could be currently controlling a different unit

	GAME_CLIENT_STATE					m_eGameClientState;

	GAMECLIENT *						m_pPrev;						// prev & next in game
	GAMECLIENT *						m_pNext;

	struct SREWARD_SESSION *			m_pRewardSession;
	struct STRADE_SESSION *				m_pTradeSession;

	class rjdMovementTracker *			m_pMovementTracker;
	class MessageTracker				m_tCMessageTracker;
	class MessageTracker				m_tSMessageTracker;

	//Badges should never be changed; only created on client creation.
	PROTECTED_BADGE_COLLECTION *		m_pBadges;

	int									m_nMissedPingCount;
	VECTOR								m_vPositionLastReported;		// position client reports in reqmove

	GAMECLIENT_KNOWN_DATA				m_Known;

#if !ISVERSION(CLIENT_ONLY_VERSION)
	GAMECLIENT_NETWORK_DATA				m_NetData;

	WCHAR								m_wszGuildName[MAX_GUILD_NAME];
	BYTE								m_eGuildRank;
	WCHAR								m_guildRankName[MAX_CHARACTER_NAME];

	TokenBucket							m_emailSendLimitTokenBucket;
	TokenBucket							m_emailGeneralActionTokenBucket;

	GAMECLIENT() : m_emailSendLimitTokenBucket(1,10), m_emailGeneralActionTokenBucket(10, 5) {};
#endif

	DWORD								m_dwGameClientFlags;
};


//----------------------------------------------------------------------------
// EXPORTED FUNCTIONS
//----------------------------------------------------------------------------
BOOL ClientListInit(
	struct GAME * game,
	unsigned int max_players = DEFAULT_MAX_PLAYERS_PER_GAME);

BOOL InitClientMesssageTracking(
	void);

void ClientListFree(
	struct GAME * game);

int ClientListGetClientCount(
	struct GAME_CLIENT_LIST * client_list);

void ClientListExitClients(
	GAME* game);

GAMECLIENT * ClientAddToGame(
	GAME * game,
	APPCLIENTID idAppClient);

void GameRemoveClient(
	GAME * game,
	GAMECLIENT * client);

#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL ClientRemoveFromGameNow(
	GAME * game,
	GAMECLIENT * client,
	BOOL bSaveSend);
#endif

#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL ClientRemoveFromGame(
	struct GAME * game,
	GAMECLIENT * client,
	DWORD msecDelay);
#endif

GAMECLIENT * ClientGetById(
	struct GAME * game,
	GAMECLIENTID idClient);

NETCLIENTID64 ClientGetNetId(
	struct GAME * game,
	GAMECLIENTID idClient);

NETCLIENTID64 ClientGetNetId(
	GAMECLIENT * client);

BOOL ClientValidateIdConsistency(
	GAME * game,
	GAMECLIENTID idGameClient);

int GameGetClientCount(
	struct GAME * game);

int GameGetInGameClientCount(
	struct GAME * game);

GAMECLIENT * GameGetNextClient(
	struct GAME * game,
	GAMECLIENT * client);

void ClientSetPlayerUnit(
	struct GAME * game,
	GAMECLIENT * client,
	struct UNIT * unit);

void ClientSetControlUnit(
	struct GAME * game,
	GAMECLIENT * client,
	struct UNIT * unit);

BOOL ClientGetName(
	GAMECLIENT * client,
	WCHAR * wszName,
	unsigned int len);

const char * ClientGetName(
	GAMECLIENT * client);

GAMECLIENT * ClientGetFirstInGame(
	GAME * game);

GAMECLIENT * ClientGetNextInGame( 
	GAMECLIENT * client);

GAMECLIENT * ClientGetFirstInLevel(
	struct LEVEL * level);

GAMECLIENT * ClientGetNextInLevel(
	GAMECLIENT * client,
	struct LEVEL * level);

GAMECLIENTID ClientGetId(
	GAMECLIENT * client);

BOOL SendMessageToClient(
	GAME * game,
	GAMECLIENT * client,
	NET_CMD command,
	MSG_STRUCT * msgstruct);

BOOL SendMessageToClient(
	GAME * game,
	GAMECLIENTID idClient,
	NET_CMD command,
	struct MSG_STRUCT * msgstruct);

void SendClientMessages(
	GAME * game);

struct ROOM * ClientGetFirstRoomKnown(
	GAMECLIENT * client,
	UNIT * unit);

struct ROOM * ClientGetNextRoomKnown(
	GAMECLIENT * client,
	UNIT * unit,
	struct ROOM * room);

void ClientSetRoomKnown(
	GAMECLIENT * client,
	struct ROOM * room,
	BOOL bKnown);

BOOL ClientIsRoomKnown(
	GAMECLIENT * client,
	struct ROOM * room,
	BOOL bIgnoreClientPlayerState = FALSE);
	
void ClientClearAllRoomsKnown(
	GAMECLIENT * client,
	struct LEVEL * level);

// client game doesn't have any GAMECLIENTs
BOOL c_ClientAlwaysKnowsUnitInWorld( 
	UNIT * unit);

BOOL s_ClientAlwaysKnowsUnitInWorld( 
	GAMECLIENT* pClient,
	UNIT * unit);

BOOL UnitIsCachedOnClient(
	GAMECLIENT * client,
	UNIT * unit);

BOOL ClientCanKnowUnit( 
	GAMECLIENT * client, 
	UNIT * unit,
	BOOL bIgnoreClientPlayerState = FALSE,
	BOOL bAllowStashWhenClosed = FALSE);

BOOL ClientCanKnowInventoryOf(	
	GAMECLIENT * client, 
	UNIT * unit,
	BOOL bIgnoreClientPlayerState = FALSE);

BOOL ClientAddKnownUnit(
	GAMECLIENT * client,
	UNIT * unit,
	DWORD dwUnitKnownFlags = 0);		// see UNIT_KNOWN_FLAG

BOOL ClientRemoveKnownUnit(
	GAMECLIENT * client,
	UNIT * unit);

void ClientSetKnownFlag( 
	GAMECLIENT * client,
	UNIT * unit,
	UNIT_KNOWN_FLAG eFlag,
	BOOL bValue);

BOOL ClientTestKnownFlag(
	GAMECLIENT * client,
	UNIT * unit,
	UNIT_KNOWN_FLAG eFlag);
	
void ClientRemoveCachedUnits(
	GAMECLIENT * client,
	UNIT_KNOWN_FLAG eFlagToMatch);
	
BOOL UnitIsKnownByClient(
	GAMECLIENT * client,
	UNIT * unit);

UNITID ClientKnownIndexToUnitId(
	GAMECLIENT * client,
	unsigned int index);

unsigned int ClientKnownUnitIdToIndex(
	GAMECLIENT * client,
	UNITID idUnit);

BOOL ClientRemoveAllKnownUnits(
	GAME * game,
	GAMECLIENT * client);

#if !ISVERSION(CLIENT_ONLY_VERSION)

void s_ClientSetGuildAssociation(
	GAMECLIENT * client,
	const WCHAR * guildName,
	BYTE guildRank,
	const WCHAR * rankName );

BOOL s_ClientGetGuildAssociation(
	GAMECLIENT * client,
	WCHAR * guildName,
	DWORD guildNameLen,
	BYTE & guildRank,
	WCHAR * rankName,
	DWORD rankNameLen );

BOOL s_ClientIsSendMailMessageAllowed(
	GAMECLIENT * client );

BOOL s_ClientIsGeneralEmailActionAllowed(
	GAMECLIENT * client );
	
#endif	//	!ISVERSION(CLIENT_ONLY_VERSION

//----------------------------------------------------------------------------
// INLINE FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline GAME_CLIENT_STATE ClientGetState(
	GAMECLIENT * client)
{
	ASSERT(client);
	if (!client)
	{
		return GAME_CLIENT_STATE_FREE;
	}
	return client->m_eGameClientState;
}

//----------------------------------------------------------------------------
inline void ClientSetState(
	GAMECLIENT * client,
	GAME_CLIENT_STATE eState)
{
	ASSERT_RETURN(client);
	client->m_eGameClientState = eState;
}

//----------------------------------------------------------------------------
inline UNIT * ClientGetControlUnit(
	GAMECLIENT * client,
	BOOL bIgnoreState = FALSE)
{
	ASSERT_RETNULL(client);
	if (bIgnoreState == TRUE || ClientGetState(client) == GAME_CLIENT_STATE_INGAME)
	{
		return client->m_pControlUnit;	
	}
	return NULL;	
}

//----------------------------------------------------------------------------
inline UNIT * ClientGetPlayerUnit(
	GAMECLIENT * client,
	BOOL bIgnoreState = FALSE)
{
	ASSERT_RETNULL(client);
	if (bIgnoreState == TRUE || ClientGetState(client) == GAME_CLIENT_STATE_INGAME)
	{
		return client->m_pPlayerUnit;	
	}
	return NULL;			
}

//----------------------------------------------------------------------------
inline UNIT * ClientGetPlayerUnitForced(
	GAMECLIENT * client)
{
	ASSERT_RETNULL(client);
	return client->m_pPlayerUnit;
}

//----------------------------------------------------------------------------
inline NETCLIENTID64 ClientGetNetId(
	GAMECLIENT * client)
{
	ASSERT_RETINVALID(client);
	return client->m_idNetClient;
}

//----------------------------------------------------------------------------
inline APPCLIENTID ClientGetAppId(
	GAMECLIENT * client)
{
	ASSERT_RETINVALID(client);
	return client->m_idAppClient;
}

//----------------------------------------------------------------------------
inline void GameClientSetFlag(
	GAMECLIENT *client,
	GAMECLIENT_FLAG eFlag,
	BOOL bValue)
{
	ASSERTV_RETURN( client, "Expected client" );
	SETBIT( client->m_dwGameClientFlags, eFlag, bValue );
}

//----------------------------------------------------------------------------
inline BOOL GameClientTestFlag(
	GAMECLIENT *client,
	GAMECLIENT_FLAG eFlag)
{
	ASSERTV_RETFALSE( client, "Expected client" );
	return TESTBIT( client->m_dwGameClientFlags, eFlag );
}

BOOL ClientGetBadges(
	GAMECLIENT *client,
	BADGE_COLLECTION &badges);

#endif


