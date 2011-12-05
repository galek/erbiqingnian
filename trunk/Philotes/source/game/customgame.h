//----------------------------------------------------------------------------
// customgame.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _CUSTOMGAME_H_
#define _CUSTOMGAME_H_


//----------------------------------------------------------------------------
// EXPORTED FUNCTIONS
//----------------------------------------------------------------------------
BOOL s_CustomGameSetup(
	GAME * game,
	const GAME_SETUP * game_setup);

void s_CustomGameEventAddPlayer(
	GAME * game,
	UNIT * player);

void s_CustomGameEventKill(
	GAME * game,
	UNIT * attacker,
	UNIT * defender);

struct ROOM * s_CustomGameGetStartPosition(
	GAME * game,
	UNIT * unit,
	LEVELID idLevel,
	VECTOR & vPosition, 
	VECTOR & vFaceDirection,
	int * pnRotation = NULL);


#endif // _CUSTOMGAME_H_
