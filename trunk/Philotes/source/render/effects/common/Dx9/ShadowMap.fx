//
// Shadow Map - renders geometry into the shadow map texture, recording depth
//
// CHB 2006.07.14 - This has been effectively copied-and-pasted into
// ShadowMap11.fx for use with a reduced number of bones. If it turns
// out regular maintenance is required, a better way of sharing code
// should be used.
//
#include "_common.fx"
#include "Dx9/_AnimatedShared20.fxh"
#include "../../source/Dx9/dx9_shadowtypeconstants.h"
#include "Dx9/_Shadow.fxh"


// transformations
//////////////////////////////////////////////////////////////////////////////////////////////

struct PS_SHADOWMAP_INPUT
{
    float4 Position  : POSITION;
	float3 TexCoords : TEXCOORD3;
	float2 Depth	 : TEXCOORD0;
};

PS_SHADOWMAP_INPUT VSShared( float3 pos, float2 tex )
{
	PS_SHADOWMAP_INPUT output = (PS_SHADOWMAP_INPUT)0.0f;
	
	//Compute worldPos, for VSM we need this to be separate
    float4 vWorldPos = mul( float4( pos, 1.0f ), World );
    
    // Compute the projected coordinates
    output.Position = mul( mul( vWorldPos,  View ), Projection );
    output.TexCoords.xy = tex;
    output.Depth.xy = output.Position.zw;

	return output;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// this format is used for rigid 16
PS_SHADOWMAP_INPUT VS_SHADOWMAP_16 ( VS_R16_POS_TEX In ) 
{
    PS_SHADOWMAP_INPUT Out = (PS_SHADOWMAP_INPUT)0;
    
    Out = VSShared( In.Position.xyz, In.TexCoord.xy );
    
    return Out;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
// this format is used for rigid 32/64 - diffuse tex in zw
PS_SHADOWMAP_INPUT VS_SHADOWMAP_32_64 ( VS_R3264_POS_TEX In ) 
{
    PS_SHADOWMAP_INPUT Out = (PS_SHADOWMAP_INPUT)0;
    
    Out = VSShared( In.Position.xyz, In.TexCoord.zw );
    
    return Out;
}

//--------------------------------------------------------------------------------------------
//--------------------------------------------------------------------------------------------
PS_SHADOWMAP_INPUT VS_ANIMATED_SHADOWMAP ( VS_ANIMATED_POS_TEX_INPUT In ) 
{
    PS_SHADOWMAP_INPUT Out = (PS_SHADOWMAP_INPUT)0;

	float4 Position;
#if 1
	// For some reason the player's katanas come on when the cheap function is 
	// used. Crates and some other "animated", stationary objects require this
	// function too. Probably not all of the bones are set properly.
	GetPositionCheap( Position, In ); 
#else
	GetPosition( Position, In );
#endif	
	
	Out = VSShared( Position.xyz, In.DiffuseMap.xy );

    return Out;
}

float4 PS_SHADOWMAP ( PS_SHADOWMAP_INPUT In, uniform bool bAlphaTest ) : COLOR
{
    float4 cColor = (float4)0.0f;
	cColor.a = tex2D( DiffuseMapSampler, In.TexCoords.xy ).a;    

    AlphaTest(bAlphaTest, cColor );
    
   	cColor.r = In.Depth.x / In.Depth.y;
	
    return cColor;
}

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

DECLARE_TECH Rigid16Shader < SHADER_VERSION_22 int Index = 0; int ShadowType = SHADOW_TYPE_DEPTH; > 
{
    pass P0
    {
        COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_SHADOWMAP_16() );
        COMPILE_SET_PIXEL_SHADER( _PS_1_1, PS_SHADOWMAP(true) );

#ifndef ENGINE_TARGET_DX10
		ZENABLE = TRUE;
		ZWRITEENABLE = TRUE;
		COLORWRITEENABLE	= 0;
		//COLORWRITEENABLE	= RED;
		ALPHATESTENABLE		= TRUE;
		ALPHABLENDENABLE	= FALSE;
        CULLMODE			= NONE;
#endif
    }  
}

DECLARE_TECH Rigid3264Shader < SHADER_VERSION_22 int Index = 1; int ShadowType = SHADOW_TYPE_DEPTH; > 
{
    pass P0
    {
        COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_SHADOWMAP_32_64() );
        COMPILE_SET_PIXEL_SHADER( _PS_1_1, PS_SHADOWMAP(true) );
		
#ifndef ENGINE_TARGET_DX10
		ZENABLE = TRUE;
		ZWRITEENABLE = TRUE;
		COLORWRITEENABLE	= 0;
		//COLORWRITEENABLE	= RED;
		ALPHATESTENABLE		= TRUE;
		ALPHABLENDENABLE	= FALSE;
        CULLMODE			= NONE;
#endif
    }  
}

//--------------------------------------------------------------------------------------------

DECLARE_TECH AnimatedShader < SHADER_VERSION_22 int Index = 2; int ShadowType = SHADOW_TYPE_DEPTH; > 
{
    pass P0
    {
        COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_ANIMATED_SHADOWMAP() );
        COMPILE_SET_PIXEL_SHADER( _PS_1_1, PS_SHADOWMAP(true) );
		
#ifndef ENGINE_TARGET_DX10
		ZENABLE = TRUE;
		ZWRITEENABLE = TRUE;
		COLORWRITEENABLE	= 0;        
		//COLORWRITEENABLE	= RED;
		ALPHATESTENABLE		= TRUE;
		ALPHABLENDENABLE	= FALSE;
        CULLMODE			= NONE;
#endif
    }  
}

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

struct PS_SHADOWMAP_COLOR_INPUT
{
    float4 Position : POSITION;
	float2 Depth	 : TEXCOORD0;
	float2 TexCoords : TEXCOORD1;
};

PS_SHADOWMAP_COLOR_INPUT VSShared_Color(float3 Pos, float2 tex)
{
	PS_SHADOWMAP_COLOR_INPUT Output;
	
	// Compute worldPos, for VSM we need this to be separate
    float4 vWorldPos = mul( float4( Pos, 1.0f ), World );
    
    // Compute the projected coordinates
    Output.Position = mul( mul( vWorldPos,  View ), Projection );
    Output.Depth.xy = Output.Position.zw;

    
    Output.TexCoords = tex;

	// That's it.    
	return Output;
}

// this format is used for rigid 16
PS_SHADOWMAP_COLOR_INPUT VS_SHADOWMAP_RIGID_16_COLOR ( VS_R16_POS_TEX In ) 
{
    return VSShared_Color( In.Position.xyz, In.TexCoord.xy );
}

// this format is used for rigid 32/64 - diffuse tex in zw
PS_SHADOWMAP_COLOR_INPUT VS_SHADOWMAP_RIGID_32_64_COLOR ( VS_R3264_POS_TEX In ) 
{
    return VSShared_Color( In.Position.xyz, In.TexCoord.zw );
}

PS_SHADOWMAP_COLOR_INPUT VS_SHADOWMAP_ANIMATED_COLOR ( VS_ANIMATED_POS_TEX_INPUT In ) 
{
	float4 Position;
	GetPositionCheap( Position, In );
    return VSShared_Color( Position.xyz, In.DiffuseMap.xy );
}

float4 PS_SHADOWMAP_COLOR ( PS_SHADOWMAP_COLOR_INPUT In ) : COLOR
{
	float4 cColor = (float4)0.0f;
	
	float a = tex2D( DiffuseMapSampler, In.TexCoords.xy ).a;	
	clip( a - GLOW_MIN );
	
   	cColor.r = In.Depth.x / In.Depth.y;
   	cColor.a = a;
	
    return cColor;
}

//--------------------------------------------------------------------------------------------

DECLARE_TECH Rigid16ColorShader < SHADER_VERSION_22 int Index = 0; int ShadowType = SHADOW_TYPE_COLOR; >
{
    pass P0
    {
        COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_SHADOWMAP_RIGID_16_COLOR() );
        COMPILE_SET_PIXEL_SHADER( ps_2_0, PS_SHADOWMAP_COLOR() );
		
#ifndef ENGINE_TARGET_DX10
		COLORWRITEENABLE	= RED;        
		ALPHATESTENABLE		= FALSE;
		ALPHABLENDENABLE	= FALSE;
#endif
    }  
}

//--------------------------------------------------------------------------------------------

DECLARE_TECH Rigid3264ColorShader < SHADER_VERSION_22 int Index = 1; int ShadowType = SHADOW_TYPE_COLOR; >
{
    pass P0
    {
        COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_SHADOWMAP_RIGID_32_64_COLOR() );
        COMPILE_SET_PIXEL_SHADER( ps_2_0, PS_SHADOWMAP_COLOR() );
		
#ifndef ENGINE_TARGET_DX10
		COLORWRITEENABLE	= RED;        
		ALPHATESTENABLE		= FALSE;
		ALPHABLENDENABLE	= FALSE;
#endif
    }  
}

//--------------------------------------------------------------------------------------------

DECLARE_TECH AnimatedColorShader < SHADER_VERSION_22 int Index = 2; int ShadowType = SHADOW_TYPE_COLOR; >
{
    pass P0
    {
        COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_SHADOWMAP_ANIMATED_COLOR() );
        COMPILE_SET_PIXEL_SHADER( ps_2_0, PS_SHADOWMAP_COLOR() );
		
#ifndef ENGINE_TARGET_DX10
		COLORWRITEENABLE	= RED;        
		ALPHATESTENABLE		= FALSE;
		ALPHABLENDENABLE	= FALSE;
#endif
    }  
}

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

struct PS_DEPTHCOPY_INPUT
{
    float4 Position			: POSITION;
    float2 TexCoord			: TEXCOORD0;
};

PS_DEPTHCOPY_INPUT VS_DepthCopy( VS_FULLSCREEN_QUAD In )
{
	PS_DEPTHCOPY_INPUT Out = (PS_DEPTHCOPY_INPUT)0;
	Out.Position = float4( In.Position.xy, 0.0f, 1.0f );
	Out.TexCoord = In.Position.xy * float2( 0.5f, -0.5f ) + 0.5f;
	Out.TexCoord += gvPixelAccurateOffset;

    return Out;
}

struct PS_DEPTHCOPY_OUTPUT
{
	float4 Color : COLOR;
	float Depth : DEPTH;
};

PS_DEPTHCOPY_OUTPUT PS_DepthCopy( PS_DEPTHCOPY_INPUT In )
{
	PS_DEPTHCOPY_OUTPUT Out = (PS_DEPTHCOPY_OUTPUT)0;
	half4 vColor = tex2D( ColorShadowMapSampler, In.TexCoord );
	Out.Depth = vColor.r;
	Out.Color.r = vColor.r;
	
	return Out;
}

DECLARE_TECH DepthCopy < SHADER_VERSION_22 int Index = 3; int ShadowType = SHADOW_TYPE_ANY; >
{
	pass P0
	{
		COMPILE_SET_VERTEX_SHADER( vs_1_1,  VS_DepthCopy() );
		COMPILE_SET_PIXEL_SHADER( _PS_1_1, PS_DepthCopy() );

		//DXC_DEPTHSTENCIL_ZREAD_ZWRITE;
		//DXC_RASTERIZER_SOLID_NONE;
		//DXC_BLEND_RGB_NO_BLEND;
		//DXC_BLEND_RGB_SRCALPHA_INVSRCALPHA_ADD;

		//DX10_GLOBAL_STATE

#ifndef ENGINE_TARGET_DX10
		CULLMODE			= NONE;
		ColorWriteEnable	= 0; //RED | GREEN | BLUE | ALPHA;
		AlphaBlendEnable	= FALSE;
		AlphaTestEnable		= FALSE;

#if 0
		AlphaBlendEnable	= TRUE;
		BlendOp				= Add;
		SrcBlend			= SrcAlpha;
		DestBlend			= InvSrcAlpha;
		AlphaTestEnable		= TRUE;
#endif		
#endif
	}
}