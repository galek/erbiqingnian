//----------------------------------------------------------------------------
// FILE: proc.cpp
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "combat.h"
#include "console.h"
#include "game.h"
#include "inventory.h"
#include "proc.h"
#include "units.h"
#include "skills.h"
#include "stats.h"
#include "unit_priv.h"

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
#if ISVERSION(CHEATS_ENABLED)
static BOOL sgnProbabilityOverride = 0;
#endif
BOOL gbProcTrace = FALSE;

static DWORD PROC_SKILL_START_FLAGS = 
		SKILL_START_FLAG_NO_REPEAT_EVENT |
		SKILL_START_FLAG_DONT_RETARGET |
		SKILL_START_FLAG_DONT_SET_COOLDOWN |
		SKILL_START_FLAG_IGNORE_COOLDOWN |
		SKILL_START_FLAG_INITIATED_BY_SERVER |
		SKILL_START_FLAG_SEND_FLAGS_TO_CLIENT;

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
enum PROC_DELIVERY_TYPE
{
	PDT_INVALID = -1,
	
	PDT_ON_ATTACK,
	PDT_ON_DID_HIT,
	PDT_ON_GOT_HIT,
	PDT_ON_DID_KILL,
	PDT_ON_GOT_KILLED,
	
	PDT_NUM_TYPES			// keep this last please
};

//----------------------------------------------------------------------------
struct DELAYED_PROC
{
	int nSkill;
	UNITID idTarget;
	VECTOR vTargetLocation;
	UNITID idInstrumentToApply;
	int nDamageIncrement;
};

//----------------------------------------------------------------------------
struct PROC_INSTANCE
{

	GAME *pGame;

	PROC_DELIVERY_TYPE eDeliveryType;	// how proc was delivered (on hit, on attack, on kill, etc)
	int nProc;							// row in procs.xls
	int nProcLevel;
	DWORD dwChance;
	DWORD dwLastRunAttemptTick;			// last tick the proc was attempted to be run on
	DWORD dwLastExecutionTick;			// last tick this proc was executed on
	int nDamageIncrement;				// damage increment as percent 0-100

	UNIT *pAttacker;					// the attacker that weilds pInstrument
	UNIT *pTarget;						// for on damage or on kill type procs this is the target
	UNIT *pInstrument;					// item that has the proc

	EVENT_DATA *pEventData;				// event data used to trigger proc

	PROC_INSTANCE::PROC_INSTANCE(
		PROC_DELIVERY_TYPE eDeliveryType,
		UNIT *pAttacker,
		UNIT *pInstrument,
		UNIT *pTarget,
		EVENT_DATA *pEventData,
		int nDamageIncrement)
	{	
		ASSERT( pInstrument );		// all procs must be executed from an instrument
		this->pGame = UnitGetGame( pInstrument );
		
		this->eDeliveryType = eDeliveryType;
		this->nProc = (int)pEventData->m_Data1;
		this->nProcLevel = (int)pEventData->m_Data3;
		this->dwChance = (DWORD)pEventData->m_Data2;
		this->dwLastRunAttemptTick = (DWORD)pEventData->m_Data4;	// this gets put back in the destructor
		this->dwLastExecutionTick = (DWORD)pEventData->m_Data5;	// this gets put back in the destructor
		this->nDamageIncrement = nDamageIncrement;

		this->pAttacker = pAttacker;
		this->pTarget = pTarget;
		this->pInstrument = pInstrument;
		this->pEventData = pEventData;

	}

	PROC_INSTANCE::~PROC_INSTANCE(
		void)
	{
		// put back values that we want to preserve till next invocation
		pEventData->m_Data4 = dwLastRunAttemptTick;
		pEventData->m_Data5 = dwLastExecutionTick;	
	}

};

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PROC_DEFINITION *ProcGetDefinition(
	GAME* pGame,
	int nProc)
{
	return (PROC_DEFINITION *)ExcelGetData( pGame, DATATABLE_PROCS, nProc );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(CHEATS_ENABLED)
int ProcSetProbabilityOverride(
	int nProbability)
{
	sgnProbabilityOverride = nProbability;
	return sgnProbabilityOverride;
}
int ProcGetProbabilityOverride()
{
	return sgnProbabilityOverride;
}
#endif

#if !ISVERSION(CLIENT_ONLY_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL s_sDoesProcApplyWeapon( 
	const PROC_INSTANCE *pProcInstance)
{
	ASSERTX_RETFALSE( pProcInstance, "Expected proc instance" );

	// only instruments that are weapons apply weapon
	return UnitIsA( pProcInstance->pInstrument, UNITTYPE_WEAPON );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sDoDelayedProcSkill(
	GAME *pGame,
	UNIT *pUnit,
	const struct EVENT_DATA& tEventData)
{
	DELAYED_PROC *pDelayedProc = (DELAYED_PROC *)tEventData.m_Data1;
	ASSERT_RETFALSE(pDelayedProc);

	ASSERT_RETFALSE( MAX_WEAPONS_PER_UNIT == 2 );
	// get the instrument to apply
	UNIT *pInstruments[ MAX_WEAPONS_PER_UNIT ];
	pInstruments[ 0 ] = UnitGetById( pGame, pDelayedProc->idInstrumentToApply );
	pInstruments[ 1 ] = NULL;

	// apply instrument
	UNIT * pOldEquippedWeapons[ MAX_WEAPONS_PER_UNIT ];
	if (pInstruments[ 0 ])
	{
		// save old weapon
		UnitInventoryGetEquippedWeapons( pUnit, pOldEquippedWeapons );

		// set instrument as current weapon
		UnitInventorySetEquippedWeapons( pUnit, pInstruments );	

	} else {
		ZeroMemory( pOldEquippedWeapons, sizeof(UNIT *) * MAX_WEAPONS_PER_UNIT );
	}

	STATS * pStats = StatsListInit(pGame);
	if(pStats)
	{
		StatsSetStat(pGame, pStats, STATS_DAMAGE_INCREMENT_PROC, pDelayedProc->nDamageIncrement);
		StatsListAdd(pUnit, pStats, FALSE);
	}

	// do the skill
	SkillStartRequest(
		pGame,
		pUnit,
		pDelayedProc->nSkill,
		pDelayedProc->idTarget,
		pDelayedProc->vTargetLocation,
		PROC_SKILL_START_FLAGS,
		SkillGetNextSkillSeed( pGame ),
		pInstruments[ 0 ] ? UnitGetStat(pInstruments[ 0 ], STATS_LEVEL) : 0);

	if(pStats)
	{
		StatsListRemove( pGame, pStats );

		StatsListFree( pGame, pStats );
	}

	// remove instrument (if any)
	if (pInstruments[ 0 ] != NULL)
	{
		UnitInventorySetEquippedWeapons( pUnit, pOldEquippedWeapons );		
	}

	// free the delayed proc data
	GFREE( pGame, pDelayedProc );

	return TRUE;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UNIT *sProcGetTargetOwner( 
	PROC_INSTANCE *pProcInstance)
{
	ASSERT_RETNULL(pProcInstance);

	UNIT *pTargetLocationSource = NULL;
	const PROC_DEFINITION *pProcDefinition = ProcGetDefinition( pProcInstance->pGame, pProcInstance->nProc );
	ASSERT_RETNULL(pProcDefinition);

	if (TESTBIT( pProcDefinition->dwFlags, PROC_TARGET_INSTRUMENT_OWNER_BIT ))
	{
		// our target location will be at the unit who owns the instrument
		pTargetLocationSource = UnitGetUltimateOwner( pProcInstance->pInstrument );
	}
	else
	{
		if (pProcInstance->pTarget)
		{
			// there is a target, use it
			pTargetLocationSource = UnitGetUltimateOwner( pProcInstance->pTarget );
		}
		else
		{
			// no target, our "target location" will be the owner of the unit with the proc
			pTargetLocationSource = UnitGetUltimateOwner( pProcInstance->pInstrument );
		}

	}

	return pTargetLocationSource;

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sExecuteProc(
	PROC_INSTANCE *pProcInstance)
{
	ASSERT_RETURN(pProcInstance);

	const PROC_DEFINITION *pProcDefinition = ProcGetDefinition( pProcInstance->pGame, pProcInstance->nProc );	
	ASSERT_RETURN(pProcDefinition);

	// print execute
	if (gbProcTrace)
	{
		ConsoleString( 
			CONSOLE_SYSTEM_COLOR, 
			L"(%d) '%S' executing proc '%S'", 
			GameGetTick( pProcInstance->pGame ),
			UnitGetDevName( pProcInstance->pInstrument ),
			pProcDefinition->szName);
	}

	// get target location source
	UNIT *pTargetLocationSource = sProcGetTargetOwner( pProcInstance );
	
	// get the target of the proc skill
	UNITID idTarget = INVALID_ID;
	if (pProcInstance->pTarget)
	{
		// there is a target, use it
		UNIT *pTargetOwner = UnitGetUltimateOwner( pProcInstance->pTarget );
		idTarget = UnitGetId( pTargetOwner );
	}

	// setup target location of skill to execute	
	VECTOR vTargetLocation;	
	if (pTargetLocationSource != NULL)
	{

		// set target location
		vTargetLocation = UnitGetPosition( pTargetLocationSource );	

		// offset to center of target location if required			
		if (pProcDefinition->bVerticalCenter)
		{
			vTargetLocation.fZ += (UnitGetCollisionHeight( pTargetLocationSource ) / 2.0f);		
		}

	}

	// who is the source
	UNIT *pSource = pProcInstance->pAttacker;

	// save the applied weapon
	UNIT * pOldEquippedWeapons[ MAX_WEAPONS_PER_UNIT ];
	ZeroMemory( pOldEquippedWeapons, sizeof( UNIT * ) * MAX_WEAPONS_PER_UNIT );	

	// apply the instrument with the proc as the weapon (this will also remove any
	// applied weapon currently set in the inventory, which we want to do because if
	// we have a firebolter equipped, this proc should only be doing damage from itself
	// and not from the firebolter
	UNIT *pInstrumentToApply[ MAX_WEAPONS_PER_UNIT ];
	ZeroMemory( pInstrumentToApply, sizeof(UNIT *) * MAX_WEAPONS_PER_UNIT );
	if (s_sDoesProcApplyWeapon( pProcInstance ))
	{

		// what is the instrument to apply
		pInstrumentToApply[ 0 ] = pProcInstance->pInstrument;

		// save the old weapon
		UnitInventoryGetEquippedWeapons( pSource, pOldEquippedWeapons );	

		// apply instrument as weapon
		UnitInventorySetEquippedWeapons( pSource, pInstrumentToApply );

	}

	// start any skills for this proc	
	for (int i = 0; i < MAX_PROC_SKILLS; ++i)
	{
		const PROC_SKILL *pProcSkill = &pProcDefinition->tProcSkills[ i ];

		if (pProcSkill->nSkill != INVALID_LINK)
		{
			// start skill now or setup timer for delayed start
			if (pProcDefinition->flDelayedProcTimeInSeconds == 0.0f)
			{
				STATS * pStats = StatsListInit(pProcInstance->pGame);
				if(pStats)
				{
					StatsSetStat(pProcInstance->pGame, pStats, STATS_DAMAGE_INCREMENT_PROC, pProcInstance->nDamageIncrement);
					StatsListAdd(pSource, pStats, FALSE);
				}
				SkillStartRequest(
					pProcInstance->pGame,
					pSource,
					pProcSkill->nSkill,
					idTarget,
					vTargetLocation,
					PROC_SKILL_START_FLAGS,
					SkillGetNextSkillSeed( pProcInstance->pGame ),
					pInstrumentToApply[ 0 ] ? UnitGetStat(pInstrumentToApply[ 0 ], STATS_LEVEL) : 0);
				if(pStats)
				{
					StatsListRemove( pProcInstance->pGame, pStats );

					StatsListFree( pProcInstance->pGame, pStats );
				}
			}
			else
			{

				// allocate data to execute the skill
				DELAYED_PROC *pDelayedProc = (DELAYED_PROC *)GMALLOCZ( pProcInstance->pGame, sizeof( DELAYED_PROC ) );
				ASSERT_RETURN(pDelayedProc);
				pDelayedProc->idInstrumentToApply = UnitGetId( pInstrumentToApply[ 0 ] );
				pDelayedProc->idTarget = idTarget;
				pDelayedProc->nSkill = pProcSkill->nSkill;
				pDelayedProc->vTargetLocation = vTargetLocation;
				pDelayedProc->nDamageIncrement = pProcInstance->nDamageIncrement;

				// setup event data
				EVENT_DATA tEventData;
				tEventData.m_Data1 = (DWORD_PTR)pDelayedProc;

				// what is the delay time
				DWORD dwDelayTimeInTicks = (int)(pProcDefinition->flDelayedProcTimeInSeconds * GAME_TICKS_PER_SECOND);

				// register 
				UnitRegisterEventTimed( pSource, sDoDelayedProcSkill, &tEventData, dwDelayTimeInTicks );
			}
		}
	}

	// because procs don't fire very often (like normal weapons do), I think it's better
	// to reapply the original weapon back to the inventory so that there is no danger of
	// anything else in the code accidentally using the proc stats that are applied
	if (s_sDoesProcApplyWeapon( pProcInstance ))
	{
		UnitInventorySetEquippedWeapons( pSource, pOldEquippedWeapons );
	}

	// save that this source has executed a proc on this current tick
	DWORD dwCurrentTick = GameGetTick( pProcInstance->pGame );
	UnitSetStat( pSource, STATS_PROC_LAST_EXECUTED_TICK, dwCurrentTick );

	// save the tick this proc was last executed on
	pProcInstance->dwLastExecutionTick = dwCurrentTick;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sProcCanExecute(
	PROC_INSTANCE *pProcInstance)
{
	ASSERTX_RETFALSE(pProcInstance, "Expected Proc Instance");
	UNIT *pSource = pProcInstance->pAttacker;

	// to keep it simple, don't allow a player to execute a lot of procs on the same tick.  There is
	// no reason why we couldn't, but I think it's too busy when they all stack simultaneously on
	// each other
	DWORD dwCurrentTick = GameGetTick( pProcInstance->pGame );
	DWORD dwProcLastExecutedTick = UnitGetStat( pSource, STATS_PROC_LAST_EXECUTED_TICK );
	if (dwProcLastExecutedTick == dwCurrentTick)
	{
		return FALSE;
	}

	if(s_sDoesProcApplyWeapon( pProcInstance ) && pProcInstance->pInstrument && !ItemIsEquipped(pProcInstance->pInstrument, pProcInstance->pAttacker))
		return FALSE;

	return TRUE;  // ok to execute

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTryProcStart(
	PROC_INSTANCE *pProcInstance)
{
	ASSERT_RETURN(pProcInstance);

	const PROC_DEFINITION *pProcDefinition = ProcGetDefinition( pProcInstance->pGame, pProcInstance->nProc );
	ASSERT_RETURN(pProcDefinition);

	// lets only do this on the server, we can certainly do it on the client if want, but that 
	// potential place for hackers to exploit
	if (IS_SERVER( pProcInstance->pGame ) == FALSE)
	{
		return;
	}

	// procs will not execute for targets who's owners are inanimate (boxes)
	UNIT *pTargetOwner = sProcGetTargetOwner( pProcInstance );
	if (pTargetOwner && UnitIsA( pTargetOwner, UNITTYPE_CREATURE ) == FALSE)
	{
		return;
	}

	// procs can only run once per tick, we do this because, for instance, on damage procs can get
	// called multiple times from one single attack, like when a missile hits a target, and the explosion
	// of the missile as two separate attack events.  Not to mention that the proc itself will likely 
	// cause more damage
	DWORD dwCurrentTick = GameGetTick( pProcInstance->pGame );
	if (dwCurrentTick == pProcInstance->dwLastRunAttemptTick)
	{
		return;
	}

	// enough time must have passed for the cooldown to execute again
	DWORD dwCooldownInTicks = (int)(GAME_TICKS_PER_SECOND_FLOAT * pProcDefinition->flCooldownInSeconds);
	if (dwCurrentTick - pProcInstance->dwLastExecutionTick < dwCooldownInTicks)
	{
		return;
	}

	// the proc tried to run on this tick
	pProcInstance->dwLastRunAttemptTick = dwCurrentTick;

	// what is our chance to execute
	DWORD dwChance = pProcInstance->dwChance;

	// We no longer factor damage increment into the chance a proc fires.
	// The damage increment of the combat is instead included as the damage increment of the proc

	// do debug development overrides
#if ISVERSION(CHEATS_ENABLED)
	if (sgnProbabilityOverride != 0)
	{
		dwChance = sgnProbabilityOverride;
	}
#endif

	DWORD dwRoll = RandGetNum( pProcInstance->pGame, 0, 100 );

	// print attempt
	if (gbProcTrace)
	{
		ConsoleString( 
			CONSOLE_SYSTEM_COLOR, 
			L"(%d) '%S' attempt proc '%S', chance = %d%%, roll = %d%%", 
			GameGetTick( pProcInstance->pGame ),
			UnitGetDevName( pProcInstance->pInstrument ),
			pProcDefinition->szName,
			dwChance,
			dwRoll);
	}

	// roll for execute
	if (dwRoll <= dwChance)
	{
		if (sProcCanExecute( pProcInstance ))
		{
			sExecuteProc( pProcInstance );
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ProcOnAttack(
	GAME *pGame,
	UNIT *pInstrument,
	UNIT *pAttacker,
	EVENT_DATA *pEventData,
	void * /*pData*/,
	DWORD /*dwId */)
{

	// ignore attack events with no instruments (ie, things to attack with), this
	// happens when a monster holding a weapon with a proc executes an aggressive skill
	// which will trigger the on attack event even when there is no weapon associated
	// with the skill.  We may want to one day change the behavior of these kind
	// of procs so that we can put them directly on mosters and have them work
	// with any attack they do.  In that case, we'll prolly just make another
	// type of proc that deals with attack anytime as opposed to with weapon or something -Colin
	if (pInstrument == NULL)
	{
		return;
	}

	// setup start struct
	PROC_INSTANCE tProcInstance(
		PDT_ON_ATTACK, 
		pAttacker, 
		pInstrument, 
		pAttacker, 
		pEventData,
		DAMAGE_INCREMENT_FULL);
	
	// try to start any procs available
	sTryProcStart( &tProcInstance );

}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ProcOnDidKill(
	GAME *pGame,
	UNIT *pInstrument,
	UNIT *pTarget,
	EVENT_DATA *pEventData,
	void * /*pData*/,
	DWORD /*dwId */)
{
	UNIT *pAttacker = UnitGetUltimateOwner( pInstrument );
	
	// setup start struct
	PROC_INSTANCE tProcInstance(
		PDT_ON_DID_KILL, 
		pAttacker, 
		pInstrument, 
		pTarget, 
		pEventData,
		DAMAGE_INCREMENT_FULL);

	// try to start any procs available
	sTryProcStart( &tProcInstance );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ProcOnDidHit(
	GAME *pGame,
	UNIT *pInstrument,
	UNIT *pTarget,
	EVENT_DATA *pEventData,
	void *pData,
	DWORD /*dwId */)
{
	ASSERTX_RETURN( pData, "Expected event data param" );
	const struct COMBAT *pCombat = (const struct COMBAT *)pData;
	
	// get attacker
	UNIT *pAttacker = UnitGetUltimateOwner( pInstrument );

	// fill out proc start struct
	PROC_INSTANCE tProcInstance( 
		PDT_ON_DID_HIT, 
		pAttacker, 
		pInstrument, 
		pTarget, 
		pEventData, 
		s_CombatGetDamageIncrement(pCombat));

	// try to start any procs available
	sTryProcStart( &tProcInstance );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ProcOnGotHit(
	GAME *pGame,
	UNIT *pInstrument,
	UNIT *pTarget,
	EVENT_DATA *pEventData,
	void *pData,
	DWORD /*dwId */)
{
	ASSERTX_RETURN( pData, "Expected event data param" );
	const struct COMBAT *pCombat = (const struct COMBAT *)pData;

	// get attacker
	UNIT *pAttacker = UnitGetUltimateOwner( pInstrument );

	// fill out proc start struct
	PROC_INSTANCE tProcInstance( 
		PDT_ON_GOT_HIT, 
		pAttacker, 
		pInstrument, 
		pTarget, 
		pEventData,
		s_CombatGetDamageIncrement(pCombat));

	// try to start any procs available
	sTryProcStart( &tProcInstance );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ProcOnGotKilled(
	GAME *pGame,
	UNIT *pInstrument,
	UNIT *pTarget,
	EVENT_DATA *pEventData,
	void * /*pData*/,
	DWORD /*dwId */)
{
	UNIT *pAttacker = UnitGetUltimateOwner( pInstrument );

	// fill out proc start struct
	PROC_INSTANCE tProcInstance( 
		PDT_ON_GOT_KILLED, 
		pAttacker, 
		pInstrument, 
		pTarget, 
		pEventData,
		DAMAGE_INCREMENT_FULL);

	// try to start any procs available
	sTryProcStart( &tProcInstance );

}
#endif // #if !ISVERSION(CLIENT_ONLY_VERSION)
