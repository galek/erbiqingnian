#pragma once

#ifndef _EXCEL_H_
#include "excel.h"
#endif

#if !defined(GRANNY_H)
#include "granny.h"
#endif

#include "c_attachment.h"
#include "interpolationpath.h"

#ifndef _UNITTYPES_H_
#include "unittypes.h"
#endif

#ifndef _CONDITION_H_
#include "condition.h"
#endif


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
typedef enum {
	ANIM_EVENT_ADD_ATTACHMENT					= 0,
	ANIM_EVENT_REMOVE_ATTACHMENT				= 1,
	ANIM_EVENT_REMOVE_ALL_ATTACHMENTS			= 2,
	ANIM_EVENT_RAGDOLL_ENABLE					= 3,
	ANIM_EVENT_ANIMATION_FREEZE					= 4,
	ANIM_EVENT_RAGDOLL_DISABLE					= 5,
	ANIM_EVENT_DISABLE_DRAW						= 6,
	ANIM_EVENT_APPLY_FORCE_TO_RAGDOLL			= 7,
	ANIM_EVENT_DO_SKILL							= 8,
	ANIM_EVENT_CONTACT_POINT					= 9,
	ANIM_EVENT_ADD_ATTACHMENT_SHRINK_BONE		=10,
	ANIM_EVENT_ADD_ATTACHMENT_TO_RANDOM_BONE	=11,
	ANIM_EVENT_ENABLE_DRAW						=12,
	ANIM_EVENT_FLOAT_ATTACHMENTS_AT_BONES		=13,
	ANIM_EVENT_RAGDOLL_FALL_APART				=14,
	ANIM_EVENT_ADD_ATTACHMENT_TORCHLIGHT		=15,
	ANIM_EVENT_CAMERA_SHAKE						=16,
	ANIM_EVENT_SHOW_WEAPONS						=17,
	ANIM_EVENT_HIDE_WEAPONS						=18,
} ANIM_EVENT_TYPE;

#define MAYHEM_FILE_EXTENSION "hkx"

typedef SIMPLE_DYNAMIC_ARRAY<DWORD> APPEARANCE_EVENTS;


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
struct ANIM_EVENT 
{
	ANIM_EVENT_TYPE		  eType;
	float				  fTime;
	float				  fRandChance;
	float				  fParam;
	ATTACHMENT_DEFINITION tAttachmentDef;
	CONDITION_DEFINITION  tCondition;
};

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
#define ANIM_DEFAULT_OWNER_ID 1
#define ANIM_INIT_OWNER_ID	  2
#define MAX_ANIMATION_RESULT (64)  // arbitrary size for storage on the stack
#define NUM_ANIMATION_CONDITIONS 4

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
#define ANIMATION_DEFINITION_FLAG_FORCE_ANIMATION_SPEED	MAKE_MASK(0)
#define ANIMATION_DEFINITION_FLAG_PREVIEW_IMPACT		MAKE_MASK(1)
//#define ANIMATION_DEFINITION_FLAG_RAGDOLL_MOVES		MAKE_MASK(2)
#define ANIMATION_DEFINITION_FLAG_BLEND_WITH_POSE		MAKE_MASK(3)
#define ANIMATION_DEFINITION_FLAG_UNUSED				MAKE_MASK(4)
#define ANIMATION_DEFINITION_FLAG_PICK_VARIATION		MAKE_MASK(5)
//#define ANIMATION_DEFINITION_FLAG_ALWAYS_BLENDS		MAKE_MASK(6)
#define ANIMATION_DEFINITION_FLAG_CHANGE_STANCE			MAKE_MASK(7)
#define ANIMATION_DEFINITION_FLAG_USE_STANCE			MAKE_MASK(8)
#define ANIMATION_DEFINITION_FLAG_CAN_MIX				MAKE_MASK(9)
#define ANIMATION_DEFINITION_FLAG_DONT_RECOMPUTE		MAKE_MASK(10)
#define ANIMATION_DEFINITION_FLAG_HOLD_LAST_FRAME		MAKE_MASK(11)
#define ANIMATION_DEFINITION_FLAG_HOLD_STANCE			MAKE_MASK(12)
#define ANIMATION_DEFINITION_FLAG_MIX_BY_SPEED			MAKE_MASK(13)
#define ANIMATION_DEFINITION_FLAG_MATCH_MIX_ANIMSPEED	MAKE_MASK(14)
#define ANIMATION_DEFINITION_FLAG_NOT_IN_TOWN			MAKE_MASK(15)
#define ANIMATION_DEFINITION_FLAG_ONLY_IN_TOWN			MAKE_MASK(16)
#define ANIMATION_DEFINITION_FLAG_FORCE_SPEED			MAKE_MASK(17)
#define ANIMATION_DEFINITION_FLAG_FORCE_ON_CONDITION	MAKE_MASK(18)

struct ANIMATION_DEFINITION 
{
	DWORD dwFlags;

	int nUnitMode;
	int nGroup;

	char pszFile	[ MAX_XML_STRING_LENGTH ];
	int nFileIndex;
	class hkAnimationBinding	*pBinding;
	
	// for use by Tugboat - which doesn't have Havok
	struct granny_animation		*pGrannyAnimation;
	struct granny_file			*pGrannyFile;

	BOOL	bIsMobile;
	int nBoneWeights;

	float fStartOffset;
	float fDuration;
	float fVelocity;
	float fTurnSpeed;
	float fEaseIn;
	float fEaseOut;
	float fStanceFadeTimePercent;
	int nWeight;
	CInterpolationPath<CFloatPair>	tRagdollBlend;
	CInterpolationPath<CFloatPair>	tRagdollPower;
	CInterpolationPath<CFloatPair>	tSelfIllumation;
	CInterpolationPath<CFloatPair>	tSelfIllumationBlend;

	ANIM_EVENT * pEvents;
	int			 nEventCount;

	int nStartStance;
	int nStartStance2;
	int nStartStance3;
	int nEndStance;

	int nPriorityBoost;

	int nPreviewMode; // for tool use
	int nAnimationCondition[ NUM_ANIMATION_CONDITIONS ];		// index into animation_condition table
	CONDITION_DEFINITION  tCondition;

	ANIMATION_DEFINITION * pNextInGroup;
};

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
struct ANIMATION_GROUP_DATA 
{
	char	szName[ DEFAULT_INDEX_SIZE ];
	BOOL	bPlayRightLeftAnims;
	BOOL	bPlayLegAnims;
	BOOL	bOnlyPlaySubgroups;
	BOOL	bShowInHammer;
	BOOL	bCopyFootsteps;
	int		nDefaultStance;
	int		nDefaultStanceInTown;
	BOOL	bCanStartSkillWithLeftWeapon;
	float	fSecondsToHoldStance;
	float	fSecondsToHoldStanceInTown;
	// all of these reference other animation groups
	int		nFallBack;
	int		nRightWeapon;
	int		nLeftWeapon;
	int		nRightAnims; 
	int		nLeftAnims;
	int		nLegAnims;
};

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
typedef BOOL (*PFN_ANIMATION_CONDITION)( struct ANIMATION_CONDITION_PARAMS & tData );
struct ANIMATION_CONDITION_DATA
{
	char szName[ DEFAULT_INDEX_SIZE ];
	char szFunctionName[ DEFAULT_INDEX_SIZE ];
	SAFE_FUNC_PTR(PFN_ANIMATION_CONDITION, pfnHandler);
	int nPriorityBoostSuccess;
	int nPriorityBoostFailure;
	BOOL bRemoveOnFailure;

	BOOL bCheckOnPlay;
	BOOL bCheckOnContextChange;
	BOOL bCheckOnUpdateWeights;

	BOOL bClearRestingFlag;

	BOOL bIgnoreOnFailure;
	BOOL bIgnoreStanceOutsideCondition;
};

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
struct ANIMATION_STANCE_DATA 
{
	char	szName[ DEFAULT_INDEX_SIZE ];
	BOOL	bDontChangeFrom;
};

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
struct ANIMATION_RESULT_INSTANCE
{
	int nAppearanceId;
	int nAnimationId;
};

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
struct ANIMATION_RESULT
{
	ANIMATION_RESULT_INSTANCE tResults[ MAX_ANIMATION_RESULT ];  // new animations added
	int nNumResults;
	
	ANIMATION_RESULT::ANIMATION_RESULT( void ) { nNumResults = 0; }
	
};

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
#define PLAY_ANIMATION_FLAG_LOOPS					MAKE_MASK( 0 )
#define PLAY_ANIMATION_FLAG_ONLY_ONCE				MAKE_MASK( 1 )
#define PLAY_ANIMATION_FLAG_DEFAULT_ANIM			MAKE_MASK( 2 )
#define PLAY_ANIMATION_FLAG_ONLY_EVENTS				MAKE_MASK( 3 )
#define PLAY_ANIMATION_FLAG_FORCE_TO_PLAY			MAKE_MASK( 4 )
#define PLAY_ANIMATION_FLAG_RAND_START_TIME			MAKE_MASK( 5 )
#define PLAY_ANIMATION_FLAG_USES_SPEED				MAKE_MASK( 6 )
#define PLAY_ANIMATION_FLAG_ADJUSTING_CONTEXT		MAKE_MASK( 7 )
#define PLAY_ANIMATION_FLAG_USE_EASE_OVERRIDE		MAKE_MASK( 8 )

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
const ANIMATION_GROUP_DATA* AnimationGroupGetData(
	int nGroupIndex);

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
const ANIMATION_CONDITION_DATA* AnimationConditionGetData(
	int nConditionIndex);

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void c_AnimationsInit( 
	struct APPEARANCE_DEFINITION & tAppearanceDef,
	BOOL bForceSync = FALSE );

void c_AnimationsFlagSoundsForLoad( 
	struct APPEARANCE_DEFINITION & tAppearanceDef,
	BOOL bLoadNow);

void c_AnimationsFree( 
	struct APPEARANCE_DEFINITION * pAppearance,
	BOOL bFreePaths );

void c_AnimationPlayByMode( 
	UNIT * pUnit, 
	int nUnitMode, 
	float fDuration, 
	DWORD dwPlayAnimFlags,
	ANIMATION_RESULT *pAnimResult = NULL );

void c_AnimationPlayByMode( 
	UNIT * pUnit,
	GAME * pGame,
	int nModelId,
	int nUnitMode, 
	int nAnimationGroup,
	float fDuration, 
	DWORD dwPlayAnimFlags,
	BOOL bPlaySubgroups = TRUE,
	int nTargetStance = INVALID_ID,
	ANIMATION_RESULT *pAnimResult = NULL,
	float fEaseOverride = 0.0f,
	DWORD dwAnimationFlags = NULL,
	int nAnimation = INVALID_ID,
	int * pnUnitType = NULL );

BOOL c_AnimationEndByMode( 
	UNIT * pUnit, 
	int nUnitMode );

BOOL c_AnimationPlay( 
	GAME * pGame, 
	int nModelId, 
	ANIMATION_DEFINITION * pAnimDef, 
	float fDuration, 
	DWORD dwPlayAnimFlags,
	ANIMATION_RESULT *pAnimResult,
	float fEaseOverride = 0.0f );

void c_AnimationDoAllEvents( 
	GAME * pGame, 
	int nModelId, 
	int nOwnerId,
	ANIMATION_DEFINITION & tAnimDef,
	BOOL bClearAllAttachments );

bool c_AnimationHasFreezeEvent( 
	ANIMATION_DEFINITION & tAnimDef );

void c_AnimationScheduleEvents( 
	GAME * pGame, 
	int nModelId, 
	float fDuration,
	ANIMATION_DEFINITION & tAnimDef,
	BOOL bLooping,
	BOOL bClearAttachments,
	BOOL bOnlyEvents = FALSE );			// animation isn't played

void c_AnimationRescheduleEvents( 
	 const ANIMATION_DEFINITION *pAnimDef,
	 APPEARANCE_EVENTS * pEvents, 
	 float fDuration );					// must already be scaled by control speed

void c_AnimationCancelEvents( 
	const ANIMATION_DEFINITION *pAnimDef,
	APPEARANCE_EVENTS * pEvents );

void c_AnimationStopAll( 
	GAME * pGame, 
	int nModelId );

void c_AnimationComputeDuration( 
	ANIMATION_DEFINITION & tAnimation );
	
void c_AnimationComputeVelocity( 
	ANIMATION_DEFINITION & tAnimation );

ANIM_EVENT * c_AnimationEventAdd( 
	ANIMATION_DEFINITION & tAnimDef );

void c_AnimationComputeFileIndices( 
	APPEARANCE_DEFINITION * pAppearanceDef );

BOOL c_AnimationConditionEvaluate(
	GAME * pGame,
	UNIT* pUnit,
	int nAnimationCondition,
	int nModelId,
	ANIMATION_DEFINITION * pAnimDef );
	
BOOL ExcelAnimationConditionPostProcess(
	struct EXCEL_TABLE * table);

BOOL c_AnimationPlayTownAnims( 
	UNIT * pUnit );

