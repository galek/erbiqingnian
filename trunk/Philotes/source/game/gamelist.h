//----------------------------------------------------------------------------
// gamelist.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _GAMELIST_H_
#define _GAMELIST_H_


#define DECOUPLE_GAME_LIST ISVERSION(SERVER_VERSION) // Set this to zero to enable old game list functionality

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------
struct GAMELIST;


//----------------------------------------------------------------------------
// EXPORTED FUNCTIONS
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
// initialize game list
BOOL GameListInit(
	struct MEMORYPOOL * pool,
	GAMELIST * & glist);
#endif

#if !ISVERSION(CLIENT_ONLY_VERSION)
// free game list
void GameListFree(
	GAMELIST * & glist);
#endif

#if !ISVERSION(CLIENT_ONLY_VERSION)
// initialize & return a new game
GAME * GameListAddGame(
	GAMELIST * glist,
	enum GAME_SUBTYPE eGameSubType,
	const struct GAME_SETUP * game_setup = NULL,
	int nDifficulty = 0);
#endif

#if !ISVERSION(CLIENT_ONLY_VERSION)
// simulate games
void GameListProcessGames(
	GAMELIST * glist,
	unsigned int sim_frames);
#endif

#if !ISVERSION(CLIENT_ONLY_VERSION)
// get messages
struct SERVER_MAILBOX * GameListGetPlayerToGameMailbox(
	GAMELIST * glist,
	GAMEID idGame);
#endif

#if !ISVERSION(CLIENT_ONLY_VERSION)
// returns the currently selected game on the server
struct GAME * AppGetSrvGame(
	void);
#endif

#if !ISVERSION(CLIENT_ONLY_VERSION)
void GameListReleaseGameData(
	GAMELIST * glist,
	GAME * & game);
#endif

#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL GameListQueryGameIsRunning(
	GAMELIST * glist,
	GAMEID idGame);
#endif

#if !ISVERSION(CLIENT_ONLY_VERSION)
void GameListMessageAllGames(
	GAMELIST * glist,
	NETCLIENTID64 netId,
	MSG_STRUCT * message,
	NET_CMD command);
#endif

#endif // _GAMELIST_H_
