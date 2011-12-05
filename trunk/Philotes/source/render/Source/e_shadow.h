//----------------------------------------------------------------------------
// e_shadow.h
//
// - Header for shadow function/structures
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __E_SHADOW__
#define __E_SHADOW__



//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

class OptionState;	// CHB 2007.02.26

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

inline BOOL e_ShadowsActive()
{
	extern BOOL gbShadowsActive;
	return gbShadowsActive;
}

inline void e_ShadowsSetActive( BOOL bActive )
{
	extern BOOL gbShadowsActive;
	gbShadowsActive = bActive;
}

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

PRESULT e_ShadowsCreate();
PRESULT e_ShadowBufferRemove( int nShadowBufferID );
PRESULT e_ShadowBufferRemoveAll();
float e_ShadowBufferGetCosTheta( int nShadowBufferID );
PRESULT e_ShadowBufferGetLightPosInWorldView( int nShadowBufferID, const struct MATRIX * pWorldView, struct VECTOR4 * pvOutPos );
PRESULT e_ShadowBufferGetLightDirInWorldView( int nShadowBufferID, const struct MATRIX * pWorldView, struct VECTOR4 * pvOutDir );
PRESULT e_UpdateShadowBuffers();
PRESULT e_UpdateParticleLighting(void);	// CHB 2007.02.17
#if 0	// CHB 2007.02.17 - Not used.
PRESULT e_ShadowBufferSetDirectionalLightParams( int nShadowBufferID, const VECTOR& vFocus, const VECTOR& vDir, float fScale );
#endif
PRESULT e_ShadowBufferSetPointLightParams( int nShadowBufferID, float fFOV, struct VECTOR &vPos, struct VECTOR &vLookAt );
PRESULT e_PCSSSetLightSize(float fSize, float fScalegfPCSSLightSize );
bool e_UseShadowMaps(void);
bool e_ShadowsEnabled(void);
bool e_IsShadowTypeAvailable(const OptionState * pState, int nType);	// CHB 2007.02.22
int e_GetActiveShadowType(void);	// CHB 2007.02.27

int e_GetActiveShadowTypeForEnv(
	BOOL bForceArticulated,
	ENVIRONMENT_DEFINITION * pEnvDef = NULL );

#endif // __E_SHADOW__
