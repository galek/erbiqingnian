#ifndef _PARTICLE_MESH_FXH_
#define _PARTICLE_MESH_FXH_

float4 ParticleWorlds[ 3 * MAX_PARTICLE_MESHES_PER_BATCH ]; 
float4 ParticleColors[ MAX_PARTICLE_MESHES_PER_BATCH ];
float ParticleGlow[ MAX_PARTICLE_MESHES_PER_BATCH ];

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

#if defined(FXC_LEGACY_COMPILE)
typedef float4 UBYTE_TYPE;
#else
typedef int4 UBYTE_TYPE;
#endif

struct VS_PARTICLE_MESH_INPUT
{
	UBYTE_TYPE Position	: POSITION;
	UBYTE_TYPE Tex		: TEXCOORD0;
};

struct PS_BASIC_PARTICLE_INPUT
{
    float4 Position		: POSITION;
    float2 Tex			: TEXCOORD0;
    float4 Color		: COLOR0;
    float  Fog			: FOG;
};

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
float4 sGetPosition(VS_PARTICLE_MESH_INPUT In) 
{
	// this should be pre-multiplied by 3
#if defined(FXC_LEGACY_COMPILE)
	// CHB 2007.09.10 - D3DCOLORtoUBYTE4 un-swizzles the D3DCOLOR
	// data.  So when we ask for 'z' here, the compiler generates
	// code for 'x', which actually refers to 'z' in the source
	// data.
	int MatrixIndex = D3DCOLORtoUBYTE4(In.Tex).z;
#else
	int MatrixIndex = In.Tex.z;
#endif

	float3x4 mTransform;
	mTransform[0] = ParticleWorlds[ MatrixIndex + 0 ];
	mTransform[1] = ParticleWorlds[ MatrixIndex + 1 ];
	mTransform[2] = ParticleWorlds[ MatrixIndex + 2 ];

#if defined(FXC_LEGACY_COMPILE)
	// CHB 2007.09.11 - We need to swap x & z, since the D3DCOLOR
	// vertex declaration swizzles for ARGB.  Also, w is integer 1
	// in the source data, which comes out near 0 for D3DCOLOR, so
	// we fill in our own 1 here.
	float4 Position = float4(In.Position.z, In.Position.y, In.Position.x, 1.0f);
#else
	// CHB 2007.09.10 - Actually, non-1.1 could benefit from using D3DCOLOR instead of UBYTE here
	float4 Position = In.Position;
	Position.xyz *= (1.0f / 255.0f);
#endif
	Position.xyz = mul( mTransform, Position );
	Position.w = 1.0f;

	return Position;
}

float2 sGetDiffuseMapUV(VS_PARTICLE_MESH_INPUT In)
{
#if defined(FXC_LEGACY_COMPILE)
	// CHB 2007.09.11 - Again, since we're not using the
	// D3DCOLORtoUBYTE4 intrinsic here, we need to swizzle
	// (in this case, by swapping 'x' and 'z') manually.
	return In.Tex.zy;
#else
	return In.Tex.xy * (1.0f / 255.0f);
#endif
}

int sGetColorIndex(VS_PARTICLE_MESH_INPUT In)
{
#if defined(FXC_LEGACY_COMPILE)
	return D3DCOLORtoUBYTE4(In.Tex).w;
#else
	return In.Tex.w;
#endif
}

#endif // _PARTICLE_MESH_FXH_