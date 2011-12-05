//----------------------------------------------------------------------------
// missiles.cpp
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
#include "level.h"
#include "c_sound.h"
#include "unit_priv.h"
#include "clients.h"
#include "missiles.h"
#include "combat.h"
#ifdef HAVOK_ENABLED
	#include "havok.h"
#endif
#include "ai.h"
#include "movement.h"
#include "s_message.h"
#include "c_particles.h"
#include "c_appearance.h"
#include "skills.h"
#ifdef HAVOK_ENABLED
	#include "c_ragdoll.h"
#endif
#include "items.h"
#include "unittag.h"
#include "weapons.h"
#include "filepaths.h"
#include "e_model.h"
#include "teams.h"
#include "c_camera.h"
#include "script.h"
#include "states.h"
#include "unitmodes.h"
#include "gameunits.h"
#include "room_path.h"
#include "excel_private.h"
#include "path.h"
#include "statssave.h"

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
#define MISSILE_RANGE			(1000.0f)
#define SPREAD_DELAY_IN_TICKS	(1)

static void sHitDoSkill(
	GAME* pGame,
	UNIT* pMissile,
	UNIT* pTarget,
	const UNIT_DATA * missile_data,
	int nSkill );

//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sUnknownEffect(	
	const UNIT_DATA *pUnitData, 
	const char *szParticleName)
{
	const int MAX_MESSAGE = 2048;
	char szMessage[ MAX_MESSAGE ];
	
	PStrPrintf( 
		szMessage, 
		MAX_MESSAGE, 
		"Invalid particle effect '%s' in unit '%s'",
		szParticleName,
		pUnitData->szName);

	ASSERTX( 0, szMessage );		
			
}




//----------------------------------------------------------------------------
// not thread safe
//----------------------------------------------------------------------------
static void c_sInitEffectSet(
	UNIT_DATA * unit_data,
	MISSILE_EFFECT eEffect)
{	
	PARTICLE_EFFECT_SET *pEffectSet = (PARTICLE_EFFECT_SET *)&unit_data->MissileEffects[eEffect];

	c_InitEffectSet(pEffectSet, unit_data->szParticleFolder, unit_data->szName);
}

//----------------------------------------------------------------------------
// returns fuse timer
//----------------------------------------------------------------------------
inline float sGetFuseTimer( 
	UNIT *pMissile )
{
	ASSERT_RETVAL( pMissile, 0.0f );
	
	if ( !UnitDataTestFlag( pMissile->pUnitData, UNIT_DATA_FLAG_IGNORE_FUSE_MS ) )
	{
		float fFuseTimer((float)UnitGetStat( pMissile, STATS_MISSILE_FUSE_MS ));
		if( fFuseTimer > 0 )
		{
			return fFuseTimer/MSECS_PER_SEC;
		}
	}
	return pMissile->pUnitData->fFuse;
}

//----------------------------------------------------------------------------
// not thread safe usage of unit_data
//----------------------------------------------------------------------------
void c_MissileDataInitParticleEffectSets( 
	UNIT_DATA * unit_data)
{
	ASSERT_RETURN(unit_data);

	for (unsigned int ii = 0; ii < NUM_MISSILE_EFFECTS; ++ii)
	{
		c_sInitEffectSet(unit_data, (MISSILE_EFFECT)ii);		
	}	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ExcelMissilesPostProcess( 
	EXCEL_TABLE * table)
{
	ExcelPostProcessUnitDataInit(table, TRUE);

	for (unsigned int ii = 0; ii < ExcelGetCountPrivate(table); ++ii)
	{
		UNIT_DATA * unit_data = (UNIT_DATA *)ExcelGetDataPrivate(table, ii);
		if(!unit_data)
			continue;

		if (unit_data->nUnitType == INVALID_ID)
		{
			unit_data->nUnitType = UNITTYPE_MISSILE;
		}
		int nCount( 0 );
		for( int t = 0; t < NUM_SKILL_MISSED; t++ )
		{
			if( unit_data->nSkillIdMissedArray[ t ] != INVALID_ID )
			{
				nCount++;
			}
			else
			{
				break; //this fixes errors with bad skills.
			}
		}
		unit_data->nSkillIdMissedCount = nCount;
		unit_data->fPathingCollisionRadius	= unit_data->fCollisionRadius;
		unit_data->fCollisionHeight	= unit_data->fCollisionRadius;
	}
	
	return TRUE;
}


//----------------------------------------------------------------------------
//  Hit function helpers
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sHitDoDamage(
	GAME* pGame,
	UNIT* pMissile,
	UNIT* pTarget)
{
	VECTOR vDirection, vBackupDirection;

	const UNIT_DATA * missile_data = MissileGetData(pGame, pMissile);
	ASSERT_RETURN(missile_data);

	if ( AppIsHellgate() )
	{
		if ( UnitDataTestFlag( missile_data, UNIT_DATA_FLAG_HAVOK_IMPACT_IGNORES_DIRECTION ) )
		{
			vDirection = VECTOR( 0 );
			vBackupDirection = VECTOR(0, 0, -1);
		}
#ifdef HAVOK_ENABLED
		else if ( pMissile->pHavokRigidBody )
		{
			HavokRigidBodyGetVelocity( pMissile->pHavokRigidBody, vDirection );
			VectorNormalize( vDirection );
			vBackupDirection = vDirection;
		}
#endif
		else
		{
			vDirection = UnitGetMoveDirection(pMissile);
			vBackupDirection = vDirection;
		}
	} else {
		vDirection = UnitGetMoveDirection(pMissile);
		vBackupDirection = vDirection;
	}

	VECTOR vVelocity;
	float fVelocity = UnitGetVelocityForImpact(pMissile);
	if(fVelocity <= 0.0f)
	{
		fVelocity = UnitGetVelocity(pMissile);
	}
	if(fVelocity < EPSILON)
	{
		fVelocity = 1.0f;
	}
	VectorScale(vVelocity, vBackupDirection, fVelocity / GAME_TICKS_PER_SECOND_FLOAT );
	// back up a little in case the missile has already penetrated
	VECTOR vSource = UnitGetPosition(pMissile) - vVelocity;

#ifdef HAVOK_ENABLED
	HAVOK_IMPACT tImpact;
	HavokImpactInit( pGame, tImpact, missile_data->fForce, vSource, &vDirection );
	if ( UnitDataTestFlag( missile_data, UNIT_DATA_FLAG_FORCE_IGNORES_SCALE ) )
	{
		tImpact.dwFlags |= HAVOK_IMPACT_FLAG_IGNORE_SCALE;
	}
#endif
	UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT ];
	UnitGetWeaponsTag( pMissile, TAG_SELECTOR_MISSILE_SOURCE, pWeapons );
	if ( pTarget && !UnitDataTestFlag(missile_data, UNIT_DATA_FLAG_ATTACKS_LOCATION_ON_HIT_UNIT) )
	{
		DWORD dwFlags = 0;
		SETBIT(dwFlags, UAU_DIRECT_MISSILE_BIT);
		s_UnitAttackUnit( pMissile, 
						  pTarget, 
						  pWeapons, 
						  0,
#ifdef HAVOK_ENABLED
						  &tImpact,
#endif
						  dwFlags,
						  -1,
						  missile_data->nDamageType );
	}
	else
	{
		s_UnitAttackLocation(pMissile, 
							 pWeapons, 
							 0, 
							 UnitGetRoom(pMissile), 
							 vSource, 
#ifdef HAVOK_ENABLED
							 tImpact, 
#endif
							 0 );
	}
}
#endif // #if !ISVERSION(CLIENT_ONLY_VERSION)


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void c_sHitCreateImpact(
	GAME* pGame,
	UNIT* pMissile,
	UNIT* pTarget,
	const UNIT_DATA * missile_data,
	VECTOR * pvNormal)
{
	ASSERT_RETURN(missile_data);

	if(missile_data->nMaximumImpactFrequency > 0)
	{
		GAME_TICK tiLastImpactTick = (GAME_TICK)UnitGetStat(pMissile, STATS_LAST_IMPACT_TICK);
		GAME_TICK tiCurrentTick = GameGetTick(pGame);
		if((int)(tiCurrentTick - tiLastImpactTick) < missile_data->nMaximumImpactFrequency)
		{
			return;
		}
		UnitSetStat(pMissile, STATS_LAST_IMPACT_TICK, tiCurrentTick);
	}

	int nModelId = c_UnitGetModelId( pMissile );
	VECTOR vHitPosition;
	if ( UnitDataTestFlag( missile_data, UNIT_DATA_FLAG_STICK_ON_HIT ) && ( nModelId != INVALID_ID ) && UnitTestFlag( pMissile, UNITFLAG_DONT_SET_WORLD ))
		vHitPosition = e_ModelGetPosition( nModelId ); // an attached missile has a different model position - so grab the model position
	else 
	{
		vHitPosition = UnitGetPosition(pMissile);
		if( AppIsTugboat() && pTarget )
		{
			VECTOR Target = UnitGetPosition( pTarget );
			vHitPosition.x = Target.x;
			vHitPosition.y = Target.y;
		}
	}
	VECTOR vHitDelta = UnitGetMoveDirection(pMissile);
	vHitDelta *= -missile_data->fHitBackup;
	vHitPosition += vHitDelta;

	MISSILE_EFFECT eEffect = pTarget ? MISSILE_EFFECT_IMPACT_UNIT : MISSILE_EFFECT_IMPACT_WALL;
	VECTOR vNormal = pvNormal ? *pvNormal : UnitGetFaceDirection(pMissile, FALSE);
	c_MissileEffectCreate(pGame, pMissile, NULL, UnitGetClass( pMissile ), eEffect,
		vHitPosition, vNormal);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sMoveAttachedUnit(
	GAME* pGame,
	UNIT* pUnit,
	const struct EVENT_DATA& tEventData)
{
	UNIT * pTarget = UnitGetById( pGame, (DWORD)tEventData.m_Data1 );
	if ( ! pTarget )
		return FALSE;
	VECTOR vDelta;
	vDelta.fX = EventParamToFloat( tEventData.m_Data2 );
	vDelta.fY = EventParamToFloat( tEventData.m_Data3 );
	vDelta.fZ = EventParamToFloat( tEventData.m_Data4 );
	UnitSetPosition( pUnit, UnitGetPosition( pTarget ) + vDelta );
	UnitUpdateRoom( pUnit, UnitGetRoom( pTarget ), TRUE );

	UnitRegisterEventTimed( pUnit, sMoveAttachedUnit, &tEventData, 1 );
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitAttachToUnit( 
	UNIT * pUnit, 
	UNIT * pTarget )
{
	ASSERT_RETURN( pUnit );
	ASSERT_RETURN( pTarget );
	VECTOR vDelta = UnitGetPosition( pUnit ) - UnitGetPosition( pTarget );
	UnitStepListRemove( UnitGetGame( pUnit ), pUnit, SLRF_FORCE );
	EVENT_DATA tEventData( UnitGetId( pTarget ), EventFloatToParam( vDelta.fX ), 
												 EventFloatToParam( vDelta.fY ), 
												 EventFloatToParam( vDelta.fZ ) );
	UnitRegisterEventTimed( pUnit, sMoveAttachedUnit, &tEventData, 1 );
	UnitSetFlag( pUnit, UNITFLAG_ATTACHED, TRUE );
	UnitSetAITarget(pUnit, pTarget, TRUE);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void UnitDetach( 
	UNIT * pUnit )
{
	ASSERT_RETURN( pUnit );
	UnitUnregisterTimedEvent( pUnit, sMoveAttachedUnit );
	UnitSetFlag( pUnit, UNITFLAG_ATTACHED, FALSE );
	if ( IS_CLIENT( pUnit ) )
	{
		UnitSetFlag( pUnit, UNITFLAG_DONT_SET_WORLD, FALSE );
		DWORD dwAIGetTargetFlags = 0;
		SETBIT(dwAIGetTargetFlags, GET_AI_TARGET_ONLY_OVERRIDE_BIT);
		UNIT * pTarget = UnitGetAITarget(pUnit, dwAIGetTargetFlags);
		if ( pTarget )
		{
			int nModelId = c_UnitGetModelId( pTarget );
			if ( nModelId != INVALID_ID )
				c_ModelAttachmentRemoveAllByOwner( nModelId, ATTACHMENT_OWNER_MISSILE, UnitGetId( pUnit ), ATTACHMENT_FUNC_FLAG_FLOAT );
		}
	}
	UnitSetAITarget(pUnit, INVALID_ID, TRUE);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sStickToWallOrTarget (
	GAME * pGame,
	UNIT * pMissile,
	UNIT * pTarget,
	UNIT * pSourceMissile,
	const UNIT_DATA * missile_data )
{
	ASSERT_RETURN(pGame && pMissile);
	ASSERT_RETURN(missile_data);

	if ( pTarget )
	{
		if ( IS_CLIENT( pGame ) )
		{
			int nTargetModelId = c_UnitGetModelId( pTarget );
			int nMissileModelId = c_UnitGetModelId( pMissile );
			if ( nTargetModelId != INVALID_ID && nMissileModelId != INVALID_ID )
			{
				ATTACHMENT_DEFINITION tAttachDef;
				e_AttachmentDefInit( tAttachDef );
				tAttachDef.eType = ATTACHMENT_MODEL;

				VECTOR vBackup = UnitGetMoveDirection( pSourceMissile );
				VECTOR vSourcePos = UnitGetPosition( pSourceMissile ) - vBackup;
				if ( c_AppearanceInitAttachmentOnMesh ( pGame, nTargetModelId, tAttachDef, vSourcePos, UnitGetMoveDirection( pSourceMissile ) ) )
				{
					UnitSetFlag( pMissile, UNITFLAG_DONT_SET_WORLD, TRUE );
					c_ModelAttachmentAdd( nTargetModelId, tAttachDef, "", TRUE, ATTACHMENT_OWNER_MISSILE, UnitGetId( pMissile ), nMissileModelId );
					c_ModelSetAttachmentNormals( nMissileModelId, tAttachDef.vNormal );
					c_ModelAttachmentWarpToCurrentPosition( nMissileModelId );
				}
			}
		}
		UnitAttachToUnit( pMissile, pTarget );

	} else {
		UnitStepListRemove( pGame, pMissile, SLRF_FORCE );
		if ( IS_CLIENT( pGame ) && pMissile != pSourceMissile )
		{
			VECTOR vDirection = UnitGetFaceDirection( pSourceMissile, FALSE );
			UnitSetFaceDirection( pMissile, vDirection, TRUE );
			UnitSetMoveDirection( pMissile, vDirection );
		}
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UNITID sFindLowestUnitIdGreaterThanOrLowestNotEqualTo(
	UNITID idTarget,
	UNITID * pnUnitIds,
	int nIdCount)
{
	ASSERT_RETINVALID(nIdCount > 0);

	UNITID idReturn = INVALID_ID;
	for(int i=0; i<nIdCount; i++)
	{
		if(pnUnitIds[i] > idTarget)
		{
			idReturn = pnUnitIds[i];
			break;
		}
	}

	if(idReturn == INVALID_ID)
	{
		idReturn = pnUnitIds[0];
		if(idReturn == idTarget)
		{
			idReturn = INVALID_ID;
		}
	}
	return idReturn;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
VECTOR MissileBounceFindNewDirection(
	GAME * pGame,
	UNIT * pSeekerUnit,
	VECTOR vSearchCenter,
	VECTOR vSearchDirection,
	UNIT * pTarget,
	const VECTOR & vNormal,
	DWORD dwFlags,
	float * pRange)
{
	BOOL bNewDirectionIsValid = FALSE;
	VECTOR vNewDirection = vNormal;
	if(!bNewDirectionIsValid && TESTBIT(dwFlags, MBND_RETARGET_BIT))
	{
		UNITID idTargets[10];

		SKILL_TARGETING_QUERY tTargetQuery;
		tTargetQuery.pnUnitIds = idTargets;
		tTargetQuery.fMaxRange = pRange ? *pRange : 20.0f;
		tTargetQuery.fLOSDistance = tTargetQuery.fMaxRange;
		tTargetQuery.nUnitIdMax = 10;
		tTargetQuery.pvLocation = &vSearchCenter;
		SETBIT( tTargetQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_ENEMY );
		//SETBIT( tTargetQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_OBJECT );
		SETBIT( tTargetQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_CHECK_LOS );
		SETBIT( tTargetQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_ALLOW_DESTRUCTABLES );
		SETBIT( tTargetQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_CLOSEST );
		SETBIT( tTargetQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_SORT_BY_UNITID );
		SETBIT( tTargetQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_USE_LOCATION );
		tTargetQuery.pSeekerUnit = pSeekerUnit;
		SkillTargetQuery( pGame, tTargetQuery );
		if (tTargetQuery.nUnitIdCount > 0)
		{
			UNITID idOldTarget = UnitGetId( pTarget );
			UNITID idNewTarget = sFindLowestUnitIdGreaterThanOrLowestNotEqualTo(idOldTarget, idTargets, tTargetQuery.nUnitIdCount);
			if(idNewTarget != INVALID_ID && idNewTarget != idOldTarget)
			{
				UNIT * pUnitTarget = UnitGetById(pGame, idNewTarget);
				if(TESTBIT(dwFlags, MBND_HAS_SIDE_EFFECTS_BIT))
				{
					UnitSetStat( pSeekerUnit, STATS_AI_TARGET_ID, idNewTarget );
				}
				vNewDirection = UnitGetCenter(pUnitTarget) - vSearchCenter;
				VectorNormalize(vNewDirection);
				bNewDirectionIsValid = TRUE;
			}
		}
		else
		{
			bNewDirectionIsValid = FALSE;
		}
	}

	if(!bNewDirectionIsValid && TESTBIT(dwFlags, MBND_NEW_DIRECTION_BIT))
	{
		RAND tRand;
		RandInit( tRand, UnitGetStat( pSeekerUnit, STATS_SKILL_SEED ) );
		vNewDirection = RandGetVector(tRand);
		vNewDirection.fZ = 0.0f;
		VectorNormalize(vNewDirection);
		if(VectorDot(vNewDirection, vNormal) < 0)
		{
			vNewDirection = -vNewDirection;
		}
		if(TESTBIT(dwFlags, MBND_HAS_SIDE_EFFECTS_BIT))
		{
			UnitSetStat( pSeekerUnit, STATS_SKILL_SEED, 0, RandGetNum( tRand ) );
		}
		bNewDirectionIsValid = TRUE;
	}

	if(!bNewDirectionIsValid)
	{
		// using the reflection calculation from 3d graphics stuff... 2 ( N dot L ) * N - L
		VECTOR vMoveDirection = vSearchDirection;
		float fScalar = 2.0f * VectorDot( vNormal, vMoveDirection );
		vNewDirection *= fScalar;
		vNewDirection -= vMoveDirection;
		vNewDirection = -vNewDirection; // we need to move away from the surface
	}
	
	return vNewDirection;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHitBounce (
	GAME * pGame,
	UNIT * pMissile,
	UNIT * pTarget,
	const UNIT_DATA * missile_data,
	VECTOR * pvBackgroundNormal )
{
	VECTOR vNormal;
	if ( pTarget ) 
	{
		BOOL bUseCenter = TRUE;
		if ( AppIsHellgate() )
		{
			VECTOR vMissilePos = UnitGetPosition( pMissile );
			VECTOR vMissileDir = UnitGetMoveDirection( pMissile );

			if ( vMissileDir.fZ < -0.1f )
			{
				VECTOR vTargetTop = UnitGetPositionAtPercentHeight( pTarget, 0.8f );
				if ( vMissilePos.fZ > vTargetTop.fZ )
				{
					vNormal = VECTOR( 0, 0, 1 );
					bUseCenter = FALSE;
				}
			} 
			else if ( vMissileDir.fZ > 0.1f )
			{
				VECTOR vTargetTop = UnitGetPositionAtPercentHeight( pTarget, 0.2f );
				if ( vMissilePos.fZ < vTargetTop.fZ )
				{
					vNormal = VECTOR( 0, 0, -1 );
					bUseCenter = FALSE;
				}
			}
			if ( bUseCenter )
			{
				vNormal = UnitGetCenter( pTarget ) - UnitGetPosition( pMissile );
				vNormal.fZ = 0.0f;
				VectorNormalize( vNormal );

				if ( VectorDot( vNormal, vMissileDir ) < 0.0f )
					vNormal = -vNormal;
			}
		} 
		else
		{
			vNormal = UnitGetCenter( pTarget ) - UnitGetPosition( pMissile );
			VectorNormalize( vNormal );
		}
	}
	else if ( pvBackgroundNormal )
	{
		vNormal = *pvBackgroundNormal;
		// Mythos wants to reflect on a plane
		if( AppIsTugboat() )
		{
			vNormal.fZ = 0;
			VectorNormalize( vNormal, vNormal );
		}
	} 
	else
	{
		return;
	}

	DWORD dwNewDirectionFlags = 0;
	SETBIT(dwNewDirectionFlags, MBND_RETARGET_BIT, UnitGetStat( pMissile, STATS_BOUNCE_RETARGET ) == TRUE);
	SETBIT(dwNewDirectionFlags, MBND_NEW_DIRECTION_BIT, UnitGetStat( pMissile, STATS_BOUNCE_NEW_DIRECTION ) == TRUE);
	SETBIT(dwNewDirectionFlags, MBND_HAS_SIDE_EFFECTS_BIT, TRUE);
	VECTOR vNewDirection = MissileBounceFindNewDirection(pGame, pMissile, UnitGetPosition(pMissile), UnitGetMoveDirection(pMissile), pTarget, vNormal, dwNewDirectionFlags);

	UnitSetMoveDirection( pMissile, vNewDirection );

#ifdef HAVOK_ENABLED
	if ( pMissile->pHavokRigidBody )
	{
		VECTOR vPreviousVelocity( 0.0f );
		HavokRigidBodyGetVelocity( pMissile->pHavokRigidBody, vPreviousVelocity );
		float fSpeed = VectorLength( vPreviousVelocity );
		if ( missile_data->fBounce != 0.0f )
			fSpeed *= missile_data->fBounce;
		vNewDirection *= fSpeed;
		HavokRigidBodySetVelocity( pMissile->pHavokRigidBody, &vNewDirection );
	}
#endif

	if(!UnitIsOnGround(pMissile))
	{
		UnitSetZVelocity(pMissile, -UnitGetZVelocity(pMissile) * (1.0f - missile_data->fDampening));
	}

	UnitSetFlag( pMissile, UNITFLAG_DONT_COLLIDE_NEXT_STEP, TRUE ); // keep it from repeating this collision
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static UNIT * sHitFireMissile(
	GAME * pGame,
	UNIT * pMissile,
	UNIT * pTarget,
	const UNIT_DATA * missile_data,
	VECTOR * pvNormal,
	UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT ],
	BOOL bOnFuseOrFree,
	BOOL bOnMiss )
{
	ASSERT_RETNULL(pGame && pMissile);
	ASSERT_RETNULL(missile_data);

	int nMissileHitClass;
	if( bOnMiss )
	{
		nMissileHitClass = missile_data->nMissileOnMissed;
	}
	else if ( bOnFuseOrFree )
	{
		nMissileHitClass = missile_data->nMissileOnFreeOrFuse;
	}
	else if ( pTarget )
		nMissileHitClass = missile_data->nMissileHitUnit;
	else
		nMissileHitClass = missile_data->nMissileHitBackground;

	if (nMissileHitClass == INVALID_ID)
		return NULL;

	PARAM dwMissileTag = 0;
	const UNIT_DATA* hit_missile_data = MissileGetData(pGame, nMissileHitClass);
	if (hit_missile_data && hit_missile_data->nMissileTag > 0)
	{
		UnitGetStatAny(pMissile, STATS_MISSILE_TAG, &dwMissileTag);
	}

	int nSkill = UnitGetStat( pMissile, STATS_AI_SKILL_PRIMARY );
	UNIT * pMissileHit = MissileFire(pGame, 
									nMissileHitClass, 
									pMissile, // we want this missile as the owner so that it passes on its stats
									pWeapons, 
									nSkill,
									pTarget, VECTOR(0),
									UnitGetPosition( pMissile ),
									pvNormal ? *pvNormal : VECTOR( 0, 0, 1),
									UnitGetStat(pMissile, STATS_SKILL_SEED), 
									dwMissileTag,
									MAKE_MASK(MF_FROM_MISSILE_BIT));

	if ( pMissileHit )
	{
		const UNIT_DATA * pMissileHitData = UnitGetData( pMissileHit );
		if ( UnitDataTestFlag( pMissileHitData, UNIT_DATA_FLAG_STICK_ON_HIT ) )
			sStickToWallOrTarget( pGame, pMissileHit, pTarget, pMissile, pMissileHitData );

		if ( UnitDataTestFlag(pMissileHitData, UNIT_DATA_FLAG_INHERITS_DIRECTION) && !pTarget )
		{
			UnitSetFaceDirection( pMissileHit, -UnitGetFaceDirection( pMissile, TRUE ), TRUE, FALSE );
		}

		if ( pTarget )
			sHitDoSkill( pGame, pMissileHit, pTarget, hit_missile_data, hit_missile_data->nSkillIdHitUnit );
		else
			sHitDoSkill( pGame, pMissileHit, pTarget, hit_missile_data, hit_missile_data->nSkillIdHitBackground );
	}

	return pMissileHit;
}

//----------------------------------------------------------------------------
enum MISSILE_KILL_RESULT
{
	MISSILE_DESTROYED,
	MISSILE_OK,
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static MISSILE_KILL_RESULT sHitKillMissile(
	GAME* pGame,
	UNIT* pMissile,
	UNIT* pTarget,
	const UNIT_DATA * missile_data)
{
	ASSERTX_RETVAL( missile_data, MISSILE_DESTROYED, "Expected missile data" );
	BOOL bFreeMissile = FALSE;
	
	if (pTarget)
	{
		if ( UnitDataTestFlag( missile_data, UNIT_DATA_FLAG_KILL_ON_UNIT_HIT ) &&
			UnitGetStat( pMissile, STATS_BOUNCE_ON_HIT_UNIT ) == FALSE &&
			UnitGetStat( pMissile, STATS_BOUNCE_RETARGET ) == FALSE)
		{
			bFreeMissile = TRUE;
		}
	}
	else
	{
		if (UnitDataTestFlag( missile_data, UNIT_DATA_FLAG_KILL_ON_BACKGROUND_HIT ) &&
			UnitGetStat( pMissile, STATS_BOUNCE_ON_HIT_BACKGROUND ) == FALSE &&
			UnitGetStat( pMissile, STATS_BOUNCE_RETARGET ) == FALSE)
		{
			bFreeMissile = TRUE;
		}
	}
	
	// do the conditional missile free	
	if (bFreeMissile == TRUE)
	{
		MissileConditionalFree(pMissile);	
		return MISSILE_DESTROYED;	
	}
	
	return MISSILE_OK;
	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sHitDoSkill(
	GAME* pGame,
	UNIT* pMissile,
	UNIT* pTarget,
	const UNIT_DATA * missile_data,
	int nSkill )
{
	ASSERT_RETURN(missile_data);
	if ( nSkill == INVALID_ID )
		return;

	DWORD dwSkillSeed = UnitGetStat( pMissile, STATS_SKILL_SEED );

	UNITID idTarget = UnitGetId( pTarget );
	if ( idTarget == INVALID_ID )
	{// we might need to check to see if we auto-hit or not
		pTarget = SkillGetAnyTarget( pMissile );
		idTarget = UnitGetId( pTarget );
	}
	VECTOR vTarget = UnitGetStatVector( pMissile, STATS_SKILL_TARGET_X, nSkill, 0 );

	VECTOR vHitDelta = UnitGetMoveDirection( pMissile );
	vHitDelta *= -missile_data->fHitBackup;
	vTarget += vHitDelta;

	DWORD dwFlags = 0;
	if ( IS_SERVER( pGame ) )
		dwFlags |= SKILL_START_FLAG_INITIATED_BY_SERVER;

	SkillStartRequest( pGame, pMissile, nSkill, idTarget, vTarget, dwFlags, dwSkillSeed );
}


//----------------------------------------------------------------------------
//  Hit functions
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sMissileHitFuncCommon(
	GAME* pGame,
	UNIT* pMissile,
	UNIT* pTarget,
	EVENT_DATA* pHandlerData,
	VECTOR * pvBackgroundNormal,
	DWORD dwId,
	BOOL bCollided,
	BOOL bOnFuse,
	BOOL bOnFree,
	BOOL bMissed)
{
	if (UnitTestFlag(pMissile, UNITFLAG_DELAYED_FREE))
	{
		return; // must be multiple hits or something.
	}

	const UNIT_DATA * missile_data = MissileGetData(pGame, pMissile);
	ASSERT_RETURN(missile_data);

	if( AppIsTugboat() &&
		pTarget &&
		pTarget->bStructural )
	{
		//Structural objects act as background pieces. If this 
		//doesn't occure you could warp into all sorts of bad spots.
		pTarget = NULL;
	}


	if ( pTarget && ! UnitDataTestFlag( missile_data, UNIT_DATA_FLAG_HITS_UNITS ) )
		return;

	DWORD dwUnitGetAITargetFlags = 0;
	if ( ! UnitTestFlag( pMissile, UNITFLAG_ATTACHED ) ) // if we are attached to something, then use it at the target
	{
		SETBIT(dwUnitGetAITargetFlags, GET_AI_TARGET_ONLY_OVERRIDE_BIT);
	}
	UNITID idAIOverrideTarget = UnitGetAITargetId(pMissile, dwUnitGetAITargetFlags);
	if(idAIOverrideTarget != INVALID_ID)
	{
		pTarget = UnitGetById(pGame, idAIOverrideTarget);
		if ( pTarget && !UnitsAreInRange( pMissile, pTarget, 0.0f, AppIsHellgate() ? 0.1f : .35f, NULL ) ) // just because we got to this function, it doesn't mean that this target is anywhere close to the missile
			pTarget = NULL;
	}

	dwUnitGetAITargetFlags = 0;
	SETBIT(dwUnitGetAITargetFlags, GET_AI_TARGET_IGNORE_OVERRIDE_BIT);
	UNITID idAITarget = UnitGetAITargetId(pMissile, dwUnitGetAITargetFlags);
	if ( pTarget && UnitDataTestFlag( missile_data, UNIT_DATA_FLAG_DONT_HIT_SKILL_TARGET ) && UnitGetId(pTarget) == idAITarget )
	{
		return;
	}

	BOOL bHitLastTarget = FALSE;
	UNITID idLastTarget = UnitGetStat( pMissile, STATS_MISSILE_LAST_TARGET_ID );
	if ( pTarget && UnitGetId(pTarget) == idLastTarget )
	{
		bHitLastTarget = TRUE;
	}
	if ( pTarget )
		UnitSetStat( pMissile, STATS_MISSILE_LAST_TARGET_ID, UnitGetId( pTarget ) );

	UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT ];
	UnitGetWeaponsTag( pMissile, TAG_SELECTOR_MISSILE_SOURCE, pWeapons );

	if (IS_CLIENT(pGame))
	{
		if ((UnitDataTestFlag(missile_data, UNIT_DATA_FLAG_ADD_IMPACT_ON_FUSE) && bOnFuse ) ||
			(UnitDataTestFlag(missile_data, UNIT_DATA_FLAG_ADD_IMPACT_ON_FREE) && bOnFree ) ||
			(UnitDataTestFlag(missile_data, UNIT_DATA_FLAG_ADD_IMPACT_ON_HIT_BACKGROUND) && ! pTarget && !bOnFree ) ||
			(UnitDataTestFlag(missile_data, UNIT_DATA_FLAG_ADD_IMPACT_ON_HIT_UNIT) && pTarget ) )
		{
			c_sHitCreateImpact(pGame, pMissile, pTarget, missile_data, pvBackgroundNormal);
		}
	}

#if !ISVERSION(CLIENT_ONLY_VERSION)
	if (IS_SERVER(pGame))
	{
		if ((UnitDataTestFlag( missile_data, UNIT_DATA_FLAG_DAMAGE_ONLY_ON_FUSE ) && bOnFuse) ||
			(UnitDataTestFlag( missile_data, UNIT_DATA_FLAG_DAMAGES_ON_HIT_BACKGROUND ) && ! pTarget ) ||
			(UnitDataTestFlag( missile_data, UNIT_DATA_FLAG_DAMAGES_ON_HIT_UNIT ) && pTarget ) )
		{
			s_sHitDoDamage(pGame, pMissile, pTarget);
		}
	}
#endif // #if !ISVERSION(CLIENT_ONLY_VERSION)

	
	if ( UnitDataTestFlag( missile_data, UNIT_DATA_FLAG_STICK_ON_HIT ) )
	{
		sStickToWallOrTarget( pGame, pMissile, pTarget, pMissile, missile_data );
	}
	else if (( (UnitGetStat( pMissile, STATS_BOUNCE_ON_HIT_UNIT ) || UnitGetStat( pMissile, STATS_BOUNCE_RETARGET ))	&& pTarget != NULL && ! bHitLastTarget ) ||
			 (  UnitGetStat( pMissile, STATS_BOUNCE_ON_HIT_BACKGROUND )													&& pTarget == NULL ) )
	{		
		BOOL bRetarget = UnitGetStat( pMissile, STATS_BOUNCE_RETARGET );
		sHitBounce( pGame, pMissile, pTarget, missile_data, pvBackgroundNormal );
		int nBounceCount = UnitGetStat( pMissile, STATS_MISSILE_RICOCHET_COUNT );
		if( nBounceCount > 0 ) //-1 should always ricochet
		{									
			nBounceCount = UnitGetStat( pMissile, STATS_MISSILE_RICOCHET_COUNT ) - 1;
			UnitSetStat( pMissile, STATS_MISSILE_RICOCHET_COUNT, nBounceCount ); //the first time doesn't count. ( ricochet 1 should ricochet once, if we put the bounce count before hand it ends on it's first hit )			
#if !ISVERSION(CLIENT_ONLY_VERSION)
			if(bRetarget)
			{
				int percent = UnitGetStat( pMissile, STATS_MISSILE_RICOCHET_DMG_MODIFIER );
				if( percent != 0 )
				{
					int nCurr = UnitGetStat( pMissile, STATS_DAMAGE_INCREMENT );
					int nNew = nCurr ? ( nCurr * percent ) / 100 : percent;
					UnitSetStat(pMissile, STATS_DAMAGE_INCREMENT, nNew );
				}
			}
#endif

			if(AppIsHellgate())
			{
				// reset the range
				BOOL bReflect = UnitGetStat( pMissile, STATS_BOUNCE_ON_HIT_UNIT );
				float fRange = UnitGetStatFloat( pMissile, STATS_MISSILE_DISTANCE, 1);
				if(bReflect && bRetarget)
				{
					fRange /= 2.0f;
				}
				else
				{
					fRange /= 3.0f;
				}
				UnitSetStatFloat(pMissile, STATS_MISSILE_DISTANCE, 0, fRange);
			}
		}
		else if( nBounceCount == 0 )	
		{
			UnitSetStat( pMissile, STATS_BOUNCE_ON_HIT_UNIT, 0 );
			UnitSetStat( pMissile, STATS_BOUNCE_ON_HIT_BACKGROUND, 0 );
			UnitSetStat( pMissile, STATS_BOUNCE_RETARGET, 0 );
		}
	}

	sHitFireMissile(pGame, pMissile, pTarget, missile_data, pvBackgroundNormal, pWeapons, bOnFuse || bOnFree, bMissed );

	if ( bOnFuse )
		sHitDoSkill(pGame, pMissile, pTarget, missile_data, missile_data->nSkillIdOnFuse );
	else if ( pTarget )
		sHitDoSkill(pGame, pMissile, pTarget, missile_data, missile_data->nSkillIdHitUnit );
	else
		sHitDoSkill(pGame, pMissile, pTarget, missile_data, missile_data->nSkillIdHitBackground );

	if (sHitKillMissile(pGame, pMissile, pTarget, missile_data) == MISSILE_DESTROYED)
	{
		return;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sMissileHandleCollided(
	GAME* pGame,
	UNIT* pMissile,
	UNIT* pTarget,
	EVENT_DATA* pHandlerData,
	void* pData,
	DWORD dwId )
{
	sMissileHitFuncCommon( pGame, pMissile, pTarget, pHandlerData, (VECTOR *)pData, INVALID_ID, TRUE, FALSE, FALSE, FALSE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_sMissileHandleFreed(
	GAME* pGame,
	UNIT* pMissile,
	UNIT* pTarget,
	EVENT_DATA* pHandlerData,
	void* pData,
	DWORD dwId )
{
	ASSERTX_RETURN( IS_CLIENT( pGame ), "Expected client" );
	DWORD dwUnitFreedFlags = (DWORD)CAST_PTR_TO_INT(pData);
	if ( ( dwUnitFreedFlags & UFF_TRUE_FREE ) == 0 )
		sMissileHitFuncCommon( pGame, pMissile, pTarget, pHandlerData, NULL, dwId, FALSE, FALSE, TRUE, FALSE );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sMissileHitEndRange(
	GAME* pGame,
	UNIT* pMissile,
	UNIT* pTarget,
	EVENT_DATA* pHandlerData,
	void* pData,
	DWORD dwId )
{
	const UNIT_DATA * missile_data = MissileGetData(pGame, pMissile);
	if( missile_data && missile_data->nSkillIdMissedCount > 0 )
	{
		int nIndex = RandGetNum( pGame->rand, missile_data->nSkillIdMissedCount );
		sHitDoSkill(pGame, pMissile, pTarget, missile_data, missile_data->nSkillIdMissedArray[ nIndex ] );
	}else if( AppIsTugboat() )
	{
		//imagine if this was hitting the ground(background)
		sMissileHitFuncCommon( pGame, pMissile, NULL, pHandlerData, NULL, dwId, FALSE, FALSE, TRUE, TRUE );
	}


	MissileConditionalFree(pMissile);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sHandleMissileHavokHit( 
	GAME * pGame, 
	UNIT * pUnit, 
	const VECTOR & vNormal,
	void * data)
{
	UnitTriggerEvent( pGame, EVENT_DIDCOLLIDE, pUnit, NULL, (void *) &vNormal );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sHandleFuseMissileEvent( 
	GAME* pGame,
	UNIT* pUnit,
	const struct EVENT_DATA& event_data)
{
	UNITID idUnit = UnitGetId( pUnit );
	VECTOR vBackgroundNormal = UnitGetFaceDirection(pUnit, FALSE);
	UNITID idTarget = INVALID_ID;
	if ( UnitTestFlag( pUnit, UNITFLAG_ATTACHED ) )
		idTarget = UnitGetAITargetId( pUnit );
	sMissileHitFuncCommon( pGame, pUnit, NULL, NULL, &vBackgroundNormal, idTarget, FALSE, TRUE, FALSE, FALSE );

	pUnit = UnitGetById( pGame, idUnit );
	if ( pUnit )
	{
		MissileConditionalFree( pUnit );			
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void MissileSetFuse(
	UNIT * pMissile,
	int nTicks )
{
	MissileUnFuse(pMissile);
	UnitRegisterEventTimed( pMissile, sHandleFuseMissileEvent, &EVENT_DATA(), nTicks);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void MissileUnFuse(
	UNIT * pMissile)
{
	UnitUnregisterTimedEvent( pMissile, sHandleFuseMissileEvent );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sHandleRepeatDamageEvent( 
	GAME* pGame,
	UNIT* pMissile,
	const struct EVENT_DATA& event_data)
{
	const UNIT_DATA * missile_data = MissileGetData(pGame, pMissile);
	ASSERT_RETFALSE(missile_data);

	BOOL bDamageFirstTime = (BOOL) event_data.m_Data1;

	if ( missile_data->nDamageRepeatChance == 0 ||
		 missile_data->nDamageRepeatChance > (int)RandGetNum( pGame, 100 ) || 
		 bDamageFirstTime )
	{
#if !ISVERSION(CLIENT_ONLY_VERSION)
		if(IS_SERVER(pGame))
		{
#ifdef HAVOK_ENABLED
			HAVOK_IMPACT tImpact;
			HavokImpactInit( pGame, tImpact, missile_data->fForce, UnitGetPosition(pMissile), NULL );
			if (UnitDataTestFlag( missile_data, UNIT_DATA_FLAG_FORCE_IGNORES_SCALE ))
			{
				tImpact.dwFlags |= HAVOK_IMPACT_FLAG_IGNORE_SCALE;
			}
#endif

			DWORD dwAttackFlags = 0;
			if(UnitGetStat(pMissile, STATS_APPLY_DUALWEAPON_FOCUS_SCALING) == 1)
			{
				SETBIT(dwAttackFlags, UAU_APPLY_DUALWEAPON_FOCUS_SCALING_BIT, TRUE);
			}
			UNIT * pTarget = UnitGetAITarget( pMissile );
			if ( pTarget &&
				!UnitDataTestFlag( missile_data, UNIT_DATA_FLAG_IGNORE_AI_TARGET_ON_REPEAT_DMG ) )
			{
				s_UnitAttackUnit( pMissile, 
					pTarget, 
					NULL, 
					0,
#ifdef HAVOK_ENABLED
					&tImpact,
#endif
					dwAttackFlags);
			} else {
				s_UnitAttackLocation(pMissile, 
					NULL, 
					0, 
					UnitGetRoom(pMissile), 
					UnitGetPosition(pMissile), 
#ifdef HAVOK_ENABLED
					tImpact, 
#endif
					dwAttackFlags);
			}
		}
#endif // #if !ISVERSION(CLIENT_ONLY_VERSION)
		if( missile_data->nSkillIdOnDamageRepeat != INVALID_ID )
		{
			UNIT * pTarget = UnitGetAITarget( pMissile );
			sHitDoSkill(pGame, pMissile, pTarget, missile_data, missile_data->nSkillIdOnDamageRepeat );
		}
		
	}

	if ( !UnitTestFlag( pMissile, UNITFLAG_JUSTFREED) )
	{
		UnitRegisterEventTimed(pMissile, sHandleRepeatDamageEvent, &EVENT_DATA(FALSE), missile_data->nDamageRepeatRate);
	}

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float MissileGetRangePercent(
	UNIT * source,
	UNIT * pWeapon,
	const struct UNIT_DATA * missile_data)
{
	if (!missile_data)
	{
		return 1.0f;
	}
	int range = source ? 100 + UnitGetStat(source, STATS_MISSILE_RANGE) : 100;
	if ( pWeapon && pWeapon != source && UnitHasInventory( source ) )
	{ 
		//UNIT * pEquippedWeapons[ MAX_WEAPONS_PER_UNIT ];
		//pEquippedWeapons[ 0 ] = pWeapon;
		//for ( int i = 1; i < MAX_WEAPONS_PER_UNIT; i++ )
		//	pEquippedWeapons[ i ] = NULL;
		//UnitInventorySetEquippedWeapons( source, pEquippedWeapons );

		 //this is the way to do with without changing the equipped weapons
		UNIT * pEquippedWeapons[ MAX_WEAPONS_PER_UNIT ];
		UnitInventoryGetEquippedWeapons( source, pEquippedWeapons );
		BOOL bThisWeaponIsEquipped = FALSE;
		for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
		{
			if ( pEquippedWeapons[ i ] == pWeapon )
			{
				bThisWeaponIsEquipped = TRUE;
			}
			else if ( pEquippedWeapons[ i ] )
				range -= UnitGetStat( pEquippedWeapons[ i ], STATS_MISSILE_RANGE );
		}

		if ( ! bThisWeaponIsEquipped )
			range += UnitGetStat( pWeapon, STATS_MISSILE_RANGE );

	}
	range = MIN(range, missile_data->nRangeMaxDeltaPercent);
	range = MAX(range, missile_data->nRangeMinDeltaPercent);
	return (float)range / 100.0f;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static float sMissileGetVelocityBonusPct(
	UNIT * pUnit)
{
	ASSERT_RETZERO(pUnit);
	return (float)(UnitGetStat(pUnit, STATS_MISSILE_VELOCITY_BONUS_PCT) + UnitGetStat(pUnit, STATS_MISSILE_VELOCITY_BONUS_PCT_OFFWEAPON))  / 100.0f;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float MissileGetFireSpeed(
	GAME * pGame,
	const UNIT_DATA * pMissileData,
	UNIT * pOwner,
	UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT ],
	const VECTOR& vMoveDirectionIn)
{
	float fVelocity = pMissileData->tVelocities[0].fVelocityBase;
	float fVelocityMax = pMissileData->tVelocities[0].fVelocityMax;
	float fVelocityMin = pMissileData->tVelocities[0].fVelocityMin;
	VECTOR vMoveDirection = vMoveDirectionIn;

	if (pMissileData->tHavokShapeFallback.pHavokShape || pMissileData->tHavokShapePrefered.pHavokShape )
	{// if we are using a havok shape, then our velocity is affected by range
		float fRangePercent = MissileGetRangePercent(pOwner, pWeapons ? pWeapons[0] : NULL, pMissileData);
		fVelocity *= fRangePercent;
	}
	if ( UnitDataTestFlag( pMissileData, UNIT_DATA_FLAG_USE_SOURCE_VELOCITY ) && pOwner)	
	{// adjust forward speed to compensate for pOwner's velocity
		VECTOR vSourceMoveDirection = UnitGetMoveDirection(pOwner);
		float fSourceVelocity = UnitGetVelocity(pOwner);
		fVelocity += fSourceVelocity * VectorDot(vMoveDirection, vSourceMoveDirection);
	}

	float fBonusPercent = 0;
	for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
	{
		if ( pWeapons && pWeapons[ i ] )
			fBonusPercent += sMissileGetVelocityBonusPct(pWeapons[ i ]);
	}
	fBonusPercent += pOwner ? sMissileGetVelocityBonusPct(pOwner) : 0;

	if(fBonusPercent != 0.0f)
	{
		fVelocity += fVelocity * fBonusPercent;
	}

	ASSERT( fVelocity == 0.0f || fVelocityMax != 0.0f );
	if ( fVelocity > fVelocityMax )
		fVelocity = fVelocityMax;
	if ( fVelocity < fVelocityMin )
		fVelocity = fVelocityMin;
	
	return fVelocity;
}

static BOOL sRemoveIgnoreCollision(
	GAME* pGame,
	UNIT* pUnit,
	const EVENT_DATA& event_data)
{
	if( !pUnit )
		return FALSE;
	UnitSetFlag( pUnit, UNITFLAG_IGNORE_COLLISION, FALSE );
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSetMissileToHoming(
	GAME* pGame,
	UNIT* pUnit,
	const EVENT_DATA& event_data)
{
	if( !pUnit )
		return FALSE;
	
	if( UnitGetAITarget( pUnit ) == NULL )
		return FALSE;

	MissileSetHoming( pUnit, 0, 0.0f );
	UnitSetStat( pUnit, STATS_HOMING_TARGET_ID, UnitGetId( UnitGetAITarget( pUnit ) ) );
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const struct UNIT_DATA * MissileGetData(
	GAME * game,
	UNIT * unit)
{
	ASSERT_RETNULL(unit);
	ASSERT(UnitGetGenus(unit) == GENUS_MISSILE);
	return MissileGetData(game, UnitGetClass(unit));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT * MissileInit(
	GAME * pGame,
	int nClass,
	UNITID id,
	UNIT * pOwner,
	UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT ],
	int nSkill,
	int nTeam,
	ROOM * room,
	const VECTOR & vPosition,
	const VECTOR & vMoveDirectionIn,
	const VECTOR & vFaceDirection,
	const BOOL * pbElementEffects /*= NULL*/)
{
	//ASSERT_RETNULL(pGame && room && nClass >= 0); // this is turned off because we are sending extra units to the client
	if ( IS_CLIENT( pGame ) )
	{
		if ( !room  )
			return NULL;
	} else {
		ASSERT_RETNULL( room )
	}
	ASSERT_RETNULL(pGame && nClass >= 0);
/*
	trace("MissileInit (%c): (%8.2f, %8.2f, %8.2f)  (%d)\n",
		IS_SERVER(pGame) ? 's' : 'c'),
		pvPosition->fX, pvPosition->fY, pvPosition->fZ, nAngle);
*/
	UNIT_DATA * missile_data = (UNIT_DATA *)ExcelGetData(pGame, DATATABLE_MISSILES, nClass);
	if (!missile_data)
	{
		return NULL;
	}
	if ( UnitDataTestFlag( missile_data, UNIT_DATA_FLAG_SERVER_ONLY ) && !IS_SERVER(pGame))
	{
		return NULL;
	}
	if ( UnitDataTestFlag( missile_data, UNIT_DATA_FLAG_CLIENT_ONLY ) && !IS_CLIENT(pGame))
	{
		return NULL;
	}
	if (!pOwner)
	{
		return NULL;
	}
	if ( UnitDataTestFlag( missile_data, UNIT_DATA_FLAG_MISSILE_IS_GORE ) && AppTestFlag( AF_CENSOR_PARTICLES ) )
	{
		return NULL;
	}

	UnitDataLoad( pGame, DATATABLE_MISSILES, nClass );

	UNIT * pWeaponsToUse[MAX_WEAPONS_PER_UNIT];
	// Fixup weapons
	if(!pWeapons && UnitIsA(pOwner, UNITTYPE_MISSILE))
	{
		UnitGetWeaponsTag( pOwner, TAG_SELECTOR_MISSILE_SOURCE, pWeaponsToUse );
		pWeapons = pWeaponsToUse;
	}

	VECTOR vMoveDirection = vMoveDirectionIn;	
	float fVelocity = MissileGetFireSpeed(pGame, missile_data, pOwner, pWeapons, vMoveDirectionIn);
	
	UNIT_CREATE newunit( missile_data );
	newunit.species = MAKE_SPECIES(GENUS_MISSILE, nClass);
	newunit.unitid = id;
	newunit.pRoom = room;
	newunit.vPosition = vPosition;
	newunit.vFaceDirection = vFaceDirection;
	newunit.nAngle = 0;
	newunit.fAcceleration = missile_data->fAcceleration;
	if ( UnitDataTestFlag( missile_data, UNIT_DATA_FLAG_USE_SOURCE_APPEARANCE ) && pWeapons && pWeapons[ 0 ] )
	{
		newunit.nAppearanceDefId = UnitGetAppearanceDefId( pWeapons[ 0 ] );
		newunit.pSourceData = UnitGetData( pWeapons[0] );
	}
	if (missile_data->fScale != 0.0f && missile_data->fScaleDelta != 0.0f)
	{
		newunit.fScale = RandGetFloat(pGame, missile_data->fScale, missile_data->fScale + missile_data->fScaleDelta);
	}
#if ISVERSION(DEBUG_ID)
	PStrPrintf(newunit.szDbgName, MAX_ID_DEBUG_NAME, "%s", missile_data->szName);
#endif

	if (!UnitDataTestFlag( missile_data, UNIT_DATA_FLAG_SYNC ) && IS_CLIENT(pGame))
	{
		SET_MASK(newunit.dwUnitCreateFlags, UCF_CLIENT_ONLY);	
	}

	UNIT * pUnit = UnitInit(pGame, &newunit);
	if (!pUnit)
	{
		return NULL;
	}

	// set elemental effects (if any)
	if (pbElementEffects != NULL)
	{
		for (int i = 0; i < NUM_DAMAGE_TYPES; ++i)
		{
			if (pbElementEffects[ i ] == TRUE)
			{
				UnitSetStat( pUnit, STATS_MISSILE_MUZZLE_TYPE, i, TRUE);
				UnitSetStat( pUnit, STATS_MISSILE_TRAIL_TYPE, i, TRUE);
				UnitSetStat( pUnit, STATS_MISSILE_PROJECTILE_TYPE, i, TRUE);
				UnitSetStat( pUnit, STATS_MISSILE_IMPACT_TYPE, i, TRUE);
			}
		}	
	}
		
	
	if( pUnit->pUnitData->flSecondsBeforeBecomingCollidable > 0.0f )
	{
		UnitSetFlag( pUnit, UNITFLAG_IGNORE_COLLISION, TRUE );
		int ticks = (int)( pUnit->pUnitData->flSecondsBeforeBecomingCollidable * GAME_TICKS_PER_SECOND_FLOAT );
		UnitRegisterEventTimed( pUnit, sRemoveIgnoreCollision, EVENT_DATA(), ticks);
	}
	if( pUnit->pUnitData->flHomingAfterXSeconds > 0.0f )
	{
		int ticks = (int)( pUnit->pUnitData->flHomingAfterXSeconds * GAME_TICKS_PER_SECOND_FLOAT );
		UnitRegisterEventTimed( pUnit, sSetMissileToHoming, EVENT_DATA(  ), ticks);
	}

	// not a set length / straight line bullet. Needs to collide with background
	UnitSetFlag(pUnit, UNITFLAG_RAYTESTCOLLISION, !UnitDataTestFlag( missile_data, UNIT_DATA_FLAG_NO_RAY_COLLISION ));
	UnitSetFlag(pUnit, UNITFLAG_AI_INIT, FALSE);
	UnitSetFlag(pUnit, UNITFLAG_KNOCKSBACK_WITH_MOVEMENT, TRUE);

	// set a flag to skip the first step of movement. This is because the step list is processed after
	// the pUnit is created, so we want it to look like it comes out of the end of the gun on first draw
	// rather than stepping forward first.
	UnitSetFlag(pUnit, UNITFLAG_SKIPFIRSTSTEP);

	UNIT * pOwnerToAssign = pOwner;
	{ // we can't have missiles owning missiles - it messes up exp when units in the middle are removed which frequently happens with missiles
		UNIT * pOwnerPrev = NULL;
		int nReps = 0;	//Prevent more complex 2 unit infinite loop.
		while( pOwnerToAssign && UnitIsA( pOwnerToAssign, UNITTYPE_MISSILE ) )
		{
			pOwnerPrev = pOwnerToAssign;
			pOwnerToAssign = UnitGetOwnerUnit( pOwnerToAssign );
			if ( pOwnerPrev == pOwnerToAssign )
				break; // prevent an infinite loop
			nReps++;
			ASSERTX_BREAK(nReps < 1000, "Infinite loop in missile ownership");
		}
		if ( ! pOwnerToAssign )
			pOwnerToAssign = pOwner;
	}
	UnitSetOwner(pUnit, pOwnerToAssign);

	if ( pOwnerToAssign )
	{ // the missile needs a level to help some scripts create the proper balance
		UnitSetExperienceLevel( pUnit, UnitGetExperienceLevel( pOwnerToAssign ) );
	}
	// we have some items ( like gold ) that fire 'visual' missiles that really have no team - make 'em neutral.
	if( nTeam == INVALID_TEAM )
	{
		nTeam = TEAM_NEUTRAL;
	}
	TeamAddUnit(pUnit, nTeam);

	for (int ii = 0; ii < NUM_UNIT_PROPERTIES; ii++)
	{
		ExcelEvalScript(pGame, pUnit, NULL, NULL, DATATABLE_MISSILES, (DWORD)OFFSET(UNIT_DATA, codeProperties[ii]), nClass);
	}

	pUnit->dwTestCollide = TARGETMASK_GOOD | TARGETMASK_BAD; // TODO: make this mask properly

	if( AppIsTugboat() )
	{
		pUnit->dwTestCollide = pUnit->dwTestCollide | TARGETMASK_OBJECT;
	}

	VECTOR vVelocity;
	VectorScale(vVelocity, vMoveDirection, fVelocity);
 
	UnitRegisterEventHandler(pGame, pUnit, EVENT_DIDCOLLIDE,	sMissileHandleCollided, NULL);
	if (IS_CLIENT( pGame ))
	{
		UnitRegisterEventHandler(pGame, pUnit, EVENT_ON_FREED,	c_sMissileHandleFreed,	NULL);
	}
	UnitRegisterEventHandler(pGame, pUnit, EVENT_HITENDRANGE,	sMissileHitEndRange,	NULL);
	UnitRegisterEventHandler(pGame, pUnit, EVENT_OUT_OF_BOUNDS,	sMissileHitEndRange,	NULL);

	if (pWeapons)
	{
		UnitAddWeaponsTag( pUnit, TAG_SELECTOR_MISSILE_SOURCE, pWeapons );
	}

	if( pWeapons )
	{
		StateSetAttackStart( pWeapons[0], NULL, pUnit, false );
	}
	else
	{
		StateSetAttackStart( NULL, NULL, pUnit, false );
	}


	// this is specifically being used for minions that fire missiles and die before their missiles
	// reach their target, resulting in a lapse of real ownership
	if (pOwner && IS_SERVER(pGame) && UnitDataTestFlag( missile_data, UNIT_DATA_FLAG_MISSILE_USE_ULTIMATEOWNER ) )
	{
		pOwner = UnitGetUltimateOwner( pUnit );
		UnitSetOwner( pUnit, pOwner );
	}


#if !ISVERSION(CLIENT_ONLY_VERSION)
	// assign missile damage properties here
	if(IS_SERVER(pGame))
	{
		int nDamageIncrement = missile_data->nServerSrcDamage;
		if (pOwner && nDamageIncrement > 0)
		{
			UnitTransferDamages(pUnit, pOwner, pWeapons, 0, !UnitDataTestFlag(missile_data, UNIT_DATA_FLAG_DONT_TRANSFER_RIDERS_FROM_OWNER), UnitDataTestFlag(missile_data, UNIT_DATA_FLAG_IS_NONWEAPON_MISSILE));
			if ( nDamageIncrement != 100 )
			{
				int nCurr = UnitGetStat( pUnit, STATS_DAMAGE_INCREMENT );
				int nNew = nCurr ? ( nCurr * nDamageIncrement ) / 100 : nDamageIncrement;
				UnitSetStat(pUnit, STATS_DAMAGE_INCREMENT, nNew );
			}
		}
	}
#endif // #if !ISVERSION(CLIENT_ONLY_VERSION)
	if( IS_CLIENT(pGame) && !UnitDataTestFlag(missile_data, UNIT_DATA_FLAG_DONT_TRANSFER_DAMAGES_ON_CLIENT))
	{
		UnitTransferDamages(pUnit, pOwner, pWeapons, 0, !UnitDataTestFlag(missile_data, UNIT_DATA_FLAG_DONT_TRANSFER_RIDERS_FROM_OWNER));
	}
#ifdef HAVOK_ENABLED
	if (pUnit->pHavokRigidBody)
	{
		HavokRigidBodySetVelocity(pUnit->pHavokRigidBody, &vVelocity);
		if (UnitDataTestFlag( missile_data, UNIT_DATA_FLAG_HITS_BACKGROUND ))
		{
			HavokRigidBodySetHitFunc(pGame, pUnit, pUnit->pHavokRigidBody, sHandleMissileHavokHit, NULL);
		}
	}
#endif

	UnitSetVelocity( pUnit, fVelocity );

	UnitSetDefaultTargetType(pGame, pUnit, missile_data);

	if (fVelocity != 0.0f || UnitDataTestFlag( pUnit->pUnitData, UNIT_DATA_FLAG_ALWAYS_CHECK_FOR_COLLISIONS ) )
	{
		UnitSetMoveDirection(pUnit, vMoveDirection);
		UnitStepListAdd(pGame, pUnit);
	}

	if(missile_data->fJumpVelocity)
	{
		UnitStartJump(pGame, pUnit, missile_data->fJumpVelocity, FALSE, FALSE);
	}

	if (!UnitDataTestFlag( missile_data, UNIT_DATA_FLAG_SYNC ) || IS_SERVER(pGame))
	{
		int nPeriod = UnitGetStat(pUnit, STATS_AI_PERIOD);
		if (UnitGetStat(pUnit, STATS_AI) != 0)
		{
			WARNX(nPeriod, "AI period set to 0 - defaulting to 10");
			if (nPeriod == 0)
			{
				nPeriod = 10;
				UnitSetStat(pUnit, STATS_AI_PERIOD, nPeriod);
			}
			AI_Init(pGame, pUnit);
		}
	}

	// do this before saving the missile for the skill
	// and make sure to do it before making the new particle system
	if ( UnitDataTestFlag( missile_data, UNIT_DATA_FLAG_DESTROY_OTHER_MISSILES ) && pOwner )
	{
		UnitTriggerEvent( pGame, EVENT_SKILL_DESTROYING_MISSILES, pOwnerToAssign, pWeapons ? pWeapons[ 0 ] : NULL, &EVENT_DATA( nSkill, pWeapons ? UnitGetId( pWeapons[ 0 ] ) : INVALID_ID ) );
	}


	if (IS_CLIENT(pGame))
	{
		c_UnitUpdateGfx( pUnit ); // we need this so that the model has the proper room
		ATTACHMENT_DEFINITION tAttachDef;
		if ( UnitGetVelocity( pUnit ) == 0.0f )
		{
			tAttachDef.vNormal = vMoveDirectionIn;
			tAttachDef.dwFlags |= ATTACHMENT_FLAGS_ABSOLUTE_NORMAL;
		}
		c_MissileEffectAttachToUnit(pGame, pUnit, NULL, nClass, MISSILE_EFFECT_PROJECTILE, tAttachDef, ATTACHMENT_OWNER_MISSILE, MISSILE_EFFECT_PROJECTILE);

		if (missile_data->m_nSound != INVALID_ID)
		{
			CLT_VERSION_ONLY(
				c_SoundPlay(missile_data->m_nSound, &vPosition, &SOUND_PLAY_EXINFO(&vVelocity) ) );
		}
		ASSERT(pUnit->pGfx);
	}


	if ( nSkill == INVALID_ID && pWeapons && pWeapons[ 0 ] )
		nSkill = ItemGetPrimarySkill( pWeapons[ 0 ] );	
	float fRangePercent = MissileGetRangePercent(pOwner, pWeapons ? pWeapons[0] : NULL, missile_data);
	if ( nSkill != INVALID_ID )
	{
		UnitSetStat( pUnit, STATS_AI_SKILL_PRIMARY, nSkill );
		const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkill );
		if(pSkillData)
		{
			if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_SAVE_MISSILES ) )
			{
				SkillSaveUnitForRemovalOnSkillStop( pGame, pOwnerToAssign, pUnit, pWeapons ? pWeapons[ 0 ] : NULL, nSkill );
			}
			if( pSkillData->nFuseMissilesOnState != INVALID_ID )
			{
				SkillSaveUnitForFuseOnStateClear( pGame, pOwnerToAssign, pUnit, pSkillData->nFuseMissilesOnState );
			}
			UnitSetStat(pUnit, STATS_FIELD_OVERRIDE, pSkillData->nFieldMissile);

			// TRAVIS: Making this server only - we are getting client asserts in multiplayer.
			int nSkillLevel = 0;
			if ( pOwnerToAssign )
			{
				nSkillLevel = SkillGetLevel( pOwnerToAssign, nSkill );
			}
			if (pOwner && IS_SERVER(pGame) && !UnitDataTestFlag( missile_data, UNIT_DATA_FLAG_MISSILE_IGNORE_POSTLAUNCH ) )
			{
				int codeLen1 = 0;
				BYTE * codePtr1 = ExcelGetScriptCode(pGame, DATATABLE_SKILLS, pSkillData->codeStatsServerPostLaunch, &codeLen1);

				int codeLen2 = 0;
				BYTE * codePtr2 = ExcelGetScriptCode(pGame, DATATABLE_SKILLS, pSkillData->codeStatsServerPostLaunchPostProcess, &codeLen2);
				
				if (codePtr1 || codePtr2)
				{
					STATS * stats = StatsListInit(pGame);
					if (codePtr1)
					{
						VMExecI(pGame, pUnit, pOwner, stats, nSkill, nSkillLevel, nSkill, nSkillLevel, INVALID_ID, codePtr1, codeLen1);
					}
					if (codePtr2)
					{
						VMExecI(pGame, pUnit, pOwner, stats, nSkill, nSkillLevel, nSkill, nSkillLevel, INVALID_ID, codePtr2, codeLen2);
					}
					StatsListAdd(pUnit, stats, FALSE);
				}
			}

			if (pOwner)
			{
				int code_len = 0;
				BYTE * code_ptr = ExcelGetScriptCode(pGame, DATATABLE_SKILLS, pSkillData->codeStatsPostLaunch, &code_len);
				struct STATS * pStatsList = NULL;
				if (code_ptr)
				{
					pStatsList = StatsListInit(pGame);
					VMExecI(pGame, pUnit, pStatsList, nSkill, nSkillLevel, nSkill, nSkillLevel, INVALID_ID, code_ptr, code_len);
					StatsListAdd(pUnit, pStatsList, FALSE);
				}
			}


			
			if( AppIsTugboat() )
			{
				//Tugboat adds missile ranges to specific missiles. So only the missile has the missile range correctly.
				fRangePercent *= MissileGetRangePercent(pUnit, NULL, missile_data);
			}
			if ( pOwnerToAssign )
			{
				int nSkillLevel = SkillGetLevel( pOwnerToAssign, nSkill );
				if ( pSkillData->fRangePercentPerLevel != 0.0f && nSkillLevel != 0 )
				{
					fRangePercent += (pSkillData->fRangePercentPerLevel * nSkillLevel) / 100.0f;
				}
			}

		}

	}


	float fRange = missile_data->fRangeBase * fRangePercent;
	const SKILL_DATA * pSkillData = SkillGetData( pGame, nSkill );
	if( SkillDataTestFlag( pSkillData, SKILL_FLAG_FIRE_TO_LOCATION ) )
	{
		VECTOR vDelta = UnitGetStatVector( pOwner, STATS_SKILL_TARGET_X, nSkill, 0 );			
		if( VectorIsZero( vDelta ) )
		{
			if( SkillDataTestFlag( pSkillData, SKILL_FLAG_FIRE_TO_DEFAULT_UNIT ) )
			{
				vDelta = UnitGetPosition( pOwner );
			}
			
		}
		vDelta -= vPosition;
		float fNewRange = VectorLength( vDelta );	
		fRange = ( fNewRange < fRange ) ? fNewRange : fRange;
		VectorNormalize( vDelta, vDelta );
		//vDelta *= fRange;
		vDelta = vMoveDirection * fRange;
		if( IS_CLIENT( pGame ) &&
			UnitDataTestFlag( missile_data, UNIT_DATA_FLAG_MISSILE_PLOT_ARC ) )
		{
			UNIT* pUltimateOwner = UnitGetUltimateOwner( pOwner );
			pUnit->m_pActivePath->Clear();
			pUnit->m_pActivePath->CPathAddPoint(vPosition, VECTOR(0, 0, 1));
			VECTOR vApex = vPosition + vDelta / 2.0f;
			vApex.fZ += RandGetFloat( pGame, missile_data->fMissileArcHeight, missile_data->fMissileArcHeight + missile_data->fMissileArcDelta );
			pUnit->m_pActivePath->CPathAddPoint(vApex, VECTOR(0, 0, 1));
			vApex = vPosition + vDelta;
			vApex.fZ = pUltimateOwner->vPosition.fZ;
			pUnit->m_pActivePath->CPathAddPoint(vApex, VECTOR(0, 0, 1));
		}
	}
	else
	{

		if( IS_CLIENT( pGame ) &&
			UnitDataTestFlag( missile_data, UNIT_DATA_FLAG_MISSILE_PLOT_ARC ) )
		{
			VECTOR vDelta = vMoveDirection * fRange;

			pUnit->m_pActivePath->Clear();
			pUnit->m_pActivePath->CPathAddPoint(vPosition, VECTOR(0, 0, 1));
			VECTOR vApex = vPosition + vDelta / 2.0f;
			vApex.fZ += RandGetFloat( pGame, missile_data->fMissileArcHeight, missile_data->fMissileArcHeight + missile_data->fMissileArcDelta );
			pUnit->m_pActivePath->CPathAddPoint(vApex, VECTOR(0, 0, 1));
			vDelta += vPosition;
			if( pOwner )
			{
				vDelta.fZ = pOwner->vPosition.fZ;
			}
			pUnit->m_pActivePath->CPathAddPoint(vDelta, VECTOR(0, 0, 1));
		}

	}

	if( IS_SERVER( pGame ) &&
		pUnit &&
		pWeapons &&
		pWeapons[ 0 ] )
	{
		//this was added because our missiles fire from the center of the player. But when they fire a weapon the visual is fired
		//from the muzzle. And with things like Rifles, it's a huge difference. This has problems with trying to synch server and client.
		fRange += pWeapons[ 0 ]->pUnitData->fServerMissileOffset;
	}

	UnitSetStatFloat(pUnit, STATS_MISSILE_DISTANCE, 0, fRange);
	UnitSetStatFloat(pUnit, STATS_MISSILE_DISTANCE, 1, fRange);

	if( AppIsTugboat() )
	{
		
		fVelocity = MissileGetFireSpeed(pGame, missile_data, pUnit, pWeapons, vMoveDirectionIn);
		//this works because the velocity overwrites the missile speed set by the player
		//This can be different because the missile can get speed increases from scripts.
		UnitSetVelocity( pUnit, fVelocity );
	}

	for(int i=0; i<NUM_INIT_SKILLS; i++)
	{
		if (missile_data->nInitSkill[i] != INVALID_ID)
		{
			DWORD dwFlags = 0;
			if ( IS_SERVER( pGame ) )
				dwFlags |= SKILL_START_FLAG_INITIATED_BY_SERVER;
			SkillStartRequest(pGame, pUnit, missile_data->nInitSkill[i], INVALID_ID, VECTOR(0), dwFlags, SkillGetNextSkillSeed(pGame));
		}
	}

	UnitClearFlag(pUnit, UNITFLAG_JUSTCREATED);

#if !(ISVERSION(SERVER_VERSION) && !ISVERSION(DEVELOPMENT))		// don't assert on live servers to reduce server spam
	ASSERTX( VectorIsZero( pUnit->vPosition ) == FALSE, missile_data->szName );
#endif

	UnitPostCreateDo(pGame, pUnit);

	return pUnit;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT* MissileInitOnUnit(
	GAME* pGame,
	int nClass,
	UNITID id,
	UNIT * pOwner,
	UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT ],
	int nSkill,
	UNIT * pTargetUnit,
	int nTeam,
	ROOM* room )
{
	ASSERT_RETNULL( IS_CLIENT( pGame ) );

	if ( ! pTargetUnit )
		return NULL;

	int nTargetModelId = c_UnitGetModelId( pTargetUnit );
	VECTOR vPosition;
#ifdef HAVOK_ENABLED
	BOOL bRagdollPos = c_RagdollGetPosition( nTargetModelId, vPosition );
#else
	BOOL bRagdollPos = FALSE;
#endif
	if ( ! bRagdollPos || VectorIsZero( vPosition ) )
	{
		vPosition = UnitGetAimPoint( pTargetUnit );
	}

	VECTOR vDirection( 0, 0, 1 );
	UNIT * pMissile = MissileInit( pGame, nClass, id, pOwner, pWeapons, nSkill, nTeam, room, vPosition, vDirection, vDirection );

	if ( ! pMissile )
		return NULL;

	const UNIT_DATA * pMissileData = UnitGetData( pMissile );
	if ( UnitDataTestFlag( pMissileData, UNIT_DATA_FLAG_STICK_ON_INIT ) && pOwner )
	{
		{ // attach the missile to the target
			VECTOR vSourcePos = UnitGetCenter( pOwner );
			int nWeaponIndex = WardrobeGetWeaponIndex( pOwner, pWeapons ? UnitGetId( pWeapons[ 0 ] ) : INVALID_ID );
			VECTOR vDirection;
			if ( ! UnitGetWeaponPositionAndDirection( pGame, pOwner, nWeaponIndex, NULL, &vDirection ) )
			{
				vDirection = VECTOR( 0.0f, 1.0f, 0.0f );
			}

			ATTACHMENT_DEFINITION tAttachDef;
			if ( ! c_AppearanceInitAttachmentOnMesh ( pGame, nTargetModelId, tAttachDef, vSourcePos, vDirection ) )
			{
				tAttachDef.vPosition = UnitGetCenter( pTargetUnit ) - UnitGetPosition( pTargetUnit );
			}
			int nMissileModelId = c_UnitGetModelId( pMissile );
			UnitSetFlag( pMissile, UNITFLAG_DONT_SET_WORLD, TRUE );
			c_ModelAttachmentAdd( nTargetModelId, tAttachDef, "", TRUE, ATTACHMENT_OWNER_MISSILE, UnitGetId( pMissile ), nMissileModelId );
		}

		ATTACHMENT_DEFINITION tAttachDef;
		e_AttachmentDefInit( tAttachDef );
		tAttachDef.vNormal = VECTOR( -1, 0, 0 );
		tAttachDef.eType = ATTACHMENT_PARTICLE;
		PStrCopy( tAttachDef.pszBone, MUZZLE_BONE_DEFAULT, MAX_XML_STRING_LENGTH );

		if ( pWeapons && pWeapons[ 0 ] )
		{
			c_AttachmentSetWeaponIndex ( pGame, tAttachDef, pOwner, UnitGetId( pWeapons[ 0 ] ) );
		}

		int nSkill = pWeapons ? ItemGetPrimarySkill( pWeapons[ 0 ] ) : INVALID_ID;

		int nTrail = c_MissileEffectAttachToUnit( pGame, pOwner, pWeapons ? pWeapons[ 0 ] : NULL, 
												UnitGetClass( pMissile ), MISSILE_EFFECT_TRAIL, tAttachDef, 
												ATTACHMENT_OWNER_SKILL, nSkill );

		if ( nTrail != INVALID_ID )
		{
			e_AttachmentDefInit( tAttachDef );
			tAttachDef.eType = ATTACHMENT_PARTICLE_ROPE_END;

			int nMissileModelId = c_UnitGetModelId( pMissile );
			ASSERT_RETFALSE( nMissileModelId != INVALID_ID );

			c_ModelAttachmentAdd( nMissileModelId, tAttachDef, "", FALSE, ATTACHMENT_OWNER_SKILL, nSkill, nTrail );
		}
	}

	sHitDoSkill(pGame, pMissile, pTargetUnit, pMissileData, pMissileData->nSkillIdHitUnit );

	return pMissile;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void s_sSendMissileToClients(
	UNIT *pMissile,
	UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT ],
	UNITID idTarget,
	int nSkill,
	const VECTOR *pvPosition,
	const VECTOR *pvMoveDirection)
{
	ASSERTX_RETURN( pMissile, "Expected unit" );
	ASSERTX_RETURN( UnitIsA( pMissile, UNITTYPE_MISSILE ), "Expected missile" );
	GAME *pGame = UnitGetGame( pMissile );
	ASSERTX_RETURN( IS_SERVER( pGame ), "Server only" );	
	const UNIT_DATA *pMissileData = MissileGetData( pGame, pMissile );
	ASSERTX_RETURN(pMissileData, "Expected missile data");
		
	UNIT *pOwner = UnitGetUltimateOwner( pMissile );
	UNITID idSource = pOwner ? UnitGetId( pOwner ) : INVALID_ID;

	// missile is now ready to send messages
	UnitSetCanSendMessages( pMissile, TRUE );

	// where is the missile	
	ROOM * pRoom = UnitGetRoom( pMissile );
	ASSERTX_RETURN( pRoom, "Synchronized missile is not in any room" );
	
	// setup message for clients
	BYTE bCommand = 0;
	MSG_STRUCT *pMessage = NULL;	
	MSG_SCMD_MISSILENEWONUNIT tMessageNewOnUnit;
	MSG_SCMD_MISSILENEWXYZ tMessageNewXYZ;			
	if (UnitDataTestFlag( pMissileData, UNIT_DATA_FLAG_STICK_ON_INIT ) && idTarget != INVALID_ID)
	{

		// setup message
		tMessageNewOnUnit.nClass = UnitGetClass( pMissile );;
		tMessageNewOnUnit.id = UnitGetId( pMissile );
		tMessageNewOnUnit.idSource = idSource;
		ASSERT( MAX_WEAPONS_PER_UNIT == 2 );
		tMessageNewOnUnit.idWeapon0 = pWeapons ? UnitGetId( pWeapons[ 0 ] ) : INVALID_ID;
		tMessageNewOnUnit.idWeapon1 = pWeapons ? UnitGetId( pWeapons[ 1 ] ) : INVALID_ID;
		tMessageNewOnUnit.nTeam = UnitGetTeam( pMissile );
		tMessageNewOnUnit.nSkill = (WORD)nSkill;
		tMessageNewOnUnit.idRoom = RoomGetId( pRoom );
		tMessageNewOnUnit.idTarget = idTarget;

		// save this as the message to use
		pMessage = &tMessageNewOnUnit;
		bCommand = SCMD_MISSILENEWONUNIT;

	} 
	else 
	{

		// setup message
		tMessageNewXYZ.nClass = UnitGetClass( pMissile );;
		tMessageNewXYZ.id = UnitGetId( pMissile );
		tMessageNewXYZ.idSource = idSource;
		ASSERT( MAX_WEAPONS_PER_UNIT == 2 );
		tMessageNewXYZ.idWeapon0 = pWeapons ? UnitGetId( pWeapons[ 0 ] ) : INVALID_ID;
		tMessageNewXYZ.idWeapon1 = pWeapons ? UnitGetId( pWeapons[ 1 ] ) : INVALID_ID;
		tMessageNewXYZ.nTeam = UnitGetTeam( pMissile );
		tMessageNewXYZ.nSkill = (WORD)nSkill;
		tMessageNewXYZ.idRoom = RoomGetId( pRoom );

		ASSERTX( pvPosition, "Expected position" );
		tMessageNewXYZ.position = *pvPosition;

		ASSERTX( pvMoveDirection, "Expected move direction" );
		tMessageNewXYZ.MoveDirection = *pvMoveDirection;

		if(UnitDataTestFlag(pMissileData, UNIT_DATA_FLAG_XFER_MISSILE_STATS))
		{
			BIT_BUF tBuffer(tMessageNewXYZ.tBuffer, MAX_NEW_UNIT_BUFFER);
			XFER<XFER_SAVE> tXfer(&tBuffer);
			StatsXfer(pGame, UnitGetStats(pMissile), tXfer, STATS_WRITE_METHOD_NETWORK);
			int nMessageSize = tBuffer.GetLen();
			MSG_SET_BLOB_LEN(tMessageNewXYZ, tBuffer, nMessageSize);
		}
		else
		{
			int nMessageSize = 0;
			ZeroMemory(&tMessageNewXYZ.tBuffer, sizeof(tMessageNewXYZ.tBuffer));
			MSG_SET_BLOB_LEN(tMessageNewXYZ, tBuffer, nMessageSize);
		}

		// save this as the message to use
		pMessage = &tMessageNewXYZ;
		bCommand = SCMD_MISSILENEWXYZ;

	}

	// setup a structure for missile data ... I know this is a little messy, but I
	// really want to pipe all send units to clients through a single function -Colin
	NEW_MISSILE_MESSAGE_DATA tNewMissileMessageData;
	tNewMissileMessageData.bCommand = bCommand;
	tNewMissileMessageData.pMessage = pMessage;

	// send to all clients who care
	for (GAMECLIENT *pClient = ClientGetFirstInGame( pGame ); 
		 pClient;
		 pClient = ClientGetNextInGame( pClient ))
	{
		if (ClientIsRoomKnown( pClient, pRoom ))
		{
			s_SendUnitToClient( pMissile, pClient, 0, &tNewMissileMessageData );			
		}
	}
	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void MissileGetReflectsAndRetargets(
	UNIT * pMissile,
	UNIT * pOwner,
	BOOL & bReflects,
	BOOL & bRetargets,
	int & nRicochetCount )
{
	ASSERT_RETURN( pMissile );
	ASSERT_RETURN( pOwner );

	RAND tRand;
	RandInit( tRand, UnitGetStat( pMissile, STATS_SKILL_SEED ) );

	{
		int nChance = UnitGetStat( pOwner, STATS_CHANCE_THAT_MISSILES_REFLECT );
		if ( !nChance )
			bReflects = FALSE;
		else if ( (int) RandGetNum( tRand, 100 ) < nChance )
		{
			bReflects = TRUE;
			nRicochetCount = 1;
		}
		else
			bReflects = FALSE;
	}
	{
		int nChance = UnitGetStat( pOwner, STATS_CHANCE_THAT_MISSILES_RETARGET );
		if ( !nChance )
			bRetargets = FALSE;
		else if ( (int) RandGetNum( tRand, 100 ) < nChance )
		{
			bRetargets = TRUE;
			nRicochetCount = 2;
			if(bReflects)
			{
				nRicochetCount++;
			}
		}
		else
			bRetargets = FALSE;
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT * MissileFire(
	GAME * pGame,
	int nMissileClass,
	UNIT * pOwner,
	UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT ],
	int nSkill,
	UNIT * pTarget,
	const VECTOR & vTarget,
	const VECTOR & vPositionIn,
	const VECTOR & vMoveDirectionIn,
	DWORD dwRandSeed,
	DWORD dwMissileTag,
	DWORD dwMissileFlags /*= 0*/,
	const BOOL *pbElementEffects /*= NULL*/,
	int nMissileNumber /*= 0*/)
{ 
	const UNIT_DATA * missile_data = MissileGetData(pGame, nMissileClass);
	ASSERT_RETNULL(missile_data);
		
	// get skill data if we're firing a missile as part of a skill
	const SKILL_DATA * skill_data = NULL;
	if (nSkill != INVALID_LINK)
	{
		skill_data = SkillGetData(pGame, nSkill);
	}

	if (UnitDataTestFlag( missile_data, UNIT_DATA_FLAG_SYNC ) && IS_CLIENT(pGame))
	{
		return NULL;
	}

	// not sure why, but sometimes we can get a zero move direction
	VECTOR vMoveDirection = vMoveDirectionIn;
	if ( VectorIsZero ( vMoveDirection ) ) 
	{
		vMoveDirection.fY = 1.0f;
	}
	
	// get target room (if any)
	ROOM *pMissileRoom = UnitGetRoom( pOwner );
	
	// what position to put missile at (when firing from a missile we always use the in provided
	// instead of positions stored as a stat in the owner when they launched the skill)
	VECTOR vPosition;
	if (skill_data != NULL &&
		SkillDataTestFlag( skill_data, SKILL_FLAG_TARGET_POS_IN_STAT ) &&
		TESTBIT( dwMissileFlags, MF_FROM_MISSILE_BIT ) == FALSE)
	{
		vPosition = UnitGetStatVector( pOwner, STATS_SKILL_TARGET_X, nSkill, 0 );			
	}
	else 
	{
		vPosition = vPositionIn;
		// what room is vPositionIn inside?!!! is it ok to leave as room of owner??!?!? - colin
	}
	
	if (pTarget && UnitDataTestFlag( missile_data, UNIT_DATA_FLAG_STICK_ON_INIT ))
	{
		vPosition = UnitGetPosition( pTarget );
		if(TESTBIT( dwMissileFlags, MF_PLACE_ON_CENTER_BIT ) == TRUE)
		{
			vPosition.fZ += UnitGetCollisionHeight(pTarget) / 2.0f;
		}
	}

	{
		ROOM * pRoomFromSearch = RoomGetFromPosition( pGame, pMissileRoom, &vPosition );
		if ( pRoomFromSearch )
			pMissileRoom = pRoomFromSearch;
	}
	
	// create missile
	UNIT * missile = MissileInit(
		pGame, 
		nMissileClass, 
		INVALID_ID, 
		pOwner, 
		pWeapons, 
		nSkill, 
		UnitGetTeam( pOwner ),
		pMissileRoom, 
		vPosition, 
		vMoveDirection, 
		vMoveDirection,
		pbElementEffects );
		
	if (!missile)
	{
		return NULL;
	}
	if (missile_data->nMissileTag && dwMissileTag)
	{
		UnitSetFlag(missile, UNITFLAG_MISSILETAG);
		UnitSetStat(missile, STATS_MISSILE_TAG, dwMissileTag, missile_data->nMissileTag);
	}
	UnitSetStat( missile, STATS_SKILL_SEED, 0, dwRandSeed );

	UNITID idTarget = pTarget ? UnitGetId( pTarget ) : INVALID_ID;

	int nAI = UnitGetStat( missile, STATS_AI );
	// in tugboat, missiles can have very specific targets
	// that are needed in the collision check to prioritize them
	if ( nAI || AppIsTugboat() )
	{
		if ( idTarget != INVALID_ID )
		{
			UnitSetAITarget(missile, idTarget);
		}
		if ( ! VectorIsZero( vTarget ) )
		{
			UnitSetStatVector( missile, STATS_AI_TARGET_X, 0, vTarget );
		}
	}
	if (missile_data->nSkillIdHitUnit != INVALID_ID)
	{
		if ( idTarget != INVALID_ID )
		{
			SkillSetTarget( missile, missile_data->nSkillIdHitUnit, INVALID_ID, idTarget );
		}
		if ( ! VectorIsZero( vTarget ) )
		{
			UnitSetStatVector( missile, STATS_SKILL_TARGET_X, missile_data->nSkillIdHitUnit, 0, vTarget );
		}
		UnitSetStat( missile, STATS_SKILL_SEED, missile_data->nSkillIdHitUnit, dwRandSeed );
	}

	if (UnitIsDecoy( pOwner ))
	{
		UnitSetFlag(missile, UNITFLAG_IS_DECOY);
	}

	// enable reflective qualities if possible
	int nReflectiveFuseTimeInTicks = -1;
	UNIT *pUltimateOwner = UnitGetUltimateOwner( pOwner );

	BOOL bIsReflective = FALSE;
	BOOL bBulletsRetarget = FALSE;
	int nMaxRicochetCount = UnitGetStat( missile, STATS_MISSILE_MAX_RICOCHET_COUNT );
	MissileGetReflectsAndRetargets( missile, pOwner, bIsReflective, bBulletsRetarget, nMaxRicochetCount );

	BOOL bRicochetWalls = nMaxRicochetCount > 0;
	if ( ( bIsReflective || 
		   bRicochetWalls ) &&
		skill_data && 
		skill_data->flReflectiveLifetimeInSeconds > 0.0f &&
		TESTBIT( missile_data->dwBounceFlags, BF_CANNOT_RICOCHET_BIT ) == FALSE)
	{
	
		// reflective bullets have a fuse so they don't go forever
		nReflectiveFuseTimeInTicks = (int)(skill_data->flReflectiveLifetimeInSeconds * GAME_TICKS_PER_SECOND);
		
		// set the stats that make them reflective
		UnitSetStat( missile, STATS_BOUNCE_ON_HIT_UNIT, bIsReflective );  //reflective or retarget if true ricochets off everything
		UnitSetStat( missile, STATS_BOUNCE_ON_HIT_BACKGROUND, TRUE );	  //ricochet reflects off all walls
		UnitSetStat( missile, STATS_MISSILE_RICOCHET_COUNT, nMaxRicochetCount ); //put the count in the units stats.
	}

	if ( TESTBIT( missile_data->dwBounceFlags, BF_BOUNCE_ON_HIT_UNIT_BIT ) ||
		 TESTBIT( missile_data->dwBounceFlags, BF_BOUNCE_ON_HIT_BACKGROUND_BIT ) )
	{
		UnitSetStat( missile, STATS_MISSILE_RICOCHET_COUNT, -1 );
	}

	// set missile as homing if the owner has homing bullets on and this skill
	// allows homing bullets
	if (missile_data->flHomingTurnAngleInRadians != 0.0f &&
		UnitHasState( pGame, pUltimateOwner, STATE_HOMING_BULLETS))
	{
		MissileSetHoming( missile, nMissileNumber );
	}

	int nChanceToPenetrateShields = UnitGetStat( pOwner, STATS_CHANCE_THAT_MISSILES_PENETRATE_SHIELDS );
	if ( nChanceToPenetrateShields != 0 && (int)RandGetNum( pGame, 100 ) < nChanceToPenetrateShields )
	{
		UnitSetStat( missile, STATS_SHIELD_PENETRATION_PCT, 100 );
	}


	if(bBulletsRetarget)
	{
		UnitSetStat(missile, STATS_BOUNCE_RETARGET, TRUE);
	}

	// register a fuse event if any fuse time is specified	
	float fFuseTimer = sGetFuseTimer( missile );
	if ( fFuseTimer || nReflectiveFuseTimeInTicks != -1 )
	{
	
		// get the fuse time from excel
		int nFuseFromData = (int)(fFuseTimer * GAME_TICKS_PER_SECOND_FLOAT);
		if( fFuseTimer > 0.0f)
		{
			nFuseFromData = MAX(1, nFuseFromData);
		}

		// use the longer fuse time specified
		int nTicks = MAX(nFuseFromData, nReflectiveFuseTimeInTicks);
		
		// register event for fuse
		UnitRegisterEventTimed( missile, sHandleFuseMissileEvent, &EVENT_DATA(), nTicks);
		
	}

	if (missile_data->nDamageRepeatRate > 0)
	{
		BOOL bRegisterEvent = FALSE;
		if(IS_CLIENT(pGame) && missile_data->nSkillIdOnDamageRepeat >= 0)
		{
			bRegisterEvent = TRUE;
		}
		else if(IS_SERVER(pGame))
		{
			bRegisterEvent = TRUE;
		}
		if(bRegisterEvent)
		{
			int nRepeat = missile_data->nDamageRepeatRate;
			if(missile_data->bRepeatDamageImmediately)
			{
				nRepeat = 1;
			}
			UnitRegisterEventTimed( missile, sHandleRepeatDamageEvent, &EVENT_DATA(missile_data->bRepeatDamageImmediately), nRepeat);
		}
	}

	if ( UnitDataTestFlag( missile_data, UNIT_DATA_FLAG_SYNC ) )
	{
		s_sSendMissileToClients( missile, pWeapons, idTarget, nSkill, &vPosition, &vMoveDirection );
	}

	return missile;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void MissileChangeOwner(
	GAME * pGame,
	UNIT * pMissile,
	UNIT * pNewOwner,
	UNIT * pWeapon,
	DWORD dwFlags)
{
	ASSERT_RETURN(pGame);
	ASSERT_RETURN(pMissile);
	ASSERT_RETURN(pNewOwner);
	UNIT * pPreviousOwner = UnitGetOwnerUnit(pMissile);
	UnitSetOwner(pMissile, pNewOwner);
	TeamAddUnit(pMissile, UnitGetTeam(pNewOwner), FALSE);
	if( UnitGetTargetType( pNewOwner ) != TARGET_DEAD )
	{
		UnitChangeTargetType(pMissile, UnitGetTargetType(pNewOwner));
	}
	
	UNIT_TAG * pOldMissileSourceTag = UnitGetTag(pMissile, TAG_SELECTOR_MISSILE_SOURCE);
	if(pOldMissileSourceTag)
	{
		UnitRemoveTag(pMissile, pOldMissileSourceTag);
	}
	if(pWeapon)
	{
		UnitAddUnitIdTag( pMissile, TAG_SELECTOR_MISSILE_SOURCE, UnitGetId( pWeapon ) );
	}
	if(pPreviousOwner && TESTBIT(dwFlags, MCO_FIRE_AT_PREVIOUS_OWNER_BIT))
	{
		VECTOR vDirection = UnitGetCenter(pPreviousOwner) - UnitGetPosition(pMissile);
		VectorNormalize(vDirection);
		UnitSetMoveDirection(pMissile, vDirection);
		UnitSetAITarget(pMissile, pPreviousOwner);
		UnitSetStatVector(pMissile, STATS_AI_TARGET_X, 0, UnitGetPosition(pPreviousOwner));
	}
	if(!TESTBIT(dwFlags, MCO_DONT_RESET_RANGE_BIT))
	{
		const UNIT_DATA * pMissileData = UnitGetData(pMissile);
		UnitSetStatFloat(pMissile, STATS_MISSILE_DISTANCE, 0, pMissileData->fRangeBase);
		UnitUnregisterTimedEvent(pMissile, sHandleFuseMissileEvent);
		float fFuseIs = sGetFuseTimer( pMissile );
		if(fFuseIs > 0.0f)
		{
			int nTicks = (int)( fFuseIs * GAME_TICKS_PER_SECOND_FLOAT);
			nTicks = MAX(1, nTicks);
			UnitRegisterEventTimed( pMissile, sHandleFuseMissileEvent, &EVENT_DATA(), nTicks);
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void s_SendMissilesChangeOwner(
	UNIT * pNewOwner,
	UNIT * pWeapon,
	UNITID * pUnitIds,
	int nUnitIdCount,
	DWORD dwFlags)
{
	ASSERTX_RETURN( pNewOwner, "Expected unit" );
	ASSERTX_RETURN( pUnitIds, "Expected missile list" );

	if (UnitCanSendMessages( pNewOwner ) == FALSE)
	{
		return;
	}

	MSG_SCMD_MISSILES_CHANGEOWNER msg;	
	msg.idUnit = UnitGetId(pNewOwner);
	msg.idWeapon = UnitGetId(pWeapon);
	msg.nMissileCount = nUnitIdCount;
	for(int i=0; i<MAX_TARGETS_PER_QUERY; i++)
	{
		if(i < nUnitIdCount)
		{
			msg.nMissiles[i] = pUnitIds[i];
		}
		else
		{
			msg.nMissiles[i] = INVALID_ID;
		}
	}
#ifdef _DEBUG
	ASSERTX(!(dwFlags & ~0xFF), "Flags for Missiles Change Owner has too many bits to send!");
#endif
	msg.bFlags = (BYTE)dwFlags;

	s_SendUnitMessage( pNewOwner, SCMD_MISSILES_CHANGEOWNER, &msg );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sMissileAddParticleSystemToUnit( 
	GAME * pGame, 
	UNIT * pUnit, 
	int nParticleSystemDefId,
	ATTACHMENT_DEFINITION & tAttachDef,
	int nAttachmentOwnerType,
	int nAttachmentOwnerId,
	float fDuration,
	BOOL bForceNew = FALSE )
{
	if ( nParticleSystemDefId == INVALID_ID )
		return INVALID_ID;

	tAttachDef.eType = ATTACHMENT_PARTICLE;
	tAttachDef.nAttachedDefId = nParticleSystemDefId;
	tAttachDef.nBoneId		  = INVALID_ID;
	int nModelId = c_UnitGetModelId( pUnit );

	int nAttachmentId = c_ModelAttachmentAdd( nModelId, tAttachDef, "", bForceNew, 
										  nAttachmentOwnerType, nAttachmentOwnerId, INVALID_ID );

	nParticleSystemDefId = tAttachDef.nAttachedDefId;

	ATTACHMENT * pAttachment = NULL;
	if ( nAttachmentId != INVALID_ID )
	{
		pAttachment = c_ModelAttachmentGet( nModelId, nAttachmentId );
		if ( fDuration != 0.0f )
		{
			c_ParticleSystemSetParam( pAttachment->nAttachedId, PARTICLE_SYSTEM_PARAM_DURATION, fDuration );
		}
		c_ParticleSystemSetVelocity( pAttachment->nAttachedId, UnitGetVelocityVector( pUnit ) );
	} 
	return pAttachment ? pAttachment->nAttachedId : INVALID_ID;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL c_sAddElementToEffectList(
	int nElement,
	const PARTICLE_EFFECT_SET * pEffect,
	const UNIT_DATA * missile_data,
	BOOL * bHasElementType,
	int * pnParticleEffectIds,
	int & nParticleEffectCount)
{
	ASSERT_RETFALSE(missile_data);

	if (bHasElementType[nElement])
	{
		return TRUE;
	}

	BOOL bHasElement = FALSE;

	const PARTICLE_ELEMENT_EFFECT *pElementEffect = &pEffect->tElementEffect[ nElement ];
	
	// if we can tag it onto the end of the default, then do so
	if ( pElementEffect->nPerElementId != INVALID_ID )
	{
		bHasElementType[nElement] = TRUE;
		bHasElement = TRUE;
		pnParticleEffectIds[ nParticleEffectCount ] = pElementEffect->nPerElementId;
		nParticleEffectCount++;
	}

	return bHasElement;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL c_sAddElementsFromStat(
	UNIT * pUnit,
	WORD wStat,
	const PARTICLE_EFFECT_SET * pEffect,
	const UNIT_DATA * missile_data,
	BOOL * bHasElementType,
	int * pnParticleEffectIds,
	int & nParticleEffectCount)
{
	int num_damage_types = GetNumDamageTypes(AppGetCltGame());

	BOOL bHasElement = FALSE;
	STATS_ENTRY pStatsEntries[MAX_DAMAGE_TYPES];
	int nEffectTypes = UnitGetStatsRange( pUnit, wStat, 0, pStatsEntries, arrsize(pStatsEntries));
	for ( int i = 0; i < nEffectTypes; i++ )
	{
		int nElement = STAT_GET_PARAM( pStatsEntries[ i ].stat );

		ASSERT_RETFALSE(nElement >= 0 && nElement < num_damage_types);

		BOOL bHas = c_sAddElementToEffectList(nElement, pEffect, missile_data, bHasElementType, pnParticleEffectIds, nParticleEffectCount);
		if (!bHasElement)
		{
			bHasElement = bHas;
		}
	}
	return bHasElement;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_MissileGetParticleEffectDefs( 
	int nMissileClass)
{
	UNIT_DATA * missile_data = (UNIT_DATA *)ExcelGetDataNotThreadSafe(NULL, DATATABLE_MISSILES, nMissileClass);
	if(!missile_data)
		return;
	for(int eEffect=0; eEffect<NUM_MISSILE_EFFECTS; eEffect++)
	{
		c_sInitEffectSet(missile_data, (MISSILE_EFFECT)eEffect);		
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_MissileGetParticleEffectDefs( 
	GAME * pGame, 
	UNIT * pUnit, 
	int nMissileClass,
	MISSILE_EFFECT eEffect,
	int * pnParticleEffectIds,
	int & nParticleEffectCount)
{
	UnitDataLoad( pGame, DATATABLE_MISSILES, nMissileClass );
	ASSERT_RETURN(IS_CLIENT(pGame));

	UNIT_DATA * missile_data = (UNIT_DATA *)ExcelGetDataNotThreadSafe(EXCEL_CONTEXT(pGame), DATATABLE_MISSILES, nMissileClass);
	ASSERT_RETURN(missile_data);
	nParticleEffectCount = 0;
	ASSERT( eEffect < NUM_MISSILE_EFFECTS && eEffect >= 0 );
	PARTICLE_EFFECT_SET * pEffect = &missile_data->MissileEffects[eEffect];

	c_sInitEffectSet(missile_data, eEffect);		

	// add default effect (if any)
	if ( pEffect->nDefaultId != INVALID_ID )
	{
		pnParticleEffectIds[ nParticleEffectCount ] = pEffect->nDefaultId;
		nParticleEffectCount++;
	}

	static const WORD spwEffectToStatTable[ NUM_MISSILE_EFFECTS ] = 
	{
		STATS_MISSILE_MUZZLE_TYPE,		//MISSILE_EFFECT_MUZZLE,
		STATS_MISSILE_TRAIL_TYPE,		//MISSILE_EFFECT_TRAIL,
		STATS_MISSILE_PROJECTILE_TYPE,	//MISSILE_EFFECT_PROJECTILE,
		STATS_MISSILE_IMPACT_TYPE,		//MISSILE_EFFECT_IMPACT_WALL,
		STATS_MISSILE_IMPACT_TYPE,		//MISSILE_EFFECT_IMPACT_UNIT,
	};

	BOOL bHasElementType[MAX_DAMAGE_TYPES];
	memset(bHasElementType, FALSE, arrsize(bHasElementType) * sizeof(BOOL));

	BOOL bHasElement = FALSE;
	WORD wStat = spwEffectToStatTable[ eEffect ];
	bHasElement = c_sAddElementsFromStat(pUnit, wStat, pEffect, missile_data, bHasElementType, pnParticleEffectIds, nParticleEffectCount);

	if (eEffect == MISSILE_EFFECT_PROJECTILE)
	{
		if (UnitDataTestFlag( missile_data, UNIT_DATA_FLAG_TRAIL_EFFECTS_USE_PROJECTILE ))
		{
			wStat = spwEffectToStatTable[ MISSILE_EFFECT_TRAIL ];
			bHasElement = c_sAddElementsFromStat(pUnit, wStat, pEffect, missile_data, bHasElementType, pnParticleEffectIds, nParticleEffectCount);
		}
		if (UnitDataTestFlag( missile_data, UNIT_DATA_FLAG_IMPACT_EFFECTS_USE_PROJECTILE ))
		{
			wStat = spwEffectToStatTable[ MISSILE_EFFECT_IMPACT_UNIT ];
			bHasElement = c_sAddElementsFromStat(pUnit, wStat, pEffect, missile_data, bHasElementType, pnParticleEffectIds, nParticleEffectCount);
		}
	}

	if (!bHasElement)
	{
		c_sAddElementToEffectList(DAMAGE_TYPE_ALL, pEffect, missile_data, bHasElementType, pnParticleEffectIds, nParticleEffectCount);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int c_MissileEffectAttachToUnit( 
	GAME * pGame, 
	UNIT * pUnit, 
	UNIT * pWeapon, 
	int nMissileClass,
	MISSILE_EFFECT eEffect,
	ATTACHMENT_DEFINITION & tAttachDef,
	int nAttachmentOwnerType,
	int nAttachmentOwnerId,
	float fDuration,
	BOOL bForceNew )
{
	int pnParticleEffectIds[ MAX_DAMAGE_TYPES + 1 ];
	int nParticleEffectCount = 0;
	{
		UNIT * pEffectUnit = pWeapon ? pWeapon : pUnit;
		c_MissileGetParticleEffectDefs( pGame, pEffectUnit, nMissileClass, eEffect, pnParticleEffectIds, nParticleEffectCount );
	}

	if ( pWeapon )
	{
		const UNIT_DATA * pWeaponUnitData = UnitGetData( pWeapon );
		if ( UnitDataTestFlag(pWeaponUnitData, UNIT_DATA_FLAG_NO_WEAPON_MODEL) )
			pWeapon = NULL;
	}
	UNIT * pOwnerUnit = pWeapon ? pWeapon : pUnit;

	if ( nParticleEffectCount <= 0 )
		return INVALID_ID;

	int nParticleSystemFirst = sMissileAddParticleSystemToUnit( pGame, pOwnerUnit, 
																	pnParticleEffectIds[ 0 ],
																	tAttachDef, 
																	nAttachmentOwnerType,
																	nAttachmentOwnerId,
																	fDuration,
																	bForceNew );

	if ( nParticleSystemFirst == INVALID_ID )
		return INVALID_ID;

	for ( int i = 1; i < nParticleEffectCount; i++ )
	{
		int nSystem = c_ParticleSystemAddToEnd( nParticleSystemFirst, pnParticleEffectIds[ i ] );
		if ( fDuration != 0.0f )
		{
			c_ParticleSystemSetParam( nSystem, PARTICLE_SYSTEM_PARAM_DURATION, fDuration );
		}
	}

	const UNIT_DATA * pMissileData = MissileGetData( pGame, nMissileClass );
	float fRangePercent = MissileGetRangePercent(pUnit, pWeapon, pMissileData );
	c_ParticleSystemSetParam( nParticleSystemFirst, PARTICLE_SYSTEM_PARAM_RANGE_PERCENT, fRangePercent );

	c_ParticleSystemSetVisible( nParticleSystemFirst, TRUE );
	c_ParticleSystemSetDraw   ( nParticleSystemFirst, TRUE );

	return nParticleSystemFirst;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define CAMERA_SHAKE_CHECK_Z_THRESHOLD 5.0f
static void c_sMissileShakeCamera(
	GAME * pGame,
	const UNIT_DATA * pMissileData,
	const VECTOR & vPosition)
{
	if(!pMissileData)
		return;

	VECTOR vDirection = RandGetVector(pGame);
	VectorNormalize(vDirection);
	float fDuration = pMissileData->fImpactCameraShakeDuration;
	float fMagnitude = pMissileData->fImpactCameraShakeMagnitude;
	float fDegrade = pMissileData->fImpactCameraShakeDegrade;

	UNIT * pControlUnit = GameGetControlUnit(pGame);
	if ( ! pControlUnit )
		return;  // sometimes this is called as the game is shutting down

	VECTOR vControlUnitPosition = UnitGetPosition(pControlUnit);
	float fDistance;
	if(fabs(vControlUnitPosition.fZ - vPosition.fZ) < CAMERA_SHAKE_CHECK_Z_THRESHOLD)
	{
		fDistance = VectorDistanceXY(vControlUnitPosition, vPosition);
	}
	else
	{
		fDistance = VectorLength(vControlUnitPosition - vPosition);
	}
	if(fDistance >= 1.0f)
	{
		fMagnitude /= (fDistance);
	}

	if(fDuration > 0.0f)
	{
		c_CameraShakeRandom(pGame, vDirection, fDuration, fMagnitude, fDegrade);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int c_MissileEffectCreate( 
	GAME * pGame, 
	UNIT * pUnit, 
	UNIT * pWeapon, 
	int nMissileClass,
	MISSILE_EFFECT eEffect,
	VECTOR & vPosition,
	VECTOR & vNormal,
	float fDuration)
{
	ASSERT_RETINVALID(pGame);
	ASSERT_RETINVALID(pUnit);
	ASSERT_RETINVALID(nMissileClass != INVALID_ID);
	
	UNIT * pOwnerUnit = pWeapon ? pWeapon : pUnit;
	
	int pnParticleEffectIds[ MAX_DAMAGE_TYPES + 1 ];
	int nParticleEffectCount = 0;
	c_MissileGetParticleEffectDefs( pGame, pOwnerUnit, nMissileClass, eEffect, pnParticleEffectIds, nParticleEffectCount );

	if ( nParticleEffectCount <= 0 )
		return INVALID_ID;

	ROOM * pOwnerRoom = UnitGetRoom( pUnit );
	ROOM * pRoom = RoomGetFromPosition( pGame, pOwnerRoom, &vPosition );
	if ( !pRoom )
		pRoom = pOwnerRoom;

	int nParticleSystemFirst = c_ParticleSystemNew( pnParticleEffectIds[ 0 ], &vPosition, &vNormal, RoomGetId( pRoom ) );
	if ( fDuration != 0.0f )
	{
		c_ParticleSystemSetParam( nParticleSystemFirst, PARTICLE_SYSTEM_PARAM_DURATION, fDuration );
	}
	for ( int i = 1; i < nParticleEffectCount; i++ )
	{
		int nSystem = c_ParticleSystemAddToEnd( nParticleSystemFirst, pnParticleEffectIds[ i ] );
		if ( fDuration != 0.0f )
		{
			c_ParticleSystemSetParam( nSystem, PARTICLE_SYSTEM_PARAM_DURATION, fDuration );
		}
	}

	const UNIT_DATA * pMissileData = MissileGetData( pGame, nMissileClass );
	float fRangePercent = MissileGetRangePercent( pOwnerUnit, pWeapon, pMissileData );
	c_ParticleSystemSetParam( nParticleSystemFirst, PARTICLE_SYSTEM_PARAM_RANGE_PERCENT, fRangePercent );

	c_ParticleSystemSetVisible( nParticleSystemFirst, TRUE );
	c_ParticleSystemSetDraw   ( nParticleSystemFirst, TRUE );

	c_sMissileShakeCamera(pGame, pMissileData, vPosition);

	return nParticleSystemFirst;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_MissileEffectUpdateRopeEnd( 
	GAME * pGame, 
	UNIT * pUnit, 
	UNIT * pWeapon, 
	int nMissileClass,
	MISSILE_EFFECT eEffect,
	int nParentParticleSystem,
	BOOL bClearSystem )
{
	ASSERT_RETURN(pGame);
	ASSERT_RETURN(pUnit);
	ASSERT_RETURN( nMissileClass != INVALID_ID );

	UNIT * pOwnerUnit = pWeapon ? pWeapon : pUnit;

	int pnParticleEffectIds[ MAX_DAMAGE_TYPES + 1 ];
	int nParticleEffectCount = 0;
	if (!bClearSystem)
	{
		c_MissileGetParticleEffectDefs(pGame, pOwnerUnit, nMissileClass, eEffect, pnParticleEffectIds, nParticleEffectCount);
	}

	int nRopeEndCurrent = c_ParticleSystemGetRopeEndSystem( nParentParticleSystem );
	int nRopeEndCurrentDefId = c_ParticleSystemGetDefinition( nRopeEndCurrent );

	if ( bClearSystem || nParticleEffectCount == 0 || nRopeEndCurrentDefId != pnParticleEffectIds[ 0 ] )
	{// replace the current system
		int nNewSystemId = c_ParticleSystemSetRopeEndSystem( nParentParticleSystem, 
			nParticleEffectCount ? pnParticleEffectIds[ 0 ] : INVALID_ID );
		for ( int i = 1; i < nParticleEffectCount; i++ )
		{
			c_ParticleSystemAddToEnd( nNewSystemId, pnParticleEffectIds[ i ] );
		}

		if ( nNewSystemId != INVALID_ID )
		{
			const UNIT_DATA * pMissileData = MissileGetData( pGame, nMissileClass );
			float fRangePercent = MissileGetRangePercent(pOwnerUnit, pWeapon, pMissileData);
			c_ParticleSystemSetParam( nNewSystemId, PARTICLE_SYSTEM_PARAM_RANGE_PERCENT, fRangePercent );
		}
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int MissileGetParticleSystem(
	UNIT * pMissile,
	MISSILE_EFFECT eEffect )
{
	int nModelId = c_UnitGetModelId( pMissile );
	if ( nModelId == INVALID_ID )
		return INVALID_ID;

	int nAttachment = c_ModelAttachmentFind( nModelId, ATTACHMENT_OWNER_MISSILE, eEffect );
	if ( nAttachment == INVALID_ID )
		return INVALID_ID;

	ATTACHMENT * pAttachment = c_ModelAttachmentGet( nModelId, nAttachment );
	if ( ! pAttachment || pAttachment->eType != ATTACHMENT_PARTICLE )
		return INVALID_ID;

	return pAttachment->nAttachedId;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define TICKS_BETWEEN_REFLECTS 8
BOOL MissilesReflect(
	GAME* pGame,
	UNIT* pUnit,
	const struct EVENT_DATA& event_data)
{
	ASSERT_RETFALSE( pUnit );

	int nTicksBetweenReflects = UnitGetStat(pUnit, STATS_CHANCE_TO_REFLECT_MISSILES_TICKS);
	if(nTicksBetweenReflects <= 0)
	{
		nTicksBetweenReflects = TICKS_BETWEEN_REFLECTS;
	}

	if(IsUnitDeadOrDying(pUnit))
	{
		// Dead units shouldn't reflect missiles.  But, they CAN be resurrected, in which
		// case, they should resume reflecting missiles.
		UnitRegisterEventTimed( pUnit, MissilesReflect, &event_data, nTicksBetweenReflects );
		return TRUE;
	}

	float fRadius = UnitGetHolyRadius( pUnit );
	if ( fRadius <= 0.0f )
	{
		fRadius = float(UnitGetStat(pUnit, STATS_CHANCE_TO_REFLECT_MISSILE_RADIUS)) / 10.0f;
		if(fRadius <= 0.0f)
		{
			// Sometimes we get the missile reflect CHANCE before we get the missile reflect RADIUS
			// In that case, we are given one chance (and one chance only) to redo this procedure.
			if(event_data.m_Data1)
			{
				UnitRegisterEventTimed( pUnit, MissilesReflect, &EVENT_DATA(FALSE), nTicksBetweenReflects );
				return TRUE;
			}
			return FALSE;
		}
	}

	int nMissileReflectChance = UnitGetStat( pUnit, STATS_CHANCE_TO_REFLECT_MISSILES );
	if ( nMissileReflectChance <= 0 )
		return FALSE;

	UNITID idUnits[MAX_TARGETS_PER_QUERY];
	SKILL_TARGETING_QUERY tTargetingQuery;
	tTargetingQuery.fMaxRange = fRadius;
	tTargetingQuery.pSeekerUnit = pUnit;
	tTargetingQuery.pnUnitIds = idUnits;
	tTargetingQuery.nUnitIdMax = MAX_TARGETS_PER_QUERY;
	tTargetingQuery.tFilter.nUnitType = UNITTYPE_MISSILE;
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_ENEMY );
	SETBIT( tTargetingQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_CLOSEST );
	SkillTargetQuery(pGame, tTargetingQuery);

	UNITID idUnitsToSend[MAX_TARGETS_PER_QUERY];
	for(int i=0; i<MAX_TARGETS_PER_QUERY; i++)
	{
		idUnitsToSend[i] = INVALID_ID;
	}
	int nSendIndex = 0;
	DWORD dwFlags = 0;
	SETBIT(dwFlags, MCO_FIRE_AT_PREVIOUS_OWNER_BIT);
	for(int i=0; i<tTargetingQuery.nUnitIdCount; i++)
	{
		UNITID idMissile = tTargetingQuery.pnUnitIds[i];
		UNIT * pMissile = UnitGetById(pGame, idMissile);
		ASSERT_CONTINUE(pMissile);

		if ( ! UnitIsA( pMissile, UNITTYPE_MISSILE ) )
			continue;

		if(UnitTestFlag(pMissile, UNITFLAG_ATTACHED))
			continue;

		const UNIT_DATA * pUnitData = MissileGetData(pGame, pMissile);

		if(!UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_CAN_REFLECT))
			continue;

		// this is our way of syncing the client and server's roll for the missile - might work...
		RAND tRand;
		RandInit( tRand, UnitGetStat( pMissile, STATS_SKILL_SEED ) );

		int nRoll = RandGetNum( tRand, 100 );
		if ( nRoll > nMissileReflectChance )
			continue;

		if ( IS_CLIENT( pGame ) && pUnitData->nReflectParticleId >= 0 )
		{
			VECTOR vNormal = UnitGetFaceDirection(pMissile, FALSE);
			int nSystem = c_ParticleSystemNew( pUnitData->nReflectParticleId, &UnitGetPosition( pMissile ), &vNormal, UnitGetRoomId( pMissile ) );
			c_ParticleSystemSetVisible( nSystem, TRUE );
			c_ParticleSystemSetDraw   ( nSystem, TRUE );
		}

		MissileChangeOwner(pGame, pMissile, pUnit, NULL, dwFlags);
		idUnitsToSend[nSendIndex] = idMissile;
		nSendIndex++;
	}
	UnitRegisterEventTimed( pUnit, MissilesReflect, &event_data, nTicksBetweenReflects );
	
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define TICKS_BETWEEN_FIZZLES 8
BOOL MissilesFizzle(
	GAME* pGame,
	UNIT* pUnit,
	const struct EVENT_DATA& event_data)
{
	ASSERT_RETFALSE( pUnit );
	if(IsUnitDeadOrDying(pUnit))
	{
		// Dead units shouldn't fizzle missiles.  But, they CAN be resurrected, in which
		// case, they should resume fizzling missiles.
		UnitRegisterEventTimed( pUnit, MissilesFizzle, NULL, TICKS_BETWEEN_FIZZLES );
		return TRUE;
	}

	float fRadius = UnitGetHolyRadius( pUnit );
	if ( fRadius <= 0.0f )
		return FALSE;

	int nMizzileFizzleChance = UnitGetStat( pUnit, STATS_CHANCE_TO_FIZZLE_MISSILES );
	if ( nMizzileFizzleChance <= 0 )
		return FALSE;

	UNITID pUnitIds[ MAX_TARGETS_PER_QUERY ];
	SKILL_TARGETING_QUERY tTargetQuery;
	tTargetQuery.pnUnitIds = pUnitIds;
	tTargetQuery.fMaxRange = fRadius;
	tTargetQuery.nUnitIdMax = MAX_TARGETS_PER_QUERY;
	tTargetQuery.tFilter.nUnitType = UNITTYPE_MISSILE;
	SETBIT( tTargetQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_ENEMY );
	SETBIT( tTargetQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_CLOSEST );
	tTargetQuery.pSeekerUnit = pUnit;

	SkillTargetQuery( pGame, tTargetQuery );
	for ( int i = 0; i < tTargetQuery.nUnitIdCount; i++ )
	{
		UNIT * pMissile = UnitGetById( pGame, pUnitIds[ i ] );
		if ( ! pMissile )
			continue;

		if ( ! UnitIsA( pMissile, UNITTYPE_MISSILE ) )
			continue;

		const UNIT_DATA * pUnitData = UnitGetData( pMissile);
		if ( ! UnitDataTestFlag(pUnitData, UNIT_DATA_FLAG_CAN_FIZZLE) )
			continue;

		// this is our way of syncing the client and server's roll for the missile - might work...
		RAND tRand;
		RandInit( tRand, UnitGetStat( pMissile, STATS_SKILL_SEED ) );

		int nRoll = RandGetNum( tRand, 100 );
		if ( nRoll > nMizzileFizzleChance )
			continue;

		if ( IS_CLIENT( pGame ) && pUnitData->nFizzleParticleId >= 0 )
		{
			VECTOR vNormal = UnitGetFaceDirection(pMissile, FALSE);
			int nSystem = c_ParticleSystemNew( pUnitData->nFizzleParticleId, &UnitGetPosition( pMissile ), &vNormal, UnitGetRoomId( pMissile ) );
			c_ParticleSystemSetVisible( nSystem, TRUE );
			c_ParticleSystemSetDraw   ( nSystem, TRUE );
		}

		UnitUnregisterEventHandler( pGame, pMissile, EVENT_ON_FREED, c_sMissileHandleFreed ); // This prevents the missile from fizzling.

		MissileConditionalFree(pMissile);	
	}
	UnitRegisterEventTimed( pUnit, MissilesFizzle, NULL, TICKS_BETWEEN_FIZZLES );
	
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void MissileConditionalFree(
	UNIT * missile,
	DWORD flags)
{
	ASSERT_RETURN(missile);
	if (UnitTestFlag(missile, UNITFLAG_FREED))
	{
		return;
	}
	ASSERT_RETURN(UnitIsA(missile, UNITTYPE_MISSILE));
	
	GAME * game = UnitGetGame(missile);	
	ASSERT_RETURN(game);
	const UNIT_DATA * missile_data = MissileGetData(game, missile);
	ASSERT(missile_data);
	
	// free missile if we are the server or if the missile is not synched
	if (IS_SERVER(game) || !UnitDataTestFlag(missile_data, UNIT_DATA_FLAG_SYNC))
	{
		if (UnitDataTestFlag(missile_data, UNIT_DATA_FLAG_SYNC))
		{
			flags |= UFF_SEND_TO_CLIENTS;
		}
		UnitFree(missile, flags);
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void MissileSetHoming(
	UNIT *pMissile,
	int nMissileNumber,
	float fDistBeforeHoming)
{
	ASSERTX_RETURN( pMissile, "Expected unit" );
	ASSERTX_RETURN( UnitIsA( pMissile, UNITTYPE_MISSILE ), "Expected missile" );
	
	// set stat
	UnitSetStat( pMissile, STATS_IS_HOMING, TRUE );
	
	// record where we were launced at
	const VECTOR &vPosition = UnitGetPosition( pMissile );
	UnitSetStatVector( pMissile, STATS_MISSILE_LAUNCH_POSITION_X, 0, vPosition );
	
	// set the distance this homing missile must travel in order to activate 
	// its "homing-ness", we use the missile number of which missile this is 
	// in a group that is simultaneously fired so that they can all be offset 
	// a little differently and won't totally be in sync with each other
	fDistBeforeHoming *= pMissile->pUnitData->flHomingUnitScalePct;
	float flBaseDist = fDistBeforeHoming;
	float flBaseDistSq = flBaseDist * flBaseDist;
	float flExtraDist = (float)(nMissileNumber % 4) * 3.0f;
	float flExtraDistSq = flExtraDist * flExtraDist;
	float flDistSq = flBaseDistSq + flExtraDistSq;
	UnitSetStatFloat( pMissile, STATS_HOMING_ACTIVATION_RANGE_SQ, 0, flDistSq );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL MissileIsHoming(
	UNIT *pMissile)
{
	ASSERTX_RETNULL( pMissile, "Expected missile" );
	return UnitGetStat( pMissile, STATS_IS_HOMING );
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UNIT *MissileGetHomingTarget(
	UNIT *pMissile)
{
	ASSERTX_RETNULL( pMissile, "Expected missile" );	
	UNIT *pTarget = NULL;

	// must be homing
	if (MissileIsHoming( pMissile ))
	{
		GAME *pGame = UnitGetGame( pMissile );

		// a homing missile cannot find a target until it has travelled far enough away
		// from its launch position
		BOOL bCanSeekTarget = UnitGetStat( pMissile, STATS_HOMING_CAN_SEEK_TARGET );
		if (bCanSeekTarget == FALSE)
		{

			// how far from the launch position must the missile have traveled?
			float flHomingActivationRangeSq = UnitGetStatFloat( pMissile, STATS_HOMING_ACTIVATION_RANGE_SQ );

			// check the distance from the launch position			
			VECTOR vLaunchPos = UnitGetStatVector( pMissile, STATS_MISSILE_LAUNCH_POSITION_X, 0 );
			float flDistSq = VectorDistanceSquared( vLaunchPos, UnitGetPosition( pMissile ) );
			if (flDistSq >= flHomingActivationRangeSq)
			{
				UnitSetStat( pMissile, STATS_HOMING_CAN_SEEK_TARGET, TRUE );
				bCanSeekTarget = TRUE;
			}

		}

		// continue target logic
		if (bCanSeekTarget)
		{

			// get our target
			UNITID idTarget = UnitGetStat( pMissile, STATS_HOMING_TARGET_ID );
			pTarget = UnitGetById( pGame, idTarget );

			// if no target, try to get another one
			if (pTarget == NULL || IsUnitDeadOrDying( pTarget ))
			{ 

				// clear the target out of our stat
				pTarget = NULL;
				UnitClearStat( pMissile, STATS_HOMING_TARGET_ID, 0 );

				// look for another target now				
				SKILL_TARGETING_QUERY tTargetQuery;
				tTargetQuery.pnUnitIds = &idTarget;
				tTargetQuery.fMaxRange = 20.0f;
				tTargetQuery.fLOSDistance = tTargetQuery.fMaxRange;
				tTargetQuery.nUnitIdMax = 1;
				SETBIT( tTargetQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_ENEMY );
				SETBIT( tTargetQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_TARGET_OBJECT );
				SETBIT( tTargetQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_CHECK_LOS );
				SETBIT( tTargetQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_ALLOW_DESTRUCTABLES );
				SETBIT( tTargetQuery.tFilter.pdwFlags, SKILL_TARGETING_BIT_CLOSEST );
				tTargetQuery.pSeekerUnit = pMissile;
				SkillTargetQuery( pGame, tTargetQuery );
				if (tTargetQuery.nUnitIdCount == 1)
				{
					UnitSetStat( pMissile, STATS_HOMING_TARGET_ID, idTarget );
				}

			}

		}

	}

	return pTarget;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int c_sFieldMissileSelectFieldMissile(
	GAME * pGame,
	FIELD_MISSILE_MAP * pFieldMissileMap)
{
	ASSERT_RETINVALID(pGame);
	ASSERT_RETINVALID(IS_CLIENT(pGame));
	ASSERT_RETINVALID(pFieldMissileMap);
	ASSERT_RETINVALID(pFieldMissileMap->pFieldMissiles);

	int nFieldMissileCount = pFieldMissileMap->nFieldMissileCount;

	GAME_TICK tiNewestMissileTick = GameGetTick(pGame);
	int nNewestMissileIndex = INVALID_ID;

	for(int i=0; i<nFieldMissileCount; i++)
	{
		const FIELD_MISSILE * pFieldMissile = (const FIELD_MISSILE *)&pFieldMissileMap->pFieldMissiles[i];
		ASSERT_CONTINUE(pFieldMissile);

		if(nNewestMissileIndex < 0 || tiNewestMissileTick < pFieldMissile->tiFuseStartTime)
		{
			nNewestMissileIndex = i;
			tiNewestMissileTick = pFieldMissile->tiFuseStartTime;
		}
	}
	return nNewestMissileIndex;
}

//----------------------------------------------------------------------------
// Forward declaration
//----------------------------------------------------------------------------
static void c_sFieldMissileClearPathNode(
	GAME* pGame,
	UNIT* pMissile,
	UNIT* ,
	EVENT_DATA* event_data,
	void* data,
	DWORD dwId );

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL c_sFieldMissileMapTestShouldPlaceFieldMissile(
	ROOM * pRoomSource, 
	ROOM_PATH_NODE * pPathNodeSource, 
	ROOM * pRoomDest, 
	ROOM_PATH_NODE * pPathNodeDest, 
	float fDistance, 
	void * pUserData)
{
	ASSERT_RETFALSE(pUserData);

	BOOL * pbConnectionsHaveFields = (BOOL*)pUserData;
	FIELD_MISSILE_MAP * pFieldMissileMap = HashGet( pRoomDest->tFieldMissileMap, pPathNodeDest->nIndex );
	if(pFieldMissileMap && pFieldMissileMap->pUnit /*&& fDistance <= 1.0f*/)
	{
		*pbConnectionsHaveFields = TRUE;
		return FALSE;
	}
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void c_sFieldMissileMapUpdateMissile(
	GAME * pGame,
	UNIT * pOwner,
	FIELD_MISSILE_MAP * pFieldMissileMap,
	int nMissileClass,
	ROOM * pRoom,
	ROOM_PATH_NODE * pPathNode)
{
	BOOL bConnectionsHaveFields = FALSE;
	RoomPathNodeIterateConnections(pRoom, pPathNode, c_sFieldMissileMapTestShouldPlaceFieldMissile, &bConnectionsHaveFields);
	int nFieldMissileIndex = c_sFieldMissileSelectFieldMissile(pGame, pFieldMissileMap);
	if(!bConnectionsHaveFields && nFieldMissileIndex >= 0)
	{
		if(!pFieldMissileMap->pUnit || UnitGetClass(pFieldMissileMap->pUnit) != pFieldMissileMap->pFieldMissiles[nFieldMissileIndex].nMissileClass)
		{
			if(pFieldMissileMap->pUnit)
			{
				// We must unregister this here so that the field missile map doesn't get freed out from underneath us
				UnitUnregisterEventHandler(pGame, pFieldMissileMap->pUnit, EVENT_ON_FREED, c_sFieldMissileClearPathNode);
				MissileConditionalFree(pFieldMissileMap->pUnit);
			}
			pFieldMissileMap->pUnit = NULL;
			VECTOR vMissilePosition = RoomPathNodeGetExactWorldPosition(pGame, pRoom, pPathNode);
#if HELLGATE_ONLY
			VECTOR vMissileNormal = AppIsHellgate() ? RoomPathNodeGetWorldNormal(pGame, pRoom, pPathNode) : VECTOR( 0, 0, 1 );
#else
			VECTOR vMissileNormal( 0, 0, 1);
#endif
			DWORD dwMissileTag = 1;
			UNIT * field = MissileFire(
				pGame,
				pFieldMissileMap->pFieldMissiles[nFieldMissileIndex].nMissileClass,
				pOwner,
				NULL,
				INVALID_ID,
				NULL,
				VECTOR(0),
				vMissilePosition,
				vMissileNormal,
				0,
				dwMissileTag);
			pFieldMissileMap->pUnit = field;
			UnitRegisterEventHandler(pGame, pFieldMissileMap->pUnit, EVENT_ON_FREED, c_sFieldMissileClearPathNode, EVENT_DATA(UnitGetId(pOwner), RoomGetId(pRoom), pPathNode->nIndex, nMissileClass));
		}
		if(pFieldMissileMap->pUnit)
		{
			int nTicks = pFieldMissileMap->pFieldMissiles[nFieldMissileIndex].tiFuseEndTime - GameGetTick(pGame);
			UnitSetFuse(pFieldMissileMap->pUnit, nTicks);
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void c_sFieldMissileMapCleanup(
	GAME * pGame,
	ROOM * pRoom,
	int nPathNodeIndex)
{
	FIELD_MISSILE_MAP * pFieldMissileMap = HashGet( pRoom->tFieldMissileMap, nPathNodeIndex );
	if(!pFieldMissileMap)
		return;

	GAME_TICK tiCurrentTick = GameGetTick(pGame);
	for(int i=0; i<pFieldMissileMap->nFieldMissileCount; i++)
	{
		FIELD_MISSILE * pFieldMissile = (FIELD_MISSILE *)&pFieldMissileMap->pFieldMissiles[i];
		ASSERT_CONTINUE(pFieldMissile);

		if(pFieldMissile->tiFuseEndTime <= tiCurrentTick)
		{
			MemoryCopy(pFieldMissile, sizeof(FIELD_MISSILE), &pFieldMissileMap->pFieldMissiles[pFieldMissileMap->nFieldMissileCount-1], sizeof(FIELD_MISSILE));
			pFieldMissileMap->nFieldMissileCount--;
			i--;
			ASSERT_BREAK(pFieldMissileMap->nFieldMissileCount >= 0);
		}
	}
	if(pFieldMissileMap->nFieldMissileCount <= 0)
	{
		HashRemove(pRoom->tFieldMissileMap, nPathNodeIndex);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void c_sFieldMissileClearPathNode(
	GAME* pGame,
	UNIT* pMissile,
	UNIT* ,
	EVENT_DATA* event_data,
	void* data,
	DWORD dwId )
{
	UNITID idOwner = (UNITID)event_data->m_Data1;
	ROOMID idRoom = (ROOMID)event_data->m_Data2;
	int nPathNodeIndex = (int)event_data->m_Data3;
	int nMissileClass = (int)event_data->m_Data4;
	ROOM * pRoom = RoomGetByID( pGame, idRoom );
	if(!pRoom)
		return;

	UNIT * pOwner = UnitGetById(pGame, idOwner);
	if(!pOwner)
		return;

	ASSERT_RETURN(pGame && IS_CLIENT(pGame));

	c_sFieldMissileMapCleanup(pGame, pRoom, nPathNodeIndex);

	FIELD_MISSILE_MAP * pFieldMissileMap = HashGet( pRoom->tFieldMissileMap, nPathNodeIndex );
	if(pFieldMissileMap)
	{
		if(pFieldMissileMap->pUnit == pMissile)
		{
			pFieldMissileMap->pUnit = NULL;
		}
		c_sFieldMissileMapUpdateMissile(pGame, pOwner, pFieldMissileMap, nMissileClass, pRoom, RoomGetRoomPathNode(pRoom, nPathNodeIndex));
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void c_sFieldMissileMapAddMissile(
	GAME * pGame,
	FIELD_MISSILE_MAP * pFieldMissileMap,
	int nMissileClass,
	DWORD nFuseDurationTicks)
{
	ASSERT_RETURN(pGame);
	ASSERT_RETURN(IS_CLIENT(pGame));
	ASSERT_RETURN(pFieldMissileMap);

	int nFieldMissileCount = pFieldMissileMap->nFieldMissileCount;
	int nFieldMissileIndex = INVALID_ID;
	for(int i=0; i<nFieldMissileCount; i++)
	{
		const FIELD_MISSILE * pFieldMissile = (const FIELD_MISSILE *)&pFieldMissileMap->pFieldMissiles[i];
		ASSERT_CONTINUE(pFieldMissile);

		if(pFieldMissile->nMissileClass == nMissileClass)
		{
			nFieldMissileIndex = i;
			break;
		}
	}
	if(nFieldMissileIndex < 0)
	{
		pFieldMissileMap->pFieldMissiles = (FIELD_MISSILE*)GREALLOC(pGame, pFieldMissileMap->pFieldMissiles, (pFieldMissileMap->nFieldMissileCount+1) * sizeof(FIELD_MISSILE));
		nFieldMissileIndex = pFieldMissileMap->nFieldMissileCount;
		pFieldMissileMap->nFieldMissileCount++;
	}
	ASSERT_RETURN(nFieldMissileIndex >= 0);

	FIELD_MISSILE * pFieldMissile = &pFieldMissileMap->pFieldMissiles[nFieldMissileIndex];
	pFieldMissile->nMissileClass = nMissileClass;
	pFieldMissile->tiFuseStartTime = GameGetTick(pGame);
	pFieldMissile->tiFuseEndTime = pFieldMissile->tiFuseStartTime + nFuseDurationTicks;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL c_sFieldMissileOccupyPathNode(
	GAME * pGame,
	UNIT * pOwner,
	int nMissileClass,
	ROOM * pRoom,
	ROOM_PATH_NODE * pPathNode,
	DWORD nFuseDurationTicks)
{
	ASSERT_RETFALSE(pGame);
	ASSERT_RETFALSE(pRoom);
	ASSERT_RETFALSE(pPathNode);

	if ( IS_SERVER( pGame ) )
	{
		return FALSE;
	}

	const UNIT_DATA * pMissileData = MissileGetData(pGame, nMissileClass);
	ASSERT_RETFALSE(pMissileData);

	if(!UnitIsA(pMissileData->nUnitType, UNITTYPE_FIELD_MISSILE))
	{
		return FALSE;
	}

	c_sFieldMissileMapCleanup(pGame, pRoom, pPathNode->nIndex);

	FIELD_MISSILE_MAP * pFieldMissileMap = HashGet( pRoom->tFieldMissileMap, pPathNode->nIndex );
	if( !pFieldMissileMap )
	{
		pFieldMissileMap = HashAdd( pRoom->tFieldMissileMap, pPathNode->nIndex );
		ASSERT_RETFALSE( pFieldMissileMap );
	}
	c_sFieldMissileMapAddMissile(pGame, pFieldMissileMap, nMissileClass, nFuseDurationTicks);
	c_sFieldMissileMapUpdateMissile(pGame, pOwner, pFieldMissileMap, nMissileClass, pRoom, pPathNode);
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_MissileCreateField(
	GAME * pGame,
	UNIT * pOwner,
	int nFieldMissile,
	const VECTOR & vPosition,
	float fRadius,
	DWORD nDurationTicks)
{
	ASSERT_RETURN(IS_CLIENT(pGame));

	LEVEL * pLevel = UnitGetLevel(pOwner);
	if(!pLevel)
	{
		return;
	}

	UNIT * pControlUnit = GameGetControlUnit(pGame);
	ASSERT_RETURN(pControlUnit);
	if(UnitGetLevel(pControlUnit) != pLevel)
	{
		return;
	}

	ROOM * pRoom = RoomGetFromPosition(pGame, pLevel, &vPosition);
	if(!pRoom)
	{
		return;
	}

	const int nMaxFieldMissiles = 2000; // = 25 meters square, plus some extra
	FREE_PATH_NODE tOutput[nMaxFieldMissiles];
	NEAREST_NODE_SPEC tSpec;
	SETBIT( tSpec.dwFlags, NPN_ALLOW_OCCUPIED_NODES );
	SETBIT( tSpec.dwFlags, NPN_CHECK_LOS );
	SETBIT( tSpec.dwFlags, NPN_DONT_CHECK_HEIGHT );
	SETBIT( tSpec.dwFlags, NPN_DONT_CHECK_RADIUS );
	SETBIT( tSpec.dwFlags, NPN_EMPTY_OUTPUT_IS_OKAY );
	SETBIT( tSpec.dwFlags, NPN_USE_EXPENSIVE_SEARCH );
	tSpec.fMaxDistance = fRadius;
	int nCountToUse = CEIL(fRadius * fRadius * PI + 1.0f); // The pirates area: Pi arrrr squared, plus a bit
	nCountToUse = PIN(nCountToUse, 1, nMaxFieldMissiles);
	int nNumNodes = RoomGetNearestPathNodes(pGame, pRoom, vPosition, nCountToUse, tOutput, &tSpec);

	const UNIT_DATA * hit_missile_data = MissileGetData(pGame, nFieldMissile);
	ASSERT_RETURN(hit_missile_data);

	for(int i=0; i<nNumNodes; i++)
	{
		c_sFieldMissileOccupyPathNode(pGame, pOwner, nFieldMissile, tOutput[i].pRoom, tOutput[i].pNode, nDurationTicks);
	}
}
