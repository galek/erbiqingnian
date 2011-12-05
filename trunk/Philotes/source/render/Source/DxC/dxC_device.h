//----------------------------------------------------------------------------
// dxC_device.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DXC_DEVICE__
#define __DXC_DEVICE__


#define MAX_SWAP_CHAINS		4
#define DEFAULT_SWAP_CHAIN	0


#include "dxC_target.h"


class OptionState;



inline PRESULT dxC_HandleD3DReturn( PRESULT pr )
{
	if ( SUCCEEDED(pr) )
		return pr;

	switch ( pr )
	{
#ifdef ENGINE_TARGET_DX9
	case D3DERR_DEVICELOST:
	case D3DERR_DEVICENOTRESET:
		{
			PRESULT dx9_ResetDevice( void );
			V( dx9_ResetDevice() );
			return S_PE_TRY_AGAIN;
		}
	case D3DERR_OUTOFVIDEOMEMORY:
#endif // DX9
	case E_OUTOFMEMORY:
		{
			PRESULT e_Cleanup( BOOL dump = FALSE, BOOL cleanparts = FALSE );
			V( e_Cleanup() );
			return S_PE_TRY_AGAIN;
		}
	}
	return pr;
}


#define NEEDS_DEVICE( pr, expr )	{ DX9_BLOCK(dx9_CheckDeviceLost()); while ( S_PE_TRY_AGAIN == ( pr = dxC_HandleD3DReturn( expr ) ) ) { trace("Sleeping...\n"); Sleep(1000); DX9_BLOCK(dx9_CheckDeviceLost()); } }


//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

inline LPD3DCDEVICE dxC_GetDevice()
{
	extern SPD3DCDEVICE gpDevice;
	return gpDevice;
}

inline int dxC_GetCurrentSwapChainIndex()
{
	extern int gnCurrentSwapChainIndex;
	return gnCurrentSwapChainIndex;
}

#ifdef ENGINE_TARGET_DX10
//Because of DX10's weak reference holding, all get functions call AddRef on the returned interface
//since we always hold a reference we need to release this additional ref so as not to blow out refcounts
inline void dx10_GetRTsDSWithoutAddingRefs(  UINT nCount, LPD3DCRENDERTARGETVIEW* ppArrayRTs, LPD3DCDEPTHSTENCILVIEW* ppDS )
{
	dxC_GetDevice()->OMGetRenderTargets( nCount, ppArrayRTs, ppDS );

	if( ppArrayRTs )
	{
		for( UINT i = 0; i < nCount; ++i )
			if( ppArrayRTs[i] )
				(ppArrayRTs[i])->Release();
	}
	if( ppDS && *ppDS )
		(*ppDS)->Release();
}
#endif

inline PRESULT dxC_GetRenderTargetDepthStencil0( LPD3DCRENDERTARGETVIEW* ppRT, LPD3DCDEPTHSTENCILVIEW* ppDS = NULL )
{
#ifdef ENGINE_TARGET_DX9
		V_RETURN( dxC_GetDevice()->GetRenderTarget( 0, ppRT ) );
		if( ppDS )
		{
			V_RETURN( dxC_GetDevice()->GetDepthStencilSurface( ppDS ) );
		}
#endif
#ifdef ENGINE_TARGET_DX10
		dx10_GetRTsDSWithoutAddingRefs( 1, ppRT, ppDS );
#endif

	return S_OK;
}

// CHB 2007.03.02 - We may be able to do away with this.
// KMNV no I don't think we can
// CHB 2007.03.08 - It was mostly a comment to myself, but the idea
// goes like this.  The presentation parameters are now built
// entirely from OptionState state variables.  The present
// parameters can be constructed at will from an OptionState state
// object.  If that's true, what's the point in keeping a copy of
// the presentation parameters around, when you can get the
// information from the original source -- the state object?
// However, other things were more pressing that moment, so I
// didn't take the time to convince myself that there wasn't some
// oddball use of the PPs I wasn't aware of that would blow the
// whole theory to bits.
// CML 2007.06.05 - I think the D3DPRESENT_PARAMETERS can be changed
//   by the call to devicecreate, so keep
inline const D3DPRESENT_PARAMETERS & dxC_GetD3DPP( int nIndex = DEFAULT_SWAP_CHAIN )
{
	extern D3DPRESENT_PARAMETERS gtD3DPPs[ MAX_SWAP_CHAINS ];
	ASSERT( IS_VALID_INDEX( nIndex, MAX_SWAP_CHAINS ) );
	return gtD3DPPs[ nIndex ];
}

inline PRESULT dxC_GetResourceType(LPD3DCBASERESOURCE pResource, D3DRESOURCETYPE* pType)
{
	ASSERT_RETINVALIDARG( pResource );
	ASSERT_RETINVALIDARG( pType );
#ifdef ENGINE_TARGET_DX9
	*pType = pResource->GetType();
#elif defined(ENGINE_TARGET_DX10)
	pResource->GetType( pType );
#endif
	return S_OK;
}

DXCADAPTER dxC_GetAdapter();
void dxC_SetAdapter(DXCADAPTER adapter);
PRESULT dxC_DeviceCreate( HWND hWnd );
PRESULT dxC_GetD3DDisplayMode( E_SCREEN_DISPLAY_MODE & DisplayMode );
PRESULT dxC_ClearBackBufferPrimaryZ(UINT ClearFlags, D3DCOLOR ClearColor = 0, float depth = 1.0f, UINT8 stencil = 0);
PRESULT dxC_ClearFPBackBufferPrimaryZ(UINT ClearFlags, float* ClearColor, float depth = 1.0f, UINT8 stencil = 0);
PRESULT dxC_DebugValidateDevice();
PRESULT dxC_CreateRTV( LPD3DCTEXTURE2D pTexture, LPD3DCRENDERTARGETVIEW* ppRTV, UINT nLevel = 0);
PRESULT dxC_CreateDSV( LPD3DCTEXTURE2D pTexture, LPD3DCDEPTHSTENCILVIEW* ppRTV );
void dxC_StateToPresentation(const OptionState * pState, D3DPRESENT_PARAMETERS & Pres);
DXC_FORMAT dxC_GetBackbufferRenderTargetFormat( const OptionState * pOverrideState = NULL );
DXC_FORMAT dxC_GetBackbufferAdapterFormat( const OptionState * pOverrideState = NULL );
PRESULT dxC_GetAdapterOrdinalFromWindowPos( UINT & nAdapterOrdinal );

void dxC_SetD3DPresentation(const D3DPRESENT_PARAMETERS & tPP, int nIndex = 0);		// CHB 2007.03.02 - We may be able to do away with this.
PRESULT dxC_BeginScene( int nSwapChainIndex = DEFAULT_SWAP_CHAIN );
PRESULT dxC_EndScene();
PRESULT dxC_CreateAdditionalSwapChain(
	__out int & nIndex,
	DWORD nWidth,
	DWORD nHeight,
	HWND hWnd );
PRESULT dxC_ReleaseSwapChains();
PRESULT dxC_GrabSwapChainAndBackbuffer( int nSwapChainIndex );

#endif //__DXC_DEVICE__
