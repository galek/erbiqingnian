//----------------------------------------------------------------------------
// dx9_target.cpp
//
// Implementation for render and depth target functions/structures
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"

#include "dxC_texture.h"
#include "dxC_state.h"
#include "dxC_effect.h"
#include "dxC_caps.h"
#include "dxC_shadow.h"
#include "dxC_drawlist.h"
#include "dxC_main.h"
#include "performance.h"
#include "dxC_profile.h"
#include "dxC_target.h"
#include "dxC_hdrange.h"
#include "e_optionstate.h"	// KMNV 2007.03.13
#include "dxC_2d.h"
#include "dxC_settings.h"

#ifdef ENGINE_TARGET_DX10
#include "dx10_fluid.h"
#endif

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

// uncomment to enable timing of video-memory to system-memory transfers
//#define VID_TO_SYS_TIMERS

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

enum
{
	RTB_MAP = -3,
	RTB_CANVAS = -2,
	RTB_BACK = -1,
};


enum
{
	RTSF_1X = 0,
	RTSF_DOWN_2X,
	RTSF_DOWN_4X,
	RTSF_DOWN_8X,
	RTSF_DOWN_16X,
};

const EFFECT_TARGET RTT_ANY = FXTGT_INVALID;

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

struct RENDER_TARGET_SPEC
{
	int nReservedEnum;
	BOOL bPerSwapChain;
	DXC_FORMAT format;
	BOOL bDepth;
	int eWidth;
	int eHeight;
	int eScaleFactor;
	BOOL bReserved;
	BOOL bAlternateBackBuffer;
	BOOL bSystemCopy;
	int nPriority;
	BOOL bHellgate;
	BOOL bTugboat;
	BOOL bDx9;
	BOOL bMSAA;
	EFFECT_TARGET eMinTarget;
};

struct D3D_RENDER_TARGET_DESC
{
	int						nIndex;
	DXC_FORMAT				eFormat;
	DWORD					dwWidth;
	DWORD					dwHeight;
	D3DC_USAGE				tBinding;
	//RENDER_TARGET_TYPE	eType;
	int						nPitch;
	int						nPriority;
	UINT					nMSAASamples;
	UINT					nMSAAQuality;
	BOOL					bMipChain;

	BOOL					bBackbuffer;
	int						nSwapChainIndex;
	int						nReservedEnum;

#ifdef ENGINE_TARGET_DX9 //Don't need sys copy in DX10
	BOOL					bSysCopy;
#endif
	//BOOL					bReserved;
	//SPD3DCTEXTURE2D	    pResource;		// CHB 2007.01.18 - Not used.
};

struct D3D_RENDER_TARGET
{
	int							nId;
	D3D_RENDER_TARGET*			pNext;

	BOOL						bBackbuffer;		// this object is a stub for a backbuffer
	int							nSwapChainIndex;	// is this a swap-chain-specific render target?
	int							nReservedEnum;		// the SWAP or GLOBAL enum

	SPD3DCTEXTURE2D				pResource;
	SPD3DCRENDERTARGETVIEW		pRTV;

	BOOL						bDirty;		// this target has been written to
	DWORD						dwClearToken; // a visibility token for when the RT was last cleared

#if defined( ENGINE_TARGET_DX9 )
	SPD3DCTEXTURE2D				pSystemResource;

#elif defined( ENGINE_TARGET_DX10 )
	SPD3DCTEXTURE2D				pResolveResource; //For MSAA resolves
	DXC_FORMAT					fmtResolve;
	SPD3DCSHADERRESOURCEVIEW	pSRV;
	BOOL						bResolveDirty;
#endif

#if ISVERSION(DEVELOPMENT)
	int							nDebugSets;	// number of times this RT was set since target creation
	int							nDebugIdle;	// number of frames since this RT was last set
#endif
};

struct D3D_DS_TARGET
{
	int							nId;
	D3D_DS_TARGET*				pNext;

	int							nSwapChainIndex;	// is this a swap-chain-specific depth target?
	int							nReservedEnum;		// the SWAP or GLOBAL enum

	SPD3DCTEXTURE2D				pResource;  //Slightly wasteful in dx9 because we don't always need a resource
	SPD3DCDEPTHSTENCILVIEW		pDSV;

	BOOL						bDirty;		// this target has been written to

#ifdef ENGINE_TARGET_DX10 
	SPD3DCSHADERRESOURCEVIEW	pSRV;
#endif

#if ISVERSION(DEVELOPMENT)
	int							nDebugSets;	// number of times this DT was set since target creation
	int							nDebugIdle;	// number of frames since this DT was last set
#endif
};


CHash<D3D_RENDER_TARGET>		gtRenderTargets;
CHash<D3D_DS_TARGET>			gtDSTargets;
//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

RENDER_TARGET_INDEX	gpCurrentRenderTargets[MAX_MRTS_ALLOWED] = {RENDER_TARGET_INVALID};
DEPTH_TARGET_INDEX	gnCurrentDepthTarget					 = DEPTH_TARGET_INVALID;
int		gnCurrentRenderTargetCount  = 0;		// MRTs currently set
bool	gBStaticRTsCreated			= false;

DEPTH_TARGET_INDEX		gnGlobalDepthTargetIndex [NUM_GLOBAL_DEPTH_TARGETS];
RENDER_TARGET_INDEX		gnGlobalRenderTargetIndex[NUM_GLOBAL_RENDER_TARGETS];

// Full count of all depth targets, incl global and all swap chains
int		giDepthStencilCount										= 0;

// Full count of all render targets, incl global and all swap chains
int		giRenderTargetCount										= 0;

int gnDebugRenderTargetTexture = INVALID_ID;
CArrayIndexed<D3D_RENDER_TARGET_DESC> gtRenderTargetDesc; // NUM_RENDER_TARGETS isn't a complete count, but a good start
SPD3DCSHADERRESOURCEVIEW gpDebugTextureView	= NULL;

RENDER_TARGET_SPEC gtGlobalRenderTargetSpecs[] =
{
#define DEFINE_ONLY_GLOBAL_TARGETS
// { code name, depth target, width basis, height basis, scale factor, reserved, system mem copy, priority }
#define DEFINE_TARGET(name, format, depth,width,height,scale,rsvd,altbb,syscopy,priority,bHg,bTug,bDx9,bMSAA,mintarget) { GLOBAL_RT_##name, FALSE, format, depth, width, height, scale, rsvd, altbb, syscopy, priority, bHg, bTug, bDx9, bMSAA, mintarget },
#include "dxC_target_def.h"
};


RENDER_TARGET_SPEC gtBackbufferLinkedRenderTargetSpecs[] =
{
#define DEFINE_ONLY_BACKBUFFER_LINKED_TARGETS
	// { code name, depth target, width basis, height basis, scale factor, reserved, system mem copy, priority }
#define DEFINE_TARGET(name, format, depth,width,height,scale,rsvd,altbb,syscopy,priority,bHg,bTug,bDx9,bMSAA,mintarget) { SWAP_RT_##name, TRUE, format, depth, width, height, scale, rsvd, altbb, syscopy, priority, bHg, bTug, bDx9, bMSAA, mintarget },
#include "dxC_target_def.h"
};



//----------------------------------------------------------------------------
// IMPLEMENTATIONS
//----------------------------------------------------------------------------

void sRenderTargetHashAdd( int & nIndex, BOOL bBackbuffer, int nSwapChainIndex, int nReservedEnum )
{
	if ( nIndex < 0 )
	{
		nIndex = giRenderTargetCount;
		++giRenderTargetCount;
	}
	HashAdd( gtRenderTargets, nIndex );

	D3D_RENDER_TARGET * pRT = HashGet( gtRenderTargets, nIndex );
	ASSERT_RETURN(pRT);
	pRT->bBackbuffer     = bBackbuffer;
	pRT->nSwapChainIndex = nSwapChainIndex;
	pRT->nReservedEnum   = nReservedEnum;
}

void sDepthTargetHashAdd( int & nIndex, int nSwapChainIndex, int nReservedEnum )
{
	if ( nIndex < 0 )
	{
		nIndex = giDepthStencilCount;
		++giDepthStencilCount;
	}
	HashAdd( gtDSTargets, nIndex );

	D3D_DS_TARGET * pDS = HashGet( gtDSTargets, nIndex );
	ASSERT_RETURN(pDS);
	pDS->nSwapChainIndex = nSwapChainIndex;
	pDS->nReservedEnum   = nReservedEnum;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
SPD3DCTEXTURE2D & dxC_RTResourceGet( RENDER_TARGET_INDEX nTarget, BOOL bUseSubResourceIfAvailable /*= FALSE*/ )
{

	static SPD3DCTEXTURE2D tNull;
#if 0 && ISVERSION(DEVELOPMENT) && defined( ENGINE_TARGET_DX9 )
	ASSERT_RETVAL( nTarget != RENDER_TARGET_BACKBUFFER, tNull );
#endif

	ASSERT_RETVAL( IS_VALID_INDEX(nTarget, giRenderTargetCount), tNull );

	D3D_RENDER_TARGET* pRT = HashGet( gtRenderTargets, nTarget );
	ASSERT_RETVAL( pRT, tNull );

#if defined(ENGINE_TARGET_DX10)
	if ( bUseSubResourceIfAvailable && pRT->pResolveResource )
	{
		if( pRT->pResource && pRT->bResolveDirty )
		{
			dxC_GetDevice()->ResolveSubresource( pRT->pResolveResource, 0, pRT->pResource, 0, pRT->fmtResolve );
			pRT->bResolveDirty = false;
		}

		return pRT->pResolveResource;
	}
#endif

	return pRT->pResource;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
SPD3DCRENDERTARGETVIEW & dxC_RTSurfaceGet( RENDER_TARGET_INDEX nTarget )
{
	static SPD3DCRENDERTARGETVIEW tNull;

	ASSERT_RETVAL( IS_VALID_INDEX(nTarget, giRenderTargetCount), tNull );

	D3D_RENDER_TARGET* pRT = HashGet( gtRenderTargets, nTarget );
	ASSERT_RETVAL( pRT, tNull );

	return pRT->pRTV;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
SPD3DCSHADERRESOURCEVIEW & dxC_RTShaderResourceGet( RENDER_TARGET_INDEX nTarget )
{
	static SPD3DCSHADERRESOURCEVIEW tNull;

	ASSERT_RETVAL( IS_VALID_INDEX(nTarget, giRenderTargetCount), tNull );

#if defined(ENGINE_TARGET_DX9)
	return dxC_RTResourceGet( nTarget );

#elif defined(ENGINE_TARGET_DX10)
	D3D_RENDER_TARGET* pRT = HashGet( gtRenderTargets, nTarget );
	ASSERT_RETVAL( pRT, tNull );

	if( pRT->pResolveResource && pRT->pResource && pRT->bResolveDirty )
	{
		dxC_GetDevice()->ResolveSubresource( pRT->pResolveResource, 0, pRT->pResource, 0, pRT->fmtResolve );
		pRT->bResolveDirty = false;
	}
	return pRT->pSRV;
#endif
}

SPD3DCTEXTURE2D & dxC_RTSystemTextureGet( RENDER_TARGET_INDEX nTarget )
{
	static SPD3DCTEXTURE2D tNull;

	ASSERT_RETVAL( IS_VALID_INDEX(nTarget, giRenderTargetCount), tNull );

#ifdef ENGINE_TARGET_DX9
	D3D_RENDER_TARGET* pRT = HashGet( gtRenderTargets, nTarget );
	ASSERT_RETVAL( pRT, tNull );

	return pRT->pSystemResource;

#elif defined( ENGINE_TARGET_DX10 )
	ASSERTX(0, "DX10 Doesn't have system textures!");
	return tNull;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SPD3DCDEPTHSTENCILVIEW & dxC_DSSurfaceGet( DEPTH_TARGET_INDEX nTarget )
{
	static SPD3DCDEPTHSTENCILVIEW tNull;

	ASSERT_RETVAL( IS_VALID_INDEX(nTarget, giDepthStencilCount), tNull );

	D3D_DS_TARGET* pDSTarget = HashGet( gtDSTargets, nTarget );
	ASSERT_RETVAL( pDSTarget, tNull );

	return pDSTarget->pDSV;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SPD3DCTEXTURE2D & dxC_DSResourceGet( DEPTH_TARGET_INDEX nTarget )
{
	static SPD3DCTEXTURE2D tNull;

	ASSERT_RETVAL( IS_VALID_INDEX(nTarget, giDepthStencilCount), tNull );

	D3D_DS_TARGET* pDSTarget = HashGet( gtDSTargets, nTarget );
	ASSERT_RETVAL( pDSTarget, tNull );

	return pDSTarget->pResource;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
SPD3DCSHADERRESOURCEVIEW & dxC_DSShaderResourceView( DEPTH_TARGET_INDEX nTarget )
{
	static SPD3DCSHADERRESOURCEVIEW tNull;

	ASSERT_RETVAL( IS_VALID_INDEX(nTarget, giDepthStencilCount), tNull );

#ifdef ENGINE_TARGET_DX9
	return dxC_DSResourceGet( nTarget );

#elif defined(ENGINE_TARGET_DX10)
	D3D_DS_TARGET* pDSTarget = HashGet( gtDSTargets, nTarget );
	ASSERT_RETVAL( pDSTarget, tNull );

	return pDSTarget->pSRV;
#endif
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_RTMarkClearToken( RENDER_TARGET_INDEX nRTID )
{
#ifdef ENGINE_TARGET_DX10
	if( nRTID == RENDER_TARGET_NULL )
		return S_OK;
#endif

	ASSERT_RETINVALIDARG( IS_VALID_INDEX(nRTID, giRenderTargetCount) );

	D3D_RENDER_TARGET* pRT = HashGet( gtRenderTargets, nRTID );
	ASSERT_RETINVALIDARG( pRT );
	pRT->dwClearToken = e_GetVisibilityToken();

	return S_OK;
}

PRESULT dxC_RTGetClearToken( RENDER_TARGET_INDEX nRTID, DWORD& dwClearToken )
{
#ifdef ENGINE_TARGET_DX10
	if( nRTID == RENDER_TARGET_NULL )
	{
		dwClearToken = e_GetVisibilityToken();
		return S_OK;
	}
#endif

	ASSERT_RETINVALIDARG( IS_VALID_INDEX(nRTID, giRenderTargetCount) );

	D3D_RENDER_TARGET* pRT = HashGet( gtRenderTargets, nRTID );
	ASSERT_RETINVALIDARG( pRT );
	dwClearToken = pRT->dwClearToken;

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_RTSetDirty( RENDER_TARGET_INDEX nRTID, BOOL bDirty /*= TRUE*/ )
{
#ifdef ENGINE_TARGET_DX10
	if( nRTID == RENDER_TARGET_NULL )
		return S_OK;
#endif

	ASSERT_RETINVALIDARG( IS_VALID_INDEX(nRTID, giRenderTargetCount) );

	D3D_RENDER_TARGET* pRT = HashGet( gtRenderTargets, nRTID );
	ASSERT_RETINVALIDARG( pRT );
	pRT->bDirty = bDirty;

#ifdef ENGINE_TARGET_DX10
	pRT->bResolveDirty = bDirty;
#endif
	return S_OK;
}

PRESULT dxC_RTGetDirty( RENDER_TARGET_INDEX nRTID, BOOL &bDirty )
{
#ifdef ENGINE_TARGET_DX10
	if( nRTID == RENDER_TARGET_NULL )
	{
		bDirty = false;
		return S_OK;
	}
#endif

	ASSERT_RETINVALIDARG( IS_VALID_INDEX(nRTID, giRenderTargetCount) );

	D3D_RENDER_TARGET* pRT = HashGet( gtRenderTargets, nRTID );
	ASSERT_RETINVALIDARG( pRT );
	bDirty = pRT->bDirty;
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_DSSetDirty( DEPTH_TARGET_INDEX nDSID, BOOL bDirty /*= TRUE*/ )
{
	ASSERT_RETINVALIDARG( IS_VALID_INDEX(nDSID, giDepthStencilCount) );

	D3D_DS_TARGET* pDSTarget = HashGet( gtDSTargets, nDSID );
	ASSERT_RETINVALIDARG( pDSTarget );
	pDSTarget->bDirty = bDirty;
	return S_OK;
}


PRESULT dxC_DSGetDirty( DEPTH_TARGET_INDEX nDSID, BOOL &bDirty )
{
	ASSERT_RETINVALIDARG( IS_VALID_INDEX(nDSID, giDepthStencilCount) );

	D3D_DS_TARGET* pDSTarget = HashGet( gtDSTargets, nDSID );
	ASSERT_RETINVALIDARG( pDSTarget );
	bDirty = pDSTarget->bDirty;
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dxC_TargetSetCurrentDirty( BOOL bDirty /*= TRUE*/ )
{
	// This function could check states such as colorwriteenable and zwriteenable that would prevent any
	// actual dirtying of the target(s).

	for( UINT i = 0; i < MAX_MRTS_ALLOWED; ++i )
	{
		RENDER_TARGET_INDEX nRT = dxC_GetCurrentRenderTarget( i );
		if ( nRT == RENDER_TARGET_INVALID )
			break;

		V( dxC_RTSetDirty( nRT, bDirty ) );
	}

	DEPTH_TARGET_INDEX nDS = dxC_GetCurrentDepthTarget();
	if ( nDS != DEPTH_TARGET_NONE )
	{
		V( dxC_DSSetDirty( nDS, bDirty ) );
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
DEPTH_TARGET_INDEX dxC_GetCurrentDepthTarget()
{
	return gnCurrentDepthTarget;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
DXC_FORMAT dxC_GetCurrentDepthTargetFormat()
{
	DEPTH_TARGET_INDEX nDepthTarget = dxC_GetCurrentDepthTarget();

	D3DC_DEPTH_STENCIL_VIEW_DESC tDesc;
	ASSERT_RETVAL( IS_VALID_INDEX( nDepthTarget, giDepthStencilCount ), D3DFMT_UNKNOWN );

	D3D_DS_TARGET* pDSTarget = HashGet( gtDSTargets, nDepthTarget );

	ASSERT_RETVAL( pDSTarget && pDSTarget->pDSV, D3DFMT_UNKNOWN );
	pDSTarget->pDSV->GetDesc( &tDesc );
	return tDesc.Format;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sRTDTMarkSet( RENDER_TARGET_INDEX eRT, DEPTH_TARGET_INDEX eDT )
{
	REF( eRT );
	REF( eDT );

	if ( dxC_IsPixomaticActive() )
		return;

#if ISVERSION(DEVELOPMENT)

	ASSERT_RETURN(    IS_VALID_INDEX(eRT, giRenderTargetCount) 
		           || DXC_9_10( 0, eRT == RENDER_TARGET_NULL ) );		// DX10 supports rendering with a NULL render target (used for shadows)
	ASSERT_RETURN( IS_VALID_INDEX(eDT, giDepthStencilCount) || eDT == DEPTH_TARGET_NONE );

	D3D_RENDER_TARGET* pRT = HashGet( gtRenderTargets, eRT );
	if ( pRT )
	{
		pRT->nDebugSets++;
		pRT->nDebugIdle = 0;
	}

	D3D_DS_TARGET* pDS = HashGet( gtDSTargets, eDT );
	if ( pDS )
	{
		pDS->nDebugSets++;
		pDS->nDebugIdle = 0;
	}
#endif // DEVELOPMENT
}

#ifdef ENGINE_TARGET_DX9
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dx9_SetDepthTarget( DEPTH_TARGET_INDEX nDepthTarget )
{
	ASSERT_RETINVALIDARG( IS_VALID_INDEX(nDepthTarget, giDepthStencilCount) || nDepthTarget == DEPTH_TARGET_NONE );

	if ( dxC_GetCurrentDepthTarget() == nDepthTarget )
		return S_FALSE;
	gnCurrentDepthTarget = nDepthTarget;

	if ( dxC_IsPixomaticActive() )
	{
		return dxC_PixoSetDepthTarget( nDepthTarget );
	}

	if ( nDepthTarget == DEPTH_TARGET_NONE )
	{
		V_RETURN( dxC_GetDevice()->SetDepthStencilSurface( NULL ) );
		return S_OK;
	}

	V_RETURN( dxC_GetDevice()->SetDepthStencilSurface( dxC_DSSurfaceGet( nDepthTarget ) ) );
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dx9_SetRenderTarget( RENDER_TARGET_INDEX nRenderTarget )
{
	ASSERT_RETINVALIDARG( IS_VALID_INDEX(nRenderTarget, giRenderTargetCount) || nRenderTarget == RENDER_TARGET_NULL );

	if ( dxC_GetCurrentRenderTarget() == nRenderTarget )
		return S_FALSE;
	gpCurrentRenderTargets[0] = nRenderTarget;


	if ( dxC_IsPixomaticActive() )
	{
		return dxC_PixoSetRenderTarget( nRenderTarget );
	}


	D3D_RENDER_TARGET* pRT = HashGet( gtRenderTargets, nRenderTarget );
	ASSERT_RETFAIL( pRT );

	// If this is the backbuffer of one of the swap chains, go into that swap chain and grab it
	if ( pRT->bBackbuffer )
	{
		LPD3DCSWAPCHAIN pSwapChain = dxC_GetD3DSwapChain( pRT->nSwapChainIndex );
		ASSERT_RETFAIL( pSwapChain );
		SPDIRECT3DSURFACE9 pBackBuffer;
		//V_RETURN( dxC_GetDevice()->GetBackBuffer( 0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer ) );
		V_RETURN( pSwapChain->GetBackBuffer( 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer ) );
		V_RETURN( dxC_GetDevice()->SetRenderTarget( 0, pBackBuffer ) );
		return S_OK;
	}

	//if ( nRenderTarget == RENDER_TARGET_BACKBUFFER )
	//{
	//	// This is a hack to get the HDR rendertarget
	//	// Should only get here in HDR mode
	//	LPD3DCRENDERTARGETVIEW pRTV = HDRScene::GetRenderTarget();
	//	ASSERT_RETFAIL( pRTV );
	//	V_RETURN( dxC_GetDevice()->SetRenderTarget( 0, pRTV ) );
	//	return S_OK;
	//}

	ASSERT_RETFAIL( pRT->pRTV );

	V_RETURN( dxC_GetDevice()->SetRenderTarget( 0, pRT->pRTV ) );
	return S_OK;
}

#endif // DX9


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_ClearSetRenderTargetCache()
{
	for ( int i = 0; i < MAX_MRTS_ALLOWED; ++i )
		gpCurrentRenderTargets[i] = RENDER_TARGET_INVALID;
	gnCurrentDepthTarget = DEPTH_TARGET_INVALID;
	return S_OK;
}

#ifdef ENGINE_TARGET_DX10
PRESULT dx10_SetRenderTargetWithDepthStencil( UINT nRenderTargetCount, RENDER_TARGET_INDEX* pnRenderTarget, DEPTH_TARGET_INDEX nDepthTarget, UINT nLevel )
{
	ASSERT( nRenderTargetCount > 0 && nRenderTargetCount <= MAX_MRTS_ALLOWED && pnRenderTarget );
	gnCurrentRenderTargetCount = nRenderTargetCount;
	gnCurrentDepthTarget = nDepthTarget;

	SPD3DCRENDERTARGETVIEW pD3D10RTV[ MAX_MRTS_ALLOWED ];
	for ( UINT nRT = 0; nRT < MAX_MRTS_ALLOWED; nRT++ )
	{
		pD3D10RTV[ nRT ] = NULL;

		if ( nRT < nRenderTargetCount )
		{
			gpCurrentRenderTargets[nRT] = pnRenderTarget[nRT];
			if( pnRenderTarget[ nRT ] != RENDER_TARGET_NULL )
			{
				D3D_RENDER_TARGET* pRenderTarget = HashGet( gtRenderTargets, pnRenderTarget[ nRT ] );
				ASSERT_RETFAIL( pRenderTarget );

				if( nLevel > 0 )
				{
					V_RETURN( dxC_CreateRTV( pRenderTarget->pResource, &pD3D10RTV[ nRT ].p, nLevel ) );
				}
				else
					pD3D10RTV[ nRT ] = pRenderTarget->pRTV;
			}
			else
				pD3D10RTV[ nRT ] = NULL;
		}
	}

	SPD3DCDEPTHSTENCILVIEW pDSV = NULL;
	if( nDepthTarget != DEPTH_TARGET_NONE )
	{
		D3D_DS_TARGET* pDSTarget = HashGet( gtDSTargets, nDepthTarget );
		ASSERT_RETFAIL( pDSTarget );

		pDSV = pDSTarget->pDSV;
	}

	dxC_GetDevice()->OMSetRenderTargets( nRenderTargetCount, (LPD3DCRENDERTARGETVIEW*)pD3D10RTV, pDSV );
	dxC_ResetFullViewport();  //Doing this here to emulate Dx9's behavior of setting a default viewport
	return S_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

RENDER_TARGET_INDEX dxC_GetSwapChainRenderTargetIndexEx( int nSwapChainIndex, SWAP_CHAIN_RENDER_TARGET eRT )
{
	ASSERT_RETVAL( IS_VALID_INDEX(nSwapChainIndex, MAX_SWAP_CHAINS), RENDER_TARGET_INVALID );
	ASSERT_RETVAL( IS_VALID_INDEX(eRT, NUM_SWAP_CHAIN_RENDER_TARGETS), RENDER_TARGET_INVALID );

	extern DXCSWAPCHAIN gtSwapChains[ MAX_SWAP_CHAINS ];
	return (RENDER_TARGET_INDEX)gtSwapChains[nSwapChainIndex].nRenderTargets[eRT];
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

RENDER_TARGET_INDEX dxC_GetSwapChainRenderTargetIndex( int nSwapChainIndex, SWAP_CHAIN_RENDER_TARGET eRT )
{
	HDR_MODE eHDR = e_GetValidatedHDRMode();
	if ( eHDR != HDR_MODE_LINEAR_FRAMEBUFFER && eRT == SWAP_RT_BACKBUFFER )
	{
		if ( e_GetRenderFlag( RENDER_FLAG_RENDER_TO_FULLRT ) & MAKE_MASK(nSwapChainIndex) )
		{
			// If rendering to the full RT instead of the backbuffer, map BACKBUFFER to FULL_MSAA instead of FINAL_BACKBUFFER
			eRT = SWAP_RT_FULL_MSAA;
		}
		else
		{
			// In LDR mode, BACKBUFFER and FINAL_BACKBUFFER are the same rendertarget
			eRT = SWAP_RT_FINAL_BACKBUFFER;
		}
	}

	return dxC_GetSwapChainRenderTargetIndexEx( nSwapChainIndex, eRT );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_SetRenderTargetWithDepthStencil( GLOBAL_RENDER_TARGET eGlobalRT, DEPTH_TARGET_INDEX eDT, UINT nLevel /*= 0*/ )
{
	RENDER_TARGET_INDEX eRT = dxC_GetGlobalRenderTargetIndex( eGlobalRT );

	return dxC_SetRenderTargetWithDepthStencil( eRT, eDT, nLevel );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_SetRenderTargetWithDepthStencil( SWAP_CHAIN_RENDER_TARGET eSwapRT, DEPTH_TARGET_INDEX eDT, UINT nLevel /*= 0*/, int nSwapChainIndex /*= -1*/ )
{
	if ( nSwapChainIndex == -1 )
		nSwapChainIndex = dxC_GetCurrentSwapChainIndex();

	RENDER_TARGET_INDEX eRT = dxC_GetSwapChainRenderTargetIndex( nSwapChainIndex, eSwapRT );

	return dxC_SetRenderTargetWithDepthStencil( eRT, eDT, nLevel );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_SetRenderTargetWithDepthStencil( RENDER_TARGET_INDEX nRenderTarget, DEPTH_TARGET_INDEX nDepthTarget, UINT nLevel )
{
	if(    gnCurrentRenderTargetCount == 1
		&& dxC_GetCurrentRenderTarget() == nRenderTarget 
		&& dxC_GetCurrentDepthTarget() == nDepthTarget 
		&& nLevel == 0 ) //Checking if level is 0 here because we need to create a new RTV on the fly if it isn't
	{
		return S_FALSE;
	}

	gnCurrentRenderTargetCount = 1;

	sRTDTMarkSet( nRenderTarget, nDepthTarget );

#ifdef ENGINE_TARGET_DX9
	ASSERT_RETFAIL( nRenderTarget != RENDER_TARGET_NULL );  //Can't set a NULL RT in DX9
	ASSERTX_RETVAL( nLevel == 0, E_NOTIMPL, "Don't support rendering to mipchain in DX9 yet." );
	V_RETURN( dx9_SetRenderTarget( nRenderTarget ) );
	V_RETURN( dx9_SetDepthTarget( nDepthTarget ) ); 
#endif
	DX10_BLOCK( V_RETURN( dx10_SetRenderTargetWithDepthStencil( 1, &nRenderTarget, nDepthTarget, nLevel ) ); )
	return S_OK;
}

PRESULT dxC_SetMultipleRenderTargetsWithDepthStencil( UINT nRenderTargetCount, RENDER_TARGET_INDEX* pnRenderTarget, DEPTH_TARGET_INDEX nDepthTarget, UINT nLevel )
{
#ifdef ENGINE_TARGET_DX9
	ASSERTX_RETVAL( 0, E_NOTIMPL, "Don't support MRTs on DX9 yet." );
#else
	ASSERT( nRenderTargetCount > 0 && nRenderTargetCount <= MAX_MRTS_ALLOWED && pnRenderTarget );

	if(	   gnCurrentRenderTargetCount == nRenderTargetCount
		&& dxC_GetCurrentRenderTarget() == pnRenderTarget[ 0 ] 
		&& dxC_GetCurrentDepthTarget() == nDepthTarget 
		&& nLevel == 0 ) //Checking if level is 0 here because we need to create a new RTV on the fly if it isn't
	{
		return S_FALSE;
	}

	gnCurrentRenderTargetCount = nRenderTargetCount;

	//HDR_MODE eHDR = e_GetValidatedHDRMode();
	//if ( eHDR != HDR_MODE_LINEAR_FRAMEBUFFER && pnRenderTarget[ 0 ] == RENDER_TARGET_BACKBUFFER )
	//{
	//	if ( e_GetRenderFlag( RENDER_FLAG_RENDER_TO_FULLRT ) )
	//	{
	//		// If rendering to the full RT instead of the backbuffer, map BACKBUFFER to FULL_RT instead of FINAL_BACKBUFFER
	//		pnRenderTarget[ 0 ] = dxC_GetSwapChainFullRenderTarget( dxC_GetCurrentSwapChainIndex() );
	//	}
	//	else
	//	{
	//		// In LDR mode, BACKBUFFER and FINAL_BACKBUFFER are the same rendertarget
	//		pnRenderTarget[ 0 ] = RENDER_TARGET_FINAL_BACKBUFFER;
	//	}
	//}

	for ( UINT nRT = 0; nRT < nRenderTargetCount; nRT++ )
		sRTDTMarkSet( pnRenderTarget[ nRT ], nDepthTarget );

	V_RETURN( dx10_SetRenderTargetWithDepthStencil( nRenderTargetCount, pnRenderTarget, nDepthTarget, nLevel ) );
#endif

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
UINT dxC_GetCurrentRenderTargetCount()
{
	return gnCurrentRenderTargetCount;
}

RENDER_TARGET_INDEX dxC_GetCurrentRenderTarget( int nMRTIndex )
{
	if( nMRTIndex >= gnCurrentRenderTargetCount || nMRTIndex >= MAX_MRTS_ALLOWED )
		return RENDER_TARGET_INVALID;

	return gpCurrentRenderTargets[nMRTIndex];
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

#ifdef ENGINE_TARGET_DX10
PRESULT dx10_RenderTargetAddSRV( RENDER_TARGET_INDEX nRTID )
{
	D3D_RENDER_TARGET* pRT = HashGet( gtRenderTargets, nRTID );
	ASSERT_RETFAIL( pRT );
	ASSERT_RETFAIL( pRT->pResource );
	//ASSERT_RETFAIL( ! pRT->bBackbuffer );

	pRT->pResolveResource = NULL;
	
	D3D10_TEXTURE2D_DESC texDesc;
	pRT->pResource->GetDesc( &texDesc );
	bool bMSAA = texDesc.SampleDesc.Count > 1;

	if( bMSAA )
	{
		pRT->fmtResolve = dx10_ResourceFormatToRTVFormat( texDesc.Format );
		dxC_Create2DTexture( texDesc.Width, texDesc.Height, 1, D3DC_USAGE_2DTEX_RT, texDesc.Format, &pRT->pResolveResource.p, NULL, NULL, 1 );
	}

	V_RETURN( CreateSRVFromTex2D( bMSAA ? pRT->pResolveResource : pRT->pResource, &pRT->pSRV.p, 0 ) );

	return S_OK;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#ifdef ENGINE_TARGET_DX9
static PRESULT s_dx9_CreateRenderTarget(
	DWORD dwWidth,
	DWORD dwHeight,
	DXC_FORMAT eFormat,
	DWORD dwMSAAType,
	DWORD dwMSAAQuality,
	BOOL bLockable,
	LPDIRECT3DSURFACE9 * ppSurface )
{
	ASSERT_RETINVALIDARG( ppSurface );
	ASSERT_RETINVALIDARG( dwWidth > 0 );
	ASSERT_RETINVALIDARG( dwHeight > 0 );

	return dxC_GetDevice()->CreateRenderTarget(
		dwWidth,
		dwHeight,
		eFormat,
		(D3DMULTISAMPLE_TYPE)dwMSAAType,
		dwMSAAQuality,
		bLockable,
		ppSurface,
		NULL );
}
#endif // ENGINE_TARGET_DX9

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static PRESULT sCreateRT( D3D_RENDER_TARGET_DESC * pRTDesc )
{
#ifdef DX10_EMULATE_DX9_BEHAVIOR
	//switch( pRTDesc->nIndex )
	//{
	//case RENDER_TARGET_DEPTH:
	//case RENDER_TARGET_DEPTH_WO_PARTICLES:
	//case RENDER_TARGET_MOTION:
	//case RENDER_TARGET_FLUID_BOXDEPTH:
	//	return S_OK;
	//}
#endif

	ASSERT_RETINVALIDARG( pRTDesc );
	if ( pRTDesc->tBinding == D3DC_USAGE_2DTEX_RT )
	{
		ASSERT_RETFAIL( IS_VALID_INDEX(pRTDesc->nIndex, giRenderTargetCount) );
		//ASSERT_RETFAIL( pRTDesc->nIndex != RENDER_TARGET_FINAL_BACKBUFFER );


		ASSERT_RETFAIL( pRTDesc->tBinding != 0 );
		
		if ( pRTDesc->bBackbuffer )
			return S_FALSE;

		D3D_RENDER_TARGET* pRT = HashGet( gtRenderTargets, pRTDesc->nIndex );
		ASSERT_RETFAIL( pRT );

#ifdef ENGINE_TARGET_DX9
		if ( pRTDesc->nMSAASamples > 1 )
		{
			// create a rendertarget surface directly -- otherwise, can't create MSAA
			V_RETURN( s_dx9_CreateRenderTarget(
				pRTDesc->dwWidth,
				pRTDesc->dwHeight,
				pRTDesc->eFormat,
				pRTDesc->nMSAASamples,
				pRTDesc->nMSAAQuality,
				FALSE,
				&pRT->pRTV ) );

			// NO TEXTURE for MSAA render targets!
			pRT->pResource = NULL;
		}
		else
#endif // DX9
		{
			int nNumMIPs = e_GetNumMIPLevels( MAX( pRTDesc->dwWidth, pRTDesc->dwHeight ) );
			V_RETURN( dxC_Create2DTexture( pRTDesc->dwWidth, pRTDesc->dwHeight, pRTDesc->bMipChain ? nNumMIPs : 1, pRTDesc->tBinding, pRTDesc->eFormat, &pRT->pResource.p, NULL, NULL, pRTDesc->nMSAASamples, pRTDesc->nMSAAQuality ) );
			V_RETURN( dxC_CreateRTV( pRT->pResource, &pRT->pRTV.p ) );
		}

#ifdef ENGINE_TARGET_DX10
		dx10_RenderTargetAddSRV( (RENDER_TARGET_INDEX)pRTDesc->nIndex );
#endif

		if ( ( pRTDesc->nMSAASamples <= 1 && !pRT->pResource ) || !pRT->pRTV )
		{
			char szFormat[ 32 ];
			dxC_TextureFormatGetDisplayStr( pRTDesc->eFormat, szFormat, 32 );
			ASSERTV_MSG( "Failed to create render target:\nIndex: %d\nRes: %dx%d\nFormat: %s",
				pRTDesc->nIndex,
				pRTDesc->dwWidth,
				pRTDesc->dwHeight,
				szFormat );
		}

		gD3DStatus.dwVidMemTotal += pRTDesc->dwWidth * pRTDesc->dwHeight * dx9_GetTextureFormatBitDepth(pRTDesc->eFormat) / 8;
		gD3DStatus.dwVidMemRenderTargets += pRTDesc->dwWidth * pRTDesc->dwHeight * dx9_GetTextureFormatBitDepth(pRTDesc->eFormat) / 8;

		// system memory backing for updating non-default texture surfaces
#ifdef ENGINE_TARGET_DX9 //Don't need sys copies in DX10
		if ( pRTDesc->bSysCopy )
		{
			V_RETURN( D3DXCreateTexture( dxC_GetDevice(), pRTDesc->dwWidth, pRTDesc->dwHeight, 1,
				D3DX_DEFAULT, pRTDesc->eFormat, D3DPOOL_SYSTEMMEM, &pRT->pSystemResource ) );
		}
#endif
		dxC_RTSetDirty( (RENDER_TARGET_INDEX)pRTDesc->nIndex ); //Should always be dirty to start
	}
	else if( pRTDesc->tBinding == D3DC_USAGE_2DTEX_DS )
	{
		ASSERT_RETFAIL( IS_VALID_INDEX(pRTDesc->nIndex, giDepthStencilCount) );

		if ( pRTDesc->bBackbuffer )
			return S_FALSE;

		if (pRTDesc->dwWidth && pRTDesc->dwHeight)
		{
			D3D_DS_TARGET* pDSTarget = HashGet( gtDSTargets, pRTDesc->nIndex );
			ASSERT_RETFAIL( pDSTarget );

#ifdef ENGINE_TARGET_DX9
			// Depth/stencil resources may already have been created by shadow code (or auto depth buffers on DX9)
			if ( ! pDSTarget->pDSV )
			{
				V_RETURN( dxC_GetDevice()->CreateDepthStencilSurface( pRTDesc->dwWidth, pRTDesc->dwHeight,
					pRTDesc->eFormat, D3DMULTISAMPLE_NONE, 0, TRUE, &pDSTarget->pDSV, NULL ) );
			}
#elif defined(ENGINE_TARGET_DX10)
			if ( ! pDSTarget->pResource.p )
			{
				V_RETURN( dxC_Create2DTexture( pRTDesc->dwWidth, pRTDesc->dwHeight, 1, pRTDesc->tBinding, pRTDesc->eFormat, &pDSTarget->pResource.p, NULL, NULL, pRTDesc->nMSAASamples, pRTDesc->nMSAAQuality ) );
			}
			if ( ! pDSTarget->pDSV.p )
			{
				V_RETURN( dxC_CreateDSV( pDSTarget->pResource, &pDSTarget->pDSV.p ) );
			}
#endif
			dxC_DSSetDirty( (DEPTH_TARGET_INDEX)pRTDesc->nIndex ); //Should always be dirty to start
		}
	}
	else if ( pRTDesc->tBinding == D3DC_USAGE_2DTEX_SM )
	{
		if (pRTDesc->dwWidth && pRTDesc->dwHeight)
		{
			D3D_DS_TARGET* pDSTarget = HashGet( gtDSTargets, pRTDesc->nIndex );
			ASSERT_RETFAIL( pDSTarget );

			V_RETURN( dxC_Create2DTexture( pRTDesc->dwWidth, pRTDesc->dwHeight, 1, pRTDesc->tBinding, pRTDesc->eFormat, &pDSTarget->pResource.p, NULL, NULL, pRTDesc->nMSAASamples, pRTDesc->nMSAAQuality ) );
			V_RETURN( dxC_CreateDSV( pDSTarget->pResource, &pDSTarget->pDSV.p ) );
			DX10_BLOCK( V_RETURN( CreateSRVFromTex2D( pDSTarget->pResource, &pDSTarget->pSRV.p ) ); )
			dxC_DSSetDirty( (DEPTH_TARGET_INDEX)pRTDesc->nIndex ); //Should always be dirty to start
		}
	}
	else 
	{
		ASSERT_RETINVALIDARG( 0 && "Invalid render target type encountered!" );
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void dxC_RTReleaseAll( RENDER_TARGET_INDEX nTarget )
{
	D3D_RENDER_TARGET* pRT = HashGet( gtRenderTargets, nTarget );
	ASSERT_RETURN( pRT );

	pRT->pResource = NULL;
	DX10_BLOCK( pRT->pResolveResource = NULL; )
	pRT->pRTV = NULL;
	DX10_BLOCK( pRT->pSRV = NULL; )
	DX10_BLOCK( pRT->fmtResolve = D3DFMT_UNKNOWN; )

	dxC_RTSetDirty( nTarget, true );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_TargetSetAllDirty( BOOL bDirty /*= TRUE*/ )
{
	for ( int nRT = 0; nRT < giRenderTargetCount; nRT++ )
	{
		V( dxC_RTSetDirty( (RENDER_TARGET_INDEX)nRT, bDirty ) );
	}

	for ( int nDS = 0; nDS < giDepthStencilCount; nDS++ )
	{
		V( dxC_DSSetDirty( (DEPTH_TARGET_INDEX)nDS, bDirty ) );
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dxC_RestoreRenderTargets ()
{
	// iterate rendertargetdesc list and restore the render targets
	for ( int nRTDesc = gtRenderTargetDesc.GetFirst();
		nRTDesc != INVALID_ID;
		nRTDesc = gtRenderTargetDesc.GetNextId( nRTDesc ) )
	{
		D3D_RENDER_TARGET_DESC * pRTDesc = gtRenderTargetDesc.Get( nRTDesc );
		if ( !pRTDesc )
			continue;

		//pRTDesc->nIndex = nRTDesc;
		V_RETURN( sCreateRT( pRTDesc ) );
	}

	gBStaticRTsCreated = true;

	// CHB 2007.09.10 - On DX10, the "auto" zbuffer is not auto.
	// So, create it here. This is deliberately placed below the
	// setting of gBStaticRTsCreated to true, otherwise
	// dxC_AddRenderTargetDesc() will bypass creating the buffer.
#ifdef ENGINE_TARGET_DX10
	dx10_CreateDepthBuffer( DEFAULT_SWAP_CHAIN );
#endif

	V( dxC_TargetSetAllDirty() );

	return S_OK;
}

void dxC_GetRenderTargetName( int nRT, char * pszStr, int nBufLen )
{
	ASSERT_RETURN( pszStr );
	PStrPrintf( pszStr, nBufLen, "<unknown>" );

	D3D_RENDER_TARGET * pRT = HashGet( gtRenderTargets, nRT );
	if ( ! pRT )
		return;

	if ( pRT->nSwapChainIndex != INVALID_ID )
	{
		// Swap-chain RT
		SWAP_CHAIN_RENDER_TARGET eRT = (SWAP_CHAIN_RENDER_TARGET)pRT->nReservedEnum;
		switch (eRT)
		{
#define RT(x) case x: PStrPrintf( pszStr, nBufLen, "%s %d", #x, pRT->nSwapChainIndex ); return;

			RT(SWAP_RT_BACKBUFFER)
			RT(SWAP_RT_FINAL_BACKBUFFER)

#define DEFINE_ONLY_BACKBUFFER_LINKED_TARGETS
#define DEFINE_TARGET(name, format, depth,width,height,scale,rsvd,altbb,syscopy,priority,bHg,bTug,bDx9,bMSAA,mintarget) case SWAP_RT_##name: PStrPrintf( pszStr, nBufLen, "%s%s %d", "SWAP_RT_", #name, pRT->nSwapChainIndex ); break;
#include "dxC_target_def.h"

#undef RT
			default:				PStrCopy( pszStr, "<unknown swap rt>", nBufLen );
		}

	}
	else
	{
		// Global RT
		GLOBAL_RENDER_TARGET eRT = (GLOBAL_RENDER_TARGET)pRT->nReservedEnum;
		switch (eRT)
		{
#define RT(x) case x: PStrCopy( pszStr, #x, nBufLen ); return;

#define DEFINE_ONLY_GLOBAL_TARGETS
#define DEFINE_TARGET(name, format, depth,width,height,scale,rsvd,altbb,syscopy,priority,bHg,bTug,bDx9,bMSAA,mintarget) case GLOBAL_RT_##name: PStrPrintf( pszStr, nBufLen, "%s%s", "GLOBAL_RT_", #name ); break;
#include "dxC_target_def.h"

			RT(GLOBAL_RT_SHADOW_COLOR)
			RT(GLOBAL_RT_SHADOW_COLOR_MID)
			RT(GLOBAL_RT_SHADOW_COLOR_LOW)
			RT(GLOBAL_RT_LIGHTING_MAP)
			RT(GLOBAL_RT_FLUID_TEMP1)
			RT(GLOBAL_RT_FLUID_TEMP2)
			RT(GLOBAL_RT_FLUID_TEMPVELOCITY)

#undef RT
			default:				PStrCopy( pszStr, "<unknown global rt>", nBufLen );
		}
	}
}


void dxC_GetDepthTargetName( int nDT, char * pszStr, int nBufLen )
{
	ASSERT_RETURN( pszStr );
	PStrPrintf( pszStr, nBufLen, "<unknown>" );

	D3D_DS_TARGET * pDT = HashGet( gtDSTargets, nDT );
	if ( ! pDT )
		return;

	if ( pDT->nSwapChainIndex != INVALID_ID )
	{
		// Swap-chain DT
		SWAP_CHAIN_DEPTH_TARGET eDT = (SWAP_CHAIN_DEPTH_TARGET)pDT->nReservedEnum;
		switch (eDT)
		{
		case SWAP_DT_AUTO:		PStrPrintf( pszStr, nBufLen, "SWAP_DT_%d_%s", pDT->nSwapChainIndex, "AUTO" ); break;
		default:				PStrCopy( pszStr, "<unknown swap dt>", nBufLen );
		}
	}
	else
	{
		// Global DT
		if ( pDT->nReservedEnum >= _GLOBAL_DT_FIRST_SHADOWMAP && pDT->nReservedEnum <= _GLOBAL_DT_LAST_SHADOWMAP )
		{
			// SHADOWMAP
			SHADOWMAP eSM = (SHADOWMAP)( pDT->nReservedEnum - _GLOBAL_DT_FIRST_SHADOWMAP );
			ASSERT( IS_VALID_INDEX( eSM, NUM_SHADOWMAPS ) );
			switch (eSM)
			{
			case SHADOWMAP_HIGH:	PStrPrintf( pszStr, nBufLen, "GLOBAL_DT_SHADOWMAP_HIGH" ); break;
			case SHADOWMAP_MID:		PStrPrintf( pszStr, nBufLen, "GLOBAL_DT_SHADOWMAP_MID" ); break;
			case SHADOWMAP_LOW:		PStrPrintf( pszStr, nBufLen, "GLOBAL_DT_SHADOWMAP_LOW" ); break;
			default:				PStrCopy( pszStr, "<unknown global shadow dt>", nBufLen );
			}
		}
		else
		{
			PStrCopy( pszStr, "<unknown global dt>", nBufLen );
		}
	}
}


// CHB 2006.01.18
void dxC_DumpRenderTargets(void)
{
	trace("- BEGIN RENDER TARGET DUMP -\n");

	const int cnNameLen = 64;
	char szName[cnNameLen];

	for (int nRTDesc = gtRenderTargetDesc.GetFirst(); nRTDesc != INVALID_ID; nRTDesc = gtRenderTargetDesc.GetNextId(nRTDesc))
	{
		trace("\nRT desc id #%d\n", nRTDesc);
		D3D_RENDER_TARGET_DESC * pRTDesc = gtRenderTargetDesc.Get(nRTDesc);
		if (pRTDesc == 0)
		{
			continue;
		}

		trace("  object @ 0x%p\n", pRTDesc);

		switch (pRTDesc->tBinding)
		{
			case D3DC_USAGE_2DTEX_RT:
				dxC_GetRenderTargetName(pRTDesc->nIndex, szName, cnNameLen);
				break;
			case D3DC_USAGE_2DTEX_DS:
				PStrPrintf( szName, cnNameLen, "<Depth>" );
				break;
			case D3DC_USAGE_2DTEX_SM:
				PStrPrintf( szName, cnNameLen, "<ShadowMap>" );
				break;
		}
		trace("  Name: %s (%d)\n", szName, pRTDesc->nIndex);

		trace("  Dimensions: %u x %u\n", pRTDesc->dwWidth, pRTDesc->dwHeight);

		const char * pBinding = "<unknown>";
		switch (pRTDesc->tBinding)
		{
			case D3DC_USAGE_2DTEX_RT: pBinding = "RT"; break;
			case D3DC_USAGE_2DTEX_DS: pBinding = "DS"; break;
			case D3DC_USAGE_2DTEX_SM: pBinding = "SM"; break;
			default: break;
		}
		trace("  Binding: %s\n", pBinding);

		char szFmt[32];
		dxC_TextureFormatGetDisplayStr( pRTDesc->eFormat, szFmt, arrsize(szFmt) );
		trace("  Format: %s\n", szFmt );

		trace("  Priority: %d\n", pRTDesc->nPriority);
	}
	
	trace("\n- END RENDER TARGET DUMP -\n");
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sPrintTargetInfo(
	WCHAR * pwszLine,
	int nLineLen,
	WCHAR * pwszStr,
	int nStrLen,
	const char * pszType,
	int nID,
	const char * pszName,
	const char * pszResourceType,
	DWORD dwWidth,
	DWORD dwHeight,
	const char * pszFmt,
	int nDebugSets,
	int nDebugIdle
	)
{
	nDebugSets = MAX( nDebugSets, 0 );
	nDebugIdle = MAX( nDebugIdle, 0 );

	PStrPrintf( pwszLine, nLineLen, L"%-4S %4d %-30S %-10S %4d %4d %-18S %8d %8d\n",
		pszType,
		nID,
		pszName,
		pszResourceType,
		dwWidth,
		dwHeight,
		pszFmt,
		nDebugSets,
		nDebugIdle
		);
	PStrCat( pwszStr, pwszLine, nStrLen );
}


void dxC_DebugRenderTargetsGetList( WCHAR * pwszStr, int nBufLen )
{
	REF( pwszStr );
	REF( nBufLen );

#if ISVERSION(DEVELOPMENT)
	const int cnLineLen = 256;
	WCHAR wszLine[ cnLineLen ];

	PStrPrintf( pwszStr, nBufLen, L"Rendertargets and Depthtargets:\n" );
	PStrPrintf( wszLine, cnLineLen, L"%-8s %-4s %-30s %-10s %-4s %-4s %-18s %-8s %-8s\n",
		L"TYPE",
		L"ID",
		L"NAME",
		L"RESTYPE",
		L"X",
		L"Y",
		L"FMT",
		L"SETS",
		L"IDLE"
		);
	PStrCat( pwszStr, wszLine, nBufLen );

	const int cnNameLen = 64;
	char szName[cnNameLen];

	const char * pUnknown = "<unknown>";
	const char * pszResType = "<unknown>";
	char szFmt[32];
	DWORD dwWidth, dwHeight;

	for ( D3D_RENDER_TARGET * pTarget = HashGetFirst( gtRenderTargets );
		pTarget;
		pTarget = HashGetNext( gtRenderTargets, pTarget ) )
	{
		dxC_GetRenderTargetName(pTarget->nId, szName, cnNameLen);

		enum
		{
			_RT_TEXTURE = 0,
			_RT_RTV,
			_RT_SYSTEM,
			// count
			NUM_RT_RESOURCES
		};

		SPD3DCTEXTURE2D pRTTex;
		SPD3DCRENDERTARGETVIEW pRTV;
		SPD3DCTEXTURE2D pRTSys;

		for ( int i = 0; i < NUM_RT_RESOURCES; i++ )
		{
			switch ( i )
			{
			case _RT_TEXTURE:
				pRTTex = dxC_RTResourceGet( (RENDER_TARGET_INDEX)pTarget->nId );
				if ( pRTTex )
				{
					pszResType = "Texture";
					D3DC_TEXTURE2D_DESC tDesc;
					V( dxC_Get2DTextureDesc( pRTTex, 0, &tDesc ) );
					dxC_TextureFormatGetDisplayStr( tDesc.Format, szFmt, arrsize(szFmt) );
					dwWidth  = tDesc.Width;
					dwHeight = tDesc.Height;
					break;
				}
				goto _nextRTiteration;
			case _RT_RTV:
				if ( pRTTex )
					goto _nextRTiteration;
				pRTV = dxC_RTSurfaceGet( (RENDER_TARGET_INDEX)pTarget->nId );
				if ( pRTV )
				{
					pszResType = "Surface";
					D3DC_RENDER_TARGET_VIEW_DESC tDesc;
					V( dxC_GetRenderTargetViewDesc( pRTV, &tDesc ) );
					dxC_TextureFormatGetDisplayStr( tDesc.Format, szFmt, arrsize(szFmt) );
					V( dxC_GetRenderTargetDimensions( (int&)dwWidth, (int&)dwHeight, (RENDER_TARGET_INDEX)pTarget->nId ) );
					break;
				}
				goto _nextRTiteration;
			case _RT_SYSTEM:
#ifdef ENGINE_TARGET_DX9
				pRTSys = dxC_RTSystemTextureGet( (RENDER_TARGET_INDEX)pTarget->nId );
				if ( pRTSys )
				{
					pszResType = "Sys Tex";
					D3DC_TEXTURE2D_DESC tDesc;
					V( dxC_Get2DTextureDesc( pRTSys, 0, &tDesc ) );
					dxC_TextureFormatGetDisplayStr( tDesc.Format, szFmt, arrsize(szFmt) );
					dwWidth  = tDesc.Width;
					dwHeight = tDesc.Height;
					break;
				}
#endif
				goto _nextRTiteration;
			}

			sPrintTargetInfo(
				wszLine,
				cnLineLen,
				pwszStr,
				nBufLen,
				"RT",
				pTarget->nId,
				szName,
				pszResType,
				dwWidth,
				dwHeight,
				szFmt,
				pTarget->nDebugSets,
				pTarget->nDebugIdle );

_nextRTiteration:
			;
		}
	}


	for ( D3D_DS_TARGET * pTarget = HashGetFirst( gtDSTargets );
		pTarget;
		pTarget = HashGetNext( gtDSTargets, pTarget ) )
	{
		dxC_GetDepthTargetName( pTarget->nId, szName, cnNameLen );

		enum
		{
			_DS_TEXTURE = 0,
			_DS_RTV,
			// count
			NUM_DS_RESOURCES
		};

		SPD3DCTEXTURE2D pDSTex;
		SPD3DCDEPTHSTENCILVIEW pDSV;

		for ( int i = 0; i < NUM_DS_RESOURCES; i++ )
		{
			switch ( i )
			{
			case _DS_TEXTURE:
				pDSTex = dxC_DSResourceGet( (DEPTH_TARGET_INDEX)pTarget->nId );
				if ( pDSTex )
				{
					pszResType = "Texture";
					D3DC_TEXTURE2D_DESC tDesc;
					V( dxC_Get2DTextureDesc( pDSTex, 0, &tDesc ) );
					dxC_TextureFormatGetDisplayStr( tDesc.Format, szFmt, arrsize(szFmt) );
					dwWidth  = tDesc.Width;
					dwHeight = tDesc.Height;
					break;
				}
				goto _nextDSiteration;
			case _DS_RTV:
				if ( pDSTex )
					goto _nextDSiteration;
				pDSV = dxC_DSSurfaceGet( (DEPTH_TARGET_INDEX)pTarget->nId );
				if ( pDSV )
				{
					pszResType = "Surface";
					D3DC_DEPTH_STENCIL_VIEW_DESC tDesc;
					V( dxC_GetDepthStencilViewDesc( pDSV, &tDesc ) );
					dxC_TextureFormatGetDisplayStr( tDesc.Format, szFmt, arrsize(szFmt) );
					if ( pTarget->pResource )
					{
						D3DC_TEXTURE2D_DESC tDesc;
						V( dxC_Get2DTextureDesc( pTarget->pResource, 0, &tDesc ) );
						dwWidth  = tDesc.Width;
						dwHeight = tDesc.Height;
					} else
					{
						dwWidth  = 0;
						dwHeight = 0;
					}
					break;
				}
				goto _nextDSiteration;
			}

			sPrintTargetInfo(
				wszLine,
				cnLineLen,
				pwszStr,
				nBufLen,
				"DS",
				pTarget->nId,
				szName,
				pszResType,
				dwWidth,
				dwHeight,
				szFmt,
				pTarget->nDebugSets,
				pTarget->nDebugIdle );

_nextDSiteration:
			;
		}
	}

#endif // DEVELOPMENT
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void dxC_RenderTargetsMarkFrame()
{
#if ISVERSION(DEVELOPMENT)
	for ( D3D_RENDER_TARGET * pRT = HashGetFirst( gtRenderTargets );
		pRT;
		pRT = HashGetNext( gtRenderTargets, pRT ) )
	{
		pRT->nDebugSets = 0;
		pRT->nDebugIdle++;
	}

	for ( D3D_DS_TARGET * pDS = HashGetFirst( gtDSTargets );
		pDS;
		pDS = HashGetNext( gtDSTargets, pDS ) )
	{
		pDS->nDebugSets = 0;
		pDS->nDebugIdle++;
	}
#endif // DEVELOPMENT
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static int sRenderTargetDescLessThan( D3D_RENDER_TARGET_DESC * pLHS, void * pKey )
{
	const D3D_RENDER_TARGET_DESC * pRHS = static_cast<const D3D_RENDER_TARGET_DESC *>(pKey);

	// Sort by pitch, larger going earlier.
	if (pLHS->nPitch < pRHS->nPitch)
	{
		return 1;
	}
	if (pLHS->nPitch > pRHS->nPitch)
	{
		return -1;
	}

	// Smaller priority value means higher priority.
	if (pLHS->nPriority < pRHS->nPriority)
	{
		return -1;
	}
	if (pLHS->nPriority > pRHS->nPriority)
	{
		return 1;
	}

	// All other things being equal, sort by binding.
	// The binding values just happen to be in the order
	// we want, namely RT < DS < SM.
	if (pLHS->tBinding < pRHS->tBinding)
	{
		return -1;
	}
	if (pLHS->tBinding > pRHS->tBinding)
	{
		return 1;
	}

	// Equivalent.
	return 0;
}


int dxC_AddRenderTargetDesc(
	BOOL bBackBuffer,
	int nSwapChainIndex,
	int nReservedEnum,
	DWORD dwWidth,
	DWORD dwHeight,
	D3DC_USAGE tBinding,
	DXC_FORMAT eFormat,
	int nIndex /*= -1*/,
	BOOL bDX9SysCopy /*= FALSE*/,
	int nPriority /*= 1000*/,
	UINT nMSAASamples /*= 1*/,
	UINT nMSAAQuality /*= 0*/,
	BOOL bMipChain /*= FALSE*/ )
{
	ASSERT_DO( nIndex == -1 )
	{
		nIndex = -1;
	}

	D3D_RENDER_TARGET_DESC Key;
	Key.dwWidth = dwWidth;
	Key.dwHeight = dwHeight;
	Key.tBinding = tBinding;
	Key.eFormat = eFormat;
	Key.nIndex = nIndex;
	Key.nPriority = nPriority;
	Key.nPitch = dwWidth * dx9_GetTextureFormatBitDepth( eFormat ) / 8;
#ifdef ENGINE_TARGET_DX9
	Key.bSysCopy = bDX9SysCopy;
	//Key.nMSAASamples = 1;
	Key.nMSAASamples = nMSAASamples;
	Key.nMSAAQuality = nMSAAQuality;
#elif defined( ENGINE_TARGET_DX10 )
	Key.nMSAASamples = nMSAASamples;
	Key.nMSAAQuality = nMSAAQuality;
#endif
	Key.bMipChain = bMipChain;
	Key.bBackbuffer = bBackBuffer;
	Key.nSwapChainIndex = nSwapChainIndex;
	Key.nReservedEnum = nReservedEnum;

	int nRTDesc;
	D3D_RENDER_TARGET_DESC * pRTDesc = gtRenderTargetDesc.InsertSorted( &nRTDesc, &Key, sRenderTargetDescLessThan );
	*pRTDesc = Key;

	if( nIndex < 0 )
	{
		if( tBinding == D3DC_USAGE_2DTEX_RT )
		{
			//nIndex = giRenderTargetCount++;
			//HashAdd( gtRenderTargets, nIndex );
			sRenderTargetHashAdd( nIndex, bBackBuffer, nSwapChainIndex, nReservedEnum );
		}
		else
		{
			//nIndex = giDepthStencilCount++;
			//HashAdd( gtDSTargets, nIndex );
			sDepthTargetHashAdd( nIndex, nSwapChainIndex, nReservedEnum );
		}
	}

	pRTDesc->nIndex = nIndex;




	// Fill in the index for later lookup
	if ( tBinding == D3DC_USAGE_2DTEX_RT )
	{
		// Render target
		RENDER_TARGET_INDEX eRT = (RENDER_TARGET_INDEX)nIndex;
		if ( nSwapChainIndex != INVALID_ID )
		{
			DXCSWAPCHAIN * ptSC = dxC_GetDXCSwapChain( nSwapChainIndex );
			ptSC->nRenderTargets[ nReservedEnum ] = eRT;
		}
		else
		{
			gnGlobalRenderTargetIndex[ nReservedEnum ] = eRT;
		}
	}
	else
	{
		// Depth or shadow target
		DEPTH_TARGET_INDEX eDT = (DEPTH_TARGET_INDEX)nIndex;
		if ( nSwapChainIndex != INVALID_ID )
		{
			DXCSWAPCHAIN * ptSC = dxC_GetDXCSwapChain( nSwapChainIndex );
			ptSC->nDepthTargets[ nReservedEnum ] = eDT;
		}
		else
		{
			gnGlobalDepthTargetIndex[ nReservedEnum ] = eDT;
		}
	}



	if( gBStaticRTsCreated )
	{
		// must update the rendertarget desc with the real index
		//pRTDesc->nIndex = nIndex;
		V( sCreateRT( pRTDesc ) );
	}

	return nIndex;
}

void dxC_ReleaseRTsDependOnBB( int nSwapChainIndex )
{
	for ( int i = 0; i < NUM_SWAP_CHAIN_RENDER_TARGETS; i++ )
	{
		RENDER_TARGET_INDEX eRT = dxC_GetSwapChainRenderTargetIndexEx( nSwapChainIndex, (SWAP_CHAIN_RENDER_TARGET)i );
		if ( eRT == RENDER_TARGET_INVALID )
			continue;
		dxC_RTReleaseAll( eRT );
	}
}

static int sGetRTSpecHashIndex( int nSwapChainIndex, int nIndex )
{
	int nOffset = nSwapChainIndex;
}

static void sAddDescRTs( RENDER_TARGET_SPEC * ptRenderTargetSpecs, int nSpecs, int nSwapChainIndex = -1 )
{
	// these shouldn't be hard-coded numbers, they should be derived in a more comprehensive performance profile
	const float cfMinVideoMem = 100.0f;
	float fAvailMem = e_GetMaxPhysicalVideoMemoryMB() * 1.1f;


	int nWindowWidth  = 0;
	int nWindowHeight = 0;

	int nSizeFromSwapChain = nSwapChainIndex;
	if ( nSizeFromSwapChain <= -1 )
	{
		// The global MAP-sized rendertargets base their size off of the default backbuffer.
		nSizeFromSwapChain = DEFAULT_SWAP_CHAIN;
	}


	int nStatic;
	if ( nSwapChainIndex < 0 )
	{
		// Global
		nStatic = NUM_STATIC_GLOBAL_RENDER_TARGETS;
	}
	else
	{
		// Swap
		nStatic = NUM_STATIC_SWAP_CHAIN_RENDER_TARGETS;
	}

	const D3DPRESENT_PARAMETERS & tPP = dxC_GetD3DPP( nSizeFromSwapChain );
	nWindowWidth  = DXC_9_10( tPP.BackBufferWidth, tPP.BufferDesc.Width );
	nWindowHeight = DXC_9_10( tPP.BackBufferHeight, tPP.BufferDesc.Height );

	int nCanvasSize = 1024;

	int nMapSize = (nWindowHeight / 2);
	nMapSize = (int)NEXTPOWEROFTWO( (DWORD)nMapSize );


	for ( int i = 0; i < nSpecs; i++ )
	{
		if ( AppIsHellgate() && ! ptRenderTargetSpecs[ i ].bHellgate )
			continue;
		if ( AppIsTugboat() && ! ptRenderTargetSpecs[ i ].bTugboat )
			continue;

		// CHB 2007.08.31
		if ((ptRenderTargetSpecs[i].eMinTarget != FXTGT_INVALID) && (dxC_GetCurrentMaxEffectTarget() < ptRenderTargetSpecs[i].eMinTarget))
		{
			continue;
		}

		// Special cases for the swap chain ones
		if ( ptRenderTargetSpecs[i].bPerSwapChain )
		{
			// Special cases:
			switch ( ptRenderTargetSpecs[i].nReservedEnum )
			{
			case SWAP_RT_FLUID_BOXDEPTH:
				if ( ! e_OptionState_GetActive()->get_bFluidEffects() )
					continue;
				break;
			case SWAP_RT_DEPTH:
			case SWAP_RT_DEPTH_WO_PARTICLES:
				// Not disabling the depth target because that would not only disable DoF but soft particles, too.
				break;
			case SWAP_RT_MOTION:
				if ( ! e_OptionState_GetActive()->get_bDX10ScreenFX() )
					continue;
				break;
			}
		}

		ASSERTX_CONTINUE( nSwapChainIndex > -1 || ptRenderTargetSpecs[ i ].eWidth  != RTB_BACK, "Trying to use backbuffer size as basis for non-swap-chain render target!" );
		ASSERTX_CONTINUE( nSwapChainIndex > -1 || ptRenderTargetSpecs[ i ].eHeight != RTB_BACK, "Trying to use backbuffer size as basis for non-swap-chain render target!" );

		int nWidth, nHeight;
		switch ( ptRenderTargetSpecs[ i ].eWidth )
		{
		case RTB_BACK:		nWidth = nWindowWidth; break;
		case RTB_CANVAS:	nWidth = nCanvasSize; break;
		case RTB_MAP:		nWidth = nMapSize; break;
		default:			nWidth = 0;
		}
		switch ( ptRenderTargetSpecs[ i ].eHeight )
		{
		case RTB_BACK:		nHeight = nWindowHeight; break;
		case RTB_CANVAS:	nHeight = nCanvasSize; break;
		case RTB_MAP:		nHeight = nMapSize; break;
		default:			nHeight = 0;
		}
		ASSERT_CONTINUE( nWidth > 0 && nHeight > 0 );
		switch ( ptRenderTargetSpecs[ i ].eScaleFactor )
		{
		case RTSF_DOWN_2X:	nWidth = nWidth >> 1; nHeight = nHeight >> 1; break;
		case RTSF_DOWN_4X:	nWidth = nWidth >> 2; nHeight = nHeight >> 2; break;
		case RTSF_DOWN_8X:	nWidth = nWidth >> 3; nHeight = nHeight >> 3; break;
		case RTSF_DOWN_16X:	nWidth = nWidth >> 4; nHeight = nHeight >> 4; break;
		case RTSF_1X:
		default:		break;
		}

		BOOL bCreate = TRUE;

		// Allow creation of SWAP_RT_FULL_MSAA only if it's requested via renderflags
		if ( ptRenderTargetSpecs[ i ].bAlternateBackBuffer )
		{
			if ( 0 == ( e_GetRenderFlag( RENDER_FLAG_RENDER_TO_FULLRT ) & MAKE_MASK(nSwapChainIndex) ) )
				bCreate = FALSE;
		}

#ifdef ENGINE_TARGET_DX9
		if( ! ptRenderTargetSpecs[ i ].bDx9 )
			bCreate = FALSE;
#endif

		if ( bCreate )
		{
			int nSpecHashIndex = dxC_AddRenderTargetDesc(
				FALSE,
				nSwapChainIndex,
				ptRenderTargetSpecs[ i ].nReservedEnum,
				nWidth,
				nHeight,
				ptRenderTargetSpecs[ i ].bDepth ? D3DC_USAGE_2DTEX_DS : D3DC_USAGE_2DTEX_RT,
				ptRenderTargetSpecs[ i ].format,
				-1,
				ptRenderTargetSpecs[ i ].bSystemCopy,
				ptRenderTargetSpecs[ i ].nPriority,
				ptRenderTargetSpecs[ i ].bMSAA ? e_OptionState_GetActive()->get_nMultiSampleType() : 1,
				ptRenderTargetSpecs[ i ].bMSAA ? e_OptionState_GetActive()->get_nMultiSampleQuality() : 0 );


		}

	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_AddAutoDepthTargetDesc( SWAP_CHAIN_DEPTH_TARGET eSwapDT, int nSwapChainIndex )
{
#ifdef ENGINE_TARGET_DX9

	int nWidth, nHeight;
	DXC_FORMAT tFormat;
	DWORD dwMSType;
	DWORD dwMSQuality;

	if ( dxC_IsPixomaticActive() )
	{
		dxC_PixoGetBufferDimensions( nWidth, nHeight );
		tFormat = D3DFMT_D16;
		dwMSType = 0;
		dwMSQuality = 0;
	}
	else
	{
		ASSERT_RETINVALIDARG( dxC_GetD3DSwapChain( nSwapChainIndex ) );

		const D3DPRESENT_PARAMETERS & tPP = dxC_GetD3DPP( nSwapChainIndex );
		//ASSERT_RETFAIL( tPP.EnableAutoDepthStencil );
		DXC_FORMAT tDepthFmt = tPP.AutoDepthStencilFormat;
		if ( ! tPP.EnableAutoDepthStencil )
			tDepthFmt = dxC_GetDefaultDepthStencilFormat();

		nWidth		= tPP.BackBufferWidth;
		nHeight		= tPP.BackBufferHeight;
		tFormat		= tDepthFmt;
		dwMSType	= tPP.MultiSampleType;
		dwMSQuality = tPP.MultiSampleQuality;
	}

	DEPTH_TARGET_INDEX eDTIndex = (DEPTH_TARGET_INDEX)dxC_AddRenderTargetDesc(
		TRUE,
		nSwapChainIndex,
		eSwapDT,
		nWidth,
		nHeight,
		D3DC_USAGE_2DTEX_DS,
		tFormat,
		-1,
		FALSE,
		1,
		dwMSType,
		dwMSQuality );

	if ( ! dxC_IsPixomaticActive() )
	{
		DXCSWAPCHAIN * ptSC = dxC_GetDXCSwapChain( nSwapChainIndex );
		ptSC->nDepthTargets[ eSwapDT ] = eDTIndex;
	}

#else
	REF( eSwapDT );
	REF( nSwapChainIndex );
#endif

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void dxC_AddDescRTsGlobal()
{
	sAddDescRTs( gtGlobalRenderTargetSpecs, arrsize(gtGlobalRenderTargetSpecs), -1 );
}

void dxC_AddDescRTsDependOnBB( int nSwapChainIndex )
{
	sAddDescRTs( gtBackbufferLinkedRenderTargetSpecs, arrsize(gtBackbufferLinkedRenderTargetSpecs), nSwapChainIndex );
}


void dxC_AddRenderTargetDescs()
{
	gtRenderTargetDesc.Clear();

	if ( dxC_IsPixomaticActive() )
		return;

	for ( int nSwapChain = 0; nSwapChain < MAX_SWAP_CHAINS; ++nSwapChain )
	{
		if ( ! dxC_GetD3DSwapChain( nSwapChain ) )
			continue;
		dxC_AddDescRTsDependOnBB( nSwapChain );
	}
	dxC_AddDescRTsGlobal();

	V( e_ShadowsCreate() );

#ifdef ENGINE_TARGET_DX10
	V( dx10_AddFluidRTDescs() );
#endif

	// OK, it isn't really yet, but this assumes it will be a few functions later
	gbBlurInitialized = TRUE;
}




//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dxC_GetRenderTargetDimensions(
	int & nWidth,
	int & nHeight,
	RENDER_TARGET_INDEX nTarget /*= RENDER_TARGET_INVALID*/ )
{
	RENDER_TARGET_INDEX nRT = nTarget;
	if ( nRT == RENDER_TARGET_INVALID )
		nRT = dxC_GetCurrentRenderTarget();
#ifdef ENGINE_TARGET_DX10
	if( nRT == RENDER_TARGET_NULL )
	{
		DEPTH_TARGET_INDEX nDS = dxC_GetCurrentDepthTarget();

		if( nDS == DEPTH_TARGET_NONE )
		{
			nWidth = 0;
			nHeight = 0;
		}
		else
		{
			D3DC_TEXTURE2D_DESC tDesc;
			dxC_Get2DTextureDesc( dxC_DSResourceGet( nDS ), 0, &tDesc );
			nWidth = tDesc.Width;
			nHeight = tDesc.Height;
		}

		return S_OK;
	}
#endif

	//if ( ! bDontTranslate && e_GetRenderFlag( RENDER_FLAG_RENDER_TO_FULLRT ) )
	//{
	//	// If we're rendering directly to the FULL rendertarget as if it were the backbuffer, use the swap-chain's backbuffer size
	//	if ( nRT == RENDER_TARGET_FULL )
	//		nRT = RENDER_TARGET_BACKBUFFER;
	//}


	//if ( nSwapChainIndex == -1 )
	//	nSwapChainIndex = dxC_GetCurrentSwapChainIndex();
	//if ( ! dxC_GetSwapChain( nSwapChainIndex ) )
	//	nSwapChainIndex = DEFAULT_SWAP_CHAIN;
	//ASSERT( dxC_GetSwapChain( nSwapChainIndex ) );

	//if ( nRT == RENDER_TARGET_FULL )
	//	nRT = dxC_GetSwapChainFullRenderTarget( nSwapChainIndex );

	ASSERT_RETINVALIDARG( nRT != RENDER_TARGET_INVALID );
	D3D_RENDER_TARGET* pRT = HashGet( gtRenderTargets, nRT );
	ASSERT_RETFAIL( pRT );


	if ( pRT->bBackbuffer )
	{
		ASSERT_RETFAIL( dxC_GetD3DSwapChain( pRT->nSwapChainIndex ) );
		//e_GetWindowSize( nWidth, nHeight );
		const D3DPRESENT_PARAMETERS & tPP = dxC_GetD3DPP( pRT->nSwapChainIndex );
		nWidth = DXC_9_10( tPP.BackBufferWidth, tPP.BufferDesc.Width );
		nHeight = DXC_9_10( tPP.BackBufferHeight, tPP.BufferDesc.Height );
	}
	else
	{
		D3DC_TEXTURE2D_DESC tDesc;
		ASSERT_RETINVALIDARG( nRT >= 0 );
		ASSERT_RETINVALIDARG( nRT < giRenderTargetCount );
#ifdef ENGINE_TARGET_DX10
		V_RETURN( dxC_Get2DTextureDesc( dxC_RTResourceGet( nRT ), 0, &tDesc ) );
#else
		SPDIRECT3DSURFACE9 pSurf = dxC_RTSurfaceGet( nRT );
		ASSERT_RETFAIL( pSurf );
		V_RETURN( pSurf->GetDesc( &tDesc ) );
#endif
		nWidth  = tDesc.Width;
		nHeight = tDesc.Height;
	}

	return S_OK;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
DXC_FORMAT dxC_GetRenderTargetFormat( RENDER_TARGET_INDEX nTarget )
{
	RENDER_TARGET_INDEX nRT = nTarget;
	if ( nRT == RENDER_TARGET_INVALID )
		nRT = dxC_GetCurrentRenderTarget();
	ASSERT_RETVAL( nRT != RENDER_TARGET_INVALID, D3DFMT_UNKNOWN );
	ASSERT_RETVAL( nRT != RENDER_TARGET_NULL, D3DFMT_UNKNOWN );

	D3D_RENDER_TARGET* pRT = HashGet( gtRenderTargets, nRT );
	ASSERT_RETVAL( pRT, D3DFMT_UNKNOWN );

	//ASSERT_RETVAL( nRT != RENDER_TARGET_BACKBUFFER, D3DFMT_UNKNOWN );
	if ( pRT->bBackbuffer )
	{
		return dxC_GetD3DPP( pRT->nSwapChainIndex ).BackBufferFormat;
	} else
	{
		//if ( nRT == RENDER_TARGET_FULL )
		//	nRT = dxC_GetSwapChainFullRenderTarget( dxC_GetCurrentSwapChainIndex() );

		D3DC_TEXTURE2D_DESC tDesc;
		ASSERT_RETVAL( nRT >= 0, D3DFMT_UNKNOWN );
		ASSERT_RETVAL( nRT < giRenderTargetCount, D3DFMT_UNKNOWN );
		//ASSERT( dxC_RTShaderResourceGet( nRT ) );
		//HRVERIFY( dxC_RTShaderResourceGet( nRT )->GetLevelDesc( 0, &tDesc ) );
		V( dxC_Get2DTextureDesc( dxC_RTResourceGet( nRT ), 0, &tDesc ) );
		return tDesc.Format;
	}
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int dxC_GetRenderTargetSwapChainIndex( RENDER_TARGET_INDEX nTarget )
{
	RENDER_TARGET_INDEX nRT = nTarget;
	if ( nRT == RENDER_TARGET_INVALID )
		return INVALID_ID;

	D3D_RENDER_TARGET* pRT = HashGet( gtRenderTargets, nRT );
	ASSERT_RETVAL( pRT, INVALID_ID );

	return pRT->nSwapChainIndex;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int dxC_GetDepthTargetSwapChainIndex( DEPTH_TARGET_INDEX nTarget )
{
	DEPTH_TARGET_INDEX nDT = nTarget;
	if ( nDT == DEPTH_TARGET_INVALID || nDT == DEPTH_TARGET_NONE )
		return INVALID_ID;

	D3D_DS_TARGET* pDT = HashGet( gtDSTargets, nDT );
	ASSERT_RETVAL( pDT, INVALID_ID );

	return pDT->nSwapChainIndex;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BOOL dxC_GetRenderTargetIsBackbuffer( RENDER_TARGET_INDEX nTarget )
{
	RENDER_TARGET_INDEX nRT = nTarget;
	if ( nRT == RENDER_TARGET_INVALID )
		return FALSE;

	D3D_RENDER_TARGET* pRT = HashGet( gtRenderTargets, nRT );
	ASSERT_RETVAL( pRT, FALSE );

	return pRT->bBackbuffer;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dxC_CopyBackbufferToTexture( SPD3DCTEXTURE2D pDestTexture, CONST RECT * pBBRect, CONST RECT * pDestRect )
{
#ifdef ENGINE_TARGET_DX9
	SPDIRECT3DSURFACE9 pBackBufferSurf;
	SPDIRECT3DSURFACE9 pDestSurf;

	ASSERT_RETINVALIDARG( pDestTexture );
	
	pBackBufferSurf = dxC_RTSurfaceGet( dxC_GetSwapChainRenderTargetIndex( dxC_GetCurrentSwapChainIndex(), SWAP_RT_BACKBUFFER ) );
	ASSERT_RETINVALIDARG( pBackBufferSurf );
	V_RETURN( pDestTexture->GetSurfaceLevel( 0, &pDestSurf ) );
	ASSERT_RETFAIL( pDestSurf );

	V_RETURN( dxC_GetDevice()->StretchRect( pBackBufferSurf, pBBRect, pDestSurf, pDestRect, D3DTEXF_NONE ) );
#elif defined(ENGINE_TARGET_DX10)

	SPD3DCTEXTURE2D pBBTexture = dxC_RTResourceGet( dxC_GetSwapChainRenderTargetIndex( dxC_GetCurrentSwapChainIndex(), SWAP_RT_BACKBUFFER ) );
	
	if( e_OptionState_GetActive()->get_nMultiSampleType() > 1 )
	{
		if ( pBBRect && pDestRect )
		{
			// If MSAA is on but we have subrects, we have to resolve to a temp buffer and then copy
			RENDER_TARGET_INDEX nRT = dxC_GetSwapChainRenderTargetIndex( dxC_GetCurrentSwapChainIndex(), SWAP_RT_FULL );
			SPD3DCTEXTURE2D pTempTexture = dxC_RTResourceGet( nRT );
			ASSERT_RETFAIL( pTempTexture );
			dxC_GetDevice()->ResolveSubresource( pTempTexture, 0, pBBTexture, 0,  dxC_GetBackbufferRenderTargetFormat() );

			D3D10_BOX tSrcBox;
			tSrcBox.left		= pBBRect->left;
			tSrcBox.top			= pBBRect->top;
			tSrcBox.right		= pBBRect->right;
			tSrcBox.bottom		= pBBRect->bottom;
			tSrcBox.front		= 0;
			tSrcBox.back		= 1;
			dxC_GetDevice()->CopySubresourceRegion( pDestTexture, 0, (UINT)pDestRect->left, (UINT)pDestRect->top, 0, pTempTexture, 0, &tSrcBox );
		}
		else
		{
			dxC_GetDevice()->ResolveSubresource( pDestTexture, 0, pBBTexture, 0,  dxC_GetBackbufferRenderTargetFormat() );
		}
	}
	else
	{
		if ( pBBRect && pDestRect )
		{
			D3D10_BOX tSrcBox;
			tSrcBox.left		= pBBRect->left;
			tSrcBox.top			= pBBRect->top;
			tSrcBox.right		= pBBRect->right;
			tSrcBox.bottom		= pBBRect->bottom;
			tSrcBox.front		= 0;
			tSrcBox.back		= 1;
			dxC_GetDevice()->CopySubresourceRegion( pDestTexture, 0, (UINT)pDestRect->left, (UINT)pDestRect->top, 0, pBBTexture, 0, &tSrcBox );
		}
		else
		{
			dxC_GetDevice()->CopyResource( pDestTexture, pBBTexture );
		}
	}
#endif

	return S_OK;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_CopyRTFullMSAAToBackBuffer()
{
#ifdef ENGINE_TARGET_DX9
	int nSwapChainIndex = dxC_GetCurrentSwapChainIndex();

	if ( 0 == ( e_GetRenderFlag( RENDER_FLAG_RENDER_TO_FULLRT ) & MAKE_MASK(nSwapChainIndex) ) )
	{
		return S_FALSE;
	}

	// turn off fog
	V( e_SetFogEnabled( FALSE ) );

	// get the effect
	D3D_EFFECT * pEffect = dxC_EffectGet( dx9_EffectGetNonMaterialID( NONMATERIAL_EFFECT_GAMMA ) );
	if ( ! pEffect )
		return S_FALSE;
	if ( ! dxC_EffectIsLoaded( pEffect ) )
		return S_FALSE;

	EFFECT_TECHNIQUE * ptTechnique = NULL;
	TECHNIQUE_FEATURES tFeatures;
	tFeatures.Reset();
	tFeatures.nInts[ TF_INT_INDEX ] = 1;
	V( dxC_EffectGetTechniqueByFeatures( pEffect, tFeatures, &ptTechnique ) );
	ASSERT_RETFAIL( ptTechnique && ptTechnique->hHandle );

	UINT nPassCount;
	V_RETURN( dxC_SetTechnique( pEffect, ptTechnique, &nPassCount ) );
	ASSERT_RETFAIL( nPassCount == 1 );

	UINT nPass = 0;
	{
		D3DC_EFFECT_PASS_HANDLE hPass = dxC_EffectGetPassByIndex( pEffect->pD3DEffect, ptTechnique->hHandle, nPass );
		ASSERT(hPass != 0);
		if (hPass != 0)
		{
			RENDER_TARGET_INDEX nFullMSAART = dxC_GetSwapChainRenderTargetIndexEx( nSwapChainIndex, SWAP_RT_FULL_MSAA );
			RENDER_TARGET_INDEX nFullRT = dxC_GetSwapChainRenderTargetIndexEx( nSwapChainIndex, SWAP_RT_FULL );
			RENDER_TARGET_INDEX nBackRT = dxC_GetSwapChainRenderTargetIndexEx( nSwapChainIndex, SWAP_RT_FINAL_BACKBUFFER );


			// First copy FULL_MSAA to FULL.  This forces an MSAA resolve, and we can texture out of FULL.
			SPD3DCRENDERTARGETVIEW pFullMSAARTV = dxC_RTSurfaceGet( nFullMSAART );
			ASSERT_RETFAIL( pFullMSAARTV );
			SPD3DCRENDERTARGETVIEW pFullRTV = dxC_RTSurfaceGet( nFullRT );
			ASSERT_RETFAIL( pFullRTV );

			V_RETURN( dxC_GetDevice()->StretchRect(
				pFullMSAARTV, 
				NULL, 
				pFullRTV, 
				NULL, 
				D3DTEXF_LINEAR ) );

			// The contents of FULL_MSAA should be resolved an in FULL, ready to texture out of!
			SPD3DCBASESHADERRESOURCEVIEW pSrcTexture = dxC_RTShaderResourceGet( nFullRT );
			ASSERT_RETFAIL( pSrcTexture );

			V_RETURN( dxC_EffectBeginPass( pEffect, nPass ) );
			V_RETURN( dxC_SetRenderTargetWithDepthStencil( SWAP_RT_FINAL_BACKBUFFER, DEPTH_TARGET_NONE, 0, nSwapChainIndex ) );
			V( dxC_EffectSetPixelAccurateOffset( pEffect, *ptTechnique ) );
			V( dxC_EffectSetScreenSize( pEffect, *ptTechnique, nFullRT ) );  // fills in the full source texture size
			V( dxC_EffectSetViewport( pEffect, *ptTechnique, nBackRT ) );	// fills in the source rect (viewport into the source texture)
			V( dx9_EffectSetTexture( pEffect, *ptTechnique, EFFECT_PARAM_DIFFUSEMAP0, pSrcTexture ) );
			dxC_SetRenderState( D3DRS_ZENABLE, FALSE );
			dxC_SetRenderState( D3DRS_ZWRITEENABLE, FALSE );
			V_RETURN(dx9_ScreenDrawCover(pEffect));
		}
	}
#endif // DX9
	return S_OK;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dxC_CopyRenderTargetToTexture( RENDER_TARGET_INDEX nRTID, SPD3DCTEXTURE2D& pDestTexture, BOUNDING_BOX & pBBox )
{
	return E_NOTIMPL;

	// can't copy from card -> managed
	// must copy card -> same size sysmem tex
	// then lockrect managed tex and blt up sysmem tex
	D3DC_TEXTURE2D_DESC tDesc;
	V( dxC_Get2DTextureDesc( pDestTexture, 0, &tDesc ) );


#ifdef ENGINE_TARGET_DX9  //Messy...I like DX10's staging resources better
	SPDIRECT3DSURFACE9 pSourceSurf;
	SPDIRECT3DSURFACE9 pDestSystemSurf, pDestCardSurf;

	ASSERT_RETINVALIDARG( nRTID >= 0 && nRTID < giRenderTargetCount );
	ASSERT_RETINVALIDARG( pDestTexture );
	//ASSERT( c_sGetRenderTarget() != nRTID );
	pSourceSurf = dxC_RTSurfaceGet( nRTID );

	V_RETURN( pDestTexture->GetSurfaceLevel( 0, &pDestCardSurf ) );
	ASSERT_RETFAIL( dxC_RTSystemTextureGet( nRTID ) );
	V_RETURN( dxC_RTSystemTextureGet( nRTID )->GetSurfaceLevel( 0, &pDestSystemSurf ) );

	RECT tSourceRect, tDestRect;
	D3DC_VIEWPORT tVP;
	V_RETURN( dxC_GetDevice()->GetViewport( &tVP ) );
	SetRect( &tSourceRect,
		(int)tVP.TopLeftX, 
		(int)tVP.TopLeftY, 
		(int)tVP.TopLeftX + tVP.Width, 
		(int)tVP.TopLeftY + tVP.Height );
	SetRect( &tDestRect,
		(int)pBBox.vMin.fX,
		(int)pBBox.vMin.fY,
		(int)pBBox.vMin.fX + tVP.Width,
		(int)pBBox.vMin.fY + tVP.Height );

//#ifdef _DEBUG
	//D3DC_TEXTURE2D_DESC tDesc;
	//HRVERIFY( pDestCardSurf->GetDesc( &tDesc ) );
	ASSERT_RETFAIL( tDestRect.right <= (int)tDesc.Width && tDestRect.bottom <= (int)tDesc.Height );
//#endif

#ifdef VID_TO_SYS_TIMERS
	if ( 0 )
	{
		TIMER_START( "sync" );
		SPD3DCQUERY pQuery;
		V_RETURN( dxC_GetDevice()->CreateQuery( D3DQUERYTYPE_EVENT, &pQuery ) );
		V_RETURN( pQuery->Issue( D3DISSUE_END ) );
		while ( pQuery->GetData( NULL, 0, D3DGETDATA_FLUSH ) == S_FALSE )
			;
		UINT64 iiMSec = TIMER_END();
		//DebugString( OP_DEBUG, "###  %I64u ms in query sync", iiMSec );
		trace( "###  %I64u ms in query sync\n", iiMSec );
	}

	if ( 0 )
	{
		TIMER_START( "prelr" );
		//DWORD dwFlags = D3DLOCK_DONOTWAIT;
		DWORD dwFlags = 0;
		D3DLOCKED_RECT tRect;
		PRESULT pr = pDestSystemSurf->LockRect( &tRect, NULL, dwFlags );
		UINT64 iiMSec = TIMER_END();
		//DebugString( OP_DEBUG, "###  %I64u ms in pre  LockRect flags %d result %d", iiMSec, dwFlags, hr );
		trace( "###  %I64u ms in pre  LockRect flags %d result %d\n", iiMSec, dwFlags, hr );
		if ( SUCCEEDED(pr) )
			V( pDestSystemSurf->UnlockRect() );
	}

	{
		TIMER_START( "grtd" );
#endif // VID_TO_SYS_TIMERS

		D3DC_TEXTURE2D_DESC tSourceDesc;
		V_RETURN( pSourceSurf->GetDesc( &tSourceDesc ) );
		D3DC_TEXTURE2D_DESC tDescSysDesc;
		V_RETURN( pDestSystemSurf->GetDesc( &tDescSysDesc ) );
		ASSERT_RETFAIL( tSourceDesc.Format == tDescSysDesc.Format );
		ASSERT_RETFAIL( tSourceDesc.Width  == tDescSysDesc.Width );
		ASSERT_RETFAIL( tSourceDesc.Height == tDescSysDesc.Height );

		V_RETURN( dxC_GetDevice()->GetRenderTargetData( pSourceSurf, pDestSystemSurf ) );


#ifdef VID_TO_SYS_TIMERS
		UINT64 iiMSec = TIMER_END();
		//DebugString( OP_DEBUG, "###  %I64u ms in GetRenderTargetData", iiMSec );
		trace( "###  %I64u ms in GetRenderTargetData (%dx%d)\n", iiMSec, tDesc.Width, tDesc.Height );
	}

	if ( 0 )
	{
		TIMER_START( "postlr" );
		DWORD dwFlags = D3DLOCK_DONOTWAIT;
		//DWORD dwFlags = 0;
		D3DLOCKED_RECT tRect;
		PRESULT pr = pDestSystemSurf->LockRect( &tRect, NULL, dwFlags );
		UINT64 iiMSec = TIMER_END();
		//DebugString( OP_DEBUG, "###  %I64u ms in post LockRect flags %d result %d", iiMSec, dwFlags, hr );
		trace( "###  %I64u ms in post LockRect flags %d result %d\n", iiMSec, dwFlags, hr );
		if ( SUCCEEDED(pr) )
			V( pDestSystemSurf->UnlockRect() );
	}

	{
		TIMER_START( "lsfs" );
#endif // VID_TO_SYS_TIMERS


		V_RETURN( D3DXLoadSurfaceFromSurface( pDestCardSurf, NULL, &tDestRect, pDestSystemSurf, NULL, &tSourceRect, D3DX_FILTER_NONE, 0 ) );


#ifdef VID_TO_SYS_TIMERS
		UINT64 iiMSec = TIMER_END();
		//DebugString( OP_DEBUG, "###  %I64u ms in LoadSurfaceFromSurface", iiMSec );
		trace( "###  %I64u ms in LoadSurfaceFromSurface\n", iiMSec );
	}
#endif // VID_TO_SYS_TIMERS

	//gpDebugTexture = pDestTexture;
#elif defined(ENGINE_TARGET_DX10)
	SPD3DCTEXTURE2D pSrc = dxC_RTResourceGet( nRTID );

	if( dx9_IsCompressedTextureFormat( tDesc.Format ) )
	{
#ifdef DXTLIB
		V_RETURN( dxC_CompressToGPUTexture( pSrc, pDestTexture  ) );
#endif
	}
	else
		dxC_GetDevice()->CopyResource( pDestTexture, pSrc );
#endif

	return S_OK;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dxC_AddRenderToTexture( int nModelID, SPD3DCTEXTURE2D pDestTexture, RECT * pDestVPRect, int nEnvDefID, RECT * pDestSpacingRect )
{
	return E_NOTIMPL;

	ASSERT_RETINVALIDARG( pDestTexture && pDestVPRect );

	// weird hack here -- see comment in sRenderInterfaceCommand
	BOUNDING_BOX tBBox;
	float fZMin = (float)pDestVPRect->right;
	float fZMax = (float)pDestVPRect->bottom;
	if ( pDestSpacingRect ) 
	{
		fZMin = (float)pDestSpacingRect->right;
		fZMax = (float)pDestSpacingRect->bottom;
	}
	tBBox.vMin = VECTOR( (float)pDestVPRect->left,  (float)pDestVPRect->top,    fZMin  );
	tBBox.vMax = VECTOR( (float)pDestVPRect->right, (float)pDestVPRect->bottom, fZMax );

	DL_SET_STATE_DWORD( DRAWLIST_INTERFACE, DLSTATE_OVERRIDE_ENVDEF, nEnvDefID );

	D3D_DRAWLIST_COMMAND * pCommand = dx9_DrawListAddCommand( DRAWLIST_INTERFACE );
	ASSERT_RETFAIL( pCommand );
	pCommand->nCommand	= DLCOM_DRAW;
	pCommand->nID		= nModelID;
	pCommand->pData		= pDestTexture;
	pCommand->bBox		= tBBox;

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT e_AddRenderToTexture( int nModelID, int nDestTextureID, RECT * pDestVPRect, int nEnvDefID, RECT * pDestSpacingRect )
{
	return E_NOTIMPL;

	D3D_TEXTURE * pTexture = dxC_TextureGet( nDestTextureID );
	ASSERT_RETINVALIDARG( pTexture );
	ASSERT_RETFAIL( pTexture->pD3DTexture );
	ASSERT_RETINVALIDARG( pDestVPRect );
	return dxC_AddRenderToTexture( nModelID, pTexture->pD3DTexture, pDestVPRect, nEnvDefID, pDestSpacingRect );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_TargetBuffersInit()
{
	ArrayInit(gtRenderTargetDesc, NULL, NUM_STATIC_GLOBAL_RENDER_TARGETS);
	HashInit( gtRenderTargets, NULL, NUM_STATIC_GLOBAL_RENDER_TARGETS );
	HashInit( gtDSTargets, NULL, max( 2, NUM_STATIC_GLOBAL_DEPTH_TARGETS ) );

	//for( int i = 0; i < NUM_STATIC_GLOBAL_RENDER_TARGETS; ++i )
	//{
	//	sRenderTargetHashAdd( i, FALSE, -1, i );
	//	//HashAdd( gtRenderTargets, i );
	//}

	//for( int i = 0; i < NUM_STATIC_GLOBAL_DEPTH_TARGETS; ++i )
	//{
	//	sDepthTargetHashAdd( i, -1, i );
	//	//HashAdd( gtDSTargets, i );
	//}

	for ( int i = 0; i < NUM_GLOBAL_DEPTH_TARGETS; ++i )
		gnGlobalDepthTargetIndex[ i ] = DEPTH_TARGET_INVALID;
	for ( int i = 0; i < NUM_GLOBAL_RENDER_TARGETS; ++i )
		gnGlobalRenderTargetIndex[ i ] = RENDER_TARGET_INVALID;

	for ( int i = 0; i < MAX_SWAP_CHAINS; ++i )
	{
		DXCSWAPCHAIN * ptSC = dxC_GetDXCSwapChain( i );
		ASSERT_CONTINUE(ptSC);
		ptSC->pSwapChain = NULL;
		memset( ptSC->nRenderTargets, -1, sizeof(ptSC->nRenderTargets) );
		memset( ptSC->nDepthTargets,  -1, sizeof(ptSC->nDepthTargets) );
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PRESULT dxC_TargetBuffersRelease()
{
	// release render targets and Z Buffers
	for( D3D_RENDER_TARGET* pRT = HashGetFirst( gtRenderTargets ); pRT; pRT = HashGetNext( gtRenderTargets, pRT ) )
	{
		pRT->pResource = NULL;
		pRT->pRTV = NULL;
		DX9_BLOCK( pRT->pSystemResource = NULL; )
		DX10_BLOCK( pRT->pSRV = NULL; )
	}

	for( D3D_DS_TARGET* pDSTarget = HashGetFirst( gtDSTargets ); pDSTarget; pDSTarget = HashGetNext( gtDSTargets, pDSTarget ) )
	{
		pDSTarget->pResource = NULL;
		pDSTarget->pDSV = NULL;
		DX10_BLOCK( pDSTarget->pSRV = NULL; )
	}

	for ( int i = 0; i < MAX_SWAP_CHAINS; ++i )
	{
		DXCSWAPCHAIN * ptSC = dxC_GetDXCSwapChain( i );
		ASSERT_CONTINUE(ptSC);
		memset( ptSC->nRenderTargets, -1, sizeof(ptSC->nRenderTargets) );
		memset( ptSC->nDepthTargets,  -1, sizeof(ptSC->nDepthTargets) );
	}

	HashClear( gtRenderTargets );
	HashClear( gtDSTargets );

	giRenderTargetCount = 0;
	giDepthStencilCount = 0;

	gtRenderTargetDesc.Clear();
	gBStaticRTsCreated = false;

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PRESULT dxC_TargetBuffersDestroy()
{
	gtRenderTargetDesc.Destroy();
	V_RETURN( dxC_TargetBuffersRelease() );

	HashFree(gtRenderTargets);
	HashFree(gtDSTargets);

	return S_OK;
}
