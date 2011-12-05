//
// Particle Mesh Shader for use with Havok FX 
//

#include "_common.fx"
#include "StateBlocks.fxh"
float4 gBugLerp;
float4 Bones[18]; // 3 x 4 * 6
float4 ParticleGlobalColor;
float4 gvScale;
//LightDirectional
//LightDirectionalColor

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
struct VS_PARTICLE_MESH_INPUT
{
    float4 Position			: POSITION;
    float3 Normal			: NORMAL;
    float4 Tex				: TEXCOORD0;
    float4 TransformRow0	: TEXCOORD1;
    float4 TransformRow1	: TEXCOORD2;
    float4 TransformRow2	: TEXCOORD3;
    float  Seed				: TEXCOORD4;
};

struct PS_BASIC_PARTICLE_INPUT
{
    float4 Position			: POSITION;
    float2 Tex				: TEXCOORD0;
    float3 EnvMapCoords		: TEXCOORD1;
    float4 Normal			: TEXCOORD2;
    float  Fog				: FOG;
};

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//--------------------------------------------------------------------------------------------
PS_BASIC_PARTICLE_INPUT VS_BASIC ( VS_PARTICLE_MESH_INPUT In )
{
    PS_BASIC_PARTICLE_INPUT Out = (PS_BASIC_PARTICLE_INPUT)0;

	float4 Position = In.Position;
	half3 Normal = In.Normal;

	if ( In.Tex.z >= 2 )
	{
		float SeededLerp = cos(gBugLerp.x + In.Seed) * 0.5f + 0.5f;

		float3x4 Bone0;
		Bone0[0] = Bones[ 0 ];
		Bone0[1] = Bones[ 1 ];
		Bone0[2] = Bones[ 2 ];
		
		float3x4 Bone1;
		Bone1[0] = Bones[ 3 ];
		Bone1[1] = Bones[ 4 ];
		Bone1[2] = Bones[ 5 ];
		Position.xyz = lerp( mul(Bone0, Position), mul(Bone1, Position), SeededLerp );
		Normal.xyz = lerp( mul((half3x3)Bone0, Normal), mul((half3x3)Bone1, Normal), SeededLerp );
	}
		
 	float3x4 mTransform;
	mTransform[0] = In.TransformRow0;
	mTransform[1] = In.TransformRow1;
	mTransform[2] = In.TransformRow2;
	
	if ( gBugLerp.y < 0 ) 
	{
		mTransform[ 1 ].xyz = -mTransform[ 1 ].xyz;
	}

	Position = gvScale * Position;
	Out.Position.xyz = mul( mTransform, Position );
	Out.Position.w = 1.0f;
	Out.Position.z += 0.1f;
	
	Out.Normal.xyz = normalize( mul( (half3x3)mTransform, Normal ).xyz );

	half3 vViewVector		= normalize( EyeInObject.xyz - Out.Position.xyz );
	half3 vLightVector		= DirLightsDir(OBJECT_SPACE)[ DIR_LIGHT_0_SPECULAR ].xyz;
	half3 vHalfVector		= normalize( vViewVector + vLightVector );
	half fPointSpecular		= GetSpecular20PowNoNormalize( vHalfVector, Out.Normal.xyz, 8.0f );
	Out.Normal.w = fPointSpecular;

	Out.Tex.xy	 = In.Tex.xy / 255.0f;
	
	FOGMACRO( Out.Position )
    
    Out.Position = mul( Out.Position, ViewProjection );
 
    return Out;
}

//--------------------------------------------------------------------------------------------
float4 PS_BASIC ( 
	PS_BASIC_PARTICLE_INPUT In,
	float  Face			: vFace, uniform bool bAlphaTest ) : COLOR
{
	half3 vNormal = normalize( half3( In.Normal.xyz ) );
	if ( Face < 0 )
	{
		vNormal = -vNormal;
	}
	
	half4 LightColor = LightAmbient;
	for ( int i = 0; i < 2; i++ )
	{
		half Diffuse = PSGetDiffuseBrightness( DirLightsDir(OBJECT_SPACE)[ i ].xyz, vNormal );
		LightColor += Diffuse * DirLightsColor[ i ];
	}

	LightColor.xyz += In.Normal.w * DirLightsColor[ DIR_LIGHT_0_SPECULAR ].xyz;

	LightColor.w = 1.0f;
	
	float4 color = tex2D(DiffuseMapSampler,  In.Tex) * LightColor * ParticleGlobalColor;
    AlphaTest(bAlphaTest, color );
    return color;
}

//--------------------------------------------------------------------------------------------
float4 PS_TEST ( PS_BASIC_PARTICLE_INPUT In ) : COLOR
{
	return float4(1.0f, 0, 0, 1);
}

//////////////////////////////////////////////////////////////////////////////////////////////
// we really aren't working on supporting this type of shader yet...
DECLARE_TECH TVertexShaderOnly  < 
	SHADER_VERSION_33
	int Additive = 0; 
	int Glow = 0; > 
{
    pass P0
    {
        COMPILE_SET_VERTEX_SHADER( vs_3_0, VS_BASIC());
        COMPILE_SET_PIXEL_SHADER ( ps_3_0, PS_BASIC(true));
        DXC_BLEND_RGB_SRCALPHA_INVSRCALPHA_ADD;
        //Alpha Test to do in the shader

#ifndef ENGINE_TARGET_DX10
		AlphaTestEnable	 = TRUE;

		COLORWRITEENABLE = BLUE | GREEN | RED;
		AlphaBlendEnable = TRUE;
		SrcBlend         = SRCALPHA;
		DestBlend        = INVSRCALPHA;
#endif
    }
}
