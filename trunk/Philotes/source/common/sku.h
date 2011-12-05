//----------------------------------------------------------------------------
// FILE: sku.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

// S.tock K.eeping U.nit incase you are curious

#ifndef __SKU_H_
#define __SKU_H_

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Forward Declarations
//----------------------------------------------------------------------------
enum LANGUAGE;
enum WORLD_REGION;

//----------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------

enum SKU_CONSTANTS
{
	SKU_MAX_LANGUAGES = 64,
	SKU_AD_PUBLIC_KEY_SIZE = 64,
	SKU_MAX_MOVIELIST_INTRO_SIZE = 64,
	SKU_MAX_REGIONS = 32,				// arbitrary
};

//----------------------------------------------------------------------------
struct SKU_DATA
{
	char szName[ DEFAULT_INDEX_SIZE ];
	WORD wCode;
	BOOL bDefault;
	BOOL bServer;
	BOOL bDeveloper;
	BOOL bCensorLocked;
	BOOL bCensorParticles;
	BOOL bCensorBoneShrinking;
	BOOL bCensorMonsterClassReplacementsNoHumans;
	BOOL bCensorMonsterClassReplacementsNoGore;
	BOOL bCensorPvPForUnderAge;
	BOOL bCensorPvP;
	BOOL bCensorMovies;
	BOOL bLowQualityMoviesOnly;	
	char szAdPublicKey[ SKU_AD_PUBLIC_KEY_SIZE ];
	int nAdPublicKey;
	char szMovielistIntro[ SKU_MAX_MOVIELIST_INTRO_SIZE ];
	int nMovielistIntro;
	int nMovieTitleScreen;
	int nMovieTitleScreenWide;
	LANGUAGE eLanguages[ SKU_MAX_LANGUAGES ];
	BOOL bSpecialLauncher;
	WORLD_REGION eRegions[ SKU_MAX_REGIONS ];
};

//----------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------

BOOL SKUExcelPostProcessAll(
	EXCEL_TABLE *pTable);

const SKU_DATA *SKUGetData(
	int nSKU);
	
int SKUGetByName( 
	GAME *pGame,
	const char *pszSKU);

BOOL SKUUsesBackgroundTextureLanguage(
	int nSKU,
	LANGUAGE eLanguageBackgroundTexure);

BOOL SKUInitializeForApp(
	void);

BOOL SKUWriteToPak( 
	int nSKU);

int SKUGetDefault(
	void);

BOOL SKUOverride(
	int nSKU);

BOOL SKUContainsLanguage(
	int nSKU,
	LANGUAGE eLangauge,
	BOOL bCheckDevelopmentLanguage);

BOOL SKUContainsAudioLanguage(
	int nSKU,
	LANGUAGE eLanguageAudio);
	
int SKUGetCurrent();

int SKUGetIntroMovieList(
	int nSKU);

int ReadSKUFromFileBuffer(
	BYTE *pFileData,
	DWORD dwSize);

BOOL SKUContainsRegion(
	int nSKU,
	WORLD_REGION eRegion);

WORLD_REGION SKUGetSingleRegion(
	int nSKU);

BOOL SKUAllowsHQMovies(
	int nSKU);

BOOL SKUCensorsMovies(
	int nSKU);

void SKUSetSKUToValidate(
	const char *pszSKUToValidate);

int SKUGetSKUToValidate(
	void);
	
#endif
