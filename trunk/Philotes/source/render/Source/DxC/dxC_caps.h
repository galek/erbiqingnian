//----------------------------------------------------------------------------
// dxC_caps.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DX9_CAPS__
#define __DX9_CAPS__

#include "e_caps.h"

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------


#define DT_NVIDIA_VENDOR_ID		0x10DE
#define DT_ATI_VENDOR_ID		0x1002
#define DT_S3_VENDOR_ID			0x5333
#define DT_INTEL_VENDOR_ID		0x8086
#define DT_3DLABS_VENDOR_ID		0x3D3D
#define DT_MATROX_VENDOR_ID		0x102B
#define DT_SIS_VENDOR_ID		0x1039
#define DT_XGI_VENDOR_ID		0x18CA


#define DT_ENGINE_TARGET_DX9_ID		MAKEFOURCCSTR( "DX09" )
#define DT_ENGINE_TARGET_DX10_ID	MAKEFOURCCSTR( "DX10" )

#define DX9_MAX_SAMPLERS		16

//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------

#define IS_VALID_PLATFORM_CAP_TYPE( eType )	( (eType) > DX9CAP_DISABLE && (eType) < NUM_PLATFORM_CAPS )

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

#define INCLUDE_CAPS_ENUM
enum PLATFORM_CAP
{
	DX9CAP_DISABLE = 0,
#include "..\\dx9\\dx9_caps_def.h"
	// count
	NUM_PLATFORM_CAPS
};
#undef INCLUDE_CAPS_ENUM


//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

struct VideoDeviceInfo
{
	static const int MAX_NAME_LEN = 128;

	DWORD VendorID;
	DWORD DeviceID;
	DWORD SubSysID;
	DWORD Revision;
	char szDeviceName[ MAX_NAME_LEN ];
	LARGE_INTEGER DriverVersion;

	PRESULT FromD3D( const void * pD3DAdapter);
};

struct PlatformCap
{
	DWORD dwFourCC;
	char szDesc[ 128 ];
	BOOL bSmallerBetter;
};

//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

//void dx9_DetectGeneralD3DCapabilities( DWORD dwAdapter, D3DC_DRIVER_TYPE tType );
PRESULT dxC_DetectAdapter( DXCADAPTER tAdapter );
//void dx9_DetectD3DDeviceCapabilities();
void dx9_CalculateMaxPhysicalVideoMemory();
void dx9_DetermineTextureMemoryAdjustment();
HRESULT dx9_DetectNonD3DCapabilities();
int dx9_GetMaxTextureStages();
int dx9_GetMaxStreams();
int dxC_CapsGetMaxSquareTextureResolution();
//void dx9_PlatformCapSetValue( const PLATFORM_CAP eCap, DWORD dwDisplayMode, DWORD dwColorDepthMode, DWORD dwValue );
PRESULT dx9_PlatformCapSetValue( const PLATFORM_CAP eCap, DWORD dwValue );
PRESULT dx9_PlatformCapGetMinMax( const PLATFORM_CAP eCap, DWORD & nMin, DWORD & nMax );	// CHB 2007.01.18
PRESULT dx9_PlatformCapSetMinMax( const PLATFORM_CAP eCap, DWORD nMin, DWORD nMax );	// CHB 2007.01.02
VERTEX_SHADER_VERSION dxC_CapsGetMaxVertexShaderVersion();
PIXEL_SHADER_VERSION dxC_CapsGetMaxPixelShaderVersion();
PRESULT dx9_GatherNonDeviceCaps();
DWORD dx9_CapsGetAugmentedMaxPixelShaderVersion(void);	// CHB 2007.07.25

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------
//Please access caps through dx9_CapGetValue
//inline const D3DCAPS9 & dx9_GetDeviceCaps()
//{
//	extern D3DCAPS9 gtD3DDeviceCaps;
//	return gtD3DDeviceCaps;
//}

inline const char * dx9_CapGetDescription( const PLATFORM_CAP eCap )
{
	ASSERT_RETNULL( IS_VALID_PLATFORM_CAP_TYPE( eCap ) );
	extern PlatformCap gtPlatformCaps[];
	return gtPlatformCaps[ eCap ].szDesc;
}

inline DWORD dx9_CapGetFourCC( const PLATFORM_CAP eCap )
{
	ASSERT_RETNULL( IS_VALID_PLATFORM_CAP_TYPE( eCap ) );
	extern PlatformCap gtPlatformCaps[];
	return gtPlatformCaps[ eCap ].dwFourCC;
}

inline void dx9_CapGetFourCCString( const PLATFORM_CAP eCap, CHAR * pszOutString, int nBufLen )
{
	pszOutString[0] = NULL;
	ASSERT_RETURN( IS_VALID_PLATFORM_CAP_TYPE( eCap ) );
	extern PlatformCap gtPlatformCaps[];
	PStrCopy( pszOutString, (CHAR*) & FOURCCSTRING( gtPlatformCaps[ eCap ].dwFourCC ), min(nBufLen, 5) );	// CHB 2006.12.27
}

DWORD dx9_CapGetValue( const PLATFORM_CAP eCap );	// CHB 2007.01.02

inline BOOL dx9_CapIsSmallerBetter( const PLATFORM_CAP eCap )
{
	ASSERT_RETNULL( IS_VALID_PLATFORM_CAP_TYPE( eCap ) );
	extern PlatformCap gtPlatformCaps[];
	return gtPlatformCaps[ eCap ].bSmallerBetter;
}

#endif // __DX9_CAPS__
