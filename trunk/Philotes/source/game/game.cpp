//----------------------------------------------------------------------------
// game.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "prime.h"
#include "game.h"
#include "gamevariant.h"
#include "gamelist.h"
#include "clients.h"
#include "s_network.h"
#include "s_message.h"
#include "c_tasks.h"
#include "player.h"
#include "missiles.h"
#include "movement.h"
#include "skills.h"
#include "level.h"
#include "dungeon.h"
#include "picker.h"
#include "perfhier.h"
#include "combat.h"
#include "teams.h"
#include "states.h"
#include "unitidmap.h"
#include "performance.h"
#include "appcommon.h"
#include "appcommontimer.h"
#include "quests.h"
#include "trade.h"
#include "config.h"
#include "script.h"
#include "tasks.h"
#include "customgame.h"
#include "ctf.h"
#include "unit_metrics.h"
#include "s_quests.h"
#include "c_quests.h"
#include "party.h"
#include "openlevel.h"
#include "Quest.h"
#include "bookmarks.h"
#include "unitfile.h"
#include "dbunit.h"
#include "c_camera.h"
#include "c_recipe.h"
#include "c_itemupgrade.h"
#include "c_unitnew.h"
#include "global_themes.h"
#include "player_move_msg.h"
#include "wardrobe.h"
#include "partymatchmaker.h"
#include "asyncrequest.h"

#ifdef HAVOK_ENABLED
#include "havok.h"
#endif

#if ISVERSION(SERVER_VERSION)
#include "game.cpp.tmh"
#include "winperf.h"
#include <Watchdogclient_counter_definitions.h>
#include <PerfHelper.h>
#else
#include "c_ui.h"
#endif
#include "svrstd.h"

#if USE_MEMORY_MANAGER
#include "memoryallocator_HEAP.h"
#include "memoryallocator_POOL.h"
#include "memoryallocator_CRT.h"
#include "memoryallocator_WRAPPER.h"
#include "memorymanager_i.h"
using namespace FSCommon;
#endif

//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
#define DEFAULT_SKILL_SEED 1324						// default game-level random number seed for skills


//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void GameSetupInit(
	GAME_SETUP &tSetup)
{
	tSetup.nNumTeams = 0;
	GameVariantInit( tSetup.tGameVariant, NULL );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void GameIncrementGameTick(
	GAME * game)
{
	ASSERT_RETURN(game);
	game->tiGameTick++;
	game->tiGameCurTime += GAME_TICKS_PER_SECOND;
	game->fGameTime = float(double(game->tiGameCurTime) / double(MSECS_PER_SEC));
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sGameProcessTick(
	GAME * game)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	PERF(SRV_UPDATE_CLIENTS);

	// update levels
	s_LevelsUpdate(game);

	// update quests
	s_QuestsUpdate(game);				
	
	// do database commits
	s_DBUnitOperationsCommit(game);
	
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
static void c_sGameProcessTick(
	GAME * game)
{
#if !ISVERSION(SERVER_VERSION)
	// update client-side-only quest logic
	c_QuestsUpdate(game);
#endif
}
#endif //!ISVERSION(SERVER_VERSION)


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sUpdateGameServerContextTickRate(
	void)
{
#if ISVERSION(SERVER_VERSION)
	GameServerContext * pContext = (GameServerContext *)CurrentSvrGetContext();
	if (pContext) PERF_ADD64(pContext->m_pPerfInstance, GameServer,GameServerGameTickRate,1);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sUpdateGameServerContextGameCount(
	int ii)
{
#if ISVERSION(SERVER_VERSION)
	GameServerContext * pContext = (GameServerContext *)CurrentSvrGetContext();
	if (pContext) PERF_ADD64(pContext->m_pPerfInstance,GameServer,GameServerGameCount,ii);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void GameProcessTick(
	GAME * game)
{
//	trace("[%c] - Processing Tick %i\n", IS_SERVER(game) ? 'S' : 'C', GameGetTick(game));

	// move units
	if (!AppIsTugboat() || IS_SERVER(game))
	{
		DoUnitStepping(game);
	}

	// server only logic
	if (IS_SERVER(game))
	{
		s_sGameProcessTick(game);
	}
	else
	{
#if !ISVERSION(SERVER_VERSION)
		c_sGameProcessTick(game);
#endif 
	}
	
	UnitFreeListFree(game);

	s_sUpdateGameServerContextTickRate();

	// we've not simulated another game tick
	GameIncrementGameTick(game);
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
BOOL c_DoGameTick(
	GAME * game)
{
	ASSERT(IS_CLIENT(game));

	// send out packet of my current move state
	if (AppIsHellgate())
	{
		c_PlayerSendMove(game);
	}

	{
		PERF(CLT_EVENTS);
		GameEventsProcess(game);
	}

	// process current tick
	GameProcessTick(game);

	return TRUE;
}
#endif // !ISVERSION(SERVER_VERSION)


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sGameInitDebugParameters(
	GAME * game,
	const GLOBAL_DEFINITION * global_definition)
{
#if ISVERSION(DEVELOPMENT) || _PROFILE
	ASSERT_RETFALSE(game);
	ASSERT_RETFALSE(global_definition);

	GameSetDebugFlag(game, DEBUGFLAG_AI_FREEZE,			  global_definition->dwGameFlags & GLOBAL_GAMEFLAG_AIFREEZE);
	GameSetDebugFlag(game, DEBUGFLAG_AI_NOTARGET,		  global_definition->dwGameFlags & GLOBAL_GAMEFLAG_AINOTARGET);
	GameSetDebugFlag(game, DEBUGFLAG_PLAYER_INVULNERABLE, global_definition->dwGameFlags & GLOBAL_GAMEFLAG_PLAYERINVULNERABLE);
	GameSetDebugFlag(game, DEBUGFLAG_ALL_INVULNERABLE,	  global_definition->dwGameFlags & GLOBAL_GAMEFLAG_ALLINVULNERABLE);
	GameSetDebugFlag(game, DEBUGFLAG_ALWAYS_GETHIT,		  global_definition->dwGameFlags & GLOBAL_GAMEFLAG_ALWAYSGETHIT);
	GameSetDebugFlag(game, DEBUGFLAG_ALWAYS_SOFTHIT,	  global_definition->dwGameFlags & GLOBAL_GAMEFLAG_ALWAYSSOFTHIT);
	GameSetDebugFlag(game, DEBUGFLAG_ALWAYS_KILL,		  global_definition->dwGameFlags & GLOBAL_GAMEFLAG_ALWAYSKILL);
	GameSetDebugFlag(game, DEBUGFLAG_ALLOW_CHEATS,		  global_definition->dwGameFlags & GLOBAL_GAMEFLAG_ALLOW_CHEATS);
	GameSetDebugFlag(game, DEBUGFLAG_CAN_EQUIP_ALL_ITEMS, global_definition->dwGameFlags & GLOBAL_GAMEFLAG_CAN_EQUIP_ALL_ITEMS);
	
	game->m_idDebugClient = INVALID_CLIENTID;
	
	int statscount = ExcelGetNumRows(game, DATATABLE_STATS);
	game->pdwDebugStatsTrace = (DWORD*)GMALLOC(game, DWORD_FLAG_SIZE(statscount) * sizeof(DWORD));
	for (int ii = 0; ii < DWORD_FLAG_SIZE(statscount); ii++)
	{
		game->pdwDebugStatsTrace[ii] = 0xffffffff;
	}

	game->m_idUnitWithAbsurdNumberOfEventHandlers = INVALID_ID;
	game->m_idDebugUnit = INVALID_ID;
	game->m_nDebugTagDisplay = INVALID_ID;
#endif

	return TRUE;
}

#if DEBUG_MEMORY_ALLOCATIONS	
#define	useHeaderAndFooter true
#else
#define useHeaderAndFooter false
#endif	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
GAME * GameInit(
	GAME_TYPE eGameType,
	GAMEID idGame,
	GAME_SUBTYPE eGameSubType,					// = GAME_SUBTYPE_DEFAULT
	const GAME_SETUP * game_setup,				// = NULL
	DWORD dwGameFlags,
	int nDifficulty)							// = 0
{
	TIMER_START("Game Init")

	const GLOBAL_DEFINITION * global_definition = DefinitionGetGlobal();
	ASSERT_RETNULL(global_definition);

	GAME * game = (GAME *)MALLOCZ(NULL, sizeof(GAME));
    ASSERT_RETNULL(game);
	AUTODESTRUCT<GAME> autod(game, GameFree);

	// This counter is only used to identify games by name in the mem snapshots
	//
	static unsigned int nextGameCounter = 1;
	unsigned int gameCounter = nextGameCounter++;

#if USE_MEMORY_MANAGER	

	WCHAR crtAllocatorName[64];
	_snwprintf_s(crtAllocatorName, _countof(crtAllocatorName), _countof(crtAllocatorName) - 1, L"%s - %d", eGameType == GAME_TYPE_CLIENT ? L"ClientCRT" : L"ServerCRT", gameCounter);
	CMemoryAllocatorCRT<useHeaderAndFooter>* crtAllocator = new CMemoryAllocatorCRT<useHeaderAndFooter>(crtAllocatorName);
	ASSERT(crtAllocator->Init(NULL));
	ASSERT(IMemoryManager::GetInstance().AddAllocator(crtAllocator));
	game->m_MemPoolCRT = crtAllocator;

	WCHAR heapAllocatorName[64];
	_snwprintf_s(heapAllocatorName, _countof(heapAllocatorName), _countof(heapAllocatorName) - 1, L"%s - %d", eGameType == GAME_TYPE_CLIENT ? L"ClientHEAP" : L"ServerHEAP", gameCounter);
	CMemoryAllocatorHEAP<true, useHeaderAndFooter, 32>* heapAllocator = new CMemoryAllocatorHEAP<true, useHeaderAndFooter, 32>(heapAllocatorName);
	ASSERT(heapAllocator->Init(crtAllocator, 248 * KILO, 256 * KILO, 256 * KILO));
	ASSERT(IMemoryManager::GetInstance().AddAllocator(heapAllocator));
	game->m_MemPoolHEAP = heapAllocator;

#if ISVERSION(SERVER_VERSION)

	// Use an alternate bin definition table for Mythos
	//
	static const MEMORY_BIN_DEF tugboatBinDefinitions[] =
	{ // size,	gran,  bucketsize
			8,	   8,	 8192,
		   32,	   8,	32768,
		   64,	  16,	65536,
		  128,	  32,	65536,
		  256,	  64,	65536,
		  512,	 128,   65536,
		 1024,	 256,   65536,
		 2048,	1024,   65536,
		 4096,	2048,   65536,
	};

	WCHAR poolAllocatorName[64];
	_snwprintf_s(poolAllocatorName, _countof(poolAllocatorName), _countof(poolAllocatorName) - 1, L"%s - %d", eGameType == GAME_TYPE_CLIENT ? L"ClientPOOL" : L"ServerPOOL", gameCounter);
	CMemoryAllocatorPOOL<useHeaderAndFooter, true, 256>* poolAllocator = new CMemoryAllocatorPOOL<true, true, 256>(poolAllocatorName);

	const MEMORY_BIN_DEF* binDefinitions = NULL;
	unsigned int binDefinitionCount = 0;

	if(AppIsTugboat())
	{
		binDefinitions = &tugboatBinDefinitions[0];
		binDefinitionCount = _countof(tugboatBinDefinitions);
	}
	ASSERT(poolAllocator->Init(heapAllocator, 4096, true, true, binDefinitions, binDefinitionCount));

	ASSERT(IMemoryManager::GetInstance().AddAllocator(poolAllocator));
	game->m_MemPool = poolAllocator;
#else
	CMemoryAllocatorWRAPPER* poolAllocator = new CMemoryAllocatorWRAPPER();
	poolAllocator->Init(IMemoryManager::GetInstance().GetDefaultAllocator(), heapAllocator);
	game->m_MemPool = poolAllocator;
#endif // end ISVERSION(SERVER_VERSION)
		
#else
	game->m_MemPool = MemoryPoolInit(eGameType == GAME_TYPE_CLIENT ? L"GamePoolClient" : L"GamePoolServer", DefaultMemoryAllocator);
#endif
	
	game->m_idGame = idGame;
	game->eGameType = eGameType;
	game->eGameState = GAMESTATE_STARTUP;
	GameSetSubType(game, eGameSubType);
	game->m_dwAIguid = 0;
	game->m_dwSkillSeed = DEFAULT_SKILL_SEED;
	game->m_dwGameFlags = dwGameFlags;
	game->m_idPartyAuto = INVALID_ID;
	game->m_pSystemEmails = NULL;
	game->m_csSystemEmails.Init();
	game->m_pAsyncRequestGlobals = NULL;
	game->m_pCombatStack = NULL;
	
	// save variant information
	GameVariantInit( game->m_tGameVariant, NULL );
	if (game_setup)
	{
		game->m_tGameVariant = game_setup->tGameVariant;
	}

	// for now, because difficutly is a separate param and is not properly integrated into
	// the game variant for purposes of game initialization, we need to take that difficulty
	// value here over what is in the game variant struct
	game->m_tGameVariant.nDifficultyCurrent = nDifficulty;
	
	game->tiGameStartTime = AppCommonGetCurTime();
	
	game->m_LastProcessTimestamp = 0;

	game->pServerContext = (GameServerContext *)CurrentSvrGetContext();

	if (IS_SERVER(game))
	{
		MemorySetThreadPool(GameGetMemPool(game));
	}

	ClientListInit(game);
	c_PlayerInitForGame(game);
	c_UnitNewInitForGame(game);
#ifdef HAVOK_ENABLED
	if (AppIsHellgate())
	{
		HavokInit(game);
	}
#endif
	UnitHashInit(game);
	EventSystemInit(game);
	AsyncRequestInitForGame(game);	
	GameInitVM(game);

	// below this needs to be looked at closely!
	ASSERT_RETNULL(DungeonInit(game, global_definition->dwSeed));
	RandInit(game->rand);
	
	game->m_dwReserveKey = RandGetNum(game);

#if ISVERSION(DEVELOPMENT)
	ASSERT_RETNULL(sGameInitDebugParameters(game, global_definition));
#endif
	
	if (IS_SERVER(game))		// server specific stuff
	{
#if !ISVERSION(CLIENT_ONLY_VERSION)
		s_TasksInitForGame(game);		
		s_DatabaseUnitsInitForGame(game);
		s_PartyMatchmakerInitForGame(game);
#endif 
	}
#if !ISVERSION(SERVER_VERSION)
	else						// client specific stuff
	{
		GameSetCameraUnit(game, INVALID_ID);
		if (!AppIsHammer())
		{
			c_TasksInitForGame(game);
			c_RecipeInitForGame(game);
			c_ItemUpgradeInitForGame(game);
		}
	}
#endif //!SERVER_VERSION

	StatesInit( game );
	TeamsInit(game);
	UnitMetricsSystemInit(game);

	game->m_pUnitIdMap = UnitIdMapInit(game);
	
	ASSERT_RETNULL(PartyListInit(game));
	TradeInit(game);

	game->m_GroupList = NULL;

	DungeonPickDRLGs(game, nDifficulty);
	
	MetagameInit(game, game_setup);
	
	if (IS_SERVER(game))		// server specific stuff
	{
		s_GlobalThemesUpdate( game );
		int nAffixCount = ExcelGetNumRows(game, DATATABLE_AFFIXES);
		game->pAffixValidForCurrent = (BYTE*)GMALLOC(game, BYTE_FLAG_SIZE(nAffixCount) * sizeof(BYTE));
		int nAffixGroupCount = ExcelGetNumRows(game, DATATABLE_AFFIX_GROUPS);
		game->pAffixGroupValidForCurrent = (BYTE*)GMALLOC(game, BYTE_FLAG_SIZE(nAffixGroupCount) * sizeof(BYTE));
	}

	s_sUpdateGameServerContextGameCount(1);
	autod.Clear();

	return game;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void GameFree(
	GAME * game)
{
	ASSERT_RETURN(game && game != EMPTY_GAME_SLOT);

	game->eGameState = GAMESTATE_SHUTDOWN;

#if !ISVERSION(CLIENT_ONLY_VERSION)
	if (IS_SERVER(game))
	{
		OpenLevelClearAllFromGame(game);
	}
#endif

	// free clients here
	TradeFree(game);
	PartyListFree(game);

	UnitIdMapFree(game->m_pUnitIdMap);
	game->m_pUnitIdMap = NULL;

	UnitMetricsSystemClose(game);

#if !ISVERSION(SERVER_VERSION)
	c_ItemUpgradeFreeForGame(game);
	c_RecipeCloseForGame(game);
	c_TasksCloseForGame(game);
	c_UIFreeClientUnits(game);

#endif

#if !ISVERSION(CLIENT_ONLY_VERSION)
	s_PartyMatchmakerFreeForGame(game);
	s_DatabaseUnitsFreeForGame(game);
	s_TasksCloseForGame(game);
	s_QuestForTugboatFree(game);	//free quest system
#endif

	PickerFree(game);

	if (game->pAffixValidForCurrent)
		GFREE(game, game->pAffixValidForCurrent);
	if (game->pAffixGroupValidForCurrent)
		GFREE(game, game->pAffixGroupValidForCurrent);

	// CML 2008.01.29 - Separating the ad level exit code from the rest of DungeonFree to fix chicken/egg problem.
	DungeonFreeAds(game);

#if !ISVERSION(SERVER_VERSION)
	// CML 2008.01.29 - Moving the SetCurrentLevel(-1) to before DungeonFree to allow multiplayer games a chance to save the automap.
	if (IS_CLIENT(game))
	{
		AppSetCurrentLevel(INVALID_ID);
	}
#endif

	game->m_csSystemEmails.Free();
	DungeonFree(game);
	AsyncRequestFreeForGame(game);		
	EventSystemFree(game);
	WardrobeHashFree(game);
	UnitHashFree(game);
	TeamsFree(game);
	MetagameFree(game);
	// moving the VM free to after the hash free - some events when destroying items want the VM.
	GameFreeVM(game);
#ifdef HAVOK_ENABLED
	if (AppIsHellgate())
	{
		HavokClose(game);
	}
#endif
#if !ISVERSION(SERVER_VERSION)
	c_UnitNewFreeForGame(game);
	c_PlayerCloseForGame(game);
#endif // !ISVERSION(SERVER_VERSION)
	ClientListFree(game);

#if ISVERSION(DEVELOPMENT)
	if (game->pdwDebugStatsTrace)
	{
		GFREE(game, game->pdwDebugStatsTrace);
	}
#endif

#if !ISVERSION(SERVER_VERSION)
	if (IS_CLIENT(game))
	{
		// Moved up to before DungeonFree()
		//AppSetCurrentLevel(INVALID_ID);
		AppSetCltGame( NULL );
		AppSetSrvGameId(INVALID_ID);
	}
#endif

#if USE_MEMORY_MANAGER
	if(game->m_MemPoolCRT)
	{
		game->m_MemPoolCRT->Term();
		IMemoryManager::GetInstance().RemoveAllocator(game->m_MemPoolCRT);
		delete game->m_MemPoolCRT;
		game->m_MemPoolCRT = NULL;
	}
	
	if(game->m_MemPoolHEAP)
	{
		game->m_MemPoolHEAP->Term();
		IMemoryManager::GetInstance().RemoveAllocator(game->m_MemPoolHEAP);
		delete game->m_MemPoolHEAP;
		game->m_MemPoolHEAP = NULL;
	}
	
	if(game->m_MemPool)
	{
		game->m_MemPool->Term();
#if ISVERSION(SERVER_VERSION)
		IMemoryManager::GetInstance().RemoveAllocator(game->m_MemPool);
#endif
		delete game->m_MemPool;
		game->m_MemPool = NULL;
	}
#else	
	MemoryPoolFree(GameGetMemPool(game));
#endif

	FREE(NULL, game);
	game = NULL;

#if ISVERSION(SERVER_VERSION)
	s_sUpdateGameServerContextGameCount(-1);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL GameAllowsActionLevels(
	GAME * game)
{
	ASSERT_RETFALSE(game);
	
	// single player games allow it all
	if (AppGetType() == APP_TYPE_SINGLE_PLAYER)
	{
		return TRUE;
	}
	
	switch (GameGetSubType(game))
	{
		case GAME_SUBTYPE_DUNGEON:		return TRUE;
		case GAME_SUBTYPE_TUTORIAL:		return TRUE;
		case GAME_SUBTYPE_BIGTOWN:		return FALSE;
		case GAME_SUBTYPE_MINITOWN:		return FALSE;
		case GAME_SUBTYPE_OPENWORLD:	return TRUE;
		case GAME_SUBTYPE_CUSTOM:		return TRUE;
		case GAME_SUBTYPE_PVP_PRIVATE:
		case GAME_SUBTYPE_PVP_PRIVATE_AUTOMATED:
			return TRUE;
		case GAME_SUBTYPE_PVP_CTF:	return TRUE;
		default: ASSERT_RETFALSE(0);
	}
}

	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT* GameGetDebugUnit(
	GAME * game)
{
#if (ISVERSION(DEVELOPMENT))
	ASSERT_RETNULL(game);
	if (game->m_idDebugUnit == INVALID_ID)
	{
		return NULL;
	}
	return UnitGetById(game, game->m_idDebugUnit);
#else
	return NULL;
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void GameSetDebugUnit(
	GAME * game,
	UNIT * unit)
{
#if (ISVERSION(DEVELOPMENT))
	ASSERT_RETURN(game);
	if (!unit)
	{
		game->m_idDebugUnit = INVALID_ID;
	}
	else
	{
		game->m_idDebugUnit = UnitGetId(unit);
	}
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void GameSetDebugUnit(
	GAME * game,
	UNITID idUnit)
{
#if (ISVERSION(DEVELOPMENT))
	ASSERT_RETURN(game);
	game->m_idDebugUnit = idUnit;
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT * GameGetCameraUnit(
	GAME * game)
{
	ASSERT_RETVAL(game && IS_CLIENT(game), NULL);
	return game->pCameraUnit;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void GameSetCameraUnit(
	GAME * game,
	UNITID idUnit )
{
	ASSERT_RETURN(game && IS_CLIENT(game));

	BOOL bUnitChanged = idUnit != game->idCameraUnit; 
	if (bUnitChanged)
	{
		c_CameraResetBone();
	}
	game->idCameraUnit = idUnit;
	AppSetDetachedCamera(idUnit == INVALID_ID);
	c_UnitUpdateViewModel(GameGetControlUnit(game), FALSE);

	UNIT * pUnit = UnitGetById( game, idUnit );
	game->pCameraUnit = pUnit;
	if ( pUnit )
	{
		if (GameGetControlUnit(game) == pUnit)
		{
			// Make Camera face the same direction as the unit we're attached to.
			int nAngle = VectorDirectionToAngleInt( UnitGetFaceDirection( pUnit, FALSE ) );
			c_PlayerSetAngles(game, nAngle);
		}

		// this is to help the logic which prevents some view types depending on a unit's inventory
		if ( bUnitChanged )
		{
			int nViewType = c_CameraGetViewType();
			c_CameraSetViewType( nViewType );
		}
	}
}


//----------------------------------------------------------------------------
// in "slow time" mode, run only once every N ticks
//----------------------------------------------------------------------------
static BOOL sTimeSlowCanRunTick(
	GAME * game)
{
	ASSERT_RETFALSE(game);

	if (AppGetType() == APP_TYPE_SINGLE_PLAYER)
	{
		return TRUE;
	}

	if (!GameGetGameFlag(game, GAMEFLAG_TIMESLOW))
	{
		return TRUE;
	}
	
	game->nTimeSlowCount++;
	if (game->nTimeSlowCount >= game->nTimeSlowSkipValue)
	{
		game->nTimeSlowCount = 0;
		return TRUE;
	}

	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
void drbDebugClearPathCount();
#endif

static BOOL SrvGameTick(
	GAME * game)
{
	ASSERT(IS_SERVER(game));

#if ISVERSION(DEVELOPMENT)
	drbDebugClearPathCount();
#endif

	if ( !sTimeSlowCanRunTick( game ) )
	{
		return FALSE;
	}

#ifdef HAVOK_ENABLED
	if (AppIsHellgate())
	{
		PERF(SRV_HAVOK_UPDATE)
 		s_HavokUpdate(game, GAME_TICK_TIME_FLOAT);
	}
#endif

	{
		PERF(SRV_GAME_EVENTS_PROCESS)
		GameEventsProcess(game);
	}

	// process current tick
	{
		PERF(SRV_GAME_PROCESS_TICK)
		GameProcessTick(game);
	}

	return TRUE;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int SrvGameSimulate(
	GAME * game,
	unsigned int sim_frames)
{
	if (!game)
	{
		return 0;
	}
	ASSERT(IS_SERVER(game));

#if !ISVERSION(CLIENT_ONLY_VERSION)
	SrvGameProcessMessages(game);
	
	if (GameGetState(game) == GAMESTATE_REQUEST_SHUTDOWN)
	{
		return 0;
	}

	{
#if DECOUPLE_GAME_LIST	
		// If this is the first time the game has been processed, just simulate one frame
		//
		if(game->m_LastProcessTimestamp == 0)
		{
			UINT64 now = PGetPerformanceCounter();
		
			sim_frames = 1;		
			game->m_LastProcessTimestamp = now;			
		}
		else
		{			
			// Figure out how many frames to simulate based on the last time the game was processed
			//		
			UINT64 perfTicksPerSecond = PGetPerformanceFrequency();
			UINT64 perfTicksPerGameTick = perfTicksPerSecond / GAME_TICKS_PER_SECOND;
			
			UINT64 now = PGetPerformanceCounter();
			now = MAX<UINT64>(now, game->m_LastProcessTimestamp); // Account for apparent inconsistencies?			
			UINT64 perfTicksElapsedTime = now - game->m_LastProcessTimestamp;
			
			// If less than a game frame has passed since the game was last processed, sleep until
			// at least one game frame should be processed
			//
			if(perfTicksElapsedTime < perfTicksPerGameTick)
			{
				UINT64 perfTicksSleepTime = perfTicksPerGameTick - perfTicksElapsedTime;
				float sleepTimeInMilliseconds = (float)perfTicksSleepTime / (float)perfTicksPerSecond * 1000.0f;
				
				if(sleepTimeInMilliseconds)
				{
					Sleep(0);
				}
			}
			
			now = PGetPerformanceCounter();
			now = MAX<UINT64>(now, game->m_LastProcessTimestamp); // Account for apparent inconsistencies?
			perfTicksElapsedTime = now - game->m_LastProcessTimestamp;				
	
			UINT64 lastsimtick = game->m_LastProcessTimestamp / perfTicksPerGameTick;
			UINT64 cursimtick =  now / perfTicksPerGameTick;

			sim_frames = (unsigned int)(cursimtick - lastsimtick);
			if (sim_frames > 0)
			{
				game->m_LastProcessTimestamp = now;
			}
			
		}
#endif		
	
		PERF(SRV_GAMETICK)
		// update server timing and determine if it is time to run a game tick
		for (unsigned int ii = 0; ii < sim_frames; ii++)
		{
			SrvGameTick(game);
		}
	}

	SendClientMessages(game);
#endif //#if !ISVERSION(CLIENT_ONLY_VERSION)
	return sim_frames;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void GameInitVM(
	GAME * game)
{
	ASSERT_RETURN(game);
	ASSERT(game->m_vm == NULL);
	game->m_vm = VMInitGame(GameGetMemPool(game));	
}	


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void GameFreeVM(
	GAME * game)
{
	ASSERT_RETURN(game);
	ASSERT_RETURN(game->m_vm);
	
	VMFreeGame(game->m_vm);
	game->m_vm = NULL;
}	


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void GameSendToClient(
	GAME * game,
	GAMECLIENT * client)
{
	{
		MSG_SCMD_YOURID msg;
		msg.id = ClientGetId(client);
		s_SendMessage(game, ClientGetId(client), SCMD_YOURID, &msg);
	}
	{
		MSG_SCMD_GAME_NEW msg;
		msg.idGame = GameGetId(game);
		msg.dwCurrentTick = GameGetTick(game);
		msg.bGameSubType = (BYTE)GameGetSubType(game);
		msg.dwGameFlags = game->m_dwGameFlags;
		msg.tGameVariant = game->m_tGameVariant;
		s_SendMessage(game, ClientGetId(client), SCMD_GAME_NEW, &msg);

		s_GlobalThemeSendAllToClient( game, ClientGetId( client ) );
	}

	TeamsSendToClient(game, client);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL GameIsReservedGame(	
	GAME * game)
{
	return GameGetReservedCount(game) > 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int GameGetReservedCount(	
	GAME * game)
{
	return game->m_nReserveCount;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void GameSetTimeSlow(
	GAME * game,
	int nSkipCount )
{
	ASSERT_RETURN( game );
	GameSetGameFlag( game, GAMEFLAG_TIMESLOW, TRUE );
	game->nTimeSlowCount = 0;
	game->nTimeSlowSkipValue = nSkipCount;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void GameClearTimeSlow(
	GAME * game )
{
	ASSERT_RETURN( game );
	GameSetGameFlag( game, GAMEFLAG_TIMESLOW, FALSE ); 
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL GameIsVariant(
	GAME * game,
	GAME_VARIANT_TYPE eVariant)
{
	ASSERTV_RETFALSE( game, "Expected game" );
	const GAME_VARIANT &tGameVariant = game->m_tGameVariant;
	return TESTBIT( tGameVariant.dwGameVariantFlags, eVariant );	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void GameSetVariant(
	GAME * game,
	GAME_VARIANT_TYPE eVariant,
	BOOL bEnabled)
{
	ASSERTV_RETURN( game, "Expected game" );
	GAME_VARIANT &tGameVariant = game->m_tGameVariant;
	SETBIT( tGameVariant.dwGameVariantFlags, eVariant, bEnabled );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PARTYID GameGetAutoPartyId(
	GAME *pGame)
{
	ASSERTX_RETINVALID( pGame, "Expected game" );
	PARTYID idParty = pGame->m_idPartyAuto;

	if (idParty == INVALID_ID)
	{
		// go through all clients
		for (GAMECLIENT *pClient = ClientGetFirstInGame( pGame );
			 pClient;
			 pClient = ClientGetNextInGame( pClient ))
		{
			UNIT *pUnit = ClientGetControlUnit( pClient );
			PARTYID idPartyThisUnit = UnitGetPartyId( pUnit );
			
			// for purposes of auto partying, we never allow more than one party into the same
			// game instance ... in fact, it's the parties that determine which game
			// instance players travel to when they go through warps ... this will probably
			// change one day tho
			ASSERTX_CONTINUE( idParty == INVALID_ID || idParty == idPartyThisUnit, "More than one party detected in game instance!!!" );
			idParty = idPartyThisUnit;
		}
	}
		
	// return the party found (if any)
	return idParty;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_GameExit( GAME * game, GAMECLIENT * client, BOOL bShowCredits )
{
	ASSERT_RETURN(game && IS_SERVER(game));

	UNIT * player = ClientGetPlayerUnit(client);
	if (player)
	{
		if( AppIsTugboat() )
		{
			PlayerSetRespawnLocation( KRESPAWN_TYPE_PRIMARY, player, NULL, UnitGetLevel( player ) );
		}
		// save the player before restarting
		AppPlayerSave(player);
	}

	MSG_SCMD_GAME_CLOSE msg;
	msg.idGame = GameGetId(game);
	msg.bRestartingGame = TRUE;
	msg.bShowCredits = (BYTE)bShowCredits;
	s_SendMessage(game, ClientGetId(client), SCMD_GAME_CLOSE, &msg);

	if ( AppGetType() != APP_TYPE_CLOSED_SERVER &&
		(AppGetType() == APP_TYPE_SINGLE_PLAYER ||
		GameGetClientCount(game) == 1))
	{
		GameSetState(game, GAMESTATE_REQUEST_SHUTDOWN);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void GamePushCombat(GAME * pGame, struct COMBAT * pCombat)
{
	ASSERT_RETURN(pGame);
	ASSERT_RETURN(pCombat);
	CombatSetNext(pCombat, pGame->m_pCombatStack);
	pGame->m_pCombatStack = pCombat;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct COMBAT * GamePopCombat(GAME * pGame, struct COMBAT * pCombat)
{
	ASSERT_RETNULL(pGame);
	ASSERT_RETNULL(pGame->m_pCombatStack);
	ASSERT_RETNULL(pGame->m_pCombatStack == pCombat);
	struct COMBAT * pCombatNext = CombatGetNext(pGame->m_pCombatStack);
	struct COMBAT * pCombatCurrent = pGame->m_pCombatStack;
	CombatSetNext(pCombatCurrent, NULL);
	pGame->m_pCombatStack = pCombatNext;
	return pCombatCurrent;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct COMBAT * GamePeekCombat(GAME * pGame)
{
	ASSERT_RETNULL(pGame);
	return pGame->m_pCombatStack;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL GameIsPVP(GAME * pGame)
{
	if (pGame && (pGame->eGameSubType == GAME_SUBTYPE_PVP_PRIVATE ||
		pGame->eGameSubType == GAME_SUBTYPE_PVP_PRIVATE_AUTOMATED ||
		pGame->eGameSubType == GAME_SUBTYPE_PVP_CTF ||
		pGame->eGameSubType == GAME_SUBTYPE_CUSTOM) )
	{
		return TRUE;
	}

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL GameIsUtilityGame( 
	GAME *pGame)
{	
	ASSERTX_RETFALSE( pGame, "Expected game" );
	return TESTBIT( pGame->m_tGameVariant.dwGameVariantFlags, GV_UTILGAME );
}
