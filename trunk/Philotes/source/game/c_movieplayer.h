//----------------------------------------------------------------------------
// c_moveiplayer.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifdef	_MOVIEPLAYER_H_
//#pragma message ("    HINT: " __FILE__ " included multiple times in this translation unit")
#else
#define _MOVIEPLAYER_H_

#define MAX_LANGUAGES_PER_MOVIE 10
#define MAX_SKUS_PER_MOVIE 32
#define MAX_REGIONS_PER_MOVIE 32
#define MAX_MOVIES_PER_SUBTITLE 4

enum LANGUAGE;
enum WORLD_REGION;

struct MOVIE_DATA
{
	char pszName[DEFAULT_INDEX_SIZE];
	char pszFileName[DEFAULT_INDEX_SIZE];
	char pszFileNameLowres[DEFAULT_INDEX_SIZE];
	LANGUAGE eLanguages[MAX_LANGUAGES_PER_MOVIE];
	WORLD_REGION eRegions[MAX_REGIONS_PER_MOVIE];
	BOOL bLoops;
	BOOL bUseInCredits;
	BOOL bWidescreen;
	BOOL bCanPause;
	BOOL bNoSkip;
	BOOL bNoSkipFirstTime;
	BOOL bPutInMainPak;
	BOOL bForceAllowHQ;
	BOOL bOnlyWithCensoredSKU;
	BOOL bDisallowInCensoredSKU;
	float flCreditMovieDisplayTimeInSeconds;
};

#define MAX_MOVIES_IN_LIST 8
#define NUM_MOVIE_LISTS 2
struct MOVIE_LIST_DATA
{
	char pszName[DEFAULT_INDEX_SIZE];
	int nMovieList[NUM_MOVIE_LISTS][MAX_MOVIES_IN_LIST];
};

struct MOVIE_SUBTITLE_DATA
{
	char pszName[DEFAULT_INDEX_SIZE];
	int nMovie[MAX_MOVIES_PER_SUBTITLE];
	LANGUAGE eLanguage;
	int nString;
	int nStartTime;
	int nHoldTime;
};

const MOVIE_DATA *MovieGetData(
	int nMovie);

struct MOVIE_CONFIG_OPTIONS
{
	bool bAllowSkippingOfFirstTimeLogoMoviesForEALegal;

	void Exchange(class SettingsExchange & se);
};

// init/cleanup functions
void c_MoviePlayerAppInit();
void c_MoviePlayerInit();
void c_MoviePlayerClose();
void c_MoviePlayerAppClose();

BOOL c_MoviePlayerOpen(int nMovie);
BOOL c_MoviePlayerShowFrame();
void c_MoviePlayerSetPaused(BOOL bPaused);
BOOL c_MoviePlayerUserControlGetPaused();
void c_MoviePlayerUserControlSetPaused(BOOL bPaused);
void c_MoviePlayerUserControlTogglePaused();
void c_MoviePlayerSkipToEnd();
BOOL c_MoviePlayerCanPause();

int c_MoviePlayerGetPlaying();

enum APP_STATE c_MovePlayerGetNextState();

BOOL ExcelMovieSubtitlesPostProcess( 
	struct EXCEL_TABLE * table);

#endif
