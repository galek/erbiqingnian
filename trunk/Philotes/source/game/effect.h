//----------------------------------------------------------------------------
// FILE: effect.h
// DESC: One shot effects
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __EFFECT_H_
#define __EFFECT_H_

//----------------------------------------------------------------------------
// Forward Declarations
//----------------------------------------------------------------------------
struct UNIT;
struct GAME;
struct ROOM;

//----------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
enum EFFECT_TYPE
{
	ET_INVALID = -1,
	
	ET_RESTORE_VITALS,
	
	ET_NUM_TYPES		// keep this last
};

//----------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------

void s_EffectAtUnit(
	UNIT *pUnit,
	EFFECT_TYPE eEffectType);

void s_EffectAtLocation(
	GAME *pGame,
	ROOM *pRoom,
	const VECTOR *pvLocation,
	EFFECT_TYPE eEffectType);

void c_EffectAtUnit(
	UNIT *pUnit,
	EFFECT_TYPE eEffectType);

void c_EffectAtLocation(
	const VECTOR *pvLocation,
	EFFECT_TYPE eEffectType);

#endif