// Copyright (c) 2003 Flagship Studios, Inc.
// 
//

#ifndef _SHPHERICALHARMONICS_FX_
#define _SHPHERICALHARMONICS_FX_

//
// Functions for computing spherical harmonics lighting values
//

// must include _Common.fx before this file


// -------------------------------------------------------------------------------
// STRUCTS
// -------------------------------------------------------------------------------

// -------------------------------------------------------------------------------
// GLOBALS
// -------------------------------------------------------------------------------

typedef half	SHFLOAT;
typedef half3	SHFLOAT3;
typedef half4	SHFLOAT4;

SHFLOAT4	cAr;
SHFLOAT4	cAg;
SHFLOAT4	cAb;
SHFLOAT4	cBr;
SHFLOAT4	cBg;
SHFLOAT4	cBb;
SHFLOAT4	cC;

half4 gvHemisphereLightColors[2];

// -------------------------------------------------------------------------------
// FUNCTIONS
// -------------------------------------------------------------------------------



half3 SHEval_4_RGB( half3 N )
{
	SHFLOAT3 x1;

	half4 N_1 = half4( N.xyz, 1.f );

    // Linear + constant polynomial terms
	x1.r = dot( cAr, N_1 );
	x1.g = dot( cAg, N_1 );
	x1.b = dot( cAb, N_1 );

	return x1;
}




half3 SHEval_9_RGB( half3 N )
{
	SHFLOAT3 x1, x2, x3;

	half4 N_1 = half4( N.xyz, 1.f );

    // Linear + constant polynomial terms
	x1.r = dot( cAr, N_1 );
	x1.g = dot( cAg, N_1 );
	x1.b = dot( cAb, N_1 );

    // 4 of the quadratic polynomials
	SHFLOAT4 vB = N.xyzz * N.yzzx;
	x2.r = dot( cBr, vB );
	x2.g = dot( cBg, vB );
	x2.b = dot( cBb, vB );

    // Final quadratic polynomial
	SHFLOAT vC = N.x*N.x - N.y*N.y;
	x3 = cC.rgb * vC;

	return x1 + x2 + x3;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------


half3 HemisphereLightEx( half3 N, half3 L )
{
	half fDot = dot( N, L );
	return lerp( gvHemisphereLightColors[1], gvHemisphereLightColors[0], fDot * half(0.5f) + half(0.5f) );
}

half3 HemisphereLight( half3 N, uniform int nSpace )
{
	half3 vDir;
	if ( nSpace == OBJECT_SPACE )
		vDir = DirLightsDir(OBJECT_SPACE)[ DIR_LIGHT_0_DIFFUSE ];
	else
		vDir = DirLightsDir(WORLD_SPACE)[ DIR_LIGHT_0_DIFFUSE ];
	
	return HemisphereLightEx( N, vDir );
}


#endif // _SHPHERICALHARMONICS_FX_
