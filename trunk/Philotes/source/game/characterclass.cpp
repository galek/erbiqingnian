//----------------------------------------------------------------------------
// FILE: characterclass.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "excel.h"
#include "characterclass.h"
#include "player.h"
#include "unit_priv.h"
#include "units.h"

//----------------------------------------------------------------------------
// Function Definitions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL CharacterClassIsEnabled( 
	int nCharacterClass, 
	int nCharacterRace,
	GENDER eGender)
{
	ASSERTX_RETFALSE(nCharacterClass >= 0, "Invalid character class");
	
	const CHARACTER_CLASS_DATA *pCharClassData = CharacterClassGetData( nCharacterClass );
	if (!pCharClassData)
	{
		return FALSE;
	}

	ASSERTX_RETFALSE(eGender >= 0 && eGender < GENDER_NUM_CHARACTER_GENDERS, "Invalid character gender. (I hope you don't get that very often)");
	const CHARACTER_CLASS *pClass = &pCharClassData->tClasses[ eGender ];
	ASSERTX_RETFALSE(pClass, "Unable to find character class");
	
	ASSERTX_RETFALSE(nCharacterRace >= 0 && nCharacterRace < MAX_CHARACTER_RACES, "Invalid character race.");
	return pClass->nPlayerClass[nCharacterRace] != INVALID_LINK && pClass->bEnabled == TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const CHARACTER_CLASS_DATA *CharacterClassGetData(
	int nCharacterClass)
{
	if(nCharacterClass < 0)
		return NULL;
	return (const CHARACTER_CLASS_DATA *)ExcelGetData( NULL, DATATABLE_CHARACTER_CLASS, nCharacterClass );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int CharacterClassNumClasses(
	void)
{
	return ExcelGetNumRows( NULL, DATATABLE_CHARACTER_CLASS );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int CharacterClassGet(
	int nPlayerClass)
{
//	ASSERTX_RETINVALID( UnitIsA( nPlayerClass, UNITTYPE_PLAYER ), "Expected player" );  // can't mix UNITTYPE with class like this
	int nNumRaces = ExcelGetNumRows( NULL, DATATABLE_PLAYER_RACE );
	int nNumCharacterClasses = CharacterClassNumClasses();
	for (int i = 0; i < nNumCharacterClasses; ++i)
	{
		const CHARACTER_CLASS_DATA *pClassData = CharacterClassGetData( i );
		if(!pClassData)
			continue;

		for (int j = 0; j < GENDER_NUM_CHARACTER_GENDERS; ++j)
		{
			GENDER eGender = (GENDER)j;
			const CHARACTER_CLASS *pCharClass = &pClassData->tClasses[ eGender ];
			for( int k = 0; k < nNumRaces; k++ )
			{
				if (pCharClass->nPlayerClass[k] == nPlayerClass)
				{
					return i;
				}
			}
		}
	}

	return INVALID_LINK;
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int CharacterClassGet(
	UNIT *pPlayer)
{
	ASSERTX_RETINVALID( pPlayer, "Expected unit" );
	return CharacterClassGet( UnitGetClass( pPlayer ) );
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int CharacterClassGetPlayerClass( 
	int nCharacterClass, 
	int nCharacterRace, 
	GENDER eGender)
{
	ASSERTX_RETINVALID(nCharacterClass >= 0, "Invalid character class");

	if(!CharacterClassIsEnabled(nCharacterClass, nCharacterRace, eGender))
		return INVALID_ID;

	const CHARACTER_CLASS_DATA *pCharClassData = CharacterClassGetData( nCharacterClass );
	ASSERTX_RETINVALID(pCharClassData, "Unable to find character class data");

	ASSERTX_RETINVALID(eGender >= 0 && eGender < GENDER_NUM_CHARACTER_GENDERS, "Invalid character gender. (I hope you don't get that very often)");
	const CHARACTER_CLASS *pClass = &pCharClassData->tClasses[ eGender ];
	ASSERTX_RETINVALID(pClass, "Unable to find character class");

	ASSERTX_RETINVALID(nCharacterRace >= 0 && nCharacterRace < MAX_CHARACTER_RACES, "Invalid character race.");

	int nPlayerClass = pClass->nPlayerClass[nCharacterRace];
	ASSERTX_RETINVALID( nPlayerClass != INVALID_LINK, "No player class defined for requested character class and gender" );
	return nPlayerClass;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int CharacterClassGetDefault(	
	void)
{
	int nNumCharClasses = CharacterClassNumClasses();
	for (int i = 0; i < nNumCharClasses; ++i)
	{
		const CHARACTER_CLASS_DATA *pClassData = CharacterClassGetData( i );
		if (pClassData && pClassData->bDefault)
		{
			return i;
		}
	}
	ASSERTX_RETINVALID( 0, "No default character class in character datatable" );
}