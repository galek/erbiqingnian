// ---------------------------------------------------------------------------
// FILE:	minitown.cpp
// DESC:	handles minitowns on a physical server
//
//  Copyright 2003, Flagship Studios
// ---------------------------------------------------------------------------
#include "stdafx.h"
#include "prime.h"
#include "game.h"
#include "globalindex.h"
#include "minitown.h"
#include "excel.h"
#include "level.h"
#include "s_network.h"
#include "clients.h"
#include "warp.h"
#include "servers.h"
#include "ServerRunnerAPI.h"
#if ISVERSION(SERVER_VERSION)
#include "minitown.cpp.tmh"
#endif

// ---------------------------------------------------------------------------
// STRUCTS
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// per game nodes stored by MINITOWN_LIST
// ---------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
struct MINITOWN_NODE
{
	GAMEID									m_idGame;
	GAME_SETUP								m_tSetup;			// setup that was used for the game
	long									m_MaxPlayerCount;
	volatile long							m_lPlayersInGameOrWaitingToJoin;
	volatile long							m_lPlayersInGame;
};
#endif


// ---------------------------------------------------------------------------
// list of all instances for a particular minitown level
// ---------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
struct MINITOWN_LIST
{
	unsigned int							m_Level;						// level definition id
	SIMPLE_ARRAY<MINITOWN_NODE *, 8>		m_GameList;

	MINITOWN_LIST() : m_Level(INVALID_ID)
	{
		m_GameList.Init();
	}
	MINITOWN_LIST(unsigned int in_level) : m_Level(in_level)
	{
		m_GameList.Init();
	}
	BOOL Init(unsigned int level_definition_id, const LEVEL_DEFINITION * level_data);
	MINITOWN_NODE * CreateNewGame(GAME_SUBTYPE eGameSubType, const GAME_SETUP * setup);
	GAMEID GetAvailableGameId(APPCLIENTID idAppClient, GAME_SUBTYPE eGameSubType, const GAME_SETUP * setup);
	void FreeGameId(GAMEID idGame);
	BOOL StatTrace();
};
#endif


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
int MiniTownListCompare(
	const MINITOWN_LIST & a, const MINITOWN_LIST & b)
{
	if (a.m_Level > b.m_Level)
	{
		return 1;
	}
	else if (a.m_Level < b.m_Level)
	{
		return -1;
	}
	return 0;
}
#endif


// ---------------------------------------------------------------------------
// APP - level structure containing all minitowns
// ---------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
struct MINITOWN
{
	CCritSectLite							m_CS;
	SORTED_ARRAYEX<MINITOWN_LIST, 
		MiniTownListCompare, 8>				m_List;		// sorted array of minitownlists

	BOOL Init(void);
	GAMEID GetAvailableGameId(APPCLIENTID idAppClient, unsigned int level, GAME_SUBTYPE eGameSubType, const GAME_SETUP * setup);
	void FreeGameId(GAMEID idGame);
	BOOL StatTrace(unsigned int level);
};
#endif


// ---------------------------------------------------------------------------
// STATIC FUNCTIONS
// ---------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static GAMEID sCreateMiniTownGame(
	GAME_SUBTYPE eGameSubType,
	const GAME_SETUP * setup = NULL,
	int nDifficulty = 0)
{
	GAMEID idGame = SrvNewGame(eGameSubType, setup, TRUE, nDifficulty);
	ASSERT_RETINVALID(!GAMEID_IS_INVALID(idGame));
	return idGame;
}
#endif


// ---------------------------------------------------------------------------
// MEMBER FUNCTIONS
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL MINITOWN_LIST::Init(
	unsigned int level_definition_id,
	const LEVEL_DEFINITION * level_data)
{
	ASSERT_RETFALSE(level_data);
	m_Level = level_definition_id;
	m_GameList.Init();
	return TRUE;
}
#endif


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
MINITOWN_NODE * MINITOWN_LIST::CreateNewGame(
	GAME_SUBTYPE eGameSubType,
	const GAME_SETUP * setup)
{
	const LEVEL_DEFINITION * level_data = LevelDefinitionGet(m_Level);
	ASSERT_RETNULL(level_data);

	MINITOWN_NODE * minitown = (MINITOWN_NODE *)MALLOCZ(NULL, sizeof(MINITOWN_NODE));
	ASSERT_RETNULL(minitown);
	minitown->m_idGame = sCreateMiniTownGame(eGameSubType, setup);
	if (setup)
	{
		minitown->m_tSetup = *setup;
	}
	else
	{
		GameSetupInit( minitown->m_tSetup );
	}
	ASSERT_DO(minitown->m_idGame != INVALID_GAMEID)
	{
		FREE(NULL, minitown);
		return NULL;
	}
	minitown->m_MaxPlayerCount = level_data->nMaxPlayerCount;
	if (minitown->m_MaxPlayerCount <= 0)
	{
		minitown->m_MaxPlayerCount = INT_MAX;
	}
	m_GameList.InsertEnd(MEMORY_FUNC_FILELINE(NULL, minitown));
	return minitown;
}
#endif


// ---------------------------------------------------------------------------
// for now, this list only contains 1 game in element 0, later, if the
// game instance fills up, we may need to create multiple entries
// ---------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
GAMEID MINITOWN_LIST::GetAvailableGameId(
	APPCLIENTID idAppClient,
	GAME_SUBTYPE eGameSubType,
	const GAME_SETUP * setup)
{
	BOOL bNeedPVPWorld = TESTBIT( setup->tGameVariant.dwGameVariantFlags, GV_PVPWORLD );

	MINITOWN_NODE * minitown = NULL;
	for (unsigned int ii = 0; ii < m_GameList.Count(); ++ii)
	{
		minitown = m_GameList[ii];
		ASSERT_CONTINUE(minitown);
		ASSERT_CONTINUE(minitown->m_idGame != INVALID_GAMEID);

		// here we might want to compare the setup of the minitown game compared
		// to the setup passed in here ... for instance, we might have certain game
		// modes that require us to split up the community in towns (league) that
		// might be mixed in a server with other communities. this is where we'd do it
		
		long nPendingPlayerCount = InterlockedIncrement(&minitown->m_lPlayersInGameOrWaitingToJoin);		
		BOOL bPVPWorld = TESTBIT( minitown->m_tSetup.tGameVariant.dwGameVariantFlags, GV_PVPWORLD );
		if (nPendingPlayerCount <= minitown->m_MaxPlayerCount &&
			bPVPWorld == bNeedPVPWorld)
		{
			break;
		}
		InterlockedDecrement(&minitown->m_lPlayersInGameOrWaitingToJoin);
		minitown = NULL;
	}

	if (!minitown)
	{
		minitown = CreateNewGame(eGameSubType, setup);
		ASSERT_RETVAL(minitown, INVALID_GAMEID);
		ASSERT_RETVAL(minitown->m_idGame != INVALID_GAMEID, INVALID_GAMEID);
		InterlockedIncrement(&minitown->m_lPlayersInGameOrWaitingToJoin);
	}
	
	if (!ClientSystemSetPendingGameToJoin(AppGetClientSystem(), idAppClient, &minitown->m_lPlayersInGameOrWaitingToJoin, &minitown->m_lPlayersInGame))
	{
		InterlockedDecrement(&minitown->m_lPlayersInGameOrWaitingToJoin);
		return INVALID_GAMEID;
	}
	TraceGameInfo("Got gameid %I64x with %d in game, %d total out of %d max",
		minitown->m_idGame, minitown->m_lPlayersInGame,
		minitown->m_lPlayersInGameOrWaitingToJoin, minitown->m_MaxPlayerCount);

	return minitown->m_idGame;
}
#endif


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void MINITOWN_NODE_DELETE(
	MEMORYPOOL * pool,
	MINITOWN_NODE * & node)
{
	ASSERT_RETURN(node);
	FREE(pool, node);
}
#endif


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void MINITOWN_LIST::FreeGameId(
	GAMEID idGame)
{
	unsigned int jj = 0;
	while (jj < m_GameList.Count())
	{
		MINITOWN_NODE * minitown = m_GameList[jj];
		ASSERT_CONTINUE(minitown);
		if (minitown->m_idGame == idGame)
		{
			m_GameList.Delete(MEMORY_FUNC_FILELINE(NULL, jj, MINITOWN_NODE_DELETE));
		}
		else
		{
			++jj;
		}
	}
}
#endif

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL MINITOWN_LIST::StatTrace()
{
	MINITOWN_NODE * minitown = NULL;
	for (unsigned int ii = 0; ii < m_GameList.Count(); ++ii)
	{
		minitown = m_GameList[ii];
		ASSERT_CONTINUE(minitown);
		TraceGameInfo("Gameid %I64x has %d in game, %d total out of %d max",
			minitown->m_idGame, minitown->m_lPlayersInGame,
			minitown->m_lPlayersInGameOrWaitingToJoin, minitown->m_MaxPlayerCount);
	}
	
	return TRUE;
}
#endif

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL MINITOWN::Init(
	void)
{
	m_CS.Init();

	CSLAutoLock autolock(&m_CS);
	
	// count up the total number of minitown levels
	unsigned int levelcount = LevelGetDefinitionCount();
	for (unsigned int ii = 0; ii < levelcount; ++ii)
	{
		const LEVEL_DEFINITION * level_data = LevelDefinitionGet(ii);
		if (!level_data)
		{
			continue;
		}
		if (level_data->eSrvLevelType == SRVLEVELTYPE_MINITOWN ||
			level_data->eSrvLevelType == SRVLEVELTYPE_OPENWORLD ||
			level_data->eSrvLevelType == SRVLEVELTYPE_CUSTOM ||
			level_data->eSrvLevelType == SRVLEVELTYPE_PVP_CTF)
		{
			MINITOWN_LIST minitownlist;
			structclear(minitownlist);
			minitownlist.Init(ii, level_data);
			m_List.Insert(NULL, minitownlist);
		}
	}

	return TRUE;
}
#endif


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
GAMEID MINITOWN::GetAvailableGameId(
	APPCLIENTID idAppClient,
	unsigned int level,
	GAME_SUBTYPE eGameSubType,
	const GAME_SETUP * setup)
{
	GAMEID idGame = INVALID_ID;
	
	CSLAutoLock autolock(&m_CS);
	
	MINITOWN_LIST key;
	key.m_Level = level;
	MINITOWN_LIST * minitownlist = m_List.FindExact(key);
	ASSERT_RETVAL(minitownlist, idGame);
		
	return minitownlist->GetAvailableGameId(idAppClient, eGameSubType, setup);
}
#endif


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void MINITOWN::FreeGameId(
	GAMEID idGame)
{
	ASSERT_RETURN(idGame != INVALID_GAMEID);

	CSLAutoLock autolock(&m_CS);
	
	unsigned int listcount = m_List.Count();
	for (unsigned int ii = 0; ii < listcount; ++ii)
	{
		m_List[ii].FreeGameId(idGame);
	}
}
#endif

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL MINITOWN::StatTrace(
	unsigned int level)
{
	CSLAutoLock autolock(&m_CS);

	MINITOWN_LIST key;
	key.m_Level = level;
	MINITOWN_LIST * minitownlist = m_List.FindExact(key);

	ASSERT_RETFALSE(minitownlist);
	return minitownlist->StatTrace();
}
#endif


// ---------------------------------------------------------------------------
// EXPORTED FUNCTIONS
// ---------------------------------------------------------------------------
//----------------------------------------------------------------------------
// DESC:	sets up the minitowns structure for an app
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL MiniTownInit(
	MINITOWN * & minitown)
{
	MiniTownFree(minitown);
	
	minitown = (MINITOWN *)MALLOCZ(NULL, sizeof(MINITOWN));
	if (!minitown->Init())
	{
		MiniTownFree(minitown);
		return FALSE;
	}
	return TRUE;
}
#endif


// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static void sMiniTownListDelete(
	MEMORYPOOL * pool,
	MINITOWN_LIST & list)
{
	list.m_GameList.Destroy(NULL);
}
#endif


//----------------------------------------------------------------------------
// DESC:	frees the minitowns structure for an app
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void MiniTownFree(	
	MINITOWN * & minitown)
{
	if (!minitown)
	{
		return;
	}
	minitown->m_List.Destroy(NULL, sMiniTownListDelete);
	
	FREE(NULL, minitown);
	minitown = NULL;
}
#endif


//----------------------------------------------------------------------------
// gets a game id for a minitown level (single player)
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static GAMEID sMiniTownGetAvaliableGameId_SP(
	MINITOWN * & minitown,
	int level,
	GAME_SUBTYPE eGameSubType,
	const GAME_SETUP * setup /* = NULL */,
	int nDifficulty = 0)
{
	GAMEID idGame = AppGetSrvGameId();
	if (idGame == INVALID_ID)
	{
		idGame = sCreateMiniTownGame(eGameSubType, setup, nDifficulty);
	}
	return idGame;
}
#endif


//----------------------------------------------------------------------------
// gets a game id for a minitown level
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
GAMEID MiniTownGetAvailableGameId(
	APPCLIENTID idAppClient,
	int nLevelDef,
	const GAME_SETUP &tSetup,
	int nDifficulty /*= 0*/)
{

	GameServerContext * serverContext = (GameServerContext*)CurrentSvrGetContext();
	ASSERT_RETINVALID( serverContext );
	MINITOWN *minitown = serverContext->m_MiniTown;

	// multiplayer should always create towns of default difficulty and
	// with no special game flags.  We will want to change this
	// for a league server/town when we have it
	if (AppIsMultiplayer())
	{
		const GAME_VARIANT &tGameVariant = tSetup.tGameVariant;
		ASSERTV( nDifficulty == GlobalIndexGet( GI_DIFFICULTY_DEFAULT ), "Multipalyer town can only be created with default difficulty" );
		ASSERTV( tGameVariant.nDifficultyCurrent == GlobalIndexGet( GI_DIFFICULTY_DEFAULT ), "Multipalyer town can only be created with default difficulty" );
		ASSERTV( AppIsTugboat() || tGameVariant.dwGameVariantFlags == 0, "Mutliplayer town not allowed to have any special game variant flags (yet)" );
	}
	
	GAME_SUBTYPE eGameSubType = GetGameSubTypeByLevel(nLevelDef);

	// if it's single player, simply return the SrvGameId
	if (AppIsHellgate() && !AppTestDebugFlag(ADF_SP_USE_INSTANCE) && AppGetType() == APP_TYPE_SINGLE_PLAYER)
	{
		return sMiniTownGetAvaliableGameId_SP(minitown, nLevelDef, eGameSubType, &tSetup, nDifficulty);
	}
	else
	{
		return minitown->GetAvailableGameId(idAppClient, nLevelDef, eGameSubType, &tSetup);
	}
}
#endif

//----------------------------------------------------------------------------
// traces the stats of the minitownlist for the particular level def
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL MiniTownStatTrace(
	int nLevelDef)
{
	GameServerContext * serverContext = (GameServerContext*)CurrentSvrGetContext();
	ASSERT_RETFALSE( serverContext );
	MINITOWN *minitown = serverContext->m_MiniTown;

	return minitown->StatTrace(nLevelDef);
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void MiniTownFreeGameId(
	MINITOWN * & minitown,
	GAMEID idGame)
{
#if !ISVERSION(SERVER_VERSION)
	if (AppGetSrvGameId() == idGame)
	{
		AppSetSrvGameId(INVALID_ID);
		return;
	}
#endif //!SERVER_VERSION
	
	ASSERT_RETURN(minitown);
	minitown->FreeGameId(idGame);
}
#endif
