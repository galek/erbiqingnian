//----------------------------------------------------------------------------
// appcommon.h
//----------------------------------------------------------------------------
#ifndef _APPCOMMON_H_
#define _APPCOMMON_H_

#ifndef _PTIME_H_
#include "ptime.h"
#endif

#ifndef _SCRIPT_TYPES_H_
#include "script_types.h"
#endif

#include "gamewindow.h"
#include "ospathchar.h"


//----------------------------------------------------------------------------
// ENUMERATIONS
//----------------------------------------------------------------------------
enum APP_GAME
{
	APP_GAME_INVALID,
	
	APP_GAME_UNKNOWN,
	APP_GAME_HELLGATE,
	APP_GAME_TUGBOAT,
	APP_GAME_ALL,								// used by tables, g_AppGame should never be set to this
	
	APP_GAME_NUM_GAMES
};


//----------------------------------------------------------------------------
enum APP_FLAG
{
	AF_PROCESS_WHEN_INACTIVE_BIT,
	AF_NO_PLAYER_SAVING,
	AF_CENSOR_LOCKED,
	AF_CENSOR_PARTICLES,
	AF_CENSOR_NO_BONE_SHRINKING,
	AF_CENSOR_NO_HUMANS,
	AF_CENSOR_NO_GORE,
	AF_CENSOR_NO_PVP,
	AF_UNITMETRICS,
	AF_UNITMETRICS_HCE_ONLY,
	AF_UNITMETRICS_PVP_ONLY,
	AF_DEMO_MODE,
	AF_RESPEC,
	AF_PATCH_ONLY,
	AF_TEST_CENTER_SUBSCRIBER,
};


//----------------------------------------------------------------------------
enum APP_DEBUG_FLAG
{
	ADF_COLIN_BIT,
	ADF_JABBERWOCIZE,
	ADF_TEXT_LENGTH,			// strings always return the length helper string
	ADF_TEXT_LABELS,			// strings always return labels for text
	ADF_TEXT_POINT_SIZE,		// the ui will render point size numbers instead of text
	ADF_TEXT_DEVELOPER,			// text is always shown in developer language
	ADF_TEXT_FONT_NAME,			// text shows the name of the font instead
	ADF_UI_NO_CLIPPING,			// toggle ui clipping	
	ADF_FILL_PAK_BIT,
	ADF_FILL_SOUND_PAK_BIT,
	ADF_FILL_LOCALIZED_PAK_BIT,
	ADF_FILL_PAK_MIN_BIT,
	ADF_FILL_PAK_FORCE,
	ADF_FILL_PAK_ADVERT_DUMP_BIT,
	ADF_TEXT_STATS_DISP_DEBUGNAME,
	ADF_SP_USE_INSTANCE,
	ADF_FILL_PAK_CLIENT_BIT,
	ADF_DELAY_EXCEL_LOAD_POST_PROCESS,
	ADF_DONT_ADD_TO_PAKFILE,
	ADF_ALLOW_FILE_VERSION_UPDATE,
	ADF_START_SOUNDFIELD_BIT,
	ADF_LOAD_ALL_LODS,			// CHB 2007.07.11 - flag to force loading of all LODs (useful for previewing)
	ADF_DIFFICULTY_OVERRIDE,
	ADF_NORMAL,
	ADF_NIGHTMARE,
	ADF_HELL,
	ADF_NOMOVIES,
	ADF_TOWN_FORCED, // everywhere is considered town
	ADF_FIXED_SEEDS,
	ADF_AUTHENTICATE_LOCAL,		// use a local authentication server (for development)
	ADF_FORCE_DATA_VERSION_CHECK,
	ADF_FILL_PAK_USES_CONVERT_FILTERS, // when in "convert" mode
	ADF_FORCE_ASSERTS_ON,		// for use in "convert" mode when -nopopups is specified in order to allow some asserts through.
	ADF_DUMP_SKILL_ICONS,
	ADF_DUMP_SKILL_DESCS,		// for dumping all skill descriptions to a csv.
	ADF_SILENT_CHECKOUT,		// don't display the "checkout?" dialog, instead assuming "yes"
	ADF_FORCE_ALLOW_ASSET_UPDATE,	// force allowing assets to update
	ADF_FORCE_ASSET_CONVERSION_UPDATE,	// force asset conversion to skip FileNeedsUpdate checks
	ADF_IN_CONVERT_STEP,		// currently in asset convert code
	ADF_FORCE_SYNC_LOAD,		// force synchronous loading (immediate, blocking loads)
};

//----------------------------------------------------------------------------
enum APP_TYPE
{
	APP_TYPE_NONE,
	APP_TYPE_SINGLE_PLAYER,
	APP_TYPE_CLOSED_CLIENT,
	APP_TYPE_CLOSED_SERVER,
	APP_TYPE_OPEN_CLIENT,
	APP_TYPE_OPEN_SERVER,
};

//----------------------------------------------------------------------------
// STRUCTURES
//----------------------------------------------------------------------------
struct APP_COMMON
{
	GAME_WINDOW				m_tMainWindow;
	HINSTANCE				m_hInstance;
	struct COMMON_TIMER		*pTime;
	APP_TYPE				m_eAppType;			// moved from APP

	BOOL					m_bActive;			// moved from APP
	BOOL					m_bMinimized;		// moved from APP
	BOOL					m_bMenuLoop;		// indicates the app system menu loop is active
	BOOL					m_bSizeMove;		// indicates the app window is in the size/move state

	volatile LONG			m_lGuidIterator;	// used to generate guids

	BOOL					m_bUpdatePakFile;	// update pak file (formerly "generate" pak file, and different from "fill" pak file)
	BOOL					m_bFillPakFile;		// run a full fillpak, and then quit
	int						m_nDistributedPakFileGenerationIndex; // An index used to mod the generation of pak files when done on the server
	int						m_nDistributedPakFileGenerationMax;	  // The total number of servers used to pak file generation
	BOOL					m_bUsePakFile;		// use pak file
	BOOL					m_bDirectLoad;		// read files from directories
	BOOL					m_bRegenExcel;		// force reparse of all excel tables
	BOOL					m_bSilentAssert;
	BOOL					m_bOneParty;		// everyone goes to same game when they exit minitown
	BOOL					m_bOneInstance;		// all levels are in the same game instance (helpful for net debugging)
	BOOL					m_bSingleplayerOnly;		// Opening menu skips the multi/single player decision and behaves as if single had been selected
	BOOL					m_bMultiplayerOnly;			// Opening menu skips the multi/single player decision and behaves as if multi had been selected
	int						m_nMonsterScalingOverride;
	BOOL					m_bDisableSound;	// Disables sound, but still performs allocations
    BOOL                    m_bGenMinPak; // quit after some time in the loading screen.
    DWORD                   m_GenMinPakStartTick;
	BOOL                    m_bRelaunchMCE;		// Relaunch Media Center after exiting game.
	BOOL					m_bForceWindowed;	// Force starting up in windowed mode
	DWORD					m_dwExcelManifestPackage; // for Excel loading
	WORD					m_wExcelVersionMajor; // for Excel loading
	WORD					m_wExcelVersionMinor; // for Excel loading
	BOOL					m_bDisableKeyHooks;	// Disables low-level windows keyboard hooks (eg, trapping Windows key)
	BOOL					m_bForceLoadAllLanguages;  // force load all the possible languages
	BOOL					m_bNoMinSpecCheck;	// Disables all minspec checks
	BOOL					m_bDisablePakTypeChange;

	TCHAR					m_szDemoLevel[DEFAULT_INDEX_SIZE];		// Name of the demo level def for Demo Mode (play a flythrough and show perf stats at the end)

	// the following are used for saving to the database
	BOOL					m_bDatabase;
	TCHAR					m_szDatabaseAddress[MAX_PATH];
	TCHAR					m_szDatabaseServer[MAX_PATH];
	TCHAR					m_szDatabaseUser[MAX_PATH];
	TCHAR					m_szDatabasePassword[MAX_PATH];
	TCHAR					m_szDatabaseDb[MAX_PATH];
	struct DATABASE *		m_Database;

	struct FILE_SYSTEM *	m_pFileSystem;

	OS_PATH_CHAR			m_szRootDirectory[MAX_PATH];
	int						m_nRootDirectoryStrLen;

	DWORD					m_dwMainThreadAsyncThreadId;	// async thread id for main thread
	LPCSTR					m_sGenericName;

	DWORD					m_dwGlobalFlagsToSet;
	DWORD					m_dwGlobalFlagsToClear;
	DWORD					m_dwGameFlagsToSet;
	DWORD					m_dwGameFlagsToClear;
    struct INET_CONTEXT*    m_pInetContext;

	DWORD					m_dwAppFlags;
	UINT64					m_ullAppDebugFlags;

	CHAR					m_szCmdLine[MAX_PATH];
};


//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------
extern APP_COMMON			gtAppCommon;
extern APP_GAME				g_AppGame;


//----------------------------------------------------------------------------
// MACROS
//----------------------------------------------------------------------------
//	default to hellgate for server builds
#if !defined(HELLGATE) && !defined(TUGBOAT)
#define HELLGATE 1
#endif

// These macros are meant to bracket code that should be compiled into development builds for
// both targets but not release builds in the hellgate target, and should do a runtime check
// when the code is included in cross-target (development) builds.
#if ( ISVERSION(DEVELOPMENT) || defined(_BOT) ) && !TUGBOAT_FORCE
	inline BOOL AppIsHellgate(
		void)
	{
		return g_AppGame == APP_GAME_HELLGATE;
	}
#else
	#ifndef TUGBOAT
		#define AppIsHellgate()				1
	#else
		#define AppIsHellgate()				0
	#endif /* ! TUGBOAT */
#endif /* ISVERSION(DEVELOPMENT) */


#if ( ISVERSION(DEVELOPMENT) || defined(_BOT) ) && !TUGBOAT_FORCE
	inline BOOL AppIsTugboat(
		void)
	{
		return g_AppGame == APP_GAME_TUGBOAT;
	}
#else
	#ifdef TUGBOAT
		#define AppIsTugboat()				1
	#else
		#define AppIsTugboat()				0
	#endif /* TUGBOAT */
#endif /* ISVERSION(DEVELOPMENT) */


#define HELLGATE_ONLY	( ! TUGBOAT || ISVERSION(DEVELOPMENT) ) && !TUGBOAT_FORCE
#define TUGBOAT_ONLY	(   TUGBOAT || ISVERSION(DEVELOPMENT) )


#define HAVOK_ENABLED
#define HAVOKFX_ENABLED

// disable Havok and HavokFX in non-hellgate, non-development targets
#if !defined( HELLGATE ) && ( !ISVERSION( DEVELOPMENT ) || TUGBOAT_FORCE )
	#ifdef HAVOK_ENABLED
		#undef HAVOK_ENABLED
	#endif
	#ifdef HAVOKFX_ENABLED
		#undef HAVOKFX_ENABLED
	#endif
#endif // !HELLGATE && !DEVELOPMENT

#define XFIRE_VOICECHAT_ENABLED

//----------------------------------------------------------------------------
// EXPORTED FUNCTIONS
//----------------------------------------------------------------------------
void CommonInit(
	HINSTANCE hInstance, 
	LPSTR pCmdLine,
	WCHAR * strRootDirectoryOverride);

void CommonClose(
	void);

void AppCommonSetDirToRoot(
	void);

void AppGameSet( 
	APP_GAME eAppGame);

BOOL AppIsBeta(void);
BOOL AppIsDemo(void);

//----------------------------------------------------------------------------
// CRC
//----------------------------------------------------------------------------
DWORD CRC(
	DWORD crc,
	const BYTE * buffer,
	int len);


//----------------------------------------------------------------------------
// INLINES
//----------------------------------------------------------------------------
inline APP_GAME AppGameGet(
	void)
{
	return g_AppGame;
}	
	
inline HWND AppCommonGetHWnd(
	void)
{
	return gtAppCommon.m_tMainWindow.m_hWnd;
}

inline void AppCommonSetHWnd( HWND hWnd )
{
	gtAppCommon.m_tMainWindow.m_hWnd = hWnd;
}

inline HINSTANCE AppCommonGetHInstance(
	void)
{
	return gtAppCommon.m_hInstance;
}

//----------------------------------------------------------------------------
inline DWORD AppCommonGetMainThreadAsyncId(
	void)
{
	return gtAppCommon.m_dwMainThreadAsyncThreadId;
}

//----------------------------------------------------------------------------
inline void AppCommonSetMainThreadAsyncId(
	DWORD dwThreadId)
{
	gtAppCommon.m_dwMainThreadAsyncThreadId = dwThreadId;
}

//----------------------------------------------------------------------------
inline struct COMMON_TIMER *& AppCommonGetTimer(
	void)
{
	return gtAppCommon.pTime;
}

//----------------------------------------------------------------------------
inline int AppCommonGetWindowWidth(
	void)
{
	return gtAppCommon.m_tMainWindow.m_nWindowWidth;
}

//----------------------------------------------------------------------------
inline int AppCommonGetWindowHeight(
	void)
{
	return gtAppCommon.m_tMainWindow.m_nWindowHeight;
}

//----------------------------------------------------------------------------
inline int& AppCommonGetWindowWidthRef(
	void)
{
	return gtAppCommon.m_tMainWindow.m_nWindowWidth;
}

//----------------------------------------------------------------------------
inline int& AppCommonGetWindowHeightRef(
	void)
{
	return gtAppCommon.m_tMainWindow.m_nWindowHeight;
}

//----------------------------------------------------------------------------
inline BOOL AppCommonGetSilentAssert(
	void)
{
	return gtAppCommon.m_bSilentAssert;
}

//----------------------------------------------------------------------------
BOOL AppCommonAllowAssetUpdate(
	void);

//----------------------------------------------------------------------------
inline BOOL AppCommonGetUpdatePakFile(
	void)
{
	return gtAppCommon.m_bUpdatePakFile;
}

//----------------------------------------------------------------------------
inline BOOL AppCommonGetFillPakFile(
	void)
{
	return gtAppCommon.m_bFillPakFile;
}

//----------------------------------------------------------------------------
inline BOOL AppCommonUsePakFile(
	void)
{
	return gtAppCommon.m_bUsePakFile;
}

//----------------------------------------------------------------------------
inline BOOL AppCommonGetRegenExcel(
	void)
{
	return gtAppCommon.m_bRegenExcel;
}

//----------------------------------------------------------------------------
inline BOOL AppCommonGetDirectLoad(
	void)
{
	return gtAppCommon.m_bDirectLoad;
}

//----------------------------------------------------------------------------
inline BOOL AppCommonGetOneParty(
	void)
{
	return gtAppCommon.m_bOneParty;
}

//----------------------------------------------------------------------------
inline BOOL AppCommonGetOneInstance(
	void)
{
	return gtAppCommon.m_bOneInstance;
}

//----------------------------------------------------------------------------
BOOL AppCommonSingleplayerOnly(
	void);

//----------------------------------------------------------------------------
inline BOOL AppCommonMultiplayerOnly(
	void)
{
	return gtAppCommon.m_bMultiplayerOnly;
}

//----------------------------------------------------------------------------
inline BOOL AppCommonGetDemoMode(
	void)
{
	return gtAppCommon.m_szDemoLevel[0] != NULL;
}

//----------------------------------------------------------------------------
inline const TCHAR* AppCommonGetDemoLevelName(
	void)
{
	return gtAppCommon.m_szDemoLevel;
}

//----------------------------------------------------------------------------
inline BOOL AppCommonGetForceWindowed(
	void)
{
	return gtAppCommon.m_bForceWindowed;
}

//----------------------------------------------------------------------------
inline BOOL AppCommonGetNoMinSpecCheck(
	void)
{
	return gtAppCommon.m_bNoMinSpecCheck;
}

//----------------------------------------------------------------------------
inline BOOL AppCommonGetDisableKeyHooks(
	void)
{
	return gtAppCommon.m_bDisableKeyHooks;
}

//----------------------------------------------------------------------------
inline int AppCommonGetMonsterScalingOverride(
	void)
{
	return gtAppCommon.m_nMonsterScalingOverride;
}

//----------------------------------------------------------------------------
inline void AppCommonSetMonsterScalingOverride(
	unsigned int monsterscaling)
{
	gtAppCommon.m_nMonsterScalingOverride = monsterscaling;
}

//----------------------------------------------------------------------------
inline const OS_PATH_CHAR * AppCommonGetRootDirectory(
	int * len)
{
	if (len)
	{
		*len = gtAppCommon.m_nRootDirectoryStrLen;
	}
	return gtAppCommon.m_szRootDirectory;
}

//----------------------------------------------------------------------------
inline BOOL AppCommonGetForceLoadAllLanguages(void)
{
	return gtAppCommon.m_bForceLoadAllLanguages;
}


//----------------------------------------------------------------------------
inline BOOL AppUseDatabase(
	void)
{
	return gtAppCommon.m_bDatabase;
}


//----------------------------------------------------------------------------
inline struct DATABASE * AppGetDatabase(
	void)
{
	return gtAppCommon.m_Database;
}


//----------------------------------------------------------------------------
struct FILE_SYSTEM;
inline FILE_SYSTEM *& AppCommonGetFileSystemRef(
	void)
{
	return gtAppCommon.m_pFileSystem;
}
//----------------------------------------------------------------------------
inline BOOL AppCommonGetMinPak(void)
{
    return gtAppCommon.m_bGenMinPak;
}
inline DWORD AppCommonGetMinPakStartTick(
    void)
{
    return gtAppCommon.m_GenMinPakStartTick;
}
//----------------------------------------------------------------------------
inline BOOL AppCommonGetRelaunchMCE( void )
{
	return gtAppCommon.m_bRelaunchMCE;
}
//----------------------------------------------------------------------------
inline INET_CONTEXT *AppCommonGetHttp(void)
{
    return gtAppCommon.m_pInetContext;
}
//----------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
// replacing the #ifdef HAMMER for libraries
inline void c_SetToolMode( BOOL bEnabled )
{
	extern BOOL gbToolMode;
	gbToolMode = bEnabled;
}


//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
// replacing the #ifdef HAMMER for libraries
inline BOOL c_GetToolMode()
{
	extern BOOL gbToolMode;
	return gbToolMode;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline DWORD * AppGetFlags(
	void)
{
	return &gtAppCommon.m_dwAppFlags;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void AppSetFlag(
	APP_FLAG eFlag,
	BOOL bValue)
{
	DWORD * flags = AppGetFlags();
	ASSERT_RETURN(flags);
	SETBIT(*flags, eFlag, bValue);
}	


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BOOL AppTestFlag(
	APP_FLAG eFlag)
{
	DWORD * flags = AppGetFlags();
	ASSERT_RETFALSE(flags);
	return TESTBIT(*flags, eFlag);
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AppSetCensorFlag(
	APP_FLAG eFlag,
	BOOL bValue,
	BOOL bForce );

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BOOL AppToggleFlag(
	APP_FLAG eFlag)
{
	AppSetFlag(eFlag, !AppTestFlag(eFlag));
	return AppTestFlag(eFlag);
}	



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline UINT64 * AppGetDebugFlags(
	void)
{
	return &gtAppCommon.m_ullAppDebugFlags;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void AppSetDebugFlag(
	APP_DEBUG_FLAG eFlag,
	BOOL bValue)
{
#if ISVERSION(DEVELOPMENT)
	UINT64 * flags = AppGetDebugFlags();
	ASSERT_RETURN(flags);
	ASSERT_RETURN(eFlag < sizeof(flags[0]) * BITS_PER_BYTE);
	SETBIT((BYTE*)flags, eFlag, bValue);
#else
	REF(eFlag);
	REF(bValue);
	return;
#endif	
}	


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BOOL AppTestDebugFlag(
	APP_DEBUG_FLAG eFlag)
{
#if ISVERSION(DEVELOPMENT)
	UINT64 * flags = AppGetDebugFlags();
	ASSERT_RETFALSE( flags );
	ASSERT_RETFALSE(eFlag < sizeof(flags[0]) * BITS_PER_BYTE);
	return TESTBIT((BYTE*)flags, eFlag);
#else
	REF(eFlag);
	return FALSE;
#endif	
}	


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BOOL AppToggleDebugFlag(
	APP_DEBUG_FLAG eFlag)
{
#if ISVERSION(DEVELOPMENT)
	UINT64 * flags = AppGetDebugFlags();
	ASSERT_RETFALSE( flags );
	ASSERT_RETFALSE(eFlag < sizeof(flags[0]) * BITS_PER_BYTE);
	TOGGLEBIT((BYTE*)flags, eFlag);
	return TESTBIT((BYTE*)flags, eFlag);
#else
	REF(eFlag);
	return 0;
#endif	
}	


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline void AppCommonRunMessageLoop()
{
	GameWindowMessageLoop( gtAppCommon.m_tMainWindow );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// CHB 2007.07.11
inline
bool AppCommonGetForceLoadAllLODs(void)
{
	return AppCommonGetFillPakFile() || c_GetToolMode() || AppTestDebugFlag(ADF_LOAD_ALL_LODS);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BOOL AppCommonIsAnyFillpak(
	void)
{
#if ISVERSION(DEVELOPMENT)
	return !!( AppTestDebugFlag( ADF_FILL_PAK_BIT ) || 
			   AppTestDebugFlag( ADF_FILL_PAK_MIN_BIT ) || 
			   AppTestDebugFlag( ADF_FILL_PAK_CLIENT_BIT ) ||
			   AppTestDebugFlag( ADF_FILL_LOCALIZED_PAK_BIT ));
#else
	return FALSE;
#endif
}

inline BOOL AppCommonIsFillpakInConvertMode(
	void )
{
#if ISVERSION( DEVELOPMENT )
	return !!( AppCommonIsAnyFillpak() && AppTestDebugFlag( ADF_FILL_PAK_USES_CONVERT_FILTERS ) );
#else
	return FALSE;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BOOL AppCommonGetDumpSkillIcons(void)
{
#if ISVERSION(DEVELOPMENT)
	return !! AppTestDebugFlag( ADF_DUMP_SKILL_ICONS );
#else
	return FALSE;
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

inline void AppCommonSetMaxExcelVersion()
{
	gtAppCommon.m_wExcelVersionMajor = 0xffff;
	gtAppCommon.m_wExcelVersionMinor = 0xffff;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void AppCommonSetExcelPackageAll();

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// CHB 2007.08.01
typedef const WCHAR * (APP_GET_GLOBAL_STRING_CALLBACK)(unsigned);
const WCHAR * AppCommonGetGlobalString(unsigned nString, const WCHAR * pDefault);
void AppCommonSetGlobalStringCallback(APP_GET_GLOBAL_STRING_CALLBACK * pCallback);

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
APP_TYPE AppGetType(void);
BOOL AppIsSinglePlayer(void);
BOOL AppIsMultiplayer(void);

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AppSetActive( BOOL bActive );
BOOL AppIsActive(void);
void AppSetMinimized( BOOL bMinimized );
BOOL AppIsMinimized(void);
void AppSetMenuLoop( BOOL bMenuLoop );
BOOL AppIsMenuLoop(void);
void AppSetMoveWindow( BOOL bMoveWindow );
BOOL AppIsMoveWindow(void);
BOOL AppAppliesMinDataPatchesInLauncher(void);

#endif
