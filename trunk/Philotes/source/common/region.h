//----------------------------------------------------------------------------
// FILE: region.h
//----------------------------------------------------------------------------

#ifndef __REGION_H_
#define __REGION_H_

//----------------------------------------------------------------------------
// Forward Declarations
//----------------------------------------------------------------------------
enum REGION_URL;
enum LANGUAGE;

//----------------------------------------------------------------------------
// Types
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
enum REGION_CONSTANTS
{
	REGION_MAX_URL_LENGTH = 2048,				// arbitrary
	REGION_MAX_URL_ARGUMENT_LENGTH  = 64,		// arbitrary
};

//----------------------------------------------------------------------------
enum WORLD_REGION
{
	WORLD_REGION_INVALID = -1,

	// Do *NOT* reorder/renumber, these are stored in excel tables, add new ones to end *ONLY*
	// unless you also cause the excel cooked data to rebuild
	WORLD_REGION_NORTH_AMERICA	=  0,			// do not renumber/reorder
	WORLD_REGION_EUROPE			=  1,			// do not renumber/reorder
	WORLD_REGION_JAPAN			=  2,			// do not renumber/reorder
	WORLD_REGION_KOREA			=  3,			// do not renumber/reorder
	WORLD_REGION_CHINA			=  4,			// do not renumber/reorder
	WORLD_REGION_TAIWAN			=  5,			// do not renumber/reorder
	WORLD_REGION_SOUTHEAST_ASIA	=  6,			// do not renumber/reorder
	WORLD_REGION_SOUTH_AMERICA	=  7,			// do not renumber/reorder
	WORLD_REGION_AUSTRALIA		=  8,			// do not renumber/reorder
	// Do *NOT* reorder/renumber, these are stored in excel tables, add new ones to end *ONLY*	
	// unless you also cause the excel cooked data to rebuild
	
	WORLD_REGION_NUM_REGIONS
	
};

//----------------------------------------------------------------------------
struct REGION_DATA
{
	WORLD_REGION eRegion;									// the region index
	char szName[ DEFAULT_INDEX_SIZE ];						// name of region
	BOOL bDefault;											// is default region 
	char szURLArgument[ REGION_MAX_URL_ARGUMENT_LENGTH  ];	// argument to use in URLs when a [REGION] variable needs to be replaced with a selected region
	BOOL bStartLauncherUponUpdate;
};

//----------------------------------------------------------------------------
enum REGION_URL_REPLACEMENT_FLAGS
{
	RURF_USE_STAGING_BIT,					// convert to staging URL for the test environment (ie. www.hellgatelondon.com becomes www-staging.hellgatelondon.com)
};

//----------------------------------------------------------------------------
struct URL_REPLACEMENT_INFO
{

	enum { URL_REPLACE_MAX_VERSION = 32 };
	
	DWORD dwReplacementFlags;	// see REGION_URL_REPLACEMENT_FLAGS
	LANGUAGE eLanguage;			// this is the language to use in replacements when it calls for it
	WORLD_REGION eRegion;		// the region to use for replacements
	WCHAR uszVersion[ URL_REPLACE_MAX_VERSION ];		// version (generic, like for anything or launcher or something)
	WCHAR uszSPVersion[ URL_REPLACE_MAX_VERSION ];		// SP version for replacements
	WCHAR uszMPVersion[ URL_REPLACE_MAX_VERSION ];		// MP version for replacements
	WCHAR uszMPDataVersion[ URL_REPLACE_MAX_VERSION ];	// MP data version for replacements

	URL_REPLACEMENT_INFO::URL_REPLACEMENT_INFO( void );  // constructor
		
};

//----------------------------------------------------------------------------
// Function Prototypes
//----------------------------------------------------------------------------

const REGION_DATA *RegionGetData(
	APP_GAME eAppGame,
	WORLD_REGION eRegion);

const char *RegionGetDevName(
	APP_GAME eAppGame,
	WORLD_REGION eRegion);
	
void RegionInitForApp(
	void);
	
WORLD_REGION RegionGetCurrent(
	void);

BOOL RegionGetURL(
	APP_GAME eGame,
	REGION_URL eRegionURL,
	WORLD_REGION eRegion,	
	WCHAR *puszBuffer,
	int nBufferLen,
	URL_REPLACEMENT_INFO *pReplacementInfo,
	BOOL bStaging /*= FALSE*/);
		
BOOL RegionCurrentGetURL(
	APP_GAME eAppGame,
	REGION_URL eRegionURL,
	WCHAR *puszBuffer,
	int nBufferLen);

BOOL RegionCurrentGetURL(
	REGION_URL eRegionURL,
	WCHAR *puszBuffer,
	int nBufferLen);

WORLD_REGION RegionGetByName( 
	APP_GAME eAppGame,
	const char *pszRegion);

void RegionGetURLArgument(
	APP_GAME eAppGame,
	WORLD_REGION eRegion,
	WCHAR *puszBuffer,
	int nBufferLen);

BOOL RegionURLDoReplacements(
	APP_GAME eAppGame,
	WCHAR *puszBuffer, 
	int nBufferLen,
	const URL_REPLACEMENT_INFO *pReplacementInfo);
	
#endif