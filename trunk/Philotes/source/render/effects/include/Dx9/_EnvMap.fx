// Copyright (c) 2003 Flagship Studios, Inc.
// 
//

//
// Functions for transforming a world normal into view space and looking up a sphere map
//

// must include _Common.fx before this file


// -------------------------------------------------------------------------------
// STRUCTS
// -------------------------------------------------------------------------------

// -------------------------------------------------------------------------------
// GLOBALS
// -------------------------------------------------------------------------------

// -------------------------------------------------------------------------------
// FUNCTIONS
// -------------------------------------------------------------------------------

half2 SphereMapGetCoords( half3 N )
{
	// transform world-space normal into camera space
	N = mul( N, View );

	half2 vCoords;

	// high quality, slower
	//vCoords.u = asin( N.x ) / half(PI) + half(0.5f);
	//vCoords.v = asin( N.y ) / half(PI) + half(0.5f);

	// low quality, faster
    vCoords.x = N.x * half(0.5f) + half(0.5f);
    vCoords.y = N.y * half(0.5f) + half(0.5f);

	return vCoords;
}

half3 SphereMapLookup( half3 N )
{
	// looking up into the env map sampler, though not into a cube map texture
	//return tex2D( SphereEnvironmentMapSampler, N.xy );
	return tex2D( SphereEnvironmentMapSampler, SphereMapGetCoords( N ) );
}

half3 GetReflectionVector( half3 V, half3 N )
{
	return reflect( V, N );
}

half3 CubeMapGetCoords( half3 V, half3 N )
{
	return GetReflectionVector( V, N );
}

half3 CubeMapGetCoords( half3 P, half3 E, half3 N )
{
	half3 V	= normalize( P - E );
	return CubeMapGetCoords( V, N );
}

half3 CubeMapLookup( half3 vEnvMapCoords )
{
	half4 vCoords;
	vCoords.xyz = vEnvMapCoords;
	vCoords.w   = gfEnvironmentMapLODBias;
	return texCUBEbias( CubeEnvironmentMapSampler, vCoords );
}

half4 EnvMapLookupCoords(
	in half3 N,
	in half3 vCubeCoords,
	in half fSpecPower,
	in half fSpecAlpha,
	uniform bool bCubeEnvironmentMap,
	uniform bool bSphereEnvironmentMap,
	uniform bool bSpecular )
{
	half4 vEnvMap = (half4)0.0f;

	if ( bCubeEnvironmentMap || gbCubeEnvironmentMap )
	{
		vEnvMap.xyz = CubeMapLookup( vCubeCoords );
	}
	else if ( bSphereEnvironmentMap || gbSphereEnvironmentMap )
	{
		vEnvMap.xyz = SphereMapLookup( N );
	}

	if ( bCubeEnvironmentMap || bSphereEnvironmentMap || 
		gbCubeEnvironmentMap || gbSphereEnvironmentMap )
	{
		vEnvMap.w   = gfEnvironmentMapPower;
		if ( bSpecular || gbSpecular )
		{
			vEnvMap.w    *= fSpecPower;
			if ( ( fSpecAlpha * gfEnvironmentMapInvertGlossThreshold ) > gfEnvironmentMapGlossThreshold )
			{
				vEnvMap.w = 0.0f;
			}
		}
	}
	
	return vEnvMap;
}

half4 EnvMapLookup(
	in half3 N,
	in half3 V,
	in half fSpecPower,
	in half fSpecAlpha,
	uniform bool bCubeEnvironmentMap,
	uniform bool bSphereEnvironmentMap,
	uniform bool bSpecular )
{
	return EnvMapLookupCoords(
		N,
		normalize( GetReflectionVector( V, N ) ),
		fSpecPower,
		fSpecAlpha,
		bCubeEnvironmentMap,
		bSphereEnvironmentMap,
		bSpecular );
}
