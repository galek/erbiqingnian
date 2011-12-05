//----------------------------------------------------------------------------
// dxC_shadow.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DXC_SHADOW__
#define __DXC_SHADOW__

#include "e_shadow.h"
#include "dxC_effect.h"
#include "dxC_target.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------
#define SHADOW_RANGE_FUDGE			1.2f

#define SHADOW_BUFFER_FLAG_DIRTY				MAKE_MASK(0)
#define SHADOW_BUFFER_FLAG_USE_COLOR_BUFFER		MAKE_MASK(1)
#define SHADOW_BUFFER_FLAG_RENDER_STATIC		MAKE_MASK(2)
#define SHADOW_BUFFER_FLAG_RENDER_DYNAMIC		MAKE_MASK(3)
#define SHADOW_BUFFER_FLAG_ALWAYS_DIRTY			MAKE_MASK(4)
#define SHADOW_BUFFER_FLAG_EXTRA_OUTDOOR_PASS	MAKE_MASK(5)
#define SHADOW_BUFFER_FLAG_FULL_REGION			MAKE_MASK(6)
#define SHADOW_BUFFER_FLAG_USE_GRID				MAKE_MASK(7)

//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

struct D3D_SHADOW_BUFFER
{
	DWORD										dwFlags;				// Shadow flags
	SHADOWMAP									eShadowMap;				// Shadow map being used
	RENDER_TARGET_INDEX							eRenderTarget;			// Render target used with shadow map
	DXC_9_10( RENDER_TARGET_INDEX, SHADOWMAP )	eRenderTargetPrimer;	// A render target that is used to prime the shadow map, NOTE: With DX10 these are actual shadow maps

	MATRIX										mShadowView;			// View matrix for shadow map ( from light )
	MATRIX										mShadowProj;        	// Projection matrix for shadow map
	MATRIX										mShadowProjBiased;      // Biased projection matrix for use when rendering to shadow map 
	float										fFOV;					// Field of view of the shadow cone
	VECTOR										vLightPos;				// Light position in world
	VECTOR										vLightDir;				// Light direction in world
	float										fScale;					// Scale of the dir light box/cylinder
	int											nWidth;					// Width of shadow buffer (in pixels)
	int											nHeight;				// Height of shadow buffer (in pixels)
	BOUNDING_BOX								tVP;					// Viewport in shadow map
	BOOL										bDirectionalLight;		// shadow map for a directional light
	VECTOR										vWorldFocus;			// shadow map focus point in world
	DWORD										dwDebugColor;			// color used when rendering debug output
	PLANE										tOBBPlanes[ 6 ];		// planes representing the actual SM coverage area
};

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

PRESULT dx9_SetShadowMapParameters(
	const D3D_EFFECT * pEffect,
	const EFFECT_TECHNIQUE & tTechnique, 
	int nShadow, 
	const ENVIRONMENT_DEFINITION * pEnvDef,
	const MATRIX * pmWorld, 
	const MATRIX * pmView, 
	const MATRIX * pmProj,
	BOOL bForceCurrentEnv = FALSE );
PRESULT dx9_ShadowsInit();
PRESULT dx9_ShadowsDestroy();
PRESULT dxC_DirtyStaticShadowBuffers();
PRESULT dxC_OverrideShadowBufferScale( int nShadowBuffer, float fScale );
D3D_SHADOW_BUFFER * dxC_GetParticleLightingMapShadowBuffer(void);	// CHB 2007.02.17
PRESULT dx9_SetParticleLightingMapParameters(const D3D_EFFECT * pEffect, const EFFECT_TECHNIQUE & tTechnique, const MATRIX * pmWorld);	// CHB 2007.02.17

PRESULT dxC_ShadowBufferNew( int nShadowMap, RENDER_TARGET_INDEX nRenderTarget, DWORD dwFlags = 0, int nRenderTargetPrimer = -1 );


//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

inline D3D_SHADOW_BUFFER * dx9_ShadowBufferGet( int nShadowBufferID )
{
	extern CArrayIndexed<D3D_SHADOW_BUFFER>	gtShadowBuffers;
	D3D_SHADOW_BUFFER * pShadowBuffer = gtShadowBuffers.Get( nShadowBufferID );
	ASSERT_RETNULL( pShadowBuffer );
	return pShadowBuffer;
}

inline int dx9_ShadowBufferGetFirstID()
{
	extern CArrayIndexed<D3D_SHADOW_BUFFER>	gtShadowBuffers;
	return gtShadowBuffers.GetFirst();
}

inline int dx9_ShadowBufferGetNextID( int nShadowBufferID )
{
	extern CArrayIndexed<D3D_SHADOW_BUFFER>	gtShadowBuffers;
	return gtShadowBuffers.GetNextId( nShadowBufferID );
}

inline int dx9_ShadowBufferGetCount()
{
	extern CArrayIndexed<D3D_SHADOW_BUFFER>	gtShadowBuffers;
	return gtShadowBuffers.Count();
}

#endif // __DXC_SHADOW__