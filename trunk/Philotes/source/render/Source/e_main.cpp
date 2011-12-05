//----------------------------------------------------------------------------
// e_main.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"
#include "appcommontimer.h"
#include "camera_priv.h"
#include "e_screenshot.h"

#include "e_main.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

PFN_GET_GAME_TIME_FLOAT gpfn_GameGetTimeFloat	= NULL;

// engine real-time timer
DWORD					gdwLastTicks			= 0;
float					gfElapsedSecs			= 0.f;

// engine current-time timer (respects pauses)
TIME					gtLastCurrentTime		= TIME_ZERO;
float					gfElapsedCurrentSecs	= 0.f;


PFN_PARTICLEDRAW	gpfn_ParticlesDraw	= NULL;
PFN_PARTICLEDRAW	gpfn_ParticlesDrawLightmap = NULL;
PFN_PARTICLEDRAW	gpfn_ParticlesDrawPreLightmap = NULL;
PFN_TOOLNOTIFY		gpfn_ToolUpdate		= NULL;
PFN_TOOLNOTIFY		gpfn_ToolRender		= NULL;
PFN_TOOLNOTIFY		gpfn_ToolDeviceLost	= NULL;



static DWORD		sgdwOverrideFlags	= 0;

static MATRIX		sgmOverrideViewMatrix;
static VECTOR		sgvOverrideEyeLocation;
static VECTOR		sgvOverrideEyeDirection;
static VECTOR		sgvOverrideEyeLookat;


//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// IMPLEMENTATION
//----------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_GetWorldViewProjMatricies( 
	MATRIX *pmatWorld, 
	MATRIX *pmatView, 
	MATRIX *pmatProj, 
	MATRIX *pmatProj2,
	BOOL bAllowStaleData )
{
	e_InitMatrices( pmatWorld, pmatView, pmatProj, pmatProj2, NULL, NULL, NULL, bAllowStaleData );
	return S_OK;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_GetWorldViewProjMatrix( MATRIX *pmOut, BOOL bAllowStaleData /*= FALSE*/)
{
	MATRIX mWorld;
	MATRIX mView;
	MATRIX mProj;
	e_InitMatrices( & mWorld, & mView, & mProj, NULL, NULL, NULL, NULL, bAllowStaleData );
	MatrixMultiply ( pmOut, & mWorld, & mView );
	MatrixMultiply ( pmOut, pmOut, & mProj );
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_GetViewDirection( VECTOR *pvOut )
{
	e_InitMatrices( NULL, NULL, NULL, NULL, pvOut );
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_GetViewPosition( VECTOR *pvOut )
{
	e_InitMatrices( NULL, NULL, NULL, NULL, NULL, pvOut );
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_GetViewFocusPosition( VECTOR *pvOut )
{
	e_InitMatrices( NULL, NULL, NULL, NULL, NULL, NULL, pvOut );
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

float e_GetElapsedTime()
{
	return gfElapsedSecs;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_InitElapsedTime()
{
	gdwLastTicks = AppCommonGetRealTime();
	gtLastCurrentTime = AppCommonGetCurTime();
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

float e_GetElapsedTimeRespectingPauses()
{
	return gfElapsedCurrentSecs;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

TIME e_GetCurrentTimeRespectingPauses()
{
	return gtLastCurrentTime;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_UpdateElapsedTime()
{
	if ( e_ScreenShotRepeatFrame() )
		return;


	// Timer that respects pauses
	TIME tCurTime = AppCommonGetCurTime();
	if ( gtLastCurrentTime > tCurTime )
	{
		// wraparound!
		gtLastCurrentTime = tCurTime;
	}

	gfElapsedCurrentSecs = ( tCurTime - gtLastCurrentTime ) / (float)MSECS_PER_SEC;
	gtLastCurrentTime = tCurTime;


	if ( AppCommonIsAnythingPaused() )
		return;


	// Realtime from start of app
	DWORD dwCurTicks = AppCommonGetRealTime();
	if ( gdwLastTicks > dwCurTicks )
	{
		// wraparound may have occured -- handle it cleanishly
#if ISVERSION(DEVELOPMENT)
		//ErrorDialog( "Engine timer seems to have wrapped around.  You can ignore this error but please report it.\n\nLast: %d\nCurr: %d", gdwLastTicks, dwCurTicks );
#endif
		gdwLastTicks = dwCurTicks;
	}

	gfElapsedSecs = ( dwCurTicks - gdwLastTicks ) / (float)MSECS_PER_SEC;
	gdwLastTicks = dwCurTicks;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL e_DX10IsEnabled()
{
	return DXC_9_10( FALSE, TRUE );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL e_ShouldRender()
{
	BOOL bShouldRender = TRUE;

	if ( AppIsMinimized() )
		bShouldRender = FALSE;
	if ( e_IsFullscreen() && ! AppIsActive() )
		bShouldRender = FALSE;
	if ( AppTestFlag(AF_PROCESS_WHEN_INACTIVE_BIT) )
		bShouldRender = TRUE;

	return bShouldRender;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void e_SetCurrentViewOverrides( MATRIX * pmatView, VECTOR * pvEyeLocation, VECTOR * pvEyeDirection, VECTOR * pvEyeLookat )
{
	sgdwOverrideFlags = 0;

	if ( pmatView )
	{
		sgmOverrideViewMatrix = *pmatView;
		sgdwOverrideFlags |= VIEW_OVERRIDE_VIEW_MATRIX;
	}

	if ( pvEyeLocation )
	{
		sgvOverrideEyeLocation = *pvEyeLocation;
		sgdwOverrideFlags |= VIEW_OVERRIDE_EYE_LOCATION;
	}

	if ( pvEyeDirection )
	{
		sgvOverrideEyeDirection = *pvEyeDirection;
		sgdwOverrideFlags |= VIEW_OVERRIDE_EYE_DIRECTION;
	}

	if ( pvEyeLookat )
	{
		sgvOverrideEyeLookat = *pvEyeLookat;
		sgdwOverrideFlags |= VIEW_OVERRIDE_EYE_LOOKAT;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void e_GetCurrentViewOverrides( MATRIX * pmatView, VECTOR * pvEyeLocation, VECTOR * pvEyeDirection, VECTOR * pvEyeLookat )
{
	if ( pmatView       && sgdwOverrideFlags & VIEW_OVERRIDE_VIEW_MATRIX )
		*pmatView		= sgmOverrideViewMatrix;
	if ( pvEyeLocation  && sgdwOverrideFlags & VIEW_OVERRIDE_EYE_LOCATION )
		*pvEyeLocation	= sgvOverrideEyeLocation;
	if ( pvEyeDirection && sgdwOverrideFlags & VIEW_OVERRIDE_EYE_DIRECTION )
		*pvEyeDirection	= sgvOverrideEyeDirection;
	if ( pvEyeLookat    && sgdwOverrideFlags & VIEW_OVERRIDE_EYE_LOOKAT )
		*pvEyeLookat	= sgvOverrideEyeLookat;
}
