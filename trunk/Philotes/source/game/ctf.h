//----------------------------------------------------------------------------
// ctf.h
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _CTF_H_
#define _CTF_H_

struct PvPGameData;

//----------------------------------------------------------------------------
// ENUMERATIONS
//----------------------------------------------------------------------------

enum CTF_FLAG_STATUS
{
	CTF_FLAG_STATUS_SAFE,
	CTF_FLAG_STATUS_CARRIED_BY_ENEMY,
	CTF_FLAG_STATUS_DROPPED_IN_WORLD,
	CTF_FLAG_STATUS_NOT_APPLICABLE,		// ignore status in non-CTF PvP games
};

//----------------------------------------------------------------------------
// EXPORTED FUNCTIONS
//----------------------------------------------------------------------------
BOOL CTFInit(
	GAME * game,
	const GAME_SETUP * game_setup);

void s_CTFEventAddPlayer(
	GAME * game,
	UNIT * player);

struct ROOM * s_CTFGetStartPosition(
	GAME * game,
	UNIT * unit,
	LEVELID idLevel,
	VECTOR & vPosition, 
	VECTOR & vFaceDirection,
	int * pnRotation = NULL);

BOOL c_CTFStartCountdown(int nCountdownSeconds);

void c_CTFEndMatch(int nScoreTeam0, int nScoreTeam1);

BOOL c_CTFGetGameData(PvPGameData *pPvPGameData);

#endif // _CTF_H_
