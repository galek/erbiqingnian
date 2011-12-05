//----------------------------------------------------------------------------
// unittypes.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _UNITMODES_H_
#define _UNITMODES_H_

#ifndef _UNITS_H_
#include "units.h"
#endif

#include "..\data_common\excel\unitmodes_hdr.h"

//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
enum UNITMODE_GROUP
{
	MODEGROUP_IDLE,
	MODEGROUP_ATTACK,
	MODEGROUP_MOVE,
	MODEGROUP_GETHIT,
	MODEGROUP_SOFTHIT,
	MODEGROUP_DYING,
	MODEGROUP_DEAD,
	MODEGROUP_ITEM_USING,
	MODEGROUP_ITEM_IN_INVENTORY,
	MODEGROUP_ITEM_HOVERING,
	MODEGROUP_OPEN,
	MODEGROUP_CLOSE,
	MODEGROUP_OPENED,
	MODEGROUP_NPC_DIALOG,
	MODEGROUP_LOITERING,
	MODEGROUP_EMOTE,
};

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct UNITMODEGROUP_DATA
{
	char			szName[ DEFAULT_INDEX_SIZE ];
	WORD			wCode;
	DWORD			pdwModeFlags[ DWORD_FLAG_SIZE( NUM_UNITMODES ) ];
};

//----------------------------------------------------------------------------
typedef BOOL (*PFN_UNITMODE_CLEAR)(UNIT* unit, UNITMODE from, UNITMODE mode);
typedef BOOL (*PFN_UNITMODE_DO)(UNIT* unit, UNITMODE mode);
typedef BOOL (*PFN_UNITMODE_END)(UNIT* unit, UNITMODE mode, WORD wParam);

//----------------------------------------------------------------------------
struct UNITMODE_DATA
{
	EXCEL_STR		(szName);								// key
	WORD			wCode;
	int				pnBlockingModes[MAX_UNITMODES];
	BOOL			bBlockOnGround;
	int				pnWaitModes[MAX_UNITMODES];
	int				pnClearModes[MAX_UNITMODES];
	int				pnBlockEndModes[MAX_UNITMODES];
	int				nGroup;
	UNITMODE		eOtherHandMode;
	UNITMODE		eBackupMode;
	BOOL			bForceClear;
	BOOL			bClearAi;
	BOOL			bClearSkill;
	int				nState;
	int				nClearState;
	int				nClearStateEnd;
	BOOL			bDoEvent;
	BOOL			bEndEvent;
	EXCEL_STR		(szDoFunction);							
	SAFE_FUNC_PTR(PFN_UNITMODE_DO, pfnDo);		// set at runtime
	EXCEL_STR		(szClearFunction);							
	SAFE_FUNC_PTR(PFN_UNITMODE_CLEAR, pfnClear);// set at runtime
	EXCEL_STR		(szEndFunction);							
	SAFE_FUNC_PTR(PFN_UNITMODE_END, pfnEnd);	// set at runtime
	int				nEndMode;
	int				nVelocityIndex;
	int				nVelocityPriority;
	int				nAnimationPriority;
	int				nLoadPriorityPercent;
	BOOL			bVelocityUsesMeleeSpeed;
	BOOL			bVelocityChangedByStats;
	BOOL			bNoLoop;
	BOOL			bRestoreAi;
	BOOL			bSteps;
	BOOL			bLazyEndForControlUnit;
	BOOL			bNoAnimation;
	BOOL			bRagdoll;
	BOOL			bPlayRightAnim;
	BOOL			bPlayLeftAnim;
	BOOL			bPlayTorsoAnim;
	BOOL			bPlayLegAnim;
	BOOL			bPlayJustDefault;
	BOOL			bIsAggressive;
	BOOL			bPlayAllVariations;
	BOOL			bResetMixableOnStart;
	BOOL			bResetMixableOnEnd;
	BOOL			bRandStartTime;
	BOOL			bDurationFromAnims;
	BOOL			bDurationFromContactPoint;
	BOOL			bClearAdjustStance;
	BOOL			bCheckCanInterrupt;
	BOOL			bUseBackupModeAnims;
	BOOL			bPlayOnInventoryModel;
	BOOL			bHideWeapons;
	BOOL			bEmoteAllowedHellgate;
	BOOL			bEmoteAllowedMythos;
};

//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------
BOOL ExcelUnitModesPostProcess(
	struct EXCEL_TABLE * table);

const UNITMODE_DATA* UnitModeGetData(
	int nModeIndex);

BOOL s_UnitSetMode(
	UNIT* unit,
	UNITMODE mode,
	WORD wParam,
	float fDurationInSeconds,
	float fDurationForEndEventInSeconds = 0,
	BOOL bSend = TRUE);

BOOL s_TryUnitSetMode(
	UNIT * unit,
	UNITMODE mode);

BOOL s_UnitSetMode(
	UNIT* unit,
	UNITMODE mode,
	BOOL bSend = TRUE);

BOOL c_UnitSetRandomModeInGroup(
	UNIT* unit,
	UNITMODE_GROUP modegroup);

BOOL UnitEndMode(
	UNIT* unit,
	UNITMODE mode,
	WORD wParam = 0,
	BOOL bSend = TRUE);

BOOL UnitEndModeGroup(
	UNIT * pUnit,
	UNITMODE_GROUP eGroup);

BOOL UnitTestModeGroup(
	UNIT * pUnit,
	UNITMODE_GROUP eModeGroup );

BOOL ModeTestModeGroup(
	UNITMODE eMode,
	UNITMODE_GROUP eModeGroup);

BOOL c_UnitSetMode(
	UNIT* unit,
	UNITMODE mode,
	WORD wParam = 0,
	float fDurationInSeconds = 0.0f);

BOOL UnitModeIsJumping(
	UNIT *pUnit);

BOOL UnitModeIsJumpRecovering(
	UNIT *pUnit);

void UnitModeStartJumping(
	UNIT *pUnit);

void UnitModeEndJumping(
	UNIT *pUnit,
	BOOL bWarping );
	
void c_UnitInitializeAnimations(
	UNIT* unit);

UNITMODE UnitGetMoveMode(
	UNIT* unit);

void UnitClearStepModes(
	UNIT *unit);

void UnitStepTargetReached(
	UNIT* unit);

BOOL c_UnitModeOverrideClientPathing(
	UNITMODE eMode);

inline BOOL UnitTestMode(
	const UNIT * unit,
	UNITMODE mode)
{
	ASSERT_RETFALSE(unit);
	ASSERT_RETFALSE((unsigned int)mode < NUM_UNITMODES);
	return TESTBIT(unit->pdwModeFlags, mode);
}


//----------------------------------------------------------------------------
// Externals
//----------------------------------------------------------------------------
extern float JUMP_VELOCITY;

#endif // _UNITMODES_H_