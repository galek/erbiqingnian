//----------------------------------------------------------------------------
// e_particle.cpp
//
// - Implementation for particle system render functions/structures
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"

#include "e_main.h"
#include "e_particles_priv.h"
#include "e_settings.h"
#include "perfhier.h"
#include "dxC_profile.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

PFN_PARTICLESYSTEM_GET	gpfn_ParticleSystemGet = NULL;
PFN_PARTICLES_GET_NEARBY_UNITS gpfn_ParticlesGetNearbyUnits = NULL;

//----------------------------------------------------------------------------
// IMPLEMENTATION
//----------------------------------------------------------------------------

void e_ParticleSystemsInitCallbacks(
	PFN_PARTICLESYSTEM_GET pfn_ParticleSystemGet,
	PFN_PARTICLES_GET_NEARBY_UNITS pfn_ParticlesGetNearbyUnits)
{
	gpfn_ParticleSystemGet = pfn_ParticleSystemGet;
	gpfn_ParticlesGetNearbyUnits = pfn_ParticlesGetNearbyUnits;
}


PRESULT e_ParticlesDraw(
	BYTE bDrawLayer,
	const MATRIX * pmatView,
	const MATRIX * pmatProj,
	const MATRIX * pmatProj2,
	const VECTOR & vEyePosition,
	const VECTOR & vEyeDirection,
	BOOL bPixelShader,
	BOOL bDepthOnly )
{
	extern PFN_PARTICLEDRAW gpfn_ParticlesDraw;

	if ( ! e_GetRenderFlag( RENDER_FLAG_PARTICLES_ENABLED ) || ! gpfn_ParticlesDraw )
		return S_FALSE;

	PERF(DRAW_PART);
	D3D_PROFILE_REGION( L"Particles" );

	return gpfn_ParticlesDraw( bDrawLayer, pmatView, pmatProj, pmatProj2, vEyePosition, vEyeDirection, bPixelShader, bDepthOnly );
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

DWORD e_GetColorFromPath( 
	float fPathTime, 
	const CInterpolationPath<CFloatTriple> * pPath )
{
	CFloatTriple tColorTriple = pPath->GetValue( fPathTime );
	return ARGB_MAKE_FROM_FLOAT( tColorTriple.x, tColorTriple.y, tColorTriple.z, 0.0f);
}