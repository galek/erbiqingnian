//----------------------------------------------------------------------------
// e_common.cpp
//
// - Implementation for common engine code
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDE
//----------------------------------------------------------------------------
#include "e_pch.h"
#include "e_common.h"
#include "e_budget.h"
#include "e_features.h"
#include "e_main.h"
#include "e_settings.h"
#include "e_dpvs.h"
#include "e_optionstate.h"	// CHB 2007.02.27
#include "e_caps.h"			// CHB 2007.02.27
#include "e_initdb.h"		// CHB 2007.03.08
#include "e_minspec.h"		// CHB 2007.03.14
#include "e_compatdb.h"		// CHB 2007.03.30
#include "e_region.h"
//#include "appcommon.h"
#include "syscaps.h"
#include "prime.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

RAND	gtEngineRand;

#if ISVERSION(DEVELOPMENT)
const char gszDrawLayerNames[ NUM_DRAW_LAYERS ][ DRAW_LAYER_NAME_MAX_LEN ] =
{
	"DRAW_LAYER_GENERAL",
	"DRAW_LAYER_ENV",
	"DRAW_LAYER_SCREEN_EFFECT"
};
#endif // DEVELOPMENT

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

PRESULT e_InitPlatformGlobals();
void e_FeatureLines_Init(void);
void e_FeatureLines_Deinit(void);

//----------------------------------------------------------------------------
// IMPLEMENTATION
//----------------------------------------------------------------------------

static
PRESULT e_InitGlobals()
{
	RandInit( gtEngineRand );

	V( e_InitPlatformGlobals() );
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_InitStarterCaps(void)
{
	// CML 2007.01.29 -- We have to create a device to get the device caps, to set meaningful initial values

	HINSTANCE hInstance = AppCommonGetHInstance();
	HWND hWnd = NULL;
	const char szName[] = "initial";

	if ( ! e_IsNoRender() )
	{
		V_RETURN( e_GatherGeneralCaps() );

#ifdef ENGINE_TARGET_DX9
		if ( e_ShouldCreateMinimalDevice() )
		{
			WNDCLASSEX	tWndClass;

			tWndClass.cbSize = sizeof( tWndClass );
			tWndClass.style = CS_HREDRAW | CS_VREDRAW;
			tWndClass.lpfnWndProc = DefWindowProc;
			tWndClass.cbClsExtra = 0;
			tWndClass.cbWndExtra = 0;
			tWndClass.hInstance = hInstance;
			tWndClass.hIcon = LoadIcon( NULL, IDI_APPLICATION );
			tWndClass.hCursor = LoadCursor( NULL, IDC_ARROW );
			tWndClass.hbrBackground = (HBRUSH)GetStockObject( BLACK_BRUSH );
			tWndClass.lpszMenuName = NULL;
			tWndClass.lpszClassName = szName;
			tWndClass.hIconSm = LoadIcon( NULL, IDI_APPLICATION );

			RegisterClassEx( &tWndClass );

			// create a dummy window to attach D3D to
			hWnd = CreateWindowEx( 0, szName, szName, WS_POPUP | WS_CAPTION,
				0, 0, 100, 100,
				NULL, NULL, hInstance, NULL );

			ASSERT_RETVAL( hWnd, E_FAIL );

			V( e_DeviceCreateMinimal( hWnd ) );
		}
#endif

		V( e_GatherCaps() );

#ifdef ENGINE_TARGET_DX9
		if ( e_ShouldCreateMinimalDevice() )
		{
			V( e_DeviceRelease() );
			DestroyWindow(hWnd);
			UnregisterClass(szName, hInstance);
		}
#endif
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_CommenceInit(void)
{
	V( e_InitGlobals() );
	e_SetDesktopSize();

	//
	SysCaps_Init();

	// Create D3D object.
	V( e_PlatformInit() );

	// Depends on:
	//	* e_InitStarterCaps(): Needs caps to be initialized.
	V( e_OptionState_Init() );

	// Initialize StarterCaps from "minimal device"
	// Depends on:
	//	* e_PlatformInit(): Cannot initialize starter caps without a D3D object.
	V( e_InitStarterCaps() );

	// Apply Caps restrictions from CompatDB

	// Initialize FeatureLine (stops) system.
	e_FeatureLines_Init();

	// set initial render flag settings
	V( e_InitializeRenderFlags() );

	V( e_ModelBudgetInit() );

	V( e_TextureBudgetInit() );

	V( e_ReaperInit() );

	e_UploadManagerGet().Init();
	e_UploadManagerGet().SetLimit( 2 * 1024 * 1024 );

	e_FeaturesInit();

	V( e_DPVS_Init() );

	e_CompatDB_Init();	// CML 2007.10.12

	V( e_InitDB_Init() );

	if (!AppCommonGetNoMinSpecCheck())	// CHB 2007.09.21 - that's a double negative
	{
		e_MinSpec_Check();
	}

	V( e_RegionsReset() );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_ConcludeInit(void)
{
	CComPtr<OptionState> pState;
	V_RETURN(e_OptionState_CloneActive(&pState));

	V(e_InitWindowSize(pState));

	V(e_OptionState_CommitToActive(pState));
	e_OptionState_ResumeCommit();
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_Deinit(void)
{
	e_FeatureLines_Deinit();
	V( e_OptionState_Deinit() );
	e_CompatDB_Deinit();
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

bool e_IsNoRender(void)
{
	return !!( ( AppCommonIsAnyFillpak() && ! AppCommonIsFillpakInConvertMode() ) ||
			   AppGetAllowFillLoadXML() ||
			   AppCommonGetDumpSkillIcons() );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

RAND & e_GetEngineOnlyRand()
{
	return gtEngineRand;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int e_GetResolutionDownsamples( int nFromWidth, int nFromHeight, int nToWidth, int nToHeight )
{
	const int MAX_ITERATIONS = 32;
	int i;
	for ( i = 0; i < MAX_ITERATIONS; i++ )
	{
		if ( ( nFromWidth  >> i ) <= nToWidth  &&
			( nFromHeight >> i ) <= nToHeight )
			break;
	}
	ASSERT_RETZERO( i < MAX_ITERATIONS );
	return i;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

const TCHAR * e_GetTargetName()
{
	return ENGINE_TARGET_NAME;
}
