//----------------------------------------------------------------------------
// dxC_state.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DXC_STATE__
#define __DXC_STATE__

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define MAX_CLIP_PLANES			DXC_9_10(6, 8)

#include "dxC_statemanager.h"
#include "dxC_fvf.h"

#if defined(ENGINE_TARGET_DX9)
	#include "dx9_state.h"
#else if defined(ENGINE_TARGET_DX10)
	#include "dx10_state.h"
#endif

#include "boundingbox.h"
#include "dxC_model.h"

#include "dxC_pixo.h"

#if defined(ENGINE_TARGET_DX9)
#	define TRY_STATE_MANAGER( statemanager, method )	\
			if( statemanager )							\
				{V_RETURN( statemanager->method );}		\
			else										\
				{V_RETURN( dxC_GetDevice()->method );}
#elif defined(ENGINE_TARGET_DX10)
#	define TRY_STATE_MANAGER( statemanager, method )	\
			ASSERT_RETFAIL( statemanager );				\
			V_RETURN( statemanager->method );
#endif


using namespace FSSE;

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

// simple world space position and color
struct XYZ_COL_VERTEX
{
	VECTOR pos;
	DWORD  col;
}; // 16 byte

// simple world space position, color, and uv
struct XYZ_COL_UV_VERTEX
{
	VECTOR pos;
	DWORD  col;
	VECTOR2 uv;
}; // 24 byte


//----------------------------------------------------------------------------
// DECLARATIONS
//----------------------------------------------------------------------------
PRESULT dxC_ResetViewport( BOOL bSkyboxDepth = FALSE );
PRESULT dxC_ViewportGetDimensions( D3DC_VIEWPORT & tVP, RENDER_TARGET_INDEX eRT, BOOL bSkyboxDepth = FALSE, int nSwapChainIndex = INVALID_ID );
PRESULT dxC_GetFullViewportDesc( D3DC_VIEWPORT & tVP, BOOL bSkyboxDepth = FALSE, int nSwapChainIndex = INVALID_ID );
PRESULT dxC_ResetFullViewport( BOOL bSkyboxDepth = FALSE, int nSwapChainIndex = INVALID_ID );
PRESULT dxC_GetFullScissorRect( E_RECT & tFullRect );
PRESULT dxC_SetScissorRect ( const RECT * ptRect, UINT index = 0 );
PRESULT dxC_SetScissorRect ( BOUNDING_BOX & tBBox, UINT index = 0 );
PRESULT dxC_PushClipPlane( const D3DXVECTOR3 * pVec1, const D3DXVECTOR3 * pVec2, const D3DXVECTOR3 * pVec3 );
PRESULT dxC_PushClipPlane( const PLANE * pPlane );
PRESULT dxC_PopClipPlane();
PRESULT dxC_ResetClipPlanes( BOOL bCommit = TRUE );
PRESULT dxC_CommitClipPlanes();
PRESULT dxC_UpdateDebugRenderStates();

#if defined(ENGINE_TARGET_DX9)
	PRESULT dx9_VertexDeclarationsRestore();
	PRESULT dx9_VertexDeclarationsDestroy();
#elif defined(ENGINE_TARGET_DX10)
	PRESULT dx10_CreateIAObject( VERTEX_DECL_TYPE vType, ID3D10EffectPass* pPass, LPD3DCIALAYOUT* ppD3DIA );
#endif
PRESULT dx9_SetFVF( D3DC_FVF dwFVF );

PRESULT dxC_SetTextureConditional ( 
	D3D_MODEL * pModel,
	TEXTURE_TYPE eType,
	int nLOD,	// CHB 2006.11.28
	const EFFECT_TECHNIQUE & tTechnique,
	const D3D_EFFECT * pEffect, 
	EFFECT_PARAM eParam,
	int nStage, 
	const D3D_MODEL_DEFINITION * pModelDef,
	const D3D_MESH_DEFINITION * pMesh,
	int nMeshIndex,
	BOOL bAllowDbgMipOverride = FALSE,
	const MATERIAL * pMaterial = NULL );
PRESULT dxC_SetTexture ( 
	int nTextureId, 
	const EFFECT_TECHNIQUE & tTechnique,
	const D3D_EFFECT * pEffect, 
	EFFECT_PARAM eParam, 
	int nStage, 
	const D3D_MODEL_DEFINITION * pModelDef = NULL,
	const MATERIAL * pMaterial = NULL,
	TEXTURE_TYPE eDefaultType = TEXTURE_NONE,
	BOOL bAllowDbgMipOverride = FALSE );
PRESULT dxC_SetViewport( 
	DWORD dwULX, 
	DWORD dwULY,
	DWORD dwWidth,
	DWORD dwHeight,
	float fMinZ,
	float fMaxZ );
PRESULT dxC_SetViewport( const D3DC_VIEWPORT& tVP );
PRESULT dxC_GetViewport( D3DC_VIEWPORT& tVP );

PRESULT dxC_SetBones(
	const D3D_MODEL * pModel,
	int nMesh,
	int nLOD,	// CHB 2006.11.30
	const D3D_EFFECT * pEffect,
	const EFFECT_TECHNIQUE & tTechnique );

PRESULT dxC_SetBonesEx(
	const D3D_MODEL * pModel,
	int nMesh,
	const D3D_EFFECT * pEffect,
	const EFFECT_TECHNIQUE & tTechnique,
	const float * pfBones ) ;

PRESULT dxC_SetClipPlaneRect ( const D3DXVECTOR3 * pvCorners, int nCorners, const D3DXMATRIXA16 * pmatProj, BOOL bCommit = TRUE );
PRESULT dxC_SetClipPlaneRect ( const BOUNDING_BOX * ptBBox, const D3DXMATRIXA16 * pmatProj, BOOL bCommit = TRUE );

PRESULT dxC_SetStreamSource( UINT StreamNumber, LPD3DCVB pStreamData, UINT OffsetInBytes, UINT Stride );
PRESULT dxC_SetStreamSource( UINT StreamNumber, LPD3DCVB pStreamData, UINT OffsetInBytes, VERTEX_DECL_TYPE eType, D3DC_FVF dwFVF, UINT Stride, const D3D_EFFECT* pEffect = NULL );
PRESULT dxC_SetStreamSource( struct D3D_VERTEX_BUFFER_DEFINITION& tVBDef, UINT OffsetInBytes, const D3D_EFFECT* pEffect = NULL );

//inline void dxC_SetIndices( SPD3DCIB )
//{
//	//KMNV TODO: Shouldn't be necessary
//}
PRESULT dxC_SetIndices( LPD3DCIB pIB, D3DFORMAT ibFormat );
PRESULT dxC_SetIndices( const struct D3D_INDEX_BUFFER_DEFINITION & tIBDef, BOOL bForce = FALSE );

inline PRESULT dxC_SetTexture( DWORD dwStage, LPD3DCBASESHADERRESOURCEVIEW pTexture )
{
	TRY_STATE_MANAGER( gpStateManager, SetTexture( dwStage, pTexture ) );

	gnStateStatsNonEffectCalls++;
	gnStateStatsTotalCalls++;

	return S_OK;
}

STATE_MANAGER_TYPE dx9_SetStateManager( STATE_MANAGER_TYPE eType = STATE_MANAGER_INVALID );
PRESULT dx9_StateManagerDestroy();
void dx9_DirtyStateCache();
WCHAR * dx9_GetStateStats( int nMaxLen );
void dx9_ResetFrameStateStats();
void dx9_AddStatesTypeStats( WCHAR * pwszStr, UINT * pnTotalChangesPerType, UINT * pnActualChangesPerType );
void dx9_AddStatesNonEffectStats( WCHAR * pwszStr, UINT nNonEffectChanges );
PRESULT dxC_StateGetStats( STATE_SET_TYPE eStat, UINT & nActual, UINT & nTotal );

#if defined(ENGINE_TARGET_DX9)
DWORD dx9_GetUsage( D3DC_USAGE usage );
D3DPOOL dx9_GetPool( D3DC_USAGE usage );

#elif defined(ENGINE_TARGET_DX10)
D3D10_CPU_ACCESS_FLAG dx10_GetCPUAccess( D3DC_USAGE usage );
D3D10_USAGE dx10_GetUsage( D3DC_USAGE usage );
D3D10_BIND_FLAG dx10_GetBindFlags( D3DC_USAGE usage );
#endif

UINT dxC_GetVertexSize( int nStream, const D3D_VERTEX_BUFFER_DEFINITION* pVertexBufferDef, const struct D3D_EFFECT * pEffect = NULL );
UINT dxC_GetStreamCount( const D3D_VERTEX_BUFFER_DEFINITION* pVertexBufferDef, const struct D3D_EFFECT* pEffect = NULL );
PRESULT dxC_SetVertexDeclaration( VERTEX_DECL_TYPE eType, const struct D3D_EFFECT * pEffect = NULL );
#if defined(ENGINE_TARGET_DX9)
	#define dxC_SetRenderState dxC_SetRenderState
#elif defined(ENGINE_TARGET_DX10)
	PRESULT dxC_SetRenderState(D3DRS_RASTER tState, DWORD_PTR dwValue, bool force = false);
	PRESULT dxC_SetRenderState(D3DRS_BLEND tState, DWORD_PTR dwValue, bool force = false);
	PRESULT dxC_SetRenderState(DRDRS_DS tState, DWORD_PTR dwValue, bool force = false);
#endif


//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------
inline PRESULT dxC_SetTransform( D3DTRANSFORMSTATETYPE tState, const D3DXMATRIXA16 * pMatrix )
{
	if ( dxC_IsPixomaticActive() )
		return S_FALSE;

	TRY_STATE_MANAGER( gpStateManager, SetTransform( tState, pMatrix ));

	gnStateStatsNonEffectCalls++;
	gnStateStatsTotalCalls++;

	return S_OK;
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

inline PRESULT dxC_SetAlphaTestEnable( BOOL bEnable )
{
	return dxC_SetRenderState( D3DRS_ALPHATESTENABLE, bEnable );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

inline PRESULT dxC_SetAlphaBlendEnable( BOOL bEnable )
{
#ifdef ENGINE_TARGET_DX9
	if ( e_GetRenderFlag( RENDER_FLAG_ONE_BIT_ALPHA ) )
	{
		ONCE
		{
			ASSERT_BREAK( dx9_CapGetValue( DX9CAP_SEPARATE_ALPHA_BLEND_ENABLE ) );
			// Use separate alpha blend enable
			dxC_SetRenderState( D3DRS_SEPARATEALPHABLENDENABLE, bEnable );
			if ( bEnable )
			{
				dxC_SetRenderState( D3DRS_BLENDOPALPHA,   D3DBLENDOP_MAX );
				dxC_SetRenderState( D3DRS_SRCBLENDALPHA,  D3DBLEND_SRCALPHA );
				dxC_SetRenderState( D3DRS_DESTBLENDALPHA, D3DBLEND_DESTALPHA );
			}
		}
	}
#endif // DX9
	return dxC_SetRenderState( D3DRS_ALPHABLENDENABLE, bEnable );
}


#endif //__DXC_STATE__
