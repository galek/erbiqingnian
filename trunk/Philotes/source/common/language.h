//----------------------------------------------------------------------------
// FILE: language.h
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

#ifndef __LANGUAGE_H_
#define __LANGUAGE_H_

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// FORWARD DECLARATIOS
//----------------------------------------------------------------------------
struct GAME;
struct DATA_TABLE;

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
enum LANGUAGE
{
	LANGUAGE_INVALID = -1,
	LANGUAGE_CURRENT = LANGUAGE_INVALID,		// niceness for functions that want to support it

	// Do *NOT* reorder/renumber, these are stored in excel tables, add new ones to end *ONLY*
	// unless you also cause the excel cooked data to rebuild
	LANGUAGE_ENGLISH				=  0,		// do not renumber/reorder
	LANGUAGE_JAPANESE				=  1,		// do not renumber/reorder
	LANGUAGE_KOREAN					=  2,		// do not renumber/reorder
	LANGUAGE_CHINESE_SIMPLIFIED		=  3,		// do not renumber/reorder
	LANGUAGE_CHINESE_TRADITIONAL	=  4,		// do not renumber/reorder
	LANGUAGE_FRENCH					=  5,		// do not renumber/reorder
	LANGUAGE_SPANISH				=  6,		// do not renumber/reorder
	LANGUAGE_GERMAN					=  7,		// do not renumber/reorder
	LANGUAGE_ITALIAN				=  8,		// do not renumber/reorder
	LANGUAGE_POLISH					=  9,		// do not renumber/reorder
	LANGUAGE_CZECH					= 10,		// do not renumber/reorder
	LANGUAGE_HUNGARIAN				= 11,		// do not renumber/reorder
	LANGUAGE_RUSSIAN				= 12,		// do not renumber/reorder
	LANGUAGE_THAI					= 13,		// do not renumber/reorder
	LANGUAGE_VIETNAMESE				= 14,		// do not renumber/reorder
			
	// Do *NOT* reorder/renumber, these are stored in excel tables, add new ones to end *ONLY*
	// unless you also cause the excel cooked data to rebuild
	
	LANGUAGE_NUM_LANGUAGES					// keep this last please	
};

//----------------------------------------------------------------------------
enum LANGUAGE_CONSTANTS
{
	MAX_LANGUAGE_URL_ARGUMENT_LENGTH  = 64,			// arbitrary
	MAX_LANGUAGE_NAME = 64,							// arbitrary
	MAX_LANGAUGE_FONT_ATLAS = 64,					// arbitrary
	MAX_LANGAUGE_DATA_DIRECTORY = 64,				// arbitrary
	MAX_LANGUAGE_LOCALE_STRING = 64,				// arbitrary
};

//----------------------------------------------------------------------------
struct LANGUAGE_DATA
{
	LANGUAGE eLanguage;										// the language enum reference
	char szName[ MAX_LANGUAGE_NAME ];						// language name
	char szURLArgument[ MAX_LANGUAGE_URL_ARGUMENT_LENGTH ];  // this language uses this string when used as arguments to urls
	char szFontAtlas[ MAX_LANGAUGE_FONT_ATLAS ];			// the filename of the xml definition the UI will use to render fonts for this language
	char szDataDirectory[ MAX_LANGAUGE_DATA_DIRECTORY ];	// directory in which the string data files reside	
	char szLocaleString[ MAX_LANGUAGE_LOCALE_STRING ];		// locale string for windows setlocale
	BOOL bDeveloperLanguage;								// the language we use for development
	LANGUAGE eLanguageAudio;								// language localized sound files will be played in
	LANGUAGE eLanguageBackgroundTextures;					// language localized background files to be used
	int nAbbrevChatNameLen;									// maximum length of abbreviated names in the chat window when that option is enabled.
};

//----------------------------------------------------------------------------
// PROTOTYPES
//----------------------------------------------------------------------------

void LanguageSetOverride(
	const char *pszLanguageOverride);

BOOL LanguageOverrideIsEnabled(
	void);
		
const LANGUAGE_DATA *LanguageGetData(
	APP_GAME eAppGame,
	LANGUAGE eLanguage);

void LanguageInitForApp(
	APP_GAME eAppGame);

void LanguageFreeForApp(
	void);
	
void LanguageSetCurrent(
	APP_GAME eAppGame,
	LANGUAGE eLanguage);
	
LANGUAGE LanguageGetCurrent(
	void);

_locale_t LanguageGetLocale(
	void);
							
const char *LanguageGetCurrentName(
	void);
	
const char *LanguageGetName(
	APP_GAME eAppGame,
	LANGUAGE eLanguage);
	
LANGUAGE LanguageGetByName(
	APP_GAME eAppGame,
	const WCHAR *puszName);

LANGUAGE LanguageGetByName(
	APP_GAME eAppGame,
	const char *pszName);

BOOL LanguageIsDeveloper(
	APP_GAME eAppGame,
	LANGUAGE eLanguage);

LANGUAGE LanguageGetDevLanguage(
	void);

LANGUAGE LanguageCurrentGetAudioLanguage(
	APP_GAME eAppGame);
	
LANGUAGE LanguageGetAudioLanguage(
	APP_GAME eAppGame,
	LANGUAGE eLanguage);

LANGUAGE LanguageGetBackgroundTextureLanguage(
	APP_GAME eAppGame,
	LANGUAGE eLanguage);

void LanguageFormatNumberString(
	WCHAR *puszDest, 
	int nDestLen, 
	const WCHAR *puszSource);

void LanguageFormatFloatString(
	WCHAR *puszDest,
	int nDestLen,
	const float flValue,
	int nNumDecimalPlaces);

void LanguageFormatIntString(
	WCHAR *puszDest,
	int nDestLen,
	const int nValue);

void LanguageGetURLArgument(
	APP_GAME eAppGame,
	LANGUAGE eLanguage,
	WCHAR *puszBuffer,
	int nBufferLen);
						
void LanguageGetDurationString(
	WCHAR *szBuf, 
	int nBufSize, 
	int nDurationSeconds);

void LanguageGetTimeString(
	WCHAR *szBuf, 
	int nBufSize, 
	int nTimeSeconds);

int LanguageGetNumLanguages(
	APP_GAME eAppGame);

typedef void (FN_LANGUAGE_ITERATE_CALLBACK)( const LANGUAGE_DATA *pLanguageData, void *pCallbackData );

void LanguageIterateLanguages(
	APP_GAME eAppGame,
	FN_LANGUAGE_ITERATE_CALLBACK *pfnCallback,
	void *pCallbackData);
	
#endif
