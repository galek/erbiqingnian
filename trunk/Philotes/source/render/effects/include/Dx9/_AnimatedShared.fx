//
// Animated Shared - functions that are shared between shaders
//

//////////////////////////////////////////////////////////////////////////////////////////////
// Shared Structures
//////////////////////////////////////////////////////////////////////////////////////////////

struct PS11_DIFFUSE_INPUT
{
    float4 Position			: POSITION; //in projection space
    float4 Diffuse			: COLOR0; 
    float2 LightMap 		: TEXCOORD0; 
    float2 DiffuseMap 		: TEXCOORD1; 
};

//--------------------------------------------------------------------------------------------
// No pixel shader
//--------------------------------------------------------------------------------------------
#define TECHNIQUESHADERONLY_LIGHTMAP( points, skinned ) technique TVertexShaderOnly < \
	int VSVersion = VS_1_1; \
	int PSVersion = PS_NONE; \
	int PointLights = points; \
	int Specular = 0; \
	int Skinned = skinned; \
	int NormalMap = 0; \
	int SelfIllum = 1; > { \
	pass P0 { \
		VertexShader = compile vs_1_1 VS11_NPS_ACTOR( points, skinned ); \
        PixelShader  = NULL; \
        COMBINE_LIGHTMAP_DIFFUSE \
		}}

#define TECHNIQUESHADERONLY_NOLIGHTMAP( points, skinned ) technique TVertexShaderOnly < \
	int VSVersion = VS_1_1; \
	int PSVersion = PS_NONE; \
	int PointLights = points; \
	int Specular = 0; \
	int Skinned = skinned; \
	int NormalMap = 0; \
	int SelfIllum = 0; > { \
	pass P0 { \
		VertexShader = compile vs_1_1 VS11_NPS_ACTOR( points, skinned ); \
        PixelShader  = NULL; \
		ColorOp[0]   = MODULATE; \
		ColorArg1[0] = TEXTURE; \
		ColorArg2[0] = DIFFUSE; \
		AlphaArg1[0] = DIFFUSE; \
		AlphaOp[0]   = SELECTARG1; \
		ColorOp[1]   = DISABLE; \
		AlphaOp[1]   = DISABLE; \
		MipFilter[0] = LINEAR; \
		MinFilter[0] = LINEAR; \
		MagFilter[0] = LINEAR; \
		}}

