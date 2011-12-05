//--------------------------------------------------------------------------------------
// File: gameexplorerhelper.cpp
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
//#include "stdafx.h"

#define _WIN32_DCOM
#define _CRT_SECURE_NO_DEPRECATE

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)       { if(p) { delete (p);     (p)=NULL; } }
#endif    
#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] (p);   (p)=NULL; } }
#endif    
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }
#endif

#define UNICODE

#include "gameexplorerhelper.h"

#ifdef GAME_EXPLORER_ENABLED

#include <windows.h>
#include <shlobj.h>
#include <rpcsal.h>
#include <gameux.h>
#include <crtdbg.h>
#include <msi.h>
#include <msiquery.h>
#include <strsafe.h>
#include <assert.h>
#include <wbemidl.h>
#include <objbase.h>
#include <shellapi.h>
#include <shlobj.h>

#define NO_SHLWAPI_STRFCNS
#include <shlwapi.h>

// Uncomment to get a debug messagebox
//#define SHOW_DEBUG_MSGBOXES

//--------------------------------------------------------------------------------------
// Forward declarations 
//--------------------------------------------------------------------------------------
LPWSTR GetPropertyFromMSI( MSIHANDLE hMSI, LPCWSTR szPropName );
HRESULT ConvertStringToGUID( const WCHAR* strSrc, GUID* pGuidDest );


//--------------------------------------------------------------------------------------
// This stores the install location, generates a instance GUID one hasn't been set, and 
// sets up the CustomActionData properties for the deferred custom actions
//--------------------------------------------------------------------------------------
UINT __stdcall SetMSIGameExplorerProperties( MSIHANDLE hModule ) 
{
	HRESULT hr;
	GUID guidGameInstance;
	bool bFoundExistingGUID = false;
	WCHAR szCustomActionData[1024] = {0};
	WCHAR strAbsPathToGDF[1024] = {0};
	WCHAR strGameInstanceGUID[128] = {0};
	WCHAR* szInstallDir = NULL;
	WCHAR* szALLUSERS = GetPropertyFromMSI(hModule, L"ALLUSERS");
	WCHAR* szRelativePathToGDF = GetPropertyFromMSI(hModule, L"RelativePathToGDF");
	WCHAR* szProductCode = GetPropertyFromMSI(hModule, L"ProductCode");

	// See if the install location property is set.  If it is, use that.  
	// Otherwise, get the property from TARGETDIR
	bool bGotInstallDir = false;
	if( szProductCode )
	{
		DWORD dwBufferSize = 1024;
		szInstallDir = new WCHAR[dwBufferSize];
		if( ERROR_SUCCESS == MsiGetProductInfo( szProductCode, INSTALLPROPERTY_INSTALLLOCATION, szInstallDir, &dwBufferSize ) )
			bGotInstallDir = true;
	}
	if( !bGotInstallDir )
		szInstallDir = GetPropertyFromMSI(hModule, L"TARGETDIR");

	// Set the ARPINSTALLLOCATION property to the install dir so that 
	// the uninstall custom action can have it when getting the INSTALLPROPERTY_INSTALLLOCATION
	MsiSetPropertyW( hModule, L"ARPINSTALLLOCATION", szInstallDir ); 

	// Get game instance GUID if it exists
	StringCchCopy( strAbsPathToGDF, 1024, szInstallDir );
	StringCchCat( strAbsPathToGDF, 1024, szRelativePathToGDF );
	hr = RetrieveGUIDForApplicationW( strAbsPathToGDF, &guidGameInstance );
	if( FAILED(hr) || strGameInstanceGUID[0] == 0 )
	{
		// If GUID is not there, then generate GUID
		CoCreateGuid( &guidGameInstance );
	}
	else
	{
		bFoundExistingGUID = true;
	}
	StringFromGUID2( guidGameInstance, strGameInstanceGUID, 128 );

	// Set the CustomActionData property for the "AddToGameExplorerUsingMSI" deferred custom action.
	StringCchCopy( szCustomActionData, 1024, szInstallDir );
	StringCchCat( szCustomActionData, 1024, szRelativePathToGDF );
	StringCchCat( szCustomActionData, 1024, L"|" );
	StringCchCat( szCustomActionData, 1024, szInstallDir );
	StringCchCat( szCustomActionData, 1024, L"|" );
	if( szALLUSERS && (szALLUSERS[0] == '1' || szALLUSERS[0] == '2') )  
		StringCchCat( szCustomActionData, 1024, L"3" );
	else
		StringCchCat( szCustomActionData, 1024, L"2" );
	StringCchCat( szCustomActionData, 1024, L"|" );
	StringCchCat( szCustomActionData, 1024, strGameInstanceGUID );
	MsiSetProperty( hModule, L"AddToGameExplorerUsingMSI", szCustomActionData );

	// Set the CustomActionData property for the "RollBackAddToGameExplorer" deferred custom action.
	MsiSetProperty( hModule, L"RollBackAddToGameExplorer", strGameInstanceGUID );

	// Set the CustomActionData property for the "RemoveFromGameExplorerUsingMSI" deferred custom action.
	MsiSetProperty( hModule, L"RemoveFromGameExplorerUsingMSI", strGameInstanceGUID );

#ifdef SHOW_DEBUG_MSGBOXES
	WCHAR sz[1024];
	StringCchPrintf( sz, 1024, L"szInstallDir='%s'\nszRelativePathToGDF='%s'\nstrAbsPathToGDF='%s'\nALLUSERS='%s'\nbFoundExistingGUID=%d\nstrGameInstanceGUID='%s'", 
		szInstallDir, szRelativePathToGDF, strAbsPathToGDF, szALLUSERS, bFoundExistingGUID, strGameInstanceGUID );
	MessageBox( NULL, sz, L"SetMSIGameExplorerProperties", MB_OK );	// CHB 2007.08.01 - String audit: development
#endif

	if( szRelativePathToGDF ) delete [] szRelativePathToGDF;
	if( szALLUSERS ) delete [] szALLUSERS;
	if( szInstallDir ) delete [] szInstallDir;
	if( szProductCode ) delete [] szProductCode;

	return ERROR_SUCCESS;
}


//--------------------------------------------------------------------------------------
// The CustomActionData property must be formated like so:
//      "<path to GDF binary>|<game install path>|<install scope>|<generated GUID>"
// for example:
//      "C:\MyGame\GameGDF.dll|C:\MyGame|2|{1DE6CE3D-EA69-4671-941F-26F789F39C5B}"
//--------------------------------------------------------------------------------------
UINT __stdcall AddToGameExplorerUsingMSI( MSIHANDLE hModule ) 
{
	HRESULT hr = E_FAIL;
	WCHAR* szCustomActionData = GetPropertyFromMSI(hModule, L"CustomActionData");

	if( szCustomActionData )
	{
		WCHAR szGDFBinPath[MAX_PATH] = {0};
		WCHAR szGameInstallPath[MAX_PATH] = {0};
		GAME_INSTALL_SCOPE InstallScope = GIS_ALL_USERS;
		GUID InstanceGUID;

		WCHAR* pFirstDelim = wcschr( szCustomActionData, '|' );
		WCHAR* pSecondDelim = wcschr( pFirstDelim + 1, '|' );
		WCHAR* pThirdDelim = wcschr( pSecondDelim + 1, '|' );

		if( pFirstDelim )
		{
			*pFirstDelim = 0;            
			if( pSecondDelim )
			{
				*pSecondDelim = 0;
				if( pThirdDelim )
				{
					*pThirdDelim = 0;
					ConvertStringToGUID( pThirdDelim + 1, &InstanceGUID );
				}
				InstallScope = (GAME_INSTALL_SCOPE)_wtoi( pSecondDelim + 1 );
			}
			StringCchCopy( szGameInstallPath, MAX_PATH, pFirstDelim + 1 );
		}
		StringCchCopy( szGDFBinPath, MAX_PATH, szCustomActionData );

#ifdef SHOW_DEBUG_MSGBOXES
		WCHAR sz[1024];
		WCHAR szGUID[64] = {0};
		if( pThirdDelim )
			StringCchCopy( szGUID, 64, pThirdDelim + 1 );
		StringCchPrintf( sz, 1024, L"szGDFBinPath='%s'\nszGameInstallPath='%s'\nInstallScope='%d'\nszGUID='%s'", 
			szGDFBinPath, szGameInstallPath, InstallScope, szGUID );
		MessageBox( NULL, sz, L"AddToGameExplorerUsingMSI", MB_OK );	// CHB 2007.08.01 - String audit: development
#endif

		hr = AddToGameExplorerW( szGDFBinPath, szGameInstallPath, InstallScope, &InstanceGUID );

		delete [] szCustomActionData;
	}

	return ( SUCCEEDED(hr) ) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
}


//--------------------------------------------------------------------------------------
// The CustomActionData property must be formated like so:
//      "<Game Instance GUID>"
// for example:
//      "{1DE6CE3D-EA69-4671-941F-26F789F39C5B}"
//--------------------------------------------------------------------------------------
UINT __stdcall RemoveFromGameExplorerUsingMSI( MSIHANDLE hModule ) 
{
	GUID InstanceGUID;
	HRESULT hr = E_FAIL;
	WCHAR* szCustomActionData = GetPropertyFromMSI(hModule, L"CustomActionData");

	if( szCustomActionData )
	{
		WCHAR strInstanceGUID[64] = {0};
		StringCchCopy( strInstanceGUID, 64, szCustomActionData );
		ConvertStringToGUID( strInstanceGUID, &InstanceGUID );

#ifdef SHOW_DEBUG_MSGBOXES
		WCHAR sz[1024];
		StringCchPrintf( sz, 1024, L"strInstanceGUID='%s'", strInstanceGUID );
		MessageBox( NULL, sz, L"RemoveFromGameExplorerUsingMSI", MB_OK );	// CHB 2007.08.01 - String audit: development
#endif 

		hr = RemoveFromGameExplorer( &InstanceGUID );
		delete [] szCustomActionData;
	}

	return ( SUCCEEDED(hr) ) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
}


//--------------------------------------------------------------------------------------
// Adds a game to the Game Explorer
//--------------------------------------------------------------------------------------
STDAPI AddToGameExplorerW( WCHAR* strGDFBinPath, WCHAR *strGameInstallPath, 
						  GAME_INSTALL_SCOPE InstallScope, GUID *pInstanceGUID )
{
	HRESULT hr = E_FAIL;
	bool    bCleanupCOM = false;
	BOOL    bHasAccess = FALSE;
	BSTR    bstrGDFBinPath = NULL;
	BSTR    bstrGameInstallPath = NULL;
	IGameExplorer* pFwGameExplorer = NULL;

	if( strGDFBinPath == NULL || strGameInstallPath == NULL || pInstanceGUID == NULL )
	{
		assert( false );
		return E_INVALIDARG;
	}

	bstrGDFBinPath = SysAllocString( strGDFBinPath );
	bstrGameInstallPath = SysAllocString( strGameInstallPath );
	if( bstrGDFBinPath == NULL || bstrGameInstallPath == NULL )
	{
		hr = E_OUTOFMEMORY;
		goto LCleanup;
	}

	hr = CoInitialize( 0 );
	bCleanupCOM = SUCCEEDED(hr); 

	// Create an instance of the Game Explorer Interface
	hr = CoCreateInstance( __uuidof(GameExplorer), NULL, CLSCTX_INPROC_SERVER, 
		__uuidof(IGameExplorer), (void**) &pFwGameExplorer );
	if( FAILED(hr) || pFwGameExplorer == NULL )
	{
		// On Windows XP or eariler, write registry keys to known location 
		// so that if the machine is upgraded to Windows Vista or later, these games will 
		// be automatically found.
		// 
		// Depending on GAME_INSTALL_SCOPE, write to:
		//      HKLM\Software\Microsoft\Windows\CurrentVersion\GameUX\GamesToFindOnWindowsUpgrade\{GUID}\
		// or
		//      HKCU\Software\Classes\Software\Microsoft\Windows\CurrentVersion\GameUX\GamesToFindOnWindowsUpgrade\{GUID}\
		// and write there these 2 string values: GDFBinaryPath and GameInstallPath 
		//
		HKEY hKeyGamesToFind = NULL, hKeyGame = NULL;
		LONG lResult;
		DWORD dwDisposition;
		if( InstallScope == GIS_CURRENT_USER )
			lResult = RegCreateKeyEx( HKEY_CURRENT_USER, L"Software\\Classes\\Software\\Microsoft\\Windows\\CurrentVersion\\GameUX\\GamesToFindOnWindowsUpgrade", 
			0, NULL, 0, KEY_WRITE, NULL, &hKeyGamesToFind, &dwDisposition );
		else
			lResult = RegCreateKeyEx( HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\Windows\\CurrentVersion\\GameUX\\GamesToFindOnWindowsUpgrade", 
			0, NULL, 0, KEY_WRITE, NULL, &hKeyGamesToFind, &dwDisposition );

#ifdef SHOW_DEBUG_MSGBOXES
		WCHAR sz[1024];
		StringCchPrintf( sz, 1024, L"WinXP Path: RegCreateKeyEx lResult=%d", lResult );
		MessageBox( NULL, sz, L"AddToGameExplorerW", MB_OK );	// CHB 2007.08.01 - String audit: development
#endif

		if( ERROR_SUCCESS == lResult ) 
		{
			WCHAR strGameInstanceGUID[128] = {0};
			if( *pInstanceGUID == GUID_NULL )
				GenerateGUID( pInstanceGUID );
			hr = StringFromGUID2( *pInstanceGUID, strGameInstanceGUID, 128 );

			if( SUCCEEDED(hr) )
			{
				lResult = RegCreateKeyEx( hKeyGamesToFind, strGameInstanceGUID, 0, NULL, 0, KEY_WRITE, NULL, &hKeyGame, &dwDisposition );
				if( ERROR_SUCCESS == lResult ) 
				{
					size_t nGDFBinPath = 0, nGameInstallPath = 0;
					StringCchLength( strGDFBinPath, MAX_PATH, &nGDFBinPath );
					StringCchLength( strGameInstallPath, MAX_PATH, &nGameInstallPath );
					RegSetValueEx( hKeyGame, L"GDFBinaryPath", 0, REG_SZ, (BYTE*)strGDFBinPath, (DWORD)((nGDFBinPath + 1)*sizeof(WCHAR)) );
					RegSetValueEx( hKeyGame, L"GameInstallPath", 0, REG_SZ, (BYTE*)strGameInstallPath, (DWORD)((nGameInstallPath + 1)*sizeof(WCHAR)) );
				}
				if( hKeyGame ) RegCloseKey( hKeyGame );
			}
		}
		if( hKeyGamesToFind ) RegCloseKey( hKeyGamesToFind );
	}
	else
	{
		hr = pFwGameExplorer->VerifyAccess( bstrGDFBinPath, &bHasAccess );
		if( FAILED(hr) || !bHasAccess )
			goto LCleanup;

		hr = pFwGameExplorer->AddGame( bstrGDFBinPath, bstrGameInstallPath, 
			InstallScope, pInstanceGUID );

#ifdef SHOW_DEBUG_MSGBOXES
		WCHAR sz[1024];
		WCHAR strGameInstanceGUID[128] = {0};
		StringFromGUID2( *pInstanceGUID, strGameInstanceGUID, 128 );
		StringCchPrintf( sz, 1024, L"Vista Path: szGDFBinPath='%s'\nszGameInstallPath='%s'\nInstallScope='%d'\nszGUID='%s'", 
			bstrGDFBinPath, bstrGameInstallPath, InstallScope, strGameInstanceGUID );
		MessageBox( NULL, sz, L"AddToGameExplorerW", MB_OK );	// CHB 2007.08.01 - String audit: development
#endif
	}

LCleanup:
	if( bstrGDFBinPath ) SysFreeString( bstrGDFBinPath );
	if( bstrGameInstallPath ) SysFreeString( bstrGameInstallPath );
	if( pFwGameExplorer ) pFwGameExplorer->Release();
	if( bCleanupCOM ) CoUninitialize();

	return hr;
}


//--------------------------------------------------------------------------------------
// Removes a game from the Game Explorer
//--------------------------------------------------------------------------------------
STDAPI RemoveFromGameExplorer( GUID *pInstanceGUID )
{   
	HRESULT hr = E_FAIL;
	bool    bCleanupCOM = false;
	IGameExplorer* pFwGameExplorer = NULL;

	hr = CoInitialize( 0 );
	bCleanupCOM = SUCCEEDED(hr); 

	// Create an instance of the Game Explorer Interface
	hr = CoCreateInstance( __uuidof(GameExplorer), NULL, CLSCTX_INPROC_SERVER, 
		__uuidof(IGameExplorer), (void**) &pFwGameExplorer );
	if( SUCCEEDED(hr) ) 
	{
		// Remove the game from the Game Explorer
		hr = pFwGameExplorer->RemoveGame( *pInstanceGUID );
	}
	else 
	{
		// On Windows XP remove reg keys
		if( pInstanceGUID == NULL )
			goto LCleanup;

		WCHAR strGameInstanceGUID[128] = {0};
		hr = StringFromGUID2( *pInstanceGUID, strGameInstanceGUID, 128 );
		if( FAILED(hr) )
			goto LCleanup;

		WCHAR szKeyPath[1024];
		if( SUCCEEDED( StringCchPrintf( szKeyPath, 1024, L"Software\\Classes\\Software\\Microsoft\\Windows\\CurrentVersion\\GameUX\\GamesToFindOnWindowsUpgrade\\%s", strGameInstanceGUID ) ) )
			SHDeleteKey( HKEY_CURRENT_USER, szKeyPath );

		if( SUCCEEDED( StringCchPrintf( szKeyPath, 1024, L"Software\\Microsoft\\Windows\\CurrentVersion\\GameUX\\GamesToFindOnWindowsUpgrade\\%s", strGameInstanceGUID ) ) )
			SHDeleteKey( HKEY_LOCAL_MACHINE, szKeyPath );

		hr = S_OK;
		goto LCleanup;
	}

#ifdef SHOW_DEBUG_MSGBOXES
	WCHAR sz[1024];
	WCHAR strGameInstanceGUID[128] = {0};
	StringFromGUID2( *pInstanceGUID, strGameInstanceGUID, 128 );
	StringCchPrintf( sz, 1024, L"pInstanceGUID='%s'", strGameInstanceGUID );
	MessageBox( NULL, sz, L"RemoveFromGameExplorer", MB_OK );	// CHB 2007.08.01 - String audit: development
#endif

LCleanup:
	if( pFwGameExplorer ) pFwGameExplorer->Release();
	if( bCleanupCOM ) CoUninitialize();

	return hr;
}


//--------------------------------------------------------------------------------------
// Adds application from exception list
//--------------------------------------------------------------------------------------
STDAPI AddToGameExplorerA( CHAR* strGDFBinPath, CHAR *strGameInstallPath, 
						  GAME_INSTALL_SCOPE InstallScope, GUID *pInstanceGUID )
{
	WCHAR wstrBinPath[MAX_PATH] = {0};
	WCHAR wstrInstallPath[MAX_PATH] = {0};

	MultiByteToWideChar(CP_ACP, 0, strGDFBinPath, MAX_PATH, wstrBinPath, MAX_PATH);
	MultiByteToWideChar(CP_ACP, 0, strGameInstallPath, MAX_PATH, wstrInstallPath, MAX_PATH);

	return AddToGameExplorerW( wstrBinPath, wstrInstallPath, InstallScope, pInstanceGUID );
}


//--------------------------------------------------------------------------------------
// Generates a random GUID
//--------------------------------------------------------------------------------------
STDAPI GenerateGUID( GUID *pInstanceGUID ) 
{
	return CoCreateGuid( pInstanceGUID ); 
}


//--------------------------------------------------------------------------------------
// Gets a property from MSI.  Deferred custom action can only access the property called
// "CustomActionData"
//--------------------------------------------------------------------------------------
LPWSTR GetPropertyFromMSI( MSIHANDLE hMSI, LPCWSTR szPropName )
{
	DWORD dwSize = 0, dwBufferLen = 0;
	LPWSTR szValue = NULL;

	UINT uErr = MsiGetProperty( hMSI, szPropName, L"", &dwSize );
	if( (ERROR_SUCCESS == uErr) || (ERROR_MORE_DATA == uErr) )
	{
		dwSize++; // Add NULL term
		dwBufferLen = dwSize;
		szValue = new WCHAR[ dwBufferLen ];
		if( szValue )
		{
			uErr = MsiGetProperty( hMSI, szPropName, szValue, &dwSize );
			if( (ERROR_SUCCESS != uErr) )
			{
				// Cleanup on failure
				delete[] szValue;
				szValue = NULL;
			} 
			else
			{
				// Make sure buffer is null-terminated
				szValue[ dwBufferLen - 1 ] = '\0';
			}
		} 
	}

	return szValue;
}


//-----------------------------------------------------------------------------
// Name: ConvertStringToGUID()
// Desc: Converts a string to a GUID
//-----------------------------------------------------------------------------
HRESULT ConvertStringToGUID( const WCHAR* strSrc, GUID* pGuidDest )
{
	UINT aiTmp[10];

	if( swscanf( strSrc, L"{%8X-%4X-%4X-%2X%2X-%2X%2X%2X%2X%2X%2X}",
		&pGuidDest->Data1, 
		&aiTmp[0], &aiTmp[1], 
		&aiTmp[2], &aiTmp[3],
		&aiTmp[4], &aiTmp[5],
		&aiTmp[6], &aiTmp[7],
		&aiTmp[8], &aiTmp[9] ) != 11 )
	{
		ZeroMemory( pGuidDest, sizeof(GUID) );
		return E_FAIL;
	}
	else
	{
		pGuidDest->Data2       = (USHORT) aiTmp[0];
		pGuidDest->Data3       = (USHORT) aiTmp[1];
		pGuidDest->Data4[0]    = (BYTE) aiTmp[2];
		pGuidDest->Data4[1]    = (BYTE) aiTmp[3];
		pGuidDest->Data4[2]    = (BYTE) aiTmp[4];
		pGuidDest->Data4[3]    = (BYTE) aiTmp[5];
		pGuidDest->Data4[4]    = (BYTE) aiTmp[6];
		pGuidDest->Data4[5]    = (BYTE) aiTmp[7];
		pGuidDest->Data4[6]    = (BYTE) aiTmp[8];
		pGuidDest->Data4[7]    = (BYTE) aiTmp[9];
		return S_OK;
	}
}


STDAPI RetrieveGUIDForApplicationA( CHAR* szPathToGDFdll, GUID* pGUID )
{
	WCHAR wszPathToGDFdll[MAX_PATH] = {0};
	HRESULT hr;

	MultiByteToWideChar(CP_ACP, 0, szPathToGDFdll, MAX_PATH, wszPathToGDFdll, MAX_PATH);
	hr = RetrieveGUIDForApplicationW( wszPathToGDFdll, pGUID );

	return hr;
}

//-----------------------------------------------------------------------------
STDAPI RetrieveGUIDForApplicationW( WCHAR* szPathToGDFdll, GUID* pGUID )
{
	HRESULT hr;
	IWbemLocator*  pIWbemLocator = NULL;
	IWbemServices* pIWbemServices = NULL;
	BSTR           pNamespace = NULL;
	IEnumWbemClassObject* pEnum = NULL;
	bool bFound = false;

	CoInitialize(0);

	hr = CoCreateInstance( __uuidof(WbemLocator), NULL, CLSCTX_INPROC_SERVER, 
		__uuidof(IWbemLocator), (LPVOID*) &pIWbemLocator );
	if( SUCCEEDED(hr) && pIWbemLocator )
	{
		// Using the locator, connect to WMI in the given namespace.
		pNamespace = SysAllocString( L"\\\\.\\root\\cimv2\\Applications\\Games" );

		hr = pIWbemLocator->ConnectServer( pNamespace, NULL, NULL, 0L,
			0L, NULL, NULL, &pIWbemServices );
		if( SUCCEEDED(hr) && pIWbemServices != NULL )
		{
			// Switch security level to IMPERSONATE. 
			CoSetProxyBlanket( pIWbemServices, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, 
				RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, 0 );

			BSTR bstrQueryType = SysAllocString( L"WQL" );

			// Double up the '\' marks for the WQL query
			WCHAR szDoubleSlash[2048];
			int iDest = 0, iSource = 0;
			for( ;; )
			{
				if( szPathToGDFdll[iSource] == 0 || iDest > 2000 )
					break;
				szDoubleSlash[iDest] = szPathToGDFdll[iSource];
				if( szPathToGDFdll[iSource] == L'\\' )
				{
					iDest++; szDoubleSlash[iDest] = L'\\';
				}
				iDest++;
				iSource++;
			}
			szDoubleSlash[iDest] = 0;

			WCHAR szQuery[1024];
			StringCchPrintf( szQuery, 1024, L"SELECT * FROM GAME WHERE GDFBinaryPath = \"%s\"", szDoubleSlash );
			BSTR bstrQuery = SysAllocString( szQuery );

			hr = pIWbemServices->ExecQuery( bstrQueryType, bstrQuery, 
				WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
				NULL, &pEnum );
			if( SUCCEEDED(hr) )
			{
				IWbemClassObject* pGameClass = NULL;
				DWORD uReturned = 0;
				BSTR pPropName = NULL;

				// Get the first one in the list
				hr = pEnum->Next( 5000, 1, &pGameClass, &uReturned );
				if( SUCCEEDED(hr) && uReturned != 0 && pGameClass != NULL )
				{
					VARIANT var;

					// Get the InstanceID string
					pPropName = SysAllocString( L"InstanceID" );
					hr = pGameClass->Get( pPropName, 0L, &var, NULL, NULL );
					if( SUCCEEDED(hr) && var.vt == VT_BSTR )
					{
						bFound = true;
						if( pGUID ) ConvertStringToGUID( var.bstrVal, pGUID );
					}

					if( pPropName ) SysFreeString( pPropName );
				}

				SAFE_RELEASE( pGameClass );
			}

			SAFE_RELEASE( pEnum );
		}

		if( pNamespace ) SysFreeString( pNamespace );
		SAFE_RELEASE( pIWbemServices );
	}

	SAFE_RELEASE( pIWbemLocator );

#ifdef SHOW_DEBUG_MSGBOXES
	WCHAR sz[1024];
	StringCchPrintf( sz, 1024, L"szPathToGDFdll=%s bFound=%d", szPathToGDFdll, bFound );
	MessageBox( NULL, sz, L"RetrieveGUIDForApplicationW", MB_OK );	// CHB 2007.08.01 - String audit: development
#endif

	return (bFound) ? S_OK : E_FAIL;
}


//-----------------------------------------------------------------------------
STDAPI CreateTaskA( GAME_INSTALL_SCOPE InstallScope, GUID* pGUID, BOOL bSupportTask,				
				   int nTaskID, CHAR* strTaskName, CHAR* strLaunchPath, CHAR* strCommandLineArgs ) 	    
{
	WCHAR wstrTaskName[MAX_PATH] = {0};
	WCHAR wstrLaunchPath[MAX_PATH] = {0};
	WCHAR wstrCommandLineArgs[MAX_PATH] = {0};

	MultiByteToWideChar(CP_ACP, 0, strTaskName, MAX_PATH, wstrTaskName, MAX_PATH);
	MultiByteToWideChar(CP_ACP, 0, strLaunchPath, MAX_PATH, wstrLaunchPath, MAX_PATH);
	MultiByteToWideChar(CP_ACP, 0, strCommandLineArgs, MAX_PATH, wstrCommandLineArgs, MAX_PATH);

	return CreateTaskW( InstallScope, pGUID, bSupportTask, nTaskID, 
		wstrTaskName, wstrLaunchPath, wstrCommandLineArgs );
}


//-----------------------------------------------------------------------------
STDAPI CreateTaskW( GAME_INSTALL_SCOPE InstallScope,   // Either GIS_CURRENT_USER or GIS_ALL_USERS 
				   GUID* pGUID,					   // valid GameInstance GUID that was passed to AddGame()
				   BOOL bSupportTask,				   // if TRUE, this is a support task otherwise it is a play task
				   int nTaskID,					   // ID of task
				   WCHAR* strTaskName,				   // Name of task.  Ex "Play"
				   WCHAR* strLaunchPath,			   // Path to exe.  Example: "C:\Program Files\Microsoft\MyGame.exe"
				   WCHAR* strCommandLineArgs ) 	   // Can be NULL.  Example: "-windowed"
{
	HRESULT hr;
	WCHAR strPath[512];
	WCHAR strGUID[256];
	WCHAR strCommonFolder[MAX_PATH];
	WCHAR strShortcutFilePath[512];

	// Get base path based on install scope
	if( InstallScope == GIS_CURRENT_USER )
		SHGetFolderPath( NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, strCommonFolder );
	else
		SHGetFolderPath( NULL, CSIDL_COMMON_APPDATA, NULL, SHGFP_TYPE_CURRENT, strCommonFolder );

	// Convert GUID to string
	hr = StringFromGUID2( *pGUID, strGUID, 256 );
	if( FAILED(hr) )
		return hr;

	// Create dir path for shortcut
	StringCchPrintf( strPath, 512, L"%s\\Microsoft\\Windows\\GameExplorer\\%s\\%s\\%d", 
		strCommonFolder, strGUID, (bSupportTask) ? L"SupportTasks" : L"PlayTasks", nTaskID );

	// Create the directory and all intermediate directories
	SHCreateDirectoryEx( NULL, strPath, NULL ); 

	// Create full file path to shortcut 
	StringCchPrintf( strShortcutFilePath, 512, L"%s\\%s.lnk", strPath, strTaskName );

#ifdef SHOW_DEBUG_MSGBOXES
	WCHAR sz[1024];
	StringCchPrintf( sz, 1024, L"strShortcutFilePath='%s' strTaskName='%s'", strShortcutFilePath, strTaskName );
	MessageBox( NULL, sz, L"CreateTaskW", MB_OK );	// CHB 2007.08.01 - String audit: development
#endif

	// Create shortcut
	CoInitialize( NULL );
	IShellLink* psl; 
	hr = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, 
		IID_IShellLink, (LPVOID*)&psl); 
	if (SUCCEEDED(hr)) 
	{ 
		// Setup shortcut
		psl->SetPath( strLaunchPath ); 
		if( strCommandLineArgs ) psl->SetArguments( strCommandLineArgs ); 

		// These shortcut settings aren't needed for tasks
		// if( strIconPath ) psl->SetIconLocation( strIconPath, nIcon );
		// if( wHotkey ) psl->SetHotkey( wHotkey );
		// if( nShowCmd ) psl->SetShowCmd( nShowCmd );
		// if( strDescription ) psl->SetDescription( strDescription );

		// Set working dir to path of launch exe
		WCHAR strFullPath[512];
		WCHAR* strExePart; 
		GetFullPathName( strLaunchPath, 512, strFullPath, &strExePart );
		if( strExePart ) *strExePart = 0;
		psl->SetWorkingDirectory( strFullPath );

		// Save shortcut to file
		IPersistFile* ppf; 
		hr = psl->QueryInterface(IID_IPersistFile, (LPVOID*)&ppf); 
		if (SUCCEEDED(hr)) 
		{ 
			hr = ppf->Save( strShortcutFilePath, TRUE ); 
			ppf->Release(); 
		} 
		psl->Release(); 
	} 

	CoUninitialize();

	return S_OK;
}

//-----------------------------------------------------------------------------
STDAPI RemoveTasks( GUID* pGUID ) // valid GameInstance GUID that was passed to AddGame()
{
	HRESULT hr;
	WCHAR strPath[512] = {0};
	WCHAR strGUID[256];
	WCHAR strLocalAppData[MAX_PATH];
	WCHAR strCommonAppData[MAX_PATH];

	// Get base path based on install scope
	if( FAILED( hr = SHGetFolderPath( NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, strLocalAppData ) ) )
		return hr;

	if( FAILED( hr = SHGetFolderPath( NULL, CSIDL_COMMON_APPDATA, NULL, SHGFP_TYPE_CURRENT, strCommonAppData ) ) )
		return hr;

	// Convert GUID to string
	if( FAILED( hr = StringFromGUID2( *pGUID, strGUID, 256 ) ) )
		return hr;

	if( FAILED( hr = StringCchPrintf( strPath, 512, L"%s\\Microsoft\\Windows\\GameExplorer\\%s", strLocalAppData, strGUID ) ) )
		return hr;

	SHFILEOPSTRUCT fileOp;
	ZeroMemory( &fileOp, sizeof(SHFILEOPSTRUCT) );
	fileOp.wFunc = FO_DELETE;
	fileOp.pFrom = strPath;
	fileOp.fFlags = FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;
	SHFileOperation( &fileOp );

	if( FAILED( hr = StringCchPrintf( strPath, 512, L"%s\\Microsoft\\Windows\\GameExplorer\\%s", strCommonAppData, strGUID ) ) )
		return hr;

	ZeroMemory( &fileOp, sizeof(SHFILEOPSTRUCT) );
	fileOp.wFunc = FO_DELETE;
	fileOp.pFrom = strPath;
	fileOp.fFlags = FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;
	SHFileOperation( &fileOp );

	return S_OK;
}

#endif	// end GAME_EXPLORER_ENABLED
