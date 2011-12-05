//
//

#include "_common.fx"


half4 gvBias;
half4 gvScale;
half4 gvUVScale;

half4 gvAutomapColorSame;
half4 gvAutomapColorShadow;
half4 gvBandWeights;


struct PS_AUTOMAP
{
	float4 Position  : POSITION;
	float2 TexCoord0 : TEXCOORD0;
	float2 TexCoord1 : TEXCOORD1;
};




// ----------------------------------------------------------------------------------------------


PS_AUTOMAP VS_Automap(
	VS_POSITION_TEX In )
{
	PS_AUTOMAP Out = (PS_AUTOMAP)0;

	half3 vPos = half3( In.Position.xy, 0 );
	vPos.xy += gvBias.xy;
	vPos = mul( World, vPos );		// Rotate
	vPos.xy *= gvScale.xy;
	Out.Position = float4( vPos.xy, gvBias.z, 1.f );
	

	float2 vScale = gvUVScale.zw;
	float2 vBias = gvUVScale.xy;
	Out.TexCoord0 = In.TexCoord.xy;
	Out.TexCoord1 = ( In.TexCoord.xy + vBias.xy ) * vScale.xy;


	// Intentionally leaving out the .5 pixel offset to get slight blurring
	//Out.TexCoord += gvPixelAccurateOffset;

	return Out;
}


// ----------------------------------------------------------------------------------------------


float4 PS_Automap_Walls(
	in PS_AUTOMAP In ) : COLOR
{
	float4 vMap = tex2D( DiffuseMapSampler, In.TexCoord0 );

	half4 vFog = tex2D( LightMapSampler, In.TexCoord1 );

	half3 vCol = gvAutomapColorSame.rgb * dot( vMap.xyz, gvBandWeights.xyz ).xxx * vMap.a * vFog.b;
	return float4( vCol, 1.f );
}


float4 PS_Automap_Backdrop(
	in PS_AUTOMAP In ) : COLOR
{
	half4 vMap = tex2D( DiffuseMapSampler, In.TexCoord0 );

	half4 vFog = tex2D( LightMapSampler, In.TexCoord1 );

	half fAlpha = vMap.a * vFog.b;

	return float4( gvAutomapColorShadow.rgb * vMap.aaa, gvAutomapColorShadow.a * fAlpha );
}


DECLARE_TECH TVertexAndPixelShader
  < SHADER_VERSION_11
	int nIndex = 0; >
{
    pass Backdrop
    {
        // shaders
        COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_Automap() );
        COMPILE_SET_PIXEL_SHADER( _PS_1_1, PS_Automap_Backdrop() );
    }  

    pass Walls
    {
        // shaders
        COMPILE_SET_VERTEX_SHADER( vs_1_1, VS_Automap() );
        COMPILE_SET_PIXEL_SHADER( _PS_1_1, PS_Automap_Walls() );
    }  
}