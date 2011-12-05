//----------------------------------------------------------------------------
// dxC_Buffer.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"

#include "dxC_buffer.h"
#include "dxC_state.h"
#include "dxC_vertexdeclarations.h"
#include "dxC_profile.h"
#include "dxC_pixo.h"
#include "e_main.h"


using namespace FSSE;

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

typedef VS_FULLSCREEN_QUAD_STREAM_0 FULLSCREEN_QUAD_UV_STRUCT;
typedef VS_POSITION_TEX_STREAM_0 BASIC_QUAD_XZ_STRUCT;


struct PREFETCH_RENDER_DATA
{
	static const int MAX_TEXTURES = 8;

	int nTextures[ MAX_TEXTURES ];
};



//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

CHash<D3D_VERTEX_BUFFER>					gtVertexBuffers;
int											gnNextVertexBufferId = 1; // start at 1 to detect erroneously initialized buffers

CHash<D3D_INDEX_BUFFER>						gtIndexBuffers;
int											gnNextIndexBufferId = 1; // start at 1 to detect erroneously initialized buffers

static D3D_VERTEX_BUFFER_DEFINITION			gtQuadVBs[ QUAD_TYPE_COUNT ];

static D3D_VERTEX_BUFFER_DEFINITION			gtPrefetchVBDef;
static D3D_INDEX_BUFFER_DEFINITION			gtPrefetchIBDef;

//----------------------------------------------------------------------------
// IMPLEMENTATIONS
//----------------------------------------------------------------------------

PRESULT dxC_BuffersInit()
{
	const int VERTEX_BUFFER_HASH_SIZE = 64;
	const int INDEX_BUFFER_HASH_SIZE  = 256;

	HashInit( gtVertexBuffers,				NULL, VERTEX_BUFFER_HASH_SIZE );
	HashInit( gtIndexBuffers,				NULL, INDEX_BUFFER_HASH_SIZE );

	for( UINT i = 0; i < QUAD_TYPE_COUNT; ++i )
	{
		dxC_VertexBufferDefinitionInit( gtQuadVBs[ i ] );
	}

	return S_OK;
}

PRESULT dxC_BuffersDestroy()
{
	V( dxC_BuffersReleaseResourceAll() );

	HashFree( gtVertexBuffers );
	HashFree( gtIndexBuffers );
	return S_OK;
}

PRESULT dxC_BuffersReleaseResourceAll()
{
	for ( D3D_VERTEX_BUFFER * pVB = HashGetFirst( gtVertexBuffers );
		pVB;
		pVB = HashGetNext( gtVertexBuffers, pVB ) )
	{
		pVB->pD3DVertices = NULL;
	}

	for ( D3D_INDEX_BUFFER * pIB = HashGetFirst( gtIndexBuffers );
		pIB;
		pIB = HashGetNext( gtIndexBuffers, pIB ) )
	{
		pIB->pD3DIndices = NULL;
	}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void dxC_VertexBufferDefinitionInit( D3D_VERTEX_BUFFER_DEFINITION & tVBDef )
{
	static const D3D_VERTEX_BUFFER_DEFINITION tZero = ZERO_VB_DEF;
	tVBDef = tZero;
}

void dxC_IndexBufferDefinitionInit( D3D_INDEX_BUFFER_DEFINITION & tIBDef )
{
	static const D3D_INDEX_BUFFER_DEFINITION tZero = ZERO_IB_DEF;
	tIBDef = tZero;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_CreateVertexBufferEx(
	UINT size,
	D3DC_USAGE usage,
	D3D_VERTEX_BUFFER * ptVBuffer,
	DWORD dwDx9FVF /*= 0x00000000*/,
	void* pInitialData /*= NULL*/ )
{
	if ( e_IsNoRender() )
		return S_FALSE;
	if ( dxC_IsPixomaticActive() )
		return S_FALSE;

	ASSERT_RETINVALIDARG( ptVBuffer );

	ptVBuffer->tUsage = usage;

#ifdef ENGINE_TARGET_DX9
	if ( ! e_IsNoRender() )
	{
		// in case the device was lost, try to reset it before proceeding
		//V_RETURN( e_CheckValid() );
	}

	PRESULT pr = E_FAIL;
	NEEDS_DEVICE( pr, dxC_GetDevice()->CreateVertexBuffer( size, dx9_GetUsage( usage ), dwDx9FVF, dx9_GetPool( usage ), &ptVBuffer->pD3DVertices, NULL ) );
	ASSERTX_RETVAL( SUCCEEDED(pr), pr, "Failed to create vertex buffer!" )
	ASSERT_RETFAIL( ptVBuffer->pD3DVertices );

	if( !pInitialData )
		return S_OK;

	// CHB 2006.10.12
	// Direct3D9: (ERROR) :Can specify D3DLOCK_DISCARD or D3DLOCK_NOOVERWRITE for only Vertex Buffers created with D3DUSAGE_DYNAMIC
	V_RETURN( dxC_UpdateBufferEx<LPD3DCVB>( ptVBuffer->pD3DVertices, pInitialData, 0, size , dx9_GetUsage( usage ) == D3DUSAGE_DYNAMIC ) );

	if ( dx9_GetPool( usage ) == D3DPOOL_DEFAULT ) 
	{
		gD3DStatus.dwVidMemTotal += size;
		gD3DStatus.dwVidMemVertexBuffer += size;
	}
#endif

	DX10_BLOCK( V_RETURN( dx10_CreateBuffer( size, dx10_GetUsage( usage ), dx10_GetBindFlags( usage ) | D3D10_BIND_VERTEX_BUFFER, dx10_GetCPUAccess( usage ), &ptVBuffer->pD3DVertices, pInitialData ) ); )

	V_RETURN( dxC_VertexBufferInitEngineResource( ptVBuffer ) );

	return S_OK;
}

PRESULT dxC_CreateVertexBuffer( int nStream, D3D_VERTEX_BUFFER_DEFINITION & tVBDef, void * pInitialData /*= NULL*/ )
{
	ASSERT_RETINVALIDARG( tVBDef.nD3DBufferID[ nStream ] != 0 );
	ASSERT_RETFAIL( ! dxC_VertexBufferD3DExists( tVBDef.nD3DBufferID[ nStream ] ) );
	ASSERT_RETFAIL( tVBDef.nBufferSize[ nStream ] >= (int)( dxC_GetVertexSize( nStream, &tVBDef ) * tVBDef.nVertexCount ) );

	tVBDef.nD3DBufferID[ nStream ] = dxC_VertexBufferAdd();
	D3D_VERTEX_BUFFER * pVB = dxC_VertexBufferGet( tVBDef.nD3DBufferID[ nStream ] );
	ASSERT_RETFAIL( pVB );

	return dxC_CreateVertexBufferEx( tVBDef.nBufferSize[ nStream ], tVBDef.tUsage, pVB, tVBDef.dwFVF, pInitialData );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_CreateIndexBufferEx(
	UINT size,
	DXC_FORMAT format,
	D3DC_USAGE usage,
	D3D_INDEX_BUFFER* pIndexBuffer,
	void* pInitialData /*= NULL*/ )
{
	if ( e_IsNoRender() )
		return S_FALSE;
	if ( dxC_IsPixomaticActive() )
		return S_FALSE;

	ASSERT_RETINVALIDARG( pIndexBuffer );

	pIndexBuffer->tUsage = usage;

#ifdef ENGINE_TARGET_DX9
	if ( ! e_IsNoRender() )
	{
		// in case the device was lost, try to reset it before proceeding
		V_RETURN( e_CheckValid() );
	}

	ASSERTX_RETVAL( !pInitialData, E_NOTIMPL, "Not handiling initial data in DX9 yet!" );
	PRESULT pr = E_FAIL;
	NEEDS_DEVICE( pr, dxC_GetDevice()->CreateIndexBuffer( size, dx9_GetUsage( usage ), format, dx9_GetPool( usage ), &pIndexBuffer->pD3DIndices, NULL ) );
	ASSERTX_RETVAL( SUCCEEDED(pr), pr, "Failed to create index buffer!" )

	if ( dx9_GetPool( usage ) == D3DPOOL_DEFAULT )
	{
		gD3DStatus.dwVidMemTotal += size;
		//gD3DStatus.dwVidMemIndexBuffer += size;
	}
#endif

	DX10_BLOCK( V_RETURN( dx10_CreateBuffer( size, dx10_GetUsage( usage ), dx10_GetBindFlags( usage ) | D3D10_BIND_INDEX_BUFFER, dx10_GetCPUAccess( usage ), &pIndexBuffer->pD3DIndices, pInitialData ) ); )

	V_RETURN( dxC_IndexBufferInitEngineResource( pIndexBuffer ) );

	return S_OK;
}

PRESULT dxC_CreateIndexBuffer( D3D_INDEX_BUFFER_DEFINITION & tIBDef, void * pInitialData /*= NULL*/ )
{
	ASSERT_RETINVALIDARG( tIBDef.nD3DBufferID != 0 );
	ASSERT_RETFAIL( ! dxC_IndexBufferD3DExists( tIBDef.nD3DBufferID ) );

	tIBDef.nD3DBufferID = dxC_IndexBufferAdd();
	D3D_INDEX_BUFFER * pIB = dxC_IndexBufferGet( tIBDef.nD3DBufferID );
	ASSERT_RETFAIL( pIB );

	return dxC_CreateIndexBufferEx( tIBDef.nBufferSize, tIBDef.tFormat, tIBDef.tUsage, pIB, pInitialData );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_UpdateBuffer(
	int nStream,
	D3D_VERTEX_BUFFER_DEFINITION & tVBDef,
	void* pSysData,
	UINT offset,
	UINT size,
	BOOL bDiscard /*= FALSE*/ )
{
	if ( e_IsNoRender() )
		return S_FALSE;
	if ( dxC_IsPixomaticActive() )
		return S_FALSE;

	ASSERT_RETINVALIDARG( nStream < (int)dxC_GetStreamCount( &tVBDef ) );
	ASSERT_RETINVALIDARG( offset <= (UINT)tVBDef.nBufferSize[ nStream ] );
	ASSERT_RETINVALIDARG( ( offset + size ) <= (UINT)tVBDef.nBufferSize[ nStream ] );
	D3D_VERTEX_BUFFER * pVB = dxC_VertexBufferGet( tVBDef.nD3DBufferID[ nStream ] );
	ASSERT_RETINVALIDARG( pVB && pVB->pD3DVertices );

#ifdef ENGINE_TARGET_DX9
	if ( dx9_GetPool( tVBDef.tUsage ) != D3DPOOL_DEFAULT )
		ASSERTX_RETFAIL( ! bDiscard, "Cannot lock non-default pool buffer with discard flag!" );
#endif

	return dxC_UpdateBufferEx(
		pVB->pD3DVertices,
		pSysData,
		offset,
		size,
		pVB->tUsage,
		bDiscard );
}

PRESULT dxC_UpdateBuffer(
	D3D_INDEX_BUFFER_DEFINITION & tIBDef,
	void * pSysData,
	UINT offset,
	UINT size,
	BOOL bDiscard /*= FALSE*/ )
{
	if ( e_IsNoRender() )
		return S_FALSE;
	if ( dxC_IsPixomaticActive() )
		return S_FALSE;

	ASSERT_RETINVALIDARG( offset <= (UINT)tIBDef.nBufferSize );
	ASSERT_RETINVALIDARG( ( offset + size ) <= (UINT)tIBDef.nBufferSize );
	D3D_INDEX_BUFFER * pIB = dxC_IndexBufferGet( tIBDef.nD3DBufferID );
	ASSERT_RETINVALIDARG( pIB && pIB->pD3DIndices );

#ifdef ENGINE_TARGET_DX9
	if ( dx9_GetPool( tIBDef.tUsage ) != D3DPOOL_DEFAULT )
		ASSERTX_RETFAIL( ! bDiscard, "Cannot lock non-default pool buffer with discard flag!" );
#endif

	return dxC_UpdateBufferEx(
		pIB->pD3DIndices,
		pSysData,
		offset,
		size,
		pIB->tUsage,
		bDiscard );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static inline VERTEX_DECL_TYPE sGetDecl( QUAD_TYPE QuadType )
{
	switch ( QuadType )
	{
	case FULLSCREEN_QUAD_UV:
		return VERTEX_DECL_FULLSCREEN_QUAD; 
		break;

	case BASIC_QUAD_XY_PLANE:
		return VERTEX_DECL_XYZ_UV;
		break;
	};

	ASSERTX( 0, "Unsupported quad type!" );
	return VERTEX_DECL_INVALID;
}

PRESULT dxC_QuadVBsCreate()
{
	//FULLSCREEN_QUAD_UV
	{
		FULLSCREEN_QUAD_UV_STRUCT quadUV[ 6 ];
		quadUV[0].Position = D3DXVECTOR2( -1.0f,  1.0f );
		quadUV[1].Position = D3DXVECTOR2( -1.0f, -1.0f );
		quadUV[2].Position = D3DXVECTOR2(  1.0f,  1.0f );
		quadUV[3].Position = D3DXVECTOR2(  1.0f,  1.0f );
		quadUV[4].Position = D3DXVECTOR2( -1.0f, -1.0f );
		quadUV[5].Position = D3DXVECTOR2(  1.0f, -1.0f );

		D3D_VERTEX_BUFFER_DEFINITION & tVBDef = gtQuadVBs[ FULLSCREEN_QUAD_UV ];

		dxC_VertexBufferDefinitionInit( tVBDef );
		tVBDef.tUsage = D3DC_USAGE_BUFFER_CONSTANT;
		tVBDef.eVertexType = sGetDecl( FULLSCREEN_QUAD_UV );
		tVBDef.nVertexCount = 6;
		tVBDef.nBufferSize[ 0 ] = tVBDef.nVertexCount * dxC_GetVertexSize( 0, &tVBDef );

		size_t nSize = sizeof(quadUV);
		tVBDef.pLocalData[0] = MALLOC( NULL, nSize );
		MemoryCopy( tVBDef.pLocalData[0], nSize, quadUV, nSize );

		V_RETURN( dxC_CreateVertexBuffer( 0, tVBDef, quadUV ) );
	}

	//BASIC_QUAD_XY_PLANE
	{
		static const int scnBasicQuadVertCount = 6;
		BASIC_QUAD_XZ_STRUCT vQuadBasic[ scnBasicQuadVertCount ];
		vQuadBasic[ 0 ].Position = D3DXVECTOR3( -1, -1, 0 );
		vQuadBasic[ 0 ].TexCoord = D3DXVECTOR2( 0, 1 );
		vQuadBasic[ 1 ].Position = D3DXVECTOR3( -1, 1, 0 );
		vQuadBasic[ 1 ].TexCoord = D3DXVECTOR2( 0, 0 );
		vQuadBasic[ 2 ].Position = D3DXVECTOR3( 1, -1, 0 );
		vQuadBasic[ 2 ].TexCoord = D3DXVECTOR2( 1, 1 );
		vQuadBasic[ 3 ].Position = D3DXVECTOR3( 1, -1, 0 );
		vQuadBasic[ 3 ].TexCoord = D3DXVECTOR2( 1, 1 );
		vQuadBasic[ 4 ].Position = D3DXVECTOR3( -1, 1, 0 );
		vQuadBasic[ 4 ].TexCoord = D3DXVECTOR2( 0, 0 );
		vQuadBasic[ 5 ].Position = D3DXVECTOR3( 1, 1, 0 );
		vQuadBasic[ 5 ].TexCoord = D3DXVECTOR2( 1, 0 );	

		D3D_VERTEX_BUFFER_DEFINITION & tVBDef = gtQuadVBs[ BASIC_QUAD_XY_PLANE ];

		dxC_VertexBufferDefinitionInit( tVBDef );
		tVBDef.tUsage = D3DC_USAGE_BUFFER_CONSTANT;
		tVBDef.eVertexType = sGetDecl( BASIC_QUAD_XY_PLANE );
		tVBDef.nVertexCount = scnBasicQuadVertCount;
		tVBDef.nBufferSize[ 0 ] = tVBDef.nVertexCount * dxC_GetVertexSize( 0, &tVBDef );

		size_t nSize = sizeof(vQuadBasic);
		tVBDef.pLocalData[0] = MALLOC( NULL, nSize );
		MemoryCopy( tVBDef.pLocalData[0], nSize, vQuadBasic, nSize );

		V_RETURN( dxC_CreateVertexBuffer( 0, tVBDef, vQuadBasic ) );
	}

	// Prefetch triangle.
	{
		// Verts
		VECTOR vTri[3] =
		{
			VECTOR( 0,0,0 ),		// Intentionally degenerate triangle
			VECTOR( 0,0,0 ),
			VECTOR( 0,0,0 ),
		};

		// Indexes
		unsigned short uInds[3] = { 0, 1, 2 };

		dxC_VertexBufferDefinitionInit( gtPrefetchVBDef );
		gtPrefetchVBDef.tUsage = D3DC_USAGE_BUFFER_CONSTANT;
		gtPrefetchVBDef.eVertexType = VERTEX_DECL_SIMPLE;
		gtPrefetchVBDef.nVertexCount = 3;
		gtPrefetchVBDef.nBufferSize[0] = gtPrefetchVBDef.nVertexCount * dxC_GetVertexSize( 0, &gtPrefetchVBDef );
		V_RETURN( dxC_CreateVertexBuffer( 0, gtPrefetchVBDef, vTri ) );

		dxC_IndexBufferDefinitionInit( gtPrefetchIBDef );
		gtPrefetchIBDef.tUsage = D3DC_USAGE_BUFFER_REGULAR;
		gtPrefetchIBDef.tFormat = D3DFMT_INDEX16;
		gtPrefetchIBDef.nIndexCount = 3;
		gtPrefetchIBDef.nBufferSize = sizeof(unsigned short) * gtPrefetchIBDef.nIndexCount;

		V_RETURN( dxC_CreateIndexBuffer( gtPrefetchIBDef ) );
		V_RETURN( dxC_UpdateBuffer( gtPrefetchIBDef, uInds, 0, gtPrefetchIBDef.nBufferSize ) );
	}

	return S_OK;
}

PRESULT dxC_QuadVBsDestroy()
{
	for( UINT i = 0; i < QUAD_TYPE_COUNT; ++i )
	{
		V( dxC_VertexBufferDestroy( gtQuadVBs[ i ].nD3DBufferID[ 0 ] ) );
		gtQuadVBs[ i ].nD3DBufferID[ 0 ] = INVALID_ID;
	}

	V( dxC_VertexBufferDestroy( gtPrefetchVBDef.nD3DBufferID[0] ) );
	gtPrefetchVBDef.nD3DBufferID[0] = INVALID_ID;
	V( dxC_IndexBufferDestroy( gtPrefetchIBDef.nD3DBufferID ) );
	gtPrefetchIBDef.nD3DBufferID = INVALID_ID;

	return S_OK;
}


PRESULT dxC_SetQuad( QUAD_TYPE QuadType, const D3D_EFFECT* pEffect )
{
	return dxC_SetStreamSource( gtQuadVBs[ QuadType ], 0, pEffect ); 
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_VertexBufferShouldSet( const D3D_VERTEX_BUFFER * pVB, int & nReplacement )
{
	nReplacement = INVALID_ID;
	ASSERT_RETINVALIDARG( pVB );
	if ( TESTBIT_DWORD( pVB->tEngineRes.dwFlagBits, ENGINE_RESOURCE::FLAGBIT_UPLOADED ) )
		goto noprefetch;
	if ( ! TESTBIT_DWORD( pVB->tEngineRes.dwFlagBits, ENGINE_RESOURCE::FLAGBIT_MANAGED ) )
		goto noprefetch;

	if ( e_UploadManagerGet().CanUpload() )
		goto noprefetch;

	nReplacement = gtPrefetchVBDef.nD3DBufferID[0];
	dxC_DPSetPrefetch( TRUE );
	return S_OK;

noprefetch:
	dxC_DPSetPrefetch( FALSE );
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_VertexBufferDidSet( D3D_VERTEX_BUFFER * pVB, DWORD dwBytes )
{
	ASSERT_RETINVALIDARG( pVB );

	if ( TESTBIT_DWORD( pVB->tEngineRes.dwFlagBits, ENGINE_RESOURCE::FLAGBIT_UPLOADED ) )
		return S_OK;

	e_UploadManagerGet().MarkUpload( dwBytes, RESOURCE_UPLOAD_MANAGER::VBUFFER );
	SETBIT( pVB->tEngineRes.dwFlagBits, ENGINE_RESOURCE::FLAGBIT_UPLOADED, TRUE );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_VertexBufferInitEngineResource( D3D_VERTEX_BUFFER * pVB )
{
	BOOL bManaged;

#ifdef ENGINE_TARGET_DX9
	bManaged = !!( dx9_GetPool( pVB->tUsage ) == D3DPOOL_MANAGED );
#else	// DX10
	// CML TODO:  I don't know how to handle upload throttling in DX10 yet.
	bManaged = FALSE;
#endif

	SETBIT( pVB->tEngineRes.dwFlagBits, ENGINE_RESOURCE::FLAGBIT_MANAGED, bManaged );
	SETBIT( pVB->tEngineRes.dwFlagBits, ENGINE_RESOURCE::FLAGBIT_UPLOADED, FALSE );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_IndexBufferShouldSet( const D3D_INDEX_BUFFER * pIB, int & nReplacement )
{
	// CML 2007.09.11: Always allow index buffers through.
	return TRUE;

	//nReplacement = INVALID_ID;
	//ASSERT_RETINVALIDARG( pIB );
	//if ( TESTBIT_DWORD( pIB->tEngineRes.dwFlagBits, ENGINE_RESOURCE::FLAGBIT_UPLOADED ) )
	//	return S_OK;
	//if ( ! TESTBIT_DWORD( pIB->tEngineRes.dwFlagBits, ENGINE_RESOURCE::FLAGBIT_MANAGED ) )
	//	return S_OK;

	//if ( e_UploadManagerGet().CanUpload() )
	//	return S_OK;

	//nReplacement = gtPrefetchIBDef.nD3DBufferID;
	//return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_IndexBufferDidSet( D3D_INDEX_BUFFER * pIB, DWORD dwBytes )
{
	ASSERT_RETINVALIDARG( pIB );

	if ( TESTBIT_DWORD( pIB->tEngineRes.dwFlagBits, ENGINE_RESOURCE::FLAGBIT_UPLOADED ) )
		return S_OK;

	e_UploadManagerGet().MarkUpload( dwBytes, RESOURCE_UPLOAD_MANAGER::IBUFFER );
	SETBIT( pIB->tEngineRes.dwFlagBits, ENGINE_RESOURCE::FLAGBIT_UPLOADED, TRUE );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_IndexBufferInitEngineResource( D3D_INDEX_BUFFER * pIB )
{
	BOOL bManaged;

#ifdef ENGINE_TARGET_DX9
	bManaged = !!( dx9_GetPool( pIB->tUsage ) == D3DPOOL_MANAGED );
#else	// DX10
	// CML TODO:  I don't know how to handle upload throttling in DX10 yet.
	bManaged = FALSE;
#endif

	SETBIT( pIB->tEngineRes.dwFlagBits, ENGINE_RESOURCE::FLAGBIT_MANAGED, bManaged );
	SETBIT( pIB->tEngineRes.dwFlagBits, ENGINE_RESOURCE::FLAGBIT_UPLOADED, FALSE );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_SetPrefetchIndices( BOOL bForce /*= FALSE*/ )
{
	return dxC_SetIndices( gtPrefetchIBDef, bForce );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_PrefetchRender()
{

	return S_OK;
}