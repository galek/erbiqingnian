//----------------------------------------------------------------------------
// minitown.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _MINITOWN_H_
#define _MINITOWN_H_

struct MINITOWN;


//----------------------------------------------------------------------------
// EXPORTED FUNCTIONS
//----------------------------------------------------------------------------
// initialize minitwon list
#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL MiniTownInit(
	MINITOWN * & minitown);
#endif

// free minitown list
#if !ISVERSION(CLIENT_ONLY_VERSION)
void MiniTownFree(
	MINITOWN * & minitown);
#endif

// find an existing uncrowded minitown or create a new one
#if !ISVERSION(CLIENT_ONLY_VERSION)
GAMEID MiniTownGetAvailableGameId(
	APPCLIENTID idAppClient,
	int nLevelDef,
	const GAME_SETUP &tSetup,
	int nDifficulty = 0);
#endif

#if !ISVERSION(CLIENT_ONLY_VERSION)
void MiniTownFreeGameId(
	MINITOWN * & minitown,
	GAMEID idGame);
#endif

#if !ISVERSION(CLIENT_ONLY_VERSION)
BOOL MiniTownStatTrace(
	int nLevelDef);
#endif

#endif // _MINITOWN_H_

