//----------------------------------------------------------------------------
// dx10_device.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"

#include "dx10_settings.h"
#include "dxC_settings.h"
#include "..\\dx9\\dx9_settings.h"
#include "dxC_target.h"
#include "e_optionstate.h"
#include "dxC_device.h"
#include "dxC_state.h"
#include "e_screen.h"
#include "e_automap.h"
#include "dxC_movie.h"
#include "dxC_main.h"

using namespace FSSE;

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
//SPDXGISWAPCHAIN												gpSwapChain = NULL;
SPDXGIFACTORY												gpDXGIFactory = NULL;
SPDXGIOUTPUT												gpDXGIOutput = NULL;
ID3D10Debug*												g_pD3D10Debug = NULL;
ID3D10SwitchToRef*											g_pD3D10SwitchToRef = NULL;
ID3D10InfoQueue*											g_pD3D10InfoQue = NULL;


void dx10_KillEverything()
{
	extern DXCSWAPCHAIN gtSwapChains[ MAX_SWAP_CHAINS ];
	extern SPD3DCDEVICE gpDevice;
	extern DXCADAPTER s_pAdapter;

	for ( int i = 0; i < MAX_SWAP_CHAINS; ++i )
	{
		if( gtSwapChains[i].pSwapChain )
		{
			gtSwapChains[i].pSwapChain->SetFullscreenState( false, NULL ); //Can't be fullscreen if we're to destroy the swapchain
			gtSwapChains[i].pSwapChain = NULL;
		}
	}

	gpDevice = NULL;
	SAFE_RELEASE( s_pAdapter );
	gpDXGIOutput = NULL;
	gpDXGIFactory = NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_GetAdapterOrdinalFromWindowPos( UINT & nAdapterOrdinal )
{
	HMONITOR hMon = NULL;
	V_RETURN( dxC_GetDefaultMonitorHandle( hMon ) );
	ASSERT_RETFAIL( hMon );

	UINT nOutput;
	UINT nAdapter = 0;
	IDXGIAdapter* adapter = NULL;
	while ( DXGI_ERROR_NOT_FOUND != gpDXGIFactory->EnumAdapters( nAdapter, &adapter) )
	{
		if ( adapter )
		{
			nOutput = 0;
			IDXGIOutput * output = NULL;
			while ( DXGI_ERROR_NOT_FOUND != adapter->EnumOutputs( nOutput, &output ) )
			{
				if ( output )
				{
					DXGI_OUTPUT_DESC tOutputDesc;
					V( output->GetDesc( &tOutputDesc ) );
					if ( tOutputDesc.Monitor == hMon )
					{
						// Found our monitor/adapter!

						nAdapterOrdinal = nAdapter;
						//pState->set_nAdapterOrdinal( nAdapter );
						//e_OptionState_CommitToActive( (const OptionState*)pState );

						output->Release();
						adapter->Release();
						return S_OK;
					}
					output->Release();
				}
				++nOutput;
			}
			adapter->Release();
		}
		++nAdapter;
	}

	return E_FAIL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dx10_Init( OptionState* pState )
{
	dx10_KillEverything();

	// Create our DXGI factory
	CreateDXGIFactory( __uuidof(IDXGIFactory), (void**)&gpDXGIFactory.p);
	ASSERT_RETFAIL(gpDXGIFactory);

	IDXGIAdapter * pAdapter = NULL;

	// Search for a PerfHUD adapter.
#if ISVERSION(DEVELOPMENT)
	if ( pState )
	{
		UINT nAdapterOrdinal = pState->get_nAdapterOrdinal();
		D3DC_DRIVER_TYPE nDeviceDriverType = pState->get_nDeviceDriverType();

		UINT nAdapter = 0;
		IDXGIAdapter* adapter = NULL;
		while ( gpDXGIFactory->EnumAdapters(nAdapter, &adapter) != DXGI_ERROR_NOT_FOUND )
		{
			if (adapter)
			{
				DXGI_ADAPTER_DESC adaptDesc;
				V_DO_SUCCEEDED( adapter->GetDesc( &adaptDesc ) )
				{
					if ( PStrCmp( adaptDesc.Description, L"NVIDIA PerfHUD" ) == 0 )
					{
						pAdapter = adapter;
						nAdapterOrdinal = nAdapter;
						nDeviceDriverType = D3D10_DRIVER_TYPE_REFERENCE;
						LogMessage( LOG_FILE, "Found NVIDIA NVPerfHUD device!\n" );
						break;
					}
				}
				adapter->Release();
			}
			++nAdapter;
		}

		pState->set_nAdapterOrdinal( nAdapterOrdinal );
		pState->set_nDeviceDriverType( nDeviceDriverType );
		e_OptionState_CommitToActive( (const OptionState*)pState );
	}
#endif

	UINT nOutput = 0;	// default to first enumerated output for this adapter

	if ( ! pAdapter )
	{
		if ( ! pState )
		{
			V_RETURN( gpDXGIFactory->EnumAdapters( pState ? pState->get_nAdapterOrdinal() : 0, &pAdapter ) );
		}
		else
		{
			UINT nAdapterOrdinal = pState->get_nAdapterOrdinal();
			if ( SUCCEEDED( dxC_GetAdapterOrdinalFromWindowPos( nAdapterOrdinal ) ) )
			{
				IDXGIAdapter * adapter = NULL;
				PRESULT pr = gpDXGIFactory->EnumAdapters( nAdapterOrdinal, &adapter );
				if ( pr != DXGI_ERROR_NOT_FOUND )
				{
					if ( SUCCEEDED(pr) )
					{
						pAdapter = adapter;
						pState->set_nAdapterOrdinal( nAdapterOrdinal );
						e_OptionState_CommitToActive( (const OptionState*)pState );
					}
				}
			}
		}
	}

	dxC_SetAdapter( pAdapter );
	ASSERTV_RETFAIL(dxC_GetAdapter() != NULL, "Couldn't find dx10 adapter for ordinal %d!", pState->get_nAdapterOrdinal() );
	DXGI_ADAPTER_DESC adaptDesc;
	V_RETURN( dxC_GetAdapter()->GetDesc( &adaptDesc ) );


	V_RETURN( dxC_GetAdapter()->EnumOutputs( nOutput, &gpDXGIOutput.p ) );
	ASSERTX_RETFAIL(dx10_DXGIGetOutput() != NULL, "Couldn't find dx10 output!");

	//Finally create the device
	D3D10_DRIVER_TYPE driverType = pState ? pState->get_nDeviceDriverType() : D3D10_DRIVER_TYPE_HARDWARE;
	return dx10_CreateDevice( AppCommonGetHWnd(), driverType );
}

HRESULT dx10_CreateBuffer(SIZE_T ByteWidth, D3D10_USAGE Usage, UINT BindFlags, UINT CPUAccessFlags, ID3D10Buffer ** ppBuffer, void* pInitialData )
{
	D3D10_BUFFER_DESC buffDesc;
	buffDesc.ByteWidth = ByteWidth;
	buffDesc.Usage = Usage;
	buffDesc.BindFlags = BindFlags;
	buffDesc.CPUAccessFlags = CPUAccessFlags;
	buffDesc.MiscFlags = 0;

	if( pInitialData )
    {
        D3D10_SUBRESOURCE_DATA initData;
        ZeroMemory( &initData, sizeof(D3D10_SUBRESOURCE_DATA) );
        initData.pSysMem = pInitialData;
        initData.SysMemPitch = ByteWidth;  //Pitch is always the size because a buffer is 1D

		return dxC_GetDevice()->CreateBuffer( &buffDesc, &initData, ppBuffer );
	}
	else
	    return dxC_GetDevice()->CreateBuffer(&buffDesc, NULL, ppBuffer);
}


PRESULT dxC_Device_RegisterEventHandler(void)
{
	return S_OK;
}

PRESULT dx10_GrabBBPointers()
{
	PRESULT pr = S_OK;

	if ( ! dxC_GetD3DSwapChain() )
		return S_FALSE;

	int nSwapChainIndex = DEFAULT_SWAP_CHAIN;

	//Grab the backbuffer
	RENDER_TARGET_INDEX nRT = dxC_GetSwapChainRenderTargetIndex( nSwapChainIndex, SWAP_RT_FINAL_BACKBUFFER );
	if ( nRT == RENDER_TARGET_INVALID )
	{
		// Add the RT desc for this

		const D3DPRESENT_PARAMETERS & tPP = dxC_GetD3DPP( nSwapChainIndex );

		DXCSWAPCHAIN * ptSC = dxC_GetDXCSwapChain( nSwapChainIndex );
		ptSC->nRenderTargets[ SWAP_RT_FINAL_BACKBUFFER ] = (RENDER_TARGET_INDEX)dxC_AddRenderTargetDesc(
			TRUE,
			nSwapChainIndex,
			SWAP_RT_FINAL_BACKBUFFER,
			tPP.BufferDesc.Width,
			tPP.BufferDesc.Height,
			D3DC_USAGE_2DTEX_RT,
			(DXC_FORMAT)tPP.BackBufferFormat,
			ptSC->nRenderTargets[ SWAP_RT_FINAL_BACKBUFFER ],
			FALSE,
			1,
			tPP.MultiSampleType,
			tPP.MultiSampleQuality );

		nRT = dxC_GetSwapChainRenderTargetIndex( nSwapChainIndex, SWAP_RT_FINAL_BACKBUFFER );
		ASSERT( nRT != RENDER_TARGET_INVALID );
	}


	V_RETURN( dxC_GetD3DSwapChain()->GetBuffer( 0, __uuidof( ID3D10Texture2D ), (LPVOID*)&dxC_RTResourceGet( nRT ).p ) );
	V_RETURN( dxC_CreateRTV( dxC_RTResourceGet( nRT ), &dxC_RTSurfaceGet( nRT ).p ) );
	V_RETURN( dx10_RenderTargetAddSRV( nRT ) );

	return pr;
}

void dx10_CreateDepthBuffer( int nSwapChainIndex )
{
	//Need to create our primary depth stencil view, this is a D3DC_USAGE_2DTEX_SM because we want to be able to sample from it
	//hardcoding format
	dxC_AddRenderTargetDesc( FALSE, nSwapChainIndex, SWAP_DT_AUTO, e_GetWindowWidth(), e_GetWindowHeight(), ( e_OptionState_GetActive()->get_nMultiSampleType() <= 1 ) ? D3DC_USAGE_2DTEX_SM : D3DC_USAGE_2DTEX_DS, D3DFMT_D24X8, -1, 0, 1, e_OptionState_GetActive()->get_nMultiSampleType(), e_OptionState_GetActive()->get_nMultiSampleQuality() );
}

static
PRESULT sResizePrimaryTargets()
{
	RENDER_TARGET_INDEX eRTFinal = dxC_GetSwapChainRenderTargetIndex( DEFAULT_SWAP_CHAIN, SWAP_RT_FINAL_BACKBUFFER );
	dxC_RTReleaseAll( eRTFinal );

	DEPTH_TARGET_INDEX eDTAuto = dxC_GetSwapChainDepthTargetIndex( DEFAULT_SWAP_CHAIN, SWAP_DT_AUTO );
	dxC_DSResourceGet( eDTAuto ) = NULL;
	dxC_DSShaderResourceView( eDTAuto ) = NULL;
	dxC_DSSurfaceGet( eDTAuto ) = NULL;
	dxC_DSSetDirty( eDTAuto, true );

	dxC_SetRenderTargetWithDepthStencil( RENDER_TARGET_NULL, DEPTH_TARGET_NONE );  //Need to do this so we set the targets when we're done

	extern DXCSWAPCHAIN gtSwapChains[ MAX_SWAP_CHAINS ];
	for ( int i = 0; i < MAX_SWAP_CHAINS; ++i )
	{
		if( gtSwapChains[i].pSwapChain )  	//Just re-create the swap chain this handles all mode changes and MSAA changes
		{
			gtSwapChains[i].pSwapChain->SetFullscreenState( false, NULL ); //Can't be fullscreen if we're to destroy the swapchain
			gtSwapChains[i].pSwapChain = NULL;
		}
		dxC_ReleaseRTsDependOnBB( i );
	}
	// Recreate main swap chain
	ASSERT( ! gtSwapChains[0].pSwapChain );
	if ( ! gtSwapChains[0].pSwapChain )
	{
		V( dx10_CreateSwapChain( e_OptionState_GetActive(), &gtSwapChains[0].pSwapChain.p ) );
	}

	dx10_GrabBBPointers();

	dx10_CreateDepthBuffer( DEFAULT_SWAP_CHAIN );

	dxC_AddDescRTsDependOnBB( DEFAULT_SWAP_CHAIN );

	e_AutomapsMarkDirty();
	V( dxC_MoviesDeviceReset() );

	return S_OK;
}

PRESULT dx10_ResizePrimaryTargets()
{
	e_SetChangingDevice(true);	// CHB 2007.08.23
	PRESULT nResult = sResizePrimaryTargets();
	e_SetChangingDevice(false);	// CHB 2007.08.23
	return nResult;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//PRESULT dx10_ToggleFullscreen(
//						void)
//{
//	return dxC_GetSwapChain()->SetFullscreenState( !e_OptionState_GetActive()->get_bWindowed(), NULL );
//}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PRESULT dx10_CreateSwapChain( const OptionState * pState, IDXGISwapChain ** ppSwapChainOut )
{
	D3DPRESENT_PARAMETERS tPresentationParameters;
	dxC_StateToPresentation(pState, tPresentationParameters);
	tPresentationParameters.hDeviceWindow = AppCommonGetHWnd();

	// CHB 2007.09.06 - Without this flag, DX10 will not actually
	// select the right display resolution for full screen mode.
	tPresentationParameters.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	// We have to be the foreground window before this call is made, so try to force the issue.
	//SetActiveWindow( tPresentationParameters.OutputWindow );
	//SetForegroundWindow( tPresentationParameters.OutputWindow );
	////SetWindowPos( tPresentationParameters.OutputWindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW );
	//AppCommonRunMessageLoop();

	PRESULT nResult = gpDXGIFactory->CreateSwapChain( dxC_GetDevice(), &tPresentationParameters, ppSwapChainOut );

	// CHB 2007.08.24 - Note: if CreateSwapChain() returns
	// DXGI_STATUS_OCCLUDED (not an error), that means it
	// failed to acquire full screen mode.
	// SetFullscreenState() will also fail in this case.
	// This will happen if the window is not the active
	// foreground window when this is called.
	//ASSERT((tPresentationParameters.Windowed != 0) || (nResult != DXGI_STATUS_OCCLUDED));

	if (SUCCEEDED(nResult))
	{
#if ISVERSION(DEVELOPMENT)
		DXGI_SWAP_CHAIN_DESC ChainDesc;
		DXGI_OUTPUT_DESC OutputDesc;
		(*ppSwapChainOut)->GetDesc(&ChainDesc);
		CComPtr<IDXGIOutput> pOutput;
		(*ppSwapChainOut)->GetContainingOutput(&pOutput);
		if (pOutput != 0)
		{
			pOutput->GetDesc(&OutputDesc);
		}
#endif
		// CHB 2007.09.21 - Disable DX10's helpful handling of Alt-Enter.
		// Note that using DXGI_MWA_NO_ALT_ENTER by itself is ineffective
		// due to a bug in the SDK. See http://forums.xna.com/thread/8127.aspx
		gpDXGIFactory->MakeWindowAssociation(tPresentationParameters.OutputWindow, DXGI_MWA_NO_WINDOW_CHANGES | DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_PRINT_SCREEN );

		//
		( tPresentationParameters.MultiSampleType > 1 ) ? dxC_SetRenderState( D3DRS_MULTISAMPLEENABLE, true ) : dxC_SetRenderState( D3DRS_MULTISAMPLEENABLE, false );
		dxC_SetD3DPresentation(tPresentationParameters);
	}

	// CHB 2007.08.29 - In DX10, acquiring full screen mode will
	// fail if the window is not the active foreground window at
	// the time CreateSwapChain() or SetFullscreenState() is
	// called. See the comment at the definition of
	// dx10_CheckFullscreenMode() for a detailed explanation.
	dx10_CheckFullscreenMode(false);

	return nResult;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
// CHB 2007.08.27
/*
	Full screen mode in DX10 is a bit of a pain. Either that or I'm
	missing some fundamental concept.

	Full screen can be taken away from you at any time, or may fail
	to be acquired when requested. Here are some common scenarios
	where full screen mode is lost or fails to be acquired:

		*	The user alt-tabs to another application

		*	The window loses its status as active foreground
			window

		*	The window is not the active foreground window when
			CreateSwapChain() is called.

	Dealing with this is not as straightforward as it might seem.
	For instance, what do you do when the user's settings request
	full screen but it failed to acquire during initialization?

	The first solution attempted involved polling the full screen
	mode every frame and restoring full screen if needed. This works
	great except that when the monitor is powered down by the power
	policy, you lose full screen. Restoring full screen every frame
	causes the monitor to continually power up and down in a cycle.
	Detecting monitor power down is straightforward (with the
	WM_SYSCOMMAND message), but no reliable way of detecting monitor
	power up was found. Keep in mind that when the monitor powers
	down, the activation and foreground states of the window do not
	change, however DirectX takes full screen status away from you.
	Also, the swap chain is not considered to be occluded while the
	monitor is powered down.

	Thus a design decision was made to move the engine to windowed
	mode when full screen is lost. This works, especially for the
	initialization case, though with some annoyances:

		*	Monitor power-down sends the application to windowed mode

		*	Alt-tabbing away sends the application to windowed mode,
			and alt-tabbing back does not return to full screen mode

	Handling the latter case (alt-tabbing) was managed by watching
	the WM_ACTIVATE message. A satisfactory solution for the former
	case was not found.
*/
PRESULT dx10_CheckFullscreenMode(bool bReestablishFullscreen)
{
	// Only run this function if we want to be in fullscreen
	if ( e_OptionState_GetActive()->get_bWindowed() && ! dxC_GetWantFullscreen() )
	{
		return S_OK;
	}

	LPD3DCSWAPCHAIN pChain = dxC_GetD3DSwapChain();
	if (pChain == 0)
	{
		return S_OK;
	}

	BOOL bFullscreen = FALSE;
	V_RETURN(pChain->GetFullscreenState(&bFullscreen, 0));
	if (bFullscreen)
	{
		return S_OK;
	}

	// We've lost full screen mode, but we've just been
	// activated, so try to restore full screen.
	// (For example, just alt-tabbed back.)
	if (bReestablishFullscreen)
	{
		DXGI_SWAP_CHAIN_DESC Desc;
		HRESULT nResult = pChain->GetDesc(&Desc);

		nResult = pChain->SetFullscreenState(true, 0);
		switch (nResult)
		{
			case DXGI_ERROR_NOT_CURRENTLY_AVAILABLE:
			case DXGI_STATUS_OCCLUDED:
				break;
			default:
				V_RETURN(nResult);
				nResult = pChain->GetDesc(&Desc);
				bFullscreen = TRUE;
		}
	}


	//if ( ! bFullscreen )
	//{
		// We've lost full screen mode, so switch to windowed.
		// First, record that we want fullscreen mode so that later we know to go back to it.
		// Then officially switch to windowed (for the time being).
	//}

	dxC_SetWantFullscreen( ! bFullscreen );

	CComPtr<OptionState> pState;
	V_RETURN(e_OptionState_CloneActive(&pState));
	pState->set_bWindowed( ! bFullscreen );
	V_RETURN(e_OptionState_CommitToActive(pState));
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dx10_GetDisplayModeList( DXC_FORMAT tFormat, UINT * pIndex, DXGI_MODE_DESC * pModeDesc )
{
	ASSERT( pIndex );

	// CHB 2007.09.06 - I don't think we want scaling modes.
	return dx10_DXGIGetOutput()->GetDisplayModeList( tFormat, DXGI_ENUM_MODES_INTERLACED /* | DXGI_ENUM_MODES_SCALING*/, pIndex, pModeDesc );
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
template<class T, class U>
inline
void sConvert(const T & From, U & To)
{
	To.Width = From.Width;
	To.Height = From.Height;
	To.RefreshRate.Numerator = From.RefreshRate.Numerator;
	To.RefreshRate.Denominator = From.RefreshRate.Denominator;
	To.Format = static_cast<DXGI_FORMAT>(From.Format);
	To.ScanlineOrdering = static_cast<DXGI_MODE_SCANLINE_ORDER>(From.ScanlineOrdering);
	To.Scaling = static_cast<DXGI_MODE_SCALING>(From.Scaling);
}

const E_SCREEN_DISPLAY_MODE dxC_ConvertNativeDisplayMode(const DXGI_MODE_DESC & Desc)
{
	E_SCREEN_DISPLAY_MODE Mode;
	sConvert(Desc, Mode);
	return Mode;
}

const DXGI_MODE_DESC dxC_ConvertNativeDisplayMode(const E_SCREEN_DISPLAY_MODE & Mode)
{
	DXGI_MODE_DESC Desc;
	sConvert(Mode, Desc);
	return Desc;
}
