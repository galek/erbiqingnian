//----------------------------------------------------------------------------
// FILE: region.cpp
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Includes
//----------------------------------------------------------------------------
  // must be first for pre-compiled headers
#include "excel.h"
#include "excel_private.h"
#include "filepaths.h"
#include "language.h"
#include "prime.h"
#include "region.h"
#include "serverlist.h"
#include "sku.h"
#include "stringreplacement.h"
#include "config.h"

//----------------------------------------------------------------------------
// Globals
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
struct REGION_GLOBALS
{
	WORLD_REGION eRegionCurrent;			// the app is currently running in this region
};
static REGION_GLOBALS sgtRegionGlobals;
#define INSTALLER_REGION_FILE L"region.dat"

//----------------------------------------------------------------------------
static REGION_DATA sgtRegionTableHellgate[] = 
{
	// REGION						NAME,				DEFAULT URLARG  START LAUNCHER
	{ WORLD_REGION_NORTH_AMERICA,	"NorthAmerica",		TRUE,	"NA",	FALSE},
	{ WORLD_REGION_EUROPE,			"Europe",			FALSE,	"EU",	FALSE},
	{ WORLD_REGION_JAPAN,			"Japan",			FALSE,	"JP",	FALSE},
	{ WORLD_REGION_KOREA,			"Korea",			FALSE,	"KR",	TRUE},
	{ WORLD_REGION_CHINA,			"China",			FALSE,	"CN",	FALSE},
	{ WORLD_REGION_TAIWAN,			"Taiwan",			FALSE,	"TW",	FALSE},
	{ WORLD_REGION_SOUTHEAST_ASIA,	"SoutheastAsia",	FALSE,	"SE",	FALSE},
	{ WORLD_REGION_SOUTH_AMERICA,	"SouthAmerica",		FALSE,	"SA",	FALSE},
	{ WORLD_REGION_AUSTRALIA,		"Australia",		FALSE,	"AU",	FALSE},

	// keep this entry last	
	{ WORLD_REGION_INVALID,			NULL,				FALSE,	NULL,	NULL } // keep this last
	
};

//----------------------------------------------------------------------------
static REGION_DATA sgtRegionTableMythos[] = 
{
	// REGION						NAME,				DEFAULT, URL
	{ WORLD_REGION_NORTH_AMERICA,	"NorthAmerica",		TRUE,	"NA" },

	// keep this entry last	
	{ WORLD_REGION_INVALID,			NULL,				FALSE,	NULL } // keep this last
	
};

//----------------------------------------------------------------------------
// Definitions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
URL_REPLACEMENT_INFO::URL_REPLACEMENT_INFO( void )
	:	dwReplacementFlags( 0 ),
		eLanguage( LANGUAGE_INVALID ),
		eRegion( WORLD_REGION_INVALID )
{ 
	uszVersion[ 0 ] = 0;
	uszSPVersion[ 0 ] = 0;
	uszMPVersion[ 0 ] = 0;
	uszMPDataVersion[ 0 ] = 0;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static const REGION_DATA *sGetRegionTable(
	APP_GAME eAppGame)
{
	if (eAppGame == APP_GAME_HELLGATE)
	{
		return sgtRegionTableHellgate;
	}
	else if (eAppGame == APP_GAME_TUGBOAT)
	{
		return sgtRegionTableMythos;
	}
	
	return NULL;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const REGION_DATA *RegionGetData(
	APP_GAME eAppGame,
	WORLD_REGION eRegion)
{
	ASSERTX_RETNULL( eRegion > WORLD_REGION_INVALID && eRegion < WORLD_REGION_NUM_REGIONS, "Invalid region" );
	const REGION_DATA *pTable = sGetRegionTable( eAppGame );
	ASSERTX_RETNULL( pTable, "Expected region table" );

	for (const REGION_DATA *pRegionData = pTable; 
		 pRegionData && pRegionData->eRegion != WORLD_REGION_INVALID; 
		 ++pRegionData)
	{
		if (pRegionData->eRegion == eRegion)
		{
			return pRegionData;
		}
	}

	return NULL;
		
}	

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
const char *RegionGetDevName(
	APP_GAME eAppGame,
	WORLD_REGION eRegion)
{
	const REGION_DATA *pRegionData = RegionGetData( eAppGame, eRegion );
	ASSERTX_RETNULL( pRegionData, "Expected region data" );
	return pRegionData->szName;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
WORLD_REGION RegionGetByName( 
	APP_GAME eAppGame,
	const char *pszRegion)
{
	ASSERTX_RETVAL( pszRegion, WORLD_REGION_INVALID, "Expected region string" );
	ASSERTX_RETVAL( eAppGame != WORLD_REGION_INVALID, WORLD_REGION_INVALID, "Invalid region" );
	const REGION_DATA *pTable = sGetRegionTable( eAppGame );
	ASSERTX_RETVAL( pTable, WORLD_REGION_INVALID, "Expected region table" );

	for (const REGION_DATA *pRegionData = pTable; 
		 pRegionData && pRegionData->eRegion != WORLD_REGION_INVALID; 
		 ++pRegionData)
	{
		if (PStrICmp( pRegionData->szName, pszRegion ) == 0)
		{
			return pRegionData->eRegion;
		}
	}
	
	return WORLD_REGION_INVALID;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static WORLD_REGION sRegionGetInstallerSelection(
	void)
{
	WORLD_REGION eRegion = WORLD_REGION_INVALID;

	// construct path to file
	WCHAR uszRegionFile[ MAX_PATH ];
	PStrPrintf( uszRegionFile, MAX_PATH, L"%S%s", FILE_PATH_DATA, INSTALLER_REGION_FILE );

	// read the region from the file that the installer created
	const int MAX_TOKEN = 256;
	char szRegion[ MAX_TOKEN ];
	if (FileParseSimpleToken( uszRegionFile, "Region", szRegion, MAX_TOKEN ))
	{
		eRegion = RegionGetByName( AppGameGet(), szRegion );
	}

	return eRegion;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static WORLD_REGION sRegionGetDefault(
	APP_GAME eAppGame)
{
	const REGION_DATA *pTable = sGetRegionTable( eAppGame );
	ASSERTX_RETVAL( pTable, WORLD_REGION_INVALID, "Expected region table" );

	for (const REGION_DATA *pRegionData = pTable; 
		 pRegionData && pRegionData->eRegion != WORLD_REGION_INVALID; 
		 ++pRegionData)
	{
		if (pRegionData->bDefault == TRUE)
		{
			return pRegionData->eRegion;
		}
	}
	
	return WORLD_REGION_INVALID;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RegionInitForApp(
	void)
{
	if(AppIsHammer())
	{
		return;
	}

	// init region
	sgtRegionGlobals.eRegionCurrent = WORLD_REGION_INVALID;
	
	// read setting from installer selection
	WORLD_REGION eRegion = sRegionGetInstallerSelection();
	
	// if no region is set, try to use one from the SKU datatable (if possible)
	if (eRegion == WORLD_REGION_INVALID)
	{
		int nSKU = SKUGetCurrent();
		eRegion = SKUGetSingleRegion( nSKU );
	}

	// if no region set, use a default one
	if (eRegion == WORLD_REGION_INVALID)
	{
		eRegion = sRegionGetDefault( AppGameGet() );		
	}

	// set the current region
	ASSERTX( eRegion != WORLD_REGION_INVALID, "Unable to set region for app" );
	sgtRegionGlobals.eRegionCurrent = eRegion;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
WORLD_REGION RegionGetCurrent(
	void)
{
	return sgtRegionGlobals.eRegionCurrent;	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
void RegionGetURLArgument(
	APP_GAME eAppGame,
	WORLD_REGION eRegion,
	WCHAR *puszBuffer,
	int nBufferLen)
{
	#if ! ISVERSION(SERVER_VERSION)

	ASSERTX_RETURN( eRegion != WORLD_REGION_INVALID, "Expected region link" );
	ASSERTX_RETURN( puszBuffer, "Expected buffer" );
	ASSERTX_RETURN( nBufferLen, "Invalid buffer len" );
	
	// init result
	puszBuffer[ 0 ] = 0;
	
	const REGION_DATA *pRegionData = RegionGetData( eAppGame, eRegion );
	ASSERTX_RETURN( pRegionData, "Expected region data" );
	PStrCvt( puszBuffer, pRegionData->szURLArgument, nBufferLen );

	#else
		REF( eAppGame );
		REF( eRegion );
		REF( puszBuffer );
		REF( nBufferLen );	
	#endif //! SERVER_VERSION
}
		
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL RegionURLDoReplacements(	
	APP_GAME eAppGame,
	WCHAR * puszBuffer, 
	int nBufferLen,
	const URL_REPLACEMENT_INFO *pReplacementInfo)
{
#if !ISVERSION(SERVER_VERSION)
	ASSERTX_RETFALSE( puszBuffer, "Expected buffer" );
	ASSERTX_RETFALSE( nBufferLen, "Invalid buffer size" );
	ASSERTX_RETFALSE( pReplacementInfo, "Expected replacement info" );
	ASSERTX_RETFALSE( pReplacementInfo->eRegion != WORLD_REGION_INVALID, "Invalid region" );
	ASSERTX_RETFALSE( pReplacementInfo->eLanguage != LANGUAGE_INVALID, "Invalid language" );
	
	// do language replacements
	const int MAX_URL_ARGUMENT = 64;
	WCHAR uszLanguageArgument[ MAX_URL_ARGUMENT ];
	LanguageGetURLArgument( eAppGame, pReplacementInfo->eLanguage, uszLanguageArgument, MAX_URL_ARGUMENT );
	const WCHAR *puszLangToken = StringReplacementTokensGet( SR_LANGUAGE );
	ASSERTX_RETFALSE( puszLangToken, "Expected lang token" );
	PStrReplaceToken( puszBuffer, nBufferLen, puszLangToken, uszLanguageArgument );

	// do region replacements
	WCHAR uszRegionArgument[ MAX_URL_ARGUMENT ];
	RegionGetURLArgument( eAppGame, pReplacementInfo->eRegion, uszRegionArgument, MAX_URL_ARGUMENT );
	const WCHAR *puszRegionToken = StringReplacementTokensGet( SR_REGION );
	ASSERTX_RETFALSE( puszRegionToken, "Expected region token" );
	PStrReplaceToken( puszBuffer, nBufferLen, puszRegionToken, uszRegionArgument );
		
	// we apparently want to also replace the token "parameters" with "locale=[LANGUAGE_ARG]",
	// which seems kind of ugly to me
	const int MAX_PARAM_REPLACEMENT = 256;
	WCHAR uszCompleteLocaleArgument[ MAX_PARAM_REPLACEMENT ] = { 0 };
	PStrPrintf( uszCompleteLocaleArgument, MAX_PARAM_REPLACEMENT, L"locale=%s", uszLanguageArgument );
	const WCHAR *puszLocaleParametersToken = StringReplacementTokensGet( SR_LOCALE_PARAM );
	PStrReplaceToken( puszBuffer, nBufferLen, puszLocaleParametersToken, uszCompleteLocaleArgument );

	// do account name replacements
	const int MAX_ACCOUNT_NAME = 256;
	WCHAR uszAccountName[ MAX_ACCOUNT_NAME ];
	PStrCvt( uszAccountName, gApp.userName, MAX_ACCOUNT_NAME );	
	const WCHAR *puszAccountNameToken = StringReplacementTokensGet( SR_ACCOUNT_NAME );
	ASSERTX_RETFALSE( puszAccountNameToken, "Expected account name token" );		
	PStrReplaceToken( puszBuffer, nBufferLen, puszAccountNameToken, uszAccountName );

	// version
	if (pReplacementInfo->uszVersion[ 0 ])
	{
		const WCHAR *puszToken = StringReplacementTokensGet( SR_VERSION );
		ASSERTX_RETFALSE( puszToken, "Expected version token" );
		PStrReplaceToken( puszBuffer, nBufferLen, puszToken, pReplacementInfo->uszVersion );		
	}
	
	// sp version
	if (pReplacementInfo->uszSPVersion[ 0 ])
	{
		const WCHAR *puszToken = StringReplacementTokensGet( SR_SINGLEPLAYER_VERSION );
		ASSERTX_RETFALSE( puszToken, "Expected single player version token" );
		PStrReplaceToken( puszBuffer, nBufferLen, puszToken, pReplacementInfo->uszSPVersion );		
	}
	
	// mp version
	if (pReplacementInfo->uszMPDataVersion[ 0 ])
	{
		const WCHAR *puszToken = StringReplacementTokensGet( SR_MULTIPLAYER_VERSION );
		ASSERTX_RETFALSE( puszToken, "Expected multiplayer version token" );
		PStrReplaceToken( puszBuffer, nBufferLen, puszToken, pReplacementInfo->uszMPDataVersion );		
	}
	
	// mp data version
	if (pReplacementInfo->uszMPDataVersion[ 0 ])
	{
		const WCHAR *puszToken = StringReplacementTokensGet( SR_MULTIPLAYER_DATA_VERSION );
		ASSERTX_RETFALSE( puszToken, "Expected multiplayer data version token" );
		PStrReplaceToken( puszBuffer, nBufferLen, puszToken, pReplacementInfo->uszMPDataVersion );		
	}
	
	// make the whole thing staging if requested
	if (TESTBIT( pReplacementInfo->dwReplacementFlags, RURF_USE_STAGING_BIT ))
	{
		ServerListURLMakeStaging( puszBuffer, nBufferLen );	
	}
		
#else

	REF(eAppGame);
	REF(puszBuffer);
	REF(nBufferLen);
	REF(pReplacementInfo);
		
#endif

	return TRUE;
	
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static T_SERVER * sGetFirstServerForRegion(
	WORLD_REGION eRegion)
{
	ASSERTX_RETNULL( eRegion != WORLD_REGION_INVALID, "Expected region link" );

	T_SERVER *pBest = NULL;

//	const SERVERLIST_DEFINITION * pServerlist = DefinitionGetServerlist();
	const T_SERVERLIST * pServerlist = ServerListGet();
	if (pServerlist)	
	{
		for (int i=0; i < pServerlist->nRegionViewCount; i++)
		{
			if (pServerlist->pServerDefinitions[i].eRegion == eRegion)
			{
				return &(pServerlist->pServerDefinitions[i]);
			}
			if (pServerlist->pServerDefinitions[i].eRegion == WORLD_REGION_INVALID)
			{
				pBest = &(pServerlist->pServerDefinitions[i]);
			}
		}
		if (pBest)
			return pBest;
	}

//	const SERVERLISTOVERRIDE_DEFINITION * pServerlistOverride = DefinitionGetServerlistOverride();
	const T_SERVERLIST * pServerlistOverride = ServerListOverrideGet();
	if (pServerlistOverride)
	{
		for (int i=0; i < pServerlistOverride->nRegionViewCount; i++)
		{
			if (pServerlistOverride->pServerDefinitions[i].eRegion == eRegion)
			{
				return &(pServerlistOverride->pServerDefinitions[i]);
			}
			if (pServerlistOverride->pServerDefinitions[i].eRegion == WORLD_REGION_INVALID)
			{
				pBest = &(pServerlistOverride->pServerDefinitions[i]);
			}
		}
	}

	return pBest;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
static REGION_URL sGetRegionURLToUse( 
	REGION_URL eRegionURL, 
	BOOL bStaging)
{

	if (bStaging == TRUE)
	{
	
		switch (eRegionURL)
		{
		
			//----------------------------------------------------------------------------
			case RU_LAUNCHER_PATCH_SERVER:
			{
				return RU_LAUNCHER_PATCH_SERVER_STAGING;
			}
			
			//----------------------------------------------------------------------------		
			//case RU_LAUNCHER_PATCH_DATA_SERVER01:
			//	 RU_LAUNCHER_PATCH_DATA_SERVER02:
			//	 RU_LAUNCHER_PATCH_DATA_SERVER03:
			//	 RU_LAUNCHER_PATCH_DATA_SERVER04:
			//	 RU_LAUNCHER_PATCH_DATA_SERVER05:
			//	 RU_LAUNCHER_PATCH_DATA_SERVER06:
			//	 RU_LAUNCHER_PATCH_DATA_SERVER07:
			//	 RU_LAUNCHER_PATCH_DATA_SERVER08:
			//{
			//	return RU_LAUNCHER_PATCH_DATA_SERVER_STAGING;
			//}
			
		}

	}
	
	return eRegionURL;
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL RegionGetURL(
	APP_GAME eAppGame,
	REGION_URL eRegionURL,
	WORLD_REGION eRegion,
	WCHAR *puszBuffer,
	int nBufferLen,
	URL_REPLACEMENT_INFO *pReplacementInfo,
	BOOL bStaging /*= FALSE*/)
{
#if !ISVERSION(SERVER_VERSION)
	ASSERTX_RETFALSE( puszBuffer, "Expected buffer" );
	ASSERTX_RETFALSE( nBufferLen, "Invalid buffer size" );
	ASSERTX_RETFALSE( pReplacementInfo, "Expected URL replacement info" );	
	ASSERTX_RETFALSE( eRegion > WORLD_REGION_INVALID && eRegion < WORLD_REGION_NUM_REGIONS, "Invalid world region" );

	// for staging test environments, we swap out some URLS for other ones
	eRegionURL = sGetRegionURLToUse( eRegionURL, bStaging );
	
	T_SERVER *pServerDef = AppGetServerSelection();

	puszBuffer[0] = L'\0';
	if (pServerDef && pServerDef->szURLs[eRegionURL])
	{
		PStrCvt(puszBuffer, pServerDef->szURLs[eRegionURL], nBufferLen);
	}

	if (!puszBuffer[0])
	{
//		const SERVERLIST_DEFINITION * pServerlist = DefinitionGetServerlist();
		const T_SERVERLIST * pServerlist = ServerListGet();
		if (pServerlist)
		{
			for (int i=0; i < pServerlist->nRegionViewCount; i++)
			{
				T_REGION_VIEW &tRegionView = pServerlist->pRegionViews[i];
				if (tRegionView.eRegion == eRegion)
				{
					const char *pszURL = tRegionView.szURLs[eRegionURL];

					// if we got some text, use it
					if (pszURL && PStrLen( pszURL ) > 0)
					{
						PStrCvt(puszBuffer, pszURL, nBufferLen);
						break;
					}
					
				}
				
			}	
			
		}
		
	}

	// replace variables in the URL we're retrieving
	return RegionURLDoReplacements( eAppGame, puszBuffer, nBufferLen, pReplacementInfo );

#else
	REF( eAppGame );
	REF( eRegionURL );
	REF( eRegion );
	REF( puszBuffer );
	REF( nBufferLen );
	REF( pReplacementInfo );
	REF( bStaging );
	return FALSE;
#endif

}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL RegionCurrentGetURL(
	APP_GAME eAppGame,
	REGION_URL eRegionURL,
	WCHAR *puszBuffer,
	int nBufferLen)
{	
	WORLD_REGION eRegionCurrent = (WORLD_REGION)RegionGetCurrent();
	
	// setup replacement info
	URL_REPLACEMENT_INFO tReplaceInfo;
	tReplaceInfo.eLanguage = LanguageGetCurrent();
	tReplaceInfo.eRegion = eRegionCurrent;
	
	// get URL
	return RegionGetURL( 
		eAppGame, 
		eRegionURL, 
		eRegionCurrent,
		puszBuffer, 
		nBufferLen, 
		&tReplaceInfo,
		FALSE);	
		
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
BOOL RegionCurrentGetURL(
	REGION_URL eRegionURL,
	WCHAR *puszBuffer,
	int nBufferLen)
{
	return RegionCurrentGetURL( AppGameGet(), eRegionURL, puszBuffer, nBufferLen );
}
