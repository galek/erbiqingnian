//----------------------------------------------------------------------------
// dxC_settings.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DXC_SETTINGS__
#define __DXC_SETTINGS__

#include "e_settings.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define DEFAULT_WINDOW_WIDTH				800
#define DEFAULT_WINDOW_HEIGHT				600

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

class OptionState;

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

DXC_FORMAT dxC_GetDefaultDepthStencilFormat();
DXC_FORMAT dxC_GetDefaultDisplayFormat( BOOL bWindowed );
int dxC_GetDisplayModeCount( DXC_FORMAT tFormat );
PRESULT dxC_EnumDisplayModes( DXC_FORMAT tFormat, UINT nIndex, E_SCREEN_DISPLAY_MODE & ModeDesc, BOOL bSilentError = FALSE );
bool e_IsCSAAAvailable();	// CML: I hate doing this, but the way we have things set up right now necessitates it
bool e_IsMultiSampleAvailable(const OptionState * pState, DXC_MULTISAMPLE_TYPE nMultiSampleType, int nQuality );	// CHB 2007.03.05
PRESULT dxC_GetDefaultMonitorHandle( HMONITOR & hMon );

#endif //__DXC_SETTINGS__
