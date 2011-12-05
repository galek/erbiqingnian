//----------------------------------------------------------------------------
// Prime v2.0
//
// c_movieplayer.cpp
// 
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"

#if defined(TUGBOAT)
#define BINK_ENABLED 0
#else
#define BINK_ENABLED 1
#endif

#if !ISVERSION(SERVER_VERSION)
#include "c_movieplayer.h"
#include "c_sound_util.h"
#include "e_settings.h"
#include "prime.h"
#include "excel_private.h"
#include "excel.h"
#include "pakfile.h"
#include "stringtable.h"
#include "language.h"
#include "uix.h"
#include "region.h"
#include "sku.h"
#include "gameoptions.h"
#include "globalindex.h"
#include "settings.h"

#if BINK_ENABLED
#include "Bink.h"
#include "mss.h"
#include "e_movie.h"

#define MEMORY_ROUTINES_ENABLED
#define MILES_ENABLED

using namespace FSSE;

static int sgnCurrentSubtitle = 0;
static SIMPLE_DYNAMIC_ARRAY<MOVIE_SUBTITLE_DATA*> tMovieSubtitleList;

//----------------------------------------------------------------------------
struct GAME_MOVIE
{
	HBINK pMovie;
	int nMovieIndex;
};

// For Bink movie player
static GAME_MOVIE sgtGameMovie;
static MOVIE_HANDLE sghEngineMovie = NULL;
static BOOL sgbMovieLoops = FALSE;
static BOOL sgbMoviePaused = FALSE;
static BOOL sgbMoviePausedUserControl = FALSE;

// For @#$@%#! miles sound player (which is the only way to get 5.1 out of Bink - bah)
static HDIGDRIVER sgpMiles = NULL;

// Override - if this is FALSE then we NEVER do 5.1.  If it's TRUE then we respect the speaker config
static BOOL sgbSystemSupportsFiveDotOne = TRUE;

// Pakfile handle for reading from the Pak file
static ASYNC_FILE * pPakFileHandle = NULL;

static BOOL sgbMemoryFunctionInitialized = FALSE;

static MOVIE_CONFIG_OPTIONS sgtOptions;
static BOOL sgbAllowSkippingOfFirstTimeMovies = FALSE;

static BOOL sgbMovieSystemInitialized = FALSE;

BOOL gbForceLowQualityMovies = FALSE;

//----------------------------------------------------------------------------
// Memory Management Routines
// I'm obliged to return memory aligned along 32-byte boundaries.
// So, in order to get this to happen properly, I have to allocate an extra
// chunk of RAM, and return an aligned pointer.  But, that aligned pointer
// will be returned back to me in the FREE call.  So, I have to store the
// actual pointer somewhere well-known.  Here's the option I've selected
// 2.) Store the allocated address in the eight bytes prior to the returned
//     address.  This has the added complication that I have to ensure that I
//     have sufficient "blank" space before the returned pointer
//----------------------------------------------------------------------------
#ifdef MEMORY_ROUTINES_ENABLED
#define BINK_MEMORY_ALIGNMENT 32
#define BINK_MEMORY_ALIGNMENT_MASK (~31)
void PTR4* RADLINK c_MoviePlayerAlloc(
	U32 num_bytes)
{
#if USE_MEMORY_MANAGER
	return ALIGNED_MALLOC(g_ScratchAllocator, num_bytes, BINK_MEMORY_ALIGNMENT);
#else
	//*
	//trace("%d Bytes requested\n", num_bytes);
	int nBonusBytes = BINK_MEMORY_ALIGNMENT + sizeof(DWORD_PTR);
	int nNumBytesToActuallyAllocate = num_bytes + nBonusBytes;
	void * pPtr = MALLOCBIG(NULL, nNumBytesToActuallyAllocate);
	DWORD_PTR dwMemoryLocation = (DWORD_PTR)pPtr;
	dwMemoryLocation += nBonusBytes;
	dwMemoryLocation &= BINK_MEMORY_ALIGNMENT_MASK;
	DWORD_PTR * pdwActualPointer = (DWORD_PTR*)(dwMemoryLocation - sizeof(DWORD_PTR));
	*pdwActualPointer = (DWORD_PTR)pPtr;

	return (void*)dwMemoryLocation;
#endif
}

void RADLINK c_MoviePlayerFree(
	void * ptr)
{
#if USE_MEMORY_MANAGER
	FREE(g_ScratchAllocator, ptr);
#else
	//*
	DWORD_PTR * dwActualPointer = (DWORD_PTR*)((DWORD_PTR)ptr - sizeof(DWORD_PTR));
	void * pActualPtr = (void*)(*dwActualPointer);
	FREE(NULL, pActualPtr);
#endif
}

#ifdef MILES_ENABLED
#define MILES_MEMORY_ALIGNMENT 16
#define MILES_MEMORY_ALIGNMENT_MASK (~15)
void FAR * AILCALLBACK c_MilesAlloc(
	UINTa size)
{
	return ALIGNED_MALLOC(NULL, (MEMORY_SIZE)size, 16);
}

void AILCALLBACK c_MilesFree(
	void FAR * ptr)
{
	FREE(NULL, ptr);
}
#endif
#endif // MEMORY_ROUTINES_ENABLED
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const MOVIE_DATA *MovieGetData(
	int nMovie)
{
	return (const MOVIE_DATA *)ExcelGetData( NULL, DATATABLE_MOVIES, nMovie );	
}	

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
static
void sMovieExchangeFunc(SettingsExchange & se, void * pContext)	// CHB 2007.01.16
{
	static_cast<MOVIE_CONFIG_OPTIONS *>(pContext)->Exchange(se);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_MoviePlayerAppInit()
{


#if BINK_ENABLED

	if(sgbMovieSystemInitialized)
	{
		return;
	}
#ifdef MEMORY_ROUTINES_ENABLED
	if(!sgbMemoryFunctionInitialized)
	{
		BinkSetMemory(c_MoviePlayerAlloc, c_MoviePlayerFree);
		sgbMemoryFunctionInitialized = TRUE;
		Settings_Load("Movies", &sMovieExchangeFunc, &sgtOptions);
		sgbAllowSkippingOfFirstTimeMovies = !!sgtOptions.bAllowSkippingOfFirstTimeLogoMoviesForEALegal;
		sgtOptions.bAllowSkippingOfFirstTimeLogoMoviesForEALegal = true;
		Settings_Save("Movies", &sMovieExchangeFunc, &sgtOptions);
		AIL_mem_use_malloc(c_MilesAlloc);
		AIL_mem_use_free(c_MilesFree);
	}
#endif // MEMORY_ROUTINES_ENABLED

#ifdef MILES_ENABLED
	if(!sgpMiles)
	{
		AIL_startup();

		if(c_SoundGetSupportsFiveDotOne())
		{
			sgpMiles = AIL_open_digital_driver( 44100, 16, MSS_MC_51_DISCRETE, 0 );
		}
		else
		{
			sgpMiles = AIL_open_digital_driver( 44100, 16, 2, 0 );
		}
	}

	if(sgpMiles)
	{
		BinkSoundUseMiles( sgpMiles );
	}
	else
#endif // MILES_ENABLED
	{
		BinkSoundUseDirectSound( c_SoundGetOutputHandle() );
		sgbSystemSupportsFiveDotOne = FALSE;
	}
	sgbMovieSystemInitialized = TRUE;
#endif // BINK_ENABLED

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_MoviePlayerInit()
{
#if BINK_ENABLED
					
	sgtGameMovie.pMovie = NULL;
	sgtGameMovie.nMovieIndex = INVALID_LINK;
	
	tMovieSubtitleList.Destroy();
	ArrayInit(tMovieSubtitleList, NULL, 50);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if BINK_ENABLED
static void c_sMoviePlayerCloseFile()
{
	UIShowSubtitle(L"");
	if(sgtGameMovie.pMovie)
	{
		BinkClose(sgtGameMovie.pMovie);
		sgtGameMovie.pMovie = NULL;
		sgtGameMovie.nMovieIndex = INVALID_LINK;
	}
	if(pPakFileHandle)
	{
		AsyncFileClose(pPakFileHandle);
		pPakFileHandle = NULL;
	}
	sgbMovieLoops = FALSE;
	sgbMoviePaused = FALSE;
	sgbMoviePausedUserControl = FALSE;
	V( e_MovieRemove( sghEngineMovie ) );
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_MoviePlayerClose()
{
#if BINK_ENABLED
	c_sMoviePlayerCloseFile();
	tMovieSubtitleList.Destroy();
#endif // BINK_ENABLED
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_MoviePlayerAppClose()
{

#if BINK_ENABLED
	if(!sgbMovieSystemInitialized)
	{
		return;
	}
#ifdef MILES_ENABLED
	if(sgpMiles)
	{
		AIL_close_digital_driver(sgpMiles);
		sgpMiles = NULL;
	}

	AIL_shutdown();
#endif // MILES_ENABLED
	sgbMovieSystemInitialized = FALSE;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if BINK_ENABLED
static void c_sMoviePlayerTraceError()
{
#if _DEBUG
	trace("Bink Error: %s\n", BinkGetError());
#endif
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if BINK_ENABLED
static int c_sMoviePlayerFillInSubtitleListCompare(
	const void * pLeft,
	const void * pRight)
{
	MOVIE_SUBTITLE_DATA ** pSubtitleLeft = (MOVIE_SUBTITLE_DATA **)pLeft;
	MOVIE_SUBTITLE_DATA ** pSubtitleRight = (MOVIE_SUBTITLE_DATA **)pRight;

	if((*pSubtitleLeft)->nStartTime < (*pSubtitleRight)->nStartTime)
	{
		return -1;
	}
	else if((*pSubtitleLeft)->nStartTime > (*pSubtitleRight)->nStartTime)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL c_sMoviePlayerIsSubtitleForMovie(
	const MOVIE_SUBTITLE_DATA * pMovieSubtitleData,
	int nMovie)
{
	ASSERT_RETFALSE(pMovieSubtitleData);
	for(int i=0; i<MAX_MOVIES_PER_SUBTITLE; i++)
	{
		if(pMovieSubtitleData->nMovie[i] == nMovie)
		{
			return TRUE;
		}
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void c_sMoviePlayerFillInSubtitleList(
	int nMovie)
{
	sgnCurrentSubtitle = 0;

	ArrayClear(tMovieSubtitleList);

	int nCurrentLanguage = LanguageGetCurrent();
	int nRowCount = ExcelGetNumRows(EXCEL_CONTEXT(AppGetCltGame()), DATATABLE_MOVIE_SUBTITLES);
	for(int i=0; i<nRowCount; i++)
	{
		MOVIE_SUBTITLE_DATA * pMovieSubtitleData = (MOVIE_SUBTITLE_DATA *)ExcelGetData(EXCEL_CONTEXT(AppGetCltGame()), DATATABLE_MOVIE_SUBTITLES, i);
		if(pMovieSubtitleData && pMovieSubtitleData->eLanguage == nCurrentLanguage && c_sMoviePlayerIsSubtitleForMovie(pMovieSubtitleData, nMovie))
		{
			ArrayAddItem(tMovieSubtitleList, pMovieSubtitleData);
		}
	}
	tMovieSubtitleList.QSort(c_sMoviePlayerFillInSubtitleListCompare);
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if BINK_ENABLED
//*
static PRESULT c_sMoviePlayerOpenAddToPak(
	const char * pszFileName,
	int nMovieIndex,
	PAK_ENUM ePak)
{
	BYTE buffer[sizeof(FILE_HEADER)];

	DECLARE_LOAD_SPEC(spec, pszFileName);
	spec.flags |= PAKFILE_LOAD_ADD_TO_PAK | PAKFILE_LOAD_IGNORE_BUFFER_SIZE | PAKFILE_LOAD_ADD_TO_PAK_NO_COMPRESS | PAKFILE_LOAD_GIANT_FILE;
	spec.buffer = buffer;
	spec.size = arrsize(buffer);
	spec.priority = ASYNC_PRIORITY_SOUNDS;
	spec.pakEnum = ePak;

#if ISVERSION(DEVELOPMENT)
	if(PakFileNeedsUpdate(spec))
	{
		// We need to add this to the pak.  Open the file off the disk, and add it to the pak
		sgtGameMovie.pMovie = BinkOpen(pszFileName, BINKSNDTRACK | BINKNOFRAMEBUFFERS);
		//PakFileLoad(spec);
	}
	else
#else
	// We do this here in non-development builds so that we parse out the strFilePath and strFileName get set properly.
	PakFileNeedsUpdate(spec);
#endif
	{
		// The file is already in the pak.  Get the handle and offset to pass to Bink
		if(pPakFileHandle)
		{
			AsyncFileClose(pPakFileHandle);
		}
		pPakFileHandle = PakFileGetAsyncFileCopy(spec);
		if(pPakFileHandle)
		{
			sgtGameMovie.pMovie = BinkOpen((const char *)AsyncFileGetHandle(pPakFileHandle), BINKSNDTRACK | BINKFILEHANDLE | BINKNOFRAMEBUFFERS);
		}
	}

	if ( !sgtGameMovie.pMovie )
		return E_FAIL;

	// save the movie index that we're playing
	sgtGameMovie.nMovieIndex = nMovieIndex;

	PRESULT pr = e_MovieCreate( sghEngineMovie, sgtGameMovie.pMovie );

	if ( FAILED(pr) )
	{
		trace( "Failed to create Bink movie engine resources!\n" );
	}
	if ( FAILED(pr) || pr == S_FALSE )
	{
		c_sMoviePlayerCloseFile();
		return pr;
	}

	V( e_MovieSetFullscreen( sghEngineMovie ) );

	ASSERT_RETFAIL( sgtGameMovie.pMovie );
	return S_OK;
}
// */
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL c_sMoviePlayerOpenFile(
	const MOVIE_DATA * pMovie,
	int nMovie,
	const char * pszFileName,
	BOOL bLowQuality)
{
#if BINK_ENABLED
	c_sMoviePlayerCloseFile();

	// L, C, R, Ls, Rs, LFE
	U32 TrackIDsToOpenFiveDotOne[ 6 ] = { 0, 1, 2, 3, 4, 5 };
	// L, R
	U32 TrackIDsToOpenStereo[ 2 ] = { 6, 7 };

	LANGUAGE eAudioLanguage = LanguageCurrentGetAudioLanguage( AppGameGet() );
	int nAudioIndex = 0;
	for(int i=0; i<MAX_LANGUAGES_PER_MOVIE; i++)
	{
		if(pMovie->eLanguages[i] == LANGUAGE_INVALID)
			break;
		if(pMovie->eLanguages[i] == eAudioLanguage)
		{
			nAudioIndex = i;
			break;
		}
	}

	const int NUM_AUDIO_TRACKS_PER_LANGUAGE = 8;
	int nTrackOffset = 0;
	if(nAudioIndex > 0)
	{
		nTrackOffset = nAudioIndex*NUM_AUDIO_TRACKS_PER_LANGUAGE;
		for(int i=0; i<6; i++)
		{
			TrackIDsToOpenFiveDotOne[i] += nTrackOffset;
		}
		for(int i=0; i<2; i++)
		{
			TrackIDsToOpenStereo[i] += nTrackOffset;
		}
	}

#ifdef MILES_ENABLED
	BOOL bSupportsMultiChannel = sgbSystemSupportsFiveDotOne && c_SoundGetSupportsFiveDotOne();
#else
	BOOL bSupportsMultiChannel = FALSE;
#endif // MILES_ENABLED
	U32 * pTrackIDsToOpen = NULL;
	int nTrackCount = 0;
	if(bSupportsMultiChannel)
	{
		pTrackIDsToOpen = TrackIDsToOpenFiveDotOne;
		nTrackCount = 6;
	}
	else
	{
		pTrackIDsToOpen = TrackIDsToOpenStereo;
		nTrackCount = 2;
	}
	BinkSetSoundTrack(nTrackCount, pTrackIDsToOpen);

	PAK_ENUM ePak = PAK_MOVIES;
	if(!pMovie->bPutInMainPak)
	{
		if(bLowQuality)
		{
			ePak = PAK_MOVIES_LOW;
		}
		else
		{
			ePak = PAK_MOVIES_HIGH;
		}
	}

	PRESULT pr = c_sMoviePlayerOpenAddToPak(pszFileName, nMovie, ePak);
	if ( pr == S_OK )
	{
		SOUND_CONFIG_OPTIONS tOptions;
		structclear(tOptions);
		c_SoundGetSoundConfigOptions(&tOptions);
		float fVolumeScale = tOptions.bMuteSound ? 0.0f : tOptions.fSoundVolume;
#ifdef MILES_ENABLED
		if(sgpMiles)
		{
			if(bSupportsMultiChannel)
			{
				// Apparently, the speaker configurations are thus:
				// 0 - Left
				// 1 - Right
				// 2 - Center
				// 3 - LFE
				// 4 - Left Surround
				// 5 - Right Surround
				// Our tracks are arranged thus:
				// 0 - Left
				// 1 - Center
				// 2 - Right
				// 3 - Left Surround
				// 4 - Right Surround
				// 5 - LFE
				// So:
				// Track 0 - Speaker 0
				// Track 1 - Speaker 2
				// Track 2 - Speaker 1
				// Track 3 - Speaker 4
				// Track 4 - Speaker 5
				// Track 5 - Speaker 3
				S32 volumes[6];
				memclear(volumes, 6 * sizeof(S32));
				volumes[0] = (S32)(fVolumeScale * 32768);
				BinkSetMixBinVolumes(sgtGameMovie.pMovie, 0 + nTrackOffset, 0, volumes, nTrackCount);
				memclear(volumes, 6 * sizeof(S32));
				volumes[2] = (S32)(fVolumeScale * 32768);
				BinkSetMixBinVolumes(sgtGameMovie.pMovie, 1 + nTrackOffset, 0, volumes, nTrackCount);
				memclear(volumes, 6 * sizeof(S32));
				volumes[1] = (S32)(fVolumeScale * 32768);
				BinkSetMixBinVolumes(sgtGameMovie.pMovie, 2 + nTrackOffset, 0, volumes, nTrackCount);
				memclear(volumes, 6 * sizeof(S32));
				volumes[4] = (S32)(fVolumeScale * 32768);
				BinkSetMixBinVolumes(sgtGameMovie.pMovie, 3 + nTrackOffset, 0, volumes, nTrackCount);
				memclear(volumes, 6 * sizeof(S32));
				volumes[5] = (S32)(fVolumeScale * 32768);
				BinkSetMixBinVolumes(sgtGameMovie.pMovie, 4 + nTrackOffset, 0, volumes, nTrackCount);
				memclear(volumes, 6 * sizeof(S32));
				volumes[3] = (S32)(fVolumeScale * 32768);
				BinkSetMixBinVolumes(sgtGameMovie.pMovie, 5 + nTrackOffset, 0, volumes, nTrackCount);
			}
			else
			{
				// Apparently, the speaker configurations are thus:
				// 0 - Left
				// 1 - Right
				// Our tracks are arranged thus:
				// 6 - Left
				// 7 - Right
				// So:
				// Track 6 - Speaker 0
				// Track 7 - Speaker 1
				//*
				S32 volumes[6];
				memclear(volumes, 6 * sizeof(S32));
				volumes[0] = (S32)(fVolumeScale * 32768);
				BinkSetMixBinVolumes(sgtGameMovie.pMovie, 6 + nTrackOffset, 0, volumes, nTrackCount);
				memclear(volumes, 6 * sizeof(S32));
				volumes[1] = (S32)(fVolumeScale * 32768);
				BinkSetMixBinVolumes(sgtGameMovie.pMovie, 7 + nTrackOffset, 0, volumes, nTrackCount);
				// */
			}
		}
#endif // MILES_ENABLED

		// Fill in the subtitle list:
		c_sMoviePlayerFillInSubtitleList(nMovie);
		sgbMovieLoops = pMovie->bLoops;
		sgbMoviePaused = FALSE;
		sgbMoviePausedUserControl = FALSE;

		return TRUE;
	}
	else if ( FAILED( pr ) )
	{
		c_sMoviePlayerTraceError();
	}
#endif
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL c_sMoviePlayerIsForSKU(
	const MOVIE_DATA * pMovie)
{
	int nCurrentSKU = SKUGetCurrent();
	for(int i=0; i<MAX_REGIONS_PER_MOVIE; i++)
	{
		WORLD_REGION eRegion = pMovie->eRegions[ i ];
		if (eRegion != WORLD_REGION_INVALID)
		{
			if (SKUContainsRegion( nCurrentSKU, eRegion ))
			{
				return TRUE;
			}
		}
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL c_sMoviePlayerSKUHasLQMoviesOnly(
	const MOVIE_DATA * pMovie)
{
	ASSERT_RETFALSE(pMovie);

	if(pMovie->bForceAllowHQ)
	{
		return FALSE;
	}

	return !SKUAllowsHQMovies(SKUGetCurrent());
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_MoviePlayerOpen(int nMovie)
{
#if BINK_ENABLED
	const MOVIE_DATA * pMovie = (const MOVIE_DATA*)ExcelGetData(EXCEL_CONTEXT(AppGetCltGame()), DATATABLE_MOVIES, nMovie);
	if(!pMovie)
	{
		return FALSE;
	}

	if(!c_sMoviePlayerIsForSKU(pMovie))
	{
		return FALSE;
	}

	// First check if we're loading lowres movies
	if(gbForceLowQualityMovies || e_MovieUseLowResolution())
	{
		if(pMovie->pszFileNameLowres && pMovie->pszFileNameLowres[0])
		{
			if(c_sMoviePlayerOpenFile(pMovie, nMovie, pMovie->pszFileNameLowres, TRUE))
			{
				return TRUE;
			}
		}
	}

	// If there's no lowres movie, or the lowres movie failed to load, then try highres
	if(!c_sMoviePlayerSKUHasLQMoviesOnly(pMovie))
	{
		if(c_sMoviePlayerOpenFile(pMovie, nMovie, pMovie->pszFileName, FALSE))
		{
			return TRUE;
		}
	}

	// If the highres movie failed to load, then try the lowres movie.  (Yes, I realize that we might be trying lowres twice)
	return c_sMoviePlayerOpenFile(pMovie, nMovie, pMovie->pszFileNameLowres, TRUE);
#else
	return FALSE;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_MoviePlayerShowFrame()
{
#if BINK_ENABLED
	if(!sgtGameMovie.pMovie)
		return FALSE;

	if(!BinkWait(sgtGameMovie.pMovie))
	{
		GAMEOPTIONS tOptions;
		GameOptions_Get(tOptions);
		if(tOptions.bEnableCinematicSubtitles)
		{
			int nSubTitleCount = (int)tMovieSubtitleList.Count();
			if(nSubTitleCount > 0 && sgnCurrentSubtitle < nSubTitleCount && sgtGameMovie.pMovie->FrameNum >= (unsigned int)tMovieSubtitleList[sgnCurrentSubtitle]->nStartTime)
			{
				UIShowSubtitle(StringTableGetStringByIndex(tMovieSubtitleList[sgnCurrentSubtitle]->nString));
				sgnCurrentSubtitle++;
			}
			else if(sgnCurrentSubtitle > 0 && sgtGameMovie.pMovie->FrameNum >= (unsigned int)tMovieSubtitleList[sgnCurrentSubtitle-1]->nStartTime + tMovieSubtitleList[sgnCurrentSubtitle-1]->nHoldTime)
			{
				UIShowSubtitle(L"");
			}
		}
		else
		{
			UIShowSubtitle(L"");
		}

		// Are we at the end of the movie?
		if ( sgtGameMovie.pMovie->FrameNum == sgtGameMovie.pMovie->Frames )
		{
			// Yup, return FALSE if the movie doesn't loop
			if(sgbMovieLoops)
			{
				BinkGoto(sgtGameMovie.pMovie, 0, 0);
				return TRUE;
			}
			return FALSE;
		}
		else
		{
			// Nope, advance to the next frame.
			return TRUE;
		}
	}
	return TRUE;
#else
	return FALSE;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
APP_STATE c_MovePlayerGetNextState()
{
	APP_STATE eAppStateBeforeMovie = AppGetStateBeforeMovie();

	switch(eAppStateBeforeMovie)
	{
		case APP_STATE_INIT:
			return APP_STATE_MAINMENU;
		case APP_STATE_PLAYMOVIELIST:
			ASSERTX(0, "Why are we going into playmovie state from playmovie state?");
			return APP_STATE_MAINMENU;
		default:
			return eAppStateBeforeMovie;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int c_MoviePlayerGetPlaying()
{
#if BINK_ENABLED
	if (sgtGameMovie.pMovie != NULL)
	{
		ASSERTX_RETINVALID( sgtGameMovie.nMovieIndex != INVALID_LINK, "Expected movie link" );
		return sgtGameMovie.nMovieIndex;
	}
#endif
	return INVALID_LINK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if BINK_ENABLED
static void c_sMoviePlayerSetPaused()
{
	if(sgbMoviePaused || sgbMoviePausedUserControl)
	{
		BinkPause(sgtGameMovie.pMovie, TRUE);
	}
	else
	{
		if(!sgbMoviePaused && !sgbMoviePausedUserControl)
		{
			BinkPause(sgtGameMovie.pMovie, FALSE);
		}
	}
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_MoviePlayerSetPaused(
	BOOL bPaused)
{
#if BINK_ENABLED
	if(sgtGameMovie.pMovie)
	{
		sgbMoviePaused = bPaused;
		c_sMoviePlayerSetPaused();
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_MoviePlayerUserControlGetPaused()
{
#if BINK_ENABLED
	return sgbMoviePausedUserControl;
#else
	return FALSE;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_MoviePlayerUserControlSetPaused(
	BOOL bPaused)
{
#if BINK_ENABLED
	if(sgtGameMovie.pMovie)
	{
		sgbMoviePausedUserControl = bPaused;
		c_sMoviePlayerSetPaused();
		UIShowPausedMessage(sgbMoviePausedUserControl);
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_MoviePlayerUserControlTogglePaused()
{
	c_MoviePlayerUserControlSetPaused(!c_MoviePlayerUserControlGetPaused());
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL c_MoviePlayerCanPause()
{
#if BINK_ENABLED
	if(sgtGameMovie.pMovie && sgtGameMovie.nMovieIndex >= 0)
	{
		const MOVIE_DATA * pMovieData = MovieGetData(sgtGameMovie.nMovieIndex);
		if(pMovieData)
		{
			return pMovieData->bCanPause;
		}
	}
	return FALSE;
#else
	return FALSE;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_MoviePlayerSkipToEnd()
{
#if BINK_ENABLED
	if(sgtGameMovie.pMovie)
	{
		const MOVIE_DATA * pMovieData = MovieGetData(sgtGameMovie.nMovieIndex);
		if(pMovieData && (pMovieData->bNoSkip || (!sgbAllowSkippingOfFirstTimeMovies && pMovieData->bNoSkipFirstTime)))
		{
			return;
		}

		//BinkGoto(sgtGameMovie.pMovie, sgtGameMovie.pMovie->Frames-1, 0);
		c_sMoviePlayerCloseFile();
		UIShowPausedMessage(FALSE);

		if(sgbAllowSkippingOfFirstTimeMovies)
		{
			gApp.m_nCurrentMovieListSet++;
			gApp.m_nCurrentMovieListIndex = -1;
		}
	}
#endif
}

#endif // !ISVERSION(SERVER_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int c_sParseTimeCode(
	const WCHAR * uszTimeCode)
{
	// We're going to be naive here.  The format is:
	// HH:MM:SS:FF
	// So, first we'll assert on the string length:
	int nStringLength = PStrLen(uszTimeCode);
	ASSERT_RETINVALID(nStringLength == 11);

	int nHours = PStrToInt(uszTimeCode);
	int nMinutes = PStrToInt(uszTimeCode + 3);
	int nSeconds = PStrToInt(uszTimeCode + 6);
	int nFrames = PStrToInt(uszTimeCode + 9);

	return static_cast<int>((nHours * 3600 + nMinutes * 60 + nSeconds) * 29.97f) + nFrames;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#define SUBTITLE_HOLD_TIME_MINIMUM 60
#define SUBTITLE_HOLD_TIME_MAXIMUM 450
#define SUBTITLE_HOLD_TIME_MULTIPLIER 4
BOOL ExcelMovieSubtitlesPostProcess( 
	EXCEL_TABLE * table)
{
#if !ISVERSION(SERVER_VERSION)
	ASSERT_RETFALSE(table);
	ASSERT_RETFALSE(table->m_Index == DATATABLE_MOVIE_SUBTITLES);

	for (unsigned int ii = 0; ii < ExcelGetCountPrivate(table); ++ii)
	{
		MOVIE_SUBTITLE_DATA * movie_subtitle = (MOVIE_SUBTITLE_DATA *)ExcelGetDataPrivate(table, ii);
		if (movie_subtitle)
		{
			if(movie_subtitle->nString >= 0 && movie_subtitle->eLanguage == LanguageGetCurrent())
			{
				BOOL bAttributeFound = FALSE;
				DWORD dwTimeCodeAttribute = StringTableGetTimeCodeAttribute( &bAttributeFound );
				if (bAttributeFound)
				{
					BOOL bFound = FALSE;				
					DWORD dwStringFlags = 0;
					SETBIT( dwStringFlags, SF_ATTRIBUTE_MATCH_AT_LEAST_BIT );
					const WCHAR * uszTimeCode = StringTableGetStringByIndexVerbose(
						movie_subtitle->nString, 
						dwTimeCodeAttribute,
						dwStringFlags,
						NULL, 
						&bFound);
					if(bFound && uszTimeCode && uszTimeCode[0] != L'\0')
					{
						movie_subtitle->nStartTime = c_sParseTimeCode(uszTimeCode);
					}
				}

				const WCHAR * uszString = StringTableGetStringByIndex(movie_subtitle->nString);
				int nLength = PStrLen(uszString);
				movie_subtitle->nHoldTime = nLength * SUBTITLE_HOLD_TIME_MULTIPLIER;
				movie_subtitle->nHoldTime = PIN(movie_subtitle->nHoldTime, SUBTITLE_HOLD_TIME_MINIMUM, SUBTITLE_HOLD_TIME_MAXIMUM);
				//trace("String Length of String %d = %d; Hold Time is %d Frames, Pinned to %d frames\n'%S'\n", movie_subtitle->nString, nLength, (nLength*SUBTITLE_HOLD_TIME_MULTIPLIER), movie_subtitle->nHoldTime, uszString);
			}
		}
	}
#endif

	return TRUE;
}

#if !ISVERSION(SERVER_VERSION)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void MOVIE_CONFIG_OPTIONS::Exchange(SettingsExchange & se)	// CHB 2007.01.16
{
	SETTINGS_EX(se, bAllowSkippingOfFirstTimeLogoMoviesForEALegal);
}
#endif
