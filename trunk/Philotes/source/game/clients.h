//----------------------------------------------------------------------------
// clients.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifdef	_CLIENTS_H_
//#pragma message ("    HINT: " __FILE__ " included multiple times in this translation unit")
#else
#define _CLIENTS_H_


//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#ifndef _NET_H_
#include "net.h"
#endif

#ifndef _ACCOUNTBADGES_H_
#include "accountbadges.h"
#endif

#ifndef _GAMECLIENT_H_
#include "gameclient.h"
#endif

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------
struct LEVEL;
struct UNIT_FILE;
enum UNIT_COLLECTION_TYPE;
enum PLAYER_LOAD_TYPE;

//----------------------------------------------------------------------------
// TYPEDEF
//----------------------------------------------------------------------------
typedef DWORD							APPCLIENTID;
typedef unsigned __int64				GAMECLIENTID;
typedef UINT16							SVRINSTANCE; // also defined in ServerRunnerAPI.h
typedef DWORD							UNIT_COLLECTION_ID;

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
enum PLAYER_SAVE_FORMAT
{
	PSF_INVALID,
	PSF_HIERARCHY,			// player file with unit hierarchy
	PSF_DATABASE,			// player file loaded from database (individual units, no hierarchy)
};

void ClientSaveFileInit(
	struct CLIENT_SAVE_FILE &tSaveFile);
	
//----------------------------------------------------------------------------
struct CLIENT_SAVE_FILE
{

	WCHAR uszCharName[ MAX_CHARACTER_NAME ];
	BYTE *pBuffer;
	DWORD dwBufferSize;
	PLAYER_SAVE_FORMAT eFormat;
	PGUID guidUnitRoot;					// for when loading item trees
	PGUID guidDestPlayer;				// for when loading item trees
	PLAYER_LOAD_TYPE ePlayerLoadType;	// the type of load this was for
	
	CLIENT_SAVE_FILE::CLIENT_SAVE_FILE( void )
	{
		ClientSaveFileInit( *this );
	}
		
};

//----------------------------------------------------------------------------
// System app client ids
//----------------------------------------------------------------------------
#define APPCLIENTID_AUCTION_SYSTEM		((APPCLIENTID)-2)
#define APPCLIENTID_ITEM_RESTORE_SYSTEM ((APPCLIENTID)-3)

//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------
// net client functions
BOOL NetClientSetAppClientId(
	NETCLIENTID64 idNetClient,
	APPCLIENTID idAppClient);

APPCLIENTID NetClientGetAppClientId(
	void * data);

APPCLIENTID NetClientGetAppClientId(
	NETCLIENTID64 idClient);

GAMEID NetClientGetGameId(
	void * data);

GAMEID NetClientGetGameId(
	NETCLIENTID64 idClient);

GAMECLIENTID NetClientGetGameClientId(
	void * data);

GAMECLIENTID NetClientGetGameClientId(
	NETCLIENTID64 idClient);

UNIQUE_ACCOUNT_ID NetClientGetUniqueAccountId(
	void * data);

UNIQUE_ACCOUNT_ID NetClientGetUniqueAccountId(
	NETCLIENTID64 idClient);

SVRINSTANCE NetClientGetBillingInstance(
	NETCLIENTID64 idClient );

BOOL NetClientClearGame(
	NETCLIENTID64 idNetClient);

BOOL NetClientSetGameAndName(
	NETCLIENTID64 idNetClient,
	GAMEID idGame,
	const WCHAR * wszPlayerName,
	struct SERVER_MAILBOX * mailbox);

BOOL NetClientGetName(
	NETCLIENTID64 idNetClient,
	WCHAR * wszPlayerName,
	unsigned int nMaxLength);

BOOL NetClientQueryRemoved(
	NETCLIENTID64 idClient);

BOOL NetClientQueryDetached(
	NETCLIENTID64 idClient);

size_t NetClientGetUserDataSize(
	void);

void NetClientInitUserData(
	void * data);

BOOL NetClientValidateIdConsistency(
	NETCLIENTID64 idNetClient);


// app client functions
struct CLIENTSYSTEM* ClientSystemInit(
	void);

void ClientSystemFree(
	CLIENTSYSTEM * client_system);

APPCLIENTID ClientSystemNewClient(
	CLIENTSYSTEM * client_system,
	NETCLIENTID64 idNetClient,
	UNIQUE_ACCOUNT_ID idAccount,
	const BADGE_COLLECTION &tBadges,
	time_t LoginTime,
	BOOL bSubjectToFatigue,
	int iFatigueOnlineSecs,
	time_t GagUntilTime,
	MEMORYPOOL* pPool = NULL);

void ClientSystemRemoveClient(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idClient);

BOOL ClientSaveFilePostProcess( 
	CLIENTSYSTEM *pClientSystem,
	APPCLIENTID idAppClient,
	UNIT_COLLECTION_TYPE eCollectionType,
	UNIT_COLLECTION_ID idCollection);

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
	DWORD save_size);

BOOL ClientSystemAddClientSaveFileChunk(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idClient,
	UNIT_COLLECTION_TYPE eCollectionType,
	UNIT_COLLECTION_ID idCollection,
	const BYTE * buf,
	unsigned int size,
	BOOL & bDone,
	PLAYER_SAVE_FORMAT & eFormat);

BOOL ClientSaveDataGetRootUnitBuffer(
	const CLIENT_SAVE_FILE *pClientSaveFile,
	UNIT_FILE *pUnitFile);
	
BOOL ClientSystemGetClientSaveFile(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idClient,
	UNIT_COLLECTION_TYPE eCollectionType,
	UNIT_COLLECTION_ID idCollection,
	CLIENT_SAVE_FILE *pClientSaveFile);

BOOL ClientSystemRecycleClientSaveFile(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idAppClient,
	UNIT_COLLECTION_TYPE eCollectionType,
	UNIT_COLLECTION_ID idCollection);

void ClientSystemFreeCharacterSaveFile(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idClient,
	BOOL bForce);

BOOL ClientSystemSetClientInstanceSaveBuffer(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idClient,
	const WCHAR * name,
	BYTE * buffer,
	DWORD save_size,
	DWORD key);

void ClientSystemSetGaggedUntilTime(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idAppClient,
	time_t tTimeGaggedUntil);

time_t ClientSystemGetGaggedUntilTime(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idAppClient);
	
BOOL ClientSystemSetLogoutSaveCallback(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idAppClient);

BOOL ClientSystemGetClientInstanceSaveBuffer(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idClient,
	CLIENT_SAVE_FILE *pClientSaveFile,
	DWORD * key);

BOOL ClientSystemClearClientGame(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idAppClient);

BOOL ClientSystemSetClientGame(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idClient,
	GAMEID idGame,
	const WCHAR * name);

BOOL ClientSystemSetGameClient(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idAppClient,
	GAMECLIENTID idGameClient);

GAMEID ClientSystemGetClientGame(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idClient);

GAMECLIENTID ClientSystemGetClientGameId(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idClient);

BOOL ClientSystemIsGameReserved( 
	CLIENTSYSTEM * client_system,
	GAMEID idGame,
	int nAreaID = INVALID_ID );

int ClientSystemGetReservedArea( 
	CLIENTSYSTEM * client_system,
	GAMEID idGame );

void ClientSystemSetReservedGame(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idAppClient,
	GAMECLIENT *gameclient,
	GAME * game,
	DWORD dwReserveKey,
	int nAreaID );

void ClientSystemClearReservedGame(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idAppClient,
	GAMECLIENT *pGameClient);

GAMEID ClientSystemGetReservedGame(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idClient,
	DWORD & dwReserveKey);

BOOL ClientSystemGetLevelGoingTo(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idClient,
	struct LEVEL_GOING_TO *pLevelGoingTo);

void ClientSystemSetLevelGoingTo(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idClient,
	GAMEID idGame,
	struct LEVEL_TYPE *pLevelTypeGoingTo);

BOOL ClientSystemSetPendingGameToJoin(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idClient,
	volatile long * pPlayersInGameOrWaitingToJoin,
	volatile long * pPlayersInGame);

void ClientSystemSetServerWarpGame(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idClient,
	GAMEID idGame,
	const struct WARP_SPEC &tWarpSpec);

GAMEID ClientSystemGetServerWarpGame(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idClient,
	WARP_SPEC &tWarpSpec);

BOOL ClientSystemSetGuid(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idAppClient,
	PGUID guid);

PGUID ClientSystemGetGuid(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idAppClient);

PARTYID ClientSystemGetParty(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idAppClient);

BOOL ClientSystemSetParty(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idAppClient,
	PARTYID idParty);
								
APPCLIENTID ClientSystemFindByName(
	CLIENTSYSTEM * client_system,
	const WCHAR * wszName);

APPCLIENTID ClientSystemFindByPguid(
	CLIENTSYSTEM * client_system,
	PGUID clientGuid );

NETCLIENTID64 ClientGetNetId(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idClient);

UNIQUE_ACCOUNT_ID ClientGetUniqueAccountId(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idClient);

const WCHAR *ClientGetCharName(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idClient);

BOOL ClientHasBadge(
	GAMECLIENT * pClient,
	int nBadge );

BOOL ClientHasPermission(
	GAMECLIENT * pClient,
	const BADGE_COLLECTION & tPermission );

BOOL ClientGetBadges(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idClient,
	BADGE_COLLECTION &tOutput);

BADGE_COLLECTION ClientGetBadges(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idClient);

BOOL AppClientAddAccomplishmentBadge(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idClient,
	int idBadge);

BOOL GameClientAddAccomplishmentBadge(
	GAMECLIENT * pClient,
	int idBadge);

// returns 0 if the client is not subject to fatigue rules, otherwise returns accumulated online time in seconds
int ClientGetFatigueOnlineSecs(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idClient);

BOOL ClientSystemTransferClients(
	CLIENTSYSTEM * client_system,
	GAMECLIENT *pGameClient,
	NETCLIENTID64 idNetClientTo, 
	NETCLIENTID64 idNetClientFrom);

BOOL ClientSystemGetName(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idAppClient,
	WCHAR * wszName,
	unsigned int len);

BOOL ClientSystemGetCharacterInfo(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idAppClient,
	struct CHARACTER_INFO &tCharacterInfo);

BOOL ClientSystemSetCharacterInfo(
	CLIENTSYSTEM * client_system,
	APPCLIENTID idAppClient,
	const CHARACTER_INFO &tCharacterInfo);
	
void ClientSystemSendPlayerList(
	CLIENTSYSTEM * client_system,
	NETCLIENTID64 idNetClient);

void ClientSystemSendMessageToAll(
	CLIENTSYSTEM * client_system,
	NETCLIENTID64 idNetClient,
	BYTE command,
	MSG_STRUCT * msg);

void ClientSystemPostPartyMemberRemoved( 
	GAMEID idGame, 
	NETCLIENTID64 idNetClient,	
	PGUID guid);

BOOL ClientSystemIsSystemAppClient(
	APPCLIENTID idAppClient);
	
#endif // _CLIENTS_H_
