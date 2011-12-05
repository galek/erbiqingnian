//
// Particle Mesh Shader for use with Havok FX 
//

#include "_common.fx"
#include "StateBlocks.fxh"

float4 ParticleGlobalColor;
float4 ParticleGlobalGlow;
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
struct VS_PARTICLE_MESH_INPUT
{
    float4 Position			: POSITION;
    float4 Tex				: TEXCOORD0;
    float4 TransformRow0	: TEXCOORD1;
    float4 TransformRow1	: TEXCOORD2;
    float4 TransformRow2	: TEXCOORD3;
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
float4 sGetPosition(
	VS_PARTICLE_MESH_INPUT In ) 
{
	float3x4 mTransform;
	mTransform[0] = In.TransformRow0;
	mTransform[1] = In.TransformRow1;
	mTransform[2] = In.TransformRow2;

    float4 Position;
    Position.xyz = mul( mTransform, In.Position );
    Position.w = 1.0f;
    
    return Position;
}

//--------------------------------------------------------------------------------------------
PS_BASIC_PARTICLE_INPUT VS_BASIC ( VS_PARTICLE_MESH_INPUT In )
{
    PS_BASIC_PARTICLE_INPUT Out = (PS_BASIC_PARTICLE_INPUT)0;

	Out.Position = sGetPosition( In );
    Out.Color	 = ParticleGlobalColor;
    Out.Tex.xy	 = In.Tex.xy / 255.0f;
    
	FOGMACRO( Out.Position )
    
    Out.Position = mul( Out.Position, ViewProjection );
 
    return Out;
}

//--------------------------------------------------------------------------------------------
PS_BASIC_PARTICLE_INPUT VS_BASIC_ADD ( VS_PARTICLE_MESH_INPUT In )
{
    PS_BASIC_PARTICLE_INPUT Out = (PS_BASIC_PARTICLE_INPUT)0;

	Out.Position = sGetPosition( In );
    Out.Tex.xy	 = In.Tex.xy / 255.0f;
    
	FOGMACRO_OUTPUT( half fFog, Out.Position )
    Out.Color	 = ParticleGlobalColor;
	if ( FogAdditiveParticleLum )
		Out.Color	*= fFog;
	Out.Fog = 0.0f;

    Out.Position = mul( Out.Position, ViewProjection );

	// don't actually output and use fog
	//Out.Fog		 = fFog;
    
    return Out;
}

//--------------------------------------------------------------------------------------------
float4 PS_BASIC ( PS_BASIC_PARTICLE_INPUT In, uniform bool bAlphaTest ) : COLOR
{
    float4 color = tex2D(DiffuseMapSampler,  In.Tex) * In.Color;
    AlphaTest(bAlphaTest, color );
    return color;
}

//--------------------------------------------------------------------------------------------
float4 PS_TEST ( PS_BASIC_PARTICLE_INPUT In ) : COLOR
{
	return float4(1.0f, 0, 0, 1);
}

//////////////////////////////////////////////////////////////////////////////////////////////
struct PS_GLOWCONSTANT_ADD_PARTICLE_INPUT
{
    float4 Position		: POSITION;
    float4 Color		: COLOR0;
    float4 Glow			: COLOR1;
    float2 Tex			: TEXCOORD0;
    float  Fog			: FOG;
};

PS_GLOWCONSTANT_ADD_PARTICLE_INPUT VS_GLOWCONSTANT_ADD ( VS_PARTICLE_MESH_INPUT In )
{
    PS_GLOWCONSTANT_ADD_PARTICLE_INPUT Out = (PS_GLOWCONSTANT_ADD_PARTICLE_INPUT)0;

	Out.Position = sGetPosition( In );
    Out.Tex.xy	 = In.Tex.xy / 255.0f;
    
	FOGMACRO_OUTPUT( half fFog, Out.Position )

    Out.Color	 = ParticleGlobalColor;
    Out.Glow	 = ParticleGlobalGlow;
	if ( FogAdditiveParticleLum )
	{
		Out.Color	*= fFog;
		Out.Glow	*= fFog;  // fog the clow constant
	}
	Out.Fog = 0.0f;

    Out.Position = mul( Out.Position, ViewProjection );

    // don't actually output and use fog
	//Out.Fog		 = fFog;
    
    return Out;
}

//--------------------------------------------------------------------------------------------
float4 PS_GLOWCONSTANT_ADD ( PS_GLOWCONSTANT_ADD_PARTICLE_INPUT In ) : COLOR
{
    float4 Color = tex2D(DiffuseMapSampler,  In.Tex) * In.Color;
    Color.xyz *= Color.w;
    Color.w = In.Glow.w;
   
    return Color;
}

//////////////////////////////////////////////////////////////////////////////////////////////
struct PS_GLOWCONSTANT_PARTICLE_INPUT
{
    float4 Position		: POSITION;
    float4 Glow			: COLOR0;
    float2 Tex			: TEXCOORD0;
};

PS_GLOWCONSTANT_PARTICLE_INPUT VS_GLOWCONSTANT ( VS_PARTICLE_MESH_INPUT In )
{
    PS_GLOWCONSTANT_PARTICLE_INPUT Out = (PS_GLOWCONSTANT_PARTICLE_INPUT)0;

	Out.Position = sGetPosition( In );

	FOGMACRO_OUTPUT( half fFog, Out.Position )
    Out.Glow = ParticleGlobalGlow * fFog;   // fog the glow constant
    Out.Tex.xy	 = In.Tex.xy / 255.0f;
    
    Out.Position = mul( Out.Position, ViewProjection );

    return Out;
}

//--------------------------------------------------------------------------------------------
float4 PS_GLOWCONSTANT ( PS_GLOWCONSTANT_PARTICLE_INPUT In, uniform bool bAlphaTest ) : COLOR
{
   float4 color = float4(0,0,0,tex2D(DiffuseMapSampler,  In.Tex).w * In.Glow.w);
   AlphaTest(bAlphaTest, color );
   return color;
}

//////////////////////////////////////////////////////////////////////////////////////////////
DECLARE_TECH TVertexAndPixelShader_a  < 
	SHADER_VERSION_33
	int Additive = 0; 
	int Glow = 0; > 
{
    pass P0
    {
        // shaders
        COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_BASIC());
        COMPILE_SET_PIXEL_SHADER ( _PS_1_1, PS_BASIC(true));
        DXC_BLEND_RGB_SRCALPHA_INVSRCALPHA_ADD;
        //FOG + ALPHATEST todo in the pixel shader

#ifndef ENGINE_TARGET_DX10
		FogEnable		 = TRUE;
		AlphaTestEnable	 = TRUE;

		AlphaBlendEnable = TRUE;
		COLORWRITEENABLE = BLUE | GREEN | RED;
		SrcBlend         = SRCALPHA;
		DestBlend        = INVSRCALPHA;
#endif
    }  
}

//////////////////////////////////////////////////////////////////////////////////////////////
DECLARE_TECH TVertexAndPixelShader_b  < 
	SHADER_VERSION_33
	int Additive = 1; 
	int Glow = 0; > 
{
    pass P0
    {
        // shaders
        COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_BASIC_ADD());
        COMPILE_SET_PIXEL_SHADER ( _PS_1_1, PS_BASIC(true));
        DXC_BLEND_RGB_SRCALPHA_INVSRCALPHA_ADD;
        //ALPHATEST todo in the pixel shader

#ifndef ENGINE_TARGET_DX10
		FogEnable		 = FALSE;
		AlphaTestEnable	 = TRUE;

		AlphaBlendEnable = TRUE;
		COLORWRITEENABLE = BLUE | GREEN | RED;
		SrcBlend         = SRCALPHA;
		DestBlend        = ONE;
#endif
    }  
}

//////////////////////////////////////////////////////////////////////////////////////////////
DECLARE_TECH TVertexAndPixelShader_c  < 
	SHADER_VERSION_33
	int Additive = 0; 
	int Glow = 1; > 
{
    pass P0
    {
        // shaders
        COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_BASIC());
        COMPILE_SET_PIXEL_SHADER ( _PS_1_1, PS_BASIC(true));

		DXC_BLEND_RGBA_SRCALPHA_INVSRCALPHA_ADD;
        //FOG + ALPHATEST todo in the pixel shader

#ifndef ENGINE_TARGET_DX10
		FogEnable		 = TRUE;
		AlphaTestEnable	 = TRUE;

		AlphaBlendEnable = TRUE;
		COLORWRITEENABLE = BLUE | GREEN | RED | ALPHA;
		SrcBlend         = SRCALPHA;
		DestBlend        = INVSRCALPHA;
#endif
    }  
}

//////////////////////////////////////////////////////////////////////////////////////////////
DECLARE_TECH TVertexAndPixelShader_d  < 
	SHADER_VERSION_33
	int Additive = 1; 
	int Glow = 1; > 
{
    pass P0
    {
        // shaders
        COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_BASIC_ADD());
        COMPILE_SET_PIXEL_SHADER ( _PS_1_1, PS_BASIC(true));
		DXC_BLEND_RGB_SRCALPHA_ONE_ADD;
        //ALPHATEST todo in the pixel shader

#ifndef ENGINE_TARGET_DX10
		FogEnable		 = FALSE;
		AlphaTestEnable	 = TRUE;

		AlphaBlendEnable = TRUE;
		COLORWRITEENABLE = BLUE | GREEN | RED | ALPHA;
		SrcBlend         = SRCALPHA;
		DestBlend        = ONE;
#endif
    }  
}

//////////////////////////////////////////////////////////////////////////////////////////////
DECLARE_TECH TVertexAndPixelShader_e  < 
	SHADER_VERSION_33
	int Additive = 0; 
	int Glow = 1;
	int GlowConstant = 1; > 
{
    pass P0
    {
        // shaders
        COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_BASIC());
        COMPILE_SET_PIXEL_SHADER ( _PS_1_1, PS_BASIC(true));

		DXC_BLEND_RGB_SRCALPHA_INVSRCALPHA_ADD;
        //FOG + ALPHATEST todo in the pixel shader

#ifndef ENGINE_TARGET_DX10
		FogEnable		 = TRUE;
		AlphaTestEnable	 = TRUE;

		AlphaBlendEnable = TRUE;
		COLORWRITEENABLE = BLUE | GREEN | RED;
		SrcBlend         = SRCALPHA;
		DestBlend        = INVSRCALPHA;
#endif
    }  

    pass P1
    {
        // shaders
		COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_GLOWCONSTANT());
		COMPILE_SET_PIXEL_SHADER ( _PS_1_1, PS_GLOWCONSTANT(false));
		DXC_BLEND_RGBA_ONE_ONE_ADD;

#ifndef ENGINE_TARGET_DX10
		FogEnable		 = FALSE;
		AlphaTestEnable	 = FALSE;

		AlphaBlendEnable = TRUE;
		COLORWRITEENABLE = BLUE | GREEN | RED | ALPHA;
		SrcBlend         = ONE;
		DestBlend        = ONE;
#endif
   }  
}

//////////////////////////////////////////////////////////////////////////////////////////////
DECLARE_TECH TVertexAndPixelShader_f  < 
	SHADER_VERSION_33
	int Additive = 1; 
	int Glow = 1;
	int GlowConstant = 1; > 
{
    pass P0
    {
        // shaders
        COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_GLOWCONSTANT_ADD());
        COMPILE_SET_PIXEL_SHADER ( _PS_1_1, PS_GLOWCONSTANT_ADD());
		DXC_BLEND_RGBA_ONE_ONE_ADD;

#ifndef ENGINE_TARGET_DX10
		FogEnable		 = FALSE;
		AlphaTestEnable	 = FALSE;

		COLORWRITEENABLE = BLUE | GREEN | RED | ALPHA;
		AlphaBlendEnable = TRUE;
		SrcBlend         = ONE;
		DestBlend        = ONE;
#endif
    }  
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
        // shaders
        COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_BASIC());
        COMPILE_SET_PIXEL_SHADER ( _PS_1_1, PS_BASIC(true));
        DXC_BLEND_RGB_SRCALPHA_INVSRCALPHA_ADD;
        //ALPHATEST todo in the pixel shader
        // texture stages 

#ifndef ENGINE_TARGET_DX10
		AlphaTestEnable	 = TRUE;

		COLORWRITEENABLE = BLUE | GREEN | RED;
		AlphaBlendEnable = TRUE;
		SrcBlend         = SRCALPHA;
		DestBlend        = INVSRCALPHA;
#endif
    }
}
