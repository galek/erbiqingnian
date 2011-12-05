//----------------------------------------------------------------------------
// weapons.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#pragma once

#ifndef _WARDROBE_H_
#include "wardrobe.h"
#endif


#define MUZZLE_BONE_DEFAULT "Muzzle"

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

int c_WeaponGetAttachmentBoneIndex( 
	int nAppearanceDef, 
	int nWeaponIndex,
	int nWeaponBoneType = 0 );

const char * c_WeaponGetAttachmentBoneName( 
	int nAppearanceDef, 
	int nWeaponIndex,
	int nWeaponBoneType,
	VECTOR & vNormal,
	float & fRotation );

void c_WeaponGetAimPosition( GAME *pGame, float *pfRotation, float *pfPitch );

void c_UnitHitUnit(
	GAME * pGame,
	UNITID idAttacker,
	UNITID idDefender,
	BYTE bCombatResult,
	int nHitState,
	BOOL bIsMelee);

void UnitUpdateWeaponPosAndDirecton( 
	GAME * pGame,
	UNIT * pUnit,
	int nWeaponIndex,
	UNIT * pTargetUnit,
	BOOL bForceAimAtTarget,
	BOOL bDontAimAtCrosshairs = FALSE,
	BOOL* bInvalidTarget = NULL );

UNIT * c_WeaponFindTargetForControlUnit( 
	GAME * pGame,
	int nWeaponIndex,
	int nSkill = INVALID_ID,
	UNITID idPrevious = INVALID_ID );

void c_WeaponSetLockOnVanity(
	BOOL bLock);
BOOL c_WeaponGetLockOnVanity();