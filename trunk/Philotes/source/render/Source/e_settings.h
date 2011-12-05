//----------------------------------------------------------------------------
// e_settings.h
//
// - Header for graphical settings structure
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __E_SETTINGS__
#define __E_SETTINGS__

#include "config.h"
#include "e_texture.h"
#include "e_ui.h"
#include "appcommon.h"


//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define SETTING_NEEDS_NONE						0
#define SETTING_NEEDS_RESET						MAKE_MASK(0)
#define SETTING_NEEDS_FXRELOAD					MAKE_MASK(1)
#define SETTING_NEEDS_RESTART					MAKE_MASK(2)
#define SETTING_NEEDS_WINDOWSIZE				MAKE_MASK(3)
#define SETTING_NEEDS_APP_EXIT					MAKE_MASK(4)	// CHB 2007.05.10 - Must exit (and restart) the application
//#define SETTING_NEEDS_FULLSCREEN_TOGGLE			MAKE_MASK(6)

// This is the minimum vertical size we will offer as a resolution choice or select automatically.
#define SCREEN_ENUMERATE_MINIMUM_VERTICAL_SIZE	710


#define SETTINGS_DEF_WINDOWED_RES_WIDTH			800
#define SETTINGS_DEF_WINDOWED_RES_HEIGHT		600
#define SETTINGS_DEF_FULLSCREEN_ANTIALIASING	0
#define SETTINGS_DEF_FULLSCREEN_REFRESH_RATE_NUMERATOR		60
#define SETTINGS_DEF_FULLSCREEN_REFRESH_RATE_DENOMINATOR	1

#define RANDOM_WINDOW_RES_MIN_X					800
#define RANDOM_WINDOW_RES_MIN_Y					600
#define RANDOM_WINDOW_RES_MAX_X					1920
#define RANDOM_WINDOW_RES_MAX_Y					1200
#define RANDOM_WINDOW_RES_MIN_ASPECT			1.f
#define RANDOM_WINDOW_RES_MAX_ASPECT			2.f

#define SETTINGS_MIN_RES_WIDTH					800
#define SETTINGS_MIN_RES_HEIGHT					600

#define RENDERFLAG_NAME_MAX_LEN					64

#define DEFAULT_ADAPTIVE_BATCH_TARGET			300

#define PRESENT_DELAY_FRAMES_DEFAULT			4

#define MIN_GAMMA_ADJUST						0.5f
#define MAX_GAMMA_ADJUST						3.0f

//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------

#define TRACE_EVENT(pszEvent)				e_TraceEvent( pszEvent, __LINE__ )
#define TRACE_EVENT_INT(pszEvent, nInt)		e_TraceEventInt( pszEvent, nInt, __LINE__ )

#define RAD_TO_DEG( fRadians )	( fRadians * 180.0f / PI )
#define DEG_TO_RAD( fDegrees )	( fDegrees * PI / 180.0f )

#define MAP_VALUE_TO_RANGE( fromVal, fromMin, fromMax, toMin, toMax )		\
			( ( (float(fromVal) - fromMin) / (fromMax - fromMin) ) * (toMax - toMin) + toMin )

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

enum FADE_TYPE
{
	FADE_IN = 0,
	FADE_OUT,
	// 
	NUM_FADE_TYPES
};

//-------------------------------------------------------------------------------------------------
// render flags (for console commands)
#define INCLUDE_RENDERFLAG_ENUM
enum RENDER_FLAG {
	RENDER_FLAG_INVALID = -1,
#include "e_renderflag_def.h"
	NUM_RENDER_FLAGS // count
};
#undef INCLUDE_RENDERFLAG_ENUM

enum
{
	RF_COLLISION_DRAW_NONE = 0,
	RF_COLLISION_DRAW_ONLY,
	RF_COLLISION_DRAW_ON_TOP,
	// count
	NUM_COLLISION_DRAW_MODES
};

enum RENDERFLAG_TYPE
{
	RFTYPE_INVALID = -1,
	RFTYPE_BOOL = 0,
	RFTYPE_INT,
	RFTYPE_ID,
	// count
	NUM_RENDERFLAG_TYPES
};

enum
{
	DEBUGTEX_SCALE_FIT = -1,
	DEBUGTEX_SCALE_ACTUAL = 0,
};

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

typedef void (*PFN_RENDER_FLAG_ON_SET)( RENDER_FLAG eFlag, int nNewValue );

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

class OptionState;

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

// CHB 2007.10.01 - For use with e_GetWindowStyle().
struct E_WINDOW_STYLE
{
	// Inputs.
	bool bWindowedIn;
	bool bActiveStateIn;
	int nClientWidthIn;
	int nClientHeightIn;
	int nDesktopWidthIn;
	int nDesktopHeightIn;

	// Outputs.
	DWORD nStyleOut;
	DWORD nStyleMaskOut;
	DWORD nExStyleOut;
	DWORD nExStyleMaskOut;
};

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

inline BOOL e_GetRenderEnabled()
{
	extern BOOL gbRenderEnabled;
	return gbRenderEnabled;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

inline BOOL e_GetUIOnlyRender()
{
	extern BOOL gbUIOnlyRender;
	return gbUIOnlyRender;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

inline BOOL e_GetUICovered()
{
	extern BOOL gbUICovered;
	return gbUICovered;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

inline BOOL e_GetUICoveredLeft()
{
	extern BOOL gbUICoveredLeft;
	return gbUICoveredLeft;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

inline float e_GetUICoverageWidth()
{
	extern float gfUICoverageWidth;
	return gfUICoverageWidth;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

inline BOOL e_InCheapMode()
{
	extern BOOL gbCheapMode;
	return gbCheapMode;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

bool e_IsFullscreen();
int e_GetWindowWidth();
int e_GetWindowHeight();
void e_SetWindowSize( int nWidth, int nHeight );

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

inline void e_GetWindowSize( int & nWidth, int & nHeight )
{
	nWidth = e_GetWindowWidth();
	nHeight = e_GetWindowHeight();
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

inline float e_GetWindowAspect()
{
	int nWidth, nHeight;
	e_GetWindowSize( nWidth, nHeight );
	ASSERT_RETZERO( nWidth && nHeight );
	return float(nWidth) / float(nHeight);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_GetDesktopSize( int & nWidth, int & nHeight );
void e_SetDesktopSize(int nWidth, int nHeight);		// CHB 2007.08.23
void e_SetDesktopSize(void);						// CHB 2007.08.23

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

inline void e_TraceEvent(
	const char * pszEvent, 
	const int nLine )
{
#if ISVERSION(DEVELOPMENT)
	extern BOOL gbDebugTrace;
	if ( gbDebugTrace )
		DebugString( OP_DEBUG, "# [ Line: %5d ] TRACE EVENT     :   %s", nLine, pszEvent );
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

inline void e_TraceEventInt( 
	const char * pszEvent, 
	const int nInt, 
	const int nLine )
{
#if ISVERSION(DEVELOPMENT)
	extern BOOL gbDebugTrace;
	if ( gbDebugTrace )
		DebugString( OP_DEBUG, "# [ Line: %5d ] TRACE EVENT     :   %s [ %d ]", nLine, pszEvent, nInt );
#endif
}

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

PRESULT e_InitializeRenderFlags();
void e_UpdateGlobalFlags();
PRESULT e_SetRenderFlag( RENDER_FLAG eFlag, int nValue );
int e_GetRenderFlag( RENDER_FLAG eFlag );
int e_ToggleRenderFlag( RENDER_FLAG eFlag );
void e_SetToolMode( BOOL bEnabled );
void e_SetRenderEnabled( BOOL bEnabled );
void e_SetUIOnlyRender( BOOL bUIOnly );


void e_SetUICoverage( float Width,
					  BOOL CoverAll,
					  BOOL Left );

void e_SetDebugTrace( BOOL bEnabled );
BOOL e_ToggleDebugTrace();
void e_SetFogColor( DWORD dwColor );
PRESULT e_SetFogEnabled( BOOL fValue, BOOL bForce = FALSE );
BOOL e_GetFogEnabled();
void e_SetFarClipPlane( float fDistance, BOOL bForce = FALSE );
float e_GetFarClipPlane();
float e_GetNearClipPlane();
DWORD e_GetFogColor();
BOOL e_ToggleDebugRenderFlag();
BOOL e_ToggleDebugTextureFlag();
BOOL e_ToggleDebugViewFlag();
BOOL e_ToggleClipEnable();
BOOL e_ToggleScissorEnable();
void e_RenderFlagPostCommand( int nFlag );
void e_RenderFlagPostCommandEngine( int nFlag );
void e_ToggleFullscreen( int nWidth = -1, int nHeight = -1 );
PRESULT e_SetFullscreen( BOOL bFullscreen, int nWidth, int nHeight, int nRefreshRate = 0 );
BOOL e_ToggleWireFrame();
int e_ToggleCollisionDraw();
BOOL e_ToggleHullDraw();
BOOL e_ToggleClipDraw();
BOOL e_ToggleDebugDraw();
void e_SetFogDistances( float fNearDistance, float fFarDistance, BOOL bForce = FALSE );
void e_GetFogDistances( float & fNearDistance, float & fFarDistance );
void e_SetFogMaxDensity( float fPercent );
void e_StartCheapMode();
void e_EndCheapMode();
void e_DisableVisibleRender();
void e_EnableVisibleRender();
PRESULT e_SetWindowSizeEx( int width, int height );
PRESULT e_SetWindowSizeEx2( int width, int height, int nRefreshRate );	// CHB 2007.01.25
PRESULT e_InitWindowSize(OptionState * pState);
PRESULT e_NotifyExternalDisplayChange(int nWidth, int nHeight);	// CHB 2007.07.26
void e_PreNotifyWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);	// CHB 2007.09.26
void e_NotifyWindowMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);	// CHB 2007.08.29
void e_SetChangingDevice(bool bChanging);	// CHB 2007.08.23
const char * e_GetDisplayModeString();
const char * e_GetDisplaySettingsString();
PRESULT e_GetFadeColor( DWORD & dwColor );
PRESULT e_BeginFade( FADE_TYPE eType, unsigned int nDurationMS, DWORD dwFadeColor, void (*pfnFadeEndCallback)() = NULL );
PRESULT e_UpdateFade();
PRESULT e_SetSolidColorRenderEnabled( BOOL bEnabled );
PRESULT e_SetSolidColorRenderColor( DWORD dwColor );
void e_UpdateDistanceSpring( int nBatchCount );
void e_SetupDistanceSpring();
float e_GetDistanceSpringCoef();
int e_GetRenderFlagCount();
const char * e_GetRenderFlagName( int nFlag );
RENDERFLAG_TYPE e_GetRenderFlagType( RENDER_FLAG nFlag );
PRESULT e_SetMaxDetail( int nMinDetail );
int e_GetMaxDetail();
PRESULT e_ResetClipAndFog();
PRESULT e_SettingsCommitActionFlags( DWORD dwActionFlags, BOOL bForceSync = FALSE );
bool e_IsGlowAvailable(const OptionState * pState);
PRESULT e_DetectDefaultFullscreenResolution( struct E_SCREEN_DISPLAY_MODE & tMode, OptionState* pState );
void e_PresentDelaySet( DWORD dwFrames = PRESENT_DELAY_FRAMES_DEFAULT );
void e_PresentDelayUpdate();
BOOL e_PresentEnabled();

PRESULT e_DoWindowSize(
	int nCurWidth = -1,
	int nCurHeight = -1 );
void e_UpdateWindowStyle(void);	// CHB 2007.10.01
bool e_IsInternalWindowSizeEvent(void);	// CHB 2007.09.26
void e_FrameBufferDimensionsToWindowSize(int & nWidthInOut, int & nHeightInOut);
const E_WINDOW_STYLE e_GetWindowStyle(void);
void e_CenterWindow(int nWidth, int nHeight, int & nPosXOut, int & nPosYOut);	// returns screen x,y of top left corner of window given its dimensions
bool e_ResolutionShouldEnumerate( const struct E_SCREEN_DISPLAY_MODE & tMode );

#endif // __E_SETTINGS__
