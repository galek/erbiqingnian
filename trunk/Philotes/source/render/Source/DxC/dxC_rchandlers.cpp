//----------------------------------------------------------------------------
// dxC_rchandlers.cpp
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"

#include "dxC_render.h"
#include "dxC_target.h"
#include "dxC_state.h"
#include "dxC_meshlist.h"
#include "dxC_commandbuffer.h"
#include "e_particle.h"

#include "dxC_rchandlers.h"


namespace FSSE
{

#define GET_RCHDATA( eType, pCommand )									\
	const RCHDATA * pData;												\
	RCD_ARRAY_GET( eType, pCommand->m_nDataID, pData );					\
	V_RETURN( pData->IsValid( RenderCommand::eType ) );				\
	const RCD_TYPE(eType) * pRC = (const RCD_TYPE(eType)*) pData;


//----------------------------------------------------------------------------
// RCHDATA_* vectors

#define INCLUDE_RCD_DATA_VECTOR
#include "dxC_rendercommand_def.h"


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_RenderCommandInit()
{
#define INCLUDE_RCD_DATA_ARRAY_INIT
#include "dxC_rendercommand_def.h"
	return S_OK;
}

PRESULT dxC_RenderCommandDestroy()
{
#define INCLUDE_RCD_DATA_ARRAY_DESTROY
#include "dxC_rendercommand_def.h"
	return S_OK;
}

PRESULT dxC_RenderCommandClearLists()
{
#define INCLUDE_RCD_DATA_ARRAY_CLEAR
#include "dxC_rendercommand_def.h"
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------


PRESULT dxC_RCH_DrawMeshList( const RenderCommand * pCommand )
{
	GET_RCHDATA( DRAW_MESH_LIST, pCommand );
	ASSERT_RETINVALIDARG( pRC->nMeshListID >= 0 );

	PRESULT pr = S_OK;

	MeshList * pMeshList = NULL;
	V_RETURN( dxC_MeshListGet( pRC->nMeshListID, pMeshList ) );
	ASSERT_RETFAIL( pMeshList );

	//e_ResetClipAndFog();

	// for each mesh in the list, render it
	for ( MeshList::const_iterator i = pMeshList->begin();
		i != pMeshList->end();
		++i )
	{
		if ( TESTBIT_DWORD( i->pMeshDraw->dwFlagBits, FSSE::MeshDraw::FLAGBIT_DEPTH_ONLY ) )
		{
			V_SAVE_ERROR( pr, dxC_RenderMeshDepth( *(i->pMeshDraw) ) );
		}
		else if ( TESTBIT_DWORD( i->pMeshDraw->dwFlagBits, FSSE::MeshDraw::FLAGBIT_BEHIND ) )
		{
			V_SAVE_ERROR( pr, dxC_RenderMeshBehind( *(i->pMeshDraw) ) );
		}
		else
		{
			V_SAVE_ERROR( pr, dxC_RenderMesh( *(i->pMeshDraw) ) );
		}
	}

	return pr;
}


PRESULT dxC_RCH_SetTarget( const RenderCommand * pCommand )
{
	GET_RCHDATA( SET_TARGET, pCommand );

#if defined( ENGINE_TARGET_DX10 ) && defined( DX10_USE_MRTS ) && !defined( DX10_EMULATE_DX9_BEHAVIOR )
	if ( pRC->nRTCount > 1 )
	{
		PRESULT pr;
		V_SAVE( pr, dxC_SetMultipleRenderTargetsWithDepthStencil( pRC->nRTCount, (RENDER_TARGET_INDEX*)pRC->eColor, 
			pRC->eDepth, pRC->nLevel ) );
		//V( dxC_ResetViewport() );

		//ID3D10RenderTargetView* pRT[] = { NULL, NULL, NULL };
		//float color[4] = ARGB_DWORD_TO_RGBA_FLOAT4( 0x00000000 );
		//color[ 0 ] = e_GetFarClipPlane();
		//dx10_GetRTsDSWithoutAddingRefs(  pRC->nRTCount, pRT, NULL );
		//if ( pRT[ 0 ] )
		//	dxC_GetDevice()->ClearRenderTargetView( pRT[ 0 ], color );

		//if ( pRT[ 1 ] )
		//{
		//	static const float clearColor[ 4 ] = ARGB_DWORD_TO_RGBA_FLOAT4( 0x00000000 );
		//	dxC_GetDevice()->ClearRenderTargetView( pRT[ 1 ], clearColor );
		//}

		return pr;
	}
	else
#endif
	return dxC_SetRenderTargetWithDepthStencil( pRC->eColor[ 0 ], pRC->eDepth, pRC->nLevel );
}


PRESULT dxC_RCH_ClearTarget( const RenderCommand * pCommand )
{
	GET_RCHDATA( CLEAR_TARGET, pCommand );

#ifdef ENGINE_TARGET_DX10	
	UINT nRTs = pRC->nRTCount;

#ifdef DX10_EMULATE_DX9_BEHAVIOR
	nRTs = 1;
#endif

	ID3D10RenderTargetView* pRT[] = { NULL, NULL, NULL };
	ID3D10DepthStencilView* pDS = NULL;
	dx10_GetRTsDSWithoutAddingRefs(  nRTs, pRT, &pDS );

	for( UINT i = 0; i < nRTs; ++i )
	{
		if( pRT[ i ] )
		{
			dxC_RTMarkClearToken( dxC_GetCurrentRenderTarget( i ) );
			dxC_GetDevice()->ClearRenderTargetView( pRT[i], (float*)&(pRC->tColor[i]) );
		}
	}
		
	if( pDS )
		dxC_GetDevice()->ClearDepthStencilView( pDS, (D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL), pRC->fDepth, static_cast<UINT8>(pRC->nStencil) );
	
	return S_OK;
#else
	return dxC_ClearBackBufferPrimaryZ( pRC->dwFlags, pRC->tColor[0], pRC->fDepth, static_cast<UINT8>(pRC->nStencil) );
#endif
}


PRESULT dxC_RCH_CopyBuffer( const RenderCommand * pCommand )
{
	GET_RCHDATA( COPY_BUFFER, pCommand );

#ifdef ENGINE_TARGET_DX10
	SPD3DCTEXTURE2D pSrc = dxC_RTResourceGet( pRC->eSrc, TRUE );
	SPD3DCTEXTURE2D pDest = dxC_RTResourceGet( pRC->eDest, TRUE );

#if ISVERSION(DEVELOPMENT)
	char szName[32];
	if ( ! pSrc )
	{
		dxC_GetRenderTargetName( pRC->eSrc, szName, 32 );
		ASSERTV_RETFAIL( pSrc, "%s resource missing", szName );
	}
	if ( ! pDest )
	{
		dxC_GetRenderTargetName( pRC->eDest, szName, 32 );
		ASSERTV_RETFAIL( pDest, "%s resource missing", szName );
	}
#else
	ASSERT_RETFAIL( pSrc );
	ASSERT_RETFAIL( pDest );
#endif

	dxC_GetDevice()->CopyResource( pDest, pSrc );

	return S_OK;
#else
	// CML TODO
	return E_NOTIMPL;
#endif
}


PRESULT dxC_RCH_SetViewport( const RenderCommand * pCommand )
{
	GET_RCHDATA( SET_VIEWPORT, pCommand );
	return dxC_SetViewport( pRC->tVP );
}


PRESULT dxC_RCH_SetClip( const RenderCommand * pCommand )
{
	GET_RCHDATA( SET_CLIP, pCommand );
	V_RETURN( dxC_ResetClipPlanes( FALSE ) );
	V_RETURN( dxC_PushClipPlane( &pRC->tPlane ) );
	return dxC_CommitClipPlanes();
}


PRESULT dxC_RCH_SetScissor( const RenderCommand * pCommand )
{
	GET_RCHDATA( SET_SCISSOR, pCommand );
	return dxC_SetScissorRect( (RECT*)&pRC->tRect, 0 );
}


PRESULT dxC_RCH_SetFogEnabled( const RenderCommand * pCommand )
{
	GET_RCHDATA( SET_FOG_ENABLED, pCommand );
	return e_SetFogEnabled( pRC->bEnabled, pRC->bForce );
}


PRESULT dxC_RCH_Callback( const RenderCommand * pCommand )
{
	GET_RCHDATA( CALLBACK_FUNC, pCommand );
	ASSERT_RETINVALIDARG( pRC->pfnCallback );

	return pRC->pfnCallback( pRC->pUserData );
}


PRESULT dxC_RCH_DrawParticles( const RenderCommand * pCommand )
{
	GET_RCHDATA( DRAW_PARTICLES, pCommand );
	return e_ParticlesDraw( pRC->eDrawLayer, &pRC->mView, &pRC->mProj, &pRC->mProj2, 
				pRC->vEyePos, pRC->vEyeDir, TRUE, pRC->bDepthOnly );
}

PRESULT dxC_RCH_SetColorWrite( const RenderCommand * pCommand )
{
	GET_RCHDATA( SET_COLORWRITE, pCommand );
	
	DXC_9_10( _D3DRENDERSTATETYPE, D3DRS_BLEND ) tState = D3DRS_COLORWRITEENABLE;

#ifndef DX10_EMULATE_DX9_BEHAVIOR
	switch( pRC->nRT )
	{
	case 3:
		tState = D3DRS_COLORWRITEENABLE3;
		break;

	case 2:
		tState = D3DRS_COLORWRITEENABLE2;
		break;

	case 1:
		tState = D3DRS_COLORWRITEENABLE1;
		break;
	}
#endif

	return dxC_SetRenderState( tState, pRC->dwValue );
}

PRESULT dxC_RCH_SetDepthWrite( const RenderCommand * pCommand )
{
	GET_RCHDATA( SET_DEPTHWRITE, pCommand );
	return dxC_SetRenderState( D3DRS_ZWRITEENABLE, pRC->bDepthWriteEnable );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------


PRESULT dxC_RCH_DebugText( const RenderCommand * pCommand )
{
	return S_OK;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// RenderCommand handler table

RenderCommand::PFN_RENDERCOMMAND_HANDLER g_RenderCommandHandlers[ RenderCommand::_NUM_TYPES ] = 
{
	NULL,						// sentinel to protect against uninitialized commands
#	define INCLUDE_RCD_FUNC_ARRAY
#	include "dxC_rendercommand_def.h"
};


void RCHDATA_DRAW_MESH_LIST::GetStats( WCHAR * pwszStats, int nBufLen ) const
{
	MeshList * pMeshList = NULL;
	V_DO_FAILED( dxC_MeshListGet( nMeshListID, pMeshList ) )
	{
		return;
	}
	ASSERT_RETURN( pMeshList );
	PStrPrintf( pwszStats, nBufLen, L"Meshes: %d   bDepthOnly: %d",
		pMeshList->size(),
		bDepthOnly );
}


void RCHDATA_DEBUG_TEXT::GetStats( WCHAR * pwszStats, int nBufLen ) const
{
	PStrPrintf( pwszStats, nBufLen, L"%S",
		szText );
}

}; // namespace FSSE
