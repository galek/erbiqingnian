//----------------------------------------------------------------------------
// e_main.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __E_MAIN__
#define __E_MAIN__

#include "e_dll.h"
#include "e_common.h"
#include "e_model.h"
#include "e_anim_model.h"
#include "e_adclient.h"
#include "e_obscurance.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define RF_ALLOW_UI						MAKE_MASK(0)
#define RF_ALLOW_GENERATE_CUBEMAP		MAKE_MASK(1)
#define RF_ALLOW_FADE					MAKE_MASK(2)
#define RF_ALLOW_TOOL_RENDER			MAKE_MASK(3)
#define RF_ALLOW_SCREENSHOT				MAKE_MASK(4)
#define RF_ALLOW_AUTOMAP_UPDATE			MAKE_MASK(5)
#define RF_ALLOW_SCREENEFFECTS			MAKE_MASK(6)

#define RF_DEFAULT						( RF_ALLOW_UI | RF_ALLOW_GENERATE_CUBEMAP | RF_ALLOW_FADE | RF_ALLOW_TOOL_RENDER | RF_ALLOW_SCREENSHOT | RF_ALLOW_AUTOMAP_UPDATE | RF_ALLOW_SCREENEFFECTS )


#define VIEW_OVERRIDE_VIEW_MATRIX		MAKE_MASK(0)
#define	VIEW_OVERRIDE_EYE_LOCATION		MAKE_MASK(1)
#define VIEW_OVERRIDE_EYE_DIRECTION		MAKE_MASK(2)
#define VIEW_OVERRIDE_EYE_LOOKAT		MAKE_MASK(3)


//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

typedef float (*PFN_GET_GAME_TIME_FLOAT)( void );
typedef PRESULT  (*PFN_PARTICLEDRAW)(	
	BYTE bDrawLayer,
	const MATRIX * pmatView, 
	const MATRIX * pmatProj,
	const MATRIX * pmatProj2, 
	const VECTOR & vEyePosition,
	const VECTOR & vEyeDirection,
	BOOL bPixelShader,
	BOOL bDepthOnly );
typedef void (*PFN_TOOLNOTIFY)();

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

const TCHAR* e_GetResultString( PRESULT pr );
class UI_RECT;

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

inline void e_InitCallbacks(
	PFN_GET_GAME_TIME_FLOAT pfnTimeCallback,
	PFN_PARTICLEDRAW pfnParticleDraw,
	PFN_PARTICLEDRAW pfnParticleDrawLightmap,
	PFN_PARTICLEDRAW pfnParticleDrawPreLightmap,
	PFN_GET_BONES pfn_GetBones,
	PFN_RAGDOLL_GET_POS pfn_RagdollGetPosition,
	PFN_APPEARANCE_REMOVE pfn_AppearanceRemove,
	PFN_APPEARANCE_CREATE_SKYBOX_MODEL pfn_AppearanceCreateSkyboxModel,
	PFN_APPEARANCE_DEF_GET_HAVOK_SKELETON pfn_AppearanceDefGetHavokSkeleton,
	PFN_APPEARANCE_DEF_GET_INVERSE_SKIN_TRANSFORM pfn_AppearanceDefGetInverseSkinTransform,
	PFN_ANIMATED_MODEL_DEF_JUST_LOADED pfn_AnimatedModelDefJustLoaded,
	PFN_TOOLNOTIFY pfn_ToolUpdate,
	PFN_TOOLNOTIFY pfn_ToolRender,
	PFN_TOOLNOTIFY pfn_ToolDeviceLost,
	PFN_AD_CLIENT_ADD_AD pfn_AdClientAddAd,
	PFN_QUERY_BY_FILE pfn_ObscuranceShouldComputeForFile,
	PFN_QUERY_BY_FILE pfn_BGFileInExcel,
	PFN_QUERY_BY_FILE pfn_BGFileOccludesVisibility )
{
	extern PFN_GET_GAME_TIME_FLOAT gpfn_GameGetTimeFloat;
	gpfn_GameGetTimeFloat = pfnTimeCallback;
	extern PFN_PARTICLEDRAW gpfn_ParticlesDraw;
	extern PFN_PARTICLEDRAW gpfn_ParticlesDrawLightmap;
	extern PFN_PARTICLEDRAW gpfn_ParticlesDrawPreLightmap;
	gpfn_ParticlesDraw = pfnParticleDraw;
	gpfn_ParticlesDrawLightmap = pfnParticleDrawLightmap;
	gpfn_ParticlesDrawPreLightmap = pfnParticleDrawPreLightmap;
	extern PFN_GET_BONES gpfn_GetBones;
	gpfn_GetBones = pfn_GetBones;
	extern PFN_RAGDOLL_GET_POS gpfn_RagdollGetPosition;
	gpfn_RagdollGetPosition = pfn_RagdollGetPosition;
	extern PFN_APPEARANCE_REMOVE gpfn_AppearanceRemove;
	gpfn_AppearanceRemove = pfn_AppearanceRemove;
	extern PFN_APPEARANCE_CREATE_SKYBOX_MODEL gpfn_AppearanceCreateSkyboxModel;
	gpfn_AppearanceCreateSkyboxModel = pfn_AppearanceCreateSkyboxModel;
	extern PFN_APPEARANCE_DEF_GET_HAVOK_SKELETON gpfn_AppearanceDefGetHavokSkeleton;
	gpfn_AppearanceDefGetHavokSkeleton = pfn_AppearanceDefGetHavokSkeleton;
	extern PFN_APPEARANCE_DEF_GET_INVERSE_SKIN_TRANSFORM gpfn_AppearanceDefGetInverseSkinTransform;
	gpfn_AppearanceDefGetInverseSkinTransform = pfn_AppearanceDefGetInverseSkinTransform;
	extern PFN_ANIMATED_MODEL_DEF_JUST_LOADED gpfn_AnimatedModelDefinitionJustLoaded;
	gpfn_AnimatedModelDefinitionJustLoaded = pfn_AnimatedModelDefJustLoaded;
	extern PFN_TOOLNOTIFY gpfn_ToolUpdate;
	gpfn_ToolUpdate = pfn_ToolUpdate;
	extern PFN_TOOLNOTIFY gpfn_ToolRender;
	gpfn_ToolRender = pfn_ToolRender;
	extern PFN_TOOLNOTIFY gpfn_ToolDeviceLost;
	gpfn_ToolDeviceLost = pfn_ToolDeviceLost;
	extern PFN_AD_CLIENT_ADD_AD gpfn_AdClientAddAd;
	gpfn_AdClientAddAd = pfn_AdClientAddAd;
	extern PFN_QUERY_BY_FILE gpfn_ObscuranceShouldComputeForFile;
	gpfn_ObscuranceShouldComputeForFile = pfn_ObscuranceShouldComputeForFile;
	extern PFN_QUERY_BY_FILE gpfn_BGFileInExcel;
	gpfn_BGFileInExcel = pfn_BGFileInExcel;
	extern PFN_QUERY_BY_FILE gpfn_BGFileOccludesVisibility;
	gpfn_BGFileOccludesVisibility = pfn_BGFileOccludesVisibility;

	// set the PRESULT string callback
	PRSetFacilityGetResultStringCallback( FACILITY_PRIME_ENGINE, e_GetResultString );
}

inline float e_GameGetTimeFloat()
{
	extern PFN_GET_GAME_TIME_FLOAT gpfn_GameGetTimeFloat;
	ASSERT_RETVAL( gpfn_GameGetTimeFloat, 0.0f );
	return gpfn_GameGetTimeFloat();
}

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

void DefinitionInitEngine();

PRESULT e_PlatformInit();
PRESULT e_DeviceInit( void * pData );
PRESULT e_DeviceCreate( HWND hWnd );
PRESULT e_DeviceCreateMinimal( HWND hWnd );
PRESULT e_GatherGeneralCaps();
PRESULT e_Cleanup( BOOL bDumpLists = FALSE, BOOL bCleanParticles = FALSE );
PRESULT e_Destroy();
PRESULT e_DeviceRelease();
PRESULT e_Update();
PRESULT e_RenderUIOnly( BOOL bForceRender = FALSE );
PRESULT e_Render( int nViewerID = INVALID_ID, BOOL bForceRender = FALSE, DWORD dwFlags = RF_DEFAULT );
//PRESULT e_RenderViewer( int nViewerID );
PRESULT e_PostRender();
PRESULT e_Reset();
PRESULT e_OptimizeManagedResources();
PRESULT e_CheckValid();

BOOL e_ShouldRender();


PRESULT e_GetWorldViewProjMatrix( MATRIX *pmOut, BOOL bAllowStaleData = FALSE );
PRESULT e_GetViewDirection( VECTOR *pvOut );
PRESULT e_GetViewPosition( VECTOR *pvOut );
PRESULT e_GetViewFocusPosition( VECTOR *pvOut );
PRESULT e_GetWorldViewProjMatricies( MATRIX *pmatWorld, MATRIX *pmatView, MATRIX *pmatProj, MATRIX *pmatProj2 = NULL, BOOL bAllowStaleData = FALSE );
PRESULT e_AddRenderToTexture( int nModelID, int nDestTextureID, RECT * pDestVPRect, int nEnvDefID = INVALID_ID, RECT * pDestSpacingRect = NULL );
PRESULT e_PrepareUIModelRenders();
int  e_RenderModelList( struct RENDERLIST_PASS * pPass );
void e_SetViewportOverride( BOUNDING_BOX* pBBox );
BOUNDING_BOX*	e_GetViewportOverride( void );

PRESULT e_UIModelRender(
	int nModelID,
	UI_RECT * ptScreenCoords,
	UI_RECT * ptOrigCoords,
	float fRotationOffset,
	const VECTOR & vInventoryCamFocus,
	float fInventoryCamRotation,
	float fInventoryCamPitch,
	float fInventoryCamFOV,
	float fInventoryCamDistance,
	const char * pszInventoryEnvName );

void e_InitMatrices( 
	MATRIX *pmatWorld, 
	MATRIX *pmatView, 
	MATRIX *pmatProj, 
	MATRIX *pmatProj2 = NULL, 
	VECTOR *pvEyeVector = NULL, 
	VECTOR *pvEyeLocation = NULL,
	VECTOR *pvEyeLookat = NULL,
	BOOL bAllowStaleData = FALSE,
	const struct CAMERA_INFO * pCameraInfo = NULL );

DWORD e_GetBackgroundColor( ENVIRONMENT_DEFINITION * pEnvDefOverride = FALSE );

//void e_OutputModalMessage( const char * pszText, const char * pszCaption );	// CHB 2007.08.01 - String audit: unused

float e_GetElapsedTime();
void e_InitElapsedTime();
float e_GetElapsedTimeRespectingPauses();
TIME e_GetCurrentTimeRespectingPauses();
void e_UpdateElapsedTime();

void e_RenderEnablePresent(bool bEnable);

BOOL e_DX10IsEnabled();

void e_SetCurrentViewOverrides( MATRIX * pmatView, VECTOR * pvEyeLocation, VECTOR * pvEyeDirection, VECTOR * pvEyeLookat );
void e_GetCurrentViewOverrides( MATRIX * pmatView, VECTOR * pvEyeLocation, VECTOR * pvEyeDirection, VECTOR * pvEyeLookat );

PRESULT e_GetNextSwapChainIndex( int & nNextSwapChainIndex );

#endif // __E_MAIN__
