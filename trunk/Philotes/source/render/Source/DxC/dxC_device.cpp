//----------------------------------------------------------------------------
// dxC_device.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"

#include "dxC_device_priv.h"
#include "dxC_settings.h"
#include "dxC_caps.h"
#include "dxC_light.h"
#include "dxC_effect.h"
#include "dxC_shadow.h"
#include "dxC_target.h"
#include "dxC_drawlist.h"
#ifdef HAVOKFX_ENABLED
	#include "dxC_havokfx.h"
#endif	
#include "dxC_query.h"
#include "dxC_state.h"
#include "dxC_state_defaults.h"
#include "dxC_2d.h"
#include "dxC_resource.h"
#include "dxC_buffer.h"
#include "dxC_hdrange.h"
#include "dx9_settings.h"

#include "performance.h"
#include "e_portal.h"
#include "e_renderlist.h"
#include "e_primitive.h"
#include "e_model.h"
#include "e_granny_rigid.h"
#include "e_frustum.h"
#include "e_main.h"
#include "e_particle.h"
#include "dxC_viewer.h"
#include "dxC_meshlist.h"
#include "dxC_movie.h"
#include "dxC_pixo.h"

#include "e_compatdb.h"		// CHB 2006.12.26
#include "e_report.h"		// CHB 2007.01.18
#include "e_optionstate.h"	// CHB 2007.02.28
#include "e_screen.h"		// CHB 2007.07.26
#include "e_gamma.h"		// CHB 2007.10.02

#include "memorymanager_i.h"

using namespace FSCommon;

#define DX10_RUNNING_PERFHUD

//CC@HAVOK
#ifdef ENGINE_TARGET_DX10
#include "dx10_ParticleSimulateGPU.h"
#endif

using namespace FSSE;

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
SPD3DCDEVICE				gpDevice					= NULL;
//SPD3DCSWAPCHAIN				gpSwapChain					= NULL;
BOOL						gbDebugValidateDevice		= FALSE;

D3DPRESENT_PARAMETERS		gtD3DPPs[ MAX_SWAP_CHAINS ];
DXCSWAPCHAIN				gtSwapChains[ MAX_SWAP_CHAINS ];

int							gnCurrentSwapChainIndex		= DEFAULT_SWAP_CHAIN;

//-------------------------------------------------------------------------------------------------
// FORWARD DECLARATIONS
//-------------------------------------------------------------------------------------------------

static PRESULT sD3DRelease();
static PRESULT sDeviceRelease();

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

#ifdef ENGINE_TARGET_DX10
DXCADAPTER s_pAdapter = NULL;
#endif

DXCADAPTER dxC_GetAdapter() 
{
#ifdef ENGINE_TARGET_DX9
	return e_OptionState_GetActive()->get_nAdapterOrdinal();
#endif
#ifdef ENGINE_TARGET_DX10
	return s_pAdapter;
#endif
}

#ifdef ENGINE_TARGET_DX10
void dxC_SetAdapter(DXCADAPTER adapter)
{
#ifdef ENGINE_TARGET_DX9
	#ifdef _DEBUG
	ASSERT(false);
	pState->set_nAdapterOrdinal(adapter);
	#else
	REF(adapter);
	#endif
#endif
#ifdef ENGINE_TARGET_DX10
	s_pAdapter = adapter;
#endif
}
#endif

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_DebugValidateDevice()
{
	if ( gbDebugValidateDevice )
	{
		if ( dxC_GetDevice() )
		{
			DWORD dwPasses = 0;
			DX9_BLOCK( V_RETURN( dxC_GetDevice()->ValidateDevice( &dwPasses ) ); )
			DX10_BLOCK( ASSERTX_RETVAL( 0, E_NOTIMPL, "KMNV TODO: Validate device!") ); 
		}
	}
	return S_OK;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void dxC_SetD3DPresentation(const D3DPRESENT_PARAMETERS & tPP, int nIndex /*= 0*/ )
{
	ASSERT_RETURN( IS_VALID_INDEX( nIndex, MAX_SWAP_CHAINS ) );
	gtD3DPPs[ nIndex ] = tPP;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

DXC_FORMAT dxC_GetBackbufferRenderTargetFormat( const OptionState * pOverrideState /*= NULL*/ )
{
	if ( ! pOverrideState )
		pOverrideState = e_OptionState_GetActive();
	DXC_FORMAT BackBufferFormat = pOverrideState->get_nFrameBufferColorFormat();

	return BackBufferFormat;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

DXC_FORMAT dxC_GetBackbufferAdapterFormat( const OptionState * pOverrideState /*= NULL*/ )
{
	if ( ! pOverrideState )
		pOverrideState = e_OptionState_GetActive();
	DXC_FORMAT AdapterFormat = pOverrideState->get_nAdapterColorFormat();

#ifdef ENGINE_TARGET_DX9
	{
		// In fullscreen, we cannot do implicit backbuffer color conversion.  Use the X-alpha version if applicable.
		if ( dx9_IsEquivalentTextureFormat( AdapterFormat, D3DFMT_X8R8G8B8 ) )
			AdapterFormat = D3DFMT_X8R8G8B8;
		if ( dx9_IsEquivalentTextureFormat( AdapterFormat, D3DFMT_X1R5G5B5 ) )
			AdapterFormat = D3DFMT_X1R5G5B5;
	}
#endif

	return AdapterFormat;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#ifdef ENGINE_TARGET_DX9
static
PRESULT dx9_CreateDevice(const OptionState * pState, HWND hWnd, unsigned nBehaviorFlags, IDirect3DDevice9 * * pDeviceInterfaceOut)
{
	if ( e_IsNoRender() )
		return S_FALSE;
	ASSERT_RETFAIL( ! dxC_IsPixomaticActive() );

	// In fillpak mode, force the re-creation of the Direct3D9 object just prior to device creation.
	//if ( e_IsNoRender() )
	//{
	//	LPDIRECT3D9 pD3D9 = dx9_GetD3D( TRUE );
	//	REF( pD3D9 );
	//	ASSERTV_RETFAIL( pD3D9, "FATAL ERROR: Fillpak mode: tried to force recreation of Direct3D9 object but failed!" );
	//}

	D3DPRESENT_PARAMETERS tPresentationParameters;
	dxC_StateToPresentation(pState, tPresentationParameters);
	tPresentationParameters.hDeviceWindow = hWnd;

	PRESULT nResult = dx9_GetD3D()->CreateDevice(
		pState->get_nAdapterOrdinal(),
		pState->get_nDeviceDriverType(),
		hWnd,
		nBehaviorFlags,
		&tPresentationParameters,
		pDeviceInterfaceOut
	);

	if (SUCCEEDED(nResult))
	{
		dxC_SetD3DPresentation(tPresentationParameters);
	}

#if ISVERSION(DEVELOPMENT)
	if ( FAILED(nResult) )
	{
		// dump creation stats
		LogMessage( ERROR_LOG,
			"\n\n\n### Device Creation debug:\n"
			"%30s: %d\n"
			"%30s: %d\n"
			"%30s: %d\n"
			"%30s: %d\n"
			"%30s: %d\n"
			"%30s: %d\n"
			"%30s: %d\n"
			"%30s: %d\n"
			"%30s: %d\n"
			"%30s: %d\n"
			"%30s: %d\n"
			"%30s: %d\n"
			"%30s: %d\n"
			"%30s: %d\n"
			"%30s: %d\n"
			"%30s: %d\n"
			"%30s: %d\n"
			"%30s: %d\n"
			"%30s: %d\n"
			"\n\n"
			,
			"AdapterOrd",					pState->get_nAdapterOrdinal(),
			"DeviceType",					pState->get_nDeviceDriverType(),
			"HWND",							hWnd,
			"nBehaviorFlags",				nBehaviorFlags,
			"BackBufferWidth",				tPresentationParameters.BackBufferWidth,
			"BackBufferHeight",				tPresentationParameters.BackBufferHeight,
			"BackBufferFormat",				tPresentationParameters.BackBufferFormat,
			"BackBufferCount",				tPresentationParameters.BackBufferCount,
			"MultiSampleType",				tPresentationParameters.MultiSampleType,
			"MultiSampleQuality",			tPresentationParameters.MultiSampleQuality,
			"SwapEffect",					tPresentationParameters.SwapEffect,
			"hDeviceWindow",				tPresentationParameters.hDeviceWindow,
			"Windowed",						tPresentationParameters.Windowed,
			"EnableAutoDepthStencil",		tPresentationParameters.EnableAutoDepthStencil,
			"AutoDepthStencilFormat",		tPresentationParameters.AutoDepthStencilFormat,
			"Flags",						tPresentationParameters.Flags,
			"FullScreen_RefreshRateInHz",	tPresentationParameters.FullScreen_RefreshRateInHz,
			"PresentationInterval",			tPresentationParameters.PresentationInterval,
			"IsNoRender",					e_IsNoRender()
			);

		if ( e_IsNoRender() )
		{
			ASSERT_MSG( "Device creation failed!  Parameters dumped to error log." );
		}
	}
#endif // DEVELOPMENT

	return nResult;
}
#endif
#ifdef ENGINE_TARGET_DX10
PRESULT dx10_CreateDevice( HWND hWnd, D3D10_DRIVER_TYPE driverType )
{				
	UINT nBehaviorFlags = 
#ifdef DX10_DEBUG_DEVICE
		D3D10_CREATE_DEVICE_DEBUG;
#ifdef DX10_DEBUG_REF_LAYER
		nBehaviorFlags |= D3D10_CREATE_DEVICE_SWITCH_TO_REF;
#endif
#else
		0;
#endif

	if( gpDevice )
		return S_OK; //Device was already created

	PRESULT nResult = D3D10CreateDevice(
		dxC_GetAdapter(),
		driverType,
		NULL,
		nBehaviorFlags,
		D3D10_SDK_VERSION,
		&gpDevice.p
	);

	return nResult;
}
#endif

void e_SwitchToRef()
{
#if defined( DX10_DEBUG_DEVICE ) && defined( DX10_DEBUG_REF_LAYER ) //Only supported in Dx10 for now
	extern ID3D10SwitchToRef* g_pD3D10SwitchToRef;
	if( g_pD3D10SwitchToRef )
	{
		g_pD3D10SwitchToRef->SetUseRef( !g_pD3D10SwitchToRef->GetUseRef() );
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

// CHB 2007.03.02 - Presently only two varieties of flags are used.
#ifdef ENGINE_TARGET_DX9
const DWORD s_CreateDeviceFlags_SWVP = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
const DWORD s_CreateDeviceFlags_HWVP = D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE;
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sLoopDeviceCreate( PRESULT & pr, HWND hWnd, const OptionState * pState )
{
	//
	unsigned nAdapter = pState->get_nAdapterOrdinal();
	D3DC_DRIVER_TYPE nDriverType = pState->get_nDeviceDriverType();

#ifdef ENGINE_TARGET_DX9
	D3DC_DRIVER_TYPE nSafeCallDriverType = dx9_GetDeviceType();
	bool bSupportsHWVP = false;
	{
		D3DCAPS9 Caps;
		V_DO_SUCCEEDED(dx9_GetD3D()->GetDeviceCaps(nAdapter, nSafeCallDriverType, &Caps))
		{
			bSupportsHWVP = (Caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) != 0;
		}
	}

	if ( e_IsNoRender() )
		bSupportsHWVP = FALSE;

	unsigned nBehaviorFlags = bSupportsHWVP ? s_CreateDeviceFlags_HWVP : s_CreateDeviceFlags_SWVP;
	#ifdef _D3DSHADER_DEBUG
	nBehaviorFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
	#endif
	#ifdef DXDUMP_ENABLE
	nBehaviorFlags &= ~( D3DCREATE_PUREDEVICE );
	#endif
#endif

	// Create the D3D device
	// If for some reason the device cannot be created (e.g. Windows XP Media
	// Center is still releasing the device), then keep trying to create the
	// device for a specific amount of time.  (e.g. 15 seconds)
	UINT count = 0;
	do
	{
		// CHB 2006.12.29 - Get rid of mandatory 1/2 second startup delay.
		if (FAILED(pr))
		{
			Sleep(500);
		}
		trace("DeviceCreate:  Attempting to create device... Adapter %d, Type %d, HWnd %X\n", nAdapter, nDriverType, hWnd);
		LogMessage( LOG_FILE, "Creating device: Adapter %d, Type %d\n", nAdapter, nDriverType );
#ifdef ENGINE_TARGET_DX9
		// CHB 2007.02.05 - Oops.
		//PRESULT pr;
		V_SAVE( pr, dx9_CreateDevice( pState, hWnd, nBehaviorFlags, &gpDevice) );
		trace("DeviceCreate:  result 0x%08x, %s\n", pr, e_GetResultString(pr) );
		if ( FAILED(pr) )
		{
			LogMessage( ERROR_LOG, "\nDeviceCreate FAILED:  result 0x%08x, %s\n", pr, e_GetResultString(pr) );
		}

#else	// DX10 we create the main swapchain as the device was created earlier
		V_SAVE( pr, dx10_CreateSwapChain( pState, &gtSwapChains[0].pSwapChain.p ) );
#endif
	} while(FAILED(pr) && ++count < 30);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_GatherGeneralCaps()
{
	// make sure d3d is initialized
	DX9_BLOCK( ASSERT_RETFAIL( dx9_GetD3D() ); )

	V_RETURN(dx9_DetectNonD3DCapabilities() );
	DX9_BLOCK( V_RETURN( dx9_GatherNonDeviceCaps() ); )

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_DeviceCreateMinimal( HWND hWnd )
{
	// make sure d3d is initialized
	DX9_BLOCK( ASSERT_RETFAIL( dx9_GetD3D() ); )


	// Get ready to set up device
	PRESULT pr = S_OK;	// CHB 2006.12.29

	//if ( e_IsFullscreen() )
	//{
	//	D3DDISPLAYMODE d3dMode;
	//	V_RETURN( dxC_GetFullscreenDisplayMode( d3dMode ) );
	//	//e_SetWindowSize(d3dMode.Width, d3dMode.Height);
	//	V_RETURN( dx9_SetFullscreenPresentation( &dxC_GetD3DPP(), &d3dMode ) );
	//} else
	//{
	//ASSERT_RETFAIL( e_GetWindowWidth() && e_GetWindowHeight() );
	CComPtr<OptionState> pState;
	V_RETURN(e_OptionState_CloneActive(&pState));
	pState->SuspendUpdate();
	pState->set_bWindowed(true);
	pState->set_nFrameBufferWidth(100);
	pState->set_nFrameBufferHeight(100);
	pState->ResumeUpdate();

	// couldn't get NULLREF to work here over Remote Desktop  :/

	//D3DPRESENT_PARAMETERS d3dpp;
	//if ( AppCommonGetMinPak() || AppCommonGetFillPakFile() )
	//{
	//	ZeroMemory( &d3dpp, sizeof(D3DPRESENT_PARAMETERS) );
	//	d3dpp.Windowed         = TRUE;
	//	d3dpp.SwapEffect       = D3DSWAPEFFECT_DISCARD;
	//	d3dpp.BackBufferFormat = dxC_GetDefaultDisplayFormat( TRUE );

	//	pPP = &d3dpp;

	//	cParams.DeviceDriverType	= D3DDEVTYPE_NULLREF;
	//	cParams.Flags				= D3DCREATE_SOFTWARE_VERTEXPROCESSING;

	//	trace( "Device create for pak: fmt %d, hwnd %d\n", pPP->BackBufferFormat, hWnd );
	//}


	// Create the device
	sLoopDeviceCreate( pr, hWnd, pState );

	if ( FAILED( pr ) )
	{
		LogMessage( ERROR_LOG, "Create Device Error:" );
		LogMessage( ERROR_LOG, DXGetErrorString( pr ) );
		LogMessage( ERROR_LOG, "\n" );

		return E_FAIL;
	}

	ASSERT( dxC_GetDevice() );
	trace( "Minimal device created!\n" );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_DeviceInit( void * pData )
{
	if ( e_IsNoRender() )
		return S_FALSE;

	e_ReportInit();	// CHB 2007.01.18

	HWND hWnd = (HWND)pData;
	ASSERT_RETFAIL( hWnd );

	TIMER_START("D3D Device Init")

#if defined(ENGINE_TARGET_DX9)
	// make sure nothing is initalized
	ASSERT_RETFAIL( ! dxC_GetDevice() );

	// ... except the d3d object if running DX9
	ASSERT_RETFAIL( dx9_GetD3D() );
#elif defined( ENGINE_TARGET_DX10 )
	dx10_Init( (OptionState*)e_OptionState_GetActive() );
#endif
	// detect avail texture memory, etc.
	//KMNV TODO: Need to make API agnostic
	V_RETURN(dx9_DetectNonD3DCapabilities() );
	DX9_BLOCK( V_RETURN( dx9_GatherNonDeviceCaps() ); )

	V_DO_FAILED( dxC_DeviceCreate( hWnd ) )
	{
		ASSERTV_RETFAIL(0, "Failed to initialize D3D Device!" );
	}

	if ( ! dxC_IsPixomaticActive() )
	{
		ASSERTV_RETFAIL( dxC_GetDevice(), "Failed to initialize D3D Device, though no error was returned!" );
	}

	DX9_BLOCK( V( dx9_SaveAutoDepthStencil( DEFAULT_SWAP_CHAIN ) ); )

	//Take care of any further device related initialization
	V_RETURN( dxC_InitializeDeviceObjects() );


#ifndef DX10_GET_RUNNING_HACK
	// to smooth out hitching, if not vsync, do a software sync
	//if ( dxC_GetD3DPP().PresentationInterval != D3DPRESENT_INTERVAL_IMMEDIATE )
	//{
	//	e_SetRenderFlag( RENDER_FLAG_SYNC_TO_GPU, TRUE );
	//}
#endif

	e_CompatDB_Init();	// CHB 2006.12.26

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT e_DeviceRelease()
{
#ifdef USE_PIXO
	if ( dxC_IsPixomaticActive() )
	{
		return dxC_PixoShutdown();
	}
#endif

	gpDevice.Release();

	for ( int i = 0; i < MAX_SWAP_CHAINS; ++i )
	{
		if ( ! gtSwapChains[i].pSwapChain )
			continue;
		DX10_BLOCK( gtSwapChains[i].pSwapChain->SetFullscreenState( false, NULL ); ) //Need to switch out of fullscreen first
		gtSwapChains[i].pSwapChain = NULL;
	}

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static PRESULT sInitRenderSettings()
{
	//// HDR settings
	//e_SetHDRMode( (HDR_MODE)pSettings->nHDRMode );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_GrabSwapChainAndBackbuffer( int nSwapChainIndex )
{
	PRESULT pr = S_OK;

#ifdef ENGINE_TARGET_DX9

	if ( ! dxC_IsPixomaticActive() )
	{
		//Grab the main swapchain
		if( ! gtSwapChains[ nSwapChainIndex ].pSwapChain )
		{
			V_SAVE( pr, dxC_GetDevice()->GetSwapChain( 0, &gtSwapChains[ nSwapChainIndex ].pSwapChain ));
		}
	}

	//Grab the backbuffer
	RENDER_TARGET_INDEX nRT = dxC_GetSwapChainRenderTargetIndex( nSwapChainIndex, SWAP_RT_FINAL_BACKBUFFER );
	if ( nRT == RENDER_TARGET_INVALID )
	{
		// Add the RT desc for this

		int nWidth, nHeight;
		DXC_FORMAT tFormat;
		DWORD dwMSType;
		DWORD dwMSQuality;

		if ( ! dxC_IsPixomaticActive() )
		{
			const D3DPRESENT_PARAMETERS & tPP = dxC_GetD3DPP( nSwapChainIndex );
			DXCSWAPCHAIN * ptSC = dxC_GetDXCSwapChain( nSwapChainIndex );

			nWidth = tPP.BackBufferWidth;
			nHeight = tPP.BackBufferHeight;
			tFormat = (DXC_FORMAT)tPP.BackBufferFormat;
			dwMSType = tPP.MultiSampleType;
			dwMSQuality = tPP.MultiSampleQuality;
		}
		else
		{
			dxC_PixoGetBufferDimensions( nWidth, nHeight );
			dxC_PixoGetBufferFormat( tFormat );
			dwMSType = 0;
			dwMSQuality = 0;
		}

		RENDER_TARGET_INDEX nRTIndex = (RENDER_TARGET_INDEX)dxC_AddRenderTargetDesc(
			TRUE,
			nSwapChainIndex,
			SWAP_RT_FINAL_BACKBUFFER,
			nWidth,
			nHeight,
			D3DC_USAGE_2DTEX_RT,
			tFormat,
			-1,
			FALSE,
			1,
			dwMSType,
			dwMSQuality );

		if ( ! dxC_IsPixomaticActive() )
		{
			DXCSWAPCHAIN * ptSC = dxC_GetDXCSwapChain( nSwapChainIndex );
			ptSC->nRenderTargets[ SWAP_RT_FINAL_BACKBUFFER ] = nRTIndex;
		}

		nRT = dxC_GetSwapChainRenderTargetIndex( nSwapChainIndex, SWAP_RT_FINAL_BACKBUFFER );
		ASSERT( nRT != RENDER_TARGET_INVALID );
	}

	if ( ! dxC_IsPixomaticActive() )
	{
		LPD3DCSWAPCHAIN pSwapChain = dxC_GetD3DSwapChain( nSwapChainIndex );
		ASSERT_RETFAIL( pSwapChain );
		//V_SAVE( pr, dxC_GetDevice()->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &dxC_RTSurfaceGet( nRT ) ) );
		V_SAVE( pr, pSwapChain->GetBackBuffer( 0, D3DBACKBUFFER_TYPE_MONO, &dxC_RTSurfaceGet( nRT ) ) );
	}
#endif // DX9

	return pr;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dxC_DeviceCreate( HWND hWnd )
{
	// Get ready to set up device
	PRESULT pr = S_OK;	// CHB 2006.12.29

	//Set app hwnd
	AppCommonSetHWnd( hWnd );


	if ( e_IsNoRender() )
		return S_FALSE;



	// Init render settings based on engine settings
	V_RETURN( sInitRenderSettings() );

	// Create the D3D device
	e_SetChangingDevice(true);	// CHB 2007.08.23
	if ( dxC_IsPixomaticActive() )
	{
		V_SAVE( pr, dxC_PixoInitialize( AppCommonGetHWnd() ) );
	}
	else
	{
		sLoopDeviceCreate( pr, hWnd, e_OptionState_GetActive() );
	}
	e_SetChangingDevice(false);	// CHB 2007.08.23

	if ( FAILED( pr ) )
	{
		LogMessage( ERROR_LOG, "Create Device Error:" );
		LogMessage( ERROR_LOG, DXGetErrorString( pr ) );
		LogMessage( ERROR_LOG, "\n" );

        return FALSE;
	}

	ASSERTX_RETFAIL( dxC_IsPixomaticActive() || dxC_GetDevice(), "Device create appeared to succeed but device is NULL!" );

	trace( "Device Created!\n" );

	V( pr );

	if ( e_IsNoRender() )
	{
		return S_OK;
	}

#ifdef ENGINE_TARGET_DX9
	V( dxC_GrabSwapChainAndBackbuffer( DEFAULT_SWAP_CHAIN ) );

	return S_OK;
#endif
#ifdef ENGINE_TARGET_DX10
#ifndef DX10_RUNNING_PERFHUD
	extern ID3D10InfoQueue* g_pD3D10InfoQue;
	if( SUCCEEDED( dxC_GetDevice()->QueryInterface( __uuidof( ID3D10InfoQueue ), reinterpret_cast<void**>(&g_pD3D10InfoQue) ) ) )
	{//This is to filter out state setting spam

		D3D10_INFO_QUEUE_FILTER filtList;
		ZeroMemory( &filtList, sizeof( D3D10_INFO_QUEUE_FILTER ) );
		D3D10_MESSAGE_ID idList[3] =  { D3D10_MESSAGE_ID_DEVICE_DRAW_SHADERRESOURCEVIEW_NOT_SET, D3D10_MESSAGE_ID_DEVICE_PSSETSHADERRESOURCES_HAZARD, D3D10_MESSAGE_ID_DEVICE_OMSETRENDERTARGETS_HAZARD }; 
		filtList.DenyList.NumIDs = 3;
		filtList.DenyList.pIDList = idList;
		g_pD3D10InfoQue->PushStorageFilter( &filtList );
	}
#if defined( DX10_DEBUG_DEVICE )
#if defined( DX10_DEBUG_DEVICE_PER_API_CALL )
	extern ID3D10Debug* g_pD3D10Debug;
	V( dxC_GetDevice()->QueryInterface( __uuidof( ID3D10Debug ), reinterpret_cast< void** >( &g_pD3D10Debug ) ) );
	V( g_pD3D10Debug->SetFeatureMask( D3D10_DEBUG_FEATURE_FINISH_PER_RENDER_OP ) );
#endif
#endif
#endif

#if defined( DX10_DEBUG_DEVICE ) && defined( DX10_DEBUG_REF_LAYER )
	extern ID3D10SwitchToRef* g_pD3D10SwitchToRef;
	V( dxC_GetDevice()->QueryInterface( __uuidof( ID3D10SwitchToRef ), reinterpret_cast< void** >( &g_pD3D10SwitchToRef ) ) );
#endif

	dx9_SetStateManager(); //KMNV We need our statemanager here

    return dx10_GrabBBPointers();
#endif
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
#ifdef ENGINE_TARGET_DX10
static inline void sdx10_ClearBackBufferPrimaryZ( UINT ClearFlags, float* ClearColor = NULL, float depth = 1.0f, UINT8 stencil = 0 )
{
	ID3D10RenderTargetView* pRT;
	ID3D10DepthStencilView* pDS;
	V( dxC_GetRenderTargetDepthStencil0( &pRT, &pDS ) );

	if(ClearFlags & D3DCLEAR_TARGET && pRT )
		dxC_GetDevice()->ClearRenderTargetView( pRT, ClearColor );
	if(ClearFlags & (D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL) && pDS )
		dxC_GetDevice()->ClearDepthStencilView( pDS,  ClearFlags, depth, stencil);
}
#endif

static inline void sClearBackBufferPrimaryZ( UINT* pClearFlags )
{
	ASSERT( pClearFlags );
	// NOTE: This always clears stencil along with Z when the depth format specifies stencil.
	// Until this is changed, it is impossible to clear Z and stencil independently of each other.
	if ( *pClearFlags & D3DCLEAR_ZBUFFER )
	{
#ifdef ENGINE_TARGET_DX9
		DXC_FORMAT eFormat = dxC_GetCurrentDepthTargetFormat();
		if ( eFormat == D3DFMT_D24S8 )  
			*pClearFlags |= D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL;
		else if ( eFormat == D3DFMT_D16 || eFormat == D3DFMT_D24X8 )
			*pClearFlags |= D3DCLEAR_ZBUFFER;
		else
			ASSERT_MSG( "Unsupported depth format!" );
#elif defined( ENGINE_TARGET_DX10 )
		*pClearFlags |= D3DCLEAR_STENCIL; //Think this should be fine for dx9 as well...
#endif

	}

	// if we didn't render to it, don't clear it now
	if ( *pClearFlags & D3DCLEAR_ZBUFFER || *pClearFlags & D3DCLEAR_STENCIL )
	{
		BOOL bDirty = TRUE;
		V( dxC_DSGetDirty( dxC_GetCurrentDepthTarget(), bDirty ) );
		if ( ! bDirty )
			*pClearFlags &= ~(D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL);
	}

	if ( *pClearFlags & D3DCLEAR_TARGET )
	{
		BOOL bDirty = TRUE;
		V( dxC_RTGetDirty( dxC_GetCurrentRenderTarget(), bDirty ) );
		if ( ! bDirty )
			*pClearFlags &= ~(D3DCLEAR_TARGET);
	}
}

PRESULT dxC_ClearFPBackBufferPrimaryZ( UINT ClearFlags, float* ClearColor, float depth, UINT8 stencil)
{
#ifdef ENGINE_TARGET_DX10
	sClearBackBufferPrimaryZ( &ClearFlags );
	if ( 0 == ( ClearFlags & (D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL) ) )
	{
		return S_FALSE;
	}
	sdx10_ClearBackBufferPrimaryZ( ClearFlags, ClearColor, depth, stencil );

	return S_OK;
#else
	ASSERTX_RETFAIL( 0, "Floating point color clears are only supported in Dx10" );
#endif
}

PRESULT dxC_ClearBackBufferPrimaryZ(UINT ClearFlags, D3DCOLOR ClearColor, float depth, UINT8 stencil)
{
	if ( dxC_IsPixomaticActive() )
	{
		return dxC_PixoClearBuffers( ClearFlags, ClearColor, depth, (UINT32)stencil );
	}

	sClearBackBufferPrimaryZ( &ClearFlags );
	if ( 0 == ( ClearFlags & (D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL) ) )
	{
		return S_FALSE;
	}

	dxC_RTMarkClearToken( dxC_GetCurrentRenderTarget( 0 ) );

#ifdef ENGINE_TARGET_DX9
		V_RETURN( dxC_GetDevice()->Clear( 0, NULL, ClearFlags, ClearColor, depth, stencil) );
#elif defined(ENGINE_TARGET_DX10)
		float color[4] = ARGB_DWORD_TO_RGBA_FLOAT4( ClearColor );
		sdx10_ClearBackBufferPrimaryZ( ClearFlags, color, depth, stencil );
#endif
	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

static
void e_ApplyWindowSizeOverrides(OptionState * pState)
{
	ENGINE_OVERRIDES_DEFINITION * pOverrides = e_GetEngineOverridesDefinition();
	if (pOverrides == 0)
	{
		return;
	}

	if (pState->get_bWindowed())
	{
		int nWidth = pState->get_nFrameBufferWidth();
		int nHeight = pState->get_nFrameBufferHeight();

		if ( pOverrides->dwFlags & GFX_FLAG_RANDOM_WINDOWED_SIZE )
		{
			// select a random window size conforming to the following ranges
			E_SCREEN_DISPLAY_MODE tDesc;
			dxC_GetD3DDisplayMode(tDesc);	// CHB !!! - should be desktop mode
			int nMinX = RANDOM_WINDOW_RES_MIN_X;
			int nMinY = RANDOM_WINDOW_RES_MIN_Y;
			int nMaxX = min( tDesc.Width,  RANDOM_WINDOW_RES_MAX_X );
			int nMaxY = min( tDesc.Height, RANDOM_WINDOW_RES_MAX_Y );
			float fMinAspect = RANDOM_WINDOW_RES_MIN_ASPECT;
			float fMaxAspect = RANDOM_WINDOW_RES_MAX_ASPECT;
			int nX, nY;

			BOUNDED_WHILE( 1, 1000000 )
			{
				nX = RandGetNum( e_GetEngineOnlyRand(), nMinX, nMaxX );
				nY = RandGetNum( e_GetEngineOnlyRand(), nMinY, nMaxY );
				float fAspect = (float)nX / nY;
				if ( fAspect >= fMinAspect && fAspect <= fMaxAspect )
					break;
			}
			nWidth  = nX;
			nHeight = nY;
		}
		else if ((pOverrides->dwWindowedResWidth > 0) && (pOverrides->dwWindowedResWidth > 0))
		{
			nWidth  = pOverrides->dwWindowedResWidth;
			nHeight = pOverrides->dwWindowedResHeight;
		}
		nWidth  = MAX( nWidth,  SETTINGS_MIN_RES_WIDTH  );
		nHeight = MAX( nHeight, SETTINGS_MIN_RES_HEIGHT );

		pState->SuspendUpdate();
		pState->set_nFrameBufferWidth(nWidth);
		pState->set_nFrameBufferHeight(nHeight);
		pState->ResumeUpdate();
	}
	else
	{
		if ( pOverrides->dwFlags & GFX_FLAG_RANDOM_FULLSCREEN_SIZE )
		{
			DXC_FORMAT tFormat = dxC_GetBackbufferAdapterFormat( pState );
			UINT nCount = dxC_GetDisplayModeCount( tFormat );
			BOUNDED_WHILE( 1, 1000000 )
			{
				E_SCREEN_DISPLAY_MODE tMode;
				UINT nIndex = (UINT)RandGetNum( e_GetEngineOnlyRand(), 0, nCount - 1 );
				V_CONTINUE( dxC_EnumDisplayModes( tFormat, nIndex, tMode, TRUE ) )
				if ( tMode.Width >= SETTINGS_MIN_RES_WIDTH && tMode.Height >= SETTINGS_MIN_RES_HEIGHT )
				{
					e_Screen_DisplayModeToState(pState, tMode);
					//trace( "### %s:%d -- Setting backbuffer color format to %d -- %s\n", __FILE__, __LINE__, tMode.Format, __FUNCSIG__ );
					break;
				}
			}
		}
		if (pOverrides->dwFullscreenColorFormat != 0)
		{
			pState->set_nFrameBufferColorFormat(static_cast<DXC_FORMAT>(pOverrides->dwFullscreenColorFormat));
			//trace( "### %s:%d -- Setting backbuffer color format to %d -- %s\n", __FILE__, __LINE__, pOverrides->dwFullscreenColorFormat, __FUNCSIG__ );
		}
	}
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

// CHB 2007.07.26
void e_AdjustWindowSizeToFitDesktop(int & nWindowWidth, int & nWindowHeight)
{
	// Build mode struct.
	E_SCREEN_DISPLAY_MODE Mode;
	Mode.Width = nWindowWidth;
	Mode.Height = nWindowHeight;

	// Is this size already valid?
	if (e_Screen_IsModeValidForWindowedDisplay(Mode))
	{
		return;
	}

	const E_SCREEN_DISPLAY_MODE_VECTOR vModes = e_Screen_EnumerateDisplayModes();

	// Look for the largest size that fits the current desktop
	// while trying to maintain the current aspect ratio if possible.

	E_SCREEN_DISPLAY_MODE BestMode;
	E_SCREEN_DISPLAY_MODE BestModeAndAR;
	E_SCREEN_DISPLAY_MODE SmallestMode = Mode;

#define METRIC(m) ((m).Width * (m).Height)

	for (E_SCREEN_DISPLAY_MODE_VECTOR::const_iterator i = vModes.begin(); i != vModes.end(); ++i)
	{
		if (METRIC(*i) < METRIC(SmallestMode))
		{
			SmallestMode = *i;
		}

		if (e_Screen_IsModeValidForWindowedDisplay(*i))
		{
			// Does this mode have an identical aspect ratio?
			if (i->Width * Mode.Height == i->Height * Mode.Width)
			{
				// Same aspect ratio.
				if (METRIC(*i) > METRIC(BestModeAndAR))
				{
					BestModeAndAR = *i;
				}
			}
			else
			{
				// Other aspect ratio.
				if (METRIC(*i) > METRIC(BestMode))
				{
					BestMode = *i;
				}
			}
		}
	}

#undef METRIC

	// Choose the one with the matching aspect ratio if possible.
	E_SCREEN_DISPLAY_MODE NewMode = SmallestMode;
	if ((BestMode.Width > 0) && (BestMode.Height > 0))
	{
		NewMode = BestMode;
	}
	if ((BestModeAndAR.Width > 0) && (BestModeAndAR.Height > 0))
	{
		NewMode = BestModeAndAR;
	}

	//
	nWindowWidth = NewMode.Width;
	nWindowHeight = NewMode.Height;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

// CHB !!!
PRESULT e_InitWindowSize(OptionState * pState)
{
	// set the default window size

	E_SCREEN_DISPLAY_MODE tMode;
	e_Screen_DisplayModeFromState(pState, tMode);
	int & nWindowWidth = tMode.Width;	//!!! - can do away with this
	int & nWindowHeight = tMode.Height;	//!!! - cad do away with this
	bool bFullscreen = !pState->get_bWindowed();

	// don't do fullscreen in minpak or fillpak
	if ( e_IsNoRender() )
	{
		//nWindowWidth  = DEFAULT_WINDOW_WIDTH;
		//nWindowHeight = DEFAULT_WINDOW_HEIGHT;
		bFullscreen   = false;
	}

	// special case for Hammer
	if ( c_GetToolMode() )
	{
		const GLOBAL_DEFINITION* globalDef = DefinitionGetGlobal();
		nWindowWidth = globalDef->nDefWidth;
		nWindowHeight = globalDef->nDefHeight;
		bFullscreen = false;

		if ( nWindowWidth <= 0 || nWindowHeight <= 0 )
		{
			nWindowWidth = DEFAULT_WINDOW_WIDTH;
			nWindowHeight = DEFAULT_WINDOW_HEIGHT;
		}

		e_SetFarClipPlane( 200.0f );
		e_SetFogDistances( 0.01f, 200.0f );
	}

	if (!bFullscreen)
	{
		e_AdjustWindowSizeToFitDesktop(nWindowWidth, nWindowHeight);	// CHB 2007.07.26
		tMode.SetWindowed();
	}
	tMode.Width = nWindowWidth;
	tMode.Height = nWindowHeight;

	pState->SuspendUpdate();
	e_Screen_DisplayModeToState(pState, tMode);
	if (!e_IsNoRender())
	{
		e_ApplyWindowSizeOverrides(pState);
	}
	pState->ResumeUpdate();

	// CHB 2007.03.04 - Application could add a Commit() event handler to do this.
	AppCommonGetWindowWidthRef()  = pState->get_nFrameBufferWidth();
	AppCommonGetWindowHeightRef() = pState->get_nFrameBufferHeight();

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dxC_GetD3DDisplayMode( E_SCREEN_DISPLAY_MODE & DisplayMode )
{
#if defined(ENGINE_TARGET_DX9)
	D3DDISPLAYMODE Desc;
	V_RETURN( dx9_GetD3D()->GetAdapterDisplayMode( dxC_GetAdapter(), &Desc ) );
	DisplayMode = dxC_ConvertNativeDisplayMode(Desc);
#elif defined(ENGINE_TARGET_DX10)
	//dx10_DXGIGetOutput()->GetDisplayMode(pDisplayMode);
	//	KMNV TODO IDXGIOutput no longer uses this call, need to update
	//Actually we don't need to do anything since the current display mode doesn't matter
	//So just grab whatever
	// CHB 2007.09.06 - This is rather crap. There doesn't appear
	// to be a way to get a description (primarily, the format)
	// of an output without having full screen exclusive control
	// over it.
	DXGI_MODE_DESC Desc;
	if (dxC_GetD3DSwapChain() != 0)
	{
		DXGI_SWAP_CHAIN_DESC ChainDesc;
		V_RETURN(dxC_GetD3DSwapChain()->GetDesc(&ChainDesc));
		Desc = ChainDesc.BufferDesc;
	}
	else
	{
		Desc.Width = ::GetSystemMetrics(SM_CXSCREEN);
		Desc.Height = ::GetSystemMetrics(SM_CYSCREEN);
		Desc.RefreshRate.Numerator = SETTINGS_DEF_FULLSCREEN_REFRESH_RATE_NUMERATOR;
		Desc.RefreshRate.Denominator = SETTINGS_DEF_FULLSCREEN_REFRESH_RATE_DENOMINATOR;
		Desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		Desc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_PROGRESSIVE;
		Desc.Scaling = DXGI_MODE_SCALING_CENTERED;
	}
	DisplayMode = dxC_ConvertNativeDisplayMode(Desc);
	// CHB 2007.09.06 - This resulted in an uninitialized structure
	// being used.
//	V_RETURN( dxC_EnumDisplayModes( D3DFMT_UNKNOWN, 0, pDisplayMode ) );
#endif
	return S_OK;
}

PRESULT dxC_CreateRTV( LPD3DCTEXTURE2D pTexture, LPD3DCRENDERTARGETVIEW* ppRTV, UINT nLevel )
{
#if defined(ENGINE_TARGET_DX9)
	V_RETURN( pTexture->GetSurfaceLevel( 0, ppRTV ) );
#elif defined(ENGINE_TARGET_DX10)
	D3D10_TEXTURE2D_DESC Desc;
	pTexture->GetDesc( &Desc );

	D3D10_RENDER_TARGET_VIEW_DESC RTVDesc;
	RTVDesc.Format = dx10_ResourceFormatToRTVFormat( Desc.Format );

	if(Desc.ArraySize > 1 && Desc.SampleDesc.Count > 1)
	{
		RTVDesc.ViewDimension = D3D10_RTV_DIMENSION_TEXTURE2DMSARRAY;
		RTVDesc.Texture2DMSArray.ArraySize = Desc.ArraySize;
		RTVDesc.Texture2DMSArray.FirstArraySlice = 0;
	}	
	else if(Desc.SampleDesc.Count > 1)
	{
		RTVDesc.ViewDimension = D3D10_RTV_DIMENSION_TEXTURE2DMS;
	}
	else if(Desc.ArraySize > 1)
	{
		RTVDesc.ViewDimension = D3D10_RTV_DIMENSION_TEXTURE2DARRAY;
		RTVDesc.Texture2DArray.ArraySize = Desc.ArraySize;
		RTVDesc.Texture2DArray.FirstArraySlice = 0;
		RTVDesc.Texture2DArray.MipSlice = nLevel;
	}
	else
	{
		RTVDesc.ViewDimension = D3D10_RTV_DIMENSION_TEXTURE2D;
		RTVDesc.Texture2D.MipSlice = nLevel;
	}

	V_RETURN( dxC_GetDevice()->CreateRenderTargetView( pTexture, &RTVDesc, ppRTV ) );
#endif

	return S_OK;
}

PRESULT dxC_CreateDSV( LPD3DCTEXTURE2D pTexture, LPD3DCDEPTHSTENCILVIEW* ppRTV )
{
#ifdef ENGINE_TARGET_DX9
	V_RETURN( pTexture->GetSurfaceLevel( 0, ppRTV ) );
#elif defined(ENGINE_TARGET_DX10)
	D3D10_TEXTURE2D_DESC Desc;
	pTexture->GetDesc( &Desc );

	D3D10_DEPTH_STENCIL_VIEW_DESC DSVDesc;
	DSVDesc.Format = dx10_ResourceFormatToDSVFormat( Desc.Format );

	if(Desc.ArraySize > 1 && Desc.SampleDesc.Count > 1)
	{
		DSVDesc.ViewDimension = D3D10_DSV_DIMENSION_TEXTURE2DMSARRAY;
		DSVDesc.Texture2DMSArray.ArraySize = Desc.ArraySize;
		DSVDesc.Texture2DMSArray.FirstArraySlice = 0;
	}
	else if(Desc.SampleDesc.Count > 1)
	{
		DSVDesc.ViewDimension = D3D10_DSV_DIMENSION_TEXTURE2DMS;
	}
	else if(Desc.ArraySize > 1)
	{
		DSVDesc.ViewDimension = D3D10_DSV_DIMENSION_TEXTURE2DARRAY;
		DSVDesc.Texture2DArray.ArraySize = Desc.ArraySize;
		DSVDesc.Texture2DArray.FirstArraySlice = 0;
		DSVDesc.Texture2DArray.MipSlice = 0;
	}
	else
	{
		DSVDesc.ViewDimension = D3D10_DSV_DIMENSION_TEXTURE2D;
		DSVDesc.Texture2D.MipSlice = 0;
	}
	V_RETURN( dxC_GetDevice()->CreateDepthStencilView( pTexture, &DSVDesc, ppRTV ) );
#endif

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static PRESULT sD3DRelease()
{
#if defined(ENGINE_TARGET_DX9)
	SPDIRECT3D9 extern gpD3D;
	gpD3D.Release();
#endif

	return S_OK;
}

static PRESULT sDeviceRelease()
{
	gpDevice.Release();
	sD3DRelease();

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PRESULT e_InitializeStorage()
{
	V( dxC_ResourceSystemInit() );
	V( dxC_BuffersInit() ); 
	V( dx9_EffectsInit() );
	V( e_TexturesInit() );
	V( dx9_LightsInit() );
	V( dx9_ShadowsInit() );
	V( dx9_DrawListsInit() );
	V( dx9_DebugInit() );
	V( e_ModelsInit() );
	V( dxC_TargetBuffersInit() );
	V( e_UIInit() );
	V( e_PrimitiveInit() );
	V( e_PortalsInit() );
	V( e_RenderListInit() );
	V( e_ViewersInit() );
	V( dxC_MeshListsInit() );
	V( dxC_MoviesInit() );

	return S_OK;
}

PRESULT dx9_RestoreD3DResources()
{
	if ( e_IsNoRender() )
		return S_FALSE;

	TIMER_START("Restore D3D Resources")

	ASSERT_RETFAIL( dxC_IsPixomaticActive() || dxC_GetDevice() );

	dxC_AddRenderTargetDescs();
	V( dxC_RestoreRenderTargets() );

	if ( ! dxC_IsPixomaticActive() )
	{
		// restore state manager
		dx9_SetStateManager();
		DX9_BLOCK( V( dx9_RestoreD3DStates( TRUE ) ); )

		DEPTH_TARGET_INDEX eDT = dxC_GetSwapChainDepthTargetIndex( dxC_GetCurrentSwapChainIndex(), SWAP_DT_AUTO );
		V( dxC_SetRenderTargetWithDepthStencil( SWAP_RT_BACKBUFFER, eDT ) );

		HDRPipeline::CreateResources();
	}

	V( dxC_QuadVBsCreate() );

	V( dx9_EffectCreateNonMaterialEffects() );

	//dx9_EffectRestoreAll();

	V( e_ParticleSystemsRestore() );

	V( dx9_ScreenDrawRestore() );

	V( dx9_PrimitiveDrawRestore() );

	V( e_LoadGlobalMaterials() );

	//V( e_PortalsRestoreAll() ); 

	// create all of the meshes that we need
	//for ( MESH_DEFINITION_HASH * pHash = dx9_MeshDefinitionGetFirst(); 
	//	pHash; 
	//	pHash = dx9_MeshDefinitionGetNext( pHash ) )
	//{
	//	D3D_MESH_DEFINITION * pMesh = dx9_MeshDefinitionGetFromHash( pHash );
	//	V( dx9_MeshRestore ( pMesh ) );
	//}

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PRESULT dxC_InitializeDeviceObjects()
{
	// save auto depth/stencil surface
	//DX9_BLOCK( V( dx9_SaveAutoDepthStencil( DEFAULT_SWAP_CHAIN ) ); )//KMNV this should be handled during device creation in dx10

	//e_CompatDB_Init();	// CHB 2007.01.02

	// get all engine/platform caps and validate
	V( e_GatherCaps() );
	//e_ValidateEngineCaps();

	ENGINE_OVERRIDES_DEFINITION * pOverrides = e_GetEngineOverridesDefinition();
	// HDR settings
	e_SetHDRMode( pOverrides ? (HDR_MODE)pOverrides->nHDRMode : HDR_MODE_NONE );
	//e_SetHDRMode( HDR_MODE_LINEAR_SHADERS );			// Debug

	if ( ! e_IsNoRender() && ! dxC_IsPixomaticActive() )
	{
		ASSERT( dx9_CapGetValue( DX9CAP_MAX_POINT_SIZE ) >= 1.0f );
	}

	e_UpdateGlobalFlags();

	// CHB 2006.10.12 - This needs to come before dx9_RestoreD3DStates()
	// because the sampler defaults need to be in place before
	// dx9_RestoreD3DStates() calls dx9_ResetSamplerStates().
	V( dx9_DefaultStatesInit() );

	e_GrannyInit ();

	// moved storage inits to separate function

	DX9_BLOCK( V( dx9_VertexDeclarationsRestore() ); ) //In DX10 we create decls (IA objecs) on the fly because we need the technique they're associated with

	// create stat queries
	V( dxC_RestoreStatQueries() );
	V( dx9_ScreenBuffersInit() );

   	V( dx9_RestoreD3DResources() );

#ifdef _DEBUG
	dxC_DumpRenderTargets();	// CHB 2007.01.18
#endif

	DX9_BLOCK( V( dx9_RestoreD3DStates( TRUE ) ); )
	DX10_BLOCK( dx10_RestoreD3DStates(); )


	//HRVERIFY( dx9_GetDevice()->SetDialogBoxMode( TRUE ) );

	V( e_UpdateGamma() );

	e_GetCurrentViewFrustum()->Dirty();
	e_InitElapsedTime();

	if ( gnMainViewerID != INVALID_ID )
	{
		V( e_ViewerRemove( gnMainViewerID ) );
		gnMainViewerID = INVALID_ID;
	}
	V( e_ViewerCreate( gnMainViewerID ) );
	if ( AppIsTugboat() )
	{
		V( e_ViewerSetRenderType( gnMainViewerID, VIEWER_RENDER_MYTHOS ) );
	}
	else
	{
		V( e_ViewerSetRenderType( gnMainViewerID, VIEWER_RENDER_HELLGATE ) );
	}


#ifdef DXDUMP_ENABLE
	initDxDump( dxC_GetDevice() );
#endif

	V( e_TextureBudgetUpdate() );
	V( e_ModelBudgetUpdate() );

	V( e_RestoreDefaultTextures() );

#ifdef HAVOKFX_ENABLED
	dxC_HavokFXDeviceInit();
#endif

	//CC@HAVOK
//	DX10_BLOCK ( V( dx10_InitializeGPUMeshParticleEmitter() ); )

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT e_DestroyStorage()
{
	// reverse InitializeStorage() order
	V( dxC_MoviesDestroy() );
	V( dxC_MeshListsDestroy() );
	V( e_ViewersDestroy() );
	V( e_RenderListDestroy() );
	V( e_PortalsDestroy() );
	V( e_PrimitiveDestroy() );
	V( e_UIDestroy() );
	V( dxC_TargetBuffersDestroy() );
	V( e_ModelsDestroy() );
	V( dx9_DebugDestroy() );
	V( dx9_DrawListsDestroy() );
	V( dx9_ShadowsDestroy() );
	V( dx9_LightsDestroy() );
	V( e_TexturesDestroy() );
	V( dx9_EffectsDestroy() );
	V( dxC_BuffersDestroy() );
	V( dxC_ResourceSystemDestroy() );

	return S_OK;
}

PRESULT dxC_DestroyDeviceObjects()
{
#ifdef DXDUMP_ENABLE
	dx9_DXDestroy();
#endif

	//CC@HAVOK
	//DX10_BLOCK ( dx10_ReleaseGPUMeshParticleEmitter(); )

	HDRPipeline::DestroyResources();
	V( e_GrannyClose() );
	V( e_EnvironmentReleaseAll() );
	V( dx9_ScreenDrawRelease() );
	V( dx9_ScreenBuffersDestroy() );
	V( dxC_ReleaseStatQueries() );
	V( dx9_StateManagerDestroy() );
	V( dxC_QuadVBsDestroy() );
	DX9_BLOCK( V( dx9_VertexDeclarationsDestroy() ); )
	V( dx9_DefaultStatesDestroy() );

#ifdef HAVOKFX_ENABLED
	dxC_HavokFXDeviceRelease();
#endif

	// direct3D device
	V( sDeviceRelease() );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT e_Cleanup( BOOL bDumpLists /*= FALSE*/, BOOL bCleanParticles /*= FALSE*/ )
{
	// look for memory to reclaim

	if ( bCleanParticles )
	{
		V( e_ParticleSystemsReleaseDefinitionResources() );
	}

	// prune zero-ref model defs
	V( e_ModelDefinitionsCleanup() );

	// prune zero-ref 2D and cube textures
	V( e_TexturesCleanup() );

	// prune zero-ref vertex buffers

	// prune zero-ref index buffers

	if ( bDumpLists )
	{
		// whadda we got left?
		V( e_TextureDumpList() );
		V( e_ModelDumpList() );
	}
	
	// Flush any unused memory in the allocators
	//
#if USE_MEMORY_MANAGER	
	IMemoryManager::GetInstance().Flush();
#endif

	return S_OK;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_BeginScene( int nSwapChainIndex /*= DEFAULT_SWAP_CHAIN*/ )
{
	// no BeginScene in DX10
#ifdef ENGINE_TARGET_DX9
	if ( dxC_IsPixomaticActive() )
	{
		V_RETURN( dxC_PixoBeginScene() );
		return S_OK;
	}

	LPD3DCDEVICE pDevice = dxC_GetDevice();
	if ( pDevice )
	{
		V_RETURN( pDevice->BeginScene() );
	}
#endif

	LPD3DCSWAPCHAIN pRequestedSwapChain = dxC_GetD3DSwapChain( nSwapChainIndex );
	ASSERT_RETFAIL( pRequestedSwapChain );
	gnCurrentSwapChainIndex = nSwapChainIndex;

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_EndScene()
{
	// no EndScene in DX10
#ifdef ENGINE_TARGET_DX9
	if ( dxC_IsPixomaticActive() )
	{
		V_RETURN( dxC_PixoEndScene() );
		return S_OK;
	}

	LPD3DCDEVICE pDevice = dxC_GetDevice();
	if ( pDevice )
	{
		return pDevice->EndScene();
	}
#endif

	gnCurrentSwapChainIndex = DEFAULT_SWAP_CHAIN;

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

// CHB 2007.08.23 - This seems like a crummy way to deal with
// keeping the desktop resolution up-to-date. I'm open to
// suggestions...
static bool s_bChangingDevice = false;
void e_SetChangingDevice(bool bChanging)
{
	s_bChangingDevice = bChanging;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

// CHB 2007.07.26
PRESULT e_NotifyExternalDisplayChange(int nWidth, int nHeight)
{
	// CHB 2007.08.23 - This elaborate dance is so that we can
	// keep up-to-date on the desktop size, so that in the UI
	// (especially when going from full screen to windowed) we
	// can accurately show what window resolutions are available,
	// which is important because we don't want to create a
	// window that's larger than the desktop.
	if (!s_bChangingDevice)
	{
		e_SetDesktopSize(nWidth, nHeight);
	}

	// Ignore this for certain cases.
	if (e_IsNoRender() || c_GetToolMode() || AppCommonGetFillPakFile())
	{
		return S_FALSE;
	}

	// Only care about windowed mode.
	if (e_IsFullscreen())
	{
		return S_FALSE;
	}

	const int nWindowWidth = e_GetWindowWidth();
	const int nWindowHeight = e_GetWindowHeight();
	int nNewWindowWidth = nWindowWidth;
	int nNewWindowHeight = nWindowHeight;
	e_AdjustWindowSizeToFitDesktop(nNewWindowWidth, nNewWindowHeight);

	if ((nNewWindowWidth == nWindowWidth) && (nNewWindowHeight == nWindowHeight))
	{
		return S_FALSE;
	}

	CComPtr<OptionState> pState;
	PRESULT nResult = e_OptionState_CloneActive(&pState);
	V_RETURN(nResult);
	ASSERT_RETVAL(pState != 0, E_FAIL);

	pState->SuspendUpdate();
	pState->set_nFrameBufferWidth(nNewWindowWidth);
	pState->set_nFrameBufferHeight(nNewWindowHeight);
	pState->ResumeUpdate();

	V_RETURN(e_OptionState_CommitToActive(pState));

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT e_GetNextSwapChainIndex( int & nNextSwapChainIndex )
{
	int i;
	int nFirstAddlSwapChain = 1;
	for ( i = nFirstAddlSwapChain; i < MAX_SWAP_CHAINS; ++i )
		if ( ! gtSwapChains[i].pSwapChain )
			break;
	if ( i >= MAX_SWAP_CHAINS )
		return S_FALSE;
	nNextSwapChainIndex = i;
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------


PRESULT dxC_CreateAdditionalSwapChain( __out int & nIndex, DWORD nWidth, DWORD nHeight, HWND hWnd )
{
	nIndex = INVALID_ID;

#ifdef ENGINE_TARGET_DX10
	// Not yet supported in DX10
	return S_FALSE;
#else
	// DX9

	int i = -1;
	PRESULT pr = e_GetNextSwapChainIndex( i );
	if ( pr == S_FALSE || i < 0 )
		return S_FALSE;

	D3DPRESENT_PARAMETERS tPP;
	const OptionState * pState = e_OptionState_GetActive();
	dxC_StateToPresentation( pState, tPP );
	tPP.BackBufferWidth			= hWnd ? 0 : nWidth;		// if the hWnd is valid, set 0 to use the size of the window
	tPP.BackBufferHeight		= hWnd ? 0 : nHeight;
	tPP.Windowed				= TRUE;
	tPP.BackBufferCount			= 0;
	tPP.BackBufferFormat		= D3DFMT_UNKNOWN;
	tPP.hDeviceWindow			= hWnd;
	tPP.EnableAutoDepthStencil	= FALSE;					// No auto depth-stencil on additional swap chains
	
	V_RETURN( dxC_GetDevice()->CreateAdditionalSwapChain( &tPP, &gtSwapChains[i].pSwapChain ) );

	dxC_SetD3DPresentation( tPP, i );

	nIndex = i;

	V_RETURN( dxC_GrabSwapChainAndBackbuffer( nIndex ));

	// Either save or create the depth stencil to match
	V_RETURN( dx9_SaveAutoDepthStencil( nIndex ) );

	// Create this swap chain's rendertargets
	// Since this is after the main swap chain's render target creation, it should immediately create the new render targets.
	// They won't be in the optimal order for rendering, most likely.
	dxC_AddDescRTsDependOnBB( nIndex );

#endif
	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_ReleaseSwapChains()
{
	for ( int i = 0; i < MAX_SWAP_CHAINS; ++i )
	{
		if ( ! gtSwapChains[i].pSwapChain )
			continue;
		DX10_BLOCK( gtSwapChains[i].pSwapChain->SetFullscreenState( false, NULL ); ) //Need to switch out of fullscreen first
		gtSwapChains[i].pSwapChain = NULL;
	}
	gnCurrentSwapChainIndex = DEFAULT_SWAP_CHAIN;

	return S_OK;
}