//----------------------------------------------------------------------------
// dx9_particle.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"
#include "c_particles.h"
#include "e_particles_priv.h"
#include "dxC_occlusion.h"
#include "dxC_effect.h"
#include "dxC_model.h"
#include "dxC_state.h"
#include "dxC_profile.h"
#include "dxC_render.h"
#include "filepaths.h"
#include "pakfile.h"
#include "dxC_particle.h"
#include "dxC_portal.h"
#include "definition.h"
#include "excel.h"
#ifdef HAVOKFX_ENABLED
	#include "e_havokfx.h"
	#include "dxC_havokfx.h"
#endif
#include "dxC_buffer.h"
#include "dxC_state.h"
#include "dxC_texture.h"
#include "dxC_FVF.h"
#include "dxC_material.h"
#include "globalindex.h"
#include "perfhier.h"
#include "dxC_caps.h"

#ifdef ENGINE_TARGET_DX10
#include "dx10_ParticleSimulateGPU.h"
#endif

#include "dxC_shaderlimits.h"	// CHB 2006.06.28 - for MAX_PARTICLE_MESHES_PER_BATCH

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------
#define MAX_INSTANCES_PER_BATCH 5000
//#define DXC_DEFINE_SOME_STATE_IN_FX

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

BOOL gbParticlesNeedScreenTexture = FALSE;
static SPD3DCBASESHADERRESOURCEVIEW sgpParticleScreenTexture;
PARTICLE_DRAW_SECTION sgeParticleCurrentDrawSection = PARTICLE_DRAW_SECTION_INVALID;

//----------------------------------------------------------------------------
// IMPLEMENTATIONS
//----------------------------------------------------------------------------
D3D_EFFECT * e_ParticleSystemGetEffect( 
	const PARTICLE_SYSTEM_DEFINITION * pDefinition );

//-------------------------------------------------------------------------------------------------
// Quads
//-------------------------------------------------------------------------------------------------
#ifdef INTEL_CHANGES
// INTEL_NOTE: definition of PARTICLE_QUAD_VERTEX has been moved to c_particles.h

#else
struct PARTICLE_QUAD_VERTEX
{
	VECTOR		vPosition;
	float		fRand; // per-particle random pct, labeled as psize
	DWORD		dwDiffuse;
	DWORD		dwGlow; // labeled as specular
	float		pfTextureCoord0[2];
}; // 32 bytes... should be 32 for best cache performance!

#define VERTICES_PER_PARTICLE 6
#define TRIANGLES_PER_PARTICLE 2
#endif

//-------------------------------------------------------------------------------------------------
// Ropes and Knots
//-------------------------------------------------------------------------------------------------
enum 
{
	KNOT_DRAW_TYPE_TWO_STRIPS = 0,
	KNOT_DRAW_TYPE_ONE_STRIP_FACE_CAMERA,
	KNOT_DRAW_TYPE_ONE_STRIP_FACE_NORMAL,
	KNOT_DRAW_TYPE_TUBE_4,
	KNOT_DRAW_TYPE_TUBE_6,
	KNOT_DRAW_TYPE_TUBE_8,
	NUM_KNOT_DRAW_TYPES,
};
struct KNOT_DRAW_CHART
{
	int nVerticesPerKnot;
	int nMiddleVerticesStart;
	int nTrianglesPerKnot;
	int nFaces;
	float fFaces;
	D3D_INDEX_BUFFER_DEFINITION tIndexBufferDef;
	int nMaxKnotsPerDraw; // computed on init
};

// NOTE: don't forget to change e_ParticlesDrawInit as well as this table
static KNOT_DRAW_CHART sgtKnotDrawChart[ NUM_KNOT_DRAW_TYPES ] = 
{	// vertices,	middle,	triangles,	faces,	IB,				max
	{	5,			4,		8,			2,2.0f,	ZERO_IB_DEF,	0	},	//KNOT_DRAW_TYPE_TWO_STRIPS
	{	3,			2,		4,			1,2.0f,	ZERO_IB_DEF,	0	},	//KNOT_DRAW_TYPE_ONE_STRIP_FACE_CAMERA,
	{	3,			2,		4,			1,2.0f,	ZERO_IB_DEF,	0	},	//KNOT_DRAW_TYPE_ONE_STRIP_FACE_NORMAL,
	{	9,			5,		16,			4,4.0f,	ZERO_IB_DEF,	0	},	//KNOT_DRAW_TYPE_TUBE_4,
	{	13,			7,		24,			6,6.0f,	ZERO_IB_DEF,	0	},	//KNOT_DRAW_TYPE_TUBE_6,
	{	17,			9,		32,			8,8.0f,	ZERO_IB_DEF,	0	},	//KNOT_DRAW_TYPE_TUBE_8,
};
#define MAX_TRIANGLES_PER_KNOT 32

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
enum 
{
	PARTICLE_MODEL_VERTEX_TYPE_MULTI,
	PARTICLE_MODEL_VERTEX_TYPE_NORMALS_AND_ANIMS,
	NUM_PARTICLE_MODEL_VERTEX_TYPES,
};
struct PARTICLE_MODEL_FILE
{
	WORD wTriangleCount;
	int	nIndexBufferSize;
	int nVertexCount;
	int pnVertexBufferStart[ NUM_PARTICLE_MODEL_VERTEX_TYPES ];
	BOUNDING_BOX tMultiMeshBoundingBox;
};

//-------------------------------------------------------------------------------------------------
// Vertex Buffers
//-------------------------------------------------------------------------------------------------
enum {
	PARTICLE_VERTEX_BUFFER_BASIC,
#ifdef HAVOKFX_ENABLED
	PARTICLE_VERTEX_BUFFER_HAVOKFX_PARTICLES,
#endif // HAVOKFX_ENABLED
	NUM_PARTICLE_VERTEX_BUFFERS
};

#define MAX_VERTICES_IN_BASIC_BUFFER (VERTICES_PER_PARTICLE * 1000)

#ifdef HAVOKFX_ENABLED
	#define MAX_VERTICES_IN_HAVOKFX_PARTICLE_BUFFER (VERTICES_PER_PARTICLE)
#endif // HAVOKFX_ENABLED

static D3D_VERTEX_BUFFER_DEFINITION sgtParticleSystemVertexBuffers[ NUM_PARTICLE_VERTEX_BUFFERS ] =
{
	ZERO_VB_DEF,
#ifdef HAVOKFX_ENABLED
	ZERO_VB_DEF
#endif
};
static const int sgnParticleSystemVertexBufferSizes[ NUM_PARTICLE_VERTEX_BUFFERS ] = 
{ 
	MAX_VERTICES_IN_BASIC_BUFFER * sizeof( PARTICLE_QUAD_VERTEX ), 
#ifdef HAVOKFX_ENABLED
	MAX_VERTICES_IN_HAVOKFX_PARTICLE_BUFFER * sizeof( HAVOKFX_PARTICLE_VERTEX )
#endif // HAVOKFX_ENABLED
};

static PARTICLE_QUAD_VERTEX sgpBasicVertices[ MAX_VERTICES_IN_BASIC_BUFFER ];

static D3D_INDEX_BUFFER_DEFINITION sgtQuadIndexBuffer = ZERO_IB_DEF;

#ifdef INTEL_CHANGES
static
PARTICLE_QUAD_VERTEX * sGetVerticesBufferForIndex( 
	int index,
	PARTICLE_SYSTEM * pParticleSystem )
{
	if( index == 0 )
	{
		return sgpBasicVertices;
	}
	else
	{
		index--;
		if ( (pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_ASYNC) == 0 )
			return sgpParticleSystemAsyncMainThreadVerticesBuffer[ index ];
		else
			return sgpParticleSystemAsyncVerticesBuffer[ index ];
	}
}
#endif

//-------------------------------------------------------------------------------------------------
// Meshes
//-------------------------------------------------------------------------------------------------

// this is what is an intermediate format for grabbing values from granny
struct GRANNY_PARTICLE_MESH_VERTEX
{
	VECTOR vPosition;      // Vertex Position
	VECTOR vNormal;		// Vertex Normal
	FLOAT fTextureCoords[2];	// The texture coordinates
	BYTE pwIndices[4]; // Bone Indices
	FLOAT pflWeights[3]; // Bone Weights
	VECTOR vTangent;		// Used for Pixel Lighting
	VECTOR vBinormal;		// Used for Pixel Lighting
};

static granny_data_type_definition g_ptGrannyParticleMeshVertexDataType[] =
{
	{GrannyReal32Member, GrannyVertexPositionName,				0, 3},
	{GrannyReal32Member, GrannyVertexNormalName,				0, 3},
	{GrannyReal32Member, GrannyVertexTextureCoordinatesName "0",0, 2},
	{GrannyInt8Member,   GrannyVertexBoneIndicesName,			0, 4},
	{GrannyReal32Member, GrannyVertexBoneWeightsName,			0, 3},
	{GrannyReal32Member, GrannyVertexTangentName,				0, 3},
	{GrannyReal32Member, GrannyVertexBinormalName,				0, 3},
	{GrannyEndMember},
};

static float sgpfMeshMatrices	[ MAX_PARTICLE_MESHES_PER_BATCH * 3 * 4 ];
static float sgpfMeshColors		[ MAX_PARTICLE_MESHES_PER_BATCH * 4 ];
static float sgpfMeshGlows		[ MAX_PARTICLE_MESHES_PER_BATCH ];

//-------------------------------------------------------------------------------------------------

const D3DC_INPUT_ELEMENT_DESC g_ptParticleMeshVertexDeclarationArray_Multi[] =
{
	//					Offset					Semantic	DX9Index	DX10Index	DX9Type		DX10Type
	D3DC_INPUT_VERTEX(	0 * sizeof(D3DCOLOR),	POSITION,	0,			"",			UBYTE4,		R8G8B8A8_UINT	)
	D3DC_INPUT_VERTEX(	1 * sizeof(D3DCOLOR),	TEXCOORD,	0,			"0",		UBYTE4,		R8G8B8A8_UINT	)
	D3DC_INPUT_VERTEX(	2 * sizeof(D3DCOLOR),	NORMAL,		0,			"",			D3DCOLOR,	R8G8B8A8_UNORM	)
	D3DC_INPUT_VERTEX(	3 * sizeof(D3DCOLOR),	TANGENT,	0,			"",			D3DCOLOR,	R8G8B8A8_UNORM	)
	D3DC_INPUT_END()
};

//-------------------------------------------------------------------------------------------------

const D3DC_INPUT_ELEMENT_DESC g_ptParticleMeshVertexDeclarationArray_Multi_11[] =
{
	//					Offset					Semantic	DX9Index	DX10Index	DX9Type		DX10Type
	D3DC_INPUT_VERTEX(	0 * sizeof(D3DCOLOR),	POSITION,	0,			"",			D3DCOLOR,	R8G8B8A8_UINT	)
	D3DC_INPUT_VERTEX(	1 * sizeof(D3DCOLOR),	TEXCOORD,	0,			"0",		D3DCOLOR,	R8G8B8A8_UINT	)
	D3DC_INPUT_VERTEX(	2 * sizeof(D3DCOLOR),	NORMAL,		0,			"",			D3DCOLOR,	R8G8B8A8_UNORM	)
	D3DC_INPUT_VERTEX(	3 * sizeof(D3DCOLOR),	TANGENT,	0,			"",			D3DCOLOR,	R8G8B8A8_UNORM	)
	D3DC_INPUT_END()
};

//-------------------------------------------------------------------------------------------------
enum	
{// this simplifies the offsets in the declaration array
	MESH_VERTEX_NA_SIZE_1 = sizeof (D3DXVECTOR3),
	MESH_VERTEX_NA_SIZE_2 = MESH_VERTEX_NA_SIZE_1 + sizeof (D3DXVECTOR3)
};

#ifdef HAVOKFX_ENABLED
const D3DC_INPUT_ELEMENT_DESC g_ptParticleMeshVertexDeclarationArray_HavokFX[] =
{
	//						Offset					Semantic	DX9Index	DX10Index	DX9Type		DX10Type			StreamIndex
	D3DC_INPUT_VERTEX(		0,						POSITION,	0,			"",			FLOAT3,		R32G32B32_FLOAT			)
	D3DC_INPUT_VERTEX(		MESH_VERTEX_NA_SIZE_1,	NORMAL,		0,			"",			FLOAT3,		R32G32B32_FLOAT			)
	D3DC_INPUT_VERTEX(		MESH_VERTEX_NA_SIZE_2,	TEXCOORD,	0,			"0",		UBYTE4,		R32G32B32A32_FLOAT		)
	D3DC_INPUT_INSTANCE(	0,						TEXCOORD,	1,			"1",		FLOAT4,		R32G32B32A32_FLOAT,	1	)
	D3DC_INPUT_INSTANCE(	0,						TEXCOORD,	2,			"2",		FLOAT4,		R32G32B32A32_FLOAT,	2	)
	D3DC_INPUT_INSTANCE(	0,						TEXCOORD,	3,			"3",		FLOAT4,		R32G32B32A32_FLOAT,	3	)
	D3DC_INPUT_INSTANCE(	0,						TEXCOORD,	4,			"4",		FLOAT1,		R32G32B32A32_FLOAT,	4	)	// seed buffer
	D3DC_INPUT_END()
};

//-------------------------------------------------------------------------------------------------
enum	
{// this simplifies the offsets in the declaration array
	HAVOKFX_PARTICLE_VERTEX_SIZE_1 = sizeof (D3DXVECTOR3),
};
const D3DC_INPUT_ELEMENT_DESC g_ptParticleVertexDeclarationArray_HavokFXParticle[] =
{
	//						Offset							Semantic	DX9Index	DX10Index	DX9Type		DX10Type			StreamIndex
	D3DC_INPUT_VERTEX(		0,								POSITION,	0,			"",			FLOAT3,		R32G32B32_FLOAT			)
	D3DC_INPUT_VERTEX(		HAVOKFX_PARTICLE_VERTEX_SIZE_1,	TEXCOORD,	0,			"0",		UBYTE4,		R32G32B32A32_FLOAT		)
	D3DC_INPUT_INSTANCE(	0,								TEXCOORD,	1,			"1",		FLOAT4,		R32G32B32A32_FLOAT,	1	)	// position
	D3DC_INPUT_END()
};
#endif // HAVOKFX_ENABLED

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
// Statistics
//-------------------------------------------------------------------------------------------------
extern int gnLastBeginOcclusionModelID;
extern int gnLastEndOcclusionModelID;

static BOOL sgbOcclusionQueryStarted = FALSE;
static PARTICLE_DRAW_STATS sgtParticleDrawStats[ NUM_PARTICLE_DRAW_SECTIONS ] = {0};

static int sgnEffectShaderDefaultBasic = INVALID_ID;
static int sgnEffectShaderDefaultMesh = INVALID_ID;

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dx9_ParticleSystemsReleaseVBuffer()
{
	for ( int i = 0; i < NUM_PARTICLE_VERTEX_BUFFERS; i++ )
	{
		V( dxC_VertexBufferReleaseResource( sgtParticleSystemVertexBuffers[ i ].nD3DBufferID[ 0 ] ) );
		sgtParticleSystemVertexBuffers[ i ].nD3DBufferID[ 0 ] = INVALID_ID;
	}

	sgpParticleScreenTexture = NULL;

	return S_OK;
}

PRESULT dx9_ParticleSystemsReleaseUnmanaged()
{
	sgpParticleScreenTexture = NULL;
	for ( int i = 0; i < NUM_PARTICLE_DRAW_SECTIONS; i++ )
	{
		V( dx9_ReleaseOcclusionQuery( &sgtParticleDrawStats[i].tOcclusion ) );
	}
	sgbOcclusionQueryStarted = FALSE;
	V( dx9_ParticleSystemsReleaseVBuffer() );
	return S_OK;
}

PRESULT dx9_ParticleSystemsRestoreUnmanaged()
{
	for ( int i = 0; i < NUM_PARTICLE_DRAW_SECTIONS; i++ )
	{
		V( dx9_RestoreOcclusionQuery( &sgtParticleDrawStats[i].tOcclusion ) );
	}
	sgbOcclusionQueryStarted = FALSE;
	V( dx9_ParticleSystemsRestoreVBuffer() );
	return S_OK;
}

PRESULT dx9_ParticleSystemsRestoreVBuffer()
{
	// create the particle system vertex buffers;
	D3D_VERTEX_BUFFER_DEFINITION & tVBDefBasic = sgtParticleSystemVertexBuffers[ PARTICLE_VERTEX_BUFFER_BASIC ];
	DX9_BLOCK( tVBDefBasic.dwFVF = D3DFVF_PARTICLE_QUAD_VERTEX; )
	DX10_BLOCK( tVBDefBasic.eVertexType = VERTEX_DECL_PARTICLE_QUAD; )
	tVBDefBasic.nVertexCount = MAX_VERTICES_IN_BASIC_BUFFER;
	tVBDefBasic.nBufferSize[ 0 ] = sgnParticleSystemVertexBufferSizes[ PARTICLE_VERTEX_BUFFER_BASIC ];
	tVBDefBasic.tUsage = D3DC_USAGE_BUFFER_MUTABLE;

	V_RETURN( dxC_CreateVertexBuffer( 0, sgtParticleSystemVertexBuffers[ PARTICLE_VERTEX_BUFFER_BASIC ] ) );
 
#ifdef HAVOKFX_ENABLED
	if ( e_HavokFXIsEnabled() )
	{
		int nBufferSize = sgnParticleSystemVertexBufferSizes[ PARTICLE_VERTEX_BUFFER_HAVOKFX_PARTICLES ];

		D3D_VERTEX_BUFFER_DEFINITION & tVBDefHFX = sgtParticleSystemVertexBuffers[ PARTICLE_VERTEX_BUFFER_HAVOKFX_PARTICLES ];
		tVBDefHFX.eVertexType = VERTEX_DECL_PARTICLE_HAVOKFX_PARTICLE;
		tVBDefHFX.nVertexCount = MAX_VERTICES_IN_HAVOKFX_PARTICLE_BUFFER;
		tVBDefHFX.nBufferSize[ 0 ] = nBufferSize;
		tVBDefHFX.tUsage = D3DC_USAGE_BUFFER_MUTABLE;

		V_DO_SUCCEEDED( dxC_CreateVertexBuffer( 0, sgtParticleSystemVertexBuffers[ PARTICLE_VERTEX_BUFFER_HAVOKFX_PARTICLES ] ) )
		{
			HAVOKFX_PARTICLE_VERTEX pVertices[ MAX_VERTICES_IN_HAVOKFX_PARTICLE_BUFFER ];
			pVertices[ 0 ].vPosition = VECTOR( -1.0f,  1.0f, 0.0f );
			pVertices[ 1 ].vPosition = VECTOR(  1.0f,  1.0f, 0.0f );
			pVertices[ 2 ].vPosition = VECTOR(  1.0f, -1.0f, 0.0f );
			pVertices[ 3 ].vPosition = VECTOR( -1.0f, -1.0f, 0.0f );

			pVertices[ 0 ].pTextureCoords[ 0 ] = 0;
			pVertices[ 0 ].pTextureCoords[ 1 ] = 0;
			pVertices[ 1 ].pTextureCoords[ 0 ] = 0;
			pVertices[ 1 ].pTextureCoords[ 1 ] = 1;
			pVertices[ 2 ].pTextureCoords[ 0 ] = 1;
			pVertices[ 2 ].pTextureCoords[ 1 ] = 1;
			pVertices[ 3 ].pTextureCoords[ 0 ] = 1;
			pVertices[ 3 ].pTextureCoords[ 1 ] = 0;

//#ifdef DX9_MOST_BUFFERS_MANAGED
//			V_RETURN( dxC_UpdateBuffer( 0, sgtParticleSystemVertexBuffers[ PARTICLE_VERTEX_BUFFER_HAVOKFX_PARTICLES ], pVertices, 0, nBufferSize ) );
//#else
			V_RETURN( dxC_UpdateBuffer( 0, sgtParticleSystemVertexBuffers[ PARTICLE_VERTEX_BUFFER_HAVOKFX_PARTICLES ], pVertices, 0, nBufferSize, TRUE ) );
//#endif
		}
	}
#endif
	return S_OK;
}

PRESULT e_ParticleSystemsRestore()
{
	// create the particle system vertex buffer;
	V( dx9_ParticleSystemsRestoreVBuffer() );

	// create index buffer for knots

	for ( int n = 0; n < NUM_KNOT_DRAW_TYPES; n++ )
	{
		D3D_INDEX_BUFFER_DEFINITION & tIBDef = sgtKnotDrawChart[ n ].tIndexBufferDef;
		ASSERT_CONTINUE( tIBDef.nIndexCount != 0 );
		ASSERT_CONTINUE( tIBDef.pLocalData );
		tIBDef.tFormat = D3DFMT_INDEX16;
		tIBDef.tUsage = D3DC_USAGE_BUFFER_REGULAR;
		tIBDef.nBufferSize = tIBDef.nIndexCount * sizeof(WORD);
		V_CONTINUE( dxC_CreateIndexBuffer( tIBDef ) );

		V_CONTINUE( dxC_UpdateBuffer( tIBDef, tIBDef.pLocalData, 0, tIBDef.nBufferSize ) );
		V_CONTINUE( dxC_IndexBufferPreload( tIBDef.nD3DBufferID ) );
	}

	ONCE
	{
		static const WORD pwQuadIndices[ 6 ] = { 0, 1, 2, 0, 2, 3 };

		ASSERT_CONTINUE( ! sgtQuadIndexBuffer.pLocalData );
		sgtQuadIndexBuffer.tFormat = D3DFMT_INDEX16;
		sgtQuadIndexBuffer.tUsage = D3DC_USAGE_BUFFER_REGULAR;
		sgtQuadIndexBuffer.nIndexCount = arrsize( pwQuadIndices );
		sgtQuadIndexBuffer.nBufferSize = sgtQuadIndexBuffer.nIndexCount * sizeof(WORD);
		V_CONTINUE( dxC_CreateIndexBuffer( sgtQuadIndexBuffer ) );

		V_CONTINUE( dxC_UpdateBuffer( sgtQuadIndexBuffer, (void*)pwQuadIndices, 0, sgtQuadIndexBuffer.nBufferSize ) );
		V_CONTINUE( dxC_IndexBufferPreload( sgtQuadIndexBuffer.nD3DBufferID ) );
	}

	// create particle occlusion query
	for ( int i = 0; i < NUM_PARTICLE_DRAW_SECTIONS; i++ )
	{
		V( dx9_RestoreOcclusionQuery( &sgtParticleDrawStats[i].tOcclusion ) );
	}
	sgbOcclusionQueryStarted = FALSE;

	if ( e_ParticlesIsGPUSimEnabled() )
	{
#ifdef ENGINE_TARGET_DX10
		V( dx10_ParticleSystemsGPUInit() );
#endif
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_ParticleSystemsDumpEngineMemUsage()
{
	//ENGINE_MEMORY tMemory;
	//tMemory.Zero();

	//int nDef = DefinitionGetFirstId( DEFINITION_GROUP_PARTICLE );
	//while ( nDef != INVALID_ID )
	//{
	//	PARTICLE_SYSTEM_DEFINITION * pDef = (PARTICLE_SYSTEM_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_PARTICLE, nDef );
	//	if ( pDef )
	//	{
	//		if ( e_TextureIsLoaded( pDef->nTextureId ) )
	//		{
	//			DWORD dwBytes = 0;
	//			V( e_TextureGetSizeInMemory( pDef->nTextureId, &dwBytes ) );
	//			tMemory.dwTextureBytes += dwBytes;
	//		}

	//		dwBytes = 0;
	//		V( e_ModelDefinitionGetSizeInMemory( pDef->nModelDefId, &dwBytes ) );
	//		dwTotalModelBytes += dwBytes;

	//		nDef = DefinitionGetNextId( DEFINITION_GROUP_PARTICLE, nDef );
	//	}
	//}

	//trace( "   Particle systems textures:\n" );
	//trace( "     %d bytes:\n", dwTotalTextureBytes );
	//trace( "   Particle systems models:\n" );
	//trace( "     %d bytes:\n", dwTotalModelBytes );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_ParticleSystemsReleaseDefinitionResources()
{
	int nDef = DefinitionGetFirstId( DEFINITION_GROUP_PARTICLE );
	while ( nDef != INVALID_ID )
	{
		PARTICLE_SYSTEM_DEFINITION * pDef = (PARTICLE_SYSTEM_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_PARTICLE, nDef );
		if ( pDef )
		{
			// I'm not sure we should be checking refcount here at all ...
			if ( pDef->tRefCount.GetCount() == 0 )
			{
				e_TextureReleaseRef( pDef->nTextureId );
				pDef->nTextureId = INVALID_ID;
				e_ModelDefinitionReleaseRef( pDef->nModelDefId, TRUE );
				pDef->nModelDefId = INVALID_ID;
				e_TextureReleaseRef( pDef->nDensityTextureId );
				pDef->nDensityTextureId = INVALID_ID;
				e_TextureReleaseRef( pDef->nVelocityTextureId );
				pDef->nVelocityTextureId = INVALID_ID;
				e_TextureReleaseRef( pDef->nObstructorTextureId );
				pDef->nObstructorTextureId = INVALID_ID;

				CLEAR_MASK( pDef->dwRuntimeFlags, PARTICLE_SYSTEM_DEFINITION_RUNTIME_FLAG_LOADED );
			}
			nDef = DefinitionGetNextId( DEFINITION_GROUP_PARTICLE, nDef );
		}
		else
		{
			ASSERTX_BREAK( 0, "NULL particle definition encountered with non-invalid ID!" );
		}
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_ParticleSystemsReleaseGlobalResources()
{
	// particles
	V( dx9_ParticleSystemsReleaseVBuffer() );
	for ( int n = 0; n < NUM_KNOT_DRAW_TYPES; n++ )
	{
		V( dxC_IndexBufferReleaseResource( sgtKnotDrawChart[ n ].tIndexBufferDef.nD3DBufferID ) );
		sgtKnotDrawChart[ n ].tIndexBufferDef.nD3DBufferID = INVALID_ID;
	}

	for ( int i = 0; i < NUM_PARTICLE_DRAW_SECTIONS; i++ )
	{
		V( dx9_ReleaseOcclusionQuery( &sgtParticleDrawStats[i].tOcclusion ) );
	}
	sgbOcclusionQueryStarted = FALSE;

	for ( int n = 0; n < NUM_KNOT_DRAW_TYPES; n++ )
	{
		if ( sgtKnotDrawChart[ n ].tIndexBufferDef.pLocalData )
		{
#if USE_MEMORY_MANAGER && !ISVERSION(SERVER_VERSION)
			FREE( g_StaticAllocator, sgtKnotDrawChart[ n ].tIndexBufferDef.pLocalData );
#else
			FREE( NULL, sgtKnotDrawChart[ n ].tIndexBufferDef.pLocalData );
#endif		
			
			sgtKnotDrawChart[ n ].tIndexBufferDef.pLocalData = NULL;
		}
	}
	V( dxC_IndexBufferReleaseResource( sgtQuadIndexBuffer.nD3DBufferID ) );
	sgtQuadIndexBuffer.nD3DBufferID = INVALID_ID;
	sgtQuadIndexBuffer.tFormat = D3DFMT_UNKNOWN;
	sgpParticleScreenTexture = NULL;

	if ( e_ParticlesIsGPUSimEnabled() )
	{
#ifdef ENGINE_TARGET_DX10
		dx10_ParticleSystemsFree();
#endif
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT e_ParticlesDrawInit()
{
	int sgnIndexOffsets[ NUM_KNOT_DRAW_TYPES ][ MAX_TRIANGLES_PER_KNOT * 3 ] = 
	{
		{ // KNOT_DRAW_TYPE_TWO_STRIPS
			0, 1, 4,
			1, 4, 6,
			4, 6, 5,
			5, 4, 0,
			2, 3, 4,
			3, 4, 8,
			8, 4, 7,
			4, 7, 2,
		},
		{ // KNOT_DRAW_TYPE_ONE_STRIP_FACE_CAMERA
			0, 3, 2,
			0, 2, 1,
			1, 2, 4,
			2, 3, 4,
		},
		{ // KNOT_DRAW_TYPE_ONE_STRIP_FACE_NORMAL
			0, 3, 2,
			0, 2, 1,
			1, 2, 4,
			2, 3, 4,
		},
		{}, // KNOT_DRAW_TYPE_TUBE_4
		{}, // KNOT_DRAW_TYPE_TUBE_6
		{}, // KNOT_DRAW_TYPE_TUBE_8
	};

	for ( int n = KNOT_DRAW_TYPE_TUBE_4; n <= KNOT_DRAW_TYPE_TUBE_8; n++ )
	{
		{//compute the template
			sgnIndexOffsets[ n ][  0 ] = 0;
			sgnIndexOffsets[ n ][  1 ] = sgtKnotDrawChart[ n ].nMiddleVerticesStart;
			sgnIndexOffsets[ n ][  2 ] = 1;

			sgnIndexOffsets[ n ][  3 ] = 0;
			sgnIndexOffsets[ n ][  4 ] = sgtKnotDrawChart[ n ].nVerticesPerKnot;
			sgnIndexOffsets[ n ][  5 ] = sgtKnotDrawChart[ n ].nMiddleVerticesStart;

			sgnIndexOffsets[ n ][  6 ] = 1;
			sgnIndexOffsets[ n ][  7 ] = sgtKnotDrawChart[ n ].nMiddleVerticesStart;
			sgnIndexOffsets[ n ][  8 ] = sgtKnotDrawChart[ n ].nVerticesPerKnot + 1;

			sgnIndexOffsets[ n ][  9 ] = sgtKnotDrawChart[ n ].nMiddleVerticesStart;
			sgnIndexOffsets[ n ][ 10 ] = sgtKnotDrawChart[ n ].nVerticesPerKnot;
			sgnIndexOffsets[ n ][ 11 ] = sgtKnotDrawChart[ n ].nVerticesPerKnot + 1;
		}

		//make the rest using the first as a template
		for ( int i = 1; i < sgtKnotDrawChart[ n ].nFaces; i++ ) 
		{
			int nIndexStart = i * 12;
			for ( int j = 0; j < 12; j++ )
			{
				sgnIndexOffsets[ n ][ nIndexStart + j ] = sgnIndexOffsets[ n ][ j ] + i;
			}
		}
	}

	for ( int n = 0; n < NUM_KNOT_DRAW_TYPES; n++ )
	{
		ASSERT_CONTINUE( sgtKnotDrawChart[ n ].nVerticesPerKnot );
		sgtKnotDrawChart[ n ].nMaxKnotsPerDraw = MAX_VERTICES_IN_BASIC_BUFFER / sgtKnotDrawChart[ n ].nVerticesPerKnot;
		sgtKnotDrawChart[ n ].tIndexBufferDef.nIndexCount = sgtKnotDrawChart[ n ].nMaxKnotsPerDraw * sgtKnotDrawChart[ n ].nTrianglesPerKnot * 3;
		
#if USE_MEMORY_MANAGER && !ISVERSION(SERVER_VERSION)
		sgtKnotDrawChart[ n ].tIndexBufferDef.pLocalData = (BYTE *) MALLOC( g_StaticAllocator, sizeof(WORD) * sgtKnotDrawChart[ n ].tIndexBufferDef.nIndexCount ); 
#else		
		sgtKnotDrawChart[ n ].tIndexBufferDef.pLocalData = (BYTE *) MALLOC( NULL, sizeof(WORD) * sgtKnotDrawChart[ n ].tIndexBufferDef.nIndexCount ); 
#endif
		
		ASSERT_CONTINUE( sgtKnotDrawChart[ n ].tIndexBufferDef.pLocalData );

		for ( int i = 0; i < sgtKnotDrawChart[ n ].nMaxKnotsPerDraw; i++ )
		{
			WORD * pwIndexStart = & ((WORD*)sgtKnotDrawChart[ n ].tIndexBufferDef.pLocalData)[ 3 * sgtKnotDrawChart[ n ].nTrianglesPerKnot * i ];
			int nVertexStart = sgtKnotDrawChart[ n ].nVerticesPerKnot * i;
			for ( int j = 0; j < 3 * sgtKnotDrawChart[ n ].nTrianglesPerKnot; j++ )
			{
				*(pwIndexStart + j) = (WORD)(nVertexStart + sgnIndexOffsets[ n ][ j ]);
			}
		}
	}

	for ( int i = 0; i < NUM_PARTICLE_VERTEX_BUFFERS; i++ )
		dxC_VertexBufferDefinitionInit( sgtParticleSystemVertexBuffers[ i ] );

	sgnEffectShaderDefaultBasic = GlobalIndexGet( GI_PARTICLE_SHADER );
	sgnEffectShaderDefaultMesh  = GlobalIndexGet( GI_PARTICLE_MESH_SHADER );
	ASSERT_RETFAIL( sgnEffectShaderDefaultBasic != INVALID_ID );
	ASSERT_RETFAIL( sgnEffectShaderDefaultMesh  != INVALID_ID );

	sgeParticleCurrentDrawSection = PARTICLE_DRAW_SECTION_INVALID;
	for ( int i = 0; i < NUM_PARTICLE_DRAW_SECTIONS; i++ )
	{
		V( dx9_ReleaseOcclusionQuery( &sgtParticleDrawStats[i].tOcclusion ) );
	}
	sgbOcclusionQueryStarted = FALSE;

	ZeroMemory( &sgtParticleDrawStats, sizeof(PARTICLE_DRAW_STATS) * NUM_PARTICLE_DRAW_SECTIONS );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void e_ParticlesAddTotalWorldArea( float fArea )
{
	ASSERT_RETURN( IS_VALID_INDEX( sgeParticleCurrentDrawSection, NUM_PARTICLE_DRAW_SECTIONS ) );
	sgtParticleDrawStats[ sgeParticleCurrentDrawSection ].fWorldArea += fArea;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void e_ParticlesSetPredictedScreenArea( float fArea )
{
	ASSERT_RETURN( IS_VALID_INDEX( sgeParticleCurrentDrawSection, NUM_PARTICLE_DRAW_SECTIONS ) );
	sgtParticleDrawStats[ sgeParticleCurrentDrawSection ].fScreenAreaPredicted = fArea;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static inline void sParticlesAddDrawn( int nCount = 1 )
{
	ASSERT_RETURN( IS_VALID_INDEX( sgeParticleCurrentDrawSection, NUM_PARTICLE_DRAW_SECTIONS ) );
	sgtParticleDrawStats[ sgeParticleCurrentDrawSection ].nDrawn += nCount;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL e_ParticleDefinitionsCanDrawTogether( const PARTICLE_SYSTEM_DEFINITION * pFirst, const PARTICLE_SYSTEM_DEFINITION * pSecond )
{

	if ( ( pFirst ->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_DRAW_ROPE ) != 0 ||
		 ( pSecond->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_DRAW_ROPE ) != 0  )
		return FALSE;

	if ( ( pFirst ->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_TEXTURE_ADDITIVE_BLEND ) !=
		 ( pSecond->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_TEXTURE_ADDITIVE_BLEND ) )
		return FALSE;

	if ( ( pFirst ->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_GLOW ) !=
		 ( pSecond->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_GLOW ) )
		return FALSE;

	if ( ( pFirst ->dwFlags2 & PARTICLE_SYSTEM_DEFINITION_FLAG2_GLOWCONSTANT ) !=
		 ( pSecond->dwFlags2 & PARTICLE_SYSTEM_DEFINITION_FLAG2_GLOWCONSTANT ) )
		return FALSE;

	if ( ( pFirst ->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_TEXTURE_MULTIPLY_BLEND ) !=
		 ( pSecond->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_TEXTURE_MULTIPLY_BLEND ) )
		return FALSE;

	return TRUE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
EFFECT_TECHNIQUE * e_ParticleSystemGetTechnique(
	PARTICLE_SYSTEM_DEFINITION * pDefinition,
	D3D_EFFECT * pEffect )
{
	DWORD dwTechniqueFlags = 0;

#ifdef ENGINE_TARGET_DX10
	// TYLER - This is where you would override for no soft particles.
	if ( (pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_DX10_DONT_USE_SOFT_PARTICLES) == 0 )
		dwTechniqueFlags |= TECHNIQUE_FLAG_PARTICLE_SOFT_PARTICLES;
#endif

	if ( ( pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_GLOW ) != 0 )
	{
		dwTechniqueFlags |= TECHNIQUE_FLAG_PARTICLE_GLOW;

		if ( pDefinition->dwFlags2 & PARTICLE_SYSTEM_DEFINITION_FLAG2_GLOWCONSTANT ) 
			dwTechniqueFlags |= TECHNIQUE_FLAG_PARTICLE_GLOW_CONSTANT;
	}

	if ( pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_TEXTURE_ADDITIVE_BLEND )
		dwTechniqueFlags |= TECHNIQUE_FLAG_PARTICLE_ADDITIVE;

	if ( pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_TEXTURE_MULTIPLY_BLEND )
		dwTechniqueFlags |= TECHNIQUE_FLAG_PARTICLE_MULTIPLY;

	int nTechniqueIndex = 0;

	// AE 2008.01.07: We needed to support DXT5 textures that were not normal maps, 
	// so that we could use the alpha channel for alpha testing. Currently, the 
	// particle distortion shader is the only particle shader that is a screen 
	// effect. If more particle shaders are added that have multiple techniques, 
	// then we will want to add an additional flag to differentiate.
	EFFECT_DEFINITION * pEffectDef = e_ParticleSystemGetEffectDef( pDefinition );
	if ( pEffectDef && TESTBIT_MACRO_ARRAY( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_IS_SCREEN_EFFECT ) )
	{
		D3D_TEXTURE * pTexture = dxC_TextureGet( pDefinition->nTextureId );
		if ( pTexture )
		{
			TEXTURE_DEFINITION* pTexDef = (TEXTURE_DEFINITION*)DefinitionGetById( 
				DEFINITION_GROUP_TEXTURE, pTexture->nDefinitionId );
			
			if ( ! TEST_MASK( pTexDef->dwConvertFlags, TEXTURE_CONVERT_FLAG_DXT5NORMALMAP ) )
				nTechniqueIndex = 1;
		}
	}


	EFFECT_TECHNIQUE * ptTechnique = NULL;
	TECHNIQUE_FEATURES tFeat;
	tFeat.Reset();
	tFeat.dwFlags = dwTechniqueFlags;
	tFeat.nInts[ TF_INT_INDEX ] = nTechniqueIndex;
	V( dxC_EffectGetTechniqueByFeatures( pEffect, tFeat, &ptTechnique ) );
	return ptTechnique;
	//return dx9_EffectGetTechniquePtrParticle( pEffect, dwTechniqueFlags );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static void sSetParticleRenderStates(
	PARTICLE_SYSTEM_DEFINITION * pDefinition,
	int nPass,
	BOOL bParticleMesh,
	BOOL bDepthOnly )
{
	// Read depth if not marked otherwise
	BOOL bReadDepth = ( pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_DISABLE_READ_Z ) == 0;

#ifdef DX10_SOFT_PARTICLES
	if ( bParticleMesh )
	{
		// Read depth if not marked otherwise
	}
	else if ( ( pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_DX10_DONT_USE_SOFT_PARTICLES ) == 0 )
	{
		// Don't read depth for actual non-mesh soft particles
		bReadDepth = FALSE;
	}
#endif
	V( dxC_SetRenderState( D3DRS_ZENABLE, bReadDepth ? D3DZB_TRUE : D3DZB_FALSE ) );


#ifndef DXC_DEFINE_SOME_STATE_IN_FX
	// why not allow zwrite for normal particles too? Especially good for foliage.
	V( dxC_SetRenderState( D3DRS_ZWRITEENABLE, ( pDefinition->dwFlags2 & PARTICLE_SYSTEM_DEFINITION_FLAG2_WRITE_Z ) != 0 ) );
	// if we're doing zwrite for particles, we tend to want to boost the alphawrite threshold way up high
	// to prevent haloing.
	if( ( pDefinition->dwFlags2 & PARTICLE_SYSTEM_DEFINITION_FLAG2_WRITE_Z ) != 0 )
	{
		V( dxC_SetRenderState(D3DRS_ALPHAREF,			(DWORD)254) );
	}
	else
	{
		V( dxC_SetRenderState(D3DRS_ALPHAREF,			(DWORD)1) );
	}

	BOOL bAlphaTest = TRUE;
	if ( (    ( pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_TEXTURE_ADDITIVE_BLEND )
		   && ( pDefinition->dwFlags2 & PARTICLE_SYSTEM_DEFINITION_FLAG2_GLOWCONSTANT ) )
		 ||	( pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_TEXTURE_MULTIPLY_BLEND ) )
		bAlphaTest = FALSE;
	dxC_SetRenderState( D3DRS_ALPHATESTENABLE, bAlphaTest ); 
	dxC_SetRenderState( D3DRS_ALPHABLENDENABLE, ! bDepthOnly );
	int nBlendOp = D3DBLENDOP_ADD;

	if ( ( pDefinition->dwFlags2 & PARTICLE_SYSTEM_DEFINITION_FLAG2_DRAW_FRONT_FACE_ONLY ) != 0 &&
		pDefinition->nModelDefId != INVALID_ID )
	{
		V( dxC_SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW ) );
	} else {
		V( dxC_SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE ) );
	}

	if ( pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_TEXTURE_ADDITIVE_BLEND )
	{

		if ( pDefinition->dwFlags2 & PARTICLE_SYSTEM_DEFINITION_FLAG2_GLOWCONSTANT )
		{
			dxC_SetRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );
			dxC_SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );
#ifdef ENGINE_TARGET_DX10
			dxC_SetRenderState( D3DRS_SRCBLENDALPHA, D3DBLEND_ONE );
			dxC_SetRenderState( D3DRS_DESTBLENDALPHA, D3DBLEND_ONE );				
#endif
		}
		else
		{
			dxC_SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
			dxC_SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );
#ifdef ENGINE_TARGET_DX10
			dxC_SetRenderState( D3DRS_SRCBLENDALPHA, D3DBLEND_SRCALPHA );
			dxC_SetRenderState( D3DRS_DESTBLENDALPHA, D3DBLEND_ONE );				
#endif
		}
	}
	else if ( pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_TEXTURE_MULTIPLY_BLEND )
	{

		dxC_SetRenderState( D3DRS_SRCBLEND, D3DBLEND_ZERO );
		dxC_SetRenderState( D3DRS_DESTBLEND, D3DBLEND_SRCCOLOR );
#ifdef ENGINE_TARGET_DX10
		dxC_SetRenderState( D3DRS_SRCBLENDALPHA, D3DBLEND_ZERO );
		dxC_SetRenderState( D3DRS_DESTBLENDALPHA, D3DBLEND_ONE );				
#endif
	}
	else if ( ( pDefinition->dwFlags2 & PARTICLE_SYSTEM_DEFINITION_FLAG2_GLOWCONSTANT ) && nPass > 0 )
	{
		dxC_SetRenderState( D3DRS_SRCBLEND, D3DBLEND_ONE );
		dxC_SetRenderState( D3DRS_DESTBLEND, D3DBLEND_ONE );
#ifdef ENGINE_TARGET_DX10
		dxC_SetRenderState( D3DRS_SRCBLENDALPHA, D3DBLEND_ONE );
		dxC_SetRenderState( D3DRS_DESTBLENDALPHA, D3DBLEND_ONE );				
#endif
		nBlendOp = D3DBLENDOP_MAX;
	}
	else
	{
		dxC_SetRenderState( D3DRS_SRCBLEND, D3DBLEND_SRCALPHA );
		dxC_SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
#ifdef ENGINE_TARGET_DX10
		dxC_SetRenderState( D3DRS_SRCBLENDALPHA, D3DBLEND_SRCALPHA );
		dxC_SetRenderState( D3DRS_DESTBLENDALPHA, D3DBLEND_INVSRCALPHA );				
#endif
	}

	dxC_SetRenderState( D3DRS_BLENDOP, nBlendOp );
#ifdef ENGINE_TARGET_DX10
	dxC_SetRenderState( D3DRS_BLENDOPALPHA, nBlendOp );
#endif

	if( (pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_GLOW) != 0 )
	{
		if ((pDefinition->dwFlags2 & PARTICLE_SYSTEM_DEFINITION_FLAG2_GLOWCONSTANT ) != 0 &&
			(pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_TEXTURE_ADDITIVE_BLEND) == 0 )
		{
			if ( nPass == 0 )
				dxC_SetRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE );
			else
				dxC_SetRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA );
		} else {
			dxC_SetRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA );
		}
	}
	else
		dxC_SetRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE  );
#endif

}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL e_ParticleSystemUsesProj2(
	PARTICLE_SYSTEM * pParticleSystem )
{
	return FALSE;
	//BOOL bUseProj2 = FALSE;
	//if ( pParticleSystem->dwFlags & PARTICLE_SYSTEM_FLAG_FIRST_PERSON )
	//	bUseProj2 = TRUE;
	//if ( pParticleSystem->pDefinition->dwFlags2 & PARTICLE_SYSTEM_DEFINITION_FLAG2_DONT_CHANGE_FOV )
	//	bUseProj2 = FALSE;
	//return bUseProj2;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT e_ParticlesDrawMeshes( 
	PARTICLE_DRAW_INFO & tDrawInfo,
	BOOL bDepthOnly )
{
	if ( ! tDrawInfo.nMeshesInBatchCurr || ! tDrawInfo.pParticleSystemCurr )
		return S_FALSE;

	PRESULT pr = E_FAIL;

	ONCE
	{
		D3D_EFFECT* pEffect = NULL;

		if ( bDepthOnly )
			pEffect = dxC_EffectGet( dx9_EffectGetNonMaterialID( NONMATERIAL_EFFECT_ZBUFFER ) );
		else 
			pEffect = e_ParticleSystemGetEffect( tDrawInfo.pParticleSystemCurr->pDefinition );

		if ( !pEffect || ! dxC_EffectIsLoaded( pEffect ) )
		{
			pr = S_OK;
			break;
		}

		PARTICLE_SYSTEM_DEFINITION * pDefinition = tDrawInfo.pParticleSystemCurr->pDefinition;
		ASSERT_BREAK( pDefinition );
		EFFECT_TECHNIQUE * pTechnique = NULL;

		if ( bDepthOnly )
		{
			TECHNIQUE_FEATURES tFeat;
			tFeat.Reset();
			tFeat.nInts[ TF_INT_INDEX ] = 5;
			V_SAVE_ACTION( pr, dxC_EffectGetTechniqueByFeatures( pEffect, tFeat, &pTechnique ), break );
		}
		else
			pTechnique = e_ParticleSystemGetTechnique( pDefinition, pEffect );
		if ( ! pTechnique )
		{
			pr = S_OK;
			break;
		}
		ASSERT_BREAK( pTechnique->hHandle );

		ASSERT_BREAK( pDefinition->nModelDefId != INVALID_ID );
		D3D_MODEL_DEFINITION * pModelDef = dxC_ModelDefinitionGet( pDefinition->nModelDefId, LOD_LOW );
		if ( ! pModelDef )
		{
			pr = S_OK;
			break;
		}

		UINT nPassCount;
		V_RETURN( dxC_SetTechnique( pEffect, pTechnique, &nPassCount ) );

		V( dx9_EffectSetMatrix ( pEffect, *pTechnique, EFFECT_PARAM_VIEWPROJECTIONMATRIX,
			e_ParticleSystemUsesProj2( tDrawInfo.pParticleSystemCurr ) ? &tDrawInfo.matViewProjection2 : &tDrawInfo.matViewProjection ) );

		V( dx9_EffectSetRawValue ( pEffect, *pTechnique, EFFECT_PARAM_PARTICLEMESH_WORLDS,
			&sgpfMeshMatrices[ 0 ], 0, MAX_PARTICLE_MESHES_PER_BATCH * 3 * 4 * sizeof( float ) ) );

		V( dx9_EffectSetRawValue ( pEffect, *pTechnique, EFFECT_PARAM_PARTICLEMESH_COLORS,
			&sgpfMeshColors[ 0 ],	0, MAX_PARTICLE_MESHES_PER_BATCH * 4 * sizeof( float ) ) );

		// CHB 2006.10.11
#if 1
		V( dx9_EffectSetFloatArray(pEffect, *pTechnique, EFFECT_PARAM_PARTICLEMESH_GLOWS, &sgpfMeshGlows[ 0 ], MAX_PARTICLE_MESHES_PER_BATCH) );
#else
		V( dx9_EffectSetRawValue ( pEffect, *pTechnique, EFFECT_PARAM_PARTICLEMESH_GLOWS,
			&sgpfMeshGlows[ 0 ],	0, MAX_PARTICLE_MESHES_PER_BATCH * sizeof( float ) ) );
#endif

		D3DXVECTOR4 vEyePos;
		*(VECTOR*)&vEyePos = tDrawInfo.vEyePosition;
		V( dx9_EffectSetVector( pEffect, *pTechnique, EFFECT_PARAM_EYEINOBJECT, &vEyePos ) );  // I know, it's in world, not object

		V( dx9_EffectSetFogParameters( pEffect, *pTechnique, 1.0f ) );

		D3D_MESH_DEFINITION * pMeshDef = dx9_ModelDefinitionGetMesh ( pModelDef, PARTICLE_MODEL_VERTEX_TYPE_MULTI );
		ASSERT_BREAK( pMeshDef );

		V_BREAK( dxC_SetIndices( pMeshDef->tIndexBufferDef ) );

		D3D_VERTEX_BUFFER_DEFINITION & tVertexBufferDef = pModelDef->ptVertexBufferDefs[ pMeshDef->nVertexBufferIndex ];
		V_BREAK( dx9_SetFVF( D3DC_FVF_NULL ) );
		V_BREAK( dxC_SetVertexDeclaration( tVertexBufferDef.eVertexType, pEffect ) );

		V_BREAK( dxC_SetStreamSource( tVertexBufferDef, 0 ) );

		if ( pDefinition->nTextureId != INVALID_ID )
		{
			D3D_TEXTURE * pTexture = dxC_TextureGet( pDefinition->nTextureId );
			if ( tDrawInfo.bPixelShader )
			{
				V( dx9_EffectSetTexture( pEffect, *pTechnique, EFFECT_PARAM_DIFFUSEMAP0, pTexture->GetShaderResourceView() ) );
			} else {
				V( dxC_SetTexture( 0, pTexture->GetShaderResourceView() ) );
			}
			dx9_ReportSetTexture( pTexture->nId );
		}


		for ( int i = 0; i < (int) nPassCount; i++ )
		{
			V_BREAK( dxC_EffectBeginPass( pEffect, i ) );

			sSetParticleRenderStates( pDefinition, i, TRUE, bDepthOnly );

			V_BREAK( dxC_DrawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, 0, tVertexBufferDef.nVertexCount, 
				0, pMeshDef->wTriangleCount * tDrawInfo.nMeshesInBatchCurr,
				pEffect, METRICS_GROUP_PARTICLE ) );
		}

		pr = S_OK;
	}

	tDrawInfo.nMeshesInBatchCurr = 0;

	return pr;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT e_ParticlesDrawBasic(	
	PARTICLE_DRAW_INFO & tDrawInfo,
	BOOL bClearBuffer,
	BOOL bDepthOnly )
{
	if ( tDrawInfo.nVerticesInBuffer == 0 ||
		 tDrawInfo.nTrianglesInBatchCurr == 0 )
	{
		tDrawInfo.nVerticesInBuffer = 0;
		tDrawInfo.nFirstVertexOfBatch = 0;
		return S_OK;
	}

	D3D_INDEX_BUFFER tStandinIB = {0};
	tStandinIB.nId = INVALID_ID;
	D3D_INDEX_BUFFER * pIB;
	int nIndexBufferCur = tDrawInfo.nIndexBufferCurr;
	if ( nIndexBufferCur < 0 )
		pIB = &tStandinIB;
	else
		pIB = dxC_IndexBufferGet( nIndexBufferCur );
	ASSERT_RETFAIL( pIB );

	ASSERT_RETFAIL( tDrawInfo.pParticleSystemCurr );
	PARTICLE_SYSTEM_DEFINITION * pDefinition = tDrawInfo.pParticleSystemCurr->pDefinition;
	ASSERT_RETFAIL( pDefinition );

	D3DXVECTOR4 vEyePos;

	D3D_EFFECT* pEffect = NULL;

	if ( bDepthOnly )
		pEffect = dxC_EffectGet( dx9_EffectGetNonMaterialID( NONMATERIAL_EFFECT_ZBUFFER ) );
	else 
		pEffect = e_ParticleSystemGetEffect( pDefinition );

	if ( ! dxC_EffectIsLoaded( pEffect ) )
		goto draw_basic_end;

	EFFECT_TECHNIQUE * pTechnique = NULL;

	if ( bDepthOnly )
	{
		TECHNIQUE_FEATURES tFeat;
		tFeat.Reset();
		tFeat.nInts[ TF_INT_INDEX ] = 4;
		V_RETURN( dxC_EffectGetTechniqueByFeatures( pEffect, tFeat, &pTechnique ) );
	}
	else
		pTechnique = e_ParticleSystemGetTechnique( pDefinition, pEffect );

	if ( ! pTechnique )
	{
		goto draw_basic_end;
	}
	EFFECT_TECHNIQUE & tTechnique = *pTechnique;
	ASSERT_RETFAIL( tTechnique.hHandle );

	UINT nPassCount;
	V_RETURN( dxC_SetTechnique( pEffect, &tTechnique, &nPassCount ) );
	EFFECT_DEFINITION * pEffectDef = e_ParticleSystemGetEffectDef( pDefinition );
	if ( pEffectDef && TESTBIT_MACRO_ARRAY( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_IS_SCREEN_EFFECT ) )
	{
		if ( bDepthOnly )
			goto draw_basic_end;

		// TRAVIS: I'm setting screensize and viewport here so that i can xform the backbuffer UVS for
		// different viewport configs
		RENDER_TARGET_INDEX nRT = dxC_GetSwapChainRenderTargetIndex( dxC_GetCurrentSwapChainIndex(), SWAP_RT_BACKBUFFER );
		V( dxC_EffectSetScreenSize( pEffect, tTechnique, nRT ) );
		V( dxC_EffectSetViewport( pEffect, tTechnique, nRT ) );
	}
	V( dx9_EffectSetBool( pEffect, tTechnique, EFFECT_PARAM_FOGADDITIVEPARTICLELUM, FALSE ) );

	MATRIX * pmViewProj = e_ParticleSystemUsesProj2( tDrawInfo.pParticleSystemCurr ) ? &tDrawInfo.matViewProjection2 : &tDrawInfo.matViewProjection;

	*(VECTOR*)&vEyePos = tDrawInfo.vEyePosition;
	V( dx9_EffectSetVector( pEffect, tTechnique, EFFECT_PARAM_EYEINWORLD, &vEyePos ) );

	V( dx9_EffectSetMatrix( pEffect, tTechnique, EFFECT_PARAM_WORLDVIEWPROJECTIONMATRIX, pmViewProj ) );

	//NVTL: set the soft particle params
	V( dx9_EffectSetFloat(pEffect, *pTechnique, EFFECT_PARAM_SOFTPARTICLE_CONTRAST, pDefinition->fSoftParticleContrast) );
	V( dx9_EffectSetFloat(pEffect, *pTechnique, EFFECT_PARAM_SOFTPARTICLE_SCALE,    pDefinition->fSoftParticleScale) );

#ifdef DX10_PARTICLES_SET_POINTLIGHTPOS
	if( dx9_EffectIsParamUsed( pEffect, tTechnique, EFFECT_PARAM_POINTLIGHTPOS_WORLD ) && dx9_EffectIsParamUsed( pEffect, tTechnique, EFFECT_PARAM_POINTLIGHTFALLOFF_WORLD ) )
	{
		PERF( DRAW_SELECTLIGHTS );

		int pnLights[ MAX_LIGHTS_PER_EFFECT ];
        int nLights;

        //NVTL: we want the lights closer to the particle system. Not to the View...
        VECTOR &vPPos = tDrawInfo.pParticleSystemCurr->vPosition;
		const int nMaxLights = 2;
		V( dx9_MakePointLightList( pnLights, nLights, vPPos, LIGHT_FLAG_DEFINITION_APPLIED, LIGHT_FLAG_SPECULAR_ONLY, MAX_LIGHTS_PER_EFFECT ) );

		BOUNDING_SPHERE tBS( vPPos, 1.f );
		V( e_LightSelectAndSortPointList(
			tBS,
			pnLights,
			nLights,
			pnLights,
			nLights,
			nMaxLights,
			0,
			0 ) );
		V( dx9_EffectSetFloat( pEffect, tTechnique,EFFECT_PARAM_POINTLIGHTCOUNT, (float)nLights ) );

		D3DXVECTOR4 pvPos[nMaxLights];
		D3DXVECTOR4 pvColors[nMaxLights];
		D3DXVECTOR4 pvFalloff[nMaxLights];

		for(int i = 0; i < nLights; ++i )
		{
			D3D_LIGHT* pLight = dx9_LightGet( pnLights[ i ] );
			ASSERT( pLight );
			pvPos[ i ] = D3DXVECTOR4( pLight->vPosition.fX,pLight->vPosition.fY, pLight->vPosition.fZ, 1 );

			//setting original color and intensity
			float intensity = pLight->vFalloff.z;
			pvColors[ i ] = D3DXVECTOR4(pLight->vDiffuse.x/intensity, pLight->vDiffuse.y/intensity,pLight->vDiffuse.z/intensity, intensity );
			pvFalloff[ i ] = D3DXVECTOR4( pLight->vFalloff.x,pLight->vFalloff.y, pLight->vFalloff.z, pLight->vFalloff.w );
		}		

		V( dx9_EffectSetVectorArray( pEffect, tTechnique,EFFECT_PARAM_POINTLIGHTPOS_WORLD,		pvPos,		nLights ) );
		V( dx9_EffectSetVectorArray( pEffect, tTechnique,EFFECT_PARAM_POINTLIGHTCOLOR,			pvColors,	nLights ) );
		V( dx9_EffectSetVectorArray( pEffect, tTechnique,EFFECT_PARAM_POINTLIGHTFALLOFF_WORLD,	pvFalloff,	nLights ) );
	
		ENVIRONMENT_DEFINITION * pEnvDef = e_GetCurrentEnvironmentDef();

		if ( pEnvDef )
			dx9_EffectSetDirectionalLightingParams( NULL, pEffect, pEffectDef, NULL, tTechnique, pEnvDef, EFFECT_SET_DIR_DONT_MULTIPLY_INTENSITY );
	}
#endif

	int nVerticesToDraw = tDrawInfo.nVerticesInBuffer - tDrawInfo.nFirstVertexOfBatch;
	int nLockSize = nVerticesToDraw * sizeof( PARTICLE_QUAD_VERTEX );
	int nLockOffset = tDrawInfo.nFirstVertexOfBatch * sizeof( PARTICLE_QUAD_VERTEX );
	ASSERT_RETFAIL( nLockOffset + nLockSize <= sgnParticleSystemVertexBufferSizes[ PARTICLE_VERTEX_BUFFER_BASIC ] );

#ifdef INTEL_CHANGES
	// select the appropriate vertex buffer based on nParticleSystemAsyncIndex, then point at the proper section
	// indentified by nVerticesMultiplier
	PARTICLE_QUAD_VERTEX * pVerticesStart = (
			sGetVerticesBufferForIndex( tDrawInfo.nParticleSystemAsyncIndex, tDrawInfo.pParticleSystemCurr ) +
			( tDrawInfo.nVerticesMultiplier * MAX_VERTICES_IN_BASIC_BUFFER )
	);
	V_RETURN( dxC_UpdateBuffer( 0, sgtParticleSystemVertexBuffers[ PARTICLE_VERTEX_BUFFER_BASIC ], ( pVerticesStart + tDrawInfo.nFirstVertexOfBatch ), nLockOffset, nLockSize, TRUE ) );
#else
//#ifdef DX9_MOST_BUFFERS_MANAGED
//	V_RETURN( dxC_UpdateBuffer( 0, sgtParticleSystemVertexBuffers[ PARTICLE_VERTEX_BUFFER_BASIC ], &sgpBasicVertices[ tDrawInfo.nFirstVertexOfBatch ], nLockOffset, nLockSize ) );
//#else
	V_RETURN( dxC_UpdateBuffer( 0, sgtParticleSystemVertexBuffers[ PARTICLE_VERTEX_BUFFER_BASIC ], &sgpBasicVertices[ tDrawInfo.nFirstVertexOfBatch ], nLockOffset, nLockSize, TRUE ) );
//#endif
#endif

	V_RETURN( dxC_SetIndices( (*pIB).pD3DIndices, D3DFMT_INDEX16 ) );  //TYLER TODO: Are particle systems always using 16 bit indices?
	V_RETURN( dxC_SetStreamSource( sgtParticleSystemVertexBuffers[ PARTICLE_VERTEX_BUFFER_BASIC ], 0 ) );

	if ( dx9_EffectIsParamUsed( pEffect, tTechnique, EFFECT_PARAM_PARTICLE_FADEIN ))
	{
		PARTICLE_SYSTEM * pParticleSystem = tDrawInfo.pParticleSystemCurr;
		D3DXVECTOR4 vFadeIn;
		vFadeIn.x = pParticleSystem->fAlphaRef;
		vFadeIn.y = min( pParticleSystem->fAlphaMin, 0.99f );
		vFadeIn.z = 1.0f / (1.0f - vFadeIn.y);
		//vFadeIn.w = 0.4f + 0.2f * sin( 6.0f * PI * pParticleSystem->fPathTime ); // lerp value for the diffuse textures
		vFadeIn.w = 0.5f;

		V( dx9_EffectSetVector( pEffect, tTechnique, EFFECT_PARAM_PARTICLE_FADEIN, &vFadeIn ) );
	}

	if ( dx9_EffectIsParamUsed( pEffect, tTechnique, EFFECT_PARAM_PARTICLE_TEXTURE_MATRIX ) )
	{
		PARTICLE_SYSTEM * pParticleSystem = tDrawInfo.pParticleSystemCurr;
		D3DXVECTOR4 pvTextureMatrix[ 4 ];
		// the rotation
		D3DXMATRIX mRotation;
		//float fScaling = 0.85f + 0.3f * sin( 3.0f * pParticleSystem->fPathTime * PI );
		float fScaling = 0.85f;
		float fRotation = pParticleSystem->fPathTime * 0.9f;
		D3DXVECTOR2 vCenter( 0.75f, 0.25f );
		D3DXVECTOR2 vTranslation;
		vTranslation.x = pParticleSystem->fPathTime * -1.0f;
		vTranslation.y = pParticleSystem->fPathTime * 0.0f;
		D3DXMatrixAffineTransformation2D( &mRotation, fScaling, &vCenter, fRotation, &vTranslation );
		pvTextureMatrix[ 0 ].x = mRotation._11;
		pvTextureMatrix[ 0 ].y = mRotation._21;
		pvTextureMatrix[ 0 ].z = mRotation._41;
		pvTextureMatrix[ 1 ].x = mRotation._12;
		pvTextureMatrix[ 1 ].y = mRotation._22;
		pvTextureMatrix[ 1 ].z = mRotation._42;

		//fScaling = 0.9f + 0.2f * cos( 4.0f * pParticleSystem->fPathTime * PI );
		fScaling = 0.9f;
		fRotation = pParticleSystem->fPathTime * -1.0f;
		vCenter.x = 0.25f;
		vCenter.y = 0.75f;
		vTranslation.x = pParticleSystem->fPathTime * 1.0f;
		vTranslation.y = pParticleSystem->fPathTime * -1.0f;
		D3DXMatrixAffineTransformation2D( &mRotation, fScaling, &vCenter, fRotation, &vTranslation );
		pvTextureMatrix[ 2 ].x = mRotation._11;
		pvTextureMatrix[ 2 ].y = mRotation._21;
		pvTextureMatrix[ 2 ].z = mRotation._41;
		pvTextureMatrix[ 3 ].x = mRotation._12;
		pvTextureMatrix[ 3 ].y = mRotation._22;
		pvTextureMatrix[ 3 ].z = mRotation._42;

		V( dx9_EffectSetVectorArray( pEffect, tTechnique, EFFECT_PARAM_PARTICLE_TEXTURE_MATRIX, pvTextureMatrix, 4 ) );
	}

	if ( dx9_EffectIsParamUsed( pEffect, tTechnique, EFFECT_PARAM_UV_SCALE ) )
	{
		PARTICLE_SYSTEM * pParticleSystem = tDrawInfo.pParticleSystemCurr;

		VECTOR pvCorners[ 4 ];
		// specifies which corners we want, in which order
		int nOffsets[ 4 ] = { 0, 1, 2, 5 };
#ifdef INTEL_CHANGES
		// select the appropriate vertex buffer based on nParticleSystemAsyncIndex, then point at the proper section
		// indentified by nVerticesMultiplier
		PARTICLE_QUAD_VERTEX * pVerticesStart = (
			sGetVerticesBufferForIndex( tDrawInfo.nParticleSystemAsyncIndex, tDrawInfo.pParticleSystemCurr ) +
			( tDrawInfo.nVerticesMultiplier * sgnParticleSystemVertexBufferSizes[ PARTICLE_VERTEX_BUFFER_BASIC ] )
		);
		PARTICLE_QUAD_VERTEX * pVertex = NULL;
		for ( int i = 0; i < 4; i++ )
		{
			pVertex = pVerticesStart + tDrawInfo.nFirstVertexOfBatch + nOffsets[ i ];
			pvCorners[ i ] = pVertex->vPosition;
		}
#else
		for ( int i = 0; i < 4; i++ )
		{
			PARTICLE_QUAD_VERTEX * pVertex = &sgpBasicVertices[ tDrawInfo.nFirstVertexOfBatch + nOffsets[ i ] ];
			pvCorners[ i ] = pVertex->vPosition;
		}
#endif
		// Chris, do that voodoo that you do 

		// find the portal
		PORTAL * pPortal = e_PortalGet( pParticleSystem->nPortalID );
		if ( ! pPortal )
			goto draw_basic_end;

		// this is the less-ideal way to do it -- should be getting the data from something else and setting it later
		//if ( 0 == ( pPortal->dwFlags & PORTAL_FLAG_SHAPESET ) )
		//{
		//	V( e_PortalSetShape( pPortal, pvCorners, 4, NULL ) );
		//}
		//if ( 0 == ( pPortal->dwFlags & PORTAL_FLAG_TARGETSET ) )
		//{
		//	V( e_PortalConnect( pPortal ) );
		//}

		// update all portals, but only render the visible ones
		//if ( ! e_PortalIsActive( pPortal ) ||
		//	 ! e_PortalHasFlags( pPortal, PORTAL_FLAG_VISIBLE ) )
		//	 goto draw_basic_end;

		//LPD3DCSHADERRESOURCEVIEW pPortalTex = dx9_PortalGetRenderedTextureShaderResource( pPortal );//dx9_PortalGetRenderedTexture( pPortal );
		//if ( pPortalTex )
		//{
		//	// assume pixel shader for now
		//	ASSERT( tDrawInfo.bPixelShader );
		//	V( dx9_EffectSetTexture( pEffect, tTechnique, EFFECT_PARAM_LIGHTMAP, (LPD3DCBASESHADERRESOURCEVIEW)pPortalTex ) );
		//	// no d3d_texture to report... it's a render target
		//}

		D3D_TEXTURE * pUtilTex = dxC_TextureGet( e_GetUtilityTexture( TEXTURE_RGB_00_A_FF ) );
		if ( pUtilTex )
		{
			V( dx9_EffectSetTexture( pEffect, tTechnique, EFFECT_PARAM_DIFFUSEMAP0, pUtilTex->GetShaderResourceView() ) );
		}

/*
		V( dxC_EffectSetPixelAccurateOffset( pEffect, tTechnique ) );

		int nWidth, nHeight;
		dxC_GetRenderTargetDimensions( nWidth, nHeight, dxC_GetCurrentRenderTarget() );

		int nRTWidth, nRTHeight;
		dxC_GetRenderTargetDimensions( nRTWidth, nRTHeight, dx9_PortalGetRenderTarget( pPortal )  );
		// since the rendertarget is likely larger than the screen, set the UV scale factors
		D3DXVECTOR4 vUVScale;
		vUVScale.x = (float)nWidth / nRTWidth;
		vUVScale.y = (float)nHeight / nRTHeight;
		V( dx9_EffectSetVector( pEffect, tTechnique, EFFECT_PARAM_UV_SCALE, &vUVScale ) );
*/
	}

	if ( dx9_EffectIsParamUsed( pEffect, tTechnique, EFFECT_PARAM_SCREENFX_TEXTURERT ) )
	{
		// this is a screen effect particle
		ONCE
		{
			LPD3DCBASESHADERRESOURCEVIEW pScreenTex = dxC_ParticlesGetScreenTexture();
			if ( ! pScreenTex )
				break;
			V( dx9_EffectSetTexture( pEffect, tTechnique, EFFECT_PARAM_SCREENFX_TEXTURERT, pScreenTex ) );

			V( dxC_EffectSetPixelAccurateOffset( pEffect, tTechnique ) );
		}
	}

	if ( pDefinition->nTextureId != INVALID_ID )
	{
		D3D_TEXTURE * pTexture = dxC_TextureGet( pDefinition->nTextureId );
		if ( tDrawInfo.bPixelShader )
		{
			//HRVERIFY( pEffect->pD3DEffect->SetTexture( pEffect->phParams[ EFFECT_PARAM_DIFFUSEMAP0 ], pTexture->pD3DTexture ) );
			#ifdef ENGINE_TARGET_DX10
            if( dx9_EffectIsParamUsed( pEffect, tTechnique, /*EFFECT_PARAM_RAIN_TEXTUREARRAY*/EFFECT_PARAM_DIRLIGHTCOLOR ) )  //KMNV HACK: Due to our inability to find out if a shader is using a given texture
			{
				V( dx9_EffectSetTexture( pEffect, tTechnique, EFFECT_PARAM_RAIN_TEXTUREARRAY, pTexture->GetShaderResourceView() ) ); 
			}
            #endif
			{
				V( dx9_EffectSetTexture( pEffect, tTechnique, EFFECT_PARAM_DIFFUSEMAP0, pTexture->GetShaderResourceView() ) );
			}
		} else
		{
			V( dxC_SetTexture( 0, pTexture->GetShaderResourceView() ) );
		}
		dx9_ReportSetTexture( pTexture->nId );
	}

	// AE 2006.01.18 - Turned this off for 'filtereffects' to work.
	// set up fog -- the effect can blow it away
	//V( e_SetFogEnabled( e_GetRenderFlag( RENDER_FLAG_FOG_ENABLED ) ) );

//Texture is set globally now
#ifdef DX10_SOFT_PARTICLES
//    //NVTL : Soft billboards
//    //NVTL : setup depth things
//    //set the depth texture:   EFFECT_PARAM_SMOKE_TEXTURE_SCENE_DEPTH
	DEPTH_TARGET_INDEX dt = dxC_GetCurrentDepthTarget();
	if ( ( pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_DX10_DONT_USE_SOFT_PARTICLES ) == 0 )
	{
		V( dxC_SetRenderTargetWithDepthStencil(dxC_GetCurrentRenderTarget(), DEPTH_TARGET_NONE, 0) );
		RENDER_TARGET_INDEX eDwoPRT = dxC_GetSwapChainRenderTargetIndex( dxC_GetCurrentSwapChainIndex(), SWAP_RT_DEPTH_WO_PARTICLES );
		V( dx9_EffectSetTexture( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_TEXTURE_SCENE_DEPTH, dxC_RTShaderResourceGet( eDwoPRT ) ) );
	}
#endif
    //----

	for ( int i = 0; i < (int) nPassCount; i++ )
	{
		V( dxC_EffectBeginPass( pEffect, i ) );

		sSetParticleRenderStates( pDefinition, i, FALSE, bDepthOnly );

		if ( pIB && pIB->pD3DIndices )
		{
			V( dxC_DrawIndexedPrimitive( D3DPT_TRIANGLELIST, tDrawInfo.nFirstVertexOfBatch, 0, nVerticesToDraw, 0, tDrawInfo.nTrianglesInBatchCurr, pEffect, METRICS_GROUP_PARTICLE ) );
		} else {
			V( dxC_DrawPrimitive( D3DPT_TRIANGLELIST, tDrawInfo.nFirstVertexOfBatch, tDrawInfo.nTrianglesInBatchCurr, pEffect, METRICS_GROUP_PARTICLE ) );
		}
		sParticlesAddDrawn( tDrawInfo.nTrianglesInBatchCurr / TRIANGLES_PER_PARTICLE );

	}

	// AE 2006.01.18 - Turned this off for 'filtereffects' to work.
	//V( dxC_SetRenderState( D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_RED | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA ) );

#ifdef DX10_SOFT_PARTICLES
	//NVTL : Soft billboards
	if ( ( pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_DX10_DONT_USE_SOFT_PARTICLES ) == 0 )
	{
		dx9_EffectSetTexture( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_TEXTURE_SCENE_DEPTH, NULL );
		V( dxC_SetRenderTargetWithDepthStencil(dxC_GetCurrentRenderTarget(), dt, 0) );
	}
#endif

draw_basic_end:
	if ( tDrawInfo.nVerticesInBuffer == MAX_VERTICES_IN_BASIC_BUFFER || bClearBuffer )
	{
		tDrawInfo.nVerticesInBuffer = 0;
	} 

	tDrawInfo.nFirstVertexOfBatch = tDrawInfo.nVerticesInBuffer;
	tDrawInfo.nTrianglesInBatchCurr = 0;

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void e_ParticleDrawAddRope(
	PARTICLE_SYSTEM * pParticleSystem,
	PARTICLE_DRAW_INFO & tDrawInfo,
	BOOL bDepthOnly )
{
	int nKnotCount = pParticleSystem->tKnots.Count();

	int nKnotDrawType = KNOT_DRAW_TYPE_TWO_STRIPS;
	if ( pParticleSystem->pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_ROPE_DRAW_NORMAL_FACING )
		nKnotDrawType = KNOT_DRAW_TYPE_ONE_STRIP_FACE_NORMAL;
	else if ( pParticleSystem->pDefinition->dwFlags2 & PARTICLE_SYSTEM_DEFINITION_FLAG2_ROPE_DRAW_TUBE_4 )
		nKnotDrawType = KNOT_DRAW_TYPE_TUBE_4;
	else if ( pParticleSystem->pDefinition->dwFlags2 & PARTICLE_SYSTEM_DEFINITION_FLAG2_ROPE_DRAW_TUBE_6 )
		nKnotDrawType = KNOT_DRAW_TYPE_TUBE_6;
	else if ( pParticleSystem->pDefinition->dwFlags2 & PARTICLE_SYSTEM_DEFINITION_FLAG2_ROPE_DRAW_TUBE_8 )
		nKnotDrawType = KNOT_DRAW_TYPE_TUBE_8;
	else if ( pParticleSystem->pDefinition->dwFlags2 & PARTICLE_SYSTEM_DEFINITION_FLAG2_ROPE_DRAW_ONE_STRIP )
		nKnotDrawType = KNOT_DRAW_TYPE_ONE_STRIP_FACE_CAMERA;

	KNOT_DRAW_CHART * pKnotDrawChart = & sgtKnotDrawChart[ nKnotDrawType ];

	// see if there is enough room left in the vertex buffer before we start
	if ( tDrawInfo.nVerticesInBuffer + nKnotCount * pKnotDrawChart->nVerticesPerKnot >= MAX_VERTICES_IN_BASIC_BUFFER )
	{
		V( e_ParticlesDrawBasic( tDrawInfo, TRUE, bDepthOnly ) );
	}

	int nSegment = 0;
	float fTotalDistance = 0.0f;

	VECTOR vOffsetPrev1(0);
	VECTOR vOffsetPrev2(0);
	BOOL bOffsetPrevSet = FALSE;

	BOOL bRopeIsCircular = (pParticleSystem->pDefinition->dwFlags2 & PARTICLE_SYSTEM_DEFINITION_FLAG2_ROPE_USES_CIRCLE_PHYSICS) != 0;
	VECTOR vOffsetLast1;
	VECTOR vOffsetLast2;
	BOOL bOffsetLastSet = FALSE;

	for ( int nKnotCurr = pParticleSystem->tKnots.GetFirst(); 
		nKnotCurr != INVALID_ID; 
		nKnotCurr = pParticleSystem->tKnots.GetNextId( nKnotCurr ) )
	{
		if ( tDrawInfo.nVerticesInBuffer + pKnotDrawChart->nVerticesPerKnot > MAX_VERTICES_IN_BASIC_BUFFER )
			break;

		KNOT * pKnotCurr = pParticleSystem->tKnots.Get( nKnotCurr );

		VECTOR vOffsetCurr1;
		VECTOR vOffsetCurr2;
		int nKnotNext = pParticleSystem->tKnots.GetNextId( nKnotCurr );
		DWORD dwDiffuseMid;
		DWORD dwGlowMid;
		VECTOR vPositionMid;
		float fKnotDistance = 0.0f;

		VECTOR vEyeDirection = tDrawInfo.vEyePosition - pKnotCurr->vPosition;
		VectorNormalize( vEyeDirection );

		BOOL bUsesOffsetCurr2 = FALSE;
		KNOT * pKnotNext = nKnotNext != INVALID_ID ? pParticleSystem->tKnots.Get( nKnotNext ) : NULL;
		if ( pKnotNext )
		{
			VECTOR vKnotDelta = pKnotNext->vPosition - pKnotCurr->vPosition;
			fKnotDistance = VectorLength( vKnotDelta );
			VectorNormalize( vKnotDelta );

			switch ( nKnotDrawType )
			{
			case KNOT_DRAW_TYPE_ONE_STRIP_FACE_CAMERA:
				VectorCross( vOffsetCurr1, vEyeDirection, vKnotDelta );
				VectorNormalize( vOffsetCurr1 );
				break;
			case KNOT_DRAW_TYPE_ONE_STRIP_FACE_NORMAL:
				vOffsetCurr1 = pKnotCurr->vNormal;
				break;
			default:
				{
					float fEyeDot = fabsf( VectorDot( vEyeDirection, vKnotDelta ) );
#define	EYE_DOT_LIMIT 0.7f
					if ( bOffsetPrevSet && fEyeDot > EYE_DOT_LIMIT )
					{
						VECTOR vUsingEye;
						VectorCross( vUsingEye, vEyeDirection, vKnotDelta );
						VectorNormalize( vUsingEye );
						VectorCross( vOffsetCurr1, vOffsetPrev2, vKnotDelta );
						VectorNormalize( vOffsetCurr1 );
						if ( VectorDot( vOffsetCurr1, vUsingEye ) < 0 )
							vUsingEye = - vUsingEye;
						vOffsetCurr1 = VectorLerp( vOffsetCurr1, vUsingEye, (fEyeDot - EYE_DOT_LIMIT) / (1.0f - EYE_DOT_LIMIT) );
						VectorNormalize( vOffsetCurr1 );
					} else {
						VectorCross( vOffsetCurr1, vEyeDirection, vKnotDelta );
						VectorNormalize( vOffsetCurr1 );
					}
					VectorCross( vOffsetCurr2, vOffsetCurr1, vKnotDelta );
					VectorNormalize( vOffsetCurr2 );
					bUsesOffsetCurr2 = TRUE;
				}break;
			}

			BYTE bBlue  = (BYTE) ((((pKnotCurr->dwDiffuse >>  0) & 0x000000ff) + ((pKnotNext->dwDiffuse >>  0) & 0x000000ff)) / 2);
			BYTE bGreen = (BYTE) ((((pKnotCurr->dwDiffuse >>  8) & 0x000000ff) + ((pKnotNext->dwDiffuse >>  8) & 0x000000ff)) / 2);
			BYTE bRed   = (BYTE) ((((pKnotCurr->dwDiffuse >> 16) & 0x000000ff) + ((pKnotNext->dwDiffuse >> 16) & 0x000000ff)) / 2);
			BYTE bAlpha = (BYTE) (((pKnotCurr->dwDiffuse >> 24) + (pKnotNext->dwDiffuse >> 24)) / 2);
			dwDiffuseMid = D3DCOLOR_RGBA( bRed, bGreen, bBlue, bAlpha );
			BYTE bGlow  = (BYTE) (((pKnotCurr->dwGlow >> 24) + (pKnotNext->dwGlow >> 24)) / 2);
			dwGlowMid	= D3DCOLOR_RGBA( 0, 0, 0, bGlow );
			vPositionMid = (pKnotCurr->vPosition + pKnotNext->vPosition) / 2.0f;
		} else {
			if ( nKnotCurr == pParticleSystem->tKnots.GetFirst() )
				break;
			vOffsetCurr1 = vOffsetLast1;
			vOffsetCurr2 = vOffsetLast2;
			dwDiffuseMid = pKnotCurr->dwDiffuse;
			dwGlowMid	 = pKnotCurr->dwGlow;
			vPositionMid = pKnotCurr->vPosition;
		}

		// make sure that it doesn't twist
		if ( bOffsetPrevSet && VectorDot( vOffsetCurr1, vOffsetPrev1 ) < 0 )
		{
			vOffsetCurr1 = -vOffsetCurr1;
		}
		vOffsetPrev1 = vOffsetCurr1;
		if ( bUsesOffsetCurr2 )
		{
			if ( bOffsetPrevSet && VectorDot( vOffsetCurr2, vOffsetPrev2 ) < 0 )
			{
				vOffsetCurr2 = -vOffsetCurr2;
			}
			vOffsetPrev2 = vOffsetCurr2;
		}
		bOffsetPrevSet = TRUE;

		// adjust knot scale and save offsets so that we can use them to avoid twisting on the next loop
		if ( pKnotNext )
		{
			vOffsetCurr1 *= pKnotNext->fScale / 2.0f;
			if ( bUsesOffsetCurr2 )
			{
				vOffsetCurr2 *= pKnotNext->fScale / 2.0f;
			}
			if ( !bRopeIsCircular || !bOffsetLastSet )
			{
				vOffsetLast1 = vOffsetCurr1;
				if ( bUsesOffsetCurr2 )
					vOffsetLast2 = vOffsetCurr2;
				bOffsetLastSet = TRUE;
			}
		}

		int nVertexFirst = tDrawInfo.nVerticesInBuffer;

#ifdef INTEL_CHANGES
		// select the appropriate vertex buffer based on nParticleSystemAsyncIndex, then point at the proper section
		// indentified by nVerticesMultiplier
		PARTICLE_QUAD_VERTEX * pVertices = (
			sGetVerticesBufferForIndex( tDrawInfo.nParticleSystemAsyncIndex, tDrawInfo.pParticleSystemCurr ) +
			( tDrawInfo.nVerticesMultiplier * sgnParticleSystemVertexBufferSizes[ PARTICLE_VERTEX_BUFFER_BASIC ] )
		);
#else
		PARTICLE_QUAD_VERTEX * pVertices = & sgpBasicVertices[ 0 ];
#endif

		// color
		for ( int i = 0; i < pKnotDrawChart->nMiddleVerticesStart; i++ )
		{
			pVertices[ nVertexFirst + i ].dwDiffuse = pKnotCurr->dwDiffuse;
			pVertices[ nVertexFirst + i ].dwGlow	= pKnotCurr->dwGlow;
		}
		for ( int i = pKnotDrawChart->nMiddleVerticesStart; 
			i < pKnotDrawChart->nVerticesPerKnot; i++ )
		{
			pVertices[ nVertexFirst + i ].dwDiffuse = dwDiffuseMid;
			pVertices[ nVertexFirst + i ].dwGlow	= dwGlowMid;
		}

		// position
		switch ( nKnotDrawType )
		{
		case KNOT_DRAW_TYPE_TWO_STRIPS:
			pVertices[ nVertexFirst + 0 ].vPosition = (D3DXVECTOR3 &) ( pKnotCurr->vPosition + vOffsetCurr1 );
			pVertices[ nVertexFirst + 1 ].vPosition = (D3DXVECTOR3 &) ( pKnotCurr->vPosition - vOffsetCurr1 );
			pVertices[ nVertexFirst + 2 ].vPosition = (D3DXVECTOR3 &) ( pKnotCurr->vPosition + vOffsetCurr2 );
			pVertices[ nVertexFirst + 3 ].vPosition = (D3DXVECTOR3 &) ( pKnotCurr->vPosition - vOffsetCurr2 );
			pVertices[ nVertexFirst + 4 ].vPosition = (D3DXVECTOR3 &) vPositionMid;
			break;
		case KNOT_DRAW_TYPE_ONE_STRIP_FACE_NORMAL:
		case KNOT_DRAW_TYPE_ONE_STRIP_FACE_CAMERA:
			pVertices[ nVertexFirst + 0 ].vPosition = (D3DXVECTOR3 &) ( pKnotCurr->vPosition + vOffsetCurr1 );
			pVertices[ nVertexFirst + 1 ].vPosition = (D3DXVECTOR3 &) ( pKnotCurr->vPosition - vOffsetCurr1 );
			pVertices[ nVertexFirst + 2 ].vPosition = (D3DXVECTOR3 &) vPositionMid;
			break;
		case KNOT_DRAW_TYPE_TUBE_4:
			{
				pVertices[ nVertexFirst + 0 ].vPosition = (D3DXVECTOR3 &) ( pKnotCurr->vPosition + vOffsetCurr1 );
				pVertices[ nVertexFirst + 1 ].vPosition = (D3DXVECTOR3 &) ( pKnotCurr->vPosition + vOffsetCurr2 );
				pVertices[ nVertexFirst + 2 ].vPosition = (D3DXVECTOR3 &) ( pKnotCurr->vPosition - vOffsetCurr1 );
				pVertices[ nVertexFirst + 3 ].vPosition = (D3DXVECTOR3 &) ( pKnotCurr->vPosition - vOffsetCurr2 );
				pVertices[ nVertexFirst + 4 ].vPosition = pVertices[ nVertexFirst + 0 ].vPosition;
			}
			break;
		case KNOT_DRAW_TYPE_TUBE_6:
			{
				pVertices[ nVertexFirst + 0 ].vPosition = (D3DXVECTOR3 &) ( pKnotCurr->vPosition + vOffsetCurr1 );
				pVertices[ nVertexFirst + 3 ].vPosition = (D3DXVECTOR3 &) ( pKnotCurr->vPosition - vOffsetCurr1 );

				VECTOR vThirdOffset = VectorLerp( vOffsetCurr1, vOffsetCurr2, 0.333333f );
				float fThirdAdjust = pKnotCurr->fScale / VectorLength( vThirdOffset );
				fThirdAdjust /= 2.0f;  // we use scale divided by 2
				vThirdOffset *= fThirdAdjust;
				pVertices[ nVertexFirst + 1 ].vPosition = (D3DXVECTOR3 &) ( pKnotCurr->vPosition + vThirdOffset );
				pVertices[ nVertexFirst + 4 ].vPosition = (D3DXVECTOR3 &) ( pKnotCurr->vPosition - vThirdOffset );

				vThirdOffset = VectorLerp( -vOffsetCurr1, vOffsetCurr2, 0.3333333f );
				vThirdOffset *= fThirdAdjust;
				pVertices[ nVertexFirst + 2 ].vPosition = (D3DXVECTOR3 &) ( pKnotCurr->vPosition + vThirdOffset );
				pVertices[ nVertexFirst + 5 ].vPosition = (D3DXVECTOR3 &) ( pKnotCurr->vPosition - vThirdOffset );
				pVertices[ nVertexFirst + 6 ].vPosition = pVertices[ nVertexFirst + 0 ].vPosition;
			}
			break;
		case KNOT_DRAW_TYPE_TUBE_8:
			{
				pVertices[ nVertexFirst + 0 ].vPosition = (D3DXVECTOR3 &) ( pKnotCurr->vPosition + vOffsetCurr1 );
				pVertices[ nVertexFirst + 2 ].vPosition = (D3DXVECTOR3 &) ( pKnotCurr->vPosition + vOffsetCurr2 );
				pVertices[ nVertexFirst + 4 ].vPosition = (D3DXVECTOR3 &) ( pKnotCurr->vPosition - vOffsetCurr1 );
				pVertices[ nVertexFirst + 6 ].vPosition = (D3DXVECTOR3 &) ( pKnotCurr->vPosition - vOffsetCurr2 );

				VECTOR vMidOffset;
				vMidOffset = ( vOffsetCurr1 + vOffsetCurr2) / 2.0f;	
				float fMidAdjust = pKnotCurr->fScale / VectorLength( vMidOffset );
				fMidAdjust /= 2.0f; // we use scale divided by 2
				vMidOffset *= fMidAdjust;
				pVertices[ nVertexFirst + 1 ].vPosition = (D3DXVECTOR3 &) ( pKnotCurr->vPosition + vMidOffset );
				pVertices[ nVertexFirst + 5 ].vPosition = (D3DXVECTOR3 &) ( pKnotCurr->vPosition - vMidOffset );

				vMidOffset = (-vOffsetCurr1 + vOffsetCurr2) / 2.0f;	
				vMidOffset *= fMidAdjust;
				pVertices[ nVertexFirst + 3 ].vPosition = (D3DXVECTOR3 &) ( pKnotCurr->vPosition + vMidOffset );
				pVertices[ nVertexFirst + 7 ].vPosition = (D3DXVECTOR3 &) ( pKnotCurr->vPosition - vMidOffset );
				pVertices[ nVertexFirst + 8 ].vPosition = pVertices[ nVertexFirst + 0 ].vPosition;
			}
			break;
		}
		switch ( nKnotDrawType )
		{// add the middle vertices
		case KNOT_DRAW_TYPE_TUBE_4:
		case KNOT_DRAW_TYPE_TUBE_6:
		case KNOT_DRAW_TYPE_TUBE_8:
			VECTOR vOffsetToMiddle = vPositionMid - pKnotCurr->vPosition;
			int nCurr = 0;
			int nNext = 1;
			for ( int i = pKnotDrawChart->nMiddleVerticesStart;
				i < pKnotDrawChart->nVerticesPerKnot; i++ )
			{
				VECTOR vAverage = ( pVertices[ nVertexFirst + nCurr ].vPosition + pVertices[ nVertexFirst + nNext ].vPosition ) / 2.0f;

				pVertices[ nVertexFirst + i ].vPosition = (D3DXVECTOR3 &) ( vAverage + vOffsetToMiddle );

				nCurr++;
				if ( i == pKnotDrawChart->nVerticesPerKnot - 2 )
					nNext = 0;
				else
					nNext++;
			}
			break;
		}

		// Set texture coords
		float fTop		= pParticleSystem->fTextureSlideY;
		float fBottom	= pParticleSystem->fTextureSlideY + 1.0f;
		float fLeft;
		float fMid;
		if ( pParticleSystem->fRopeMetersPerTexture != 0.0f )
		{
			fLeft = fTotalDistance / pParticleSystem->fRopeMetersPerTexture + pParticleSystem->fTextureSlideX;
			fMid  = (fTotalDistance + pKnotCurr->fSegmentLength / 2.0f) / pParticleSystem->fRopeMetersPerTexture;
			fMid += pParticleSystem->fTextureSlideX;
		} else {
			fLeft = (float) nSegment - pParticleSystem->fTextureSlideX;
			fMid  = fLeft + 0.5f;
		}
		if ( pParticleSystem->pDefinition->dwFlags & PARTICLE_SYSTEM_DEFINITION_FLAG_STRETCH_TEXTURE_DOWN_ROPE )
		{
			fLeft = (float) ( nSegment ) / ( nKnotCount - 1 );
			float fNext = (float) ( nSegment + 1 ) / ( nKnotCount - 1 );
			fMid = (fLeft + fNext) / 2.0f;
			fLeft -= pParticleSystem->fTextureSlideX;
			fMid  -= pParticleSystem->fTextureSlideX;
		}
	
		if ( !(pParticleSystem->pDefinition->dwFlags2 & PARTICLE_SYSTEM_DEFINITION_FLAG2_WRAP_TEXTURE_AROUND_ROPE) ||
			pKnotDrawChart->nFaces <= 1 )
		{// this happens to work for all of the draw types
			int i = 0;
			for ( ; i < pKnotDrawChart->nMiddleVerticesStart; i += 2 )
			{
				pVertices[ nVertexFirst + i ].pfTextureCoord0[ 0 ] = fTop;
				pVertices[ nVertexFirst + i ].pfTextureCoord0[ 1 ] = fLeft;

				pVertices[ nVertexFirst + i + 1 ].pfTextureCoord0[ 0 ] = fBottom;
				pVertices[ nVertexFirst + i + 1 ].pfTextureCoord0[ 1 ] = fLeft;
			}

			for ( ; i < pKnotDrawChart->nVerticesPerKnot; i++ )
			{
				pVertices[ nVertexFirst + i ].pfTextureCoord0[ 0 ] = ( fTop + fBottom ) / 2.0f;
				pVertices[ nVertexFirst + i ].pfTextureCoord0[ 1 ] = fMid;
			}
		} else { 
			int i = 0;
			float fYCurrent = 0;
			float fYDelta = 1.0f / pKnotDrawChart->fFaces;
			for ( ; i < pKnotDrawChart->nMiddleVerticesStart; i ++ )
			{
				pVertices[ nVertexFirst + i ].pfTextureCoord0[ 0 ] = fTop + fYCurrent;
				pVertices[ nVertexFirst + i ].pfTextureCoord0[ 1 ] = fLeft;
				fYCurrent += fYDelta;
			}

			fYCurrent = fYDelta / 2.0f;
			for ( ; i < pKnotDrawChart->nVerticesPerKnot; i++ )
			{
				pVertices[ nVertexFirst + i ].pfTextureCoord0[ 0 ] = fTop + fYCurrent;
				pVertices[ nVertexFirst + i ].pfTextureCoord0[ 1 ] = fMid;
				fYCurrent += fYDelta;
			}		
		}

		tDrawInfo.nVerticesInBuffer += pKnotDrawChart->nVerticesPerKnot;
		nSegment++;
		sParticlesAddDrawn();
		e_ParticlesAddTotalWorldArea( pKnotCurr->fScale * fKnotDistance );
		fTotalDistance += pKnotCurr->fSegmentLength;
	}

	// draw the rope - because of how we use the index buffer and have arbitrary lengths... 
	// It seems like we have to be one rope per batch
	if ( tDrawInfo.nVerticesInBuffer && nSegment > 1 )
	{
		tDrawInfo.nIndexBufferCurr = pKnotDrawChart->tIndexBufferDef.nD3DBufferID;
		tDrawInfo.nTrianglesInBatchCurr = pKnotDrawChart->nTrianglesPerKnot * (nSegment - 1);
		V( e_ParticlesDrawBasic( tDrawInfo, FALSE, bDepthOnly ) );
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

#include "optimize_on.h"	// CHB 2006.08.21

void e_ParticleDrawAddQuad(
	const PARTICLE_SYSTEM * const NOALIAS pParticleSystem,
	const PARTICLE * const NOALIAS pParticle,
	PARTICLE_DRAW_INFO & tDrawInfo,
	VECTOR & vOffset,
	VECTOR & vPerpOffset,
	VECTOR & vPositionShift,
	BOOL bDepthOnly )
{
#ifdef INTEL_CHANGES
	if ( ( pParticleSystem->pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_ASYNC_SIMULATION ) != 0 )
	{
		// just cut off particles beyond a certain point, since we can't draw from this thread
		if ( tDrawInfo.nVerticesInBuffer + VERTICES_PER_PARTICLE >= PARTICLE_SYSTEM_ASYNC_PARTICLES_MAX )
		{
			return;
		}
	}
	else if ( tDrawInfo.nVerticesInBuffer + VERTICES_PER_PARTICLE >= MAX_VERTICES_IN_BASIC_BUFFER )
	{
		V( e_ParticlesDrawBasic( tDrawInfo, TRUE, bDepthOnly ) );
	}
#else
	if ( tDrawInfo.nVerticesInBuffer + VERTICES_PER_PARTICLE >= MAX_VERTICES_IN_BASIC_BUFFER )
	{
		V( e_ParticlesDrawBasic( tDrawInfo, TRUE, bDepthOnly ) );
	}
#endif

	if (pParticleSystem->pDefinition->pnTextureFrameCount[ 0 ] <= 0)
	{
		return;
	}

	tDrawInfo.nIndexBufferCurr = INVALID_ID;
	tDrawInfo.nTrianglesInBatchCurr += TRIANGLES_PER_PARTICLE;

	float fScale = pParticle->fScale;
	if ( pParticle->fStretchDiamond != 0.0f )
	{
		float fStretch = pParticle->fStretchDiamond + 1.0f;
		vOffset *= fScale * fStretch;
		vPerpOffset *= fScale * (2.0f - fStretch);
	} else {
		vOffset *= fScale;
		vPerpOffset *= fScale;
		vPositionShift *= fScale;
	}

#ifdef INTEL_CHANGES
	// select the appropriate vertex buffer based on nParticleSystemAsyncIndex, then point at the proper section
	// indentified by nVerticesMultiplier
	PARTICLE_QUAD_VERTEX * pVerticesStart = (
			sGetVerticesBufferForIndex( tDrawInfo.nParticleSystemAsyncIndex, tDrawInfo.pParticleSystemCurr ) +
			( tDrawInfo.nVerticesMultiplier * sgnParticleSystemVertexBufferSizes[ PARTICLE_VERTEX_BUFFER_BASIC ] )
	);
	PARTICLE_QUAD_VERTEX * pVertices = pVerticesStart + tDrawInfo.nVerticesInBuffer;
#else
	PARTICLE_QUAD_VERTEX * pVertices = & sgpBasicVertices[ tDrawInfo.nVerticesInBuffer ];
#endif
	// color
	for ( int nVertex = 0; nVertex < VERTICES_PER_PARTICLE; nVertex++ )
	{
		pVertices[ nVertex ].dwDiffuse = pParticle->dwDiffuse;
		pVertices[ nVertex ].dwGlow	  = pParticle->dwGlowColor;
	}

	// calculate position for this vector
	pVertices[ 0 ].vPosition = pParticle->vPosition + vOffset;
	pVertices[ 1 ].vPosition = pParticle->vPosition - vOffset;
	pVertices[ 2 ].vPosition = pParticle->vPosition - vPerpOffset;

	pVertices[ 3 ].vPosition = pVertices[ 1 ].vPosition;
	pVertices[ 4 ].vPosition = pVertices[ 0 ].vPosition;
	pVertices[ 5 ].vPosition = pParticle->vPosition + vPerpOffset;

	for ( int nVertex = 0; nVertex < VERTICES_PER_PARTICLE; nVertex++ )
	{
		//pVertex->vPosition = pVertex->vPosition + vPositionShift;
		pVertices[nVertex].vPosition.x += vPositionShift.x;
		pVertices[nVertex].vPosition.y += vPositionShift.y;
		pVertices[nVertex].vPosition.z += vPositionShift.z;

		// set random percent
		pVertices[nVertex].fRand = pParticle->fRandPercent;
	}

	// Set texture coords
	int nOffsetU = (int)pParticle->fFrame / pParticleSystem->pDefinition->pnTextureFrameCount[ 0 ];
	int nOffsetV = (int)pParticle->fFrame % pParticleSystem->pDefinition->pnTextureFrameCount[ 0 ];
	float fFrameSizeU = pParticleSystem->pDefinition->pfTextureFrameSize[ 0 ];
	float fFrameSizeV = pParticleSystem->pDefinition->pfTextureFrameSize[ 1 ];

	float fTop		= fFrameSizeU * nOffsetU;
	float fBottom	= fFrameSizeU * (nOffsetU + 1);
	float fLeft		= fFrameSizeV * nOffsetV;
	float fRight	= fFrameSizeV * (nOffsetV + 1);
	if ( pParticle->dwFlags & PARTICLE_FLAG_MIRROR_TEXTURE )
	{
		float fTemp = fLeft;
		fLeft = fRight;
		fRight = fTemp;
	}
	pVertices[ 0 ].pfTextureCoord0[ 0 ] = fLeft;
	pVertices[ 0 ].pfTextureCoord0[ 1 ] = fTop;

	pVertices[ 1 ].pfTextureCoord0[ 0 ] = fRight;
	pVertices[ 1 ].pfTextureCoord0[ 1 ] = fBottom;

	pVertices[ 2 ].pfTextureCoord0[ 0 ] = fRight;
	pVertices[ 2 ].pfTextureCoord0[ 1 ] = fTop;

	pVertices[ 3 ].pfTextureCoord0[ 0 ] = fRight;
	pVertices[ 3 ].pfTextureCoord0[ 1 ] = fBottom;

	pVertices[ 4 ].pfTextureCoord0[ 0 ] = fLeft;
	pVertices[ 4 ].pfTextureCoord0[ 1 ] = fTop;

	pVertices[ 5 ].pfTextureCoord0[ 0 ] = fLeft;
	pVertices[ 5 ].pfTextureCoord0[ 1 ] = fBottom;

	tDrawInfo.nVerticesInBuffer += 6;
#ifdef INTEL_CHANGES
	// INTEL_NOTE: these metrics aren't safe to capture from multiple threads.  Important?
	if ( tDrawInfo.nParticleSystemAsyncIndex == 0 )
	{
		e_ParticlesAddTotalWorldArea( pParticle->fScale * pParticle->fScale );
	}
#else
	e_ParticlesAddTotalWorldArea( pParticle->fScale * pParticle->fScale );
#endif
}

#include "optimize_restore.h"	// CHB 2006.08.21

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static void sDecompressColorToVector ( 
	float pfOutVector[ 4 ], 
	DWORD dwColor )
{
	// convert from a color
	float fInv255 = 1.0f / 255.0f;
	pfOutVector[ 0 ] = (FLOAT)( ( dwColor >> 16 ) & 0x000000ff ) * fInv255;
	pfOutVector[ 1 ] = (FLOAT)( ( dwColor >>  8 ) & 0x000000ff ) * fInv255;
	pfOutVector[ 2 ] = (FLOAT)( ( dwColor >>  0 ) & 0x000000ff ) * fInv255;
	pfOutVector[ 3 ] = (FLOAT)( ( dwColor >> 24 ) & 0x000000ff ) * fInv255;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_ParticleDrawAddMesh(
	PARTICLE_SYSTEM * pParticleSystem,
	PARTICLE * pParticle,
	PARTICLE_DRAW_INFO & tDrawInfo,
	VECTOR & vOffset,
	VECTOR & vPerpOffset,
	VECTOR & vPositionShift,
	BOOL bDepthOnly )
{
	sParticlesAddDrawn();
	int nMaxMeshes = min( tDrawInfo.pParticleSystemCurr->pDefinition->nMeshesPerBatchMax, MAX_PARTICLE_MESHES_PER_BATCH );
	if ( tDrawInfo.nMeshesInBatchCurr >= nMaxMeshes )
	{
		V( e_ParticlesDrawMeshes( tDrawInfo, bDepthOnly ) );
	}

	VECTOR vUp;
	VectorCross( vUp, vPerpOffset, vOffset );
	VectorNormalize( vUp );
	VectorNormalize( vPerpOffset );
	if ( VectorIsZero( vUp ) || VectorIsZero( vPerpOffset ) )
	{// stopgap for bad input vectors
		vUp = VECTOR( 0, 0, 1 );
		vPerpOffset = VECTOR( 0, 1, 0 );
	}

	MATRIX mMatrix;
	MatrixFromVectors( mMatrix, pParticle->vPosition + vPositionShift, vUp, vPerpOffset );

	VECTOR vScale( pParticle->fScale );

	VECTOR vBoxSize = BoundingBoxGetSize( &pParticleSystem->pDefinition->tMultiMeshCompressionBox );
	float fBoxScale = MAX( vBoxSize.fX, MAX( vBoxSize.fY, vBoxSize.fZ ));
	// the position is offset and scaled so that it can be compressed to 3 bytes - this matrix decompresses it
	vScale *= fBoxScale;

	VECTOR vBoxOffset = pParticleSystem->pDefinition->tMultiMeshCompressionBox.vMin;
	vBoxOffset *= pParticle->fScale;

	MATRIX mScale;
	MatrixTranslationScale ( &mScale, &vBoxOffset, &vScale );

	MatrixMultiply( &mMatrix, &mScale, &mMatrix );

	//MatrixMultiply( &mMatrix, &mMatrix, tDrawInfo.bUseProj2 ? &tDrawInfo.matViewProjection2 : &tDrawInfo.matViewProjection );
	MatrixTranspose( &mMatrix, &mMatrix );

	memcpy( &sgpfMeshMatrices[ tDrawInfo.nMeshesInBatchCurr * 12 ],
			(float *) &mMatrix, sizeof( float ) * 12 );

	sDecompressColorToVector ( &sgpfMeshColors[ tDrawInfo.nMeshesInBatchCurr * 4 ], pParticle->dwDiffuse );

	sgpfMeshGlows[ tDrawInfo.nMeshesInBatchCurr ] = (float) (pParticle->dwGlowColor >> 24) / 255.0f;

	tDrawInfo.nMeshesInBatchCurr++;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_ParticleDrawUpdateStats()
{
	int nWidth, nHeight;
	e_GetWindowSize( nWidth, nHeight );

	for ( int i = 0; i < NUM_PARTICLE_DRAW_SECTIONS; i++ )
	{
		sgtParticleDrawStats[ i ].fScreenAreaMeasured = 0.f;
		sgtParticleDrawStats[ i ].fWorldArea = 0.f;
		//sgtParticleDrawStats[ i ].nDrawn = 0; // this has been moved so that it works
		V( dx9_CheckOcclusionQuery( &( sgtParticleDrawStats[ i ].tOcclusion ) ) );
 		sgtParticleDrawStats[ i ].fScreenAreaMeasured = (float)sgtParticleDrawStats[ i ].tOcclusion.dwPixelsDrawn / (float)( nWidth * nHeight );
		sgtParticleDrawStats[ i ].nTimesBegun = 0;
		sgtParticleDrawStats[ i ].fScreenAreaPredicted = 0.0f;
	}

	sgbOcclusionQueryStarted = FALSE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_ParticleDrawEnterSection( PARTICLE_DRAW_SECTION eSection )
{
	ASSERT( sgeParticleCurrentDrawSection == PARTICLE_DRAW_SECTION_INVALID );
	ASSERT_RETURN( IS_VALID_INDEX( eSection, NUM_PARTICLE_DRAW_SECTIONS ) );

	sgeParticleCurrentDrawSection = eSection;
	sgtParticleDrawStats[ eSection ].nTimesBegun++;
	sgtParticleDrawStats[ eSection ].nDrawn = 0;

	ASSERT_RETURN( ! sgbOcclusionQueryStarted );

	PRESULT pr;
	V_SAVE( pr, dx9_BeginOcclusionQuery( &sgtParticleDrawStats[ sgeParticleCurrentDrawSection ].tOcclusion ) );

	if ( SUCCEEDED(pr) )
	{
		sgbOcclusionQueryStarted = !!( S_OK == pr );
	}

	gnLastBeginOcclusionModelID = -2;
	gnLastEndOcclusionModelID   = -2;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_ParticleDrawLeaveSection()
{
	ASSERT_RETURN( IS_VALID_INDEX( sgeParticleCurrentDrawSection, NUM_PARTICLE_DRAW_SECTIONS ) );

	if ( sgbOcclusionQueryStarted )
	{
		V( dx9_EndOcclusionQuery( &(sgtParticleDrawStats[ sgeParticleCurrentDrawSection ].tOcclusion) ) );
		gnLastEndOcclusionModelID = -2;
		sgbOcclusionQueryStarted = FALSE;
	}
	else
	{
		sgtParticleDrawStats[ sgeParticleCurrentDrawSection ].tOcclusion.dwPixelsDrawn = 0;
	}

	sgeParticleCurrentDrawSection = PARTICLE_DRAW_SECTION_INVALID;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_ParticleDrawInitFrame(
	PARTICLE_DRAW_SECTION eSection,
	PARTICLE_DRAW_INFO & tDrawInfo )
{
	e_ParticleDrawEnterSection( eSection );

	//for ( int i = 0; i < NUM_PARTICLE_EFFECTS; i++ )
	//{
	//	if ( sgnEffects[ i ] == INVALID_ID )
	//		continue;
	//	sgpEffects[ i ] = dx9_EffectGet( sgnEffects[ i ] );
	//	ASSERT ( sgpEffects[ i ] );
	//	if ( ! dxC_EffectIsLoaded( sgpEffects[ i ] ) )
	//		continue;

	//	D3D_EFFECT * pEffect = sgpEffects[ i ];
	//	EFFECT_TECHNIQUE * pTechnique = dx9_EffectGetTechniquePtrParticle( pEffect, 0 );
	//	if ( ! pTechnique )
	//		continue;

	//	dx9_EffectSetFogParameters( pEffect, *pTechnique );
	//}

#ifdef HAVOKFX_ENABLED
	if ( eSection == PARTICLE_DRAW_SECTION_NORMAL && e_HavokFXIsEnabled() )
	{
		dxC_HavokFXGetRigidAllTransforms( (LPD3DCVB*)tDrawInfo.ppRigidBodyTransforms, (LPD3DCVB*)&tDrawInfo.pParticlePositions, NULL);
		tDrawInfo.pParticleVelocities = NULL;
	}
#endif
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void e_ParticleDrawEndFrame()
{
	e_ParticleDrawLeaveSection();

	V( dxC_SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW ) );
	V( dxC_SetRenderState( D3DRS_ZWRITEENABLE, TRUE ) );
	V( dxC_SetRenderState(D3DRS_ALPHAREF,			(DWORD)1) );
	V( e_SetFogEnabled( e_GetRenderFlag( RENDER_FLAG_FOG_ENABLED ) ) );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
#ifdef HAVOKFX_ENABLED
static PRESULT sCreateMeshForHavokFX( 
	D3D_MODEL_DEFINITION * pModelDefinition, 
	PARTICLE_MODEL_FILE * pModelFile,
	INDEX_BUFFER_TYPE * pwFileIndexBuffer, 
	BYTE * pbFileStart )
{
	D3D_MESH_DEFINITION tMeshDef;
	ZeroMemory( &tMeshDef, sizeof( D3D_MESH_DEFINITION ) );
	tMeshDef.nBoneToAttachTo = INVALID_ID;

	tMeshDef.wTriangleCount = pModelFile->wTriangleCount;
	ASSERT_RETFAIL( tMeshDef.wTriangleCount );
	tMeshDef.tIndexBufferDef.nIndexCount = 3 * tMeshDef.wTriangleCount;
	tMeshDef.tIndexBufferDef.nBufferSize = sizeof( INDEX_BUFFER_TYPE ) * tMeshDef.tIndexBufferDef.nIndexCount;
	tMeshDef.tIndexBufferDef.pLocalData = (void *) MALLOC( NULL, tMeshDef.tIndexBufferDef.nBufferSize );
	ASSERT_RETFAIL( tMeshDef.tIndexBufferDef.pLocalData );
	memcpy( tMeshDef.tIndexBufferDef.pLocalData, pwFileIndexBuffer, tMeshDef.tIndexBufferDef.nBufferSize );
	memset( tMeshDef.pnTextures, -1, sizeof( int ) * NUM_TEXTURE_TYPES );

	D3D_VERTEX_BUFFER_DEFINITION * pVertexBufferDef = dxC_ModelDefinitionAddVertexBufferDef( pModelDefinition, &tMeshDef.nVertexBufferIndex );
	tMeshDef.ePrimitiveType = D3DPT_TRIANGLELIST;

	dx9_ModelDefinitionAddMesh( pModelDefinition, &tMeshDef );

	pVertexBufferDef->dwFVF = D3DC_FVF_NULL;
	pVertexBufferDef->eVertexType = VERTEX_DECL_PARTICLE_MESH_HAVOKFX;
	pVertexBufferDef->nVertexCount = pModelFile->nVertexCount;
	pVertexBufferDef->nBufferSize[ 0 ] = pVertexBufferDef->nVertexCount * dxC_GetVertexSize( 0, pVertexBufferDef );
	GRANNY_PARTICLE_MESH_VERTEX * pGrannyVerts = NULL;
	ASSERT_RETFAIL( pVertexBufferDef->nBufferSize[ 0 ] );
	pVertexBufferDef->pLocalData[ 0 ] = (void *) MALLOC( NULL, pVertexBufferDef->nBufferSize[ 0 ] );
	ASSERT_RETFAIL( pVertexBufferDef->pLocalData[ 0 ] );
	memcpy( pVertexBufferDef->pLocalData[ 0 ], pbFileStart + pModelFile->pnVertexBufferStart[ PARTICLE_MODEL_VERTEX_TYPE_NORMALS_AND_ANIMS ], pVertexBufferDef->nBufferSize[ 0 ] );

	return S_OK;
}
#endif

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static PRESULT sCreateMeshForConstantBufferDuplication( 
	PARTICLE_SYSTEM_DEFINITION * pDefinition,
	D3D_MODEL_DEFINITION * pModelDefinition, 
	PARTICLE_MODEL_FILE * pModelFile,
	INDEX_BUFFER_TYPE * pwFileIndexBuffer, 
	BYTE * pbFileStart )
{
	// this needs a better method - we should adjust to the card's constant register count
	{
		if ( ! pDefinition->nMeshesPerBatchMax )
			pDefinition->nMeshesPerBatchMax = MAX_PARTICLE_MESHES_PER_BATCH;
		else
			pDefinition->nMeshesPerBatchMax = min( MAX_PARTICLE_MESHES_PER_BATCH, pDefinition->nMeshesPerBatchMax );

		int nMaxMeshesForIndexBuffer = (1 << ( 8 * sizeof( INDEX_BUFFER_TYPE ) )) / pModelFile->nVertexCount;
		pDefinition->nMeshesPerBatchMax = min( pDefinition->nMeshesPerBatchMax, nMaxMeshesForIndexBuffer );
	}

	D3D_MESH_DEFINITION tMeshDef;
	ZeroMemory( &tMeshDef, sizeof( D3D_MESH_DEFINITION ) );
	tMeshDef.nBoneToAttachTo = INVALID_ID;

	tMeshDef.wTriangleCount = pModelFile->wTriangleCount;
	ASSERT_RETFAIL( tMeshDef.wTriangleCount );
	dxC_IndexBufferDefinitionInit( tMeshDef.tIndexBufferDef );
	tMeshDef.tIndexBufferDef.nIndexCount = pDefinition->nMeshesPerBatchMax * 3 * tMeshDef.wTriangleCount;
	tMeshDef.tIndexBufferDef.nBufferSize = sizeof( INDEX_BUFFER_TYPE ) * tMeshDef.tIndexBufferDef.nIndexCount;
	tMeshDef.tIndexBufferDef.pLocalData = (void *) MALLOC( NULL, tMeshDef.tIndexBufferDef.nBufferSize );
	memset( tMeshDef.pnTextures, -1, sizeof( int ) * NUM_TEXTURE_TYPES );

	{ // duplicate the index buffer so that it indexes for each batch
		INDEX_BUFFER_TYPE * pwIndexBuffer = (INDEX_BUFFER_TYPE*)tMeshDef.tIndexBufferDef.pLocalData;
		int nIndicesPerBatch = pModelFile->wTriangleCount * 3;
		int nIndexBufferSizePerMesh = sizeof( INDEX_BUFFER_TYPE ) * nIndicesPerBatch;
		for ( int nBatch = 0; nBatch < pDefinition->nMeshesPerBatchMax; nBatch++ )
		{
			memcpy( pwIndexBuffer, pwFileIndexBuffer, nIndexBufferSizePerMesh );
			if ( nBatch )
			{
				INDEX_BUFFER_TYPE * pwCurr = pwIndexBuffer;
				int nIndexOffset = nBatch * pModelFile->nVertexCount;
				for ( int i = 0; i < nIndicesPerBatch; i++ )
				{
					*pwCurr += nIndexOffset; 
					pwCurr++;
				}
			}
			pwIndexBuffer += nIndicesPerBatch;
		}
	}

	D3D_VERTEX_BUFFER_DEFINITION * pVertexBufferDef = dxC_ModelDefinitionAddVertexBufferDef( pModelDefinition, &tMeshDef.nVertexBufferIndex );
	tMeshDef.ePrimitiveType = D3DPT_TRIANGLELIST;

	dx9_ModelDefinitionAddMesh( pModelDefinition, &tMeshDef );

	pVertexBufferDef->dwFVF = D3DC_FVF_NULL;
	pVertexBufferDef->eVertexType = VERTEX_DECL_PARTICLE_MESH;
	ASSERT( sizeof ( PARTICLE_MESH_VERTEX_MULTI ) == 16 ); // 16 is a good size for a vertex - it fits on borders well

	pVertexBufferDef->nVertexCount = pDefinition->nMeshesPerBatchMax * pModelFile->nVertexCount;
	pVertexBufferDef->nBufferSize[ 0 ] = pDefinition->nMeshesPerBatchMax * pVertexBufferDef->nVertexCount * dxC_GetVertexSize( 0, pVertexBufferDef );
	ASSERT_RETFAIL( pVertexBufferDef->nBufferSize[ 0 ] );
	pVertexBufferDef->pLocalData[ 0 ] = (void *) MALLOC( NULL, pVertexBufferDef->nBufferSize[ 0 ] );
	ASSERT_RETFAIL( pVertexBufferDef->pLocalData[ 0 ] );

	// index each copy of the vertices so that they know what constant data to use
	ASSERT( pDefinition->nMeshesPerBatchMax * 3 <= 256 ); // if this isn't true, change the shader and calculate the matrix index in the VS
	ASSERT( pDefinition->nMeshesPerBatchMax <= 256 ); // if this isn't true, change the vertex format - try to get a good size
	PARTICLE_MESH_VERTEX_MULTI * pVertex = (PARTICLE_MESH_VERTEX_MULTI *)pVertexBufferDef->pLocalData[ 0 ];
	BYTE * pFileVertexBuffer = pbFileStart + pModelFile->pnVertexBufferStart[ PARTICLE_MODEL_VERTEX_TYPE_MULTI ];
	for ( int nBatch = 0; nBatch < pDefinition->nMeshesPerBatchMax; nBatch ++ )
	{
		memcpy( pVertex, pFileVertexBuffer, pModelFile->nVertexCount * sizeof( PARTICLE_MESH_VERTEX_MULTI ) );

		for ( int i = 0; i < pModelFile->nVertexCount; i++ )
		{
			pVertex->bMatrixIndex = 3 * nBatch;
			pVertex->bModelIndex  = nBatch;
			pVertex++;
		}
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void e_ParticleDrawInitFrameLightmap(
	PARTICLE_DRAW_SECTION eSection,
	PARTICLE_DRAW_INFO & tDrawInfo )
{
	e_ParticleDrawEnterSection( eSection );

	// only draw one side of triangles - we are submitting them counter-clockwise
	//for ( int i = 0; i < NUM_PARTICLE_EFFECTS; i++ )
	//{
	//	//if ( sgnEffects[ i ] == INVALID_ID )
	//	//	continue;
	//	//sgpEffects[ i ] = dx9_EffectGet( sgnEffects[ i ] );
	//	//ASSERT ( sgpEffects[ i ] );
	//	//if ( ! dxC_EffectIsLoaded( sgpEffects[ i ] ) )
	//	//	continue;

	//	//D3D_EFFECT * pEffect = sgpEffects[ i ];
	//	//EFFECT_TECHNIQUE * pTechnique = dx9_EffectGetTechniquePtrParticle( pEffect, 0 );
	//	//if ( ! pTechnique )
	//	//	continue;

	//	//dx9_EffectSetFogParameters( pEffect, *pTechnique );
	//	//D3DXVECTOR4 vEyePos;
	//	//*(VECTOR*)&vEyePos = tDrawInfo.vEyePosition;
	//	//dx9_EffectSetVector( pEffect, *ptTechnique, EFFECT_PARAM_EYEINOBJECT, &vEyePos );  // I know, it's in world, not object
	//}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void e_ParticleDrawEndFrameLightmap()
{
	e_ParticleDrawLeaveSection();

	V( dxC_SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW ) );
	V( dxC_SetRenderState( D3DRS_ZWRITEENABLE, TRUE ) );
	V( dxC_SetRenderState( D3DRS_ZENABLE, TRUE ) );
	V( e_SetFogEnabled( e_GetRenderFlag( RENDER_FLAG_FOG_ENABLED ) ) );
}
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static inline BYTE sZeroToOneFloatToByte( float fVal )
{
	return (BYTE)(255 * fVal);
}

#ifdef HAVOKFX_ENABLED
static PRESULT sCreatePerInstanceNoiseBuffer( D3D_MODEL_DEFINITION * pModelDefinition )
{
	D3D_VERTEX_BUFFER_DEFINITION * pVertexBufferDef = dxC_ModelDefinitionAddVertexBufferDef( pModelDefinition, 0);

	pVertexBufferDef->dwFVF = D3DC_FVF_NULL;
	pVertexBufferDef->eVertexType = VERTEX_DECL_PARTICLE_MESH_HAVOKFX;

	pVertexBufferDef->nVertexCount = MAX_INSTANCES_PER_BATCH;
	pVertexBufferDef->nBufferSize[ 0 ] = pVertexBufferDef->nVertexCount * dxC_GetVertexSize( 0, pVertexBufferDef );
	ASSERT_RETFAIL( pVertexBufferDef->nBufferSize[ 0 ] );
	pVertexBufferDef->pLocalData[ 0 ] = (void *) MALLOC( NULL, pVertexBufferDef->nBufferSize[ 0 ] );
	ASSERT_RETFAIL( pVertexBufferDef->pLocalData[ 0 ] );

	RAND Rand;
	RandInit( Rand, (DWORD)pVertexBufferDef->pLocalData[ 0 ] );

	for(int i = 0; i < MAX_INSTANCES_PER_BATCH; ++i)
	{
		float test = RandGetFloat(Rand, -(float)i, (float)i);
		((float*)pVertexBufferDef->pLocalData[ 0 ])[i] = test;//RandGetFloat(Rand, -(float)i, (float)i);
	}

	return S_OK;
}
#endif

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
struct PARTICLE_MODEL_CALLBACKDATA
{
	int nParticleSystemId;
};

static PRESULT sParticleModelDefPostAsyncLoad( 
	ASYNC_DATA & data)
{
	PAKFILE_LOAD_SPEC * spec = (PAKFILE_LOAD_SPEC *)data.m_UserData;
	ASSERT_RETINVALIDARG(spec);

	if (data.m_IOSize == 0) 
	{
		return S_FALSE;
	}

	PARTICLE_MODEL_CALLBACKDATA * pCallbackData = (PARTICLE_MODEL_CALLBACKDATA *)spec->callbackData;
	ASSERT_RETFAIL(pCallbackData);

	ONCE
	{
		if ( ! spec->buffer )
			break;

		PARTICLE_SYSTEM_DEFINITION * pParticleSystemDef = (PARTICLE_SYSTEM_DEFINITION *) DefinitionGetById( DEFINITION_GROUP_PARTICLE, pCallbackData->nParticleSystemId );
		ASSERT_BREAK( pParticleSystemDef != NULL )

		BYTE * pbFileStart = (BYTE *)spec->buffer;
		ASSERT_BREAK( pbFileStart );
		ASSERT_BREAK( data.m_IOSize >= sizeof(FILE_HEADER) );

		FILE_HEADER * pFileHeader = (FILE_HEADER *) pbFileStart;
		ASSERT_BREAK( pFileHeader->dwMagicNumber == PARTICLE_MODEL_FILE_MAGIC_NUMBER);
		ASSERT_BREAK( pFileHeader->nVersion == PARTICLE_MODEL_FILE_VERSION);

		ASSERT_BREAK( data.m_IOSize >= sizeof(FILE_HEADER) + sizeof(PARTICLE_MODEL_FILE) );
		PARTICLE_MODEL_FILE * pModelFile = (PARTICLE_MODEL_FILE *)(pbFileStart + sizeof(FILE_HEADER));
		INDEX_BUFFER_TYPE * pwFileIndexBuffer = (INDEX_BUFFER_TYPE *)((BYTE *)pModelFile + sizeof( PARTICLE_MODEL_FILE ));

		// SERIOUS TODO: check that the size of file is adequate for pwFileIndexBuffer, otherwise major security leak / buffer overrun!!!

		// -- Create Model --
		D3D_MODEL_DEFINITION * pModelDefinition = MALLOCZ_TYPE( D3D_MODEL_DEFINITION, NULL, 1 );
		PStrCopy( pModelDefinition->tHeader.pszName, spec->fixedfilename, MAX_XML_STRING_LENGTH );
		pParticleSystemDef->nModelDefId = dx9_ModelDefinitionNewFromMemory ( pModelDefinition, LOD_LOW, FALSE, INVALID_ID );
		e_ModelDefinitionAddRef( pParticleSystemDef->nModelDefId );
		ASSERT_BREAK( pParticleSystemDef->nModelDefId != INVALID_ID );
		
		// We don't want to track particle models.
		MODEL_DEFINITION_HASH* pHash = dxC_ModelDefinitionGetHash( pParticleSystemDef->nModelDefId );
		ASSERT_BREAK( pHash );
		if ( pHash->tEngineRes.IsTracked() )
			gtReaper.tModels.Remove( &pHash->tEngineRes );
		pModelDefinition->tBoundingBox = pModelFile->tMultiMeshBoundingBox;
		pModelDefinition->nMeshesPerBatchMax = pParticleSystemDef->nMeshesPerBatchMax;
		pParticleSystemDef->tMultiMeshCompressionBox = pModelFile->tMultiMeshBoundingBox;

		// -- Create Meshes --

		// Create Mesh for Simple vertices - used for drawing many copies at once using constants 
		V( sCreateMeshForConstantBufferDuplication( pParticleSystemDef, pModelDefinition, pModelFile, pwFileIndexBuffer, pbFileStart ) );
#ifdef HAVOKFX_ENABLED
		if ( e_HavokFXIsEnabled() )
		{
			V( sCreateMeshForHavokFX( pModelDefinition, pModelFile, pwFileIndexBuffer, pbFileStart ) );
			V( sCreatePerInstanceNoiseBuffer( pModelDefinition ) );
		}
#endif

		V( e_ModelDefinitionRestore ( pParticleSystemDef->nModelDefId, LOD_LOW ) );
	}

	return S_OK;
}


//-------------------------------------------------------------------------------------------------
// TODO: 
// adjust MAX_PARTICLE_MESHES_PER_BATCH to constant register size - how do we compile the fxo differently or declare the variable with non-fixed length?
//-------------------------------------------------------------------------------------------------

void e_ParticleSystemLoadModel(
	PARTICLE_SYSTEM_DEFINITION * pDefinition )
{
	if ( pDefinition->nModelDefId != INVALID_ID )
		return;

	if ( pDefinition->pszModelDefName[ 0 ] == 0 || 
		PStrCmp( pDefinition->pszModelDefName, BLANK_STRING ) == 0 )
	{
		pDefinition->nModelDefId = INVALID_ID;
		return;
	}

	pDefinition->nModelDefId = e_ModelDefinitionFind( pDefinition->pszModelDefName, MAX_XML_STRING_LENGTH );
	if ( pDefinition->nModelDefId != INVALID_ID )
	{
		D3D_MODEL_DEFINITION * pModelDefinition = dxC_ModelDefinitionGet( pDefinition->nModelDefId, LOD_LOW );
		if ( pModelDefinition )
		{
			pDefinition->nMeshesPerBatchMax = pModelDefinition->nMeshesPerBatchMax;
			pDefinition->tMultiMeshCompressionBox = pModelDefinition->tBoundingBox;
		}
		return;
	}

	char szFileWithPath[ MAX_PATH ];
	PStrPrintf( szFileWithPath, MAX_PATH, "%s%s", FILE_PATH_PARTICLE, pDefinition->pszModelDefName );

	char pszModelFileName[DEFAULT_FILE_WITH_PATH_SIZE];
	PStrReplaceExtension(pszModelFileName, DEFAULT_FILE_WITH_PATH_SIZE, szFileWithPath, PARTICLE_MODEL_FILE_EXTENSION );

	DebugString( OP_LOW, LOADING_STRING(particle mesh), pDefinition->pszModelDefName );

	// check and see if we need to update the binary file
	if ( AppCommonAllowAssetUpdate() && FileNeedsUpdate(pszModelFileName, szFileWithPath, PARTICLE_MODEL_FILE_MAGIC_NUMBER, PARTICLE_MODEL_FILE_VERSION) )
	{
		granny_file * pGrannyFile = e_GrannyReadFile( szFileWithPath );

		ASSERTX_RETURN( pGrannyFile, szFileWithPath );

		granny_file_info * pFileInfo =	GrannyGetFileInfo( pGrannyFile );
		ASSERT_RETURN( pFileInfo );

		// -- Coordinate System --
		// Tell Granny to transform the file into our coordinate system
		e_GrannyConvertCoordinateSystem ( pFileInfo );

		// -- The Model ---
		// Right now we assumes that there is only one model in the file.
		if ( pFileInfo->ModelCount > 1 )
		{
			const GLOBAL_DEFINITION* pGlobal = DefinitionGetGlobal();
			if ( pGlobal->dwFlags & GLOBAL_FLAG_DATA_WARNINGS )
			{
				ErrorDialog( "Too many models in %s.  Model count is %d", pDefinition->pszModelDefName, pFileInfo->MaterialCount );
			}
		}

		granny_model * pGrannyModel = pFileInfo->Models[0]; 

		int nMeshIndex = 0;
		{
			int nMostVertices = 0;
			for ( int i = 0; i < pGrannyModel->MeshBindingCount; i++ )
			{
				granny_mesh * pMesh = pGrannyModel->MeshBindings[ i ].Mesh;
				if ( ! GrannyGetMeshIndexCount ( pMesh ) || 
					! GrannyGetMeshTriangleGroupCount( pMesh ) )
					continue;
				int nVertexCount = GrannyGetMeshVertexCount( pMesh );
				if ( nVertexCount > nMostVertices )
				{
					nMeshIndex = i;
					nMostVertices = nVertexCount;
				}
			}
		}
		ASSERT_RETURN( nMeshIndex != pGrannyModel->MeshBindingCount );
		granny_mesh * pMesh = pGrannyModel->MeshBindings[ nMeshIndex ].Mesh;

		ASSERT_RETURN ( GrannyGetMeshTriangleGroupCount( pMesh ) == 1 );
		granny_tri_material_group *pGrannyTriangleGroupArray = GrannyGetMeshTriangleGroups( pMesh );
		granny_tri_material_group * pGrannyTriangleGroup = & pGrannyTriangleGroupArray[ 0 ];

		// -- Index Buffer --
		int nMeshIndexCount = GrannyGetMeshIndexCount ( pMesh );
		ASSERT_RETURN ( nMeshIndexCount > 0 );

		TCHAR fullfilename[MAX_PATH];
		FileGetFullFileName(fullfilename, pszModelFileName, MAX_PATH);

		// -- Create Particle Model File --
		HANDLE hFile = CreateFile( fullfilename, GENERIC_WRITE | GENERIC_READ, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
		if ( hFile == INVALID_HANDLE_VALUE )
		{
			if ( DataFileCheck( fullfilename ) )
				hFile = CreateFile( fullfilename, GENERIC_WRITE | GENERIC_READ, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL );
		}		
		ASSERTX_RETURN( hFile != INVALID_HANDLE_VALUE, fullfilename );
		
		// write file header
		FILE_HEADER tFileHeader;
		//tFileHeader.dwMagicNumber = DRLG_FILE_MAGIC_NUMBER; // save this at the end
		tFileHeader.nVersion = PARTICLE_MODEL_FILE_VERSION;
		DWORD dwBytesWritten = 0;
		WriteFile( hFile, &tFileHeader, sizeof( FILE_HEADER ), &dwBytesWritten, NULL );
		int nFileOffset = dwBytesWritten;

		PARTICLE_MODEL_FILE tModelFile;
		structclear(tModelFile);
		tModelFile.wTriangleCount = pGrannyTriangleGroup->TriCount;
		ASSERT_RETURN( tModelFile.wTriangleCount );
		tModelFile.nIndexBufferSize = 3 * sizeof( INDEX_BUFFER_TYPE ) * tModelFile.wTriangleCount;

		// write the rest of the file header - we will rewrite it again later with offsets
		BOOL bWritten = WriteFile( hFile, &tModelFile, sizeof( PARTICLE_MODEL_FILE ), &dwBytesWritten, NULL );
		ASSERT( dwBytesWritten == sizeof( PARTICLE_MODEL_FILE ) );
		nFileOffset += dwBytesWritten;

		{
			// -- Index Buffer --
			INDEX_BUFFER_TYPE * pwIndexBuffer = (INDEX_BUFFER_TYPE *) MALLOC( NULL, tModelFile.nIndexBufferSize );
			GrannyCopyMeshIndices ( pMesh, sizeof ( INDEX_BUFFER_TYPE ), (BYTE *) pwIndexBuffer );

			// write the index buffer
			bWritten = WriteFile( hFile, pwIndexBuffer, tModelFile.nIndexBufferSize, &dwBytesWritten, NULL );
			ASSERT( dwBytesWritten == tModelFile.nIndexBufferSize );
			nFileOffset += dwBytesWritten;
			FREE( NULL, pwIndexBuffer );
		}

		// -- Vertex Buffer --
		tModelFile.nVertexCount = GrannyGetMeshVertexCount( pMesh );
		ASSERT_RETURN( tModelFile.nVertexCount );

		// Convert Granny's vertices to our format 
		GRANNY_PARTICLE_MESH_VERTEX * pGrannyVerts = (GRANNY_PARTICLE_MESH_VERTEX *) MALLOC( NULL, tModelFile.nVertexCount * sizeof( GRANNY_PARTICLE_MESH_VERTEX ) );
		GrannyCopyMeshVertices( pMesh, g_ptGrannyParticleMeshVertexDataType, (BYTE *)pGrannyVerts );

		{ // PARTICLE_MODEL_VERTEX_TYPE_MULTI
			{
				tModelFile.tMultiMeshBoundingBox.vMin = pGrannyVerts->vPosition;
				tModelFile.tMultiMeshBoundingBox.vMax = tModelFile.tMultiMeshBoundingBox.vMin;
				GRANNY_PARTICLE_MESH_VERTEX * pCurr = pGrannyVerts;
				for ( int i = 0; i < tModelFile.nVertexCount; i++, pCurr++ )
				{
					BoundingBoxExpandByPoint( &tModelFile.tMultiMeshBoundingBox, (const VECTOR *) &pCurr->vPosition );
				}
				ASSERT( ! VectorIsZero( BoundingBoxGetSize( & tModelFile.tMultiMeshBoundingBox ) ) );
			}

			PARTICLE_MESH_VERTEX_MULTI * pVertices = (PARTICLE_MESH_VERTEX_MULTI *) MALLOCZ( NULL, tModelFile.nVertexCount * sizeof( PARTICLE_MESH_VERTEX_MULTI ) );
			ASSERT( sizeof( PARTICLE_MESH_VERTEX_MULTI ) == 16 ); // just making sure that we are on a 32 byte border

			// convert the granny verts to our format
			PARTICLE_MESH_VERTEX_MULTI * pVertex = &pVertices[ 0 ];
			GRANNY_PARTICLE_MESH_VERTEX * pGrannyVertex = & pGrannyVerts[ 0 ];
			VECTOR vScale = BoundingBoxGetSize( & tModelFile.tMultiMeshBoundingBox );
			float fScale = MAX( vScale.fX, MAX( vScale.fY, vScale.fZ ));
			for ( int i = 0; i < tModelFile.nVertexCount; i++ )
			{
				pVertex->fTextureCoords[ 0 ] = sZeroToOneFloatToByte( pGrannyVertex->fTextureCoords[ 0 ] );
				pVertex->fTextureCoords[ 1 ] = sZeroToOneFloatToByte( pGrannyVertex->fTextureCoords[ 1 ] );
				pVertex->bMatrixIndex = 0;
				pVertex->bModelIndex = 0;

				// transform point into a zero to one space so that it can be compressed
				VECTOR vPosition = pGrannyVertex->vPosition;
				vPosition -= tModelFile.tMultiMeshBoundingBox.vMin;
				vPosition /= fScale;

				pVertex->pPosition[ 0 ] = sZeroToOneFloatToByte( vPosition.x );
				pVertex->pPosition[ 1 ] = sZeroToOneFloatToByte( vPosition.y );
				pVertex->pPosition[ 2 ] = sZeroToOneFloatToByte( vPosition.z );
				pVertex->pPosition[ 3 ] = 1;

				pVertex->Normal		= dx9_CompressVectorToColor( (D3DXVECTOR3 &) pGrannyVertex->vNormal );
				pVertex->Tangent	= dx9_CompressVectorToColor( (D3DXVECTOR3 &) pGrannyVertex->vTangent );

				pVertex++;
				pGrannyVertex++;
			}

			bWritten = WriteFile( hFile, pVertices, tModelFile.nVertexCount * sizeof( PARTICLE_MESH_VERTEX_MULTI ), &dwBytesWritten, NULL );
			ASSERT( dwBytesWritten == tModelFile.nVertexCount * sizeof( PARTICLE_MESH_VERTEX_MULTI ) );
			tModelFile.pnVertexBufferStart[ PARTICLE_MODEL_VERTEX_TYPE_MULTI ] = nFileOffset;
			nFileOffset += dwBytesWritten;

			FREE( NULL, pVertices );
		}

		{ // PARTICLE_MODEL_VERTEX_TYPE_NORMALS_AND_ANIMS
			PARTICLE_MESH_VERTEX_NORMALS_AND_ANIMS * pVertices = (PARTICLE_MESH_VERTEX_NORMALS_AND_ANIMS *) MALLOCZ( NULL, tModelFile.nVertexCount * sizeof( PARTICLE_MESH_VERTEX_NORMALS_AND_ANIMS ) );
			// convert the granny verts to our format
			PARTICLE_MESH_VERTEX_NORMALS_AND_ANIMS * pVertex = &pVertices[ 0 ];
			GRANNY_PARTICLE_MESH_VERTEX * pGrannyVertex = & pGrannyVerts[ 0 ];
			for ( int i = 0; i < tModelFile.nVertexCount; i++ )
			{
				pVertex->fTextureCoords[ 0 ] = sZeroToOneFloatToByte( pGrannyVertex->fTextureCoords[ 0 ] );
				pVertex->fTextureCoords[ 1 ] = sZeroToOneFloatToByte( pGrannyVertex->fTextureCoords[ 1 ] );
				pVertex->bBoneIndex = pGrannyVertex->pwIndices[ 0 ];

				pVertex->vPosition = pGrannyVertex->vPosition;

				VectorNormalize( pGrannyVertex->vNormal );

				pVertex->vNormal = pGrannyVertex->vNormal;

				pVertex++;
				pGrannyVertex++;
			}

			bWritten = WriteFile( hFile, pVertices, tModelFile.nVertexCount * sizeof( PARTICLE_MESH_VERTEX_NORMALS_AND_ANIMS ), &dwBytesWritten, NULL );
			ASSERT( dwBytesWritten == tModelFile.nVertexCount * sizeof( PARTICLE_MESH_VERTEX_NORMALS_AND_ANIMS ) );
			tModelFile.pnVertexBufferStart[ PARTICLE_MODEL_VERTEX_TYPE_NORMALS_AND_ANIMS ] = nFileOffset;
			nFileOffset += dwBytesWritten;

			FREE( NULL, pVertices );
		}

		if ( pGrannyVerts )
			FREE ( NULL, pGrannyVerts );

		// now write the header again with the magic number to show that we finished successfully
		tFileHeader.dwMagicNumber	= PARTICLE_MODEL_FILE_MAGIC_NUMBER; // add the magic number at the end in case we stop while debugging

		SetFilePointer( hFile, 0, 0, FILE_BEGIN );
		WriteFile( hFile, &tFileHeader, sizeof( FILE_HEADER ), &dwBytesWritten, NULL );
		WriteFile( hFile, &tModelFile, sizeof( PARTICLE_MODEL_FILE ), &dwBytesWritten, NULL );
		ASSERT( dwBytesWritten == sizeof( PARTICLE_MODEL_FILE ) );

		FileClose( hFile );

		GrannyFreeAllFileSections( pGrannyFile );
	}

	// -- Load File --
	DECLARE_LOAD_SPEC(spec, pszModelFileName);
	spec.flags |= PAKFILE_LOAD_ADD_TO_PAK | PAKFILE_LOAD_FREE_BUFFER | PAKFILE_LOAD_FREE_CALLBACKDATA;
	spec.priority = ASYNC_PRIORITY_PARTICLE_DEFINITIONS;
	spec.fpLoadingThreadCallback = sParticleModelDefPostAsyncLoad;

	PARTICLE_MODEL_CALLBACKDATA * pCallbackData = (PARTICLE_MODEL_CALLBACKDATA *) MALLOC( NULL, sizeof(PARTICLE_MODEL_CALLBACKDATA) );
	pCallbackData->nParticleSystemId = pDefinition->tHeader.nId;
	spec.callbackData = pCallbackData;
	PakFileLoad(spec);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int e_ParticlesGetDrawnTotal( PARTICLE_DRAW_SECTION eSection /*= PARTICLE_DRAW_SECTION_INVALID*/ )
{
	if ( eSection == PARTICLE_DRAW_SECTION_INVALID )
	{
		int nDrawn = 0;
		for ( int i = 0; i < NUM_PARTICLE_DRAW_SECTIONS; i++ )
		{
			nDrawn += sgtParticleDrawStats[ i ].nDrawn;
		}
		return nDrawn;
	}

	ASSERT_RETZERO( IS_VALID_INDEX( eSection, NUM_PARTICLE_DRAW_SECTIONS ) );
	return sgtParticleDrawStats[ eSection ].nDrawn;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

float e_ParticlesGetTotalWorldArea( PARTICLE_DRAW_SECTION eSection /*= PARTICLE_DRAW_SECTION_INVALID*/ )
{
	if ( eSection == PARTICLE_DRAW_SECTION_INVALID )
	{
		float fWorldArea = 0;
		for ( int i = 0; i < NUM_PARTICLE_DRAW_SECTIONS; i++ )
		{
			fWorldArea += sgtParticleDrawStats[ i ].fWorldArea;
		}
		return fWorldArea;
	}

	ASSERT_RETVAL( IS_VALID_INDEX( eSection, NUM_PARTICLE_DRAW_SECTIONS ), 0.f );
	return sgtParticleDrawStats[ eSection ].fWorldArea;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

float e_ParticlesGetTotalScreenArea( 
	PARTICLE_DRAW_SECTION eSection /*= PARTICLE_DRAW_SECTION_INVALID*/,
	BOOL bPredicted /*= FALSE*/ )
{
	if ( eSection == PARTICLE_DRAW_SECTION_INVALID )
	{
		float fScreenArea = 0;
		for ( int i = 0; i < NUM_PARTICLE_DRAW_SECTIONS; i++ )
		{
			fScreenArea += bPredicted ? sgtParticleDrawStats[ i ].fScreenAreaPredicted : sgtParticleDrawStats[ i ].fScreenAreaMeasured;
		}
		return fScreenArea;
	}

	ASSERT_RETVAL( IS_VALID_INDEX( eSection, NUM_PARTICLE_DRAW_SECTIONS ), 0.f );
	return bPredicted ? sgtParticleDrawStats[ eSection ].fScreenAreaPredicted : sgtParticleDrawStats[ eSection ].fScreenAreaMeasured;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static PRESULT sSetHavokFXDrawParams( 
	D3D_EFFECT * pEffect,
	EFFECT_TECHNIQUE & tTechnique,
	PARTICLE_SYSTEM * pParticleSystem )
{
	V( dx9_EffectSetColor( pEffect, tTechnique, EFFECT_PARAM_PARTICLE_GLOBAL_COLOR, (D3DXVECTOR4*)&pParticleSystem->vHavokFXSystemColor ) );

	VECTOR4 vGlowColor;
	vGlowColor.fX = vGlowColor.fY = vGlowColor.fZ = 0.0f;
	vGlowColor.fW = pParticleSystem->fHavokFXSystemGlow;
 	V( dx9_EffectSetColor( pEffect, tTechnique, EFFECT_PARAM_PARTICLE_GLOBAL_GLOW, (D3DXVECTOR4*)&vGlowColor ) );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static float sGetBugAnimationLoop(
	float fTime,
	float fMultiplier )
{
	float fVal = fMultiplier * fTime;

	// get into 0 to 1 range
	fVal = fVal - FLOOR(fVal); 

	const float fMaxValue = TWOxPI;
	// change it so that it loops between 0 and 2 * PI
	fVal *= 2.0f * fMaxValue; 
	if ( fVal > fMaxValue )
		fVal = 2.0f * fMaxValue - fVal;

	return fVal;
}
//inline float sGetBugAnimationLoop(
//	float fTime,
//	float fMultiplier )
//{
//	return fmodf( fMultiplier * fTime, TWOxPI );
//}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

#ifdef HAVOKFX_ENABLED
static PRESULT e_ParticlesDrawMeshesWithHavokFX( 
	PARTICLE_DRAW_INFO & tDrawInfo,
	int nVerticesToSkip,
	PARTICLE_SYSTEM * pParticleSystem )
{
	if ( ! e_HavokFXIsEnabled() )
		return S_FALSE;

	LPD3DCDEVICE pDevice = dxC_GetDevice();

	PARTICLE_SYSTEM_DEFINITION * pDefinition = pParticleSystem->pDefinition;

	ASSERT_RETFAIL( pDefinition->nModelDefId != INVALID_ID );
	D3D_MODEL_DEFINITION * pModelDef = dxC_ModelDefinitionGet( pDefinition->nModelDefId, LOD_LOW );
	if ( ! pModelDef )
		return S_FALSE;

	EFFECT_DEFINITION * pEffectDef = e_ParticleSystemGetEffectDef( pDefinition );
	ASSERT_RETFAIL( pEffectDef );
	ASSERT_RETFAIL( TESTBIT_MACRO_ARRAY( pEffectDef->dwFlags, EFFECTDEF_FLAGBIT_REQUIRES_HAVOKFX ) );

	ENVIRONMENT_DEFINITION * pEnvDef = e_GetCurrentEnvironmentDef();
	if ( ! pEnvDef )
		return E_FAIL;

	D3D_EFFECT * pEffect = e_ParticleSystemGetEffect( pDefinition );
	if ( ! dxC_EffectIsLoaded( pEffect ) )
		return E_FAIL;

	ASSERT_RETFAIL( pDefinition );
	EFFECT_TECHNIQUE * pTechnique = e_ParticleSystemGetTechnique( pDefinition, pEffect );
	if ( ! pTechnique )
		return E_FAIL;
	ASSERT_RETFAIL( pTechnique->hHandle );

	UINT nPassCount;
	V_RETURN( dxC_SetTechnique( pEffect, pTechnique, &nPassCount ) );

	D3D_MESH_DEFINITION * pMeshDef = dx9_ModelDefinitionGetMesh ( pModelDef, PARTICLE_MODEL_VERTEX_TYPE_NORMALS_AND_ANIMS );
	ASSERT_RETFAIL( pMeshDef );

	D3D_VERTEX_BUFFER_DEFINITION & tVertexBufferDef = pModelDef->ptVertexBufferDefs[ pMeshDef->nVertexBufferIndex ];

	D3DXVECTOR4 vEyePos;
	*(VECTOR*)&vEyePos = tDrawInfo.vEyePosition;
	V( dx9_EffectSetVector( pEffect, *pTechnique, EFFECT_PARAM_EYEINOBJECT, &vEyePos ) );  // I know, it's in world, not object

	// CML 2006.10.23: These are now set in dxC_SetStreamSource based on the vbufferdef
	//V( dx9_SetFVF( D3DC_FVF_NULL ) );
	//V( dxC_SetVertexDeclaration( VERTEX_DECL_PARTICLE_MESH_HAVOKFX, pEffect ) );

	int nParticleCount = min( pParticleSystem->nHavokFXParticleCount, MAX_INSTANCES_PER_BATCH );

	int nStreamCurr = 0;
	V_RETURN( dxC_SetStreamSource( tVertexBufferDef, 0 ) );
	//dx9_GetDevice()->SetStreamSourceFreq( nStreamCurr, D3DSTREAMSOURCE_INDEXEDDATA | nParticleCount );
	nStreamCurr++;
  
	for ( int i = 0; i < HAVOKFX_NUM_RIGIDBODY_TRANSFORM_ROWS; i++ )
	{
		int nOffsetToSkipFloor = nVerticesToSkip * 4 * sizeof( float );
		V( dxC_SetStreamSource( nStreamCurr, (LPD3DCVB)tDrawInfo.ppRigidBodyTransforms[ i ], nOffsetToSkipFloor, 4 * sizeof( float ) ) );
		//dx9_GetDevice()->SetStreamSourceFreq( nStreamCurr, D3DSTREAMSOURCE_INSTANCEDATA | 1 );
		nStreamCurr++;
	}

	//KMNV TODO HACK: Assuming seed buffer is last
	{
		D3D_VERTEX_BUFFER_DEFINITION * pRandomVertexBuffer = &pModelDef->ptVertexBufferDefs[ pModelDef->nVertexBufferDefCount - 1 ];
		V( dxC_SetStreamSource( *pRandomVertexBuffer, 0 ) );
		//dx9_GetDevice()->SetStreamSourceFreq( nStreamCurr, D3DSTREAMSOURCE_INSTANCEDATA | 1 );
		nStreamCurr++;
	}

	if ( pDefinition->nTextureId != INVALID_ID )
	{
		D3D_TEXTURE * pTexture = dxC_TextureGet( pDefinition->nTextureId );

		V( dx9_EffectSetTexture( pEffect, *pTechnique, EFFECT_PARAM_DIFFUSEMAP0, pTexture->GetShaderResourceView() ) );

		dx9_ReportSetTexture( pTexture->nId );
	}

	V( sSetHavokFXDrawParams( pEffect, *pTechnique, pParticleSystem ) );

	V( dx9_EffectSetMatrix ( pEffect, *pTechnique, EFFECT_PARAM_VIEWPROJECTIONMATRIX,
		e_ParticleSystemUsesProj2( tDrawInfo.pParticleSystemCurr ) ? &tDrawInfo.matViewProjection2 : &tDrawInfo.matViewProjection ) );

	if ( dx9_EffectIsParamUsed( pEffect, *pTechnique, EFFECT_PARAM_BUG_ANIM_LERP ) )
	{
		enum 
		{
			BUG_ROTATION_WING_MIN,
			BUG_ROTATION_WING_MAX,
			NUM_BUG_ROTATION_MATRICES,
		};
		MATRIX mRotationMatrices[ NUM_BUG_ROTATION_MATRICES ];
		MatrixRotationY( mRotationMatrices[ BUG_ROTATION_WING_MIN	], -PI_OVER_FOUR );
		MatrixRotationY( mRotationMatrices[ BUG_ROTATION_WING_MAX	], PI_OVER_FOUR );

		float pfBoneBuffer[ NUM_BUG_ROTATION_MATRICES * 3 * 4 ];
		float * pflSmallPointer	= pfBoneBuffer;
		for ( int i = 0; i < NUM_BUG_ROTATION_MATRICES; i++ )
		{// this is where we remap and copy the bones for this mesh
			const float * pflBigPointer = (const float *) ( mRotationMatrices [ i ] );
			memcpy( pflSmallPointer, pflBigPointer, sizeof (float) * 12 );
			pflSmallPointer += 12;
		}

		V( dx9_EffectSetRawValue( pEffect, *pTechnique, EFFECT_PARAM_BONES, pfBoneBuffer, 0, 3 * 4 * sizeof(float) * NUM_BUG_ROTATION_MATRICES ) );

		VECTOR4 vBugAnimLerp( 0.0f, 0.0f, 0.0f, 0.0f );
		vBugAnimLerp.fX = sGetBugAnimationLoop( pParticleSystem->fElapsedTime, 4.0f );
		if ( pParticleSystem->pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_FLIP_MESH_ALONG_Y )
			vBugAnimLerp.fY = -1.0f;
		V( dx9_EffectSetVector( pEffect, *pTechnique, EFFECT_PARAM_BUG_ANIM_LERP, &vBugAnimLerp ) );

		D3DXVECTOR4 vEyePos;
		*(VECTOR*)&vEyePos = tDrawInfo.vEyePosition;
		V( dx9_EffectSetVector( pEffect, *pTechnique, EFFECT_PARAM_EYEINOBJECT, &vEyePos ) );  // I know, it's in world, not object

		float fScale = pParticleSystem->fHavokFXSystemScale;
		D3DXVECTOR4 vScale( fScale, fScale, fScale, 1.0f );
		V( dx9_EffectSetVector( pEffect, *pTechnique, EFFECT_PARAM_SCALE, &vScale ) );
	}

	V( dxC_SetIndices( pMeshDef->tIndexBufferDef ) );

	V( dx9_EffectSetDirectionalLightingParams( NULL, pEffect, NULL, NULL, *pTechnique, pEnvDef ) );

	for ( int i = 0; i < (int) nPassCount; i++ )
	{
		V_CONTINUE( dxC_EffectBeginPass( pEffect, i ) );

		if ( pDefinition->dwFlags2 & PARTICLE_SYSTEM_DEFINITION_FLAG2_DRAW_FRONT_FACE_ONLY )
		{
			V( dxC_SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW ) );
		} else {
			V( dxC_SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE ) );
		}

		V( dxC_SetRenderState( D3DRS_ZWRITEENABLE, ( pDefinition->dwFlags2 & PARTICLE_SYSTEM_DEFINITION_FLAG2_WRITE_Z ) != 0 ) );

		V( dxC_SetRenderState( D3DRS_ZENABLE, ( pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_DISABLE_READ_Z ) == 0 ) );

		V( dxC_DrawIndexedPrimitiveInstanced( D3DPT_TRIANGLELIST, 0, tVertexBufferDef.nVertexCount, 0, pMeshDef->wTriangleCount, nParticleCount, nStreamCurr, pEffect->pD3DEffect, METRICS_GROUP_PARTICLE ) );
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static PRESULT e_ParticlesDrawParticlesWithHavokFXParticles( 
	PARTICLE_DRAW_INFO & tDrawInfo,
	PARTICLE_SYSTEM * pParticleSystem )
{
	if ( ! e_HavokFXIsEnabled() )
		return S_FALSE;

	LPD3DCDEVICE pDevice = dxC_GetDevice();

	PARTICLE_SYSTEM_DEFINITION * pDefinition = pParticleSystem->pDefinition;
	if ( ! tDrawInfo.pParticlePositions)// || ! tDrawInfo.pParticleVelocities )
		return S_FALSE;

//	ASSERT( pParticleSystem->pDefinition->nShader == PARTICLE_EFFECT_MESH_HAVOKFX_PARTICLE );
	D3D_EFFECT * pEffect = e_ParticleSystemGetEffect( pDefinition );
	if ( ! dxC_EffectIsLoaded( pEffect ) )
		return S_FALSE;

	ASSERT_RETFAIL( pDefinition );
	EFFECT_TECHNIQUE * pTechnique = e_ParticleSystemGetTechnique( pDefinition, pEffect );
	if ( ! pTechnique )
		return S_FALSE;
	ASSERT_RETFAIL( pTechnique->hHandle );

	UINT nPassCount;
	V_RETURN( dxC_SetTechnique( pEffect, pTechnique, &nPassCount ) );

	D3DXVECTOR4 vEyePos;
	*(VECTOR*)&vEyePos = tDrawInfo.vEyePosition;
	V( dx9_EffectSetVector( pEffect, *pTechnique, EFFECT_PARAM_EYEINOBJECT, &vEyePos ) );  // I know, it's in world, not object

	// CML: 2006.10.23
	//dx9_SetFVF( D3DC_FVF_NULL );
	//dxC_SetVertexDeclaration( VERTEX_DECL_PARTICLE_HAVOKFX_PARTICLE, pEffect );

	ASSERT_RETFAIL( dxC_IndexBufferD3DExists( sgtQuadIndexBuffer.nD3DBufferID ) );

	V( dxC_SetIndices( sgtQuadIndexBuffer ) );

	int nParticleCount = e_HavokFXGetSystemParticleCount( pParticleSystem->nId );

	int nStreamCurr = 0;
	V( dxC_SetStreamSource( sgtParticleSystemVertexBuffers[ PARTICLE_VERTEX_BUFFER_HAVOKFX_PARTICLES ], 0 ) );
	nStreamCurr++;

	int nOffsetToSkipFloor = 0; //nVerticesToSkip * 4 * sizeof( float );
	V( dxC_SetStreamSource( nStreamCurr, (LPD3DCVB)tDrawInfo.pParticlePositions, nOffsetToSkipFloor, 4 * sizeof( float ) ) );
	nStreamCurr++;

	if ( pDefinition->nTextureId != INVALID_ID )
	{
		D3D_TEXTURE * pTexture = dxC_TextureGet( pDefinition->nTextureId );

		V( dx9_EffectSetTexture( pEffect, *pTechnique, EFFECT_PARAM_DIFFUSEMAP0, pTexture->GetShaderResourceView() ) );

		dx9_ReportSetTexture( pTexture->nId );
	}

	float fScale = pParticleSystem->fHavokFXSystemScale;
	D3DXVECTOR4 vScale( fScale, fScale, fScale, 1.0f );
	V( dx9_EffectSetVector( pEffect, *pTechnique, EFFECT_PARAM_SCALE, &vScale ) ); 

	V( sSetHavokFXDrawParams( pEffect, *pTechnique, pParticleSystem ) );

	V( dx9_EffectSetMatrix ( pEffect, *pTechnique, EFFECT_PARAM_VIEWPROJECTIONMATRIX,
		e_ParticleSystemUsesProj2( tDrawInfo.pParticleSystemCurr ) ? &tDrawInfo.matViewProjection2 : &tDrawInfo.matViewProjection ) );

	//ENVIRONMENT_DEFINITION * pEnvDef = e_GetCurrentEnvironmentDef();
	//dx9_EffectSetDirectionalLightingParams( pEffect, NULL, NULL, *pTechnique, pEnvDef );

	for ( int i = 0; i < (int) nPassCount; i++ )
	{
		V_CONTINUE( dxC_EffectBeginPass( pEffect, i ) );

		if ( pDefinition->dwFlags2 & PARTICLE_SYSTEM_DEFINITION_FLAG2_DRAW_FRONT_FACE_ONLY )
		{
			V( dxC_SetRenderState( D3DRS_CULLMODE, D3DCULL_CCW ) );
		} else {
			V( dxC_SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE ) );
		}

		V( dxC_SetRenderState( D3DRS_ZWRITEENABLE, ( pDefinition->dwFlags2 & PARTICLE_SYSTEM_DEFINITION_FLAG2_WRITE_Z ) != 0 ) );

		V( dxC_SetRenderState( D3DRS_ZENABLE, ( pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_DISABLE_READ_Z ) == 0 ) );

		//dxC_DrawIndexedPrimitive( D3DPT_TRIANGLELIST, 0, 0, 4, 0, 2, pEffect->pD3DEffect, METRICS_GROUP_PARTICLE );
		V( dxC_DrawIndexedPrimitiveInstanced( D3DPT_TRIANGLELIST, 0, 4, 0, 2, nParticleCount, nStreamCurr, pEffect->pD3DEffect, METRICS_GROUP_PARTICLE ) );
	}

	return S_OK;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_ParticlesSystemDrawWithHavokFX( 
	PARTICLE_DRAW_INFO & tDrawInfo,
	PARTICLE_SYSTEM * pParticleSystem )
{
	if ( pParticleSystem->pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_USE_HAVOKFX_PARTICLES )
	{
		V( e_ParticlesDrawParticlesWithHavokFXParticles( tDrawInfo, pParticleSystem ) );
	}
	else if ( pParticleSystem->pDefinition->nModelDefId != INVALID_ID )
	{
		int nVerticesToSkip = 0;

		if ( ! e_HavokFXGetSystemVerticesToSkip ( pParticleSystem->nId, nVerticesToSkip )) 
			return S_FALSE;

		V( e_ParticlesDrawMeshesWithHavokFX( tDrawInfo, nVerticesToSkip, pParticleSystem ) );
	}

	return S_OK;
}
#endif

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT e_ParticlesDrawGPUSim( 
	PARTICLE_DRAW_INFO & tDrawInfo,
	PARTICLE_SYSTEM * pParticleSystem, float SmokeRenderThickness, float SmokeAmbientLight, DWORD diffuseColor, float windInfluence, float SmokeDensityModifier, float SmokeVelocityModifier, VECTOR vOffset, float glowMinDensity)
{
#ifdef ENGINE_TARGET_DX10
	PARTICLE_SYSTEM_DEFINITION * pDefinition = pParticleSystem->pDefinition;
	ASSERT_RETFAIL( pDefinition );
	D3D_EFFECT * pEffect = e_ParticleSystemGetEffect( pDefinition );

	EFFECT_TECHNIQUE * pTechnique = e_ParticleSystemGetTechnique( pDefinition, pEffect );
	if ( ! pTechnique )
		return S_FALSE;
	ASSERT_RETFAIL( pTechnique->hHandle );


	//SETUP TEXTURE
	if ( pDefinition->nTextureId != INVALID_ID )
	{
		D3D_TEXTURE * pTexture = dxC_TextureGet( pDefinition->nTextureId );
		if ( tDrawInfo.bPixelShader )
		{
			V( dx9_EffectSetTexture( pEffect, *pTechnique, EFFECT_PARAM_DIFFUSEMAP0, pTexture->GetShaderResourceView() ) );
		}
		else
		{
			V( dxC_SetTexture( 0, pTexture->GetShaderResourceView() ) );
		}
		dx9_ReportSetTexture( pTexture->nId );
	}


	
	tDrawInfo.pParticleSystemCurr2 = pParticleSystem;
	tDrawInfo.pEffect = pEffect;
	tDrawInfo.pTechnique = pTechnique;

	//set rendering parameters for the smoke

	V( dx9_EffectSetFloat(pEffect,*pTechnique,EFFECT_PARAM_SMOKE_AMBIENT_LIGHT,SmokeAmbientLight) );
	V( dx9_EffectSetFloat(pEffect,*pTechnique,EFFECT_PARAM_SMOKE_DENSITY_MODIFIER,SmokeDensityModifier) );
	V( dx9_EffectSetFloat(pEffect,*pTechnique,EFFECT_PARAM_SMOKE_VELOCITY_MODIFIER,SmokeVelocityModifier) );
	V( dx9_EffectSetInt  (pEffect,*pTechnique,EFFECT_PARAM_SMOKE_DECAY_GRID_DISTANCE, pParticleSystem->pDefinition->nGridBorder ) );
    V( dx9_EffectSetFloat(pEffect,*pTechnique,EFFECT_PARAM_SMOKE_GLOW_MIN_DENSITY,glowMinDensity) );
    float glowColCompensation[4] = {pDefinition->vGlowCompensationColor.x, pDefinition->vGlowCompensationColor.y, pDefinition->vGlowCompensationColor.z, 1.0 };
	V( dx9_EffectSetRawValue( pEffect,*pTechnique,EFFECT_PARAM_SMOKE_GLOW_COLOR_COMPENSATION,glowColCompensation,0, sizeof(D3DXVECTOR4) ) );
	float color[4]; 
	sDecompressColorToVector ( color, diffuseColor );		

	float velocityMultiplier = pParticleSystem->pDefinition->fVelocityMultiplier;
	float velocityClamp =  pParticleSystem->pDefinition->fVelocityClamp;
	float sizeMultiplier =  pParticleSystem->pDefinition->fSizeMultiplier;

	V(dx9_EffectSetFloat( pEffect, *pTechnique, EFFECT_PARAM_SMOKE_CONFINEMENT_EPSILON, pDefinition->fVorticityConfinementScale ) );

	//CC@HAVOK
	return dx10_ParticleSystemGPURender(pParticleSystem->nId, tDrawInfo, color, windInfluence, vOffset,velocityMultiplier,velocityClamp,sizeMultiplier,SmokeRenderThickness);
#else
	return S_FALSE;
#endif
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT e_ParticleSystemGPUSimInit( 
	PARTICLE_SYSTEM * pParticleSystem )
{
#ifdef ENGINE_TARGET_DX10
	PARTICLE_SYSTEM_DEFINITION * pDefinition = pParticleSystem->pDefinition;
	ASSERT_RETFAIL( pDefinition );
    return dx10_ParticleSystemGPUInitialize(pParticleSystem);
#else
	return S_FALSE;
#endif
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT e_ParticleSystemGPUSimDestroy( 
	PARTICLE_SYSTEM * pParticleSystem )
{
    DX10_BLOCK( dx10_ParticleSystemGPUDestroy(pParticleSystem->nId); return S_OK; );
	DX9_BLOCK( return S_FALSE; );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
BOOL e_ParticlesIsGPUSimEnabled()
{
#ifdef ENGINE_TARGET_DX9
	return FALSE;
#else
	// CML 2007.08.24 - The fluid sim smoke is misbehaving on AMD DX10 parts.  Disabling for now.
	//if ( dx9_CapGetValue( DX9CAP_GPU_VENDOR_ID ) == DT_NVIDIA_VENDOR_ID )
		return TRUE;
	//return FALSE;
#endif
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
EFFECT_DEFINITION * e_ParticleSystemGetEffectDef( 
	const PARTICLE_SYSTEM_DEFINITION * pDefinition )
{
	int nShaderLocation = e_GetCurrentLevelLocation();
	if( nShaderLocation == LEVEL_LOCATION_UNKNOWN )
	{
		nShaderLocation =  LEVEL_LOCATION_INDOOR;
	}
	if ( nShaderLocation == LEVEL_LOCATION_UNKNOWN )
		return NULL;

	int nEffectDef = INVALID_ID;
	if ( pDefinition->nShaderType != INVALID_ID )
	{
		nEffectDef = dxC_GetShaderTypeEffectDefinitionID( pDefinition->nShaderType, nShaderLocation );
		if ( nEffectDef == INVALID_ID )
			nEffectDef = dxC_GetShaderTypeEffectDefinitionID( pDefinition->nShaderType, SHADER_TYPE_DEFAULT );
		if ( nEffectDef == INVALID_ID )
		{
			V( dx9_LoadShaderType( pDefinition->nShaderType, SHADER_TYPE_DEFAULT ) );
			nEffectDef = dxC_GetShaderTypeEffectDefinitionID( pDefinition->nShaderType, SHADER_TYPE_DEFAULT );
		}
	}

	// If it asks for a different shader and we can't load it, don't bother with basic.
	if ( pDefinition->nShaderType != INVALID_ID && nEffectDef == INVALID_ID )
		return NULL;

	if ( nEffectDef == INVALID_ID )
	{
		int nDefault = sgnEffectShaderDefaultBasic;
		if ( pDefinition->pszModelDefName[ 0 ] != 0 )
			nDefault = sgnEffectShaderDefaultMesh;
		nEffectDef = dxC_GetShaderTypeEffectDefinitionID( nDefault, nShaderLocation );
		if ( nEffectDef == INVALID_ID )
		{
			V( dx9_LoadShaderType( nDefault, SHADER_TYPE_DEFAULT ) );
			nEffectDef = dxC_GetShaderTypeEffectDefinitionID( nDefault, SHADER_TYPE_DEFAULT );
		}
		ASSERT_RETNULL( nEffectDef != INVALID_ID );
	}
	return (EFFECT_DEFINITION *)ExcelGetData(EXCEL_CONTEXT(), DATATABLE_EFFECTS_FILES, nEffectDef );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
D3D_EFFECT * e_ParticleSystemGetEffect( 
	const PARTICLE_SYSTEM_DEFINITION * pDefinition )
{
	EFFECT_DEFINITION * pEffectDef = e_ParticleSystemGetEffectDef( pDefinition );	
	return pEffectDef ? dxC_EffectGet( pEffectDef->nD3DEffectId ) : NULL;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void dxC_ParticlesSetScreenTexture( SPD3DCBASESHADERRESOURCEVIEW pTexture )
{
	sgpParticleScreenTexture = pTexture;
}

SPD3DCBASESHADERRESOURCEVIEW dxC_ParticlesGetScreenTexture()
{
	return sgpParticleScreenTexture;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_ParticleDrawStatsGetString(
	WCHAR * pwszStr,
	int nBufLen )
{
	const int cnBufLen = 1024;
	WCHAR wszTemp[ cnBufLen ];

	PStrPrintf( pwszStr, nBufLen, L"Particle draw stats:\n"  L"%15s %10s %10s %10s %10s\n",
		L"SECTION",
		L"PASSES",
		L"DRAWN",
		L"WORLD",
		L"SCREENS" );

	// this should be global and better-defined...
	// at least it's only used here
	ASSERT( NUM_PARTICLE_DRAW_SECTIONS == 6 );
	const WCHAR szDrawSections[ NUM_PARTICLE_DRAW_SECTIONS ][ 64 ] =
	{
		L"General",
		L"Environment",
		L"Screeneffects",
		L"Pre-lightmap",
		L"Line-of-sight",
		L"Lightmap"
	};

	for ( int i = 0; i < NUM_PARTICLE_DRAW_SECTIONS; i++ )
	{
		PStrPrintf( wszTemp, cnBufLen, L"%15s %10d %10d %10.1f %10.3f\n",
			szDrawSections[ i ],
			sgtParticleDrawStats[ i ].nTimesBegun,
			sgtParticleDrawStats[ i ].nDrawn,
			sgtParticleDrawStats[ i ].fWorldArea,
			sgtParticleDrawStats[ i ].fScreenAreaMeasured );
		PStrCat( pwszStr, wszTemp, nBufLen );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sParticleSystemLoadGPUSimTexture(
	int & nTextureId,
	const char * pszFileName,
	int nFormat )
{
	if ( nTextureId != INVALID_ID )
		return S_FALSE;
	if ( ! pszFileName[0] )
		return S_FALSE;

	char szFile[ DEFAULT_FILE_WITH_PATH_SIZE ];
	PStrPrintf( szFile, DEFAULT_FILE_WITH_PATH_SIZE, "%stextures\\%s", FILE_PATH_PARTICLE, pszFileName );
	V( e_TextureNewFromFile( &nTextureId, szFile, TEXTURE_GROUP_PARTICLE, TEXTURE_NONE, 0, nFormat ) );

	e_TextureAddRef( nTextureId );

	return S_OK;
}

//----------------------------------------------------------------------------

PRESULT e_ParticleSystemLoadGPUSimTextures( PARTICLE_SYSTEM_DEFINITION * pDefinition )
{
	ASSERT_RETINVALIDARG( pDefinition );
	if ( 0 == ( pDefinition->dwFlags3 & PARTICLE_SYSTEM_DEFINITION_FLAG3_DX10_FLUID ) )
		return S_FALSE;
	V( sParticleSystemLoadGPUSimTexture( pDefinition->nDensityTextureId,    pDefinition->pszTextureDensityName,		D3DFMT_A8R8G8B8 ) );
	V( sParticleSystemLoadGPUSimTexture( pDefinition->nVelocityTextureId,   pDefinition->pszTextureVelocityName,	D3DFMT_A8R8G8B8 ) );
	V( sParticleSystemLoadGPUSimTexture( pDefinition->nObstructorTextureId, pDefinition->pszTextureObstructorName,	D3DFMT_L8 ) );
	return S_OK;
}