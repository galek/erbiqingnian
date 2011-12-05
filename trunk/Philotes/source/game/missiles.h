//----------------------------------------------------------------------------
// missiles.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _MISSILES_H_
#define _MISSILES_H_


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#ifndef _EXCEL_H_
#include "excel.h"
#endif


//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct NEW_MISSILE_MESSAGE_DATA
{
	BYTE bCommand;
	MSG_STRUCT *pMessage;
};


//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------

inline const struct UNIT_DATA * MissileGetData(
	GAME * game,
	int nClass)
{
	return (const struct UNIT_DATA *)ExcelGetData(game, DATATABLE_MISSILES, nClass);
}

const struct UNIT_DATA * MissileGetData(
	GAME * game,
	UNIT * unit);

UNIT* MissileInit(
	GAME* game,
	int nClass,
	UNITID id,
	UNIT * pOwner,
	UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT ],
	int nSkill,
	int nTeam,
	struct ROOM* room,
	const VECTOR& vPosition,
	const VECTOR& vMoveDirection,
	const VECTOR& vFaceDirection,
	const BOOL *pbElementEffects = NULL );	// array of size NUM_DAMAGE_TYPES for what elemental effects this missile has

void MissileConditionalFree(
	UNIT *pMissile,
	DWORD dwFlags = 0 );

UNIT* MissileInitOnUnit(
	GAME* game,
	int nClass,
	UNITID id,
	UNIT * pOwner,
	UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT ],
	int nSkill,
	UNIT * pTarget,
	int nTeam,
	ROOM* room );

void UnitAttachToUnit( 
	UNIT * pUnit, 
	UNIT * pTarget );

//----------------------------------------------------------------------------
float MissileGetFireSpeed(
	GAME * pGame,
	const UNIT_DATA * pMissileData,
	UNIT * pOwner,
	UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT ],
	const VECTOR& vMoveDirectionIn);

enum MISSILE_FLAGS
{
	MF_FROM_MISSILE_BIT = 1,		// a missile fired from a missile (like blaze pistol missiles "firing" the missile that sticks to things")
	MF_PLACE_ON_CENTER_BIT = 2,		// place the missile on the center if it's a stick on hit missile
};

// a generalized function for firing a missile and telling clients about it
UNIT * MissileFire(
	GAME * pGame,
	int nMissileClass,
	UNIT * pOwner,
	UNIT * pWeapons[ MAX_WEAPONS_PER_UNIT ],
	int nSkill,
	UNIT * pTarget,
	const VECTOR & vTarget,
	const VECTOR & position,
	const VECTOR & vMoveDirectionInput,
	DWORD dwRandSeed,
	DWORD dwMissileTag,
	DWORD dwMissileFlags = 0,				// see MISILE_FLAGS
	const BOOL *pbElementEffects = NULL,	// array of size NUM_DAMAGE_TYPES for what elemental effects this missile has
	int nMissileNumber = 0);	// when firing multiple missiles, this is with Nth missile it is starting at 0

BOOL ExcelMissilesPostProcess( 
	struct EXCEL_TABLE * table);

int c_MissileEffectAttachToUnit( 
	GAME * pGame, 
	UNIT * pUnit, 
	UNIT * pWeapon, 
	int nMissileClass,
	enum MISSILE_EFFECT eEffect,
	struct ATTACHMENT_DEFINITION & tAttachDef,
	int nAttachmentOwnerType,
	int nAttachmentOwnerId,
	float fDuration = 0.0f,
	BOOL bForceNew = FALSE );

int c_MissileEffectCreate( 
	GAME * pGame, 
	UNIT * pUnit, 
	UNIT * pWeapon, 
	int nMissileClass,
	MISSILE_EFFECT eEffect,
	VECTOR & vPosition,
	VECTOR & vNormal,
	float fDuration = 0.0f );

void c_MissileEffectUpdateRopeEnd( 
	GAME * pGame, 
	UNIT * pUnit, 
	UNIT * pWeapon, 
	int nMissileClass,
	MISSILE_EFFECT eEffect,
	int nParentParticleSystem,
	BOOL bClearSystem );

float MissileGetRangePercent(
	UNIT * source,
	UNIT * pWeapon,
	const struct UNIT_DATA * missile_data);

void MissileSetFuse(
	UNIT * pMissile,
	int nTicks );

void MissileUnFuse(
	UNIT * pMissile);

int MissileGetParticleSystem(
	UNIT * pMissile,
	MISSILE_EFFECT eEffect );

BOOL MissilesFizzle(
	GAME* pGame,
	UNIT* pUnit,
	const struct EVENT_DATA& event_data);

BOOL MissilesReflect(
	GAME* pGame,
	UNIT* pUnit,
	const struct EVENT_DATA& event_data);

enum
{
	MCO_FIRE_AT_PREVIOUS_OWNER_BIT,
	MCO_DONT_RESET_RANGE_BIT,
};

void MissileChangeOwner(
	GAME * pGame,
	UNIT * pMissile,
	UNIT * pNewOwner,
	UNIT * pWeapon,
	DWORD dwFlags);

void s_SendMissilesChangeOwner(
	UNIT * pNewOwner,
	UNIT * pWeapon,
	UNITID * pUnitIds,
	int nUnitIdCount,
	DWORD dwFlags);

void MissileSetHoming(
	UNIT *pMissile,
	int nMissileNumber,
	float fDistBeforeHoming = 1.5f );

BOOL MissileIsHoming(
	UNIT *pMissile);
	
UNIT *MissileGetHomingTarget(
	UNIT *pMissile);

void UnitDetach( 
	UNIT * pUnit );

void c_MissileDataInitParticleEffectSets( 
	UNIT_DATA * unit_data);

enum
{
	MBND_RETARGET_BIT,
	MBND_NEW_DIRECTION_BIT,
	MBND_HAS_SIDE_EFFECTS_BIT,
};

VECTOR MissileBounceFindNewDirection(
	GAME * pGame,
	UNIT * pSeekerUnit,
	VECTOR vSearchCenter,
	VECTOR vSearchDirection,
	UNIT * pTarget,
	const VECTOR & vNormal,
	DWORD dwFlags,
	float * pRange = NULL);

void c_MissileGetParticleEffectDefs( 
	int nMissileClass);

void c_MissileCreateField(
	GAME * pGame,
	UNIT * pOwner,
	int nFieldMissile,
	const VECTOR & vPosition,
	float fRadius,
	DWORD nDurationTicks);

void MissileGetReflectsAndRetargets(
	UNIT * pMissile,
	UNIT * pOwner,
	BOOL & bReflects,
	BOOL & bRetargets,
	int & nRicochetCount );

#endif // _MISSILES_H_
