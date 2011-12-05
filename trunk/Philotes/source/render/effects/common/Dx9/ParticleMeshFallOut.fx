//
// Particle Shader 
//

// transformations
#include "_common.fx"
#include "../../source/DxC/dxC_shaderlimits.h"	// CHB 2006.06.28 - for MAX_PARTICLE_MESHES_PER_BATCH
#include "StateBlocks.fxh"

float4 ParticleWorlds[ 3 * MAX_PARTICLE_MESHES_PER_BATCH ]; 
float4 ParticleColors[ MAX_PARTICLE_MESHES_PER_BATCH ];
float ParticleGlow[ MAX_PARTICLE_MESHES_PER_BATCH ];

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
struct VS_PARTICLE_MESH_INPUT
{
    float4 Position		: POSITION;
    float4 Tex			: TEXCOORD0;
    float4 Normal		: NORMAL;
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
	float4 PositionIn,
	float MatrixIndex ) // this should be pre-multiplied by 3
{
	float3x4 mTransform;
	mTransform[0] = ParticleWorlds[ MatrixIndex + 0 ];
	mTransform[1] = ParticleWorlds[ MatrixIndex + 1 ];
	mTransform[2] = ParticleWorlds[ MatrixIndex + 2 ];

    float4 Position;
    PositionIn.xyz /= 255.0f;
    Position.xyz = mul( mTransform, PositionIn );
    Position.w = 1.0f;
    
    return Position;
}

//--------------------------------------------------------------------------------------------
float GetFallOut ( VS_PARTICLE_MESH_INPUT In, float4 Position )
{
	float3x4 mTransform;
	mTransform[0] = ParticleWorlds[ In.Tex.z + 0 ];
	mTransform[1] = ParticleWorlds[ In.Tex.z + 1 ];
	mTransform[2] = ParticleWorlds[ In.Tex.z + 2 ];

	float3 EyeAngle = normalize( EyeInObject.xyz - Position.xyz );
	half3 vInNormal = UNBIAS( In.Normal );
	half3 vNormalInWorld = mul( mTransform, float4( vInNormal, 0.0f ) );
	float Angle = dot( normalize(vNormalInWorld), EyeAngle );
	return FallOutThickness * (Angle * Angle);
}

//--------------------------------------------------------------------------------------------
PS_BASIC_PARTICLE_INPUT VS_BASIC ( VS_PARTICLE_MESH_INPUT In )
{
    PS_BASIC_PARTICLE_INPUT Out = (PS_BASIC_PARTICLE_INPUT)0;

	Out.Position = sGetPosition( In.Position, In.Tex.z );
    Out.Color	 = ParticleColors[ In.Tex.w ];
    Out.Tex.xy	 = In.Tex.xy / 255.0f;
	Out.Color.w  *= GetFallOut ( In, Out.Position );
   
	FOGMACRO( Out.Position )
    
    Out.Position = mul( Out.Position, ViewProjection );
 
    return Out;
}

//--------------------------------------------------------------------------------------------
PS_BASIC_PARTICLE_INPUT VS_BASIC_ADD ( VS_PARTICLE_MESH_INPUT In )
{
    PS_BASIC_PARTICLE_INPUT Out = (PS_BASIC_PARTICLE_INPUT)0;

	Out.Position = sGetPosition( In.Position, In.Tex.z );
    Out.Tex		 = In.Tex.xy / 255.0f;
    
	FOGMACRO_OUTPUT( half fFog, Out.Position )
    Out.Color	 = ParticleColors[ In.Tex.w ];
	if ( FogAdditiveParticleLum )
		Out.Color	*= fFog;
	Out.Fog = 0.0f;

	Out.Color.w  *= GetFallOut ( In, Out.Position );
	
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

	Out.Position = sGetPosition( In.Position, In.Tex.z );
    Out.Tex		 = In.Tex / 255.0f;
    
	FOGMACRO_OUTPUT( half fFog, Out.Position )

    Out.Color	 = ParticleColors[ In.Tex.w ];
    Out.Glow.xyzw	= ParticleGlow[ In.Tex.w ];
	if ( FogAdditiveParticleLum )
	{
		Out.Color	*= fFog;
		Out.Glow	*= fFog;  // fog the clow constant
	}
	Out.Fog = 0.0f;

	Out.Color.w  *= GetFallOut ( In, Out.Position );

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

	Out.Position = sGetPosition( In.Position, In.Tex.z );

	FOGMACRO_OUTPUT( half fFog, Out.Position )
    Out.Glow.xyzw = ParticleGlow[ In.Tex.w ] * fFog;   // fog the glow constant
    Out.Tex		  = In.Tex / 255.0f;
    
	Out.Glow.w  *= GetFallOut ( In, Out.Position );

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
        COMPILE_SET_PIXEL_SHADER ( _PS_1_1, PS_BASIC(true));
        DXC_BLEND_RGB_SRCALPHA_INVSRCALPHA_ADD;

        //Fog + Alpha test to do in pixel shader

#ifndef ENGINE_TARGET_DX10
//		FogEnable		 = TRUE;
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
	SHADER_VERSION_11
	int Additive = 1; 
	int Glow = 0; > 
{
    pass P0
    {
        // shaders
        COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_BASIC_ADD());
        COMPILE_SET_PIXEL_SHADER ( _PS_1_1, PS_BASIC(true));
        DXC_BLEND_RGB_SRCALPHA_ONE_ADD;

        //Alpha test to do in pixel shader

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
	SHADER_VERSION_11
	int Additive = 0; 
	int Glow = 1; > 
{
    pass P0
    {
        // shaders
        COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_BASIC());
        COMPILE_SET_PIXEL_SHADER ( _PS_1_1, PS_BASIC(true));
        DXC_BLEND_RGBA_SRCALPHA_INVSRCALPHA_ADD;

        //Fog + Alpha test to do in pixel shader

#ifndef ENGINE_TARGET_DX10
//		FogEnable		 = TRUE;
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
	SHADER_VERSION_11
	int Additive = 1; 
	int Glow = 1; > 
{
    pass P0
    {
        // shaders
        COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_BASIC_ADD());
        COMPILE_SET_PIXEL_SHADER ( _PS_1_1, PS_BASIC(true));
        DXC_BLEND_RGBA_SRCALPHA_ONE_ADD;

        //Alpha test to do in pixel shader

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
	SHADER_VERSION_11
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

        //Fog + Alpha test to do in pixel shader

#ifndef ENGINE_TARGET_DX10
//		FogEnable		 = TRUE;
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
	SHADER_VERSION_11
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
technique TVertexShaderOnly  < 
	int VSVersion = VS_1_1;
	int PSVersion = PS_NONE;
	int Additive = 0; 
	int Glow = 0; > 
{
    pass P0
    {
        // shaders
        VertexShader = compile vs_1_1 VS_BASIC ();
        PixelShader  = NULL;
        
        // texture stages
        ColorOp[0]   = MODULATE;
        ColorArg1[0] = TEXTURE;
        ColorArg2[0] = DIFFUSE;
        AlphaOp[0]   = MODULATE;
        AlphaArg1[0] = TEXTURE;
        AlphaArg2[0] = DIFFUSE;

        ColorOp[1]   = DISABLE;
        AlphaOp[1]   = DISABLE;
 
		MipFilter[0] = LINEAR;
		MinFilter[0] = LINEAR;
		MagFilter[0] = LINEAR;

		AlphaBlendEnable = TRUE;
		SrcBlend         = SRCALPHA;
		DestBlend        = INVSRCALPHA;
		AlphaTestEnable	 = TRUE;
		FogEnable		 = FALSE;
    }
}
