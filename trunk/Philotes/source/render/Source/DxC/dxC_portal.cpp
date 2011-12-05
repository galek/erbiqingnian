//----------------------------------------------------------------------------
// dx9_portal.cpp
//
// - Implementation for portal functions/structures
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"
#include "dxC_target.h"
#include "dxC_texture.h"
#include "dxC_model.h"
#include "dxC_state.h"
#include "dxC_main.h"

#include "dxC_portal.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

//RENDER_TARGET gnPortalRenderTarget = RENDER_TARGET_INVALID;		// preferred render target for rendering to portal texture

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// IMPLEMENTATION
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dx9_PortalsRestoreUnmanaged()
{
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dx9_PortalsReleaseUnmanaged()
{
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_PortalsRestoreAll()
{
	for ( PORTAL * pPortal = e_PortalGetFirst();
		pPortal;
		pPortal = e_PortalGetNext( pPortal ) )
	{
		e_PortalRestore( pPortal );
	}
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_PortalsReleaseAll()
{
	for ( PORTAL * pPortal = e_PortalGetFirst();
		pPortal;
		pPortal = e_PortalGetNext( pPortal ) )
	{
		e_PortalRelease( pPortal );
	}
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_PortalRestore( PORTAL * pPortal )
{
	//ASSERT_RETURN( pPortal );

	//int nWidth, nHeight;
	//e_GetWindowSize( nWidth, nHeight );
	//gnPortalRenderTarget = RENDER_TARGET_FULL;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_PortalRelease( PORTAL * pPortal )
{
	ASSERT_RETURN( pPortal );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_PortalStoreRender( int nPortalID )
{
/*
	// copy off rendered world into render target for later use
	PORTAL * pPortal = e_PortalGet( nPortalID );
	ASSERT_RETINVALIDARG( pPortal );

	// matching the "push" in e_SetupPortal()
	V( dxC_PopClipPlane() );

	// debug
	//dxC_SetRenderTargetWithDepthStencil( gnPortalRenderTarget, ZBUFFER_NONE );
	//dxC_ClearBackBufferPrimaryZ( D3DCLEAR_TARGET );//dx9_ClearBuffers( TRUE, FALSE, 0x00000000 );
	//dxC_SetRenderTargetWithDepthStencil( RENDER_TARGET_BACKBUFFER, ZBUFFER_AUTO );
	// /debug


	SPD3DCTEXTURE2D pDestTexture = dxC_RTResourceGet( gnPortalRenderTarget );
	ASSERT_RETFAIL( pDestTexture );

	// too much flooring error here, should be rounding or similar
	int nWidth, nHeight;
	e_GetWindowSize( nWidth, nHeight );
	RECT tRect;
	SetRect(
		&tRect,
		(int)( ( pPortal->tScreenBBox.vMin.x * 0.5f + 0.5f ) * nWidth ),
		(int)( ( pPortal->tScreenBBox.vMin.y * 0.5f + 0.5f ) * nHeight ),
		(int)( ( pPortal->tScreenBBox.vMax.x * 0.5f + 0.5f ) * nWidth ),
		(int)( ( pPortal->tScreenBBox.vMax.y * 0.5f + 0.5f ) * nHeight ) );

	LONG nTop = tRect.top, nBottom = tRect.bottom;
	tRect.top	 = nHeight - nBottom;
	tRect.bottom = nHeight - nTop;

	tRect.left   = PIN<long>( tRect.left,   0, nWidth );
	tRect.top    = PIN<long>( tRect.top,    0, nHeight );
	tRect.right  = PIN<long>( tRect.right,  0, nWidth );
	tRect.bottom = PIN<long>( tRect.bottom, 0, nHeight );

	if ( tRect.bottom > tRect.top && tRect.right > tRect.left )
	{
  		V_RETURN( dxC_CopyBackbufferToTexture( pDestTexture, &tRect, &tRect ) );
		//dxC_DebugTextureGetShaderResource() = pDestTexture;
	}

	V( dxC_ResetViewport() );

	// CHB 2006.10.31 - On nVidia, setting the viewport appears to
	// have the side-effect of resetting the scissor rectangle to
	// the viewport. This behavior is not observed on ATI. To enjoy
	// consistent behavior between the two, we reset the scissor
	// rectangle here.
	//
	// This fixes the bug where rendering broke for the back sides
	// of hellrifts on ATI cards, which was cause by the hellrift's
	// scissor rectangle not being clear at this point for ATI, but
	// implicitly reset on nVidia.
	V( dxC_SetScissorRect(0) );

	V( dxC_ResetClipPlanes() );
*/
	return S_OK;
}
