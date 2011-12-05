//----------------------------------------------------------------------------
// FILE: proc.h
//
// (C)Copyright 2005, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __PROC_H_
#define __PROC_H_

//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// FORWARD DECLARATION
//----------------------------------------------------------------------------
struct UNIT;
struct EVENT_DATA;

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
enum PROC_CONSTANTS
{
	MAX_PROC_SKILLS = 2,
};

//----------------------------------------------------------------------------
struct PROC_SKILL
{
	int nSkill;
	int nParam;
};

//----------------------------------------------------------------------------
enum PROC_FLAGS
{
	PROC_TARGET_INSTRUMENT_OWNER_BIT,
};

//----------------------------------------------------------------------------
struct PROC_DEFINITION
{
	char szName[ DEFAULT_INDEX_SIZE ];			// name
	WORD wCode;									// code 
	BOOL bVerticalCenter;						// target location is manually set at "targets vertical center"
	float flCooldownInSeconds;					// time required between execution of this proc
	DWORD dwFlags;								// see PROC_FLAGS
	float flDelayedProcTimeInSeconds;			// time to delay execution of skill
	PROC_SKILL tProcSkills[ MAX_PROC_SKILLS ];	// the skills to execute
};

//----------------------------------------------------------------------------
// FUNCTION PROTOTYPES
//----------------------------------------------------------------------------

PROC_DEFINITION *ProcGetDefinition(
	GAME *pGame,
	int nProc);

#if ISVERSION(CHEATS_ENABLED)
int ProcSetProbabilityOverride(
	int nProbability);
int ProcGetProbabilityOverride();
#endif
	
#if !ISVERSION(CLIENT_ONLY_VERSION)

void ProcOnAttack(
	GAME *pGame,
	UNIT *pInstrument,
	UNIT *pAttacker,
	EVENT_DATA *pEventData,
	void * /*pData*/,
	DWORD /*dwId */);

void ProcOnDidKill(
	GAME *pGame,
	UNIT *pWeapon,
	UNIT *pTarget,
	EVENT_DATA *pEventData,
	void * /*pData*/,
	DWORD /*dwId */);

void ProcOnDidHit(
	GAME *pGame,
	UNIT *pInstrument,
	UNIT *pTarget,
	EVENT_DATA *pEventData,
	void *pData,
	DWORD /*dwId */);

void ProcOnGotHit(
	GAME *pGame,
	UNIT *pInstrument,
	UNIT *pTarget,
	EVENT_DATA *pEventData,
	void *pData,
	DWORD /*dwId */);

void ProcOnGotKilled(
	GAME *pGame,
	UNIT *pInstrument,
	UNIT *pTarget,
	EVENT_DATA *pEventData,
	void * /*pData*/,
	DWORD /*dwId */);

#endif // #if !ISVERSION(CLIENT_ONLY_VERSION)
		
#endif
