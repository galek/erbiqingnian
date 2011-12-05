//----------------------------------------------------------------------------
// FILE: language.cpp
//
// (C)Copyright 2006, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
  // must be first for pre-compiled headers
#include "config.h"
#include "definition.h"
#include "excel.h"
#include "filepaths.h"
#include "fileio.h"
#include "gameconfig.h"
#include "globalindex.h"
#include "language.h"
#include "stringtable.h"
#include "sku.h"
#include <locale.h>
#if ISVERSION(SERVER_VERSION)
#include "language.cpp.tmh"
#endif

//----------------------------------------------------------------------------
// TYPES
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// NOTE: The sole reason that these language tables are no longer in excel
// and are instead in code is because it makes it easy for any application
// to link to common.lib and get the FULL langauge information.  Such
// applications include the installer, launcher, patcher etc.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct LANGUAGE_DATA sgtLanguageTableHellgate[] = 
{

	// LANGUAGE						NAME					URL ARG		FONT ATLAS							DATA DIRECTORY					LOCALE STRING			DEV		LANGUAGE AUDIO		LANGUAGE BKGRND	  ABBRV CHAT NAME
	{ LANGUAGE_ENGLISH,				"English",				"en_US",	"Fonts_Atlas",						"strings/English",				"English",				TRUE,	LANGUAGE_ENGLISH,	LANGUAGE_ENGLISH, 		9 },
	{ LANGUAGE_JAPANESE,			"Japanese",				"ja_JP",	"Japanese_Fonts_atlas",				"strings/Japanese",				"Japanese",				FALSE,	LANGUAGE_JAPANESE,	LANGUAGE_ENGLISH, 		8 },
	{ LANGUAGE_KOREAN,				"Korean",				"ko_KR",	"Korean_Fonts_atlas",				"strings/Korean",				"Korean",				FALSE,	LANGUAGE_KOREAN,	LANGUAGE_KOREAN, 		8 },
	{ LANGUAGE_CHINESE_SIMPLIFIED,	"Chinese_Simplified",	"zh_CHS",	"Chinese_Simplified_Fonts_atlas",	"strings/Chinese_Simplified",	"Chinese (Simplified)",	FALSE,	LANGUAGE_ENGLISH,	LANGUAGE_INVALID, 		8 },
	{ LANGUAGE_CHINESE_TRADITIONAL,	"Chinese_Traditional",	"zh_CHT",	"Chinese_Traditional_Fonts_atlas",	"strings/Chinese_Traditional",	"Chinese (Traditional)",FALSE,	LANGUAGE_ENGLISH,	LANGUAGE_ENGLISH, 		8 },
	{ LANGUAGE_FRENCH,				"French",				"fr_FR",	"French_Fonts_atlas",				"strings/French",				"French",				FALSE,	LANGUAGE_FRENCH,	LANGUAGE_ENGLISH, 		9 },
	{ LANGUAGE_SPANISH,				"Spanish",				"es_ES",	"Spanish_Fonts_atlas",				"strings/Spanish",				"Spanish",				FALSE,	LANGUAGE_ENGLISH,	LANGUAGE_ENGLISH, 		9 },
	{ LANGUAGE_GERMAN,				"German",				"de_DE",	"German_Fonts_atlas",				"strings/German",				"German",				FALSE,	LANGUAGE_GERMAN,	LANGUAGE_ENGLISH, 		9 },	
	{ LANGUAGE_ITALIAN,				"Italian",				"it_IT",	"Italian_Fonts_atlas",				"strings/Italian",				"Italian",				FALSE,	LANGUAGE_ENGLISH,	LANGUAGE_ENGLISH, 		9 },		
	{ LANGUAGE_POLISH,				"Polish",				"pl_PL",	"Polish_Fonts_atlas",				"strings/Polish",				"Polish",				FALSE,	LANGUAGE_ENGLISH,	LANGUAGE_ENGLISH, 		9 },
	{ LANGUAGE_CZECH,				"Czech",				"cs_CZ",	"Czech_Fonts_atlas",				"strings/Czech",				"Czech",				FALSE,	LANGUAGE_ENGLISH,	LANGUAGE_ENGLISH, 		9 },
	{ LANGUAGE_HUNGARIAN,			"Hungarian",			"hu_HU",	"Hungarian_Fonts_atlas",			"strings/Hungarian",			"Hungarian",			FALSE,	LANGUAGE_ENGLISH,	LANGUAGE_ENGLISH, 		9 },
	{ LANGUAGE_RUSSIAN,				"Russian",				"ru_RU",	"Russian_Fonts_atlas",				"strings/Russian",				"Russian",				FALSE,	LANGUAGE_ENGLISH,	LANGUAGE_ENGLISH, 		9 },
	{ LANGUAGE_THAI,				"Thai",					"th_TH",	"Thai_Fonts_atlas",					"strings/Thai",					"Thai",					FALSE,	LANGUAGE_ENGLISH,	LANGUAGE_ENGLISH, 		9 },
	{ LANGUAGE_VIETNAMESE,			"Vietnamese",			"vi_VN",	"Vietnamese_Fonts_atlas",			"strings/Vietnamese",			"Vietnamese",			FALSE,	LANGUAGE_ENGLISH,	LANGUAGE_ENGLISH, 		9 },
			
	// keep this last
	{ LANGUAGE_INVALID,				NULL,					NULL,		NULL,								NULL,							NULL,					FALSE,	LANGUAGE_INVALID,	LANGUAGE_INVALID,},

	
};

//----------------------------------------------------------------------------
struct LANGUAGE_DATA sgtLanguageTableMythos[] = 
{

	// LANGUAGE						NAME					URL ARG		FONT ATLAS							DATA DIRECTORY					LOCALE STRING			DEV		LANGUAGE AUDIO		LANGUAGE BKGRND	
	{ LANGUAGE_ENGLISH,				"English",				"en_US",	"Fonts_Atlas",						"strings/English",				"English",				TRUE,	LANGUAGE_ENGLISH,	LANGUAGE_ENGLISH },
	{ LANGUAGE_JAPANESE,			"Japanese",				"ja_JP",	"Japanese_Fonts_atlas",				"strings/Japanese",				"Japanese",				FALSE,	LANGUAGE_JAPANESE,	LANGUAGE_ENGLISH,},
	{ LANGUAGE_KOREAN,				"Korean",				"ko_KR",	"Korean_Fonts_atlas",				"strings/Korean",				"Korean",				FALSE,	LANGUAGE_KOREAN,	LANGUAGE_KOREAN, },
	{ LANGUAGE_CHINESE_SIMPLIFIED,	"Chinese_Simplified",	"zh_CHS",	"Chinese_Simplified_Fonts_atlas",	"strings/Chinese_Simplified",	"Chinese (Simplified)",	FALSE,	LANGUAGE_ENGLISH,	LANGUAGE_INVALID,},
	{ LANGUAGE_CHINESE_TRADITIONAL,	"Chinese_Traditional",	"zh_CHT",	"Chinese_Traditional_Fonts_atlas",	"strings/Chinese_Traditional",	"Chinese (Traditional)",FALSE,	LANGUAGE_ENGLISH,	LANGUAGE_ENGLISH,},
	{ LANGUAGE_FRENCH,				"French",				"fr_FR",	"French_Fonts_atlas",				"strings/French",				"French",				FALSE,	LANGUAGE_FRENCH,	LANGUAGE_ENGLISH,},
	{ LANGUAGE_SPANISH,				"Spanish",				"es_ES",	"Spanish_Fonts_atlas",				"strings/Spanish",				"Spanish",				FALSE,	LANGUAGE_ENGLISH,	LANGUAGE_ENGLISH,},
	{ LANGUAGE_GERMAN,				"German",				"de_DE",	"German_Fonts_atlas",				"strings/German",				"German",				FALSE,	LANGUAGE_GERMAN,	LANGUAGE_ENGLISH,},	
	{ LANGUAGE_ITALIAN,				"Italian",				"it_IT",	"Italian_Fonts_atlas",				"strings/Italian",				"Italian",				FALSE,	LANGUAGE_ENGLISH,	LANGUAGE_ENGLISH,},		
	{ LANGUAGE_POLISH,				"Polish",				"pl_PL",	"Polish_Fonts_atlas",				"strings/Polish",				"Polish",				FALSE,	LANGUAGE_ENGLISH,	LANGUAGE_ENGLISH,},
	{ LANGUAGE_CZECH,				"Czech",				"cs_CZ",	"Czech_Fonts_atlas",				"strings/Czech",				"Czech",				FALSE,	LANGUAGE_ENGLISH,	LANGUAGE_ENGLISH,},
	{ LANGUAGE_HUNGARIAN,			"Hungarian",			"hu_HU",	"Hungarian_Fonts_atlas",			"strings/Hungarian",			"Hungarian",			FALSE,	LANGUAGE_ENGLISH,	LANGUAGE_ENGLISH,},
	{ LANGUAGE_RUSSIAN,				"Russian",				"ru_RU",	"Russian_Fonts_atlas",				"strings/Russian",				"Russian",				FALSE,	LANGUAGE_ENGLISH,	LANGUAGE_ENGLISH,},
	{ LANGUAGE_THAI,				"Thai",					"th_TH",	"Thai_Fonts_atlas",					"strings/Thai",					"Thai",					FALSE,	LANGUAGE_ENGLISH,	LANGUAGE_ENGLISH,},
	{ LANGUAGE_VIETNAMESE,			"Vietnamese",			"vi_VN",	"Vietnamese_Fonts_atlas",			"strings/Vietnamese",			"Vietnamese",			FALSE,	LANGUAGE_ENGLISH,	LANGUAGE_ENGLISH,},
	// keep this last
	{ LANGUAGE_INVALID,				NULL,					NULL,		NULL,								NULL,							NULL,					FALSE,	LANGUAGE_INVALID,	LANGUAGE_INVALID }

};

//----------------------------------------------------------------------------
struct LANGUAGE_GLOBALS
{
	LANGUAGE eLanguage;			// the current language
	_locale_t nLocale;			// the current locale
};

//----------------------------------------------------------------------------
// LOCAL DATA
//----------------------------------------------------------------------------
static LANGUAGE_GLOBALS sgtLanguageGlobals;
#define INSTALLER_LANGUAGE_FILE L"language.dat"

#define MAX_LANGUAGE_OVERRIDE_STRING (128)
static char sgszLanguageOverride[ MAX_LANGUAGE_OVERRIDE_STRING ] = { 0 };
	
//----------------------------------------------------------------------------
// DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static LANGUAGE_DATA *sGetLanguageTable(
	APP_GAME eAppGame)
{
	if (eAppGame == APP_GAME_HELLGATE)
	{
		return sgtLanguageTableHellgate;
	}
	else if (eAppGame == APP_GAME_TUGBOAT)
	{
		return sgtLanguageTableMythos;
	}
	ASSERTV_RETNULL( 0, "Invalid app game '%d', unable to get language table", eAppGame );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const LANGUAGE_DATA *LanguageGetData(
	APP_GAME eAppGame,
	LANGUAGE eLanguage)
{
	const LANGUAGE_DATA *pTable = sGetLanguageTable( eAppGame );
	ASSERTX_RETNULL( pTable, "Expected language table" );

	for (const LANGUAGE_DATA *pLanguageData = pTable; 
		 pLanguageData && pLanguageData->eLanguage != LANGUAGE_INVALID; 
		 ++pLanguageData)
	{
		if (pLanguageData->eLanguage == eLanguage)
		{
			return pLanguageData;
		}
	}

	return NULL;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static LANGUAGE_GLOBALS *sLanguageGlobalsGet(
	void)
{
	return &sgtLanguageGlobals;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LanguageSetOverride(
	const char *pszLanguageOverride)
{
	ASSERTX_RETURN( pszLanguageOverride, "Expected string" );
	PStrCopy( sgszLanguageOverride, pszLanguageOverride, MAX_LANGUAGE_OVERRIDE_STRING );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL LanguageOverrideIsEnabled(
	void)
{
	return sgszLanguageOverride[ 0 ] != 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sLanguageGlobalsInit( 
	LANGUAGE_GLOBALS *pGlobals)
{
	pGlobals->eLanguage = LANGUAGE_ENGLISH;  // default language is the first row in this table
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static LANGUAGE sLanguageGetUserSelection(
	void)
{
	LANGUAGE eLanguage = LANGUAGE_INVALID;
	
	// one day, if we want to support the switching of installed languages on the fly
	// from within the game, we will want to write the selection for this particular user
	// in their settings file in the "My Documents" folder (or whereever it is)
	
	return eLanguage;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static LANGUAGE sLanguageGetInstallerSelection(
	void)
{
	LANGUAGE eLanguage = LANGUAGE_INVALID;

	// construct path to file
	WCHAR uszLanguageFile[ MAX_PATH ];
	PStrPrintf( uszLanguageFile, MAX_PATH, L"%S%s", FILE_PATH_DATA, INSTALLER_LANGUAGE_FILE );

	// read the language from the file that the installer created
	const int MAX_TOKEN = 256;
	char szLanguage[ MAX_TOKEN ];
	if (FileParseSimpleToken( uszLanguageFile, "Language", szLanguage, MAX_TOKEN ))
	{
		eLanguage = LanguageGetByName( AppGameGet(), szLanguage );
	}
		
	// return the language found
	return eLanguage;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LanguageInitForApp(
	APP_GAME eAppGame)
{
	LANGUAGE_GLOBALS *pGlobals = sLanguageGlobalsGet();
	sLanguageGlobalsInit( pGlobals );

	// get app level override (if any)
	LANGUAGE eLanguage = LANGUAGE_INVALID;
	if (sgszLanguageOverride[ 0 ] != 0)
	{
		eLanguage = LanguageGetByName( AppGameGet(), sgszLanguageOverride );		
	}
		
	// search for a language setting for this particular user
	if (eLanguage == LANGUAGE_INVALID)
	{
		eLanguage = sLanguageGetUserSelection();
	}
	
	// search for language selection from the installer if no user selection
	if (eLanguage == LANGUAGE_INVALID)
	{
		eLanguage = sLanguageGetInstallerSelection();
	}
	
	// if no language found, check the global definition (used for development)
	if (eLanguage == LANGUAGE_INVALID)
	{	
		const GLOBAL_DEFINITION *pGlobalDef = DefinitionGetGlobal();
		ASSERTX_RETURN( pGlobalDef, "Expected global definition" );
		eLanguage = LanguageGetByName( AppGameGet(), pGlobalDef->szLanguage );
	}

	// set the language found
	if (eLanguage != LANGUAGE_INVALID)
	{
		LanguageSetCurrent( eAppGame, eLanguage );
	}
		
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LanguageFreeForApp(
	void)
{
	LANGUAGE_GLOBALS *pGlobals = sLanguageGlobalsGet();
	sLanguageGlobalsInit( pGlobals );
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LanguageSetCurrent(
	APP_GAME eAppGame,
	LANGUAGE eLanguage)
{
	ASSERTX_RETURN( eLanguage != LANGUAGE_INVALID, "Invalid language" );
	
	// set the new language
	LANGUAGE_GLOBALS *pGlobals = sLanguageGlobalsGet();
	pGlobals->eLanguage = eLanguage;

	const LANGUAGE_DATA *pLanguageData = LanguageGetData( eAppGame, eLanguage );
	ASSERTX_RETURN( pLanguageData, "Language data not found" );
	if (pLanguageData->szLocaleString && pLanguageData->szLocaleString[0])
	{
		pGlobals->nLocale = _create_locale(LC_ALL, pLanguageData->szLocaleString);
		//
		// NOTE: CAN'T DO THIS BECAUSE IT TOTALLY BREAKS INT<->FLOAT CONVERSION!!
		//
		//if (setlocale( LC_ALL, pLanguageData->szLocaleString ) == NULL)
		//{
		//	TraceError( "Unable to set locale for region '%s'", pLanguageData->szLocaleString);
		//}
	}
	// debug print	
	TraceDebugOnly("Language set to '%s'\n", LanguageGetCurrentName());
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
LANGUAGE LanguageGetCurrent(
	void)
{
	LANGUAGE_GLOBALS *pGlobals = sLanguageGlobalsGet();
	return pGlobals->eLanguage;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
_locale_t LanguageGetLocale(
	void)
{
	LANGUAGE_GLOBALS *pGlobals = sLanguageGlobalsGet();
	return pGlobals->nLocale;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const char *LanguageGetCurrentName(
	void)
{
	LANGUAGE eLanguageCurrent = LanguageGetCurrent();
	ASSERTX_RETNULL( eLanguageCurrent != LANGUAGE_INVALID, "No language set as current" );	
	APP_GAME eAppGame = AppGameGet();
	return LanguageGetName( eAppGame, eLanguageCurrent );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const char *LanguageGetName(
	APP_GAME eAppGame,
	LANGUAGE eLanguage)
{
	ASSERTX_RETNULL(eLanguage != LANGUAGE_INVALID, "Invalid language");
	const LANGUAGE_DATA *pLanguageData = LanguageGetData( eAppGame, eLanguage );
	return pLanguageData->szName;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
LANGUAGE LanguageGetByName(
	APP_GAME eAppGame,
	const WCHAR *puszName)
{

	// convert name to ascii
	char uszName[ DEFAULT_INDEX_SIZE ];
	PStrCvt( uszName, puszName, DEFAULT_INDEX_SIZE );
	
	// find by ascii
	return LanguageGetByName( eAppGame, uszName );
		
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
LANGUAGE LanguageGetByName(
	APP_GAME eAppGame,
	const char *pszName)
{
	ASSERTX_RETVAL( pszName, LANGUAGE_INVALID, "Expected langauge name" );
	const LANGUAGE_DATA *pTable = sGetLanguageTable( eAppGame );
	ASSERTX_RETVAL( pTable, LANGUAGE_INVALID, "Expected langauge table" );

	// go through table
	for (const LANGUAGE_DATA *pLanguageData = pTable; 
		 pLanguageData && pLanguageData->eLanguage != LANGUAGE_INVALID; 
		 ++pLanguageData)
	{
		if (PStrICmp( pLanguageData->szName, pszName ) == 0)
		{
			return pLanguageData->eLanguage;
		}
	}

	return LANGUAGE_INVALID;
			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL LanguageIsDeveloper(
	APP_GAME eAppGame,
	LANGUAGE eLanguage)
{
	const LANGUAGE_DATA *pLanguageData = LanguageGetData( eAppGame, eLanguage );
	return pLanguageData->bDeveloperLanguage;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
LANGUAGE LanguageGetDevLanguage(
	void)
{

#if !ISVERSION(DEVELOPMENT)
	return LANGUAGE_INVALID;
#else

	APP_GAME eAppGame = AppGameGet();
	const LANGUAGE_DATA *pTable = sGetLanguageTable( eAppGame );
	ASSERTX_RETVAL( pTable, LANGUAGE_INVALID, "Expected language table" );

	// go through table
	for (const LANGUAGE_DATA *pLanguageData = pTable; 
		 pLanguageData && pLanguageData->eLanguage != LANGUAGE_INVALID; 
		 ++pLanguageData)
	{
		if (pLanguageData->bDeveloperLanguage == TRUE)
		{
			return pLanguageData->eLanguage;
		}
	}

	ASSERTX_RETVAL( 0, LANGUAGE_INVALID, "Must have a dev language" );
	
#endif
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
LANGUAGE LanguageCurrentGetAudioLanguage(
	APP_GAME eAppGame)
{
	LANGUAGE eLanguage = LanguageGetCurrent();
	return LanguageGetAudioLanguage( eAppGame, eLanguage );
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
LANGUAGE LanguageGetAudioLanguage(
	APP_GAME eAppGame,
	LANGUAGE eLanguage)
{
	ASSERTX_RETVAL( eLanguage != LANGUAGE_INVALID, LANGUAGE_INVALID, "Expected langauge link" );
	const LANGUAGE_DATA *pLanguageData = LanguageGetData( eAppGame, eLanguage );
	if (pLanguageData)
	{
		return pLanguageData->eLanguageAudio;
	}
	return LANGUAGE_INVALID;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
LANGUAGE LanguageGetBackgroundTextureLanguage(
	APP_GAME eAppGame,
	LANGUAGE eLanguage)
{
	ASSERTX_RETVAL( eLanguage != LANGUAGE_INVALID, LANGUAGE_INVALID, "Expected langauge" );
	const LANGUAGE_DATA *pLanguageData = LanguageGetData( eAppGame, eLanguage );
	return pLanguageData->eLanguageBackgroundTextures;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LanguageFormatNumberString(
	WCHAR *puszDest, 
	int nDestLen, 
	const WCHAR *puszSource)
{
	ASSERTX_RETURN( puszDest && puszSource && (int)PStrLen( puszSource ) < nDestLen, "Invalid params for money localization" );	
	const WCHAR *puszDigitGroupSize = GlobalStringGet( GS_THOUSANDS_SEPARATOR_MAX_GROUP_SIZE );
	int nDigitGroupSize = PStrToInt( puszDigitGroupSize );
	const WCHAR *puszGroupingSeperator = GlobalStringGet( GS_THOUSANDS_SEPARATOR );
	int nGroupingSeperatorLen = PStrLen( puszGroupingSeperator );
	const WCHAR *puszFractionSeparator = GlobalStringGet( GS_FRACTION_SEPARATOR );	
	int nFractionSeparatorLen = PStrLen( puszFractionSeparator );
	
	// get source len
	int nSourceLen = PStrLen( puszSource );

	// if this is not completely a "number string" then we won't do anything
	int nLastDigitIndex = -1;
	int nFractionIndex = -1;
	BOOL bIsNumberString = TRUE;
	for (int i = 0; i < nSourceLen; ++i)
	{
		WCHAR ucChar = puszSource[ i ];
		BOOL bIsDigit = iswdigit( ucChar ) != 0;
		BOOL bIsFormatCharacter = FALSE;
		if (ucChar == L'+' || ucChar == L'-' || ucChar == L'.' || ucChar == L'%')
		{
			bIsFormatCharacter = TRUE;
		}
		
		if (bIsDigit == FALSE && bIsFormatCharacter == FALSE)
		{
			bIsNumberString = FALSE;
			break;
		}
		else
		{
		
			if (bIsDigit)
			{
				nLastDigitIndex = i;	
			}
			
			if (ucChar == L'.')
			{
				if (nFractionIndex == -1)
				{
					nFractionIndex = i;
				}
			}
			
		}
		
	}
	if (bIsNumberString == FALSE)
	{
		PStrCopy( puszDest, puszSource, nDestLen );
	}
	else
	{
		int nDestIndex = 0;
		puszDest[ 0 ] = 0;

		// copy all the leading non digit characters to the dest string
		int nFirstDigitIndex = -1;
		for (int i = 0; i < nSourceLen; ++i)
		{
			WCHAR ucChar = puszSource[ i ];
			if (iswdigit( ucChar ) == FALSE)
			{
				puszDest[ nDestIndex++ ] = ucChar;	
				puszDest[ nDestIndex ] = 0;
			}
			else
			{
				if (nFirstDigitIndex == -1)
				{
					nFirstDigitIndex = i;
					break;
				}
			}
		}
		
		if (nFirstDigitIndex != -1)
		{
		
			// how many digits are there to consider in the grouping calculations (we are ignoring
			// all the leading non-digits and everything after the fraction separator
			int nLastDigitIndexToGroup;
			if (nFractionIndex != -1)
			{
				nLastDigitIndexToGroup = nFractionIndex - 1;
			}
			else
			{
				nLastDigitIndexToGroup = nLastDigitIndex;
			}
			int nNumDigitsToGroup = nLastDigitIndexToGroup - nFirstDigitIndex + 1;
			
			// how many groups are there	
			int nNumInLeftMostGroup = (nDigitGroupSize ? nNumDigitsToGroup % nDigitGroupSize : 0);
			if (nNumInLeftMostGroup == 0)
			{
				nNumInLeftMostGroup = nDigitGroupSize;
			}

			int nCurrentInGroup = nNumInLeftMostGroup;
			for (int i = nFirstDigitIndex; i <= nLastDigitIndexToGroup; ++i)
			{
			
				// copy character
				puszDest[ nDestIndex++ ] = puszSource[ i ];
				
				// terminate what we have so far
				puszDest[ nDestIndex ] = 0;
				
				// is this a group that needs a separator
				if (--nCurrentInGroup == 0 && nLastDigitIndexToGroup - i >= nDigitGroupSize)
				{
					nCurrentInGroup = nDigitGroupSize;
					PStrCat( puszDest, puszGroupingSeperator, nDestLen );
					nDestIndex += nGroupingSeperatorLen;
				}
					
			}

			// now do any fractional part
			if (nFractionIndex != -1)
			{
			
				// add the fraction separator
				if (nFractionIndex > nFirstDigitIndex)
				{
					PStrCat( puszDest, puszFractionSeparator, nDestLen );
					nDestIndex += nFractionSeparatorLen;
				}
				
				// copy all the fraction digits
				for (int i = nFractionIndex + 1; i < nSourceLen; ++i)
				{
					puszDest[ nDestIndex++ ] = puszSource[ i ];
					puszDest[ nDestIndex ] = 0;
				}
				
			}			
			else
			{
			
				// now do any traling characters
				for (int i = nLastDigitIndex + 1; i < nSourceLen; ++i)
				{
					puszDest[ nDestIndex++ ] = puszSource[ i ];
					puszDest[ nDestIndex ] = 0;			
				}

			}
						
		}
		
	}		

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LanguageFormatFloatString(
	WCHAR *puszDest,
	int nDestLen,
	const float flValue,
	int nNumDecimalPlaces)
{
	ASSERTX_RETURN( puszDest, "Expected dest buffer" );
	ASSERTX_RETURN( nDestLen > 0, "Invalid dest len" );
	ASSERTX_RETURN( nNumDecimalPlaces >= 0, "Invalid num decimal places" );
	
	// construct format specifier
	const int MAX_FORMAT = 64;
	WCHAR uszFormat[ MAX_FORMAT ];
	uszFormat[ 0 ] = 0;
	PStrPrintf( uszFormat, MAX_FORMAT, L"%%.%df", nNumDecimalPlaces );

	// construct float value as string	
	const int MAX_BUFFER = 64;
	WCHAR uszTemp[ MAX_BUFFER ];
	PStrPrintf( uszTemp, MAX_BUFFER, uszFormat, flValue );
	
	// language format the string
	LanguageFormatNumberString( puszDest, nDestLen, uszTemp );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LanguageFormatIntString(
	WCHAR *puszDest,
	int nDestLen,
	const int nValue)
{
	ASSERTX_RETURN( puszDest, "Expected dest buffer" );
	ASSERTX_RETURN( nDestLen > 0, "Invalid dest len" );

	// construct value as string	
	const int MAX_BUFFER = 64;
	WCHAR uszTemp[ MAX_BUFFER ];
	PStrPrintf( uszTemp, MAX_BUFFER, L"%d", nValue );
	
	// language format the string
	LanguageFormatNumberString( puszDest, nDestLen, uszTemp );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LanguageGetURLArgument(
	APP_GAME eAppGame,
	LANGUAGE eLanguage,
	WCHAR *puszBuffer,
	int nBufferLen)
{
	ASSERTX_RETURN( eLanguage != LANGUAGE_INVALID, "Expected language enum" );
	ASSERTX_RETURN( puszBuffer, "Expected buffer" );
	ASSERTX_RETURN( nBufferLen, "Invalid buffer len" );
	
	// init result
	puszBuffer[ 0 ] = 0;
	
	const LANGUAGE_DATA *pLanguageData = LanguageGetData( eAppGame, eLanguage );
	ASSERTX_RETURN( pLanguageData, "Expected language data" );
	PStrCvt( puszBuffer, pLanguageData->szURLArgument, nBufferLen );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LanguageGetDurationString(
	WCHAR *szBuf, 
	int nBufSize, 
	int nDurationSeconds)
{
	if (nDurationSeconds > 59)
	{
		WCHAR szNum[32];
		LanguageFormatIntString(szNum, arrsize(szNum), ( nDurationSeconds / 60 ));
		PStrCopy(szBuf, GlobalStringGet(GS_MINUTES_REMAINING), nBufSize);

		PStrReplaceToken( szBuf, nBufSize, L"[TIME_REMAINING]", szNum );
	}
	else if (nDurationSeconds > 0)
	{
		LanguageFormatIntString(szBuf, nBufSize, nDurationSeconds);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LanguageGetTimeString(
	WCHAR *szBuf, 
	int nBufSize, 
	int nTimeSeconds)
{
	// get the grouping separator
	const WCHAR *puszSeparator = GlobalStringGet( GS_TIME_GROUPING_SEPARATOR );
	
	int nHours = nTimeSeconds / SECONDS_PER_MINUTE / MINUTES_PER_HOUR;
	nTimeSeconds -= nHours * MINUTES_PER_HOUR * SECONDS_PER_MINUTE;
	int nMinutes = nTimeSeconds / SECONDS_PER_MINUTE;
	nTimeSeconds -= nMinutes * SECONDS_PER_MINUTE;
							
	if (nHours > 0)
	{
		PStrPrintf( 
			szBuf, 
			nBufSize, 
			L"%02d%s%02d%s%02d", 
			nHours, 
			puszSeparator,
			nMinutes, 
			puszSeparator,
			nTimeSeconds );		
	}
	else
	{
		PStrPrintf( 
			szBuf, 
			nBufSize, 
			L"%02d%s%02d", 
			nMinutes, 
			puszSeparator,
			nTimeSeconds );					
	}


}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int LanguageGetNumLanguages(
	APP_GAME eAppGame)
{
	const LANGUAGE_DATA *pTable = sGetLanguageTable( eAppGame );
	ASSERTX_RETZERO( pTable, "Expected language table" );
	
	int nCount = 0;
	for (const LANGUAGE_DATA *pLanguageData = pTable; 
		 pLanguageData && pLanguageData->eLanguage != LANGUAGE_INVALID; 
		 ++pLanguageData)
	{
		nCount++;
	}

	return nCount;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void LanguageIterateLanguages(
	APP_GAME eAppGame,
	FN_LANGUAGE_ITERATE_CALLBACK *pfnCallback,
	void *pCallbackData)
{
	ASSERTX_RETURN( pfnCallback, "Expected callback" );
	const LANGUAGE_DATA *pTable = sGetLanguageTable( eAppGame );
	ASSERTX_RETURN( pTable, "Expected language table" );
	
	for (const LANGUAGE_DATA *pLanguageData = pTable; 
		 pLanguageData && pLanguageData->eLanguage != LANGUAGE_INVALID; 
		 ++pLanguageData)
	{
		pfnCallback( pLanguageData, pCallbackData );
	}
	
}
