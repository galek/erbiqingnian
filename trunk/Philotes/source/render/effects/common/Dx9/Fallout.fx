//
// Fallout Shader 
//
// the alpha falls off the more that the normal points away from the eye

// transformations
#include "_common.fx"
#include "Dx9/_ScrollUV.fx"
#include "StateBlocks.fxh"

//////////////////////////////////////////////////////////////////////////////////////////////
// Shaders
//////////////////////////////////////////////////////////////////////////////////////////////

/*
	With PS 1.1, it seems TEXCOORD* slots can be used for either
	texture coordinates or colors.  If used for anything other than
	texture coordinates, the 'texcoord' instruction is employed to
	use the TEXCOORD register as a color.  The run-time balks at
	this if only one scalar component of the register is initialized
	(since it's apparently expecting 3):
	
	Direct3D9: VS->PS Linker: X137: (Error) Based on a pixel shader tex* instruction using texture coordinate set [1], the vertex shader is expected to output texcoord1.xyz. However the current vertex shader only outputs texcoord1.x. 

	So for the Alpha member below, we make it a float3, and in the
	vertex shader, duplicate the same value across all three
	components.  This doesn't add any instructions to the assembly.
*/

struct PS_FALLOUT_INPUT
{
	float4 Position			: POSITION;
	float2 DiffuseMapTex	: TEXCOORD0;
	float3 Alpha			: TEXCOORD1;
	float  Fog				: FOG;
};


//--------------------------------------------------------------------------------------------
// 1.1
//--------------------------------------------------------------------------------------------
PS_FALLOUT_INPUT VS_FALLOUT (
	VS_BACKGROUND_INPUT_32 In,
	uniform bool bScroll )
{
	PS_FALLOUT_INPUT Out = (PS_FALLOUT_INPUT)0;

	Out.Position  = mul( float4( In.Position, 1.0f ), WorldViewProjection);          // position (projected)
	float3 EyeAngle = normalize( EyeInObject.xyz - In.Position.xyz );
	half3 vInNormal = RIGID_NORMAL;
	float Angle = dot( vInNormal, EyeAngle );
	Out.Alpha.z = Out.Alpha.y = Out.Alpha.x = FallOutThickness * (Angle * Angle);

	if ( bScroll )
	{
		Out.DiffuseMapTex.xy = ScrollTextureCoordsHalf( In.LightMapDiffuse.zw, SCROLL_CHANNEL_DIFFUSE );
	}
	else
	{
		Out.DiffuseMapTex.xy = In.LightMapDiffuse.zw;
	}

	FOGMACRO( In.Position )

	return Out;
}

//--------------------------------------------------------------------------------------------
float4 PS_FALLOUT( PS_FALLOUT_INPUT In, uniform bool bAlphaTest ) : COLOR
{
	float4 DiffuseMap = tex2D( DiffuseMapSampler, In.DiffuseMapTex ) * gfSelfIlluminationPower;
	DiffuseMap.w *= In.Alpha.x;
    AlphaTest(bAlphaTest, DiffuseMap );
    return DiffuseMap;
}

//////////////////////////////////////////////////////////////////////////////////////////////
// Techniques
//////////////////////////////////////////////////////////////////////////////////////////////

#ifndef ENGINE_TARGET_DX10
#	define STATE							\
		DXC_BLEND_RGB_SRCALPHA_INVSRCALPHA_ADD; \
		DXC_DEPTHSTENCIL_ZREAD; \
		FogEnable		 = TRUE;			\
		AlphaTestEnable	 = TRUE;			\
		AlphaBlendEnable = TRUE;			\
		BlendOp			 = ADD;				\
		SrcBlend         = SRCALPHA;		\
		DestBlend        = INVSRCALPHA;		\
		ZWriteEnable	 = FALSE;			
#else
#	define STATE							\
		DXC_BLEND_RGB_SRCALPHA_INVSRCALPHA_ADD; \
		DXC_DEPTHSTENCIL_ZREAD;
#endif


#define TECHNIQUE( scroll ) \
	DECLARE_TECH TVertexAndPixelShader##scroll < \
	SHADER_VERSION_11 \
	int ScrollUV  = scroll; > \
{ \
	pass P0 \
	{ \
		COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_FALLOUT( scroll ) ); \
		COMPILE_SET_PIXEL_SHADER ( _PS_1_1, PS_FALLOUT(true)); \
		STATE \
	} \
}

TECHNIQUE(0)
TECHNIQUE(1)

/*

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

//*/