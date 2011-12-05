//----------------------------------------------------------------------------
// dx10_device.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __DX10_DEVICE__
#define __DX10_DEVICE__

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

inline LPDXGIOUTPUT dx10_DXGIGetOutput()
{
	extern SPDXGIOUTPUT gpDXGIOutput;
	return gpDXGIOutput;
}

//----------------------------------------------------------------------------
// DECLARATIONS
//----------------------------------------------------------------------------

PRESULT dx10_CreateBuffer(SIZE_T ByteWidth, D3D10_USAGE Usage, UINT BindFlags, UINT CPUAccessFlags, ID3D10Buffer ** ppBuffer, void* pInitialData = NULL );
inline void dx10_RestoreD3DStates(bool force = false){ REF( force ); /*NUTTAPONG TODO: Re-factor DX9_RestoreD3DStates into a dxC call with dx9 and dx10 specifics}*/;};
PRESULT dx10_Init( OptionState* pState = NULL );
PRESULT dx10_ResizePrimaryTargets();
PRESULT dx10_GrabBBPointers();
//PRESULT dx10_ToggleFullscreen();
PRESULT dx10_CreateDevice( HWND hWnd, D3D10_DRIVER_TYPE driverType );
PRESULT dx10_CreateSwapChain( const OptionState * pState, IDXGISwapChain ** ppSwapChainOut );
void dx10_StepDOF();
void dx10_KillEverything();  //Release swapchain, device, output and factory
PRESULT dx10_CheckFullscreenMode(bool bReestablishFullscreen);	// CHB 2007.08.29
PRESULT dx10_GetDisplayModeList( DXC_FORMAT tFormat, UINT* pIndex, DXGI_MODE_DESC * pModeDesc );
const E_SCREEN_DISPLAY_MODE dxC_ConvertNativeDisplayMode(const DXGI_MODE_DESC & Desc);
const DXGI_MODE_DESC dxC_ConvertNativeDisplayMode(const E_SCREEN_DISPLAY_MODE & Mode);
void dx10_CreateDepthBuffer( int nSwapChainIndex );	// CHB 2007.09.10

#endif //__DX10_DEVICE__
