//----------------------------------------------------------------------------
// FILE: specialeffect.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __SPECIAL_EFFECT_H_
#define __SPECIAL_EFFECT_H_

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------
struct UNIT_DATA;
struct GAME;
enum DAMAGE_TYPES;
struct UNIT;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct SPECIAL_EFFECT
{
	int nAbilityPercent;		// percent of ability set in item levels table
	int nDefensePercent;		// percent of defense set in item levels table
	int nStrengthPercent;		// strength of effect (different for each type, knockback force, spectral reduction %, toxic strength etc)
	float flDurationInSeconds;	// duration in seconds
	int nDurationInTicks;		// duration in ticks
};

//----------------------------------------------------------------------------
// PROTOTYPES
//----------------------------------------------------------------------------

void SpecialEffectPostProcess( 
	UNIT_DATA *pUnitData);

void SpecialEffectExecute( 
	GAME *pGame,
	DAMAGE_TYPES eDamageType,
	UNIT *pAttacker,
	UNIT *pDefender,
	int nStrength,
	int nDurationInTicks);
	
#endif