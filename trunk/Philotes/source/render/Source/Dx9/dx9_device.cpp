//----------------------------------------------------------------------------
// dx9_device.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
	
#include "e_pch.h"

#include "dxC_effect.h"
#include "dxC_model.h"
#include "dxC_state.h"
#include "dxC_state_defaults.h"
#include "dxC_settings.h"
#include "dxC_caps.h"
#include "dxC_target.h"
#include "dxC_main.h"
#include "dxC_2d.h"
#include "dxC_environment.h"
#include "dxC_primitive.h"
#include "dxC_shadow.h"
#include "dxC_drawlist.h"
#include "dxC_query.h"
#include "dxC_particle.h"
#include "dx9_ui.h"
#include "dxC_wardrobe.h"
#include "dxC_effect.h"
#include "performance.h"
#include "e_frustum.h"
#include "dxC_occlusion.h"
#include "dxC_portal.h"
#include "e_renderlist.h"
#include "dxC_buffer.h"
#include "dxC_profile.h"
#include "dxC_hdrange.h"
#include "e_visibilitymesh.h"
#include "e_optionstate.h"	// CHB 2007.02.28
#include "dxC_movie.h"
#include "dxC_automap.h"

#include "dx9_device.h"
#ifdef HAVOKFX_ENABLED
	#include "dxC_HavokFX.h"
#endif
#include "dxC_fvf.h"

#include "globalindex.h"	// CHB 2007.08.01
#include "e_screen.h"		// CHB 2007.09.07
#include "e_gamma.h"		// CHB 2007.10.02

#include "dxC_pixo.h"

using namespace FSSE;

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

BOOL					gbDeviceLost			= FALSE;
BOOL					gbDeviceNeedReset		= FALSE;
SPDIRECT3D9				gpD3D					= NULL;


//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------
extern BOOL				gbDebugValidateDevice;

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// IMPLEMENTATION
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

D3DC_DRIVER_TYPE dx9_GetDeviceType()
{
	D3DC_DRIVER_TYPE nDeviceDriverType = e_OptionState_GetActive()->get_nDeviceDriverType();

	// can't pass NULLREF to any IDirect3D9 methods
	if ( nDeviceDriverType == D3DDEVTYPE_NULLREF )
		return D3DDEVTYPE_REF;

	return nDeviceDriverType;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

PRESULT dxC_GetAdapterOrdinalFromWindowPos( UINT & nAdapterOrdinal )
{
	HMONITOR hMon = NULL;
	V_RETURN( dxC_GetDefaultMonitorHandle( hMon ) );
	ASSERT_RETFAIL( hMon );

	UINT nAdapterCount = dx9_GetD3D()->GetAdapterCount();
	for ( UINT nAdapter = 0; nAdapter < nAdapterCount; ++nAdapter )
	{
		if ( hMon == dx9_GetD3D()->GetAdapterMonitor( nAdapter ) )
		{
			// Found our monitor/adapter!
			nAdapterOrdinal = nAdapter;
			return S_OK;
		}
	}

	return E_FAIL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

namespace
{

class LocalEventHandler : public OptionState_EventHandler
{
	public:
		virtual void Update(OptionState * pState) override;
};

void LocalEventHandler::Update(OptionState * pState)
{
	if ( e_IsNoRender() )
		return;
	if ( dxC_ShouldUsePixomatic() || dxC_IsPixomaticActive() )
		return;

	unsigned nAdapterOrdinal = pState->get_nAdapterOrdinal();
	D3DC_DRIVER_TYPE nDeviceDriverType = pState->get_nDeviceDriverType();

	BOOL bFound = FALSE;

#if ISVERSION(DEVELOPMENT)
	// Look for 'NVIDIA NVPerfHUD' adapter
	// If it is present, override default settings
	for (UINT Adapter = 0; Adapter < dx9_GetD3D()->GetAdapterCount() ; Adapter++)
	{
		D3DADAPTER_IDENTIFIER9 Identifier;
		V( dx9_GetD3D()->GetAdapterIdentifier( Adapter, 0, &Identifier ) );
		LogMessage( LOG_FILE, "Adapter: %d [%s]\n", Adapter, Identifier.Description );
		const char szPerfHUD[] = "NVIDIA NVPerfHUD";
		const char szPerfHUD4Bug[] = "NVIDIA\x09NVPerfHUD";
		const char szPerfHUD5[] = "NVIDIA PerfHUD";
		if ( PStrCmp( Identifier.Description, szPerfHUD5 ) == 0 ||
			 PStrCmp( Identifier.Description, szPerfHUD ) == 0 ||
			 PStrCmp( Identifier.Description, szPerfHUD4Bug ) == 0 )  // workaround for broken NVPerfHUD 4 string
		{
			nAdapterOrdinal = Adapter;
			nDeviceDriverType = D3DDEVTYPE_REF;
			LogMessage( LOG_FILE, "Found NVIDIA NVPerfHUD device!\n" );
			bFound = TRUE;
			break;
		}
	}
#endif // DEBUG_VERSION

#ifdef _D3DSHADER_DEBUG
	nDeviceDriverType = D3DC_DRIVER_TYPE_REF;
#endif

	// Detect the adapter from the default monitor.
	if ( ! bFound )
	{
		nAdapterOrdinal = D3DADAPTER_DEFAULT;

		if ( SUCCEEDED( dxC_GetAdapterOrdinalFromWindowPos( nAdapterOrdinal ) ) )
		{
			bFound = TRUE;
		}
	}

	ASSERTX( bFound, "Couldn't find dx9 adapter!  Using default..." );

	pState->set_nAdapterOrdinal(nAdapterOrdinal);
	pState->set_nDeviceDriverType(nDeviceDriverType);
}

};	// namespace

PRESULT dxC_Device_RegisterEventHandler(void)
{
	return e_OptionState_RegisterEventHandler(new LocalEventHandler);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static BOOL sIsWorkstationLocked()
{
	ASSERTV_RETFALSE( ! e_IsNoRender(), "Shouldn't be checking IsWorkstationLocked during fillpak!" );

	HDESK hd = OpenDesktop( "Default", 0, FALSE, DESKTOP_SWITCHDESKTOP );
	ASSERT_RETFALSE( hd );

	BOOL bIsLocked = FALSE;
	if ( ! SwitchDesktop( hd ) )
		bIsLocked = TRUE;

	CloseDesktop( hd );

	return bIsLocked;
}

LPDIRECT3D9 dx9_GetD3D( BOOL bForceRecreate /*= FALSE*/ )
{
	ASSERT_RETNULL( ! e_IsNoRender() );

	BOOL bDesktopWasLocked = FALSE;

	// The sIsWorkstationLocked check is redundant if we're forcing it anyway.
	if ( ! bForceRecreate )
	{
		while ( sIsWorkstationLocked() )
		{
			bDesktopWasLocked = TRUE;
			trace( "Workstation is locked...Sleeping until it is unlocked.\n" );
			Sleep( 500 );
			AppCommonRunMessageLoop();		
		}
	}

	if ( bForceRecreate || bDesktopWasLocked )
	{
		// Apparently, if you create a D3D object and then lock the desktop
		// you have to re-create the D3D object.
		if ( ! e_IsNoRender() )
			LogMessage( "Workstation was locked, so the D3D object must be re-created." );
		gpD3D.Release();
		dx9_Init();
	}

	return gpD3D;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void dx9_SetResetDevice()
{
	D3DXDebugMute( TRUE );
	gbDeviceNeedReset = TRUE;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dx9_SaveAutoDepthStencil( int nSwapChainIndex )
{
	if ( e_IsNoRender() )
		return S_OK;

	V_RETURN( dxC_AddAutoDepthTargetDesc( SWAP_DT_AUTO, nSwapChainIndex ) );

	if ( dxC_IsPixomaticActive() )
		return S_OK;

	ASSERT_RETFAIL( dxC_GetDevice() );

	const D3DPRESENT_PARAMETERS & tPP = dxC_GetD3DPP( nSwapChainIndex );

	DEPTH_TARGET_INDEX eDT = dxC_GetSwapChainDepthTargetIndex( nSwapChainIndex, SWAP_DT_AUTO );
	SPDIRECT3DSURFACE9 & pDSV = dxC_DSSurfaceGet( eDT );
	pDSV = NULL;

	if ( tPP.EnableAutoDepthStencil )
	{
		V_RETURN( dxC_GetDevice()->GetDepthStencilSurface( &pDSV ) );
		ASSERT_RETFAIL( pDSV );
	}
	else
	{
		// Can't use auto depth-stencil with addl swap chains!  Make a new one by hand!
		DXC_FORMAT tDSFormat = dxC_GetDefaultDepthStencilFormat();
#ifdef ENGINE_TARGET_DX9
		V_RETURN( dxC_GetDevice()->CreateDepthStencilSurface(
			tPP.BackBufferWidth,
			tPP.BackBufferHeight,
			tDSFormat,
			tPP.MultiSampleType,
			tPP.MultiSampleQuality,
			TRUE,
			&pDSV,
			NULL ) );
#elif ENGINE_TARGET_DX10
		SPD3DCTEXTURE2D & pResource = dxC_DSResourceGet( eDT );
		pResource = NULL;

		if ( ! pResource.p )
		{
			V_RETURN( dxC_Create2DTexture( tPP.BackBufferWidth, tPP.BackBufferHeight, 1, D3DC_USAGE_2DTEX_DS, tDSFormat, &pResource.p, NULL, NULL, tPP.MultiSampleType, tPP.MultiSampleQuality ) );
		}
		if ( ! pDSV.p )
		{
			V_RETURN( dxC_CreateDSV( pResource, &pDSV.p ) );
		}
#endif
	}

	dxC_DSSetDirty( eDT ); //Should always be dirty to start

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

PRESULT dx9_Init()
{
	TIMER_START("D3D Init")

	// make sure nothing is initalized
	ASSERT_RETFAIL( !gpD3D );

	// Create the D3D object

//#ifdef USE_PIXO
//	if ( dxC_IsPixomaticActive() )
//	{
//		// do nothing yet
//	}
//	else
//#endif
	{
		// Normal Direct3D9 object
		gpD3D.Attach( Direct3DCreate9( D3D_SDK_VERSION ) );
		ASSERTV_RETFAIL(gpD3D, "Failed to create D3D object!");
	}

	// Get the current desktop format
	//dxC_GetD3DDisplayMode( &gtDesktopMode );

	// is debug runtime
	// CHB 2006.10.10 - This is never used.
#if 0
	if ( GetModuleHandleW( L"d3d9d.dll" ) )
		gbRuntimeDebug = TRUE;
#endif

	// in non-debug builds, disable ability to use PIX to see into API calls
	// CHB 2006.10.10 - Allow me to use PIX at all times.
#if (!ISVERSION(DEBUG_VERSION)) && (!defined(USERNAME_cbatson))
	D3DPERF_SetOptions(1);
#endif

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dx9_ReleaseUnmanagedResources()
{
	// On Reset or DeviceLost, we need to free resources created in D3DPOOL_DEFAULT and prepare for the D3DDevice->Reset() call

	trace( "Releasing unmanaged resources... \n" );

	//  queries
	trace( " ... queries... ");
	V( dx9_ReleaseAllOcclusionQueries() );
	V( dxC_ReleaseStatQueries() );

	V( dxC_TargetBuffersRelease() );
	trace( "done!\n ... render targets and zbuffers... ");

	dxC_DebugTextureGetShaderResource() = NULL;
	gbBlurInitialized = FALSE;

	//  particle vertex buffer
	//    - NULL vertex buffer
	//    - release queries
	trace( "done!\n ... particle system buffers and queries... ");
	V( dx9_ParticleSystemsReleaseUnmanaged() );

	//  UI_TEXTUREDRAW vertex buffers
	//    - NULL vertex buffers
	trace( "done!\n ... UI vbuffers... ");
	V( dx9_UIReleaseVBuffers() );

	// quads
	V( dxC_QuadVBsDestroy() );

	//  UI_TEXTURE vertex buffers
	//    - NULL vertex buffers
	// handled via global flag - big hack
	// forces update (and therefore release) of UI_TEXTURE objects
	//V( e_UIX_Render() );

	//  All effects
	//    - Iterate all effects, calling OnDeviceLost()
	trace( "done!\n ... d3dx effects... ");
	V( dx9_NotifyEffectsDeviceLost() );

	//  debug primitive renders
	trace( "done!\n ... debug primitives... ");
	V( dx9_PrimitiveDrawDeviceLost() );

	//  portal render system
	V( dx9_PortalsReleaseUnmanaged() );

	// movies
	V( dxC_MoviesDeviceLost() );

	//V( dxC_WardrobeDeviceLost() );

	// tugboat-specific
	if ( AppIsTugboat() )
	{
		CVisibilityMesh * pVisMesh = e_VisibilityMeshGetCurrent();
		if ( pVisMesh )
		{
			pVisMesh->NotifyDeviceLost();
		}
	}

	// notify the tool that the device was lost (sortof a hack)
	trace( "done!\n ... tool resources... ");
	if ( c_GetToolMode() )
	{
		extern PFN_TOOLNOTIFY gpfn_ToolDeviceLost;
		if ( gpfn_ToolDeviceLost )
			gpfn_ToolDeviceLost();

		// clear pending renders that may contain smart pointers to released resources
		DL_CLEAR_LIST( DRAWLIST_INTERFACE );
	}

	//  HavokFX
#ifdef HAVOKFX_ENABLED
	trace( "done!\n ... havokFX... ");
	dxC_HavokFXDeviceLost();
#endif	

	V( dxC_AutomapDeviceLost() );

	// check that all default pool buffers are out of the appropriate lists
	for ( D3D_VERTEX_BUFFER* pVB = dxC_VertexBufferGetFirst();
		pVB;
		pVB = dxC_VertexBufferGetNext( pVB ) )
	{
		ASSERT( dx9_GetPool( pVB->tUsage ) != D3DPOOL_DEFAULT || !pVB->pD3DVertices );
	}

	for ( D3D_INDEX_BUFFER* pIB = dxC_IndexBufferGetFirst();
		pIB;
		pIB = dxC_IndexBufferGetNext( pIB ) )
	{
		ASSERT( dx9_GetPool( pIB->tUsage ) != D3DPOOL_DEFAULT || !pIB->pD3DIndices );
	}


	// check that all default pool textures are out of the sgtTextures list

	for (D3D_TEXTURE* pTexture = dxC_TextureGetFirst(); pTexture; )
	{
		D3D_TEXTURE* pNext = dxC_TextureGetNext( pTexture );
		// make sure that either it's not a POOL_DEFAULT texture or it's already been released
		ASSERT( dx9_GetPool( pTexture->tUsage ) != D3DPOOL_DEFAULT || !pTexture->pD3DTexture );
		pTexture = pNext;
	}

	for (D3D_CUBETEXTURE* pCubeTexture = dx9_CubeTextureGetFirst(); pCubeTexture; )
	{
		D3D_CUBETEXTURE* pNext = dx9_CubeTextureGetNext( pCubeTexture );
		// make sure that either it's not a POOL_DEFAULT texture or it's already been released
		ASSERT( pCubeTexture->nD3DPool != D3DPOOL_DEFAULT || !pCubeTexture->pD3DTexture );
		pCubeTexture = pNext;
	}


	trace( "done!\nSuccess!\n" );

	return S_OK;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
PRESULT dx9_RestoreUnmanagedResources()
{
	// On Reset or DeviceLost, we need to free resources created in D3DPOOL_DEFAULT and prepare for the D3DDevice->Reset() call

	trace( "Restoring unmanaged resources... \n" );

	//  all rendertargets
	//    - recreate render targets (auto-filled)
	//  all depth stencil targets
	//    - recreate ds targets (auto-filled)
	trace( "... render and depth stencil targets... ");
	// make updated-sized render targets - some of them operate relative to screen res

	dxC_AddRenderTargetDescs();
	V( dxC_RestoreRenderTargets() );

	//  UI_TEXTUREDRAW vertex buffers
	//    - recreate vertex buffers (auto-filled)
	trace( "done!\n ... UI vbuffers... ");
	V( e_UIRestore() );

	//  particle vertex buffer
	//    - recreate vertex buffer (auto-filled)
	//    - recreate queries
	trace( "done!\n ... particle system vbuffers and queries... ");
	V( dx9_ParticleSystemsRestoreUnmanaged() );

	V( dxC_AutomapDeviceReset() );

	// quads
	V( dxC_QuadVBsCreate() );

	// movies
	V( dxC_MoviesDeviceReset() );

	//  queries
	trace( "done!\n ... queries... " );
	V( dx9_RestoreAllOcclusionQueries() );
	V( dxC_RestoreStatQueries() );

	// tugboat-specific
	if ( AppIsTugboat() )
	{
		CVisibilityMesh * pVisMesh = e_VisibilityMeshGetCurrent();
		if ( pVisMesh )
		{
			pVisMesh->NotifyDeviceReset();		
			pVisMesh->UpdateLevelWideVisibility();
		}
	}


	//  UI_TEXTURE vertex buffers
	//    - recreate vertex buffers (auto-filled)
	// handled via global flag - big hack
	// this forces an update which will cause the vbuffers to be restored
	//  ... could probably wait until the first frame after reset...
	//e_UIX_Render();

	//  portal render system
	V( dx9_PortalsRestoreUnmanaged() );

	//V( dxC_WardrobeDeviceReset() );

	//  All effects
	//    - Iterate all effects, calling OnDeviceReset()
	trace( "done!\n ... d3dx effects... ");
	V( dx9_NotifyEffectsDeviceReset() );

	//  HavokFX
#ifdef HAVOKFX_ENABLED
	trace( "done!\n ... havokFX... ");
	dxC_HavokFXDeviceReset();
#endif	

	trace( "done!\nSuccess!\n" );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
PRESULT sResetDevice(
	void)
{
	TIMER_START( "Reset Device" );

	PRESULT pr;
	BOOL bReset = FALSE;

	if ( gbDeviceNeedReset )
	{
		D3DXDebugMute( FALSE );
		gbDeviceNeedReset = FALSE;
	}

	// if it isn't true yet (for example, ResetDevice() called for a fullscreen toggle), set it
	gbDeviceLost = TRUE;

	BOOL bLogTrace = LogGetTrace( 0 );
	LogSetTrace( 0, TRUE );

	V( dxC_ReleaseSwapChains() );
	V( dx9_ReleaseUnmanagedResources() );
	LogMessage( "Resetting device... " );

	// need to make local copy, because we pass D3DPP to CreateDevice/Reset() and either can change the structure
	D3DPRESENT_PARAMETERS tD3DPP;
	dxC_StateToPresentation(e_OptionState_GetActive(), tD3DPP);
	V_SAVE( pr, dxC_GetDevice()->Reset( &tD3DPP ) );

	if ( FAILED( pr ) )
	{
		const char* pszError = DXGetErrorString(pr);
		LogMessage("c_sResetDevice ERROR: %s\n", pszError);
		LogMessage( "Looping on ResetDevice... " );

		// loop on reset as long as the device is still lost
		while ( ( pr = dxC_GetDevice()->Reset( &tD3DPP ) ) == D3DERR_DEVICELOST )
		{
			AppCommonRunMessageLoop();
			Sleep( 0 );
		}

		if ( pr == D3DERR_DRIVERINTERNALERROR )
		{
			// fatal error, report and exit app
			LogMessage( "\nFATAL ERROR: Internal driver error detected in d3dDevice->Reset()!  Exiting...\n");
			GlobalStringMessageBox(AppCommonGetHWnd(), GS_MSG_ERROR_DRIVER_ERROR_TEXT, GS_MSG_UNRECOVERABLE_ERROR, MB_ICONERROR | MB_OK);
			PostQuitMessage(0);
		}

		ASSERT( pr != D3DERR_INVALIDCALL );

		if ( pr == D3DERR_OUTOFVIDEOMEMORY )
		{
			// fatal error, report and exit app
			LogMessage( "\nFATAL ERROR: Out of video memory, unable to proceed!  Exiting...\n");
			GlobalStringMessageBox(AppCommonGetHWnd(), GS_MSG_ERROR_NO_VIDEO_MEMORY_TEXT, GS_MSG_UNRECOVERABLE_ERROR, MB_ICONEXCLAMATION | MB_OK);
			PostQuitMessage(0);
		}

		if ( pr == E_OUTOFMEMORY )
		{
			// fatal error, report and exit app
			LogMessage( "\nFATAL ERROR: Out of memory, unable to proceed!  Exiting...\n");
			GlobalStringMessageBox(AppCommonGetHWnd(), GS_MSG_ERROR_NO_MEMORY_TEXT, GS_MSG_UNRECOVERABLE_ERROR, MB_ICONEXCLAMATION | MB_OK);
			PostQuitMessage(0);
		}
	}

	dxC_SetD3DPresentation(tD3DPP);
	dxC_ClearSetRenderTargetCache();

	V( dxC_GrabSwapChainAndBackbuffer( DEFAULT_SWAP_CHAIN ) );

	LogMessage( "success!\n" );
	gbDeviceLost = FALSE;

	// cycle state manager to fry cache
	dx9_SetStateManager( STATE_MANAGER_TRACK );
	dx9_SetStateManager();
	// this doesn't do the trick, yet
	//gpStateManager->DirtyCachedValues();

	V( dxC_GetDevice()->EvictManagedResources() );
	V( e_MarkEngineResourcesUnloaded() );
	e_PresentDelaySet();
	V( dx9_SaveAutoDepthStencil( DEFAULT_SWAP_CHAIN ) );
	V( e_UpdateGamma() );	// CHB 2007.09.27 - Moved up so UI has a chance to set gamma when its restore callback is called
	V( dx9_RestoreUnmanagedResources() );
	V( dx9_RestoreD3DStates( TRUE ) );

	// reset scissor rect and viewport
	V( dxC_SetScissorRect( NULL ) );
	V( dxC_ResetViewport() );

	bReset = TRUE;

//	V( e_UpdateGamma() );	// CHB 2007.09.27 - Moved up; see above.
	e_GetCurrentViewFrustum()->Dirty();
	V( e_AutomapsMarkDirty() );

	unsigned int tMS = (unsigned int)TIMER_END();
	LogMessage( "Reset Device duration: %ums\n", tMS );

	LogSetTrace( 0, bLogTrace );

	return S_OK;
}

PRESULT dx9_ResetDevice(
	void)
{
	// CHB 2007.08.23 - Need to bracket our changes to the display so
	// we can tell which WM_DISPLAYCHANGE messages were caused by us.
	e_SetChangingDevice(true);
	PRESULT nResult = sResetDevice();
	e_SetChangingDevice(false);
	return nResult;
}

PRESULT dx9_CheckDeviceLost()
{
	PRESULT pr = dxC_GetDevice()->TestCooperativeLevel();

	if ( FAILED( pr ) )
	{
		//BOOL bLogTrace = LogGetTrace( 0 );
		//LogSetTrace( 0, TRUE );
		//LogMessage( "Device lost!\n" );
		//LogSetTrace( 0, bLogTrace );
		gbDeviceLost = TRUE;
	}

	return pr;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#ifdef DX9_DEVICE_ALLOW_SWVP
bool dx9_IsSWVPDevice(void)
{
	D3DDEVICE_CREATION_PARAMETERS dcp;
	if (SUCCEEDED(dxC_GetDevice()->GetCreationParameters(&dcp)))
	{
		if ((dcp.BehaviorFlags & D3DCREATE_SOFTWARE_VERTEXPROCESSING) != 0)
		{
			return true;
		}
	}
	return false;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PRESULT dx9_RestoreD3DStates( BOOL bForce )
{
	if ( e_IsNoRender() )
		return S_FALSE;

	if ( dxC_IsPixomaticActive() )
		return S_FALSE;

	D3D_PROFILE_REGION( L"Restore D3D States" );

	// temp float used for casting purposes
	float tf;
	MATRIX matIdentity;
	MatrixIdentity( &matIdentity );
	int nWidth, nHeight;
	e_GetWindowSize( nWidth, nHeight );

	// set ALL D3D states to something reasonable

	V( dx9_SetMaterial( &gStubMtrl ) );
	for ( DWORD i = 0; i < dx9_CapGetValue( DX9CAP_MAX_ACTIVE_LIGHTS ); i++ )
	{
		V( dx9_SetLight( i, &gStubLight ) );
	}
	V( dx9_SetNPatchMode( 0.f ) );
	//dx9_SetDepthTarget( ZBUFFER_NONE );
	for ( int i = 0; i < dx9_GetMaxTextureStages(); i++ )
	{
		V( dxC_SetTexture( i, NULL ) );
		V( dx9_SetTextureStageState( i, D3DTSS_COLOROP,						D3DTOP_DISABLE,					bForce ) );
		V( dx9_SetTextureStageState( i, D3DTSS_COLORARG1,					D3DTA_TEXTURE,					bForce ) );
		V( dx9_SetTextureStageState( i, D3DTSS_COLORARG2,					D3DTA_CURRENT,					bForce ) );
		V( dx9_SetTextureStageState( i, D3DTSS_ALPHAOP,						D3DTOP_DISABLE,					bForce ) );
		V( dx9_SetTextureStageState( i, D3DTSS_ALPHAARG1,					D3DTA_DIFFUSE,					bForce ) );
		V( dx9_SetTextureStageState( i, D3DTSS_ALPHAARG2,					D3DTA_CURRENT,					bForce ) );
		V( dx9_SetTextureStageState( i, D3DTSS_BUMPENVMAT00,				FLOAT_AS_DWORD(tf = 0.f),		bForce ) );
		V( dx9_SetTextureStageState( i, D3DTSS_BUMPENVMAT01,				FLOAT_AS_DWORD(tf = 0.f),		bForce ) );
		V( dx9_SetTextureStageState( i, D3DTSS_BUMPENVMAT10,				FLOAT_AS_DWORD(tf = 0.f),		bForce ) );
		V( dx9_SetTextureStageState( i, D3DTSS_BUMPENVMAT11,				FLOAT_AS_DWORD(tf = 0.f),		bForce ) );
		V( dx9_SetTextureStageState( i, D3DTSS_TEXCOORDINDEX,				i,								bForce ) );
		V( dx9_SetTextureStageState( i, D3DTSS_BUMPENVLSCALE,				FLOAT_AS_DWORD(tf = 0.f),		bForce ) );
		V( dx9_SetTextureStageState( i, D3DTSS_BUMPENVLOFFSET,				FLOAT_AS_DWORD(tf = 0.f),		bForce ) );
		V( dx9_SetTextureStageState( i, D3DTSS_TEXTURETRANSFORMFLAGS,		D3DTTFF_DISABLE,				bForce ) );
		V( dx9_SetTextureStageState( i, D3DTSS_COLORARG0,					D3DTA_CURRENT,					bForce ) );
		V( dx9_SetTextureStageState( i, D3DTSS_ALPHAARG0,					D3DTA_CURRENT,					bForce ) );
		V( dx9_SetTextureStageState( i, D3DTSS_RESULTARG,					D3DTA_CURRENT,					bForce ) );
		V( dx9_SetTextureStageState( i, D3DTSS_CONSTANT,					(DWORD)0x00000000,				bForce ) );
	}
	V( dxC_SetTransform( D3DTS_VIEW,			(const D3DXMATRIXA16*)&matIdentity ) );
	V( dxC_SetTransform( D3DTS_PROJECTION,		(const D3DXMATRIXA16*)&matIdentity ) );
	//V( dxC_SetTransform( D3DTS_TEXTURE0,		(const D3DXMATRIXA16*)&matIdentity ) );
	//V( dxC_SetTransform( D3DTS_TEXTURE1,		(const D3DXMATRIXA16*)&matIdentity ) );
	//V( dxC_SetTransform( D3DTS_TEXTURE2,		(const D3DXMATRIXA16*)&matIdentity ) );
	//V( dxC_SetTransform( D3DTS_TEXTURE3,		(const D3DXMATRIXA16*)&matIdentity ) );
	//V( dxC_SetTransform( D3DTS_TEXTURE4,		(const D3DXMATRIXA16*)&matIdentity ) );
	//V( dxC_SetTransform( D3DTS_TEXTURE5,		(const D3DXMATRIXA16*)&matIdentity ) );
	//V( dxC_SetTransform( D3DTS_TEXTURE6,		(const D3DXMATRIXA16*)&matIdentity ) );
	//V( dxC_SetTransform( D3DTS_TEXTURE7,		(const D3DXMATRIXA16*)&matIdentity ) );
	//for ( DWORD i = 0; i < 256; i++ )
	//{
	//	V( dxC_SetTransform( D3DTS_WORLDMATRIX( i ), (const D3DXMATRIXA16*)&matIdentity ) );
	//}
	for ( DWORD i = 0; i < dx9_CapGetValue( DX9CAP_MAX_VERTEX_STREAMS ); i++ )
	{
		V( dxC_SetStreamSource( i, NULL, 0, 0 ) );
		V( dxC_GetDevice()->SetStreamSourceFreq( i, 1 ) );
	}
	for ( DWORD i = 1; i <  dx9_CapGetValue( DX9CAP_MAX_ACTIVE_RENDERTARGETS ); i++ )
	{
		V( dxC_GetDevice()->SetRenderTarget( i, NULL ) );
	}
	V( dxC_GetDevice()->SetDepthStencilSurface( NULL ) );
	//sSetVertexDeclaration( NULL );  // can't set null vert decl
	V( dx9_SetVertexShader( NULL ) );
	//dxC_SetIndices( NULL );
	V( dx9_SetPixelShader( NULL ) );
	//dxC_SetRenderTarget( RENDER_TARGET_BACKBUFFER );
	V( dxC_ClearSetRenderTargetCache() );
	//V( dxC_SetRenderTargetWithDepthStencil( RENDER_TARGET_BACKBUFFER, ZBUFFER_NONE ) );
	for ( DWORD i = 0; i < dx9_CapGetValue( DX9CAP_MAX_ACTIVE_TEXTURES ); i++ )
	{
		V( dx9_SetSamplerState( i, D3DSAMP_ADDRESSU,				D3DTADDRESS_WRAP,			bForce ) );
		V( dx9_SetSamplerState( i, D3DSAMP_ADDRESSV,				D3DTADDRESS_WRAP,			bForce ) );
		V( dx9_SetSamplerState( i, D3DSAMP_ADDRESSW,				D3DTADDRESS_WRAP,			bForce ) );
		V( dx9_SetSamplerState( i, D3DSAMP_BORDERCOLOR,			(DWORD)0x00000000,			bForce ) );
		V( dx9_SetSamplerState( i, D3DSAMP_MAGFILTER,			D3DTEXF_POINT,				bForce ) );
		V( dx9_SetSamplerState( i, D3DSAMP_MINFILTER,			D3DTEXF_POINT,				bForce ) );
		V( dx9_SetSamplerState( i, D3DSAMP_MIPFILTER,			D3DTEXF_NONE,				bForce ) );
		V( dx9_SetSamplerState( i, D3DSAMP_MIPMAPLODBIAS,		(DWORD)0,					bForce ) );
		V( dx9_SetSamplerState( i, D3DSAMP_MAXMIPLEVEL,			(DWORD)0,					bForce ) );
		V( dx9_SetSamplerState( i, D3DSAMP_MAXANISOTROPY,		(DWORD)1,					bForce ) );
		V( dx9_SetSamplerState( i, D3DSAMP_SRGBTEXTURE,			FALSE,						bForce ) );
		V( dx9_SetSamplerState( i, D3DSAMP_ELEMENTINDEX,			(DWORD)0,					bForce ) );
		V( dx9_SetSamplerState( i, D3DSAMP_DMAPOFFSET,			(DWORD)0,					bForce ) );
	}

	// CHB 2006.12.29 - Skip this if the device is using SWVP.
	if (!dx9_IsSWVPDevice())
	{
		V( dxC_GetDevice()->SetSoftwareVertexProcessing( FALSE ) );
	}

	V( dxC_SetViewport( 0, 0, nWidth, nHeight, 0.f, 1.f ) );
	V( dxC_SetScissorRect( NULL ) );
	V( dx9_SetFVF( D3DC_FVF_STD ) );

	BOOL bFogEnabled = FALSE;

	// force these renderstates, just to make sure the cache system doesn't chomp them
	dxC_SetRenderState( D3DRS_ZENABLE,						D3DZB_TRUE,														bForce );
	dxC_SetRenderState( D3DRS_FILLMODE,						D3DFILL_SOLID,													bForce );
	dxC_SetRenderState( D3DRS_SHADEMODE,					D3DSHADE_GOURAUD,												bForce );
	dxC_SetRenderState( D3DRS_ZWRITEENABLE,					TRUE,															bForce );
	dxC_SetRenderState( D3DRS_ALPHATESTENABLE,				FALSE,															bForce );
	dxC_SetRenderState( D3DRS_LASTPIXEL,					TRUE,															bForce );
	dxC_SetRenderState( D3DRS_SRCBLEND,						D3DBLEND_ONE,													bForce );
	dxC_SetRenderState( D3DRS_DESTBLEND,					D3DBLEND_ZERO,													bForce );
	dxC_SetRenderState( D3DRS_CULLMODE,						D3DCULL_CCW,													bForce );
	dxC_SetRenderState( D3DRS_ZFUNC,						D3DCMP_LESSEQUAL,												bForce );
	dxC_SetRenderState( D3DRS_ALPHAREF,						(DWORD)1,														bForce );
	dxC_SetRenderState( D3DRS_ALPHAFUNC,					D3DCMP_GREATEREQUAL,													bForce );
	dxC_SetRenderState( D3DRS_DITHERENABLE,					FALSE,															bForce );
	dxC_SetRenderState( D3DRS_ALPHABLENDENABLE,				FALSE,															bForce );
	//dxC_SetRenderState( D3DRS_FOGENABLE,					bFogEnabled,													bForce );  // set below
	dxC_SetRenderState( D3DRS_SPECULARENABLE,				FALSE,															bForce );
	dxC_SetRenderState( D3DRS_FOGCOLOR,						(DWORD)0,														bForce );
	dxC_SetRenderState( D3DRS_FOGTABLEMODE,					D3DFOG_NONE,													bForce );
	dxC_SetRenderState( D3DRS_FOGSTART,						FLOAT_AS_DWORD(tf = 0.f),										bForce );
	dxC_SetRenderState( D3DRS_FOGEND,						FLOAT_AS_DWORD(tf = 1.f),										bForce );
	dxC_SetRenderState( D3DRS_FOGDENSITY,					FLOAT_AS_DWORD(tf = 1.f),										bForce );
	dxC_SetRenderState( D3DRS_RANGEFOGENABLE,				FALSE,															bForce );
	dxC_SetRenderState( D3DRS_STENCILENABLE,				FALSE,															bForce );
	dxC_SetRenderState( D3DRS_STENCILFAIL,					D3DSTENCILOP_KEEP,												bForce );
	dxC_SetRenderState( D3DRS_STENCILZFAIL,					D3DSTENCILOP_KEEP,												bForce );
	dxC_SetRenderState( D3DRS_STENCILPASS,					D3DSTENCILOP_KEEP,												bForce );
	dxC_SetRenderState( D3DRS_STENCILFUNC,					D3DCMP_ALWAYS,													bForce );
	dxC_SetRenderState( D3DRS_STENCILREF,					(DWORD)0,														bForce );
	dxC_SetRenderState( D3DRS_STENCILMASK,					(DWORD)0xffffffff,												bForce );
	dxC_SetRenderState( D3DRS_STENCILWRITEMASK,				(DWORD)0xffffffff,												bForce );
	dxC_SetRenderState( D3DRS_TEXTUREFACTOR,				(DWORD)0xffffffff,												bForce );
	dxC_SetRenderState( D3DRS_WRAP0,						(DWORD)0,														bForce );
	dxC_SetRenderState( D3DRS_WRAP1,						(DWORD)0,														bForce );
	dxC_SetRenderState( D3DRS_WRAP2,						(DWORD)0,														bForce );
	dxC_SetRenderState( D3DRS_WRAP3,						(DWORD)0,														bForce );
	dxC_SetRenderState( D3DRS_WRAP4,						(DWORD)0,														bForce );
	dxC_SetRenderState( D3DRS_WRAP5,						(DWORD)0,														bForce );
	dxC_SetRenderState( D3DRS_WRAP6,						(DWORD)0,														bForce );
	dxC_SetRenderState( D3DRS_WRAP7,						(DWORD)0,														bForce );
	dxC_SetRenderState( D3DRS_CLIPPING,						TRUE,															bForce );
	dxC_SetRenderState( D3DRS_LIGHTING,						FALSE,															bForce );
	dxC_SetRenderState( D3DRS_AMBIENT,						(DWORD)0,														bForce );
	dxC_SetRenderState( D3DRS_FOGVERTEXMODE,				D3DFOG_NONE,													bForce );
	dxC_SetRenderState( D3DRS_COLORVERTEX,					FALSE,															bForce );
	dxC_SetRenderState( D3DRS_LOCALVIEWER,					TRUE,															bForce );
	dxC_SetRenderState( D3DRS_NORMALIZENORMALS,				FALSE,															bForce );
	dxC_SetRenderState( D3DRS_DIFFUSEMATERIALSOURCE,		D3DMCS_COLOR1,													bForce );
	dxC_SetRenderState( D3DRS_SPECULARMATERIALSOURCE,		D3DMCS_COLOR2,													bForce );
	dxC_SetRenderState( D3DRS_AMBIENTMATERIALSOURCE,		D3DMCS_MATERIAL,												bForce );
	dxC_SetRenderState( D3DRS_EMISSIVEMATERIALSOURCE,		D3DMCS_MATERIAL,												bForce );
	dxC_SetRenderState( D3DRS_VERTEXBLEND,					D3DVBF_DISABLE,													bForce );
	dxC_SetRenderState( D3DRS_CLIPPLANEENABLE,				(DWORD)0,														bForce );
	dxC_SetRenderState( D3DRS_POINTSIZE,					FLOAT_AS_DWORD(tf = 1.f),										bForce );
	dxC_SetRenderState( D3DRS_POINTSIZE_MIN,				FLOAT_AS_DWORD(tf = 1.f),										bForce );
	dxC_SetRenderState( D3DRS_POINTSPRITEENABLE,			FALSE,															bForce );
	dxC_SetRenderState( D3DRS_POINTSCALEENABLE,				FALSE,															bForce );
	dxC_SetRenderState( D3DRS_POINTSCALE_A,					FLOAT_AS_DWORD(tf = 1.f),										bForce );
	dxC_SetRenderState( D3DRS_POINTSCALE_B,					FLOAT_AS_DWORD(tf = 0.f),										bForce );
	dxC_SetRenderState( D3DRS_POINTSCALE_C,					FLOAT_AS_DWORD(tf = 0.f),										bForce );
	dxC_SetRenderState( D3DRS_MULTISAMPLEANTIALIAS,			TRUE,															bForce );
	dxC_SetRenderState( D3DRS_MULTISAMPLEMASK,				(DWORD)0xffffffff,												bForce );
	dxC_SetRenderState( D3DRS_PATCHEDGESTYLE,				D3DPATCHEDGE_DISCRETE,											bForce );
	dxC_SetRenderState( D3DRS_DEBUGMONITORTOKEN,			D3DDMT_ENABLE,													bForce );
	dxC_SetRenderState( D3DRS_POINTSIZE_MAX,				FLOAT_AS_DWORD(tf = 64.f),										bForce );
	dxC_SetRenderState( D3DRS_INDEXEDVERTEXBLENDENABLE,		FALSE,															bForce );
	dxC_SetRenderState( D3DRS_COLORWRITEENABLE,				D3DCOLORWRITEENABLE_RED  | D3DCOLORWRITEENABLE_GREEN |
															D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA,			bForce );
	dxC_SetRenderState( D3DRS_TWEENFACTOR,					FLOAT_AS_DWORD(tf = 0.f),										bForce );
	dxC_SetRenderState( D3DRS_BLENDOP,						D3DBLENDOP_ADD,													bForce );
	dxC_SetRenderState( D3DRS_POSITIONDEGREE,				D3DDEGREE_CUBIC,												bForce );
	dxC_SetRenderState( D3DRS_NORMALDEGREE,					D3DDEGREE_LINEAR,												bForce );
	dxC_SetRenderState( D3DRS_SCISSORTESTENABLE,			FALSE,															bForce );
	dxC_SetRenderState( D3DRS_SLOPESCALEDEPTHBIAS,			(DWORD)0,														bForce );
	dxC_SetRenderState( D3DRS_ANTIALIASEDLINEENABLE,		FALSE,															bForce );
	dxC_SetRenderState( D3DRS_MINTESSELLATIONLEVEL,			FLOAT_AS_DWORD(tf = 1.f),										bForce );
	dxC_SetRenderState( D3DRS_MAXTESSELLATIONLEVEL,			FLOAT_AS_DWORD(tf = 1.f),										bForce );
	dxC_SetRenderState( D3DRS_ADAPTIVETESS_X,				FLOAT_AS_DWORD(tf = 0.f),										bForce );
	dxC_SetRenderState( D3DRS_ADAPTIVETESS_Y,				FLOAT_AS_DWORD(tf = 0.f),										bForce );
	dxC_SetRenderState( D3DRS_ADAPTIVETESS_Z,				FLOAT_AS_DWORD(tf = 1.f),										bForce );
	dxC_SetRenderState( D3DRS_ADAPTIVETESS_W,				FLOAT_AS_DWORD(tf = 0.f),										bForce );
	dxC_SetRenderState( D3DRS_ENABLEADAPTIVETESSELLATION,	FALSE,															bForce );
	dxC_SetRenderState( D3DRS_TWOSIDEDSTENCILMODE,			FALSE,															bForce );
	dxC_SetRenderState( D3DRS_CCW_STENCILFAIL,				D3DSTENCILOP_KEEP,												bForce );
	dxC_SetRenderState( D3DRS_CCW_STENCILZFAIL,				D3DSTENCILOP_KEEP,												bForce );
	dxC_SetRenderState( D3DRS_CCW_STENCILPASS,				D3DSTENCILOP_KEEP,												bForce );
	dxC_SetRenderState( D3DRS_CCW_STENCILFUNC,				D3DCMP_ALWAYS,													bForce );
	dxC_SetRenderState( D3DRS_COLORWRITEENABLE1,			D3DCOLORWRITEENABLE_RED  | D3DCOLORWRITEENABLE_GREEN |
															D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA,			bForce );
	dxC_SetRenderState( D3DRS_COLORWRITEENABLE2,			D3DCOLORWRITEENABLE_RED  | D3DCOLORWRITEENABLE_GREEN |
															D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA,			bForce );
	dxC_SetRenderState( D3DRS_COLORWRITEENABLE3,			D3DCOLORWRITEENABLE_RED  | D3DCOLORWRITEENABLE_GREEN |
															D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_ALPHA,			bForce );
	dxC_SetRenderState( D3DRS_BLENDFACTOR,					(DWORD)0xffffffff,												bForce );
	dxC_SetRenderState( D3DRS_SRGBWRITEENABLE,				(DWORD)0,														bForce );
	dxC_SetRenderState( D3DRS_DEPTHBIAS,					(DWORD)0,														bForce );
	dxC_SetRenderState( D3DRS_WRAP8,						(DWORD)0,														bForce );
	dxC_SetRenderState( D3DRS_WRAP9,						(DWORD)0,														bForce );
	dxC_SetRenderState( D3DRS_WRAP10,						(DWORD)0,														bForce );
	dxC_SetRenderState( D3DRS_WRAP11,						(DWORD)0,														bForce );
	dxC_SetRenderState( D3DRS_WRAP12,						(DWORD)0,														bForce );
	dxC_SetRenderState( D3DRS_WRAP13,						(DWORD)0,														bForce );
	dxC_SetRenderState( D3DRS_WRAP14,						(DWORD)0,														bForce );
	dxC_SetRenderState( D3DRS_WRAP15,						(DWORD)0,														bForce );
	dxC_SetRenderState( D3DRS_SEPARATEALPHABLENDENABLE,		FALSE,															bForce );
	dxC_SetRenderState( D3DRS_SRCBLENDALPHA,				D3DBLEND_ONE,													bForce );
	dxC_SetRenderState( D3DRS_DESTBLENDALPHA,				D3DBLEND_ZERO,													bForce );
	dxC_SetRenderState( D3DRS_BLENDOPALPHA,					D3DBLENDOP_ADD,													bForce );

	// to keep in sync with our "fog-enabled" bool
	V( e_SetFogEnabled( bFogEnabled, bForce ) );

	// to maintain HDR mode
	V( dxC_RestoreHDRMode() );

	// set up our sampler states the way we want 'em
	V( dx9_ResetSamplerStates( bForce ) );

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const E_SCREEN_DISPLAY_MODE dxC_ConvertNativeDisplayMode(const D3DDISPLAYMODE & Desc)
{
	E_SCREEN_DISPLAY_MODE Mode;
	Mode.Width = Desc.Width;
	Mode.Height = Desc.Height;
	Mode.RefreshRate.Numerator = Desc.RefreshRate;
	Mode.RefreshRate.Denominator = 1;
	Mode.Format = Desc.Format;
	return Mode;
}

const D3DDISPLAYMODE dxC_ConvertNativeDisplayMode(const E_SCREEN_DISPLAY_MODE & Mode)
{
	D3DDISPLAYMODE Desc;
	Desc.Width = Mode.Width;
	Desc.Height = Mode.Height;
	Desc.RefreshRate = Mode.RefreshRate.Numerator;
	Desc.Format = static_cast<D3DFORMAT>(Mode.Format);
	return Desc;
}
