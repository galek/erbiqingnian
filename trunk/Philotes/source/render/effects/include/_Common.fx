// Copyright (c) 2003 Flagship Studios, Inc.
// 
//

#ifndef __COMMON_FX__
#define __COMMON_FX__

//
// Functions commonly used across fx files
//
#pragma warning( disable : 4707 ) 
#pragma warning( disable : 3557 ) 

#include "VertexInputStructs.fxh"
#include "dx9/_Branch.fxh"

#ifdef ENGINE_TARGET_DX10
//#define DX10_EMULATE_DX9_BEHAVIOR
#include "Dx10/dx10_Common.fxh"
#endif

#include "_Macros.fxh"



// -------------------------------------------------------------------------------
// TEXTURES and SAMPLERS
// -------------------------------------------------------------------------------

#include "Dx9/_SamplersCommon.fxh"


// -------------------------------------------------------------------------------
// GLOBALS (incl global constant buffer)
// -------------------------------------------------------------------------------
#ifdef ENGINE_TARGET_DX10
	#define CBUFFER_START_SHARED( name ) shared cbuffer name {
	#define CBUFFER_START( name ) cbuffer name {
	#define CBUFFER_END };
#else
	#define CBUFFER_START_SHARED( name )
	#define CBUFFER_START( name )
	#define CBUFFER_END
#endif

CBUFFER_START_SHARED( LightingConstantCB )
#include "Dx9/_LightingConstants.fxh"
CBUFFER_END

CBUFFER_START_SHARED( GlobalCB )

half GLOW_MIN						= half(DEFAULT_GLOW_MIN);

// -------------------------------------------------------------------------------
// STATIC BRANCHING BOOLS
// -------------------------------------------------------------------------------
bool gbSpecular;
bool gbNormalMap;
bool gbDiffuseMap2;
bool gbLightMap;
bool gbShadowMap;
bool gbShadowMapDepth;
bool gbDebugFlag;
bool gbCubeEnvironmentMap;
bool gbSphereEnvironmentMap;
bool gbSkinned;
bool gbDirectionalLight;
bool gbDirectionalLight2;
bool gbScrollLightmap;

CBUFFER_END

//--------------------------------------------------------------------------------
// GLOBALS
//--------------------------------------------------------------------------------

CBUFFER_START_SHARED( WorldMatrixCB )
	float4x4 WorldViewProjection		: WorldViewProj < string UIWidget = "None"; >;
	float4x4 WorldView				: WorldView	   < string UIWidget = "None"; >;
	float4x4 World					: World		   < string UIWidget = "None"; >;

	//--------------------------------------------------------------------------------
	float4 EyeInObject : position; // this is where the camera is...
	float4 EyeInWorld  : position; // this is where the camera is...

#ifdef ENGINE_TARGET_DX10
	float4x4 WorldViewProjectionPrev;
#endif
CBUFFER_END

CBUFFER_START_SHARED( ViewMatrixCB )
float4x4 View					: View		   < string UIWidget = "None"; >;
float4x4 ViewProjection			: ViewProj	   < string UIWidget = "None"; >;
CBUFFER_END

CBUFFER_START_SHARED( RarelyUpdatedCB )
	float4x4 Projection				: Projection	   < string UIWidget = "None"; >;
	float4 gvScreenSize;          // (xpixels, ypixels, 1/xpixels, 1/ypixels)
	float4 gvViewport;			  // (tlx, tly, brx, bry)
	float2 gvPixelAccurateOffset;
	float4 gvShadowSize;			// (xpixels, ypixels, 1/xpixels, 1/ypixels)

	const float FallOutThickness = 2.0f;
	
	//--------------------------------------------------------------------------------
	half FogMaxDistance;
	half FogMinDistance;
	half4 FogColor;
	bool FogAdditiveParticleLum;
	half gfFogFactor; // 0 to enable, 1 to disable

	//--------------------------------------------------------------------------------
#ifdef ENGINE_TARGET_DX10

	//NVTL: this is for the soft particles
	half gSoftParticleScale = 0.5;
	half gSoftParticleContrast = 2.0;

#endif // DX10
	//--------------------------------------------------------------------------------
CBUFFER_END

#ifdef ENGINE_TARGET_DX10
CBUFFER_START_SHARED( SmokeCB )
//variables for smoke simulation:
float modulate;
float4 splatColor;
float4 center; 
float size;
float4 gridDim;
float forward;
float WidthZOffset;
float SimulationTextureWidth;
float SimulationTextureHeight;
float RTWidth;		// backbuffer width
float RTHeight;		// backbuffer height
float renderScale;
int cols;
float impulseSize;
float impulseHeight;
float4x4 InverseProjection;
float4x4 InverseWorldView;
float smokeDensityModifier;
float smokeVelocityModifier;
float smokeAmbientLight;
float smokeThickness;
int maxGridDim; 
float timestep;
float decayGridDistance   = 10;
float ZFar;
float ZNear;
float ZFarMinusZNear;
float ZFarMultZNear;
float glowMinDensity = 0.4;
float4 glowColorCompensation = float4(-0.17,-0.8,-1.0,1);
float confinementEpsilon;
float maxObstacleHeight;
float distanceToScreenY;
float distanceToScreenX;
CBUFFER_END

#endif // DX10

//////////////////////////////////////////////////////////////////////////////////
////////////// Constants /////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

#ifdef FXC10_PATH
#include "_ShaderVersionConstants.fx"
#else
#include "_ShaderVersionConstants.fx"
#endif








//////////////////////////////////////////////////////////////////////////////////
////////////// Vertex Shaders ////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

struct PS_FULLSCREEN_QUAD
{
	float4 Position : POSITION;
	float2 TexCoord : TEXCOORD;
};

PS_FULLSCREEN_QUAD VS_FSQuadUV( VS_FULLSCREEN_QUAD input )
{
	PS_FULLSCREEN_QUAD output = (PS_FULLSCREEN_QUAD)0.0f;
	output.Position = float4( input.Position.xy, 0.0f, 1.0f );
	output.TexCoord = input.Position.xy * float2( 0.5f, -0.5f ) + 0.5f;

	// Simulates setting a "source rectangle" held in gvViewport
	half2 vScale		= ( gvViewport.zw - gvViewport.xy ) / ( gvScreenSize.xy);
	half2 vOffset		= gvViewport.xy / gvScreenSize.xy;
	output.TexCoord.xy	*= vScale;
	output.TexCoord.xy	+= vOffset;

	output.TexCoord += gvPixelAccurateOffset;
	return output;
}
//--------------------------------------------------------------------------------------------



//////////////////////////////////////////////////////////////////////////////////
////////////// Functions /////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////

#ifdef ENGINE_TARGET_DX10
float fGetNormDepth( float2 coords )
{
	return Texture_sceneDepth.Sample( FILTER_MIN_MAG_MIP_POINT_CLAMP, coords ).r / ZFar;
}

float fGetDepth( float2 coords )
{
	return Texture_sceneDepth.Sample( FILTER_MIN_MAG_MIP_POINT_CLAMP, coords ).r;
}
#endif


#include "Dx9/_Lighting.fxh"


#endif // __COMMON_FX__
