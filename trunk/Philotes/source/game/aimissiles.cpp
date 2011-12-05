//----------------------------------------------------------------------------
// aimissiles.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "game.h"
#include "ai.h"
#include "units.h"
#include "gameunits.h"
#include "ai_priv.h"
#include "unit_priv.h"
#include "missiles.h"
#include "movement.h"
#include "skills.h"
#include "combat.h"
#ifdef HAVOK_ENABLED
#include "havok.h"
#endif
#include "room_path.h"


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define RANDOM_MOVEMENT_ROTATION_MIN		PI / 36.0f
#define RANDOM_MOVEMENT_ROTATION_DELTA		PI / 12.0f


#define RANDOM_MOVEMENT_ROTATION_MIN_TUGBOAT		PI / 20.0f
#define RANDOM_MOVEMENT_ROTATION_DELTA_TUGBOAT		PI / 6.0f

#define RANDOM_MOVEMENT_ACCURACY			75
#define CHARGED_BOLT_PITCH_MIN			PI / 90.0f
#define CHARGED_BOLT_PITCH_DELTA		PI / 36.0f
#define CHARGED_BOLT_ROTATION_MIN		PI / 36.0f
#define CHARGED_BOLT_ROTATION_DELTA		PI / 12.0f
#define CHARGED_BOLT_MAX_BREAKS			3

#define CHARGED_BOLT_DIRECTED_NUM_BREAKS	(CHARGED_BOLT_MAX_BREAKS)
#define CHARGED_BOLT_NOVA_NUM_BREAKS		(1)



static const float sgpfBoltBreakDistances[ CHARGED_BOLT_MAX_BREAKS ] = {
	1.0f,
	2.0f,
	3.0f
};
static const float sgpfBoltNovaBreakDistances[ CHARGED_BOLT_MAX_BREAKS ] = {
	0.5f,
	1.0f,
	1.5f
};
static const int sgpnBoltAccuracy[ CHARGED_BOLT_MAX_BREAKS + 1 ] = {
	100,
	60,
	40,
	0
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sDoChargedBolt(
	GAME * pGame,
	UNIT * pUnit,
	UNIT * pUnitTarget,
	int nAiIndex,
	const float fBoltBreakDistances[ CHARGED_BOLT_MAX_BREAKS ])
{
	RAND tRand;
	RandInit( tRand, UnitGetStat( pUnit, STATS_SKILL_SEED ) );

	int nAccuracy = 75;
	int nBreaks = UnitGetStat( pUnit, STATS_CHARGED_BOLT_BREAKS );
	ASSERT( nBreaks >= 0 );
	nAccuracy -= sgpnBoltAccuracy[ MAX( 0, CHARGED_BOLT_MAX_BREAKS - nBreaks ) ];
	if ( nAccuracy < 0 )
		nAccuracy = 0;

	VECTOR vMoveDirection = UnitGetMoveDirection( pUnit );

	int nPitchRange = PrimeFloat2Int(CHARGED_BOLT_PITCH_DELTA *	   nAccuracy + 100 * CHARGED_BOLT_PITCH_MIN);
	int nAngleRange = PrimeFloat2Int(CHARGED_BOLT_ROTATION_DELTA * nAccuracy + 100 * CHARGED_BOLT_ROTATION_MIN);
	int nTemp = RandGetNum( tRand, 2 * nPitchRange ) - nPitchRange;
	float fPitchDelta = nTemp / 100.0f;
	nTemp = RandGetNum( tRand, 2 * nAngleRange ) - nAngleRange;
	float fAngleDelta = nTemp / 100.0f;

	MATRIX mRotation;
	MatrixTransform( &mRotation, NULL, fAngleDelta, fPitchDelta );
	MatrixMultiplyNormal( &vMoveDirection, &vMoveDirection, &mRotation );

	UnitSetMoveDirection( pUnit, vMoveDirection );

	DWORD dwMissileTag;
	UnitGetStatAny(pUnit, STATS_MISSILE_TAG, &dwMissileTag);

	if ( nBreaks > 0 )
	{
			
		int nMissileClass = UnitGetClass(pUnit);
		UNITID idSource = UnitGetOwnerId( pUnit );
		UNIT * pSource = idSource != INVALID_ID ? UnitGetById( pGame, idSource ) : NULL;		
		float fMaxRange = UnitGetStatFloat( pUnit, STATS_MISSILE_DISTANCE, 1 );

		float fMissileDistance = UnitGetStatFloat( pUnit, STATS_MISSILE_DISTANCE );

		if ( fMaxRange - fMissileDistance > fBoltBreakDistances[ MAX( 0, CHARGED_BOLT_MAX_BREAKS - nBreaks - 1) ] )
		{
			VECTOR vPosition = UnitGetPosition(pUnit);
			DWORD dwNewSeed = RandGetNum( tRand );
			int nSkill = UnitGetStat( pUnit, STATS_AI_SKILL_PRIMARY );
			UnitSetStatVector( pUnit, STATS_SKILL_TARGET_X, nSkill, 0, vPosition );
			UNIT * pMissileChild = MissileFire( 
				pGame, 
				nMissileClass, 
				pUnit, 
				NULL, 
				nSkill, 
				NULL, 
				VECTOR(0),
				vPosition, 
				vMoveDirection, 
				dwNewSeed, 
				dwMissileTag);
				
			UnitSetOwner( pMissileChild, pSource );
			UnitSetStatFloat( pMissileChild, STATS_MISSILE_DISTANCE, 0, fMissileDistance );
			UnitSetStatFloat( pMissileChild, STATS_MISSILE_DISTANCE, 1, fMaxRange );
			UnitSetStat( pUnit,			STATS_CHARGED_BOLT_BREAKS, nBreaks - 1 );
			UnitSetStat( pMissileChild, STATS_CHARGED_BOLT_BREAKS, nBreaks - 1 );
			// pass on the skill for targeting
		}
	}

	UnitSetStat( pUnit, STATS_SKILL_SEED, 0, RandGetNum( tRand ) );

	return TRUE;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AI_ChargedBolt(
	GAME * pGame,
	UNIT * pUnit,
	UNIT * pUnitTarget,
	int nAiIndex)
{
	return sDoChargedBolt( 
		pGame, 
		pUnit, 
		pUnitTarget, 
		nAiIndex, 
		sgpfBoltBreakDistances );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AI_ChargedBoltNova(
	GAME * pGame,
	UNIT * pUnit,
	UNIT * pUnitTarget,
	int nAiIndex)
{
	return sDoChargedBolt( 
		pGame, 
		pUnit, 
		pUnitTarget, 
		nAiIndex,
		sgpfBoltNovaBreakDistances );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AI_RandomMovement(
	GAME * pGame,
	UNIT * pUnit,
	UNIT * pUnitTarget,
	int nAiIndex )
{
	RAND tRand;
	RandInit( tRand, UnitGetStat( pUnit, STATS_SKILL_SEED ) );

	VECTOR vMoveDirection = UnitGetMoveDirection( pUnit );
	//NOTE - the Homing Turn Angle in Radians needs to probably be a new variable. But for testing purposes we'll leave it in.

	int nAngleRange(0);
	if( AppIsHellgate() )
	{
		nAngleRange = PrimeFloat2Int(RANDOM_MOVEMENT_ROTATION_DELTA * pUnit->pUnitData->flHomingTurnAngleInRadians + 100 * RANDOM_MOVEMENT_ROTATION_MIN);
	}
	else
	{
		nAngleRange = PrimeFloat2Int(RANDOM_MOVEMENT_ROTATION_DELTA_TUGBOAT * pUnit->pUnitData->flHomingTurnAngleInRadians + 100 * RANDOM_MOVEMENT_ROTATION_MIN_TUGBOAT);
	}	
	int nTemp = (int)(( RandGetFloat( tRand ) * ( 2.0f * nAngleRange ) ) - nAngleRange);
	float fAngleDelta = nTemp / 100.0f;

	MATRIX mRotation;	
	MatrixRotationZ( mRotation, fAngleDelta );
	//MatrixTransform( &mRotation, NULL, fAngleDelta, 0.0f );
	MatrixMultiplyNormal( &vMoveDirection, &vMoveDirection, &mRotation );

	UnitSetMoveDirection( pUnit, vMoveDirection );

	DWORD dwMissileTag;
	UnitGetStatAny(pUnit, STATS_MISSILE_TAG, &dwMissileTag);	
	UnitSetStat( pUnit, STATS_SKILL_SEED, 0, RandGetNum( tRand ) );
	return TRUE;
}

//----------------------------------------------------------------------------
// this is for the peacemaker
//----------------------------------------------------------------------------
BOOL AI_Peacemaker(
	GAME * pGame,
	UNIT * pUnit,
	UNIT * pUnitTarget,
	int nAiIndex)
{
	pUnitTarget = UnitGetAITarget(pUnit);
	if ( !pUnitTarget )
		return TRUE;

	VECTOR vTarget = UnitGetStatVector( pUnit, STATS_AI_TARGET_X, 0 );
	if ( ! VectorIsZero( vTarget ) )
	{
		vTarget.fZ += UnitGetCollisionHeight(pUnitTarget) / 2.0f;
		BOOL bReachedTargetPos = FALSE;
		if ( VectorDistanceSquared( vTarget, UnitGetPosition( pUnit ) ) < 0.4f )
			bReachedTargetPos = TRUE;
		if ( ! bReachedTargetPos )
		{
			VECTOR vTargetDir = vTarget - UnitGetPosition( pUnit );
			VectorNormalize( vTargetDir, vTargetDir );
			VECTOR vMoveDir = UnitGetMoveDirection( pUnit );
			if ( VectorDot( vTargetDir, vMoveDir ) < 0.5 )
				bReachedTargetPos = TRUE;
		}
		if ( bReachedTargetPos )
		{
			UnitSetStatVector( pUnit, STATS_AI_TARGET_X, 0, VECTOR(0) );
			vTarget = UnitGetAimPoint( pUnitTarget );
		} 
	} else {
		vTarget = UnitGetAimPoint( pUnitTarget );
	}

	if ( IsUnitDeadOrDying( pUnitTarget ) )
	{// move the target to the floor where the guy is standing... that way it will blow up
		vTarget = UnitGetPosition( pUnitTarget );
	}

	VECTOR vDelta = vTarget - UnitGetPosition( pUnit );
	VectorNormalize( vDelta, vDelta );
	vDelta = VectorLerp(UnitGetMoveDirection(pUnit), vDelta, 1.0f-pUnit->pUnitData->flHomingTurnAngleInRadians);
	UnitSetMoveDirection( pUnit, vDelta );
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AI_AssassinBugHiveInit(
	GAME* pGame,
	UNIT* pUnit,
	int nAiIndex)
{
	ASSERT_RETURN(pGame && pUnit);
	UnitSetStat( pUnit, STATS_AI_AWAKE_RANGE, 0, 10 );
	UnitSetStat( pUnit, STATS_AI_PARAMETERS, UnitGetStat(pUnit, STATS_AI_TIMER));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sAI_Hive(
	GAME * pGame,
	UNIT * pUnit,
	UNIT * pUnitTarget,
	int nAiIndex)
{
	pUnitTarget = UnitGetAITarget(pUnit);

	int nSkill = UnitGetStat( pUnit, STATS_AI_SKILL_PRIMARY );
	ASSERT_RETFALSE( nSkill != INVALID_ID );

	if ( pUnitTarget && !SkillIsValidTarget( pGame, pUnit, pUnitTarget, NULL, nSkill, TRUE ) )
		pUnitTarget = NULL;

	if ( !pUnitTarget )
	{
		DWORD pdwFlags[ NUM_TARGET_QUERY_FILTER_FLAG_DWORDS ];
		ZeroMemory( pdwFlags, sizeof( DWORD ) * NUM_TARGET_QUERY_FILTER_FLAG_DWORDS );
		SETBIT( pdwFlags, SKILL_TARGETING_BIT_FIRSTFOUND );
		DWORD dwAIGetTargetFlags = 0;
		pUnitTarget = AI_GetTarget( pGame, pUnit, nSkill, pdwFlags, dwAIGetTargetFlags );
		UnitSetAITarget(pUnit, pUnitTarget);
	}

	if(!pUnitTarget && !UnitOnStepList(pGame, pUnit))
	{
		UnitStepListAdd( pGame, pUnit );
	}

	int nStateOld = UnitGetStat( pUnit, STATS_AI_STATE, nAiIndex );
	int nStateNew;

	VECTOR vDelta = pUnitTarget ? UnitGetAimPoint( pUnitTarget ) - UnitGetPosition( pUnit ) : VECTOR(0);
	float fDistance = VectorLength( vDelta );
	if ( fDistance < 0.25f )
	{
		int nTimer = UnitGetStat(pUnit, STATS_AI_TIMER);
		int nStateToSet = AI_STATE_IDLE;
		if(nTimer > 0 && pUnitTarget)
		{
			nTimer--;
			if(nTimer <= 0)
			{
				nTimer = UnitGetStat(pUnit, STATS_AI_PARAMETERS);
				DWORD pdwFlags[ NUM_TARGET_QUERY_FILTER_FLAG_DWORDS ];
				ZeroMemory( pdwFlags, sizeof( DWORD ) * NUM_TARGET_QUERY_FILTER_FLAG_DWORDS );
				SETBIT( pdwFlags, SKILL_TARGETING_BIT_FIRSTFOUND );
				DWORD dwAIGetTargetFlags = 0;
				SETBIT(dwAIGetTargetFlags, AIGTF_PICK_NEW_TARGET_BIT);
				pUnitTarget = AI_GetTarget( pGame, pUnit, nSkill, pdwFlags, dwAIGetTargetFlags );
				UnitSetAITarget(pUnit, pUnitTarget);
				nStateToSet = AI_STATE_CHARGE;
				UnitStepListAdd( pGame, pUnit );
			}
			UnitSetStat(pUnit, STATS_AI_TIMER, nTimer);
		}
		if ( nStateOld == AI_STATE_CHARGE )
		{
			UnitStepListRemove( pGame, pUnit, SLRF_FORCE );
		}
		nStateNew = nStateToSet;
	} else {
		VECTOR vDirection;
		VectorScale( vDirection, vDelta, 1.0f/fDistance );
		VECTOR vMoveDirection = VectorLerp( UnitGetMoveDirection( pUnit ), vDirection, 0.05f );
		UnitSetMoveDirection( pUnit, vMoveDirection );
		UnitSetFaceDirection( pUnit, vMoveDirection );
		if ( nStateOld == AI_STATE_IDLE )
		{
			UnitStepListAdd( pGame, pUnit );
		}
		nStateNew = AI_STATE_CHARGE;
	}

	UnitSetStat( pUnit, STATS_AI_STATE, nAiIndex, nStateNew );

	ASSERT_RETFALSE(UnitGetGenus(pUnit) == GENUS_MISSILE);

#if !ISVERSION(CLIENT_ONLY_VERSION)
	if ( IS_SERVER( pUnit ) )
	{
		int nClass = UnitGetClass(pUnit);
		const UNIT_DATA * missile_data = MissileGetData(pGame, nClass);
		ASSERT_RETFALSE(missile_data);

#ifdef HAVOK_ENABLED
		HAVOK_IMPACT tImpact;
		tImpact.dwFlags = 0;
		tImpact.fForce = missile_data->fForce;
		tImpact.vSource = UnitGetPosition(pUnit);
#endif
		s_UnitAttackLocation(
			pUnit, 
			NULL, 
			0, 
			UnitGetRoom(pUnit), 
			UnitGetPosition(pUnit), 
#ifdef HAVOK_ENABLED
			tImpact, 
#endif
			0);

	}
#endif // !ISVERSION(CLIENT_ONLY_VERSION)

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AI_AssassinBugHive(
	GAME * pGame,
	UNIT * pUnit,
	UNIT * pUnitTarget,
	int nAiIndex)
{
	return sAI_Hive(pGame, pUnit, pUnitTarget, nAiIndex);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AI_ScarabBallInit(
	GAME* pGame,
	UNIT* pUnit,
	int nAiIndex)
{
	ASSERT_RETURN(pGame && pUnit);
	UnitSetStat( pUnit, STATS_AI_AWAKE_RANGE, 0, 10 );
	UnitSetStat( pUnit, STATS_AI_STATE, nAiIndex, AI_STATE_IDLE );
	UnitSetFlag( pUnit, UNITFLAG_ROOMCOLLISION );

	VECTOR vMoveDirection = UnitGetMoveDirection(pUnit);
	vMoveDirection.fZ = 0.5f;
	VectorNormalize(vMoveDirection);
	UnitSetMoveDirection(pUnit, vMoveDirection);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AI_ScarabBall(
	GAME * pGame,
	UNIT * pUnit,
	UNIT * pUnitTarget,
	int nAiIndex)
{
	pUnitTarget = UnitGetAITarget(pUnit);

	int nSkill = UnitGetStat( pUnit, STATS_AI_SKILL_PRIMARY );
	ASSERT_RETFALSE( nSkill != INVALID_ID );

	if ( pUnitTarget && !SkillIsValidTarget( pGame, pUnit, pUnitTarget, NULL, nSkill, TRUE ) )
		pUnitTarget = NULL;

	if ( !pUnitTarget )
	{
		DWORD pdwFlags[ NUM_TARGET_QUERY_FILTER_FLAG_DWORDS ];
		ZeroMemory( pdwFlags, sizeof( DWORD ) * NUM_TARGET_QUERY_FILTER_FLAG_DWORDS );
		SETBIT( pdwFlags, SKILL_TARGETING_BIT_FIRSTFOUND );
		DWORD dwAIGetTargetFlags = 0;
		pUnitTarget = AI_GetTarget( pGame, pUnit, nSkill, pdwFlags, dwAIGetTargetFlags );
		UnitSetAITarget(pUnit, pUnitTarget);
	}

	int nStateOld = UnitGetStat( pUnit, STATS_AI_STATE, nAiIndex );
	int nStateNew = nStateOld;

	switch(nStateOld)
	{
	case AI_STATE_IDLE:
		{
			if(pUnitTarget)
			{
				nStateNew = AI_STATE_CHARGE;
			}
		}
		break;
	case AI_STATE_CHARGE:
		{
			VECTOR vDelta = pUnitTarget ? UnitGetPosition( pUnitTarget ) - UnitGetPosition( pUnit ) : VECTOR(0);
			float fDistance = VectorLength( vDelta );

			if(fDistance < 0.25f)
			{
				UnitSetStat(pUnit, STATS_AI_CIRCLE_DIRECTION, 0);
				UnitClearFlag( pUnit, UNITFLAG_ROOMCOLLISION );
				nStateNew = AI_STATE_JUMP;
			}
			else
			{
				VECTOR vDirection;
				VectorScale( vDirection, vDelta, 1.0f/fDistance );
				VECTOR vMoveDirection = VectorLerp( UnitGetMoveDirection( pUnit ), vDirection, 0.05f );
				UnitSetMoveDirection( pUnit, vMoveDirection );
			}
		}
		break;
	case AI_STATE_JUMP:
		{
			VECTOR vDirection;
			int nGoingDown = UnitGetStat(pUnit, STATS_AI_CIRCLE_DIRECTION);
			if(nGoingDown == 0)
			{
				if(pUnitTarget)
				{
					VECTOR vTop = UnitGetPosition(pUnitTarget);
					vTop.fZ += UnitGetCollisionHeight(pUnitTarget);
					vDirection = vTop - UnitGetPosition(pUnit);
				}
				else
				{
					vDirection = VECTOR(0, 0, 1);
				}

				if(vDirection.fZ <= 0.1f)
				{
					UnitSetStat(pUnit, STATS_AI_CIRCLE_DIRECTION, 1);
				}
			}
			else
			{
				vDirection = pUnitTarget ? UnitGetPosition(pUnitTarget) - UnitGetPosition(pUnit) : VECTOR(0, 0, -1);
				if(vDirection.fZ >= -0.1f)
				{
					UnitSetStat(pUnit, STATS_AI_CIRCLE_DIRECTION, 0);
				}
			}
			VectorNormalize(vDirection);
			UnitSetMoveDirection(pUnit, vDirection);

#if !ISVERSION(CLIENT_ONLY_VERSION)
			if(pUnitTarget && IS_SERVER(pGame))
			{
				ASSERT_RETFALSE(UnitGetGenus(pUnit) == GENUS_MISSILE);

				int nClass = UnitGetClass(pUnit);
				const UNIT_DATA * missile_data = MissileGetData(pGame, nClass);
				ASSERT_RETFALSE(missile_data);

#ifdef HAVOK_ENABLED
				HAVOK_IMPACT tImpact;
				tImpact.dwFlags = 0;
				tImpact.fForce = missile_data->fForce;
				tImpact.vSource = UnitGetPosition(pUnit);
#endif
				DWORD dwFlags = 0;
				SETBIT(dwFlags, UAU_DIRECT_MISSILE_BIT);
				s_UnitAttackUnit(pUnit, 
							pUnitTarget, 
							NULL, 
							0, 
#ifdef HAVOK_ENABLED
							&tImpact,
#endif
							dwFlags);
			}
#endif // !ISVERSION(CLIENT_ONLY_VERSION)
		}
		break;
	default:
		{
			UnitSetFlag( pUnit, UNITFLAG_ROOMCOLLISION );
			nStateNew = AI_STATE_IDLE;
		}
		break;
	}

	if(!pUnitTarget || IsUnitDeadOrDying(pUnitTarget))
	{
		UnitSetFlag( pUnit, UNITFLAG_ROOMCOLLISION );
		nStateNew = AI_STATE_IDLE;
	}

	UnitSetStat( pUnit, STATS_AI_STATE, nAiIndex, nStateNew );

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AI_FirebrandPod(
	GAME * pGame,
	UNIT * pUnit,
	UNIT * pUnitTarget,
	int nAiIndex)
{
	VECTOR vTarget = UnitGetStatVector( pUnit, STATS_AI_TARGET_X, 0 );
	ASSERT_RETFALSE( ! VectorIsZero( vTarget ) );

	VECTOR vDeltaToTarget = vTarget - UnitGetPosition( pUnit );
	VECTOR vDirection;
	VectorNormalize( vDirection, vDeltaToTarget );
	VECTOR vMoveDirection = UnitGetMoveDirection( pUnit );

	if ( VectorDot( vMoveDirection, vDirection ) > 0 )
	{// turn towards the target
		float fAccuracy = MAX( 0.0f, 1.0f - pUnit->fVelocity / 60.0f );
		vMoveDirection = VectorLerp( vMoveDirection, vDirection, fAccuracy );		
		VectorNormalize( vMoveDirection );
	}

	// add some randomness
	VECTOR vRandom = RandGetVectorEx( pGame );
	vMoveDirection = VectorLerp( vMoveDirection, vRandom, 0.95f );		
	VectorNormalize( vMoveDirection );

	UnitSetMoveDirection( pUnit, vMoveDirection );

	pUnit->fAcceleration *= 2;

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AI_HandGrappler(
	GAME * pGame,
	UNIT * pUnit,
	UNIT * pUnitTarget,
	int nAiIndex)
{
	pUnitTarget = UnitGetAITarget(pUnit);

	int nSkill = UnitGetStat( pUnit, STATS_AI_SKILL_PRIMARY );
	ASSERT_RETFALSE( nSkill != INVALID_ID );

	if ( !pUnitTarget || !SkillIsValidTarget( pGame, pUnit, pUnitTarget, NULL, nSkill, TRUE ) )
		return FALSE;

	VECTOR vDeltaToTarget = UnitGetAimPoint( pUnitTarget ) - UnitGetPosition( pUnit );
	VECTOR vDirection;
	VectorNormalize( vDirection, vDeltaToTarget );
	VECTOR vMoveDirection = UnitGetMoveDirection( pUnit );

	if ( VectorDot( vMoveDirection, vDirection ) > 0 )
	{// turn towards the target
		float fAccuracy = MAX( 0.0f, 1.0f - pUnit->fVelocity / 20.0f );
		vMoveDirection = VectorLerp( vMoveDirection, vDirection, fAccuracy );		
		VectorNormalize( vMoveDirection );
	}

	UnitSetMoveDirection( pUnit, vMoveDirection );

	pUnit->fAcceleration *= 2;

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AI_BlessedHammer(
	GAME * pGame,
	UNIT * pUnit,
	UNIT * pUnitTarget,
	int nAiIndex)
{
	int nSkill = UnitGetStat( pUnit, STATS_AI_SKILL_PRIMARY );
	ASSERT_RETFALSE( nSkill != INVALID_ID );

	VECTOR vPerpendicular;

	VECTOR vMoveDirection = UnitGetMoveDirection( pUnit );
	VectorCross(vPerpendicular, vMoveDirection, VECTOR(0, 0, 1));

	float fAccuracy = MAX( 0.0f, 1.0f - pUnit->fVelocity / 50.0f );
	vMoveDirection = VectorLerp( vMoveDirection, vPerpendicular, fAccuracy );		
	VectorNormalize( vMoveDirection );

	UnitSetMoveDirection( pUnit, vMoveDirection );

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UNIT * sHomingGetTarget(
	GAME * pGame,
	UNIT * pUnit,
	UNIT * pTargetPrev,
	int nSkill)
{
	const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkill  );
	ASSERT_RETNULL( pSkillData );

	UNITID pnTargets[ 2 ];
	SKILL_TARGETING_QUERY tTargetingQuery( pSkillData );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_CLOSEST );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_CHECK_LOS );
	tTargetingQuery.pSeekerUnit = pUnit;
	tTargetingQuery.pnUnitIds = pnTargets;
	tTargetingQuery.nUnitIdMax = 2;
	tTargetingQuery.fMaxRange = 15.0f;

	SkillTargetQuery( pGame, tTargetingQuery );

	if ( pTargetPrev )
	{
		for ( int i = 0; i < tTargetingQuery.nUnitIdCount; i++ )
		{
			if ( pnTargets[ i ] != UnitGetId( pTargetPrev ) )
				return UnitGetById( pGame, pnTargets[ i ] );
		}
	} else if ( tTargetingQuery.nUnitIdCount ) {
		return UnitGetById( pGame, pnTargets [ 0 ] );
	}
	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAddRandomnessToMotion(
	UNIT * pUnit,
	VECTOR & vMoveDirection)
{
	// add some randomness
	RAND tRand;
	RandInit( tRand, UnitGetStat( pUnit, STATS_SKILL_SEED ) );

	VECTOR vRandom = RandGetVectorEx( tRand );
	vMoveDirection = VectorLerp( vMoveDirection, vRandom, 0.85f );		
	VectorNormalize( vMoveDirection );

	UnitSetMoveDirection( pUnit, vMoveDirection );

	UnitSetStat( pUnit, STATS_SKILL_SEED, RandGetNum(tRand) );
}

enum
{
	JUMP_STICK_TO_TARGET_BIT,
	JUMP_JUMP_ON_NEW_TARGET_BIT,
	JUMP_JUMP_ON_ATTACK_BIT,
	JUMP_JUMP_ON_NO_TARGET,
	JUMP_RANDOMNESS_BIT,
};

enum
{
	JUMP_BIT_NEW_TARGET,
	JUMP_BIT_EVER_ATTACKED,
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sJumpFromTargetToTarget(
	GAME * pGame,
	UNIT * pUnit,
	UNIT * pUnitTarget,
	int nTicksPerTarget,
	DWORD dwFlags,
	float fLerpXY,
	float fLerpZ )
{
	pUnitTarget = UnitGetAITarget(pUnit);

	int nSkill = UnitGetStat( pUnit, STATS_AI_SKILL_PRIMARY );
	ASSERT_RETFALSE( nSkill != INVALID_ID );

	int nTimeOnTarget = UnitGetStat( pUnit, STATS_AI_TIMER );

	if ( pUnitTarget && !SkillIsValidTarget( pGame, pUnit, pUnitTarget, NULL, nSkill, TRUE ) )
	{
		pUnitTarget = NULL;
		nTimeOnTarget = 0;
	}

	if ( pUnitTarget && 
		nTimeOnTarget &&
		nTicksPerTarget > 0 &&
		! TESTBIT(dwFlags, JUMP_STICK_TO_TARGET_BIT) &&
		GameGetTick( pGame ) - nTimeOnTarget > (UINT) nTicksPerTarget )
	{ // we haven't found a new target fast enough.  Go back to the old one
		pUnitTarget = NULL;
		nTimeOnTarget = 0;
		UnitSetStat( pUnit, STATS_AI_TIMER, 0 );
	}

	VECTOR vDelta = pUnitTarget ? UnitGetAimPoint( pUnitTarget ) - UnitGetPosition( pUnit ) : VECTOR(0);
	VECTOR vDirectionCurr = UnitGetMoveDirection( pUnit );

	BOOL bSwitchTargets = FALSE;
	if ( ! pUnitTarget )
	{
		bSwitchTargets = TRUE;
	}

	UNIT * pUnitToAttack = NULL;
	if ( ! TESTBIT(dwFlags, JUMP_STICK_TO_TARGET_BIT) )
	{
		const UNIT_DATA * pTargetData = pUnitTarget ? UnitGetData( pUnitTarget ) : NULL;
		float fTargetRadius = pTargetData ? UnitGetCollisionRadius( pUnitTarget ) : 0.25f;
		bSwitchTargets = FALSE;
		float fDistance = VectorLengthSquared( vDelta );
		if ( fDistance < fTargetRadius * fTargetRadius )
		{
			bSwitchTargets = TRUE;
			pUnitToAttack = pUnitTarget;
		}
	} 
	else if ( UnitTestFlag( pUnit, UNITFLAG_ATTACHED ) )
	{
		pUnitToAttack = pUnitTarget;
	} 
	else if ( ! pUnitTarget ) 
	{
		bSwitchTargets = TRUE;
	}

	BOOL bMoveTowardsTarget = TRUE;
	if ( bSwitchTargets )
	{
		if ( TESTBIT(dwFlags, JUMP_STICK_TO_TARGET_BIT) )
		{
			UnitDetach( pUnit );
			UnitStatClearBit(pUnit, STATS_AI_FLAGS, 0, JUMP_BIT_NEW_TARGET);
			UnitStepListAdd( pGame, pUnit );
		}

		bMoveTowardsTarget = FALSE;		
		UNIT * pUnitTargetNew = sHomingGetTarget( pGame, pUnit, pUnitTarget, nSkill );
		if ( TESTBIT(dwFlags, JUMP_JUMP_ON_NEW_TARGET_BIT) && pUnitTargetNew )
			vDirectionCurr = VECTOR( 0.0f, 0.0f, 1.0f ); 
		if ( pUnitTargetNew )
		{
			bMoveTowardsTarget = TRUE;
			pUnitTarget = pUnitTargetNew;
			UnitSetAITarget(pUnit, pUnitTarget);
			vDelta = UnitGetAimPoint( pUnitTarget ) - UnitGetPosition( pUnit );
		} else {
			// record the time when we gave up on this target
			UnitSetStat( pUnit, STATS_AI_TIMER, GameGetTick( pGame ) );
		}
	}

	if ( TESTBIT(dwFlags, JUMP_JUMP_ON_ATTACK_BIT) && pUnitToAttack )
		vDirectionCurr = VECTOR( 0.0f, 0.0f, 1.0f ); 

	if ( TESTBIT(dwFlags, JUMP_STICK_TO_TARGET_BIT) && UnitTestFlag( pUnit, UNITFLAG_ATTACHED ) )
		bMoveTowardsTarget = FALSE;

	if ( pUnitTarget && bMoveTowardsTarget )
	{
		VECTOR vTargetDirection;
		VECTOR vDirectionToMove;
		VectorNormalize( vTargetDirection, vDelta );
		if ( ! VectorIsZero( vTargetDirection ) )
		{
			vDirectionToMove.fX = vDirectionCurr.fX * fLerpXY + vTargetDirection.fX * (1.0f - fLerpXY);
			vDirectionToMove.fY = vDirectionCurr.fY * fLerpXY + vTargetDirection.fY * (1.0f - fLerpXY);
			vDirectionToMove.fZ = vDirectionCurr.fZ * fLerpZ  + vTargetDirection.fZ * (1.0f - fLerpZ);
			VectorNormalize( vDirectionToMove );
		} else {
			vDirectionToMove = vDirectionCurr;
		}
		UnitSetMoveDirection( pUnit, vDirectionToMove );
	} 

	if(TESTBIT(dwFlags, JUMP_RANDOMNESS_BIT))
	{
		VECTOR vMoveDirection = UnitGetMoveDirection(pUnit);
		sAddRandomnessToMotion(pUnit, vMoveDirection);
	}

	if ( pUnitToAttack )
	{
		if ( UnitGetStat( pUnit, STATS_VELOCITY_BONUS ) != -100 )
		{
			UnitSetStat( pUnit, STATS_VELOCITY_BONUS, -100 ); // slow all the way down now
			UnitCalcVelocity( pUnit );
		}
#if !ISVERSION(CLIENT_ONLY_VERSION)
		if ( IS_SERVER( pUnit ) )
		{
#ifdef HAVOK_ENABLED
			const UNIT_DATA * pUnitData = UnitGetData( pUnit );
			HAVOK_IMPACT tImpact;
			tImpact.dwFlags = 0;
			tImpact.fForce = pUnitData->fForce;
			tImpact.vSource = UnitGetPosition(pUnit);
#endif
			s_UnitAttackUnit( pUnit, 
							pUnitToAttack,
							NULL, 
							0, 
#ifdef HAVOK_ENABLED
							&tImpact, 
#endif
							0);

		}
#endif // !ISVERSION(CLIENT_ONLY_VERSION)
		else
		{
			if(!UnitStatTestBit(pUnit, STATS_AI_FLAGS, 0, JUMP_BIT_NEW_TARGET))
			{
				VECTOR vPosition = UnitGetPosition( pUnit );
				VECTOR vNormal   = UnitGetMoveDirection( pUnit );
				c_MissileEffectCreate( pGame, pUnit, NULL, UnitGetClass( pUnit ), MISSILE_EFFECT_IMPACT_UNIT, vPosition, vNormal );
				UnitStatSetBit(pUnit, STATS_AI_FLAGS, 0, JUMP_BIT_NEW_TARGET);
				UnitStatSetBit(pUnit, STATS_AI_FLAGS, 0, JUMP_BIT_EVER_ATTACKED);
			}
		}
	}

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define TICKS_PER_ARC_JUMPER_TARGET 20
#define ARC_JUMPER_LERP_XY 0.9f
#define ARC_JUMPER_LERP_Z 0.6f
BOOL AI_ArcJumper(
	GAME * pGame,
	UNIT * pUnit,
	UNIT * pUnitTarget,
	int nAiIndex)
{
	DWORD dwFlags = 0;
	SETBIT(dwFlags, JUMP_JUMP_ON_ATTACK_BIT);
	if( AppIsHellgate() )
	{
		return sJumpFromTargetToTarget( pGame, pUnit, pUnitTarget, TICKS_PER_ARC_JUMPER_TARGET, 
			dwFlags, ARC_JUMPER_LERP_XY, ARC_JUMPER_LERP_Z );
	}
	else
	{
		return sJumpFromTargetToTarget( pGame, pUnit, pUnitTarget, 1, 
			dwFlags, .3f, 0.0f );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSingleTargetHoming(
	GAME * pGame, 
	UNIT * pUnit )
{
	float fLerpXY = (float)UnitGetStat( pUnit, STATS_MISSILE_AI_TURN_LERP_XY ) / 100.0f;
	float fLerpZ = (float)UnitGetStat( pUnit, STATS_MISSILE_AI_TURN_LERP_Z ) / 100.0f;
	UNIT * pUnitTarget = UnitGetAITarget(pUnit);

	int nSkill = UnitGetStat( pUnit, STATS_AI_SKILL_PRIMARY );
	ASSERT_RETFALSE( nSkill != INVALID_ID );

	if ( ! pUnitTarget || !SkillIsValidTarget( pGame, pUnit, pUnitTarget, NULL, nSkill, TRUE ) )
	{
		pUnitTarget = sHomingGetTarget( pGame, pUnit, NULL, nSkill );
		if ( pUnitTarget )
			UnitSetAITarget( pUnit, pUnitTarget, TRUE );
	}

	if ( !pUnitTarget || !SkillIsValidTarget( pGame, pUnit, pUnitTarget, NULL, nSkill, TRUE ) )
		return FALSE;

	VECTOR vDeltaToTarget = UnitGetAimPoint( pUnitTarget ) - UnitGetPosition( pUnit );
	VECTOR vTargetDirection;
	VectorNormalize( vTargetDirection, vDeltaToTarget );

	VECTOR vDirectionCurr = UnitGetMoveDirection( pUnit );
	VECTOR vDirectionToMove;
	if ( ! VectorIsZero( vTargetDirection ) )
	{// turn towards the target
		vDirectionToMove.fX = vDirectionCurr.fX * fLerpXY + vTargetDirection.fX * (1.0f - fLerpXY);
		vDirectionToMove.fY = vDirectionCurr.fY * fLerpXY + vTargetDirection.fY * (1.0f - fLerpXY);
		vDirectionToMove.fZ = vDirectionCurr.fZ * fLerpZ  + vTargetDirection.fZ * (1.0f - fLerpZ);
	} else {
		vDirectionToMove = vDirectionCurr;
	}

	UnitSetMoveDirection( pUnit, vDirectionToMove );

	return TRUE;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AI_EelLauncher(
	GAME * pGame,
	UNIT * pUnit,
	UNIT * pUnitTarget,
	int nAiIndex)
{
	sSingleTargetHoming( pGame, pUnit );

	pUnitTarget = UnitGetAITarget(pUnit);
#if !ISVERSION(CLIENT_ONLY_VERSION)
	if ( pUnitTarget && UnitTestFlag( pUnit, UNITFLAG_ATTACHED )&&  IS_SERVER( pUnit ) )
	{
#ifdef HAVOK_ENABLED
		const UNIT_DATA * pUnitData = UnitGetData( pUnit );
		HAVOK_IMPACT tImpact;
		tImpact.dwFlags = 0;
		tImpact.fForce = pUnitData->fForce;
		tImpact.vSource = UnitGetPosition(pUnit);
#endif
		DWORD dwFlags = 0;
		SETBIT(dwFlags, UAU_DIRECT_MISSILE_BIT);
		s_UnitAttackUnit( pUnit, 
						  pUnitTarget, 
						  NULL, 
						  0, 
#ifdef HAVOK_ENABLED
						  &tImpact,
#endif
						  dwFlags);
	}
#endif
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AI_FireBeetles(
	GAME * pGame,
	UNIT * pUnit,
	UNIT * pUnitTarget,
	int nAiIndex)
{
	sSingleTargetHoming( pGame, pUnit );

	VECTOR vMoveDirection = UnitGetMoveDirection( pUnit );
	sAddRandomnessToMotion(pUnit, vMoveDirection );

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AI_SimpleHoming(
	GAME * pGame,
	UNIT * pUnit,
	UNIT * pUnitTarget,
	int nAiIndex)
{
	sSingleTargetHoming( pGame, pUnit );

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAI_DoThrowSwordRotation(
	GAME * pGame,
	UNIT * pUnit)
{
	if ( IS_CLIENT( pGame ) )
	{// spin sword
		VECTOR vMoveDirection = UnitGetMoveDirection( pUnit );
		VECTOR vSideVector;
		VectorCross( vSideVector, vMoveDirection, VECTOR( 0, 0, 1 ) );

		MATRIX mSpinRotation;
		MatrixRotationAxis( mSpinRotation, vSideVector, -0.8f );

		VECTOR vFaceDirection = UnitGetFaceDirection( pUnit, FALSE );
		VECTOR vUpDirection = UnitGetUpDirection(pUnit);
		MatrixMultiplyNormal( &vFaceDirection, &vFaceDirection, &mSpinRotation );
		MatrixMultiplyNormal( &vUpDirection, &vUpDirection, &mSpinRotation );

		UnitSetUpDirection( pUnit, vUpDirection );
		UnitSetFaceDirection( pUnit, vFaceDirection, FALSE, FALSE, FALSE );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AI_ThrowSword(
	GAME * pGame,
	UNIT * pUnit,
	UNIT * pUnitTarget,
	int nAiIndex)
{
	sAI_DoThrowSwordRotation(pGame, pUnit);
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AI_ThrowSwordHoming(
	GAME * pGame,
	UNIT * pUnit,
	UNIT * pUnitTarget,
	int nAiIndex)
{
	sSingleTargetHoming( pGame, pUnit );

	sAI_DoThrowSwordRotation(pGame, pUnit);
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAI_DoThrowShieldRotation(
	GAME * pGame,
	UNIT * pUnit)
{
	if ( IS_CLIENT( pGame ) )
	{// spin shield
		VECTOR vMoveDirection = UnitGetMoveDirection( pUnit );

		MATRIX mSpinRotation;
		MatrixRotationAxis( mSpinRotation, VECTOR( 0, 0, 1 ), -0.8f );

		VECTOR vFaceDirection = UnitGetFaceDirection( pUnit, FALSE );
		MatrixMultiplyNormal( &vFaceDirection, &vFaceDirection, &mSpinRotation );

		UnitSetFaceDirection( pUnit, vFaceDirection, FALSE, FALSE, FALSE );
		if(vFaceDirection.fX >= 0.0f)
		{
			UnitSetUpDirection( pUnit, VECTOR(0, -1, 0) );
		}
		else
		{
			UnitSetUpDirection( pUnit, VECTOR(0, 1, 0) );
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AI_ThrowShield(
	GAME * pGame,
	UNIT * pUnit,
	UNIT * pUnitTarget,
	int nAiIndex)
{
	sAI_DoThrowShieldRotation(pGame, pUnit);
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AI_ThrowShieldHoming(
	GAME * pGame,
	UNIT * pUnit,
	UNIT * pUnitTarget,
	int nAiIndex)
{
	sSingleTargetHoming( pGame, pUnit );

	sAI_DoThrowShieldRotation(pGame, pUnit);
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sAI_Artillery_FireMissile(
	GAME * pGame, 
	UNIT * pUnit, 
	const EVENT_DATA & tEventData)
{
	VECTOR vMissilePosition;
	ROOM * pFoundRoom = s_LevelGetRandomPositionAround(pGame, UnitGetLevel(pUnit), UnitGetPosition(pUnit), float(UnitGetStat(pUnit, STATS_AI_SIGHT_RANGE)), vMissilePosition);
	if(!pFoundRoom)
	{
		return FALSE;
	}

	RAND tRand;
	RandInit( tRand, UnitGetStat( pUnit, STATS_SKILL_SEED ) );

	const UNIT_DATA * pUnitData = MissileGetData(pGame, pUnit);
	ASSERT_RETFALSE(pUnitData);
	if(pUnitData->nMissileHitUnit < 0)
	{
		return FALSE;
	}

	DWORD dwFlags = 0;
	SETBIT(dwFlags, MF_FROM_MISSILE_BIT);
	MissileFire(pGame, pUnitData->nMissileHitUnit, pUnit, NULL, INVALID_ID, NULL, VECTOR(0), vMissilePosition, VECTOR(0), RandGetNum(tRand), 0, dwFlags, NULL, 0);

	UnitSetStat( pUnit, STATS_SKILL_SEED, 0, RandGetNum( tRand ) );

	UnitRegisterEventTimed(pUnit, sAI_Artillery_FireMissile, EVENT_DATA(), GAME_TICKS_PER_SECOND / 2);
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AI_ArtilleryInit(
	GAME* pGame,
	UNIT* pUnit,
	int nAiIndex)
{
	ASSERT_RETURN(pGame && pUnit);
	UnitRegisterEventTimed(pUnit, sAI_Artillery_FireMissile, EVENT_DATA(), 3 * GAME_TICKS_PER_SECOND);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AI_Artillery(
	GAME * pGame,
	UNIT * pUnit,
	UNIT * pUnitTarget,
	int nAiIndex)
{
	ASSERT_RETFALSE(pGame && pUnit);
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AI_ProximityMine(
	GAME * pGame,
	UNIT * pUnit,
	UNIT * pUnitTarget,
	int nAiIndex)
{
	ASSERT_RETFALSE(pGame && pUnit);

	UNITID pnUnitIds[1];
	SKILL_TARGETING_QUERY tTargetQuery;
	tTargetQuery.pSeekerUnit = pUnit;
	tTargetQuery.fMaxRange = float(UnitGetStat(pUnit, STATS_AI_SIGHT_RANGE));
	tTargetQuery.pnUnitIds = pnUnitIds;
	tTargetQuery.nUnitIdMax = 1;
	SETBIT( tTargetQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_ENEMY );
	SETBIT( tTargetQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_JUST_COUNT );
	tTargetQuery.tFilter.nUnitType = UNITTYPE_CREATURE;
	SkillTargetQuery(pGame, tTargetQuery);

	if(tTargetQuery.nUnitIdCount >= 1)
	{
		int nMissileFuse = UnitGetStat(pUnit, STATS_AI_PARAMETERS);
		nMissileFuse = MAX(1, nMissileFuse);
		MissileSetFuse(pUnit, nMissileFuse);
		
		return FALSE;
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AI_AttackLocation(
	GAME * pGame,
	UNIT * pUnit,
	UNIT * pUnitTarget,
	int nAiIndex)
{
	ASSERT_RETFALSE(UnitGetGenus(pUnit) == GENUS_MISSILE);

#if !ISVERSION(CLIENT_ONLY_VERSION)
	if ( IS_SERVER( pUnit ) )
	{
		int nClass = UnitGetClass(pUnit);
		const UNIT_DATA * missile_data = MissileGetData(pGame, nClass);
		ASSERT_RETFALSE(missile_data);

#ifdef HAVOK_ENABLED
		HAVOK_IMPACT tImpact;
		tImpact.dwFlags = 0;
		tImpact.fForce = missile_data->fForce;
		tImpact.vSource = UnitGetPosition(pUnit);
#endif
		s_UnitAttackLocation(
			pUnit, 
			NULL, 
			0, 
			UnitGetRoom(pUnit), 
			UnitGetPosition(pUnit), 
#ifdef HAVOK_ENABLED
			tImpact, 
#endif
			0);
			
	}
#endif // !ISVERSION(CLIENT_ONLY_VERSION)

	return TRUE;
}
