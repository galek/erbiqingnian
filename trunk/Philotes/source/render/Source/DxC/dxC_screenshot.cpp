//----------------------------------------------------------------------------
// dx9_screenshot.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#include "e_pch.h"
#include "appcommontimer.h"

#include "e_main.h"
#include "performance.h"
#include "filepaths.h"

#include "e_settings.h"
#include "e_screenshot.h"
#include "dxC_screenshot.h"
#include "e_screen.h"		// CHB 2007.09.07
#include "dxC_texture.h"
#include "dxC_target.h"
#include "dxC_viewer.h"

using namespace FSSE;

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define SCREENSHOT_CAPTURE_QUEUE_MAX	4
#define SCREENSHOT_FORMAT				D3DXIFF_BMP
#define SCREENSHOT_COMPRESSED_FORMAT	D3DXIFF_JPG
#define SCREENSHOT_EXTENSION			"bmp"
#define SCREENSHOT_COMPRESSED_EXTENSION	"jpg"
#define SCREENSHOT_PREFIX				"screenshot_"
#define SCREENSHOT_PATH					"Screenshots\\"
#define SCREENSHOT_MIN_TIME_BETWEEN		0.1f

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

struct ScreenShotInfo
{
	SCREENSHOT_PARAMS tParams;

	//BOOL bWithUI;
	BOOL bCaptured;
	OS_PATH_CHAR szFilePath[ DEFAULT_FILE_WITH_PATH_SIZE ];
	//BOOL bCompressed;
	//PFN_POST_SCREENSHOT_SAVE pfnCallback;
	//void * pCallbackData;
	float fElapsedTime;
	//int nTiled;
	int nCurTile;
	//int nViewerOverrideID;
};

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

static ScreenShotInfo gtScreenShotInfo[ SCREENSHOT_CAPTURE_QUEUE_MAX ];
static int gnScreenShotQueueCount	= INVALID_ID;
static int gnNextScreenShotFile	= INVALID_ID;
static float gfScreenShotLastGameTime = 0.0f;
static float gfScreenShotElapsedEngineTime = 0.0f;
static int gnScreenShotTiles	= 1;

#ifdef ENGINE_TARGET_DX9	// I'm not sure how this is done on DX10 yet
SPDIRECT3DSURFACE9 sgpScreenShotFullTiled;
#endif

//----------------------------------------------------------------------------
// IMPLEMENTATIONS
//----------------------------------------------------------------------------

static void sGetTileRect( ScreenShotInfo * pInfo, RECT & tRect, int nWidth, int nHeight )
{
	int nX		= pInfo->nCurTile % pInfo->tParams.nTiles;
	int nY		= pInfo->nCurTile / pInfo->tParams.nTiles;
	int nTileW	= nWidth  / pInfo->tParams.nTiles;
	int nTileH	= nHeight / pInfo->tParams.nTiles;
	int nTile	= pInfo->nCurTile;

	tRect.top		= ( nY     ) * nTileH;
	tRect.left		= ( nX     ) * nTileW;
	tRect.bottom	= ( nY + 1 ) * nTileH;
	tRect.right		= ( nX + 1 ) * nTileW;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static int sGetMaxScreenshotFileNumber( char * pszExtension )
{
	int nImage = INVALID_ID;

	char szShortname[ MAX_PATH ];
	PStrPrintf( szShortname, MAX_PATH, "%s*.%s", SCREENSHOT_PREFIX, pszExtension );

	char szRoot[ MAX_PATH ];
	PStrCvt( szRoot, FilePath_GetSystemPath(FILE_PATH_PUBLIC_USER_DATA), MAX_PATH );

	char szFilespec[ MAX_PATH ];
	PStrPrintf( szFilespec, MAX_PATH, "%s%s%s", szRoot, SCREENSHOT_PATH, szShortname );
	//FileGetFullFileName( szFilespec, szShortname, MAX_PATH );

	WIN32_FIND_DATA finddata;
	FindFileHandle handle = FindFirstFile( szFilespec, &finddata );
	if ( handle != INVALID_HANDLE_VALUE )
	{
		PStrPrintf( szShortname, MAX_PATH, "%s%%d.%s", SCREENSHOT_PREFIX, pszExtension );
		//FileGetFullFileName( szFilespec, szShortname, MAX_PATH );
		PStrCopy( szFilespec, szShortname, MAX_PATH );
		do
		{
			int nTemp;
			if ( !sscanf_s( finddata.cFileName, szFilespec, &nTemp ) )
				continue;
			if ( nTemp > nImage )
				nImage = nTemp;
		}
		while ( FindNextFile( handle, &finddata ) != 0 );

		return nImage + 1;
	}

	return 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sGetNewScreenshotFilename( OS_PATH_CHAR * pszFilePath, BOOL bCompressed )
{
	const WCHAR szFilenameFormatW[] = L"%s%S%S%06d.%S";
	const char  szFilenameFormatA[] =  "%s%s%s%06d.%s";

	const OS_PATH_CHAR * szFilenameFormat = NULL;
	if ( sizeof(OS_PATH_CHAR) == sizeof(char) )
	{
		szFilenameFormat = (OS_PATH_CHAR*)szFilenameFormatA;
	} else
	{
		szFilenameFormat = (OS_PATH_CHAR*)szFilenameFormatW;
	}

	int nImage = INVALID_ID;

	// initialize for error-checking
	pszFilePath[ 0 ] = NULL;

	// ensure screens directory exists
	char szFullpath[ MAX_PATH ];
	PStrCvt( szFullpath, FilePath_GetSystemPath(FILE_PATH_PUBLIC_USER_DATA), MAX_PATH );
	PStrCat( szFullpath, SCREENSHOT_PATH, MAX_PATH );
	CreateDirectory( szFullpath, NULL );

	char szExtension[ 8 ];
	if ( bCompressed )
		PStrCopy( szExtension, SCREENSHOT_COMPRESSED_EXTENSION, 8 );
	else
		PStrCopy( szExtension, SCREENSHOT_EXTENSION, 8 );

	if ( gnNextScreenShotFile == INVALID_ID )
	{
		// find biggest numbered file name in directory

		gnNextScreenShotFile = sGetMaxScreenshotFileNumber( SCREENSHOT_EXTENSION );
		nImage = sGetMaxScreenshotFileNumber( SCREENSHOT_COMPRESSED_EXTENSION );
		if ( nImage > gnNextScreenShotFile )
			gnNextScreenShotFile = nImage;
	}

	nImage = (int)gnNextScreenShotFile;

	ASSERT( nImage != INVALID_ID );
	ASSERT( nImage <= 1000000 ); // max the filename format supports, at least right now
	PStrPrintf( pszFilePath, DEFAULT_FILE_WITH_PATH_SIZE, szFilenameFormat, FilePath_GetSystemPath(FILE_PATH_PUBLIC_USER_DATA), SCREENSHOT_PATH, SCREENSHOT_PREFIX, nImage, szExtension );
	gnNextScreenShotFile = nImage + 1;
}

static void sQueueScreenshot( const SCREENSHOT_PARAMS & tParams )
{
	if ( gnScreenShotQueueCount >= SCREENSHOT_CAPTURE_QUEUE_MAX - 1 )
		return;

	if ( gnScreenShotQueueCount < 0 )
		gnScreenShotQueueCount = 0;
	else
		gnScreenShotQueueCount++;

	ASSERT( gnScreenShotQueueCount >= 0 && gnScreenShotQueueCount < SCREENSHOT_CAPTURE_QUEUE_MAX );
	ScreenShotInfo * pInfo = &gtScreenShotInfo[ gnScreenShotQueueCount ];
	pInfo->tParams = tParams;
	pInfo->bCaptured = FALSE;
	pInfo->fElapsedTime = 0.f;
	pInfo->nCurTile = 0;

	if ( pInfo->tParams.nTiles < 1 )
		pInfo->tParams.nTiles = gnScreenShotTiles;

	// currently not supporting tiled screenshots with UI
	if ( pInfo->tParams.nTiles > 1 )
		pInfo->tParams.bWithUI = FALSE;

	pInfo->tParams.nTiles = max( 1, pInfo->tParams.nTiles );

	// make filename
	sGetNewScreenshotFilename( pInfo->szFilePath, tParams.bSaveCompressed );
	ASSERT( pInfo->szFilePath[ 0 ] );
}

void e_CaptureScreenshot(
	const SCREENSHOT_PARAMS & tParams )
{
	if ( tParams.bBurst )
	{
		for ( int i = 0; i < SCREENSHOT_CAPTURE_QUEUE_MAX; i++ )
		{
			sQueueScreenshot( tParams );
		}
	} else
	{
		sQueueScreenshot( tParams );
	}
}

BOOL e_NeedScreenshotCapture( BOOL bWithUI, int nViewerID )
{
	if ( gnScreenShotQueueCount >= 0 )
	{
		// is this the right place to take this screen shot?
		if ( gtScreenShotInfo[ 0 ].tParams.bWithUI == bWithUI )
		{
			if ( ! gtScreenShotInfo[ 0 ].bCaptured )
			{
				if ( gtScreenShotInfo[ 0 ].tParams.nViewerOverrideID == INVALID_ID || gtScreenShotInfo[ 0 ].tParams.nViewerOverrideID == nViewerID )
				{
					return TRUE;
				}
			}
		}
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_ScreenshotCustomSavePostCaptureCallback( struct SCREENSHOT_PARAMS * ptParams, void * pImageData, RECT * pRect )
{
	ASSERT_RETURN( ptParams );
	ASSERT_RETURN( ptParams->pPostCaptureCallbackData );
	ASSERT_RETURN( pImageData );

#if defined(ENGINE_TARGET_DX9)

	const SCREENSHOT_SAVE_INFO * pSaveInfo = (const SCREENSHOT_SAVE_INFO*) ptParams->pPostCaptureCallbackData;

	LPDIRECT3DSURFACE9 pSrcSurface = (LPDIRECT3DSURFACE9)pImageData;

	const OS_PATH_CHAR * poszFilePath = pSaveInfo->GetFilePath();
	OS_PATH_CHAR oszFilePathExt[DEFAULT_FILE_WITH_PATH_SIZE];
	OS_PATH_CHAR oszPath[DEFAULT_FILE_WITH_PATH_SIZE];
	PStrGetPath( oszPath, DEFAULT_FILE_WITH_PATH_SIZE, poszFilePath );

	if ( ! FileDirectoryExists( oszPath ) )
	{
		ASSERT( FileCreateDirectory( oszPath ) );
	}

	for ( int i = 0; i < pSaveInfo->GetNumFormats(); ++i )
	{
		TEXTURE_SAVE_FORMAT tFileFmt;
		DXC_FORMAT tDataFmt;
		pSaveInfo->GetFormat( i, tFileFmt, (DWORD &)tDataFmt );
		D3DC_IMAGE_FILE_FORMAT tIFF = dxC_TextureSaveFmtToImageFileFmt( tFileFmt );
		const OS_PATH_CHAR * poszExt = dxC_TextureSaveFmtGetExtension( tIFF );

		PStrReplaceExtension( oszFilePathExt, DEFAULT_FILE_WITH_PATH_SIZE, poszFilePath, poszExt );

		D3DSURFACE_DESC tDesc;
		V_BREAK( pSrcSurface->GetDesc( &tDesc ) );
		SPDIRECT3DSURFACE9 pSaveSurface;
		if ( tFileFmt != TEXTURE_SAVE_DDS || dx9_IsEquivalentTextureFormat( tDataFmt, tDesc.Format ) )
		{
			pSaveSurface = pSrcSurface;
		}
		else
		{
			// Make a new surface, converting the formats
			V_CONTINUE( dxC_GetDevice()->CreateOffscreenPlainSurface( tDesc.Width, tDesc.Height, tDataFmt, D3DPOOL_SCRATCH, &pSaveSurface, NULL ) );
			V_CONTINUE( D3DXLoadSurfaceFromSurface(
				pSaveSurface,
				NULL,
				NULL,
				pSrcSurface,
				NULL,
				NULL,
				D3DX_FILTER_NONE,
				0 ) );
		}


		if ( pSaveInfo->TestFlag( SCREENSHOT_SAVE_INFO::OPEN_FOR_EDIT ) )
		{
			// open for edit
			TCHAR tszFilePath[ DEFAULT_FILE_WITH_PATH_SIZE ];
			PStrCvt( tszFilePath, oszFilePathExt, DEFAULT_FILE_WITH_PATH_SIZE );
			FileCheckOut( tszFilePath );
		}

		// save
		V( OS_PATH_FUNC(D3DXSaveSurfaceToFile)( oszFilePathExt, tIFF, pSaveSurface, NULL, pRect ) );

		if ( pSaveInfo->TestFlag( SCREENSHOT_SAVE_INFO::OPEN_FOR_ADD ) )
		{
			// open for add
			TCHAR tszFilePath[ DEFAULT_FILE_WITH_PATH_SIZE ];
			PStrCvt( tszFilePath, oszFilePathExt, DEFAULT_FILE_WITH_PATH_SIZE );
			FileCheckOut( tszFilePath );
		}
	}
#endif // DX9
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------


static PRESULT sCaptureScreenshot( ScreenShotInfo * pInfo )
{
	OS_PATH_CHAR szFilePath[ DEFAULT_FILE_WITH_PATH_SIZE ];
	FileGetFullFileName( szFilePath, pInfo->szFilePath, DEFAULT_FILE_WITH_PATH_SIZE );


	int nViewerID = gnMainViewerID;
	if ( pInfo->tParams.nViewerOverrideID != INVALID_ID )
		nViewerID = pInfo->tParams.nViewerOverrideID;
	Viewer * pViewer = NULL;
	V_RETURN( dxC_ViewerGet( nViewerID, pViewer ) );
	ASSERT_RETFAIL( pViewer );
	int nShotWidth, nShotHeight;
	RENDER_TARGET_INDEX nRT = dxC_GetSwapChainRenderTargetIndex( pViewer->nSwapChainIndex, SWAP_RT_BACKBUFFER );
	V_RETURN( dxC_GetRenderTargetDimensions( nShotWidth, nShotHeight, nRT ) );



	if ( pInfo->tParams.nTiles > 1 )
	{
		if ( pInfo->nCurTile >= (pInfo->tParams.nTiles * pInfo->tParams.nTiles ) )
			return S_OK;

#if defined(ENGINE_TARGET_DX9)
		if ( ! sgpScreenShotFullTiled )
		{
			int nWidth, nHeight;
			e_GetWindowSize( nWidth, nHeight );
			V_RETURN( dxC_GetDevice()->CreateOffscreenPlainSurface( pInfo->tParams.nTiles * nWidth, pInfo->tParams.nTiles * nHeight, D3DFMT_A8R8G8B8, D3DPOOL_SCRATCH, &sgpScreenShotFullTiled, NULL ) );
			ASSERT_RETFAIL( sgpScreenShotFullTiled );
		}
#endif

	}

#if defined(ENGINE_TARGET_DX9)

	LPD3DCSWAPCHAIN pSwapChain = dxC_GetD3DSwapChain( pViewer->nSwapChainIndex );
	ASSERT_RETFAIL( pSwapChain );

	int nRTFullWidth, nRTFullHeight;
	RENDER_TARGET_INDEX eRTFull = dxC_GetSwapChainRenderTargetIndexEx( pViewer->nSwapChainIndex, SWAP_RT_FULL );
	dxC_GetRenderTargetDimensions( nRTFullWidth, nRTFullHeight, eRTFull );


	SPDIRECT3DSURFACE9 pBuffer;
	RECT tRect;

	{

		TIMER_START( "Copying framebuffer" );
		if ( pInfo->tParams.bWithUI )
		{
			// use GetFrontBufferData on a 
			//   fullscreen - back-buffer-sized surface
			//   windowed - desktop-sized surface, then save just the appropriate rect

			E_SCREEN_DISPLAY_MODE tDesc;
			V_RETURN( dxC_GetD3DDisplayMode( tDesc ) );
			V_RETURN( dxC_GetDevice()->CreateOffscreenPlainSurface( tDesc.Width, tDesc.Height, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &pBuffer, NULL ) );
			ASSERT_RETFAIL( pBuffer );

			//V_RETURN( dxC_GetDevice()->GetFrontBufferData( 0, pBuffer ) );
			V_RETURN( pSwapChain->GetFrontBufferData( pBuffer ) );
		}
		else
		{
			ASSERT_RETFAIL( nShotWidth  <= nRTFullWidth  );
			ASSERT_RETFAIL( nShotHeight <= nRTFullHeight );

			E_SCREEN_DISPLAY_MODE tDesc;
			V_RETURN( dxC_GetD3DDisplayMode( tDesc ) );
			V_RETURN( dxC_GetDevice()->CreateOffscreenPlainSurface( nShotWidth, nShotHeight, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &pBuffer, NULL ) );
			ASSERT_RETFAIL( pBuffer );


			// If we don't want UI, we have to use the backbuffer
			SPDIRECT3DSURFACE9 pDestSystemSurf, pRenderedToSurf;


			SetRect( &tRect, 0, 0, nShotWidth, nShotHeight );


			// this is not the fastest way to do this, but it isn't a run-time thing, so no big deal
			if ( ! dxC_RTSystemTextureGet( eRTFull ) )
			{
				DXC_FORMAT tFmt = dxC_GetRenderTargetFormat( eRTFull );
				V( D3DXCreateTexture( dxC_GetDevice(), nRTFullWidth, nRTFullHeight, 1,
					D3DX_DEFAULT, tFmt, D3DPOOL_SYSTEMMEM, &dxC_RTSystemTextureGet( eRTFull ) ) );
			}


			if ( e_GetRenderFlag( RENDER_FLAG_RENDER_TO_FULLRT ) & MAKE_MASK(pViewer->nSwapChainIndex) )
			{
				// If we rendered directly to the full_MSAA RT, still need to copy the data off (trigger a resolve)
				// ... but the source is the FULL_MSAA RT instead of the backbuffer
				pRenderedToSurf = dxC_RTSurfaceGet( dxC_GetSwapChainRenderTargetIndexEx( pViewer->nSwapChainIndex, SWAP_RT_FULL_MSAA ) );
				ASSERT_RETFAIL( pRenderedToSurf );
			}
			else
			{
				V( pSwapChain->GetBackBuffer( 
					0, 
					D3DBACKBUFFER_TYPE_MONO, 
					&pRenderedToSurf ) );
			}

			V( dxC_GetDevice()->StretchRect(
				pRenderedToSurf, 
				&tRect, 
				dxC_RTSurfaceGet( eRTFull ), 
				&tRect, 
				D3DTEXF_LINEAR ) );


			V( dxC_RTSystemTextureGet( eRTFull )->GetSurfaceLevel( 0, &pDestSystemSurf ) );
			V( dxC_GetDevice()->GetRenderTargetData( 
				dxC_RTSurfaceGet( eRTFull ), 
				pDestSystemSurf ) );
			V( D3DXLoadSurfaceFromSurface(
				pBuffer, 
				NULL, 
				&tRect, 
				pDestSystemSurf, 
				NULL, 
				&tRect, 
				D3DX_FILTER_NONE, 
				0 ) );
		}
	}

	if ( pInfo->tParams.nTiles <= 1 )
	{
		TIMER_START( "Saving buffer to disk" );
		D3DC_IMAGE_FILE_FORMAT tFormat = pInfo->tParams.bSaveCompressed ? SCREENSHOT_COMPRESSED_FORMAT : SCREENSHOT_FORMAT;
		RECT * pRect = NULL;
		D3DPRESENT_PARAMETERS tPP = dxC_GetD3DPP( pViewer->nSwapChainIndex );
		WINDOWINFO tInfo;
		if ( tPP.Windowed )
		{
			GetWindowInfo( tPP.hDeviceWindow, &tInfo );
			if ( pInfo->tParams.bWithUI )
				pRect = & tInfo.rcClient;
			else
				pRect = & tRect;
		}
		if ( pInfo->tParams.pfnPostCaptureCallback )
		{
			pInfo->tParams.pfnPostCaptureCallback( &pInfo->tParams, pBuffer, pRect );
		}
		if ( pInfo->tParams.bSaveFile )
		{
			V_RETURN( OS_PATH_FUNC(D3DXSaveSurfaceToFile)( szFilePath, tFormat, pBuffer, NULL, pRect ) );

			// report capture
			char szLogMessage[ DEFAULT_FILE_WITH_PATH_SIZE + 64 ];
			PStrPrintf( szLogMessage, (DEFAULT_FILE_WITH_PATH_SIZE + 64), "Screen saved%s: %s\n", pInfo->tParams.bWithUI ? "" : " (no UI)", WIDE_CHAR_TO_ASCII_FOR_DEBUG_TRACE_ONLY(szFilePath) );
			LogMessage( LOG_FILE, szLogMessage );
			trace( szLogMessage );
		}
	} else
	{
		ASSERT_RETFAIL( sgpScreenShotFullTiled );
		RECT * pSrcRect = NULL;
		D3DPRESENT_PARAMETERS tPP = dxC_GetD3DPP( pViewer->nSwapChainIndex );
		WINDOWINFO tInfo;
		if ( tPP.Windowed )
		{
			GetWindowInfo( tPP.hDeviceWindow, &tInfo );
			pSrcRect = & tInfo.rcClient;
		}
		RECT tDestRect;
		int nWidth, nHeight;
		e_GetWindowSize( nWidth, nHeight );
		sGetTileRect( pInfo, tDestRect, pInfo->tParams.nTiles * nWidth, pInfo->tParams.nTiles * nHeight );

		V_RETURN( D3DXLoadSurfaceFromSurface( sgpScreenShotFullTiled, NULL, &tDestRect, pBuffer, NULL, pSrcRect, D3DTEXF_POINT, 0 ) );
	}
	pBuffer = NULL;

#elif defined(ENGINE_TARGET_DX10)

	ID3D10Texture2D *pBackBuffer = NULL;

	TIMER_START( "Copying backbuffer" );
	V_RETURN( dxC_GetD3DSwapChain()->GetBuffer( 0, __uuidof( ID3D10Texture2D ), (LPVOID*)&pBackBuffer ) );

	D3D10_TEXTURE2D_DESC tDesc;
	pBackBuffer->GetDesc( &tDesc );
	SPD3DCTEXTURE2D pBufferResolve;
	if ( tDesc.SampleDesc.Count > 1 )
	{
		V_RETURN( dxC_Create2DTexture( tDesc.Width, tDesc.Height, tDesc.MipLevels, 
			D3DC_USAGE_2DTEX, tDesc.Format, &pBufferResolve.p ) );
		dxC_GetDevice()->ResolveSubresource( pBufferResolve, 0, pBackBuffer, 0, tDesc.Format );
		pBackBuffer = pBufferResolve;
	}

	//Actually don't think this is necessary
	//ID3D10Texture2D* pStagingBuffer = NULL;
	//D3D10_TEXTURE3D_DESC Desc;
	//pBackBuffer->GetDesc( &Desc );
	//Desc.MipLevels = 1;
	//Desc.CPUAccessFlags = D3D10_CPU_ACCESS_READ ;
	//Desc.MiscFlags = 0;
	//Desc.Usage = D3D10_USAGE_STAGING;
	//Desc.BindFlags = 0;
	//dxC_GetDevice()->CreateTexture2D( &Desc, NULL, &pStagingBuffer );

	if ( pInfo->tParams.bSaveFile )
	{
		D3DC_IMAGE_FILE_FORMAT tFormat = pInfo->tParams.bSaveCompressed ? SCREENSHOT_COMPRESSED_FORMAT : SCREENSHOT_FORMAT;
		TIMER_START( "Saving backbuffer to disk" );
		V_RETURN( OS_PATH_FUNC(D3DX10SaveTextureToFile)( pBackBuffer, tFormat, szFilePath ) );
	}


#if 0 // Re-enable this if you want to write out a specific RT.
	int nRT = e_GetRenderFlag( RENDER_FLAG_RENDERTARGET_INFO );
	if ( nRT != INVALID_ID )
	{
		OS_PATH_CHAR szNewFilename[ DEFAULT_FILE_WITH_PATH_SIZE ];
		sGetNewScreenshotFilename( szNewFilename, pInfo->tParams.bSaveCompressed );
		FileGetFullFileName( szFilePath, szNewFilename, DEFAULT_FILE_WITH_PATH_SIZE );
		PStrReplaceExtension( szFilePath, DEFAULT_FILE_WITH_PATH_SIZE, szFilePath, OS_PATH_TEXT("dds") );

		RENDER_TARGET eRT = (RENDER_TARGET)nRT;
		V_RETURN( OS_PATH_FUNC(D3DX10SaveTextureToFile)( dxC_RTResourceGet( eRT, TRUE ),
			D3DXIFF_DDS, szFilePath ) );
	}
#endif

#endif

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void sSaveFullTiled( ScreenShotInfo * pInfo )
{
	if ( ! pInfo->tParams.bSaveFile )
		return;

#ifdef ENGINE_TARGET_DX9
	ASSERT_RETURN( sgpScreenShotFullTiled );
	D3DC_IMAGE_FILE_FORMAT tFormat = pInfo->tParams.bSaveCompressed ? SCREENSHOT_COMPRESSED_FORMAT : SCREENSHOT_FORMAT;
	V( OS_PATH_FUNC(D3DXSaveSurfaceToFile)( pInfo->szFilePath, tFormat, sgpScreenShotFullTiled, NULL, NULL ) );

	// report capture
	char szLogMessage[ DEFAULT_FILE_WITH_PATH_SIZE + 64 ];
	PStrPrintf( szLogMessage, (DEFAULT_FILE_WITH_PATH_SIZE + 64), "Screen saved%s: %s\n", pInfo->tParams.bWithUI ? "" : " (no UI)", pInfo->szFilePath );
	LogMessage( LOG_FILE, szLogMessage );
	trace( szLogMessage );
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void dxC_CaptureScreenshot()
{
	ASSERT( gnScreenShotQueueCount >= 0 && gnScreenShotQueueCount < SCREENSHOT_CAPTURE_QUEUE_MAX );
	ScreenShotInfo * pInfo = &gtScreenShotInfo[ 0 ];

	AppCommonSetPause(TRUE);
	UINT64 starttick = PGetPerformanceCounter();

	sCaptureScreenshot( pInfo );

	if ( pInfo->tParams.nTiles <= 1 )
	{
		pInfo->bCaptured = TRUE;
	} else
	{
		if ( pInfo->nCurTile >= ( pInfo->tParams.nTiles * pInfo->tParams.nTiles ) )
			return;

		pInfo->nCurTile++;
		if ( pInfo->nCurTile == ( pInfo->tParams.nTiles * pInfo->tParams.nTiles ) )
		{
			sSaveFullTiled( pInfo );
		}
	}

	UINT64 endtick = PGetPerformanceCounter();
	// following is necessary due to inconsistencies in QueryPerformanceCounter
	if (endtick < starttick)
	{
		endtick = starttick;
	}
	UINT64 elapsedticks = endtick - starttick;
	TIME elapsedmsec = (TIME)(elapsedticks * MSECS_PER_SEC / PGetPerformanceFrequency());
	trace( "Screenshot elapsed time: %d ms\n", (DWORD)elapsedmsec.time );
	float fElapsed = (float)elapsedmsec.time / MSECS_PER_SEC;
	gfScreenShotElapsedEngineTime -= fElapsed; // don't count this time against engine shot timeout

	gfScreenShotLastGameTime = e_GameGetTimeFloat();

	AppCommonSetPause(FALSE);

	if ( pInfo->tParams.pfnPostSaveCallback )
		pInfo->tParams.pfnPostSaveCallback( pInfo->szFilePath, pInfo->tParams.pPostSaveCallbackData );
}


// shifts the screenshot queue downward
void e_AdjustScreenShotQueue()
{
	if ( gnScreenShotQueueCount < 0 )
		return;

	BOOL bForce = FALSE;
	float fElapsed = e_GetElapsedTime();
	// This is done to track if the game isn't updating. If the screenshot request gets too old, just take it anyway.
	gfScreenShotElapsedEngineTime += fElapsed;
	if ( gfScreenShotElapsedEngineTime > ( SCREENSHOT_MIN_TIME_BETWEEN * 2 ) )
		bForce = TRUE;
	if ( c_GetToolMode() )
		bForce = TRUE;

	if ( !gbDrawUpdated )
		return;
	//if ( !gbTickUpdated || !gbDrawUpdated )
	//	return;
	float fTime = e_GameGetTimeFloat();
	if ( ! bForce && fTime < ( gfScreenShotLastGameTime + SCREENSHOT_MIN_TIME_BETWEEN ) )
		return;

	// if it was a tiled screenshot, iterate the tiles before moving on
	if ( gtScreenShotInfo[ 0 ].tParams.nTiles > 1 )
	{
		if ( gtScreenShotInfo[ 0 ].nCurTile >= ( gtScreenShotInfo[ 0 ].tParams.nTiles * gtScreenShotInfo[ 0 ].tParams.nTiles ) )
		{
			gtScreenShotInfo[ 0 ].bCaptured = TRUE;
		}
	}

	// adjust queue
	if ( gtScreenShotInfo[ 0 ].bCaptured )
	{
		MemoryMove( &gtScreenShotInfo[ 0 ], (SCREENSHOT_CAPTURE_QUEUE_MAX) * sizeof(ScreenShotInfo), &gtScreenShotInfo[ 1 ], sizeof(ScreenShotInfo) * SCREENSHOT_CAPTURE_QUEUE_MAX - 1 );
		if ( gnScreenShotQueueCount > 0 )
			gnScreenShotQueueCount--;
		else
			gnScreenShotQueueCount = INVALID_ID;
		gfScreenShotElapsedEngineTime = 0.f;

		DX9_BLOCK( sgpScreenShotFullTiled = NULL; )
	}

	gbTickUpdated = FALSE;
	gbDrawUpdated = FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL e_ScreenShotGetProjectionOverride( MATRIX * pmatProj, float fFOV, float fNear, float fFar )
{
	if ( gnScreenShotQueueCount < 0 )
		return FALSE;

	ScreenShotInfo * pInfo = &gtScreenShotInfo[ 0 ];
	if ( pInfo->tParams.nTiles <= 1 )
		return FALSE;

	if ( pInfo->nCurTile >= (pInfo->tParams.nTiles * pInfo->tParams.nTiles) )
		return FALSE;

	ASSERT_RETFALSE( pmatProj );

	RECT tOrig, tScreen;
	int nWidth, nHeight;
	e_GetWindowSize( nWidth, nHeight );

	sGetTileRect( pInfo, tScreen, nWidth, nHeight );

	tOrig.top		= 0;
	tOrig.left		= 0;
	tOrig.bottom	= nHeight;
	tOrig.right		= nWidth;

	MatrixMakeOffCenterPerspectiveProj( *pmatProj, &tOrig, &tScreen, fFOV, fNear, fFar );

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void e_ScreenShotSetTiles( int nTiles )
{
	if ( nTiles < 1 )
		nTiles = 1;
	gnScreenShotTiles = nTiles;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL e_ScreenShotRepeatFrame()
{
	if ( gnScreenShotQueueCount < 0 )
		return FALSE;

	ScreenShotInfo * pInfo = &gtScreenShotInfo[ 0 ];
	if ( pInfo->tParams.nTiles <= 1 )
		return FALSE;

	if ( pInfo->nCurTile >= ( pInfo->tParams.nTiles * pInfo->tParams.nTiles ) )
		return FALSE;

	return TRUE;
}