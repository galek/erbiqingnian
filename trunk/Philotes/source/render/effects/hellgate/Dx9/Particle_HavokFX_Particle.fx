//
// Havok FX Particle Shader 
//

// transformations
#include "_common.fx"
#include "StateBlocks.fxh"

float4 gvScale;
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
struct VS_PARTICLE_INPUT_WP
{
    float4 Position		: POSITION;
    float2 Tex			: TEXCOORD0;
    float4 WorldPos		: TEXCOORD1;
};

struct PS_BASIC_PARTICLE_INPUT
{
    float4 Position		: POSITION;
    float2 Tex			: TEXCOORD0;
    float  Fog			: FOG;
};

//--------------------------------------------------------------------------------------------
PS_BASIC_PARTICLE_INPUT VS_BASIC ( VS_PARTICLE_INPUT_WP In )
{
    PS_BASIC_PARTICLE_INPUT Out = (PS_BASIC_PARTICLE_INPUT)0;

	float2 quadoffset = In.Position.xy;
	
	Out.Position = mul( float4(In.WorldPos.xyz, 1.0f), ViewProjection);
	Out.Position.xy += quadoffset * gvScale;
	
	Out.Tex = In.Tex;
	
	FOGMACRO( In.WorldPos )
    
    return Out;
}

//--------------------------------------------------------------------------------------------
float4 PS_BASIC ( PS_BASIC_PARTICLE_INPUT In, uniform bool bAlphaTest ) : COLOR
{
    float4 color = tex2D(DiffuseMapSampler,  In.Tex);
    AlphaTest(bAlphaTest, color );
    return color;
}

//////////////////////////////////////////////////////////////////////////////////////////////
DECLARE_TECH TVertexAndPixelShader  < 
	SHADER_VERSION_11 
	int Additive = 0; 
	int Glow = 0; > 
{
    pass P0
    {
        // shaders
        COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_BASIC());
        COMPILE_SET_PIXEL_SHADER ( _PS_1_1, PS_BASIC(true));
        DXC_RASTERIZER_SOLID_NONE;
        DXC_BLEND_RGB_SRCALPHA_INVSRCALPHA_ADD;
        DXC_DEPTHSTENCIL_ZREAD;
        
#ifndef ENGINE_TARGET_DX10
		FogEnable			= FALSE;
		AlphaTestEnable		= TRUE;

		CullMode			= NONE;
		AlphaBlendEnable 	= TRUE;
		COLORWRITEENABLE 	= BLUE | GREEN | RED;
		BlendOp			 	= ADD;
		SrcBlend         	= SRCALPHA;
		DestBlend        	= INVSRCALPHA;
		ZWriteEnable	 	= FALSE;
#endif
    }  
}
