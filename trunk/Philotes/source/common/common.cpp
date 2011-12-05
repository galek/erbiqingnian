//----------------------------------------------------------------------------
// common.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------

#include "appcommon.h"
#include "filepaths.h"
#include "winos.h"
#include "email.h"
#include "crc.h"
#include "hlocks.h"
#include "region.h"
#include "config.h"

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEBUG_VERSION)
#pragma message("compiling: DEBUG_VERSION")
#endif
#if ISVERSION(RETAIL_VERSION)
#pragma message("compiling: RETAIL_VERSION")
#endif
#if ISVERSION(OPTIMIZED_VERSION)
#pragma message("compiling: OPTIMIZED_VERSION")
#endif
#if ISVERSION(DEVELOPMENT)
#pragma message("compiling: DEVELOPMENT")
#endif
#if ISVERSION(CHEATS_ENABLED)
#pragma message("compiling: CHEATS_ENABLED")
#endif
#if ISVERSION(DEMO_VERSION)
#pragma message("compiling: DEMO_VERSION")
#endif
#if ISVERSION(CLIENT_ONLY_VERSION)
#pragma message("compiling: CLIENT_ONLY_VERSION")
#endif
#if ISVERSION(SERVER_VERSION)
#pragma message("compiling: SERVER_VERSION")
#endif
#if ISVERSION(US_SERVER_VERSION)
#pragma message("compiling: US_SERVER_VERSION")
#endif
#if ISVERSION(ASIA_SERVER_VERSION)
#pragma message("compiling: ASIA_SERVER_VERSION")
#endif


//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
#define DEFAULT_CRC_KEY					0x04c11db7


//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
// Math stuffs
float			gfCos360Tbl[360];
float			gfSin360Tbl[360];
float			gfSqrtFractionTbl[101];
//RAND			gGlobalRand;
APP_COMMON		gtAppCommon;
APP_GAME		g_AppGame;


DWORD COMMON_DATATABLE_INITDB = INVALID_ID;	// CHB 2007.03.08
DWORD COMMON_DATATABLE_SOUNDOVERRIDES = INVALID_ID;
DWORD COMMON_DATATABLE_SOUNDVCAS = INVALID_ID;
DWORD COMMON_DATATABLE_SOUNDVCASETS = INVALID_ID;
DWORD COMMON_DATATABLE_SOUNDBUSES = INVALID_ID;
// replacing the #ifdef HAMMER for libraries
BOOL gbToolMode = FALSE;

#ifdef INTEL_CHANGES
CPU_INFO gtCPUInfo;
#endif


//----------------------------------------------------------------------------
// MATH
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CommonInit(
	HINSTANCE hInstance, LPSTR pCmdLine, WCHAR * strRootDirectoryOverride)
{
#ifdef INTEL_CHANGES
	CPUInfo(gtCPUInfo);
#else
	CPU_INFO info;
	CPUInfo(info);
#endif

	InitCritSectSpinWaitInterval();
	MemorySystemInit(pCmdLine);
	InitHlockSystem(NULL);
	CRCInit(DEFAULT_CRC_KEY);
	FileIOInit();
	AsyncFileSystemInit(0,0, strRootDirectoryOverride);
	if(!strRootDirectoryOverride)
	{
		LogInit();
	}
	//RandInit(gGlobalRand);
	ExcelCommonInit();

	// angle Table
	for (int ii = 0; ii < 360; ii++)
	{
		double dAngle = (ii * PI) / 180.0;
		gfCos360Tbl[ii] = (float)cos(dAngle);
		gfSin360Tbl[ii] = (float)sin(dAngle);
	}

	for (int ii = 0; ii <= 100; ii++)
	{
		gfSqrtFractionTbl[ii] = sqrt(ii / 100.0f);
	}

	//ZeroMemory( &gtAppCommon, sizeof(APP_COMMON) );
	gtAppCommon.m_hInstance = hInstance;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void CommonClose(
	void)
{
	LogFlush();

	LogFree();
	AsyncFileSystemFree();
	FileIOFree();
	FreeHlockSystem();
	MemorySystemFree();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AppCommonSetDirToRoot(
	void)
{
	if ( ! gtAppCommon.m_szRootDirectory[ 0 ] )
		return;
	PSetCurrentDirectory( gtAppCommon.m_szRootDirectory );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AppGameSet( 
	APP_GAME eAppGame)
{
	g_AppGame = eAppGame;
	FilePathInitForApp(eAppGame);
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static
const WCHAR * sGetGlobalString(unsigned /*nString*/)
{
	return 0;
}

static APP_GET_GLOBAL_STRING_CALLBACK * s_pGetGlobalStringCallback = &sGetGlobalString;

const WCHAR * AppCommonGetGlobalString(unsigned nString, const WCHAR * pDefault)
{
	const WCHAR * pStr = (*s_pGetGlobalStringCallback)(nString);
	return (pStr != 0) ? pStr : pDefault;
}

void AppCommonSetGlobalStringCallback(APP_GET_GLOBAL_STRING_CALLBACK * pCallback)
{
	ASSERT_RETURN(pCallback != 0);
	s_pGetGlobalStringCallback = pCallback;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AppIsBeta(
	void)
{
	DWORD dwPackage = ExcelGetVersionPackage();
	return ( dwPackage & EXCEL_PACKAGE_VERSION_BETA ) != 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AppIsDemo(
	void)
{
	DWORD dwPackage = ExcelGetVersionPackage();
	return ( dwPackage & EXCEL_PACKAGE_VERSION_DEMO ) != 0;
}


//----------------------------------------------------------------------------
BOOL AppCommonSingleplayerOnly(
	void)
{
	return gtAppCommon.m_bSingleplayerOnly || AppIsDemo();
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AppIsSinglePlayer(
	void)
{
#if ISVERSION(SERVER_VERSION)
	return FALSE;
#else
	return AppGetType() == APP_TYPE_SINGLE_PLAYER;
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AppIsMultiplayer(
	void)
{
#if ISVERSION(SERVER_VERSION)
	return TRUE;
#else
	return AppIsSinglePlayer() == FALSE;
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AppSetActive(
	BOOL bActive)
{
	CLT_VERSION_ONLY( gtAppCommon.m_bActive = bActive );
	SVR_VERSION_ONLY(REF(bActive));
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AppIsActive(
	void)
{
	CLT_VERSION_ONLY( return gtAppCommon.m_bActive );
	SVR_VERSION_ONLY( return TRUE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AppSetMinimized(
	BOOL bMinimized)
{
	CLT_VERSION_ONLY( gtAppCommon.m_bMinimized = bMinimized );
	SVR_VERSION_ONLY(REF(bMinimized));
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AppIsMinimized(
	void)
{
	CLT_VERSION_ONLY( return gtAppCommon.m_bMinimized );
	SVR_VERSION_ONLY( return FALSE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AppSetMenuLoop(
	BOOL bMenuLoop)
{
	CLT_VERSION_ONLY( gtAppCommon.m_bMenuLoop = bMenuLoop );
	SVR_VERSION_ONLY(REF(bMenuLoop));
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AppIsMenuLoop(
	void)
{
	CLT_VERSION_ONLY( return gtAppCommon.m_bMenuLoop );
	SVR_VERSION_ONLY( return FALSE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AppSetMoveWindow(
	BOOL bMoveWindow)
{
	CLT_VERSION_ONLY( gtAppCommon.m_bSizeMove = bMoveWindow );
	SVR_VERSION_ONLY(REF(bMoveWindow));
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AppIsMoveWindow(
	void)
{
	CLT_VERSION_ONLY( return gtAppCommon.m_bSizeMove );
	SVR_VERSION_ONLY( return FALSE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AppAppliesMinDataPatchesInLauncher( 
	void)
{
	if (AppIsHellgate())
	{
		const REGION_DATA* pRegionData = RegionGetData(APP_GAME_HELLGATE, RegionGetCurrent());
		ASSERT_RETFALSE(pRegionData != NULL);

		return pRegionData->bStartLauncherUponUpdate;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL AppCommonAllowAssetUpdate()
{
	GLOBAL_DEFINITION * pGlobals = DefinitionGetGlobal();

	if ( c_GetToolMode() )
		return TRUE;
	else if ( AppTestDebugFlag( ADF_FORCE_ALLOW_ASSET_UPDATE ) )
		return TRUE;
	else if ( AppCommonIsAnyFillpak() )
		return TRUE;
	else if ( ( ! pGlobals || TEST_MASK( pGlobals->dwFlags, GLOBAL_FLAG_GENERATE_ASSETS_IN_GAME ) )
				&& AppCommonGetUpdatePakFile() )
		return TRUE;
	else
		return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void AppCommonSetExcelPackageAll()
{
	gtAppCommon.m_dwExcelManifestPackage = EXCEL_PACKAGE_VERSION_ALL;
}
