//----------------------------------------------------------------------------
// dx9_caps.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"

#include "dxC_texture.h"
#include "e_texture_priv.h"
#include "dxC_model.h"
#include "dxC_state.h"
#include "dx9_settings.h"
#include "dx9_device.h"
#include "e_main.h"
#include "e_budget.h"
#include "dxC_pixo.h"

#include "filepaths.h"
#include "performance.h"

#include "dxC_caps.h"
#include "syscaps.h"		// CHB 2007.01.10

// should be moved to the appropriate projects
# pragma comment(lib, "wbemuuid.lib")


using namespace FSSE;

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define SUPPORTS( caps, opt )			( 0 != ( caps & opt ) )


//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------

const char gszVSVersions[ NUM_VERTEX_SHADER_VERSIONS ][ DEFAULT_INDEX_SIZE ] =
{
	"VS None",
	"VS 1.1",
	"VS 2.0",
	"VS 2.a",
	"VS 3.0",
	"VS 4.0",
};

const char gszPSVersions[ NUM_PIXEL_SHADER_VERSIONS ][ DEFAULT_INDEX_SIZE ] =
{
	"PS None",
	"PS 1.1",
	"PS 1.2",
	"PS 1.3",
	"PS 1.4",
	"PS 2.0",
	"PS 2.a",
	"PS 2.b",
	"PS 3.0",
	"PS 4.0",
};

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

extern PlatformCap gtPlatformCaps[ NUM_PLATFORM_CAPS ];


//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

// ENGINE CAPS
extern DWORD gdwPlatformCapValues[ NUM_PLATFORM_CAPS ];

//----------------------------------------------------------------------------
// IMPLEMENTATIONS
//----------------------------------------------------------------------------

#if !ISVERSION(SERVER_VERSION)
#include "e_optionstate.h"	// CHB 2007.03.02
#endif
static
bool sIsWindowed(void)
{
#if !ISVERSION(SERVER_VERSION)
	return e_OptionState_GetActive()->get_bWindowed();
#else
	return true;
#endif
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

unsigned int e_GetMaxVSConstants()
{
	return (unsigned int)dx9_CapGetValue( DX9CAP_MAX_VS_CONSTANTS );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//  We get caps in one place now e_GatherPlatformCaps
//void dx9_DetectGeneralD3DCapabilities( DWORD dwAdapter, D3DC_DRIVER_TYPE tType )
//{
//	ASSERT_RETURN( dx9_GetD3D() );
//	HRVERIFY( dx9_GetD3D()->GetDeviceCaps( dwAdapter, tType, &gtD3DDeviceCaps ) );
//}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

// We get caps in one place now e_GatherPlatformCaps
//void dx9_DetectD3DDeviceCapabilities()
//{
//	ASSERT_RETURN( dx9_GetDevice() );
//	HRVERIFY( dx9_GetDevice()->GetDeviceCaps( &gtD3DDeviceCaps ) );
//
//	// CHB 2006.06.13 - Used for testing.
//#if 0
//	gtD3DDeviceCaps.PixelShaderVersion = D3DPS_VERSION(1, 1);
//	gtD3DDeviceCaps.VertexShaderVersion = D3DVS_VERSION(1, 1);
//#endif
//}



//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------


// if all of the formats pass the check, returns TRUE, and sets *ppBest to the FIRST one that matches
static DWORD sCheckFormats( DXC_FORMAT * ptFormats, int nFormats, DWORD dwUsage, D3DRESOURCETYPE tRType, BOOL bMustSupportAll, DXC_FORMAT ** ppBest, const char * szErrStr = NULL )
{
	ASSERT_RETFALSE( dx9_GetD3D() );

	DWORD dwAdapter = dxC_GetAdapter();
	D3DC_DRIVER_TYPE tDeviceType = dx9_GetDeviceType();
	DXC_FORMAT BackBufferFormat = dxC_GetBackbufferAdapterFormat();

	PRESULT pr;
	int nSupported = 0;
	for ( int i = 0; i < nFormats; i++ )
	{
		V_SAVE( pr, dx9_GetD3D()->CheckDeviceFormat( dwAdapter, tDeviceType, BackBufferFormat, dwUsage, tRType, ptFormats[ i ] ) );
		if ( FAILED( pr ) )
		{
			if ( szErrStr )
			{
				char szFormat[32];
				dxC_TextureFormatGetDisplayStr( ptFormats[i], szFormat, 32 );
				trace( "###		DX9 Caps Info:		FORMAT [ %20s ]: %s [ %s ]\n", szFormat, szErrStr, e_GetResultString(pr) );
			}
		} else
		{
			if ( nSupported == 0 && ppBest )
				*ppBest = &ptFormats[i];
			nSupported++;
		}
	}

	if ( bMustSupportAll )
		return ( nSupported == nFormats);

	return ( nSupported != 0 );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static DWORD sGetMaxZBufferBitdepth()
{
	return 0;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static DWORD sGetHardwareShadowsSupported()
{
	ASSERT_RETFALSE( dx9_GetD3D() );

	DWORD dwVendorID = dx9_CapGetValue( DX9CAP_GPU_VENDOR_ID );
	if ( dwVendorID != DT_NVIDIA_VENDOR_ID )
	{
		return FALSE;
	}

	D3DC_DRIVER_TYPE dwDeviceType = dx9_GetDeviceType();

	// for now, we only support HAL (go, HAL!) ... or REF in some cases
	ASSERT( dwDeviceType == D3DDEVTYPE_HAL || dwDeviceType == D3DDEVTYPE_REF );

	DXC_FORMAT BackBufferFormat = dxC_GetBackbufferAdapterFormat();

	// check that the actual format can be created
	V_DO_FAILED( dx9_GetD3D()->CheckDeviceFormat(
		dxC_GetAdapter(),
		dwDeviceType,
		BackBufferFormat,
		D3DUSAGE_DEPTHSTENCIL,
		D3DRTYPE_TEXTURE,
		D3DFMT_D24X8 ) )
	{
		V_DO_FAILED( dx9_GetD3D()->CheckDeviceFormat(
			dxC_GetAdapter(),
			dwDeviceType,
			BackBufferFormat,
			D3DUSAGE_DEPTHSTENCIL,
			D3DRTYPE_TEXTURE,
			D3DFMT_D24S8 ) )
		{
			V_DO_FAILED( dx9_GetD3D()->CheckDeviceFormat(
				dxC_GetAdapter(),
				dwDeviceType,
				BackBufferFormat,
				D3DUSAGE_DEPTHSTENCIL,
				D3DRTYPE_TEXTURE,
				D3DFMT_D16 ) )
			{
				return FALSE;
			}
		}
	}

	return TRUE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static DWORD sGetNullRenderTargetSupported()
{
	ASSERT_RETFALSE( dx9_GetD3D() );

	//DWORD dwVendorID = dx9_CapGetValue( DX9CAP_GPU_VENDOR_ID );
	//if ( dwVendorID != DT_NVIDIA_VENDOR_ID )
	//{
	//	// not entirely accurate... should detect >= GeForce3
	//	return FALSE;
	//}

	if ( !sGetHardwareShadowsSupported() )
		return FALSE;

	D3DC_DRIVER_TYPE dwDeviceType = dx9_GetDeviceType();

	// for now, we only support HAL (go, HAL!) ... or REF in some cases
	ASSERT( dwDeviceType == D3DDEVTYPE_HAL || dwDeviceType == D3DDEVTYPE_REF );

	DXC_FORMAT BackBufferFormat = dxC_GetBackbufferAdapterFormat();

	// check that the actual format can be created
	V_DO_FAILED( dx9_GetD3D()->CheckDeviceFormat(
		dxC_GetAdapter(),
		dwDeviceType,
		BackBufferFormat,
		D3DUSAGE_RENDERTARGET,
		D3DRTYPE_SURFACE,
		(D3DFORMAT)MAKEFOURCC('N','U','L','L') ) )
	{
		return FALSE;
	}

	return TRUE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static DWORD sSupportsAnisotropicFiltering( D3DCAPS9 & tDeviceCaps )
{
	// required anisotropic filtering support
	DXC_FORMAT tFormats[] = 
	{
		D3DFMT_A8R8G8B8,
		D3DFMT_X8R8G8B8,
		D3DFMT_DXT1,
		D3DFMT_DXT2,
		D3DFMT_DXT3,
		D3DFMT_DXT4,
		D3DFMT_DXT5,
	};
	int nFormats = arrsize(tFormats);

	if ( ! sCheckFormats( tFormats, nFormats, D3DUSAGE_QUERY_FILTER, D3DRTYPE_TEXTURE, TRUE, NULL, "Minimum texture anisotropic filtering not supported" ) )
		return 0;

	if ( SUPPORTS( tDeviceCaps.TextureFilterCaps, D3DPTFILTERCAPS_MINFANISOTROPIC ) )
		return tDeviceCaps.MaxAnisotropy;

	return 0;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static DWORD sSupportsTrilinearFiltering( D3DCAPS9 & tDeviceCaps )
{
	// required trilinear filtering support
	DXC_FORMAT tFormats[] = 
	{
		D3DFMT_A8R8G8B8,
		D3DFMT_X8R8G8B8,
		D3DFMT_DXT1,
		D3DFMT_DXT2,
		D3DFMT_DXT3,
		D3DFMT_DXT4,
		D3DFMT_DXT5,
	};
	int nFormats = arrsize(tFormats);

	if ( ! sCheckFormats( tFormats, nFormats, D3DUSAGE_QUERY_FILTER, D3DRTYPE_TEXTURE, TRUE, NULL, "Minimum texture trilinear filtering not supported" ) )
		return FALSE;

	if ( SUPPORTS( tDeviceCaps.TextureFilterCaps, D3DPTFILTERCAPS_MIPFLINEAR ) )
		return TRUE;

	return FALSE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static DWORD sMeetsTextureFormatSRGBSupport()
{
	// required sRGBRead formats for hardware-assisted HDR lighting and rendering
	DXC_FORMAT tFormats[] = 
	{
		D3DFMT_A8R8G8B8,
		D3DFMT_X8R8G8B8,
		D3DFMT_DXT1,
		D3DFMT_DXT2,
		D3DFMT_DXT3,
		D3DFMT_DXT4,
		D3DFMT_DXT5,
	};
	int nFormats = arrsize(tFormats);

	return sCheckFormats( tFormats, nFormats, D3DUSAGE_QUERY_SRGBREAD, D3DRTYPE_TEXTURE, TRUE, NULL, "sRGB read not supported" );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static DWORD sCanOutputLinearToSRGB()
{
	// required sRGBWrite formats for hardware-assisted HDR lighting
	DXC_FORMAT tFormats[] = 
	{
		D3DFMT_A8R8G8B8,
		D3DFMT_X8R8G8B8,
	};
	int nFormats = arrsize(tFormats);

	return sCheckFormats( tFormats, nFormats, D3DUSAGE_QUERY_SRGBWRITE, D3DRTYPE_TEXTURE, TRUE, NULL, "sRGB write not supported" );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static DWORD sSupportsRenderTargetFormat( DXC_FORMAT * ptFormats, int nFormats, DWORD dwUsage = 0 )
{
	BOOL bResult = sCheckFormats( ptFormats, nFormats, dwUsage, D3DRTYPE_TEXTURE, FALSE, NULL, "Basic texturing not supported" );
	if ( bResult )
	{
		// only if we support basic texturing come in here
		// prefer filtering first
		DXC_FORMAT * pBest;
		bResult = sCheckFormats( ptFormats, nFormats, dwUsage | D3DUSAGE_RENDERTARGET | D3DUSAGE_QUERY_FILTER, D3DRTYPE_TEXTURE, FALSE, &pBest, "Render target with filtering not supported" );
		if ( ! bResult )
			bResult = sCheckFormats( ptFormats, nFormats, dwUsage | D3DUSAGE_RENDERTARGET, D3DRTYPE_TEXTURE, FALSE, &pBest, "Render target not supported" );

		return bResult ? (DWORD)*pBest : FALSE;
	}
	return FALSE;
}

//-------------------------------------------------------------------------------------------------

static DWORD sSupportsCoverageSampling()
{
	if ( dx9_CapGetValue( DX9CAP_GPU_VENDOR_ID ) != DT_NVIDIA_VENDOR_ID )
		return 0;

	bool bWindowed = sIsWindowed();
	D3DC_DRIVER_TYPE dwDeviceType = dx9_GetDeviceType();
	DXC_FORMAT BackBufferFormat = dxC_GetBackbufferAdapterFormat();

	struct
	{
		D3DMULTISAMPLE_TYPE tType;
		DWORD dwQuality;
	} Tests[] = 
	{
		{ D3DMULTISAMPLE_4_SAMPLES, 2 },		// 8x
		{ D3DMULTISAMPLE_8_SAMPLES, 0 },		// 8xQ
		{ D3DMULTISAMPLE_4_SAMPLES, 4 },		// 16x
		{ D3DMULTISAMPLE_8_SAMPLES, 2 },		// 16xQ
	};
	const int nTests = arrsize(Tests);

	for ( int i = 0; i < nTests; ++i )
	{
		DWORD dwQuality = 0;
		V_DO_FAILED( dx9_GetD3D()->CheckDeviceMultiSampleType( dxC_GetAdapter(), dx9_GetDeviceType(), dxC_GetBackbufferAdapterFormat(), bWindowed, Tests[i].tType, &dwQuality ) )
			return 0;
		if ( dwQuality <= Tests[i].dwQuality )
			return 0;
	}

	// all tests passed
	return 1;
}

//-------------------------------------------------------------------------------------------------

static DWORD sSupportsFloatHDR()
{
	// iterate floating point HDR formats

	DXC_FORMAT tFormats[] = 
	{
		D3DFMT_A16B16G16R16F,
		D3DFMT_A32B32G32R32F,
	};
	int nFormats = arrsize(tFormats);

	DWORD dwUsage = 0;
	// check if the following mandatory features are supported for this format
	//dwUsage |= D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING;
	//dwUsage |= D3DUSAGE_QUERY_FILTER;
	//dwUsage |= D3DUSAGE_QUERY_WRAPANDMIP;

	return sSupportsRenderTargetFormat( tFormats, nFormats, dwUsage );
}

//-------------------------------------------------------------------------------------------------

static DWORD sSupportsIntegerHDR()
{
	// iterate non-8-bit integer HDR formats

	DXC_FORMAT tFormats[] = 
	{
		D3DFMT_A2R10G10B10,
		D3DFMT_A2B10G10R10,
		D3DFMT_A16B16G16R16,
	};
	int nFormats = arrsize(tFormats);

	DWORD dwUsage = 0;
	// check if the following mandatory features are supported for this format
	//dwUsage |= D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING;
	//dwUsage |= D3DUSAGE_QUERY_FILTER;
	//dwUsage |= D3DUSAGE_QUERY_WRAPANDMIP;

	return sSupportsRenderTargetFormat( tFormats, nFormats, dwUsage );
}

//-------------------------------------------------------------------------------------------------

static DWORD sSmallestColorTargetFormat()
{
	// iterate floating point HDR formats

	DXC_FORMAT tFormats[] = 
	{
		D3DFMT_R3G3B2,
		D3DFMT_X1R5G5B5,
		D3DFMT_R5G6B5,
		D3DFMT_A1R5G5B5,
		D3DFMT_X4R4G4B4,
		D3DFMT_A4R4G4B4,
	};
	int nFormats = arrsize(tFormats);

	DWORD dwUsage = 0;
	// check if the following mandatory features are supported for this format
	//dwUsage |= D3DUSAGE_QUERY_POSTPIXELSHADER_BLENDING;
	//dwUsage |= D3DUSAGE_QUERY_FILTER;
	//dwUsage |= D3DUSAGE_QUERY_WRAPANDMIP;

	return sSupportsRenderTargetFormat( tFormats, nFormats, dwUsage );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static DWORD sMeetsMinimumTextureFormats()
{
	// check for all used/required texture formats

	// this could be multiple functions, tiered levels of texture support (like HDR textures, etc.)


	// required basic texture formats
	DXC_FORMAT tFormats[] = 
	{
		D3DFMT_A8R8G8B8,
		D3DFMT_X8R8G8B8,
		D3DFMT_A1R5G5B5,
		D3DFMT_X1R5G5B5,
		D3DFMT_R5G6B5,
		D3DFMT_A4R4G4B4,
		D3DFMT_X4R4G4B4,
		D3DFMT_A8,
		D3DFMT_L8,
		D3DFMT_DXT1,
		D3DFMT_DXT2,
		D3DFMT_DXT3,
		D3DFMT_DXT4,
		D3DFMT_DXT5,
	};
	int nFormats = arrsize(tFormats);

	return sCheckFormats( tFormats, nFormats, 0, D3DRTYPE_TEXTURE, TRUE, NULL, "Basic texturing not supported" );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static DWORD sMeetsMinimumRenderTargetFormats()
{
	// check for all used/required render target formats

	// this could be multiple functions, tiered levels of rt support (like HDR, etc.)

	// must have all basic texture formats
	DXC_FORMAT tAllFormats[] = 
	{
		D3DFMT_A8R8G8B8,
		D3DFMT_X8R8G8B8,
	};
	int nAllFormats = arrsize(tAllFormats);

	// must have at least one of basic texture formats
	DXC_FORMAT tOneFormats[] = 
	{
		D3DFMT_X1R5G5B5,
		D3DFMT_R5G6B5,
	};
	int nOneFormats = arrsize(tOneFormats);

	DXC_FORMAT * pBest;
	BOOL bResult = sCheckFormats( tAllFormats, nAllFormats, D3DUSAGE_RENDERTARGET, D3DRTYPE_TEXTURE, TRUE, &pBest, "Render target not supported" );
	if ( !bResult )
		return FALSE;
	bResult = sCheckFormats( tAllFormats, nAllFormats, D3DUSAGE_RENDERTARGET, D3DRTYPE_TEXTURE, FALSE, NULL, "Render target not supported" );
	return bResult ? (DWORD)*pBest : FALSE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static DWORD sMeetsMinimumDepthFormats()
{
	// check for all used/required depth target formats

	// required basic texture formats
	DXC_FORMAT tFormats[] = 
	{
		D3DFMT_D24S8,
		D3DFMT_D16,
	};
	int nFormats = arrsize(tFormats);

	DXC_FORMAT * pBest;
	BOOL bResult = sCheckFormats( tFormats, nFormats, D3DUSAGE_DEPTHSTENCIL, D3DRTYPE_TEXTURE, FALSE, &pBest, "Depth target not supported" );
	return bResult ? (DWORD)*pBest : FALSE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static BOOL sGetHardwareAccelerated()
{
	bool bWindowed = sIsWindowed();

	int nSelDevType = -1;
	int nSelFmt = -1;

	struct _FMT_PAIR
	{
		DXC_FORMAT tAdapterFmt;
		DXC_FORMAT tBBFmt;
	} tFmts[] = 
	{
		//{ D3DFMT_A8R8G8B8, D3DFMT_A8R8G8B8 },
		//{ D3DFMT_A8R8G8B8, D3DFMT_A8R8G8B8 },
		{ D3DFMT_X8R8G8B8, D3DFMT_A8R8G8B8 },
		//{ D3DFMT_A1R5G5B5, D3DFMT_A1R5G5B5 },
		//{ D3DFMT_A1R5G5B5, D3DFMT_A1R5G5B5 },
		{ D3DFMT_X1R5G5B5, D3DFMT_A1R5G5B5 },
		{ D3DFMT_R5G6B5,   D3DFMT_R5G6B5 },
	};
	int nFmts = arrsize(tFmts);

	D3DDEVTYPE tNormalDevTypes[] = 
	{
		D3DDEVTYPE_HAL,
		D3DDEVTYPE_REF,
	};
	int nNormalDevTypes = arrsize(tNormalDevTypes);
	D3DDEVTYPE tFillpakDevTypes[] = 
	{
		D3DDEVTYPE_NULLREF
	};
	int nFillpakDevTypes = arrsize(tFillpakDevTypes);


	D3DDEVTYPE * tDevTypes = tNormalDevTypes;
	int nDevTypes = nNormalDevTypes;
	if ( e_IsNoRender() )
	{
		tDevTypes = tFillpakDevTypes;
		nDevTypes = nFillpakDevTypes;		// only allow/look for NULLREF in fillpak mode
		bWindowed = TRUE;
	}

	for ( int t = 0; t < nDevTypes; ++t )
	{
		for ( int i = 0; i < nFmts; ++i )
		{
			V_DO_SUCCEEDED( dx9_GetD3D()->CheckDeviceType( dxC_GetAdapter(), tDevTypes[t], tFmts[i].tAdapterFmt, tFmts[i].tBBFmt, bWindowed ) )
			{
				nSelDevType = t;
				nSelFmt = i;
				goto report;
			}
		}
	}

	if ( e_IsNoRender() )
	{
		// Don't let this blow up...
		nSelDevType = 0;
		nSelFmt = 0;
	}
	else
	{
		// Disabled in Tugboat because Tugboat has Pixomatic as a fallback
		if ( AppIsHellgate() )
		{
			ASSERTV_MSG( "Failed to find acceptable hardware or software device backbuffer format for adapter %d!", dxC_GetAdapter() );
		}
		return FALSE;
	}

report:
	ASSERT_RETVAL( IS_VALID_INDEX( nSelDevType, nDevTypes ),	0 );
	ASSERT_RETVAL( IS_VALID_INDEX( nSelFmt, nFmts ),			0 );

	BOOL bHAL = FALSE;
	if ( tDevTypes[nSelDevType] == D3DDEVTYPE_HAL )
	{
		bHAL = TRUE;
	}
#if ISVERSION(DEVELOPMENT)
	//if ( e_IsNoRender() )
	{
		char szAdapterFmt[32];
		char szBBFmt[32];
		dxC_TextureFormatGetDisplayStr( tFmts[nSelFmt].tAdapterFmt, szAdapterFmt, 32 );
		dxC_TextureFormatGetDisplayStr( tFmts[nSelFmt].tBBFmt,		szBBFmt,	  32 );
		LogMessage( LOG_FILE, "DX sGetHardwareAccelerated: Adapter Fmt: %s, Backbuffer Fmt: %s, %s device", szAdapterFmt, szBBFmt, tDevTypes[nSelDevType] == D3DDEVTYPE_HAL ? "Hardware" : "Software" );
	}
#endif
	return bHAL;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static DWORD sGetGPUVendorID()
{
	//ASSERT_RETZERO( dx9_GetDevice() );
	// CHB 2006.06.13 - Removed this as it is redundant:
	// dx9_DetectD3DDeviceCapabilities() already called earlier.
	// Come to think of it, what was the point, since only gtAdapterID is used?
//	HRVERIFY( dx9_GetDevice()->GetDeviceCaps( &gtD3DDeviceCaps ) );
	extern VideoDeviceInfo gtAdapterID;
	return gtAdapterID.VendorID;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static DWORD sGetGPUMultipleCardsMode()
{
	return (DWORD)FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static DWORD sGetPixelShaderVersion()
{
	DWORD dwVersion = dx9_CapGetValue( DX9CAP_MAX_PS_VERSION );
	// detect max shader version
	if ( dwVersion == D3DPS_VERSION(3, 0) )
	{
		LogMessage( PERF_LOG, "Detected shader level 3.0" );
	} else if ( dwVersion >= D3DPS_VERSION(2, 0) )
	{
		// The NVIDIA Geforce FX 5x00 series is better off with mostly 1.x with a touch of 2.0.
		if ( dx9_CapGetValue( DX9CAP_GPU_VENDOR_ID ) == DT_NVIDIA_VENDOR_ID )
			LogMessage( PERF_LOG, "Detected shader level 2.0 low" );
		else
			LogMessage( PERF_LOG, "Detected shader level 2.0 high" );
	} else if ( dwVersion >= D3DPS_VERSION(1, 1) )
	{
		LogMessage( PERF_LOG, "Detected shader level 1.1" );
	} else
	{
		// right now, error out
		ASSERT_MSG( "Insufficient card capabilities to continue. Requires at least shader version 1.1.");
		PostQuitMessage(0);
	}
	return dwVersion;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

//static void sEffectDetectMaxShaderLevels()
//{
//	const char szValidateEffectName[ DEFAULT_FILE_WITH_PATH_SIZE ] = "Validate.fxo";
//	ZeroMemory( gbVertexShaderVersions, sizeof(gbVertexShaderVersions) );
//	ZeroMemory( gbPixelShaderVersions,  sizeof(gbPixelShaderVersions) );
//
//	TIMER_START( "Find valid shader versions" )
//
//	dx9_RestoreD3DStates();
//
//	// set a REALLY simple FVF
//	HRVERIFY( dx9_SetFVF( VERTEX_ONLY_FORMAT ) );
//	// set a simple texture
//	int nTexture = e_GetUtilityTexture( TEXTURE_RGBA_FF );
//	D3D_TEXTURE * pTexture = dxC_TextureGet( nTexture );
//	HRVERIFY( dxC_SetTexture( 0, pTexture ? pTexture->pD3DTexture : NULL ) );
//
//	SPD3DCBLOB pErrorBuffer;
//	SPD3DCEFFECT pD3DEffect;
//	HRESULT hResult;
//
//	char pszOriginalFileNameWithPath [ DEFAULT_FILE_WITH_PATH_SIZE ];
//	char szPath[ DEFAULT_FILE_WITH_PATH_SIZE ];
//	PStrCopy( pszOriginalFileNameWithPath, FILE_PATH_EFFECT, DEFAULT_FILE_WITH_PATH_SIZE );
//	PStrForceBackslash( pszOriginalFileNameWithPath, DEFAULT_FILE_WITH_PATH_SIZE );
//	PStrPrintf( szPath, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s", pszOriginalFileNameWithPath, ENGINE_TARGET_NAME );
//	PStrForceBackslash( szPath, DEFAULT_FILE_WITH_PATH_SIZE );
//	PStrPrintf( pszOriginalFileNameWithPath, DEFAULT_FILE_WITH_PATH_SIZE, "%s%s", szPath, szValidateEffectName );
//	TCHAR szFullFilename[DEFAULT_FILE_WITH_PATH_SIZE];
//	FileGetFullFileName(szFullFilename, pszOriginalFileNameWithPath, DEFAULT_FILE_WITH_PATH_SIZE);
//
//	hResult = D3DXCreateEffectFromFile(dx9_GetDevice(),
//		szFullFilename,
//		NULL, // CONST D3DXMACRO* pDefines,   
//		NULL, // LPD3DXINCLUDE pInclude,
//		D3DXFX_NOT_CLONEABLE,
//		NULL,
//		&pD3DEffect,
//		&pErrorBuffer);
//
//	// make sure that it worked
//	if ( FAILED( hResult ) )
//	{
//		ErrorDialog( "Failed to load validation effect!\n\n%s", szValidateEffectName );
//		return;
//	}
//	HRVERIFY( hResult );
//	ASSERT ( pD3DEffect );
//
//
//	D3DXHANDLE hTechnique = NULL;
//	D3DC_EFFECT_DESC tEffectDesc;
//	D3D_EFFECT tEffect;
//	tEffect.pD3DEffect = pD3DEffect;		// to pass to various functions expecting it
//	HRVERIFY( pD3DEffect->GetDesc( &tEffectDesc ) );
//
//	for ( int nTechnique = 0; nTechnique < (int)tEffectDesc.Techniques; nTechnique++ )
//	{
//		hTechnique = pD3DEffect->GetTechnique( nTechnique );
//		ASSERT_CONTINUE( hTechnique );
//
//		D3DC_TECHNIQUE_DESC tTechDesc;
//		HRVERIFY( pD3DEffect->GetTechniqueDesc( hTechnique, &tTechDesc ) );
//		D3DXPASS_DESC tPassDesc;
//		for ( UINT i = 0; i < tTechDesc.Passes; i++ )
//		{
//			HRVERIFY( pD3DEffect->GetPassDesc( pD3DEffect->GetPass( hTechnique, i ), &tPassDesc ) );
//		}
//
//		int nVSVersion = dx9_EffectGetAnnotation ( &tEffect, hTechnique, "VSVersion" );
//		ASSERTX_CONTINUE( nVSVersion >= 0 && nVSVersion < NUM_VERTEX_SHADER_VERSIONS, "Out-of-range vertex shader version found in validation effect technique!" );
//		BOOL & bVSVersion = gbVertexShaderVersions[ nVSVersion ];
//
//		int nPSVersion = dx9_EffectGetAnnotation ( &tEffect, hTechnique, "PSVersion" );
//		ASSERTX_CONTINUE( nPSVersion >= 0 && nPSVersion < NUM_PIXEL_SHADER_VERSIONS, "Out-of-range pixel shader version found in validation effect technique!" );
//		BOOL & bPSVersion = gbPixelShaderVersions[ nPSVersion ];
//
//		trace( "### Validating %s -- %4d : %7s %7s ... ",
//			szValidateEffectName,
//			nTechnique,
//			gszVSVersions[ nVSVersion ],
//			gszPSVersions[ nPSVersion ] );
//
//		HRESULT hr;
//		hr = pD3DEffect->ValidateTechnique( hTechnique );
//		if ( SUCCEEDED( hr ) )
//		{
//			bVSVersion = TRUE;
//			bPSVersion = TRUE;
//			trace( "succeeded\n" );
//		} else
//		{
//			trace( "failed\n" );
//		}
//	}
//
//
//	UINT nMs = (UINT)TIMER_END();
//	trace( "### %5d ms in Effect Validate\n", nMs );
//}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

VERTEX_SHADER_VERSION dxC_CapsGetMaxVertexShaderVersion()
{
	if ( e_IsNoRender() )
	{
		return VS_3_0;
	}
	if ( dxC_ShouldUsePixomatic() )
	{
		return VS_NONE;
	}


#if ISVERSION(DEVELOPMENT)
	extern VERTEX_SHADER_VERSION geDebugMaxVS;
	if ( geDebugMaxVS != VS_INVALID )
		return geDebugMaxVS;
#endif

	DWORD dwVSMax = dx9_CapGetValue( DX9CAP_MAX_VS_VERSION );

	switch( dwVSMax )
	{
	case D3DVS_VERSION(3,0):
		return VS_3_0;
	case D3DVS_VERSION(2,0):
		return VS_2_a;
	case D3DVS_VERSION(1,1):
		return VS_1_1;
	default:
		ASSERTV_MSG( "Unknown vertex shader version cap!\n\n0x%08x", dwVSMax );
	}

	return VS_NONE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PIXEL_SHADER_VERSION dxC_CapsGetMaxPixelShaderVersion()
{
	if ( e_IsNoRender() )
	{
		return PS_3_0;
	}
	if ( dxC_ShouldUsePixomatic() )
	{
		return PS_NONE;
	}


#if ISVERSION(DEVELOPMENT)
	extern PIXEL_SHADER_VERSION geDebugMaxPS;
	if ( geDebugMaxPS != PS_INVALID )
		return geDebugMaxPS;
#endif

	DWORD dwPSMax = dx9_CapGetValue( DX9CAP_MAX_PS_VERSION );

	switch( dwPSMax )
	{
	case D3DPS_VERSION(3,0):
		return PS_3_0;
	case D3DPS_VERSION(2,0):
		return PS_2_a;
	case D3DPS_VERSION(1,4):
		return PS_1_4;
	case D3DPS_VERSION(1,3):
		return PS_1_3;
	case D3DPS_VERSION(1,2):
		return PS_1_2;
	case D3DPS_VERSION(1,1):
		return PS_1_1;
	default:
		ASSERTV_MSG( "Unknown pixel shader version cap!\n\n0x%08x", dwPSMax );
	}

	return PS_NONE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

// The caps never report anything other than 2.0.
// So augment the caps with a call to D3DXGetPixelShaderProfile()
// so we can distinguish 2.a and 2.b.
DWORD dx9_CapsGetAugmentedMaxPixelShaderVersion(void)
{
	DWORD dwPSMax = dx9_CapGetValue( DX9CAP_MAX_PS_VERSION );

	if (D3DSHADER_VERSION_MAJOR(dwPSMax) == 2)
	{
		// Only PS20 cards supporting a higher instruction count are considered 2.x
		if ( dx9_CapGetValue( DX9CAP_MAX_PS20_INSTRUCTIONS ) >= 128 )
		{
			// Need to go through caps because a device may not be available.
			DWORD nProf = dx9_CapGetValue( DX9CAP_PIXEL_SHADER_PROFILE );
			switch ((nProf >> 24) & 0xff)
			{
				// ps_2_a, ps_2_b, and ps_2_x are considered '2.1'
			case 'a':
			case 'b':
			case 'x':
				dwPSMax = D3DPS_VERSION(2, 1);
				break;
			default:
				break;
			}
		}
	}

	return dwPSMax;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#if 0	// CHB 2006.06.14
void dx9_GetShaderVersionsFromGroup( int & nVSVersion, int & nPSVersion, int nShaderGroup )
{
	switch( nShaderGroup )
	{
	case SHADER_LEVEL_30:
		nVSVersion = VS_3_0;
		nPSVersion = PS_3_0;
		break;
	case SHADER_LEVEL_20:
		nVSVersion = VS_2_a;
		nPSVersion = PS_2_a;
		break;
	case SHADER_LEVEL_11:
		nVSVersion = VS_1_1;
		nPSVersion = PS_1_4;
		break;
	default:
	case SHADER_LEVEL_FF:
		nVSVersion = VS_NONE;
		nPSVersion = PS_NONE;
		break;
	}
}
#endif

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static DWORD sGetTotalEstimatedVideoMemoryMB()
{
	ASSERT_RETZERO( dxC_GetDevice() );
	return dxC_GetDevice()->GetAvailableTextureMem() / BYTES_PER_MB;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static BOOL sSupportsOcclusionQueries()
{
	PRESULT pr = dxC_GetDevice()->CreateQuery( D3DQUERYTYPE_OCCLUSION, NULL );
	return SUCCEEDED( pr );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

//static BOOL sMeetsMinimumPlatformCaps()
//{
//	// define minimum caps for platform-specific here:
//
//	// ... this should be a validation function with a clean error reporting scheme
//
//	// ... this should also be done while filling in settings (validate against "minspec" file)
//}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static BOOL sMeetsMinimumGeneralCaps()
{
	// define minimum caps for platform-specific here:

	// VERY basic minimum caps here

	return TRUE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_GatherNonDeviceCaps()
{
	// for caps that are needed for device create

	// order is important here -- some caps rely on the results of other caps

	BOOL bHAL = sGetHardwareAccelerated();
	dx9_PlatformCapSetValue( DX9CAP_3D_HARDWARE_ACCELERATION,			(DWORD)bHAL );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_GatherPlatformCaps()
{
	// CHB 2007.03.20 - Moved this from dxC_InitializeDeviceObjects().
	// The call to sGetGPUVendorID() below depends on this.
	dxC_DetectAdapter( dxC_GetAdapter() ); // APE 2007.03.09 - I added this, so gtAdapter gets initialized - e_GatherCaps() depends on it.

	e_CapSetValue( CAP_PHYSICAL_VIDEO_MEMORY_MB,			SysCaps_Get().dwPhysicalVideoMemoryBytes / BYTES_PER_MB );
	e_CapSetValue( CAP_SYSTEM_MEMORY_MB,					(DWORD)( SysCaps_Get().dwlPhysicalSystemMemoryBytes / (DWORDLONG)BYTES_PER_MB ) );
	e_CapSetValue( CAP_CPU_SPEED_MHZ,						SysCaps_Get().dwCPUSpeedMHz );
	e_CapSetValue( CAP_SYSTEM_CLOCK_SPEED_MHZ,				SysCaps_Get().dwSysClockSpeedMHz );
	e_CapSetValue( CAP_NATIVE_RESOLUTION_WIDTH,				SysCaps_Get().dwNativeResolutionWidth );
	e_CapSetValue( CAP_NATIVE_RESOLUTION_HEIGHT,			SysCaps_Get().dwNativeResolutionHeight );

	if ( ! dxC_GetDevice() )
	{
		dx9_PlatformCapSetValue( DX9CAP_GPU_VENDOR_ID,		0 );
	}
	else
	{
		dx9_PlatformCapSetValue( DX9CAP_GPU_VENDOR_ID,		sGetGPUVendorID() );
	}

	if ( e_IsNoRender() )
		return S_FALSE;

	if ( dxC_ShouldUsePixomatic() )
	{
		// CML TODO:  fill in actual Pixomatic caps here
		dx9_PlatformCapSetValue( DX9CAP_MAX_VS_VERSION,						0 );
		dx9_PlatformCapSetValue( DX9CAP_MAX_PS_VERSION,						0 );
		dx9_PlatformCapSetValue( DX9CAP_MAX_TEXTURE_WIDTH,					2048 );
		dx9_PlatformCapSetValue( DX9CAP_MAX_TEXTURE_HEIGHT,					2048 );
		dx9_PlatformCapSetValue( DX9CAP_MAX_SIMUL_RENDER_TARGETS,			1 );

		// If we should be using pixo and there's no device, we probably turned off the minimal device caps check and can safely exit
		if ( ! dxC_GetDevice() )
			return S_FALSE;
	}

	ASSERT_RETFAIL( dxC_GetDevice() );

	D3DCAPS9 tDeviceCaps;
	V_RETURN( dxC_GetDevice()->GetDeviceCaps( &tDeviceCaps ) );

	// CHB 2006.06.13 - Used for testing.
#if 0
	tDeviceCaps.PixelShaderVersion = D3DPS_VERSION(1, 1);
	tDeviceCaps.VertexShaderVersion = D3DVS_VERSION(1, 1);
#endif

	// CHB 2006.12.29 - Check for an 'infinite' number of active lights.
#ifdef DX9_DEVICE_ALLOW_SWVP
	if (tDeviceCaps.MaxActiveLights > 4096)		// 4096 seems to be D3D's internal limit
	{
		tDeviceCaps.MaxActiveLights = 1;
	}
#endif

	// order is important here -- some caps rely on the results of other caps

	dx9_PlatformCapSetValue( DX9CAP_HW_SHADOWS,							sGetHardwareShadowsSupported() );
	dx9_PlatformCapSetValue( DX9CAP_NULL_RENDER_TARGET,					sGetNullRenderTargetSupported() );
	dx9_PlatformCapSetValue( DX9CAP_DEPTHBIAS,							SUPPORTS( tDeviceCaps.RasterCaps, D3DPRASTERCAPS_DEPTHBIAS ) );
	dx9_PlatformCapSetValue( DX9CAP_SLOPESCALEDEPTHBIAS,				SUPPORTS( tDeviceCaps.RasterCaps, D3DPRASTERCAPS_SLOPESCALEDEPTHBIAS ) );

	dx9_PlatformCapSetValue( DX9CAP_GPU_MULTIPLE_CARDS_MODE,			sGetGPUMultipleCardsMode() );
	dx9_PlatformCapSetValue( DX9CAP_MAX_ZBUFFER_DEPTH_BITS,				sGetMaxZBufferBitdepth() );
	dx9_PlatformCapSetValue( DX9CAP_MAX_VS_VERSION,						tDeviceCaps.VertexShaderVersion );

	dx9_PlatformCapSetValue( DX9CAP_MAX_PS_VERSION,						tDeviceCaps.PixelShaderVersion );

	{
		DWORD nProf = 0;
		const char * pProf = D3DXGetPixelShaderProfile(dxC_GetDevice());
		if (pProf != 0)
		{
			nProf = MAKEFOURCC(pProf[0], pProf[1], pProf[3], pProf[5]);
		}
		dx9_PlatformCapSetValue(DX9CAP_PIXEL_SHADER_PROFILE, nProf);
	}

	// now just reports to debug output detected shader version support
	sGetPixelShaderVersion();

	dx9_PlatformCapSetValue( DX9CAP_MAX_VS_CONSTANTS,					tDeviceCaps.MaxVertexShaderConst );
	//dx9_PlatformCapSetValue( DX9CAP_MAX_PS_CONSTANTS,					tDeviceCaps. );
	dx9_PlatformCapSetValue( DX9CAP_MAX_VS_STATIC_BRANCH,				tDeviceCaps.VS20Caps.StaticFlowControlDepth );
	dx9_PlatformCapSetValue( DX9CAP_MAX_PS_STATIC_BRANCH,				tDeviceCaps.PS20Caps.StaticFlowControlDepth );
	dx9_PlatformCapSetValue( DX9CAP_MAX_VS_DYNAMIC_BRANCH,				tDeviceCaps.VS20Caps.DynamicFlowControlDepth );
	dx9_PlatformCapSetValue( DX9CAP_MAX_PS_DYNAMIC_BRANCH,				tDeviceCaps.PS20Caps.DynamicFlowControlDepth );
	dx9_PlatformCapSetValue( DX9CAP_MAX_PS20_INSTRUCTIONS,				tDeviceCaps.PS20Caps.NumInstructionSlots );
	dx9_PlatformCapSetValue( DX9CAP_MAX_TEXTURE_WIDTH,					tDeviceCaps.MaxTextureWidth );
	dx9_PlatformCapSetValue( DX9CAP_MAX_TEXTURE_HEIGHT,					tDeviceCaps.MaxTextureHeight );
	dx9_PlatformCapSetValue( DX9CAP_MAX_SIMUL_RENDER_TARGETS,			tDeviceCaps.NumSimultaneousRTs );
	dx9_PlatformCapSetValue( DX9CAP_LINEAR_GAMMA_SHADER_IN,				sMeetsTextureFormatSRGBSupport() );
	dx9_PlatformCapSetValue( DX9CAP_LINEAR_GAMMA_SHADER_OUT,			sCanOutputLinearToSRGB() );
	dx9_PlatformCapSetValue( DX9CAP_FLOAT_HDR,							sSupportsFloatHDR() );
	dx9_PlatformCapSetValue( DX9CAP_INTEGER_HDR,						sSupportsIntegerHDR() );
	dx9_PlatformCapSetValue( DX9CAP_OCCLUSION_QUERIES,					sSupportsOcclusionQueries() );
	dx9_PlatformCapSetValue( DX9CAP_MINIMUM_TEXTURE_FORMATS,			sMeetsMinimumTextureFormats() );
	dx9_PlatformCapSetValue( DX9CAP_MINIMUM_RENDERTARGET_FORMATS,		sMeetsMinimumRenderTargetFormats() );
	dx9_PlatformCapSetValue( DX9CAP_MINIMUM_DEPTH_FORMATS,				sMeetsMinimumDepthFormats() );
	dx9_PlatformCapSetValue( DX9CAP_SCISSOR_TEST,						SUPPORTS( tDeviceCaps.RasterCaps, D3DPRASTERCAPS_SCISSORTEST ) );
	dx9_PlatformCapSetValue( DX9CAP_MINIMUM_GENERAL_CAPS,				sMeetsMinimumGeneralCaps() );
	dx9_PlatformCapSetValue( DX9CAP_HARDWARE_MOUSE_CURSOR,				SUPPORTS( tDeviceCaps.CursorCaps, D3DCURSORCAPS_COLOR ) );
	dx9_PlatformCapSetValue( DX9CAP_MAX_USER_CLIP_PLANES,				tDeviceCaps.MaxUserClipPlanes );
	dx9_PlatformCapSetValue( DX9CAP_ANISOTROPIC_FILTERING,				sSupportsAnisotropicFiltering( tDeviceCaps ) );
	dx9_PlatformCapSetValue( DX9CAP_TRILINEAR_FILTERING,				sSupportsTrilinearFiltering( tDeviceCaps ) );
	dx9_PlatformCapSetValue( DX9CAP_SMALLEST_COLOR_TARGET,				sSmallestColorTargetFormat() );
	dx9_PlatformCapSetValue( DX9CAP_COVERAGE_SAMPLING,					sSupportsCoverageSampling() );
	dx9_PlatformCapSetValue( DX9CAP_SEPARATE_ALPHA_BLEND_ENABLE,		SUPPORTS( tDeviceCaps.PrimitiveMiscCaps, D3DPMISCCAPS_SEPARATEALPHABLEND ) );

	dx9_PlatformCapSetValue( DX9CAP_MAX_POINT_SIZE,						(DWORD)tDeviceCaps.MaxPointSize );
	dx9_PlatformCapSetValue( DX9CAP_MAX_ACTIVE_LIGHTS,					tDeviceCaps.MaxActiveLights );
	dx9_PlatformCapSetValue( DX9CAP_MAX_VERTEX_STREAMS,					tDeviceCaps.MaxStreams );
	dx9_PlatformCapSetValue( DX9CAP_MAX_ACTIVE_RENDERTARGETS,			tDeviceCaps.NumSimultaneousRTs );
	dx9_PlatformCapSetValue( DX9CAP_MAX_ACTIVE_TEXTURES,				tDeviceCaps.MaxSimultaneousTextures );
	dx9_PlatformCapSetValue( DX9CAP_HARDWARE_GAMMA_RAMP,				SUPPORTS( tDeviceCaps.Caps2, D3DCAPS2_FULLSCREENGAMMA ) );
	dx9_PlatformCapSetValue( DX9CAP_VS_POWER,							0 );
	dx9_PlatformCapSetValue( DX9CAP_PS_POWER,							0 );

	e_CapSetValue( CAP_TOTAL_ESTIMATED_VIDEO_MEMORY_MB,		sGetTotalEstimatedVideoMemoryMB() );
	//e_CapSetValue( CAP_MINIMUM_PLATFORM_CAPS,				dx9_MeetsMinimumGeneralCaps() );
	//e_CapSetValue( CAP_MINIMUM_GENERAL_CAPS,				e_MeetsMinimumGeneralCaps() );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int dx9_GetMaxTextureStages()
{
	DWORD dwStages = dx9_CapGetValue( DX9CAP_MAX_ACTIVE_TEXTURES );
	ASSERT_RETZERO( dwStages < 65536 );	// sanity check to make sure we aren't grabbing uninitialized data
	return dwStages;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

int dx9_GetMaxStreams()
{
	DWORD dwStreams = dx9_CapGetValue( DX9CAP_MAX_VERTEX_STREAMS );
	ASSERT_RETZERO( dwStreams < 65536 );	// sanity check to make sure we aren't grabbing uninitialized data
	return dwStreams;
}
