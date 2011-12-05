//----------------------------------------------------------------------------
// player_move_msg.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _PLAYER_MOVE_MSG_H_
#define _PLAYER_MOVE_MSG_H_

//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#ifndef _UNITS_H_
#include "units.h"
#endif

#ifndef _UNIT_PRIV_H_
#include "unit_priv.h"
#endif

void c_PlayerSendMove(
	GAME * game,
	BOOL bSkipPredictionTest = FALSE );

void s_PlayerMoveMsgHandleRequest(
	GAME * pGame, 
	GAMECLIENT * client,
	UNIT * pUnit,
	BYTE * data);

void s_BotMoveMsgHandleRequest(
	GAME * pGame, 
	GAMECLIENT * client,
	UNIT * pUnit,
	BYTE * data);

BOOL PlayerMoveMsgSendBotMovement(
	struct MSG_CCMD_BOT_CHEAT & tMsg,
	struct PLAYER_MOVE_INFO * pPlayerMoveInfo,
	GAME_TICK nCurrentTick,
	BOOL bIsInTown,
	const VECTOR & vPosition,
	const VECTOR & vFaceDirection,
	float fSpeed,
	UNITMODE eForwardMode,
	UNITMODE eSideMode,
	LEVELID idLevel );

void PlayerMoveMsgInitPlayer( 
	UNIT * pUnit );

struct PLAYER_MOVE_INFO * PlayerMoveInfoInit(
	MEMORYPOOL *pPool);

void c_PlayerMoveMsgUpdateOtherPlayer( 
	UNIT * pUnit );

#endif // _PLAYER_MOVE_MSG_H_

