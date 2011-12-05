//----------------------------------------------------------------------------
// dx9_common.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"
#include "dxC_state.h"
#include "dxC_buffer.h"
#include "interpolationpath.h"
#include "perfhier.h"
#include "e_profile.h"
#include "dxC_effect.h"
#include "dxC_target.h"
#include "dxC_pixo.h"


using namespace FSSE;

extern CStateManagerInterface *		gpStateManager;

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------\

SPD3DCVB							gpFSQuad = NULL;
BOOL								gbPrefetchVB = FALSE;

//----------------------------------------------------------------------------
// IMPLEMENTATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_InitPlatformGlobals()
{
	DX9_BLOCK
	(
		// create a material for rendering no-texture objects
		ZeroMemory( &gStubMtrl, sizeof( D3DMATERIAL9 ) );
		gStubMtrl.Diffuse.r = gStubMtrl.Ambient.r = 1.0f;
		gStubMtrl.Diffuse.g = gStubMtrl.Ambient.g = 1.0f;
		gStubMtrl.Diffuse.b = gStubMtrl.Ambient.b = 1.0f;
		gStubMtrl.Diffuse.a = gStubMtrl.Ambient.a = 1.0f;

		// create a stub light for filling light state structures
		ZeroMemory( &gStubLight, sizeof( D3DLIGHT9 ) );
		gStubLight.Diffuse.r = gStubLight.Ambient.r = 1.0f;
		gStubLight.Diffuse.g = gStubLight.Ambient.g = 1.0f;
		gStubLight.Diffuse.b = gStubLight.Ambient.b = 1.0f;
		gStubLight.Diffuse.a = gStubLight.Ambient.a = 1.0f;
		gStubLight.Type = D3DLIGHT_POINT;
		gStubLight.Direction.z = 1.0f;
		gStubLight.Attenuation0 = 1.0f;
	)
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL e_IsDX10Enabled()
{
	return DXC_9_10( FALSE, TRUE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static BOOL sIsD3DResult( PRESULT pr )
{
	return ( PRESULT_FACILITY( pr ) == _FACD3D );
}

static BOOL sIsEngineResult( PRESULT pr )
{
	return ( PRESULT_FACILITY( pr ) == FACILITY_PRIME_ENGINE );
}


// return the DX error string and definition as a single multi-line string
const TCHAR * e_GetResultString( PRESULT pr )
{
	const int cnMaxLen = 512;
	static TCHAR szResult[ cnMaxLen ];

	szResult[ 0 ] = NULL;

	if ( sIsD3DResult( pr ) )
	{
		const char * pszString  = DXGetErrorString( static_cast<HRESULT>(pr) );
		//const char * pszString9 = DXGetErrorString9( hr );
		const char * pszDesc    = DXGetErrorDescription( static_cast<HRESULT>(pr) );

		PStrPrintf( szResult, cnMaxLen, "%s: %s",
			pszString,
			pszDesc );

		return szResult;
	}
	else if ( sIsEngineResult( pr ) )
	{
		// When we have custom engine error messages, we would fill them in here
		switch ( pr )
		{
		case S_PE_TRY_AGAIN:
			PStrPrintf( szResult, cnMaxLen, "S_PE_TRY_AGAIN" );
			return szResult;
		}
	}

	return NULL;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

 PRESULT dxC_DrawFullscreenQuad(
	QUAD_TYPE QuadType,
	D3D_EFFECT* pEffect,
	const EFFECT_TECHNIQUE& tTechnique )
{
	V_RETURN( dxC_EffectSetPixelAccurateOffset( pEffect, tTechnique ) );
	V_RETURN( dxC_DrawQuad( QuadType, pEffect, tTechnique ) );
	return S_OK;
}

 //----------------------------------------------------------------------------
 //----------------------------------------------------------------------------

 PRESULT dxC_DrawQuad(
	 QUAD_TYPE QuadType,
	 D3D_EFFECT* pEffect,
	 const EFFECT_TECHNIQUE& tTechnique )
 {
	 V_RETURN( dxC_SetQuad( QuadType, pEffect ) );
	 V_RETURN( dxC_DrawPrimitive( D3DPT_TRIANGLELIST, 0, 2, pEffect ) );
	 V_RETURN( dxC_TargetSetCurrentDirty() );
	 return S_OK;
 }

 //----------------------------------------------------------------------------
 //----------------------------------------------------------------------------

#if ISVERSION( DEBUG_VERSION )
void sVerifyRTsAreClear()
{
	if ( ! e_GetRenderFlag( RENDER_FLAG_RENDERTARGET_VERIFY_CLEAR ) )
		return; 

	const int cnNameLen = 64;
	char szName[cnNameLen];

	UINT nRTCount = dxC_GetCurrentRenderTargetCount();
	for ( UINT i = 0; i < nRTCount; i++ )		
	{
		DWORD dwClearToken = 0;
		RENDER_TARGET_INDEX eRT = dxC_GetCurrentRenderTarget( i );

		//if ( eRT < NUM_STATIC_RENDER_TARGETS )
		{
			dxC_RTGetClearToken( eRT, dwClearToken );
			if ( dwClearToken != e_GetVisibilityToken() )
				dxC_GetRenderTargetName( eRT, szName, cnNameLen );
			ASSERTV( dwClearToken == e_GetVisibilityToken(), 
				"Render Target %s was not cleared this frame",
				szName );
		}
	}
}
#else
#define sVerifyRTsAreClear()
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------


PRESULT dxC_DrawPrimitive(
	D3DC_PRIMITIVE_TOPOLOGY PrimitiveType,
	UINT StartVertex,
	UINT PrimitiveCount,
	D3D_EFFECT* pEffect,
	METRICS_GROUP eGroup,
	UINT iPassIndex )
{
	V_RETURN( dxC_UpdateDebugRenderStates() );

	if ( dxC_IsPixomaticActive() )
	{
		V( dxC_PixoDrawPrimitive( PrimitiveType, PrimitiveCount ) );
		goto report;
	}

	if ( dxC_DPGetPrefetch() )
	{
		PrimitiveCount = 1;
	}

#ifdef ENGINE_TARGET_DX9
	if ( pEffect )
	{
		PERF( DRAW_FX_COMMIT );
		V_RETURN( pEffect->pD3DEffect->CommitChanges() );
	}

#elif defined( ENGINE_TARGET_DX10 )
	if( !pEffect )
#ifdef DX10_EMULATE_FIXED_FUNC
		if( ( pEffect = dxC_EffectGet( NONMATERIAL_EFFECT_FIXED_FUNC_EMULATOR ) ) && EFFECT_INTERFACE_ISVALID( pEffect->pD3DEffect ) )
		{
			EFFECT_TECHNIQUE* pTech; //= dx9_EffectGetTechniquePtrGeneral( pEffect, 0 );
			TECHNIQUE_FEATURES tFeat;
			tFeat.Reset();
			tFeat.nInts[ TF_INT_INDEX ] = 0;
			V_RETURN( dxC_EffectGetTechniqueByFeatures( pEffect, tFeat, &pTech ) );
			V_RETURN( dxC_SetTechnique( pEffect, pTech, NULL ) );
			V_RETURN( dxC_EffectBeginPass( pEffect, 0 ) );
		}
		else
#endif
			return E_FAIL; //Effect not loaded, fail draw call

	dxC_GetDevice()->IASetPrimitiveTopology( PrimitiveType );
	V_RETURN( gpStateManager->ApplyBeforeRendering( pEffect ) );
#endif

	V( dxC_DebugValidateDevice() );
	sVerifyRTsAreClear();

	ONCE
	{
		PERF( DRAW_DRAWPRIM );
#if defined(ENGINE_TARGET_DX9)
		V_CONTINUE( dxC_GetDevice()->DrawPrimitive(
			PrimitiveType,
			StartVertex,
			PrimitiveCount ) );

#elif defined(ENGINE_TARGET_DX10)
		dxC_GetDevice()->Draw( dx10_TopologyVertexCount( PrimitiveType, PrimitiveCount ), StartVertex );
#endif
	}

	V( dxC_TargetSetCurrentDirty() );

report:
	e_ReportBatch( eGroup );
	e_ReportPolygons( PrimitiveCount, eGroup );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_DrawIndexedPrimitive(
	D3DC_PRIMITIVE_TOPOLOGY PrimitiveType,
	INT BaseVertexIndex,
	UINT MinVertexIndex,
	UINT NumVertices,
	UINT startIndex,
	UINT primCount,
	D3D_EFFECT* pEffect,
	METRICS_GROUP eGroup,
	UINT iPassIndex )
{
	V_RETURN( dxC_UpdateDebugRenderStates() );


	if ( dxC_IsPixomaticActive() )
	{
		V( dxC_PixoDrawIndexedPrimitive( PrimitiveType, BaseVertexIndex, NumVertices, startIndex, primCount ) );
		goto report;
	}

	// If we are delaying the update of a vertex buffer, we have to use the prefetch index buffer as well.
	if ( dxC_DPGetPrefetch() )
	{
		V( dxC_SetPrefetchIndices() );
		NumVertices = 3;
		primCount = 1;
	}

#ifdef ENGINE_TARGET_DX9
	if ( pEffect )
	{
		PERF( DRAW_FX_COMMIT );
		V_RETURN( pEffect->pD3DEffect->CommitChanges() );
	}

#elif defined( ENGINE_TARGET_DX10 )
	if( !pEffect )
#ifdef DX10_EMULATE_FIXED_FUNC
		if( ( pEffect = dxC_EffectGet( dx9_EffectGetNonMaterialID( NONMATERIAL_EFFECT_FIXED_FUNC_EMULATOR ) ) ) && EFFECT_INTERFACE_ISVALID( pEffect->pD3DEffect ) )
		{
			EFFECT_TECHNIQUE* pTech; // = dx9_EffectGetTechniquePtrGeneral( pEffect, 0 );
			TECHNIQUE_FEATURES tFeat;
			tFeat.Reset();
			tFeat.nInts[ TF_INT_INDEX ] = 0;
			V_RETURN( dxC_EffectGetTechniqueByFeatures( pEffect, tFeat, &pTech ) );
			V_RETURN( dxC_SetTechnique( pEffect, pTech, NULL ) );
			V_RETURN( dxC_EffectBeginPass( pEffect, 0 ) );
		}
		else
#endif
			return E_FAIL; //Effect not loaded, fail draw call

	dxC_GetDevice()->IASetPrimitiveTopology( PrimitiveType ); 
	V_RETURN( gpStateManager->ApplyBeforeRendering( pEffect ) );
#endif

	V( dxC_DebugValidateDevice() );
	sVerifyRTsAreClear();

	ONCE
	{
		PERF( DRAW_DRAWPRIM );

#if defined(ENGINE_TARGET_DX9)
		V_CONTINUE( dxC_GetDevice()->DrawIndexedPrimitive(
			PrimitiveType,
			BaseVertexIndex,
			MinVertexIndex,
			NumVertices,
			startIndex,
			primCount ) );

#elif defined(ENGINE_TARGET_DX10)
		dxC_GetDevice()->DrawIndexed( dx10_TopologyVertexCount( PrimitiveType, primCount ) , startIndex, BaseVertexIndex );
#endif
	}

	V( dxC_TargetSetCurrentDirty() );

report:
	e_ReportBatch( eGroup );
	e_ReportPolygons( primCount, eGroup );

	return S_OK;
}

PRESULT dxC_DrawIndexedPrimitiveInstanced(
	D3DC_PRIMITIVE_TOPOLOGY PrimitiveType,
	INT BaseVertexIndex, 
	UINT NumVertices,
	UINT startIndex,
	UINT primCount,
	UINT nInstances,
	UINT nStreams,
	LPD3DCEFFECT pEffect,
	METRICS_GROUP eGroup,
	D3DC_TECHNIQUE_HANDLE hTech,
	UINT iPassIndex )
{
	V_RETURN( dxC_UpdateDebugRenderStates() );

	if ( dxC_IsPixomaticActive() )
	{
		return E_NOTIMPL;
	}

	// If we are delaying the update of a vertex buffer, we have to use the prefetch index buffer as well.
	if ( dxC_DPGetPrefetch() )
	{
		V( dxC_SetPrefetchIndices() );
		NumVertices = 3;
		primCount = 1;
	}

	if ( pEffect )
	{
		PERF( DRAW_FX_COMMIT );
		DX9_BLOCK( V_RETURN( pEffect->CommitChanges() ); )
		DX10_BLOCK( V_RETURN( hTech->GetPassByIndex( iPassIndex )->Apply( 0 ) ); )
	}

	sVerifyRTsAreClear();

	ONCE
	{
		PERF( DRAW_DRAWPRIM );

#ifdef ENGINE_TARGET_DX9
		V_CONTINUE( dxC_GetDevice()->SetStreamSourceFreq( 0, D3DSTREAMSOURCE_INDEXEDDATA | nInstances ) );
		for(UINT i = 1; i < nStreams; ++i )
		{
			V_CONTINUE( dxC_GetDevice()->SetStreamSourceFreq( i, D3DSTREAMSOURCE_INSTANCEDATA | 1 ) );
		}
		V_CONTINUE( dxC_GetDevice()->DrawIndexedPrimitive(
			PrimitiveType,
			BaseVertexIndex,
			0,
			NumVertices,
			startIndex,
			primCount ) );
		//Reset streamsourcefreq back to "regular" indexed drawing
		for ( int i = nStreams; i > 0; i-- )
		{
			V_CONTINUE( dxC_SetStreamSource( i, NULL, 0, 0 ) );
			V_CONTINUE( dxC_GetDevice()->SetStreamSourceFreq( i, 1 ) );
		}
		V_CONTINUE( dxC_GetDevice()->SetStreamSourceFreq( 0, D3DSTREAMSOURCE_INDEXEDDATA | 1 ) );
#endif
#ifdef ENGINE_TARGET_DX10
		dxC_GetDevice()->IASetPrimitiveTopology( PrimitiveType );
		//V_RETURN( gpStateManager->ApplyBeforeRendering( pEffect != NULL, pEffect->hCurrentPass ) ); 
		dxC_GetDevice()->DrawIndexedInstanced( dx10_TopologyVertexCount( PrimitiveType, NumVertices ), nInstances, startIndex, BaseVertexIndex, 0 );
#endif
	}


	V( dxC_TargetSetCurrentDirty() );

	e_ReportBatch( eGroup );
	e_ReportPolygons( primCount, eGroup );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// Please don't use this. Ok I won't.  Thanks!  What about me?
//void dxC_DrawPrimitiveUP(
//	D3DC_PRIMITIVE_TOPOLOGY PrimitiveType,
//	UINT PrimitiveCount,
//	CONST void* pVertexStreamZeroData,
//	UINT VertexStreamZeroStride,
//	LPD3DCEFFECT pEffect,
//	METRICS_GROUP eGroup )
//{
//	dxC_UpdateDebugRenderStates();
//
//	if ( pEffect )
//	{
//		PERF( DRAW_COMMIT );
//		HRVERIFY( pEffect->CommitChanges() );
//	}
//
//	dxC_DebugValidateDevice();
//
//	{
//		PERF( DRAW_DRAWPRIM );
//		HRVERIFY( dx9_GetDevice()->DrawPrimitiveUP(
//			PrimitiveType,
//			PrimitiveCount,
//			pVertexStreamZeroData,
//			VertexStreamZeroStride ) );
//	}
//
//	e_ReportBatch( eGroup );
//	e_ReportPolygons( PrimitiveCount, eGroup );
//}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//Don't use this it's too old school
//PRESULT dxC_DrawIndexedPrimitiveUP(
//	D3DC_PRIMITIVE_TOPOLOGY PrimitiveType,
//	UINT MinVertexIndex,
//	UINT NumVertices,
//	UINT PrimitiveCount,
//	CONST void* pIndexData,
//	DXC_FORMAT IndexDataFormat,
//	CONST void* pVertexStreamZeroData,
//	UINT VertexStreamZeroStride,
//	LPD3DCEFFECT pEffect,
//	METRICS_GROUP eGroup )
//{
//#ifdef ENGINE_TARGET_DX9
//	V_RETURN( dxC_UpdateDebugRenderStates() );
//
//	if ( pEffect )
//	{
//		PERF( DRAW_COMMIT );
//		V_RETURN( pEffect->CommitChanges() );
//	}
//
//	V( dxC_DebugValidateDevice() );
//
//	int nDPs = 1;
//	if ( e_GetRenderFlag( RENDER_FLAG_DBG_RENDER_ENABLED ) )
//		nDPs++;
//
//	for ( int i = 0; i < nDPs; i++ )
//	{
//		PERF( DRAW_DRAWPRIM );
//
//		V_RETURN( dxC_GetDevice()->DrawIndexedPrimitiveUP(
//			PrimitiveType,
//			MinVertexIndex,
//			NumVertices,
//			PrimitiveCount,
//			pIndexData,
//			IndexDataFormat,
//			pVertexStreamZeroData,
//			VertexStreamZeroStride ) );
//
//		if ( nDPs > 1 )
//		{
//			V( sBeginWireframe() );
//		}
//	}
//	if ( nDPs > 1 )
//	{
//		V( sEndWireframe() );
//	}
//
//	V( dxC_TargetSetCurrentDirty() );
//
//	e_ReportBatch( eGroup );
//	e_ReportPolygons( PrimitiveCount, eGroup );
//
//	return S_OK;
//#endif
//	DX10_BLOCK( ASSERTX_RETVAL( 0, E_NOTIMPL, "KMNV TODO: Get rid of/workaround lack of DIPUP" ); )
//}
