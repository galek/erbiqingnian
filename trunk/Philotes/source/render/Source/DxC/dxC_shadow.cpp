//----------------------------------------------------------------------------
// dxC_shadow.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"

#include "dxC_shadow.h"
#include "dxC_target.h"
#include "dxC_caps.h"
#include "dx9_settings.h"
#include "dxC_texture.h"
#include "dxC_environment.h"
#include "dxC_state.h"
#include "camera_priv.h"
#include "dxC_primitive.h"
#include "dxC_main.h"
#include "dxC_drawlist.h"
#include "dxC_state_defaults.h"
#include "e_frustum.h"
#include "dx9_shadowtypeconstants.h"	// CHB 2006.11.02
#include "e_optionstate.h"				// CHB 2007.02.26



//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------
CArrayIndexed<D3D_SHADOW_BUFFER> gtShadowBuffers;

//----------------------------------------------------------------------------
// IMPLEMENTATIONS
//----------------------------------------------------------------------------

float e_ShadowBufferGetCosTheta( int nShadowBufferID )
{
	D3D_SHADOW_BUFFER * pBuffer = dx9_ShadowBufferGet( nShadowBufferID );
	return cosf( pBuffer->fFOV * 0.5f );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_ShadowsInit()
{
	ArrayInit( gtShadowBuffers, NULL, NUM_SHADOWMAPS );
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_ShadowsDestroy()
{
	gtShadowBuffers.Destroy();
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static
LPD3DCTEXTURE2D sGetShadowMapTexture( int nShadowBufferID )
{	
	ASSERT_RETNULL(e_UseShadowMaps());
	
	D3D_SHADOW_BUFFER* pShadow = dx9_ShadowBufferGet( nShadowBufferID );
	ASSERT_RETNULL( pShadow );

	switch ( e_GetActiveShadowType() )
	{
		case SHADOW_TYPE_DEPTH:
			if (TEST_MASK( pShadow->dwFlags, SHADOW_BUFFER_FLAG_USE_COLOR_BUFFER ))	// CHB 2007.02.17
			{
				return dxC_RTResourceGet( pShadow->eRenderTarget );
			}
			else
			{
				GLOBAL_DEPTH_TARGET tGDT = dxC_GetGlobalDepthTargetFromShadowMap( pShadow->eShadowMap );
				DEPTH_TARGET_INDEX tDT = dxC_GetGlobalDepthTargetIndex( tGDT );
				return dxC_DSResourceGet( tDT );
			}
		case SHADOW_TYPE_COLOR:
			return dxC_RTResourceGet( pShadow->eRenderTarget );

		default:
			return NULL;
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

// CHB 2007.02.17
static
void dxC_ShadowBufferInit(D3D_SHADOW_BUFFER * pShadow)
{
	memclear( pShadow, sizeof(*pShadow) );	
	pShadow->eShadowMap = (SHADOWMAP)INVALID_ID;
	pShadow->eRenderTarget = RENDER_TARGET_INVALID;
	pShadow->eRenderTargetPrimer = DXC_9_10( RENDER_TARGET_INVALID, (SHADOWMAP)INVALID_ID );
	pShadow->dwDebugColor = 0xff00ffff;

	// Initialize the shadow matrices
	D3DXMatrixIdentity( (D3DXMATRIXA16*)&pShadow->mShadowView );
	D3DXMatrixPerspectiveFovLH( (D3DXMATRIXA16*)&pShadow->mShadowProj, PI / 4.0f, 1, 1.00f, 100.0f);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_ShadowBufferNew( int nShadowMap, RENDER_TARGET_INDEX nRenderTarget, DWORD dwFlags, int nRenderTargetPrimer )
{
	ASSERT( nRenderTargetPrimer == INVALID_ID );

	// in Tugboat, we use a shadow buffer object to guide and aim the lighting map, but not actually shadow
	ASSERT_RETINVALIDARG( nShadowMap >= 0 && nShadowMap < NUM_SHADOWMAPS || ( AppIsTugboat() && nRenderTarget >= 0 && nShadowMap < dxC_GetNumRenderTargets() ) );
	ASSERT_RETINVALIDARG( nShadowMap < dxC_GetNumRenderTargets() );

	int nShadow;
	D3D_SHADOW_BUFFER* pShadow = gtShadowBuffers.Add( &nShadow );
	ASSERT_RETFAIL( pShadow );

	dxC_ShadowBufferInit(pShadow);	// CHB 2007.02.17
	pShadow->eShadowMap = (SHADOWMAP)nShadowMap;
	pShadow->eRenderTarget = nRenderTarget;
	pShadow->eRenderTargetPrimer = DXC_9_10( (RENDER_TARGET_INDEX)nRenderTargetPrimer, (SHADOWMAP)nRenderTargetPrimer );
	pShadow->dwFlags = dwFlags;

	DWORD dwDebugColor = 0xff << ( ( nShadow % 3 ) * 8 );
	dwDebugColor |= 0xff000000; // alpha 
	pShadow->dwDebugColor = dwDebugColor;	

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_ShadowBufferRemove( int nShadowBufferID )
{
	gtShadowBuffers.Remove( nShadowBufferID );
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_ShadowBufferRemoveAll()
{
	gtShadowBuffers.Clear();
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

bool e_UseShadowMaps(void)
{
	if ( ! e_ShadowsEnabled() )
	{
		return false;
	}
	switch (e_GetActiveShadowType())
	{
		case SHADOW_TYPE_NONE:
			return false;
		case SHADOW_TYPE_DEPTH:
		case SHADOW_TYPE_COLOR:
			return true;
		default:
			ASSERT(false);
			return false;
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

#ifdef ENGINE_TARGET_DX9

static
DXC_FORMAT sFindFirstUsableFormat(const DXC_FORMAT Formats[], unsigned nFormats, DWORD nUsage, D3DRESOURCETYPE nType)
{
	// DX9CAP_3D_HARDWARE_ACCELERATION cap is bit depth of frame buffer format,
	// or 0 for non-HAL.
	D3DC_DRIVER_TYPE dwDeviceType = (dx9_CapGetValue(DX9CAP_3D_HARDWARE_ACCELERATION) > 0) ? D3DDEVTYPE_HAL : D3DDEVTYPE_SW;

	DXC_FORMAT nBackBufferFormat = dxC_GetBackbufferAdapterFormat();

	for (unsigned i = 0; i < nFormats; ++i)
	{
		DXC_FORMAT nFormat = Formats[i];
		V_DO_SUCCEEDED( dx9_GetD3D()->CheckDeviceFormat(
			dxC_GetAdapter(),
			dwDeviceType,
			nBackBufferFormat,
			nUsage,
			nType,
			nFormat ) )
		{
			return nFormat;
		}
	}
	return D3DFMT_UNKNOWN;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
#endif

static
DXC_FORMAT sGetColorShadowsColorFormat(void)
{
#ifdef ENGINE_TARGET_DX9
	// See what texture color format(s) we can use as render targets.
	// For now we just try R16F, but we could also do D3DFMT_R32F,
	// D3DFMT_G16R16F, D3DFMT_A8, D3DFMT_L8, D3DFMT_A8R8G8B8, or any
	// other format that offers at least 8 bits of precision for a
	// particular color component in the most compact size.
	// (Shaders might need to know which component, so maybe better
	// to stick with red or alpha?)

	const DXC_FORMAT Formats[] = { D3DFMT_R16F, D3DFMT_R32F, D3DFMT_A8R8G8B8, D3DFMT_X8R8G8B8 };
	return sFindFirstUsableFormat(Formats, ARRAYSIZE(Formats), D3DUSAGE_RENDERTARGET, D3DRTYPE_TEXTURE);
#elif defined( ENGINE_TARGET_DX10 )
	return DXGI_FORMAT_R32_FLOAT;
#endif
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static
DXC_FORMAT sGetColorShadowsDepthFormat(void)
{
#ifdef ENGINE_TARGET_DX9
	const DXC_FORMAT Formats[] = { D3DFMT_D16, D3DFMT_D24X8, D3DFMT_D24S8 };
	return sFindFirstUsableFormat(Formats, ARRAYSIZE(Formats), D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_SURFACE);
#elif defined( ENGINE_TARGET_DX10 )
	return DXGI_FORMAT_R32_TYPELESS;
#endif
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

bool e_IsShadowTypeAvailable(const OptionState * pState, int nType)
{
	// CHB 2007.02.17 - Texture projection for pixel shaders isn't
	// available until PS 1.4.  In any event, the 1.1 shaders don't
	// do shadows.
	bool bCapablePS =
#ifdef ENGINE_TARGET_DX10
		true;
#else
		(pState->get_nMaxEffectTarget() > FXTGT_SM_20_LOW);
		//(pState->get_dwPixelShaderVersion() >= D3DPS_VERSION(2, 0));
#endif

	BOOL bForceColorSM = FALSE;


	switch (nType)
	{
		case SHADOW_TYPE_DEPTH:
			// CHB 2007.02.22 - Note: Apparently nVidia drivers do
			// not allow depth target as a texture on the debug
			// runtime. (!)
			return bCapablePS 
				&& dx9_CapGetValue(DX9CAP_HW_SHADOWS) 
				&& !bForceColorSM
				&& !dx9_IsDebugRuntime();

		case SHADOW_TYPE_COLOR:
			return bCapablePS && (sGetColorShadowsColorFormat() != D3DFMT_UNKNOWN);

		case SHADOW_TYPE_NONE:
			return true;

		default:
			ASSERT(false);
			return false;
	}
	return false;
}

// CHB 2007.02.17
static
PRESULT sComputeShadowMapSize(unsigned & nSizeOut)
{
	// maximum amount of video mem to give to this shadow buffer
	const float fMaxPercentVideoMem = AppIsTugboat() ? 0.1f : 0.25f;
	const float cfMinVideoMem = AppIsTugboat() ? 64.0f : 100.0f;	// CHB 2007.02.16

	// adjusting for slight errors in the avail mem estimate
	int nAvailMem = e_GetMaxPhysicalVideoMemoryMB();
	float fMem = (float)nAvailMem;
	float fAvailMem = fMem * 1.1f;
	if ( fAvailMem < cfMinVideoMem )
	{
		//fMaxPercentVideoMem = 0.05f;
		// CHB !!! - disabling of shadows needs to happen during state Update() or in FeatureLine
		//e_SetRenderFlag( RENDER_FLAG_SHADOWS_ENABLED, FALSE );
		return S_FALSE;
	}

	DXC_FORMAT tFormat = D3DFMT_UNKNOWN;
	if ( dx9_IsTextureFormatOk( D3DFMT_R32F, TRUE ) )
		tFormat = D3DFMT_R32F;
	else if ( dx9_IsTextureFormatOk( D3DFMT_G16R16F, TRUE ) )
		tFormat = D3DFMT_G16R16F;
	else if ( dx9_IsTextureFormatOk( D3DFMT_A8R8G8B8, TRUE ) )
		tFormat = D3DFMT_A8R8G8B8;
	ASSERT_RETFAIL( tFormat != D3DFMT_UNKNOWN );

	int nShadowMapSize = min( dx9_CapGetValue( DX9CAP_MAX_TEXTURE_WIDTH ), dx9_CapGetValue( DX9CAP_MAX_TEXTURE_HEIGHT ) );
	int nShadowMapBitDepth = dx9_GetTextureFormatBitDepth( tFormat );
	int nShadowMaps = 1;

	if ( AppIsHellgate() )
		nShadowMaps = 6;

	// desired size must be a power of 2
	if ( !IsPowerOf2( nShadowMapSize ) )
	{
		// Grab the next highest power of 2 resolution
		nShadowMapSize = e_GetPow2Resolution( nShadowMapSize );
		// Then, take half of that resolution (still being a power of 2)
		nShadowMapSize >>= 1;
	}

	for ( ; nShadowMapSize >= 256; nShadowMapSize >>= 1 )
	{
		float fSize = float( ( nShadowMaps * nShadowMapBitDepth * nShadowMapSize * nShadowMapSize ) / 8 );
		if ( fSize <= fAvailMem * fMaxPercentVideoMem * 1024.0f * 1024.0f )
			break;
	}

	const int cnMaxTextureRes = 2048;
	if ( AppIsHellgate() )
	{
		nShadowMapSize = min( nShadowMapSize, cnMaxTextureRes );
	}
	else
	{
		// tugboat is lighter-weight, so let it use bigger textures
		nShadowMapSize = min( nShadowMapSize, 2048 );
		//nShadowMapSize = min( nShadowMapSize, 1024 );
	}

	nSizeOut = nShadowMapSize;
	return S_OK;
}

// CHB 2007.01.18
const int nShadowTargetPriority = 20;

// CHB 2007.02.17
D3D_SHADOW_BUFFER * dxC_GetParticleLightingMapShadowBuffer(void)
{
	if (AppIsTugboat())
	{
		static D3D_SHADOW_BUFFER buf;
		return &buf;
	}
#if ISVERSION(DEVELOPMENT_VERSION)
	ASSERT(false);
#endif
	return 0;
}

// CHB 2007.02.17
static
PRESULT e_ParticleLightingMapCreate(unsigned nShadowMapSize)
{
	if (AppIsTugboat())
	{
		// doing color lighting stuff in Tugboat
		int nLightingMapSize = nShadowMapSize / 4;
		nLightingMapSize = MAX( nLightingMapSize, 256 );
		dxC_AddRenderTargetDesc( FALSE, -1, GLOBAL_RT_LIGHTING_MAP, nLightingMapSize, nLightingMapSize, D3DC_USAGE_2DTEX_RT, D3DFMT_A8R8G8B8, -1, FALSE, nShadowTargetPriority + 1, 1 );
		
		D3D_SHADOW_BUFFER * pShadow = dxC_GetParticleLightingMapShadowBuffer();
		dxC_ShadowBufferInit(pShadow);
#ifndef ENGINE_TARGET_DX10  //No color targets in Dx10
		pShadow->eRenderTarget = dxC_GetGlobalRenderTargetIndex( GLOBAL_RT_LIGHTING_MAP );
#else
		return E_NOTIMPL;
		//pShadow->eShadowMap = (SHADOWMAP)GLOBAL_RT_LIGHTING_MAP;
#endif
		pShadow->nWidth = nLightingMapSize;
		pShadow->nHeight = nLightingMapSize;
		pShadow->tVP.vMin = VECTOR( 0.0f, 0.0f, 0.0f );
		pShadow->tVP.vMax = VECTOR( static_cast<float>(pShadow->nWidth), static_cast<float>(pShadow->nWidth), 1.0f );
	}
	return S_OK;
}

// need to create shadow render target with other render targets
PRESULT e_ShadowsCreate(const OptionState * pState)
{
	// free all existing shadow maps, first
	V( e_ShadowBufferRemoveAll() );

	int nShadowType = pState->get_nShadowType();

	// Get desired size.
	unsigned nShadowMapSize = 0;
	V_RETURN(sComputeShadowMapSize(nShadowMapSize));
	if ( nShadowMapSize == 0 )
		return S_FALSE;

	// Create particle light map.
	V(e_ParticleLightingMapCreate(nShadowMapSize));

	// Reduce shadow resolution as needed.
	nShadowMapSize >>= pState->get_nShadowReductionFactor();

	// CHB 2006.11.02
	switch (nShadowType)
	{
		case SHADOW_TYPE_DEPTH:
		{
			DXC_FORMAT tShadowColorFormat = sGetColorShadowsColorFormat();
			DXC_FORMAT tShadowFormat = DX9_BLOCK( D3DFMT_D24X8 ) DX10_BLOCK( D3DFMT_R32F );
			DXC_FORMAT tUnusedShadowColorFormat = (DXC_FORMAT)dx9_CapGetValue( DX9CAP_SMALLEST_COLOR_TARGET );
			const UINT nMSAASamples = 1;

#ifdef ENGINE_TARGET_DX9
			if (   dx9_CapGetValue( DX9CAP_GPU_VENDOR_ID ) == DT_NVIDIA_VENDOR_ID
				&& dx9_CapGetValue( DX9CAP_NULL_RENDER_TARGET ) )
			{
				// ATI's method:
				// ONLY use if we are not using render target primers
				//dxC_AddRenderTargetDesc( 1,	1, D3DC_USAGE_2DTEX_RT, tUnusedShadowColorFormat, -1, FALSE, nShadowTargetPriority, nMSAASamples );

				// NVIDIA's method:
				dxC_AddRenderTargetDesc( FALSE, -1, GLOBAL_RT_SHADOW_COLOR, nShadowMapSize, nShadowMapSize, D3DC_USAGE_2DTEX_RT, (DXC_FORMAT)MAKEFOURCC('N','U','L','L'), -1, FALSE, nShadowTargetPriority, nMSAASamples );
			}
			else
				// must create small, unused color surface of the same dimension for d3d -- eh.
				// want to use the smallest useful color texture since the engine doesn't use this for anything
				dxC_AddRenderTargetDesc( FALSE, -1, GLOBAL_RT_SHADOW_COLOR, nShadowMapSize, nShadowMapSize, D3DC_USAGE_2DTEX_RT, tUnusedShadowColorFormat,		-1,	FALSE, nShadowTargetPriority, nMSAASamples );
#endif
			dxC_AddRenderTargetDesc( FALSE, -1, dxC_GetGlobalDepthTargetFromShadowMap( SHADOWMAP_HIGH ), nShadowMapSize, nShadowMapSize, D3DC_USAGE_2DTEX_SM, tShadowFormat,			-1,		FALSE, nShadowTargetPriority, nMSAASamples );

			if ( AppIsHellgate() )
			{
				dxC_AddRenderTargetDesc( FALSE, -1, dxC_GetGlobalDepthTargetFromShadowMap( SHADOWMAP_MID ), nShadowMapSize, nShadowMapSize, D3DC_USAGE_2DTEX_SM, tShadowFormat,			-1,		FALSE, nShadowTargetPriority, nMSAASamples );
				dxC_AddRenderTargetDesc( FALSE, -1, dxC_GetGlobalDepthTargetFromShadowMap( SHADOWMAP_LOW ), nShadowMapSize, nShadowMapSize, D3DC_USAGE_2DTEX_SM, tShadowFormat,			-1,		FALSE, nShadowTargetPriority, nMSAASamples );
				
				// For depth shadows, we can share the same RT among all SMs
				RENDER_TARGET_INDEX fakeColorTarget =	DX9_BLOCK( dxC_GetGlobalRenderTargetIndex( GLOBAL_RT_SHADOW_COLOR ) )
														DX10_BLOCK( RENDER_TARGET_NULL );

				V( dxC_ShadowBufferNew( SHADOWMAP_HIGH, fakeColorTarget, 
					SHADOW_BUFFER_FLAG_RENDER_DYNAMIC | SHADOW_BUFFER_FLAG_ALWAYS_DIRTY | SHADOW_BUFFER_FLAG_EXTRA_OUTDOOR_PASS ) );

				V( dxC_ShadowBufferNew( SHADOWMAP_MID, fakeColorTarget, 
					SHADOW_BUFFER_FLAG_RENDER_STATIC | SHADOW_BUFFER_FLAG_USE_GRID ) );

				V( dxC_ShadowBufferNew( SHADOWMAP_LOW, fakeColorTarget, 
					SHADOW_BUFFER_FLAG_RENDER_STATIC | SHADOW_BUFFER_FLAG_FULL_REGION ) );
			}
			else
			{
				V( dxC_ShadowBufferNew( SHADOWMAP_HIGH, dxC_GetGlobalRenderTargetIndex(GLOBAL_RT_SHADOW_COLOR), SHADOW_BUFFER_FLAG_ALWAYS_DIRTY ) );
			}
			
			break;
		}

		case SHADOW_TYPE_COLOR:
		{
			const UINT nMSAASamples = 1;
			DXC_FORMAT nColorFormat = sGetColorShadowsColorFormat();	// CHB 2007.02.15
			ASSERT_RETFAIL(nColorFormat != D3DFMT_UNKNOWN);
			DXC_FORMAT nDepthFormat = DX9_BLOCK( sGetColorShadowsDepthFormat() ) DX10_BLOCK( DXGI_FORMAT_R32_TYPELESS );
			ASSERT_RETFAIL(nDepthFormat != D3DFMT_UNKNOWN);
			dxC_AddRenderTargetDesc( FALSE, -1, GLOBAL_RT_SHADOW_COLOR, nShadowMapSize, nShadowMapSize, D3DC_USAGE_2DTEX_RT, nColorFormat, -1, FALSE, nShadowTargetPriority, nMSAASamples );
			dxC_AddRenderTargetDesc( FALSE, -1, dxC_GetGlobalDepthTargetFromShadowMap( SHADOWMAP_HIGH ), nShadowMapSize, nShadowMapSize, D3DC_USAGE_2DTEX_DS, nDepthFormat, -1, FALSE, nShadowTargetPriority, nMSAASamples );

			if ( AppIsHellgate() )
			{				
				dxC_AddRenderTargetDesc( FALSE, -1, GLOBAL_RT_SHADOW_COLOR_MID, nShadowMapSize, nShadowMapSize, D3DC_USAGE_2DTEX_RT, nColorFormat,	-1,		FALSE, nShadowTargetPriority, nMSAASamples );
				dxC_AddRenderTargetDesc( FALSE, -1, GLOBAL_RT_SHADOW_COLOR_LOW, nShadowMapSize, nShadowMapSize, D3DC_USAGE_2DTEX_RT, nColorFormat,	-1,		FALSE, nShadowTargetPriority, nMSAASamples );
				
				// For color shadows, we can share the same DS among all color RTs				
				V( dxC_ShadowBufferNew( SHADOWMAP_HIGH, dxC_GetGlobalRenderTargetIndex(GLOBAL_RT_SHADOW_COLOR), 
					SHADOW_BUFFER_FLAG_USE_COLOR_BUFFER | SHADOW_BUFFER_FLAG_RENDER_DYNAMIC | SHADOW_BUFFER_FLAG_ALWAYS_DIRTY | SHADOW_BUFFER_FLAG_EXTRA_OUTDOOR_PASS ) );
				
				V( dxC_ShadowBufferNew( SHADOWMAP_HIGH, dxC_GetGlobalRenderTargetIndex(GLOBAL_RT_SHADOW_COLOR_MID), 
					SHADOW_BUFFER_FLAG_USE_COLOR_BUFFER | SHADOW_BUFFER_FLAG_RENDER_STATIC | SHADOW_BUFFER_FLAG_USE_GRID ) );

				V( dxC_ShadowBufferNew( SHADOWMAP_HIGH, dxC_GetGlobalRenderTargetIndex(GLOBAL_RT_SHADOW_COLOR_LOW), 
					SHADOW_BUFFER_FLAG_USE_COLOR_BUFFER | SHADOW_BUFFER_FLAG_RENDER_STATIC | SHADOW_BUFFER_FLAG_FULL_REGION ) );
			}
			else
			{
				V( dxC_ShadowBufferNew( SHADOWMAP_HIGH, dxC_GetGlobalRenderTargetIndex(GLOBAL_RT_SHADOW_COLOR), SHADOW_BUFFER_FLAG_ALWAYS_DIRTY ) );
			}

			break;
		}

		default:
			return S_FALSE;
	}

	return S_OK;
}

PRESULT e_ShadowsCreate()
{
	return e_ShadowsCreate(e_OptionState_GetActive());
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_ShadowBufferSetPointLightParams( int nShadowBufferID, float fFOV, VECTOR &vPos, VECTOR &vLookAt )
{
	D3D_SHADOW_BUFFER * pShadow = dx9_ShadowBufferGet( nShadowBufferID );
	if ( !pShadow )
		return S_FALSE;
	pShadow->bDirectionalLight = FALSE;
	pShadow->fFOV = fFOV;
	pShadow->vLightPos = * (D3DXVECTOR3*) &vPos;
	VECTOR vResult = vLookAt - vPos;
	VectorNormalize( vResult, vResult );
	pShadow->vLightDir = * (D3DXVECTOR3*) &vResult;

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

// CHB 2007.02.17
static
PRESULT dxC_ShadowBufferSetDirectionalLightParams( D3D_SHADOW_BUFFER * pShadow, const VECTOR& vFocus, VECTOR& vDir, float fScale )
{
	pShadow->fFOV = 0.0f;
	pShadow->bDirectionalLight = TRUE;
	pShadow->fScale = fScale;

	if ( fScale <= 0.f )
		return S_FALSE;

	float fDistance = fScale;
	if ( AppIsHellgate() )
		// double the scale to get a safe distance
		fDistance *= 2.0f;

	VECTOR vUp( 0.0f, 0.0f, 1.0f );
	if ( AppIsTugboat() )
		vUp = VECTOR( 0.0f, 1.0f, 0.0f );

	VECTOR vParallelTest;
	VectorCross( vParallelTest, vUp, vDir );
	if ( VectorIsZeroEpsilon( vParallelTest ) )
		vDir.x += 0.1f; // Offset a bit to avoid degenerate case

	VECTOR vDirNorm;
	VectorNormalize( vDirNorm, vDir );

	ASSERT_RETINVALIDARG(    Math_IsFinite( vFocus.x )
						  && Math_IsFinite( vFocus.y )
						  && Math_IsFinite( vFocus.z ) );
	ASSERT_RETINVALIDARG(    Math_IsFinite( vDir.x )
						  && Math_IsFinite( vDir.y )
						  && Math_IsFinite( vDir.z ) );
	ASSERT_RETINVALIDARG( Math_IsFinite( fScale ) );

	VECTOR vPos = vFocus - ( vDirNorm * fDistance );
	VECTOR vLookAt = vFocus;
	if ( VectorDistanceSquared( pShadow->vWorldFocus, vLookAt ) >= 0.1f )
		SET_MASK( pShadow->dwFlags, SHADOW_BUFFER_FLAG_DIRTY );

	float fNear = 0.01f;
	float fFar = fDistance * 2.0f;

	float fInf = Math_GetInfinity();
	float fNaN = Math_GetNaN();

	ASSERT_RETFAIL( Math_IsFinite( fDistance ) );
	ASSERT_RETFAIL(	   Math_IsFinite( vPos.x )
					&& Math_IsFinite( vPos.y )
					&& Math_IsFinite( vPos.z ) );

	// AE 2007.06.28: Let's always shrink the near and far planes to the region.
	//if ( TEST_MASK( pShadow->dwFlags, SHADOW_BUFFER_FLAG_FULL_REGION ) )
	if (AppIsHellgate() && ! c_GetToolMode() )
	{
		REGION* pRegion = e_GetCurrentRegion();

		ORIENTED_BOUNDING_BOX tOBB;
		MakeOBBFromAABB( tOBB, &pRegion->tBBox );
		
		PLANE tShadowPlane;
		PlaneFromPointNormal( &tShadowPlane, &vPos, &vDirNorm );

		ASSERT_RETFAIL( Math_IsFinite( tShadowPlane.d ) );

		// Swap the near / far plane distances and work back towards the
		// minimum / maximum plane distance that works, respectively.
		float fNearSq = fFar * fFar;
		float fFarSq = fNear * fNear;

		const int cnMaxSafetyIterations = 100;
		int nSafetyIterations = 0;

		for ( int i = 0; i < OBB_POINTS; i++ )
		{
			ASSERTV_BREAK( nSafetyIterations < cnMaxSafetyIterations, "Infinite loop case detected, breaking out!" );
			++nSafetyIterations;

			VECTOR& vBoxPoint = tOBB[ i ];
			VECTOR vPointOnPlane;
			NearestPointOnPlane( &tShadowPlane, &vBoxPoint, &vPointOnPlane );

			ASSERT_RETFAIL(    Math_IsFinite( vPointOnPlane.x )
							&& Math_IsFinite( vPointOnPlane.y )
							&& Math_IsFinite( vPointOnPlane.z ) );

			float fDistSq = VectorDistanceSquared( vPointOnPlane, vBoxPoint );
			ASSERT_RETFAIL( Math_IsFinite( fDistSq ) );

			// Check if the box point is in front of or behind the plane.
			if ( PlaneDotCoord( &tShadowPlane, &vBoxPoint ) > 0 )
			{
				if ( fDistSq < fNearSq )
					fNearSq = fDistSq;

				if ( fDistSq > fFarSq )
					fFarSq = fDistSq;
			}
			else
			{
				fDistSq = MAX( fDistSq, 0.2f );
				fDistance += sqrtf( fDistSq ) * 1.25; // Add a little buffer
				vPos = vFocus - ( vDirNorm * fDistance );
				PlaneFromPointNormal( &tShadowPlane, &vPos, &vDirNorm );

				// The plane was moved to include this box point, so the near
				// plane is now at zero.
				fNearSq = 0;
				fFarSq += fDistSq;

				// We changed the plane, so we have to recalculate distances.
				i = -1;
			}
		}		

		if ( fFarSq > fNearSq )
		{
			fNear = MAX( sqrtf( fNearSq ), fNear );
			fFar = sqrtf( fFarSq );
		}
	}

	pShadow->vWorldFocus = vLookAt;
	pShadow->vLightPos = vPos;
	pShadow->vLightDir = vFocus - vPos;
	VectorNormalize( pShadow->vLightDir );

	D3DXMatrixLookAtLH( (D3DXMATRIXA16*)&pShadow->mShadowView, (D3DXVECTOR3*)&vPos, (D3DXVECTOR3*)&vLookAt, (D3DXVECTOR3*)&vUp );
	// orthogonal matrices use linear-z, so it does not matter so much that our near clip plane is close.
	D3DXMatrixOrthoLH( (D3DXMATRIXA16*)&pShadow->mShadowProj, fScale, fScale, fNear, fFar );

	static float sfFarBias = -0.7f;
	static float sfFarFullRegionBias = -3.5f;
	float fBias = sfFarBias;

	if ( TEST_MASK( pShadow->dwFlags, SHADOW_BUFFER_FLAG_FULL_REGION ) )
		fBias = sfFarFullRegionBias;

	D3DXMatrixOrthoLH( (D3DXMATRIXA16*)&pShadow->mShadowProjBiased, fScale, fScale, fNear, fFar + fBias );

	if ( AppIsTugboat() )
	{
		// this code rotates the projection to better fit the viewport
		// this didn't quite return the number I was looking for, so I hard-coded

		VECTOR vView;
		V( e_GetViewDirection( &vView ) );
		vView.fZ = 0;
		VectorNormalize( vView );
		float fRot = VectorDirectionToAngle( vView );

		MATRIX mRot, mXlate, mInvXlate;
		MatrixTranslation( &mXlate, &vLookAt );
		MatrixInverse( &mInvXlate, &mXlate );
		MatrixRotationZ( mRot, fRot );				

		MatrixMultiply( &mInvXlate, &mInvXlate, &mRot );
		MatrixMultiply( &mRot, &mXlate, &mInvXlate );

		MatrixMultiply( &pShadow->mShadowProj, &pShadow->mShadowProj, &mRot );
	}
	else
	{
		ORIENTED_BOUNDING_BOX tOBB;
		V( e_ExtractFrustumPoints( (VECTOR*)&tOBB, &pShadow->mShadowView, &pShadow->mShadowProj ) );

		PLANE tPlane;
		vUp.x = 0.f;
		vUp.y = 0.f;
		vUp.z = 1.f;
		PlaneFromPointNormal( &tPlane, &pShadow->vWorldFocus, &vUp );

		VECTOR vIntersect;
		PlaneVectorIntersect( &vIntersect, &tPlane, &tOBB[0], &tOBB[4] );
		tOBB[ 0 ] = vIntersect;
		tOBB[ 4 ] = vIntersect + vUp;
		PlaneVectorIntersect( &vIntersect, &tPlane, &tOBB[1], &tOBB[5] );
		tOBB[ 1 ] = vIntersect;
		tOBB[ 5 ] = vIntersect + vUp;
		PlaneVectorIntersect( &vIntersect, &tPlane, &tOBB[2], &tOBB[6] );
		tOBB[ 2 ] = vIntersect;
		tOBB[ 6 ] = vIntersect + vUp;
		PlaneVectorIntersect( &vIntersect, &tPlane, &tOBB[3], &tOBB[7] );
		tOBB[ 3 ] = vIntersect;
		tOBB[ 7 ] = vIntersect + vUp;
		MakeOBBPlanes( pShadow->tOBBPlanes, tOBB );
	}

	if ( e_GetRenderFlag( RENDER_FLAG_SHADOWS_SHOWAREA ) )
	{
		if ( AppIsHellgate() )
		{
			V( e_PrimitiveAddSphere( PRIM_FLAG_RENDER_ON_TOP | PRIM_FLAG_SOLID_FILL | PRIM_FLAG_WIRE_BORDER, &vLookAt, 0.1f, pShadow->dwDebugColor ) );

			ORIENTED_BOUNDING_BOX tOBB;
			V( e_ExtractFrustumPoints( (VECTOR*)&tOBB, &pShadow->mShadowView, &pShadow->mShadowProj ) );
			//V( e_PrimitiveAddBox( PRIM_FLAG_RENDER_ON_TOP, tOBB, pShadow->dwDebugColor ) );

			PLANE tPlane;
			vUp.x = 0.f;
			vUp.y = 0.f;
			vUp.z = 1.f;
			PlaneFromPointNormal( &tPlane, &pShadow->vWorldFocus, &vUp );

			// Show coverage area
			VECTOR vIntersect[4];
			PlaneVectorIntersect( &vIntersect[0], &tPlane, &tOBB[0], &tOBB[4] );
			PlaneVectorIntersect( &vIntersect[1], &tPlane, &tOBB[1], &tOBB[5] );
			PlaneVectorIntersect( &vIntersect[2], &tPlane, &tOBB[2], &tOBB[6] );
			PlaneVectorIntersect( &vIntersect[3], &tPlane, &tOBB[3], &tOBB[7] );

			V( e_PrimitiveAddRect( 0, &vIntersect[ 0 ], &vIntersect[ 1 ], &vIntersect[ 2 ], &vIntersect[ 3 ], pShadow->dwDebugColor ) );

			// Show plane normals
			for ( int nPlane = 2; nPlane < 6; nPlane++ )
			{
				VECTOR vPoint;
				NearestPointOnPlane( &pShadow->tOBBPlanes[ nPlane ], &pShadow->vWorldFocus, &vPoint );
				e_PrimitiveAddSphere( 0, &vPoint, 0.5f, pShadow->dwDebugColor );

				VECTOR vEndPoint;
				VectorScale( vEndPoint, pShadow->tOBBPlanes[ nPlane ].N(), 5.0f );
				VectorAdd( vEndPoint, vPoint, vEndPoint );
				e_PrimitiveAddSphere( 0, &vEndPoint, 0.5f, pShadow->dwDebugColor );
			}
		}
		else
		{
			//V( e_PrimitiveAddLine( PRIM_FLAG_RENDER_ON_TOP, &vPos, &vFocus, pShadow->dwDebugColor ) );
			//VECTOR vB = vFocus - VECTOR( 0,0,3.0f );
			//VECTOR vT = vFocus + VECTOR( 0,0,0.f );
			//V( e_PrimitiveAddCylinder( PRIM_FLAG_SOLID_FILL | PRIM_FLAG_WIRE_BORDER, &vB, &vT, fScale * 0.5f * SHADOW_RANGE_FUDGE, pShadow->dwDebugColor ) );
			ORIENTED_BOUNDING_BOX tOBB;
			V( e_ExtractFrustumPoints( (VECTOR*)&tOBB, &pShadow->mShadowView, &pShadow->mShadowProj ) );

			VECTOR vViewPos;
			V( e_GetViewFocusPosition( &vViewPos ) );

			PLANE tPlane;
			vUp.y = 0.f;
			vUp.z = 1.f;
			PlaneFromPointNormal( &tPlane, &vViewPos, &vUp );

			VECTOR vIntersect[4];
			PlaneVectorIntersect( &vIntersect[0], &tPlane, &tOBB[0], &tOBB[4] );
			PlaneVectorIntersect( &vIntersect[1], &tPlane, &tOBB[1], &tOBB[5] );
			PlaneVectorIntersect( &vIntersect[2], &tPlane, &tOBB[2], &tOBB[6] );
			PlaneVectorIntersect( &vIntersect[3], &tPlane, &tOBB[3], &tOBB[7] );

			DWORD dwCol = 0xff003f3f;
			V( e_PrimitiveAddLine( 0, 						&vIntersect[0], &vIntersect[1], dwCol ) );
			V( e_PrimitiveAddLine( 0, 						&vIntersect[1], &vIntersect[3], dwCol ) );
			V( e_PrimitiveAddLine( 0, 						&vIntersect[3], &vIntersect[2], dwCol ) );
			V( e_PrimitiveAddLine( 0,						&vIntersect[2], &vIntersect[0], dwCol ) );

			dwCol = 0x0000c0c0;
			V( e_PrimitiveAddLine( PRIM_FLAG_RENDER_ON_TOP, &vIntersect[0], &vIntersect[1], dwCol ) );
			V( e_PrimitiveAddLine( PRIM_FLAG_RENDER_ON_TOP, &vIntersect[1], &vIntersect[3], dwCol ) );
			V( e_PrimitiveAddLine( PRIM_FLAG_RENDER_ON_TOP, &vIntersect[3], &vIntersect[2], dwCol ) );
			V( e_PrimitiveAddLine( PRIM_FLAG_RENDER_ON_TOP, &vIntersect[2], &vIntersect[0], dwCol ) );
		}
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

// CHB 2007.02.17
#if 0	// CHB 2007.02.17 - Not used.
PRESULT e_ShadowBufferSetDirectionalLightParams( int nShadowBufferID, const VECTOR& vFocus, const VECTOR& vDir, float fScale )
{
	D3D_SHADOW_BUFFER * pShadow = dx9_ShadowBufferGet( nShadowBufferID );
	if ( !pShadow )
		return S_FALSE;
	return dxC_ShadowBufferSetDirectionalLightParams( pShadow, vFocus, vDir, fScale );
}
#endif

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_ShadowBufferGetLightPosInWorldView( int nShadowBufferID, const MATRIX * pmView, VECTOR4 * pvOutPos )
{
	ASSERT_RETINVALIDARG( pmView && pvOutPos );
	D3D_SHADOW_BUFFER * pShadow = dx9_ShadowBufferGet( nShadowBufferID );
	if ( !pShadow )
		return S_FALSE;
	D3DXVec3Transform( (D3DXVECTOR4*)pvOutPos, (D3DXVECTOR3*)&pShadow->vLightPos, (D3DXMATRIXA16*)pmView );
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_ShadowBufferGetLightDirInWorldView( int nShadowBufferID, const MATRIX * pmView, VECTOR4 * pvOutDir )
{
	ASSERT_RETINVALIDARG( pmView && pvOutDir );
	D3D_SHADOW_BUFFER * pShadow = dx9_ShadowBufferGet( nShadowBufferID );
	if ( !pShadow )
		return S_FALSE;
	*(D3DXVECTOR3*)pvOutDir = *(D3DXVECTOR3*)&pShadow->vLightDir;
	pvOutDir->fW = 0.0f;  // Set w 0 so that the translation part doesn't come to play
	D3DXVec4Transform( (D3DXVECTOR4*)pvOutDir, (D3DXVECTOR4*)pvOutDir, (D3DXMATRIXA16*)pmView );  // Direction in view space
	D3DXVec3Normalize( (D3DXVECTOR3*)pvOutDir, (D3DXVECTOR3*)pvOutDir );
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
#define MAX_SHADOW_SCALE 100.0f
// CHB 2007.02.17
static
PRESULT dxC_ShadowBufferSetupDirectional(D3D_SHADOW_BUFFER * pShadow, int nLocation, const CAMERA_INFO * pCameraInfo, const ENVIRONMENT_DEFINITION * pEnvDef)
{
	// HELLGATE magic numbers!
	const float fIndoorRange	= 27.0f;
	const float fOutdoorRange	= 40.0f;

	float fScale = 0.f;

	if ( AppIsHellgate() )
	{
		if ( nLocation == LEVEL_LOCATION_INDOOR || nLocation == LEVEL_LOCATION_FLASHLIGHT 
		  || !TEST_MASK( pShadow->dwFlags, SHADOW_BUFFER_FLAG_USE_GRID ) )
			fScale = fIndoorRange;
		else if ( nLocation == LEVEL_LOCATION_OUTDOOR || nLocation == LEVEL_LOCATION_INDOORGRID )
			fScale = fOutdoorRange;
		else
			return S_FALSE;
	}

	VECTOR vFocus;
	VECTOR vDir;
	

	if ( AppIsHellgate() && ( nLocation == LEVEL_LOCATION_OUTDOOR ) &&
		! TEST_MASK( pShadow->dwFlags, SHADOW_BUFFER_FLAG_RENDER_DYNAMIC ) )
	{
		REGION* pRegion = e_GetCurrentRegion();

		if ( TEST_MASK( pShadow->dwFlags, SHADOW_BUFFER_FLAG_FULL_REGION ) )
		{
			VECTOR vBBCenter = BoundingBoxGetCenter( &pRegion->tBBox );
			vFocus = vBBCenter;

			VECTOR vBBSize = BoundingBoxGetSize( &pRegion->tBBox );
			fScale = sqrtf( VectorLengthSquaredXY( vBBSize ) );
		}
		else if ( TEST_MASK( pShadow->dwFlags, SHADOW_BUFFER_FLAG_USE_GRID ) )
		{
			vFocus = pCameraInfo->vLookAt;

			float fGridSize = fScale / 3.0f; // MAGIC number for when grid updates occur.
			vFocus /= fGridSize;
			vFocus.fX = SIGN( (int)vFocus.fX ) * FLOOR( FABS( vFocus.fX ) ) * fGridSize; 
			vFocus.fY = SIGN( (int)vFocus.fY ) * FLOOR( FABS( vFocus.fY ) ) * fGridSize;
			vFocus.fZ = SIGN( (int)vFocus.fZ ) * FLOOR( FABS( vFocus.fZ ) ) * fGridSize;

			VECTOR vBBSize = BoundingBoxGetSize( &pRegion->tBBox );
			float fRegionScale = sqrtf( VectorLengthSquaredXY( vBBSize ) );

			fScale = fScale * 2.0f;
			fScale = MIN( fScale, fRegionScale );
		}
		else
		{
			vFocus = pCameraInfo->vLookAt;
		}
	}
	else
	{
		float fDist;
		float fCamDist = sqrtf( VectorDistanceSquared( pCameraInfo->vLookAt, pCameraInfo->vPosition ) );

		if ( AppIsTugboat() )
		{

			MATRIX mWorld, mView, mProj;
			e_GetWorldViewProjMatricies( &mWorld, &mView, &mProj, NULL, TRUE );

			CameraGetProjMatrix( pCameraInfo, (MATRIX*)&mProj, TRUE );
			VECTOR pvDir;
			VECTOR pvOrig;
			BOOL bUpFacing = FALSE;
			VECTOR v;
			v.fX =  0 / mProj._11;
			v.fY = 1.0f / mProj._22;
			v.fZ =  1.0f;
			
			MATRIX m;
			MatrixMultiply( &m, &mWorld, &mView );
			MatrixInverse( &m, &m );

			// Transform the screen space pick ray into 3D space
			pvDir.fX  = v.fX*m._11 + v.fY*m._21 + v.fZ*m._31;
			pvDir.fY  = v.fX*m._12 + v.fY*m._22 + v.fZ*m._32;
			pvDir.fZ  = v.fX*m._13 + v.fY*m._23 + v.fZ*m._33;
			pvOrig.fX = m._41;
			pvOrig.fY = m._42;
			pvOrig.fZ = m._43;

			VECTOR vFar;
			float fLength = ( pCameraInfo->vLookAt.fZ - pvOrig.fZ  ) / pvDir.fZ ;
			vFar.fX = pvOrig.fX + ( pvDir.fX * fLength );
			vFar.fY = pvOrig.fY + ( pvDir.fY * fLength );
			vFar.fZ = pCameraInfo->vLookAt.fZ;
			if( fLength <= 0 )
				bUpFacing = TRUE;

			v.fX =  0 / mProj._11;
			v.fY = -1.0f / mProj._22;
			v.fZ =  1.0f;

			// Transform the screen space pick ray into 3D space
			pvDir.fX  = v.fX*m._11 + v.fY*m._21 + v.fZ*m._31;
			pvDir.fY  = v.fX*m._12 + v.fY*m._22 + v.fZ*m._32;
			pvDir.fZ  = v.fX*m._13 + v.fY*m._23 + v.fZ*m._33;
			pvOrig.fX = m._41;
			pvOrig.fY = m._42;
			pvOrig.fZ = m._43;

			VECTOR vNear;
			fLength = ( pCameraInfo->vLookAt.fZ - pvOrig.fZ  ) / pvDir.fZ;
			vNear.fX = pvOrig.fX + ( pvDir.fX * fLength );
			vNear.fY = pvOrig.fY + ( pvDir.fY * fLength );
			vNear.fZ = pCameraInfo->vLookAt.fZ;
			if( fLength <= 0 )
				bUpFacing = TRUE;

			vFocus = vFar + vNear;
			vFocus *= .5f;
			fScale = VectorLength( vFar - vNear );
			if( bUpFacing )
			{
			
				vFocus = pCameraInfo->vLookAt;
				vFar = ( vFocus - pvOrig );
				vFar.fZ = 0;
				vFocus += vFar;
				vFar *= 2.0f;
				fScale = VectorLength( vFar ) * 2.2f;
				if( fScale > MAX_SHADOW_SCALE )
				{
					fScale = MAX_SHADOW_SCALE;
				}
			}
			else
			{

				vFar -= vNear;
				VectorNormalize( vFar );
				if( fScale > MAX_SHADOW_SCALE )
				{
					fScale = MAX_SHADOW_SCALE;
				}
				vFocus = vNear + vFar * fScale * .5f;
				fScale *= 1.2f;

			}

		} 
		else
		{
			fDist = fScale * 0.5f - fCamDist;
			VECTOR vFlatDir;
			e_GetViewDirection( &vFlatDir );
			vFlatDir.z = 0.f;
			VectorNormalize( vFlatDir);
			vFocus = vFlatDir * fDist + pCameraInfo->vLookAt;
			vFocus.fZ = pCameraInfo->vLookAt.fZ;
		}
		
		
	}

	if ( AppIsTugboat() && ( nLocation == LEVEL_LOCATION_INDOOR || nLocation == LEVEL_LOCATION_INDOORGRID ) )
		vDir = VECTOR( 0, 0, -1 );
	else
		e_GetDirLightVector( pEnvDef, ENV_DIR_LIGHT_PRIMARY_DIFFUSE, &vDir );

	V( dxC_ShadowBufferSetDirectionalLightParams( pShadow, vFocus, vDir, fScale ) );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_OverrideShadowBufferScale( int nShadowBuffer, float fScale )
{
	D3D_SHADOW_BUFFER * pShadow = dx9_ShadowBufferGet( nShadowBuffer );
	if ( !pShadow )
		return S_FALSE;

	V( dxC_ShadowBufferSetDirectionalLightParams( pShadow, pShadow->vWorldFocus, pShadow->vLightDir, fScale ) );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

// CHB 2007.02.17
static
PRESULT dxC_ShadowBufferSetupDirectional(D3D_SHADOW_BUFFER * pShadow)
{
	const CAMERA_INFO * pCameraInfo = CameraGetInfo();
	if ( !pCameraInfo )
		return S_FALSE;

	const ENVIRONMENT_DEFINITION * pEnvDef = e_GetCurrentEnvironmentDef();
	if ( !pEnvDef )
		return S_FALSE;

	int nLocation = e_GetCurrentLevelLocation();

	return dxC_ShadowBufferSetupDirectional(pShadow, nLocation, pCameraInfo, pEnvDef);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static
PRESULT sSetupDirectionalShadow()
{
	if ( ! e_ShadowsEnabled() )
		return S_FALSE;

	// set shadow buffer up to surround camera (filling view area)
	const CAMERA_INFO * pCameraInfo = CameraGetInfo();
	if ( !pCameraInfo )
		return S_FALSE;

	const ENVIRONMENT_DEFINITION * pEnvDef = e_GetCurrentEnvironmentDef();
	if ( !pEnvDef )
		return S_FALSE;

	int nLocation = e_GetCurrentLevelLocation();

	float fShadowIntensity = e_GetShadowIntensity( pEnvDef );
	if ( fShadowIntensity == 0.0f )
		return S_FALSE;

	for ( int nShadow = dx9_ShadowBufferGetFirstID();
		nShadow != INVALID_ID;
		nShadow = dx9_ShadowBufferGetNextID( nShadow ) )
	{		
		if ( nLocation != LEVEL_LOCATION_OUTDOOR && nShadow != 0 )
			continue;

		D3D_SHADOW_BUFFER* pShadow = dx9_ShadowBufferGet( nShadow );
		ASSERT_RETFAIL( pShadow );

		V(dxC_ShadowBufferSetupDirectional(pShadow, nLocation, pCameraInfo, pEnvDef));
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_DirtyStaticShadowBuffers()
{
	for ( int nShadow = dx9_ShadowBufferGetFirstID(); 
			  nShadow != INVALID_ID;
			  nShadow = dx9_ShadowBufferGetNextID( nShadow ) )
	{
		D3D_SHADOW_BUFFER * pShadow = dx9_ShadowBufferGet( nShadow );
		ASSERT_CONTINUE( pShadow );

		if ( TEST_MASK( pShadow->dwFlags, SHADOW_BUFFER_FLAG_RENDER_STATIC ) )
			SET_MASK( pShadow->dwFlags, SHADOW_BUFFER_FLAG_DIRTY );
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

// CHB 2007.02.17
PRESULT e_UpdateParticleLighting(void)
{
	if (AppIsTugboat())
	{
		D3D_SHADOW_BUFFER * pShadow = dxC_GetParticleLightingMapShadowBuffer();
		V_RETURN(dxC_ShadowBufferSetupDirectional(pShadow));
	}
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_UpdateShadowBuffers()
{
	ENVIRONMENT_DEFINITION * pEnvDef = e_GetCurrentEnvironmentDef();
	if ( !pEnvDef )
		return S_FALSE;

	float fShadowIntensity = e_GetShadowIntensity( pEnvDef );
	if ( fShadowIntensity == 0.0f )
		return S_FALSE;

	for ( int nShadow = dx9_ShadowBufferGetFirstID();
		nShadow != INVALID_ID;
		nShadow = dx9_ShadowBufferGetNextID( nShadow ) )
	{
		D3D_SHADOW_BUFFER * pShadow = dx9_ShadowBufferGet( nShadow );
		ASSERT_CONTINUE( pShadow );

		if ( pShadow->nWidth == 0 && pShadow->nHeight == 0 )
		{
			D3DC_TEXTURE2D_DESC tDesc;
			// CHB 2006.11.03
			LPD3DCTEXTURE2D pTexture = sGetShadowMapTexture( nShadow );
			if ( !pTexture )
				return S_FALSE;

			V_CONTINUE( dxC_Get2DTextureDesc( pTexture, 0, &tDesc ) );
			pShadow->nWidth  = tDesc.Width;
			pShadow->nHeight = tDesc.Height;

			BOUNDING_BOX& tVP = pShadow->tVP;
			float fWidth = (float)( pShadow->nWidth );
			float fHeight = (float)( pShadow->nHeight );
			tVP.vMin = VECTOR( 0.0f, 0.0f, 0.0f );
			tVP.vMax = VECTOR( fWidth, fHeight, 1.0f );

			SET_MASK( pShadow->dwFlags, SHADOW_BUFFER_FLAG_DIRTY );
		}

		// found active shadows, enable shadow rendering
		e_ShadowsSetActive( TRUE );
	}

	V_RETURN( sSetupDirectionalShadow() );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static
PRESULT sShadowMapSetMatrix(
	D3D_SHADOW_BUFFER * pShadow, 
	const D3D_EFFECT * pEffect, 
	const EFFECT_TECHNIQUE & tTechnique, 
	EFFECT_PARAM eParam, 
	const MATRIX * pmWorld,
	D3D_MODEL * pModel = NULL )
{
	// build matrix	
	//set special texture matrix for shadow mapping
	float fOffsetX = 0.5f DX9_BLOCK( + (0.5f / (float)pShadow->nWidth) );
	float fOffsetY = 0.5f DX9_BLOCK( + (0.5f / (float)pShadow->nHeight) );
	float fBias    = 0.0f; 

	int nShadowType = e_GetActiveShadowType();
	
#if 0
	// AE 2007.06.28 - We now bias the shadow casters rather than the shadow 
	//					receivers.

	// AE 2007.01.08 - Depth biasing is only good for hw shadows because it biases
	//					the z value written to the depth buffer. Unfortunately,
	//					this does not benefit color shadows, which write depth
	//					to the color buffer.
	static BOOL bSupportsDepthBias = dx9_CapGetValue( DX9CAP_SLOPESCALEDEPTHBIAS ) && dx9_CapGetValue( DX9CAP_DEPTHBIAS );
	static float sfDefaultBias = -0.002f;
	static float sfDynamicBias = -0.0007f;
	static float sfFullRegionBiasMultiplier = 5.0f;
	if (   nShadowType != SHADOW_TYPE_DEPTH || ! bSupportsDepthBias
		|| TEST_MASK( pShadow->dwFlags, SHADOW_BUFFER_FLAG_RENDER_DYNAMIC ) )
	{
		// Don't set depth bias for dynamic shadow buffer
		if (   TEST_MASK( pShadow->dwFlags, SHADOW_BUFFER_FLAG_RENDER_DYNAMIC )
			&& ( ! pModel || ! dxC_ModelGetFlagbit( pModel, MODEL_FLAGBIT_ANIMATED ) ) )
			fBias = sfDynamicBias;
		else
		{
			fBias = sfDefaultBias;

			if ( TEST_MASK( pShadow->dwFlags, SHADOW_BUFFER_FLAG_FULL_REGION ) )
				fBias *= sfFullRegionBiasMultiplier;
		}
	}
#endif

	D3DXMATRIXA16 matTexScaleBias(	0.5f,	  0.0f,     0.0f,   0.0f,
									0.0f,    -0.5f,		0.0f,   0.0f,
									0.0f,     0.0f,     1.0f,	0.0f,
									fOffsetX, fOffsetY, fBias,  1.0f );

	D3DXMATRIXA16 matLightViewProj = *(D3DXMATRIXA16*)&pShadow->mShadowView *	*(D3DXMATRIXA16*)&pShadow->mShadowProj;
	D3DXMATRIXA16 matTex =			 *(D3DXMATRIXA16*)pmWorld *					matLightViewProj						*	matTexScaleBias;

	V_RETURN( dx9_EffectSetMatrix( pEffect, tTechnique, eParam, &matTex ) );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_SetParticleLightingMapParameters(const D3D_EFFECT * pEffect, const EFFECT_TECHNIQUE & tTechnique, const MATRIX * pmWorld)
{
	if (AppIsTugboat())
	{
		D3D_SHADOW_BUFFER * pShadow = dxC_GetParticleLightingMapShadowBuffer();
		V_RETURN(sShadowMapSetMatrix(pShadow, pEffect, tTechnique, EFFECT_PARAM_SHADOW_MATRIX, pmWorld));

		SPD3DCBASESHADERRESOURCEVIEW pLightingMapTexture = dxC_RTShaderResourceGet( dxC_GetGlobalRenderTargetIndex(GLOBAL_RT_LIGHTING_MAP) );
		if (pLightingMapTexture == 0)
		{
			return S_FALSE;
		}
		V_RETURN( dx9_EffectSetTexture( pEffect, tTechnique, EFFECT_PARAM_PARTICLE_LIGHTMAP, pLightingMapTexture ) );
	}
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_SetShadowMapParameters(
	const D3D_EFFECT * pEffect,
	const EFFECT_TECHNIQUE & tTechnique, 
	int nShadow, 
	const ENVIRONMENT_DEFINITION * pEnvDef,
	const MATRIX * pmWorld, 
	const MATRIX * pmView, 
	const MATRIX * pmProj,
	BOOL bForceCurrentEnv )
{
	MATRIX matWorldView; // = (*pmWorld) * (*pmView);
	MatrixMultiply( &matWorldView, pmWorld, pmView );
	V_RETURN( dx9_EffectSetMatrix( pEffect, tTechnique, EFFECT_PARAM_WORLDVIEW,  &matWorldView ) );
	V_RETURN( dx9_EffectSetMatrix( pEffect, tTechnique, EFFECT_PARAM_PROJECTIONMATRIX,	pmProj ) );

	D3D_SHADOW_BUFFER * pShadow = dx9_ShadowBufferGet( nShadow );
	ASSERT_RETINVALIDARG( pShadow );

	if ( pShadow->bDirectionalLight )
	{
		VECTOR4 vLightDir( pShadow->vLightDir, 0 );
		V_RETURN( dx9_EffectSetVector( pEffect, tTechnique, EFFECT_PARAM_SHADOW_LIGHT_DIR, &vLightDir ) );
	}
	else
	{
		VECTOR4 vLightPos;
		VECTOR4 vLightDir;
		V( e_ShadowBufferGetLightPosInWorldView( nShadow, pmView, &vLightPos ) );
		V( e_ShadowBufferGetLightDirInWorldView( nShadow, pmView, &vLightDir ) );
		//V_RETURN( dx9_EffectSetVector( pEffect, tTechnique, EFFECT_PARAM_SHADOW_LIGHT_POS,	    &vLightPos ) );
		V_RETURN( dx9_EffectSetVector( pEffect, tTechnique, EFFECT_PARAM_SHADOW_LIGHT_DIR,	    &vLightDir ) );
		//V_RETURN( dx9_EffectSetFloat(  pEffect, tTechnique, EFFECT_PARAM_SHADOW_CONE_COS_THETA,	e_ShadowBufferGetCosTheta( nShadow ) ) );
	}

	int nLocation = e_GetCurrentLevelLocation();

	if ( AppIsHellgate() && nLocation == LEVEL_LOCATION_OUTDOOR )
	{
		D3D_SHADOW_BUFFER * pShadow2 = dx9_ShadowBufferGet( 0 );
		ASSERT_RETFAIL( TEST_MASK( pShadow2->dwFlags, SHADOW_BUFFER_FLAG_EXTRA_OUTDOOR_PASS ) );
		V_RETURN( sShadowMapSetMatrix( pShadow2, pEffect, tTechnique, EFFECT_PARAM_SHADOW_MATRIX2, pmWorld ) );

		LPD3DCBASESHADERRESOURCEVIEW pShadow2Texture;
		if( pShadow2->eRenderTarget != RENDER_TARGET_NULL )
			pShadow2Texture = dxC_RTShaderResourceGet( pShadow2->eRenderTarget );

		GLOBAL_DEPTH_TARGET tGDT = dxC_GetGlobalDepthTargetFromShadowMap( pShadow2->eShadowMap );
		DEPTH_TARGET_INDEX tDT = dxC_GetGlobalDepthTargetIndex( tGDT );
		LPD3DCBASESHADERRESOURCEVIEW pShadow2TextureDepth = dxC_DSShaderResourceView( tDT );

		V_RETURN( dx9_EffectSetTexture( pEffect, tTechnique, EFFECT_PARAM_SHADOWMAPDEPTH, 
			(e_GetActiveShadowType() == SHADOW_TYPE_DEPTH) ? pShadow2TextureDepth : pShadow2Texture ) );
	}

	// CHB 2007.02.17
	V_RETURN(sShadowMapSetMatrix(pShadow, pEffect, tTechnique, EFFECT_PARAM_SHADOW_MATRIX, pmWorld));

	// CML 2007.04.24 - Shadow Intensity factor effect const set moved to dxC_render for constant packing purposes

    // set the shadow map texture
    int nStage = 0;

    // TRAVIS : changing to actual tex instead of depthbuffer
    LPD3DCBASESHADERRESOURCEVIEW pShadowTexture;
	if( pShadow->eRenderTarget != RENDER_TARGET_NULL )
		pShadowTexture = dxC_RTShaderResourceGet( pShadow->eRenderTarget );

    if ( AppIsTugboat() && !pShadowTexture )
	    return S_FALSE;


	GLOBAL_DEPTH_TARGET tGDT = dxC_GetGlobalDepthTargetFromShadowMap( pShadow->eShadowMap );
	DEPTH_TARGET_INDEX tDT = dxC_GetGlobalDepthTargetIndex( tGDT );
	LPD3DCBASESHADERRESOURCEVIEW pShadowTextureDepth = dxC_DSShaderResourceView( tDT );

	V( dxC_SetSamplerDefaults( SAMPLER_SHADOW ) );
	V( dxC_SetSamplerDefaults( SAMPLER_SHADOWDEPTH ) );

	dxC_EffectSetShadowSize( pEffect, tTechnique, pShadow->nWidth, pShadow->nHeight );

    if ( TRUE )
    {
		if ( AppIsTugboat() )
		{
#ifndef ENGINE_TARGET_DX10  //No color targets in Dx10
			V_RETURN( dx9_EffectSetTexture( pEffect, tTechnique, EFFECT_PARAM_SHADOWMAP, pShadowTexture ) );
#endif
			V_RETURN( dx9_EffectSetTexture( pEffect, tTechnique, EFFECT_PARAM_SHADOWMAPDEPTH, pShadowTextureDepth ) );
		}
		else
		{
			V_RETURN( dx9_EffectSetTexture( pEffect, tTechnique, EFFECT_PARAM_SHADOWMAP, (e_GetActiveShadowType() == SHADOW_TYPE_DEPTH) ? pShadowTextureDepth : pShadowTexture ) );
		}
    } 
    else if ( nStage >= 0 ) 
    {
#ifndef ENGINE_TARGET_DX10  //No color targets in Dx10
	    V_RETURN( dxC_SetTexture( nStage, pShadowTexture ) );
#endif
    }

#if 0
	if ( e_GetRenderFlag( RENDER_FLAG_SHADOWS_SHOWAREA ) )
	{
		DWORD dwColorWriteEnable = 0;

		if ( pShadow->dwDebugColor & 0x00ff0000 )
			dwColorWriteEnable = D3DCOLORWRITEENABLE_RED;
		else if ( pShadow->dwDebugColor & 0x0000ff00 )
			dwColorWriteEnable = D3DCOLORWRITEENABLE_GREEN;
		else if ( pShadow->dwDebugColor & 0x000000ff )
			dwColorWriteEnable = D3DCOLORWRITEENABLE_BLUE;

		if ( dwColorWriteEnable )
			dxC_SetRenderState( D3DRS_COLORWRITEENABLE, dwColorWriteEnable );			
	}
#endif

	return S_OK;
}

PRESULT e_PCSSSetLightSize(float fSize, float fScale )
{
#ifdef ENGINE_TARGET_DX10
	float fVec[4] = { fSize, fScale, 0.0f, 0.0f };
	V_RETURN( dx10_GetGlobalEffect()->GetVariableByName( "gfPCSSLightSize" )->AsVector()->SetFloatVector( fVec ) );
#endif

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

namespace
{

class LocalEventHandler : public OptionState_EventHandler
{
	public:
		virtual void Update(OptionState * pState) override;
		virtual void QueryActionRequired(const OptionState * pOldState, const OptionState * pNewState, unsigned & nActionFlagsOut) override;
};

void LocalEventHandler::Update(OptionState * pState)
{
	int nShadowReductionFactor = pState->get_nShadowReductionFactor();
	bool bValid =
		e_IsShadowTypeAvailable( pState, pState->get_nShadowType() ) &&
		( nShadowReductionFactor >= 0 ) && ( nShadowReductionFactor <= 1 );

	if ( ! bValid )
	{
		pState->set_nShadowType( SHADOW_TYPE_NONE );
		pState->set_nShadowReductionFactor( 0 );
	}
}

void LocalEventHandler::QueryActionRequired(const OptionState * pOldState, const OptionState * pNewState, unsigned & nActionFlagsOut)
{
	// See if any changes were made.
	bool bChanged =
		(pOldState->get_nShadowType() != pNewState->get_nShadowType()) ||
		(pOldState->get_nShadowReductionFactor() != pNewState->get_nShadowReductionFactor());
	if (!bChanged)
	{
		return;
	}

	// Set up the new shadow type.
//	V(e_ShadowsCreate(pNewState));
	nActionFlagsOut |= SETTING_NEEDS_RESET;		// resetting the device is the easiest way to go today

	// Effects loaded depend on the shadow type, so reload when that changes.
	// CHB 2007.03.20 - With the new technique caching system, it's no longer
	// necessary to reload effects when changing shadow type.  Previously,
	// under the technique matrix system, shadowing was a single bit for
	// on/off, which meant that shadow type selection was performed at effect
	// load time in order to avoid having to add yet another dimension to
	// the technique matrix.  Thank you for the new & improved way, Chris!
#if 0
	if (pOldState->get_nShadowType() != pNewState->get_nShadowType())
	{
		nActionFlagsOut |= SETTING_NEEDS_FXRELOAD;
	}
#endif
}

};	// anonymous namespace

//-------------------------------------------------------------------------------------------------

PRESULT dxC_Shadow_RegisterEventHandler(void)
{
	return e_OptionState_RegisterEventHandler(new LocalEventHandler);
}

