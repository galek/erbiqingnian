//----------------------------------------------------------------------------
// dx9_ui.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DX9_UI__
#define __DX9_UI__

#include "e_ui.h"
#include "dxC_fvf.h"
#include "dxC_model.h"
#ifdef ENGINE_TARGET_DX9
#include "dx9_device.h"	// CHB 2007.01.01 - For DX9_DEVICE_ALLOW_SWVP
#endif

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

// CHB 2007.01.01 - Software vertex processing doesn't do 32-bit indices
#ifdef DX9_DEVICE_ALLOW_SWVP
typedef WORD UI_INDEX_TYPE;
#	define UI_D3D_INDEX_FMT	D3DFMT_INDEX16
#else
// CML 2007.01.19 - With chunking off, we can use 16-bit indices
typedef WORD UI_INDEX_TYPE;
#	define UI_D3D_INDEX_FMT	D3DFMT_INDEX16
#endif

#define UI_MAX_INDEX_VALUE	( (UI_INDEX_TYPE)0xffffffff )

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

// a position in texture space mapped to a position in screen space
struct UI_SCREENVERTEX
{
	D3DXVECTOR3				vPosition;					// screen space position
	D3DCOLOR				dwColor;					// color
	float					pfTextureCoords0[2];		// position in texture space

	//float					fPadding;
}; // align size at multiple of 32 bytes

struct UI_SIMPLE_SCREEN_VERTEX
{
	D3DXVECTOR3				vPosition;					// screen space position
	D3DCOLOR				dwColor;					// color

	//float					fPadding;
};

// d3d buffers
struct UI_D3DBUFFER
{
	// for the hash
	int						nId;
	UI_D3DBUFFER*			pNext;

	BOOL					m_bDynamic;

	//struct {
	//	SPD3DCVB			m_pD3DVertexBuffer;			// video memory copy of vertex list
	//} VB[2];
	//int						m_nD3DVertexBufferSize;
	D3D_VERTEX_BUFFER_DEFINITION m_tVertexBufferDef;
	int						m_nVertexBufferD3DID[ 1 ];

	int						m_nCurrentUpdateVB;
	int						m_nCurrentRenderVB;

#ifdef UI_D3DBUFFER_KEEP_VERTEX_BUFFER_COPY		// CHB 2006.08.15
	UI_SCREENVERTEX*		m_pLocalVertexBufferPrev;
	int						m_nLocalVertexBufferCountPrev;
	int						m_nLocalVertexBufferAllocatedSize;
#endif

	//SPD3DCIB				m_pD3DIndexBuffer;			// index buffer
	//int						m_nD3DIndexBufferSize;
	D3D_INDEX_BUFFER_DEFINITION	m_tIndexBufferDef;

	UI_INDEX_TYPE*			m_pLocalIndexBufferPrev;
	int						m_nLocalIndexBufferCountPrev;
	int						m_nLocalIndexBufferAllocatedSize;

	VERTEX_DECL_TYPE		m_eVertexDeclType;
};

// local mem buffers
struct UI_LOCALBUFFER
{
	// for the hash
	int						nId;
	UI_LOCALBUFFER*			pNext;

	void*					m_pLocalVertexBuffer;
	SCREEN_VERTEX_TYPE		m_eLocalVertexType;
	int						m_nLocalVertexBufferMaxCount;
	int						m_nLocalVertexBufferCurCount;

	UI_INDEX_TYPE*			m_pLocalIndexBuffer;
	int						m_nLocalIndexBufferMaxCount;
	int						m_nLocalIndexBufferCurCount;
};

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

// CHB 2006.09.07 - This appears to be used only statically.
#if 0
BOOL dx9_UIEngineBufferIsBetter(
	UI_D3DBUFFER * pNewEngineBuffer,
	UI_D3DBUFFER * pOldEngineBuffer,
	UI_LOCALBUFFER * pLocalBuffer );
#endif

PRESULT dx9_UIReleaseVBuffers();
PRESULT dx9_SetUISamplerStates();

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

inline UI_D3DBUFFER * dx9_GetUIEngineBuffer( int nBufferID )
{
	extern CHash<UI_D3DBUFFER> gtUIEngineBuffers;
	return HashGet( gtUIEngineBuffers, nBufferID );
}

inline UI_LOCALBUFFER * dx9_GetUILocalBuffer( int nBufferID )
{
	extern CHash<UI_LOCALBUFFER> gtUILocalBuffers;
	return HashGet( gtUILocalBuffers, nBufferID );
}

#endif // __DX9_UI__
