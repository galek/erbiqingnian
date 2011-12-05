//----------------------------------------------------------------------------
// dxC_buffer.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DXC_BUFFER__
#define __DXC_BUFFER__

#include "refcount.h"
#include "dxC_resource.h"
#include "dxC_FVF.h"
#include "e_budget.h"
//#include "dxC_state.h"
#include "dxC_pixo.h"

#include "chb_timing.h"


using namespace FSSE;

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define DXC_MAX_VERTEX_STREAMS			4
#define VERTEX_BUFFER_FLAG_DEBUG		MAKE_MASK(0)
#define VERTEX_BUFFER_FLAG_SILHOUETTE	MAKE_MASK(1)
#define VERTEX_BUFFER_FLAG_FREE_LOCAL_DEPRECATED	MAKE_MASK(2)	// CHB 2006.06.27 - flag to free the local data when no longer needed

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

void dxC_VertexBufferDefinitionInit( struct D3D_VERTEX_BUFFER_DEFINITION & tVBDef );
void dxC_IndexBufferDefinitionInit( struct D3D_INDEX_BUFFER_DEFINITION & tIBDef );

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// offline/definition structures
// Note:  if you change either of these two definition structures, you must also increment the MODEL_FILE_VERSION number

struct D3D_VERTEX_BUFFER_DEFINITION
{
	int											nD3DBufferID[ DXC_MAX_VERTEX_STREAMS ];
	int											nBufferSize[ DXC_MAX_VERTEX_STREAMS ];		// Size in bytes of the vertex buffers
	DWORD										dwFlags;
	int											nVertexCount;
	VERTEX_DECL_TYPE							eVertexType;								// Vertex type for DirectX - lookup into tables of elements and decls
	D3DC_USAGE									tUsage;
	D3DC_FVF									dwFVF;										// Vertex format for DirectX - the simple way (bit flags)
	void*										pLocalData[ DXC_MAX_VERTEX_STREAMS ];
};

struct D3D_INDEX_BUFFER_DEFINITION
{
	int										nD3DBufferID;
	int										nBufferSize;		// Size in bytes of the index buffer
	int										nIndexCount;
	D3DC_USAGE								tUsage;
	DXC_FORMAT 								tFormat;			// index data format
	void*									pLocalData;
};

#define ZERO_VB_DEF		{ { INVALID_ID, INVALID_ID, INVALID_ID, INVALID_ID }, { 0, 0, 0, 0 }, 0, 0, VERTEX_DECL_INVALID, D3DC_USAGE_INVALID, D3DC_FVF_NULL, { NULL, NULL, NULL, NULL } }
#define ZERO_IB_DEF		{ INVALID_ID, 0, 0, D3DC_USAGE_INVALID, D3DFMT_UNKNOWN, NULL }

//----------------------------------------------------------------------------
// runtime structures held in a CHash

struct D3D_VERTEX_BUFFER
{
	// needed for CHash
	int							nId;
	D3D_VERTEX_BUFFER *			pNext;

	REFCOUNT					tRefCount;			// Reference count to help memory cleanup
	D3DC_USAGE					tUsage;				// for error-checking and debug reporting
	SPD3DCVB					pD3DVertices;		// D3D vertex buffer interface
	ENGINE_RESOURCE				tEngineRes;
};

struct D3D_INDEX_BUFFER
{
	// needed for CHash
	int							nId;
	D3D_INDEX_BUFFER *			pNext;

	REFCOUNT					tRefCount;			// Reference count to help memory cleanup
	D3DC_USAGE					tUsage;				// for error-checking and debug reporting
	SPD3DCIB 					pD3DIndices;		// D3D index buffer interface
	ENGINE_RESOURCE				tEngineRes;
};


//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

inline D3D_VERTEX_BUFFER * dxC_VertexBufferGet( int nID )
{
	ASSERT( nID != 0 );
	extern CHash<D3D_VERTEX_BUFFER> gtVertexBuffers;
	return HashGet( gtVertexBuffers, nID );
}

inline D3D_VERTEX_BUFFER * dxC_VertexBufferGetFirst()
{
	extern CHash<D3D_VERTEX_BUFFER> gtVertexBuffers;
	return HashGetFirst( gtVertexBuffers );
}

inline D3D_VERTEX_BUFFER * dxC_VertexBufferGetNext( D3D_VERTEX_BUFFER * pCurrent )
{
	ASSERT_RETNULL( pCurrent );
	extern CHash<D3D_VERTEX_BUFFER> gtVertexBuffers;
	return HashGetNext( gtVertexBuffers, pCurrent );
}

inline D3D_INDEX_BUFFER * dxC_IndexBufferGet( int nID )
{
	ASSERT( nID != 0 );
	extern CHash<D3D_INDEX_BUFFER> gtIndexBuffers;
	return HashGet( gtIndexBuffers, nID );
}

inline D3D_INDEX_BUFFER * dxC_IndexBufferGetFirst()
{
	extern CHash<D3D_INDEX_BUFFER> gtIndexBuffers;
	return HashGetFirst( gtIndexBuffers );
}

inline D3D_INDEX_BUFFER * dxC_IndexBufferGetNext( D3D_INDEX_BUFFER * pCurrent )
{
	ASSERT_RETNULL( pCurrent );
	extern CHash<D3D_INDEX_BUFFER> gtIndexBuffers;
	return HashGetNext( gtIndexBuffers, pCurrent );
}

//----------------------------------------------------------------------------

#ifdef ENGINE_TARGET_DX9
template <class T>
inline PRESULT dxC_UpdateBufferEx(
	LPD3DCBASEBUFFER pBuffer,
	void* pSysData,
	UINT offset,
	UINT size,
	BOOL bDiscard )
{
	void* pData = NULL;

	T pD3D9Buffer = (T)pBuffer;  
	V_RETURN( pD3D9Buffer->Lock( offset, size, &pData, bDiscard ? D3DLOCK_DISCARD : 0 ) );
	if( !pData )
		return E_FAIL;

	MemoryCopy( pData, size, pSysData, size );
	V_RETURN( pD3D9Buffer->Unlock() );

	return S_OK;
}
inline PRESULT dxC_UpdateBufferEx( LPD3DCVB pBuffer, void * pSysData, UINT offset, UINT size, D3DC_USAGE /*uSage*/, BOOL bDiscard )
{
	CHB_StatsCount(CHB_STATS_VERTEX_BUFFER_BYTES_UPLOADED, size);
	return dxC_UpdateBufferEx<LPD3DCVB>( pBuffer, pSysData, offset, size, bDiscard );
}
inline PRESULT dxC_UpdateBufferEx( LPD3DCIB pBuffer, void * pSysData, UINT offset, UINT size, D3DC_USAGE /*uSage*/, BOOL bDiscard )
{
	CHB_StatsCount(CHB_STATS_INDEX_BUFFER_BYTES_UPLOADED, size);
	return dxC_UpdateBufferEx<LPD3DCIB>( pBuffer, pSysData, offset, size, bDiscard );
}
#elif defined(ENGINE_TARGET_DX10)
PRESULT dxC_UpdateBufferEx(
	LPD3DCBASEBUFFER pBuffer,
	void* pSysData,
	UINT offset,
	UINT size,
	D3DC_USAGE uSage,
	BOOL bDiscard );
#endif

#ifdef ENGINE_TARGET_DX9
template <class T, class D>
inline PRESULT dxC_BufferPreloadEx( const T pB )
{
#if ISVERSION(DEVELOPMENT)
	D tDesc;
	V_RETURN( pB->GetDesc( &tDesc ) );
	ASSERT_RETFAIL( tDesc.Pool == D3DPOOL_MANAGED );
#endif
	ASSERT_RETFAIL( pB );
	pB->PreLoad();
	return S_OK;
}
#endif

inline PRESULT dxC_VertexBufferPreload( int nD3DBufferID )
{
	ASSERT_RETFAIL( nD3DBufferID != 0 );
	D3D_VERTEX_BUFFER * pVB = dxC_VertexBufferGet( nD3DBufferID );
	ASSERT_RETFAIL( pVB );
	if ( ! pVB->pD3DVertices )
		return S_FALSE;
#ifdef ENGINE_TARGET_DX9
	return dxC_BufferPreloadEx<LPD3DCVB, D3DVERTEXBUFFER_DESC>( pVB->pD3DVertices ); 
#endif
	return S_OK;
}

inline PRESULT dxC_IndexBufferPreload( int nD3DBufferID )
{
	ASSERT_RETFAIL( nD3DBufferID != 0 );
	D3D_INDEX_BUFFER * pIB = dxC_IndexBufferGet( nD3DBufferID );
	ASSERT_RETFAIL( pIB );
	if ( ! pIB->pD3DIndices )
		return S_FALSE;
#ifdef ENGINE_TARGET_DX9
	return dxC_BufferPreloadEx<LPD3DCIB, D3DINDEXBUFFER_DESC>( pIB->pD3DIndices ); 
#endif
	return S_OK;
}

//----------------------------------------------------------------------------

inline int dxC_VertexBufferAdd()
{
	extern CHash<D3D_VERTEX_BUFFER> gtVertexBuffers;
	extern int gnNextVertexBufferId;
	D3D_VERTEX_BUFFER * pVB = HashAdd( gtVertexBuffers, gnNextVertexBufferId++ );
	ASSERT_RETINVALID( pVB );
	return pVB->nId;
}

inline int dxC_IndexBufferAdd()
{
	extern CHash<D3D_INDEX_BUFFER> gtIndexBuffers;
	extern int gnNextIndexBufferId;
	D3D_INDEX_BUFFER * pIB = HashAdd( gtIndexBuffers, gnNextIndexBufferId++ );
	ASSERT_RETINVALID( pIB );
	return pIB->nId;
}

//----------------------------------------------------------------------------

inline PRESULT dxC_VertexBufferReleaseResource( int nID )
{
	ASSERT_RETINVALIDARG( nID != 0 );
	if ( nID == INVALID_ID )
		return S_FALSE;
	D3D_VERTEX_BUFFER * pVB = dxC_VertexBufferGet( nID );
	ASSERT_RETFAIL( pVB );
	pVB->pD3DVertices = NULL;
	return S_OK;
}

inline PRESULT dxC_IndexBufferReleaseResource( int nID )
{
	ASSERT_RETINVALIDARG( nID != 0 );
	if ( nID == INVALID_ID )
		return S_FALSE;
	D3D_INDEX_BUFFER * pIB = dxC_IndexBufferGet( nID );
	ASSERT_RETFAIL( pIB );
	pIB->pD3DIndices = NULL;
	return S_OK;
}

//----------------------------------------------------------------------------

inline PRESULT dxC_VertexBufferDestroy( int nID )
{
	ASSERT_RETINVALIDARG( nID != 0 );
	if ( nID == INVALID_ID )
		return S_FALSE;
	extern CHash<D3D_VERTEX_BUFFER> gtVertexBuffers;
	V( dxC_VertexBufferReleaseResource( nID ) );
	HashRemove( gtVertexBuffers, nID );
	return S_OK;
}

inline PRESULT dxC_IndexBufferDestroy( int nID )
{
	ASSERT_RETINVALIDARG( nID != 0 );
	if ( nID == INVALID_ID )
		return S_FALSE;
	extern CHash<D3D_INDEX_BUFFER> gtIndexBuffers;
	V( dxC_IndexBufferReleaseResource( nID ) );
	HashRemove( gtIndexBuffers, nID );
	return S_OK;
}

//----------------------------------------------------------------------------

inline BOOL dxC_VertexBufferD3DExists( int nID )
{
	ASSERT_RETFALSE( nID != 0 );
	D3D_VERTEX_BUFFER * pVB = dxC_VertexBufferGet( nID );
	if ( pVB && ( pVB->pD3DVertices || dxC_IsPixomaticActive() ) ) 
		return TRUE;
	return FALSE;
}

inline BOOL dxC_IndexBufferD3DExists( int nID )
{
	ASSERT_RETFALSE( nID != 0 );
	D3D_INDEX_BUFFER * pIB = dxC_IndexBufferGet( nID );
	if ( pIB && ( pIB->pD3DIndices || dxC_IsPixomaticActive() ) )
		return TRUE;
	return FALSE;
}

//----------------------------------------------------------------------------

inline PRESULT dxC_VertexBufferGetSizeInMemory( int nVBD3DID, DWORD * pdwBytes )
{
	ASSERT_RETFAIL( pdwBytes );
	*pdwBytes = 0;
	D3D_VERTEX_BUFFER * pVB = dxC_VertexBufferGet( nVBD3DID );
	if ( pVB && pVB->pD3DVertices )
	{
		DX9_BLOCK( D3DVERTEXBUFFER_DESC tDesc; );
		DX10_BLOCK( D3D10_BUFFER_DESC tDesc; );
		pVB->pD3DVertices->GetDesc( &tDesc );
		*pdwBytes += DXC_9_10( tDesc.Size, tDesc.ByteWidth );
	}
	return S_OK;
}

inline PRESULT dxC_IndexBufferGetSizeInMemory( int nIBD3DID, DWORD * pdwBytes )
{
	ASSERT_RETFAIL( pdwBytes );
	*pdwBytes = 0;
	D3D_INDEX_BUFFER * pIB = dxC_IndexBufferGet( nIBD3DID );
	if ( pIB && pIB->pD3DIndices )
	{
		DX9_BLOCK( D3DINDEXBUFFER_DESC tDesc; );
		DX10_BLOCK( D3D10_BUFFER_DESC tDesc; );
		pIB->pD3DIndices->GetDesc( &tDesc );
		*pdwBytes += DXC_9_10( tDesc.Size, tDesc.ByteWidth );
	}
	return S_OK;
}

//----------------------------------------------------------------------------

inline PRESULT dxC_VertexBufferDefGetSizeInMemory( const D3D_VERTEX_BUFFER_DEFINITION & tBuffer, DWORD * pdwBytes, DWORD * pdwCount = NULL )
{
	ASSERT_RETFAIL( pdwBytes );
	*pdwBytes = 0;
	if ( pdwCount )
		*pdwCount = 0;
	for ( int i = 0; i < DXC_MAX_VERTEX_STREAMS; i++ )
	{
		if ( dxC_VertexBufferD3DExists( tBuffer.nD3DBufferID[ i ] ) )
		{
			DWORD dwBytes = 0;
			V( dxC_VertexBufferGetSizeInMemory( tBuffer.nD3DBufferID[ i ], &dwBytes ) );
			*pdwBytes += dwBytes;
			if ( pdwCount )
				*pdwCount++;
		}
	}
	return S_OK;
}

inline PRESULT dxC_IndexBufferDefGetSizeInMemory( const D3D_INDEX_BUFFER_DEFINITION & tBuffer, DWORD * pdwBytes )
{
	ASSERT_RETFAIL( pdwBytes );
	*pdwBytes = 0;
	if ( dxC_IndexBufferD3DExists( tBuffer.nD3DBufferID ) )
	{
		DWORD dwBytes = 0;
		V( dxC_IndexBufferGetSizeInMemory( tBuffer.nD3DBufferID, &dwBytes ) );
		*pdwBytes += dwBytes;
	}
	return S_OK;
}

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

PRESULT dxC_BuffersInit();
PRESULT dxC_BuffersDestroy();
PRESULT dxC_BuffersReleaseResourceAll();

PRESULT dxC_CreateVertexBufferEx(
	UINT size,
	D3DC_USAGE usage,
	D3D_VERTEX_BUFFER * ptVBuffer,
	DWORD dwDx9FVF = 0x00000000,
	void* pInitialData = NULL );

PRESULT dxC_CreateVertexBuffer(
	int nStream,
	D3D_VERTEX_BUFFER_DEFINITION & tVBDef,
	void * pInitialData = NULL );

PRESULT dxC_CreateIndexBufferEx(
	UINT size,
	DXC_FORMAT format,
	D3DC_USAGE usage,
	D3D_INDEX_BUFFER* pIndexBuffer,
	void* pInitialData = NULL );

PRESULT dxC_CreateIndexBuffer(
	D3D_INDEX_BUFFER_DEFINITION & tIBDef,
	void * pInitialData = NULL );

PRESULT dxC_UpdateBuffer(
	int nStream, 
	D3D_VERTEX_BUFFER_DEFINITION & tVBDef,
	void * pSysData,
	UINT offset,
	UINT size,
	BOOL bDiscard = FALSE );

PRESULT dxC_UpdateBuffer(
	D3D_INDEX_BUFFER_DEFINITION & tIBDef,
	void * pSysData,
	UINT offset,
	UINT size,
	BOOL bDiscard = FALSE );

PRESULT dxC_QuadVBsCreate();
PRESULT dxC_QuadVBsDestroy();
PRESULT dxC_SetQuad( QUAD_TYPE QuadType, const D3D_EFFECT* pEffect );

PRESULT dxC_VertexBufferShouldSet( const D3D_VERTEX_BUFFER * pVB, int & nReplacement );
PRESULT dxC_VertexBufferDidSet( D3D_VERTEX_BUFFER * pVB, DWORD dwBytes );
PRESULT dxC_VertexBufferInitEngineResource( D3D_VERTEX_BUFFER * pVB );
PRESULT dxC_IndexBufferShouldSet( const D3D_INDEX_BUFFER * pIB, int & nReplacement );
PRESULT dxC_IndexBufferDidSet( D3D_INDEX_BUFFER * pIB, DWORD dwBytes );
PRESULT dxC_IndexBufferInitEngineResource( D3D_INDEX_BUFFER * pIB );
PRESULT dxC_SetPrefetchIndices( BOOL bForce = FALSE );




#endif //__DXC_BUFFER__
