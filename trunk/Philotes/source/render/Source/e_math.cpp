//----------------------------------------------------------------------------
// e_math.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"
#include "camera_priv.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// IMPLEMENTATION
//----------------------------------------------------------------------------

float e_GetWorldDistanceUnbiasedScreenSizeByVertFOV( float fDistance, float fSize, float fVerticalFOVRad )
{
	float fThetaRad = atan( fSize / fDistance );
	return SATURATE( fThetaRad / fVerticalFOVRad );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

float e_GetWorldDistanceUnbiasedScreenSizeByVertFOV( float fDistance, float fSize, const CAMERA_INFO * pInfo /*= NULL*/ )
{
	if ( ! pInfo )
		pInfo = CameraGetInfo();
	if ( ! pInfo )
		return 0.f;
	return e_GetWorldDistanceUnbiasedScreenSizeByVertFOV( fDistance, fSize, pInfo->fVerticalFOV );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

// CHB 2007.07.11 - Not the best place for this.
#include "e_optionstate.h"
float e_GetWorldDistanceBiasedScreenSizeByVertFOV( float fDistance, float fSize )
{
	float fScreenSize = e_GetWorldDistanceUnbiasedScreenSizeByVertFOV(fDistance, fSize);
	if (!c_GetToolMode())	// bias is for in-game settings only; no bias should be applied in Hammer
	{
		fScreenSize = e_OptionState_GetActive()->get_fLODScale() * fScreenSize + e_OptionState_GetActive()->get_fLODBias();
		fScreenSize = PIN(fScreenSize, 0.0f, 1.0f);
	}
	return fScreenSize;
}
