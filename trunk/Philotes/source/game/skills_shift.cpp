//----------------------------------------------------------------------------
// skills_shift.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "prime.h"
#include "game.h"

#if !ISVERSION(SERVER_VERSION)

#include "units.h"
#include "skills.h"
#include "events.h"
#include "unitmodes.h"
#include "items.h"
#include "uix.h"
#include "script.h"
#include "skills_shift.h"
#include "unit_priv.h"
#include "uix_components_complex.h"
#include "player.h"
#include "gameunits.h"
#include "states.h"
#include "excel_private.h"

static BOOL sgbKeyIsDown[ NUM_SKILL_ACTIVATOR_KEYS ] = { FALSE, FALSE };
static BOOL sgbKeyIsUsed[ NUM_SKILL_ACTIVATOR_KEYS ] = { FALSE, FALSE };

struct SKILL_ACTIVATOR_DATA
{
	GAME * pGame;
	UNIT * pUnit;
	const SKILL_DATA * pSkillData;
	int nSkill;
	SKILL_ACTIVATOR_KEY eKey;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sStopSkillHandler(
	GAME* game, 
	UNIT* unit, 
	const struct EVENT_DATA& event_data)
{
	SkillStopRequest(unit, (int)event_data.m_Data1, TRUE, TRUE );
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sActivatorMode( 
	const SKILL_ACTIVATOR_DATA & tData )
{
	return UnitTestMode( tData.pUnit, (UNITMODE) tData.pSkillData->nActivateMode );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCheckTarget( 
	const SKILL_ACTIVATOR_DATA & tData,
	UNIT * pTarget )
{
	if ( ! pTarget )
		return FALSE;

	BOOL bIsInRange = FALSE;
	if ( ! SkillIsValidTarget( tData.pGame, tData.pUnit, pTarget, NULL, tData.nSkill,  TRUE, &bIsInRange ) ||
		! bIsInRange )
		return FALSE;

	int code_len = 0;
	BYTE * code_ptr = ExcelGetScriptCode(tData.pGame, DATATABLE_SKILLS, tData.pSkillData->codeActivatorConditionOnTargets, &code_len);
	if ( code_ptr && ! VMExecI(tData.pGame, pTarget, NULL, code_ptr, code_len) )
		return FALSE;
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sActivatorTargetEx( 
	const SKILL_ACTIVATOR_DATA & tData,
	float fZOffset )
{
	UNIT * pTarget = UIGetTargetUnit();

	if ( ! sCheckTarget( tData, pTarget ) )
		pTarget = NULL;

	if ( pTarget && SkillShouldStart( tData.pGame, tData.pUnit, tData.nSkill, pTarget, NULL ) )
	{
		SkillSetTarget( tData.pUnit, tData.nSkill, INVALID_ID, UnitGetId( pTarget ) );
		return TRUE;
	}
	
	UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT ];
	UnitGetWeapons( tData.pUnit, tData.nSkill, pWeapons, FALSE );

	for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
	{
		if ( ! pWeapons[ i ] && i > 0 )
			continue;

		float fRangeMax = SkillGetRange( tData.pUnit, tData.nSkill, pWeapons[ i ] );

		if ( fZOffset != 0.0f )
		{
			VECTOR vLocation = UnitGetPosition( tData.pUnit );
			vLocation.fZ += fZOffset;
			pTarget = SkillGetNearestTarget( tData.pGame, tData.pUnit, tData.pSkillData, fRangeMax, &vLocation );
		} else {
			pTarget = SkillGetNearestTarget( tData.pGame, tData.pUnit, tData.pSkillData, fRangeMax );
		}

		// ignore crates, barrels, etc...
		if ( pTarget && ! UnitIsA( pTarget, UNITTYPE_CREATURE ) )
			pTarget = NULL;

		if ( ! sCheckTarget( tData, pTarget ) )
			pTarget = NULL;

		if ( pTarget && SkillShouldStart( tData.pGame, tData.pUnit, tData.nSkill, pTarget, NULL ) )
		{
			SkillSetTarget( tData.pUnit, tData.nSkill, INVALID_ID, UnitGetId( pTarget ) );
			return TRUE;
		}
	}
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sActivatorTarget( 
	const SKILL_ACTIVATOR_DATA & tData )
{
	return sActivatorTargetEx( tData, 0.0f );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sActivatorTargetWithZOffset( 
	const SKILL_ACTIVATOR_DATA & tData )
{
	return sActivatorTargetEx( tData, 2.5f );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sActivatorTargetNoSkillsOn( 
	const SKILL_ACTIVATOR_DATA & tData )
{
	if ( ! sActivatorTargetEx( tData, 0.0f ) )
		return FALSE;

	return ! SkillIsOnWithFlag( tData.pUnit, SKILL_FLAG_SKILL_IS_ON );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSeveralTargetsCommon( 
	const SKILL_ACTIVATOR_DATA & tData,
	float fDirectionTolerance )
{
	float fRangeMax = SkillGetRange( tData.pUnit, tData.nSkill, NULL );

	enum {
		NUMBER_OF_TARGETS_TO_LOOK_FOR = 3
	};
	UNITID pnTargets[ NUMBER_OF_TARGETS_TO_LOOK_FOR ];

	SKILL_TARGETING_QUERY tTargetingQuery( tData.pSkillData );
	tTargetingQuery.pSeekerUnit = tData.pUnit;
	tTargetingQuery.pnUnitIds = pnTargets;
	tTargetingQuery.nUnitIdMax = NUMBER_OF_TARGETS_TO_LOOK_FOR;
	tTargetingQuery.fMaxRange = fRangeMax;
	tTargetingQuery.fDirectionTolerance = fDirectionTolerance;
	if ( fDirectionTolerance != 1.0f )
	{
		SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_CHECK_DIRECTION );
	}
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_FIRSTFOUND );
	tTargetingQuery.codeSkillFilter = tData.pSkillData->codeActivatorConditionOnTargets;

	if ( SkillDataTestFlag( tData.pSkillData, SKILL_FLAG_IS_MELEE ) )
	{
		SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_CHECK_CAN_MELEE );
	}
	else
		tTargetingQuery.fMaxRange += UnitGetCollisionRadius( tData.pUnit );

	SkillTargetQuery( tData.pGame, tTargetingQuery );

	if ( tTargetingQuery.nUnitIdCount < NUMBER_OF_TARGETS_TO_LOOK_FOR )
		return FALSE;

	UNIT * pTarget = UnitGetById( tData.pGame, pnTargets[ 0 ] );

	if ( ! SkillShouldStart( tData.pGame, tData.pUnit, tData.nSkill, pTarget, NULL ) )
		return FALSE;

	SkillSetTarget( tData.pUnit, tData.nSkill, INVALID_ID, UnitGetId( pTarget ) );
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sActivatorSeveralTargets( 
	const SKILL_ACTIVATOR_DATA & tData )
{
	return sSeveralTargetsCommon( tData, 1.0f );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sActivatorTargetsInCone( 
	const SKILL_ACTIVATOR_DATA & tData )
{
	return sSeveralTargetsCommon( tData, 0.85f );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sActivatorSkillLasers( 
	const SKILL_ACTIVATOR_DATA & tData )
{
	return SkillIsOnWithFlag( tData.pUnit, SKILL_FLAG_USES_LASERS );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sActivatorSkillCanMultiBullets( 
	const SKILL_ACTIVATOR_DATA & tData )
{
	return SkillIsOnWithFlag( tData.pUnit, SKILL_FLAG_CAN_MULTI_BULLETS );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sActivatorAlways( 
	const SKILL_ACTIVATOR_DATA & tData )
{
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sActivatorInputAndCollision( 
	const SKILL_ACTIVATOR_DATA & tData )
{
	ASSERT_RETFALSE( IS_CLIENT( tData.pUnit ) );
	ASSERT_RETFALSE( tData.pUnit == GameGetControlUnit( tData.pGame ) );

	if ( SkillDataTestFlag( tData.pSkillData, SKILL_FLAG_DISABLED ) )
		return FALSE;

	if ( UnitHasState( tData.pGame, tData.pUnit, STATE_REMOVE_ON_MOVE ) )
		return FALSE;

	int nMovementKeys = c_PlayerGetMovement( tData.pGame );
	if ( ( tData.pSkillData->nPlayerInputOverride & nMovementKeys ) != tData.pSkillData->nPlayerInputOverride )
		return FALSE;

	VECTOR vDirection = c_PlayerGetDirectionFromInput( tData.pSkillData->nPlayerInputOverride );
	VECTOR vNewPosition = UnitGetPosition(tData.pUnit) + vDirection;
	VECTOR vNewPositionDesired = vNewPosition;

	COLLIDE_RESULT tResult;
	DWORD dwMoveFlags = MOVEMASK_TEST_ONLY | MOVEMASK_SOLID_MONSTERS;
	RoomCollide(tData.pUnit, UnitGetPosition(tData.pUnit), vDirection, &vNewPosition, UnitGetCollisionRadius(tData.pUnit), UnitGetCollisionHeight(tData.pUnit), dwMoveFlags, &tResult);

	if(TESTBIT(&tResult.flags, MOVERESULT_COLLIDE) || tResult.unit != NULL)
		return FALSE;

	VECTOR vMovedDirection = vNewPosition - UnitGetPosition(tData.pUnit);
	float fDistanceMoved = VectorNormalize( vMovedDirection );
	float fAngle = VectorDot( vDirection, vMovedDirection );

	// did we go over halfway and within 6 degrees of where we wanted?
	if ( ( fDistanceMoved < 0.5f ) || ( fAngle < 0.9f ) )
		return FALSE;

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sActivatorShouldRunForward( 
	const SKILL_ACTIVATOR_DATA & tData )
{
	int nMovementKeys = c_PlayerGetMovement( tData.pGame );
	if ( ( nMovementKeys & PLAYER_MOVE_BACK ) != 0 )
		return FALSE;

	UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT ];
	UnitGetWeapons( tData.pUnit, tData.nSkill, pWeapons, FALSE );

	int nSkillToTest = ItemGetPrimarySkill( pWeapons[ 0 ] );
	if ( nSkillToTest == INVALID_ID )
		nSkillToTest = tData.nSkill;

	VECTOR vForward = UnitGetFaceDirection( tData.pUnit, FALSE);
	for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
	{
		if ( ! c_SkillUnitShouldRunForward( tData.pGame, tData.pUnit, nSkillToTest, pWeapons[ i ], vForward, FALSE ) )
			return FALSE;
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL sSkillShouldActivate(
	SKILL_ACTIVATOR_DATA & tData)
{
#if !ISVERSION(SERVER_VERSION)
	if ( tData.pSkillData->eActivatorKey != tData.eKey )
		return FALSE;

	if ( tData.eKey == SKILL_ACTIVATOR_KEY_SHIFT &&
		(!UnitGetStat( tData.pUnit, STATS_SKILL_SHIFT_ENABLED, tData.nSkill )) && 
		!SkillDataTestFlag( tData.pSkillData, SKILL_FLAG_SHIFT_SKILL_ALWAYS_ENABLED ) )
		return FALSE;
	else if(!SkillGetLevel(tData.pUnit, tData.nSkill, FALSE) && SkillDataTestFlag(SkillGetData(tData.pGame, tData.nSkill), SKILL_FLAG_LEARNABLE))
		return FALSE;

	BOOL bIsMoving		= UnitTestModeGroup( tData.pUnit, MODEGROUP_MOVE ) ? TRUE : FALSE;
	BOOL bIgnoreMoving	= SkillDataTestFlag( tData.pSkillData, SKILL_FLAG_ACTIVATE_IGNORE_MOVING );
	BOOL bWantMoving	= SkillDataTestFlag( tData.pSkillData, SKILL_FLAG_ACTIVATE_WHILE_MOVING ) ? TRUE : FALSE;
	if ( !bIgnoreMoving && bWantMoving != bIsMoving )
		return FALSE;

	if ( tData.pSkillData->pfnActivator && !tData.pSkillData->pfnActivator( tData ) )
		return FALSE;

	int code_len = 0;
	BYTE * code_ptr = ExcelGetScriptCode( tData.pGame, DATATABLE_SKILLS, tData.pSkillData->codeActivatorCondition, &code_len);
	if (code_ptr)
	{
		int nSkillLevel = SkillGetLevel( tData.pUnit, tData.nSkill );
		if ( VMExecI( tData.pGame, tData.pUnit, NULL, tData.nSkill, nSkillLevel, tData.nSkill, nSkillLevel, INVALID_ID, code_ptr, code_len) == 0 )
			return FALSE;
	}

	if ( ! SkillCanStart( tData.pGame, tData.pUnit, tData.nSkill, INVALID_ID, AUTO_VERSION(UIGetTargetUnit(),NULL), FALSE, FALSE, 0 ))
	{
		return FALSE;
	}

	return TRUE;
#else // SERVER_VERSION
    ASSERT(FALSE);
    return FALSE;
#endif //!SERVER_VERSION
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sFindActivatorSkillBySkillLevels(
	SKILL_ACTIVATOR_DATA & tData )
{
	int nSkillToStart = INVALID_ID;
	int nMaxPriority = 0;

	UNIT_ITERATE_STATS_RANGE( tData.pUnit, STATS_SKILL_LEVEL, pStatsEntry, jj, MAX_SKILLS_PER_UNIT )
	{
		tData.nSkill = StatGetParam( STATS_SKILL_LEVEL, pStatsEntry[ jj ].GetParam(), 0 );
		if (tData.nSkill == INVALID_ID)
		{
			continue;
		}

		tData.pSkillData = SkillGetData( tData.pGame, tData.nSkill );
		if ( ! tData.pSkillData )
			continue;

		if ( tData.pSkillData->nActivatePriority > nMaxPriority )
		{
			if(sSkillShouldActivate( tData) )
			{
				nSkillToStart = tData.nSkill;
				nMaxPriority  = tData.pSkillData->nActivatePriority;
			}
			//This logic is essentially SkillGetBonusLevels....but we need the skill that has a bonus, and that function gives us the level
			else
			{
				int nSkill = INVALID_ID;
				for(int ii = 0; ii < MAX_LINKED_SKILLS && nSkill == INVALID_ID; ii++)
				{
					nSkill =  tData.pSkillData->pnLinkedLevelSkill[ii] ;
					//..and here is a c/p of the stuff above, since we have to do it w/ any linked skills
					if (nSkill == INVALID_ID)
					{
						continue;
					}

					const SKILL_DATA *pSkillData;
					pSkillData = SkillGetData( tData.pGame, nSkill );
					if ( ! pSkillData )
						continue;

					if ( pSkillData->nActivatePriority > nMaxPriority )
					{
						SKILL_ACTIVATOR_DATA tLinkedData;
						tLinkedData.eKey = tData.eKey;
						tLinkedData.pGame = tData.pGame;
						tLinkedData.pUnit = tData.pUnit;
						tLinkedData.nSkill = nSkill;
						tLinkedData.pSkillData = pSkillData;
						if(sSkillShouldActivate( tLinkedData ))
						{
							nSkillToStart = nSkill;
							nMaxPriority  = pSkillData->nActivatePriority;
						}
					}
				}
			}
		}
	}
	UNIT_ITERATE_STATS_END;

	return nSkillToStart;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UNIT * sFindActivatorSkillByInventory(
	SKILL_ACTIVATOR_DATA & tData,
	int nLocation )
{
	UNIT * pItem = UnitInventoryLocationIterate( tData.pUnit, nLocation, NULL );
	UNIT * pItemToUse = NULL;
	int nMaxPriority = 0;

	while ( pItem )
	{
		const UNIT_DATA * pUnitData = UnitGetData( pItem );
		if ( pUnitData->nSkillWhenUsed != INVALID_ID )
		{
			tData.nSkill = pUnitData->nSkillWhenUsed;
			tData.pSkillData = SkillGetData( tData.pGame, tData.nSkill );
			if ( ! tData.pSkillData )
				continue;

			if ( tData.pSkillData->nActivatePriority > nMaxPriority && sSkillShouldActivate( tData) )
			{
				pItemToUse = pItem;
				nMaxPriority  = tData.pSkillData->nActivatePriority;
			}
		}
		pItem = UnitInventoryLocationIterate( tData.pUnit, nLocation, pItem );
	}
	return pItemToUse;
}


//----------------------------------------------------------------------------
// yeah, this is kinda slow.  It is currently client-only and infrequent.
// we can speed it up by making a list of all skills that use the melee activator
//----------------------------------------------------------------------------
static int sFindActivatorSkillByAllSkills(
	SKILL_ACTIVATOR_DATA & tData )
{
	unsigned int nSkillCount = ExcelGetCount(EXCEL_CONTEXT(tData.pGame), DATATABLE_SKILLS);
	int nMaxPriority = 0;
	int nSkillToStart = INVALID_ID;
	for (unsigned int ii = 0; ii < nSkillCount; ++ii)
	{
		const SKILL_DATA * skill_data = SkillGetData(tData.pGame, ii);
		if (!skill_data)
		{
			continue;
		}
		if (skill_data->eActivatorKey != tData.eKey)
		{
			continue; // most skills fail here, so even though it is tested in sSkillShouldActivate(), I added it here too
		}

		tData.pSkillData = skill_data;
		tData.nSkill = ii;
		if (tData.pSkillData->nActivatePriority > nMaxPriority && sSkillShouldActivate(tData))
		{
			nSkillToStart = tData.nSkill;
			nMaxPriority = tData.pSkillData->nActivatePriority;
		}
	}

	return nSkillToStart;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int c_SkillsFindActivatorSkill(
	SKILL_ACTIVATOR_KEY eKey )
{
	ASSERT_RETINVALID( eKey >= 0 );
	ASSERT_RETINVALID( eKey < NUM_SKILL_ACTIVATOR_KEYS );
	ASSERT_RETINVALID( eKey != SKILL_ACTIVATOR_KEY_POTION );

	SKILL_ACTIVATOR_DATA tActivatorData;
	//tActivatorData.nSkillRequested = INVALID_ID;
	tActivatorData.pGame = AppGetCltGame();
	if ( ! tActivatorData.pGame )
		return INVALID_ID;

	tActivatorData.pUnit = GameGetControlUnit( tActivatorData.pGame );
	if ( ! tActivatorData.pUnit )
		return INVALID_ID;

	tActivatorData.eKey = eKey;

	switch ( eKey )
	{
	case SKILL_ACTIVATOR_KEY_SHIFT:
	default:
		return sFindActivatorSkillBySkillLevels(tActivatorData);

	case SKILL_ACTIVATOR_KEY_MELEE:
		return sFindActivatorSkillByAllSkills(tActivatorData);
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT * c_SkillsFindActivatorItem(
	SKILL_ACTIVATOR_KEY eKey )
{
	ASSERT_RETNULL( eKey == SKILL_ACTIVATOR_KEY_POTION );

	SKILL_ACTIVATOR_DATA tActivatorData;
	//tActivatorData.nSkillRequested = INVALID_ID;
	tActivatorData.pGame = AppGetCltGame();
	if ( ! tActivatorData.pGame )
		return NULL;

	tActivatorData.pUnit = GameGetControlUnit( tActivatorData.pGame );
	if ( ! tActivatorData.pUnit )
		return NULL;

	tActivatorData.eKey = eKey;

	return sFindActivatorSkillByInventory(tActivatorData, UnitGetBigBagInvLoc());
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sActivateSkill( 
	int nSkillToStart, 
	SKILL_ACTIVATOR_DATA & tData )
{
	if ( nSkillToStart == INVALID_ID )
		return FALSE;

	if ( c_SkillControlUnitRequestStartSkill( tData.pGame, nSkillToStart ) )
	{
		const SKILL_DATA * pSkillData = SkillGetData( tData.pGame, nSkillToStart );
		ASSERT_RETFALSE( pSkillData );

		if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_REPEAT_ALL  ) || 
			SkillDataTestFlag( pSkillData, SKILL_FLAG_REPEAT_FIRE ) || 
			SkillDataTestFlag( pSkillData, SKILL_FLAG_TRACK_REQUEST ) )
		{
			int nHoldTime = pSkillData->nMinHoldTicks;
			if (nHoldTime <= 0)
			{
				nHoldTime = 1;
			}
			//if ( ! UnitHasTimedEvent( tData.pUnit, sStopSkillHandler ))
				UnitRegisterEventTimed( tData.pUnit, sStopSkillHandler, EVENT_DATA(nSkillToStart), nHoldTime);
		}
		sgbKeyIsUsed[ tData.eKey ] = TRUE;
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_SkillsActivatorKeyDown( 
	SKILL_ACTIVATOR_KEY eKey )
{
	ASSERT_RETURN( eKey >= 0 );
	ASSERT_RETURN( eKey < NUM_SKILL_ACTIVATOR_KEYS );

	BOOL bKeyWasDown = sgbKeyIsDown[ eKey ];
	sgbKeyIsDown[ eKey ] = TRUE;

	if ( bKeyWasDown || sgbKeyIsUsed[ eKey ] )
	{
		return;
	}

	SKILL_ACTIVATOR_DATA tActivatorData;
	//tActivatorData.nSkillRequested = INVALID_ID;
	tActivatorData.pGame = AppGetCltGame();
	if ( ! tActivatorData.pGame )
		return;

	tActivatorData.pUnit = GameGetControlUnit( tActivatorData.pGame );
	if ( ! tActivatorData.pUnit )
		return;

	tActivatorData.eKey = eKey;

	switch ( eKey )
	{
	case SKILL_ACTIVATOR_KEY_POTION:
		{
			UNIT * pItem = sFindActivatorSkillByInventory( tActivatorData, UnitGetBigBagInvLoc() );
			if ( pItem  )
			{
				c_ItemUse( pItem );
			}
		}
		break;
	case SKILL_ACTIVATOR_KEY_SHIFT:
	default:
		{
			int nSkillToStart = sFindActivatorSkillBySkillLevels( tActivatorData );
			if ( nSkillToStart != INVALID_ID )
				sActivateSkill( nSkillToStart, tActivatorData );
		}
		break;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void c_SkillsActivatorKeyUp(
	SKILL_ACTIVATOR_KEY eKey )
{
	ASSERT_RETURN( eKey >= 0 );
	ASSERT_RETURN( eKey < NUM_SKILL_ACTIVATOR_KEYS );
	sgbKeyIsDown[ eKey ] = FALSE;
	sgbKeyIsUsed[ eKey ] = FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct SKILL_ACTIVATOR_LOOKUP
{
	const char *pszName;
	PFN_SKILL_ACTIVATOR fpHandler;
};

//----------------------------------------------------------------------------
// keep at bottom so that it's easier to find, and we have all of the functions declared above
//----------------------------------------------------------------------------
static const SKILL_ACTIVATOR_LOOKUP sgtSkillActivatorLookupTable[] = 
{
	{ "Mode",					sActivatorMode					},
	{ "Target",					sActivatorTarget				},
	{ "TargetWithZOffset",		sActivatorTargetWithZOffset		},
	{ "TargetNoSkillsOn",		sActivatorTargetNoSkillsOn		},
	{ "SeveralTargets",			sActivatorSeveralTargets		},
	{ "TargetsInCone",			sActivatorTargetsInCone			},
	{ "SkillLasers",			sActivatorSkillLasers			},
	{ "SkillCanMultiBullets",	sActivatorSkillCanMultiBullets	},
	{ "Always",					sActivatorAlways				},
	{ "InputAndCollision",		sActivatorInputAndCollision		},
	{ "ShouldRunForward",		sActivatorShouldRunForward		},
	{ NULL,						NULL							}, // keep this last
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void SkillsActivatorsInitExcelPostProcess(
	EXCEL_TABLE * table)
{
	ASSERT_RETURN(table->m_Index == DATATABLE_SKILLS);

	for (unsigned int ii = 0; ii < ExcelGetCountPrivate(table); ++ii)
	{
		SKILL_DATA * skill_data = (SKILL_DATA *)ExcelGetDataPrivate(table, ii);
		if (! skill_data )
			continue;

		skill_data->pfnActivator = NULL;

		const char * pszFunctionName = skill_data->szActivator;
		if (!pszFunctionName)
		{
			continue;
		}
		int nFunctionNameLength = PStrLen(pszFunctionName);
		if (nFunctionNameLength <= 0)
		{
			continue;
		}

		for (unsigned int jj = 0; sgtSkillActivatorLookupTable[jj].pszName != NULL; ++jj)
		{
			const SKILL_ACTIVATOR_LOOKUP * lookup = &sgtSkillActivatorLookupTable[jj];
			if (PStrICmp(lookup->pszName, pszFunctionName) == 0)
			{
				skill_data->pfnActivator = lookup->fpHandler;
				break;
			}
		}

		if (skill_data->pfnActivator == NULL)
		{
			ASSERTV_CONTINUE(0, "Unable to find skill shift activator '%s' for '%s' on row '%d'", skill_data->szActivator, skill_data->szName, ii);
		}
	}
}	


#endif// !ISVERSION(SERVER_VERSION)
