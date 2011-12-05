//----------------------------------------------------------------------------
// missiles.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _MISSILES_PRIV_H_
#define _MISSILES_PRIV_H_

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------
enum MISSILE_EFFECT
{
	MISSILE_EFFECT_MUZZLE,
	MISSILE_EFFECT_TRAIL, // this is also used for a laser without a target
	MISSILE_EFFECT_PROJECTILE, // this is also used for a laser with a target
	MISSILE_EFFECT_IMPACT_WALL,
	MISSILE_EFFECT_IMPACT_UNIT,
	NUM_MISSILE_EFFECTS,
};

#endif