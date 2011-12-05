//----------------------------------------------------------------------------
// dxC_caps.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"

#include "appcommon.h"
#include "dxC_caps.h"
#include "dxC_profile.h"
#include "performance.h"
#include "e_budget.h"
#include "syscaps.h"	// CHB 2007.01.10
#include "globalindex.h"	// CHB 2007.08.01
#include "launcherinfo.h"
#include "dxC_pixo.h"

using namespace FSSE;

#define USE_DXDIAG 0

//-------------------------------------------------------------------------------------------------
// GLOBALS
//-------------------------------------------------------------------------------------------------
// D3D CAPS
D3DSTATUS gD3DStatus;
VideoDeviceInfo gtAdapterID;

PlatformCap gtPlatformCaps[ NUM_PLATFORM_CAPS ] =
{
	{ MAKEFOURCC('D','S','B','L'), "Disabled", FALSE },	// DX9CAP_DISABLE stub
#include "..\\dx9\\dx9_caps_def.h"
};

static const int MAX_EXCEPTION_STRING_LENGTH = 128;

// CHB 2007.01.02
struct PlatformCapValue
{
	DWORD nValue;
	DWORD nMin;
	DWORD nMax;

	PlatformCapValue(void) throw()
	{
		nMin = 0;
		nMax = static_cast<DWORD>(-1);
	}
};
PlatformCapValue gtPlatformCapValues[ NUM_PLATFORM_CAPS ]; //KMNV TODO: Need to fill in for dx10


// simulate max tech
VERTEX_SHADER_VERSION geDebugMaxVS = VS_INVALID;	// CHB 2007.02.27 - This debug setting should be a RenderFlag
PIXEL_SHADER_VERSION  geDebugMaxPS = PS_INVALID;	// CHB 2007.02.27 - This debug setting should be a RenderFlag


//-------------------------------------------------------------------------------------------------
// DEFINES
//-------------------------------------------------------------------------------------------------
//
// Exception information
//
#define FACILITY_VISUALCPP  ((LONG)0x6d)
#define VcppException(sev,err)  ((sev) | (FACILITY_VISUALCPP<<16) | err)

#define EXPAND_MEM_STR(x)				x, sizeof(x)/sizeof(TCHAR)

//-------------------------------------------------------------------------------------------------
//IMPLEMENTATIONS
//-------------------------------------------------------------------------------------------------

#if ISVERSION(DEVELOPMENT)
static void sUpdateDXRuntime()
{
	BOOL bShortcut = FALSE;
	BOOL bNoPopups = ( PStrStr(GetCommandLine(),"-nopopups") != NULL );

	static const OS_PATH_CHAR sszPath[] = OS_PATH_TEXT("3rd Party\\DirectX Runtime\\");
	static const OS_PATH_CHAR sszEXE[]  = OS_PATH_TEXT("Full Runtime Installer\\DXSETUP.exe");
	static const OS_PATH_CHAR sszLink[] = OS_PATH_TEXT("Install Latest DirectX Runtime.exe.lnk");
	static OS_PATH_CHAR szInstall[ DEFAULT_FILE_WITH_PATH_SIZE ];

	const OS_PATH_CHAR * pszTail = bShortcut ? sszLink : sszEXE;
	PStrPrintf( szInstall, DEFAULT_FILE_WITH_PATH_SIZE, OS_PATH_TEXT("%s%s"), sszPath, pszTail );

	int nLen;
	const OS_PATH_CHAR * pszAppRoot = AppCommonGetRootDirectory( &nLen );
	ASSERT_RETURN( nLen <= DEFAULT_FILE_WITH_PATH_SIZE );

	BOOL bResult = FALSE;
	if ( bShortcut )
	{
		OS_PATH_CHAR szShortcut[ DEFAULT_FILE_WITH_PATH_SIZE ];
		PStrPrintf(szShortcut, DEFAULT_FILE_WITH_PATH_SIZE, OS_PATH_TEXT("%s%s"), pszAppRoot, szInstall);
		OS_PATH_CHAR szCommand[ DEFAULT_FILE_WITH_PATH_SIZE ];

		V_DO_SUCCEEDED( PResolveShortcut( szShortcut, DEFAULT_FILE_WITH_PATH_SIZE, szCommand, DEFAULT_FILE_WITH_PATH_SIZE ) )
		{
			bResult = PExecuteCmdLine( szCommand );
		}
	}
	else
	{
		OS_PATH_CHAR szCommand[ DEFAULT_FILE_WITH_PATH_SIZE ];
		OS_PATH_CHAR szArgs[ DEFAULT_FILE_WITH_PATH_SIZE ] = OS_PATH_TEXT(" /silent");
		PStrPrintf(szCommand, DEFAULT_FILE_WITH_PATH_SIZE, OS_PATH_TEXT("%s%s%s"), pszAppRoot, szInstall, bNoPopups ? szArgs : OS_PATH_TEXT("") );
		bResult = PExecuteCmdLine( szCommand );
	}

	if ( ! bResult )
	{
		ErrorDialog( "Error updating DirectX runtime!  Make sure you've fully synced the entire \"%s\" directory.", WIDE_CHAR_TO_ASCII_FOR_DEBUG_TRACE_ONLY( sszPath ) );
	} else
	{
        if( ! bNoPopups )
        {
    		MessageBox( NULL, "Update is complete.  Please run the application again.", "Update Complete", MB_OK | MB_ICONINFORMATION );	// CHB 2007.08.01 - String audit: development
        }
	}
	// exit	
	//AppSwitchState(APP_STATE_EXIT);
	_exit(0);
}
#endif // DEVELOPMENT

static
void sNoDX10Message(void)
{
	BOOL bMsgBox = TRUE;

	GLOBAL_DEFINITION * pDefinition = (GLOBAL_DEFINITION *) DefinitionGetGlobal();
	if (!pDefinition)
		bMsgBox = FALSE;
	else if ( 0 != ( pDefinition->dwFlags & GLOBAL_FLAG_NO_POPUPS ) )
		bMsgBox = FALSE;

	if ( bMsgBox )
	{
		WCHAR szCap[1024];
		const WCHAR * pCaption1 = GlobalStringGetEx(GS_MSG_UNRECOVERABLE_ERROR, L"Unrecoverable Error");
		const WCHAR * pCaption2 = GlobalStringGetEx(GS_DIRECTX10, L"DirectX 10");
		const WCHAR * pText = GlobalStringGetEx(GS_MSG_MIN_SPEC_ISSUE_TITLE, L"Minimum System Requirements Not Met");
		PStrPrintf( szCap, _countof(szCap), L"%s - %s", pCaption1, pCaption2 );

		::MessageBoxW(AppCommonGetHWnd(), pText, szCap, MB_OK | MB_ICONERROR);
	}
	else
	{
#if ISVERSION(DEVELOPMENT)
		LogMessage( ERROR_LOG, "FATAL ERROR: Tried to run DX10 version, but DX10 is not supported by hardare!" );
#endif
	}
}

static
void sVersionMessage(void)
{
	BOOL bMsgBox = TRUE;

	GLOBAL_DEFINITION * pDefinition = (GLOBAL_DEFINITION *) DefinitionGetGlobal();
	if (!pDefinition)
		bMsgBox = FALSE;
	else if ( 0 != ( pDefinition->dwFlags & GLOBAL_FLAG_NO_POPUPS ) )
		bMsgBox = FALSE;

	WCHAR szMsg[1024];

	const WCHAR * pText = GlobalStringGetEx(GS_MSG_ERROR_D3D_VERSION_TEXT, L"Incompatible DirectX version error.");
	const WCHAR * pCaption = GlobalStringGetEx(GS_MSG_UNRECOVERABLE_ERROR, L"Error");
	PStrPrintf(szMsg, _countof(szMsg), pText, DXC_9_10( D3D_SDK_VERSION, D3D10_SDK_VERSION), DXC_9_10( D3DX_SDK_VERSION, D3DX10_SDK_VERSION));

#if ISVERSION(DEVELOPMENT)
	PStrCat(szMsg, L"\n\nUpdate to the current runtime now?", _countof(szMsg));
	int nType = MB_OKCANCEL;
#else
	int nType = MB_OK;
#endif

	INT_PTR nRet;
	if ( bMsgBox )
		nRet = ::MessageBoxW(AppCommonGetHWnd(), szMsg, pCaption, nType | MB_ICONERROR);	// CHB 2007.08.01 - String audit: USED IN RELEASE
	else
	{
#if ISVERSION(DEVELOPMENT)
		LogMessage( ERROR_LOG, "FATAL ERROR: %S", pText );
		LogMessage( ERROR_LOG, "Please update runtime to D3D version %d and D3DX version %d.", DXC_9_10(D3D_SDK_VERSION, D3D10_SDK_VERSION), DXC_9_10(D3DX_SDK_VERSION, D3DX10_SDK_VERSION) );
#endif
		nRet = IDCANCEL;
	}

#if ISVERSION(DEVELOPMENT)
	if ( nRet == IDOK )
	{
		sUpdateDXRuntime();
	}
#else
	REF(nRet);
#endif
}

BOOL e_CheckVersion()
{
	BOOL bValidVersion = FALSE;
	__try 
	{ 
		bValidVersion = DX9_BLOCK( D3DXCheckVersion( D3D_SDK_VERSION, D3DX_SDK_VERSION ) ) DX10_BLOCK( D3DX10CheckVersion( D3D10_SDK_VERSION, D3DX10_SDK_VERSION ) );
	} 
	__except( GetExceptionCode() == VcppException(ERROR_SEVERITY_ERROR, ERROR_MOD_NOT_FOUND) ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH )
	{
		sVersionMessage();
		return FALSE;
	}

	DX10_BLOCK( bValidVersion = true; ) //KMNV TODO: D3DX10CheckVersion doesn't seem to work

#ifdef ENGINE_TARGET_DX10
	// CML 2007.09.17 - Do a more robust interface test on DX10.
	//   This is done in order to trigger a better error message if someone runs the DX10 version on Vista
	//   with a non-DX10 card.
	if ( ! SupportsDX10() )
	{
		sNoDX10Message();
		return FALSE;
	}
#endif

	if ( ! bValidVersion )
	{
		sVersionMessage();
		return FALSE;
#if 0
		if ( c_GetToolMode() )
		{
			return FALSE;
		}
		else
		{
			return FALSE;
		}
#endif
	}

	return TRUE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

DWORD e_GetEngineTarget4CC()
{
	return DXC_9_10( DT_ENGINE_TARGET_DX9_ID, DT_ENGINE_TARGET_DX10_ID );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int e_GetMaxPhysicalVideoMemoryMB()
{
	return SysCaps_Get().dwPhysicalVideoMemoryBytes / BYTES_PER_MB;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int dxC_CapsGetMaxSquareTextureResolution()
{
	return min( dx9_CapGetValue( DX9CAP_MAX_TEXTURE_WIDTH ), dx9_CapGetValue( DX9CAP_MAX_TEXTURE_HEIGHT ) );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

void e_DebugOutputPlatformCaps()
{
	DebugString( OP_DEBUG, "Platform Caps" );

	for ( int i = DX9CAP_DISABLE+1; i < NUM_PLATFORM_CAPS; i++ )
	{
		CHAR szFourCC[ FOURCCSTRING_LEN ];
		dx9_CapGetFourCCString( (PLATFORM_CAP)i, szFourCC, FOURCCSTRING_LEN );
		const CHAR * pszDesc = dx9_CapGetDescription( (PLATFORM_CAP)i );
		DWORD dwValue = dx9_CapGetValue( (PLATFORM_CAP)i );
		DebugString( OP_DEBUG, "%4s - 0x%08x - %10d - %s", szFourCC, dwValue, dwValue, pszDesc );
		LogMessage( "%4s - 0x%08x - %10d - %s", szFourCC, dwValue, dwValue, pszDesc );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#if USE_DXDIAG
static void sReportDXDiagError( const char * pszError, ... )
{
	va_list args;
	va_start(args, pszError);

	ErrorDialogV( pszError, args );	// CHB 2007.08.01 - String audit: not used
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

// CHB 2007.01.02
DWORD dx9_CapGetValue( const PLATFORM_CAP eCap )
{
	ASSERT_RETNULL( IS_VALID_PLATFORM_CAP_TYPE( eCap ) );
	return gtPlatformCapValues[ eCap ].nValue;
}

PRESULT dx9_PlatformCapSetValue( const PLATFORM_CAP eCap, DWORD dwValue )
{
	ASSERT_RETINVALIDARG( IS_VALID_PLATFORM_CAP_TYPE( eCap ) );
	// CHB 2007.01.02
	dwValue = max(dwValue, gtPlatformCapValues[eCap].nMin);
	dwValue = min(dwValue, gtPlatformCapValues[eCap].nMax);
	gtPlatformCapValues[eCap].nValue = dwValue;
	return S_OK;
}

// CHB 2007.01.02
PRESULT dx9_PlatformCapSetMinMax( const PLATFORM_CAP eCap, DWORD nMin, DWORD nMax )
{
	ASSERT_RETINVALIDARG( IS_VALID_PLATFORM_CAP_TYPE( eCap ) );
	ASSERT_RETINVALIDARG( nMin <= nMax );
	gtPlatformCapValues[eCap].nMin = nMin;
	gtPlatformCapValues[eCap].nMax = nMax;

	// Apply the new min/max
	V( dx9_PlatformCapSetValue( eCap, dx9_CapGetValue( eCap ) ) );

	return S_OK;
}

// CHB 2007.01.18
PRESULT dx9_PlatformCapGetMinMax( const PLATFORM_CAP eCap, DWORD & nMin, DWORD & nMax )
{
	ASSERT_RETINVALIDARG( IS_VALID_PLATFORM_CAP_TYPE( eCap ) );
	nMin = gtPlatformCapValues[eCap].nMin;
	nMax = gtPlatformCapValues[eCap].nMax;
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sGetInt64Value( IDxDiagContainer* pObject, WCHAR* wstrName, ULONGLONG* pullValue )
{
	PR_ASSERT_REGION( FALSE );

	VARIANT var;
	VariantInit( &var );

	V_RETURN( pObject->GetProp( wstrName, &var ) );

	// 64-bit values are stored as strings in BSTRs
	if ( var.vt != VT_BSTR )
		return E_INVALIDARG;

	*pullValue = _wtoi64( var.bstrVal );
	VariantClear( &var );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static PRESULT sGetStringValue( IDxDiagContainer* pObject, WCHAR* wstrName, TCHAR* strValue, int nStrLen )
{
	HRESULT hr;
	VARIANT var;
	VariantInit( &var );

	if ( FAILED( hr = pObject->GetProp( wstrName, &var ) ) )
		return hr;

	if ( var.vt != VT_BSTR )
		return E_INVALIDARG;

#ifdef _UNICODE
	PStrCopy( strValue, var.bstrVal, nStrLen-1 );
#else
	PStrCvt( strValue, var.bstrVal, nStrLen );   
#endif
	strValue[nStrLen-1] = TEXT('\0');
	VariantClear( &var );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

HRESULT dx9_DetectNonD3DCapabilities()
{
	TIMER_START( "DXDiag caps detection" )

#if !USE_DXDIAG
	// We will get all the necessary data using WMI
	// CHB 2007.02.27 - This occurs in e_common.cpp now.
	//SysCaps_Init();
#else
	SPDXDIAGPROVIDER  pDXDiagProvider;
	SPDXDIAGCONTAINER pDXDiagRoot;

	V_DO_FAILED( CoInitialize( NULL ) )
	{
		sReportDXDiagError( "COM already initialized!" );
		return E_FAIL;
	}

	// grab some caps from WMI
	dx9_GetWMICaps();

	V_DO_FAILED( CoCreateInstance( CLSID_DxDiagProvider,
									 NULL,
									 CLSCTX_INPROC_SERVER,
									 IID_IDxDiagProvider,
									 (LPVOID*) &pDXDiagProvider) )
	{
		return E_FAIL;
	}
	if ( pDXDiagProvider == NULL )
	{
		return E_FAIL;
	}

	// Fill out a DXDIAG_INIT_PARAMS struct and pass it to IDxDiagContainer::Initialize
	// Passing in TRUE for bAllowWHQLChecks, allows dxdiag to check if drivers are 
	// digital signed as logo'd by WHQL which may connect via internet to update 
	// WHQL certificates.    
	DXDIAG_INIT_PARAMS dxDiagInitParam;
	ZeroMemory( &dxDiagInitParam, sizeof(DXDIAG_INIT_PARAMS) );

	dxDiagInitParam.dwSize                  = sizeof(DXDIAG_INIT_PARAMS);
	dxDiagInitParam.dwDxDiagHeaderVersion   = DXDIAG_DX9_SDK_VERSION;
	dxDiagInitParam.bAllowWHQLChecks        = FALSE;
	dxDiagInitParam.pReserved               = NULL;

	V_DO_FAILED( pDXDiagProvider->Initialize( &dxDiagInitParam ) )
		return E_FAIL;

	V_DO_FAILED( pDXDiagProvider->GetRootContainer( &pDXDiagRoot ) )
		return E_FAIL;

	// grab the data of interest

	// Get the IDxDiagContainer object called "DxDiag_SystemInfo".
	// This call may take some time while dxdiag gathers the info.
	SPDXDIAGCONTAINER pContainer      = NULL;
	SPDXDIAGCONTAINER pObject         = NULL;
	V_DO_FAILED( pDXDiagRoot->GetChildContainer( L"DxDiag_SystemInfo", &pObject )
		return E_FAIL;
	if ( pObject == NULL )
		return E_FAIL;
	V_DO_FAILED( sGetInt64Value( pObject, L"ullPhysicalMemory", &gtGeneralHardwareCaps.dwlPhysicalSystemMemoryBytes ) ) )
		return E_FAIL;
	pObject = NULL;

	// Get the IDxDiagContainer object called "DxDiag_DisplayDevices".
	// This call may take some time while dxdiag gathers the info.
	DWORD nInstanceCount;
	V_DO_FAILED( pDXDiagRoot->GetChildContainer( L"DxDiag_DisplayDevices", &pContainer ) ) )
		return E_FAIL;
	V_DO_FAILED( pContainer->GetNumberOfChildContainers( &nInstanceCount ) ) )
		return E_FAIL;

	WCHAR wszContainer[256];


	
	// THIS ASSUMES THE DISPLAY DEVICE WE WANT IS IN SLOT 0
	int nItem = 0;



	V_DO_FAILED( pContainer->EnumChildContainerNames( nItem, wszContainer, 256 ) )
		return E_FAIL;
	V_DO_FAILED( pContainer->GetChildContainer( wszContainer, &pObject ) )
		return E_FAIL;
	if ( pObject == NULL )
	{
		sReportDXDiagError( "Failed to get display device info for device 0" );
		return E_FAIL;
	}

	TCHAR szMemory[ 64 ];
	V_DO_FAILED( sGetStringValue( pObject, L"szDisplayMemoryEnglish", EXPAND_MEM_STR(szMemory) ) ) )
		return E_FAIL;
	float fDisplayMemoryMB;
	float fMemoryMult = 1.f;
	int nFields = sscanf_s( szMemory, "%f MB", &fDisplayMemoryMB );
	if ( nFields != 1 )
	{
		nFields = sscanf_s( szMemory, "%fMB", &fDisplayMemoryMB );
		fMemoryMult = 1.f;
	}
	if ( nFields != 1 )
	{
		nFields = sscanf_s( szMemory, "%f GB", &fDisplayMemoryMB );
		fMemoryMult = 1024.f;
	}
	if ( nFields != 1 )
	{
		nFields = sscanf_s( szMemory, "%fGB", &fDisplayMemoryMB );
		fMemoryMult = 1024.f;
	}
	if ( nFields != 1 )
	{
		nFields = sscanf_s( szMemory, "%f KB", &fDisplayMemoryMB );
		fMemoryMult = 1.f / 1024.f;
	}
	if ( nFields != 1 )
	{
		nFields = sscanf_s( szMemory, "%fKB", &fDisplayMemoryMB );
		fMemoryMult = 1.f / 1024.f;
	}
	if ( nFields != 1 )
	{
		sReportDXDiagError( "Couldn't parse display memory string:\n\n%s", szMemory );
		return E_FAIL;
	}
	gtGeneralHardwareCaps.dwPhysicalVideoMemoryBytes = DWORD( fDisplayMemoryMB * fMemoryMult ) * BYTES_PER_MB;

	pObject = NULL;
#endif

	BOOL bTrace = LogGetTrace( PERF_LOG );
	LogSetTrace( PERF_LOG, TRUE );
	LogMessage( PERF_LOG, "D3D: Detected physical video  mem via DXDiag: %d MB", SysCaps_Get().dwPhysicalVideoMemoryBytes / BYTES_PER_MB );
	//LogMessage( PERF_LOG, "D3D: Detected virtual  video  mem via DXDiag: %d MB", SysCaps_Get().dwVirtualVideoMemoryBytes / BYTES_PER_MB );
	LogMessage( PERF_LOG, "D3D: Detected physical system mem via DXDiag: %d MB", (DWORD)( SysCaps_Get().dwlPhysicalSystemMemoryBytes / BYTES_PER_MB ) );
	LogSetTrace( PERF_LOG, bTrace );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

DWORD e_GetAvailTextureMemory()
{
	// takes the est total video memory
	int nTotal = e_CapGetValue( CAP_TOTAL_ESTIMATED_VIDEO_MEMORY_MB );

	// subtracts estimated default-only memory
	//nTotal -= gD3DStatus.dwVidMemTotal / BYTES_PER_MB;

	// subtracts budget for vb/ib
	nTotal = (int)( nTotal * ( 1.f - TEXTURE_BUDGET_EST_GEOM_TEX_RATIO ) );

	// returns result
	return nTotal;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_DetectAdapter( DXCADAPTER tAdapter )
{
	extern VideoDeviceInfo gtAdapterID;

#ifdef ENGINE_TARGET_DX9
	D3DADAPTER_IDENTIFIER9 tAdapterID;
	V_RETURN( dx9_GetD3D()->GetAdapterIdentifier( tAdapter, 0, &tAdapterID ) );
	V_RETURN( gtAdapterID.FromD3D( &tAdapterID ) );
#else // DX10
	DXGI_ADAPTER_DESC adapterDesc;
	ASSERT_RETINVALIDARG( tAdapter );
	V_RETURN( tAdapter->GetDesc(&adapterDesc) );
	V_RETURN( gtAdapterID.FromD3D( &adapterDesc ) );
#endif

	LogMessage( PERF_LOG, "\n### DIRECT3D ADAPTER/DRIVER IDS ###\n"
		"### Description:    %s\n"
		"### Driver Version: %08x %08x\n"
		"### Vendor ID:      %04x\n"
		"### Device ID:      %04x\n"
		"### Subset ID:      %08x\n"
		"### Revision:       %04x\n",
		gtAdapterID.szDeviceName,
		gtAdapterID.DriverVersion.HighPart,
		gtAdapterID.DriverVersion.LowPart,
		gtAdapterID.VendorID,
		gtAdapterID.DeviceID,
		gtAdapterID.SubSysID,
		gtAdapterID.Revision );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT VideoDeviceInfo::FromD3D( const void * pD3DAdapter)
{
	ASSERT_RETINVALIDARG( pD3DAdapter );

#ifdef ENGINE_TARGET_DX9
	const D3DADAPTER_IDENTIFIER9* pInfo = static_cast<const D3DADAPTER_IDENTIFIER9*>(pD3DAdapter);
	VendorID = pInfo->VendorId;
	DeviceID = pInfo->DeviceId;
	SubSysID = pInfo->SubSysId;
	Revision = pInfo->Revision;
	DriverVersion = pInfo->DriverVersion;
	PStrCopy( szDeviceName, MAX_NAME_LEN, pInfo->Description, MAX_DEVICE_IDENTIFIER_STRING );
#else // DX10
	const DXGI_ADAPTER_DESC* pDesc = static_cast<const DXGI_ADAPTER_DESC*>(pD3DAdapter);
	VendorID = pDesc->VendorId;
	DeviceID = pDesc->DeviceId;
	SubSysID = pDesc->SubSysId;
	Revision = pDesc->Revision;
	PStrCvt( szDeviceName, pDesc->Description, MAX_NAME_LEN );
	// CML TODO - How do I get the video driver version in DX10?
	DriverVersion.QuadPart = 0L;
#endif

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_GetMaxTextureSizes( int & nWidth, int & nHeight )
{
	nWidth = (int)dx9_CapGetValue( DX9CAP_MAX_TEXTURE_WIDTH );
	nHeight = (int)dx9_CapGetValue( DX9CAP_MAX_TEXTURE_HEIGHT );
	if ( e_IsNoRender() || dxC_IsPixomaticActive() )
	{
		nWidth = 2048;
		nHeight = 2048;
	}
	nWidth  = MIN( 2048, nWidth );
	nHeight = MIN( 2048, nHeight );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL e_ShouldCreateMinimalDevice()
{
	return ! dxC_ShouldUsePixomatic();
}