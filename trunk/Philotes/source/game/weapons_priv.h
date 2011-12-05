//----------------------------------------------------------------------------
// weapons_priv.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#ifndef _MISSILES_H_
#include "missiles.h"
#endif

#ifndef _COMBAT_H_
#include "combat.h"
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

struct MELEE_WEAPON_DATA
{
	char	szName[DEFAULT_INDEX_SIZE];
	char	szSwingEvents[DEFAULT_FILE_SIZE];
	int		nSwingEventsId;

	char	szEffectFolder[DEFAULT_FILE_SIZE];
	PARTICLE_EFFECT_SET pEffects[ NUM_COMBAT_RESULTS ];
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline const struct MELEE_WEAPON_DATA * MeleeWeaponGetData(
	GAME * pGame,
	int nMeleeWeapon)
{
	return (const MELEE_WEAPON_DATA *)ExcelGetData(pGame, DATATABLE_MELEEWEAPONS, nMeleeWeapon);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const MELEE_WEAPON_DATA * WeaponGetMeleeData(
	GAME * pGame,
	UNIT * pUnit );
