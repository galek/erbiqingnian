//----------------------------------------------------------------------------
// dx9_device.h
//
// - Header for D3D & Device objects
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DX9_DEVICE__
#define __DX9_DEVICE__


//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

// CHB 2006.12.29
#ifdef ENGINE_TARGET_DX9
#define DX9_DEVICE_ALLOW_SWVP 1	// software vertex processing
#endif

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

LPDIRECT3D9 dx9_GetD3D( BOOL bForceRecreate = FALSE );

inline BOOL dx9_DeviceLost()
{
	extern BOOL gbDeviceLost;
	return gbDeviceLost;
}

inline BOOL dx9_DeviceNeedReset()
{
	extern BOOL gbDeviceNeedReset;
	return gbDeviceNeedReset;
}

inline BOOL dx9_DeviceNeedValidate()
{
	extern BOOL gbDebugValidateDevice;
	return gbDebugValidateDevice;
}

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

D3DC_DRIVER_TYPE dx9_GetDeviceType();
void dx9_SetResetDevice();
PRESULT dx9_ReleaseUnmanagedResources();
PRESULT dx9_RestoreUnmanagedResources();
PRESULT dx9_CheckDeviceLost();
PRESULT dx9_RestoreD3DStates( BOOL bForce = FALSE );
PRESULT dx9_Init();
PRESULT dx9_SaveAutoDepthStencil( int nSwapChainIndex );
PRESULT dx9_ResetDevice( void );

const E_SCREEN_DISPLAY_MODE dxC_ConvertNativeDisplayMode(const D3DDISPLAYMODE & Desc);
const D3DDISPLAYMODE dxC_ConvertNativeDisplayMode(const E_SCREEN_DISPLAY_MODE & Mode);

// CHB 2006.12.29
#ifdef DX9_DEVICE_ALLOW_SWVP
bool dx9_IsSWVPDevice(void);
#else
#define dx9_IsSWVPDevice() false
#endif



#endif // __DX9_DEVICE__