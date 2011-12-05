//----------------------------------------------------------------------------
// dxC_settings.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"

#include "dxC_texture.h"
#include "dxC_settings.h"
#include "dxC_caps.h"
#include "dxC_state.h"
#include "dxC_main.h"
#include "..\\dx9\\dx9_settings.h"
#include "e_optionstate.h"	// CHB 2007.02.28
#include "dxC_state_defaults.h"
#include "e_viewer.h"
#include "e_screen.h"
#include "e_gamma.h"		// CHB 2007.10.02
#include "dxC_pixo.h"

#if !defined( _WIN64 ) && defined( PROFILE )
//#define PROFILE_FADE
#endif

#ifdef PROFILE_FADE
#define VSPERF_NO_DEFAULT_LIB
#include "vsperf.h"
#endif

#undef min	// some things really shouldn't be macros
#undef max	// some things really shouldn't be macros
#include <limits>

#if defined(ENGINE_TARGET_DX10)
//DX10 BB Format is hardwired
#define DX10_BBFORMAT								DXGI_FORMAT_R8G8B8A8_UNORM
#endif

using namespace FSSE;

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define NUM_DISPLAY_FORMATS			DXC_9_10( 6, 4 )

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

DXC_FORMAT gtDisplayFormatList[ NUM_DISPLAY_FORMATS ] =
{
	D3DFMT_A8R8G8B8,
	D3DFMT_X8R8G8B8,
#ifdef ENGINE_TARGET_DX9
	D3DFMT_A2R10G10B10,
	D3DFMT_X1R5G5B5,
#endif
	D3DFMT_A1R5G5B5,
	D3DFMT_R5G6B5
};

char gtDisplayFormatListStrings[ NUM_DISPLAY_FORMATS + 1 ][ 16 ] =
{
	"A8R8G8B8",
	"X8R8G8B8",
#ifdef ENGINE_TARGET_DX9
	"A2R10G10B10",
	"X1R5G5B5",
#endif
	"A1R5G5B5",
	"R5G6B5",
	"unknown"
};

const DXC_FORMAT gtAutoDepthStencilFormat = D3DFMT_D24S8;

// FULL-SCREEN FADING
static TIME sgtFadeStart			= TIME_ZERO;
static TIMEDELTA sgtFadeDuration	= 0;
//static float sgfFadePhase			= 0.f;
//static float sgfFadeRate			= 0.f;
static DWORD sgdwFadeColor			= 0x00000000;
static BOOL sgbFading				= FALSE;
static FADE_TYPE sgeFadeType		= FADE_TYPE(0);
void (*gpfnFadeEndCallback)()		= NULL;

static BOOL sgbShowSolidColor		= FALSE;
static DWORD sgdwSolidColor			= 0x00000000;

// PRESENT DELAY
static DWORD sgdwPresentDelayFrames = 0;

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------
#if defined(ENGINE_TARGET_DX9)
	//extern D3DDISPLAYMODE dx9_FindLargestScreenSizeAndFormat();
	//extern D3DDISPLAYMODE dx9_FindBestScreenFormat( int nWidth, int nHeight );
#elif defined(ENGINE_TARGET_DX10)
	extern DXGI_MODE_DESC dx10_FindLargestScreenSizeAndFormat();
#endif


//----------------------------------------------------------------------------
// IMPLEMENTATIONS
//----------------------------------------------------------------------------

DXC_FORMAT dxC_GetDefaultDepthStencilFormat()
{
	return gtAutoDepthStencilFormat;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//static PRESULT sGetDisplayMonitorInfo(int nDeviceIndex)
//{
//	FARPROC EnumDisplayDevices;
//	HINSTANCE  hInstUser32;
//	DISPLAY_DEVICE DispDev; 
//	char szSaveDeviceName[33];	// 32 + 1 for the null-terminator
//	PRESULT prRet = E_FAIL;
//
//	hInstUser32 = LoadLibrary("c:\\windows\User32.DLL");
//	if (!hInstUser32)
//		return E_FAIL;  
//
//	// Get the address of the EnumDisplayDevices function
//	EnumDisplayDevices = (FARPROC)GetProcAddress(hInstUser32,"EnumDisplayDevicesA");
//	if (!EnumDisplayDevices)
//	{
//		FreeLibrary(hInstUser32);
//		return E_FAIL;
//	}
//
//	ZeroMemory(&DispDev, sizeof(DispDev));
//	DispDev.cb = sizeof(DispDev); 
//
//	// After the first call to EnumDisplayDevices, 
//	// DispDev.DeviceString is the adapter name
//	if (EnumDisplayDevices(NULL, nDeviceIndex, &DispDev, 0)) 
//	{  
//		V_RETURN( StringCchCopy(szSaveDeviceName, 33, DispDev.DeviceName) );
//
//		// After second call, DispDev.DeviceString is the 
//		// monitor name for that device 
//		EnumDisplayDevices(szSaveDeviceName, 0, &DispDev, 0);   
//
//		// In the following, lpszMonitorInfo must be 128 + 1 for 
//		// the null-terminator.
//		//hr = StringCchCopy(lpszMonitorInfo, 129, DispDev.DeviceString);
//		//if (FAILED(hr))
//		//{
//		//	// TODO: write error handler
//		//}
//
//		prRet = S_OK;
//	}
//
//	FreeLibrary(hInstUser32);
//
//	return prRet;
//}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

DXC_FORMAT dxC_GetDefaultDisplayFormat( BOOL bWindowed )
{
#if defined(ENGINE_TARGET_DX9)
	//if ( dxC_CanUsePixomatic() )
	//{
	//	//V( sGetDisplayMonitorInfo( 0 ) );
	//	return D3DFMT_X8R8G8B8;
	//}

	if ( ! bWindowed )
	{
		// iterate a list of possible fullscreen formats
		DXC_FORMAT tFormats[] = 
		{
			D3DFMT_A8R8G8B8,
			D3DFMT_X8R8G8B8,
			D3DFMT_A1R5G5B5,
			D3DFMT_X1R5G5B5,
			D3DFMT_R5G6B5,
			//D3DFMT_A8B8G8R8,
			//D3DFMT_X8B8G8R8,
		};
		for ( unsigned nFmt = 0; nFmt < _countof(tFormats); nFmt++ )
		{
			if ( dxC_GetDisplayModeCount( tFormats[ nFmt ] ) > 0 )
			{
				return tFormats[ nFmt ];
			}
		}
		ASSERTV( false, "WTF?" );
		return D3DFMT_A8R8G8B8;		// desperation
	}
	else
	{
		return D3DFMT_X8R8G8B8;
	}
#elif defined(ENGINE_TARGET_DX10)
	return DX10_BBFORMAT;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

// CHB 2007.03.01 - Uses current adapter ordinal.
int dxC_GetDisplayModeCount( DXC_FORMAT tFormat )
{
#ifdef ENGINE_TARGET_DX9
	DXCADAPTER tAdapter = dxC_GetAdapter();
	return dx9_GetD3D()->GetAdapterModeCount( tAdapter, tFormat );
#elif defined( ENGINE_TARGET_DX10 )
	UINT nCount = 0;
	V( dx10_GetDisplayModeList( tFormat, &nCount, NULL ) );
	return nCount;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_EnumDisplayModes( DXC_FORMAT tFormat, UINT nIndex, E_SCREEN_DISPLAY_MODE & ModeDesc, BOOL bSilentError )
{
#if defined(ENGINE_TARGET_DX9)
	D3DDISPLAYMODE dm;
	PRESULT pr = dx9_GetD3D()->EnumAdapterModes( dxC_GetAdapter(), tFormat, nIndex, &dm );
	if ( pr == D3DERR_INVALIDCALL )
	{
		if ( ! bSilentError )
			ErrorDialog( "Unsupported adapter: %d", dxC_GetAdapter() );
		return pr;
	}
	if ( pr == D3DERR_NOTAVAILABLE )
	{
		if ( ! bSilentError )
		{
			char szFormat[32];
			dxC_TextureFormatGetDisplayStr( tFormat, szFormat, 32 );
			ErrorDialog( "Unsupported back-buffer format: %s", szFormat );
		}
		return pr;
	}
	if ( FAILED(pr) )
	{
		if ( ! bSilentError )
			ErrorDialog( "Unspecified error: %d: %s", pr, e_GetResultString( pr ) );
		return pr;
	}
	ModeDesc = dxC_ConvertNativeDisplayMode(dm);
#elif defined(ENGINE_TARGET_DX10)
	//This is not efficient but it works for now
	UINT nCount = dxC_GetDisplayModeCount( tFormat );
	if( nCount < nIndex )
		return E_FAIL;

	auto_ptr<DXGI_MODE_DESC> pDisplayModeArray(new DXGI_MODE_DESC[nCount]);
	V_RETURN(dx10_GetDisplayModeList( tFormat, &nCount, pDisplayModeArray.get() ));
	if( nCount < nIndex )
		return E_FAIL;
	ModeDesc = dxC_ConvertNativeDisplayMode(pDisplayModeArray.get()[nIndex]);
#endif
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------


static BOOL sValidateMode( E_SCREEN_DISPLAY_MODE & tMode )
{
	unsigned int nCount = dxC_GetDisplayModeCount( (DXC_FORMAT)tMode.Format );

	E_SCREEN_DISPLAY_MODE tEnumDesc;
	for ( unsigned int i = 0; i < nCount; i++ )
	{
		V_CONTINUE( dxC_EnumDisplayModes( (DXC_FORMAT)tMode.Format, i, tEnumDesc ) );

		//if ( ! e_ResolutionShouldEnumerate( tEnumDesc ) )
		//	continue;

		if ( tEnumDesc.Width != tMode.Width )
			continue;
		if ( tEnumDesc.Height != tMode.Height )
			continue;
		if ( ! FEQ( tEnumDesc.RefreshRate.AsFloat(), tMode.RefreshRate.AsFloat(), 0.01f ) )
			continue;

		tMode = tEnumDesc;
		return TRUE;
	}

	return FALSE;
}

//----------------------------------------------------------------------------

static
PRESULT dxC_FindWorkingFullscreenResolution( DXC_FORMAT tFormat, E_SCREEN_DISPLAY_MODE & tMode, OptionState* pState = NULL )
{
	ASSERT_RETINVALIDARG( tFormat != 0 );

	// strategy to find a working fullscreen res:
	// detect desktop res and aspect ratio
	// find smallest (> default) res that fits that aspect ratio
	// else use desktop res


	// this is a list of what we consider "common and well-supported" widths,
	//   in order to weed out wacky driver problems right off the bat

	struct DEFAULT_RES
	{
		float fAspect;
		DWORD dwWidth;
		DWORD dwHeight;
		DWORD dwRefresh;
	};
	const DEFAULT_RES ctDefaultRes[] = 
	{	//	ratio	width	height	refresh
		{	1.33f,	1024,	768,	60		},
		{	1.25f,	1280,	1024,	60		},
		{	1.60f,	1280,	800,	60		},
		{	1.77f,	1280,	720,	60		},
	};
	const int cnDefaultRes = arrsize(ctDefaultRes);

	const int cnFallbackHeight = 1400;
	const float cfAspectEpsilon = 0.01f;
	const int cnDefaultDefaultRes = 0;

	E_SCREEN_DISPLAY_MODE tDesktopMode;
	float fAR;

	V_DO_FAILED( dxC_GetD3DDisplayMode( tDesktopMode ) )
	{
		return E_FAIL;
	}

	// override to our required format
	tDesktopMode.Format = tFormat;

	fAR = (float)( tDesktopMode.Width ) / tDesktopMode.Height;
	ASSERT_RETFAIL( fAR > 0.f );
	ASSERT_RETFAIL( !tDesktopMode.IsWindowed() );


	// if the desktop mode isn't too big, just use that
	if ( tDesktopMode.Height < cnFallbackHeight )
	{
		tMode = tDesktopMode;
		return S_OK;
	}


	E_SCREEN_DISPLAY_MODE tFallbackDesc;
	int nFallback;
	for ( nFallback = 0; nFallback < cnDefaultRes; ++nFallback )
	{
		if ( ! FEQ( fAR, ctDefaultRes[nFallback].fAspect, cfAspectEpsilon ) )
			continue;

		tFallbackDesc.Width						= ctDefaultRes[nFallback].dwWidth;
		tFallbackDesc.Height					= ctDefaultRes[nFallback].dwHeight;
		tFallbackDesc.RefreshRate.Numerator		= ctDefaultRes[nFallback].dwRefresh * 1000;
		tFallbackDesc.RefreshRate.Denominator	= 1000;
		tFallbackDesc.Format					= tDesktopMode.Format;
		if ( sValidateMode( tFallbackDesc ) )
			break;
	}

	if ( nFallback < cnDefaultRes )
	{
		tMode = tFallbackDesc;
		return S_OK;
	}


	// couldn't find one, so resort to the desktop mode
	tMode = tDesktopMode;
	return S_FALSE;



	//unsigned int nCount = dxC_GetDisplayModeCount( tFormat );

	//E_SCREEN_DISPLAY_MODE tBestDesc;
	//E_SCREEN_DISPLAY_MODE tEnumDesc;
	//for ( unsigned int i = 0; i < nCount; i++ )
	//{
	//	V_CONTINUE( dxC_EnumDisplayModes( tFormat, i, tEnumDesc ) );

	//	if ( ! e_ResolutionShouldEnumerate( tEnumDesc ) )
	//		continue;

	//	float fEnumAR = (float)( tEnumDesc.Width ) / tEnumDesc.Height;
	//	if (	tEnumDesc.Format		== tFormat
	//		 &&	tEnumDesc.Height		<= nDefMaxHeight
	//		 &&	tEnumDesc.RefreshRate	== tDesktopMode.RefreshRate
	//		 &&	FEQ( fAR, fEnumAR, 0.01f ) )
	//	{
	//		// if it's in the list of favored widths
	//		for ( int f = 0; f < nFavoredWidths; f++ )
	//		{
	//			if ( pnFavoredWidths[f] == tEnumDesc.Width )
	//			{
	//				// if it's higher res than the current best
	//				if ( (		tEnumDesc.Width			>= tBestDesc.Width
	//						&&	tEnumDesc.Height		>= tBestDesc.Height )
	//					 || (	tBestDesc.Width			== 0
	//						||	tBestDesc.Height		== 0 ) )
	//				{
	//					// found our new best fullscreen res!
	//					tBestDesc = tEnumDesc;
	//				}
	//				break;
	//			}
	//		}
	//	}
	//}

	//if ( tBestDesc.Width > 0 && tBestDesc.Height > 0 )
	//{
	//	tMode = tBestDesc;
	//	return S_OK;
	//}

	//// couldn't find one, so resort to the desktop mode :/
	//tMode = tDesktopMode;

	//return S_FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_DetectDefaultFullscreenResolution( E_SCREEN_DISPLAY_MODE & tMode, OptionState* pState )
{
	tMode.Width = 0;
	tMode.Height = 0;
	DXC_FORMAT tFmt = dxC_GetDefaultDisplayFormat( FALSE );
	V_RETURN( dxC_FindWorkingFullscreenResolution( tFmt, tMode, pState ) );
	ASSERT_RETFAIL( tMode.Width > 0 );
	ASSERT_RETFAIL( tMode.Height > 0 );
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static
void e_GetWindowStyle(E_WINDOW_STYLE & ews)
{
	const DWORD nWindowedStyle = WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU;
	const DWORD nWindowedExStyle = 0;
	const DWORD nFullscreenStyle = WS_POPUP;
	const DWORD nFullscreenExStyle = WS_EX_TOPMOST;

	ews.nStyleMaskOut = nWindowedStyle | nFullscreenStyle;
	ews.nExStyleMaskOut = nWindowedExStyle | nFullscreenExStyle;

	if (ews.bWindowedIn)
	{
		if (ews.nClientHeightIn >= ews.nDesktopHeightIn)
		{
			ews.nStyleOut = WS_POPUP;
			ews.nExStyleOut = ews.bActiveStateIn ? WS_EX_TOPMOST : 0;
		}
		else
		{
			ews.nStyleOut = nWindowedStyle;
			ews.nExStyleOut = nWindowedExStyle;
		}
	}
	else
	{
		ews.nStyleOut = nFullscreenStyle;
		ews.nExStyleOut = nFullscreenExStyle;
	}
}

const E_WINDOW_STYLE e_GetWindowStyle(void)
{
	const OptionState * pState = e_OptionState_GetActive();
	E_WINDOW_STYLE ews;
	ews.bWindowedIn = pState->get_bWindowed();
	ews.bActiveStateIn = ::GetForegroundWindow() == AppCommonGetHWnd();
	ews.nClientWidthIn = pState->get_nFrameBufferWidth();
	ews.nClientHeightIn = pState->get_nFrameBufferHeight();
	e_GetDesktopSize(ews.nDesktopWidthIn, ews.nDesktopHeightIn);
	e_GetWindowStyle(ews);
	return ews;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_FrameBufferDimensionsToWindowSize(int & nWidthInOut, int & nHeightInOut)
{
	E_WINDOW_STYLE ews = e_GetWindowStyle();
	RECT tRect;
	tRect.left = 0;
	tRect.top = 0;
	tRect.right = nWidthInOut;
	tRect.bottom = nHeightInOut;
	::AdjustWindowRect(&tRect, ews.nStyleOut, false);
	nWidthInOut = tRect.right - tRect.left;
	nHeightInOut = tRect.bottom - tRect.top;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_CenterWindow(int nWidth, int nHeight, int & nPosXOut, int & nPosYOut)
{
	int nDW, nDH;
	e_IsFullscreen() ? e_GetWindowSize( nDW, nDH ) : e_GetDesktopSize( nDW, nDH );	// CHB 2007.08.23
	nPosXOut = ( nDW - nWidth )  >> 1;
	nPosYOut = ( nDH - nHeight ) >> 1;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

// CHB 2007.09.26
// Start this off as true, so that we process all initialization-
// related sizes. This will be cleared to false after the first
// call to e_DoWindowSize(), when we're ready.
static bool s_bInternalWindowSizeEvent = true;
bool e_IsInternalWindowSizeEvent(void)
{
	return s_bInternalWindowSizeEvent;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_UpdateWindowStyle(void)
{
	HWND hWnd = AppCommonGetHWnd();

	E_WINDOW_STYLE ews = e_GetWindowStyle();
	const DWORD nCurStyle = ::GetWindowLong(hWnd, GWL_STYLE);
	DWORD nNewStyle = (nCurStyle & (~ews.nStyleMaskOut)) | ews.nStyleOut;
	const DWORD nCurExStyle = ::GetWindowLong(hWnd, GWL_EXSTYLE);
	DWORD nNewExStyle = (nCurExStyle & (~ews.nExStyleMaskOut)) | ews.nExStyleOut;
	if (nNewStyle != nCurStyle)
	{
		::SetWindowLong(hWnd, GWL_STYLE, nNewStyle);
		#if ISVERSION(DEVELOPMENT)
		nNewStyle = ::GetWindowLong(hWnd, GWL_STYLE);
		#endif
	}
	if (nNewExStyle != nCurExStyle)
	{
		::SetWindowLong(hWnd, GWL_EXSTYLE, nNewExStyle);
		#if ISVERSION(DEVELOPMENT)
		nNewExStyle = ::GetWindowLong(hWnd, GWL_EXSTYLE);
		#endif
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_DoWindowSize(
	int nCurWidth /*= -1*/,
	int nCurHeight /*= -1*/ )
{
	// CHB 2007.09.06 - Please keep this function for dealing with
	// the Windows window only. Notification of back buffer size
	// changes belongs in the event handler (see, for example,
	// LocalEventHandler::CommitFrameBuffer() below) or in
	// e_SettingsCommitActionFlags() as appropriate.

	HWND hWnd = AppCommonGetHWnd();

	// Adjust window style.
	e_UpdateWindowStyle();

	if ( nCurWidth <= 0 )
		nCurWidth = e_GetWindowWidth();
	if ( nCurHeight <= 0 )
		nCurHeight = e_GetWindowHeight();

	ASSERT_RETFAIL( nCurWidth > 0 );
	ASSERT_RETFAIL( nCurHeight > 0 );

	int nWinWidth = nCurWidth;
	int nWinHeight = nCurHeight;
	e_FrameBufferDimensionsToWindowSize(nWinWidth, nWinHeight);
	int nX, nY;
	e_CenterWindow(nWinWidth, nWinHeight, nX, nY);

	HWND hInsertAfter = HWND_NOTOPMOST;
	if ( e_IsFullscreen() )
		hInsertAfter = HWND_TOPMOST;

	s_bInternalWindowSizeEvent = true;	// CHB 2007.09.26
	int nRet = SetWindowPos(hWnd, hInsertAfter, nX, nY, nWinWidth, nWinHeight, SWP_SHOWWINDOW);
	//int nRet = MoveWindow( AppCommonGetHWnd(), nX, nY, nCurWidth, nCurHeight, TRUE );
	s_bInternalWindowSizeEvent = false;	// CHB 2007.09.26

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static
bool dxC_GetModeVariations(unsigned nFormatIn, DXC_FORMAT & nAlphaFormatOut, DXC_FORMAT & nGenericFormatOut)
{
	DXC_FORMAT nFormat = static_cast<DXC_FORMAT>(nFormatIn);

#ifdef ENGINE_TARGET_DX9
	switch (nFormat)
	{
		case D3DFMT_X8R8G8B8:
		case D3DFMT_A8R8G8B8:
			nAlphaFormatOut = D3DFMT_A8R8G8B8;
			nGenericFormatOut = D3DFMT_X8R8G8B8;
			return true;

		case D3DFMT_X1R5G5B5:
		case D3DFMT_A1R5G5B5:
			nAlphaFormatOut = D3DFMT_A1R5G5B5;
			nGenericFormatOut = D3DFMT_X1R5G5B5;
			return true;

	}
#endif

	nAlphaFormatOut = nFormat;
	nGenericFormatOut = nFormat;
	return false;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static
DXC_FORMAT dxC_GetDisplayModeForEnumeration(DXC_FORMAT nFormat)
{
	DXC_FORMAT nAlphaFormat, nGenericFormat;
	dxC_GetModeVariations(nFormat, nAlphaFormat, nGenericFormat);
	return (dxC_GetDisplayModeCount(nAlphaFormat) > 1) ? nAlphaFormat : nGenericFormat;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

// Validate display mode and make adjustments as necessary.
// (Validates against currently active DirectX device.)
static
void dxC_ValidateDisplayMode(E_SCREEN_DISPLAY_MODE & Mode)
{
	// Limit dimensions to 46,340 per side, to guarantee the area
	// doesn't overflow a 32-bit int.
	Mode.Width = MIN(Mode.Width, 46340);
	Mode.Height = MIN(Mode.Height, 46340);

	// Massage mode if necessary.
	DXC_FORMAT nAlphaFormat, nGenericFormat;
	dxC_GetModeVariations(Mode.Format, nAlphaFormat, nGenericFormat);
	
	// Special case for windowed mode: use D3DFMT_A8R8G8B8.
	// This maintains existing behavior.
	#ifdef ENGINE_TARGET_DX9
	if (Mode.IsWindowed())
	{
		// Windowed.
		switch (Mode.Format)
		{
			case D3DFMT_X8R8G8B8:
			case D3DFMT_X1R5G5B5:
			case D3DFMT_R5G6B5:
				Mode.Format = D3DFMT_A8R8G8B8;
				break;
			default:
				break;
		}
	}
	#endif

	// Chose the right format.
	DXC_FORMAT nEnumFormat = static_cast<DXC_FORMAT>(Mode.Format);
	if (!Mode.IsWindowed())
	{
		// CHB 2007.03.06 - IDirect3D9::GetAdapterModeCount cannot be relied upon
		// to provide accurate results for modes with alpha.  If there is a generic
		// format, assume we can use the generic portion for alpha.  But we still
		// need to enumerate using the generic format.
#if 1
		Mode.Format = nAlphaFormat;
		nEnumFormat = dxC_GetDisplayModeForEnumeration(static_cast<DXC_FORMAT>(Mode.Format));
		if (dxC_GetDisplayModeCount(nEnumFormat) < 1)
		{
			Mode.Format = nEnumFormat = dxC_GetDefaultDisplayFormat(false);
		}
#else
		// Choose the format with the higher number of modes available.
		Mode.Format = nAlphaFormat;
		if (dxC_GetDisplayModeCount(nGenericFormat) > dxC_GetDisplayModeCount(nAlphaFormat))
		{
			// I will keep him and I will name him George.
			Mode.Format = nGenericFormat;
		}

		// Check to make sure format is available.
		if (dxC_GetDisplayModeCount(Mode.Format) < 1)
		{
			Mode.Format = dxC_GetDefaultDisplayFormat(false);
		}
#endif
	}

	// Enforce minimum width and height.
	if ((Mode.Width < SETTINGS_MIN_RES_WIDTH) || (Mode.Height < SETTINGS_MIN_RES_HEIGHT))
	{
		if (Mode.IsWindowed())
		{
			Mode.Width = SETTINGS_MIN_RES_WIDTH;
			Mode.Height = SETTINGS_MIN_RES_HEIGHT;
		}
		else
		{
			// Get mode based on "favored widths" algorithm.
			V(dxC_FindWorkingFullscreenResolution(nEnumFormat, Mode));
		}
	}

	// Nothing else to do if windowed mode.
	if (Mode.IsWindowed())
	{
		Mode.SetWindowed();	// just to make sure numerator and denomenator are both correct
		Mode.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		Mode.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		return;
	}

	// Otherwise, just choose the closest matching mode.
	const unsigned nCount = dxC_GetDisplayModeCount(nEnumFormat);

	E_SCREEN_DISPLAY_MODE tBestDesc;
	int nBestAreaDelta = std::numeric_limits<int>::max();
	float fBestRefreshDelta = std::numeric_limits<float>::max();

	for (unsigned i = 0; i < nCount; ++i)
	{
		E_SCREEN_DISPLAY_MODE tEnumDesc;
		V_CONTINUE(dxC_EnumDisplayModes(nEnumFormat, i, tEnumDesc));

		// Skip too-small ones.
		if ((tEnumDesc.Width < SETTINGS_MIN_RES_WIDTH) || (tEnumDesc.Height < SETTINGS_MIN_RES_HEIGHT))
		{
			continue;
		}

		// Only consider areas less than or equal to the target area.
		int nBasisArea = Mode.Width * Mode.Height;
		int nEnumArea = tEnumDesc.Width * tEnumDesc.Height;
		if (nEnumArea > nBasisArea)
		{
			continue;
		}
		
		// Compare screen area.
		int nDelta = ABS(nBasisArea - nEnumArea);
		if (nDelta < nBestAreaDelta)
		{
			// This one's better.
			nBestAreaDelta = nDelta;
			tBestDesc = tEnumDesc;
			// Reset the best refresh because we moved on to different dimensions.
			fBestRefreshDelta = std::numeric_limits<float>::max();
		}
		else if (nDelta > nBestAreaDelta)
		{
			// This one's worse.
			continue;
		}

		// If the area is exactly on the money; let's do the details.
		if ((tEnumDesc.Width == Mode.Width) && (tEnumDesc.Height == Mode.Height))
		{
			if (tEnumDesc.RefreshRate == Mode.RefreshRate)
			{
				// Perfect match.
				tBestDesc = tEnumDesc;
				break;
			}

			if ((tBestDesc.Width != Mode.Width) || (tBestDesc.Height != Mode.Height))
			{
				// This is an exact WxH match, whereas the existing best is not.
				tBestDesc = tEnumDesc;
				// Reset the best refresh because we moved on to different dimensions.
				fBestRefreshDelta = std::numeric_limits<float>::max();
				continue;
			}
		}

		// Decide on the basis of refresh rate.
		float fDelta = ABS(tEnumDesc.RefreshRate.AsFloat() - Mode.RefreshRate.AsFloat());
		if (fDelta < fBestRefreshDelta)
		{
			// Better refresh rate match.
			fBestRefreshDelta = fDelta;
			tBestDesc = tEnumDesc;
		}
	}

	// Choose the best match.
	if ((tBestDesc.Width > 0) && (tBestDesc.Height > 0))
	{
		// But keep the alpha-correct version of the format.
		unsigned nFormat = Mode.Format;
		Mode = tBestDesc;
		Mode.Format = nFormat;
	}
	else
	{
		// Unable to find a suitable match.
		// In this case, keep the original mode, even though it doesn't appear to be available.
		// (In fact there would have to be no modes that meet the minimum resolution requirements
		// for this to occur.)
		ASSERT(false);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void dxC_StateToPresentation(const OptionState * pState, D3DPRESENT_PARAMETERS & Pres)
{
	Pres.Windowed = pState->get_bWindowed();

	unsigned nBackBufferCount = pState->get_nBackBufferCount();
	ASSERT_DO( nBackBufferCount >= 0 && nBackBufferCount <= 2 )
	{
		nBackBufferCount = 1;
	}

#ifdef ENGINE_TARGET_DX9
	Pres.BackBufferCount = nBackBufferCount;
	Pres.BackBufferFormat = dxC_GetBackbufferRenderTargetFormat( pState );
	Pres.SwapEffect = e_IsNoRender() ? D3DSWAPEFFECT_COPY : D3DSWAPEFFECT_DISCARD;
	Pres.BackBufferWidth = pState->get_nFrameBufferWidth();
	Pres.BackBufferHeight = pState->get_nFrameBufferHeight();
	Pres.FullScreen_RefreshRateInHz = Pres.Windowed ? 0 : pState->get_nScreenRefreshHzNumerator();	// CHB 2007.09.07 - Denominator ignored for DX9
#endif
#ifdef ENGINE_TARGET_DX10
	Pres.BufferCount = nBackBufferCount + 1; //Need to include front buffer
	Pres.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	Pres.BufferUsage = DXGI_USAGE_SHADER_INPUT | DXGI_USAGE_RENDER_TARGET_OUTPUT;
	E_SCREEN_DISPLAY_MODE Mode;
	e_Screen_DisplayModeFromState(pState, Mode);
	Pres.BufferDesc = dxC_ConvertNativeDisplayMode(Mode);
#endif
	Pres.hDeviceWindow = AppCommonGetHWnd();

#ifdef ENGINE_TARGET_DX9 //No autodepthstencil in DX10
	Pres.EnableAutoDepthStencil = true;
	Pres.AutoDepthStencilFormat = gtAutoDepthStencilFormat;	// CHB !!!
	Pres.PresentationInterval = pState->get_bFlipWaitVerticalRetrace() ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE;
#endif

	//Set MSAA
	Pres.MultiSampleType = pState->get_nMultiSampleType() DX10_BLOCK( != 0 ? pState->get_nMultiSampleType() : 1 );
	Pres.MultiSampleQuality = pState->get_nMultiSampleQuality();

	Pres.Flags = 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static
bool operator==(const E_SCREEN_DISPLAY_MODE & l, const E_SCREEN_DISPLAY_MODE & r)
{
	return
		(l.Width == r.Width) &&
		(l.Height == r.Height) &&
		(l.RefreshRate == r.RefreshRate) &&
		(l.Format == r.Format);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

bool e_IsCSAAAvailable()
{
	return !!dx9_CapGetValue( DX9CAP_COVERAGE_SAMPLING );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

bool e_IsMultiSampleAvailable(const OptionState * pState, DXC_MULTISAMPLE_TYPE nMultiSampleType, int nQuality )
{
	// Check bounds.
	if ((nMultiSampleType < D3DMULTISAMPLE_NONE) || (nMultiSampleType > D3DMULTISAMPLE_16_SAMPLES))
	{
		return false;
	}

#ifdef ENGINE_TARGET_DX9

	// Check to make sure device supports this.
	if (nMultiSampleType != D3DMULTISAMPLE_NONE)
	{
		unsigned nAdapter = pState->get_nAdapterOrdinal();
		D3DC_DRIVER_TYPE nDriverType = dx9_GetDeviceType();
		bool bWindowed = pState->get_bWindowed();
		D3DFORMAT nBBFormat = dxC_GetBackbufferAdapterFormat( pState );
		D3DFORMAT nDSFormat = gtAutoDepthStencilFormat;	// CHB !!!
		DWORD dwQuality;

		if (FAILED(dx9_GetD3D()->CheckDeviceMultiSampleType(nAdapter, nDriverType, nBBFormat, bWindowed, nMultiSampleType, &dwQuality)))
		{
			return false;
		}
		if ( dwQuality <= (DWORD)nQuality )
			return false;
		if (FAILED(dx9_GetD3D()->CheckDeviceMultiSampleType(nAdapter, nDriverType, nDSFormat, bWindowed, nMultiSampleType, &dwQuality)))
		{
			return false;
		}
		if ( dwQuality <= (DWORD)nQuality )
			return false;
	}
	
#elif defined( ENGINE_TARGET_DX10 )
	if( !dxC_GetDevice() )
		return false;

	UINT nQualityLevels;
	if( FAILED( dxC_GetDevice()->CheckMultisampleQualityLevels( dxC_GetBackbufferAdapterFormat( pState ), nMultiSampleType, &nQualityLevels ) ) )
		return false;
	
	if( nQualityLevels <= (UINT)nQuality )
		return false;

#endif

	return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

namespace
{

class LocalEventHandler : public OptionState_EventHandler
{
	public:
		virtual void Update(OptionState * pState) override;
		virtual PRESULT Commit(const OptionState * pOldState, const OptionState * pNewState) override;
		virtual void QueryActionRequired(const OptionState * pOldState, const OptionState * pNewState, unsigned & nActionFlagsOut) override;

	private:
		static void UpdateNoRenderMode(OptionState * pState);
		static void UpdateBackBufferCount(OptionState * pState);
		static void UpdateFrameBufferSizeAndFormat(OptionState * pState);
		static void UpdateMultiSample(OptionState * pState);
		static void UpdateGlow(OptionState * pState);
		static void UpdateGamma(OptionState * pState);

		static PRESULT CommitFrameBuffer(const OptionState * pOldState, const OptionState * pNewState);
		static PRESULT CommitGamma(const OptionState * pOldState, const OptionState * pNewState);

		static void QueryFrameBuffer(const OptionState * pOldState, const OptionState * pNewState, unsigned & nActionFlagsOut);
		static void QueryMultiSample(const OptionState * pOldState, const OptionState * pNewState, unsigned & nActionFlagsOut);

		static bool HasFrameBufferChanged(const OptionState * pOldState, const OptionState * pNewState);
};

void LocalEventHandler::UpdateGamma(OptionState * pState)
{
	float fGammaPower = pState->get_fGammaPower();
	fGammaPower = PIN(fGammaPower, MIN_GAMMA_ADJUST, MAX_GAMMA_ADJUST);
	pState->set_fGammaPower(fGammaPower);
}

void LocalEventHandler::UpdateGlow(OptionState * pState)
{
	if (!e_IsGlowAvailable(pState))
	{
		pState->set_bGlow(false);
	}
}

void LocalEventHandler::UpdateMultiSample(OptionState * pState)
{
	if (!e_IsMultiSampleAvailable(pState, pState->get_nMultiSampleType(), pState->get_nMultiSampleQuality() ) )
	{
		pState->set_nMultiSampleType(DXC_9_10(D3DMULTISAMPLE_NONE, 0));
		pState->set_nMultiSampleQuality(0);
	}
}

void LocalEventHandler::UpdateNoRenderMode(OptionState * pState)
{
	// Detect and handle the fillpak/minpak NULLREF case
	if ( e_IsNoRender() )
	{
#ifdef ENGINE_TARGET_DX9
		LogMessage( LOG_FILE, "Detected FILLPAK or MINPAK, setting NULLREF D3D device!\n" );

		// it's running a pak generation or min-pak (say, on a server), it's not meant to render
		// CHB 2007.03.04 - Note, setting the device type to NULLREF
		// causes software vertex processing to be chosen automatically
		// during device creation.
		pState->set_nDeviceDriverType(D3DDEVTYPE_NULLREF);
		pState->set_nAdapterOrdinal(D3DADAPTER_DEFAULT);

		// CHB !!! - This should use the desktop mode.
		D3DDISPLAYMODE Mode;
		//V( dx9_GetD3D()->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &Mode) );
		Mode.Format = D3DFMT_A8R8G8B8;

		pState->set_nFrameBufferWidth(SETTINGS_MIN_RES_WIDTH);
		pState->set_nFrameBufferHeight(SETTINGS_MIN_RES_HEIGHT);
		pState->set_nFrameBufferColorFormat(Mode.Format);
		//trace( "### %s:%d -- Setting backbuffer color format to %d -- %s\n", __FILE__, __LINE__, Mode.Format, __FUNCSIG__ );
		pState->set_nBackBufferCount(1);
		pState->set_bWindowed(true);

		// CHB 2007.03.04 - Swap effect is handled in dxC_StateToPresentation().
#endif
	}
}

void LocalEventHandler::UpdateBackBufferCount(OptionState * pState)
{
	// CML 2007.08.01 - Avoid an infinite loop during _DoUpdate.
	if ( e_IsNoRender() )
		return;

	// Clamp back buffer count.
	unsigned nBackBufferCount = pState->get_nBackBufferCount();
	ASSERT_DO(nBackBufferCount <= 2)
	{
		nBackBufferCount = 1;
	}
	if ( pState->get_bTripleBuffer() )
		nBackBufferCount = 2;
	pState->set_nBackBufferCount(nBackBufferCount);
}

void LocalEventHandler::UpdateFrameBufferSizeAndFormat(OptionState * pState)
{
	// Don't bother for no-render mode.
	if (e_IsNoRender())
	{
		return;
	}

	// Get mode from state.
	E_SCREEN_DISPLAY_MODE Mode;
	e_Screen_DisplayModeFromState(pState, Mode);

	// Adjust mode as necessary.
	dxC_ValidateDisplayMode(Mode);

	// Save adjustments to state.
	// No need to Suspend() inside Update().
	e_Screen_DisplayModeToStateNoSuspend(pState, Mode);
	DXC_FORMAT nAlphaFormat, nGenericFormat;
	dxC_GetModeVariations(Mode.Format, nAlphaFormat, nGenericFormat);
	pState->set_nFrameBufferColorFormat(nAlphaFormat);
	pState->set_nAdapterColorFormat(nGenericFormat);
	//trace( "### %s:%d -- Setting backbuffer color format to %d -- %s\n", __FILE__, __LINE__, Mode.Format, __FUNCSIG__ );

#if 0	// CHB !!! - deal with HDR mode
	HDR_MODE eHDR = e_GetValidatedHDRMode();
	switch ( eHDR )
	{
	case HDR_MODE_LINEAR_FRAMEBUFFER:
		if ( dx9_CapGetValue( DX9CAP_FLOAT_HDR ) != D3DFMT_UNKNOWN )
		{
			// set to detected best float HDR mode
			tMode.Format = (DXC_FORMAT)dx9_CapGetValue( DX9CAP_FLOAT_HDR );
			break;
		} else if ( dx9_CapGetValue( DX9CAP_INTEGER_HDR ) != D3DFMT_UNKNOWN )
		{
			// set to detected best integer HDR mode
			tMode.Format = (DXC_FORMAT)dx9_CapGetValue( DX9CAP_INTEGER_HDR );
			break;
		}
	case HDR_MODE_LINEAR_SHADERS:
	case HDR_MODE_SRGB_LIGHTS:
	case HDR_MODE_NONE:
	default:
		tMode.Format = pOverrides ? (DXC_FORMAT)pOverrides->dwFullscreenColorFormat : (DXC_FORMAT)0;
	}


	if ( tMode.Format == 0 )
		tMode.Format = dxC_GetDefaultDisplayFormat( FALSE );


	if ( tMode.Width < SETTINGS_MIN_RES_WIDTH || tMode.Height < SETTINGS_MIN_RES_HEIGHT )
	{
		V_RETURN( dxC_FindWorkingFullscreenResolution( tMode.Format, tMode ) );
	}
#endif
}

void LocalEventHandler::Update(OptionState * pState)
{
	UpdateNoRenderMode(pState);
	UpdateBackBufferCount(pState);
	UpdateFrameBufferSizeAndFormat(pState);
	UpdateMultiSample(pState);
	UpdateGlow(pState);
	UpdateGamma(pState);
}

PRESULT LocalEventHandler::Commit(const OptionState * pOldState, const OptionState * pNewState)
{
	bool bSuccess = true;
	PRESULT nResult;
	V(nResult = CommitFrameBuffer(pOldState, pNewState));
	bSuccess = bSuccess && SUCCEEDED(nResult);
	V(nResult = CommitGamma(pOldState, pNewState));
	bSuccess = bSuccess && SUCCEEDED(nResult);
	return bSuccess ? S_OK : E_FAIL;
}

PRESULT LocalEventHandler::CommitGamma(const OptionState * pOldState, const OptionState * pNewState)
{
	// Check for changes.
	bool bChanged =
		(pOldState->get_fGammaPower() != pNewState->get_fGammaPower());
	if (!bChanged)
	{
		return S_FALSE;
	}

	return e_SetGamma(pNewState->get_fGammaPower());
}

bool LocalEventHandler::HasFrameBufferChanged(const OptionState * pOldState, const OptionState * pNewState)
{
	E_SCREEN_DISPLAY_MODE OldMode, NewMode;
	e_Screen_DisplayModeFromState(pOldState, OldMode);
	e_Screen_DisplayModeFromState(pNewState, NewMode);
	return
		(pOldState->get_nFrameBufferWidth() != pNewState->get_nFrameBufferWidth()) ||
		(pOldState->get_nFrameBufferHeight() != pNewState->get_nFrameBufferHeight()) ||
		(OldMode.RefreshRate != NewMode.RefreshRate) ||
		(pOldState->get_bWindowed() != pNewState->get_bWindowed()) ||
		(pOldState->get_nScanlineOrdering() != pNewState->get_nScanlineOrdering()) ||
		(pOldState->get_bTripleBuffer() != pNewState->get_bTripleBuffer()) ||
		(pOldState->get_bFlipWaitVerticalRetrace() != pNewState->get_bFlipWaitVerticalRetrace()) ||
		(pOldState->get_nFrameBufferColorFormat() != pNewState->get_nFrameBufferColorFormat());
}

PRESULT LocalEventHandler::CommitFrameBuffer(const OptionState * pOldState, const OptionState * pNewState)
{
	// Check for changes.
	if (!HasFrameBufferChanged(pOldState, pNewState))
	{
		return S_FALSE;
	}

	// Notify others of the size change.
	// CHB 2007.03.01 - This really belongs game-side; would be nice to move it...
	// CHB 2007.03.04 - Application could add a Commit() event handler to do this.
	AppCommonGetWindowWidthRef() = pNewState->get_nFrameBufferWidth();
	AppCommonGetWindowHeightRef() = pNewState->get_nFrameBufferHeight();

	// tell the viewers (and, possibly, dPVS) that the screen size has changed
	V( e_ViewersResizeScreen() );

	// Good to go.
	return S_OK;
}

void LocalEventHandler::QueryActionRequired(const OptionState * pOldState, const OptionState * pNewState, unsigned & nActionFlagsOut)
{
	QueryFrameBuffer(pOldState, pNewState, nActionFlagsOut);
	QueryMultiSample(pOldState, pNewState, nActionFlagsOut);
}

void LocalEventHandler::QueryFrameBuffer(const OptionState * pOldState, const OptionState * pNewState, unsigned & nActionFlagsOut)
{
	// Check for changes.
	if (!HasFrameBufferChanged(pOldState, pNewState))
	{
		return;
	}

	// Get modes.
	E_SCREEN_DISPLAY_MODE OldMode, NewMode;
	e_Screen_DisplayModeFromState(pOldState, OldMode);
	e_Screen_DisplayModeFromState(pNewState, NewMode);

	// Only valid modes should make it this far.
	{
		E_SCREEN_DISPLAY_MODE TempMode = NewMode;
		dxC_ValidateDisplayMode(NewMode);
		ASSERT(TempMode == NewMode);
	}

	// Request a reset.
	// All frame buffer changes require a device reset.
	// The changes will actually take effect when the device is
	// reset using the new settings.
	nActionFlagsOut |= SETTING_NEEDS_RESET;

	// Adjust window size if needed.
	if (((OldMode.Width != NewMode.Width) || (OldMode.Height != NewMode.Height) || (OldMode.RefreshRate != NewMode.RefreshRate)) /* CHB 2007.08.22 && (NewMode.RefreshRate == 0) */ )
	{
		// CHB 2007.03.13 - The window resizing needs to occur AFTER
		// the mode change (e.g., device reset). Otherwise, in the
		// case of going from a small fullscreen res to a larger
		// windowed res, if the window resize occurs before the
		// device resolution is restored to its larger original
		// size, the window size will be restricted by the OS to the
		// small fullscreen and so will not be the correct size.
		//e_DoWindowSize(NewMode.Width, NewMode.Height);
		nActionFlagsOut |= SETTING_NEEDS_WINDOWSIZE;
	}

//	if( pOldState->get_bWindowed() != pNewState->get_bWindowed() )
//	{
//		nActionFlagsOut |= SETTING_NEEDS_FULLSCREEN_TOGGLE;
//	}
}

void LocalEventHandler::QueryMultiSample(const OptionState * pOldState, const OptionState * pNewState, unsigned & nActionFlagsOut)
{
	// Check for changes.
	bool bChanged =
		(pOldState->get_nMultiSampleType() != pNewState->get_nMultiSampleType()) ||
		(pOldState->get_nMultiSampleQuality() != pNewState->get_nMultiSampleQuality());
	if (!bChanged)
	{
		return;
	}

	// Request a reset.
	// The changes will actually take effect when the device is
	// reset using the new settings.
	nActionFlagsOut |= SETTING_NEEDS_RESET;
}

};

PRESULT dxC_Settings_RegisterEventHandler(void)
{
	return e_OptionState_RegisterEventHandler(new LocalEventHandler);
}


//----------------------------------------------------------------------------
// IMPLEMENTATIONS
//----------------------------------------------------------------------------

const char * e_GetDisplayModeString()
{
	static char szMode[ 256 ];
#ifdef ENGINE_TARGET_DX9
	D3DDISPLAYMODE d3dMode;
	V( dx9_GetD3D()->GetAdapterDisplayMode( dxC_GetAdapter(), &d3dMode ) );
	int nFormat;
	for ( nFormat = 0; nFormat < NUM_DISPLAY_FORMATS; nFormat++ )
		if ( d3dMode.Format == gtDisplayFormatList[ nFormat ] )
			break;
	int nWidth, nHeight;
	if ( e_IsFullscreen() )
	{
		nWidth  = d3dMode.Width;
		nHeight = d3dMode.Height;
	} else
	{
		nWidth  = e_GetWindowWidth();
		nHeight = e_GetWindowHeight();
	}
	PStrPrintf( szMode, 256, "Display Mode: res: %dx%d fmt: %s refresh: %dHz", nWidth, nHeight, gtDisplayFormatListStrings[ nFormat ], d3dMode.RefreshRate );
#endif
#ifdef ENGINE_TARGET_DX10
	PStrPrintf( szMode, 256, "KMNV TODO: Figure out how to print Display mode in DX10" );
#endif 
	return szMode;
}


const char * e_GetDisplaySettingsString()
{
	static char szMode[ 256 ];

	const OptionState * pState = e_OptionState_GetActive();

	DXC_FORMAT tCurFmt = pState->get_nFrameBufferColorFormat();
	int nFormat;
	for ( nFormat = 0; nFormat < NUM_DISPLAY_FORMATS; nFormat++ )
		if ( tCurFmt == gtDisplayFormatList[ nFormat ] )
			break;

	float fRefresh = (float)pState->get_nScreenRefreshHzNumerator() / pState->get_nScreenRefreshHzDenominator();

	if ( pState->get_bWindowed() )
		PStrPrintf( szMode, 256, "%dx%d, %s %s", pState->get_nFrameBufferWidth(), pState->get_nFrameBufferHeight(), gtDisplayFormatListStrings[ nFormat ], "windowed" );
	else
		PStrPrintf( szMode, 256, "%dx%d @ %1.2fHz, %s %s", pState->get_nFrameBufferWidth(), pState->get_nFrameBufferHeight(), fRefresh, gtDisplayFormatListStrings[ nFormat ], "fullscreen" );

	return szMode;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static float sGetCurrentFadePhase( TIME tCurTime )
{
	TIMEDELTA tCurDelta = tCurTime - sgtFadeStart;
	float fPhase = (float)tCurDelta / sgtFadeDuration;
	fPhase = SATURATE( fPhase );
	if ( sgeFadeType == FADE_IN )
		return fPhase;
	ASSERT( sgeFadeType == FADE_OUT );
	return 1.f - fPhase;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_BeginFade( FADE_TYPE eType, unsigned int nDurationMS, DWORD dwFadeColor, void (*pfnFadeEndCallback)() )
{
	//if ( AppCommonGetDemoMode() )
	//{
	//	if ( pfnFadeEndCallback )
	//		pfnFadeEndCallback();
	//	return S_FALSE;
	//}

	ASSERT_RETINVALIDARG( nDurationMS != 0 );
	ASSERT_RETINVALIDARG( IS_VALID_INDEX( eType, NUM_FADE_TYPES ) );

	//if ( eType == FADE_IN )
	//{
	//	//e_SetUIOnlyRender( FALSE );
	//	//e_SetRenderEnabled( TRUE );
	//	//sgfFadePhase = 0.0f;
	//	//sgfFadeRate = 1.0f / (float)nDurationMS;
	//} else
	//{
	//	ASSERT( eType == FADE_OUT );
	//	//sgfFadePhase = 1.0f;
	//	//sgfFadeRate = - 1.0f / (float)nDurationMS;
	//}

	gpfnFadeEndCallback = pfnFadeEndCallback;
	sgdwFadeColor = dwFadeColor;
	sgbFading = TRUE;
	sgeFadeType = eType;
	sgtFadeDuration = (TIMEDELTA)nDurationMS;
	sgtFadeStart = e_GetCurrentTimeRespectingPauses();

#ifdef PROFILE_FADE
	// Declare enumeration to hold return value of 
	// the call to StartProfile.
	PROFILE_COMMAND_STATUS profileResult;

	profileResult = StartProfile(
		PROFILE_THREADLEVEL,
		PROFILE_CURRENTID );
#endif


	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_SetSolidColorRenderEnabled( BOOL bEnabled )
{
	sgbShowSolidColor = bEnabled;
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_SetSolidColorRenderColor( DWORD dwColor )
{
	sgdwSolidColor = dwColor;
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_GetFadeColor( DWORD & dwColor )
{
	if ( sgbFading )
	{
		dwColor = sgdwFadeColor;
	}
	else if ( sgbShowSolidColor )
	{
		dwColor = sgdwSolidColor;
		// Make sure it's fully alpha'd
		dwColor |= 0xff000000;
		return S_OK;
	}
	else
	{
		return S_FALSE;
	}

	float fPhase = sGetCurrentFadePhase( e_GetCurrentTimeRespectingPauses() );
	fPhase = ( 1.f - fPhase ) * 255.f;
	fPhase = MIN( fPhase, 255.f );
	fPhase = MAX( fPhase, 0.f );
	DWORD dwAlpha = ( (DWORD)fPhase ) << 24;

	dwColor &= 0x00ffffff;
	dwColor |= dwAlpha;

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_UpdateFade()
{
	if ( ! sgbFading )
		return S_FALSE;

	//TRACE( "UpdateFade: %f\n", fElapsedSecs );
	//sgfFadePhase += sgfFadeRate * fElapsedSecs * MSECS_PER_SEC;

	float fPhase = sGetCurrentFadePhase( e_GetCurrentTimeRespectingPauses() );

	BOOL bStopProfile = FALSE;
	if ( fPhase >= 1.f )
	{
		//sgfFadePhase = 1.f;
		//sgfFadeRate = 0.f;
		sgbFading = FALSE;
		// faded all the way in
		if ( gpfnFadeEndCallback )
			gpfnFadeEndCallback();
		gpfnFadeEndCallback = NULL;

		bStopProfile = TRUE;
	}
	else if ( fPhase <= 0.f )
	{
		//sgfFadePhase = 0.f;
		//sgfFadeRate = 0.f;
		sgbFading = FALSE;
		// faded all the way out
//		e_SetUIOnlyRender( TRUE );
//		e_SetRenderEnabled( FALSE );
		if ( gpfnFadeEndCallback )
			gpfnFadeEndCallback();
		gpfnFadeEndCallback = NULL;

		bStopProfile = TRUE;
	}

#ifdef PROFILE_FADE
	if ( bStopProfile )
	{
		// Declare enumeration to hold result of call
		// to StopProfile.
		PROFILE_COMMAND_STATUS profileResult;

		profileResult = StopProfile(
			PROFILE_THREADLEVEL,
			PROFILE_CURRENTID);
	}
#endif

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_PresentDelaySet( DWORD dwFrames )
{
	sgdwPresentDelayFrames = dwFrames;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_PresentDelayUpdate()
{
	if ( sgdwPresentDelayFrames > 0 )
		sgdwPresentDelayFrames--;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

BOOL e_PresentEnabled()
{
	return ( sgdwPresentDelayFrames == 0 );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_RenderFlagPostCommandEngine( int nFlag )
{
	// optional post-command (console, etc.) settings
	//   (for tasks that shouldn't be performed when any old setrenderflag occurs, eg at startup)
#ifdef ENGINE_TARGET_DX9 //Shouldn't be necessary in DX10
	if ( nFlag == RENDER_FLAG_FILTER_STATES )
		dx9_SetStateManager();
#endif
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT e_SetMaxDetail( int nMaxDetail )
{
	// needs to use options state now
	return E_NOTIMPL;

	e_UpdateGlobalFlags();

	e_MaterialsRefresh();

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int e_GetMaxDetail()
{
	return dxC_GetCurrentMaxEffectTarget();
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT e_SetFogEnabled( BOOL bValue, BOOL bForce )
{
	extern BOOL gbFogEnabled;

	if ( ! e_GetRenderFlag( RENDER_FLAG_FOG_ENABLED ) )
		bValue = FALSE;

	gbFogEnabled = bValue;
#ifdef ENGINE_TARGET_DX9  //DX10 Fog needs to be done in the shader
	V_RETURN( dxC_SetRenderState( D3DRS_FOGENABLE, bValue, bForce ) );
	//TRACE_EVENT_INT( "Set Fog Enabled", bValue );
#endif
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

COLORDEPTH_MODE dx9_GetCurrentColorDepthMode()
{
	// this seems unnecessary
	//ASSERT_RETVAL( dxC_GetDevice(), COLORDEPTH_MODE_INVALID );

	D3DPRESENT_PARAMETERS tPP = dxC_GetD3DPP( dxC_GetCurrentSwapChainIndex() );
	int nBitDepth = dx9_GetTextureFormatBitDepth( tPP.BackBufferFormat );
	switch ( nBitDepth )
	{
	case 16:
		return COLORDEPTH_MODE_16;
	case 32:
		return COLORDEPTH_MODE_32;
	}

	ASSERTX( 0, "Invalid back-buffer format detected!" );
	return COLORDEPTH_MODE_INVALID;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_SetFogColor( DWORD dwColor )
{
	extern DWORD gdwFogColor;
	gdwFogColor = dwColor;
	DX9_BLOCK( dxC_SetRenderState( D3DRS_FOGCOLOR, e_GetFogColor() ); )
#ifndef DX10_GET_RUNNING_HACK
	DX10_BLOCK( ASSERTX( 0, "KMNV TODO: Handle fog color with shader" ); )
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_SettingsCommitActionFlags( DWORD dwActionFlags, BOOL bForceSync /*= FALSE*/ )
{
	// CHB 2007.08.23 - Tell the UI the display is about to change
	// (so it can release the mouse).
	if (dwActionFlags != 0)
	{
		e_UIDisplayChange(false);
	}

	if ( dwActionFlags & SETTING_NEEDS_RESET )
	{
		V( e_Reset() );
	}

//#ifdef ENGINE_TARGET_DX10
//	if( dwActionFlags & SETTING_NEEDS_FULLSCREEN_TOGGLE )
//	{
//		dx10_ToggleFullscreen();
//	}
//#endif

	// CHB 2007.09.06 - Looks like DX10 needs two window sizes --
	// one before the swap chain change and one after. Actually,
	// the window probably mainly just needs the style to be set
	// before swap chain creation (otherwise the swap chain draws
	// over the window's non-client area). The second size after the
	// swap chain creation is to get the window to the right size.
#ifdef ENGINE_TARGET_DX10
	if ((dwActionFlags & (SETTING_NEEDS_RESET | SETTING_NEEDS_WINDOWSIZE)) != 0)
	{
		V( e_DoWindowSize(e_OptionState_GetActive()->get_nFrameBufferWidth(), e_OptionState_GetActive()->get_nFrameBufferHeight()) );
		V( dx10_ResizePrimaryTargets() );	// For changing resolutions in full-screen.
	}
#endif

	if ( dwActionFlags & SETTING_NEEDS_WINDOWSIZE )
	{
		V( e_DoWindowSize(e_OptionState_GetActive()->get_nFrameBufferWidth(), e_OptionState_GetActive()->get_nFrameBufferHeight()) );
	}

	if ( dwActionFlags & SETTING_NEEDS_FXRELOAD )
	{
		e_MaterialsRefresh( bForceSync );
	}

	// CHB 2007.08.23 - Tell the UI the display has changed
	// (so it can acquire the mouse).
	if (dwActionFlags != 0)
	{
		e_UIDisplayChange(true);
	}

	// Restores sampler state defaults (for anisotropic and trilinear filtering).
	V( dx9_DefaultStatesInit() );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_GetDefaultMonitorHandle( HMONITOR & hMon )
{
	hMon = MonitorFromWindow( AppCommonGetHWnd(), MONITOR_DEFAULTTONEAREST );
	ASSERT_RETFAIL( hMon );
	return S_OK;
}