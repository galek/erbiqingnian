//----------------------------------------------------------------------------
// e_screenshot.h
//
// - Header for screenshot functions/structures
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __E_SCREENSHOT__
#define __E_SCREENSHOT__


//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// ENUMS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------

typedef void (*PFN_POST_SCREENSHOT_CAPTURE)( struct SCREENSHOT_PARAMS * ptParams, void * pImageData, RECT * pRect );
typedef void (*PFN_POST_SCREENSHOT_SAVE)( const OS_PATH_CHAR * pszFilename, void * pData );

//----------------------------------------------------------------------------
// FORWARD DECLARATIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

struct SCREENSHOT_PARAMS
{
	BOOL bWithUI;													// Capture screenshot with UI
	BOOL bBurst;													// Capture several screenshots a short time apart
	BOOL bSaveCompressed;											// Save the screenshot with lossy compression (JPG)
	BOOL bSaveFile;													// Save the screenshot to the standard location/naming scheme
	int nViewerOverrideID;											// Capture the output of a viewer other than the default
	int nTiles;														// For tiled super-screenshots

	PFN_POST_SCREENSHOT_CAPTURE pfnPostCaptureCallback;				// Call this callback function after capture is complete (before saving)
	void * pPostCaptureCallbackData;								// User data for post-capture callback

	PFN_POST_SCREENSHOT_SAVE pfnPostSaveCallback;					// Call this callback function after saving the screenshot file
	void * pPostSaveCallbackData;									// User data for post-save callback

	SCREENSHOT_PARAMS() :
		bWithUI(FALSE),
		bBurst(FALSE),
		bSaveCompressed(FALSE),
		bSaveFile(TRUE),
		nViewerOverrideID(INVALID_ID),
		nTiles(-1),
		pfnPostCaptureCallback(NULL),
		pPostCaptureCallbackData(NULL),
		pfnPostSaveCallback(NULL),
		pPostSaveCallbackData(NULL)
	{}
};


struct SCREENSHOT_SAVE_INFO
{
	enum FLAGBIT
	{
		OPEN_FOR_EDIT = 0,
		OPEN_FOR_ADD,
	};

private:
	static const int		cnNameLen = DEFAULT_FILE_WITH_PATH_SIZE;
	static const int		cnMaxSaveFormats = 4;
	OS_PATH_CHAR			oszFilePath[ cnNameLen ];
	TEXTURE_SAVE_FORMAT		tFileFmt[ cnMaxSaveFormats ];
	DWORD					dwDataFmt[ cnMaxSaveFormats ];
	int						nSaveFormats;
	DWORD					dwFlagBits;

public:
	SCREENSHOT_SAVE_INFO() : nSaveFormats(0), dwFlagBits(0) {}

	void AddFormat( TEXTURE_SAVE_FORMAT tFileFormat, DWORD dwDataFormat = 0 )
	{
		ASSERT_RETURN( nSaveFormats < cnMaxSaveFormats );
		tFileFmt[ nSaveFormats ] = tFileFormat;
		dwDataFmt[ nSaveFormats ] = dwDataFormat;
		++nSaveFormats;
	}
	void SetFilename( const OS_PATH_CHAR * poszFilePath )
	{
		PStrCopy( oszFilePath, poszFilePath, cnNameLen );
	}
	void SetFlag( FLAGBIT eFlag, BOOL bVal )
	{
		SETBIT( dwFlagBits, eFlag, bVal );
	}


	int GetNumFormats() const
	{
		return nSaveFormats;
	}
	void GetFormat( int nIndex, TEXTURE_SAVE_FORMAT & tFileFormat, DWORD & dwDataFormat ) const
	{
		ASSERT_RETURN( nIndex < nSaveFormats );
		tFileFormat = tFileFmt[nIndex];
		dwDataFormat = dwDataFmt[nIndex];
	}
	const OS_PATH_CHAR * GetFilePath() const
	{
		return oszFilePath;
	}
	BOOL TestFlag( FLAGBIT eFlag ) const
	{
		return TESTBIT( dwFlagBits, eFlag );
	}
};


//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------

extern BOOL gbTickUpdated;
extern BOOL gbDrawUpdated;

//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// HEADERS
//----------------------------------------------------------------------------

void e_CaptureScreenshot( const SCREENSHOT_PARAMS & tParams );
void e_MarkGameTick();
void e_MarkGameDraw();
BOOL e_ToggleScreenshotQuality();
BOOL e_GetScreenshotCompress();
void e_AdjustScreenShotQueue();
BOOL e_NeedScreenshotCapture( BOOL bWithUI, int nViewerID );
BOOL e_ScreenShotGetProjectionOverride( MATRIX * pmatProj, float fFOV, float fNear, float fFar );
void e_ScreenShotSetTiles( int nTiles );
BOOL e_ScreenShotRepeatFrame();

void e_ScreenshotCustomSavePostCaptureCallback( struct SCREENSHOT_PARAMS * ptParams, void * pImageData, RECT * pRect );


#endif // __E_SCREENSHOT__
