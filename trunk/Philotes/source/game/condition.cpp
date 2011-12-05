//----------------------------------------------------------------------------
// condition.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "prime.h"
#include "game.h"
#include "units.h"
#include "excel_private.h"
#include "unittypes.h"
#include "condition.h"
#include "states.h"
#include "skills.h"
#include "e_main.h"
#include "characterclass.h"
#include "monsters.h"
#include "gameunits.h"
#include "wardrobe.h"
#include "uix_priv.h"
#include "unitmodes.h"

//----------------------------------------------------------------------------
struct CONDITION_INPUT
{
	GAME * pGame;
	UNIT * pUnit;
	UNIT * pSource;
	const CONDITION_DEFINITION * pDef;
	int nSkill;
	UNITID idWeapon;
	UNITTYPE nUnitType;
};

//----------------------------------------------------------------------------
struct CONDITION_LOOKUP
{
	const char *pszName;
	CONDITION_FUNCTION fpHandler;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ConditionDefinitionInit(
	CONDITION_DEFINITION & tDef )
{
	tDef.nState = INVALID_ID;
	tDef.nType = INVALID_ID;
	tDef.nUnitType = INVALID_ID;
	tDef.nMonsterClass = INVALID_ID;
	tDef.nObjectClass = INVALID_ID;
	tDef.nSkill = INVALID_ID;
	tDef.nStat = INVALID_ID;
	for(int i=0; i<DWORD_FLAG_SIZE(NUM_CONDITION_FLAGS); i++)
	{
		tDef.dwFlags[i] = 0;
	}
	for ( int i = 0; i < MAX_CONDITION_PARAMS; i++ )
	{
		tDef.tParams[ i ].fValue = 0.0f;
		tDef.tParams[ i ].nValue = 0;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ConditionDefinitionPostLoad(
	CONDITION_DEFINITION & tDef )
{
	for ( int i = 0; i < MAX_CONDITION_PARAMS; i++ )
	{
		tDef.tParams[ i ].nValue = (int)tDef.tParams[ i ].fValue;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ConditionEvalute(
	UNIT * pUnit,
	const CONDITION_DEFINITION & tDef,
	int nSkill,
	UNITID idWeapon,
	STATS * pStatsForState,
	UNITTYPE * pnUnitType )
{
	if ( tDef.nType == INVALID_ID )
		return TRUE;

	// This function may pass a unittype (for tool-mode operations where a unit is not available)
	// Letting it return out below, maintaining old behavior prior to the pnUnitType addition.

	GAME * pGame = pUnit ? UnitGetGame( pUnit ) : NULL;

	CONDITION_FUNCTION_DATA *pData = (CONDITION_FUNCTION_DATA *)ExcelGetData( pGame, DATATABLE_CONDITION_FUNCTIONS, tDef.nType );
	if ( !pData->pfnHandler )
		return TRUE;

	CONDITION_INPUT tInput;
	tInput.pDef = & tDef;
	tInput.pGame = pGame;
	tInput.pSource = pUnit;
	tInput.pUnit = pUnit;
	tInput.nSkill = nSkill;
	tInput.idWeapon = idWeapon;
	if ( pUnit )
		tInput.nUnitType = (UNITTYPE)pUnit->nUnitType;
	else if ( pnUnitType )
		tInput.nUnitType = *(UNITTYPE*)pnUnitType;
	else
		tInput.nUnitType = UNITTYPE_NONE;

	if ( TESTBIT( tDef.dwFlags, CONDITION_BIT_CHECK_OWNER ) )
	{
		tInput.pUnit = UnitGetOwnerUnit( pUnit );
	}
	else if ( TESTBIT( tDef.dwFlags, CONDITION_BIT_CHECK_TARGET ) )
	{
		tInput.pUnit = SkillGetAnyTarget( pUnit, nSkill );
	}
	else if ( TESTBIT( tDef.dwFlags, CONDITION_BIT_CHECK_WEAPON ) )
	{
		UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT ];
		UnitGetWeapons( pUnit, nSkill, pWeapons, FALSE );
		tInput.pUnit = pWeapons[ 0 ];
	}
	else if ( TESTBIT( tDef.dwFlags, CONDITION_BIT_CHECK_STATE_SOURCE ) && pStatsForState )
	{
		UNITID idSourceUnit = StatsGetStat(pStatsForState, STATS_STATE_SOURCE);
		tInput.pUnit = UnitGetById(pGame, idSourceUnit);
	}

	if ( ! tInput.pUnit && ! pnUnitType )
		return FALSE;

	if ( TESTBIT( tDef.dwFlags, CONDITION_BIT_NOT_DEAD_OR_DYING ) &&
		IsUnitDeadOrDying( tInput.pUnit ))
		return FALSE;

	if ( TESTBIT( tDef.dwFlags, CONDITION_BIT_IS_YOUR_PLAYER ) &&
		tInput.pUnit != GameGetControlUnit( pGame ) )
		return FALSE;

	if ( TESTBIT( tDef.dwFlags, CONDITION_BIT_OWNER_IS_YOUR_PLAYER ) &&
		UnitGetOwnerUnit(tInput.pUnit) != GameGetControlUnit( pGame ) )
		return FALSE;

	if ( pData->bNot )
		return !pData->pfnHandler( tInput );
	else
		return pData->pfnHandler( tInput );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sConditionHasState( CONDITION_INPUT & tInput )
{
	return UnitHasState( tInput.pGame, tInput.pUnit, tInput.pDef->nState );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sHasSkillLevel(
	UNIT *pUnit,
	int nSkill,
	int nSkillLevel)
{
	return SkillGetLevel( pUnit, nSkill ) >= nSkillLevel;
}
static BOOL sConditionHasSkillLevel( CONDITION_INPUT & tInput )
{
	return sHasSkillLevel( tInput.pUnit, tInput.pDef->nSkill, tInput.pDef->tParams[0].nValue );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sConditionOwnerHasSkillLevel( CONDITION_INPUT & tInput )
{
	if (tInput.pUnit)
	{
		UNIT *pOwner = UnitGetOwnerUnit( tInput.pUnit );
		if (pOwner)
		{
			return sHasSkillLevel( pOwner, tInput.pDef->nSkill, tInput.pDef->tParams[0].nValue );		
		}
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sConditionHasStateAndSkillLevel( CONDITION_INPUT & tInput )
{
	return (UnitHasState( tInput.pGame, tInput.pUnit, tInput.pDef->nState ) &&
		SkillGetLevel( tInput.pUnit, tInput.pDef->nSkill ) >= tInput.pDef->tParams[0].nValue);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sConditionIsUnitType( CONDITION_INPUT & tInput )
{
	return UnitIsA( tInput.nUnitType, tInput.pDef->nUnitType );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sConditionIsExactMonsterClass( CONDITION_INPUT & tInput )
{
	return UnitGetGenus( tInput.pUnit ) == GENUS_MONSTER &&
		   UnitGetClass( tInput.pUnit ) == tInput.pDef->nMonsterClass;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sConditionIsMonsterFamily( CONDITION_INPUT & tInput )
{
	return UnitGetGenus( tInput.pUnit ) == GENUS_MONSTER &&
		   tInput.pUnit->pUnitData->nFamily == tInput.pDef->nMonsterClass;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sConditionHeightInRange( CONDITION_INPUT & tInput )
{
	float fHeight = UnitGetCollisionHeight( tInput.pUnit );
	return ( fHeight > tInput.pDef->tParams[ 0 ].fValue && fHeight <= tInput.pDef->tParams[ 1 ].fValue );
}
static BOOL sConditionHeightGreaterThan( CONDITION_INPUT & tInput )
{
	float fHeight = UnitGetCollisionHeight( tInput.pUnit );
	return ( fHeight > tInput.pDef->tParams[ 0 ].fValue );
}
static BOOL sConditionHeightLessThan( CONDITION_INPUT & tInput )
{
	float fHeight = UnitGetCollisionHeight( tInput.pUnit );
	return ( fHeight <= tInput.pDef->tParams[ 0 ].fValue );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sConditionIsYourPlayer( CONDITION_INPUT & tInput )
{
	if(IS_CLIENT(tInput.pGame))
	{
		if(GameGetControlUnit(tInput.pGame) == tInput.pUnit)
		{
			return TRUE;
		}
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sConditionGetStatValue( CONDITION_INPUT & tInput )
{
	ASSERT_RETINVALID(tInput.pGame);
	ASSERT_RETINVALID(tInput.pUnit);
	ASSERT_RETINVALID(tInput.pDef->nStat >= 0);
	return UnitGetStat(tInput.pUnit, tInput.pDef->nStat);
}
static BOOL sConditionStatEquals( CONDITION_INPUT & tInput )
{
	int nStat = sConditionGetStatValue(tInput);
	if(nStat >= 0)
	{
		return (nStat == tInput.pDef->tParams[0].nValue);
	}
	return FALSE;
}
static BOOL sConditionStatInRange( CONDITION_INPUT & tInput )
{
	int nStat = sConditionGetStatValue(tInput);
	if(nStat >= 0)
	{
		return ((nStat >= tInput.pDef->tParams[0].nValue) && (nStat <= tInput.pDef->tParams[1].nValue));
	}
	return FALSE;
}
static BOOL sConditionStatGreater( CONDITION_INPUT & tInput )
{
	int nStat = sConditionGetStatValue(tInput);
	if(nStat >= 0)
	{
		return (nStat > tInput.pDef->tParams[0].nValue);
	}
	return FALSE;
}
static BOOL sConditionStatLessThan( CONDITION_INPUT & tInput )
{
	int nStat = sConditionGetStatValue(tInput);
	if(nStat >= 0)
	{
		return (nStat < tInput.pDef->tParams[0].nValue);
	}
	return FALSE;
}
static BOOL sConditionStatPercentGreater( CONDITION_INPUT & tInput )
{
	int nStat = sConditionGetStatValue(tInput);
	if(nStat >= 0)
	{
		int maxStat = StatsDataGetMaxStat(tInput.pGame, tInput.pDef->nStat);
		if (maxStat >= 0)
		{
			int nMax = UnitGetStat(tInput.pUnit, maxStat);
			int nPercent = nStat * 100 / nMax;
			return (nPercent > tInput.pDef->tParams[0].nValue);
		}
	}
	return FALSE;
}
static BOOL sConditionStatPercentLessThan( CONDITION_INPUT & tInput )
{
	int nStat = sConditionGetStatValue(tInput);
	if(nStat >= 0)
	{
		int maxStat = StatsDataGetMaxStat(tInput.pGame, tInput.pDef->nStat);
		if (maxStat >= 0)
		{
			int nMax = UnitGetStat(tInput.pUnit, maxStat);
			int nPercent = nStat * 100 / nMax;
			return (nPercent < tInput.pDef->tParams[0].nValue);
		}
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sConditionIsOutdoors( CONDITION_INPUT & tInput )
{
	ROOM * pRoom = UnitGetRoom( tInput.pUnit );
	return pRoom ? RoomIsOutdoorRoom( tInput.pGame, pRoom ) : FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sConditionDX10IsEnabled( CONDITION_INPUT & tInput )
{
	SVR_VERSION_ONLY(return FALSE);
	CLT_VERSION_ONLY(return e_DX10IsEnabled());
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sConditionIsMale( CONDITION_INPUT & tInput )
{
	GENDER eGender = UnitGetGender(tInput.pUnit);
	return eGender == GENDER_MALE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sConditionIsFemale( CONDITION_INPUT & tInput )
{
	GENDER eGender = UnitGetGender(tInput.pUnit);
	return eGender == GENDER_FEMALE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sConditionIsDualWielding( CONDITION_INPUT & tInput )
{
	int nWardrobe = UnitGetWardrobe( tInput.pUnit );
	if ( nWardrobe == INVALID_ID )
		return FALSE;
	for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
	{
		UNIT * pWeapon = WardrobeGetWeapon( tInput.pGame, nWardrobe, i );
		if ( ! pWeapon )
			return FALSE;
		if ( ! UnitIsA( pWeapon, tInput.pDef->nUnitType ) )
			return FALSE;
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sConditionHasStateAndIsDualWielding(CONDITION_INPUT &tInput)
{
	return sConditionHasState(tInput) && sConditionIsDualWielding(tInput);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sConditionIsExactObjectClass( CONDITION_INPUT & tInput )
{
	return UnitGetGenus( tInput.pUnit ) == GENUS_OBJECT &&
		   UnitGetClass( tInput.pUnit ) == tInput.pDef->nObjectClass;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sConditionClientControlUnitCanUse( CONDITION_INPUT & tInput )
{
	UNIT *pUnit = tInput.pUnit;
	ASSERTX_RETFALSE( pUnit, "Expected unit" );
	GAME *pGame = UnitGetGame( pUnit );
	if (IS_CLIENT( pGame ))
	{
#if !ISVERSION(SERVER_VERSION)	
		UNIT *pPlayer = UIGetControlUnit();
		if (pPlayer)
		{
			ASSERTX_RETFALSE( pPlayer, "Expected player" );
			return UnitCanUse( pPlayer, pUnit );
		}
#endif		
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sConditionIsAlive( CONDITION_INPUT & tInput )
{
	return !IsUnitDeadOrDying(tInput.pUnit);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sConditionIsRunning( CONDITION_INPUT & tInput )
{
	return UnitTestMode( tInput.pUnit, MODE_RUN );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sConditionGoreAllowed( CONDITION_INPUT & tInput )
{
	return !AppTestFlag( AF_CENSOR_NO_GORE );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sConditionSkillIsSuccessful( CONDITION_INPUT & tInput )
{
	return SkillIsOnTestFlag(tInput.pUnit, SIO_SUCCESSFUL_BIT, tInput.nSkill, tInput.idWeapon);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sConditionCanDestroyDead( CONDITION_INPUT & tInput )
{
	const UNIT_DATA * pUnitData = UnitGetData( tInput.pUnit );
	if ( pUnitData )
		return ! UnitDataTestFlag( pUnitData, UNIT_DATA_FLAG_NEVER_DESTROY_DEAD );
	return TRUE;
}

//----------------------------------------------------------------------------
// KCK: Used to test if the target can see the unit specified. Designed for
// use with the flash-bang grenade. Ideally this will test for both LOS and
// facing.
//----------------------------------------------------------------------------
static BOOL sConditionCanSeeSource( CONDITION_INPUT & tInput )
{
	// Monsters can always "see"
	if (UnitIsA(tInput.pUnit, UNITTYPE_MONSTER))
		return true;

	// Players are more complicated
	if (UnitIsA(tInput.pUnit, UNITTYPE_PLAYER))
	{
		ASSERT_RETFALSE(tInput.pSource);

		// KCK: Simple check, see if it's in player's 180 deg forward arc
		VECTOR	vDelta = tInput.pSource->vPosition - tInput.pUnit->vPosition;
		float	fDot = VectorDot(tInput.pUnit->vFaceDirection, vDelta);
		if (fDot < 0.0f)
			return false;

		// Do LOS check
		VECTOR vPlayerPos = UnitGetAimPoint( tInput.pUnit );
		vDelta = vPlayerPos - tInput.pSource->vPosition;
		float fDistance = VectorLength( vDelta );
		VECTOR vDirection;
		if ( fDistance != 0.0f )
			VectorScale( vDirection, vDelta, 1.0f / fDistance );
		else
			vDirection = VECTOR( 0, 1, 0 );

		if ( fDistance )
		{
			LEVEL* pLevel = UnitGetLevel( tInput.pUnit );
			float fDistToCollide = LevelLineCollideLen( tInput.pGame, pLevel, vPlayerPos, vDirection, fDistance );
			if ( fDistToCollide < fDistance )
				return false;
		}

		return true;
	}

	return false;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static const CONDITION_LOOKUP sgtConditionFunctionLookupTable[] = 
{
	{	"Has State",					sConditionHasState					},
	{	"Has Skill Level",				sConditionHasSkillLevel				},		
	{	"Owner Has Skill Level",		sConditionOwnerHasSkillLevel		},			
	{	"Has State and Skill Level",	sConditionHasStateAndSkillLevel		},
	{	"Has State and Is Dual Wielding",sConditionHasStateAndIsDualWielding},
	{	"Is UnitType",					sConditionIsUnitType				},
	{	"Height In Range",				sConditionHeightInRange				},
	{	"Height Greater Than",			sConditionHeightGreaterThan			},
	{	"Height Less Than",				sConditionHeightLessThan			},
	{	"Is Your Player",				sConditionIsYourPlayer				},
	{	"Stat Equals",					sConditionStatEquals				},
	{	"Stat In Range",				sConditionStatInRange				},
	{	"Stat Greater",					sConditionStatGreater				},
	{	"Stat Less",					sConditionStatLessThan				},
	{	"Stat Percent Greater",			sConditionStatPercentGreater		},
	{	"Stat Percent Less Than",		sConditionStatPercentLessThan		},
	{	"Is Outdoors",					sConditionIsOutdoors				},
	{	"DX10 is enabled",				sConditionDX10IsEnabled				},
	{	"Is Male",						sConditionIsMale					},
	{	"Is Female",					sConditionIsFemale					},
	{	"Is Exact Monster Class",		sConditionIsExactMonsterClass		},
	{	"Is MonsterClass",				sConditionIsExactMonsterClass		},
	{	"Is Monster Family",			sConditionIsMonsterFamily			},
	{	"Is Dual Wielding",				sConditionIsDualWielding			},
	{	"Is Exact Object Class",		sConditionIsExactObjectClass		},
	{	"Client Control Unit Can Use",	sConditionClientControlUnitCanUse	},
	{	"Is Alive",						sConditionIsAlive					},
	{	"Gore Allowed",					sConditionGoreAllowed				},
	{	"Skill Is Successful",			sConditionSkillIsSuccessful			},
	{	"Can Destroy Dead",				sConditionCanDestroyDead			},
	{	"Can See Source",				sConditionCanSeeSource				},
	{	"Is Running",					sConditionIsRunning					},
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ConditionFunctionsExcelPostProcess( 
	EXCEL_TABLE * table)
{
	ASSERT_RETFALSE(table);

	for (unsigned int ii = 0; ii < ExcelGetCountPrivate(table); ++ii)
	{
		CONDITION_FUNCTION_DATA * condition_data = (CONDITION_FUNCTION_DATA *)ExcelGetDataPrivate(table, ii);
		ASSERT_RETFALSE(condition_data);

		condition_data->pfnHandler = NULL;

		const char * pszFunctionName = condition_data->szFunction;

		if (!pszFunctionName)
		{
			continue;
		}

		int nFunctionNameLength = PStrLen(pszFunctionName);
		if (nFunctionNameLength <= 0)
		{
			continue;
		}

		for (unsigned int jj = 0; sgtConditionFunctionLookupTable[jj].pszName != NULL; ++jj)
		{
			const CONDITION_LOOKUP * pLookup = &sgtConditionFunctionLookupTable[jj];
			if (PStrICmp(pLookup->pszName, pszFunctionName) == 0)
			{
				condition_data->pfnHandler = pLookup->fpHandler;
				break;
			}
		}

		if (condition_data->pfnHandler == NULL)
		{
			ASSERTV_CONTINUE(0, "Unable to find condition handler '%s' for '%s' on row '%d'", condition_data->szFunction,  condition_data->szName, ii);
		}
	}

	return TRUE;
}
