//----------------------------------------------------------------------------
// FILE: openlevel.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//
// this file handles auto-party
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "clients.h"
#include "dungeon.h"
#include "game.h"
#include "gameglobals.h"
#include "level.h"
#include "warp.h"
#include "openlevel.h"
#include "s_partyCache.h"
#include "servers.h"
#include "ServerRunnerAPI.h"
#include "gamevariant.h"

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
static const DWORD OPEN_LEVEL_TIME_IN_MS =		(DWORD)(MSECS_PER_SEC * SECONDS_PER_MINUTE * 1.5);	// how long auto-party lasts
static const DWORD OPEN_LEVEL_TICK_THRESHOLD =	(GAME_TICKS_PER_SECOND * 15);
static const BOOL sgbOpenLevelSystemInUse = FALSE;  // open level system is now disabled
static const BOOL sgbOpenLevelSystemInUseTugboat = FALSE;  // open level system is now disabled

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// these contain potential levels for auto-party formation
//----------------------------------------------------------------------------
struct OPEN_LEVEL
{
	CCritSectLite m_cs;				// critical section for the open level
	DWORD m_dwPostTimeInMS;			// time this open level was posted
	OPEN_LEVEL *pNext;				// next
	OPEN_GAME_INFO tOpenGameInfo;	// info about this open game
};

//----------------------------------------------------------------------------
// game server level array of open levels for auto-party formation
//----------------------------------------------------------------------------
struct ALL_OPEN_LEVELS
{
	MEMORYPOOL *					m_MemPool;
	OPEN_LEVEL 					m_OpenLevels[MAX_DUNGEON_LEVELS];
};
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static void sOpenLevelInit( 
	OPEN_LEVEL & tOpenLevel)
{
	tOpenLevel.m_dwPostTimeInMS = 0;
	OpenGameInfoInit( tOpenLevel.tOpenGameInfo );
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static void sOpenLevelFree(	
	OPEN_LEVEL & tOpenLevel)
{
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
ALL_OPEN_LEVELS * sGetAllOpenLevels(
	 void)
{
	GameServerContext * serverContext = (GameServerContext *)CurrentSvrGetContext();
	ASSERT_RETNULL(serverContext);
	return serverContext->m_pAllOpenLevels;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static OPEN_LEVEL * sGetOpenLevel(
	ALL_OPEN_LEVELS *pAllOpenLevels,
	const LEVEL_TYPE & tLevelType)
{
	ASSERT_RETNULL(pAllOpenLevels);
	
	if( AppIsHellgate() )
	{
		ASSERT_RETNULL(tLevelType.nLevelDef != INVALID_LINK);
		return &pAllOpenLevels->m_OpenLevels[tLevelType.nLevelDef];
	}
	else
	{
		ASSERT_RETNULL(tLevelType.nLevelArea != INVALID_LINK);
		for (int i = 0; i < MAX_DUNGEON_LEVELS; ++i)
		{
			const OPEN_LEVEL *pOpenLevel = &pAllOpenLevels->m_OpenLevels[ i ];
			if( pOpenLevel->tOpenGameInfo.idGame == INVALID_ID )
			{
				return &pAllOpenLevels->m_OpenLevels[ i ];
			}
		}
		return NULL;
	}
}

static OPEN_LEVEL * sGetOpenLevelByGameId(
								  ALL_OPEN_LEVELS *pAllOpenLevels,
								  const LEVEL_TYPE & tLevelType,
								  GAMEID idGame )
{
	ASSERT_RETNULL(pAllOpenLevels);

	if( AppIsHellgate() )
	{
		ASSERT_RETNULL(tLevelType.nLevelDef != INVALID_LINK);
		return &pAllOpenLevels->m_OpenLevels[tLevelType.nLevelDef];
	}
	else
	{
		ASSERT_RETNULL(tLevelType.nLevelArea != INVALID_LINK);
		for (int i = 0; i < MAX_DUNGEON_LEVELS; ++i)
		{
			const OPEN_LEVEL *pOpenLevel = &pAllOpenLevels->m_OpenLevels[ i ];
			if( pOpenLevel->tOpenGameInfo.idGame == idGame )
			{
				return &pAllOpenLevels->m_OpenLevels[ i ];
			}
		}
		return NULL;
	}
}

static OPEN_LEVEL * sGetFreeOpenLevel(
								  ALL_OPEN_LEVELS *pAllOpenLevels,
								  const LEVEL_TYPE & tLevelType)
{
	ASSERT_RETNULL(pAllOpenLevels);

	if( AppIsHellgate() )
	{
		ASSERT_RETNULL(tLevelType.nLevelDef != INVALID_LINK);
		return &pAllOpenLevels->m_OpenLevels[tLevelType.nLevelDef];
	}
	else
	{
		ASSERT_RETNULL(tLevelType.nLevelArea != INVALID_LINK);
		for (int i = 0; i < MAX_DUNGEON_LEVELS; ++i)
		{
			const OPEN_LEVEL *pOpenLevel = &pAllOpenLevels->m_OpenLevels[ i ];
			if (LevelTypesAreEqual( pOpenLevel->tOpenGameInfo.tLevelType, tLevelType ) )
			{
				const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION* pLevelArea = MYTHOS_LEVELAREAS::LevelAreaDefinitionGet( tLevelType.nLevelArea );
				int nOpenPlayers = pLevelArea->nMaxOpenPlayerSize;
				if( tLevelType.bPVPWorld )
				{
					nOpenPlayers = pLevelArea->nMaxOpenPlayerSizePVPWorld;
				}
				if( pLevelArea &&
					pOpenLevel->tOpenGameInfo.nNumPlayersInOrEnteringGame + 1 <= nOpenPlayers )
				{
					return &pAllOpenLevels->m_OpenLevels[ i ];
				}
			}
		}
		return NULL;
	}
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static OPEN_LEVEL * sGetOpenLevelByIndex(
	ALL_OPEN_LEVELS * pAllOpenLevels,
	unsigned int index)
{
	ASSERT_RETNULL(pAllOpenLevels);
	ASSERT_RETNULL(index < MAX_DUNGEON_LEVELS);
	return &pAllOpenLevels->m_OpenLevels[index];		
}
#endif


//----------------------------------------------------------------------------
// EXPORTED FUNCTIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void OpenGameInfoInit(
	struct OPEN_GAME_INFO &tOpenGameInfo)
{
	tOpenGameInfo.idGame = INVALID_ID;
	tOpenGameInfo.eSubType = GAME_SUBTYPE_INVALID;
	GameVariantInit( tOpenGameInfo.tGameVariant, NULL );
	LevelTypeInit( tOpenGameInfo.tLevelType );
	tOpenGameInfo.nNumPlayersInOrEnteringGame = 0;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
ALL_OPEN_LEVELS * OpenLevelInit(
	MEMORYPOOL * pool)
{
	ALL_OPEN_LEVELS * pAllOpenLevels = (ALL_OPEN_LEVELS *)MALLOCZ(pool, sizeof(ALL_OPEN_LEVELS));
	pAllOpenLevels->m_MemPool = pool;
	
	for (unsigned int ii = 0; ii < MAX_DUNGEON_LEVELS; ++ii)
	{
		sOpenLevelInit(pAllOpenLevels->m_OpenLevels[ii]);
	}
	
	return pAllOpenLevels;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void OpenLevelFree(
	ALL_OPEN_LEVELS * pAllOpenLevels)
{
	ASSERT_RETURN(pAllOpenLevels);

	for (unsigned int ii = 0; ii < MAX_DUNGEON_LEVELS; ++ii)
	{
		sOpenLevelFree(pAllOpenLevels->m_OpenLevels[ii]);
	}
	
	FREE(pAllOpenLevels->m_MemPool, pAllOpenLevels);		
}	
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL OpenLevelSystemInUse(
	void)
{
	return AppIsHellgate() ? sgbOpenLevelSystemInUse : sgbOpenLevelSystemInUseTugboat;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL LevelCanBeOpenLevel(
	const LEVEL_TYPE & tLevelType)
{
	/*if (AppIsTugboat())
	{
		return FALSE;
	}*/
	const LEVEL_DEFINITION * level_data = NULL;
	if( AppIsHellgate() )
	{
		level_data = LevelDefinitionGet(tLevelType.nLevelDef);
	}
	else
	{
		// it doesn't really matter which of the sub-definitions of the area we check
		// as they are always all of the same type
		int nDef = MYTHOS_LEVELAREAS::LevelAreaGetDefRandom( NULL, tLevelType.nLevelArea, 0 );
		level_data = LevelDefinitionGet(nDef);
	}
	ASSERT_RETFALSE(level_data);

	switch (level_data->eSrvLevelType)
	{
	case SRVLEVELTYPE_DEFAULT:
		return TRUE;

	case SRVLEVELTYPE_TUTORIAL:
	case SRVLEVELTYPE_MINITOWN:
	case SRVLEVELTYPE_OPENWORLD:
	case SRVLEVELTYPE_BIGTOWN:
	case SRVLEVELTYPE_CUSTOM:
	case SRVLEVELTYPE_PVP_PRIVATE:
	case SRVLEVELTYPE_PVP_CTF:
	default:
		return FALSE;
	}
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL GameSubTypeCanBeOpenLevel(
	GAME_SUBTYPE eSubType)
{
	switch (eSubType)
	{
		case GAME_SUBTYPE_DUNGEON:
			return TRUE;
		case GAME_SUBTYPE_TUTORIAL:
		case GAME_SUBTYPE_BIGTOWN:
		case GAME_SUBTYPE_MINITOWN:
		case GAME_SUBTYPE_OPENWORLD:
		case GAME_SUBTYPE_CUSTOM:
		case GAME_SUBTYPE_PVP_PRIVATE:
		case GAME_SUBTYPE_PVP_PRIVATE_AUTOMATED:
		case GAME_SUBTYPE_PVP_CTF:
		default: 
			return FALSE;
	}
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL GameCanHaveOpenLevels(
	GAME *pGame)
{
	ASSERT_RETFALSE(pGame && IS_SERVER(pGame));
	
	// bail out if we're not using the open level system
	if (OpenLevelSystemInUse() == FALSE)
	{
		return FALSE;
	}
	
	return GameSubTypeCanBeOpenLevel( GameGetSubType( pGame ) );
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void OpenLevelPost(
	OPEN_GAME_INFO &tGameInfo,
	OPEN_LEVEL_STATUS eStatus)
{

	// bail out if we're not using the open level system
	if (OpenLevelSystemInUse() == FALSE)
	{
		return;
	}
	
	// Mythos doesn't do these yet
	/*if (AppIsTugboat())
	{
		return;
	}*/
	
	// validate
	if (eStatus == OLS_OPEN)
	{

		// level must be able to be an open level		
		if (!LevelCanBeOpenLevel( tGameInfo.tLevelType ))
		{
			return;
		}

		// game type must be able to have open levels
		if (!GameSubTypeCanBeOpenLevel( tGameInfo.eSubType ))
		{
			return;
		}
		
	}
	
	ALL_OPEN_LEVELS * pAllOpenLevels = sGetAllOpenLevels();
	ASSERT_RETURN(pAllOpenLevels);

	OPEN_LEVEL * pOpenLevel = NULL;
	if( AppIsHellgate() || eStatus == OLS_OPEN )
	{
		pOpenLevel = sGetOpenLevel(pAllOpenLevels, tGameInfo.tLevelType);
	}
	else
	{
		pOpenLevel = sGetOpenLevelByGameId(pAllOpenLevels, tGameInfo.tLevelType, tGameInfo.idGame);
	}
	if( AppIsTugboat() && eStatus != OLS_OPEN )
	{
		if( !pOpenLevel)
		{
			return;
		}
	}
	ASSERT_RETURN(pOpenLevel);
	
	CSLAutoLock autolock(&pOpenLevel->m_cs);
	{
		if (eStatus == OLS_OPEN)
		{
		
			// for auto-party and open levels, we only want players coming into one
			// level of the game, so we will never allow a game to have more than
			// one open level, which should not happen, but this it just to be extra safe
			// note that we don't need to critical section each open level, we're just reading them
			for (int i = 0; i < MAX_DUNGEON_LEVELS; ++i)
			{
				const OPEN_LEVEL *pOpenLevel = &pAllOpenLevels->m_OpenLevels[ i ];
				if (pOpenLevel->tOpenGameInfo.idGame == tGameInfo.idGame &&
					!LevelTypesAreEqual( pOpenLevel->tOpenGameInfo.tLevelType, tGameInfo.tLevelType ))
				{
					return;
				}
			}
						
			// if the game is different than the one that is currently posted
			OPEN_GAME_INFO &tGameInfoExisting = pOpenLevel->tOpenGameInfo;
			if (tGameInfoExisting.idGame != tGameInfo.idGame)
			{
			
				// and this level has been open long enough to be considered
				// too old or has too many people to be an open level for people to join
				DWORD dwTimeSincePost = GetTickCount() - pOpenLevel->m_dwPostTimeInMS;
				if (dwTimeSincePost >= OPEN_LEVEL_TIME_IN_MS ||
					tGameInfoExisting.nNumPlayersInOrEnteringGame >= AUTO_PARTY_MAX_MEMBER_COUNT)
				{
			
					// this game is now the open level for this level type
					tGameInfoExisting = tGameInfo;
					pOpenLevel->m_dwPostTimeInMS = GetTickCount();			
					
				}
				
			}
						
		}
		else if (pOpenLevel->tOpenGameInfo.idGame == tGameInfo.idGame)
		{
			if( eStatus == OLS_LEAVING )
			{				
				pOpenLevel->tOpenGameInfo.nNumPlayersInOrEnteringGame--;
			}
			else
			{
				sOpenLevelInit(*pOpenLevel);
			}
		}
	}

}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void OpenLevelClearAllFromGame(
	GAME * pGame)
{
	ASSERT_RETURN(pGame && IS_SERVER(pGame));
	if (!GameCanHaveOpenLevels(pGame))
	{
		return;
	}
	GAMEID idGame = GameGetId(pGame);

	ALL_OPEN_LEVELS * pAllOpenLevels = sGetAllOpenLevels();
	ASSERT_RETURN(pAllOpenLevels);

	for (unsigned int ii = 0; ii < MAX_DUNGEON_LEVELS; ++ii)
	{
		OPEN_LEVEL * pOpenLevel = sGetOpenLevelByIndex(pAllOpenLevels, ii);
		ASSERT_CONTINUE(pOpenLevel);
		{
			CSLAutoLock autolock(&pOpenLevel->m_cs);
			if (pOpenLevel->tOpenGameInfo.idGame == idGame)
			{
				sOpenLevelInit(*pOpenLevel);
			}		
		}
	}		
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
GAMEID OpenLevelGetGame(
	const LEVEL_TYPE & tLevelType,
	const struct GAME_VARIANT &tGameVariant)
{
	GAMEID idGame = INVALID_ID;

	// bail out if we're not using the open level system
	if (OpenLevelSystemInUse() == FALSE)
	{
		return INVALID_ID;
	}

	// Mythos doesn't do these yet
	/*if (AppIsTugboat())
	{
		return idGame;
	}*/

	ASSERT_RETVAL(tLevelType.nLevelDef != INVALID_LINK, idGame);

	ALL_OPEN_LEVELS * pAllOpenLevels = sGetAllOpenLevels();
	ASSERT_RETVAL(pAllOpenLevels, idGame);

	OPEN_LEVEL * pOpenLevel = sGetFreeOpenLevel(pAllOpenLevels, tLevelType);
	if( AppIsTugboat() && !pOpenLevel )
	{
		return idGame;
	}
	ASSERT_RETVAL(pAllOpenLevels, idGame);

	{
		CSLAutoLock autolock(&pOpenLevel->m_cs);

		DWORD dwTimeSincePost = GetTickCount() - pOpenLevel->m_dwPostTimeInMS;
		REF( dwTimeSincePost );
		if (AppIsTugboat() || dwTimeSincePost <= OPEN_LEVEL_TIME_IN_MS)
		{
			OPEN_GAME_INFO &tOpenGameInfo = pOpenLevel->tOpenGameInfo;
			
			// must also match variant type
			DWORD variantDifferences = 0;
			if (GameVariantsAreEqual( tOpenGameInfo.tGameVariant, tGameVariant, GVC_ALL, &variantDifferences ))
			{
				
				if( AppIsHellgate() )
				{
					// must be able to hold another player
					if (tOpenGameInfo.nNumPlayersInOrEnteringGame + 1 <= AUTO_PARTY_MAX_MEMBER_COUNT)
					{

						// record that one more player is going into this open level
						tOpenGameInfo.nNumPlayersInOrEnteringGame++;

						// record this game
						idGame = tOpenGameInfo.idGame;

					}			
				}
				else
				{
					const MYTHOS_LEVELAREAS::LEVEL_AREA_DEFINITION* pLevelArea = MYTHOS_LEVELAREAS::LevelAreaDefinitionGet( tLevelType.nLevelArea );
					int nOpenPlayers = pLevelArea->nMaxOpenPlayerSize;
					if( tLevelType.bPVPWorld )
					{
						nOpenPlayers = pLevelArea->nMaxOpenPlayerSizePVPWorld;
					}

					if( pLevelArea &&
						tOpenGameInfo.nNumPlayersInOrEnteringGame + 1 <= nOpenPlayers )
					{

						// record that one more player is going into this open level
						tOpenGameInfo.nNumPlayersInOrEnteringGame++;

						// record this game
						idGame = tOpenGameInfo.idGame;
					}
				}
				
			}
			
		}
		
	}
		
	return idGame;	
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL OpenLevelGameCanAcceptNewPlayers(
	GAME * pGame)
{
	ASSERT_RETFALSE(pGame);
	
	int nPlayerCount = GameGetClientCount(pGame);
	if (nPlayerCount >= AUTO_PARTY_MAX_MEMBER_COUNT)
	{
		return FALSE;
	}
	
	return TRUE;
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void OpenLevelAutoFormParty( 
	GAME * pGame,
	PARTYID idParty)
{

	if (!GameCanHaveOpenLevels(pGame))
	{
		return;
	}

	// bail out if we're not using the open level system
	if (OpenLevelSystemInUse() == FALSE)
	{
		return;
	}
	
	// validate the party id ... it's possible that if two players join an open level
	// at exactly the same time, the server requests to create two parties for
	// the auto parties.  And because we have to wait for a response back from the
	// party server for the party id, there is no way to know where doing it.  So, if there
	// are people in this game, and they are already in a party, we will use that party
	// and not the duplicate one comign in here
	PARTYID idPartyToUse = pGame->m_idPartyAuto;
	
	// if no game party recorded yet, try to find one to use
	if (idPartyToUse == INVALID_ID)
	{
		for (GAMECLIENT *pClient = ClientGetFirstInGame( pGame ); 
			 pClient; 
			 pClient = ClientGetNextInGame( pClient ))
		{
			UNIT *pPlayer = ClientGetControlUnit( pClient );
			if (pPlayer)
			{
				PARTYID idPartyPlayer = UnitGetPartyId( pPlayer );
				if (idPartyPlayer != INVALID_ID)
				{
					ASSERTX( idPartyToUse == INVALID_ID || idPartyToUse == idPartyPlayer, "Multiple parties found in auto party game" );
					idPartyToUse = idPartyPlayer;
				}
			}		
		}
	}
	
	// if no players are in a party, use the auto party
	if (idPartyToUse == INVALID_ID)
	{
		idPartyToUse = idParty;
	}
		
	// the server has gotten a response back from the party system of a new party
	// that has been created in order to auto form up players together
	// go through all players, make them all in one party 
	// NOTE we're not actually forming the party unless we have two or more players
	if (GameGetInGameClientCount( pGame ) > 1)
	{
		for (GAMECLIENT * client = ClientGetFirstInGame(pGame); 
			 client; 
			 client = ClientGetNextInGame(client))
		{
			UNIT * player = ClientGetControlUnit(client);
			if (UnitGetPartyId( player ) != idPartyToUse)
			{
				PartyCacheAddToParty(player, idPartyToUse);		
			}
		}
	}
		
	// this party is now the game auto party
	pGame->m_idPartyAuto = idPartyToUse;
	
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void OpenLevelPlayerLeftLevel(
	GAME *pGame,
	LEVELID idLevelLeaving)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	
	if (idLevelLeaving != INVALID_ID)	
	{
		LEVEL *pLevelLeaving = LevelGetByID( pGame, idLevelLeaving );
		if (pLevelLeaving)
		{
			if (s_LevelGetPlayerCount( pLevelLeaving ) == 0)
			{
			
				OPEN_GAME_INFO tOpenGameInfo;
				tOpenGameInfo.idGame = GameGetId( pGame );
				tOpenGameInfo.eSubType = GameGetSubType( pGame );
				tOpenGameInfo.tGameVariant = pGame->m_tGameVariant;
				tOpenGameInfo.tLevelType = LevelGetType( pLevelLeaving );
				
				OpenLevelPost( tOpenGameInfo, OLS_CLOSED );
				
			}
			else if( AppIsTugboat() )
			{
				OPEN_GAME_INFO tOpenGameInfo;
				tOpenGameInfo.idGame = GameGetId( pGame );
				tOpenGameInfo.eSubType = GameGetSubType( pGame );
				tOpenGameInfo.tGameVariant = pGame->m_tGameVariant;
				tOpenGameInfo.tLevelType = LevelGetType( pLevelLeaving );

				OpenLevelPost( tOpenGameInfo, OLS_LEAVING );
			}
		}
	}
		
#endif
	
}