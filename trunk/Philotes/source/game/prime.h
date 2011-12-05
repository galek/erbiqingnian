//----------------------------------------------------------------------------
// prime.h
//
// (C)Copyright 2003, Flagship Studios. All rights reserved.
//----------------------------------------------------------------------------
#ifdef	_PRIME_H_
//#pragma message ("    HINT: " __FILE__ " included multiple times in this translation unit")
#else
#define _PRIME_H_


//----------------------------------------------------------------------------
// INLUDES
//----------------------------------------------------------------------------
#ifndef _APPCOMMONTIMER_H_
#include "appcommontimer.h"
#endif

#ifndef _NETCLIENT_H_
#include "netclient.h"
#endif

#ifndef _FREELIST_H_
#include "freelist.h"
#endif

#ifndef _ROTLIST_H_
#include "rotlist.h"
#endif

#ifndef _MAILSLOT_H_
#include "mailslot.h"
#endif

#ifndef _CHARACTERLIMIT_H_
#include "characterlimit.h"
#endif



//----------------------------------------------------------------------------
// CONSTANTS
//----------------------------------------------------------------------------

#define EMPTY_GAME_SLOT					((struct GAME*)-1)

#define ENV_TYPE_CLIENT					0
#define ENV_TYPE_SERVER					1

#define DEBUG_FLAG_NONE					0x00000000


#ifdef _SERVER
  #define MAX_GAMES_PER_SERVER			MAX_CHARACTERS_PER_GAME_SERVER*2
  #define MAX_CLIENTS					MAX_CHARACTERS_PER_GAME_SERVER
#else
  #define MAX_GAMES_PER_SERVER			128
  #define MAX_CLIENTS					64
#endif 

#define MAX_TEAM_NAME					32
#define MAX_TEAMS_PER_GAME				256
#define DEFAULT_MAX_PLAYERS_PER_GAME	128


#ifdef _WIN64
#	define APP_PLATFORM_NAME		"x64"
#else	// x86
#	define APP_PLATFORM_NAME		"x86"
#endif


//----------------------------------------------------------------------------
// ENUMERATIONS
//----------------------------------------------------------------------------

enum APP_STATE
{
	APP_STATE_INIT,
	APP_STATE_PLAYMOVIELIST,
	APP_STATE_MAINMENU,
	APP_STATE_MENUERROR,
	APP_STATE_CHARACTER_CREATE,
	APP_STATE_CONNECTING_START,
	APP_STATE_CONNECTING_PATCH,
	APP_STATE_CONNECTING,
	APP_STATE_LOADING,
	APP_STATE_IN_GAME,
	APP_STATE_RESTART,
	APP_STATE_EXIT,
	APP_STATE_FILLPAK,
	APP_STATE_DUMPSKILLDESCS,
	APP_STATE_CREDITS,
};

//----------------------------------------------------------------------------
// TYPEDEFS
//----------------------------------------------------------------------------
typedef void (*FN_GAME_LOOP)(unsigned int sim_frames);
typedef void (*FN_PROCESS_SERVER)(unsigned int sim_frames);
typedef BOOL (*FN_PLAYER_SAVE)(struct UNIT * unit);


//----------------------------------------------------------------------------
// STRUCTS
//----------------------------------------------------------------------------

struct CHANNEL;
struct SERVER_DEFINITION;
struct T_SERVER;
struct T_SERVERLIST;

struct GAME_SETTINGS
{
	int								m_nExpBonus;
};


#define MAX_USERNAME_LEN 20			// must fit within HBS billing system 40 char limit.  allow for wide char names.
#if !ISVERSION(SERVER_VERSION)
struct APP
{
//	SERVER_DEFINITION *				pServerSelection;
	T_SERVER *						pServerSelection;
	char							szAuthenticationIP[MAX_NET_ADDRESS_LEN];
	char							szServerIP[MAX_NET_ADDRESS_LEN];
	char							szLocalIP[MAX_NET_ADDRESS_LEN];	// local client interface to use
    unsigned short                  serverPort;

	char                            userName[MAX_USERNAME_LEN];
    char                            passWord[MAX_USERNAME_LEN];

	APP_TYPE						eStartupAppType;				// app type determined by cmd line
	TIME							m_tiAppWaitTime;

#if ISVERSION(DEVELOPMENT)
	DWORD							m_dwAppDebugFlags;				// see APP_DEBUG_FLAG enum
	unsigned int					m_nStatsDebugStart;				// for /statsdebug cheat
	BOOL							m_bKeyHandlerLogging;			// logs when keys are handled (or blocked) to the screen
	char							m_szSKUOverride[DEFAULT_INDEX_SIZE];// override the SKU
#endif

	struct CONSOLE *				m_pConsole;

	BOOL							m_bWSAStartup;					// have we called WSAStartup
	BOOL							m_bForceSynch;
	BOOL							m_bNoSound;
	BOOL							m_bSkipMenu;					// skip main menu?
	BOOL							m_bPatching;
	BOOL							m_bAllowPatching;
	BOOL							m_bAllowFillLoadXML;

	FN_GAME_LOOP					m_fpGameLoop;
	FN_PROCESS_SERVER				m_fpProcessServer;
	FN_PLAYER_SAVE					m_fpPlayerSave;

	struct CHANNEL *                m_gameChannel;
	struct CHANNEL *                m_chatChannel;
	struct CHANNEL *                m_billingChannel;
	struct CHANNEL *                m_patchChannel;
#if ISVERSION(DEVELOPMENT)
	struct CHANNEL *                m_fillpakChannel;
#endif

	GAMECLIENTID					m_idRemoteClient;				// my game client id on the server
	struct BADGE_COLLECTION *		m_pRemoteBadgeCollection;		// my badges on the server

	PGUID							m_characterGuid;				// guid for chat communications
	WCHAR							m_wszPlayerName[MAX_CHARACTER_NAME];// easy access to name for chat communication
	CHANNELID						m_idLevelChatChannel;			// chat channel for level-wide chat
	WCHAR							m_wszLastWhisperSender[MAX_CHARACTER_NAME];

    MAILSLOT                        m_GameMailSlot;
    MAILSLOT                        m_ChatMailSlot;
    MAILSLOT                        m_BillingMailSlot;
    MAILSLOT                        m_PatchMailSlot;
#if ISVERSION(DEVELOPMENT)
	MAILSLOT						m_FillPakMailSlot;
#endif

	NET_COMMAND_TABLE *				m_GameClientCommandTable;		// client to game server messages
	NET_COMMAND_TABLE *				m_GameServerCommandTable;		// game server to client messages
	NET_COMMAND_TABLE *				m_GameLoadBalanceClientCommandTable;	// client to loadbalance server messages
	NET_COMMAND_TABLE *				m_GameLoadBalanceServerCommandTable;	// loadbalance server to client messages
	NET_COMMAND_TABLE *				m_ChatClientCommandTable;		// client to chat server messages
	NET_COMMAND_TABLE *				m_ChatServerCommandTable;		// chat server to client messages
	NET_COMMAND_TABLE *				m_BillingClientCommandTable;	// client to billing server messages
	NET_COMMAND_TABLE *				m_BillingServerCommandTable;	// billing server to client messages
	NET_COMMAND_TABLE *				m_PatchClientCommandTable;		// client to chat server messages
	NET_COMMAND_TABLE *				m_PatchServerCommandTable;		// chat server to client messages
#if ISVERSION(DEVELOPMENT)
	NET_COMMAND_TABLE *				m_FillPakClientCommandTable;		// client to fillpak server messages
	NET_COMMAND_TABLE *				m_FillPakServerCommandTable;		// fillpak server to client messages
#endif

	HANDLE							m_hPatchInitEvent;
	TCHAR							m_fnameDelete[DEFAULT_FILE_WITH_PATH_SIZE+1];
	BOOL							m_bHasDelete;

	struct GROUP *					m_GroupList;

	APP_STATE						m_eAppState;
	APP_STATE						m_eAppStateBeforeMovie;

	struct GAME *					pClientGame;
	GAMEID							m_idSelectedGame;

	LEVELID							m_idCurrentLevel;				// current level id

	DWORD							m_SaveCRC;
	DWORD							m_SaveSize;						// client save file for open characters
	DWORD							m_SaveCur;
	BYTE *							m_SaveFile;

	WCHAR							m_wszSavedFile[260];			// String for loading directly to saved game file
	DWORD							m_dwMainThreadID;

	int								m_nCurrentMovieList;
	int								m_nCurrentMovieListIndex;
	int								m_nCurrentMovieListSet;

	BOOL							m_bEulaHashReady;
	DWORD							m_iOldEulaHash;
	DWORD							m_iNewEulaHash;

};


//----------------------------------------------------------------------------
// EXTERNS
//----------------------------------------------------------------------------
extern APP gApp;

#endif	// !ISVERSION(SERVER_VERSION)


extern GAME_SETTINGS gGameSettings;
struct DATABASE_UNIT_COLLECTION;
struct AUCTION_ITEMSALE_INFO;
//----------------------------------------------------------------------------
// Server data
//----------------------------------------------------------------------------
struct GameServerContext
{
	//	server info
	MEMORYPOOL *								m_pMemPool;
	UINT32										m_idServer;							// actually stores a SVRID
	volatile BOOL								m_shouldExit;
	HANDLE										m_mainThread;						// entry point thread in server version
	volatile LONG								m_outstandingDbRequests;			// number of db requests still pending
	volatile LONG								m_clients;							// number of clients logged on: attaches minus removes

	//	game members
	LAZY_FREELIST<NETCLT_USER>					m_clientFreeList;					// free list of player net contexts
	CRotList_MT<NETCLT_USER *,
		MAX_CLIENTS, NULL>						m_clientFinishedList[2];			// list of disconnected net client contexts to clear
	BYTE										m_bListSwap;						// putting in a one cycle delay on deletion, by swapping lists				

	NET_COMMAND_TABLE *							m_GameServerCommandTable;			// game server command table

	struct GAMELIST *							m_GameList;							// game list
	struct MINITOWN *							m_MiniTown;							// mini towns
	struct SERVER_MAILBOX *						m_pPlayerToGameServerMailBox;		// server mailbox for game less players
	struct CLIENTSYSTEM *						m_ClientSystem;						// server clients
	struct PARTY_CACHE *						m_PartyCache;						// chat party info cache
	class DatabaseCounter *						m_DatabaseCounter;					// counts database operations per player
	class GuidTracker *							m_GuidTracker;						// tracks guids of units to check for dupes.

	struct ALL_OPEN_LEVELS *					m_pAllOpenLevels;					// open levels that we can try to put people into when joining games if we want
	
	BOOL										m_chatInitializedForApp;
#if ISVERSION(SERVER_VERSION)	
	struct PERFCOUNTER_INSTANCE_GameServer *	m_pPerfInstance;
	struct TOWN_INSTANCE_LIST *					m_pInstanceList;
	UINT64										m_nLastTownInstanceUpdateTimer;

	GAMEID										m_idUtilGameInstance;
	CCriticalSection							m_csUtilGameInstance;

	APPCLIENTID									m_idAppCltAuctionHouse;
	UNIQUE_ACCOUNT_ID							m_idAccountAuctionHouse;
	PGUID										m_idCharAuctionHouse;

	SIMPLE_DYNAMIC_ARRAY<AUCTION_ITEMSALE_INFO>	m_listAuctionItemSales;
	DWORD										m_dwAuctionHouseFeeRate;
	DWORD										m_dwAuctionHouseMaxItemSaleCount;
	CCriticalSection							m_csAuctionItemSales;

	DWORD										m_dwLastEmailCheckTick;
	struct FREELIST<DATABASE_UNIT_COLLECTION> * m_DatabaseUnitCollectionList;
#endif
};


//----------------------------------------------------------------------------
// FUNCTIONS
//----------------------------------------------------------------------------
void PrimeInitAppNameAndMutexGlobals(
	LPSTR cmdline);

BOOL AppPreInit(
	HINSTANCE hInstance,
	const char * cmdline);

BOOL AppInit(
	HINSTANCE hInstance,
	const char * cmdline);

void AppGenericDo(
	void);

void PathSetup(
	void);

HINSTANCE AppGetHInstance(
	void);

HWND AppCommonGetHWnd(
	void);

APP_TYPE AppGetType(
	void);

void AppSetType(
	APP_TYPE eAppType);

BOOL AppIsSinglePlayer(
	void);

BOOL AppIsMultiplayer(
	void);

void AppPause(
	BOOL bPause);

void AppUserPause(
	BOOL bPause);
	
#if !ISVERSION(SERVER_VERSION)
GAME* AppGetCltGame(
	void);
void AppSetCltGame(
	GAME *pGame);
#else
inline struct GAME *AppGetCltGame(void)
{
//	ASSERT_RETNULL(0);
    return NULL;
}
#endif

void AppInitCltGame(
	GAMEID idGame,
	DWORD dwCurrentTick,
	unsigned int nGameSubtype = 0,
	DWORD dwGameFlags = 0,
	struct GAME_VARIANT *pGameVariant = NULL);

void AppClose(
	void);

void AppSwitchState( 
	APP_STATE eState,
	... );

void AppSetErrorState(
	enum GLOBAL_STRING eGSTitle,
	enum GLOBAL_STRING eGSError,
	APP_STATE eNextAppState = APP_STATE_RESTART,
	BOOL bExitGame = FALSE);

void AppSetErrorStateLocalized(
	const WCHAR *puszTitle,
	const WCHAR *puszError,
	APP_STATE eNextAppState = APP_STATE_RESTART,
	BOOL bExitGame = FALSE,
	struct DIALOG_CALLBACK *pCallback = NULL,
	BOOL bShowCancel = FALSE,
	struct DIALOG_CALLBACK *pCanelCallback = NULL);

APP_STATE AppGetState( 
	void);

APP_STATE AppGetStateBeforeMovie( 
	void);

void AppStartOrStopBackgroundMovie(
	void);

void AppSingleStep(
	void);

BOOL AppGetDetachedCamera(
	void);

void AppSetDetachedCamera(
	BOOL value);

void AppDetachedCameraUpdate(
	void);
#if !ISVERSION(SERVER_VERSION)
BOOL InitGraphics(
	HWND hWnd,
	void (*pfn_ToolUpdate)() = NULL,
	void (*pfn_ToolRender)() = NULL,
	void (*pfn_ToolDeviceLost)() = NULL,
	const class OptionState * pOverrideState = NULL );

void DestroyGraphics();
#endif
struct STARTGAME_PARAMS;

void AppSetTypeByStartData(
	const STARTGAME_PARAMS & tData);

BOOL AppIsDevelopmentTest(
	void);

void AppSetFlag(
	APP_FLAG eFlag,
	BOOL bValue);
	
BOOL AppTestFlag(
	APP_FLAG eFlag);

BOOL AppToggleFlag(
	APP_FLAG eFlag);
	
void AppLoadingScreenUpdate(
	BOOL bForce = FALSE);

BOOL AppIsSaving(
	void);

BOOL AppPlayerSave(
	struct UNIT * unit);

UINT64 AppGenerateGUID(
	UINT32 serverId );

DWORD AppGetSystemRAM();

BOOL AppGetNoSound();

BOOL AppIsFillingPak();

void AppPatcherWaitingFunction();

const struct BADGE_COLLECTION *AppGetBadgeCollection(void);

void AppSetBadgeCollection(BADGE_COLLECTION &tBadges);

void FileLoadUpdateLoadingScreenCallback();

#if ISVERSION(SERVER_VERSION)
struct TOWN_INSTANCE_LIST * AppGetTownInstanceList(void);
#endif

//----------------------------------------------------------------------------
struct CLIENTSYSTEM * AppGetClientSystem(
	void);

//----------------------------------------------------------------------------
GAMEID AppGetSrvGameId(
	void);

//----------------------------------------------------------------------------
void AppSetSrvGameId(
	GAMEID idGame);

#if !ISVERSION(SERVER_VERSION)
//----------------------------------------------------------------------------
void AppSetCurrentLevel(
	int idLevel);

//----------------------------------------------------------------------------
// INLINE FUNCTIONS
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
inline LEVELID AppGetCurrentLevel(
	void)
{
	return gApp.m_idCurrentLevel;
}

inline char * AppGetAuthenticationIp(void)
{
   return &gApp.szAuthenticationIP[0]; 
}
inline T_SERVER* AppGetServerSelection(void)
{
   return gApp.pServerSelection; 
}
inline char * AppGetServerIp(void)
{
   return &gApp.szServerIP[0]; 
}
inline unsigned short AppGetServerPort(void)
{
   return gApp.serverPort;
}
inline CHANNEL *AppGetGameChannel(void)
{
    return gApp.m_gameChannel;
}
inline void AppSetGameChannel(CHANNEL* channel)
{
    gApp.m_gameChannel = channel;
}
inline void AppSetChatChannel(CHANNEL* channel)
{
    gApp.m_chatChannel = channel;
}
inline void AppSetBillingChannel(CHANNEL* channel)
{
    gApp.m_billingChannel = channel;
}
inline void AppSetPatchChannel(CHANNEL* channel)
{
    gApp.m_patchChannel = channel;
}
#if ISVERSION(DEVELOPMENT)
inline void AppSetFillPakChannel(CHANNEL* channel)
{
	gApp.m_fillpakChannel = channel;
}
#endif
inline CHANNEL *AppGetChatChannel(void)
{
    return gApp.m_chatChannel;
}
inline CHANNEL *AppGetBillingChannel(void)
{
    return gApp.m_billingChannel;
}
inline CHANNEL *AppGetPatchChannel(void)
{
	return gApp.m_patchChannel;
}
#if ISVERSION(DEVELOPMENT)
inline CHANNEL *AppGetFillPakChannel(void)
{
	return gApp.m_fillpakChannel;
}
#endif
inline MAILSLOT *AppGetGameMailSlot(void)
{
    return &gApp.m_GameMailSlot;
}
inline MAILSLOT *AppGetChatMailSlot(void)
{
    return &gApp.m_ChatMailSlot;
}
inline MAILSLOT *AppGetBillingMailSlot(void)
{
    return &gApp.m_BillingMailSlot;
}
inline MAILSLOT *AppGetPatchMailSlot(void)
{
	return &gApp.m_PatchMailSlot;
}
#if ISVERSION(DEVELOPMENT)
inline MAILSLOT *AppGetFillPakMailSlot(void)
{
	return &gApp.m_FillPakMailSlot;
}
#endif
inline HANDLE AppGetPatchInitEvent(void)
{
	return gApp.m_hPatchInitEvent;
}

inline VOID AppSetAllowPatching(void)
{
	gApp.m_bAllowPatching = TRUE;
}

inline void AppSetOldEulaHash(DWORD iOldEulaHash)
{
	gApp.m_iOldEulaHash = iOldEulaHash;
	InterlockedExchange((LONG*)&gApp.m_bEulaHashReady, 1);
}

inline BOOL AppGetOldEulaHash(DWORD& iOldEulaHash)
{
	iOldEulaHash = gApp.m_iOldEulaHash;
	return gApp.m_bEulaHashReady;
}

inline void AppSetNewEulaHash(DWORD iNewEulaHash)
{
	gApp.m_iNewEulaHash = iNewEulaHash;
}

inline void AppGetNewEulaHash(DWORD& iNewEulaHash)
{
	iNewEulaHash = gApp.m_iNewEulaHash;
}

#else	// !SERVER_VERSION

inline CHANNEL *AppGetPatchChannel(void)
{
	return NULL;
}
#endif // !SERVER_VERSION

inline VOID AppSetAllowFillLoadXML(
	BOOL bAllow)
{
	UNREFERENCED_PARAMETER(bAllow);
	CLT_VERSION_ONLY(gApp.m_bAllowFillLoadXML = bAllow;)
}

inline BOOL AppGetAllowFillLoadXML()
{
#if !ISVERSION(SERVER_VERSION)
	if (gApp.m_bAllowFillLoadXML == FALSE) 
	{
		return FALSE;
	}
	switch(AppGetState()) 
	{
		case APP_STATE_INIT:
		case APP_STATE_PLAYMOVIELIST:
		case APP_STATE_MAINMENU:
			return TRUE;
		default:
			break;
	}
	return FALSE;
#else
	return FALSE;
#endif
}

BOOL AppGetFillLoadXML_OK(
	void);

inline BOOL AppGetAllowPatching(
	void)
{
	return AUTO_VERSION(gApp.m_bAllowPatching, FALSE);
}


//----------------------------------------------------------------------------
inline BOOL AppGetForceSynch(
	void)
{
	// drb.path - disabled for the time being
	return FALSE;
	//return gApp.m_bForceSynch;
}

//----------------------------------------------------------------------------
inline BOOL AppIsPatching()
{
	SVR_VERSION_ONLY(return FALSE);
	CLT_VERSION_ONLY(return gApp.m_bPatching);
}


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
inline BOOL AppIsHammer(
	void)
{
#if ISVERSION(SERVER_VERSION)
	return FALSE;
#else
	return c_GetToolMode();
#endif
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
void PrimeGameExplorerAdd(
	BOOL bDisplayInfo);

//----------------------------------------------------------------------------
const TCHAR * AppGetName(
	void);

//----------------------------------------------------------------------------
CCriticalSection* AppGetSvrInitLock();

//----------------------------------------------------------------------------
BOOL ShouldSoundBePaused();

void DoneMainMenu(
	BOOL bStartGame,
	const struct STARTGAME_PARAMS & tData);

void AppPlayMenuMusic(
	void);

#endif // _PRIME_H_
