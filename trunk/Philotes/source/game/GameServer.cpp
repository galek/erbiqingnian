//----------------------------------------------------------------------------
// GameServer.cpp
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//
// Lib binding file to bring all of the game server functionality under
//  the server runner API roof.
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "c_message.h"
#include "s_message.h"
#include "game.h"
#include "primepriv.h"
#include "minitown.h"
#include "perfhier.h"
#include "gamelist.h"
#include "netclient.h"
#include "openLevel.h"
#include "critsect.h"
#include "s_network.h"
#include "chat.h"
#include "s_chatNetwork.h"
#include "svrstd.h"
#include "s_partyCache.h"
#include "guidcounters.h"
#include "GameServer.h"
#include "accountbadges.h"
#include <ServerSuite/Login/LoginDef.h>
#include <ServerSuite/UserServer/MsgsToUserServer.h>
#include "version.h"

#if ISVERSION(SERVER_VERSION)
#include "ServerSuite/GameLoadBalance/LoadBalancingDef.h"
#include "ServerSuite/GameLoadBalance/GameLoadBalanceGameServerRequestMsg.h"
#include "s_loadBalanceNetwork.h"
#include "ServerSuite/CSRBridge/CSRBridgeGameRequestMsgs.h"
#include "ServerSuite/CSRBridge/CSRBridgeGameResponseMsgs.h"
#include "playertrace.h"
#include "GameServer.cpp.tmh"
#include <winperf.h>
#include <Watchdogclient_counter_definitions.h>
#include <PerfHelper.h>
#include "s_townInstanceList.h"
#include "dbunit.h"
#include "utilitygame.h"
#include "s_auctionNetwork.h"
#endif


//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------
#define MAX_FREE_CLIENTS 40


//----------------------------------------------------------------------------
// GAME SERVER CONFIG STRUCT
//----------------------------------------------------------------------------
START_SVRCONFIG_DEF(GameServerCommands)
	SVRCONFIG_STR_FIELD(0, CommandLine, false, "")
END_SVRCONFIG_DEF

//----------------------------------------------------------------------------
// LOCAL STRUCTURES
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
struct ACCOUNTID_HASH_ENTRY
{
	UNIQUE_ACCOUNT_ID id;
	ACCOUNTID_HASH_ENTRY *next;
	NETCLIENTID64 idNetClient;
};

typedef CS_HASH_LOCKING<ACCOUNTID_HASH_ENTRY, UNIQUE_ACCOUNT_ID, MAX_CLIENTS> ACCOUNTID_HASH;

ACCOUNTID_HASH sAccountIdHash;
#endif
//----------------------------------------------------------------------------
//MISCELLANEOUS FUNCTIONS
//----------------------------------------------------------------------------




GAMEID GameIdNew(
	UINT index,
	UINT iter)
{
	GameServerContext * serverContext = (GameServerContext *)CurrentSvrGetContext();
	ASSERT_RETINVALID(serverContext);
	return (GAMEID)(MAKE_NBIT_VALUE(GAMEID, index, GAMEID_INDEX_START, GAMEID_INDEX_BITS) +
		MAKE_NBIT_VALUE(GAMEID, iter, GAMEID_ITER_START, GAMEID_ITER_BITS) +
		MAKE_NBIT_VALUE(GAMEID, serverContext->m_idServer, GAMEID_SVRID_START, GAMEID_SVRID_BITS));
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UINT64 GameServerGenerateGUID(
	void )
{
	GameServerContext * serverContext = (GameServerContext*)CurrentSvrGetContext();
	ASSERT_RETINVALID( serverContext );
	return AppGenerateGUID(serverContext->m_idServer );
}

//----------------------------------------------------------------------------
// Unlike SendSwitchMessageUnsafe, actually specifies the game and instance
// to warp to.  Unsafe can be used arbitrarily but will likely have evil
// consequences.  This, on the other hand, should only get called on after a
// ClientWarpToInstanceDo determines we need to switch game servers, and
// has handled the game closing and warp screen loading.
// TODO: Handle the gameserver attach so he's dumped correctly.
//----------------------------------------------------------------------------
#if ISVERSION(SERVER_VERSION)
BOOL GameServerSwitchInstance(
	NETCLIENTID64 idNetClient, 
	int nServerInstance,
	const WCHAR * szPlayerName,
	WARP_SPEC * pWarpSpec,
	GAMEID idGame)
{
	MSG_GAMELOADBALANCE_GAMESERVER_REQUEST_SWITCH_GAMESERVER msg;
	if(pWarpSpec)
	{
		msg.bWarpData = TRUE;
		msg.tWarpSpec = *pWarpSpec;
	}
	msg.idGame = idGame;
	msg.idServerToSwitchTo = ServerId(GAME_SERVER, nServerInstance);
	PStrCopy(msg.szCharName, szPlayerName, MAX_CHARACTER_NAME);
	//Get accountId and idNet from netclient context
	NETCLT_USER * user = (NETCLT_USER *)SvrNetGetClientContext(idNetClient);
	if (!user)
	{
		return FALSE;
	}
	user->m_cReadWriteLock.ReadLock();
	msg.accountId = user->m_idAccount;
	msg.idNet = user->m_netId;
	user->m_cReadWriteLock.EndRead();

	TraceInfo(TRACE_FLAG_GAME_NET,
		"Requesting switch of player %ls with accountId %I64x to gameid %I64x on server %d",
		szPlayerName, msg.accountId, idGame, msg.idServerToSwitchTo);

	return GameServerToGameLoadBalanceServerSendMessage(&msg, 
		GAMELOADBALANCE_GAMESERVER_REQUEST_SWITCH_GAMESERVER);
}
#endif

//----------------------------------------------------------------------------
// Sends a request to the Userserver to switch client to another gameserver.
// Doesn't handle any of the saving/loading character bookkeeping.
//----------------------------------------------------------------------------
#if ISVERSION(SERVER_VERSION)
BOOL GameServerSendSwitchMessageUnsafe(
	NETCLIENTID64 idNetClient, 
	int nServerInstance,
	const WCHAR * szPlayerName,
	GAME * pGameCurrent)
{
	{
		MSG_SCMD_START_LEVEL_LOAD msgLoad;
		
		s_SendMessageNetClient(idNetClient, SCMD_START_LEVEL_LOAD, &msgLoad);
	}
	//Send a SCMD_GAMECLOSE so he forgets the old game.
	{
		MSG_SCMD_GAME_CLOSE msgClose;
		msgClose.idGame = (pGameCurrent? GameGetId(pGameCurrent):INVALID_GAMEID);
		msgClose.bRestartingGame = FALSE;
		msgClose.bShowCredits = FALSE;

		s_SendMessageNetClient(idNetClient, SCMD_GAME_CLOSE, &msgClose);
	}

	return GameServerSwitchInstance(idNetClient, nServerInstance, szPlayerName, NULL, INVALID_ID);
}
#endif
//----------------------------------------------------------------------------
// FUNCTIONS USED BY SERVER_SPEC
//----------------------------------------------------------------------------
void sGameServerServerFree(
	GameServerContext * toFree )
{
	if( !toFree )
		return;

#if !ISVERSION(CLIENT_ONLY_VERSION)
	GameListFree( toFree->m_GameList );
	MiniTownFree( toFree->m_MiniTown );
	ClientSystemFree( toFree->m_ClientSystem );
	PartyCacheFree( toFree->m_PartyCache );

#if VALIDATE_UNIQUE_GUIDS
	GuidTrackerFree(toFree->m_pMemPool, toFree->m_GuidTracker);
#endif

	DatabaseCounterFree( toFree->m_pMemPool, toFree->m_DatabaseCounter );
	toFree->m_DatabaseCounter = NULL;
	OpenLevelFree( toFree->m_pAllOpenLevels );

	sAccountIdHash.Destroy();

	SvrMailboxRelease( toFree->m_pPlayerToGameServerMailBox );

	NETCLT_USER * user = NULL;
	user = toFree->m_clientFinishedList[0].Pop();
	while( user ) {
		toFree->m_clientFreeList.Free(toFree->m_pMemPool,user);
		user = toFree->m_clientFinishedList[0].Pop();
	}
	toFree->m_clientFinishedList[0].Free(toFree->m_pMemPool);
	user = toFree->m_clientFinishedList[1].Pop();
	while( user ) {
		toFree->m_clientFreeList.Free(toFree->m_pMemPool,user);
		user = toFree->m_clientFinishedList[1].Pop();
	}
	toFree->m_clientFinishedList[1].Free(toFree->m_pMemPool);
	toFree->m_clientFreeList.Destroy( toFree->m_pMemPool );

#if ISVERSION(SERVER_VERSION)
	TownInstanceListDestroy(toFree->m_pInstanceList);
	PERF_FREE_INSTANCE(toFree->m_pPerfInstance);

	toFree->m_csUtilGameInstance.Free();
	ArrayClear(toFree->m_listAuctionItemSales);
	toFree->m_csAuctionItemSales.Free();

	if (toFree->m_DatabaseUnitCollectionList != NULL) {
		toFree->m_DatabaseUnitCollectionList->Destroy( toFree->m_pMemPool );
		FREE( toFree->m_pMemPool, toFree->m_DatabaseUnitCollectionList );
	}
#endif

	FREE( toFree->m_pMemPool, toFree );
#endif //#if !ISVERSION(CLIENT_ONLY_VERSION)

}

//----------------------------------------------------------------------------
//	server startup method
//----------------------------------------------------------------------------
LPVOID GameServerServerInit(
	MEMORYPOOL * pPool,
    SVRID svrId,
	const SVRCONFIG * svrConfig)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	GameServerContext * context = NULL;
	CLT_VERSION_ONLY(UNREFERENCED_PARAMETER(svrConfig));

	CSAutoLock lock(AppGetSvrInitLock());
	ASSERT_RETNULL( SrvValidateMessageTable() );
	ASSERT_RETNULL( SrvValidateChatMessageTable() );

	SVR_VERSION_ONLY(ASSERT_RETNULL(svrConfig));
#if ISVERSION(SERVER_VERSION)

#if !ISVERSION(DEVELOPMENT)
	if(AppIsTugboat())
    {
        PrimeInitAppNameAndMutexGlobals("-tugboat");
    }
	else
		PrimeInitAppNameAndMutexGlobals(NULL);
#else 
		PrimeInitAppNameAndMutexGlobals(((GameServerCommands*)svrConfig)->CommandLine);
#endif // DEVELOPMENT
	ASSERT_GOTO(
		PrimeInit( NULL, NULL, ((GameServerCommands*)svrConfig)->CommandLine,0 ),
		_INIT_ERROR ) ;
#endif // SERVER_VERSION

	context = (GameServerContext *)MALLOCZ(pPool, sizeof(GameServerContext) );
	ASSERT_RETNULL(context);

	//	init members
	context->m_pMemPool		= pPool;
	context->m_idServer		= svrId;
	context->m_shouldExit	= FALSE;
	context->m_mainThread	= NULL;
	context->m_chatInitializedForApp = FALSE;
	context->m_clientFreeList.Init( MAX_CLIENTS, NetClientFreeUserData );
	context->m_outstandingDbRequests = 0;
	context->m_clients = 0;

	context->m_clientFinishedList[0].Init(context->m_pMemPool);
	context->m_clientFinishedList[1].Init(context->m_pMemPool);
	context->m_bListSwap = 0;

#if ISVERSION(SERVER_VERSION)
	context->m_pInstanceList = TownInstanceListCreate(context->m_pMemPool);
	ASSERT_GOTO(context->m_pInstanceList, _INIT_ERROR);
	context->m_nLastTownInstanceUpdateTimer = 0;
#endif

	context->m_pPlayerToGameServerMailBox = SvrMailboxNew(pPool);
	ASSERT_GOTO( context->m_pPlayerToGameServerMailBox, _INIT_ERROR );

	sAccountIdHash.Init(pPool);

	context->m_pAllOpenLevels = OpenLevelInit( pPool );
	ASSERT_GOTO( context->m_pAllOpenLevels, _INIT_ERROR );	
	context->m_DatabaseCounter = DatabaseCounterInit( pPool );
	ASSERT_GOTO( context->m_DatabaseCounter, _INIT_ERROR );
#if VALIDATE_UNIQUE_GUIDS
	context->m_GuidTracker = GuidTrackerInit(pPool);
	ASSERT_GOTO(context->m_GuidTracker, _INIT_ERROR);
#endif
	context->m_PartyCache = PartyCacheInit( pPool );
	ASSERT_GOTO( context->m_PartyCache, _INIT_ERROR );
	context->m_ClientSystem = ClientSystemInit();
	ASSERT_GOTO( context->m_ClientSystem, _INIT_ERROR );
	ASSERT_GOTO( MiniTownInit( context->m_MiniTown ), _INIT_ERROR );
	ASSERT_GOTO( GameListInit( pPool, context->m_GameList ), _INIT_ERROR );

#if ISVERSION(SERVER_VERSION)
    WCHAR instanceName[MAX_PERF_INSTANCE_NAME];
    swprintf(instanceName,sizeof(instanceName),L"GameServer_%hu",ServerIdGetInstance(svrId));
	context->m_pPerfInstance = PERF_GET_INSTANCE(GameServer, instanceName);

	context->m_idUtilGameInstance = INVALID_ID;
	context->m_csUtilGameInstance.Init();
	context->m_idAppCltAuctionHouse = INVALID_CLIENTID;
	context->m_dwLastEmailCheckTick = 0;
	ArrayInit(context->m_listAuctionItemSales, pPool, 16);
	context->m_csAuctionItemSales.Init();
	context->m_dwAuctionHouseFeeRate = 0;
	context->m_dwAuctionHouseMaxItemSaleCount = 0;
	ASSERT_GOTO(AuctionOwnerRetrieveAccountInfo(
		&context->m_idAccountAuctionHouse,
		&context->m_idCharAuctionHouse), _INIT_ERROR);

	context->m_DatabaseUnitCollectionList = (FREELIST<DATABASE_UNIT_COLLECTION> *)MALLOC( pPool, sizeof(FREELIST<DATABASE_UNIT_COLLECTION>) );
	ASSERT_GOTO( context->m_DatabaseUnitCollectionList, _INIT_ERROR );
	context->m_DatabaseUnitCollectionList->Init(UINT_MAX, MAX_DBUNIT_FREELIST_SIZE);
#endif
	
	return context;

_INIT_ERROR:
	sGameServerServerFree( context );
	return NULL;

#else
	REF( pPool );
	REF( svrId );
	REF( svrConfig );
	return NULL;
#endif //#if !ISVERSION(CLIENT_ONLY_VERSION)

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(SERVER_VERSION)
void GameServerEntryPoint(
	void * pContext)
{
    DWORD asyncTID = 0;
	GameServerContext * svrContext = (GameServerContext *)pContext;
	ASSERT_RETURN(svrContext);

    asyncTID = AsyncFileRegisterThread();

	TIME tiLastHeartbeat = 0.0f;
	
#if DECOUPLE_GAME_LIST	
	UINT64 lastProcessTimestamp = PGetPerformanceCounter();
	UINT64 perfTicksPerSecond = PGetPerformanceFrequency();
#endif

	// Add the auction owner client to the utility game
/*
	GAMEID idUtilGame = UtilityGameGetGameId(svrContext);
	if (idUtilGame != INVALID_GAMEID) {
		SERVER_MAILBOX* pMailBox = GameListGetPlayerToGameMailbox(svrContext->m_GameList, svrContext->m_idUtilGameInstance);
		if (pMailBox != NULL) {
			MSG_APPCMD_UTIL_GAME_ADD_AH_CLIENT msg;
			SvrNetPostFakeClientRequestToMailbox(
				pMailBox, ServiceUserId(USER_SERVER, 0, 0), &msg, APPCMD_UTIL_GAME_ADD_AH_CLIENT);
		}
	}
*/
	while (!svrContext->m_shouldExit)
	{
		PERF_END(APP);
		PERF_START(APP);
		PERF_INC();
		HITCOUNT_INC();

		unsigned int requested_sim_frames;
#if DECOUPLE_GAME_LIST
		unsigned int sim_frames = requested_sim_frames = CommonTimerSimulationUpdate(AppCommonGetTimer(), false);
		sim_frames = 1;
		
		UINT64 now = PGetPerformanceCounter();
		now = MAX<UINT64>(now, lastProcessTimestamp); // Account for apparent inconsistencies?			
		
		float elapsedPerfTicks = now - lastProcessTimestamp;
		float elapsedMilliseconds = elapsedPerfTicks / perfTicksPerSecond * 1000.0f;
		if(elapsedMilliseconds < 50.0f)
		{
			UINT64 sleepMilliseconds = 50.0f - elapsedMilliseconds;
			Sleep((DWORD)sleepMilliseconds);	
		}
				
		lastProcessTimestamp += perfTicksPerSecond / 20;
#else
		unsigned int sim_frames = requested_sim_frames = CommonTimerSimulationUpdate(AppCommonGetTimer(), true);
#endif
		if (requested_sim_frames > 2) 
		{
			TraceError("Requested %d frames at once on server.  Lag!\n", requested_sim_frames);
		}
			
		svrContext->m_pPerfInstance->GameServerSimulationFrames = requested_sim_frames;
		PERF_ADD64(svrContext->m_pPerfInstance,GameServer,GameServerSimulationFrameRate,sim_frames);

		TIME tiNow = AppCommonGetCurTime();
		if (tiNow - tiLastHeartbeat > TIME_PER_GAMESERVER_HEARTBEAT)
		{
			tiLastHeartbeat = tiNow;
			SendGameLoadBalanceHeartBeat(svrContext);
		}
//    	AsyncFileProcessComplete(asyncTID);			// shouldn't async on server

		extern void ServerOnly_DoInGame(unsigned int);
		ServerOnly_DoInGame(sim_frames);

		// tell the util game instance to check for item-emails every 2 min
		GAMEID idUtilGame = UtilityGameGetGameId(svrContext);
		if (idUtilGame != INVALID_GAMEID) {
			DWORD dwCurTick = GetTickCount();

			if (dwCurTick < svrContext->m_dwLastEmailCheckTick || 
				dwCurTick - svrContext->m_dwLastEmailCheckTick > 1000 * 60 * 2) {
				svrContext->m_dwLastEmailCheckTick = dwCurTick;
				
				SERVER_MAILBOX* pMailBox = GameListGetPlayerToGameMailbox(svrContext->m_GameList, idUtilGame);
				if (pMailBox != NULL) {
					MSG_APPCMD_UTIL_GAME_CHECK_ITEM_EMAIL msg;
					msg.EmailGUID = INVALID_GUID;
//					SvrNetPostFakeClientRequestToMailbox(
//						pMailBox, ServiceUserId(USER_SERVER, 0, 0),
//						&msg, APPCMD_UTIL_GAME_CHECK_ITEM_EMAIL);
				}
			}
		}
	}

	//	wait for all db requests to finish
	while (svrContext->m_outstandingDbRequests)
	{
		Sleep(10);
	}

	sGameServerServerFree(svrContext);

	PrimeClose();
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void GameServerCommandCallback(
	void * context,
    SR_COMMAND  command )
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	ASSERT_RETURN( context );
	GameServerContext * svrContext = (GameServerContext*)context;
	
	switch( command )
	{
	case SR_COMMAND_SHUTDOWN:
		//	in server version we set the exit flag and wait for the main thread
		//		to see it and exit.
		svrContext->m_shouldExit = TRUE;

		// CHB 2007.04.06 - Uncomment the following line to fix the memory leak.
		// Normally GameServerEntryPoint() frees the server.  However, according
		// to the server specification, there's "no main thread in open or single
		// player."  So if it's not a SERVER_VERSION, GameServerEntryPoint() is
		// never called.  In that case, free the server here.
		CLT_VERSION_ONLY( sGameServerServerFree( svrContext ) );
		break;
	default:
		ASSERT_MSG("Server Runner command not yet implemented by GAME_SERVER.");
		break;
	}
#else
	REF(context);
	REF(command);
#endif
}


//----------------------------------------------------------------------------
// Specific Server Communication Abstraction Functions
// Used to hide any needed server-level multiplexing and such from
// game process.  As far as games are concerned, there is only
// one CHAT_SERVER, whose enum they do not know.
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL GameServerToChatServerSendMessage(
	MSG_STRUCT * msg,
	NET_CMD		 command)
{
	return (SvrNetSendRequestToService(ServerId(CHAT_SERVER, 0), msg, command)
		== SRNET_SUCCESS);
}
#endif

#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL GameServerToAuctionServerSendMessage(
	MSG_STRUCT * msg,
	NET_CMD		 command)
{
	return (SvrNetSendRequestToService(ServerId(AUCTION_SERVER, 0), msg, command)
		== SRNET_SUCCESS);
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL GameServerToBillingProxySendMessage(
	MSG_STRUCT * msg,
	NET_CMD		 command,
	SVRINSTANCE billingInstance)
{
	ASSERT_RETFALSE(msg && billingInstance != INVALID_SVRINSTANCE);
	return (SvrNetSendRequestToService(ServerId(BILLING_PROXY, billingInstance), msg, command)
		== SRNET_SUCCESS);
}
#endif

//----------------------------------------------------------------------------
// PLAYER_CLIENT_CONTEXT FUNCTIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Returns TRUE if he doesn't have an existing entry.
// Returns FALSE if we're replacing another login of the same account,
//	and handles replacement voodoo.
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sGameServerAddToAccountIdHash(
	GameServerContext *pServerContext,
	NETCLIENTID64 idNetClient,
	UNIQUE_ACCOUNT_ID idAccount)
{
	ACCOUNTID_HASH_ENTRY * pEntry = sAccountIdHash.GetOrAdd(idAccount);
	//if it's already mapped to another netclient, do a replacement instead of a new logon.
	NETCLIENTID64 toReplace = INVALID_NETCLIENTID64;
	ONCE
	{
		ASSERT_BREAK(pEntry);
		if(pEntry->idNetClient != 0)
		{
			toReplace = pEntry->idNetClient;
		}
		pEntry->idNetClient = idNetClient;
	}
	sAccountIdHash.Unlock(idAccount);

	ASSERT_RETTRUE(toReplace != idNetClient); //If this happens we're probably screwed; something is deeply wrong.

	if(toReplace != INVALID_NETCLIENTID64)
	{
		GAMEID idGameToReplace = NetClientGetGameId(toReplace);

		if(idGameToReplace == INVALID_ID || 
			!GameListQueryGameIsRunning(pServerContext->m_GameList, idGameToReplace) )
		{//Post a CCMD_PLAYERREMOVE to the app mailbox
			MSG_CCMD_PLAYERREMOVE removeMsg;
			SvrNetPostFakeClientRequestToMailbox(
				pServerContext->m_pPlayerToGameServerMailBox,
				toReplace, 
				&removeMsg,
				CCMD_PLAYERREMOVE);
			return TRUE; //We can ignore the old net client, we're kicking him.
		}
		else
		{//Post a APPCMD_TAKE_OVER_CLIENT to the game mailbox.

			MSG_APPCMD_TAKE_OVER_CLIENT takeoverMsg;
			takeoverMsg.idNetClientNew = idNetClient;
			takeoverMsg.idNetClientOld = toReplace;
			takeoverMsg.idAccount = idAccount;

			SERVER_MAILBOX * mailbox = GameListGetPlayerToGameMailbox(
				pServerContext->m_GameList,
				idGameToReplace );
			SvrNetPostFakeClientRequestToMailbox(
				mailbox,
				toReplace, 
				&takeoverMsg,
				APPCMD_TAKE_OVER_CLIENT);

			return FALSE; //It's a hostile takeover, baby!
		}
	}
    else
    {
        return TRUE;
    }
}
#endif

//----------------------------------------------------------------------------
// This is only for error handling purposes.  Completely nuke an entry.
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sGameServerRemoveFromAccountIdHash(
	GameServerContext *pServerContext,
	UNIQUE_ACCOUNT_ID idAccount)
{
	UNREFERENCED_PARAMETER(pServerContext);
	ACCOUNTID_HASH_ENTRY * pEntry = sAccountIdHash.Get(idAccount);
	if(pEntry)
	{
		sAccountIdHash.Remove(idAccount);
		return TRUE;
	}
	else
	{
		ASSERTV_MSG("Account %I64x client has no account id entry!",
			idAccount);
		sAccountIdHash.Unlock(idAccount);
		return FALSE;
	}
}
#endif
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static LPVOID sGameServerClientAttached(
	LPVOID svrContext,
	SERVICEUSERID attachedUser,
    BYTE *pRawAcceptData,
    DWORD dwAcceptDataLen)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
#if ISVERSION(SERVER_VERSION)
	BYTE bufAcceptData[MAX_SMALL_MSG_STRUCT_SIZE];
	MSG_ACCEPT_DATA_TO_GAME_SERVER * acceptData = 
		(MSG_ACCEPT_DATA_TO_GAME_SERVER *) bufAcceptData;
	ASSERT(SvrNetTranslateMessageForRecv(USER_SERVER, GAME_LOADBALANCE_SERVER, TRUE,
		pRawAcceptData, dwAcceptDataLen, acceptData) );
	ASSERT(acceptData->hdr.cmd == ACCEPT_DATA_TO_GAME_SERVER);
#else
	UNREFERENCED_PARAMETER(dwAcceptDataLen);
	AcceptDataFromLoginServer *acceptData;
	acceptData = (AcceptDataFromLoginServer *)pRawAcceptData;
#endif
	GameServerContext * serverContext = (GameServerContext*)svrContext;
	ASSERT_RETNULL(serverContext);

	NETCLT_USER * toRet = serverContext->m_clientFreeList.Get( serverContext->m_pMemPool, __FILE__, __LINE__ );
	ASSERT_RETNULL( toRet );

	//	init their context
	NetClientInitUserData( toRet );
    toRet->m_idAccount = acceptData->accountId;
	toRet->m_idNetClient = attachedUser;
#if ISVERSION(SERVER_VERSION)
	toRet->m_billingInstance = acceptData->billingInstance;
	PStrCopy(toRet->szCharacter, MAX_CHARACTER_NAME,
		acceptData->tCharacterInfo.szCharName, MAX_CHARACTER_NAME);
	toRet->m_netId = acceptData->netId;
	ASSERT(MemoryCopy(&toRet->m_ipAddress, sizeof(toRet->m_ipAddress),
		&acceptData->ipAddress, sizeof(SOCKADDR_STORAGE)) );
#endif

	BOOL bCleanLogin = sGameServerAddToAccountIdHash(
		serverContext, attachedUser, toRet->m_idAccount);

#if ISVERSION(SERVER_VERSION)
	if(bCleanLogin && acceptData->bSecondLogin)
	{//This is an error and possible race condition: second login wins,
		//but their other client logged off in the fraction of a second.
		//We boot them here to ensure proper database waiting.
		toRet->m_idAppClient = INVALID_ID;
		
		ASSERTV_MSG("Booting a client with account id %I64x attempting a second login but not found on the server.",
			acceptData->accountId);
		goto _ATTACH_ERROR;
	}
#endif

	if(bCleanLogin) SvrNetAttachRequestMailbox(
		attachedUser,
		serverContext->m_pPlayerToGameServerMailBox );
	
	if(bCleanLogin)//Create a new client.
	{
		//Read badges from blob in accept message.
		BADGE_COLLECTION tBadges;
#if ISVERSION(SERVER_VERSION)
		BIT_BUF badgeBuf(acceptData->badgeBuffer, MSG_GET_BLOB_LEN(acceptData, badgeBuffer));
		BadgeCollectionReadFromBuf(badgeBuf, tBadges);
#endif
		APPCLIENTID idAppClient = ClientSystemNewClient(
									serverContext->m_ClientSystem,
									(NETCLIENTID64)attachedUser,
									toRet->m_idAccount,
									tBadges, 
									acceptData->LoginTime,
									acceptData->bSubjectToFatigue,
									acceptData->iFatigueOnlineSecs,
									acceptData->gagUntilTime,
									serverContext->m_pMemPool);
		toRet->m_idAppClient = idAppClient;

#if ISVERSION(SERVER_VERSION)
		ClientSystemSetCharacterInfo(serverContext->m_ClientSystem,
			idAppClient, acceptData->tCharacterInfo);
#endif
		ASSERT_GOTO(idAppClient != INVALID_CLIENTID, _ATTACH_ERROR);
	}
#if ISVERSION(SERVER_VERSION)
	
	if(acceptData->bGameServerSwitch && bCleanLogin)
	{
		if(acceptData->idGame != INVALID_ID)
			ClientSystemSetServerWarpGame(
			serverContext->m_ClientSystem, toRet->m_idAppClient,
			acceptData->idGame, acceptData->tWarpSpec);
		
		//Fake a playernew message.
		MSG_CCMD_PLAYERNEW msg;
		PStrCopy(msg.szCharName, acceptData->tCharacterInfo.szCharName, MAX_CHARACTER_NAME);
		msg.bNewPlayer = FALSE;
		msg.bClass = BYTE(0);
		msg.qwBuildVersion = VERSION_AS_ULONGLONG;
		msg.nDifficulty = acceptData->tCharacterInfo.nDifficulty;
		msg.dwNewPlayerFlags = 0;

		ASSERTX(SvrNetPostFakeClientRequestToMailbox(
			serverContext->m_pPlayerToGameServerMailBox,
			attachedUser, 
			&msg,
			CCMD_PLAYERNEW),
			"Could not post message for newly attached user");		
	}
	else
	{
		ASSERTX(bCleanLogin || !acceptData->bGameServerSwitch, "Simultaneous client takeover and server switch, what gives?");
	}
#endif

	TraceInfo(TRACE_FLAG_GAME_NET,"TraceClientPath: New game client: AppClientID: %d, ServiceUserID %llu account id %I64x\n",
                                toRet->m_idAppClient, attachedUser,toRet->m_idAccount);
#if ISVERSION(SERVER_VERSION)
    TraceVerbose(TRACE_FLAG_GAME_NET,"Client connected from Address: %!IPADDR!", ((struct sockaddr_in*)&acceptData->ipAddress)->sin_addr.s_addr);
	serverContext->m_pPerfInstance->GameServerClientCount = InterlockedIncrement(&serverContext->m_clients);
	PERF_ADD64(serverContext->m_pPerfInstance,GameServer,GameServerAlltimeClientCount,1);
	if(serverContext->m_pPerfInstance->GameServerClientCount >
		serverContext->m_pPerfInstance->GameServerMaximumClientCount)
		serverContext->m_pPerfInstance->GameServerMaximumClientCount =
		serverContext->m_pPerfInstance->GameServerClientCount;

	SendGameLoadBalanceLogin(serverContext, toRet);
	if (PlayerActionLoggingIsEnabled())
	{
		TracePlayerAccountLogin(toRet->m_idAccount);
	}
#endif
	return toRet;

_ATTACH_ERROR:
	if(bCleanLogin) sGameServerRemoveFromAccountIdHash(serverContext, toRet->m_idAccount);
	ClientSystemRemoveClient( serverContext->m_ClientSystem, toRet->m_idAppClient );
	serverContext->m_clientFreeList.Free( serverContext->m_pMemPool, toRet );
	SvrNetDisconnectClient( attachedUser );
#else
	REF(svrContext);
	REF(dwAcceptDataLen);
	REF(pRawAcceptData);
	REF(attachedUser);
#endif
	return NULL;
}


//----------------------------------------------------------------------------
//does not remove from game.  should only be called by s_network handlers.
//If you want to remove a player from a game, send a PLAYERREMOVE message.
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void GameServerNetClientRemove(
	NETCLIENTID64 idNetClient)
{
	GameServerContext * pServerContext = (GameServerContext*)CurrentSvrGetContext();
	ASSERT_RETURN(pServerContext);

	TraceInfo(TRACE_FLAG_GAME_NET,"Remove called for idNetClient %#018I64x\n", idNetClient);

	NETCLT_USER * pPlayerContext = (NETCLT_USER *)SvrNetGetClientContext(idNetClient);
	if(!pPlayerContext) 
	{
		SvrNetDisconnectClient(idNetClient);
		return;
	}
	ACCOUNTID_HASH_ENTRY * pEntry = sAccountIdHash.Get(pPlayerContext->m_idAccount);
	if(pEntry)
	{
		if(pEntry->idNetClient == idNetClient)
			sAccountIdHash.Remove(pPlayerContext->m_idAccount);
		else //someone has probably replaced him. (second login takes over)
			sAccountIdHash.Unlock(pPlayerContext->m_idAccount);
	}
	else
	{
		ASSERT_MSG("Net client has no account id entry!");
		sAccountIdHash.Unlock(pPlayerContext->m_idAccount);
	}

	if(pPlayerContext->m_idAppClient != INVALID_ID)
	{
		CHANNELID partyChannel = ClientSystemGetParty( pServerContext->m_ClientSystem, pPlayerContext->m_idAppClient );
		if( partyChannel != INVALID_CHANNELID )
		{
			PartyCacheRemovePartyMember(
				pServerContext->m_PartyCache,
				partyChannel,
				ClientSystemGetGuid(
				pServerContext->m_ClientSystem,
				pPlayerContext->m_idAppClient ) );
		}
	}
	APPCLIENTID idAppClient;
	pPlayerContext->m_cReadWriteLock.WriteLock();
	idAppClient = pPlayerContext->m_idAppClient;

	pPlayerContext->m_idAppClient = INVALID_ID; 
	pPlayerContext->m_bRemoveCalled = TRUE;

	pPlayerContext->m_cReadWriteLock.EndWrite();

	if(idAppClient != INVALID_ID)
		ClientSystemRemoveClient(pServerContext->m_ClientSystem,
		idAppClient );

	SvrNetDisconnectClient( idNetClient );
#if ISVERSION(SERVER_VERSION)
	pServerContext->m_pPerfInstance->GameServerClientCount = InterlockedDecrement(&pServerContext->m_clients);

	SendGameLoadBalanceLogout(pServerContext, pPlayerContext);
#endif
	//NetClientFreeUserData(pPlayerContext);
	//pServerContext->m_clientFreeList.Free( pPlayerContext );
	//Instead, add pPlayerContext to the list of pointers to be freed at the end of a message processing loop.
	pServerContext->m_clientFinishedList[pServerContext->m_bListSwap].Push(pPlayerContext);
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sGameServerBootNetClient(
	GameServerContext * pServerContext,
	NETCLT_USER * pPlayerContext)
{
	ASSERT_RETFALSE(pServerContext);
	ASSERT_RETFALSE(pPlayerContext);

	//check if he's in a game. 
	pPlayerContext->m_cReadWriteLock.WriteLock();
	pPlayerContext->m_bDetachCalled = TRUE;
	GAMEID idGame = pPlayerContext->m_idGame;
	pPlayerContext->m_cReadWriteLock.EndWrite();

	SERVER_MAILBOX * mailbox = NULL;
	if (idGame != INVALID_ID)
	{
		// post a message to the game that the client has disconnected
		mailbox = GameListGetPlayerToGameMailbox(
			pServerContext->m_GameList,
			idGame );
	}
	else
	{
		mailbox = pServerContext->m_pPlayerToGameServerMailBox;
	}

	{
		MSG_CCMD_PLAYERREMOVE msg;
		if (mailbox && 
			!SvrNetPostFakeClientRequestToMailbox(
			mailbox,
			pPlayerContext->m_idNetClient, 
			&msg,
			CCMD_PLAYERREMOVE))
		{
			TraceInfo(TRACE_FLAG_GAME_NET,"Could not post disconnect for client %lli"
				" in game %lli",pPlayerContext->m_idNetClient,idGame);
			return FALSE;
		}
	}
	return TRUE;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sGameServerGagNetClient(
	GameServerContext * pServerContext,
	NETCLT_USER * pPlayerContext,
	GAG_ACTION eGagAction)
{
	ASSERT_RETFALSE(pServerContext);
	ASSERT_RETFALSE(pPlayerContext);

	//check if he's in a game. 
	pPlayerContext->m_cReadWriteLock.ReadLock();
	GAMEID idGame = pPlayerContext->m_idGame;
	pPlayerContext->m_cReadWriteLock.EndRead();

	SERVER_MAILBOX * mailbox = NULL;
	if (idGame != INVALID_ID)
	{
		mailbox = GameListGetPlayerToGameMailbox(
			pServerContext->m_GameList,
			idGame );
	}
	else
	{
		mailbox = pServerContext->m_pPlayerToGameServerMailBox;
	}

	{
	
		// setup message
		MSG_APPCMD_PLAYERGAG msg;
		msg.nGagAction = eGagAction;

		// send		
		if (mailbox && 
			!SvrNetPostFakeClientRequestToMailbox(
			mailbox,
			pPlayerContext->m_idNetClient, 
			&msg,
			APPCMD_PLAYERGAG))
		{
			TraceInfo(TRACE_FLAG_GAME_NET,"Could not post gag for client %lli"
				" in game %lli",pPlayerContext->m_idNetClient,idGame);
			return FALSE;
		}
	}
	return TRUE;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sGameServerClientDetached(
	LPVOID				svrContext,
	SERVICEUSERID		idClient,
	LPVOID				cltContext )
{	
#if !ISVERSION(CLIENT_ONLY_VERSION)
	//Client clearing functionality moved to GameServerNetClientRemove below, called by message handler
	TraceInfo(TRACE_FLAG_GAME_NET,"Game client detached: ServiceUserID: %llu\n", idClient);

	GameServerContext * pServerContext = (GameServerContext*)svrContext;
	NETCLT_USER * pPlayerContext = (NETCLT_USER*)cltContext;

#if ISVERSION(SERVER_VERSION)
	if(pServerContext) PERF_ADD64(pServerContext->m_pPerfInstance,GameServer,
		GameServerClientDetachCount,1);
#endif

	sGameServerBootNetClient(pServerContext, pPlayerContext);
#else
	REF(svrContext);
	REF(idClient);
	REF(cltContext);
#endif
}

#if !ISVERSION(CLIENT_ONLY_VERSION)
NETCLIENTID64 GameServerGetNetClientId64FromAccountId(
	UNIQUE_ACCOUNT_ID idAccount)
{
	ACCOUNTID_HASH_ENTRY * pEntry = sAccountIdHash.Get(idAccount);
	NETCLIENTID64 idNetClient = pEntry ? pEntry->idNetClient : INVALID_NETCLIENTID64;
	sAccountIdHash.Unlock(idAccount);
	return idNetClient;
}
#endif // !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL GameServerBootAccountId(
	GameServerContext * pServerContext,
	UNIQUE_ACCOUNT_ID idAccount)
{
	ASSERT_RETFALSE(pServerContext);

	NETCLIENTID64 idNetClient = GameServerGetNetClientId64FromAccountId(idAccount);

	if (idNetClient == INVALID_NETCLIENTID64)
	{
		// seems like this should not be an error, they could have logged off -cday
		TraceError("Received boot request for nonexistent account %I64x", idAccount);
	}
	else
	{
		NETCLT_USER * pPlayerContext = (NETCLT_USER *)SvrNetGetClientContext(idNetClient);
		if(pPlayerContext)
			return sGameServerBootNetClient(pServerContext, pPlayerContext);
		else
			TraceInfo(TRACE_FLAG_GAME_NET, 
			"Could not find player context for boot with account %I64x with netclient ID %I64x",
			idAccount, idNetClient);
	}

	return FALSE;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL GameServerGagAccountId(
	GameServerContext * pServerContext,
	UNIQUE_ACCOUNT_ID idAccount,
	GAG_ACTION eGagAction)
{
	ASSERT_RETFALSE(pServerContext);

	NETCLIENTID64 idNetClient = GameServerGetNetClientId64FromAccountId(idAccount);

	if (idNetClient == INVALID_NETCLIENTID64)
	{
		// seems like this should not be an error, they could have logged off -cday
		TraceError("Received gag request for nonexistent account %I64x", idAccount);
	}
	else
	{
		NETCLT_USER * pPlayerContext = (NETCLT_USER *)SvrNetGetClientContext(idNetClient);
		if(pPlayerContext)
			return sGameServerGagNetClient(pServerContext, pPlayerContext, eGagAction);
		else
			TraceInfo(TRACE_FLAG_GAME_NET, 
			"Could not find player context for boot with account %I64x with netclient ID %I64x",
			idAccount, idNetClient);
	}

	return FALSE;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(SERVER_VERSION)
APPCLIENTID GameServerGetAuctionHouseAppClientID(
	GameServerContext* pSvrCtx)
{
	UNREFERENCED_PARAMETER(pSvrCtx);
	return APPCLIENTID_AUCTION_SYSTEM;
/*
	ASSERT_RETVAL(pSvrCtx != NULL, INVALID_ID);

	GAMEID idUtilGame = UtilityGameGetGameId(pSvrCtx);
	ASSERT_RETVAL(idUtilGame != INVALID_ID, INVALID_CLIENTID);

	CSAutoLock autoLock(&pSvrCtx->m_csUtilGameInstance);

	BADGE_COLLECTION badgeCollection;
	if (pSvrCtx->m_idAppCltAuctionHouse == INVALID_CLIENTID) {
		pSvrCtx->m_idAppCltAuctionHouse = ClientSystemNewClient(
			pSvrCtx->m_ClientSystem,
			ServiceUserId(ServerId(USER_SERVER, 0), INVALID_SVRCONNECTIONID), //Is this okay?
			INVALID_UNIQUE_ACCOUNT_ID,
			badgeCollection,
			(time_t)0,
			FALSE,
			0,
			(time_t)0);
		ASSERT_RETVAL(pSvrCtx->m_idAppCltAuctionHouse != INVALID_CLIENTID, INVALID_CLIENTID);

//		ClientSystemSetClientGame(pSvrCtx->m_ClientSystem, pSvrCtx->m_idAppCltAuctionHouse, idUtilGame, AUCTION_OWNER_NAME);


	}
	return pSvrCtx->m_idAppCltAuctionHouse;
	*/
}

UNIQUE_ACCOUNT_ID GameServerGetAuctionHouseAccountID(
	GameServerContext* pSvrCtx)
{
	return pSvrCtx->m_idAccountAuctionHouse;
}

PGUID GameServerGetAuctionHouseCharacterID(
	GameServerContext* pSvrCtx)
{
	return pSvrCtx->m_idCharAuctionHouse;
}

#endif

//----------------------------------------------------------------------------
// GAME SERVER MESSAGE TABLE INIT FUNCTIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// COMMAND HANDLER
//
// There are legitimate reasons we may want to disconnect a client from
// any mailbox and ignore him, until the game or app loop gets to processing
// him.  In this case, we'll trace his commands and quietly do nothing.
//----------------------------------------------------------------------------
static void sUserServerToGameServerRequestCmdTrace(
	LPVOID svrContext,
	SERVICEUSERID sender, 
	MSG_STRUCT * msg,
	LPVOID cltContext)
{
	GameServerContext * pServerContext = (GameServerContext*)svrContext;
	NETCLT_USER * pPlayerContext = (NETCLT_USER*)cltContext;

	TraceExtraVerbose(TRACE_FLAG_GAME_NET,"Mailboxless client %llu sent client command %d\n", sender, msg->hdr.cmd);

	if(!pServerContext) return;
	if(!pPlayerContext) 
	{
		TraceInfo(TRACE_FLAG_GAME_NET,"Disconnecting contextless client %llu\n", sender);
		SvrNetDisconnectClient(sender);
		return;
	}
}


//----------------------------------------------------------------------------
// On responses from userserver to switch messages: we do nothing.
//----------------------------------------------------------------------------
BOOL sGameServerOnRunnerRead(void *context, 
							   SVRID sender,
							   BYTE *pBuffer,
							   DWORD dwBufSize)
{
	UNREFERENCED_PARAMETER(context);
	UNREFERENCED_PARAMETER(sender);
	UNREFERENCED_PARAMETER(pBuffer);
	UNREFERENCED_PARAMETER(dwBufSize);

	return FALSE;

}
//----------------------------------------------------------------------------
// Direct callback table, these should not occur.
//----------------------------------------------------------------------------
FP_NET_REQUEST_COMMAND_HANDLER sCMessageAssertTable[] =
//#define _C_MESSAGE_H_
#undef _C_MESSAGE_ENUM_H_
#undef NET_MSG_TABLE_BEGIN
#undef NET_MSG_TABLE_DEF
#undef NET_MSG_TABLE_END
#define NET_MSG_TABLE_BEGIN					{
#define NET_MSG_TABLE_DEF(e, s, r)			sUserServerToGameServerRequestCmdTrace,
#define NET_MSG_TABLE_END(e)				};
#include "c_message.h"

extern FP_NET_RESPONSE_COMMAND_HANDLER sChatToGameResponseHandlers[];
#if ISVERSION(SERVER_VERSION)
extern FP_NET_RESPONSE_COMMAND_HANDLER sLoadBalanceToGameResponseHandlers[];
extern FP_NET_RESPONSE_COMMAND_HANDLER BillingToGameServerResponseHandlers[];
extern FP_NET_RESPONSE_COMMAND_HANDLER sBridgeToGameResponseHandlers[];
extern FP_NET_RESPONSE_COMMAND_HANDLER sBattleToGameResponseHandlers[];
extern FP_NET_RESPONSE_COMMAND_HANDLER sGlobalGameEventToGameResponseHandlers[];
extern FP_NET_RESPONSE_COMMAND_HANDLER sAuctionToGameResponseHandlers[];
#endif // ISVERSION(SERVER_VERSION)


//----------------------------------------------------------------------------
// GAME SERVER CONNECTION TABLE
//----------------------------------------------------------------------------


CONNECTION_TABLE_INIT_FUNC_START( GAME_SERVER )
	REQUIRE_SERVICE(CHAT_SERVER, sChatToGameResponseHandlers, NULL)
	OFFER_SERVICE(USER_SERVER, sCMessageAssertTable,
							c_NetGetCommandTable, s_NetGetCommandTable, 
							sGameServerClientAttached, sGameServerClientDetached )
#if ISVERSION(SERVER_VERSION)
	REQUIRE_SERVICE(GAME_LOADBALANCE_SERVER, sLoadBalanceToGameResponseHandlers, NULL)
	REQUIRE_SERVICE(BILLING_PROXY, BillingToGameServerResponseHandlers, 0);
	REQUIRE_SERVICE(CSR_BRIDGE_SERVER, sBridgeToGameResponseHandlers, 0);
	REQUIRE_SERVICE(BATTLE_SERVER, sBattleToGameResponseHandlers, NULL);
	REQUIRE_SERVICE(GLOBAL_GAME_EVENT_SERVER, sGlobalGameEventToGameResponseHandlers, NULL);
	REQUIRE_SERVICE(AUCTION_SERVER, sAuctionToGameResponseHandlers, NULL);
#endif // ISVERSION(SERVER_VERSION)
CONNECTION_TABLE_INIT_FUNC_END


//----------------------------------------------------------------------------
// GAME SERVER SPECIFICATION
//----------------------------------------------------------------------------
SERVER_SPECIFICATION SERVER_SPECIFICATION_NAME( GAME_SERVER ) =
{
	GAME_SERVER,                                 //svrType,
	"GAME_SERVER",                               //svrName,
	GameServerServerInit,
	SVR_VERSION_ONLY(SVRCONFIG_CREATOR_FUNC(GameServerCommands))
	CLT_VERSION_ONLY(NULL),		//	no config struct in open or single player.
	SVR_VERSION_ONLY( GameServerEntryPoint )
	CLT_VERSION_ONLY( NULL ),	//	no main thread in open or single player.
	GameServerCommandCallback,
	NULL,	//	no xml message handling
	CONNECTION_TABLE_INIT_FUNC_NAME( GAME_SERVER )
};
