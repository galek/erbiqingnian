//
// GlowShared - it just transforms and uses one texture - with full glow
//

#include "StateBlocks.fxh"

//////////////////////////////////////////////////////////////////////////////////////////////

float4 PS(
    VS_OUTPUT input ) : COLOR
{
	half4 cColor = tex2D(DiffuseMapSampler, input.Tex) * gfSelfIlluminationPower;
	cColor.w = ( cColor.w + gfAlphaConstant ) * gfSelfIlluminationGlow;
    return cColor;
}

//////////////////////////////////////////////////////////////////////////////////////////////
DECLARE_TECH TVertexAndPixelShader
  < SHADER_VERSION_11 >
{
    pass P0
    {
        // shaders
        COMPILE_SET_VERTEX_SHADER( vs_1_1, VS() );
		COMPILE_SET_PIXEL_SHADER( ps_2_0, PS() );

		DXC_BLEND_RGBA_NO_BLEND;

#ifndef ENGINE_TARGET_DX10
		ColorWriteEnable		= RED | GREEN | BLUE | ALPHA;
		AlphaBlendEnable		= FALSE;
		AlphaTestEnable			= FALSE;
#endif
    }  
}

//////////////////////////////////////////////////////////////////////////////////////////////
technique TVertexShaderOnly
  < int VSVersion = VS_1_1; int PSVersion = PS_NONE; >
{
    pass P0
    {
        // shaders
        VertexShader = compile vs_1_1 VS_NPS();
        PixelShader  = NULL;
        
        // texture stages
        ColorOp[0]   = MODULATE;
        ColorArg1[0] = TEXTURE;
        ColorArg2[0] = DIFFUSE;
        AlphaOp[0]   = DISABLE;		// breaks on some drivers -- should be TFACTOR (0) and select arg 1 and AlphaOp[1] = DISABLE

        ColorOp[1]   = DISABLE;
 
		MipFilter[0] = LINEAR;
		MinFilter[0] = LINEAR;
		MagFilter[0] = LINEAR;
    }
}


