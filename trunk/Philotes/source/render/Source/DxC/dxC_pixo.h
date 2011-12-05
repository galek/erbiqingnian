//----------------------------------------------------------------------------
// dxC_pixo.h
//
// (C)Copyright 2008, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DXC_PIXO__
#define __DXC_PIXO__

#ifdef USE_PIXO
#	include "pixomatic.h"
#endif

namespace FSSE
{;

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define PIXO_TEXTURE_FORMAT			D3DFMT_A8R8G8B8
#define PIXO_INDEX_FORMAT			D3DFMT_INDEX16

#define PIXOMATIC_SKIP_MIP_LEVELS	1

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

typedef void * PIXO_VERTEX_DATA;
typedef unsigned short * PIXO_INDEX_DATA;
typedef void * PIXO_TEXTURE_DATA;
typedef const unsigned int* PIXO_VERTEX_DECL;

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

#ifdef USE_PIXO

struct PIXO_SCREEN_BUFFERS
{
	// -------------------------------------------------------
	// Configuration and reflection
	// -------------------------------------------------------
	int nBufferWidth;
	int nBufferHeight;
	// Pixo buffer creation flags
	int nFlags;
	PIXO_ZBUFFER_TYPE eZBufferType;

	// -------------------------------------------------------
	// Created Pixo data
	// -------------------------------------------------------
	// our main Pixo buffer
	PIXO_BUF *pPixoBuffer;
	// pointer to out z buffer possibly offset to avoid 64k aliasing
	void *pZBuffer;
	// pointer to virtualalloc'd z buffer block
	void *pZ_buffer_block;
	int pZ_buffer_pitch;
};

#endif

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

#ifdef USE_PIXO

//// Pixomatic Direct3D Wrapper prototype. Pass in NULL for d3dInstance.
//extern "C" __declspec(dllimport) IDirect3D9 * WINAPI PixoDirect3DCreate9(
//	UINT SDKVersion);

#endif

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

#ifndef USE_PIXO
#	define dxC_IsPixomaticActive()		(FALSE)
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

inline BOOL dxC_CanUsePixomatic()
{
#if defined(USE_PIXO) && defined(ENGINE_TARGET_DX9)
	// this should check engine target setting
	return AppIsTugboat();
#endif
	return FALSE;
}

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

#ifdef USE_PIXO
BOOL dxC_IsPixomaticActive();
#endif

BOOL dxC_ShouldUsePixomatic();
PRESULT dxC_PixoInitialize( HWND hWnd );
PRESULT dxC_PixoResizeBuffers( HWND hWnd );
PRESULT dxC_PixoShutdown();

PRESULT dxC_PixoSetStreamDefinition( PIXO_VERTEX_DECL pDecl );
PRESULT dxC_PixoSetTexture( DWORD dwStage, D3D_TEXTURE * pTexture );
PRESULT dxC_PixoSetTexture( TEXTURE_TYPE eType, D3D_TEXTURE * pTexture );
PRESULT dxC_PixoSetIndices( const PIXO_INDEX_DATA pIndexData, DWORD nIndexCount, DWORD nIndexSize );
PRESULT dxC_PixoSetViewport( const D3DC_VIEWPORT * ptVP );
PRESULT dxC_PixoGetBufferDimensions( int & nWidth, int & nHeight );
PRESULT dxC_PixoGetBufferFormat( DXC_FORMAT & tFormat );

PRESULT dxC_PixoSetStream(
	DWORD dwStream,
	const PIXO_VERTEX_DATA pData,
	DWORD dwOffsetInBytes,
	DWORD dwVertexSize,
	DWORD dwVertexCount );

PRESULT dxC_PixoDrawPrimitive(
	D3DC_PRIMITIVE_TOPOLOGY PrimitiveType,
	DWORD PrimitiveCount );

PRESULT dxC_PixoDrawIndexedPrimitive(
	D3DC_PRIMITIVE_TOPOLOGY PrimitiveType,
	DWORD VertexStart,
	DWORD VertexCount,
	DWORD StartIndex,
	DWORD PrimCount );

PRESULT dxC_PixoClearBuffers(
	DWORD dwClearFlags,
	DWORD tColor,
	float fZ,
	unsigned int nStencil );

PRESULT dxC_PixoResetState();
PRESULT dxC_PixoSetWorldMatrix( const MATRIX * pMat );
PRESULT dxC_PixoSetViewMatrix( const MATRIX * pMat );
PRESULT dxC_PixoSetProjectionMatrix( const MATRIX * pMat );
PRESULT dxC_PixoSetRenderState( DWORD dwState, DWORD dwValue );
PRESULT dxC_PixoUpdateDebugStates();
PRESULT dxC_PixoSetAllowRender( BOOL bAllowRender );
PRESULT dxC_PixoSetRenderTarget( RENDER_TARGET_INDEX eRT );
PRESULT dxC_PixoSetDepthTarget( DEPTH_TARGET_INDEX eDT );

PRESULT dxC_PixoBeginScene();
PRESULT dxC_PixoBeginPixelAccurateScene();
PRESULT dxC_PixoEndScene();
PRESULT dxC_PixoPresent();



}; // namespace FSSE

#endif // __DXC_PIXO__
