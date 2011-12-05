//----------------------------------------------------------------------------
// ai.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "prime.h"
#include "game.h"
#include "ai.h"
#include "ai_priv.h"
#include "skills.h"
#include "console.h"
#include "gameunits.h"
#include "picker.h"
#include "states.h"
#include "unit_priv.h"
#include "performance.h"
#include "npc.h"
#include "pets.h"
#include "teams.h"
#include "excel_private.h"
#include "c_sound_util.h"
#include "c_sound.h"
#if ISVERSION(SERVER_VERSION)
#include "ai.cpp.tmh"
#endif

static BOOL sgbAIOutputToConsole = FALSE;
static const int RETURN_TO_PREFERRED_DIRECTION_TIME = GAME_TICKS_PER_SECOND * 5;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
static void c_sAIBehaviorTableLoad(
	GAME * pGame,
	const AI_BEHAVIOR_DEFINITION_TABLE * pAiTable)
{
	ASSERT_RETURN(pAiTable);
	for(int i=0; i<pAiTable->nBehaviorCount; i++)
	{
		const AI_BEHAVIOR_DEFINITION * pBehavior = &pAiTable->pBehaviors[i];
		ASSERT_CONTINUE(pBehavior);

		if(pBehavior->nMonsterId >= 0)
		{
			UnitDataLoad(pGame, DATATABLE_MONSTERS, pBehavior->nMonsterId);
		}
		if(pBehavior->nSkillId >= 0)
		{
			c_SkillLoad(pGame, pBehavior->nSkillId);
		}
		if(pBehavior->nSkillId2 >= 0)
		{
			c_SkillLoad(pGame, pBehavior->nSkillId2);
		}
		if(pBehavior->nSoundId >= 0)
		{
			c_SoundLoad(pBehavior->nSoundId);
		}
		if(pBehavior->nStateId >= 0)
		{
			c_StateLoad(pGame, pBehavior->nStateId);
		}
		c_sAIBehaviorTableLoad(pGame, &pBehavior->tTable);
	}
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_AIDataLoad(
	GAME * pGame,
	int nAI)
{
#if !ISVERSION(SERVER_VERSION)
	if(nAI < 0)
		return;

	AI_INIT * pAiData = (AI_INIT *) ExcelGetData( pGame, DATATABLE_AI_INIT, nAI );
	if(!pAiData)
		return;

	AI_DEFINITION * pAiDef = AIGetDefinition(pAiData);
	if(!pAiDef)
		return;

	c_sAIBehaviorTableLoad(pGame, &pAiDef->tTable);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
static void c_sAIBehaviorTableFlagSoundsForLoad(
	GAME * pGame,
	const AI_BEHAVIOR_DEFINITION_TABLE * pAiTable,
	BOOL bLoadNow)
{
	ASSERT_RETURN(pAiTable);
	for(int i=0; i<pAiTable->nBehaviorCount; i++)
	{
		const AI_BEHAVIOR_DEFINITION * pBehavior = &pAiTable->pBehaviors[i];
		ASSERT_CONTINUE(pBehavior);

		if(pBehavior->nMonsterId >= 0)
		{
			c_UnitDataFlagSoundsForLoad(pGame, DATATABLE_MONSTERS, pBehavior->nMonsterId, bLoadNow);
		}
		if(pBehavior->nSkillId >= 0)
		{
			c_SkillFlagSoundsForLoad(pGame, pBehavior->nSkillId, bLoadNow);
		}
		if(pBehavior->nSkillId2 >= 0)
		{
			c_SkillFlagSoundsForLoad(pGame, pBehavior->nSkillId2, bLoadNow);
		}
		if(pBehavior->nSoundId >= 0)
		{
			c_SoundFlagForLoad(pBehavior->nSoundId, bLoadNow);
		}
		if(pBehavior->nStateId >= 0)
		{
			c_StateFlagSoundsForLoad(pGame, pBehavior->nStateId, bLoadNow);
		}
		c_sAIBehaviorTableFlagSoundsForLoad(pGame, &pBehavior->tTable, bLoadNow);
	}
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_AIFlagSoundsForLoad(
	GAME * pGame,
	int nAI,
	BOOL bLoadNow)
{
#if !ISVERSION(SERVER_VERSION)
	if(nAI < 0)
		return;

	AI_INIT * pAiData = (AI_INIT *) ExcelGetData( pGame, DATATABLE_AI_INIT, nAI );
	if(!pAiData)
		return;

	AI_DEFINITION * pAiDef = AIGetDefinition(pAiData);
	if(!pAiDef)
		return;

	c_sAIBehaviorTableFlagSoundsForLoad(pGame, &pAiDef->tTable, bLoadNow);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNITID UnitGetAITargetId(
	UNIT * pUnit,
	DWORD dwFlags)
{
	ASSERT_RETINVALID(pUnit);
	UNITID idTarget = INVALID_ID;
	WORD wStat = STATS_AI_TARGET_ID;
	if(IS_CLIENT(pUnit) || TESTBIT(dwFlags, GET_AI_TARGET_FORCE_CLIENT_BIT))
	{
		wStat = STATS_AI_TARGET_ID_CLIENT;
	}
	if(!TESTBIT(dwFlags, GET_AI_TARGET_IGNORE_OVERRIDE_BIT))
	{
		wStat = STATS_AI_TARGET_OVERRIDE_ID;
	}
	
	idTarget = UnitGetStat(pUnit, wStat);
	if(idTarget == INVALID_ID && !TESTBIT(dwFlags, GET_AI_TARGET_IGNORE_OVERRIDE_BIT) && !TESTBIT(dwFlags, GET_AI_TARGET_ONLY_OVERRIDE_BIT))
	{
		SETBIT(dwFlags, GET_AI_TARGET_IGNORE_OVERRIDE_BIT);
		return UnitGetAITargetId(pUnit, dwFlags);
	}
	return idTarget;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT * UnitGetAITarget(
	UNIT * pUnit,
	DWORD dwFlags)
{
	ASSERT_RETNULL(pUnit);
	UNITID idTarget = UnitGetAITargetId(pUnit, dwFlags);
	UNIT * pTarget = UnitGetById(UnitGetGame(pUnit), idTarget);
	if ( pTarget && UnitHasState( UnitGetGame( pUnit ), pTarget, STATE_AI_IGNORE ) )
		pTarget = NULL;
	return pTarget;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitSetAITarget(
	UNIT * pUnit,
	UNITID idTarget,
	BOOL bOverrideTarget,
	BOOL bSendToClient)
{
	ASSERT_RETURN(pUnit);
	if(IS_SERVER(pUnit))
	{
		WORD wStat = STATS_AI_TARGET_ID;
		if(bOverrideTarget)
		{
			wStat = STATS_AI_TARGET_OVERRIDE_ID;
		}
		UnitSetStat(pUnit, wStat, idTarget);

		if(bSendToClient && !bOverrideTarget)
		{
			UnitSetStat(pUnit, STATS_AI_TARGET_ID_CLIENT, idTarget);
		}
	}
	else
	{
		UnitSetStat(pUnit, STATS_AI_TARGET_ID_CLIENT, idTarget);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitSetAITarget(
	UNIT * pUnit,
	UNIT * pTarget,
	BOOL bOverrideTarget,
	BOOL bSendToClient)
{
	ASSERT_RETURN(pUnit);
	UNITID idTarget = UnitGetId(pTarget);
	UnitSetAITarget(pUnit, idTarget, bOverrideTarget, bSendToClient);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAISendTargetToClient(
	UNIT * pUnit)
{
	if(!IS_SERVER(pUnit))
	{
		return;
	}

	UNITID idTargetServer = UnitGetAITargetId(pUnit, 0);
	DWORD dwFlags = 0;
	SETBIT(dwFlags, GET_AI_TARGET_FORCE_CLIENT_BIT);
	UNITID idTargetClient = UnitGetAITargetId(pUnit, dwFlags);
	if(idTargetServer != idTargetClient)
	{
		UnitSetAITarget(pUnit, idTargetServer, FALSE, TRUE);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sAITargetIsIgnored(
	GAME * pGame,
	UNIT * pTarget)
{
	if ( AppIsHellgate() && pTarget )
	{
		if ( UnitHasState( pGame, pTarget, STATE_STEALTH) )
			return TRUE;
	}

	return UnitHasState( pGame, pTarget, STATE_AI_IGNORE_ON_SEARCH );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sAIIsNotValidTarget(
	GAME * pGame,
	UNIT * pUnit,
	UNIT * pTarget,
	UNIT * pWeapon,
	int nSkill,
	BOOL bValidateRange)
{
	const float fGlobalMaxSightRange = AppIsHellgate() ? 60.0f : 10.0f;

	return ! SkillIsValidTarget( pGame, pUnit, pTarget, pWeapon, nSkill, TRUE ) ||
		( pTarget && ( ( bValidateRange && ! UnitsAreInRange( pUnit, pTarget, 0.0f, fGlobalMaxSightRange, NULL ) ) || sAITargetIsIgnored( pGame, pTarget ) ) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sHandleStealth(
	UNIT * pUnitSearching,
	UNIT * pTarget,
	BOOL bLastAttacker )
{
	GAME * pGame = UnitGetGame( pUnitSearching );
	if ( IS_CLIENT( pGame ) )
		return FALSE;

	if ( TestFriendly( pGame, pUnitSearching, pTarget ) )
		return FALSE;

	if ( UnitHasState( pGame, pUnitSearching, STATE_STEALTH_REVEAL_3 ) ||
		 (UnitHasState( pGame, pUnitSearching, STATE_STEALTH_REVEAL_2 ) && (UnitGetStat( pUnitSearching, STATS_HP_CUR ) < UnitGetStat( pUnitSearching, STATS_HP_MAX ))))
		return FALSE; // if the unit is already close to revealing, then they can attack you

	int nStealthBalance;
	{
		int nStealthAttackPct = 100 + UnitGetStat( pTarget, STATS_STEALTH_ATTACK_PCT );
		int nStealthAttack = PCT( UnitGetStat( pTarget, STATS_STEALTH_ATTACK ), nStealthAttackPct );

		int nStealthDefensePct = 100 + UnitGetStat( pUnitSearching, STATS_STEALTH_DEFENSE_PCT );
		int nStealthDefense = PCT( UnitGetStat( pUnitSearching, STATS_STEALTH_DEFENSE ), nStealthDefensePct );

		nStealthBalance = 100;
		if (nStealthAttack + nStealthDefense)
			nStealthBalance += (100 * (nStealthAttack - nStealthDefense)) / (nStealthAttack + nStealthDefense);
	}

	float fStealthRevealRadius = 0.0f;
	if ( ! bLastAttacker )
	{
		int nRadiusPct = UnitGetStat( pUnitSearching, STATS_STEALTH_REVEAL_RADIUS_PCT );
		nRadiusPct -= UnitGetStat( pTarget, STATS_STEALTH_REVEAL_RADIUS_DECREASE_PCT );
		nRadiusPct -= nStealthBalance;
		nRadiusPct = MAX( nRadiusPct, -50 );

		int nStealthRevealRadius = UnitGetStat( pUnitSearching, STATS_STEALTH_REVEAL_RADIUS );
		nStealthRevealRadius += PCT( nStealthRevealRadius, nRadiusPct );

		fStealthRevealRadius = (float)(nStealthRevealRadius * 0.1f);
	}

	if ( ( bLastAttacker || UnitsAreInRange( pUnitSearching, pTarget, 0.0f, fStealthRevealRadius, NULL ) ) &&
		! UnitHasState( pGame, pUnitSearching, STATE_STEALTH_REVEAL ) )
	{
		if ( IS_SERVER( pGame ) )
		{
			int nStealthRevealDuration;
			{
				int nDurationPct = UnitGetStat( pUnitSearching, STATS_STEALTH_REVEAL_DURATION_PCT );
				nDurationPct += UnitGetStat( pTarget, STATS_STEALTH_REVEAL_DURATION_INCREASE_PCT );
				nDurationPct += nStealthBalance / 2;
				nDurationPct = MIN( nDurationPct, 160 );

				nStealthRevealDuration = UnitGetStat( pUnitSearching, STATS_STEALTH_REVEAL_DURATION );
				nStealthRevealDuration += PCT( nStealthRevealDuration, nDurationPct );
				nStealthRevealDuration = MAX( nStealthRevealDuration, 1 );
			}
			nStealthRevealDuration /= 3; // we've broken this up into three stages with equal duration
			s_StateSet( pUnitSearching, pTarget, STATE_STEALTH_REVEAL_1, nStealthRevealDuration );
		}
	}

	return TRUE;
}

//****************************************************************************
// AI helper functions
//****************************************************************************
UNIT* AI_GetTarget(
	GAME * pGame,
	UNIT * pUnit,
	int nSkill,
	DWORD * pdwSearchFlags,
	DWORD dwAIGetTargetFlags,
	float fDirectionTolerance,
	const VECTOR * pvLocation,
	UNIT * pSeekerUnit)
{
	if (GameGetDebugFlag(pGame, DEBUGFLAG_AI_NOTARGET))
	{
		return NULL;
	}

	if (nSkill != INVALID_LINK)
	{
		const SKILL_DATA *pSkillData = SkillGetData( pGame, nSkill );
		if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_IS_AGGRESSIVE ) )
		{
			if (UnitGetStat( pUnit, STATS_DONT_ATTACK ) > 0)
			{
				return NULL;
			}
		}
	}

	BOOL bValidateRange = !TESTBIT(dwAIGetTargetFlags, AIGTF_DONT_VALIDATE_RANGE_BIT);

	DWORD dwAITargetFlags = 0;
	SETBIT(dwAITargetFlags, GET_AI_TARGET_ONLY_OVERRIDE_BIT);
	UNIT * pTarget = UnitGetAITarget(pUnit, dwAITargetFlags);
	if (pTarget)
	{
		if ( sAIIsNotValidTarget( pGame, pUnit, pTarget, NULL, nSkill, bValidateRange ) )
			pTarget = NULL;
		else
			return pTarget;
	}

	BOOL bUseSkillRange = TESTBIT(dwAIGetTargetFlags, AIGTF_USE_SKILL_RANGE_BIT);
	BOOL bDoTargetQuery = !TESTBIT(dwAIGetTargetFlags, AIGTF_DONT_DO_TARGET_QUERY_BIT);

	if(bDoTargetQuery)
	{
		const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkill );
		ASSERT_RETNULL( pSkillData );

		float fAiAwakeRange;
		if(bUseSkillRange)
		{
			fAiAwakeRange = (float)SkillGetRange(pUnit, nSkill);
		}
		else
		{
			fAiAwakeRange = (float)UnitGetStat(pUnit, STATS_AI_SIGHT_RANGE);
		}

		SKILL_TARGETING_QUERY tTargetingQuery( pSkillData );
		if ( pdwSearchFlags )
		{
			for ( int i = 0; i < NUM_TARGET_QUERY_FILTER_FLAG_DWORDS; i++ )
				tTargetingQuery.tFilter.pdwFlags[ i ] |= pdwSearchFlags[ i ];
		}

		int nAI = UnitGetStat(pUnit, STATS_AI);
		BOOL bFilterUnsearchables = TRUE;
		if ( nAI != INVALID_ID )
		{
			AI_INIT * pAiData = (AI_INIT *) ExcelGetData( pGame, DATATABLE_AI_INIT, nAI ); 
			if(pAiData)
			{
				if ( pAiData->bTargetClosest )
				{
					SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_CLOSEST );
				}
				if ( pAiData->bTargetRandomize )
				{
					SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_RANDOM );
				}
				if ( pAiData->bNoDestructables )
				{
					SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_NO_DESTRUCTABLES );
				}
				if ( pAiData->bCanSeeUnsearchables )
					bFilterUnsearchables = FALSE;
			}
		}

		if ( TESTBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_CHECK_CAN_MELEE ) )
		{
			SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_NO_FLYERS );
		}
		SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_NO_MERCHANTS );
		if ( fDirectionTolerance != 0.0f )
		{
			SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_CHECK_DIRECTION );
		}
		if(bFilterUnsearchables)
		{
			tTargetingQuery.tFilter.pnIgnoreUnitsWithState[ 1 ] = STATE_AI_IGNORE_ON_SEARCH;
		}
		tTargetingQuery.pvLocation = pvLocation;
		if ( pvLocation )
		{
			SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_USE_LOCATION );
		}
		CLEARBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_CHECK_CAN_MELEE ); // we are just looking for targets - not checking if we can attack right now

#define MAX_UNITS_IN_TARGET_SEARCH 10
		UNITID pnNearest[ MAX_UNITS_IN_TARGET_SEARCH ];

		tTargetingQuery.pSeekerUnit = pSeekerUnit ? pSeekerUnit : pUnit;
		tTargetingQuery.pnUnitIds = pnNearest;
		tTargetingQuery.nUnitIdMax = MAX_UNITS_IN_TARGET_SEARCH;
		tTargetingQuery.fMaxRange = fAiAwakeRange;
		tTargetingQuery.fDirectionTolerance = fDirectionTolerance;
		tTargetingQuery.nRangeModifierStat_Percent	= STATS_AI_SIGHT_RANGE_MODIFIER_PERCENT;
		tTargetingQuery.nRangeModifierStat_Min		= STATS_AI_SIGHT_RANGE_MODIFIER_MIN;

		SkillTargetQuery( pGame, tTargetingQuery );
		
		pTarget = NULL;

		if ( AppIsHellgate() )
		{
			BOOL bSelectNewTarget = TESTBIT(dwAIGetTargetFlags, AIGTF_PICK_NEW_TARGET_BIT);
			UNITID idLastAttacker = UnitGetStat( pUnit, STATS_AI_LAST_ATTACKER_ID );
			BOOL bImmuneToStealth = UnitHasState( pGame, pUnit, STATE_STEALTH_IMMUNE );
			if(bSelectNewTarget)
			{
				UNIT * pOldTarget = UnitGetAITarget(pUnit);
				UNITID idOldTarget = UnitGetId(pOldTarget);
				UNITID idNextHighest = INVALID_ID;
				UNITID idLowest = UnitGetId(pOldTarget);
				for ( int i = 0; i < tTargetingQuery.nUnitIdCount; i++ )
				{// go in reverse order so that we look at the closest last
					UNIT * pUnitCurr = UnitGetById( pGame, tTargetingQuery.pnUnitIds[ i ] );
					if ( ! pUnitCurr )
						continue;

					if ( bImmuneToStealth || !UnitHasState( pGame, pUnitCurr, STATE_STEALTH) ||
						!sHandleStealth( pUnit, pUnitCurr, UnitGetId( pUnitCurr ) == idLastAttacker ) )
					{
						UNITID idUnitCurr = UnitGetId(pUnitCurr);
						if(idUnitCurr > idOldTarget && (idUnitCurr < idNextHighest || idNextHighest == INVALID_ID))
						{
							idNextHighest = idUnitCurr;
						}
						if(idUnitCurr < idOldTarget && idUnitCurr < idLowest)
						{
							idLowest = idUnitCurr;
						}
					}
				}

				if(idNextHighest != INVALID_ID && idNextHighest > idOldTarget)
				{
					pTarget = UnitGetById( pGame, idNextHighest );
				}
				else if(idLowest < idOldTarget)
				{
					pTarget = UnitGetById( pGame, idLowest );
				}
			}
			else
			{
				//UNIT * pOldTarget = UnitGetAITarget(pUnit);
				for ( int i = tTargetingQuery.nUnitIdCount - 1; i >= 0; i-- )
				{// go in reverse order so that we look at the closest last
					UNIT * pUnitCurr = UnitGetById( pGame, tTargetingQuery.pnUnitIds[ i ] );
					if ( ! pUnitCurr )
						continue;

					if ( bImmuneToStealth || !UnitHasState( pGame, pUnitCurr, STATE_STEALTH) ||
						!sHandleStealth( pUnit, pUnitCurr, UnitGetId( pUnitCurr ) == idLastAttacker ) )
					{
						// KCK: Hacky way to avoid targetting walls. If EVERYONE_CAN_TARGET flag is set, then
						// any other target takes preference, regardless of range - unless our path is blocked.
						BOOL bLowPriority = UnitDataTestFlag(UnitGetData(pUnitCurr), UNIT_DATA_FLAG_EVERYONE_CAN_TARGET);
						BOOL bPathBlocked = UnitHasState(pGame, pUnit, STATE_PATH_BLOCKED);
						if ( !bLowPriority || bPathBlocked || (bLowPriority && pTarget && UnitDataTestFlag(UnitGetData(pTarget), UNIT_DATA_FLAG_EVERYONE_CAN_TARGET)) )
						{
							pTarget = pUnitCurr;
						}
					}
				}
			}
		} 
		else
		{
			//I changed this even after reading the comments in the next section down below...seems to work great now
			UNITID idLastAttacker = UnitGetStat( pUnit, STATS_AI_LAST_ATTACKER_ID );
			UNIT* pAttacker = UnitGetById( pGame, idLastAttacker );
			BOOL bInRange = TRUE;
			// if they're dead? They're not our last attacker anymore.
			
			if( !pAttacker || IsUnitDeadOrDying( pAttacker ) ||
				!TestHostile( pGame, pUnit, pAttacker ) ||
				!SkillIsValidTarget( pGame,
				pUnit,
				pAttacker,
				NULL,
				nSkill,
				FALSE,
				&bInRange,
				TRUE ) )
			{
				idLastAttacker = INVALID_ID;
				UnitSetStat( pUnit, STATS_AI_LAST_ATTACKER_ID, INVALID_ID );
			}
			if( idLastAttacker != INVALID_ID )
			{
				pTarget = UnitGetById( pGame, idLastAttacker );
			}
			if ( !pTarget && tTargetingQuery.nUnitIdCount > 0 ) 
			{
				pTarget = UnitGetById( pGame, tTargetingQuery.pnUnitIds[ 0 ] );
			}
		}

		if ( pTarget )
		{
			UnitSetAITarget(pUnit, pTarget);
		}
	}

	if ( ! pTarget && ! bUseSkillRange )
	{
		UNIT * pWeapons[ MAX_WEAPON_LOCATIONS_PER_SKILL ];
		UnitGetWeapons( pUnit, nSkill, pWeapons, FALSE );

		dwAITargetFlags = 0;
		SETBIT(dwAITargetFlags, GET_AI_TARGET_IGNORE_OVERRIDE_BIT);
		pTarget = UnitGetAITarget(pUnit, dwAITargetFlags);
		if ( sAIIsNotValidTarget( pGame, pUnit, pTarget, pWeapons[0], nSkill, bValidateRange ) )
		{
			// If the current target is not a valid target, then forget it
			UNITID idTarget = (UNITID) UnitGetStat( pUnit, STATS_AI_LAST_ATTACKER_ID );
			pTarget = UnitGetById( pGame, idTarget );
			if ( sAIIsNotValidTarget( pGame, pUnit, pTarget, pWeapons[0], nSkill, bValidateRange ) )
			{
				// What if another skill can target this unit?  Do we really want to lose it? - Tyler
				// TRAVIS: Tugboat does - otherwise they just keep following you forever, from what I can tell 
				// GS 12-14-2006
				// In Hellgate we still want to clear this if the target has a state that says that we should forget about it
				if( AppIsTugboat() || (pTarget && sAITargetIsIgnored(pGame, pTarget)) )
				{
					UnitSetStat(pUnit, STATS_AI_LAST_ATTACKER_ID, INVALID_ID);
				}
				pTarget = NULL;
				UnitSetAITarget(pUnit, INVALID_ID);
				if( AppIsTugboat() )
				{
					UnitSetMoveTarget( pUnit, INVALID_ID );
				}
			}
			else
			{
				UnitSetAITarget(pUnit, pTarget);
			}
		}
	}
	return pTarget;
}

//****************************************************************************
// AI helper functions
//****************************************************************************
UNIT* AI_GetNearest(
	GAME * pGame,
	UNIT * pUnit,
	DWORD * pdwSearchFlags )
{
	if (GameGetDebugFlag(pGame, DEBUGFLAG_AI_NOTARGET))
	{
		return NULL;
	}

	DWORD dwAITargetFlags = 0;
	SETBIT(dwAITargetFlags, GET_AI_TARGET_ONLY_OVERRIDE_BIT);
	UNIT * pTarget = UnitGetAITarget(pUnit, dwAITargetFlags);
	if(pTarget)
	{
		return pTarget;
	}


	UNITID pnNearest[ 1 ];

	SKILL_TARGETING_QUERY tTargetingQuery;//( pSkillData );

	if ( pdwSearchFlags )
	{
		for ( int i = 0; i < NUM_TARGET_QUERY_FILTER_FLAG_DWORDS; i++ )
			tTargetingQuery.tFilter.pdwFlags[ i ] |= pdwSearchFlags[ i ];
	}
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_FRIEND );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_ENEMY );
	tTargetingQuery.pSeekerUnit = pUnit;
	tTargetingQuery.pnUnitIds = pnNearest;
	tTargetingQuery.nUnitIdMax = 1;
	tTargetingQuery.fMaxRange = UnitGetCollisionRadius( pUnit ); //UnitGetRawCollisionRadius( pUnit ) + .5f; // Tyler - why use the raw radius?
	tTargetingQuery.fDirectionTolerance = 0.0f;

	SkillTargetQuery( pGame, tTargetingQuery );

	if ( tTargetingQuery.nUnitIdCount > 0 )
	{
		pTarget = UnitGetById( pGame, tTargetingQuery.pnUnitIds[ 0 ] );
	}
	return pTarget;
}


//****************************************************************************
// AI helper functions
//****************************************************************************
UNIT* AI_ShouldBeAwake(
	GAME * pGame,
	UNIT * pUnit,
	DWORD * pdwSearchFlags )
{
	if (GameGetDebugFlag(pGame, DEBUGFLAG_AI_NOTARGET))
	{
		return NULL;
	}

	float fAiAwakeRange = (float)UnitGetStat(pUnit, STATS_AI_AWAKE_RANGE);

	UNITID pnNearest[ 1 ];

	SKILL_TARGETING_QUERY tTargetingQuery;//( pSkillData );
	if ( pdwSearchFlags )
	{
		for ( int i = 0; i < NUM_TARGET_QUERY_FILTER_FLAG_DWORDS; i++ )
			tTargetingQuery.tFilter.pdwFlags[ i ] |= pdwSearchFlags[ i ];
	}
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_ENEMY );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_USE_LOCATION );
	tTargetingQuery.pSeekerUnit = pUnit;
	tTargetingQuery.pnUnitIds = pnNearest;
	tTargetingQuery.nUnitIdMax = 1;
	tTargetingQuery.fMaxRange = fAiAwakeRange;// * 1.25f;
	tTargetingQuery.fDirectionTolerance = 0.0f;

	SkillTargetQuery( pGame, tTargetingQuery );

	UNIT * pTarget = NULL;
	if ( tTargetingQuery.nUnitIdCount > 0 )
	{
		pTarget = UnitGetById( pGame, tTargetingQuery.pnUnitIds[ 0 ] );
	}
	return pTarget;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sAIGetFirstSkill(
	UNIT * pUnit)
{
	int nSkill = UnitGetStat( pUnit, STATS_AI_SKILL_PRIMARY );
	if ( nSkill != INVALID_ID )
		return nSkill;

	STATS_ENTRY pSkillStats[ 1 ];
	int nValues = UnitGetStatsRange(pUnit, STATS_SKILL_LEVEL, 0, pSkillStats, 1);
	if ( nValues > 0 )
		return pSkillStats[0].GetParam();
	return INVALID_ID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AIIsBusy(
	GAME * pGame,
	UNIT * pUnit )
{
	if ( UnitTestModeGroup( pUnit, MODEGROUP_ATTACK ) || UnitTestModeGroup( pUnit, MODEGROUP_GETHIT ) )
		 return TRUE;

	// see if they are currently doing a skill
	if ( SkillGetNumOn(pUnit, TRUE ) != 0 )
		return TRUE;

	return FALSE;
}

//****************************************************************************
// AI controling functions
//****************************************************************************

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAI_RavagerInit( GAME* pGame, UNIT* pUnit, int nAiIndex )
{
	UnitSetStat( pUnit, STATS_SKILL_EVENT_ID, INVALID_ID );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AI_Unregister(
	UNIT* pUnit )
{
	UnitUnregisterTimedEvent(pUnit, AI_Update);
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AI_Register(
	UNIT * pUnit,
	BOOL bRandomPeriod,
	int nForcePeriod )
{
	ASSERT_RETFALSE(pUnit);

	if (UnitTestFlag(pUnit, UNITFLAG_DEACTIVATED))
	{
		return FALSE;
	}
	ROOM * room = UnitGetRoom(pUnit);
	if (!room)
	{
		return FALSE;
	}
	if (RoomIsActive(room) == FALSE)
	{
		if (UnitEmergencyDeactivate(pUnit, "Attempt to register AI in Inactive Room"))
		{
			return FALSE;
		}
	}
	//trace("[%d] AI_Register(%s[%d], %s, %d)\n", GameGetTick(UnitGetGame(pUnit)), UnitGetDevName(pUnit), UnitGetId(pUnit), bRandomPeriod ? "TRUE" : "FALSE", nForcePeriod);
	int nAI = UnitGetStat(pUnit, STATS_AI);

	AI_INIT * pAiData = (AI_INIT *) ExcelGetData( NULL, DATATABLE_AI_INIT, nAI ); 
	if ( pAiData && pAiData->bNeverRegisterAIEvent )
		return FALSE;

	if ((IS_CLIENT(pUnit) && pAiData->bServerOnly) || 
		(IS_SERVER(pUnit) && pAiData->bClientOnly))
	{
		return FALSE;
	}

	int aiperiod = nForcePeriod != INVALID_ID ? nForcePeriod : UnitGetStat(pUnit, STATS_AI_PERIOD);
	ASSERT_RETFALSE(aiperiod > 0);
	if (bRandomPeriod)
	{
		aiperiod = RandGetNum( UnitGetGame( pUnit ), 1, aiperiod);
	}
	UnitUnregisterTimedEvent(pUnit, AI_Update);
	UnitRegisterEventTimed(pUnit, AI_Update, NULL, aiperiod);
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL ExcelAIPostProcess( 
	EXCEL_TABLE * table)
{
	ASSERT_RETFALSE(table);

	{ 
		for (unsigned int ii = 0; ii < ExcelGetCountPrivate(table); ii++)
		{
			AI_INIT * ai_def = (AI_INIT *)ExcelGetDataPrivate(table, ii);
			if ( ! ai_def )
				continue;

			if (ai_def->pszTableName[0] != 0)
			{
				ai_def->nTable = DefinitionGetIdByName(DEFINITION_GROUP_AI, ai_def->pszTableName);
			}
			else
			{
				ai_def->nTable = INVALID_ID;
			}
		}
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
AI_DEFINITION * AIGetDefinition(
	AI_INIT * pAiData)
{
	if (AppGetType() != APP_TYPE_CLOSED_SERVER)
	{
		ASSERT_RETNULL( pAiData->pszTableName[ 0 ] != 0 );
		if ( pAiData->nTable == INVALID_ID )
		{
			pAiData->nTable = DefinitionGetIdByName( DEFINITION_GROUP_AI, pAiData->pszTableName );
			ASSERT_RETNULL(pAiData->nTable != INVALID_ID);
		}
	}
	return (AI_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_AI, pAiData->nTable );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static AI_START_FUNC * sgpfnStartFunctions[] =
{
	AI_AssassinBugHiveInit,
	sAI_RavagerInit,
	AI_ScarabBallInit,
	AI_ArtilleryInit,
};
static const int sgnStartFunctions = sizeof( sgpfnStartFunctions ) / sizeof ( AI_START_FUNC * );

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static AI_DO_FUNC * sgpfnBehaviorFunctionsOld[] =
{			
	AI_ChargedBolt,		//0
	AI_Peacemaker,
	AI_ArcJumper,		//2
	AI_AssassinBugHive,
	AI_FirebrandPod,	//4
	AI_ChargedBoltNova,
	AI_ScarabBall,		//6
	AI_HandGrappler,
	AI_EelLauncher,		//8
	AI_FireBeetles,
	AI_SimpleHoming,	//10
	AI_BlessedHammer,
	AI_RandomMovement,  //12
	AI_ThrowSword,		
	AI_ThrowSwordHoming,//14
	AI_Artillery,
	AI_ProximityMine,	//16
	AI_ThrowShield,		
	AI_ThrowShieldHoming,//18
	AI_AttackLocation,
};
static const int sgnBehaviorFunctionsOld = sizeof( sgpfnBehaviorFunctionsOld ) / sizeof ( AI_DO_FUNC * );

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAIGetBehaviorStringHelper(
	const AI_BEHAVIOR_DEFINITION * pBehaviorDefinition,
	const AI_BEHAVIOR_DATA * pBehaviorData,
	char * szName,
	int nNameLength,
	BOOL & bBold,
	EXCELTABLE eTable,
	int nExcelRow,
	const char * pszMissingError )
{
	const char * pszString = ExcelGetStringIndexByLine( NULL, eTable, nExcelRow );
	ASSERT( pBehaviorData->pszName );
	PStrPrintf( szName, nNameLength, "%.2f %s : %s - %1.2f pct", 
		pBehaviorDefinition->fPriority, pBehaviorData->pszName, 
		pszString ? pszString : pszMissingError, pBehaviorDefinition->fChance );
	if ( ! pszString )
		bBold = TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAIGetBehaviorStringHelper(
	const AI_BEHAVIOR_DEFINITION * pBehaviorDefinition,
	const AI_BEHAVIOR_DATA * pBehaviorData,
	char * szName,
	int nNameLength,
	BOOL & bBold,
	EXCELTABLE eTable,
	int nExcelRow1,
	int nExcelRow2,
	const char * pszMissingError )
{
	const char * pszString1 = ExcelGetStringIndexByLine( NULL, eTable, nExcelRow1 );
	const char * pszString2 = ExcelGetStringIndexByLine( NULL, eTable, nExcelRow2 );
	ASSERT( pBehaviorData->pszName );
	PStrPrintf( szName, nNameLength, "%.2f %s : %s, %s - %1.2f pct", 
		pBehaviorDefinition->fPriority, pBehaviorData->pszName, 
		pszString1 ? pszString1 : pszMissingError, 
		pszString2 ? pszString2 : pszMissingError, 
		pBehaviorDefinition->fChance );
	if ( ! pszString1 || ! pszString2 )
		bBold = TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AIGetBehaviorString(
	const AI_BEHAVIOR_DEFINITION * pBehaviorDefinition,
	char * szName,
	int nNameLength,
	BOOL & bBold)
{
	AI_BEHAVIOR_DATA * pBehaviorData = NULL;
	if ( pBehaviorDefinition->nBehaviorId != INVALID_ID )
		pBehaviorData = (AI_BEHAVIOR_DATA *) ExcelGetData( NULL, DATATABLE_AI_BEHAVIOR, pBehaviorDefinition->nBehaviorId );

	bBold = FALSE;
	if ( ! pBehaviorData )
	{
		PStrCopy( szName, "Missing Behavior", nNameLength );
		bBold = TRUE;
	}
	else if ( pBehaviorData->pbUsesSkill[ 0 ] && pBehaviorData->pbUsesSkill[ 1 ] )
	{
		sAIGetBehaviorStringHelper( pBehaviorDefinition, pBehaviorData, szName, nNameLength, bBold, DATATABLE_SKILLS, pBehaviorDefinition->nSkillId, pBehaviorDefinition->nSkillId2, "Error - Missing Skill" );
	} 
	else if ( pBehaviorData->pbUsesSkill[ 0 ] )
	{
		sAIGetBehaviorStringHelper( pBehaviorDefinition, pBehaviorData, szName, nNameLength, bBold, DATATABLE_SKILLS, pBehaviorDefinition->nSkillId, "Error - Missing Skill" );
	} 
	else if ( pBehaviorData->bUsesState )
	{
		sAIGetBehaviorStringHelper( pBehaviorDefinition, pBehaviorData, szName, nNameLength, bBold, DATATABLE_STATES, pBehaviorDefinition->nStateId, "Error - Missing State" );
	}
	else if ( pBehaviorData->bUsesSound )
	{
		sAIGetBehaviorStringHelper( pBehaviorDefinition, pBehaviorData, szName, nNameLength, bBold, DATATABLE_SOUNDS, pBehaviorDefinition->nSoundId, "Error - Missing Sound" );
	}
	else if ( pBehaviorData->bUsesMonster )
	{
		sAIGetBehaviorStringHelper( pBehaviorDefinition, pBehaviorData, szName, nNameLength, bBold, DATATABLE_SPAWN_CLASS, pBehaviorDefinition->nMonsterId, "Error - Missing Monster Spawn" );
	} else {
		ASSERT( pBehaviorData->pszName );
		PStrPrintf( szName, nNameLength, "%.2f %s - %1.2f pct", pBehaviorDefinition->fPriority, pBehaviorData->pszName, pBehaviorDefinition->fChance );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AI_ToggleOutputToConsole()
{
	sgbAIOutputToConsole = !sgbAIOutputToConsole;
	return sgbAIOutputToConsole;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAILog(
	UNIT * pUnit,
	const char * pszMessage,
	AI_BEHAVIOR_INSTANCE * pBehaviorInstance )
{
#ifdef _DEBUG
	if ( !sgbAIOutputToConsole && !LogGetLogging( AI_LOG ) )
		return;
	ASSERT_RETURN(pUnit);

#if ISVERSION(DEVELOPMENT)
	if(UnitGetStat(pUnit, STATS_DONT_LOG_AI))
	{
		return;
	}
#endif

	GAME * pGame = UnitGetGame( pUnit );
	ASSERT_RETURN( pGame );
	int nTick = GameGetTick( pGame );
	char szOutput[ 256 ];
	if ( pBehaviorInstance )
	{
		char szBehavior[ 256 ];
		BOOL bBold = FALSE;
		AIGetBehaviorString( pBehaviorInstance->pDefinition, szBehavior, 256, bBold );
		ASSERT_RETURN( pszMessage );
		if ( bBold )
			PStrPrintf( szOutput, 256, "%d [%c] %d %s %d: %s -------------------------------------> Bad Data\n", nTick, HOST_CHARACTER(pUnit), UnitGetId(pUnit), pszMessage, pBehaviorInstance->nGUID, szBehavior );
		else
			PStrPrintf( szOutput, 256, "%d [%c] %d %s %d: %s\n", nTick, HOST_CHARACTER(pUnit), UnitGetId(pUnit), pszMessage, pBehaviorInstance->nGUID, szBehavior );
	} else {
		PStrPrintf( szOutput, 256, "%d [%c] %d: %s\n", nTick, HOST_CHARACTER(pUnit), UnitGetId(pUnit), pszMessage);
	}
#if !ISVERSION(SERVER_VERSION)
	LogMessage(AI_LOG, szOutput );
	UNIT * pDebugUnit = GameGetDebugUnit(UnitGetGame(pUnit));
	if ( sgbAIOutputToConsole && ( !pDebugUnit || pUnit == pDebugUnit ) )
	{
		WCHAR wOutput[ 256 ];
		PStrCvt( wOutput, szOutput, 256 );
		ConsoleString( CONSOLE_AI_COLOR, wOutput );
	}
#else
	TraceGameInfo("%s",szOutput);
#endif //SERVER_VERSION
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static AI_BEHAVIOR_INSTANCE_TABLE * sGetBaseTable(
	UNIT * pUnit )
{
	AI_INSTANCE * pInstance = pUnit->pAI;
	return pInstance ? &pInstance->tTable : NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static AI_BEHAVIOR_INSTANCE_TABLE * sGetCurrentTableAt(
	AI_CONTEXT & tContext,
	int nDepth )
{
	ASSERT_RETZERO(nDepth <= tContext.nStackCurr);

	AI_BEHAVIOR_INSTANCE_TABLE * pTable = &tContext.pAIInstance->tTable;
	for(int i=0; i<nDepth; i++)
	{
		ASSERT_RETNULL( pTable );
		ASSERT_RETNULL( tContext.pnStack[ i ] < pTable->nBehaviorCount );
		ASSERT_RETNULL( tContext.pnStack[ i ] >= 0 );
		pTable = pTable->pBehaviors[ tContext.pnStack[ i ] ].pTable;
	}
	return pTable;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static AI_BEHAVIOR_INSTANCE_TABLE * sGetParentTable( 
	AI_CONTEXT & tContext )
{
	return tContext.nStackCurr > 0 ? sGetCurrentTableAt(tContext, tContext.nStackCurr - 1) : NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static AI_BEHAVIOR_INSTANCE_TABLE * sGetCurrentTable( 
	AI_CONTEXT & tContext )
{
	return sGetCurrentTableAt(tContext, tContext.nStackCurr);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline int sGetParentIndex( 
	AI_CONTEXT & tContext )
{
	return tContext.nStackCurr >= 0 ? tContext.pnStack[ tContext.nStackCurr - 1 ] : -1;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static inline int & sGetCurrentIndex( 
	AI_CONTEXT & tContext )
{
	return tContext.pnStack[ tContext.nStackCurr ];
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static AI_BEHAVIOR_DATA * sGetAIBehaviorData( 
	AI_BEHAVIOR_INSTANCE_TABLE * pTable,
	int nIndex)
{
	AI_BEHAVIOR_DATA * pAIBehaviorData = NULL;
	if(pTable && nIndex >= 0)
	{
		pAIBehaviorData = (AI_BEHAVIOR_DATA*)ExcelGetData(NULL, DATATABLE_AI_BEHAVIOR, pTable->pBehaviors[nIndex].pDefinition->nBehaviorId);
	}
	return pAIBehaviorData;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sFreeBehaviorInstance(
	GAME * pGame,
	AI_BEHAVIOR_INSTANCE * pBehaviorInstance )
{
	if ( ! pBehaviorInstance->pTable )
		return;

	for ( int i = 0; i < pBehaviorInstance->pTable->nBehaviorCount; i++ )
	{
		if ( pBehaviorInstance->pTable->pBehaviors[ i ].pTable )
		{
			sFreeBehaviorInstance( pGame, & pBehaviorInstance->pTable->pBehaviors[ i ] );
		}
	}

	if ( pBehaviorInstance->pTable )
	{
		if ( pBehaviorInstance->pTable->pBehaviors )
			GFREE( pGame, pBehaviorInstance->pTable->pBehaviors );
		GFREE( pGame, pBehaviorInstance->pTable );
		pBehaviorInstance->pTable = NULL;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sBehaviorInstanceInit(
	GAME* pGame,
	AI_BEHAVIOR_INSTANCE   * pInstance,
	const AI_BEHAVIOR_DEFINITION * pDefinition,
	int nGuid = INVALID_ID)
{
	pInstance->pDefinition = pDefinition;
	pInstance->nGUID = nGuid != INVALID_ID ? nGuid : GameGetAIGuid( pGame );
	pInstance->pTable = NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCopyTable(
	GAME* pGame,
	AI_BEHAVIOR_INSTANCE_TABLE	 * pTableDest,
	const AI_BEHAVIOR_DEFINITION_TABLE * pTableSrc )
{
	pTableDest->nBehaviorCount = pTableSrc->nBehaviorCount;
	pTableDest->nCurrentBehavior = INVALID_ID;

	ASSERT_RETURN( pTableDest->nBehaviorCount < 100 );
	ASSERT_RETURN( pTableDest->nBehaviorCount >= 0 );

	pTableDest->pBehaviors = ( AI_BEHAVIOR_INSTANCE * ) GMALLOCZ( pGame, sizeof( AI_BEHAVIOR_INSTANCE ) * pTableDest->nBehaviorCount );
	for ( int i = 0; i < pTableSrc->nBehaviorCount; i++ )
	{
		ASSERT_RETURN( pTableSrc->pBehaviors[ i ].tTable.nBehaviorCount < 100 );
		ASSERT_RETURN( pTableSrc->pBehaviors[ i ].tTable.nBehaviorCount >= 0 );

		sBehaviorInstanceInit( pGame, &pTableDest->pBehaviors[ i ], &pTableSrc->pBehaviors[ i ] );

		if ( pTableSrc->pBehaviors[ i ].tTable.nBehaviorCount != 0 )
		{
			pTableDest->pBehaviors[ i ].pTable = (AI_BEHAVIOR_INSTANCE_TABLE *) GMALLOC( pGame, sizeof( AI_BEHAVIOR_INSTANCE_TABLE ) );
			sCopyTable( pGame, pTableDest->pBehaviors[ i ].pTable, &pTableSrc->pBehaviors[ i ].tTable );
		} else {
			pTableDest->pBehaviors[ i ].pTable = NULL;
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sAI_InsertBehaviorTableDelayed(
	UNIT * pUnit,
	const AI_BEHAVIOR_DEFINITION_TABLE * pBehaviorDefTable,
	AI_CONTEXT * pContext,
	int nTimeout,
	STATS * pStatsForIds)
{
	ASSERT_RETFALSE( pUnit );
	ASSERT_RETFALSE( pBehaviorDefTable );

	GAME * pGame = UnitGetGame(pUnit);
	ASSERT_RETFALSE( pGame );

	if ( ! pBehaviorDefTable->nBehaviorCount )
		return FALSE;

	// Resize the array
	pContext->pInsertedBehaviors = (AI_CONTEXT_INSERTED_BEHAVIORS*)GREALLOC(pGame,
																		pContext->pInsertedBehaviors,
																		(pContext->nInsertedBehaviorCount + pBehaviorDefTable->nBehaviorCount) * 
																			sizeof(AI_CONTEXT_INSERTED_BEHAVIORS));

	ASSERT_RETFALSE(pContext->pInsertedBehaviors);

	for( int i = 0; i < pBehaviorDefTable->nBehaviorCount; i++ )
	{
		pContext->pInsertedBehaviors[pContext->nInsertedBehaviorCount + i].pInsertedBehavior = &pBehaviorDefTable->pBehaviors[ i ];
		pContext->pInsertedBehaviors[pContext->nInsertedBehaviorCount + i].nTimeout = nTimeout;
		pContext->pInsertedBehaviors[pContext->nInsertedBehaviorCount + i].nGUID = GameGetAIGuid(pGame);
		if ( pStatsForIds )
			StatsSetStat(pGame, pStatsForIds, STATS_AI_GUID, i, pContext->pInsertedBehaviors[pContext->nInsertedBehaviorCount + i].nGUID);
	}

	pContext->nInsertedBehaviorCount += pBehaviorDefTable->nBehaviorCount;
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sRemoveAIEntryCallback(GAME* pGame, UNIT* pUnit, const struct EVENT_DATA& event_data)
{
	AI_RemoveBehavior(pUnit, (int)event_data.m_Data1);
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sAI_InsertBehavior(
	UNIT * pUnit,
	const AI_BEHAVIOR_DEFINITION * pBehaviorDef,
	int nGuid = INVALID_ID,
	int nTimeout = INVALID_ID)
{
	GAME * pGame = UnitGetGame( pUnit );

	AI_BEHAVIOR_INSTANCE_TABLE * pTableDest = pUnit->pAI ? &pUnit->pAI->tTable : NULL;
	ASSERT_RETINVALID( pTableDest );
	ASSERT_RETINVALID( pBehaviorDef );

	// Figure out the index that we'll be adding the new behavior instance to
	int nIndex;
	for( nIndex = 0; nIndex < pTableDest->nBehaviorCount; nIndex++)
	{
		if ( pTableDest->pBehaviors[ nIndex ].pDefinition->fPriority <= pBehaviorDef->fPriority )
		{
			break;
		}
	}

	// Resize the destination table's array and move the items down
	pTableDest->nBehaviorCount++;
	pTableDest->pBehaviors = ( AI_BEHAVIOR_INSTANCE * ) GREALLOC( pGame, pTableDest->pBehaviors, sizeof( AI_BEHAVIOR_INSTANCE ) * pTableDest->nBehaviorCount );
	MemoryMove( pTableDest->pBehaviors + nIndex + 1, (pTableDest->nBehaviorCount - nIndex - 1) * sizeof(AI_BEHAVIOR_INSTANCE), 
		pTableDest->pBehaviors + nIndex,
		sizeof( AI_BEHAVIOR_INSTANCE ) * ( pTableDest->nBehaviorCount - nIndex - 1 ) );

	// initialize the new instance
	sBehaviorInstanceInit( pGame, &pTableDest->pBehaviors[nIndex], pBehaviorDef, nGuid );

	// Copy the item's table
	if ( pBehaviorDef->tTable.nBehaviorCount > 0 )
	{
		pTableDest->pBehaviors[ nIndex ].pTable = (AI_BEHAVIOR_INSTANCE_TABLE *) GMALLOC( pGame, sizeof( AI_BEHAVIOR_INSTANCE_TABLE ) );
		sCopyTable(pGame, pTableDest->pBehaviors[ nIndex ].pTable, &pBehaviorDef->tTable);
	}

	if ( nTimeout > 0 )
	{
		UnitRegisterEventTimed(pUnit, sRemoveAIEntryCallback, EVENT_DATA( nGuid ), nTimeout);
	}

	return pTableDest->pBehaviors[nIndex].nGUID;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sAI_InsertBehaviorTablePreChecks(
	UNIT * pUnit)
{
	ASSERT_RETFALSE(pUnit);
	int nAI = UnitGetStat( pUnit, STATS_AI);
	if ( nAI == INVALID_ID )
		return FALSE;

	if ( ! UnitTestFlag( pUnit, UNITFLAG_AI_INIT ) )
	{
		AI_Init( UnitGetGame( pUnit ), pUnit );
		AI_Register( pUnit, FALSE );
	}

	if ( ! pUnit->pAI || !UnitTestFlag( pUnit, UNITFLAG_AI_INIT ) )
		return FALSE;
	
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define MAX_AI_BEHAVIORS_TO_ADD 16

static BOOL sAI_InsertBehaviorTable(
	UNIT * pUnit,
	const AI_BEHAVIOR_DEFINITION_TABLE * pTable,
	STATS * pStatsForIds,
	int nTimeout)
{
	if(pUnit->pAI->pContext)
	{
		return sAI_InsertBehaviorTableDelayed(pUnit, pTable, pUnit->pAI->pContext, nTimeout, pStatsForIds);
	}

	GAME * pGame = UnitGetGame( pUnit );

	int nBehaviorCount = MIN(MAX_AI_BEHAVIORS_TO_ADD, pTable->nBehaviorCount);
	for( int i = 0; i < nBehaviorCount; i++ )
	{
		int nGuid = sAI_InsertBehavior( pUnit, &pTable->pBehaviors[ i ], INVALID_ID, nTimeout );
		if ( pStatsForIds )
			StatsSetStat(pGame, pStatsForIds, STATS_AI_GUID, i, nGuid);
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AI_InsertTable(
	UNIT * pUnit,
	const AI_BEHAVIOR_DEFINITION_TABLE * pTable,
	STATS * pStatsForIds,
	int nTimeout)
{
	if(!sAI_InsertBehaviorTablePreChecks(pUnit))
		return FALSE;

	return sAI_InsertBehaviorTable(pUnit, pTable, pStatsForIds, nTimeout);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AI_InsertTable(
	UNIT * pUnit,
	int nAIDefinition,
	STATS * pStatsForIds,
	int nTimeout)
{
	if(!sAI_InsertBehaviorTablePreChecks(pUnit))
		return FALSE;

	AI_DEFINITION * pAIDef = (AI_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_AI, nAIDefinition );
	if (!pAIDef)
		return FALSE;

	return sAI_InsertBehaviorTable(pUnit, &pAIDef->tTable, pStatsForIds, nTimeout);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AI_RemoveTableByStats(
	UNIT * pUnit,
	STATS * pStatsWithIds )
{
	ASSERT_RETURN( pUnit );
	if (!pUnit->pAI)
	{
		return;
	}
	if (!pStatsWithIds)
	{
		return;
	}

	BOOL bHasAI = UnitGetStat( pUnit, STATS_AI );
	if (! bHasAI)
		return;

	STATS_ENTRY pStatsEntries[ MAX_AI_BEHAVIORS_TO_ADD ];
	int nTargets = StatsGetRange( pStatsWithIds, STATS_AI_GUID, 0, pStatsEntries, MAX_AI_BEHAVIORS_TO_ADD );
	for( int i = 0; i < nTargets; i++ )
	{
		int nGuid = pStatsEntries[i].value;
		AI_RemoveBehavior( pUnit, nGuid );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define ANGER_TIME 20
static void sHandleAngerSelf(
	GAME* pGame,
	UNIT* defender,
	UNIT* attacker,
	EVENT_DATA* ,
	void* ,
	DWORD )
{
	GAME_TICK tiLastAngerTick = UnitGetStat(defender, STATS_LAST_ANGER_TICK);
	GAME_TICK tiCurrentTick = GameGetTick(pGame);
	if((tiCurrentTick - tiLastAngerTick) < ANGER_TIME)
	{
		return;
	}
	if ( ! AIIsBusy( pGame, defender ) )
	{ // make sure he is awake and ready to fire back
		// source unit...
		if( AppIsTugboat() )	// don't we want to choose an attacker?
		{
			UNITID idAttackerSource = UnitGetOwnerId( attacker );
			if ( idAttackerSource == INVALID_ID )
				idAttackerSource = UnitGetId( attacker );
			UnitSetStat( defender, STATS_AI_LAST_ATTACKER_ID, idAttackerSource );
		}
		UnitSetStat(defender, STATS_LAST_ANGER_TICK, tiCurrentTick);
		AI_Register(defender, FALSE, 1);
	}
}

#define MAX_FRIENDS_TO_ANGER	12
void AI_AngerOthers(
	GAME* pGame,
	UNIT* defender,
	UNIT* attacker,
	int nRange /*= -1*/ )
{
	if ( ! attacker )
		return;

	// source unit...
	UNITID idAttackerSource = UnitGetOwnerId( attacker );
	if ( idAttackerSource == INVALID_ID )
		idAttackerSource = UnitGetId( attacker );
	if ( defender )
		idAttackerSource = UnitGetStat( defender, STATS_AI_LAST_ATTACKER_ID );

	if ( nRange < 0 )
	{
		const UNIT_DATA *pUnitData = UnitGetData(defender);
		ASSERT_RETURN(pUnitData);
		nRange = pUnitData->nAngerRange;
	}

	UNITID pnUnitIds[ MAX_FRIENDS_TO_ANGER ];
	SKILL_TARGETING_QUERY tTargetingQuery;
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_FIRSTFOUND );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_FRIEND );
	tTargetingQuery.fMaxRange	= (float)nRange;
	tTargetingQuery.nUnitIdMax  = MAX_FRIENDS_TO_ANGER;
	tTargetingQuery.pnUnitIds   = pnUnitIds;
	tTargetingQuery.pSeekerUnit = defender;
	SkillTargetQuery( pGame, tTargetingQuery );

	for ( int i = 0; i < tTargetingQuery.nUnitIdCount; i++ )
	{
		UNIT * pTarget = UnitGetById( pGame, tTargetingQuery.pnUnitIds[ i ] );
		if ( pTarget )
		{
			UnitSetStat( pTarget, STATS_AI_LAST_ATTACKER_ID, idAttackerSource );
		}
	}

	sHandleAngerSelf( pGame, defender, attacker, NULL, NULL, INVALID_ID );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHandleAngerOthers(
	GAME* pGame,
	UNIT* defender,
	UNIT* attacker,
	EVENT_DATA* pHandlerData,
	void* data,
	DWORD dwId )
{
	ASSERT_RETURN(pHandlerData);
	int nRange = (int)pHandlerData->m_Data1;

	AI_AngerOthers(pGame, defender, attacker, nRange);

	// only do this once from damage
	UnitUnregisterEventHandler( pGame, defender, EVENT_GOTHIT, dwId );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define MAX_FRIENDS_TO_GIVE_STATE_TO	12
static void sHandleGiveState(
	GAME* pGame,
	UNIT* defender,
	UNIT* attacker,
	EVENT_DATA* pHandlerData,
	void* data,
	DWORD dwId )
{
	if ( ! defender )
		return;

	float fRange	= EventParamToFloat( pHandlerData->m_Data1 );
	int nState		= (int)pHandlerData->m_Data2;
	int nUnitType	= (int)pHandlerData->m_Data3;

	UNITID pnUnitIds[ MAX_FRIENDS_TO_GIVE_STATE_TO ];
	SKILL_TARGETING_QUERY tTargetingQuery;
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_FIRSTFOUND );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_FRIEND );
	tTargetingQuery.fMaxRange	= fRange;
	tTargetingQuery.nUnitIdMax  = MAX_FRIENDS_TO_GIVE_STATE_TO;
	tTargetingQuery.pnUnitIds   = pnUnitIds;
	tTargetingQuery.tFilter.nUnitType = nUnitType;
	tTargetingQuery.pSeekerUnit = defender;
	SkillTargetQuery( pGame, tTargetingQuery );

	for ( int i = 0; i < tTargetingQuery.nUnitIdCount; i++ )
	{
		UNIT * pTarget = UnitGetById( pGame, tTargetingQuery.pnUnitIds[ i ] );
		if ( ! pTarget )
			continue;

		if ( UnitHasState( pGame, pTarget, nState ) )
			continue;

//		Tugboat had this code, but all of this can be done through the state			
		if ( AppIsTugboat() )
		{// TODO: remove this.  Get it into the state that Tugboat is using.
			//SkillStopAll( pGame, pTarget );
			//AI_Register(pTarget, FALSE, 1);
		}

		s_StateSet( pTarget, defender, nState, 0 );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AI_RemoveBehavior( 
	UNIT * pUnit,
	AI_BEHAVIOR_INSTANCE_TABLE * pTable,
	int nIndex,
	AI_CONTEXT * pContext )
{
	GAME * pGame = UnitGetGame( pUnit );

	ASSERT_RETURN( nIndex < pTable->nBehaviorCount );
	ASSERT_RETURN( nIndex >= 0 );
	AI_BEHAVIOR_INSTANCE * pBehaviorInstance = & pTable->pBehaviors[ nIndex ];

	// fix the stack if necessary
	if ( pContext )
	{
		for ( int i = 0; i <= pContext->nStackCurr; i++ )
		{
			AI_BEHAVIOR_INSTANCE_TABLE * pCurTable = sGetCurrentTableAt(*pContext, i);

			if ( pCurTable == pBehaviorInstance->pTable )
			{
				ASSERT_RETURN( i != 0 );
				ASSERT_RETURN( pContext->nStackCurr != 0 );
				pContext->nStackCurr = i - 1;
				pContext->pnStack[ i - 1 ]--;
				break;
			}
			if ( pCurTable == pTable )
			{
				if ( pContext->pnStack[ i ] == nIndex )
				{ 
					//if (pContext->pnStack[ i ] > 0)
					{
						pContext->pnStack[ i ]--;
					}
				}
			}
		}
	}

	if ( pBehaviorInstance->pTable )
	{
		sFreeBehaviorInstance( pGame, pBehaviorInstance );
	}
	if ( nIndex < pTable->nBehaviorCount - 1 )
	{
		MemoryMove( pTable->pBehaviors + nIndex, (pTable->nBehaviorCount - nIndex) * sizeof(AI_BEHAVIOR_INSTANCE),
			pTable->pBehaviors + nIndex + 1,
			sizeof( AI_BEHAVIOR_INSTANCE ) * ( pTable->nBehaviorCount - nIndex - 1 ) );
	}
	pTable->nBehaviorCount--;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sFindAIGuid(
	AI_BEHAVIOR_INSTANCE_TABLE * pTableIn, 
	int nGUID, 
	AI_BEHAVIOR_INSTANCE_TABLE ** pTableOut, 
	int * pnIndex)
{
	for( int i = 0; i < pTableIn->nBehaviorCount; i++ )
	{
		if ( pTableIn->pBehaviors[ i ].nGUID == nGUID )
		{
			*pTableOut = pTableIn;
			*pnIndex = i;
			return TRUE;
		}

		if ( pTableIn->pBehaviors[ i ].pTable )
		{
			if ( sFindAIGuid( pTableIn->pBehaviors[ i ].pTable, nGUID, pTableOut, pnIndex ) )
			{
				return TRUE;
			}
		}
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AI_RemoveBehavior( 
	UNIT * pUnit,
	int nGuid )
{
	AI_INSTANCE * pInstance = pUnit->pAI;
	AI_BEHAVIOR_INSTANCE_TABLE * pTable = NULL;
	int nIndex = -1;
	if ( sFindAIGuid( &pInstance->tTable, nGuid, &pTable, &nIndex ) )
	{
		AI_RemoveBehavior(pUnit, pTable, nIndex, NULL);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AI_RemoveCurrentBehavior( 
	UNIT * pUnit,
	AI_CONTEXT & tContext )
{
	AI_BEHAVIOR_INSTANCE_TABLE * pTable = sGetCurrentTable( tContext );
	ASSERT_RETURN( pTable );
	int nIndex = sGetCurrentIndex( tContext );

	AI_RemoveBehavior( pUnit, pTable, nIndex, &tContext ); 
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AI_Free(
	GAME * pGame,
	UNIT * pUnit)
{
	if ( pUnit->pAI  )
	{
		AI_Unregister(pUnit);
		AI_BEHAVIOR_INSTANCE_TABLE * pTable = & pUnit->pAI->tTable;
		for ( int i = 0; i < pTable->nBehaviorCount; i++ )
		{
			sFreeBehaviorInstance( pGame, & pTable->pBehaviors[ i ] );
		}
		if ( pTable )
			GFREE( pGame, pTable->pBehaviors );
		GFREE( pGame, pUnit->pAI );
		pUnit->pAI = NULL;
		UnitClearFlag( pUnit, UNITFLAG_AI_INIT );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AI_Init(
	GAME* pGame,
	UNIT* pUnit )
{
	ASSERT_RETURN(pGame && pUnit);

	int nAI = UnitGetStat(pUnit, STATS_AI);
	// Marsh add - I need to check to make sure that the unit was a monster; the missiles AI was being overwritten.
	if( AppIsTugboat() && 
		!UnitIsA( pUnit, UNITTYPE_MISSILE ) && !UnitIsArmed( pUnit ) )
	{
		nAI = UnitGetStat(pUnit, STATS_AI_UNARMED);
	}
	

	AI_INIT * pAiData = (AI_INIT *) ExcelGetData( pGame, DATATABLE_AI_INIT, nAI ); 
	ASSERT_RETURN(pAiData);

	BOOL bHasPreferredDirection = UnitGetStat( pUnit, STATS_HAS_PREFERRED_DIRECTION );
	if ( pAiData->bStartOnAiInit || bHasPreferredDirection == TRUE )
	{
		AI_Register(pUnit, pAiData->bRandomStartPeriod );
	}

	if (pUnit->pAI)
	{
		return;
	}

	if (AppGetType() != APP_TYPE_CLOSED_SERVER)
	{
		ASSERT_RETURN( pAiData->pszTableName[ 0 ] != 0 );
		if ( pAiData->nTable == INVALID_ID )
		{
			pAiData->nTable = DefinitionGetIdByName( DEFINITION_GROUP_AI, pAiData->pszTableName );
		}
	}

	ASSERT_RETURN( pAiData->nTable != INVALID_ID );
	AI_DEFINITION * pAiDef = (AI_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_AI, pAiData->nTable );
	if ( ! pAiDef )
	{// this may not be loaded yet
		return;
	}

	UnitSetStat(pUnit, STATS_AI_CIRCLE_DIRECTION, RandGetNum(pGame, 0, 1));

	if ( pAiData->nStartFunction != INVALID_ID && pAiData->nStartFunction < sgnStartFunctions && sgpfnStartFunctions[ pAiData->nStartFunction ] )
	{
		sgpfnStartFunctions[ pAiData->nStartFunction ]( pGame, pUnit, nAI );
	}

	if ( pAiData->bRecordSpawnPoint )
	{
		UnitSetStatVector( pUnit, STATS_AI_ANCHOR_POINT_X, 0, UnitGetPosition( pUnit ) );
	}

	const UNIT_DATA * pUnitData = UnitGetData( pUnit );
	if ( pUnitData->nAngerRange )
	{
		if ( TESTBIT( pUnitData->pdwFlags, UNIT_DATA_FLAG_ANGER_OTHERS_ON_DAMAGED ) )
		{
			UnitRegisterEventHandler(pGame, pUnit, EVENT_GOTHIT, sHandleAngerOthers, EVENT_DATA(pUnitData->nAngerRange));
		}

		if ( TESTBIT( pUnitData->pdwFlags, UNIT_DATA_FLAG_ANGER_OTHERS_ON_DEATH ) )
		{
			UnitRegisterEventHandler(pGame, pUnit, EVENT_GOTKILLED, sHandleAngerOthers, EVENT_DATA(pUnitData->nAngerRange));
		}
	}

	for ( int i = 0; i < MAX_STATES_TO_SHARE; i++ )
	{
		if ( pAiData->pStateSharing[ i ].bOnDeath &&
			 pAiData->pStateSharing[ i ].nState != INVALID_ID )
		{
			EVENT_DATA tEventData;
			tEventData.m_Data1 = EventFloatToParam( pAiData->pStateSharing[ i ].fRange );
			tEventData.m_Data2 = pAiData->pStateSharing[ i ].nState;
			tEventData.m_Data3 = pAiData->pStateSharing[ i ].nTargetUnitType;

			UnitRegisterEventHandler(pGame, pUnit, EVENT_GOTKILLED, sHandleGiveState, &tEventData );
		}
	}

	// always anger yourself
	UnitRegisterEventHandler ( pGame, pUnit, EVENT_GOTHIT, sHandleAngerSelf, NULL );

	pUnit->pAI = (AI_INSTANCE *) GMALLOCZ( pGame, sizeof( AI_INSTANCE ) );

	AI_BEHAVIOR_INSTANCE_TABLE * pTableDest = &pUnit->pAI->tTable;
	sCopyTable( pGame, pTableDest, &pAiDef->tTable );

	UnitSetFlag( pUnit, UNITFLAG_AI_INIT, TRUE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int AIPickRandomChild(
	GAME* pGame,
	AI_BEHAVIOR_INSTANCE_TABLE * pTable )
{
	if ( pTable->nBehaviorCount == 0 )
		return INVALID_ID;

	PickerStartF( pGame, picker );

	for ( int i = 0; i < pTable->nBehaviorCount; i++ )
	{
		PickerAdd(MEMORY_FUNC_FILELINE(pGame, i, pTable->pBehaviors[ i ].pDefinition->fPriority));
	}

	return PickerChoose( pGame );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDoPreferredDirection( 
	UNIT *pUnit)
{
	ASSERTX_RETURN( pUnit, "Expected unit" );
	GAME *pGame = UnitGetGame( pUnit );
	
	if(!UnitTestFlag(pUnit, UNITFLAG_FACE_DURING_INTERACTION))
	{
		return;
	}

	BOOL bHasPreferredDirection = UnitGetStat( pUnit, STATS_HAS_PREFERRED_DIRECTION );	
	if (bHasPreferredDirection)
	{
		// are we facing our preferred direction?
		VECTOR vDirectionPreferred = UnitGetStatVector( pUnit, STATS_PREFERRED_DIRECTION_X, 0 );
		VECTOR vDirectionCurrent = UnitGetFaceDirection( pUnit, FALSE );
		float flCosAngleBetween = VectorDot( vDirectionPreferred, vDirectionCurrent );
		if (flCosAngleBetween < 0.7f)
		{
			BOOL bReturnToPreferred = FALSE;

			// can we go back to our preferred direction
			if (s_TalkIsBeingTalkedTo( pUnit ) == FALSE)
			{
				DWORD dwLastInteractTick = UnitGetStat( pUnit, STATS_LAST_INTERACT_TICK );
				DWORD dwNow = GameGetTick( pGame );
				if (dwNow - dwLastInteractTick >= RETURN_TO_PREFERRED_DIRECTION_TIME)
				{
					bReturnToPreferred = TRUE;
				}
			}

			// go back
			if (bReturnToPreferred == TRUE)
			{
				AI_TurnTowardsTargetXYZ( 
					pGame, 
					pUnit, 
					vDirectionPreferred + UnitGetPosition( pUnit ), 
					UnitGetVUpDirection( pUnit )  );
			}
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AI_Update(
	GAME* pGame,
	UNIT* pUnit,
	const struct EVENT_DATA& event_data)
{
	TIMER_STARTEX("AI_Update", 2);
	ASSERT_RETFALSE(pGame && pUnit);

	UnitSetStat(pUnit, STATS_LAST_AI_UPDATE_TICK, GameGetTick(pGame));

	if (IsUnitDeadOrDying(pUnit))
	{
		return FALSE;
	}

	// do not run ai for units not in rooms, like saved pets in an inventory when you come into a game	
	if (UnitGetRoom(pUnit) == NULL)
	{
		return FALSE;
	}
	if (!RoomIsActive(UnitGetRoom(pUnit)))
	{
		if (UnitEmergencyDeactivate(pUnit, "Attempt to run AI in Inactive Room"))
		{
			return FALSE;
		}
	}

	int nAI = UnitGetStat(pUnit, STATS_AI);

	if ( !UnitTestFlag( pUnit, UNITFLAG_AI_INIT ) )
	{
		if ( nAI != INVALID_ID )
		{
			AI_Init( pGame, pUnit );
			AI_Register( pUnit, FALSE );
		}
		return FALSE;
	}

	if ( UnitHasState( pGame, pUnit, STATE_DONT_RUN_AI ) )
	{
		AI_Register( pUnit, FALSE );
	}

	AI_INIT * pAiData = (AI_INIT *) ExcelGetData( pGame, DATATABLE_AI_INIT, nAI ); 
	ASSERT_RETFALSE(pAiData);

	BOOL bDontRunAI = FALSE;
	if ( pAiData->nBlockingState != INVALID_ID && UnitHasState( pGame, pUnit, pAiData->nBlockingState ) )
	{
		bDontRunAI = TRUE;
	}

	if ( !bDontRunAI && GameGetDebugFlag(pGame, DEBUGFLAG_AI_FREEZE) && !pAiData->bAiNoFreeze && !UnitHasState(pGame, pUnit, STATE_UNFREEZE_AI) )
	{
		bDontRunAI = TRUE;
	}

#if ! ISVERSION(CLIENT_ONLY_VERSION)
	if ( AppCommonGetDemoMode() )
	{
		bDontRunAI = TRUE;
	}
#endif

	if ( !bDontRunAI )
	{
		sAILog( pUnit, "---- Run AI", NULL );
	}

	if ( !bDontRunAI && pAiData->bCheckBusy && AIIsBusy( pGame, pUnit ) )
	{
		sAILog( pUnit, "Too Busy", NULL );
		bDontRunAI = TRUE;
	}
	
	// Marsh - the stat STATS_AI_AWAKE_RANGE is zero for missiles. So the missile's AI would never be ran because it could never find a target.
	// Tyler - Hellgate data isn't ready for this yet.
	// Tyler - There are too many checks here.  It should just be a bool in UNIT_DATA.
	if( AppIsTugboat() &&
		!bDontRunAI && !UnitIsNPC( pUnit ) && UnitGetGenus( pUnit ) != GENUS_MISSILE &&
		!PetIsPlayerPet( pGame, pUnit ) )	// npcs and missiles always awake & wandering.
	{
		// if there's nothing nearby, why do the AI?
		DWORD pdwFlags[ NUM_TARGET_QUERY_FILTER_FLAG_DWORDS ];
		ZeroMemory( pdwFlags, sizeof( DWORD ) * NUM_TARGET_QUERY_FILTER_FLAG_DWORDS );
		SETBIT( pdwFlags, SKILL_TARGETING_BIT_CLOSEST );
		UNIT* pNearest = AI_ShouldBeAwake( pGame, pUnit, pdwFlags );
		if( !pNearest )
		{
			//(RandGetNum ) I know this seems weird but it does a great job of making monsters not all
			//awake at the same instant.
			UnitTriggerEvent( pGame, EVENT_AI_WENT_DORMANT, pUnit, NULL, NULL ); //broadcast the event for the DORMANT state of the monster
			AI_Register(pUnit, FALSE, 40 + RandGetNum( pGame, 20 ) ); 
			return TRUE;
		}
	}
	

	if ( bDontRunAI )
	{
		AI_Register(pUnit, FALSE);
		return TRUE;
	}

	UNIT* target = NULL;
	
	//Marsh - the following check was made for missiles because the target will get set to NULL due to the fact that the FOLLOW_RANGE on a missile is zero.	
	if( AppIsTugboat() &&
		UnitGetGenus( pUnit ) != GENUS_MISSILE )
	{
		target = UnitGetAITarget(pUnit);
		if( target )
		{
			float fAiFollowRange = (float)UnitGetStat(pUnit, STATS_AI_FOLLOW_RANGE);
			if ( fAiFollowRange != 0.0f )
			{
				VECTOR Delta = target->vPosition - pUnit->vPosition;
				// FIXME : need a real 'lose target' range here.
				if( VectorLength( Delta ) > fAiFollowRange )
				{
					target = NULL;
					UnitSetMoveTarget( pUnit, INVALID_ID );
					UnitSetAITarget(pUnit, INVALID_ID);
					UnitSetStat(pUnit, STATS_AI_LAST_ATTACKER_ID, INVALID_ID);
				}
			}
		}
	}
	if ( !target && pAiData->bWantsTarget && !GameGetDebugFlag(pGame, DEBUGFLAG_AI_NOTARGET))
	{
		int nSkill = sAIGetFirstSkill(pUnit);
		if ( nSkill != INVALID_ID )
		{
			DWORD pdwFlags[ NUM_TARGET_QUERY_FILTER_FLAG_DWORDS ];
			ZeroMemory( pdwFlags, sizeof( DWORD ) * NUM_TARGET_QUERY_FILTER_FLAG_DWORDS );
			SETBIT( pdwFlags, SKILL_TARGETING_BIT_CLOSEST );
			DWORD dwAIGetTargetFlags = 0;
			target = AI_GetTarget(pGame, pUnit, nSkill, pdwFlags, dwAIGetTargetFlags);
			if (target)
			{
				UnitSetAITarget(pUnit, target);
			}
		}
	}

	// do any turning back to preferred directions
	sDoPreferredDirection( pUnit );

	AI_BEHAVIOR_INSTANCE_TABLE * pBaseTable = sGetBaseTable( pUnit );
	if ( pBaseTable->nBehaviorCount == 0  &&
		!IsUnitDeadOrDying( pUnit ) )
	{
		AI_Register( pUnit, FALSE, INVALID_ID );	
		return FALSE;
	}

	AI_CONTEXT tContext;
	ZeroMemory( &tContext, sizeof( tContext ) );

	// WARNING WARNING WARNING!
	// If you exit early from this function you MUST MUST MUST reset pUnit->pAI->pContext = NULL;
	// If you do not do this, don't be surprised if the game crashes later on in sAI_InsertBehaviorTableDelayed()
	pUnit->pAI->pContext = &tContext;

	tContext.pAIInstance = pUnit->pAI;
	tContext.nBranchTo = INVALID_ID;
	tContext.nPeriodOverride = -1;

	BOOL bFinished = FALSE;
	int nRemoveBehaviorGuid = INVALID_ID;
	while ( TRUE )
	{
		// grab the current table and behavior
		AI_BEHAVIOR_INSTANCE_TABLE * pTable = sGetCurrentTable( tContext );
		int nIndex							= sGetCurrentIndex( tContext );
		ASSERT_GOTO( nIndex < pTable->nBehaviorCount, error );
		AI_BEHAVIOR_INSTANCE * pBehaviorInstance = & pTable->pBehaviors[ nIndex ];
		ASSERT_GOTO( pBehaviorInstance->pDefinition->nBehaviorId != INVALID_ID, error );
		AI_BEHAVIOR_DATA * pBehaviorData = (AI_BEHAVIOR_DATA *) ExcelGetData( pGame, DATATABLE_AI_BEHAVIOR, pBehaviorInstance->pDefinition->nBehaviorId );

		// first see if we need to do things the old way
		int nFunction = pBehaviorData->nFunctionTheOldWay;
		if ( nFunction != INVALID_ID )
		{
			sAILog( pUnit, "---- Ran the old way:", pBehaviorInstance );
			ASSERT_GOTO( nFunction >= 0 && nFunction < sgnBehaviorFunctionsOld, error );
			ASSERT_GOTO( sgpfnBehaviorFunctionsOld[ nFunction ], error );
			bFinished = TRUE;
			tContext.bAddAIEvent = sgpfnBehaviorFunctionsOld[ nFunction ]( pGame, pUnit, target, nAI );
			break; // until we change the AI functions, we just run the first one
		}

		pTable->nCurrentBehavior = nIndex;

		// now do it the new way
		float fChance = pBehaviorInstance->pDefinition->fChance;
		if ( fChance >= 1.0f || fChance > RandGetFloat(pGame) )
		{

			sAILog( pUnit, "Rolled for and selected:", pBehaviorInstance );
			TIMER_STARTEX(pBehaviorData->pszName, 2);
			BOOL bSuccess = TRUE;
			if ( pBehaviorData->pfnFunction != NULL )
				bSuccess = pBehaviorData->pfnFunction( pGame, pUnit, pBehaviorInstance, tContext );


			if (bSuccess && pBehaviorInstance->pDefinition->dwFlags & AI_BEHAVIOR_FLAG_ONCE && nRemoveBehaviorGuid < 0 )
			{
				nRemoveBehaviorGuid = pBehaviorInstance->nGUID;
			}

			// The behavior instance array may have been changed, so reget it
			if ( tContext.bStopAI )
			{ 
				sAILog( pUnit, "---- Stopped on:", pBehaviorInstance );
				bFinished = TRUE;
				break;
			}

			pTable = sGetCurrentTable( tContext );
			nIndex = sGetCurrentIndex( tContext );
			pBehaviorInstance = & pTable->pBehaviors[ MAX(nIndex, 0) ];

		} else {
			sAILog( pUnit, "Rolled for and failed:", pBehaviorInstance );
		}

		if ( tContext.nBranchTo != INVALID_ID )
		{
			sAILog( pUnit, "Branched on:", pBehaviorInstance );

			ASSERT_GOTO( tContext.nStackCurr >= 0, error );
			ASSERT_GOTO( tContext.nStackCurr < MAX_AI_BEHAVIOR_TREE_DEPTH - 1, error );
			ASSERT_GOTO( pBehaviorInstance->pTable, error );
			ASSERT_GOTO( pBehaviorInstance->pTable->pBehaviors, error );
			ASSERT_GOTO( pBehaviorInstance->pTable->nBehaviorCount, error );
			ASSERT_GOTO( tContext.nBranchTo < pBehaviorInstance->pTable->nBehaviorCount, error );
			tContext.nStackCurr++;
			sGetCurrentIndex(tContext) = tContext.nBranchTo;
			tContext.nBranchTo = INVALID_ID;
		} 
		else 
		{
			sGetCurrentIndex(tContext)++;

			BOOL bDone = FALSE;
			AI_BEHAVIOR_INSTANCE_TABLE * pSubTable = sGetCurrentTable(tContext);
			AI_BEHAVIOR_INSTANCE_TABLE * pParentTable = sGetParentTable(tContext);
			int nParentIndex = sGetParentIndex(tContext);
			AI_BEHAVIOR_DATA * pParentData = sGetAIBehaviorData(pParentTable, nParentIndex);
			while ( sGetCurrentIndex(tContext) >= pSubTable->nBehaviorCount ||
					(pParentData && pParentData->bForceExitBranch)) 
			{
				// we have hit the last behavior in the top table
				if ( tContext.nStackCurr == 0 )
				{
					bDone = TRUE;
					break;
				}

				// go up one level in the stack
				tContext.nStackCurr--;

				pSubTable = sGetCurrentTable(tContext);
				pParentTable = sGetParentTable(tContext);
				nParentIndex = sGetParentIndex(tContext);
				pParentData = sGetAIBehaviorData(pParentTable, nParentIndex);

				// move to the next entry at this level
				sGetCurrentIndex(tContext)++;
			}
			if ( bDone )
				break;
		} 
	}

	pUnit->pAI->pContext = NULL;

	if ( tContext.nInsertedBehaviorCount )
	{
		// CHB 2007.07.02 - warning C4189: 'pInsertTable' : local variable is initialized but not referenced
		//AI_BEHAVIOR_INSTANCE_TABLE * pInsertTable = &tContext.pAIInstance->tTable;
		for( int i = 0; i < tContext.nInsertedBehaviorCount; i++ )
		{
			const AI_BEHAVIOR_DEFINITION * pBehaviorDef = tContext.pInsertedBehaviors[ i ].pInsertedBehavior;
			int nTimeout = tContext.pInsertedBehaviors[ i ].nTimeout;
			int nGuid = tContext.pInsertedBehaviors[ i ].nGUID;
			sAI_InsertBehavior( pUnit, pBehaviorDef, nGuid, nTimeout );
		}
		GFREE(pGame, tContext.pInsertedBehaviors);
	}

	BOOL bRegister = ( ! bFinished || tContext.bAddAIEvent );
	if ( !bRegister )
	{ // some units don't use modes to register AI events - these need to register after every thought
		const UNIT_DATA * pUnitData = UnitGetData( pUnit );
		if ( UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_MODES_IGNORE_AI) )
			bRegister = TRUE;
	}

	// if the skill itself kills us, we don't want to reregister AI afterward.
	if ( bRegister &&
		!IsUnitDeadOrDying( pUnit ) && 
		!UnitTestFlag( pUnit, UNITFLAG_JUSTFREED ) )
		AI_Register( pUnit, FALSE, tContext.nPeriodOverride );

	if( nRemoveBehaviorGuid >= 0 )
	{
		AI_RemoveBehavior(pUnit, nRemoveBehaviorGuid);
	}

	sAISendTargetToClient(pUnit);

	return TRUE;
	
error:
	pUnit->pAI->pContext = NULL;
	return FALSE;
}

