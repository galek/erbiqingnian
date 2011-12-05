//----------------------------------------------------------------------------
// FILE: c_credits.cpp
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
#include "stdafx.h"  // must be first for pre-compiled headers
#include "c_credits.h"
#include "c_movieplayer.h"
#include "c_sound.h"
#include "c_sound_util.h"
#include "c_music.h"
#include "e_settings.h"
#include "excel.h"
#include "fontcolor.h"
#include "game.h"
#include "gameglobals.h"
#include "globalindex.h"
#include "language.h"
#include "prime.h"
#include "sku.h"
#include "stringtable.h"
#include "stringtable_common.h"
#include "uix.h"
#include "uix_components.h"
#include "uix_priv.h"
#include "uix_scheduler.h"

BOOL gbAutoAdvancePages = TRUE;

#if !ISVERSION(SERVER_VERSION)

//----------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
enum CREDIT_CONSTANTS
{

	MAX_CREDIT_SECTIONS = 4,	// arbitrary, this is how many UI elements we have setup for sections
	MAX_CREDIT_TITLES = 10,		// arbitrary, this is how many UI elements we have setup for sections
	MAX_CREDIT_NAMES = 10,		// arbitrary, this is how many UI elements we have setup for sections
	
	MAX_CREDIT_LINES = 2048,	// max credit lines that exist

	MAX_CREDIT_BACKGROUNDS = 32,		// max backgrounds/movies we can cycle between in the credits
	
	MAX_CREDIT_SKUS = 64,
	
};

//----------------------------------------------------------------------------
enum CREDIT_DISPLAY_STYLE
{
	CDS_SCROLL,				// scroller credits
	CDS_PAGE,				// page credits
};

//----------------------------------------------------------------------------
enum CREDIT_TYPE
{
	CT_INVALID = -1,

	CT_PAGEBREAK,	
	CT_SECTION,
	CT_TITLE,
	CT_NAME,
	CT_FONT_SIZE,
	CT_SKU_ADD,
	CT_SKU_REMOVE,
	
	CT_NUM_TYPES
	
};

//----------------------------------------------------------------------------
struct CREDIT_LINE
{
	const WCHAR *puszString;		// the text of the line
	CREDIT_TYPE eType;				// what type of line is this (section, title, name, etc)
	int nNumNewLinesBeforeString;	// how many empty lines should be inserted before this string
};

//----------------------------------------------------------------------------
struct CREDIT_GLOBALS
{

	// big pre-formatted text buffer to hold all the credits (for scrolling)
	WCHAR *puszTextBuffer;		// the text
	int nMaxTextBuffer;			// max characters in the text buffer

	// page style credits
	CREDIT_LINE tCreditLine[ MAX_CREDIT_LINES ];	// the data for page style credits
	int nNumCreditLines;							// how many tCreditLine[] we have
	int nCurrentLine;								// when displaying pages of credit lines, this is where we're at
	const CREDIT_LINE *pCurrentSection;				// current section being displayed in credit pages
	const CREDIT_LINE *pCurrentTitle;				// current section being displayed in credit pages
	DWORD dwPageEvent;								// used for all advancements in the page credits system (fade outs, text changes, movie changes, etc)
	int nMovieCurrent;								// movie to be displaying in the background
	TIME tiMovieCurrentShowTime;					// time nMovieCurrent was put on the screen
	BOOL bAdvanceTextAtFadeOut;						// advance the text when we hit the fade out state
	int nMovieBackgroundPool[ MAX_CREDIT_BACKGROUNDS ];	// the movies we have to pick from
	int nNumMovieBackgroundPool;
	int nMusic;
	DWORD dwMusicEvent;
	
	int nSKUIncluded[ MAX_CREDIT_SKUS ];
	int nNumSKUIncluded;
	
};

//----------------------------------------------------------------------------
struct CREDIT_MUSIC_TIMING
{
	int nPlayTimeInMilliseconds;
	GLOBAL_INDEX nSoundGroup;
};

static const int sgnCreditMusicDelayTimeInMilliseconds = 2 * 3700;

static const CREDIT_MUSIC_TIMING sgpCreditMusic[] = 
{
	//*
	// 3:13
	{ 193000, GI_SOUND_CREDITS_1 },
	// 1:30
	{  90000, GI_SOUND_CREDITS_2 },
	// 1:07
	{  67000, GI_SOUND_CREDITS_3 },
	// 1:19
	{  79000, GI_SOUND_CREDITS_4 },
	// 1:19
	{  79000, GI_SOUND_CREDITS_5 },
	// */
	/*
	// Test Timings
	{ 10000, GI_SOUND_CREDITS_1 },
	{ 10000, GI_SOUND_CREDITS_2 },
	{ 10000, GI_SOUND_CREDITS_3 },
	{ 10000, GI_SOUND_CREDITS_4 },
	{ 10000, GI_SOUND_CREDITS_5 },
	// */
};
static const int sgnCreditsMusicCount = sizeof(sgpCreditMusic) / sizeof(CREDIT_MUSIC_TIMING);

//----------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------

static CREDIT_GLOBALS sgtCreditGlobals;
static BOOL sgbCreditsInitialized = FALSE;

static const WCHAR *TOKEN_PAGEBREAK = L"[PAGEBREAK]";
static const WCHAR *TOKEN_SECTION = L"[SECTION]";
static const WCHAR *TOKEN_TITLE = L"[TITLE]";
static const WCHAR *TOKEN_FONT_SIZE= L"[FONTSIZE]";
static const WCHAR *TOKEN_SKU_ADD = L"[SKUADD]";
static const WCHAR *TOKEN_SKU_REMOVE = L"[SKUREMOVE]";

// credit display options
static CREDIT_DISPLAY_STYLE sgeCreditDisplayStyle = CDS_PAGE;
static const WCHAR *sgpuszIndent = L"      ";
static const WCHAR *sgpuszNewLine = L"\n";
static const DWORD SCREEN_FADE_TIME_MS = MSECS_PER_SEC / 2;
static const float PAGE_FADE_OUT_TIME_IN_SECONDS = 0.5f;
static const BOOL sgbStartAtLastSection = FALSE;
static BOOL sgbCreditsAutoEnterFromMainMenu = FALSE;

//----------------------------------------------------------------------------
// Forward Declarations
//----------------------------------------------------------------------------
static void sCreditsEnable( BOOL bEnable );
static void sCreditPageFadeOut( GAME *pGame, const CEVENT_DATA &tEventData, DWORD );
static void sCreditPageAdvanceText( GAME *pGame, const CEVENT_DATA &tEventData, DWORD );
static void sCreditsMusicPlayNext( GAME *pGame, const CEVENT_DATA &tEventData, DWORD );

//----------------------------------------------------------------------------
// Function Definitions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static CREDIT_GLOBALS *sCreditGlobalsGet(
	BOOL bValidate = TRUE)
{
	if (bValidate)
	{
		ASSERTX_RETNULL( sgbCreditsInitialized == TRUE, "Credits not initialized" );
	}
	return &sgtCreditGlobals;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCreditsPreloadSounds()
{
	for(int i=0; i<sgnCreditsMusicCount; i++)
	{
		c_SoundLoad(sgpCreditMusic[i].nSoundGroup);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCreditsUnloadSounds()
{
	c_SoundStopAll();
	for(int i=0; i<sgnCreditsMusicCount; i++)
	{
		c_SoundUnload(GlobalIndexGet(sgpCreditMusic[i].nSoundGroup));
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCreditsMusicStopTrack( 
	GAME *pGame, 
	const CEVENT_DATA &tEventData, 
	DWORD )
{
	CREDIT_GLOBALS * pGlobals = sCreditGlobalsGet();
	c_SoundStop(pGlobals->nMusic);
	DWORD dwDelayInMS = (DWORD)sgnCreditMusicDelayTimeInMilliseconds;
	pGlobals->dwMusicEvent = CSchedulerRegisterEvent( AppCommonGetCurTime() + dwDelayInMS, sCreditsMusicPlayNext, tEventData);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sCreditsMusicPlay(int nMusicIndex)
{
	ASSERTX_RETINVALID(nMusicIndex >= 0, "Invalid Credits Music Index");
	if(nMusicIndex >= sgnCreditsMusicCount)
	{
		nMusicIndex = 0;
	}
	CREDIT_GLOBALS * pGlobals = sCreditGlobalsGet();
	DWORD dwDelayInMS = (DWORD)sgpCreditMusic[nMusicIndex].nPlayTimeInMilliseconds;
	CEVENT_DATA tEventData(nMusicIndex);
	pGlobals->dwMusicEvent = CSchedulerRegisterEvent( AppCommonGetCurTime() + dwDelayInMS, sCreditsMusicStopTrack, tEventData);
	return c_SoundPlay(GlobalIndexGet(sgpCreditMusic[nMusicIndex].nSoundGroup));
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCreditsMusicPlayNext( 
	GAME *pGame, 
	const CEVENT_DATA &tEventData, 
	DWORD )
{
	CREDIT_GLOBALS * pGlobals = sCreditGlobalsGet();
	int nMusicIndex = (int)tEventData.m_Data1;
	pGlobals->nMusic = sCreditsMusicPlay(nMusicIndex+1);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int c_CreditsGetCurrentMovie(
	void)
{

	if (sgbCreditsInitialized)
	{
		CREDIT_GLOBALS *pGlobals = sCreditGlobalsGet();
		ASSERTX_RETINVALID( pGlobals, "Expected credit globals" );
		
		// return the current background movie
		return pGlobals->nMovieCurrent;
	}
	
	return INVALID_LINK;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sMovieFadedOut(
	void)
{

	// forget it if we're out now
	if (sgbCreditsInitialized == FALSE)
	{
		return;
	}

	// get globals	
	CREDIT_GLOBALS *pGlobals = sCreditGlobalsGet();
	ASSERTX_RETURN( pGlobals, "Expected credit globals" );
	
	// what's the current movie
	int nMovieCurrent = pGlobals->nMovieCurrent;
	int nMovieNext = nMovieCurrent;

	// if there is no movie, pick a movie to start at
	if (nMovieCurrent == INVALID_LINK)
	{
		RAND tRand;
		RandInit( tRand, (int)AppCommonGetCurTime() );
		int nPick = RandGetNum( tRand, 0, pGlobals->nNumMovieBackgroundPool - 1 );
		nMovieNext = pGlobals->nMovieBackgroundPool[ nPick ];
	}
	else
	{
	
		// find index of this movie in the list
		int nIndex = NONE;
		for (int i = 0; i < pGlobals->nNumMovieBackgroundPool; ++i)
		{
			if (pGlobals->nMovieBackgroundPool[ i ] == nMovieCurrent)
			{
				nIndex = i;
				break;
			}
		}
		
		// advance to the next movie, loop to first one at end
		nIndex++;
		if (nIndex >= pGlobals->nNumMovieBackgroundPool)
		{
			nIndex = 0;
		}
		
		// set next movie index
		nMovieNext = pGlobals->nMovieBackgroundPool[ nIndex ];
		
	}
		
	// set the current movie to the next movie
	if (nMovieCurrent != nMovieNext)
	{
	
		// save the movie to play
		pGlobals->nMovieCurrent = nMovieNext;
		pGlobals->tiMovieCurrentShowTime = AppCommonGetCurTime();
				
		// tell the movie player to switch
		AppStartOrStopBackgroundMovie();	
		
	}

	// advance the text if we need to
	if (pGlobals->bAdvanceTextAtFadeOut)
	{
		DWORD dwDelayInMS = (DWORD)(MSECS_PER_SEC * PAGE_FADE_OUT_TIME_IN_SECONDS);
		CEVENT_DATA tEventData;
		pGlobals->dwPageEvent = CSchedulerRegisterEvent( AppCommonGetCurTime() + dwDelayInMS, sCreditPageAdvanceText, tEventData);
	}

	// used the text advance
	pGlobals->bAdvanceTextAtFadeOut = FALSE;

	// fade in
	V( e_BeginFade( FADE_IN, SCREEN_FADE_TIME_MS, 0x00000000 ) );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCreditsChangeMovie(
	GAME *pGame,
	const CEVENT_DATA &tEventData,
	DWORD)
{
	CREDIT_GLOBALS *pGlobals = sCreditGlobalsGet();
	ASSERTX_RETURN( pGlobals, "Expected credit globals" );
	BOOL bAdvanceText = (BOOL)tEventData.m_Data1;
	
	// begin fade to back
	pGlobals->bAdvanceTextAtFadeOut = bAdvanceText;	
	V( e_BeginFade( FADE_OUT, SCREEN_FADE_TIME_MS, 0x00000000, sMovieFadedOut ) );

	// in case Present was delayed, set it to 0
	e_PresentDelaySet( 0 );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCreditUIFade(
	BOOL bFadeIn)
{
	CREDIT_GLOBALS *pGlobals = sCreditGlobalsGet();
	ASSERTX_RETURN( pGlobals, "Expected credit globals" );

	UIComponentActivate(UICOMP_LABEL_CREDITS_TEXT_PAGE, bFadeIn);
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCreditGlobalsFree(
	void)
{
	
	if(sgbCreditsInitialized == TRUE)
	{
		CREDIT_GLOBALS *pGlobals = sCreditGlobalsGet();
		ASSERTX_RETURN( pGlobals, "Expected credit globals" );

		// hide all the comps
		sCreditUIFade(FALSE);
		
		// free text buffer
		GFREE( NULL, pGlobals->puszTextBuffer );
		pGlobals->puszTextBuffer = NULL;

		// cancel any page event we have going
		if (pGlobals->dwPageEvent != 0)
		{
			CSchedulerCancelEvent( pGlobals->dwPageEvent );
			pGlobals->dwPageEvent = 0;
		}

		// cancel any music event we have going
		if (pGlobals->dwMusicEvent != 0)
		{
			CSchedulerCancelEvent( pGlobals->dwMusicEvent );
			pGlobals->dwMusicEvent = 0;
		}

		// clear movie
		pGlobals->nMovieCurrent = INVALID_LINK;

		sCreditsUnloadSounds();
		pGlobals->nMusic = INVALID_ID;

	}

	// no longer initialized
	sgbCreditsInitialized = FALSE;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCreditTextBufferAddString(
	const WCHAR *puszString)
{
	ASSERTX_RETURN( puszString, "Expected string" );
	
	// get globals
	CREDIT_GLOBALS *pGlobals = sCreditGlobalsGet( FALSE );  // bypass init validation since we're initializing
	ASSERTX_RETURN( pGlobals, "Expected credit globals" );

	// add string
	PStrCat( pGlobals->puszTextBuffer, puszString, pGlobals->nMaxTextBuffer );
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCreditTypeGetFormatting(
	CREDIT_TYPE eCurrentType,
	FONTCOLOR *peFontColor,
	int *pnNumIndentation)
{
	switch (eCurrentType)
	{
	
		//----------------------------------------------------------------------------				
		case CT_SECTION:
		{
			*peFontColor = FONTCOLOR_CREDITS_SECTION;
			break;
		}
			
		//----------------------------------------------------------------------------								
		case CT_TITLE:
		{
			// indentations
			*pnNumIndentation = 1;
			
			// color formatting
			*peFontColor = FONTCOLOR_CREDITS_TITLE;
			break;
		}
		
		//----------------------------------------------------------------------------				
		case CT_NAME:
		default:
		{
			// indentations
			*pnNumIndentation = 2;
			
			// color formatting
			*peFontColor = FONTCOLOR_CREDITS_CREDIT;
			break;
			
		}
		
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAddCreditLineToTextBuffer(
	const CREDIT_LINE *pLine,
	BOOL bDoNewLines = TRUE,
	BOOL bAppendContinued = FALSE)
{

	// ignore pagebreak formatters
	if (pLine->eType == CT_PAGEBREAK)
	{
		return;
	}
				
	// add newlines before string
	if (bDoNewLines)
	{
		for (int i = 0; i < pLine->nNumNewLinesBeforeString; ++i)
		{
			sCreditTextBufferAddString( sgpuszNewLine );
		}
	}
	
	// do some simplistic formatting
	FONTCOLOR eFontColor = FONTCOLOR_WHITE;
	int nNumIndentation = 0;
	sCreditTypeGetFormatting( pLine->eType, &eFontColor, &nNumIndentation );

	// add indentations
	for (int i = 0; i < nNumIndentation; ++i)
	{
		sCreditTextBufferAddString( sgpuszIndent );
	}

	// copy the string
	WCHAR uszString[ MAX_STRING_ENTRY_LENGTH ];	
	PStrCopy( uszString, pLine->puszString, MAX_STRING_ENTRY_LENGTH );
	
	// append continued if we want to
	if (bAppendContinued)
	{
		const WCHAR *puszContinued = GlobalStringGet( GS_CONTINUED_PAREN );
		if (puszContinued && puszContinued[ 0 ] != 0)
		{
			
			// add a space if we don't have one
			if (puszContinued[ 0 ] != L' ')
			{
				PStrCat( uszString, L" ", MAX_STRING_ENTRY_LENGTH );
			}
			
			// append text
			PStrCat( uszString, puszContinued, MAX_STRING_ENTRY_LENGTH );
			
		}
		
	}


	// color the string
	WCHAR uszStringColored[ MAX_STRING_ENTRY_LENGTH ];	
	uszStringColored[ 0 ] = 0;
	UIColorCatString( uszStringColored, MAX_STRING_ENTRY_LENGTH, eFontColor, uszString );
	
	// add the string buffer
	sCreditTextBufferAddString( uszStringColored );

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void static sInitMoviePool(
	CREDIT_GLOBALS *pGlobals)
{
	ASSERTX_RETURN( pGlobals, "Expected credit globals" );

	// clear pool	
	pGlobals->nNumMovieBackgroundPool = 0;	
	for (int i = 0; i < MAX_CREDIT_BACKGROUNDS; ++i)
	{
		pGlobals->nMovieBackgroundPool[ i ] = INVALID_LINK;
	}

	// are we running in wide screen
	//BOOL bWidescreen = g_UI.m_bWidescreen;

	// scan the movie table
	int nNumMovies = ExcelGetNumRows( EXCEL_CONTEXT(), DATATABLE_MOVIES );
	for (int nMovie = 0; nMovie < nNumMovies; ++nMovie)
	{
		const MOVIE_DATA *pMovieData = MovieGetData( nMovie );
		if (pMovieData)
		{
		
			// only consider movies that are for credits
			if (pMovieData->bUseInCredits)
			{
			
				// must match widescreen
//				if (bWidescreen == pMovieData->bWidescreen)
				{
													
					// ok, add it to array
					ASSERTX_CONTINUE( pGlobals->nNumMovieBackgroundPool <= MAX_CREDIT_BACKGROUNDS - 1, "Too many credit movies" );
					pGlobals->nMovieBackgroundPool [ pGlobals->nNumMovieBackgroundPool++ ] = nMovie;
					
				}

			}
			
		}
		
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sGetCreditsSourceStringFile(
	void)
{
	int nNumFiles = ExcelGetNumRows( EXCEL_CONTEXT(), DATATABLE_STRING_FILES );
	for (int i = 0; i < nNumFiles; ++i)
	{
		const STRING_FILE_DEFINITION *pStringFileDef = StringFileDefinitionGet( i );
		if (pStringFileDef && pStringFileDef->bCreditsFile)
		{
			return i;	
		}
	}
	return INVALID_LINK;	
}

//----------------------------------------------------------------------------
struct CREDIT_KEY
{
	const char *pszKey;
	int nStringNumber;
};

//----------------------------------------------------------------------------
struct ALL_CREDIT_KEYS
{
	CREDIT_KEY tKey[ MAX_CREDIT_LINES ];
	int nNumKeys;
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAddKey(
	const char *pszKey,
	WORD wStringNumberInSourceFile,
	void *pCallbackData)
{
	ALL_CREDIT_KEYS *pAllKeys = (ALL_CREDIT_KEYS *)pCallbackData;
	
	// add to all keys
	ASSERTX_RETURN( pAllKeys->nNumKeys <= MAX_CREDIT_LINES - 1, "Too many credit lines" );
	CREDIT_KEY *pCreditKey = &pAllKeys->tKey[ pAllKeys->nNumKeys++ ];
	pCreditKey->pszKey = pszKey;
	pCreditKey->nStringNumber = wStringNumberInSourceFile;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sCompareCreditKey(
	const void * a,
	const void * b)
{
	CREDIT_KEY * A = (CREDIT_KEY *)a;
	CREDIT_KEY * B = (CREDIT_KEY *)b;

	if (A->nStringNumber < B->nStringNumber)
	{
		return -1;
	}
	else if (A->nStringNumber > B->nStringNumber)
	{
		return 1;
	}
	return 0;
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sGetStringKeys( 
	ALL_CREDIT_KEYS *pAllKeys)
{

	// which string source file is the credit file
	int nStringFileCredits = sGetCreditsSourceStringFile();
	ASSERTX_RETURN( nStringFileCredits != INVALID_LINK, "No credit source file" );
		
	// iterate all the keys
	StringTableCommonIterateKeys( LanguageGetCurrent(), nStringFileCredits, sAddKey, pAllKeys );

	// sort all the keys
	qsort(pAllKeys, pAllKeys->nNumKeys, sizeof( CREDIT_KEY ), sCompareCreditKey );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sHasCreditTypeToken( 
	const WCHAR *puszString, 
	const WCHAR *puszToken)
{
	ASSERTX_RETFALSE( puszString, "Expected string" );
	ASSERTX_RETFALSE( puszToken, "Expected token string" );
	
	int nTokenLen = PStrLen( puszToken );
	return PStrCmp( puszString, puszToken, nTokenLen ) == 0;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sSKUIsIncluded( 
	CREDIT_GLOBALS *pGlobals, 
	int nSKU)
{
	ASSERTX_RETFALSE( pGlobals, "Expected globals" );
	ASSERTX_RETFALSE( nSKU != INVALID_LINK, "Expected SKU link" );
	
	for (int i = 0; i < pGlobals->nNumSKUIncluded; ++i)
	{
		if (pGlobals->nSKUIncluded[ i ] == nSKU)
		{
			return TRUE;
		}
	}
	
	return FALSE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sDoSKUOperation(
	CREDIT_GLOBALS *pGlobals,
	CREDIT_TYPE eType,
	int nSKU)
{
	ASSERTX_RETURN( pGlobals, "Expected globals" );
	ASSERTX_RETURN( nSKU != INVALID_LINK, "Invalid SKU link" );
	
	if (eType == CT_SKU_ADD)
	{
		BOOL bFound = FALSE;
		for (int i = 0; i < pGlobals->nNumSKUIncluded; ++i)
		{
			if (pGlobals->nSKUIncluded[ i ] == nSKU)
			{
				bFound = TRUE;
				break;
			}
		}
		if (bFound == FALSE)
		{
			ASSERTX_RETURN( pGlobals->nNumSKUIncluded <= MAX_CREDIT_SKUS - 1, "Too many SKUs" );
			pGlobals->nSKUIncluded[ pGlobals->nNumSKUIncluded++ ] = nSKU;
		}
	}
	else if (eType == CT_SKU_REMOVE)
	{
		for (int i = 0; i < pGlobals->nNumSKUIncluded; ++i)
		{
			if (pGlobals->nSKUIncluded[ i ] == nSKU)
			{
				if (pGlobals->nNumSKUIncluded > 1)
				{
					pGlobals->nSKUIncluded[ i ] = pGlobals->nSKUIncluded[ pGlobals->nNumSKUIncluded - 1 ];
				}
				else
				{
					pGlobals->nSKUIncluded[ i ] = INVALID_LINK;
				}
				
				pGlobals->nNumSKUIncluded--;
				
			}
		}
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sSKUOperation( 
	CREDIT_GLOBALS *pGlobals, 
	CREDIT_TYPE eType,
	const WCHAR *puszToken)
{
	ASSERTX_RETURN( pGlobals, "Expected globals" );
	ASSERTX_RETURN( puszToken, "Expected globals" );
	
	if (puszToken[ 0 ])
	{
		
		if (PStrICmp( puszToken, L"all" ) == 0)
		{
			int nNumSKUs = ExcelGetNumRows( EXCEL_CONTEXT(), DATATABLE_SKU );
			for (int i = 0; i < nNumSKUs; ++i)
			{
				sDoSKUOperation( pGlobals, eType, i );
			}
		}
		else
		{
			const int MAX_TOKEN = 128;
			char szToken[ MAX_TOKEN ];
			PStrCvt( szToken, puszToken, MAX_TOKEN );
			int nSKU = ExcelGetLineByStringIndex( EXCEL_CONTEXT(), DATATABLE_SKU, szToken );
			if (nSKU != INVALID_LINK)
			{
				sDoSKUOperation( pGlobals, eType, nSKU );
			}
			
		}
		
	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCreditGlobalsInit(
	void)
{
	
	// free credits if it's already initialized
	if (sgbCreditsInitialized == TRUE)
	{
		sCreditGlobalsFree();
	}

	// must be in an un-initialized state
	ASSERTX_RETURN( sgbCreditsInitialized == FALSE, "Credit globals already	initialized" );
	
	// get globals
	CREDIT_GLOBALS *pGlobals = sCreditGlobalsGet( FALSE );  // bypass init validation since we're initializing
	ASSERTX_RETURN( pGlobals, "Expected credit globals" );
			
	// init page style variables
	pGlobals->nNumCreditLines = 0;
	pGlobals->nCurrentLine = 0;
	pGlobals->pCurrentSection = NULL;
	pGlobals->pCurrentTitle = NULL;
	pGlobals->dwPageEvent = 0;
	pGlobals->dwMusicEvent = 0;
	pGlobals->nMovieCurrent = INVALID_LINK;
	pGlobals->tiMovieCurrentShowTime = 0;
	pGlobals->bAdvanceTextAtFadeOut = FALSE;
	pGlobals->nNumSKUIncluded = 0;
	pGlobals->nMusic = INVALID_ID;
	
	// init movies we have to choose from
	sInitMoviePool( pGlobals );

	// start with all skus
	sSKUOperation( pGlobals, CT_SKU_ADD, L"all" );
		
	// clear comps of interest
	UILabelSetTextByEnum( UICOMP_LABEL_CREDITS_SCROLLER, L"" );
	UILabelSetTextByEnum( UICOMP_LABEL_CREDITS_TEXT_PAGE, L"" );
	
	// allocate a buffer for the text
	pGlobals->nMaxTextBuffer = KILO * 256;
	pGlobals->puszTextBuffer = (WCHAR *)GMALLOCZ( NULL, pGlobals->nMaxTextBuffer * sizeof( WCHAR ));

	// add a few new lines so we're not starting at the very top of the screen
	sCreditTextBufferAddString( L"\n\n\n\n\n\n\n\n\n\n\n" );

	// get all the string keys of the credits in order they are to appear
	ALL_CREDIT_KEYS tAllKeys;
	tAllKeys.nNumKeys = 0;
	sGetStringKeys( &tAllKeys );

	// go through all keys
	CREDIT_TYPE eLastType = CT_INVALID;	
	for (int i = 0; i < tAllKeys.nNumKeys; ++i)
	{
		const CREDIT_KEY *pCreditKey = &tAllKeys.tKey[ i ];
		
		// get string
		BOOL bFound = FALSE;
		DWORD dwAttributes = 0;
		const WCHAR *puszString = StringTableGetStringByKeyVerbose(
			pCreditKey->pszKey,
			0,
			0,
			&dwAttributes,
			&bFound);

		// get properties of this string
		CREDIT_TYPE eCurrentType = CT_INVALID;
		if (sHasCreditTypeToken( puszString, TOKEN_SECTION ))
		{
			eCurrentType = CT_SECTION;
			puszString = puszString + PStrLen( TOKEN_SECTION );  // remove token from string
		}
		else if (sHasCreditTypeToken( puszString, TOKEN_TITLE ))
		{
			eCurrentType = CT_TITLE;
			puszString = puszString + PStrLen( TOKEN_TITLE );  // remove token from string
		}
		else if (sHasCreditTypeToken( puszString, TOKEN_PAGEBREAK ))
		{
			eCurrentType = CT_PAGEBREAK;
			puszString = puszString + PStrLen( TOKEN_PAGEBREAK );  // remove token from string
		}
		else if (sHasCreditTypeToken( puszString, TOKEN_FONT_SIZE ))
		{
			eCurrentType = CT_FONT_SIZE;
			puszString = puszString + PStrLen( TOKEN_FONT_SIZE );  // remove token from string
		}
		else if (sHasCreditTypeToken( puszString, TOKEN_SKU_ADD ))
		{
			eCurrentType = CT_SKU_ADD;
			puszString = puszString + PStrLen( TOKEN_SKU_ADD );  // remove token from string
		}
		else if (sHasCreditTypeToken( puszString, TOKEN_SKU_REMOVE ))
		{
			eCurrentType = CT_SKU_REMOVE;
			puszString = puszString + PStrLen( TOKEN_SKU_REMOVE );  // remove token from string
		}
		else
		{
			eCurrentType = CT_NAME;
		}

		// track sku changes
		if (eCurrentType == CT_SKU_ADD || eCurrentType == CT_SKU_REMOVE)
		{
			sSKUOperation( pGlobals, eCurrentType, puszString );
			continue;
		}
		
		// ignore lines that are not included in this SKU
		int nSKUCurrent = SKUGetCurrent();
		if (sSKUIsIncluded( pGlobals, nSKUCurrent ) == FALSE)
		{
			continue;
		}
		
		// how many newlines should we put in before this line
		int nNumNewLinesBeforeLine = 0;
		switch (eCurrentType)
		{
			case CT_PAGEBREAK:
			case CT_FONT_SIZE:
			case CT_SKU_ADD:
			case CT_SKU_REMOVE:
				nNumNewLinesBeforeLine = 0;
				break;
			case CT_SECTION:
				nNumNewLinesBeforeLine = 3;
				break;
			case CT_TITLE:
				nNumNewLinesBeforeLine = 2;
				break;
			case CT_NAME:
			default:
				nNumNewLinesBeforeLine = 1;
				break;					
		}			

		// add to the credit lines
		ASSERTX_RETURN( pGlobals->nNumCreditLines <= MAX_CREDIT_LINES - 1, "Too many credit lines" );
		CREDIT_LINE *pLine = &pGlobals->tCreditLine[ pGlobals->nNumCreditLines++ ];
		pLine->eType = eCurrentType;
		pLine->puszString = puszString;
		pLine->nNumNewLinesBeforeString = nNumNewLinesBeforeLine;

		// if our style is scrolling, format the text right now, otherwise just setup the lines
		if (sgeCreditDisplayStyle == CDS_SCROLL)
		{
			sAddCreditLineToTextBuffer( pLine );
		}
						
		// save this as the last type of credit
		eLastType = eCurrentType;
		
	}

	// credits are now initialized
	sgbCreditsInitialized = TRUE;
			
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCreditsStartScroller(
	 void)
{
	CREDIT_GLOBALS *pGlobals = sCreditGlobalsGet();
	ASSERTX_RETURN( pGlobals, "Expected credit globals" );

	// set the text into the scroller
	UI_COMPONENT *pCompCreditTextScroller = UIComponentGetByEnum( UICOMP_LABEL_CREDITS_SCROLLER );
	if (pCompCreditTextScroller)
	{
		UILabelSetText( pCompCreditTextScroller, pGlobals->puszTextBuffer );
	}

	// maybe set a timer to see when we're finished and auto exit?
	// TODO: <----- write me!
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCreditLineCanStartPage(
	const CREDIT_LINE *pLine)
{
	ASSERTX_RETFALSE( pLine, "Expected credit line" );
	switch (pLine->eType)
	{
		case CT_SECTION:	return TRUE;
		case CT_TITLE:		return TRUE;
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCreditPageSetupNext(
	CREDIT_GLOBALS *pGlobals,
	BOOL *pbDone,
	int *pnNumLinesOnPage,
	int *pnNumCharacterGroupsOnPage)
{
	ASSERTX_RETFALSE( pGlobals, "Expected credit globals" );
	ASSERTX_RETFALSE( pnNumLinesOnPage, "Expected num lines on page" );
	ASSERTX_RETFALSE( pnNumCharacterGroupsOnPage, "Expected num character groups on page" );
	
	// init to not being done
	*pbDone = FALSE;
	*pnNumLinesOnPage = 0;
	*pnNumCharacterGroupsOnPage = 0;

	// if we have no more lines, we're done and bail out
	if (pGlobals->nCurrentLine >= pGlobals->nNumCreditLines)
	{
		*pbDone = TRUE;
	}
	else
	{
	
		// setup how many lines we can display on a page
		int nLineStart = pGlobals->nCurrentLine;
		int nLineEnd = nLineStart;
		int nNumLinesLeft = CREDIT_MAX_LINES_PER_PAGE;
		int nLastValidPageStart = pGlobals->nCurrentLine;
		BOOL bDone = FALSE;
		int nNumContinuousNames = 0;
		BOOL bFirstLine = TRUE;
		while (!bDone)
		{

			// stop if there are no more lines
			if (pGlobals->nCurrentLine >= pGlobals->nNumCreditLines)
			{
				break;
			}
			
			// get the next line
			const CREDIT_LINE *pLine = &pGlobals->tCreditLine[ pGlobals->nCurrentLine ];
			
			// track continuous names
			if (pLine->eType == CT_NAME)
			{
				nNumContinuousNames++;
			}
			else
			{
				nNumContinuousNames = 0;
			}
			
			// if this is a validate place to start a page, record that
			if (sCreditLineCanStartPage( pLine ) ||
				(pLine->eType == CT_NAME && nNumContinuousNames >= (CREDIT_MAX_LINES_PER_PAGE / 2)))
			{
				nLastValidPageStart = pGlobals->nCurrentLine;
			}

			// advance the line
			pGlobals->nCurrentLine++;
			nLineEnd++;
			
			// if we encounter a hard page break, eat it and we're done
			if (pLine->eType == CT_PAGEBREAK)
			{
				bDone = TRUE;
				break;
			}
			
			// how many lines does this line count as
			int nLinesRequired = pLine->nNumNewLinesBeforeString;

			// some line types force a page break
			BOOL bForcePageBreak = FALSE;
			if (bFirstLine == FALSE && pLine->eType == CT_SECTION)
			{
				bForcePageBreak = TRUE;
			}
						
			// will this line fit onto the existing page
			if (nLinesRequired > nNumLinesLeft || bForcePageBreak == TRUE)
			{
			
				// if this line cannot start the next page, back up to a line that can
				if (nLastValidPageStart != pGlobals->nCurrentLine)
				{
					pGlobals->nCurrentLine = nLastValidPageStart;
					nLineEnd = nLastValidPageStart;
					nNumLinesLeft = 0;
				}

				// don't do anymore
				bDone = TRUE;
									
			}
			else
			{
				nNumLinesLeft -= nLinesRequired;
			}
			
			bFirstLine = FALSE;
			
		}
		ASSERTX( nLineStart != nLineEnd, "Credit line start is same as line end" );
		
		// construct text buffer
		int nFontSize = NONE;
		bFirstLine = TRUE;
		pGlobals->puszTextBuffer[ 0 ] = 0;
		for (int i = nLineStart; i < nLineEnd; ++i)
		{
		
			// get line
			const CREDIT_LINE *pLine = &pGlobals->tCreditLine[ i ];

			// do font size parsing
			if (pLine->eType == CT_FONT_SIZE && pLine->puszString[ 0 ])
			{
				const int MAX_TOKEN = 32;
				char szToken[ MAX_TOKEN ];
				PStrCvt( szToken, pLine->puszString, MAX_TOKEN );
				nFontSize = atoi( szToken );
				continue;
			}
			
			// track section changes
			if (pLine->eType == CT_SECTION)
			{
				pGlobals->pCurrentSection = pLine;
			}
			
			// track title changes
			if (pLine->eType == CT_TITLE)
			{
				pGlobals->pCurrentTitle = pLine;
			}

			// skip lines with no text
			if (pLine->puszString == NULL || pLine->puszString[ 0 ] == 0)
			{
				continue;
			}

			// when starting a page we have some logic to run			
			if (bFirstLine == TRUE)
			{
			
				// always add the current section at the top			
				if (pGlobals->pCurrentSection != NULL)
				{
				
					// note we don't put newline ahead of it
					sAddCreditLineToTextBuffer( pGlobals->pCurrentSection, FALSE );
					
					// always add a couple of new lines after
					sCreditTextBufferAddString( sgpuszNewLine );
					sCreditTextBufferAddString( sgpuszNewLine );

				}

				// if this line is a name, it means that we split the last section of names
				// and that we should put a "continued" title here
				if (pLine->eType == CT_NAME)
				{
				
					if (pGlobals->pCurrentTitle != NULL)
					{
						sAddCreditLineToTextBuffer( pGlobals->pCurrentTitle, TRUE, TRUE );
					}
					
				}
				
			}
						
			// add to buffer
			if (pLine != pGlobals->pCurrentSection)
			{
				sAddCreditLineToTextBuffer( pLine );
			}

			// we have written the first line
			bFirstLine = FALSE;
						
		}

		// set the text into a control
		UI_COMPONENT *pComponent = UIComponentGetByEnum( UICOMP_LABEL_CREDITS_TEXT_PAGE );
		if (pComponent)
		{
		
			// set new font size if specified
			if (nFontSize != NONE)
			{
				pComponent->m_nFontSize = nFontSize;
			}
			
			// set text
			UILabelSetText( pComponent, pGlobals->puszTextBuffer );		
			
			// start fade in
			sCreditUIFade(TRUE);
			
		}
		
		// save how many lines were setup on this page
		*pnNumLinesOnPage = nLineEnd - nLineStart;

		// save how many character groups are on this page
		int nNumCharactersOnPage = PStrLen(	pGlobals->puszTextBuffer );
		*pnNumCharacterGroupsOnPage = nNumCharactersOnPage / (CREDIT_NUM_CHARACTERS_PER_DISPLAY_TIME_FACTOR + 1);

	}
	
	return TRUE;  // success
			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCreditPageAdvanceText(
	GAME *pGame,
	const CEVENT_DATA &tEventData,
	DWORD)
{

	// forget it if not displaying credits anymore
	if (sgbCreditsInitialized == FALSE)
	{
		return;
	}

	// get globals
	CREDIT_GLOBALS *pGlobals = sCreditGlobalsGet();
	ASSERTX_RETURN( pGlobals, "Expected credit globals" );

	// setup the next page of text
	BOOL bDone = TRUE;
	int nNumLinesOnPage = 0;
	int nNumCharacterGroupsOnPage = 0;
	if (sCreditPageSetupNext( pGlobals, &bDone, &nNumLinesOnPage, &nNumCharacterGroupsOnPage ) == FALSE)
	{
		bDone = TRUE;
	}
	
	// if we're done, bail out
	if (bDone == TRUE)
	{
		sCreditsEnable( FALSE );
	}
	else
	{
	
		// setup an event to go to the next credit page in a little while
		if (gbAutoAdvancePages)
		{
			DWORD dwDelayInMS = (DWORD)((float)(CREDIT_CHARACTER_GROUP_DISPLAY_TIME_FACTOR * nNumCharacterGroupsOnPage) * MSECS_PER_SEC);
			DWORD dwMinDelayInMS = (DWORD)(CREDIT_PAGE_MIN_DISPLAY_TIME_IN_SECONDS * MSECS_PER_SEC);
			if (dwDelayInMS < dwMinDelayInMS)
			{
				dwDelayInMS = dwMinDelayInMS;
			}
			CEVENT_DATA tEventData;
			pGlobals->dwPageEvent = CSchedulerRegisterEvent( AppCommonGetCurTime() + dwDelayInMS, sCreditPageFadeOut, tEventData);
		}
		
	}
			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sCreditPageFadeOut(
	GAME *pGame,
	const CEVENT_DATA &tEventData,
	DWORD)
{

	// forget it if not displaying credits anymore
	if (sgbCreditsInitialized == FALSE)
	{
		return;
	}

	// get globals
	CREDIT_GLOBALS *pGlobals = sCreditGlobalsGet();
	ASSERTX_RETURN( pGlobals, "Expected credit globals" );

	// fade out all the visible credit components
	sCreditUIFade(FALSE);

	// is it time to change the movie
	BOOL bChangeMovie = FALSE;
	if (pGlobals->nMovieCurrent != INVALID_LINK && pGlobals->nNumMovieBackgroundPool > 1)
	{
		const MOVIE_DATA *pMovieData = MovieGetData( pGlobals->nMovieCurrent );
		TIME tiNow = AppCommonGetCurTime();
		DWORD dwSecondsElapsed = (DWORD)((tiNow - pGlobals->tiMovieCurrentShowTime) / MSECS_PER_SEC);
		if ((float)dwSecondsElapsed >= pMovieData->flCreditMovieDisplayTimeInSeconds)
		{
			bChangeMovie = TRUE;
		}
	}

	// set an event to advance the text or background after all components are hidden
	DWORD dwDelayInMS = (DWORD)(MSECS_PER_SEC * PAGE_FADE_OUT_TIME_IN_SECONDS);
	CEVENT_DATA tEventDataNext;
	if (bChangeMovie)
	{
		tEventDataNext.m_Data1 = TRUE;  // advance the text after changing movie
		pGlobals->dwPageEvent = CSchedulerRegisterEvent( AppCommonGetCurTime() + dwDelayInMS, sCreditsChangeMovie, tEventDataNext );
	}
	else 
	{
		pGlobals->dwPageEvent = CSchedulerRegisterEvent( AppCommonGetCurTime() + dwDelayInMS, sCreditPageAdvanceText, tEventDataNext );	
	}	
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCreditsStartPages(
	void)
{
	CREDIT_GLOBALS *pGlobals = sCreditGlobalsGet();
	ASSERTX_RETURN( pGlobals, "Expected credit globals" );

	// make certain we're at the beginning of the text
	ASSERTX( pGlobals->nCurrentLine == 0, "Current credit line not at start" );
	pGlobals->nCurrentLine = 0;

	// start at the last section if desired
	if (sgbStartAtLastSection)
	{
		int nLastSectionIndex = NONE;
		
		for (int i = 0; i < pGlobals->nNumCreditLines; ++i)
		{
			const CREDIT_LINE *pLine = &pGlobals->tCreditLine[ i ];
			if (pLine->eType == CT_SECTION)
			{
				nLastSectionIndex = i;
			}
		}		
		
		if (nLastSectionIndex != NONE)
		{
			pGlobals->nCurrentLine = nLastSectionIndex;
		}
		
	}
		
	// call the function to advance to the next page
	CEVENT_DATA tEventData;
	sCreditPageAdvanceText( NULL, tEventData, 0 );
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCreditsPresent( 
	GAME *pGame, 
	const CEVENT_DATA &tEventData, 
	DWORD)
{

	// set label text for scroller style
	switch (sgeCreditDisplayStyle)
	{
	
		//----------------------------------------------------------------------------
		case CDS_SCROLL:
		{
			sCreditsStartScroller();
			break;
		}
			
		//----------------------------------------------------------------------------			
		case CDS_PAGE:
		{
			sCreditsStartPages();
			break;
		}
			
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCreditsEnable(
	BOOL bEnable)
{
	// activate/deactivate the credits panel
	UIComponentActivate( UICOMP_PANEL_CREDITS, bEnable, bEnable ? 300 : 0 );

	// begin
	if (bEnable)
	{

		// clear any auto enter from the main menu we had setup (like when we beat the game)
		sgbCreditsAutoEnterFromMainMenu = FALSE;

		// hide loading screen if it's up
		UIHideLoadingScreen( FALSE );
		
		// hide the main menu
		UIHideMainMenu();
	
		// init credit globals
		sCreditGlobalsInit();

		// get globals
		CREDIT_GLOBALS *pGlobals = sCreditGlobalsGet();
		ASSERTX_RETURN( pGlobals, "Expected credit globals" );		

		// change the movie, do not advance text yet
		CEVENT_DATA tEventDataMovie;
		tEventDataMovie.m_Data1 = FALSE;  // do not advance the text
		sCreditsChangeMovie( NULL, tEventDataMovie, 0 );	

		// start showing the credits after a small delay
		DWORD dwDelayInMS = (DWORD)(CREDIT_PAGE_INITIAL_DELAY_TIME_IN_SECONDS * MSECS_PER_SEC);
		CEVENT_DATA tEventData;
		pGlobals->dwPageEvent = CSchedulerRegisterEvent( AppCommonGetCurTime() + dwDelayInMS, sCreditsPresent, tEventData );		

		// play the credits music
		pGlobals->nMusic = sCreditsMusicPlay(0);
		c_MusicStop();
	}
	else
	{

		// free credit globals
		sCreditGlobalsFree();
			
		// show the main menu
		UIShowMainMenu( DoneMainMenu );

		// fade the screen in incase we were faded out during a transition
		V( e_BeginFade( FADE_IN, SCREEN_FADE_TIME_MS, 0x00000000 ) );
	
		// play the main menu music
		AppPlayMenuMusic();
		
	}

	// start the proper background movie
	AppStartOrStopBackgroundMovie();
					
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIButtonCreditsShow(
	UI_COMPONENT *pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{
	sCreditsEnable( TRUE );
	return UIMSG_RET_NOT_HANDLED;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UI_MSG_RETVAL UIButtonCreditsExit(
	UI_COMPONENT *pComponent,
	int nMessage,
	DWORD wParam,
	DWORD lParam)
{

	// for debuggin, if auto advance pages is off the exit button will advance the page
	if (gbAutoAdvancePages == FALSE)
	{
		CEVENT_DATA tEventData;
		sCreditPageFadeOut( NULL, tEventData, 0 );
	}
	else
	{
		sCreditsEnable( FALSE );
	}
	
	return UIMSG_RET_NOT_HANDLED;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_CreditsTryAutoEnter(
	void)
{

	if (sgbCreditsAutoEnterFromMainMenu == TRUE)
	{
		sCreditsEnable( TRUE );					
	}
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_CreditsSetAutoEnter(
	BOOL bAutoEnter)
{
	sgbCreditsAutoEnterFromMainMenu = bAutoEnter;
}	

#endif  // !ISVERSION(SERVER_VERSION)