// ---------------------------------------------------------------------------
// FILE:	gamelist.cpp
// DESC:	handles lists of games
//
//  Copyright 2003, Flagship Studios
// ---------------------------------------------------------------------------
#include "stdafx.h"
#include "prime.h"
#include "game.h"
#include "gamelist.h"
#include "rotlist.h"
#include "mailslot.h"
#include "minitown.h"
#include "svrstd.h"
#include "GameServer.h"
#include "clients.h"
#include "dungeon.h"
//#include "EmbeddedServerContext.h"
#ifdef HAVOK_ENABLED
#include "havok.h"
#endif
#if ISVERSION(SERVER_VERSION)
#include "gamelist.cpp.tmh"
#endif


// ---------------------------------------------------------------------------
// CONSTANTS
// ---------------------------------------------------------------------------
#define GAME_SERVER_PROCESS_PRIORITY								THREAD_PRIORITY_NORMAL			// worker thread priority
#define GAME_SERVER_CRAP_OUT_TIME									INFINITE
#define GAME_SERVER_EXCEPTION_HANDLING								0				
#define GAME_SERVER_MT												1


// ---------------------------------------------------------------------------
// STRUCTS
// ---------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
struct GAMEID_HASH_ENTRY
{
	GAMEID id;
	GAMEID_HASH_ENTRY *next;
};

struct GAMELIST
{
	struct MEMORYPOOL *												m_pool;

	struct GAME *													m_Games[MAX_GAMES_PER_SERVER];								// array of games
	CCriticalSection												m_csGames[MAX_GAMES_PER_SERVER];							// critical sections
	struct SERVER_MAILBOX*											m_pPlayerToServerMailboxes[MAX_GAMES_PER_SERVER];			// mailboxes for each game

	CRotList_MT<unsigned int, MAX_GAMES_PER_SERVER, INVALID_ID>		m_FreeList;													// list of free indexes into m_Games
	
#if DECOUPLE_GAME_LIST
	CRotList_MT<GAMEID, MAX_GAMES_PER_SERVER, INVALID_ID>			m_WorkQueue;												// list of games to be processed
#else	
	CRotList_MT<GAMEID, MAX_GAMES_PER_SERVER, INVALID_ID>			m_ProcessList;												// list of games to be processed
	CRotList_MT<GAMEID, MAX_GAMES_PER_SERVER, INVALID_ID>			m_FinishedList;												// list of games done processing
#endif

	CS_HASH<GAMEID_HASH_ENTRY, GAMEID, MAX_GAMES_PER_SERVER>		m_ActiveHash;												// hash of all live games by id

	volatile LONG													m_nIter;													// iter value

	BOOL															m_bQuit;
	unsigned int													m_GameProcessThreads;										// number of game processing threads desired
	volatile LONG													m_GameProcessThreadsToCreate;								// number of game processing threads we need to create
	volatile LONG													m_GameProcessThreadsActive;									// active game processing threads

	volatile LONG													m_GameProcessThreadsRequested;								// set to m_GameProcessThreadsActive when it's time to start processing games
	volatile LONG													m_SimFramesToProcess;										// number of frames to process
	HANDLE															m_hGoEvent[2];												// set by main thread when it's time to start processing games
	HANDLE															m_hDoneEvent;												// set by worker thread when finished processing games
	unsigned int													m_nGoEventIndex;
};
#endif


// ---------------------------------------------------------------------------
// Externs and forward declarations
// ---------------------------------------------------------------------------
extern void (*pTimestampResetFunc)(void);
void GameListResetLastProcessTimestamp(void);


// ---------------------------------------------------------------------------
// FUNCTIONS
// ---------------------------------------------------------------------------
//----------------------------------------------------------------------------
// returns a game by id (keeps game locked)
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static GAME * sGameListGetGameData(
	GAMELIST * glist,
	GAMEID id)
{
	if (id == INVALID_ID)
	{
		return NULL;
	}
	unsigned int ii = GameIdGetIndex(id);
	ASSERT_RETNULL(ii < MAX_GAMES_PER_SERVER);

	glist->m_csGames[ii].Enter();
	ONCE
	{
		GAME * game = game = glist->m_Games[ii];
		if (!game)
		{
			break;
		}
		if (game->m_idGame != id)
		{
			break;
		}
		return game;
	}
	glist->m_csGames[ii].Leave();
	return NULL;
}
#endif


//----------------------------------------------------------------------------
// unlocks a game
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void GameListReleaseGameData(
	GAMELIST * glist,
	GAME * & game)
{
	ASSERT_RETURN(game);
	unsigned int ii = GameIdGetIndex(game->m_idGame);
	ASSERT_RETURN(ii < MAX_GAMES_PER_SERVER);

	glist->m_csGames[ii].Leave();
	game = NULL;
}
#endif


// ---------------------------------------------------------------------------
// kill game processing threads
// ---------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static void sGameListKillWorkerThreads(
	GAMELIST * glist)
{
	ASSERT_RETURN(glist);

#if GAME_SERVER_MT
	if (glist->m_GameProcessThreadsActive == 0)
	{
		goto _closehandles;
	}
	ASSERT(glist->m_GameProcessThreadsRequested == 0);
	glist->m_GameProcessThreadsRequested = glist->m_GameProcessThreadsActive;
	glist->m_bQuit = TRUE;
	glist->m_SimFramesToProcess = 0;
	glist->m_nGoEventIndex = !glist->m_nGoEventIndex;
	SetEvent(glist->m_hGoEvent[glist->m_nGoEventIndex]);
	
	DWORD ret = WaitForSingleObject(glist->m_hDoneEvent, GAME_SERVER_CRAP_OUT_TIME);
	if (ret == WAIT_TIMEOUT)
	{
		return;
	}
	ASSERT(glist->m_GameProcessThreadsActive == 0);

_closehandles:
	for (unsigned int ii = 0; ii < 2; ++ii)
	{
		if (glist->m_hGoEvent[ii])
		{
			CloseHandle(glist->m_hGoEvent[ii]);
			glist->m_hGoEvent[ii] = NULL;
		}
	}
	if (glist->m_hDoneEvent)
	{
		CloseHandle(glist->m_hDoneEvent);
		glist->m_hDoneEvent = NULL;
	}
#endif
}
#endif


// ---------------------------------------------------------------------------
// frees game list
// ---------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void GameListFree(
	GAMELIST * & glist)
{
	if (!glist)
	{
		return;
	}

	sGameListKillWorkerThreads(glist);

	for (unsigned int ii = 0; ii < MAX_GAMES_PER_SERVER; ii++)
	{
		GAME * game = glist->m_Games[ii];
		if (game)
		{
			MemorySetThreadPool(GameGetMemPool(game));
			GameFree(glist->m_Games[ii]);
			MemorySetThreadPool(NULL);
		}
		glist->m_csGames[ii].Free();
		SvrMailboxRelease(glist->m_pPlayerToServerMailboxes[ii]);	// decrement refcount
		glist->m_Games[ii] = NULL;
	}

	glist->m_FreeList.Free(glist->m_pool);
	
#if DECOUPLE_GAME_LIST	
	glist->m_WorkQueue.Free(glist->m_pool);
#else
	glist->m_ProcessList.Free(glist->m_pool);
	glist->m_FinishedList.Free(glist->m_pool);
#endif
	
	glist->m_ActiveHash.Destroy(glist->m_pool);

	FREE(glist->m_pool, glist);
	glist = NULL;
}
#endif


//----------------------------------------------------------------------------
// recycle a game id
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static void GameListRecycleGameId(
	GAMELIST * glist,
	GAMEID id)
{
	ASSERT_RETURN(glist);

	glist->m_FreeList.Push(GameIdGetIndex(id));
}
#endif


//----------------------------------------------------------------------------
// DESC:	free a game from the game list
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void GameListFreeGame(
	GAMELIST * glist,
	GAME * game)
{
	ASSERT(game);
	GAMEID idGame = GameGetId(game);
	
	GameServerContext * serverContext = (GameServerContext*)CurrentSvrGetContext();
	ASSERT_RETURN(serverContext);

	glist->m_ActiveHash.Remove(idGame);

	MiniTownFreeGameId(serverContext->m_MiniTown, idGame);
	
	glist->m_Games[GameIdGetIndex(idGame)] = NULL;
	GAME * tofree = game;
	GameListReleaseGameData(glist, game);

	//Currently, empty out chris's mailbox.
	SvrMailboxEmptyMailbox(glist->m_pPlayerToServerMailboxes[GameIdGetIndex(idGame)]);
	SvrMailboxFreeMailboxBuffers(glist->m_pPlayerToServerMailboxes[GameIdGetIndex(idGame)]);
	//In future, possibly, delete it in a way that makes sure all clients are properly severed.

	GameFree(tofree);
	MemorySetThreadPool(NULL);

	GameListRecycleGameId(glist, idGame);
}
#endif

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION) && GAME_SERVER_MT
struct GAME_SERVER_THREAD_INFO
{
	unsigned int			index;
	struct MEMORYPOOL *		pool;
	GAMELIST *				glist;
#if ISVERSION(SERVER_VERSION)
	struct PERFCOUNTER_INSTANCE_GameServerThread *	m_pPerfInstance;
#endif
};
#endif

// ---------------------------------------------------------------------------
// grab games off the process list (one by one), process them, and put them on the finished list
// ---------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static void sGameListProcessList(
	GAMELIST * glist,
	unsigned int sim_frames,
	struct GAME_SERVER_THREAD_INFO *pThreadInfo = NULL)
{
#if ISVERSION(SERVER_VERSION)
	if(pThreadInfo && pThreadInfo->m_pPerfInstance)
	{
		pThreadInfo->m_pPerfInstance->GameServerThreadMaxSimultaneousTicks =
			pThreadInfo->m_pPerfInstance->GameServerThreadGameTicks = 0;
	}
#else
	REF(pThreadInfo);
#endif

	ASSERT_RETURN(glist);
	while (TRUE)
	{
#if DECOUPLE_GAME_LIST
		GAMEID idGame = glist->m_WorkQueue.Pop();
#else	
		GAMEID idGame = glist->m_ProcessList.Pop();
#endif		
		
		// If there are no games to process
		//
		if (idGame == INVALID_ID)
		{
#if DECOUPLE_GAME_LIST			
			Sleep(MSECS_PER_GAME_TICK);
#endif		
		
			break;
		}
		GAME * game = sGameListGetGameData(glist, idGame);
		ASSERT(game);
		if (!game)
		{
			continue;
		}

		MemorySetThreadPool(GameGetMemPool(game));
/*
#if ISVERSION(SERVER_VERSION)
		if(ClientListGetClientCount(game->m_pClientList) > 1) 
			server_trace("Game %I64x has reserve count %d and client count %d",
			idGame, game->m_nReserveCount, ClientListGetClientCount(game->m_pClientList));
#endif*/
		// NOTE: game is locked, must call GameListReleaseGameData()
#if GAME_SERVER_EXCEPTION_HANDLING && ISVERSION(SERVER_VERSION)	
		__try
		{	
#endif	
		int nFramesSimulated = SrvGameSimulate(game, sim_frames);
#if ISVERSION(SERVER_VERSION)
		if(pThreadInfo && pThreadInfo->m_pPerfInstance)
		{
			PERF_ADD64(pThreadInfo->m_pPerfInstance, GameServerThread,
				GameServerThreadGameTickRate, nFramesSimulated);
			PERF_ADD64(pThreadInfo->m_pPerfInstance, GameServerThread,
				GameServerThreadGameTicks, nFramesSimulated);
			pThreadInfo->m_pPerfInstance->GameServerThreadMaxSimultaneousTicks
				= MAX(pThreadInfo->m_pPerfInstance->GameServerThreadMaxSimultaneousTicks,
				(ULONGLONG)nFramesSimulated);
		}
#endif
#if DECOUPLE_GAME_LIST
		if (nFramesSimulated > 2)
		{
			TraceError("Simulating %d ticks at once for game %I64x.  Lag!\n",
				nFramesSimulated, idGame);
		}
#else
		REF(nFramesSimulated);		
#endif
#if GAME_SERVER_EXCEPTION_HANDLING && ISVERSION(SERVER_VERSION)			
		}
		// If memory corruption has occurred while processing a game, mark it
		// as invalid and continue
		//	
		__except(GetExceptionCode() == EXCEPTION_CODE_MEMORY_CORRUPTION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
		{
			// Send exit game messages to all clients in the game
			//
			ClientListExitClients(game);

			// Signal the game to shutdown
			//
			GameSetState(game, GAMESTATE_REQUEST_SHUTDOWN);	
		}	
#endif			
		if (game->m_nReserveCount <= 0 && game->tiGameTick > 10 * GAME_TICKS_PER_SECOND && ClientListGetClientCount(game->m_pClientList) == 0)
		{
			GameSetState(game, GAMESTATE_REQUEST_SHUTDOWN);
		}
		if (GameGetState(game) == GAMESTATE_REQUEST_SHUTDOWN)
		{
			GameListFreeGame(glist, game);
		}
		else
		{
			GameListReleaseGameData(glist, game);
			
#if DECOUPLE_GAME_LIST
			glist->m_WorkQueue.Push(idGame);
#else
			glist->m_FinishedList.Push(idGame);
#endif
		}

		MemorySetThreadPool(NULL);

#if DECOUPLE_GAME_LIST	
	
		if (glist->m_bQuit || glist->m_GameProcessThreadsActive == 0 || nFramesSimulated == 0)
		{
			break;
		}
#endif				
	}
}
#endif

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION) && GAME_SERVER_MT
DWORD WINAPI GameServerThread(
	LPVOID param)    
{
	ASSERT_RETZERO(param);
	GAME_SERVER_THREAD_INFO * info = (GAME_SERVER_THREAD_INFO *)param;
	ASSERT_RETZERO(info->glist);
	GAMELIST * glist = info->glist;

	InterlockedDecrement(&glist->m_GameProcessThreadsToCreate);

	ASSERT_RETZERO(glist->m_hGoEvent[0] != NULL);
	ASSERT_RETZERO(glist->m_hGoEvent[1] != NULL);
	ASSERT_RETZERO(glist->m_hDoneEvent != NULL);

#if ISVERSION(DEBUG_VERSION)
	char thread_name[256];
	PStrPrintf(thread_name, 256, "gsv_%02d", info->index);
	SetThreadName(thread_name);
	trace("info: %16x  begin game server thread [%s]\n", info, thread_name);
#endif

#if ISVERSION(SERVER_VERSION)
	WCHAR instanceName[MAX_PERF_INSTANCE_NAME];
	swprintf(instanceName,sizeof(instanceName),L"GameServerThread_%02d", info->index);
	info->m_pPerfInstance = PERF_GET_INSTANCE(GameServerThread, instanceName);
#endif

	MemoryInitThreadPool();

#ifdef HAVOK_ENABLED
	HAVOK_THREAD_DATA tHavokThread;

	if (AppIsHellgate())
	{
		ZeroMemory( &tHavokThread, sizeof( HAVOK_THREAD_DATA ) );
		tHavokThread.pMemoryPool = info->pool;
		HavokThreadInit( tHavokThread );
	}
#endif

	InterlockedIncrement(&glist->m_GameProcessThreadsActive);
	
#if !DECOUPLE_GAME_LIST	
	unsigned int nGoEventIndex = 0;
#endif

	while (TRUE)
	{
#if !DECOUPLE_GAME_LIST	
		nGoEventIndex = !nGoEventIndex;
		DWORD ret = WaitForSingleObject(glist->m_hGoEvent[nGoEventIndex], GAME_SERVER_CRAP_OUT_TIME);
		if (ret == WAIT_TIMEOUT)
		{
			break;
		}
#endif		
		
		if (glist->m_bQuit)
		{
			break;
		}

#if !DECOUPLE_GAME_LIST
		ASSERT(glist->m_SimFramesToProcess > 0);
#endif

		unsigned int sim_frames = glist->m_SimFramesToProcess;
		sGameListProcessList(glist, sim_frames, info);

#if !DECOUPLE_GAME_LIST
		if (InterlockedDecrement(&glist->m_GameProcessThreadsRequested) == 0)
		{
			ResetEvent(glist->m_hGoEvent[nGoEventIndex]);
			SetEvent(glist->m_hDoneEvent);
		}
#else
		Sleep(1);	//If we're running decoupled, we sleep directly instead of waiting for the main thread, sleeping dependent on it.
#endif		
	}

#if ISVERSION(DEBUG_VERSION)
	trace("exit game server thread [%s]\n", thread_name);
#endif

#ifdef HAVOK_ENABLED
	if(AppIsHellgate())
	{
		HavokThreadClose( tHavokThread );
		MemoryFreeThreadPool();
	}
#endif
#if ISVERSION(SERVER_VERSION)
	PERF_FREE_INSTANCE(info->m_pPerfInstance);
#endif
	FREE(info->pool, info);

	if (InterlockedDecrement(&glist->m_GameProcessThreadsActive) == 0)
	{
		SetEvent(glist->m_hDoneEvent);
	}
	return 0;
}
#endif


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION) && GAME_SERVER_MT
static BOOL sSpawnGameServerThread(
	struct MEMORYPOOL * pool,
	GAMELIST * glist,
	unsigned int ii)
{
	ASSERT_RETFALSE(glist);

	GAME_SERVER_THREAD_INFO * info = (GAME_SERVER_THREAD_INFO *)MALLOCZ(pool, sizeof(GAME_SERVER_THREAD_INFO));
	info->pool = pool;
	info->index = ii;
	info->glist = glist;

	InterlockedIncrement(&glist->m_GameProcessThreadsToCreate);

	HANDLE hWorker = SvrRunnerCreateThread(GAME_STACK_SIZE, GameServerThread, (void *)info);
	SetThreadIdealProcessor(hWorker, ii);
	
	ASSERT_RETFALSE(hWorker != NULL);
	SetThreadPriority(hWorker, GAME_SERVER_PROCESS_PRIORITY);
	//CloseHandle(hWorker);
	return TRUE;
}
#endif


// ---------------------------------------------------------------------------
// open server
// ---------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION) && !ISVERSION(CLIENT_ONLY_VERSION)
static BOOL sCreateGameServerThreads_ClientVersion(
	struct MEMORYPOOL * pool,
	GAMELIST * glist)
{
	ASSERT_RETFALSE(glist);
	ASSERT_RETFALSE(glist->m_GameProcessThreads == 0);
	glist->m_GameProcessThreads = 0;
	return TRUE;
}
#endif


// ---------------------------------------------------------------------------
// closed server
// ---------------------------------------------------------------------------
#if ISVERSION(SERVER_VERSION) && !ISVERSION(CLIENT_ONLY_VERSION) && GAME_SERVER_MT
static BOOL sCreateGameServerThreads_ServerVersion(
	struct MEMORYPOOL * pool,
	GAMELIST * glist)
{
	ASSERT_RETFALSE(glist);
	ASSERT_RETFALSE(glist->m_GameProcessThreads == 0);

	BOOL QuickStartDesktopServerIsEnabled(void);
	if (QuickStartDesktopServerIsEnabled())
	{
		glist->m_GameProcessThreads = 0;
		return TRUE;
	}

	CPU_INFO info;
	ASSERT_RETFALSE(CPUInfo(info));
	ASSERT_RETFALSE(info.m_AvailCore > 0);
	glist->m_GameProcessThreads = info.m_AvailCore;
	if(glist->m_GameProcessThreads < 1) glist->m_GameProcessThreads = 1;

	for (unsigned int ii = 0; ii < glist->m_GameProcessThreads; ++ii)
	{
		ASSERT_RETFALSE(sSpawnGameServerThread(pool, glist, ii));
	}

	while (glist->m_GameProcessThreadsActive != (LONG)glist->m_GameProcessThreads)
	{
		Sleep(10);
	}
	return TRUE;
}
#endif


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION) && GAME_SERVER_MT
static BOOL sCreateGameServerThreads(
	struct MEMORYPOOL * pool,
	GAMELIST * & glist)
{
	for (unsigned int ii = 0; ii < 2; ++ii)
	{
		glist->m_hGoEvent[ii] = CreateEvent(NULL, TRUE, FALSE, NULL);	// manual reset
		ASSERT_RETFALSE(glist->m_hGoEvent[ii] != NULL);
	}

	glist->m_hDoneEvent = CreateEvent(NULL, FALSE, FALSE, NULL);	// auto reset
	ASSERT_RETFALSE(glist->m_hDoneEvent != NULL);

#if ISVERSION(SERVER_VERSION)
	ASSERT_RETFALSE(sCreateGameServerThreads_ServerVersion(pool, glist));
#else
	ASSERT_RETFALSE(sCreateGameServerThreads_ClientVersion(pool, glist));
#endif

	// wait until all game server threads have been created
	while (glist->m_GameProcessThreadsToCreate != 0)
	{
		Sleep(10);
	}
	return TRUE;
}
#endif


// ---------------------------------------------------------------------------
// initialize game list
// ---------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL GameListInit(
	struct MEMORYPOOL * pool,
	GAMELIST * & glist)
{
#if ISVERSION(DEVELOPMENT) && !ISVERSION(CLIENT_ONLY_VERSION)
	pTimestampResetFunc = GameListResetLastProcessTimestamp;
#endif

	GameListFree(glist);

	ONCE
	{
		glist = (GAMELIST *)MALLOCZ(pool, sizeof(GAMELIST));
		ASSERT_BREAK(glist);

		memclear(glist->m_Games, sizeof(GAME *) * MAX_GAMES_PER_SERVER);
		glist->m_pool = pool;
		glist->m_FreeList.Init(glist->m_pool);
		
#if DECOUPLE_GAME_LIST
		glist->m_WorkQueue.Init(glist->m_pool);
#else		
		glist->m_ProcessList.Init(glist->m_pool);
		glist->m_FinishedList.Init(glist->m_pool);
#endif

		glist->m_ActiveHash.Init();

		for (int ii = MAX_GAMES_PER_SERVER - 1; ii >= 0; ii--)
		{
			glist->m_csGames[ii].Init();
			glist->m_FreeList.Push((unsigned int)ii);
			glist->m_pPlayerToServerMailboxes[ii] = SvrMailboxNew(glist->m_pool);
		}

#if GAME_SERVER_MT
		ASSERT_BREAK(sCreateGameServerThreads(pool, glist));
#endif

		return TRUE;
	}

	GameListFree(glist);
	return FALSE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static LONG sGameListGetNextIter(
	GAMELIST * glist)
{
	return (InterlockedIncrement(&glist->m_nIter) & MAKE_NBIT_MASK(LONG, GAMEID_ITER_BITS));
}
#endif


//----------------------------------------------------------------------------
// initialize & return a new game (leaves game locked)
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
GAME * GameListAddGame(
	GAMELIST * glist,
	GAME_SUBTYPE eGameSubType,
	const GAME_SETUP * game_setup,	// = NULL
	int nDifficulty)
{
	ASSERT_RETNULL(glist);
		
	unsigned int index = glist->m_FreeList.Pop();
	ASSERT_RETNULL(index != INVALID_ID);

	LONG iter = sGameListGetNextIter(glist);
	GAMEID idGame = GameIdNew(index, iter);

	GAME * game = NULL;
	glist->m_csGames[index].Enter();
	ONCE
	{
		ASSERT_BREAK(glist->m_Games[index] == NULL);
		
		game = glist->m_Games[index] = GameInit(
			GAME_TYPE_SERVER, 
			idGame, 
			eGameSubType, 
			game_setup, 
			0,
			nDifficulty);			
		ASSERT_BREAK(game != NULL);
		
		MemorySetThreadPool(NULL);

		// empty out mailbox if we're not initializing it.
		SvrMailboxEmptyMailbox(glist->m_pPlayerToServerMailboxes[index]);

#if DECOUPLE_GAME_LIST
		ASSERT_BREAK(glist->m_WorkQueue.Push(idGame));
#else 
		ASSERT_BREAK(glist->m_FinishedList.Push(idGame));
#endif

		glist->m_ActiveHash.Add(glist->m_pool, idGame);

		return game;
	}

	if (glist->m_Games[index])
	{
		GameListFreeGame(glist, glist->m_Games[index]);
	}
	else
	{
		glist->m_csGames[index].Leave();
		GameListRecycleGameId(glist, idGame);
	}
	return NULL;
}
#endif


//----------------------------------------------------------------------------
// DESC:	swap finished for processed lists
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static void GameListMoveFinishedToProcessList(
	GAMELIST * glist)
{
#if !DECOUPLE_GAME_LIST
	// first, move the games from the finish list to the process list
	glist->m_ProcessList.m_cs.Enter();
	ONCE
	{
		GAMEID * temp = glist->m_ProcessList.m_pList;

#if ISVERSION(DEVELOPMENT)
		CLT_VERSION_ONLY(HALT(glist->m_ProcessList.m_nCount == 0));	// CHB 2007.08.01 - String audit: development
		ASSERT(glist->m_ProcessList.m_nCount == 0);
#endif
		glist->m_FinishedList.m_cs.Enter();
		{
			glist->m_ProcessList.m_pList = glist->m_FinishedList.m_pList;
			glist->m_ProcessList.m_nStart = glist->m_FinishedList.m_nStart;
			glist->m_ProcessList.m_nCount = glist->m_FinishedList.m_nCount;
			glist->m_FinishedList.m_pList = temp;
			glist->m_FinishedList.m_nStart = 0;
			glist->m_FinishedList.m_nCount = 0;
		}
		glist->m_FinishedList.m_cs.Leave();
	}
	glist->m_ProcessList.m_cs.Leave();
#endif	
}
#endif


//----------------------------------------------------------------------------
// DESC:	signal to process games
//			multiple game process threads, when this gets called,
//			simply signal the threads to begin processing and wait until
//			they're done (when they're all asleep again)
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void GameListProcessGames(
	GAMELIST * glist,
	unsigned int sim_frames)
{
	ASSERT_RETURN(glist);
	if (sim_frames <= 0)
	{
		return;
	}

	GameListMoveFinishedToProcessList(glist);

	if (glist->m_GameProcessThreadsActive > 0)
	{
#if !DECOUPLE_GAME_LIST	
		ASSERT(glist->m_GameProcessThreadsRequested == 0);
		glist->m_GameProcessThreadsRequested = glist->m_GameProcessThreadsActive;
		glist->m_SimFramesToProcess = sim_frames;
		glist->m_nGoEventIndex = !glist->m_nGoEventIndex;
		SetEvent(glist->m_hGoEvent[glist->m_nGoEventIndex]);

		DWORD ret = WaitForSingleObject(glist->m_hDoneEvent, GAME_SERVER_CRAP_OUT_TIME);
		if (ret != WAIT_TIMEOUT)
		{
			ASSERT(glist->m_GameProcessThreadsRequested == 0);
			glist->m_SimFramesToProcess = 0;
		}
#endif		
	}
	else
	{
		sGameListProcessList(glist, sim_frames);
	}
}
#endif

//----------------------------------------------------------------------------
// KCK: Reset last frame time on all our games (used for debugging)
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT) && !ISVERSION(CLIENT_ONLY_VERSION)
void GameListResetLastProcessTimestamp(void)
{
	GameServerContext * serverContext = (GameServerContext *)CurrentSvrGetContext();
	ASSERT_RETURN(serverContext);
	ASSERT_RETURN(serverContext->m_GameList);
#if DECOUPLE_GAME_LIST
	for (unsigned int i=0; i<serverContext->m_GameList->m_WorkQueue.m_nCount; i++)
	{
		GAMEID idGame = serverContext->m_GameList->m_WorkQueue.Peek(i);
#else	
	for (unsigned int i=0; i<serverContext->m_GameList->m_ProcessList.m_nCount; i++)
	{
		GAMEID idGame = serverContext->m_GameList->m_ProcessList.Peek(i);
#endif		
		if (idGame != INVALID_ID)
		{
			GAME * game = sGameListGetGameData(serverContext->m_GameList, idGame);
			game->m_LastProcessTimestamp = 0;
		}
	}
}

#endif

//----------------------------------------------------------------------------
// DESC:	returns the Player to Game mailbox associated with a game
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
struct SERVER_MAILBOX * GameListGetPlayerToGameMailbox(
	GAMELIST * glist,
	GAMEID idGame)
{
	ASSERT_RETNULL(glist);
	if (idGame == INVALID_ID)
	{
		return NULL;
	}

	unsigned int ii = GameIdGetIndex(idGame);
	ASSERT_RETNULL(ii < MAX_GAMES_PER_SERVER);

	return glist->m_pPlayerToServerMailboxes[ii];
}
#endif


//----------------------------------------------------------------------------
// DESC:	returns true if a game with said gameid is running
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL GameListQueryGameIsRunning(
	GAMELIST * glist,
	GAMEID idGame)
{
	ASSERT_RETFALSE(glist);
	return (glist->m_ActiveHash.Get(idGame) != NULL);
}
#endif


//----------------------------------------------------------------------------
// DESC:	a very slow operation we only do upon recovering from a chat server crash
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void GameListMessageAllGames(
	GAMELIST * glist,
	NETCLIENTID64 netId,
	MSG_STRUCT * message,
	NET_CMD command)
{
	ASSERT_RETURN(glist);
	ASSERT_RETURN(message);

	for (unsigned int ii = 0; ii < MAX_GAMES_PER_SERVER; ii++)
	{
		SvrNetPostFakeClientRequestToMailbox(
			glist->m_pPlayerToServerMailboxes[ii],
			netId,
			message,
			command );
	}
}
#endif


//----------------------------------------------------------------------------
// DESC:	returns the currently selected game on the server
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
GAME * AppGetSrvGame(
	void)
{
	GameServerContext * serverContext = (GameServerContext*)CurrentSvrGetContext();
	if (!serverContext)
	{
		return NULL;
	}

	GAMELIST * glist = serverContext->m_GameList;

	GAME * game = sGameListGetGameData(glist, AppGetSrvGameId());
	if (!game)
	{
		return NULL;
	}
	GAME * temp = game;
	GameListReleaseGameData(glist, game);

	return temp;
}
#endif
