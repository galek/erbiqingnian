//
// Particle Shader include
//

#include "_ParticlesInput.fxh"


// transformations

//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////

//--------------------------------------------------------------------------------------------
PS_BASIC_PARTICLE_INPUT		FN(VS_BASIC)
	( VS_PARTICLE_INPUT In )
{
    PS_BASIC_PARTICLE_INPUT Out = (PS_BASIC_PARTICLE_INPUT)0;

    Out.Position = mul( float4( In.Position.xyz, 1.0f ), WorldViewProjection);          // position (projected)
#ifdef DX10_SOFT_PARTICLES
    Out.Z        = Out.Position.w;
#endif
    Out.Color	 = DX10_SWIZ_COLOR_INPUT( In.Color );									  
    Out.Tex		 = In.Tex;
    
	FOGMACRO( In.Position )
    
    return Out;
}

//--------------------------------------------------------------------------------------------
PS_BASIC_PARTICLE_INPUT		FN(VS_BASIC_ADD)
	( VS_PARTICLE_INPUT In )
{
    PS_BASIC_PARTICLE_INPUT Out = (PS_BASIC_PARTICLE_INPUT)0;

    Out.Position = mul( float4( In.Position, 1.0f ), WorldViewProjection);          // position (projected)
    Out.Tex		 = In.Tex;
#ifdef DX10_SOFT_PARTICLES
    Out.Z        = Out.Position.w;
#endif
    
	FOGMACRO_OUTPUT( half fFog, In.Position )
	Out.Color		= DX10_SWIZ_COLOR_INPUT( In.Color );
	if ( FogAdditiveParticleLum )
		Out.Color	*= fFog;
	
	// don't actually output and use fog
	//Out.Fog		 = fFog;
    
    return Out;
}

//--------------------------------------------------------------------------------------------
float4		FN(PS_BASIC)
	( PS_BASIC_PARTICLE_INPUT In ) : COLOR
{
    float4 color = tex2D(DiffuseMapSampler,  In.Tex) * In.Color;
    
#ifdef DX10_SOFT_PARTICLES
	FeatherAlpha( color.a, In.Position, In.Z );
#endif

    AppControlledAlphaTest( color );
    return color;
}

//--------------------------------------------------------------------------------------------
float4		FN(PS_BASIC_MULT)
	( PS_BASIC_PARTICLE_INPUT In ) : COLOR
{
    float4 color = tex2D(DiffuseMapSampler,  In.Tex) * In.Color;

	color.rgb = lerp( half3(1,1,1), color.rgb, color.a );

#ifdef DX10_SOFT_PARTICLES
	FeatherAlpha( color.a, In.Position, In.Z );
#endif

    AppControlledAlphaTest( color );

    return color;
}

//////////////////////////////////////////////////////////////////////////////////////////////


PS_GLOWCONSTANT_ADD_PARTICLE_INPUT		FN(VS_GLOWCONSTANT_ADD)
	( VS_PARTICLE_INPUT In )
{
    PS_GLOWCONSTANT_ADD_PARTICLE_INPUT Out = (PS_GLOWCONSTANT_ADD_PARTICLE_INPUT)0;

    Out.Position = mul( float4( In.Position, 1.0f ), WorldViewProjection);          // position (projected)
#ifdef DX10_SOFT_PARTICLES
    Out.Z        = Out.Position.w;
#endif
    Out.Tex		 = In.Tex;
    
	FOGMACRO_OUTPUT( half fFog, In.Position )

    Out.Color		= DX10_SWIZ_COLOR_INPUT( In.Color );
    Out.Glow.xyzw	= In.GlowConstant.w;

	if ( FogAdditiveParticleLum )
	{
		Out.Color	*= fFog;
		Out.Glow	*= fFog;  // fog the glow constant
	}
    
    // don't actually output and use fog
	//Out.Fog		 = fFog;
    
    return Out;
}

//--------------------------------------------------------------------------------------------
float4		FN(PS_GLOWCONSTANT_ADD)
	( PS_GLOWCONSTANT_ADD_PARTICLE_INPUT In ) : COLOR
{
    float4 Color = tex2D(DiffuseMapSampler,  In.Tex) * In.Color;

#ifdef DX10_SOFT_PARTICLES
	TestDepth( In.Position, In.Z );
#endif

    Color.xyz *= Color.w;
    Color.w *= In.Glow.w;
    
    //AlphaTest( false, Color );  //Forcing off for this one shader

    return Color;
}

//////////////////////////////////////////////////////////////////////////////////////////////


PS_GLOWCONSTANT_PARTICLE_INPUT		FN(VS_GLOWCONSTANT)
	( VS_PARTICLE_INPUT In )
{
    PS_GLOWCONSTANT_PARTICLE_INPUT Out = (PS_GLOWCONSTANT_PARTICLE_INPUT)0;

    Out.Position = mul( float4( In.Position, 1.0f ), WorldViewProjection);          // position (projected)
#ifdef DX10_SOFT_PARTICLES
    Out.Z        = Out.Position.w;
#endif

	FOGMACRO_OUTPUT( half fFog, In.Position )
    Out.Glow.xyzw = In.GlowConstant.w; // * fFog;   // fog the glow constant
    Out.Tex		  = In.Tex;
    
    return Out;
}

//--------------------------------------------------------------------------------------------
float4		FN(PS_GLOWCONSTANT)
	( PS_GLOWCONSTANT_PARTICLE_INPUT In ) : COLOR
{
    float4 color = tex2D(DiffuseMapSampler,  In.Tex).w * In.Glow.w;

#ifdef DX10_SOFT_PARTICLES
	TestDepth( In.Position, In.Z );
#endif

    AppControlledAlphaTest( color );
    return color;
}






//////////////////////////////////////////////////////////////////////////////////////////////
DECLARE_TECH TECHNAME(TVertexAndPixelShader0)  <
	SHADER_VERSION_11
	int Additive = 0;
	int Glow = 0;
	int SoftParticles = SOFT_PARTICLES; >
{
    pass P0
    {
        // shaders
        COMPILE_SET_VERTEX_SHADER( vs_1_1, FN(VS_BASIC)() );
        COMPILE_SET_PIXEL_SHADER( _PS_1_1, FN(PS_BASIC)() );
        DXC_BLEND_RGB_SRCALPHA_INVSRCALPHA_ADD;
        
#ifndef ENGINE_TARGET_DX10
#ifdef DXC_DEFINE_SOME_STATE_IN_FX
		AlphaTestEnable	 = TRUE;
		FogEnable		 = FALSE;
		CullMode = NONE;
		AlphaBlendEnable = TRUE;
		BlendOp			 = ADD;
		ZWriteEnable	= FALSE;
		
		SrcBlend         = SRCALPHA;
		DestBlend        = INVSRCALPHA;
#endif
		COLORWRITEENABLE = BLUE | GREEN | RED;
#endif
    }  
}

//////////////////////////////////////////////////////////////////////////////////////////////
DECLARE_TECH TECHNAME(TVertexAndPixelShaderAdditive)  < 
	SHADER_VERSION_11
	int Additive = 1; 
	int Glow = 0;
	int SoftParticles = SOFT_PARTICLES; > 
{
    pass P0
    {
        // shaders
        COMPILE_SET_VERTEX_SHADER( vs_1_1, FN(VS_BASIC_ADD)() );
        COMPILE_SET_PIXEL_SHADER( _PS_1_1, FN(PS_BASIC)() );
       
#ifdef DXC_DEFINE_SOME_STATE_IN_FX 
        DXC_BLEND_RGB_SRCALPHA_ONE_ADD;
#endif

#ifndef ENGINE_TARGET_DX10
#ifdef DXC_DEFINE_SOME_STATE_IN_FX
		FogEnable		 = FALSE;
		AlphaTestEnable	 = TRUE;
		AlphaBlendEnable = TRUE;
		BlendOp			 = ADD;
		CullMode = NONE;
		ZWriteEnable	 = FALSE;
		
		SrcBlend         = SRCALPHA;
		DestBlend        = ONE;
#endif

		COLORWRITEENABLE = BLUE | GREEN | RED;
#endif
    }  
}



//////////////////////////////////////////////////////////////////////////////////////////////
DECLARE_TECH TECHNAME(TVertexAndPixelShaderMultiply)  < 
	SHADER_VERSION_11
	int Additive = 0; 
	int Multiply = 1; 
	int Glow = 0;
	int SoftParticles = SOFT_PARTICLES; > 
{
    pass P0
    {
        // shaders
        COMPILE_SET_VERTEX_SHADER( vs_1_1, FN(VS_BASIC)() );
        COMPILE_SET_PIXEL_SHADER( _PS_1_1, FN(PS_BASIC_MULT)() );
       
#ifdef DXC_DEFINE_SOME_STATE_IN_FX 
        DXC_BLEND_RGBA_DESTCOLOR_ONE_ADD;
#endif

#ifndef ENGINE_TARGET_DX10
#ifdef DXC_DEFINE_SOME_STATE_IN_FX
		FogEnable		 = FALSE;
		AlphaTestEnable	 = TRUE;
		AlphaBlendEnable = TRUE;
		BlendOp			 = ADD;
		CullMode = NONE;
		ZWriteEnable	 = FALSE;
		
		SrcBlend         = ZERO;
		DestBlend        = SRCCOLOR;
#endif

		COLORWRITEENABLE = BLUE | GREEN | RED;
#endif
    }  
}


//////////////////////////////////////////////////////////////////////////////////////////////
DECLARE_TECH TECHNAME(TVertexAndPixelShaderGlow)  < 
	SHADER_VERSION_11
	int Additive = 0; 
	int Glow = 1;
	int SoftParticles = SOFT_PARTICLES; > 
{
    pass P0
    {
        // shaders
        COMPILE_SET_VERTEX_SHADER( vs_1_1, FN(VS_BASIC)() );
        COMPILE_SET_PIXEL_SHADER( _PS_1_1, FN(PS_BASIC)() );

#ifdef DXC_DEFINE_SOME_STATE_IN_FX
		DXC_BLEND_RGBA_SRCALPHA_INVSRCALPHA_ADD;
#endif
		
#ifndef ENGINE_TARGET_DX10
#ifdef DXC_DEFINE_SOME_STATE_IN_FX
		FogEnable		 = FALSE;
		AlphaTestEnable	 = TRUE;
		AlphaBlendEnable = TRUE;
		BlendOp			 = ADD;
		CullMode = NONE;
		ZWriteEnable	 = FALSE;
		
		SrcBlend         = SRCALPHA;
		DestBlend        = INVSRCALPHA;
#endif

		COLORWRITEENABLE = BLUE | GREEN | RED | ALPHA;
#endif 
    }  
}

//////////////////////////////////////////////////////////////////////////////////////////////
DECLARE_TECH TECHNAME(TVertexAndPixelShader)  < 
	SHADER_VERSION_11
	int Additive = 1; 
	int Glow = 1;
	int SoftParticles = SOFT_PARTICLES; > 
{
    pass P0
    {
        // shaders
        COMPILE_SET_VERTEX_SHADER( vs_1_1, FN(VS_BASIC_ADD)() );
        COMPILE_SET_PIXEL_SHADER( _PS_1_1, FN(PS_BASIC)() );
        
#ifdef DXC_DEFINE_SOME_STATE_IN_FX
		DXC_BLEND_RGBA_SRCALPHA_ONE_ADD;

#ifndef ENGINE_TARGET_DX10
		FogEnable		 = FALSE;
		AlphaTestEnable	 = TRUE;
		AlphaBlendEnable = TRUE;
		BlendOp			 = ADD;
		CullMode = NONE;
		ZWriteEnable	 = FALSE;
		
		SrcBlend         = SRCALPHA;
		DestBlend        = ONE;
		
		COLORWRITEENABLE = BLUE | GREEN | RED | ALPHA;
#endif
#endif
    }  
}

//////////////////////////////////////////////////////////////////////////////////////////////
DECLARE_TECH TECHNAME(TVertexAndPixelShaderGlowGlowConstant)  < 
	SHADER_VERSION_11
	int Additive = 0; 
	int Glow = 1;
	int GlowConstant = 1;
	int SoftParticles = SOFT_PARTICLES; > 
{
    pass P0
    {
        // shaders
        COMPILE_SET_VERTEX_SHADER( vs_1_1, FN(VS_BASIC)() );
        COMPILE_SET_PIXEL_SHADER( _PS_1_1, FN(PS_BASIC)() );
        
#ifdef DXC_DEFINE_SOME_STATE_IN_FX
        DXC_BLEND_RGB_SRCALPHA_INVSRCALPHA_ADD;
#endif

#ifndef ENGINE_TARGET_DX10
#ifdef DXC_DEFINE_SOME_STATE_IN_FX
		AlphaTestEnable	 = TRUE;
		AlphaBlendEnable = TRUE;
		BlendOp			 = ADD;
		FogEnable		 = FALSE;
		CullMode		 = NONE;
		ZWriteEnable	 = FALSE;
		
		SrcBlend         = SRCALPHA;
		DestBlend        = INVSRCALPHA;
#endif

		COLORWRITEENABLE = BLUE | GREEN | RED;
#endif
    }  

    pass P1
    {
        // shaders
        COMPILE_SET_VERTEX_SHADER( vs_1_1, FN(VS_GLOWCONSTANT)() );
        COMPILE_SET_PIXEL_SHADER( _PS_1_1, FN(PS_GLOWCONSTANT)() );
       
#ifndef ENGINE_TARGET_DX10
#ifdef DXC_DEFINE_SOME_STATE_IN_FX
		FogEnable		 = FALSE;
		AlphaTestEnable	 = TRUE;
		AlphaBlendEnable = TRUE;
		CullMode		 = NONE;
		ZWriteEnable	 = FALSE;
#endif
		COLORWRITEENABLE = ALPHA;
		BlendOp			 = MAX;
		SrcBlend         = ONE;
		DestBlend        = ONE;
#else
		DXC_BLEND_A_ONE_ONE_MAX;
#endif 
    }  
}

//////////////////////////////////////////////////////////////////////////////////////////////
DECLARE_TECH TECHNAME(TVertexAndPixelShaderAddGlowGlowConstant)  < 
	SHADER_VERSION_11
	int Additive = 1; 
	int Glow = 1;
	int GlowConstant = 1;
	int SoftParticles = SOFT_PARTICLES; > 
{
    pass P0
    {
        // shaders
        COMPILE_SET_VERTEX_SHADER( vs_1_1, FN(VS_GLOWCONSTANT_ADD)() );
        COMPILE_SET_PIXEL_SHADER( _PS_1_1, FN(PS_GLOWCONSTANT_ADD)() );
        
#ifdef DXC_DEFINE_SOME_STATE_IN_FX
		DXC_BLEND_RGBA_ONE_ONE_ADD; //DXC_BLEND_A_ONE_ONE_MAX;
#endif

#ifndef ENGINE_TARGET_DX10
#ifdef DXC_DEFINE_SOME_STATE_IN_FX
		FogEnable		 = FALSE;
		CullMode		 = NONE;
		AlphaBlendEnable = TRUE;
		BlendOp			 = ADD;
		
		ZWriteEnable	 = FALSE;
		
		SrcBlend         = ONE;
		DestBlend        = ONE;
#endif
		AlphaTestEnable	 = FALSE;
		COLORWRITEENABLE = BLUE | GREEN | RED | ALPHA;
#endif
    }  
}

//////////////////////////////////////////////////////////////////////////////////////////////
// we really aren't working on supporting this type of shader yet...
technique TECHNAME(TVertexShaderOnly)  < 
	int VSVersion = VS_1_1;
	int PSVersion = PS_NONE;
	int Additive = 0; 
	int Glow = 0;
	int SoftParticles = SOFT_PARTICLES; > 
{
    pass P0
    {
        // shaders
        VertexShader = compile vs_1_1 FN(VS_BASIC) ();
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

#ifdef DXC_DEFINE_SOME_STATE_IN_FX
		CullMode = NONE;
		AlphaTestEnable	 = TRUE;
		AlphaBlendEnable = TRUE;
		FogEnable		 = FALSE;
		
		BlendOp			 = ADD;
		SrcBlend         = SRCALPHA;
		DestBlend        = INVSRCALPHA;
#endif
    }
}
