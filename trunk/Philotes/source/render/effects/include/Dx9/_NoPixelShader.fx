//
// Basic shaders that are usually included at the bottom of an fx file
//

//--------------------------------------------------------------------------------------------
// VS_LIGHTMAP - uses the lightmap
//--------------------------------------------------------------------------------------------

struct VS_LIGHTMAP_DIFFUSE_NOLIGHTS_OUTPUT
{
	float4 Position		: POSITION; //in projection space
	float4 Diffuse		: COLOR0;
	float2 LightMap		: TEXCOORD0;
	float2 DiffuseMap	: TEXCOORD1;
};

//--------------------------------------------------------------------------------------------
VS_LIGHTMAP_DIFFUSE_NOLIGHTS_OUTPUT VS_NPS_LIGHTMAP ( VS_BACKGROUND_INPUT_DIFF2_32 In )
{
	VS_LIGHTMAP_DIFFUSE_NOLIGHTS_OUTPUT Out = (VS_LIGHTMAP_DIFFUSE_NOLIGHTS_OUTPUT)0;

	Out.Position  = mul( In.Position, WorldViewProjection );			// position (Projected space)

	// the bump map and the diffuse texture use the same texture coordinates
	Out.DiffuseMap	= In.DiffuseMap2;
	Out.LightMap	= In.LightMapDiffuse.xy;

	Out.Diffuse.xyz = 0.0f;
	Out.Diffuse.w = 1.0f;

	return Out;
}


//--------------------------------------------------------------------------------------------
technique TVertexShaderOnly < int VSVersion = VS_1_1; int PSVersion = PS_NONE; int PointLights = 0; > 
{
    pass P0
    {
		VertexShader = compile vs_1_1 VS_NPS_LIGHTMAP();
		PixelShader  = NULL;
		COMBINE_LIGHTMAP_DIFFUSE
    }
}
