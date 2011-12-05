//
// Particle Shader 
//
// Particles aren't doing anything fancy right now - just using some alpha

// transformations
#include "_common.fx"
#include "../../source/DxC/dxC_shaderlimits.h"	// CHB 2006.06.28 - for MAX_PARTICLE_MESHES_PER_BATCH
#include "StateBlocks.fxh"
#include "Dx9\\_ParticleMesh.fxh"

//--------------------------------------------------------------------------------------------
PS_BASIC_PARTICLE_INPUT VS_BASIC ( VS_PARTICLE_MESH_INPUT In )
{
    PS_BASIC_PARTICLE_INPUT Out = (PS_BASIC_PARTICLE_INPUT)0;

	Out.Position = sGetPosition(In);
    Out.Color	 = ParticleColors[ sGetColorIndex(In) ];
    Out.Tex		 = sGetDiffuseMapUV(In);
    
	FOGMACRO( Out.Position )
    
    Out.Position = mul( Out.Position, ViewProjection );
 
    return Out;
}

//--------------------------------------------------------------------------------------------
PS_BASIC_PARTICLE_INPUT VS_BASIC_ADD ( VS_PARTICLE_MESH_INPUT In )
{
    PS_BASIC_PARTICLE_INPUT Out = (PS_BASIC_PARTICLE_INPUT)0;

	Out.Position = sGetPosition(In);
    Out.Tex		 = sGetDiffuseMapUV(In);
    
	FOGMACRO_OUTPUT( half fFog, Out.Position )
    Out.Color	 = ParticleColors[ sGetColorIndex(In) ];
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

	Out.Position = sGetPosition(In);
    Out.Tex		 = sGetDiffuseMapUV(In);
    
	FOGMACRO_OUTPUT( half fFog, Out.Position )

	int nColorIndex = sGetColorIndex(In);
    Out.Color	 = ParticleColors[ nColorIndex ];
    Out.Glow.xyzw	= ParticleGlow[ nColorIndex ];
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

	Out.Position = sGetPosition(In);

	FOGMACRO_OUTPUT( half fFog, Out.Position )
    Out.Glow.xyzw = ParticleGlow[ sGetColorIndex(In) ] * fFog;   // fog the glow constant
    Out.Tex		 = sGetDiffuseMapUV(In);
    
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
	SHADER_VERSION_11
	int Additive = 0; 
	int Glow = 0; > 
{
    pass P0
    {
        // shaders
        COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_BASIC());
        COMPILE_SET_PIXEL_SHADER( _PS_1_1, PS_BASIC(true));
        DXC_BLEND_RGB_SRCALPHA_INVSRCALPHA_ADD;

        //TODO: Alpha test in the pixel shader
        //TODO: fog in the pixel shader

#ifndef ENGINE_TARGET_DX10
//		FogEnable		 = TRUE;
		AlphaTestEnable	 = TRUE;

		BlendOp			 = ADD;
		AlphaBlendEnable = TRUE;
		COLORWRITEENABLE = BLUE | GREEN | RED;
		SrcBlend         = SRCALPHA;
		DestBlend        = INVSRCALPHA;
#endif
    }  
}

//////////////////////////////////////////////////////////////////////////////////////////////
DECLARE_TECH TVertexAndPixelShader_b  < 
	SHADER_VERSION_11
	int Additive = 1; 
	int Glow = 0; > 
{
    pass P0
    {
        // shaders
        COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_BASIC_ADD());
        COMPILE_SET_PIXEL_SHADER( _PS_1_1, PS_BASIC(true));
        DXC_BLEND_RGB_SRCALPHA_ONE_ADD;

        //TODO: Alpha test in the pixel shader

#ifndef ENGINE_TARGET_DX10
		FogEnable		 = FALSE;
		AlphaTestEnable	 = TRUE;

		BlendOp			 = ADD;
		AlphaBlendEnable = TRUE;
		COLORWRITEENABLE = BLUE | GREEN | RED;
		SrcBlend         = SRCALPHA;
		DestBlend        = ONE;
#endif
    }  
}

//////////////////////////////////////////////////////////////////////////////////////////////
DECLARE_TECH TVertexAndPixelShader_c  < 
	SHADER_VERSION_11
	int Additive = 0; 
	int Glow = 1; > 
{
    pass P0
    {
        // shaders
        COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_BASIC());
        COMPILE_SET_PIXEL_SHADER( _PS_1_1, PS_BASIC(true));
        DXC_BLEND_RGBA_SRCALPHA_INVSRCALPHA_ADD;

        //TODO: Alpha test in the pixel shader
        //TODO: Fog in the pixel shader

#ifndef ENGINE_TARGET_DX10
//		FogEnable		 = TRUE;
		AlphaTestEnable	 = TRUE;

		BlendOp			 = ADD;
		AlphaBlendEnable = TRUE;
		COLORWRITEENABLE = BLUE | GREEN | RED | ALPHA;
		SrcBlend         = SRCALPHA;
		DestBlend        = INVSRCALPHA;
#endif
    }  
}

//////////////////////////////////////////////////////////////////////////////////////////////
DECLARE_TECH TVertexAndPixelShader_d  < 
	SHADER_VERSION_11
	int Additive = 1; 
	int Glow = 1; > 
{
    pass P0
    {
        // shaders
        COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_BASIC_ADD());
        COMPILE_SET_PIXEL_SHADER( _PS_1_1, PS_BASIC(true));
        DXC_BLEND_RGBA_SRCALPHA_ONE_ADD;

        //TODO: Alpha test in the pixel shader

#ifndef ENGINE_TARGET_DX10
		FogEnable		 = FALSE;
		AlphaTestEnable	 = TRUE;

		BlendOp			 = ADD;
		AlphaBlendEnable = TRUE;
		COLORWRITEENABLE = BLUE | GREEN | RED | ALPHA;
		SrcBlend         = SRCALPHA;
		DestBlend        = ONE;
#endif
    }  
}

//////////////////////////////////////////////////////////////////////////////////////////////
DECLARE_TECH TVertexAndPixelShader_e  < 
	SHADER_VERSION_11
	int Additive = 0; 
	int Glow = 1;
	int GlowConstant = 1; > 
{
    pass P0
    {
        // shaders
        COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_BASIC());
        COMPILE_SET_PIXEL_SHADER( _PS_1_1, PS_BASIC(true));
        DXC_BLEND_RGB_SRCALPHA_INVSRCALPHA_ADD;

#ifndef ENGINE_TARGET_DX10
//		FogEnable		 = TRUE;
		AlphaTestEnable	 = TRUE;

		BlendOp			 = ADD;
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
		COMPILE_SET_PIXEL_SHADER( _PS_1_1, PS_GLOWCONSTANT(false));
        DXC_BLEND_RGBA_ONE_ONE_ADD;

#ifndef ENGINE_TARGET_DX10
		FogEnable		 = FALSE;
		AlphaTestEnable	 = FALSE;

		BlendOp			 = ADD;
		AlphaBlendEnable = TRUE;
		COLORWRITEENABLE = BLUE | GREEN | RED | ALPHA;
		SrcBlend         = ONE;
		DestBlend        = ONE;
#endif
   }  
}

//////////////////////////////////////////////////////////////////////////////////////////////
DECLARE_TECH TVertexAndPixelShader_f  < 
	SHADER_VERSION_11
	int Additive = 1; 
	int Glow = 1;
	int GlowConstant = 1; > 
{
    pass P0
    {
        // shaders
        COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_GLOWCONSTANT_ADD());
        COMPILE_SET_PIXEL_SHADER( _PS_1_1, PS_GLOWCONSTANT_ADD());
        DXC_BLEND_RGBA_ONE_ONE_ADD;

#ifndef ENGINE_TARGET_DX10
		FogEnable		 = FALSE;
		AlphaTestEnable	 = FALSE;

		BlendOp			 = ADD;
		COLORWRITEENABLE = BLUE | GREEN | RED | ALPHA;
		AlphaBlendEnable = TRUE;
		SrcBlend         = ONE;
		DestBlend        = ONE;
#endif
    }  
}
