//----------------------------------------------------------------------------
// e_viewer.h
//
// - Header for engine-API Viewer code
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __E_VIEWER__
#define __E_VIEWER__

#include "e_dpvs.h"

#include "e_viewer_render.h"

namespace FSSE
{;

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define ENABLE_VIEWER_RENDER

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Include viewer render type enum
#define INCLUDE_VR_ENUM
enum VIEWER_RENDER_TYPE
{
	VIEWER_RENDER_INVALID = -1,
#	include "e_viewer_def.h"
	NUM_VIEWER_RENDER_TYPES
};

enum VIEWER_RENDER_FLAGBIT
{
	VIEWER_FLAGBIT_SKYBOX = 0,
	VIEWER_FLAGBIT_BEHIND,
	VIEWER_FLAGBIT_PARTICLES,
	VIEWER_FLAGBIT_ALLOW_STALE_CAMERA,
	VIEWER_FLAGBIT_IGNORE_GLOBAL_CAMERA_PARAMS,
	//
	_VIEWER_NUM_FLAGBITS
};

//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

// General gameplay camera viewer ID
extern int gnMainViewerID;

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

PRESULT e_ViewersInit();
PRESULT e_ViewersDestroy();
PRESULT e_ViewerCreate( int & nID );
PRESULT e_ViewerRemove( int nID );
BOOL e_ViewerExists( int nViewerID );
PRESULT e_ViewerSetRenderType( int nViewerID, VIEWER_RENDER_TYPE eRenderType );
PRESULT e_ViewerSetRenderFlag( int nViewerID, VIEWER_RENDER_FLAGBIT eRenderFlagbit, BOOL bValue );

// Call at the end of the frame to clear lists for the next frame
PRESULT e_ViewersResetAllLists();

// Update the camera position and view once per frame
PRESULT e_ViewerUpdateCamera(
	int nViewerID,
	const struct CAMERA_INFO * pCameraInfo,
	int nRoomID,
	int nCellID );

// Set a user-defined viewport
PRESULT e_ViewerSetCustomViewport( int nViewerID, const struct E_RECT & tRect );

// Set the viewport to the full render target
PRESULT e_ViewerSetFullViewport( int nViewerID );

// Set an override game window
PRESULT e_ViewerSetGameWindow( int nViewerID, const struct GAME_WINDOW * ptGameWindow );

// Set the default draw layer for this viewer
PRESULT e_ViewerSetDefaultDrawLayer( int nViewerID, DRAW_LAYER eDefaultDrawLayer );

// Add a model to cull and render
PRESULT e_ViewerAddModel( int nViewerID, int nModelID );
PRESULT e_ViewerAddModels( int nViewerID, SIMPLE_DYNAMIC_ARRAY<int>* pModels );
PRESULT e_ViewerAddAllModels( int nViewerID );

// Perform visibility culling
PRESULT e_ViewerSelectVisible( int nViewerID );

PRESULT e_ViewersGetStats( WCHAR * pwszStr, int nBufLen );
PRESULT e_ViewerGetCameraRoom( int nViewerID, int & nRoomID );

PRESULT e_ViewerGetSwapChainIndex( int nViewerID, int & nSwapChainIndex );

PRESULT e_ViewersResizeScreen();

}; // namespace FSSE

#endif // __E_VIEWER__
