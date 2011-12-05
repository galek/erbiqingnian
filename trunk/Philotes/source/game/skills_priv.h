//----------------------------------------------------------------------------
//	skills_priv.h
//  Copyright 2003, Flagship Studios
//----------------------------------------------------------------------------
#ifndef _SKILLS_PRIV_H_
#define _SKILLS_PRIV_H_

#define SKILL_TARGET_FLAG_IN_MELEE_RANGE_ON_START	MAKE_MASK( 0 )

UNIT * SkillGetLeftHandWeapon( 
	GAME * pGame,
	UNIT * pUnit,
	int nSkill,
	UNIT * pWeapon );

DWORD SkillGetSeed( GAME * pGame, UNIT * pUnit, UNIT * pWeapon, int nSkill );

void SkillGetFiringPositionAndDirection( 
	GAME * pGame, 
	UNIT * pUnit, 
	int nSkill,
	UNITID idWeapon,
	const SKILL_EVENT * pSkillEvent,
	const UNIT_DATA * missile_data,
	VECTOR & vPositionOut,
	VECTOR * pvDirectionOut,
	UNIT * pTarget,
	VECTOR * pvTarget,
	DWORD dwSeed,
	BOOL bDontAimAtCrosshairs = FALSE,
	float flHorizSpread = 0.0f,
	BOOL bClearSkillTargetsOutOfCone = FALSE );

int SkillGetTargetIndex(
	UNIT * pUnit,
	int nSkill,
	UNITID idWeapon,
	UNITID idTarget );

BOOL SkillRegisterEvent( 
	GAME * pGame, 
	UNIT * pUnit, 
	const SKILL_EVENT * pSkillEvent,
	int nSkill,
	int nSkillLevel,
	UNITID idWeapon,
	float fDeltaTime,
	float fDuration,
	float fTimeUntilSkillStop = 0.0f,
	DWORD_PTR pData = NULL);

struct SKILL_IS_ON * SkillIsOn( 
	UNIT * pUnit, 
	int nSkill,
	UNITID idWeapon,
	BOOL bIgnoreWeapon = FALSE );

void SkillCheckMeleeTargetRange(
	UNIT * pUnit,
	int nSkill,
	const SKILL_DATA * pSkillData,
	UNITID idWeapon,
	int nSkillLevel,
	BOOL bForce );

void SkillSendMessage(
	GAME* game,
	UNIT* unit,
	BYTE bCommand,
	MSG_STRUCT * msg,
	BOOL bForceSendToKnown = FALSE);

BOOL SkillHandleSkillEvent( 
	GAME* pGame,
	UNIT* unit,
	const struct EVENT_DATA& tEventData);

void SkillClearEventHandler(
	GAME * pGame,
	UNIT * pUnit,
	UNIT * pWeapon,
	UNIT_EVENT eEvent );

SKILL_EVENTS_DEFINITION * SkillGetWeaponMeleeSkillEvents( 
	GAME * pGame, 
	UNIT * pWeapon );

float SkillScheduleEvents( 
	GAME * game, 
	UNIT * unit, 
	int skill,
	int skillLevel,
	UNITID idWeapon,
	const SKILL_DATA * skillData,
	SKILL_EVENT_HOLDER * holder,
	float fTimeOffset,
	float fModeDuration,
	BOOL bNoModeEvent,
	BOOL bForceLoopMode,
	DWORD dwFlags);

float SkillGetLeapDuration( 
	UNIT * pUnit,
	int nSkill,
	UNITMODE eMode );

int SkillGetMode(
	GAME * pGame,
	UNIT * pUnit,
	int nSkill,
	UNITID idWeapon );

#endif // _SKILLS_PRIV_H_

