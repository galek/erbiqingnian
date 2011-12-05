//----------------------------------------------------------------------------
// FILE: serverlist.h
//----------------------------------------------------------------------------

#ifndef __SERVER_LIST_H_
#define __SERVER_LIST_H_

//#define UNICODE_SERVERLIST

//----------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
enum REGION_URL
{	
	RU_INVALID = -1,
	
	RU_ACCOUNT_MANAGE_LOGIN,			// in game authenticated token
	RU_ACCOUNT_MANAGE_NAME,				// page to login to and manage my account
	RU_ACCOUNT_CREATE,					// page to create a new account
	RU_ACCOUNT_PAYMENT,					// page to login to make a payment
	RU_PETITION_SUPPORT,				// page to ask for support
	RU_CUSTOMER_SUPPORT,				// page for customer support information
	RU_ORDER_GAME,						// page to order game
	RU_MAIN_WEBSITE,					// main website
	RU_LAUNCHER_WEB_FRAME,				// the launcher web frame (gold master, we keep this around for legacy support reasons)
	RU_LAUNCHER_WEB_FRAME2,				// the Launcher web frame 2
	RU_DOWNLOAD_UPDATES,				// where we have patches that people can manually download and install

	// patch servers
	RU_LAUNCHER_PATCH_SERVER,			// the patch server that the launcher looks at for available updates to launcher/sp/mp
	RU_LAUNCHER_PATCH_SERVER_STAGING,	// the test environment for the launcher patch server	

	// patch data servers
	RU_LAUNCHER_PATCH_DATA_SERVER_FIRST,
	RU_LAUNCHER_PATCH_DATA_SERVER_01 = RU_LAUNCHER_PATCH_DATA_SERVER_FIRST,	// patch data server 1
	RU_LAUNCHER_PATCH_DATA_SERVER_02,										// patch data server 2
	RU_LAUNCHER_PATCH_DATA_SERVER_03,										// patch data server 3
	RU_LAUNCHER_PATCH_DATA_SERVER_04,										// patch data server 4
	RU_LAUNCHER_PATCH_DATA_SERVER_05,										// patch data server 5
	RU_LAUNCHER_PATCH_DATA_SERVER_06,										// patch data server 6
	RU_LAUNCHER_PATCH_DATA_SERVER_07,										// patch data server 7
	RU_LAUNCHER_PATCH_DATA_SERVER_08,										// patch data server 8
	RU_LAUNCHER_PATCH_DATA_SERVER_LAST = RU_LAUNCHER_PATCH_DATA_SERVER_08,
	RU_LAUNCHER_PATCH_DATA_SERVER_STAGING,									// patch data sever for test environment

	RU_NUM_URLS							// keep this last please
	
};

static const char * s_szServerListURLTagNames[RU_NUM_URLS] =
{
	"szURLAccountManageLogin",			// RU_ACCOUNT_MANAGE_LOGIN
	"szURLAccountManageName",			// RU_ACCOUNT_MANAGE_NAME
	"szURLAccountCreate",				// RU_ACCOUNT_CREATE
	"szURLAccountPayment",				// RU_ACCOUNT_PAYMENT
	"szURLPetitionSupport",				// RU_PETITION_SUPPORT
	"szURLCustomerSupport",				// RU_CUSTOMER_SUPPORT
	"szURLOrderGame",					// RU_ORDER_GAME
	"szURLMainWebsite",					// RU_MAIN_WEBSITE
	"szURLLauncherWebFrame",			// RU_LAUNCHER_WEB_FRAME (we keep this around for legacy support reasons)
	"szURLLauncherWebFrame2",			// RU_LAUNCHER_WEB_FRAME2
	"szURLDownloadUpdates",				// RU_DOWNLOAD_UPDATES
	"szURLLauncherPatchServer",			// RU_LAUNCHER_PATCH_SERVER
	"szURLLauncherPatchServerStaging",	// RU_LAUNCHER_PATCH_SERVER_STAGING
	"szURLLauncherPatchDataServer01",		// RU_LAUNCHER_PATCH_DATA_SERVER_01
	"szURLLauncherPatchDataServer02",		// RU_LAUNCHER_PATCH_DATA_SERVER_02
	"szURLLauncherPatchDataServer03",		// RU_LAUNCHER_PATCH_DATA_SERVER_03
	"szURLLauncherPatchDataServer04",		// RU_LAUNCHER_PATCH_DATA_SERVER_04
	"szURLLauncherPatchDataServer05",		// RU_LAUNCHER_PATCH_DATA_SERVER_05
	"szURLLauncherPatchDataServer06",		// RU_LAUNCHER_PATCH_DATA_SERVER_06
	"szURLLauncherPatchDataServer07",		// RU_LAUNCHER_PATCH_DATA_SERVER_07
	"szURLLauncherPatchDataServer08",		// RU_LAUNCHER_PATCH_DATA_SERVER_08
	"szURLLauncherPatchDataServerStaging",	// RU_LAUNCHER_PATCH_DATA_SERVER_STAGING
};										

//----------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------

#ifdef UNICODE_SERVERLIST
	#define SERVER_LIST_FILENAME					 L"serverlist.xml"			// used to be .uni.xml, but making it the same for simplicity
	#define SERVER_LIST_FILENAME_SB					  "serverlist.xml"			// used to be .uni.xml, but making it the same for simplicity
	#define SERVER_LIST_OVERRIDE_FILENAME			 L"serverlistoverride.xml"	// used to be .uni.xml, but making it the same for simplicity
	#define SERVER_LIST_OVERRIDE_FILENAME_SB		  "serverlistoverride.xml"	// used to be .uni.xml, but making it the same for simplicity
#else
	#define SERVER_LIST_FILENAME					 L"serverlist.xml"
	#define SERVER_LIST_FILENAME_SB					  "serverlist.xml"
	#define SERVER_LIST_OVERRIDE_FILENAME			 L"serverlistoverride.xml"
	#define SERVER_LIST_OVERRIDE_FILENAME_SB		  "serverlistoverride.xml"
#endif	

#define SERVER_LIST_HELLGATE					 L"data\\"##SERVER_LIST_FILENAME
#define SERVER_LIST_HELLGATE_T			(TCHAR *) "data\\"##SERVER_LIST_FILENAME_SB
#define SERVER_LIST_OVERRIDE_HELLGATE			 L"data\\"##SERVER_LIST_OVERRIDE_FILENAME
#define SERVER_LIST_OVERRIDE_HELLGATE_T	(TCHAR *) "data\\"##SERVER_LIST_OVERRIDE_FILENAME_SB
#define SERVER_LIST_MYTHOS						 L"data_mythos\\"##SERVER_LIST_FILENAME
#define SERVER_LIST_MYTHOS_T			(TCHAR *) "data_mythos\\"##SERVER_LIST_FILENAME_SB
#define SERVER_LIST_OVERRIDE_MYTHOS				 L"data_mythos\\"##SERVER_LIST_OVERRIDE_FILENAME
#define SERVER_LIST_OVERRIDE_MYTHOS_T	(TCHAR *) "data_mythos\\"##SERVER_LIST_OVERRIDE_FILENAME_SB

//----------------------------------------------------------------------------
// ServerList Structs
//----------------------------------------------------------------------------

struct T_SERVER
{
	char				szOperationalName[MAX_XML_STRING_LENGTH];
#ifdef UNICODE_SERVERLIST
	WCHAR				wszCommonName[MAX_XML_STRING_LENGTH];
#else
	char				wszCommonName[MAX_XML_STRING_LENGTH];
#endif
	char				szIP[MAX_XML_STRING_LENGTH];
	enum WORLD_REGION	eRegion;
	BOOL				bAllowRandomSelect;

	char				szURLs[RU_NUM_URLS][MAX_XML_STRING_LENGTH];
};

struct T_REGION_VIEW
{
	enum WORLD_REGION	eRegion;

	int					nViewRegionCount;
	WORLD_REGION *		peViewRegions;

	char				szURLs[RU_NUM_URLS][MAX_XML_STRING_LENGTH];
};

struct T_SERVERLIST
{
	int					nServerDefinitionCount;
	T_SERVER *			pServerDefinitions;

	int					nRegionViewCount;
	T_REGION_VIEW *		pRegionViews;
};

struct T_SERVERLISTOVERRIDE
{
	int					nServerDefinitionCount;
	T_SERVER *			pServerDefinitions;

	int					nRegionViewCount;
	T_REGION_VIEW *		pRegionViews;
};

//----------------------------------------------------------------------------
// Prototypes
//----------------------------------------------------------------------------

void ServerListURLMakeStaging(
	WCHAR *puszBuffer,
	int nBufferLen);

const char *ServerListURLGetTag(
	REGION_URL eRegionURL,
	BOOL bUseStaging = FALSE);

BOOL ServerListLoad(
	const WCHAR *puszServerListFilename,
	const WCHAR *puszServerListOverrideFilename = NULL);

BOOL ServerListLoad(
	struct GAME *pGame);

void ServerListFree(
	void);

T_SERVERLIST * ServerListGet(
	void);

T_SERVERLIST * ServerListOverrideGet(
	void);

#endif
