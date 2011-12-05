//----------------------------------------------------------------------------
// FILE: openlevel.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifdef __OPENLEVEL_H_
//#pragma message ("    HINT: " __FILE__ " included multiple times in this translation unit")
#else
#define __OPENLEVEL_H_

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "gamevariant.h"

//----------------------------------------------------------------------------
// Forward Declarations
//----------------------------------------------------------------------------
struct MEMORYPOOL;
struct ALL_OPEN_LEVELS;
struct LEVEL_TYPE;

//----------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
enum OPEN_LEVEL_STATUS
{
	OLS_OPEN,
	OLS_CLOSED,
	OLS_LEAVING
};

void OpenGameInfoInit(
	struct OPEN_GAME_INFO &tOpenGameInfo);
	
//----------------------------------------------------------------------------
struct OPEN_GAME_INFO
{
	GAMEID idGame;					// the game id
	GAME_SUBTYPE eSubType;			// subtype of game
	GAME_VARIANT tGameVariant;		// variant information of game
	LEVEL_TYPE tLevelType;			// level type
	int nNumPlayersInOrEnteringGame;// # of player in or waiting to enter game
	
	OPEN_GAME_INFO::OPEN_GAME_INFO( void )
	{
		OpenGameInfoInit( *this );
	}
};

//----------------------------------------------------------------------------
// Exported Functions
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)

ALL_OPEN_LEVELS *OpenLevelInit(
	MEMORYPOOL *pPool);

void OpenLevelFree(
	ALL_OPEN_LEVELS *pAllOpenLevels);

BOOL OpenLevelSystemInUse(
	void);
	
BOOL LevelCanBeOpenLevel(
	const LEVEL_TYPE &tLevelType);

BOOL GameCanHaveOpenLevels(
	GAME *pGame);

BOOL GameSubTypeCanBeOpenLevel(
	GAME_SUBTYPE eSubType);

void OpenLevelPost(
	OPEN_GAME_INFO &tGameInfo,
	OPEN_LEVEL_STATUS eStatus);
	
void OpenLevelClearAllFromGame(
	GAME *pGame);
	
GAMEID OpenLevelGetGame(
	const LEVEL_TYPE &tLevelType,
	const struct GAME_VARIANT &tGameVariant);
	
BOOL OpenLevelGameCanAcceptNewPlayers(
	GAME *pGame);

void OpenLevelAutoFormParty( 
	GAME *pGame,
	PARTYID idParty);
	
#endif  // !ISVERSION(CLIENT_ONLY_VERSION)

void OpenLevelPlayerLeftLevel(
	GAME *pGame,
	LEVELID idLevelLeaving);

#endif