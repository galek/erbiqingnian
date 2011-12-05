//----------------------------------------------------------------------------
// e_shadow.cpp
//
// - Implementation for shadow functions/structures
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"
#include "e_shadow.h"
#include "e_optionstate.h"
#include "Dx9/dx9_shadowtypeconstants.h"
#include "e_settings.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

BOOL gbShadowsActive = FALSE;

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// IMPLEMENTATION
//----------------------------------------------------------------------------

int e_GetActiveShadowType(void)
{
	return e_OptionState_GetActive()->get_nShadowType();
}

bool e_ShadowsEnabled(void)
{
	if ( ! e_GetRenderFlag( RENDER_FLAG_SHADOWS_ENABLED ) )
		return FALSE;
	return e_GetActiveShadowType() != SHADOW_TYPE_NONE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

int e_GetActiveShadowTypeForEnv(
	BOOL bForceArticulated,
	ENVIRONMENT_DEFINITION * pEnvDef /*= NULL*/ )
{
	int nActiveType = e_GetActiveShadowType();
	if ( nActiveType == SHADOW_TYPE_NONE )
		return nActiveType;

	if ( bForceArticulated )
		return nActiveType;

	if ( e_EnvUseBlobShadows( pEnvDef ) )
		return SHADOW_TYPE_NONE;
	return nActiveType;
}
