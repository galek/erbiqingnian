//----------------------------------------------------------------------------
// dxC_target.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DXC_TARGET__
#define __DXC_TARGET__

#include "boundingbox.h"
#include "dxC_texture.h"

#define LARGEST_FLUID_WIDTH  70 
#define LARGEST_FLUID_HEIGHT 70
#define LARGEST_FLUID_DEPTH  200

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define MAX_MRTS_ALLOWED 3

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

enum RENDER_TARGET_INDEX
{
	RENDER_TARGET_NULL = -2,
	RENDER_TARGET_INVALID = -1,
};

enum SHADOWMAP
{
	SHADOWMAP_HIGH,
	SHADOWMAP_MID,
	SHADOWMAP_LOW,
	NUM_SHADOWMAPS,
};

enum DEPTH_TARGET_INDEX
{
	DEPTH_TARGET_INVALID = -2,
	DEPTH_TARGET_NONE = -1,
};


// Render targets created once globally
enum GLOBAL_RENDER_TARGET
{
	GLOBAL_RT_INVALID = -1,

	GLOBAL_RT_SHADOW_COLOR,
	GLOBAL_RT_SHADOW_COLOR_MID,
	GLOBAL_RT_SHADOW_COLOR_LOW,

	GLOBAL_RT_LIGHTING_MAP,			// mythos lighting map

	GLOBAL_RT_FLUID_TEMP1,          //for smoke simulation; shared texture between all smoke instances
	GLOBAL_RT_FLUID_TEMP2,          //for smoke simulation; shared texture between all smoke instances
	GLOBAL_RT_FLUID_TEMPVELOCITY,   //for smoke simulation; shared texture between all smoke instances

#define DEFINE_ONLY_GLOBAL_TARGETS
#	include "dxC_target_def.h"

	// count
	NUM_GLOBAL_RENDER_TARGETS,
	// markers
	SHADOW_RENDER_TARGET_START = GLOBAL_RT_SHADOW_COLOR,
	NUM_STATIC_GLOBAL_RENDER_TARGETS = GLOBAL_RT_FLUID_TEMPVELOCITY+1
};

enum GLOBAL_DEPTH_TARGET
{
	GLOBAL_DT_INVALID = -1,
	// shadowmaps (from SHADOWMAP enum)
	_GLOBAL_DT_FIRST_SHADOWMAP,
	_GLOBAL_DT_LAST_SHADOWMAP = NUM_SHADOWMAPS-1,
	// count
	NUM_GLOBAL_DEPTH_TARGETS,
	// markers
	NUM_STATIC_GLOBAL_DEPTH_TARGETS = _GLOBAL_DT_LAST_SHADOWMAP+1
};


// Render targets created per-swap-chain
enum SWAP_CHAIN_RENDER_TARGET
{
	SWAP_RT_BACKBUFFER = -2,				// BACKBUFFER is a virtual rendertarget, mapping to either FINAL_BACKBUFFER or FULL
	SWAP_RT_INVALID = -1,

	//SWAP_RT_BACKBUFFER,						// if ( RENDER_FLAG_RENDER_TO_FULLRT ), same as RT_FULL
	//										// else, swap chain backbuffer

	SWAP_RT_FINAL_BACKBUFFER,				// in HDR mode this is the final LDR backbuffer, otherwise:
											// if ( RENDER_FLAG_RENDER_TO_FULLRT ), final swap chain backbuffer
											// else same as BACKBUFFER

#define DEFINE_ONLY_BACKBUFFER_LINKED_TARGETS
#	include "dxC_target_def.h"

	// count per swap-chain
	NUM_SWAP_CHAIN_RENDER_TARGETS,
	// markers
	NUM_STATIC_SWAP_CHAIN_RENDER_TARGETS = SWAP_RT_FINAL_BACKBUFFER+1
};

enum SWAP_CHAIN_DEPTH_TARGET
{
	SWAP_DT_INVALID = -1,
	SWAP_DT_AUTO,
	// count per swap-chain
	NUM_SWAP_CHAIN_DEPTH_TARGETS,
	// markers
	//NUM_STATIC_SWAP_CHAIN_DEPTH_TARGETS = SWAP_DT_AUTO+1
};







// TEMP
typedef int ZBUFFER_TARGET;



//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

struct DXCSWAPCHAIN
{
	SPD3DCSWAPCHAIN pSwapChain;
	int nRenderTargets[ NUM_SWAP_CHAIN_RENDER_TARGETS ];
	int nDepthTargets[ NUM_SWAP_CHAIN_DEPTH_TARGETS ];
};

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

inline SPD3DCSHADERRESOURCEVIEW & dxC_DebugTextureGetShaderResource()
{
	extern SPD3DCSHADERRESOURCEVIEW gpDebugTextureView;
	return gpDebugTextureView;
}

inline PRESULT dxC_GetRenderTargetViewDesc( LPD3DCRENDERTARGETVIEW pRTV, D3DC_RENDER_TARGET_VIEW_DESC* pDesc)
{
	ASSERT_RETINVALIDARG( pRTV );
	DX9_BLOCK( V_RETURN( pRTV->GetDesc( pDesc ) ) );
	DX10_BLOCK( pRTV->GetDesc( pDesc ) );

	return S_OK;
}

inline PRESULT dxC_GetDepthStencilViewDesc( LPD3DCDEPTHSTENCILVIEW pDSV, D3DC_DEPTH_STENCIL_VIEW_DESC* pDesc)
{
	ASSERT_RETINVALIDARG( pDSV );
	DX9_BLOCK( V_RETURN( pDSV->GetDesc( pDesc ) ) );
	DX10_BLOCK( pDSV->GetDesc( pDesc ) );

	return S_OK;
}

inline PRESULT dxC_GetShaderResourceViewDesc( LPD3DCSHADERRESOURCEVIEW pSRV, D3DC_SHADER_RESOURCE_VIEW_DESC* pDesc)
{
	ASSERT_RETINVALIDARG( pSRV );
	DX9_BLOCK( V_RETURN( pSRV->GetLevelDesc( 0, pDesc ) ) );
	DX10_BLOCK( pSRV->GetDesc( pDesc ) );

	return S_OK;
}

#ifdef ENGINE_TARGET_DX10
PRESULT dx10_RenderTargetAddSRV( RENDER_TARGET_INDEX nRTID );
#endif

SPD3DCTEXTURE2D & dxC_RTResourceGet( RENDER_TARGET_INDEX nTarget, BOOL bUseSubResourceIfAvailable = FALSE );
SPD3DCRENDERTARGETVIEW & dxC_RTSurfaceGet( RENDER_TARGET_INDEX nTarget );
SPD3DCSHADERRESOURCEVIEW & dxC_RTShaderResourceGet( RENDER_TARGET_INDEX nTarget );
void dxC_RTReleaseAll( RENDER_TARGET_INDEX nTarget );

SPD3DCDEPTHSTENCILVIEW & dxC_DSSurfaceGet( DEPTH_TARGET_INDEX nTarget );

//inline SPD3DCDEPTHSTENCILVIEW & dxC_DSSurfaceGet( DEPTH_TARGET_INDEX nTarget )
//{
//	return dxC_DSSurfaceGet( nTarget );
//}

//----------------------------------------------------------------------------
SPD3DCTEXTURE2D & dxC_DSResourceGet( DEPTH_TARGET_INDEX nTarget );

//inline SPD3DCTEXTURE2D & dxC_DSResourceGet( DEPTH_TARGET_INDEX nTarget )
//{
//	return dxC_DSResourceGet( nTarget );
//}



//----------------------------------------------------------------------------
SPD3DCSHADERRESOURCEVIEW & dxC_DSShaderResourceView( DEPTH_TARGET_INDEX nTarget );

inline SPD3DCSHADERRESOURCEVIEW & dxC_DSShaderResourceView( SHADOWMAP nTarget )
{
	return dxC_DSShaderResourceView( (DEPTH_TARGET_INDEX)nTarget );
}

//----------------------------------------------------------------------------

inline LPD3DCSWAPCHAIN dxC_GetD3DSwapChain( int nIndex = DEFAULT_SWAP_CHAIN )
{
	ASSERT_RETNULL( IS_VALID_INDEX(nIndex, MAX_SWAP_CHAINS) );

	extern DXCSWAPCHAIN gtSwapChains[ MAX_SWAP_CHAINS ];
	return gtSwapChains[nIndex].pSwapChain;
}

//----------------------------------------------------------------------------

RENDER_TARGET_INDEX dxC_GetSwapChainRenderTargetIndexEx( int nSwapChainIndex, SWAP_CHAIN_RENDER_TARGET eRT );
RENDER_TARGET_INDEX dxC_GetSwapChainRenderTargetIndex( int nSwapChainIndex, SWAP_CHAIN_RENDER_TARGET eRT );

//----------------------------------------------------------------------------

inline DXCSWAPCHAIN * dxC_GetDXCSwapChain( int nIndex = DEFAULT_SWAP_CHAIN )
{
	ASSERT_RETNULL( IS_VALID_INDEX(nIndex, MAX_SWAP_CHAINS) );

	extern DXCSWAPCHAIN gtSwapChains[ MAX_SWAP_CHAINS ];
	return &gtSwapChains[nIndex];
}

//----------------------------------------------------------------------------

inline DEPTH_TARGET_INDEX dxC_GetSwapChainDepthTargetIndex( int nSwapChainIndex, SWAP_CHAIN_DEPTH_TARGET eDT )
{
	ASSERT_RETVAL( IS_VALID_INDEX(nSwapChainIndex, MAX_SWAP_CHAINS), DEPTH_TARGET_INVALID );
	ASSERT_RETVAL( IS_VALID_INDEX(eDT, NUM_SWAP_CHAIN_DEPTH_TARGETS), DEPTH_TARGET_INVALID );

	extern DXCSWAPCHAIN gtSwapChains[ MAX_SWAP_CHAINS ];
	return (DEPTH_TARGET_INDEX)gtSwapChains[nSwapChainIndex].nDepthTargets[eDT];
}

//----------------------------------------------------------------------------

inline RENDER_TARGET_INDEX dxC_GetGlobalRenderTargetIndex( GLOBAL_RENDER_TARGET eRT )
{
	ASSERT_RETVAL( IS_VALID_INDEX(eRT, NUM_GLOBAL_RENDER_TARGETS), RENDER_TARGET_INVALID );

	extern RENDER_TARGET_INDEX gnGlobalRenderTargetIndex[NUM_GLOBAL_RENDER_TARGETS];
	return gnGlobalRenderTargetIndex[eRT];
}

//----------------------------------------------------------------------------

inline DEPTH_TARGET_INDEX dxC_GetGlobalDepthTargetIndex( GLOBAL_DEPTH_TARGET eDT )
{
	ASSERT_RETVAL( IS_VALID_INDEX(eDT, NUM_GLOBAL_DEPTH_TARGETS), DEPTH_TARGET_INVALID );

	extern DEPTH_TARGET_INDEX gnGlobalDepthTargetIndex[NUM_GLOBAL_DEPTH_TARGETS];
	return gnGlobalDepthTargetIndex[eDT];
}

//----------------------------------------------------------------------------

inline GLOBAL_DEPTH_TARGET dxC_GetGlobalDepthTargetFromShadowMap( SHADOWMAP eShadowMap )
{
	ASSERT_RETVAL( IS_VALID_INDEX(eShadowMap, NUM_SHADOWMAPS), GLOBAL_DT_INVALID );
	return (GLOBAL_DEPTH_TARGET)( (int)eShadowMap + (int)_GLOBAL_DT_FIRST_SHADOWMAP );
}

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

PRESULT dxC_ClearSetRenderTargetCache();

UINT dxC_GetCurrentRenderTargetCount();
DEPTH_TARGET_INDEX  dxC_GetCurrentDepthTarget();
RENDER_TARGET_INDEX dxC_GetCurrentRenderTarget( int nMRTIndex = 0 );

DXC_FORMAT dxC_GetCurrentDepthTargetFormat();

#ifdef ENGINE_TARGET_DX9
SPD3DCTEXTURE2D & dxC_RTSystemTextureGet( RENDER_TARGET_INDEX nTarget );
#endif


PRESULT dxC_SetRenderTargetWithDepthStencil( GLOBAL_RENDER_TARGET eGlobalRT, DEPTH_TARGET_INDEX eDT, UINT nLevel = 0 );
PRESULT dxC_SetRenderTargetWithDepthStencil( SWAP_CHAIN_RENDER_TARGET eSwapRT, DEPTH_TARGET_INDEX eDT, UINT nLevel = 0, int nSwapChainIndex = -1 );
PRESULT dxC_SetRenderTargetWithDepthStencil( RENDER_TARGET_INDEX nRenderTarget, DEPTH_TARGET_INDEX nDepthTarget, UINT nLevel = 0 );
PRESULT dxC_SetMultipleRenderTargetsWithDepthStencil( UINT nRenderTargetCount, RENDER_TARGET_INDEX* pnRenderTarget, DEPTH_TARGET_INDEX nDepthTarget, UINT nLevel = 0 );
PRESULT dxC_RestoreRenderTargets();
void dxC_AddRenderTargetDescs();
void dxC_AddDescRTsDependOnBB( int nSwapChainIndex );
void dxC_ReleaseRTsDependOnBB( int nSwapChainIndex );
PRESULT dxC_AddAutoDepthTargetDesc( SWAP_CHAIN_DEPTH_TARGET eSwapDT, int nSwapChainIndex );
int dxC_AddRenderTargetDesc(
	BOOL bBackBuffer,
	int nSwapChainIndex,
	int nReservedEnum,
	DWORD dwWidth,
	DWORD dwHeight,
	D3DC_USAGE tBinding,
	DXC_FORMAT eFormat,
	int nIndex = -1,
	BOOL bDX9SysCopy = FALSE,
	int nPriority = 1000,
	UINT nMSAASamples = 1,
	UINT nMSAAQuality = 0,
	BOOL bMipChain = FALSE );
void dxC_DumpRenderTargets(void);	// CHB 2007.01.18
void dxC_DebugRenderTargetsGetList( WCHAR * pwszStr, int nBufLen );
void dxC_RenderTargetsMarkFrame();

DXC_FORMAT dxC_GetRenderTargetFormat( RENDER_TARGET_INDEX nTarget );
//Just use dxC_ClearBackBufferPrimaryZ
//void dx9_ClearBuffers(
//	BOOL bClearColor,
//	BOOL bClearDepth,
//	DWORD dwColor = 0,
//	float fDepth = 1.0f,
//	DWORD dwStencil = 0 );
PRESULT dxC_CopyBackbufferToTexture( SPD3DCTEXTURE2D pDestTexture, CONST RECT * pBBRect, CONST RECT * pDestRect );
PRESULT dxC_CopyRenderTargetToTexture( RENDER_TARGET_INDEX nRTID, SPD3DCTEXTURE2D& pDestTexture, BOUNDING_BOX & pBBox );
PRESULT dxC_AddRenderToTexture( int nModelID, SPD3DCTEXTURE2D pDestTexture, RECT * pDestVPRect, int nEnvDefID, RECT * pDestSpacingRect );
PRESULT dxC_TargetBuffersInit();
PRESULT dxC_TargetBuffersRelease();
PRESULT dxC_TargetBuffersDestroy();
PRESULT dxC_GetRenderTargetDimensions( int & nWidth, int & nHeight, RENDER_TARGET_INDEX nTarget = RENDER_TARGET_INVALID );
int dxC_GetRenderTargetSwapChainIndex( RENDER_TARGET_INDEX nTarget );
int dxC_GetDepthTargetSwapChainIndex( DEPTH_TARGET_INDEX nTarget );
BOOL dxC_GetRenderTargetIsBackbuffer( RENDER_TARGET_INDEX nTarget );

PRESULT dxC_RTMarkClearToken( RENDER_TARGET_INDEX nRTID );
PRESULT dxC_RTGetClearToken( RENDER_TARGET_INDEX nRTID, DWORD& dwClearToken );
PRESULT dxC_RTSetDirty( RENDER_TARGET_INDEX nRTID, BOOL bDirty = TRUE );
PRESULT dxC_RTGetDirty( RENDER_TARGET_INDEX nRTID, BOOL &bDirty );
PRESULT dxC_DSSetDirty( DEPTH_TARGET_INDEX nDSID, BOOL bDirty = TRUE );
PRESULT dxC_DSGetDirty( DEPTH_TARGET_INDEX nDSID, BOOL &bDirty );
PRESULT dxC_TargetSetCurrentDirty( BOOL bDirty = TRUE );
PRESULT dxC_TargetSetAllDirty( BOOL bDirty = TRUE );

PRESULT dxC_CopyRTFullMSAAToBackBuffer();

void dxC_GetRenderTargetName( int nRT, char * pszStr, int nBufLen );
void dxC_GetDepthTargetName( int nDT, char * pszStr, int nBufLen );

PRESULT dxC_TextureCopy(
	int nDestTexture,
	SPD3DCTEXTURE2D pD3DSource,
	int nStartingMip );

#ifdef ENGINE_TARGET_DX10
PRESULT dx10_SetRenderTargetWithDepthStencil( UINT nRenderTargetCount, RENDER_TARGET_INDEX* pnRenderTarget, DEPTH_TARGET_INDEX nDepthTarget, UINT nLevel = 0 );
#endif

inline int dxC_GetNumRenderTargets()
{
	extern int giRenderTargetCount;
	return giRenderTargetCount;
}

#endif //__DXC_TARGET__