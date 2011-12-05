//
// _TexturesCommon - header file for texture structs/registers and common samplers
//

#include "Dx9/_TexturesCommon.fxh"




TEXSHARED TEXTURE2D tDiffuseMap;
TEXSAMPLER DiffuseMapSampler : register(SAMPLER(SAMPLER_DIFFUSE)) = sampler_state
{
	Texture = (tDiffuseMap);
	
#ifdef ENGINE_TARGET_DX10
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = WRAP;
	AddressV = WRAP;
#endif
};





TEXSHARED TEXTURE2D tLightMap;
TEXSAMPLER LightMapSampler : register(SAMPLER(SAMPLER_LIGHTMAP)) = sampler_state
{
	Texture = (tLightMap);
	
#ifdef ENGINE_TARGET_DX10
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = CLAMP;
	AddressV = CLAMP;
#endif
};




TEXSHARED TEXTURE2D tSelfIllumMap;
TEXSAMPLER SelfIlluminationMapSampler : register(SAMPLER(SAMPLER_SELFILLUM)) = sampler_state
{
	Texture = (tSelfIllumMap);
	
#ifdef ENGINE_TARGET_DX10
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = WRAP;
	AddressV = WRAP;
#endif
};





TEXTURE2D tDiffuseMap2;
/*	
int DiffuseMap2AddressU = ADDRESS_CLAMP;
int DiffuseMap2AddressV = ADDRESS_CLAMP;
*/
TEXSAMPLER DiffuseMapSampler2 : register(SAMPLER(SAMPLER_DIFFUSE2)) = sampler_state
{
	Texture = (tDiffuseMap2);
	
#ifdef ENGINE_TARGET_DX10
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = CLAMP;
	AddressV = CLAMP;
#endif
};





TEXSHARED TEXTURE2D tNormalMap;
TEXSAMPLER NormalMapSampler : register(SAMPLER(SAMPLER_NORMAL)) = sampler_state
{
	Texture = (tNormalMap);
	
#ifdef ENGINE_TARGET_DX10
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = WRAP;
	AddressV = WRAP;
#endif
};






TEXSHARED TEXTURE2D tSpecularMap;
TEXSAMPLER SpecularMapSampler : register(SAMPLER(SAMPLER_SPECULAR)) = sampler_state
{
	Texture = (tSpecularMap);
	
#ifdef ENGINE_TARGET_DX10
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = WRAP;
	AddressV = WRAP;
#endif
};







TEXTURECUBE tCubeEnvironmentMap;
TEXSAMPLER CubeEnvironmentMapSampler : register(SAMPLER(SAMPLER_CUBEENVMAP)) = sampler_state
{
	Texture = (tCubeEnvironmentMap);
	
#ifdef ENGINE_TARGET_DX10
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = WRAP;
	AddressV = WRAP;
#endif
};




#ifdef FXC_LEGACY_COMPILE
#	define SPHEREMAP_SAMPLER_REGISTER	SAMPLER(SAMPLER_DIFFUSE)
#else
#	define SPHEREMAP_SAMPLER_REGISTER	SAMPLER(SAMPLER_SPHEREENVMAP)
#endif

TEXTURE2D tSphereEnvironmentMap;
TEXSAMPLER SphereEnvironmentMapSampler : register(SPHEREMAP_SAMPLER_REGISTER) = sampler_state
{
	Texture = (tSphereEnvironmentMap);

#ifdef ENGINE_TARGET_DX10
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = WRAP;
	AddressV = WRAP;
#endif
};




// CHB 2007.02.15
TEXSHARED TEXTURE2D tParticleLightMap;
TEXSAMPLER ParticleLightMapSampler : register(SAMPLER(SAMPLER_PARTICLE_LIGHTMAP)) = sampler_state
{
	Texture = (tParticleLightMap);
	
#ifdef ENGINE_TARGET_DX10
	Filter = MIN_MAG_MIP_POINT;  //Comparison mode doesn't seem to work with compat mode
	AddressU = WRAP;
	AddressV = WRAP;
	ComparisonFunc = Less_Equal;
#endif
};





TEXSHARED TEXTURE2D tShadowMapDepth;
TEXSHARED TEXTURE2D tShadowMap;
TEXSAMPLER ShadowMapSampler : register(SAMPLER(SAMPLER_SHADOW)) = sampler_state
{
	Texture = (tShadowMap);
	
#ifdef ENGINE_TARGET_DX10
	Filter = MIN_MAG_MIP_POINT;  //Comparison mode doesn't seem to work with compat mode
	AddressU = CLAMP;
	AddressV = CLAMP;
	ComparisonFunc = Less_Equal;
#endif
};
TEXSAMPLER ColorShadowMapSampler : register(SAMPLER(SAMPLER_SHADOW)) = sampler_state
{
	Texture = (tShadowMap);
	
#ifdef ENGINE_TARGET_DX9
    MipFilter = POINT;
    MinFilter = POINT;
    MagFilter = POINT;
#endif
#ifdef ENGINE_TARGET_DX10
	Filter = MIN_MAG_MIP_POINT;  //Comparison mode doesn't seem to work with compat mode
	AddressU = CLAMP;
	AddressV = CLAMP;
	ComparisonFunc = Less_Equal;
#endif
};




TEXSAMPLER ShadowMapDepthSampler : register(SAMPLER(SAMPLER_SHADOWDEPTH)) = sampler_state
{
	Texture = (tShadowMapDepth);
	
#ifdef ENGINE_TARGET_DX10
	Filter = MIN_MAG_MIP_POINT;  //Comparison mode doesn't seem to work with compat mode
	AddressU = CLAMP;
	AddressV = CLAMP;
	ComparisonFunc = Less_Equal;
#endif
};
TEXSAMPLER ExtraColorShadowMapSampler : register(SAMPLER(SAMPLER_SHADOWDEPTH)) = sampler_state
{
	Texture = (tShadowMapDepth);
#ifdef ENGINE_TARGET_DX9
    MipFilter = POINT;
    MinFilter = POINT;
    MagFilter = POINT;
#endif
	
#ifdef ENGINE_TARGET_DX10
	Filter = MIN_MAG_MIP_POINT;  //Comparison mode doesn't seem to work with compat mode
	AddressU = CLAMP;
	AddressV = CLAMP;
	ComparisonFunc = Less_Equal;
#endif
};




TEXSHARED TEXTURE2D tSpecularLookup;
TEXSAMPLER SpecularLookupSampler : register(SAMPLER(SAMPLER_SPECLOOKUP)) = sampler_state
{
	Texture = (tSpecularLookup);
	
#ifdef ENGINE_TARGET_DX10
	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = CLAMP;
	AddressV = CLAMP;
#endif
};

#ifdef ENGINE_TARGET_DX10
TEXSAMPLER SpecularLookupSampler1D = sampler_state
{
	Texture = (tSpecularLookup);
	

	Filter = MIN_MAG_MIP_LINEAR;
	AddressU = CLAMP;
	AddressV = CLAMP;

};
#else
#	define SpecularLookupSampler1D SpecularLookupSampler
#endif

#ifdef ENGINE_TARGET_DX10
shared sampler FILTER_MIN_MAG_MIP_POINT_CLAMP : register( SAMPLER( SAMPLER_SHADOWDEPTH ) ) 
{
	Filter = MIN_MAG_MIP_POINT;
    AddressU = Clamp;
    AddressV = Clamp;	
};

shared SamplerComparisonState  FILTER_PCF : register( SAMPLER( SAMPLER_SHADOW ) )
{
    AddressU = Clamp;
    AddressV = Clamp;
    Filter = COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
    ComparisonFunc = Less_Equal;
};

shared sampler FILTER_MIN_MAG_MIP_LINEAR_CLAMP : register( SAMPLER( SAMPLER_DX10_FULLSCREEN ) )
{
	Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Clamp;
    AddressV = Clamp;	
};
#endif





//--------------------------------------------------------------------------------------------
// TEXTURE READ FUNCTIONS
//--------------------------------------------------------------------------------------------



//--------------------------------------------------------------------------------------------

half3 ReadNormalMap( half2 vCoords )
{
#ifdef ENGINE_TARGET_DX10

	// BC5 compresses 2-channels (x & y), so we need to calculate Z.
	// CHB 2007.09.13 - Looks like 'g' maps to 'x' and 'r' maps to 'y'.
	half3 N	= tex2D( NormalMapSampler, vCoords ).grb * half(2) - half(1);

#else

	// uses the DXT5 normal map 2-channel compression method -- calculates Z	
	half3 N	= tex2D( NormalMapSampler, vCoords ).agb * half(2) - half(1);

#endif	

	UNPACK_XY_NORMAL( N );

	// CML 2007.12.20 - Must normalize following Normal Map texture read.
	return normalize(N);
}


//--------------------------------------------------------------------------------------------

half2 ReadDuDvMap( sampler DuDvSampler, half2 vCoords )
{
#ifdef ENGINE_TARGET_DX10

	// BC5 compresses 2-channels (x & y), so we need to calculate Z.
	// CHB 2007.09.13 - Looks like 'g' maps to 'x' and 'r' maps to 'y'.
	half2 dUdV = tex2D( DuDvSampler, vCoords ).gr * half(2) - half(1);

#else

	// uses the DXT5 normal map 2-channel compression method
	// stores red in alpha, hence the swizzle	
	half2 dUdV = tex2D( DuDvSampler, vCoords ).ag * half(2) - half(1);

#endif	

	return dUdV;
}

//--------------------------------------------------------------------------------------------

half3 ReadDuDvMapWithAlpha( sampler DuDvSampler, half2 vCoords )
{
	// NOTE: DXT5 NM and BC5 not supported by this function. Instead, use ReadDuDvMap().
	half3 dUdVwA = tex2D( DuDvSampler, vCoords ).rga;
	dUdVwA.rg = dUdVwA.rg * half(2) - half(1);
	return dUdVwA;
}