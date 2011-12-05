//----------------------------------------------------------------------------
// stringtable.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "appcommon.h"
#include "characterclass.h"
#include "console.h"
#include "definition_common.h"
#include "excel.h"
#include "excel_private.h"
#include "fileio.h"
#include "filepaths.h"
#include "fileio.h"
#include "game.h"
#include "globalindex.h"
#include "language.h"
#include "pakfile.h"
#include "performance.h"
#include "prime.h"
#include "stringtable.h"
#include "sku.h"
#include "log.h"

//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------
#define STRING_FILE_EXTENSION					"xls"	
#define EXPORTED_STRING_EXTENSION				"uni"	
#define EXPORTED_STRING_EXTENSION_MASK			"*.uni"

// TODO: we could define these in data in a well known string label if necessary, that
// way they could be different for every language, but the code will have to know about
// all the number forms by evaluating some data driven thing that gives it the rules too
#define DEFAULT_ATTRIBUTE_NAME					L"default"
#define TIMECODE_ATTRIBUTE_NAME					L"timecode"	// use for cinematics timing
#define MASCULINE_ATTRIBUTE_NAME				L"masculine"
#define FEMININE_ATTRIBUTE_NAME					L"feminine"
#define NEUTER_ATTRIBUTE_NAME					L"neuter"
#define UNDER18_ATTRIBUTE_NAME					L"Under18"

#define SKU_VALIDATE_RESULT_FILENAME			"sku_validate_result.txt"

static WCHAR *guszTextLength = L"_-_-_-_-_1_-_-_-_-_2_-_-_-_-_3_-_-_-_-_4_-_-_-_-_5_-_-_-_-_6_-_-_-_-_7_-_-_-_-_8_-_-_-_-_9_-_-_-_-_A";
static BOOL sgbStringTableFillDevLanguage = FALSE;

//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct STRING_DATA_FILE
{
	char szDataFile[ MAX_PATH ];	// data files used to load the string table
	char szSourceFile[ MAX_PATH ];	// source data files used to generate szDataFiles[]
	BOOL bIsCommon;
	BOOL bLoadedByGame;
	int nStringFileIndex;			// index in string_files.xls
};

//----------------------------------------------------------------------------
struct STRING_TABLE_INFO
{
	STRING_DATA_FILE tDataFile[ MAX_STRING_SOURCE_FILES ];
	int nNumDataFiles;
};

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct STRING_TABLE_GLOBALS
{
	STRING_TABLE_INFO *pTableInfo;
	int nNumInfo;
};
static STRING_TABLE_GLOBALS sgtStringTableGlobals;

//----------------------------------------------------------------------------
// FUNCTION DEFINITIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
STRING_TABLE_GLOBALS *sStringTableGlobalsGet(
	void)
{
	return &sgtStringTableGlobals;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sStringTableInfoInit(	
	STRING_TABLE_INFO *pInfo)
{
	memclear( pInfo, sizeof( STRING_TABLE_INFO ) );	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StringTableFree(
	void)
{

	// free string tables
	StringTableCommonFree();		
		
	// free storage
	STRING_TABLE_GLOBALS *pGlobals = sStringTableGlobalsGet();	
	
#if USE_MEMORY_MANAGER && !ISVERSION(SERVER_VERSION)
	FREE( g_StaticAllocator, pGlobals->pTableInfo );
#else		
	FREE( NULL, pGlobals->pTableInfo );
#endif
	pGlobals->pTableInfo = NULL;
	pGlobals->nNumInfo = 0;
						
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sStringTableGetIndex(
	STRING_TABLE_ENUM eTable)
{

	// if certain app flags are set, we will always use the developer table
	if (AppTestDebugFlag( ADF_TEXT_DEVELOPER ))
	{
		eTable = STE_DEVELOPER;
	}
	
	if (eTable == STE_DEVELOPER)
	{
		return LanguageGetDevLanguage();
	}
	else
	{
		return LanguageGetCurrent();
	}
	
}

//----------------------------------------------------------------------------
typedef void (* STRING_FILE_ITERATE_CALLBACK)( const FILE_ITERATE_RESULT *pResult, int nStringFileIndex, BOOL bIsCommon, BOOL bLoadedByGame, void *pUserData );

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sStringDataFilesIterate(
	APP_GAME eAppGame,
	LANGUAGE eLanguage,
	STRING_FILE_ITERATE_CALLBACK pfnCallback,
	void *pUserData)
{
	// do nothing for invalid languages
	if (eLanguage == LANGUAGE_INVALID)
	{
		return;
	}
			
	// get the current language
	const LANGUAGE_DATA *pLanguageData = LanguageGetData( eAppGame, eLanguage );

	// iterate all the data files for this language
	int nNumFiles = ExcelGetCount( NULL, DATATABLE_STRING_FILES );
	for (int i = 0; i < nNumFiles; ++i)
	{
		const STRING_FILE_DEFINITION *ptStringFileDef = StringFileDefinitionGet( i );	
		FILE_ITERATE_RESULT tResult;
		
		// construct filename
		PStrPrintf( 
			tResult.szFilename, 
			MAX_PATH, 
			"%s.%s.%s", 
			ptStringFileDef->szName, 
			STRING_FILE_EXTENSION, 
			EXPORTED_STRING_EXTENSION);

		// construct relative filepath			
		PStrPrintf( 
			tResult.szFilenameRelativepath,
			MAX_PATH, 
			"%s%s/%s", 
			StringTableCommonGetStringFileGetExcelPath( ptStringFileDef->bIsCommon ), 
			pLanguageData->szDataDirectory,
			tResult.szFilename);
		
		// call the callback
		pfnCallback( &tResult, i, ptStringFileDef->bIsCommon, ptStringFileDef->bLoadedByGame, pUserData );
				
	}
	
}		


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAddDataFile(
	const FILE_ITERATE_RESULT *pResult,
	int nStringFileIndex,
	BOOL bIsCommon,
	BOOL bLoadedByGame,
	void *pUserData)
{
	STRING_TABLE_INFO *pTableInfo = (STRING_TABLE_INFO *)pUserData;
	
	// there must be room for the string
	ASSERTX_RETURN( pTableInfo->nNumDataFiles < MAX_STRING_SOURCE_FILES, "Too many string data files" );

	// save filename
	STRING_DATA_FILE *pDataFile = &pTableInfo->tDataFile[ pTableInfo->nNumDataFiles++ ];
	PStrCopy( pDataFile->szDataFile, pResult->szFilename, MAX_PATH );
	
	// save if it's common or not
	pDataFile->bIsCommon = bIsCommon;
	pDataFile->bLoadedByGame = bLoadedByGame;
	pDataFile->nStringFileIndex = nStringFileIndex;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sForceFirstDataFile( 
	STRING_TABLE_INFO *pTableInfo, 
	const char *pszName)
{
	BOOL bFound = FALSE;
	
	for (int i = 0; i < pTableInfo->nNumDataFiles; ++i)
	{
		STRING_DATA_FILE *pDataFile = &pTableInfo->tDataFile[ i ];
		if (PStrStrI( pDataFile->szSourceFile, pszName ) != NULL)
		{
			bFound = TRUE;
			
			// put in first location if it's not already
			if (i != 0)
			{
				STRING_DATA_FILE tTemp = pTableInfo->tDataFile[ 0 ];
				pTableInfo->tDataFile[ 0 ] = *pDataFile;
				pTableInfo->tDataFile[ i ] = tTemp;
			}
		}
	}
	ASSERTX( bFound, "Unable to find match to force first string data file" );
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sLoadAllDataFileNames(
	APP_GAME eAppGame)
{
	STRING_TABLE_GLOBALS *pGlobals = sStringTableGlobalsGet();	
	
	// get the number of languages we can possibly load
	int nNumLanguages = LanguageGetNumLanguages( AppGameGet() );
	for (int nLanguage = 0; nLanguage < nNumLanguages; ++nLanguage)
	{
		LANGUAGE eLanguage = (LANGUAGE)nLanguage;
		ASSERTX_CONTINUE( eLanguage < pGlobals->nNumInfo, "Not enough string table info storage for all languages" );
		STRING_TABLE_INFO *pTableInfo = &pGlobals->pTableInfo[ eLanguage ];
		
		// if no files loaded, load them now
		if (pTableInfo->nNumDataFiles == 0)
		{

			// gather the filenames		
			sStringDataFilesIterate( eAppGame, eLanguage, sAddDataFile, pTableInfo );

			// construct the source excel file names
			for (int j = 0; j < pTableInfo->nNumDataFiles; ++j)
			{
				STRING_DATA_FILE *pDataFile = &pTableInfo->tDataFile[ j ];		
				const char *pszFilename = pDataFile->szDataFile;
				
				// remove the last extension appended to the end of the filename
				char *pszSourceFilename = pDataFile->szSourceFile;
				PStrRemoveExtension( pszSourceFilename, MAX_PATH, pszFilename );
				
			}

			// force Strings_Strings.xls to load first
			sForceFirstDataFile( pTableInfo, "Strings_Strings" );
						
		}

	}
			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const STRING_FILE_DEFINITION * StringFileDefinitionGet(
	int nStringFile)
{
	return (const STRING_FILE_DEFINITION *)ExcelGetData(NULL, DATATABLE_STRING_FILES, nStringFile);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL StringTableInitForApp(
	void)
{
	STRING_TABLE_GLOBALS *pGlobals = sStringTableGlobalsGet();
	ASSERTX_RETFALSE( pGlobals, "Expected globals" );

	pGlobals->nNumInfo = 0;
	pGlobals->pTableInfo = NULL;

	return TRUE;
	
}
	
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StringTableInit(
	APP_GAME eAppGame)
{
	
	// init our own internal string table info
	STRING_TABLE_GLOBALS *pGlobals = sStringTableGlobalsGet();
	ASSERTX_RETURN( pGlobals, "Expected globals" );
	if (pGlobals->pTableInfo == NULL)
	{

		// init string table library
		int nNumLanguages = LanguageGetNumLanguages( AppGameGet() );
		int nDefaultTableIndex = sStringTableGetIndex( STE_DEFAULT );
		StringTableCommonInit( nNumLanguages, nDefaultTableIndex, DEFAULT_ATTRIBUTE_NAME );

		// allocate memory for the table infos
		pGlobals->nNumInfo = nNumLanguages;
		
#if USE_MEMORY_MANAGER && !ISVERSION(SERVER_VERSION)
		pGlobals->pTableInfo = (STRING_TABLE_INFO *)MALLOCZ( g_StaticAllocator, sizeof( STRING_TABLE_INFO ) * pGlobals->nNumInfo );
#else		
		pGlobals->pTableInfo = (STRING_TABLE_INFO *)MALLOCZ( NULL, sizeof( STRING_TABLE_INFO ) * pGlobals->nNumInfo );
#endif

		// init each entry
		for (int i = 0; i < pGlobals->nNumInfo; ++i)
		{
			STRING_TABLE_INFO *pTableInfo = &pGlobals->pTableInfo[ i ];
			sStringTableInfoInit( pTableInfo );
		}
				
		// load the data filenames
		sLoadAllDataFileNames( eAppGame );

	}
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static DWORD sGetDebugStringFlags(
	DWORD dwStringFlags)
{	
	if (AppTestDebugFlag( ADF_TEXT_LABELS ))
	{	
		SETBIT( dwStringFlags, SF_RETURN_LABEL_BIT );
	}
	return dwStringFlags;
}
	
//----------------------------------------------------------------------------
// return the index for a given keystr
//----------------------------------------------------------------------------
int StringTableGetStringIndexByKey(
	const char * keystr,
	STRING_TABLE_ENUM eTable /*= STE_DEFAULT*/)
{
	int nStringTableIndex = sStringTableGetIndex( eTable );
	return StringTableCommonGetStringIndexByKey( nStringTableIndex, keystr );
}

//----------------------------------------------------------------------------
// return a string by index
//----------------------------------------------------------------------------
const WCHAR * StringTableGetStringByIndex(
	int nStringIndex,
	STRING_TABLE_ENUM eTable /*= STE_DEFAULT*/)
{
	return StringTableGetStringByIndexVerbose(
		nStringIndex,
		0,
		0,
		NULL,
		NULL,
		eTable);
}	

//----------------------------------------------------------------------------
// return a string by index using matching attributes etc
//----------------------------------------------------------------------------
const WCHAR * StringTableGetStringByIndexVerbose(
	int nStringIndex,
	DWORD dwAttributesToMatch /*= 0*/,
	DWORD dwStringFlags,
	DWORD *pdwAttributesOfString /*= NULL*/,
	BOOL *pbFound /*= NULL*/,
	STRING_TABLE_ENUM eTable /*= STE_DEFAULT*/)
{
	int nStringTableIndex = sStringTableGetIndex( eTable );
	
	// tack on some extra flags based on app level debug flags
	dwStringFlags = sGetDebugStringFlags( dwStringFlags );

	// text length debugging	
	if (AppTestDebugFlag( ADF_TEXT_LENGTH ))
	{
		return guszTextLength;
	}
	
	return StringTableCommonGetStringByIndexVerbose(
		nStringTableIndex,
		nStringIndex,
		dwAttributesToMatch,
		dwStringFlags,
		pdwAttributesOfString,
		pbFound);
}	

//----------------------------------------------------------------------------
// return a string by key lookup
//----------------------------------------------------------------------------
const WCHAR * StringTableGetStringByKey(
	const char *pszKey,
	STRING_TABLE_ENUM eTable /*= STE_DEFAULT*/)
{	
	return StringTableGetStringByKeyVerbose( pszKey, 0, 0, NULL, NULL, eTable );
}

//----------------------------------------------------------------------------
// return a string by key lookup with verbose info
//----------------------------------------------------------------------------
const WCHAR * StringTableGetStringByKeyVerbose(
	const char *pszKey,
	DWORD dwAttributesToMatch,
	DWORD dwStringFlags,
	DWORD *pdwAttributesOfString,
	BOOL *pbFound,
	STRING_TABLE_ENUM eTable /*= STE_DEFAULT*/)
{
	int nStringTableIndex = sStringTableGetIndex( eTable );
	
	// tack on some extra flags based on app level debug flags
	dwStringFlags = sGetDebugStringFlags( dwStringFlags );

	// text length debugging	
	if (AppTestDebugFlag( ADF_TEXT_LENGTH ))
	{
		return guszTextLength;
	}
	
	return StringTableCommonGetStringByKeyVerbose(
		nStringTableIndex,
		pszKey,
		dwAttributesToMatch,
		dwStringFlags,
		pdwAttributesOfString,
		pbFound);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const char *StringTableGetKeyByStringIndex(
	int nStringIndex,
	STRING_TABLE_ENUM eTable /*= STE_DEFAULT*/)
{
	int nStringTableIndex = sStringTableGetIndex( eTable );
	return StringTableCommonGetKeyByStringIndex( nStringTableIndex, nStringIndex );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
STRING_TABLE_INFO *sStringTableInfoGet(
	LANGUAGE eLanguage)
{
	ASSERTX_RETNULL( eLanguage != LANGUAGE_INVALID, "Expected language" );
	STRING_TABLE_GLOBALS *pGlobals = sStringTableGlobalsGet();
	ASSERTX_RETNULL( pGlobals, "Expected globals" );
	ASSERTX_RETNULL( pGlobals->pTableInfo, "Expected table info" );
	ASSERTX_RETNULL( eLanguage >= 0 && eLanguage < pGlobals->nNumInfo, "Invalid language index" );
	return &pGlobals->pTableInfo[ eLanguage ];
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sStringTableDataIsCurrent(
	LANGUAGE eLanguage)
{
	
	// get language
	APP_GAME eAppGame = AppGameGet();	
	const LANGUAGE_DATA *pLanguageData = LanguageGetData( eAppGame, eLanguage );
			
	// get the string table
	STRING_TABLE_INFO *pTableInfo = sStringTableInfoGet( eLanguage );
	
	// check each file
	for (int i = 0; i < pTableInfo->nNumDataFiles; ++i)
	{
		const STRING_DATA_FILE *pDataFile = &pTableInfo->tDataFile[ i ];

		// must be loaded by game
		if (pDataFile->bLoadedByGame)
		{
					
			// get source file
			char szSourceFile[ MAX_PATH ];
			PStrPrintf( 
				szSourceFile, 
				MAX_PATH, 
				"%s%s\\%s", 
				StringTableCommonGetStringFileGetExcelPath( pDataFile->bIsCommon ), 
				pLanguageData->szDataDirectory, 
				pDataFile->szDataFile);

			// get cooked file
			char szCookedFile[ MAX_PATH ];
			PStrPrintf( 
				szCookedFile, 
				MAX_PATH, 
				"%s.%s", 
				szSourceFile, 
				COOKED_FILE_EXTENSION);
			
			if (FileNeedsUpdate(szCookedFile, szSourceFile, STRING_TABLE_MAGIC, STRING_TABLE_VERSION, FILE_UPDATE_UPDATE_IF_NOT_IN_PAK))
			{
				return FALSE;
			}

		}
		
	}

	return TRUE;
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sAppLoadsLanguage(
	LANGUAGE eLanguage,
	int nSKU,
	BOOL bAllowDevLanguage)
{
	ASSERTX_RETFALSE( eLanguage != LANGUAGE_INVALID, "Invalid language" );
	
	if (AppCommonGetForceLoadAllLanguages()) 
	{
		return TRUE;
	}

	// if we have a SKU specified, we need to check the language datatable
	if (nSKU != INVALID_LINK)
	{
		if (SKUContainsLanguage( nSKU, eLanguage, bAllowDevLanguage ))
		{
			return TRUE;
		}
	}
	else
	{
		
		// we load the current language selected by the user
		if (eLanguage == LanguageGetCurrent())
		{
			return TRUE;
		}
		
		// we load the development language if one is allowed
		if (bAllowDevLanguage)
		{
			int eLanguageDev = LanguageGetDevLanguage();
			if (eLanguageDev != LANGUAGE_INVALID && eLanguage == eLanguageDev)
			{
				return TRUE;
			}
			
		}
		
	}
		
	return FALSE;
	
}
			
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL StringTablesAreAllCurrent(
	void)
{

	// init string tables
	APP_GAME eAppGame = AppGameGet();
	StringTableInit( eAppGame );

	// check all string tables
	STRING_TABLE_GLOBALS *pGlobals = sStringTableGlobalsGet();		
	for (int nLanguage = 0; nLanguage < pGlobals->nNumInfo; ++nLanguage)
	{
		LANGUAGE eLanguage = (LANGUAGE)nLanguage;
		if (sAppLoadsLanguage( eLanguage, INVALID_LINK, TRUE ))
		{
			if (sStringTableDataIsCurrent( eLanguage ) == FALSE)
			{
				return FALSE;
			}
		}
	}

	// when forcing a language via the command line, we will always load direct
	if (LanguageOverrideIsEnabled())
	{
		return FALSE;
	}
		
	// it's all good!
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sLoadStringDataFile(
	LANGUAGE eLanguage,
	const STRING_DATA_FILE *pDataFile,
	BOOL bForceDirect,
	BOOL bAddToPak)
{				
	ASSERTX_RETFALSE( eLanguage != LANGUAGE_INVALID, "Expected language" );
	ASSERTX_RETFALSE( pDataFile, "Expected string data file" );

	// get the language data
	APP_GAME eAppGame = AppGameGet();	
	const LANGUAGE_DATA *pLanguageData = LanguageGetData( eAppGame, eLanguage );
	ASSERTX_RETFALSE( pLanguageData, "Expected language data" );

	// construct full path to file
	const char *pszFilename = pDataFile->szDataFile;
	char szFullFilename[ MAX_PATH ];
	PStrCopy( szFullFilename, pLanguageData->szDataDirectory, arrsize( szFullFilename ) );
	PStrForceBackslash( szFullFilename, MAX_PATH );
	PStrCat( szFullFilename, pszFilename, MAX_PATH );
			
	// load file
	BOOL bResult = ExcelCommonLoadStringTable( 
		szFullFilename, 
		pDataFile->nStringFileIndex,
		pDataFile->bIsCommon, 
		bForceDirect, 
		eLanguage, 
		bAddToPak);
		
	// check for error		
	if (bResult == FALSE)
	{
		const int MAX_MESSAGE_SIZE = 1048;
		CHAR szMessage[ MAX_MESSAGE_SIZE ];
		PStrPrintf( szMessage, MAX_MESSAGE_SIZE, "Unable to load string table '%s'", szFullFilename );
		ASSERTX( bResult, szMessage );
		return FALSE;
	}
	
	return TRUE;  // success
	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sLoadStringTableData(
	LANGUAGE eLanguage,
	BOOL bForceDirect,
	BOOL bAddToPak)
{	
	// get string table info
	STRING_TABLE_INFO *pTableInfo = sStringTableInfoGet(eLanguage);
				
	// load all string data files
	for (int i = 0; i < pTableInfo->nNumDataFiles; ++i)
	{
		// load string data file
		const STRING_DATA_FILE *pDataFile = &pTableInfo->tDataFile[i];

		// skip files not loaded by the game		
		if (pDataFile->bLoadedByGame == FALSE)
		{
			continue;
		}
		
		// do the load
		sLoadStringDataFile(eLanguage, pDataFile, bForceDirect, bAddToPak);
	}

	return TRUE;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
EXCEL_UPDATE_STATE StringTablesLoadAll(
	void)
{

	// we will not add to pack files via this normal excel initialization
	// if we are filling pak files ... strings are part of the SKU pack process
	BOOL bAddToPak = TRUE;
	if (AppIsFillingPak() && PakFileUsedInApp(PAK_LOCALIZED))
	{
		bAddToPak = FALSE;
	}

	EXCEL_UPDATE_STATE eResult = EXCEL_UPDATE_STATE_ERROR;
	BOOL bCurrent = (!AppCommonGetRegenExcel() && StringTablesAreAllCurrent());
	if (bCurrent)
	{
		ExcelTrace("\nEXCEL::ExcelStringTablesNeedUpdate()   UP TO DATE\n");
		BOOL bSuccess = StringTablesLoad(FALSE, bAddToPak, INVALID_LINK);
		if (bSuccess)
		{
			eResult = EXCEL_UPDATE_STATE_UPTODATE;
		}
	}
	else
	{
		ExcelTrace("\nEXCEL::ExcelStringTablesNeedUpdate()   UPDATING\n");
		BOOL bSuccess = StringTablesLoad(TRUE, bAddToPak, INVALID_LINK);
		if (bSuccess)
		{
			eResult = EXCEL_UPDATE_STATE_UPDATED;
		}
		
	}
	return eResult;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL StringTablesLoad(
	BOOL bForceDirect,
	BOOL bAddToPak,
	int nSKU)
{
	TIMER_STARTEX("Loading all string tables", 100);
	APP_GAME eAppGame = AppGameGet();		
	
	// when filling a pak, we must have the language set to english ... all other
	// languages will be forced to have the exact string order as the english version
	BOOL bFillingPak = AppIsFillingPak();
	if (bFillingPak)
	{
		LANGUAGE eLanguage = LanguageGetCurrent();
		LANGUAGE eLanguageDev = LanguageGetDevLanguage();
		ASSERTX_RETFALSE( eLanguageDev != LANGUAGE_INVALID, "Development language must be set for fillpak" );
		ASSERTV_RETFALSE( 
			eLanguage == eLanguageDev, 
			"Fillpak *must* run in the development language '%s'",
			LanguageGetName( eAppGame, eLanguageDev ));
	}
	
	// do we allow loading of the development language
	BOOL bAllowDevLanguage = FALSE;
	#if ISVERSION( DEVELOPMENT )
		bAllowDevLanguage = TRUE;
	#endif
	
	// when filling the localized pak, we will never include the dev language unless
	// specifically requested by the fillpak command line, we need this because we normally
	// fill paks from an .exe built with a development configuration, and in general 
	// we don't ever want to consider the dev language when making pak builds.  However, for 
	// pak builds that are packaging development versions (like a debug optimized build for QA that
	// has cheats enabled) we need to include the dev language because the development version
	// of the game will be looking for it when it starts up (because it's development of course)
	// this would all be greatly cleaned up by making the fillpak .exe built using another
	// configuration, or relase or something, but that is a much larger change (if we even can) -cday
	if (bFillingPak && 
		AppTestDebugFlag( ADF_FILL_LOCALIZED_PAK_BIT ) && 
		StringTableFillPakDevLanguage() == FALSE)
	{
		bAllowDevLanguage = FALSE;
	}
	
	// init system
	StringTableInit( eAppGame );

	// go through each string table desired and load it
	STRING_TABLE_GLOBALS *pGlobals = sStringTableGlobalsGet();	
	for (int nLanguage = 0; nLanguage < pGlobals->nNumInfo; ++nLanguage)
	{
		LANGUAGE eLanguage = (LANGUAGE)nLanguage;
		if (sAppLoadsLanguage(eLanguage, nSKU, bAllowDevLanguage))
		{
			if (sLoadStringTableData(eLanguage, bForceDirect, bAddToPak) == FALSE)
			{
				return FALSE;
			}
		}
	}	
	
	// global indices must now re-lookup thier strings
#if !ISVERSION(SERVER_VERSION)	
	GlobalStringCacheStrings();
#endif
	
	return TRUE;		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sFindFirstTable(
	STRING_TABLE_GLOBALS *pGlobals)
{
	ASSERTX_RETINVALID( pGlobals, "Expected globals" );
	
	for (int i = 0; i < pGlobals->nNumInfo; ++i)
	{
		if (StringTableCommonGetCount( i ) > 0)
		{
			return i;  // return index to table
		}
	}
	
	return NONE;
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static const char *sStringTableGetName(
	APP_GAME eAppGame,
	int nTableIndex)
{
	const STRING_TABLE_GLOBALS *pGlobals = sStringTableGlobalsGet();
	ASSERTX_RETNULL( pGlobals, "Expected globals" );
	ASSERTX_RETNULL( nTableIndex >= 0 && nTableIndex < pGlobals->nNumInfo, "Invalid table index" );
	
	// table index is our language
	LANGUAGE eLanguage = (LANGUAGE)nTableIndex;  
	return LanguageGetName( eAppGame, eLanguage );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sLogStringTableCompareError(
	const char *pszMessage,
	const OS_PATH_CHAR *pszResultFilename)
{
	ASSERTX_RETURN( pszMessage, "Expected message" );
	
	// log to an easy place for the build system to get the error
	LogMessage( SKU_LOG, pszMessage );

	// create a file with the failure result message in it
	HANDLE hFile = FileOpen( pszResultFilename, GENERIC_WRITE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL );
	if (hFile)
	{
		int nMessageLength = PStrLen( pszMessage );
		FileWrite( hFile, pszMessage, nMessageLength );
		FileClose( hFile );
	}
		
	// assert
	ASSERTX( 0, pszMessage );
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sCompareStringTables( 
	APP_GAME eAppGame,
	int nTableIndexA,
	int nTableIndexB,
	const OS_PATH_CHAR *pszResultFilename)
{
	ASSERTX_RETFALSE( nTableIndexA != NONE, "Expected string table A" );
	ASSERTX_RETFALSE( nTableIndexA != NONE, "Expected string table B" );
	const int MAX_MESSAGE = 256;
	char szMessage[ MAX_MESSAGE ];

	// go through the entries
	int nNumEntriesA = StringTableCommonGetCount( nTableIndexA );
	for (int i = 0; i < nNumEntriesA; ++i)
	{
		
		// get string key node from A
		const char *pszKeyA = StringTableCommonGetKeyByKeyIndex( i, nTableIndexA );
		ASSERTX_RETFALSE( pszKeyA, "Expected key from table A" );
		
		// get string key node from B
		const char *pszKeyB = StringTableCommonGetKeyByKeyIndex( i, nTableIndexB );
		if (pszKeyB == NULL)
		{
			PStrPrintf( 
				szMessage, 
				MAX_MESSAGE, 
				"String key '%s' [%d] found in table '%s' but is not present in table '%s'",
				pszKeyA,
				i,
				sStringTableGetName( eAppGame, nTableIndexA ),
				sStringTableGetName( eAppGame, nTableIndexB ));
				
			// log the error
			sLogStringTableCompareError( szMessage, pszResultFilename );
			
			// you have failed!
			return FALSE;

		}
		else
		{
			if (PStrCmp( pszKeyA, pszKeyB ) != 0)
			{
				PStrPrintf( 
					szMessage, 
					MAX_MESSAGE, 
					"String key mismatch '%s' [%d] found in table '%s' but is '%s' [%d] in table '%s'",
					pszKeyA,
					i,
					sStringTableGetName( eAppGame, nTableIndexA ),
					pszKeyB,
					i,
					sStringTableGetName( eAppGame, nTableIndexB ));
					
				// log the error
				sLogStringTableCompareError( szMessage, pszResultFilename );					
				
				// you have failed my son!
				return FALSE;
				
			}
			
		}
		
	}
	
	return TRUE;
			
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sAllLanguageTablesAreIdentical(
	const OS_PATH_CHAR *pszResultFilename)
{
	STRING_TABLE_GLOBALS *pGlobals = sStringTableGlobalsGet();
	APP_GAME eAppGame = AppGameGet();

	// find the first language that is loaded with data
	int nTableIndexFirst = sFindFirstTable( pGlobals );
	ASSERTX_RETFALSE( nTableIndexFirst != NONE, "No loaded string tables found" );
	
	// iterate each language we have loaded
	for (int nTableIndexOther = nTableIndexFirst + 1; nTableIndexOther < pGlobals->nNumInfo; ++nTableIndexOther)
	{
		
		// only check if string table has actual data
		if (StringTableCommonGetCount( nTableIndexOther ) > 0)
		{
		
			// compare table against the first one
			if (sCompareStringTables( eAppGame, nTableIndexFirst, nTableIndexOther, pszResultFilename ) == FALSE)
			{
				return FALSE;
			}
			
		}
		
	}

	return TRUE;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL StringTablesFillPak(
	int nSKU)
{
	ASSERTX_RETFALSE( nSKU != INVALID_LINK, "Expected SKU link" );
	ASSERTX_RETFALSE( PakFileUsedInApp( PAK_LOCALIZED ), "Localized pak is not in use for this app" );
	
	// verify this SKU
#if ISVERSION(DEVELOPMENT)	
	if (StringTableCompareSKULanguages( nSKU ) == FALSE)
	{
		const SKU_DATA *pSKUData = SKUGetData( nSKU );
		ASSERTV_RETFALSE( 0, "!!! PAKSKU '%s' failed - Strings are not synchronized !!!", pSKUData->szName );
	}
#endif

	// clear out the tables
	StringTableFree();
		
	// load all the languages included in this SKU and add them to the pak file
	StringTablesLoad( TRUE, TRUE, nSKU );

	// load up the default tables again just incase something after this point needs them
	StringTableFree();
	StringTableInit( AppGameGet() );
		
	return TRUE;	
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sTouchDataFile(
	const FILE_ITERATE_RESULT *pResult,
	int nStringFileIndex,
	BOOL bIsCommon,
	BOOL bLoadedByGame,
	void *pUserData)
{
	UNREFERENCED_PARAMETER(pUserData);
	UNREFERENCED_PARAMETER(bIsCommon);
	UNREFERENCED_PARAMETER(bLoadedByGame);
	UNREFERENCED_PARAMETER(nStringFileIndex);
	FileTouch( pResult->szFilenameRelativepath );	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StringTableTouchSourceFiles(
	LANGUAGE eLanguage)
{	

	// iterate data files for language
	sStringDataFilesIterate( AppGameGet(), eLanguage, sTouchDataFile, NULL );	
					
}
		
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD StringTableAddAttribute(
	LANGUAGE eLanguage,
	const WCHAR *puszAttributeName)
{
	return StringTableCommonAddAttribute( eLanguage, puszAttributeName );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD StringTableGetGrammarNumberAttribute(
	GRAMMAR_NUMBER eNumber,
	STRING_TABLE_ENUM eTable /*= STE_DEFAULT*/)
{	
	int nStringTableIndex = sStringTableGetIndex( eTable );
	return StringTableCommonGetGrammarNumberAttribute( nStringTableIndex, eNumber );	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD StringTableGetUnder18Attribute(
	STRING_TABLE_ENUM eTable /*= STE_DEFAULT*/)
{	
	int nStringTableIndex = sStringTableGetIndex( eTable );

	// get bit used to represent attribute
	BOOL bFound = FALSE;
	int nBit = StringTableCommonGetAttributeBit( nStringTableIndex, UNDER18_ATTRIBUTE_NAME, &bFound );
	
	// return as a bit mask
	DWORD dwAttributeFlags = 0;		
	if (bFound == TRUE)
	{
		SETBIT( dwAttributeFlags, nBit );
	}
	
	return dwAttributeFlags;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD StringTableGetTimeCodeAttribute(
	BOOL *pbFound /*= NULL*/,
	STRING_TABLE_ENUM eTable /*= STE_DEFAULT*/)
{
	int nStringTableIndex = sStringTableGetIndex( eTable );
	const WCHAR *puszAttributeName = TIMECODE_ATTRIBUTE_NAME;

	// get bit used to represent attribute
	BOOL bFound = FALSE;
	int nBit = StringTableCommonGetAttributeBit( nStringTableIndex, puszAttributeName, &bFound );

	// set found param if present
	if (pbFound)
	{
		*pbFound = bFound;
	}
		
	// return as a bit mask
	DWORD dwAttributeFlags = 0;	
	if (bFound)
	{
		SETBIT( dwAttributeFlags, nBit );
	}
	return dwAttributeFlags;
	
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD StringTableGetGenderAttribute(
	enum GENDER eGender,
	STRING_TABLE_ENUM eTable /*= STE_DEFAULT*/)
{
	int nStringTableIndex = sStringTableGetIndex( eTable );
	const WCHAR *puszAttributeName = NULL;
	switch (eGender)
	{
		case GENDER_MALE:	puszAttributeName = MASCULINE_ATTRIBUTE_NAME; break;
		case GENDER_FEMALE:	puszAttributeName = FEMININE_ATTRIBUTE_NAME; break;
		case GENDER_NEUTER:	puszAttributeName = NEUTER_ATTRIBUTE_NAME; break;
	}
	
	// get bit used to represent attribute
	BOOL bFound = FALSE;
	int nBit = StringTableCommonGetAttributeBit( nStringTableIndex, puszAttributeName, &bFound );
	
	// return as a bit mask
	DWORD dwAttributeFlags = 0;		
	if (bFound == TRUE)
	{
		SETBIT( dwAttributeFlags, nBit );
	}
	
	return dwAttributeFlags;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int StringTableGetAttributeBit(
	const WCHAR *puszAttributeName,
	STRING_TABLE_ENUM eTable /*= STE_DEFAULT*/)
{
	ASSERTX_RETINVALID( puszAttributeName, "Expected attribute" );
	int nStringTableIndex = sStringTableGetIndex( eTable );

	// get bit used to represent attribute
	BOOL bFound = FALSE;
	int nBit = StringTableCommonGetAttributeBit( nStringTableIndex, puszAttributeName, &bFound );

	// return bit
	return nBit;
		
}

// CHB 2006.09.07 - Is referenced using ISVERSION(DEVELOPMENT) in consolecmd.cpp.
#if defined(_DEBUG) || ISVERSION(DEVELOPMENT)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sVerifyError( 
	const char *pszKey,
	const char *pszError,
	const WCHAR *puszTextDev, 
	const WCHAR *puszTextLang)
{
	const int MAX_MESSAGE = 1024;
	
	char szTextDev[ MAX_MESSAGE ];
	PStrCvt( szTextDev, puszTextDev, MAX_MESSAGE );
	
	char szTextLang[ MAX_MESSAGE ];
	PStrCvt( szTextLang, puszTextLang, MAX_MESSAGE );

	char szMessage[ MAX_MESSAGE ];	
	PStrPrintf( 
		szMessage, 
		MAX_MESSAGE, 
		"Key='%s' Error='%s'\n", 
		pszKey,
		pszError);
	trace( szMessage );
	CLT_VERSION_ONLY( LogText( ERROR_LOG, szMessage ) );
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sVerifyFormatSpecifiers(
	const char *pszKey,
	char cPrefix,
	const WCHAR *puszTextDev,
	const WCHAR *puszTextLang)
{
	const int MAX_FORMAT_SPECIFIERS = 32;
	WCHAR cFormatSpecifiers[ MAX_FORMAT_SPECIFIERS ];
	int nNumFormatSpecifiers = 0;
	
	// gather up all the printf format specifiers, making sure to ignore
	int nLengthDev = PStrLen( puszTextDev );
	for (int i = 0; i < nLengthDev; ++i)
	{
		if (puszTextDev[ i ] == cPrefix)
		{
			// store the next character
			if (i + 1 < nLengthDev)
			{
				cFormatSpecifiers[ nNumFormatSpecifiers++ ] = puszTextDev[ i + 1 ];
			}
			else
			{
				sVerifyError( pszKey, "Source text incomplete format specifier", puszTextDev, puszTextLang );
				return FALSE;
			}
			i++;  // skip the format specifier itself
		}
	}

	// scan the language string
	int nCurrentSpecifier = 0;
	int nLengthLang = PStrLen( puszTextLang );
	for (int i = 0; i < nLengthLang; ++i)
	{
		if (puszTextLang[ i ] == cPrefix)
		{
			// look at the next character
			if (i + 1 < nLengthLang)
			{
				if (cFormatSpecifiers[ nCurrentSpecifier++ ] != puszTextLang[ i + 1 ])
				{
					sVerifyError( pszKey, "Format specifier mismatch", puszTextDev, puszTextLang );
					return FALSE;
				}
			}
			else
			{
				sVerifyError( pszKey, "Language text incomplete format specifier", puszTextDev, puszTextLang );
				return FALSE;
			}
			i++;  // skip the format specifier itselv			
		}	
		
	}

	if (nCurrentSpecifier != nNumFormatSpecifiers)
	{
		sVerifyError( pszKey, "Language text missing format specifiers", puszTextDev, puszTextLang );
		return FALSE;
	}
	else
	{
		return TRUE;
	}
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sVerifyString( 
	const char *pszKey,
	const WCHAR *puszTextDev, 
	const WCHAR *puszTextLang)
{
	int nNumErrors = 0;
		
	// verify for %X specifiers
	if (sVerifyFormatSpecifiers( pszKey, '%', puszTextDev, puszTextLang ) == FALSE)
	{
		nNumErrors++;
	}
	
	// verify for \ specifiers
	if (sVerifyFormatSpecifiers( pszKey, '\\', puszTextDev, puszTextLang ) == FALSE)
	{
		nNumErrors++;
	}
	
	return nNumErrors == 0;
	
}	

//----------------------------------------------------------------------------
#define MAX_STRINGS_AT_NODE (256)

//----------------------------------------------------------------------------
struct ALL_STRINGS_AT_NODE
{
	const WCHAR *puszString[ MAX_STRINGS_AT_NODE ];
	int nNumStringsAtNode;	
};

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sGetAllStringsAtKey(
	const STRING_ENTRY_INFO &tEntryInfo,
	void *pCallbackData)
{
	ALL_STRINGS_AT_NODE *pAllStrings = (ALL_STRINGS_AT_NODE *)pCallbackData;
	ASSERTX_RETURN( pAllStrings->nNumStringsAtNode < MAX_STRINGS_AT_NODE - 1, "Too many strings" );
	pAllStrings->puszString[ pAllStrings->nNumStringsAtNode++ ] = tEntryInfo.puszString;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sVerifyLanguageStringByKey( 
	const char *pszKey,
	LANGUAGE eLanguageDev,
	LANGUAGE eLanguageToVerify)
{
	ASSERTX_RETTRUE( pszKey, "Expected string key" );
	int nNumErrors = 0;

	// get all the strings in the developer string table at this key node
	ALL_STRINGS_AT_NODE tDevStrings;
	tDevStrings.nNumStringsAtNode = 0;
	StringTableCommonIterateStringsAtKey( eLanguageDev, pszKey, sGetAllStringsAtKey, &tDevStrings );
	ASSERTX_RETFALSE( tDevStrings.nNumStringsAtNode, "No strings found in dev table at key index" );

	// get all the strings in the language to test at this key node
	ALL_STRINGS_AT_NODE tLangStrings;
	tLangStrings.nNumStringsAtNode = 0;
	StringTableCommonIterateStringsAtKey( eLanguageToVerify, pszKey, sGetAllStringsAtKey, &tLangStrings );
	
	// we must have some language strings to verify
	if (tLangStrings.nNumStringsAtNode == 0)
	{
		const int MAX_MESSAGE = 1024;
		WCHAR uszKey[ MAX_MESSAGE ];
		PStrPrintf( uszKey, MAX_MESSAGE, L"%S", pszKey );
		sVerifyError( pszKey, "Missing node in language", uszKey, uszKey );
		nNumErrors++;	
	}
	else
	{
	
		// go through all the strings in the dev lang
		for (int nDevIndex = 0; nDevIndex < tDevStrings.nNumStringsAtNode; ++nDevIndex)
		{
		
			// get the dev string
			const WCHAR *puszDevString = tDevStrings.puszString[ nDevIndex ];
			
			// go through strings at language node
			for (int nLangIndex = 0; nLangIndex < tLangStrings.nNumStringsAtNode; ++nLangIndex)
			{
			
				// get lang string
				const WCHAR *puszLangString = tLangStrings.puszString[ nLangIndex ];
					
				// verify
				if (sVerifyString( pszKey, puszDevString, puszLangString ) == FALSE)
				{
					nNumErrors++;
				}
			
			}
						
		}
		
	}

	return nNumErrors == 0;
					
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void StringTableVerifyLanguage(
	void)
{
	trace( ">>> Begin String Table Verification\n" );
	CLT_VERSION_ONLY( LogText( ERROR_LOG, ">>> Begin String Table Verification\n" ) );

	// what is the dev language
	LANGUAGE eLanguageDev = LanguageGetDevLanguage();
	
	// what is the current language
	LANGUAGE eLanguageCurrent = LanguageGetCurrent();
	
	// go through all strings in the developer language
	int nCount = StringTableCommonGetCount( STE_DEVELOPER );
	int nNumErrors = 0;	
	for (int i = 0; i < nCount; ++i)
	{
		const char *pszKey = StringTableCommonGetKeyByKeyIndex( i, STE_DEVELOPER );
		if (pszKey)
		{
			if (sVerifyLanguageStringByKey( pszKey, eLanguageDev, eLanguageCurrent ) == FALSE)
			{
				nNumErrors++;
			}
		}

	}

	trace( "<<< String Table Verification Complete - '%d' Errors\n", nNumErrors );
//	ConsoleString( "<<< String Table Verification Complete - '%d' Errors\n", nNumErrors );
	CLT_VERSION_ONLY( LogText( ERROR_LOG, "<<< String Table Verification Complete - '%d' Errors\n", nNumErrors ) );
}

#endif

#if ISVERSION(DEVELOPMENT)
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL StringTableCompareSKULanguages(
	int nSKU)
{
	// To bypass the synchronized check for strings, uncomment this line:
	//return TRUE;

	ASSERTX_RETFALSE( nSKU != INVALID_LINK, "Expected SKU" );
	BOOL bResult = TRUE;

	// delete any result file
	OS_PATH_CHAR szFile[ MAX_PATH ];
	PStrPrintf( szFile, MAX_PATH, L"%S%S", FILE_PATH_DATA, SKU_VALIDATE_RESULT_FILENAME );
	if (FileExists( szFile ))
	{
		FileDelete( szFile );
	}
		
	// clear any data previously loaded
	StringTableFree();
	
	// load all languages for this SKU and to *not* add to pak
	StringTablesLoad( TRUE, FALSE, nSKU );
	
	// verify that all the strings are identical between each languages
	if (sAllLanguageTablesAreIdentical( szFile ) == FALSE)
	{
		bResult = FALSE;
	}

	// remove any string data loaded and load the regular tables again
	StringTableFree();
	StringTableInit( AppGameGet() );
		
	return bResult;
	
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int StringGetForApp(
	const int nStringArray[ APP_GAME_NUM_GAMES ])
{	
	int nString = nStringArray[ APP_GAME_ALL ];
	if (nString == INVALID_LINK)
	{
		APP_GAME eAppGame = APP_GAME_UNKNOWN;
		if (AppIsHellgate())
		{
			eAppGame = APP_GAME_HELLGATE;
		}
		else if (AppIsTugboat())
		{
			eAppGame = APP_GAME_TUGBOAT;
		}
		nString = nStringArray[ eAppGame ];
	}
	return nString;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL StringTableFillPakDevLanguage(
	void)
{
	return sgbStringTableFillDevLanguage;
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL StringTableSetFillPakDevLanguage(
	BOOL bEnable)
{
	sgbStringTableFillDevLanguage = bEnable;
	return sgbStringTableFillDevLanguage;
}	
