//----------------------------------------------------------------------------
// dx9_ui.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"

#include "dxC_texture.h"
#include "e_texture_priv.h"
#include "dxC_state.h"
#include "dxC_target.h"
#include "dxC_caps.h"
#include "dxC_profile.h"
#include "e_ui.h"
#include "dx9_ui.h"
#include "pakfile.h"
#include "appcommontimer.h"
#include "dxC_buffer.h"
#include "chb_timing.h"
#include "performance.h"
#include "perfhier.h"
#include "e_automap.h"

using namespace FSSE;

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------
//#define _TRACE_FONT_TEXTURE

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

// references an existing UI_D3DBUFFER on the gtUIEngineBuffers hash
struct UI_D3DBUFFER_REF
{
	int nId;
	UI_D3DBUFFER_REF * pNext;
};

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

CHash<UI_LOCALBUFFER>	gtUILocalBuffers;
int						gnLocalBuffersNextID = 0;
CHash<UI_D3DBUFFER>		gtUIEngineBuffers;
int						gnEngineBuffersNextID = 0;

CHash<UI_D3DBUFFER_REF>	gtEngineBuffersGarbage;

float					gfGlobalUIZ = 0.003f;

DWORD					gdwFontRowAge = 0;

#ifdef _TRACE_FONT_TEXTURE
int	nFontRowSwaps = 0;
#endif

static size_t			sgnSizeOfVertex[ NUM_SCREEN_VERTEX_TYPES ] = 
{
	sizeof(UI_SCREENVERTEX),
	sizeof(UI_SIMPLE_SCREEN_VERTEX)
};

static VERTEX_DECL_TYPE	sgtUIVertexDeclType[ NUM_SCREEN_VERTEX_TYPES ] =
{
	VERTEX_DECL_XYZ_COL_UV,
	VERTEX_DECL_XYZ_COL
};

static EFFECT_TECHNIQUE_CACHE sgtUITechniqueCache;

static BOOL sgbUIFontResetNeeded = FALSE;

void e_UIRequestFontTextureReset(BOOL bRequest)
{
	sgbUIFontResetNeeded = bRequest;
}
BOOL e_UIGetFontTextureResetRequest()
{
	return sgbUIFontResetNeeded;
}
//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

extern SIMPLE_DYNAMIC_ARRAY<UI_DEBUG_LABEL> gtUIDebugLabels;

//----------------------------------------------------------------------------
// IMPLEMENTATIONS
//----------------------------------------------------------------------------

static PRESULT sUISetStates();
static PRESULT sUISetSamplerStates();
static PRESULT sUIEngineBufferFree( int nEngineBufferID );

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static UI_SCREENVERTEX * sGetLocalBufferScreenVertexBuffer( UI_LOCALBUFFER * pBuffer, int nOffset )
{
	ASSERT_RETNULL( pBuffer );
	ASSERT_RETNULL( pBuffer->m_eLocalVertexType == SCREEN_VERTEX_XYZUV );
	return ( (UI_SCREENVERTEX*)pBuffer->m_pLocalVertexBuffer ) + nOffset;
}

static UI_SIMPLE_SCREEN_VERTEX * sGetLocalBufferSimpleScreenVertexBuffer( UI_LOCALBUFFER * pBuffer, int nOffset )
{
	ASSERT_RETNULL( pBuffer );
	ASSERT_RETNULL( pBuffer->m_eLocalVertexType == SCREEN_VERTEX_XYZ );
	return ( (UI_SIMPLE_SCREEN_VERTEX*)pBuffer->m_pLocalVertexBuffer ) + nOffset;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL e_UIEngineBufferIsDynamic( int nBufferID )
{
	UI_D3DBUFFER * pBuffer = dx9_GetUIEngineBuffer( nBufferID );
	ASSERT_RETFALSE( pBuffer );
	return pBuffer->m_bDynamic;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
BOOL dx9_UIEngineBufferIsBetter( UI_D3DBUFFER * pNewEngineBuffer, UI_D3DBUFFER * pOldEngineBuffer, UI_LOCALBUFFER * pLocalBuffer )
{
	ASSERT_RETFALSE( pNewEngineBuffer->m_tVertexBufferDef.dwFVF == pOldEngineBuffer->m_tVertexBufferDef.dwFVF 
					 || pNewEngineBuffer->m_tVertexBufferDef.dwFVF == D3DC_FVF_NULL
					 || pOldEngineBuffer->m_tVertexBufferDef.dwFVF == D3DC_FVF_NULL );
	ASSERT_RETFALSE( pNewEngineBuffer->m_tVertexBufferDef.eVertexType == pOldEngineBuffer->m_tVertexBufferDef.eVertexType
					 || pNewEngineBuffer->m_tVertexBufferDef.eVertexType == VERTEX_DECL_INVALID
					 || pOldEngineBuffer->m_tVertexBufferDef.eVertexType == VERTEX_DECL_INVALID );

	if ( pNewEngineBuffer->m_tVertexBufferDef.nVertexCount >= pLocalBuffer->m_nLocalVertexBufferCurCount )
	{
		if ( pOldEngineBuffer->m_tVertexBufferDef.nVertexCount < pLocalBuffer->m_nLocalVertexBufferCurCount )
		{
			return TRUE;
		}
		else if ( pNewEngineBuffer->m_tVertexBufferDef.nVertexCount < pOldEngineBuffer->m_tVertexBufferDef.nVertexCount )
		{
			return TRUE;
		}
	}
	else if ( pNewEngineBuffer->m_tVertexBufferDef.nVertexCount > pOldEngineBuffer->m_tVertexBufferDef.nVertexCount )
	{
		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

int e_UIGetLocalBufferVertexCurCount( int nLocalBufferID )
{

	UI_LOCALBUFFER * local = dx9_GetUILocalBuffer( nLocalBufferID );
	ASSERT_RETZERO( local );
	return local->m_nLocalVertexBufferCurCount;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

int e_UIGetLocalBufferVertexMaxCount( int nLocalBufferID )
{

	UI_LOCALBUFFER * local = dx9_GetUILocalBuffer( nLocalBufferID );
	ASSERT_RETZERO( local );
	return local->m_nLocalVertexBufferMaxCount;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_UIGetNewEngineBuffer(
	int nLocalBufferID,
	int& nEngineBufferID,
	BOOL bDynamic )
{
	// whatever I end up doing here, I still need to try to reuse old buffers with bDynamic set

	ASSERT( nEngineBufferID == INVALID_ID );
	ASSERT( ! HashGet( gtUIEngineBuffers, nEngineBufferID ) );
	ASSERT( ! HashGet( gtEngineBuffersGarbage, nEngineBufferID ) );

	nEngineBufferID = INVALID_ID;

	UI_LOCALBUFFER * local = dx9_GetUILocalBuffer( nLocalBufferID );
	ASSERT_RETINVALIDARG( local );

	UI_D3DBUFFER * best = NULL;
	UI_D3DBUFFER_REF * curRef = HashGetFirst( gtEngineBuffersGarbage );
	UI_D3DBUFFER_REF * nextRef = NULL;
	while (curRef)
	{
		nextRef = HashGetNext( gtEngineBuffersGarbage, curRef );
		UI_D3DBUFFER * cur = HashGet( gtUIEngineBuffers, curRef->nId );

		ASSERT( cur );

		if ( !cur )
		{
			HashRemove( gtEngineBuffersGarbage, curRef->nId );
			curRef = nextRef;
			continue;
		}

		//ASSERT( cur->m_tVertexBufferDef.nD3DBufferID[0] == INVALID_ID );
		//ASSERT( cur->m_tIndexBufferDef.nD3DBufferID == INVALID_ID );
		//for ( int i = 0; i < _ARRAYSIZE(best->m_nVertexBufferD3DID); i++ )
		//{
		//	ASSERT( cur->m_nVertexBufferD3DID[ i ] == INVALID_ID );
		//}

		if (bDynamic == cur->m_bDynamic)
		{
			if (!best)
			{
				best = cur;
			}
			else if ( dx9_UIEngineBufferIsBetter( cur, best, local ) )
			{
				best = cur;
			}
		}

		curRef = nextRef;
	}
	if (best)
	{
		HashRemove( gtEngineBuffersGarbage, best->nId );
	}
	else
	{
		HashAdd( gtUIEngineBuffers, gnEngineBuffersNextID );
		gnEngineBuffersNextID++;
		best = HashGet( gtUIEngineBuffers, gnEngineBuffersNextID - 1 );
		ASSERT_RETINVALID(best);

		dxC_VertexBufferDefinitionInit( best->m_tVertexBufferDef );
		dxC_IndexBufferDefinitionInit( best->m_tIndexBufferDef );
		for ( int i = 0; i < _ARRAYSIZE(best->m_nVertexBufferD3DID); i++ )
		{
			best->m_nVertexBufferD3DID[ i ] = INVALID_ID;
		}
	}
	nEngineBufferID = best->nId;
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// UI_**DRAW: copy local index buffer to d3d index buffer
//----------------------------------------------------------------------------
static PRESULT sUICopyIndexBuffer(
	UI_LOCALBUFFER * pLocalBuffer,
	UI_D3DBUFFER * pEngineBuffer,
	BOOL bForce = FALSE)
{
	if ( !pEngineBuffer || !pLocalBuffer )
	{
		return E_INVALIDARG;
	}
	if ( pEngineBuffer->m_tIndexBufferDef.nD3DBufferID == INVALID_ID )
	{
		return S_FALSE;
	}
	ASSERT_RETFAIL(pEngineBuffer->m_tIndexBufferDef.nIndexCount >= pLocalBuffer->m_nLocalIndexBufferCurCount);

	int buffersize = pLocalBuffer->m_nLocalIndexBufferCurCount * sizeof(UI_INDEX_TYPE);

	// see if we need to send anything to the card
	if ( pLocalBuffer->m_nLocalIndexBufferCurCount == pEngineBuffer->m_nLocalIndexBufferCountPrev &&
		 pEngineBuffer->m_pLocalIndexBufferPrev &&
		 !bForce &&
		 memcmp( pLocalBuffer->m_pLocalIndexBuffer, pEngineBuffer->m_pLocalIndexBufferPrev, buffersize) == 0 )
		return S_FALSE;

	if ( pEngineBuffer->m_nLocalIndexBufferAllocatedSize < buffersize || 
		! pEngineBuffer->m_pLocalIndexBufferPrev )
	{
		pEngineBuffer->m_pLocalIndexBufferPrev = (UI_INDEX_TYPE *) REALLOC( NULL, pEngineBuffer->m_pLocalIndexBufferPrev, buffersize );
		pEngineBuffer->m_nLocalIndexBufferAllocatedSize = buffersize;
	}
	memcpy( pEngineBuffer->m_pLocalIndexBufferPrev, pLocalBuffer->m_pLocalIndexBuffer, buffersize );
	pEngineBuffer->m_nLocalIndexBufferCountPrev = pLocalBuffer->m_nLocalIndexBufferCurCount;

//#ifdef DX9_MOST_BUFFERS_MANAGED
//	V_RETURN( dxC_UpdateBuffer( pEngineBuffer->m_tIndexBufferDef, pLocalBuffer->m_pLocalIndexBuffer, 0, buffersize ) );
//#else
	V_RETURN( dxC_UpdateBuffer( pEngineBuffer->m_tIndexBufferDef, pLocalBuffer->m_pLocalIndexBuffer, 0, buffersize, pEngineBuffer->m_bDynamic ) );
//#endif
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
LPD3DCVB sUIEngineBufferGetRenderVertexBuffer(UI_D3DBUFFER * pBuf)
{
	D3D_VERTEX_BUFFER * pVB = dxC_VertexBufferGet( pBuf->m_nVertexBufferD3DID[ pBuf->m_nCurrentRenderVB ] );
	ASSERT_RETNULL( pVB && pVB->pD3DVertices );
	return pVB->pD3DVertices;
}

static
D3D_VERTEX_BUFFER* sUIEngineBufferGetUpdateVertexBuffer(UI_D3DBUFFER * pBuf)
{
	D3D_VERTEX_BUFFER * pVB = dxC_VertexBufferGet( pBuf->m_nVertexBufferD3DID[ pBuf->m_nCurrentUpdateVB ] );
	ASSERT_RETFALSE( pVB && pVB->pD3DVertices );
	return pVB;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// UI_**DRAW: copy local vertex buffer to d3d vertex buffer
//----------------------------------------------------------------------------
static PRESULT sUICopyVertexBuffer(
	UI_LOCALBUFFER * pLocalBuffer,
	UI_D3DBUFFER * pEngineBuffer,
	BOOL bForce = FALSE)
{
	if ( !pEngineBuffer || !pLocalBuffer )
	{
		return E_INVALIDARG;
	}
	if ( pEngineBuffer->m_nVertexBufferD3DID[ pEngineBuffer->m_nCurrentUpdateVB ] == INVALID_ID )
	{
		return S_FALSE;
	}
	ASSERT_RETFAIL(pEngineBuffer->m_tVertexBufferDef.nVertexCount >= pLocalBuffer->m_nLocalVertexBufferCurCount);

	int buffersize = SIZE_TO_INT(pLocalBuffer->m_nLocalVertexBufferCurCount * sgnSizeOfVertex[ pLocalBuffer->m_eLocalVertexType ]);
	if (buffersize <= 0)
	{
		return S_FALSE;
	}
	ASSERT_RETFAIL( pEngineBuffer->m_tVertexBufferDef.nBufferSize[ 0 ] >= buffersize );

	// see if we need to send anything to the card
#ifdef UI_D3DBUFFER_KEEP_VERTEX_BUFFER_COPY		// CHB 2006.08.15
	if ( pLocalBuffer->m_nLocalVertexBufferCurCount == pEngineBuffer->m_nLocalVertexBufferCountPrev &&
		 pEngineBuffer->m_pLocalVertexBufferPrev &&
		 !bForce &&
		 memcmp( pLocalBuffer->m_pLocalVertexBuffer, pEngineBuffer->m_pLocalVertexBufferPrev, buffersize) == 0 )
		return S_FALSE;

	if ( pEngineBuffer->m_nLocalVertexBufferAllocatedSize < buffersize || 
		! pEngineBuffer->m_pLocalVertexBufferPrev )
	{
		pEngineBuffer->m_pLocalVertexBufferPrev = (UI_SCREENVERTEX *) REALLOC( NULL, pEngineBuffer->m_pLocalVertexBufferPrev, buffersize );
		pEngineBuffer->m_nLocalVertexBufferAllocatedSize = buffersize;
	}
	memcpy( pEngineBuffer->m_pLocalVertexBufferPrev, pLocalBuffer->m_pLocalVertexBuffer, buffersize );
	pEngineBuffer->m_nLocalVertexBufferCountPrev = pLocalBuffer->m_nLocalVertexBufferCurCount;
#endif

	CHB_StatsCount(CHB_STATS_UI_VERTEX_BUFFER_BYTES_UPLOADED, buffersize);

	D3D_VERTEX_BUFFER * pVB = sUIEngineBufferGetUpdateVertexBuffer( pEngineBuffer );
	if ( !pVB )
		return E_FAIL;

	V_DO_SUCCEEDED( dxC_UpdateBufferEx(
			pVB->pD3DVertices,
			pLocalBuffer->m_pLocalVertexBuffer,
			0,
			buffersize,
			pVB->tUsage,
			pEngineBuffer->m_bDynamic ) )
	{
		pEngineBuffer->m_nCurrentRenderVB = pEngineBuffer->m_nCurrentUpdateVB;
		pEngineBuffer->m_nCurrentUpdateVB = (pEngineBuffer->m_nCurrentUpdateVB + 1) % _ARRAYSIZE(pEngineBuffer->m_nVertexBufferD3DID);
		return S_OK;
	}

	return E_FAIL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sUIDrawGetEngineBuffer(
	int nLocalBuffer,
	int & nEngineBuffer,
	BOOL bDynamic )
{
	ASSERT_RETINVALID( nLocalBuffer != INVALID_ID );
	if (nEngineBuffer != INVALID_ID )
	{
		UI_D3DBUFFER * pBuf = HashGet( gtUIEngineBuffers, nEngineBuffer );
		if ( pBuf )
		{
			if ( dxC_VertexBufferD3DExists( pBuf->m_nVertexBufferD3DID[ pBuf->m_nCurrentUpdateVB ] ) )
				return S_FALSE;
			else
			{
				V( sUIEngineBufferFree( nEngineBuffer ) );
			}
		}
	}

	nEngineBuffer = INVALID_ID;
	V_RETURN( e_UIGetNewEngineBuffer( nLocalBuffer, nEngineBuffer, bDynamic ) );

	return S_OK;
}

//----------------------------------------------------------------------------
// UI_TEXTUREDRAW: restore d3d buffers and copy local data over
//----------------------------------------------------------------------------
PRESULT e_UICopyToEngineBuffer(
	int nLocalBufferID,
	int & nEngineBufferID,
	BOOL bDynamic,
	BOOL bForce /*= FALSE*/ )
{
	PERF(UI_COPY_TO_ENGINE);

	V_RETURN( sUIDrawGetEngineBuffer( nLocalBufferID, nEngineBufferID, bDynamic ) );

	UI_D3DBUFFER * pEngineBuffer = dx9_GetUIEngineBuffer( nEngineBufferID );
	UI_LOCALBUFFER * pLocalBuffer = dx9_GetUILocalBuffer( nLocalBufferID );

	V_RETURN( e_UIEngineBufferRestoreResize( nEngineBufferID, nLocalBufferID, bDynamic, bForce ) );
	V_RETURN( sUICopyVertexBuffer( pLocalBuffer, pEngineBuffer, bForce ) );
	V_RETURN( sUICopyIndexBuffer( pLocalBuffer, pEngineBuffer, bForce ) );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
PRESULT sUIEngineBufferFreeVertexBuffers(UI_D3DBUFFER * pBuf)
{
	for (unsigned i = 0; i < _ARRAYSIZE(pBuf->m_nVertexBufferD3DID); ++i)
	{
#ifdef ENGINE_TARGET_DX9
		gD3DStatus.dwVidMemTotal -= pBuf->m_tVertexBufferDef.nBufferSize[ 0 ];
		gD3DStatus.dwVidMemVertexBuffer -= pBuf->m_tVertexBufferDef.nBufferSize[ 0 ];
#endif

		V( dxC_VertexBufferReleaseResource( pBuf->m_nVertexBufferD3DID[ i ] ) );
		pBuf->m_nVertexBufferD3DID[ i ] = INVALID_ID;
	}
	pBuf->m_tVertexBufferDef.nBufferSize[ 0 ] = 0;

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PRESULT e_UIEngineBufferRestoreResize(
	int nEngineBufferID,
	int nLocalBufferID,
	BOOL bDynamic,
	BOOL bForce /*= FALSE*/ )
{
#if defined(ENGINE_TARGET_DX9)
	if (dx9_DeviceLost() == TRUE)
	{
		return S_FALSE;
	}
#endif

	UI_D3DBUFFER * engine = dx9_GetUIEngineBuffer( nEngineBufferID );
	UI_LOCALBUFFER * local = dx9_GetUILocalBuffer( nLocalBufferID );
	if (!engine || !local)
	{
		return E_FAIL;
	}

	if (!bForce)
	{
		if (engine->m_tVertexBufferDef.nVertexCount >= local->m_nLocalVertexBufferCurCount &&
			engine->m_tIndexBufferDef.nIndexCount >= local->m_nLocalIndexBufferCurCount)
		{
			return S_FALSE;
		}
	}

	{
		V( sUIEngineBufferFreeVertexBuffers(engine) );
		engine->m_tVertexBufferDef.nVertexCount = 0;

		ASSERT_RETFAIL(local->m_nLocalVertexBufferMaxCount >= local->m_nLocalVertexBufferCurCount);

		int buffersize = SIZE_TO_INT(local->m_nLocalVertexBufferMaxCount * sgnSizeOfVertex[ local->m_eLocalVertexType ]);
		//DWORD dwUsageFlags = bDynamic ? D3DUSAGE_DYNAMIC : 0;
		//dwUsageFlags |= D3DUSAGE_WRITEONLY;  // performance speedup for POOL_DEFAULT if we aren't writing to it
		VERTEX_DECL_TYPE eVertexDeclType = sgtUIVertexDeclType[ local->m_eLocalVertexType ];

		dxC_VertexBufferDefinitionInit( engine->m_tVertexBufferDef );
		engine->m_tVertexBufferDef.eVertexType = eVertexDeclType;
		engine->m_tVertexBufferDef.nBufferSize[ 0 ] = buffersize;
		//engine->m_tVertexBufferDef.nStructSize = sgnSizeOfVertex[ local->m_eLocalVertexType ];
		engine->m_tVertexBufferDef.nVertexCount = local->m_nLocalVertexBufferMaxCount;
//#ifdef DX9_MOST_BUFFERS_MANAGED
		engine->m_tVertexBufferDef.tUsage = bDynamic ? D3DC_USAGE_BUFFER_MUTABLE : D3DC_USAGE_BUFFER_REGULAR;
//#else
//		engine->m_tVertexBufferDef.tUsage = bDynamic ? D3DC_USAGE_BUFFER_MUTABLE : D3DC_USAGE_BUFFER_CONSTANT;
//#endif
		for (unsigned i = 0; i < _ARRAYSIZE(engine->m_nVertexBufferD3DID); ++i)
		{
			V_DO_FAILED( dxC_CreateVertexBuffer( 0, engine->m_tVertexBufferDef ) )
			{
				V( sUIEngineBufferFreeVertexBuffers(engine) );
				engine->m_bDynamic = FALSE;
				engine->m_tVertexBufferDef.nBufferSize[ 0 ] = 0;
				return E_FAIL;
			}
			// save this d3d_vertex_buffer id
			engine->m_nVertexBufferD3DID[ i ] = engine->m_tVertexBufferDef.nD3DBufferID[ 0 ];
			engine->m_tVertexBufferDef.nD3DBufferID[ 0 ] = INVALID_ID;
		}

		engine->m_bDynamic = bDynamic;
		engine->m_eVertexDeclType = eVertexDeclType;
		for (unsigned i = 0; i < _ARRAYSIZE(engine->m_nVertexBufferD3DID); ++i)
		{
			if ( bDynamic && dxC_VertexBufferD3DExists( engine->m_nVertexBufferD3DID[ i ] ) )
			{
				gD3DStatus.dwVidMemTotal += engine->m_tVertexBufferDef.nBufferSize[ 0 ];
				gD3DStatus.dwVidMemVertexBuffer += engine->m_tVertexBufferDef.nBufferSize[ 0 ];
			}
		}
	}

	{
		if ( dxC_IndexBufferD3DExists( engine->m_tIndexBufferDef.nD3DBufferID ) )
		{
			gD3DStatus.dwVidMemTotal -= engine->m_tIndexBufferDef.nBufferSize;
		}
		V( dxC_IndexBufferReleaseResource( engine->m_tIndexBufferDef.nD3DBufferID ) );

		ASSERT_RETFAIL(local->m_nLocalIndexBufferMaxCount >= local->m_nLocalIndexBufferCurCount);

		int buffersize = local->m_nLocalIndexBufferMaxCount * sizeof(UI_INDEX_TYPE);

		dxC_IndexBufferDefinitionInit( engine->m_tIndexBufferDef );

		engine->m_tIndexBufferDef.tFormat = UI_D3D_INDEX_FMT;
		engine->m_tIndexBufferDef.nBufferSize = buffersize;
		engine->m_tIndexBufferDef.nIndexCount = local->m_nLocalIndexBufferMaxCount;

//#ifdef DX9_MOST_BUFFERS_MANAGED
		engine->m_tIndexBufferDef.tUsage = bDynamic ? D3DC_USAGE_BUFFER_MUTABLE : D3DC_USAGE_BUFFER_REGULAR;
//#else
//		engine->m_tIndexBufferDef.tUsage = bDynamic ? D3DC_USAGE_BUFFER_MUTABLE : D3DC_USAGE_BUFFER_CONSTANT;
//#endif
		V_DO_FAILED( dxC_CreateIndexBuffer( engine->m_tIndexBufferDef ) )
		{
			engine->m_bDynamic = FALSE;
			V( sUIEngineBufferFreeVertexBuffers(engine) );
			dxC_VertexBufferDefinitionInit( engine->m_tVertexBufferDef );
			dxC_IndexBufferDefinitionInit( engine->m_tIndexBufferDef );
			engine->m_tVertexBufferDef.nBufferSize[ 0 ] = 0;
			engine->m_eVertexDeclType = VERTEX_DECL_INVALID;
			return E_FAIL;
		}
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define VERTEX_BUFFER_REDUCTION_DELTA 100
#define INDEX_BUFFER_REDUCTION_DELTA 100
//----------------------------------------------------------------------------
// UI_TEXTUREDRAW: realloc local vertex & index buffers for new size
//----------------------------------------------------------------------------
static PRESULT sResizeLocalBuffers(
	UI_LOCALBUFFER * pBuf,
	SCREEN_VERTEX_TYPE eType,
	int nVertexCount,
	int nIndexCount )
{
	// TRAVIS: Now I'm sizing down buffers if they pass below a certain threshhold.
	// In combination with the changes to UITextureToDrawGet where we seek out matching or
	// close-but-larger buffers to start with, reallocs should be less than before anyway,
	// so this shouldn't really increase the number of reallocs overall - but it will make memory usage tighter
	// and let us recover from a glut of high-vert-count buffers
	// Chris recommends reallocing based on pow2, but I haven't done that yet.
	if (pBuf->m_nLocalVertexBufferMaxCount < nVertexCount || pBuf->m_nLocalVertexBufferMaxCount > nVertexCount + VERTEX_BUFFER_REDUCTION_DELTA )
	{
		int nNewCount = MIN(nVertexCount, (int)UI_MAX_INDEX_VALUE);		// this is the highest number an index can be, so this is the most vertices we can support
		if (pBuf->m_nLocalVertexBufferMaxCount < nNewCount || pBuf->m_nLocalVertexBufferMaxCount > nNewCount + VERTEX_BUFFER_REDUCTION_DELTA )
		{
			pBuf->m_eLocalVertexType = eType;
			pBuf->m_pLocalVertexBuffer = REALLOC(NULL, pBuf->m_pLocalVertexBuffer, SIZE_TO_INT(sgnSizeOfVertex[ eType ] * nNewCount));
			if ( ! pBuf->m_pLocalVertexBuffer )
			{
				pBuf->m_nLocalVertexBufferMaxCount = 0;
				return E_OUTOFMEMORY;
			}
			pBuf->m_nLocalVertexBufferMaxCount = nNewCount;
		}
	}
	if (pBuf->m_nLocalIndexBufferMaxCount < nIndexCount  || pBuf->m_nLocalVertexBufferMaxCount > nIndexCount + INDEX_BUFFER_REDUCTION_DELTA )
	{
		pBuf->m_pLocalIndexBuffer = (UI_INDEX_TYPE*)REALLOC(NULL, pBuf->m_pLocalIndexBuffer, sizeof(UI_INDEX_TYPE) * nIndexCount);
		if ( ! pBuf->m_pLocalIndexBuffer )
		{
			pBuf->m_nLocalIndexBufferMaxCount = 0;
			return E_OUTOFMEMORY;
		}
		pBuf->m_nLocalIndexBufferMaxCount = nIndexCount;
	}

	return S_OK;
}

PRESULT e_UITextureResizeLocalBuffers(
	int nLocalBufferID,
	int nVertexCount,
	int nIndexCount)
{
	UI_LOCALBUFFER * pBuf = dx9_GetUILocalBuffer( nLocalBufferID );
	ASSERT_RETINVALIDARG( pBuf );
	return sResizeLocalBuffers( pBuf, SCREEN_VERTEX_XYZUV, nVertexCount, nIndexCount );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_UIPrimitiveResizeLocalBuffers(
	int nLocalBufferID,
	int nVertexCount,
	int nIndexCount )
{
	UI_LOCALBUFFER * pBuf = dx9_GetUILocalBuffer( nLocalBufferID );
	ASSERT_RETINVALIDARG( pBuf );
	return sResizeLocalBuffers( pBuf, SCREEN_VERTEX_XYZ, nVertexCount, nIndexCount );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sUIEngineBufferFree(
	int nEngineBufferID )
{
	UI_D3DBUFFER * buf = dx9_GetUIEngineBuffer( nEngineBufferID );
	ASSERT_RETINVALIDARG(buf);
	V( sUIEngineBufferFreeVertexBuffers(buf) );
	V( dxC_IndexBufferReleaseResource( buf->m_tIndexBufferDef.nD3DBufferID ) );
	buf->m_tIndexBufferDef.nD3DBufferID = INVALID_ID;
#ifdef UI_D3DBUFFER_KEEP_VERTEX_BUFFER_COPY		// CHB 2006.08.15
	if ( buf->m_pLocalVertexBufferPrev )
	{
		FREE( NULL, buf->m_pLocalVertexBufferPrev );
		buf->m_pLocalVertexBufferPrev = NULL;
	}
#endif
	if ( buf->m_pLocalIndexBufferPrev )
	{
		FREE( NULL, buf->m_pLocalIndexBufferPrev );
		buf->m_pLocalIndexBufferPrev = NULL;
	}
	//FREE(NULL, buf);

	if ( HashGet( gtEngineBuffersGarbage, buf->nId ) )
		HashRemove( gtEngineBuffersGarbage, buf->nId );

	HashRemove( gtUIEngineBuffers, buf->nId );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sUILocalBufferFree(
	int nLocalBufferID )
{
	UI_LOCALBUFFER * buf = dx9_GetUILocalBuffer( nLocalBufferID );
	ASSERT_RETINVALIDARG(buf);
	if (buf->m_pLocalVertexBuffer)
	{
		FREE(NULL, buf->m_pLocalVertexBuffer);
	}
	if (buf->m_pLocalIndexBuffer)
	{
		FREE(NULL, buf->m_pLocalIndexBuffer);
	}

	//FREE(NULL, buf);
	HashRemove( gtUILocalBuffers, buf->nId );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// UI_TEXTUREDRAW: free the local & d3d buffers
//----------------------------------------------------------------------------
PRESULT e_UITextureDrawBuffersFree(
	int nEngineBufferID,
	int nLocalBufferID )
{
	if (nEngineBufferID != INVALID_ID)
	{
		V( sUIEngineBufferFree(nEngineBufferID) );
	}
	if (nLocalBufferID != INVALID_ID)
	{
		V( sUILocalBufferFree(nLocalBufferID) );
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_UIPrimitiveDrawBuffersFree(
	int nEngineBufferID,
	int nLocalBufferID )
{
	if (nEngineBufferID != INVALID_ID)
	{
		V( sUIEngineBufferFree(nEngineBufferID) );
	}
	if (nLocalBufferID != INVALID_ID)
	{
		V( sUILocalBufferFree(nLocalBufferID) );
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sGetBestFitPow2Res(
	UINT nNumChunks,
	UINT nChunkWidth,
	UINT nChunkHeight,
	UINT& nHorizChunks,
	UINT& nVertChunks,
	UINT& nHorizPixels,
	UINT& nVertPixels)
{
#define MAX_DIMENSION	2048

	UINT nVertPixelsNeeded = 0;
	UINT nBestGap = (UINT)-1;
	UINT  iBest = -1;

	for (UINT i=0; i < 16 && UINT(2 << (i+1)) <= nNumChunks * nChunkWidth; i++)
	{
		if (UINT(2 << i) >= nChunkWidth &&
			UINT(2 << i) <= MAX_DIMENSION)
		{
			nHorizPixels = 2 << i;
			nHorizChunks = (nHorizPixels / nChunkWidth) + (nHorizPixels % nChunkWidth != 0);
			nVertChunks = (nNumChunks / nHorizChunks) + (nNumChunks % nHorizChunks != 0);
			nVertPixelsNeeded = nVertChunks * nChunkHeight;
			nVertPixels = e_GetPow2Resolution(nVertPixelsNeeded);
			if (nVertPixels <= MAX_DIMENSION &&
				nVertPixels - nVertPixelsNeeded < nBestGap)
			{
				iBest = i;
				nBestGap = nVertPixels - nVertPixelsNeeded;
			}
		}
	}

	ASSERT_RETURN(iBest != -1);
	nHorizPixels = 2 << iBest;
	nHorizChunks = (nHorizPixels / nChunkWidth) + (nHorizPixels % nChunkWidth != 0);
	nVertChunks = (nNumChunks / nHorizChunks) + (nNumChunks % nHorizChunks != 0);
	nVertPixelsNeeded = nVertChunks * nChunkHeight;
	nVertPixels = e_GetPow2Resolution(nVertPixelsNeeded);
}

static inline DWORD sGetPixel( 
	UINT nChunk, 
	UINT nChunkX, 
	UINT nChunkY, 
	D3D_TEXTURE* pTexture)
{
	UINT nChunksPerLine = pTexture->nWidthInPixels / pTexture->pChunkData->nChunkWidth;
	UINT nBmpX = nChunkX + ((nChunk % nChunksPerLine) * pTexture->pChunkData->nChunkWidth);
	UINT nBmpY = nChunkY + ((nChunk / nChunksPerLine) * pTexture->pChunkData->nChunkHeight);

	DWORD *pdwData = (DWORD *)pTexture->pbLocalTextureData;
	return pdwData[(nBmpY * pTexture->nWidthInPixels) + nBmpX];
}

static inline DWORD sGetPixel( 
	UINT x, 
	UINT y, 
	D3D_TEXTURE* pTexture)
{
	DWORD *pdwData = (DWORD *)pTexture->pbLocalTextureData;
	return pdwData[(y * pTexture->nWidthInPixels) + x];
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sChunkCompare(
	UINT chunk1,
	UINT chunk2,
	D3D_TEXTURE * pTexture)
{
	UINT texturewidth = pTexture->nWidthInPixels;
	UINT width = pTexture->pChunkData->nChunkWidth;
	UINT height = pTexture->pChunkData->nChunkHeight;
	UINT chunksPerLine = texturewidth / width;
	UINT cmpwidth = width * sizeof(DWORD);
	UINT span = texturewidth - width;		// number of pixels to advance from end of a line to the beginning of the next line
	UINT xoffs1 = (chunk1 % chunksPerLine) * width;
	UINT yoffs1 = (chunk1 / chunksPerLine) * height;
	UINT xoffs2 = (chunk2 % chunksPerLine) * width;
	UINT yoffs2 = (chunk2 / chunksPerLine) * height;

	DWORD * cur1 = (DWORD *)pTexture->pbLocalTextureData + yoffs1 * texturewidth + xoffs1;
	DWORD * cur2 = (DWORD *)pTexture->pbLocalTextureData + yoffs2 * texturewidth + xoffs2;
	DWORD * end = cur1 + height * texturewidth;
	while (cur1 < end)
	{
		if (memcmp(cur1, cur2, cmpwidth) != 0)
		{
			return FALSE;
		}
		cur1 += texturewidth;
		cur2 += texturewidth;
	}
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sChunkComputeCRC(
	UINT chunk,
	D3D_TEXTURE * pTexture)
{
	UINT texturewidth = pTexture->nWidthInPixels;
	UINT width = pTexture->pChunkData->nChunkWidth;
	UINT height = pTexture->pChunkData->nChunkHeight;
	UINT chunksPerLine = texturewidth / width;
	UINT bytewidth = width * sizeof(DWORD);
	UINT xoffs = (chunk % chunksPerLine) * width;
	UINT yoffs = (chunk / chunksPerLine) * height;

	DWORD * cur = (DWORD *)pTexture->pbLocalTextureData + yoffs * texturewidth + xoffs;
	DWORD * end = cur + height * texturewidth;
	DWORD crc = 0;
	while (cur < end)
	{
		crc += CRC(crc, (BYTE *)cur, bytewidth);
		cur += texturewidth;
	}
	return crc;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sChunkGet(UINT x, UINT y, UINT nChunkWidth, UINT nChunkHeight, UINT nBMPWidth,
				 UINT &nChunk, UINT &nChunkX, UINT &nChunkY )
{
	UINT nChunksPerRow = nBMPWidth / nChunkWidth;
	nChunk = ((y / nChunkHeight) * nChunksPerRow) + (x / nChunkWidth);
	nChunkX = x % nChunkWidth;
	nChunkY = y % nChunkHeight;
}

static int sLoadChunkedTexture( int nTextureId, UINT nChunkWidth, UINT nChunkHeight, const CHAR * szBaseFilename)
{
	// CML 2007.1.19
	ASSERTX_RETINVALID( 0, "Chunking is disabled! Don't try to use it!" );


	//ASSERTX_RETVAL(szBaseFilename, INVALID_ID, "No base filename!");

	//CHAR szRegularFilename[MAX_PATH];
	//PStrPrintf(szRegularFilename, MAX_PATH, "%s.png", szBaseFilename);
	//CHAR szChunkedFilename[MAX_PATH];
	//PStrPrintf(szChunkedFilename, MAX_PATH, "%s_chunked.png", szBaseFilename);
	//CHAR szChunkDataFilename[MAX_PATH];
	//PStrPrintf(szChunkDataFilename, MAX_PATH, "%s_chunkdata.dat", szBaseFilename);

	//if (!FileExists(szChunkDataFilename))
	//{
	//	return INVALID_ID;
	//}

	//if (FileExists(szChunkedFilename))
	//{
	//	UINT64 nRegTime   = FileGetLastModifiedTime(szRegularFilename);
	//	UINT64 nChunkTime = FileGetLastModifiedTime(szChunkedFilename);
	//	if (nRegTime > nChunkTime)
	//	{
	//		return INVALID_ID;
	//	}
	//}
	//else
	//{
	//	return INVALID_ID;
	//}

	//HANDLE hFile = FileOpen(szChunkDataFilename, GENERIC_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY | FILE_FLAG_SEQUENTIAL_SCAN);
	//if (hFile == INVALID_FILE)
	//{
	//	return INVALID_ID;
	//}

	//if (FileGetSize(hFile) < sizeof(TEXTURE_CHUNK_DATA)  - sizeof(UINT *))
	//{
	//	FileClose(hFile);
	//	return INVALID_ID;
	//}

	//TEXTURE_CHUNK_DATA *pChunkData = (TEXTURE_CHUNK_DATA *)MALLOCZ(NULL, sizeof(TEXTURE_CHUNK_DATA));
	//FileRead(hFile, (void *)pChunkData, sizeof(TEXTURE_CHUNK_DATA)  - sizeof(UINT *));
	//if (pChunkData->nChunkDataVersion != CHUNK_DATA_VERSION || 
	//	pChunkData->nChunkHeight != nChunkHeight ||
	//	pChunkData->nChunkWidth != nChunkWidth)
	//{
	//	FREE(NULL, pChunkData);
	//	FileClose(hFile);
	//	return INVALID_ID;
	//}

	//pChunkData->pnChunkMap = (UINT *)MALLOC(NULL, sizeof(UINT) * pChunkData->nMapSize);
	//FileRead(hFile, (void *)pChunkData->pnChunkMap, sizeof(UINT) * pChunkData->nMapSize);
	//FileClose(hFile);

	//// Call back to the normal texture load function, but be sure to pass a zero chunk size
	////   so it'll only load the texture we pass and nothing else
	//V( e_UITextureLoadTextureFile( nTextureId, szChunkedFilename, 0, nTextureId ) );
	//if (nTextureId == INVALID_ID)
	//{
	//	return INVALID_ID;
	//}

	//D3D_TEXTURE* d3dtex = dxC_TextureGet(nTextureId);
	//ASSERT_RETINVALID(d3dtex);

	//d3dtex->pChunkData = pChunkData;
	//return nTextureId;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL e_UIChunkTexture(
	D3D_TEXTURE * pTexture,
	UINT nChunkWidth,
	UINT nChunkHeight,
	const CHAR * szBaseFilename)
{
	// CML 2007.1.19
	ASSERTX_RETINVALID( 0, "Chunking is disabled! Don't try to use it!" );

	ASSERT_RETZERO(pTexture);

	if (!pTexture->pbLocalTextureData)
	{
		unsigned int nSize;
		V( e_TextureGetBits(pTexture->nId, (void**)&pTexture->pbLocalTextureData, nSize, TRUE ) );
		ASSERT_RETZERO(nSize > 0);
	}

	UINT nArraySize = (pTexture->nWidthInPixels / nChunkWidth) * (pTexture->nHeight / nChunkHeight);

	ASSERT_RETZERO(pTexture->pChunkData == NULL);
	pTexture->pChunkData = (TEXTURE_CHUNK_DATA *)MALLOCZ(NULL, sizeof(TEXTURE_CHUNK_DATA));
	pTexture->pChunkData->nChunkDataVersion = CHUNK_DATA_VERSION;
	pTexture->pChunkData->nChunkHeight = nChunkHeight;
	pTexture->pChunkData->nChunkWidth = nChunkWidth;
	pTexture->pChunkData->nOrigWidthInPixels = pTexture->nWidthInPixels;
	pTexture->pChunkData->nOrigHeight = pTexture->nHeight;
	pTexture->pChunkData->nMapSize = nArraySize;
	pTexture->pChunkData->pnChunkMap = (UINT *)MALLOCZ(NULL, nArraySize * sizeof (UINT));

	// iterate each chunk, add uniques to pnSavedChunkArray
	UINT * ChunkCRC = (UINT *)MALLOCZ(NULL, nArraySize * sizeof(UINT));		// array of CRCs for each chunk
	AUTOFREE autofreeChunkCRC(NULL, ChunkCRC);

	UINT * pnSavedChunkArray = (UINT *)MALLOCZ(NULL, nArraySize * sizeof (UINT));
	AUTOFREE autofreeSavedChunkArray(NULL, pnSavedChunkArray);

	UINT nNumSavedChunks = 0;
	UINT nDupeChunks = 0;

	{
	TIMER_START("chunk");

	for (UINT ii = 0; ii < nArraySize; ++ii)
	{
		ChunkCRC[ii] = sChunkComputeCRC(ii, pTexture);

		BOOL found = FALSE;
		for (UINT jj = 0; jj < ii; ++jj)
		{
			if (ChunkCRC[ii] != ChunkCRC[jj])
			{
				continue;
			}
			if (!sChunkCompare(ii, jj, pTexture))
			{
				found = FALSE;
				continue;
			}
			found = TRUE;
			pTexture->pChunkData->pnChunkMap[ii] = pTexture->pChunkData->pnChunkMap[jj];
			++nDupeChunks;
			break;
		}

		if (!found)
		{
			pTexture->pChunkData->pnChunkMap[ii] = nNumSavedChunks + 1;
			pnSavedChunkArray[nNumSavedChunks] = ii;
			++nNumSavedChunks;
		}
	}
	}

	UINT nChunkedTexWidth, nChunkedTexHeight;
	sGetBestFitPow2Res(
		nNumSavedChunks, 
		pTexture->pChunkData->nChunkWidth, 
		pTexture->pChunkData->nChunkHeight,
		pTexture->pChunkData->nHorizChunks, 
		pTexture->pChunkData->nVertChunks,
		nChunkedTexWidth,
		nChunkedTexHeight);

	SPD3DCTEXTURE2D pD3DTexture;
	V( dxC_Create2DTexture( nChunkedTexWidth, nChunkedTexHeight, 1, D3DC_USAGE_2DTEX_SCRATCH, (DXC_FORMAT)pTexture->nFormat, &pD3DTexture ) );

	D3DLOCKED_RECT tLockedRect;
	V( dxC_MapTexture( pD3DTexture, 0, &tLockedRect ) ); 
	ASSERT_RETFALSE(dxC_nMappedRectPitch( tLockedRect ) == nChunkedTexWidth * 4 );
	DWORD *pSaveBuffer = (DWORD *)(void*)dxC_pMappedRectData( tLockedRect );

	// Memory move
	for (UINT y=0; y < nChunkedTexHeight; y++)
	{
		for (UINT x=0; x < nChunkedTexWidth; x++)
		{
			UINT iSavd = ((y / nChunkHeight) * pTexture->pChunkData->nHorizChunks) + (x / nChunkWidth);
			if (iSavd < nNumSavedChunks &&
				y < pTexture->pChunkData->nVertChunks * nChunkHeight &&
				x < pTexture->pChunkData->nHorizChunks * nChunkWidth)
			{
				int nChunkX = x % nChunkWidth;
				int nChunkY = y % nChunkHeight;
				DWORD dwPixel = sGetPixel(pnSavedChunkArray[iSavd], nChunkX, nChunkY, pTexture);
				*pSaveBuffer = dwPixel;
			}
			else
			{
				*pSaveBuffer = 0;
			}
			pSaveBuffer++;
		}
	}

	// Now copy the new (chunked) pixels over to the texture
	FREE(NULL, pTexture->pbLocalTextureData);
	pTexture->nWidthInPixels = nChunkedTexWidth;
	pTexture->nHeight = nChunkedTexHeight;
	pTexture->nWidthInBytes = nChunkedTexWidth * 4;

	UINT nChunkTexSize = pTexture->nHeight * pTexture->nWidthInBytes;
	ASSERT( nChunkTexSize > 0 );
	pTexture->pbLocalTextureData = (BYTE *)MALLOC(NULL, nChunkTexSize);
	MemoryCopy(pTexture->pbLocalTextureData, nChunkTexSize, tLockedRect.pBits, nChunkTexSize);
	V( dxC_UnmapTexture( pD3DTexture, 0 ) );

	// Save the chunked texture out to a DDS file
	CHAR szChunkedFilename[MAX_PATH];
	PStrPrintf(szChunkedFilename, MAX_PATH, "%s_chunked.png", szBaseFilename);
	V( dxC_SaveTextureToFile( szChunkedFilename, (D3DC_IMAGE_FILE_FORMAT)D3DXIFF_PNG, pD3DTexture, TRUE ) );
	// Save out the additional chunk data
	CHAR szChunkDataFilename[MAX_PATH];
	PStrPrintf(szChunkDataFilename, MAX_PATH, "%s_chunkdata.dat", szBaseFilename);
	HANDLE hFile = FileOpen(szChunkDataFilename, GENERIC_WRITE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL);
	ASSERTV_RETTRUE( hFile != INVALID_FILE, "Unable to open file: '%s' for writing", szChunkDataFilename);

	// first write everything but the map pointer
	FileWrite(hFile, (void *)pTexture->pChunkData, sizeof( TEXTURE_CHUNK_DATA )  - sizeof(UINT *));
	// then write the map data
	FileWrite(hFile, (void *)pTexture->pChunkData->pnChunkMap, sizeof(UINT) * pTexture->pChunkData->nMapSize );
	FileClose(hFile);

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PRESULT e_UITextureCreateEmpty(
	int nWidth,
	int nHeight,
	int& nTextureID,
	BOOL bMask )
{
	nTextureID = INVALID_ID;
	// DX10 fakes ARGB4 with ARGB8, causing lock-and-walk to be incorrect.
	DXC_FORMAT tFormat = DX9_BLOCK( bMask ? D3DFMT_A4R4G4B4 : ) D3DFMT_A8R8G8B8;
	REF( bMask );
	V_RETURN( dx9_TextureNewEmpty( &nTextureID, nWidth, nHeight, 1, tFormat, TEXTURE_GROUP_UI, TEXTURE_DIFFUSE ) );
	ASSERT_RETFAIL( nTextureID != INVALID_ID );
	e_TextureAddRef( nTextureID );
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PRESULT e_UITextureLoadTextureFile(
	int nDefaultId,
	const char* filename,
	int nChunkSize,
	int& nTextureID,
	BOOL bAllowAsync,
	BOOL bLoadFromLocalizedPak)
{
	CHAR szBaseFilename[MAX_PATH];
	PStrRemoveExtension(szBaseFilename, MAX_PATH, filename);

	nTextureID = INVALID_ID;
	DWORD dwLoadFlags = 0;
	if ( ! bAllowAsync )
		dwLoadFlags |= TEXTURE_LOAD_NO_ASYNC_DEFINITION | TEXTURE_LOAD_NO_ASYNC_TEXTURE;
	if (AppCommonIsAnyFillpak())
	{
		dwLoadFlags |= TEXTURE_LOAD_FILLPAK_LOAD_FROM_PAK;
	}

	V_RETURN( e_TextureNewFromFile(
		&nTextureID,
		filename,
		TEXTURE_GROUP_UI,
		TEXTURE_DIFFUSE,
		TEXTURE_FLAG_NOLOD,
		0,
		NULL,
		NULL,
		NULL,
		NULL,
		dwLoadFlags,
		0.0f,
		D3DC_USAGE_INVALID,
		(bLoadFromLocalizedPak ? PAK_LOCALIZED : PAK_DEFAULT)) );

	ASSERT_RETFAIL( nTextureID != INVALID_ID );

	D3D_TEXTURE* texture = dxC_TextureGet( nTextureID );

	ASSERT_RETFAIL( texture );
	ASSERT( bAllowAsync || texture->nWidthInPixels > 0 );
	ASSERT( bAllowAsync || texture->nWidthInBytes > 0 );
	ASSERT( bAllowAsync || texture->nHeight > 0 );
	ASSERT( texture->nId == nTextureID );

	e_TextureAddRef( texture->nId );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PRESULT e_UITextureAddFrame( UI_TEXTURE_FRAME *pFrame, int nTextureID, const UI_RECT& rect )
{
	if ( e_IsNoRender() )
		return S_FALSE;

	ASSERT_RETINVALIDARG(pFrame);
	D3D_TEXTURE* d3dtex = dxC_TextureGet(nTextureID);
	ASSERT_RETINVALIDARG(d3dtex);

	if (pFrame->m_bHasMask)
	{
		pFrame->m_nMaskSize = sizeof(BYTE) * ((UINT)pFrame->m_fWidth * (UINT)pFrame->m_fHeight) / 8;

#if USE_MEMORY_MANAGER && !ISVERSION(SERVER_VERSION)
		pFrame->m_pbMaskBits = (BYTE *)MALLOCZ(g_StaticAllocator, pFrame->m_nMaskSize);
#else
		pFrame->m_pbMaskBits = (BYTE *)MALLOCZ(NULL, pFrame->m_nMaskSize);
#endif		
		
		ASSERT_RETVAL( pFrame->m_pbMaskBits, E_OUTOFMEMORY );
	}

	// if we need the texture bits (for chunking or masking) make sure we have them
	if ( !d3dtex->pbLocalTextureData && 
		(d3dtex->pChunkData || pFrame->m_bHasMask))
	{
		unsigned int nSize;
		V_RETURN( e_TextureGetBits( d3dtex->nId, (void**)&d3dtex->pbLocalTextureData, nSize, TRUE ) );
		ASSERT_RETFAIL( nSize > 0 );
		ASSERT_RETFAIL(d3dtex->pbLocalTextureData);
	}

	if (!d3dtex->pChunkData || d3dtex->pChunkData->nChunkHeight == 0  || d3dtex->pChunkData->nChunkWidth == 0)
	{
		pFrame->m_nNumChunks = 1;
		pFrame->m_pChunks = (UI_TEXTURE_FRAME_CHUNK *)MALLOC(NULL, sizeof(UI_TEXTURE_FRAME_CHUNK) * pFrame->m_nNumChunks);
		ASSERT_RETVAL( pFrame->m_pChunks, E_OUTOFMEMORY );
		memclear(pFrame->m_pChunks, sizeof(UI_TEXTURE_FRAME_CHUNK) * pFrame->m_nNumChunks);

		int nWidth, nHeight;
		V( e_TextureGetOriginalResolution( nTextureID, nWidth, nHeight ) );

		ASSERT_RETFAIL( d3dtex->nWidthInPixels > 0 );
		ASSERT_RETFAIL( d3dtex->nHeight > 0 );
		int nXRatio = nWidth  / d3dtex->nWidthInPixels;
		int nYRatio = nHeight / d3dtex->nHeight;

		ASSERT_RETFAIL( nWidth > 0 );
		ASSERT_RETFAIL( nHeight > 0 );
		pFrame->m_pChunks->m_fU1 = rect.m_fX1/(float)nWidth;
		pFrame->m_pChunks->m_fV1 = rect.m_fY1/(float)nHeight;
		pFrame->m_pChunks->m_fU2 = rect.m_fX2/(float)nWidth;
		pFrame->m_pChunks->m_fV2 = rect.m_fY2/(float)nHeight;

		pFrame->m_pChunks->m_fWidth = pFrame->m_fWidth;
		pFrame->m_pChunks->m_fHeight = pFrame->m_fHeight;

		if (pFrame->m_bHasMask)
		{
			BYTE *pbBytePos = pFrame->m_pbMaskBits;
			short unsigned int nBitNum = 0;
			for (int jj = (int)rect.m_fY1; jj < (int)rect.m_fY2; jj++)
			{
				int tj = jj / nYRatio;
				for (int ii = (int)rect.m_fX1; ii < (int)rect.m_fX2; ii++)
				{
					int ti = ii / nXRatio;
					DWORD dwPixel = sGetPixel(ti, tj, d3dtex);
					*pbBytePos |= ((dwPixel >> 24) != 0) << nBitNum;

					nBitNum++;
					if (nBitNum > 7)
					{
						pbBytePos++;
						nBitNum = 0;
					}
				}
			}
		}

		//	pbBytePos = pFrame->m_pbMaskBits;
		//	nBitNum = 0;
		//	for (int jj = (int)rect.m_fY1; jj < (int)rect.m_fY2; jj++)
		//	{
		//		for (int ii = (int)rect.m_fX1; ii < (int)rect.m_fX2; ii++)
		//		{
		//			if ((*pbBytePos) & (1 << nBitNum))
		//				trace("1");
		//			else
		//				trace(" ");

		//			nBitNum++;
		//			if (nBitNum > 7)
		//			{
		//				pbBytePos++;
		//				nBitNum = 0;
		//			}
		//		}
		//		trace("\n");
		//	}
		//	trace("\n--------------------------------------------------------------------------------\n");
		//}

		return S_OK;
	}

	UINT curX = (UINT)rect.m_fX1;
	UINT curY = (UINT)rect.m_fY1;

	UINT nNumChunks = 0;
	UINT nInsane = (UINT)((CEIL(pFrame->m_fWidth / (float)d3dtex->pChunkData->nChunkWidth)+1.0f) *
		(CEIL(pFrame->m_fHeight / (float)d3dtex->pChunkData->nChunkHeight)+1.0f));

	//This is the first chunk array which is a guess size.  We'll copy it into a properly-sized memory space after we determine what that size is.
	UI_TEXTURE_FRAME_CHUNK *pTempChunkArray = (UI_TEXTURE_FRAME_CHUNK *)MALLOCZ(NULL, nInsane * sizeof(UI_TEXTURE_FRAME_CHUNK));
	memclear(pTempChunkArray, nInsane * sizeof(UI_TEXTURE_FRAME_CHUNK));
	
	for (;curY < rect.m_fY2 && nNumChunks <= nInsane; nNumChunks++)
	{
		if (nNumChunks > 0)
		{
			pTempChunkArray[nNumChunks].m_fXOffset = (float)(rect.m_fX1 - curX);
			pTempChunkArray[nNumChunks].m_fYOffset = (float)(rect.m_fY1 - curY);
		}

		UINT nChunk, nChunkX, nChunkY;
		sChunkGet(curX, 
			curY, 
			d3dtex->pChunkData->nChunkWidth, 
			d3dtex->pChunkData->nChunkHeight, 
			d3dtex->pChunkData->nOrigWidthInPixels, 
			nChunk, 
			nChunkX, 
			nChunkY);

		ASSERT_CONTINUE(nChunk < d3dtex->pChunkData->nMapSize);
		int nSavedChunk = d3dtex->pChunkData->pnChunkMap[nChunk]-1;
		ASSERT_CONTINUE(nSavedChunk >= 0);
		int nSavedChunkX = ((nSavedChunk % d3dtex->pChunkData->nHorizChunks) * d3dtex->pChunkData->nChunkWidth);
		int nSavedChunkY = ((nSavedChunk / d3dtex->pChunkData->nHorizChunks) * d3dtex->pChunkData->nChunkHeight);

		pTempChunkArray[nNumChunks].m_fU1 = (float)(nSavedChunkX + nChunkX) / (float)d3dtex->nWidthInPixels;
		pTempChunkArray[nNumChunks].m_fV1 = (float)(nSavedChunkY + nChunkY) / (float)d3dtex->nHeight;

		float fWidth = (float)d3dtex->pChunkData->nChunkWidth - nChunkX;
		float fHeight = (float)d3dtex->pChunkData->nChunkHeight - nChunkY;

		// if the chunk is bigger than the frame, be sure to only use the part of it we want
		if ((curX - (UINT)rect.m_fX1) + fWidth > pFrame->m_fWidth)
		{
			fWidth = pFrame->m_fWidth - (curX - (UINT)rect.m_fX1);
		}
		if ((curY - (UINT)rect.m_fY1) + fHeight > pFrame->m_fHeight)
		{
			fHeight = pFrame->m_fHeight - (curY - (UINT)rect.m_fY1);
		}
		pTempChunkArray[nNumChunks].m_fWidth = fWidth;
		pTempChunkArray[nNumChunks].m_fHeight = fHeight;
		ASSERT_CONTINUE(pTempChunkArray[nNumChunks].m_fWidth > 0.0f && pTempChunkArray[nNumChunks].m_fHeight > 0.0f);

		pTempChunkArray[nNumChunks].m_fU2 = (float)(nSavedChunkX + nChunkX + fWidth) / (float)d3dtex->nWidthInPixels;
		pTempChunkArray[nNumChunks].m_fV2 = (float)(nSavedChunkY + nChunkY + fHeight) / (float)d3dtex->nHeight;

		if (pFrame->m_bHasMask)
		{
			int nFrameX = curX - (UINT)rect.m_fX1;
			int nFrameY = curY - (UINT)rect.m_fY1;
			for (int jj = 0; jj < (int)fHeight; jj++)
			{
				for (int ii = 0; ii < (int)fWidth; ii++)
				{
					DWORD dwPixel = sGetPixel(nSavedChunkX + nChunkX + ii, nSavedChunkY + nChunkY + jj, d3dtex);
					int nFlatPixelPos = nFrameX + ii + ((nFrameY + jj) * (int)pFrame->m_fWidth);
					int nByte = nFlatPixelPos / 8;
					int nBit  = nFlatPixelPos % 8;
					BYTE *pbBytePos = pFrame->m_pbMaskBits;
					pbBytePos += nByte;
					*pbBytePos |= ((dwPixel >> 24) != 0) << nBit;
				}
			}
		}

		curX += d3dtex->pChunkData->nChunkWidth - nChunkX;
		if (curX >= rect.m_fX2)
		{
			curX = (UINT)rect.m_fX1;
			curY += d3dtex->pChunkData->nChunkHeight - nChunkY;
		}
		ASSERT(nNumChunks <= nInsane);
	}
	ASSERT(nNumChunks > 0);

	pFrame->m_nNumChunks = nNumChunks;
	if (nNumChunks > 0)
	{
		// Now we can copy from the guessed-sized array to the properly-sized one.  Efficiency dontcha know.
		pFrame->m_pChunks = (UI_TEXTURE_FRAME_CHUNK *)MALLOC(NULL, sizeof(UI_TEXTURE_FRAME_CHUNK) * pFrame->m_nNumChunks);
		ASSERT_RETVAL( pFrame->m_pChunks, E_OUTOFMEMORY );
		
		memclear(pFrame->m_pChunks, sizeof(UI_TEXTURE_FRAME_CHUNK) * pFrame->m_nNumChunks);
		memcpy(pFrame->m_pChunks, pTempChunkArray, sizeof(UI_TEXTURE_FRAME_CHUNK) * pFrame->m_nNumChunks);
	}

	FREE(NULL, pTempChunkArray);
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PRESULT e_UIRestoreTextureFile(
	int nTextureID )
{
	D3D_TEXTURE* pTexture = dxC_TextureGet(nTextureID);

	if ( pTexture && pTexture->pbLocalTextureData )
	{
		V_RETURN( e_TextureRestore( nTextureID ) );
		return S_OK;
	}

	return S_FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PRESULT e_UISaveTextureFile(
	int nTextureID, 
	const char * pszFileNameWithPath,
	BOOL bUpdatePak /*= TRUE*/ )
{
	V_RETURN( e_TextureSave( nTextureID, pszFileNameWithPath, 0, TEXTURE_SAVE_PNG, bUpdatePak ) );

	return S_OK;
}

//----------------------------------------------------------------------------
// ATLAS loading: prepare texture font
//----------------------------------------------------------------------------
PRESULT UIX_TEXTURE_FONT::Init(
	UIX_TEXTURE *pTexture)
{

	D3D_TEXTURE* d3dtex = dxC_TextureGet(pTexture->m_nTextureId);
	ASSERT_RETINVALIDARG(d3dtex);
	//ASSERT_RETFALSE(d3dtex->pbLocalTextureData);

	if ( ! pTexture->m_arrFontRows.Initialized() )
	{
		int nWidth, nHeight;
		V( e_TextureGetOriginalResolution( pTexture->m_nTextureId, nWidth, nHeight ) );
		pTexture->m_fTextureHeight = (float)nHeight;
		pTexture->m_fTextureWidth  = (float)nWidth;

		if ( nHeight != 0 )
		{
			ArrayInit(pTexture->m_arrFontRows, NULL, nHeight / 16);
		}
	}

	if ( ! e_IsNoRender() && ! d3dtex->pbLocalTextureData )
	{
		unsigned int nSize;
		V( e_TextureGetBits( d3dtex->nId, (void**)&d3dtex->pbLocalTextureData, nSize, TRUE ) );
		ASSERT( nSize > 0 );
	}

	TCHAR fullname[MAX_PATH];
	m_bLoadedFromFile = FALSE;
	// look for local font
	if( PStrLen( m_szLocalPath ) != 0 )
	{
		FileGetFullFileName(fullname, m_szLocalPath, MAX_PATH);

		DECLARE_LOAD_SPEC(spec, fullname);
		spec.flags |= PAKFILE_LOAD_ADD_TO_PAK | PAKFILE_LOAD_DONT_LOAD_FROM_PAK;
		spec.flags |= PAKFILE_LOAD_ALWAYS_WARN_IF_MISSING;
		void * pBits = PakFileLoadNow( spec );

		if ( ! e_IsNoRender() && pBits )
		{
			ASSERT( spec.bytesread > 0 );
			DWORD dwInstalled;
			m_hInstalledFont = NULL;
			int nFontTries = 10;
			int nCurTry = 0;
			while ( ! m_hInstalledFont && nCurTry < nFontTries )
			{
				m_hInstalledFont = AddFontMemResourceEx( pBits, spec.bytesread, 0, &dwInstalled );
				if ( ! m_hInstalledFont )
				{
					trace( "No font, sleeping: %s\n", fullname );
					Sleep( 100 );
					nCurTry++;
				}
			}

			if( m_hInstalledFont != 0 )
			{
				m_bLoadedFromFile = TRUE;
				//SendMessage( HWND_BROADCAST, WM_FONTCHANGE, 0, 0 );
			}
			ASSERTV(m_hInstalledFont, "WIN: Failed to add memory resource for font %s - spec.bytesread = %u - dwInstalled = %u", fullname, spec.bytesread, dwInstalled );

			FREE( spec.pool, pBits );
		}
	}

	m_hInstalledAsciiFont = NULL;
	if( PStrLen( m_szAsciiLocalPath ) != 0 )
	{
		FileGetFullFileName(fullname, m_szAsciiLocalPath, MAX_PATH);

		DECLARE_LOAD_SPEC(spec, fullname);
		spec.flags |= PAKFILE_LOAD_ADD_TO_PAK | PAKFILE_LOAD_DONT_LOAD_FROM_PAK;
		spec.flags |= PAKFILE_LOAD_ALWAYS_WARN_IF_MISSING;
		void * pBits = PakFileLoadNow( spec );

		if ( ! e_IsNoRender() && pBits )
		{
			ASSERT( spec.bytesread > 0 );
			DWORD dwInstalled;
			m_hInstalledAsciiFont = AddFontMemResourceEx( pBits, spec.bytesread, 0, &dwInstalled );
			ASSERTV(m_hInstalledFont, "WIN: Failed to add memory resource for font %s - spec.bytesread = %u - dwInstalled = %u", fullname, spec.bytesread, dwInstalled );
			FREE( spec.pool, pBits );
		}
	}

	m_pTexture = pTexture;

	HashInit(m_hashSizes, NULL, 5);		

	return S_OK;
}

void UIX_TEXTURE_FONT::Destroy(
	void)
{
	
	CHAR_HASH *pCharHash = m_hashSizes.GetFirstItem();
	while (pCharHash)
	{
		CHAR_HASH *pNextHash = m_hashSizes.GetNextItem(pCharHash);
		HashFree(*pCharHash);
		pCharHash = pNextHash;
	}
	HashFree(m_hashSizes);

	if (m_pGlyphset)
	{
		FREE(NULL, m_pGlyphset);
	}
	if( m_bLoadedFromFile && m_hInstalledFont )
	{
		ASSERTX( RemoveFontMemResourceEx( m_hInstalledFont ), "Failed unloading private font!" );
	}
	if (m_hInstalledAsciiFont)
	{
		ASSERTX( RemoveFontMemResourceEx( m_hInstalledAsciiFont ), "Failed unloading private font!" );
	}
}

void UIX_TEXTURE_FONT::Reset(
	void)
{
	CHAR_HASH *pCharHash = m_hashSizes.GetFirstItem();
	while (pCharHash)
	{
		CHAR_HASH *pNextHash = m_hashSizes.GetNextItem(pCharHash);
		HashFree(*pCharHash);

		pCharHash = pNextHash;
	}
	//HashFree(m_hashSizes);

	//HashInit(m_hashSizes, NULL, 5);		

	m_hashSizes.Clear();
}

void UIX_TEXTURE_FONT::CheckRows(
	void)
{
	CHAR_HASH *pCharHash = m_hashSizes.GetFirstItem();
	while (pCharHash)
	{
		CHAR_HASH *pNextHash = m_hashSizes.GetNextItem(pCharHash);

		UI_TEXTURE_FONT_FRAME *pFrame = pCharHash->GetFirstItem();
		while (pFrame)
		{
			if (pFrame->m_nTextureFontRowId != INVALID_ID)
			{
				UIX_TEXTURE_FONT_ROW *pRow = m_pTexture->m_arrFontRows.Get(pFrame->m_nTextureFontRowId);
				ASSERT(pRow);

				BOOL bFound = FALSE;
				UINT iChar = pRow->m_arrFontCharacters.GetFirst();
				while (iChar != INVALID_ID)
				{
					UI_TEXTURE_FONT_FRAME **ppChar = pRow->m_arrFontCharacters.Get(iChar);
					ASSERT(ppChar && *ppChar);
					if ((*ppChar) == pFrame)
					{
						bFound = TRUE;
						break;
					}
					iChar = pRow->m_arrFontCharacters.GetNextId(iChar);
				}
				ASSERT(bFound);
			}

			pFrame = pCharHash->GetNextItem(pFrame);
		}


		pCharHash = pNextHash;
	}
}
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline static void sFontTextureTraceRows(
	UIX_TEXTURE * pTexture)
{
	ASSERT_RETURN(pTexture);
	float fLowestY = 0.0f;
	int iRow = pTexture->m_arrFontRows.GetFirst();
	while (iRow != INVALID_ID)
	{
		UIX_TEXTURE_FONT_ROW * pRow = pTexture->m_arrFontRows.Get(iRow);

		trace("%d - y = %0.0f\t h = %0.2f\n", iRow, pRow->m_fRowY, pRow->m_fRowHeight);
		iRow = pTexture->m_arrFontRows.GetNextId(iRow);
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline static BOOL sFontTextureCheckRows(
	UIX_TEXTURE * pTexture)
{
	ASSERT_RETFALSE(pTexture);
	float fLowestY = 0.0f;

	int iRow = pTexture->m_arrFontRows.GetFirst();
	while (iRow != INVALID_ID)
	{
		UIX_TEXTURE_FONT_ROW * pRow = pTexture->m_arrFontRows.Get(iRow);
		if (fLowestY > pRow->m_fRowY)
		{
			sFontTextureTraceRows(pTexture);
			return FALSE;
		}

		fLowestY = pRow->m_fRowY + pRow->m_fRowHeight;
		iRow = pTexture->m_arrFontRows.GetNextId(iRow);
	}

	return TRUE;
}

#include "dictionary.h"
inline static void sFontTextureCheckCharRowIDs(
	UIX_TEXTURE * pTexture)
{
	if (pTexture->m_pFonts)
	{
		int nFonts = StrDictionaryGetCount(pTexture->m_pFonts);
		for (int i=0; i < nFonts; i++)
		{
			UIX_TEXTURE_FONT *pFont = (UIX_TEXTURE_FONT *)StrDictionaryGet(pTexture->m_pFonts, i);
			ASSERT_RETURN(pFont);
			pFont->CheckRows();
		}
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDebugOutputFontTextureChars(
	UIX_TEXTURE * pTexture,
	DWORD dwCurTime,
	DWORD dwOldest)
{
	ASSERT_RETURN(pTexture);
#if ISVERSION(DEVELOPMENT)
	UINT nCount = pTexture->m_arrFontRows.Count();
	UIX_TEXTURE_FONT_ROW * pRow = NULL;
	int iRow = pTexture->m_arrFontRows.GetFirst();
	while (iRow != INVALID_ID)
	{
		pRow = pTexture->m_arrFontRows.Get(iRow);
		ASSERT_RETURN(pRow);

		UINT nCharCount = pRow->m_arrFontCharacters.Count();
		TraceDebugOnly("Y:%4.0f H:%4.0f X:%4.0f #:%4d  age:%u", pRow->m_fRowY, pRow->m_fRowHeight, pRow->m_fRowEndX, nCharCount, dwCurTime - pRow->m_dwLastAccess );
		iRow = pTexture->m_arrFontRows.GetNextId(iRow);
	}
#endif
}

inline static float sTextureGetFontRowSpace(
	UIX_TEXTURE * pTexture, 
	UINT iRow)
{
	UIX_TEXTURE_FONT_ROW * pRow = pTexture->m_arrFontRows.Get(iRow);
	ASSERT_RETVAL(pRow, 0.0f);
	int iNextRow = pTexture->m_arrFontRows.GetNextId(iRow);
	if (iNextRow != INVALID_ID)
	{
		UIX_TEXTURE_FONT_ROW * pNextRow = pTexture->m_arrFontRows.Get(iNextRow);
		ASSERT_RETVAL(pNextRow, 0.0f);
		return pNextRow->m_fRowY - pRow->m_fRowY;
	}

	return (pTexture->m_rectFontArea.m_fY2 - 1.0f) - pRow->m_fRowY;
}

static const int VERT_SPACING = 2;
static const int HORIZ_SPACING = 2;

static UIX_TEXTURE_FONT_ROW * sTextureGetEmptyFontRow(
	UIX_TEXTURE * pTexture, 
	int nFontheight,
	int nCharwidth,
	int& nRowId)
{
	ASSERT_RETNULL(pTexture);
	ASSERT_RETNULL(nFontheight);

	nRowId = INVALID_ID;
	UINT nCount = pTexture->m_arrFontRows.Count();
	UIX_TEXTURE_FONT_ROW * pRow = NULL;
	if (nCount == 0)
	{
		pRow = pTexture->m_arrFontRows.Add(&nRowId);
		ASSERT(pRow);
		structclear(pRow->m_arrFontCharacters);
		structclear(*pRow);

		pRow->m_fRowEndX = pTexture->m_rectFontArea.m_fX1;
		pRow->m_fRowY = pTexture->m_rectFontArea.m_fY1;
		pRow->m_fRowHeight = (float)nFontheight;
		ArrayInit(pRow->m_arrFontCharacters, NULL, (int)(pTexture->m_rectFontArea.m_fY2 - pTexture->m_rectFontArea.m_fY1) / nFontheight);
		//sFontTextureCheckRows(pTexture);
		return pRow;
	}

	// see if there's a row of the proper height that has space left
	int iRow = pTexture->m_arrFontRows.GetFirst();
	while (iRow != INVALID_ID)
	{
		pRow = pTexture->m_arrFontRows.Get(iRow);
		ASSERT_RETNULL(pRow);
		if (pRow->m_fRowHeight == (float)nFontheight && 
			pRow->m_fRowEndX + nCharwidth + HORIZ_SPACING < pTexture->m_rectFontArea.m_fX2)
		{
			// there's room on this row for at least one more character
			//sFontTextureCheckRows(pTexture);
			//sFontTextureCheckCharRowIDs(pTexture);
			nRowId = iRow;
			return pRow;
		}
		iRow = pTexture->m_arrFontRows.GetNextId(iRow);
	}

	// ok, now see if there's room for a whole new row
	iRow = pTexture->m_arrFontRows.GetLast();
	UIX_TEXTURE_FONT_ROW * pLastRow = pTexture->m_arrFontRows.Get(iRow);
	ASSERT_RETNULL(pLastRow);

	// proposed new Y = pLastRow->m_fRowY + pLastRow->m_fRowHeight + VERT_SPACING
	// total new row height = nFontheight + VERT_SPACING
	if (pLastRow->m_fRowY + pLastRow->m_fRowHeight + VERT_SPACING + nFontheight + VERT_SPACING < pTexture->m_rectFontArea.m_fY2)
	{
		pRow = pTexture->m_arrFontRows.Add(&nRowId, TRUE);
		ASSERT(pRow);
		structclear(pRow->m_arrFontCharacters);
		structclear(*pRow);

		pRow->m_fRowEndX = pTexture->m_rectFontArea.m_fX1;
		pRow->m_fRowY = pLastRow->m_fRowY + pLastRow->m_fRowHeight + VERT_SPACING;
		pRow->m_fRowHeight = (float)nFontheight;

		ASSERT_RETNULL( pRow->m_fRowY + nFontheight + VERT_SPACING < pTexture->m_rectFontArea.m_fY2 );

		ArrayInit(pRow->m_arrFontCharacters, NULL, (int)(pTexture->m_rectFontArea.m_fY2 - pTexture->m_rectFontArea.m_fY1) / nFontheight);
		//sFontTextureCheckRows(pTexture);
		//sFontTextureCheckCharRowIDs(pTexture);
		return pRow;
	}

	// find the oldest row of the proper size and replace it
	//  or, if there are more consecutive rows that are older, replace them with a new row
	DWORD dwOldest = (DWORD)-1;
	UIX_TEXTURE_FONT_ROW * pOldestRow = NULL;
	float fRowSpace = 0.0f;
	UINT iOldest = INVALID_ID;
	int nOldestCount = 0;
	iRow = pTexture->m_arrFontRows.GetFirst();
	while (iRow != INVALID_ID)
	{
		pRow = pTexture->m_arrFontRows.Get(iRow);
		ASSERT_RETNULL(pRow);

		fRowSpace = sTextureGetFontRowSpace(pTexture, iRow); 

		DWORD dwLastAccess = pRow->m_dwLastAccess;
		int nNumRowsNeeded = 1;
		int iNextRow = pTexture->m_arrFontRows.GetNextId(iRow);
		while (fRowSpace < (float)nFontheight + VERT_SPACING &&
			iNextRow != INVALID_ID)
		{
			UIX_TEXTURE_FONT_ROW * pNextRow = pTexture->m_arrFontRows.Get(iNextRow);
			dwLastAccess = MAX(dwLastAccess, pNextRow->m_dwLastAccess);
			fRowSpace += sTextureGetFontRowSpace(pTexture, iNextRow);
			nNumRowsNeeded++;
			iNextRow = pTexture->m_arrFontRows.GetNextId(iNextRow);
		}

		if (fRowSpace >= (float)nFontheight + VERT_SPACING && 
			dwLastAccess < dwOldest)
		{
			iOldest = iRow;
			nOldestCount = nNumRowsNeeded;
			dwOldest = dwLastAccess;
		}

		iRow = pTexture->m_arrFontRows.GetNextId(iRow);
	}

	DWORD dwCurTime = AppCommonGetRealTime();
	if (dwCurTime - dwOldest < 100 /*&& dwCurTime - dwOldest > 0*/)
	{
		// we're trying to replace something that's not that old
		TraceError(" !!! too much thrashing on the font texture");
#if ISVERSION(DEVELOPMENT)
		sDebugOutputFontTextureChars(pTexture, dwCurTime, dwOldest);
#endif
		e_UIRequestFontTextureReset(TRUE);
	}

	if (iOldest != INVALID_ID)
	{
		//if (nOldestCount > 1)
		//{
		//	trace("--- before deleting rows %d - %d\n", iOldest, iOldest + nOldestCount - 1 );
		//	sFontTextureTraceRows(pTexture);

		//}
		iRow = iOldest;
		int nCount = 0;
		while (iRow != INVALID_ID &&
			nCount < nOldestCount)
		{
			UIX_TEXTURE_FONT_ROW * pRowToClear = pTexture->m_arrFontRows.Get(iRow);
			ASSERT_RETNULL(pRowToClear);
			pRowToClear->m_fRowEndX = pTexture->m_rectFontArea.m_fX1;
			UINT iChar = pRowToClear->m_arrFontCharacters.GetFirst();
			while (iChar != INVALID_ID)
			{
				UI_TEXTURE_FONT_FRAME **ppChar = pRowToClear->m_arrFontCharacters.Get(iChar);
				ASSERT_RETNULL(ppChar && *ppChar);
				(*ppChar)->m_nTextureFontRowId = INVALID_ID;
				iChar = pRowToClear->m_arrFontCharacters.GetNextId(iChar);
			}
			pRowToClear->m_arrFontCharacters.Clear();

			int iNextRow = pTexture->m_arrFontRows.GetNextId(iRow);
			if (iRow != iOldest)
			{
				pRowToClear->m_arrFontCharacters.Destroy();
				pTexture->m_arrFontRows.Remove(iRow);
			}
			iRow = iNextRow;
			nCount++;
		}

		pOldestRow = pTexture->m_arrFontRows.Get(iOldest);
		ASSERT(pOldestRow);
		ASSERT(pOldestRow->m_arrFontCharacters.Count() == 0);
		pOldestRow->m_fRowEndX = pTexture->m_rectFontArea.m_fX1;
		pOldestRow->m_fRowHeight = (float)nFontheight;

#ifdef _TRACE_FONT_TEXTURE
		gnFontRowSwaps++;
#endif
		
		//if (nOldestCount > 1)
		//{
		//	trace("--- after deleting rows %d - %d\n", iOldest, iOldest + nOldestCount - 1);
		//	sFontTextureTraceRows(pTexture);

		//}

		//sFontTextureCheckRows(pTexture);
		//sFontTextureCheckCharRowIDs(pTexture);
		nRowId = iOldest;
		return pOldestRow;
	}

	nRowId = iRow;
	return pRow;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PRESULT e_UITextureFontAddCharactersToTexture(
	UIX_TEXTURE_FONT* font,
	const WCHAR *szCharacters,
	int nFontSize)
{
	if ( e_IsNoRender() )
		return S_FALSE;

	ASSERT_RETFAIL(font);

#define SPACE_BETWEEN_LINES		2
	static const int MAX_ERRMSG_LEN = 1024;
	char szErrmsg[MAX_ERRMSG_LEN];

	D3D_TEXTURE* d3dtex = NULL;
	BITMAPINFOHEADER tBitMapInfo;
	structclear(tBitMapInfo);
	DWORD* pdwMemPixelBuf = NULL;
	HBITMAP hBitMap = NULL;
	HFONT hGDIFont = NULL;
	HFONT hGDIAsciiFont = NULL;
	HDC hdc = NULL;
	TEXTMETRIC textmetric;
	BOOL bGDIInitialized = FALSE;
	int nRowId = INVALID_ID;
	UIX_TEXTURE_FONT_ROW *pFontRow = NULL;
	BOOL bSwitchFonts = FALSE;
	BOOL bUseAsciiFont = FALSE;

	const WCHAR *wcCurrent = szCharacters;
	WCHAR str[2] = { L'\0', L'\0' };


	while (wcCurrent && *wcCurrent)
	{
		if (PStrIsPrintable(*wcCurrent))
		{
			UI_TEXTURE_FONT_FRAME * pCharFrame = font->GetChar(*wcCurrent, nFontSize);
			if (!pCharFrame)
			{
				//pCharFrame = HashAdd(font->m_hashCharacters, (int)*wcCurrent);
				pCharFrame = font->AddChar(*wcCurrent, nFontSize);
			}

			ASSERT_RETFAIL(pCharFrame);
			if (pCharFrame->m_nTextureFontRowId == INVALID_ID)
			{
				//trace( "Adding character to font texture: [%d] '%C' \n", *wcCurrent, *wcCurrent );

				bSwitchFonts = (hGDIFont == NULL || bUseAsciiFont != iswascii( *wcCurrent ));
				bUseAsciiFont = iswascii( *wcCurrent );

				// initialize the texture and the GDI objects
				if (d3dtex == NULL || bSwitchFonts)
				{
					if (d3dtex == NULL)
					{
						d3dtex = dxC_TextureGet(font->m_pTexture->m_nTextureId);
						ASSERT_RETFAIL(d3dtex);
						ASSERT_RETFAIL(d3dtex->pD3DTexture);

						// create device independent bitmap
						//   make it large enough to comfortably hold one character
						tBitMapInfo.biSize = sizeof(BITMAPINFOHEADER);
						tBitMapInfo.biWidth = nFontSize * 2;
						tBitMapInfo.biHeight = -nFontSize * 2;
						tBitMapInfo.biPlanes = 1;
						tBitMapInfo.biBitCount = 32;
						tBitMapInfo.biCompression = BI_RGB;

						hdc = CreateCompatibleDC(NULL);
						if (hdc == NULL)
						{
							int len = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK, NULL, 
								GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)szErrmsg, MAX_ERRMSG_LEN, NULL);

							ASSERTX_RETFAIL(hdc, szErrmsg);
							// CHB 2007.08.01 - This should be unreachable because
							// of the ASSERTX_RETFAIL above.
							//HALTX(hdc, "Error initializing font");	// CHB 2007.08.01 - String audit: commented
						}

						hBitMap = CreateDIBSection(hdc, (tagBITMAPINFO*)&tBitMapInfo, DIB_RGB_COLORS, (void**)&pdwMemPixelBuf, NULL, 0);
						if (hBitMap == NULL)
						{
							FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK, NULL, 
								GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)szErrmsg, MAX_ERRMSG_LEN, NULL);

							ASSERTX_RETFAIL(hBitMap, szErrmsg);
							// CHB 2007.08.01 - This should be unreachable because
							// of the ASSERTX_RETFAIL above.
							//HALTX(hBitMap, "Error initializing font");	// CHB 2007.08.01 - String audit: commented
						}
						ASSERT_RETFAIL(pdwMemPixelBuf);

						SelectObject(hdc, hBitMap);
						SetTextColor(hdc, 0x00ffffff);
						SetBkColor(hdc, 0);
						SetBkMode(hdc, OPAQUE);
			            SelectObject(hdc, GetStockObject(BLACK_BRUSH)); 
			            SelectObject(hdc, GetStockObject(BLACK_PEN)); 

						hGDIFont = CreateFont(nFontSize, 0, 0, 0, font->m_nWeight, font->m_bItalic, 
							FALSE, FALSE, DEFAULT_CHARSET, OUT_TT_ONLY_PRECIS, CLIP_DEFAULT_PRECIS,
							CLEARTYPE_QUALITY /*ANTIALIASED_QUALITY*/, DEFAULT_PITCH, font->m_szSystemName);
						if (hGDIFont == NULL)
						{
							int len = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK, NULL, 
								GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)szErrmsg, MAX_ERRMSG_LEN, NULL);

							ASSERTX_RETFAIL(hGDIFont, szErrmsg);
							// CHB 2007.08.01 - This should be unreachable because
							// of the ASSERTX_RETFAIL above.
							//HALTX(hGDIFont, "Error initializing font");	// CHB 2007.08.01 - String audit: commented
						}

						if (font->m_szAsciiFontName && font->m_szAsciiFontName[0] && PStrICmp(font->m_szSystemName, font->m_szAsciiFontName) != 0)	// if this font uses a different font for ascii characters
						{
							hGDIAsciiFont = CreateFont(nFontSize, 0, 0, 0, font->m_nWeight, font->m_bItalic, 
								FALSE, FALSE, DEFAULT_CHARSET, OUT_TT_ONLY_PRECIS, CLIP_DEFAULT_PRECIS,
								CLEARTYPE_QUALITY /*ANTIALIASED_QUALITY*/, DEFAULT_PITCH, font->m_szAsciiFontName);
							if (hGDIAsciiFont == NULL)
							{
								int len = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS | FORMAT_MESSAGE_MAX_WIDTH_MASK, NULL, 
									GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)szErrmsg, MAX_ERRMSG_LEN, NULL);

								ASSERTX_RETFAIL(hGDIAsciiFont, szErrmsg);
								// CHB 2007.08.01 - This should be unreachable because
								// of the ASSERTX_RETFAIL above.
								//HALTX(hGDIAsciiFont, "Error initializing font");	// CHB 2007.08.01 - String audit: commented
							}
						}
					}

					SelectObject(hdc, (bUseAsciiFont && hGDIAsciiFont != NULL ? hGDIAsciiFont : hGDIFont));

					GetTextMetrics(hdc, &textmetric);

					//if (!font->m_pGlyphset)
					//{
					//	DWORD dwSize = GetFontUnicodeRanges(hdc, NULL);
					//	ASSERTX(dwSize > 0, "Empty GetFontUnicodeRanges");
					//	font->m_pGlyphset = (GLYPHSET *)MALLOCZ(NULL, dwSize);
					//	GetFontUnicodeRanges(hdc, font->m_pGlyphset);
					//}

					bGDIInitialized = TRUE;

				#if (ISVERSION(DEBUG_VERSION))
					char szFontFace[512];
					GetTextFace(hdc, 512, szFontFace);
					if (PStrICmp(
						bUseAsciiFont && hGDIAsciiFont != NULL ? font->m_szAsciiFontName : font->m_szSystemName, szFontFace) != 0)
					{
						ASSERTV("You do not have the font [%s] installed on your system.\nThe replacement font [%s] is being used instead.", font->m_szSystemName, szFontFace);
					}
				#endif

				}

				int nFontheight = MAX(textmetric.tmHeight, textmetric.tmAscent + textmetric.tmDescent);
				ABC abc;
				if (!GetCharABCWidthsW(hdc, *wcCurrent, *wcCurrent, &abc))
				{
					// CLEAN UP GDI STUFF
					if (bGDIInitialized)
					{
						DeleteObject(hGDIFont);
						DeleteObject(hGDIAsciiFont);
						DeleteObject(hBitMap);
						DeleteDC(hdc);
					}

					ASSERTX(FALSE, "Failed to get sizing info for font");
					return E_FAIL;
				}
				SIZE size;
				str[0] = *wcCurrent;
				GetTextExtentPoint32W(hdc, str, 1, &size);

				int nFullWidth = ABS(abc.abcA) + abc.abcB + (abc.abcC > 0 ? abc.abcC : 0);

				if (!pFontRow ||
					pFontRow->m_fRowEndX + nFullWidth + HORIZ_SPACING >= font->m_pTexture->m_rectFontArea.m_fX2)	// if we've run out of room on this row
				{
					pFontRow = sTextureGetEmptyFontRow(font->m_pTexture, nFontheight, nFullWidth, nRowId);
					
					//sFontTextureCheckRows(font->m_pTexture);

					//sFontTextureCheckCharRowIDs(font->m_pTexture);

					if (!pFontRow)
					{
						// CLEAN UP GDI STUFF
						if (bGDIInitialized)
						{
							DeleteObject(hGDIFont);
							DeleteObject(hGDIAsciiFont);
							DeleteObject(hBitMap);
							DeleteDC(hdc);
						}

						// ERROR MESSAGE
						ASSERTX(FALSE, "Failed to find space on font texture");

						return E_FAIL;
					}
					ASSERT_RETFAIL(nRowId != INVALID_ID);
				}
				
				RECT rcOut;
				SetRect(&rcOut, 1 - abc.abcA, 1, HORIZ_SPACING + nFullWidth, VERT_SPACING + nFontheight);

				// clear the space first
				Rectangle(hdc, 0, 0, nFontSize * 2, nFontSize * 2);

				// Draw the character in the DC using GDI
				int ret = DrawTextW(hdc, str, 1, &rcOut, DT_LEFT | DT_NOCLIP | DT_NOPREFIX);

				// save the info about this character
				pCharFrame->m_fU1 = (pFontRow->m_fRowEndX + 1.0f) / font->m_pTexture->m_fTextureWidth;
				pCharFrame->m_fV1 = (pFontRow->m_fRowY + 1.0f) / font->m_pTexture->m_fTextureHeight;
				pCharFrame->m_fU2 = (pFontRow->m_fRowEndX + (float)(abc.abcB) + 1.0f) / font->m_pTexture->m_fTextureWidth;
				pCharFrame->m_fV2 = (pFontRow->m_fRowY + (float)(nFontheight) + 1.0f) / font->m_pTexture->m_fTextureHeight;
				pCharFrame->m_fPreWidth = (float)abc.abcA * font->m_fWidthRatio;
				pCharFrame->m_fWidth = (float)abc.abcB * font->m_fWidthRatio;
				pCharFrame->m_fPostWidth = (float)abc.abcC * font->m_fWidthRatio;

				// copy the actual pixel values to the d3d texture
				D3DLOCKED_RECT tLockedRect;
				RECT tDestRect;
				SetRect( &tDestRect,
					(int)   pFontRow->m_fRowEndX,									// l
					(int)   pFontRow->m_fRowY,										// t
					(int) ( pFontRow->m_fRowEndX + nFullWidth  + HORIZ_SPACING ),	// r
					(int) ( pFontRow->m_fRowY    + nFontheight + VERT_SPACING  ) );	// b		

				ASSERT_RETFAIL( pFontRow->m_fRowEndX + nFullWidth  + HORIZ_SPACING < d3dtex->nWidthInPixels );
				ASSERT_RETFAIL( pFontRow->m_fRowY    + nFontheight + VERT_SPACING < d3dtex->nHeight );

				V_RETURN( dxC_MapTexture( d3dtex->pD3DTexture, 0, &tLockedRect ) );
				ASSERT_RETFAIL( dxC_pMappedRectData( tLockedRect ) );
#ifdef ENGINE_TARGET_DX9
				if ( d3dtex->nFormat == D3DFMT_A4R4G4B4 )
				{
					int srcpitch = nFontSize * 2;
					ASSERT( ( sizeof(WORD) * (int)font->m_pTexture->m_fTextureWidth ) == dxC_nMappedRectPitch( tLockedRect ) );
					for (int yy = 0; yy < nFontheight + VERT_SPACING; yy++)
					{
						DWORD* src = pdwMemPixelBuf + (yy * srcpitch);
						//DX9_BLOCK(  DWORD* dest = (DWORD*)(void*)tLockedRect.pBits + yy * (int)font->m_pTexture->m_fTextureWidth; )
						WORD* dest = (WORD*)(void*)dxC_pMappedRectData( tLockedRect ) + (((int)pFontRow->m_fRowY /*+ 1*/ + yy) * (int)font->m_pTexture->m_fTextureWidth) + ((int)pFontRow->m_fRowEndX /*+ 1*/);
						for (int xx = 0; xx < nFullWidth + HORIZ_SPACING; xx++)
						{
							*dest = (WORD)( 0x0fff + (((*src & 0xff) >> 4 ) << 12) );
							src++;
							dest++;
						}
					}
				}
				else
#endif
				{
					ASSERT_RETFAIL( d3dtex->nFormat == D3DFMT_A8R8G8B8 )
					int srcpitch = nFontSize * 2;
					ASSERT( ( sizeof(DWORD) * (int)font->m_pTexture->m_fTextureWidth ) == dxC_nMappedRectPitch( tLockedRect ) );
					for (int yy = 0; yy < nFontheight + VERT_SPACING; yy++)
					{
						DWORD* src = pdwMemPixelBuf + (yy * srcpitch);
						//DX9_BLOCK(  DWORD* dest = (DWORD*)(void*)tLockedRect.pBits + yy * (int)font->m_pTexture->m_fTextureWidth; )
						DWORD* dest = (DWORD*)(void*)dxC_pMappedRectData( tLockedRect ) + (((int)pFontRow->m_fRowY /*+ 1*/ + yy) * (int)font->m_pTexture->m_fTextureWidth) + ((int)pFontRow->m_fRowEndX /*+ 1*/);
						ASSERT_BREAK( dest && src );
						for (int xx = 0; xx < nFullWidth + HORIZ_SPACING; xx++)
						{
							ASSERT_BREAK( dest < (DWORD*)dxC_pMappedRectData( tLockedRect ) + d3dtex->nWidthInPixels * d3dtex->nHeight );
							*dest = 0x00ffffff + ((*src & 0xff) << 24);
							src++;
							dest++;
						}
					}
				}

				V_RETURN( dxC_UnmapTexture( d3dtex->pD3DTexture, 0 ) );

				// Update the font row information
				pFontRow->m_fRowEndX += nFullWidth + HORIZ_SPACING;
				//pFontRow->m_fRowEndX += 1.0f;

				int nNewId = INVALID_ID;
				UI_TEXTURE_FONT_FRAME **ppChar = pFontRow->m_arrFontCharacters.Add(&nNewId, TRUE);
				ASSERT_RETFAIL(ppChar);
				*ppChar = pCharFrame;
				pCharFrame->m_nTextureFontRowId = nRowId;

			}

			ASSERT_RETFAIL(pCharFrame->m_nTextureFontRowId != INVALID_ID);
			UIX_TEXTURE_FONT_ROW * pCharRow = font->m_pTexture->m_arrFontRows.Get(pCharFrame->m_nTextureFontRowId);
			ASSERT_RETFAIL(pCharRow);
			pCharRow->m_dwLastAccess = AppCommonGetRealTime();
		}

		wcCurrent++;
	}

	// release the DC and its parts (BMP freed by geometry list)
	if (bGDIInitialized)
	{
		DeleteObject(hGDIFont);
		DeleteObject(hGDIAsciiFont);
		DeleteObject(hBitMap);
		DeleteDC(hdc);
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

inline void AddScreenVertexEx(
	UI_SCREENVERTEX* vertex,
	float fX,
	float fY,
	float fU,
	float fV,
	float fZDelta,
	DWORD dwColor)
{
	vertex->vPosition.x = fX;
	vertex->vPosition.y = fY;
	vertex->vPosition.z = gfGlobalUIZ + fZDelta;
	vertex->dwColor = dwColor;
	vertex->pfTextureCoords0[0] = fU;
	vertex->pfTextureCoords0[1] = fV;
}

inline void AddScreenVertexEx(
	UI_SIMPLE_SCREEN_VERTEX* vertex,
	float fX,
	float fY,
	float fZDelta,
	DWORD dwColor)
{
	vertex->vPosition.x = fX;
	vertex->vPosition.y = fY;
	vertex->vPosition.z = gfGlobalUIZ + fZDelta;
	vertex->dwColor = dwColor;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float e_UIConvertAbsoluteZToDeltaZ(
	float fAbsZ )
{
	return fAbsZ - gfGlobalUIZ;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_UIWriteElementFrame(
	int nLocalBufferID,
	UI_RECT & drawrect,
	UI_TEXRECT & texrect,
	float fZDelta,
	DWORD dwColor )
{
	UI_LOCALBUFFER * pLocal = dx9_GetUILocalBuffer( nLocalBufferID );
	ASSERT_RETINVALIDARG( pLocal );

	int idx = pLocal->m_nLocalVertexBufferCurCount;

	if (pLocal->m_nLocalIndexBufferCurCount + 6 > pLocal->m_nLocalIndexBufferMaxCount ||
		pLocal->m_nLocalVertexBufferCurCount + 4 > pLocal->m_nLocalVertexBufferMaxCount)
	{
		return S_FALSE;
	}

	UI_SCREENVERTEX* vertex = sGetLocalBufferScreenVertexBuffer( pLocal, idx );
	ASSERT_RETFAIL( vertex );

	AddScreenVertexEx(vertex + 0, drawrect.m_fX1, drawrect.m_fY1, texrect.m_fU1, texrect.m_fV1, fZDelta, dwColor);	// UL
	AddScreenVertexEx(vertex + 1, drawrect.m_fX2, drawrect.m_fY1, texrect.m_fU2, texrect.m_fV1, fZDelta, dwColor);	// UR
	AddScreenVertexEx(vertex + 2, drawrect.m_fX1, drawrect.m_fY2, texrect.m_fU1, texrect.m_fV2, fZDelta, dwColor);	// LL
	AddScreenVertexEx(vertex + 3, drawrect.m_fX2, drawrect.m_fY2, texrect.m_fU2, texrect.m_fV2, fZDelta, dwColor);	// LR
	pLocal->m_nLocalVertexBufferCurCount += 4;
	ASSERT_RETFAIL(pLocal->m_nLocalVertexBufferCurCount <= pLocal->m_nLocalVertexBufferMaxCount);

	UI_INDEX_TYPE* index = pLocal->m_pLocalIndexBuffer + pLocal->m_nLocalIndexBufferCurCount;
	index[0] = idx + 0;
	index[1] = idx + 1;
	index[2] = idx + 2;
	index[3] = idx + 1;
	index[4] = idx + 3;
	index[5] = idx + 2;
	pLocal->m_nLocalIndexBufferCurCount += 6;
	ASSERT_RETFAIL(pLocal->m_nLocalIndexBufferCurCount <= pLocal->m_nLocalIndexBufferMaxCount);

	return S_OK;
}

////----------------------------------------------------------------------------
////----------------------------------------------------------------------------
//
//PRESULT e_UIWriteRotatedFrame(
//	int nLocalBufferID,
//	UI_RECT & drawrect,
//	UI_TEXRECT & texrect,
//	float fZDelta,
//	DWORD dwColor,
//	float fRotateCenterX,
//	float fRotateCenterY,
//	float fAngle )
//{
//	UI_LOCALBUFFER * pLocal = dx9_GetUILocalBuffer( nLocalBufferID );
//	ASSERT_RETINVALIDARG( pLocal );
//
//	int idx = pLocal->m_nLocalVertexBufferCurCount;
//
//	if (pLocal->m_nLocalIndexBufferCurCount + 6 > pLocal->m_nLocalIndexBufferMaxCount ||
//		pLocal->m_nLocalVertexBufferCurCount + 4 > pLocal->m_nLocalVertexBufferMaxCount)
//	{
//		return S_FALSE;
//	}
//
//	UI_SCREENVERTEX* vertex = sGetLocalBufferScreenVertexBuffer( pLocal, idx );
//	ASSERT_RETFAIL( vertex );
//
//	AddScreenVertexEx(vertex + 0, drawrect.m_fX1, drawrect.m_fY1, texrect.m_fU1, texrect.m_fV1, fZDelta, dwColor);	// UL
//	AddScreenVertexEx(vertex + 1, drawrect.m_fX2, drawrect.m_fY1, texrect.m_fU2, texrect.m_fV1, fZDelta, dwColor);	// UR
//	AddScreenVertexEx(vertex + 2, drawrect.m_fX1, drawrect.m_fY2, texrect.m_fU1, texrect.m_fV2, fZDelta, dwColor);	// LL
//	AddScreenVertexEx(vertex + 3, drawrect.m_fX2, drawrect.m_fY2, texrect.m_fU2, texrect.m_fV2, fZDelta, dwColor);	// LR
//
//	// perform rotation
//	float sinvalue = sinf(fAngle);
//	float cosvalue = cosf(fAngle);
//
//	float centerx = fRotateCenterX;  // already multiplied by ratio
//	float centery = fRotateCenterY;
//
//	for (int ii = 0; ii < 4; ii++)
//	{
//		float deltax = centerx - vertex[ii].vPosition.x;
//		float deltay = centery - vertex[ii].vPosition.y;
//		vertex[ii].vPosition.x = centerx + (deltax * cosvalue) - (deltay * sinvalue);
//		vertex[ii].vPosition.y = centery + (deltay * cosvalue) + (deltax * sinvalue);
//	}
//
//	pLocal->m_nLocalVertexBufferCurCount += 4;
//	ASSERT_RETFAIL(pLocal->m_nLocalVertexBufferCurCount <= pLocal->m_nLocalVertexBufferMaxCount);
//
//	UI_INDEX_TYPE* index = pLocal->m_pLocalIndexBuffer + pLocal->m_nLocalIndexBufferCurCount;
//	index[0] = idx + 0;
//	index[1] = idx + 1;
//	index[2] = idx + 2;
//	index[3] = idx + 1;
//	index[4] = idx + 3;
//	index[5] = idx + 2;
//	pLocal->m_nLocalIndexBufferCurCount += 6;
//	ASSERT_RETFAIL(pLocal->m_nLocalIndexBufferCurCount <= pLocal->m_nLocalIndexBufferMaxCount);
//
//	return S_OK;
//}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_UIWriteRotatedFrame(
	int nLocalBufferID,
	UI_RECT & drawrect,
	float fOrigWidth,
	float fOrigHeight,
	UI_TEXRECT & texrect,
	float fZDelta,
	DWORD dwColor,
	float fRotateCenterX,
	float fRotateCenterY,
	float fAngle )
{
	UI_LOCALBUFFER * pLocal = dx9_GetUILocalBuffer( nLocalBufferID );
	ASSERT_RETINVALIDARG( pLocal );

	int idx = pLocal->m_nLocalVertexBufferCurCount;

	if (pLocal->m_nLocalIndexBufferCurCount + 6 > pLocal->m_nLocalIndexBufferMaxCount ||
		pLocal->m_nLocalVertexBufferCurCount + 4 > pLocal->m_nLocalVertexBufferMaxCount)
	{
		return S_FALSE;
	}

	// find the scale  
	float fScaleX = (drawrect.m_fX2 - drawrect.m_fX1) / fOrigWidth;
	float fScaleY = (drawrect.m_fY2 - drawrect.m_fY1) / fOrigHeight;

	// translate to the rotation center
	drawrect.m_fX1 -= fRotateCenterX;
	drawrect.m_fX2 -= fRotateCenterX;
	drawrect.m_fY1 -= fRotateCenterY;
	drawrect.m_fY2 -= fRotateCenterY;

	drawrect.m_fX1 /= fScaleX;
	drawrect.m_fX2 /= fScaleX;
	drawrect.m_fY1 /= fScaleY;
	drawrect.m_fY2 /= fScaleY;

	float sinvalue = sinf(fAngle);
	float cosvalue = cosf(fAngle);

	VECTOR2 v[4];
	v[0].x = drawrect.m_fX1; v[0].y = drawrect.m_fY1; 
	v[1].x = drawrect.m_fX2; v[1].y = drawrect.m_fY1; 
	v[2].x = drawrect.m_fX1; v[2].y = drawrect.m_fY2; 
	v[3].x = drawrect.m_fX2; v[3].y = drawrect.m_fY2; 
	for (int ii = 0; ii < 4; ii++)
	{
		// perform rotation
		float deltax = v[ii].x;
		float deltay = v[ii].y;
		v[ii].x = (deltax * cosvalue) - (deltay * sinvalue);
		v[ii].y = (deltay * cosvalue) + (deltax * sinvalue);

		// scale this point back
		v[ii].x *= fScaleX;
		v[ii].y *= fScaleY;

		//translate back
		v[ii].x += fRotateCenterX;
		v[ii].y += fRotateCenterY;
	}

	UI_SCREENVERTEX* vertex = sGetLocalBufferScreenVertexBuffer( pLocal, idx );
	ASSERT_RETFAIL( vertex );

	AddScreenVertexEx(vertex + 0, v[0].x, v[0].y, texrect.m_fU1, texrect.m_fV1, fZDelta, dwColor);	// UL
	AddScreenVertexEx(vertex + 1, v[1].x, v[1].y, texrect.m_fU2, texrect.m_fV1, fZDelta, dwColor);	// UR
	AddScreenVertexEx(vertex + 2, v[2].x, v[2].y, texrect.m_fU1, texrect.m_fV2, fZDelta, dwColor);	// LL
	AddScreenVertexEx(vertex + 3, v[3].x, v[3].y, texrect.m_fU2, texrect.m_fV2, fZDelta, dwColor);	// LR

	//// perform rotation
	//float sinvalue = sinf(fAngle);
	//float cosvalue = cosf(fAngle);

	//for (int ii = 0; ii < 4; ii++)
	//{
	//	float deltax = centerx - vertex[ii].vPosition.x;
	//	float deltay = centery - vertex[ii].vPosition.y;
	//	vertex[ii].vPosition.x = centerx + (deltax * cosvalue) - (deltay * sinvalue);
	//	vertex[ii].vPosition.y = centery + (deltay * cosvalue) + (deltax * sinvalue);
	//}

	pLocal->m_nLocalVertexBufferCurCount += 4;
	ASSERT_RETFAIL(pLocal->m_nLocalVertexBufferCurCount <= pLocal->m_nLocalVertexBufferMaxCount);

	UI_INDEX_TYPE* index = pLocal->m_pLocalIndexBuffer + pLocal->m_nLocalIndexBufferCurCount;
	index[0] = idx + 0;
	index[1] = idx + 1;
	index[2] = idx + 2;
	index[3] = idx + 1;
	index[4] = idx + 3;
	index[5] = idx + 2;
	pLocal->m_nLocalIndexBufferCurCount += 6;
	ASSERT_RETFAIL(pLocal->m_nLocalIndexBufferCurCount <= pLocal->m_nLocalIndexBufferMaxCount);

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_UIWriteElementText(
	int nLocalBufferID,
	const WCHAR wc,
	int nFontSize,
	float x1, 
	float y1, 
	float x2, 
	float y2,
	float fZDelta,
	DWORD dwColor,
	UI_RECT & cliprect,
	UIX_TEXTURE_FONT * font)
{
	UI_LOCALBUFFER * pLocal = dx9_GetUILocalBuffer( nLocalBufferID );
	ASSERT_RETINVALIDARG( pLocal );

	int idx = pLocal->m_nLocalVertexBufferCurCount;

	UI_SCREENVERTEX* vertex = sGetLocalBufferScreenVertexBuffer( pLocal, idx );
	ASSERT_RETFAIL( vertex );

	UI_TEXTURE_FONT_FRAME* chrframe = font->GetChar(wc, nFontSize);
	ASSERT_RETVAL(chrframe, -1);
	UI_RECT drawrect(x1, y1, x2, y2);
	UI_TEXRECT texrect(chrframe->m_fU1, chrframe->m_fV1, chrframe->m_fU2, chrframe->m_fV2);
	if (!e_UIElementDoClip(&drawrect, &texrect, &cliprect))
	{
		return S_FALSE;
	}

	if (pLocal->m_nLocalIndexBufferCurCount + 6 > pLocal->m_nLocalIndexBufferMaxCount ||
		pLocal->m_nLocalVertexBufferCurCount + 4 > pLocal->m_nLocalVertexBufferMaxCount)
	{
		return S_FALSE;
	}

	AddScreenVertexEx(vertex + 0, drawrect.m_fX1, drawrect.m_fY1, texrect.m_fU1, texrect.m_fV1, fZDelta, dwColor);	// UL
	AddScreenVertexEx(vertex + 1, drawrect.m_fX2, drawrect.m_fY1, texrect.m_fU2, texrect.m_fV1, fZDelta, dwColor);	// UR
	AddScreenVertexEx(vertex + 2, drawrect.m_fX1, drawrect.m_fY2, texrect.m_fU1, texrect.m_fV2, fZDelta, dwColor);	// LL
	AddScreenVertexEx(vertex + 3, drawrect.m_fX2, drawrect.m_fY2, texrect.m_fU2, texrect.m_fV2, fZDelta, dwColor);	// LR
	vertex += 4;
	pLocal->m_nLocalVertexBufferCurCount += 4;
	ASSERT_RETFAIL(pLocal->m_nLocalVertexBufferCurCount <= pLocal->m_nLocalVertexBufferMaxCount);

	UI_INDEX_TYPE* index = pLocal->m_pLocalIndexBuffer + pLocal->m_nLocalIndexBufferCurCount;
	index[0] = idx + 0;
	index[1] = idx + 1;
	index[2] = idx + 2;
	index[3] = idx + 1;
	index[4] = idx + 3;
	index[5] = idx + 2;
	index += 6;
	pLocal->m_nLocalIndexBufferCurCount += 6;
	ASSERT_RETFAIL(pLocal->m_nLocalIndexBufferCurCount <= pLocal->m_nLocalIndexBufferMaxCount);

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_UIWriteElementTri(
	int nLocalBufferID,
	const UI_TEXTURETRI & tri,
	float fZDelta,
	DWORD dwColor )
{
	UI_LOCALBUFFER * pLocal = dx9_GetUILocalBuffer( nLocalBufferID );
	ASSERT_RETFAIL( pLocal );

	int idx = pLocal->m_nLocalVertexBufferCurCount;

	if (pLocal->m_nLocalIndexBufferCurCount + 3 > pLocal->m_nLocalIndexBufferMaxCount ||
		pLocal->m_nLocalVertexBufferCurCount + 3 > pLocal->m_nLocalVertexBufferMaxCount)
	{
		return S_FALSE;
	}

	UI_SCREENVERTEX* vertex = sGetLocalBufferScreenVertexBuffer( pLocal, idx );
	ASSERT_RETFAIL( vertex );

	AddScreenVertexEx(vertex + 0, tri.x1, tri.y1, tri.u1, tri.v1, fZDelta, dwColor);	// UL
	AddScreenVertexEx(vertex + 1, tri.x2, tri.y2, tri.u2, tri.v2, fZDelta, dwColor);	// UR
	AddScreenVertexEx(vertex + 2, tri.x3, tri.y3, tri.u3, tri.v3, fZDelta, dwColor);	// LL
	pLocal->m_nLocalVertexBufferCurCount += 3;
	ASSERT_RETFAIL(pLocal->m_nLocalVertexBufferCurCount <= pLocal->m_nLocalVertexBufferMaxCount);

	UI_INDEX_TYPE* index = pLocal->m_pLocalIndexBuffer + pLocal->m_nLocalIndexBufferCurCount;
	index[0] = idx + 0;
	index[1] = idx + 1;
	index[2] = idx + 2;
	pLocal->m_nLocalIndexBufferCurCount += 3;
	ASSERT_RETFAIL(pLocal->m_nLocalIndexBufferCurCount <= pLocal->m_nLocalIndexBufferMaxCount);

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_UIWriteElementRect(
	int nLocalBufferID,
	const UI_TEXTURERECT & tRect,
	float fZDelta,
	DWORD dwColor )
{
	UI_LOCALBUFFER * pLocal = dx9_GetUILocalBuffer( nLocalBufferID );
	ASSERT_RETFAIL( pLocal );

	int idx = pLocal->m_nLocalVertexBufferCurCount;

	if (pLocal->m_nLocalIndexBufferCurCount + 6 > pLocal->m_nLocalIndexBufferMaxCount ||
		pLocal->m_nLocalVertexBufferCurCount + 4 > pLocal->m_nLocalVertexBufferMaxCount)
	{
		return S_FALSE;
	}

	UI_SCREENVERTEX* vertex = sGetLocalBufferScreenVertexBuffer( pLocal, idx );
	ASSERT_RETFAIL( vertex );

	AddScreenVertexEx(vertex + 0, tRect.x1, tRect.y1, tRect.u1, tRect.v1, fZDelta, dwColor);	// UL
	AddScreenVertexEx(vertex + 1, tRect.x2, tRect.y2, tRect.u2, tRect.v2, fZDelta, dwColor);	// UR
	AddScreenVertexEx(vertex + 2, tRect.x3, tRect.y3, tRect.u3, tRect.v3, fZDelta, dwColor);	// LL
	AddScreenVertexEx(vertex + 3, tRect.x4, tRect.y4, tRect.u4, tRect.v4, fZDelta, dwColor);	// LR

	pLocal->m_nLocalVertexBufferCurCount += 4;
	ASSERT_RETFAIL(pLocal->m_nLocalVertexBufferCurCount <= pLocal->m_nLocalVertexBufferMaxCount);

	UI_INDEX_TYPE* index = pLocal->m_pLocalIndexBuffer + pLocal->m_nLocalIndexBufferCurCount;
	index[0] = idx + 0;
	index[1] = idx + 1;
	index[2] = idx + 2;
	index[3] = idx + 2;
	index[4] = idx + 3;
	index[5] = idx + 0;
	pLocal->m_nLocalIndexBufferCurCount += 6;
	ASSERT_RETFAIL(pLocal->m_nLocalIndexBufferCurCount <= pLocal->m_nLocalIndexBufferMaxCount);

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_UIWriteElementQuad(
	int nLocalBufferID,
	const UI_QUAD tQuad,
	float fZDelta,
	DWORD dwColor )
{
	UI_LOCALBUFFER * pLocal = dx9_GetUILocalBuffer( nLocalBufferID );
	ASSERT_RETFAIL( pLocal );

	int idx = pLocal->m_nLocalVertexBufferCurCount;

	if (pLocal->m_nLocalIndexBufferCurCount + 6 > pLocal->m_nLocalIndexBufferMaxCount ||
		pLocal->m_nLocalVertexBufferCurCount + 4 > pLocal->m_nLocalVertexBufferMaxCount)
	{
		return S_FALSE;
	}

	UI_SIMPLE_SCREEN_VERTEX* vertex = sGetLocalBufferSimpleScreenVertexBuffer( pLocal, idx );
	ASSERT_RETFAIL( vertex );

	AddScreenVertexEx(vertex + 0, tQuad.x1, tQuad.y1, fZDelta, dwColor);	// UL
	AddScreenVertexEx(vertex + 1, tQuad.x2, tQuad.y2, fZDelta, dwColor);	// UR
	AddScreenVertexEx(vertex + 2, tQuad.x3, tQuad.y3, fZDelta, dwColor);	// LL
	AddScreenVertexEx(vertex + 3, tQuad.x4, tQuad.y4, fZDelta, dwColor);	// LR

	pLocal->m_nLocalVertexBufferCurCount += 4;
	ASSERT_RETFAIL(pLocal->m_nLocalVertexBufferCurCount <= pLocal->m_nLocalVertexBufferMaxCount);

	UI_INDEX_TYPE* index = pLocal->m_pLocalIndexBuffer + pLocal->m_nLocalIndexBufferCurCount;
	index[0] = idx + 0;
	index[1] = idx + 1;
	index[2] = idx + 2;
	index[3] = idx + 1;
	index[4] = idx + 3;
	index[5] = idx + 2;
	pLocal->m_nLocalIndexBufferCurCount += 6;
	ASSERT_RETFAIL(pLocal->m_nLocalIndexBufferCurCount <= pLocal->m_nLocalIndexBufferMaxCount);

	return S_OK;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_UIWriteElementQuadStrip(
	int nLocalBufferID,
	const UI_QUADSTRIP & tPoints,
	float fZDelta,
	DWORD dwColor )
{
	UI_LOCALBUFFER * pLocal = dx9_GetUILocalBuffer( nLocalBufferID );
	ASSERT_RETFAIL( pLocal );

	int idx = pLocal->m_nLocalVertexBufferCurCount;

	UI_SIMPLE_SCREEN_VERTEX* vertex = sGetLocalBufferSimpleScreenVertexBuffer( pLocal, idx );
	ASSERT_RETFAIL( vertex );
	UI_INDEX_TYPE* index = pLocal->m_pLocalIndexBuffer + pLocal->m_nLocalIndexBufferCurCount;

	int nSegments = tPoints.nPointCount / 2;

	// seed with first segment of strip
	const VECTOR2 * pvP = &tPoints.vPoints[ 0 ];
	AddScreenVertexEx(vertex + 0, pvP[ 0 ].x, pvP[ 0 ].y, fZDelta, dwColor);
	AddScreenVertexEx(vertex + 1, pvP[ 1 ].x, pvP[ 1 ].y, fZDelta, dwColor);
	pLocal->m_nLocalVertexBufferCurCount += 2;
	ASSERT_RETFAIL(pLocal->m_nLocalVertexBufferCurCount <= pLocal->m_nLocalVertexBufferMaxCount);
	vertex += 2;

	for ( int nSeg = 1; nSeg < nSegments; nSeg++ )
	{
		const VECTOR2 * pvP = &tPoints.vPoints[ nSeg * 2 ];
		AddScreenVertexEx(vertex + 0, pvP[ 0 ].x, pvP[ 0 ].y, fZDelta, dwColor);
		AddScreenVertexEx(vertex + 1, pvP[ 1 ].x, pvP[ 1 ].y, fZDelta, dwColor);

		pLocal->m_nLocalVertexBufferCurCount += 2;
		ASSERT(pLocal->m_nLocalVertexBufferCurCount <= pLocal->m_nLocalVertexBufferMaxCount);
		vertex += 2;

		index[0] = idx + 1;
		index[1] = idx + 0;
		index[2] = idx + 2;
		index[3] = idx + 1;
		index[4] = idx + 2;
		index[5] = idx + 3;
		pLocal->m_nLocalIndexBufferCurCount += 6;
		ASSERT(pLocal->m_nLocalIndexBufferCurCount <= pLocal->m_nLocalIndexBufferMaxCount);
		idx += 2;
		index += 6;
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_UITrashEngineBuffer( int & nEngineBuffer )
{
	if ( nEngineBuffer == INVALID_ID )
		return E_INVALIDARG;
	UI_D3DBUFFER * pEngine = dx9_GetUIEngineBuffer( nEngineBuffer );

	//ASSERT( pEngine );

	// CML 2007.05.10: The engine buffer specified may have been removed from a call to dx9_UIReleaseVBuffers.
	if ( pEngine )
	{
		// create a ref on the garbage list
		HashAdd( gtEngineBuffersGarbage, pEngine->nId );
	}

	nEngineBuffer = INVALID_ID;

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_UITextureDrawClearLocalBufferCounts( int nLocalBuffer )
{
	UI_LOCALBUFFER * pLocal = dx9_GetUILocalBuffer( nLocalBuffer );
	ASSERT_RETINVALIDARG( pLocal );

	pLocal->m_nLocalVertexBufferCurCount = 0;
	pLocal->m_nLocalIndexBufferCurCount = 0;

	return S_OK;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_UIPrimitiveDrawClearLocalBufferCounts(
	int nLocalBuffer )
{
	UI_LOCALBUFFER * pLocal = dx9_GetUILocalBuffer( nLocalBuffer );
	ASSERT_RETINVALIDARG( pLocal );

	pLocal->m_nLocalVertexBufferCurCount = 0;
	pLocal->m_nLocalIndexBufferCurCount = 0;

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_UIGetNewLocalBuffer( int& nID )
{
	HashAdd( gtUILocalBuffers, gnLocalBuffersNextID );
	gnLocalBuffersNextID++;
	nID = gnLocalBuffersNextID - 1;
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// free what's left on the garbage lists
PRESULT e_UICleanupUnused()
{
	UI_D3DBUFFER_REF * pRef = HashGetFirst( gtEngineBuffersGarbage );
	UI_D3DBUFFER_REF * pNext = NULL;
	while ( pRef )
	{
		pNext = HashGetNext( gtEngineBuffersGarbage, pRef );
		V( sUIEngineBufferFree( pRef->nId ) );
		HashRemove( gtEngineBuffersGarbage, pRef->nId );
		pRef = pNext;
	}
	// should be redundant, but resets the linkage for better cache coherence
	HashClear( gtEngineBuffersGarbage );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// free all UI engine buffers
PRESULT e_UIFree()
{
	UI_D3DBUFFER * pBuf = HashGetFirst( gtUIEngineBuffers );
	UI_D3DBUFFER * pNext = NULL;
	while ( pBuf )
	{
		pNext = HashGetNext( gtUIEngineBuffers, pBuf );
		V( sUIEngineBufferFree( pBuf->nId ) );
		//HashRemove( gtUIEngineBuffers, pBuf->nId );
		pBuf = pNext;
	}
	HashClear( gtUIEngineBuffers );
	HashClear( gtEngineBuffersGarbage );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PRESULT e_UIInit()
{
	HashInit( gtUIEngineBuffers, NULL, 50 );
	HashInit( gtUILocalBuffers, NULL, 50 );
	HashInit( gtEngineBuffersGarbage, NULL, 50 );
	ArrayInit(gtUIDebugLabels, NULL, 1);
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PRESULT e_UIDestroy()
{
	V( e_UIFree() );
	HashFree( gtUIEngineBuffers );
	HashFree( gtUILocalBuffers );
	HashFree( gtEngineBuffersGarbage );
	gtUIDebugLabels.Destroy();
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PRESULT e_UITextureRender(
	int nTextureID,
	int nEngineBuffer,
	int nLocalBuffer,
	BOOL bOpaque,
	BOOL bGrayout)
{
#if defined(ENGINE_TARGET_DX9)
	if (dx9_DeviceLost() == TRUE)
	{
		return S_FALSE;
	}
#endif

	V( sUISetSamplerStates() );

	ASSERT_RETFAIL(nTextureID != INVALID_ID);	// for now, only draw textured geometry

	// let async-loaded UI textures finalize here
	BOOL bLoaded = e_TextureIsLoaded( nTextureID, TRUE );

	D3D_TEXTURE* d3dtex = dxC_TextureGet(nTextureID);
	ASSERT_RETFAIL(d3dtex);

	UI_D3DBUFFER* d3dbuf = dx9_GetUIEngineBuffer( nEngineBuffer );
	if (!d3dbuf)
	{
		return S_FALSE;
	}

	UI_LOCALBUFFER* localbuf = dx9_GetUILocalBuffer( nLocalBuffer );
	if (!localbuf)
	{
		return S_FALSE;
	}

	if (localbuf->m_nLocalVertexBufferCurCount <= 0 ||
		localbuf->m_nLocalIndexBufferCurCount <= 0)
	{
		return S_FALSE;
	}

	int nEffectID = dx9_EffectGetNonMaterialID( NONMATERIAL_EFFECT_UI );
	ASSERT_RETFAIL( nEffectID != INVALID_ID );
	D3D_EFFECT * pEffect = dxC_EffectGet( nEffectID );
	ASSERT_RETFAIL( pEffect );

	TECHNIQUE_FEATURES tFeat;
	tFeat.Reset();
	tFeat.nInts[ TF_INT_INDEX ] = localbuf->m_eLocalVertexType + ( 2 * (int)bGrayout );
	EFFECT_TECHNIQUE * ptTechnique;
	V_RETURN( dxC_EffectGetTechnique( nEffectID, tFeat, &ptTechnique, &sgtUITechniqueCache ) );
	ASSERT_RETFAIL( ptTechnique );

	UINT nPasses;
	V_RETURN( dxC_SetTechnique( pEffect, ptTechnique, &nPasses ) );
	ASSERT( nPasses == 1 );
	V_RETURN( dxC_EffectBeginPass( pEffect, 0 ) );

	dx9_ReportSetTexture( d3dtex->nId );
	V( dx9_EffectSetTexture( pEffect, *ptTechnique, EFFECT_PARAM_DIFFUSEMAP0, d3dtex->GetShaderResourceView() ) );

	V( dxC_TextureDidSet( d3dtex ) );

	V( dxC_EffectSetScreenSize( pEffect, *ptTechnique ) );

	D3DC_PRIMITIVE_TOPOLOGY d3dPrim = D3DPT_TRIANGLELIST;

	// render the vertex buffer contents
	ASSERT_RETFAIL( d3dbuf->m_eVertexDeclType != VERTEX_DECL_INVALID );
	V( dxC_SetVertexDeclaration( d3dbuf->m_eVertexDeclType, pEffect ) );
	V( dxC_SetStreamSource(0, sUIEngineBufferGetRenderVertexBuffer(d3dbuf), 0, SIZE_TO_UINT(sgnSizeOfVertex[ localbuf->m_eLocalVertexType ])) );

	V( dxC_SetIndices(d3dbuf->m_tIndexBufferDef, TRUE) );

	V_RETURN( dxC_DrawIndexedPrimitive(d3dPrim, 0, 0, localbuf->m_nLocalVertexBufferCurCount, 0, DP_GETPRIMCOUNT_TRILIST( localbuf->m_nLocalIndexBufferCurCount ), pEffect, METRICS_GROUP_UI ) );
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_UIPrimitiveRender(
	int nEngineBuffer,
	int nLocalBuffer,
	DWORD dwDrawFlags )
{
#if defined(ENGINE_TARGET_DX9)
	if (dx9_DeviceLost() == TRUE)
	{
		return S_FALSE;
	}

	//sUISetSamplerStates();
#endif
	UI_D3DBUFFER* d3dbuf = dx9_GetUIEngineBuffer( nEngineBuffer );
	if ( !d3dbuf || !dxC_IndexBufferD3DExists( d3dbuf->m_tIndexBufferDef.nD3DBufferID ) || !sUIEngineBufferGetRenderVertexBuffer(d3dbuf) )
	{
		return S_FALSE;
	}

	UI_LOCALBUFFER* localbuf = dx9_GetUILocalBuffer( nLocalBuffer );
	if (!localbuf)
	{
		return S_FALSE;
	}

	if ( localbuf->m_nLocalVertexBufferCurCount <= 0 ||
		 localbuf->m_nLocalIndexBufferCurCount <= 0 )
	{
		return S_FALSE;
	}


	int nEffectID = dx9_EffectGetNonMaterialID( NONMATERIAL_EFFECT_UI );
	ASSERT_RETFAIL( nEffectID != INVALID_ID );
	D3D_EFFECT * pEffect = dxC_EffectGet( nEffectID );
	ASSERT_RETFAIL( pEffect );

	TECHNIQUE_FEATURES tFeat;
	tFeat.Reset();
	tFeat.nInts[ TF_INT_INDEX ] = localbuf->m_eLocalVertexType;
	EFFECT_TECHNIQUE * ptTechnique;
	V_RETURN( dxC_EffectGetTechnique( nEffectID, tFeat, &ptTechnique, &sgtUITechniqueCache ) );
	ASSERT_RETFAIL( ptTechnique );

	UINT nPasses;
	V_RETURN( dxC_SetTechnique( pEffect, ptTechnique, &nPasses ) );
	ASSERT( nPasses == 1 );
	V_RETURN( dxC_EffectBeginPass( pEffect, 0 ) );

	V( dxC_EffectSetScreenSize( pEffect, *ptTechnique ) );


	V( dxC_SetRenderState( D3DRS_ZWRITEENABLE,		!!(dwDrawFlags & UI_PRIM_ZWRITE) ) );
	V( dxC_SetRenderState( D3DRS_ZENABLE,			!!(dwDrawFlags & UI_PRIM_ZTEST) ) );
	V( dxC_SetRenderState( D3DRS_ALPHABLENDENABLE,	!!(dwDrawFlags & UI_PRIM_ALPHABLEND) ) );

	D3DC_PRIMITIVE_TOPOLOGY d3dPrim = D3DPT_TRIANGLELIST;

	// render the vertex buffer contents
	ASSERT_RETFAIL(d3dbuf->m_eVertexDeclType);
	V( dxC_SetVertexDeclaration( d3dbuf->m_eVertexDeclType, pEffect ) );
	V( dxC_SetStreamSource(0, sUIEngineBufferGetRenderVertexBuffer(d3dbuf), 0, SIZE_TO_UINT(sgnSizeOfVertex[ localbuf->m_eLocalVertexType ])) );

	V( dxC_SetIndices( d3dbuf->m_tIndexBufferDef, TRUE ) );

	V_RETURN( dxC_DrawIndexedPrimitive(d3dPrim, 0, 0, localbuf->m_nLocalVertexBufferCurCount, 0, DP_GETPRIMCOUNT_TRILIST( localbuf->m_nLocalIndexBufferCurCount), pEffect, METRICS_GROUP_UI ) );

	V( dxC_SetRenderState( D3DRS_ZWRITEENABLE,		D3DZB_TRUE ) );
	V( dxC_SetRenderState( D3DRS_ZENABLE,			D3DZB_TRUE ) );
	V( dxC_SetRenderState( D3DRS_ALPHABLENDENABLE,	TRUE ) );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sUISetSamplerStates()
{
	if ( dxC_IsPixomaticActive() )
		return S_FALSE;

#ifdef ENGINE_TARGET_DX9
	V( dx9_SetSamplerState( 0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP ) );
	V( dx9_SetSamplerState( 0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP ) );
	V( dx9_SetSamplerState( 0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR ) );
	V( dx9_SetSamplerState( 0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR ) );
	V( dx9_SetSamplerState( 0, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR ) );
	V( dx9_SetSamplerState( 0, D3DSAMP_SRGBTEXTURE, FALSE ) );
#endif
#ifndef DX10_GET_RUNNING_HACK
	DX10_BLOCK( ASSERTX( 0, "KMNV TODO: SetSamplerState" ); )
#endif
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sUISetStates()
{
	if ( dxC_IsPixomaticActive() )
		return E_NOTIMPL;

	// set wanted states
#ifdef ENGINE_TARGET_DX9
	V( dx9_SetVertexShader( NULL ) );
	V( dx9_SetPixelShader( NULL ) );
#endif
	//dxC_SetIndices(NULL);

	//dx9_SetDepthTarget( ZBUFFER_AUTO );
	//dxC_SetRenderTarget( RENDER_TARGET_BACKBUFFER );
	DEPTH_TARGET_INDEX eDT = dxC_GetSwapChainDepthTargetIndex( dxC_GetCurrentSwapChainIndex(), SWAP_DT_AUTO );
	V( dxC_SetRenderTargetWithDepthStencil( SWAP_RT_FINAL_BACKBUFFER, eDT ) );


	dxC_SetRenderState(D3DRS_COLORWRITEENABLE,	D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA );
	dxC_SetRenderState(D3DRS_ALPHABLENDENABLE,	TRUE);
	dxC_SetRenderState(D3DRS_SRCBLEND,			D3DBLEND_SRCALPHA);
	dxC_SetRenderState(D3DRS_DESTBLEND,			D3DBLEND_INVSRCALPHA);
	dxC_SetRenderState(D3DRS_BLENDOP,				D3DBLENDOP_ADD);
	dxC_SetRenderState(D3DRS_ALPHATESTENABLE,		TRUE);
	dxC_SetRenderState(D3DRS_ALPHAREF,			1L);
	dxC_SetRenderState(D3DRS_ALPHAFUNC,			D3DCMP_GREATEREQUAL);
	dxC_SetRenderState(D3DRS_ZENABLE,				D3DZB_TRUE);
	dxC_SetRenderState(D3DRS_ZWRITEENABLE,		D3DZB_TRUE);
#ifdef ENGINE_TARGET_DX9
	V( dx9_SetSamplerState( 0, D3DSAMP_SRGBTEXTURE, FALSE ) );
	V( dx9_SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE) );
	V( dx9_SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE) );
	V( dx9_SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE) );
	V( dx9_SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE) );
	//V( dx9_SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE) );		// these two set just before render
	//V( dx9_SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE) );
	V( dxC_SetTexture(1, NULL) );
	V( dx9_SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE) );
	V( dx9_SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE) );
	//V( dxC_SetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME) );
#endif
#ifndef DX10_GET_RUNNING_HACK
	DX10_BLOCK( ASSERTX( 0, "KMNV TODO!"); )
#endif

	V( sUISetSamplerStates() );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sUIRender(
	void)
{
	V( sUISetStates() );

	e_UIRenderAll();

	//dxC_SetIndices(NULL);
	dxC_SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);

	dxC_SetRenderState(D3DRS_ALPHABLENDENABLE,	FALSE);
	dxC_SetRenderState(D3DRS_SRCBLEND,			D3DBLEND_ONE);
	dxC_SetRenderState(D3DRS_DESTBLEND,			D3DBLEND_ZERO);
	dxC_SetRenderState(D3DRS_BLENDOP,				D3DBLENDOP_ADD);
	dxC_SetRenderState(D3DRS_ALPHAREF,			1L);
	dxC_SetRenderState(D3DRS_ALPHAFUNC,			D3DCMP_GREATEREQUAL);
	dxC_SetRenderState(D3DRS_ZENABLE,				D3DZB_TRUE);
	dxC_SetRenderState(D3DRS_ZWRITEENABLE,		D3DZB_TRUE);
#ifdef ENGINE_TARGET_DX9
	V( dx9_SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_DISABLE) );
	V( dx9_SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_DISABLE) );
#endif
#ifndef DX10_GET_RUNNING_HACK
	DX10_BLOCK( ASSERTX(0, "KMNV TODO: dx9_SetTextureStageState " ); )
#endif

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_UIX_Render(
	void)
{
	D3D_PROFILE_REGION( L"UIX" );

	V_RETURN( sUIRender() );
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PRESULT dx9_UIReleaseVBuffers(
	void)
{
	// this hash walk hits all of the allocated engine buffers
	UI_D3DBUFFER * buf = HashGetFirst( gtUIEngineBuffers );

	while ( buf )
	{
		UI_D3DBUFFER* next = HashGetNext( gtUIEngineBuffers, buf );
		V( sUIEngineBufferFree( buf->nId ) );
		buf = next;

		//V( sUIEngineBufferFreeVertexBuffers(buf) );
		//buf->m_tVertexBufferDef.nVertexCount = 0;
		//V( dxC_IndexBufferReleaseResource( buf->m_tIndexBufferDef.nD3DBufferID ) );
		//buf->m_tIndexBufferDef.nD3DBufferID = INVALID_ID;
		//buf->m_tIndexBufferDef.nIndexCount = 0;
	}
	// ... so walking the garbage isn't necessary (it's all references to the gtUIEngineBuffers hash)

	ASSERT( HashGetCount( gtUIEngineBuffers ) == 0 );

	// Now move all engine buffers to the trash.
	HashClear( gtEngineBuffersGarbage );
	//for ( UI_D3DBUFFER * buf = HashGetFirst( gtUIEngineBuffers );
	//	buf;
	//	buf = HashGetNext( gtUIEngineBuffers, buf ) )
	//{
	//	int nBufferID = buf->nId;
	//	V( e_UITrashEngineBuffer( nBufferID ) );
	//}

	//ASSERT( HashGetCount( gtEngineBuffersGarbage ) == HashGetCount( gtUIEngineBuffers ) );


	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_UIAutomapRenderCallback( const UI_RECT & tScreenRect, DWORD dwColor, float fZDelta, float fScale )
{
	REGION * pRegion = e_GetCurrentRegion();
	if ( ! pRegion )
		return;

	float fZFinal = fZDelta + gfGlobalUIZ;

	E_RECT tRect;
	tRect.Set( (int)tScreenRect.m_fX1, (int)tScreenRect.m_fY1, (int)tScreenRect.m_fX2, (int)tScreenRect.m_fY2 );
	V( e_AutomapRenderDisplay( pRegion->nAutomapID, tRect, dwColor, fZFinal, fScale ) );

	// restore any states we changed
	V( sUISetStates() );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_UIElementGetCallbackFunc(
	const char * pszName,
	int nMaxLen,
	PFN_UI_RENDER_CALLBACK & pfnCallback )
{
	ASSERT_RETINVALIDARG( pszName );
	if ( ! pszName[0] )
		return S_FALSE;

	// list of UI callbacks could be a bit cleaner (like a table!)

	if ( ! PStrICmp( pszName, AUTOMAP_COMPONENT_NAME, nMaxLen ) )
	{
		// It's an automap!
		pfnCallback = e_UIAutomapRenderCallback;
		return S_OK;
	}

	return S_FALSE;
}
