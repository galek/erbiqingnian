// Copyright (c) 2003 Flagship Studios, Inc.
// 
//

#ifndef __LIGHTING_CONSTANTS_FX__
#define __LIGHTING_CONSTANTS_FX__

//
// Lighting structures and functions commonly used across fx files
//

// --------------------------------------------------------------------------------------
// DEFINES
// --------------------------------------------------------------------------------------


#define MAX_POINT_LIGHTS_PER_SHADER			5
#define MAX_SPECULAR_LIGHTS_PER_SHADER		2
#define MAX_SPOT_LIGHTS_PER_SHADER			1


// these must match ENV_DIR_LIGHT_TYPE
#define DIR_LIGHT_0_DIFFUSE					0
#define DIR_LIGHT_1_DIFFUSE					1
#define DIR_LIGHT_0_SPECULAR				2
#define MAX_DIR_LIGHTS_PER_SHADER			3



#define OBJECT_SPACE	0
#define WORLD_SPACE		1
// count
#define NUM_SPACES		2


// --------------------------------------------------------------------------------------
// STRUCTS
// --------------------------------------------------------------------------------------

//
// Light falloff:   x == far end / ( far start - far end ),		y == 1 / ( far start - far end )
//


struct Light
{
	half3 	Albedo;

	half3 	Diffuse;
	half3 	Emissive;
	half3 	Specular;
	half4 	Reflect;			// env map power/lerp factor in alpha

	half	SpecularPower;
	half	SpecularGloss;
	half	SpecularAlpha;
	half	Glow;
	half	Opacity;
	half	LightingAmount;		// 1.f == fully lit
};


// CHB 2007.07.17
struct PS_COMMON_SPOTLIGHT_INPUT
{
    float4 St;					// spot light to vertex
    float4 Nt;					// spot light direction
    float4 Unused0;
    float4 Cs;					// spot light color
};


// --------------------------------------------------------------------------------------
// GLOBALS
// --------------------------------------------------------------------------------------

// MATRICES
//CBUFFER_START_SHARED( ShadowCB )

float4x4 ViewToLightProjection                 < string UIWidget = "None"; >;

float4x4 gmShadowMatrix;
float4x4 gmShadowMatrix2;
float4x4 gmShadowView;
float4x4 gmShadowProjection;

half3 ShadowLightDir;

//CBUFFER_END


//CBUFFER_START_SHARED( LightingCB )
// DIFFUSE POINT LIGHTS

#define PointLightsPos(space)			_PointLightsPos_##space
#define PointLightsFalloff(space)		_PointLightsFalloff_##space

float4 _PointLightsPos_0    [ MAX_POINT_LIGHTS_PER_SHADER ];
float4 _PointLightsPos_1    [ MAX_POINT_LIGHTS_PER_SHADER ];
half4  PointLightsColor     [ MAX_POINT_LIGHTS_PER_SHADER ];
half4  _PointLightsFalloff_0[ MAX_POINT_LIGHTS_PER_SHADER ];
half4  _PointLightsFalloff_1[ MAX_POINT_LIGHTS_PER_SHADER ];
int PointLightCount;




// SPECULAR POINT LIGHTS

#define SpecularLightsPos(space)			_SpecularLightsPos_##space
#define SpecularLightsFalloff(space)		_SpecularLightsFalloff_##space

float4 _SpecularLightsPos_0	   [ MAX_SPECULAR_LIGHTS_PER_SHADER ];
float4 _SpecularLightsPos_1	   [ MAX_SPECULAR_LIGHTS_PER_SHADER ];
half4  SpecularLightsColor     [ MAX_SPECULAR_LIGHTS_PER_SHADER ];
half4  _SpecularLightsFalloff_0[ MAX_SPECULAR_LIGHTS_PER_SHADER ];
half4  _SpecularLightsFalloff_1[ MAX_SPECULAR_LIGHTS_PER_SHADER ];




// DIRECTIONAL LIGHTS

#define DirLightsDir(space)			_DirLightsDir_##space

half4 _DirLightsDir_0[ MAX_DIR_LIGHTS_PER_SHADER ];
half4 _DirLightsDir_1[ MAX_DIR_LIGHTS_PER_SHADER ];
half4 DirLightsColor [ MAX_DIR_LIGHTS_PER_SHADER ];




// SPOT LIGHTS

#define SpotLightsPos(space)			_SpotLightsPos_##space
#define SpotLightsFalloff(space)		_SpotLightsFalloff_##space
#define SpotLightsNormal(space)			_SpotLightsNormal_##space

float4	_SpotLightsPos_0    [ MAX_SPOT_LIGHTS_PER_SHADER ];			// position
float4	_SpotLightsPos_1    [ MAX_SPOT_LIGHTS_PER_SHADER ];			// position
half4	_SpotLightsNormal_0 [ MAX_SPOT_LIGHTS_PER_SHADER ];			// normal
half4	_SpotLightsNormal_1 [ MAX_SPOT_LIGHTS_PER_SHADER ];			// normal
half4	_SpotLightsFalloff_0[ MAX_SPOT_LIGHTS_PER_SHADER ];			// falloff
half4	_SpotLightsFalloff_1[ MAX_SPOT_LIGHTS_PER_SHADER ];			// falloff
half4	SpotLightsColor     [ MAX_SPOT_LIGHTS_PER_SHADER ];			// color:3, strength:1



// MISC ENVIRONMENT

half4 LightAmbient;



// MATERIAL

half4 gvSpecularMaterialData;
#define SpecularGlossiness						gvSpecularMaterialData.xy
#define SpecularBrightness						gvSpecularMaterialData.z
#define SpecularGlow							gvSpecularMaterialData.w

half4 gvMiscLightingData;
#define gfShadowIntensity						gvMiscLightingData.y
#define gfAlphaConstant							gvMiscLightingData.z
#define gfAlphaForLighting						gvMiscLightingData.w	// CHB 2007.09.11 - PS 1.1 compiler generates better code when this is w (!)

half4 gvMiscMaterialData;
#define SpecularMaskOverride					gvMiscMaterialData.x
#define gfSelfIlluminationGlow					gvMiscMaterialData.y
#define gfSelfIlluminationPower					gvMiscMaterialData.z
#define gfSubsurfaceScatterSharpness			gvMiscMaterialData.w

half4 gvEnvironmentMapData;
#define gfEnvironmentMapPower					gvEnvironmentMapData.x
#define gfEnvironmentMapGlossThreshold			gvEnvironmentMapData.y
#define gfEnvironmentMapInvertGlossThreshold	gvEnvironmentMapData.z
#define gfEnvironmentMapLODBias					gvEnvironmentMapData.w

half4 gvSubsurfaceScatterColorPower;

//CBUFFER_END


#endif // __LIGHTING_CONSTANTS_FX__
