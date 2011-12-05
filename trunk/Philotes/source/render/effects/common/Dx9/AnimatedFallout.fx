//
// Animated Fallout Shader 
//
// the alpha falls off the more that the normal points away from the eye

// transformations
#include "_common.fx"
#include "StateBlocks.fxh"
#include "Dx9/_AnimatedShared20.fxh"

//////////////////////////////////////////////////////////////////////////////////////////////
// Shaders
//////////////////////////////////////////////////////////////////////////////////////////////
struct PS_FALLOUT_INPUT
{
    float4 Position		: POSITION;
    float2 DiffuseMap	: TEXCOORD0;
    float  Alpha		: TEXCOORD1;
    float  Fog			: FOG;
};

//Substituting in  VS_ANIMATED_INPUT VS_BACKGROUND_INPUT_32, should be fine
//struct VS_FALLOUT_INPUT
//{
//    float4 Position		: POSITION;
//    float3 Normal		: NORMAL;
//    float2 DiffuseMap	: TEXCOORD1;
//};

//--------------------------------------------------------------------------------------------
// 2.0
//--------------------------------------------------------------------------------------------
PS_FALLOUT_INPUT VS20_FALLOUT ( VS_ANIMATED_INPUT In )
{
    PS_FALLOUT_INPUT Out = (PS_FALLOUT_INPUT)0;
    
    float4 Position;
    float3 Normal;
	GetPositionAndNormal( Position, Normal, In );

    Out.Position  = mul( Position, WorldViewProjection );          // position (projected)
    Out.DiffuseMap = In.DiffuseMap;									  
    float3 EyeAngle = normalize( EyeInObject.xyz - Position.xyz );
	float Angle = dot( Normal, EyeAngle );
	Out.Alpha = FallOutThickness * (Angle * Angle);

	FOGMACRO( In.Position )

    return Out;
}

//--------------------------------------------------------------------------------------------
float4 PS_FALLOUT( PS_FALLOUT_INPUT In, uniform bool bAlphaTest ) : COLOR
{
	float4 DiffuseMap = tex2D( DiffuseMapSampler, In.DiffuseMap ) * gfSelfIlluminationPower;
	DiffuseMap.w *= In.Alpha;
    AlphaTest(bAlphaTest, DiffuseMap );
    return DiffuseMap;
}

//--------------------------------------------------------------------------------------------
// 1.1
//--------------------------------------------------------------------------------------------
PS_FALLOUT_INPUT VS11_FALLOUT ( VS_ANIMATED_INPUT In )
{
    PS_FALLOUT_INPUT Out = (PS_FALLOUT_INPUT)0;
    
    float4 Position;
    float3 Normal;
	GetPositionAndNormal( Position, Normal, In );
    Out.DiffuseMap = In.DiffuseMap;									  

    float3 EyeAngle = normalize( EyeInObject.xyz - In.Position.xyz );
	float Angle = dot( Normal, EyeAngle );
	Out.Alpha = FallOutThickness * (Angle * Angle);

	FOGMACRO( In.Position )

    return Out;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Techniques
//////////////////////////////////////////////////////////////////////////////////////////////
DECLARE_TECH TVertexAndPixelShader < SHADER_VERSION_22 int PointLights = 0; int StaticLights = 0; > 
{
    pass P0
    {
        COMPILE_SET_VERTEX_SHADER( vs_2_0, VS20_FALLOUT() );
        COMPILE_SET_PIXEL_SHADER( ps_2_0, PS_FALLOUT(true) );
        
        DXC_RASTERIZER_SOLID_BACK;
        DXC_BLEND_RGB_SRCALPHA_INVSRCALPHA_ADD;
        DXC_DEPTHSTENCIL_ZREAD;

#ifndef ENGINE_TARGET_DX10
		AlphaTestEnable	 = TRUE;

		AlphaBlendEnable = TRUE;
		COLORWRITEENABLE = BLUE | GREEN | RED;
		SrcBlend         = SRCALPHA;
		DestBlend        = INVSRCALPHA;
		ZWriteEnable	 = FALSE;
#endif
    }  
}

/*
//--------------------------------------------------------------------------------------------
technique TVertexAndPixelShader < int ShaderGroup = SHADER_GROUP_11; int PointLights = 0; int StaticLights = 0; > 
{
    pass P0
    {
        VertexShader = compile vs_1_1 VS11_FALLOUT();
        PixelShader  = compile _PS_1_1 PS_FALLOUT(true);

		AlphaBlendEnable = TRUE;
		SrcBlend         = SRCALPHA;
		DestBlend        = INVSRCALPHA;
		ZWriteEnable	 = FALSE;
		AlphaTestEnable	 = TRUE;
    }  
}

//--------------------------------------------------------------------------------------------
technique TVertexShaderOnly < int ShaderGroup = SHADER_GROUP_NPS; int PointLights = 0; int StaticLights = 0; > 
{
    pass P0
    {
        VertexShader = compile vs_1_1 VS11_FALLOUT();
        PixelShader  = NULL;
        
        ColorOp[0]   = SELECTARG1;
        ColorArg1[0] = TEXTURE;
        AlphaOp[0]   = SELECTARG1;
        AlphaArg1[0] = TEXTURE;

        ColorOp[1]   = DISABLE;
        AlphaOp[1]   = DISABLE;
 
		MipFilter[0] = LINEAR;
		MinFilter[0] = LINEAR;
		MagFilter[0] = LINEAR;

		AlphaBlendEnable = TRUE;
		SrcBlend         = SRCALPHA;
		DestBlend        = INVSRCALPHA;
		ZWriteEnable	 = FALSE;
		AlphaTestEnable	 = TRUE;
    }
}
*/
