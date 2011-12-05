//----------------------------------------------------------------------------
// demolevel.h
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifndef _DEMOLEVEL_H_
#define _DEMOLEVEL_H_

#include "e_demolevel.h"

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

#define DEMO_LEVEL_VERSION	4

#define DEMO_LEVEL_COUNTDOWN_SECONDS	2



// ***************************************************************************
// Uncomment to display debug information
//#define DEMO_LEVEL_DISPLAY_DEBUG_INFO
// ***************************************************************************



//----------------------------------------------------------------------------
// FORWARD REFERENCES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// ENUMERATIONS
//----------------------------------------------------------------------------

enum DEMO_LEVEL_CAMERA_TYPE
{
	DLCT_INDOOR = 0,
	DLCT_OUTDOOR = 1,
	DLCT_INDOOR_SLIDESHOW = 2,
	DLCT_OUTDOOR_SLIDESHOW = 3,
	DLCT_INDOOR_NEW = 4,
	DLCT_OUTDOOR_NEW = 5,
	// count
	DLCT_MAX_TYPES,
	// specific
	DLCT_DEFAULT = DLCT_INDOOR,
};

//----------------------------------------------------------------------------
// STRUCTURES
//----------------------------------------------------------------------------

struct DEMO_LEVEL_DEFINITION
{
	DEFINITION_HEADER	tHeader;

	DWORD				dwFlags;
	int					nLevelDefinition;
	int					nDRLGDefinition;
	DWORD				dwDRLGSeed;
	int					nCameraType;
	float				fMetersPerSecond;
};

//----------------------------------------------------------------------------
// EXPORTED FUNCTIONS
//----------------------------------------------------------------------------

DEMO_LEVEL_DEFINITION * DemoLevelGetDefinition();
void DemoLevelFollowPath( VECTOR * pvOutPosition, VECTOR * pvOutDirection );
void DemoLevelCleanup();
PRESULT DemoLevelGeneratePath();
PRESULT DemoLevelDebugGetRoomPath( ROOMID *& pRoomPath, int & nRoomCount );
PRESULT DemoLevelDebugRenderPath( const VECTOR2 & vWCenter, float fScale, const VECTOR2 & vSCenter );
void DemoLevelSetState( FSSE::DEMO_LEVEL_STATUS::STATE eState );
void DemoLevelInterrupt();
void DemoLevelLoadScreenFadeCallback();
UNIT * DemoLevelInitializePlayer();

#endif // _DEMOLEVEL_H_