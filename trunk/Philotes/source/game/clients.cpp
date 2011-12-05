//----------------------------------------------------------------------------
// clients.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
// contains:
// CLIENTSYSTEM - a app or thread level client tracking system
// CLIENT - a game level client structure
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "dbhellgate.h"
#include "prime.h"
#include "clients.h"
#include "units.h" // includes game.h
#include "player.h"
#include "s_message.h"
#include "gamelist.h"
#include "tasks.h"
#include "crc.h"
#include "quests.h"
#include "c_message.h"
#include "s_network.h"
#include "pets.h"
#include "items.h"
#include "trade.h"
#include "console.h"
#include "chat.h"
#include "unittag.h"
#include "unit_priv.h"
#include "player.h"
#include "s_quests.h"
#include "s_reward.h"
#include "s_townportal.h"
#include "s_trade.h"
#include "netclient.h"
#include "rjdmovementtracker.h"
#include "bookmarks.h"
#include "unitfile.h"
#include "dbunit.h"
#include "accountbadges.h"
#include "stash.h"
#include "svrstd.h"
#include "GameServer.h"
#include "Quest.h"
#include "config.h"
#include "ServerSuite/BillingProxy/BillingMsgs.h"
#include "objects.h"
#if ISVERSION(SERVER_VERSION)
#include "clients.cpp.tmh"
#include <Watchdogclient_counter_definitions.h>
#include <PerfHelper.h>

#include "ServerSuite/GameLoadBalance/GameLoadBalanceGameServerRequestMsg.h"
#include "s_loadBalanceNetwork.h"
#include "guidcounters.h"
#include "ehm.h"
#endif 


//----------------------------------------------------------------------------
// APP - LEVEL CLIENT STRUCTURES
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// ENUMERATIONS
//----------------------------------------------------------------------------
enum CLIENT_STATE
{
	CLIENT_STATE_NONE,
	CLIENT_STATE_CONNECTED,
	CLIENT_STATE_WAITINGFORSAVEFILE,
	CLIENT_STATE_WAITINGTOENTERGAME,
	CLIENT_STATE_WAITINGTOSWITCHINSTANCE,
	CLIENT_STATE_INGAME,
};


// ---------------------------------------------------------------------------
// MACROS
// ---------------------------------------------------------------------------
#define APPCLIENTID_GET_INDEX(id)		((id) & 0xffff)						// get the array index from the client id
#define APPCLIENTID_GET_ITER(id)		((id) >> 16)						// get the iter from the client id
#define MAKE_APPCLIENTID(idx, itr)		(((itr) << 16) + ((idx) & 0xffff))	// construct client id from index + iter

#define MAX_UNIT_COLLECTION (16)		// arbitrary

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUnitCollectionInit(
	struct UNIT_COLLECTION *pCollection, 
	BOOL bFreeCollectionData);

//----------------------------------------------------------------------------
struct UNIT_COLLECTION
{

	PLAYER_LOAD_TYPE				eLoadType;
	UNIT_COLLECTION_ID				idCollection;
	PGUID							guidUnitTreeRoot;
	PGUID							guidDestPlayer;
	DWORD							dwSaveCRC;
	DWORD							dwSaveSize;
	DWORD							dwSaveCur;
	BYTE *							pSaveFile;
	PLAYER_SAVE_FORMAT				eFormat;
	
	UNIT_COLLECTION::UNIT_COLLECTION(
		void)
	{	
		sUnitCollectionInit( this, FALSE );
	}	
	
};

//----------------------------------------------------------------------------
// CLIENT stores clients on the app-level
// CLIENTSYSTEM contains all clients, it uses a pre-allocated store/garbage
//   list to avoid extra allocations
//----------------------------------------------------------------------------
struct APPCLIENT
{
	CCritSectLite					m_cs;
	CLIENT_STATE					m_eState;
	APPCLIENTID						m_idClient;
	NETCLIENTID64					m_idNetClient;
	UNIQUE_ACCOUNT_ID				m_idAccount;
	GAMECLIENTID					m_idGameClient;
	GAMEID							m_idGame;
	LEVEL_GOING_TO					m_tLevelGoingTo;
	CHARACTER_INFO					m_tCharacterInfo;
	PGUID							m_guid;

	//Badges should never be changed; only created on client connection.
	PROTECTED_BADGE_COLLECTION *	m_pBadges;

	time_t							m_LoginTime;
	BOOL							m_bSubjectToFatigue;
	int								m_iFatigueOnlineSecs; // initial fatigue online time when session began
	time_t							m_GagUntilTime;			// time an account is gagged until

	// the reserved game is the dungeon instance the client has reserved even
	// if they're not in it, so they can go back to whatever they were doing
	// each client gets to reserve one game (the last dungeon game they were
	// in).  when the client disconnects or grabs another reserved game, we
	// need to send a message to the reserved game to unreserve it.
	// each game generates a unique reserve key which the client (on the game
	// system not the the actual client!) must provide in order to do stuff
	// w/ the reserved game
	GAMEID							m_idReservedGame;
	int								m_idReservedArea;
	DWORD							m_dwReserveKey;

	volatile long *					m_pGameToJoinPendingPlayerCount;
	BOOL							m_bGameToJoinPendingPlayerCount;
	volatile long *					m_pCurrentGamePlayerCount;
	BOOL							m_bCurrentGamePlayerCount;

	// the warp game is stored state data from when we switch servers,
	// so that the client can go to the correct game and instance when they
	// finish loading.
	GAMEID							m_idServerWarpGame;
	WARP_SPEC						m_tWarpSpec;

	// party data
	PARTYID							m_idParty;
	PARTYID							m_idInvite;			// which party i'm currently invited to
	GAMEID							m_idGameReserved;	// what game (if any) the invite party is for
	APPCLIENTID						m_idInvitor;

	UNIT_COLLECTION 				m_tUnitCollectionCharacter;	// unit collection for character loads			
	SIMPLE_DYNAMIC_ARRAY<UNIT_COLLECTION> m_tUnitCollections; // unit collections used for single item tree loads (like email attachments) or account unit loads (shared stash)
	
	APPCLIENT *						m_pNext;
};


struct APPCLIENT_REF
{
	PGUID			id;
	APPCLIENT_REF *	next;
	LONG			appClientIndex;
};

typedef HASH<APPCLIENT_REF,PGUID,MAX_CLIENTS> APPCLIENTHASH;

//----------------------------------------------------------------------------
enum SYSTEM_APP_CLIENT
{	
	SAC_INVALID = -1,
	
	SAC_AUCTION,				// app client for the auction system
	SAC_ITEM_RESORE,			// app client for the item restore system
	
	SAC_NUM_SYSTEMS				// keep this last
};

//----------------------------------------------------------------------------
struct CLIENTSYSTEM
{
	APPCLIENT						m_Clients[MAX_CLIENTS];
	APPCLIENTHASH					m_ClientsByGuid;
	CReaderWriterLock_NS_FAIR		m_ClientsByGuidLock;

	LONG							m_nCurrIter;

	CCritSectLite					m_csFreeList;
	APPCLIENT *						m_pFreeList;

	APPCLIENT						m_SystemAppClients[ SAC_NUM_SYSTEMS ];
	
};


//----------------------------------------------------------------------------
// APP-LEVEL CLIENT FUNCTIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ClientSaveFileInit(
	CLIENT_SAVE_FILE &tSaveFile)
{
	tSaveFile.pBuffer = NULL;
	tSaveFile.dwBufferSize = 0;
	tSaveFile.eFormat = PSF_HIERARCHY;
	tSaveFile.guidUnitRoot = INVALID_GUID;
	tSaveFile.guidDestPlayer = INVALID_GUID;
	tSaveFile.ePlayerLoadType = PLT_INVALID;
	tSaveFile.uszCharName[ 0 ] = 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUnitCollectionInit( 
	UNIT_COLLECTION *pCollection,
	BOOL bFreeCollectionData)
{
	ASSERTX_RETURN( pCollection, "Expected collection" );

	// free any data associated with this collection
	if (bFreeCollectionData && pCollection->pSaveFile)
	{

		if (pCollection->eFormat == PSF_DATABASE)
		{
			DATABASE_UNIT_COLLECTION *pDBUnitsToFree = (DATABASE_UNIT_COLLECTION *)pCollection->pSaveFile;
			s_DatabaseUnitCollectionFree( pDBUnitsToFree );
		}
		else
		{
			FREE( NULL, pCollection->pSaveFile );
		}
		
	}

	// init the collection members
	pCollection->eLoadType = PLT_INVALID;
	pCollection->idCollection = INVALID_ID;
	pCollection->guidDestPlayer = INVALID_GUID;
	pCollection->guidUnitTreeRoot = INVALID_GUID;
	pCollection->dwSaveCRC = 0;
	pCollection->dwSaveSize = 0;
	pCollection->dwSaveCur = 0;
	pCollection->pSaveFile = NULL;
	pCollection->eFormat = PSF_INVALID;
			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT_COLLECTION *sClientFindUnitCollection(
	APPCLIENT *pAppClient,
	UNIT_COLLECTION_ID idCollection,
	BOOL bAllocateNewIfNotFound)
{
	ASSERTX_RETNULL( pAppClient, "Expected app client" );
	ASSERTX_RETNULL( idCollection != INVALID_ID, "Invalid collection id" );
	
	// search the collection for the id in question
	UNIT_COLLECTION *pFirstUnusedCollection = NULL;
	for (UINT32 i = 0; i < pAppClient->m_tUnitCollections.Count(); ++i)
	{
		UNIT_COLLECTION *pCollection = &pAppClient->m_tUnitCollections[ i ];
		if (pCollection->idCollection == idCollection)
		{
			return pCollection;
		}

		// keep track of the first unused collection
		if (pFirstUnusedCollection == NULL && pCollection->idCollection == INVALID_ID)
		{
			pFirstUnusedCollection = pCollection;
		}
	}
		
	// if not found, can create a new one
	if (bAllocateNewIfNotFound == TRUE)
	{
		if (pFirstUnusedCollection != NULL) 
		{
			pFirstUnusedCollection->idCollection = idCollection;
			return pFirstUnusedCollection;
		} 
		else 
		{
			int index = ArrayAddItem(pAppClient->m_tUnitCollections, UNIT_COLLECTION());
			UNIT_COLLECTION& unitCollection = pAppClient->m_tUnitCollections[index];
			unitCollection.idCollection = idCollection;
			return &unitCollection;
		}
	}
	
	return NULL;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UNIT_COLLECTION *sClientGetUnitCollection(
	APPCLIENT *pAppClient, 
	UNIT_COLLECTION_ID idCollection, 
	UNIT_COLLECTION_TYPE eCollectionType,
	BOOL bCreateIfNotFound)
{
	ASSERTX_RETNULL( pAppClient, "Expected app client" );
	ASSERTX_RETNULL( idCollection != INVALID_ID || eCollectionType == UCT_CHARACTER, "Expected collection id" );
	
	if (eCollectionType == UCT_CHARACTER)
	{
		
		// return the single unit collection we use for character loads on this client
		return &pAppClient->m_tUnitCollectionCharacter;
		
	}
	else
	{
		return sClientFindUnitCollection( pAppClient, idCollection, bCreateIfNotFound );
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sFreeClientBadges(
	APPCLIENT * client)
{
	ASSERT_RETURN(client);
	
	if (client->m_pBadges)
	{
		FREE(NULL, client->m_pBadges);
		client->m_pBadges = NULL;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void ClientSystemClearClient(
	CLIENTSYSTEM * client_system,
	APPCLIENT * client,
	unsigned int index)
{
	{
		RWFairWriteAutoLock writeLock = &client_system->m_ClientsByGuidLock;
		client_system->m_ClientsByGuid.Remove( client->m_guid );
	}
	ASSERT(index == (unsigned int)(client - client_system->m_Clients));

	// note there is no need to do this, we are already locked	-cday
	// CSLAutoLock tCSAutoLock( &client->m_cs );
	
	// free the data from the character unit collection
	sUnitCollectionInit( &client->m_tUnitCollectionCharacter, TRUE );

	// clear any unit collections we were loading (single item trees like email attachments or shared stash loading)
	for (UINT32 i = 0; i < client->m_tUnitCollections.Count(); ++i)
	{
		UNIT_COLLECTION *pCollection = &client->m_tUnitCollections[ i ];		
		sUnitCollectionInit( pCollection, TRUE );
	}
					
	sFreeClientBadges(client);
	client->m_eState = CLIENT_STATE_NONE;
	client->m_idClient = MAKE_APPCLIENTID(index, InterlockedIncrement(&client_system->m_nCurrIter));
	client->m_idNetClient = INVALID_NETCLIENTID64;
	client->m_idAccount = INVALID_UNIQUE_ACCOUNT_ID;
	client->m_idGameClient = INVALID_GAMECLIENTID;
	client->m_idGame = INVALID_ID;
	client->m_tLevelGoingTo.idGame = INVALID_ID;
	LevelTypeInit( client->m_tLevelGoingTo.tLevelType );
	client->m_tCharacterInfo.szCharName[0] = 0;
	client->m_guid = INVALID_GUID;
	client->m_LoginTime = 0;
	client->m_bSubjectToFatigue = false;
	client->m_iFatigueOnlineSecs = 0;
	client->m_GagUntilTime = 0;
	client->m_idReservedGame = INVALID_ID;
	client->m_idReservedArea = INVALID_ID;
	client->m_dwReserveKey = 0;
	client->m_pGameToJoinPendingPlayerCount = NULL;
	client->m_bGameToJoinPendingPlayerCount = FALSE;
	client->m_pCurrentGamePlayerCount = NULL;
	client->m_bCurrentGamePlayerCount = FALSE;
	client->m_idServerWarpGame = INVALID_ID;
	client->m_idParty = INVALID_ID;
	client->m_idInvite = INVALID_ID;
	client->m_idGameReserved = INVALID_ID;
	client->m_idInvitor = (APPCLIENTID)INVALID_CLIENTID;
	ArrayClear(client->m_tUnitCollections);

	client->m_pNext = NULL;
}

//----------------------------------------------------------------------------
struct SYSTEM_APP_CLIENT_LOOKUP
{
	SYSTEM_APP_CLIENT eSystem ;
	APPCLIENTID idAppClient;	
};
static const SYSTEM_APP_CLIENT_LOOKUP sgtSystemAppClientLookup[] = 
{
	 { SAC_AUCTION,		APPCLIENTID_AUCTION_SYSTEM },
	 { SAC_ITEM_RESORE,	APPCLIENTID_ITEM_RESTORE_SYSTEM },
	 // add more system lookups here!
};
static const int sgnNumSystemAppClientsLookup = arrsize( sgtSystemAppClientLookup );

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
CLIENTSYSTEM * ClientSystemInit(
	void)
{
	CLIENTSYSTEM * client_system = (CLIENTSYSTEM *)MALLOCZ(NULL, sizeof(CLIENTSYSTEM));
	ASSERT_RETNULL(client_system);

	client_system->m_ClientsByGuid.Init();
	client_system->m_ClientsByGuidLock.Init();

	APPCLIENT * client = client_system->m_Clients;
	for (unsigned int ii = 0; ii < MAX_CLIENTS; ii++, client++)
	{
		ClientSystemClearClient(client_system, client, ii);
		if (ii < MAX_CLIENTS - 1)
		{
			client_system->m_Clients[ii].m_pNext = client_system->m_Clients + ii + 1;
		}
	}
	client_system->m_pFreeList = client_system->m_Clients;
	InterlockedIncrement(&client_system->m_nCurrIter);

	InitClientMesssageTracking();

	// initialize the app client services
	for (int i = 0; i < sgnNumSystemAppClientsLookup; ++i)
	{
	
		// get init data for this service
		const SYSTEM_APP_CLIENT_LOOKUP *pLookup = &sgtSystemAppClientLookup[ i ];
		ASSERTX_CONTINUE( 
			pLookup->eSystem > SAC_INVALID && pLookup->eSystem < SAC_NUM_SYSTEMS, 
			"Invalid app client system" );
		
		// initialize the client system for this service
		APPCLIENT *pAppClient = &client_system->m_SystemAppClients[ pLookup->eSystem ];		
		pAppClient->m_idNetClient = INVALID_NETCLIENTID64;
		pAppClient->m_idAccount = INVALID_UNIQUE_ACCOUNT_ID;
		pAppClient->m_idClient = pLookup->idAppClient;
		pAppClient->m_cs.Init();
		ArrayInit( pAppClient->m_tUnitCollections, NULL, MAX_UNIT_COLLECTION );
		
	}
	
	return client_system;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ClientSystemFree(
	CLIENTSYSTEM * client_system)
{
	ASSERT_RETURN(client_system);
	client_system->m_ClientsByGuidLock.Free();
	client_system->m_ClientsByGuid.Destroy(NULL);
	
	// free each of the system clients
	for (int i = 0; i < SAC_NUM_SYSTEMS; ++i)
	{
		APPCLIENT *pAppClient = &client_system->m_SystemAppClients[ i ];
		pAppClient->m_cs.Free();
		ArrayClear(pAppClient->m_tUnitCollections);
	}
	FREE(NULL, client_system);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static APPCLIENT * ClientSystemGetClientFromFreeList(
	CLIENTSYSTEM * client_system)
{
	CSLAutoLock autolock(&client_system->m_csFreeList);
	ASSERT_RETNULL(client_system->m_pFreeList);

	APPCLIENT * client = client_system->m_pFreeList;
	client_system->m_pFreeList = client->m_pNext;
	return client;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
APPCLIENTID ClientSystemNewClient(
	CLIENTSYSTEM * client_system,
	NETCLIENTID64 idNetClient,
	UNIQUE_ACCOUNT_ID idAccount,
	const BADGE_COLLECTION &tBadges,
	time_t LoginTime,
	BOOL bSubjectToFatigue,
	int iFatigueOnlineSecs,
	time_t GagUntilTime,
	MEMORYPOOL* pPool)
{
	ASSERT_RETVAL(client_system, (APPCLIENTID)INVALID_CLIENTID);
	ASSERT_RETVAL(idNetClient != INVALID_NETCLIENTID64, (APPCLIENTID)INVALID_CLIENTID);

	APPCLIENT * client = ClientSystemGetClientFromFreeList(client_system);
	if (!client)
	{
		return (APPCLIENTID)INVALID_CLIENTID;
	}

	ASSERT(client->m_eState == CLIENT_STATE_NONE);
	client->m_eState = CLIENT_STATE_CONNECTED;
	client->m_idNetClient = idNetClient;
	client->m_idAccount = idAccount;
	ASSERT(client->m_pBadges == NULL);
	client->m_pBadges = (PROTECTED_BADGE_COLLECTION *)MALLOCZ(NULL, sizeof(PROTECTED_BADGE_COLLECTION));
	new (client->m_pBadges) PROTECTED_BADGE_COLLECTION(tBadges);
	client->m_LoginTime = LoginTime;
	client->m_bSubjectToFatigue = bSubjectToFatigue;
	client->m_iFatigueOnlineSecs = iFatigueOnlineSecs;
	client->m_GagUntilTime = GagUntilTime;
	ArrayInit(client->m_tUnitCollections, pPool, MAX_UNIT_COLLECTION);

	return client->m_idClient;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static SYSTEM_APP_CLIENT sGetSystemAppClientByClientID( 
	APPCLIENTID idAppClient)
{
	ASSERTX_RETVAL( idAppClient != INVALID_CLIENTID, SAC_INVALID, "Invalid app client id" );
	
	// check the lookup table
	for (int i = 0; i < sgnNumSystemAppClientsLookup; ++i)
	{
		const SYSTEM_APP_CLIENT_LOOKUP *pLookup = &sgtSystemAppClientLookup[ i ];
		if (pLookup->idAppClient == idAppClient)
		{
			return pLookup->eSystem;
		}
	}
	
	return SAC_INVALID;  // not found
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static APPCLIENT * ClientSystemGetClientById(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idClient)
{

	// no client
	if (idClient == INVALID_CLIENTID)
	{
		return NULL;
	}
	
	// check for "system" app clients
	SYSTEM_APP_CLIENT eSystem = sGetSystemAppClientByClientID( idClient );
	if (eSystem != SAC_INVALID && eSystem < SAC_NUM_SYSTEMS)
	{
		return &client_system->m_SystemAppClients[ eSystem ];
	}

	// a real appclient
	unsigned int index = APPCLIENTID_GET_INDEX(idClient);
	ASSERT_RETNULL(index >= 0 && index < MAX_CLIENTS);
	return client_system->m_Clients + index;
	
}


//----------------------------------------------------------------------------
// TODO: this is totally brute force right now, need to implement a name index
//----------------------------------------------------------------------------
APPCLIENTID ClientSystemFindByName(
	CLIENTSYSTEM * client_system,
	const WCHAR * wszName)
{
	ASSERT_RETVAL(client_system, (APPCLIENTID)INVALID_CLIENTID);
	
	APPCLIENT * client = client_system->m_Clients;
	for (unsigned int ii = 0; ii < MAX_CLIENTS; ii++, client++)
	{
		CSLAutoLock autolock(&client->m_cs);
		if (client->m_eState != CLIENT_STATE_INGAME)
		{
			continue;
		}
		if (client->m_tCharacterInfo.szCharName && PStrICmp(wszName, client->m_tCharacterInfo.szCharName, MAX_CHARACTER_NAME) == 0)
		{
			return client->m_idClient;
		}		
	}
	
	return (APPCLIENTID)INVALID_CLIENTID;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
APPCLIENTID ClientSystemFindByPguid(
	CLIENTSYSTEM * client_system,
	PGUID clientGuid)
{
	ASSERT_RETVAL(client_system, INVALID_CLIENTID);
	APPCLIENT_REF * clientRef = NULL;
	{
		RWFairReadAutoLock readLock = &client_system->m_ClientsByGuidLock;
		clientRef = client_system->m_ClientsByGuid.Get(clientGuid);
	}
	if (clientRef)
	{
		ASSERT_RETX(clientRef->appClientIndex < MAX_CLIENTS,INVALID_CLIENTID);
		return client_system->m_Clients[clientRef->appClientIndex].m_idClient;
	}
	return INVALID_CLIENTID;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void ClientSystemRecycleClient(
	CLIENTSYSTEM * client_system,
	APPCLIENT * client)
{
	CSLAutoLock(&client_system->m_csFreeList);
	client->m_pNext = client_system->m_pFreeList;
	client_system->m_pFreeList = client;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void ClientSystemPostUnreserveGameMessage(
	NETCLIENTID64 idNetClient,
	PGUID guid,
	GAMEID idReservedGame,
	DWORD dwReserveKey)
{
	if (idReservedGame == INVALID_ID)
	{
		return;
	}
	ASSERT_RETURN(idNetClient != INVALID_NETCLIENTID64);

	MSG_CCMD_UNRESERVEGAME msg;
	msg.dwReserveKey = dwReserveKey;
	msg.guid = guid;

	SrvPostMessageToMailbox(idReservedGame, idNetClient, CCMD_UNRESERVEGAME, &msg);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAppClientClearGamePlayerCounters(
	APPCLIENT * client)
{
	ASSERT_RETURN(client)
	if (client->m_pGameToJoinPendingPlayerCount && client->m_bGameToJoinPendingPlayerCount)
	{
		InterlockedDecrement(client->m_pGameToJoinPendingPlayerCount);
	}
	client->m_pGameToJoinPendingPlayerCount = NULL;
	client->m_bGameToJoinPendingPlayerCount = FALSE;

	if (client->m_pCurrentGamePlayerCount && client->m_bCurrentGamePlayerCount)
	{
		InterlockedDecrement(client->m_pCurrentGamePlayerCount);
	}
	client->m_pCurrentGamePlayerCount = NULL;
	client->m_bCurrentGamePlayerCount = FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ClientSystemRemoveClient(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idClient)
{
	ASSERT_RETURN(client_system);
	APPCLIENT * client = ClientSystemGetClientById(client_system, idClient);
	ASSERT_RETURN(client);

	NETCLIENTID64 idNetClient = INVALID_NETCLIENTID64;
	PGUID guid = INVALID_GUID;
	GAMEID idReservedGame = INVALID_ID;
	DWORD dwReserveKey = 0;

	{
		CSLAutoLock(&client->m_cs);
		if (client->m_idClient != idClient)
		{
			return;
		}
		ASSERT_RETURN(client->m_eState != CLIENT_STATE_NONE);
			
		idReservedGame = client->m_idReservedGame;
		dwReserveKey = client->m_dwReserveKey;
		idNetClient = client->m_idNetClient;
		guid = client->m_guid;

		sAppClientClearGamePlayerCounters(client);

		ClientSystemClearClient(client_system, client, APPCLIENTID_GET_INDEX(client->m_idClient));
	}

	ClientSystemRecycleClient(client_system, client);
	
	ClientSystemPostUnreserveGameMessage(idNetClient, guid, idReservedGame, dwReserveKey);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ClientSaveFilePostProcess( 
	CLIENTSYSTEM *pClientSystem,
	APPCLIENTID idAppClient,
	UNIT_COLLECTION_TYPE eCollectionType,
	UNIT_COLLECTION_ID idCollection)
{
	ASSERTX_RETFALSE( pClientSystem, "Expected client system" );
	ASSERTX_RETFALSE( idAppClient != INVALID_CLIENTID, "Invalid client id" );
	ASSERTX_RETFALSE( eCollectionType > UCT_INVALID && eCollectionType < UCT_NUM_TYPES, "Invlaid collection type" );
	
	// get the file buffer
	CLIENT_SAVE_FILE tClientSaveFile;
	ClientSystemGetClientSaveFile( pClientSystem, idAppClient, eCollectionType, idCollection, &tClientSaveFile );

	// for player files from the database, reorganize the units into the regular save format
	if (tClientSaveFile.eFormat == PSF_DATABASE)
	{		
	
		DATABASE_UNIT_COLLECTION *pDBUCollection = (DATABASE_UNIT_COLLECTION *)tClientSaveFile.pBuffer;
		if (s_DatabaseUnitsPostProcess( 
				pDBUCollection, 
				tClientSaveFile.ePlayerLoadType, 
				eCollectionType, 
				tClientSaveFile.guidUnitRoot ) == FALSE)
		{
			return FALSE;
		}
		
		// store the unit collection in the save file pointer for this client
		APPCLIENT *pAppClient = ClientSystemGetClientById( pClientSystem, idAppClient );
		ASSERTX_RETFALSE( pAppClient, "Expected client" );		
		pAppClient->m_cs.Enter();
		UNIT_COLLECTION *pCollection = sClientGetUnitCollection( pAppClient, idCollection, eCollectionType, FALSE );
		if (pCollection)
		{
			if (eCollectionType == UCT_CHARACTER)
			{
				ASSERT( pAppClient->m_eState == CLIENT_STATE_WAITINGTOENTERGAME );
			}
			ASSERT( pCollection->eFormat == PSF_DATABASE );
			pCollection->dwSaveCRC = 0;  // hmm, is this necessary? -cday
		}
		pAppClient->m_cs.Leave();		

	}
	else
	{

		// get unit file
		UNIT_FILE tUnitFile;
		ClientSaveDataGetRootUnitBuffer( &tClientSaveFile, &tUnitFile );
	
		// what CRC do we check
		DWORD dwCRC = 0;
		APPCLIENT *pAppClient = ClientSystemGetClientById( pClientSystem, idAppClient );
		pAppClient->m_cs.Enter();
		UNIT_COLLECTION *pCollection = sClientGetUnitCollection( pAppClient, idCollection, eCollectionType, FALSE );
		ASSERT( pCollection );
		if (pCollection)
		{
			dwCRC = pCollection->dwSaveCRC;
		}
		pAppClient->m_cs.Leave();
		
		// check the CRC
		ASSERTX_RETFALSE( 
			UnitFileCheckCRC( dwCRC, tUnitFile.pBuffer, tUnitFile.dwBufferSize ),
			"Re-assembled unit file does not pass CRC check!!" );
		
		
	}

	return TRUE;
				
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ClientSystemSetClientSaveFile(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idClient,
	PGUID guidDestPlayer,
	PLAYER_SAVE_FORMAT eFormat,	
	PGUID guidUnitTreeRoot,
	const WCHAR * name,
	UNIT_COLLECTION_TYPE eCollectionType,
	UNIT_COLLECTION_ID idCollection,
	PLAYER_LOAD_TYPE eLoadType,
	int nDifficultySelected,
	DWORD save_crc,
	DWORD save_size)
{
	ASSERT_RETFALSE(client_system);
	ASSERT_RETFALSE(name && name[0] != 0);
	ASSERT_RETFALSE(eCollectionType > UCT_INVALID && eCollectionType < UCT_NUM_TYPES);
	ASSERT_RETFALSE(eLoadType > PLT_INVALID && eLoadType < PLT_NUM_TYPES);
	
	// unit tree roots are required when loading item trees
	if (eLoadType == PLT_ITEM_TREE ||
		eLoadType == PLT_AH_ITEMSALE_SEND_TO_AH)
	{
		ASSERTX_RETFALSE(
			guidUnitTreeRoot != INVALID_GUID, 
			"Root unit tree GUID required for when loading item trees" );
	}
	
	APPCLIENT * client = ClientSystemGetClientById(client_system, idClient);
	ASSERT_RETFALSE(client);
	DWORD dwMinDataSize = UNITFILE_MIN_SIZE_ALL_UNITS;
	DWORD dwMaxDataSize = MAX( (DWORD)sizeof( DATABASE_UNIT_COLLECTION ), (DWORD)UNITFILE_MAX_SIZE_ALL_UNITS );
	ASSERT_RETFALSE(save_size >= dwMinDataSize && save_size <= dwMaxDataSize );

	BOOL result = FALSE;

	client->m_cs.Enter();
	UNIT_COLLECTION *pCollection = NULL;
	ONCE
	{
		if (client->m_idClient != idClient)
		{
			break;
		}

		// when loading a full character, we put the client into a state where they are
		// waiting for the save file
		if (eCollectionType == UCT_CHARACTER)
		{
			ASSERT_BREAK(client->m_eState == CLIENT_STATE_CONNECTED);
			client->m_eState = CLIENT_STATE_WAITINGFORSAVEFILE;
		}
		
		CHARACTER_INFO &tCharacterInfo = client->m_tCharacterInfo;
		PStrCopy(tCharacterInfo.szCharName, name, MAX_CHARACTER_NAME);
		
		pCollection = sClientGetUnitCollection( client, idCollection, eCollectionType, TRUE );
		if (pCollection == NULL)
		{
			break;
		}
		pCollection->dwSaveCRC = save_crc;
		pCollection->dwSaveSize = save_size;
		pCollection->eFormat = eFormat;
		pCollection->guidUnitTreeRoot = guidUnitTreeRoot;
		pCollection->guidDestPlayer = guidDestPlayer;
		pCollection->eLoadType = eLoadType;
		
		// there must be a destination player when loading an item tree
		if (eCollectionType == UCT_ITEM_COLLECTION)
		{
			ASSERTX_BREAK( guidDestPlayer != INVALID_GUID, "A destination player guid is required when load item collection trees" );
		}

		// save the difficulty this client wants to play with this character
		// NOTE: The only reason we're checking for invalid_link here is because
		// the code path for open server and closed server is slightly different.
		// until we can verify that closed server should set the selected difficulty
		// through this code path we won't stomp over any value that might already
		// be set in the client
		if (nDifficultySelected != INVALID_LINK)
		{
			tCharacterInfo.nDifficulty = nDifficultySelected;
		}
		ASSERTX( tCharacterInfo.nDifficulty != INVALID_LINK, "Expected difficulty selected in character info for client" );
		
		result = TRUE;
	}
	
	if (result == FALSE && pCollection)
	{
		sUnitCollectionInit( pCollection, TRUE );
	}
	
	client->m_cs.Leave();

	return result;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ClientSystemAddClientSaveFileChunk(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idClient,
	UNIT_COLLECTION_TYPE eCollectionType,
	UNIT_COLLECTION_ID idCollection,
	const BYTE * buf,
	unsigned int size,
	BOOL & bDone,
	PLAYER_SAVE_FORMAT & eFormat)
{
	bDone = FALSE;
	ASSERT_RETFALSE(client_system);
	APPCLIENT * client = ClientSystemGetClientById(client_system, idClient);
	ASSERT_RETFALSE(client);
	ASSERT_RETFALSE(size <= MAX_PLAYERSAVE_BUFFER);
	ASSERT_RETFALSE(eCollectionType > UCT_INVALID && eCollectionType < UCT_NUM_TYPES);

	BOOL result = FALSE;

	client->m_cs.Enter();
	UNIT_COLLECTION *pCollection = NULL;
	ONCE
	{
		if (client->m_idClient != idClient)
		{
			break;
		}
		
		// if we're receiving a full character, the client better be in a state where
		// it is waiting for the same file
		if (eCollectionType == UCT_CHARACTER)
		{
			ASSERT_BREAK(client->m_eState == CLIENT_STATE_WAITINGFORSAVEFILE);
		}
		
		pCollection = sClientGetUnitCollection( client, idCollection, eCollectionType, FALSE );
		if (pCollection == NULL)
		{
			break;
		}
		if (pCollection->dwSaveCur == 0)
		{
			ASSERT_BREAK(!pCollection->pSaveFile);
			pCollection->pSaveFile = (BYTE *)MALLOC(NULL, pCollection->dwSaveSize);
		}
		ASSERT_BREAK(pCollection->pSaveFile);
		if (pCollection->dwSaveSize - pCollection->dwSaveCur > MAX_PLAYERSAVE_BUFFER)
		{
			ASSERT_BREAK(size == MAX_PLAYERSAVE_BUFFER);
		}
		else
		{
			ASSERT_BREAK(pCollection->dwSaveCur + size == pCollection->dwSaveSize);
		}
		memcpy(pCollection->pSaveFile + pCollection->dwSaveCur, buf, size);
		pCollection->dwSaveCur += size;

		if (pCollection->dwSaveCur < pCollection->dwSaveSize)
		{
			result = TRUE;
			break;
		}

		// when done loading a character unit collection, this client will be waiting to
		// enter the game and use this character
		if (eCollectionType == UCT_CHARACTER)
		{
			client->m_eState = CLIENT_STATE_WAITINGTOENTERGAME;
		}
		
		bDone = TRUE;
		eFormat = pCollection->eFormat;
		result = TRUE;
	}
	
	// if error, free this collection
	if (result == FALSE && pCollection)
	{
		sUnitCollectionInit( pCollection, TRUE );
	}
	
	client->m_cs.Leave();

	return result;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ClientSaveDataGetRootUnitBuffer(
	const CLIENT_SAVE_FILE *pClientSaveFile,
	UNIT_FILE *pUnitFile)
{
	ASSERTX_RETFALSE( pClientSaveFile, "Expected client save file" );
	ASSERTX_RETFALSE( pUnitFile, "Expected unit buffer" );
	
	// init return result
	pUnitFile->pBuffer = NULL;
	pUnitFile->dwBufferSize = 0;

	// check for possible formats
	switch (pClientSaveFile->eFormat)
	{
	
		//----------------------------------------------------------------------------
		case PSF_HIERARCHY:
		{
			pUnitFile->pBuffer = pClientSaveFile->pBuffer;
			pUnitFile->dwBufferSize = pClientSaveFile->dwBufferSize;
			break;		
		}
		
		//----------------------------------------------------------------------------
		case PSF_DATABASE:
		{
			DATABASE_UNIT_COLLECTION *pDBUCollection = (DATABASE_UNIT_COLLECTION *)pClientSaveFile->pBuffer;
			DATABASE_UNIT *pDBUnit = s_DatabaseUnitGetRoot( pDBUCollection );
			pUnitFile->pBuffer = s_DBUnitGetDataBuffer( pDBUCollection, pDBUnit );
			pUnitFile->dwBufferSize = pDBUnit->dwDataSize;
			break;
		}
		
		//----------------------------------------------------------------------------
		default:
		{
			ASSERTX_RETFALSE( 0, "Unknown client save file format" );
		}
		
	}
		
	return TRUE;
	
}	


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ClientSystemGetClientSaveFile(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idClient,
	UNIT_COLLECTION_TYPE eCollectionType,
	UNIT_COLLECTION_ID idCollection,
	CLIENT_SAVE_FILE *pClientSaveFile)
{
	ASSERTX_RETFALSE(client_system, "Expected client system" );
	APPCLIENT * client = ClientSystemGetClientById(client_system, idClient);
	ASSERTX_RETFALSE(client, "Unknown client" );
	ASSERTX_RETFALSE( pClientSaveFile, "Expected client save file struct" );
	ASSERTX_RETFALSE(eCollectionType > UCT_INVALID && eCollectionType < UCT_NUM_TYPES, "Invalid collection type");

	BOOL result = FALSE;

	client->m_cs.Enter();
	ONCE
	{
		if (client->m_idClient != idClient)
		{
			break;
		}
		if (client->m_eState != CLIENT_STATE_WAITINGTOENTERGAME && eCollectionType == UCT_CHARACTER)
		{
			break;
		}
		
		UNIT_COLLECTION *pCollection = sClientGetUnitCollection( client, idCollection, eCollectionType, FALSE );
		if (!pCollection->pSaveFile || pCollection->dwSaveCur != pCollection->dwSaveSize)
		{
			break;
		}
		
		PStrCopy( pClientSaveFile->uszCharName, client->m_tCharacterInfo.szCharName, MAX_CHARACTER_NAME);	
		pClientSaveFile->pBuffer = pCollection->pSaveFile;
		pClientSaveFile->dwBufferSize = pCollection->dwSaveSize;
		pClientSaveFile->eFormat = pCollection->eFormat;
		pClientSaveFile->guidUnitRoot = pCollection->guidUnitTreeRoot;
		pClientSaveFile->guidDestPlayer = pCollection->guidDestPlayer;
		pClientSaveFile->ePlayerLoadType = pCollection->eLoadType;
		result = TRUE;
		
	}
	client->m_cs.Leave();

	return result;
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ClientSystemRecycleClientSaveFile(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idAppClient,
	UNIT_COLLECTION_TYPE eCollectionType,
	UNIT_COLLECTION_ID idCollection)
{
	ASSERTX_RETFALSE(client_system, "Expected client system" );
	APPCLIENT * client = ClientSystemGetClientById(client_system, idAppClient);
	ASSERTX_RETFALSE(client, "Unknown client" );
	ASSERTX_RETFALSE(eCollectionType == UCT_ITEM_COLLECTION, "You can only recycle item collections, not characters");
	ASSERTX_RETFALSE(idCollection != INVALID_ID, "Expected unit collection id" );

	BOOL result = FALSE;

	client->m_cs.Enter();
	ONCE
	{
		if (client->m_idClient != idAppClient)
		{
			break;
		}		

		UNIT_COLLECTION *pCollection = sClientGetUnitCollection( client, idCollection, eCollectionType, FALSE );
		if (pCollection)
		{
			sUnitCollectionInit( pCollection, TRUE );
		}
		
	}
	client->m_cs.Leave();

	return result;
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ClientSystemFreeCharacterSaveFile(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idClient,
	BOOL bForce)
{
	ASSERT_RETURN(client_system);
	APPCLIENT * client = ClientSystemGetClientById(client_system, idClient);
	ASSERT_RETURN(client);
	
	client->m_cs.Enter();
	ONCE
	{
		if (client->m_idClient != idClient)
		{
			ASSERT_MSG("AppClient has wrong ID in ClientSystemFreeCharacterSaveFile()!");
			break;
		}
		if (client->m_eState != CLIENT_STATE_WAITINGTOENTERGAME && bForce == FALSE)
		{
			break;
		}
		
		// recycle the collection used for the character loading
		sUnitCollectionInit( &client->m_tUnitCollectionCharacter, TRUE );
		
	}
	client->m_cs.Leave();
					
}


//----------------------------------------------------------------------------
// save a client file to the appclient when changing instances on the same
// server
//----------------------------------------------------------------------------
BOOL ClientSystemSetClientInstanceSaveBuffer(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idClient,
	const WCHAR * name,
	BYTE * data,
	DWORD save_size,
	DWORD key)
{
	ASSERT_RETFALSE(client_system);
	ASSERT_RETFALSE(name && name[0] != 0);
	APPCLIENT * client = ClientSystemGetClientById(client_system, idClient);
	ASSERT_RETFALSE(client);
	ASSERT_RETFALSE(save_size >= UNITFILE_MIN_SIZE_ALL_UNITS && save_size <= UNITFILE_MAX_SIZE_ALL_UNITS);

	BOOL result = FALSE;

	// free any previous data for entire character files
	ClientSystemFreeCharacterSaveFile( client_system, idClient, TRUE );

	client->m_cs.Enter();
	UNIT_COLLECTION *pCollection = NULL;
	ONCE
	{
		if (client->m_idClient != idClient)
		{
			break;
		}
		ASSERT_BREAK(client->m_eState == CLIENT_STATE_INGAME);

		client->m_eState = CLIENT_STATE_WAITINGTOSWITCHINSTANCE;
		PStrCopy(client->m_tCharacterInfo.szCharName, name, MAX_CHARACTER_NAME);
		
		pCollection = sClientGetUnitCollection( client, INVALID_ID, UCT_CHARACTER, FALSE );
		if (pCollection == NULL)
		{
			break;
		}
		pCollection->dwSaveCRC = key;
		pCollection->dwSaveSize = save_size;
		pCollection->eFormat = PSF_HIERARCHY;
		ASSERT_BREAK(pCollection->pSaveFile == NULL);
		pCollection->pSaveFile = (BYTE *)MALLOC(NULL, save_size);
		memcpy(pCollection->pSaveFile, data, save_size); 

		result = TRUE;
	}
	
	if (result == FALSE && pCollection)
	{
		sUnitCollectionInit( pCollection, TRUE );
	}
	
	client->m_cs.Leave();

	return result;
}

	
//----------------------------------------------------------------------------
// get data stored for instancing
//----------------------------------------------------------------------------
BOOL ClientSystemGetClientInstanceSaveBuffer(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idClient,
	CLIENT_SAVE_FILE *pClientSaveFile,
	DWORD * key)
{
	ASSERTX_RETFALSE( client_system, "Expected client system" );
	APPCLIENT * client = ClientSystemGetClientById(client_system, idClient);
	ASSERTX_RETFALSE( client, "Unknown client" );
	ASSERTX_RETFALSE( pClientSaveFile, "Expected save file storage" );
	ASSERTX_RETFALSE( key, "Expected key storage" );

	BOOL result = FALSE;

	client->m_cs.Enter();
	ONCE
	{
		if (client->m_idClient != idClient)
		{
			ASSERT_MSG("AppClient has wrong ID in ClientSystemGetClientInstanceSaveBuffer()!");
			break;
		}
		if (client->m_eState != CLIENT_STATE_WAITINGTOSWITCHINSTANCE)
		{
			break;
		}
		
		UNIT_COLLECTION *pCollection = sClientGetUnitCollection( client, INVALID_ID, UCT_CHARACTER, FALSE );
		if (pCollection == NULL || !pCollection->pSaveFile || pCollection->dwSaveSize <= 0)
		{
			break;
		}
		
		// copy over data to output param
		PStrCopy(pClientSaveFile->uszCharName, client->m_tCharacterInfo.szCharName, MAX_CHARACTER_NAME);
		pClientSaveFile->pBuffer = pCollection->pSaveFile;
		pClientSaveFile->dwBufferSize = pCollection->dwSaveSize;
		pClientSaveFile->eFormat = pCollection->eFormat;
		*key = pCollection->dwSaveCRC;
		
		result = TRUE;
	}
	client->m_cs.Leave();

	return result;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ClientSystemSetGuid(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idAppClient,
	PGUID guid)
{
	ASSERT_RETFALSE(client_system);
	APPCLIENT * client = ClientSystemGetClientById(client_system, idAppClient);
	ASSERT_RETFALSE(client);

	BOOL result = FALSE;
	ONCE
	{
		CSLAutoLock clientLock = &client->m_cs;
		if (client->m_idClient != idAppClient)
		{
			break;
		}
		if (client->m_guid != INVALID_GUID)
		{
			break;
		}
		client->m_guid = guid;
		{
			RWFairWriteAutoLock writeLock = &client_system->m_ClientsByGuidLock;
			APPCLIENT_REF * guidRef = client_system->m_ClientsByGuid.Add(NULL,guid);
			ASSERT_BREAK(guidRef);
			guidRef->appClientIndex = (LONG)(client - client_system->m_Clients);
		}
		result = TRUE;
	}

	return result;	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PGUID ClientSystemGetGuid(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idAppClient)
{
	ASSERT_RETFALSE(client_system);
	APPCLIENT * client = ClientSystemGetClientById(client_system, idAppClient);
	ASSERT_RETFALSE(client);

	PGUID guid = INVALID_GUID;
	
	client->m_cs.Enter();
	ONCE
	{
		if (client->m_idClient != idAppClient)
		{
			break;
		}
		guid = client->m_guid;
	}
	client->m_cs.Leave();

	return guid;	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ClientSystemGetName(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idAppClient,
	WCHAR * wszName,
	unsigned int len)
{
	ASSERT_RETFALSE(client_system);
	APPCLIENT * client = ClientSystemGetClientById(client_system, idAppClient);
	ASSERT_RETFALSE(client);

	BOOL result = FALSE;

	client->m_cs.Enter();
	ONCE
	{
		if (client->m_idClient != idAppClient)
		{
			break;
		}
		PStrCopy(wszName, client->m_tCharacterInfo.szCharName, len);
		result = TRUE;
	}
	client->m_cs.Leave();

	return result;	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ClientSystemSetCharacterInfo(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idAppClient,
	const CHARACTER_INFO &tCharacterInfo)
{
	ASSERT_RETFALSE(client_system);
	APPCLIENT * client = ClientSystemGetClientById(client_system, idAppClient);
	ASSERT_RETFALSE(client);

	BOOL result = FALSE;

	client->m_cs.Enter();
	ONCE
	{
		if (client->m_idClient != idAppClient)
		{
			break;
		}
		client->m_tCharacterInfo = tCharacterInfo;
		result = TRUE;
	}
	client->m_cs.Leave();

	return result;	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ClientSystemGetCharacterInfo(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idAppClient,
	CHARACTER_INFO &tCharacterInfo)
{
	ASSERT_RETFALSE(client_system);
	APPCLIENT * client = ClientSystemGetClientById(client_system, idAppClient);
	ASSERT_RETFALSE(client);

	BOOL result = FALSE;

	client->m_cs.Enter();
	ONCE
	{
		if (client->m_idClient != idAppClient)
		{
			break;
		}
		tCharacterInfo = client->m_tCharacterInfo;
		result = TRUE;
	}
	client->m_cs.Leave();

	return result;	
}

#if ISVERSION(SERVER_VERSION)
struct LogoutDBFinishContext
{
	MSG_GAMELOADBALANCE_GAMESERVER_REQUEST_CLIENT_FINISHED_DATABASE_TRANSACTIONS
		msg;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sPostLogoutDBFinishMessage(
	void * context)
{
	ASSERT_RETURN(context);
	LogoutDBFinishContext * pContext =
		(LogoutDBFinishContext *) context;

	GameServerToGameLoadBalanceServerSendMessage(
		&pContext->msg,
		GAMELOADBALANCE_GAMESERVER_REQUEST_CLIENT_FINISHED_DATABASE_TRANSACTIONS);
	FREE(NULL, pContext);
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ClientSystemSetGaggedUntilTime(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idAppClient,
	time_t tTimeGaggedUntil)
{
	ASSERTX_RETURN( client_system, "Expected client system" );
	APPCLIENT * pAppClient = ClientSystemGetClientById( client_system, idAppClient );
	ASSERTX_RETURN( pAppClient, "Expected app client" );	
	pAppClient->m_GagUntilTime = tTimeGaggedUntil;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
time_t ClientSystemGetGaggedUntilTime(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idAppClient)
{
	ASSERTX_RETFALSE( client_system, "Expected client system" );
	APPCLIENT * pAppClient = ClientSystemGetClientById( client_system, idAppClient );
	ASSERTX_RETFALSE( pAppClient, "Expected app client" );	
	return pAppClient->m_GagUntilTime;
}

//----------------------------------------------------------------------------
// Server only: using information from the APPCLIENT, we set up the necessary
// information to inform the server we're done logging out and saving.
//----------------------------------------------------------------------------
BOOL ClientSystemSetLogoutSaveCallback(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idAppClient)
{
#if ISVERSION(SERVER_VERSION)
	ASSERT_RETFALSE(client_system);
	APPCLIENT * client = ClientSystemGetClientById(client_system, idAppClient);
	ASSERT_RETFALSE(client);

	LogoutDBFinishContext *pContext = (LogoutDBFinishContext *)MALLOCZ(NULL, sizeof(LogoutDBFinishContext));
	ASSERT_RETFALSE(pContext);

	BOOL result = FALSE;
	PGUID guidPlayer = INVALID_GUID;

	client->m_cs.Enter();
	ONCE
	{
		if (client->m_idClient != idAppClient)
		{
			break;
		}
		guidPlayer = client->m_guid;
		//We don't set the idNet, as by now they're disconnected anyway.
		pContext->msg.accountId = client->m_idAccount;
		PStrCopy(pContext->msg.szCharName, client->m_tCharacterInfo.szCharName, MAX_CHARACTER_NAME);

		result = TRUE;
	}
	client->m_cs.Leave();

	BOOL bRet = TRUE;

	if(result)
	{
		GameServerContext *pServerContext =
			(GameServerContext *)CurrentSvrGetContext();
		CBRA(pServerContext);
		CBRA(pServerContext->m_DatabaseCounter);

		pServerContext->m_DatabaseCounter->
			SetCallbackOnZero(guidPlayer, pContext, 
			sPostLogoutDBFinishMessage);
		pServerContext->m_DatabaseCounter->SetRemoveOnZero(guidPlayer);
	}

	return result;
Error:
	if(pContext) FREE(NULL, pContext);
	return FALSE;
#else
	return FALSE;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ClientSystemClearClientGame(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idAppClient)
{
	ASSERT_RETFALSE(client_system);
	APPCLIENT * client = ClientSystemGetClientById(client_system, idAppClient);
	ASSERT_RETFALSE(client);

	TraceGameInfo("AppClientClearGame() -- appclient: %d", idAppClient);

	NETCLIENTID64 idNetClient = INVALID_NETCLIENTID64;

	{
		CSLAutoLock autolock(&client->m_cs);
		if (client->m_idClient != idAppClient)
		{
			return FALSE;
		}

		sAppClientClearGamePlayerCounters(client);
		client->m_idGame = INVALID_ID;
		client->m_idGameClient = INVALID_GAMECLIENTID;
		idNetClient = client->m_idNetClient;
	}

	if (idNetClient != INVALID_NETCLIENTID64 && !NetClientClearGame(idNetClient))
	{
		return FALSE;
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ClientSystemSetClientGame(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idClient,
	GAMEID idGame,
	const WCHAR * name)
{
	ASSERT_RETFALSE(client_system);
	ASSERT_RETFALSE(idGame != INVALID_ID);
	ASSERT_RETFALSE(name && name[0] != 0);
	APPCLIENT * client = ClientSystemGetClientById(client_system, idClient);
	ASSERT_RETFALSE(client);

	NETCLIENTID64 idNetClient = INVALID_NETCLIENTID64;
	BOOL result = FALSE;

	ONCE
	{
		CSLAutoLock autolock(&client->m_cs);

		if (client->m_idClient != idClient)
		{
			break;
		}
		ASSERT_BREAK(client->m_eState == CLIENT_STATE_CONNECTED || 
			client->m_eState == CLIENT_STATE_WAITINGTOENTERGAME ||
			client->m_eState == CLIENT_STATE_WAITINGTOSWITCHINSTANCE);

		if (client->m_pCurrentGamePlayerCount && !client->m_bCurrentGamePlayerCount)
		{
			InterlockedIncrement(client->m_pCurrentGamePlayerCount);
			client->m_bCurrentGamePlayerCount = TRUE;
		}

		client->m_eState = CLIENT_STATE_INGAME;
		client->m_idGame = idGame;
		PStrCopy(client->m_tCharacterInfo.szCharName, name, MAX_CHARACTER_NAME);

		idNetClient = client->m_idNetClient;

		result = TRUE;
	}

	GameServerContext * serverContext = (GameServerContext*)CurrentSvrGetContext();
	ASSERT_RETFALSE( serverContext );
#if !ISVERSION( CLIENT_ONLY_VERSION )
	if (!NetClientSetGameAndName(
			idNetClient,
			idGame,
			name,
			GameListGetPlayerToGameMailbox( serverContext->m_GameList, idGame)) )
	{
		return FALSE;
	}
#endif
	return result;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ClientSystemSetGameClient(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idAppClient,
	GAMECLIENTID idGameClient)
{
	ASSERT_RETFALSE(client_system);
	ASSERT_RETFALSE(idAppClient != INVALID_ID);

	APPCLIENT * client = ClientSystemGetClientById(client_system, idAppClient);
	ASSERT_RETFALSE(client);

	NETCLIENTID64 idNetClient = INVALID_NETCLIENTID64;
	BOOL result = FALSE;
	client->m_cs.Enter();
	ONCE
	{
		if (client->m_idClient != idAppClient)
		{
			break;
		}

		client->m_idGameClient = idGameClient;

		idNetClient = client->m_idNetClient;

		result = TRUE;
	}
	client->m_cs.Leave();

	if (!NetClientSetGameClientId(idNetClient, idGameClient))
	{
		return FALSE;
	}
	TraceInfo(TRACE_FLAG_GAME_NET, 
		"TraceClientPath: assigning idGameClient %I64x to AppClientID: %d, NetClientID %llu",
		idGameClient, idAppClient, idNetClient);
	return result;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
GAMEID ClientSystemGetClientGame(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idClient)
{
	ASSERT_RETINVALID(client_system);
	APPCLIENT * client = ClientSystemGetClientById(client_system, idClient);
	ASSERT_RETINVALID(client);

	GAMEID idGame = INVALID_ID;

	client->m_cs.Enter();
	ONCE
	{
		if (client->m_idClient != idClient)
		{
			break;
		}
		ASSERT_BREAK(client->m_eState == CLIENT_STATE_INGAME);
		idGame = client->m_idGame;
	}
	client->m_cs.Leave();
	return idGame;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
GAMECLIENTID ClientSystemGetClientGameId(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idClient)
{
	ASSERT_RETINVALID(client_system);
	APPCLIENT * client = ClientSystemGetClientById(client_system, idClient);
	ASSERT_RETINVALID(client);

	GAMECLIENTID idGameClient = INVALID_GAMECLIENTID;

	client->m_cs.Enter();
	ONCE
	{
		if (client->m_idClient != idClient)
		{
			break;
		}
		idGameClient = client->m_idGameClient;
	}
	client->m_cs.Leave();
	return idGameClient;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ClientSystemIsGameReserved( CLIENTSYSTEM * client_system,
								GAMEID idGame,
								int nAreaID /*= INVALID_ID*/ )
{
	ASSERT_RETFALSE(client_system);

	BOOL bResult = FALSE;
	APPCLIENT * client = client_system->m_Clients;
	for (unsigned int ii = 0; ii < MAX_CLIENTS; ii++, client++)
	{
		client->m_cs.Enter();

		if( client->m_idReservedGame == idGame &&
			( nAreaID == INVALID_ID || nAreaID == client->m_idReservedArea ) )
		{
			bResult = TRUE;
		}
		client->m_cs.Leave();
		if( bResult )
		{
			return TRUE;
		}
	}
	return bResult;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int ClientSystemGetReservedArea( CLIENTSYSTEM * client_system,
								  GAMEID idGame )
{
	ASSERT_RETFALSE(client_system);

	int nAreaID = INVALID_ID;
	APPCLIENT * client = client_system->m_Clients;
	for (unsigned int ii = 0; ii < MAX_CLIENTS; ii++, client++)
	{
		client->m_cs.Enter();

		if( client->m_idReservedGame == idGame  )
		{
			nAreaID = client->m_idReservedArea;
		}
		client->m_cs.Leave();
		if( nAreaID != INVALID_ID )
		{
			return nAreaID;
		}
	}
	return nAreaID;
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ClientSystemSetReservedGame(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idAppClient,
	GAMECLIENT *gameclient,
	GAME * game,
	DWORD dwReserveKey,
	int nAreaID )
{
	ASSERT_RETURN(client_system);
	APPCLIENT * client = ClientSystemGetClientById(client_system, idAppClient);
	ASSERT_RETURN(client);

	GAMEID idGame = game ? GameGetId(game) : INVALID_ID;

	// if we have a game, this game client cannot be in a state that will allow
	// a game to be reserved
	if (idGame != INVALID_ID && gameclient)
	{
		if (GameClientTestFlag( gameclient, GCF_CANNOT_RESERVE_GAME ))
		{
			return;
		}
	}
	
	NETCLIENTID64 idNetClient = INVALID_NETCLIENTID64;
	PGUID guid = INVALID_GUID;
	GAMEID idOldReservedGame = INVALID_ID;
	DWORD dwOldReserveKey = 0;

	client->m_cs.Enter();
	ONCE
	{
		if (client->m_idClient != idAppClient)
		{
			break;
		}
		idNetClient = client->m_idNetClient;
		guid = client->m_guid;
		idOldReservedGame = client->m_idReservedGame;
		dwOldReserveKey = client->m_dwReserveKey;
		client->m_idReservedGame = idGame;
		client->m_dwReserveKey = dwReserveKey;
		client->m_idReservedArea = nAreaID;
		if (game != NULL && 
			idOldReservedGame != idGame)
		{
			++game->m_nReserveCount;
		}
	}
	client->m_cs.Leave();
	
	if (idOldReservedGame == INVALID_ID)
	{
		return;
	}
	if (idOldReservedGame == idGame)
	{
		return;
	}
	
	ClientSystemPostUnreserveGameMessage(idNetClient, guid, idOldReservedGame, dwOldReserveKey);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ClientSystemClearReservedGame(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idAppClient,
	GAMECLIENT *pGameClient)
{
	ClientSystemSetReservedGame( client_system, idAppClient, pGameClient, NULL, 0, INVALID_ID );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
GAMEID ClientSystemGetReservedGame(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idClient,
	DWORD & dwReserveKey)
{
	ASSERT_RETINVALID(client_system);
	APPCLIENT * client = ClientSystemGetClientById(client_system, idClient);
	ASSERT_RETINVALID(client);

	GAMEID idGame = INVALID_ID;

	client->m_cs.Enter();
	ONCE
	{
		if (client->m_idClient != idClient)
		{
			break;
		}
		idGame = client->m_idReservedGame;
		dwReserveKey = client->m_dwReserveKey;
	}
	client->m_cs.Leave();
	return idGame;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ClientSystemGetLevelGoingTo(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idClient,
	LEVEL_GOING_TO *pLevelGoingTo)
{
	ASSERT_RETFALSE(client_system);
	APPCLIENT * client = ClientSystemGetClientById(client_system, idClient);
	ASSERT_RETFALSE(client);
	ASSERT_RETFALSE(pLevelGoingTo);

	client->m_cs.Enter();
	ONCE
	{
		if (client->m_idClient != idClient)
		{
			break;
		}
		*pLevelGoingTo = client->m_tLevelGoingTo;
	}
	client->m_cs.Leave();
	
	return TRUE;

}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ClientSystemSetLevelGoingTo(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idClient,
	GAMEID idGame,
	LEVEL_TYPE *pLevelTypeGoingTo)
{
	ASSERT_RETURN(client_system);
	APPCLIENT * client = ClientSystemGetClientById(client_system, idClient);
	ASSERT_RETURN(client);

	client->m_cs.Enter();
	ONCE
	{
		if (client->m_idClient != idClient)
		{
			break;
		}
		
		LEVEL_GOING_TO &tLevelGoingTo = client->m_tLevelGoingTo;
		
		// set or clear game id
		tLevelGoingTo.idGame = idGame;
		
		// set or clear level type
		if (pLevelTypeGoingTo)
		{
			tLevelGoingTo.tLevelType = *pLevelTypeGoingTo;
		}
		else
		{
			LevelTypeInit( tLevelGoingTo.tLevelType );
		}
		
	}
	client->m_cs.Leave();
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ClientSystemSetPendingGameToJoin(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idClient,
	volatile long * pPlayersInGameOrWaitingToJoin,
	volatile long * pPlayersInGame)
{
	ASSERT_RETFALSE(client_system);
	APPCLIENT * client = ClientSystemGetClientById(client_system, idClient);
	ASSERT_RETFALSE(client);

	CSLAutoLock(&client->m_cs);
	if (client->m_idClient != idClient)
	{
		return FALSE;
	}
	ASSERT_RETFALSE(client->m_pGameToJoinPendingPlayerCount == NULL);
	ASSERT_RETFALSE(client->m_bGameToJoinPendingPlayerCount == FALSE);
	ASSERT_RETFALSE(client->m_pCurrentGamePlayerCount == NULL);
	ASSERT_RETFALSE(client->m_bCurrentGamePlayerCount == FALSE);
	client->m_pGameToJoinPendingPlayerCount = pPlayersInGameOrWaitingToJoin;
	client->m_bGameToJoinPendingPlayerCount = TRUE;
	client->m_pCurrentGamePlayerCount = pPlayersInGame;
	client->m_bCurrentGamePlayerCount = FALSE;
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ClientSystemSetServerWarpGame(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idClient,
	GAMEID idGame,
	const WARP_SPEC &tWarpSpec)
{
	ASSERT_RETURN(client_system);
	APPCLIENT * client = ClientSystemGetClientById(client_system, idClient);
	ASSERT_RETURN(client);

	client->m_cs.Enter();
	ONCE
	{
		if (client->m_idClient != idClient)
		{
			break;
		}

		client->m_idServerWarpGame = idGame;
		client->m_tWarpSpec = tWarpSpec;
	}
	client->m_cs.Leave();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
GAMEID ClientSystemGetServerWarpGame(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idClient,
	WARP_SPEC &tWarpSpec)
{
	ASSERT_RETINVALID(client_system);
	APPCLIENT * client = ClientSystemGetClientById(client_system, idClient);
	ASSERT_RETINVALID(client);

	GAMEID idGame = INVALID_ID;

	client->m_cs.Enter();
	ONCE
	{
		if (client->m_idClient != idClient)
		{
			break;
		}
		idGame = client->m_idServerWarpGame;
		if(idGame != INVALID_ID) 
		{
			tWarpSpec = client->m_tWarpSpec;
		}
	}
	client->m_cs.Leave();
	return idGame;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PARTYID ClientSystemGetParty(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idAppClient)
{
	ASSERT_RETINVALID(client_system);
	APPCLIENT * client = ClientSystemGetClientById(client_system, idAppClient);
	ASSERT_RETINVALID(client);

	PARTYID idParty = INVALID_ID;

	client->m_cs.Enter();
	ONCE
	{
		if (client->m_idClient != idAppClient)
		{
			break;
		}
		idParty = client->m_idParty;
	}
	client->m_cs.Leave();

	return idParty;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ClientSystemSetParty(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idAppClient,
	PARTYID idParty)
{
	ASSERT_RETINVALID(client_system);
	APPCLIENT * client = ClientSystemGetClientById(client_system, idAppClient);
	if( !client )
		return INVALID_ID;

	BOOL result = FALSE;

	client->m_cs.Enter();
	ONCE
	{
		if (client->m_idClient != idAppClient)
		{
			break;
		}
		client->m_idParty = idParty;
		client->m_idInvite = INVALID_ID;
		client->m_idGameReserved = INVALID_ID;
		client->m_idInvitor = (APPCLIENTID)INVALID_CLIENTID;
		result = TRUE;
	}
	client->m_cs.Leave();

	return result;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
NETCLIENTID64 ClientGetNetId(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idClient)
{
	ASSERT_RETINVALID(client_system);
	APPCLIENT * client = ClientSystemGetClientById(client_system, idClient);
	ASSERT_RETINVALID(client);

	NETCLIENTID64 idNetClient = INVALID_NETCLIENTID64;
	client->m_cs.Enter();
	if(client->m_idClient == idClient) idNetClient = client->m_idNetClient;
	client->m_cs.Leave();

	return idNetClient;
}


//----------------------------------------------------------------------------
// Set at creation; threadsafe afterward
//----------------------------------------------------------------------------
UNIQUE_ACCOUNT_ID ClientGetUniqueAccountId(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idClient)
{
	ASSERT_RETINVALID(client_system);
	APPCLIENT * client = ClientSystemGetClientById(client_system, idClient);
	ASSERT_RETINVALID(client);

	UNIQUE_ACCOUNT_ID idAccount = INVALID_UNIQUE_ACCOUNT_ID;

	if(client->m_idClient == idClient) idAccount = client->m_idAccount;

	return idAccount;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const WCHAR * ClientGetCharName(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idClient)
{
	ASSERT_RETNULL(client_system);
	APPCLIENT * client = ClientSystemGetClientById(client_system, idClient);
	ASSERT_RETNULL(client);

	if (client->m_idClient == idClient)
	{
		return client->m_tCharacterInfo.szCharName;
	}

	return NULL;
}


//----------------------------------------------------------------------------
// Set at creation time; should never be changed afterward.  If you change
// the badges after login, that is bad.  Very bad!
//----------------------------------------------------------------------------
BOOL ClientGetBadges(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idClient,
	BADGE_COLLECTION &tOutput)
{
	ASSERT_RETFALSE(client_system);
	APPCLIENT * client = ClientSystemGetClientById(client_system, idClient);
	ASSERT_RETFALSE(client);

	if(AppGetType() == APP_TYPE_SINGLE_PLAYER)
	{//Return the global definition cheat badges
		GLOBAL_DEFINITION * pGlobal = DefinitionGetGlobal();
		tOutput = (pGlobal->Badges);
		return TRUE;
	}
	else
	{
		BOOL toRet = FALSE;
		client->m_cs.Enter();
		ONCE
		{
			if(client->m_idClient != idClient) break;
			ASSERT_BREAK(client->m_pBadges);
			client->m_pBadges->GetBadgeCollection(tOutput);
			toRet = TRUE;
		}
		client->m_cs.Leave();

		return toRet;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BADGE_COLLECTION ClientGetBadges(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idClient)
{
	BADGE_COLLECTION toRet;
	ClientGetBadges(client_system, idClient, toRet);
	//If this fails, toRet should remain as initialized, i.e. no badges.
	return toRet;
}


//----------------------------------------------------------------------------
// WARNING: The app client by nature has no access to the game client because
// it does not have the game lock.  So just calling this function will NOT
// update the game client badges.
//
// Use GameClientAddAccomplishmentBadge to update the game client badges.  That
// function should call this one, then update the game client badges.
//----------------------------------------------------------------------------
BOOL AppClientAddAccomplishmentBadge(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idClient,
	int idBadge)
{
	ASSERTX_RETFALSE(BadgeIsAccomplishment(idBadge), 
		"Only accomplishment badges can be added by the game server directly.");
	ASSERT_RETFALSE(client_system);
	APPCLIENT * client = ClientSystemGetClientById(client_system, idClient);
	ASSERT_RETFALSE(client);

	if(AppGetType() == APP_TYPE_SINGLE_PLAYER)
	{//Change the global definition cheat badges
		GLOBAL_DEFINITION * pGlobal = DefinitionGetGlobal();
		(pGlobal->Badges).AddBadge(idBadge);
		return TRUE;
	}
	else
	{
		BOOL toRet = FALSE;
		UNIQUE_ACCOUNT_ID idAccount = INVALID_UNIQUE_ACCOUNT_ID;
		client->m_cs.Enter();
		ONCE
		{
			if(client->m_idClient != idClient) break;
			ASSERT_BREAK(client->m_pBadges);
			idAccount = client->m_idAccount;
			ASSERT_BREAK(idAccount != INVALID_UNIQUE_ACCOUNT_ID);
			client->m_pBadges->AddAccomplishmentBadge(idBadge);
			toRet = TRUE;
		}
		client->m_cs.Leave();

		if(toRet)
		{
#if !ISVERSION(CLIENT_ONLY_VERSION)
			//Send a message to the billing proxy to update the database with our accomplishment.
			BILLING_REQUEST_MODIFY_ACCOMPLISHMENT_BADGE msg;
			msg.AccountId = idAccount;
			msg.iBadgeId = idBadge;
			msg.bAddOrRemove = TRUE; //Not sure what this flag does, ask Paxton in code review.

			toRet = GameServerToBillingProxySendMessage(
				&msg, EBillingServerRequest_ModifyAccomplishmentBadge, NetClientGetBillingInstance(client->m_idNetClient));
#endif
		}

		return toRet;
	}
}


//----------------------------------------------------------------------------
// will return 0 if the client is not subject to fatigue rules, otherwise returns accumulated online time in seconds
//----------------------------------------------------------------------------
int ClientGetFatigueOnlineSecs(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idClient)
{
	ASSERT_RETFALSE(client_system);
	APPCLIENT * client = ClientSystemGetClientById(client_system, idClient);
	ASSERT_RETFALSE(client);

	return !client->m_bSubjectToFatigue ? 0
		: client->m_iFatigueOnlineSecs + static_cast<int>(time(0) - client->m_LoginTime);
}


//----------------------------------------------------------------------------
// Take all the clients from "From" and transfer them to "To"
// Careful: don't grab all the locks and deadlock everything.
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL ClientSystemTransferClients(
	CLIENTSYSTEM * client_system,
	GAMECLIENT *pGameClient,
	NETCLIENTID64 idNetClientTo, 
	NETCLIENTID64 idNetClientFrom)
{
	ASSERT_RETFALSE(client_system);
	ASSERT_RETFALSE(idNetClientFrom != idNetClientTo);

	//Get the NETCLT_USER of From, and look at all the data.
	NETCLT_USER *fromData = (NETCLT_USER *) SvrNetGetClientContext(idNetClientFrom);
	ASSERT_RETFALSE(fromData);
	NETCLT_USER *toData = (NETCLT_USER *) SvrNetGetClientContext(idNetClientTo);
	ASSERT_RETFALSE(toData);

	GameServerContext *pServerContext = (GameServerContext *)CurrentSvrGetContext();
	ASSERT_RETFALSE(pServerContext);

	APPCLIENTID idAppClient = INVALID_ID;
	GAMECLIENTID idGameClient = INVALID_ID;
	GAMEID idGame = INVALID_ID;

	fromData->m_cReadWriteLock.ReadLock();
	{
		idAppClient = fromData->m_idAppClient;
		idGameClient = fromData->m_idGameClient;
		idGame = fromData->m_idGame;
	}
	fromData->m_cReadWriteLock.EndRead();

	ASSERT_RETFALSE(idGameClient == ClientGetId(pGameClient));
	APPCLIENT * appclient = ClientSystemGetClientById(client_system, idAppClient);
	ASSERT_RETFALSE(appclient);

	toData->m_cReadWriteLock.WriteLock();
	{
		toData->m_idAppClient = idAppClient;
		toData->m_idGameClient = idGameClient;
		toData->m_idGame = idGame;
	}
	toData->m_cReadWriteLock.EndWrite();

	appclient->m_cs.Enter();
	appclient->m_idNetClient = idNetClientTo;
	appclient->m_cs.Leave();

	fromData->m_cReadWriteLock.WriteLock();
	{
		fromData->m_idAppClient = INVALID_ID;
		fromData->m_idGameClient = INVALID_ID;
		fromData->m_idGame = INVALID_ID;
	}
	fromData->m_cReadWriteLock.EndWrite();

	pGameClient->m_idNetClient = idNetClientTo;	

	SvrNetAttachRequestMailbox(idNetClientTo, GameListGetPlayerToGameMailbox(
		pServerContext->m_GameList, idGame ) );

	return TRUE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ClientSystemSendPlayerList(
	CLIENTSYSTEM * client_system,
	NETCLIENTID64 idNetClient)
{
	ASSERT_RETURN(client_system);
	ASSERT_RETURN(idNetClient != INVALID_NETCLIENTID64);

	unsigned int total = 0;
	
	APPCLIENT * client = client_system->m_Clients;
	for (unsigned int ii = 0; ii < MAX_CLIENTS; ii++, client++)
	{
		client->m_cs.Enter();
		ONCE
		{
			if (client->m_eState != CLIENT_STATE_INGAME)
			{
				break;
			}
			s_SendSysTextNet(CHAT_TYPE_WHISPER, FALSE, idNetClient, client->m_tCharacterInfo.szCharName);
			total++;
		}
		client->m_cs.Leave();
	}
	s_SendSysTextNetFmt(CHAT_TYPE_WHISPER, FALSE, idNetClient, CONSOLE_SYSTEM_COLOR, GlobalStringGet( GS_TOTAL_PLAYERS ), total);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ClientSystemSendMessageToAll(
	CLIENTSYSTEM * client_system,
	NETCLIENTID64 idNetClient,
	BYTE command,
	MSG_STRUCT * msg)
{
	REF(idNetClient);
	ASSERT_RETURN(client_system);

	APPCLIENT * client = client_system->m_Clients;
	for (unsigned int ii = 0; ii < MAX_CLIENTS; ii++, client++)
	{
		client->m_cs.Enter();
		ONCE
		{
			if (client->m_eState != CLIENT_STATE_INGAME)
			{
				break;
			}
			ASSERT_BREAK(client->m_idNetClient != INVALID_NETCLIENTID64);
			s_SendMessageNetClient(client->m_idNetClient, command, msg);
		}
		client->m_cs.Leave();
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define ABR(x)		if(!(x)) {bRet = FALSE;}

BOOL ClientValidateIdConsistency(
	GAME * game,
	GAMECLIENTID idGameClient)
{
	GAMECLIENT * client = ClientGetById(game, idGameClient);
	ASSERT_RETFALSE(client);

	BOOL bRet = TRUE;

	NETCLIENTID64 idNetClient = client->m_idNetClient;
	APPCLIENTID idAppClient = client->m_idAppClient;

	CLIENTSYSTEM * pClientSystem = AppGetClientSystem();

	NETCLIENTID64 idNetClientFromAppClient = ClientGetNetId(pClientSystem, idAppClient);
	ABR(idNetClient == idNetClientFromAppClient);

	GAMECLIENTID idGameClientFromAppClient = ClientSystemGetClientGameId(pClientSystem, idAppClient);
	ABR(idGameClient == idGameClientFromAppClient);

	ABR(NetClientValidateIdConsistency(idNetClient));
	ABR(NetClientValidateIdConsistency(idNetClientFromAppClient));

	ABR(NetClientGetGameId(idNetClient) == GameGetId(game));

	return bRet;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ClientSystemIsSystemAppClient(
	APPCLIENTID idAppClient)
{
	return sGetSystemAppClientByClientID( idAppClient ) != SAC_INVALID;
}
