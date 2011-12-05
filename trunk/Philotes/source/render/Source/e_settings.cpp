//----------------------------------------------------------------------------
// e_settings.cpp
//
// - Implementation for graphical settings structure
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"
#include "e_definition.h"
#include "e_texture.h"
#include "e_settings.h"
#include "e_environment.h"
#include "e_caps.h"
#include "e_debug.h"
#include "e_hdrange.h"
#include "e_main.h"
#include "e_optionstate.h"	// CHB 2007.02.28
#include "dxC_caps.h"
#include "dx9_shadowtypeconstants.h"	// CHB 2006.11.02
#include "excel_private.h"
#include "datatables.h"
#include "e_dpvs.h"
#include "e_screen.h"

// debug
#include "e_model.h"

// CHB 2007.01.01 - For dx9_CapGetValue()
#ifdef ENGINE_TARGET_DX9
#include "dxC_caps.h"
#endif

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define DISTANCE_SPRING_MIN_DIST					30.f
#define DISTANCE_SPRING_NULL_ZONE					0.1f
#define DISTANCE_SPRING_MIN_BATCH_THRESHOLD			20

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

struct DISTANCE_SPRING
{
	float fDistanceCoef;		// multiplier for environment-defined distances
	float fStrengthPlus;		// positive change spring strength
	float fStrengthMinus;		// negative change spring strength
	float fMaxDeltaPlus;		// max absolute positive delta per iteration (non-fixed timestep!)
	float fMaxDeltaMinus;		// max absolute negative delta per iteration (non-fixed timestep!)
};

struct RENDERFLAG_DEFINITION
{
	char szName[ RENDERFLAG_NAME_MAX_LEN ];
	RENDERFLAG_TYPE eType;
	int nDefaultValue;
	int nTugboatDefaultValue;
};

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

int gpnRenderFlags[ NUM_RENDER_FLAGS ] = { 0 };
PFN_RENDER_FLAG_ON_SET gpfn_RenderFlagOnSet[ NUM_RENDER_FLAGS ] = { NULL };

RENDERFLAG_DEFINITION gtRenderFlags[ NUM_RENDER_FLAGS ] =
{
#include "e_renderflag_def.h"
};

BOOL gbRenderEnabled = TRUE;
BOOL gbUIOnlyRender = FALSE;
BOOL gbCheapMode = FALSE;
BOOL gbDebugTrace = FALSE;

BOOL gbUICovered		= FALSE;
BOOL gbUICoveredLeft	= FALSE;
float gfUICoverageWidth = 0;

float gfNearClipPlane = 0.1f;
float gfFarClipPlane = 50.0f;

DWORD gdwFogColor	   = RGB( 25, 30, 30 );
float gfFogFarDistance = gfFarClipPlane;
float gfFogNearDistance = 0.0f;
float gfFogMaxDensity  = 1.0f;
BOOL gbFogEnabled = FALSE;

DISTANCE_SPRING gtDistanceSpring = { 1.f, 0.f, 0.f, 0.f, 0.f };

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

void e_RFOSAdaptiveDistances( RENDER_FLAG eFlag, int nNewValue );

//----------------------------------------------------------------------------
// IMPLEMENTATION
//----------------------------------------------------------------------------

static float sGetSpringMinCoef()
{
	// determine the min coef based on the min distance and current env distance settings
	ENVIRONMENT_DEFINITION * pEnvDef = e_GetCurrentEnvironmentDef();
	if ( ! pEnvDef )
		return 1.0f;
	return DISTANCE_SPRING_MIN_DIST / e_GetClipDistance( pEnvDef );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_InitializeRenderFlags()
{
	int nCount = NUM_RENDER_FLAGS;
	for ( int i = 0; i < nCount; i++ )
	{
		if ( AppIsTugboat() )
			e_SetRenderFlag( (RENDER_FLAG)i, gtRenderFlags[ i ].nTugboatDefaultValue );
		else
			e_SetRenderFlag( (RENDER_FLAG)i, gtRenderFlags[ i ].nDefaultValue );
	}

	// check settings
	ENGINE_OVERRIDES_DEFINITION * pOverrides = e_GetEngineOverridesDefinition();
	if ( pOverrides )
	{
		if ( pOverrides->dwFlags & GFX_FLAG_LOD_EFFECTS_ONLY )
			e_SetRenderFlag( RENDER_FLAG_LOD_EFFECTS_ONLY,						TRUE );
		if ( pOverrides->dwFlags & GFX_FLAG_DISABLE_ADAPTIVE_BATCHES )
			e_SetRenderFlag( RENDER_FLAG_ADAPTIVE_DISTANCES,					FALSE );
		if ( pOverrides->dwFlags & GFX_FLAG_DISABLE_DYNAMIC_LIGHTS )
			e_SetRenderFlag( RENDER_FLAG_DYNAMIC_LIGHTS,						FALSE );
		if ( pOverrides->dwFlags & GFX_FLAG_DISABLE_NORMAL_MAPS )
			e_SetRenderFlag( RENDER_FLAG_FORCE_GEOMETRY_NORMALS,				TRUE );
		if ( pOverrides->dwFlags & GFX_FLAG_DISABLE_PARTICLES )
			e_SetRenderFlag( RENDER_FLAG_PARTICLES_ENABLED,						FALSE );
		if ( pOverrides->dwFlags & GFX_FLAG_SOFTWARE_SYNC )
			e_SetRenderFlag( RENDER_FLAG_SYNC_TO_GPU,							TRUE );
	}
#ifdef DX10_DISABLE_FX_LOD
	e_SetRenderFlag( RENDER_FLAG_LOD_EFFECTS, FALSE );
#endif

#ifndef ENABLE_DPVS
	e_SetRenderFlag( RENDER_FLAG_DPVS_ENABLED, FALSE );
	//e_SetRenderFlag( RENDER_FLAG_DBG_RENDER_ENABLED, FALSE );
#endif


#if ! ISVERSION(DEVELOPMENT)
	// disable development renderflags
	e_SetRenderFlag( RENDER_FLAG_OCCLUSION_QUERY_ENABLED, FALSE );

#endif

	// don't render skybox in Hammer (at least, by default)
	if ( c_GetToolMode() )
		e_SetRenderFlag( RENDER_FLAG_SKYBOX,								FALSE );

#if defined( ENGINE_TARGET_DX9 ) || defined( DX10_EMULATE_DX9_BEHAVIOR )
	// Disable zpass by default on DX9
	e_SetRenderFlag( RENDER_FLAG_ZPASS_ENABLED, FALSE );
#endif

	gpfn_RenderFlagOnSet[ RENDER_FLAG_ADAPTIVE_DISTANCES ]	= e_RFOSAdaptiveDistances;

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_RFOSAdaptiveDistances( RENDER_FLAG eFlag, int nNewValue )
{
	ASSERT( eFlag == RENDER_FLAG_ADAPTIVE_DISTANCES );
	if ( nNewValue == TRUE )
	{
		// reinitialize distance spring on reset of adaptive distances
		e_SetupDistanceSpring();
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

bool e_IsGlowAvailable(const OptionState * pState)
{
	ASSERT_RETVAL( pState, false );
	if ( AppIsTugboat() )
	{
		if ( pState->get_nMaxEffectTarget() >= FXTGT_SM_20_LOW )
			return true;
		return false;
	}
	// AppIsHellgate
	return true;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_UpdateGlobalFlags()
{
	//gbPixelShader = ( sgnMaximumShaderLevel > 0 );

	//if ( !gbPixelShader )
	//{
	//	gpnRenderFlags[ RENDER_FLAG_BLUR_ENABLED  ] = FALSE;
	//	gpnRenderFlags[ RENDER_FLAG_ZPASS_ENABLED ] = FALSE;
	//	gpnRenderFlags[ RENDER_FLAG_FOG_ENABLED   ] = FALSE;
	//}

	if ( e_GetMaxDetail() < FXTGT_SM_20_LOW )
	{
		// CHB 2006.06.21 - Occlusion query not available on 1.1.
		gpnRenderFlags[ RENDER_FLAG_OCCLUSION_QUERY_ENABLED ] = FALSE;
	}

	// CHB 2007.01.01 - Short circuit this and don't bother if we can't do OQ.
	gpnRenderFlags[RENDER_FLAG_OCCLUSION_QUERY_ENABLED] = gpnRenderFlags[RENDER_FLAG_OCCLUSION_QUERY_ENABLED] && dx9_CapGetValue(DX9CAP_OCCLUSION_QUERIES);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int e_GetRenderFlag( RENDER_FLAG eFlag )
{
	ASSERT_RETZERO( eFlag >= 0 && eFlag < NUM_RENDER_FLAGS );
	return gpnRenderFlags[ eFlag ];
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT e_SetRenderFlag( RENDER_FLAG eFlag, int nValue )
{
	ASSERT_RETINVALIDARG( eFlag >= 0 && eFlag < NUM_RENDER_FLAGS );

	// optional intermediate steps
	if ( eFlag == RENDER_FLAG_SHOW_MIP_LEVELS )
	{
		if ( nValue && !e_GetRenderFlag( eFlag ) )
		{
			V( e_CreateDebugMipTextures() );
		}
		else if ( !nValue )
		{
			V( e_ReleaseDebugMipTextures() );
		}
		V( e_SetRenderFlag( RENDER_FLAG_NEUTRAL_LIGHTMAP, nValue ) );
	}

	// debug
	//if ( eFlag == RENDER_FLAG_DBG_RENDER_ENABLED )
	//{
	//	static BOOL bOn = 0;
	//	if ( ! bOn )
	//	{
	//		e_ModelSetAlpha( nValue, 0.5f );
	//		bOn = 1;
	//	} else
	//	{
	//		e_ModelSetAlpha( nValue, 1.f );
	//		bOn = 0;
	//	}
	//}

	// enable an easy toggle for flags taking an ID argument
	if ( gtRenderFlags[ eFlag ].eType == RFTYPE_ID )
	{
		if ( nValue != INVALID_ID && e_GetRenderFlag( eFlag ) == nValue )
		{
			nValue = INVALID_ID;
		}
	}

	if ( eFlag == RENDER_FLAG_MODEL_RENDER_INFO ||
		 eFlag == RENDER_FLAG_TEXTURE_INFO )
	{
		if ( nValue != INVALID_ID )
			DebugKeyHandlerStart( 0 );
		else
			DebugKeyHandlerEnd();
	}

	if ( eFlag == RENDER_FLAG_SHOW_PORTAL_INFO )
	{
		if ( nValue != 0 )
			DebugKeyHandlerStart( 0 );
		else
			DebugKeyHandlerEnd();
	}

	if ( eFlag == RENDER_FLAG_RENDERTARGET_INFO )
	{
		e_SetRenderFlag( RENDER_FLAG_DBG_TEXTURE_ENABLED, ( nValue != INVALID_ID ) );
	}

	//if ( eFlag == RENDER_FLAG_SET_LOD_ENABLED )
	//	if ( ( ! nValue ) && e_GetRenderFlag( eFlag ) )
	//		c_sClearAllTextureLODs();

	if ( gpfn_RenderFlagOnSet[ eFlag ] )
		gpfn_RenderFlagOnSet[ eFlag ]( eFlag, nValue );

	gpnRenderFlags[ eFlag ] = nValue;
	e_UpdateGlobalFlags();

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int e_ToggleRenderFlag( RENDER_FLAG eFlag )
{
	ASSERT_RETZERO( eFlag >= 0 && eFlag < NUM_RENDER_FLAGS );
	switch ( e_GetRenderFlagType( eFlag ) )
	{
	case RFTYPE_BOOL:
		e_SetRenderFlag( eFlag, !e_GetRenderFlag( eFlag ) );
		break;
	case RFTYPE_ID:
		if ( e_GetRenderFlag( eFlag ) != INVALID_ID )
			e_SetRenderFlag( eFlag, INVALID_ID );
		break;
	case RFTYPE_INT:
		e_SetRenderFlag( eFlag, !e_GetRenderFlag( eFlag ) );
		break;
	}
	return e_GetRenderFlag( eFlag );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_SetRenderEnabled( BOOL bEnabled )
{
	gbRenderEnabled = bEnabled;
	trace( "*** Set Render Enabled: %d\n", bEnabled );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_SetUIOnlyRender( BOOL bUIOnly )
{
	gbUIOnlyRender = bUIOnly;
	trace( "*** Set UI-Only Render: %d\n", bUIOnly );
}

//-------------------------------------------------------------------------------------------------
// The horizontal area of the screen covered by Mythos UI panes
//-------------------------------------------------------------------------------------------------

void e_SetUICoverage( float Width,
					  BOOL bCoverAll,
					  BOOL bLeft )
{
	gfUICoverageWidth = Width;
	gbUICovered = bCoverAll;
	gbUICoveredLeft = bLeft;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_SetDebugTrace( BOOL bEnabled )
{
	gbDebugTrace = bEnabled;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL e_ToggleDebugTrace()
{
	e_SetDebugTrace( !gbDebugTrace );
	return gbDebugTrace;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL e_ToggleWireFrame()
{
	e_ToggleRenderFlag( RENDER_FLAG_WIREFRAME );
	e_UpdateGlobalFlags();
	return e_GetRenderFlag( RENDER_FLAG_WIREFRAME );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int e_ToggleCollisionDraw()
{
	int nCur = e_GetRenderFlag( RENDER_FLAG_COLLISION_MESH_DRAW );
	nCur++;
	if ( nCur >= NUM_COLLISION_DRAW_MODES )
		nCur = 0;
	e_SetRenderFlag( RENDER_FLAG_COLLISION_MESH_DRAW, nCur );
	return e_GetRenderFlag( RENDER_FLAG_COLLISION_MESH_DRAW );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL e_ToggleHullDraw()
{
	e_ToggleRenderFlag( RENDER_FLAG_HULL_MESH_DRAW );
	return e_GetRenderFlag( RENDER_FLAG_HULL_MESH_DRAW );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL e_ToggleClipDraw()
{
	e_ToggleRenderFlag( RENDER_FLAG_CLIP_MESH_DRAW );
	return e_GetRenderFlag( RENDER_FLAG_CLIP_MESH_DRAW );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL e_ToggleDebugDraw()
{
	e_ToggleRenderFlag( RENDER_FLAG_DEBUG_MESH_DRAW );
	return e_GetRenderFlag( RENDER_FLAG_DEBUG_MESH_DRAW );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL e_ToggleClipEnable()
{
	e_ToggleRenderFlag( RENDER_FLAG_CLIP_ENABLED );
	return e_GetRenderFlag( RENDER_FLAG_CLIP_ENABLED );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL e_ToggleScissorEnable()
{
	e_ToggleRenderFlag( RENDER_FLAG_SCISSOR_ENABLED );
	return e_GetRenderFlag( RENDER_FLAG_SCISSOR_ENABLED );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BOOL e_GetFogEnabled()
{
	return gbFogEnabled;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void e_SetFogDistances( float fNearDistance, float fFarDistance, BOOL bForce )
{
	if ( !e_GetRenderFlag( RENDER_FLAG_FOG_OVERRIDE ) || bForce )
	{
		gfFogFarDistance  = fFarDistance;
		gfFogNearDistance = fNearDistance;
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void e_GetFogDistances( float & fNearDistance, float & fFarDistance )
{
	fFarDistance  = gfFogFarDistance;
	fNearDistance = gfFogNearDistance;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void e_SetFogMaxDensity( float fPercent )
{
	gfFogMaxDensity = fPercent;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_SetFarClipPlane( float fDistance, BOOL bForce )
{
	if ( !e_GetRenderFlag( RENDER_FLAG_CLIP_OVERRIDE ) || bForce )
		gfFarClipPlane = fDistance;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

float e_GetFarClipPlane()
{
	return gfFarClipPlane;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

float e_GetNearClipPlane()
{
	return gfNearClipPlane;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

DWORD e_GetFogColor()
{
	return gdwFogColor;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL e_ToggleDebugRenderFlag()
{
	return e_ToggleRenderFlag( RENDER_FLAG_DBG_RENDER_ENABLED );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL e_ToggleShadowFlag()
{
	return e_ToggleRenderFlag( RENDER_FLAG_SHADOWS_ENABLED );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL e_ToggleDebugTextureFlag()
{
	return e_ToggleRenderFlag( RENDER_FLAG_DBG_TEXTURE_ENABLED );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_RenderFlagPostCommand( int nFlag )
{
	// optional post-command (console, etc.) settings
	//   (for tasks that shouldn't be performed when any old setrenderflag occurs, eg at startup)

	// and the platform-specific ones:
	e_RenderFlagPostCommandEngine( nFlag );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_StartCheapMode()
{
	gbCheapMode = TRUE;
	//ENVIRONMENT_DEFINITION * pEnvDef = e_GetCurrentEnvironmentDef();
	//e_SetFarClipPlane( e_GetClipDistance( pEnvDef ) );
	//e_SetFogDistances( e_GetFogStartDistance( pEnvDef ), e_GetSilhouetteDistance( pEnvDef ) );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_EndCheapMode()
{
	gbCheapMode = FALSE;
	//ENVIRONMENT_DEFINITION * pEnvDef = e_GetCurrentEnvironmentDef();
	//e_SetFarClipPlane( e_GetClipDistance( pEnvDef ) );
	//e_SetFogDistances( e_GetFogStartDistance( pEnvDef ), e_GetSilhouetteDistance( pEnvDef ) );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_UpdateDistanceSpring( int nBatchCount )
{
	// if the batches are less than some minimum, we must be in some special circumstance -- just exit
	if ( nBatchCount < DISTANCE_SPRING_MIN_BATCH_THRESHOLD )
		return;

	//GLOBAL_DEFINITION * pGlobals = DefinitionGetGlobal();
	ENGINE_OVERRIDES_DEFINITION * pOverrides = e_GetEngineOverridesDefinition();

	if( !pOverrides )
		return;

	if ( pOverrides->dwAdaptiveBatchTarget < DISTANCE_SPRING_MIN_BATCH_THRESHOLD )
		return;

	int nTargetBatchCount = pOverrides->dwAdaptiveBatchTarget;

	// + delta means move view distance out, - delta means move view distance in
	float fDelta = 1.0f - ( (float)nBatchCount / (float)nTargetBatchCount );

	// null zone -- avoid jitter
	if ( fabs( fDelta ) < DISTANCE_SPRING_NULL_ZONE )
		fDelta = 0.0f;

	if ( fDelta > 0.0f )
	{
		fDelta *= gtDistanceSpring.fStrengthPlus;
		fDelta = min( fDelta, gtDistanceSpring.fMaxDeltaPlus );
	}
	else if ( fDelta < 0.0f )
	{
		fDelta *= gtDistanceSpring.fStrengthMinus;
		fDelta = max( fDelta, - gtDistanceSpring.fMaxDeltaMinus );
	}

	float fCoef = gtDistanceSpring.fDistanceCoef + fDelta;

	float fMinCoef = sGetSpringMinCoef();
	gtDistanceSpring.fDistanceCoef = min( 1.0f, max( fCoef, fMinCoef ) );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_SetupDistanceSpring()
{
	gtDistanceSpring.fDistanceCoef		= 1.0f;
	gtDistanceSpring.fStrengthPlus		= 0.05f;	// increase cost rate
	gtDistanceSpring.fStrengthMinus		= 0.15f;	// decrease cost rate
	gtDistanceSpring.fMaxDeltaPlus		= 0.2f;
	gtDistanceSpring.fMaxDeltaMinus		= 0.2f;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

float e_GetDistanceSpringCoef()
{
	float fBias = 1.0f;
	if ( e_InCheapMode() ) // bias distances to cheapen rendering cost
		fBias = 0.5f;
	//GLOBAL_DEFINITION * pGlobals = DefinitionGetGlobal();
	ENGINE_OVERRIDES_DEFINITION * pOverrides = e_GetEngineOverridesDefinition();
	if ( e_GetRenderFlag( RENDER_FLAG_ADAPTIVE_DISTANCES ) && ( ! pOverrides || ( 0 == ( pOverrides->dwFlags & GFX_FLAG_DISABLE_ADAPTIVE_BATCHES ) ) ) )
	{
		float fCoef = gtDistanceSpring.fDistanceCoef * fBias;
		float fMinCoef = sGetSpringMinCoef();
		return min( 1.0f, max( fCoef, fMinCoef ) );
	} else
	{
        return fBias;
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int e_GetRenderFlagCount()
{
	return NUM_RENDER_FLAGS;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

const char * e_GetRenderFlagName( int nFlag )
{
	ASSERT_RETNULL( nFlag >= 0 && nFlag < NUM_RENDER_FLAGS );
	return gtRenderFlags[ nFlag ].szName;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

RENDERFLAG_TYPE e_GetRenderFlagType( RENDER_FLAG eFlag )
{
	ASSERT_RETVAL( eFlag >= 0 && eFlag < NUM_RENDER_FLAGS, RFTYPE_INVALID );
	return gtRenderFlags[ eFlag ].eType;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_ResetClipAndFog()
{
	ENVIRONMENT_DEFINITION * pEnvDef = e_GetCurrentEnvironmentDef();
	if ( pEnvDef )
	{
		e_SetFarClipPlane( e_GetClipDistance( pEnvDef ) );
		e_SetFogDistances( e_GetFogStartDistance( pEnvDef ), e_GetSilhouetteDistance( pEnvDef ) );
		e_SetFogColor( e_GetFogColorFromEnv( pEnvDef ) );

		e_SetFogEnabled( e_GetRenderFlag( RENDER_FLAG_FOG_ENABLED ) );
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

bool e_IsFullscreen()
{
	return !e_OptionState_GetActive()->get_bWindowed();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

int e_GetWindowWidth()
{
	return e_OptionState_GetActive()->get_nFrameBufferWidth();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

int e_GetWindowHeight()
{
	return e_OptionState_GetActive()->get_nFrameBufferHeight();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static int s_nDesktopWidth = 0;
static int s_nDesktopHeight = 0;

void e_SetDesktopSize(int nWidth, int nHeight)
{
	s_nDesktopWidth = nWidth;
	s_nDesktopHeight = nHeight;
}

void e_SetDesktopSize(void)
{
	e_SetDesktopSize(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
}

void e_GetDesktopSize( int & nWidth, int & nHeight )
{
	// Use cached values because the system metrics report
	// the screen dimensions, not the desktop dimensions
	// (and in full screen mode we alter the screen
	// dimensions).
	nWidth = s_nDesktopWidth;
	nHeight = s_nDesktopHeight;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

bool e_ResolutionShouldEnumerate( const E_SCREEN_DISPLAY_MODE & tMode )
{
	return !!( tMode.Height >= SCREEN_ENUMERATE_MINIMUM_VERTICAL_SIZE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
EXCEL_STRUCT_DEF(RENDERFLAG_DEFINITION)
	EF_STRING(	"Command",									szName																				)
	EXCEL_SET_INDEX(0, EXCEL_INDEX_WARN_DUPLICATE, "Command")
EXCEL_STRUCT_END
EXCEL_TABLE_DEF(DATATABLE_RENDER_FLAGS, RENDERFLAG_DEFINITION, APP_GAME_ALL, EXCEL_TABLE_SHARED, "renderflags") 
	EXCEL_TABLE_POST_CREATE(gtRenderFlags)
EXCEL_TABLE_END
