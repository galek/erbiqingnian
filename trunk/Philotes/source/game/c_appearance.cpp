/////////////////////////////////////////////////////////////////////////////////////////////////
//
//	File: c_appearance.cpp
//
//	Author : Flagship Studios
//			
//	(C)Copyright 2003, Flagship Studios. All rights reserved.
//
//
//	CHANGELOG :
//
//		10-16-06 : Travis - when I merged the appearanceplayanim code, I forgot to pull in Tugboat's specific Granny blending functionality,
//							which was causing some anims not to play, or to blend incorrectly.
//
//		10-11-06 : Travis - Granny anims were not properly playing - was only checking pBinding for Havok anims when starting the anim. Works now.
//							Wardrobe anims still not playing - I think this is a skeleton issue in wardrobe
//
//		10-9-06 : Travis - removed unitspeed functions - unitspeed is now retrieved in appearance when the animation is played
//
//		9-29-06 : Travis - Merged Hellgate and Tugboat appearance code.
//
//		9-28-06 : Travis - Removed Havok commented code, reformatted source, added function-bracketing comments
//
////////////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "prime.h"
#include "c_appearance.h"
#include "c_appearance_priv.h" // also includes units.h, which includes game.h
#include "dungeon.h"
#include "c_granny_rigid.h"
#include "granny.h"
#include "appcommon.h"
#include "appcommontimer.h"
#include "filepaths.h"
#include "performance.h"
#include "level.h"
#include "config.h"
#include "wardrobe.h"
#include "wardrobe_priv.h"
#include "uix_scheduler.h"
#ifdef HAVOK_ENABLED
	#include "c_ragdoll.h"
#endif
#include "monsters.h"
#include "unitmodes.h"
#include "c_attachment.h"
#include "pakfile.h"
#include "e_anim_model.h"
#include "e_model.h"
#include "e_texture.h"
#include "unit_priv.h"
#include "c_camera.h"
#include "e_drawlist.h"
#include "e_common.h"
#include "e_settings.h"

#include "e_havokfx.h"
#include "performance.h"
#include "perfhier.h"
#include "skills.h"
#include "definition_priv.h"

#define MIN_BONES_FOR_SHRINKING					4
#define MAX_ANIMS_PER_APPEARANCE				3000
#define APPEARANCE_ANIMATION_HASH_SIZE  		5
#define MAX_CONTROL_SPEED 						100.0f
granny_local_pose *g_ptSharedLocalPose = NULL;
#ifdef HAVOK_ENABLED
hkDefaultChunkCache* g_havokChunkCache = NULL;

//hkLocalArray<hkQsTransform> * sgptSharedLocalPoseMayhem = NULL;
//#define HK_CACHE_STATS
#ifdef HK_CACHE_STATS
static int havokCacheStatsCounter = 0;
#endif
#include <hkbase/stream//impl/hkPrintfStreamWriter.cxx>
#endif

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static float sgfMaxDistanceForFullAnim = 20.0f;
static float sgfSampleReductionFactor = 40.0f;
static APPEARANCE_METRICS sgtAppearanceMetrics;
extern BOOL sgbHavokOpen;
static BOOL sgbAnimTrace = FALSE;
static void sAppearanceDefinitionLoadWithSkeleton( APPEARANCE_DEFINITION *pAppearanceDef );
static int sAppearanceDefinitionGetModelDefinition( APPEARANCE_DEFINITION *pAppearanceDef );

const float kEaseInConstant = .25f;
const float kEaseOutConstant = .2f;


static const char *sgpszWeightGroupNames[NUM_WEIGHT_GROUP_TYPES] =
{
	"Look Thin",						// WEIGHT_GROUP_THIN, 
	"Look Thin Lengthen", 				// WEIGHT_GROUP_THIN_LENGTHEN, 
	"Look Strong",						// WEIGHT_GROUP_STRONG, 
	"Look Strong Lengthen",				// WEIGHT_GROUP_STRONG_LENGTHEN, 
	"Look Short", 						// WEIGHT_GROUP_SHORT, 
	"Look Short Scale",   				// WEIGHT_GROUP_SHORT_SCALE, 
	"Look Tall",  						// WEIGHT_GROUP_TALL, 
	"Look Tall Scale",					// WEIGHT_GROUP_TALL_SCALE, 
	"Random Attachments", 				// WEIGHT_GROUP_ADD_RANDOM_ATTACHMENT, 
	"Bones To Shrink",					// WEIGHT_GROUP_BONES_TO_SHRINK, 
};
#define WEIGHT_GROUP_NAME_LOOK_THIN 						"Look Thin"
#define WEIGHT_GROUP_NAME_LOOK_STRONG   					"Look Strong"
#define WEIGHT_GROUP_NAME_LOOK_SHORT						"Look Short"
#define WEIGHT_GROUP_NAME_LOOK_TALL 						"Look Tall"
#define WEIGHT_GROUP_NAME_RANDOM_ATTACHMENT					"Random Attachments"
#define WEIGHT_GROUP_NAME_BONES_TO_SHRINK   				"Bones To Shrink"

#define MOTION_BLUR
/////////////////////////////////////////////////////////////////////////////
// Appearance Mesh  
/////////////////////////////////////////////////////////////////////////////
struct APPEARANCE_MESH
{
	float pfBoneMatrices3x4[3 * 4 * MAX_BONES_PER_MESH_RENDER_CHUNK]; // our copy of the bones after being transformed 
#ifdef MOTION_BLUR
	float pfPrevBoneMatrices3x4[3 * 4 * MAX_BONES_PER_MESH_RENDER_CHUNK]; // our copy of the previous bones before being updated
#endif
};

/////////////////////////////////////////////////////////////////////////////
// Animation Instance 
/////////////////////////////////////////////////////////////////////////////
#define ANIMATION_FLAG_EASING_IN						MAKE_MASK( 0 )
#define ANIMATION_FLAG_EASING_OUT   					MAKE_MASK( 1 )
#define ANIMATION_FLAG_IS_ENABLED   					MAKE_MASK( 2 )
#define ANIMATION_FLAG_LAST_OF_SIMILAR  				MAKE_MASK( 3 )
#define ANIMATION_FLAG_DURATION_ADJUSTED				MAKE_MASK( 4 )
#define ANIMATION_FLAG_SPEED_CONTROLS_ANIMSPEED			MAKE_MASK( 5 )
#define ANIMATION_FLAG_REMOVE							MAKE_MASK( 6 )
struct ANIMATION
{
	int						nId;					// for hash
	ANIMATION				*pNext; 				// for hash 
	DWORD					dwPlayFlags;
	DWORD					dwFlags;
	float					fMovementSpeed;
	struct granny_control	*pGrannyControl;
#ifdef HAVOK_ENABLED
	class CMyAnimControl	*pControl;
#endif
	ANIMATION_DEFINITION	*pDefinition;
	float					fTimeSinceStart;
	float					fDuration;
#ifdef HAVOK_ENABLED
	class MyControlListener	*pListener;
#endif
	int						nPriority;
	int						nSortValue;
	float					fAnimSpeed;
	float					fMaxMasterWeight;
};

__declspec(align(16)) struct BONE_TO_TRACK
{
#ifdef HAVOK_ENABLED
	hkQsTransform mBoneInModel;
#endif
	int nBone;
};



#define APPEARANCE_FLAG_HAS_ANIMATED						MAKE_MASK( 0 )
#define APPEARANCE_FLAG_FREEZE_ANIMATION					MAKE_MASK( 1 )
#define APPEARANCE_FLAG_UPDATE_WEIGHTS  					MAKE_MASK( 2 )
#define APPEARANCE_FLAG_CHECK_FOR_LOADED_ANIMS  			MAKE_MASK( 3 )
#define APPEARANCE_FLAG_LAST_FRAME  						MAKE_MASK( 4 )
#define APPEARANCE_FLAG_AGGRESSIVE  						MAKE_MASK( 5 )
#define APPEARANCE_FLAG_ADJUSTING_CONTEXT   				MAKE_MASK( 6 )
#define APPEARANCE_FLAG_IS_IN_TOWN							MAKE_MASK( 7 )
#define APPEARANCE_FLAG_HIDDEN								MAKE_MASK( 8 )
#define APPEARANCE_FLAG_FADE_OUT_FACING						MAKE_MASK( 9 )
#define APPEARANCE_FLAG_NEEDS_BONES_LOD_0 					MAKE_MASK(10 )
#define APPEARANCE_FLAG_NEEDS_BONES_LOD_1 					MAKE_MASK(11 )
#define APPEARANCE_MASK_NEEDS_BONES		 					(APPEARANCE_FLAG_NEEDS_BONES_LOD_0 | APPEARANCE_FLAG_NEEDS_BONES_LOD_1)
#define APPEARANCE_FLAG_ANIMS_MARKED_FOR_REMOVE				MAKE_MASK(12 )
#define APPEARANCE_FLAG_MUST_TRANSFER_POSE					MAKE_MASK(13 )
#define APPEARANCE_FLAG_HIDE_WEAPONS						MAKE_MASK(14 )

/////////////////////////////////////////////////////////////////////////////
// Appearance - these share ids with models 
/////////////////////////////////////////////////////////////////////////////
struct APPEARANCE
{
	int									nId;				// needed for the hash 
	APPEARANCE							*pNext;

	DWORD								dwFlags;

	DWORD								dwNewFlags;
	int									nDefinitionId;
	int									nModelDefinitionId;	// CHB 2006.10.17 - Referring to ANIMATING_MODEL_DEFINITION
	UNITID								idUnit;
	int									nMeshCount_[LOD_COUNT];	// CHB 2006.11.30
	APPEARANCE_MESH						*pMeshes_[LOD_COUNT];	// CHB 2006.11.30

#ifdef HAVOK_ENABLED
	hkAnimatedSkeleton		*pAnimatedSkeleton;
	hkArray<hkQsTransform>  pPoseInModel;
	hkArray<hkQsTransform>  pPoseInLocal;
	hkListShape				*pPoseShape;
	hkSimpleShapePhantom	*pPosePhantom;
#endif

	granny_model_instance				*pGrannyInstance;   // Granny's instance of this model 
	granny_world_pose					*pWorldPose;		// The current world-space state of the model 

	SIMPLE_DYNAMIC_ARRAY<MATRIX>		pGrannyPoseInModel;
	SIMPLE_DYNAMIC_ARRAY<MATRIX>		pGrannyPoseInLocal;

#ifdef HAVOK_ENABLED
	SIMPLE_DYNAMIC_ARRAY_ALIGNED<BONE_TO_TRACK,16> pBonesToTrack;
#endif

	float								fApproximateSize;   // used to adjust sample timing 
	float								fAnimSampleTimer;
	float								fAnimSampleTimeDelay;

	float								fRagdollBlend;
	float								fRagdollPower;

	float								fHeadTurnGain;

	int									nStanceCurrent;
	DWORD								nLastAdjustContextTick;
	float								fTime;
	int									nAnimationGroup;
	CHash<ANIMATION>					pAnimations;
	int									nNextAnimID;

	float								fSpeed;

	BOOL								bFullbright;
	BOOL								bDrawBehind;
	BOOL								bHit;
	float								fBehindColor;
	float								AmbientLight;

	APPEARANCE_SHAPE					tAppearanceShape;
	float								*pfFattenedBones;
	float								*pfLengthenedBones;
	int									*pnShrunkBones;
	int									nShrunkBoneCount;
	VECTOR								vFacingTarget;  		// this is for turning the neck - world  position to look at
	VECTOR								vFacingTargetLastGood; 	// this is the last target that we could twist and see - model relative
	VECTOR								vFacingDirection;		// this is for turning the neck - relative direction last frame 

	APPEARANCE_EVENTS					tEventIds;

	int									nWardrobe;
};

static CHash<APPEARANCE> sgpAppearances;
#define POSE_PHANTOM 0 // I don't think that we are really using these anymore, but they are causing crashes

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////

struct SKELETON_FILE_CALLBACK_DATA
{
	int nAppearanceDefinitionId;
};

struct ANIM_GROUP
{
	int						nId;
	ANIMATION_DEFINITION	*pAnimDefs[NUM_UNITMODES];
	float					pfDurations[NUM_UNITMODES];
};

static BOOL sAppearanceUpdateContext( 
	APPEARANCE &tAppearance,
	ANIMATION *pChangingAnimation,
	ANIMATION_DEFINITION *pAnimDef,
	BOOL bForce = FALSE );

static void sAnimationRemove( APPEARANCE *pAppearance,
							  int nAnimationId );
static void c_sAppearanceApplyShape( APPEARANCE &tAppearance,
									 APPEARANCE_DEFINITION &tAppearanceDef );

void c_AppearanceUpdateAnimsWithVelocity( 
	APPEARANCE* pAppearance );

void c_sAppearanceRandomizeStartOffsetAnimations( 
	APPEARANCE *pAppearance,
	float fTimePercent );


///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef HAVOK_ENABLED		
static void sVector3SafeNormalize( 
	hkVector4 & vVector,
	float fError = 0.05f )
{
	if ( vVector.lengthSquared3() < fError )
		vVector.set( 0.0f, 1.0f, 0.0f );
	else
		vVector.normalize3();
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static float sAnimationGetDuration( ANIMATION_DEFINITION &tDef )
{
	if( AppIsHellgate() )
	{
#ifdef HAVOK_ENABLED
		if ( ! tDef.pBinding )
			return ( 0.0f );
		ASSERT_RETZERO( tDef.pBinding->m_animation );
		return tDef.pBinding->m_animation->m_duration;
#endif
	}
	if( AppIsTugboat() )
	{
		if ( tDef.pGrannyAnimation )
		{
			ASSERT_RETZERO( tDef.pGrannyAnimation );
			return ( tDef.pGrannyAnimation->Duration );
		}
	}
	return ( 0.0f );
} // sAnimationGetDuration()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static float sAnimationGetDuration( ANIMATION &tAnimation )
{
	ASSERT_RETZERO( tAnimation.pDefinition );
	return ( sAnimationGetDuration( *tAnimation.pDefinition ) );
} //  sAnimationGetDuration()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static void c_sControlSetSpeed( granny_control *pControl,
								float fSpeed,
								ANIMATION *pAnimation,
								APPEARANCE_EVENTS * pEvents )
{
	ASSERT_RETURN( fSpeed < 100.0f );

	if ( pControl )
	{
		float fOldSpeed = GrannyGetControlSpeed( pControl );
		GrannySetControlSpeed( pControl, fSpeed );

		// if the timing of animation events should change
		if ( fSpeed != fOldSpeed  && pEvents != NULL)
		{
			ASSERT_RETURN( pAnimation );
			// cancel events if animation is frozen, otherwise reschedule
			// for new speed
			if ( fSpeed == 0.0f )
			{
				c_AnimationCancelEvents( pAnimation->pDefinition, pEvents );
			}
			else
			{
				float fDurationAtSpeed1 = sAnimationGetDuration( *pAnimation );
				c_AnimationRescheduleEvents( 
					pAnimation->pDefinition, 
					pEvents, 
					fDurationAtSpeed1 / fSpeed );
			}
		}

	}
} // c_sControlSetSpeed()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static float inline c_sControlGetSpeed( granny_control *pControl )
{
	return ( pControl == NULL ) ? 0.0f : GrannyGetControlSpeed( pControl );
} // c_sControlGetSpeed()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef HAVOK_ENABLED
static void c_sControlSetSpeed(
	hkDefaultAnimationControl * pControl,
	float fSpeed,
	ANIMATION *pAnimation,
	APPEARANCE_EVENTS * pEvents )
{
	ASSERT_RETURN( fSpeed <= MAX_CONTROL_SPEED );
	if ( pControl )
	{
		float fOldSpeed = pControl->getPlaybackSpeed();
		pControl->setPlaybackSpeed( fSpeed );

		// if the timing of animation events should change
		if ( fSpeed != fOldSpeed  && pEvents != NULL)
		{
			ASSERT_RETURN( pAnimation );
			// cancel events if animation is frozen, otherwise reschedule
			// for new speed
			if ( fSpeed == 0.0f )
			{
				c_AnimationCancelEvents( pAnimation->pDefinition, pEvents );
			}
			else
			{
				float fDurationAtSpeed1 = sAnimationGetDuration( *pAnimation );
				c_AnimationRescheduleEvents( 
					pAnimation->pDefinition, 
					pEvents, 
					fDurationAtSpeed1 / fSpeed );
			}
		}
	}
} // c_sControlSetSpeed()

static float inline c_sControlGetSpeed( hkDefaultAnimationControl * pControl )
{
	return ( pControl == HK_NULL ) ? 0.0f :  pControl->getPlaybackSpeed();
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static inline APPEARANCE_DEFINITION *sAppearanceGetDefinition( APPEARANCE &tAppearance )
{
	return ( AppearanceDefinitionGet( tAppearance.nDefinitionId ) );
} // sAppearanceGetDefinition()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static void sAnimTrace( const char *pszEvent,
						APPEARANCE &tAppearance,
						ANIMATION *pAnimation )
{
	if ( !sgbAnimTrace )
	{
		return;
	}

	char szOutput[256];

	APPEARANCE_DEFINITION *pAppearanceDef	= sAppearanceGetDefinition( tAppearance );
	if ( !pAppearanceDef )
	{
		return;
	}

	if ( !pAppearanceDef->pszModel )
	{
		PStrCopy( szOutput, "Appearance missing model\n", 256 );
	}

	int dwTime								= ( int )AppCommonGetCurTime();
	float fTimeInSeconds					= ( float )dwTime / 1000.0f;
	if ( pAnimation )
	{
		const char * pszWeightGroupName = c_AppearanceDefinitionGetWeightGroupName( pAppearanceDef, pAnimation->pDefinition->nBoneWeights );

		PStrPrintf( 
			szOutput, 256, 
			"%f %s ID:%d File:%s Model:%d(%s) WGrp:%s Pri:%d Group:%s \n",	
			fTimeInSeconds, 
			pszEvent, 
			pAnimation->nId,
			pAnimation->pDefinition->pszFile,
			tAppearance.nId, 
			pAppearanceDef->pszModel, 
			pszWeightGroupName ? pszWeightGroupName : "None",
			pAnimation->nPriority,
			AnimationGroupGetData( pAnimation->pDefinition->nGroup )->szName );
	}
	else
	{
		PStrPrintf( 
			szOutput, 256, 
			"%f %s Model:%d(%s)\n",	
			fTimeInSeconds, 
			pszEvent, 
			tAppearance.nId,
			pAppearanceDef->pszModel );
	}

	trace( szOutput );
} // sAnimTrace()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef HAVOK_ENABLED
static void sSetControlLocalTime( 
	 hkDefaultAnimationControl* control,
	 float fLocalTime,
	 float fDuration )
{
	fLocalTime = PIN( fLocalTime, 0.0f, fDuration );
	control->setLocalTime( fLocalTime );
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef HAVOK_ENABLED
class MyControlListener : public hkAnimationControlListener, public hkDefaultAnimationControlListener
{
public:

	MyControlListener() 
	{
	}


	virtual void loopOverflowCallback(hkDefaultAnimationControl* control, hkReal deltaTime, hkUint32 overflows) 
	{
		ANIMATION * pAnimation = HashGet( m_pAppearance->pAnimations, m_nAnimationId );

		BOOL bLoops = ( pAnimation->dwPlayFlags & PLAY_ANIMATION_FLAG_LOOPS ) != 0;
		if ( pAnimation->pDefinition &&
			( pAnimation->pDefinition->dwFlags & ANIMATION_DEFINITION_FLAG_HOLD_LAST_FRAME ) != 0 &&
			!AppIsHammer() )
		{
			bLoops = FALSE;
		}
		if ( !bLoops )
		{// kinda freeze the animation so that we still have one.  Hopefully, another animation will come along
			float fDuration = sAnimationGetDuration( *pAnimation );
			float fLocalTime = fDuration - deltaTime;
			sSetControlLocalTime( control, fLocalTime, fDuration );
			c_sControlSetSpeed( control, 0.0f, pAnimation, &m_pAppearance->tEventIds );
			if ( ! pAnimation->pDefinition ||
				( pAnimation->pDefinition->dwFlags & ANIMATION_DEFINITION_FLAG_HOLD_LAST_FRAME ) == 0 )
			{
				sAnimTrace( "ANIM_EASE_OUT LOOP OVERFLOW", *m_pAppearance, pAnimation );
				control->easeOut( pAnimation->pDefinition->fEaseOut );
				pAnimation->dwFlags |= ANIMATION_FLAG_EASING_OUT;
				m_pAppearance->dwFlags |= APPEARANCE_FLAG_UPDATE_WEIGHTS;
			}
		} else {
			if ( ! pAnimation->pDefinition || 
				! pAnimation->pDefinition->nEventCount || ! bLoops )
				return;

			float fSpeed = c_sControlGetSpeed( control );
			if ( fSpeed != 0.0f )
			{
				float fDurationAtSpeed1 = sAnimationGetDuration( *pAnimation );
				c_AnimationScheduleEvents( 
					AppGetCltGame(), 
					m_pAppearance->nId, 
					fDurationAtSpeed1 / fSpeed,
					*pAnimation->pDefinition, 
					TRUE, 
					FALSE );
			}
		}
	}

	virtual void loopUnderflowCallback(hkDefaultAnimationControl* control, hkReal deltaTime, hkUint32 underflows)
	{
	}

	virtual void easedInCallback(hkDefaultAnimationControl* control, hkReal deltaTime)
	{
		ANIMATION * pAnimation = HashGet( m_pAppearance->pAnimations, m_nAnimationId );
		if ( ! pAnimation )
			return;
		pAnimation->dwFlags &= ~ANIMATION_FLAG_EASING_IN;
		m_pAppearance->dwFlags |= APPEARANCE_FLAG_UPDATE_WEIGHTS;
	}

	virtual void easedOutCallback(hkDefaultAnimationControl* control, hkReal deltaTime)
	{
		ANIMATION * pAnimation = HashGet( m_pAppearance->pAnimations, m_nAnimationId );
		if ( ! pAnimation )
			return;
		sAnimTrace( "EASE_OUT_CALLBACK", *m_pAppearance, pAnimation );			
		if ( (pAnimation->dwPlayFlags & PLAY_ANIMATION_FLAG_DEFAULT_ANIM ) == 0 )
		{
			pAnimation->dwFlags |= ANIMATION_FLAG_REMOVE;
			m_pAppearance->dwFlags |= APPEARANCE_FLAG_ANIMS_MARKED_FOR_REMOVE;
			//sAnimationRemove( m_pAppearance, m_nAnimationId );
		}
	}

	virtual void controlDeletedCallback(hkAnimationControl* control)
	{
	}

	APPEARANCE * m_pAppearance;
	int			 m_nAnimationId;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
class CMyAnimControl : public hkDefaultAnimationControl
{
public:
	CMyAnimControl( const hkAnimationBinding* binding )
		: hkDefaultAnimationControl( binding )
	{
	}

	void ForceeaseIn( float fDuration )
	{
		m_easeT = 0.0f;
		easeIn( fDuration );
	}

	BOOL ForceEaseBackIn()
	{
		if ( ! m_easeDir )
			m_easeT = 1.0f - m_easeT;
		m_easeDir = TRUE;
		return m_easeT != 1.0f;
	}

	BOOL IsEasedOut()
	{
		return m_easeT == 1.0f && !m_easeDir;
	}
};
#endif


///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static int c_sAppearanceDefinitionGetBoneNumber( APPEARANCE_DEFINITION &tAppearanceDef,
												 const char *pszName );

/////////////////////////////////////////////////////////////////////////////
// Appearance - General
/////////////////////////////////////////////////////////////////////////////
void c_AppearanceToggleAnimTrace( void )
{
	sgbAnimTrace = !sgbAnimTrace;
} // c_AppearanceToggleAnimTrace()

static DWORD sGetAppearanceNeedsBonesFlag(
	int nLod )
{
	ASSERT (nLod < 2);
	return nLod ? APPEARANCE_FLAG_NEEDS_BONES_LOD_1 : APPEARANCE_FLAG_NEEDS_BONES_LOD_0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
int c_AppearanceGetWardrobe( int nAppearanceId )
{
	APPEARANCE *pAppearance = HashGet( sgpAppearances, nAppearanceId );
	if ( !pAppearance )
	{
		return INVALID_ID;
	}

	return ( pAppearance->nWardrobe );
} // c_AppearanceGetWardrobe()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
float c_AppearanceGetHeightFactor( int idAppearance )
{
	APPEARANCE *pAppearance = HashGet( sgpAppearances, idAppearance );
	if ( pAppearance )
	{
		if ( pAppearance->pfLengthenedBones )
		{
			return ( pAppearance->pfLengthenedBones[0] + 1.0f ); // +1 cause it's a delta
		}
	}

	return ( 1.0f );
} // c_AppearanceGetHeightFactor()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
int c_AppearanceGetAimBone( int nAppearanceId,
							VECTOR &vOffset )
{
	APPEARANCE *pAppearance					= HashGet( sgpAppearances, nAppearanceId );
	if ( !pAppearance )
	{
		return ( INVALID_ID );
	}

	APPEARANCE_DEFINITION *pAppearanceDef	= sAppearanceGetDefinition( *pAppearance );
	if ( !pAppearanceDef )
	{
		return ( INVALID_ID );
	}

	vOffset = pAppearanceDef->vAimOffset;
	return ( pAppearanceDef->nAimBone );
} // c_AppearanceGetAimBone()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
void c_AppearanceGetFootBones(
	int nAppearanceId,
	int & nLeftFootBone,
	int & nRightFootBone)
{
	nLeftFootBone = nRightFootBone = INVALID_ID;

	APPEARANCE* pAppearance = HashGet(sgpAppearances, nAppearanceId);
	if (!pAppearance)
	{
		return;
	}

	APPEARANCE_DEFINITION * pAppearanceDef = sAppearanceGetDefinition( *pAppearance );
	if ( ! pAppearanceDef )
	{
		return;
	}
	
	nLeftFootBone = pAppearanceDef->nLeftFootBone;
	nRightFootBone = pAppearanceDef->nRightFootBone;
} // c_AppearanceGetFootBones()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
void c_AppearanceTrackBone( int nAppearanceId,
							int nBone )
{
#ifdef HAVOK_ENABLED
	APPEARANCE *pAppearance		= HashGet( sgpAppearances, nAppearanceId );
	if ( !pAppearance )
	{
		return;
	}

	if ( !pAppearance->pBonesToTrack.IsInitialized() )
	{
		ArrayInit(pAppearance->pBonesToTrack, NULL, 2);
	}

	int nCount = pAppearance->pBonesToTrack.Count();
	for ( int i	= 0; i < nCount; i++ )
	{
		if ( pAppearance->pBonesToTrack[i].nBone == nBone )
		{
			return;
		}
	}

	BONE_TO_TRACK tBoneToTrack;
	tBoneToTrack.nBone = nBone;
	tBoneToTrack.mBoneInModel.setIdentity();
	ArrayAddItem(pAppearance->pBonesToTrack, tBoneToTrack);
#else
	REF( nAppearanceId );
	REF( nBone );
#endif
} // c_AppearanceTrackBone()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
granny_skeleton *sAppearanceDefGetSkeletonGranny( APPEARANCE_DEFINITION &tDef )
{
	if ( tDef.pGrannyModel )
	{
		return ( tDef.pGrannyModel->Skeleton );
	}

	return ( NULL );
} // sAppearanceDefGetSkeletonGranny()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef HAVOK_ENABLED
hkSkeleton * sAppearanceDefGetSkeletonMayhem( APPEARANCE_DEFINITION & tDef )
{
	if ( tDef.pMeshBinding )
		return tDef.pMeshBinding->m_skeleton;
	return NULL;
} // sAppearanceDefGetSkeletonMayhem()
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef HAVOK_ENABLED
hkSkeleton * c_AppearanceDefinitionGetSkeletonHavok( int nAppearanceDefId )
{
	APPEARANCE_DEFINITION * pAppearanceDef = AppearanceDefinitionGet( nAppearanceDefId ); 
	if ( ! pAppearanceDef )
		return NULL;

	return sAppearanceDefGetSkeletonMayhem( *pAppearanceDef );
} // c_AppearanceDefinitionGetSkeletonHavok()
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef HAVOK_ENABLED
void * c_AppearanceDefinitionGetSkeletonHavokVoid( int nAppearanceDefId )
{
	APPEARANCE_DEFINITION * pAppearanceDef = AppearanceDefinitionGet( nAppearanceDefId ); 
	if ( ! pAppearanceDef )
		return NULL;

	return sAppearanceDefGetSkeletonMayhem( *pAppearanceDef );
} // c_AppearanceDefinitionGetSkeletonHavokVoid()
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
granny_skeleton *c_AppearanceDefinitionGetSkeletonGranny( int nAppearanceDefId )
{
	APPEARANCE_DEFINITION *pAppearanceDef = AppearanceDefinitionGet( nAppearanceDefId );
	if ( !pAppearanceDef )
	{
		return ( NULL );
	}

	return ( sAppearanceDefGetSkeletonGranny( *pAppearanceDef ) );
} // c_AppearanceDefinitionGetSkeletonGranny()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
int c_AppearanceGetDefinition( int nAppearanceId )
{
	APPEARANCE *pAppearance = HashGet( sgpAppearances, nAppearanceId );
	if ( !pAppearance )
	{
		return ( INVALID_ID );
	}

	return ( pAppearance->nDefinitionId );
} // c_AppearanceGetDefinition()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
void c_AppearanceCreateSkyboxModel( SKYBOX_DEFINITION * pSkyboxDef, 
									SKYBOX_MODEL * pSkyboxModel, 
									const char * pszFolder )
{
	ASSERT_RETURN( pSkyboxModel );
	ASSERT_RETURN( pSkyboxModel->nModelID == INVALID_ID );

	GAME * pGame = AppGetCltGame();

	int nAppDef = DefinitionGetIdByName( DEFINITION_GROUP_APPEARANCE, pSkyboxModel->szModelFile );
	ASSERT_RETURN( nAppDef != INVALID_ID );

	APPEARANCE_DEFINITION * pAppearanceDef = (APPEARANCE_DEFINITION*)DefinitionGetById( DEFINITION_GROUP_APPEARANCE, nAppDef );
	if ( ! pAppearanceDef )
	{
		return;
	}

	// how does this handle async loading?
	int nModelID = c_AppearanceNew( nAppDef, 0 );
	ASSERT_RETURN( nModelID != INVALID_ID );

	c_ModelSetDrawLayer( nModelID, DRAW_LAYER_ENV );
	c_ModelSetRegion( nModelID, pSkyboxDef->nRegionID );

	RAND tRand;
	RandInit( tRand );

	pSkyboxModel->vOffset = VECTOR(
		pSkyboxModel->fScatterRad ? RandGetFloat( tRand, -pSkyboxModel->fScatterRad, pSkyboxModel->fScatterRad ) : pSkyboxModel->fScatterRad,
		pSkyboxModel->fScatterRad ? RandGetFloat( tRand, -pSkyboxModel->fScatterRad, pSkyboxModel->fScatterRad ) : pSkyboxModel->fScatterRad,
		pSkyboxModel->fAltitude );
	MATRIX matWorld;
	MatrixTranslation( &matWorld, &pSkyboxModel->vOffset );
	UNITMODE nAnimMode = MODE_WALK;
	BOOL bHasMode;
	/*float fAnimDuration = */
	AppearanceDefinitionGetAnimationDuration( pGame, nAppDef, 0, nAnimMode, bHasMode );
	ASSERTV( bHasMode, "Skybox appearance model missing walk mode!\n\n%s%s", pszFolder, pSkyboxModel->szModelFile );
	if ( bHasMode )
	{
		c_AnimationPlayByMode( NULL, pGame, nModelID, nAnimMode, 0, 0.0f, PLAY_ANIMATION_FLAG_LOOPS | PLAY_ANIMATION_FLAG_RAND_START_TIME );
	}

	c_ModelSetLocation( nModelID, &matWorld, pSkyboxModel->vOffset, INVALID_ID, MODEL_DYNAMIC );

	pSkyboxModel->nModelID = nModelID;
} // c_AppearanceCreateSkyboxModel()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
void c_AppearanceHideWeapons( int nAppearanceId,
							  BOOL bHide )
{
	APPEARANCE *pAppearance = HashGet( sgpAppearances, nAppearanceId );
	if ( !pAppearance )
	{
		return;
	}


	if ( bHide )
		pAppearance->dwFlags |= APPEARANCE_FLAG_HIDE_WEAPONS;
	else
		pAppearance->dwFlags &= ~APPEARANCE_FLAG_HIDE_WEAPONS;

	
} // c_AppearanceHideWeapons()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL c_AppearanceAreWeaponsHidden( int nAppearanceId )
{
	APPEARANCE *pAppearance = HashGet( sgpAppearances, nAppearanceId );
	if ( !pAppearance )
	{
		return ( FALSE );
	}

	return ( pAppearance->dwFlags & APPEARANCE_FLAG_HIDE_WEAPONS ) != 0;
} // c_AppearanceAreWeaponsHidden()



///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
void c_AppearanceFreeze( int nAppearanceId,
						 BOOL bFreeze )
{
	APPEARANCE *pAppearance = HashGet( sgpAppearances, nAppearanceId );
	if ( !pAppearance )
	{
		return;
	}

	if( AppIsHellgate() )
	{
		if ( bFreeze )
			pAppearance->dwFlags |= APPEARANCE_FLAG_FREEZE_ANIMATION;
		else
			pAppearance->dwFlags &= ~APPEARANCE_FLAG_FREEZE_ANIMATION;
	}
	if( AppIsTugboat() )
	{
		if ( bFreeze )
		{
			pAppearance->dwFlags |= APPEARANCE_FLAG_FREEZE_ANIMATION;
			pAppearance->dwFlags |= APPEARANCE_FLAG_UPDATE_WEIGHTS;
		}
		else
		{
			pAppearance->dwFlags &= ~APPEARANCE_FLAG_FREEZE_ANIMATION;
			pAppearance->dwFlags &= ~APPEARANCE_FLAG_LAST_FRAME;
			pAppearance->dwFlags |= APPEARANCE_FLAG_UPDATE_WEIGHTS;
	
		}
	}
} // c_AppearanceFreeze()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL c_AppearanceIsFrozen( int nAppearanceId )
{
	APPEARANCE *pAppearance = HashGet( sgpAppearances, nAppearanceId );
	if ( !pAppearance )
	{
		return ( FALSE );
	}

	return ( pAppearance->dwFlags & APPEARANCE_FLAG_FREEZE_ANIMATION ) != 0;
} // c_AppearanceIsFrozen()


///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
void c_AppearanceHide( int nAppearanceId,
						 BOOL bHide )
{
	APPEARANCE *pAppearance = HashGet( sgpAppearances, nAppearanceId );
	if ( !pAppearance )
	{
		return;
	}


	if ( bHide )
		pAppearance->dwFlags |= APPEARANCE_FLAG_HIDDEN;
	else
		pAppearance->dwFlags &= ~APPEARANCE_FLAG_HIDDEN;
} // c_AppearanceHide()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL c_AppearanceIsHidden( int nAppearanceId )
{
	APPEARANCE *pAppearance = HashGet( sgpAppearances, nAppearanceId );
	if ( !pAppearance )
	{
		return ( FALSE );
	}

	return ( pAppearance->dwFlags & APPEARANCE_FLAG_HIDDEN ) != 0;
} // c_AppearanceIsHidden()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL c_AppearanceDestroyAllAnimations( int nAppearanceId )
{
	APPEARANCE *pAppearance = HashGet( sgpAppearances, nAppearanceId );
	ASSERT_RETFALSE( pAppearance );

	for ( ANIMATION * pAnimation = HashGetFirst( pAppearance->pAnimations ); pAnimation != NULL;
		  pAnimation = HashGetNext( pAppearance->pAnimations, pAnimation ) )
	{
		if ( pAnimation->pGrannyControl )
		{
			GrannyFreeControl( pAnimation->pGrannyControl );
			pAnimation->pGrannyControl = NULL;
		}
#ifdef HAVOK_ENABLED
		if ( pAnimation->pControl )
		{
			pAppearance->pAnimatedSkeleton->removeAnimationControl( pAnimation->pControl );
			pAnimation->pControl->removeReference();
			pAnimation->pControl = NULL;
		}
		if ( pAnimation->pListener )
		{
			delete pAnimation->pListener;
		}
#endif
	}

	HashClear( pAppearance->pAnimations );
	if ( pAppearance->pGrannyInstance )
	{
		GrannyFreeCompletedModelControls( pAppearance->pGrannyInstance );
	}

	sAnimTrace( "Destroy All", *pAppearance, NULL );
	return ( TRUE );
} // c_AppearanceDestroyAllAnimations()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static
void sCachePoseToMeshMatrices(APPEARANCE * pAppearance, int nModelDefId, int nLOD, const MATRIX * pPose, int nBones, BOOL bInitializing = FALSE )
{
	int nMeshCount = e_AnimatedModelDefinitionGetMeshCount( nModelDefId, nLOD );
	ASSERT_RETURN( nMeshCount < MAX_MESHES_PER_MODEL );

	if (nMeshCount > pAppearance->nMeshCount_[nLOD] && 
		pAppearance->pMeshes_[nLOD] != 0)
	{
		FREE(NULL, pAppearance->pMeshes_[nLOD]);
		pAppearance->pMeshes_[nLOD] = 0;
		
	}
	if (pAppearance->pMeshes_[nLOD] == 0 && ( nMeshCount > 0 ) )
	{
		pAppearance->pMeshes_[nLOD] = (APPEARANCE_MESH*)MALLOC(NULL, sizeof(APPEARANCE_MESH) * nMeshCount);
		pAppearance->nMeshCount_[nLOD] = nMeshCount;
	}

	if (nMeshCount > 0)
	{
		DWORD dwFlag = sGetAppearanceNeedsBonesFlag( nLOD );
		pAppearance->dwFlags &= ~dwFlag;
	}

	for ( int nMesh = 0; nMesh < nMeshCount; nMesh++ )
	{
		APPEARANCE_MESH *pAnimatingMesh		= &pAppearance->pMeshes_[nLOD][nMesh];
		const int * pnBoneMapping			= e_AnimatedModelDefinitionGetBoneMapping(nModelDefId, nLOD, nMesh);
		int nBonesPerShader					= e_AnimatedModelGetMaxBonesPerShader();
		ASSERT_RETURN( nBonesPerShader <= MAX_BONES_PER_MESH_RENDER_CHUNK )
		float * pflSmallPointer				= pAnimatingMesh->pfBoneMatrices3x4;

#ifdef MOTION_BLUR
		// Copy bones from the last update.
		if ( e_IsDX10Enabled() && ! bInitializing )
		{
			MemoryCopy( pAnimatingMesh->pfPrevBoneMatrices3x4, sizeof( pAnimatingMesh->pfPrevBoneMatrices3x4 ),
				pAnimatingMesh->pfBoneMatrices3x4, sizeof( pAnimatingMesh->pfPrevBoneMatrices3x4 ) );
		}
#endif

		for ( int i = 0; i < nBonesPerShader; i++ )
		{
			// this is where we remap and copy the bones for this mesh
			ASSERT_RETURN( pnBoneMapping[i] < nBones );

			const float *pflBigPointer = ( const float * )( pPose[pnBoneMapping[i]] );
			memcpy( pflSmallPointer, pflBigPointer, sizeof( float ) * 12 );
			pflSmallPointer += 12;
		}

#ifdef MOTION_BLUR
		// Copy "last frame's" bones with the same as the current if this
		// is this is the first time the bones are being initialized.
		if ( e_IsDX10Enabled() && bInitializing )
		{
			MemoryCopy( pAnimatingMesh->pfPrevBoneMatrices3x4, sizeof( pAnimatingMesh->pfPrevBoneMatrices3x4 ),
				pAnimatingMesh->pfBoneMatrices3x4, sizeof( pAnimatingMesh->pfPrevBoneMatrices3x4 ) );
		}
#endif
	}
}

static
void sTransferPoseToMeshMatrices( APPEARANCE *pAppearance, BOOL bInitializing = FALSE )
{
#if ! ISVERSION(SERVER_VERSION)
	MATRIX pBoneMatrices4x4[MAX_BONES_PER_MODEL];

	APPEARANCE_DEFINITION *pAppearanceDef	= sAppearanceGetDefinition( *pAppearance );

	int nBoneCount							= pAppearanceDef->nBoneCount;

	// Sample the active animations and combine into a single pose 
	// convert the pose into an array of matrices for the shader

	for ( int nMatrix						= 0; nMatrix < nBoneCount; nMatrix++ )
	{
#ifdef HAVOK_ENABLED
		if ( pAppearance->pAnimatedSkeleton )
		{
			pAppearance->pPoseInModel[ nMatrix ].get4x4ColumnMajor( (float *) &pBoneMatrices4x4[ nMatrix ] );

			if ( pAppearanceDef->pMeshBinding )
			{
				MATRIX mInverseWorldBind;
				pAppearanceDef->pMeshBinding->m_inverseWorldBindPose[ nMatrix ].get4x4ColumnMajor( (float *) &mInverseWorldBind );
				MatrixMultiply ( & pBoneMatrices4x4[ nMatrix ], & mInverseWorldBind, & pBoneMatrices4x4[ nMatrix ] );
			}
			// transpose all of the matrices since we need them to be row-major 
			MatrixTranspose( & pBoneMatrices4x4[ nMatrix ], & pBoneMatrices4x4[ nMatrix ] );
		} 
		else 
#endif
		{
			/* these bones don't move... so set it all to identity */
			MatrixIdentity( &pBoneMatrices4x4[nMatrix] );
		}
	}


	// this could restore the model definition (and hopefully the LOD one as well)
	int nAppModelDef = c_AppearanceGetModelDefinition( pAppearance );

	// Cut off the last row since it is just 0,0,0,1.0 anyway. It will just eat up
	// registers on the card ;
	// Also, remap the bones to match the bones used in the mesh
	//Tugboat doesn't have LOD models.

	// CHB 2006.11.30 - Unfortunately, the mesh matrices already act
	// as a cache for drawing.  The actual pose underneath can
	// change without updating the cache, intentionally.  Thus we
	// can't depend on the pose as a cache, so we must build the
	// mesh matrices for all LODs just in case they might be used.
	// Examples of the problems observed prior to this:
	//
	//	-	After a monster was killed, its death poses were not
	//		identical for high- and low-LOD views.
	//
	//	-	After a monster was killed, crossing the LOD distance
	//		would cause the dead monster to snap into the binding
	//		pose.
	for ( int nLOD = 0; nLOD < LOD_COUNT; nLOD++ )
		sCachePoseToMeshMatrices(pAppearance, nAppModelDef, nLOD, pBoneMatrices4x4, nBoneCount, bInitializing );	
#endif //  ! ISVERSION(SERVER_VERSION)
} // sTransferPoseToMeshMatrices()


/////////////////////////////////////////////////////////////////////////////
//	I'm pretty sure that this doesn't work properly. Some bad math going on.
//	When I get some better art for previewing the effect that we are looking for, I'll return to this. - Tyler
//
//	Travis - is this still bad juju?-
/////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL c_AppearanceInitAttachmentOnMesh( GAME *pGame,
									   int nAppearanceId,
									   ATTACHMENT_DEFINITION &tAttachDef,
									   const VECTOR &vSource,
									   const VECTOR &vDirection )
{
#if ! ISVERSION(SERVER_VERSION)

	// find the distance between the bones and the ray. 
	// Record the top 5 skinned bones - ranked by closest to source and closest to
	// line

	int nAppearanceDefId					= c_AppearanceGetDefinition( nAppearanceId );
	if ( nAppearanceDefId == INVALID_ID )
		return FALSE;

	int nBoneCount							= c_AppearanceDefinitionGetBoneCount( nAppearanceDefId );

	APPEARANCE *pAppearance					= sgpAppearances.GetItem( nAppearanceId );
	ASSERT_RETFALSE( pAppearance );

	const MATRIX *pmWorldInverse			= e_ModelGetWorldScaledInverse( nAppearanceId );
	ASSERT_RETFALSE( pmWorldInverse );

	VECTOR vSourceInModel;
	MatrixMultiply( &vSourceInModel, &vSource, pmWorldInverse );

	APPEARANCE_DEFINITION *pAppearanceDef	= sAppearanceGetDefinition( *pAppearance );
	if ( !pAppearanceDef )
	{
		return ( FALSE );
	}
	MATRIX mBoneMatrix;
	int nBestBone = INVALID_ID;

#ifdef HAVOK_ENABLED
	if( AppIsHellgate() )
	{
		if ( !pAppearanceDef->pMeshBinding )
		{
			return ( FALSE );
		}
	
		int nModelDef = e_ModelGetDefinition( nAppearanceId );
		DWORD * pdwBoneIsSkinned = e_AnimatedModelDefinitionGetBoneIsSkinnedFlags( nModelDef );
	
		nBestBone = INVALID_ID;
		float fBestDistSquared = 0.0f;
		for ( int i = 0; i < nBoneCount; i++ )
		{
			if ( pdwBoneIsSkinned && ! TESTBIT( pdwBoneIsSkinned, i ) )
				continue;
	
			// we need a function kinda like this... maybe keep track of bools as we split the mesh up
			// if ( ! BoneIsSkinned( nAppearanceDefId, i ) )
			//	continue;
			hkVector4 vHavokBonePos = pAppearance->pPoseInModel[ i ].getTranslation();
			VECTOR vBonePos( vHavokBonePos( 0 ), vHavokBonePos( 1 ), vHavokBonePos( 2 ) );
	
			VECTOR vDeltaToBone = vBonePos - vSourceInModel;
			vDeltaToBone.fZ *= 4.0f; // I'm more concerned about being close in the Z than X and Y
			float fDistSquared = VectorLengthSquared( vDeltaToBone );
	
			if ( nBestBone == INVALID_ID || fDistSquared < fBestDistSquared )
			{
				fBestDistSquared = fDistSquared;
				nBestBone = i;
			} 
		}
	
		if ( nBestBone == INVALID_ID )
		{
			return ( FALSE );
		}
	
	
		hkVector4 vHavokBonePos = pAppearance->pPoseInModel[ nBestBone ].getTranslation();
		VECTOR vBonePos( vHavokBonePos( 0 ), vHavokBonePos( 1 ), vHavokBonePos( 2 ) );
	

		pAppearance->pPoseInModel[ nBestBone ].get4x4ColumnMajor( (float *) &mBoneMatrix );
	}
#endif

	if( AppIsTugboat() )
	{
	
		int nModelDef			= e_ModelGetDefinition( nAppearanceId );
		DWORD *pdwBoneIsSkinned	= e_AnimatedModelDefinitionGetBoneIsSkinnedFlags( nModelDef);
	
		
	
		for ( int i	= 0; i < nBoneCount; i++ )
		{
			if ( pdwBoneIsSkinned && !TESTBIT( pdwBoneIsSkinned, i ) )
			{
				continue;
	
			}
		}
	
		if ( nBestBone == INVALID_ID )
		{
			return ( FALSE );
		}
	
		mBoneMatrix						= pAppearance->pGrannyPoseInModel[nBestBone];
	}

	const MATRIX *pmScaledWorld				= e_ModelGetWorldScaled( nAppearanceId );
	ASSERT_RETFALSE( pmScaledWorld );

	MATRIX mBoneWorld;
	MatrixMultiply( &mBoneWorld, &mBoneMatrix, pmScaledWorld );

	MATRIX mBoneWorldInverse;
	MatrixInverse( &mBoneWorldInverse, &mBoneWorld );
	tAttachDef.vPosition = VECTOR( 0.0f );
	MatrixMultiplyNormal( &tAttachDef.vNormal, &vDirection, &mBoneWorldInverse );

	VectorNormalize( tAttachDef.vNormal );
	tAttachDef.nBoneId = nBestBone;

#endif //  ! ISVERSION(SERVER_VERSION)

	return ( TRUE );
} // c_AppearanceInitAttachmentOnMesh()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static void c_sAppearanceUpdateApproximateSize(
	APPEARANCE * pAppearance )
{

	if ( INVALID_ID == c_AppearanceGetModelDefinition( pAppearance ) )
	{
		return;
	}

	BOUNDING_BOX tBoundingBox;
	if ( S_OK != e_GetModelRenderAABBInObject( pAppearance->nId, &tBoundingBox ) )
	{
		return;
	}

	VECTOR vBoxSize = BoundingBoxGetSize( &tBoundingBox );
	pAppearance->fApproximateSize = MAX( vBoxSize.fX, MAX( vBoxSize.fY, vBoxSize.fZ ) );
} // c_sAppearanceUpdateApproximateSize()


///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static void sAppearanceInitializeBones( 
	APPEARANCE *pAppearance,
	int nBoneCount )
{
	BOOL bCanTransferBones = FALSE;
	int nModelDefId = c_AppearanceGetModelDefinition( pAppearance );
	for ( int nLOD = 0; nLOD < LOD_COUNT; nLOD++ )
	{
		DWORD dwBoneFlagForLOD = sGetAppearanceNeedsBonesFlag( nLOD );
		if ( pAppearance->dwFlags & dwBoneFlagForLOD )
		{
			int nMeshCount = e_AnimatedModelDefinitionGetMeshCount( nModelDefId, nLOD );
			if ( nMeshCount != 0 )
				bCanTransferBones = TRUE;
		}
	}

#ifdef HAVOK_ENABLED
	BOOL bInitializeBones = bCanTransferBones;
	// setting size on pposeinmodel with Tugboat will crash in debugopt, where Havok is still enabled.
	if ( AppIsHellgate() &&
		nBoneCount && ! pAppearance->pPoseInLocal.getSize() )
	{
		pAppearance->pPoseInModel.setSize( nBoneCount );
		pAppearance->pPoseInLocal.setSize( nBoneCount );
		bInitializeBones = TRUE;
	}

	if( AppIsHellgate() && bInitializeBones )
	{
#if ! ISVERSION(SERVER_VERSION)
		if ( ! nBoneCount )
		{
			return;
		}
	
		const MATRIX * pmMatrices = NULL;
		if ( ! pAppearance->pAnimatedSkeleton && nBoneCount )
		{
			pmMatrices = e_AnimatedModelDefinitionGetBoneRestPoses( c_AppearanceGetModelDefinition( pAppearance ) );
			if ( ! pmMatrices )
			{
				return;  // this happens due to async.  We will return to here
			}
		}

		if ( pmMatrices )
		{
			for ( int i = 0; i < nBoneCount; i++ )
			{
				pAppearance->pPoseInModel[ i ].set4x4ColumnMajor( (float *) &pmMatrices[ i ] );
			}
		} 
		else if ( pAppearance->pAnimatedSkeleton && nBoneCount ) 
		{
			const hkSkeleton * pSkeleton = pAppearance->pAnimatedSkeleton->getSkeleton();
			ASSERT_RETURN( pSkeleton );
			ASSERT_RETURN( nBoneCount == pSkeleton->m_numReferencePose );
			for ( int i = 0; i < nBoneCount; i++ )
			{
				pAppearance->pPoseInLocal[ i ] = pSkeleton->m_referencePose[ i ];
			}			
			hkSkeletonUtils::transformLocalPoseToModelPose( pSkeleton->m_numBones, 
				pSkeleton->m_parentIndices, 
				pAppearance->pPoseInLocal.begin(), 
				pAppearance->pPoseInModel.begin() );
		} else {
			for ( int i = 0; i < nBoneCount; i++ )
			{
				pAppearance->pPoseInModel[ i ].setIdentity();
			}
		}


#endif //  ! ISVERSION(SERVER_VERSION)
	}
#endif

	if( AppIsTugboat() && bCanTransferBones )
	{
		ArrayInit(pAppearance->pGrannyPoseInModel, NULL, 1);
		ArrayInit(pAppearance->pGrannyPoseInLocal, NULL, 1);
	
		MATRIX Identity;
		MatrixIdentity( &Identity );
	
		for ( int i = 0; i < nBoneCount; i++ )
		{
			ArrayAddItem(pAppearance->pGrannyPoseInModel, Identity);
		}
	
		if ( !pAppearance->pGrannyInstance && nBoneCount )
		{
#if ! ISVERSION(SERVER_VERSION)
			const MATRIX *pmMatrices	= e_AnimatedModelDefinitionGetBoneRestPoses( c_AppearanceGetModelDefinition( pAppearance ) );
			if( pmMatrices )
			{
				for ( int i = 0; i < nBoneCount; i++ )
				{
					pAppearance->pGrannyPoseInModel[i] = pmMatrices[i];
				}
			}

#endif //  ! ISVERSION(SERVER_VERSION)
		}
	}

	if ( bCanTransferBones )
	{
		sTransferPoseToMeshMatrices( pAppearance, TRUE );
		// TRAVIS: we need to reapply the current state of the skeleton - this could all happen AFTER valid
		// animation! And more importantly, the last frame of frozen animations. This makes sure
		// that newly initialized models get that last pose.
#if ! ISVERSION(SERVER_VERSION)
		if( AppIsTugboat() && nBoneCount && pAppearance->pWorldPose )
		{
			nBoneCount		= GrannyGetSourceSkeleton( pAppearance->pGrannyInstance )->BoneCount;

			granny_local_pose *ptLocalPose			= g_ptSharedLocalPose;
			MATRIX mIdentity;
			MatrixIdentity( &mIdentity );
			GrannySampleModelAnimationsAccelerated( pAppearance->pGrannyInstance, nBoneCount, mIdentity,
														ptLocalPose, pAppearance->pWorldPose );

			MATRIX* pBoneMatrices4x4Granny	= (MATRIX*)GrannyGetWorldPoseComposite4x4Array(
					pAppearance->pWorldPose );

			// transpose all of the matrices since we need them to be row-major 
			for ( int nMatrix = 0; nMatrix < nBoneCount; nMatrix++ )
			{
				MatrixTranspose( &pBoneMatrices4x4Granny[nMatrix], &pBoneMatrices4x4Granny[nMatrix] );
			}

			
			// Cut off the last row since it is just 0,0,0,1.0 anyway. It will just eat up
			// registers on the card  Also, remap the bones to match the bones used in the mesh
			sCachePoseToMeshMatrices(pAppearance, pAppearance->nModelDefinitionId, LOD_HIGH, pBoneMatrices4x4Granny, nBoneCount);
			
		}
#endif //  ! ISVERSION(SERVER_VERSION)

	}
} // sAppearanceInitializeBones()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static void c_sAppearanceNew( void *pDefinition,
							  EVENT_DATA *pEventData )
{
	APPEARANCE_DEFINITION *pAppearanceDef	= ( APPEARANCE_DEFINITION * )pDefinition;
	GAME *pGame								= AppGetCltGame();

	int nModelId							= (int)pEventData->m_Data1;
	DWORD dwFlags 							= (DWORD)pEventData->m_Data2;
	REF(dwFlags);
	UNITID idUnit							= (int)pEventData->m_Data3;
	UNIT *pUnit								= NULL;
	if ( idUnit != INVALID_ID && pGame != NULL )
	{
		pUnit = UnitGetById( pGame, idUnit );
	}

	int nAppearanceDefId					= pAppearanceDef->tHeader.nId;

	// this could reload a freed modeldef
	int nModelDefId							= sAppearanceDefinitionGetModelDefinition( pAppearanceDef );

	APPEARANCE *pAppearance					= HashGet( sgpAppearances, nModelId );


	// It can happen that the appearance is destroyed between when it says to load 
	// it and when it is actually finished loading.

	if ( !pAppearance )
	{
		return;
	}

	//e_ModelSetLODDistance( nModelId, pAppearanceDef->fLODDistance );	// CHB 2006.10.20
	e_ModelSetLODScreensize( nModelId, pAppearanceDef->fLODScreensize );

	MODEL_DISTANCE_CULL_TYPE eCullType = MODEL_DISTANCE_CULL_ANIMATED_ACTOR;
	MODEL_FAR_CULL_METHOD eCullMethod = MODEL_FAR_CULL_BY_DISTANCE;

	if ( pUnit )
	{
		const UNIT_DATA * pUnitData = UnitGetData( pUnit );
		if ( pUnitData && UnitDataTestFlag( pUnitData, UNIT_DATA_FLAG_CULL_BY_SCREENSIZE ) )
		{
			eCullType = MODEL_DISTANCE_CULL_ANIMATED_PROP;
			eCullMethod = MODEL_FAR_CULL_BY_SCREENSIZE;
		}
	}// if the appearance has no unit, that means it is automatically an animated prop
	else if ( AppIsTugboat() )
	{
		eCullType = MODEL_DISTANCE_CULL_ANIMATED_PROP;
		eCullMethod = MODEL_FAR_CULL_BY_SCREENSIZE;
	}

	BOOL bCanCull = TRUE;
	if ( TEST_MASK( pAppearanceDef->dwFlags, APPEARANCE_DEFINITION_FLAG_DONT_DISTANCE_CULL ) )
	{
		eCullType = MODEL_DISTANCE_DONT_CULL;
		bCanCull = FALSE;
	}
	
	V( e_ModelSetFlagbit( nModelId, MODEL_FLAGBIT_DISTANCE_CULLABLE, bCanCull ) );
	e_ModelSetDistanceCullType( nModelId, eCullType );

	e_ModelSetFarCullMethod( nModelId, eCullMethod );

	if( TEST_MASK( pAppearanceDef->dwFlags, APPEARANCE_DEFINITION_FLAG_NO_SHADOW ) )
	{
		e_ModelSetFlagbit( nModelId, MODEL_FLAGBIT_NOSHADOW, TRUE );
	}

	nModelDefId = c_AppearanceGetModelDefinition( pAppearance );
	pAppearance->nModelDefinitionId = nModelDefId;
	pAppearanceDef->nModelDefId = nModelDefId;

#ifdef HAVOK_ENABLED
	if( AppIsHellgate() )
	{
		hkSkeleton * pSkeleton = sAppearanceDefGetSkeletonMayhem( *pAppearanceDef );
	
		if ( pSkeleton )
		{
			pAppearance->pAnimatedSkeleton = new hkAnimatedSkeleton( pSkeleton );
			pAppearance->pAnimatedSkeleton->addReference();
			pAppearance->pAnimatedSkeleton->setReferencePoseWeightThreshold( 0.0001f );
			c_RagdollNew( pGame, nAppearanceDefId, nModelId );
	
			// Make a list shape to pose the character for collision (particles etc)
			pAppearance->pPoseShape = HK_NULL;
			pAppearance->pPosePhantom = HK_NULL;
		}
	}
#endif
	if( AppIsTugboat() )
	{
		if ( pAppearanceDef->pGrannyModel )
		{
			pAppearance->pGrannyInstance = GrannyInstantiateModel( pAppearanceDef->pGrannyModel );
	
	
			// This is a buffer that stores the world-space matrices for all the bones of the
			// model
	
			int nBoneCount = GrannyGetSourceSkeleton( pAppearance->pGrannyInstance )->BoneCount;
	
			pAppearance->pWorldPose = GrannyNewWorldPose( nBoneCount );
			// initialize this so we have decent starting info for initial attachments.
			granny_local_pose *ptLocalPose			= g_ptSharedLocalPose;
			MATRIX mIdentity;
			MatrixIdentity( &mIdentity );
			GrannySampleModelAnimationsAccelerated( pAppearance->pGrannyInstance, nBoneCount, mIdentity,
														ptLocalPose, pAppearance->pWorldPose );
		}
	}


	if( AppIsTugboat() )
	{
		if ( pAppearance->nModelDefinitionId != INVALID_ID )
			pAppearance->dwFlags |= sGetAppearanceNeedsBonesFlag( LOD_HIGH );
	}
	else
	{
		if ( pAppearance->nModelDefinitionId != INVALID_ID )
			pAppearance->dwFlags |= APPEARANCE_MASK_NEEDS_BONES;
	}

	if ( pAppearanceDef->nBoneCount != 0 )
	{
		sAppearanceInitializeBones( pAppearance, pAppearanceDef->nBoneCount );
	}

	c_sAppearanceApplyShape( *pAppearance, *pAppearanceDef );
	if ( pAppearance->fApproximateSize <= 0.0f )
	{
		c_sAppearanceUpdateApproximateSize( pAppearance );
	}

	c_AppearanceDefinitionApplyToModel( nAppearanceDefId, nModelId );

	pAppearance->vFacingDirection = VECTOR( 0, 1, 0 );  	 // this is straight ahead 



} // c_sAppearanceNew()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
void AppearanceShapeInit( APPEARANCE_SHAPE &tShape )
{
	tShape.bHeight = 0;
	tShape.bWeight = 0;
} // AppearanceShapeInit()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
int c_AppearanceNew( int nAppearanceDefId,
					 DWORD dwFlags,
					 UNIT *pUnit,
					 const UNIT_DATA *pUnitData,
					 int nWardrobe,
					 int nStartingStance,
					 APPEARANCE_SHAPE *pAppearanceShape,
					 int nWardrobeFallback )
{
#if ! ISVERSION(SERVER_VERSION)
	if ( nAppearanceDefId == INVALID_ID )
	{
		int nModelID;
		V_DO_FAILED( e_ModelNew( &nModelID, INVALID_ID ) )
			return INVALID_ID;
		return nModelID;
	}

	GAME *pGame								= AppGetCltGame();

	APPEARANCE_DEFINITION *pAppearanceDef	= AppearanceDefinitionGet( nAppearanceDefId );
	REF( pAppearanceDef );

	int nModelId;
	V( e_ModelNew( &nModelId, INVALID_ID ) );
	e_ModelSetAppearanceDefinition( nModelId, nAppearanceDefId );

	// create animating model 
	APPEARANCE *pAppearance					= HashAdd( sgpAppearances, nModelId );
#ifdef HAVOK_ENABLED
	if( AppIsHellgate() )
	{
		new ( &pAppearance->pPoseInLocal )hkArray<hkQsTransform>;
		new ( &pAppearance->pPoseInModel )hkArray<hkQsTransform>;
	}
#endif

	ASSERT( pAppearance );
	pAppearance->nDefinitionId = nAppearanceDefId;
	pAppearance->nWardrobe = INVALID_ID;
	pAppearance->bFullbright = FALSE;
	pAppearance->bDrawBehind = FALSE;
	pAppearance->AmbientLight = 1;
	pAppearance->bHit = FALSE;
	pAppearance->fBehindColor = 0;
	pAppearance->nAnimationGroup = INVALID_ID;
	HashInit( pAppearance->pAnimations, NULL, APPEARANCE_ANIMATION_HASH_SIZE );
	pAppearance->nNextAnimID = 0;
	pAppearance->nStanceCurrent = max( nStartingStance, 1 );
	pAppearance->idUnit = pUnit ? UnitGetId( pUnit ) : INVALID_ID;
	// CHB 2006.11.30
	for (int j = 0; j < LOD_COUNT; ++j)
	{
		pAppearance->pMeshes_[j] = NULL;
		pAppearance->nMeshCount_[j] = 0;
	}
	pAppearance->dwNewFlags = dwFlags;
	pAppearance->fAnimSampleTimeDelay = ( 1.0f / 60.0f );
	pAppearance->fApproximateSize = 0.0f;
	if ( pAppearanceShape )
	{
		memcpy( &pAppearance->tAppearanceShape, pAppearanceShape, sizeof( APPEARANCE_SHAPE ) );
	}
	else
	{
		pAppearance->tAppearanceShape.bHeight = 125;
		pAppearance->tAppearanceShape.bWeight = 125;
	}
	pAppearance->vFacingTarget = VECTOR( 0 );
	pAppearance->vFacingTargetLastGood = VECTOR( 0 );

	ArrayInit(pAppearance->tEventIds, NULL, 1);

	int nModelDefId							= INVALID_ID;
	if ( nWardrobe != INVALID_ID )
	{
		pAppearance->nWardrobe = nWardrobe;
		nModelDefId = c_WardrobeGetModelDefinition( nWardrobe );
	}
	else if ( TEST_MASK( dwFlags, APPEARANCE_NEW_FLAG_FORCE_WARDROBE ) )
	{
		int nAppearanceGroup		= pUnitData ? pUnitData->
			pnWardrobeAppearanceGroup[UNIT_WARDROBE_APPEARANCE_GROUP_THIRDPERSON] : INVALID_ID;
		if ( dwFlags & APPEARANCE_NEW_IGNORE_UNIT_WARDROBE_APPEARANCE_GROUP )
		{
			nAppearanceGroup = INVALID_ID;
		}

		if ( dwFlags & APPEARANCE_NEW_FIRSTPERSON_WARDROBE_APPEARANCE_GROUP )
		{
			nAppearanceGroup = pUnitData ?
				pUnitData->pnWardrobeAppearanceGroup[UNIT_WARDROBE_APPEARANCE_GROUP_FIRSTPERSON] : INVALID_ID;
		}

		WARDROBE_INIT tWardrobeInit( nAppearanceGroup, nAppearanceDefId, pUnit ? UnitGetType( pUnit ) : UNITTYPE_ANY );
		tWardrobeInit.bSharesModelDefinition = ( dwFlags & APPEARANCE_NEW_FLAG_WARDROBE_SHARES_MODELDEF ) != 0;
		tWardrobeInit.nMipBonus = ( dwFlags & APPEARANCE_NEW_MASK_WARDROBE_MIP_BONUS ) >> APPEARANCE_NEW_SHIFT_WARDROBE_MIP_BONUS;

		tWardrobeInit.nWardrobeFallback = nWardrobeFallback;
		tWardrobeInit.bForceFallback = pUnitData ? UnitDataTestFlag( pUnitData, UNIT_DATA_FLAG_ALWAYS_USE_WARDROBE_FALLBACK ) : FALSE;

		if (   TEST_MASK( dwFlags, APPEARANCE_NEW_UNIT_IS_CONTROL_UNIT )
			|| GameGetControlUnit( pGame ) == pUnit )
			tWardrobeInit.ePriority = WARDROBE_PRIORITY_PC_LOCAL;
		else if ( c_UnitIsInMyParty( pUnit ) )
			tWardrobeInit.ePriority = WARDROBE_PRIORITY_PC_PARTY;
		else if ( UnitIsPlayer( pUnit ) )
			tWardrobeInit.ePriority = WARDROBE_PRIORITY_PC_REMOTE;
		else if ( UnitIsA( pUnit, UNITTYPE_WARDROBED_CHARACTER ) )
			tWardrobeInit.ePriority = WARDROBE_PRIORITY_NPC;

		if ( IS_CLIENT( pGame ) && pUnit && pUnitData && 
			UnitDataTestFlag( pUnitData, UNIT_DATA_FLAG_LOW_LOD_IN_TOWN ) ) //&& UnitIsInTown( pUnit ) ) // can't see whether it is in town with no ROOM
		{
			tWardrobeInit.nLOD = LOD_LOW;
			tWardrobeInit.bForceLOD = TRUE;
		}

		if ( AppIsHammer() )
		{
			int nWardrobeBody = pAppearanceDef->nPreviewWardrobeBody;
			if ( nWardrobeBody == INVALID_ID )
				nWardrobeBody = pUnitData ? pUnitData->nWardrobeBody : INVALID_ID;
			if ( nWardrobeBody != INVALID_ID )
			{
				WardrobeApplyBodyDataToInit( pGame, tWardrobeInit, nWardrobeBody,
											 pAppearanceDef->nWardrobeAppearanceGroup );
			}

			if ( ( pAppearanceDef->dwFlags & APPEARANCE_DEFINITION_FLAG_WARDROBE_USES_INVENTORY ) != 0 )
			{
				// we need to make a head, hair, etc... when viewing players in Hammer 
				WardrobeRandomizeInitStruct( pGame, tWardrobeInit, pAppearanceDef->nWardrobeAppearanceGroup, FALSE );
			}
		}

		pAppearance->nWardrobe = WardrobeInit( pGame, tWardrobeInit );
		ASSERT_RETINVALID( pAppearance->nWardrobe != INVALID_ID );
		nModelDefId = c_WardrobeGetModelDefinition( pAppearance->nWardrobe );
		ASSERT( nWardrobeFallback == INVALID_ID || nModelDefId != INVALID_ID );
	}

	if ( nModelDefId != INVALID_ID )
	{
		V( e_ModelSetDefinition( nModelId, nModelDefId, LOD_ANY ) );
	}

	pAppearance->nModelDefinitionId = nModelDefId;

	EVENT_DATA eventData( nModelId, dwFlags, UnitGetId(pUnit) );
	DefinitionAddProcessCallback(DEFINITION_GROUP_APPEARANCE, nAppearanceDefId, c_sAppearanceNew, &eventData);
	return ( nModelId );
#else
	return INVALID_ID;
#endif
} // c_AppearanceNew()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static void c_sAppearanceDestroy( APPEARANCE *pAppearance )
{
	c_ModelRemove( pAppearance->nId );

	pAppearance->tEventIds.Destroy();

	// CHB 2006.11.30
	for (int j = 0; j < LOD_COUNT; ++j)
	{
		if ( pAppearance->pMeshes_[j] != 0 )
		{

			FREE( NULL, pAppearance->pMeshes_[j] );
			pAppearance->pMeshes_[j] = NULL;
		}
		pAppearance->nMeshCount_[j] = 0;
	}

	if ( pAppearance->nWardrobe != INVALID_ID )
	{
		if ( ( pAppearance->dwNewFlags & APPEARANCE_NEW_DONT_FREE_WARDROBE ) == 0 )
		{
			WardrobeFree( AppGetCltGame(), pAppearance->nWardrobe );
		}

		pAppearance->nWardrobe = INVALID_ID;
	}

	if ( pAppearance->pfFattenedBones )
	{
		FREE( NULL, pAppearance->pfFattenedBones );
		pAppearance->pfFattenedBones = NULL;
	}

	if ( pAppearance->pfLengthenedBones )
	{
		FREE( NULL, pAppearance->pfLengthenedBones );
		pAppearance->pfLengthenedBones = NULL;
	}

	if ( pAppearance->pnShrunkBones )
	{
		FREE( NULL, pAppearance->pnShrunkBones );
		pAppearance->pnShrunkBones = NULL;
	}

#ifdef HAVOK_ENABLED
	if ( pAppearance->pBonesToTrack.IsInitialized() )
	{
		pAppearance->pBonesToTrack.Destroy();
	}

	if( AppIsHellgate() )
	{
		ASSERT(sgbHavokOpen);
		if ( sgbHavokOpen == TRUE )
		{
			c_RagdollRemove( pAppearance->nId );
	
			if ( pAppearance->pAnimatedSkeleton )
			{
				pAppearance->pAnimatedSkeleton->removeReference();
				pAppearance->pAnimatedSkeleton = NULL;
			}
	
			if( pAppearance->pPosePhantom )
			{
				if( pAppearance->pPosePhantom->getWorld() )
				{
					hkWorld* world = pAppearance->pPosePhantom->getWorld();
					world->removePhantom( pAppearance->pPosePhantom );
				}
				pAppearance->pPosePhantom->removeReference();
				pAppearance->pPosePhantom = HK_NULL;
			}
	
			if( pAppearance->pPoseShape )
			{
				pAppearance->pPoseShape->removeReference();
				pAppearance->pPoseShape = HK_NULL;
			}
	
			for ( ANIMATION* pAnimation = HashGetFirst( pAppearance->pAnimations );
				pAnimation != NULL ;
				pAnimation = HashGetNext( pAppearance->pAnimations, pAnimation ) )
			{
				if ( pAnimation->pListener )
					delete pAnimation->pListener;
			}
			HashFree( pAppearance->pAnimations );
	
			pAppearance->pPoseInModel.clearAndDeallocate();
			pAppearance->pPoseInLocal.clearAndDeallocate();
		}
	}
#endif
	if( AppIsTugboat() )
	{
		HashFree( pAppearance->pAnimations );
	
		pAppearance->pGrannyPoseInModel.Destroy();
		pAppearance->pGrannyPoseInLocal.Destroy();
	
		if( pAppearance->pWorldPose )
		{
			GrannyFreeWorldPose( pAppearance->pWorldPose );
		}

		if ( pAppearance->pGrannyInstance )
		{
			GrannyFreeModelInstance( pAppearance->pGrannyInstance );
	
		}
	}
} // c_sAppearanceDestroy()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
void c_AppearanceDestroy( int nAppearanceId )
{
	APPEARANCE *pAppearance = HashGet( sgpAppearances, nAppearanceId );
	if ( !pAppearance )
	{
		c_ModelRemove( nAppearanceId );
		return;
	}

	c_sAppearanceDestroy( pAppearance );

	HashRemove( sgpAppearances, nAppearanceId );
} // c_AppearanceDestroy()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
void c_AppearanceSetFullbright( int nAppearanceId,
								BOOL bFullbright )
{
	APPEARANCE *pAppearance = HashGet( sgpAppearances, nAppearanceId );
	if ( pAppearance )
	{
		pAppearance->bFullbright = bFullbright;
		return;
	}
} // c_AppearanceSetFullbright()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
void c_AppearanceSetDrawBehind( int nAppearanceId,
								BOOL bDrawBehind )
{
	APPEARANCE *pAppearance = HashGet( sgpAppearances, nAppearanceId );
	if ( pAppearance )
	{
		pAppearance->bDrawBehind = bDrawBehind;
		return;
	}
} // c_AppearanceSetDrawBehind()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
void c_AppearanceSetDrawBehind( APPEARANCE *pAppearance,
								BOOL bDrawBehind )
{
	if ( pAppearance )
	{
		pAppearance->bDrawBehind = bDrawBehind;
		return;
	}
} // c_AppearanceSetDrawBehind()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
void c_AppearanceSetAmbientLight( int nAppearanceId,
								  float AmbientLight )
{
	APPEARANCE *pAppearance = HashGet( sgpAppearances, nAppearanceId );
	if ( pAppearance )
	{
		pAppearance->AmbientLight = AmbientLight;
		return;
	}
} // c_AppearanceSetAmbientLight()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
float c_AppearanceGetModelAmbientLight( int nAppearanceId )
{
	APPEARANCE *pAppearance = HashGet( sgpAppearances, nAppearanceId );
	if ( pAppearance )
	{
		return ( e_ModelGetAmbientLight( pAppearance->nId ) );
	}

	return ( 1 );
} // c_AppearanceGetModelAmbientLight()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
void c_AppearanceSetHit( int nAppearanceId )
{
	APPEARANCE *pAppearance = HashGet( sgpAppearances, nAppearanceId );
	if ( pAppearance )
	{
		pAppearance->bHit = TRUE;
		return;
	}
} // c_AppearanceSetHit()


///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
void c_AppearanceSetBehindColor( int nAppearanceId, float fBehindColor )
{
	APPEARANCE *pAppearance = HashGet( sgpAppearances, nAppearanceId );
	if ( pAppearance )
	{
		pAppearance->fBehindColor = fBehindColor;
		return;
	}
} // c_AppearanceSetBehindCOlor()


///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
float c_AppearanceGetHit( int nAppearanceId )
{
	APPEARANCE *pAppearance = HashGet( sgpAppearances, nAppearanceId );
	if ( pAppearance )
	{
		return ( e_ModelGetHit( pAppearance->nId ) );
	}

	return ( 0 );
} // c_AppearanceGetHit()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL c_AppearanceIsAggressive( int nAppearanceId )
{
	APPEARANCE *pAppearance = HashGet( sgpAppearances, nAppearanceId );
	if ( !pAppearance )
	{
		return ( FALSE );
	}

	return ( pAppearance->dwFlags & APPEARANCE_FLAG_AGGRESSIVE ) != 0;
} // c_AppearanceIsAggressive()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
void c_AppearanceMakeAggressive( int nAppearanceId )
{
	APPEARANCE *pAppearance = HashGet( sgpAppearances, nAppearanceId );
	if ( !pAppearance )
	{
		return;
	}

	if ( ( pAppearance->dwFlags & APPEARANCE_FLAG_AGGRESSIVE ) != 0 )
	{
		return;
	}

	pAppearance->dwFlags |= APPEARANCE_FLAG_AGGRESSIVE;
	sAppearanceUpdateContext( *pAppearance, NULL, NULL );
} // c_AppearanceMakeAggressive()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
struct UNIT *c_AppearanceGetUnit( GAME *pGame,
								  int nAppearanceId )
{
	APPEARANCE *pAppearance = HashGet( sgpAppearances, nAppearanceId );
	if ( !pAppearance )
	{
		return ( NULL );
	}

	return ( UnitGetById( pGame, pAppearance->idUnit ) );
} // c_AppearanceGetUnit()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL c_AppearanceIsAdjustingContext( int nAppearanceId )
{
	APPEARANCE *pAppearance = HashGet( sgpAppearances, nAppearanceId );
	if ( !pAppearance )
	{
		return ( FALSE );
	}

	return ( pAppearance->dwFlags & APPEARANCE_FLAG_ADJUSTING_CONTEXT ) != 0;
} // c_AppearanceIsAdjustingContext()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
int c_AppearanceGetStance( int nAppearanceId )
{
	APPEARANCE *pAppearance = HashGet( sgpAppearances, nAppearanceId );
	if ( !pAppearance )
	{
		return ( STANCE_DEFAULT );
	}

	return ( pAppearance->nStanceCurrent );
} // c_AppearanceGetStance()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// APPEARANCE - Events
///////////////////////////////////////////////////////////////////////////////////////////////////////
APPEARANCE_EVENTS *c_AppearanceGetEvents( int nAppearanceId )
{
	APPEARANCE *pAppearance = HashGet( sgpAppearances, nAppearanceId );
	if ( !pAppearance )
	{
		return ( NULL );
	}

	return ( &pAppearance->tEventIds );
} // c_AppearanceGetEvents()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
void c_AppearanceRemoveEvent( int nAppearanceId,
							  DWORD dwTicket )
{
	if ( dwTicket == 0 )
	{
		return;
	}

	// remove the event from the list 
	APPEARANCE_EVENTS *pEvents	= c_AppearanceGetEvents( nAppearanceId );
	ASSERT_RETURN( pEvents );

	int nCount					= pEvents->Count();
	for ( int i					= 0; i < nCount; i++ )
	{
		DWORD *pdwCurr = ( DWORD * )pEvents->GetPointer( i );
		if ( *pdwCurr == dwTicket )
		{
			ArrayRemove(*pEvents, i);
			break;
		}
	}
} // c_AppearanceRemoveEvent()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
void c_AppearanceCancelAllEvents( int nAppearanceId )
{
	APPEARANCE_EVENTS *pEvents	= c_AppearanceGetEvents( nAppearanceId );
	if ( !pEvents )
	{
		return;
	}

	int nCount					= pEvents->Count();
	for ( int i					= 0; i < nCount; i++ )
	{
		DWORD *pdwTicket = ( DWORD * )pEvents->GetPointer( i );
		CSchedulerCancelEvent( *pdwTicket );
	}

	ArrayClear(*pEvents);
}


///////////////////////////////////////////////////////////////////////////////////////////////////////
// APPEARANCE - Animation
///////////////////////////////////////////////////////////////////////////////////////////////////////
static ANIMATION *c_sAppearanceGetAnimationByMode( APPEARANCE &tAppearance,
												   int nUnitMode,
												   int *pnId )
{
	for ( ANIMATION * pAnimation = HashGetFirst( tAppearance.pAnimations ); 
		  pAnimation != NULL;
		  pAnimation = HashGetNext( tAppearance.pAnimations, pAnimation ) )
	{
		if ( pAnimation->pDefinition->nUnitMode == nUnitMode )
		{
			if ( pnId )
			{
				*pnId = pAnimation->nId;
			}

			return ( pAnimation );
		}
	}

	return ( NULL );
} // c_sAppearanceGetAnimationByMode()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL c_AppearanceIsPlayingAnimation( int nAppearanceId,
									 ANIMATION_DEFINITION *pAnimDef )
{
	APPEARANCE *pAppearance = HashGet( sgpAppearances, nAppearanceId );
	if ( !pAppearance )
	{
		return ( FALSE );
	}

	for ( ANIMATION * pAnimation = HashGetFirst( pAppearance->pAnimations ); 
		  pAnimation != NULL;
		  pAnimation = HashGetNext( pAppearance->pAnimations, pAnimation ) )
	{
		if ( pAnimation->pDefinition == pAnimDef && ( pAnimation->dwFlags & ANIMATION_FLAG_EASING_OUT ) == 0 )
		{
			return ( TRUE );
		}
	}

	return ( FALSE );
} // c_AppearanceIsPlayingAnimation()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL c_AppearanceIsPlayingAnimation( int nAppearanceId,
									 int nUnitMode )
{
	APPEARANCE *pAppearance = HashGet( sgpAppearances, nAppearanceId );
	if ( !pAppearance )
	{
		return ( FALSE );
	}

	for ( ANIMATION * pAnimation = HashGetFirst( pAppearance->pAnimations ); 
		  pAnimation != NULL;
		  pAnimation = HashGetNext( pAppearance->pAnimations, pAnimation ) )
	{
		if ( pAnimation->pDefinition->nUnitMode == nUnitMode && 
			 ( pAnimation->dwFlags & ANIMATION_FLAG_EASING_OUT ) == 0 )
		{
			return ( TRUE );
		}
	}

	return ( FALSE );
} // c_AppearanceIsPlayingAnimation()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL c_AppearanceIsApplyingAnimation( int nAppearanceId,
									ANIMATION_DEFINITION *pAnimDef )
{
	// uh, none of this code works for tugboat, so basically, none of our
	// animation events happen. I really don't feel like messing with it
	// right now either. Please keep tugboat in mind when making
	// big animation changes!
	if( AppIsTugboat() )
	{
		return ( TRUE );
	}
	APPEARANCE *pAppearance = HashGet( sgpAppearances, nAppearanceId );
	if ( !pAppearance )
	{
		return ( FALSE );
	}

	for ( ANIMATION * pAnimation = HashGetFirst( pAppearance->pAnimations ); 
		pAnimation != NULL;
		pAnimation = HashGetNext( pAppearance->pAnimations, pAnimation ) )
	{
		if ( pAnimation->pDefinition == pAnimDef 
#ifdef HAVOK_ENABLED
			&& ( !pAnimation->pControl
				|| pAnimation->pControl->getMasterWeight() > 0.0f )
#endif
				)
		{
			return ( TRUE );
		}
	}

	return ( FALSE );
} // c_AppearanceIsApplyingAnimation()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
float c_AppearanceGetAnimationPercent( int nAppearanceId,
									   int nUnitMode,
									   int &nOverflowCount )
{
	APPEARANCE *pAppearance		= HashGet( sgpAppearances, nAppearanceId );
	if ( !pAppearance )
	{
		return ( 0.0f );
	}

	nOverflowCount = 0;

	ANIMATION *pAnimation		= c_sAppearanceGetAnimationByMode( *pAppearance, nUnitMode, NULL );
#ifdef HAVOK_ENABLED
	if( AppIsHellgate() )
	{
		if ( !pAnimation || !pAnimation->pControl )
			return 0.0f;
	
		if ( !pAnimation->pControl )
			return 0.0f;
	
		float fDuration = sAnimationGetDuration( *pAnimation );
		float fLocalTime = pAnimation->pControl->getLocalTime();
	
		nOverflowCount = pAnimation->pControl->getOverflowCount();
		return FMODF( fLocalTime, fDuration )  / fDuration;

	}
#endif
	if( AppIsTugboat() )
	{
		if ( !pAnimation || ( !pAnimation->pGrannyControl ) )
		{
			return ( 0.0f );
		}
	
		if ( pAnimation->pGrannyControl )
		{
			// Find out the duration of the local clock 
			// Find out the animation-relative time for the control 
			float fDuration = sAnimationGetDuration( *pAnimation );
			float fLocalTime		= GrannyGetControlRawLocalClock( pAnimation->pGrannyControl );

			nOverflowCount = ( int )( fLocalTime / fDuration );
			return ( fmodf( fLocalTime, fDuration ) / fDuration );
		}
	
		float fLocalTime			= 0;
	
		nOverflowCount = ( int )( fLocalTime / pAnimation->fDuration );
		return ( fmodf( fLocalTime, pAnimation->fDuration ) / pAnimation->fDuration );
	}
	return 0.0f;
} // c_AppearanceGetAnimationPercent()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static float sDebugDisplayAnimationPercent = 0.0f;

void c_AppearanceSetDebugDisplayAnimationPercent(
	float fPercent)
{
	sDebugDisplayAnimationPercent = fPercent;
}

float c_AppearanceGetDebugDisplayAnimationPercent()
{
	return sDebugDisplayAnimationPercent;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef HAVOK_ENABLED
static BOOL sRagdollIsUsed( RAGDOLL * pRagdoll)
{
	BOOL bUseRagdoll = FALSE;
	if ( pRagdoll )
	{
		bUseRagdoll = TRUE;
	}
	if ( pRagdoll && (pRagdoll->dwFlags & RAGDOLL_FLAG_DISABLED) != 0 )
	{
		bUseRagdoll = FALSE;
	}
	return ( bUseRagdoll );
} // sRagdollIsUsed()
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static BOOL sAnimationIsActive( ANIMATION *pAnimation )
{
	if ( ( pAnimation->dwFlags & ANIMATION_FLAG_EASING_OUT ) != 0 )
	{
		return ( FALSE );
	}

	if ( pAnimation->dwPlayFlags & PLAY_ANIMATION_FLAG_LOOPS )
	{
		return ( TRUE );
	}

	if ( pAnimation->fDuration - ( pAnimation->fTimeSinceStart + pAnimation->pDefinition->fStartOffset ) < 0.1f ) // are we almost done playing it?
	{
		// are we almost done playing it? 
		return ( FALSE );
	}

	return ( TRUE );
} // sAnimationIsActive()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef HAVOK_ENABLED
static void sAnimationAddListener( 
	APPEARANCE * pAppearance, 
	ANIMATION * pAnimation,
	int nAnimationId )
{
	pAnimation->pListener = new MyControlListener();
	pAnimation->pListener->m_pAppearance = pAppearance;
	pAnimation->pListener->m_nAnimationId = nAnimationId;

	// Listen for overflow
	pAnimation->pControl->addDefaultControlListener( pAnimation->pListener );
	// Listen for deletion
	pAnimation->pControl->addAnimationControlListener( pAnimation->pListener );
} // sAnimationAddListener()
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static void sRemoveStanceAnimation( GAME *game,
									const CEVENT_DATA &data,
									DWORD dwTicket )
{
	int nAppearanceId			= (int)data.m_Data1;
	int nAnimationId			= (int)data.m_Data2;

	APPEARANCE *pAppearance		= HashGet( sgpAppearances, nAppearanceId );
	if ( pAppearance )
	{
		sAnimationRemove( pAppearance, nAnimationId );
	}
} // sRemoveStanceAnimation()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static BOOL c_sAppearanceMonitorContext( GAME *pGame,
										UNIT *pUnit,
										const struct EVENT_DATA &tEventData )
{
	ASSERT_RETFALSE( pUnit );
	UnitRegisterEventTimed( pUnit, c_sAppearanceMonitorContext, &tEventData, 1 );

	int nAppearanceId = (int)tEventData.m_Data1;

	APPEARANCE* pAppearance = HashGet(sgpAppearances, nAppearanceId);
	if ( ! pAppearance )
		return FALSE;

	if ( pAppearance->nAnimationGroup == INVALID_ID )
		return FALSE;
	BOOL bUpdateContext = FALSE;
	if ( c_AppearanceIsAggressive( nAppearanceId ) && 
		UnitGetTicksSinceLastAggressiveSkill( pGame, pUnit ) >= 3 * GAME_TICKS_PER_SECOND &&
		GameGetTick( pGame ) - pAppearance->nLastAdjustContextTick >= 3 * GAME_TICKS_PER_SECOND )
	{
		pAppearance->dwFlags &= ~APPEARANCE_FLAG_AGGRESSIVE;
		bUpdateContext = TRUE;
	}

	BOOL bUpdateStance = TRUE;
	ANIMATION_GROUP_DATA * pAnimGroupData = (ANIMATION_GROUP_DATA *) ExcelGetData ( NULL, DATATABLE_ANIMATION_GROUP, pAppearance->nAnimationGroup );
	int nDesiredStance = c_AnimationPlayTownAnims( pUnit ) ? pAnimGroupData->nDefaultStanceInTown : pAnimGroupData->nDefaultStance;
	if ( pAppearance->nStanceCurrent == nDesiredStance )
	{
		bUpdateStance = FALSE;
	}

	ANIMATION_STANCE_DATA * pStanceData = (ANIMATION_STANCE_DATA *) ExcelGetData( pGame, DATATABLE_ANIMATION_STANCE, pAppearance->nStanceCurrent );
	if ( pStanceData->bDontChangeFrom )
	{
		bUpdateStance = FALSE;
	}

	if ( ! bUpdateStance && ! bUpdateContext )
	{
		return FALSE;
	}

	if ( bUpdateStance )
	{
		bUpdateStance = FALSE;

		float fSecondsToHoldStance = c_AnimationPlayTownAnims( pUnit ) ? pAnimGroupData->fSecondsToHoldStanceInTown : pAnimGroupData->fSecondsToHoldStance;
		for ( ANIMATION* pAnimation = HashGetFirst( pAppearance->pAnimations ); 
			pAnimation != NULL; 
			pAnimation = HashGetNext( pAppearance->pAnimations, pAnimation ) )
		{
			if ( ( pAnimation->dwFlags & ANIMATION_FLAG_IS_ENABLED ) == 0 )
			{
				continue;
			}
#ifdef HAVOK_ENABLED
			if ( ( pAnimation->pDefinition->dwFlags & ANIMATION_DEFINITION_FLAG_HOLD_STANCE ) != 0 
				&& pAnimation->pControl && !pAnimation->pControl->IsEasedOut() )
			{
				bUpdateStance = FALSE;
				break;
			}
#endif
			if ( ( pAnimation->pDefinition->dwFlags & ANIMATION_DEFINITION_FLAG_USE_STANCE ) == 0 )
			{
				continue;
			}
			if ( pAnimation->pDefinition->nStartStance  != pAppearance->nStanceCurrent &&
				 pAnimation->pDefinition->nStartStance2 != pAppearance->nStanceCurrent &&
				 pAnimation->pDefinition->nStartStance3 != pAppearance->nStanceCurrent )
			{
				continue;
			}
			if ( pAnimation->fTimeSinceStart > fSecondsToHoldStance )
			{
				bUpdateStance = TRUE;
			}
		}
	}

	if ( bUpdateStance )
	{
		sAnimTrace( "ADJUSTING_STANCE", *pAppearance, NULL );

		float fDuration = 1.0f;
		ANIMATION_RESULT tAnimResult;
		c_AnimationPlayByMode( 
			pUnit, 
			pGame, 
			nAppearanceId, 
			MODE_ADJUST_STANCE, 
			pAppearance->nAnimationGroup, 
			fDuration, 
			PLAY_ANIMATION_FLAG_ONLY_ONCE, 
			TRUE, 
			nDesiredStance,
			&tAnimResult); 

		// remove any animations that don't have files used for adjusting stance, we need to
		// do this ourselves because there is no actual animation file for havok to process
		// and remove when finished
		TIME tiTime = (TIME)( fDuration * SCHEDULER_TIME_PER_SECOND );			
		for ( int i = 0; i < tAnimResult.nNumResults; ++i )
		{
			int nAppearanceId = tAnimResult.tResults[ i ].nAppearanceId;	
			int nAnimationID = tAnimResult.tResults[ i ].nAnimationId;
			if ( c_AnimationHasFile( nAppearanceId, nAnimationID ) == FALSE )
			{
				CEVENT_DATA tEventData( nAppearanceId, nAnimationID );
				CSchedulerRegisterEvent( tiTime, sRemoveStanceAnimation, tEventData );
			}
		}
	}
	else if ( bUpdateContext )
	{
		sAppearanceUpdateContext( *pAppearance, NULL, NULL );
	}

	return ( TRUE );
} // c_sAppearanceMonitorContext()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static BOOL sAppearanceUpdateContext( 
	APPEARANCE &tAppearance,
	ANIMATION *pChangingAnimation,
	ANIMATION_DEFINITION *pAnimDef,
	BOOL bForce )
{
	GAME *pGame								= AppGetCltGame();

	UNIT *pUnit								= UnitGetById( pGame, tAppearance.idUnit );
	if ( !pUnit )
	{
		return ( FALSE );
	}

	int nNewStance = tAppearance.nStanceCurrent;
	{
		BOOL bChangeContext = bForce;
		if ( pAnimDef && ( pAnimDef->dwFlags & ANIMATION_DEFINITION_FLAG_CHANGE_STANCE ) != 0 )
		{
			nNewStance = pAnimDef ? pAnimDef->nEndStance : tAppearance.nStanceCurrent;
			if ( nNewStance != 0 && nNewStance != tAppearance.nStanceCurrent )
				bChangeContext = TRUE;
		}

		if ( ! pAnimDef )
			bChangeContext = TRUE;

		const UNITMODE_DATA * pModeData = (pAnimDef && pAnimDef->nUnitMode != INVALID_ID) ? UnitModeGetData( pAnimDef->nUnitMode ) : NULL;
		if ( pModeData && pModeData->bIsAggressive && !c_AppearanceIsAggressive( tAppearance.nId ) )
		{
			bChangeContext = TRUE;
			tAppearance.dwFlags |= APPEARANCE_FLAG_AGGRESSIVE;
		}

		if ( ! bChangeContext )
			return TRUE;
	}

	float fEaseTime;
	if (pAnimDef && pChangingAnimation) 
		fEaseTime = pAnimDef->fStanceFadeTimePercent * pChangingAnimation->fDuration;
	else if ( c_AppearanceIsAggressive( tAppearance.nId ) && ! bForce ) 
		fEaseTime = 0.0f;
	else
		fEaseTime = 0.25f;
	APPEARANCE_DEFINITION *pAppearanceDef	= sAppearanceGetDefinition( tAppearance );
	if ( !pAppearanceDef )
	{
		return ( FALSE );
	}

	struct STANCE_CHANGE_ANIM
	{
		UNITMODE	eMode;
		DWORD		dwPlayFlags;
		float		fDuration;
	};

	STANCE_CHANGE_ANIM pAnimsToPlay[MAX_ANIMS_PER_APPEARANCE];
	int nAnimsToPlay						= 0;

	tAppearance.dwFlags |= APPEARANCE_FLAG_ADJUSTING_CONTEXT;


	for ( ANIMATION * pAnimation = HashGetFirst( tAppearance.pAnimations ), *pNextAnimation = NULL; pAnimation != NULL;
		  pAnimation = pNextAnimation )
	{
		pNextAnimation = HashGetNext( tAppearance.pAnimations, pAnimation );

		if ( pAnimation == pChangingAnimation )
			continue;

		// see if this animation already matches our current stance and stuff
		BOOL bFailedAnimationConditions = FALSE;
		for ( int i = 0; i < NUM_ANIMATION_CONDITIONS; i++ )
		{
			int nAnimationCondition = pAnimation->pDefinition->nAnimationCondition[ i ];

			ANIMATION_CONDITION_DATA * pCondition = nAnimationCondition != INVALID_ID ? (ANIMATION_CONDITION_DATA *) ExcelGetData( NULL, DATATABLE_ANIMATION_CONDITION, nAnimationCondition ) : NULL;
			if ( pCondition && pCondition->bCheckOnContextChange && 
				!c_AnimationConditionEvaluate( pGame, pUnit, nAnimationCondition, tAppearance.nId, pAnimation->pDefinition ) )
			{
				bFailedAnimationConditions = TRUE;
				break;
			}
		}

		if ( !bFailedAnimationConditions && ! bForce )
		{
	
			if ( ( pAnimation->pDefinition->dwFlags & ANIMATION_DEFINITION_FLAG_USE_STANCE ) == 0 ||
				 ( pAnimation->pDefinition->dwFlags & ANIMATION_DEFINITION_FLAG_CHANGE_STANCE ) != 0 )
			{
				continue;
			}
	
			if ( pAnimation->pDefinition->nEndStance == nNewStance )
			{
				continue;
			}
		}

		int nUnitMode			= pAnimation->pDefinition->nUnitMode;
		if ( nUnitMode == INVALID_ID )
		{
			continue;
		}
		// remove or ease out the animation
		BOOL bDefaultAnim		= FALSE;
		if ( pAnimation->dwPlayFlags & PLAY_ANIMATION_FLAG_DEFAULT_ANIM )
		{
			bDefaultAnim = TRUE;
			pAnimation->dwPlayFlags &= ~PLAY_ANIMATION_FLAG_DEFAULT_ANIM;   	 // this way it will clear after easing out 
		}

		if ( pAnimation->dwFlags & ANIMATION_FLAG_EASING_OUT )
		{
#ifdef HAVOK_ENABLED
			if( AppIsHellgate() )
			{
				if ( pAnimation->pControl && pAnimation->pControl->IsEasedOut() )
				{
					sAnimationRemove( &tAppearance, pAnimation->nId );
				}
			}
#endif
			if( AppIsTugboat() )
			{
				if ( pAnimation->pGrannyControl && GrannyGetControlEffectiveWeight( pAnimation->pGrannyControl ) <= 0 )
				{
					sAnimationRemove( &tAppearance, pAnimation->nId );
				}
			}
			continue;
		}

		BOOL bRemoveAnimation	= FALSE;

#ifdef HAVOK_ENABLED
		if ( pAnimation->pControl )
		{
			sAnimTrace( "ANIM_EASE_OUT UPDATE_STANCE", tAppearance, pAnimation );
			pAnimation->pControl->easeOut( fEaseTime );
		}
		else 
#endif
		if ( pAnimation->pGrannyControl )
		{
			GrannyEaseControlOut( pAnimation->pGrannyControl, fEaseTime );
		}
		else
		{
			bRemoveAnimation = TRUE;
		}

		pAnimation->dwFlags |= ANIMATION_FLAG_EASING_OUT;

		// remember what animations we need to play
		BOOL bInList = FALSE;
		for ( int i = 0; i < nAnimsToPlay; i++ )
		{
			if ( pAnimsToPlay[i].eMode == nUnitMode )
			{
				bInList = TRUE;
				break;
			}
		}

		if ( !bInList )
		{
			ASSERT_CONTINUE( nAnimsToPlay < MAX_ANIMS_PER_APPEARANCE );
			pAnimsToPlay[nAnimsToPlay].eMode = ( UNITMODE )nUnitMode;
			pAnimsToPlay[nAnimsToPlay].fDuration = pAnimation->fDuration;
			pAnimsToPlay[nAnimsToPlay].dwPlayFlags = 0;
			if ( pAnimation->dwPlayFlags & PLAY_ANIMATION_FLAG_LOOPS )
			{
				pAnimsToPlay[ nAnimsToPlay ].fDuration = 0.0f;
				pAnimsToPlay[ nAnimsToPlay ].dwPlayFlags |= PLAY_ANIMATION_FLAG_LOOPS;
			}
			if ( pAnimation->dwFlags & ANIMATION_FLAG_DURATION_ADJUSTED )
			{
				pAnimsToPlay[ nAnimsToPlay ].fDuration = pAnimation->fDuration * pAnimation->fAnimSpeed;
			}

			if ( bDefaultAnim )
			{
				pAnimsToPlay[nAnimsToPlay].dwPlayFlags |= PLAY_ANIMATION_FLAG_DEFAULT_ANIM;
			}

			nAnimsToPlay++;
		}

		if ( bRemoveAnimation )
		{
			sAnimationRemove( &tAppearance, pAnimation->nId );
		}
	}

	tAppearance.nStanceCurrent = nNewStance;
	tAppearance.nLastAdjustContextTick = GameGetTick( pGame );

	int nAnimationGroup						= UnitGetAnimationGroup( pUnit );

	sAnimTrace( "CHANGING_STANCE", tAppearance, NULL );

	for ( int i	= 0; i < nAnimsToPlay; i++ )
	{

		c_AnimationPlayByMode( pUnit, pGame, tAppearance.nId,
									  pAnimsToPlay[ i ].eMode,
									  nAnimationGroup,
									  pAnimsToPlay[ i ].fDuration,
									  pAnimsToPlay[ i ].dwPlayFlags | PLAY_ANIMATION_FLAG_ADJUSTING_CONTEXT | PLAY_ANIMATION_FLAG_USE_EASE_OVERRIDE,
									  TRUE, INVALID_ID, NULL, fEaseTime );
	}


	return ( TRUE );
} // sAppearanceUpdateContext()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////

BOOL c_AppearanceUpdateContext( 
	int nModelId,
	BOOL bForce )
{
	APPEARANCE * pAppearance = HashGet( sgpAppearances, nModelId );
	if ( ! pAppearance )
		return FALSE;
	return sAppearanceUpdateContext( *pAppearance, NULL, NULL, bForce );
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static int sAnimCompare( const void *pFirst,
						 const void *pSecond )
{
	ANIMATION *p1	= *( ANIMATION ** )pFirst;
	ANIMATION *p2	= *( ANIMATION ** )pSecond;

	if ( p1->nSortValue > p2->nSortValue )
	{
		return ( -1 );
	}

	if ( p1->nSortValue < p2->nSortValue )
	{
		return ( 1 );
	}

	return ( 0 );
} // sAnimCompare()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static BOOL sAppearanceUpdateWeights( APPEARANCE &tAppearance )
{
#ifdef HAVOK_ENABLED		
	if( AppIsHellgate() )
	{
		APPEARANCE_DEFINITION * pAppearanceDef = sAppearanceGetDefinition( tAppearance ); 
		if ( ! pAppearanceDef )
			return FALSE;
	
		tAppearance.dwFlags &= ~APPEARANCE_FLAG_UPDATE_WEIGHTS;
	
		if ( HashGetFirst( tAppearance.pAnimations ) == NULL )
			sAnimTrace( "NO_ANIMATIONS", tAppearance, NULL ); 
	
		ANIMATION * pSortedAnims[ MAX_ANIMS_PER_APPEARANCE ];
		int nAnimCount = 0;	
		
		ANIMATION * pAnimsToRemove[ MAX_ANIMS_PER_APPEARANCE ];
		int nAnimToRemoveCount = 0;

		for ( ANIMATION* pAnimation= HashGetFirst( tAppearance.pAnimations ); 
			pAnimation != NULL; 
			pAnimation = HashGetNext( tAppearance.pAnimations, pAnimation ))
		{
			if ( ! pAnimation->pControl )
				continue;
			ASSERT_RETFALSE( nAnimCount < MAX_ANIMS_PER_APPEARANCE );
			
			// if animation has a condition, evaluate it and apply any priority changes to the sort value
			int nPriority = pAnimation->nPriority;

			BOOL bFailedCondition = FALSE;
			for ( int i = 0; i < NUM_ANIMATION_CONDITIONS; i++ )
			{
				int nAnimationCondition = pAnimation->pDefinition->nAnimationCondition[ i ];		
				if (nAnimationCondition == INVALID_LINK)
					continue;

				const ANIMATION_CONDITION_DATA* pAnimConditionData = AnimationConditionGetData( nAnimationCondition );		
				
				GAME* pGame = AppGetCltGame();
				UNIT* pUnit = UnitGetById( pGame, tAppearance.idUnit );
				if ( ! pAnimConditionData->bCheckOnUpdateWeights )
					continue;

				if (c_AnimationConditionEvaluate( pGame, pUnit, nAnimationCondition, tAppearance.nId, pAnimation->pDefinition ))
				{
					// grant success priority boost
					nPriority += pAnimConditionData->nPriorityBoostSuccess;
				}
				else
				{
					if ( pAnimConditionData->bIgnoreOnFailure )
					{
						bFailedCondition = TRUE;
						break;
					}

					// remove animation on failure
					if (pAnimConditionData->bRemoveOnFailure)
					{
						pAnimsToRemove[ nAnimToRemoveCount++ ] = pAnimation;
						bFailedCondition = TRUE;
						break;
					}

					// grant failure priority boost
					nPriority += pAnimConditionData->nPriorityBoostFailure;
				}
			}
			if ( bFailedCondition )
				continue;
	
			// add animation to consider
			pSortedAnims[ nAnimCount ] = pAnimation;
			nAnimCount++;
			
			pAnimation->nSortValue = 1000 * nPriority + 10 * pAnimation->pDefinition->nBoneWeights;
			if ( pAnimation->dwFlags & ANIMATION_FLAG_EASING_OUT )
				pAnimation->nSortValue += 5;
			if ( pAnimation->dwFlags & ANIMATION_FLAG_EASING_IN )
				pAnimation->nSortValue += 2;
						
		}
		qsort( pSortedAnims, nAnimCount, sizeof( ANIMATION * ), sAnimCompare );
	
		// mark the last animation in each group of similar animations.
		for ( int i = 0; i < nAnimCount; )
		{
			ANIMATION * pAnimation = pSortedAnims[ i ];
		
			pAnimation->dwFlags &= ~ANIMATION_FLAG_LAST_OF_SIMILAR;
			int nLast = i;
			for ( ; i < nAnimCount; i++ )
			{
				ANIMATION * pAnimationCurr = pSortedAnims[ i ];
				pAnimationCurr->dwFlags &= ~ANIMATION_FLAG_LAST_OF_SIMILAR;
				if ( pAnimationCurr->nPriority != pAnimation->nPriority )
					break;
				if ( pAnimationCurr->pDefinition->nBoneWeights != pAnimation->pDefinition->nBoneWeights )
					break;
	
				nLast = i;
			}
	
			// mark the last animation in the group
			ANIMATION * pAnimationLast = pSortedAnims[ nLast ];
	//		if ( ( pAnimationLast->dwFlags & ANIMATION_FLAG_EASING_IN ) == 0 )
				pAnimationLast->dwFlags |= ANIMATION_FLAG_LAST_OF_SIMILAR;
		}
	
		int pnEaseOutGroups[ MAX_ANIMS_PER_APPEARANCE ];
		int nEaseOutGroupCount = 0;
	
		BOOL bEaseOutAll = FALSE;
		for ( int i = 0; i < nAnimCount; i++ )
		{
			ANIMATION * pAnimation = pSortedAnims[ i ];
	
			if ( ! pAnimation->pControl )
				continue;
	
			BOOL bEaseOut = bEaseOutAll;
			if ( ! bEaseOut )
			{
				for ( int j = 0; j < nEaseOutGroupCount; j++ )
				{
					if ( pAnimation->pDefinition->nBoneWeights == pnEaseOutGroups[ j ] )
					{
						bEaseOut = TRUE;
						break;
					}
				}
			}
			if ( ! bEaseOut )
			{
				if ( ( pAnimation->dwFlags & ANIMATION_FLAG_EASING_OUT ) != 0 )
					continue;
	
				if ( (pAnimation->dwFlags & ANIMATION_FLAG_LAST_OF_SIMILAR) != 0 )
				{
					if ( pAnimation->pDefinition->nBoneWeights == INVALID_ID )
						bEaseOutAll = TRUE;
					else {
						pnEaseOutGroups[ nEaseOutGroupCount ] = pAnimation->pDefinition->nBoneWeights;
						nEaseOutGroupCount++;
					}
				}
	
				pAnimation->pControl->setMasterWeight( pAnimation->fMaxMasterWeight );
	
				if ( (pAnimation->dwFlags & ANIMATION_FLAG_IS_ENABLED) == 0 &&
					(pAnimation->dwPlayFlags & PLAY_ANIMATION_FLAG_DEFAULT_ANIM ) != 0 &&
					nAnimCount > 1 )
				{
					sAnimTrace( "ANIM_EASE_IN", tAppearance, pAnimation );
					float fEaseIn = pAnimation->pDefinition->fEaseIn > 0.0f ? pAnimation->pDefinition->fEaseIn : 0.1f;
					pAnimation->pControl->ForceeaseIn( fEaseIn );
				}
	
				pAnimation->dwFlags |= ANIMATION_FLAG_IS_ENABLED;
			}
			else 
			{
				if ( (pAnimation->dwFlags & ANIMATION_FLAG_EASING_OUT ) != 0  )
				{
				}
				else if ( (pAnimation->dwFlags & ANIMATION_FLAG_IS_ENABLED) != 0 &&
					(pAnimation->dwPlayFlags & PLAY_ANIMATION_FLAG_DEFAULT_ANIM ) != 0 &&
					! pAnimation->pControl->IsEasedOut() )
				{
					sAnimTrace( "ANIM_EASE_OUT UPDATE_WEIGHTS", tAppearance, pAnimation );
					pAnimation->pControl->easeOut( pAnimation->pDefinition->fEaseOut );
				} else {
					sAnimTrace( "ANIM_SET_WEIGHT TO ZERO UPDATE_WEIGHTS", tAppearance, pAnimation );
					pAnimation->pControl->setMasterWeight( 0.0f );
				}
				pAnimation->dwFlags &= ~ANIMATION_FLAG_IS_ENABLED;
			}
		}
	
		// make sure that each animation that can mix has the same local time
		BOOL pbLocalTimeNeedsUpdate[ MAX_ANIMS_PER_APPEARANCE ];
		for ( int i = 0; i < nAnimCount; i++ )
		{
			pbLocalTimeNeedsUpdate[ i ] = ( pSortedAnims[ i ]->pDefinition->dwFlags & ANIMATION_DEFINITION_FLAG_CAN_MIX ) != 0;
		}
	
		for ( int i = 0; i < nAnimCount; i++ )
		{
			if ( !pbLocalTimeNeedsUpdate[ i ] )
				continue;
			if ( ! pSortedAnims[ i ]->pControl )
				continue;
	
			float fLocalTimePercent = 0.0f;
			for ( int j = i; j < nAnimCount; j++ )
			{
				ANIMATION * pAnimationCurr = pSortedAnims[ j ];
	
				if ( ( pAnimationCurr->dwFlags & ANIMATION_FLAG_IS_ENABLED ) != 0 &&
					pAnimationCurr->pControl &&
					pAnimationCurr->pControl->getPlaybackSpeed() != 0.0f &&
					( pAnimationCurr->pDefinition->dwFlags & ANIMATION_DEFINITION_FLAG_CAN_MIX ) != 0 )
				{
					float fCurrTime = pAnimationCurr->pControl->getLocalTime();
					if ( fCurrTime != 0.0f )
					{
						fLocalTimePercent = fCurrTime / sAnimationGetDuration( *pAnimationCurr->pDefinition );
						if ( pAnimationCurr->pDefinition->fStartOffset != 0.0f )
						{
							fLocalTimePercent -= pAnimationCurr->pDefinition->fStartOffset;
							while ( fLocalTimePercent < 0.0f )
							{
								fLocalTimePercent += 1.0f;
							}
						}
						break;
					}
				}
			}
	
			fLocalTimePercent = PIN( fLocalTimePercent, 0.0f, 1.0f );
	
			// sync up the local times
			for ( int j = i; j < nAnimCount; j++ )
			{
				ANIMATION * pAnimationCurr = pSortedAnims[ j ];
	
				if ( pAnimationCurr->pControl && 
					( pAnimationCurr->pDefinition->dwFlags & ANIMATION_DEFINITION_FLAG_CAN_MIX ) != 0 )
				{
					float fDuration = sAnimationGetDuration( *pAnimationCurr->pDefinition );
					float fPercent = fLocalTimePercent;
					if ( pAnimationCurr->pDefinition->fStartOffset != 0.0f )
					{
						fPercent += pAnimationCurr->pDefinition->fStartOffset;
						while ( fPercent > 1.0f )
							fPercent -= 1.0f;
					}
					sSetControlLocalTime( pAnimationCurr->pControl, fDuration * fPercent, fDuration );
					pbLocalTimeNeedsUpdate[ i ] = FALSE;
				}
	
			}
		}
	
		// remove animations that failed their condition (if directed)
		for (int i = 0; i < nAnimToRemoveCount; ++i)
		{
			ANIMATION* pAnimation = pAnimsToRemove[ i ];
			
			if (pAnimation->pControl && 
				pAnimation->dwFlags & ANIMATION_FLAG_IS_ENABLED &&
				pAnimation->pControl->IsEasedOut() == FALSE)
			{
				pAnimation->dwFlags |= ANIMATION_FLAG_EASING_OUT;
				sAnimTrace( "ANIM_EASE_OUT UPDATE_WEIGHTS FAILED_CONDITION", tAppearance, pAnimation );
				pAnimation->pControl->easeOut( pAnimation->pDefinition->fEaseOut );
				c_sControlSetSpeed( pAnimation->pControl, 0.0f, pAnimation, &tAppearance.tEventIds );
			} 
			else 
			{
				sAnimationRemove( &tAppearance, pAnimation->nId );
			}
			
		}
	
		tAppearance.dwFlags &= ~APPEARANCE_FLAG_ADJUSTING_CONTEXT;
	}
#endif // HAVOK_ENABLED
	if( AppIsTugboat() )
	{
		APPEARANCE_DEFINITION *pAppearanceDef	= sAppearanceGetDefinition( tAppearance );
		if ( !pAppearanceDef )
		{
			return ( FALSE );
		}
	
		tAppearance.dwFlags &= ~APPEARANCE_FLAG_UPDATE_WEIGHTS;
	
		if ( HashGetFirst( tAppearance.pAnimations ) == NULL )
		{
			sAnimTrace( "NO_ANIMATIONS", tAppearance, NULL );
	
		}
	
		int nAnimCount							= 0;
	
		float fEaseTime							= 0;
		for ( ANIMATION * pAnimation = HashGetFirst( tAppearance.pAnimations ); pAnimation != NULL;
	
	
	
			  pAnimation = HashGetNext( tAppearance.pAnimations, pAnimation ) )
		{
			if ( !pAnimation->pGrannyControl )
			{
				continue;
			}
	
			if ( pAnimation->dwFlags & ANIMATION_FLAG_EASING_OUT &&
				 ( pAnimation->dwPlayFlags & PLAY_ANIMATION_FLAG_DEFAULT_ANIM ) == 0 )
			{
				float fTime = sAnimationGetDuration( *pAnimation->pDefinition ) -
					GrannyGetControlRawLocalClock( pAnimation->pGrannyControl );
				if ( fTime > fEaseTime && GrannyGetControlLoopCount( pAnimation->pGrannyControl ) != 0 )
				{
					fEaseTime = fTime;
				}
				continue;
			}
			nAnimCount++;
		}
	
		if ( fEaseTime > kEaseOutConstant || fEaseTime == 0 )
		{
			fEaseTime = kEaseOutConstant;
		}
	
		for ( ANIMATION * pAnimation = HashGetFirst( tAppearance.pAnimations ); pAnimation != NULL;
			  pAnimation = HashGetNext( tAppearance.pAnimations, pAnimation ) )
		{
			if ( !pAnimation->pGrannyControl )
			{
				continue;
			}
	
			float weight	= 0;
	
			BOOL HasFreeze	= c_AnimationHasFreezeEvent( *pAnimation->pDefinition );
	
			if ( pAnimation->pGrannyControl )
			{
				weight = GrannyGetControlEffectiveWeight( pAnimation->pGrannyControl );
			}
	
			if ( nAnimCount > 1 )
			{
				if ( pAnimation->pGrannyControl )
				{
					if ( !( pAnimation->dwFlags & ANIMATION_FLAG_EASING_OUT ) &&
						 ( pAnimation->dwPlayFlags & PLAY_ANIMATION_FLAG_DEFAULT_ANIM ) != 0 &&
						 GrannyGetControlEffectiveWeight( pAnimation->pGrannyControl ) > 0 && !HasFreeze )
					{
						if ( tAppearance.dwFlags & APPEARANCE_FLAG_FREEZE_ANIMATION )
						{
							GrannyEaseControlOut( pAnimation->pGrannyControl, 0 );
						}
						else
						{
							float fEaseOut = kEaseOutConstant;
							if ( pAnimation->pDefinition->fEaseOut == -1 )
							{
								fEaseOut = 0;
							}
							else if ( pAnimation->pDefinition->fEaseOut != 0 )
	
	
							{
								fEaseOut = pAnimation->pDefinition->fEaseOut;
							}
							GrannyEaseControlOut( pAnimation->pGrannyControl, fEaseOut );
						} 
	
 						pAnimation->dwFlags |= ANIMATION_FLAG_EASING_OUT;
						pAnimation->dwFlags &= ~ANIMATION_FLAG_EASING_IN;
					}
	
					else if ( tAppearance.dwFlags & APPEARANCE_FLAG_FREEZE_ANIMATION &&
							  ( pAnimation->dwPlayFlags & PLAY_ANIMATION_FLAG_DEFAULT_ANIM ) == 0 && HasFreeze &&
							  nAnimCount == 2 )
					{
						pAnimation->dwFlags &= ~ANIMATION_FLAG_EASING_OUT;
						pAnimation->dwFlags |= ANIMATION_FLAG_EASING_IN;
						GrannyEaseControlIn( pAnimation->pGrannyControl, 0, TRUE );
						GrannySetControlRawLocalClock( pAnimation->pGrannyControl,
													   sAnimationGetDuration( *pAnimation->pDefinition ) );
	
						tAppearance.dwFlags |= APPEARANCE_FLAG_LAST_FRAME;
					}
				}
			}
	
			else
			{
				if ( pAnimation->pGrannyControl )
				{
					if ( !( pAnimation->dwFlags & ANIMATION_FLAG_EASING_IN ) &&
						 ( pAnimation->dwPlayFlags & PLAY_ANIMATION_FLAG_DEFAULT_ANIM ) != 0 &&
						 GrannyGetControlEffectiveWeight( pAnimation->pGrannyControl ) < 1 )
					{
						pAnimation->dwFlags &= ~ANIMATION_FLAG_EASING_OUT;
						pAnimation->dwFlags |= ANIMATION_FLAG_EASING_IN;
						GrannyEaseControlIn( pAnimation->pGrannyControl, fEaseTime, TRUE );
						if( GrannyGetControlWeight( pAnimation->pGrannyControl ) == 0 )
						{
							GrannySetControlWeight( pAnimation->pGrannyControl, 1 );
						}
					}
					else if ( tAppearance.dwFlags & APPEARANCE_FLAG_FREEZE_ANIMATION )
					{
						pAnimation->dwFlags &= ~ANIMATION_FLAG_EASING_OUT;
						pAnimation->dwFlags |= ANIMATION_FLAG_EASING_IN;
						GrannyEaseControlIn( pAnimation->pGrannyControl, 0, TRUE );
						tAppearance.dwFlags |= APPEARANCE_FLAG_LAST_FRAME;

					}
				}
	
			}
		}
	}

	return ( TRUE );
} // sAppearanceUpdateWeights()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static void sAnimationRemove( APPEARANCE *pAppearance,
							  int nAnimationId )
{
#ifdef HAVOK_ENABLED
	if( AppIsHellgate() )
	{
		if ( ! pAppearance->pAnimatedSkeleton )
			return;
	
		ANIMATION * pAnimation = HashGet( pAppearance->pAnimations, nAnimationId );
		if ( ! pAnimation )
			return;
	
		sAnimTrace( "REMOVE_ANIM", *pAppearance, pAnimation );
	
		if ( pAnimation->pControl )
		{
			pAppearance->pAnimatedSkeleton->removeAnimationControl( pAnimation->pControl );
			pAnimation->pControl->removeReference();
			pAnimation->pControl = NULL;
		}
	
		if ( pAnimation->pListener )
			delete pAnimation->pListener;
	
		/*
		if (pAppearance->nId == 1 && nAnimationId == 1)
		{
			LogMessage( "Removing 1/1" );
		}
		// */
	
		HashRemove( pAppearance->pAnimations, nAnimationId );
	
		pAppearance->dwFlags |= APPEARANCE_FLAG_UPDATE_WEIGHTS;
	}
#endif
	if( AppIsTugboat() )
	{
		if ( !pAppearance->pGrannyInstance )
		{
			return;
		}
	
		ANIMATION *pAnimation = HashGet( pAppearance->pAnimations, nAnimationId );
		if ( !pAnimation )
		{
			return;
		}
	
		sAnimTrace( "REMOVE_ANIM", *pAppearance, pAnimation );
	
		if ( pAnimation->pGrannyControl )
		{
			GrannyFreeControl( pAnimation->pGrannyControl );
			pAnimation->pGrannyControl = NULL;
		}
	
	
	
	
	
		if ( pAppearance->nId == 1 && nAnimationId == 1 )
		{
			LogMessage( "Removing 1/1" );
		}
	
	
		HashRemove( pAppearance->pAnimations, nAnimationId );
	
		pAppearance->dwFlags |= APPEARANCE_FLAG_UPDATE_WEIGHTS;
	}
} // sAnimationRemove()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static void sAnimationSetSpeedAndDuration( APPEARANCE *pAppearance,
										   ANIMATION *pAnimation )
{
	ANIMATION_DEFINITION *pAnimDef	= pAnimation->pDefinition;
	ASSERT_RETURN( pAnimDef );
#ifdef HAVOK_ENABLED		
	if( AppIsHellgate() )
	{
		ASSERT_RETURN( pAnimDef->pBinding );
	
		float fPeriod = sAnimationGetDuration( *pAnimDef );
		ASSERT_RETURN( fPeriod > 0.001f );
	
		float fControlSpeed = 1.0f;
		if ( pAnimDef->dwFlags & ANIMATION_DEFINITION_FLAG_FORCE_ANIMATION_SPEED )
		{
			pAnimation->fAnimSpeed = 1.0f;
			if ( pAnimDef->fDuration )
				fControlSpeed = fPeriod / pAnimDef->fDuration;
		} 
		else 
		{
			if ( pAnimDef->fVelocity == 0.0f )
			{
				pAnimation->fAnimSpeed = 1.0f;
			}
	
			if ( pAnimation->fDuration != fPeriod && pAnimation->fDuration != 0.0f && fPeriod != 0.0f )
				fControlSpeed = fPeriod / pAnimation->fDuration;
			else if ( pAnimDef->pBinding )
				pAnimation->fDuration = fPeriod;
	
			if ( pAnimation->fAnimSpeed != 0.0f && pAnimation->fAnimSpeed != 1.0f && 
				(pAnimation->dwFlags & ANIMATION_FLAG_DURATION_ADJUSTED) == 0 )
			{
				fControlSpeed *= pAnimation->fAnimSpeed;
				pAnimation->fDuration /= pAnimation->fAnimSpeed;
				pAnimation->dwFlags |= ANIMATION_FLAG_DURATION_ADJUSTED;
			}
			if( fControlSpeed > MAX_CONTROL_SPEED )
				fControlSpeed = MAX_CONTROL_SPEED;
		}
	
		c_sControlSetSpeed( pAnimation->pControl, fControlSpeed, pAnimation, &pAppearance->tEventIds );
	}
#endif
	if( AppIsTugboat() )
	{
		ASSERT_RETURN( pAnimDef->pGrannyAnimation );
	
		float fPeriod					= sAnimationGetDuration( *pAnimDef );
		ASSERT_RETURN( fPeriod > 0.001f );
	
		float fControlSpeed				= 1.0f;
		float fAnimSpeed				= 1.0f;
		if ( pAnimDef->dwFlags & ANIMATION_DEFINITION_FLAG_FORCE_ANIMATION_SPEED )
		{
	
			if ( pAnimDef->fDuration )
			{
				fControlSpeed = fPeriod / pAnimDef->fDuration;
			}
		}
		else
		{

	
			if ( pAnimDef->bIsMobile )
			{
				float fScale = pAnimation->fMovementSpeed;
				if ( fScale != 0 )
				{
					if ( fScale < .01f )
					{
						fScale = .01f;
					}
	
					fAnimSpeed *= fScale;
				}
			}
	
			if ( pAnimation->fDuration != fPeriod && pAnimation->fDuration != 0.0f && fPeriod != 0.0f )
			{
				fControlSpeed = fPeriod / pAnimation->fDuration;
			}
			else if ( pAnimDef->pGrannyAnimation )
			{
				pAnimation->fDuration = fPeriod;
	
			}
	
			if ( ( ( fAnimSpeed != 0.0f && fAnimSpeed != 1.0f ) || ( pAnimDef->fVelocity != 0 && pAnimDef->fVelocity != 1 ) ) 
				  && ( pAnimation->dwFlags & ANIMATION_FLAG_DURATION_ADJUSTED ) == 0 )
			{
				fControlSpeed *= fAnimSpeed;
	
	
				pAnimation->dwFlags |= ANIMATION_FLAG_DURATION_ADJUSTED;
			}
	
			if( fControlSpeed > MAX_CONTROL_SPEED )
			{
				fControlSpeed = MAX_CONTROL_SPEED;
			}
	
	
	
		}
	
		c_sControlSetSpeed( pAnimation->pGrannyControl, fControlSpeed, pAnimation, &pAppearance->tEventIds );
	}
} // sAnimationSetSpeedAndDuration()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static void sAnimationAddControlAndUpdateDuration( APPEARANCE *pAppearance,
												   ANIMATION *pAnimation,
												   float fEaseOverride )
{
#ifdef HAVOK_ENABLED		
	if( AppIsHellgate() )
	{
		ANIMATION_DEFINITION * pAnimDef = pAnimation->pDefinition;
		pAnimation->pControl = new CMyAnimControl( pAnimDef->pBinding );
	
		c_AppearanceUpdateAnimsWithVelocity( pAppearance );
		sAnimationSetSpeedAndDuration( pAppearance, pAnimation );
	
		float fLocalTime = 0.0f;
		float fDuration = sAnimationGetDuration( *pAnimDef );
		if ( pAnimDef->fStartOffset != 0.0f )
		{
			float fOffset = PIN( pAnimDef->fStartOffset, 0.0f, 1.0f );
			fLocalTime = fDuration * fOffset;
		}
	
		if ( fLocalTime != 0.0f )
		{
			sSetControlLocalTime( pAnimation->pControl, fLocalTime, fDuration );
		}

		if ( pAnimation->dwPlayFlags & PLAY_ANIMATION_FLAG_RAND_START_TIME )
		{
			RAND tRand;
			RandInit( tRand );
			c_sAppearanceRandomizeStartOffsetAnimations( pAppearance, RandGetFloat( tRand, 1.0f ) );		
		}
	
		sAnimationAddListener( pAppearance, pAnimation, pAnimation->nId );
	
		float fEaseIn = pAnimDef->fEaseIn;
		if ( ( pAnimation->dwPlayFlags & PLAY_ANIMATION_FLAG_USE_EASE_OVERRIDE ) != 0 &&
			fEaseOverride != 0.0f )
			fEaseIn = fEaseOverride;
		if ( fEaseIn > 0.0f )
		{
			sAnimTrace( "ANIM_EASE_IN", *pAppearance, pAnimation );	
			pAnimation->pControl->ForceeaseIn( fEaseIn );
			pAnimation->dwFlags |= ANIMATION_FLAG_EASING_IN;
		}
	
		pAppearance->pAnimatedSkeleton->addAnimationControl( pAnimation->pControl );
	
		if ( pAnimation->pDefinition->nBoneWeights != INVALID_ID )
		{
			hkArray< hkUint8 > & pnControlWeights = pAnimation->pControl->getTracksWeights();
	
			int nBoneCount = pnControlWeights.getSize();
	
			// Note: all the places that call this should be dealing with async load gracefully
			// If they don't, then that's bad.
			APPEARANCE_DEFINITION * pAppearanceDef = sAppearanceGetDefinition( *pAppearance );
			ASSERT(pAppearanceDef);
	
			if (pAppearanceDef)
			{
				float * pfWeights = & pAppearanceDef->pfWeights[ pAppearanceDef->nBoneCount * pAnimation->pDefinition->nBoneWeights ];
	
				for ( int i = 0; i < nBoneCount; i++ )
				{
					pnControlWeights[ i ]  = (hkUint8)max( (int)(pfWeights[ i ] * 255), 0 );
				}
			}
		}
	}
#endif
	if( AppIsTugboat() )
	{
		GAME *pGame						= AppGetCltGame();
		ANIMATION_DEFINITION *pAnimDef	= pAnimation->pDefinition;
	
		pAnimation->pGrannyControl = NULL;
		if ( pAnimDef->pGrannyAnimation )
		{
			pAnimation->pGrannyControl = GrannyPlayControlledAnimation( pGame->fAnimationUpdateTime,
																		pAnimDef->pGrannyAnimation,
																		pAppearance->pGrannyInstance );
			if ( pAnimation->dwPlayFlags & PLAY_ANIMATION_FLAG_LOOPS
				 && !c_AnimationHasFreezeEvent( *pAnimation->pDefinition ) )
			{
				GrannySetControlLoopCount( pAnimation->pGrannyControl, 0 );
			}
			else
			{
				GrannySetControlLoopCount( pAnimation->pGrannyControl, 1 );
			}
	
			GrannyFreeControlOnceUnused( pAnimation->pGrannyControl );
		}
	
		sAnimationSetSpeedAndDuration( pAppearance, pAnimation );
	
		if ( pAnimation->dwPlayFlags & PLAY_ANIMATION_FLAG_RAND_START_TIME )
		{
			if ( pAnimation->pGrannyControl )
			{
				GrannySetControlRawLocalClock( pAnimation->pGrannyControl,
											   RandGetFloat( pGame, sAnimationGetDuration( *pAnimDef ) ) );
			}
		}
	
		if ( pAnimation->pGrannyControl )
		{
			bool StartFromCurrent = FALSE;
			if ( pAnimation->pDefinition->fEaseIn == 99 )
			{
				StartFromCurrent = true;
			}
	
			if ( pAnimation->pDefinition->fEaseIn == -1 )
			{
				pAnimation->dwFlags &= ~ANIMATION_FLAG_EASING_IN;
				pAnimation->dwFlags &= ~ANIMATION_FLAG_EASING_OUT;
	
				for ( ANIMATION * pActiveAnimation = HashGetFirst( pAppearance->pAnimations ); pActiveAnimation != NULL;
					  pActiveAnimation = HashGetNext( pAppearance->pAnimations, pActiveAnimation ) )
				{
					if ( !pActiveAnimation->pGrannyControl || pActiveAnimation->pDefinition == pAnimation->pDefinition )
					{
						continue;
					}
					GrannySetControlWeight( pActiveAnimation->pGrannyControl, 0 );
	
					pActiveAnimation->dwFlags &= ~ANIMATION_FLAG_EASING_IN;
					pActiveAnimation->dwFlags &= ~ANIMATION_FLAG_EASING_OUT;
				}
			}
			else
			{
				GrannyEaseControlIn( pAnimation->pGrannyControl, kEaseInConstant, StartFromCurrent );
				pAnimation->dwFlags |= ANIMATION_FLAG_EASING_IN;
				pAnimation->dwFlags &= ~ANIMATION_FLAG_EASING_OUT;
			}
		}
	}
} // sAnimationAddControlAndUpdateDuration()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static void sTrackNewAnimation( 
	ANIMATION_RESULT *pAnimResult, 
	APPEARANCE *pAppearance, 
	ANIMATION *pAnimation)
{
	if (pAnimResult)
	{
		ASSERTX_RETURN( pAnimation, "Expected animation" );	
		ASSERTX_RETURN( pAnimResult->nNumResults <= MAX_ANIMATION_RESULT - 1, "Anim result buffer to small" );	
		pAnimResult->tResults[ pAnimResult->nNumResults ].nAppearanceId = pAppearance->nId;		
		pAnimResult->tResults[ pAnimResult->nNumResults ].nAnimationId = pAnimation->nId;
		pAnimResult->nNumResults++;
	}	
} // sTrackNewAnimation()	

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static void sAnimationInit( ANIMATION *pAnimation )
{
	ASSERTX_RETURN( pAnimation, "Expected animation" );

	// initialize all data members (leave the hash ones alone though) 
	pAnimation->dwPlayFlags = 0;
	pAnimation->dwFlags = 0;
	pAnimation->fMovementSpeed = 0.0f;
#ifdef HAVOK_ENABLED		
	pAnimation->pControl = NULL;
#endif
	pAnimation->pGrannyControl = NULL;

	pAnimation->pDefinition = NULL;
	pAnimation->fTimeSinceStart = 0.0f;
	pAnimation->fDuration = 0.0f;
#ifdef HAVOK_ENABLED		
	pAnimation->pListener = NULL;
#endif

	pAnimation->nPriority = 0;
	pAnimation->nSortValue = 0;
} // sAnimationInit()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// Hellgate's version!
///////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL c_AppearancePlayAnimation ( 
	GAME * pGame, 
	int nAppearanceId, 
	ANIMATION_DEFINITION * pAnimDef, 
	float fDuration, 
	DWORD dwFlags,
	ANIMATION_RESULT *pAnimResult,
	float *pfDurationUsed,				// return the actual duration (multiplied by the control speed) of the last animation played with the same def
	float fEaseOverride )
{
	if ( pfDurationUsed != NULL )
		*pfDurationUsed = 0.0f;

	ASSERT_RETFALSE( pAnimDef );
	ASSERT_RETFALSE( fDuration == 0.0f || fDuration >= 0.001f );

	APPEARANCE* pAppearance = HashGet(sgpAppearances, nAppearanceId);
	if (!pAppearance)
	{
		return FALSE;
	}

	if ( ( pAnimDef->dwFlags & ANIMATION_DEFINITION_FLAG_FORCE_ANIMATION_SPEED ) != 0 &&
		fDuration > pAnimDef->fDuration )
		dwFlags |= PLAY_ANIMATION_FLAG_LOOPS;

	//if ( dwFlags & PLAY_ANIMATION_FLAG_ONLY_ONCE ) 
	{
		for ( ANIMATION* pAnimation = HashGetFirst( pAppearance->pAnimations );
			pAnimation != NULL;
			pAnimation = HashGetNext( pAppearance->pAnimations, pAnimation ) )
		{

			if( AppIsTugboat() )
			{
				if ( pAnimation->pDefinition == pAnimDef && pAnimation->dwPlayFlags & PLAY_ANIMATION_FLAG_DEFAULT_ANIM )
				{
					return ( TRUE );
				}

				float fEaseOut = kEaseOutConstant;
				if ( pAnimation->pDefinition->fEaseOut == -1 )
				{
					fEaseOut = 0;
				}
				else if ( pAnimation->pDefinition->fEaseOut != 0 )
				{
					fEaseOut = pAnimation->pDefinition->fEaseOut;
				}
	
				if ( pAnimation->pDefinition != pAnimDef && pAnimation->pGrannyControl )
				{
					if ( pAnimation->dwPlayFlags & PLAY_ANIMATION_FLAG_DEFAULT_ANIM )
					{
					}
					else if ( ( pAnimation->dwFlags & ANIMATION_FLAG_EASING_OUT ) == 0 )
					{
						float fEaseTime = sAnimationGetDuration( *pAnimation->pDefinition ) -
							GrannyGetControlRawLocalClock( pAnimation->pGrannyControl );
						if ( fEaseTime > fEaseOut || GrannyGetControlLoopCount( pAnimation->pGrannyControl ) == 0 )
						{
							fEaseTime = fEaseOut;
						}
	
						GrannyEaseControlOut( pAnimation->pGrannyControl, fEaseTime );
						pAnimation->dwFlags |= ANIMATION_FLAG_EASING_OUT;
						pAnimation->dwFlags &= ~ANIMATION_FLAG_EASING_IN;
						pAppearance->dwFlags |= APPEARANCE_FLAG_UPDATE_WEIGHTS;
					}
				}
				else if ( pAnimation->pDefinition == pAnimDef && pAnimation->pGrannyControl )
				{
					if ( pfDurationUsed != NULL )
						*pfDurationUsed = 
							sAnimationGetDuration( *pAnimation ) * 
							c_sControlGetSpeed( pAnimation->pGrannyControl );

					if ( pAnimation->dwPlayFlags & PLAY_ANIMATION_FLAG_DEFAULT_ANIM )
					{
						return ( TRUE );
					}
	
					if ( dwFlags & PLAY_ANIMATION_FLAG_LOOPS )
					{
						if ( GrannyGetControlLoopCount( pAnimation->pGrannyControl ) == 0 )
						{
							if ( pAnimation->dwFlags & ANIMATION_FLAG_EASING_OUT )
							{
								GrannyEaseControlIn( pAnimation->pGrannyControl, kEaseInConstant, TRUE );
								pAnimation->dwFlags |= ANIMATION_FLAG_EASING_IN;
								pAnimation->dwFlags &= ~ANIMATION_FLAG_EASING_OUT;
								pAppearance->dwFlags |= APPEARANCE_FLAG_UPDATE_WEIGHTS;
							}

							return ( TRUE );
						}
					}
	
					if ( GrannyGetControlLoopCount( pAnimation->pGrannyControl ) == 0 )
					{
						if ( ( pAnimation->dwFlags & ANIMATION_FLAG_EASING_OUT ) == 0 )
						{
							GrannyEaseControlOut( pAnimation->pGrannyControl, fEaseOut );
							pAnimation->dwFlags |= ANIMATION_FLAG_EASING_OUT;
							pAnimation->dwFlags &= ~ANIMATION_FLAG_EASING_IN;
							pAppearance->dwFlags |= APPEARANCE_FLAG_UPDATE_WEIGHTS;
						}
					}
					else
					{
						if ( ( pAnimation->dwFlags & ANIMATION_FLAG_EASING_OUT ) == 0 )
						{
							float fEaseTime = sAnimationGetDuration( *pAnimation->pDefinition ) -
								GrannyGetControlRawLocalClock( pAnimation->pGrannyControl );
							if ( fEaseTime > fEaseOut )
							{
								fEaseTime = fEaseOut;
							}
	
							GrannyEaseControlOut( pAnimation->pGrannyControl, fEaseTime );
							pAnimation->dwFlags |= ANIMATION_FLAG_EASING_OUT;
							pAnimation->dwFlags &= ~ANIMATION_FLAG_EASING_IN;
							pAppearance->dwFlags |= APPEARANCE_FLAG_UPDATE_WEIGHTS;
						}
					}
				}
			}
			else if( AppIsHellgate() )
			{	
				if ( pAnimation->pDefinition == pAnimDef )
				{ // we have found a copy of the same animation already playing - either don't play the new one or create a new one on the same frame
					if ( dwFlags & PLAY_ANIMATION_FLAG_DEFAULT_ANIM )
					{
						pAnimation->dwPlayFlags |= PLAY_ANIMATION_FLAG_DEFAULT_ANIM;
					}
	
					if ( pAnimation->dwPlayFlags & PLAY_ANIMATION_FLAG_LOOPS ) 
					{
						if ( pAnimation->dwFlags & ANIMATION_FLAG_EASING_OUT )
						{
							pAnimation->dwFlags &= ~ANIMATION_FLAG_EASING_OUT;
#ifdef HAVOK_ENABLED
							if( AppIsHellgate() )
							{
								if ( pAnimation->pControl->ForceEaseBackIn() )
								{
									pAnimation->dwFlags |= ANIMATION_FLAG_EASING_IN;
								}
							}
#endif
							c_AppearanceUpdateAnimsWithVelocity( pAppearance );
							sAnimationSetSpeedAndDuration( pAppearance, pAnimation );
							pAppearance->dwFlags |= APPEARANCE_FLAG_UPDATE_WEIGHTS;

							if ( ( dwFlags & PLAY_ANIMATION_FLAG_ADJUSTING_CONTEXT ) == 0 )
							{
								sAppearanceUpdateContext( *pAppearance, pAnimation, pAnimDef );
							}
						} 
						return FALSE;
					} 
					else 
					{
						if (( pAnimation->dwFlags & ANIMATION_FLAG_EASING_OUT ) != 0 &&
							 pAnimation->fDuration - (pAnimation->fTimeSinceStart + pAnimation->pDefinition->fStartOffset) > 0.1f ) // are we almost done playing it?
						{
							 return FALSE;
						}
					}
				}
			}
		}

	}

	ANIMATION * pAnimation = NULL;

	UNITMODE_DATA * pModeData = (UNITMODE_DATA *) ExcelGetData( pGame, DATATABLE_UNITMODES, pAnimDef->nUnitMode );
	ASSERT_RETFALSE( pModeData );

	int nNewId = pAppearance->nNextAnimID++;
	pAnimation = HashAdd( pAppearance->pAnimations, nNewId );
	sAnimationInit( pAnimation );
	pAnimation->fDuration	= fDuration;
	pAnimation->pDefinition = pAnimDef;
	pAnimation->nPriority	= pModeData->nAnimationPriority + pAnimDef->nPriorityBoost;
	pAnimation->dwPlayFlags = dwFlags;
	pAnimation->fMaxMasterWeight = 1.0f;
	if( AppIsTugboat() && pGame && pAppearance->idUnit != INVALID_ID )
	{
		UNIT* pUnit = UnitGetById( pGame, pAppearance->idUnit );
		if( pUnit )
		{
			pAnimation->fMovementSpeed = UnitGetSpeed( pUnit );
		}
	}

	sAnimTrace( "PLAY_ANIM", *pAppearance, pAnimation );

	// -- Play Animation --
	if ( pAppearance->pGrannyInstance )
	{
		if ( !pAnimDef->pGrannyFile && pAnimDef->pszFile[0] != 0 )
		{
			pAppearance->dwFlags |= APPEARANCE_FLAG_CHECK_FOR_LOADED_ANIMS;
		}
		else if ( pAnimDef->pGrannyFile )
		{
			sAnimationAddControlAndUpdateDuration( pAppearance, pAnimation, fEaseOverride );
		}
	}
#ifdef HAVOK_ENABLED
	else if ( pAppearance->pAnimatedSkeleton )
	{
		if ( ! pAnimDef->pBinding && pAnimDef->pszFile[ 0 ] != 0 )
		{
			pAppearance->dwFlags |= APPEARANCE_FLAG_CHECK_FOR_LOADED_ANIMS;
		}
		else if ( pAnimDef->pBinding )
		{
			sAnimationAddControlAndUpdateDuration( pAppearance, pAnimation, fEaseOverride );
		}
	}
#endif

	if ( ! pAnimDef->tRagdollBlend.IsEmpty() )
	{
		c_AppearanceFreeze ( nAppearanceId );
	}

	if ( ( dwFlags & PLAY_ANIMATION_FLAG_ADJUSTING_CONTEXT ) == 0 )
	{
		sAppearanceUpdateContext( *pAppearance, pAnimation, pAnimDef );
	}

	pAppearance->dwFlags |= APPEARANCE_FLAG_UPDATE_WEIGHTS;

	sTrackNewAnimation( pAnimResult, pAppearance, pAnimation );
	
	if ( pfDurationUsed != NULL )
	{
		*pfDurationUsed = sAnimationGetDuration( *pAnimation );
		if ( AppIsTugboat() )
		{
			if ( pAnimation->pGrannyControl != NULL )
				*pfDurationUsed /= 
					c_sControlGetSpeed( pAnimation->pGrannyControl );
		}
		else if ( AppIsHellgate() )
		{
#ifdef HAVOK_ENABLED
			if ( pAnimation->pControl != NULL )
				*pfDurationUsed /= 
					c_sControlGetSpeed( pAnimation->pControl );
#endif
		}
	}

	return TRUE;
} // c_AppearancePlayAnimation()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL c_AppearanceUpdateAnimationGroup( UNIT *pUnit,
									   GAME *pGame,
									   int nAppearanceId,
									   int nAnimationGroupNew )
{
	APPEARANCE *pAppearance					= HashGet( sgpAppearances, nAppearanceId );
	if ( !pAppearance )
	{
		return ( FALSE );
	}



	BOOL bIsInTown = c_AnimationPlayTownAnims( pUnit ); // Elite players are going to look tougher in town

	if ( (( pAppearance->dwFlags & APPEARANCE_FLAG_IS_IN_TOWN ) != 0) == bIsInTown &&
		pAppearance->nAnimationGroup == nAnimationGroupNew )
		return FALSE;
	pAppearance->nAnimationGroup = nAnimationGroupNew;
	if ( bIsInTown )
		pAppearance->dwFlags |= APPEARANCE_FLAG_IS_IN_TOWN;
	else
		pAppearance->dwFlags &= ~APPEARANCE_FLAG_IS_IN_TOWN;

	APPEARANCE_DEFINITION *pAppearanceDef	= sAppearanceGetDefinition( *pAppearance );
	if ( !pAppearanceDef )
	{
		return ( FALSE );
	}

	ANIMATION_GROUP_DATA *pAnimGroup		= ( ANIMATION_GROUP_DATA * )ExcelGetData( pGame, DATATABLE_ANIMATION_GROUP,
																					  nAnimationGroupNew );

	if ( AppIsHellgate() )
	{
		ANIMATION_STANCE_DATA *pStanceData = ( ANIMATION_STANCE_DATA * )ExcelGetData( pGame,
																					   DATATABLE_ANIMATION_STANCE,
																					   pAppearance->nStanceCurrent );
		if (!pStanceData->bDontChangeFrom)
		{
			ASSERT_RETVAL( pAnimGroup != NULL, FALSE );
			int nDesiredStance = bIsInTown ? pAnimGroup->nDefaultStanceInTown : pAnimGroup->nDefaultStance;
			pAppearance->nStanceCurrent = nDesiredStance;
		}
	}

	struct REPLAY_CHART
	{
		BOOL	bPlay;
		float	fLocalTimePercent;
		DWORD	dwPlayFlags;
	};

	REPLAY_CHART ptReplayChart[MAX_UNITMODES];
	for ( int i								= 0; i < MAX_UNITMODES; i++ )
	{
		ptReplayChart[i].bPlay = FALSE;
		ptReplayChart[i].fLocalTimePercent = 0.0f;
		ptReplayChart[i].dwPlayFlags = 0;
	}

	// make sure that we have a default animation 
	BOOL bHasDefault						= FALSE;

	// record what animations are being played and remove them 
	for ( ANIMATION * pAnimation = HashGetFirst( pAppearance->pAnimations ), *pAnimationNext = NULL;
		  pAnimation != NULL; pAnimation = pAnimationNext )
	{
		pAnimationNext = HashGetNext( pAppearance->pAnimations, pAnimation );

		if ( pAnimation->dwPlayFlags & PLAY_ANIMATION_FLAG_DEFAULT_ANIM )
		{
			bHasDefault = TRUE;
			if( AppIsTugboat() ) // I had cases where my default anim got removed! does this happen to
			{
				pAnimation = pAnimationNext;
				continue;
			}
		}

		UNITMODE eMode = ( UNITMODE )pAnimation->pDefinition->nUnitMode;
		if ( ( pAnimation->dwFlags & ANIMATION_FLAG_EASING_OUT ) == 0 )
		{
			ptReplayChart[eMode].bPlay = TRUE;


#ifdef HAVOK_ENABLED
			if( AppIsHellgate() )
			{
				float fLocalTime = pAnimation->pControl ? pAnimation->pControl->getLocalTime() : 0.0f;
				float fDuration = pAnimation->pDefinition ? sAnimationGetDuration( *pAnimation->pDefinition ) : 0.0f;
				ptReplayChart[ eMode ].fLocalTimePercent = fDuration ? fLocalTime / fDuration : 0.0f;
			}
#endif

			if( AppIsTugboat() )
			{
				float fLocalTime = 0;
				ptReplayChart[eMode].fLocalTimePercent = pAnimation->pDefinition ? fLocalTime /
					sAnimationGetDuration( *pAnimation->pDefinition ) : 0.0f;
			}


			ptReplayChart[eMode].dwPlayFlags = pAnimation->dwPlayFlags;
		}

		sAnimationRemove( pAppearance, pAnimation->nId );
	}

	// create a default if there wasn't one 
	if ( !bHasDefault )
	{
		DWORD dwPlayAnimFlags = PLAY_ANIMATION_FLAG_ONLY_ONCE | PLAY_ANIMATION_FLAG_LOOPS | PLAY_ANIMATION_FLAG_DEFAULT_ANIM;

		c_AnimationPlayByMode( pUnit, pGame, nAppearanceId, MODE_IDLE, nAnimationGroupNew, 0.0f, dwPlayAnimFlags );

	}

	// play all of the current animations with the new animation group 
	for ( int i	= 0; i < NUM_UNITMODES; i++ )
	{
		if ( !ptReplayChart[i].bPlay )
		{
			continue;
		}
		if( AppIsHellgate() )
		{
			float fDuration = i == MODE_DYING ? 5.0f : 0.0f;
			c_AnimationPlayByMode( pUnit, pGame, nAppearanceId, (UNITMODE)i, nAnimationGroupNew, fDuration, ptReplayChart[ i ].dwPlayFlags );
		}
		if( AppIsTugboat() )
		{
			c_AnimationPlayByMode( pUnit, pGame, nAppearanceId, ( UNITMODE )i, nAnimationGroupNew, 0.0f, ptReplayChart[i].dwPlayFlags );
		}
	}

	// change the local times to match what was there before 
	for ( ANIMATION * pAnimation = HashGetFirst( pAppearance->pAnimations ); pAnimation != NULL;
		  pAnimation = HashGetNext( pAppearance->pAnimations, pAnimation ) )
	{
#ifdef HAVOK_ENABLED
		if( AppIsHellgate() )
		{
			if ( ! pAnimation->pControl )
			{
				continue;
			}
			int nUnitMode = pAnimation->pDefinition->nUnitMode;
			ASSERT_RETFALSE( nUnitMode >= 0 && nUnitMode < NUM_UNITMODES );
			float fDuration = sAnimationGetDuration( *pAnimation->pDefinition );
			float fLocalTimePercent = PIN( ptReplayChart[ nUnitMode ].fLocalTimePercent, 0.0f, 1.0f );
			sSetControlLocalTime( pAnimation->pControl, fDuration * fLocalTimePercent, fDuration );
		}
#endif
		if( AppIsTugboat() )
		{
			if ( !pAnimation->pGrannyControl )
			{
				continue;
			}
	
			int nUnitMode				= pAnimation->pDefinition->nUnitMode;
			ASSERT_RETFALSE( nUnitMode >= 0 && nUnitMode < NUM_UNITMODES );
	
			float fDuration				= sAnimationGetDuration( *pAnimation->pDefinition );
			float fLocalTimePercent		= PIN( ptReplayChart[nUnitMode].fLocalTimePercent, 0.0f, 1.0f );
	
			GrannySetControlRawLocalClock( pAnimation->pGrannyControl, fDuration * fLocalTimePercent );
		}
	}

	return ( TRUE );
} // c_AppearanceUpdateAnimationGroup()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL c_AppearanceSetAnimationSpeed( int nAppearanceId,
									int nUnitMode,
									float fAnimSpeed )
{
	APPEARANCE *pAppearance		= HashGet( sgpAppearances, nAppearanceId );
	if ( !pAppearance )
	{
		return ( FALSE );
	}

	ANIMATION *pAnimation		= c_sAppearanceGetAnimationByMode( *pAppearance, nUnitMode, NULL );
	if ( pAnimation )
	{
#ifdef HAVOK_ENABLED		
		if( AppIsHellgate() )
		{
			c_sControlSetSpeed( pAnimation->pControl, fAnimSpeed, pAnimation, &pAppearance->tEventIds );
		}
#endif
		if( AppIsTugboat() )
		{
			if ( pAnimation->pGrannyControl )
			{
				c_sControlSetSpeed( pAnimation->pGrannyControl, fAnimSpeed, pAnimation, &pAppearance->tEventIds  );
			}
		}
		return ( TRUE );
	}

	return ( FALSE );
} // c_AppearanceSetAnimationSpeed()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL c_AppearanceSetAnimationPercent( int nAppearanceId,
									  int nUnitMode,
									  float fPercent )
{
	APPEARANCE *pAppearance		= HashGet( sgpAppearances, nAppearanceId );
	if ( !pAppearance )
	{
		return ( FALSE );
	}

	fPercent = PIN( fPercent, 0.0f, 1.0f );

	ANIMATION *pAnimation		= c_sAppearanceGetAnimationByMode( *pAppearance, nUnitMode, NULL );
#ifdef HAVOK_ENABLED		
	if( AppIsHellgate() )
	{
		if ( pAnimation && pAnimation->pControl && pAnimation->pDefinition )
		{
			float fDuration = sAnimationGetDuration( *pAnimation->pDefinition );
			sSetControlLocalTime( pAnimation->pControl, fDuration * fPercent, fDuration );
			return TRUE;
		}
	}
#endif
	if( AppIsTugboat() )
	{
		if ( pAnimation && pAnimation->pGrannyControl && pAnimation->pDefinition )
		{
			float fDuration = pAnimation->fDuration;
			GrannySetControlRawLocalClock( pAnimation->pGrannyControl, fPercent * fDuration );

			pAppearance->dwFlags &= ~APPEARANCE_FLAG_LAST_FRAME;
			pAppearance->fAnimSampleTimer = 1;
			c_AppearanceSystemAndHavokUpdate( 
				AppGetCltGame(),
				0.0001f );


			pAppearance->fAnimSampleTimer = 0;

			return ( TRUE );
	
	
		}
	}

	return ( FALSE );
} // c_AppearanceSetAnimationPercent()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL c_AppearanceEndAnimation( int nAppearanceId,
							   int nUnitMode )
{
	APPEARANCE *pAppearance			= HashGet( sgpAppearances, nAppearanceId );
	if ( !pAppearance )
	{
		return ( FALSE );
	}

	BOOL bAnimRemoved = FALSE;
#ifdef HAVOK_ENABLED
	BOOL bAnimEasedOut = FALSE;
#endif
	for ( ANIMATION * pAnimCurr = HashGetFirst( pAppearance->pAnimations ); pAnimCurr != NULL; )
	{
		ANIMATION *pAnimNext = HashGetNext( pAppearance->pAnimations, pAnimCurr );

		if ( ( pAnimCurr->pDefinition->nUnitMode == nUnitMode ) &&
			 ( pAnimCurr->dwPlayFlags & PLAY_ANIMATION_FLAG_DEFAULT_ANIM ) == 0 )
		{
			sAnimTrace( "END_ANIM", *pAppearance, pAnimCurr );
#ifdef HAVOK_ENABLED		
			if( AppIsHellgate() )
			{
				if ( pAnimCurr->pControl && ( pAnimCurr->dwFlags & ANIMATION_FLAG_IS_ENABLED ) != 0 &&
					 !pAnimCurr->pControl->IsEasedOut())
				{
					pAnimCurr->dwFlags |= ANIMATION_FLAG_EASING_OUT;
					sAnimTrace( "ANIM_EASE_OUT END ANIM", *pAppearance, pAnimCurr );
					pAnimCurr->pControl->easeOut( pAnimCurr->pDefinition->fEaseOut );
					c_sControlSetSpeed( pAnimCurr->pControl, 0.0f, pAnimCurr, &pAppearance->tEventIds );
					bAnimEasedOut = TRUE;
				} 
				else 
				{
					sAnimationRemove( pAppearance, pAnimCurr->nId );
				}
				bAnimRemoved = TRUE;
			}
#endif
			if( AppIsTugboat() )
			{
				float fEaseOut = kEaseOutConstant;
				if ( pAnimCurr->pDefinition->fEaseOut == -1 )
				{
					fEaseOut = 0;
				}
				else if ( pAnimCurr->pDefinition->fEaseOut != 0 )
				{
					fEaseOut = pAnimCurr->pDefinition->fEaseOut;
				}
	
				if ( pAnimCurr->pGrannyControl && GrannyGetControlEffectiveWeight( pAnimCurr->pGrannyControl ) > 0 )
				{
					// we now ONLY remove looping anims - we let blending take care of the rest 
					if ( GrannyGetControlLoopCount( pAnimCurr->pGrannyControl ) == 0 && !( pAnimCurr->dwFlags & ANIMATION_FLAG_EASING_OUT
																						   
																						   ) )
					{
						pAnimCurr->dwFlags &= ~ANIMATION_FLAG_EASING_IN;
						pAnimCurr->dwFlags |= ANIMATION_FLAG_EASING_OUT;
	
						float fEaseTime = sAnimationGetDuration( *pAnimCurr->pDefinition ) -
							GrannyGetControlRawLocalClock( pAnimCurr->pGrannyControl );
						if ( fEaseTime > fEaseOut || GrannyGetControlLoopCount( pAnimCurr->pGrannyControl ) == 0 )
						{
							fEaseTime = fEaseOut;
						}
	
						GrannyEaseControlOut( pAnimCurr->pGrannyControl, fEaseTime );   	
	
						bAnimRemoved = TRUE;
					}
					else if ( ( pAppearance->dwFlags & APPEARANCE_FLAG_FREEZE_ANIMATION ) == 0 &&
							  c_AnimationHasFreezeEvent( *pAnimCurr->pDefinition ) )
					{
						sAnimationRemove( pAppearance, pAnimCurr->nId );
						bAnimRemoved = TRUE;
					}
				}
				else
				{
					sAnimationRemove( pAppearance, pAnimCurr->nId );
					bAnimRemoved = TRUE;
				}
			}
		}

		pAnimCurr = pAnimNext;
	}

	if ( bAnimRemoved )
	{
		pAppearance->dwFlags |= APPEARANCE_FLAG_UPDATE_WEIGHTS;
	}


	// some modes reset mix times of all animations on the appearance when they are
	// set we do this with the jump landing animations, they reset the mix on the
	// appearance so the feet on the landing line up with the start of a new run cycle

	const UNITMODE_DATA *pModeData	= UnitModeGetData( nUnitMode );
	if ( pModeData->bResetMixableOnEnd )
	{
		c_AppearanceSyncMixableAnimations( nAppearanceId, 0.0f );
	}

	return ( TRUE );
} // c_AppearanceEndAnimation()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
void c_AppearanceFaceTarget( 
	int nAppearanceId,
	const VECTOR &vTarget,
	BOOL bForce )
{
	APPEARANCE *pAppearance = HashGet( sgpAppearances, nAppearanceId );
	if ( !pAppearance )
	{
		return;
	}

	if ( VectorIsZero( vTarget ) )
	{
		pAppearance->dwFlags |= APPEARANCE_FLAG_FADE_OUT_FACING;
		if ( bForce )
		{
			pAppearance->fHeadTurnGain = 0.0f;
			pAppearance->vFacingTarget = vTarget;
		}
	}
	else 
	{
		pAppearance->dwFlags &= ~APPEARANCE_FLAG_FADE_OUT_FACING;

		if ( VectorDistanceSquared( vTarget, pAppearance->vFacingTarget ) > 30.0f * 30.0f ||
			 VectorIsZero( pAppearance->vFacingTarget ) ||
			 pAppearance->fHeadTurnGain == 0.0f )
			bForce = TRUE;

		if ( bForce )
			pAppearance->vFacingTarget = vTarget;
		else
			pAppearance->vFacingTarget = VectorLerp( vTarget, pAppearance->vFacingTarget, 0.4f );
	}
} // c_AppearanceFaceTarget()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
void c_AppearanceUpdateAnimsWithVelocity( APPEARANCE* pAppearance )
{
#ifdef HAVOK_ENABLED		
	enum
	{
		MAX_ANIMATIONS_TO_MIX_BY_SPEED = 20,
	};
	ANIMATION * pnAnimationsToMixByVel[ MAX_ANIMATIONS_TO_MIX_BY_SPEED ];
	int nAnimationsToMixByVel = 0;

	float fScale = e_ModelGetScale( pAppearance->nId ).fX;
	if ( pAppearance->pfLengthenedBones )
	{
		fScale *= c_AppearanceGetHeightFactor( pAppearance->nId );
	}
	ASSERT_RETURN( fScale > 0.001f );

	// find all of the animations marked as Mix By Speed
	// also, clear the ANIMATION_FLAG_SPEED_CONTROLS_ANIMSPEED flag from all animations
	// if they don't Mix by Speed, then set the animation speed according the speed that the appearance is moving
	for ( ANIMATION* pAnimation = HashGetFirst( pAppearance->pAnimations );
		pAnimation != NULL; 
		pAnimation = HashGetNext( pAppearance->pAnimations, pAnimation ) )
	{
		pAnimation->dwFlags &= ~ANIMATION_FLAG_SPEED_CONTROLS_ANIMSPEED; // clear the flag on all animations before setting it below

		if ( pAppearance->fSpeed == 0.0f ) // we will come back to this function when we get a speed
			continue;

		if ( ! pAnimation->pDefinition || ! pAnimation->pControl )
			continue;

		if ( pAnimation->pDefinition->fVelocity == 0.0f &&
			( pAnimation->pDefinition->pBinding || pAnimation->pDefinition->pGrannyFile ) )
			continue;

		if ( ( pAnimation->pDefinition->dwFlags & ANIMATION_DEFINITION_FLAG_FORCE_ANIMATION_SPEED ) != 0 )
			continue;

		if ( ( pAnimation->dwFlags & ANIMATION_FLAG_EASING_OUT ) != 0 )
			continue;

		if ( ( pAnimation->pDefinition->dwFlags & ANIMATION_DEFINITION_FLAG_MIX_BY_SPEED ) != 0 &&
			nAnimationsToMixByVel < MAX_ANIMATIONS_TO_MIX_BY_SPEED )
		{// save off animations which mix by speed
			pnAnimationsToMixByVel[ nAnimationsToMixByVel++ ] = pAnimation;
		} 
		else if ( pAnimation->pDefinition->fVelocity != 0.0f )
		{// adjust other animations by the scale and speed of the appearance
			pAnimation->dwFlags |= ANIMATION_FLAG_SPEED_CONTROLS_ANIMSPEED;
			pAnimation->fAnimSpeed = pAppearance->fSpeed / ( pAnimation->pDefinition->fVelocity * fScale );
		}
	}

	// set anim speed of animations which mix by speed
	// (this is for appearances that have fast and slow versions of the run animation
	float fMixAnimSpeed = 1.0;
	if ( nAnimationsToMixByVel && pAppearance->fSpeed != 0.0f )
	{
		// find out the min and max velocities of our mix by speed animations
		float fMaxVel = pnAnimationsToMixByVel[ 0 ]->pDefinition->fVelocity;
		float fMinVel = pnAnimationsToMixByVel[ 0 ]->pDefinition->fVelocity;
		int nMaxVelUnitMode = pnAnimationsToMixByVel[ 0 ]->pDefinition->nUnitMode; // we have to check the mode so that we don't drop anims using a different mode
		int nMinVelUnitMode = nMaxVelUnitMode;
		BOOL bDifferentModes = FALSE;
		for ( int i = 1; i < nAnimationsToMixByVel; i++ )
		{
			float fVelocity = pnAnimationsToMixByVel[ i ]->pDefinition->fVelocity;
			int nUnitMode = pnAnimationsToMixByVel[ i ]->pDefinition->nUnitMode;

			if ( nUnitMode != nMinVelUnitMode || nUnitMode != nMaxVelUnitMode )
				bDifferentModes = TRUE;

			if ( fVelocity > fMaxVel)
			{
				fMaxVel = fVelocity;
				nMaxVelUnitMode = nUnitMode;
			}
			if ( fVelocity < fMinVel)
			{
				fMinVel = fVelocity;
				nMinVelUnitMode = nUnitMode;
			}
		}

		float fSpeedAdjusted = pAppearance->fSpeed / fScale;

		float fDelta = fMaxVel - fMinVel;

		BOOL bMixByAverage = bDifferentModes;		
		BOOL bMixBySpeed = !bDifferentModes && nAnimationsToMixByVel > 1 && fDelta > 1.1f && ( fSpeedAdjusted < fMaxVel && fSpeedAdjusted > fMinVel );		

		// set the animation speed for the mix by speed animations
		float fTotalSpeed = 0.0f;
		float fAnimsToMixByVelWithWeight = 0.0f;
		for ( int i = 0; i < nAnimationsToMixByVel; i++ )
		{
			ANIMATION * pAnimation = pnAnimationsToMixByVel[ i ];

			pAnimation->dwFlags |= ANIMATION_FLAG_SPEED_CONTROLS_ANIMSPEED;

			if ( bMixBySpeed )
			{
				pAnimation->fMaxMasterWeight = (fDelta - fabsf( fSpeedAdjusted - pAnimation->pDefinition->fVelocity )) / fDelta;
				pAnimation->fMaxMasterWeight = PIN(pAnimation->fMaxMasterWeight, 0.0f, 1.0f);

			} else {
				
				if ( nAnimationsToMixByVel == 1 || fDelta <= 0.1f )
				{
					pAnimation->fMaxMasterWeight = 1.0f;
				}
				// turn off anims that are too slow or too fast - we are moving outside of the min/max range
				else if ( pAnimation->pDefinition->fVelocity <= fMaxVel - (fDelta / 2.0f) && // allow for small error
					fSpeedAdjusted > fMaxVel - 0.1f && pAnimation->pDefinition->nUnitMode == nMaxVelUnitMode ) 
				{
					pAnimation->fMaxMasterWeight = 0.0f;
				}
				else if ( pAnimation->pDefinition->fVelocity >= fMinVel + (fDelta / 2.0f) && // allow for small error
						fSpeedAdjusted < fMinVel + 0.1f && pAnimation->pDefinition->nUnitMode == nMinVelUnitMode )
				{
					pAnimation->fMaxMasterWeight = 0.0f;
				}
				else
				{
					pAnimation->fMaxMasterWeight = 1.0f;
				}
			}

			if ( pAnimation->dwFlags & ANIMATION_FLAG_IS_ENABLED )
				pAnimation->pControl->setMasterWeight( pAnimation->fMaxMasterWeight );

			if ( pAnimation->fMaxMasterWeight )
			{
				fTotalSpeed += pAnimation->pDefinition->fVelocity;
				fAnimsToMixByVelWithWeight += 1.0f;
			}
		}

		float fAverageSpeed = fAnimsToMixByVelWithWeight ? fTotalSpeed / fAnimsToMixByVelWithWeight : fMinVel;

		// set the animation speed for the mix by speed animations
		for ( int i = 0; i < nAnimationsToMixByVel; i++ )
		{
			ANIMATION * pAnimation = pnAnimationsToMixByVel[ i ];

			if ( bMixBySpeed )
			{
				// this blending makes up for adjusting the speed of the animation played
				pAnimation->fAnimSpeed = 1.0f;
			} 
			else if ( bMixByAverage )
			{
				pAnimation->fAnimSpeed = fAverageSpeed ? fSpeedAdjusted / fAverageSpeed : 1.0f;
			}
			else
			{
				pAnimation->fAnimSpeed = pAnimation->pDefinition->fVelocity ? fSpeedAdjusted / pAnimation->pDefinition->fVelocity : 1.0f;
			}

			if ( pAnimation->fMaxMasterWeight > 0.1f &&
				pAnimation->fAnimSpeed != 0.0f )
				fMixAnimSpeed = pAnimation->fAnimSpeed;
		}

	}

	// for arms we want to have the animation speed match the speed of the legs
	// so go through the animations and share the animation speed
	if ( pAppearance->fSpeed != 0.0f )
	{
		for ( ANIMATION* pAnimation = HashGetFirst( pAppearance->pAnimations );
			pAnimation != NULL; 
			pAnimation = HashGetNext( pAppearance->pAnimations, pAnimation ) )
		{
			if ( ! pAnimation->pDefinition || ! pAnimation->pControl )
				continue;

			if ( pAnimation->pDefinition->fVelocity == 0.0f &&
				( pAnimation->pDefinition->pBinding || pAnimation->pDefinition->pGrannyFile ) )
				continue;

			if ( ( pAnimation->pDefinition->dwFlags & ANIMATION_DEFINITION_FLAG_MATCH_MIX_ANIMSPEED ) != 0 &&
				( pAnimation->pDefinition->dwFlags & ANIMATION_DEFINITION_FLAG_FORCE_ANIMATION_SPEED ) == 0 )
			{
				pAnimation->fAnimSpeed = fMixAnimSpeed;

				pAnimation->dwFlags |= ANIMATION_FLAG_SPEED_CONTROLS_ANIMSPEED;

				ASSERT_CONTINUE( pAnimation->fAnimSpeed != 0.0f );
				pAnimation->fDuration = sAnimationGetDuration( *pAnimation->pDefinition );
				pAnimation->fDuration /= pAnimation->fAnimSpeed;
				pAnimation->dwFlags |= ANIMATION_FLAG_DURATION_ADJUSTED;
			} 

			// update the duration and control speed on all of the animations regulated by speed
			if ( pAnimation->dwFlags & ANIMATION_FLAG_SPEED_CONTROLS_ANIMSPEED )
			{
				c_sControlSetSpeed( pAnimation->pControl, pAnimation->fAnimSpeed, pAnimation, &pAppearance->tEventIds );

				ASSERT_CONTINUE( pAnimation->fAnimSpeed != 0.0f );
				pAnimation->fDuration = sAnimationGetDuration( *pAnimation->pDefinition );
				pAnimation->fDuration /= pAnimation->fAnimSpeed;
				pAnimation->dwFlags |= ANIMATION_FLAG_DURATION_ADJUSTED;
			}
		}
	}

	pAppearance->dwFlags |= APPEARANCE_FLAG_UPDATE_WEIGHTS; // this helps the mix animation percents match up

#endif
} // c_AppearanceUpdateAnimsWithVelocity()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
void c_AppearanceUpdateSpeed( int nAppearanceId,
							  float fSpeed )
{
	APPEARANCE *pAppearance		= HashGet( sgpAppearances, nAppearanceId );
	if ( !pAppearance )
	{
		return;
	}

	if ( fSpeed == 0.0f )
	{
		return;
	}

	if ( fSpeed == pAppearance->fSpeed )
	{
		return;
	}

	if( AppIsHellgate() )
	{
		pAppearance->fSpeed = fSpeed;
	
		c_AppearanceUpdateAnimsWithVelocity( pAppearance );
	}
	if( AppIsTugboat() )
	{
		if ( pAppearance->fSpeed == 0.0f )
	
	
		{
			pAppearance->fSpeed = fSpeed;
			return;
		}
	
		float fProportion			= fSpeed / pAppearance->fSpeed;
		pAppearance->fSpeed = fSpeed;
		for ( ANIMATION * pAnimation = HashGetFirst( pAppearance->pAnimations ); pAnimation != NULL;
			  pAnimation = HashGetNext( pAppearance->pAnimations, pAnimation ) )
		{
			if ( !pAnimation->pDefinition->bIsMobile || !pAnimation->pDefinition->pGrannyAnimation )
			{
				continue;
			}
	
			pAnimation->fMovementSpeed = fSpeed;
	
			if ( !pAnimation->pGrannyControl )
			{
				continue;
			}
	
			float fPlaySpeed = 0.0f;
	
			fPlaySpeed = GrannyGetControlSpeed( pAnimation->pGrannyControl );
	
			if ( fPlaySpeed == 0.0f )
			{
				continue;
			}
			c_sControlSetSpeed( pAnimation->pGrannyControl, fPlaySpeed * fProportion, pAnimation, &pAppearance->tEventIds );
		}
	}
} // c_AppearanceUpdateSpeed()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
float c_AppearanceGetTurnSpeed( int nAppearanceId )
{
	APPEARANCE *pAppearance = HashGet( sgpAppearances, nAppearanceId );
	if ( !pAppearance )
	{
		return ( 0.0f );
	}

	for ( ANIMATION * pAnimation = HashGetFirst( pAppearance->pAnimations ); 
		  pAnimation != NULL;
		  pAnimation = HashGetNext( pAppearance->pAnimations, pAnimation ) )
	{
		if ( pAnimation->pDefinition->fTurnSpeed == 0.0f )
		{
			continue;
		}

		if ( pAnimation->dwFlags & ANIMATION_FLAG_EASING_OUT )
		{
			continue;
		}
		if( AppIsTugboat() )
		{
			return ( pAnimation->pDefinition->fTurnSpeed * GrannyGetControlSpeed( pAnimation->pGrannyControl ) );
		}
#ifdef HAVOK_ENABLED		
		else
		{
			return pAnimation->pDefinition->fTurnSpeed * pAnimation->pControl->getPlaybackSpeed();
		}
#endif
	}

	return ( 0.0f );
} // c_AppearanceGetTurnSpeed()


///////////////////////////////////////////////////////////////////////////////////////////////////////
//	Appearance - Bone Accessing 
//	this is the current transform of the bone in model space - assuming that we are not setting the world matrix 
///////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL c_AppearanceGetBoneMatrix( int nAppearanceId,
								MATRIX *pmMatrix,
								int nBoneNumber,
								BOOL bTrackedPosition /*=FALSE*/ )
{
	APPEARANCE *pAppearance = HashGet( sgpAppearances, nAppearanceId );
	if ( !pAppearance )
	{
		return ( FALSE );
	}

	if ( nBoneNumber == INVALID_ID )
	{
		return ( FALSE );
	}
#ifdef HAVOK_ENABLED		
	if( AppIsHellgate() )
	{
		if ( nBoneNumber >= pAppearance->pPoseInModel.getSize() )
		{
			MatrixIdentity( pmMatrix );
		}
		else
		{
			if ( ! bTrackedPosition )
				pAppearance->pPoseInModel[ nBoneNumber ].get4x4ColumnMajor( (float *) pmMatrix );
			else {
				int nTrackedBones = pAppearance->pBonesToTrack.Count();
				for ( int i = 0; i < nTrackedBones; i++ )
				{
					if ( pAppearance->pBonesToTrack[ i ].nBone == nBoneNumber )
					{
						pAppearance->pBonesToTrack[ i ].mBoneInModel.get4x4ColumnMajor( (float *) pmMatrix );
						return TRUE;
					}
				}
				return FALSE;
			}
		}
	}
#endif
	if( AppIsTugboat() )
	{
		if ( pAppearance->pGrannyInstance )
		{
	
			ASSERT( nBoneNumber >= 0 );
	
			granny_skeleton *pSkeleton	= GrannyGetSourceSkeleton( pAppearance->pGrannyInstance );
			ASSERT( nBoneNumber < pSkeleton->BoneCount );
	
			granny_real32 *pBoneMatrix	= GrannyGetWorldPose4x4( pAppearance->pWorldPose, nBoneNumber );
			;
	
			memcpy( pmMatrix, pBoneMatrix, sizeof( MATRIX ) );
	
			return ( TRUE );
		}
		else
		{
			ASSERT( nBoneNumber >= 0 );
			if ( nBoneNumber >= ( int )pAppearance->pGrannyPoseInModel.Count() )
			{
				MatrixIdentity( pmMatrix );
			}
			else
			{
				*pmMatrix = pAppearance->pGrannyPoseInModel[nBoneNumber];
			}

		}
	}
	return ( TRUE );
} // c_AppearanceGetBoneMatrix()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
float *c_AppearanceGetBones( int nAppearanceId, int nMeshIndex, int nLOD, BOOL bGetPrev /*= FALSE*/ )	// CHB 2006.11.30
{
#if ! ISVERSION(SERVER_VERSION)
	ASSERT_RETNULL((0 <= nLOD) && (nLOD < LOD_COUNT));

	APPEARANCE * pAppearance = HashGet( sgpAppearances, nAppearanceId );
//	if ( !pAppearance || !pAppearance->pMeshes || !pAppearance->nMeshCount )
	if ( !pAppearance )
	{
		return NULL;
	}

	int nMeshCount = e_AnimatedModelDefinitionGetMeshCount( pAppearance->nModelDefinitionId, nLOD );
	if ( nMeshCount <= 0 )
		return NULL;

	ASSERT_RETNULL( nMeshIndex >= 0 && nMeshIndex < nMeshCount );

	// CHB 2006.10.18 - Is this necessary?
	if ( !pAppearance->pMeshes_[nLOD] || !pAppearance->nMeshCount_[nLOD] )
	{
		return NULL;
	}

	if ( bGetPrev && e_IsDX10Enabled() )
#ifdef MOTION_BLUR
		return pAppearance->pMeshes_[nLOD][nMeshIndex].pfPrevBoneMatrices3x4;
#else
		return pAppearance->pMeshes_[nLOD][nMeshIndex].pfBoneMatrices3x4;
#endif
	else
		return pAppearance->pMeshes_[nLOD][nMeshIndex].pfBoneMatrices3x4;
#else
	return NULL;
#endif //  ! ISVERSION(SERVER_VERSION)
}


///////////////////////////////////////////////////////////////////////////////////////////////////////
// Appearance Definition
///////////////////////////////////////////////////////////////////////////////////////////////////////

const INVENTORY_VIEW_INFO * AppearanceDefinitionGetInventoryView( int nAppearanceDefId,
																  int nIndex )
{
	APPEARANCE_DEFINITION * pAppearanceDef = AppearanceDefinitionGet( nAppearanceDefId ); 
	if ( ! pAppearanceDef )
		return NULL;

	if ( ! IS_VALID_INDEX( nIndex, pAppearanceDef->nInventoryViews ) )
		return NULL;

	return &pAppearanceDef->pInventoryViews[ nIndex ];
} // AppearanceDefinitionGetInventoryView()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
void AppearanceDefinitionDestroyNonDefData( 
	APPEARANCE_DEFINITION *pDef,
	BOOL bFreePaths )
{
	if ( !TESTBIT(pDef->tHeader.dwFlags, DHF_LOADED_BIT) )
	{
		return;
	}

	ASSERT( pDef );
	c_AnimationsFree( pDef, bFreePaths );

	if ( pDef->pAnimGroups )
	{
		FREE( NULL, pDef->pAnimGroups );
		pDef->pAnimGroups = NULL;
	}

	for ( int i = 0; i < NUM_TEXTURE_TYPES; i++ )
	{
		// CHB 2006.11.28
		for (int j = 0; j < LOD_COUNT; ++j)
		{
			if ( pDef->pnTextureOverrides[i][j] != INVALID_ID )
			{
				e_TextureReleaseRef( pDef->pnTextureOverrides[i][j] );
			}
		}
	}
	if( AppIsTugboat() )
	{
		if( pDef->pGrannyFile )
		{
			GrannyFreeFile( pDef->pGrannyFile );
		}
		if( pDef->pGrannyInstance )
		{
			GrannyFreeModelInstance( pDef->pGrannyInstance );
		}
	}

#ifdef HAVOK_ENABLED
	if( AppIsHellgate() )
	{
		if ( pDef->pLoader )
		{
			delete pDef->pLoader;
			pDef->pLoader = NULL;
		}
		pDef->pMeshBinding = NULL;
	}
#endif
}



///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static PRESULT sSkeletonFileLoadedCallback( ASYNC_DATA &data )
{
	PAKFILE_LOAD_SPEC *spec						= ( PAKFILE_LOAD_SPEC * )data.m_UserData;
	ASSERT_RETINVALIDARG( spec );

	if ( data.m_IOSize == 0 )
	{
		return S_FALSE;
	}

	SKELETON_FILE_CALLBACK_DATA *pCallbackData	= ( SKELETON_FILE_CALLBACK_DATA * )spec->callbackData;
	ASSERT_RETFAIL( pCallbackData );

	void *pDef = CDefinitionContainer::GetContainerByType( DEFINITION_GROUP_APPEARANCE )->GetById( pCallbackData->nAppearanceDefinitionId, TRUE );
	APPEARANCE_DEFINITION *pAppearanceDef		= ( APPEARANCE_DEFINITION * )pDef;

	if ( !pAppearanceDef )
	{
		return S_FALSE;
	}

	if ( spec->buffer )
	{
#ifdef HAVOK_ENABLED
		if( AppIsHellgate() )
		{
			hkMemoryStreamReader tStreamReader( spec->buffer, data.m_IOSize, hkMemoryStreamReader::MEMORY_INPLACE );
	
	//		hkVersionUtil::updateToCurrentVersion( tStreamReader, hkVersionRegistry::getInstance() );
	
			hkRootLevelContainer * pContainer = pAppearanceDef->pLoader->load( &tStreamReader );
	
			ASSERT_RETFAIL( pContainer );
	
			hkAnimationContainer* pAnimationContainer = reinterpret_cast<hkAnimationContainer*>( pContainer->findObjectByType( hkAnimationContainerClass.getName() ));
			hkxScene * pScene = reinterpret_cast<hkxScene*>( pContainer->findObjectByType( hkxSceneClass.getName() ));
	
			ASSERT_RETFAIL( pAnimationContainer );
			ASSERTX_RETFAIL( pAnimationContainer->m_numSkins > 0, "No skin binding found in skeleton!" );
			pAppearanceDef->pMeshBinding = pAnimationContainer->m_skins[ 0 ];
	
			for ( int i = 0; i < pScene->m_numSkinBindings; i++ )
			{
				if ( pScene->m_skinBindings[ i ]->m_mesh == pAppearanceDef->pMeshBinding->m_mesh )
				{
					pScene->m_skinBindings[ i ]->m_initSkinTransform.get4x4ColumnMajor( pAppearanceDef->mInverseSkinTransform );
					MatrixInverse( &pAppearanceDef->mInverseSkinTransform, &pAppearanceDef->mInverseSkinTransform );
					break;
				}
			}
	
			ASSERT_RETFAIL( pAppearanceDef->pMeshBinding );
		}
#endif
		if( AppIsTugboat() )
		{
			granny_file_info *pFileInfo( NULL );
			if( AppIsHammer() )
			{
				granny_file *pLoadedFile		= GrannyReadEntireFileFromMemory( data.m_IOSize, spec->buffer );

				pAppearanceDef->pGrannyFile = pLoadedFile;
				ASSERT_RETFAIL( pLoadedFile );

				pFileInfo = GrannyGetFileInfo( pLoadedFile );
							
			}
			else
			{
				//TRAVIS - I had to remove this. It was breaking the playing of animations in Hammer.
				if( !pAppearanceDef->pGrannyFile )
				{
					granny_file *pLoadedFile		= GrannyReadEntireFileFromMemory( data.m_IOSize, spec->buffer );
			
					pAppearanceDef->pGrannyFile = pLoadedFile;
					ASSERT_RETFAIL( pLoadedFile );
				}
		
				pFileInfo = GrannyGetFileInfo( pAppearanceDef->pGrannyFile );				
			}
			ASSERT( pFileInfo );
	
			// -- Coordinate System -- ;
			// Tell Granny to transform the file into our coordinate system
		
			e_GrannyConvertCoordinateSystem( pFileInfo );
	
			if ( pFileInfo->ModelCount > 0 )
			{
				pAppearanceDef->pGrannyModel = pFileInfo->Models[0];
	
			}
		}
	}

	SETBIT(pAppearanceDef->tHeader.dwFlags, DHF_LOADED_BIT);

	sAppearanceDefinitionLoadWithSkeleton( pAppearanceDef );

	return S_OK;
} // sSkeletonFileLoadedCallback()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static ANIM_GROUP *sAppearanceDefinitionGetAnimGroup( APPEARANCE_DEFINITION *pAppearanceDef,
													  int nAnimationGroup )
{
	if ( !pAppearanceDef )
	{
		return ( NULL );
	}

	for ( int i = 0; i < pAppearanceDef->nAnimGroupCount; i++ )
	{
		ANIM_GROUP *pAnimGroup = &pAppearanceDef->pAnimGroups[i];
		if ( pAnimGroup->nId == nAnimationGroup )
		{
			return ( pAnimGroup );
		}
	}

	return ( NULL );
} // sAppearanceDefinitionGetAnimGroup()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static void sGetDurationFromSubgroup( APPEARANCE_DEFINITION *pAppearanceDef,
									  ANIM_GROUP *pAnimGroup,
									  BOOL bPlayAnim,
									  int nOtherGroup,
									  UNITMODE eMode )
{
	// This should only be called from AppearanceDefPostXMLLoad, and so the Appearance
	// Def should always be valid.

	ASSERT( pAppearanceDef );
	if ( bPlayAnim && pAnimGroup->pfDurations[eMode] == 0.0f )
	{
		ANIM_GROUP *pAnimGroupOther = sAppearanceDefinitionGetAnimGroup( pAppearanceDef, nOtherGroup );
		if ( pAnimGroupOther && pAnimGroupOther->pfDurations[eMode] != 0.0f )
		{
			pAnimGroup->pfDurations[eMode] = pAnimGroupOther->pfDurations[eMode];
		}
	}
} // sGetDurationFromSubgroup()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// This is for when Hammer wants to load the definition, but NOT the skeleton.
///////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL nToolModeDontLoadSkeleton = FALSE;

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL AppearanceDefinitionPostXMLLoad( void *pDef,
									  BOOL bForceSync )
{
	APPEARANCE_DEFINITION *pAppearanceDef	= ( APPEARANCE_DEFINITION * )pDef;
	{
		for ( int i = 0; i < NUM_WEIGHT_GROUP_TYPES; i++ )
		{
			const char *pszName		= c_AppearanceDefinitionGetWeightGroupNameByType( ( WEIGHT_GROUP_TYPE )i );
			if ( !pszName || pszName[0] == 0 )
			{
				continue;
			}

			pAppearanceDef->pnWeightGroupTypes[i] = INVALID_ID;

			for ( int j	= 0; j < pAppearanceDef->nWeightGroupCount; j++ )
			{
				const char *pszGroupName = c_AppearanceDefinitionGetWeightGroupName( pAppearanceDef, j );
				if ( PStrCmp( pszGroupName, pszName ) == 0 )
				{
					pAppearanceDef->pnWeightGroupTypes[i] = j;
					break;
				}
			}
		}
	}

	pAppearanceDef->fHeadTurnTotalLimit = cos( DEG_TO_RAD( pAppearanceDef->nHeadTurnTotalLimitDegrees) );
	pAppearanceDef->fHeadTurnBoneLimit  = DEG_TO_RAD( pAppearanceDef->nHeadTurnBoneLimitDegrees);
	{

		// sometimes the path is fully qualified (2nd char is a ':'), or... ;
		// sometimes the file name starts with FILE_PATH_DATA and sometimes not...

		if ( pAppearanceDef->tHeader.pszName[1] != ':' && PStrICmp( pAppearanceDef->tHeader.pszName, FILE_PATH_DATA,
																	PStrLen( FILE_PATH_DATA ) ) )
		{
			char szTemp[MAX_PATH];
			PStrGetPath( szTemp, MAX_PATH, pAppearanceDef->tHeader.pszName );
			PStrPrintf( pAppearanceDef->pszFullPath, MAX_XML_STRING_LENGTH, "%s%s", FILE_PATH_DATA, szTemp );
		}
		else
		{
			PStrGetPath( pAppearanceDef->pszFullPath, MAX_XML_STRING_LENGTH, pAppearanceDef->tHeader.pszName );
		}

		PStrForceBackslash( pAppearanceDef->pszFullPath, DEFAULT_FILE_WITH_PATH_SIZE );
	}

	// find stuff in excel - we should bake this into the definition 
	for ( int i								= 0; i < pAppearanceDef->nAnimationCount; i++ )
	{
		ANIMATION_DEFINITION *pAnimDef = &pAppearanceDef->pAnimations[i];
		if ( pAnimDef->nGroup == INVALID_ID )
		{
			pAnimDef->nGroup = 0;
		}
	}

	// find out which animations blend 
	ASSERT_RETFALSE( ( MAX_UNITMODES / 32 ) * 32 == MAX_UNITMODES );
	ZeroMemory( pAppearanceDef->pdwModeBlendMasks, MAX_UNITMODES / 8 );
	for ( int i								= 0; i < pAppearanceDef->nAnimationCount; i++ )
	{
		ANIMATION_DEFINITION *pAnimDef = &pAppearanceDef->pAnimations[i];
		if ( pAnimDef->nUnitMode == INVALID_ID )
		{
			continue;
		}

		if ( pAnimDef->nBoneWeights != INVALID_ID || ( pAnimDef->dwFlags & ANIMATION_DEFINITION_FLAG_CAN_MIX ) != 0 )
		{
			SETBIT( pAppearanceDef->pdwModeBlendMasks, pAnimDef->nUnitMode, TRUE );
		}
	}
#ifdef _DEBUG
	// check to see that all of the files that should have blends do
	//for ( int i = 0; i < pAppearanceDef->nAnimationCount; i++ )
	//{
	//	ANIMATION_DEFINITION * pAnimDef = &pAppearanceDef->pAnimations[ i ];
	//	if ( pAnimDef->nUnitMode == INVALID_ID )
	//		continue;
	//	if ( TESTBIT( pAppearanceDef->pdwModeBlendMasks, pAnimDef->nUnitMode ) )
	//	{
	//		WARNX( pAnimDef->nBoneWeights != INVALID_ID, pAnimDef->pszFile );
	//	} else {
	//		WARNX( pAnimDef->nBoneWeights == INVALID_ID, pAnimDef->pszFile );
	//	}
	//}
#endif

	// fill in the animation groups array 
	pAppearanceDef->pAnimGroups = NULL;
	pAppearanceDef->nAnimGroupCount = 0;
	if ( pAppearanceDef->dwFlags & APPEARANCE_DEFINITION_FLAG_ADD_ALL_ANIMGROUPS )
	{
		pAppearanceDef->nAnimGroupCount = ExcelGetCount(EXCEL_CONTEXT(), DATATABLE_ANIMATION_GROUP);
		pAppearanceDef->pAnimGroups = ( ANIM_GROUP * )REALLOC( NULL, pAppearanceDef->pAnimGroups, sizeof
															   ( ANIM_GROUP ) *
															   pAppearanceDef->nAnimGroupCount );
		ZeroMemory( pAppearanceDef->pAnimGroups, pAppearanceDef->nAnimGroupCount * sizeof( ANIM_GROUP ) );
		for ( int i = 0; i < pAppearanceDef->nAnimGroupCount; i++ )
		{
			pAppearanceDef->pAnimGroups[i].nId = i;
		}
	}

	for ( int i								= 0; i < pAppearanceDef->nAnimationCount; i++ )
	{
		ANIMATION_DEFINITION *pAnimDef	= &pAppearanceDef->pAnimations[i];
		if ( pAnimDef->nUnitMode == INVALID_ID )
		{
			continue;
		}

		ANIM_GROUP *pAnimGroup			= NULL;
		for ( int j						= 0; j < pAppearanceDef->nAnimGroupCount; j++ )
		{
			ANIM_GROUP *pAnimGroupCurr = &pAppearanceDef->pAnimGroups[j];
			if ( pAnimGroupCurr->nId == pAnimDef->nGroup )
			{
				pAnimGroup = pAnimGroupCurr;
				break;
			}
		}

		if ( !pAnimGroup )
		{
			pAppearanceDef->nAnimGroupCount++;
			pAppearanceDef->pAnimGroups = ( ANIM_GROUP * )REALLOC( NULL, pAppearanceDef->pAnimGroups, sizeof
																   ( ANIM_GROUP ) *
																   pAppearanceDef->nAnimGroupCount );
			pAnimGroup = &pAppearanceDef->pAnimGroups[pAppearanceDef->nAnimGroupCount - 1];
			ZeroMemory( pAnimGroup, sizeof( ANIM_GROUP ) );
			pAnimGroup->nId = pAnimDef->nGroup;
		}

		ASSERT_RETFALSE( pAnimGroup );
		pAnimDef->pNextInGroup = pAnimGroup->pAnimDefs[pAnimDef->nUnitMode];
		pAnimGroup->pAnimDefs[pAnimDef->nUnitMode] = pAnimDef;
	}

	ASSERTX(NUM_UNITMODES == ExcelGetCount(EXCEL_CONTEXT(), DATATABLE_UNITMODES), "Code doesn't match data - unitmodes.xls");
	for ( int i								= 0; i < pAppearanceDef->nAnimGroupCount; i++ )
	{
		ANIM_GROUP *pAnimGroup					= &pAppearanceDef->pAnimGroups[i];
		ANIMATION_GROUP_DATA *pAnimGroupDef		= ( ANIMATION_GROUP_DATA * )ExcelGetData( NULL,
																						  DATATABLE_ANIMATION_GROUP,
																						  i );
		for ( int j								= 0; j < NUM_UNITMODES; j++ )
		{
			UNITMODE_DATA *pModeData = ( UNITMODE_DATA * )ExcelGetData( NULL, DATATABLE_UNITMODES, j );
			if ( ! pModeData )
				continue;

			if ( pAnimGroupDef->bPlayRightLeftAnims && pAnimGroupDef->bOnlyPlaySubgroups )
			{
				sGetDurationFromSubgroup( pAppearanceDef, pAnimGroup, pModeData->bPlayRightAnim,
										  pAnimGroupDef->nRightAnims, ( UNITMODE )j );
				sGetDurationFromSubgroup( pAppearanceDef, pAnimGroup, pModeData->bPlayLeftAnim,
										  pAnimGroupDef->nLeftAnims, ( UNITMODE )j );
			}
			else
			{
				ANIMATION_DEFINITION *pAnimDef = pAnimDef = pAnimGroup->
					pAnimDefs[j];
				pAnimGroup->pfDurations[j] = pAnimDef ? pAnimDef->fDuration : 0.0f;
			}

			if ( pAnimGroupDef->bPlayLegAnims )
			{
				sGetDurationFromSubgroup( pAppearanceDef, pAnimGroup, pModeData->bPlayLegAnim,
										  pAnimGroupDef->nLegAnims, ( UNITMODE )j );
			}
			if ( pAnimGroup->pfDurations[ j ] == 0.0f && pModeData->eBackupMode != INVALID_ID )
			{
				ASSERT( pModeData->eBackupMode < j ); // we could do this in a second pass if this is a problem...
				pAnimGroup->pfDurations[ j ] = pAnimGroup->pfDurations[ pModeData->eBackupMode ];
			}
		}
	}

	if ( AppGetType() != APP_TYPE_CLOSED_SERVER )
	{
		if ( c_GetToolMode() )
		{
			if(nToolModeDontLoadSkeleton)
				return TRUE;

			bForceSync = TRUE;
		}

		for ( int i = 0; i < pAppearanceDef->nInventoryViews; i++ )
		{
			// to force the load
			DefinitionGetIdByName( DEFINITION_GROUP_ENVIRONMENT, pAppearanceDef->pInventoryViews[ i ].pszEnvName, -1, bForceSync );
		}

		return ( AppearanceDefinitionLoadSkeleton( pAppearanceDef, bForceSync ) );
	}
	else
	{
		return ( TRUE );
	}
} // AppearanceDefinitionPostXMLLoad()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static void sAppearanceDefinitionLoadWithSkeleton( APPEARANCE_DEFINITION *pAppearanceDef )
{
	//GAME *pGame = AppGetCltGame();
	// perf debugging
	//#define MAX_PERF_TITLE (256)
	//char szPerfTitle[ MAX_PERF_TITLE ];
	//sprintf( szPerfTitle, "AppearanceDefinitionLoad(): %s", pAppearanceDef->tHeader.pszName );
	//TIMER_STARTEX( szPerfTitle, 50 )
	//	if ( IS_CLIENT( pGame ) )
	{
		if ( pAppearanceDef->nInitialized == 0 )
		{
			char szSkeletonFileWithPath[MAX_PATH];
			if ( pAppearanceDef->pszSkeleton[0] != 0 )
			{
				PStrPrintf( szSkeletonFileWithPath, MAX_PATH, "%s%s", pAppearanceDef->pszFullPath,
							pAppearanceDef->pszSkeleton );
			}
			else
			{
				szSkeletonFileWithPath[0] = 0;
			}

#ifdef HAVOK_ENABLED
			if( AppIsHellgate() )
			{
				hkSkeleton * pSkeleton = sAppearanceDefGetSkeletonMayhem( *pAppearanceDef );
	
				pAppearanceDef->pHavokShape = NULL;
				if ( pAppearanceDef->pszHavokShape[ 0 ] != 0 )
				{
					char szHavokShapeFileWithPath[ MAX_PATH ];
					PStrPrintf( szHavokShapeFileWithPath, MAX_PATH, "%s%s", pAppearanceDef->pszFullPath, pAppearanceDef->pszHavokShape );
					HavokLoadShapeForAppearance( pAppearanceDef->tHeader.nId, szHavokShapeFileWithPath );
				} 
	
				c_AnimationsInit( *pAppearanceDef );
	
				if ( pAppearanceDef->pszModel[ 0 ] != 0 )
				{
					char szFileWithPath[ MAX_PATH ];
					PStrPrintf( szFileWithPath, MAX_PATH, "%s%s", pAppearanceDef->pszFullPath, pAppearanceDef->pszModel );
	
					if ( pAppearanceDef->nWardrobeBaseId == INVALID_ID )
					{
						e_AnimatedModelDefinitionUpdate( pAppearanceDef->tHeader.nId, szSkeletonFileWithPath, szFileWithPath, FALSE );
	
						BOOL bModelShouldLoadTextures = (pAppearanceDef->dwFlags & APPEARANCE_DEFINITION_FLAG_USES_TEXTURE_OVERRIDES) == 0;
						if ( AppIsHammer() )
							bModelShouldLoadTextures = TRUE;
	
#if !ISVERSION(SERVER_VERSION)
						pAppearanceDef->nModelDefId = e_AnimatedModelDefinitionLoad( 
							pAppearanceDef->tHeader.nId, szFileWithPath, bModelShouldLoadTextures );
#endif //!ISVERSION(SERVER_VERSION)
					}
				}
	
				for ( int i = 0; i < pAppearanceDef->nInventoryViews; i++ )
				{
					if ( pAppearanceDef->pInventoryViews[ i ].pszEnvName[ 0 ] != 0 )
					{
						// this helps load the environments into the pak file and preloads them before they are used
						DefinitionGetIdByName( DEFINITION_GROUP_ENVIRONMENT, pAppearanceDef->pInventoryViews[ i ].pszEnvName );
					}
				}
	
				if ( pSkeleton )
				{
					struct BONE_TABLE {
						int nOffsetToBoneName;
						int nOffsetToBoneIndex;
					};

					const BONE_TABLE sgtBoneTable[] =
					{
						{ OFFSET( APPEARANCE_DEFINITION, pszAimBone ),			OFFSET( APPEARANCE_DEFINITION, nAimBone )			},
						{ OFFSET( APPEARANCE_DEFINITION, pszNeck ),				OFFSET( APPEARANCE_DEFINITION, nNeck )				},
						{ OFFSET( APPEARANCE_DEFINITION, pszSpineTop ),			OFFSET( APPEARANCE_DEFINITION, nSpineTop )			},
						{ OFFSET( APPEARANCE_DEFINITION, pszSpineBottom ),		OFFSET( APPEARANCE_DEFINITION, nSpineBottom )		},
						{ OFFSET( APPEARANCE_DEFINITION, pszLeftFootBone ),		OFFSET( APPEARANCE_DEFINITION, nLeftFootBone )		},
						{ OFFSET( APPEARANCE_DEFINITION, pszRightFootBone ),	OFFSET( APPEARANCE_DEFINITION, nRightFootBone )		},
						{ OFFSET( APPEARANCE_DEFINITION, pszRightWeaponBone[0]),OFFSET( APPEARANCE_DEFINITION, pnWeaponBones[0][0] )},
						{ OFFSET( APPEARANCE_DEFINITION, pszRightWeaponBone[1]),OFFSET( APPEARANCE_DEFINITION, pnWeaponBones[1][0] )},
						{ OFFSET( APPEARANCE_DEFINITION, pszLeftWeaponBone[0]),	OFFSET( APPEARANCE_DEFINITION, pnWeaponBones[0][1] )},
						{ OFFSET( APPEARANCE_DEFINITION, pszLeftWeaponBone[1]),	OFFSET( APPEARANCE_DEFINITION, pnWeaponBones[1][1] )},
					};
					int nBoneTableSize = ARRAYSIZE( sgtBoneTable );
					for ( int i = 0; i < nBoneTableSize; i++ )
					{
						char * pszBoneName = (char *)((BYTE *)pAppearanceDef + sgtBoneTable[ i ].nOffsetToBoneName );
						int * pnIndex = (int *)((BYTE *)pAppearanceDef + sgtBoneTable[ i ].nOffsetToBoneIndex);
						if ( pszBoneName[ 0 ] != 0 )
							*pnIndex = c_sAppearanceDefinitionGetBoneNumber( *pAppearanceDef, pszBoneName );
						else
							*pnIndex = INVALID_ID;
					}
				} 
	
				if ( pSkeleton )
				{
					pAppearanceDef->nBoneCount = pSkeleton->m_numBones;
				} else {
					pAppearanceDef->nBoneCount = e_AnimatedModelDefinitionGetBoneCount( pAppearanceDef->nModelDefId );
				}

				if ( pAppearanceDef->pszRagdoll[ 0 ] != 0 && pSkeleton )
				{
					RAGDOLL_DEFINITION * pRagdollDef = NULL;
					c_RagdollGetDefAndRagdoll(	pAppearanceDef->tHeader.nId, INVALID_ID, &pRagdollDef, NULL );
	
					if ( ! pRagdollDef )
					{
						char szFile[ DEFAULT_FILE_WITH_PATH_SIZE ];
						if ( AppTestDebugFlag( ADF_FILL_PAK_BIT ) && AppIsHellgate() )
						{ // load the hkx file for the other version
#ifdef _WIN64
							PStrPrintf( szFile, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s", pAppearanceDef->pszFullPath, pAppearanceDef->pszRagdoll );
#else
							char szBaseName[ MAX_FILE_NAME_LENGTH ];
							PStrRemoveExtension( szBaseName, MAX_FILE_NAME_LENGTH, pAppearanceDef->pszRagdoll);
							PStrPrintf( szFile, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s_64.%s", pAppearanceDef->pszFullPath,
								szBaseName, MAYHEM_FILE_EXTENSION );
#endif
							DECLARE_LOAD_SPEC( spec, szFile );
							spec.flags |= PAKFILE_LOAD_ADD_TO_PAK | PAKFILE_LOAD_FREE_BUFFER;
							spec.priority = ASYNC_PRIORITY_APPEARANCES;

							PakFileLoadNow( spec );
						}

#ifdef _WIN64
						const char* pszExtension = PStrGetExtension(pAppearanceDef->pszRagdoll);
						if( PStrICmp( pszExtension, "." RAGDOLL_FILE_EXTENSION ) == 0 )
						{
							char szBaseName[ MAX_FILE_NAME_LENGTH ];
							PStrRemoveExtension( szBaseName, MAX_FILE_NAME_LENGTH, pAppearanceDef->pszRagdoll);
							PStrPrintf( szFile, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s_64%s", pAppearanceDef->pszFullPath,
								szBaseName, pszExtension );
						}
						else
						{
							PStrPrintf( szFile, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s", pAppearanceDef->pszFullPath, pAppearanceDef->pszRagdoll );
						}
#else
						PStrPrintf( szFile, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s", pAppearanceDef->pszFullPath, pAppearanceDef->pszRagdoll );
#endif
						BOOL fRet = c_RagdollDefinitionNew( szFile, pAppearanceDef->tHeader.nId, pSkeleton );
						REF(fRet);
					}
					//ASSERT( fRet );
				}
			}
#endif
			if( AppIsTugboat() )
			{
				pAppearanceDef->pGrannyInstance = NULL;
				granny_skeleton *pGrannySkeleton		= NULL;
				if ( pAppearanceDef->pGrannyModel && !pAppearanceDef->pGrannyInstance )
	
				{
					pAppearanceDef->pGrannyInstance = GrannyInstantiateModel( pAppearanceDef->pGrannyModel );
					pGrannySkeleton = GrannyGetSourceSkeleton( pAppearanceDef->pGrannyInstance );
	
				}
	
				c_AnimationsInit( *pAppearanceDef );
	
				if ( pAppearanceDef->pszModel[0] != 0 )
				{
					char szFileWithPath[MAX_PATH];
					PStrPrintf( szFileWithPath, MAX_PATH, "%s%s", pAppearanceDef->pszFullPath, pAppearanceDef->pszModel );
	
					if ( pAppearanceDef->nWardrobeBaseId == INVALID_ID )
					{
						e_AnimatedModelDefinitionUpdate( pAppearanceDef->tHeader.nId, szSkeletonFileWithPath,
														 szFileWithPath, FALSE );
	
						BOOL bModelShouldLoadTextures = ( pAppearanceDef->dwFlags & APPEARANCE_DEFINITION_FLAG_USES_TEXTURE_OVERRIDES
														  
														  ) == 0;
						if ( AppIsHammer() )

						{
							bModelShouldLoadTextures = TRUE;
						}
	
#if !ISVERSION( SERVER_VERSION )
						pAppearanceDef->nModelDefId = e_AnimatedModelDefinitionLoad( 
							pAppearanceDef->tHeader.nId, szFileWithPath, bModelShouldLoadTextures );
#endif /*! ISVERSION(SERVER_VERSION) */
					}
				}
	
				if ( pGrannySkeleton )
				{
					pAppearanceDef->nBoneCount = pGrannySkeleton->BoneCount;
				}
				else
				{
					pAppearanceDef->nBoneCount = e_AnimatedModelDefinitionGetBoneCount( pAppearanceDef->nModelDefId );
				}
			}

			for ( int i	= 0; i < pAppearanceDef->nInventoryViews; i++ )
			{
				INVENTORY_VIEW_INFO *pInventoryView = &pAppearanceDef->
					pInventoryViews[i];
				if ( pInventoryView->pszBoneName[0] != 0 )
				{
					pInventoryView->nBone = c_sAppearanceDefinitionGetBoneNumber( *pAppearanceDef,
																				  pInventoryView->pszBoneName );
				}
			}

#if !ISVERSION(SERVER_VERSION)
			if ( AppIsHammer() || AppIsTugboat() )
			{
				for ( int i	= 0; i < NUM_TEXTURE_TYPES; i++ )
				{
					// CHB 2006.11.28
					for (int j = 0; j < LOD_COUNT; ++j)
					{
						e_TextureReleaseRef( pAppearanceDef->pnTextureOverrides[i][j] );
						pAppearanceDef->pnTextureOverrides[i][j] = INVALID_ID;
					}
					if ( pAppearanceDef->pszTextureOverrides[i][0] != 0 )
					{
						c_AppearanceLoadTextureOverrides(pAppearanceDef->pnTextureOverrides[i], pAppearanceDef->pszFullPath, pAppearanceDef->pszTextureOverrides[i], i, pAppearanceDef->nModelDefId);
					}
				}
			}
#endif //SERVER_VERSION

			pAppearanceDef->nInitialized = 1;
		}
	}

	CDefinitionContainer::GetContainerByType( DEFINITION_GROUP_APPEARANCE )->CallAllPostLoadCallbacks( pAppearanceDef->tHeader.nId );
} // sAppearanceDefinitionLoadWithSkeleton()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL AppearanceDefinitionLoadSkeleton( APPEARANCE_DEFINITION *pAppearanceDef,
									   BOOL bForceSync )
{
#ifdef HAVOK_ENABLED
	if ( AppIsHellgate() && !pAppearanceDef->pLoader )
	{
		pAppearanceDef->pLoader = new hkLoader;
	}
#endif

	MatrixIdentity( &pAppearanceDef->mInverseSkinTransform );

	char szSkeletonFileWithPath[MAX_PATH];
	if ( pAppearanceDef->pszSkeleton[0] != 0 )
	{

		if ( AppTestDebugFlag( ADF_FILL_PAK_BIT ) && AppIsHellgate() )
		{ // load the hkx file for the other version
#ifdef _WIN64
			PStrPrintf( szSkeletonFileWithPath, MAX_PATH, "%s%s", pAppearanceDef->pszFullPath,
				pAppearanceDef->pszSkeleton );
#else
			char	szBaseName[MAX_FILE_NAME_LENGTH];
			PStrRemoveExtension( szBaseName, MAX_FILE_NAME_LENGTH, pAppearanceDef->pszSkeleton );
			PStrPrintf( szSkeletonFileWithPath, MAX_PATH, "%s%s_64%s", pAppearanceDef->pszFullPath, szBaseName, PStrGetExtension( pAppearanceDef->pszSkeleton ) );
#endif
			DECLARE_LOAD_SPEC( spec, szSkeletonFileWithPath );
			spec.flags |= PAKFILE_LOAD_ADD_TO_PAK | PAKFILE_LOAD_FREE_BUFFER;
			spec.priority = ASYNC_PRIORITY_APPEARANCES;

			PakFileLoadNow( spec );
		}

#ifdef _WIN64
		if ( AppIsHellgate() )
		{
			char	szBaseName[MAX_FILE_NAME_LENGTH];
			PStrRemoveExtension( szBaseName, MAX_FILE_NAME_LENGTH, pAppearanceDef->pszSkeleton );
			PStrPrintf( szSkeletonFileWithPath, MAX_PATH, "%s%s_64%s", pAppearanceDef->pszFullPath, szBaseName, PStrGetExtension( pAppearanceDef->pszSkeleton ) );
		}
		else
			PStrPrintf( szSkeletonFileWithPath, MAX_PATH, "%s%s", pAppearanceDef->pszFullPath,
				pAppearanceDef->pszSkeleton );
#else
		PStrPrintf( szSkeletonFileWithPath, MAX_PATH, "%s%s", pAppearanceDef->pszFullPath,
					pAppearanceDef->pszSkeleton );
#endif
		DECLARE_LOAD_SPEC( spec, szSkeletonFileWithPath );
		spec.flags |= PAKFILE_LOAD_ADD_TO_PAK | PAKFILE_LOAD_FREE_BUFFER | PAKFILE_LOAD_FREE_CALLBACKDATA;

		SKELETON_FILE_CALLBACK_DATA *pCallbackData = ( SKELETON_FILE_CALLBACK_DATA * )MALLOCZ( NULL, sizeof(  SKELETON_FILE_CALLBACK_DATA ) );
		pCallbackData->nAppearanceDefinitionId = pAppearanceDef->tHeader.nId;

		spec.callbackData = pCallbackData;
		spec.fpLoadingThreadCallback = sSkeletonFileLoadedCallback;
		spec.priority = ASYNC_PRIORITY_APPEARANCES;

		if ( bForceSync )
		{
			PakFileLoadNow( spec );
			return ( TRUE );
		}
		else
		{
			ASSERT( !c_GetToolMode() );
			CLEARBIT(pAppearanceDef->tHeader.dwFlags, DHF_LOADED_BIT);		// I know ... ugly, but we still need to load more stuff 
			PakFileLoad( spec );
			return ( FALSE );
		}
	}
	else
	{
		sAppearanceDefinitionLoadWithSkeleton( pAppearanceDef );
		return ( TRUE );
	}
} // AppearanceDefinitionLoadSkeleton()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
int AppearanceDefinitionLoad( GAME *pGame,
							  const char *pszName,
							  const char *pszFolder )
{
	if ( pszName[0] == 0 )
	{
		return ( INVALID_ID );
	}

	char szFullName[MAX_PATH];
	BOOL bAddSuffix		= TRUE;
	int nNameLength		= PStrLen( pszName );
	int nSuffixLength	= PStrLen( APPEARANCE_SUFFIX );
	if ( nNameLength > nSuffixLength )
	{
		if ( PStrCmp( pszName + nNameLength - nSuffixLength, APPEARANCE_SUFFIX ) == 0 )
		{
			bAddSuffix = FALSE;
		}
	}

	if ( bAddSuffix )
	{
		PStrPrintf( szFullName, MAX_PATH, "%s%s%s", pszFolder, pszName, APPEARANCE_SUFFIX );
	}
	else
	{
		PStrPrintf( szFullName, MAX_PATH, "%s%s", pszFolder, pszName );
	}

	int nId				= DefinitionGetIdByFilename( DEFINITION_GROUP_APPEARANCE, szFullName );
	return ( nId );
} // AppearanceDefinitionLoad()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
void c_AppearanceDefinitionFlagSoundsForLoad( APPEARANCE_DEFINITION *pAppearanceDef, BOOL bLoadNow )
{
	ASSERT_RETURN( pAppearanceDef );
	c_AnimationsFlagSoundsForLoad( *pAppearanceDef, bLoadNow );
} // c_AppearanceDefinitionFlagSoundsForLoad()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
void c_AppearanceDefinitionApplyToModel( int nAppearanceDefId,
										 int nModelId )
{

	// This is only called from the AppearanceNew callback, which happens when the 
	// appearance definition has already loaded, so it had BETTER be a legitimate
	// appearance pointer here.

	APPEARANCE_DEFINITION *pAppearanceDef	= AppearanceDefinitionGet( nAppearanceDefId );
	ASSERT_RETURN( pAppearanceDef );

	APPEARANCE *pAppearance					= HashGet( sgpAppearances, nModelId );
	if ( !pAppearance )
	{
		return;
	}

	GAME *pGame								= AppGetCltGame();

	// this could reload a freed modeldef
	V( e_ModelSetDefinition( nModelId, sAppearanceDefinitionGetModelDefinition( pAppearanceDef ), LOD_ANY ) );

	if ( pAppearanceDef->nModelDefId != INVALID_ID )
	{
		for ( int i = 0; i < NUM_TEXTURE_TYPES; i++ )
		{
			// CHB 2006.11.28
			e_ModelSetTextureOverrideLOD( nModelId, ( TEXTURE_TYPE )i, pAppearanceDef->pnTextureOverrides[i] );
		}
	}

	if ( ( pAppearance->dwNewFlags & APPEARANCE_NEW_DONT_DO_INIT_ANIMATION ) == 0 )
	{
		c_AnimationDoAllEvents( pGame, nModelId, ANIM_INIT_OWNER_ID, pAppearanceDef->tInitAnimation, TRUE );
	}

	if ( pAppearanceDef->dwFlags & APPEARANCE_DEFINITION_FLAG_TWO_SIDED )
	{
		e_ModelSetFlagbit( nModelId, MODEL_FLAGBIT_TWO_SIDED, TRUE );
	}

	UNIT * pUnit = c_AppearanceGetUnit( pGame, nModelId );
	if ( pUnit )
	{
		if ( ( pAppearanceDef->dwFlags & APPEARANCE_DEFINITION_FLAG_MONITOR_STANCE ) != 0 &&
			!UnitHasTimedEvent( pUnit, c_sAppearanceMonitorContext ))
		{
			UnitRegisterEventTimed( pUnit, c_sAppearanceMonitorContext, EVENT_DATA( nModelId ), GAME_TICKS_PER_SECOND / 4 );
		}
	}


	if ( ( pAppearance->dwNewFlags & APPEARANCE_NEW_PLAY_IDLE ) != 0 )
	{
		UNITMODE nAnimMode = MODE_IDLE;
		BOOL bHasMode;
		AppearanceDefinitionGetAnimationDuration( NULL, nAppearanceDefId, 0, nAnimMode, bHasMode );
		if ( bHasMode )
		{
			c_AnimationPlayByMode( NULL, NULL, nModelId, nAnimMode, 0, 0.0f, PLAY_ANIMATION_FLAG_LOOPS | PLAY_ANIMATION_FLAG_RAND_START_TIME );
		}
	}
} // c_AppearanceDefinitionApplyToModel()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
void c_AppearanceForceInitAnimation( int nModelId )
{
	APPEARANCE *pAppearance					= HashGet( sgpAppearances, nModelId );
	if ( !pAppearance )
	{
		return;
	}

	APPEARANCE_DEFINITION *pAppearanceDef	= sAppearanceGetDefinition( *pAppearance );
	if ( !pAppearanceDef )
	{
		return;
	}

	c_AnimationDoAllEvents( AppGetCltGame(), nModelId, ANIM_INIT_OWNER_ID, pAppearanceDef->tInitAnimation, TRUE );
} // c_AppearanceForceInitAnimation()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static int sAppearanceDefinitionGetModelDefinition( APPEARANCE_DEFINITION *pAppearanceDef )
{
	if ( !pAppearanceDef )
	{
		return ( INVALID_ID );
	}

	if ( pAppearanceDef->nModelDefId != INVALID_ID && !e_ModelDefinitionExists( pAppearanceDef->nModelDefId ) )
	{
		pAppearanceDef->nModelDefId = INVALID_ID;

		// model definition was freed... must load anew
		if ( pAppearanceDef->pszModel[ 0 ] != 0 )
		{
			char szFileWithPath[ MAX_PATH ];
			PStrPrintf( szFileWithPath, MAX_PATH, "%s%s", pAppearanceDef->pszFullPath, pAppearanceDef->pszModel );

			if ( pAppearanceDef->nWardrobeBaseId == INVALID_ID )
			{
				//e_AnimatedModelDefinitionUpdate( pAppearanceDef->tHeader.nId, szSkeletonFileWithPath, szFileWithPath, FALSE );

				BOOL bModelShouldLoadTextures = (pAppearanceDef->dwFlags & APPEARANCE_DEFINITION_FLAG_USES_TEXTURE_OVERRIDES) == 0;
				if ( AppIsHammer() )
					bModelShouldLoadTextures = TRUE;

#if !ISVERSION(SERVER_VERSION)
				pAppearanceDef->nModelDefId = e_AnimatedModelDefinitionLoad( 
					pAppearanceDef->tHeader.nId, szFileWithPath, bModelShouldLoadTextures );
#endif //!ISVERSION(SERVER_VERSION)

				ASSERT_RETINVALID( pAppearanceDef->nModelDefId == INVALID_ID || e_ModelDefinitionExists( pAppearanceDef->nModelDefId ) );
			}
		}
	}

	return ( pAppearanceDef->nModelDefId );
} // c_AppearanceDefinitionGetModelDefinition()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
int c_AppearanceDefinitionGetModelDefinition( int nAppearanceDefId )
{
	APPEARANCE_DEFINITION *pAppearanceDef = AppearanceDefinitionGet( nAppearanceDefId );
	return sAppearanceDefinitionGetModelDefinition( pAppearanceDef );
} // c_AppearanceDefinitionGetModelDefinition()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
int c_AppearanceGetModelDefinition( APPEARANCE * pAppearance )
{
	if ( !pAppearance )
		return INVALID_ID;

	if ( pAppearance->nWardrobe != INVALID_ID )
		return c_WardrobeGetModelDefinition( pAppearance->nWardrobe );
	
	APPEARANCE_DEFINITION *pAppearanceDef = sAppearanceGetDefinition( *pAppearance );
	return sAppearanceDefinitionGetModelDefinition( pAppearanceDef );
} // c_AppearanceDefinitionGetModelDefinition()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
const char *c_AppearanceDefinitionGetInventoryEnvName( int nAppearanceDefId,
													   int nInventoryCamIndex )
{
	APPEARANCE_DEFINITION *pAppearanceDef = AppearanceDefinitionGet( nAppearanceDefId );
	if ( !pAppearanceDef )
	{
		return ( NULL );
	}

	ASSERT_RETNULL( IS_VALID_INDEX( nInventoryCamIndex, pAppearanceDef->nInventoryViews ) );
	return ( pAppearanceDef->pInventoryViews[nInventoryCamIndex].pszEnvName );
} // c_AppearanceDefinitionGetInventoryEnvName()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
MATRIX *c_AppearanceDefinitionGetInverseSkinTransform( int nAppearanceDefId )
{
	APPEARANCE_DEFINITION *pAppearanceDef = AppearanceDefinitionGet( nAppearanceDefId );
	if ( !pAppearanceDef )
	{
		return ( NULL );
	}

	return ( &pAppearanceDef->mInverseSkinTransform );
} // c_AppearanceDefinitionGetInverseSkinTransform()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
const char *c_AppearanceDefinitionGetWeightGroupName( APPEARANCE_DEFINITION *pAppearanceDef,
													  int nIndex )
{
	if ( !pAppearanceDef )
	{
		return ( NULL );
	}

	if ( nIndex < 0 || nIndex >= pAppearanceDef->nWeightGroupCount )
	{
		return ( NULL );
	}

	return ( &pAppearanceDef->pszWeightGroups[nIndex * MAX_XML_STRING_LENGTH] );
} // c_AppearanceDefinitionGetWeightGroupName()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
const char *c_AppearanceDefinitionGetWeightGroupNameByType( WEIGHT_GROUP_TYPE eType )
{
	if ( eType < 0 || eType > NUM_WEIGHT_GROUP_TYPES )
	{
		return ( NULL );
	}

	return ( sgpszWeightGroupNames[eType] );
} // c_AppearanceDefinitionGetWeightGroupNameByType()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL c_AppearanceDefinitionTestModeBlends( APPEARANCE_DEFINITION *pAppearanceDef,
										   int nUnitMode )
{
	ASSERT_RETFALSE( pAppearanceDef );
	return ( TESTBIT( pAppearanceDef->pdwModeBlendMasks, nUnitMode ) );
} // c_AppearanceDefinitionTestModeBlends()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
void c_AppearanceDefinitionGetMaxRopeBends( int nAppearanceDefId,
											float &fMaxBendXY,
											float &fMaxBendZ )
{
	fMaxBendXY = 0.0f;
	fMaxBendZ = 0.0f;

	APPEARANCE_DEFINITION *pAppearanceDef = AppearanceDefinitionGet( nAppearanceDefId );
	if ( !pAppearanceDef )
	{
		return;
	}

	fMaxBendXY = pAppearanceDef->fMaxRopeBendXY;
	fMaxBendZ = pAppearanceDef->fMaxRopeBendZ;
} // c_AppearanceDefinitionGetMaxRopeBends()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
ANIMATION_DEFINITION *AppearanceDefinitionGetAnimation( GAME *pGame,
														int nAppearanceDefId,
														int nAnimationGroup,
														UNITMODE eMode )
{
	APPEARANCE_DEFINITION *pAppearanceDef	= AppearanceDefinitionGet( nAppearanceDefId );
	ANIM_GROUP *pAnimGroup					= sAppearanceDefinitionGetAnimGroup( pAppearanceDef, nAnimationGroup );
	return ( pAnimGroup ? pAnimGroup->pAnimDefs[eMode] : NULL );
} // AppearanceDefinitionGetAnimation()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL AppearanceDefinitionGetWeaponOffset( int nAppearanceDefId,
										  int nWeaponIndex,
										  VECTOR &vOffset )
{
	vOffset.fX = vOffset.fY = vOffset.fZ = 0.0f;

	APPEARANCE_DEFINITION *pAppearanceDef = AppearanceDefinitionGet( nAppearanceDefId );
	if ( !pAppearanceDef )
	{
		return ( FALSE );
	}

	ASSERT_RETFALSE( nWeaponIndex >= 0 && nWeaponIndex < MAX_WEAPONS_PER_UNIT );

	if ( VectorIsZero( pAppearanceDef->pvMuzzleOffset[nWeaponIndex] ) )
	{
		return ( FALSE );
	}

	vOffset = pAppearanceDef->pvMuzzleOffset[nWeaponIndex];
	return ( TRUE );
} // AppearanceDefinitionGetWeaponOffset()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
float AppearanceDefinitionGetAnimationDuration( GAME *pGame,
												int nAppearanceDefId,
												int nAnimationGroup,
												UNITMODE eMode,
												BOOL &bHasMode,
												BOOL bIgnoreBackups /*=FALSE*/ )
{
	APPEARANCE_DEFINITION *pAppearanceDef	= AppearanceDefinitionGet( nAppearanceDefId );
	if ( !pAppearanceDef )
	{
		bHasMode = TRUE;
		return ( 1.0f );
	}

	const UNITMODE_DATA * pModeData = (UNITMODE_DATA *) ExcelGetData( pGame, DATATABLE_UNITMODES, eMode );
	if ( !pModeData )
	{
		bHasMode = FALSE;
		return 1.0f;
	}

	if ( pModeData->nGroup == MODEGROUP_EMOTE )
	{
		if ( (AppIsHellgate() && !pModeData->bEmoteAllowedHellgate) ||
			 (AppIsTugboat() && !pModeData->bEmoteAllowedMythos) )
		{
			bHasMode = FALSE;
			return 1.0f;
		}
	}

	if ( pModeData->bPlayJustDefault ||
		 AppIsTugboat() )
		nAnimationGroup = 0;

	ANIM_GROUP *pAnimGroup					= sAppearanceDefinitionGetAnimGroup( pAppearanceDef, nAnimationGroup );

	// fall back on another anim group if necessary 
	if ( !pAnimGroup || pAnimGroup->pfDurations[eMode] == 0.0f )
	{
		for ( int i = 0; i < pAppearanceDef->nAnimGroupCount; i++ )
		{
			if ( pAppearanceDef->pAnimGroups[i].pfDurations[eMode] != 0.0f )
			{
				pAnimGroup = &pAppearanceDef->pAnimGroups[i];
				break;
			}
		}
	}

	bHasMode = pAnimGroup && ( pAnimGroup->pfDurations[eMode] != 0.0f );

	if( bIgnoreBackups )
	{
		if( pAnimGroup && !pAnimGroup->pAnimDefs[eMode] )
		{
			bHasMode = FALSE;
		}
	}
	if( AppIsTugboat() )
	{
		if( pAnimGroup && 
			pAnimGroup->pAnimDefs[eMode] &&
			pAnimGroup->pAnimDefs[eMode]->fVelocity != 0 )
			return pAnimGroup->pfDurations[eMode] / pAnimGroup->pAnimDefs[eMode]->fVelocity;

		return ( pAnimGroup ? pAnimGroup->pfDurations[eMode] : 1.0f );
	}
	return ( pAnimGroup ? pAnimGroup->pfDurations[eMode] : 1.0f );
} // AppearanceDefinitionGetAnimationDuration()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
float AppearanceDefinitionGetContactPoint( int nAppearanceDef,
										   int nAnimationGroup,
										   UNITMODE eMode,
										   float fPointValue )
{
	APPEARANCE_DEFINITION *pAppearanceDef	= AppearanceDefinitionGet( nAppearanceDef );

	const ANIM_GROUP *pAnimGroup			= sAppearanceDefinitionGetAnimGroup( pAppearanceDef, nAnimationGroup );
	if ( !pAnimGroup )
	{
		return ( fPointValue );
	}

	const ANIMATION_DEFINITION *pAnimDef	= pAnimGroup->pAnimDefs[eMode];
	if ( !pAnimDef )
	{
		//return ( fPointValue );
		float fTime = 0.0f;
		ANIMATION_GROUP_DATA * pGroupData = (ANIMATION_GROUP_DATA *) ExcelGetData( NULL, DATATABLE_ANIMATION_GROUP, nAnimationGroup );
		if ( pGroupData->nRightAnims != INVALID_ID && pGroupData->nRightAnims != nAnimationGroup )
			fTime = AppearanceDefinitionGetContactPoint( nAppearanceDef, pGroupData->nRightAnims, eMode, fPointValue );
		if ( fTime == 0.0f )
		{
			if ( pGroupData->nLeftAnims != INVALID_ID && pGroupData->nLeftAnims != nAnimationGroup )
				fTime = AppearanceDefinitionGetContactPoint( nAppearanceDef, pGroupData->nLeftAnims, eMode, fPointValue );
		}
		return fTime;
	}

	for ( int i = 0; i < pAnimDef->nEventCount; i++ )
	{
		const ANIM_EVENT *pEvent = &pAnimDef->pEvents[i];
		if ( (fPointValue == 0.0f || pEvent->fParam == fPointValue) && 
			 pEvent->eType == ANIM_EVENT_CONTACT_POINT)
		{
			return ( pEvent->fTime );
		}
	}

	return ( fPointValue );
} // AppearanceDefinitionGetContactPoint()


///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
VECTOR AppearanceDefinitionGetBoundingBoxOffset( int nAppearanceDefId )
{
	APPEARANCE_DEFINITION *pAppearanceDef	= AppearanceDefinitionGet( nAppearanceDefId );

	// this could reload a freed modeldef
	int nModelDefId = sAppearanceDefinitionGetModelDefinition( pAppearanceDef );

	if ( !pAppearanceDef || !pAppearanceDef->nInitialized || nModelDefId == INVALID_ID )
	{
		return ( VECTOR( 0 ) );
	}

	const BOUNDING_BOX *pBoundingBox		= e_ModelDefinitionGetBoundingBox( nModelDefId, LOD_ANY );
	if ( pBoundingBox )
	{
		return ( BoundingBoxGetCenter( pBoundingBox ) );
	}

	return ( VECTOR( 0.0f ) );
} // AppearanceDefinitionGetBoundingBoxOffset()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
VECTOR AppearanceDefinitionGetBoundingBoxSize( int nAppearanceDefId )
{
	APPEARANCE_DEFINITION *pAppearanceDef	= AppearanceDefinitionGet( nAppearanceDefId );
	VECTOR vDefault		( 0.16f, 0.7f, 0.26f );

	// this could reload a freed modeldef
	int nModelDefId = sAppearanceDefinitionGetModelDefinition( pAppearanceDef );

	if ( !pAppearanceDef || !pAppearanceDef->nInitialized || nModelDefId == INVALID_ID )
	{
		return ( vDefault );
	}

	const BOUNDING_BOX *pBoundingBox		= e_ModelDefinitionGetBoundingBox( nModelDefId, LOD_ANY );
	if ( pBoundingBox )
	{
		return ( BoundingBoxGetSize( pBoundingBox ) );
	}

	return ( vDefault );
} // AppearanceDefinitionGetBoundingBoxSize()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
const struct BOUNDING_BOX *AppearanceDefinitionGetBoundingBox( int nAppearanceDefId )
{
	APPEARANCE_DEFINITION *pAppearanceDef = AppearanceDefinitionGet( nAppearanceDefId );

	// this could reload a freed modeldef
	int nModelDefId = sAppearanceDefinitionGetModelDefinition( pAppearanceDef );

	if ( !pAppearanceDef || !pAppearanceDef->nInitialized || nModelDefId == INVALID_ID )
	{
		return ( NULL );
	}

	return ( e_ModelDefinitionGetBoundingBox( nModelDefId, LOD_ANY ) );
} // AppearanceDefinitionGetBoundingBox()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
float AppearanceDefinitionGetSpinSpeed( int nAppearanceDefId )
{
	APPEARANCE_DEFINITION *pAppearanceDef = AppearanceDefinitionGet( nAppearanceDefId );
	if ( !pAppearanceDef )
	{
		return ( 0.5f );
	}

	return ( pAppearanceDef->fSpinSpeed );
} // AppearanceDefinitionGetSpinSpeed()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
int AppearanceDefinitionFilterAnimationGroup( GAME *pGame,
											  int nAppearanceDefId,
											  int nAnimationGroup )
{
	APPEARANCE_DEFINITION *pAppearanceDef	= AppearanceDefinitionGet( nAppearanceDefId );
	if ( !pAppearanceDef || !pAppearanceDef->nAnimGroupCount )
	{
		return ( DEFAULT_ANIMATION_GROUP );
	}

	ANIM_GROUP *pAnimGroup					= sAppearanceDefinitionGetAnimGroup( pAppearanceDef, nAnimationGroup );
	while ( !pAnimGroup )
	{
		ASSERT_RETINVALID( nAnimationGroup != 0 );

		const ANIMATION_GROUP_DATA *pAnimGroupData = ( const ANIMATION_GROUP_DATA * )ExcelGetData( pGame,
																								   DATATABLE_ANIMATION_GROUP, nAnimationGroup );
		ASSERT_RETINVALID( pAnimGroupData->nFallBack != INVALID_ID );
		nAnimationGroup = pAnimGroupData->nFallBack;
		pAnimGroup = sAppearanceDefinitionGetAnimGroup( pAppearanceDef, nAnimationGroup );
	}

	return ( nAnimationGroup );
} // AppearanceDefinitionFilterAnimationGroup()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
const char *c_AppearanceDefinitionGetBoneName( int nAppearanceDefId,
											   int nBoneId )
{
	APPEARANCE_DEFINITION *pAppearanceDef = AppearanceDefinitionGet( nAppearanceDefId );
	if ( !pAppearanceDef )
	{
		return ( NULL );
	}

	if ( nBoneId >= pAppearanceDef->nBoneCount )
	{
		return ( NULL );
	}
#ifdef HAVOK_ENABLED		
	if( AppIsHellgate() )
	{
		hkSkeleton * pSkeleton = sAppearanceDefGetSkeletonMayhem( *pAppearanceDef );
		if ( pSkeleton )
		{
			return pSkeleton->m_bones[ nBoneId ]->m_name;
		}
	}
#endif
	{
		ASSERT( pAppearanceDef->nModelDefId == INVALID_ID || e_ModelDefinitionExists( pAppearanceDef->nModelDefId ) );
		return ( e_AnimatedModelDefinitionGetBoneName( pAppearanceDef->nModelDefId, nBoneId ) );
	}
} // c_AppearanceDefinitionGetBoneName()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static int c_sAppearanceDefinitionGetBoneNumber( APPEARANCE_DEFINITION &tAppearanceDef,
												 const char *pszName )
{
#ifdef HAVOK_ENABLED		
	if( AppIsHellgate() )
	{
		hkSkeleton * pSkeleton = sAppearanceDefGetSkeletonMayhem( tAppearanceDef );
		if ( pSkeleton )
		{
			int nNameLength = PStrLen( pszName );
			for ( int i = 0; i < pSkeleton->m_numBones; i++ )
			{
				if ( PStrICmp( pSkeleton->m_bones[ i ]->m_name, pszName, nNameLength ) == 0 )
					return i;
			}
		}
		else
		{
			ASSERT( tAppearanceDef.nModelDefId == INVALID_ID || e_ModelDefinitionExists( tAppearanceDef.nModelDefId ) );
			return ( e_AnimatedModelDefinitionGetBoneNumber( tAppearanceDef.nModelDefId, pszName ) );
		}
	}
#endif
	if( AppIsTugboat() )
	{
		granny_skeleton *pGrannySkeleton = sAppearanceDefGetSkeletonGranny( tAppearanceDef );	
		if ( pGrannySkeleton )
		{
			int nNameLength		= PStrLen( pszName );
			for ( int i			= 0; i < pGrannySkeleton->BoneCount; i++ )
			{
				const char *pszBoneName = pGrannySkeleton->Bones[i].Name;
				if ( PStrICmp( pszBoneName, pszName, nNameLength ) == 0 )
				{
					return ( i );
	
				}
	
	
			}
	
	
		}	
		else
		{
			ASSERT( tAppearanceDef.nModelDefId == INVALID_ID || e_ModelDefinitionExists( tAppearanceDef.nModelDefId ) );
			return ( e_AnimatedModelDefinitionGetBoneNumber( tAppearanceDef.nModelDefId, pszName ) );
		}
	}

	return ( INVALID_ID );
} // c_sAppearanceDefinitionGetBoneNumber()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
int c_AppearanceDefinitionGetBoneNumber( int nAppearanceDefId,
										 const char *pszName )
{
	APPEARANCE_DEFINITION *pAppearanceDef = AppearanceDefinitionGet( nAppearanceDefId );
	if ( !pAppearanceDef )
	{
		return ( INVALID_ID );
	}

	return ( c_sAppearanceDefinitionGetBoneNumber( *pAppearanceDef, pszName ) );
} // c_AppearanceDefinitionGetBoneNumber()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
int c_AppearanceDefinitionGetBoneCount( int nAppearanceDefId )
{
	if ( nAppearanceDefId == INVALID_ID )
	{
		return ( 0 );
	}

	APPEARANCE_DEFINITION *pAppearanceDef = AppearanceDefinitionGet( nAppearanceDefId );
	return ( pAppearanceDef ? pAppearanceDef->nBoneCount : 0 );
} // c_AppearanceDefinitionGetBoneCount()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL c_AppearanceBonesLoaded( int nAppearanceId )
{

	if ( nAppearanceId == INVALID_ID )
	{
		return ( TRUE );
	}
	APPEARANCE *pAppearance					= HashGet( sgpAppearances, nAppearanceId );
	if ( !pAppearance )
	{
		return TRUE;
	}

	return !( pAppearance->dwFlags & sGetAppearanceNeedsBonesFlag( AppIsHellgate() ? e_ModelGetFirstLOD( nAppearanceId ) : LOD_HIGH ) );
} // c_AppearanceDefinitionBonesLoaded()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
void c_AnimatedModelDefinitionJustLoaded( int nAppearanceDefId,
										  int nModelDefinition )
{
	APPEARANCE_DEFINITION *pAppearanceDef = AppearanceDefinitionGet( nAppearanceDefId );
	ASSERT_RETURN( pAppearanceDef );		// this should be around if anim model is calling this 

#ifdef HAVOK_ENABLED
	if ( ! pAppearanceDef->pMeshBinding )
	{
		pAppearanceDef->nBoneCount = e_AnimatedModelDefinitionGetBoneCount( nModelDefinition );
	}
#else
	// TRAVIS: we need bone counts too!
		pAppearanceDef->nBoneCount = e_AnimatedModelDefinitionGetBoneCount( nModelDefinition );
#endif

} // c_AnimatedModelDefinitionJustLoaded()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
void AppearanceDefinitionUpdateBoneWeights( APPEARANCE_DEFINITION &tGlobalDefinition,
											APPEARANCE_DEFINITION &tDefinitionToUpdate )
{
	if ( !tGlobalDefinition.nInitialized )
	{
		AppearanceDefinitionLoadSkeleton( &tGlobalDefinition, TRUE );
	}
#ifdef HAVOK_ENABLED
	if( AppIsHellgate() )
	{
		hkSkeleton * pSkeleton = sAppearanceDefGetSkeletonMayhem( tGlobalDefinition );
		if ( pSkeleton )
		{
			int nBoneCountNew = pSkeleton->m_numBones;
			int * pnBoneMapping = (int *) MALLOCZ( NULL, nBoneCountNew * sizeof( int ) );
			BOOL bMappingChanged = nBoneCountNew != tDefinitionToUpdate.nBoneCount;
			for ( int i = 0; i < nBoneCountNew; i++ )
			{
				pnBoneMapping[ i ] = INVALID_ID;
	
				// do the bones already match up?
				if ( i < tDefinitionToUpdate.nBoneCount &&
					PStrCmp( pSkeleton->m_bones[ i ]->m_name, 
						& tDefinitionToUpdate.pszBoneNames[ i * MAX_XML_STRING_LENGTH ] ) == 0 )
				{
					pnBoneMapping[ i ] = i;
					continue;
				}
				bMappingChanged = TRUE;
	
				// do we already have this bone in another place?
				for ( int j = 0; j < tDefinitionToUpdate.nBoneCount; j++ )
				{
					if ( j == i ) // we already checked this one
						continue;
					if ( PStrCmp( pSkeleton->m_bones[ i ]->m_name, 
							& tDefinitionToUpdate.pszBoneNames[ j * MAX_XML_STRING_LENGTH ] ) == 0 )
					{// swap the bones
						pnBoneMapping[ i ] = j;
						break;
					}
				}
			}
	
			// reallocate structures
			if ( nBoneCountNew != tDefinitionToUpdate.nBoneCount )
			{
				if ( nBoneCountNew > tDefinitionToUpdate.nBoneCount )
				{
					tDefinitionToUpdate.pszBoneNames = (char *) 
						REALLOC( NULL, tDefinitionToUpdate.pszBoneNames, sizeof( char ) * MAX_XML_STRING_LENGTH * nBoneCountNew );
				}
			}
	
			// remap the bones
			if ( bMappingChanged )
			{
				float * pfWeightsNew = (float *) MALLOCZ( NULL, sizeof(float) * nBoneCountNew * tDefinitionToUpdate.nWeightGroupCount );
				for ( int i = 0; i < nBoneCountNew; i++ )
				{
					//if ( pnBoneMapping[ i ] == i )
					//	continue;
	
					PStrCopy( & tDefinitionToUpdate.pszBoneNames[ i * MAX_XML_STRING_LENGTH ], 
						pSkeleton->m_bones[ i ]->m_name, MAX_XML_STRING_LENGTH );
					if ( pnBoneMapping[ i ] != INVALID_ID )
					{
						for ( int j = 0; j < tDefinitionToUpdate.nWeightGroupCount; j++ )
						{
							pfWeightsNew[ j * nBoneCountNew + i ] = tDefinitionToUpdate.pfWeights[ j * tDefinitionToUpdate.nBoneCount + pnBoneMapping[ i ] ];
						}
					}
				}
				FREE(NULL, tDefinitionToUpdate.pfWeights);
				tDefinitionToUpdate.pfWeights = pfWeightsNew;
			}
			tDefinitionToUpdate.nBoneCount = nBoneCountNew;
			tDefinitionToUpdate.nWeightCount = tDefinitionToUpdate.nBoneCount * tDefinitionToUpdate.nWeightGroupCount;
			FREE( NULL, pnBoneMapping );
		}
	}
#endif
	if( AppIsTugboat() )
	{
		if ( tGlobalDefinition.pGrannyModel )
	
		{
			if( !tGlobalDefinition.pGrannyInstance )
			{
				tGlobalDefinition.pGrannyInstance	= GrannyInstantiateModel( tGlobalDefinition.pGrannyModel );
			}
	
			granny_skeleton *pGrannySkeleton		= GrannyGetSourceSkeleton( tGlobalDefinition.pGrannyInstance );
			int nBoneCountNew						= pGrannySkeleton->BoneCount;
	
			int *pnBoneMapping						= ( int * )MALLOCZ( NULL, nBoneCountNew * sizeof( int ) );
			BOOL bMappingChanged					= nBoneCountNew != tDefinitionToUpdate.nBoneCount;
			for ( int i								= 0; i < nBoneCountNew; i++ )
			{
				pnBoneMapping[i] = INVALID_ID;
	
				// do the bones already match up? 
				if ( i < tDefinitionToUpdate.nBoneCount &&
					 PStrCmp( pGrannySkeleton->Bones[i].Name, &tDefinitionToUpdate.pszBoneNames[i * MAX_XML_STRING_LENGTH
							  ] ) == 0 )
				{
					pnBoneMapping[i] = i;
					continue;
				}
	
				bMappingChanged = TRUE;
	
				// do we already have this bone in another place? 
				for ( int j = 0; j < tDefinitionToUpdate.nBoneCount; j++ )
				{
					if ( j == i )
					{
						// we already checked this one 
						continue;
					}
	
					if ( PStrCmp( pGrannySkeleton->Bones[i].Name, &tDefinitionToUpdate.pszBoneNames[j *
								  MAX_XML_STRING_LENGTH] ) == 0 )
					{
						// swap the bones 
						pnBoneMapping[i] = j;
						break;
					}
				}
			}
	
			// reallocate structures 
			if ( nBoneCountNew != tDefinitionToUpdate.nBoneCount )
			{
				if ( nBoneCountNew > tDefinitionToUpdate.nBoneCount )
				{
					tDefinitionToUpdate.pszBoneNames = ( char* )REALLOC( NULL,
																		 tDefinitionToUpdate.pszBoneNames, sizeof( char ) *
																		 MAX_XML_STRING_LENGTH *
																		 nBoneCountNew );
				}
			}
	
			// remap the bones 
			if ( bMappingChanged )
			{
				float *pfWeightsNew		= ( float * )MALLOCZ( NULL, sizeof( float ) * nBoneCountNew *
															  tDefinitionToUpdate.nWeightGroupCount );
				for ( int i				= 0; i < nBoneCountNew; i++ )
				{
					if ( pnBoneMapping[i] == i )
					{
						continue;
					}
	
					PStrCopy( &tDefinitionToUpdate.pszBoneNames[i * MAX_XML_STRING_LENGTH], pGrannySkeleton->Bones[i].Name,
							  MAX_PATH );
					if ( pnBoneMapping[i] != INVALID_ID )
					{
						for ( int j = 0; j < tDefinitionToUpdate.nWeightGroupCount; j++ )
						{
							pfWeightsNew[j * nBoneCountNew + i] = tDefinitionToUpdate.pfWeights[j *
								tDefinitionToUpdate.nBoneCount + pnBoneMapping[i]];
						}
					}
				}
	
				FREE( NULL, tDefinitionToUpdate.pfWeights );
				tDefinitionToUpdate.pfWeights = pfWeightsNew;
			}
	
			tDefinitionToUpdate.nBoneCount = nBoneCountNew;
			tDefinitionToUpdate.nWeightCount = tDefinitionToUpdate.nBoneCount * tDefinitionToUpdate.nWeightGroupCount;
			FREE( NULL, pnBoneMapping );
	
	
	
		}
	}
} // AppearanceDefinitionUpdateBoneWeights()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
float *c_AppearanceDefinitionGetBoneWeights( int nAppearanceDefId,
											 int nType )
{
	if ( nType < 0 || nType > NUM_WEIGHT_GROUP_TYPES )
	{
		return ( NULL );
	}

	APPEARANCE_DEFINITION *pAppearanceDef = AppearanceDefinitionGet( nAppearanceDefId );
	if ( !pAppearanceDef )
	{
		return ( NULL );
	}

	if ( pAppearanceDef->nWeightCount != pAppearanceDef->nBoneCount * pAppearanceDef->nWeightGroupCount )
	{
		AppearanceDefinitionUpdateBoneWeights( *pAppearanceDef, *pAppearanceDef );
	}

	if ( pAppearanceDef->pnWeightGroupTypes[nType] < 0 || 
		 pAppearanceDef->pnWeightGroupTypes[nType] >= pAppearanceDef->nWeightGroupCount )
	{
		return ( NULL );
	}

	ASSERT_RETNULL( pAppearanceDef->pfWeights != NULL || pAppearanceDef->nWeightCount == 0 );

	return
		( pAppearanceDef->pfWeights + pAppearanceDef->pnWeightGroupTypes[nType] * pAppearanceDef->nBoneCount );
} // c_AppearanceDefinitionGetBoneWeights()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
int c_AppearanceDefinitionGetNumBoneWeights( int nAppearanceDefId )
{
	APPEARANCE_DEFINITION *pAppearanceDef = AppearanceDefinitionGet( nAppearanceDefId );
	if ( !pAppearanceDef )
	{
		return ( 0 );
	}

	return ( pAppearanceDef->nWeightGroupCount );
} // c_AppearanceDefinitionGetNumBoneWeights()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
float *c_AppearanceDefinitionGetBoneWeightsByName( int nAppearanceDefId,
												   const char *pszName )
{
	APPEARANCE_DEFINITION *pAppearanceDef	= AppearanceDefinitionGet( nAppearanceDefId );
	if ( !pAppearanceDef )
	{
		return ( NULL );
	}

	if ( pAppearanceDef->nWeightCount != pAppearanceDef->nBoneCount * pAppearanceDef->nWeightGroupCount )
	{
		AppearanceDefinitionUpdateBoneWeights
			( *pAppearanceDef, *pAppearanceDef );
	}

	ASSERT_RETNULL( pAppearanceDef->pfWeights != NULL || pAppearanceDef->nWeightCount == 0 );

	int nIndex								= -1;
	for ( int i								= 0; i < pAppearanceDef->nWeightGroupCount; i++ )
	{
		if ( !PStrCmp( &pAppearanceDef->pszWeightGroups[i * MAX_XML_STRING_LENGTH], pszName, MAX_XML_STRING_LENGTH ) )
		{
			nIndex = i;
			break;
		}
	}

	if ( nIndex < 0 )
	{
		return ( NULL );
	}

	return ( pAppearanceDef->pfWeights + nIndex * pAppearanceDef->nBoneCount );
} // c_AppearanceDefinitionGetBoneWeightsByName()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// APPEARANCE SYSTEM -
///////////////////////////////////////////////////////////////////////////////////////////////////////
void c_AppearanceSystemInit( void )
{
	HashInit( sgpAppearances, NULL, 256 );

	// create the buffer for all of the bones 
	g_ptSharedLocalPose = GrannyNewLocalPose( MAX_BONES_PER_MODEL );

	// create a cache to handle the caching of animation data
#ifdef HAVOK_ENABLED
	if ( AppIsHellgate() &&
		 g_havokChunkCache == NULL )
	{

		//hkChunkPoolCinfo cpCinfo[4];

		////OLD CACHE SETTINGS
		//cpCinfo[0].m_buckets = 7;
		//cpCinfo[0].m_slots = 3;
		//cpCinfo[0].m_chunkSize = 16 * 1024;		// 336k   ( 21 * 16k -> 336k )

		//cpCinfo[1].m_buckets = 7;
		//cpCinfo[1].m_slots = 3;
		//cpCinfo[1].m_chunkSize = 32 * 1024;		// 672k   ( 21 * 32k -> 672k )

		//cpCinfo[2].m_buckets = 3;
		//cpCinfo[2].m_slots = 3;
		//cpCinfo[2].m_chunkSize = 64 * 1024;		// 576k   ( 9 * 64k -> 576k )

		//cpCinfo[3].m_buckets = 3;
		//cpCinfo[3].m_slots = 2;
		//cpCinfo[3].m_chunkSize = 560 * 1024;	// 3360k   ( 6 * 560k -> 3360k )

		////total size ~5MB
		////





		////NEW CACHE SETTINGS ( BUT THESE ARE OPTIMIZED FOR EXPORTED ANIMATIONS WHERE BLOCK COMPRESSION IS DISABLED )
		hkChunkPoolCinfo cpCinfo[3];		


		cpCinfo[0].m_buckets = 7;
		cpCinfo[0].m_slots = 3;
		cpCinfo[0].m_chunkSize = 64 * 1024;		
		//1344k

		cpCinfo[1].m_buckets = 3;
		cpCinfo[1].m_slots = 2;
		cpCinfo[1].m_chunkSize = 512 * 1024;
		//3072k

		cpCinfo[2].m_buckets = 3;
		cpCinfo[2].m_slots = 2;
		cpCinfo[2].m_chunkSize = 562 * 1024;	
		//3372k

		//total size  ~7.7MB



		hkDefaultChunkCacheCinfo cacheCinfo;
		cacheCinfo.m_numberOfCachePools = 3;
		cacheCinfo.m_cachePools.setSize( 3 );
		cacheCinfo.m_cachePools[0] = cpCinfo[0];
		cacheCinfo.m_cachePools[1] = cpCinfo[1];
		cacheCinfo.m_cachePools[2] = cpCinfo[2];
		//cacheCinfo.m_cachePools[3] = cpCinfo[3];

		g_havokChunkCache = new hkDefaultChunkCache( cacheCinfo );
	}
#endif

} // c_AppearanceSystemInit()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
void c_AppearanceSystemClose( void )
{
	GrannyFreeLocalPose( g_ptSharedLocalPose );

	for ( APPEARANCE * pAppearance = HashGetFirst( sgpAppearances ); 
		  pAppearance;
		  pAppearance = HashGetFirst( sgpAppearances ) )
	{
		c_sAppearanceDestroy( pAppearance );
		HashRemove( sgpAppearances, pAppearance->nId );
	}
	// CHB 2006.09.05 - Would cause an access violation during
	// abort of initialization.
#ifdef HAVOK_ENABLED
	if (g_havokChunkCache != 0) 
	{
		g_havokChunkCache->removeReference();
		g_havokChunkCache = NULL;
	}
#endif
	HashFree( sgpAppearances );
} // c_AppearanceSystemClose()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef HAVOK_ENABLED
static void sDebugDraw( const hkSkeleton * pSkeleton,
						const hkQsTransform* pose, 
						int color )
{
	const hkInt16* hierarchy = pSkeleton->m_parentIndices; 
	hkUint32 numBones = pSkeleton->m_numBones;

	hkLocalArray<hkQsTransform> poseInWorld( numBones );
	poseInWorld.setSize( numBones );

	hkSkeletonUtils::transformLocalPoseToWorldPose( numBones, hierarchy, hkQsTransform::getIdentity(), pose, poseInWorld.begin() );

	for( hkUint32 i = 0; i < numBones; i++ )
	{
		hkVector4 p1;
		hkVector4 p2;

		p1 = poseInWorld[i].getTranslation();


		hkReal boneLen = 1.0f;

		// Display connections between child and parent
		if ( hierarchy[i] == -1 )
		{
			p2.set(0,0,0);
		}
		else
		{
			p2 = poseInWorld[ hierarchy[i] ].getTranslation();
			HK_DISPLAY_LINE( p1, p2, color );

			hkVector4 bone; bone.setSub4(p1,p2);
			boneLen = bone.length3();
			boneLen = (boneLen > 10.0f) ? 10.0f : boneLen;
			boneLen = (boneLen < 0.1f) ? 0.1f : boneLen;
 		}
	}
} // sDebugDraw()
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
int c_AppearanceShrunkBoneAdd( int nAppearance,
							   int nBoneToAdd )
{
	if ( AppTestFlag( AF_CENSOR_NO_BONE_SHRINKING ) )
		return INVALID_ID;

	APPEARANCE *pAppearance					= HashGet( sgpAppearances, nAppearance );
	if ( !pAppearance )
	{
		return ( INVALID_ID );
	}

	APPEARANCE_DEFINITION *pAppearanceDef	= sAppearanceGetDefinition( *pAppearance );
	if ( !pAppearanceDef )
	{
		return ( INVALID_ID );
	}

	GAME *pGame								= AppGetCltGame();
	if ( nBoneToAdd == INVALID_ID )
	{
		float *pfWeights	= c_AppearanceDefinitionGetBoneWeights( pAppearanceDef->tHeader.nId,
																	WEIGHT_GROUP_BONES_TO_SHRINK );
		int nBoneCount		= c_AppearanceDefinitionGetBoneCount( pAppearanceDef->tHeader.nId );
		if ( pfWeights )
		{
			// use the appearance def to find a bone to shrink 
			int pnBonesAvail[MAX_BONES_PER_MODEL];
			int nBoneAvailCount		= 0;
			for ( int i				= 0; i < nBoneCount; i++ )
			{
				if ( pfWeights[i] <= 0.0f )
				{
					continue;
				}

				BOOL bAlreadyShrunk		= FALSE;
				for ( int j				= 0; j < pAppearance->nShrunkBoneCount; j++ )
				{
					if ( pAppearance->pnShrunkBones[j] == i )
					{
						bAlreadyShrunk = TRUE;
						break;
					}
				}

				if ( bAlreadyShrunk )
				{
					continue;
				}

				pnBonesAvail[nBoneAvailCount] = i;
				nBoneAvailCount++;
			}

			if ( nBoneAvailCount == 0 )
			{
				return ( INVALID_ID );
			}

			nBoneToAdd = pnBonesAvail[RandGetNum( pGame, nBoneAvailCount )];
		}
		else
		{
			// pick some random bone to shrink 
			if ( nBoneCount < MIN_BONES_FOR_SHRINKING )
			{
				return ( INVALID_ID );
			}

			nBoneToAdd = RandGetNum( pGame, MIN_BONES_FOR_SHRINKING + 1, nBoneCount - 1 );
		}
	}

	if ( nBoneToAdd == INVALID_ID )
	{
		return ( INVALID_ID );
	}

	pAppearance->pnShrunkBones = ( int* )REALLOC( NULL, pAppearance->pnShrunkBones, sizeof( int ) *
												  ( pAppearance->nShrunkBoneCount + 1 ) );
	pAppearance->pnShrunkBones[pAppearance->nShrunkBoneCount] = nBoneToAdd;
	pAppearance->nShrunkBoneCount++;
	return ( nBoneToAdd );
} // c_AppearanceShrunkBoneAdd()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
void c_AppearanceShrunkBoneClearAll( int nAppearance )
{
	APPEARANCE *pAppearance = HashGet( sgpAppearances, nAppearance );
	if ( !pAppearance )
	{
		return;
	}

	pAppearance->nShrunkBoneCount = 0;
} // c_AppearanceShrunkBoneClearAll()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static void c_sAppearanceShapeClear( APPEARANCE &tAppearance,
									 APPEARANCE_DEFINITION &tAppearanceDef )
{
	if ( tAppearance.pfFattenedBones )
	{
		ZeroMemory( tAppearance.pfFattenedBones, sizeof( float ) * tAppearanceDef.nBoneCount );
	}

	if ( tAppearance.pfLengthenedBones )
	{
		ZeroMemory( tAppearance.pfLengthenedBones, sizeof( float ) * tAppearanceDef.nBoneCount );
	}
} // c_sAppearanceShapeClear()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
void c_AppearanceShapeSet( int nAppearanceId,
						   const APPEARANCE_SHAPE &tAppearanceShape )
{
	APPEARANCE *pAppearance					= HashGet( sgpAppearances, nAppearanceId );
	if ( !pAppearance )
	{
		return;
	}

	pAppearance->tAppearanceShape = tAppearanceShape;

	APPEARANCE_DEFINITION *pAppearanceDef	= sAppearanceGetDefinition( *pAppearance );
	if ( !pAppearanceDef )
	{
		return;
	}

	c_sAppearanceShapeClear( *pAppearance, *pAppearanceDef );
	c_sAppearanceApplyShape( *pAppearance, *pAppearanceDef );
} // c_AppearanceShapeSet()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
const APPEARANCE_SHAPE * c_AppearanceShapeGet( int nAppearanceId )
{
	APPEARANCE * pAppearance = HashGet( sgpAppearances, nAppearanceId );
	if ( ! pAppearance )
	{
		return ( NULL );
	}

	return &pAppearance->tAppearanceShape;
} // c_AppearanceShapeGet()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
void AppearanceShapeRandomize( const APPEARANCE_SHAPE_CREATE &tAppearanceShapeCreate,
							   APPEARANCE_SHAPE &tAppearanceShape )
{
	RAND tRand;
	RandInit( tRand );

	BYTE bHeightMax		= max( tAppearanceShapeCreate.bHeightMax, tAppearanceShapeCreate.bHeightMin );
	BYTE bWeightMax		= max( tAppearanceShapeCreate.bWeightMax, tAppearanceShapeCreate.bWeightMin );
	BYTE bHeightMin		= min( tAppearanceShapeCreate.bHeightMax, tAppearanceShapeCreate.bHeightMin );
	BYTE bWeightMin		= min( tAppearanceShapeCreate.bWeightMax, tAppearanceShapeCreate.bWeightMin );
	BOOL bDone			= FALSE;
	BOUNDED_WHILE( !bDone, 1000 )
	{
		tAppearanceShape.bHeight = (BYTE) RandGetNum( tRand, bHeightMin, bHeightMax );
		tAppearanceShape.bWeight = (BYTE) RandGetNum( tRand, bWeightMin, bWeightMax );

		if ( tAppearanceShapeCreate.bUseLineBounds )
		{
			BYTE bDelta = tAppearanceShapeCreate.bHeightMax - tAppearanceShapeCreate.bHeightMin;
			ASSERTX_RETURN( bDelta != 0, "Appearance shape will do a divide by zero!" );
			
			int nWeightMin = tAppearanceShapeCreate.bWeightLineMin0 + 
				((tAppearanceShape.bHeight - tAppearanceShapeCreate.bHeightMin) * ( tAppearanceShapeCreate.bWeightLineMax0 - tAppearanceShapeCreate.bWeightLineMin0 ) 
				/ (bDelta) );

			int nWeightMax = tAppearanceShapeCreate.bWeightLineMin1 + 
				((tAppearanceShape.bHeight - tAppearanceShapeCreate.bHeightMin) * ( tAppearanceShapeCreate.bWeightLineMax1 - tAppearanceShapeCreate.bWeightLineMin1 ) 
				/ (bDelta) );

			if ( tAppearanceShape.bWeight >= (BYTE) nWeightMin &&
				 tAppearanceShape.bWeight <= (BYTE) nWeightMax )
				 bDone = TRUE;
		} 
		else 
		{
			bDone = TRUE;
		}
	} 
} // AppearanceShapeRandomize()+

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
void AppearanceShapeRandomizeByQuality( UNIT *pUnit,
										int nQuality,
										const APPEARANCE_SHAPE_CREATE &tDefaultShapeCreate )
{
	GAME *pGame							= UnitGetGame( pUnit );
	APPEARANCE_SHAPE *pAppearanceShape	= pUnit->pAppearanceShape;
	if ( UnitIsA( pUnit, UNITTYPE_MONSTER ) )
	{
		const MONSTER_QUALITY_DATA *pMonsterQualityData = MonsterQualityGetData( pGame, nQuality );
		ASSERTX_RETURN( pMonsterQualityData, "Expected monster quality data" );
		AppearanceShapeRandomize( pMonsterQualityData->tAppearanceShapeCreate, *pAppearanceShape );
	}
	else
	{
		AppearanceShapeRandomize( tDefaultShapeCreate, *pAppearanceShape );
	}
} // AppearanceShapeRandomizeByQuality()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static void c_sAppearanceApplyShape( APPEARANCE &tAppearance,
									 APPEARANCE_DEFINITION &tAppearanceDef,
									 WEIGHT_GROUP_TYPE eWeightGroupMin,
									 WEIGHT_GROUP_TYPE eWeightGroupMax,
									 BYTE bSize,
									 BOOL bLengthenBones )
{
	if ( tAppearanceDef.pnWeightGroupTypes[eWeightGroupMin] == INVALID_ID ||
		 tAppearanceDef.pnWeightGroupTypes[eWeightGroupMax] == INVALID_ID )
	{
		return;
	}

	WEIGHT_GROUP_TYPE eWeightGroupToUse		= bSize >= 127 ? eWeightGroupMax : eWeightGroupMin;

	float *pfBoneWeights = c_AppearanceDefinitionGetBoneWeights( tAppearanceDef.tHeader.nId,
																					eWeightGroupToUse );
	if ( !pfBoneWeights )
	{
		return;
	}

	float ** ppfBones = bLengthenBones ? &tAppearance.pfLengthenedBones : &tAppearance.pfFattenedBones;
	if ( !*ppfBones )
	{
		*ppfBones = ( float* )MALLOCZ( NULL, sizeof( float ) * tAppearanceDef.nBoneCount );
	}

	float fLerp;
	if ( bSize >= 127 )
	{
		fLerp = ( bSize - 127 ) / 128.0f;
	}
	else
	{
		fLerp = 1.0f - bSize / 128.0f;
	}

	for ( int i								= 0; i < tAppearanceDef.nBoneCount; i++ )
	{
		( *ppfBones )[i] += pfBoneWeights[i] * fLerp;
	}
} // c_sAppearanceApplyShape()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static void c_sAppearanceApplyShape( APPEARANCE &tAppearance,
									 APPEARANCE_DEFINITION &tAppearanceDef )
{
	c_sAppearanceApplyShape( tAppearance, tAppearanceDef, WEIGHT_GROUP_THIN, WEIGHT_GROUP_STRONG,
							 tAppearance.tAppearanceShape.bWeight, FALSE );
	c_sAppearanceApplyShape( tAppearance, tAppearanceDef, WEIGHT_GROUP_THIN_LENGTHEN, WEIGHT_GROUP_STRONG_LENGTHEN,
							 tAppearance.tAppearanceShape.bWeight, TRUE );
	c_sAppearanceApplyShape( tAppearance, tAppearanceDef, WEIGHT_GROUP_SHORT, WEIGHT_GROUP_TALL,
							 tAppearance.tAppearanceShape.bHeight, TRUE );
	c_sAppearanceApplyShape( tAppearance, tAppearanceDef, WEIGHT_GROUP_SHORT_SCALE, WEIGHT_GROUP_TALL_SCALE,
							 tAppearance.tAppearanceShape.bHeight, FALSE );
} // c_sAppearanceApplyShape()


///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
void c_AppearanceAdjustBonesFromBoneWeightGroup( int nAppearance,
												 int nWeightGroup,
												 BOOL bLengthen )
{
	APPEARANCE *pAppearance					= HashGet( sgpAppearances, nAppearance );
	if ( !pAppearance )
	{
		return;
	}

	APPEARANCE_DEFINITION *pAppearanceDef	= sAppearanceGetDefinition( *pAppearance );
	if ( nWeightGroup < 0 )
	{
		return;
	}

	if ( nWeightGroup >= pAppearanceDef->nWeightGroupCount )
	{
		return;
	}

	float ** ppfBonePtr = bLengthen ? (&pAppearance->pfLengthenedBones) : (&pAppearance->pfFattenedBones);
	if ( !*ppfBonePtr )
	{
		*ppfBonePtr = ( float* )MALLOC( NULL, sizeof( float ) * pAppearanceDef->nBoneCount );
	}

	float *pfWeights						= &pAppearanceDef->pfWeights[pAppearanceDef->nBoneCount * nWeightGroup];
	memcpy( *ppfBonePtr, pfWeights, sizeof( float ) * pAppearanceDef->nBoneCount );
} // c_AppearanceAdjustBonesFromBoneWeightGroup()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static void sAppearanceApplyFattenedBones( APPEARANCE *pAppearance,
										   APPEARANCE_DEFINITION * pAppearanceDef,
										   int nBoneCount )
{
#ifdef HAVOK_ENABLED		
	if( AppIsHellgate() )
	{
		if ( ! pAppearance->pfFattenedBones || ! pAppearance->pfLengthenedBones )
			return;
	
		for ( int i = 0; i < nBoneCount; i++ )
		{
			int nAxis = 0;
	
			hkVector4 vTranslation = pAppearance->pPoseInLocal[ i ].getTranslation();
	
			if ( pAppearanceDef->dwFlags & APPEARANCE_DEFINITION_FLAG_LENGTHEN_POS_X_AXIS )
			{
				nAxis = 0;
			} else {
				if ( fabsf( vTranslation.getSimdAt( 1 ) ) > fabsf( vTranslation.getSimdAt( 0 ) ) )
					nAxis = 1;
				if ( fabsf( vTranslation.getSimdAt( 2 ) ) > fabsf( vTranslation.getSimdAt( nAxis ) ) )
					nAxis = 2;
	
				// some bones don't translate and we fatten all of the axis on those
				if ( fabsf( vTranslation.getSimdAt( nAxis ) ) < 0.001f )
					nAxis = -1;
			}
	
			float fFatFactor = pAppearance->pfFattenedBones[ i ];
			fFatFactor += 1.0f;
	
			float fLengthenFactor = pAppearance->pfLengthenedBones[ i ];
			fLengthenFactor += 1.0f;
	
			hkVector4 vScaleMult( nAxis == 0 ? fLengthenFactor : fFatFactor,
								  nAxis == 1 ? fLengthenFactor : fFatFactor,
								  nAxis == 2 ? fLengthenFactor : fFatFactor, 1.0f );
			hkVector4 vScale = pAppearance->pPoseInModel[ i ].getScale();
			vScale.mul4( vScaleMult );
			pAppearance->pPoseInModel[ i ].setScale( vScale );
		}
	}
#endif
} // sAppearanceApplyFattenedBones()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static void sAppearanceApplyLengthenedBones( APPEARANCE *pAppearance,
											 int nBoneCount )
{
#ifdef HAVOK_ENABLED		
	if( AppIsHellgate() )
	{
		if ( ! pAppearance->pfLengthenedBones )
			return;
	
		if ( ! pAppearance->pAnimatedSkeleton )
			return;
	
		const hkSkeleton * pSkeleton = pAppearance->pAnimatedSkeleton->getSkeleton();
	
		if ( ! pSkeleton )
			return;
	
		for ( int i = 0; i < nBoneCount; i++ )
		{
			int nParent = pSkeleton->m_parentIndices[ i ];
			if ( nParent == INVALID_ID )
				nParent = i;
	
			int nAxis = 0;
			hkVector4 vTranslation = pAppearance->pPoseInLocal[ i ].getTranslation();
			if ( fabsf( vTranslation.getSimdAt( 1 ) ) > fabsf( vTranslation.getSimdAt( 0 ) ) )
				nAxis = 1;
			if ( fabsf( vTranslation.getSimdAt( 2 ) ) > fabsf( vTranslation.getSimdAt( nAxis ) ) )
				nAxis = 2;
	
			float fLengthFactor = pAppearance->pfLengthenedBones[ nParent ];
			fLengthFactor += 1.0f;
			if ( fLengthFactor == 1.0f )
				continue;
	
			hkVector4 vTranlationMult( fLengthFactor, fLengthFactor, fLengthFactor, 1.0f );
			//hkVector4 vTranlationMult( nAxis == 0 ? fLengthFactor : 0.0f,
			//					  nAxis == 1 ? fLengthFactor : 0.0f,
			//					  nAxis == 2 ? fLengthFactor : 0.0f, 1.0f );
			vTranslation.mul4( vTranlationMult );
			pAppearance->pPoseInLocal[ i ].setTranslation( vTranslation );
		}
	
		// adjust the scale of the weapons through their bones
		int pnWeaponBones[ MAX_WEAPONS_PER_UNIT ];
		if ( pAppearance->nWardrobe != INVALID_ID && 
			c_WardrobeGetWeaponBones( pAppearance->nId, pAppearance->nWardrobe, pnWeaponBones ) )
		{
			for ( int i = 0; i < MAX_WEAPONS_PER_UNIT; i++ )
			{
				if ( pnWeaponBones[ i ] == INVALID_ID )
					continue;
	
				int nWeaponBoneCurr = pnWeaponBones[ i ];
				ASSERT_CONTINUE( nWeaponBoneCurr >= 0 && nWeaponBoneCurr < nBoneCount );
	
				float fScaleChange = pAppearance->pfLengthenedBones[ nWeaponBoneCurr ] + 1.0f;
				if ( fScaleChange == 1.0f )
					continue;
	
				hkVector4 vScale = pAppearance->pPoseInLocal[ nWeaponBoneCurr ].getScale();
				vScale.mul4( fScaleChange );
				pAppearance->pPoseInLocal[ nWeaponBoneCurr ].setScale( vScale );
			}
		}
	}
#endif
} // sAppearanceApplyLengthenedBones()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static void sAppearanceApplyShrunkBones( APPEARANCE *pAppearance,
										 int nBoneCount )
{
#ifdef HAVOK_ENABLED		
	if( AppIsHellgate() )
	{
		ASSERT_RETURN( pAppearance->pnShrunkBones || ! pAppearance->nShrunkBoneCount );
		ASSERT_RETURN( nBoneCount < MAX_BONES_PER_MODEL );
		ASSERT_RETURN( pAppearance->pAnimatedSkeleton );
	
		BOOL pbBoneShouldShrink[ MAX_BONES_PER_MODEL ];
		ZeroMemory( pbBoneShouldShrink, nBoneCount * sizeof(BOOL) );
		BOOL pbBoneIsShrunk[ MAX_BONES_PER_MODEL ];
		ZeroMemory( pbBoneIsShrunk, nBoneCount * sizeof(BOOL) );
		for ( int i = 0; i < pAppearance->nShrunkBoneCount; i++ )
		{// mark shrunk parents and shrink them without adjusting the translation
			int nBone = pAppearance->pnShrunkBones[ i ];
			if ( nBone < 0 || nBone >= nBoneCount )
				continue;
	
			pbBoneShouldShrink[ nBone ] = TRUE;
			pbBoneIsShrunk[ nBone ] = TRUE;
			pAppearance->pPoseInLocal[ nBone ].setScale( hkVector4( 0.001f, 0.001f, 0.001f ) );
		}
	
		const hkSkeleton * pSkeleton = pAppearance->pAnimatedSkeleton->getSkeleton();
	
		for ( int i = 0; i < nBoneCount; i++ )
		{// mark shrunk children and shrink bones
			int nParent = pSkeleton->m_parentIndices[ i ];
			if ( nParent == INVALID_ID )
				continue;
			if ( ! pbBoneShouldShrink[ nParent ] )
				continue;
	
			pbBoneShouldShrink[ i ] = TRUE;
			if ( pbBoneIsShrunk[ i ] )
				continue;
	
			pbBoneIsShrunk[ i ] = TRUE;
		
			pAppearance->pPoseInLocal[ i ].setScale( hkVector4( 0.001f, 0.001f, 0.001f ) );
			pAppearance->pPoseInLocal[ i ].setTranslation( hkVector4( 0.00f, 0.00f, 0.00f ) );
		}
	}
#endif
} // sAppearanceApplyShrunkBones()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
void c_AppearanceGetDebugMetrics( APPEARANCE_METRICS &tMetrics )
{
	tMetrics = sgtAppearanceMetrics;
} // c_AppearanceGetDebugMetrics()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static float sAnimationGetPathTime( ANIMATION *pAnimation,
									float fTimeDelta )
{
	if ( pAnimation->fDuration == 0.0f )
	{
		return ( 0.0f );
	}

	return
		( pAnimation->fTimeSinceStart + pAnimation->pDefinition->fStartOffset + fTimeDelta ) / pAnimation->fDuration;
} // sAnimationGetPathTime()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef HAVOK_ENABLED
static void sVerifyPose( 
	const hkSkeleton * pSkeleton,
	hkPose & tPose )
{
	const hkArray<hkQsTransform>& tModelSpace = tPose.getPoseModelSpace();
	const hkArray<hkQsTransform>& tLocalSpace = tPose.getPoseLocalSpace();
	int nBones = tModelSpace.getSize();
	for ( int i = 0; i < nBones; i++ )
	{
		const char * pszBoneName = pSkeleton->m_bones[ i ] ? pSkeleton->m_bones[ i ]->m_name : "noname";
		REF(pszBoneName);	// CHB 2007.03.16 - Silence compiler warning.
		ASSERTX( tModelSpace[ i ].m_rotation.isOk(), pszBoneName );
		ASSERTX( tModelSpace[ i ].m_translation.isOk3(), pszBoneName );
		ASSERTX( tModelSpace[ i ].m_scale.isOk3(), pszBoneName );

		ASSERTX( tLocalSpace[ i ].m_rotation.isOk(), pszBoneName );
		ASSERTX( tLocalSpace[ i ].m_translation.isOk3(), pszBoneName );
		ASSERTX( tLocalSpace[ i ].m_scale.isOk3(), pszBoneName );
	}
} // sVerifyPose()
#endif //#ifdef HAVOK_ENABLED

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static void sUpdateSelfIllumination( GAME *pGame,
									 APPEARANCE &tAppearance,
									 APPEARANCE_DEFINITION &tAppearanceDef,
									 float fTimeDelta )
{
	e_ModelUpdateIllumination( tAppearance.nId, fTimeDelta, tAppearance.bFullbright, tAppearance.bDrawBehind, tAppearance.bHit, tAppearance.AmbientLight, tAppearance.fBehindColor );
	tAppearance.bHit = FALSE;

	if( AppIsHellgate() )
	{

		RAND tRand;
		RandInit( tRand );
		float fSelfIllum = 0.0f;
		float fSelfIllumBlend = 0.0f;
		for ( ANIMATION* pAnimation= HashGetFirst( tAppearance.pAnimations ); 
			pAnimation != NULL; 
			pAnimation = HashGetNext( tAppearance.pAnimations, pAnimation ))
		{
			if ( ! pAnimation->pDefinition )
				continue;

			if ( ( pAnimation->dwFlags & ANIMATION_FLAG_EASING_OUT ) == 0 
	#ifdef HAVOK_ENABLED
				&& ( !pAnimation->pControl || pAnimation->pControl->getMasterWeight() > 0.0f ) 
	#endif
				 )
			{
				float fDuration = pAnimation->fDuration;
	#ifdef HAVOK_ENABLED
				float fTimeInAnim = pAnimation->pControl ? pAnimation->pControl->getLocalTime() : pAnimation->fTimeSinceStart;
	#else
				float fTimeInAnim = pAnimation->fTimeSinceStart;
	#endif
				float fPercent = fDuration ? fTimeInAnim / fDuration : 0.0f;
				fPercent = PIN( fPercent, 0.0f, 1.0f );

				CFloatPair tValuePair = pAnimation->pDefinition->tSelfIllumation.GetValue( fPercent );
				fSelfIllum		= tValuePair.x == tValuePair.y ? tValuePair.x : RandGetFloat( tRand, tValuePair.x, tValuePair.y );
				tValuePair = pAnimation->pDefinition->tSelfIllumationBlend.GetValue( fPercent );
				fSelfIllumBlend	= tValuePair.x == tValuePair.y ? tValuePair.x : RandGetFloat( tRand, tValuePair.x, tValuePair.y );
			}
		}

		e_ModelSetIllumination( tAppearance.nId, fSelfIllum, fSelfIllumBlend );

	}
} // sUpdateSelfIllumination()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
#ifdef HAVOK_ENABLED		
static void sComputeIKChain( 
	APPEARANCE * pAppearance,
	int nBone,
	hkQsTransform & mModelSpacePrevBoneInOut,
	const hkVector4 & vFacingTarget,
	const hkVector4 & vForward,
	const hkVector4 * pvTwistAxis,
	float fGain,
	float fTurnLimit,
	BOOL bAccumulateRotation )
{
	hkVector4 vFacingTargetLocal;
	vFacingTargetLocal.setTransformedInversePos( mModelSpacePrevBoneInOut, vFacingTarget );

	hkVector4 vFacingDirectionLocal;
	vFacingDirectionLocal.setSub4( vFacingTargetLocal, pAppearance->pPoseInLocal[ nBone ].getTranslation() );
	sVector3SafeNormalize( vFacingDirectionLocal );

	hkVector4 vForwardLocal;
	vForwardLocal.setRotatedInverseDir( mModelSpacePrevBoneInOut.m_rotation, vForward );
	sVector3SafeNormalize( vForwardLocal );

	if ( pvTwistAxis )
	{
		hkVector4 vFacingDirectionLocalOrig = vFacingDirectionLocal;
		hkVector4 vTwistLocal;
		vTwistLocal.setRotatedInverseDir( mModelSpacePrevBoneInOut.m_rotation, *pvTwistAxis );
		sVector3SafeNormalize( vTwistLocal );

		float fDot = vFacingDirectionLocal.dot3( vTwistLocal );
		vTwistLocal.mul4( fDot );
		vFacingDirectionLocal.sub3clobberW( vTwistLocal );

		if ( vFacingDirectionLocal.lengthSquared3() < 0.01f )
			vFacingDirectionLocal = vFacingDirectionLocalOrig;
		else
			sVector3SafeNormalize( vFacingDirectionLocal );
	}
	if ( ! bAccumulateRotation )
		mModelSpacePrevBoneInOut.setMulEq(pAppearance->pPoseInLocal[ nBone ]);

	hkQuaternion spineRotation;
	spineRotation.setShortestRotation( vForwardLocal, vFacingDirectionLocal );

	if ( ! spineRotation.isOk() )
		return;

	float fAngle = spineRotation.getAngle();
	if(fAngle != 0.0f)
	{
		hkVector4 vAxis;
		spineRotation.getAxis( vAxis );
		spineRotation.setAxisAngle( vAxis, fAngle * fGain );
	}

	// For each spine link
	hkQuaternion newRotation; 
	newRotation.setMul( spineRotation, pAppearance->pPoseInLocal[ nBone ].m_rotation );

	pAppearance->pPoseInLocal[ nBone ].m_rotation = newRotation;

	if ( bAccumulateRotation )
		mModelSpacePrevBoneInOut.setMulEq(pAppearance->pPoseInLocal[ nBone ]);
}
#endif

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static BOOL sAppearanceCanFaceCurrentTarget( 
	APPEARANCE * pAppearance,
	float * pfDeltaFromTwist = NULL )
{
#ifdef HAVOK_ENABLED		
	APPEARANCE_DEFINITION * pAppearanceDef = sAppearanceGetDefinition( *pAppearance );
	if (!pAppearanceDef)
		return TRUE; // nothing says that it can't

	if ( ( pAppearanceDef->dwFlags & APPEARANCE_DEFINITION_FLAG_TWIST_NECK ) == 0 )
		return FALSE; // can't twist towards it

	if ( VectorIsZero ( pAppearance->vFacingTarget ) ||
		pAppearanceDef->nNeck == INVALID_ID || 
		pAppearanceDef->nSpineTop == INVALID_ID ||
		pAppearanceDef->nSpineBottom == INVALID_ID )
		return FALSE;

	const MATRIX * pmWorldInverse = e_ModelGetWorldScaledInverse( pAppearance->nId );
	if ( ! pmWorldInverse )
		return FALSE;

	hkQsTransform mWorldInverse;
	mWorldInverse.set4x4ColumnMajor( (float *) pmWorldInverse );

	hkVector4 vFacingTarget( pAppearance->vFacingTarget.fX, pAppearance->vFacingTarget.fY, pAppearance->vFacingTarget.fZ );
	vFacingTarget.setTransformedPos( mWorldInverse, vFacingTarget );

	hkVector4 vForward( pAppearanceDef->vNeckAim.x, pAppearanceDef->vNeckAim.y, pAppearanceDef->vNeckAim.z );
	sVector3SafeNormalize( vForward );

	hkVector4 vToTarget = vFacingTarget;
	sVector3SafeNormalize( vToTarget );
	float fDot = vForward.dot3( vToTarget );
	if ( fDot < pAppearanceDef->fHeadTurnTotalLimit )
	{
		fDot = PIN( fDot, -1.0f, 1.0f );
		if ( pfDeltaFromTwist )
		{
			*pfDeltaFromTwist = acos( fDot ) - DegreesToRadians( (float) pAppearanceDef->nHeadTurnTotalLimitDegrees );
		}
		return FALSE;
	}
#endif
	return TRUE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL c_AppearanceCanFaceCurrentTarget( 
	int nAppearanceId,
	float * pfDeltaFromTwist )
{
	if ( pfDeltaFromTwist )
		*pfDeltaFromTwist = 0.0f;
	APPEARANCE *pAppearance	= HashGet( sgpAppearances, nAppearanceId );
	if (! pAppearance )
		return TRUE;
	return sAppearanceCanFaceCurrentTarget( pAppearance, pfDeltaFromTwist );
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL c_AppearanceCanTwistNeck( int nAppearanceId )
{
	APPEARANCE *pAppearance	= HashGet( sgpAppearances, nAppearanceId );
	if (! pAppearance )
		return FALSE;
	
	APPEARANCE_DEFINITION * pAppearanceDef = sAppearanceGetDefinition( *pAppearance );
	if (! pAppearanceDef )
		return FALSE;

	return ( pAppearanceDef->dwFlags & APPEARANCE_DEFINITION_FLAG_TWIST_NECK );
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static void sHandleHeadTurning( 
	APPEARANCE * pAppearance,
	APPEARANCE_DEFINITION * pAppearanceDef )
{
#ifdef HAVOK_ENABLED		
	if (!pAppearanceDef)
		return;

	const hkSkeleton * pSkeleton = pAppearance->pAnimatedSkeleton->getSkeleton();
	int nBoneCount = pSkeleton->m_numBones;

	if ( VectorIsZero ( pAppearance->vFacingTarget ) ||
		pAppearanceDef->nNeck == INVALID_ID || 
		pAppearanceDef->nSpineTop == INVALID_ID ||
		pAppearanceDef->nSpineBottom == INVALID_ID )
		return;

	float fDeltaFromTwist = 0.0f;
	BOOL bUpdateLastGood = FALSE;
	if ( (!sAppearanceCanFaceCurrentTarget( pAppearance, &fDeltaFromTwist ) && fDeltaFromTwist > 0.1f ) ||
		(pAppearance->dwFlags & APPEARANCE_FLAG_FADE_OUT_FACING ) != 0 )
	{
		pAppearance->fHeadTurnGain *= 1.0f - fabsf( pAppearanceDef->fHeadTurnSpeed ); // lerp with zero
		if ( pAppearance->fHeadTurnGain <= 0.001f )
		{
			pAppearance->fHeadTurnGain = 0.0f;
			return;
		}
	} else {
		bUpdateLastGood = TRUE;
		pAppearance->fHeadTurnGain = 0.5f * pAppearance->fHeadTurnGain + 0.5f; // lerp with one
		pAppearance->fHeadTurnGain = PIN( pAppearance->fHeadTurnGain, 0.0f, 1.0f );
	}

	const MATRIX * pmWorldInverse = e_ModelGetWorldScaledInverse( pAppearance->nId );
	if ( ! pmWorldInverse )
		return;

	hkQsTransform mWorldInverse;
	mWorldInverse.set4x4ColumnMajor( (float *) pmWorldInverse );

	hkVector4 vFacingTarget;
	if ( bUpdateLastGood )
	{
		vFacingTarget.set( pAppearance->vFacingTarget.fX, pAppearance->vFacingTarget.fY, pAppearance->vFacingTarget.fZ );
		vFacingTarget.setTransformedPos( mWorldInverse, vFacingTarget );
		pAppearance->vFacingTargetLastGood = VECTOR( vFacingTarget.getSimdAt(0), vFacingTarget.getSimdAt(1), vFacingTarget.getSimdAt(2) );
	} else {
		vFacingTarget.set( pAppearance->vFacingTargetLastGood.fX, pAppearance->vFacingTargetLastGood.fY, pAppearance->vFacingTargetLastGood.fZ );
	}

	hkVector4 vForward( pAppearanceDef->vNeckAim.x, pAppearanceDef->vNeckAim.y, pAppearanceDef->vNeckAim.z );
	sVector3SafeNormalize( vForward );

	// keeping this transformation around allows us to work on the local space of each bone - this is the model space of the previous bone
	hkQsTransform mModelSpacePrevBone; mModelSpacePrevBone.setIdentity();

	hkLocalArray<hkInt16> pnBoneChainRootToSpineBottom( 10 );
	hkSkeletonUtils::getBoneChain( *pSkeleton, (hkInt16)0, (hkInt16)pAppearanceDef->nSpineBottom, pnBoneChainRootToSpineBottom);
	for ( int i = 0; i < pnBoneChainRootToSpineBottom.getSize() - 1; i++ )
	{ // get the model space up until the bone before the spine bottom
		int nBone = pnBoneChainRootToSpineBottom[ i ];
		mModelSpacePrevBone.setMulEq(pAppearance->pPoseInLocal[ nBone ]);
	}

	if ( ( pAppearanceDef->dwFlags & APPEARANCE_DEFINITION_FLAG_TWIST_NECK ) != 0 )
	{
		float fGainForNeck  = (pAppearance->dwFlags & APPEARANCE_FLAG_AGGRESSIVE) != 0 ? 0.0f : (pAppearanceDef->fHeadTurnPercentVsTorso / 100.0f);
		float fGainForTorso = 1.0f - fGainForNeck;

		hkLocalArray<hkInt16> pnBoneChainSpineBottomToTop( 10 );
		hkSkeletonUtils::getBoneChain( *pSkeleton, (hkInt16)pAppearanceDef->nSpineBottom, (hkInt16)pAppearanceDef->nSpineTop, pnBoneChainSpineBottomToTop);

		float fGainPerPart;
		{
			int nBonesInChain = MAX(1, pnBoneChainSpineBottomToTop.getSize());
			fGainPerPart = fGainForTorso / (float)(nBonesInChain);
		}
		fGainPerPart *= pAppearance->fHeadTurnGain;
		for ( int i = 0; i < pnBoneChainSpineBottomToTop.getSize(); i++ )
		{
			int nBone = pnBoneChainSpineBottomToTop[ i ];
			ASSERT_CONTINUE( nBone >= 0 );
			ASSERT_CONTINUE( nBone < nBoneCount );
			sComputeIKChain( pAppearance, nBone, mModelSpacePrevBone, vFacingTarget, vForward, NULL, fGainPerPart, 
				pAppearanceDef->fHeadTurnBoneLimit, FALSE );
		}

		if ( fGainForNeck )
		{
			hkLocalArray<hkInt16> pnBoneChainSpineTopToNeck( 10 );
			hkSkeletonUtils::getBoneChain( *pSkeleton, (hkInt16)pAppearanceDef->nSpineTop, (hkInt16)pAppearanceDef->nNeck, pnBoneChainSpineTopToNeck);
			{
				int nBonesInChain = MAX(1, pnBoneChainSpineTopToNeck.getSize() - 1);
				fGainPerPart = fGainForNeck / (float)(nBonesInChain);
			}
			fGainPerPart *= pAppearance->fHeadTurnGain;
			for ( int i = 1; i < pnBoneChainSpineTopToNeck.getSize(); i++ )
			{
				int nBone = pnBoneChainSpineTopToNeck[ i ];
				ASSERT_CONTINUE( nBone >= 0 );
				ASSERT_CONTINUE( nBone < nBoneCount );
				sComputeIKChain( pAppearance, nBone, mModelSpacePrevBone, vFacingTarget, vForward, NULL, fGainPerPart, 
					pAppearanceDef->fHeadTurnBoneLimit, FALSE );
			}
		}
	}
	else
	{
		hkVector4 vUpVector;
		vUpVector.set(0, 0, 1.0f, 0);
		{ // turn
			ASSERT_RETURN( ( pAppearanceDef->dwFlags & APPEARANCE_DEFINITION_FLAG_TURN_AND_POINT ) != 0 );
			hkLocalArray<hkInt16> pnBoneChainSpineBottomToTop( 10 );
			hkSkeletonUtils::getBoneChain( *pSkeleton, (hkInt16)pAppearanceDef->nSpineBottom, (hkInt16)pAppearanceDef->nSpineTop, pnBoneChainSpineBottomToTop);

			float fGainPerPart;
			{
				int nBonesInChain = MAX(1, pnBoneChainSpineBottomToTop.getSize() - 1);
				fGainPerPart = 1.0f / (float)(nBonesInChain);
			}
			fGainPerPart *= pAppearance->fHeadTurnGain;
			for ( int i = 0; i < pnBoneChainSpineBottomToTop.getSize() - 1; i++ )
			{
				hkVector4 vForwardCurr;
				vForwardCurr.setRotatedDir( mModelSpacePrevBone.getRotation(), vForward );

				int nBone = pnBoneChainSpineBottomToTop[ i ];
				ASSERT_CONTINUE( nBone >= 0 );
				ASSERT_CONTINUE( nBone < nBoneCount );
				sComputeIKChain( pAppearance, nBone, mModelSpacePrevBone, vFacingTarget, vForwardCurr, &vUpVector, fGainPerPart, 
					pAppearanceDef->fHeadTurnBoneLimit, TRUE );
			}
		}

		{ // point
			hkLocalArray<hkInt16> pnBoneChainSpineTopToNeck( 10 );
			hkSkeletonUtils::getBoneChain( *pSkeleton, (hkInt16)pAppearanceDef->nSpineTop, (hkInt16)pAppearanceDef->nNeck, pnBoneChainSpineTopToNeck );

			float fGainPerPart;
			{
				int nBonesInChain = MAX(1, pnBoneChainSpineTopToNeck.getSize());
				fGainPerPart = 1.0f / (float)(nBonesInChain);
			}
			fGainPerPart *= pAppearance->fHeadTurnGain;
			for ( int i = 1; i < pnBoneChainSpineTopToNeck.getSize(); i++ )
			{
				hkVector4 vForwardCurr;
				vForwardCurr.setRotatedDir( mModelSpacePrevBone.getRotation(), vForward );

				int nBone = pnBoneChainSpineTopToNeck[ i ];
				ASSERT_CONTINUE( nBone >= 0 );
				ASSERT_CONTINUE( nBone < nBoneCount );
				sComputeIKChain( pAppearance, nBone, mModelSpacePrevBone, vFacingTarget, vForwardCurr, NULL, fGainPerPart, 
					pAppearanceDef->fHeadTurnBoneLimit, FALSE );
			}
		}
	}
#endif
} // sHandleHeadTurning()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
void c_AppearanceSystemAndHavokUpdateTugboat( GAME *pGame,
									   		  float fDelta )
{
#if !ISVERSION(SERVER_VERSION)
	pGame->fAnimationUpdateTime += fDelta;
	if ( pGame->fAnimationUpdateTime < 0 )
	{
		pGame->fAnimationUpdateTime = 0;
	}

	while ( pGame->fAnimationUpdateTime > 3600 )
	{
		pGame->fAnimationUpdateTime -= 3600;
		GrannyRecenterAllControlClocks( -3600 );
	}

	ZeroMemory( &sgtAppearanceMetrics, sizeof( APPEARANCE_METRICS ) );
	if ( !fDelta )
	{
		return;
	}

	// first step the animations 
	MATRIX mIdentity;
	MatrixIdentity( &mIdentity );

	// update the flagged appearances
	for ( APPEARANCE * pAppearance = HashGetFirst( sgpAppearances ); pAppearance;
		  pAppearance = HashGetNext( sgpAppearances, pAppearance ) )
	{
		BOOL bVisible = TRUE;
		//if ( ( e_ModelGetFlags( pAppearance->nId ) & MODEL_FLAG_VISIBLE ) == 0 )
		if ( ! ( e_ModelCurrentVisibilityToken( pAppearance->nId ) || c_GetToolMode() ) &&
			 e_ModelGetDrawLayer( pAppearance->nId ) != DRAW_LAYER_UI )
		{
			bVisible = FALSE;
		}

		if ( ( pAppearance->dwFlags & APPEARANCE_FLAG_FREEZE_ANIMATION ) == 0 )
		{
			if ( pAppearance->dwFlags & APPEARANCE_FLAG_CHECK_FOR_LOADED_ANIMS )
			{
				BOOL bClearFlag = TRUE;
				for ( ANIMATION * pAnimation = HashGetFirst( pAppearance->pAnimations ); pAnimation != NULL;
					  pAnimation = HashGetNext( pAppearance->pAnimations, pAnimation ) )
				{
					if ( ( !pAnimation->pDefinition->pGrannyFile ) && pAnimation->pDefinition->pszFile[0] != 0 )
					{
						bClearFlag = FALSE;
					}
					else if ( pAnimation->pDefinition->pGrannyFile && !pAnimation->pGrannyControl )
					{
						sAnimationAddControlAndUpdateDuration( pAppearance, pAnimation, 0 );
						pAppearance->dwFlags |= APPEARANCE_FLAG_UPDATE_WEIGHTS;
					}
				}

				if ( bClearFlag )
				{
					pAppearance->dwFlags &= ~APPEARANCE_FLAG_CHECK_FOR_LOADED_ANIMS;
				}
			}

			int count			= 0;

			BOOL bHasFreeze		= FALSE;
			for ( ANIMATION * pAnimation = HashGetFirst( pAppearance->pAnimations ); pAnimation != NULL;
				  pAnimation = HashGetNext( pAppearance->pAnimations, pAnimation ) )
			{
				if ( pAnimation->pGrannyControl )
				{
					if ( c_AnimationHasFreezeEvent( *pAnimation->pDefinition ) ||
						pAnimation->pDefinition->fEaseIn == -1)
					{
						bHasFreeze = TRUE;
					}

					count++;
				}
			}

			if ( count <= 1 && !bVisible && !bHasFreeze )
			{
				pAppearance->fAnimSampleTimer = 0;
			}
			else
			{
				pAppearance->fAnimSampleTimer = 1;
				if ( bHasFreeze )
				{
					pAppearance->fAnimSampleTimer = 2;
				}
			}

			if ( pAppearance->fAnimSampleTimer >= 1 )
			{
				for ( ANIMATION * pAnimation = HashGetFirst( pAppearance->pAnimations ); pAnimation != NULL;
					  pAnimation = HashGetNext( pAppearance->pAnimations, pAnimation ) )
				{
					if ( pAnimation->pGrannyControl )
					{
						float fLocalTime	= GrannyGetControlRawLocalClock( pAnimation->pGrannyControl );
						int looping			= GrannyGetControlLoopCount( pAnimation->pGrannyControl );

						BOOL HasFreeze		= c_AnimationHasFreezeEvent( *pAnimation->pDefinition );

						// looping animations that have just looped need to reschedule events 
						float fEaseOut		= kEaseOutConstant;
						if ( pAnimation->pDefinition->fEaseOut == -1 )
						{
							fEaseOut = 0;
						}
						else if ( pAnimation->pDefinition->fEaseOut != 0 )
						{
							fEaseOut = pAnimation->pDefinition->fEaseOut;
						}

						if ( looping == 0 && !( pAnimation->dwFlags & ANIMATION_FLAG_EASING_OUT ) )
						{
							// schedule events for the next loop 
							// if this time step will step the 
							// looping animation past the end, accounting
							// for the playback speed
							float fDurationAtSpeed1 = 
								sAnimationGetDuration( *pAnimation );
							float fSpeed = c_sControlGetSpeed( pAnimation->pGrannyControl );
							if ( fSpeed != 0.0f )
							{
								float fDurationAtCurrentSpeed = 
										fDurationAtSpeed1 / fSpeed;
								if ( fLocalTime / fSpeed + fDelta >= fDurationAtCurrentSpeed )
									c_AnimationScheduleEvents( 
										pGame, 
										pAppearance->nId, 
										fDurationAtCurrentSpeed,
										*pAnimation->pDefinition, 
										TRUE,
										TRUE );
							}
						}

						// dead animations that didn't get removed we want to just ease out/
						if ( looping != 0 && fLocalTime > sAnimationGetDuration( *pAnimation->pDefinition ) -
							 fEaseOut && !( pAnimation->dwFlags & ANIMATION_FLAG_EASING_OUT ) && !HasFreeze )
						{
							float fEaseTime = sAnimationGetDuration( *pAnimation->pDefinition ) -
								GrannyGetControlRawLocalClock( pAnimation->pGrannyControl );
							if ( fEaseTime > fEaseOut )
							{
								fEaseTime = fEaseOut;
							}

							GrannyEaseControlOut( pAnimation->pGrannyControl, fEaseTime );
							pAnimation->dwFlags |= ANIMATION_FLAG_EASING_OUT;
							pAnimation->dwFlags &= ~ANIMATION_FLAG_EASING_IN;
							pAppearance->dwFlags |= APPEARANCE_FLAG_UPDATE_WEIGHTS;
						}

						if ( looping != 0 && pAnimation->dwFlags & ANIMATION_FLAG_EASING_OUT &&
							 fLocalTime >= sAnimationGetDuration( *pAnimation->pDefinition ) )
						{
							sAnimationRemove( pAppearance, pAnimation->nId );
							pAppearance->dwFlags |= APPEARANCE_FLAG_UPDATE_WEIGHTS;
							continue;
						}

						if ( ( pAnimation->dwFlags & ANIMATION_FLAG_EASING_IN ) &&
							 GrannyGetControlEffectiveWeight( pAnimation->pGrannyControl ) >= 1 )
						{
							pAnimation->dwFlags &= ~ANIMATION_FLAG_EASING_IN;
							pAppearance->dwFlags |= APPEARANCE_FLAG_UPDATE_WEIGHTS;
						}

						if ( GrannyGetControlEffectiveWeight( pAnimation->pGrannyControl ) <= 0 &&
							 !( pAnimation->dwFlags & ANIMATION_FLAG_EASING_IN ) && !HasFreeze )
						{
							// animation is now disabled 
							if ( ( pAnimation->dwPlayFlags & PLAY_ANIMATION_FLAG_DEFAULT_ANIM ) == 0 )
							{
								sAnimationRemove( pAppearance, pAnimation->nId );
								pAppearance->dwFlags |= APPEARANCE_FLAG_UPDATE_WEIGHTS;
							}
							else if ( pAnimation->dwFlags & ANIMATION_FLAG_EASING_OUT )
							{
								pAnimation->dwFlags &= ~ANIMATION_FLAG_EASING_OUT;
								pAppearance->dwFlags |= APPEARANCE_FLAG_UPDATE_WEIGHTS;
							}
						}
					}
				}

				if ( pAppearance->dwFlags & APPEARANCE_FLAG_UPDATE_WEIGHTS )
				{
					sAppearanceUpdateWeights( *pAppearance );
				}
			}
		}
		else
		{
			if ( pAppearance->dwFlags & APPEARANCE_FLAG_UPDATE_WEIGHTS )
			{
				sAppearanceUpdateWeights( *pAppearance );
			}
		}

		if ( pAppearance->dwFlags & sGetAppearanceNeedsBonesFlag( LOD_HIGH ) )
		{
			int nBoneCount = c_AppearanceDefinitionGetBoneCount( pAppearance->nDefinitionId );
			if ( nBoneCount  )
			{
				sAppearanceInitializeBones( pAppearance, nBoneCount );
			}
		}
	}

	// update time on each animation 
	for ( APPEARANCE * pAppearance = HashGetFirst( sgpAppearances ); pAppearance;
		  pAppearance = HashGetNext( sgpAppearances, pAppearance ) )
	{
		granny_local_pose *ptLocalPose			= g_ptSharedLocalPose;
		APPEARANCE_DEFINITION *pAppearanceDef	= sAppearanceGetDefinition( *pAppearance );
		if ( !pAppearanceDef )
		{
			continue;
		}

		//if ( ( e_ModelGetFlags( pAppearance->nId ) & MODEL_FLAG_VISIBLE ) != 0 )
		if ( e_ModelCurrentVisibilityToken( pAppearance->nId ) || c_GetToolMode() )
		{
			sUpdateSelfIllumination( pGame, *pAppearance, *pAppearanceDef, fDelta );
		}

		if ( pAppearance->fAnimSampleTimer == 0 )
		{
			continue;
		}


		c_AttachmentsRemoveNonExisting( pAppearance->nId );
		if ( pAppearance->pGrannyInstance )
		{
			//DWORD dwDrawFlags	= e_ModelGetFlags( pAppearance->nId );
			BOOL bHasFreeze		= FALSE;
			for ( ANIMATION * pAnimation = HashGetFirst( pAppearance->pAnimations ); pAnimation != NULL;
				  pAnimation = HashGetNext( pAppearance->pAnimations, pAnimation ) )
			{
				if ( pAnimation->pGrannyControl )
				{
					if ( c_AnimationHasFreezeEvent( *pAnimation->pDefinition ) ||
						pAnimation->pDefinition->fEaseIn == -1 )
					{
						bHasFreeze = TRUE;
					}
				}
			}
			if ( !bHasFreeze && (
				e_ModelGetFlagbit( pAppearance->nId, MODEL_FLAGBIT_NODRAW ) ||
				e_ModelGetFlagbit( pAppearance->nId, MODEL_FLAGBIT_NODRAW_TEMPORARY ) ) )
			{
				continue;
			}

			// -- Animate -- 
			int nBoneCount		= GrannyGetSourceSkeleton( pAppearance->pGrannyInstance )->BoneCount;
			if ( ( pAppearance->dwFlags & APPEARANCE_FLAG_FREEZE_ANIMATION ) == 0 ||
				 ( pAppearance->dwFlags & APPEARANCE_FLAG_LAST_FRAME ) != 0 )
			{
				pAppearance->dwFlags &= ~APPEARANCE_FLAG_LAST_FRAME;
				if( ( pAppearance->dwFlags & APPEARANCE_FLAG_FREEZE_ANIMATION ) == 0 )
				{
					GrannySetModelClock( pAppearance->pGrannyInstance, pGame->fAnimationUpdateTime );
				}

				GrannySampleModelAnimationsAccelerated( pAppearance->pGrannyInstance, nBoneCount, mIdentity,
														ptLocalPose, pAppearance->pWorldPose );

				int count							= 0;
				for ( granny_model_control_binding *
					  Binding = GrannyModelControlsBegin( pAppearance->pGrannyInstance );
					  Binding != GrannyModelControlsEnd( pAppearance->pGrannyInstance );
					  Binding = GrannyModelControlsNext( Binding ) )
				{
					count++;
				}

				GrannyFreeCompletedModelControls( pAppearance->pGrannyInstance );

			
				// -- Grab Bones -- 
				// We need row-major 3x4 on the card, and Granny gives us column-major 4x4
				// matrices.
	
				MATRIX* pBoneMatrices4x4Granny	= (MATRIX*)GrannyGetWorldPoseComposite4x4Array(
					pAppearance->pWorldPose );

				// transpose all of the matrices since we need them to be row-major 
				for ( int nMatrix = 0; nMatrix < nBoneCount; nMatrix++ )
				{
					MatrixTranspose( &pBoneMatrices4x4Granny[nMatrix], &pBoneMatrices4x4Granny[nMatrix] );
				}

		
				// Cut off the last row since it is just 0,0,0,1.0 anyway. It will just eat up
				// registers on the card  Also, remap the bones to match the bones used in the mesh
				sCachePoseToMeshMatrices(pAppearance, pAppearance->nModelDefinitionId, LOD_HIGH, pBoneMatrices4x4Granny, nBoneCount);
			}
		}

		pAppearance->dwFlags |= APPEARANCE_FLAG_HAS_ANIMATED;

		if ( pAppearance->bDrawBehind )
		{
			c_ModelAttachmentsUpdate( pAppearance->nId, MODEL_ICONIC );
		}
		else
		{
			c_ModelAttachmentsUpdate( pAppearance->nId );
		}

		for ( ANIMATION * pAnimation = HashGetFirst( pAppearance->pAnimations ); pAnimation != NULL;
			  pAnimation = HashGetNext( pAppearance->pAnimations, pAnimation ) )
		{
			if ( ( pAppearance->dwFlags & APPEARANCE_FLAG_FREEZE_ANIMATION ) != 0 )
			{
				continue;
			}

			pAnimation->fTimeSinceStart += fDelta;
		}
	}
#endif //!ISVERSION(SERVER_VERSION)
} // c_AppearanceSystemAndHavokUpdateTugboat()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
#if !ISVERSION(SERVER_VERSION) && defined( HAVOK_ENABLED )
static void sPrepForMapPose( 
	hkArray<hkQsTransform> & pPoseInModel)
{
	int nSize = pPoseInModel.getSize();
	for ( int i = 0; i < nSize; i++ )
	{
		hkQsTransform * pTransform = &pPoseInModel[ i ];
		if ( ! pTransform->getRotation().isOk() )
		{
			pTransform->setRotation( hkQuaternion::getIdentity() );
		}
		if ( pTransform->getTranslation().lengthSquared3() > 1000000.0f ||
			 !pTransform->getTranslation().isOk4() )
		{
			pTransform->setTranslation( hkVector4( 0.0f, 0.0f, 0.0f, 0.0f ) );
		}
	}
}
#endif


///////////////////////////////////////////////////////////////////////////////////////////////////////
// CHB 2006.10.06 - Function to page in animation data.
///////////////////////////////////////////////////////////////////////////////////////////////////////

static
void ProbeForRead(const void * Address, unsigned Length)
{
	if (Length > 0)
	{
		volatile const unsigned char * p = static_cast<const unsigned char *>(Address);
		int nPageSize = 4096;
		for (unsigned i = 0; i < Length; i += nPageSize)
		{
			p[i];
		}
		p[Length - 1];
	}
}

void c_AppearanceProbeAllAnimations(void)
{
#ifdef HAVOK_ENABLED
	hkInterleavedSkeletalAnimation Dummy;
	const void * pVtable_hkInterleavedSkeletalAnimation = *reinterpret_cast<void * *>(static_cast<void *>(&Dummy));

	for (APPEARANCE * pAppearance = HashGetFirst(sgpAppearances); pAppearance != 0; pAppearance = HashGetNext(sgpAppearances, pAppearance))
	{
		const hkAnimatedSkeleton * pAnimatedSkeleton = pAppearance->pAnimatedSkeleton;
		if (pAnimatedSkeleton == 0)
		{
			continue;
		}

		// Iterate over controls.
		int nControls = pAnimatedSkeleton->getNumAnimationControls();
		for (int nControl = 0; nControl < nControls; ++nControl)
		{
			const hkAnimationControl * pAnimationControl = pAnimatedSkeleton->getAnimationControl(nControl);
			const hkAnimationBinding * pAnimationBinding = pAnimationControl->getAnimationBinding();
			const hkSkeletalAnimation * pSkeletalAnimation = pAnimationBinding->m_animation;

			// Check to see if it's a hkInterleavedSkeletalAnimation.
			// Havok API doesn't appear to offer an "official" way to
			// do this, and we can't reliably use dynamic_cast<>
			// without RTTI. So we use "poor man's method" of
			// comparing vtable pointers.
			const void * pVtable = *reinterpret_cast<const void * const *>(static_cast<const void *>(pSkeletalAnimation));
			if (pVtable == pVtable_hkInterleavedSkeletalAnimation)
			{
				const hkInterleavedSkeletalAnimation * pInterleavedSkeletalAnimation = static_cast<const hkInterleavedSkeletalAnimation *>(pSkeletalAnimation);
				ProbeForRead(pInterleavedSkeletalAnimation->m_transforms, pInterleavedSkeletalAnimation->m_numTransforms * sizeof(*pInterleavedSkeletalAnimation->m_transforms));
			}
		}
	}
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
// Since we don't scale the ragdolls, we need to tell havok that the ragdolls are not in the same place as the unit.
// Otherwise, they won't fall the proper distance.  We might need to check whether this is a good place to put the ragdoll... but we need that for ragdolls in general.
///////////////////////////////////////////////////////////////////////////////////////////////////////
static void sAdjustWorldMatrixForRagdoll(
	GAME * pGame,
	APPEARANCE * pAppearance, 
	class hkQsTransform & mWorldTransform )
{
#if !ISVERSION(SERVER_VERSION) && defined( HAVOK_ENABLED )
	float fScale = e_ModelGetScale( pAppearance->nId ).fX;
	if ( fScale == 1.0f || pAppearance->idUnit == INVALID_ID )
		return;
	UNIT * pUnit = UnitGetById( pGame, pAppearance->idUnit );
	if ( ! pUnit )
		return;
	if ( UnitIsOnGround( pUnit ) && ! UnitGetStat( pUnit, STATS_CAN_FLY ) )
		return;

	const float fBufferInZ = 0.1f;
	const float fMaxJumpHeight = 30.0f;
	int nFaceMaterial = 0;
	float fDistance = LevelLineCollideLen( pGame, UnitGetLevel( pUnit ), UnitGetPosition( pUnit ) + VECTOR( 0, 0, fBufferInZ), VECTOR( 0, 0, -1.0f ), fMaxJumpHeight, nFaceMaterial, NULL );
	if ( fDistance > fBufferInZ )
	{
		hkVector4 vOffsetForScale;
		ASSERT_RETURN( fScale != 0.0f );
		vOffsetForScale.set( 0.0f, 0.0f, fDistance / fScale - fDistance, 0.0f );
		vOffsetForScale.add4( mWorldTransform.getTranslation() );
		mWorldTransform.setTranslation( vOffsetForScale );
	}
#endif
}


///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
void c_AppearanceSystemAndHavokUpdateHellgate( 
	GAME * pGame,
	float fDelta )
{
#if !ISVERSION(SERVER_VERSION) && defined( HAVOK_ENABLED )
	pGame->fHavokUpdateTime += fDelta;

	ZeroMemory( &sgtAppearanceMetrics, sizeof( APPEARANCE_METRICS ) );

	if ( ! fDelta )
		return;

	// first step the animations
	MATRIX mIdentity;
	MatrixIdentity( & mIdentity );

	hkQsTransform mHavokIdentity;
	mHavokIdentity.setIdentity();

	VECTOR vEyeLocation = c_GetCameraLocation( pGame );

	// update the flagged appearances
	for (APPEARANCE* pAppearance = HashGetFirst(sgpAppearances); pAppearance; pAppearance = HashGetNext(sgpAppearances, pAppearance))
	{
		if ( pAppearance->dwFlags & APPEARANCE_FLAG_CHECK_FOR_LOADED_ANIMS )
		{
			BOOL bClearFlag = TRUE;
			for ( ANIMATION* pAnimation = HashGetFirst( pAppearance->pAnimations ); 
				pAnimation != NULL; 
				pAnimation = HashGetNext( pAppearance->pAnimations, pAnimation ) )
			{
				if ( !pAnimation->pDefinition->pBinding && pAnimation->pDefinition->pszFile[ 0 ] != 0 )
				{
					bClearFlag = FALSE;
				}
				else if ( pAnimation->pDefinition->pBinding && !pAnimation->pControl )
				{
					sAnimationAddControlAndUpdateDuration( pAppearance, pAnimation, 0.0f );
					pAppearance->dwFlags |= APPEARANCE_FLAG_UPDATE_WEIGHTS;
				}
			}
			if ( bClearFlag )
				pAppearance->dwFlags &= ~APPEARANCE_FLAG_CHECK_FOR_LOADED_ANIMS;
		}

		if ( pAppearance->dwFlags & APPEARANCE_FLAG_UPDATE_WEIGHTS )
			sAppearanceUpdateWeights( *pAppearance );

		if ( pAppearance->dwFlags & APPEARANCE_MASK_NEEDS_BONES )
		{
			int nBoneCount = c_AppearanceDefinitionGetBoneCount( pAppearance->nDefinitionId	);
			if ( nBoneCount )
			{
				sAppearanceInitializeBones( pAppearance, nBoneCount );
			}
		}

		//if ( pAppearance->nModelDefinitionId == INVALID_ID )
		//	continue;

		hkAnimatedSkeleton * pAnimatedSkeleton = pAppearance->pAnimatedSkeleton;
		if ( ! pAnimatedSkeleton )
			continue;

		if ( ( pAppearance->dwFlags & APPEARANCE_FLAG_FREEZE_ANIMATION ) == 0 )
		{
			// don't step the ragdolled appearances here - it just messes them up
			RAGDOLL * pRagdoll = NULL;
			RAGDOLL_DEFINITION * pRagdollDef = NULL;
			c_RagdollGetDefAndRagdoll( pAppearance->nDefinitionId, pAppearance->nId, &pRagdollDef, &pRagdoll);

			if ( sRagdollIsUsed( pRagdoll ) )
				continue;

#if POSE_PHANTOM
			if( pRagdollDef && pRagdollDef->pRagdollInstance )
			{
				if( !pAppearance->pPoseShape )
				{
					const int numBones = pRagdollDef->pRagdollInstance->getNumBones();
					ASSERT( numBones == pRagdollDef->pSkeletonRagdoll->m_numBones );
					hkLocalArray<hkShape*> ragdollShapes( numBones );
					ragdollShapes.setSize( numBones );
					for( int rb = 0; rb < numBones; rb++ )
					{
						const hkRigidBody* boneBody = 
							pRagdollDef->pRagdollInstance->getRigidBodyOfBone( rb );
						const hkShape* boneShape = boneBody->getCollidable()->getShape();
						ragdollShapes[rb] = 
							new hkTransformShape( boneShape, hkTransform::getIdentity() );
					}

					pAppearance->pPoseShape = 
						new hkListShape( ragdollShapes.begin(), ragdollShapes.getSize() );
					ASSERT( pAppearance->pPoseShape );

					// pPoseShape owns the hkTransformShapes which will be used to position 
					// the bones, so remove reference on those.
					for( int rb = 0; rb < ragdollShapes.getSize(); rb++ )
					{
						ragdollShapes[rb]->removeReference();
					}
				}

				if( !pAppearance->pPosePhantom )
				{
					pAppearance->pPosePhantom =
						new hkSimpleShapePhantom( pAppearance->pPoseShape, hkTransform::getIdentity() );
					pAppearance->pPosePhantom->getCollidableRw()->setCollisionFilterInfo( hkGroupFilter::calcFilterInfo( COLFILTER_POSE_PHANTOM_LAYER, COLFILTER_POSE_PHANTOM_SYSTEM ) );
					hkWorld* pWorld = HavokGetWorld( pAppearance->nId );
					if( pWorld )
						pWorld->addPhantom( pAppearance->pPosePhantom );
				}
			}
#endif // POSE_PHANTOM

			pAnimatedSkeleton->stepDeltaTime( fDelta );
			if ( pAppearance->dwFlags & APPEARANCE_FLAG_ANIMS_MARKED_FOR_REMOVE )
			{
				for ( ANIMATION* pAnimation = HashGetFirst( pAppearance->pAnimations ); 
					pAnimation != NULL; )
				{
					ANIMATION* pNext = HashGetNext( pAppearance->pAnimations, pAnimation );
					if ( pAnimation->dwFlags & ANIMATION_FLAG_REMOVE )
					{
						sAnimationRemove( pAppearance, pAnimation->nId );
					}
					pAnimation = pNext;
				}
				pAppearance->dwFlags &= ~APPEARANCE_FLAG_ANIMS_MARKED_FOR_REMOVE;
			}

			sgtAppearanceMetrics.nSkeletonStepCountPerFrame++;
		}

		if ( pAppearance->dwFlags & APPEARANCE_FLAG_UPDATE_WEIGHTS )
			sAppearanceUpdateWeights( *pAppearance );

	}

	const float fStepsToTake = (float)CEIL( pGame->fHavokUpdateTime / MAYHEM_UPDATE_STEP );

	int nWorldSteps = 0;
	BOOL bHavokStepped = FALSE;
	while ( pGame->fHavokUpdateTime > 0.0f )
	{
		for (APPEARANCE* pAppearance = HashGetFirst(sgpAppearances); pAppearance; pAppearance = HashGetNext(sgpAppearances, pAppearance))
		{
			//if ( pAppearance->nModelDefinitionId == INVALID_ID )
			//	continue;

			hkAnimatedSkeleton * pAnimatedSkeleton = pAppearance->pAnimatedSkeleton;
			if ( ! pAnimatedSkeleton )
				continue;

			RAGDOLL * pRagdoll = NULL;
			RAGDOLL_DEFINITION * pRagdollDef = NULL;
			c_RagdollGetDefAndRagdoll( pAppearance->nDefinitionId, pAppearance->nId, &pRagdollDef, &pRagdoll);

			BOOL bRagdollIsUsed = sRagdollIsUsed( pRagdoll ); 
			if ( (bRagdollIsUsed && ( pRagdoll->dwFlags & RAGDOLL_FLAG_DEFINITION_APPLIED ) == 0) ||
				 (pRagdoll && (pRagdoll->dwFlags & RAGDOLL_FLAG_NEEDS_TO_ENABLE) != 0 ) )
			{ // just in case it was enabled before it could load
				c_RagdollEnable( pAppearance->nId );
			}

			int nWardrobe = c_AppearanceGetWardrobe( pAppearance->nId );
			if ( bRagdollIsUsed && 
				(( pRagdoll->dwFlags & RAGDOLL_FLAG_CAN_REMOVE_FROM_WORLD ) != 0 || pRagdoll->fTimeEnabled > 15.0f ) && 
				(pAppearance->dwFlags & APPEARANCE_FLAG_HAS_ANIMATED) != 0 &&
				pAppearance->fRagdollBlend > 0.8f &&
				(nWardrobe == INVALID_ID || c_WardrobeIsUpdated( nWardrobe )) )
			{
				c_RagdollFreeze( pAppearance->nId );
			}

			if ( ! bRagdollIsUsed || ! pRagdoll->pRagdollInstance )
				continue;

			if ( pRagdoll->dwFlags & RAGDOLL_FLAG_FROZEN )
				continue;

			//pAnimatedSkeleton->stepDeltaTime( MAYHEM_UPDATE_STEP );
			//sgtAppearanceMetrics.nSkeletonStepCountPerFrame++;

			pAppearance->fRagdollPower = 0.0f;
			pAppearance->fRagdollBlend = 1.0f;

			for ( ANIMATION* pAnimation = HashGetFirst( pAppearance->pAnimations );
				pAnimation != NULL;
				pAnimation = HashGetNext( pAppearance->pAnimations, pAnimation ) )
			{
				if ( ! pAnimation->pDefinition->tRagdollBlend.IsEmpty() &&
					 ! pAnimation->pDefinition->tRagdollPower.IsEmpty() )
				{
					float fRagdollPathTime = sAnimationGetPathTime( pAnimation, 0.0f );

					CFloatPair tFloatPair = pAnimation->pDefinition->tRagdollBlend.GetValue( fRagdollPathTime );
					pAppearance->fRagdollBlend = tFloatPair.x;
					if ( tFloatPair.x != tFloatPair.y )
					{
						pAppearance->fRagdollBlend += (RandGetFloat(pGame) * (tFloatPair.y - tFloatPair.x));
					}

					tFloatPair = pAnimation->pDefinition->tRagdollPower.GetValue( fRagdollPathTime );
					pAppearance->fRagdollPower = tFloatPair.x;
					//if ( tFloatPair.x != tFloatPair.y ) // this never looks good
					//{
					//	pAppearance->fRagdollPower += (RandGetFloat() * (tFloatPair.y - tFloatPair.x));
					//}

					break;
				}
			}

			c_RagdollSetPower( pAppearance->nId, pAppearance->fRagdollPower );


			// only do this per MAYHEM_UPDATE_STEP for ragdolls - where we must do it that often
			const hkSkeleton * pSkeleton = pAppearance->pAnimatedSkeleton->getSkeleton();
			{
				PERF(HAVOK_SAMPLE)
				pAppearance->pAnimatedSkeleton->sampleAndCombineAnimations( pAppearance->pPoseInLocal.begin(), g_havokChunkCache );
			}
			PERF(HAVOK_NOTSAMPLE)

			hkSkeletonUtils::transformLocalPoseToModelPose( pSkeleton->m_numBones, 
															pSkeleton->m_parentIndices, 
															pAppearance->pPoseInLocal.begin(), 
															pAppearance->pPoseInModel.begin() );
			pAppearance->dwFlags |= APPEARANCE_FLAG_MUST_TRANSFER_POSE;

			int nRagdollBones = pRagdollDef->pSkeletonRagdoll->m_numBones;
			REF(nRagdollBones);
			hkPose tRagdollPose( pRagdollDef->pSkeletonRagdoll );

			sPrepForMapPose( pAppearance->pPoseInModel );
			pRagdollDef->pSkeletonMapperKeyToRagdoll->mapPose( pAppearance->pPoseInModel.begin(), 
																pRagdollDef->pSkeletonRagdoll->m_referencePose, 
																tRagdollPose.writeAccessPoseModelSpace().begin(),
																hkSkeletonMapper::CURRENT_POSE );

			hkQsTransform mWorldTransform;
			const MATRIX * pmWorld = e_ModelGetWorld( pAppearance->nId );
			if ( pmWorld )
				mWorldTransform.set4x4ColumnMajor( (float *) pmWorld );
			else
				mWorldTransform.setIdentity();

			BOOL bSetRagdollTransform = FALSE;
			if ( pRagdoll->dwFlags & RAGDOLL_FLAG_NEEDS_WORLD_TRANSFORM )
			{ // if we just turned the ragdoll on, we need to move it into the right place
				if( pAppearance->pPosePhantom )
				{
					hkWorld* pWorld = pAppearance->pPosePhantom->getWorld();
					if( pWorld )
					{
						pWorld->removePhantom( pAppearance->pPosePhantom );
					}
				}
				bSetRagdollTransform = TRUE;

				sAdjustWorldMatrixForRagdoll( pGame, pAppearance, mWorldTransform );

				if ( pRagdoll->pRagdollInstance )
					pRagdoll->pRagdollInstance->setPoseModelSpace( tRagdollPose.getPoseModelSpace().begin(), mWorldTransform );

				if ( pRagdoll->pRagdollRigidBodyController )
					pRagdoll->pRagdollRigidBodyController->reinitialize();

				pRagdoll->dwFlags &= ~RAGDOLL_FLAG_NEEDS_WORLD_TRANSFORM;
			}

			// ragdoll pulling doesn't use fRagdollPower (it could, to scale the ControlData)
			if ( pRagdoll->dwFlags & RAGDOLL_FLAG_IS_BEING_PULLED && !bSetRagdollTransform )
			{
				// maybe change this blend depending on length of time
				// pulled / distance from target / other?
				//pAppearance->fRagdollBlend = 0.5f;
				sVerifyPose( pRagdollDef->pSkeletonRagdoll, tRagdollPose );
				ASSERT( fStepsToTake != 0.0f );
				hkQsTransform mInterpolatedWorldTransform;
				mInterpolatedWorldTransform.setInterpolate4( pRagdoll->mPreviousWorldTransform, mWorldTransform, fStepsToTake ? (float)(nWorldSteps+1)/fStepsToTake : 0.0f );
				pRagdoll->pRagdollRigidBodyController->driveToPose( MAYHEM_UPDATE_STEP, tRagdollPose.getPoseLocalSpace().begin(), mInterpolatedWorldTransform, NULL );
				
				hkQsTransform rootWorld;
				rootWorld.setMul( mInterpolatedWorldTransform, tRagdollPose.getPoseModelSpace()[0] );
				hkKeyFrameUtility::applyHardKeyFrame( rootWorld.getTranslation(), 
					rootWorld.getRotation(), 
					1.0f/MAYHEM_UPDATE_STEP, 
					pRagdoll->pRagdollInstance->getRigidBodyOfBone(0) );
			}
			else if ( pAppearance->fRagdollPower > 0.0f )
			{
				sVerifyPose( pRagdollDef->pSkeletonRagdoll, tRagdollPose );
				c_RagdollDriveToPose( pRagdoll, tRagdollPose.getPoseLocalSpace().begin() );
			}

			if( 0 == nWorldSteps )
				pRagdoll->mPreviousWorldTransform = mWorldTransform;

			if ( bSetRagdollTransform )
			{
				float fScale = e_ModelGetScale( pAppearance->nId ).fX;
				REF(fScale);
				c_RagdollApplyAllImpacts( pRagdoll->nId );
			}

			

			//CC@HAVOK add: check for penetrations if ragdoll is active
			if(pRagdoll && !(pRagdoll->dwFlags & RAGDOLL_FLAG_DISABLED) )
			{
				c_RagdollCheckAndCorrectPenetrations(pRagdoll);
			}
			//

		}

		// then step Havok
		pGame->fHavokUpdateTime -= MAYHEM_UPDATE_STEP;
		if (!AppIsHammer())
		{
			int nLevelCount = DungeonGetNumLevels( pGame );
			{
				TIMER_STARTEX("Havok Update", DEFAULT_PERF_THRESHOLD_SMALL);
				for ( int i = 0; i < nLevelCount; i++ )
				{
					LEVEL * pLevel = LevelGetByID( pGame, (LEVELID)i );
					if ( pLevel && pLevel->m_pHavokWorld )
						pLevel->m_pHavokWorld->stepDeltaTime( MAYHEM_UPDATE_STEP );
				}
			}
		}
		else
		{
			hkWorld * pWorld = HavokGetHammerWorld();
			ASSERT_CONTINUE( pWorld );
			pWorld->stepDeltaTime( MAYHEM_UPDATE_STEP );
			sgtAppearanceMetrics.nWorldStepCountPerFrame++;
		}
		nWorldSteps++;
		bHavokStepped = TRUE;
	}

	//@@CB just to confirm - didn't hit this when I ran
	//ASSERT( nWorldSteps == (int)fStepsToTake );

	for (APPEARANCE* pAppearance = HashGetFirst(sgpAppearances);
		pAppearance; 
		pAppearance = HashGetNext(sgpAppearances, pAppearance))
	{
		//if ( pAppearance->nModelDefinitionId == INVALID_ID )
		//	continue;

		APPEARANCE_DEFINITION * pAppearanceDef = sAppearanceGetDefinition( *pAppearance );

		if (!pAppearanceDef)
			continue;

		c_AttachmentsRemoveNonExisting( pAppearance->nId );

		RAGDOLL * pRagdoll = NULL;
		RAGDOLL_DEFINITION * pRagdollDef = NULL;
		c_RagdollGetDefAndRagdoll( pAppearance->nDefinitionId, pAppearance->nId, &pRagdollDef, &pRagdoll);

		BOOL bRagdollIsUsed = sRagdollIsUsed( pRagdoll ) && pRagdollDef;

		if ( pRagdoll && ( pRagdoll->dwFlags & RAGDOLL_FLAG_DEFINITION_APPLIED ) == 0 )
			bRagdollIsUsed = FALSE;

		if ( bRagdollIsUsed && (pRagdoll->dwFlags & RAGDOLL_FLAG_FROZEN) != 0 &&
			( pAppearance->dwFlags & APPEARANCE_FLAG_MUST_TRANSFER_POSE ) == 0 )
			continue;

		if ( pRagdoll && bRagdollIsUsed )
			pRagdoll->fTimeEnabled += fDelta;

		pAppearance->fAnimSampleTimer += fDelta;
		if ( c_ModelGetRoomID( pAppearance->nId ) != INVALID_ID && 0 == ( pAppearance->dwNewFlags & APPEARANCE_NEW_FORCE_ANIMATE ) )
		{
			//DWORD dwDrawFlags = e_ModelGetFlags( pAppearance->nId );
			if ( !bRagdollIsUsed && 
				( ! ( e_ModelCurrentVisibilityToken( pAppearance->nId ) || c_GetToolMode() )
				 || e_ModelGetFlagbit( pAppearance->nId, MODEL_FLAGBIT_NODRAW ) ) )
				//((dwDrawFlags & MODEL_FLAG_VISIBLE) == 0 || (dwDrawFlags & MODEL_FLAG_NODRAW) != 0 ) )
			{
				if( pAppearance->pPosePhantom && pAppearance->pPosePhantom->getWorld() )
					pAppearance->pPosePhantom->getWorld()->removePhantom( pAppearance->pPosePhantom );
				continue;
			}

			if ( pAppearance->fApproximateSize <= 0.0f )
				c_sAppearanceUpdateApproximateSize( pAppearance );

			float fDistanceToCamera = e_ModelGetDistanceToEye( pAppearance->nId, vEyeLocation );
			if ( bRagdollIsUsed || // ragdolls get more love
				 fDistanceToCamera < sgfMaxDistanceForFullAnim ||
				(pAppearance->dwFlags & APPEARANCE_FLAG_MUST_TRANSFER_POSE) != 0 ) 
				pAppearance->fAnimSampleTimeDelay = 0.0f; // animate at full speed
			else
			{
				float fDistToMax = fDistanceToCamera - sgfMaxDistanceForFullAnim; 
				float fSize = pAppearance->fApproximateSize * e_ModelGetScale( pAppearance->nId ).fX;
				float fScaleFactor = fSize ? 
					sgfSampleReductionFactor - ( fDistToMax / (fSize * fSize) ) : 0.0f;
				pAppearance->fAnimSampleTimeDelay = 1.0f / (max( 1.0f, fScaleFactor ));
			}
		}

		sUpdateSelfIllumination( pGame, *pAppearance, *pAppearanceDef, fDelta );

		if ( ! pAppearance->pAnimatedSkeleton )
			continue;

		if ( ! c_GetToolMode() )
		{
			if ( (pAppearance->dwFlags & APPEARANCE_MASK_NEEDS_BONES) != APPEARANCE_MASK_NEEDS_BONES &&
				pAppearance->pAnimatedSkeleton->getNumAnimationControls() == 0 )
				continue;

			if ( pAppearance->fAnimSampleTimer < pAppearance->fAnimSampleTimeDelay )
				continue;

			pAppearance->fAnimSampleTimer -= max( fDelta, pAppearance->fAnimSampleTimeDelay );
			if ( pAppearance->fAnimSampleTimer > pAppearance->fAnimSampleTimeDelay )
				pAppearance->fAnimSampleTimer = 0.0f; /// this happens when they suddenly become visible
		}

		const hkSkeleton * pSkeleton = pAppearance->pAnimatedSkeleton->getSkeleton();
		int nBoneCount = pSkeleton->m_numBones;
		if ( ! nBoneCount )
			continue;

		if ( ! bRagdollIsUsed )
		{ // if we aren't in ragdoll mode, then we need to update the local here
			PERF(HAVOK_SAMPLE)
			pAppearance->pAnimatedSkeleton->sampleAndCombineAnimations( pAppearance->pPoseInLocal.begin(), g_havokChunkCache );
			sgtAppearanceMetrics.nSampleAndCombineCountPerFrame++;

			// convert to model too so we can pose collision shapes:
			// use the anim sample time delay from last time around; will be
			// 0 if the model is close enough to be fully animated. We should
			// also look to cap the number of these guys in other ways (ie,
			// have a fixed number of posed bones, which we will require to
			// interact with HavokFX anyway).
			if( pAppearance->pPosePhantom )
			{
				hkWorld* pWorld = HK_NULL;
				if ((pWorld = pAppearance->pPosePhantom->getWorld()) != NULL)
					pWorld->removePhantom( pAppearance->pPosePhantom );
				else
					pWorld = HavokGetWorld( pAppearance->nId );

				if( pWorld && (0.0f == pAppearance->fAnimSampleTimeDelay) )
				{
					hkSkeletonUtils::transformLocalPoseToModelPose( pSkeleton->m_numBones, 
						pSkeleton->m_parentIndices, 
						pAppearance->pPoseInLocal.begin(), 
						pAppearance->pPoseInModel.begin() );
	
					// map the pose to where the ragdoll would be
					const int numBones = pRagdollDef->pSkeletonRagdoll->m_numBones;
					hkLocalArray<hkQsTransform> ragdollPose( numBones );
					ragdollPose.setSize( numBones );
					sPrepForMapPose( pAppearance->pPoseInModel );
					pRagdollDef->pSkeletonMapperKeyToRagdoll->mapPose( 
						pAppearance->pPoseInModel.begin(), 
						pRagdollDef->pSkeletonRagdoll->m_referencePose, 
						ragdollPose.begin(),
						hkSkeletonMapper::CURRENT_POSE );
	
					for( int cs = 0; cs < numBones; cs++ )
					{
						hkTransform boneTransform;
						ragdollPose[cs].copyToTransformNoScale( boneTransform );
	
						const hkShape* childShape = pAppearance->pPoseShape->getChildShape(cs);
						static_cast<hkTransformShape*>(const_cast<hkShape*>(childShape))->setTransform( boneTransform );
					}
					
					const MATRIX* pmWorld = e_ModelGetWorld( pAppearance->nId );
					if( pmWorld )
					{
						hkTransform phantomTransform;
						phantomTransform.set4x4ColumnMajor( (float*)pmWorld );
						pAppearance->pPosePhantom->setTransform( phantomTransform );
						pWorld->addPhantom( pAppearance->pPosePhantom );
					}
				}
			}

#ifdef HK_CACHE_STATS
			havokCacheStatsCounter++;

			if ( havokCacheStatsCounter > 100000 )
			{
				hkOstream tFile( "c:\\animCacheStats.txt" );
				g_havokChunkCache->printCacheStats( &tFile);
				havokCacheStatsCounter = -1000000;
			}
#endif			
		} else {
			// TODO: CB the lovely ragdoll smoothing code below won't
			// do anything if this code doesn't run because Havok
			// didn't step, but if I change it to run every frame
			// then everything is all flickery; haven't looked into
			// it.
			if ( ! bHavokStepped )
				continue;
PERF(HAVOK_NOTSAMPLE)
			hkPose tRagdollPose ( pRagdollDef->pSkeletonRagdoll );
			const MATRIX * pmWorld = e_ModelGetWorld( pAppearance->nId );

			if ( ! pmWorld )
				continue;
			hkQsTransform mHavokWorld;
			mHavokWorld.set4x4ColumnMajor( (float *) pmWorld );

			sAdjustWorldMatrixForRagdoll( pGame, pAppearance, mHavokWorld );

#if 0
			pRagdoll->pRagdollInstance->getPoseModelSpace ( tRagdollPose.writeAccessPoseModelSpace().begin(), mHavokWorld );
#else
			// CB: using this technique; ie, finding the current 'client-world' time by
			// summing the current time as a result of stepping the world and the
			// remaining time (we will always be either on time or ahead by a step 
			// so the case of a negative pGame->fHavokUpdateTime is correctly handled 
			// - since the world will be in the future, the position we want is back
			// in time from there) - is a good way of interpolating between frames. 
			// Just use the calculated time value as the first argument to 
			// hkRigidBody::approxTransformAt to get the appropriate transform for the
			// current client time.
			if ( pRagdoll && pRagdoll->pRagdollInstance )
			{
				if ( pRagdoll->pRagdollInstance->getWorld() )
				{
					float timeForThisRagdoll = pRagdoll->pRagdollInstance->getWorld()->getCurrentTime() + pGame->fHavokUpdateTime;

					hkQsTransform mHavokModel;
					mHavokModel.setInverse( mHavokWorld );

					for( int rb = 0; rb < tRagdollPose.getSkeleton()->m_numBones; rb++ )
					{
						hkTransform transformAtClientTime;
						pRagdoll->pRagdollInstance->getRigidBodyOfBone(rb)->approxTransformAt( timeForThisRagdoll, transformAtClientTime );

						hkQsTransform qstransformAtClientTime;
						qstransformAtClientTime.setFromTransformNoScale( transformAtClientTime );

						tRagdollPose.writeAccessPoseModelSpace()[rb].setMul(mHavokModel, qstransformAtClientTime);
					}

				} else {
					// this is in case the ragdoll was taken out of the world before the bones were updated.
					sPrepForMapPose( tRagdollPose.writeAccessPoseLocalSpace() );
					pRagdoll->pRagdollInstance->getPoseModelSpace ( tRagdollPose.writeAccessPoseModelSpace().begin(), mHavokWorld );
				}

				hkPose tRagdollInKeyPose( hkPose::LOCAL_SPACE, pSkeleton, pAppearance->pPoseInLocal );

				//sDebugDraw( pRagdollDef->pSkeletonRagdoll, tRagdollPose.getPoseLocalSpace().begin(),  0xff00ff00 );

				// map ragdoll pose onto keyframe skeleton
				sPrepForMapPose( tRagdollInKeyPose.writeAccessPoseLocalSpace() );
				tRagdollPose.syncLocalSpace();
				sPrepForMapPose( tRagdollPose.writeAccessPoseLocalSpace() );

				pRagdollDef->pSkeletonMapperRagdollToKey->mapPose( tRagdollPose, tRagdollInKeyPose, hkSkeletonMapper::CURRENT_POSE );

				//sDebugDraw( pRagdollDef->pSkeletonRagdoll, tRagdollPose.getPoseLocalSpace().begin(),  0xffffff00 );

				//sDebugDraw( pSkeleton, tRagdollInKeyPose.getPoseLocalSpace().begin(),  0xffffffff );

				// set blend weights
				pAppearance->fRagdollBlend = (pAppearance->fRagdollBlend > 1.0f) ? 1.0f : pAppearance->fRagdollBlend;
				pAppearance->fRagdollBlend = (pAppearance->fRagdollBlend < 0.0f) ? 0.0f : pAppearance->fRagdollBlend;
				hkLocalArray<hkReal> weights(nBoneCount);
				weights.setSize(nBoneCount, 1.0f);
				int nRagdollBones = pRagdollDef ? pRagdollDef->pSkeletonMapperRagdollToKey->m_mapping.m_simpleMappings.getSize() : 0;
				for ( int i = 0; i < nRagdollBones; i++ )
				{
					int nBoneIndex = pRagdollDef->pSkeletonMapperRagdollToKey->m_mapping.m_simpleMappings[ i ].m_boneB;
					weights[ nBoneIndex ] = pAppearance->fRagdollBlend;
				}

				// blend ragdoll with keyframe poses
				hkSkeletonUtils::blendPoses(nBoneCount, 
					pAppearance->pPoseInLocal.begin(), 
					tRagdollInKeyPose.getPoseLocalSpace().begin(), 
					&weights[ 0 ],
					pAppearance->pPoseInLocal.begin());

				sPrepForMapPose( pAppearance->pPoseInLocal );
			}
#endif
		
		}
PERF(HAVOK_NOTSAMPLE)
		// adjust for world scale matrix
		if ( (pAppearanceDef->dwFlags & APPEARANCE_DEFINITION_FLAG_ALWAYS_ADJUST_SCALE) != 0 && 
			pAppearance->pPoseInLocal[ 0 ].getTranslation().lengthSquared4() != 0.0f )
		{
			float fScale = e_ModelGetScale( pAppearance->nId ).fX;
			ASSERT_CONTINUE( fScale != 0.0f );
			float fInvScale = 1.0f / fScale;
			hkVector4 vLocalTranslation = pAppearance->pPoseInLocal[ 0 ].getTranslation();
			vLocalTranslation.mul4( hkVector4( fInvScale, fInvScale, fInvScale, fInvScale ) );
			pAppearance->pPoseInLocal[ 0 ].setTranslation( vLocalTranslation );
		}

		if ( ( pAppearance->dwFlags & APPEARANCE_FLAG_FREEZE_ANIMATION ) == 0 &&
			 ( pAppearance->dwFlags & APPEARANCE_FLAG_HAS_ANIMATED) != 0 &&
			 ! bRagdollIsUsed &&
			 pAppearanceDef )
		{
			if ( ( pAppearanceDef->dwFlags & APPEARANCE_DEFINITION_FLAG_TWIST_NECK ) != 0 ||
				 ( pAppearanceDef->dwFlags & APPEARANCE_DEFINITION_FLAG_TURN_AND_POINT ) != 0 )
				sHandleHeadTurning( pAppearance, pAppearanceDef );
		}

		// do this before converting to Model Pose
		sAppearanceApplyLengthenedBones ( pAppearance, nBoneCount );

		if ( pAppearance->pnShrunkBones )
		{
			sAppearanceApplyShrunkBones( pAppearance, nBoneCount );
		}

#ifdef HAVOK_ENABLED
		{
			int nTrackBoneCount = pAppearance->pBonesToTrack.Count();
			for ( int i = 0; i < nTrackBoneCount; i++ )
			{
				ASSERT_CONTINUE( pAppearance->pBonesToTrack[ i ].nBone >= 0 );
				ASSERT_CONTINUE( pAppearance->pBonesToTrack[ i ].nBone < nBoneCount );
				pAppearance->pBonesToTrack[ i ].mBoneInModel = pAppearance->pPoseInModel[ pAppearance->pBonesToTrack[ i ].nBone ];
			}
		}
#endif
		hkSkeletonUtils::transformLocalPoseToModelPose( nBoneCount, 
														pSkeleton->m_parentIndices, 
														pAppearance->pPoseInLocal.begin(), 
														pAppearance->pPoseInModel.begin() );

		// do this after converting to Model Pose
		sAppearanceApplyFattenedBones ( pAppearance, pAppearanceDef, nBoneCount );

		//if ( ( e_ModelGetFlags( pAppearance->nId ) & MODEL_FLAG_NODRAW ) == 0 )
		//	sDebugDraw( pSkeleton, pAppearance->pPoseInLocal.begin(),  0xffff00ff );

		pAppearance->dwFlags |= APPEARANCE_FLAG_HAS_ANIMATED;
		pAppearance->dwFlags &= ~APPEARANCE_FLAG_MUST_TRANSFER_POSE;

		sTransferPoseToMeshMatrices( pAppearance );

		c_ModelAttachmentsUpdate( pAppearance->nId );
	}

	// update time on each animation
	for (APPEARANCE* pAppearance = HashGetFirst(sgpAppearances); pAppearance; pAppearance = HashGetNext(sgpAppearances, pAppearance))
	{
		for ( ANIMATION* pAnimation = HashGetFirst( pAppearance->pAnimations ); 
			  pAnimation != NULL; 
			  pAnimation = HashGetNext( pAppearance->pAnimations, pAnimation ) )
		{
			if ( ( pAppearance->dwFlags & APPEARANCE_FLAG_FREEZE_ANIMATION ) != 0  &&
				pAnimation->pDefinition->tRagdollBlend.IsEmpty() )
				continue;

			pAnimation->fTimeSinceStart += fDelta;
		}
	}

	// step HavokFX at frame rate (looks better)
	{
		PERF(HAVOK_FX)
		e_HavokFXStepWorld( fDelta );
	}

	// we can't step this with every step of the havok world - it is too slow
	c_HavokStepVisualDebugger();
#endif //!ISVERSION(SERVER_VERSION)
} // c_AppearanceSystemAndHavokUpdateHellgate()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
void c_AppearanceSystemAndHavokUpdate( GAME *pGame,
									   float fDelta )
{
	if( AppIsHellgate() )
	{
		c_AppearanceSystemAndHavokUpdateHellgate( pGame, fDelta );
	}
	if( AppIsTugboat() )
	{
		c_AppearanceSystemAndHavokUpdateTugboat( pGame, fDelta );
	}
}
///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
void c_AppearanceSyncMixableAnimations( 
	int nAppearanceId,
	float fTimePercent )
{
#ifdef HAVOK_ENABLED		
	if( AppIsHellgate() )
	{
		APPEARANCE *pAppearance = HashGet( sgpAppearances, nAppearanceId );
		if ( pAppearance )
		{
	
	//trace( "---\n" );	
	//trace( "Synchronizing Mixable Animations\n" );	
			for (ANIMATION* pAnimation = HashGetFirst( pAppearance->pAnimations );
				 pAnimation != NULL;
				 pAnimation = HashGetNext( pAppearance->pAnimations, pAnimation ))
			{
	
	//APPEARANCE_DEFINITION * pAppearanceDef = sAppearanceGetDefinition( *pAppearance );			
	//const char * pszWeightGroupName = c_AppearanceDefinitionGetWeightGroupName( pAppearanceDef, pAnimation->pDefinition->nBoneWeights );			
	//trace( 
	//	"ANIM:%s MIX:%d WG:%s EI:%f EO:%f\n", 
	//	pAnimation->pDefinition->pszFile,
	//	(pAnimation->pDefinition->dwFlags & ANIMATION_DEFINITION_FLAG_CAN_MIX) != 0,
	//	pszWeightGroupName ? pszWeightGroupName : "None",
	//	pAnimation->pDefinition->fEaseIn,
	//	pAnimation->pDefinition->fEaseOut);
		
				if (pAnimation->pDefinition->dwFlags & ANIMATION_DEFINITION_FLAG_CAN_MIX &&
					pAnimation->pControl)
				{
					sSetControlLocalTime( pAnimation->pControl, fTimePercent * pAnimation->fDuration, pAnimation->fDuration );
				}			
				
			}
	//trace( "---\n" );
			
		}
	}
#endif
} // c_AppearanceSyncMixableAnimations()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
void c_sAppearanceRandomizeStartOffsetAnimations( 
	APPEARANCE *pAppearance,
	float fTimePercent )
{
#ifdef HAVOK_ENABLED		
	if( AppIsHellgate() && pAppearance )
	{
	//trace( "---\n" );	
	//trace( "Synchronizing Mixable Animations\n" );	
		for (ANIMATION* pAnimation = HashGetFirst( pAppearance->pAnimations );
			 pAnimation != NULL;
			 pAnimation = HashGetNext( pAppearance->pAnimations, pAnimation ))
		{
	//APPEARANCE_DEFINITION * pAppearanceDef = sAppearanceGetDefinition( *pAppearance );			
	//const char * pszWeightGroupName = c_AppearanceDefinitionGetWeightGroupName( pAppearanceDef, pAnimation->pDefinition->nBoneWeights );			
	//trace( 
	//	"ANIM:%s MIX:%d WG:%s EI:%f EO:%f\n", 
	//	pAnimation->pDefinition->pszFile,
	//	(pAnimation->pDefinition->dwFlags & ANIMATION_DEFINITION_FLAG_CAN_MIX) != 0,
	//	pszWeightGroupName ? pszWeightGroupName : "None",
	//	pAnimation->pDefinition->fEaseIn,
	//	pAnimation->pDefinition->fEaseOut);
	
			if ((pAnimation->dwPlayFlags & PLAY_ANIMATION_FLAG_RAND_START_TIME) != 0 &&
				pAnimation->pControl)
			{
				sSetControlLocalTime( pAnimation->pControl, fTimePercent * pAnimation->fDuration, pAnimation->fDuration );
			}			
	//trace( "---\n" );
		}
	}
#endif
} // c_AppearanceSyncMixableAnimations()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
float c_AppearanceAnimationGetDuration( int nAppearanceId,
										int nAnimationId )
{
	// get appearance 
	APPEARANCE *pAppearance		= HashGet( sgpAppearances, nAppearanceId );

	// get animation instance 
	ANIMATION *pAnimation		= HashGet( pAppearance->pAnimations, nAnimationId );

	// return the velocity speed adjusted duration of animation instance 
	return ( pAnimation->fDuration );
} // c_AppearanceAnimationGetDuration()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL c_AnimationHasFile( int nAppearanceId,
						 int nAnimationId )
{
	// get appearance 
	APPEARANCE *pAppearance		= HashGet( sgpAppearances, nAppearanceId );

	// get animation instance 
	ANIMATION *pAnimation		= HashGet( pAppearance->pAnimations, nAnimationId );

	// check for file in definition 
	if ( pAnimation->pDefinition->pszFile == NULL || pAnimation->pDefinition->pszFile[0] == 0 )
	{
		return ( FALSE );		// no file 
	}

	return ( TRUE ); 			// has file 
} // c_AnimationHasFile()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
static void c_sDumpAnims( int nAppearanceId,
						  const char *pszTitle )
{
	APPEARANCE *pAppearance = HashGet( sgpAppearances, nAppearanceId );
	if ( pAppearance )
	{
		BOOL bPreviousTrace = sgbAnimTrace;
		sgbAnimTrace = TRUE;
		trace( "ANIMATION DUMP: '%s'\n", pszTitle );
		for ( ANIMATION * pAnimation = HashGetFirst( pAppearance->pAnimations ); 
			  pAnimation != NULL;
			  pAnimation = HashGetNext( pAppearance->pAnimations, pAnimation ) )
		{
			sAnimTrace( pszTitle, *pAppearance, pAnimation );
		}

		sgbAnimTrace = bPreviousTrace;
	}
} // c_sDumpAnims()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
void c_AppearanceDumpAnims( UNIT *pPlayer )
{
	int nAppearanceFirst = c_UnitGetModelIdFirstPerson( pPlayer );
	int nAppearanceThird = c_UnitGetModelIdThirdPerson( pPlayer );
	c_sDumpAnims( nAppearanceFirst, "First Person" );

	c_sDumpAnims( nAppearanceThird, "Third Person" );
} // c_AppearanceDumpAnims()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL c_AppearanceCheckOtherAnimsForSameStance( int nAppearanceId,
											   const ANIMATION_DEFINITION *pAnimDef )
{
	APPEARANCE *pAppearance = HashGet( sgpAppearances, nAppearanceId );
	ASSERT_RETFALSE( pAppearance );
	for ( ANIMATION * pAnimation = HashGetFirst( pAppearance->pAnimations ); pAnimation != NULL;
		  pAnimation = HashGetNext( pAppearance->pAnimations, pAnimation ) )
	{
		if ( !pAnimation->pDefinition )
		{
			continue;
		}

		if ( pAnimation->pDefinition->nUnitMode != pAnimDef->nUnitMode )
		{
			continue;
		}

		if ( pAnimation->fTimeSinceStart != 0.0f )
		{
			continue;   	 // we only want animation that we just started 
		}

		if ( pAnimation->pDefinition->nStartStance != pAnimDef->nStartStance )
		{
			return ( FALSE );
		}

		if ( pAnimation->pDefinition->nEndStance != pAnimDef->nEndStance )
		{
			return ( FALSE );
		}
	}

	return ( TRUE );
}	// c_AppearanceCheckOtherAnimsForSameStance()

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
void c_AppearanceLoadTextureOverrides(
	int pnOverrideIdsOut[], 
	const char * pszFullPath, 
	const char * pszTextureOverride, 
	int nTextureType, 
	int nModelDefId)
{
#if !ISVERSION( SERVER_VERSION )
	if ( ! e_TextureIsValidID( pnOverrideIdsOut[LOD_HIGH] ) )
	{
		char szFile[DEFAULT_FILE_WITH_PATH_SIZE];
		PStrPrintf( szFile, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s", pszFullPath, pszTextureOverride );
		V( e_TextureNewFromFile( &pnOverrideIdsOut[LOD_HIGH], szFile, TEXTURE_GROUP_UNITS, nTextureType ) );
		//e_TextureAddRef( pnOverrideIdsOut[LOD_HIGH] );// we aren't going to add a ref until we really use the texture
		ASSERTV( pnOverrideIdsOut[ LOD_HIGH ] != 0, "Erroneous zero texture ID encountered in appearance load overrides!" );
	}
	// Only go after the low LOD version if we have a low LOD model.
	// Need a better way to do this...
//	int nNextDef = e_ModelDefinitionGetLODNext(nModelDefId);
//	if (nNextDef != INVALID_ID)
	switch ( nTextureType )
	{
	case TEXTURE_DIFFUSE:
	case TEXTURE_SELFILLUM:
		if ( ! e_TextureIsValidID( pnOverrideIdsOut[LOD_LOW] ) )
		{
			char szFile[DEFAULT_FILE_WITH_PATH_SIZE];
			PStrPrintf( szFile, DEFAULT_FILE_WITH_PATH_SIZE, "%slow\\%s", pszFullPath, pszTextureOverride );
			V( e_TextureNewFromFile( &pnOverrideIdsOut[LOD_LOW], szFile, TEXTURE_GROUP_UNITS, nTextureType, TEXTURE_FLAG_NOERRORIFMISSING ) );
			//e_TextureAddRef( pnOverrideIdsOut[LOD_LOW] );// we aren't going to add a ref until we really use the texture
			ASSERTV( pnOverrideIdsOut[ LOD_LOW ] != 0, "Erroneous zero texture ID encountered in appearance load overrides!" );
		}
		break;
	default:
		pnOverrideIdsOut[LOD_LOW] = INVALID_ID;
		break;
	}
#endif
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
// 
///////////////////////////////////////////////////////////////////////////////////////////////////////
void c_AppearanceLoadTextureSpecificOverrides(
	int* pnOverrideIdOut, 
	const char * pszFullPath, 
	const char * pszTextureOverride, 
	int nTextureType, 
	int nModelDefId )
{
#if !ISVERSION( SERVER_VERSION )
	if ( e_TextureIsValidID( *pnOverrideIdOut ) )
		return;

	char szFile[DEFAULT_FILE_WITH_PATH_SIZE];
	PStrPrintf( szFile, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s", pszFullPath, pszTextureOverride );
	V( e_TextureNewFromFile( pnOverrideIdOut, szFile, TEXTURE_GROUP_UNITS, nTextureType ) );
	//e_TextureAddRef( *pnOverrideIdOut ); // we aren't going to add a ref until we really use the texture
	ASSERTV( pnOverrideIdOut != 0, "Erroneous zero texture ID encountered in appearance load overrides!" );
#endif
}
