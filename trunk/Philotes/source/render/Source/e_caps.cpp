//----------------------------------------------------------------------------
// e_caps.cpp
//
// - Implementation for capabilities structure
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"
#include "e_caps.h"
#include "e_optionstate.h"	// CHB 2007.03.02

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

EngineCap gtCaps[ NUM_ENGINE_CAPS ] =
{
	{ MAKEFOURCC('D','S','B','L'), "Disabled", FALSE },		// CAP_DISABLE stub
	#include "e_caps_def.h"
};

DWORD gdwCapValues[ NUM_ENGINE_CAPS ];

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// IMPLEMENTATION
//----------------------------------------------------------------------------

PRESULT e_GatherCaps()
{
	// in theory, generic caps could be loaded here
	// however, even the generic caps tend to have a platform-specific detection method, so do them all in GatherPlatformCaps
	V_RETURN( e_GatherPlatformCaps() );
	V_RETURN( e_OptionState_InitCaps() );	// CHB 2007.03.02
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_DebugOutputCaps()
{
	DebugString( OP_DEBUG, "Engine Caps" );

	for ( int i = CAP_DISABLE+1; i < NUM_ENGINE_CAPS; i++ )
	{
		CHAR szFourCC[ FOURCCSTRING_LEN ];
		e_CapGetFourCCString( (ENGINE_CAP)i, szFourCC, FOURCCSTRING_LEN );
		const CHAR * pszDesc = e_CapGetDescription( (ENGINE_CAP)i );
		DWORD dwValue = e_CapGetValue( (ENGINE_CAP)i );
		DebugString( OP_DEBUG, "%4s - 0x%08x - %10d - %s", szFourCC, dwValue, dwValue, pszDesc );
		LogMessage( "%4s - 0x%08x - %10d - %s", szFourCC, dwValue, dwValue, pszDesc );
	}

	e_DebugOutputPlatformCaps();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_CapSetValue( const ENGINE_CAP eCap, const DWORD dwValue )
{
	ASSERT_RETINVALIDARG( IS_VALID_CAP_TYPE( eCap ) );
	gdwCapValues[ eCap ] = dwValue;
	return S_OK;
}