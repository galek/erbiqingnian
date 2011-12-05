//----------------------------------------------------------------------------
// dxC_primitive.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DXC_PRIMITIVE__
#define __DXC_PRIMITIVE__

#include "e_primitive.h"
#include "dxC_FVF.h"
#include "dxC_buffer.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define NUM_BOUNDING_BOX_LIST_INDICES	36
#define NUM_BOUNDING_BOX_LIST_VERTICES	8
#define NUM_BOUNDING_BOX_2D_LIST_INDICES 8
#define NUM_BOUNDING_BOX_2D_LIST_VERTICES 4

//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

enum PRIM_RENDER_TYPE
{
	PRIM_RENDER_TRI = 0,
	PRIM_RENDER_LINE,
	NUM_PRIM_RENDER_TYPES
};

enum PRIM_RENDER_PASS
{
	PRIM_PASS_IN_WORLD = 0,
	PRIM_PASS_ON_TOP,
	NUM_PRIM_PASSES
};

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

typedef struct {
	VECTOR vPosition;
} BOUNDING_BOX_VERTEX;

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

extern D3D_INDEX_BUFFER_DEFINITION  gtBoundingBoxIndexBuffer;
extern D3D_VERTEX_BUFFER_DEFINITION gtBoundingBoxVertexBuffer;

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

PRESULT dx9_PrimitiveDrawRestore();
PRESULT dx9_PrimitiveDrawDestroy();
PRESULT dx9_PrimitiveDrawDeviceLost();

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// CLASSES
//----------------------------------------------------------------------------

template < class _VERTEX >
class PRIMITIVE_RENDER_LIST
{
protected:
	D3D_VERTEX_BUFFER_DEFINITION					m_tVBuffer;
	int												m_nPrimCount;

	int												m_nMaxVerts;
	int												m_nVertsPerPrim;
	BOOL											m_bRenderOnTop;
	BOOL											m_bNeedTransform;
	D3DC_FVF										m_dwFVF;
	SIMPLE_DYNAMIC_ARRAY<PRIMITIVE_INFO_RENDER> *	m_ptRenderList;
public:
	PRESULT Init( int nMaxVerts, int nVertsPerPrim, BOOL bRenderOnTop, BOOL bNeedTransform, D3DC_FVF dwFVF, SIMPLE_DYNAMIC_ARRAY<PRIMITIVE_INFO_RENDER> * ptRenderList )
	{
		ASSERT_RETINVALIDARG( ptRenderList );
		ASSERT_RETINVALIDARG( nVertsPerPrim == 2 || nVertsPerPrim == 3 );
		m_nMaxVerts			= nMaxVerts;
		m_nVertsPerPrim		= nVertsPerPrim;
		m_bRenderOnTop		= bRenderOnTop;
		m_bNeedTransform	= bNeedTransform;
		m_dwFVF				= dwFVF;
		m_ptRenderList		= ptRenderList;

		dxC_VertexBufferDefinitionInit( m_tVBuffer );

		return S_OK;
	}
	PRESULT Alloc()
	{
		// create vbuffer
		m_tVBuffer.dwFVF = DX9_BLOCK( m_dwFVF ) DX10_BLOCK( D3DC_FVF_NULL );
		m_tVBuffer.nVertexCount = m_nMaxVerts;
		//m_tVBuffer.nStructSize = sizeof(_VERTEX);
		m_tVBuffer.nBufferSize[ 0 ] = m_tVBuffer.nVertexCount * sizeof( _VERTEX );
		m_tVBuffer.tUsage = D3DC_USAGE_BUFFER_MUTABLE;
		V_RETURN( dxC_CreateVertexBuffer( 0, m_tVBuffer ) );
		return S_OK;
	}
	PRESULT Destroy()
	{
		return dxC_VertexBufferDestroy( m_tVBuffer.nD3DBufferID[ 0 ] );
	}
	PRESULT Clear()
	{
		return S_OK;
	}
	PRESULT Release()
	{
		V( dxC_VertexBufferReleaseResource( m_tVBuffer.nD3DBufferID[ 0 ] ) );
		m_tVBuffer.nD3DBufferID[ 0 ] = INVALID_ID;
		return S_OK;
	}
	PRESULT DeviceLost()
	{
		return Release();
	}
	PRESULT Add()
	{
		if ( ! dxC_VertexBufferD3DExists( m_tVBuffer.nD3DBufferID[ 0 ] ) )
		{
			V( Alloc() );
		}
		ASSERT_RETFAIL( dxC_VertexBufferD3DExists( m_tVBuffer.nD3DBufferID[ 0 ] ) );

		D3DXMATRIXA16 matWVP;
		D3DXVECTOR4 vTransformed;
		V( e_GetWorldViewProjMatrix( (MATRIX*)&matWVP ) );

		int nItemCount = m_ptRenderList->Count();
		if ( nItemCount * m_nVertsPerPrim > m_nMaxVerts )
		{
			// double the size of the vbuffer until it fits the verts
			BOUNDED_WHILE( nItemCount * m_nVertsPerPrim > m_nMaxVerts, 100000 )
				m_nMaxVerts *= 2;
			V( Release() );
			V( Alloc() );
		}

		D3D_VERTEX_BUFFER * pVB = dxC_VertexBufferGet( m_tVBuffer.nD3DBufferID[ 0 ] );
		ASSERT_RETFAIL( pVB && pVB->pD3DVertices );

		_VERTEX * pVerts;
		V( dxC_MapEntireBuffer( pVB->pD3DVertices, D3DLOCK_DISCARD, D3D10_MAP_WRITE_DISCARD, (void **)&pVerts ) );
		ASSERT_RETFAIL( pVerts );

		int nVert = 0;
		for ( int i = 0; i < nItemCount; i++ )
		{
			PRIMITIVE_INFO_RENDER & tInfo = (*m_ptRenderList)[ i ];
			if ( (   m_bRenderOnTop && 0 == ( tInfo.dwFlags & PRIM_FLAG_RENDER_ON_TOP ) ) ||
				 ( ! m_bRenderOnTop &&        tInfo.dwFlags & PRIM_FLAG_RENDER_ON_TOP   ) )
				 continue;

			if ( (   m_bNeedTransform && 0 == ( tInfo.dwFlags & _PRIM_FLAG_NEED_TRANSFORM ) ) ||
				 ( ! m_bNeedTransform &&        tInfo.dwFlags & _PRIM_FLAG_NEED_TRANSFORM   ) )
				 continue;

			sBuildDebugPrimitiveVertex( &pVerts[ nVert ], m_nVertsPerPrim, tInfo, &matWVP );

			nVert += m_nVertsPerPrim;
		}
		V( dxC_Unmap( pVB->pD3DVertices ) );
		m_nPrimCount = nVert / m_nVertsPerPrim;
		return S_OK;
	}
	PRESULT Render()
	{
		if ( ! m_nPrimCount )
			return S_OK;
		//dx9_SetFVF( m_dwFVF );
		V_RETURN( dxC_SetStreamSource( m_tVBuffer, 0 ) );
		V_RETURN( dxC_DrawPrimitive( ( m_nVertsPerPrim == 2 ) ? D3DPT_LINELIST : D3DPT_TRIANGLELIST, 0, m_nPrimCount, NULL, METRICS_GROUP_DEBUG ) );
		return S_OK;
	}
};


#endif //__DXC_PRIMITIVE__