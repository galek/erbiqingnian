//----------------------------------------------------------------------------
// game.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#if defined(_MSC_VER)
#pragma once
#endif
#ifdef	_GAME_H_
//#pragma message ("    HINT: " __FILE__ " included multiple times in this translation unit")
#else
#define	_GAME_H_

#ifndef _PTIME_H_
#include "ptime.h"
#endif

#ifndef _RANDOM_H_
#include "random.h"
#endif

#ifndef __GAME_VARIANT_H_
#include "gamevariant.h"
#endif

#ifndef _UNITEVENTS_HDR_
#define _UNITEVENTS_HDR_
#include "..\data_common\excel\unitevents_hdr.h"
#endif

#if USE_MEMORY_MANAGER
#include "memoryallocator_pool.h"
#include "memoryallocator_heap.h"
#include "memoryallocator_crt.h"
#include "memoryallocator_wrapper.h"
#endif

#ifndef _CLIENTS_H_
#include "clients.h"
#endif

//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------
#define GMALLOC(g, s)					MALLOC(GameGetMemPool(g), s)
#define GMALLOCFL(g, s, f, l)			MALLOCFL(GameGetMemPool(g), s, f, l)
#define GMALLOCZ(g, s)					MALLOCZ(GameGetMemPool(g), s)
#define GMALLOCZFL(g, s, f, l)			MALLOCZFL(GameGetMemPool(g), s, f, l)
#define GREALLOC(g, p, s)				REALLOC(GameGetMemPool(g), p, s)
#define GREALLOCFL(g, p, s, f, l)		REALLOCFL(GameGetMemPool(g), p, s, f, l)
#define GREALLOCZ(g, p, s)				REALLOCZ(GameGetMemPool(g), p, s)
#define GREALLOCZFL(g, p, s, f, l)		REALLOCZFL(GameGetMemPool(g), p, s, f, l)
#define GFREE(g, p)						FREE(GameGetMemPool(g), p)
#define GAUTOFREE(g, p)					AUTOFREE(GameGetMemPool(g), p)
#define CLTSRVSTR(g)					(IS_SERVER(g) ? "SERVER" : "CLIENT")


//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------
enum GAME_TYPE
{
	GAME_TYPE_CLIENT,
	GAME_TYPE_SERVER
};

enum GAME_INSTANCE_TYPE{				// KCK Added this for a specific purpose, but it seems like this should be combined
	GAME_INSTANCE_NONE = 0,				// with GAME_SUBTYPE somehow. Probably as a list of attibute flags (like: "isTown")
	GAME_INSTANCE_TOWN,			
	GAME_INSTANCE_PVP,
};

enum GAME_SUBTYPE
{
	GAME_SUBTYPE_INVALID = -1,
	
	GAME_SUBTYPE_DUNGEON,
	GAME_SUBTYPE_TUTORIAL,
	GAME_SUBTYPE_BIGTOWN,
	GAME_SUBTYPE_MINITOWN,
	GAME_SUBTYPE_CUSTOM,
	GAME_SUBTYPE_PVP_PRIVATE,
	GAME_SUBTYPE_PVP_PRIVATE_AUTOMATED,
	GAME_SUBTYPE_PVP_CTF,
	GAME_SUBTYPE_OPENWORLD,
	//GAME_SUBTYPE_PVP_CTF_AUTOMATED,
	GAME_SUBTYPE_DEFAULT = GAME_SUBTYPE_DUNGEON,
};

enum GAME_STATE
{
	GAMESTATE_STARTUP,
	GAMESTATE_LOADING,
	GAMESTATE_RUNNING,
	GAMESTATE_REQUEST_SHUTDOWN,
	GAMESTATE_SHUTDOWN,
};

enum
{
	DEBUGFLAG_AI_FREEZE,
	DEBUGFLAG_AI_NOTARGET,
	DEBUGFLAG_PLAYER_INVULNERABLE,
	DEBUGFLAG_ALL_INVULNERABLE,
	DEBUGFLAG_ALWAYS_GETHIT,
	DEBUGFLAG_ALWAYS_SOFTHIT,
	DEBUGFLAG_ALWAYS_KILL,
	DEBUGFLAG_COMBAT_TRACE,
	DEBUGFLAG_MODE_TRACE,
	DEBUGFLAG_STATS_DEBUG,
	DEBUGFLAG_STATS_SERVER,
	DEBUGFLAG_STATS_BASEUNIT,
	DEBUGFLAG_STATS_MODLISTS,
	DEBUGFLAG_STATS_RIDERS,
	DEBUGFLAG_STATS_TRACE,
	DEBUGFLAG_STATS_DUMP,
	DEBUGFLAG_ALLOW_CHEATS,
	DEBUGFLAG_CAN_EQUIP_ALL_ITEMS,
	DEBUGFLAG_OPEN_WARPS,
	DEBUGFLAG_JABBERWOCIZE,
	DEBUGFLAG_UI_EDIT_MODE,
	DEBUGFLAG_GOD,
	DEBUGFLAG_AUTOMAP_MONSTERS,
	DEBUGFLAG_INFINITE_BREATH,
	DEBUGFLAG_INFINITE_POWER,
	DEBUGFLAG_SKILL_LEVEL_CHEAT,
	DEBUGFLAG_REVEAL_ALL_ACHIEVEMENTS,
	DEBUGFLAG_SIMULATE_BAD_FRAMERATE,

	NUM_DEBUGFLAGS,
};

//----------------------------------------------------------------------------
enum GAMEFLAG
{

	GAMEFLAG_INVALID = -1,
	
	GAMEFLAG_TIMESLOW,
	GAMEFLAG_RESTARTING,
	GAMEFLAG_DISABLE_ADS,
	GAMEFLAG_DISABLE_PLAYER_INTERACTION,
	GAMEFLAG_UNITMETRICS_ENABLED,
	
	GAMEFLAG_NUM_FLAGS,						// keep this last please
	
};

#define MAX_GLOBAL_THEMES	128

//----------------------------------------------------------------------------
// TYPEDEFs
//----------------------------------------------------------------------------
template <class T> class CHash;
typedef CHash<struct WARDROBE> WARDROBE_HASH;

//----------------------------------------------------------------------------
// CUSTOM GAME SETUP
//----------------------------------------------------------------------------

void GameSetupInit(
	struct GAME_SETUP &tSetup);

//----------------------------------------------------------------------------
struct GAME_SETUP
{

	// here we have the various values for setting up options in a game
	unsigned int nNumTeams;
	GAME_VARIANT tGameVariant;		// mode of play for this game
	
	GAME_SETUP::GAME_SETUP( void )
	{
		GameSetupInit( *this );
	}
	
};

//----------------------------------------------------------------------------
// GAME STRUCTURE
//----------------------------------------------------------------------------
struct GAME
{
	GAMEID							m_idGame;
	GAME *							m_pNextGame;					// linked list of games, WARNING: these are not locked by the general game lock!!!

	struct GameServerContext *		pServerContext;					// context for the server hosting this game.
	struct UNIT *					m_FreeList;						// free unit list

	GAME_TYPE						eGameType;						// ENV_TYPE_CLIENT or ENV_TYPE_SERVER
	GAME_STATE						eGameState;
	GAME_SUBTYPE					eGameSubType;
	DWORD							m_dwGameFlags;
	unsigned int					m_NestedEventsCounter[NUM_UNIT_EVENTS];	// track nested triggered events

	TIME							tiGameStartTime;				// AppGetCurTime() at GameInit()
	TIME							tiGameCurTime;					// starts at 0, advance by GAME_TICKS_PER_SECOND each simulation frame
	GAME_TICK						tiGameTick;						// current game frame
	float							fGameTime;						// seconds
	
	// Timers used for decoupled game list enumeration
	//
	UINT64							m_LastProcessTimestamp;			// The QueryPerfCounter value of the last time the game was processed

	int								nTimeSlowSkipValue;				// how many frames to skip before running again
	int								nTimeSlowCount;					// current count on frame skip

#if USE_MEMORY_MANAGER

#if DEBUG_MEMORY_ALLOCATIONS
#define cUseDebugHeadersAndFooters true
#else
#define cUseDebugHeadersAndFooters false
#endif

	FSCommon::CMemoryAllocatorCRT<cUseDebugHeadersAndFooters>* m_MemPoolCRT;					// fallback allocator for HEAP
	FSCommon::CMemoryAllocatorHEAP<true, cUseDebugHeadersAndFooters, 32>* m_MemPoolHEAP;					// fallback allocator
#if ISVERSION(SERVER_VERSION)	
	FSCommon::CMemoryAllocatorPOOL<cUseDebugHeadersAndFooters, true, 256>* m_MemPool;						// memory system
#else
	FSCommon::CMemoryAllocatorWRAPPER* m_MemPool;						// memory system
#endif // end ISVERSION(SERVER_VERSION)

#else
	struct MEMORYPOOL *				m_MemPool;						// memory system
#endif // end USE_MEMORY_MANAGER

	struct GAME_CLIENT_LIST *		m_pClientList;					// clients

	float							fHavokUpdateTime;
	float							fAnimationUpdateTime;

	RAND							rand;							// generic random seed for unrepeatable random stuff

	RAND							randLevelSeed;					// seed for DRLG
	DWORD							m_dwRoomIterator;				// iterator for room searches

	struct DUNGEON *				m_Dungeon;						// dungeon

	struct EVENT_SYSTEM *			pEventSystem;					// event scheduler
	struct UNIT_HASH *				pUnitHash;						// unit hash

	WARDROBE_HASH *					pWardrobeHash;					// wardrobes for this game

	struct UNIT *					pStepList;						// unit stepping gets processed every frame

	struct PARTY_LIST *				m_PartyList;					// parties in the game

	DWORD							idStats;						// one-up statlist id
	DWORD							m_idEventHandler;				// one-up event handler id

	struct VMGAME *					m_vm;							// script state

	DWORD							m_dwAIguid;						// Globally unique IDs for AIs
	WORD							m_wMissileTag;					// tag for missile firing groups
	DWORD							m_dwSkillSeed;					// Seed for skills

#if (ISVERSION(DEVELOPMENT)) || _PROFILE				
	DWORD 							m_pdwDebugFlags[DWORD_FLAG_SIZE(NUM_DEBUGFLAGS)];		// flags set by console for debugging
	DWORD							m_dwCombatTraceFlags;			// flags for individual trace items
	GAMECLIENTID					m_idDebugClient;				// client to send debug info to
	DWORD *							pdwDebugStatsTrace;				// flags for stats
	int								m_nDebugTagDisplay;

	int								m_nDebugMonsterLevel;
	int								m_nDebugItemFixedLevel;
											
	UNITID							m_idUnitWithAbsurdNumberOfEventHandlers;
	TIME							m_tiUnitWithAbsurdNumberOfEventHandlersTime;

	UNITID							m_idDebugUnit;
#endif

	struct PICKERS *				m_pPickers;

	struct TEAM_TABLE *				m_pTeams;

	struct GROUP *					m_GroupList;

	struct UNIT_RECORD_DEFINITION * m_UnitRecording;

	DWORD							dwGlobalThemesEnabled[DWORD_FLAG_SIZE(MAX_GLOBAL_THEMES)];

	int								nDifficulty;

	struct METAGAME *				m_pMetagame;					// metagame messaging: for PvP games and quests 

	// the following section is server only
	struct UNIT_ID_MAP *				m_pUnitIdMap;
	struct TASK_GLOBALS	*				m_pTaskGlobals;
	struct KNOWN_GLOBALS *				m_pKnownGlobals;
	struct DBUNIT_GLOBALS *				m_pDBUnitGlobals;
	struct RECIPE_GLOBALS *				m_pClientRecipeGlobals;
	struct COMBAT *						m_pCombatStack;
	
	int								m_nReserveCount;				// number of clients reserving this game
	DWORD							m_dwReserveKey;					// reserve key must match to use reserved game

	// the following section is client only
	UNIT *							pCameraUnit;
	UNIT *							pControlUnit;					// unit we're controlling
	UNITID							idCameraUnit;

	DWORD							dwRoomIterateSequenceNumber;	
	DWORD							dwRoomListTimestamp;

	GAME_TICK						tiLastLevelUpdate;				// tick levels were last updated on
	GAME_VARIANT					m_tGameVariant;					// information about the game mode/variants
	PARTYID							m_idPartyAuto;					// id of party to use for auto parties	

	struct EMAIL_SPEC *				m_pSystemEmails;				// system emails being composed (ie not being composed by players such as CSR or Auction house)
	struct ASYNC_REQUEST_GLOBALS *	m_pAsyncRequestGlobals;			// system globals for the async requests
	CCriticalSection				m_csSystemEmails;

	BYTE *							pAffixGroupValidForCurrent;
	BYTE *							pAffixValidForCurrent;
		
};


//----------------------------------------------------------------------------
// INCLUDES		// these includes require the game structure
//----------------------------------------------------------------------------
#ifndef _GAMERAND_H_
#include "gamerand.h"
#endif


//----------------------------------------------------------------------------
// EXPORTED FUNCTIONS
//----------------------------------------------------------------------------
void GameProcessTick(
	GAME * game);

#if !ISVERSION(SERVER_VERSION)
BOOL c_DoGameTick(
	GAME * game);
#endif //!ISVERSION(SERVER_VERSION)

GAME * GameInit(
	GAME_TYPE eGameType,
	GAMEID idGame,
	GAME_SUBTYPE eGameSubType = GAME_SUBTYPE_DEFAULT,
	const GAME_SETUP * game_setup = NULL,
	DWORD dwGameFlags = 0,
	int nDifficulty = 0);

void GameFree(
	GAME * game);

BOOL GameAllowsActionLevels(
	GAME * game);

void GameIncrementGameTick(
	GAME * game);

UNIT* GameGetDebugUnit(
	GAME * game);

void GameSetDebugUnit(
	GAME * game,
	UNIT* unit);

void GameSetDebugUnit(
	GAME * game,
	UNITID idUnit);

UNIT * GameGetCameraUnit(
	GAME * game);

void GameSetCameraUnit(
	GAME * game,
	UNITID idUnit );

void GameTraceUnits(
	GAME * game,
	BOOL bVerbose);

int SrvGameSimulate(
	GAME * game,
	unsigned int sim_frames);

void GameInitVM(
	GAME * game);

void GameFreeVM(
	GAME * game);
		
void GameSendToClient(
	GAME * game,
	struct GAMECLIENT * client);

BOOL GameIsReservedGame(	
	GAME * game);

int GameGetReservedCount(	
	GAME * game);

void GameSetTimeSlow(
	GAME * game,
	int nSkipCount );

void GameClearTimeSlow(
	GAME * game );

BOOL GameIsVariant(
	GAME * game,
	enum GAME_VARIANT_TYPE eVariant);

void GameSetVariant(
	GAME * game,
	enum GAME_VARIANT_TYPE eVariant,
	BOOL bEnabled);
		
//----------------------------------------------------------------------------
// INLINE FUNCTIONS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// CHB 2007.03.12 - Add references to silence warnings about unreferenced variables.
#if ISVERSION(CLIENT_ONLY_VERSION)

#define IS_CLIENT(x) (REF(x), TRUE)
#define IS_SERVER(x) (REF(x), FALSE)

#elif ISVERSION(SERVER_VERSION)

#define IS_CLIENT(x) (REF(x), FALSE)
#define IS_SERVER(x) (REF(x), TRUE)

#else

inline BOOL IS_CLIENT(
	const GAME * game)
{
	return game->eGameType == GAME_TYPE_CLIENT;
} 

//----------------------------------------------------------------------------
inline BOOL IS_SERVER(
	const GAME * game)
{
	return game->eGameType == GAME_TYPE_SERVER;
}

#endif

//----------------------------------------------------------------------------
inline GAME_SUBTYPE GameGetSubType(
	const GAME * game)
{
	return game->eGameSubType;
}

//----------------------------------------------------------------------------
inline void GameSetSubType(
	GAME * game,
	GAME_SUBTYPE eSubType)
{
	game->eGameSubType = eSubType;
}

//----------------------------------------------------------------------------
inline UNIT* GameGetControlUnit(
	const GAME * game)
{
	ASSERT_RETNULL(game && IS_CLIENT(game));
	return game->pControlUnit;
}

//----------------------------------------------------------------------------
inline struct MEMORYPOOL * GameGetMemPool(
	const GAME * game)
{
	if (!game)
	{
		return NULL;
	}
	return game->m_MemPool;
}


//----------------------------------------------------------------------------
inline GAME_TICK GameGetTick(
	const GAME * game)
{
	return game->tiGameTick;
}

//----------------------------------------------------------------------------
inline float GameGetTimeFloat(
	const GAME * game)
{
	// please don't use this function, it loses precision
	return game->fGameTime;
}

//----------------------------------------------------------------------------
inline GAMEID GameGetId(
	const GAME * game)
{
	return game->m_idGame;
}

//----------------------------------------------------------------------------
inline GAME_STATE GameGetState(
	const GAME * game)
{
	return game->eGameState;
}

//----------------------------------------------------------------------------
inline void GameSetState(
	GAME * game,
	GAME_STATE eState)
{
	game->eGameState = eState;
}

//----------------------------------------------------------------------------
inline DWORD GameGetAIGuid(
	GAME * game)
{
	return game->m_dwAIguid++;
}

//----------------------------------------------------------------------------
inline DWORD GameGetRoomIterator(
	GAME * game)
{
	++game->m_dwRoomIterator;
	if (game->m_dwRoomIterator == 0)
	{
		++game->m_dwRoomIterator;
	}
	return game->m_dwRoomIterator;
}

//----------------------------------------------------------------------------
inline BOOL GameGetDebugFlag(
	GAME * game,
	int flag)
{
	BOOL bResult = FALSE;
#if (ISVERSION(DEVELOPMENT) && ISVERSION(CHEATS_ENABLED))
	if (!game)
	{
		return FALSE;
	}
	bResult = TESTBIT(game->m_pdwDebugFlags, flag);
#else
	UNREFERENCED_PARAMETER(flag);
	UNREFERENCED_PARAMETER(game);
#endif
	return bResult;
}


//----------------------------------------------------------------------------
inline void GameSetDebugFlag(
	GAME * game,
	int flag,
	BOOL value)
{
#if (ISVERSION(DEVELOPMENT) && ISVERSION(CHEATS_ENABLED))
	ASSERT_RETURN(game);
	SETBIT(game->m_pdwDebugFlags, flag, value);
#else
	UNREFERENCED_PARAMETER(game);
	UNREFERENCED_PARAMETER(flag);
	UNREFERENCED_PARAMETER(value);
#endif
}


//----------------------------------------------------------------------------
inline void GameToggleDebugFlag(
	GAME * game,
	int flag)
{
#if (ISVERSION(DEVELOPMENT) && ISVERSION(CHEATS_ENABLED))
	ASSERT_RETURN(game);
	TOGGLEBIT(game->m_pdwDebugFlags, flag);
#else
	UNREFERENCED_PARAMETER(game);
	UNREFERENCED_PARAMETER(flag);
#endif
}


//----------------------------------------------------------------------------
inline BOOL GameGetGameFlag(
	GAME * game,
	int flag)
{
	ASSERT_RETFALSE(game);
	return TESTBIT(&game->m_dwGameFlags, flag);
}


//----------------------------------------------------------------------------
inline void GameSetGameFlag(
	GAME * game,
	int flag,
	BOOL value)
{
	ASSERT_RETURN(game);
	SETBIT(&game->m_dwGameFlags, flag, value);
}


//----------------------------------------------------------------------------
inline void GameToggleGameFlag(
	GAME * game,
	int flag)
{
	ASSERT_RETURN(game);
	TOGGLEBIT(&game->m_dwGameFlags, flag);
}


//----------------------------------------------------------------------------
inline WORD GameGetMissileTag(
	GAME * game)
{
	game->m_wMissileTag++;
	if (!game->m_wMissileTag)
	{
		game->m_wMissileTag++;
	}
	return game->m_wMissileTag;
}

PARTYID GameGetAutoPartyId(
	GAME *pGame);

void s_GameExit(
	GAME * game,
	GAMECLIENT * client,
	BOOL bShowCredits = FALSE );

void GamePushCombat(
	GAME * pGame, 
	struct COMBAT * pCombat);
	
struct COMBAT * GamePopCombat(
	GAME * pGame, 
	struct COMBAT * pCombat);
	
struct COMBAT * GamePeekCombat(
	GAME * pGame);

BOOL GameIsPVP(
	GAME * pGame);

BOOL GameIsUtilityGame( 
	GAME *pGame);

#endif // _GAME_H_
