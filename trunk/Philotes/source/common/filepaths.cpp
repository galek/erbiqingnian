//----------------------------------------------------------------------------
// filepaths.cpp
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------

#include "appcommon.h"
#include "config.h"
#include "pakfile.h"
#include "filepaths.h"
#include "language.h"
#include "prime.h"

#ifndef _DATATABLES_H_
#include "datatables.h"
#endif

#include <shlobj.h>


//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------

#define OVERRIDE_PATH_CODE_MARKER_CHAR	'.'

//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
char FILE_PATH_DATA[MAX_PATH];
char FILE_PATH_DATA_COMMON[MAX_PATH];
char FILE_PATH_PAK_FILES[MAX_PATH];

char FILE_PATH_EXCEL_COMMON[MAX_PATH];
char FILE_PATH_EXCEL[MAX_PATH];
char FILE_PATH_STRINGS[MAX_PATH];
char FILE_PATH_STRINGS_COMMON[MAX_PATH];

char FILE_PATH_UNITS[MAX_PATH];
char FILE_PATH_MONSTERS[MAX_PATH];
char FILE_PATH_PLAYERS[MAX_PATH];
char FILE_PATH_ITEMS[MAX_PATH];
char FILE_PATH_WEAPONS[MAX_PATH];
char FILE_PATH_MISSILES[MAX_PATH];
char FILE_PATH_OBJECTS[MAX_PATH];
char FILE_PATH_BACKGROUND[MAX_PATH];
char FILE_PATH_BACKGROUND_PROPS[MAX_PATH];
char FILE_PATH_BACKGROUND_DEBUG[MAX_PATH];
char FILE_PATH_SKILLS[MAX_PATH];
char FILE_PATH_STATES[MAX_PATH];
char FILE_PATH_ICONS[MAX_PATH];

char FILE_PATH_TOOLS[MAX_PATH];
char FILE_PATH_UI_XML[MAX_PATH];

char FILE_PATH_GLOBAL[MAX_PATH];

char FILE_PATH_PARTICLE[MAX_PATH];
char FILE_PATH_PARTICLE_ROPE_PATHS[MAX_PATH];

char FILE_PATH_EFFECT_COMMON[MAX_PATH];
char FILE_PATH_EFFECT[MAX_PATH];
char FILE_PATH_EFFECT_SOURCE[MAX_PATH];
char FILE_PATH_SOUND[MAX_PATH];
char FILE_PATH_SOUND_REVERB[MAX_PATH];
char FILE_PATH_SOUND_ADSR[MAX_PATH];
char FILE_PATH_SOUND_EFFECTS[MAX_PATH];
char FILE_PATH_SOUND_SYNTH[MAX_PATH];
char FILE_PATH_SOUND_HIGHQUALITY[MAX_PATH];
char FILE_PATH_SOUND_LOWQUALITY[MAX_PATH];

char FILE_PATH_MATERIAL[MAX_PATH];
char FILE_PATH_LIGHT[MAX_PATH];
char FILE_PATH_PALETTE[MAX_PATH];
char FILE_PATH_AI[MAX_PATH];
char FILE_PATH_SCRIPT[MAX_PATH];
char FILE_PATH_SCREENFX[MAX_PATH];
char FILE_PATH_DEMOLEVEL[MAX_PATH];

char FILE_PATH_HAVOKFX_SHADERS[MAX_PATH];

static OS_PATH_CHAR szSystemPaths[FILE_PATH_SYS_MAX][MAX_PATH];

//----------------------------------------------------------------------------
// IMPLEMENTATIONS
//----------------------------------------------------------------------------

const OS_PATH_CHAR * FilePath_GetSystemPath(FILE_PATH_SYS_ENUM nWhich)
{
	ASSERT_RETNULL(nWhich < FILE_PATH_SYS_MAX);
	return szSystemPaths[nWhich];
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static
const OS_PATH_CHAR * FilePath_GetAppName(APP_GAME eAppGame)
{
	switch (eAppGame)
	{
		case APP_GAME_HELLGATE:
			return OS_PATH_TEXT("Hellgate");
		case APP_GAME_TUGBOAT:
			return OS_PATH_TEXT("Mythos");
		default:
			ASSERT_RETNULL(false);
			return 0;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const LEVEL_FILE_PATH *LevelFilePathGetData(
	int nLine)
{
	return (const LEVEL_FILE_PATH *)ExcelGetData(EXCEL_CONTEXT(), DATATABLE_LEVEL_FILE_PATHS, nLine);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void FilePathInitForApp(
	APP_GAME eAppGame)
{
#if !ISVERSION(SERVER_VERSION)
	// Get LUA folders
	OS_PATH_CHAR szLocalAppdata[MAX_PATH];
	V( OS_PATH_FUNC(SHGetFolderPath)( NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, szLocalAppdata ) );
	
	// Public (user-editable) user data.
	OS_PATH_CHAR szPublicUserData[MAX_PATH];
	V( OS_PATH_FUNC(SHGetFolderPath)( NULL, CSIDL_PERSONAL, NULL, SHGFP_TYPE_CURRENT, szPublicUserData ) );
#if ISVERSION(DEMO_VERSION)
	PStrPrintf(szSystemPaths[FILE_PATH_PUBLIC_USER_DATA], _countof(szSystemPaths[FILE_PATH_PUBLIC_USER_DATA]), OS_PATH_TEXT("%s\\My Games\\%s Demo\\"), szPublicUserData, FilePath_GetAppName(eAppGame));
#else // ISVERSION(DEMO_VERSION)
	PStrPrintf(szSystemPaths[FILE_PATH_PUBLIC_USER_DATA], _countof(szSystemPaths[FILE_PATH_PUBLIC_USER_DATA]), OS_PATH_TEXT("%s\\My Games\\%s\\"), szPublicUserData, FilePath_GetAppName(eAppGame));
#endif // ISVERSION(DEMO_VERSION)
#else // !ISVERSION(SERVER_VERSION)
    PStrPrintf(szSystemPaths[FILE_PATH_PUBLIC_USER_DATA], _countof(szSystemPaths[FILE_PATH_PUBLIC_USER_DATA]),OS_PATH_TEXT("%s__user_data\\"),FilePath_GetAppName(eAppGame));
#endif // !ISVERSION(SERVER_VERSION)
	if( PStrStr(GetCommandLine(),"-buildserver") != NULL )
    {
        PStrPrintf(szSystemPaths[FILE_PATH_PUBLIC_USER_DATA], _countof(szSystemPaths[FILE_PATH_PUBLIC_USER_DATA]),OS_PATH_TEXT("%s_buildserver_logs\\"),FilePath_GetAppName(eAppGame));
    }

	// Try to create the directory here.
	// What to do if it fails?

	// root data directory
	if (eAppGame == APP_GAME_HELLGATE)
	{
		PStrPrintf( FILE_PATH_DATA,			MAX_PATH, "data\\");
	}
	else if (eAppGame == APP_GAME_TUGBOAT)
	{		
		PStrPrintf( FILE_PATH_DATA,			MAX_PATH, "data_mythos\\");
	}
	else
	{
		CLT_VERSION_ONLY(HALTX(false, "eAppGame is not valid"));	// CHB 2007.08.01 - String audit: USED IN RELEASE
        ASSERTX_RETURN(false, "eAppGame is not valid");
	}
	PStrPrintf( FILE_PATH_DATA_COMMON,		MAX_PATH, "data_common\\");
	PStrCopy(	FILE_PATH_PAK_FILES,		FILE_PATH_DATA,		MAX_PATH );

#if ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
	//	to support running both client and server on same machine, push
	//		server pak file down 1 dir in -desktop server mode.
	BOOL DesktopServerIsEnabled( void );
	if(DesktopServerIsEnabled())
	{
		PStrCat(FILE_PATH_PAK_FILES, "ServerPakFiles\\", MAX_PATH);
		ASSERTX(
			FileCreateDirectory(FILE_PATH_PAK_FILES),
			"Unable to create -desktop mode pak files directory.");
	}
#endif

	// *common* excel directory
	PStrPrintf( FILE_PATH_EXCEL_COMMON,		MAX_PATH, "%s%s", FILE_PATH_DATA_COMMON, "excel\\" );	

	// excel
	PStrPrintf( FILE_PATH_EXCEL,			MAX_PATH, "%s%s", FILE_PATH_DATA, "excel\\" );
	PStrPrintf( FILE_PATH_STRINGS,			MAX_PATH, "%s%s", FILE_PATH_DATA, "excel\\strings\\" );
	PStrPrintf( FILE_PATH_STRINGS_COMMON,	MAX_PATH, "%s%s", FILE_PATH_DATA_COMMON, "excel\\strings\\" );

	// units	
	PStrPrintf( FILE_PATH_UNITS,			MAX_PATH, "%s%s", FILE_PATH_DATA, "units\\" );
	PStrPrintf( FILE_PATH_MONSTERS,			MAX_PATH, "%s%s", FILE_PATH_DATA, "units\\monsters\\" );
	PStrPrintf( FILE_PATH_PLAYERS,			MAX_PATH, "%s%s", FILE_PATH_DATA, "units\\players\\" );
	PStrPrintf( FILE_PATH_ITEMS,			MAX_PATH, "%s%s", FILE_PATH_DATA, "units\\items\\" );
	PStrPrintf( FILE_PATH_WEAPONS,			MAX_PATH, "%s%s", FILE_PATH_DATA, "units\\items\\weapons\\" );
	PStrPrintf( FILE_PATH_MISSILES,			MAX_PATH, "%s%s", FILE_PATH_DATA, "units\\missiles\\" );
	PStrPrintf( FILE_PATH_OBJECTS,			MAX_PATH, "%s%s", FILE_PATH_DATA, "units\\objects\\" );
	
	PStrPrintf( FILE_PATH_BACKGROUND,		MAX_PATH, "%s%s", FILE_PATH_DATA, "background\\" );
	PStrPrintf( FILE_PATH_BACKGROUND_PROPS, MAX_PATH,						  "Props\\" );
	PStrPrintf( FILE_PATH_BACKGROUND_DEBUG, MAX_PATH, "%s%s", FILE_PATH_DATA, "background\\debug\\" );
	PStrPrintf( FILE_PATH_SKILLS,			MAX_PATH, "%s%s", FILE_PATH_DATA, "skills\\" );
	PStrPrintf( FILE_PATH_STATES,			MAX_PATH, "%s%s", FILE_PATH_DATA, "states\\" );
	PStrPrintf( FILE_PATH_ICONS,			MAX_PATH, "%s%s", FILE_PATH_DATA, "icons\\" );

	PStrPrintf( FILE_PATH_TOOLS,			MAX_PATH, "%s%s", FILE_PATH_DATA, "tools\\" );
	PStrPrintf( FILE_PATH_UI_XML,			MAX_PATH, "%s%s", FILE_PATH_DATA, "uix\\xml\\" );
	
	PStrPrintf( FILE_PATH_GLOBAL,			MAX_PATH, "%s", FILE_PATH_DATA );

	PStrPrintf( FILE_PATH_PARTICLE,				MAX_PATH, "%s%s", FILE_PATH_DATA, "particles\\" );
	PStrPrintf( FILE_PATH_PARTICLE_ROPE_PATHS,	MAX_PATH, "%s%s", FILE_PATH_DATA, "particles\\paths\\" );

	PStrPrintf( FILE_PATH_EFFECT_COMMON,	MAX_PATH, "%s%s", FILE_PATH_DATA_COMMON, "effects\\" );	
	PStrPrintf( FILE_PATH_EFFECT,			MAX_PATH, "%s%s", FILE_PATH_DATA, "effects\\" );
	PStrPrintf( FILE_PATH_EFFECT_SOURCE,	MAX_PATH,						  "source\\engine\\effects\\" );
	PStrPrintf( FILE_PATH_SOUND,			MAX_PATH,						  "sounds\\" );
	PStrPrintf( FILE_PATH_SOUND_REVERB,		MAX_PATH, "%s%s", FILE_PATH_DATA, "sounds\\reverb\\" );
	PStrPrintf( FILE_PATH_SOUND_ADSR,		MAX_PATH, "%s%s", FILE_PATH_DATA, "sounds\\adsr\\" );
	PStrPrintf( FILE_PATH_SOUND_EFFECTS,	MAX_PATH, "%s%s", FILE_PATH_DATA, "sounds\\effects\\" );
	PStrPrintf( FILE_PATH_SOUND_SYNTH,		MAX_PATH, "%s%s", FILE_PATH_DATA, "sounds\\synth\\" );

	PStrPrintf( FILE_PATH_MATERIAL,			MAX_PATH, "%s%s", FILE_PATH_DATA, "materials\\" );
	PStrPrintf( FILE_PATH_LIGHT,			MAX_PATH, "%s%s", FILE_PATH_DATA, "lights\\" );
	PStrPrintf( FILE_PATH_PALETTE,			MAX_PATH, "%s%s", FILE_PATH_DATA, "palettes\\" );
	PStrPrintf( FILE_PATH_AI,				MAX_PATH, "%s%s", FILE_PATH_DATA, "ai\\" );
	PStrPrintf( FILE_PATH_SCREENFX,			MAX_PATH, "%s%s", FILE_PATH_DATA, "screenfx\\" );
	PStrPrintf( FILE_PATH_DEMOLEVEL,		MAX_PATH, "%s%s", FILE_PATH_DATA, "demolevel\\" );

	// Maps to a private per-user location
	// Not sure if this should be in CSIDL_LOCAL_APPDATA, which is different on different machines.
	// Changing this to use data/ or data_mythos/ for consistency.                          -rli
	//PStrPrintf( FILE_PATH_SCRIPT,			MAX_PATH, "%s%s", local_appdata, "script\\" );
	PStrPrintf( FILE_PATH_SCRIPT,			MAX_PATH, "%s%s", FILE_PATH_DATA, "script\\" );

	// havok
	PStrPrintf( FILE_PATH_HAVOKFX_SHADERS,	MAX_PATH, "%s%s", FILE_PATH_DATA, "havokfx\\" );

	if (eAppGame == APP_GAME_HELLGATE)
	{
		// Sound quality directories
		PStrPrintf( FILE_PATH_SOUND_HIGHQUALITY,MAX_PATH,					  "high\\" );
		PStrPrintf( FILE_PATH_SOUND_LOWQUALITY,	MAX_PATH,					  "low\\" );
	}
	else if (eAppGame == APP_GAME_TUGBOAT)
	{
		// Sound quality directories
		PStrPrintf( FILE_PATH_SOUND_HIGHQUALITY,MAX_PATH,					  "" );
		PStrPrintf( FILE_PATH_SOUND_LOWQUALITY,	MAX_PATH,					  "" );
	}
	else
	{
		CLT_VERSION_ONLY(HALTX(false, "eAppGame is not valid"));	// CHB 2007.08.01 - String audit: USED IN RELEASE
        ASSERTX_RETURN(false, "eAppGame is not valid");
	}
	
	// All of these map to a private per-user location
	PStrPrintf( szSystemPaths[FILE_PATH_LOG],			MAX_PATH, OS_PATH_TEXT("%sLogs\\"),			FilePath_GetSystemPath(FILE_PATH_PUBLIC_USER_DATA) );
	//PStrPrintf( szSystemPaths[FILE_PATH_SAVE],			MAX_PATH, OS_PATH_TEXT("%sSave\\"),			FilePath_GetSystemPath(FILE_PATH_PUBLIC_USER_DATA) );			
	PStrPrintf( szSystemPaths[FILE_PATH_SAVE_BASE],					MAX_PATH, OS_PATH_TEXT("%sSave\\"),							FilePath_GetSystemPath(FILE_PATH_PUBLIC_USER_DATA) );			
	PStrPrintf( szSystemPaths[FILE_PATH_SINGLEPLAYER_SAVE],			MAX_PATH, OS_PATH_TEXT("%sSave\\Singleplayer\\"),			FilePath_GetSystemPath(FILE_PATH_PUBLIC_USER_DATA) );			
	PStrPrintf( szSystemPaths[FILE_PATH_MULTIPLAYER_SAVE],			MAX_PATH, OS_PATH_TEXT("%sSave\\Multiplayer\\"),			FilePath_GetSystemPath(FILE_PATH_PUBLIC_USER_DATA) );			
	PStrPrintf( szSystemPaths[FILE_PATH_DEMO_SAVE],					MAX_PATH, OS_PATH_TEXT("%sSave\\"),			FilePath_GetSystemPath(FILE_PATH_PUBLIC_USER_DATA) );			
	PStrPrintf( szSystemPaths[FILE_PATH_UNIT_RECORD],	MAX_PATH, OS_PATH_TEXT("%sSave\\Record\\"),	FilePath_GetSystemPath(FILE_PATH_PUBLIC_USER_DATA) );
	PStrPrintf( szSystemPaths[FILE_PATH_CRASH],			MAX_PATH, OS_PATH_TEXT("%sError\\"),		FilePath_GetSystemPath(FILE_PATH_PUBLIC_USER_DATA) );
	PStrPrintf( szSystemPaths[FILE_PATH_USER_SETTINGS],	MAX_PATH, OS_PATH_TEXT("%sSettings\\"),		FilePath_GetSystemPath(FILE_PATH_PUBLIC_USER_DATA) );
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PRESULT OverridePathRemoveCode( char * pszStripped, int nBufLen, const char * pszName )
{
	ASSERT_RETINVALIDARG( pszName );
	ASSERT_RETINVALIDARG( pszStripped );

	// strip .* off of the end of the name
	const char * pszCode = PStrRChr( pszName, OVERRIDE_PATH_CODE_MARKER_CHAR );
	if ( ! pszCode )
		return E_FAIL;

	int nNewLen = (int)( pszCode - pszName );
	ASSERT_RETFAIL( nNewLen < nBufLen );
	PStrCopy( pszStripped, nBufLen, pszName, nNewLen );
	pszStripped[ nNewLen ] = NULL;

	return S_OK;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD GetTextureOverridePathCode( const char * pszName )
{
	ASSERT_RETZERO( pszName );

	// strip .* off of the end of the name
	const char * pszCode = PStrRChr( pszName, OVERRIDE_PATH_CODE_MARKER_CHAR );
	if ( ! pszCode )
		return 0;
	pszCode++;  // after the OVERRIDE_PATH_CODE_MARKER_CHAR
	if ( PStrLen( pszCode ) != 4 )
	{
		const GLOBAL_DEFINITION* pGlobal = DefinitionGetGlobal();
		if ( pGlobal->dwFlags & GLOBAL_FLAG_DATA_WARNINGS )
		{
			ASSERTV_MSG("Invalid level path code: \"%s\". Must be four alpha-numeric or '_' characters, with no trailing spaces!", pszName );
		}
		return 0;
	}

	DWORD dwFourCC = *(DWORD*)pszCode;
	return dwFourCC;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
int GetOverridePathLine( DWORD dwFourCC )
{
	if ( ! dwFourCC )
		return INVALID_ID;
	// look up code in excel
	return ExcelGetLineByCode(EXCEL_CONTEXT(), DATATABLE_LEVEL_FILE_PATHS, dwFourCC);
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void TestOverridePathCode()
{
	DWORD dwFourCC;
	dwFourCC = GetTextureOverridePathCode( "floor." );
	dwFourCC = GetTextureOverridePathCode( "floor.ABC" );
	dwFourCC = GetTextureOverridePathCode( "floor.whazap?.-OCA" );
	dwFourCC = GetTextureOverridePathCode( "floor.CRAP" );
	dwFourCC = GetTextureOverridePathCode( "floor.CRP " );
	dwFourCC = GetTextureOverridePathCode( "floor.CRAP " );
	dwFourCC = GetTextureOverridePathCode( "floor.CRAP.AGN" );
	dwFourCC = GetTextureOverridePathCode( "floor.CRAP.AGN1" );
	dwFourCC = GetTextureOverridePathCode( "floor" );
	dwFourCC = GetTextureOverridePathCode( ".CRAP" );
	dwFourCC = GetTextureOverridePathCode( "floor.1G_N" );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sGetFolderForCurrentLanguage(
	int *pnLocaliedFolders,
	int nNumFolders)
{
	ASSERTX_RETINVALID( pnLocaliedFolders, "Expected localized folder code buffer" );
	ASSERTX_RETINVALID( nNumFolders > 0, "Invalid localized folder code buffer size" );
	
	// get the current language and app
	LANGUAGE eLanguage = LanguageGetCurrent();
	APP_GAME eAppGame = AppGameGet();
	
	// what language is used for the localized textures
	LANGUAGE eLanguageTextures = LanguageGetBackgroundTextureLanguage( eAppGame, eLanguage );
	// NOTE that it's OK for this to be INVALID_LINK when a language wants to use a background
	// texture folder that is "blank" or otherwise good for any language
	
	// check to see if any of the provided folder codes are allowed in this langauge
	for (int i = 0; i < nNumFolders; ++i)
	{
		int nFolder = pnLocaliedFolders[ i ];
		ASSERTX_CONTINUE( nFolder != INVALID_LINK, "Expected folder link" );
		const LEVEL_FILE_PATH *pFilePath = LevelFilePathGetData( nFolder );
		if (pFilePath->eLanguage == eLanguageTextures)
		{
			return nFolder;
		}		
	}	
	
	return INVALID_LINK;

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
OVERRIDE_PATH::OVERRIDE_PATH( void )
	:	ePakEnum( PAK_DEFAULT )
{ 
	szPath[ 0 ] = 0;
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void OverridePathByLine( 
	int nLine, 
	OVERRIDE_PATH *pOverridePath,
	int nLOD)
{
	ASSERT( pOverridePath );
	if ( nLine == INVALID_ID )
		return;

	// Don't override low LOD paths -- the low LOD textures should be in the low\ folder underneath the high-poly GR2 (with the low-poly one).
	if ( nLOD == LOD_LOW )
		return;

	// get file path
	const LEVEL_FILE_PATH * pLevelPath = LevelFilePathGetData( nLine );

	// does this folder code refer to other folder codes based on language?
	int nLocalizedFolders[ LEVEL_FILE_PATH_MAX_LOCALIZED_FOLDERS ];
	int nNumLocalizedFolders = LevelFilePathGetLocalizedFolders( pLevelPath, nLocalizedFolders, LEVEL_FILE_PATH_MAX_LOCALIZED_FOLDERS );
	
	// if this folder instructs us to check for localized assets, figure out which folder code
	// has our actual path
	if (nNumLocalizedFolders > 0)
	{
	
		// get the final folder code to use based on the currently selected language
		int nFolder = sGetFolderForCurrentLanguage( nLocalizedFolders, nNumLocalizedFolders );
		
		// override the path with the new folder code instead
		OverridePathByLine( nFolder, pOverridePath, nLOD );
		return;
	}
	else
	{
	
		if (!pLevelPath->pszPath  || !pOverridePath)
		{
			return;  // no override
		}

		char szTemp[ DEFAULT_FILE_WITH_PATH_SIZE ];
		PStrRemoveEntirePath( szTemp, DEFAULT_FILE_WITH_PATH_SIZE, pOverridePath->szPath );
		if ( nLOD == LOD_LOW )
		{
			PStrPrintf( 
				pOverridePath->szPath, 
				DEFAULT_FILE_WITH_PATH_SIZE, 
				"%s%slow\\%s", 
				FILE_PATH_DATA, 
				pLevelPath->pszPath, szTemp );
		}
		else
		{
			PStrPrintf( 
				pOverridePath->szPath, 
				DEFAULT_FILE_WITH_PATH_SIZE, 
				"%s%s%s", 
				FILE_PATH_DATA, 
				pLevelPath->pszPath, 
				szTemp);
		}
		
		// save pak file used for files in this folder
		pOverridePath->ePakEnum = pLevelPath->ePakfile;
		
	}
	
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
void OverridePathByCode( 
	DWORD dwFourCC, 
	OVERRIDE_PATH *pOverridePath,
	int nLOD /*= LOD_ANY*/)
{
	int nLine = GetOverridePathLine( dwFourCC );
	OverridePathByLine( nLine, pOverridePath, nLOD );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
int LevelFilePathGetLocalizedFolders(
	const LEVEL_FILE_PATH *pLevelFilePath,
	int *pnLocalizedFolders,
	int nMaxLocalizedFolders)
{
	ASSERTX_RETZERO( pLevelFilePath, "Expected level file path" );
	ASSERTX_RETZERO( pnLocalizedFolders, "Expected localized folder code buffer" );
	ASSERTX_RETZERO( nMaxLocalizedFolders > 0, "Invalid localied folder code buffer size" );
	
	int nCount = 0;
	for (int i = 0; i < LEVEL_FILE_PATH_MAX_LOCALIZED_FOLDERS; ++i)
	{
		int nFolder = pLevelFilePath->nFolderLocalized[ i ];
		if (nFolder== INVALID_LINK)
		{
			break;
		}
		ASSERTX_BREAK( nCount <= nMaxLocalizedFolders - 1, "Localized folder code buffer too small" );
		pnLocalizedFolders[ nCount++ ] = nFolder;
	}
	
	return nCount;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL FilePathGetInstallFolder(
	APP_GAME eAppGame,
	WCHAR *puszBuffer,
	DWORD dwBufferSize)
{
	ASSERTX_RETFALSE( puszBuffer, "Expected path" );
	ASSERTX_RETFALSE( dwBufferSize, "Invalid buffer size" );
	BOOL bResult = FALSE;
	
	// open registry key
	HKEY hKeyHellgateExists = 0;
	if (RegOpenKeyExW(
			HKEY_LOCAL_MACHINE, 
			REGISTRY_DIR, 
			0, 
			KEY_READ, 
			&hKeyHellgateExists) == ERROR_SUCCESS) 
	{

		// find the key
		if (eAppGame == APP_GAME_HELLGATE)
		{
			if (RegQueryValueExW(
					hKeyHellgateExists,  
					HELLGATECUKEY, 
					0, 
					NULL, 
					(BYTE*)puszBuffer, 
					&dwBufferSize) == ERROR_SUCCESS)
			{
				bResult = TRUE;
			}

		}
			
	}
	
	if (hKeyHellgateExists != 0) 
	{
		RegCloseKey(hKeyHellgateExists);
	}

	return bResult;
	
}
