//----------------------------------------------------------------------------
// dxC_environment.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DXC_ENVIRONMENT__
#define __DXC_ENVIRONMENT__

#include "e_environment.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define SKYBOX_FAR_PLANE_PCT		1.f

//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

namespace FSSE
{
	struct RENDER_CONTEXT;
}

struct SKYBOX_CALLBACK_DATA
{
	struct FSSE::RENDER_CONTEXT *		pContext;
	int									nSkyboxPass;
};

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

PRESULT dx9_RenderSkybox( int nDrawList, D3D_DRAWLIST_COMMAND * pCommand );
PRESULT dxC_SkyboxRenderCallback( void * pUserData );

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------
template< int O > const SH_COEFS_RGB<O> * dx9_EnvGetBackgroundSHCoefs( const ENVIRONMENT_DEFINITION * pEnvDef, BOOL bLinearSpace );
template< int O > const SH_COEFS_RGB<O> * dx9_EnvGetAppearanceSHCoefs( const ENVIRONMENT_DEFINITION * pEnvDef, BOOL bLinearSpace );

template<> inline const SH_COEFS_RGB<2> * dx9_EnvGetBackgroundSHCoefs< 2 > ( const ENVIRONMENT_DEFINITION * pEnvDef, BOOL bLinearSpace )
{
	return bLinearSpace ? &pEnvDef->tBackgroundSHCoefsLin_O2 : &pEnvDef->tBackgroundSHCoefs_O2;
}

template<> inline const SH_COEFS_RGB<3> * dx9_EnvGetBackgroundSHCoefs< 3 > ( const ENVIRONMENT_DEFINITION * pEnvDef, BOOL bLinearSpace )
{
	return bLinearSpace ? &pEnvDef->tBackgroundSHCoefsLin_O3 : &pEnvDef->tBackgroundSHCoefs_O3;
}

template<> inline const SH_COEFS_RGB<2> * dx9_EnvGetAppearanceSHCoefs< 2 > ( const ENVIRONMENT_DEFINITION * pEnvDef, BOOL bLinearSpace )
{
	return bLinearSpace ? &pEnvDef->tAppearanceSHCoefsLin_O2 : &pEnvDef->tAppearanceSHCoefs_O2;
}

template<> inline const SH_COEFS_RGB<3> * dx9_EnvGetAppearanceSHCoefs< 3 > ( const ENVIRONMENT_DEFINITION * pEnvDef, BOOL bLinearSpace )
{
	return bLinearSpace ? &pEnvDef->tAppearanceSHCoefsLin_O3 : &pEnvDef->tAppearanceSHCoefs_O3;
}

#endif // __DXC_ENVIRONMENT__