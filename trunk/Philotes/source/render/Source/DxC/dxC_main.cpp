//----------------------------------------------------------------------------
// dx9_main.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"

#include "dxC_state.h"
#include "dxC_device_priv.h"
#ifdef HAVOKFX_ENABLED
	#include "dxC_HavokFX.h"
#endif	
#include "dxC_light.h"
#include "dxC_target.h"
#include "dxC_model.h"

#ifdef ENGINE_TARGET_DX9
#include "dxC_effect.h"

#include "dxC_state_defaults.h"

#include "dxC_texture.h"
#include "dx9_settings.h"
#include "dxC_environment.h"

#include "dxC_shadow.h"

#include "dxC_query.h"
#include "dxC_irradiance.h"

#include "dxC_caps.h"
#include "dxC_render.h"
#include "dxC_shadow.h"
#include "dxC_particle.h"

#include "dxC_primitive.h"
#include "dxC_occlusion.h"

#include "dxC_portal.h"

#include "dxC_renderlist.h"
#endif

#ifdef ENGINE_TARGET_DX10
	#include "dx10_device.h"
#endif

#include "camera_priv.h"
#include "perfhier.h"
#include "appcommontimer.h"
#include "spring.h"
#include "e_frustum.h"
#include "matrix.h"

#include "e_main.h"
#include "e_settings.h"
#include "e_portal.h"
#include "e_shadow.h"
#include "e_screenshot.h"
#include "e_debug.h"
#include "e_renderlist.h"

#include "dxC_texture.h"
#include "dxC_profile.h"
#include "dxC_drawlist.h"
#include "dxC_2d.h"
#include "dxC_target.h"
#include "dxC_state.h"
#include "dxC_state_defaults.h"
#include "dxC_primitive.h"
#include "dxC_screenshot.h"
#include "dxC_environment.h"
#include "dxC_hdrange.h"
#include "dxC_viewer.h"
#include "dxC_meshlist.h"
#include "dxC_rchandlers.h"

#include "dxC_render.h"

#include "dxC_main.h"
#include "dxC_caps.h"
#include "dxC_query.h"

#include "e_budget.h"
#include "e_dpvs_priv.h"
#include "dxC_movie.h"
#include "dxC_automap.h"

#include "chb_timing.h"
#include "e_pfprof.h"

#include "game.h"	// CHB 2007.02.01 - For use by sUpdateLevelVisibilityPosition()
#include "level.h"	// CHB 2007.02.01 - For use by sUpdateLevelVisibilityPosition()

#include "e_optionstate.h"	// CHB 2007.03.05
#include "e_gamma.h"		// CHB 2007.10.02

#include "dx9_shadowtypeconstants.h"
#include "e_demolevel.h"
#include "dxC_pixo.h"

using namespace FSSE;

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define DEVICE_LOST_SLEEP_MS	500

// CHB 2006.07.11 - Define this to enable VTune triggers.
//#define CHB_ENABLE_VTUNE_TRIGGERS 1
#ifdef CHB_ENABLE_VTUNE_TRIGGERS
#pragma comment(lib,"VTTriggers.lib")
#include "../../../../3rd Party/VTune/Include/VTTriggers.h"
static HANDLE hVTuneTrigger = 0;
#endif


//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

BOOL  gbBlurInitialized = FALSE;

BOOL  gbWantFullscreen = FALSE;

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

static PRESULT sRenderFrameUIOnly();
static PRESULT sRenderFrame( Viewer * pViewer, DWORD dwFlags );
//static PRESULT sRenderFrameCustomViewer( int nViewerID );

//----------------------------------------------------------------------------
// IMPLEMENTATIONS
//----------------------------------------------------------------------------

PRESULT e_PlatformInit()
{
#ifdef CHB_ENABLE_VTUNE_TRIGGERS
	{
		HANDLE hHandle = 0;
		DWORD nResult = VT_RegisterTrigger(_T("CHB - CPU Frame End"), 10, _T("Frames"), &hHandle);
		if (nResult == 0) {
			// Success.
			hVTuneTrigger = hHandle;
		}
	}
#endif
	V_RETURN( e_InitializeStorage() );

	DX9_BLOCK( return dx9_Init(); )
	DX10_BLOCK( return dx10_Init( ) );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_Destroy()
{
	V( e_DPVS_DebugDumpData() );

	V( dxC_DestroyDeviceObjects() );
	V( e_DestroyStorage() );
	V( e_DPVS_Destroy() );
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

#if 0
void e_OutputModalMessage( const char * pszText, const char * pszCaption )
{
	MessageBox( AppCommonGetHWnd(), pszText, pszCaption, MB_OK | MB_ICONEXCLAMATION );	// CHB 2007.08.01 - String audit: unused
}
#endif

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
DWORD e_GetBackgroundColor( ENVIRONMENT_DEFINITION * pEnvDefOverride /*= FALSE*/ )
{
	DWORD dwClearColor;
	if ( e_GetRenderFlag( RENDER_FLAG_WIREFRAME ) )
	{
		dwClearColor = ARGB_MAKE_FROM_INT( 190, 190, 190, 0 );
	} else
	{
		if ( e_GetRenderFlag( RENDER_FLAG_SHOW_OVERDRAW ) )
		{
			dwClearColor = 0;
		} else {
			ENVIRONMENT_DEFINITION * pEnvDef = e_GetCurrentEnvironmentDef();
			dwClearColor = e_EnvGetBackgroundColor( pEnvDef );
		}
	}
	return dwClearColor;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT sClearToBackgroundColor()
{
	DWORD dwClearColor = e_GetBackgroundColor();
	V_RETURN( dxC_ClearBackBufferPrimaryZ(D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, dwClearColor, 1.0f, 0 ) );
	return S_OK;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void e_InitMatrices( 
	MATRIX *pmatWorld, 
	MATRIX *pmatView, 
	MATRIX *pmatProj, 
	MATRIX *pmatProj2 /*= NULL*/, 
	VECTOR *pvEyeVector /*= NULL*/, 
	VECTOR *pvEyeLocation /*= NULL*/,
	VECTOR *pvEyeLookat /*= NULL*/,
	BOOL bAllowStaleData /*= FALSE*/,
	const struct CAMERA_INFO * pCameraInfo /*= NULL*/ )
{
	if ( pmatWorld )
	{
		MatrixIdentity( pmatWorld );
	}

	if ( ! pCameraInfo )
		pCameraInfo = CameraGetInfo();

	ASSERT_RETURN( pCameraInfo );

    // Set up our view matrix. Can be specified, or default as from player view
	{
		if ( pCameraInfo )
		{
			if ( pmatView )
			{
				CameraGetViewMatrix( pCameraInfo, (MATRIX*)pmatView, bAllowStaleData );
			}

			if (pvEyeVector)
			{
				*pvEyeVector	= (D3DXVECTOR3 &)pCameraInfo->vDirection;
			}
			if (pvEyeLocation)
			{
				*pvEyeLocation	= (D3DXVECTOR3 &)pCameraInfo->vPosition;
			}
			if (pvEyeLookat)
			{
				*pvEyeLookat	= (D3DXVECTOR3 &)pCameraInfo->vLookAt;
			}
		} else
		{
			if ( pmatView )
				MatrixIdentity( pmatView );
			if ( pvEyeVector )
				*pvEyeVector = VECTOR(0,1,0);
			if ( pvEyeLocation )
				*pvEyeLocation = VECTOR(0,0,0);
			if ( pvEyeLookat )
				*pvEyeLookat = VECTOR(0,0,0);
		}

		e_GetCurrentViewOverrides( pmatView, pvEyeLocation, pvEyeVector, pvEyeLookat );
	}

	BOUNDING_BOX* pViewportOverride = e_GetViewportOverride();

    // The projection matrix
	if ( pmatProj && pCameraInfo )
	{
		CameraGetProjMatrix( pCameraInfo, (MATRIX*)pmatProj, bAllowStaleData, pViewportOverride );
		if ( ! pViewportOverride )
			e_ScreenShotGetProjectionOverride( (MATRIX*)pmatProj, pCameraInfo->fVerticalFOV, e_GetNearClipPlane(), e_GetFarClipPlane() );
	} else if ( pmatProj )
	{
		MatrixIdentity( pmatProj );
	}


	if ( pmatProj2 && pCameraInfo )
	{
		CAMERA_INFO tCamInfo = *pCameraInfo;
		tCamInfo.fVerticalFOV = DEG_TO_RAD(33.75f); // Horizontal was:  ( PI / 4.0f ); (45 degrees)
		CameraGetProjMatrix( &tCamInfo, (MATRIX*)pmatProj2, bAllowStaleData, pViewportOverride );
		if ( ! pViewportOverride )
			e_ScreenShotGetProjectionOverride( (MATRIX*)pmatProj2, tCamInfo.fVerticalFOV, e_GetNearClipPlane(), e_GetFarClipPlane() );
	} else if ( pmatProj2 )
	{
		MatrixIdentity( pmatProj2 );
	}


	if ( e_GeneratingCubeMap() && pCameraInfo && pmatView && pmatProj )
	{
		// if we're generating a cube map, override the matrices based on the current face
		VECTOR vEyeVector		= pCameraInfo->vDirection;
		VECTOR vEyeLocation		= pCameraInfo->vPosition;
		e_SetupCubeMapMatrices( pmatView, pmatProj, pmatProj2, &vEyeVector, &vEyeLocation );
		if ( pvEyeVector )
			*pvEyeVector = vEyeVector;
	}

	// update view frustum, if it needs it and we have view and proj
	VIEW_FRUSTUM * pFrustum = e_GetCurrentViewFrustum();
	if ( ! pFrustum->IsCurrent() && pmatView && pmatProj )
	{
		pFrustum->Update( pmatView, pmatProj );
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT e_CheckValid()
{
#ifdef ENGINE_TARGET_DX9
	if ( dxC_IsPixomaticActive() )
		return S_OK;

	PRESULT pr;

	V_SAVE( pr, dx9_CheckDeviceLost() );
	if ( pr == D3DERR_DEVICENOTRESET )
	{
		V_DO_FAILED( dx9_ResetDevice() )
		{
			ASSERT_MSG( "Failed to restore device!" );
			return E_FAIL;
		}
	}
	if ( pr == D3DERR_DEVICELOST )
	{
		//Sleep( DEVICE_LOST_SLEEP_MS );

		return S_FALSE;
	}
#endif // DX9

	//No lost device in DX10
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT e_Update()
{
	if ( ! e_GetRenderEnabled() )
		return S_FALSE;

	e_DebugShowInfo();

	e_UpdateElapsedTime();

	V( dxC_GPUTimerUpdate() );
	V( dxC_CPUTimerUpdate() );

	e_DemoLevelGatherPreFrameStats();

	const CAMERA_INFO * pCameraInfo = CameraGetInfo();

	V( dxC_MovieUpdate() );

	DX9_BLOCK  //KMNV TODO: Need dx10
	(
		dx9_DumpQueryReportNewFrame();
	)

	if ( e_GetUIOnlyRender() || ( AppIsTugboat() && e_GetUICovered() ) || !pCameraInfo )
		return S_FALSE;

	TRACE_EVENT( "Update Begin" );

	//V( e_ViewerUpdateCamera( gnMainViewerID, pCameraInfo ) );

	e_SetLevelLocationRenderFlags();
	e_UpdateEnvironmentTransition();

	V( gtScreenEffects.Update() );

	extern PFN_TOOLNOTIFY gpfn_ToolUpdate;
	if ( c_GetToolMode() )
	{
		if ( gpfn_ToolUpdate )
			gpfn_ToolUpdate();
	}

	// CHB 2007.02.17
	V( e_UpdateParticleLighting() );

	// select shadow map lights and update shadow map parameters
	if ( e_UseShadowMaps() )
	{
		V( e_UpdateShadowBuffers() );
	}

	e_ParticleDrawUpdateStats();

	if ( e_GeneratingCubeMap() )
	{
		return S_FALSE;
	}

	// dumps all memory usage (through our allocator) to the perf log
	//MemoryTrace( NULL );

	if ( e_GetDrawAllLights() )
	{
		e_DebugDrawLights();
	}


	//static SPRING tSpring = {0};
	////static LERP_SPRING tSpring = {0};

	//SpringSetTarget( &tSpring, (float)e_GetRenderFlag( RENDER_FLAG_DBG_RENDER_ENABLED ) );
	//SpringSetStrength( &tSpring, 0.01f );
	//SpringSetDampen( &tSpring, 0.99f );
	//SpringSetNullZone( &tSpring, 20.f );
	//SpringUpdate( &tSpring, e_GetElapsedTime() );

	//e_SpringDebugDisplay( &tSpring, 0.5f );

	dxC_RenderTargetsMarkFrame();

	VECTOR vEye, vLookat;
	e_InitMatrices( NULL, NULL, NULL, NULL, NULL, &vEye, &vLookat );
	e_PortalsUpdateVisible( vEye, vLookat );
	//e_PortalDebugRenderAll();
	e_DebugRenderPortals();

	//e_DPVS_Update();
	//if ( e_GetRenderFlag( RENDER_FLAG_DPVS_ENABLED ) )
	//{
	//	e_RenderListBegin();
	//	e_DPVS_MakeRenderList();
	//	e_RenderListEnd();
	//}

	TRACE_EVENT( "Update End" );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT e_PostRender()
{
	TRACE_EVENT( "Post Render Begin" );

	// TRAVIS: I seem to need to do this in Mythos even if render isn't enabled ( app inactive),
	// since we don't
	// pause after focus is lost. This seems to be cleaning up some stuff
	// that needs to get cleaned up -
	// so far, no adverse effects, and the intermittent crazy memory leak/performance
	// hit when ALT-Tabbed out of Mythos SEEMS to go away.
	// please smack me if this was a stupid thing to do.
	//if ( ! e_GetRenderEnabled() )
	//	return S_FALSE;

	V( e_PrimitiveInfoClear() );
	V( e_ReaperUpdate() );
	e_UpdateVisibilityToken();
	e_UploadManagerGet().NewFrame();
	e_AdjustScreenShotQueue();

	e_DemoLevelGatherPostFrameStats();

	TRACE_EVENT( "Post Render End" );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static PRESULT sRenderTextureSprite( int nScale, LPD3DCSHADERRESOURCEVIEW lpTexture, D3DC_TEXTURE2D_DESC & tDesc )
{
	SPD3DCSPRITE pSprite;
	DX9_BLOCK
	(
		V_RETURN( D3DXCreateSprite( dxC_GetDevice(), &pSprite ) );
	)
	DX10_BLOCK
	(
		V_RETURN( D3DX10CreateSprite( dxC_GetDevice(), NULL, &pSprite) );  //KMNV: May want to size this better
	)
	V_RETURN( pSprite->Begin( 0 ) );

	BOOL bAlphaBlend = !! e_GetRenderFlag( RENDER_FLAG_TEXTURE_VIEW_ALPHA_BLEND );
	BOOL bAlphaTest = !! e_GetRenderFlag( RENDER_FLAG_TEXTURE_VIEW_ALPHA_TEST );
	BOOL bViewAlpha = !! e_GetRenderFlag( RENDER_FLAG_TEXTURE_VIEW_ALPHA );


	if ( bViewAlpha )
	{
		// This is set up to multiply the texture RGB times black and then add the texture alpha
		// to that color, so that we can display the alpha channel.

#ifdef ENGINE_TARGET_DX9

		int nStage = 0;


		dx9_SetTextureStageState( nStage, D3DTSS_CONSTANT, 0xffffffff );
		dx9_SetTextureStageState( nStage, D3DTSS_COLORARG1, D3DTA_CONSTANT );
		dx9_SetTextureStageState( nStage, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
		dx9_SetTextureStageState( nStage, D3DTSS_COLOROP, D3DTOP_SELECTARG1 );
		dx9_SetTextureStageState( nStage, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1 );
		nStage++;


		dx9_SetTextureStageState( nStage, D3DTSS_CONSTANT, 0xff000000 );
		dx9_SetTextureStageState( nStage, D3DTSS_COLORARG1, D3DTA_CURRENT );		// result from previous
		dx9_SetTextureStageState( nStage, D3DTSS_COLORARG2, D3DTA_CONSTANT );
		dx9_SetTextureStageState( nStage, D3DTSS_ALPHAARG1, D3DTA_CURRENT );		// result from previous
		dx9_SetTextureStageState( nStage, D3DTSS_ALPHAARG2, D3DTA_CONSTANT );
		dx9_SetTextureStageState( nStage, D3DTSS_COLOROP, D3DTOP_BLENDCURRENTALPHA );
		dx9_SetTextureStageState( nStage, D3DTSS_ALPHAOP, D3DTOP_SELECTARG2 );
		nStage++;

		dx9_SetTextureStageState( nStage, D3DTSS_COLOROP, D3DTOP_DISABLE );
		dx9_SetTextureStageState( nStage, D3DTSS_ALPHAOP, D3DTOP_DISABLE );

#endif // DX9

		bAlphaBlend = FALSE;
		bAlphaTest = FALSE;
	}


	dxC_SetRenderState( D3DRS_ALPHABLENDENABLE, bAlphaBlend );
	dxC_SetRenderState( D3DRS_ALPHATESTENABLE, bAlphaTest );
	if ( bAlphaBlend )
	{
		dxC_SetRenderState( D3DRS_BLENDOP, D3DBLENDOP_ADD );
		dxC_SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
		dxC_SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
	}



	// since this is used for texture debugging, set for point filtering
#if defined(ENGINE_TARGET_DX9)
	V_RETURN( dx9_SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_POINT ) );
	V_RETURN( dx9_SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_POINT ) );
	V_RETURN( dx9_SetSamplerState( 0, D3DSAMP_MIPFILTER, D3DTEXF_POINT ) );
#endif

	MATRIX mScale;
	if ( nScale == DEBUGTEX_SCALE_FIT )
	{
		int nWidth, nHeight;
		V( dxC_GetRenderTargetDimensions( nWidth, nHeight ) );
		float fScale = min( (float)nWidth / tDesc.Width, (float)nHeight / tDesc.Height );
		MatrixScale( &mScale, fScale );
	} else if ( nScale == DEBUGTEX_SCALE_ACTUAL )
	{
		MatrixIdentity( &mScale );
	} else
	{
		int nFactor = 1 << nScale;
		float fScale = 1.f / nFactor;
		MatrixScale( &mScale, fScale );
	}

#ifdef ENGINE_TARGET_DX9

	V( pSprite->SetTransform( (D3DXMATRIX*)&mScale ) );
	V( pSprite->Draw( lpTexture, NULL, NULL, NULL, 0xffffffff ) );

#elif defined(ENGINE_TARGET_DX10)

	int nWidth, nHeight;
	V( dxC_GetRenderTargetDimensions( nWidth, nHeight ) );

	D3DXMATRIXA16 matProjection;
	D3DXMatrixOrthoOffCenterLH(&matProjection, 0, nWidth, 0, nHeight, 0.1f, 10);
	//V( pSprite->SetProjectionTransform( &matProjection ) );

	MATRIX mIdentity;
	MatrixIdentity( &mIdentity );
	//V( pSprite->SetViewTransform( (D3DXMATRIX*)&mIdentity ) );
	//V( pSprite->SetProjectionTransform( (D3DXMATRIX*)&mIdentity ) );

	D3DXMATRIXA16 matTranslation;
	D3DXMATRIXA16 matScaling;
	float scaleX = (float)nWidth / tDesc.Width;
	float scaleY = (float)nHeight / tDesc.Height;
	D3DXMatrixScaling( &matScaling, scaleX, scaleY, 1.0f );
	D3DXMatrixTranslation( &matTranslation, 0, nHeight, 0.1f);

	D3DX10_SPRITE sprite;
	sprite.ColorModulate = 0xffffffff;
	sprite.pTexture = lpTexture;
	sprite.matWorld = (D3DXMATRIX)mScale;
	//sprite.matWorld = matScaling*matTranslation;
	sprite.TextureIndex = 0;
	sprite.TexCoord = D3DXVECTOR2( 0.f, 0.f );
	sprite.TexSize = D3DXVECTOR2( 1.f, 1.f );
	V( pSprite->DrawSpritesImmediate( &sprite, 1, 0, 0));  //KMNV TODO: Anything more to set up  here?

	// unbind the input layout and vertex buffer
	dxC_GetDevice()->IASetInputLayout( NULL );
	//dxC_GetDevice()->IASetVertexBuffers( 0, 0, NULL, NULL, NULL );

#endif

	//if ( gpDebugTexture )
	//{
	//	HRVERIFY( pSprite->Draw( gpDebugTexture, NULL, NULL, NULL, 0xffffffff ) );
	//} else {
	//	HRVERIFY( pSprite->Draw( dxC_RTShaderResourceGet( RENDER_TARGET_CANVAS1 ), NULL, NULL, NULL, 0xffffffff ) );
	//}
	V( pSprite->End() );

	return S_OK;
}

static void sRenderTextureInfo()
{
	int nMode = e_GetRenderFlag( RENDER_FLAG_TEXTURE_VIEW_SCALE );
	int nTexture = e_GetRenderFlag( RENDER_FLAG_TEXTURE_INFO );
	if ( nTexture != INVALID_ID )
	{
		D3DC_TEXTURE2D_DESC tDesc;
		D3D_TEXTURE * pTexture = dxC_TextureGet( nTexture );
		if ( !pTexture )
			return;
		LPD3DCSHADERRESOURCEVIEW lpTexture = pTexture->GetShaderResourceView();
		if ( !lpTexture )
			return;
		V( dxC_Get2DTextureDesc( pTexture, (UINT)DebugKeyGetValue(), &tDesc ) );
		V( sRenderTextureSprite( nMode, lpTexture, tDesc ) );
	}
}

static void sRenderDebugTexture()
{
	int nMode = e_GetRenderFlag( RENDER_FLAG_TEXTURE_VIEW_SCALE );
	if ( e_GetRenderFlag( RENDER_FLAG_DBG_TEXTURE_ENABLED ) )
	{
		D3D_PROFILE_REGION( L"Debug Texture" );

		// debug render texture flat
		D3DC_TEXTURE2D_DESC tDesc;
		LPD3DCSHADERRESOURCEVIEW lpDebugTexture = dxC_DebugTextureGetShaderResource();
		if ( !lpDebugTexture )
		{
			//lpDebugTexture = dxC_RTShaderResourceGet( RENDER_TARGET_CANVAS1 );
			//V( dxC_Get2DTextureDesc( dxC_RTResourceGet( RENDER_TARGET_CANVAS1 ), 0, &tDesc ) );
			return;
		}
		else
		{
			DX9_BLOCK( V( dxC_Get2DTextureDesc( dxC_DebugTextureGetShaderResource(), 0, &tDesc ) ); )
			DX10_BLOCK
			(
				// Unfortunately, we can't get a TEXTURE2D_DESC from a SRV,
				// so we'll default the height and width to 1024x1024.
				D3D10_SHADER_RESOURCE_VIEW_DESC tSRVDesc;
				dxC_DebugTextureGetShaderResource()->GetDesc( &tSRVDesc );
				ASSERT( tSRVDesc.ViewDimension == D3D10_SRV_DIMENSION_TEXTURE2D );
				tDesc.Height = 1024;
				tDesc.Width = 1024;
			)
		}

		V( sRenderTextureSprite( nMode, lpDebugTexture, tDesc ) );
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static PRESULT sBeginRender( int nSwapChainIndex = DEFAULT_SWAP_CHAIN )
{
	V( dx9_ResetEffectCache() );
	
	//dx9_DirtyStateCache();

	// clobber all states with known good defaults -- but no force
	dxC_StatQueriesBeginFrame();

	V( dx9_ResetSamplerStates() );

#ifdef ENGINE_TARGET_DX9
	dx9_ResetFrameStateStats();
	V( dx9_RestoreD3DStates() ); 
	if ( dxC_IsPixomaticActive() )
	{
		dxC_PixoResetState();
	}
#endif
	DX10_BLOCK( dx10_RestoreD3DStates(); )

	V( dxC_BeginScene( nSwapChainIndex ) );


	// Try to fix SLI by forcing a clear once per frame
	V( dxC_TargetSetAllDirty() );


	// reset device states
	dxC_SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
	dxC_SetRenderState( D3DRS_ALPHATESTENABLE, FALSE );
	dxC_SetRenderState( D3DRS_ALPHAREF, 1L );
	dxC_SetRenderState( D3DRS_ALPHAFUNC, D3DCMP_GREATEREQUAL );
	dxC_SetRenderState( D3DRS_ZWRITEENABLE, TRUE );
	dxC_SetRenderState( D3DRS_ZENABLE, TRUE );
	dxC_SetRenderState( D3DRS_SCISSORTESTENABLE, FALSE );
	//if ( AppIsTugboat() )
	{
		DEPTH_TARGET_INDEX eDT = dxC_GetSwapChainDepthTargetIndex( dxC_GetCurrentSwapChainIndex(), SWAP_DT_AUTO );
		V( dxC_SetRenderTargetWithDepthStencil( SWAP_RT_BACKBUFFER, eDT ) );
		V( dxC_ResetFullViewport() );
		//dxC_ClearBuffers( TRUE, TRUE, 0x00000000 );
		V( dxC_ClearBackBufferPrimaryZ( D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,  0x00000000 ) );
	}
	V( dxC_ResetViewport() );
	dxC_SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
	dxC_SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW );
	dxC_SetRenderState( D3DRS_STENCILENABLE, FALSE );
	dxC_SetRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA );

	V( dxC_GPUTimerBeginFrame() );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static PRESULT sEndRender()
{
	V( dxC_GPUTimerEndFrame() );

	// End the scene
	V( dxC_EndScene() );

	dx9_StatQueriesEndFrame();
#ifdef DX10_DOF
	dx10_StepDOF();
#endif

	return S_OK;
}

PRESULT e_ParticlesDrawLightmap( MATRIX& matView, MATRIX& matProj, MATRIX& matProj2, VECTOR& vEyeLocation, VECTOR& vEyeVector )
{
	if ( ! e_GetRenderFlag( RENDER_FLAG_LIGHT_PARTICLES_ENABLED ) )
		return S_FALSE;
	extern PFN_PARTICLEDRAW gpfn_ParticlesDrawLightmap;
	ASSERT_RETFAIL( gpfn_ParticlesDrawLightmap );
	return gpfn_ParticlesDrawLightmap( 0, &matView, &matProj, &matProj2, vEyeLocation, vEyeVector, TRUE, FALSE );
}


PRESULT e_ParticlesDrawPreLightmap( MATRIX& matView, MATRIX& matProj, MATRIX& matProj2, VECTOR& vEyeLocation, VECTOR& vEyeVector )
{
	if ( ! e_GetRenderFlag( RENDER_FLAG_LIGHT_PARTICLES_ENABLED ) )
		return S_FALSE;
	extern PFN_PARTICLEDRAW gpfn_ParticlesDrawPreLightmap;
	ASSERT_RETFAIL( gpfn_ParticlesDrawPreLightmap );
	return gpfn_ParticlesDrawPreLightmap( 0, &matView, &matProj, &matProj2, vEyeLocation, vEyeVector, TRUE, FALSE );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static PRESULT sFlushGPUPipeline()
{
#ifdef ENGINE_TARGET_DX9	
	SPD3DCQUERY pSyncQuery;
	V_RETURN( dxC_GetDevice()->CreateQuery( D3DQUERYTYPE_EVENT, &pSyncQuery ) );
	V_RETURN( pSyncQuery->Issue( D3DISSUE_END ) );
	DWORD dwData;
	BOUNDED_WHILE( S_FALSE == pSyncQuery->GetData( &dwData, sizeof(DWORD), D3DGETDATA_FLUSH ), 1000000000 )
		;
#endif
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static bool s_bReestablishFullscreen = false;

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_PreNotifyWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_WINDOWPOSCHANGING:
			// Ignore external attempts to change the window size.
			// This is useful for the following case, for example:
			//	- Set desktop to a smallish size
			//	- In the game options, select a game resolution
			//		that equals the desktop size
			//	- Now set desktop to a larger size
			// Windows will try to resize a full screen window
			// when the desktop size changes.
			if (!e_IsInternalWindowSizeEvent())
			{
				WINDOWPOS * pWP = reinterpret_cast<WINDOWPOS *>(lParam);
				pWP->flags |= SWP_NOSIZE;
			}
			break;

		default:
			break;
	}
}

// Engine implementation details that depend on window messages.
void e_NotifyWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if ( ! e_OptionState_IsInitialized() )
		return;

	switch (uMsg)
	{
		case WM_ACTIVATE:
		{
			bool bActivated = LOWORD(wParam) != WA_INACTIVE;	// matches prime.cpp
			s_bReestablishFullscreen = s_bReestablishFullscreen || bActivated;

			// CHB 2007.08.27 - This is mostly for DX10, which
			// isn't hiding the window when alt-tabbing away
			// from full screen mode.
//#ifdef ENGINE_TARGET_DX10
			// CHB 2007.08.29 - That's bizarre, why does this make a difference on DX9?
			if (e_IsFullscreen())
			{
				::ShowWindow(hWnd, bActivated ? SW_SHOWNOACTIVATE : SW_SHOWMINNOACTIVE);
			}
//#endif

			// CHB 2007.10.01 - Window style now depends on activation state.
			e_UpdateWindowStyle();

			break;
		}

		case WM_DISPLAYCHANGE:
			// CHB 2007.07.26 - Tell the engine about the display change.
			// We may also want to pay attention to the window size &
			// position messages.
			e_NotifyExternalDisplayChange(LOWORD(lParam), HIWORD(lParam));
			break;

		default:
			break;
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void dxC_CheckFullscreenMode(void)
{
	bool bReestablishFullscreen = s_bReestablishFullscreen;
	s_bReestablishFullscreen = false;

#ifdef ENGINE_TARGET_DX10
	V(dx10_CheckFullscreenMode(bReestablishFullscreen));
#endif

	if (e_OptionState_GetActive()->get_bWindowed())
	{
		return;
	}

	if (bReestablishFullscreen)
	{
		V(e_DoWindowSize());
		DX10_BLOCK( V( dx10_ResizePrimaryTargets() ) );
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static PRESULT sRenderBeginCommon( BOOL & bReturn )
{
	bReturn = TRUE;

	// clear stats
	dx9_ResetTextureUseStats();

	if ( e_IsNoRender() )
		return S_FALSE;
	if ( ! e_GetRenderEnabled() )
		return S_OK;
	if ( !dxC_GetDevice() && ! dxC_IsPixomaticActive() )
		return S_FALSE;
	if ( S_OK != e_CheckValid() )
		return S_FALSE;
	if ( AppCommonGetFillPakFile() )
		return S_FALSE;

#if ISVERSION(DEVELOPMENT)
	if ( dxC_DebugShaderProfileRender() )
		return S_OK;
#endif

	// CHB 2007.08.27 - In DX10, acquiring full screen mode will
	// fail if the window is not the active foreground window at
	// the time CreateSwapChain() or SetFullscreenState() is
	// called. Here we poll to restore the full screen mode when
	// possible, or put the application into windowed mode. See
	// the comment at the definition of dx10_CheckFullscreenMode()
	// for a detailed explanation.
	dxC_CheckFullscreenMode();

	TRACE_EVENT( "Render Begin" );
	//keytrace( "Render Begin --------------------------------------------------------------------\n" );

	bReturn = FALSE;
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static PRESULT sRenderEndCommon()
{
	// clear the draw lists in preparation for the next frame
	// this is to let Hammer or any other non-prime code add models to the draw lists prior to D3D_Render
	for ( int i = 0; i < NUM_DRAWLISTS; i++ )
		DL_CLEAR_LIST( i );

	e_GetCurrentViewFrustum()->Dirty();

	TRACE_EVENT( "Render End" );
	//keytrace( "Render End   --------------------------------------------------------------------\n" );

	return S_OK;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static PRESULT sPresent( int nSwapChainIndex = DEFAULT_SWAP_CHAIN )
{
	PERF(DRAW_PRESENT);

	// NVIDIA PerfKit security heartbeat -- must go right before Present
#ifdef NVIDIA_PERFKIT_ENABLE
	NVPerfHeartbeatD3D();
#endif

	if ( dxC_IsPixomaticActive() )
	{
		V_RETURN( dxC_PixoPresent() );
		return S_OK;
	}

	PRESULT pr;
	{
		LPD3DCSWAPCHAIN pSwapChain = dxC_GetD3DSwapChain( nSwapChainIndex );
		ASSERTV_RETFAIL( pSwapChain, "Couldn't find swapchain with index %d!", nSwapChainIndex );

		e_PfProfMark(("Present()"));
		DX9_BLOCK
		(
		V_SAVE( pr, pSwapChain->Present( NULL, NULL, NULL, NULL, 0 ) );
		)
		DX10_BLOCK
		(
			V_SAVE( pr, pSwapChain->Present(0, 0) );
		)
		// pr may hold an error value like D3DERR_LOSTDEVICE; this is normal
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT e_RenderUIOnly( BOOL bForceRender )
{
	BOOL bReturn = FALSE;
	PRESULT pr = sRenderBeginCommon( bReturn );
	if ( bReturn )
		return pr;

	V( e_UpdateFade() );

	V_RETURN( sRenderFrameUIOnly() );
	//if ( e_PresentEnabled() )
	{
		V_RETURN( sPresent() );
	}
	// TRAVIS : we aren't clearing here because this ends up being the loading screen
	// - so we flick to black and hang there during long loads.
	// let's leave the last present visible -
	/*else
	{
		GDI_ClearWindow( AppCommonGetHWnd() );
	}*/

	V_RETURN( sRenderEndCommon() );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

//PRESULT e_RenderViewer( int nViewerID )
//{
//	return E_NOTIMPL;
//
//	BOOL bReturn = FALSE;
//	PRESULT pr = sRenderBeginCommon( bReturn );
//	if ( bReturn )
//		return pr;
//
//	// ---------------------------------------------------------------------------------------------------
//
//	V_RETURN( sRenderFrameCustomViewer( nViewerID ) );
//
//	// ---------------------------------------------------------------------------------------------------
//
//	Viewer * pViewer = NULL;
//	V_RETURN( dxC_ViewerGet( nViewerID, pViewer ) );
//	V_RETURN( sPresent( pViewer->nSwapChainIndex ) );
//
//	// ---------------------------------------------------------------------------------------------------
//
//	V_RETURN( sRenderEndCommon() );
//
//	return S_OK;
//
//}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT e_Render( int nViewerID /*= INVALID_ID*/, BOOL bForceRender /*= FALSE*/, DWORD dwFlags /*= RF_DEFAULT*/ )
{
	if ( nViewerID == INVALID_ID )
		nViewerID = gnMainViewerID;
	Viewer * pViewer = NULL;
	V( dxC_ViewerGet( nViewerID, pViewer ) );
	ASSERT_RETINVALIDARG( pViewer );

	if ( e_GetRenderFlag( RENDER_FLAG_DELAY_FRAME ) > 0 )
	{
		Sleep( e_GetRenderFlag( RENDER_FLAG_DELAY_FRAME ) );
	}

	if ( e_GetUIOnlyRender() && ( dwFlags & RF_ALLOW_UI ) )
	{
		return e_RenderUIOnly( bForceRender );
	}

	//trace( "RENDER NON-UI: " );

	BOOL bReturn = FALSE;
	PRESULT pr = sRenderBeginCommon( bReturn );
	if ( bReturn )
	{
		//trace( "exit after sRenderBeginCommon: %d\n", pr );
		return pr;
	}


	if ( e_GetCurrentEnvironmentDefID() == INVALID_ID && ! bForceRender )
	{
		//trace( "exit with no current env defID\n" );
		return S_OK;
	}

	if ( e_PresentEnabled() && ( dwFlags & RF_ALLOW_FADE ) )
	{
		V( e_UpdateFade() );
	}

	// ---------------------------------------------------------------------------------------------------

	CHB_TimingBegin(CHB_TIMING_CPU);


	BOOL bFrustumCull, bClip, bScissor;
	bFrustumCull   = e_GetRenderFlag( RENDER_FLAG_FRUSTUM_CULL_MODELS );
	bClip          = e_GetRenderFlag( RENDER_FLAG_CLIP_ENABLED );
	bScissor       = e_GetRenderFlag( RENDER_FLAG_SCISSOR_ENABLED );
	int nRenderPasses = 1;

	if ( e_GeneratingCubeMap() && ( dwFlags & RF_ALLOW_GENERATE_CUBEMAP ) )
	{
		// setup cube map settings
		e_SetupCubeMapGeneration();
		nRenderPasses = 6;
	}


	do 
	{
		//trace( "sRenderFrame... " );

		V_RETURN( sRenderFrame( pViewer, dwFlags ) );

		// ---------------------------------------------------------------------------------------------------

		CHB_TimingBegin(CHB_TIMING_TEST);

		SPD3DCQUERY pSyncQuery;
#ifndef CHB_ENABLE_TIMING
		if ( e_GetRenderFlag( RENDER_FLAG_SYNC_TO_GPU ) )
#endif
		{
			D3D_PROFILE_REGION( L"Sync To GPU" );

			// issue event query
#if defined(ENGINE_TARGET_DX9)
				// CHB 2007.02.05 - Allow this to fail silently. Not all hardware supports this query.
				dxC_GetDevice()->CreateQuery( D3DQUERYTYPE_EVENT, &pSyncQuery );
				if (pSyncQuery != 0)
				{
					V( pSyncQuery->Issue( D3DISSUE_END ) );
				}

#elif defined(ENGINE_TARGET_DX10)
				D3D10_QUERY_DESC qDesc;
				qDesc.Query = D3D10_QUERY_EVENT;
				qDesc.MiscFlags = 0;
				V( dxC_GetDevice()->CreateQuery(&qDesc, &pSyncQuery));
				pSyncQuery->Begin();
#endif
			
		}

		CHB_TimingEnd(CHB_TIMING_TEST);

		CHB_TimingEnd(CHB_TIMING_CPU);
		CHB_TimingBegin(CHB_TIMING_SYNC);

		if ( e_GeneratingCubeMap() && ( dwFlags & RF_ALLOW_GENERATE_CUBEMAP ) )
			e_SaveCubeMapFace();

		if ( e_PresentEnabled() )
		{
			//trace( "present... " );
			V_RETURN( sPresent( pViewer->nSwapChainIndex ) );
		}
		else
		{
			// TRAVIS : we aren't clearing here because this ends up being the loading screen
			// - so we flick to black and hang there during long loads.
			// let's leave the last present visible -
			//GDI_ClearWindow( AppCommonGetHWnd() );

			//trace( "present delay... " );
			e_PresentDelayUpdate();
		}

#ifndef CHB_ENABLE_TIMING
		if ( e_GetRenderFlag( RENDER_FLAG_SYNC_TO_GPU ) && pSyncQuery )
#endif
		{
			PERF(DRAW_SYNC);
			// loop on return, forcing flush
			DWORD dwData;
			BOUNDED_WHILE( S_FALSE == pSyncQuery->GetData( &dwData, sizeof(DWORD), D3DGETDATA_FLUSH ), 1000000000 )
				;
		}

		CHB_TimingEnd(CHB_TIMING_SYNC);

		if ( dwFlags & RF_ALLOW_SCREENSHOT )
		{
			if ( e_ScreenShotRepeatFrame() && e_NeedScreenshotCapture( FALSE, nViewerID ) )
				dxC_CaptureScreenshot();

			// Terminating condition
			if ( ! e_ScreenShotRepeatFrame() )
				nRenderPasses--;
		}
		else
		{
			nRenderPasses--;
		}
	}
	while ( nRenderPasses > 0 );


	if ( e_GeneratingCubeMap() && ( dwFlags & RF_ALLOW_GENERATE_CUBEMAP ) )
	{
		// save cube map
		e_SaveFullCubeMap();

		e_SetRenderFlag( RENDER_FLAG_FRUSTUM_CULL_MODELS,		bFrustumCull   );
		e_SetRenderFlag( RENDER_FLAG_CLIP_ENABLED,				bClip		   );
		e_SetRenderFlag( RENDER_FLAG_SCISSOR_ENABLED,			bScissor	   );
	}

	CHB_TimingEnd(CHB_TIMING_FRAME);

	CHB_TimingEndFrame();
	CHB_StatsEndFrame();
	CHB_StatsBeginFrame();

	CHB_TimingBegin(CHB_TIMING_FRAME);

	//if ( gbDumpDrawList )
	//{
	//	V( dx9_DumpDrawListBuffer() );
	//}

	DX9_BLOCK( dx9_DumpQueryEndFrame(); )  //KMNV: TODO Need to handle queries in dx10

	//e_UpdateDistanceSpring( gtBatchInfo.nTotalBatches );

	V_RETURN( sRenderEndCommon() );


	if ( c_GetToolMode() && ( dwFlags & RF_ALLOW_TOOL_RENDER ) )
	{
		extern PFN_TOOLNOTIFY gpfn_ToolRender;
		if ( gpfn_ToolRender )
			gpfn_ToolRender();
	}

	//trace( "done.\n" );

	return S_OK;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static PRESULT sRenderFrameUIOnly()
{
	// all profile regions need to not span Present() -- otherwise PIX chokes
	D3D_PROFILE_REGION( L"D3D UI-Only Render" );

	V_RETURN( sBeginRender() );

	if ( dxC_IsPixomaticActive() )
	{
		// Call this to commit any backbuffer scaling before rendering pixel-accurate items
		V_RETURN( dxC_PixoBeginPixelAccurateScene() );
	}

	// turn off wireframe
	dxC_SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
	dxC_SetRenderState( D3DRS_SCISSORTESTENABLE, FALSE );

	// turn off fog
	e_SetFogEnabled( FALSE );

	// in case we're showing a movie, tell it to draw now
	V( dxC_MovieRender() );

	dxC_SetRenderState( D3DRS_ZENABLE, TRUE );
	dxC_SetRenderState( D3DRS_ZWRITEENABLE, TRUE );

	V( dxC_ResetFullViewport() );

	if ( ! e_ScreenShotRepeatFrame() && ! e_GeneratingCubeMap() && e_GetRenderFlag( RENDER_FLAG_RENDER_UI ) )
	{
		PERF(DRAW_UI);
		V( e_UIX_Render() );
	}

	// if a screenshot with UI is requested, capture now
	if ( ! e_ScreenShotRepeatFrame() && e_NeedScreenshotCapture( TRUE, gnMainViewerID ) && ! e_GeneratingCubeMap() )
	{
		dxC_CaptureScreenshot();
	}


	V( e_PrimitiveInfoClear() );


	// if a fade is in progress
	DWORD dwFadeColor;
	if ( S_OK == e_GetFadeColor( dwFadeColor ) )
	{
		DL_CLEAR_LIST( DRAWLIST_PRIMARY );
		dx9_DrawListAddCommand( DRAWLIST_PRIMARY, DLCOM_DRAW_LAYER,	LAYER_FADE, dwFadeColor );
		V( dx9_RenderDrawList( DRAWLIST_PRIMARY ) );
		DL_CLEAR_LIST( DRAWLIST_PRIMARY );
	}

	e_RenderManualGamma();	// CHB 2007.10.02

	V_RETURN( sEndRender() );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static PRESULT sRenderFrame( Viewer * pViewer, DWORD dwFlags )
{
	ASSERT_RETFAIL( pViewer );

	// all profile regions need to not span Present() -- otherwise PIX chokes
	D3D_PROFILE_REGION( L"D3D Render" );

	// Don't allow screeneffects if we're rendering to RT_FULL instead of the backbuffer
	if ( e_GetRenderFlag( RENDER_FLAG_RENDER_TO_FULLRT ) & MAKE_MASK(pViewer->nSwapChainIndex) )
		dwFlags &= ~(RF_ALLOW_SCREENEFFECTS);

	V_RETURN( sBeginRender( pViewer->nSwapChainIndex ) );

	BOOL bRenderNonUI = FALSE;

	if ( AppIsTugboat() )
	{
	    if ( ! e_GetUIOnlyRender() &&
		     ! e_GetUICovered() )
			bRenderNonUI = TRUE;
	} else
	{
		if ( ! e_GetUIOnlyRender() )
			bRenderNonUI = TRUE;
	}

	if ( bRenderNonUI )
	{
		if ( dwFlags & RF_ALLOW_AUTOMAP_UPDATE )
		{
			// Update the current automap, including re-rendering the intermediate automap texture if needs be.
			REGION * pRegion = e_GetCurrentRegion();
			V( e_AutomapRenderGenerate( pRegion->nAutomapID ) );
		}


#ifdef ENABLE_VIEWER_RENDER
		if ( e_GetRenderFlag( RENDER_FLAG_USE_VIEWER ) )
		{
			// CML HACK: until I get viewer type shadow working
			BOOL bRenderToSM = FALSE;
			if ( AppIsTugboat() )
			{
				if ( e_UseShadowMaps() && e_ShadowsActive() && ! e_GetUIOnlyRender() && !e_GetUICovered() )
					bRenderToSM = TRUE;
			} else
			{
				if ( e_UseShadowMaps() && e_ShadowsActive() && ! e_GetUIOnlyRender() )
					bRenderToSM = TRUE;
			}

			if ( bRenderToSM )
			{
				PERF(DRAW_SHADOWS);
				V( dx9_RenderToShadowMaps() );
			}
			e_SetFogEnabled( FALSE );
			V(dx9_RenderToParticleLightingMap());	// CHB 2007.02.17
			e_SetFogEnabled( e_GetRenderFlag( RENDER_FLAG_FOG_ENABLED ) );




			//	per viewer
			PRESULT pr = E_FAIL;
			V_SAVE( pr, dxC_ViewerRender( pViewer->nID ) );
			if ( pr == S_FALSE )
				goto end_render;
			ASSERT_GOTO( SUCCEEDED(pr), end_render );
		}
		else
#endif // ENABLE_VIEWER_RENDER
		{
			{
				// this gets moved to an intermediate step
				// after sorting, the uber-passes (normal, portal) are broken up into
				// sections - begin portal, end portal, opaque model, bg skybox, alpha model, am skybox, etc.
				e_RenderListSortPasses();
				// e_RenderListBuildSections();
			}

			{
				if( AppIsTugboat() )
				{
					V( dxC_ClearBackBufferPrimaryZ( D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER,  0x00000000 ) );
				}
				e_RenderListCommit();
			}
		}
	}

	// ---------------------------------------------------------------------------------------------------

	dxC_SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
	dxC_SetRenderState( D3DRS_SCISSORTESTENABLE, FALSE );
	if( AppIsTugboat() ) // tug has the half-scsreen viewports, which need to be set up before these screen effects will render properly
	{
		V( dxC_ResetViewport() );
	}
	if ( bRenderNonUI && ( dwFlags & RF_ALLOW_SCREENEFFECTS ) )
	{
		V( gtScreenEffects.Render( CScreenEffects::TYPE_FULLSCREEN, CScreenEffects::LAYER_PRE_BLOOM ) );
	}

	// ---------------------------------------------------------------------------------------------------

	HDR_MODE eHDR = e_GetValidatedHDRMode();

	if ( eHDR == HDR_MODE_LINEAR_FRAMEBUFFER )
	{
		// in HDR mode, measure luminance
		V( Luminance::MeasureLuminance() );

		// post-process

		// apply luminance
		V( HDRPipeline::ApplyLuminance() );
	}
	else
	{
		// in LDR mode, do normal postprocessing

		if ( bRenderNonUI && ( dwFlags & RF_ALLOW_SCREENEFFECTS ) )
		{
			PERF(DRAW_BLUR);
			// CHB 2007.03.06
			//if ( e_SettingGetBool( ES_GLOW ) && gbBlurInitialized /*&& !e_GeneratingCubeMap()*/ )
			if ( e_OptionState_GetActive()->get_bGlow() && gbBlurInitialized && !e_GetRenderFlag( RENDER_FLAG_WIREFRAME ) && e_GetRenderFlag( RENDER_FLAG_BLUR_ENABLED ) && !e_GeneratingCubeMap() )
			{
				D3D_PROFILE_REGION( L"Blur" );

				// turn off fog
				e_SetFogEnabled( FALSE );

				dxC_SetRenderState( D3DRS_ZENABLE, FALSE );
				dxC_SetRenderState( D3DRS_ZWRITEENABLE, FALSE );

				dxC_SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
				dxC_SetRenderState( D3DRS_ALPHATESTENABLE, FALSE );

				DEPTH_TARGET_INDEX eDepth = DEPTH_TARGET_NONE;

				RENDER_TARGET_INDEX nRTFull			= dxC_GetSwapChainRenderTargetIndex( dxC_GetCurrentSwapChainIndex(), SWAP_RT_FULL );
				RENDER_TARGET_INDEX nRTBlurTargetX	= dxC_GetSwapChainRenderTargetIndex( dxC_GetCurrentSwapChainIndex(), SWAP_RT_BLUR_TARGETX );
				RENDER_TARGET_INDEX nRTBlurTargetY	= dxC_GetSwapChainRenderTargetIndex( dxC_GetCurrentSwapChainIndex(), SWAP_RT_BLUR_TARGETY );
				RENDER_TARGET_INDEX nRTFiltered		= dxC_GetSwapChainRenderTargetIndex( dxC_GetCurrentSwapChainIndex(), SWAP_RT_BLUR_FILTERED );
				RENDER_TARGET_INDEX nRTFinal		= dxC_GetSwapChainRenderTargetIndex( dxC_GetCurrentSwapChainIndex(), SWAP_RT_FINAL_BACKBUFFER );


				// CHB 2007.08.30 - 1.1 uses different glow.
				bool bCheapGlow = dxC_GetCurrentMaxEffectTarget() <= FXTGT_SM_11;
				if (bCheapGlow)
				{
					// This downsamples. On 1.1, CombineLayers.fx1
					// takes care of baking in the alpha component
					// (handled by LAYER_DOWNSAMPLE on other
					// targets).
					//if( AppIsTugboat() )
					{
						dxC_ResetFullViewport();
					}
					dx9_DrawListAddCommand( DRAWLIST_PRIMARY, DLCOM_COPY_BACKBUFFER,			dxC_RTResourceGet( nRTBlurTargetY ) );
				}
				else
				{
					const int nLayerX = DX9_BLOCK( LAYER_GAUSS_S_X ) DX10_BLOCK( LAYER_GAUSS_L_X );
					const int nLayerY = DX9_BLOCK( LAYER_GAUSS_S_Y ) DX10_BLOCK( LAYER_GAUSS_L_Y );

#ifdef ENGINE_TARGET_DX9  //With Dx10 we can texture directly from the bb
					dx9_DrawListAddCommand( DRAWLIST_PRIMARY, DLCOM_COPY_BACKBUFFER,			dxC_RTResourceGet( nRTFull ) );
#endif
					dx9_DrawListAddCommand( DRAWLIST_PRIMARY, DLCOM_SET_RENDER_DEPTH_TARGETS,	nRTFiltered, eDepth );
					DL_CLEAR_COLOR( DRAWLIST_PRIMARY, 0 );
					dx9_DrawListAddCommand( DRAWLIST_PRIMARY, DLCOM_DRAW_LAYER,					LAYER_DOWNSAMPLE, dxC_RTShaderResourceGet( DXC_9_10( nRTFull, nRTFinal ) ) );
					dx9_DrawListAddCommand( DRAWLIST_PRIMARY, DLCOM_SET_RENDER_DEPTH_TARGETS,	nRTBlurTargetX, eDepth );
					DL_CLEAR_COLOR( DRAWLIST_PRIMARY, 0 );
					dx9_DrawListAddCommand( DRAWLIST_PRIMARY, DLCOM_DRAW_LAYER,					nLayerX, dxC_RTShaderResourceGet( nRTFiltered ) );
					dx9_DrawListAddCommand( DRAWLIST_PRIMARY, DLCOM_SET_RENDER_DEPTH_TARGETS,	nRTBlurTargetY, eDepth );
					DL_CLEAR_COLOR( DRAWLIST_PRIMARY, 0 );
					dx9_DrawListAddCommand( DRAWLIST_PRIMARY, DLCOM_DRAW_LAYER,					nLayerY, dxC_RTShaderResourceGet( nRTBlurTargetX ) );
				}
				
				// AE 2007.07.30: I've disabled this for the moment because we are now doing motion blur and DOF. I need to start with the BB rendering to FULL.
#if defined( DX10_DOF ) && 0
				RENDER_TARGET_INDEX nFinal = gtScreenEffects.nFullScreenPasses() > 0 ? nRTFull : nRTFinal;  //If we're going to do some fullscreen screenFX don't render to the backbuffer
#else
				// We need to clear this at least once a frame before using, otherwise SLI suffers.
				dx9_DrawListAddCommand( DRAWLIST_PRIMARY, DLCOM_SET_RENDER_DEPTH_TARGETS,	nRTFull, eDepth );
				DL_CLEAR_COLOR( DRAWLIST_PRIMARY, 0 );

				RENDER_TARGET_INDEX nFinal = nRTFinal;  
#endif
				dx9_DrawListAddCommand( DRAWLIST_PRIMARY, DLCOM_SET_RENDER_DEPTH_TARGETS,	nFinal, eDepth );
				dx9_DrawListAddCommand( DRAWLIST_PRIMARY, DLCOM_DRAW_LAYER,					LAYER_COMBINE, dxC_RTShaderResourceGet( nRTBlurTargetY ) );
				V( dx9_RenderDrawList( DRAWLIST_PRIMARY ) );

				DL_CLEAR_LIST( DRAWLIST_PRIMARY );

				V( dxC_SetSamplerDefaults( SAMPLER_DIFFUSE ) );
				DEPTH_TARGET_INDEX eDT = dxC_GetSwapChainDepthTargetIndex( dxC_GetCurrentSwapChainIndex(), SWAP_DT_AUTO );
				V( dxC_SetRenderTargetWithDepthStencil( SWAP_RT_FINAL_BACKBUFFER, eDT ) );
			} 
		}
	}

	// turn off wireframe
	dxC_SetRenderState( D3DRS_FILLMODE, D3DFILL_SOLID );
	dxC_SetRenderState( D3DRS_SCISSORTESTENABLE, FALSE );

	if( AppIsTugboat() ) // tug has the half-scsreen viewports, which need to be set up before these screen effects will render properly
	{
		V( dxC_ResetViewport() );
	}
	if ( bRenderNonUI && ( dwFlags & RF_ALLOW_SCREENEFFECTS ) )
	{
		V( gtScreenEffects.Render( CScreenEffects::TYPE_FULLSCREEN, CScreenEffects::LAYER_ON_TOP ) );
		V( gtScreenEffects.Render( CScreenEffects::TYPE_PARTICLE,   CScreenEffects::LAYER_ON_TOP ) );
	}

	// turn off fog
	e_SetFogEnabled( FALSE );

	dxC_SetRenderState( D3DRS_ZENABLE, TRUE );
	dxC_SetRenderState( D3DRS_ZWRITEENABLE, TRUE );

	if ( dwFlags & RF_ALLOW_SCREENSHOT )
	{
		// if a screenshot without UI is requested, capture now
		if ( ! e_ScreenShotRepeatFrame() && e_NeedScreenshotCapture( FALSE, pViewer->nID ) && ! e_GeneratingCubeMap() )
		{
			dxC_CaptureScreenshot();
		}
	}

	// before the UI could muck with the zbuffer, render the debug prims that have been queued up
	#ifndef DX10_GET_RUNNING_HACK
	V( e_UpdateDebugPrimitives() );
   	V( e_RenderDebugPrimitives( PRIM_PASS_IN_WORLD ) );
    #endif
	V( dxC_ResetFullViewport() );
	sRenderTextureInfo();

	if ( dxC_IsPixomaticActive() )
	{
		// Call this to commit any backbuffer scaling before rendering pixel-accurate items
		V_RETURN( dxC_PixoBeginPixelAccurateScene() );
	}

	if ( dwFlags & RF_ALLOW_UI )
	{
		if ( ! e_ScreenShotRepeatFrame() && ! e_GeneratingCubeMap() && e_GetRenderFlag( RENDER_FLAG_RENDER_UI ) )
		{
			PERF(DRAW_UI);
			V( e_UIX_Render() );
		}
	}

	sRenderDebugTexture();

	if ( dwFlags & RF_ALLOW_SCREENSHOT )
	{
		// if a screenshot with UI is requested, capture now
		if ( ! e_ScreenShotRepeatFrame() && e_NeedScreenshotCapture( TRUE, pViewer->nID ) && ! e_GeneratingCubeMap() )
		{
			dxC_CaptureScreenshot();
		}
	}

	V( dx9_RenderDebugBoxes() );
	V( e_RenderDebugPrimitives( PRIM_PASS_ON_TOP ) );

	if ( dwFlags & RF_ALLOW_FADE )
	{
		// if a fade is in progress
		DWORD dwFadeColor;
		if ( S_OK == e_GetFadeColor( dwFadeColor ) )
		{
			DL_CLEAR_LIST( DRAWLIST_PRIMARY );
			dx9_DrawListAddCommand( DRAWLIST_PRIMARY, DLCOM_DRAW_LAYER,	LAYER_FADE, dwFadeColor );
			V( dx9_RenderDrawList( DRAWLIST_PRIMARY ) );
			DL_CLEAR_LIST( DRAWLIST_PRIMARY );
		}
	}

	if ( e_GetRenderFlag( RENDER_FLAG_RENDER_TO_FULLRT ) & MAKE_MASK(pViewer->nSwapChainIndex) )
	{
		// Need to copy Full RT to backbuffer
		V( dxC_CopyRTFullMSAAToBackBuffer() );
	}

	if ( dwFlags & RF_ALLOW_SCREENEFFECTS )
	{
		e_RenderManualGamma();	// CHB 2007.10.02
	}

end_render:
	V_RETURN( sEndRender() );

#ifdef CHB_ENABLE_VTUNE_TRIGGERS
	if (hVTuneTrigger != 0) {
		VT_SetTrigger(hVTuneTrigger);
	}
#endif

	if ( e_GetRenderFlag( RENDER_FLAG_USE_VIEWER ) )
	{
		V( dxC_RenderCommandClearLists() );
		V( dxC_MeshListsClear() );
	}

	return S_OK;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sDrawListAddModel(
	int nDrawList,
	int nModelID,
	DWORD dwModelDefFlagsDoDraw,
	DWORD dwModelDefFlagsDontDraw,
	BOOL bOccluded,
	int nCommandType = DLCOM_DRAW,
	BOOL bAllowInvisible = FALSE,
	BOOL bNoDepthTest = FALSE )
{
	int nIsolateModel = e_GetRenderFlag( RENDER_FLAG_ISOLATE_MODEL );
	if ( nIsolateModel != INVALID_ID && nIsolateModel != nModelID )
		return;

	D3D_MODEL* pModel = dx9_ModelGet( nModelID );
	ASSERT_RETURN(pModel);

	// if this model has been debug marked, render visible bounding box around it
	if ( gnDebugIdentifyModel != INVALID_ID && gnDebugIdentifyModel == nModelID && nCommandType == DLCOM_DRAW )
	{
		VECTOR * pOBB = e_GetModelRenderOBBInWorld( nModelID );
		V( e_PrimitiveAddBox( PRIM_FLAG_SOLID_FILL | PRIM_FLAG_WIRE_BORDER, pOBB, 0x00101320 ) );
	}

	if ( ! dx9_ModelShouldRender( pModel ) && ! bAllowInvisible )
		return;
	D3D_MODEL_DEFINITION * pModelDefinition = dxC_ModelDefinitionGet( pModel->nModelDefinitionId, 
													e_ModelGetDisplayLOD( nModelID ) );
	if ( ! pModelDefinition )
		return;
	if ( ! dx9_ModelDefinitionShouldRender( pModelDefinition, pModel, dwModelDefFlagsDoDraw, dwModelDefFlagsDontDraw ) && ! bAllowInvisible )
		return;


	int nCommand = nCommandType;
	if ( bOccluded == TRUE )
		nCommand = DLCOM_DRAW_BOUNDING_BOX;

	if ( bNoDepthTest )
		DL_SET_STATE_DWORD( nDrawList, DLSTATE_NO_DEPTH_TEST, TRUE );
	if ( bAllowInvisible )
		DL_SET_STATE_DWORD( nDrawList, DLSTATE_ALLOW_INVISIBLE_MODELS, TRUE );

	// unsorted
	DWORD dwFlags = 0;
	dx9_AppendModelToDrawList( nDrawList, nModelID, nCommandType, dwFlags );
	
	if ( bNoDepthTest )
		DL_SET_STATE_DWORD( nDrawList, DLSTATE_NO_DEPTH_TEST, FALSE );
	if ( bAllowInvisible )
		DL_SET_STATE_DWORD( nDrawList, DLSTATE_ALLOW_INVISIBLE_MODELS, FALSE );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sDrawListAddModelList(
	int nDrawList,
	const RENDERLIST_PASS * pPass,
	DWORD dwModelDefFlagsDoDraw,
	DWORD dwModelDefFlagsDontDraw,
	int nCommandType,
	BOOL bDrawBehind = FALSE )
{
	ASSERT_RETURN( pPass );

	int nCount = pPass->tModelList.Count();
	for ( int i = 0; i < nCount; i++ )
	{
		const Renderable & tInfo = pPass->tModelList[ i ];
		int nModelID = (int)tInfo.dwData;
		BOOL bOccluded = FALSE;

		D3D_MODEL* pModel = dx9_ModelGet( nModelID );
		if ( ! pModel )
			continue;

		if( ! dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_DRAWBEHIND ) && bDrawBehind )
		{
			continue;
		}

		BOOL bOccludable = ( tInfo.dwFlags & RENDERABLE_FLAG_OCCLUDABLE );
		if ( AppIsTugboat() )
			bOccludable = FALSE;	// no occlusion culling in Tugboat
		if ( bOccludable )
			bOccluded = e_ModelGetOccluded( nModelID );

		// models that aren't in the frustum or are occluded but not occlusion queried shouldn't have gotten here

		//if ( e_GetRenderFlag( RENDER_FLAG_CLIP_ENABLED ) )
		//	dx9_DrawListAddClipPlanes( nDrawList, &pVisRoom->ClipBox );
		if ( AppIsHellgate() )
		{
		if ( e_GetCurrentLevelLocation() == LEVEL_LOCATION_INDOOR || e_GetRenderFlag( RENDER_FLAG_USE_OUTDOOR_CULLING ) )
			dx9_DrawListAddScissorRects( nDrawList, &tInfo.tBBox );
		}

		if ( bOccluded == FALSE )
		{
			if ( bOccludable )
				gtBatchInfo.nRoomsVisible++;

			sDrawListAddModel( nDrawList, nModelID, dwModelDefFlagsDoDraw, dwModelDefFlagsDontDraw, bOccluded, nCommandType, bOccludable );
		} else if ( bOccludable )
		{
			// just render the room's bounding box, no models
			sDrawListAddModel( nDrawList, nModelID, dwModelDefFlagsDoDraw, dwModelDefFlagsDontDraw, bOccluded, DLCOM_DRAW_BOUNDING_BOX, TRUE, TRUE );
			//gtBatchInfo.nRoomOcclusion++;
		}
	}
}

static BOUNDING_BOX gbViewport;

void e_SetViewportOverride( BOUNDING_BOX* pBBox )
{
	if( pBBox )
	{
		gbViewport = *pBBox;
	}
	else
	{
		gbViewport.vMax.fX = 0;
	}
}

BOUNDING_BOX*	e_GetViewportOverride( void )
{
	if( gbViewport.vMax.fX != 0 )
	{
		return &gbViewport;
	}
	else
	{
		return NULL;
	}
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

int e_RenderModelList( RENDERLIST_PASS * pPass )
{
	ASSERTX_RETVAL( 0, 1, "Not implemented!" );

	ASSERT_RETVAL( pPass, 1 );

	{
		// update clip and fog
		V( e_ResetClipAndFog() );

		MATRIX matWorld, matView, matProj, matProj2;
		VECTOR vEyeVector;
		VECTOR vEyeLocation;
		VECTOR vEyeLookat;

		{
			e_InitMatrices( &matWorld, &matView, &matProj, &matProj2, &vEyeVector, &vEyeLocation, &vEyeLookat, FALSE );

			V( dxC_SetTransform( D3DTS_PROJECTION, (D3DXMATRIXA16*)&matProj ) );
		}


		if ( pPass->eType == RL_PASS_NORMAL || pPass->eType == RL_PASS_PORTAL )
		{
			{
				//dx9_RenderToInterfaceTexture( &matView, &matProj, (VECTOR*)&vEyeLocation, TRUE );
			}
			BOOL bRenderToSM = FALSE;
			if ( AppIsTugboat() )
			{
				if ( e_UseShadowMaps() && e_ShadowsActive() && ! e_GetUIOnlyRender() && !e_GetUICovered() )
					bRenderToSM = TRUE;
			} else
			{
				if ( e_UseShadowMaps() && e_ShadowsActive() && ! e_GetUIOnlyRender() )
					bRenderToSM = TRUE;
			}

			if ( bRenderToSM )
			{
				PERF(DRAW_SHADOWS);
				V( dx9_RenderToShadowMaps() );
			}
			if( AppIsTugboat() )
			{
				e_SetFogEnabled( FALSE );
				V(dx9_RenderToParticleLightingMap());	// CHB 2007.02.17
				e_SetFogEnabled( e_GetRenderFlag( RENDER_FLAG_FOG_ENABLED ) );
			}

		}


		{
			// Turn on wireframe if we are in wireframe mode
			BOOL bDrawingWireframe = e_GetRenderFlag( RENDER_FLAG_WIREFRAME );
			dxC_SetRenderState( D3DRS_FILLMODE, e_GetRenderFlag( RENDER_FLAG_WIREFRAME ) ? D3DFILL_WIREFRAME : D3DFILL_SOLID );

			dxC_SetRenderState( D3DRS_SCISSORTESTENABLE, e_GetRenderFlag( RENDER_FLAG_SCISSOR_ENABLED ) );

			//if ( e_GeneratingCubeMap() )
			//	e_CubeMapGenSetViewport();

			DEPTH_TARGET_INDEX eAutoDT = dxC_GetSwapChainDepthTargetIndex( dxC_GetCurrentSwapChainIndex(), SWAP_DT_AUTO );

			if ( pPass->dwFlags & RL_PASS_FLAG_CLEAR_BACKGROUND )
			{
				if ( e_GetValidatedHDRMode() == HDR_MODE_LINEAR_FRAMEBUFFER )
				{
					V( dxC_SetRenderTargetWithDepthStencil( SWAP_RT_FINAL_BACKBUFFER, eAutoDT ) );
					V( dxC_ResetViewport() );
					V( sClearToBackgroundColor() );
					V( dxC_SetRenderTargetWithDepthStencil( SWAP_RT_BACKBUFFER, eAutoDT ) );
					V( dxC_ClearBackBufferPrimaryZ(D3DCLEAR_TARGET, 0, 1.0f, 0 ) );
				} else
				{
					V( dxC_SetRenderTargetWithDepthStencil( SWAP_RT_BACKBUFFER, eAutoDT ) );
					V( dxC_ResetViewport() );
					V( sClearToBackgroundColor() );
				}
			}

//This is all handled in the depth pass now
//#if defined( ENGINE_TARGET_DX10 ) && defined( DX10_USE_MRTS )
//			RENDER_TARGET nRTs[] = { RENDER_TARGET_BACKBUFFER, RENDER_TARGET_DEPTH, RENDER_TARGET_MOTION };
//			V( dxC_SetMultipleRenderTargetsWithDepthStencil( ARRAYSIZE( nRTs ), nRTs, ZBUFFER_AUTO ) );
//			V( dxC_ResetViewport() );
//
//			ID3D10RenderTargetView* pRT[] = { NULL, NULL, NULL };
//			float color[4] = ARGB_DWORD_TO_RGBA_FLOAT4( 0x00000000 );
//			color[ 0 ] = e_GetFarClipPlane();
//			dx10_GetRTsDSWithoutAddingRefs( ARRAYSIZE( nRTs ), pRT, NULL );
//			if ( pRT[ 1 ] )
//				dxC_GetDevice()->ClearRenderTargetView( pRT[ 1 ], color );
//
//			if ( pRT[ 2 ] )
//			{
//				static const float clearColor[ 4 ] = ARGB_DWORD_TO_RGBA_FLOAT4( 0x00000000 );
//				dxC_GetDevice()->ClearRenderTargetView( pRT[ 2 ], clearColor );
//			}
//#else
			V( dxC_SetRenderTargetWithDepthStencil( SWAP_RT_BACKBUFFER, eAutoDT ) );

			// TRAVIS: Now that the particle lightmap render happens before this, we
			// have to set the viewport again after we switch back to backbuffer, for
			// inset screen cases ( like half-screen in Tugboat )
			V( dxC_ResetViewport() );
//#endif

		}

		DL_CLEAR_LIST( DRAWLIST_PRIMARY );

		{
			e_SetFogEnabled( FALSE );

			// Render skybox, if applicable
			if ( pPass->dwFlags & RL_PASS_FLAG_RENDER_SKYBOX )
			{
				DL_SET_STATE_PTR(   DRAWLIST_PRIMARY, DLSTATE_EYE_LOCATION_PTR,		&vEyeLocation );
				DL_CALLBACK(		DRAWLIST_PRIMARY, dx9_RenderSkybox, SKYBOX_PASS_BACKDROP, NULL, 0, NULL );
				DL_CLEAR_DEPTH(		DRAWLIST_PRIMARY );
			}

			V( dx9_RenderDrawList( DRAWLIST_PRIMARY ) );
			DL_CLEAR_LIST(		DRAWLIST_PRIMARY );
			e_SetFogEnabled( e_GetRenderFlag( RENDER_FLAG_FOG_ENABLED ) );

			V( dxC_SetSamplerDefaults( SAMPLER_LIGHTMAP ) );
		}

		BOOL bZWriteEnabled = TRUE;
		dxC_SetRenderState( D3DRS_ZWRITEENABLE, bZWriteEnabled );

		DL_SET_STATE_PTR(   DRAWLIST_PRIMARY, DLSTATE_VIEW_MATRIX_PTR,		&matView );
		DL_SET_STATE_PTR(   DRAWLIST_PRIMARY, DLSTATE_PROJ_MATRIX_PTR,		&matProj );
		DL_SET_STATE_PTR(   DRAWLIST_PRIMARY, DLSTATE_PROJ2_MATRIX_PTR,		&matProj2 );
		DL_SET_STATE_PTR(   DRAWLIST_PRIMARY, DLSTATE_EYE_LOCATION_PTR,		&vEyeLocation );

		switch ( pPass->eType )
		{
		case RL_PASS_DEPTH:
			if ( e_OptionState_GetActive()->get_bDepthOnlyPass() )
			{
				UNDEFINED_CODE_X( "ZPass needs to be re-implemented for new system!" );
				break;

				D3D_PROFILE_REGION( L"ZPass" );

				DL_SET_STATE_DWORD( DRAWLIST_PRIMARY, DLSTATE_MESH_FLAGS_DO_DRAW,   MESH_FLAG_SILHOUETTE );
				DL_SET_STATE_DWORD( DRAWLIST_PRIMARY, DLSTATE_MESH_FLAGS_DONT_DRAW, MESH_FLAG_ALPHABLEND );
				DL_SET_STATE_DWORD( DRAWLIST_PRIMARY, DLSTATE_USE_LIGHTS,			FALSE );
				sDrawListAddModelList( DRAWLIST_PRIMARY, pPass, 0, 0, TRUE, DLCOM_DRAW_DEPTH );
			}
			break;
		case RL_PASS_REFLECTION:
			// set appropriate reflection matrices here?
		case RL_PASS_PORTAL:
		case RL_PASS_NORMAL:
			if ( AppIsTugboat() )
			{
				// opaque models
			    {
				    D3D_PROFILE_REGION( L"Opaque Static Models" );
    
				    DL_SET_STATE_DWORD( DRAWLIST_PRIMARY, DLSTATE_MESH_FLAGS_DO_DRAW,   MESH_FLAG_SILHOUETTE );
				    DL_SET_STATE_DWORD( DRAWLIST_PRIMARY, DLSTATE_MESH_FLAGS_DONT_DRAW, MESH_FLAG_ALPHABLEND | MESH_FLAG_SKINNED | MESH_FLAG_NOCOLLISION );
				    DL_SET_STATE_DWORD( DRAWLIST_PRIMARY, DLSTATE_USE_LIGHTS,			TRUE );
				    sDrawListAddModelList( DRAWLIST_PRIMARY, pPass, 0, 0, DLCOM_DRAW );
			    }
				if( e_GetActiveShadowType() == SHADOW_TYPE_NONE )
				{
					RENDER_CONTEXT tContext;

					tContext.matProj = matProj;
					tContext.matProj2 = matProj2;
					tContext.matView = matView;
					tContext.pEnvDef = e_GetCurrentEnvironmentDef();
					tContext.vEyeDir_w = vEyeVector;
					tContext.vEyePos_w = vEyeLocation;

					dx9_DrawListAddCommand( DRAWLIST_PRIMARY, DLCOM_DRAW_BLOB_SHADOW, 0, 0, (void*)&tContext );
				}


			    // 'behind' models
				// Render skybox, if applicable
				if ( pPass->dwFlags & RL_PASS_FLAG_DRAW_BEHIND )
			    {

				    D3D_PROFILE_REGION( L"Opaque Skinned Models" );
    
				    DL_SET_STATE_DWORD( DRAWLIST_PRIMARY, DLSTATE_MESH_FLAGS_DO_DRAW,   MESH_FLAG_SKINNED | MESH_FLAG_ALPHABLEND | MESH_FLAG_SILHOUETTE | MESH_FLAG_NOCOLLISION );
				    DL_SET_STATE_DWORD( DRAWLIST_PRIMARY, DLSTATE_MESH_FLAGS_DONT_DRAW, 0 );
				    DL_SET_STATE_DWORD( DRAWLIST_PRIMARY, DLSTATE_USE_LIGHTS,			FALSE );
				    sDrawListAddModelList( DRAWLIST_PRIMARY, pPass, 0, 0, DLCOM_DRAW_BEHIND, TRUE );
			    }
			    // opaque models
			    {
				    D3D_PROFILE_REGION( L"Opaque Skinned Models" );
    
				    DL_SET_STATE_DWORD( DRAWLIST_PRIMARY, DLSTATE_MESH_FLAGS_DO_DRAW,   MESH_FLAG_SKINNED | MESH_FLAG_NOCOLLISION );
					DL_SET_STATE_DWORD( DRAWLIST_PRIMARY, DLSTATE_MESH_FLAGS_DONT_DRAW, MESH_FLAG_ALPHABLEND );					
				    DL_SET_STATE_DWORD( DRAWLIST_PRIMARY, DLSTATE_USE_LIGHTS,			TRUE );
				    sDrawListAddModelList( DRAWLIST_PRIMARY, pPass, 0, 0, DLCOM_DRAW );
			    }
			}
			else
			{
				// non-Tugboat case
			    // opaque models
			    {
				    D3D_PROFILE_REGION( L"Opaque Models" );
    
					// AE 2006.12.13
					if ( e_OptionState_GetActive()->get_bDepthOnlyPass() )
					{
						DL_SET_STATE_DWORD( DRAWLIST_PRIMARY, DLSTATE_MESH_FLAGS_DO_DRAW,   MESH_FLAG_SILHOUETTE );
						DL_SET_STATE_DWORD( DRAWLIST_PRIMARY, DLSTATE_MESH_FLAGS_DONT_DRAW, MESH_FLAG_ALPHABLEND );
						DL_SET_STATE_DWORD( DRAWLIST_PRIMARY, DLSTATE_USE_LIGHTS,			FALSE );
						sDrawListAddModelList( DRAWLIST_PRIMARY, pPass, 0, 0, DLCOM_DRAW_DEPTH );
					}					
					
				    DL_SET_STATE_DWORD( DRAWLIST_PRIMARY, DLSTATE_MESH_FLAGS_DO_DRAW,   MESH_FLAG_SILHOUETTE );
				    DL_SET_STATE_DWORD( DRAWLIST_PRIMARY, DLSTATE_MESH_FLAGS_DONT_DRAW, MESH_FLAG_ALPHABLEND );
				    DL_SET_STATE_DWORD( DRAWLIST_PRIMARY, DLSTATE_USE_LIGHTS,			TRUE );
				    sDrawListAddModelList( DRAWLIST_PRIMARY, pPass, 0, 0, DLCOM_DRAW );
			    }
			}

			// alpha models
			{
				D3D_PROFILE_REGION( L"Alpha Models" );

				DL_SET_STATE_DWORD( DRAWLIST_PRIMARY, DLSTATE_USE_LIGHTS,			TRUE );
				DL_SET_STATE_DWORD( DRAWLIST_PRIMARY, DLSTATE_MESH_FLAGS_DO_DRAW,   MESH_FLAG_ALPHABLEND );
				DL_SET_STATE_DWORD( DRAWLIST_PRIMARY, DLSTATE_MESH_FLAGS_DONT_DRAW, MESH_FLAG_SILHOUETTE );
				DL_FENCE( DRAWLIST_PRIMARY );
				pPass->tModelList.SetIndexingMethod( ARRAY_INDEXING_METHOD_REVERSE );
				sDrawListAddModelList( DRAWLIST_PRIMARY, pPass, MODELDEF_FLAG_HAS_ALPHA, 0, DLCOM_DRAW );
				pPass->tModelList.SetIndexingMethod( ARRAY_INDEXING_METHOD_FORWARD );
			}

		case RL_PASS_GENERATECUBEMAP:
			{

			}
			break;
		}

		V( dx9_RenderDrawList( DRAWLIST_PRIMARY ) );
		DL_CLEAR_LIST( DRAWLIST_PRIMARY );


		// Render skybox alpha pass, if applicable
		if ( pPass->dwFlags & RL_PASS_FLAG_RENDER_SKYBOX )
		{
			e_SetFogEnabled( FALSE );

			// workaround for skybox models clipping out -- this value should be detected
			e_SetFarClipPlane( 3000.f );
			MATRIX matSkyboxProj;
			e_InitMatrices( NULL, NULL, &matSkyboxProj, NULL, NULL, NULL );

			DL_SET_STATE_PTR(   DRAWLIST_PRIMARY, DLSTATE_EYE_LOCATION_PTR,		&vEyeLocation );
			DL_SET_STATE_PTR(   DRAWLIST_PRIMARY, DLSTATE_PROJ_MATRIX_PTR,		&matSkyboxProj );
			DL_CALLBACK( DRAWLIST_PRIMARY, dx9_RenderSkybox, SKYBOX_PASS_AFTER_MODELS, NULL, 0, NULL );
			V( dx9_RenderDrawList( DRAWLIST_PRIMARY ) );
			DL_CLEAR_LIST( DRAWLIST_PRIMARY );

			e_SetFogEnabled( FALSE );
			if ( pPass->dwFlags & RL_PASS_FLAG_RENDER_PARTICLES )
			{
				V( e_ParticlesDraw( DRAW_LAYER_ENV, &matView, &matSkyboxProj, &matProj2, vEyeLocation, vEyeVector, TRUE, FALSE ) );
			}

			V( e_ResetClipAndFog() );

			V( e_SetFogEnabled( e_GetRenderFlag( RENDER_FLAG_FOG_ENABLED ) ) );

			V( dxC_SetSamplerDefaults( SAMPLER_LIGHTMAP ) );
		}


		// layer on particles, if applicable
		if ( pPass->dwFlags & RL_PASS_FLAG_RENDER_PARTICLES )
		{
			e_SetFogEnabled( FALSE );
			V( e_ParticlesDraw( DRAW_LAYER_GENERAL, &matView, &matProj, &matProj2, vEyeLocation, vEyeVector, TRUE, FALSE ) );
		}
	}


	return 0;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_Reset()
{
#ifdef ENGINE_TARGET_DX9
	V(dx9_ResetDevice());
#endif
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_OptimizeManagedResources()
{
	return DXC_9_10( dxC_GetDevice()->EvictManagedResources(), S_OK );
}
