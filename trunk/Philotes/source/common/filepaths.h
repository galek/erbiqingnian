#ifndef __FILEPATHS_H__
#define __FILEPATHS_H__

#include "ospathchar.h"
#ifndef __LOD__
#include "lod.h"
#endif

#include "pakfile.h"

//----------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Forward Declarations
//----------------------------------------------------------------------------
enum LANGUAGE;

#define LEVEL_FILE_PATH_MAX_LOCALIZED_FOLDERS (8)  // but must be large enough to hold all languages we want to localize art for

//----------------------------------------------------------------------------
struct LEVEL_FILE_PATH
{
	DWORD		dwCode;
	char		pszPath[DEFAULT_FILE_WITH_PATH_SIZE];
	
	// localized data
	int			nFolderLocalized[ LEVEL_FILE_PATH_MAX_LOCALIZED_FOLDERS ];  // this folder code is not a folder, but rather it refers to other folders that can be used based on the current language
	LANGUAGE	eLanguage;	// this folder contains only localized assets for this language
	PAK_ENUM	ePakfile;	// pakfile used for this folder
	
};

//----------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------

const LEVEL_FILE_PATH *LevelFilePathGetData(
	int nLine);
	
void FilePathInitForApp(
	enum APP_GAME eAppGame);

PRESULT OverridePathRemoveCode(
	char * pszStripped,
	int nBufLen,
	const char * pszName );

DWORD GetTextureOverridePathCode( 
	const char * pszName);

int GetOverridePathLine( 
	DWORD dwFourCC);

struct OVERRIDE_PATH
{
	char szPath[ DEFAULT_FILE_WITH_PATH_SIZE ];
	enum PAK_ENUM ePakEnum;	
	
	OVERRIDE_PATH::OVERRIDE_PATH( void );
	
};
	
void OverridePathByLine( 
	int nLine, 
	OVERRIDE_PATH *pOverridePath,
	int nLOD = LOD_ANY );
	
void OverridePathByCode( 
	DWORD dwFourCC, 
	OVERRIDE_PATH *pOverridePath,
	int nLOD = LOD_ANY );

int LevelFilePathGetLocalizedFolders(
	const LEVEL_FILE_PATH *pLevelFilePath,
	int *pnLocalizedFolders,
	int nMaxLocalizedFolders);

//----------------------------------------------------------------------------
// Externals
//----------------------------------------------------------------------------
extern char FILE_PATH_DATA[MAX_PATH];
extern char FILE_PATH_DATA_COMMON[MAX_PATH];
extern char FILE_PATH_PAK_FILES[MAX_PATH];

extern char FILE_PATH_EXCEL_COMMON[MAX_PATH];
extern char FILE_PATH_EXCEL[MAX_PATH];
extern char FILE_PATH_STRINGS[MAX_PATH];
extern char FILE_PATH_STRINGS_COMMON[MAX_PATH];

extern char FILE_PATH_UNITS[MAX_PATH];
extern char FILE_PATH_MONSTERS[MAX_PATH];
extern char FILE_PATH_PLAYERS[MAX_PATH];
extern char FILE_PATH_ITEMS[MAX_PATH];
extern char FILE_PATH_WEAPONS[MAX_PATH];
extern char FILE_PATH_MISSILES[MAX_PATH];
extern char FILE_PATH_OBJECTS[MAX_PATH];
extern char FILE_PATH_BACKGROUND[MAX_PATH];
extern char FILE_PATH_BACKGROUND_PROPS[MAX_PATH];
extern char FILE_PATH_BACKGROUND_DEBUG[MAX_PATH];
extern char FILE_PATH_SKILLS[MAX_PATH];
extern char FILE_PATH_STATES[MAX_PATH];
extern char FILE_PATH_ICONS[MAX_PATH];

extern char FILE_PATH_TOOLS[MAX_PATH];
extern char FILE_PATH_UI_XML[MAX_PATH];

extern char FILE_PATH_GLOBAL[MAX_PATH];

extern char FILE_PATH_PARTICLE[MAX_PATH];
extern char FILE_PATH_PARTICLE_ROPE_PATHS[MAX_PATH];

extern char FILE_PATH_EFFECT_COMMON[MAX_PATH];
extern char FILE_PATH_EFFECT[MAX_PATH];
extern char FILE_PATH_EFFECT_SOURCE[MAX_PATH];
extern char FILE_PATH_SOUND[MAX_PATH];
extern char FILE_PATH_SOUND_REVERB[MAX_PATH];
extern char FILE_PATH_SOUND_ADSR[MAX_PATH];
extern char FILE_PATH_SOUND_EFFECTS[MAX_PATH];
extern char FILE_PATH_SOUND_SYNTH[MAX_PATH];
extern char FILE_PATH_SOUND_HIGHQUALITY[MAX_PATH];
extern char FILE_PATH_SOUND_LOWQUALITY[MAX_PATH];

extern char FILE_PATH_MATERIAL[MAX_PATH];
extern char FILE_PATH_LIGHT[MAX_PATH];
extern char FILE_PATH_PALETTE[MAX_PATH];
extern char FILE_PATH_AI[MAX_PATH];
extern char FILE_PATH_SCRIPT[MAX_PATH];
extern char FILE_PATH_SCREENFX[MAX_PATH];
extern char FILE_PATH_DEMOLEVEL[MAX_PATH];

extern char FILE_PATH_HAVOKFX_SHADERS[MAX_PATH];

// CHB 2007.01.11
enum FILE_PATH_SYS_ENUM
{
	FILE_PATH_PUBLIC_USER_DATA,		// i.e., My Documents/My Games/<AppName>
	FILE_PATH_LOG,
	FILE_PATH_SAVE_BASE,
	FILE_PATH_SINGLEPLAYER_SAVE,
	FILE_PATH_MULTIPLAYER_SAVE,
	FILE_PATH_DEMO_SAVE,
	FILE_PATH_UNIT_RECORD,
	FILE_PATH_CRASH,
	FILE_PATH_USER_SETTINGS,

	FILE_PATH_SYS_MAX
};

#define HELLGATECUKEY						L"HellgateCUKey"
#define REGISTRY_DIR						L"SOFTWARE\\Flagship Studios\\Hellgate London"
#ifdef DemoPackage_Condition
#undef HELLGATECUKEY
#undef REGISTRY_DIR
#define HELLGATECUKEY						L"HellgateCUKeyDemo"
#define REGISTRY_DIR						L"SOFTWARE\\Flagship Studios\\Hellgate London Demo"
#endif 
#ifdef OnlinePackage_Condition
#undef HELLGATECUKEY
#undef REGISTRY_DIR
#define HELLGATECUKEY						L"HellgateCUKey"
#define REGISTRY_DIR						L"SOFTWARE\\Flagship Studios\\Hellgate London"
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline
FILE_PATH_SYS_ENUM GetSavePathEnum(void)
{
	if( AppIsDemo() )
		return FILE_PATH_DEMO_SAVE;
	if( AppIsMultiplayer() )
		return FILE_PATH_MULTIPLAYER_SAVE;
	return FILE_PATH_SINGLEPLAYER_SAVE;
}
#define FILE_PATH_SAVE GetSavePathEnum()

const OS_PATH_CHAR * FilePath_GetSystemPath(FILE_PATH_SYS_ENUM nWhich);

BOOL FilePathGetInstallFolder(
	APP_GAME eAppGame,
	WCHAR *puszBuffer,
	DWORD dwBufferSize);
	
#endif
