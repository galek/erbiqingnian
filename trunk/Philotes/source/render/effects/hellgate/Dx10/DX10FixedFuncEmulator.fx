#include "_Common.fx"


//Global textures
Texture2D g_TextureStageArray;
bool bUIOpaque = true;

struct SimplePS_Input
{
	float4 pos : SV_Position;
	float2 tex : TEXCOORD0;
	float4 color : COLOR;
};

SimplePS_Input SimpleVS( VS_BACKGROUND_INPUT_16 Input )
{
	SimplePS_Input output = (SimplePS_Input)0.0f;
	output.pos = mul( float4( Input.Position.xyz, 1.0f ), WorldViewProjection );
	output.tex = UNBIAS_TEXCOORD( Input.TexCoord );
	return output;
}


SimplePS_Input QuadEmulatorVS( VS_UI_XYZW_COL_UV input ) //VERTEX_DECL_TEST input )
{
	SimplePS_Input output = (SimplePS_Input)0.0f;

	output.pos.x = (input.Position.x / (gvScreenSize.x/2.0)) -1;
	output.pos.y = -(input.Position.y / (gvScreenSize.y/2.0)) +1;
	output.pos.z = input.Position.z;
	output.pos.w = 1.0f;

	output.tex = input.Tex;
	output.color = input.Diff.bgra;
	return output;
}

float4 QuadEmulatorPS( SimplePS_Input input ) : SV_Target0
{
	float4 color = g_TextureStageArray.Sample( FILTER_MIN_MAG_MIP_POINT_CLAMP, float4( input.tex, 0.0f, 0.0f ) );
	
	if( !bUIOpaque )
		color.rgb *= input.color.rgb;
	
	color.a *= input.color.a;

	AppControlledAlphaTest( color );
	return color;
}

PixelShader QuadShader = CompileShader( ps_4_0, QuadEmulatorPS() );

technique10 QuadEmulator < SHADER_VERSION_44 >
{
	pass p0
	{
		SetVertexShader( CompileShader( vs_4_0, QuadEmulatorVS() ) );
		SetGeometryShader( NULL );
		SetPixelShader( QuadShader );
		
		DXC_RASTERIZER_SOLID_NONE;
		DXC_BLEND_RGBA_SRCALPHA_INVSRCALPHA_ADD;
		DXC_DEPTHSTENCIL_ZREAD_ZWRITE;
	}
}