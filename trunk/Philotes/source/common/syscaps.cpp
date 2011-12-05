//-----------------------------------------------------------------------------
//
// (C)Copyright 2007, Flagship Studios. All rights reserved.
//
//-----------------------------------------------------------------------------


#include "syscaps.h"
#pragma warning(push)
#pragma warning(disable:6386)
#include <comdef.h>
#pragma warning(pop)

#pragma warning(push)
#pragma warning(disable:4819)
#include <Wbemidl.h>
#pragma warning(pop)

// Originally from dxC_caps.cpp - moved here to be accessible to common code.

#define SAFE_RELEASE(p) ((((p) == 0) ? 0 : ((p)->Release(), (p) = 0)), (void)0)  //{ if (p) { (p)->Release(); (p)=NULL; } }

static SYSCAPS gtGeneralHardwareCaps;

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#if !ISVERSION(SERVER_VERSION)

static void sGetWMIQueryData( DWORD * pdwValue, const WCHAR * pwszName, IWbemClassObject* pclsObj )
{
	VARIANT vtProp;
	VariantInit(&vtProp);

	// Get the value of the property specified by pwszName
	HRESULT hr = pclsObj->Get( pwszName, 0, &vtProp, 0, 0);
	ASSERTX_RETURN( SUCCEEDED(hr), "Error getting WMI property" );
	*pdwValue = vtProp.uintVal;	

	VariantClear(&vtProp);
}

static void sGetWMIQueryData( DWORDLONG* pdwValue, const WCHAR * pwszName, IWbemClassObject* pclsObj )
{
	VARIANT vtProp;
	VariantInit(&vtProp);

	// Get the value of the property specified by pwszName
	HRESULT hr = pclsObj->Get( pwszName, 0, &vtProp, 0, 0);
	ASSERTX_RETURN( SUCCEEDED(hr), "Error getting WMI property" );
	if (vtProp.vt == VT_BSTR)
		*pdwValue = _wcstoui64(vtProp.bstrVal, NULL, 10);
	else
		*pdwValue = vtProp.ullVal;	

	VariantClear(&vtProp);
}

template <typename T>
static void sEnumAndGetWMIQueryData( T* pdwValue, const WCHAR* pwszName, 
									 IEnumWbemClassObject* pEnumerator,
									 BOOL bGetMax = FALSE )
{
	IWbemClassObject *pclsObj;
	ULONG uReturn = 0;
	BOOL bQueriedData = FALSE;
	*pdwValue = 0;

	while (pEnumerator)
	{
		HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
		(void)hr;

		if (0 == uReturn)
		{
			if( !bQueriedData )
				throw( __FUNCTION__ );
			break;
		}
		
		T dwValue = 0;
		sGetWMIQueryData( &dwValue, pwszName, pclsObj );

		if ( !bGetMax || ( dwValue > *pdwValue ) )
			*pdwValue = dwValue;

		bQueriedData = TRUE;
	}
}

#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

const SYSCAPS & SysCaps_Get(void)
{
	return gtGeneralHardwareCaps;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void SysCaps_Init(void)
{
	ZeroMemory( &gtGeneralHardwareCaps, sizeof(gtGeneralHardwareCaps) );

#if !ISVERSION(SERVER_VERSION)

	HRESULT hres(0);
	IWbemLocator *pLoc = NULL;
	IWbemServices *pSvc = NULL;
	IEnumWbemClassObject* pEnumerator = NULL;

	try
	{
		////////////////////////////////////////////////////////////////
		// Step 1:
		// Initialize COM.
		//
		hres = CoInitialize( NULL );
		if ( FAILED(hres) )
		{
			throw("Failed to initialize COM library.");
		}


		////////////////////////////////////////////////////////////////
		// Step 2:
		// Set general COM security levels
		// Note: If you are using Windows 2000, you need to specify
		// the default authentication credentials for a user by using
		// a SOLE_AUTHENTICATION_LIST structure in the pAuthList 
		// parameter of CoInitializeSecurity
		//
		hres =  CoInitializeSecurity(
			NULL, 
			-1,                          // COM authentication
			NULL,                        // Authentication services
			NULL,                        // Reserved
			RPC_C_AUTHN_LEVEL_DEFAULT,   // Default authentication 
			RPC_C_IMP_LEVEL_IMPERSONATE, // Default Impersonation  
			NULL,                        // Authentication info
			EOAC_NONE,                   // Additional capabilities 
			NULL                         // Reserved
			);

	                      
		if ( hres != RPC_E_TOO_LATE && FAILED(hres) )
			throw("Failed to initialize security.");
	    

		////////////////////////////////////////////////////////////////
		// Step 3:
		// Obtain the initial locator to WMI
		//
		const CLSID CLSID_WbemLocator = __uuidof(WbemLocator);
		const IID IID_IWbemLocator = __uuidof(IWbemLocator);

		hres = CoCreateInstance(
			CLSID_WbemLocator,             
			0, 
			CLSCTX_INPROC_SERVER, 
			IID_IWbemLocator, (LPVOID*)&pLoc);
	 
		if (FAILED(hres))
			throw( "Failed to create IWbemLocator object." );


		////////////////////////////////////////////////////////////////
		// Step 4:
		// Connect to WMI through the IWbemLocator::ConnectServer method
		//

		// Connect to the root\cimv2 namespace with
		// the current user and obtain pointer pSvc
		// to make IWbemServices calls.
		hres = pLoc->ConnectServer(
			 _bstr_t(L"ROOT\\CIMV2"), // Object path of WMI namespace
			 NULL,                    // User name. NULL = current user
			 NULL,                    // User password. NULL = current
			 0,                       // Locale. NULL indicates current
			 NULL,                    // Security flags.
			 0,                       // Authority (e.g. Kerberos)
			 0,                       // Context object 
			 &pSvc                    // pointer to IWbemServices proxy
			 );

		if (FAILED(hres))
			throw( "Could not connect." );

		//cout << "Connected to ROOT\\CIMV2 WMI namespace" << endl;


		////////////////////////////////////////////////////////////////
		// Step 5: 
		// Set security levels on the proxy 
		//
		hres = CoSetProxyBlanket(
		   pSvc,                        // Indicates the proxy to set
		   RPC_C_AUTHN_WINNT,           // RPC_C_AUTHN_xxx
		   RPC_C_AUTHZ_NONE,            // RPC_C_AUTHZ_xxx
		   NULL,                        // Server principal name 
		   RPC_C_AUTHN_LEVEL_CALL,      // RPC_C_AUTHN_LEVEL_xxx 
		   RPC_C_IMP_LEVEL_IMPERSONATE, // RPC_C_IMP_LEVEL_xxx
		   NULL,                        // client identity
		   EOAC_NONE                    // proxy capabilities 
		);

		if (FAILED(hres))
			throw( "Could not set proxy blanket." );



		// Processor
		{
			hres = pSvc->ExecQuery(
				bstr_t("WQL"), 
				bstr_t("SELECT MaxClockSpeed, ExtClock FROM Win32_Processor"),
				WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, 
				NULL,
				&pEnumerator);

			if (FAILED(hres))
				throw( "Win32_Processor WMI query failed." );			

			IWbemClassObject *pclsObj;
			ULONG uReturn = 0;

			while (pEnumerator)
			{
				hres = pEnumerator->Next(WBEM_INFINITE, 1, 
					&pclsObj, &uReturn);

				if (0 == uReturn)
				{
					break;
				}

				sGetWMIQueryData( &gtGeneralHardwareCaps.dwCPUSpeedMHz,		 L"MaxClockSpeed",	pclsObj );
				sGetWMIQueryData( &gtGeneralHardwareCaps.dwSysClockSpeedMHz, L"ExtClock",		pclsObj );
			}
		}


		// DesktopMonitor
		{
			hres = pSvc->ExecQuery(
				bstr_t("WQL"), 
				bstr_t("SELECT ScreenWidth, ScreenHeight FROM Win32_DesktopMonitor"),
				WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, 
				NULL,
				&pEnumerator);

			if (FAILED(hres))
				throw( "Win32_DesktopMonitor WMI query failed." );

			IWbemClassObject *pclsObj;
			ULONG uReturn = 0;

			while (pEnumerator)
			{
				hres = pEnumerator->Next(WBEM_INFINITE, 1, 
					&pclsObj, &uReturn);

				if (0 == uReturn)
				{
					break;
				}

				sGetWMIQueryData( &gtGeneralHardwareCaps.dwNativeResolutionWidth,  L"ScreenWidth",	pclsObj );
				sGetWMIQueryData( &gtGeneralHardwareCaps.dwNativeResolutionHeight, L"ScreenHeight", pclsObj );
			}
		}



		// ComputerSystem
		{
			hres = pSvc->ExecQuery(
				bstr_t("WQL"), 
				bstr_t("SELECT TotalPhysicalMemory FROM Win32_ComputerSystem"),
				WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, 
				NULL,
				&pEnumerator);

			if (FAILED(hres))
				throw( "Win32_ComputerSystem WMI query failed." );

			sEnumAndGetWMIQueryData( &gtGeneralHardwareCaps.dwlPhysicalSystemMemoryBytes,  L"TotalPhysicalMemory",	pEnumerator );
		}



		// VideoController
		{
			hres = pSvc->ExecQuery(
				bstr_t("WQL"), 
				bstr_t("SELECT AdapterRAM FROM Win32_VideoController"),
				WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, 
				NULL,
				&pEnumerator);

			if (FAILED(hres))
				throw( "Win32_VideoController WMI query failed." );			

			sEnumAndGetWMIQueryData( &gtGeneralHardwareCaps.dwPhysicalVideoMemoryBytes,  L"AdapterRAM",	pEnumerator, TRUE );
		}
	}
	catch( const char* errorString )
	{
		REF(errorString);
		ASSERTV_MSG( "%s Error code = %x.", errorString, hres );
	}

    // Cleanup
	// ========
	SAFE_RELEASE(pSvc);
	SAFE_RELEASE(pLoc);
    SAFE_RELEASE(pEnumerator);
    //SAFE_RELEASE(pclsObj);
    //CoUninitialize();

	//trace( "WMI: physical video memory: %u\n", gtGeneralHardwareCaps.dwPhysicalVideoMemoryBytes );
#endif
}
