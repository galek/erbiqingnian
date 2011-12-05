//----------------------------------------------------------------------------
// Prime v2.0
//
// prime.cpp
// 
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// INCLUDES
//----------------------------------------------------------------------------
#include "stdafx.h"
#include "appcommon.h"
#include "prime.h"
#include "primePriv.h"
#include "cmdline.h"
#include "characterclass.h"
#include "game.h"
#include "gamelist.h"
#include "c_input.h"
#include "c_sound_util.h"
#include "s_gdi.h"
#include "clients.h"
#include "c_network.h"
#include "s_network.h"
#include "c_connmgr.h"
#include "c_message.h"
#include "console_priv.h"
#include "c_camera.h"
#include "camera_priv.h"
#include "units.h"
#include "player.h"
#include "weapons.h"
#include "c_appearance.h"
#include "c_animation.h"
#include "excel.h"
#include "c_ui.h"
#include "script.h"
#include "movement.h"
#include "c_particles.h"
#include "uix_money.h"
#include "s_message.h"
#include "uix.h"
#include "uix_priv.h"	// for startgame params
#include "uix_components.h"
#include "uix_menus.h"
#include "uix_chat.h"
#include "inventory.h"
#include "unitmodes.h"
#include "CrashHandler.h"
#include "language.h"
#include "performance.h"
#include "uix_scheduler.h"
#include "pakfile.h"
#include "fillpak.h"
#include "perfhier.h"
#include "appcommontimer.h"
#include "c_attachment.h"
#include "definition_priv.h"
#include "gameconfig.h"
#include "debugbars_priv.h"
#include "debug.h"
#include "console.h"
#include "e_common.h"
#include "e_caps.h"
#include "e_main.h"
#include "e_optionstate.h"
#include "e_settings.h"
#include "e_profile.h"
#include "e_screenshot.h"
#include "e_ui.h"
#include "e_anim_model.h"
#include "e_renderlist.h"
#include "e_budget.h"
#include "dungeon.h"
#include "minitown.h"
#include "unitdebug.h"
#include "wardrobe_priv.h"
#include "gameunits.h"
#include "colorset.h"
#include "debugwindow.h"
#include "c_townportal.h"
#include "dbhellgate.h"
#include "gameexplorer.h"
#include "lightmaps.h"
#include "trade.h"
#include "jobs.h"
#include "rli_net.h"
#include "c_sound_init.h"
#include "globalindex.h"
#include "states.h"
#include "svrstd.h"
#include "GameServer.h"
#include "s_chatNetwork.h"
#include "c_chatNetwork.h"
#include "c_adclient.h"
#include "chat.h"
#include "quests.h"
#include "sku.h"
#include "region.h"
#include "wordfilter.h"
#include "serverlist.h"
#include "skills.h"
#include "skill_strings.h"


#if !ISVERSION(SERVER_VERSION)
#include "fillpakservermsg.h"
#include "patchclientmsg.h"
#include "patchclient.h"
#include "EmbeddedServerRunner.h"
#include "id.h"
#include "cryptoapi.h"
#include "nethttp.h"
#include "c_authticket.h"
#include "c_characterselect.h"
#include "c_connection.h"
#include "mailslot.h"
#include "compression.h"
#include "patchclient.h"
#include "c_imm.h"
#include "rpcsal.h"
#include "gameux.h"
#include "settings.h"	// CHB 2007.01.15
#include "e_effect_priv.h"
#include "gfxoptions.h"
#include "c_voicechat.h"
#include "c_movieplayer.h"
#include "ServerSuite/GameLoadBalance/GameLoadBalanceDef.h"
#include "ServerSuite/BillingProxy/c_BillingClient.h"
#include "accountbadges.h"
#include "c_music.h"
#include "filepaths.h"
#include "c_eula.h"
#include "e_optionstate.h"
#include "c_credits.h"
#include "c_chatBuddy.h"
#include "c_chatIgnore.h"
#include "stringreplacement.h"
#include "demolevel.h"
#include "automap.h"
#include "c_playerEmail.h"

#ifdef TUGBOAT
	#include "common/resource_tugboat.h"
#else
	#include "common/resource.h"
#endif

#endif //!ISVERSION(SERVER_VERSION)

#if ISVERSION(SERVER_VERSION)
#include "prime.cpp.tmh"
#include "eventlog.h"
#include "Ping0ServerMessages.h"
#endif

#include "e_pfprof.h"
#include "e_compatdb.h"		// CHB 2006.12.13
#include "uix_options.h"	// CHB 2007.03.29
#include "sysinfo.h"		// CHB 2007.04.05 - for initialization and deinitialization
#include <string>			// CHB 2007.07.31

#ifdef HAVOK_ENABLED
	#include "havok.h"
	#include "c_ragdoll.h"
#endif

// For IMM stuffs.  We should wrap this in a function in c_imm.h
#include <Imm.h>

#ifdef _PROFILE
#define VSPERF_NO_DEFAULT_LIB
#include "vsperf.h"
#endif

//----------------------------------------------------------------------------
// DEFINES
//----------------------------------------------------------------------------

#define HELLGATE_APP_NAME			_T("Hellgate:London")
#define HELLGATE_GENERIC_NAME		_T("Hellgate")
#define HELLGATE_MUTEX_NAME			"Hellgate"

#ifdef _WIN64
#define TUGBOAT_APP_NAME			_T("Mythos (x64)")
#else
#define TUGBOAT_APP_NAME			_T("Mythos")
#endif
#define TUGBOAT_GENERIC_NAME		_T("Mythos")
#define TUGBOAT_MUTEX_NAME			"Mythos"

#define DATABASE_PARTITION_COUNT 20


//----------------------------------------------------------------------------
// GLOBALS
//----------------------------------------------------------------------------
const TCHAR * sgszGenericAppName = NULL;
const TCHAR * sgszAppName = NULL;
static const char * sgszAppMutexName = NULL;
BOOL bPrimeInitialized = FALSE;

#if !ISVERSION(SERVER_VERSION)
APP gApp;
#endif

GAME_SETTINGS gGameSettings;

#if ISVERSION(DEVELOPMENT) && defined(PERFORMANCE_ENABLE_TIMER)
CTimer gLoadTimer("Load Timer");
#endif

//#define SHOW_WM_DEBUG_TRACES
#ifdef SHOW_WM_DEBUG_TRACES
#	define WM_DEBUG_TRACE(str) DEBUG_TRACE(str)
#else
#	define WM_DEBUG_TRACE(str) 
#endif // SHOW_WM_DEBUG_TRACES


//----------------------------------------------------------------------------
// CONNECTING STATE GLOBALS
//----------------------------------------------------------------------------
WARDROBE_BODY		gWardrobeBody;
APPEARANCE_SHAPE	gAppearanceShape;


//----------------------------------------------------------------------------
//  Forwards
//----------------------------------------------------------------------------
static void AppMainMenuDo(
	unsigned int sim_frames);
void FileLoadUpdateLoadingScreenCallback();
void UISetCurrentMenu(UI_COMPONENT * pMenu);


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PrimeInitAppNameAndMutexGlobals(
	LPSTR cmdline)
{
	static CCriticalSection csLock(TRUE);
	CSAutoLock lock(&csLock);

#ifdef TUGBOAT
	AppGameSet(APP_GAME_TUGBOAT);
#else
	if (cmdline && StrStrI(cmdline, _T("-tugboat")) != NULL)		// can't use the global, too early in app init
	{
		AppGameSet(APP_GAME_TUGBOAT);
	}
	else
	{
		AppGameSet(APP_GAME_HELLGATE);
	}
#endif

	if (AppIsHellgate())
	{
		static const TCHAR * g_HellgateGenericName = HELLGATE_GENERIC_NAME;
		//static const TCHAR * g_HellgateAppName = HELLGATE_APP_NAME;
		static const TCHAR * g_HellgateMutexName = HELLGATE_MUTEX_NAME;

		// Assemble app name from platform and engine.
		static TCHAR g_HellgateAppName[ 64 ];
#if ISVERSION(SERVER_VERSION)
		PStrPrintf( g_HellgateAppName, 64, _T("%s (%s)"), HELLGATE_APP_NAME, APP_PLATFORM_NAME );
#else
		PStrPrintf( g_HellgateAppName, 64, _T("%s (%s %s)"), HELLGATE_APP_NAME, APP_PLATFORM_NAME, e_GetTargetName() );
#endif

		sgszGenericAppName = g_HellgateGenericName;
		sgszAppName = g_HellgateAppName;
		sgszAppMutexName = g_HellgateMutexName;
	}

	if (AppIsTugboat())
	{
		static const TCHAR * g_TugboatGenericName = TUGBOAT_GENERIC_NAME;
		static const TCHAR * g_TugboatAppName = TUGBOAT_APP_NAME;
		static const TCHAR * g_TugboatMutexName = TUGBOAT_MUTEX_NAME;

		sgszGenericAppName = g_TugboatGenericName;
		sgszAppName = g_TugboatAppName;
		sgszAppMutexName = g_TugboatMutexName;
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const TCHAR * AppGetName(
	void)
{
	return sgszAppName;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void sAppSignalExit(
	void *pUserData, 
	DWORD dwCallbackData)
{
	CLT_VERSION_ONLY( PostQuitMessage(0) );

#if ISVERSION(SERVER_VERSION)
	GameServerContext * serverContext = (GameServerContext*)CurrentSvrGetContext();
	ASSERT_RETURN( serverContext );
	serverContext->m_shouldExit = TRUE;
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
APP_STATE AppGetState(
	void)
{
	CLT_VERSION_ONLY( return gApp.m_eAppState );
	SVR_VERSION_ONLY( return APP_STATE_IN_GAME );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
APP_STATE AppGetStateBeforeMovie( 
	void)

{
	CLT_VERSION_ONLY( return gApp.m_eAppStateBeforeMovie );
	SVR_VERSION_ONLY( return APP_STATE_IN_GAME );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
#include "windowsmessages.h"
void AppReloadUI(
	void)
{
	CSchedulerFree();
	UIFree();
	if ( AppGetCltGame() )
	{
		UnitClearIcons( AppGetCltGame() );
	}
	e_TexturesCleanup( TEXTURE_GROUP_UI );
	CSchedulerInit();
	UIInit(TRUE, FALSE);

	ConsoleInitInterface();				// Reset the console (it needs to find the component ID of its textbox again)

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sAppErrorStateRestartCallback(
	void *pUserData, 
	DWORD dwCallbackData)
{
#if !ISVERSION(SERVER_VERSION)
	// Called after user accepts the error dialog

	APP_STATE eNextAppState = (APP_STATE)dwCallbackData;
	ASSERT_RETURN(eNextAppState >= APP_STATE_INIT && eNextAppState <= APP_STATE_FILLPAK);
	AppSwitchState(eNextAppState);

#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sAppCheckCommandLineParam(
	LPCSTR szCmdLine,
	LPCSTR szFlag)
{
	ASSERT_RETFALSE(szCmdLine != NULL);
	ASSERT_RETFALSE(szFlag != NULL);

	LPCSTR szSearch = PStrStrI(szCmdLine, szFlag);
	if (szSearch != NULL) {
		UINT32 dwStrLen = PStrLen(szFlag);
		if (szSearch[dwStrLen] == '\0' || szSearch[dwStrLen] == ' ') {
			return TRUE;
		}
	}
	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
// This function does not work correctly, but it'll do for the intended purpose
static void sAppErrorStateCallbackGotoDownloadUpdates(
	void *pUserData,
	DWORD dwCallbackData)
{
	if (AppAppliesMinDataPatchesInLauncher()) {
		AppSwitchState(APP_STATE_EXIT);

		WCHAR szLauncherPath[MAX_PATH];
		PStrPrintf(szLauncherPath, MAX_PATH, L"%sLauncher.exe", AppCommonGetRootDirectory(NULL));

		LPCSTR szFlagAutoUpdateMP = "-autoupdateMP";
		LPCSTR szFlagRedirect = "-redirect";

		WCHAR szCmdLine[MAX_PATH];
		PStrPrintf(szCmdLine, MAX_PATH, L"%S %S %S",
			gtAppCommon.m_szCmdLine,
			sAppCheckCommandLineParam(gtAppCommon.m_szCmdLine, szFlagAutoUpdateMP) ? "" : szFlagAutoUpdateMP,
			sAppCheckCommandLineParam(gtAppCommon.m_szCmdLine, szFlagRedirect) ? "" : szFlagRedirect);

		// Restart the game
		SHELLEXECUTEINFOW exInfo;
		memclear(&exInfo, sizeof(exInfo));
		exInfo.cbSize = sizeof(exInfo);
		exInfo.lpVerb = L"Open";
		exInfo.lpFile = szLauncherPath;
		exInfo.lpParameters = szCmdLine;
		exInfo.nShow = SW_SHOW;
		ShellExecuteExW(&exInfo);
	} else {
		// go back to restart state
		AppSwitchState( APP_STATE_RESTART );

		// launch web browser and go to the update page
		const int MAX_URL = 2048;
		WCHAR uszURL[ MAX_URL ] = { 0 };
		if (RegionCurrentGetURL( RU_DOWNLOAD_UPDATES, uszURL, MAX_URL ))
		{
			if (uszURL[ 0 ])
			{
				ShowWindow( AppCommonGetHWnd(), SW_MINIMIZE );
				ShellExecuteW( NULL, NULL, uszURL, NULL, NULL, SW_SHOW );		
			}
		}
	}		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AppSetErrorStateLocalized(
	const WCHAR *puszTitle,
	const WCHAR *puszError,
	APP_STATE eNextAppState /*= APP_STATE_RESTART*/,
	BOOL bExitGame /*= FALSE*/,
	DIALOG_CALLBACK *pCallbackOverride /*= NULL*/,
	BOOL bShowCancel /*= FALSE*/,
	DIALOG_CALLBACK *pCanelCallback /*= NULL*/)
{

	// setup default callback
	DIALOG_CALLBACK tDefaultCallback;
	if (pCallbackOverride)
	{
		tDefaultCallback = *pCallbackOverride;
	}
	DIALOG_CALLBACK *pCallback = &tDefaultCallback;
			
	// Be sure this is before the generic dialog is called because it can disable the cursor
	AppSwitchState(APP_STATE_MENUERROR);

	if (bExitGame)
	{
	
		// setup text
		const int MAX_MESSAGE = 1024;
		WCHAR uszErrorText[ MAX_MESSAGE ];
		if (bShowCancel)
		{
			PStrCopy( 
				uszErrorText, 
				puszError,
				MAX_MESSAGE);
		}
		else
		{
			PStrPrintf( 
				uszErrorText, 
				MAX_MESSAGE, 
				L"%s\n\n%s", 
				puszError,
				GlobalStringGet( GS_PREFIX_MESSAGE_CLICK_TO_EXIT ));
		}

		// if no callback override from caller of function do our own
		if (pCallbackOverride == NULL)
		{
			pCallback->pfnCallback = sAppSignalExit;
		}
		
		UIShowGenericDialog(
			puszTitle,
			uszErrorText,
			bShowCancel,
			pCallback,
			pCanelCallback);
	}
	else
	{
	
		// if no callback override from caller of function do our own
		if (pCallbackOverride == NULL)
		{
			pCallback->pfnCallback = sAppErrorStateRestartCallback;
			pCallback->dwCallbackData = (DWORD)eNextAppState;
		}
	
		UIShowGenericDialog(
			puszTitle,
			puszError,
			bShowCancel, 
			pCallback,
			pCanelCallback);			
	}

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AppSetErrorState(
	GLOBAL_STRING eGSTitle,
	GLOBAL_STRING eGSError,
	APP_STATE eNextAppState /*= APP_STATE_RESTART*/,
	BOOL bExitGame /*= FALSE*/)
{
	AppSetErrorStateLocalized(
		GlobalStringGet( eGSTitle ),
		GlobalStringGet( eGSError ),
		eNextAppState,
		bExitGame);
}
#endif //!SERVER_VERSION



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AppPause(
	BOOL bPause)
{
	if ( !AppIsTugboat() && AppIsSinglePlayer())
	{
		AppCommonSetPause( bPause );
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AppUserPause(
	BOOL bPause)
{
	if (!AppIsTugboat() && AppIsSinglePlayer())
	{
		CommonTimerSetPause(AppCommonGetTimer(), bPause);
		if (!bPause)
		{
			AppCommonSetStepSpeed(APP_STEP_SPEED_DEFAULT);
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
HINSTANCE AppGetHInstance(
	void)
{
	CLT_VERSION_ONLY( return AppCommonGetHInstance() );
	SVR_VERSION_ONLY( return NULL );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
GAME * AppGetCltGame(
	void)
{
	return gApp.pClientGame;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
void AppSetCltGame(
	GAME *pGame)
{
	trace("Setting client game.\n");
	if (pGame)
	{
		ASSERTX_RETURN( gApp.pClientGame == NULL, "Client game already set" );
	}
	gApp.pClientGame = pGame;
}	
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
void sUpdateCensorshipFromBadges()
{
	const BADGE_COLLECTION * pBadges = AppGetBadgeCollection();
	ASSERT_RETURN( pBadges );

	BOOL bUnderAge = pBadges->HasBadge( ACCT_STATUS_UNDERAGE );
	BOOL bCensorGore = bUnderAge;
	BOOL bCensorBoneShrinking = bUnderAge;
	BOOL bCensorParticles = bUnderAge;
	BOOL bCensorPvP = FALSE;

	int nSkuCurrent = SKUGetCurrent();
	if ( nSkuCurrent != INVALID_ID )
	{
		const SKU_DATA * pSkuData = (const SKU_DATA *) ExcelGetData( EXCEL_CONTEXT(), DATATABLE_SKU, nSkuCurrent );
		if ( pSkuData )
		{
			if ( pSkuData->bCensorMonsterClassReplacementsNoGore )
				bCensorGore = TRUE;
			if ( pSkuData->bCensorBoneShrinking )
				bCensorBoneShrinking = TRUE;
			if ( pSkuData->bCensorParticles )
				bCensorParticles = TRUE;
			if ( pSkuData->bCensorPvPForUnderAge )
				bCensorPvP = bUnderAge;
			if ( pSkuData->bCensorPvP )
				bCensorPvP = TRUE;		
		}
	}
	AppSetCensorFlag( AF_CENSOR_NO_GORE, bCensorGore, TRUE );
	AppSetCensorFlag( AF_CENSOR_NO_BONE_SHRINKING, bCensorBoneShrinking, TRUE );
	AppSetCensorFlag( AF_CENSOR_PARTICLES, bCensorParticles, TRUE );
	AppSetCensorFlag( AF_CENSOR_NO_PVP, bCensorPvP, TRUE );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
const BADGE_COLLECTION *AppGetBadgeCollection(void)
{
	if(AppGetType() == APP_TYPE_SINGLE_PLAYER ||
	   AppGetType() == APP_TYPE_OPEN_SERVER || 
	   AppGetType() == APP_TYPE_OPEN_CLIENT)
	{
		//Return the global definition cheat badges
		GLOBAL_DEFINITION * pGlobal = DefinitionGetGlobal();
		return &(pGlobal->Badges);
	}
	else return gApp.m_pRemoteBadgeCollection; //Return badge collection server
	//sent to us at login.  Could be null, if we have not yet logged in.
}
#endif

//----------------------------------------------------------------------------
// Badges actually get set multiple times, since we log into multiple
// servers who all report the badges as we log in.  But that's ok.
//
// We assume all servers report the same badges.  If we get different badges
// and haven't called AppResetPlayerData(), that is an error.
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
void AppSetBadgeCollection(BADGE_COLLECTION &tBadges)
{
	if(!gApp.m_pRemoteBadgeCollection)
	{
		gApp.m_pRemoteBadgeCollection =(BADGE_COLLECTION*) MALLOC(NULL, sizeof(BADGE_COLLECTION));
		new (gApp.m_pRemoteBadgeCollection) BADGE_COLLECTION(tBadges);
	}
	else
	{
		ASSERT(*gApp.m_pRemoteBadgeCollection == tBadges);
	}
	sUpdateCensorshipFromBadges();
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sCommonInitCallbacks(
	void)
{
#if !ISVERSION(SERVER_VERSION)
	SetConsoleStringCallbackChar(ConsoleString1);
	SetConsoleStringCallbackWideChar(ConsoleString1);
	SetDebugStringCallbackChar(ConsoleDebugStringV);
	SetDebugStringCallbackWideChar(ConsoleDebugStringV);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void AppResetPlayerData(
	void )
{
#if !ISVERSION(SERVER_VERSION)
	gApp.m_characterGuid = INVALID_GUID;
	gApp.m_wszPlayerName[0] = 0;
	gApp.m_idLevelChatChannel = INVALID_CHANNELID;
	FREE(NULL, gApp.m_pRemoteBadgeCollection);
	gApp.m_pRemoteBadgeCollection = NULL;

	c_IgnoreListInit();
	c_BuddyListInit();
	c_PlayerClearParty();
	c_PlayerClearPartyInvites();
	c_PlayerClearGuildInfo();
	c_PlayerClearGuildLeaderInfo();
	c_PlayerInitGuildMemberList();
	c_ClearIgnoredChatChannels();
	c_ClearHypertextChatMessages();
	c_EulaStateReset();
	c_PlayerEmailInit();
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AppForceAllowSoundsToLoad()
{
	return (AppIsFillingPak() || AppGetAllowFillLoadXML());
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_AppInit( HINSTANCE hInstance )
{
#if !ISVERSION(SERVER_VERSION)
	#if defined(USERNAME_cbatson)
	bool bFlag = !!gApp.m_bNoSound;	// CHB
	#endif

	structclear(gApp);

	#if defined(USERNAME_cbatson)
	gApp.m_bNoSound = bFlag;		// CHB
	#endif

	//gApp.hInstance = hInstance;		// duplicated in appcommon
	gApp.m_idSelectedGame = INVALID_ID;	// initialize this

	gApp.m_gameChannel = NULL;
	gApp.m_chatChannel = NULL;
	gApp.m_billingChannel = NULL;
	gApp.m_patchChannel = NULL;
#if ISVERSION(DEVELOPMENT)
	gApp.m_fillpakChannel = NULL;
#endif

	AppResetPlayerData();
	PathSetup();

	gApp.szServerIP[0] = 0;
	gApp.pServerSelection = NULL;
	gApp.szAuthenticationIP[0] = 0;
    gApp.userName[0] = 0;
    gApp.passWord[0] = 0;
	gApp.m_wszLastWhisperSender[0] = 0;

	if (AppIsHellgate())
	{
		gApp.eStartupAppType = APP_TYPE_CLOSED_CLIENT;
	}
	else if (AppIsTugboat())
	{
		gApp.eStartupAppType = APP_TYPE_CLOSED_CLIENT;
	}

    gApp.serverPort = DEFAULT_SERVER_PORT;

    gApp.m_GameClientCommandTable = c_NetGetCommandTable();
    gApp.m_GameServerCommandTable = s_NetGetCommandTable();
	ClientRegisterImmediateMessages();
	gApp.m_GameLoadBalanceClientCommandTable = GameLoadBalanceGetClientRequestTable();
	gApp.m_GameLoadBalanceServerCommandTable = GameLoadBalanceGetResponseTable();
    gApp.m_ChatServerCommandTable= c_ChatNetGetServerResponseCmdTable();
    gApp.m_ChatClientCommandTable =c_ChatNetGetClientRequestCmdTable();
    gApp.m_BillingClientCommandTable = BuildBillingClientRequestCommandTable();
	gApp.m_BillingServerCommandTable = BuildBillingClientResponseCommandTable();
    gApp.m_PatchClientCommandTable = PatchClientGetRequestTable();
	gApp.m_PatchServerCommandTable = PatchClientGetResponseTable();
#if 0
	gApp.m_FillPakClientCommandTable = FillPakClientGetRequestTable();
	gApp.m_FillPakServerCommandTable = FillPakClientGetResponseTable();
#endif

	gApp.m_hPatchInitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
#if ISVERSION(RETAIL_VERSION) && !_PROFILE
	gApp.m_bAllowPatching = TRUE;
#else
	gApp.m_bAllowPatching = FALSE;
#endif
	gApp.m_bAllowFillLoadXML = FALSE;
	gApp.m_fnameDelete[0] = '\0';
	gApp.m_bHasDelete = FALSE;
	gApp.m_bEulaHashReady = FALSE;
	gApp.m_iOldEulaHash = 0;
	gApp.m_iNewEulaHash = 0;
#else
	SVR_VERSION_ONLY(REF(hInstance));
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static
BOOL sInitPakFile(void)
{
#if ISVERSION(SERVER_VERSION) && !ISVERSION(DEBUG_VERSION)
	if (!PakFileInitForApp())
	{
		TraceError("Game server failed to load pakfiles from disk\n");
		LogErrorInEventLog(EventCategoryGameServer, MSG_ID_GAME_SERVER_PAKFILE_ERROR, NULL, NULL, 0);
		return FALSE;
	}
#else
	ASSERT_RETFALSE(PakFileInitForApp());
#endif
	return true;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(CLIENT_ONLY_VERSION)
void sDownloadServerListCallback(
	HTTP_DOWNLOAD * download)
{
	ASSERT_RETURN(download);

	if (!download->buffer)
	{
		return;
	}

	AUTOFREE autofree(download->pool, download->buffer);

	OS_PATH_CHAR filename[MAX_PATH];
	PStrPrintf(filename, arrsize(filename), OS_PATH_TEXT("serverlist.xml"));

	FileSave(filename, download->buffer, download->len);
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AppPreInit(
	HINSTANCE hInstance,
	const char * cmdline)
{
	TIMER_START("AppPreInit()");
	
#if !ISVERSION(SERVER_VERSION)
	CrashHandlerInit("Prime");
#endif

    CLT_VERSION_ONLY(CryptoInit());
	TraceDebugOnly(">>>>>>>>>>>>>>>>>>>>>>> Begin Load <<<<<<<<<<<<<<<<<<<<<<<");	

#if ISVERSION(DEVELOPMENT) && defined(PERFORMANCE_ENABLE_TIMER)
	gLoadTimer.Start();
#endif

	CLT_VERSION_ONLY(structclear(gtAppCommon));
	gtAppCommon.m_hInstance = hInstance;
    CLT_VERSION_ONLY(gtAppCommon.m_pInetContext = NetHttpInit(NULL,NULL));
	
	// c_AppInit() calls PathSetup()
	// PathSetup sets a value in the gtAppCommon structure, so we have to call this AFTER
	// clearing out gtAppCommon
	CLT_VERSION_ONLY(c_AppInit( hInstance ));

	
	// CHB 2006.09.28 - Want to use the pak file in a retail build.
#if ISVERSION(RETAIL_VERSION)
	gtAppCommon.m_bDirectLoad	 = FALSE;
	gtAppCommon.m_bUpdatePakFile = FALSE;
	gtAppCommon.m_bUsePakFile	 = TRUE;
#endif

#if ISVERSION(DEVELOPMENT)
	gtAppCommon.m_bDirectLoad	 = TRUE;
	gtAppCommon.m_bUpdatePakFile = TRUE;
	gtAppCommon.m_bUsePakFile	 = TRUE;
#endif

#if ISVERSION(DEMO_VERSION)
	gtAppCommon.m_bDirectLoad	 = FALSE;
	gtAppCommon.m_bUpdatePakFile = TRUE;
	gtAppCommon.m_bUsePakFile	 = TRUE;
#endif

#if ISVERSION(SERVER_VERSION)	//	retail hellgate server needs a filling pak
	gtAppCommon.m_bDirectLoad	 = TRUE;
	gtAppCommon.m_bUpdatePakFile = TRUE;
	gtAppCommon.m_bUsePakFile	 = TRUE;
#endif

	gtAppCommon.m_sGenericName = sgszGenericAppName;

	if (AppIsHammer())
	{
		gtAppCommon.m_bDirectLoad	 = TRUE;
		gtAppCommon.m_bUpdatePakFile = FALSE;
		gtAppCommon.m_bUsePakFile	 = FALSE;
	}

	if (!CommonTimerInit(AppCommonGetTimer()))
	{
		CommonTimerFree(AppCommonGetTimer());
		return FALSE;
	}

	gGameSettings.m_nExpBonus = 0;
	if (cmdline)
	{
		AppParseCommandLine(cmdline);
	}

	// BSP 5-14-07
	// This is for community day and I believe alpha
	//   Force multiplayer only in clientonlyrelease 
	if (ISVERSION(CLIENT_ONLY_VERSION))
	{
		gtAppCommon.m_bMultiplayerOnly = TRUE;
		gtAppCommon.m_bSingleplayerOnly = FALSE;
	}

	// #if ISVERSION(CLIENT_ONLY_VERSION)
	// HttpDownload(NULL, "http://www.mythos.com/serverlist.xml", sDownloadServerListCallback);
	// #endif

	if (cmdline != NULL) {
		PStrCopy(gtAppCommon.m_szCmdLine, cmdline, MAX_PATH);
	}

	return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AppInit(
	HINSTANCE hInstance,
	const char * cmdline)
{
	TIMER_START("AppInit()");

	if (!PrimeDidPreInitForGlobalStrings())
	{
		if (!AppPreInit(hInstance, cmdline))
		{
			return false;
		}
	}

	ASSERTX_RETFALSE( sizeof( time_t ) == sizeof( __time64_t ), "Expected time_t is 64 bits" );
	
	SysInfo_Init();	// CHB 2007.04.05

	CLT_VERSION_ONLY(c_JobsInit());

	if (!PrimeDidPreInitForGlobalStrings())
	{
		if (!sInitPakFile())
		{
			return FALSE;
		}
	}

	if (!PrimeDidPreInitForGlobalStrings())
	{
		TIMER_START("CDefinitionContainer::Init()")
		CDefinitionContainer::Init();
	}
	
#if !ISVERSION(SERVER_VERSION)
	AsyncFileSetProcessLoopCallback( FileLoadUpdateLoadingScreenCallback );
#endif

	GLOBAL_DEFINITION * global = DefinitionGetGlobal();
	if ( global )
	{ // we have to do this here because we can't do it when parsing the commandline
		global->dwFlags |= gtAppCommon.m_dwGlobalFlagsToSet;
		global->dwFlags &= ~gtAppCommon.m_dwGlobalFlagsToClear;
		global->dwGameFlags |= gtAppCommon.m_dwGameFlagsToSet;
		global->dwGameFlags &= ~gtAppCommon.m_dwGameFlagsToClear;
	}
#if !ISVERSION(SERVER_VERSION)
	if (!e_CheckVersion())
	{
		return FALSE;
	}
#endif
		
	LogCheckEnabled();

	MemoryInitThreadPool();

#ifdef HAVOK_ENABLED
	if (AppIsHellgate())
	{
		TIMER_START("HavokSystemInit()");
		HavokSystemInit();
	}
#endif

#if !ISVERSION(SERVER_VERSION)
	c_SoundInitEarlyCallbacks(AppIsFillingPak, AppForceAllowSoundsToLoad);
#endif

	if (!PrimeDidPreInitForGlobalStrings())
	{
		StringTableInitForApp();
		LanguageInitForApp( AppGameGet() );
	}
	QuestsInitForApp();
	
#if defined(XFIRE_VOICECHAT_ENABLED) && !ISVERSION(SERVER_VERSION)
	if (!AppIsHammer())
		CLT_VERSION_ONLY( c_VoiceChatInit() );
#endif

	if (!PrimeDidPreInitForGlobalStrings())
	{
		TIMER_START("ScriptInitEx()");
		ScriptInitEx();
	}

	if (global)
	{
		if (!AppIsHammer())
        {
            if (global->dwFlags & GLOBAL_FLAG_FORCE_SYNCH)
            {
                CLT_VERSION_ONLY( gApp.m_bForceSynch = TRUE );
            }
#if !ISVERSION(SERVER_VERSION)
			//	server runner controls this on a global level
            if (global->dwFlags & GLOBAL_FLAG_SILENT_ASSERT || AppCommonGetMinPak() || //AppTestDebugFlag(ADF_FILL_PAK_CLIENT_BIT) ||
                global->dwFlags & GLOBAL_FLAG_NO_POPUPS)
            {
                gtAppCommon.m_bSilentAssert = TRUE;
            }
#endif
        }
    }

	CLT_VERSION_ONLY( c_TownPortalInitForApp() );

	CLT_VERSION_ONLY( AppSetType(gApp.eStartupAppType) );

	{
		TIMER_START("ExcelLoadData()");
		if (!PrimeDidPreInitForGlobalStrings())
		{
			EXCEL_LOAD_MANIFEST manifest;
			ASSERT_RETFALSE(ExcelInit(manifest));
		}
		ASSERT_RETFALSE(ExcelLoadData());
	}

	ChatFilterInitForApp();
	NameFilterInitForApp();

	// set the SKU for the app
	SKUInitializeForApp();
	
	// do SKU validation (if requested)
#if ISVERSION(DEVELOPMENT)	
	int nSKUToValidate = SKUGetSKUToValidate();
	if (nSKUToValidate != INVALID_LINK)
	{
	
		// do the compare
		StringTableCompareSKULanguages( nSKUToValidate );
		
		// when in this validation mode, we always exit the app ... this is
		// a tool for the build system to be able to quickly verify the 
		// validity of all the string data
		return FALSE;
		
	}
#endif

#if !ISVERSION(SERVER_VERSION) && ISVERSION(DEVELOPMENT)
	if(gApp.m_szSKUOverride[0] != '\0')
	{
		int nSKU = ExcelGetLineByStringIndex(NULL, DATATABLE_SKU, gApp.m_szSKUOverride);
		SKUOverride(nSKU);
	}
#endif

	// set the region for the app
	RegionInitForApp();

	c_AdClientFixupSKUs();

	{
		TIMER_START("ConsoleInitBuffer");
		ConsoleInitBuffer();
	}

	CLT_VERSION_ONLY( c_WardrobeSystemInit() );
	
#if !ISVERSION(SERVER_VERSION)
	// Initialize engine settings.
	V(e_CommenceInit());
	Settings_InitAndLoad();		// Adjust settings between e_CommenceInit() and e_ConcludeInit()
	V_DO_FAILED(e_ConcludeInit())
	{
		goto _error;
	}
#endif

	if(AppIsHellgate())
	{
#if !ISVERSION(SERVER_VERSION) && !defined(TUGBOAT)
		c_AdClientInit();
#endif		
	}

	const CONFIG_DEFINITION * config = DefinitionGetConfig();
	if (config)
	{
		if (!AppIsHammer())
		{
			if (config->dwFlags & CONFIG_FLAG_NOSOUND)
			{
				CLT_VERSION_ONLY( gApp.m_bNoSound = TRUE );
			}
		}
	}
	// this is for adding the serverlist to the pak
//	const SERVERLIST_DEFINITION * pServerlist = DefinitionGetServerlist();
//	UNREFERENCED_PARAMETER(pServerlist);

	ServerListLoad(AppGetCltGame());

	sCommonInitCallbacks();

	if (AppUseDatabase())
	{
		gtAppCommon.m_Database = DatabaseInit(gtAppCommon.m_szDatabaseAddress, gtAppCommon.m_szDatabaseServer, 
			gtAppCommon.m_szDatabaseUser, gtAppCommon.m_szDatabasePassword, gtAppCommon.m_szDatabaseDb);
		if (!AppGetDatabase())
		{
			ASSERTX(0, "Error Initializing Database");
			goto _error;
		}
		if (!DatabaseCreateTables(AppGetDatabase()))
		{
			ASSERTX(0, "Error Creating Database Tables");
			goto _error;
		}
	}

	CLT_VERSION_ONLY( gApp.m_dwMainThreadID = GetCurrentThreadId() );
	CLT_VERSION_ONLY( PatchClientSetWaitFunction(AppPatcherWaitingFunction, 30) );

	CLT_VERSION_ONLY( AppSwitchState(APP_STATE_INIT) );

	CLT_VERSION_ONLY( gApp.m_nCurrentMovieList = INVALID_LINK );
	CLT_VERSION_ONLY( gApp.m_nCurrentMovieListIndex = -1 );
	CLT_VERSION_ONLY( gApp.m_nCurrentMovieListSet = -1 );

	return TRUE;

_error:
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AppClose(
	void)
{
#if !ISVERSION(SERVER_VERSION)
	c_JobsFree();
	c_WardrobeSystemDestroy();

	if (AppGetCltGame())
	{
		GameFree(AppGetCltGame());
		AppSetCltGame( NULL );
	}

	c_MoviePlayerAppClose();
	c_InputClose();
	c_SoundClose();
	e_DeviceRelease();

    NetHttpFree(AppCommonGetHttp());
#endif

	if (AppUseDatabase())
	{
		if (gtAppCommon.m_Database)
		{
			DatabaseFree(AppGetDatabase());
		}
	}


	if(AppIsHellgate())
	{
#if !ISVERSION(SERVER_VERSION) && !defined(TUGBOAT)
		c_AdClientShutdown();
#endif		
	}

#if !ISVERSION(SERVER_VERSION)
	UIOptionsDeinit();	// CHB 2007.03.29
	GfxOptions_Deinit();// CHB 2007.10.22
	e_Deinit();			// CHB 2007.03.29
#endif

	QuestsFreeForApp();
	LanguageFreeForApp();
	StringTableFree();
	ExcelFree();
	CDefinitionContainer::Close();

	ServerListFree();

#if defined(XFIRE_VOICECHAT_ENABLED) && !ISVERSION(SERVER_VERSION)
	if (!AppIsHammer())
		CLT_VERSION_ONLY( c_VoiceChatClose() );
#endif

#ifdef HAVOK_ENABLED
	if (AppIsHellgate())
	{
		HavokSystemClose();
	}
#endif

	ScriptFreeEx();
	ConsoleFree();

	CLT_VERSION_ONLY( c_TownPortalFreeForApp() );
	CLT_VERSION_ONLY( c_ClearIgnoredChatChannels() );
	CLT_VERSION_ONLY( c_ClearHypertextChatMessages() );

    CLT_VERSION_ONLY(NetCommandTableFree(gApp.m_GameClientCommandTable));
    CLT_VERSION_ONLY(NetCommandTableFree(gApp.m_GameServerCommandTable));
	CLT_VERSION_ONLY(NetCommandTableFree(gApp.m_GameLoadBalanceClientCommandTable));
	CLT_VERSION_ONLY(NetCommandTableFree(gApp.m_GameLoadBalanceServerCommandTable));
    CLT_VERSION_ONLY(NetCommandTableFree(gApp.m_ChatClientCommandTable));
    CLT_VERSION_ONLY(NetCommandTableFree(gApp.m_ChatServerCommandTable));
    CLT_VERSION_ONLY(NetCommandTableFree(gApp.m_BillingClientCommandTable));
    CLT_VERSION_ONLY(NetCommandTableFree(gApp.m_BillingServerCommandTable));
	CLT_VERSION_ONLY(NetCommandTableFree(gApp.m_PatchClientCommandTable));
	CLT_VERSION_ONLY(NetCommandTableFree(gApp.m_PatchServerCommandTable));
#if 0
	CLT_VERSION_ONLY(NetCommandTableFree(gApp.m_FillPakClientCommandTable));
	CLT_VERSION_ONLY(NetCommandTableFree(gApp.m_FillPakServerCommandTable));
#endif 
	CLT_VERSION_ONLY(CloseHandle(gApp.m_hPatchInitEvent));
	CLT_VERSION_ONLY(FREE(NULL, gApp.m_pRemoteBadgeCollection));

	// CHB 2006.09.05 - Would cause an access violation during
	// abort of initialization.
	if (AppCommonGetTimer() != 0) {
		CommonTimerFree(AppCommonGetTimer());
	}

	NameFilterFreeForApp();
	ChatFilterFreeForApp();
	PakFileFreeForApp();

	MemoryFreeThreadPool();

	SysInfo_Deinit();	// CHB 2007.04.05

#if !defined(_WIN64)
	CLT_VERSION_ONLY( CrashHandlerDispose() );
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AppSingleStep(
	void)
{
	if (AppIsMultiplayer() == FALSE)
	{
		CommonTimerSingleStep(AppCommonGetTimer());
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
void AppInitHammerGame(
	GAME * game)
{
	const GLOBAL_DEFINITION * global_definition = DefinitionGetGlobal();
	ASSERT_RETURN(global_definition);
	const GAME_GLOBAL_DEFINITION * game_global_definition = DefinitionGetGameGlobal();
	ASSERT_RETURN(game_global_definition);
			
	int nLevelDefinition = game_global_definition->nLevelDefinition;
	if ( nLevelDefinition == INVALID_LINK )
	{
		nLevelDefinition = 0;		
	}
	
	LEVEL_SPEC tSpec;
	tSpec.idLevel = 0;
	tSpec.tLevelType.nLevelDef = nLevelDefinition;
	tSpec.nDRLGDef = DungeonGetLevelDRLG(game, nLevelDefinition);
	tSpec.dwSeed = global_definition->dwSeed;
	tSpec.bPopulateLevel = TRUE;

	// tugboat specific	
	if (AppIsTugboat())
	{
		tSpec.tLevelType.nLevelArea = 0;  // supposed to be hard coded?? -Colin
	}

	// add level
	LevelAdd( game, tSpec );
	
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
void AppInitCltGame(
	GAMEID idGame,
	DWORD dwCurrentTick,
	unsigned int nGameSubtype,
	DWORD dwGameFlags,
	GAME_VARIANT *pGameVariant)
{
	trace("Initializing client game %I64x.\n", idGame);

	ASSERT( AppGetCltGame() == NULL );
	
	GAME_SETUP tSetupDefault;
	GameSetupInit( tSetupDefault );
	GAME *game = GameInit(
		GAME_TYPE_CLIENT, 
		idGame, 
		(GAME_SUBTYPE)nGameSubtype, 
		&tSetupDefault, 
		dwGameFlags);
	MemorySetThreadPool(NULL);
	ASSERTX_RETURN(game, "Unable to create client game" );
	AppSetCltGame(game);
	AppSetSrvGameId(idGame);

	game->m_dwGameFlags = dwGameFlags;

	if (AppIsHammer())
	{
		AppInitHammerGame(game);
	}

	gApp.pClientGame->eGameState = GAMESTATE_LOADING;
	
	// set the current game tick to the current game tick of the server, we need to
	// do this now so that during the load process, when we schedule game events they
	// occur at the right time for our now current game ... TODO: maybe clear out some of the
	// events that were here before that would be considered old????? -Colin
	game->tiGameTick = dwCurrentTick;

	// CHB 2006.10.09 - Show FPS display in release build.
#if defined(USERNAME_cbatson) //&& (!defined(_DEBUG))
	UISendMessage(WM_TOGGLEDEBUGDISPLAY, 0 /*UIDEBUG_FPS*/, true);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AppRemoveCltGame(
	void)
{
	trace("Removing client game.\n");
	GAME * game = gApp.pClientGame;
	if (!game)
	{
		return FALSE;
	}

	GameFree(game);
	AppSetCltGame( NULL );
	CharCreateResetUnitLists();

	AppSetSrvGameId(INVALID_ID);

	return TRUE;
}
#endif


#define ExitInactive()						{ Sleep(40); return; }
//#define ExitIfAppIsInactive()				if (!AppIsActive() && !AppTestFlag(AF_PROCESS_WHEN_INACTIVE_BIT)) ExitInactive()
//#define IfAppIsInactive()					if (!AppIsActive() && !AppTestFlag(AF_PROCESS_WHEN_INACTIVE_BIT))


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void ServerAppDraw(
	void)
{
	CommonTimerDisplayFramerateUpdate(AppCommonGetTimer());
	// mark camera data as stale
	CameraSetUpdated(FALSE);
}


//----------------------------------------------------------------------------
// game must be real, unit can be null (character creation)
// bForceRender is false for game, true for character create
//----------------------------------------------------------------------------
static void ClientDrawWorld(
	GAME * game,
	UNIT * unit,
	BOOL bForceRender)
{
	REF(bForceRender);
	SVR_VERSION_ONLY(REF(unit));
	SVR_VERSION_ONLY(REF(game));

#if !ISVERSION(SERVER_VERSION)
	float fElapsedTime = AppCommonGetElapsedTime();

	// draw world
	{
		PERF(DRAW_ROOM)
		if ( ! e_GetRenderFlag( RENDER_FLAG_DPVS_ENABLED ) )
			e_RenderListBegin();
		c_RoomDrawUpdate(game, unit, fElapsedTime);
		if ( ! e_GetRenderFlag( RENDER_FLAG_DPVS_ENABLED ) )
			e_RenderListEnd();
	}

	// update the render lists
	c_UpdateCubeMapGeneration();
	V( e_Update() );

	// zero render metrics
	e_ZeroRenderMetrics();

	// update animation - we could do this less often than drawing
	{
		PERF(HAVOK)
		c_AppearanceSystemAndHavokUpdate(game, fElapsedTime);
	}

	{
		PERF(PARTICLES)
		HITCOUNT(PARTICLES)
		c_ParticleSystemUpdateAll(game, fElapsedTime);
	}

	e_MarkGameDraw();

	{
		PERF(D3D_RENDER)
		V( e_Render(FALSE) );
	}

	V( e_PostRender() );

	CommonTimerDisplayFramerateUpdate(AppCommonGetTimer());

	// mark camera data as stale
	CameraSetUpdated(FALSE);
#endif //!ISVERSION(SERVER_VERSION)
}


//----------------------------------------------------------------------------
// must have game && control unit in this mode
//----------------------------------------------------------------------------
static void ClientAppDraw_InGame(
	void)
{
	PERF(DRAW)

	if ( ! e_ShouldRender() )
	{
		ExitInactive();
	}

	GAME * game = AppGetCltGame();
	if (!game)
	{
		return;
	}

	UNIT * unit = GameGetControlUnit(game);
	if (!unit)
	{
		return;
	}

	if (GameGetDebugFlag(game, DEBUGFLAG_SIMULATE_BAD_FRAMERATE))
	{
		Sleep(100);
	}

	ClientDrawWorld(game, unit, FALSE);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void CharacterCreationDraw(
	void)
{
	PERF(DRAW)

	if ( ! e_ShouldRender() )
	{
		ExitInactive();
	}

	GAME * game = AppGetCltGame();
	if (!game)
	{
		return;
	}

	UNIT * unit = GameGetControlUnit(game);

	ClientDrawWorld(game, unit, TRUE);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void MainMenuDraw(
	void)
{
	PERF(DRAW)
#if !ISVERSION(SERVER_VERSION)

	if ( ! e_ShouldRender() )
	{
		ExitInactive();
	}

	if( AppIsHellgate() )
	{
	
		c_CameraUpdate(NULL, 0);
	
		CameraSetUpdated(TRUE);
	
		// update the render lists
		V( e_Update() );
		V( e_RenderUIOnly( FALSE ) );
	}
	if( AppIsTugboat() )
	{
		GAME * game = AppGetCltGame();
		if (!game)
		{
			return;
		}
	
		c_CameraUpdate(game, 0);

		CameraSetUpdated(TRUE);
	
		UNIT * unit = GameGetControlUnit(game);
	
		ClientDrawWorld(game, unit, TRUE);
	}
#endif //!ISVERSION(SERVER_VERSION)
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void EulaScreenDraw(
	void)

{


	c_CameraUpdate(NULL, 0);

	CameraSetUpdated(TRUE);

	V( e_RenderUIOnly( TRUE ) );
}


//----------------------------------------------------------------------------
// might have game and / or unit
//----------------------------------------------------------------------------
static void LoadingScreenDraw(
	BOOL bForce = FALSE)
{
	//static const DWORD minTicksBetweenDraws = (1000/20);
	PERF(DRAW)

	if (AppGetState() == APP_STATE_CONNECTING_PATCH ||
		( AppIsTugboat() && AppGetState() == APP_STATE_LOADING ))
	{
		UIShowPatchClientProgressChange();
	}

	//static DWORD lastTick = 0;

	//if (bForce == FALSE) 
	//{
	//	DWORD curTick = AppCommonGetRealTime();
	//	if (curTick - lastTick < minTicksBetweenDraws)
	//	{
	//		Sleep(1);
	//		return;
	//	}
	//	lastTick = curTick;
	//}

	c_CameraUpdate(NULL, 0);

	CameraSetUpdated(TRUE);

	V( e_Update() );
	V( e_RenderUIOnly( TRUE ) );

}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
void AppLoadingScreenUpdate(
	BOOL bForce)
{
	PERF(DRAW)

	AppGenericDo();

	CommonTimerUpdate(AppCommonGetTimer());

	UIProcess();

	c_CameraUpdate(AppGetCltGame(), 0);


	LoadingScreenDraw(bForce);
}
#endif !ISVERSION(SERVER_VERSION)

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
void FileLoadUpdateLoadingScreenCallback(
	void)
{
	static LONG last_time = 0;
	static const LONG interval = 100;

	LONG cur_time = GetRealClockTime();
	if (cur_time - last_time < interval)
	{
		return;
	}

	last_time = cur_time;

	AppCommonRunMessageLoop();
	if ( UIShowingLoadingScreen() && !UIUpdatingLoadingScreen())
		AppLoadingScreenUpdate( TRUE );
}
#endif !ISVERSION(SERVER_VERSION)

//----------------------------------------------------------------------------
// Fixing obvious dupe bug by forcing trading to stop before we log people
// out in s_network, making sure we save. -Robert  
// P.S. who put this here?  Stop putting in simple dupe bugs!
//----------------------------------------------------------------------------
static BOOL sOpenPlayerCanBeSaved(
	UNIT *pUnit)
{

	// cannot be saved while trading ... it simplified things
	if (TradeIsTrading( pUnit ))
	{
		return FALSE;
	}
	
	// oooo, kaaaay, tooo, gooooo
	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AppPlayerSave(
	struct UNIT * unit)
{

	if(!AppIsSaving())
	{
		return FALSE;
	}
	
#if ISVERSION(SERVER_VERSION)
	// Ping0 servers do incremental saves to a character database
	// Temporarily disabled to force full save of new persistence data
	if (!s_DatabaseUnitsAreEnabled() == TRUE)
	{
		return TRUE;
	}
	else
	{
		if(sOpenPlayerCanBeSaved(unit) == TRUE)
		{
			if(s_IncrementalDBUnitsAreEnabled() == TRUE)
			{
				return s_DBUnitOperationsImmediateCommit(unit);
			}
			else
			{
				return PlayerSaveToDatabase(unit);
			}
		}
	}
	return FALSE;
#else
	if (sOpenPlayerCanBeSaved( unit ) == TRUE)
	{
		ASSERT_RETFALSE(gApp.m_fpPlayerSave);
		return gApp.m_fpPlayerSave(unit);
	}
	else
	{
		return FALSE;
	}
#endif

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void UpdateSound(
	int frames)
{
	CLT_VERSION_ONLY( c_SoundUpdate( frames ) );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static void ServerProcessGames(
	unsigned int sim_frames)
{
#if !ISVERSION( CLIENT_ONLY_VERSION )
	PERF(SERVER);
	GameServerContext * serverContext = (GameServerContext *)CurrentSvrGetContext();
	ASSERT_RETURN(serverContext);
	GameListProcessGames(serverContext->m_GameList, sim_frames);
#endif
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static void ProcessServer(
	unsigned int sim_frames)
{
#if !ISVERSION(SERVER_VERSION)
	EmbeddedServerRunnerMasqueradeThreadAsServerThread(GAME_SERVER);	
#endif //!ISVERSION(SERVER_VERSION)
	SrvProcessAppMessages();
	ServerProcessGames(sim_frames);
	SrvProcessDisconnectedNetclients();
	SrvProcessDirtyTownInstanceLists();
}
#endif 


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void ProcessServerAllowPause(
	unsigned int sim_frames)
{
#if !ISVERSION(CLIENT_ONLY_VERSION)
#if !ISVERSION(SERVER_VERSION)
	EmbeddedServerRunnerMasqueradeThreadAsServerThread(GAME_SERVER); 
#endif
	SrvProcessAppMessages();
	ServerProcessGames(sim_frames);
	SrvProcessDisconnectedNetclients();
	SrvProcessDirtyTownInstanceLists();
#endif // #if !ISVERSION(CLIENT_ONLY_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION( SERVER_VERSION )
static void AppProcessServer(
	unsigned int sim_frames)
{
	if (gApp.m_fpProcessServer)
	{
		gApp.m_fpProcessServer(sim_frames);
	}
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
void ShutdownClientGame(
	void)
{
	GAME * game = AppGetCltGame();
	if (!game)
	{
		return;
	}
	BOOL bRestarting = GameGetGameFlag(game, GAMEFLAG_RESTARTING);

	c_ParticleSystemsDestroyAll();
	GameFree(game);

	//if( AppIsHellgate() )
	//{
	//	UIHideLoadingScreen();
	//}
	//CSchedulerFree();
	//UIFree();

	MemoryTraceAllFLIDX();
	//CSchedulerInit();
	//UIInit(TRUE);
	UIShowLoadingScreen(0);
		
	// set client state to restarting if we are
	if (bRestarting)
	{
		AppSetType(gApp.eStartupAppType);
		AppSwitchState(APP_STATE_RESTART);
	}
}
#endif //!SERVER_VERSION


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ClientDoLoadTimerHack(
	GAME * game)
{
	if (GameGetTick(game) == 2)
	{
#if ISVERSION(DEVELOPMENT) && defined(PERFORMANCE_ENABLE_TIMER)
		UINT64 loadTime = gLoadTimer.End();
		trace(">>>>>>>>>>>>>>>>>>>>>>> Load Completed in %I64u msecs <<<<<<<<<<<<<<<<<<<<<<<", loadTime);
#endif
	}
}


#if !ISVERSION(SERVER_VERSION)
//----------------------------------------------------------------------------
// must have game && control unit
//----------------------------------------------------------------------------
void ClientProcessGame(
	GAME * game,
	unsigned int sim_frames)
{
	ASSERT_RETURN(game && IS_CLIENT(game));

	UNIT * unit = GameGetControlUnit(game);
	if (!unit)
	{
		return;
	}

	V( e_ModelsUpdate() );

	if (PatchClientRetrieveResetErrorStatus() != 0) {
		AppSetErrorState(GS_NETWORK_ERROR_TITLE, GS_NETWORK_ERROR);
		return;
	}

	CltProcessMessages(500);

	c_ChatNetProcessChatMessages();
	c_BillingNetProcessMessages();

	c_WardrobeUpdate();

	AsyncFileProcessComplete(AppCommonGetMainThreadAsyncId());

	c_DirectInputUpdate(game);

	{	// simulate
		PERF(CLT_SIM);
		for (unsigned int ii = 0; ii < sim_frames; ++ii)
		{	
 			c_DoGameTick(game);
			ClientDoLoadTimerHack(game);
		}
#if !ISVERSION( CLIENT_ONLY_VERSION )			
		c_SinglePlayerSynch(game);
#endif
		UnitDebugUpdate(game);
	}
	
	PlayerMovementUpdate(game, AppCommonGetElapsedTime());
	AppDetachedCameraUpdate();
	c_CameraUpdate(game, sim_frames);

	// Update sound every frame, not every tick.  This avoids nasty
	// problems where 3D sound positions are updated every frame, while
	// the listener position is only updated every tick.
	UpdateSound(sim_frames);
	
	if(AppIsHellgate() && sim_frames)
	{
#if !ISVERSION(SERVER_VERSION) && !defined(TUGBOAT)
		c_AdClientUpdate();
#endif
	}

	if (!UnitGetRoom(unit))
	{
		return;
	}

	{
		PERF(CLT_INTERPOLATE);
		DoUnitStepInterpolating(game, AppCommonGetElapsedTickTime(), AppCommonGetElapsedTime());

		if (AppIsHellgate())
		{
			PlayerRotationInterpolating(game, AppCommonGetElapsedTickTime());
		}
	}

	c_UIUpdateSelection(game);
	c_UIUpdateNames(game);
	UIProcessChatBubbles(game);
	c_UIUpdateDebugLabels();

	if ( e_ShouldRender() )
	{
		UIProcess();
	}

	if (AppIsHellgate())
	{
		c_PlayerUpdateClientFX(game);
	}

#if DEBUG_WINDOW
	DaveDebugWindowUpdate(game);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ClientProcessGame(
	unsigned int sim_frames)
{
	if (!AppGetCltGame())
	{
		return;
	}

	ClientProcessGame(AppGetCltGame(), sim_frames);

	if (AppGetCltGame() && GameGetState(AppGetCltGame()) == GAMESTATE_REQUEST_SHUTDOWN)
	{
		ShutdownClientGame();
	}
}

#endif  //!ISVERSION(SERVER_VERSION)


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void GlobalDebugTrackerUpdate(
	void)
{
	GlobalEventTrackerUpdate();
	GlobalEventHandlerTrackerUpdate();
}


//----------------------------------------------------------------------------
// server only (closed server) game loop
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
void ServerOnly_DoInGame(
	unsigned int sim_frames)
{
	CommonTimerElapse(AppCommonGetTimer());
	GlobalDebugTrackerUpdate();
	ProcessServer(sim_frames);
	ServerAppDraw();
}
#endif


#if !ISVERSION(SERVER_VERSION)
//----------------------------------------------------------------------------
// client only (open or closed client) game loop
//----------------------------------------------------------------------------
static void ClientOnly_DoInGame(
	unsigned int sim_frames)
{
	PERF(CLIENT);

	CommonTimerElapse(AppCommonGetTimer());
	GlobalDebugTrackerUpdate();
	
	
#ifdef _PROFILE
	extern bool g_ProfileSnapshot;
	extern long g_ProfileSnapshotCount;
	if(g_ProfileSnapshot)
	{
		PROFILE_COMMAND_STATUS profileResult = StartProfile(PROFILE_GLOBALLEVEL,PROFILE_CURRENTID);
		profileResult = SuspendProfile(PROFILE_GLOBALLEVEL, PROFILE_CURRENTID);
		profileResult = MarkProfile(0);		

		g_ProfileSnapshotCount = 0;
		g_ProfileSnapshot = false;		
	}

	if(g_ProfileSnapshotCount < 1)
	{
		ResumeProfile(PROFILE_GLOBALLEVEL, PROFILE_CURRENTID);
	}	
#endif
	
	
	ClientProcessGame(sim_frames);
	ClientAppDraw_InGame();
	

#ifdef _PROFILE
	if(g_ProfileSnapshotCount < 1)
	{
		PROFILE_COMMAND_STATUS profileResult = SuspendProfile(PROFILE_GLOBALLEVEL, PROFILE_CURRENTID);
		profileResult = MarkProfile(g_ProfileSnapshotCount++);		
	}	

	if(g_ProfileSnapshotCount == 1)
	{
		StopProfile(PROFILE_GLOBALLEVEL, PROFILE_CURRENTID);
		g_ProfileSnapshotCount++;
	}
#endif

	
}


//----------------------------------------------------------------------------
// open server game loop
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION) && !ISVERSION(RETAIL_VERSION)
static void OpenServer_DoInGame(
	unsigned int sim_frames)
{
	CommonTimerElapse(AppCommonGetTimer());
	GlobalDebugTrackerUpdate();
	ProcessServer(sim_frames);

	PERF(CLIENT);
	ClientProcessGame(sim_frames);
	ClientAppDraw_InGame();
}
#endif


//----------------------------------------------------------------------------
// single player game loop
//----------------------------------------------------------------------------
#if !ISVERSION(CLIENT_ONLY_VERSION)
static void SinglePlayer_DoInGame(
	unsigned int sim_frames)
{
	if ( ! e_ShouldRender() )
	{
		ExitInactive();
	}

	CommonTimerElapse(AppCommonGetTimer());
	GlobalDebugTrackerUpdate();

#if ISVERSION(DEVELOPMENT) && !ISVERSION(SERVER_VERSION)
	if (AppIsActive() && AppGetCltGame() != NULL)
	{
		DebugSetTimeLimiter();
	}
	else
	{
		DebugClearTimeLimiter();
	}
#endif

#ifdef _PROFILE
	extern bool g_ProfileSnapshot;
	extern long g_ProfileSnapshotCount;
	if(g_ProfileSnapshot)
	{
		PROFILE_COMMAND_STATUS profileResult = StartProfile(PROFILE_GLOBALLEVEL,PROFILE_CURRENTID);
		profileResult = SuspendProfile(PROFILE_GLOBALLEVEL, PROFILE_CURRENTID);
		profileResult = MarkProfile(0);		

		g_ProfileSnapshotCount = 0;
		g_ProfileSnapshot = false;		
	}

	if(g_ProfileSnapshotCount < 1)
	{
		ResumeProfile(PROFILE_GLOBALLEVEL, PROFILE_CURRENTID);
	}	
#endif

	ProcessServer(sim_frames);
	

	PERF(CLIENT);

	ClientProcessGame(sim_frames);

	ClientAppDraw_InGame();
	
#ifdef _PROFILE
	if(g_ProfileSnapshotCount < 1)
	{
		PROFILE_COMMAND_STATUS profileResult = SuspendProfile(PROFILE_GLOBALLEVEL, PROFILE_CURRENTID);
		profileResult = MarkProfile(g_ProfileSnapshotCount++);		
	}	

	if(g_ProfileSnapshotCount == 1)
	{
		StopProfile(PROFILE_GLOBALLEVEL, PROFILE_CURRENTID);
		g_ProfileSnapshotCount++;
	}
#endif

#if ISVERSION(DEVELOPMENT) && !ISVERSION(SERVER_VERSION)
	DebugClearTimeLimiter();
#endif
}
#endif


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL CheckConnectingTimeOut(
	void)
{
	if (AppGetType() == APP_TYPE_SINGLE_PLAYER)
	{
		return FALSE;
	}

	static const DWORD wait_timeout = 1800 * MSECS_PER_SEC; //Changing wait time to half an hour from 25 secs -rjd

	if (gApp.m_tiAppWaitTime == TIME_ZERO)
	{
		gApp.m_tiAppWaitTime = AppCommonGetCurTime();
		return FALSE;
	}
	if (TimeGetElapsed(gApp.m_tiAppWaitTime, AppCommonGetCurTime()) < wait_timeout)
	{
		return FALSE;
	}

	gApp.m_tiAppWaitTime = TIME_ZERO;
	AppSetErrorState(
		GS_ERROR,
		GS_ERROR_CONNECTING_TO_SERVER,
		APP_STATE_RESTART, 
		AppGetCltGame() && GameGetControlUnit(AppGetCltGame()));
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
float GameGetTimeFloatCallback()
{
	return GameGetTimeFloat(AppGetCltGame());
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void c_InitEngineCallbacks(
	PFN_TOOLNOTIFY pfn_ToolUpdate,
	PFN_TOOLNOTIFY pfn_ToolRender,
	PFN_TOOLNOTIFY pfn_ToolDeviceLost)
{
	e_InitCallbacks(
		GameGetTimeFloatCallback,
		c_ParticlesDraw,
		c_ParticlesDrawLightmap,
		c_ParticlesDrawPreLightmap,
		c_AppearanceGetBones,
#ifdef HAVOK_ENABLED
		c_RagdollGetPosition,
#else
		NULL,
#endif
		c_AppearanceDestroy,
		c_AppearanceCreateSkyboxModel,
#ifdef HAVOK_ENABLED
		c_AppearanceDefinitionGetSkeletonHavokVoid,
#else
		NULL,
#endif
		c_AppearanceDefinitionGetInverseSkinTransform,
		c_AnimatedModelDefinitionJustLoaded,
		pfn_ToolUpdate,
		pfn_ToolRender,
		pfn_ToolDeviceLost,
		c_AdClientAddAd,
		RoomFileWantsObscurance,
		RoomFileInExcel,
		RoomFileOccludesVisibility );

	e_UIInitCallbacks(
		UIRenderAll,
		UIRestoreVBuffers,
		UIDisplayChange );	// CHB 2007.08.23
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL InitGraphics(
	HWND hWnd,
	PFN_TOOLNOTIFY pfn_ToolUpdate /*= NULL*/,
	PFN_TOOLNOTIFY pfn_ToolRender /*= NULL*/,
	PFN_TOOLNOTIFY pfn_ToolDeviceLost /*= NULL*/,
	const OptionState * pOverrideState /*= NULL*/ )
{
	// seed callbacks
	c_AttachmentInit();
	InitCameraFunctions( c_CameraGetInfoCommon, c_CameraUpdateCallback, c_CameraGetNearFar );
	c_InitDebugBars( SetUIBarCallback );
	c_InitEngineCallbacks( pfn_ToolUpdate, pfn_ToolRender, pfn_ToolDeviceLost );

	c_ParticleSystemsInit();


	// If there is a temporary state to use, apply it now
	if ( pOverrideState )
	{
		V( e_OptionState_CommitToActive(pOverrideState) );
	}

	V_DO_FAILED( e_DeviceInit( hWnd ) )
		return FALSE;

	c_AppearanceSystemInit();
	V( e_AnimatedModelDefinitionSystemInit() );
	c_RoomSystemInit();
	c_StateLightingDataEngineLoad();

	// commit the window size and topmost setting
	V( e_DoWindowSize() );
	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void DestroyGraphics()
{
	// don't bother cleaning up from fillpak, since it blows away memory willy-nilly and causes refcount asserts
	TIMER_START_NEEDED_EX("DestroyGraphics()", 1000);

	c_AppearanceSystemClose();
	V( e_AnimatedModelDefinitionSystemClose() );
	c_RoomSystemDestroy();
	c_ParticleSystemsDestroy();

	V( e_Destroy() );

	c_AttachmentDestroy();
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AppSetTypeByStartData(
	const STARTGAME_PARAMS & tData)
{
	if (tData.m_bSinglePlayer)
	{
		AppSetType(APP_TYPE_SINGLE_PLAYER);
		return;
	}

	switch (gApp.eStartupAppType)
	{
	case APP_TYPE_NONE:
	case APP_TYPE_SINGLE_PLAYER:
		gApp.eStartupAppType = APP_TYPE_CLOSED_CLIENT;
	}

	AppSetType(gApp.eStartupAppType);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AppPlayMenuMusic()
{
	int nMenuMusic = GlobalIndexGet(GI_MUSIC_MENU);
	if(c_MusicGetCurrentPlayingTrack() != nMenuMusic)
	{
		c_MusicPlay(nMenuMusic);
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL FillPakClientInit()
{
	// Nothing for now
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL GameClientInit()
{
	int nPlayerClass = CharacterClassGetPlayerClass( 
		g_UI_StartgameParams.m_nCharacterClass, 
		g_UI_StartgameParams.m_nCharacterRace, 
		g_UI_StartgameParams.m_eGender);

	ASSERT_RETFALSE(nPlayerClass != INVALID_ID);

	WCHAR szPlayerName[MAX_CHARACTER_NAME];
	ConnectionStateGetCharacterName(szPlayerName, MAX_CHARACTER_NAME);


	trace("Initializing game client with name %ls\n", szPlayerName);

	if (g_UI_StartgameParams.m_bNewPlayer) 
	{
		c_SendNewPlayer(
			g_UI_StartgameParams.m_bNewPlayer, 
			szPlayerName, 
			g_UI_StartgameParams.m_nPlayerSlot,
			nPlayerClass, 
			&gWardrobeBody, 
			&gAppearanceShape,
			g_UI_StartgameParams.m_dwGemActivation,
			g_UI_StartgameParams.dwNewPlayerFlags,
			GlobalIndexGet( GI_DIFFICULTY_DEFAULT )); // A new character can *only* start w/ Normal difficulty
	} 
	else 
	{
		c_SendNewPlayer(
			g_UI_StartgameParams.m_bNewPlayer, 
			szPlayerName,
			g_UI_StartgameParams.m_nPlayerSlot,
			nPlayerClass, 
			NULL, 
			NULL,
			g_UI_StartgameParams.m_dwGemActivation,
			0,  // unused? we will get what need need out of it from player save file? -cday
			g_UI_StartgameParams.m_nDifficulty);
	}

	return TRUE;
}

//----------------------------------------------------------------------------
// callback function to UIShowMainMenu
//----------------------------------------------------------------------------
void DoneMainMenu(
	BOOL bStartGame,
	const STARTGAME_PARAMS & tData)
{
	if (!bStartGame)
	{
		AppSwitchState(APP_STATE_EXIT);
		return;
	}

	AppSetTypeByStartData(tData);

	if (tData.m_bNewPlayer)
	{
		GAME * game = AppGetCltGame();
		ASSERT_RETURN(game);
		UNIT * unit = UnitGetById(game, tData.m_idCharCreateUnit);
		ASSERT_RETURN(unit);
		int nWardrobe = UnitGetWardrobe(unit);
		ASSERT_RETURN(nWardrobe != INVALID_ID);
		WARDROBE_BODY* pBody = c_WardrobeGetBody( nWardrobe );
		ASSERT_RETURN( pBody );
		MemoryCopy(&gWardrobeBody, sizeof(WARDROBE_BODY), pBody, sizeof(WARDROBE_BODY));
		MemoryCopy(&gAppearanceShape, sizeof(APPEARANCE_SHAPE), unit->pAppearanceShape, sizeof(APPEARANCE_SHAPE));
	}

	// remove the dummy client game we created especially for the main menu
	AppRemoveCltGame();
}
#endif //!SERVER_VERSION


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void sOldDataVersion(
	const WCHAR *puszAuthMessage)
{			
#if !ISVERSION(SERVER_VERSION)
	ASSERTX_RETURN( puszAuthMessage, "Expected auth message string" );
	
	// get message
	const int MAX_MESSAGE = 2048;
	WCHAR uszMessage[ MAX_MESSAGE ] = { 0 };				   
	PStrCopy( uszMessage, puszAuthMessage, MAX_MESSAGE );

	// get URL
	const int MAX_URL = 1024;
	WCHAR uszURL[ MAX_URL ] = { 0 };
	RegionCurrentGetURL( RU_DOWNLOAD_UPDATES, uszURL, MAX_URL );

	// replace URL in message
	const WCHAR *puszToken = StringReplacementTokensGet( SR_PATCH_URL );
	PStrReplaceToken( uszMessage, MAX_MESSAGE, puszToken, uszURL );

	// setup dialog callback
	DIALOG_CALLBACK tCallback;
	tCallback.pfnCallback = sAppErrorStateCallbackGotoDownloadUpdates;
	
	// open message dialog
	AppSetErrorStateLocalized( 
		GlobalStringGet( GS_MULTI_PLAYER_UPDATE_AVAILABLE ),
		uszMessage,
		APP_STATE_RESTART,
		FALSE,
		&tCallback);
#endif
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
#define AppSetConnectionErrorState(eState) AppSetConnectionErrorStateWithLine(eState, __LINE__)

static void AppSetConnectionErrorStateWithLine(CONNECTION_STATE eState, int nLine)
{
	if (eState == CONNECTION_STATE_AUTHTICKET_FAILED)
	{
		AUTH_FAILURE tAuthFailure;
		AuthTicketGetFailureReason( &tAuthFailure );		
		
		if (tAuthFailure.dwFailureCode == AUTH_TICKET_ERROR_411_OLD_DATA_VERSION)
		{
			sOldDataVersion( tAuthFailure.uszFailureReason );
		}
		else
		{
			AppSetErrorStateLocalized( 
				GlobalStringGet( GS_NETWORK_ERROR ),
				tAuthFailure.uszFailureReason);  // TODO: Localize this!
		}
	}
	if (eState == CONNECTION_STATE_BILLING_FAILED) {
		AppSetErrorStateLocalized(GlobalStringGet(GS_ERROR_CONNECTING_TO_SERVER), 
			c_BillingGetSubscriptionTypeString(c_BillingGetSubscriptionType()));
	}
	else if (AppGetState() != APP_STATE_MENUERROR) // don't replace existing menu error
	{
		if(eState == CONNECTION_STATE_FIRST_SERVER_CONNECTION_FAILED)
		{//If it's the first server we connect to other than auth, it's probably a firewall issue.
			const int MAX_MESSAGE = 256;
			WCHAR uszMessage[ MAX_MESSAGE ];
			PStrPrintf(
				uszMessage,
				MAX_MESSAGE, 
				GlobalStringGet( GS_CONNECTION_ERROR_CHECK_CONFIG ),
				eState, 
				nLine);
			AppSetErrorStateLocalized( 
				GlobalStringGet( GS_ERROR_CONNECTING_TO_SERVER ),
				uszMessage );
		}
		else
		{//otherwise, it's probably not.
			const int MAX_MESSAGE = 256;
			WCHAR uszMessage[ MAX_MESSAGE ];
			PStrPrintf(
				uszMessage,
				MAX_MESSAGE,
				L"%ls (%d-%d)",
				GlobalStringGet( GS_NETWORK_ERROR ),
				eState, 
				nLine);
			AppSetErrorStateLocalized( 
				GlobalStringGet( GS_ERROR_CONNECTING_TO_SERVER ),
				uszMessage );
		}
	}	
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AppGenericDo(
	void)
{
#if !ISVERSION(SERVER_VERSION)
	// in other app states, we should be calling ConnectionStateUpdate() regularly, so we don't need to recheck
	APP_STATE state = AppGetState();
	if (state == APP_STATE_LOADING || state == APP_STATE_IN_GAME) {
		if (!ConnectionStateVerifyExpectedChannelStates()) {
			AppSetErrorState(
				GS_NETWORK_ERROR_TITLE,
				GS_NETWORK_ERROR);
			return;
		}
	}
	SendChannelKeepAliveMsgs();
#endif //!ISVERSION(SERVER_VERSION)

	CLT_VERSION_ONLY( c_JobsUpdate() );
	if (!AppTestDebugFlag(ADF_FILL_PAK_CLIENT_BIT)) {
		e_CompatDB_CheckForUpdates();	// CHB 2006.12.13
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AppExitInit(
	APP_STATE eFromState, 
	va_list args)
{
	REF(eFromState);
	REF(args);

#if ISVERSION(DEVELOPMENT)
	if (AppCommonGetMinPak() || AppIsFillingPak())
	{
		PakFileLog();
	}
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AppExitDo(
	unsigned int sim_frames)
{
	REF(sim_frames);

	PostQuitMessage(0);

	PostQuitMessage(0);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void AppLoadingInit(
	APP_STATE eFromState, 
	va_list args )
{
	REF(eFromState);

#if !ISVERSION(SERVER_VERSION)
	WCHAR * title = va_arg(args, WCHAR *);
	WCHAR * text  = va_arg(args, WCHAR *);
	PatchClientClearUIProgressMessage();
	UIShowLoadingScreen(0, title, text);
	e_SetUIOnlyRender(TRUE);
	gApp.m_tiAppWaitTime = 0;


	// If this is a demo run, update the global override DRLG seed with the demo run seed
	if ( AppCommonGetDemoMode() )
	{
		GLOBAL_DEFINITION * pGlobalDefinition = DefinitionGetGlobal();
		GAME_GLOBAL_DEFINITION * pGameGlobalDefinition = DefinitionGetGameGlobal();
		DEMO_LEVEL_DEFINITION * pDemoDef = DemoLevelGetDefinition();
		if ( pDemoDef && pGlobalDefinition )
		{
			pGlobalDefinition->dwSeed = pDemoDef->dwDRLGSeed;
		}
		if ( pDemoDef && pGameGlobalDefinition )
		{
			pGameGlobalDefinition->nDRLGOverride		= pDemoDef->nDRLGDefinition;
			pGameGlobalDefinition->nLevelDefinition		= pDemoDef->nLevelDefinition;
		}
	}
	


extern BOOL gbLoadingUpdateNow;
if (gbLoadingUpdateNow == TRUE)
{	
	AppLoadingScreenUpdate( TRUE );	
}
#else
	REF(args);
    ASSERT(FALSE);
#endif //!ISVERSION(SERVER_VERSION)
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void AppLoadingEnd()
{
}


//----------------------------------------------------------------------------
// loading screen
//----------------------------------------------------------------------------
static void AppLoadingDo(
	unsigned int sim_frames)
{
	SVR_VERSION_ONLY(REF(sim_frames));
#if !ISVERSION(SERVER_VERSION)
	PERF(CLIENT);

	AppGenericDo();

	AppProcessServer(sim_frames);

	CommonTimerElapse(AppCommonGetTimer());

	if (PatchClientRetrieveResetErrorStatus() != 0) {
		AppSetErrorState(GS_NETWORK_ERROR_TITLE, GS_NETWORK_ERROR);
		return;
	}

	CltProcessMessages(500);

	if (CheckConnectingTimeOut())
	{
		return;
	}

	GAME * game = AppGetCltGame();

	AsyncFileProcessComplete(AppCommonGetMainThreadAsyncId());

	if (game)
	{
		ClientProcessGame(sim_frames);

		// if there is no control unit, it didn't really do a UI process
		if( !GameGetControlUnit(game) )
		{
			UIProcess();	
		}
		game = AppGetCltGame();
		if (game && GameGetState(game) == GAMESTATE_REQUEST_SHUTDOWN)
		{
			ShutdownClientGame(); 
		}
	}
	else	//otherwise it is called in ClientProcessGame
	{
		UIProcess();	
	}

	game = AppGetCltGame();
	if(game)
	{
		c_DirectInputUpdate(game); // The above game pointer may have been invalidated...
	}

	c_WardrobeUpdate();		

	UpdateSound(sim_frames);

	c_MoviePlayerShowFrame();

	if(AppIsHellgate() && sim_frames)
	{
#if !ISVERSION(SERVER_VERSION) && !defined(TUGBOAT)
		c_AdClientUpdate();
#endif
	}

	LoadingScreenDraw();
#endif //!SERVER_VERSION
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void AppRestartInit(
	APP_STATE eFromState, 
	va_list args )
{
	REF(args);
	REF(eFromState);
#if !ISVERSION(SERVER_VERSION)
	CLT_VERSION_ONLY( UICloseAll() );
	CLT_VERSION_ONLY( UIChatClose(FALSE) );

	ConnectionStateFree();
	ConsoleClear();
	UIHideLoadingScreen();
	UIUnitLabelsAdd(NULL, 0, 0);
	if (c_VoiceChatIsEnabled())
	{
		c_VoiceChatStop();
		c_VoiceChatStart();
	}
	else
	{
		c_VoiceChatStop();	
	}
	PatchClientRetrieveResetErrorStatus();
	if(AppGetCltGame() != NULL)
	{
		trace("Closing current game for restart.");
		AppRemoveCltGame();
	}

#else
    ASSERT(FALSE);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void AppRestartDo(
	unsigned int sim_frames)
{
	REF(sim_frames);
	AppSwitchState(APP_STATE_MAINMENU);
	AppResetPlayerData();
}

void AppConnectingStartInit(
	APP_STATE eFromState, 
	va_list args)
{
	UNREFERENCED_PARAMETER(args);
	UNREFERENCED_PARAMETER(eFromState);

#if !ISVERSION(SERVER_VERSION)
	ConnectionStateFree();

	UIShowLoadingScreen(0);
	e_SetUIOnlyRender(TRUE);

	ConnectionStateInit(NULL);

	gApp.m_tiAppWaitTime = 0;

#else
	ASSERT(FALSE);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static void AppConnectingStartDo(
	unsigned int sim_frames)
{
#if !ISVERSION(SERVER_VERSION)
	AppGenericDo();
	AppProcessServer(sim_frames);

	// All this is apparently necessary to update UI stuff	
	c_DirectInputUpdate(AppGetCltGame());							// update any input
	UIUpdateLoadingScreen();
	c_MoviePlayerShowFrame();
	UIProcess();
	AsyncFileProcessComplete(AppCommonGetMainThreadAsyncId());
	UpdateSound(sim_frames);
	if(AppIsHellgate() && sim_frames)
	{
#if !ISVERSION(SERVER_VERSION) && !defined(TUGBOAT)
		c_AdClientUpdate();
#endif
	}

	MainMenuDraw();
 
	{
		CONNECTION_STATE eState = ConnectionStateUpdate();

		switch(eState)
		{
		case CONNECTION_STATE_GETTING_TICKET:
			break; //Still waiting for ticket.
		case CONNECTION_STATE_CONNECTING_TO_PATCH:
			AppSwitchState(APP_STATE_CONNECTING_PATCH);
			break;
		case CONNECTION_STATE_CONNECTING_TO_BILLING:
		case CONNECTION_STATE_CONNECTING_TO_CHAT_AND_GAME:
			AppSwitchState(APP_STATE_CONNECTING);
			break;
		case CONNECTION_STATE_DISCONNECTED:
		default:
			AppSetConnectionErrorState(eState);			
		}
	}

#else
	UNREFERENCED_PARAMETER(sim_frames);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AppConnectingPatchInit(
	APP_STATE eFromState, 
	va_list args)
{
	UNREFERENCED_PARAMETER(args);
	UNREFERENCED_PARAMETER(eFromState);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void AppConnectingPatchDo(
	unsigned int sim_frames)
{
#if !ISVERSION(SERVER_VERSION)

 	AppGenericDo();
	AppProcessServer(sim_frames);

	if (PatchClientRetrieveResetErrorStatus() != 0) {
		AppSetErrorState(GS_ERROR_PATCH_UNKNOWN, GS_ERROR_PATCH_UNKNOWN);
		return;
	}

	CONNECTION_STATE eOldState = ConnectionStateGetState();
	BOOL bAdvanceState = !AppTestFlag( AF_PATCH_ONLY ) || !(eOldState == CONNECTION_STATE_PATCHING);
	CONNECTION_STATE eState = ConnectionStateUpdate(bAdvanceState);
	switch(eState)
	{
	case CONNECTION_STATE_CONNECTING_TO_PATCH:
	case CONNECTION_STATE_PATCHING:
		AppLoadingDo(sim_frames);
		Sleep(50);
		break;
	case CONNECTION_STATE_CONNECTING_TO_BILLING:
	case CONNECTION_STATE_CONNECTING_TO_CHAT_AND_GAME:
	
		// if this client was only given an authorization ticket to do patching, we will
		// not allow this client to go forward
		if (AppTestFlag( AF_PATCH_ONLY ))
		{
			GLOBAL_STRING eString = GS_ERROR_AUTH_INVALID_DATA_VERSION_411;
			if (AppAppliesMinDataPatchesInLauncher())
			{
				eString = GS_ERROR_AUTH_INVALID_DATA_VERSION_411_RUN_LAUNCHER;
			}
			const WCHAR *puszMessage = GlobalStringGet( eString );
			ASSERTX_RETURN( puszMessage, "Expected message" );
			sOldDataVersion( puszMessage );			
			return;
		}
		
		AppSwitchState(APP_STATE_CONNECTING);
		break;
	case CONNECTION_STATE_DISCONNECTED:
	default:
		AppSetConnectionErrorState(eState);
	}

#else
	UNREFERENCED_PARAMETER(sim_frames);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void AppConnectingPatchEnd()
{
#if !ISVERSION(SERVER_VERSION)
	PatchClientClearUIProgressMessage();
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
void AppRestartForPatchServer()
{
	WCHAR cmdline[2*DEFAULT_FILE_WITH_PATH_SIZE];

	if (gApp.m_bHasDelete) {
		PStrCopy(cmdline, GetCommandLineW(), 2*DEFAULT_FILE_WITH_PATH_SIZE);
	} else {
		PStrPrintf(cmdline, 2*DEFAULT_FILE_WITH_PATH_SIZE, L"%s -delete%S",
			GetCommandLineW(), gApp.m_fnameDelete);
	}
	trace("cmdline: %S\n", cmdline);

	// Parse out the exe name and the param list
	LPWSTR param = NULL, cTmp = NULL;;
	if (*cmdline == L'\"') {
		cTmp = PStrChr(cmdline+1, L'\"');
		if (cTmp != NULL) {
			cTmp = PStrChr(cTmp+1, L' ');
		}
	} else {
		cTmp = PStrChr(cmdline, L' ');
	}
	if (cTmp == NULL) {
		param = NULL;
	} else {
		*cTmp = L'\0';
		param = cTmp+1;
	}

	WCHAR szLauncherPath[MAX_PATH];
	if( AppIsHellgate() )
	{
		PStrPrintf(szLauncherPath, MAX_PATH, L"%sLauncher.exe", AppCommonGetRootDirectory(NULL));
	}
	else	// TRAVIS : Hey - we don't USE the launcher yet!
	{
		GetModuleFileNameW(NULL, szLauncherPath, DEFAULT_FILE_WITH_PATH_SIZE);
	}

	// Restart the game
	SHELLEXECUTEINFOW exInfo;
	memclear(&exInfo, sizeof(exInfo));
	exInfo.cbSize = sizeof(exInfo);
	exInfo.lpVerb = L"Open";
	exInfo.lpFile = szLauncherPath;
	exInfo.lpParameters =  param;
	exInfo.nShow = SW_SHOW;
	if (!ShellExecuteExW(&exInfo)) {
		DWORD dwError = GetLastError();
		PStrPrintf(cmdline, 2*DEFAULT_FILE_WITH_PATH_SIZE, L"Cannot execute: error %d", dwError);
		trace("%S\n", cmdline);
//		ASSERTX(FALSE, cmdline);
	}

	AppSwitchState(APP_STATE_EXIT);
}
#endif
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void AppConnectingInit(
	APP_STATE eFromState, 
	va_list args)
{
	UNREFERENCED_PARAMETER(args);
	UNREFERENCED_PARAMETER(eFromState);
#if !ISVERSION(SERVER_VERSION)

	BOOL bSinglePlayer = (AppGetType() == APP_TYPE_SINGLE_PLAYER);
	WCHAR * connectingText = (bSinglePlayer) ? L"" : GlobalStringGet( GS_CONNECTING );
	UIShowLoadingScreen(0, connectingText, L"");

#else
    ASSERT(FALSE);
#endif //!SERVER_VERSION
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void AppConnectingDo(
	unsigned int sim_frames)
{
#if !ISVERSION(SERVER_VERSION)
	AppGenericDo();
	AppProcessServer(sim_frames);

	c_DirectInputUpdate(NULL);

	UIUpdateLoadingScreen();
	c_MoviePlayerShowFrame();
	UIProcess();

	c_EulaStateUpdate();
	EULA_STATE eEulaState = c_EulaGetState();
	if (eEulaState == EULA_STATE_EULA || eEulaState == EULA_STATE_TOS) {

		if( AppIsHellgate() )
		{
			UIHideLoadingScreen(FALSE);
		}
		UISetCursorState(UICURSOR_STATE_POINTER);
		EulaScreenDraw();
	} else {
		if( AppIsHellgate() )
		{
			UIShowLoadingScreen(FALSE);
		}
		UISetCursorState(UICURSOR_STATE_WAIT);
		
		LoadingScreenDraw();
	}

	AsyncFileProcessComplete(AppCommonGetMainThreadAsyncId());

	{
		//Once we finish billing, we must IMMEDIATELY advance to
		//the next state.  Otherwise, it un-finishes itself.
		//If we're connecting to game, however, we need to not advance 
		//until we're ready for character select.
		CONNECTION_STATE eOldState = ConnectionStateGetState();
		CONNECTION_STATE eNewState = ConnectionStateUpdate( 
			(eOldState == CONNECTION_STATE_CONNECTING_TO_BILLING) );
		switch(eNewState)
		{
		case CONNECTION_STATE_CONNECTING_TO_BILLING:
			break;
		case CONNECTION_STATE_CONNECTING_TO_CHAT_AND_GAME:
			break; //keep going until we're authed.
		case CONNECTION_STATE_SELECTING_CHARACTER:
			if(eEulaState == EULA_STATE_DONE)
			{
				UIMultiplayerStartCharacterSelect();
			}
			break;
		case CONNECTION_STATE_DISCONNECTED:
		default:
			AppSetConnectionErrorState(eOldState);
			break;
		}
	}

#else
	UNREFERENCED_PARAMETER(sim_frames);
    ASSERT(FALSE);
#endif //!SERVER_VERSION
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(DEVELOPMENT)
#if !ISVERSION(SERVER_VERSION)
extern HANDLE hSendFileEvent;
VOID FillPakClientResponseHandler(
	struct NETCOM* pNetcom,
	NETCLIENTID netCltId,
	BYTE* data,
	UINT32 msg_size);
#endif
#endif

void AppFillPakInit(
	APP_STATE eFromState, 
	va_list args )
{
	REF(args);
	REF(eFromState);
#if !ISVERSION(SERVER_VERSION)
	if ( ! AppGetCltGame() )
		AppInitCltGame(0, 0);
	ASSERT(AppGetCltGame());
	e_SetUIOnlyRender(TRUE);
	AppSetDebugFlag(ADF_DONT_ADD_TO_PAKFILE, FALSE);

	if (AppTestDebugFlag(ADF_FILL_PAK_CLIENT_BIT)) {
#if ISVERSION(DEVELOPMENT)
		ASSERTX_GOTO(AppGetType() == APP_TYPE_CLOSED_CLIENT, "Has to run with -fpc", _err);
		AppConnectingStartInit(eFromState, args);
		e_WardrobeInit();

		NetCommandTableRegisterImmediateMessageCallback(
			gApp.m_FillPakServerCommandTable,
			FPC_FILE_INFO_RESPONSE,
			FillPakClientResponseHandler);
		hSendFileEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		ASSERT_GOTO(hSendFileEvent != NULL, _err);
#endif
	}

	return;

#if ISVERSION(DEVELOPMENT)
_err:
	AppSwitchState(APP_STATE_EXIT);
#endif

#else
	ASSERT(FALSE);
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AppFillPakDo(
	unsigned int sim_frames)
{
	REF(sim_frames);
#if !ISVERSION(SERVER_VERSION)
	GLOBAL_DEFINITION * pDefinition = (GLOBAL_DEFINITION *) DefinitionGetGlobal();
	int nNumErrors = 0;
    BOOL bNoPopups = FALSE;
    if(pDefinition)
    {
        if(pDefinition->dwFlags & GLOBAL_FLAG_NO_POPUPS)
        {
            bNoPopups = TRUE;
        }
    }

	if (!AppTestDebugFlag(ADF_FILL_PAK_CLIENT_BIT)) 
	{
		// Normal single-client fillpak

		AppGenericDo();	

		TracePersonal("FILLPAK - AppFillPakDo() - Start");

		// fill normal pak file
		if (AppTestDebugFlag( ADF_FILL_PAK_BIT ))
		{
			FILLPAK_TYPE ePakType = AppTestDebugFlag( ADF_FILL_PAK_MIN_BIT ) ? FILLPAK_MIN : FILLPAK_DEFAULT;
			if (FillPak( ePakType) == FALSE)
			{
				nNumErrors++;
				if(!bNoPopups) MessageBox( NULL, "Error filling pak file", "Error", MB_OK | MB_ICONERROR );	// CHB 2007.08.01 - String audit: development
			}
		}

		// fill sound pak file
		if (AppTestDebugFlag( ADF_FILL_SOUND_PAK_BIT ))
		{
			if (FillPak( FILLPAK_SOUND ) == FALSE)
			{
				nNumErrors++;
				if(!bNoPopups) MessageBox( NULL, "Error filling sound pak file", "Error", MB_OK | MB_ICONERROR );	// CHB 2007.08.01 - String audit: development
			}		
		}

		// fill language pak
		if (AppTestDebugFlag( ADF_FILL_LOCALIZED_PAK_BIT ))
		{
			if (FillPak( FILLPAK_LOCALIZED ) == FALSE)
			{
				nNumErrors++;
				if(!bNoPopups) MessageBox( NULL, "Error filling language pak file", "Error", MB_OK | MB_ICONERROR );	// CHB 2007.08.01 - String audit: development	
			}
		}
		
		TracePersonal("FILLPAK - AppFillPakDo() - Finished");


		// generate advert dump file
		if (AppTestDebugFlag( ADF_FILL_PAK_ADVERT_DUMP_BIT ))
		{
			if (FillPak( FILLPAK_ADVERT_DUMP ) == FALSE)
			{
				nNumErrors++;
				if(!bNoPopups) MessageBox( NULL, "Error generating advert dump", "Error", MB_OK | MB_ICONERROR );	// CHB 2007.08.01 - String audit: development
			}		
		}

		TraceDebugOnly("Fill Pak Complete with %d errors\n", nNumErrors );			
        if(nNumErrors > 0 && bNoPopups)
        {
           ExitProcess(nNumErrors); 
        }
	} else {
#if 0
		// client running against fillpak server
		ASSERT_GOTO(AppGetType() == APP_TYPE_CLOSED_CLIENT, _err);

 		AppGenericDo();
		AppProcessServer(sim_frames);

		CONNECTION_STATE eState = FillPakClient_ConnectionStateUpdate();
		switch(eState)
		{
			case CONNECTION_STATE_STARTUP:
			case CONNECTION_STATE_GETTING_TICKET:
			case CONNECTION_STATE_CONNECTING_TO_PATCH:
			case CONNECTION_STATE_PATCHING:
			case CONNECTION_STATE_CONNECTING_TO_FILLPAK:
				AppLoadingDo(sim_frames);
				Sleep(50);
				return;
			case CONNECTION_STATE_CONNECTED_TO_FILLPAK:
				break;
			case CONNECTION_STATE_DISCONNECTED:
			default:
				AppSetConnectionErrorState(eState);
				return;
		}

		CHANNEL* pChannel = AppGetFillPakChannel();
		ASSERT_GOTO(pChannel != NULL, _err);

		MAILSLOT* pMailslot = ConnectionManagerGetMailSlotForChannel(pChannel);
		ASSERT_GOTO(pMailslot != NULL, _err);

		BYTE message[MAX_SMALL_MSG_STRUCT_SIZE];
		MSG_BUF* msg = MailSlotPopMessage(pMailslot);

		if (msg != NULL) {
			if (NetTranslateMessageForRecv(gApp.m_FillPakServerCommandTable, msg->msg, msg->size, (MSG_STRUCT *)message)) {
				BOOL bExit = FALSE;
				FillPak_Execute((MSG_STRUCT*)message, bExit);
				if (bExit) {
					AppSwitchState(APP_STATE_EXIT);
					return;
				}
			} else {
				ASSERTX(FALSE, "Invalid Server Fillpak Message");
			}
			MailSlotRecycleMessages(pMailslot, msg, msg);
		}
		Sleep(0);
		return;
#endif
	}


//_err:
#endif// !ISVERSION(SERVER_VERSION)
	// exit	
	AppSwitchState(APP_STATE_EXIT);

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AppDumpSkillDescsInit(
	APP_STATE eFromState, 
	va_list args )
{

#if !ISVERSION(SERVER_VERSION)

	//We need to create a client game
	if ( ! AppGetCltGame() )
		AppInitCltGame(0,0);
	
	ASSERT(AppGetCltGame());
			


#endif

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AppDumpSkillsDescsDo(
	unsigned int sim_frames)
{

#if !ISVERSION(SERVER_VERSION)

	
	 
	
	//loop through all the languages
	//for(int nLanguage = 5; nLanguage < 6 ; ++nLanguage)  //LanguageGetNumLanguages(AppGameGet())
	//{

		APP_GAME appGame = AppGameGet();
		
		char* tmplanguageName = "";
		tmplanguageName = (char *)LanguageGetName(appGame,LanguageGetCurrent());
		//LanguageSetOverride(tmplanguageName);
		//LanguageSetCurrent(appGame,(LANGUAGE)nLanguage);		
		//LanguageInitForApp(appGame);
		//AppGameSet(appGame);
		
		//SKUInitializeForApp();
		//LanguageInitForApp( AppGameGet() );


		GAME * pGame = AppGetCltGame();					
		ASSERT(pGame);
		
		WCHAR uszLanguageName[MAX_LANGUAGE_NAME] = L"";

		
		StringTableFree();
		StringTablesLoad(true,false,SKUGetCurrent());


		for( int t = 0; t < MAX_LANGUAGE_NAME; t++ )
		{
			uszLanguageName[ t ] = (WCHAR)tmplanguageName[ t];
		}
		
		
		int nNumCharacterClasses = CharacterClassNumClasses(); 

		//Make all the paper dolls (we need units to get the skill strings)
		for (int nCharacterClass = 0; nCharacterClass < nNumCharacterClasses; ++nCharacterClass) 
		{          
			//only do enabled classes
			if (!CharacterClassIsEnabled( nCharacterClass, 0, (GENDER)0 ))
			{
				continue;
			}

			//Get the current class (we're only doing one race and gender per class)
			int nPlayerClass = CharacterClassGetPlayerClass( nCharacterClass, 0, (GENDER)0 );
			PLAYER_SPEC tSpec;
			tSpec.nPlayerClass = nPlayerClass;
			UNIT * pUnit = PlayerInit( pGame, tSpec );
			//set the player's level
			UnitSetStat(pUnit,STATS_LEVEL,50);



			//Create the unit and set it as the control;
			UnitStatsInit(pUnit);
			GameSetControlUnit(pGame,pUnit);

			unsigned int numSkills = ExcelGetNumRows(pGame, DATATABLE_SKILLS);
			WCHAR uszHeader[MAX_SKILL_TOOLTIP] = L"Skill Name\tSkill Description\t";
			
			WCHAR uszSkillName[ MAX_SKILL_TOOLTIP ] = L"";
			WCHAR uszSkillDescription[ MAX_SKILL_TOOLTIP ] = L"";
			
			//WCHAR uszSkillDescriptionStripped[ MAX_SKILL_TOOLTIP ];
			WCHAR uszSkillPerLevelEffect[ MAX_SKILL_TOOLTIP ] = L"";
			
			WCHAR uszClassString[ MAX_SKILL_TOOLTIP ] = L"";
	

			char filename[MAX_PATH] = "";
			char fullname[MAX_PATH] = "";

					
			//get the string name for the class
			UnitGetClassString(pUnit, uszClassString, arrsize(uszClassString));
					
			//set the output file (this is likey temporary)
			if(AppIsTugboat())
			{
				
				PStrPrintf((char*)filename, arrsize(filename), (const char*)"data_mythos\\skill_descriptions_%s_%s.txt", (char *)UnitGetDevName(pUnit),LanguageGetCurrentName());
			}
			else
			{
				PStrPrintf((char*)filename, arrsize(filename), (const char*)"data\\skill_descriptions_%s_%s.txt", (char *)UnitGetDevName(pUnit),LanguageGetCurrentName());
			}

			FileGetFullFileName(fullname, filename, MAX_PATH);

			//loop trhough MAX_LEVELS_PER_SKILL and add the column headers for the effect strings
			for(int i = 1; i <= MAX_LEVELS_PER_SKILL; ++i)
			{
				PStrCat(uszHeader,L"Skill Level Effect",arrsize(uszHeader));
				PStrCat(uszHeader,L" ",arrsize(uszHeader));
				WCHAR uszNumber[5];
				_itow_s(i,uszNumber,10);
				PStrCat(uszHeader,uszNumber,arrsize(uszHeader));
				PStrCat(uszHeader,L"\t",arrsize(uszHeader));

			}
			PStrCat(uszHeader,L"\r\n",arrsize(uszHeader));
			
			//open the file and write the header	
			HANDLE file = FileOpen(fullname, GENERIC_WRITE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL);
				if (file == INVALID_FILE)
					continue;
			FileWrite(file,(const void *) uszHeader,PStrLen(uszHeader, arrsize(uszHeader))*sizeof(WCHAR));

			//loop through all the skills
			for (unsigned int skill = 0; skill < numSkills; ++skill)
			{
				const SKILL_DATA * skillData = SkillGetData(pGame, skill);
			
				if (!skillData ||
					!SkillDataTestFlag(skillData, SKILL_FLAG_LEARNABLE) ||
					!UnitIsA(pUnit, skillData->nRequiredUnittype) ||
					SkillDataTestFlag( skillData, SKILL_FLAG_DISABLED ) )
					{
						continue;
					}
				
				//get the name
				DWORD dwSkillLearnFlags = 0;
				SETBIT( dwSkillLearnFlags, SKILLLEARN_BIT_NAME );
				if (!UIGetHoverTextSkill( skill, 0, pUnit, dwSkillLearnFlags, uszSkillName, arrsize( uszSkillName ) ) )
					continue;
				if (!PStrCmp(uszSkillName,L"MISSING STRING INDEX '-1'") || !PStrCmp(uszSkillName,L""))
					continue;
				PStrCat(uszSkillName,L"\t", arrsize(uszSkillName));

				//get the desc
				dwSkillLearnFlags = 0;
				SETBIT( dwSkillLearnFlags, SKILLLEARN_BIT_DESCRIPTION );
				if (!UIGetHoverTextSkill( skill, 0, pUnit, dwSkillLearnFlags, uszSkillDescription, arrsize( uszSkillDescription ) ) )
					continue;
				if (!PStrCmp(uszSkillDescription,L"MISSING STRING INDEX '-1'") || !PStrCmp(uszSkillDescription,L""))
					continue;
				PStrReplaceToken(uszSkillDescription, arrsize(uszSkillDescription), L"Description:", L"");
				PStrCat(uszSkillDescription,L"\t", arrsize(uszSkillDescription));

				//get the level effects
				dwSkillLearnFlags = 0;
				SETBIT( dwSkillLearnFlags, SKILLLEARN_BIT_CURRENT_LEVEL_EFFECT );
				int maxSkillLevel = SkillGetMaxLevelForUI(pUnit, skill);
				
				for (int skillLevel = 0; skillLevel <= maxSkillLevel; ++skillLevel)
				{
	
					if (UIGetHoverTextSkill( skill, skillLevel,pUnit, dwSkillLearnFlags, uszSkillPerLevelEffect, arrsize( uszSkillPerLevelEffect ) ) )
					{
						PStrCat(uszSkillDescription, uszSkillPerLevelEffect, arrsize(uszSkillDescription));
						PStrReplaceToken(uszSkillDescription, arrsize(uszSkillDescription), L"Effect:", L"");
						PStrCat(uszSkillDescription,L"\t", arrsize(uszSkillDescription));
					}
					else
					{
						continue;
					}

				}
				
				WCHAR uszSkillLine [ MAX_SKILL_TOOLTIP ] = L"";
				PStrCat(uszSkillLine,uszSkillName,arrsize(uszSkillLine));
				PStrCat(uszSkillLine,uszSkillDescription,arrsize(uszSkillLine));

				PStrReplaceToken(uszSkillLine, arrsize(uszSkillLine), L"`\n", L"$"); 
				PStrReplaceToken(uszSkillLine, arrsize(uszSkillLine), L"`\a", L"$");
				PStrReplaceToken(uszSkillLine, arrsize(uszSkillLine), L"`\r", L"$"); 
				PStrCat(uszSkillLine, L"\r\n", arrsize(uszSkillLine));

				// strip out the color codes
				WCHAR uszSkillLineStripped[MAX_SKILL_TOOLTIP];
				WCHAR *to = uszSkillLineStripped;
				WCHAR *from = uszSkillLine;
				WCHAR *charCode = NULL;
				*to = L'\0';
				do 
				{
					charCode = PStrChr(from, L'{');
					if (charCode)
					{
						*charCode = L'\0';
						++charCode;
					}
					PStrCat(to, from, arrsize(uszSkillLine));
					if (charCode)
					{
						from = PStrChr(charCode, L'}');
						if (from)
						++from;
					}
				} while (charCode && from && *from != L'\0');

						// newlines
						// replace LF with CRLF (two steps, since replacement string contains originial string)
						//PStrReplaceToken(uszSkillDescriptionStripped, arrsize(uszSkillDescriptionStripped), L"`\n", L"\r\a"); // magic ` allows white space token
						//PStrReplaceToken(uszSkillDescriptionStripped, arrsize(uszSkillDescriptionStripped), L"`\a", L"\n"); // magic ` allows white space token
						//PStrCat(uszSkillDescriptionStripped, L"\r\n\r\n\r\n", arrsize(uszSkillDescriptionStripped));
						
				
				//DWORD byteswritten = 
	
				FileWrite(file,(const void *) uszSkillLineStripped,PStrLen(uszSkillLineStripped, arrsize(uszSkillLineStripped))*sizeof(WCHAR));
			}

			FileClose(file);

 
		}
	//}


	#endif

	AppSwitchState(APP_STATE_EXIT);
	return;
}



//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#if !ISVERSION(SERVER_VERSION)
static unsigned s_nAppInGameFrames = 0;
#endif

static void AppInGameInit(
	APP_STATE eFromState, 
	va_list args)
{
	REF(args);
	REF(eFromState);
#if !ISVERSION(SERVER_VERSION)
	s_nAppInGameFrames = 0;
	gApp.m_tiAppWaitTime = 0;
	if (AppGetType() != APP_TYPE_CLOSED_SERVER)
	{
		
		//UIShowLoadingScreen(TRUE);	this should already be showing from AppLoadingInit
		UIGameStartInit();
		ConsoleInitInterface();				// Reset the console (it needs to find the component ID of its textbox again)
		ConsoleSetEditActive(FALSE);
		if ( AppGetCltGame() )
		{
			UNITID idControlUnit = UnitGetId(UIGetControlUnit());
			UISendMessage(WM_SETCONTROLUNIT, idControlUnit, 0);
			UISendMessage(WM_INVENTORYCHANGE, idControlUnit, -1);	//force redraw of the inventory
		}

		UIHideLoadingScreen();
	}
	e_SetUIOnlyRender(FALSE);

	e_PresentDelaySet();
	
	UISendMessage( WM_RESTART, 0, 0 );

	if ( AppCommonGetDemoMode() )
	{
		UICloseAll( TRUE, TRUE );
		UICrossHairsShow(FALSE);
	}

#endif //!SERVER_VERSION
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void AppInGameDo(
	unsigned int sim_frames)
{
	SVR_VERSION_ONLY(REF(sim_frames));
#if !ISVERSION(SERVER_VERSION)
	++s_nAppInGameFrames;

	// CHB 2006.10.06 - Once video driver processing of data is
	// complete, time to page in other important game data before
	// the first frame is displayed to the user. Finally, enable
	// presentation.
	if (s_nAppInGameFrames == 1)
	{
		e_PfProfMark(("AppInGameDo() calling c_AppearanceProbeAllAnimations()"));
		c_AppearanceProbeAllAnimations();
	}

	#ifdef ENABLE_PAGE_FAULT_PROFILING
	if (s_nAppInGameFrames <= 60) {
		e_PfProfMark(("AppInGameDo() start of frame %d", s_nAppInGameFrames));
	}
	#endif

	AppGenericDo();

	ASSERT_RETURN(gApp.m_fpGameLoop);
	gApp.m_fpGameLoop(sim_frames);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void AppInGameEnd()
{
//	CLT_VERSION_ONLY( UICloseAll() );
	CLT_VERSION_ONLY( UIUnitLabelsAdd(NULL, 0, 0.0f) );		//this will erase all the names above heads
	CLT_VERSION_ONLY( e_SetUIOnlyRender(TRUE) );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

#ifdef ENABLE_PAGE_FAULT_PROFILING
static int s_nCharacterSelectionScreenDrawFrames = 0;
#endif

static void AppCreateInit(
	APP_STATE eFromState, 
	va_list args)
{
	REF(args);
	REF(eFromState);
#if !ISVERSION(SERVER_VERSION)
	#ifdef ENABLE_PAGE_FAULT_PROFILING
	s_nCharacterSelectionScreenDrawFrames = 0;
	#endif

	// Perform an engine cleanup in case we have come from a previous play session.
	//c_ParticleSystemsDestroyAll();
	e_Cleanup( FALSE, TRUE );
	e_ReaperReset();

	// preserve the clt game between calls to create screen -- we free it before we enter gameplay
	if ( ! AppGetCltGame() )
	{
		DWORD dwGameFlags = 0;
		SETBIT(dwGameFlags, GAMEFLAG_DISABLE_ADS);
		AppInitCltGame(0, 0, 0, dwGameFlags);
	}
	ASSERT(AppGetCltGame());

	const WCHAR *puszString = GlobalStringGet( GS_LOADING );
	UIShowLoadingScreen(0, puszString);
	if( AppIsTugboat() )
	{
		UIProcess();
		LoadingScreenDraw();
	}
	else
	{
		c_UIUpdateSelection(NULL);		// turn off the crosshairs
	}
	AppPlayMenuMusic();

	e_PresentDelaySet();

	UIMuteSounds(TRUE);
	UIComponentActivate(UICOMP_CHARACTER_SELECT, TRUE);
	e_SetUIOnlyRender(FALSE);
	UIMuteSounds(FALSE);
	CharacterSelectLoginQueueUiInit();

	if(AppGetType() != APP_TYPE_CLOSED_CLIENT)
	{	//We skip directly here for single player, thus don't get to init.
		ConnectionStateFree();
		ConnectionStateInit(NULL);
	}
	// to match multiplayer, we should open connections before advancing into character creation state
	if(AppGetType() == APP_TYPE_SINGLE_PLAYER)
		ASSERT(ConnectionStateAdvanceToState(CONNECTION_STATE_SELECTING_CHARACTER));

#else
    ASSERT(FALSE);
#endif
}

//----------------------------------------------------------------------------
// character create
// has game, might have unit
//----------------------------------------------------------------------------

static void AppCreateDo(
	unsigned int sim_frames)
{
	SVR_VERSION_ONLY(REF(sim_frames));
#if !ISVERSION(SERVER_VERSION)
	GAME * game = AppGetCltGame();
	ASSERTONCE(game); //This can't be an assert_return, or it loops infinitely!

	CommonTimerElapse(AppCommonGetTimer());

	c_BillingNetProcessMessages(); // need to process billing msgs to get account status updates

	AppGenericDo();

	for(unsigned int ii=0; ii<sim_frames; ii++)
	{
		c_DoGameTick(game);
		//GameEventsProcess(game);
		//GameIncrementGameTick(game);
	}
	UnitDebugUpdate(game);

	V( e_ModelsUpdate() );

	AsyncFileProcessComplete(AppCommonGetMainThreadAsyncId());

	c_DirectInputUpdate(game);

	c_WardrobeUpdate();		

	// game could have been removed here, and the previous 
	// pointer would be invalid
	game = AppGetCltGame();
	c_CameraUpdate(game, sim_frames);

	c_UIUpdateDebugLabels();
	UIProcess();	// game may have been destroyed by here
	
	UpdateSound(sim_frames);
	if(AppIsHellgate() && sim_frames)
	{
#if !ISVERSION(SERVER_VERSION) && !defined(TUGBOAT)
		c_AdClientUpdate();
#endif
	}

	if (!CharacterSelectWaitingInLoginQueue() &&
		UICharacterSelectionScreenIsReadyToDraw()) {

		#ifdef ENABLE_PAGE_FAULT_PROFILING
		if (s_nCharacterSelectionScreenDrawFrames < 10) {
			e_PfProfMark(("CharacterCreationDraw() start of frame %d", ++s_nCharacterSelectionScreenDrawFrames));
		}
		#endif

		UIHideLoadingScreen();

		UICharacterSelectionScreenUpdate();

		CharacterCreationDraw();
		
		if( game )
		{
			GameSetState(game, GAMESTATE_RUNNING);
		}
	} else {
		LoadingScreenDraw();
	}

	if(AppGetState() == APP_STATE_CHARACTER_CREATE)
	{	//We could have already exited this state in the UI processing.
		CONNECTION_STATE eState = ConnectionStateUpdate();
		switch(eState)
		{
		case CONNECTION_STATE_SELECTING_CHARACTER:
			break; //keep on selecting.
		case CONNECTION_STATE_CONNECTED:
			{
				const WCHAR *puszString = GlobalStringGet( GS_LOADING );
				AppSwitchState(APP_STATE_LOADING, puszString, L"");
				break;
			}
		case CONNECTION_STATE_DISCONNECTED:
			AppSetConnectionErrorState(eState);
			break;
		default:
			if(AppGetType() == APP_TYPE_CLOSED_CLIENT)
			{
				AppSetConnectionErrorState(eState);
			}
			else break;
		}
	}
#else
    ASSERT(FALSE);
#endif //!ISVERSION(SERVER_VERSION)
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void AppCreateEnd()
{
#if !ISVERSION(SERVER_VERSION)
	// remove the dummy client game we created especially for the main menu
	AppRemoveCltGame();
	UIComponentActivate(UICOMP_CHARACTER_SELECT, FALSE);
	UIComponentActivate(UICOMP_CHARACTER_CREATE, FALSE);

	UIHideMainMenu();
	e_SetUIOnlyRender(TRUE);
	UINewSessionInit();
#else //!ISVERSION(SERVER_VERSION)
    ASSERT(FALSE);
#endif //!ISVERSION(SERVER_VERSION)
}


//----------------------------------------------------------------------------
// main menus 
//		except for: 
//		character creation
//		loading screen
//		waiting for connection
//----------------------------------------------------------------------------
static void AppMainMenuDo(
	unsigned int sim_frames)
{
	SVR_VERSION_ONLY(REF(sim_frames));
#if !ISVERSION(SERVER_VERSION)
	PERF(CLIENT);

    // This attempts to generate the minimum pak file needed to talk to the
    // patch server so that we can download the rest.
    if(AppCommonGetMinPak() || AppGetAllowFillLoadXML())
    {
        if (GetTickCount() - AppCommonGetMinPakStartTick() > 60000)
        {
			// check for fillpak mode
			if (AppTestDebugFlag( ADF_FILL_PAK_BIT ) )
			{
				TracePersonal("FILLPAK - AppMainMenuDo() - after 60 seconds - continuing?");
				AppSwitchState(APP_STATE_FILLPAK);
			}
			else
       	 	{
				TracePersonal("FILLPAK - AppMainMenuDo() - after 60 seconds - exiting");
            	AppSwitchState(APP_STATE_EXIT);
				return;
       		}
    	}
    }
	if( AppIsHellgate() )
	{
		AppGenericDo();
	
		// update any input
		c_DirectInputUpdate(AppGetCltGame());
	
		UIProcess();
	
		AsyncFileProcessComplete(AppCommonGetMainThreadAsyncId());
	
		UpdateSound(sim_frames);
	
		c_MoviePlayerShowFrame();

		if(sim_frames)
		{
#if !ISVERSION(SERVER_VERSION) && !defined(TUGBOAT)
			c_AdClientUpdate();
#endif
		}

		MainMenuDraw();
	}
	if( AppIsTugboat() )
	{
		GAME * game = AppGetCltGame();
		ASSERT_RETURN(game);
	
		CommonTimerElapse(AppCommonGetTimer());
	
		GameEventsProcess(game);
		GameIncrementGameTick(game);
	
		AsyncFileProcessComplete(AppCommonGetMainThreadAsyncId());
	
		c_DirectInputUpdate(game);
	
		c_JobsUpdate();

		V( e_ModelsUpdate() );
	
		c_WardrobeUpdate();		
	
		UIProcess();	// game may have been destroyed by here
	
		UpdateSound(sim_frames);
		
		MainMenuDraw();
	}
		
#else
    ASSERT(FALSE);
#endif //!ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void AppMenuErrorInit(
	APP_STATE eFromState, 
	va_list args)
{
	REF(args);
	REF(eFromState);

#if !ISVERSION(SERVER_VERSION)
	CLT_VERSION_ONLY( UICloseAll() );
	CLT_VERSION_ONLY( UIChatClose(FALSE) );

	UIHideLoadingScreen(FALSE);
	UIUnitLabelsAdd(NULL, 0, 0);
	AppPlayMenuMusic();
	if (AppIsHellgate())
	{
		e_SetUIOnlyRender(TRUE);
	}
	ConnectionStateFree(); //Instantly kill all connections so we don't die while on the error screen.
#else
    ASSERT(FALSE);
#endif //!ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void AppMainMenuEnd()
{
#if !ISVERSION(SERVER_VERSION)
	if( AppIsHellgate())
	{
		UISetCurrentMenu(NULL);
		UIComponentActivate(UICOMP_STARTGAME_SCREEN, FALSE);
	}
#else //!ISVERSION(SERVER_VERSION)
	ASSERT(FALSE);
#endif //!ISVERSION(SERVER_VERSION)
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void AppMainMenuErrorDo(
	unsigned int sim_frames)
{
	SVR_VERSION_ONLY(REF(sim_frames));
#if !ISVERSION(SERVER_VERSION)
	PERF(CLIENT);

	if( AppIsTugboat() )
	{
		CommonTimerElapse(AppCommonGetTimer());
	
		GAME * game = AppGetCltGame();
		if (game)
		{
			GameEventsProcess(game);
			GameIncrementGameTick(game);
		}
	
		AsyncFileProcessComplete(AppCommonGetMainThreadAsyncId());
	
		c_DirectInputUpdate(game);
	
		c_JobsUpdate();
	
		c_WardrobeUpdate();		
	
		UIProcess();	// game may have been destroyed by here
	
		UpdateSound(sim_frames);
		
		// update the render lists
		V( e_Update() );
		V( e_RenderUIOnly( FALSE ) );
	}
	else
	{
		AppGenericDo();
	
		// update any input
		c_DirectInputUpdate(AppGetCltGame());
	
		UIProcess();
	
		AsyncFileProcessComplete(AppCommonGetMainThreadAsyncId());
	
		UpdateSound(sim_frames);

		c_MoviePlayerShowFrame();

		if(sim_frames)
		{
#if !ISVERSION(SERVER_VERSION) && !defined(TUGBOAT)
			c_AdClientUpdate();
#endif
		}
	
		MainMenuDraw();
	}
#else
    ASSERT(FALSE);
#endif //!ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void AppMainMenuInit(
	APP_STATE eFromState, 
	va_list args)
{
	REF(args);
	REF(eFromState);

#if !ISVERSION(SERVER_VERSION)
	CLT_VERSION_ONLY( UICloseAll() );
	CLT_VERSION_ONLY( UIChatClose(FALSE) );
	CLT_VERSION_ONLY( c_VoiceChatLeaveRoom() );
	
	// force all nonmaterial effects to load in minpak
	if( AppCommonGetMinPak() ||
		AppGetAllowFillLoadXML())
	{
		// just load all effects
		e_EffectsFillMinpak();
		e_LoadNonStateScreenEffects(FALSE);
		// also, load global materials
		e_LoadGlobalMaterials();

	}

	if( AppIsTugboat() )
	{
		if( !AppGetCltGame() )
		{
			AppInitCltGame(0, 0);
		}
		ASSERT(AppGetCltGame());
		UIGameRestart();
		UIShowMainMenu(DoneMainMenu);
		e_SetUIOnlyRender(FALSE);
		if( AppCommonGetMinPak() ||
			AppGetAllowFillLoadXML())
		{
			c_LevelCreatePaperdollLevel( 0 );
		}
		
	}
	AppPlayMenuMusic();
	if( AppIsHellgate() )
	{
		c_UIUpdateSelection(NULL);			// turn off the crosshairs
		UIUnitLabelsAdd(NULL, 0, 0);
		UIChatClose(FALSE);
		UIComponentActivate(UICOMP_STARTGAME_SCREEN, TRUE);
		e_SetUIOnlyRender(TRUE);
		UIShowMainMenu(DoneMainMenu);
	}

	// automatically go into the credits if requested
	c_CreditsTryAutoEnter();
			
#else
    ASSERT(FALSE);
#endif //!ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void AppPlayMovieListEnd()
{
#if !ISVERSION(SERVER_VERSION)
	c_MoviePlayerClose();

	// Ok, normally you're not supposed to do this, but this is sort of a special case
	UI_COMPONENT *pComponent = UIComponentGetByEnum(UICOMP_MAINSCREEN);
	if (pComponent)
	{
		pComponent->m_bVisible = TRUE;
	}
	UIShowSubtitle(L"");
	UIShowPausedMessage(FALSE);

	e_SetUIOnlyRender(FALSE);

	if(AppGetCltGame())
	{
		MSG_CCMD_MOVIE_FINISHED msg;
		msg.nMovieList = gApp.m_nCurrentMovieList;
		c_SendMessage(CCMD_MOVIE_FINISHED, &msg);
	}

#else //!ISVERSION(SERVER_VERSION)
    ASSERT(FALSE);
#endif //!ISVERSION(SERVER_VERSION)
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if !ISVERSION(SERVER_VERSION)
static BOOL sAppPlayMovieListDoStartNextMovie()
{
	ASSERT_RETFALSE(gApp.m_nCurrentMovieListSet >= 0);

	if(gApp.m_nCurrentMovieList < 0)
	{
		return FALSE;
	}

	if(gApp.m_nCurrentMovieListSet >= NUM_MOVIE_LISTS)
	{
		return FALSE;
	}

	if(gApp.m_nCurrentMovieListIndex >= (MAX_MOVIES_IN_LIST - 1))
	{
		gApp.m_nCurrentMovieListSet++;
		gApp.m_nCurrentMovieListIndex = -1;
		return sAppPlayMovieListDoStartNextMovie();
	}

	MOVIE_LIST_DATA * pMovieList = (MOVIE_LIST_DATA*)ExcelGetData(EXCEL_CONTEXT(AppGetCltGame()), DATATABLE_MOVIELISTS, gApp.m_nCurrentMovieList);
	if(!pMovieList)
	{
		return FALSE;
	}

	gApp.m_nCurrentMovieListIndex = MAX(-1, gApp.m_nCurrentMovieListIndex);
	gApp.m_nCurrentMovieListIndex++;

	if(pMovieList->nMovieList[gApp.m_nCurrentMovieListSet][gApp.m_nCurrentMovieListIndex] < 0)
	{
		gApp.m_nCurrentMovieListSet++;
		gApp.m_nCurrentMovieListIndex = -1;
		return sAppPlayMovieListDoStartNextMovie();
	}

	if(c_MoviePlayerOpen(pMovieList->nMovieList[gApp.m_nCurrentMovieListSet][gApp.m_nCurrentMovieListIndex]))
	{
		return TRUE;
	}

	return sAppPlayMovieListDoStartNextMovie();
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void AppPlayMovieListDo(
	unsigned int sim_frames)
{
	SVR_VERSION_ONLY(REF(sim_frames));
#if !ISVERSION(SERVER_VERSION)
	if( AppTestDebugFlag(ADF_NOMOVIES) )
	{
		AppSwitchState(c_MovePlayerGetNextState());
		return;
	}

	if( AppIsHellgate() )
	{
		if(gApp.m_nCurrentMovieListIndex < 0)
		{
			sAppPlayMovieListDoStartNextMovie();
		}

		AppGenericDo();
	
		// update any input
		c_DirectInputUpdate(AppGetCltGame());

		UIProcess();
	
		AsyncFileProcessComplete(AppCommonGetMainThreadAsyncId());

		if ( ! e_ShouldRender() )
		{
			ExitInactive();
		}
	
		UpdateSound(sim_frames);

		// Put movie updating here.
		if(!c_MoviePlayerShowFrame())
		{
			if(!sAppPlayMovieListDoStartNextMovie())
			{
				AppSwitchState(c_MovePlayerGetNextState());
			}
		}

		c_CameraUpdate(NULL, 0);

		CameraSetUpdated(TRUE);

		V( e_Update() );
		V( e_RenderUIOnly( TRUE ) );
	}
	else
	{
		AppSwitchState(c_MovePlayerGetNextState());
	}
#else
    ASSERT(FALSE);
#endif //!ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static void AppPlayMovieListInit(
	APP_STATE eFromState, 
	va_list args)
{
	REF(eFromState);
	REF(args);

#if !ISVERSION(SERVER_VERSION)
	CLT_VERSION_ONLY( UICloseAll() );
	CLT_VERSION_ONLY( UIChatClose(FALSE) );

	// Skip to next state in case of fillpak or fillloadxml
	if (e_IsNoRender()) {
		AppSwitchState(c_MovePlayerGetNextState());
	}
	int nMovieList = va_arg(args, int);

	e_SetUIOnlyRender(TRUE);

	// Ok, normally you're not supposed to do this, but this is sort of a special case
	UI_COMPONENT *pComponent = UIComponentGetByEnum(UICOMP_MAINSCREEN);
	if (pComponent)
	{
		pComponent->m_bVisible = FALSE;
	}
	UIChatClose(FALSE);

	GDI_ClearWindow(AppCommonGetHWnd());
	gApp.m_eAppStateBeforeMovie = eFromState;
	gApp.m_nCurrentMovieList = nMovieList;
	gApp.m_nCurrentMovieListIndex = -1;
	gApp.m_nCurrentMovieListSet = 0;
	c_MoviePlayerInit();
#else
    ASSERT(FALSE);
#endif //!ISVERSION(SERVER_VERSION)
}

//----------------------------------------------------------------------------
// transition to either main menu or connecting
//----------------------------------------------------------------------------
static void AppInitDo(
	unsigned int sim_frames)
{
	REF(sim_frames);
	if (AppGetType() == APP_TYPE_CLOSED_SERVER)
	{
		ASSERTX(0, "Closed server now has it's own init path, AppInitDo should not be called." );
		return;
	}
#if !ISVERSION(SERVER_VERSION)

	// check for fillpak mode
	if ( !AppTestDebugFlag( ADF_FILL_PAK_MIN_BIT ) &&
		( AppTestDebugFlag( ADF_FILL_PAK_BIT ) ||
		  AppTestDebugFlag( ADF_FILL_PAK_ADVERT_DUMP_BIT ) ||
		  AppTestDebugFlag( ADF_FILL_SOUND_PAK_BIT ) ||
		  AppTestDebugFlag( ADF_FILL_LOCALIZED_PAK_BIT ) ) )
	{
		AppSwitchState(APP_STATE_FILLPAK);
		return;
	}

	if (AppTestDebugFlag(ADF_DUMP_SKILL_DESCS))
	{

		//Do the skills desctiption dump		
		AppSwitchState(APP_STATE_DUMPSKILLDESCS);
		return;
	}

	// try to skip menu	
	if (gApp.m_bSkipMenu)
	{
		ONCE
		{
			//Sorry guys, no skipping the menu for multiplayer.
			//why the hell not?  that sux big time!!!!
			AppSetType(APP_TYPE_SINGLE_PLAYER);
			ConnectionStateFree();
			ConnectionStateInit(NULL);
			ConnectionStateAdvanceToState(CONNECTION_STATE_SELECTING_CHARACTER);
		}
		if (   AppCommonGetDemoMode()
			|| UISkipMainMenu(DoneMainMenu))
		{
			AppSwitchState(APP_STATE_LOADING, GlobalStringGet(GS_LOADING), L"");
			CharacterSelectSetCharName( L"", 0, 0, FALSE ); // HACK HACK HACK
			ConnectionStateAdvanceToState(CONNECTION_STATE_CONNECTED);
			return;
		}
	}
	
	// start main menu
	if(AppIsHellgate())
	{
		int nSKU = SKUGetCurrent();
		int nMovieListIntro = SKUGetIntroMovieList( nSKU );
		if (nMovieListIntro == INVALID_LINK)
		{
			nMovieListIntro = GlobalIndexGet( GI_MOVIE_LIST_INTRO );
		}
		AppSwitchState( APP_STATE_PLAYMOVIELIST, nMovieListIntro );
	}
	else
	{
		AppSwitchState(APP_STATE_MAINMENU);
	}
	
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static BOOL sAppStatePlaysBackgroundMovie(
	APP_STATE eAppState)
{
	switch(eAppState)
	{
	case APP_STATE_INIT:
	case APP_STATE_PLAYMOVIELIST:
	case APP_STATE_CHARACTER_CREATE:
	case APP_STATE_IN_GAME:
	case APP_STATE_EXIT:
	case APP_STATE_FILLPAK:
	case APP_STATE_CONNECTING_PATCH:
	case APP_STATE_CONNECTING_START:
	case APP_STATE_CONNECTING:
	case APP_STATE_LOADING:
	case APP_STATE_RESTART:
		return FALSE;

	case APP_STATE_MAINMENU:
	case APP_STATE_MENUERROR:
	case APP_STATE_CREDITS:
		return TRUE;
	}

	return FALSE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static int sGetBackgroundMovieToPlay(
	void)
{
	int nMovieIndex = INVALID_LINK;
	
#if !ISVERSION(SERVER_VERSION)	
	// if the credits are playing, use that one
	nMovieIndex = c_CreditsGetCurrentMovie();
	
	// if no credits, do the menu movies
	if (nMovieIndex == INVALID_LINK)
	{
		const SKU_DATA * pSKUData = SKUGetData(SKUGetCurrent());
		if(pSKUData)
		{
			if(g_UI.m_bWidescreen)
			{
				nMovieIndex = pSKUData->nMovieTitleScreenWide;
			}
			else
			{
				nMovieIndex = pSKUData->nMovieTitleScreen;
			}
		}
		else
		{
			nMovieIndex = INVALID_ID;
		}
	}
#endif
	
	// return the movie (if any)
	return nMovieIndex;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AppStartOrStopBackgroundMovie(
	void)
{
#if !ISVERSION(SERVER_VERSION)
	APP_STATE eAppState = AppGetState();
	if(sAppStatePlaysBackgroundMovie(eAppState))
	{
		int nMoviePlaying = c_MoviePlayerGetPlaying();
		int nMovieToPlay = sGetBackgroundMovieToPlay();
		if (nMovieToPlay != INVALID_LINK && nMoviePlaying != nMovieToPlay)
		{
			c_MoviePlayerClose();
			c_MoviePlayerInit();
			c_MoviePlayerOpen(nMovieToPlay);
			UI_COMPONENT * pComponent = UIComponentGetByEnum(UICOMP_STARTGAME_SCREEN);
			if(pComponent)
			{
				UIComponentHandleUIMessage(pComponent, UIMSG_PAINT, 0, 0);
			}
		}
	}
	else
	{
		if(c_MoviePlayerGetPlaying() != INVALID_LINK)
		{
			c_MoviePlayerClose();
		}
	}
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
typedef void (*FN_STATE_DO)(unsigned int sim_frames);
typedef void (*FN_STATE_BEGIN)(APP_STATE eFromState, va_list);
typedef void (*FN_STATE_END)();

struct APP_STATE_TABLE
{
	APP_STATE			eState;
	const char *		szName;
	FN_STATE_BEGIN		fpBegin;
	FN_STATE_DO			fpDo;
	FN_STATE_END		fpEnd;
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
APP_STATE_TABLE gAppStateTbl[] =
{
	{ APP_STATE_INIT,					"init",			NULL,						AppInitDo,				NULL },
	{ APP_STATE_PLAYMOVIELIST,			"play movies",	AppPlayMovieListInit,		AppPlayMovieListDo,		AppPlayMovieListEnd },
	{ APP_STATE_MAINMENU,				"main menu",	AppMainMenuInit,			AppMainMenuDo,			AppMainMenuEnd },
	{ APP_STATE_MENUERROR,				"menu error",	AppMenuErrorInit,			AppMainMenuErrorDo,		NULL },
	{ APP_STATE_CHARACTER_CREATE,		"char create",	AppCreateInit,				AppCreateDo,			AppCreateEnd },
	{ APP_STATE_CONNECTING_START,		"connecting",	AppConnectingStartInit,		AppConnectingStartDo,	NULL },
	{ APP_STATE_CONNECTING_PATCH,		"connecting",	AppConnectingPatchInit,		AppConnectingPatchDo,	AppConnectingPatchEnd },
	{ APP_STATE_CONNECTING,				"connecting",	AppConnectingInit,			AppConnectingDo,		NULL },
	{ APP_STATE_LOADING,				"loading",		AppLoadingInit,				AppLoadingDo,			AppLoadingEnd },
	{ APP_STATE_IN_GAME,				"game",			AppInGameInit,				AppInGameDo,			AppInGameEnd },
	{ APP_STATE_RESTART,				"restart",		AppRestartInit,				AppRestartDo,			NULL },
	{ APP_STATE_EXIT,					"exit",			AppExitInit,				AppExitDo,				NULL },
	{ APP_STATE_FILLPAK,				"fillpak",		AppFillPakInit,				AppFillPakDo,			NULL },
	{ APP_STATE_DUMPSKILLDESCS,			"dump skill descs", AppDumpSkillDescsInit,	AppDumpSkillsDescsDo,	NULL },
};


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AppSwitchState(
	APP_STATE eState,
	... )
{
	SVR_VERSION_ONLY(REF(eState));
#if !ISVERSION(SERVER_VERSION)

	ASSERT_RETURN(eState >= 0 && eState < sizeof(gAppStateTbl));

	trace("AppSwitchState() -- %s\n", gAppStateTbl[eState].szName);

	e_PfProfMark(("AppSwitchState(); Enter End; %s", gAppStateTbl[gApp.m_eAppState].szName));
	if (gAppStateTbl[gApp.m_eAppState].fpEnd)
	{
		gAppStateTbl[gApp.m_eAppState].fpEnd();
	}
	e_PfProfMark(("AppSwitchState(); Leave End; %s", gAppStateTbl[gApp.m_eAppState].szName));

	APP_STATE eFromState = gApp.m_eAppState;
	gApp.m_eAppState = eState;

	AppStartOrStopBackgroundMovie();

	e_PfProfMark(("AppSwitchState(); Enter Begin; %s", gAppStateTbl[gApp.m_eAppState].szName));
	if (gAppStateTbl[gApp.m_eAppState].fpBegin)
	{
		va_list args;
		va_start( args, eState );
		gAppStateTbl[gApp.m_eAppState].fpBegin( eFromState, args );
	}
	e_PfProfMark(("AppSwitchState(); Leave Begin; %s", gAppStateTbl[gApp.m_eAppState].szName));

#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
APP_TYPE AppGetType(
	void)
{
	CLT_VERSION_ONLY( return gtAppCommon.m_eAppType );
	SVR_VERSION_ONLY( return APP_TYPE_CLOSED_SERVER );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AppSetType(
	APP_TYPE eAppType)
{
	SVR_VERSION_ONLY(REF(eAppType));
#if !ISVERSION(SERVER_VERSION)
	if (gtAppCommon.m_eAppType == eAppType)
	{
		return;
	}
	gtAppCommon.m_eAppType = eAppType;
	gApp.m_bPatching = FALSE;
	switch (eAppType)
	{
#if !ISVERSION(CLIENT_ONLY_VERSION)
	case APP_TYPE_SINGLE_PLAYER:
		gApp.m_fpGameLoop = SinglePlayer_DoInGame;
		gApp.m_fpProcessServer = ProcessServerAllowPause;
		gApp.m_fpPlayerSave = PlayerSaveToFile;
		sUpdateCensorshipFromBadges();
		break;	
#endif
#if !ISVERSION(CLIENT_ONLY_VERSION) && !ISVERSION(RETAIL_VERSION)
	case APP_TYPE_OPEN_SERVER:
		gApp.m_fpGameLoop = OpenServer_DoInGame;
		gApp.m_fpProcessServer = ProcessServer;
		gApp.m_fpPlayerSave = PlayerSaveToSendBuffer;
		break;
#endif
	case APP_TYPE_CLOSED_CLIENT:
		gApp.m_fpGameLoop = ClientOnly_DoInGame;
		gApp.m_fpProcessServer = NULL;
		gApp.m_fpPlayerSave = NULL;
		if (gApp.m_bAllowPatching) {
			gApp.m_bPatching = TRUE;
		}
		break;
#if !ISVERSION(CLIENT_ONLY_VERSION) && !ISVERSION(RETAIL_VERSION)
	case APP_TYPE_OPEN_CLIENT:
		gApp.m_fpGameLoop = ClientOnly_DoInGame;
		gApp.m_fpProcessServer = NULL;
		gApp.m_fpPlayerSave = NULL;
		break;
#endif
	case APP_TYPE_CLOSED_SERVER:
		ASSERTX(0, "Closed server now has its own loop, does not draw, and currently does not save players." );
		break;
	default:
		ASSERT(0);
		break;
	}
#endif  //!ISVERSION(SERVER_VERSION)
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AppIsDevelopmentTest(
	void)
{

	// if there is a starting level override set, we're running a test in hammer
	GAME_GLOBAL_DEFINITION *pGlobalDefinition = DefinitionGetGameGlobal();
	return pGlobalDefinition->nLevelDefinition != INVALID_LINK;
	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AppIsSaving(
	void)
{
	return !AppTestFlag(AF_NO_PLAYER_SAVING);
}

//----------------------------------------------------------------------------
// Our database is partitioned by guid number.  In order to get even
// database load on each partition, we evenly partition our guidspace
// based upon game server instance.
//
// For now, we do a simple modulo and multiply formula.
//
// We have 11 bits (0x7ff) to work with.
//----------------------------------------------------------------------------
static UINT64 sAppGetMagicPHuLookupGuidPrefixFromServerInstance(
	SVRINSTANCE serverInstance)
{
	return
		(0x7ff * (serverInstance%DATABASE_PARTITION_COUNT))/DATABASE_PARTITION_COUNT +
		(serverInstance / DATABASE_PARTITION_COUNT)+1;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
UINT64 AppGenerateGUID(
	UINT32 serverId )
{
	SVRINSTANCE serverInstance = 0;
	serverInstance = ServerIdGetInstance( serverId );

	//	TODO: needs to store server type as well...

	PGUID id;
	//RJD: switching guid to specification in technical design document from long ago, since it makes sense:
	//38 bit timestamp divided by 16 milliseconds, 14 bit increment, 11 bits gameserver instance put through magic phu lookup table, 1 bit zero.
	//OBSELETE: 9 bit GameServer Instance, 2 bits blank, one bit zero. 
	id	= ( (UINT64)InterlockedIncrement(&gtAppCommon.m_lGuidIterator) & 0x3fff )
		+ ( ((UINT64)(AppCommonGetTrueTime()) & 0x3fffffffff0) << 10 ) //Division by 16 by ignoring the 4 minor bits, and left shifting 4 bits less.
		+ ((sAppGetMagicPHuLookupGuidPrefixFromServerInstance(serverInstance) & 0x07ff ) << 52);
		//+ (((UINT64)serverInstance & 0x01ff) << 52);
	//  plus last three bits, the highest order of which is zero.  Possibly server instance expands to 11 bits; 0x07ff.
	return id;
	//Note on derivation of time:  I didn't use realtime because it's only 32 bits, which just isn't enough when dealing in milliseconds.
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
struct CLIENTSYSTEM * AppGetClientSystem(
	void)
{
	GameServerContext * context = (GameServerContext*)CurrentSvrGetContext();
	ASSERT_RETNULL(context);
	ASSERT_RETNULL(context->m_ClientSystem);
	return context->m_ClientSystem;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
#if ISVERSION(SERVER_VERSION)
TOWN_INSTANCE_LIST * AppGetTownInstanceList(
	void)
{
	GameServerContext * context = (GameServerContext*)CurrentSvrGetContext();
	ASSERT_RETNULL(context);
	ASSERT_RETNULL(context->m_pInstanceList);
	return context->m_pInstanceList;
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
GAMEID AppGetSrvGameId(
	 void)
{
	CLT_VERSION_ONLY( return gApp.m_idSelectedGame );
	SVR_VERSION_ONLY( ASSERT_RETINVALID(0); return INVALID_ID; )
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AppSetSrvGameId(
	GAMEID idGame)
{
	CLT_VERSION_ONLY( gApp.m_idSelectedGame = idGame );
	SVR_VERSION_ONLY( idGame );
	SVR_VERSION_ONLY( ASSERT(0) );
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PathSetup(
	void)
{
#if !ISVERSION(SERVER_VERSION)
	OS_PATH_CHAR path[256];
	if (!PGetModuleDirectory(path, _countof(path)))
	{
		HALTX_GS(false, GS_MSG_ERROR_APPLICATION_ROOT_DIRECTORY_TEXT, "Unable to determine root directory for application.");	// CHB 2007.08.01 - String audit: USED IN RELEASE
		return;
	}
	PSetCurrentDirectory(path);
	trace("set current directory: %s\n", OS_PATH_TO_ASCII_FOR_DEBUG_TRACE_ONLY(path));
	PStrCopy(gtAppCommon.m_szRootDirectory, path, _countof(gtAppCommon.m_szRootDirectory));
	gtAppCommon.m_nRootDirectoryStrLen = PStrLen(gtAppCommon.m_szRootDirectory, _countof(gtAppCommon.m_szRootDirectory));
#else
    ASSERT(FALSE);
#endif
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static HANDLE sCreateMutex(const char * pName, const char * pSuffix = 0)
{
	std::string Name = std::string("Global\\") + std::string(pName);
	if (pSuffix != 0)
	{
		Name += std::string(pSuffix);
	}
	return ::CreateMutexA(0, true, Name.c_str());
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const TCHAR * sGetWindowClassName(void)
{
	return sgszAppName;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PrimeBringForwardExistingInstance(void)
{
	// Find a window belonging to our class.
	HWND hWnd = ::FindWindow(sGetWindowClassName(), 0);
	if (hWnd != 0)
	{
		::SetForegroundWindow(hWnd);
		if (::IsIconic(hWnd))
		{
			::ShowWindow(hWnd, SW_RESTORE);
		}
	}
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
PRIME_RUNNING PrimeIsRunning(
	void)
{
	const unsigned WAIT_FOR_OTHER_APP_TO_CLOSE = 10;				// wait 5 seconds for other instance of app to close

	HANDLE hMutex;
	DWORD nError;
	unsigned ii = 0;

	do
	{
		// The return value is 0 and the error is ERROR_ACCESS_DENIED if the mutex was created in a different session.
		hMutex = sCreateMutex(sgszAppMutexName);
		nError = ::GetLastError();
		if (hMutex != 0)
		{
			if (nError == ERROR_SUCCESS)
			{
				return PRIME_RUNNING_FIRST;
			}
			::CloseHandle(hMutex);
		}
		::Sleep(1000);
	}
	while (++ii < WAIT_FOR_OTHER_APP_TO_CLOSE);

	// CHB 2007.10.02 - This is the single specific case known to
	// indicate that the mutex was created in a different session.
	// PRIME_RUNNING_OTHER_SESSION should not be the catch-all
	// return value.
	if ((hMutex == 0) && (nError == ERROR_ACCESS_DENIED))
	{
		return PRIME_RUNNING_OTHER_SESSION;
	}

	// CHB 2007.10.02 - In this case, hMutex is generally non-null
	// and nError is ERROR_ALREADY_EXISTS. Note that closing the
	// handle is already handled above, although it doesn't matter
	// too much since the app exits immediately after bringing the
	// existing window forward.
	return PRIME_RUNNING_THIS_SESSION;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PrimeIsRemote(
	void)
{
	return ( GetSystemMetrics( SM_REMOTESESSION ) != 0 );
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PrimeVerifyAccess(
	const TCHAR* gdfexe )
{
	HRESULT hr = CoInitialize( 0 );
	ASSERT( SUCCEEDED(hr) );

	BOOL result = TRUE;
#if !ISVERSION(SERVER_VERSION)
	IGameExplorer* pGE = 0;
	hr = CoCreateInstance( __uuidof(GameExplorer), NULL, CLSCTX_INPROC_SERVER,
						   __uuidof(IGameExplorer), (void**) &pGE );
	if( SUCCEEDED(hr) && pGE != 0 )
	{
		TCHAR path[ MAX_PATH ];

		// Assumes our gdfexe is in the same directory as our running application
		size_t len = GetModuleFileName(NULL, path, MAX_PATH);
		len = PStrGetPath(path, MAX_PATH, path );
		PStrForceBackslash( path, MAX_PATH );
		PStrCat( path, gdfexe, MAX_PATH );

		// This silliness is needed to get the full path into an OLE compatible form for the COM call
		WCHAR wpath[ MAX_PATH ];
		PStrCvt( wpath, path, MAX_PATH );

		BSTR gdf = SysAllocString( wpath );

		hr = pGE->VerifyAccess( gdf, &result );
		if ( FAILED(hr) )
			result = FALSE;

		SysFreeString( gdf );

		pGE->Release();
	}
#endif
	CoUninitialize();

	return result;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PrimeGameExplorerAdd(
	BOOL bDisplayInfo)
{
	//#define HELLGATE_BINARY L"C:\\Prime\\bin\\Hellgate.exe"
	//#define HELLGATE_GDF_BINARY L"C:\\Prime\\bin\\HellgateGDFBinary.dll"
	//#define HELLGATE_DIRECTORY L"C:\\Prime\\"

	// setup install path
	WCHAR uszInstallPath[ MAX_PATH ];
	PStrCvt( uszInstallPath, gtAppCommon.m_szRootDirectory, _countof(uszInstallPath) );
	PStrForceBackslash( uszInstallPath, _countof(uszInstallPath) );

	// setup binary with GDC information
	WCHAR uszGDFBinaryPath[ MAX_PATH ];
	PStrPrintf( uszGDFBinaryPath, MAX_PATH, L"%sbin\\%sGDFBinary.dll", uszInstallPath, sgszGenericAppName);

	// setup exe path
	WCHAR uszGameExePath[ MAX_PATH ];
	PStrPrintf( uszGameExePath, MAX_PATH, L"%sbin\\%s.exe", uszInstallPath, sgszGenericAppName);
	
	// setup error buffer
	GAME_EXPLORER_ERROR tError;
	BOOL bSuccess = GameExplorerAdd( 
		uszGameExePath,
		uszGDFBinaryPath,
		uszInstallPath,
		tError );
		
	if (bDisplayInfo)
	{
		ConsoleString( CONSOLE_SYSTEM_COLOR, "Games Explorer Add: '%s'", bSuccess ? "OK" : "FAILED" );
		if (bSuccess == FALSE)
		{
			//MessageBoxW( NULL, tError.uszError, NULL, MB_OK );	// CHB 2007.08.01 - String audit: commented
			ConsoleString( CONSOLE_SYSTEM_COLOR, "Error: '%s'", tError.uszError );		
		}
	}

	//if (bSuccess)
	//{
	//	MessageBoxW( NULL, L"Game Registered", L"Registration OK", MB_OK );	// CHB 2007.08.01 - String audit: commented
	//	BOOL bSuccess = GameExplorerRemove( tError );		
	//	if (bSuccess == TRUE)
	//	{
	//		MessageBoxW( NULL, L"Game Un-registered", L"Unregistration OK", MB_OK );	// CHB 2007.08.01 - String audit: commented
	//	}
	//	else
	//	{
	//		MessageBoxW( NULL, tError.uszError, L"Unregistration FAILED", MB_OK );	// CHB 2007.08.01 - String audit: commented
	//	}
	//}
	//else
	//{
	//	MessageBoxW( NULL, tError.uszError, L"Registration Failed", MB_OK );	// CHB 2007.08.01 - String audit: commented
	//}	
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PrimeRun(
	void)
{
	BOOL bNoQuit = TRUE;
	while (bNoQuit)
	{
		MSG tMsg;
		HWND hWnd = NULL;
		if (PeekMessage(&tMsg, hWnd, 0, 0, PM_NOREMOVE))
		{
			bNoQuit = GetMessage(&tMsg, NULL, 0, 0);
			TranslateMessage(&tMsg);
			DispatchMessage(&tMsg);
		} 
		else 
		{
			PERF_END(APP);
			PERF_START(APP);
			PERF_INC();
			HITCOUNT_INC();

			unsigned int sim_frames = CommonTimerSimulationUpdate(AppCommonGetTimer(), true);

			// Update Xfire Voice Chat
			if (!AppIsHammer() && sim_frames > 0)
			{
				CLT_VERSION_ONLY(c_VoiceChatUpdate());
			}


			gAppStateTbl[AppGetState()].fpDo(sim_frames);

			// Inserted to help Windows and the client play better.  If this causes trouble in the server, feel free to mark it client-only.
			Sleep(0);
		}
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

LONG iIgnoreEscapeKeyUpMsg = 0;  //Used for temp hack to ignore escape key-up messages

static
LRESULT CALLBACK sPrimeWndProc(
	HWND hWnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam)
{
#if !ISVERSION(SERVER_VERSION)
	// TRAVIS: Temporarily removing this until we can resolve why Mythos loading screen goes bonkers
	// as a result - missed messages for loadingscreendraw that were needed?
	//if (!CrashHandlerActive())
	{

		switch (uMsg)
		{
			case WM_ACTIVATE:										// indicates this app was (de)activated
			{
				if (!CrashHandlerActive())
				{

					BOOL bActive = ( LOWORD(wParam) != WA_INACTIVE );

#if defined(_DEBUG) && defined(SHOW_WM_DEBUG_TRACES)
					trace( "WM_ACTIVATE: active=%s\n", bActive ? "yes" : "no" );
#endif // _DEBUG && SHOW_WM_DEBUG_TRACES
					AppSetActive(bActive);

					// CHB 2007.10.01 - Repaint the background when
					// the activation state changes.
					::RedrawWindow(hWnd, 0, 0, RDW_ERASE | RDW_INVALIDATE);

					if ( ! e_OptionState_IsInitialized() )
						return 0;
					if( !bPrimeInitialized )
					{
						return 0;
					}
					// Mythos doesn't ever really pause
					//if(!bActive && AppIsHellgate() )
					//{
					//	c_SoundPauseAll(TRUE);
					//}

					if (UIShowingLoadingScreen() && !AppGetCltGame())
					{
						AppLoadingScreenUpdate();
					}

					c_MoviePlayerSetPaused(!bActive);

					if ( ! AppIsMenuLoop() && ! AppIsMoveWindow() )
					{
						c_InputMouseAcquire(AppGetCltGame(), bActive);
					}

					if (!AppGetCltGame())
					{
						// CML 2007.10.11 - Using the same logic as CHB below, breaking instead of returning 0.
						break;
					}

					static BOOL bLastPaused = FALSE;
					if (!bActive)	// deactivating
					{
						bLastPaused = AppCommonIsPaused();
						AppPause(TRUE);
					}
					else
					{
						AppPause(bLastPaused);
					}
				}
				if( AppIsTugboat() )
				{
					UIUpdateHardwareCursor();
				}

				// CHB 2007.08.27 - The docs say, "If an application
				// processes this message, it should return zero."
				// Well, we don't really *process* this message, we
				// only listen in on it. That is, we don't actually
				// do anything with the window state. We need to call
				// the default window procedure because that handles
				// some subtle behind-the-scenes stuff for us. Oh,
				// and it also returns 0.
				break;
				//return 0;
			}

			case WM_SETFOCUS:
				if( AppIsTugboat() )
				{
					UIUpdateHardwareCursor();
					return 0;
				}
				break;

			case WM_KILLFOCUS:
				WM_DEBUG_TRACE( "WM_KILLFOCUS\n" );
				c_InputMouseAcquire(AppGetCltGame(), FALSE);
				break;

			case WM_SETCURSOR:
				if( AppIsHellgate() )
					return 1;
				UIUpdateHardwareCursor();
				return 0;
				break;
			//case WM_CLOSE:											// indicates a proper close instruction was sent
			//	WM_DEBUG_TRACE( "WM_CLOSE\n" );
			//	break;	// run default wndproc

			//case WM_COMPACTING:										// indicates a low system-memory condition
			//	WM_DEBUG_TRACE( "WM_COMPACTING\n" );
			//	break;	// run default wndproc

			case WM_CONTEXTMENU:									// indicates right mouse click in window
				WM_DEBUG_TRACE( "WM_CONTEXTMENU\n" );
				return 0;
				//break;	// run default wndproc

			case WM_CREATE:											// indicates window is being created
				WM_DEBUG_TRACE( "WM_CREATE\n" );
				InvalidateRect( hWnd, NULL, FALSE );
				GDI_ShowLogo( hWnd );
				break;	// run default wndproc

			case WM_DISPLAYCHANGE:									// indicates desktop resolution changed
				WM_DEBUG_TRACE( "WM_DISPLAYCHANGE\n" );
				// CHB 2007.07.26 - Tell the engine about the display change.
				// We may also want to pay attention to the window size &
				// position messages.
				InvalidateRect( hWnd, NULL, FALSE );
				GDI_ShowLogo( hWnd );
				break;	// run default wndproc

			case WM_ENTERMENULOOP:									// indicates entering window menu loop
				WM_DEBUG_TRACE( "WM_ENTERMENULOOP\n" );
				c_InputMouseAcquire(AppGetCltGame(), FALSE);
				::ShowCursor( TRUE );
				AppSetMenuLoop(TRUE);
				break;	// run default wndproc

			case WM_ENTERSIZEMOVE:									// indicates entering window size/move loop
				WM_DEBUG_TRACE( "WM_ENTERSIZEMOVE\n" );
				c_InputMouseAcquire(AppGetCltGame(), FALSE);
				AppSetMoveWindow(TRUE);
				break;	// run default wndproc

			case WM_EXITMENULOOP:									// indicates exiting window menu loop
				WM_DEBUG_TRACE( "WM_EXITMENULOOP\n" );
				::ShowCursor( FALSE );
				c_InputMouseAcquire(AppGetCltGame(), TRUE);
				AppSetMenuLoop(FALSE);
				break;	// run default wndproc

			case WM_EXITSIZEMOVE:									// indicates exiting window size/move loop
				WM_DEBUG_TRACE( "WM_EXITSIZEMOVE\n" );
				AppSetMoveWindow(FALSE);
				c_InputMouseAcquire(AppGetCltGame(), TRUE);
				break;	// run default wndproc

			//case WM_GETMINMAXINFO:									// indicates size/position of window is about to change
			//	//WM_DEBUG_TRACE( "WM_GETMINMAXINFO\n" );
			//	break;

			case WM_NCRBUTTONDOWN:									// indicates right button down on non-client window area
				WM_DEBUG_TRACE( "WM_NCRBUTTONDOWN\n" );
				return 0;

			case WM_ERASEBKGND:										// indicates window background needs erasing
				// CHB 2007.10.01
				//if ( !e_GetScreenshotMode() )
				if (0)
				{
					HDC hDC = reinterpret_cast<HDC>(wParam);
					GDI_ShowLogo( hWnd, hDC );
				}
				return TRUE;

			// CHB 2007.10.01 - Painting logic removed in favor
			// of background erasing.
#if 0
			case WM_PAINT:											// indicates need to redraw window portions
				RECT tRect;
				BOOL bUpdate;
				bUpdate = GetUpdateRect( hWnd, &tRect, FALSE );
	#if defined(_DEBUG) && defined(SHOW_WM_DEBUG_TRACES)
				trace( "WM_PAINT: update:%d l[%d] t[%d] r[%d] b[%d]\n", bUpdate, tRect.left, tRect.top, tRect.right, tRect.bottom );
	#endif // _DEBUG && SHOW_WM_DEBUG_TRACES
				//InvalidateRect( hWnd, NULL, FALSE );
				//if ( e_GetScreenshotMode() )
				//	bUpdate = FALSE;
				if ( bUpdate )
				{
					PAINTSTRUCT tPaint;
					BeginPaint( hWnd, &tPaint );
					GDI_ShowLogo( hWnd );
					EndPaint( hWnd, &tPaint );
					return 0;
				}
				break;	// run default wndproc
#endif

			//case WM_POWERBROADCAST:									// indicates a power-management event
			//	WM_DEBUG_TRACE( "WM_POWERBROADCAST\n" );
			//	break;	// run default wndproc

			case WM_SHOWWINDOW:										// indicates window is about to be hidden/shown
	#if defined(_DEBUG) && defined(SHOW_WM_DEBUG_TRACES)
				trace( "WM_SHOWWINDOW: %s\n", wParam ? "show" : "hide" );
	#endif // _DEBUG && SHOW_WM_DEBUG_TRACES
				if( AppIsTugboat() )
				{
					UIUpdateHardwareCursor();
				}
				break;	// run default wndproc

			case WM_SIZE:											// indicates window's size has changed
				//WM_DEBUG_TRACE( "WM_SIZE\n" );
				//trace( "WM_SIZE: " );
				switch (wParam)
				{
				case SIZE_MAXHIDE:
				case SIZE_MINIMIZED:
					AppSetMinimized( TRUE );
					c_SoundPauseAll(TRUE);
					break;

				case SIZE_MAXIMIZED:
				case SIZE_MAXSHOW:
				case SIZE_RESTORED:
					AppSetMinimized( FALSE );
					if( AppIsTugboat() )
					{
						UIUpdateHardwareCursor();
					}
					//c_SoundPauseAll(FALSE);
					break;
				}
				break;	// run default wndproc

			case WM_SIZING:											// indicates window is resizing
				//WM_DEBUG_TRACE( "WM_SIZING\n" );
				// enforce the same aspect ratio/size
				RECT tCltRect;
				GetWindowRect( hWnd, &tCltRect );
				int nCW, nCH;
				nCW = tCltRect.right  - tCltRect.left;
				nCH = tCltRect.bottom - tCltRect.top;
				// if this is the first sizing...
				if ( nCW <= 0 || nCH <= 0 )
					break;
				// otherwise, force the same size/aspect ratio
				RECT * ptSizeRect;
				ptSizeRect = (RECT*) lParam;
				*ptSizeRect = tCltRect;
				if( AppIsTugboat() )
				{
					UIUpdateHardwareCursor();
				}
				return TRUE;

			case WM_WINDOWPOSCHANGING:
				WINDOWPOS * pWP;
				pWP = (WINDOWPOS*) lParam;
				//trace( "### WINDOWPOSCHANGING: x %d, y %d, w %d, h %d\n", pWP->x, pWP->y, pWP->cx, pWP->cy );
				break;

			//case WM_SYSCOMMAND:										// indicates a windows system command, such as screensaver
			//	WM_DEBUG_TRACE( "WM_SYSCOMMAND\n" );
			//	break;	// run default wndproc

			case WM_KEYDOWN:
			{
				WCHAR pwszBuff[20];
				BYTE lpKeyState[256];

				static int sDeadKeyW = -1;
				static int sDeadKeyL = -1;
				static BYTE sDeadKeyKeyState[256];

				int n;

				InputFlushKeyboardBuffer();

				// re-process any pending dead keys (this looks weird but ToUnicode is internal-state dependent)
				//
				if (sDeadKeyW != -1 && wParam != VK_SHIFT && wParam != VK_CONTROL && wParam != VK_MENU)
				{
					n = ToUnicode(      
						(UINT)sDeadKeyW, //UINT wVirtKey,
						(UINT)sDeadKeyL, //UINT wScanCode,
						(const BYTE *)sDeadKeyKeyState,
						pwszBuff,
						sizeof(pwszBuff)/sizeof(WCHAR),
						0);

					ASSERT_RETZERO(n == -1);

					sDeadKeyW = -1;
					sDeadKeyL = -1;
				}

				// finally, convert the new key and process it
				//
				GetKeyboardState(lpKeyState);

				n = ToUnicode(      
					(UINT)wParam, //UINT wVirtKey,
					(UINT)lParam, //UINT wScanCode,
					(const BYTE *)lpKeyState,
					pwszBuff,
					sizeof(pwszBuff)/sizeof(WCHAR),
					0);

				if (n > 0)
				{
					for (int i=0; i<n; ++i)
					{
						c_InputHandleKeys( AppGetCltGame(), WM_CHAR, pwszBuff[i], lParam );
					}
				}
				else if (n < 0)
				{
					// user pressed a dead key... save it for later
					//
					sDeadKeyW = (int)wParam;
					sDeadKeyL = (int)lParam;
					memcpy(sDeadKeyKeyState, lpKeyState, sizeof(lpKeyState));
				}

				// Fall through to case below...
			}

			case WM_KEYUP:
			case WM_SYSKEYDOWN:
			case WM_SYSKEYUP:
			case WM_HOTKEY:
				if (lParam == 0) {
					return 0;
				}
				// Fall through to case below...

//			case WM_CHAR:
			case WM_UNICHAR:
	//			trace("key msg 0x%x key 0x%x lParam 0x%x\n", uMsg, wParam, lParam);
				if (uMsg == WM_UNICHAR)
				{
					uMsg = WM_CHAR;
				}
				if (uMsg == WM_KEYDOWN && wParam == VK_PROCESSKEY) {
	//				wParam = IMM_GetVirtualKey();
					wParam = MapVirtualKey(LOBYTE(HIWORD(lParam)), 1); // MAPVK_VSC_TO_VK = 1
					return 0;
				}
				if (uMsg == WM_KEYUP && wParam == VK_ESCAPE) {
					if (InterlockedExchange(&iIgnoreEscapeKeyUpMsg, 0) != 0) {
						return 0;
					}
				}
				if (!AppGetCltGame())
				{
					if (AppGetState() == APP_STATE_MAINMENU ||
						AppGetState() == APP_STATE_PLAYMOVIELIST)
					{
						return c_InputHandleKeys( NULL, uMsg, wParam, lParam );
					}
					return 0;
				}
				return c_InputHandleKeys(AppGetCltGame(), uMsg, wParam, lParam);

			case WM_XBUTTONDOWN:
			case WM_XBUTTONUP:
			case WM_NCXBUTTONDBLCLK:
				return c_InputHandleMouseButton(AppGetCltGame(), uMsg, wParam, lParam);

	//		case WM_MOUSEMOVE:
			case WM_INPUT:
				{
					if (!CrashHandlerActive())
						return c_InputHandleMouseMove(uMsg, wParam, lParam);
				}
				break;

			case WM_NCMOUSEMOVE:
				{
					if (!CrashHandlerActive())
						return c_InputHandleNCMouseMove(uMsg, wParam, lParam);
				}
				break;

			case WM_DESTROY :
				WM_DEBUG_TRACE( "WM_DESTROY\n" );
				PostQuitMessage(0);
				return 0;

			case WM_INPUTLANGCHANGE:
	//			trace("Input Language Change\n");
				IMM_UpdateLanguageChange();
				UISendMessage(WM_INPUTLANGCHANGE, 0, 0);
				return 1;

			// prevent windows to pop up the composition window
			case WM_IME_SETCONTEXT:
				lParam &= ~ISC_SHOWUIALL;
				break;

			case WM_IME_STARTCOMPOSITION:
	//			trace("IME Start Composition\n");
				IMM_StartComposition();
				return 0;

			case WM_IME_COMPOSITION:
	//			trace("IME Composition\n");
				IMM_ProcessComposition(lParam);
				return 0;

			case WM_IME_ENDCOMPOSITION:
	//			trace("IME End Composition\n");
				IMM_EndComposition();
				return 0;

			case WM_IME_NOTIFY:
	//			trace("IME Notify\n");
				{
					BOOL bTrapped = FALSE;
					ASSERT(IMM_HandleNotify(wParam, lParam, &bTrapped));
					if (bTrapped) {
						return 0;
					}
				}
				break;

			case WM_QUERYENDSESSION:
				WM_DEBUG_TRACE( "WM_QUERYENDSESSION\n" );
				if ( lParam == 0x1 /*ENDSESSION_CLOSEAPP*/ )
				{
					// Indicate we are Windows Vista Restart Manager aware
					return 1;
				}
				break;

			case WM_ENDSESSION:
				WM_DEBUG_TRACE( "WM_ENDSESSION\n" );
				if ( lParam == 0x1 /*ENDSESSION_CLOSEAPP*/ )
				{
					if ( wParam )
					{
						// Windows Vista Restart Manager wants us to shutdown
						// Since we don't support restart, we just say 'go ahead'
						// TODO - maybe want to force save of the character and/or tell the system we are logging out
						//		  before returning...
						return 1;
					}
				}
				break;

			case WM_NCCALCSIZE:
				// For our splash window we intercept this message to keep the
				// title bar and caption from showing up. 
				
				// AE 2007.09.09:
				// Currently, it can be determined that the splash window is 
				// displaying by checking whether the OptionState system has
				// been initialized.
				if ( ! e_OptionState_IsInitialized() )
					return 0;
				break;

		}
	}
#endif //!SERVER_VERSION

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

// CHB 2007.08.27
LRESULT CALLBACK c_PrimeWndProc(
	HWND hWnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam)
{
	e_PreNotifyWindowMessage(hWnd, uMsg, wParam, lParam);
#if defined(_DEBUG) && defined(SHOW_WM_DEBUG_TRACES)
	static int nLevel = 0;
	char szLevel[64];
	ASSERT(nLevel < _countof(szLevel));
	memset(szLevel, '>', nLevel);
	szLevel[nLevel] = '\0';
	++nLevel;	// this should be okay since the procedure should be called from a single thread
	const char * pMessageName = WindowsDebugGetMessageName(uMsg);
	trace("WM  IN: %s%s (%u) - %u / %u [%u, %u]\n", szLevel, pMessageName, uMsg, wParam, lParam, LOWORD(lParam), HIWORD(lParam));
#endif
	LRESULT nResult = sPrimeWndProc(hWnd, uMsg, wParam, lParam);
#if defined(_DEBUG) && defined(SHOW_WM_DEBUG_TRACES)
	trace("WM OUT: %s%s (%u) = %u\n", szLevel, pMessageName, uMsg, nResult);
	ASSERT(nLevel > 0);
	--nLevel;
#endif
	e_NotifyWindowMessage(hWnd, uMsg, wParam, lParam);
	return nResult;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
LRESULT CALLBACK s_PrimeWndProc(
	HWND hWnd,
	UINT uMsg,
	WPARAM wParam,
	LPARAM lParam)
{
	switch (uMsg) 
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL CreatePrimeWindow(
	HINSTANCE hInstance,
	int nShowCmd)
{
#if !ISVERSION(SERVER_VERSION)
	TIMER_START("CreatePrimeWindow()")

	WNDCLASSEX sgWndClass;
	sgWndClass.cbSize = sizeof(WNDCLASSEX);
	sgWndClass.style = CS_HREDRAW | CS_VREDRAW;
	if (AppGetType() == APP_TYPE_CLOSED_SERVER)
	{
		sgWndClass.lpfnWndProc = s_PrimeWndProc;
	}
	else
	{
		sgWndClass.lpfnWndProc = c_PrimeWndProc;
	}
	sgWndClass.cbClsExtra = 0;
	sgWndClass.cbWndExtra = 0;
	sgWndClass.hInstance = AppGetHInstance();
	sgWndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	sgWndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	sgWndClass.lpszMenuName = NULL;
	sgWndClass.lpszClassName = sGetWindowClassName();

#ifdef HELLGATE
	sgWndClass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_HGLICON_32X32));
	sgWndClass.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_HGLICON_16X16));
#elif defined(TUGBOAT)
	sgWndClass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICONLARGE));
	sgWndClass.hIconSm = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICONSMALL));
#else
	sgWndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	sgWndClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
#endif

	RegisterClassEx(&sgWndClass);

	int nWinWidth = 640;
	int nWinHeight = 480;
	
	int nX, nY;
	int nDW, nDH;
	e_SetDesktopSize(); // Initialize this first.
	e_GetDesktopSize( nDW, nDH );
	nX = ( nDW - nWinWidth )  >> 1;
	nY = ( nDH - nWinHeight ) >> 1;

	DWORD dwStyle = WS_OVERLAPPED;
	DWORD dwStyleEx = WS_EX_TOOLWINDOW;

	gtAppCommon.m_tMainWindow.m_hWnd = CreateWindowEx(
		dwStyleEx,				// dwExStyle
		sgszAppName,			// class name
		sgszAppName,			// window name
		dwStyle,				// styles
		nX,						// X position
		nY,						// Y position
		nWinWidth,				// width
		nWinHeight,				// height
		NULL,					// parent
		NULL,					// menu
		hInstance,				// instance
		NULL);					// lParam

	if (!AppCommonGetHWnd())
	{
		// DWORD dwError = GetLastError();
		return FALSE;
	}

	if (AppGetType() != APP_TYPE_CLOSED_SERVER)
	{
		GDI_Init( AppCommonGetHWnd(), hInstance );
	}

	InvalidateRect( AppCommonGetHWnd(), NULL, FALSE );
	ShowWindow(AppCommonGetHWnd(), nShowCmd);
	UpdateWindow(AppCommonGetHWnd());
	GDI_ShowLogo(AppCommonGetHWnd(), LOGO_SPLASH);


#endif  //!ISVERSION(SERVER_VERSION)
	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL UpdatePrimeWindow(
	CComPtr<OptionState> & pOptionState_Update )
{
#if !ISVERSION(SERVER_VERSION)
	TIMER_START("UpdatePrimeWindow()")

	if ( ! pOptionState_Update )
		return TRUE;

	// Refresh fullscreen mode
	CComPtr<OptionState> pState;
	V_DO_FAILED( e_OptionState_CloneActive( &pState ) )
	{
		return FALSE;
	}
	pState->SuspendUpdate();
	pState->set_bWindowed( pOptionState_Update->get_bWindowed() );
	pState->ResumeUpdate();
	V_DO_FAILED(e_OptionState_CommitToActive(pState))
	{
		return FALSE;
	}

#endif  //!ISVERSION(SERVER_VERSION)

	return TRUE;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

BOOL InitPrimeWindow(
	HINSTANCE hInstance,
	int nShowCmd,
	CComPtr<OptionState> & pOptionState_Update )
{
#if !ISVERSION(SERVER_VERSION)
	TIMER_START("InitPrimeWindow()")

	if (!AppCommonGetHWnd())
	{
		// DWORD dwError = GetLastError();
		return FALSE;
	}

	E_WINDOW_STYLE ews = e_GetWindowStyle();

	SetWindowLongPtr( AppCommonGetHWnd(), GWL_STYLE, ews.nStyleOut );
	SetWindowLongPtr( AppCommonGetHWnd(), GWL_EXSTYLE, ews.nExStyleOut );

	int nWidth, nHeight;
	if (AppTestDebugFlag(ADF_FILL_PAK_CLIENT_BIT)) {
		nWidth = 640;
		nHeight = 480;
	} else {
		e_GetWindowSize( nWidth, nHeight );
	}

	CComPtr<OptionState> pTemporaryState;

	// CML: Disabling this until further debugging can make it work.
	//if ( e_IsFullscreen() )
	//{
	//	// If we are destined for fullscreen, we want to delay the actual fullscreen set until after the full device initialization/effects loads.
	//	// For now, set to windowed (same res) and create the device that way.
	//	ONCE
	//	{
	//		ASSERT_BREAK( ! pOptionState_Update );

	//		// Save current optionstate.
	//		V_BREAK( e_OptionState_CloneActive( &pOptionState_Update ) );

	//		V_BREAK( e_OptionState_CloneActive( &pTemporaryState ) );
	//		pTemporaryState->set_bWindowed(TRUE);

	//		// Set temporary optionstate.
	//		//CComPtr<OptionState> pState;
	//		//V_BREAK( e_OptionState_CloneActive( &pState ) );
	//		//pState->SuspendUpdate();
	//		//pState->set_bWindowed(TRUE);
	//		//pState->ResumeUpdate();
	//		//V(e_OptionState_CommitToActive(pState));
	//	}
	//} else
	{
		// No updating, we're staying windowed!
		pOptionState_Update = NULL;
	}

	// CHB 2007.08.22
	int nWinWidth = nWidth;
	int nWinHeight = nHeight;
	e_FrameBufferDimensionsToWindowSize(nWinWidth, nWinHeight);
	int nX, nY;
	e_CenterWindow(nWinWidth, nWinHeight, nX, nY);

	// get screen position of window (if any)
	//const CONFIG_DEFINITION *pConfigDef = DefinitionGetConfig();
	ENGINE_OVERRIDES_DEFINITION * pOverrides = e_GetEngineOverridesDefinition();
	if (pOverrides && pOverrides->nWindowedPositionX != -1) 
	{
		nX = pOverrides->nWindowedPositionX;
	}
	if (pOverrides && pOverrides->nWindowedPositionY != -1)
	{
		nY = pOverrides->nWindowedPositionY;
	}

	SetWindowPos( AppCommonGetHWnd(), HWND_NOTOPMOST, 
		nX, nY, nWinWidth, nWinHeight, SWP_SHOWWINDOW );

	if (AppGetType() == APP_TYPE_CLOSED_SERVER)
	{		
		if (!gdi_init(AppCommonGetHWnd(), nWidth, nHeight))
		{
			return FALSE;
		}
	}
	else
	{
		if (!InitGraphics(AppCommonGetHWnd(), NULL, NULL, NULL, pTemporaryState))
		{
			return FALSE;
		}
	}
#endif  //!ISVERSION(SERVER_VERSION)

	ASSERT( RegisterHotKey( AppCommonGetHWnd(), HGVK_SCREENSHOT_UI,    0,           VK_SNAPSHOT ) );
	ASSERT( RegisterHotKey( AppCommonGetHWnd(), HGVK_SCREENSHOT_NO_UI, MOD_CONTROL, VK_SNAPSHOT ) );

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void ClosePrimeWindow(
	void)
{
	UnregisterHotKey( AppCommonGetHWnd(), HGVK_SCREENSHOT_UI );
	UnregisterHotKey( AppCommonGetHWnd(), HGVK_SCREENSHOT_NO_UI );


	if (AppGetType() == APP_TYPE_CLOSED_SERVER)
	{
		gdi_free();
	}
#if !ISVERSION(SERVER_VERSION)
	else
	{
		DestroyGraphics();
	}
	if (AppGetType() != APP_TYPE_CLOSED_SERVER)
	{
		GDI_Destroy();
	}
#endif// !ISVERSION(SERVER_VERSION)

	DestroyWindow(AppCommonGetHWnd());
	gtAppCommon.m_tMainWindow.m_hWnd = NULL;

    UnregisterClass(sgszAppName, AppGetHInstance());
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL ShouldSoundBePaused()
{
#if !ISVERSION(SERVER_VERSION)
	if(AppIsHammer())
	{
		return FALSE;
	}

	if( AppIsHellgate() )
	{
		if(AppGetState() == APP_STATE_IN_GAME)
		{
			if(UIGetCurrentMenu())
			{
				return TRUE;
			}
			if(UIComponentGetActiveByEnum(UICOMP_OPTIONSDIALOG))
			{
				return TRUE;
			}
		}
		else if(AppGetState() == APP_STATE_PLAYMOVIELIST)
		{
			// Movie audio goes through its own thing
			return TRUE;
		}
	}
#endif

	if ( ! e_ShouldRender() )
	{
		return TRUE;
	}
	return FALSE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL PrimeInit(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR lpCmdLine,
	int nShowCmd)
{
	REF(hPrevInstance);
	TIMER_START("PrimeInit()")

	if ( AppCommonIsFillpakInConvertMode() )
		nShowCmd = SW_MINIMIZE;

	if (!CreatePrimeWindow( hInstance, nShowCmd ))
	{
		return false;
	}

	// CHB 2006.09.05 - Ignoring the result would lead to an
	// access violation.
	if (!AppInit(hInstance, lpCmdLine))
	{
		return FALSE;
	}

	CComPtr<OptionState> pOptionState_Update;
	if (!InitPrimeWindow( hInstance, nShowCmd, pOptionState_Update ))
	{
		return FALSE;
	}

	if (AppGetType() == APP_TYPE_CLOSED_SERVER)
	{
		AppSetActive(TRUE);
		return TRUE;
	}

#if !ISVERSION(SERVER_VERSION)


	if (!c_InputInit(AppCommonGetHWnd(), AppGetHInstance()))
	{
		return FALSE;
	}

	PlayerDeleteOldVersions();

	c_SoundInitCallbacks(
		RoomGetMaterialInfoByTriangleNumber,
		ClientControlUnitLineCollideAll,
		ClientControlUnitLineCollideLen,
		ClientControlUnitGetRoom,
		ClientControlUnitVMExecI,
		AppGetSystemRAM,
		AppGetNoSound,
		IsControlUnitDeadOrDying,
		ClientControlUnitGetPosition,
		LanguageCurrentGetAudioLanguage,
		LanguageGetName,
		SKUContainsAudioLanguage,
		ShouldSoundBePaused,
		c_CameraGetViewType,
		VIEW_1STPERSON,
		VIEW_3RDPERSON);
	c_SoundInit(AppCommonGetHWnd());

	c_MoviePlayerAppInit();

	CSchedulerInit();
	if (!AppTestDebugFlag(ADF_FILL_PAK_CLIENT_BIT) &&
		!AppTestDebugFlag(ADF_FILL_LOCALIZED_PAK_BIT)) {
		// if running fillpak client, delay UI-loading till later
		UIInit();
	}
	ConsoleInitInterface();

	IMM_Initialize(NULL);
//	IMM_Enable(FALSE);

#ifdef DEBUG_WINDOW
	DaveDebugWindowInit();
#endif

	if (!UpdatePrimeWindow( pOptionState_Update ) )
	{
		return FALSE;
	}

#endif //!SERVER_VERSION
	bPrimeInitialized = TRUE;

	return TRUE;
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void PrimeClose(
	void)
{
#if !ISVERSION(SERVER_VERSION)
	IMM_Shutdown();

    ConnectionStateFree();
	
	UIFree();

	CSchedulerFree();

	if (AppGetCltGame())
	{
		GameFree(AppGetCltGame());
		AppSetCltGame( NULL );
	}

	// swapped this and AppClose -- doing d3d_cleanup before definitions, etc. get nuked
	ClosePrimeWindow();
	ConsoleFree();

#endif //!SERVER_VERSION

	AppClose();

#ifdef DEBUG_WINDOW
	DaveDebugWindowClose();
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
DWORD AppGetSystemRAM()
{
	return e_CapGetValue(CAP_SYSTEM_MEMORY_MB);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AppGetNoSound()
{
	CLT_VERSION_ONLY(return gApp.m_bNoSound);
	SVR_VERSION_ONLY(return TRUE);
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void AppSetCensorFlag(
	APP_FLAG eFlag,
	BOOL bValue,
	BOOL bForce )
{
	if ( !bForce && AppTestFlag( AF_CENSOR_LOCKED ) )
		return;
	AppSetFlag( eFlag, bValue );
	if ( eFlag == AF_CENSOR_PARTICLES )
	{
		c_ParticleSetGlobalBit( PARTICLE_GLOBAL_BIT_CENSORSHIP_ENABLED, bValue );
	}
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL AppIsFillingPak()
{
	CLT_VERSION_ONLY(return AppTestDebugFlag( ADF_FILL_PAK_BIT ) || 
							AppTestDebugFlag( ADF_FILL_SOUND_PAK_BIT ) || 
							AppTestDebugFlag( ADF_FILL_PAK_MIN_BIT ) ||
							AppTestDebugFlag( ADF_FILL_LOCALIZED_PAK_BIT ));
	SVR_VERSION_ONLY(return FALSE);
}

CCriticalSection csInitLock(TRUE);

CCriticalSection* AppGetSvrInitLock()
{
	return &csInitLock;
}

BOOL AppGetFillLoadXML_OK()
{
	return (AppGetAllowFillLoadXML() &&
		(AppGetState() == APP_STATE_INIT ||
		AppGetState() == APP_STATE_MAINMENU));
}

#if !ISVERSION(SERVER_VERSION)
void AppPatcherWaitingFunction()
{
#if !ISVERSION(SERVER_VERSION)
	if (AppIsTugboat() || AppIsHellgate()) {
		if ((AppGetState() == APP_STATE_LOADING) && 
			(gApp.m_dwMainThreadID == GetCurrentThreadId())) {
			AppLoadingScreenUpdate(TRUE);
		}
	}
#endif
}
#endif

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

static bool s_bDidPreInit = false;

BOOL PrimePreInitForGlobalStrings(HINSTANCE hInstance, LPSTR pCmdLine)
{
	PathSetup();
	CommonInit(hInstance, pCmdLine, NULL);		// this must come first
	ASSERT_RETFALSE(AppPreInit(hInstance, pCmdLine));
	StringTableInitForApp();
	ScriptInitEx();
	ASSERT_RETFALSE(sInitPakFile());
	CDefinitionContainer::Init();
	{
		EXCEL_LOAD_MANIFEST manifest;
		ASSERT_RETFALSE(ExcelInit(manifest));
		LanguageInitForApp( AppGameGet() );
		ASSERT_RETFALSE(ExcelLoadDatatable(DATATABLE_STRING_FILES));	// DATATABLE_GLOBAL_STRING depends on DATATABLE_STRING_FILES to initialize string tables
		ASSERT_RETFALSE(ExcelLoadDatatable(DATATABLE_GLOBAL_INDEX));
		ASSERT_RETFALSE(ExcelStringTablesLoadAll());					// DATATABLE_GLOBAL_STRING needs string tables loaded
		ASSERT_RETFALSE(ExcelLoadDatatable(DATATABLE_GLOBAL_STRING));
	}

	s_bDidPreInit = true;
	return true;
}

BOOL PrimeDidPreInitForGlobalStrings(void)
{
	return s_bDidPreInit;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

void AppSetCurrentLevel(
	int idLevel)
{
#if ! ISVERSION(SERVER_VERSION)
	if ( AppIsMultiplayer() )
	{
		if ( gApp.m_idCurrentLevel != idLevel )
			c_SaveAutomap( AppGetCltGame(), gApp.m_idCurrentLevel );
	}

	gApp.m_idCurrentLevel = idLevel;
#endif
}
