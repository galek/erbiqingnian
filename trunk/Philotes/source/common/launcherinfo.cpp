#include "StdAfx.h"
#include "launcherinfo.h"
#include "filepaths.h"
#include <stdio.h>

#include <d3d9.h>
#include <d3d10.h>


#define LAUNCHERINFO_INI_FILENAME	"launcher.ini"

static char * sGetLauncherINIFilePath()
{
	const OS_PATH_CHAR * pszUserData = FilePath_GetSystemPath( FILE_PATH_PUBLIC_USER_DATA );
	ASSERT_RETNULL( pszUserData );

	OS_PATH_CHAR szFile[ DEFAULT_FILE_WITH_PATH_SIZE ];
	PStrPrintf( szFile, DEFAULT_FILE_WITH_PATH_SIZE, OS_PATH_TEXT("%s%s"), pszUserData, OS_PATH_TEXT(LAUNCHERINFO_INI_FILENAME) );
	static char szLauncherIni[ DEFAULT_FILE_WITH_PATH_SIZE ];
	PStrCvt( szLauncherIni, szFile, DEFAULT_FILE_WITH_PATH_SIZE );
	return szLauncherIni;
}


LauncherInfo::LauncherInfo()
{
	Init();
}

LauncherInfo* LauncherInfo::Init()
{
	bNoSave = FALSE;	// CHB 2007.08.16 - otherwise would be uninitialized in the case of failure
	success = FALSE;
	nDX10 = 0;

	char * pszFilePath = sGetLauncherINIFilePath();
	ASSERT_RETNULL( pszFilePath );

	FILE* ini;
	fopen_s(&ini, pszFilePath, "r");
	if(!ini)
	{
		return NULL;
	}
	char info[256];
	size_t nNumRead = fread_s(info, 256, sizeof(char), 7, ini);
	if ( nNumRead == 7 )
	{
		nDX10 = info[5]-'0';
		success = TRUE;
	}

	fclose(ini);
	return this;
}

LauncherInfo::~LauncherInfo(void)
{
	if(bNoSave)
		return;

	char * pszFilePath = sGetLauncherINIFilePath();
	ASSERT_RETURN( pszFilePath );

	FILE* ini;
	fopen_s(&ini, pszFilePath, "w");
	if(ini)
	{
		fprintf_s(ini, "dx10=%d\n", nDX10);	// CHB 2007.08.16 - needs to print an integer, not string
		fclose(ini);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL LauncherGetDX10(void)
{
	LauncherInfo li;
	li.bNoSave = true;	// CHB 2007.08.16 - get should not need to save
	return li.nDX10;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL LauncherSetDX10(BOOL dx)
{
	LauncherInfo li;
	li.nDX10 = dx;
	return dx;
}

// ---------------------------------------------------------------------------
// DirectX macros

#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)      { if (p) { (p)->Release(); (p)=NULL; } }
#endif

#define FSS_ERR_NODIRECT3D9			MAKE_HRESULT( SEVERITY_ERROR, 0xf55, 0x1009 )
#define FSS_ERR_NODIRECT3D10		MAKE_HRESULT( SEVERITY_ERROR, 0xf55, 0x1010 )

// ---------------------------------------------------------------------------
// DirectX globals

HMODULE g_hModD3D9  = NULL;

typedef IDirect3D9* (WINAPI * LPDIRECT3DCREATE9) (UINT);
LPDIRECT3DCREATE9 g_DynamicDirect3DCreate9 = NULL;
LPDIRECT3D9 g_pD3D9 = NULL;

HMODULE g_hModD3D10 = NULL;
HMODULE g_hModDXGI  = NULL;
IDXGIFactory* g_pDXGIFactory = NULL;

typedef HRESULT     (WINAPI * LPD3D10CREATEDEVICE)( IDXGIAdapter*, D3D10_DRIVER_TYPE, HMODULE, UINT, UINT32, ID3D10Device** );
LPD3D10CREATEDEVICE g_DynamicD3D10CreateDevice = NULL;
typedef HRESULT     (WINAPI * LPCREATEDXGIFACTORY)(REFIID, void ** );
LPCREATEDXGIFACTORY g_DynamicCreateDXGIFactory = NULL;

// ---------------------------------------------------------------------------
// DirectX functions

static bool sEnsureD3D9APIs( void )
{
	if ( ! g_hModD3D9 )
	{
		// This may fail if Direct3D 9 isn't installed
		g_hModD3D9 = LoadLibraryW( L"d3d9.dll" );
		if ( ! g_hModD3D9 )
			return false;

		g_DynamicDirect3DCreate9 = (LPDIRECT3DCREATE9) GetProcAddress( g_hModD3D9, "Direct3DCreate9" );
		if ( ! g_DynamicDirect3DCreate9 )
			return false;
	}

	return true;
}

static bool sEnsureD3D10APIs( void )
{
	if ( ! g_hModD3D10 )
	{
		// This may fail if Direct3D 10 isn't installed
		g_hModD3D10 = LoadLibraryW( L"d3d10.dll" );
		if ( ! g_hModD3D10 )
			return false;

		g_DynamicD3D10CreateDevice = (LPD3D10CREATEDEVICE) GetProcAddress( g_hModD3D10, "D3D10CreateDevice" );
		if ( ! g_DynamicD3D10CreateDevice )
			return false;
	}

	if ( ! g_hModDXGI )
	{
		g_hModDXGI = LoadLibraryW( L"dxgi.dll" );
		if ( ! g_hModDXGI )
			return false;

		g_DynamicCreateDXGIFactory = (LPCREATEDXGIFACTORY) GetProcAddress( g_hModDXGI, "CreateDXGIFactory" );
		if ( ! g_DynamicCreateDXGIFactory )
			return false;
	}

	return true;
}

static bool sEnumerateD3D10Device()
{
	UINT nAdapter = 0;
	IDXGIAdapter * pAdapter = NULL;

	while ( g_pDXGIFactory->EnumAdapters(nAdapter, &pAdapter) != DXGI_ERROR_NOT_FOUND )
	{
		if (pAdapter)
		{
			if ( SUCCEEDED( pAdapter->CheckInterfaceSupport( __uuidof(ID3D10Device), NULL ) ) )
			{
				ID3D10Device * pDevice = NULL;
				if ( SUCCEEDED( g_DynamicD3D10CreateDevice( pAdapter, D3D10_DRIVER_TYPE_HARDWARE, NULL, D3D10_CREATE_DEVICE_SINGLETHREADED, D3D10_SDK_VERSION, &pDevice ) ) )
				{
					if ( pDevice )
					{
						// looks like we can create a DX10 device
						pDevice->Release();
						pAdapter->Release();
						return true;
					}
				}
			}

			// CHB 2007.08.20 - The adapter needs to be released each time.
			pAdapter->Release();
			pAdapter = 0;
		}

		++nAdapter;
	}

	return false;
}


HRESULT CleanupD3D()
{
	SAFE_RELEASE( g_pD3D9 );
	SAFE_RELEASE( g_pDXGIFactory );

	return S_OK;
}


HRESULT CanCreateD3D10()
{
	ASSERT_DO( NULL == g_pDXGIFactory )
	{
		SAFE_RELEASE( g_pDXGIFactory );
	}

	// This may fail if Direct3D 10 isn't installed
	if ( ! sEnsureD3D10APIs() )
		return FSS_ERR_NODIRECT3D10;

	g_DynamicCreateDXGIFactory( __uuidof( IDXGIFactory ), (LPVOID*)&g_pDXGIFactory );
	if( g_pDXGIFactory == NULL )
		return FSS_ERR_NODIRECT3D10;

	if ( ! sEnumerateD3D10Device() )
		return FSS_ERR_NODIRECT3D10;

	CleanupD3D();

	return S_OK;
}


//--------------------------------------------------------------------------------------
// Creates a Direct3D object if one has not already been created  
//--------------------------------------------------------------------------------------
HRESULT CanCreateD3D9()
{
	ASSERT_DO( NULL == g_pD3D9 )
	{
		SAFE_RELEASE(g_pD3D9);
	}

	// This may fail if Direct3D 9 isn't installed
	// This may also fail if the Direct3D headers are somehow out of sync with the installed Direct3D DLLs
	if ( ! sEnsureD3D9APIs() )
		return FSS_ERR_NODIRECT3D9;

	g_pD3D9 = g_DynamicDirect3DCreate9( D3D_SDK_VERSION );
	if( g_pD3D9 == NULL )
	{
		// If still NULL, then D3D9 is not availible
		return FSS_ERR_NODIRECT3D9;
	}

	CleanupD3D();

	return S_OK;
}


BOOL SupportsDX9()
{
	HRESULT hr = CanCreateD3D9();

	return !( FAILED(hr) );
}


BOOL SupportsDX10()
{
#if !ISVERSION(SERVER_VERSION)
	if ( ! WindowsIsVistaOrLater() )
		return FALSE;

	HRESULT hr = CanCreateD3D10();

	return !( FAILED(hr) );
#else
	return FALSE;
#endif
}
