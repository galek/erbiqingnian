//----------------------------------------------------------------------------
// weapons.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "prime.h"
#include "game.h"
#include "globalindex.h"
#include "units.h"
#include "weapons.h"
#include "c_appearance.h"
#include "gameunits.h"
#include "unit_priv.h"
#include "c_camera.h"
#include "camera_priv.h"
#include "c_particles.h"
#include "skills.h"
#include "items.h"
#include "weapons_priv.h"
#include "c_appearance_priv.h"
#include "states.h"
#include "uix.h"
#include "e_model.h"
#include "excel_private.h"
#include "e_primitive.h"
#include "colors.h"
//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
#define AIM_CONE_DOT 0.85f

static BOOL sgbLockWeaponsOnVanity = TRUE;

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
static const char sgpszWeaponAttachmentNames[ MAX_WEAPONS_PER_UNIT ][ 16 ] =
{
	"Bip01 Prop1",
	"Bip01 Prop2"
};

static const VECTOR sgpvWeaponAttachmentNormals[ NUM_WEAPON_BONE_TYPES ] =
{
	VECTOR(1,0,0),
	VECTOR(1,0,0)
};

static const float sgpfWeaponAttachmentRotations[ MAX_WEAPONS_PER_UNIT ][ NUM_WEAPON_BONE_TYPES ] =
{// these are in degrees
	{	0.0f,	0.0f, }, // WEAPON_INDEX_RIGHT_HAND
	{	0.0f,	180.0f, } // WEAPON_INDEX_LEFT_HAND
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int c_WeaponGetAttachmentBoneIndex( 
	int nAppearanceDef, 
	int nWeaponIndex,
	int nWeaponBoneType )
{
	ASSERT_RETINVALID( nWeaponIndex >= 0 && nWeaponIndex < MAX_WEAPONS_PER_UNIT );
	ASSERT_RETINVALID( nWeaponBoneType >= 0 && nWeaponBoneType < NUM_WEAPON_BONE_TYPES );
	APPEARANCE_DEFINITION * pAppearanceDef = (APPEARANCE_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_APPEARANCE, nAppearanceDef );
	if ( ! pAppearanceDef )
		return INVALID_ID;
	if ( pAppearanceDef->pnWeaponBones[ nWeaponBoneType ][ nWeaponIndex ] != INVALID_ID )
		return pAppearanceDef->pnWeaponBones[ nWeaponBoneType ][ nWeaponIndex ];
	return c_AppearanceDefinitionGetBoneNumber( nAppearanceDef, sgpszWeaponAttachmentNames[ nWeaponIndex ] );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const char * c_WeaponGetAttachmentBoneName( 
	int nAppearanceDef, 
	int nWeaponIndex,
	int nWeaponBoneType,
	VECTOR & vNormal,
	float & fRotation )
{
	ASSERT_RETNULL( nWeaponIndex >= 0 && nWeaponIndex < MAX_WEAPONS_PER_UNIT );
	ASSERT_RETNULL( nWeaponBoneType >= 0 && nWeaponBoneType < NUM_WEAPON_BONE_TYPES );
	vNormal = sgpvWeaponAttachmentNormals[ nWeaponBoneType ];
	fRotation = sgpfWeaponAttachmentRotations[ nWeaponIndex ][ nWeaponBoneType ];
	APPEARANCE_DEFINITION * pAppearanceDef = (APPEARANCE_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_APPEARANCE, nAppearanceDef );
	if ( ! pAppearanceDef )
		return sgpszWeaponAttachmentNames[ nWeaponIndex ]; // just in case we haven't loaded the appearance yet
	const char * pszBoneName = nWeaponIndex ? pAppearanceDef->pszLeftWeaponBone[ nWeaponBoneType ] : pAppearanceDef->pszRightWeaponBone[ nWeaponBoneType ];
	if ( pszBoneName[ 0 ] != 0 )
		return pszBoneName;
	return sgpszWeaponAttachmentNames[ nWeaponIndex ];
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_WeaponSetLockOnVanity(
	BOOL bLock )
{
	sgbLockWeaponsOnVanity = bLock;
}

BOOL c_WeaponGetLockOnVanity()
{
	return sgbLockWeaponsOnVanity;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void sAdjustingAimingByCamera(
	UNIT * pUnit,
	const CAMERA_INFO * pCameraInfo,
	VECTOR & vAimDirection,
	VECTOR & vAimStart,
	UNIT * pTargetUnit,
	UNIT * pWeapon )
{
	ASSERT_RETURN( AppIsHellgate() );
	
	ASSERT_RETURN( pUnit == GameGetControlUnit( UnitGetGame( pUnit )) );
	VECTOR vCameraPosition = CameraInfoGetPosition(pCameraInfo);
	VECTOR vCameraDirection = CameraInfoGetDirection(pCameraInfo);

	BOOL bNoLowAiming = FALSE;
	BOOL bNoHighAiming = FALSE;

	int nWeaponSkill = ItemGetPrimarySkill( pWeapon );
	if ( nWeaponSkill != INVALID_ID )
	{
		const SKILL_DATA * pSkillData = SkillGetData( NULL, nWeaponSkill );
		if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_NO_LOW_AIMING_THIRDPERSON ) )
		{
			bNoLowAiming = TRUE;
		}
		if ( SkillDataTestFlag( pSkillData, SKILL_FLAG_NO_HIGH_AIMING_THIRDPERSON ) )
		{
			bNoHighAiming = TRUE;
		}
	}
	if ( c_CameraGetViewType() != VIEW_1STPERSON && ! pTargetUnit )
	{
		if ( ( vCameraDirection.fZ < 0.0f && bNoLowAiming ) || ( vCameraDirection.fZ > 0.0f && bNoHighAiming ) )
			vAimDirection = UnitGetFaceDirection( pUnit, FALSE );
		else
		{
			vAimDirection = vCameraDirection;
			{
				// find the point above the unit following the camera
				VECTOR vPlaneNormal = UnitGetFaceDirection( pUnit, FALSE );
				vPlaneNormal.fZ = 0.0f;
				VectorNormalize( vPlaneNormal );
				PLANE tPlane;
				PlaneFromPointNormal( &tPlane, &UnitGetPosition( pUnit ), &vPlaneNormal );

				VECTOR vIntersect( 0.0f );
				VECTOR vTargetPos = vCameraPosition + 20.0f * vCameraDirection;
				PlaneVectorIntersect( &vIntersect, &tPlane, &vCameraPosition, &vTargetPos );
				//vAimStart.fZ = vCameraPosition.fZ;
				vAimStart = vIntersect;
			}
		}
	}
	else
	{
		vAimDirection = vCameraDirection;
	}
	if ( c_CameraGetViewType() == VIEW_1STPERSON )
		vAimStart = vCameraPosition;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

#define AIM_MAX_DISTANCE		25.0f

void UnitUpdateWeaponPosAndDirecton( 
	GAME * pGame,
	UNIT * pUnit,
	int nWeaponIndex,
	UNIT * pTargetUnit,
	BOOL bForceAimAtTarget,
	BOOL bDontAimAtCrosshairs,
	BOOL* bInvalidTarget )
{
	const CAMERA_INFO * pCameraInfo = IS_CLIENT( pGame ) ? c_CameraGetInfo( FALSE ) : NULL;
	int nWardrobe = UnitGetWardrobe( pUnit );
	UNIT * pWeapon = NULL;
	if ( nWardrobe != INVALID_ID && nWeaponIndex != INVALID_ID )
	{
		pWeapon = WardrobeGetWeapon( pGame, nWardrobe, nWeaponIndex );
	}

	BOOL bIsControlUnit = IS_SERVER( pGame ) ? FALSE : GameGetControlUnit( pGame ) == pUnit;
	if( bIsControlUnit )
	{
		UNIT * pCameraUnit = GameGetCameraUnit( pGame );
		if ( AppIsHellgate() )
		{ // no weapon, so just follow the eye.
			if ( pUnit != pCameraUnit ) 
			{
				pWeapon = pCameraUnit;
				nWeaponIndex = 0;
			}
			else if (! pWeapon )
			{
				if ( pCameraInfo && nWeaponIndex != INVALID_ID )
				{
					pUnit->pvWeaponPos[ nWeaponIndex ] = CameraInfoGetPosition(pCameraInfo);
					pUnit->pvWeaponDir[ nWeaponIndex ] = CameraInfoGetDirection(pCameraInfo);
				}
				return;
			} 
			if ( c_CameraGetViewType() == VIEW_VANITY )
			{
				if ( c_WeaponGetLockOnVanity() )
					return;
			} else {
				c_WeaponSetLockOnVanity( TRUE ); // just in case we didn't clear it when clearing vanity
			}
				
			if ( AppGetDetachedCamera() )
				return;
		}
	}
	// get the position and direction of the muzzle
	VECTOR vPosition = VECTOR( 0.0f );
	if ( UnitTestFlag( pUnit, UNITFLAG_USE_APPEARANCE_MUZZLE_OFFSET ) )
	{
		UnitGetWeaponPositionAndDirection( pGame, pUnit, nWeaponIndex, &vPosition, NULL );
	}
	else if ( pWeapon && IS_CLIENT( pGame ) )
	{
		if ( UnitIsA( pWeapon, UNITTYPE_MELEE ) )
		{
			vPosition = UnitGetCenter( pUnit );
		} 
		else 
		{
			// fire attachment is being used to find the correct bone
			int nModelId;
			int nAppearanceDefId;
			int nBoneIndex = INVALID_ID;

			const UNIT_DATA * pWeaponData = UnitGetData( pWeapon );
			ASSERT_RETURN( pWeaponData );
			if ( UnitDataTestFlag(pWeaponData, UNIT_DATA_FLAG_NO_WEAPON_MODEL) )
			{
				nModelId    = c_UnitGetModelId( pUnit );
				ASSERT_RETURN( nModelId    != INVALID_ID );
				nAppearanceDefId = e_ModelGetAppearanceDefinition( nModelId );
				//ASSERT_RETURN( nAppearanceDefId != INVALID_ID );
				if ( nAppearanceDefId != INVALID_ID )
				{
					nBoneIndex = c_WeaponGetAttachmentBoneIndex( nAppearanceDefId, nWeaponIndex );
				}

			} else {
				nModelId    = c_UnitGetModelId( pWeapon	);
				ASSERT_RETURN( nModelId    != INVALID_ID );
				nAppearanceDefId = e_ModelGetAppearanceDefinition( nModelId );
				//ASSERT_RETURN( nAppearanceDefId != INVALID_ID );
				if ( nAppearanceDefId != INVALID_ID )
				{
					nBoneIndex = c_AppearanceDefinitionGetBoneNumber( nAppearanceDefId, MUZZLE_BONE_DEFAULT );
				}
			}

			if ( nBoneIndex != INVALID_ID )
			{
				const MATRIX * pmWeaponWorld = e_ModelGetWorldScaled( nModelId );
				MATRIX mBone;
				BOOL bRet = c_AppearanceGetBoneMatrix ( nModelId, &mBone, nBoneIndex );
				ASSERT_RETURN( bRet );

				MATRIX mTransform;
				MatrixMultiply( &mTransform, &mBone, pmWeaponWorld );
				MatrixMultiply( &vPosition, &vPosition, &mTransform );

				if ( AppIsHellgate() && bIsControlUnit )
					c_CameraAdjustPointForFirstPersonFOV( pGame, vPosition, TRUE );

				if ( UnitTestFlag( pUnit, UNITFLAG_CLIENT_PLAYER_MOVING ) )
				{
					VECTOR vMovement = UnitGetMoveDirection( pUnit );
					float fVelocity = UnitGetVelocity( pUnit ) * GAME_TICK_TIME * 1.5f;
					vMovement *= fVelocity;
					vPosition += vMovement;
				}
			}

		}
	} 
	else 
	{
		if (AppIsHellgate())
		{
			vPosition = c_UnitGetPosition( pUnit );
		}
		else if (AppIsTugboat())
		{
			// TRAVIS: FIXME!
			vPosition = UnitGetCenter( pUnit );
			vPosition.fZ += 1.0f;
		}
	}

	VECTOR vDirection = UnitGetFaceDirection( pUnit, FALSE );

	// we have to get the skill using the weapon - not necessarily the weapon's skill
	// cabalist focus items can be used by many different skills - which changes the target selection
	int nSkill = SkillGetSkillCurrentlyUsingWeapon( pUnit, pWeapon ); 

	const SKILL_DATA * pSkillData = nSkill != INVALID_ID ? SkillGetData( pGame, nSkill ) : NULL;

	// decide whether we really want to aim towards this target
	UNIT * pSelected = NULL;
	if ( ! pSkillData || SkillDataTestFlag( pSkillData, SKILL_FLAG_CAN_TARGET_UNIT ) )
		pSelected = pTargetUnit;
	if ( bForceAimAtTarget )
		pSelected = pTargetUnit;

	// TRAVIS: might have to remove - server doesn't necessarily look like client
	if( AppIsTugboat() && pSelected && pSkillData )
	{

		if( pSkillData->fFiringCone > 0 )
		{
	
			VECTOR vAttackPosition = vPosition - vDirection * .5f;
			VECTOR vDelta = UnitGetPosition( pSelected ) - vAttackPosition;
			vDelta.fZ = 0;
			VectorNormalize( vDelta, vDelta );
			float fDot = acos( PIN( VectorDot( vDirection, vDelta ), -1.0f, 1.0f ) );
			float Radians = pSkillData->fFiringCone;
			if ( fDot > Radians  )
			{
				if( bInvalidTarget )
				{
					*bInvalidTarget = TRUE;
				}
				pSelected = NULL;
			}
		}
	}
	
	if ( pSkillData && ! SkillIsValidTarget( pGame, pUnit, pSelected, pWeapon, nSkill, NULL, FALSE ) )
		pSelected = NULL;

	// establish your aiming direction
	VECTOR vAimDirection = vDirection;
	VECTOR vAimStart = vPosition;
	if ( AppIsHellgate() && bIsControlUnit )
		sAdjustingAimingByCamera( pUnit, pCameraInfo, vAimDirection, vAimStart, pSelected, pWeapon );

	VECTOR vAimPoint;
	if ( pSelected )
	{  
		vAimPoint = UnitGetAimPoint( pSelected );
		VECTOR vToAimPoint = vAimPoint - vPosition;
		vToAimPoint.fZ = 0.0f;
		VectorNormalize( vToAimPoint );
		VECTOR vAimDirectionNoZ = vAimDirection;
		vAimDirectionNoZ.fZ = 0.0f;
		VectorNormalize( vAimDirectionNoZ );
		// don't include z in this one... it makes you shoot the ground when you shouldn't
		if ( AppIsHellgate() && vToAimPoint.fX * vAimDirectionNoZ.fX + vToAimPoint.fY * vAimDirectionNoZ.fY < AIM_CONE_DOT )
		{
			pSelected = NULL;
			if ( bIsControlUnit )
				sAdjustingAimingByCamera( pUnit, pCameraInfo, vAimDirection, vAimStart, NULL, pWeapon );
		} 
	}

	VECTOR vEndLoc;
	float fLerpTowardsEndLoc = AppIsHellgate() ? 1.0f : 0.0f;
	if ( pSelected )
	{
		vEndLoc = vAimPoint;
		if ( AppIsHellgate() && IS_CLIENT( pGame ) && pSelected == GameGetControlUnit( pGame ) )
		{ // when aiming at the player, aim at towards the camera
			VECTOR vCameraPosition = CameraInfoGetPosition(pCameraInfo);
			vEndLoc.fX = vCameraPosition.fX;
			vEndLoc.fY = vCameraPosition.fY;
		}
	}
	else if ( AppIsHellgate() && bIsControlUnit && ! bDontAimAtCrosshairs )
	{
		int nViewType = c_CameraGetViewType();
		if ( nViewType >= VIEW_DIABLO )
		{
			float fCameraAngle = CameraInfoGetAngle(pCameraInfo);
			vEndLoc = vPosition;
			vEndLoc.fX += cosf( fCameraAngle );
			vEndLoc.fY += sinf( fCameraAngle );
		}
		else 
		{
			// code to shoot at the crosshairs from the gun position
			// ray out in front of camera...
			//float fDist = LevelLineCollideLen(pGame, UnitGetLevel( pUnit ), vAimStart, 
			//	vAimDirection, AIM_MAX_DISTANCE);
			float fDist = AIM_MAX_DISTANCE;
			UNIT * pAimTarget = SelectTarget( pGame, pUnit, 0, vAimStart, vAimDirection, AIM_MAX_DISTANCE, NULL, &fDist, NULL, nSkill, pWeapon );
			if ( pAimTarget )
			{
				fDist += 0.5f;
			}
			VECTOR vDelta = vAimDirection;
			if ( fDist < 2.0f && nViewType != VIEW_1STPERSON) 
			{
				fLerpTowardsEndLoc = (fDist > 0.0f ? fDist : 0.0f) / 2.0f;
			}
			vDelta *= fDist; // this keeps us for looking/aiming just above the player's head
			vEndLoc = vAimStart + vDelta;
		}
	} else {
		fLerpTowardsEndLoc = 0.0f;
	}

	if ( fLerpTowardsEndLoc > 0.0f )
	{
		VECTOR vTowardsEndLoc = vEndLoc - vPosition;
		if ( AppIsTugboat() )
			vTowardsEndLoc.fZ = 0;
		vDirection = VectorLerp( vTowardsEndLoc, vDirection, fLerpTowardsEndLoc );
		VectorNormalize( vDirection );
	}
	pUnit->pvWeaponPos[ nWeaponIndex ] = vPosition;
	pUnit->pvWeaponDir[ nWeaponIndex ] = vDirection;

#if (0)
	V( e_PrimitiveAddCone( PRIM_FLAG_SOLID_FILL | PRIM_FLAG_RENDER_ON_TOP,
		&vPosition,
		1.0f,
		&vDirection, 
		PI_OVER_FOUR / 2.0f,  
		GFXCOLOR_RED ) );
#endif

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static UNIT * sSelectTargetInDirection( 
	GAME * pGame,
	UNIT * pUnit,
	VECTOR & vPosition,
	VECTOR & vDirection,
	int nWeaponSkill,
	UNIT * pWeapon,
	DWORD dwFlags = 0,
	UNITID idPrevious = INVALID_ID,
	float* pfLevelLineCollideResults = NULL )
{
	BOOL bIsInRange = FALSE;
	DWORD dwSelectionFlags = dwFlags | SELECTTARGET_FLAG_CHECK_SLIM_AND_FAT;
	UNIT * pTarget = SelectTarget( pGame, pUnit, dwSelectionFlags,
		vPosition, vDirection, 500.0f, &bIsInRange,NULL, NULL, nWeaponSkill, 
		pWeapon, idPrevious, NULL, pfLevelLineCollideResults );
	return pTarget;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

#if !ISVERSION(SERVER_VERSION)

UNIT * c_WeaponFindTargetForControlUnit( 
	GAME * pGame,
	int nWeaponIndex,
	int nSkill,
	UNITID idPrevious )
{
	ASSERT_RETNULL( AppIsHellgate() ); // there were no calls to this in the Tugboat code

	UNIT * pUnit = GameGetControlUnit( pGame );

	const CAMERA_INFO * pCameraInfo = c_CameraGetInfo( FALSE );

	int nWardrobe = UnitGetWardrobe( pUnit );
	if ( nWardrobe == INVALID_ID )
		return NULL;

	UNIT * pWeapon = NULL;
	if ( nWeaponIndex == INVALID_ID )
	{
		UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT ];
		UnitGetWeapons( pUnit, nSkill, pWeapons, FALSE );
		pWeapon = pWeapons[ 0 ];
	} else {
		pWeapon = WardrobeGetWeapon( pGame, nWardrobe, nWeaponIndex );
	}

	if ( pWeapon || GameGetCameraUnit( pGame ) != pUnit )
		UnitUpdateWeaponPosAndDirecton( pGame, pUnit, nWeaponIndex, NULL, FALSE ); // unlock the weapon from its target

#define NUM_TARGETING_DIRECTIONS 2
	VECTOR pvPosition[ NUM_TARGETING_DIRECTIONS ];
	VECTOR pvDirection[ NUM_TARGETING_DIRECTIONS ];

	int nCameraViewType = c_CameraGetViewType();
	// if no cursor, get position and direction the old way...
	// vector from player straight ahead in the middle of the screen
	if (! pCameraInfo || pCameraInfo->eProjType != PROJ_PERSPECTIVE)
	{
		return NULL;
	}

	if ( nCameraViewType == VIEW_3RDPERSON )
	{
		VECTOR vFaceDirectionNoZ = UnitGetFaceDirection( pUnit, FALSE );
		vFaceDirectionNoZ.fZ = 0.0f;
		VectorNormalize( vFaceDirectionNoZ );

		if ( ! GetLinePlaneIntersection( pCameraInfo->vPosition, pCameraInfo->vLookAt, 		
			UnitGetPosition( pUnit ), vFaceDirectionNoZ, pvPosition[ 0 ] ) )	
		{
			pvPosition[ 0 ] = pCameraInfo->vPosition;
		}

		pvDirection[ 0 ] = pCameraInfo->vDirection;
		pvPosition[ 1 ] = UnitGetCenter( pUnit );
		pvDirection[ 1 ] = vFaceDirectionNoZ;
	}
	else if ( pWeapon )
	{
		UnitGetWeaponPositionAndDirection( pGame, pUnit, nWeaponIndex, NULL, &pvDirection[ 0 ] );
		pvPosition[ 0 ] = pCameraInfo->vPosition;
		pvPosition[ 1 ] = pvPosition[ 0 ];
		pvDirection[ 1 ] = CameraInfoGetDirection(pCameraInfo);
	}
	else 
	{
		pvPosition[ 0 ]  = pCameraInfo->vPosition;
		pvDirection[ 0 ] = CameraInfoGetDirection(pCameraInfo);
		pvPosition[ 1 ] = pvPosition[ 0 ];
		pvDirection[ 1 ] = pvDirection[ 0 ];
	}

	UNIT * pTarget = NULL;
	DWORD dwSelectFlags = SELECTTARGET_FLAG_ONLY_IN_FRONT_OF_UNIT;
	float fLevelLineCollideResults[ NUM_TARGETING_DIRECTIONS ] = { -1, -1 };
	for ( int i = 0; i < 2; i++ )
	{
		pTarget = sSelectTargetInDirection( pGame, pUnit, pvPosition[ 0 ], pvDirection[ 0 ], 
			nSkill, pWeapon, dwSelectFlags, INVALID_ID, &fLevelLineCollideResults[ 0 ] );
		if ( pTarget )
			break;

		if ( pvDirection[ 0 ] != pvDirection[ 1 ] )
		{
			pTarget = sSelectTargetInDirection( pGame, pUnit, pvPosition[ 1 ], 
				pvDirection[ 1 ], nSkill, pWeapon, dwSelectFlags,
				INVALID_ID, &fLevelLineCollideResults[ 1 ] );
			if ( pTarget )
				break;
		} 
		dwSelectFlags |= SELECTTARGET_FLAG_ALLOW_NOTARGET;
	}

	if ( pTarget && nWeaponIndex != INVALID_ID )
		UnitUpdateWeaponPosAndDirecton( pGame, pUnit, nWeaponIndex, pTarget, FALSE );

	return pTarget;
}
#endif //!ISVERSION(SERVER_VERSION)


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const MELEE_WEAPON_DATA * WeaponGetMeleeData(
	GAME * pGame,
	UNIT * pUnit )
{
	const UNIT_DATA * item_data = ItemGetData(pUnit);
	ASSERT_RETNULL(item_data);
	if (item_data->nMeleeWeapon == INVALID_ID)
	{
		return NULL;
	}
	return (MELEE_WEAPON_DATA *) ExcelGetData( pGame, DATATABLE_MELEEWEAPONS, item_data->nMeleeWeapon );
}


//----------------------------------------------------------------------------
// This differs from WeaponGetMeleeData in that it is NOT const
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
MELEE_WEAPON_DATA * c_WeaponGetMeleeData(
	GAME * pGame,
	UNIT * pUnit )
{
	const UNIT_DATA * item_data = ItemGetData(pUnit);
	ASSERT_RETNULL(item_data);
	if (item_data->nMeleeWeapon == INVALID_ID)
	{
		return NULL;
	}
	return (MELEE_WEAPON_DATA *) ExcelGetData( pGame, DATATABLE_MELEEWEAPONS, item_data->nMeleeWeapon );
}
#endif //!ISVERSION(SERVER_VERSION)


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAddEffectToList(
	const char * pszName,
	const char * pszFolder,
	int & nParticleDefId,
	int * pnParticleDefArray,
	int & nParticleDefCount )
{
	char szParticleName[ DEFAULT_FILE_WITH_PATH_SIZE ];
	if ( pszName )
	{
		if ( pszName[ 0 ] == 0 )
		{
			nParticleDefId = INVALID_ID;
		}
		else 
		{
			char szFolder[ DEFAULT_FILE_WITH_PATH_SIZE ];
			PStrCopy( szFolder, pszFolder, DEFAULT_FILE_WITH_PATH_SIZE );
			PStrForceBackslash( szFolder, DEFAULT_FILE_WITH_PATH_SIZE );
			PStrPrintf( szParticleName, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s", szFolder, pszName );
			nParticleDefId = DefinitionGetIdByName( DEFINITION_GROUP_PARTICLE, szParticleName );
		}
	}

	if ( nParticleDefId != INVALID_ID )
	{
		pnParticleDefArray[ nParticleDefCount ] = nParticleDefId;
		nParticleDefCount++;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define MAX_PARTICLES_PER_HIT 6
#define MAX_SOUNDS_PER_HIT 6

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
static void c_sAddMeleeWeaponEffects(
	GAME * pGame,
	UNIT * pAttacker,
	UNIT * pDefender,
	BYTE bCombatResult,
	int pnParticleDefsToAdd[ MAX_PARTICLES_PER_HIT ],
	int & nParticleDefCount )
{
	MELEE_WEAPON_DATA * pMeleeWeaponData = c_WeaponGetMeleeData( pGame, pAttacker );
	if ( ! pMeleeWeaponData )
		return;

	ASSERT_RETURN( bCombatResult < NUM_COMBAT_RESULTS );
	PARTICLE_EFFECT_SET * pEffect = & pMeleeWeaponData->pEffects[ bCombatResult ];

	sAddEffectToList( pEffect->szDefault, pMeleeWeaponData->szEffectFolder, pEffect->nDefaultId, pnParticleDefsToAdd, nParticleDefCount );

	STATS_ENTRY pStatsEntry[MAX_DAMAGE_TYPES];
	int nCount = UnitGetStatsRange( pAttacker, STATS_MELEE_IMPACT_TYPE, 0, pStatsEntry, arrsize(pStatsEntry));
	int nElementIndex = 0;
	if ( nCount )
	{
		nElementIndex = RandGetNum(pGame, nCount);
	} 
	
	PARTICLE_ELEMENT_EFFECT *pElementEffect = &pEffect->tElementEffect[ nElementIndex ];
	sAddEffectToList( 
		pElementEffect->szPerElement, 
		pMeleeWeaponData->szEffectFolder, 
		pElementEffect->nPerElementId, 
		pnParticleDefsToAdd, 
		nParticleDefCount );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int c_sCalcGetHitSound(
	GAME * pGame,
	UNIT * pAttacker,
	UNIT * pDefender,
	BYTE bCombatResult )
{
	const UNIT_DATA * pUnitData = UnitGetData( pDefender );
	if ( pUnitData->pnGetHitSounds[ UNIT_HIT_SOUND_COUNT - 1 ] == INVALID_ID )
		return INVALID_ID; // they don't have these sounds... don't worry about it

	if( bCombatResult == COMBAT_RESULT_FUMBLE )
	{
		bCombatResult = COMBAT_RESULT_HIT_TINY;
	}
	static const float pfDangerPerResult[ NUM_UNIT_COMBAT_RESULTS ] = 
	{
		 3.0f,	//COMBAT_RESULT_HIT_TINY = 0,
		10.0f,	//COMBAT_RESULT_HIT_LIGHT,
		30.0f,	//COMBAT_RESULT_HIT_MEDIUM,
		50.0f,	//COMBAT_RESULT_HIT_HARD,
		99.0f,	//COMBAT_RESULT_KILL,
		 0.0f,	//COMBAT_RESULT_BLOCK,
	};
	ASSERT_RETINVALID( bCombatResult < NUM_UNIT_COMBAT_RESULTS );
	int nHpMax = UnitGetStat( pDefender, STATS_HP_MAX );
	if ( nHpMax == 0 )
		return INVALID_ID;

	float fHitpointPercent = (float) UnitGetStat( pDefender, STATS_HP_CUR ) / (float) nHpMax;

	int nHitSound = 0;

	if ( fHitpointPercent <= 0.0f )
		return pUnitData->pnGetHitSounds[ UNIT_HIT_SOUND_COUNT - 1 ];
	else {
		float fDanger = pfDangerPerResult[ bCombatResult ] / fHitpointPercent;
		const float fDangerMax = pfDangerPerResult[ COMBAT_RESULT_KILL ] / 0.05f;
		if ( fDanger > fDangerMax )
			fDanger = fDangerMax;

		//const float fAlwaysPlayMark = pfDangerPerResult[ COMBAT_RESULT_HIT_MEDIUM ] / 1.0f;
		//float fChanceToPlay = sqrt( fDanger / fAlwaysPlayMark );

		//if ( RandGetFloat( pGame ) > fChanceToPlay )
		//	return INVALID_ID;

		// throttle the low end
		if ( fDanger < 5.0f )
		{
			if ( RandGetFloat( pGame ) > fDanger / 10.0f )
				return INVALID_ID;
		}

		static const float pfDangerToGetHitSound[ UNIT_HIT_SOUND_COUNT ] = 
		{
			 8.0f,	
			15.0f,	
			30.0f,	
			40.0f,	
			50.0f,	
			99.0f,	
		};
		for ( int i = 0; i < UNIT_HIT_SOUND_COUNT - 1; i++ )
		{
			if ( pfDangerToGetHitSound[ i ] > fDanger )
				return pUnitData->pnGetHitSounds[ nHitSound ];
		}
		return pUnitData->pnGetHitSounds[ UNIT_HIT_SOUND_COUNT - 1 ];
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_UnitHitUnit(
	GAME * pGame,
	UNITID idAttacker,
	UNITID idDefender,
	BYTE bCombatResult,
	int nHitState,
	BOOL bIsMelee)
{
	UNIT * pDefender = UnitGetById( pGame, idDefender );
	if ( ! pDefender )
		return;

	UNIT * pAttacker = UnitGetById( pGame, idAttacker );

	if ( pAttacker )
	{
		UNIT * pUnitToTest = pAttacker;
		UNIT * pContainer = UnitGetContainer( pAttacker );
		if ( pContainer && UnitIsPhysicallyInContainer( pAttacker ) )
			pUnitToTest = pContainer;
		if ( AppIsHellgate() && pUnitToTest == GameGetControlUnit( pGame ) && bCombatResult != COMBAT_RESULT_BLOCK )
		{
			int nStateToUse = bCombatResult < COMBAT_RESULT_HIT_MEDIUM ? STATE_HIT_BY_CONTROL_UNIT : STATE_HIT_BY_CONTROL_UNIT_HARD;
			const STATE_DATA* state_data = (const STATE_DATA*)ExcelGetData( pGame, DATATABLE_STATES, nStateToUse );
			if(state_data)
			{
				c_StateSet( pDefender, pUnitToTest, nStateToUse, 0, state_data->nDefaultDuration, INVALID_ID );
			}
		}
	}

	if ( UnitHasState( pGame, pDefender, STATE_HAS_GETHIT_PARTICLES ) )
	{
		if ( !pAttacker || !UnitIsA( pAttacker, UNITTYPE_MELEE ))
			return;

	} 
	else 
	{// mark the unit so that we don't do this too often
		const STATE_DATA* state_data = (const STATE_DATA*)ExcelGetData( pGame, DATATABLE_STATES, STATE_HAS_GETHIT_PARTICLES );
		if(state_data)
		{
			if( AppIsHellgate() )
			{
				c_StateSet( pDefender, pDefender, STATE_HAS_GETHIT_PARTICLES, 0, state_data->nDefaultDuration, INVALID_ID );
			}
			else
			{
				c_StateSet( pDefender, pDefender, STATE_HAS_GETHIT_PARTICLES, 0, state_data->nDefaultDuration / 2, INVALID_ID );
			}
		}
	}

	if ( nHitState > 0 )
	{// set the hit state
		const STATE_DATA* state_data = (const STATE_DATA*)ExcelGetData( pGame, DATATABLE_STATES, nHitState );
		if(state_data)
		{
			c_StateSet( pDefender, pDefender, nHitState, 0, state_data->nDefaultDuration, INVALID_ID );
		}
	}

	int pnParticleDefsToAdd[ MAX_PARTICLES_PER_HIT ];
	int nParticleDefCount = 0;

	int pnSoundsToAdd[ MAX_SOUNDS_PER_HIT ];
	int nSoundCount = 0;

	const UNIT_DATA * pAttackerUnitData = UnitGetData( pAttacker );
	if ( pAttacker && UnitGetContainer( pAttacker ) && UnitIsPhysicallyInContainer( pAttacker ) )
	{ 
		UNIT * pAttackerHolder = UnitGetContainer( pAttacker );
		if ( pAttackerHolder && pAttackerHolder->pAppearanceOverrideUnitData ) // this helps when metamorph transforms
			pAttackerUnitData = pAttackerHolder->pAppearanceOverrideUnitData;
	}

	if ( bCombatResult == COMBAT_RESULT_BLOCK )
	{
		int nWardrobe = UnitGetWardrobe( pDefender );
		// We're not guaranteed that this unit has a wardrobe, but it's really okay if it doesn't.
		if( nWardrobe != INVALID_ID )
		{
			for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
			{
				UNIT * pShield = WardrobeGetWeapon( pGame, nWardrobe, i );
				if ( ! pShield )
					continue;
				if ( ! UnitIsA( pShield, UNITTYPE_SHIELD ) )
				{
					pShield = NULL;
					continue;
				}

				const UNIT_DATA * pShieldData = UnitGetData( pShield );
				if ( pShieldData && pShieldData->m_nBlockSound != INVALID_ID )
				{
					pnSoundsToAdd[ nSoundCount++ ] = pShieldData->m_nBlockSound;
				}
			}
		}
	} else {
		if(bIsMelee)
		{
			if ( pAttackerUnitData && pAttackerUnitData->nDamagingSound != INVALID_ID )
			{ 
				pnSoundsToAdd[ nSoundCount++ ] = pAttackerUnitData->nDamagingSound;
			}
			if ( pAttackerUnitData && pAttackerUnitData->nDamagingMeleeParticleId != INVALID_ID )
			{ // we are only getting attacker info on a melee attack 
				pnParticleDefsToAdd[ nParticleDefCount++ ] = pAttackerUnitData->nDamagingMeleeParticleId;
			}
		}
		if ( pAttacker && UnitGetGenus(pAttacker) == GENUS_ITEM && bIsMelee )
		{
			c_sAddMeleeWeaponEffects( pGame, pAttacker, pDefender, bCombatResult, pnParticleDefsToAdd, nParticleDefCount );
		}

		int nBloodDefId = c_UnitGetBloodParticleDef( pGame, pDefender, bCombatResult );
		sAddEffectToList( NULL, NULL, nBloodDefId, pnParticleDefsToAdd, nParticleDefCount );

		int nGetHitSound = c_sCalcGetHitSound( pGame, pAttacker, pDefender, bCombatResult );
		if ( nGetHitSound != INVALID_ID )
			pnSoundsToAdd[ nSoundCount++ ] = nGetHitSound;
	}

	if ( nParticleDefCount == 0 && nSoundCount == 0 )
		return;

	// find the best position for the effects
	VECTOR vParticlePos = UnitGetAimPoint( pDefender );
	
	if ( pDefender != GameGetControlUnit( pGame ) )
	{// bump the point out toward the camera
		const CAMERA_INFO * pCamera = c_CameraGetInfo( FALSE );
		if ( pCamera )
		{
			VECTOR vDeltaToEye = CameraInfoGetPosition(pCamera) - vParticlePos;
			VectorNormalize( vDeltaToEye );
			vDeltaToEye *= c_UnitGetMeleeImpactOffset( pGame, pDefender );
			vParticlePos += vDeltaToEye;
		}
	}

	int nDefenderModelId = c_UnitGetModelId( pDefender );
	if ( nDefenderModelId == INVALID_ID )
		return;

	{ // adjust the position into the defender's model space
		const MATRIX * pmWorldScaledInverse = e_ModelGetWorldScaledInverse( nDefenderModelId );
		MatrixMultiply( &vParticlePos, &vParticlePos, pmWorldScaledInverse );
	}

	for ( int i = 0; i < nSoundCount; i++ )
	{
		// add the effects
		ATTACHMENT_DEFINITION tAttachDef;
		ZeroMemory( &tAttachDef, sizeof( ATTACHMENT_DEFINITION ) );
		tAttachDef.eType = ATTACHMENT_SOUND;
		tAttachDef.nAttachedDefId = pnSoundsToAdd[ i ];
		tAttachDef.nBoneId		  = INVALID_ID;
		tAttachDef.vPosition	  = vParticlePos;
		tAttachDef.nVolume		  = 1;

		c_ModelAttachmentAdd( nDefenderModelId, tAttachDef, "", TRUE, ATTACHMENT_OWNER_NONE, INVALID_ID, INVALID_ID );
	}
	
	if ( nParticleDefCount )
	{
		// add the effects
		ATTACHMENT_DEFINITION tAttachDef;
		ZeroMemory( &tAttachDef, sizeof( ATTACHMENT_DEFINITION ) );
		tAttachDef.eType = ATTACHMENT_PARTICLE;
		tAttachDef.nAttachedDefId = pnParticleDefsToAdd[ 0 ];
		tAttachDef.nBoneId		  = INVALID_ID;
		tAttachDef.vPosition	  = vParticlePos;

		int nAttachmentId = c_ModelAttachmentAdd( nDefenderModelId, tAttachDef, "", TRUE, ATTACHMENT_OWNER_NONE, INVALID_ID, INVALID_ID );

		int nParticleSystemFirst = INVALID_ID;
		{
			ATTACHMENT * pAttachment = NULL;
			if ( nAttachmentId != INVALID_ID )
			{
				pAttachment = c_ModelAttachmentGet( nDefenderModelId, nAttachmentId );
				if(pAttachment)
				{
					nParticleSystemFirst = pAttachment->nAttachedId;
				}
			} 
		}
		if ( nParticleSystemFirst == INVALID_ID )
			return;

		for ( int i = 1; i < nParticleDefCount; i++ )
		{
			c_ParticleSystemAddToEnd( nParticleSystemFirst, pnParticleDefsToAdd[ i ] );
		}

		// these must be last - it will change all of the systems that have been added
		c_ParticleSystemSetVisible( nParticleSystemFirst, TRUE );
		c_ParticleSystemSetDraw   ( nParticleSystemFirst, TRUE );
	}
}
#endif// !ISVERSION(SERVER_VERSION)
