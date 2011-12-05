//
// Particle Shader - Fade in for the Hell Gates
//

// transformations
#include "_common.fx"
#include "StateBlocks.fxh"

float4 ParticleFadeIn;
float4 ParticleTextureMatrix[ 4 ];
float ParticleBurnDelta = ( 15.0f / 255.0f );
 
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
struct PS_BASIC_PARTICLE_INPUT
{
    float4 Position		: POSITION;
    float4 Color		: COLOR0;
    float2 AlphaTex		: TEXCOORD0;
    float2 DiffuseTex0	: TEXCOORD1;
    float2 DiffuseTex1	: TEXCOORD2;
    float  Fog			: FOG;
};

//--------------------------------------------------------------------------------------------
PS_BASIC_PARTICLE_INPUT VS_BASIC ( VS_PARTICLE_INPUT In )
{
    PS_BASIC_PARTICLE_INPUT Out = (PS_BASIC_PARTICLE_INPUT)0;

    Out.Position = mul( float4( In.Position, 1.0f ), WorldViewProjection);          // position (projected)
    Out.Color	 = In.Color;									  
    Out.AlphaTex = In.Tex;
    // transform the texture coords - rotate and shift them
    float2x3 mTransform;
    mTransform[ 0 ] = ParticleTextureMatrix[ 0 ].xyz;
    mTransform[ 1 ] = ParticleTextureMatrix[ 1 ].xyz;
    Out.DiffuseTex0 = mul( mTransform, float3( In.Tex.x, In.Tex.y, 1.0f ) );

    mTransform[ 0 ] = ParticleTextureMatrix[ 2 ].xyz;
    mTransform[ 1 ] = ParticleTextureMatrix[ 3 ].xyz;
    Out.DiffuseTex1 = mul( mTransform, float3( In.Tex.x, In.Tex.y, 1.0f ) );
    
	FOGMACRO( In.Position )
    
    return Out;
}

//--------------------------------------------------------------------------------------------
float4 PS_BASIC ( PS_BASIC_PARTICLE_INPUT In, uniform bool bAlphaTest ) : COLOR
{
    float Alpha = tex2D(DiffuseMapSampler,  In.AlphaTex).w;
    float4 Diffuse = lerp( tex2D(DiffuseMapSampler, In.DiffuseTex0), tex2D(DiffuseMapSampler, In.DiffuseTex1), ParticleFadeIn.w);
    // map the alpha to a different space
	if ( Alpha > ParticleFadeIn.x )
    {
		if ( Alpha < ParticleBurnDelta + ParticleFadeIn.x )
			Diffuse = float4( In.Color.xyz, 1.0f );
		else
			Diffuse.w = ParticleFadeIn.y + ( Alpha - ParticleFadeIn.x ) * ParticleFadeIn.z;
    } else {
		Diffuse.w = 0;
    }
    AlphaTest(bAlphaTest, Diffuse);
    return Diffuse;
}


//////////////////////////////////////////////////////////////////////////////////////////////
// CHB 2006.06.16 - This was originally SHADER_GROUP_20, so using VS_2_0
// here to maintain current run-time behavior, although this differs from
// the actual compilation version level 'vs_1_1'.
DECLARE_TECH TVertexAndPixelShader  < 
	SHADER_VERSION_22
	int Additive = 0; 
	int Glow = 0; > 
{
    pass P0
    {
        // shaders
        COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_BASIC());
        COMPILE_SET_PIXEL_SHADER ( ps_2_0, PS_BASIC(true));
        DXC_RASTERIZER_SOLID_NONE;
        DXC_BLEND_RGBA_SRCALPHA_INVSRCALPHA_ADD;
        DXC_DEPTHSTENCIL_ZREAD;
        //TODO: Alpha test to be done in the pixel shader...

#ifndef ENGINE_TARGET_DX10
		FogEnable			= FALSE;
		AlphaTestEnable		= TRUE;

		CullMode			= NONE;
		AlphaBlendEnable 	= TRUE;
		COLORWRITEENABLE 	= BLUE | GREEN | RED | ALPHA;
		SrcBlend         	= SRCALPHA;
		DestBlend        	= INVSRCALPHA;
		ZWriteEnable	 	= FALSE;
#endif
    }  
}
