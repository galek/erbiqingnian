#ifndef __DXC_VERTEX_DECLARATIONS__
#define __DXC_VERTEX_DECLARATIONS__

// This header will auto generate vertex declarations for FXC, DX9 and DX10.
//
// Example:
// VERTEX_DECL_START( VS_BACKGROUND_INPUT_32 )
//	 SLOT_DECLARATION( FLOAT3_32,	Position,			POSITION,	0, VS_BACKGROUND_INPUT_32 )
//	 SLOT_DECLARATION( FLOAT4_8,	Normal,				NORMAL,		0, VS_BACKGROUND_INPUT_32 )
//	 SLOT_DECLARATION( FLOAT4_32,	LightMapDiffuse,	TEXCOORD,	0, VS_BACKGROUND_INPUT_32 )
// VERTEX_DECL_END( VS_BACKGROUND_INPUT_32, 1 )
//
// This generates the following in FX:
// struct VS_BACKGROUND_INPUT_32
// {
//	 float3 Position : POSITION0;
//	 float4 Normal : NORMAL0;
//   float4 LightMapDiffuse : TEXCOORD0;
// }
//
// The following in dx9/dx10:
// struct VS_BACKGROUND_INPUT_32_STREAM_0
// {
//   VECTOR3 Position;
//   VECTOR4 Normal;
//   VECTOR4 LightMapDiffuse;
// }
//
// As well as the following enum which contains information about the vertex declaration
// enum VS_BACKGROUND_INPUT_32_INFO
// {
//	 VS_BACKGROUND_INPUT_32_COUNT			<---- Number of elements in our declaration
//	 VS_BACKGROUND_INPUT_32_STREAM_COUNT	<---- Number of streams in our declaration
// };
//
//
// The following in dx9:
// const D3DC_INPUT_ELEMENT_DESC name[] =
// {
//	 { 0, 0, D3DDECLTYPE_FLOAT3,					D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0 },
//	 { 0, sizeof( D3DXVECTOR4 ),					D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0 },
//	 { 0, sizeof( D3DXVECTOR4 ) + sizeof(DWORD ),	D3DDECLTYPE_FLOAT4, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0 },
//	 D3DDECL_END(), 
// };
//
// The following in dx10:
// const D3DC_INPUT_ELEMENT_DESC name[] =
// {
//	 { "POSITION",	0,	DXGI_FORMAT_R32G32B32_FLOAT,		0,	0,											D3D10_INPUT_PER_VERTEX_DATA,	0},
//	 { "NORMAL",		0,	DXGI_FORMAT_R8G8B8A8_UNORM,		0,	sizeof( D3DXVECTOR4 ),						D3D10_INPUT_PER_VERTEX_DATA,	0},
//	 { "TEXCOORD",	0,	DXGI_FORMAT_R32G32B32A32_FLOAT,		0,	sizeof( D3DXVECTOR4 ) + sizeof(DWORD ),		D3D10_INPUT_PER_VERTEX_DATA,	0},
// };
//
// It is important that all parameters be filled in.  Some things to note:
// SLOT_DECLARATION( type, name, semantic, index, pixosem, declname ) <-- declname is the name of the current declaration


//--------------------------------------------------------------------------------------------
//MACROS
//--------------------------------------------------------------------------------------------

// Utility macro for Pixomatic vertex declaration
#undef PIXO_STREAM_SEMANTIC
#define PIXO_STREAM_SEMANTIC( pixosem )


#if defined( DXC_VERTEX_STREAMS )
	#undef DXC_VERTEX_STREAMS

	#define DXC_VERTEX_STRUCT
	#define DXC_VDECL_STREAM0
	#undef __DXC_VERTEX_DECLARATIONS__
	#include "dxC_VertexDeclarations.h"

	#define DXC_VERTEX_STRUCT
	#define DXC_VDECL_STREAM1
	#undef __DXC_VERTEX_DECLARATIONS__
	#include "dxC_VertexDeclarations.h"

	#define DXC_VERTEX_STRUCT
	#define DXC_VDECL_STREAM2
	#undef __DXC_VERTEX_DECLARATIONS__
	#include "dxC_VertexDeclarations.h"

	#define DXC_VERTEX_STRUCT
	#define DXC_VDECL_STREAM3
	#undef __DXC_VERTEX_DECLARATIONS__
	#include "dxC_VertexDeclarations.h"

	#define VERTEX_DECL_START( name )
	#define STREAM_START( stream )
	#define STREAM_SLOT_DECLARATION( stream, type, name, semantic, index, pixosem, declname )
	#define SLOT_DECLARATION( type, name, semantic, index, pixosem, declname )
	#define VERTEX_DECL_END( name, streamcount )
	#define DECLARE_FORMAT( fxtype, dx9type, dx10type, structtype )

#elif defined( DXC_VERTEX_STRUCT )
	#define STREAM0_FIELD( type, name )
	#define STREAM1_FIELD( type, name )
	#define STREAM2_FIELD( type, name )
	#define STREAM3_FIELD( type, name )

	#if defined( DXC_VDECL_STREAM0 )
		#undef STREAM0_FIELD
		#define STREAM0_FIELD( type, name ) type name;
		#define VERTEX_DECL_START( name ) struct name##_STREAM_0 {
	#elif defined( DXC_VDECL_STREAM1 )
		#undef STREAM1_FIELD
		#define STREAM1_FIELD( type, name ) type name;
		#define VERTEX_DECL_START( name ) struct name##_STREAM_1 {
	#elif defined( DXC_VDECL_STREAM2 )
		#undef STREAM2_FIELD
		#define STREAM2_FIELD( type, name ) type name;
		#define VERTEX_DECL_START( name ) struct name##_STREAM_2 {
	#elif defined( DXC_VDECL_STREAM3 )
		#undef STREAM3_FIELD
		#define STREAM3_FIELD( type, name ) type name;
		#define VERTEX_DECL_START( name ) struct name##_STREAM_3 {
	#endif

	#define STREAM_START( stream )
	#define STREAM_SLOT_DECLARATION( stream, type, name, semantic, index, pixosem, declname) STREAM##stream##_FIELD( type, name )
	#define SLOT_DECLARATION( type, name, semantic, index, pixosem, declname ) STREAM_SLOT_DECLARATION( 0, type, name, semantic, index, pixosem, declname )
	#define VERTEX_DECL_END( name, streamcount ) }; 
	#define DECLARE_FORMAT( fxtype, dx9type, dx10type, structtype ) structtype
	#define DECLARE_FORMAT_EX( fx9type, fx10type, dx9type, dx10type, structtype ) structtype

#elif defined( VERTEX_DECL_FX )  //In this mode we treat each declaration as an FX structure 
	#define VERTEX_DECL_START( name ) struct name {
	#define STREAM_START( stream )
	#define STREAM_SLOT_DECLARATION( stream, type, name, semantic, index, pixosem, declname ) type name : semantic##index;
	#define SLOT_DECLARATION( type, name, semantic, index, pixosem, declname ) STREAM_SLOT_DECLARATION( 0, type, name, semantic, index, pixosem, declname )
	#define VERTEX_DECL_END( name, streamcount ) };

	#define DECLARE_FORMAT( fxtype, dx9type, dx10type, structtype ) fxtype
	#ifdef ENGINE_TARGET_DX10
		#define DECLARE_FORMAT_EX( fx9type, fx10type, dx9type, dx10type, structtype ) fx10type
	#else
		#define DECLARE_FORMAT_EX( fx9type, fx10type, dx9type, dx10type, structtype ) fx9type
	#endif

#elif defined( DXC_VERTEX_PIXO_DECL ) //In this mode we treat each declaration as a Pixomatic vertex declaration
	#ifdef USE_PIXO
		#define VERTEX_DECL_START( name ) const unsigned int PIXO_##name[] = {
		#undef STREAM_START
		#define STREAM_START( stream ) PIXO_STREAM_SETSTREAM(stream),
		// Utility macro for Pixomatic
		#undef PIXO_STREAM_SEMANTIC
		#define PIXO_STREAM_SEMANTIC( pixosem )	PIXO_STREAM_##pixosem,
		#define STREAM_SLOT_DECLARATION( stream, type, name, semantic, index, pixosem, declname ) PIXO_STREAM_SEMANTIC(pixosem)
		#define SLOT_DECLARATION( type, name, semantic, index, pixosem, declname ) STREAM_SLOT_DECLARATION( 0, type, name, semantic, index, pixosem, declname )
		#define VERTEX_DECL_END( name, streamcount )	PIXO_STREAM_END };	enum PIXO_##name##_INFO { PIXO_##name##_COUNT = sizeof( PIXO_##name ) / sizeof( unsigned int ), PIXO_##name##_STREAM_COUNT = streamcount };
		#define DECLARE_FORMAT( fxtype, dx9type, dx10type, structtype )
	#else // not USE_PIXO
		#define VERTEX_DECL_START( name ) const unsigned int* PIXO_##name = NULL;
		#define STREAM_START( stream )
		#define STREAM_SLOT_DECLARATION( stream, type, name, semantic, index, pixosem, declname )
		#define SLOT_DECLARATION( type, name, semantic, index, pixosem, declname )
		#define VERTEX_DECL_END( name, streamcount )
		#define DECLARE_FORMAT( fxtype, dx9type, dx10type, structtype )
	#endif // USE_PIXO

#elif defined( ENGINE_TARGET_DX9 )  //In this mode we treat each declaration as a dx9 vertex declaration
	#define DXC_VERTEX_STREAMS
	#undef __DXC_VERTEX_DECLARATIONS__
	#include "dxC_VertexDeclarations.h"

	#define VERTEX_DECL_START( name ) const D3DC_INPUT_ELEMENT_DESC name[] = {
	#define STREAM_START( stream )
	#define STREAM_SLOT_DECLARATION( stream, type, name, semantic, index, pixosem, declname ) { stream, OFFSET( declname##_STREAM_##stream##, name), type, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_##semantic, index },
	#define SLOT_DECLARATION( type, name, semantic, index, pixosem, declname ) STREAM_SLOT_DECLARATION( 0, type, name, semantic, index, pixosem, declname )
	#define VERTEX_DECL_END( name, streamcount ) D3DDECL_END(), }; enum name##_INFO { name##_COUNT = sizeof( name ) / sizeof( D3DC_INPUT_ELEMENT_DESC ), name##_STREAM_COUNT = streamcount };
	#define DECLARE_FORMAT( fxtype, dx9type, dx10type, structtype ) dx9type
	#define DECLARE_FORMAT_EX( fx9type, fx10type, dx9type, dx10type, structtype ) dx9type

#elif defined( ENGINE_TARGET_DX10 ) //In this mode we treat each declaration as a dx10 vertex declaration
	#define DXC_VERTEX_STREAMS
	#undef __DXC_VERTEX_DECLARATIONS__
	#include "dxC_VertexDeclarations.h"

	#define VERTEX_DECL_START( name ) const D3DC_INPUT_ELEMENT_DESC name[] = {
	#define STREAM_START( stream )
	#define STREAM_SLOT_DECLARATION( stream, type, name, semantic, index, pixosem, declname ) { #semantic, index, type, stream, OFFSET( declname##_STREAM_##stream##, name ), D3D10_INPUT_PER_VERTEX_DATA, 0 },
	#define SLOT_DECLARATION( type, name, semantic, index, pixosem, declname ) STREAM_SLOT_DECLARATION( 0, type, name, semantic, index, pixosem, declname ) 
	#define VERTEX_DECL_END( name, streamcount ) }; enum name##_INFO { name##_COUNT = sizeof( name ) / sizeof( D3DC_INPUT_ELEMENT_DESC ), name##_STREAM_COUNT = streamcount };
	#define DECLARE_FORMAT( fxtype, dx9type, dx10type, structtype ) dx10type
	#define DECLARE_FORMAT_EX( fx9type, fx10type, dx9type, dx10type, structtype ) dx10type

#else
	#define VERTEX_DECL_START( name )
	#define STREAM_START( stream )
	#define STREAM_SLOT_DECLARATION( stream, type, name, semantic, index, pixosem, declname )
	#define SLOT_DECLARATION( type, name, semantic, index, pixosem, declname )
	#define VERTEX_DECL_END( name, streamcount )
	#define DECLARE_FORMAT( fxtype, dx9type, dx10type, structtype )

#endif
	
//--------------------------------------------------------------------------------------------
// FORMATS
//--------------------------------------------------------------------------------------------

#ifndef VERTEX_DECL_FX
	#ifndef __DXC_VERTEX_DECLARATIONS_TYPES__
	#define __DXC_VERTEX_DECLARATIONS_TYPES__

	union DXC_BYTE4
	{
		BYTE data[4];

		inline BYTE& operator[] (int nElement ) 
		{ 
			return data[ nElement ]; 
		}

		inline const BYTE& operator[] (int nElement ) const
		{ 
			return data[ nElement ]; 
		}
	};

	union DXC_SHORT2
	{
		short data[2];
		
		inline short& operator[] (int nElement ) 
		{ 
			return data[ nElement ]; 
		}

		inline const short& operator[] (int nElement ) const
		{ 
			return data[ nElement ]; 
		}
	};
	#endif //__DXC_VERTEX_DECLARATIONS_TYPES__
#endif //#ifndef VERTEX_DECL_FX

// DECLARE_FORMAT( fxtype, dx9type, dx10type, structtype, count )
//  Here we define formats for use in our declarations, make sure to fill in all formats 
//  including the struct type and element count (which is used for offset computation)

#define FLOAT4_32 DECLARE_FORMAT(	float4, D3DDECLTYPE_FLOAT4,		DXGI_FORMAT_R32G32B32A32_FLOAT,		D3DXVECTOR4 )
#define FLOAT3_32 DECLARE_FORMAT(	float3, D3DDECLTYPE_FLOAT3,		DXGI_FORMAT_R32G32B32_FLOAT,		D3DXVECTOR3 )
#define FLOAT2_32 DECLARE_FORMAT(	float2, D3DDECLTYPE_FLOAT2,		DXGI_FORMAT_R32G32_FLOAT,			D3DXVECTOR2 )
#define FLOAT1_32 DECLARE_FORMAT(	float,  D3DDECLTYPE_FLOAT1,		DXGI_FORMAT_R32_FLOAT,				float )

#define FLOAT4_8  DECLARE_FORMAT(	float4,	D3DDECLTYPE_D3DCOLOR,	DXGI_FORMAT_R8G8B8A8_UNORM,			DWORD )

#define UBYTE4	  DECLARE_FORMAT(	int4,	D3DDECLTYPE_UBYTE4,		DXGI_FORMAT_R8G8B8A8_UINT,			DXC_BYTE4 )

// FLOAT2_SINT is an odd format because in dx9 the interger data is automatically converted to float, this is not so in dx10
#define FLOAT2_16_SINT	DECLARE_FORMAT_EX( float2, int2, D3DDECLTYPE_SHORT2,	DXGI_FORMAT_R16G16_SINT,	DXC_SHORT2 )
//#define FLOAT2_16_UNORM DECLARE_FORMAT_EX( float2, int2, D3DDECLTYPE_USHORT2N, DXGI_FORMAT_R16G16_UNORM,	DXC_SHORT2 )

//Invalid can't be cross API beccause there is no single component short in dx9
//#define FLOAT1_UINT DECLARE_FORMAT_EX( float,  int,  D3DDECLTYPE_SHORT,  DXGI_FORMAT_R8_UINT,      short    ) 

//--------------------------------------------------------------------------------------------
//Vertex Declarations
//--------------------------------------------------------------------------------------------

#define VS_BACKGROUND_STREAM_0( name ) 	STREAM_START(0)		SLOT_DECLARATION( FLOAT3_32, Position, POSITION, 0,	POSXYZ, name )
#define VS_BACKGROUND_STREAM_1( name ) 	STREAM_START(1)		STREAM_SLOT_DECLARATION( 1, FLOAT4_32, LightMapDiffuse, TEXCOORD, 0, TEX0, name )		PIXO_STREAM_SEMANTIC(TEX1)

VERTEX_DECL_START( VS_BACKGROUND_INPUT_64 )
	VS_BACKGROUND_STREAM_0( VS_BACKGROUND_INPUT_64 )
	VS_BACKGROUND_STREAM_1( VS_BACKGROUND_INPUT_64 )
	STREAM_START(2)
	STREAM_SLOT_DECLARATION( 2, FLOAT4_8,		Normal,				NORMAL,		0,	NORMAL,		VS_BACKGROUND_INPUT_64 )
	STREAM_SLOT_DECLARATION( 2, FLOAT2_32,		DiffuseMap2,		TEXCOORD,	1,	SKIP(2),	VS_BACKGROUND_INPUT_64 )
	STREAM_SLOT_DECLARATION( 2, FLOAT3_32,		Tangent,			TANGENT,	0,	SKIP(3),	VS_BACKGROUND_INPUT_64 )
	STREAM_SLOT_DECLARATION( 2, FLOAT3_32,		Binormal,			BINORMAL,	0,	SKIP(3),	VS_BACKGROUND_INPUT_64 )
VERTEX_DECL_END( VS_BACKGROUND_INPUT_64, 3 )

VERTEX_DECL_START( VS_BACKGROUND_INPUT_32 )
	VS_BACKGROUND_STREAM_0( VS_BACKGROUND_INPUT_32 )	
	VS_BACKGROUND_STREAM_1( VS_BACKGROUND_INPUT_32 )	
	STREAM_START(2)
	STREAM_SLOT_DECLARATION( 2, FLOAT4_8,			Normal,				NORMAL,		0,	NORMAL,		VS_BACKGROUND_INPUT_32 )
VERTEX_DECL_END( VS_BACKGROUND_INPUT_32, 3 )

VERTEX_DECL_START( VS_BACKGROUND_INPUT_16 )
	VS_BACKGROUND_STREAM_0( VS_BACKGROUND_INPUT_16 )
	STREAM_START(1)
	STREAM_SLOT_DECLARATION( 1, FLOAT2_16_SINT,	TexCoord,			TEXCOORD,	0,	TEX0,		VS_BACKGROUND_INPUT_16 )
VERTEX_DECL_END( VS_BACKGROUND_INPUT_16, 2 )

VERTEX_DECL_START( VS_BACKGROUND_ZBUFFER_INPUT )
	VS_BACKGROUND_STREAM_0( VS_BACKGROUND_ZBUFFER_INPUT )
VERTEX_DECL_END( VS_BACKGROUND_ZBUFFER_INPUT, 1 )

VERTEX_DECL_START( VS_PARTICLE_INPUT )
	STREAM_START(0)
	SLOT_DECLARATION( FLOAT3_32,		Position,			POSITION,	0,	POSXYZ,		VS_PARTICLE_INPUT )
	SLOT_DECLARATION( FLOAT1_32,		Rand,				PSIZE,		0,	SKIP(1),	VS_PARTICLE_INPUT )
	SLOT_DECLARATION( FLOAT4_8,			Color,				COLOR,		0,	DIFFUSE,	VS_PARTICLE_INPUT )
	SLOT_DECLARATION( FLOAT4_8,			GlowConstant,		COLOR,		1,	SPECULAR,	VS_PARTICLE_INPUT )
	SLOT_DECLARATION( FLOAT2_32,		Tex,				TEXCOORD,	0,	TEX0,		VS_PARTICLE_INPUT )
VERTEX_DECL_END( VS_PARTICLE_INPUT, 1 )

VERTEX_DECL_START( VS_SIMPLE_INPUT )
	STREAM_START(0)
	SLOT_DECLARATION( FLOAT3_32,		Position,			POSITION,	0,	POSXYZ,		VS_SIMPLE_INPUT )
VERTEX_DECL_END( VS_SIMPLE_INPUT, 1 )

VERTEX_DECL_START( VS_GPU_PARTICLE_INPUT )
	STREAM_START(0)
    SLOT_DECLARATION(FLOAT3_32,			Position,           POSITION,   0,  POSXYZ,		VS_GPU_PARTICLE_INPUT )
	SLOT_DECLARATION(FLOAT3_32,			Velocity,           TEXCOORD,   0,  SKIP(3),	VS_GPU_PARTICLE_INPUT )
	SLOT_DECLARATION(FLOAT2_16_SINT,	Type,               TEXCOORD,   1,  SKIP(1),	VS_GPU_PARTICLE_INPUT )   
VERTEX_DECL_END( VS_GPU_PARTICLE_INPUT, 1 )

/* CML 2008.04.17 - Assumes Indices is always 4 bytes. */
#define VS_ANIMATED_STREAM_0( name, byte_type )	\
	STREAM_START(0) \
	SLOT_DECLARATION( FLOAT3_32,		Position,           POSITION,		0,  POSXYZ,		name ) \
	SLOT_DECLARATION( byte_type,		Indices,			BLENDINDICES,   0,  SKIP(1),	name ) \
	SLOT_DECLARATION( FLOAT3_32,		Weights,			BLENDWEIGHT,	0,  SKIP(3),	name )

#define VS_ANIMATED_STREAM_1( name ) \
	STREAM_START(1) \
	STREAM_SLOT_DECLARATION( 1, FLOAT2_32,			DiffuseMap,			TEXCOORD,		0,  TEX0,		name )

#define VS_ANIMATED_STREAM_2( name ) \
	STREAM_START(2) \
	STREAM_SLOT_DECLARATION( 2, FLOAT3_32,			Normal,				NORMAL,			0,  NORMAL,		name )\
	STREAM_SLOT_DECLARATION( 2, FLOAT3_32,			Tangent,			TANGENT,		0,  SKIP(3),	name )\
	STREAM_SLOT_DECLARATION( 2, FLOAT4_8,			Binormal,           BINORMAL,		0,  SKIP(1),	name )

VERTEX_DECL_START( VS_ANIMATED_INPUT )
	VS_ANIMATED_STREAM_0( VS_ANIMATED_INPUT, UBYTE4 )
	VS_ANIMATED_STREAM_1( VS_ANIMATED_INPUT )
	VS_ANIMATED_STREAM_2( VS_ANIMATED_INPUT )
VERTEX_DECL_END( VS_ANIMATED_INPUT, 3 )

VERTEX_DECL_START( VS_ANIMATED_POS_TEX_INPUT )
	VS_ANIMATED_STREAM_0( VS_ANIMATED_POS_TEX_INPUT, UBYTE4 )
	VS_ANIMATED_STREAM_1( VS_ANIMATED_POS_TEX_INPUT )
VERTEX_DECL_END( VS_ANIMATED_POS_TEX_INPUT, 2 )

VERTEX_DECL_START( VS_ANIMATED_ZBUFFER_INPUT )
	VS_ANIMATED_STREAM_0( VS_ANIMATED_ZBUFFER_INPUT, UBYTE4 )
VERTEX_DECL_END( VS_ANIMATED_ZBUFFER_INPUT, 1 )

// CHB 2006.12.27 - Important distinction: UBYTE4 is not available on all 1.1 hardware, so D3DCOLOR is used instead.
VERTEX_DECL_START( VS_ANIMATED_INPUT_11 )
	VS_ANIMATED_STREAM_0( VS_ANIMATED_INPUT_11, FLOAT4_8 )
	VS_ANIMATED_STREAM_1( VS_ANIMATED_INPUT_11 )
	VS_ANIMATED_STREAM_2( VS_ANIMATED_INPUT_11 )
VERTEX_DECL_END( VS_ANIMATED_INPUT_11, 3 )

// CHB 2006.12.29 - Important distinction: UBYTE4 is not available on all 1.1 hardware, so D3DCOLOR is used instead.
VERTEX_DECL_START( VS_ANIMATED_ZBUFFER_INPUT_11 )
	VS_ANIMATED_STREAM_0( VS_ANIMATED_ZBUFFER_INPUT_11, FLOAT4_8 )
VERTEX_DECL_END( VS_ANIMATED_ZBUFFER_INPUT_11, 1 )

VERTEX_DECL_START( VS_R3264_POS_TEX ) // to match with rigid 32/64
	STREAM_START(0)
	STREAM_SLOT_DECLARATION( 0, FLOAT3_32,			Position,           POSITION,		0,  POSXYZ,		VS_R3264_POS_TEX )
	STREAM_START(1)
	STREAM_SLOT_DECLARATION( 1, FLOAT4_32,			TexCoord,           TEXCOORD,		0,  TEX0,		VS_R3264_POS_TEX )		PIXO_STREAM_SEMANTIC(TEX1)
VERTEX_DECL_END( VS_R3264_POS_TEX, 2 )

VERTEX_DECL_START( VS_R16_POS_TEX ) // to match with rigid16
	STREAM_START(0)
	STREAM_SLOT_DECLARATION( 0, FLOAT3_32,			Position,           POSITION,		0,  POSXYZ,		VS_R16_POS_TEX )
	STREAM_START(1)
	STREAM_SLOT_DECLARATION( 1, FLOAT2_32,			TexCoord,           TEXCOORD,		0,  TEX0,		VS_R16_POS_TEX )
VERTEX_DECL_END( VS_R16_POS_TEX, 2 )

VERTEX_DECL_START( VS_POSITION_TEX )
	STREAM_START(0)
	STREAM_SLOT_DECLARATION( 0, FLOAT3_32,			Position,           POSITION,		0,  POSXYZ,		VS_POSITION_TEX )
	STREAM_SLOT_DECLARATION( 0, FLOAT2_32,			TexCoord,           TEXCOORD,		0,  TEX0,		VS_POSITION_TEX )
VERTEX_DECL_END( VS_POSITION_TEX, 1 )

VERTEX_DECL_START( VS_FULLSCREEN_QUAD )  //Very simple vertex format that assumes the vshader will generate texcoords based on 2D position
	STREAM_START(0)
	SLOT_DECLARATION(FLOAT2_32,			Position,           POSITION,		0,  POSSCREENXY,	VS_FULLSCREEN_QUAD )
VERTEX_DECL_END( VS_FULLSCREEN_QUAD, 1 )

VERTEX_DECL_START( VS_PARTICLE_SIMULATION )
	STREAM_START(0)
	SLOT_DECLARATION(FLOAT4_32,			Position,           POSITION,		0,  POSXYZW,	VS_PARTICLE_SIMULATION )
	SLOT_DECLARATION(FLOAT4_32,			VelocityAndTimer,   TEXCOORD,		0,  SKIP(4),	VS_PARTICLE_SIMULATION )
	SLOT_DECLARATION(FLOAT2_32,			SeedDuration,		TEXCOORD,		1,  SKIP(2),	VS_PARTICLE_SIMULATION )
VERTEX_DECL_END( VS_PARTICLE_SIMULATION, 1 )

VERTEX_DECL_START( VS_XYZ_COL )
	STREAM_START(0)
	SLOT_DECLARATION( FLOAT3_32,		Position,			POSITION,		0,	POSXYZ,		VS_XYZ_COL )
	SLOT_DECLARATION( FLOAT4_8,			Color,				COLOR,			0,	DIFFUSE,	VS_XYZ_COL )
VERTEX_DECL_END( VS_XYZ_COL, 1 )

VERTEX_DECL_START( VS_XYZ_COL_UV )
	STREAM_START(0)
	SLOT_DECLARATION( FLOAT3_32,		Position,			POSITION,		0,	POSXYZ,		VS_XYZ_COL_UV )
	SLOT_DECLARATION( FLOAT4_8,			Color,				COLOR,			0,	DIFFUSE,	VS_XYZ_COL_UV )
	SLOT_DECLARATION( FLOAT2_32,		TexCoord,			TEXCOORD,		0,	TEX0,		VS_XYZ_COL_UV )
VERTEX_DECL_END( VS_XYZ_COL_UV, 1 )

VERTEX_DECL_START( VS_FLUID_SIMULATION_INPUT )
	STREAM_START(0)
    SLOT_DECLARATION(FLOAT2_32,         Position,           POSITION,       0, SKIP(2),		VS_FLUID_SIMULATION_INPUT )
	SLOT_DECLARATION(FLOAT3_32,			Tex0,   			TEXCOORD,		0, SKIP(3),		VS_FLUID_SIMULATION_INPUT )
	SLOT_DECLARATION(FLOAT4_32,			Tex1,   			TEXCOORD,		1, SKIP(4),		VS_FLUID_SIMULATION_INPUT )
	SLOT_DECLARATION(FLOAT4_32,			Tex2,   			TEXCOORD,		2, SKIP(4),		VS_FLUID_SIMULATION_INPUT )
	SLOT_DECLARATION(FLOAT4_32,			Tex3,   			TEXCOORD,		3, SKIP(4),		VS_FLUID_SIMULATION_INPUT )
	SLOT_DECLARATION(FLOAT3_32,			Tex7,   			TEXCOORD,		7, SKIP(3),		VS_FLUID_SIMULATION_INPUT )
VERTEX_DECL_END( VS_FLUID_SIMULATION_INPUT, 1 )

VERTEX_DECL_START( VS_FLUID_RENDERING_INPUT )
	STREAM_START(0)
    SLOT_DECLARATION(FLOAT4_32,         Position,           POSITION,       0, SKIP(4),		VS_FLUID_RENDERING_INPUT )
	SLOT_DECLARATION(FLOAT3_32,			Tex,    			TEXCOORD,		0, SKIP(3),		VS_FLUID_RENDERING_INPUT )
VERTEX_DECL_END( VS_FLUID_RENDERING_INPUT, 1 )


//--------------------------------------------------------------------------------------------
//UNDEFINES - Needed for the extra passes through the header
//--------------------------------------------------------------------------------------------
#if defined( DXC_VERTEX_STRUCT )
	#undef DXC_VERTEX_STRUCT

	#ifdef DXC_VDECL_STREAM0
		#undef DXC_VDECL_STREAM0
	#endif
	#ifdef DXC_VDECL_STREAM1
		#undef DXC_VDECL_STREAM1
	#endif
	#ifdef DXC_VDECL_STREAM2
		#undef DXC_VDECL_STREAM2
	#endif
	#ifdef DXC_VDECL_STREAM3
		#undef DXC_VDECL_STREAM3
	#endif

	#undef STREAM0_FIELD
	#undef STREAM1_FIELD
	#undef STREAM2_FIELD
	#undef STREAM3_FIELD
#endif

#undef VERTEX_DECL_START
#undef SLOT_DECLARATION
#undef STREAM_SLOT_DECLARATION
#undef VERTEX_DECL_END
#undef DECLARE_FORMAT
#undef DECLARE_FORMAT_EX

#endif //__DXC_VERTEX_DECLARATIONS__