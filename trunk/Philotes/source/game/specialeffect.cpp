//----------------------------------------------------------------------------
// FILE: specialeffect.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "ptime.h"
#include "specialeffect.h"
#include "unit_priv.h"

//----------------------------------------------------------------------------
// DEFINTIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SpecialEffectPostProcess( 
	UNIT_DATA *pUnitData)
{
	ASSERTX_RETURN( pUnitData, "Expected unit data" );
	
	for (int i = 0; i < NUM_DAMAGE_TYPES; ++i)
	{
		SPECIAL_EFFECT *pSpecialEffect = &pUnitData->tSpecialEffect[ i ];
		
		// save duration in ticks
		pSpecialEffect->nDurationInTicks = (int)(pSpecialEffect->flDurationInSeconds * GAME_TICKS_PER_SECOND_FLOAT);
	}
		
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SpecialEffectExecute( 
	GAME *pGame,
	DAMAGE_TYPES eDamageType,
	UNIT *pAttacker,
	UNIT *pDefender,
	int nStrength,
	int nDurationInTicks)
{	
}
	