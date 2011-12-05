#pragma once
// c_appearance.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//#include "prime.h"


#ifndef _DEFINITION_HEADER_INCLUDED_
#include "definition.h"
#endif


typedef enum UNITMODE;

// this gives space for the engine to store more or fewer bones depending on caps and targets
#define MAX_BONES_PER_MESH_RENDER_CHUNK		100

#define STANCE_DEFAULT 1

#define RAGDOLL_DELAY_ALLOWED_FOR_IMPULSE_IN_TICKS ((int)(0.25f * GAME_TICKS_PER_SECOND_FLOAT))
#define MAX_WARDROBE_LAYERS_PER_APPEARANCE 5

#define APPEARANCE_SUFFIX "_appearance.xml"

struct APPEARANCE;

struct APPEARANCE_SHAPE
{
	BYTE bHeight;
	BYTE bWeight;
};

enum WEIGHT_GROUP_TYPE
{
	WEIGHT_GROUP_THIN,
	WEIGHT_GROUP_THIN_LENGTHEN,
	WEIGHT_GROUP_STRONG,
	WEIGHT_GROUP_STRONG_LENGTHEN,
	WEIGHT_GROUP_SHORT,
	WEIGHT_GROUP_SHORT_SCALE,
	WEIGHT_GROUP_TALL,
	WEIGHT_GROUP_TALL_SCALE,
	WEIGHT_GROUP_ADD_RANDOM_ATTACHMENT,
	WEIGHT_GROUP_BONES_TO_SHRINK,
	NUM_WEIGHT_GROUP_TYPES,
};
const char * c_AppearanceDefinitionGetWeightGroupNameByType( 
	WEIGHT_GROUP_TYPE eType );

struct APPEARANCE_DEFINITION;
struct ANIMATION_DEFINITION;
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void c_AppearanceSystemInit();
void c_AppearanceSystemClose();
void c_AppearanceSystemAndHavokUpdate ( 
	struct GAME * pGame,
	float fDelta );

//-------------------------------------------------------------------------------------------------
void c_AppearanceToggleAnimTrace();

void AppearanceDefinitionDestroyNonDefData(
	APPEARANCE_DEFINITION * pDef,
	BOOL bFreePaths );

BOOL AppearanceDefinitionPostXMLLoad(
	void * pDef,
	BOOL bForceSync );

BOOL AppearanceDefinitionLoadSkeleton(
	APPEARANCE_DEFINITION * pAppearance,
	BOOL bForceSync );

int AppearanceDefinitionLoad(
	GAME * pGame,
	const char * pszName,
	const char * pszFolder);

void c_AppearanceDefinitionFlagSoundsForLoad(
	APPEARANCE_DEFINITION * pAppearanceDef,
	BOOL bLoadNow);

void c_AppearanceDefinitionApplyToModel( 
	int nAppearanceDefId, 
	int nModelId );

int c_AppearanceDefinitionGetModelDefinition( 
	int nAppearanceDefId );

float AppearanceDefinitionGetAnimationDuration ( 
	GAME* pGame, 
	int nAppearanceDef,
	int nAnimationGroup,
	UNITMODE eMode,
	BOOL & bHasMode,
	BOOL bIgnoreBackups =FALSE );

float AppearanceDefinitionGetContactPoint( 
	int nAppearanceDef, 
	int nAnimationGroup, 
	UNITMODE eMode, 
	float fPointValue = 0.0f);

const char * c_AppearanceDefinitionGetBoneName( 
	int nAppearanceDefId,
	int nBoneId );

int c_AppearanceDefinitionGetBoneNumber ( 
	int nAppearanceDefId, 
	const char * pszName );

int c_AppearanceDefinitionGetBoneCount( 
	int nAppearanceDefId );

BOOL c_AppearanceBonesLoaded( 
	int nAppearanceId );

void c_AppearanceCreateSkyboxModel(
	struct SKYBOX_DEFINITION * pSkyboxDef,
	struct SKYBOX_MODEL * pSkyboxModel,
	const char * pszFolder );

class hkSkeleton * c_AppearanceDefinitionGetSkeletonHavok( 
	int nAppearanceDefId );

void * c_AppearanceDefinitionGetSkeletonHavokVoid(
	int nAppearanceDefId );

struct granny_skeleton	*c_AppearanceDefinitionGetSkeletonGranny( 
	int nAppearanceDefId );

void AppearanceDefinitionClearWardrobe(
	GAME * pGame,
	APPEARANCE_DEFINITION * pDef );

void AppearanceDefinitionUpdateBoneWeights( 
	APPEARANCE_DEFINITION & tGlobalDefinition,
	APPEARANCE_DEFINITION & tDefinitionToUpdate	);

int c_AppearanceDefinitionGetNumBoneWeights(
	int nAppearanceDefId);

float * c_AppearanceDefinitionGetBoneWeights( 
	int nAppearanceDef, 
	int Type );

float * c_AppearanceDefinitionGetBoneWeightsByName( 
	int nAppearanceDefId, 
	const char * pszName );

const char * c_AppearanceDefinitionGetWeightGroupName( 
	APPEARANCE_DEFINITION * pDef,
	int nIndex );

const char * c_AppearanceDefinitionGetInventoryEnvName( 
	int nAppearanceDef );

BOOL c_AppearanceDefinitionTestModeBlends( 
	APPEARANCE_DEFINITION * pAppearanceDef,
	int nUnitMode );

void c_AppearanceDefinitionGetMaxRopeBends( 
	int nAppearanceDefId,
	float & fMaxBendXY,
	float & fMaxBendZ );

struct MATRIX * c_AppearanceDefinitionGetInverseSkinTransform(
	int nAppearanceDef );

void c_AnimatedModelDefinitionJustLoaded( 
	int nAppearanceDefId,
	int nModelDefinition );

float AppearanceDefinitionGetSpinSpeed( 
	int nAppearanceDefId);

void c_AppearanceForceInitAnimation( 
	int nModelId );


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
inline APPEARANCE_DEFINITION * AppearanceDefinitionGet( int nId )
{
	if ( nId == INVALID_ID )
		return NULL;
	return (APPEARANCE_DEFINITION*) DefinitionGetById( DEFINITION_GROUP_APPEARANCE, nId );
}

//-------------------------------------------------------------------------------------------------
#define APPEARANCE_NEW_FLAG_FORCE_WARDROBE						MAKE_MASK(0)
#define APPEARANCE_NEW_FLAG_WARDROBE_SHARES_MODELDEF			MAKE_MASK(1)
#define APPEARANCE_NEW_MASK_WARDROBE_MIP_BONUS					( MAKE_MASK(2) | MAKE_MASK(3) | MAKE_MASK(4) )
#define APPEARANCE_NEW_BITS_WARDROBE_MIP_BONUS					3
#define APPEARANCE_NEW_SHIFT_WARDROBE_MIP_BONUS					2
#define APPEARANCE_NEW_IGNORE_UNIT_WARDROBE_APPEARANCE_GROUP	MAKE_MASK(5)
#define APPEARANCE_NEW_DONT_DO_INIT_ANIMATION					MAKE_MASK(6)
#define APPEARANCE_NEW_FORCE_ANIMATE							MAKE_MASK(7)
#define APPEARANCE_NEW_FIRSTPERSON_WARDROBE_APPEARANCE_GROUP	MAKE_MASK(8)
#define APPEARANCE_NEW_DONT_FREE_WARDROBE						MAKE_MASK(9)
#define APPEARANCE_NEW_PLAY_IDLE								MAKE_MASK(10)
#define APPEARANCE_NEW_UNIT_IS_CONTROL_UNIT						MAKE_MASK(11)

void AppearanceShapeInit(
	APPEARANCE_SHAPE &tShape);
	
int c_AppearanceNew( 
	int nAppearanceDefId, 
	DWORD dwFlags,
	struct UNIT * pUnit = NULL,
	const struct UNIT_DATA * pUnitData = NULL,
	int nWardrobe = INVALID_ID,
	int nStartingStance = STANCE_DEFAULT,
	APPEARANCE_SHAPE * pAppearanceShape = NULL,
	int nWardrobeFallback = INVALID_ID );

void c_AppearanceDestroy( 
	int nAppearanceId );

void c_AppearanceSetModelDef(
	struct GAME * pGame,
	int nAppearance, 
	int nModelDef,
	int nAppearanceDefId );

void				c_AppearanceSetFullbright( int nAppearanceId,
											   BOOL bFullbright );

void				c_AppearanceSetDrawBehind( int nAppearanceId,
											   BOOL bDrawBehind );

void				c_AppearanceSetDrawBehind( APPEARANCE *pAppearance,
											   BOOL bDrawBehind );


void				c_AppearanceSetAmbientLight( int nAppearanceId,
												 float AmbientLight );

void				c_AppearanceSetHit( int nAppearanceId );

void				 c_AppearanceSetBehindColor( int nAppearanceId, 
												 float fBehindColor );

float				c_AppearanceGetModelAmbientLight( int nAppearanceId );

float				c_AppearanceGetHit( int nAppearanceId );

void c_AppearanceFreeze ( 
	int nAppearanceId,
	BOOL bValue = TRUE );

BOOL c_AppearanceIsFrozen ( 
	int nAppearanceId );

void c_AppearanceHideWeapons ( 
	int nAppearanceId,
	BOOL bValue = TRUE );

BOOL c_AppearanceAreWeaponsHidden ( 
	int nAppearanceId );

void c_AppearanceHide ( 
	int nAppearanceId,
	BOOL bValue = TRUE );

BOOL c_AppearanceIsHidden ( 
	int nAppearanceId );

BOOL c_AppearanceDestroyAllAnimations ( 
	int nAppearanceId );

int c_AppearanceGetDefinition(
	int nAppearanceId );

int c_AppearanceGetWardrobe(
	int nAppearanceId );

float c_AppearanceGetHeightFactor( 
	int idAppearance);

void c_AppearanceTrackBone( 
	int nAppearanceId,
	int nBone );

int c_AppearanceGetAimBone(
	int nAppearanceId,
	struct VECTOR & vOffset );

void c_AppearanceGetFootBones(
	int nAppearanceId,
	int & nLeftFootBone,
	int & nRightFootBone);

int c_AppearanceGetStance(
	int nAppearanceId );

BOOL c_AppearanceInitAttachmentOnMesh ( 
	struct GAME * pGame,
	int nAppearanceId,
	struct ATTACHMENT_DEFINITION & tAttachDef,
	const struct VECTOR & vSource,
	const struct VECTOR & vDirection );

int c_AppearanceShrunkBoneAdd(
	int nAppearance,
	int nBone = INVALID_ID );
void c_AppearanceShrunkBoneClearAll(
	int nAppearance );

void c_AppearanceAdjustBonesFromBoneWeightGroup(
	int nAppearance,
	int nWeightGroup,
	BOOL bLengthen );

//-------------------------------------------------------------------------------------------------
BOOL c_AppearanceRagdollMoves ( 
   int nAppearanceId );

void c_AppearanceFaceTarget (
	int nAppearanceId,
	const struct VECTOR & vTarget,
	BOOL bForce = FALSE );

//-------------------------------------------------------------------------------------------------
float * c_AppearanceGetBones ( 
	int nAppearanceId, 
	int nMeshIndex,
	int nLOD,
	BOOL bGetPrev = FALSE );	// CHB 2006.11.30

BOOL c_AppearanceGetBoneMatrix( 
	int nAnimatingModelId, 
	struct MATRIX * pmMatrix, 
	int nBoneNumber,
	BOOL bTrackedPosition = FALSE );

BOOL c_AppearanceGetBoneCompositeMatrix( 
	int nAnimatingModelId, 
	struct MATRIX * pmMatrix, 
	int nBoneNumber );

//-------------------------------------------------------------------------------------------------
BOOL c_AppearanceIsPlayingAnimation( 
	int nAppearanceId,
	ANIMATION_DEFINITION * pAnimDef );
BOOL c_AppearanceIsPlayingAnimation( 
	int nAppearanceId,
	int nUnitMode );
BOOL c_AppearanceIsApplyingAnimation( 
	int nAppearanceId,
	ANIMATION_DEFINITION *pAnimDef );

BOOL c_AppearancePlayAnimation ( 
	struct GAME * pGame, 
	int nAppearanceId, 
	ANIMATION_DEFINITION * pAnimation, 
	float fDuration, 
	DWORD dwFlags,
	struct ANIMATION_RESULT *pAnimResult,
	float *pfDurationUsed,
	float fEaseOverride );


BOOL c_AppearanceShouldPlayBlendingAnimation( 
	int nAppearanceId,
	int nUnitMode );

BOOL c_AppearanceEndAnimation(
	int nAppearanceId,
	int nUnitMode );

BOOL c_AppearanceSetAnimationSpeed(
	int nAppearanceId,
	int nUnitMode,
	float fAnimSpeed );

BOOL c_AppearanceSetAnimationPercent(
	int nAppearanceId,
	int nUnitMode,
	float fPercent );

BOOL c_AppearanceUpdateAnimationGroup( 
	UNIT * pUnit,
	struct GAME * pGame,
	int nAppearance,
	int nAnimationGroup );

void c_AppearanceUpdateSpeed( 
	int nAppearanceId, 
	float fSpeed );

float c_AppearanceGetTurnSpeed( 
	int nAppearanceId );

//#ifdef HAMMER
float c_AppearanceGetAnimationPercent( 
	int nAppearanceId,
	int nUnitMode,
	int & nOverflowCount );
//#endif

void c_AppearanceSetDebugDisplayAnimationPercent(
	float fPercent);

float c_AppearanceGetDebugDisplayAnimationPercent();

struct UNIT * c_AppearanceGetUnit(
	struct GAME * pGame,
	int nAppearanceId );

BOOL c_AppearanceIsAggressive(
	int nAppearanceId );

void c_AppearanceMakeAggressive(
	int nAppearanceId );

BOOL c_AppearanceUpdateContext( 
	int nModelId,
	BOOL bForce );

//-------------------------------------------------------------------------------------------------
typedef SIMPLE_DYNAMIC_ARRAY<DWORD> APPEARANCE_EVENTS;

APPEARANCE_EVENTS * c_AppearanceGetEvents( 
	int nAppearanceId );

void c_AppearanceRemoveEvent( 
	int nAppearanceId,
	DWORD dwTicket );

void c_AppearanceCancelAllEvents( 
	int nAppearanceId );

void c_AppearanceSyncMixableAnimations(
	int nAppearanceId,
	float fTimePercent);

float c_AppearanceAnimationGetDuration(
	int nAppearanceId,
	int nAnimationId);

BOOL c_AnimationHasFile( 
	int nAppearanceId, 
	int nAnimationId);

void c_AppearanceDumpAnims( 
	UNIT* pPlayer);

void AppearanceShapeRandomize( 
	const struct APPEARANCE_SHAPE_CREATE & tAppearanceShapeCreate,
	APPEARANCE_SHAPE & tAppearanceShape );

void c_AppearanceShapeSet( 
	int nAppearanceId,
	const APPEARANCE_SHAPE & tAppearanceShape );

const APPEARANCE_SHAPE * c_AppearanceShapeGet( 
	int nAppearanceId );

void AppearanceShapeRandomizeByQuality(
	UNIT * pUnit,
	int nQuality,
	const APPEARANCE_SHAPE_CREATE & tAppearanceShapeCreate);
	
BOOL AppearanceDefinitionGetWeaponOffset(
	int nAppearanceDefId,
	int nWeaponIndex,
	VECTOR & vOffset );

const struct INVENTORY_VIEW_INFO * AppearanceDefinitionGetInventoryView(
	int nAppearanceDefId,
	int nIndex );

int c_AppearanceGetModelDefinition(
	struct APPEARANCE * pAppearance );

BOOL c_AppearanceCanFaceCurrentTarget( 
	int nAppearanceId,
	float * pfDeltaFromTwist );

BOOL c_AppearanceCanTwistNeck( 
	int nAppearanceId );


//-------------------------------------------------------------------------------------------------

// CHB 2006.10.06
void c_AppearanceProbeAllAnimations(void);

// CHB 2006.11.28
void c_AppearanceLoadTextureOverrides(int pnOverrideIdsOut[], const char * pszFullPath, const char * pszTextureOverride, /*TEXTURE_TYPE*/ int nTextureType, int nModelDefId);

void c_AppearanceLoadTextureSpecificOverrides(int* pnOverrideIdOut, const char * pszFullPath, const char * pszTextureOverride, /*TEXTURE_TYPE*/ int nTextureType, int nModelDefId);
